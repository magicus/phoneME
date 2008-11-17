/*
 * @(#)Runtime.c	1.62 06/10/10
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

/*
 *      Link foreign methods.  This first half of this file contains the
 *	machine independent dynamic linking routines.
 *	See "BUILD_PLATFORM"/java/lang/linker_md.c to see
 *	the implementation of this shared dynamic linking
 *	interface.
 *
 *	NOTE - source in this file is POSIX.1 compliant, host
 *	       specific code lives in the platform specific
 *	       code tree.
 */

#include "jni.h"
#include "jni_util.h"
#include "jvm.h"

#include "java_lang_Runtime.h"

#ifdef __cplusplus
#define this XthisX
#endif

JNIEXPORT jlong JNICALL
Java_java_lang_Runtime_freeMemory(JNIEnv *env, jobject this)
{
    return JVM_FreeMemory();
}

JNIEXPORT jlong JNICALL
Java_java_lang_Runtime_totalMemory(JNIEnv *env, jobject this)
{
    return JVM_TotalMemory();
}

JNIEXPORT jlong JNICALL
Java_java_lang_Runtime_maxMemory(JNIEnv *env, jobject this)
{
   return JVM_MaxMemory();
}

JNIEXPORT void JNICALL
Java_java_lang_Runtime_gc(JNIEnv *env, jobject this)
{
    JVM_GC();
}

JNIEXPORT void JNICALL
Java_java_lang_Runtime_traceInstructions(JNIEnv *env, jobject this, jboolean on)
{
    JVM_TraceInstructions(on);
}

JNIEXPORT void JNICALL
Java_java_lang_Runtime_traceMethodCalls(JNIEnv *env, jobject this, jboolean on)
{
    JVM_TraceMethodCalls(on);
}

JNIEXPORT void JNICALL
Java_java_lang_Runtime_runFinalization0(JNIEnv *env, jobject this)
{
    jclass cl;
    jmethodID mid;

    if ((cl = (*env)->FindClass(env, "java/lang/ref/Finalizer"))
	&& (mid = (*env)->GetStaticMethodID(env, cl,
					    "runFinalization", "()V"))) {
	(*env)->CallStaticVoidMethod(env, cl, mid);
    }
}

JNIEXPORT jboolean JNICALL
Java_java_lang_Runtime_safeExit(JNIEnv *env, jclass j_l_r, jint status)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    return CVMsafeExit(ee, status);
}

