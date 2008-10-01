/*
 * jvmti hprof
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

#ifndef HPROF_TRACE_H
#define HPROF_TRACE_H

void         trace_increment_all_sample_costs(jint count, jthread *threads, 
                        SerialNumber *thread_serial_nums, int depth,
			jboolean skip_init);

void         trace_get_all_current(jint count, jthread *threads, 
                        SerialNumber *thread_serial_nums, int depth,
			jboolean skip_init, TraceIndex *traces, 
			jboolean always_care);

TraceIndex   trace_get_current(jthread thread,
                        SerialNumber thread_serial_num, int depth,
                        jboolean skip_init,
                        FrameIndex *frames_buffer,
                        jvmtiFrameInfo *jframes_buffer);

void         trace_init(void);
TraceIndex   trace_find_or_create(SerialNumber thread_serial_num,
                        jint n_frames, FrameIndex *frames,
			jvmtiFrameInfo *jframes_buffer);
SerialNumber trace_get_serial_number(TraceIndex index);
void         trace_increment_cost(TraceIndex index, 
                        jint num_hits, jlong self_cost, jlong total_cost);
void         trace_list(void);
void         trace_cleanup(void);

void         trace_clear_cost(void);
void         trace_output_unmarked(JNIEnv *env);
void         trace_output_cost(JNIEnv *env, double cutoff);
void         trace_output_cost_in_prof_format(JNIEnv *env);

#endif

