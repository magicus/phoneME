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

// CopyrightVersion 1.2

#include "incls/_precompiled.incl"
#include "incls/_FieldType.cpp.incl"

HANDLE_CHECK(FieldType, is_valid_field_type())

BasicType FieldType::basic_type() {
  if (length() == 1) {
    return primitive_field_basic_type_at(0);
  } else {
    GUARANTEE(length() == 2, "sanity");
    return object_basic_type_at(0);
  }
}

int FieldType::allocation_byte_size(bool is_static_field) {
  BasicType type = basic_type();
  jint byte_size = byte_size_for(type);
  return is_static_field 
       ? align_size_up(byte_size, T_INT_byte_size)
       : byte_size;
}
