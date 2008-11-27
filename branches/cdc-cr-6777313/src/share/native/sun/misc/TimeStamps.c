/*
 * @(#)TimeStamps.c	1.11 06/10/10
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
#include "sun_misc_TimeStamps.h"
#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/time.h"
#include "javavm/include/timestamp.h"
#include "javavm/include/globals.h"
#include "javavm/include/common_exceptions.h"

JNIEXPORT jboolean JNICALL
Java_sun_misc_TimeStamps_recordTimeStamps__Ljava_lang_String_2(
    JNIEnv* env, jclass cls, jstring location)
{    
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    const char* str = NULL;
    jboolean retval = JNI_TRUE;

    if (location != NULL) {
        str = (*env)->GetStringUTFChars(env,location, 0);
    }
    if (!CVMtimeStampRecord(ee, str, -1)) {
	retval = JNI_FALSE;
    }
    if (str != NULL) {
        (*env)->ReleaseStringUTFChars(env, location, str);
    }
    return retval;
}

JNIEXPORT jboolean JNICALL
Java_sun_misc_TimeStamps_recordTimeStamps__I(
    JNIEnv* env, jclass cls, jint position)
{    
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    return CVMtimeStampRecord(ee, NULL, (int)position);
}

JNIEXPORT jboolean JNICALL
Java_sun_misc_TimeStamps_printTimeStamps(JNIEnv *env, jclass cls)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    return CVMtimeStampPrint(ee);
}

JNIEXPORT jboolean JNICALL
Java_sun_misc_TimeStamps_isEnabled(JNIEnv *env, jclass cls)
{
    return CVMglobals.timeStampEnabled;
}

JNIEXPORT void JNICALL
Java_sun_misc_TimeStamps_enable(JNIEnv *env, jclass cls)
{
    CVMglobals.timeStampEnabled = CVM_TRUE;
}
