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

class ArrayClass: public JavaClass {
 public:
  HANDLE_DEFINITION(ArrayClass, JavaClass);

  bool compute_is_subtype_of(JavaClass* other_class);

  // ^ObjArrayClass
  ReturnOop compute_higher_dimension(JVM_SINGLE_ARG_TRAPS);

#ifndef PRODUCT
  void iterate(OopVisitor* visitor);
  static void iterate_oopmaps(oopmaps_doer do_map, void* param);
#endif

 public:
  // To avoid endless lists of friends the static offset computation
  // routines are all public.
  static int element_class_offset() {
    return FIELD_OFFSET(ArrayClassDesc, array._element_class);
  }
};
