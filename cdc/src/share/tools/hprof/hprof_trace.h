/*
 * @(#)hprof_trace.h	1.12 06/10/10
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

#ifndef _HPROF_TRACE_H
#define _HPROF_TRACE_H

#define CPU_SAMPLES_RECORD_NAME ("CPU SAMPLES")
#define CPU_TIMES_RECORD_NAME ("CPU TIME (ms)")
#define MONITOR_SAMPLES_RECORD_NAME ("MONITOR SAMPLES")
#define MONITOR_TIMES_RECORD_NAME ("MONITOR TIME (ms)")

/* Frames */
typedef struct hprof_frame_t {
    int marked;                        /* been written to output? */
    signed int lineno;                 /* line number */
    struct hprof_method_t *method;     /* method being executed */
} hprof_frame_t;

/* Stack traces */
typedef struct hprof_trace_t {
    unsigned int marked;               /* been written to output? */
    unsigned int serial_num;           /* serial number, starts from 1 */
    unsigned int num_hits;             /* number of hits */
    jlong cost;                        /* # of times sampled or time in ms */
    unsigned int thread_serial_num;    /* thread serial number */
    unsigned int n_frames;             /* number of frames */
    struct hprof_frame_t *frames[1];   /* frames */
} hprof_trace_t;

/* Auxilary data structure used for collecting trace info. */
typedef struct hprof_trace_iterate_t {
    hprof_trace_t **traces;               
    int index;
    int total_count;
} hprof_trace_iterate_t;

void hprof_trace_table_init(void);
hprof_trace_t * hprof_alloc_tmp_trace(int n_frames, JNIEnv *env_id);
hprof_trace_t * hprof_intern_tmp_trace(hprof_trace_t *trace_tmp);
hprof_trace_t * hprof_intern_jvmpi_trace(JVMPI_CallFrame *frames, int n_frames,
					 JNIEnv *env_id);
hprof_trace_t *hprof_get_trace(JNIEnv *env_id, int depth);
void hprof_output_unmarked_traces(void);
void hprof_output_trace_cost(float cutoff, char *record_name);
void hprof_output_trace_cost_in_prof_format(void);
void hprof_clear_trace_cost(void);

void hprof_frame_table_init(void);
hprof_frame_t * hprof_intern_jvmpi_frame(jmethodID method_id, int lineno);

#ifdef HASH_STATS
void hprof_print_frame_hash_stats(FILE *fp);
void hprof_print_trace_hash_stats(FILE *fp);
#endif /* HASH_STATS */
#ifdef WATCH_ALLOCS
void hprof_free_frame_table(void);
void hprof_free_trace_table(void);
#endif /* WATCH_ALLOCS */

#endif /* _HPROF_TRACE_H */
