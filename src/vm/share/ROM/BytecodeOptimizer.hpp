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

#if ENABLE_ROM_GENERATOR && !USE_PRODUCT_BINARY_IMAGE_GENERATOR
class ConstantPoolRewriter;

class BytecodeOptimizer {
public:
  BytecodeOptimizer() {
    jvm_memset(_num_optimized_bytecodes, 0, sizeof(_num_optimized_bytecodes));
  };
  void set_owner(ConstantPoolRewriter* p_owner) {
    _owner = p_owner;
  }
  ReturnOop optimize_bytecodes(Method *method JVM_TRAPS);
  void print_bytecode_statistics(Stream *stream);

private:
  ReturnOop optimize_static_arrays(Method *method JVM_TRAPS);
  bool has_static_arrays(Method *method, int& new_method_size);  
  void add_static_array_header(Method *method, int bci, int code_for_array_type);
  void add_static_array_item(Method *method, int value1, int value2, 
       int code_for_array_type, int number, int bci_start);



  bool check_array_index(Bytecodes::Code code, Method* method, int bci);
  bool get_values(Bytecodes::Code code, Method* method, int bci, int& value1,
                  int& value2);
  void get_values_from_cp(Method* method, int index, int& value1, int& value2);

  

  ConstantPoolRewriter* _owner;
  // Accounting: how many bytecodes optimized
  int _num_optimized_bytecodes[Bytecodes::number_of_java_codes];

  //these values are used for parsing
  int delta_offset;
  int parser_state;
  int array_element_count;
  int old_code_size;

  void reset_parser();
  void reset_parser(Method* m, int& bci_stackmap_update_from,
                    int& stackmap_shift JVM_TRAPS);
};
#endif //ENABLE_ROM_GENERATOR && !USE_PRODUCT_BINARY_IMAGE_GENERATOR
