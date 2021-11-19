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

#if ENABLE_ROM_GENERATOR

class ROMClassPatternMatcher {
private:
  Symbol _class;
  bool   _has_wildcards;
  bool   _match_found;

protected:
  static bool name_matches_pattern(const char name[],    const int name_len, 
                                   const char pattern[], const int pattern_len){
    return Universe::name_matches_pattern(name, name_len, pattern, pattern_len);
  }
  static bool name_matches_pattern(const SymbolDesc* name, const Symbol* pattern) {
    return name_matches_pattern(name->utf8_data(), name->utf8_length(),
                                pattern->utf8_data(), pattern->length());
  }

  bool match(const InstanceClass* klass) const {
    return name_matches_pattern((SymbolDesc*)klass->name(), &_class);
  }

  // Override this method to parse pattern
  virtual bool initialize(const char pattern[], const int len JVM_TRAPS);

  // Override this method to handle all matching classes
  virtual void handle_class(const InstanceClass* klass JVM_TRAPS);

  void set_match_found(void) {
    _match_found = true;
  }

  static void error(const char msg[]);
  static void invalid_pattern(void);
  static void not_found(const char name[]);
public:
  static bool validate(const char pattern[], const int length);

  bool is_match_found(void) const {
    return _match_found;
  }

  const Symbol* class_pattern(void) const {
    return &_class;
  }

  void run(const char pattern[] JVM_TRAPS);
};

#endif // ENABLE_ROM_GENERATOR
