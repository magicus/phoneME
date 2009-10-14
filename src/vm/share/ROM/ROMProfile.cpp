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
    Universe::new_profile(JVM_SINGLE_ARG_OZCHECK_0(profile));

  OopDesc* p;
  #define ROMPROFILE_INIT_VECTORS(type, name)\
    p = Universe::new_vector(JVM_SINGLE_ARG_ZCHECK_0(p)); \
    profile().set_##name(p);
  ROMPROFILE_VECTORS_DO(ROMPROFILE_INIT_VECTORS)
  #undef ROMPROFILE_INIT_VECTORS

  return profile().obj();
}

ReturnOop ROMProfile::create(const char name[] JVM_TRAPS) {
  UsingFastOops fast_oops;
  ROMProfile::Fast profile = create(JVM_SINGLE_ARG_OZCHECK_0(profile));

  OopDesc* p = SymbolTable::symbol_for(name JVM_ZCHECK_0(p));
  profile().set_profile_name(p);  

  ROMOptimizer::profiles_vector()->add_element(&profile JVM_CHECK_0);
  return profile().obj();
}

ReturnOop ROMProfile::allocate_hidden_set(JVM_SINGLE_ARG_TRAPS) {
  OopDesc* p = ROMBitSet::create(JVM_SINGLE_ARG_NO_CHECK);
  set_hidden_set(p);
  return p;
}

void ROMProfile::fill_hidden_set( void ) {
  ROMVector::Raw hidden_packages = this->hidden_packages();
  ROMVector::Raw hidden_classes  = this->hidden_classes();
  ROMBitSet::Raw hidden_set      = this->hidden_set();;

  for( SystemClassStream st; st.has_next(); ) {
    InstanceClass::Raw klass = st.next();
    if( ROMOptimizer::class_matches_classes_list (&klass, &hidden_classes ) ||
        ROMOptimizer::class_matches_packages_list(&klass, &hidden_packages) ) {
      hidden_set().set_bit( klass().class_id() );
    }
  }
}

#if ENABLE_MEMBER_HIDING
bool ROMProfile::is_hidden_method(const InstanceClass* klass,
                                  const Method* method) const {
  const Symbol::Raw class_name = klass->name();
  const Symbol::Raw method_name = method->name();
  const OopDesc* method_signature = method->signature();

  const ROMVector::Raw class_patterns = hidden_method_classes();
  const ROMVector::Raw name_patterns = hidden_method_names();
  const ROMVector::Raw signatures = hidden_method_signatures();

  const int number_of_patterns = class_patterns().size();
  GUARANTEE(name_patterns().size() == number_of_patterns, "Sanity");
  GUARANTEE(signatures().size() == number_of_patterns, "Sanity");

  for( int i = 0; i < number_of_patterns; i++ ) {
    const SymbolDesc* class_pattern =
      (const SymbolDesc*) class_patterns().element_at(i);
    if( class_name().matches_pattern(class_pattern) ) {
      const SymbolDesc* name_pattern =
        (const SymbolDesc*) name_patterns().element_at(i);
      if( method_name().matches_pattern(name_pattern) ) {
        const OopDesc* signature = signatures().element_at(i);
        if( !signature || signature == method_signature ) {
          return true;
        }
      }
    }
  }
  return false;
}

bool ROMProfile::is_hidden_field (const InstanceClass* klass,
                                  const OopDesc* field) const {
  const Symbol::Raw class_name = klass->name();

  const ROMVector::Raw class_patterns = hidden_field_classes();
  const ROMVector::Raw name_patterns = hidden_field_names();

  const int number_of_patterns = class_patterns().size();
  GUARANTEE(name_patterns().size() == number_of_patterns, "Sanity");

  for( int i = 0; i < number_of_patterns; i++ ) {
    const SymbolDesc* class_pattern =
      (const SymbolDesc*) class_patterns().element_at(i);
    if( class_name().matches_pattern(class_pattern) ) {
      const SymbolDesc* name_pattern =
        (const SymbolDesc*) name_patterns().element_at(i);
      if( ((const SymbolDesc*)field)->matches_pattern(name_pattern) ) {
        return true;
      }
    }
  }
  return false;
}
#endif // ENABLE_MEMBER_HIDING

#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
