/*
 * @(#)jvm.h	1.11 06/10/27
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

#ifndef _EXPORT_JVM_H_
#define _EXPORT_JVM_H_

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/ansi/stdarg.h"
#include "javavm/include/porting/ansi/stddef.h"
#include "javavm/include/porting/io.h"
#include "javavm/include/porting/net.h"
#include "javavm/include/utils.h"
#include "javavm/include/string.h"	/* CVMstringIntern() */

#include "javavm/export/jni.h"

/* Avoid conflicts with our own types */

#include "javavm/include/jvm2romnatives.h"
#include "javavm/jdk-export/jvm.h"
#include "javavm/include/jvm2cvm.h"

/* This was added for JVMTI and JVMPI support of starting "system
   threads" -- threads for which the main function is a raw C
   function. nativeFunc must not be NULL, If the nativeFuncArg
   argument is non-NULL, it must be allocated on the heap by the
   caller and deallocated by the user's code (likely in the user's
   spawned thread). */
void
JVM_StartSystemThread(JNIEnv *env, jobject thread,
		      void(*nativeFunc)(void *), void* nativeFuncArg);

#if JAVASE >= 16
/*
 * Java thread state support
 *
 * FIXME - not actually supported yet, but helps to fix build errors.
 */
enum {
    JAVA_THREAD_STATE_NEW           = 0,
    JAVA_THREAD_STATE_RUNNABLE      = 1,
    JAVA_THREAD_STATE_BLOCKED       = 2,
    JAVA_THREAD_STATE_WAITING       = 3,
    JAVA_THREAD_STATE_TIMED_WAITING = 4,
    JAVA_THREAD_STATE_TERMINATED    = 5,
    JAVA_THREAD_STATE_COUNT         = 6
};
#endif

#endif /* !_EXPORT_JVM_H_ */
