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
#include "incls/_ROMProfile.cpp.incl"

#if ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
void ROMProfile::initialize(char *name JVM_TRAPS) {  

  // [1] Allocating Desc.
  OopDesc* obj = Universe::new_profile(JVM_SINGLE_ARG_CHECK);
  set_obj(obj);

  // [2] Initializing profile name.
  Symbol::Raw symbol = SymbolTable::symbol_for(name JVM_CHECK);
  set_profile_name(&symbol);  

  // UsingFastOops fastOops;
  // [3] Profile hidden classes.
  ROMVector::Raw list = Universe::new_vector(JVM_SINGLE_ARG_CHECK);
  set_hidden_classes(&list);

  // [4] Hidden packages within the profile.
  list = Universe::new_vector(JVM_SINGLE_ARG_CHECK);
  set_hidden_packages(&list);

  // [5] Restricted packages within the profile.
  list = Universe::new_vector(JVM_SINGLE_ARG_CHECK);
  set_restricted_packages(&list);
}

int ROMProfile::calc_bitmap_raw_size() {
  const int class_count = ROMWriter::number_of_romized_java_classes();
  // Division with rounding up to the higher integer value.
  return (class_count - 1) / BitsPerByte + 1;
}

#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
