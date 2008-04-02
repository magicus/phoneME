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

#ifndef PRODUCT
class StringTableVisitor : public StackObj {
 public:
  virtual void do_string(String* string JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    JVM_PURE_VIRTUAL_1_PARAM(string);
  }
};
#endif

class StringTable: public ObjArray {
  static juint _count;

public:
  HANDLE_DEFINITION(StringTable, ObjArray);
  
  static StringTable* current( void ) {
    return Universe::string_table();
  }

  ReturnOop interned_string_for(String *string JVM_TRAPS);

  static ReturnOop initialize(const int size JVM_TRAPS) {
    GUARANTEE(is_power_of_2(size), "sanity");
    ReturnOop p = Universe::new_obj_array(size JVM_NO_CHECK);
    return p;
  }

#ifndef PRODUCT
  void iterate(StringTableVisitor* visitor JVM_TRAPS);
#endif

private:
  // Rehash all symbols into a new bigger table
  void expand(JVM_SINGLE_ARG_TRAPS);
  void insert(String* string);

friend class ObjectHeap;
};
