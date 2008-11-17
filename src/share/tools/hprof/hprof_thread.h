/*
 * @(#)hprof_thread.h	1.16 06/10/10
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

#ifndef _HPROF_THREAD_H
#define _HPROF_THREAD_H

#include "hprof_monitor.h"

/* Threads */
typedef struct hprof_thread_t {
    JNIEnv *env_id;                    /* env of thread */
    struct hprof_objmap_t *thread_id;  /* ptr to jvm<--->hprof object map */
    unsigned int serial_num;           /* unique serial number */
} hprof_thread_t;

/* recording an active thread */
typedef struct live_thread_t {
    struct live_thread_t *next;          /* next thread */
    struct live_thread_t *nextSuspended; /* next suspended thread */
    struct hprof_objmap_t *tid;          /* thread object ID */
    JNIEnv *env;                         /* JNIEnv */
    int cpu_sampled:1;                   /* thread CPU profile on? */
} live_thread_t;

/* list of active threads */
extern live_thread_t *live_thread_list;
extern int num_live_threads;

void hprof_thread_table_init(void);
hprof_thread_t * hprof_intern_thread(JNIEnv *env_id);
void hprof_remove_thread(JNIEnv *env_id);
hprof_thread_t *hprof_lookup_thread(JNIEnv *env_id);
void hprof_thread_start_event(JNIEnv *env_id,
			      char *t_name,
			      char *g_name,
			      char *p_name,
			      jobjectID thread_id,
			      int requested);
void hprof_thread_end_event(JNIEnv *env_id);
hprof_thread_t * hprof_fetch_thread_info(JNIEnv *env);
void hprof_print_thread_info(JNIEnv *env, int leading_comma);

/* method time */
typedef struct hprof_method_time_t {
    jmethodID method_id;               /* method id */
    jlong start_time;                  /* method start time */
    jlong time_in_callees;             /* time in callees */
    jlong time_in_gc;                  /* time in gc */
} hprof_method_time_t;

/* thread local info for timing methods */
typedef struct hprof_frames_cost_t {
    struct hprof_frames_cost_t *next;
    int num_frames;                    /* # of frames in call stack */
    int frames_index;                  /* start index into frames_array */
    jlong self_time;                   /* time spent only in the method */
    jlong total_time;                  /* total - including time in callees */
    unsigned int num_hits;             /* count */
} hprof_frames_cost_t;

typedef struct hprof_thread_local_t {
    struct hprof_method_time_t *stack_top;    /* stack top */
    int stack_limit;                          /* stack limit */
    struct hprof_method_time_t *stack;        /* for tracking mthd entry/exit */ 
    JVMPI_RawMonitor table_lock;              /* table lock */
    jmethodID *frames_array;                  /* frames seen by this thread */
    int cur_frame_index;                      /* next free frame */
    int frames_array_limit;                   /* frames array limit */
    hprof_frames_cost_t **table;              /* hashtable for frames' cost */
#ifdef HASH_STATS
    /*
     * TODO - Figure out if this hash table can use the generic routines
     * and if not, document a reason why so some else doesn't puzzle
     * over this.
     */
    hprof_name_t *thread_name;                /* thread's name */
    hprof_name_t *group_name;                 /* thread's group name */
    hprof_name_t *parent_name;                /* thread's parent name */
    /* there is no element count since every miss gets added to the table */
    unsigned int table_hits;                  /* frames' cost table hits */
    unsigned int table_misses;                /* frames' cost table misses */
#endif /* HASH_STATS */
    hprof_contended_monitor_t *mon;           /* last contended monitor info */
    jlong gc_start_time;                      /* time GC started */
} hprof_thread_local_t;

void hprof_bill_all_thread_local_tables(void);
void hprof_bill_frames_cost_table(JNIEnv *env_id);
#ifdef HASH_STATS
void hprof_print_per_thread_hash_stats(FILE *fp, hprof_thread_local_t *info);
void hprof_print_thread_hash_stats(FILE *fp);
#endif /* HASH_STATS */
#ifdef WATCH_ALLOCS
void hprof_free_thread_table(void);
#endif /* WATCH_ALLOCS */

/* thread local limits */

/*
 * Initial number of elements in the stack of method_time_t objects
 * that tracks cost information for frames that we have entered. This
 * value should be set to a reasonable guess as to the number of
 * methods deep a thread calls. This stack doubles in size for each
 * reallocation and does not shrink. The elements are persistent
 * until the associated frame is exited. Upon frame exit, the
 * element's information is added to the associated frames_cost_t
 * entry from the frames_cost_t hash table.
 */
#define HPROF_STACK_LIMIT 16

/*
 * Initial number of elements in the array of jmethodIDs that tracks
 * the call traces for the frames that we have exited. This value
 * should be set to a reasonable guess as to the number of unique
 * methods called by a thread. This number is multiplied by the
 * trace depth set at run-time to include space for the callers.
 * This array doubles in size for each reallocation and does not
 * shrink. The call traces are persistent until the thread exits.
 */
#define HPROF_FRAMES_ARRAY_LIMIT (64 * prof_trace_depth)

/* use a small table since this is per-thread; must be a power of 2 for mask */
#define HPROF_FRAMES_TABLE_SIZE  256
#define HPROF_FRAMES_TABLE_MASK  (HPROF_FRAMES_TABLE_SIZE - 1)

#endif /* _HPROF_THREAD_H */
