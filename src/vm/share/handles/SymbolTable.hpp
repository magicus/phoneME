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

class SymbolTable: public ObjArray {
public:
  HANDLE_DEFINITION(SymbolTable, ObjArray);

  static juint hash(utf8 name, int name_length);
  static juint hash(Symbol *symbol);

  static SymbolTable* current( void ) {
    return Universe::symbol_table();
  }

  static ReturnOop symbol_for(const char * s JVM_TRAPS) {
    return current()->symbol_for(NULL, (utf8)s, jvm_strlen(s),
                                  /*check_only*/false JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop symbol_for(TypeArray *byte_array, utf8 s, int length
                              JVM_TRAPS) {
    return current()->symbol_for(byte_array, s, length, /*check_only*/false
                                  JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop symbol_for(TypeArray *byte_array JVM_TRAPS) {
    return current()->symbol_for(byte_array, (utf8)(byte_array->base_address()),
                                  byte_array->length(), /*check_only*/false
                                  JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop symbol_for(utf8 s, int len JVM_TRAPS) {
    return current()->symbol_for(NULL, s, len, /*check_only*/false
                                  JVM_NO_CHECK_AT_BOTTOM);
  }
  // Used by KNI -- Check only. Don't allocate if not found
  static ReturnOop check_symbol_for(const char * s JVM_TRAPS) {
    return current()->symbol_for(NULL, (utf8)s, jvm_strlen(s),
                                  /*check_only*/true JVM_NO_CHECK_AT_BOTTOM);
  }
  // Used by KNI
  static ReturnOop symbol_for(String* string JVM_TRAPS) {
    return symbol_for(string, false JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop slashified_symbol_for(utf8 s, int len JVM_TRAPS);

  // Used by KNI
  static ReturnOop slashified_symbol_for(const char * s JVM_TRAPS);
  // Used by KNI
  static ReturnOop slashified_symbol_for(String* string JVM_TRAPS) {
    return symbol_for(string, true JVM_NO_CHECK_AT_BOTTOM);
  }
  
  static ReturnOop initialize(const int size JVM_TRAPS) {
    GUARANTEE(is_power_of_2(size), "sanity");
    return Universe::new_obj_array(size JVM_NO_CHECK_AT_BOTTOM);
  }

private:
  // This is the main work-horse for looking up (and optionally creating)
  // a Symbol. The UTF8 source of the symbol may come from either byte_array,
  // or s.
  //
  // If byte_array is non-null, it contains the source of the UTF8 characters,
  // and the length of the source is equal to byte_array->length(). In this
  // case, s must point to the interior of byte_array.
  //
  // If byte_array is null, the source is pointed to by s and the length is
  // len.
  ReturnOop symbol_for(TypeArray *byte_array, utf8 s, int len, 
                       bool check_only JVM_TRAPS);
  
  // Maximum (desired) number of symbols to be contained by
  // this symbol table. Note: this table may contain more than this
  // number of Symbols if an attempt to grow the table has failed.
  jint desired_max_symbol_count() const {
    return length() / 4;
  }

  static ReturnOop symbol_for(String* string, bool slashify JVM_TRAPS);

  void grow_and_replace_symbol_table(void);
  void insert(Symbol* symbol);

  inline ReturnOop find_from_rom(int i, utf8 s, int len);

  friend class ObjectHeap;
};
