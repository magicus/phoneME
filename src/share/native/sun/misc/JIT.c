/*
 * @(#)JIT.c	1.27 06/10/10
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

#include "jni.h"
#include "jvm.h"

#include "jni.h"
#include "jvm.h"

#ifndef CVM_JIT
JNIEXPORT jboolean JNICALL 
Java_sun_misc_JIT_compileMethod(JNIEnv *env, jclass cls, jobject methodObject,
				jboolean verbose)
{
    return JNI_FALSE; /* Do nothing if !CVM_JIT */
}

JNIEXPORT jboolean JNICALL 
Java_sun_misc_JIT_neverCompileMethod(JNIEnv *env, jclass cls,
				     jobject methodObject)
{
    return JNI_FALSE; /* Do nothing if !CVM_JIT */
}

JNIEXPORT jboolean JNICALL 
Java_sun_misc_JIT_reparseJitOptions(JNIEnv *env, jclass cls,
				    jobject opts)
{
    return JNI_FALSE;
}
#else
#include "javavm/include/reflect.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/directmem.h"
#include "javavm/include/globals.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/utils.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitutils.h"
#include "javavm/include/jit_common.h"


JNIEXPORT jboolean JNICALL 
Java_sun_misc_JIT_compileMethod(JNIEnv *env, jclass cls, jobject methodObject,
				jboolean verbose)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMMethodBlock* mb;
    CVMClassBlock*  cb;
    CVMJITReturnValue rtnValue;

    if (methodObject == NULL) {
	CVMthrowNullPointerException(ee, NULL);
	return CVM_FALSE;
    }

    mb = CVMreflectGCSafeGetMethodBlock(CVMjniEnv2ExecEnv(env),
	(CVMObjectICell*)methodObject, NULL);
    if (mb==NULL){
	/* exception has been thrown */
	return JNI_FALSE;
    }
    cb = CVMmbClassBlock(mb);
    if (!CVMmbIsJava(mb)) {
	const char* what;
	if (CVMmbIs(mb, NATIVE)) {
	    what = "native";
	} else if (CVMmbIs(mb, ABSTRACT)) {
	    what = "abstract";
	} else {
	    what = "<unknown>";
	    CVMassert(CVM_FALSE);
	}

        CVMtraceJITAny(("Cannot compile %s method %M\n", what, mb));
	return JNI_FALSE;
    }

    /* Compile aggressively */
    {
      	CVMmbInvokeCostSet(mb, 0);
	CVMmbCompileFlags(mb) |= CVMJIT_COMPILE_ONCALL;
    }

    CVMtraceJITAny(("COMPILING %C.%M\n", cb, mb));
    rtnValue = CVMJITcompileMethod(ee, mb);

    CVMtraceJITAnyExec({
	if (rtnValue == CVMJIT_SUCCESS) {
            CVMconsolePrintf("Compiled %C.%M successfully\n", cb, mb);
	} else {
            CVMconsolePrintf("Compile failure for %C.%M: ", cb, mb);
	    switch (rtnValue) {
	    case CVMJIT_OUT_OF_MEMORY:
		CVMconsolePrintf("Out of memory\n");
		break;
	    case CVMJIT_CANNOT_COMPILE:
		CVMconsolePrintf("Cannot compile\n");
		break;
	    default:
		CVMconsolePrintf("Unknown return value\n");
		break;
	    }
	}
    });

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL 
Java_sun_misc_JIT_neverCompileMethod(JNIEnv *env, jclass cls,
		jobject methodObject)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMMethodBlock* mb;
    CVMClassBlock*  cb;

    mb = CVMreflectGCSafeGetMethodBlock(CVMjniEnv2ExecEnv(env),
	(CVMObjectICell*)methodObject, NULL);
    if (mb==NULL){
	/* exception has been thrown */
	return JNI_FALSE;
    }
    cb = CVMmbClassBlock(mb);
    if (CVMmbIsJava(mb)) {
	return CVMJITneverCompileMethod(ee, mb);
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL 
Java_sun_misc_JIT_reparseJitOptions(JNIEnv *env, jclass cls,
				    jobject opts)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    char* jitopts;
    jboolean result;

    CVMD_gcUnsafeExec(ee, { 
	jitopts = CVMconvertJavaStringToCString(ee, opts);
    });
    if (jitopts == NULL) {
	result = JNI_FALSE;
    } else {
#ifdef CVM_MTASK
	result = CVMjitReinitialize(ee, jitopts);
#else
	CVMconsolePrintf("Re-initialization of JIT only supported for mTASK");
	result = JNI_FALSE;
#endif
	free((void*)jitopts);
    }
    
    return result;
}

#endif /* CVM_JIT */
