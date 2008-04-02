/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

#if ENABLE_COMPILER

// The code generator is an abstract three operand machine used
// by the compiler to generate native code. To facilitate porting the
// interface is intended to be platform independant.

class CodeGenerator: public BinaryAssembler {
 public:
  static CodeGenerator* allocate( const int size JVM_TRAPS ) {
    OopDesc* compiled_method = Universe::new_compiled_method(size JVM_NO_CHECK);
    if( compiled_method ) {
#ifdef PRODUCT
      CodeGenerator* gen = COMPILER_OBJECT_ALLOCATE( CodeGenerator );
      if( gen )
#else
      static CodeGenerator _code_generator;
      CodeGenerator* gen = &_code_generator;
#endif
      {
        _compiler_code_generator = gen;
        gen->initialize( compiled_method );
        return gen;
      }
    }
    return NULL;
  }
  static void terminate( void ) {
    _compiler_code_generator = NULL;
  }
  static CodeGenerator* current( void ) {
    return _compiler_code_generator;
  }

  // generate the jmp to interpreter_method_entry for overflow
  void overflow(const Assembler::Register&, const Assembler::Register&);
  // generate the code for the method entry of the given method
  void method_entry       (Method* method JVM_TRAPS);

#if ENABLE_REMEMBER_ARRAY_LENGTH
  //preload the first array object which appears in the method 
  //parameters in a register.
  void preload_parameter (Method* method);
#else
  void preload_parameter (Method* method) {}
#endif

  // load/store value from/to the location at the specified index
  void load_from_address  (Value& result, BasicType type,
                            MemoryAddress& address, Condition cond = always);
  void load_from_location (Value& result, jint index, Condition cond = always);
  void store_to_location  (Value& value,  jint index);

  // load/store value from/to given object at the specified offset
  void load_from_object   (Value& result, Value& object, jint offset, 
                              bool null_check JVM_TRAPS);
  void store_to_object    (Value& value,  Value& object, jint offset,
                              bool null_check JVM_TRAPS);

  // load/store value from/to given array at the specified index
  void load_from_array    (Value& result, Value& array, Value& index);
  void store_to_array     (Value& value,  Value& array, Value& index);

  // static array initialization
  void init_static_array  (Value& result JVM_TRAPS);

  // check that an object isn't null
  void null_check         (const Value& object JVM_TRAPS);
#if ENABLE_NPCE
  //do the null check by NPCE. if need_tigger_instr is true, compiler
  //will emit a LDR instr in order to trigger a exception in case of a 
  //access of null pointer.
  //if is_quick_return is true, the compiler won't record the LDR instr into the entry 
  //label of stub. Skip the recording due to the LDR instr is not emitted yet. It will 
  //be record later. see store_to_address_and_record_offset_of_exception_instr()
  void null_check_by_npce(Value& object, bool need_tigger_instr, 
            bool is_quick_return, BasicType type_of_data  JVM_TRAPS);

  //do the null check if needed. 
  void maybe_null_check_by_npce(Value& value, bool need_tigger_instr, 
             bool is_quick_return, BasicType type_of_data JVM_TRAPS) ;

private:

  //record the address of LDR instr during emit code of store to adress
  //this method is corresponding to the is_quick_return case in null_check_by_npce
  void store_to_address_and_record_offset_of_exception_instr (Value& value,  BasicType type,
                         MemoryAddress& address);
public:
#endif //ENABLE_NPCE
  // check that the array isn't null and that the given index is within the
  // bounds of the given array.
  void array_check        (Value& array, Value& index JVM_TRAPS);

  // check that the given object can be stored in the given array 
  void type_check         (Value& object, Value& array, Value& index JVM_TRAPS);

  // synchronize (enter/exit) on the given object
  void monitor_enter      (Value& object JVM_TRAPS);
  void monitor_exit       (Value& object JVM_TRAPS);

  // return (result) from the current activation
  void return_result      (Value& value JVM_TRAPS);
  void return_error       (Value& value JVM_TRAPS);
  void return_void        (JVM_SINGLE_ARG_TRAPS);

  // unlock the current activation (only works for synchronized methods)
  void unlock_activation  (JVM_SINGLE_ARG_TRAPS);

  // check that all monitors in the current activation are unlocked
  void check_monitors     (JVM_SINGLE_ARG_TRAPS);

  // pop all expression stack elements from the CPU stack thereby clearing it
  void clear_stack        (void);

  // clear a specific object location (used in merge code)
  void clear_object_location (jint index);

  // ensure there's sufficient room on the stack for storing a value
  // with the given kind at the given index
  void ensure_sufficient_stack_for(int index, BasicType kind);

  // platform independent move operations
  void move(const Value& dst, const Value& src, const Condition cond = always);
  void move(Value& dst, ExtendedValue& src, Condition cond = always);
  void move(Value& dst, Oop* obj,    Condition cond = always);
  void move(Assembler::Register dst, Assembler::Register src,
                                     Condition cond = always);

  // Branch unconditionally.
  void branch(int destination JVM_TRAPS);

  // Branch if integer comparison satisfies condition.
  void branch_if(BytecodeClosure::cond_op condition, int destination, 
                 Value& op1, Value& op2, const bool flags_set JVM_TRAPS); 

  // Branch if integer comparison satisfies condition.
  void branch_if(BytecodeClosure::cond_op condition, int destination, 
                 Value& op1, Value& op2 JVM_TRAPS) {
    branch_if( condition, destination, op1, op2, false JVM_NO_CHECK );
  }

  bool forward_branch_optimize(int next_bci,
                               BytecodeClosure::cond_op condition,
                               int destination, Value& op1, Value& op2
                               JVM_TRAPS);

  void branch_if_do(BytecodeClosure::cond_op condition, Value& op1, Value& op2,
                    int destination JVM_TRAPS);

  void if_then_else(Value& result, BytecodeClosure::cond_op condition,
                    Value& op1, Value& op2,
                    ExtendedValue& result_true, ExtendedValue& result_false
                    JVM_TRAPS);

  void if_iinc(Value& result, BytecodeClosure::cond_op condition,
               Value& op1, Value& op2,
               Value& value, int increment JVM_TRAPS);

  // Table/lookup switch.
  void table_switch(Value& index, jint table_index, jint default_dest, 
                    jint low, jint high JVM_TRAPS);

  void lookup_switch(Value& index, jint table_index, jint default_dest, 
                     jint num_of_pairs JVM_TRAPS);

    // Allocation.
  void new_object(Value& result, JavaClass* klass JVM_TRAPS);
  void new_object_array(Value& result, JavaClass* element_class, 
                        Value& length JVM_TRAPS);
  void new_basic_array(Value& result, BasicType type, Value& length JVM_TRAPS);
  void new_multi_array(Value& result JVM_TRAPS);

  // Check cast.
  void check_cast(Value& object, Value& klass, int class_id JVM_TRAPS);
  void instance_of(Value& result, Value& object, Value& klass, int class_id
                   JVM_TRAPS);

  // Conversions
  // l2i is handled in generic code
  void i2b(Value& result, Value& value JVM_TRAPS);
  void i2c(Value& result, Value& value JVM_TRAPS);
  void i2s(Value& result, Value& value JVM_TRAPS);
  void i2l(Value& result, Value& value JVM_TRAPS);

#if ENABLE_FLOAT
  void i2f(Value& result, Value& value JVM_TRAPS);
  void i2d(Value& result, Value& value JVM_TRAPS);
  void l2f(Value& result, Value& value JVM_TRAPS);
  void l2d(Value& result, Value& value JVM_TRAPS);
  void f2i(Value& result, Value& value JVM_TRAPS);
  void f2l(Value& result, Value& value JVM_TRAPS);
  void f2d(Value& result, Value& value JVM_TRAPS);
  void d2i(Value& result, Value& value JVM_TRAPS);
  void d2l(Value& result, Value& value JVM_TRAPS);
  void d2f(Value& result, Value& value JVM_TRAPS);
#endif

  // Compare operations.
  void long_cmp  (Value& result, Value& op1, Value& op2 JVM_TRAPS);

#if ENABLE_FLOAT
  void float_cmp (Value& result, BytecodeClosure::cond_op cond, 
                  Value& op1, Value& op2 JVM_TRAPS);
  void double_cmp(Value& result, BytecodeClosure::cond_op cond, 
                  Value& op1, Value& op2 JVM_TRAPS);
  void fpu_cmp_helper(Value& result, Value& op1, Value& op2, 
                      bool unordered_is_less);
#endif

  // Generic binary operations. These are the platform *independant* routines
  // the BytecodeCompileClosure calls. They do constant folding, so the
  // platform  *dependent* implementation does not have to worry about that.

  void int_binary(Value& result, Value& op1, Value& op2, 
                  BytecodeClosure::binary_op op JVM_TRAPS);
  void int_unary(Value& result, Value& op1, 
                  BytecodeClosure::unary_op op JVM_TRAPS);

  void long_binary(Value& result, Value& op1, Value& op2, 
                   BytecodeClosure::binary_op op JVM_TRAPS);
  void long_unary(Value& result, Value& op1, 
                   BytecodeClosure::unary_op op JVM_TRAPS);

#if ENABLE_FLOAT
  void float_binary(Value& result, Value& op1, Value& op2, 
                   BytecodeClosure::binary_op op JVM_TRAPS);
  void float_unary(Value& result, Value& op1, 
                   BytecodeClosure::unary_op op JVM_TRAPS);

  void double_binary(Value& result, Value& op1, Value& op2, 
                   BytecodeClosure::binary_op op JVM_TRAPS);
  void double_unary(Value& result, Value& op1, 
                   BytecodeClosure::unary_op op JVM_TRAPS);
#endif

  // Generate an osr entry.
  void osr_entry(JVM_SINGLE_ARG_TRAPS) {
    osr_entry(false JVM_NO_CHECK_AT_BOTTOM);
  }
  void osr_entry(bool force JVM_TRAPS);

  // Go to interpreted code by jumping through interpreter dispatch table.
  void go_to_interpreter(JVM_SINGLE_ARG_TRAPS);

  // Bail out to the interpreter.
  void uncommon_trap(JVM_SINGLE_ARG_TRAPS);

  void check_bytecode_counter();

  // Check for stack overflow and timer ticks
  void check_timer_tick(JVM_SINGLE_ARG_TRAPS);
  void check_stack_overflow(Method *m JVM_TRAPS);

  // Method invocation.
  void invoke(const Method* method, bool must_do_null_check JVM_TRAPS);
  void invoke_virtual(Method* method, int vtable_index, 
                      BasicType return_type JVM_TRAPS);
  void invoke_interface(JavaClass* klass, int itable_index, 
                        int parameters_size, BasicType return_type JVM_TRAPS);
  void invoke_native(BasicType return_kind, address entry JVM_TRAPS);

  bool quick_catch_exception(const Value &value, JavaClass* catch_type,
                             int handler_bci JVM_TRAPS);

  // Runtime call.
  void call_vm_extra_arg(const Register extra_arg);
  void call_vm_extra_arg(const int extra_arg);
  void call_vm(address entry, BasicType return_type_value JVM_TRAPS);

  // Check we have sufficient space. If not throw exception
  void check_free_space(JVM_SINGLE_ARG_TRAPS);

  void maybe_null_check(Value& value JVM_TRAPS);
  bool need_null_check(const Value &value) { 
    return !value.must_be_nonnull();
  }

  void check_cast_stub(CompilationQueueElement* cqe JVM_TRAPS);
  void instance_of_stub(CompilationQueueElement* cqe JVM_TRAPS);
  void new_object_stub(CompilationQueueElement* cqe JVM_TRAPS);
  void new_type_array_stub(CompilationQueueElement* cqe JVM_TRAPS);

  // Throw exception in the simple case (the method is not synchronized,
  // has no monitor bytecodes, and no handler in the current method covers
  // this exception).
  void throw_simple_exception(int rte JVM_TRAPS);

  // Can we generate a inline exception sequence for the current bci,
  // without creating a ThrowExceptionStub?
  // i.e., if no handler in the current method catches the exception, and
  // the current method does not do any locking.
  bool is_inline_exception_allowed(int rte JVM_TRAPS);

#ifndef PRODUCT
  void verify_location_is_constant(jint index, const Value& constant);
#endif

#if USE_AOT_COMPILATION && !ENABLE_ISOLATES
  void initialize_class(InstanceClass* klass JVM_TRAPS);
#endif

#if ENABLE_ISOLATES
  // Load task mirror. Performs class initialization barrier if necessary.
  // Used in conjunction with load_from_object & store_to_object.
  void load_task_mirror(Oop* klass, Value& statics_holder,
          bool needs_cib JVM_TRAPS);
  void check_cib(Oop* klass JVM_TRAPS);
#endif

#if ENABLE_INLINED_ARRAYCOPY
  bool arraycopy(JVM_SINGLE_ARG_TRAPS);
  bool unchecked_arraycopy(BasicType array_element_type JVM_TRAPS);  
#endif

  void bytecode_prolog();
  void flush_epilogue(JVM_SINGLE_ARG_TRAPS);

  bool omit_stack_frame( void ) const {
    return _omit_stack_frame;
  }
  void set_omit_stack_frame( const bool value ) {
    _omit_stack_frame = value;
  }

  OopDesc* finish( void );

 protected:

  // Generic binary operations.
  // NOTE: result must be uninitialized when this routine is called;
  //       op1 must be in a register; and
  //       op2 can be in a register or it can be an immediate.
  void int_binary_do(Value& result, Value& op1, Value& op2,
                     const BytecodeClosure::binary_op op JVM_TRAPS);
  void long_binary_do(Value& result, Value& op1, Value& op2,
                      BytecodeClosure::binary_op op JVM_TRAPS);

  void int_unary_do(Value& result, Value& op1, 
                     BytecodeClosure::unary_op op JVM_TRAPS);
  void long_unary_do(Value& result, Value& op1, 
                      BytecodeClosure::unary_op op JVM_TRAPS);
#if ENABLE_FLOAT
  void float_unary_do(Value& result, Value& op1, 
                      BytecodeClosure::unary_op op JVM_TRAPS);
  void double_unary_do(Value& result, Value& op1, 
                       BytecodeClosure::unary_op op JVM_TRAPS);

  // Alas, these must also handle constant folding, since that is more
  // machine-dependent than one would hope.   Maybe I can fix that later
  void float_binary_do(Value& result, Value& op1, Value& op2, 
                       BytecodeClosure::binary_op op JVM_TRAPS);

  void double_binary_do(Value& result, Value& op1, Value& op2, 
                        BytecodeClosure::binary_op op JVM_TRAPS);
#endif

  // Platform independant constant folding operations.
  void int_constant_fold(Value& result, Value& op1, Value& op2,
                         const BytecodeClosure::binary_op op JVM_TRAPS);

  void int_constant_fold(Value& result, Value& op1, 
                         const BytecodeClosure::unary_op op JVM_TRAPS);

  void long_constant_fold(Value& result, Value& op1, Value& op2,
                          const BytecodeClosure::binary_op op JVM_TRAPS);

  void long_constant_fold(Value& result, Value& op1, 
                          const BytecodeClosure::unary_op op JVM_TRAPS);

#if ENABLE_FLOAT
  void float_constant_fold(Value& result, Value& op1, 
                           const BytecodeClosure::unary_op op JVM_TRAPS);

  void double_constant_fold(Value& result, Value& op1, 
                            const BytecodeClosure::unary_op op JVM_TRAPS);
#endif

  // Operations on operations.
  static bool is_commutative( const BytecodeClosure::binary_op op );
  static bool is_reversible ( const BytecodeClosure::binary_op op );
  static BytecodeClosure::binary_op
         reverse_operation  ( const BytecodeClosure::binary_op op );

  // Integer arithmetic
  void irsb(Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void imul(Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void idiv(Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void irem(Value& result, Value& op1, Value& op2 JVM_TRAPS);

  // Integer logical operations
  void ishl (Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void ishr (Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void iushr(Value& result, Value& op1, Value& op2 JVM_TRAPS);

  // Long arithmetic. 
  void ladd(Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void lsub(Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void lrsb(Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void lmul(Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void ldiv(Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void lrem(Value& result, Value& op1, Value& op2 JVM_TRAPS);

  // Long logical operations.
  void land (Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void lor  (Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void lxor (Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void lshl (Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void lshr (Value& result, Value& op1, Value& op2 JVM_TRAPS);
  void lushr(Value& result, Value& op1, Value& op2 JVM_TRAPS);

  // Convert condition to jump condition.
  static Condition 
         convert_condition( const BytecodeClosure::cond_op condition );

  // Helper routine for calling routine system
  void runtime_long_op(Value& result, Value& op1, Value& op2,
                       bool check_zero, address routine JVM_TRAPS);

#if ENABLE_LOOP_OPTIMIZATION && ARM
  public:
  void conditional_jmp(Assembler::Condition cond,
                            int destination JVM_TRAPS);
#endif

 private:

  // store value to the given memory address
  void store_to_address (Value& value, BasicType type,
                         MemoryAddress& address);


  // Do conditional jump.
  void conditional_jump(const BytecodeClosure::cond_op condition,
                        const int destination,
                        const bool assume_backward_jumps_are_taken
                        JVM_TRAPS);

  void conditional_jump_do(BytecodeClosure::cond_op condition, 
                           Label& destination);

  static inline VirtualStackFrame* frame ( void );

  static jint bci ( void ) {
    return jvm_fast_globals.compiler_bci;
  }
  static Method* method ( void ) {
    return jvm_fast_globals.compiler_method;
  }
  static jint next_bci ( const jint bci ) {
    return method()->next_bci( bci );
  }
  static jint next_bci ( void ) {
    return next_bci( bci() );
  }

  void flush_frame(JVM_SINGLE_ARG_TRAPS);

#if ENABLE_APPENDED_CALLINFO
  CallInfoWriter _callinfo_writer;
  CallInfoWriter* callinfo_writer( void ) { return &_callinfo_writer; }
  void append_callinfo_record(const int code_offset JVM_TRAPS);
#endif

  void initialize( OopDesc* compiled_method ) {
    BinaryAssemblerCommon::initialize( compiled_method );
#if ENABLE_APPENDED_CALLINFO
    _callinfo_writer.initialize( this->compiled_method() );
#endif
    _omit_stack_frame = false;
  }

  bool _omit_stack_frame;

  // Platform dependent stuff
#include "incls/_CodeGenerator_pd.hpp.incl"
  friend class VirtualStackFrame;
  friend class ForwardBranchOptimizer;
};

#endif
