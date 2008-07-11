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

static jint JNICALL 
_JNI_GetVersion(JNIEnv * env) {
  return JNI_VERSION_1_4;
}

static jint JNICALL
_JNI_PushLocalFrame(JNIEnv *env, jint capacity) {
  SETUP_ERROR_CHECKER_ARG;
  jint ret = env->EnsureLocalCapacity(capacity);
  if (ret != JNI_OK) {
    return ret;
  }

  JniFrame::Raw frame = JniFrame::allocate_jni_frame(JVM_SINGLE_ARG_NO_CHECK);

  if (frame.is_null()) {
    Thread::clear_current_pending_exception();
    Throw::out_of_memory_error(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return JNI_ENOMEM;
  }

  Thread::Raw thread = Thread::current();
  
  thread().push_jni_frame(&frame);

  return JNI_OK;
}

static jobject JNICALL
_JNI_PopLocalFrame(JNIEnv *env, jobject result) {
  Oop::Raw resultOop;

  if (result != NULL) {
    resultOop = *decode_handle(result);
  }

  Thread::Raw thread = Thread::current();
  JniFrame::Raw frame = thread().pop_jni_frame();

  if (frame.not_null()) {
    JniFrame::Raw prev_frame = thread().jni_frame();
    const jint current_local_index = frame().local_ref_index();
    jint prev_local_index = 0;
    if (prev_frame.not_null()) {
      prev_local_index = prev_frame().local_ref_index();
    }
    if (prev_local_index < current_local_index) {
      ObjArray::Raw locals = thread().local_references();
      for (int i = prev_local_index; i < current_local_index; i++) {
        locals().obj_at_clear(i);
      }
    }
  }

  if (resultOop.not_null()) {
    return (jobject)ObjectHeap::register_local_reference(&resultOop);
  } else {
    return NULL;
  }
}

static jobject JNICALL
_JNI_NewGlobalRef(JNIEnv* env, jobject obj) {  
  if (obj == NULL) {
    return NULL;
  }

  SETUP_ERROR_CHECKER_ARG;
  UsingFastOops fast_oops;
  Oop::Fast oop = *decode_handle(obj);

  const int ref_index = 
    ObjectHeap::register_strong_reference(&oop JVM_MUST_SUCCEED);
  
  return (jobject)ref_index;
}

static void JNICALL
_JNI_DeleteGlobalRef(JNIEnv* env, jobject obj) {
  if (obj == NULL) {
    return;
  }

  const int ref_index = (int)obj;
  ObjectHeap::unregister_strong_reference(ref_index);
}

static void JNICALL
_JNI_DeleteLocalRef(JNIEnv* env, jobject localRef) {
  if (localRef == NULL) {
    return;
  }

  const int ref_index = (int)localRef;
  ObjectHeap::unregister_local_reference(ref_index);
}

static jboolean JNICALL
_JNI_IsSameObject(JNIEnv* env, jobject ref1, jobject ref2) {
  Oop::Raw oop1 = *decode_handle(ref1);
  Oop::Raw oop2 = *decode_handle(ref2);
  return oop1.equals(&oop2) ? JNI_TRUE : JNI_FALSE;
}

static jobject JNICALL
_JNI_NewLocalRef(JNIEnv* env, jobject ref) {
  if (ref == NULL) {
    return NULL;
  }

  UsingFastOops fast_oops;
  Oop::Fast oop = *decode_handle(ref);

  if (oop.is_null()) {
    return NULL;
  }

  jint ret = env->EnsureLocalCapacity(1);
  if (ret != JNI_OK) {
    return NULL;
  }

  return (jobject)ObjectHeap::register_local_reference(&oop);
}

static jint JNICALL
_JNI_EnsureLocalCapacity(JNIEnv* env, jint capacity) {
  if (capacity < 0) {
    return JNI_ERR;
  }

  UsingFastOops fast_oops;
  Thread::Fast thread = Thread::current();
  ObjArray::Fast locals = thread().local_references();

  int local_count = 0;

  {
    JniFrame::Raw frame = thread().jni_frame();

    if (frame.not_null()) {
      local_count = frame().local_ref_index() + 1;
    }
  }

  const int requested_length = local_count + capacity;

  if (locals.not_null()) {
    const int length = locals().length();
    if (requested_length <= length) {
      return JNI_OK;
    }
  }

  SETUP_ERROR_CHECKER_ARG;
  ObjArray::Fast new_locals = Universe::new_obj_array(requested_length 
                                                      JVM_NO_CHECK);

  if (new_locals.is_null()) {
    // Make sure OutOfMemoryError is thrown
    Thread::clear_current_pending_exception();
    Throw::out_of_memory_error(JVM_SINGLE_ARG_NO_CHECK);
    return JNI_ERR;
  }

  thread().set_local_references(new_locals.obj());

  if (locals.not_null()) {
    const int length = locals().length();
    ObjArray::array_copy(&locals, 0, &new_locals, 0, length JVM_MUST_SUCCEED);
  }

  return JNI_OK;
}

static jobject JNICALL
_JNI_NewWeakGlobalRef(JNIEnv* env, jobject obj) {
  if (obj == NULL) {
    return NULL;
  }

  SETUP_ERROR_CHECKER_ARG;
  UsingFastOops fast_oops;
  Oop::Fast oop = *decode_handle(obj);

  const int ref_index = 
    ObjectHeap::register_weak_reference(&oop JVM_MUST_SUCCEED);

  // IMPL_NOTE: throw OutOfMemoryError in case of failure
  return (jobject)ref_index;
}

static void JNICALL
_JNI_DeleteWeakGlobalRef(JNIEnv* env, jobject obj) {
  if (obj == NULL) {
    return;
  }

  const int ref_index = (int)obj;
  ObjectHeap::unregister_weak_reference(ref_index);
}

static struct JNINativeInterface _jni_native_interface = {
    NULL, // reserved0
    NULL, // reserved1
    NULL, // reserved2

    NULL, // reserved3

    _JNI_GetVersion, // GetVersion

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

    _JNI_PushLocalFrame, // PushLocalFrame
    _JNI_PopLocalFrame, // PopLocalFrame

    _JNI_NewGlobalRef,    // NewGlobalRef
    _JNI_DeleteGlobalRef, // DeleteGlobalRef
    _JNI_DeleteLocalRef, // DeleteLocalRef
    _JNI_IsSameObject, // IsSameObject

    _JNI_NewLocalRef, // NewLocalRef
    _JNI_EnsureLocalCapacity, // EnsureLocalCapacity

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

    _JNI_NewWeakGlobalRef,    // NewWeakGlobalRef
    _JNI_DeleteWeakGlobalRef, // DeleteWeakGlobalRef

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
