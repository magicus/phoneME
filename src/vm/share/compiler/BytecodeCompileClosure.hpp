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

#if ENABLE_COMPILER

class BytecodeCompileClosure: public BytecodeClosure {
 private:
  Compiler* _compiler;
  CodeGenerator* _code_generator;
  int       _active_bci;
  jubyte    _has_exception_handlers;
  bool      _has_clinit;   // if this or any super has clinit 
#if ENABLE_CODE_PATCHING
  static int _jump_from_bci;
#endif

 public:
  BytecodeCompileClosure() {
    _active_bci = -10000;
  }
  void initialize(Compiler* compiler, Method* method, int active_bci);
  void set_code_generator(CodeGenerator* value) {
    _code_generator = value;
  }
  void set_compiler(Compiler* compiler) {
    _compiler = compiler;
  }
#if ENABLE_CODE_PATCHING
  static int jump_from_bci(void) {
    return _jump_from_bci;
  }
  static void set_jump_from_bci(const int bci) {
    _jump_from_bci = bci;
  }
  void set_jump_from_current_bci(const int dest) {
    const int current= bci();
    if( dest < current ) {
      set_jump_from_bci( bci );
    }
  }
#else
  static void set_jump_from_bci(const int /*bci*/) {}
  static void set_jump_from_current_bci(const int /*dest*/) {}
#endif  
  void set_active_bci(const int bci) {
    _active_bci = bci;
  }
  int active_bci(void) const {
    return _active_bci;
  }
  bool is_active_bci(const int bci) const {
    return _active_bci == bci;
  }
  bool is_active_bci(void) const {
    return is_active_bci( bci() );
  }

  void set_has_clinit(bool val) {
    _has_clinit = val;
  }
  bool has_clinit(void) const {
    return _has_clinit;
  }
  // Epilogue and prologue.
  virtual void bytecode_prolog(JVM_SINGLE_ARG_TRAPS);
  virtual void bytecode_epilog(JVM_SINGLE_ARG_TRAPS);

  // Push operations.
  virtual void push_int   (jint    value JVM_TRAPS);
  virtual void push_long  (jlong   value JVM_TRAPS);
  virtual void push_obj   (Oop*    value JVM_TRAPS);
  virtual void push_float (jfloat  value JVM_TRAPS);
  virtual void push_double(jdouble value JVM_TRAPS);

  // Local operations.
  virtual void load_local (BasicType kind, int index JVM_TRAPS);
  virtual void store_local(BasicType kind, int index JVM_TRAPS);

  // Increment local integer operation.
  virtual void increment_local_int(int index, jint offset JVM_TRAPS);

  // Array operations
  virtual void array_length(JVM_SINGLE_ARG_TRAPS);
  virtual void load_array(BasicType kind JVM_TRAPS);
  virtual void store_array(BasicType kind JVM_TRAPS);

  // Binary operations.
  virtual void binary(BasicType kind, binary_op op JVM_TRAPS);
  void unary(BasicType kind, unary_op op JVM_TRAPS);

  // Basic type conversions
  virtual void convert(BasicType from, BasicType to JVM_TRAPS);

  // Unary arithmetic operations
  virtual void neg(BasicType kind JVM_TRAPS);
 
  // Low-level stack manipulation operations.
  virtual void pop(JVM_SINGLE_ARG_TRAPS);
  virtual void pop_and_npe_if_null(JVM_SINGLE_ARG_TRAPS);
  virtual void pop2(JVM_SINGLE_ARG_TRAPS);
  virtual void dup(JVM_SINGLE_ARG_TRAPS);
  virtual void dup2(JVM_SINGLE_ARG_TRAPS);
  virtual void dup_x1(JVM_SINGLE_ARG_TRAPS);
  virtual void dup2_x1(JVM_SINGLE_ARG_TRAPS);
  virtual void dup_x2(JVM_SINGLE_ARG_TRAPS);
  virtual void dup2_x2(JVM_SINGLE_ARG_TRAPS);
  virtual void swap(JVM_SINGLE_ARG_TRAPS);
#if !ENABLE_CPU_VARIANT
  virtual void init_static_array(JVM_SINGLE_ARG_TRAPS);
#endif 
  // Branches.
  virtual void branch        (int dest JVM_TRAPS);
  virtual void branch_if     (cond_op cond, int dest JVM_TRAPS);
  virtual void branch_if_icmp(cond_op cond, int dest JVM_TRAPS);
  virtual void branch_if_acmp(cond_op cond, int dest JVM_TRAPS);
#if ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS
  void branch_if_flags(JVM_SINGLE_ARG_TRAPS);
#endif

  // Compare operations.
  virtual void compare(BasicType kind, cond_op cond JVM_TRAPS);

  // Type check bytecodes.
  virtual void check_cast (int index JVM_TRAPS);
  virtual void instance_of(int index JVM_TRAPS);

  // Returns.
  virtual void throw_exception(JVM_SINGLE_ARG_TRAPS);
  virtual void return_op(BasicType kind JVM_TRAPS);

  virtual void table_switch (jint table_index, jint default_dest, jint low,
                             jint high JVM_TRAPS);
  virtual void lookup_switch(jint table_index, jint default_dest,
                             jint num_of_pairs JVM_TRAPS);

  // Field accessors.
  void non_fast_field_access(int index, bool is_get JVM_TRAPS);
  virtual void get_field(int index JVM_TRAPS);
  virtual void put_field(int index JVM_TRAPS);
  virtual void fast_get_field(BasicType field_type, int field_offset JVM_TRAPS);
  virtual void fast_put_field(BasicType field_type, int field_offset JVM_TRAPS);

  // Static field accessors.
  virtual void get_static(int index JVM_TRAPS);
  virtual void put_static(int index JVM_TRAPS);

  // Allocation.
  virtual void new_object(int index JVM_TRAPS);
  virtual void new_basic_array(int type JVM_TRAPS);
  virtual void new_object_array(int index JVM_TRAPS);
  virtual void new_multi_array(int index, int num_of_dims JVM_TRAPS);

  // Monitor operations.
  virtual void monitor_enter(JVM_SINGLE_ARG_TRAPS);
  virtual void monitor_exit(JVM_SINGLE_ARG_TRAPS);

  // Method invocations.
  virtual void invoke_static(int index JVM_TRAPS);
  virtual void invoke_interface(int index, int num_of_args JVM_TRAPS);
  virtual void fast_invoke_virtual(int index JVM_TRAPS);
  virtual void fast_invoke_virtual_final(int index JVM_TRAPS);
  virtual void fast_invoke_special(int index JVM_TRAPS);
  virtual void invoke_native(BasicType return_kind, address entry JVM_TRAPS);

  // Unresolved method invocations.
  virtual void invoke_special(int index JVM_TRAPS);
  virtual void invoke_virtual(int index JVM_TRAPS);

  jubyte get_invoker_tag(int index, Bytecodes::Code bytecode JVM_TRAPS);
  bool try_resolve_invoker(int index, Bytecodes::Code bytecode JVM_TRAPS);
  bool try_resolve_static_field(int index, bool is_get JVM_TRAPS);

  // Compile the current compiler bytecode.
  bool compile(JVM_SINGLE_ARG_TRAPS);

#if USE_DEBUG_PRINTING
  void print_on(Stream *st);
  void p();
#endif

 private:
  void check_exception_handler_start(JVM_SINGLE_ARG_TRAPS);

  // Helper function for invoking a method directly (w/o going through
  // vtable or itable.
  void do_direct_invoke(Method * method, bool must_do_null_check JVM_TRAPS);

  // Helper function for invoking a method directly (w/o going through
  // vtable or itable.
  void direct_invoke(int index, bool must_do_null_check JVM_TRAPS);

  // Helper to retrieve the class at cp_index
  ReturnOop get_klass_or_null(int cp_index, bool must_be_initialized JVM_TRAPS);

  // Helper to retrieve the class at class_id (must be initialized)
  ReturnOop get_klass_or_null_from_id(int class_id);
#if USE_AOT_COMPILATION && !ENABLE_ISOLATES
  // Helper to retrieve the class at class_id and initialize it if needed.
  ReturnOop get_klass_from_id_and_initialize(int class_id JVM_TRAPS);
#endif

#if ENABLE_COMPILER_TYPE_INFO
  // Assign a class id to the passed field value.
  void set_field_class_id(Value& field_value, InstanceClass * ic, 
                          int field_offset, bool is_static);
#endif

  // Accessors for the next bytecode index.
  int _next_bytecode_index;
  int  next_bytecode_index    (void) const { return _next_bytecode_index; }
  void set_next_bytecode_index(const int bci) { _next_bytecode_index = bci; }

  bool is_compilation_done()   const { return next_bytecode_index() == -1;  }
  void terminate_compilation()       { set_next_bytecode_index(-1);         }

  // Accessor for the stack frame.
  inline VirtualStackFrame* frame     ( void ) const;

  // Accessors for the code generator used to do the compilation.
  inline CodeGenerator* code_generator( void ) const;

  friend class CodeGenerator;
  friend class ForwardBranchOptimizer;

  void frame_push(Value &value);
  void frame_pop(Value& value);

private:
  // If next bytecode can be read ahead, returns its index, otherwise -1
  int get_next_bci( void ) const;

  void set_default_next_bytecode_index(const jint bci);

  void throw_null_pointer_exception(JVM_SINGLE_ARG_TRAPS);
  void array_check(Value& array, Value& index JVM_TRAPS);

  void osr_entry(JVM_SINGLE_ARG_TRAPS);
#if ENABLE_ISOLATES
  // Determine the requirement for a class initialization barrier
  // when using a class. The access_static_var flag indicate that
  // the requirement are for accessing an static variable, which may
  // further needs masking.
  void cib_requirements(int class_id, bool &needs_cib,
          bool access_static_var);
#endif

#if ENABLE_CSE
  enum {
    not_found = -1 //no eliminatable byte codes is found
  };

  //can the byte codes start from current bci be eliminated
  bool is_following_codes_can_be_eliminated(jint& bci_after_elimination);

  //record the byte codes creating current result value into the  notation
  //of its register
  void record_current_code_snippet(void);

  //test whether current byte code can be elimiated before compiling it
  bool code_eliminate_prologue(Bytecodes::Code code);

  //do byte codes information record after compiling of current byte code.
  //So the CSE algorithm could use the recorded infomation and the result 
  //created by current byte code to find new redundant byte codes.
  void code_eliminate_epilogue(Bytecodes::Code code);

  //if the multi-entry is caused by osr, we try to preserve the register 
  //notation as much as possible.  
  void record_passable_statue_of_osr_entry(int dest_bci);
#else
  bool code_eliminate_prologue(Bytecodes::Code code) {return false;}
  void code_eliminate_epilogue(Bytecodes::Code code) {}
  void record_passable_statue_of_osr_entry(int dest) {}
#endif

  class PoppedValue : public Value {
  public:
    PoppedValue(BasicType type);
  };
};

#endif
