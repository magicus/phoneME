/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 */

#include "incls/_precompiled.incl"
#include "incls/_jni.cpp.incl"

#if ENABLE_JNI

static struct JNINativeInterface _jni_native_interface = {
    NULL,
    NULL,
    NULL,

    NULL,

    NULL, // GetVersion

    NULL, // DefineClass
    NULL, // FindClass

    NULL, // FromReflectedMethod
    NULL, // FromReflectedField

    NULL, // ToReflectedMethod

    NULL, // GetSuperclass
    NULL, // IsAssignableFrom

    NULL, // ToReflectedField

    NULL, // Throw
    NULL, // ThrowNew
    NULL, // ExceptionOccurred
    NULL, // ExceptionDescribe
    NULL, // ExceptionClear
    NULL, // FatalError

    NULL, // PushLocalFrame
    NULL, // PopLocalFrame

    NULL, // NewGlobalRef
    NULL, // DeleteGlobalRef
    NULL, // DeleteLocalRef
    NULL, // IsSameObject

    NULL, // NewLocalRef
    NULL, // EnsureLocalCapacity

    NULL, // AllocObject
    NULL, // NewObject
    NULL, // NewObjectV
    NULL, // NewObjectA

    NULL, // GetObjectClass
    NULL, // IsInstanceOf

    NULL, // GetMethodID

    NULL, // CallObjectMethod
    NULL, // CallObjectMethodV
    NULL, // CallObjectMethodA
    NULL, // CallBooleanMethod
    NULL, // CallBooleanMethodV
    NULL, // CallBooleanMethodA
    NULL, // CallByteMethod
    NULL, // CallByteMethodV
    NULL, // CallByteMethodA
    NULL, // CallCharMethod
    NULL, // CallCharMethodV
    NULL, // CallCharMethodA
    NULL, // CallShortMethod
    NULL, // CallShortMethodV
    NULL, // CallShortMethodA
    NULL, // CallIntMethod
    NULL, // CallIntMethodV
    NULL, // CallIntMethodA
    NULL, // CallLongMethod
    NULL, // CallLongMethodV
    NULL, // CallLongMethodA
    NULL, // CallFloatMethod
    NULL, // CallFloatMethodV
    NULL, // CallFloatMethodA
    NULL, // CallDoubleMethod
    NULL, // CallDoubleMethodV
    NULL, // CallDoubleMethodA
    NULL, // CallVoidMethod
    NULL, // CallVoidMethodV
    NULL, // CallVoidMethodA

    NULL, // CallNonvirtualObjectMethod
    NULL, // CallNonvirtualObjectMethodV
    NULL, // CallNonvirtualObjectMethodA
    NULL, // CallNonvirtualBooleanMethod
    NULL, // CallNonvirtualBooleanMethodV
    NULL, // CallNonvirtualBooleanMethodA
    NULL, // CallNonvirtualByteMethod
    NULL, // CallNonvirtualByteMethodV
    NULL, // CallNonvirtualByteMethodA
    NULL, // CallNonvirtualCharMethod
    NULL, // CallNonvirtualCharMethodV
    NULL, // CallNonvirtualCharMethodA
    NULL, // CallNonvirtualShortMethod
    NULL, // CallNonvirtualShortMethodV
    NULL, // CallNonvirtualShortMethodA
    NULL, // CallNonvirtualIntMethod
    NULL, // CallNonvirtualIntMethodV
    NULL, // CallNonvirtualIntMethodA
    NULL, // CallNonvirtualLongMethod
    NULL, // CallNonvirtualLongMethodV
    NULL, // CallNonvirtualLongMethodA
    NULL, // CallNonvirtualFloatMethod
    NULL, // CallNonvirtualFloatMethodV
    NULL, // CallNonvirtualFloatMethodA
    NULL, // CallNonvirtualDoubleMethod
    NULL, // CallNonvirtualDoubleMethodV
    NULL, // CallNonvirtualDoubleMethodA
    NULL, // CallNonvirtualVoidMethod
    NULL, // CallNonvirtualVoidMethodV
    NULL, // CallNonvirtualVoidMethodA

    NULL, // GetFieldID

    NULL, // GetObjectField
    NULL, // GetBooleanField
    NULL, // GetByteField
    NULL, // GetCharField
    NULL, // GetShortField
    NULL, // GetIntField
    NULL, // GetLongField
    NULL, // GetFloatField
    NULL, // GetDoubleField

    NULL, // SetObjectField
    NULL, // SetBooleanField
    NULL, // SetByteField
    NULL, // SetCharField
    NULL, // SetShortField
    NULL, // SetIntField
    NULL, // SetLongField
    NULL, // SetFloatField
    NULL, // SetDoubleField

    NULL, // GetStaticMethodID

    NULL, // CallStaticObjectMethod
    NULL, // CallStaticObjectMethodV
    NULL, // CallStaticObjectMethodA
    NULL, // CallStaticBooleanMethod
    NULL, // CallStaticBooleanMethodV
    NULL, // CallStaticBooleanMethodA
    NULL, // CallStaticByteMethod
    NULL, // CallStaticByteMethodV
    NULL, // CallStaticByteMethodA
    NULL, // CallStaticCharMethod
    NULL, // CallStaticCharMethodV
    NULL, // CallStaticCharMethodA
    NULL, // CallStaticShortMethod
    NULL, // CallStaticShortMethodV
    NULL, // CallStaticShortMethodA
    NULL, // CallStaticIntMethod
    NULL, // CallStaticIntMethodV
    NULL, // CallStaticIntMethodA
    NULL, // CallStaticLongMethod
    NULL, // CallStaticLongMethodV
    NULL, // CallStaticLongMethodA
    NULL, // CallStaticFloatMethod
    NULL, // CallStaticFloatMethodV
    NULL, // CallStaticFloatMethodA
    NULL, // CallStaticDoubleMethod
    NULL, // CallStaticDoubleMethodV
    NULL, // CallStaticDoubleMethodA
    NULL, // CallStaticVoidMethod
    NULL, // CallStaticVoidMethodV
    NULL, // CallStaticVoidMethodA

    NULL, // GetStaticFieldID

    NULL, // GetStaticObjectField
    NULL, // GetStaticBooleanField
    NULL, // GetStaticByteField
    NULL, // GetStaticCharField
    NULL, // GetStaticShortField
    NULL, // GetStaticIntField
    NULL, // GetStaticLongField
    NULL, // GetStaticFloatField
    NULL, // GetStaticDoubleField

    NULL, // SetStaticObjectField
    NULL, // SetStaticBooleanField
    NULL, // SetStaticByteField
    NULL, // SetStaticCharField
    NULL, // SetStaticShortField
    NULL, // SetStaticIntField
    NULL, // SetStaticLongField
    NULL, // SetStaticFloatField
    NULL, // SetStaticDoubleField

    NULL, // NewString
    NULL, // GetStringLength
    NULL, // GetStringChars
    NULL, // ReleaseStringChars

    NULL, // NewStringUTF
    NULL, // GetStringUTFLength
    NULL, // GetStringUTFChars
    NULL, // ReleaseStringUTFChars

    NULL, // GetArrayLength
 
    NULL, // NewObjectArray
    NULL, // GetObjectArrayElement
    NULL, // SetObjectArrayElement

    NULL, // NewBooleanArray
    NULL, // NewByteArray
    NULL, // NewCharArray
    NULL, // NewShortArray
    NULL, // NewIntArray
    NULL, // NewLongArray
    NULL, // NewFloatArray
    NULL, // NewDoubleArray

    NULL, // GetBooleanArrayElements
    NULL, // GetByteArrayElements
    NULL, // GetCharArrayElements
    NULL, // GetShortArrayElements
    NULL, // GetIntArrayElements
    NULL, // GetLongArrayElements
    NULL, // GetFloatArrayElements
    NULL, // GetDoubleArrayElements

    NULL, // ReleaseBooleanArrayElements
    NULL, // ReleaseByteArrayElements
    NULL, // ReleaseCharArrayElements
    NULL, // ReleaseShortArrayElements
    NULL, // ReleaseIntArrayElements
    NULL, // ReleaseLongArrayElements
    NULL, // ReleaseFloatArrayElements
    NULL, // ReleaseDoubleArrayElements

    NULL, // GetBooleanArrayRegion
    NULL, // GetByteArrayRegion
    NULL, // GetCharArrayRegion
    NULL, // GetShortArrayRegion
    NULL, // GetIntArrayRegion
    NULL, // GetLongArrayRegion
    NULL, // GetFloatArrayRegion
    NULL, // GetDoubleArrayRegion

    NULL, // SetBooleanArrayRegion
    NULL, // SetByteArrayRegion
    NULL, // SetCharArrayRegion
    NULL, // SetShortArrayRegion
    NULL, // SetIntArrayRegion
    NULL, // SetLongArrayRegion
    NULL, // SetFloatArrayRegion
    NULL, // SetDoubleArrayRegion

    NULL, // RegisterNatives
    NULL, // UnregisterNatives

    NULL, // MonitorEnter
    NULL, // MonitorExit

    NULL, // GetJavaVM

    NULL, // GetStringRegion
    NULL, // GetStringUTFRegion

    NULL, // GetPrimitiveArrayCritical
    NULL, // ReleasePrimitiveArrayCritical

    NULL, // GetStringChars     
    NULL, // ReleaseStringChars 

    NULL, // NewWeakGlobalRef
    NULL, // DeleteWeakGlobalRef

    NULL, // ExceptionCheck

    /* JNI_VERSION_1_4 additions: */
    NULL, // NewDirectByteBuffer
    NULL, // GetDirectBufferAddress
    NULL, // GetDirectBufferCapacity
};

extern "C" {

  JNIEnv _jni_env;

  void jni_initialize() {
    _jni_env.functions = &_jni_native_interface;
  }
}

#endif // ENABLE_JNI
