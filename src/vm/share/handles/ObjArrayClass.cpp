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
# include "incls/_ObjArrayClass.cpp.incl"

HANDLE_CHECK(ObjArrayClass, is_obj_array_class())

bool ObjArrayClass::compute_is_subtype_of(JavaClass* other_class) {
  if (!other_class->is_obj_array_class()) {
    return ArrayClass::compute_is_subtype_of(other_class);
  }
  ObjArrayClass::Raw other         = other_class;
  JavaClass::Raw     element       = element_class();
  JavaClass::Raw     other_element = other().element_class();
  return element().is_subtype_of(&other_element);
}

ReturnOop ObjArrayClass::get_array_class(jint distance JVM_TRAPS) {
  if (distance == 1) {
    return obj();
  }
  UsingFastOops fast_oops;
  ObjArrayClass::Fast ac =
    compute_higher_dimension(JVM_SINGLE_ARG_ZCHECK_0(ac));
  return ac().get_array_class(distance - 1 JVM_NO_CHECK_AT_BOTTOM_0);
}

ReturnOop ObjArrayClass::multi_allocate(int rank, JavaFrame* frame JVM_TRAPS) { 
  UsingFastOops fast_oops;
  int length = frame->expression_at(rank - 1)->as_int();
  ObjArray::Fast result = Universe::allocate_array(this, length, oopSize        
                                                   JVM_ZCHECK_0(result));
  JavaClass::Fast lower;
  if (rank > 1) {
    // Allocate all the sub arrays
    lower = element_class();
    if (lower.is_obj_array_class()) {
      ObjArrayClass *oc = (ObjArrayClass*)&lower;
      if (length == 0) {
        /*
         * If there is a zero dimension value, subsequent dimensions should not
         * be allocated, but JVMS requires to throw NegativeArraySizeException 
         * if *any* of dimension values is negative.
         */
        for (int r = rank - 2; r >= 0; r--) {
          int sub_length = frame->expression_at(r)->as_int();
          if (sub_length < 0) {
            Throw::throw_exception(
              Symbols::java_lang_NegativeArraySizeException() JVM_THROW_0);
          }
        }
      }
      for (int index = 0; index < length; index++) {  
        JavaOop::Raw temp = oc->multi_allocate(rank-1, frame JVM_ZCHECK_0(temp));
        result().obj_at_put(index, &temp);
      }                    
    } else {
      GUARANTEE(rank == 2, "Sanity check");
      TypeArrayClass* ta = (TypeArrayClass*)&lower;
      jint sub_length = frame->expression_at(0)->as_int();
      if (sub_length < 0) {
        Throw::throw_exception(Symbols::java_lang_NegativeArraySizeException() 
                           JVM_THROW_0);
      }
      for (int index = 0; index < length; index++) {          
        JavaOop::Raw temp = Universe::new_type_array(ta, sub_length 
                                                     JVM_ZCHECK_0(temp));
        result().obj_at_put(index, &temp);
      }                    
    }
  }
  return result;
}

void ObjArrayClass::compute_name(JVM_SINGLE_ARG_TRAPS) {
  // All ObjArrayClasses must have a non-zero klass index, except maybe
  // one of the classes created during genesis
  GUARANTEE(Universe::before_main() || class_id() != 0, 
            "class must be registered");
  Symbol::Raw array_class_name = 
      TypeSymbol::obj_array_class_name(this JVM_NO_CHECK);
  if (array_class_name.not_null()) {
    set_name(&array_class_name);
  }
}

bool ObjArrayClass::check_access_by(InstanceClass* sender_class, 
                                    FailureMode fail_mode JVM_TRAPS) {
  UsingFastOops fast_oops;
  JavaClass::Fast ec = element_class();
  return ec().check_access_by(sender_class, fail_mode JVM_NO_CHECK_AT_BOTTOM_0);
}

#ifndef PRODUCT

void ObjArrayClass::print_value_on(Stream* st) {
  st->print("ObjArrayClass ");
  print_name_on(st);
}

void ObjArrayClass::iterate(OopVisitor* visitor) {
  ArrayClass::iterate(visitor);
}

#endif
