/*
 * @(#)jvmti_jni.c	1.3 06/10/26
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

/* This file contains a version of JNI needed for JVMTI functionality.
 */
#ifdef CVM_JVMTI
#include "javavm/include/globals.h"
#include "javavm/export/jni.h"
#include "javavm/include/jni_impl.h"
#include "javavm/include/jvmtiExport.h"

/* Note that third argument is unused -- was probably originally in an
   assert. If you decide to put that argument back in to the body,
   note that the WRAPPER_GETFIELD macro invocations below use a
   nonexistent (in CVM) member of the fieldblock called
   "signature". */
#define WRAPPER_GETFIELD(ReturnType,Result,ty) \
static ReturnType JNICALL \
jvmti_jni_Get##Result##Field(JNIEnv *env, jobject obj, jfieldID fieldID) \
{ \
    CVMFieldBlock* fb = fieldID; \
    ReturnType result; \
    if (CVMjvmtiIsWatchingFieldAccess()) {		     \
        CVMjvmtiPostFieldAccessEvent(CVMjniEnv2ExecEnv(env), \
				     obj, fb);		     \
    } \
    result = CVMjniGet##Result##Field(env,obj,fieldID); \
    return result; \
}

WRAPPER_GETFIELD(jobject,Object,(fb->signature[0] == 'L' ||
				 fb->signature[0] == '['))
WRAPPER_GETFIELD(jboolean,Boolean,(fb->signature[0] == 'Z'))
WRAPPER_GETFIELD(jbyte,Byte,(fb->signature[0] == 'B'))
WRAPPER_GETFIELD(jshort,Short,(fb->signature[0] == 'S'))
WRAPPER_GETFIELD(jchar,Char,(fb->signature[0] == 'C'))
WRAPPER_GETFIELD(jint,Int,(fb->signature[0] == 'I'))
WRAPPER_GETFIELD(jlong,Long,(fb->signature[0] == 'J'))
WRAPPER_GETFIELD(jfloat,Float,(fb->signature[0] == 'F'))
WRAPPER_GETFIELD(jdouble,Double,(fb->signature[0] == 'D'))

#define WRAPPER_SETFIELD(ValueType,Result,JValueField) \
static void JNICALL \
jvmti_jni_Set##Result##Field(JNIEnv *env, jobject obj, jfieldID fieldID, \
			  ValueType value) \
{ \
    CVMFieldBlock* fb = fieldID; \
    if (CVMjvmtiIsWatchingFieldModification()) { \
        jvalue jval; \
        jval.JValueField = value; \
        CVMjvmtiPostFieldModificationEvent(CVMjniEnv2ExecEnv(env), \
                                                  obj, fb, jval); \
    } \
    CVMjniSet##Result##Field(env,obj,fieldID,value); \
}

WRAPPER_SETFIELD(jobject,Object,l)
WRAPPER_SETFIELD(jboolean,Boolean,z)
WRAPPER_SETFIELD(jbyte,Byte,b)
WRAPPER_SETFIELD(jshort,Short,s)
WRAPPER_SETFIELD(jchar,Char,c)
WRAPPER_SETFIELD(jint,Int,i)
WRAPPER_SETFIELD(jlong,Long,j)
WRAPPER_SETFIELD(jfloat,Float,f)
WRAPPER_SETFIELD(jdouble,Double,d)

#define WRAPPER_GETSTATICFIELD(ReturnType,Result,ty) \
static ReturnType JNICALL \
jvmti_jni_GetStatic##Result##Field(JNIEnv *env, jclass clazz, \
				   jfieldID fieldID)	      \
{ \
    CVMFieldBlock* fb = fieldID; \
    ReturnType result; \
    if (CVMjvmtiIsWatchingFieldAccess()) {			\
        CVMjvmtiPostFieldAccessEvent(CVMjniEnv2ExecEnv(env),	\
				     NULL, fb);			\
    }								\
    result = CVMjniGetStatic##Result##Field(env,clazz,fieldID); \
    return result;						\
}

WRAPPER_GETSTATICFIELD(jobject,Object,(fb->signature[0] == 'L' ||
				       fb->signature[0] == '['))
WRAPPER_GETSTATICFIELD(jboolean,Boolean,(fb->signature[0] == 'Z'))
WRAPPER_GETSTATICFIELD(jbyte,Byte,(fb->signature[0] == 'B'))
WRAPPER_GETSTATICFIELD(jshort,Short,(fb->signature[0] == 'S'))
WRAPPER_GETSTATICFIELD(jchar,Char,(fb->signature[0] == 'C'))
WRAPPER_GETSTATICFIELD(jint,Int,(fb->signature[0] == 'I'))
WRAPPER_GETSTATICFIELD(jlong,Long,(fb->signature[0] == 'J'))
WRAPPER_GETSTATICFIELD(jfloat,Float,(fb->signature[0] == 'F'))
WRAPPER_GETSTATICFIELD(jdouble,Double,(fb->signature[0] == 'D'))

#define WRAPPER_SETSTATICFIELD(ValueType,Result,JValueField) \
static void JNICALL \
jvmti_jni_SetStatic##Result##Field(JNIEnv *env, jclass clazz, \
				   jfieldID fieldID,	      \
				   ValueType value)	      \
{ \
    CVMFieldBlock* fb = fieldID; \
    if (CVMjvmtiIsWatchingFieldModification()) { \
        jvalue jval; \
        jval.JValueField = value; \
        CVMjvmtiPostFieldModificationEvent(CVMjniEnv2ExecEnv(env), \
						  NULL, fb, jval); \
    } \
    CVMjniSetStatic##Result##Field(env,clazz,fieldID,value); \
}

WRAPPER_SETSTATICFIELD(jobject,Object,l)
WRAPPER_SETSTATICFIELD(jboolean,Boolean,z)
WRAPPER_SETSTATICFIELD(jbyte,Byte,b)
WRAPPER_SETSTATICFIELD(jshort,Short,s)
WRAPPER_SETSTATICFIELD(jchar,Char,c)
WRAPPER_SETSTATICFIELD(jint,Int,i)
WRAPPER_SETSTATICFIELD(jlong,Long,j)
WRAPPER_SETSTATICFIELD(jfloat,Float,f)
WRAPPER_SETSTATICFIELD(jdouble,Double,d)

void
CVMjvmtiInstrumentJNINativeInterface()
{
    struct JNINativeInterface *interface =
	CVMjniGetInstrumentableJNINativeInterface();

    /* Override the functions we need to moniotr */
    interface->GetObjectField = &jvmti_jni_GetObjectField;
    interface->GetBooleanField = &jvmti_jni_GetBooleanField;
    interface->GetByteField = &jvmti_jni_GetByteField;
    interface->GetCharField = &jvmti_jni_GetCharField;
    interface->GetShortField = &jvmti_jni_GetShortField;
    interface->GetIntField = &jvmti_jni_GetIntField;
    interface->GetLongField = &jvmti_jni_GetLongField;
    interface->GetFloatField = &jvmti_jni_GetFloatField;
    interface->GetDoubleField = &jvmti_jni_GetDoubleField;

    interface->SetObjectField = &jvmti_jni_SetObjectField;
    interface->SetBooleanField = &jvmti_jni_SetBooleanField;
    interface->SetByteField = &jvmti_jni_SetByteField;
    interface->SetCharField = &jvmti_jni_SetCharField;
    interface->SetShortField = &jvmti_jni_SetShortField;
    interface->SetIntField = &jvmti_jni_SetIntField;
    interface->SetLongField = &jvmti_jni_SetLongField;
    interface->SetFloatField = &jvmti_jni_SetFloatField;
    interface->SetDoubleField = &jvmti_jni_SetDoubleField;

    interface->GetStaticObjectField = &jvmti_jni_GetStaticObjectField;
    interface->GetStaticBooleanField = &jvmti_jni_GetStaticBooleanField;
    interface->GetStaticByteField = &jvmti_jni_GetStaticByteField;
    interface->GetStaticCharField = &jvmti_jni_GetStaticCharField;
    interface->GetStaticShortField = &jvmti_jni_GetStaticShortField;
    interface->GetStaticIntField = &jvmti_jni_GetStaticIntField;
    interface->GetStaticLongField = &jvmti_jni_GetStaticLongField;
    interface->GetStaticFloatField = &jvmti_jni_GetStaticFloatField;
    interface->GetStaticDoubleField = &jvmti_jni_GetStaticDoubleField;

    interface->SetStaticObjectField = &jvmti_jni_SetStaticObjectField;
    interface->SetStaticBooleanField = &jvmti_jni_SetStaticBooleanField;
    interface->SetStaticByteField = &jvmti_jni_SetStaticByteField;
    interface->SetStaticCharField = &jvmti_jni_SetStaticCharField;
    interface->SetStaticShortField = &jvmti_jni_SetStaticShortField;
    interface->SetStaticIntField = &jvmti_jni_SetStaticIntField;
    interface->SetStaticLongField = &jvmti_jni_SetStaticLongField;
    interface->SetStaticFloatField = &jvmti_jni_SetStaticFloatField;
    interface->SetStaticDoubleField = &jvmti_jni_SetStaticDoubleField;
}

#endif /* CVM_JVMTI */
