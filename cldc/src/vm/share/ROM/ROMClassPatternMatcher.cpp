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
#include "incls/_ROMClassPatternMatcher.cpp.incl"

#if ENABLE_ROM_GENERATOR

void ROMClassPatternMatcher::error(const char msg[]) {
#if USE_SOURCE_IMAGE_GENERATOR
  ROMOptimizer::config_warning(msg);
#else
  tty->print_cr(msg);
#endif
}

inline void ROMClassPatternMatcher::not_found(const char name[]) {
#if USE_SOURCE_IMAGE_GENERATOR
  ROMOptimizer::config_warning_not_found(name);
#else
  tty->print_cr("%s not found", name);
#endif
}

void ROMClassPatternMatcher::invalid_pattern(void) {
  error( "Invalid pattern" );
}

bool ROMClassPatternMatcher::validate(const char pattern[], const int length) {
  if( !length ) {
    invalid_pattern();
    return false;
  }
  for( int i = 0; i < length; i++ ) {
    switch( pattern[i] ) {
      case '*': if (i != (length-1))
      case '?': {
        error( "Only patterns with * at the end are supported" );
        return false;
      }
    }
  }
  return true;
}

void ROMClassPatternMatcher::handle_class(const InstanceClass* klass JVM_TRAPS){
  set_match_found();
}

bool ROMClassPatternMatcher::initialize(const char* pattern,
                                             const int length JVM_TRAPS) {
  if (!validate(pattern, length)) {
    return false;
  }

  _has_wildcards = pattern[length-1] == '*';
  OopDesc* p = SymbolTable::slashified_symbol_for((utf8) pattern,
                                               length JVM_NO_CHECK_AT_BOTTOM);
  _class = p;
  return p != NULL;
}

void ROMClassPatternMatcher::run(const char pattern[] JVM_TRAPS) {
  _match_found = false;

  const bool pattern_is_valid =
    initialize(pattern, jvm_strlen(pattern) JVM_NO_CHECK_AT_BOTTOM);
  if (!pattern_is_valid) {
    return;
  }

  UsingFastOops fast_oops;
  InstanceClass::Fast klass;

  if (!_has_wildcards) {
    LoaderContext ctx(&_class, ErrorOnFailure);
    klass = SystemDictionary::find(&ctx, /*lookup_only=*/ true,
                                         /*check_only= */ true JVM_NO_CHECK);
    if (klass.not_null()) {
      handle_class(&klass JVM_CHECK);
    } else {
      Thread::clear_current_pending_exception();
    }
  } else {
    for (SystemClassStream st; st.has_next();) {
      klass = st.next();
      if (match(&klass)) {
        handle_class(&klass JVM_CHECK);
      }
    }
  }
  if (!is_match_found()) {
    not_found(pattern);
  }
}

#endif // ENABLE_ROM_GENERATOR
