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

class ObjArrayClass: public ArrayClass {
 public:
  HANDLE_DEFINITION_CHECK(ObjArrayClass, ArrayClass);

 public:
  // To avoid endless lists of friends the static offset computation routines are all public.
  static int element_class_offset() { return FIELD_OFFSET(ArrayClassDesc, array._element_class); }

 public:
  // The klass of the elements of this array type
  // ^JavaClass
  ReturnOop element_class() const          { return  obj_field(element_class_offset()); }
  void set_element_class(JavaClass* value) { obj_field_put(element_class_offset(), value); }

  // The one-dimensional type (instanceKlass or typeArrayKlass)
  // ^JavaClass

  bool compute_is_subtype_of(JavaClass* other_class);

  // ^ ObjArrayClass,
  ReturnOop get_array_class(jint distance JVM_TRAPS);

  // ^ObjArray, Allocation of multi array
  ReturnOop multi_allocate(int rank, JavaFrame* frame JVM_TRAPS);

  // Compute the name of the class and set name
  void compute_name(JVM_SINGLE_ARG_TRAPS);

  bool check_access_by(InstanceClass* sender_class, FailureMode fail_mode
                       JVM_TRAPS);

#ifndef PRODUCT
  void iterate(OopVisitor* visitor);
  void print_value_on(Stream* st);
#endif

};
