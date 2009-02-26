/*
 * @(#)Float.c	1.13 06/10/10
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

#include "java_lang_Float.h"

/*
 * Find the float corresponding to a given bit pattern
 */
JNIEXPORT jfloat JNICALL
Java_java_lang_Float_intBitsToFloat(JNIEnv *env, jclass unused, jint v)
{
    union {
	int i;
	float f;
    } u;
    u.i = (long)v;
    return (jfloat)u.f;
}

/*
 * Find the bit pattern corresponding to a given float, collapsing NaNs
 */
JNIEXPORT jint JNICALL
Java_java_lang_Float_floatToIntBits(JNIEnv *env, jclass unused, jfloat v)
{
    union {
	int i;
	float f;
    } u;
    if (JVM_IsNaN((float)v)) {
        return 0x7fc00000;
    }
    u.f = (float)v;
    return (jint)u.i;
}

/*
 * Find the bit pattern corresponding to a given float, NOT collapsing NaNs
 */
JNIEXPORT jint JNICALL
Java_java_lang_Float_floatToRawIntBits(JNIEnv *env, jclass unused, jfloat v)
{
    union {
	int i;
	float f;
    } u;
    u.f = (float)v;
    return (jint)u.i;
}
