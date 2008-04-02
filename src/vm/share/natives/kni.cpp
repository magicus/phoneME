/*
 *   
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
 */

#include "incls/_precompiled.incl"
#include "incls/_kni.cpp.incl"

extern "C" int _in_kvm_native_method;

//
// Version information
//
KNIEXPORT jint KNI_GetVersion() {
  return KNI_VERSION;
}

//
// Class and interface operations
//
KNIEXPORT void KNI_FindClass(const char* name, jclass classHandle) {
  SETUP_ERROR_CHECKER_ARG;
  UsingFastOops fast_oops;

  kni_clear_handle(classHandle); // just in case class is not found, or
                                 // OutOfMemory

  // Find the class
  Symbol::Fast class_name = SymbolTable::symbol_for(name  _KNI_CHECK);
  ArrayClass::Fast ac;

  if (FieldType::is_array(&class_name)) {
    FieldType::Raw ft =
      TypeSymbol::parse_array_class_name(&class_name  _KNI_CHECK);
    ac = ft().object_type();
    JavaClass::Raw jc = ac().obj();
    while (jc().is_obj_array_class()) {
      ObjArrayClass::Raw oac = jc().obj();
      jc = oac().element_class();
    }

    if (jc().is_instance_class()) {
      InstanceClass::Raw ic = jc().obj();
      if (ic.not_null() && !ic().is_fake_class() && ic().is_initialized()) {
        kni_set_handle(classHandle, ac().java_mirror());
      }
    } else {
      GUARANTEE(jc().is_type_array_class(), "unknown kind class");
#if ENABLE_ISOLATES
      JavaClassObj::Raw result =
        ac().get_or_allocate_java_mirror(_KNI_SINGLE_ARG_CHECK);
      kni_set_handle(classHandle, result.obj());
#else
      kni_set_handle(classHandle, ac().java_mirror());
#endif
    }
  } else {
    LoaderContext top_ctx(&class_name, ErrorOnFailure);
    InstanceClass::Raw ic = SystemDictionary::find(&top_ctx,
                                                   /*lookup_only=*/ true,
                                                   /*check_only= */ true
                                                   _KNI_CHECK);
    if (ic.not_null() && ic().is_initialized()) {
      kni_set_handle(classHandle, ic().java_mirror());
    }
  }
}

KNIEXPORT void KNI_GetSuperClass(jclass classHandle, jclass superclassHandle) {
  kni_clear_handle(superclassHandle); // just in case there's no superclass

  JavaClassObj::Raw sub_mirror = kni_read_handle(classHandle);
  GUARANTEE(sub_mirror.not_null(), "null argument to KNI_GetSuperClass");
  JavaClass::Raw sub_class = sub_mirror().java_class();
  JavaClass::Raw super_class = sub_class().super();

  if (super_class.not_null()) {
    JavaClassObj::Raw m = super_class().java_mirror();
    kni_set_handle(superclassHandle, m.obj());
  }
}

KNIEXPORT jboolean KNI_IsAssignableFrom(jclass classHandle1,
                                        jclass classHandle2) {
  // Create handle for the first class
  JavaClassObj::Raw first_mirror = kni_read_handle(classHandle1);
  GUARANTEE(first_mirror.not_null(), "null argument to KNI_IsInstanceOf");
  JavaClass::Raw first_class = first_mirror().java_class();

  // Create handle for the second class
  JavaClassObj::Raw second_mirror = kni_read_handle(classHandle2);
  GUARANTEE(second_mirror.not_null(), "null argument to KNI_IsInstanceOf");
  JavaClass::Raw second_class = second_mirror().java_class();

  // Do subtype check
  return CAST_TO_JBOOLEAN(first_class().is_subtype_of(&second_class));
}

//
// Exceptions and errors
//
KNIEXPORT jint KNI_ThrowNew(const char* name, const char* message) {

  if(_jvm_in_quick_native_method) {
    _jvm_quick_native_exception = (char*)name;
    return KNI_OK;
  }

  UsingFastOops fast_oops;
  SETUP_ERROR_CHECKER_ARG;

  //make sure consecutive ThrowNew calls don't destroy one another
  Thread::clear_current_pending_exception();

  // (1) Lookup the exception class
  Symbol::Fast class_name = SymbolTable::symbol_for(name _KNI_OOME_CHECK_(KNI_ERR));

  if (class_name.equals(Symbols::java_lang_OutOfMemoryError())) {
    // Avoid allocating the exception object when we're running out of memory.
    Throw::out_of_memory_error(JVM_SINGLE_ARG_NO_CHECK);
    return KNI_OK;
  }

  //The following call is necessary for adherence to the KNI spec.
  //  KNI_ERR must be returned for bogus classnames.
  SystemDictionary::resolve(&class_name, 
                            ExceptionOnFailure _KNI_OOME_CHECK_(KNI_ERR));

  String::Fast str;
  if(message != NULL) {
    str = Universe::new_string(message, jvm_strlen(message) _KNI_OOME_CHECK_(KNI_ERR));
  } 

  {
    Throwable::Raw exception 
      = Throw::new_exception(&class_name, &str JVM_NO_CHECK);
    if (exception.not_null()) {
      Thread::set_current_pending_exception(&exception);
    } else {
      // OOME should be thrown by previous call automatically
    }
  }

  return KNI_OK;
}

KNIEXPORT void KNI_FatalError(const char* message) {
  NOT_PRODUCT(tty->print("JVM_FATAL ERROR in native method: "));
  tty->print_cr("%s", message);
  JVM_FATAL(native_method_error);
}

//
// Object operations
//
KNIEXPORT void KNI_GetObjectClass(jobject objectHandle, jclass classHandle) {
  OopDesc* object = kni_read_handle(objectHandle);
  SETUP_ERROR_CHECKER_ARG;

  if (object == 0) {
    kni_clear_handle(classHandle);
  } else {
    UsingFastOops fast_oops;
    JavaClass::Fast blueprint = (OopDesc*)object->blueprint();
    JavaClassObj::Fast m = blueprint().get_or_allocate_java_mirror(JVM_SINGLE_ARG_NO_CHECK);
    kni_set_handle(classHandle, m.obj());
  }
}

KNIEXPORT jboolean KNI_IsInstanceOf(jobject objectHandle, jclass classHandle) {
  // Create handle for objectHandle
  Oop::Raw object = kni_read_handle(objectHandle);
  if (object.is_null()) {
    return KNI_TRUE;
  }
  JavaClass::Raw object_class = object.blueprint();

  // Create handle for classHandle
  JavaClassObj::Raw mirror = kni_read_handle(classHandle);
  GUARANTEE(mirror.not_null(), "null argument to KNI_IsInstanceOf");
  JavaClass::Raw other_class = mirror().java_class();

  // Do subtype check
  return CAST_TO_JBOOLEAN(object_class().is_subtype_of(&other_class));
}

KNIEXPORT jboolean KNI_IsSameObject(jobject obj1, jobject obj2) {
  OopDesc* object1 = kni_read_handle(obj1);
  OopDesc* object2 = kni_read_handle(obj2);
  // Do identity check
  return CAST_TO_JBOOLEAN(object1 == object2);
}

//
// Helper function for field access
//
jfieldID _KNI_field_lookup_helper(jclass classHandle, const char* name,
                                  const char* signature, bool is_static) {
  UsingFastOops fast_oops;
  JavaClassObj::Fast mirror = kni_read_handle(classHandle);
  InstanceClass::Fast ic = mirror().java_class();
  InstanceClass::Fast holder(ic.obj());

  GUARANTEE(ic.is_instance_class(), "sanity check");

  SETUP_ERROR_CHECKER_ARG;
  Symbol::Fast n = SymbolTable::check_symbol_for(name _KNI_CHECK_0);
  Symbol::Fast s = TypeSymbol::parse((char*)signature _KNI_CHECK_0);

  Field field(&holder, &n, &s);

#ifndef PRODUCT
  // If you're hit by this GUARANTEE, make sure you have use
  // the DontRenameNonPublicFields option in the -romconfig file for
  // this class.
  //GUARANTEE(!field.is_valid(), "Field was renamed by Romizer.");

  if (!field.is_valid()) {
    tty->print_cr("WARNING: invalid KNI field access: %s", name);
  }
#endif

  // No exception check (OutOfMemoryError) done, field lookup will
  // fail and NULL be returned below anyway.
  if (!field.is_valid() || (field.is_static() != is_static) ||
      // We do not support field IDs in super classes:
      (field.is_static() && !holder().equals(&ic))) {
    return (jfieldID)0;
  } else {
    return (jfieldID)((jint)field.offset());
  }
}

//
// Instance field access
//
KNIEXPORT jfieldID KNI_GetFieldID(jclass classHandle, const char* name,
                                  const char* signature) {
  return _KNI_field_lookup_helper(classHandle, name, signature, false);
}

KNIEXPORT void
KNI_GetObjectField(jobject objectHandle, jfieldID fieldID, jobject toHandle) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_GetObjectField");
  kni_set_handle(toHandle, *object->obj_field_addr((int)fieldID));
}

KNIEXPORT void
KNI_SetObjectField(jobject objectHandle, jfieldID fieldID, jobject fromHandle)
{
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_SetObjectField");
  oop_write_barrier(object->obj_field_addr((int)fieldID),
                    kni_read_handle(fromHandle));
}

KNIEXPORT jint KNI_GetIntField(jobject objectHandle, jfieldID fieldID) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_GetIntField");
  return *object->int_field_addr((int)fieldID);
}

//
// String operations
//
KNIEXPORT jsize KNI_GetStringLength(jstring stringHandle) {
  String::Raw string = kni_read_handle(stringHandle);
  return (jsize)string.is_null() ? -1 : string().count();
}

KNIEXPORT void KNI_GetStringRegion(jstring stringHandle, jsize offset, jsize n,
                                   jchar* jcharbuf) {
  String::Raw string = kni_read_handle(stringHandle);
  GUARANTEE(string.not_null(), "null argument to KNI_GetStringRegion");
  jsize offset0 = string().offset();
  TypeArray::Raw ta = string().value();
  for (int i = 0; i < n; i++) {
    *jcharbuf++ = ta().char_at(offset0 + i + offset);
  }
}

KNIEXPORT void KNI_NewString(const jchar* src, jsize n, jstring stringHandle) {
  SETUP_ERROR_CHECKER_ARG;
  UsingFastOops fast_oops;
  kni_clear_handle(stringHandle); // just in case of OutOfMemory

  // Allocate Unicode string
  TypeArray::Fast ta = Universe::new_char_array(n _KNI_CHECK);
  for (int i = 0; i < n; ++i) {
    ta().char_at_put(i, src[i]);
  }
  OopDesc *str = Universe::new_string(&ta, 0, n _KNI_CHECK);
  kni_set_handle(stringHandle, str);
}

KNIEXPORT void KNI_NewStringUTF(const char* utf8chars, jstring stringHandle) {
  SETUP_ERROR_CHECKER_ARG;
  kni_clear_handle(stringHandle); // just in case of OutOfMemory
  OopDesc *str = Universe::new_string(utf8chars, jvm_strlen(utf8chars)
                                      _KNI_CHECK)
  kni_set_handle(stringHandle, str);
}

//
// Array operations
//
KNIEXPORT jsize KNI_GetArrayLength(jarray arrayHandle) {
  Array::Raw a = kni_read_handle(arrayHandle);
  return (jsize)a.is_null() ? -1 : a().length();
}

KNIEXPORT jboolean
KNI_GetBooleanArrayElement(jbooleanArray arrayHandle, jsize index) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_GetBooleanArrayElement");
  return type_array().bool_at(index);
}

KNIEXPORT jbyte
KNI_GetByteArrayElement(jbyteArray arrayHandle, jsize index) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_GetByteArrayElement");
  return type_array().byte_at(index);
}

KNIEXPORT jchar
KNI_GetCharArrayElement(jcharArray arrayHandle, jsize index) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_GetCharArrayElement");
  return type_array().char_at(index);
}

KNIEXPORT jshort
KNI_GetShortArrayElement(jshortArray arrayHandle, jsize index) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_GetShortArrayElement");
  return type_array().short_at(index);
}

KNIEXPORT jint
KNI_GetIntArrayElement(jintArray arrayHandle, jsize index) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_GetIntArrayElement");
  return type_array().int_at(index);
}

KNIEXPORT jlong
KNI_GetLongArrayElement(jlongArray arrayHandle, jsize index) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_GetLongArrayElement");
  return type_array().long_at(index);
}

#if ENABLE_FLOAT

KNIEXPORT jfloat
KNI_GetFloatArrayElement(jfloatArray arrayHandle, jsize index) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(),"null arg to KNI_GetFloatArrayElement");
  return type_array().float_at(index);
}

KNIEXPORT jdouble
KNI_GetDoubleArrayElement(jdoubleArray arrayHandle, jsize index) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_GetDoubleArrayElement");
  return type_array().double_at(index);
}

#endif

KNIEXPORT void KNI_SetBooleanArrayElement(jbooleanArray arrayHandle,
                                          jsize index, jboolean value) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_SetBoobleanArrayElement");
  type_array().bool_at_put(index, value);
}

KNIEXPORT void
KNI_SetByteArrayElement(jbyteArray arrayHandle, jsize index, jbyte value) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_SetByteArrayElement");
  type_array().byte_at_put(index, value);
}

KNIEXPORT void
KNI_SetCharArrayElement(jcharArray arrayHandle, jsize index, jchar value) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_SetCharArrayElement");
  type_array().char_at_put(index, value);
}

KNIEXPORT void
KNI_SetShortArrayElement(jshortArray arrayHandle, jsize index, jshort value) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_SetShortArrayElement");
  type_array().short_at_put(index, value);
}

KNIEXPORT void
KNI_SetIntArrayElement(jintArray arrayHandle, jsize index, jint value) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_SetIntArrayElement");
  type_array().int_at_put(index, value);
}

KNIEXPORT void
KNI_SetLongArrayElement(jlongArray arrayHandle, jsize index, jlong value) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_SetLongArrayElement");
  type_array().long_at_put(index, value);
}

#if ENABLE_FLOAT

KNIEXPORT void
KNI_SetFloatArrayElement(jfloatArray arrayHandle, jsize index, jfloat value) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_SetFloatArrayElement");
  type_array().float_at_put(index, value);
}

KNIEXPORT void KNI_SetDoubleArrayElement(jdoubleArray arrayHandle,
                                         jsize index, jdouble value) {
  TypeArray::Raw type_array = kni_read_handle(arrayHandle);
  GUARANTEE(type_array.not_null(), "null arg to KNI_SetDoubleArrayElement");
  type_array().double_at_put(index, value);
}

#endif

KNIEXPORT void KNI_GetObjectArrayElement(jobjectArray arrayHandle,
                                         jsize index, jobject toHandle) {
  ObjArray::Raw obj_array = kni_read_handle(arrayHandle);
  GUARANTEE(obj_array.not_null(), "null arg to KNI_GetObjectArrayElement");
  kni_set_handle(toHandle, obj_array().obj_at(index));
}

KNIEXPORT void KNI_SetObjectArrayElement(jobjectArray arrayHandle,
                                         jsize index, jobject fromHandle) {
  ObjArray::Raw obj_array = kni_read_handle(arrayHandle);
  GUARANTEE(obj_array.not_null(), "null arg to KNI_SetObjectArrayElement");
  obj_array().obj_at_put(index, kni_read_handle(fromHandle));
}

static inline address _KNI_GetRawArrayRegion_start_address(jarray src,
                                                           jsize offset) {
  TypeArray::Raw type_array = kni_read_handle(src);
  GUARANTEE(type_array.not_null(), "null arg to KNI_[Get|Set]RawArrayRegion");
  address ta_start =
      (address)&((char*)type_array.obj())[type_array().base_offset() + offset];
  return (ta_start);
}

KNIEXPORT void KNI_GetRawArrayRegion(jarray arrayHandle, jsize offset,
                                     jsize n, jbyte* dstBuffer) {
  address ta_start = _KNI_GetRawArrayRegion_start_address(arrayHandle, offset);
  (void)jvm_memmove(dstBuffer, ta_start, n);
}

KNIEXPORT void KNI_SetRawArrayRegion(jarray arrayHandle, jsize offset,
                                     jsize n, const jbyte* srcBuffer) {
  address ta_start = _KNI_GetRawArrayRegion_start_address(arrayHandle, offset);
  (void)jvm_memmove(ta_start, srcBuffer, n);
}

//
// Parameter passing
//

static inline address parameter_address(const int index) {
  return _kni_parameter_base +
     index * JavaStackDirection * BytesPerStackElement;
}

KNIEXPORT jboolean KNI_GetParameterAsBoolean(jint index) {
  GUARANTEE(!_in_kvm_native_method, "sanity");
  return (jboolean)(*(jint*)parameter_address(index));
}

KNIEXPORT jbyte KNI_GetParameterAsByte(jint index) {
  GUARANTEE(!_in_kvm_native_method, "sanity");
  return (jbyte)(*(jint*)parameter_address(index));
}

KNIEXPORT jchar KNI_GetParameterAsChar(jint index) {
  GUARANTEE(!_in_kvm_native_method, "sanity");
  return (jchar)(*(jint*)parameter_address(index));
}

KNIEXPORT jshort KNI_GetParameterAsShort(jint index) {
  GUARANTEE(!_in_kvm_native_method, "sanity");
  return (jshort)(*(jint*)parameter_address(index));
}

KNIEXPORT jint KNI_GetParameterAsInt(jint index) {
  GUARANTEE(!_in_kvm_native_method, "sanity");
  return *(jint*)parameter_address(index);
}

KNIEXPORT jlong KNI_GetParameterAsLong(jint index) {
  GUARANTEE(!_in_kvm_native_method, "sanity");
  jint AA = *(jint*)parameter_address(index);
  jint BB = *(jint*)parameter_address(index+1);

  if (JavaStackDirection < 0) {
    GUARANTEE(parameter_address(index+1) < parameter_address(index), "Sanity");
    return jlong_from_low_high(BB, AA);
  } else {
    GUARANTEE(parameter_address(index+1) > parameter_address(index), "Sanity");
    return jlong_from_low_high(AA, BB);
  }
}

#if ENABLE_FLOAT

KNIEXPORT jfloat KNI_GetParameterAsFloat(jint index) {
  GUARANTEE(!_in_kvm_native_method, "sanity");
  return *(jfloat*)parameter_address(index);
}

KNIEXPORT jdouble KNI_GetParameterAsDouble(jint index) {
  GUARANTEE(!_in_kvm_native_method, "sanity");
  jlong j = KNI_GetParameterAsLong(index);
  return *(jdouble*)&j;
}

#endif

KNIEXPORT void KNI_GetParameterAsObject(jint index, jobject obj) {
  GUARANTEE(!_in_kvm_native_method, "sanity");
  kni_set_handle(obj, *((OopDesc**)parameter_address(index)));
}

KNIEXPORT void KNI_GetThisPointer(jobject toHandle) {
  GUARANTEE(!_in_kvm_native_method, "sanity");
#ifdef AZZERT
  if (!_jvm_in_quick_native_method) {
    if (!ObjectHeap::is_finalizing()) {
      JavaFrame frame(Thread::current());
      Method::Raw method = frame.method();
      GUARANTEE(!method().is_static(), "static method has no 'this' pointer");
    }
  } else {
    // IMPL_NOTE: we don't have current method info ... this should be fixed to
    // make it possible to do the GUARANTEE as above.
  }
#endif
  KNI_GetParameterAsObject(0, toHandle);
}

KNIEXPORT void KNI_GetClassPointer(jclass toHandle) {
  JavaFrame frame(Thread::current());
  Method::Raw method = frame.method();
  jint holder_id = method().holder_id();
  OopDesc* java_class = Universe::class_list()->obj_at(holder_id);
  JavaClass::Raw jc = java_class;

#if ENABLE_ISOLATES 
  // At this point, it is possible that the class perhaps is still being
  // initialized, in which case jc().java_mirror() would return NULL.

  TaskMirror::Raw tm = jc().real_task_mirror();
  JavaClassObj::Raw m = tm().real_java_mirror();
#else
  JavaClassObj::Raw m = jc().java_mirror();
#endif

  kni_set_handle(toHandle, m.obj());
}

KNIEXPORT int _KNI_push_handles(int n, _KNI_HandleInfo* info, jobject* handles)
{
  info->prev = (_KNI_HandleInfo*)last_kni_handle_info;
  info->total_count = n;
  info->declared_count = 0;
  info->handles = handles;
  last_kni_handle_info = info;

  // Must clear it for GC to work properly
  jvm_memset(handles, 0, n * sizeof(jobject));

  return (0);
}

KNIEXPORT void _KNI_pop_handles(_KNI_HandleInfo* info) {
  GUARANTEE(last_kni_handle_info != NULL, "sanity");
  GUARANTEE(info->declared_count <= info->total_count, "KNI handles overflow");
  last_kni_handle_info = info->prev;
}
