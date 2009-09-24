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
#include "incls/_ROMFieldPatternMatcher.cpp.incl"

#if ENABLE_ROM_GENERATOR && ENABLE_MEMBER_HIDING

inline
bool ROMFieldPatternMatcher::match_field(const Field* field) const {
  return name_matches_pattern((SymbolDesc*)field->name(), &_name);
}

bool ROMFieldPatternMatcher::initialize(const char* pattern, const int len
                                         JVM_TRAPS) {
  const bool pattern_is_valid = 
    ROMMemberPatternMatcher::initialize(pattern, len JVM_NO_CHECK_AT_BOTTOM);
  if (pattern_is_valid) {
    if (_signature.is_null()) {
      return true;
    }
    invalid_pattern();
  }
  return false;
}

void
ROMFieldPatternMatcher::handle_class(const InstanceClass* klass JVM_TRAPS) {
  TypeArray::Raw fields = klass->fields();
  const int length = fields().length();
  for (int i = 0; i < length; i += Field::NUMBER_OF_SLOTS) {
    const Field field((InstanceClass*) klass, i, &fields);
    if (match_field(&field)) {
      set_match_found();
      handle_matching_field(&field);
    }
  }
}

void ROMFieldPatternMatcher::handle_matching_field(const Field* field) {
  // Do nothing
}
#endif // ENABLE_ROM_GENERATOR && ENABLE_MEMBER_HIDING
