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
#define ROMPROFILE_FIELDS_DO(template)  \
  template(Symbol,    profile_name       )  \
  template(ROMVector, hidden_classes     )  \
  template(ROMVector, hidden_packages    )  \
  template(ROMVector, restricted_packages)  \
  template(TypeArray, hidden_set         )

private:
#define ROMPROFILE_COUNT_FIELDS(type, name) name##_index,
  enum {
    ROMPROFILE_FIELDS_DO(ROMPROFILE_COUNT_FIELDS)
    _pointer_count
  };
#undef ROMPROFILE_COUNT_FIELDS

#define ROMPROFILE_DEFINE_FIELD(type, name) OopDesc* _##name;
  ROMPROFILE_FIELDS_DO(ROMPROFILE_DEFINE_FIELD)
#undef ROMPROFILE_DEFINE_FIELD

  static size_t allocation_size( void ) { 
    return align_allocation_size( sizeof(ROMProfileDesc) );
  }

  static int pointer_count( void ) {
    return _pointer_count; // return the number of Handles aggregated by the object.
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

#define ROMPROFILE_DEFINE_ACCESSOR(type, name)\
  DEFINE_ACCESSOR_OBJ(ROMProfile, type, name)
  ROMPROFILE_FIELDS_DO(ROMPROFILE_DEFINE_ACCESSOR)
#undef ROMPROFILE_DEFINE_ACCESSOR

public:
  static ReturnOop create(JVM_SINGLE_ARG_TRAPS);
  static ReturnOop create(const char name[] JVM_TRAPS);

  OopDesc* allocate_hidden_set(JVM_SINGLE_ARG_TRAPS);
  void fill_hidden_set(JVM_SINGLE_ARG_TRAPS);
#undef ROMPROFILE_FIELDS_DO
};

#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
