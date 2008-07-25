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

#define FOR_ALL_PRIMITIVE_TYPES(template) \
  template(jboolean, Boolean) \
  template(jbyte,    Byte) \
  template(jchar,    Char) \
  template(jshort,   Short) \
  template(jint,     Int) \
  template(jlong,    Long) \
  template(jfloat,   Float) \
  template(jdouble,  Double)

static jobject JNICALL
new_local_ref_for_oop(JNIEnv* env, Oop * oop) {
  jint ret = env->EnsureLocalCapacity(1);
  if (ret != JNI_OK) {
    return NULL;
  }

  return (jobject)ObjectHeap::register_local_reference(oop);
}

static jobject JNICALL
new_local_ref(JNIEnv* env) {
  Oop dummy;
  return new_local_ref_for_oop(env, &dummy);
}

static jint JNICALL 
_JNI_GetVersion(JNIEnv * env) {
  return JNI_VERSION_1_4;
}

static jclass JNICALL
_JNI_FindClass(JNIEnv *env, const char * name) {
  // IMPL_NOTE: must throw typed Errors as required by JNI spec
  SETUP_ERROR_CHECKER_ARG;

  UsingFastOops fast_oops;

  Symbol::Fast class_name = SymbolTable::symbol_for(name JVM_CHECK_0);
  if (FieldType::is_array(&class_name)) {
    Symbol::Raw parsed_class_name = 
      TypeSymbol::parse_array_class_name(&class_name JVM_NO_CHECK);
    // TypeSymbol::parse_array_class_name() throws a plain Error
    // if class name is invalid, JNI spec requires NoClassDefFoundError
    if (CURRENT_HAS_PENDING_EXCEPTION) {
      Thread::clear_current_pending_exception();
      Throw::class_not_found(&class_name, ErrorOnFailure JVM_THROW_0);
    }
    class_name = parsed_class_name;
  }

  JavaClass::Fast cl = SystemDictionary::resolve(&class_name,
                                                 ErrorOnFailure
                                                 JVM_CHECK_0);

  AZZERT_ONLY(Symbol::Fast actual_name = cl().name());
  GUARANTEE(actual_name().matches(&class_name),
            "Inconsistent class name lookup result");

  // For hidden classes we throw NoClassDefFoundError if lookup
  // is directly performed by user's code.
  if (cl().is_hidden() && Thread::current()->has_user_frames_until(1)) {
    Throw::class_not_found(&class_name, ErrorOnFailure JVM_THROW_0);
  }

#if ENABLE_ISOLATES
  JavaClassObj::Fast result =
    cl().get_or_allocate_java_mirror(JVM_SINGLE_ARG_CHECK_0);
#else
  JavaClassObj::Fast result = cl().java_mirror();
#endif

  GUARANTEE(!result.is_null(), "mirror must not be null");

  if (cl().is_instance_class()) {
    ((InstanceClass*)&cl)->initialize(JVM_SINGLE_ARG_CHECK_0);
  }

  return new_local_ref_for_oop(env, &result);
}

static jclass JNICALL
_JNI_GetSuperclass(JNIEnv *env, jclass sub) {
  // Cannot just forward to KNI_GetSuperClass() since KNI and JNI specs 
  // require different behavior for interfaces.
  JavaClassObj::Raw sub_mirror = *decode_handle(sub);
  GUARANTEE(sub_mirror.not_null(), "null argument to GetSuperclass");
  JavaClass::Raw sub_class = sub_mirror().java_class();

  if (!sub_class().is_interface()) {
    JavaClass::Raw super_class = sub_class().super();

    if (super_class.not_null()) {
      JavaClassObj::Raw m = super_class().java_mirror();
      return new_local_ref_for_oop(env, &m);
    }
  }

  return NULL;
}

static jboolean JNICALL
_JNI_IsAssignableFrom(JNIEnv *env, jclass sub, jclass sup) {
  if (sub == NULL || sup == NULL) {
    return JNI_FALSE;
  }

  return KNI_IsAssignableFrom(sub, sup);
}

static jint JNICALL
_JNI_Throw(JNIEnv *env, jthrowable obj) {
  if (obj == NULL) {
    return JNI_ERR;
  }

  Oop::Raw oop = *decode_handle(obj);
  if (oop.is_null()) {
    return JNI_ERR;
  }

  JavaClass::Raw object_class = oop.blueprint();
  if (!object_class().is_subtype_of(Universe::throwable_class())) {
    return JNI_ERR;
  }

  Thread::set_current_pending_exception(&oop);
  return JNI_OK;
}

static jthrowable JNICALL
_JNI_ExceptionOccured(JNIEnv *env) {
  Oop::Raw exc = Thread::current_pending_exception();

  if (exc.is_null()) {
    return NULL;
  }

  return new_local_ref_for_oop(env, &exc);
}

static void JNICALL
_JNI_ExceptionDescribe(JNIEnv *env) {
  Throwable::Raw exc = Thread::current_pending_exception();

  if (exc.is_null()) {
    return;
  }

  exc().print_stack_trace();
}

static void JNICALL
_JNI_ExceptionClear(JNIEnv *env) {
  Thread::clear_current_pending_exception();
}

static void JNICALL
_JNI_FatalError(JNIEnv *env, const char * msg) {
  KNI_FatalError(msg);
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
  if (ref1 == NULL || ref2 == NULL) {
    return ref1 == ref2 ? JNI_TRUE : JNI_FALSE;
  }
  return KNI_IsSameObject(ref1, ref2);
}

static jobject JNICALL
_JNI_NewLocalRef(JNIEnv* env, jobject ref) {
  if (ref == NULL) {
    return NULL;
  }

  UsingFastOops fast_oops;
  Oop::Fast oop = *decode_handle(ref);

  return new_local_ref_for_oop(env, &oop);
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
_JNI_AllocObject(JNIEnv *env, jclass clazz) {
  SETUP_ERROR_CHECKER_ARG;

  if (clazz == NULL) {
    return NULL;
  }

  JavaClassObj::Raw cls_mirror = *decode_handle(clazz);
  GUARANTEE(cls_mirror.not_null(), "null argument to GetSuperclass");
  JavaClass::Raw cls = cls_mirror().java_class();

  if (!cls.is_instance_class() || 
      cls().is_interface() || cls().is_abstract()) {
    Throw::instantiation(ExceptionOnFailure JVM_THROW_0);
  }

  UsingFastOops fast_oops;
  InstanceClass::Fast instance_cls = cls.obj();

  Oop::Raw obj = Universe::new_instance(&instance_cls JVM_CHECK_0);

  return new_local_ref_for_oop(env, &obj);
}

static jclass JNICALL
_JNI_GetObjectClass(JNIEnv *env, jobject obj) {
  if (obj == NULL) {
    return NULL;
  }

  jobject clazz = new_local_ref(env);

  KNI_GetObjectClass(obj, clazz);

  if (*decode_handle(clazz) == NULL) {
    return NULL;
  }

  return clazz;
}

static jboolean JNICALL
_JNI_IsInstanceOf(JNIEnv *env, jobject obj, jclass clazz) {
  if (obj == NULL || clazz == NULL) {
    return JNI_FALSE;
  }

  return KNI_IsInstanceOf(obj, clazz);
}

static jfieldID JNICALL 
_JNI_GetFieldID(JNIEnv *env, jclass clazz, const char *name, const char *sig) {
  // IMPL_NOTE: if the class is not initialized yet,
  // we must initialize it here.
  // IMPL_NOTE: throw NoSuchFieldError if the field is not found
  return KNI_GetFieldID(clazz, name, sig);
}

static jobject JNICALL
_JNI_GetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID) {
  jobject toHandle = new_local_ref(env);
  if (toHandle == NULL) {
    return NULL;
  }

  KNI_GetObjectField(obj, fieldID, toHandle);

  return toHandle;
}

#define DEFINE_GET_INSTANCE_FIELD(jtype, Type) \
static jtype JNICALL \
_JNI_Get ## Type ## Field(JNIEnv *env, jobject obj, jfieldID fieldID) { \
  return KNI_Get ## Type ## Field(obj, fieldID); \
}

FOR_ALL_PRIMITIVE_TYPES(DEFINE_GET_INSTANCE_FIELD)

static void JNICALL
_JNI_SetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID, jobject val) {
  KNI_SetObjectField(obj, fieldID, val);
}

#define DEFINE_SET_INSTANCE_FIELD(jtype, Type) \
static void JNICALL \
_JNI_Set ## Type ## Field(JNIEnv *env, jobject obj, jfieldID fieldID, \
                          jtype val) { \
  KNI_Set ## Type ## Field(obj, fieldID, val); \
}

FOR_ALL_PRIMITIVE_TYPES(DEFINE_SET_INSTANCE_FIELD)

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

static jboolean JNICALL
_JNI_ExceptionCheck(JNIEnv *env) {
  return (CURRENT_HAS_PENDING_EXCEPTION) ? JNI_TRUE : JNI_FALSE;
}


static struct JNINativeInterface _jni_native_interface = {
    NULL, // reserved0
    NULL, // reserved1
    NULL, // reserved2

    NULL, // reserved3

    _JNI_GetVersion, // GetVersion

    NULL, // DefineClass
    _JNI_FindClass, // FindClass

    NULL, // FromReflectedMethod
    NULL, // FromReflectedField

    NULL, // ToReflectedMethod

    _JNI_GetSuperclass, // GetSuperclass
    _JNI_IsAssignableFrom, // IsAssignableFrom

    NULL, // ToReflectedField

    _JNI_Throw, // Throw
    NULL, // ThrowNew
    _JNI_ExceptionOccured, // ExceptionOccurred
    _JNI_ExceptionDescribe, // ExceptionDescribe
    _JNI_ExceptionClear, // ExceptionClear
    _JNI_FatalError, // FatalError

    _JNI_PushLocalFrame, // PushLocalFrame
    _JNI_PopLocalFrame, // PopLocalFrame

    _JNI_NewGlobalRef,    // NewGlobalRef
    _JNI_DeleteGlobalRef, // DeleteGlobalRef
    _JNI_DeleteLocalRef, // DeleteLocalRef
    _JNI_IsSameObject, // IsSameObject

    _JNI_NewLocalRef, // NewLocalRef
    _JNI_EnsureLocalCapacity, // EnsureLocalCapacity

    _JNI_AllocObject, // AllocObject
    NULL, // NewObject
    NULL, // NewObjectV
    NULL, // NewObjectA

    _JNI_GetObjectClass, // GetObjectClass
    _JNI_IsInstanceOf, // IsInstanceOf

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

    _JNI_GetFieldID, // GetFieldID

    _JNI_GetObjectField, // GetObjectField
    _JNI_GetBooleanField, // GetBooleanField
    _JNI_GetByteField, // GetByteField
    _JNI_GetCharField, // GetCharField
    _JNI_GetShortField, // GetShortField
    _JNI_GetIntField, // GetIntField
    _JNI_GetLongField, // GetLongField
    _JNI_GetFloatField, // GetFloatField
    _JNI_GetDoubleField, // GetDoubleField

    _JNI_SetObjectField, // SetObjectField
    _JNI_SetBooleanField, // SetBooleanField
    _JNI_SetByteField, // SetByteField
    _JNI_SetCharField, // SetCharField
    _JNI_SetShortField, // SetShortField
    _JNI_SetIntField, // SetIntField
    _JNI_SetLongField, // SetLongField
    _JNI_SetFloatField, // SetFloatField
    _JNI_SetDoubleField, // SetDoubleField

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

    _JNI_ExceptionCheck, // ExceptionCheck

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
