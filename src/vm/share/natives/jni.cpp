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
  template(jboolean, Boolean, BOOLEAN) \
  template(jbyte,    Byte,    BYTE) \
  template(jchar,    Char,    CHAR) \
  template(jshort,   Short,   SHORT) \
  template(jint,     Int,     INT) \
  template(jlong,    Long,    LONG) \
  template(jfloat,   Float,   FLOAT) \
  template(jdouble,  Double,  DOUBLE)

#define FOR_ALL_TYPES(template) \
  FOR_ALL_PRIMITIVE_TYPES(template) \
  template(jobject,  Object,  OBJECT)

/*
 * Helper function. 
 * For a given local reference, deletes the reference and returns NULL 
 * if the referent is NULL, otherwise just returns the reference.
 */
static inline jobject local_ref_or_null(JNIEnv* env, jobject local_ref) {
  GUARANTEE(local_ref != NULL, "Non-null handle expected");
  if (*decode_handle(local_ref) == NULL) {
    env->DeleteLocalRef(local_ref);
    return NULL;
  }

  return local_ref;
}

static inline jobject
new_local_ref_for_oop(JNIEnv* env, Oop * oop) {
  jint ret = env->EnsureLocalCapacity(1);
  if (ret != JNI_OK) {
    return NULL;
  }

  return (jobject)ObjectHeap::register_local_reference(oop);
}

static inline jobject
new_local_ref(JNIEnv* env) {
  Oop dummy;
  return new_local_ref_for_oop(env, &dummy);
}

static inline jobject
new_local_ref_or_null_for_oop(JNIEnv* env, Oop * oop) {
  return oop->not_null() ? new_local_ref_for_oop(env, oop) : NULL;
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
      return new_local_ref_or_null_for_oop(env, &m);
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

  return new_local_ref_or_null_for_oop(env, &exc);
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

  return local_ref_or_null(env, clazz);
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
  if (clazz == NULL) {
    return NULL;
  }
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

#define DEFINE_GET_INSTANCE_FIELD(jtype, Type, TYPE) \
static jtype JNICALL \
_JNI_Get ## Type ## Field(JNIEnv *env, jobject obj, jfieldID fieldID) { \
  return KNI_Get ## Type ## Field(obj, fieldID); \
}

FOR_ALL_PRIMITIVE_TYPES(DEFINE_GET_INSTANCE_FIELD)

#define DEFINE_SET_INSTANCE_FIELD(jtype, Type, TYPE) \
static void JNICALL \
_JNI_Set ## Type ## Field(JNIEnv *env, jobject obj, jfieldID fieldID, \
                          jtype val) { \
  KNI_Set ## Type ## Field(obj, fieldID, val); \
}

FOR_ALL_TYPES(DEFINE_SET_INSTANCE_FIELD)

static jfieldID JNICALL 
_JNI_GetStaticFieldID(JNIEnv *env, jclass clazz, 
                      const char *name, const char *sig) {
  if (clazz == NULL) {
    return NULL;
  }
  // IMPL_NOTE: if the class is not initialized yet,
  // we must initialize it here.
  // IMPL_NOTE: throw NoSuchFieldError if the field is not found

  UsingFastOops fast_oops;
  JavaClassObj::Fast mirror = *decode_handle(clazz);
  InstanceClass::Fast ic = mirror().java_class();
  InstanceClass::Fast holder(ic.obj());

  GUARANTEE(ic.is_instance_class(), "sanity check");

  SETUP_ERROR_CHECKER_ARG;
  Symbol::Fast n = SymbolTable::check_symbol_for(name JVM_CHECK_0);
  Symbol::Fast s = TypeSymbol::parse((char*)sig JVM_CHECK_0);

  Field field(&holder, &n, &s);

#ifndef PRODUCT
  // NOTE: field could be renamed by Romizer.
  // Make sure you have used the DontRenameNonPublicFields option in the
  // -romconfig file for this class.
  if (!field.is_valid()) {
    Throw::no_such_field_error(JVM_SINGLE_ARG_THROW_0);
  }
#endif

  jfieldID fieldID = 0;

  if (field.is_valid() && field.is_static()) {
    jushort class_index = holder().class_id();
    jushort offset = field.offset();
    fieldID = (jfieldID)construct_jint_from_jushorts(class_index, offset);
  }

  return fieldID;
}

static inline jobject
field_holder(JNIEnv* env, jfieldID fieldID) {
  jushort class_id = extract_high_jushort_from_jint((jint)fieldID);
  UsingFastOops fast_oops;
  JavaClass::Fast java_class = Universe::class_from_id(class_id);
  JavaClassObj::Fast holder = java_class().java_mirror();
  return new_local_ref_or_null_for_oop(env, &holder);
}

static inline jint
field_offset(jfieldID fieldID) {
  return extract_low_jushort_from_jint((jint)fieldID);
}

static jobject JNICALL
_JNI_GetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID) {
  jobject holder = field_holder(env, fieldID);

  if (holder == NULL) {
    return NULL;
  }

  const jint offset = field_offset(fieldID);

  jobject toHandle = new_local_ref(env);
  if (toHandle == NULL) {
    return NULL;
  }

  // NOTE: assumed that KNI implementation uses offset as field ID.
  KNI_GetStaticObjectField(holder, (jfieldID)offset, toHandle);

  _JNI_DeleteLocalRef(env, holder);

  return toHandle;
}

#define DEFINE_GET_STATIC_FIELD(jtype, Type, TYPE) \
static jtype JNICALL \
_JNI_GetStatic ## Type ## Field(JNIEnv *env, jclass clazz, jfieldID fieldID) {\
  jobject holder = field_holder(env, fieldID);                          \
                                                                        \
  if (holder == NULL) {                                                 \
    return 0;                                                           \
  }                                                                     \
                                                                        \
  const jint offset = field_offset(fieldID);                            \
                                                                        \
  /* NOTE: assumed that KNI implementation uses offset as field ID. */  \
  jtype val = KNI_GetStatic ## Type ## Field(holder, (jfieldID)offset); \
                                                                        \
  _JNI_DeleteLocalRef(env, holder);                                     \
                                                                        \
  return val;                                                           \
}

FOR_ALL_PRIMITIVE_TYPES(DEFINE_GET_STATIC_FIELD)

#define DEFINE_SET_STATIC_FIELD(jtype, Type, TYPE) \
static void JNICALL \
_JNI_SetStatic ## Type ## Field(JNIEnv *env, jclass clazz, jfieldID fieldID, \
                                jtype val) { \
  jobject holder = field_holder(env, fieldID);                          \
                                                                        \
  if (holder == NULL) {                                                 \
    return;                                                             \
  }                                                                     \
                                                                        \
  const jint offset = field_offset(fieldID);                            \
                                                                        \
  /* NOTE: assumed that KNI implementation uses offset as field ID. */  \
  KNI_SetStatic ## Type ## Field(holder, (jfieldID)offset, val);        \
                                                                        \
  _JNI_DeleteLocalRef(env, holder);                                     \
}

FOR_ALL_TYPES(DEFINE_SET_STATIC_FIELD)

static jstring JNICALL
_JNI_NewString(JNIEnv *env, const jchar *unicode, jsize len) {
  jobject string = new_local_ref(env);

  if (string == NULL) {
    return NULL;
  }

  KNI_NewString(unicode, len, string);

  return string;
}

static jint JNICALL
_JNI_GetStringLength(JNIEnv *env, jstring str) {
  if (str == NULL) {
    return -1;
  }

  return KNI_GetStringLength(str);
}

static const jchar * JNICALL
_JNI_GetStringChars(JNIEnv *env, jstring str, jboolean *isCopy) {
  if (isCopy != NULL) {
    *isCopy = JNI_TRUE;
  }

  jint len = _JNI_GetStringLength(env, str);

  if (len < 0) {
    return NULL;
  }

  if (len == 0) {
    // See CR 5071855. malloc(0) can return NULL.
    return (jchar *)jvm_malloc(1);
  }

  jchar * buf = (jchar *)jvm_malloc(len * sizeof(jchar));
  if (buf == NULL) {
    return NULL;
  }

  KNI_GetStringRegion(str, 0, len, buf);

  return buf;
}

static void JNICALL
_JNI_ReleaseStringChars(JNIEnv *env, jstring str, const jchar *chars) {
  if (chars != NULL) {
    jvm_free((void*)chars);
  }
}

static jstring JNICALL
_JNI_NewStringUTF(JNIEnv *env, const char *utf) {
  jobject string = new_local_ref(env);

  if (string == NULL) {
    return NULL;
  }

  KNI_NewStringUTF(utf, string);

  return string;
}

static jint JNICALL
_JNI_GetStringUTFLength(JNIEnv *env, jstring str) {
  if (str == NULL) {
    return -1;
  }

  String::Raw string = *decode_handle(str);
  if (string.is_null()) {
    return -1;
  }

  TypeArray::Raw value = string().value();
  const jint offset = string().offset();
  const jint length = string().length();

  CharStream stream(&value, offset, length);
  return stream.utf8_length();
}

static const char * JNICALL
_JNI_GetStringUTFChars(JNIEnv *env, jstring str, jboolean *isCopy) {
  if (isCopy != NULL) {
    *isCopy = JNI_TRUE;
  }

  if (str == NULL) {
    return NULL;
  }

  String::Raw string = *decode_handle(str);
  if (string.is_null()) {
    return NULL;
  }

  TypeArray::Raw value = string().value();
  const jint offset = string().offset();
  const jint length = string().length();

  if (length == 0) {
    // See CR 5071855. malloc(0) can return NULL.
    return (char *)jvm_malloc(1);
  }

  const jint utf8len = length * 3;
  char * buf = (char *)jvm_malloc(utf8len);
  if (buf == NULL) {
    return NULL;
  }

  LiteralStream stream(buf, 0, utf8len);

  int i, index;
  for (index = 0, i = 0; i < length; i++) {
    jchar ch = value().char_at(i + offset);
    index = stream.utf8_write(index, ch);
  }
  GUARANTEE(index < utf8len, "UTF8 encoder failed");

  return buf;
}

static void JNICALL
_JNI_ReleaseStringUTFChars(JNIEnv *env, jstring str, const char *chars) {
  if (chars != NULL) {
    jvm_free((void*)chars);
  }
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

static jboolean JNICALL
_JNI_ExceptionCheck(JNIEnv *env) {
  return (CURRENT_HAS_PENDING_EXCEPTION) ? JNI_TRUE : JNI_FALSE;
}

static jsize JNICALL
_JNI_GetArrayLength(JNIEnv* env, jarray array) {
  if (array == NULL) {
    return -1;
  }

  return KNI_GetArrayLength(array);
}

static jobjectArray JNICALL
_JNI_NewObjectArray(JNIEnv *env, jsize len, jclass clazz, jobject init) {
  if (clazz == NULL) {
    return NULL;
  }

  jobject array = new_local_ref(env);

  if (array == NULL) {
    return NULL;
  }

  SNI_NewObjectArray(clazz, len, array);

  if (*decode_handle(array) == NULL) {
    SETUP_ERROR_CHECKER_ARG;
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
  }

  if (init != NULL) {
    for (int i = 0; i < len; i++) {
      KNI_SetObjectArrayElement(array, i, init);
    }
  }

  return array;
}

static jobject JNICALL
_JNI_GetObjectArrayElement(JNIEnv* env, jobjectArray array, jsize index) {
  if (array == NULL) {
    return NULL;
  }

  jobject element = new_local_ref(env);

  if (array == NULL) {
    return NULL;
  }

  jsize length = KNI_GetArrayLength(array);

  if (index < 0 || index >= length) {
    SETUP_ERROR_CHECKER_ARG;
    Throw::array_index_out_of_bounds_exception(empty_message JVM_THROW_0);
  }

  KNI_GetObjectArrayElement(array, index, element);

  return local_ref_or_null(env, element);
}

static void JNICALL
_JNI_SetObjectArrayElement(JNIEnv* env, jobjectArray array, jsize index,
                           jobject val) {
  if (array == NULL) {
    return;
  }

  jsize length = KNI_GetArrayLength(array);

  if (index < 0 || index >= length) {
    SETUP_ERROR_CHECKER_ARG;
    Throw::array_index_out_of_bounds_exception(empty_message JVM_THROW);
  }

  KNI_SetObjectArrayElement(array, index, val);
}

#define DEFINE_NEW_ARRAY(jtype, Type, TYPE)                        \
static jtype ## Array JNICALL                                      \
_JNI_New ## Type ## Array(JNIEnv *env, jsize len) {                \
  jobject array = new_local_ref(env);                              \
  if (array == NULL) {                                             \
    return NULL;                                                   \
  }                                                                \
                                                                   \
  SNI_NewArray(SNI_ ## TYPE ## _ARRAY, len, array);                \
                                                                   \
  if (*decode_handle(array) == NULL) {                             \
    SETUP_ERROR_CHECKER_ARG;                                       \
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);            \
  }                                                                \
                                                                   \
  return array;                                                    \
}

FOR_ALL_PRIMITIVE_TYPES(DEFINE_NEW_ARRAY)

#define DEFINE_GET_ARRAY_ELEMENTS(jtype, Type, TYPE)                 \
static jtype * JNICALL                                               \
_JNI_Get ## Type ## ArrayElements(JNIEnv *env, jtype ## Array array, \
                                  jboolean *isCopy) {                \
  if (isCopy != NULL) {                                              \
    *isCopy = JNI_TRUE;                                              \
  }                                                                  \
                                                                     \
  const jint length = _JNI_GetArrayLength(env, array);               \
                                                                     \
  if (length < 0) {                                                  \
    return NULL;                                                     \
  }                                                                  \
                                                                     \
  if (length == 0) {                                                 \
    /* See CR 5071855. malloc(0) can return NULL. */                 \
    return (jtype *)jvm_malloc(1);                                   \
  }                                                                  \
                                                                     \
  const jsize size = length * sizeof(jtype);                         \
  jtype * buf = (jtype *)jvm_malloc(size);                           \
  if (buf == NULL) {                                                 \
    return NULL;                                                     \
  }                                                                  \
                                                                     \
  KNI_GetRawArrayRegion(array, 0, size, (jbyte*)buf);                \
                                                                     \
  return buf;                                                        \
}                                                                    

FOR_ALL_PRIMITIVE_TYPES(DEFINE_GET_ARRAY_ELEMENTS)
                                                                     
#define DEFINE_RELEASE_ARRAY_ELEMENTS(jtype, Type, TYPE)             \
static void JNICALL                                                  \
_JNI_Release ## Type ## ArrayElements(JNIEnv *env,                   \
                                      jtype ## Array array,          \
                                      jtype * elems, jint mode) {    \
  switch (mode) {                                                    \
  case 0:                                                            \
  case JNI_COMMIT:                                                   \
    const jint length = _JNI_GetArrayLength(env, array);             \
    if (length > 0) {                                                \
      KNI_SetRawArrayRegion(array, 0, length * sizeof(jtype),        \
                            (const jbyte*)elems);                    \
    }                                                                \
  }                                                                  \
                                                                     \
  switch (mode) {                                                    \
  case 0:                                                            \
  case JNI_ABORT:                                                    \
    jvm_free((void*)elems);                                          \
  }                                                                  \
}

FOR_ALL_PRIMITIVE_TYPES(DEFINE_RELEASE_ARRAY_ELEMENTS)

#define DEFINE_GET_ARRAY_REGION(jtype, Type, TYPE)                       \
static void JNICALL                                                      \
_JNI_Get ## Type ## ArrayRegion(JNIEnv *env, jtype ## Array array,       \
                                jsize start, jsize len, jtype * buf) {   \
  const jint array_len = _JNI_GetArrayLength(env, array);                \
  if (array_len < 0) {                                                   \
    return;                                                              \
  }                                                                      \
                                                                         \
  if (start < 0 || len < 0 ||                                            \
      (juint)len + (juint)start > (juint)array_len) {                    \
    SETUP_ERROR_CHECKER_ARG;                                             \
    Throw::array_index_out_of_bounds_exception(empty_message JVM_THROW); \
  }                                                                      \
                                                                         \
  const jsize offset = start * sizeof(jtype);                            \
  const jsize size = len * sizeof(jtype);                                \
  KNI_GetRawArrayRegion(array, offset, size, (jbyte*)buf);               \
}

FOR_ALL_PRIMITIVE_TYPES(DEFINE_GET_ARRAY_REGION)

#define DEFINE_SET_ARRAY_REGION(jtype, Type, TYPE)                       \
static void JNICALL                                                      \
_JNI_Set ## Type ## ArrayRegion(JNIEnv *env, jtype ## Array array,       \
                                jsize start, jsize len,                  \
                                const jtype * buf) {                     \
  const jint array_len = _JNI_GetArrayLength(env, array);                \
  if (array_len < 0) {                                                   \
    return;                                                              \
  }                                                                      \
                                                                         \
  if (start < 0 || len < 0 ||                                            \
      (juint)len + (juint)start > (juint)array_len) {                    \
    SETUP_ERROR_CHECKER_ARG;                                             \
    Throw::array_index_out_of_bounds_exception(empty_message JVM_THROW); \
  }                                                                      \
                                                                         \
  const jsize offset = start * sizeof(jtype);                            \
  const jsize size = len * sizeof(jtype);                                \
  KNI_SetRawArrayRegion(array, offset, size, (const jbyte*)buf);         \
}

FOR_ALL_PRIMITIVE_TYPES(DEFINE_SET_ARRAY_REGION)

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

    _JNI_GetStaticFieldID, // GetStaticFieldID

    _JNI_GetStaticObjectField, // GetStaticObjectField
    _JNI_GetStaticBooleanField, // GetStaticBooleanField
    _JNI_GetStaticByteField, // GetStaticByteField
    _JNI_GetStaticCharField, // GetStaticCharField
    _JNI_GetStaticShortField, // GetStaticShortField
    _JNI_GetStaticIntField, // GetStaticIntField
    _JNI_GetStaticLongField, // GetStaticLongField
    _JNI_GetStaticFloatField, // GetStaticFloatField
    _JNI_GetStaticDoubleField, // GetStaticDoubleField

    _JNI_SetStaticObjectField, // SetStaticObjectField
    _JNI_SetStaticBooleanField, // SetStaticBooleanField
    _JNI_SetStaticByteField, // SetStaticByteField
    _JNI_SetStaticCharField, // SetStaticCharField
    _JNI_SetStaticShortField, // SetStaticShortField
    _JNI_SetStaticIntField, // SetStaticIntField
    _JNI_SetStaticLongField, // SetStaticLongField
    _JNI_SetStaticFloatField, // SetStaticFloatField
    _JNI_SetStaticDoubleField, // SetStaticDoubleField

    _JNI_NewString, // NewString
    _JNI_GetStringLength, // GetStringLength
    _JNI_GetStringChars, // GetStringChars
    _JNI_ReleaseStringChars, // ReleaseStringChars

    _JNI_NewStringUTF, // NewStringUTF
    _JNI_GetStringUTFLength, // GetStringUTFLength
    _JNI_GetStringUTFChars, // GetStringUTFChars
    _JNI_ReleaseStringUTFChars, // ReleaseStringUTFChars

    _JNI_GetArrayLength, // GetArrayLength
 
    _JNI_NewObjectArray, // NewObjectArray
    _JNI_GetObjectArrayElement, // GetObjectArrayElement
    _JNI_SetObjectArrayElement, // SetObjectArrayElement

    _JNI_NewBooleanArray, // NewBooleanArray
    _JNI_NewByteArray, // NewByteArray
    _JNI_NewCharArray, // NewCharArray
    _JNI_NewShortArray, // NewShortArray
    _JNI_NewIntArray, // NewIntArray
    _JNI_NewLongArray, // NewLongArray
    _JNI_NewFloatArray, // NewFloatArray
    _JNI_NewDoubleArray, // NewDoubleArray

    _JNI_GetBooleanArrayElements, // GetBooleanArrayElements
    _JNI_GetByteArrayElements, // GetByteArrayElements
    _JNI_GetCharArrayElements, // GetCharArrayElements
    _JNI_GetShortArrayElements, // GetShortArrayElements
    _JNI_GetIntArrayElements, // GetIntArrayElements
    _JNI_GetLongArrayElements, // GetLongArrayElements
    _JNI_GetFloatArrayElements, // GetFloatArrayElements
    _JNI_GetDoubleArrayElements, // GetDoubleArrayElements

    _JNI_ReleaseBooleanArrayElements, // ReleaseBooleanArrayElements
    _JNI_ReleaseByteArrayElements, // ReleaseByteArrayElements
    _JNI_ReleaseCharArrayElements, // ReleaseCharArrayElements
    _JNI_ReleaseShortArrayElements, // ReleaseShortArrayElements
    _JNI_ReleaseIntArrayElements, // ReleaseIntArrayElements
    _JNI_ReleaseLongArrayElements, // ReleaseLongArrayElements
    _JNI_ReleaseFloatArrayElements, // ReleaseFloatArrayElements
    _JNI_ReleaseDoubleArrayElements, // ReleaseDoubleArrayElements

    _JNI_GetBooleanArrayRegion, // GetBooleanArrayRegion
    _JNI_GetByteArrayRegion, // GetByteArrayRegion
    _JNI_GetCharArrayRegion, // GetCharArrayRegion
    _JNI_GetShortArrayRegion, // GetShortArrayRegion
    _JNI_GetIntArrayRegion, // GetIntArrayRegion
    _JNI_GetLongArrayRegion, // GetLongArrayRegion
    _JNI_GetFloatArrayRegion, // GetFloatArrayRegion
    _JNI_GetDoubleArrayRegion, // GetDoubleArrayRegion

    _JNI_SetBooleanArrayRegion, // SetBooleanArrayRegion
    _JNI_SetByteArrayRegion, // SetByteArrayRegion
    _JNI_SetCharArrayRegion, // SetCharArrayRegion
    _JNI_SetShortArrayRegion, // SetShortArrayRegion
    _JNI_SetIntArrayRegion, // SetIntArrayRegion
    _JNI_SetLongArrayRegion, // SetLongArrayRegion
    _JNI_SetFloatArrayRegion, // SetFloatArrayRegion
    _JNI_SetDoubleArrayRegion, // SetDoubleArrayRegion

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
