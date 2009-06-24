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
#include "incls/_ROMProfile.cpp.incl"

#if ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
ReturnOop ROMProfile::create(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ROMProfile::Fast profile =
    Universe::new_profile(JVM_SINGLE_ARG_OZCHECK(profile));

  OopDesc* p = Universe::new_vector(JVM_SINGLE_ARG_ZCHECK_0(p));
  profile().set_hidden_classes(p);

  p = Universe::new_vector(JVM_SINGLE_ARG_ZCHECK_0(p));
  profile().set_hidden_packages(p);

  p = Universe::new_vector(JVM_SINGLE_ARG_ZCHECK_0(p));
  profile().set_restricted_packages(p);
  return profile().obj();
}

ReturnOop ROMProfile::create(const char name[] JVM_TRAPS) {
  UsingFastOops fast_oops;
  ROMProfile::Fast profile = create(JVM_SINGLE_ARG_OZCHECK(profile));

  OopDesc* p = SymbolTable::symbol_for(name JVM_ZCHECK_0(p));
  profile().set_profile_name(p);  

  ROMOptimizer::profiles_vector()->add_element(&profile JVM_CHECK_0);
  return profile().obj();
}

int ROMProfile::calc_bitmap_raw_size( void ) {
  const int class_count = ROMWriter::number_of_romized_java_classes();
  // Division with rounding up to the higher integer value.
  return (class_count + (BitsPerByte - 1)) / BitsPerByte;
}

#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
