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

#if ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR

class ROMProfileDesc : public MixedOopDesc {
private:
  // Specified profile name.
  OopDesc* _profile_name;

  // The list of hidden classes for the specified profile.
  OopDesc* _hidden_classes;

  // The list of hidden packages.
  OopDesc* _hidden_packages;

  // The list of restricted packages.
  OopDesc* _restricted_packages;

  static size_t allocation_size( void ) { 
    return align_allocation_size( sizeof(ROMProfileDesc) );
  }

  static int pointer_count( void ) {
    return 4; // return the number of Handles aggrigated by the object.
  }

public:
  friend class ROMProfile;
  friend class Universe;
};

class ROMProfile : public MixedOop {
public:
  // Defining constructors, copy constructors, 
  // assingment operators.
  HANDLE_DEFINITION(ROMProfile, MixedOop);

  DEFINE_ACCESSOR_OBJ(ROMProfile, Symbol,    profile_name       )  
  DEFINE_ACCESSOR_OBJ(ROMProfile, ROMVector, hidden_classes     )
  DEFINE_ACCESSOR_OBJ(ROMProfile, ROMVector, hidden_packages    )
  DEFINE_ACCESSOR_OBJ(ROMProfile, ROMVector, restricted_packages)  

public:
  static ReturnOop create(JVM_SINGLE_ARG_TRAPS);
  static ReturnOop create(const char name[] JVM_TRAPS);
  static int calc_bitmap_raw_size();
};

#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
