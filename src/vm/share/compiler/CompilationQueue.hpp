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

class CompilationQueueElement: public CompilerObject {  
 public:
  enum CompilationQueueElementType {
    compilation_continuation,
    throw_exception_stub,
    type_check_stub,
    check_cast_stub,
    instance_of_stub,
#if ENABLE_INLINE_COMPILER_STUBS
    new_object_stub,
    new_type_array_stub,
#endif
    osr_stub,
    stack_overflow_stub,
    timer_tick_stub,
    quick_catch_stub
#if ENABLE_INTERNAL_CODE_OPTIMIZER
    , entry_stub
#endif
  };

 protected:
  CompilationQueueElement*     _next;   // Link to next element in queue
  VirtualStackFrame*           _frame;  // The virtual stack frame  
  CompilationQueueElementType  _type;   // Type of this queue element  
  int                          _bci;    // The bytecode index
  // Labels.
  int                          _entry_label;
  int                          _return_label;
  // Registers.
  Assembler::Register          _register_0;
  Assembler::Register          _register_1;
  // A spare location
  int                          _info;
  // Used by CompilationContinuation: code size before compiling this element.
  int                          _code_size_before;
  // true if this is a CompilationContinuation and it has been suspended.
  jboolean                     _is_suspended;
  jboolean                     _entry_has_been_bound;

 public:
#if ENABLE_LOOP_OPTIMIZATION && ARM
  enum {
    max_search_length   = 3
  };
#endif

#ifndef PRODUCT
  static void iterate_oopmaps(oopmaps_doer do_map, void *param);
  void iterate(OopVisitor* visitor);
#endif


 protected:
  void generic_compile(address addr JVM_TRAPS);
  void generic_compile(address addr,
                       const Assembler::Register stack_ptr JVM_TRAPS);

  typedef void (CodeGenerator::*custom_compile_func)
    (CompilationQueueElement* element  JVM_TRAPS);
  void custom_compile(custom_compile_func producer JVM_TRAPS);

public:
  BinaryAssembler::Label entry_label( void ) const {
    BinaryAssembler::Label L; 
    L._encoding = _entry_label; 
    return L;
  }
  void set_entry_label( const BinaryAssembler::Label value ) { 
    _entry_label = value._encoding;
  }
  void clear_entry_label( void ) {
    _entry_label = 0;
  }

  BinaryAssembler::Label return_label( void ) const {
    BinaryAssembler::Label L;
    L._encoding = _return_label;
    return L;
  }
  void set_return_label( const BinaryAssembler::Label value ) {
    _return_label = value._encoding;
  } 
  void clear_return_label( void ) {
    _return_label = 0;
  }

 private:  
  static CompilationQueueElement* _pool;  // Pool of recycled elements
 public:
  static void reset_pool ( void ) { _pool = NULL; }

  // Recycled elements are kept live in the pool until
  // the end of current compilation or a GC
  inline void free( void );

  // CompilationQueueElement
  static CompilationQueueElement*
    allocate( const CompilationQueueElementType type, const int bci JVM_TRAPS );

  void insert( void );

  // Compile this CompilationQueueElement.
  // Returns true if the element finished compilation, false if it
  // has been suspended and needs to be resumed in the future.
  bool compile(JVM_SINGLE_ARG_TRAPS);

  // Field accessors.
  CompilationQueueElementType type( void ) const {
    return _type;
  }
  void set_type(const CompilationQueueElementType value) {
    _type = value;
  }

  VirtualStackFrame* frame ( void ) const { return _frame; }
  void set_frame( VirtualStackFrame* value ) { _frame = value; }

  jint bci( void ) const   { return _bci; }
  void set_bci( const jint value ) { _bci = value; }

  Assembler::Register register_0( void ) const {
    return _register_0;
  }
  void set_register_0( const Assembler::Register value ) {
    _register_0 = value;
  }

  Assembler::Register register_1( void ) const {
    return _register_1;
  }
  void set_register_1( const Assembler::Register value ) {
    _register_1 = value;
  }

  jint info( void ) const { return _info; }
  void set_info( const jint value ) { _info = value; }

  bool is_suspended( void ) const {
    return _is_suspended;
  }
  void set_is_suspended( const bool value ) {
    _is_suspended = CAST_TO_JBOOLEAN(value);
  }

  bool entry_has_been_bound( void ) const {
    return _entry_has_been_bound;
  }
  void set_entry_has_been_bound( const bool value ) {
    _entry_has_been_bound = CAST_TO_JBOOLEAN(value);
  }

  jint code_size_before( void ) const {
    return _code_size_before;
  }
  void set_code_size_before( const jint value ) {
    _code_size_before = value;
  }

  CompilationQueueElement* next( void ) const {
    return _next;
  }
  void set_next( CompilationQueueElement* value ) {
    _next = value;
  }
};

#define DEFINE_CAST( name, subtype )\
static name* cast( CompilationQueueElement* value ) {             \
  GUARANTEE(value->type() == subtype, "Type check");              \
  return (name*) value;                                           \
}                                                                 \
static const name* cast( const CompilationQueueElement* value ) { \
  GUARANTEE(value->type() == subtype, "Type check");              \
  return (const name*) value;                                     \
}


class CompilationContinuation: public CompilationQueueElement {
  void begin_compile(JVM_SINGLE_ARG_TRAPS);
  bool compile_bytecodes(JVM_SINGLE_ARG_TRAPS);
  void end_compile( void );

 public:
  static CompilationContinuation* insert( const jint bci,
                    const BinaryAssembler::Label entry_label JVM_TRAPS);

  static CompilationContinuation* insert( Compiler * const compiler,
                                          const jint bci,
                                          const BinaryAssembler::Label entry_label 
                                          JVM_TRAPS);

  enum { 
    cc_flag_need_osr_entry        = 1,
    cc_flag_is_exception_handler  = 2,
    cc_flag_run_immediately       = 4,
    cc_flag_forward_branch_target = 8
  };

  void set_need_osr_entry( void ) {
    set_info(info() | cc_flag_need_osr_entry);
  }
  bool need_osr_entry( void ) const {
    return (info() & cc_flag_need_osr_entry) != 0;
  }

  void set_is_exception_handler( void ) { 
    set_info(info() | cc_flag_is_exception_handler);
  }
  bool is_exception_handler( void ) const {
    return (info() & cc_flag_is_exception_handler) != 0;
  }

  void set_run_immediately( void ) {
    set_info(info() | cc_flag_run_immediately);
  }
  bool run_immediately( void ) const {
    return (info() & cc_flag_run_immediately) != 0;
  }

  void set_forward_branch_target( void ) { 
    set_info(info() | cc_flag_forward_branch_target); 
  }
  bool forward_branch_target( void ) const { 
    return (info() & cc_flag_forward_branch_target) != 0;
  }

  // Compile this continuation.
  // Returns true if the element finished compilation, false if it
  // has been suspended and needs to be resumed in the future.
  bool compile(JVM_SINGLE_ARG_TRAPS);
  DEFINE_CAST( CompilationContinuation, compilation_continuation )
};


class ThrowExceptionStub: public CompilationQueueElement {
 public:
  // Runtime exceptions that can be thrown by compiled code
  enum RuntimeException { 
    rte_null_pointer                 = 0,
    rte_array_index_out_of_bounds    = 1,
    rte_illegal_monitor_state        = 2,
    rte_division_by_zero             = 3,
    rte_incompatible_class_change    = 4,
    number_of_runtime_exceptions
  };
  static address exception_allocator(int rte);
  static ReturnOop exception_class(int rte);
private:
  // ^ThrowExceptionStub
  static ThrowExceptionStub* allocate(RuntimeException rte, int bci JVM_TRAPS);
protected:
  static ThrowExceptionStub* allocate_or_share(RuntimeException rte JVM_TRAPS);

private:
  address exception_thrower( void ) const;

  void set_rte(RuntimeException rte) {
    GUARANTEE(info() == 0, "Only set rte once");
    set_info((jint)rte);
  }
  RuntimeException get_rte( void ) const {
    return RuntimeException(info() & ~0x80000000);
  }

public:
  void set_is_persistent( void ) { set_info(info() | 0x80000000); }
  bool is_persistent    ( void ) const { return info() < 0; }

  // Throw an exception.
  void compile(JVM_SINGLE_ARG_TRAPS);

  DEFINE_CAST( ThrowExceptionStub, throw_exception_stub )

#if ENABLE_NPCE  
  friend class CodeGenerator;
#endif  
#if ENABLE_INTERNAL_CODE_OPTIMIZER || ENABLE_NPCE
  friend class Compiler;
#endif  
};

inline void CompilationQueueElement::free( void ) {
  if( !( type() == CompilationQueueElement::throw_exception_stub &&
         ThrowExceptionStub::cast(this)->is_persistent() ) ) {
    _next = _pool;
    _pool = this;
  }
}

#define BEGIN_THROW_EXCEPTION_STUB_SUBCLASS( name, type )\
class name: public ThrowExceptionStub {                   \
 public:                                                  \
  static name* allocate_or_share(JVM_SINGLE_ARG_TRAPS) {  \
    return (name*) ThrowExceptionStub::allocate_or_share( \
      type JVM_NO_CHECK_AT_BOTTOM);                       \
  }

BEGIN_THROW_EXCEPTION_STUB_SUBCLASS( UnlockExceptionStub,
  rte_illegal_monitor_state )
};

BEGIN_THROW_EXCEPTION_STUB_SUBCLASS( IndexCheckStub,
  rte_array_index_out_of_bounds )
};

BEGIN_THROW_EXCEPTION_STUB_SUBCLASS( NullCheckStub,
  rte_null_pointer )
#if ENABLE_NPCE
  //the stub related with a byte code which will emit two LDR instrs
  bool is_two_words( void ) const { return _return_label != 0; }

  //the offset from the first LDR to the second LDR in words
  jint offset_of_second_instr_in_words( void ) const {
    return _return_label;
  }

  //set the offset from the second LDR to the first LDR in words.
  //we mark the offset(offset from the first ldr/str) of 
  //second inst on the stub,
  //so we will generate another npce relation item 
  //for the second ldr/str.
  void set_is_two_instr( const int offset = 1 ) {
    _return_label = offset;
  }
#endif
};

BEGIN_THROW_EXCEPTION_STUB_SUBCLASS( ZeroDivisorCheckStub,
  rte_division_by_zero )
};

BEGIN_THROW_EXCEPTION_STUB_SUBCLASS( IncompatibleClassChangeStub,
  rte_incompatible_class_change )
};

class TypeCheckStub: public CompilationQueueElement {
 public:
  static TypeCheckStub* allocate( const jint bci,
                         const BinaryAssembler::Label entry_label,
                         const BinaryAssembler::Label return_label JVM_TRAPS) {
    TypeCheckStub* stub = (TypeCheckStub*)
      CompilationQueueElement::allocate( type_check_stub, bci JVM_NO_CHECK );
    if( stub ) {
      stub->set_entry_label ( entry_label  ); 
      stub->set_return_label( return_label );
    }
    return stub;
  }

  void compile(JVM_SINGLE_ARG_TRAPS);
  DEFINE_CAST( TypeCheckStub, type_check_stub )
};

class InstanceOfStub: public CompilationQueueElement {
 public:
  Assembler::Register result_register( void ) const {
    return register_0();
  }
  void set_result_register( const Assembler::Register value) {
    set_register_0(value);
  }

  int class_id( void ) const {
    return info();
  }
  void set_class_id( const int class_id ) {
    set_info( class_id );
  }

  static InstanceOfStub* allocate(const jint bci, const jint class_id,
           const BinaryAssembler::Label entry_label,
           const BinaryAssembler::Label return_label, 
           const BinaryAssembler::Register result_register JVM_TRAPS) {
    InstanceOfStub* stub = (InstanceOfStub*)
      CompilationQueueElement::allocate(instance_of_stub, bci JVM_NO_CHECK);
    if( stub ) {
      stub->set_entry_label( entry_label ); 
      stub->set_return_label( return_label );
      stub->set_result_register( result_register );
      stub->set_class_id( class_id );
    }
    return stub;
  }

  void compile( JVM_SINGLE_ARG_TRAPS );
  DEFINE_CAST( InstanceOfStub, instance_of_stub )
};

class CheckCastStub: public CompilationQueueElement {
 public:
  int class_id( void ) const {
    return info();
  }
  void set_class_id( const int class_id ) {
    set_info( class_id );
  }

  static void insert( const int bci, const int class_id,
                      const BinaryAssembler::Label entry_label,
                      const BinaryAssembler::Label return_label JVM_TRAPS);

  void compile( JVM_SINGLE_ARG_TRAPS );
  DEFINE_CAST( CheckCastStub, check_cast_stub )
};

class StackOverflowStub: public CompilationQueueElement {
 public:
  static StackOverflowStub* allocate( const BinaryAssembler::Label entry_label,
                            const BinaryAssembler::Label return_label,
                            const Assembler::Register stack_pointer,
                            const Assembler::Register method_pointer JVM_TRAPS) {
    StackOverflowStub* stub = (StackOverflowStub*)
      CompilationQueueElement::allocate(stack_overflow_stub, 0 JVM_NO_CHECK);
    if( stub ) {
      stub->set_entry_label(entry_label); 
      stub->set_return_label(return_label);
      stub->set_register_1(method_pointer);
      stub->set_register_0(stack_pointer);
    }
    return stub;
  }

  void compile(JVM_SINGLE_ARG_TRAPS);
  DEFINE_CAST( StackOverflowStub, stack_overflow_stub )
};

class TimerTickStub: public CompilationQueueElement {
 public:
  enum { invalid_code_offset   = -1 };

  static TimerTickStub* allocate( const jint bci,
                         const BinaryAssembler::Label entry_label,
                         const BinaryAssembler::Label return_label JVM_TRAPS) {
    return allocate(bci, invalid_code_offset, 
                    entry_label, return_label JVM_NO_CHECK_AT_BOTTOM);
  }

  static TimerTickStub* allocate( const jint bci, const jint code_offset,
                          const BinaryAssembler::Label entry_label,
                          const BinaryAssembler::Label return_label JVM_TRAPS) {
    TimerTickStub* stub = (TimerTickStub*)
        CompilationQueueElement::allocate(timer_tick_stub, bci JVM_NO_CHECK);
    if( stub ) {
      stub->set_info(code_offset);
      stub->set_entry_label(entry_label); 
      stub->set_return_label(return_label);
    }
    return stub;
  }

  void compile(JVM_SINGLE_ARG_TRAPS);
  DEFINE_CAST( TimerTickStub, timer_tick_stub )
};

class QuickCatchStub: public CompilationQueueElement {
 public:
  static QuickCatchStub* allocate(const jint bci,
                            const Value &value, const jint handler_bci,
                            const BinaryAssembler::Label entry_label JVM_TRAPS);

  void compile(JVM_SINGLE_ARG_TRAPS);
  DEFINE_CAST( QuickCatchStub, quick_catch_stub )
};

class OSRStub: public CompilationQueueElement {
  void emit_osr_entry_and_callinfo(CodeGenerator* gen JVM_TRAPS);

 public:
  static OSRStub* allocate( const jint bci,
                            const BinaryAssembler::Label entry_label JVM_TRAPS){
    OSRStub* stub = (OSRStub*)
      CompilationQueueElement::allocate(osr_stub, bci JVM_NO_CHECK);
    if( stub ) {
      stub->set_entry_label(entry_label);
    }
    return stub;
  }

  void compile(JVM_SINGLE_ARG_TRAPS);
  DEFINE_CAST( OSRStub, osr_stub )
};

#if ENABLE_INLINE_COMPILER_STUBS
class NewObjectStub: public CompilationQueueElement {
 public:
  Assembler::Register result_register( void ) const {
    return register_0();
  }
  void set_result_register( const Assembler::Register value ) {
    set_register_0( value );
  }

  Assembler::Register java_near( void ) const {
    return register_1();
  }
  void set_java_near( const Assembler::Register value ) {
    set_register_1(value);
  }

  static NewObjectStub* allocate( const jint bci,
                          const Assembler::Register obj,
                          const Assembler::Register jnear,
                          const BinaryAssembler::Label entry_label,
                          const BinaryAssembler::Label return_label JVM_TRAPS) {
    NewObjectStub* stub = (NewObjectStub*)
        CompilationQueueElement::allocate(new_object_stub, bci JVM_NO_CHECK);
    if( stub ) {
      stub->set_result_register(obj);
      stub->set_java_near(jnear);
      stub->set_entry_label(entry_label); 
      stub->set_return_label(return_label);
    }
    return stub;
  }

  void compile(JVM_SINGLE_ARG_TRAPS);
  DEFINE_CAST( NewObjectStub, new_object_stub )
};

class NewTypeArrayStub: public CompilationQueueElement {
 public:
  Assembler::Register result_register( void ) const {
    return register_0();
  }
  void set_result_register(const Assembler::Register value ) {
    set_register_0( value );
  }

  Assembler::Register java_near( void ) const {
    return register_1();
  }
  void set_java_near(Assembler::Register value) {
    set_register_1(value);
  }

  Assembler::Register length( void ) const {
    return Assembler::Register( info() );
  }
  void set_length( const Assembler::Register value ) {
    set_info( int(value) );
  }

  static NewTypeArrayStub* allocate(const jint bci,
                            const Assembler::Register obj,
                            const Assembler::Register jnear,
                            const Assembler::Register length,
                            const BinaryAssembler::Label entry_label,
                            const BinaryAssembler::Label return_label JVM_TRAPS) {
    NewTypeArrayStub* stub = (NewTypeArrayStub*)
        CompilationQueueElement::allocate(new_type_array_stub, bci JVM_NO_CHECK);
    if( stub ) {
      stub->set_result_register(obj);
      stub->set_java_near(jnear);
      stub->set_length(length);
      stub->set_entry_label(entry_label); 
      stub->set_return_label(return_label);
    }
    return stub;
  }

  void compile(JVM_SINGLE_ARG_TRAPS);
  DEFINE_CAST( NewTypeArrayStub, new_type_array_stub )
};
#endif // ENABLE_INLINE_COMPILER_STUBS

#if ENABLE_INTERNAL_CODE_OPTIMIZER
class EntryStub: public CompilationQueueElement  {
 public:
  static EntryStub* allocate( const jint bci, 
                      const BinaryAssembler::Label entry_label JVM_TRAPS) {
    EntryStub* stub = (EntryStub*)
          CompilationQueueElement::allocate(entry_stub, bci JVM_NO_CHECK);
    if( stub ) {
      stub->set_entry_label( entry_label );
    }
    return stub;
  }
};
#endif

#undef DEFINE_CAST
#undef BEGIN_THROW_EXCEPTION_STUB_SUBCLASS

#endif /* ENABLE_COMPILER */
