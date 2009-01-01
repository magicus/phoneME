/*
 * @(#)Array.c	1.31 06/10/10
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
#include "java_lang_reflect_Array.h"

/*
 * Native code for java.lang.reflect.Array.
 *
 */

#define OBJ "Ljava/lang/Object;"

/* Work-around for Symbian tool bug.  No longer needed. */
static JNINativeMethod methods[] = {
    {"get", "(" OBJ "I)" OBJ,
	(void *)&Java_java_lang_reflect_Array_get},
    {"set", "(" OBJ "I" OBJ ")V",
	(void *)&Java_java_lang_reflect_Array_set}
};

JNIEXPORT void JNICALL
Java_java_lang_reflect_Array_init(JNIEnv *env, jclass cls)
{
    /* Work-around for Symbian tool bug.  No longer needed. */
    (*env)->RegisterNatives(env, cls, methods,
	sizeof methods / sizeof methods[0]);
}

/*
 *
 */
JNIEXPORT jint JNICALL
Java_java_lang_reflect_Array_getLength(JNIEnv *env, jclass ignore, jobject arr)
{
    return JVM_GetArrayLength(env, arr);
}

/*
 *
 */
JNIEXPORT jobject JNICALL
Java_java_lang_reflect_Array_get(JNIEnv *env, jclass ignore, jobject arr,
				 jint index)
{
    return JVM_GetArrayElement(env, arr, index);
}

JNIEXPORT jboolean JNICALL
Java_java_lang_reflect_Array_getBoolean(JNIEnv *env, jclass ignore, jobject arr,
					jint index)
{
    return JVM_GetPrimitiveArrayElement(env, arr, index, JVM_T_BOOLEAN).z;
}

JNIEXPORT jbyte JNICALL
Java_java_lang_reflect_Array_getByte(JNIEnv *env, jclass ignore, jobject arr,
				     jint index)
{
    return JVM_GetPrimitiveArrayElement(env, arr, index, JVM_T_BYTE).b;
}

JNIEXPORT jchar JNICALL
Java_java_lang_reflect_Array_getChar(JNIEnv *env, jclass ignore, jobject arr,
				     jint index)
{
    return JVM_GetPrimitiveArrayElement(env, arr, index, JVM_T_CHAR).c;
}

JNIEXPORT jshort JNICALL
Java_java_lang_reflect_Array_getShort(JNIEnv *env, jclass ignore, jobject arr,
				     jint index)
{
    return JVM_GetPrimitiveArrayElement(env, arr, index, JVM_T_SHORT).s;
}

JNIEXPORT jint JNICALL
Java_java_lang_reflect_Array_getInt(JNIEnv *env, jclass ignore, jobject arr,
				     jint index)
{
    return JVM_GetPrimitiveArrayElement(env, arr, index, JVM_T_INT).i;
}

JNIEXPORT jlong JNICALL
Java_java_lang_reflect_Array_getLong(JNIEnv *env, jclass ignore, jobject arr,
				     jint index)
{
    return JVM_GetPrimitiveArrayElement(env, arr, index, JVM_T_LONG).j;
}

JNIEXPORT jfloat JNICALL
Java_java_lang_reflect_Array_getFloat(JNIEnv *env, jclass ignore, jobject arr,
				     jint index)
{
    return JVM_GetPrimitiveArrayElement(env, arr, index, JVM_T_FLOAT).f;
}

JNIEXPORT jdouble JNICALL
Java_java_lang_reflect_Array_getDouble(JNIEnv *env, jclass ignore, jobject arr,
				     jint index)
{
    return JVM_GetPrimitiveArrayElement(env, arr, index, JVM_T_DOUBLE).d;
}

/*
 *
 */
JNIEXPORT void JNICALL
Java_java_lang_reflect_Array_set(JNIEnv *env, jclass ignore, jobject arr,
				 jint index, jobject val)
{
    JVM_SetArrayElement(env, arr, index, val);
}

JNIEXPORT void JNICALL
Java_java_lang_reflect_Array_setBoolean(JNIEnv *env, jclass ignore,
					jobject arr, jint index, jboolean z)
{
    jvalue v;
    v.z = z;
    JVM_SetPrimitiveArrayElement(env, arr, index, v, JVM_T_BOOLEAN);
}

JNIEXPORT void JNICALL
Java_java_lang_reflect_Array_setByte(JNIEnv *env, jclass ignore,
					jobject arr, jint index, jbyte b)
{
    jvalue v;
    v.b = b;
    JVM_SetPrimitiveArrayElement(env, arr, index, v, JVM_T_BYTE);
}

JNIEXPORT void JNICALL
Java_java_lang_reflect_Array_setChar(JNIEnv *env, jclass ignore,
					jobject arr, jint index, jchar c)
{
    jvalue v;
    v.c = c;
    JVM_SetPrimitiveArrayElement(env, arr, index, v, JVM_T_CHAR);
}

JNIEXPORT void JNICALL
Java_java_lang_reflect_Array_setShort(JNIEnv *env, jclass ignore,
					jobject arr, jint index, jshort s)
{
    jvalue v;
    v.s = s;
    JVM_SetPrimitiveArrayElement(env, arr, index, v, JVM_T_SHORT);
}

JNIEXPORT void JNICALL
Java_java_lang_reflect_Array_setInt(JNIEnv *env, jclass ignore,
					jobject arr, jint index, jint i)
{
    jvalue v;
    v.i = i;
    JVM_SetPrimitiveArrayElement(env, arr, index, v, JVM_T_INT);
}

JNIEXPORT void JNICALL
Java_java_lang_reflect_Array_setLong(JNIEnv *env, jclass ignore,
					jobject arr, jint index, jlong j)
{
    jvalue v;
    v.j = j;
    JVM_SetPrimitiveArrayElement(env, arr, index, v, JVM_T_LONG);
}

JNIEXPORT void JNICALL
Java_java_lang_reflect_Array_setFloat(JNIEnv *env, jclass ignore,
					jobject arr, jint index, jfloat f)
{
    jvalue v;
    v.f = f;
    JVM_SetPrimitiveArrayElement(env, arr, index, v, JVM_T_FLOAT);
}

JNIEXPORT void JNICALL
Java_java_lang_reflect_Array_setDouble(JNIEnv *env, jclass ignore,
					jobject arr, jint index, jdouble d)
{
    jvalue v;
    v.d = d;
    JVM_SetPrimitiveArrayElement(env, arr, index, v, JVM_T_DOUBLE);
}

/*
 *
 */
JNIEXPORT jobject JNICALL
Java_java_lang_reflect_Array_newArray(JNIEnv *env, jclass ignore,
				      jclass eltClass, jint length)
{
    return JVM_NewArray(env, eltClass, length);
}

JNIEXPORT jobject JNICALL
Java_java_lang_reflect_Array_multiNewArray(JNIEnv *env, jclass ignore,
					   jclass eltClass, jintArray dim)
{
    return JVM_NewMultiArray(env, eltClass, dim);
}
