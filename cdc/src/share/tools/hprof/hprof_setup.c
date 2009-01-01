/*
 * @(#)hprof_setup.c	1.22 06/10/10
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

#include "javavm/include/porting/ansi/string.h"
#include "javavm/include/porting/path.h"
#include "hprof.h"

typedef struct {
    char *name;
    int *ptr;
} binary_switch_t;

static binary_switch_t binary_switches[] = {
    {"lineno", &lineno_in_traces},
    {"thread", &thread_in_traces},
    {"doe",    &dump_on_exit},
    {"monitor", &monitor_tracing}
#ifdef WATCH_ALLOCS
    , {"pat",    &print_alloc_track_on_exit}
#endif /* WATCH_ALLOCS */
#ifdef HASH_STATS
    , {"pghs", &print_global_hash_stats_on_exit}
    , {"pths", &print_thread_hash_stats_on_exit}
    , {"pvhs", &print_verbose_hash_stats}
#endif /* HASH_STATS */
};

static int get_tok(char **src, char *buf, int buflen, char sep)
{
    int i;
    char *p = *src;
    for (i = 0; i < buflen; i++) {
        if (p[i] == 0 || p[i] == sep) {
	    buf[i] = 0;
	    if (p[i] == sep) {
	        i++;
	    }
	    *src += i;
	    return i;
	}
	buf[i] = p[i];
    }
    /* overflow */
    return 0;
}

static void hprof_print_usage(void)
{
    fprintf(stdout,
        "Hprof usage: -Xrunhprof[:help]|[<option>=<value>, ...]\n"
        "\n"
        "Option Name and Value  Description                Default\n"
        "---------------------  ----------------------     -------\n"
        "heap=dump|sites|all    heap profiling             all\n"
        "cpu=samples|times|old  CPU usage                  off\n"
        "monitor=y|n            monitor contention         n\n"
        "format=a|b             ascii or binary output     a\n"
        "file=<file>            write data to file         java.hprof(.txt for ascii)\n"
        "net=<host>:<port>      send data over a socket    write to file\n"
        "depth=<size>           stack trace depth          4\n"
        "cutoff=<value>         output cutoff point        0.0001\n"
        "lineno=y|n             line number in traces?     y\n"
        "thread=y|n             thread in traces?          n\n"
        "doe=y|n                dump on exit?              y\n"
#ifdef WATCH_ALLOCS
        "pat=y|n                print allocation track?    n\n"
#endif /* WATCH_ALLOCS */
#ifdef HASH_STATS
        "pghs=y|n               print global hash stats?   n\n"
        "pths=y|n               print thread hash stats?   n\n"
        "pvhs=y|n               print verbose hash stats?  n\n"
#endif /* HASH_STATS */
        "\nExample: java -Xrunhprof:cpu=samples,file=log.txt,"
        "depth=3 FooClass\n\n");
}
             
	     
static void hprof_do_setup(char *str)
{
    FILE *fp = NULL;
    int output_specified = FALSE;
    char output_filename[CVM_PATH_MAXLEN];
    
    hprof_is_on = TRUE;

    if (str == 0)
        str = "";
    
    if ((strcmp(str, "help")) == 0) {
        hprof_print_usage();
	CALL(ProfilerExit)((jint)0);
    }

    while (*str) {
        char buf[1000];
	if (!get_tok(&str, buf, sizeof(buf), '=')) {
	    goto bad_option;
	}
        if (strcmp(buf, "file") == 0) {
	    if (output_specified ||
	        !get_tok(&str, buf, sizeof(buf), ',')) {
	        goto bad_option;
	    }
	    strcpy(output_filename, buf);
	    output_specified = TRUE;
	} else if (strcmp(buf, "net") == 0) {
	    char port_str[6];
	    int port;
	    if (output_specified ||
		!get_tok(&str, buf, sizeof(buf), ':') ||
		!get_tok(&str, port_str, sizeof(port_str), ',')) {
                goto bad_option;
	    }
            port = atoi(port_str);
            if (port <= 0 || port > 65535) {
                fprintf(stderr, "HPROF ERROR: bad port number\n");
                goto bad_option_no_msg;
            }
	    hprof_fd = hprof_real_connect(buf, (unsigned short)port);
	    if (hprof_fd <= 0) {
	        fprintf(stderr, "HPROF ERROR: can't connect to %s:%u", 
			buf, port);
		    goto bad_option_no_msg;
	    }
	    hprof_socket_p = TRUE;
	    output_specified = TRUE;
        } else if (strcmp(buf, "format") == 0) {
	    if (!get_tok(&str, buf, sizeof(buf), ',')) {
	        goto bad_option;
	    }
	    if (strcmp(buf, "a") == 0) {
	        output_format = 'a';
	    } else if (strcmp(buf, "b") == 0) {
	        output_format = 'b';
	    } else {
	        goto bad_option;
	    }
        } else if (strcmp(buf, "depth") == 0) {
	    if (!get_tok(&str, buf, sizeof(buf), ',')) {
	        goto bad_option;
	    }
	    max_trace_depth = atol(buf);
	    prof_trace_depth = max_trace_depth;
        } else if (strcmp(buf, "cutoff") == 0) {
	    if (!get_tok(&str, buf, sizeof(buf), ',')) {
	        goto bad_option;
	    }
	    hprof_cutoff_point = atof(buf);
	} else if (strcmp(buf, "cpu") == 0) {
	    if (!get_tok(&str, buf, sizeof(buf), ',')) {
	        goto bad_option;
	    }
	    if ((strcmp(buf, "samples") == 0) ||
		(strcmp(buf, "y") == 0)) {
	        cpu_sampling = TRUE;
	    } else if (strcmp(buf, "times") == 0) {
	        cpu_timing = TRUE;
		timing_format = NEW_PROF_OUTPUT_FORMAT;
	    } else if (strcmp(buf, "old") == 0) {
	        cpu_timing = TRUE;
		timing_format = OLD_PROF_OUTPUT_FORMAT;
	    } else {
	        goto bad_option;
	    }
	} else if (strcmp(buf, "heap") == 0) {
	    if (!get_tok(&str, buf, sizeof(buf), ',')) {
	        goto bad_option;
	    }
	    if (strcmp(buf, "dump") == 0) {
	        heap_dump = TRUE;
	    } else if (strcmp(buf, "sites") == 0) {
	        alloc_sites = TRUE;
	    } else if (strcmp(buf, "all") == 0) {
	        heap_dump = TRUE;
	        alloc_sites = TRUE;
	    } else {
	        goto bad_option;
	    }
	} else {
	    int i;
	    int n_switches = 
	        sizeof(binary_switches) / sizeof(binary_switch_t); 
	    for (i = 0; i < n_switches; i++) {
	        if (strcmp(binary_switches[i].name, buf) == 0) {
		    if (!get_tok(&str, buf, sizeof(buf), ',')) {
		        goto bad_option;
		    }
		    if (strcmp(buf, "y") == 0) {
		        *(binary_switches[i].ptr) = TRUE;
		    } else if (strcmp(buf, "n") == 0) {
		        *(binary_switches[i].ptr) = FALSE;
		    } else {
		        goto bad_option;
		    }
		    break;
		}
	    }
	    if (i >= n_switches) {
	        goto bad_option;
	    }
	}
    }
   
    if (output_format == 'b') {
        if (cpu_timing) {
	    fprintf(stderr, "HPROF ERROR: cpu=times|old "
		    "is not supported with format=b\n");
	    goto bad_option_no_msg;
	}
	
	if (monitor_tracing) {
	    fprintf(stderr, "HPROF ERROR: monitor=y is not "
		    "supported with format=b\n");
	    goto bad_option_no_msg;
	}
    }
	    
    if (timing_format == OLD_PROF_OUTPUT_FORMAT) {
        prof_trace_depth = 2;
    }

    if (!output_specified) {
        if (output_format == 'a') {
	    strcpy(output_filename, "java.hprof.txt");
	} else {
	    strcpy(output_filename, "java.hprof");
	}
    }

    if (!hprof_socket_p) {
        if (output_format == 'a') {
	    fp = fopen(output_filename, "w");
	} else {
	    fp = fopen(output_filename, "wb");
	}
	if (fp == 0) {
	    fprintf(stderr, "HPROF ERROR: can't write profile file: %s\n", 
		    output_filename);
	    goto bad_option_no_msg;
	}
	hprof_fp = fp;
    }

    /* By default we turn on alloc_sites and heap_dump */
    if (!cpu_timing && !cpu_sampling && !monitor_tracing && !alloc_sites && 
	!heap_dump) {
        alloc_sites = heap_dump = TRUE;
    }

    return;

bad_option:
    fprintf(stderr, "HPROF ERROR: bad hprof option\n");
bad_option_no_msg:
    CALL(ProfilerExit)((jint)1);
}

static jint hprof_enable_events(void)
{
    if (CALL(EnableEvent)(JVMPI_EVENT_CLASS_LOAD, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_CLASS_UNLOAD, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_THREAD_START, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_THREAD_END, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_DUMP_DATA_REQUEST, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_RESET_DATA_REQUEST, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_JVM_INIT_DONE, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_JVM_SHUT_DOWN, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }

    
    if (cpu_timing) {
        if (CALL(EnableEvent)(JVMPI_EVENT_METHOD_ENTRY, NULL) != JVMPI_SUCCESS) {
	    return JNI_ERR;
	}
	if (CALL(EnableEvent)(JVMPI_EVENT_METHOD_EXIT, NULL) != JVMPI_SUCCESS) {
	    return JNI_ERR;
	}
    }

    if (monitor_tracing) {
        if (CALL(EnableEvent)(JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER, NULL) != 
	    JVMPI_SUCCESS) {
	    return JNI_ERR;
	}
        if (CALL(EnableEvent)(JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED, NULL) !=
	    JVMPI_SUCCESS) {
	    return JNI_ERR;
	}
        if (CALL(EnableEvent)(JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT, NULL) !=
	    JVMPI_SUCCESS) {
	    return JNI_ERR;
	}
        if (CALL(EnableEvent)(JVMPI_EVENT_MONITOR_CONTENDED_ENTER, NULL) !=
	    JVMPI_SUCCESS) {
	    return JNI_ERR;
	}
        if (CALL(EnableEvent)(JVMPI_EVENT_MONITOR_CONTENDED_ENTERED, NULL) !=
	    JVMPI_SUCCESS) {
	    return JNI_ERR;
	}
        if (CALL(EnableEvent)(JVMPI_EVENT_MONITOR_CONTENDED_EXIT, NULL) !=
	    JVMPI_SUCCESS) {
	    return JNI_ERR;
	}
        if (CALL(EnableEvent)(JVMPI_EVENT_MONITOR_WAIT, NULL) != JVMPI_SUCCESS) {
	    return JNI_ERR;
	}
        if (CALL(EnableEvent)(JVMPI_EVENT_MONITOR_WAITED, NULL) != JVMPI_SUCCESS) {
	    return JNI_ERR;
	}
    }

    if (CALL(EnableEvent)(JVMPI_EVENT_GC_START, NULL) != JVMPI_SUCCESS) {
         return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_GC_FINISH, NULL) != JVMPI_SUCCESS) {
         return JNI_ERR;
    }

    if (heap_dump || alloc_sites) {
        if (CALL(EnableEvent)(JVMPI_EVENT_OBJ_ALLOC, NULL) != JVMPI_SUCCESS) {
	     return JNI_ERR;
	}
    }

    if (CALL(EnableEvent)(JVMPI_EVENT_OBJ_MOVE, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_OBJ_FREE, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_NEW_ARENA, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_DELETE_ARENA, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_JNI_GLOBALREF_ALLOC, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }
    if (CALL(EnableEvent)(JVMPI_EVENT_JNI_GLOBALREF_FREE, NULL) != JVMPI_SUCCESS) {
        return JNI_ERR;
    }

    return JNI_OK;
}

void hprof_init_setup(char *profiler_options)
{        
    hprof_do_setup(profiler_options);
    
    total_alloced_bytes = jlong_zero;
    total_alloced_instances = jlong_zero;

    hprof_io_setup();
    hprof_class_table_init();
    hprof_thread_table_init();
    hprof_method_table_init();
    hprof_name_table_init();
    hprof_frame_table_init();
    hprof_trace_table_init();
    hprof_site_table_init();
    hprof_objmap_init();
    if (monitor_tracing) {
        hprof_raw_monitor_table_init();
        hprof_contended_monitor_table_init();
    }

    /* Create data access lock */
    data_access_lock = CALL(RawMonitorCreate)("_hprof_data_access_lock");
    
    hprof_enable_events();
    
    /* Write the header information */
    if (output_format == 'b') {
        jint settings = 0;
        hprof_write_raw(HPROF_HEADER, strlen(HPROF_HEADER) + 1);
	hprof_write_u4(sizeof(void *));
	{
	    jlong t = hprof_get_timemillis();
	    hprof_write_u4(jlong_high(t));
	    hprof_write_u4(jlong_low(t));
	}
	hprof_write_header(HPROF_CONTROL_SETTINGS, 4 + 2);
	if (heap_dump || alloc_sites) settings |= 1;
	if (cpu_sampling) settings |= 2;
	hprof_write_u4(settings);
	hprof_write_u2((unsigned short)max_trace_depth);
    } else if ((!cpu_timing) || (timing_format != OLD_PROF_OUTPUT_FORMAT)) {
        /* we don't want the prelude file for the old prof output format */
        time_t t = time(0);
	char prelude_file[CVM_PATH_MAXLEN];
	FILE *prelude_fd;

	hprof_get_prelude_path(prelude_file);

	prelude_fd = fopen(prelude_file, "r");
	if (prelude_fd == NULL) {
	    fprintf(stderr, "Can't open %s\n", prelude_file);
	    CALL(ProfilerExit)((jint)1);
	}
	hprof_printf("%s, created %s\n", HPROF_HEADER, ctime(&t));
	while (1) {
	    char buf[128];
	    int nbytes = fread(buf, 1, sizeof(buf), prelude_fd);
	    if (nbytes == 0) {
	      break;
	    }
	    hprof_write_raw(buf, nbytes);
	}
	fclose(prelude_fd);
	hprof_printf("\n--------\n\n");
    }
    
    micro_sec_ticks = hprof_get_milliticks() * 1000;
}

/**
 * Socket connection
 */
#define HPROF_MAX_HOSTADDR_SIZE 512

