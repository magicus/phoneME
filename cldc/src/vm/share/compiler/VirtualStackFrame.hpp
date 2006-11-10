/*
 *
 * Portions Copyright  2003-2006 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * This structure describes the internal layout of one location inside
 * the VirtualStackFrame. It's good for quick access of the VirtualStackFrame,
 * but it works only when GC doesn't occur. For the general case, where
 * GC may occur, use the Location class instead (see Location.hpp)
 */
class RawLocationData {
protected:
  jubyte _type;
  jubyte _where;
  jubyte _status;
  jubyte _flags;
  juint  _value;
  juint  _length;
#ifndef PRODUCT
public:
  void print_on(Stream *);
  void p();
#endif

};

class RawLocation;

class VirtualStackFrame: public MixedOop {
 private:
  static Method* method() {
    return jvm_fast_globals.compiler_method;
  }
  static CodeGenerator* code_generator() {
    return jvm_fast_globals.compiler_code_generator;
  }

  static int num_stack_lock_words() {
     return jvm_fast_globals.num_stack_lock_words;
  }

 public:
  HANDLE_DEFINITION(VirtualStackFrame, MixedOop);

#if USE_COMPILER_LITERALS_MAP
  enum {
    literals_map_size = Assembler::number_of_registers
  };
#else
  enum {
    literals_map_size = 0
  };
#endif

#if ENABLE_CSE
  friend class BytecodeCompileClosure;

  
  static jint _cse_tag ;
  static jint _pop_bci;
  static jint _passable_osr;
  static bool _clean;
 private:
  void   tag_push();
  void   tag_pop();
  void     decode_tag(jint tag);
 public:
  enum osr_const {
    passable = 1,
    unpassable = 0
  };
  static inline void  set_osr_passable(void) {
    _passable_osr = passable;
  }
  static inline void set_osr_unpassable(void) {
    _passable_osr = unpassable;
  }
  static inline bool osr_passable(void) {
    return _passable_osr != unpassable;
  }
  static inline void set_unkilled_regs(jint regs) {
    _passable_osr = regs;
  }
  static inline jint osr_unkilled_regs(void) {
    return _passable_osr;
  }
  static inline  void clear_cse_status( void ) {
    _pop_bci = -1;
    _cse_tag = 0;
    _clean = true;
  }
  static inline bool is_status_cleaned(void) {
    return _clean;
  }
  static inline void reset_clean_status(void) {
    _clean = false;
  }
  enum tag_const {
    low_bit = 16,
    low_mask = 0xffff,
    high_bit = 16,
    high_mask = (0xffff<<16)
  };

  static inline jint get_first_bci(void) {
    jint first_bci = 0;
    first_bci = (_cse_tag & low_mask) - 1;
    if ( (_cse_tag & high_mask) != 0 ) {
      first_bci  = (_cse_tag >> low_bit) - 1;
    }
    return first_bci;
  }
  static inline bool is_popped(void) {
    return  _pop_bci != -1;
  }
  static inline jint assemble_tag(jint current_bci) {
    //bci encoded in tag start from 1. 
    jint tag = current_bci + 1 ;
    if ( _pop_bci  == current_bci) {
      //the pop item is eat by this bci
      //we should track its _cse_tag.
      if ( (_cse_tag & high_mask) == 0) {
        tag  |= (( (_cse_tag) & low_mask) << low_bit); 
      } else {
        tag  |= ( (_cse_tag) & high_mask);
      }
    } 
    return tag;
  }
#endif
  // Allocate a new instance of VirtualStackFrame.
  static ReturnOop allocate(int location_map_size JVM_TRAPS);

  // Construct a new virtual stack frame for the given method.
  static ReturnOop create(Method* method JVM_TRAPS);

  // clone this virtual stack frame
  ReturnOop clone(JVM_SINGLE_ARG_TRAPS);

  // copy this virtual stack frame to dst
  void copy_to(VirtualStackFrame* dst) const;

  // clone this virtual stack frame, but adjust stack for exception
  ReturnOop clone_for_exception(int handler_bci JVM_TRAPS);

  // clear this virtual stack frame
  void clear();

  // clear a specific location
  void clear(int location);

  // adjust the virtual stack frame for an invoke.
  // - pop the parameter block and
  // - push the result
  void adjust_for_invoke(int parameter_size, BasicType return_type);

  // Flush all cached or changed locations to memory. After this operation
  // no locations will be mapped in registers anymore.
  void flush();

  // A platform-dependent way of flushing the stack quickly. Currently
  // we do this on ARM only by using post/pre-indexing addressing modes
  bool flush_quick();

  // flush all cached locations that are in the FPU
  void flush_fpu();

#if ENABLE_EMBEDDED_CALLINFO
  // flush all tags in this virtual stack frame
  ReturnOop generate_callinfo_stackmap(JVM_SINGLE_ARG_TRAPS);
#endif

  void mark_as_flushed();

  // write all changed locations to memory
  void write_changes();
  
  // update all cached locations by reading from memory
  void update_caches();
  
  // Spill a specific register into all locations it maps. After this
  // operation, all locations that previously mapped to this register will
  // not map to any register.
  void spill_register(Assembler::Register reg);

  // For all locations that uses this register, make sure that any changes
  // in the register are committed to memory.
  void commit_changes(Assembler::Register reg);

  // Make sure a frame doesn't use a register, either by spilling it or by
  // moving it to a free register.
  void unuse_register(Assembler::Register reg);

  // Check if this frame is conformant to the other frame, so you can
  // branch from this frame to the other frame w/o merging.
  bool is_conformant_to(VirtualStackFrame* other);

  // generate code for merging the states of the two virtual stack frames
  void conform_to(VirtualStackFrame* other) {
    verify_conform_to(other);
    conform_to_impl(other);
  }

  void verify_conform_to(VirtualStackFrame* /*other*/) PRODUCT_RETURN;

#ifndef PRODUCT
  // IMPL_NOTE: temporary workaround for the cases when optimized VSF merge 
  // cannot be applied. Need to revisit these cases.
  bool can_do_optimized_merge(VirtualStackFrame* other);
#endif

  void conform_to_impl(VirtualStackFrame* other) {
#ifndef PRODUCT
    if (UseVSFMergeOptimization && can_do_optimized_merge(other)) {
      conform_to_optimized_impl(other);
    } else {
#else
    {
#endif
      conform_to_reference_impl(other);
    }
  }

  // make sure any other virtual stack frame can be made conformant with this
  // one 
  void conformance_entry(bool merging);

  // make sure a frame types conform to the stack map, and discard any
  // frame types that don't match
  void conform_to_stack_map(int bci);

  // returns if this virtual stack frame fits into the compiled compact
  // format (lazy tagging) 
  bool fits_compiled_compact_format() const;

#if ENABLE_EMBEDDED_CALLINFO
  void fill_in_tags(CallInfo& info, int parameters_size);
#endif

#if ENABLE_APPENDED_CALLINFO
  void fill_callinfo_record(CallInfoWriter* callinfo);
#endif

  // stack manipulation operations
  void swap();
  void pop();
  void pop2();
  void dup();
  void dup2();
  void dup_x1();
  void dup_x2();
  void dup2_x1();
  void dup2_x2();

  // type-specific stack manipulation operations
  void pop(BasicType kind);
  void pop(Value& result);
  void push(const Value& value);

  void value_at(Value& result, int location);
  void value_at(ExtendedValue& result, int location);
  void value_at_put(int location, const Value& value);

  // Set the (real) stack pointer to a specific value.
  void set_stack_pointer(jint location);

  // Dirtify a given register by invalidating all mappings of this register.
  void dirtify(Assembler::Register reg);

  bool is_mapping_something(const Assembler::Register reg) const;

#if ENABLE_REMEMBER_ARRAY_LENGTH && ARM
  bool is_allocated(const Assembler::Register reg) const;
#endif 
 public:
#if USE_COMPILER_FPU_MAP
  static jint fpu_register_map_offset() {
    return FIELD_OFFSET(VirtualStackFrameDesc, _fpu_register_map);
  }
#endif
  static jint real_stack_pointer_offset() {
    return FIELD_OFFSET(VirtualStackFrameDesc, _real_stack_pointer);
  }
  static jint virtual_stack_pointer_offset() {
    return FIELD_OFFSET(VirtualStackFrameDesc, _virtual_stack_pointer);
  }
  static jint saved_stack_pointer_offset() {
    return FIELD_OFFSET(VirtualStackFrameDesc, _saved_stack_pointer);
  }
  static jint flush_count_offset() {
    return FIELD_OFFSET(VirtualStackFrameDesc, _flush_count);
  }
  static jint literals_mask_offset() {
    return FIELD_OFFSET(VirtualStackFrameDesc, _literals_mask);
  }   
#if ENABLE_REMEMBER_ARRAY_LENGTH  
  static jint bound_mask_offset() {
    return FIELD_OFFSET(VirtualStackFrameDesc, _bound_mask);
  }    
#endif

  // Accessors for the virtual stack pointer variable.
  inline jint virtual_stack_pointer() const { 
    return int_field(virtual_stack_pointer_offset()); 
  }
  void set_virtual_stack_pointer(jint value) { 
    int_field_put(virtual_stack_pointer_offset(), value); 
  }
  void increment_virtual_stack_pointer() {
    set_virtual_stack_pointer(virtual_stack_pointer() + 1); 
  }
  void decrement_virtual_stack_pointer() { 
    set_virtual_stack_pointer(virtual_stack_pointer() - 1); 
  }  

  inline static jint header_size() {
    return VirtualStackFrameDesc::header_size();
  }
  inline static jint literals_map_base_offset() {
    return header_size();
  }
  address literals_map_base() const {
    return ((address)obj()) + literals_map_base_offset();
  }
  inline jint location_map_size() const {
    return object_size() - (header_size() + literals_map_size * sizeof(int));
  }
  inline static jint location_map_base_offset() {
    return literals_map_base_offset() + 
           (literals_map_size * sizeof(int));
  }
#if ENABLE_CSE  
  jint bci_stack_base_offset() {
    return location_map_base_offset() + 
            location_map_size() - 
            method()->max_execution_stack_count() *
            sizeof(jint);
  }
  jint *tag_at(int index)  {
    address base = ((address)obj()) +bci_stack_base_offset();
    return (jint*)( base + sizeof(jint) * index);
  }
#endif
  RawLocation *raw_location_at(int index) const {
    address base = ((address)obj()) + location_map_base_offset();
    return (RawLocation*)(base + index * sizeof(RawLocationData));
  }

  // Marks the exclusive end of the end of the stack. To iterate all locations:
  // for (raw_loc = raw_location_at(0); 
  //      raw < raw_location_end(raw_location_at(0));) {
  //     ....;
  //     raw_loc += is_two_word(raw_loc->type()) ? 2 : 1;
  // }
  RawLocation *raw_location_end(RawLocation *start) const {
    GUARANTEE(start == raw_location_at(0), "sanity");
    RawLocationData *p = (RawLocationData*) ((void*)start);
    return (RawLocation*) ((void*)(p + (virtual_stack_pointer() + 1)));
  }

  void clear_stack() { 
    set_virtual_stack_pointer(method()->max_locals() - 1);
  }

#if USE_COMPILER_FPU_MAP
  ReturnOop fpu_register_map() const { 
    return obj_field(fpu_register_map_offset());      
  }
  void set_fpu_register_map(TypeArray* value) {
    obj_field_put(fpu_register_map_offset(), (Oop*)value);
  }
#endif

  jint literals_mask() const { 
    return int_field(literals_mask_offset());      
  }
#if ENABLE_REMEMBER_ARRAY_LENGTH
  jint bound_mask() const { 
    return int_field(bound_mask_offset());      
  }
  void set_bound_mask(jint value) { 
    int_field_put(bound_mask_offset(), value);      
  }
#endif
  jint stack_pointer() const { 
    return int_field(real_stack_pointer_offset());
  }
  
  jint flush_count() const { 
    return int_field(flush_count_offset());          
  }
  void clear_flush_count() { 
    int_field_put(flush_count_offset(), 0);           
  }
  void set_flush_count(jint value) { 
    int_field_put(flush_count_offset(), value);      
  }
  void set_literals_mask(jint value) { 
    int_field_put(literals_mask_offset(), value);      
  }

  bool reveiver_must_be_nonnull(int size_of_parameters) const;
  void receiver(Value& value, int size_of_parameters) {
    value_at(value, virtual_stack_pointer() - size_of_parameters + 1);
  }

#if USE_COMPILER_LITERALS_MAP
  Assembler::Register get_literal(int imm32, LiteralAccessor& la);

  void set_has_literal_value(Assembler::Register reg, int imm32) {
    GUARANTEE((int)reg >= 0 && (int)reg < literals_map_size, "range");
    const int offset = literals_map_base_offset() + (reg * sizeof(int));
    int_field_put(offset, imm32);
    set_literals_mask(literals_mask() | (1 << reg));
  }  
  bool has_literal_value(Assembler::Register reg, int& imm32) {
    GUARANTEE((int)reg >= 0 && (int)reg < literals_map_size, "range");
    if (literals_mask() & (1 << reg)) {
      const int offset = literals_map_base_offset() + (reg * sizeof(int));
      imm32 = int_field(offset);
      return true;
    }
    return false;
  }
  void clear_literals() {
    set_literals_mask(0);
  }
#else
  void clear_literals() {}
#endif

#if ENABLE_REMEMBER_ARRAY_CHECK && \
     ENABLE_REMEMBER_ARRAY_LENGTH && ENABLE_NPCE
bool VirtualStackFrame::is_value_must_be_index_checked(
        Assembler::Register, Value &value);
void VirtualStackFrame::set_value_must_be_index_checked(
        Assembler::Register, Value &value);
#endif

#if ENABLE_REMEMBER_ARRAY_LENGTH && ARM
  void rebound(VirtualStackFrameDesc* osr_frame);
  Assembler::Register get_bound(
       Assembler::Register reg, bool first_time, Assembler::Condition cond);
  void set_bound_value(Assembler::Register bound_reg, 
                                       Assembler::Register reg); 
  bool has_bound_value(Assembler::Register) const;
  bool has_bound(void) const;
  Assembler::Register get_bound_register(void) const;
  Assembler::Register get_base_register (void) const;

  int get_bound_access_count(void) const;
  void add_bound_access_count();
  void clear_bound() { 
    set_bound_mask(0); 
  }
  bool set_is_not_first_time_access(Assembler::Register reg);
#endif
  
  void set_value_must_be_nonnull(Value &value);
  void set_value_has_known_min_length(Value &value, int length);

  // Debug dump the state of the virtual stack frame.
  void dump_fp_registers(bool /*as_comment*/) PRODUCT_RETURN;
  void dump(bool /*as_comment*/)              PRODUCT_RETURN;
  void print()                                PRODUCT_RETURN;

 private:
  void conform_to_reference_impl(VirtualStackFrame* other);

#ifndef PRODUCT
  void conform_to_optimized_impl(VirtualStackFrame* other);
#endif

  // VSF merge helper routines.

  // Prologue - misc stuff to be done before the merge:
  //  - flush float and double locations if using compiler FPU map;
  //  - clear compiler literals
  //  - adjust stack pointer
  //  - discard locations with type conflict
  //  - AZZERT_ONLY: verify that if a target location contains an immediate 
  //    value, the corresponding source location has the same value.
  void conform_to_prologue(VirtualStackFrame* other);
  // Epilogue - misc stuff to be done after the merge:
  //  - adjust stack pointer.
  void conform_to_epilogue(VirtualStackFrame* other);

#ifndef PRODUCT
  // First phase - process all required register->memory and immediate->memory 
  // transfers.
  // Returns true iff a second phase is needed (some location requires
  // register->register transfer).
  bool conform_to_phase_one(VirtualStackFrame* other);
  // Second phase - process all required register->register transfers.
  void conform_to_phase_two(VirtualStackFrame* other);
  // Third phase - process all required memory->register and immediate->register 
  // transfers.
  void conform_to_phase_three(VirtualStackFrame* other);
#endif

  // Get the number of locals.
  int locals() const { return method()->max_locals(); }

  // Get the total amount of locations.
  int locations() const { 
    return method()->max_execution_stack_count() - num_stack_lock_words(); 
  }

  // Check if a given location satisfies some criteria.
  bool is_local (int location) const {
    return location < locals();
  }

  bool is_mapped_by(RawLocation *raw_location, Assembler::Register reg) const;

  // Check if a given location is mapped by a given register.
  bool is_mapped_by(int index, Assembler::Register reg) const {
    return is_mapped_by(raw_location_at(index), reg);
  }

  // Accessors for the stack pointer variable.
  void set_real_stack_pointer(jint value) { 
      int_field_put(real_stack_pointer_offset(), value); 
  }

  // Returns the type of a expression stack element
  BasicType expression_stack_type(int index);

  // Expression stack manipulation used for implementing all
  // dup and swap operations.
  void stack_2_1_to_1_2(void);
  void stack_1_to_1_1(void);
  void stack_2_1_to_2_1_2_1(void);
  void stack_2_1_to_1_2_1(void); 
  void stack_3_2_1_to_1_3_2_1(void); 
  void stack_3_2_1_to_2_1_3_2_1(void);
  void stack_4_3_2_1_to_2_1_4_3_2_1(void);

  friend class OSRStub;
};

// This class is used to save the virtual stack frame state before
// VM calls and to restore it before returning.
class PreserveVirtualStackFrameState: public StackObj {
 public:
  PreserveVirtualStackFrameState(VirtualStackFrame* vsf JVM_TRAPS) : _frame(vsf){ 
     save(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);    
  }
  ~PreserveVirtualStackFrameState() { 
     restore(); 
  }

  void save(JVM_SINGLE_ARG_TRAPS);
  void restore();

 private:
  VirtualStackFrame*  frame() const { return _frame; }
  VirtualStackFrame*  saved_frame() { return &_saved_frame; }

  FastOopInStackObj       _must_precede_fast_oop;
  VirtualStackFrame*      _frame;
  VirtualStackFrame::Fast _saved_frame;
};

class LiteralElementStream {
public:
  LiteralElementStream(VirtualStackFrame* vsf) {
    _vsf = vsf;
    _index = -1;
    next();
  }
  void next();
  bool eos() { 
    return _index >= Assembler::number_of_registers;
  }
  void reset() {
    _index = -1; next();
  }
  Assembler::Register reg() {
    return (Assembler::Register)_index;
  }
  int value() {
    int offset = _vsf->literals_map_base_offset() + (_index * sizeof(jint));
    return _vsf->int_field(offset);
  }

private:
  VirtualStackFrame* _vsf;
  int                _index;
};

#ifdef PRODUCT
#define REGISTER_REFERENCES_CHECKER
#else
#define REGISTER_REFERENCES_CHECKER \
  RegisterReferenceChecker __register_reference_checker

class RegisterReferenceChecker {
public:
  RegisterReferenceChecker() {
    for (int i = 0; i < Assembler::number_of_registers; i++) {
      Assembler::Register reg = (Assembler::Register)i;
      _register_references[i] = RegisterAllocator::references(reg);
    }
  }
  ~RegisterReferenceChecker() {
#ifdef AZZERT
    bool ok = true;
#endif
    for (int i = 0; i < Assembler::number_of_registers; i++) {
      Assembler::Register reg = (Assembler::Register)i;
      if (_register_references[i] != RegisterAllocator::references(reg)) {
#if ARM | defined(HITACHI_SH)
        const char* name = Disassembler::reg_name(reg);
#else
        const char* name = Assembler::name_for_long_register(reg);
#endif
        tty->print_cr("register_references[%s] = %d -> %d", name, 
                      _register_references[i],
                      RegisterAllocator::references(reg));
#ifdef AZZERT
        ok = false;
#endif
      }
    }
    GUARANTEE(ok, "Register references changed");
  }
private:
  int _register_references[Assembler::number_of_registers];
};

#endif

#endif /* ENABLE_COMPILER */
