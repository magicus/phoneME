/*
 * @(#)Thread.c	1.88 06/10/10
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

/*-
 *      Stuff for dealing with threads.
 *      originally in threadruntime.c, Sun Sep 22 12:09:39 1991
 */

#include "jni.h"
#include "jvm.h"

#include "java_lang_Thread.h"

#ifdef CVM_JAVALANGTHREAD_REGISTER_NATIVES

#define THD "Ljava/lang/Thread;"
#define OBJ "Ljava/lang/Object;"


static const JNINativeMethod methods[] = {
#ifdef JDK12
    {"start",            "()V",        (void *)&JVM_StartThread},
    {"stop0",            "(" OBJ ")V", (void *)&JVM_StopThread},
    {"isAlive",          "()Z",        (void *)&JVM_IsThreadAlive},
#else
    {"start0",            "(I)V",        (void *)&JVM_StartThread},
#endif
#ifdef CVM_HAVE_DEPRECATED
    {"suspend0",         "()V",        (void *)&JVM_SuspendThread},
    {"resume0",          "()V",        (void *)&JVM_ResumeThread},
#endif
    {"setPriority0",     "(I)V",       (void *)&JVM_SetThreadPriority},
    {"yield",            "()V",        (void *)&JVM_Yield},
#ifdef JDK12
    {"sleep",            "(J)V",       (void *)&JVM_Sleep},
#else
    {"sleep0",           "(J)V",       (void *)&JVM_Sleep},
#endif
    {"currentThread",    "()" THD,     (void *)&JVM_CurrentThread},
#ifdef CVM_HAVE_DEPRECATED
    {"countStackFrames", "()I",        (void *)&JVM_CountStackFrames},
#endif
    {"interrupt0",       "()V",        (void *)&JVM_Interrupt},
    {"isInterrupted",    "(Z)Z",       (void *)&JVM_IsInterrupted},
    {"holdsLock",        "(" OBJ ")Z", (void *)&JVM_HoldsLock},
};

#undef THD
#undef OBJ

JNIEXPORT void JNICALL
Java_java_lang_Thread_registerNatives(JNIEnv *env, jclass cls)
{
    (*env)->RegisterNatives(env, cls, methods, 12);
}

#endif /* CVM_JAVALANGTHREAD_REGISTER_NATIVES */

#include "javavm/include/indirectmem.h"

/* CVM only.  Perhaps all Thread natives should be changed to use CNI */
JNIEXPORT void JNICALL
Java_java_lang_Thread_setCurrentThread(JNIEnv *env, jclass cls, jobject thread)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMID_icellAssign(ee, CVMcurrentThreadICell(ee), thread);
}

JNIEXPORT void JNICALL
Java_java_lang_Thread_setThreadExiting(JNIEnv *env, jclass cls)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    ee->threadExiting = CVM_TRUE;
#ifdef CVM_JVMTI
    ee->threadState = CVM_THREAD_TERMINATED;
#endif
}
