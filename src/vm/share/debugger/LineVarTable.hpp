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

class LineVarTable : public ObjArray {
public:
  HANDLE_DEFINITION_CHECK(LineVarTable, ObjArray);

  enum {
    LINE_TABLE_INDEX = 0,
    VAR_TABLE_INDEX,
    SIZEOF_LINE_VAR
  };

  ReturnOop line_number_table() {
    return obj_at(LINE_TABLE_INDEX);
  }

  ReturnOop local_variable_table() {
    return obj_at(VAR_TABLE_INDEX);
  }

  void set_line_number_table(OopDesc *value) {
    obj_at_put(LINE_TABLE_INDEX, value);
  }

  void set_local_variable_table(OopDesc *value) {
    obj_at_put(VAR_TABLE_INDEX, value);
  }

  void clear_line_number_table() {
    obj_at_clear(LINE_TABLE_INDEX);
  }

  void clear_local_variable_table() {
    obj_at_clear(VAR_TABLE_INDEX);
  }

  static ReturnOop new_line_var_table(JVM_SINGLE_ARG_TRAPS) {
    return Universe::new_obj_array(SIZEOF_LINE_VAR JVM_NO_CHECK_AT_BOTTOM);
  }
};

class LocalVariableTable : public TypeArray {
public:
  HANDLE_DEFINITION_CHECK(LocalVariableTable, TypeArray);

  // each entry is tuple of shorts: code_index, code_length, slot

  bool is_compressed() {
    return is_byte_array();
  }

  jushort start_pc(int index) {
    GUARANTEE(((index * 3) + 1) < length(),
              "local var table: start_pc index out of range");
    return is_compressed() ? ubyte_at((index * 3) + 1) :
                   ushort_at((index * 3) + 1);
  }

  void set_start_pc(int index, jushort start_pc) {
    GUARANTEE(((index * 3) + 1) < length(),
              "local var table: start_pc index out of range");
    GUARANTEE(is_short_array(), "Local variable table wrong type");
    ushort_at_put((index * 3) + 1, start_pc);
  }

  void set_start_pc(int index, jubyte start_pc) {
    GUARANTEE(((index * 3) + 1) < length(),
              "local var table: start_pc index out of range");
    GUARANTEE(is_byte_array(), "Local variable table wrong type");
    ubyte_at_put((index * 3) + 1, start_pc);
  }

  jushort code_length(int index) {
    GUARANTEE(((index * 3) + 2) < length(),
              "local var table: code_length index out of range");
    return is_compressed() ? ubyte_at((index * 3) + 2) :
                   ushort_at((index * 3) + 2);
  }

  void set_code_length(int index, jushort code_length) {
    GUARANTEE(((index * 3) + 2) < length(),
              "local var table: code_length index out of range");
    GUARANTEE(is_short_array(), "Local variable table wrong type");
    ushort_at_put((index * 3) + 2, code_length);
  }

  void set_code_length(int index, jubyte code_length) {
    GUARANTEE(((index * 3) + 2) < length(),
              "local var table: code_length index out of range");
    GUARANTEE(is_byte_array(), "Local variable table wrong type");
    ubyte_at_put((index * 3) + 2, code_length);
  }

  jushort slot(int index) {
    GUARANTEE((index * 3) < length(),
              "local var table: slot index out of range");
    return is_compressed() ? ubyte_at(index * 3) :
                   ushort_at(index * 3);
  }

  void set_slot(int index, jushort slot) {
    GUARANTEE((index * 3) < length(),
              "local var table: slot index out of range");
    GUARANTEE(is_short_array(), "Local variable table wrong type");
    ushort_at_put(index * 3, slot);
  }

  void set_slot(int index, jubyte slot) {
    GUARANTEE((index * 3) < length(),
              "local var table: slot index out of range");
    GUARANTEE(is_byte_array(), "Local variable table wrong type");
    ubyte_at_put(index * 3, slot);
  }
  
  jint count() {
    GUARANTEE((length() % 3) == 0, "Illegal size of local variable table");
    return (length() / 3);
  }

};

class LineNumberTable : public TypeArray {
public:
  HANDLE_DEFINITION_CHECK(LineNumberTable, TypeArray);

  bool is_compressed() {
    return is_byte_array();
  }

  jushort pc(int index) {
    GUARANTEE((index * 2) < length(),
              "line number table pc index out of bounds");
    return is_compressed() ? ubyte_at(index * 2) :
                   ushort_at(index * 2);
  }

  void set_pc(int index, jushort start_pc) {
    GUARANTEE((index * 2) < length(),
              "line number table pc index out of bounds");
    GUARANTEE(is_short_array(), "Line number table wrong type");
    short_at_put(index * 2, start_pc);
  }
  
  void set_pc(int index, jubyte start_pc) {
    GUARANTEE((index * 2) < length(),
              "line number table pc index out of bounds");
    GUARANTEE(is_byte_array(), "Line number table wrong type");
    ubyte_at_put(index * 2, start_pc);
  }

  jushort line_number(int index) {
    GUARANTEE(((index * 2) + 1) < length(),
              "line number table line_num index out of bounds");
    return is_compressed() ? ubyte_at((index * 2) + 1) :
                  short_at((index * 2) + 1);
  }

  void set_line_number(int index, jushort line_num) {
    GUARANTEE(((index * 2) + 1) < length(),
              "line number table line_num index out of bounds");
    GUARANTEE(is_short_array(), "Line number table wrong type");
    ushort_at_put((index * 2) + 1, line_num);
  }

  void set_line_number(int index, jubyte line_num) {
    GUARANTEE(((index * 2) + 1) < length(),
              "line number table line_num index out of bounds");
    GUARANTEE(is_byte_array(), "Line number table wrong type");
    ubyte_at_put((index * 2) + 1, line_num);
  }

  jint count() {
    GUARANTEE((length() % 2) == 0, "Illegal size of line number table");
    return (length() / 2);
  }
};
