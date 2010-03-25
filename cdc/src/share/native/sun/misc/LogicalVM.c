/*
 * @(#)LogicalVM.c	1.9 06/10/10
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

#include "javavm/include/interpreter.h"
#include "javavm/include/utils.h"
#include "jni_util.h"
#include "sun_misc_Isolate.h"


JNIEXPORT void JNICALL
Java_sun_misc_Isolate_initContext(JNIEnv* env, jobject isolate,
				  jobject initThread)
{
    if (!CVMLVMjobjectInit(env, isolate, initThread)) {
	if (!(*env)->ExceptionCheck(env)) {
	    JNU_ThrowOutOfMemoryError(env, "Logical VM initialization");
	}
	CVMtraceLVM(("LVM: Failed to initialize Logical VM: %I\n", isolate));
	CVMdebugExec((*env)->ExceptionDescribe(env));
    }
}

JNIEXPORT void JNICALL
Java_sun_misc_Isolate_destroyContext(JNIEnv* env, jobject isolate)
{
    CVMLVMjobjectDestroy(env, isolate);
}

JNIEXPORT void JNICALL
Java_sun_misc_Isolate_switchContext(JNIEnv* env, jclass isolate, 
				    jobject targetIsolate)
{
    CVMLVMcontextSwitch(env, targetIsolate);
}

/* Worm hole to invoke java.lang.Shutdown.waitAllUserThreads...() */
JNIEXPORT void JNICALL
Java_sun_misc_Isolate_waitAllUserThreadsExitAndShutdown(JNIEnv* env,
							jclass lvmCls)
{
    jclass cls = CVMcbJavaInstance(CVMsystemClass(java_lang_Shutdown));
    CVMjniCallStaticVoidMethod(
        env, cls,
	CVMglobals.java_lang_Shutdown_waitAllUserThreadsExitAndShutdown);
}

/* Worm hole to invoke Finalizer.runAllFinalizersOfSystemClass() */
JNIEXPORT void JNICALL
Java_sun_misc_Isolate_runAllFinalizersOfSystemClass(JNIEnv* env,
						    jclass lvmCls)
{
    jclass cls = CVMcbJavaInstance(CVMsystemClass(java_lang_ref_Finalizer));
    CVMjniCallStaticVoidMethod(
        env, cls,
	CVMglobals.java_lang_ref_Finalizer_runAllFinalizersOfSystemClass);
}



