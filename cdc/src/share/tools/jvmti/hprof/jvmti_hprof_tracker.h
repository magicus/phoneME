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

#ifndef HPROF_TRACKER_H
#define HPROF_TRACKER_H

/* The internal qualified classname */

#define OBJECT_CLASS_SIG	"Ljava/lang/Object;"
#define OBJECT_INIT_NAME	"<init>"
#define OBJECT_INIT_SIG		"()V"

#define TRACKER_PACKAGE         "com/sun/demo/jvmti/hprof"
#define TRACKER_CLASS_NAME      TRACKER_PACKAGE "/Tracker"
#define TRACKER_CLASS_SIG       "L" TRACKER_CLASS_NAME ";"

#define TRACKER_NEWARRAY_NAME        "NewArray"
#define TRACKER_NEWARRAY_SIG         "(Ljava/lang/Object;)V"
#define TRACKER_NEWARRAY_NATIVE_NAME "nativeNewArray"
#define TRACKER_NEWARRAY_NATIVE_SIG  "(Ljava/lang/Object;Ljava/lang/Object;)V"

#define TRACKER_OBJECT_INIT_NAME        "ObjectInit"
#define TRACKER_OBJECT_INIT_SIG         "(Ljava/lang/Object;)V"
#define TRACKER_OBJECT_INIT_NATIVE_NAME "nativeObjectInit"
#define TRACKER_OBJECT_INIT_NATIVE_SIG  "(Ljava/lang/Object;Ljava/lang/Object;)V"

#define TRACKER_CALL_NAME               "CallSite"
#define TRACKER_CALL_SIG                "(II)V"
#define TRACKER_CALL_NATIVE_NAME        "nativeCallSite"
#define TRACKER_CALL_NATIVE_SIG         "(Ljava/lang/Object;II)V"


#define TRACKER_RETURN_NAME             "ReturnSite"
#define TRACKER_RETURN_SIG              "(II)V"
#define TRACKER_RETURN_NATIVE_NAME      "nativeReturnSite"
#define TRACKER_RETURN_NATIVE_SIG       "(Ljava/lang/Object;II)V"

#define TRACKER_ENGAGED_NAME               "engaged"
#define TRACKER_ENGAGED_SIG                "I"

void     tracker_setup_class(void);
void     tracker_setup_methods(JNIEnv *env);
void     tracker_engage(JNIEnv *env);
void     tracker_disengage(JNIEnv *env);
jboolean tracker_method(jmethodID method);

#endif
