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

#if USE_DEBUG_PRINTING

/**
 * Closure to be passed to byte code iteration routines
 * to dispatch over different types of byte codes and
 * print out individual textual representations of them.
 */
class BytecodePrintClosure : public BytecodeClosure {
 public:
  BytecodePrintClosure(Stream* st, bool include_nl, bool verbose) {
    this->_st = st;
    this->_include_nl = include_nl;
    this->_verbose = verbose;
  }

  // Wrapper methods
  virtual void bytecode_prolog(JVM_SINGLE_ARG_TRAPS);
  virtual void bytecode_epilog(JVM_SINGLE_ARG_TRAPS);

  // Loads & stores
  virtual void push_int   (jint    value JVM_TRAPS);
  virtual void push_long  (jlong   value JVM_TRAPS);
  virtual void push_float (jfloat  value JVM_TRAPS);
  virtual void push_double(jdouble value JVM_TRAPS);
  virtual void push_obj   (Oop*    value JVM_TRAPS);

  virtual void load_local(BasicType kind, int index JVM_TRAPS);
  virtual void store_local(BasicType kind, int index JVM_TRAPS);

  virtual void increment_local_int(int index, jint offset JVM_TRAPS);

  // Array operations
  // void array_length(JVM_SINGLE_ARG_TRAPS);

  virtual void load_array(BasicType kind JVM_TRAPS);
  virtual void store_array(BasicType kind JVM_TRAPS);

  // Stack operations
  // void nop(JVM_SINGLE_ARG_TRAPS);
  // void pop(JVM_SINGLE_ARG_TRAPS);
  // void pop2(JVM_SINGLE_ARG_TRAPS);
  // void dup(JVM_SINGLE_ARG_TRAPS);
  // void dup2(JVM_SINGLE_ARG_TRAPS);

  // void dup_x1(JVM_SINGLE_ARG_TRAPS);
  // void dup2_x1(JVM_SINGLE_ARG_TRAPS);
  // void dup_x2(JVM_SINGLE_ARG_TRAPS);
  // void dup2_x2(JVM_SINGLE_ARG_TRAPS);
  // void swap(JVM_SINGLE_ARG_TRAPS);

  // Unary arithmetic operations
  virtual void neg(BasicType kind JVM_TRAPS);

  // Binary operations
  virtual void binary(BasicType kind, binary_op op JVM_TRAPS);

  // Conversion operations
  virtual void convert(BasicType from, BasicType to JVM_TRAPS);

  // Control transfer operations
  virtual void branch_if     (cond_op cond, int dest JVM_TRAPS);
  virtual void branch_if_icmp(cond_op cond, int dest JVM_TRAPS);
  virtual void branch_if_acmp(cond_op cond, int dest JVM_TRAPS);

  virtual void compare(BasicType kind, cond_op cond JVM_TRAPS);

  virtual void branch(int dest JVM_TRAPS);

  virtual void return_op(BasicType kind JVM_TRAPS);

  virtual void table_switch(jint table_index, jint default_dest, jint low, jint high
                    JVM_TRAPS);
  virtual void lookup_switch(jint table_index, jint default_dest, jint num_of_pairs
                     JVM_TRAPS);

  // get/set for dynamic class loading
  virtual void get_field(int index JVM_TRAPS);
  virtual void put_field(int index JVM_TRAPS);
    
  // For dynamic initialization and loading
  virtual void get_static(int index JVM_TRAPS);
  virtual void put_static(int index JVM_TRAPS);

  // Method invocation
  virtual void invoke_interface(int index, int nofArgs JVM_TRAPS);

  // Method invocations for dynamic loading of classes
  virtual void invoke_special(int index JVM_TRAPS);
  virtual void invoke_static(int index JVM_TRAPS);
  virtual void invoke_virtual(int index JVM_TRAPS);
  virtual void invoke_native(BasicType return_kind, address entry JVM_TRAPS);

  // Exception handling
  // void throw_exception(JVM_SINGLE_ARG_TRAPS);

  // Object and array allocation
  virtual void new_object(int index JVM_TRAPS);
  virtual void new_basic_array (int type JVM_TRAPS);
  virtual void new_object_array(int index JVM_TRAPS);
  virtual void new_multi_array(int index, int num_of_dims JVM_TRAPS);

  virtual void check_cast(int index JVM_TRAPS);
  virtual void instance_of(int index JVM_TRAPS);

  // Monitors
  // void monitor_enter(JVM_SINGLE_ARG_TRAPS);
  // void monitor_exit(JVM_SINGLE_ARG_TRAPS);

  // Extra methods for handling fast bytecodes that are hard to revert.
  virtual void fast_invoke_virtual(int index JVM_TRAPS);
  virtual void fast_invoke_virtual_final(int index JVM_TRAPS);
  virtual void fast_invoke_special(int index JVM_TRAPS);
  virtual void fast_get_field(BasicType field_type, int field_offset JVM_TRAPS);
  virtual void fast_put_field(BasicType field_type, int field_offset JVM_TRAPS);

  void print_verbose_interface_method(int index JVM_TRAPS);
  void print_verbose_virtual_method(int index JVM_TRAPS);
  void print_verbose_static_method(int index JVM_TRAPS);
  void print_verbose_class(int index JVM_TRAPS);
  void print_verbose_static_field(int index JVM_TRAPS);
#if !ENABLE_CPU_VARIANT
  virtual void init_static_array(JVM_SINGLE_ARG_TRAPS);
#endif
 private:
  void print_field_name(int index, int is_static, int is_get JVM_TRAPS);
  Stream* _st;
  bool _include_nl; // print a new line character after every bytecode.
  bool _verbose;
};

#endif
