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

/**
 * A FieldType is used to determine the type of a field.
 */
class FieldType: public TypeSymbol {
public:
  HANDLE_DEFINITION_CHECK(FieldType, TypeSymbol);
  ~FieldType() {}

  // Return basic type for field
  BasicType basic_type();

  ReturnOop object_type() {
    return object_type_at(0);
  }

  ReturnOop object_type_symbol() {
    JavaClass::Raw klass = object_type();
    return klass().name();
  }

  // Returns the allocation of a field in bytes
  int allocation_byte_size(bool is_static_field);

  static bool is_array(Symbol* signature) {
    return (signature->length() > 1) && (signature->byte_at(0) == '[');
  }
};
