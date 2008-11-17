/*
 * @(#)hprof_listener.c	1.11 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER  
 *   
 * This program is free software; you can redistribute it and/or  
 * modify it under the terms of the GNU General Public License version  
 * 2 only, as published by the Free Software Foundation.   
 *   
 * This program is distributed in the hope that it will be useful, but  
 * WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  
 * General Public License version 2 for more details (a copy is  
 * included at /legal/license.txt).   
 *   
 * You should have received a copy of the GNU General Public License  
 * version 2 along with this work; if not, write to the Free Software  
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  
 * 02110-1301 USA   
 *   
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa  
 * Clara, CA 95054 or visit www.sun.com if you need additional  
 * information or have any questions. 
 *
 */

#include "hprof.h"

static jint 
recv_fully(int f, char *buf, int len)
{
    int nbytes = 0;
    while (nbytes < len) {
        int res = recv(f, buf + nbytes, len - nbytes, 0);
	if (res < 0) {
	    if (hprof_is_on) {
	        hprof_is_on = FALSE;
	        fprintf(stderr,
			"HPROF ERROR: failed to read cmd from socket\n");
	    }
	    CALL(ProfilerExit)((jint)1);
	}
	nbytes += res;
    }
    return nbytes;
}

static unsigned char recv_u1(void)
{
    unsigned char c;
    recv_fully(hprof_fd, (char *)&c, sizeof(unsigned char));
    return c;
}

static unsigned short recv_u2(void)
{
    unsigned short s;
    recv_fully(hprof_fd, (char *)&s, sizeof(unsigned short));
    return ntohs(s);
}

static unsigned int recv_u4(void)
{
    unsigned int i;
    recv_fully(hprof_fd, (char *)&i, sizeof(unsigned int));
    return ntohl(i);
}

static void * recv_id(void)
{
    void *result;
    recv_fully(hprof_fd, (char *)&result, sizeof(void *));
    return result;
}

/* the callback thread */
static void
hprof_callback(void *p)
{
    while (hprof_is_on) {
        unsigned char tag = recv_u1();
        recv_u4();
        recv_u4();

	switch (tag) {
	case HPROF_CMD_GC:
	    CALL(RunGC)();
	    break;
	case HPROF_CMD_DUMP_HEAP: {
	    hprof_get_heap_dump();
	    break;
	}
	case HPROF_CMD_ALLOC_SITES: {
	    unsigned short flags = recv_u2();
	    unsigned int i_tmp = recv_u4();
            /* NOTE: The following (void *) intermediate cast is here solely
               for the purpose of resolving GCC strict-aliasing warnings. */
 	    float ratio = *(float *)(void *)(&i_tmp);
	    CALL(RawMonitorEnter)(data_access_lock);
	    hprof_output_sites(flags, ratio);
	    CALL(RawMonitorExit)(data_access_lock);
	    break;
	}
	case HPROF_CMD_HEAP_SUMMARY: {
	    CALL(RawMonitorEnter)(data_access_lock);
	    hprof_write_header(HPROF_HEAP_SUMMARY, 24);
	    hprof_write_u4(total_live_bytes);
	    hprof_write_u4(total_live_instances);
	    hprof_write_u4(jlong_high(total_alloced_bytes));
	    hprof_write_u4(jlong_low(total_alloced_bytes));
	    hprof_write_u4(jlong_high(total_alloced_instances));
	    hprof_write_u4(jlong_low(total_alloced_instances));
	    CALL(RawMonitorExit)(data_access_lock);
	    break;
	}
	case HPROF_CMD_EXIT:
	    hprof_is_on = FALSE;
	    fprintf(stderr, 
		    "HPROF: received exit event, exiting ...\n");
	    CALL(ProfilerExit)((jint)0);
	case HPROF_CMD_DUMP_TRACES:
	    CALL(RawMonitorEnter)(data_access_lock);
	    hprof_output_unmarked_traces();
	    CALL(RawMonitorExit)(data_access_lock);
	    break;
	case HPROF_CMD_CPU_SAMPLES: {
            unsigned int i_tmp;
            float ratio;
            recv_u2();
            i_tmp = recv_u4();
            /* NOTE: The following (void *) intermediate cast is here solely
               for the purpose of resolving GCC strict-aliasing warnings. */
            ratio = *(float *)(void *)(&i_tmp);
	    CALL(RawMonitorEnter)(data_access_lock);
	    hprof_output_trace_cost(ratio, CPU_SAMPLES_RECORD_NAME);
	    CALL(RawMonitorExit)(data_access_lock);
	    break;
	}
	case HPROF_CMD_CONTROL: {
	    unsigned short cmd = recv_u2();
	    if (cmd == 0x0001) {
	        CALL(EnableEvent)(JVMPI_EVENT_OBJ_ALLOC, NULL);
	    } else if (cmd == 0x0002) {
	        CALL(DisableEvent)(JVMPI_EVENT_OBJ_ALLOC, NULL);
	    } else if (cmd == 0x0003) {
	        hprof_objmap_t *thread_id = recv_id(); 
		hprof_cpu_sample_on(thread_id);
	    } else if (cmd == 0x0004) {
		hprof_objmap_t *thread_id = recv_id();
		hprof_cpu_sample_off(thread_id);
	    } else if (cmd == 0x0005) {
	        CALL(RawMonitorEnter)(data_access_lock);
	        hprof_clear_trace_cost();
		CALL(RawMonitorExit)(data_access_lock);
	    } else if (cmd == 0x0006) {
	        CALL(RawMonitorEnter)(data_access_lock);
		hprof_clear_site_table();
		CALL(RawMonitorExit)(data_access_lock);
	    } else if (cmd == 0x0007) {
	        max_trace_depth = recv_u2();
	    }
	    break;
	}
	default:
	    if (hprof_is_on) {
		hprof_is_on = FALSE;
	        fprintf(stderr,
			"HPROF ERROR : failed to recognize cmd %d, exiting..\n",
			(int)tag);
	    }
	    CALL(ProfilerExit)((jint)1);
	}
	CALL(RawMonitorEnter)(data_access_lock);
	hprof_flush();
	CALL(RawMonitorExit)(data_access_lock);
    }
}

void hprof_start_listener_thread(void)
{
    if (hprof_socket_p) {
        if (CALL(CreateSystemThread)("Hprof listener",
				     JVMPI_MAXIMUM_PRIORITY, 
				     hprof_callback) == JNI_ERR) {
	    fprintf(stderr, "HPROF ERROR: unable to create listener thread\n");
	}
    }
}
