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
    GUARANTEE(!Thread::current()->has_pending_entries(),
              "Must be no pending entries at this point");

    ((InstanceClass*)&cl)->initialize(JVM_SINGLE_ARG_CHECK_0);

    if (Thread::current()->has_pending_entries()) {
      invoke_entry_void();

      if (env->ExceptionCheck()) {
        return NULL;
      }
    }
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

static jint JNICALL 
_JNI_ThrowNew(JNIEnv *env, jclass clazz, const char *msg) {
  UsingFastOops fast_oops;
  SETUP_ERROR_CHECKER_ARG;

  if (clazz == NULL) {
    return JNI_ERR;
  }

  JavaClassObj::Fast class_mirror = *decode_handle(clazz);
  if (class_mirror.is_null()) {
    return JNI_ERR;
  }

  InstanceClass::Fast ic = class_mirror().java_class();
  if (ic.is_null()) {
    return JNI_ERR;
  }

  //make sure consecutive ThrowNew calls don't destroy one another
  Thread::clear_current_pending_exception();

  Symbol::Fast class_name = ic().name();

  if (Symbols::java_lang_OutOfMemoryError()->equals(&class_name)) {
    // Avoid allocating the exception object when we're running out of memory.
    Throw::out_of_memory_error(JVM_SINGLE_ARG_NO_CHECK);
    return JNI_OK;
  }
    
  String::Fast str;
  if(msg != NULL) {
    str = Universe::new_string(msg, jvm_strlen(msg) JVM_CHECK_(JNI_ERR));
  } 

  {
    Throwable::Raw exception 
      = Throw::new_exception(&class_name, &str JVM_NO_CHECK);
    if (exception.not_null()) {
      Thread::set_current_pending_exception(&exception);
    }
  }

  return JNI_OK;
}

static jthrowable JNICALL
_JNI_ExceptionOccurred(JNIEnv *env) {
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
  if (ref_index < 0) {
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
  }
  
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
  OopDesc* object1 = NULL;
  OopDesc* object2 = NULL;
  if (ref1 != NULL) {
    object1 = *decode_handle(ref1);
  }
  if (ref2 != NULL) {
    object2 = *decode_handle(ref2);
  }
  return object1 == object2 ? JNI_TRUE : JNI_FALSE;
}

static jobject JNICALL
_JNI_NewLocalRef(JNIEnv* env, jobject ref) {
  if (ref == NULL) {
    return NULL;
  }

  UsingFastOops fast_oops;
  Oop::Fast oop = *decode_handle(ref);

  return new_local_ref_or_null_for_oop(env, &oop);
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

  if (requested_length < 0) {
    // Overflow
    return JNI_ERR;
  }

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
  UsingFastOops fast_oops;

  if (clazz == NULL) {
    return NULL;
  }

  JavaClassObj::Fast cls_mirror = *decode_handle(clazz);
  GUARANTEE(cls_mirror.not_null(), "null argument to GetSuperclass");
  JavaClass::Fast cls = cls_mirror().java_class();

  if (!cls.is_instance_class() || 
      cls().is_interface() || cls().is_abstract()) {
    Throw::instantiation(ExceptionOnFailure JVM_THROW_0);
  }

  InstanceClass::Fast instance_cls = cls.obj();

  Oop::Fast obj = Universe::new_instance(&instance_cls JVM_CHECK_0);

  return new_local_ref_for_oop(env, &obj);
}

static jobject JNICALL 
_JNI_NewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  jobject obj = env->AllocObject(clazz);
  
  if (obj == NULL) {
    return NULL;
  }
  
  va_list args;
  va_start(args, methodID);
  env->CallNonvirtualVoidMethodV(obj, clazz, methodID, args);
  va_end(args);

  return env->ExceptionCheck() ? NULL : obj;
}

static jobject JNICALL 
_JNI_NewObjectV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args) {
  jobject obj = env->AllocObject(clazz);
  
  if (obj == NULL) {
    return NULL;
  }
  
  env->CallNonvirtualVoidMethodV(obj, clazz, methodID, args);

  return env->ExceptionCheck() ? NULL : obj;
}

static jobject JNICALL 
_JNI_NewObjectA(JNIEnv *env, jclass clazz, jmethodID methodID, 
                const jvalue *args) {
  jobject obj = env->AllocObject(clazz);
  
  if (obj == NULL) {
    return NULL;
  }
  
  env->CallNonvirtualVoidMethodA(obj, clazz, methodID, args);

  return env->ExceptionCheck() ? NULL : obj;
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
  if (clazz == NULL || *decode_handle(clazz) == NULL) {
    env->FatalError("null class arg for IsInstanceOf");
    return JNI_FALSE;
  }

  if (obj == NULL) {
    return JNI_TRUE;
  }

  return KNI_IsInstanceOf(obj, clazz);
}

static jmethodID JNICALL 
_JNI_GetMethodID(JNIEnv *env, jclass clazz, 
                 const char *name, const char *sig) {
  if (clazz == NULL) {
    env->FatalError("null class arg for GetMethodID");
    return NULL;
  }

  UsingFastOops fast_oops;
  JavaClassObj::Fast class_mirror = *decode_handle(clazz);
  if (class_mirror.is_null()) {
    env->FatalError("null class arg for GetMethodID");
    return NULL;
  }

  InstanceClass::Fast ic = class_mirror().java_class();

  SETUP_ERROR_CHECKER_ARG;
  Symbol::Fast n = SymbolTable::symbol_for(name JVM_CHECK_0);
  Symbol::Fast s = TypeSymbol::parse((char*)sig JVM_CHECK_0);

  Method::Fast m;
  if (Symbols::object_initializer_name()->equals(&n)) {
    m = ic().find_local_method(&n, &s, /*non-static only*/ true);
  } else {
    m = ic().lookup_method(&n, &s, /*non-static only*/ true);
  }

  if (m.is_null()) {
    Throw::no_such_method_error(JVM_SINGLE_ARG_THROW_0);
  }

  InstanceClass::Fast holder = m().holder();
  const jushort class_id = holder().class_id();

  int mtable_index = m().method_table_index();

  jushort method_id = 0;

  if (mtable_index >= 0) {
    if (mtable_index > JniMethodIndexMask) {
      // Cannot encode
      env->FatalError("too many methods in class");
      return NULL;
    }

    method_id = mtable_index | JniMtableMethod;
  } else {
    int vtable_index = m().vtable_index();

    if (vtable_index > JniMethodIndexMask) {
      // Cannot encode
      env->FatalError("too many methods in class");
      return NULL;
    }

    if (vtable_index < 0) {
      env->FatalError("method lookup failed");
      return NULL;
    }

    method_id = vtable_index | JniVtableMethod;
  }

  return (jmethodID)construct_jint_from_jushorts(class_id, method_id);
}

static jfieldID JNICALL 
_JNI_GetFieldID(JNIEnv *env, jclass clazz, const char *name, const char *sig) {
  if (clazz == NULL) {
    return NULL;
  }
  // IMPL_NOTE: if the class is not initialized yet,
  // we must initialize it here.
  // IMPL_NOTE: throw NoSuchFieldError if the field is not found
  jfieldID fieldID = KNI_GetFieldID(clazz, name, sig);
  if (fieldID == NULL) {
    SETUP_ERROR_CHECKER_ARG;
    Throw::no_such_field_error(JVM_SINGLE_ARG_THROW_0);
  }

  return fieldID;
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

static jmethodID
_JNI_GetStaticMethodID(JNIEnv *env, jclass clazz, 
                       const char *name, const char *sig) {
  if (clazz == NULL) {
    env->FatalError("null class arg for GetMethodID");
    return NULL;
  }

  UsingFastOops fast_oops;
  JavaClassObj::Fast class_mirror = *decode_handle(clazz);
  if (class_mirror.is_null()) {
    env->FatalError("null class arg for GetMethodID");
    return NULL;
  }

  InstanceClass::Fast ic = class_mirror().java_class();

  SETUP_ERROR_CHECKER_ARG;
  Symbol::Fast n = SymbolTable::symbol_for(name JVM_CHECK_0);
  Symbol::Fast s = TypeSymbol::parse((char*)sig JVM_CHECK_0);

  Method::Fast m;
  if (Symbols::class_initializer_name()->equals(&n)) {
    m = ic().find_local_method(&n, &s, /*non-static only*/ false);
  } else {
    m = ic().lookup_method(&n, &s, /*non-static only*/ false);
  }

  if (m.is_null()) {
    Throw::no_such_method_error(JVM_SINGLE_ARG_THROW_0);
  }

  InstanceClass::Fast holder = m().holder();
  const jushort class_id = holder().class_id();

  const int mtable_index = m().method_table_index();

  if (mtable_index > JniMethodIndexMask) {
    // Cannot encode
    env->FatalError("too many methods in class");
    return NULL;
  }

  if (mtable_index < 0) {
    env->FatalError("static method lookup failed");
    return NULL;
  }

  const jushort method_id = mtable_index | JniMtableMethod;
  return (jmethodID)construct_jint_from_jushorts(class_id, method_id);
}

static ReturnOop 
new_entry_activation(JNIEnv *env, jobject obj, jclass cls, jmethodID methodID,
                     Signature *sig, 
                     bool is_virtual_call, bool is_static_call) {
  GUARANTEE(!is_static_call || !is_virtual_call, "Static virtual call");

  if (env->ExceptionCheck()) {
    env->FatalError("pending exception");
    return NULL;
  }

  if (methodID == NULL) {
    env->FatalError("null method id");
    return NULL;
  }

  const jushort method_id = extract_low_jushort_from_jint((jint)methodID);
  const jushort method_index = method_id & JniMethodIndexMask;
  const juint class_id = extract_high_jushort_from_jint((jint)methodID);

  const bool is_virtual_method = 
    (method_id & JniVtableMethod) == JniVtableMethod;

  if (is_static_call && is_virtual_method) {
    env->FatalError("Invalid method ID: static call of virtual method");
    return NULL;
  }

  InstanceClass::Raw method_holder = Universe::class_from_id(class_id);

  if (method_holder.is_null()) {
    env->FatalError("Invalid method ID: no holder");
    return NULL;
  }

  const bool is_interface_method = method_holder().is_interface();

  InstanceClass::Raw receiver_class;

  if (!is_static_call) {
    Instance::Raw receiver;
    if (obj != NULL) {
      receiver = *decode_handle(obj);
    }

    if (receiver.is_null()) {
      jclass exc_clazz = env->FindClass("java/lang/NullPointerException");
      if (exc_clazz != NULL) {
        env->ThrowNew(exc_clazz, "null object");
      } else {
        env->FatalError("Cannot find java/lang/NullPointerException class");
      }
      return NULL;
    }

    receiver_class = receiver().blueprint();

    GUARANTEE(receiver_class.not_null(), "Null receiver class");

    if (!receiver_class().is_subtype_of(&method_holder)) {
      env->FatalError(
        "Invalid method ID: receiver not assignable to method holder class");
      return NULL;
    }
  }

  if (is_virtual_call) {
    if (is_virtual_method || is_interface_method) {
      method_holder = receiver_class;
    }
  } else {
    // NOTE: just a verification of arguments. Disable for non-debug version?
    if (cls == NULL) {
      env->FatalError("null class");
      return NULL;
    }
    
    JavaClassObj::Raw class_mirror = *decode_handle(cls);
    if (class_mirror.is_null()) {
      env->FatalError("null class");
      return NULL;
    }

    JavaClass::Raw java_class = class_mirror().java_class();

    if (java_class().class_id() != class_id) {
      env->FatalError(
        "Invalid method ID: method class does not match passed class");
      return NULL;
    }
  }

  GUARANTEE(method_holder.not_null(), "Null holder class");

  SETUP_ERROR_CHECKER_ARG;
  UsingFastOops fast_oops;
  Method::Fast method;
  ClassInfo::Fast ci = method_holder().class_info();

  if (is_virtual_method) {
    if (method_index >= ci().vtable_length()) {
      env->FatalError("Invalid method ID");
      return NULL;
    }
    
    method = ci().vtable_method_at(method_index);
  } else {
    if (is_interface_method) {
      const jushort itable_length = ci().itable_length();

      for (int index = 0; index < itable_length; index++) {
        if (ci().itable_interface_class_id_at(index) == class_id) {
          int offset = ci().itable_offset_at(index);

          method = ci().obj_field(offset + method_index * sizeof(jobject));
        }
      }
    } else {
      ObjArray::Raw methods = method_holder().methods();

      if (method_index >= methods().length()) {
        env->FatalError("Invalid method ID");
        return NULL;
      }

      method = methods().obj_at(method_index);
    }
  }

  GUARANTEE(method.not_null(), "Cannot find method");
  GUARANTEE(is_static_call == method().is_static(), "Cannot find method");

  sig->set_obj(method().signature());

  const jint param_size = sig->parameter_word_size(is_static_call);

  return Universe::new_entry_activation(&method, param_size 
                                        JVM_NO_CHECK_AT_BOTTOM);
}

static ReturnOop 
new_entry_activation_V(JNIEnv *env, jobject obj, jclass cls, 
                       jmethodID methodID, va_list args,
                       bool is_virtual, bool is_static) {
  UsingFastOops fast_oops;
  Signature::Fast sig;
  EntryActivation::Fast entry = 
    new_entry_activation(env, obj, cls, methodID, &sig,
                         is_virtual, is_static);

  if (entry.is_null()) {
    return NULL;
  }

  GUARANTEE(!env->ExceptionCheck(), "Unexpected exception");
  GUARANTEE(sig.not_null(), "Null signature");

  SignatureStream ss(&sig, is_static, true/*fast*/);

  if (!is_static) {
    GUARANTEE(!ss.eos() && ss.type() == T_OBJECT, 
              "Invalid type for receiver arg");

    Oop::Raw oop;
      
    if (obj != NULL) {
      oop = *decode_handle(obj);
    }

    entry().obj_at_put(0, &oop);

    ss.next();
  }

  for ( ; !ss.eos(); ss.next()) {
    switch (ss.type()) {
    case T_BOOLEAN:
    case T_CHAR:
    case T_BYTE:
    case T_SHORT:
    case T_INT:
      entry().int_at_put(ss.index(), va_arg(args, jint));
      break;
    case T_FLOAT:
      // `float' is promoted to `double' when passed through `...'
      entry().float_at_put(ss.index(), (jfloat)va_arg(args, jdouble));
      break;
    case T_LONG:
      entry().long_at_put(ss.index(), va_arg(args, jlong));
      break;
    case T_DOUBLE:
      entry().double_at_put(ss.index(), va_arg(args, jdouble));
      break;
    case T_OBJECT: {
      jobject object_arg = va_arg(args, jobject);
      Oop::Raw oop;
      
      if (object_arg != NULL) {
        oop = *decode_handle(object_arg);
      }

      entry().obj_at_put(ss.index(), &oop);
      break;
    }
    default:
      SHOULD_NOT_REACH_HERE();
    }
  }

  return entry.obj();
}

static ReturnOop 
new_entry_activation_A(JNIEnv *env, jobject obj, jclass cls, 
                       jmethodID methodID, const jvalue * args,
                       bool is_virtual, bool is_static) {
  UsingFastOops fast_oops;
  Signature::Fast sig;
  EntryActivation::Fast entry = 
    new_entry_activation(env, obj, cls, methodID, &sig, 
                         is_virtual, is_static);

  if (entry.is_null()) {
    return NULL;
  }

  GUARANTEE(!env->ExceptionCheck(), "Unexpected exception");
  GUARANTEE(sig.not_null(), "Null signature");

  SignatureStream ss(&sig, is_static, true/*fast*/);

  if (!is_static) {
    GUARANTEE(!ss.eos() && ss.type() == T_OBJECT, 
              "Invalid type for receiver arg");

    Oop::Raw oop;
      
    if (obj != NULL) {
      oop = *decode_handle(obj);
    }

    entry().obj_at_put(0, &oop);

    ss.next();
  }

  for ( ; !ss.eos(); ss.next(), args++) {
    switch (ss.type()) {
    case T_BOOLEAN:
      entry().int_at_put(ss.index(), args->z);
      break;
    case T_CHAR:
      entry().int_at_put(ss.index(), args->c);
      break;
    case T_BYTE:
      entry().int_at_put(ss.index(), args->b);
      break;
    case T_SHORT:
      entry().int_at_put(ss.index(), args->s);
      break;
    case T_INT:
      entry().int_at_put(ss.index(), args->i);
      break;
    case T_FLOAT:
      entry().float_at_put(ss.index(), args->f);
      break;
    case T_LONG:
      entry().long_at_put(ss.index(), args->j);
      break;
    case T_DOUBLE:
      entry().double_at_put(ss.index(), args->d);
      break;
    case T_OBJECT: {
      jobject obj = args->l;
      Oop::Raw oop;
      
      if (obj != NULL) {
        oop = *decode_handle(obj);
      }

      entry().obj_at_put(ss.index(), &oop);
      break;
    }
    default:
      SHOULD_NOT_REACH_HERE();
    }
  }

  return entry.obj();
}

#define VIRTUAL_NAME
#define NON_VIRTUAL_NAME   Nonvirtual
#define STATIC_NAME        Static
#define RECEIVER_DECL_VIRTUAL      jobject obj,
#define RECEIVER_DECL_NON_VIRTUAL  jobject obj,
#define RECEIVER_DECL_STATIC
#define CLASS_DECL_VIRTUAL     
#define CLASS_DECL_NON_VIRTUAL     jclass cls,
#define CLASS_DECL_STATIC          jclass cls,
#define DUMMY_RECEIVER_VIRTUAL 
#define DUMMY_RECEIVER_NON_VIRTUAL 
#define DUMMY_RECEIVER_STATIC      jobject obj = NULL;
#define DUMMY_CLASS_VIRTUAL        jclass cls = NULL; 
#define DUMMY_CLASS_NON_VIRTUAL
#define DUMMY_CLASS_STATIC
#define IS_VIRTUAL_VIRTUAL     true
#define IS_VIRTUAL_NON_VIRTUAL false
#define IS_VIRTUAL_STATIC      false
#define IS_STATIC_VIRTUAL      false
#define IS_STATIC_NON_VIRTUAL  false
#define IS_STATIC_STATIC       true

#define DOTDOTDOT_NAME     
#define V_NAME             V
#define A_NAME             A
#define ARG_DECL_DOTDOTDOT ...
#define ARG_DECL_V         va_list args
#define ARG_DECL_A         const jvalue * args
#define VA_START_DOTDOTDOT va_list args; va_start(args, methodID)
#define VA_START_V         
#define VA_START_A
#define VA_END_DOTDOTDOT   va_end(args);
#define VA_END_V           
#define VA_END_A
#define new_entry_activation_DOTDOTDOT new_entry_activation_V

#define ENTRY_RETURN_BOOLEAN invoke_entry_word_return
#define ENTRY_RETURN_BYTE    invoke_entry_word_return
#define ENTRY_RETURN_CHAR    invoke_entry_word_return
#define ENTRY_RETURN_SHORT   invoke_entry_word_return
#define ENTRY_RETURN_INT     invoke_entry_word_return
#define ENTRY_RETURN_LONG    invoke_entry_long_return
#define ENTRY_RETURN_FLOAT   invoke_entry_float_return
#define ENTRY_RETURN_DOUBLE  invoke_entry_double_return
#define ENTRY_RETURN_VOID    invoke_entry_void_return  
#define ENTRY_RETURN_OBJECT  invoke_entry_word_return  

#define RETURN_INVOKE_ENTRY_BOOLEAN return invoke_entry_word()
#define RETURN_INVOKE_ENTRY_BYTE    return invoke_entry_word()
#define RETURN_INVOKE_ENTRY_CHAR    return invoke_entry_word()
#define RETURN_INVOKE_ENTRY_SHORT   return invoke_entry_word()
#define RETURN_INVOKE_ENTRY_INT     return invoke_entry_word()
#define RETURN_INVOKE_ENTRY_LONG    return invoke_entry_long()
#define RETURN_INVOKE_ENTRY_FLOAT   return invoke_entry_float()
#define RETURN_INVOKE_ENTRY_DOUBLE  return invoke_entry_double()
#define RETURN_INVOKE_ENTRY_VOID    invoke_entry_void()
#define RETURN_INVOKE_ENTRY_OBJECT                     \
  do {                                                 \
    UsingFastOops fast_oops;                           \
    Oop::Fast oop = (OopDesc*)invoke_entry_word(); \
    return new_local_ref_or_null_for_oop(env, &oop);   \
  } while (0)

#define BOOLEAN_ZERO 0
#define BYTE_ZERO    0   
#define CHAR_ZERO    0   
#define SHORT_ZERO   0  
#define INT_ZERO     0    
#define LONG_ZERO    0   
#define FLOAT_ZERO   0  
#define DOUBLE_ZERO  0 
#define VOID_ZERO     
#define OBJECT_ZERO  0 


#define FOR_ALL_CALL_TYPES(template) \
  FOR_ALL_ARG_TYPES(template, VIRTUAL)     \
  FOR_ALL_ARG_TYPES(template, NON_VIRTUAL) \
  FOR_ALL_ARG_TYPES(template, STATIC)      \

#define FOR_ALL_ARG_TYPES(template, arg)         \
  FOR_ALL_RETURN_TYPES(template, DOTDOTDOT, arg) \
  FOR_ALL_RETURN_TYPES(template, V, arg)         \
  FOR_ALL_RETURN_TYPES(template, A, arg)

#define FOR_ALL_RETURN_TYPES(template, arg1, arg2) \
  template(jboolean, Boolean, BOOLEAN, arg1, arg2) \
  template(jbyte,    Byte,    BYTE,    arg1, arg2) \
  template(jchar,    Char,    CHAR,    arg1, arg2) \
  template(jshort,   Short,   SHORT,   arg1, arg2) \
  template(jint,     Int,     INT,     arg1, arg2) \
  template(jlong,    Long,    LONG,    arg1, arg2) \
  template(jfloat,   Float,   FLOAT,   arg1, arg2) \
  template(jdouble,  Double,  DOUBLE,  arg1, arg2) \
  template(void,     Void,    VOID,    arg1, arg2) \
  template(jobject,  Object,  OBJECT,  arg1, arg2)

#define CONCAT5(a,b,c,d,e) a ## b ## c ## d ## e

#define CALL_METHOD_NAME(RETURN_TYPE, ARG_TYPE_NAME, CALL_TYPE_NAME) \
  CONCAT5(_JNI_Call, CALL_TYPE_NAME, RETURN_TYPE, Method, ARG_TYPE_NAME)

#define DEFINE_CALL_METHOD(jtype, Type, TYPE, ARG_TYPE, CALL_TYPE)          \
static jtype JNICALL                                                        \
CALL_METHOD_NAME(Type, ARG_TYPE ## _NAME, CALL_TYPE ## _NAME)               \
                                            (JNIEnv *env,                   \
                                             RECEIVER_DECL_ ## CALL_TYPE    \
                                             CLASS_DECL_ ## CALL_TYPE       \
                                             jmethodID methodID,            \
                                             ARG_DECL_ ## ARG_TYPE) {       \
  DUMMY_RECEIVER_ ## CALL_TYPE                                              \
  DUMMY_CLASS_ ## CALL_TYPE                                                 \
                                                                            \
  VA_START_ ## ARG_TYPE;                                                    \
                                                                            \
  EntryActivation::Raw entry =                                              \
    new_entry_activation_ ## ARG_TYPE(env, obj, cls, methodID, args,        \
                                      IS_VIRTUAL_ ## CALL_TYPE,             \
                                      IS_STATIC_ ## CALL_TYPE);             \
                                                                            \
  VA_END_ ## ARG_TYPE;                                                      \
                                                                            \
  if (entry.is_null()) {                                                    \
    return TYPE ## _ZERO;                                                   \
  }                                                                         \
                                                                            \
  entry().set_return_point((address)ENTRY_RETURN_ ## TYPE);                 \
                                                                            \
  GUARANTEE(!Thread::current()->has_pending_entries(),                      \
            "Must be no pending entries at this point");                    \
                                                                            \
  Thread::current()->append_pending_entry(&entry);                          \
                                                                            \
  RETURN_INVOKE_ENTRY_ ## TYPE;                                             \
}

FOR_ALL_CALL_TYPES(DEFINE_CALL_METHOD)

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

  // NOTE: field could be renamed by Romizer.
  // Make sure you have used the DontRenameNonPublicFields option in the
  // -romconfig file for this class.
  if (!field.is_valid()) {
    Throw::no_such_field_error(JVM_SINGLE_ARG_THROW_0);
  }

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

static void
unchecked_get_string_utf_region(String * string, jsize start, jsize len,
                                char * buf) {
  TypeArray::Raw value = string->value();
  const jint offset = string->offset();
  const jint utf8len = len * 3;

  LiteralStream stream(buf, 0, utf8len);

  int i, index;
  for (index = 0, i = start; i < start + len; i++) {
    jchar ch = value().char_at(i + offset);
    index = stream.utf8_write(index, ch);
  }
  GUARANTEE(index <= utf8len, "UTF8 encoder failed");
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

  unchecked_get_string_utf_region(&string, 0, length, buf);

  return buf;
}

static void JNICALL
_JNI_ReleaseStringUTFChars(JNIEnv *env, jstring str, const char *chars) {
  if (chars != NULL) {
    jvm_free((void*)chars);
  }
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

  jobject null_ref = NULL;
  if (val == NULL) {
    null_ref = new_local_ref(env);
    if (null_ref == NULL) {
      return;
    }
    val = null_ref;
  } else {
    // array type checking
    Oop::Raw obj = *decode_handle(val);
    if (obj.not_null()) {
      Oop::Raw arr = *decode_handle(array);
      if (arr.not_null() && arr.is_obj_array()) {
        ObjArrayClass::Raw arr_class = arr.blueprint();
        JavaClass::Raw element_class = arr_class().element_class();
        JavaClass::Raw obj_class = obj.blueprint();
        if (!obj_class().is_subtype_of(&element_class)) {
          SETUP_ERROR_CHECKER_ARG;
          Throw::array_store_exception(arraycopy_incompatible_types JVM_THROW);
        }
      }
    }
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

static void JNICALL 
_JNI_GetStringRegion(JNIEnv *env, jstring str, 
                     jsize start, jsize len, jchar *buf) {
  const jint str_len = _JNI_GetStringLength(env, str);
  if (str_len < 0) {
    return;
  }

  if (start < 0 || len < 0 ||
      (juint)len + (juint)start > (juint)str_len) {
    SETUP_ERROR_CHECKER_ARG;
    Throw::string_index_out_of_bounds_exception(empty_message JVM_THROW);
  }

  KNI_GetStringRegion(str, start, len, buf);
}

static void JNICALL 
_JNI_GetStringUTFRegion(JNIEnv *env, jstring str, 
                        jsize start, jsize len, char *buf) {
  const jint str_len = _JNI_GetStringLength(env, str);
  if (str_len < 0) {
    return;
  }

  String::Raw string = *decode_handle(str);
  if (string.is_null()) {
    return;
  }

  if (start < 0 || len < 0 ||
      (juint)len + (juint)start > (juint)str_len) {
    SETUP_ERROR_CHECKER_ARG;
    Throw::string_index_out_of_bounds_exception(empty_message JVM_THROW);
  }

  unchecked_get_string_utf_region(&string, start, len, buf);
}

static void * JNICALL 
_JNI_GetPrimitiveArrayCritical(JNIEnv *env, jarray array, jboolean *isCopy) {
  if (array == NULL) {
    return NULL;
  }

  TypeArray::Raw type_array = *decode_handle(array);
  if (type_array.is_null()) {
    return NULL;
  }

  TypeArrayClass::Raw array_class = type_array().blueprint();
  const jint scale = array_class().scale();

  void * elements = NULL;
  switch (scale) {
  case 1: 
    elements = _JNI_GetByteArrayElements(env, array, isCopy); 
    break;
  case 2: 
    elements = _JNI_GetShortArrayElements(env, array, isCopy);
    break;
  case 4: 
    elements = _JNI_GetIntArrayElements(env, array, isCopy);
    break;
  case 8: 
    elements = _JNI_GetLongArrayElements(env, array, isCopy);
    break;
  default: 
    SHOULD_NOT_REACH_HERE();
  }

  return elements;
}

static void JNICALL 
_JNI_ReleasePrimitiveArrayCritical(JNIEnv *env, jarray array, 
                                   void *carray, jint mode) {
  if (array == NULL) {
    return;
  }

  TypeArray::Raw type_array = *decode_handle(array);
  if (type_array.is_null()) {
    return;
  }

  TypeArrayClass::Raw array_class = type_array().blueprint();
  const jint scale = array_class().scale();

  switch (scale) {
  case 1: 
    _JNI_ReleaseByteArrayElements(env, array, (jbyte*)carray, mode);
    break;
  case 2: 
    _JNI_ReleaseShortArrayElements(env, array, (jshort*)carray, mode);
    break;
  case 4: 
    _JNI_ReleaseIntArrayElements(env, array, (jint*)carray, mode);
    break;
  case 8: 
    _JNI_ReleaseLongArrayElements(env, array, (jlong*)carray, mode);
    break;
  default:
    SHOULD_NOT_REACH_HERE();
  }
}

static const jchar * JNICALL 
_JNI_GetStringCritical(JNIEnv *env, jstring string, jboolean *isCopy) {
  return _JNI_GetStringChars(env, string, isCopy);
}

static void JNICALL
_JNI_ReleaseStringCritical(JNIEnv *env, jstring string, const jchar *cstring) {
  _JNI_ReleaseStringChars(env, string, cstring);
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
  if (ref_index < 0) {
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
  }

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
    _JNI_ThrowNew, // ThrowNew
    _JNI_ExceptionOccurred, // ExceptionOccurred
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
    _JNI_NewObject, // NewObject
    _JNI_NewObjectV, // NewObjectV
    _JNI_NewObjectA, // NewObjectA

    _JNI_GetObjectClass, // GetObjectClass
    _JNI_IsInstanceOf, // IsInstanceOf

    _JNI_GetMethodID, // GetMethodID

    _JNI_CallObjectMethod, // CallObjectMethod
    _JNI_CallObjectMethodV, // CallObjectMethodV
    _JNI_CallObjectMethodA, // CallObjectMethodA
    _JNI_CallBooleanMethod, // CallBooleanMethod
    _JNI_CallBooleanMethodV, // CallBooleanMethodV
    _JNI_CallBooleanMethodA, // CallBooleanMethodA
    _JNI_CallByteMethod, // CallByteMethod
    _JNI_CallByteMethodV, // CallByteMethodV
    _JNI_CallByteMethodA, // CallByteMethodA
    _JNI_CallCharMethod, // CallCharMethod
    _JNI_CallCharMethodV, // CallCharMethodV
    _JNI_CallCharMethodA, // CallCharMethodA
    _JNI_CallShortMethod, // CallShortMethod
    _JNI_CallShortMethodV, // CallShortMethodV
    _JNI_CallShortMethodA, // CallShortMethodA
    _JNI_CallIntMethod, // CallIntMethod
    _JNI_CallIntMethodV, // CallIntMethodV
    _JNI_CallIntMethodA, // CallIntMethodA
    _JNI_CallLongMethod, // CallLongMethod
    _JNI_CallLongMethodV, // CallLongMethodV
    _JNI_CallLongMethodA, // CallLongMethodA
    _JNI_CallFloatMethod, // CallFloatMethod
    _JNI_CallFloatMethodV, // CallFloatMethodV
    _JNI_CallFloatMethodA, // CallFloatMethodA
    _JNI_CallDoubleMethod, // CallDoubleMethod
    _JNI_CallDoubleMethodV, // CallDoubleMethodV
    _JNI_CallDoubleMethodA, // CallDoubleMethodA
    _JNI_CallVoidMethod, // CallVoidMethod
    _JNI_CallVoidMethodV, // CallVoidMethodV
    _JNI_CallVoidMethodA, // CallVoidMethodA

    _JNI_CallNonvirtualObjectMethod, // CallNonvirtualObjectMethod
    _JNI_CallNonvirtualObjectMethodV, // CallNonvirtualObjectMethodV
    _JNI_CallNonvirtualObjectMethodA, // CallNonvirtualObjectMethodA
    _JNI_CallNonvirtualBooleanMethod, // CallNonvirtualBooleanMethod
    _JNI_CallNonvirtualBooleanMethodV, // CallNonvirtualBooleanMethodV
    _JNI_CallNonvirtualBooleanMethodA, // CallNonvirtualBooleanMethodA
    _JNI_CallNonvirtualByteMethod, // CallNonvirtualByteMethod
    _JNI_CallNonvirtualByteMethodV, // CallNonvirtualByteMethodV
    _JNI_CallNonvirtualByteMethodA, // CallNonvirtualByteMethodA
    _JNI_CallNonvirtualCharMethod, // CallNonvirtualCharMethod
    _JNI_CallNonvirtualCharMethodV, // CallNonvirtualCharMethodV
    _JNI_CallNonvirtualCharMethodA, // CallNonvirtualCharMethodA
    _JNI_CallNonvirtualShortMethod, // CallNonvirtualShortMethod
    _JNI_CallNonvirtualShortMethodV, // CallNonvirtualShortMethodV
    _JNI_CallNonvirtualShortMethodA, // CallNonvirtualShortMethodA
    _JNI_CallNonvirtualIntMethod, // CallNonvirtualIntMethod
    _JNI_CallNonvirtualIntMethodV, // CallNonvirtualIntMethodV
    _JNI_CallNonvirtualIntMethodA, // CallNonvirtualIntMethodA
    _JNI_CallNonvirtualLongMethod, // CallNonvirtualLongMethod
    _JNI_CallNonvirtualLongMethodV, // CallNonvirtualLongMethodV
    _JNI_CallNonvirtualLongMethodA, // CallNonvirtualLongMethodA
    _JNI_CallNonvirtualFloatMethod, // CallNonvirtualFloatMethod
    _JNI_CallNonvirtualFloatMethodV, // CallNonvirtualFloatMethodV
    _JNI_CallNonvirtualFloatMethodA, // CallNonvirtualFloatMethodA
    _JNI_CallNonvirtualDoubleMethod, // CallNonvirtualDoubleMethod
    _JNI_CallNonvirtualDoubleMethodV, // CallNonvirtualDoubleMethodV
    _JNI_CallNonvirtualDoubleMethodA, // CallNonvirtualDoubleMethodA
    _JNI_CallNonvirtualVoidMethod, // CallNonvirtualVoidMethod
    _JNI_CallNonvirtualVoidMethodV, // CallNonvirtualVoidMethodV
    _JNI_CallNonvirtualVoidMethodA, // CallNonvirtualVoidMethodA

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

    _JNI_GetStaticMethodID, // GetStaticMethodID

    _JNI_CallStaticObjectMethod, // CallStaticObjectMethod
    _JNI_CallStaticObjectMethodV, // CallStaticObjectMethodV
    _JNI_CallStaticObjectMethodA, // CallStaticObjectMethodA
    _JNI_CallStaticBooleanMethod, // CallStaticBooleanMethod
    _JNI_CallStaticBooleanMethodV, // CallStaticBooleanMethodV
    _JNI_CallStaticBooleanMethodA, // CallStaticBooleanMethodA
    _JNI_CallStaticByteMethod, // CallStaticByteMethod
    _JNI_CallStaticByteMethodV, // CallStaticByteMethodV
    _JNI_CallStaticByteMethodA, // CallStaticByteMethodA
    _JNI_CallStaticCharMethod, // CallStaticCharMethod
    _JNI_CallStaticCharMethodV, // CallStaticCharMethodV
    _JNI_CallStaticCharMethodA, // CallStaticCharMethodA
    _JNI_CallStaticShortMethod, // CallStaticShortMethod
    _JNI_CallStaticShortMethodV, // CallStaticShortMethodV
    _JNI_CallStaticShortMethodA, // CallStaticShortMethodA
    _JNI_CallStaticIntMethod, // CallStaticIntMethod
    _JNI_CallStaticIntMethodV, // CallStaticIntMethodV
    _JNI_CallStaticIntMethodA, // CallStaticIntMethodA
    _JNI_CallStaticLongMethod, // CallStaticLongMethod
    _JNI_CallStaticLongMethodV, // CallStaticLongMethodV
    _JNI_CallStaticLongMethodA, // CallStaticLongMethodA
    _JNI_CallStaticFloatMethod, // CallStaticFloatMethod
    _JNI_CallStaticFloatMethodV, // CallStaticFloatMethodV
    _JNI_CallStaticFloatMethodA, // CallStaticFloatMethodA
    _JNI_CallStaticDoubleMethod, // CallStaticDoubleMethod
    _JNI_CallStaticDoubleMethodV, // CallStaticDoubleMethodV
    _JNI_CallStaticDoubleMethodA, // CallStaticDoubleMethodA
    _JNI_CallStaticVoidMethod, // CallStaticVoidMethod
    _JNI_CallStaticVoidMethodV, // CallStaticVoidMethodV
    _JNI_CallStaticVoidMethodA, // CallStaticVoidMethodA

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

    _JNI_GetStringRegion, // GetStringRegion
    _JNI_GetStringUTFRegion, // GetStringUTFRegion

    _JNI_GetPrimitiveArrayCritical, // GetPrimitiveArrayCritical
    _JNI_ReleasePrimitiveArrayCritical, // ReleasePrimitiveArrayCritical

    _JNI_GetStringCritical, // GetStringCritical     
    _JNI_ReleaseStringCritical, // ReleaseStringCritical 

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
