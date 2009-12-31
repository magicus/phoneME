/*
 * @(#)StrictMath.c	1.58 06/10/10
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
#include "fdlibm.h"

#include "java_lang_StrictMath.h"

/* Work-around for Symbian tool bug.  No longer needed. */
static JNINativeMethod methods[] = {
    {"atan", "(D)D", (void *)&Java_java_lang_StrictMath_atan}
};

JNIEXPORT void JNICALL
Java_java_lang_StrictMath_init(JNIEnv *env, jclass cls)
{
    /* Work-around for Symbian tool bug.  No longer needed. */
    (*env)->RegisterNatives(env, cls, methods,
	sizeof methods / sizeof methods[0]);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_cos(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmCos((double)d);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_sin(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmSin((double)d);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_tan(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmTan((double)d);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_asin(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmAsin((double)d);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_acos(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmAcos((double)d);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_atan(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmAtan((double)d);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_exp(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmExp((double)d);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_log(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmLog((double)d);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_sqrt(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmSqrt((double)d);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_ceil(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmCeil((double)d);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_floor(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmFloor((double)d);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_atan2(JNIEnv *env, jclass unused, jdouble d1, jdouble d2)
{
    return (jdouble) CVMfdlibmAtan2((double)d1, (double)d2);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_pow(JNIEnv *env, jclass unused, jdouble d1, jdouble d2)
{
    return (jdouble) CVMfdlibmPow((double)d1, (double)d2);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_IEEEremainder(JNIEnv *env, jclass unused,
                                  jdouble dividend,
                                  jdouble divisor)
{
    return (jdouble) CVMfdlibmRemainder((double)dividend, (double)divisor);
}

JNIEXPORT jdouble JNICALL
Java_java_lang_StrictMath_rint(JNIEnv *env, jclass unused, jdouble d)
{
    return (jdouble) CVMfdlibmRint((double)d);
}
