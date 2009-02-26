/*
 * @(#)hprof_global.h	1.18 06/10/10
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

#ifndef _HPROF_GLOBALS_H
#define _HPROF_GLOBALS_H

#include "jvmpi.h"
#include "javavm/include/porting/ansi/stdlib.h"
#include "javavm/include/porting/ansi/stdio.h"

#define TRUE  1
#define FALSE 0

extern JavaVM *jvm;

extern int hprof_is_on;            /* whether hprof is enabled */
extern int hprof_fd;	           /* Non-zero file or socket descriptor. */
extern FILE *hprof_fp;	           /* FILE handle. */
extern int hprof_socket_p;         /* True if hprof_fd is a socket. */

extern int max_trace_depth;
extern int prof_trace_depth;
extern int thread_in_traces;
extern int lineno_in_traces;
extern char output_format;
extern float hprof_cutoff_point;
extern int dump_on_exit;
#ifdef HASH_STATS
extern int print_global_hash_stats_on_exit;
extern int print_thread_hash_stats_on_exit;
extern int print_verbose_hash_stats;
#endif /* HASH_STATS */
#ifdef WATCH_ALLOCS
extern unsigned int jmethodID_array_peak;	/* jmethodID element max */
extern unsigned int jmethodID_array_reallocs;	/* # of reallocations */
extern unsigned int jmethodID_array_threads;	/* # of threads to realloc */
extern unsigned int jmethodID_array_total;	/* jmethodID element total */
extern unsigned int method_time_stack_peak;	/* largest method_time array */
extern unsigned int method_time_stack_reallocs;	/* # of reallocations */
extern unsigned int method_time_stack_threads;	/* # of threads to realloc */
extern int          print_alloc_track_on_exit;
#endif /* WATCH_ALLOCS */
extern int cpu_sampling;
extern int monitor_sampling;
extern int monitor_tracing;
extern int heap_dump;
extern int alloc_sites;

/* cpu timing output formats */
#define OLD_PROF_OUTPUT_FORMAT 0
#define NEW_PROF_OUTPUT_FORMAT 1
extern int cpu_timing;
extern int timing_format;

extern jlong total_alloced_bytes;
extern jlong total_alloced_instances;
extern long total_live_bytes;
extern long total_live_instances;

/* Data access Lock */
extern JVMPI_RawMonitor data_access_lock;
/* Dump lock */
extern JVMPI_RawMonitor hprof_dump_lock;

/* profiler interface */
extern JVMPI_Interface *hprof_jvm_interface;
#define CALL(f) (hprof_jvm_interface->f)

/* version */
#define HPROF_HEADER "JAVA PROFILE 1.0.1"

/*
 * These enums are needed for all compilations, but are only
 * used when WATCH_ALLOCS is defined.
 */

enum {
    ALLOC_HASH_CLASS             =  0,	/* hprof_class_table */
    ALLOC_HASH_CONTENDED_MONITOR =  1,	/* hprof_contended_monitor_table */
    ALLOC_HASH_FRAME             =  2,	/* hprof_frame_table */
    ALLOC_HASH_FRAMES_COST       =  3,	/* per-thread frames_cost_t table */
    ALLOC_HASH_METHOD            =  4,	/* hprof_method_table */
    ALLOC_HASH_NAME              =  5,	/* hprof_name_table */
    ALLOC_HASH_OBJMAP            =  6,	/* hprof_objmap_table */
    ALLOC_HASH_RAW_MONITOR       =  7,	/* hprof_raw_monitor_table */
    ALLOC_HASH_SITE              =  8,	/* hprof_site_table */
    ALLOC_HASH_THREAD            =  9,	/* hprof_thread_table */
    ALLOC_HASH_TRACE             = 10,	/* hprof_trace_table */

    ALLOC_HASH_MAX               = 11	/* number of hash table types */
};

enum {
    ALLOC_TYPE_OTHER             =  0,	/* non-typed allocation */

    ALLOC_TYPE_ARRAY             =  1,	/* misc. arrays */
    ALLOC_TYPE_BUCKET            =  2,	/* hprof_bucket_t objects */
    ALLOC_TYPE_CALLFRAME         =  3,	/* JVMPI_CallFrame objects */
    ALLOC_TYPE_CALLTRACE         =  4,	/* JVMPI_CallTrace objects */
    ALLOC_TYPE_CLASS             =  5,	/* hprof_class_t objects */
    ALLOC_TYPE_CONTENDED_MONITOR =  6,	/* hprof_contended_monitor_t objects */
    ALLOC_TYPE_FIELD             =  7,	/* hprof_field_t objects */
    ALLOC_TYPE_FRAME             =  8,	/* hprof_frame_t objects */
    ALLOC_TYPE_FRAMES_COST       =  9,	/* hprof_frames_cost_t objects */
    ALLOC_TYPE_GLOBALREF         = 10,	/* hprof_globalref_t objects */
    ALLOC_TYPE_JMETHODID         = 11,	/* jmethodID values */
    ALLOC_TYPE_LIVE_THREAD       = 12,	/* live_thread_t objects */
    ALLOC_TYPE_METHOD            = 13,	/* hprof_method_t objects */
    ALLOC_TYPE_METHOD_TIME       = 14,	/* hprof_method_time_t objects */
    ALLOC_TYPE_NAME              = 15,	/* hprof_name_t objects */
    ALLOC_TYPE_OBJMAP            = 16,	/* hprof_objmap_t objects */
    ALLOC_TYPE_RAW_MONITOR       = 17,	/* hprof_raw_monitor_t objects */
    ALLOC_TYPE_SITE              = 18,	/* hprof_site_t objects */
    ALLOC_TYPE_THREAD            = 19,	/* hprof_thread_t objects */
    ALLOC_TYPE_THREAD_LOCAL      = 20,	/* hprof_thread_local_t objects */
    ALLOC_TYPE_TRACE             = 21,	/* hprof_frame_t ptrs for tracing */

    ALLOC_TYPE_HASH_TABLES       = 22,	/* hash tables - MUST BE NEXT TO LAST */
    ALLOC_TYPE_MAX               = ALLOC_TYPE_HASH_TABLES + ALLOC_HASH_MAX
};

#ifdef WATCH_ALLOCS
/*
 * If we are watching allocations, then the macro calls the tagging
 * version of the allocator and hprof_free() needs to be a real function.
 */
#define HPROF_CALLOC(tag, size) hprof_calloc_tag(tag, size)
void *hprof_calloc_tag(unsigned int tag, unsigned int size);
void hprof_free(void *ptr);
void hprof_print_alloc_track(FILE *fp);
#else /* !WATCH_ALLOCS */
/*
 * If we are not watching allocations, then the macro calls the
 * non-tagging version of the allocator and hprof_free() can just
 * be a macro that maps to free().
 */
#define HPROF_CALLOC(tag, size) hprof_calloc(size)
#define hprof_free free
#endif /* WATCH_ALLOCS */
/*
 * Keep original allocator around for older uses. If we are watching
 * allocations then these will be logged against "other".
 */
void *hprof_calloc(unsigned int size);

extern unsigned int micro_sec_ticks;

#endif /* _HPROF_GLOBALS_H */
