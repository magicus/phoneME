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

#ifndef HPROF_MONITOR_H
#define HPROF_MONITOR_H

void monitor_init(void);
void monitor_list(void);
void monitor_cleanup(void);

void monitor_clear(void);
void monitor_write_contended_time(JNIEnv *env, double cutoff);

void monitor_contended_enter_event(JNIEnv *env_id, jthread thread, 
			jobject object);
void monitor_contended_entered_event(JNIEnv* env_id, jthread thread, 
			jobject object);
void monitor_wait_event(JNIEnv *env_id, jthread thread, 
			jobject object, jlong timeout);
void monitor_waited_event(JNIEnv *env_id, jthread thread, 
			jobject object, jboolean timed_out);

#endif
