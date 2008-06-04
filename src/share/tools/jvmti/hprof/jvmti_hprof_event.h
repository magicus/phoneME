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

#ifndef HPROF_EVENT_H
#define HPROF_EVENT_H

/* From BCI: */
void event_object_init(JNIEnv *env, jthread thread, jobject obj);
void event_newarray(JNIEnv *env, jthread thread, jobject obj);
void event_call(JNIEnv *env, jthread thread, 
		ClassIndex cnum, MethodIndex mnum);
void event_return(JNIEnv *env, jthread thread, 
		ClassIndex cnum, MethodIndex mnum);

/* From JVMTI: */
void event_class_load(JNIEnv *env, jthread thread, jclass klass, jobject loader);
void event_class_prepare(JNIEnv *env, jthread thread, jclass klass, jobject loader);
void event_thread_start(JNIEnv *env_id, jthread thread);
void event_thread_end(JNIEnv *env_id, jthread thread);
void event_exception_catch(JNIEnv *env, jthread thread, jmethodID method,
		jlocation location, jobject exception);

#endif
