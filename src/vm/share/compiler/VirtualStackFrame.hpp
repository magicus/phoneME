/*
 *   
 *
 * Portions Copyright  2000-2007 Sun Microsystems, Inc. All Rights
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
#if ENABLE_COMPILER_TYPE_INFO
  jushort _class_id;
  jushort _padding;
#endif
#ifndef PRODUCT
public:
  void print_on(Stream *);
  void p();
#endif

};

class RawLocation;

class VirtualStackFrame: public MixedOop {
 private:
  static Method* method();

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

  //some osr entry is loop entry also
  //so they are unpassable and all the cached notation
  //status should be cleaned.
  //the osr entry created by "if" is passable. 
  enum osr_const {
    passable = 1,
    unpassable = 0
  };

  //high bits stand for the begin index
  //low bits stand for the end index.
  enum tag_const {
    low_bit = 16,
    low_mask = 0xffff,
    high_bit = 16,
    high_mask = (0xffff<<16)
  };

  //tag popped from tag stack
  static jint _cse_tag ;

  //the bci triggest the latest popping from operation stack
  static jint _pop_bci;

  //whether the notations could pass through the 
  //osr entry.
  static jint _passable_entry;

  //whether the tracking of byte code snippet of each value 
  //in the operation stack should be aborted.
  static bool _abort;
 private:

  //push tag on tag stack.      
  //the tag will contain the leftest bci 
  //of current item.
  //example of current execution stack and tag
  //cur bci = 2:
  //|         1  |bci2 iconst 1 tag:(0x0,0x2)
  //|         2  |bci1 iconst 2 tag:(0x0,0x1)
  //cur bci = 3:
  //after execution of iadd
  //|        3   |bci3 iadd     tag:(0x1,0x3)
  //cur bci = 4
  //|   1    |bci 4 iconst 1   tag:(0x0, 0x4)
  //|   3    |bci 3 iadd        tag:(0x1,0x3)
  //cur bci = 5
  //after execution of iadd bci 5
  //|   4    |bci 5 iadd       tag:(0x1, 0x5)
  
  //high bits of tag2 is 0x1 (begin bci of result of iadd(bci=3) which is the bci of iconst2)
  //low bits of tag2 is 0x5
  void   push_tag();
  
  //pop tag from tag stack.
  //get the leftest bci of current result
  //value
  void   pop_tag();

  //get the bci information from a tag
  void   decode_tag(jint tag);

  //return the size of tag stack used in CSE.
  static jint size_of_tag_stack(Method* method);
 public:

  static inline void  mark_as_passable(void) {
    _passable_entry = passable;
  }
  
  static inline void mark_as_unpassable(void) {
    _passable_entry = unpassable;
  }
  
  static inline bool is_entry_passable(void) {
    return _passable_entry != unpassable;
  }

  //when the entry is passable, we also record the registers whose notation 
  //shouldn't be cleaned for multi-entry caused by OSR.
  static inline void record_remained_registers(jint bitmap_of_registers) {
    _passable_entry = bitmap_of_registers;
  }
  
  static inline jint remained_registers(void) {
    return _passable_entry;
  }

  //reset the status values for beginning the tracking of byte code snippet 
  //of each value in the operation stack.
  static inline void  init_status_of_current_snippet_tracking(void) {
    _pop_bci = -1;
    _cse_tag = 0;
    _abort = false;
  }

  //reset the status values for aborting from the tracking of byte code snippet 
  //of each value in the operation stack.
  //aborted status will be set if we want to abort from the generation of the
  //notation of a value in some cases, such as we come across a snippet like the begin_bci > end_bci(
  //caused by goto)
  //and so on.
  static inline  void abort_tracking_of_current_snippet( void ) {
    _pop_bci = -1;
    _cse_tag = 0;
    _abort = true;
  }

  //tell whether the current status is caused by aborting of tracking
  static inline bool is_aborted_from_tracking(void) {
    return _abort;
  }

  //reset the abort status, in case a aborted is set during the compilation of
  //last byte code.
  static inline void mark_as_not_aborted(void) {
    _abort = false;
  }

  //check whether the _pop_bci is updated due to popping action of 
  //operation stack
  static inline bool is_popped(void) {
    return  _pop_bci != -1;
  }

  // get the first bci of the byte code string 
  // from which we get the result.
  // since the value store in _cse_tag is start from 1,
  // we substract 1 here(bci start from zero).
  static inline jint first_bci_of_current_snippet(void) {
    jint first_bci = 0;
    first_bci = (_cse_tag & low_mask) - 1;
    if ( (_cse_tag & high_mask) != 0 ) {
      first_bci  = (_cse_tag >> low_bit) - 1;
    }
    return first_bci;
  }

  //creat a tag from the current bci and the popped bci.
  //The popped bci is extract from cse_tag. 
  //The popped bci will be the leftest bci of an byte code snippet.
  //For example, (1+2)+4, the popped bci of final result 7 should
  //be the bci of iconst 1.
  //if the current bci won't cause a pop operation, the tag is the current  bci
  
  static inline jint create_tag(jint current_bci) {
    //bci encoded in tag start from 1. 
    jint tag = current_bci + 1 ;
    if ( _pop_bci  == current_bci) {
      //the pop item is eat by this bci
      //we should track its _cse_tag.
      //for example: a iadd is compiled
      if ( (_cse_tag & high_mask) == 0) {
         //the pop item is atomic
         //we make the bci of that pop item as 
         //the begin bci of current tag
        tag  |= (( (_cse_tag) & low_mask) << low_bit); 
      } else {
        //the pop item is calculated from a expression
        //we use its begin bci as the begin bci of current tag
        tag  |= ( (_cse_tag) & high_mask);
      }
    } 
    return tag;
  }

  //the offset of bci stack base in the VirtualStackFrame
  jint bci_stack_base_offset() {
    return location_map_base_offset() + 
            location_map_size() - 
            method()->max_execution_stack_count() *
            sizeof(jint);
  }

  //the tag corresponding to the index of item on the operation stack 
  jint *tag_at(int index)  {
    address base = ((address)obj()) +bci_stack_base_offset();
    return (jint*)( base + sizeof(jint) * index);
  }

  //clean the register notations whose value should be cleaned due to multi-entry
  void wipe_notation_for_osr_entry();
#else  
 private:
  void   push_tag() {}
  void   pop_tag() {}
  static jint size_of_tag_stack(Method* method) {return 0;}
 public: 
  void wipe_notation_for_osr_entry() {}
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
  void flush(JVM_SINGLE_ARG_TRAPS);

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

#if ENABLE_ARM_VFP
  jint* literals_mask_addr() const {
    return ((VirtualStackFrameDesc*)obj())->_literals_mask;
  }
#endif

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
#endif  // USE_COMPILER_FPU_MAP
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

  bool reveiver_must_be_nonnull(int size_of_parameters) const;
  void receiver(Value& value, int size_of_parameters) {
    value_at(value, virtual_stack_pointer() - size_of_parameters + 1);
  }

#if USE_COMPILER_LITERALS_MAP
  Assembler::Register get_literal(int imm32, LiteralAccessor& la);

  void set_has_literal_value(Assembler::Register reg, const int imm32) {    
    GUARANTEE((int)reg >= 0 && (int)reg < literals_map_size, "range");
    set_literal(reg, imm32);
#if ENABLE_ARM_VFP
    jint* p = literals_mask_addr();
    if( reg >= BitsPerInt ) {
      p++;
      reg = Assembler::Register(reg - BitsPerInt);
    }
    const jint mask = 1 << reg;
    if(imm32 == 0) {
      *p |= mask;
    } else {
      *p &=~mask;
    }
#endif
  }
  
  jint has_literal(Assembler::Register reg) const {
#if ENABLE_ARM_VFP
    jint imm32 = get_literal(reg);
    if(imm32 == 0) {
      const jint* p = literals_mask_addr();
      if( reg >= BitsPerInt ) {
        p++;
        reg = Assembler::Register(reg - BitsPerInt);
      }
      imm32 = (*p >> reg) & 1;
    }
    return imm32;
#else    
    return get_literal(reg);
#endif    
  }
  
  void clear_literals(void);
  
  bool has_no_literals(void) const;
  
  jint get_literal(const Assembler::Register reg) const {
    const int offset = literals_map_base_offset() + (reg * sizeof(jint));
    return int_field(offset);    
  }
  
  void set_literal(const Assembler::Register reg, const jint imm32) {
    const int offset = literals_map_base_offset() + (reg * sizeof(jint));
    int_field_put(offset, imm32);    
  }
  
  void clear_literal(Assembler::Register reg) {
    set_literal( reg, 0 );
#if ENABLE_ARM_VFP
    jint* p = literals_mask_addr();
    if( reg >= BitsPerInt ) {
      p++;
      reg = Assembler::Register(reg - BitsPerInt);
    }
    *p &=~ (1 << reg);
#endif    
  }
  
#if ENABLE_ARM_VFP
  Assembler::Register find_zero                 (void) const;
  Assembler::Register find_non_NaN              (void) const;
  Assembler::Register find_double_non_NaN       (void) const;
  Assembler::Register find_double_vfp_literal(const jint lo, const jint hi) const;
  
  static bool is_non_NaN(const jint imm32) {
    enum { NaN_mask = 0x7F800000 };
    return imm32 && (imm32 & NaN_mask) != NaN_mask;
  }
  
  bool result_register_contains(const jint imm32) const {
    const jint s0_value = get_literal(Assembler::s0);
    return imm32 == s0_value &&
      (s0_value || (literals_mask_addr()[0] & (1 << Assembler::s0)));
  }
  
  bool result_register_contains(const jint lo, const jint hi) const {    
    const jint mask = ~literals_mask_addr()[0];
    {
      const jint s0_value = get_literal(Assembler::s0);
      if (lo != s0_value) {
        return false;
      }
      if (s0_value == 0 && (mask & (1 << Assembler::s0)) ) {
        return false;
      }
    }
    {
      const jint s1_value = get_literal(Assembler::s1);
      if (hi != s1_value) {
        return false;
      }
      if (s1_value == 0 && (mask & (1 << Assembler::s1)) ) {
        return false;
      }
    }
    return true;
  }

#endif    
  
#else // !USE_COMPILER_FPU_MAP
  void clear_literal(const Assembler::Register) {}
  void clear_literals(void) {}
  bool has_no_literals(void) const { return true; }
#endif


#if ENABLE_REMEMBER_ARRAY_LENGTH
  enum BoundMaskElement {
    index_bits = 0xfff, // store the location index of a array bound checked variable
    max_value_of_index = 0xfff,
    bound_bits = (0xf <<12), // store the array length
    bound_shift = 12,
    base_bits = (0xf << 16), // store the array base 
    base_shift = 16,
    flag_bits = (0xf<<28), // store the array access count
    flag_shift = 28,
    max_value_of_count = 0xf,
  };

  //return the bound_mask bitmap
  jint bound_mask() const { 
    return int_field(bound_mask_offset());      
  }

  //set the bound_mask bitmap
  void set_bound_mask(jint value) { 
    int_field_put(bound_mask_offset(), value);      
  }

  //get the register which hold the array base address from
  //bound_mask
  Assembler::Register base_register (void) const;

  //get the flag of bound_mask
  int  bound_flag(void) const;

  //get the register which hold the length of the array whose 
  //address is cached in the base_register();
  Assembler::Register length_register(void) const;

  //index of local variable which did boundary check against 
  //the value of length_register()
  jint bound_index(void) const ;

  //try to cache the array length into the bound mask bitmap
  //the array base address is passwd by base_reg
  Assembler::Register cached_array_length(
       Assembler::Register base_reg, bool first_time, 
       Assembler::Condition cond);

  //check whether the array whose base register is hold in "reg" is 
  //cached by boundary mask bitmap
  bool is_cached_array_bound_of(Assembler::Register reg) const;

  //whether there's array cached in the bound mask
  bool is_bound_mask_valid(void) const;

  //clear the bound mask bitmap of current VSF
  void clear_bound();

  //free the registers allocted for caching of array boundary
  Assembler::Register free_length_register();
  
#if  ENABLE_REMEMBER_ARRAY_CHECK && ENABLE_NPCE  
  //clear the must_be_index_checked status of all the values in current VSF
  void clear_must_be_index_checked_status_of_values(void);

  //whether the length is cached in bound mask and 
  //value is mask as must_be_index_checked
  bool is_value_must_be_index_checked(
        Assembler::Register length, Value &value);

  //mark the value as index_checked, 
  //the "length" must have been cached in 
  //bound mask
  void set_value_must_be_index_checked(
        Assembler::Register length, Value &value);

  //whether need to skip the array boundary check.
  //And set must_be_index_checked if the local value is not checked before.
  bool try_to_set_must_be_index_checked(Assembler::Register length, Value& value);
#else
  void clear_must_be_index_checked_status_of_values(void){}
  bool is_value_must_be_index_checked(
        Assembler::Register, Value &value) {return false;}
  void set_value_must_be_index_checked(
        Assembler::Register, Value &value) {}
  bool try_to_set_must_be_index_checked(Assembler::Register length, Value& value) {return false;}
#endif

  //set the flag bound mask.
  void set_boundary_flag(void);

  //try to recache the same array base and length into the bound mask 
  //with the same base_reg and bound_reg
  void recache_array_length(Assembler::Register base_reg, 
          Assembler::Register bound_reg);

  //cache the index of local variable into the bound mask.
  //the value should have done boundary check against the array 
  //length cached in the bound mask
  void set_boundary_index(jint index);

  //update the base register and length register of
  //the bound mask with the value of parameters
  void set_boundary_value(Assembler::Register bound_reg, 
                                       Assembler::Register base_reg); 

  //mark the array holding by a local variable isn't first time accessed.
  //so the compiler can omit  the loading of array length in next time.
  //We won't cache non local variable.
  bool set_is_not_first_time_access(Assembler::Register base_reg);

  //during register allocator, try to free the register used for boundary caching
  //if exists.
  Assembler::Register try_to_free_length_register();

  //check is there any location occupied that register.
  //we must make sure the base register is allocated in the frame
  //before we do the preload of array length
  bool is_allocated(const Assembler::Register base_reg) const;
  
  static jint bound_mask_offset() {
    return FIELD_OFFSET(VirtualStackFrameDesc, _bound_mask);
  }

#else
  void clear_must_be_index_checked_status_of_values(void) {}
  bool is_value_must_be_index_checked(
        Assembler::Register, Value &value) {return false;}
  void set_value_must_be_index_checked(
        Assembler::Register, Value &value) {}
  bool try_to_set_must_be_index_checked(Assembler::Register length, Value& value) {return false;}
  jint bound_mask() const { return 0;}
  Assembler::Register base_register (void) const {return Assembler::no_reg;}
  int  bound_flag(void) const {return 0;}
  Assembler::Register length_register(void) const {return Assembler::no_reg;}
  void clear_bound() {}
  bool is_bound_mask_valid(void) const {return false;}
  void recache_array_length(Assembler::Register base_reg, 
          Assembler::Register bound_reg) {}; 
  void set_bound_mask(jint value) {} 
  Assembler::Register try_to_free_length_register() {return Assembler::no_reg;}
#endif
  
  void set_value_must_be_nonnull(Value &value);
  void set_value_has_known_min_length(Value &value, int length);
#if ENABLE_COMPILER_TYPE_INFO
  void set_value_class(Value & value, JavaClass * java_class);
#endif

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

class VirtualStackFrameContext: public StackObj {
 private:
  FastOopInStackObj       _must_precede_fast_oop;
  VirtualStackFrame::Fast _saved_frame;
 
 public:
  VirtualStackFrameContext(VirtualStackFrame* context);
  ~VirtualStackFrameContext();
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
