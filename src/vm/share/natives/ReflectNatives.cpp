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

# include "incls/_precompiled.incl"
# include "incls/_ReflectNatives.cpp.incl"

extern "C" {

#if ENABLE_ISOLATES

/* Helper function to get the fields */
static inline jfieldID find_instance_field(InstanceClass* ic, String* name,
                             		   const bool is_static JVM_TRAPS) {
  UsingFastOops fast_oops;

  GUARANTEE(ic->not_null(), "Isolate is not ready");
  TypeArray::Fast fields = ic->fields();
  Symbol::Fast n = SymbolTable::symbol_for(name JVM_CHECK_0);
  // IMPL_NOTE: for now we don't check the signatures
  // we also cannot check the type of the field because we are using a
  // class_list that is not the current running one.
  //  Symbol::Fast s = TypeSymbol::parse("Ljava/lang/Object" JVM_CHECK_0);

  for (int index = 0; index < fields().length(); index += 5) {
    Field f(ic, index);
    if ((f.is_static() == is_static)) { // && (f.type() == bt)) {
      Symbol::Raw name = f.name();
      if (name().matches(&n)) {
        return (jfieldID)((jint)f.offset());
      }
    }
  }
  // Throw a no such field error if the field cannot be found
  Throw::no_such_field_error(JVM_SINGLE_ARG_CHECK_0);
  return NULL;
}

ReturnOop find_instance_class_from_task(Task* task, String* class_name
                                        JVM_TRAPS) {
  UsingFastOops fast_oops;

  ObjArray::Fast dictionary = Universe::system_dictionary();
  Symbol::Fast name = 
      SymbolTable::slashified_symbol_for(class_name  JVM_CHECK_0);
  LoaderContext ctx(&name, ErrorOnFailure);
  InstanceClass::Fast ic =
      SystemDictionary::find_class_in_dictionary(&dictionary, &ctx, true);
  if (ic.not_null()) {
    return ic.obj();
  }

  dictionary = task->dictionary();
  if (dictionary.not_null()) {
    ic = SystemDictionary::find_class_in_dictionary(&dictionary, &ctx, true);
    if (ic.not_null()) {
      return ic.obj();
    }
  }
  
  Throw::class_not_found(&ctx JVM_THROW_0);
  return NULL;
}

/* Native implementation for static fields */
ReturnOop Java_com_sun_cldchi_test_Reflect_getStaticField(JVM_SINGLE_ARG_TRAPS)
{
  UsingFastOops fast_oops;

  int task_id = KNI_GetParameterAsInt(1);
  String::Fast class_name = GET_PARAMETER_AS_OOP(2);
  String::Fast field_name = GET_PARAMETER_AS_OOP(3);

  if (task_id < Task::FIRST_TASK) {
    Throw::error(isolate_not_started JVM_CHECK_0);
    return NULL;
  }

  Task::Fast task = Task::get_task(task_id);
  InstanceClass::Fast ic =
      find_instance_class_from_task(&task, &class_name JVM_CHECK_0);
  jfieldID fieldID =
      find_instance_field(&ic, &field_name, /*T_OBJECT,*/ true JVM_CHECK_0);

  ObjArray::Fast mirror_list = task().mirror_list();
  OopDesc* java_class = mirror_list().obj_at(ic().class_id());
  return *java_class->obj_field_addr((int)fieldID);
}

jint Java_com_sun_cldchi_test_Reflect_getStaticIntField(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;

  int task_id = KNI_GetParameterAsInt(1);
  String::Fast class_name = GET_PARAMETER_AS_OOP(2);
  String::Fast field_name = GET_PARAMETER_AS_OOP(3);

  if (task_id < Task::FIRST_TASK) {
    Throw::error(isolate_not_started JVM_CHECK_0);
    return 0;
  }

  Task::Fast task = Task::get_task(task_id);
  InstanceClass::Fast ic =
      find_instance_class_from_task(&task, &class_name JVM_CHECK_0);
  jfieldID fieldID =
      find_instance_field(&ic, &field_name, /*T_OBJECT,*/ true JVM_CHECK_0);

  ObjArray::Fast mirror_list = task().mirror_list();
  OopDesc* java_class = mirror_list().obj_at(ic().class_id());
  return *java_class->int_field_addr((int)fieldID);
}

jint
Java_com_sun_cldchi_test_Reflect_getStaticBooleanField(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;

  int task_id = KNI_GetParameterAsInt(1);
  String::Fast class_name = GET_PARAMETER_AS_OOP(2);
  String::Fast field_name = GET_PARAMETER_AS_OOP(3);

  if (task_id < Task::FIRST_TASK) {
    Throw::error(isolate_not_started JVM_CHECK_0);
    return 0;
  }

  Task::Fast task = Task::get_task(task_id);
  InstanceClass::Fast ic =
      find_instance_class_from_task(&task, &class_name JVM_CHECK_0);
  jfieldID fieldID =
      find_instance_field(&ic, &field_name, /*T_OBJECT,*/ true JVM_CHECK_0);

  ObjArray::Fast mirror_list = task().mirror_list();
  OopDesc* java_class = mirror_list().obj_at(ic().class_id());
  return *java_class->bool_field_addr((int)fieldID);
}


/* Native implementation for instance fields */
ReturnOop
Java_com_sun_cldchi_test_Reflect_getInstanceField(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;

  Instance::Fast object = GET_PARAMETER_AS_OOP(1);
  String::Fast field = GET_PARAMETER_AS_OOP(2);
  InstanceClass::Fast ic = object().blueprint();

  jfieldID fieldID =
      find_instance_field(&ic, &field, /*T_OBJECT,*/ false JVM_CHECK_0);

  OopDesc* java_class = object();
  return *java_class->obj_field_addr((int)fieldID);
}

jint Java_com_sun_cldchi_test_Reflect_getIntField(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;

  Instance::Fast object = GET_PARAMETER_AS_OOP(1);
  String::Fast field = GET_PARAMETER_AS_OOP(2);
  InstanceClass::Fast ic = object().blueprint();

  jfieldID fieldID =
      find_instance_field(&ic, &field, /*T_INT,*/ false JVM_CHECK_0);

  OopDesc* java_class = object();
  return *java_class->int_field_addr((int)fieldID);
}

jint Java_com_sun_cldchi_test_Reflect_getBooleanField(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;

  Instance::Fast object = GET_PARAMETER_AS_OOP(1);
  String::Fast field = GET_PARAMETER_AS_OOP(2);
  InstanceClass::Fast ic = object().blueprint();

  jfieldID fieldID =
      find_instance_field(&ic, &field, /*T_BOOLEAN,*/ false JVM_CHECK_0);

  OopDesc* java_class = object();
  return *java_class->bool_field_addr((int)fieldID);
}

#endif // ENABLE_ISOLATES

} // extern "C"
