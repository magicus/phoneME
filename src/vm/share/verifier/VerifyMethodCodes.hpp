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

/**
   \class VerifyMethodCodes
   Used by the verifier to perform operations on Java bytecodes 
   during bytecode verification. 

   This VM has a new, two-phase class file verifier. 
   In order to run on this VM, class files must first be processed 
   with a special "pre-verifier" tool. This phase is typically
   done on the development workstation.  During execution,
   the runtime verifier (defined in this file) of this 
   VM performs the actual class file verification based on 
   both runtime information and pre-verification information.
 */

class VerifyMethodCodes : public BytecodeClosure { 
  public:
  // The existence of this constructor helps GCC 2.9 generate
  // smaller code
  VerifyMethodCodes();

  ~VerifyMethodCodes() {
    // The existence of this destructor helps GCC 2.9 generate
    // smaller code
  }

  void verify(Method* method, ObjArray* stackmaps JVM_TRAPS);

  virtual void bytecode_prolog(JVM_SINGLE_ARG_TRAPS); //called before each bytecode
  virtual void bytecode_epilog(JVM_SINGLE_ARG_TRAPS); //called after each bytecode
  virtual void push_int   (jint    value JVM_TRAPS);
  virtual void push_long  (jlong   value JVM_TRAPS);
  virtual void push_float (jfloat  value JVM_TRAPS);
  virtual void push_double(jdouble value JVM_TRAPS);
  virtual void push_obj   (Oop*    value JVM_TRAPS);
  virtual void load_local(BasicType kind, int index JVM_TRAPS) ;
  virtual void store_local(BasicType kind, int index JVM_TRAPS);
  virtual void increment_local_int(int index, jint offset JVM_TRAPS);
  virtual void array_length(JVM_SINGLE_ARG_TRAPS);
  virtual void load_array(BasicType kind JVM_TRAPS);
  virtual void store_array(BasicType kind JVM_TRAPS);
  virtual void pop(JVM_SINGLE_ARG_TRAPS);
  virtual void pop2(JVM_SINGLE_ARG_TRAPS);
  virtual void dup(JVM_SINGLE_ARG_TRAPS);
  virtual void dup2(JVM_SINGLE_ARG_TRAPS);
  virtual void dup_x1(JVM_SINGLE_ARG_TRAPS);
  virtual void dup2_x1(JVM_SINGLE_ARG_TRAPS);
  virtual void dup_x2(JVM_SINGLE_ARG_TRAPS);
  virtual void dup2_x2(JVM_SINGLE_ARG_TRAPS);
  virtual void swap(JVM_SINGLE_ARG_TRAPS);
  virtual void neg(BasicType kind JVM_TRAPS);
  virtual void convert(BasicType from, BasicType to JVM_TRAPS) ;
  virtual void branch_if     (cond_op cond, int dest JVM_TRAPS);
  virtual void branch_if_icmp(cond_op cond, int dest JVM_TRAPS);
  virtual void branch_if_acmp(cond_op cond, int dest JVM_TRAPS);
  virtual void binary(BasicType kind, binary_op op JVM_TRAPS);
  virtual void compare(BasicType kind, cond_op cond JVM_TRAPS);
  virtual void branch(int dest JVM_TRAPS);
  virtual void return_op(BasicType kind JVM_TRAPS);
  virtual void table_switch(jint table_index, jint default_dest, jint low, 
                            jint high JVM_TRAPS);
  virtual void lookup_switch(jint table_index, jint default_dest, 
                             jint num_of_pairs JVM_TRAPS);
  virtual void get_field(int index JVM_TRAPS);
  virtual void put_field(int index JVM_TRAPS);
  virtual void get_static(int index JVM_TRAPS);
  virtual void put_static(int index JVM_TRAPS);
  virtual void invoke_interface(int index, int num_of_args JVM_TRAPS);
  virtual void invoke_special(int index JVM_TRAPS);
  virtual void invoke_static(int index JVM_TRAPS);
  virtual void invoke_virtual(int index JVM_TRAPS);
  virtual void invoke_native(BasicType return_kind, address entry JVM_TRAPS);
  virtual void throw_exception(JVM_SINGLE_ARG_TRAPS);
  virtual void new_object(int index JVM_TRAPS);
  virtual void new_basic_array(int type JVM_TRAPS);
  virtual void new_object_array(int index JVM_TRAPS);
  virtual void new_multi_array(int index, int num_of_dims JVM_TRAPS);
  virtual void check_cast(int index JVM_TRAPS);
  virtual void instance_of(int index JVM_TRAPS);
  virtual void monitor_enter(JVM_SINGLE_ARG_TRAPS);
  virtual void monitor_exit(JVM_SINGLE_ARG_TRAPS);
  virtual void fast_invoke_virtual(int index JVM_TRAPS);
  virtual void fast_invoke_virtual_final(int index JVM_TRAPS);
  virtual void fast_get_field(BasicType field_type, int field_offset JVM_TRAPS);
  virtual void fast_put_field(BasicType field_type, int field_offset JVM_TRAPS);

  virtual void uncommon_trap(JVM_SINGLE_ARG_TRAPS);
  virtual void illegal_code(JVM_SINGLE_ARG_TRAPS);
    
private:
  void populate_stackmap_cache(Method *method);
  void check_not_final_override(Method *method JVM_TRAPS);
  void check_handlers(Method *method, TypeArray* starts JVM_TRAPS);
  void check_news(ObjArray* stackmaps, TypeArray* starts JVM_TRAPS);
  void handle_field_reference(int index, bool is_static, bool is_put JVM_TRAPS);
  void handle_invoker(int index, int num_of_args, Bytecodes::Code code JVM_TRAPS);
  
  bool is_protected_access(int index, bool is_method JVM_TRAPS);
  
  void push(BasicType type JVM_TRAPS);
  void push_integer(JVM_SINGLE_ARG_TRAPS);
  void push_long(JVM_SINGLE_ARG_TRAPS);
  void push_float(JVM_SINGLE_ARG_TRAPS);
  void push_double(JVM_SINGLE_ARG_TRAPS);
  
  void pop_integer(JVM_SINGLE_ARG_TRAPS);
  void pop(BasicType type JVM_TRAPS);
  void pop_array_of(BasicType type JVM_TRAPS);
  
  void initialize_locals(Method* method JVM_TRAPS);
  void check_local(int index, BasicType type JVM_TRAPS);
  void set_local(int index, BasicType type JVM_TRAPS);
  
  VerifierFrame* frame() { return &_verifier_frame; }
  void print_frame() { frame()->print_frame(); }
  
  int pop_invoke_arguments(Signature* method_signature JVM_TRAPS) { 
    return frame()->pop_invoke_arguments(method_signature JVM_NO_CHECK_AT_BOTTOM);
  }
  void push_invoke_result(Signature* method_signature JVM_TRAPS);
  void method_return_type(Signature* method_signature, 
                          BasicType& kind, Symbol *klass JVM_TRAPS);
  bool is_subclass_of(JavaClass* from, Symbol* to_name);
  
  bool is_array_name(Symbol* name);
  ReturnOop reference_array_element_name(Symbol *array_name);

private:
  bool no_control_flow() { return _no_control_flow; }
  void set_no_control_flow (bool no_control_flow) { 
    _no_control_flow = no_control_flow;
  }

  int  get_stackmap_entry_offset(int index);
  void check_exception_handlers_for_bci(JVM_SINGLE_ARG_TRAPS);
  void initialize_exceptions_start_end();

private:
  FastOopInStackObj  _must_precede_fast_oop;
  Symbol::Fast       _fast_name1, _fast_name2, _fast_name3, _fast_name4;
  VerifierFrame      _verifier_frame;
  bool               _no_control_flow;
  int                _current_stackmap_index;
  int                _num_stackmaps;
  jushort            _min_exception_start_bci; // min bci for the beginning
                                               // of a try block (inclusive)
  jushort            _max_exception_end_bci;   // max bci for the end
                                               // of a try block (exclusive)
};
