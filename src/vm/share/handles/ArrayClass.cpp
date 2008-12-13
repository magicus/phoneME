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
# include "incls/_ArrayClass.cpp.incl"

bool ArrayClass::compute_is_subtype_of(JavaClass* other_class) {
  if (Universe::object_class()->equals(other_class)) return true;
  // ADD THIS IF SERIALIZATION SHOULD BE SUPPORTED 
  // if (Universe::clonable_class()->equals(other_class))) return true;
  // if (Universe::serializable_class()->equals(other_class))) return true;
  return false;
}

ReturnOop ArrayClass::compute_higher_dimension(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ObjArrayClass::Fast result = array_class();
  if (result.is_null()) {
    result = Universe::new_obj_array_class(this JVM_CHECK_0);
    set_array_class(&result JVM_CHECK_0);
    // With fixes to ClassBySig in VMImpl.cpp we don't need to do this
    //    VMEvent::class_prepare_event(&result);
  }
#ifdef AZZERT
  JavaClass::Fast elementType = result().element_class();
  GUARANTEE(elementType.equals(this), "Bad element type");
  ArrayClass::Fast myArrayClass = this->array_class();
  GUARANTEE(myArrayClass.equals(&result), "Bad array class");
#endif
  return result;
}

#ifndef PRODUCT 

void ArrayClass::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  JavaClass::iterate(visitor);
  { 
    NamedField id("element_class", true);
    visitor->do_oop(&id, element_class_offset(), true);
  }

  for (int i = element_class_offset() + BytesPerWord; 
       i < JavaClassDesc::header_size(); i += BytesPerWord) {
    NamedField id("unused", true);
    visitor->do_int(&id, i, true);
  }
#endif
}

void ArrayClass::iterate_oopmaps(oopmaps_doer do_map, void* param) {
#if USE_OOP_VISITOR
  JavaClass::iterate_oopmaps(do_map, param);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, element_class);

#if ENABLE_ISOLATES
  // IMPL_NOTE: delete? JavaClassDesc::generate_mirror_table_oopmap(do_offset);
#endif
#endif
}

#endif
