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

#ifndef HPROF_TLS_H
#define HPROF_TLS_H

void         tls_init(void);
TlsIndex     tls_find_or_create(JNIEnv *env, jthread thread);
void         tls_agent_thread(JNIEnv *env, jthread thread);
SerialNumber tls_get_thread_serial_number(TlsIndex index);
void         tls_list(void);
void         tls_delete_global_references(JNIEnv *env);
void         tls_garbage_collect(JNIEnv *env);
void         tls_cleanup(void);
void         tls_thread_ended(JNIEnv *env, TlsIndex index);
void         tls_sample_all_threads(JNIEnv *env);

MonitorIndex tls_get_monitor(TlsIndex index);
void         tls_set_monitor(TlsIndex index, MonitorIndex monitor_index);

void         tls_set_thread_object_index(TlsIndex index, 
			ObjectIndex thread_object_index);

jint         tls_get_tracker_status(JNIEnv *env, jthread thread, 
			jboolean skip_init, jint **ppstatus, TlsIndex* pindex,
		        SerialNumber *pthread_serial_num, 
			TraceIndex *ptrace_index);

void         tls_set_sample_status(ObjectIndex object_index, jint sample_status);
jint         tls_sum_sample_status(void);

void         tls_dump_traces(JNIEnv *env);

void         tls_monitor_start_timer(TlsIndex index);
jlong        tls_monitor_stop_timer(TlsIndex index);

void         tls_dump_monitor_state(JNIEnv *env);

void         tls_push_method(TlsIndex index, jmethodID method);
void         tls_pop_method(TlsIndex index, jthread thread, jmethodID method);
void         tls_pop_exception_catch(TlsIndex index, jthread thread, jmethodID method);

TraceIndex   tls_get_trace(TlsIndex index, JNIEnv *env,
                           int depth, jboolean skip_init);

void         tls_set_in_heap_dump(TlsIndex index, jint in_heap_dump);
jint         tls_get_in_heap_dump(TlsIndex index);
void         tls_clear_in_heap_dump(void);

TlsIndex     tls_find(SerialNumber thread_serial_num);

#endif
