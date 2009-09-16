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

class ROMMethodPatternMatcher: public ROMClassPatternMatcher {
private:
  Symbol _method, _signature;

protected:
  virtual bool initialize(const char* pattern, const int length JVM_TRAPS);
  virtual void handle_class(const InstanceClass* klass JVM_TRAPS);

  // Override this method to handle all matching methods
  virtual void handle_matching_method(Method* method JVM_TRAPS);

  bool match_method(const Method* method) const {
    return name_matches_pattern((SymbolDesc*)method->name(), &_method) &&
           (_signature.is_null() || _signature.obj() == method->signature());
  }
};

#endif // ENABLE_ROM_GENERATOR
