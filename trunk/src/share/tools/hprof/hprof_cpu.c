/*
 * @(#)hprof_cpu.c	1.16 06/10/10
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

#include "jvmpi.h"
#include "hprof.h"

static JVMPI_RawMonitor hprof_cpu_lock;

void
hprof_cpu_sample_off(hprof_objmap_t *thread_id)
{
    cpu_sampling = FALSE;
    if (thread_id != NULL) {
        if (live_thread_list != NULL) {
	    live_thread_t *list;
	    CALL(RawMonitorEnter)(data_access_lock);
	    for(list = live_thread_list; list; list = list->next) {
	        if (list->tid == thread_id) {
		    list->cpu_sampled = 0;
		}
		if (list->cpu_sampled) {
		    cpu_sampling = TRUE;
		}
	    }
	    CALL(RawMonitorExit)(data_access_lock);
	}
    }
}

/* handler for enabling cpu sampling */
void
hprof_cpu_sample_on(hprof_objmap_t *thread_id)
{
    hprof_start_cpu_sampling_thread();
    if (thread_id == NULL) {
        cpu_sampling = TRUE;
    } else {
        /* turn on cpu sampling for the specified thread */
        if (live_thread_list) {
	    live_thread_t *list;
	    CALL(RawMonitorEnter)(data_access_lock);
	    for(list = live_thread_list; list; list = list->next) {
	        if (list->tid == thread_id) {
		    list->cpu_sampled = 1;
		    cpu_sampling = TRUE;
		}
	    }
	    CALL(RawMonitorExit)(data_access_lock);
	}
    }
    if (hprof_cpu_lock) {
        /* notify the CPU sampling thread */
        CALL(RawMonitorEnter)(hprof_cpu_lock);
	CALL(RawMonitorNotifyAll)(hprof_cpu_lock);
	CALL(RawMonitorExit)(hprof_cpu_lock);
    }
}

static void
hprof_cpu_loop(void *p)
{
    float avg_sample_time = 1;
    int last_sample_time = 1;
    int pause_time = 1;
    JNIEnv *my_env;
    
    (*jvm)->GetEnv(jvm, (void **)(void *)&my_env, JNI_VERSION_1_2);
    
    hprof_cpu_lock = CALL(RawMonitorCreate)("_Hprof CPU sampling lock");

    CALL(RawMonitorEnter)(hprof_cpu_lock);

    while (1) {
	avg_sample_time = last_sample_time * 0.1 + avg_sample_time * 0.9;

        if (cpu_sampling) {
	    /* Adjust the sampling interval according to the cost of
	     * each sample.
	     */
	    int avg_time = (int)(avg_sample_time);
	    if (avg_time == 0) {
	        avg_time = 1;
	    }
	    if (avg_time > pause_time * 2) {
	        pause_time = avg_time;
	    }
	    if (pause_time > 1 && avg_time < pause_time) {
	        pause_time = avg_time;
	    }
	    CALL(RawMonitorWait)(hprof_cpu_lock, jint_to_jlong(pause_time));
	} else {
	    /* If cpu profiling has been turned off, wait forever
	     * on the lock.
	     */
	    CALL(RawMonitorWait)(hprof_cpu_lock, jlong_zero);
	    continue;
	}

	CALL(DisableGC)();
	CALL(RawMonitorEnter)(hprof_dump_lock);
	CALL(RawMonitorEnter)(data_access_lock);
	
	last_sample_time = hprof_get_milliticks();
	
	{
	    live_thread_t *list;
	    live_thread_t *suspended_list = NULL;
	    int n_traces = 0;
            int i;
	    JVMPI_CallTrace *traces;
	    
	    /* Allocate space for all the traces we might collect, maximum
	       value is number of live threads */
	    traces = HPROF_CALLOC(ALLOC_TYPE_CALLTRACE,
		num_live_threads * sizeof(JVMPI_CallTrace));
	    for (i = 0; i < num_live_threads; i++) {
	        traces[i].frames = HPROF_CALLOC(ALLOC_TYPE_CALLFRAME,
		    max_trace_depth * sizeof(JVMPI_CallFrame));
	    }
	    
	    /* suspend all the runnable threads */
	    for (list = live_thread_list; list; list = list->next) {
	        if (list->cpu_sampled && 
		    list->env != my_env &&
		    ((CALL(GetThreadStatus)(list->env) & 
		      (~JVMPI_THREAD_INTERRUPTED)) == JVMPI_THREAD_RUNNABLE)) {
		    CALL(SuspendThread)(list->env);
		    list->nextSuspended = suspended_list;
		    suspended_list = list;
		}
	    }
	    
	    /* get traces for all the threads that were actually running */
	    for (list = suspended_list; list; list = list->nextSuspended) {
		if (CALL(ThreadHasRun)(list->env)) {
		    traces[n_traces].env_id = list->env;
		    CALL(GetCallTrace)(&(traces[n_traces]),
				       max_trace_depth);
		    if (traces[n_traces].num_frames > 0) {
		        n_traces++;
		    }
		}
	    }
       	    
	    /* resume all the suspended threads */
	    for (list = suspended_list; list; list = list->nextSuspended) {
	        CALL(ResumeThread)(list->env);
	    }
	    
	    /* record all the traces */
	    if (n_traces > 0) {
		for (i = 0; i < n_traces; i++) {
		    JVMPI_CallTrace *jtrace = &(traces[i]);
		    hprof_trace_t *htrace = 
		        hprof_intern_jvmpi_trace(jtrace->frames, 
						 jtrace->num_frames, 
						 jtrace->env_id);
		    if (htrace == NULL) {
		        fprintf(stderr,
				"HPROF ERROR: NULL trace in cpu_sample\n");
		    } else {
		        htrace->num_hits++;
			htrace->cost =
			    jlong_add(htrace->cost, jint_to_jlong(1));
		    }
		}
	    }

	    /* free up the space allocated to collect traces */
	    for(i = 0; i < num_live_threads; i++) {
	        hprof_free(traces[i].frames);
	    }
	    hprof_free(traces);
	}
		
	last_sample_time = hprof_get_milliticks() - last_sample_time;

	CALL(RawMonitorExit)(data_access_lock);
	CALL(RawMonitorExit)(hprof_dump_lock);
	CALL(EnableGC)();
    }
    /* never reached */
}

void hprof_start_cpu_sampling_thread(void)
{
    static int started = FALSE;
    if (started || cpu_sampling == FALSE)
        return;
    started = TRUE;
    /* start the cpu sampling thread */
    if (CALL(CreateSystemThread)("HPROF CPU profiler",
				 JVMPI_MAXIMUM_PRIORITY,
				 hprof_cpu_loop) < 0) {
        fprintf(stderr, "HPROF ERROR: unable to create CPU sampling thread\n");
    }
}

