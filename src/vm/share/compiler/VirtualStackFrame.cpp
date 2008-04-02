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

#include "incls/_precompiled.incl"
#include "incls/_VirtualStackFrame.cpp.incl"

#if ENABLE_COMPILER

int VirtualStackFrame::_location_map_size;

Method* VirtualStackFrame::method( void ) {
  return Compiler::root()->method();
}

#if ENABLE_CSE

void VirtualStackFrame::push_tag() {
  // IMPL_NOTE: need to revisit for inlining
  if (!Compiler::is_inlining()) {
    jint bci_stack_pointer = virtual_stack_pointer() -
      method()->max_locals();
    if ( bci_stack_pointer >= 0 ) {
      //add 1 since bci start from zero.
      jshort bci = (jshort) code_generator()->bci();
      jint tag = create_tag(bci);
      jint* top_tag = tag_at(bci_stack_pointer);
      *top_tag = tag;
    }
  }
}

//for example, iadd will pop twice, so the last pop will
//get the begin index of current expression
void  VirtualStackFrame::pop_tag() {
  // IMPL_NOTE: need to revisit for inlining
  if (!Compiler::is_inlining()) {
    jint bci_stack_pointer = virtual_stack_pointer() -
      method()->max_locals();
    if ( bci_stack_pointer >= 0 ) {

      BasicType type = expression_stack_type(0);

      if (type == T_DOUBLE || type == T_LONG) {
        bci_stack_pointer --;
      }

      jint* top_tag = tag_at(bci_stack_pointer);

      jint  tag = (*top_tag);

      //update VFS tag status
      decode_tag(tag);
    }
  }
}

void  VirtualStackFrame::decode_tag(jint tag) {
    _cse_tag = tag;
    _pop_bci = (jshort) code_generator()->bci();
}

jint VirtualStackFrame::size_of_tag_stack(Method* method) {
  return method->max_execution_stack_count()*sizeof(jint);
}

void VirtualStackFrame::wipe_notation_for_osr_entry() {
  VERBOSE_CSE(("clear notation for osr entry bci =%d",
       Compiler::current()->bci()));
  VERBOSE_CSE(("osr is  %s",
          VirtualStackFrame::is_entry_passable()?"passable":"un-passable" ));

  VirtualStackFrame::abort_tracking_of_current_snippet();
  if (VirtualStackFrame::is_entry_passable()) {
    RegisterAllocator::wipe_all_notation_except(
      VirtualStackFrame::remained_registers());
  } else {
    RegisterAllocator::wipe_all_notations();
  }
  VirtualStackFrame::mark_as_unpassable();
}
#define ABORT_CSE_TRACKING VirtualStackFrame::abort_tracking_of_current_snippet();\
      RegisterAllocator::wipe_all_notations();
#else
#define ABORT_CSE_TRACKING
#endif


bool VirtualStackFrame::is_mapped_by(const RawLocation* item,
                                     const Assembler::Register reg) const {
  if (!item->is_flushed() && item->in_register()) {
    if (item->value() == (jint)reg) {
      return true;
    }
    const BasicType type = item->type();
    if (is_two_word(type)) {
      item ++; // Advance to next stack item
      if (item->value() == (jint)reg) {
        return true;
      }
    }
  }
  return false;
}

bool
VirtualStackFrame::is_mapping_something(const Assembler::Register reg) const {
  const RawLocation* raw_location = raw_location_at(0);
  const RawLocation* end  = raw_location_end(raw_location);

  while (raw_location < end) {
    if (!raw_location->is_flushed() && raw_location->in_register()) {
      if (raw_location->value() == (jint)reg) {
        return true;
      }
      if (raw_location->is_two_word()) {
        raw_location ++;
        if (raw_location->value() == (jint)reg) {
          return true;
        }
      }
    } else {
      if (raw_location->is_two_word()) {
        raw_location ++;
      }
    }
    raw_location ++;
  }

#if USE_COMPILER_LITERALS_MAP
  if (has_literal(reg)) {
    return true;
  }
#endif

  //remember array length
  //if the the register is allocated for boundary caching
  //we return as the register is allocated.
  //It will be allocated later by try_to_free_length_register()
  //if there's no other free registers.
  if (bound_mask() != 0) {
    if (length_register() == reg) {
        //allocate_or_fail will allocate it later
        //if there's no other free register
        //available.
        return true;
    }
  }

  return false;
}

bool VirtualStackFrame::fits_compiled_compact_format( void ) const {
  const RawLocation *raw_location = raw_location_at(0);
  const RawLocation *end  = raw_location_end(raw_location);

  for( ; raw_location < end; raw_location++ ) {
    const BasicType t = raw_location->type();
    switch( t ) {
      case T_ILLEGAL:
      case T_OBJECT :
      case T_INT    : break;
      default: return false;
    }
    // Only one-word items can fit in compiled compact format
    GUARANTEE(!is_two_word(t), "sanity");
  }

  return true;
}

#if ENABLE_EMBEDDED_CALLINFO
void VirtualStackFrame::fill_in_tags(CallInfo& info, int parameters_size) {
  AllocationDisabler allocation_not_allowed;

  RawLocation *raw_location = raw_location_at(0);
  RawLocation *end  = raw_location_end(raw_location);
  int index = 0;

  while (raw_location < end) {
    BasicType stack_type = raw_location->stack_type();

    if (stack_type == T_OBJECT) {
      int tag_index;
      if (virtual_stack_pointer() - index < parameters_size) {
        tag_index = virtual_stack_pointer() - index;
      } else {
        tag_index = (CallInfo::format1_tag_width - 1)
                     - (virtual_stack_pointer() - index - parameters_size);
      }
      info.set_oop_tag_at(tag_index);
    } else {
      GUARANTEE(stack_type == T_ILLEGAL || stack_type == T_INT,
                "frame should only contain uninits, ints and objects");
    }

    if (is_two_word(stack_type)) {
      index += 2;
      raw_location += 2;
    } else {
      index ++;
      raw_location += 1;
    }
  }
}

ReturnOop VirtualStackFrame::generate_callinfo_stackmap(JVM_SINGLE_ARG_TRAPS) {
  TypeArray::Raw result = CallInfo::new_tag_array(virtual_stack_pointer() + 1
                                             JVM_CHECK_0);
  AllocationDisabler allocation_not_allowed;
  RawLocation *raw_location = raw_location_at(0);
  RawLocation *end  = raw_location_end(raw_location);
  int index = 0;

  while (raw_location < end) {
    BasicType type = raw_location->type();
    CallInfo::set_tag(&result, index, type);

    if (is_two_word(type)) {
      index += 2;
      raw_location += 2;
    } else {
      index ++;
      raw_location += 1;
    }
  }

  return result;
}
#endif // ENABLE_EMBEDDED_CALLINFO

#if ENABLE_APPENDED_CALLINFO
void VirtualStackFrame::fill_callinfo_record(CallInfoWriter* writer) {
  AllocationDisabler allocation_not_allowed;

  RawLocation *raw_location = raw_location_at(0);
  RawLocation *end  = raw_location_end(raw_location);
  int index = 0;

  while (raw_location < end) {
    BasicType stack_type = raw_location->stack_type();

    if (stack_type == T_OBJECT) {
      writer->write_oop_tag_at(index);
    }

    if (is_two_word(stack_type)) {
      index += 2;
      raw_location += 2;
    } else {
      index ++;
      raw_location += 1;
    }
  }
}
#endif // ENABLE_APPENDED_CALLINFO

VirtualStackFrame* VirtualStackFrame::create(Method* method JVM_TRAPS) {
  // Allocate a new virtual stack frame with a location map.
  // Since all entries in the location map initially are zero this will
  // make all locations contain an uninitialized zero value.

  // NOTE: We have to allocate room for one more location to support pushing
  // parameters to the runtime system onto the Java stack.
  // For native methods, there must also be space on the "stack" for
  // a long return value (2 words).
  int inliner_stack_count = 2;
#if ENABLE_INLINE
  inliner_stack_count += 3;
#endif

  set_location_map_size( Location::size() *
    (inliner_stack_count + method->max_execution_stack_count() -
     num_stack_lock_words()) + size_of_tag_stack(method) );

  VirtualStackFrame* frame =
    VirtualStackFrame::allocate(JVM_SINGLE_ARG_ZCHECK_0( frame ) );

  // Setup the stack pointer and the virtual stack pointer.
  {
    const int real_stack_pointer = code_generator()->omit_stack_frame() ?
      method->size_of_parameters() : method->max_locals();
    frame->set_real_stack_pointer( real_stack_pointer - 1 );
  }
  frame->set_virtual_stack_pointer(method->max_locals() - 1);
  frame->clear_flush_count();

  AllocationDisabler allocation_not_allowed_in_this_block;
  {
    Signature::Raw signature = method->signature();
    for (SignatureStream ss(&signature, method->is_static());
            !ss.eos(); ss.next()) {
      const BasicType type = ss.type();
      const int index = ss.index();
      RawLocation* raw_location = frame->raw_location_at(index);
      Value value(type);
      if (index == 0 && !method->is_static()) {
        value.set_must_be_nonnull();
      }
      raw_location->write_value(value);
      raw_location->mark_as_flushed();
#if ENABLE_COMPILER_TYPE_INFO
      if (type == T_OBJECT || type == T_ARRAY) {
        JavaClass::Raw type_class = (index == 0 && !method->is_static()) ? 
          method->holder() : ss.type_klass();
        GUARANTEE(type_class.not_null(), "Sanity");
        raw_location->set_class_id(type_class().class_id());
        if (type_class().is_final_type()) {
          raw_location->set_is_exact_type();
        }
      }
#endif
    }

    for (int k = method->size_of_parameters(); k < frame->locations(); k++) {
      // initialize all locations that aren't parameters to illegal values
      RawLocation* raw_location = frame->raw_location_at(k);
      Value value(T_ILLEGAL);
      raw_location->write_value(value);
    }
  }

  return frame;
}

VirtualStackFrame* VirtualStackFrame::clone(JVM_SINGLE_ARG_TRAPS) { 
  VirtualStackFrame* result =
    VirtualStackFrame::allocate(JVM_SINGLE_ARG_NO_CHECK);
  if( result ) {
    copy_to( result );
  }
  return result;
}

void VirtualStackFrame::clear() {
  const int max_locals = method()->max_locals();
  // reset the stack pointers
  set_virtual_stack_pointer(max_locals - 1);
  set_real_stack_pointer(max_locals - 1);

  // update the CPU stack pointer
  code_generator()->clear_stack();

  // clear all locations
  for (int i = 0; i < max_locals; i++) {
    clear(i);
  }
}

VirtualStackFrame*
VirtualStackFrame::clone_for_exception(int handler_bci JVM_TRAPS) {
  int first_stack = method()->max_locals();
  VirtualStackFrame* result = clone(JVM_SINGLE_ARG_NO_CHECK);
  if( result ) {
    result->set_virtual_stack_pointer(first_stack);
    result->set_real_stack_pointer(first_stack);
    Location location(result, first_stack);
    Value value(T_OBJECT);
    location.write_value(value);
    result->mark_as_flushed();
    result->conform_to_stack_map(handler_bci);
  }
  return result;
}

void VirtualStackFrame::clear(int location) {
  const Value value(T_ILLEGAL);
  raw_location_at(location)->write_value(value);
}

void VirtualStackFrame::set_stack_pointer(int location) {
  if (location != stack_pointer()) {
    // adjust the stack pointer register
    code_generator()->increment_stack_pointer_by(location - stack_pointer());
    // adjust the stack pointer variable
    set_real_stack_pointer(location);
  }
}

void VirtualStackFrame::adjust_for_invoke(int parameter_size,
                                          BasicType return_type) {
  // Pop the parameter block
  set_virtual_stack_pointer(virtual_stack_pointer() - parameter_size);
  // Push the result
  if (return_type != T_VOID) {
    const bool double_word = is_two_word(return_type);
    // Adjust the virtual stack pointer.
    increment_virtual_stack_pointer();
    if (double_word) {
      increment_virtual_stack_pointer();
    }

    // Mark the location as flushed to memory (handled by callee)
    int index = virtual_stack_pointer() - (double_word ? 1 : 0);
    {
      AllocationDisabler allocation_not_allowed_in_this_block;
      RawLocation* raw_location = raw_location_at(index);
      Value r(return_type);
      raw_location->write_value(r);
      raw_location->mark_as_flushed();
    }
  }

  // Make sure virtual stack pointer matches the real stack pointer
  set_real_stack_pointer(virtual_stack_pointer());
}

#if !ARM
void VirtualStackFrame::dirtify(Assembler::Register reg) {
  for (VSFStream s(this); !s.eos(); s.next()) {
    if (is_mapped_by(s.index(), reg)) {
      Location location(this, s.index());
      location.mark_as_changed();
    }
  }
}
#endif

void VirtualStackFrame::flush(JVM_SINGLE_ARG_TRAPS) {
  bool quick_flush_possible = flush_quick();
  if (!quick_flush_possible) {
    if (stack_pointer() < virtual_stack_pointer()) {
      // This optimization prevent a lot of thrashing of the sp register
      set_stack_pointer(virtual_stack_pointer());
    }

    {
      AllocationDisabler allocation_not_allowed;
      RawLocation *raw_location = raw_location_at(0);
      RawLocation *end = raw_location_end(raw_location);
      int index = 0;

      while (raw_location < end) {
        raw_location->flush(index);
        if (is_two_word(raw_location->type())) {
          index += 2;
          raw_location += 2;
        } else {
          index ++;
          raw_location++;
        }
      }
    }
  }

  code_generator()->flush_epilogue(JVM_SINGLE_ARG_CHECK);

  // Mark all the literals as being junk
  clear_literals();

  //remember array length: clear cached value
  clear_bound();

  // adjust the stack pointer to reflect the virtual stack pointer.
  // (after the loop above, we might still have stack >= vstack.
  set_stack_pointer(virtual_stack_pointer());

#if USE_COMPILER_FPU_MAP && ENABLE_FLOAT
  GUARANTEE( fpu_register_map().is_clearable(),
             "mapped registers exist on FPU stack" );
  code_generator()->fpu_clear();
#endif

  set_flush_count(flush_count() + 1);
  ABORT_CSE_TRACKING;
}

void VirtualStackFrame::mark_as_flushed() {
  for (VSFStream s(this); !s.eos(); s.next()) {
    Location l(this, s.index());
    l.mark_as_flushed();
  }

  // Remove any literals we might have
  clear_literals();

  //remember array length: clear cached value
  clear_bound();

}

void VirtualStackFrame::conform_to_stack_map(int bci) {
  AllocationDisabler allocation_not_allowed_in_this_function;

  StackmapList::Raw method_stackmaps =
      StackmapGenerator::find_verifier_map(method());
  StackmapGenerator gen(method());
  TypeArray::Raw map = gen.generate_stackmap(&method_stackmaps, bci);

  RawLocation *raw_location = raw_location_at(0);
  RawLocation *end = raw_location_end(raw_location);
  int index = 0;

  while (raw_location < end) {
    GUARANTEE(index < gen.abstract_stack_top(), " Invalid stack height");
    BasicType type = raw_location->type();
    Tag expected_type = (Tag)map().ubyte_at(index);
    Tag current_type = StackmapGenerator::basictype_to_stackmaptag(type);

    if (current_type != expected_type) {
      if (expected_type == obj_tag) {
        GUARANTEE(index < method()->max_locals(), "Sanity??");
      }
      raw_location->mark_as_illegal();
    }

    if (is_two_word(type)) {
      index += 2;
      raw_location += 2;
    } else {
      index ++;
      raw_location++;
    }
  }
}

void VirtualStackFrame::conformance_entry(bool merging) {
  AllocationDisabler allocation_not_allowed_in_this_function;

  // IMPL_NOTE: need revisit why don't kill if inlining
  if (!Compiler::is_inlining()) {
    // Kill any stack elements whose type doesn't conform to the stack map
    conform_to_stack_map(code_generator()->bci());
  }
  // Make sure we haven't got any immediates, floats
  // or doubles mapped in the virtual stack frame.
  RawLocation *raw_location = raw_location_at(0);
  RawLocation *end  = raw_location_end(raw_location);
  int index = 0;
#if ENABLE_CSE
  //collecting preserve information for OSR entry
  bool check_register = false;
  jint register_bitmap = 0 ;
  if (VirtualStackFrame::is_entry_passable() &&
   !merging ) {
    check_register = true;

  }
#endif

  while (raw_location < end) {
    if (!raw_location->is_flushed()) {
#if USE_COMPILER_FPU_MAP
      BasicType type = raw_location->type();
      if (type == T_FLOAT || type == T_DOUBLE) {
        raw_location->flush(index);
      } else
#endif
      /* else */ if (merging && raw_location->is_immediate()) {
        if (RegisterAllocator::has_free(raw_location->is_two_word() ? 2 : 1)) {
          Value value(raw_location, index);
          value.materialize();
          raw_location->write_value(value);
        } else {
          raw_location->flush(index);
        }
      }
    }

#if ENABLE_CSE
    if (check_register &&
        raw_location->where() == Value::T_REGISTER) {
      jint reg = (jint) raw_location->get_register();
      register_bitmap |= (1<< reg);
    }
#endif
    if (merging) {
      raw_location->clear_flags();
#if ENABLE_COMPILER_TYPE_INFO
      raw_location->reset_class_id();
#endif
    }
    if (raw_location->is_two_word()) {
      raw_location += 2;
      index += 2;
    } else {
      raw_location ++;
      index ++;
    }
  }

#if ENABLE_CSE
  if (check_register) {
    //if those register are still in register
    //we won't clear theire notation later.
    VirtualStackFrame::record_remained_registers( register_bitmap);
  }
#endif

  // Mark all the literals as being junk
  clear_literals();

  if (merging) {
    //clear cached value
    clear_bound();
  }

#if USE_COMPILER_FPU_MAP && ENABLE_FLOAT
  GUARANTEE( fpu_register_map().is_clearable(),
             "mapped registers exist on FPU stack" );
  code_generator()->fpu_clear();
#endif //

  if (merging) {
    // Make sure no register maps two different locations.
    AllocationDisabler allocation_not_allowed_in_this_block;
    for (int j = 0; j < Assembler::number_of_registers; j++) {
      Assembler::Register reg = (Assembler::Register)j;
      bool used = false;

      RawLocation *raw_location = raw_location_at(0);
      RawLocation *end = raw_location_end(raw_location);
      int index = 0;

      while (raw_location < end) {
        if (is_mapped_by(raw_location, reg)) {
          if (!used) {
            used = true;
          } else {
            if (RegisterAllocator::has_free(1)) {
              Assembler::Register new_reg = RegisterAllocator::allocate();
              code_generator()->move(new_reg, reg);
              raw_location->change_register(new_reg, reg);
              RegisterAllocator::dereference(new_reg);
            } else {
              raw_location->flush(index);
            }
          }
        }
        if (is_two_word(raw_location->type())) {
          index += 2;
          raw_location += 2;
        } else {
          index ++;
          raw_location++;
        }
      }
    }
  }
}

#if USE_COMPILER_FPU_MAP || ENABLE_ARM_VFP

void VirtualStackFrame::flush_fpu() {
  for (VSFStream s(this); !s.eos(); s.next()) {
    Location location(this, s.index());
    if (!location.is_flushed()) {
      BasicType type = location.type();
      if (type == T_FLOAT || type == T_DOUBLE) {
        location.flush();
      }
    }
  }
}

#endif

inline void VirtualStackFrame::conform_to_prologue(VirtualStackFrame* other) {
  REGISTER_REFERENCES_CHECKER;
  GUARANTEE(virtual_stack_pointer() == other->virtual_stack_pointer(),
            "other must have same virtual stack pointer");

  // Make sure we haven't got any floats or doubles mapped in the source
  // virtual  stack frame.
  // The destination stack frame will not have floats or doubles
  // mapped either, see VirtualStackFrame::conformance_entry.
#if USE_COMPILER_FPU_MAP && ENABLE_FLOAT
  flush_fpu();

  GUARANTEE( fpu_register_map().is_clearable(),
             "mapped registers exist on FPU stack" );
  code_generator()->fpu_clear();
  GUARANTEE( other->fpu_register_map().is_empty(),
            "destination x86 FPU stack should be empty");
#endif

  clear_literals();

  //remember array length: clear cached value
  clear_bound();

  // This optimization prevents a lot of sp thrashing
  if (stack_pointer() < other->stack_pointer()) {
    set_stack_pointer(other->stack_pointer());
  }

  // In case of type conflict, we throw away the old value
  {
    AllocationDisabler allocation_not_allowed_in_this_block;
    RawLocation *src = raw_location_at(0);
    RawLocation *end = raw_location_end(src);
    RawLocation *dst = other->raw_location_at(0);

    while (src < end) {
      if (src->stack_type() != dst->stack_type()) {
        src->mark_as_illegal();
      }
      if (is_two_word(src->type())) {
        src++;
        dst++;
        src->set_type(T_ILLEGAL);
      }
      src++;
      dst++;
    }
  }

#ifdef AZZERT
  // Make sure that all literals in the target frame are correct
  // Note,  this must be a separate loop from the almost identical loop
  // below, since this might allocate registers in the frame().
  for (VSFStream ta(this); !ta.eos(); ta.next()) {
    Location src_loc(this, ta.index());
    Location dst_loc(other, ta.index());
    if (!dst_loc.is_flushed() && dst_loc.is_immediate()) {
      GUARANTEE(src_loc.is_flushed(), "Must be flushed");
      const Value dst( &dst_loc );
      code_generator()->verify_location_is_constant(ta.index(), dst);
    }
    if (src_loc.type() == T_ILLEGAL && dst_loc.is_two_word()) {
      ta.next();
    }
  }
#endif
}

inline void VirtualStackFrame::conform_to_epilogue(VirtualStackFrame* other) {
  // make sure the real stack pointer is set properly
  set_stack_pointer(other->stack_pointer());

  //re-load the preloaded array length
  //after frame conformance
  if (other->is_bound_mask_valid()) {
    recache_array_length(other->base_register(), other->length_register());
  }
}

// Helper macros to iterate over all locations in source and target frame.
// It is different from VSFStream in loop increment, also it is faster
// as operates on raw pointers. No allocations are allowed in the loop.

#define FOR_EACH_LOCATION_IN_FRAMES_DO(src_frame, dst_frame, src, dst, index) \
{                                                                             \
  AllocationDisabler allocation_not_allowed_in_this_block;                    \
  RawLocation *src = src_frame->raw_location_at(0);                           \
  RawLocation * const end = src_frame->raw_location_end(src);                 \
  RawLocation *dst = dst_frame->raw_location_at(0);                           \
  int index = 0;                                                              \
                                                                              \
  while (src < end) {

#define FOR_EACH_LOCATION_DONE(src, dst, index)             \
    if (is_two_word(src->type()) ||                         \
        (src->type() == T_ILLEGAL && dst->is_two_word())) { \
      src += 2;                                             \
      dst += 2;                                             \
      index += 2;                                           \
    } else {                                                \
      src++;                                                \
      dst++;                                                \
      index ++;                                             \
    }                                                       \
  }                                                         \
}

#define FOR_EACH_LOCATION_DO(src, dst, index) \
  FOR_EACH_LOCATION_IN_FRAMES_DO(this, other, src, dst, index)

bool VirtualStackFrame::is_conformant_to(VirtualStackFrame* other) {
  if (virtual_stack_pointer() != other->virtual_stack_pointer()) {
    return false;
  }

  if (GenerateCompilerComments && DumpVSFInComments) {
    dump(true);
    other->dump(true);
  }

  FOR_EACH_LOCATION_DO(src, dst, index)
  {
    if (src->is_flushed()) {
      if (!dst->is_flushed()) {
        return false;
      }
    } else if ((src->type()   != dst->type())   ||
               (src->where()  != dst->where())  ||
               (src->status() != dst->status()) ||
               (src->value()  != dst->value())) {
      return false;
    }
  }
  FOR_EACH_LOCATION_DONE(src, dst, index);

  return true;
}

void VirtualStackFrame::conform_to_reference_impl(VirtualStackFrame* other) {
  conform_to_prologue(other);

  FOR_EACH_LOCATION_DO(src, dst, index)
    if (src->has_register_conflict_with(index, other, dst, index)) {
      // in case of a register conflict we flush the location
      src->flush(index);
    }
  FOR_EACH_LOCATION_DONE(src, dst, index);

  // Need to handle immediate->memory transfers in a separate loop,
  // since this might allocate registers mapped in the frame.
  FOR_EACH_LOCATION_DO(src, dst, index)
    if (dst->is_flushed() && src->is_immediate()) {
      src->conform_to(index, dst, index);
    }
  FOR_EACH_LOCATION_DONE(src, dst, index);

  FOR_EACH_LOCATION_DO(src, dst, index)
    if (dst->is_flushed() || !dst->is_immediate()) {
      // make the individual source location conformant to the destination
      // location
      src->conform_to(index, dst, index);
    }
  FOR_EACH_LOCATION_DONE(src, dst, index);

  conform_to_epilogue(other);
}

#ifndef PRODUCT

// Perform LOC_STORE operations in a separate phase, since immediate->memory
// transfers require a spare register on some platforms and getting a spare
// register might require flushing of some locations. So this should be done
// before we attempt to process any register->register transfers at the second
// phase.
inline bool VirtualStackFrame::conform_to_phase_one(VirtualStackFrame* other) {
  REGISTER_REFERENCES_CHECKER;

  bool reg_reg_transfer_found = false;

  FOR_EACH_LOCATION_DO(src, dst, index)
    if (dst->is_flushed() || !dst->is_immediate()) {
      // Perform LOC_STORE if necessary.
      const RawLocation::Actions remaining_actions =
        src->do_conform_to(index, dst, index, RawLocation::LOC_STORE);
      // Check if this location needs register->register transfers to conform.
      if (!reg_reg_transfer_found && !src->is_immediate() &&
          (remaining_actions & RawLocation::REG_STORE)) {
        // NOTE: this location can be flushed later in this cycle, so
        // register_transfers_found flag set doesn't guarantee there are some
        // register transfers, but the flag cleared guarantees that there are
        // no register transfers, since flushing cannot introduce any
        // register transfers.
        reg_reg_transfer_found = true;
      }
    }
  FOR_EACH_LOCATION_DONE(src, dst, index);

  return reg_reg_transfer_found;
}

class TransferGraph : public StackObj {
#define FOR_ALL_NODE_PROPERTIES(template) \
   template(tree)                         \
   template(closed_cycle)                 \
   template(open_cycle)                   \
   template(leaf)                         \
   template(root)                         \
   template(visited)                      \
   template(changed)

 private:

//#define DEFINE_ENUM_ELEMENT(name) name,
//  enum {
//    FOR_ALL_NODE_PROPERTIES(DEFINE_ENUM_ELEMENT)
//  };
//#undef DEFINE_ENUM_ELEMENT

  //Changed to fix ADS compiler warning: "superfluous ',' in enum"
  enum {
    tree,
    closed_cycle,
    open_cycle,
    leaf,
    root,
    visited,
    changed
  };

//#define DEFINE_MARK_ENUM_ELEMENT(name) name ## _mark = 1 << name,
//  enum Mark {
//    zero_mark = 0,
//    FOR_ALL_NODE_PROPERTIES(DEFINE_MARK_ENUM_ELEMENT)
//  };
//#undef DEFINE_MARK_ENUM_ELEMENT

  //Changed to fix ADS compiler warning: "superfluous ',' in enum"
  enum Mark {
    zero_mark         = 0,
    tree_mark         = 1 << tree,
    closed_cycle_mark = 1 << closed_cycle,
    open_cycle_mark   = 1 << open_cycle,
    leaf_mark         = 1 << leaf,
    root_mark         = 1 << root,
    visited_mark      = 1 << visited,
    changed_mark      = 1 << changed
  };

  enum {
    NODE_MARK_MASK = tree_mark | closed_cycle_mark | open_cycle_mark
  };

  typedef Assembler::Register Register;
 public:
  TransferGraph(VirtualStackFrame* source, VirtualStackFrame* target) {
    // Fill with invalid initial values.
    jvm_memset(_src_for, -1, sizeof _src_for);
    jvm_memset(_pair_for, -1, sizeof _pair_for);
    jvm_memset(_flags, 0, sizeof _flags);

    _transfer_count = 0;

    initialize(source, target);
  }

  Register src_for(Register reg) const {
    check_register(reg);
    return _src_for[reg];
  }

  Register pair_for(Register reg) const {
    check_register(reg);
    return _pair_for[reg];
  }

#define DEFINE_GETTER(name)                      \
  bool is_ ## name(Register reg) const {         \
    check_register(reg);                         \
    return (_flags[reg] & name ## _mark) != 0;   \
  }

  FOR_ALL_NODE_PROPERTIES(DEFINE_GETTER)

#undef DEFINE_GETTER

  void markup();

  void shift_open_cycles();

  void process_closed_cycles();

  void process_trees_and_open_cycles();
 private:
  void initialize(VirtualStackFrame* source, VirtualStackFrame* target);

  void generate_transfer_operations(const Register src_reg,
                                    const Mark reg_mark);
  void generate_move(const Register dst_reg, const Register src_reg) const;

  Register get_spare_register();
  // Uninstall the link between src and dst and update the marks.
  // Both src and dst belong to a closed cycle or a loop.
  // In case of loop src == dst.
  // The marks should be updated, because after the link is removed, the
  // closed cycle becomes a tree, dst becomes the root, src becomes the leaf.
  void remove_link_from_closed_cycle(const Register src, const Register dst);
  Register get_leaf() const;

  void install_link(Register src, Register dst) {
    check_register(src);
    check_register(dst);
    {
      const Register old_src = src_for(dst);
      if (old_src != Assembler::no_reg) {
        check_register(old_src);
        RegisterAllocator::dereference(old_src);
      } else {
        RegisterAllocator::reference(dst);
        _transfer_count++;
      }
      RegisterAllocator::reference(src);
    }
    _src_for[dst] = src;
  }

  void uninstall_link(Register src, Register dst) {
    check_register(src);
    check_register(dst);
    GUARANTEE(src_for(dst) == src, "Sanity");

    RegisterAllocator::dereference(src);
    RegisterAllocator::dereference(dst);
    _transfer_count--;

    _src_for[dst] = Assembler::no_reg;
  }

  void record_pair(Register reg, Register pair) {
    check_register(reg);
    check_register(pair);
    _pair_for[reg] = pair;
    _pair_for[pair] = reg;
  }

  Mark mark(Register reg) const {
    return (Mark)(_flags[reg] & NODE_MARK_MASK);
  }

  void set_mark(Register reg, Mark mark) {
    GUARANTEE((mark & ~NODE_MARK_MASK) == 0, "Sanity");
    _flags[reg] &= ~NODE_MARK_MASK;
    _flags[reg] |= mark;
  }

#define DEFINE_SETTER(name)                                  \
  void set_ ## name(Register reg) {                          \
    check_register(reg);                                     \
    GUARANTEE((name ## _mark & NODE_MARK_MASK) == 0 ||       \
              mark(reg) == zero_mark, "Already marked");     \
    GUARANTEE((_flags[reg] & name ## _mark) == 0,            \
              "Already marked");                             \
    _flags[reg] |= name ## _mark;                            \
  }

  FOR_ALL_NODE_PROPERTIES(DEFINE_SETTER)

#undef DEFINE_SETTER

#define DEFINE_UNSETTER(name)                 \
  void unset_ ## name(Register reg) {         \
    check_register(reg);                      \
    _flags[reg] &= ~name ## _mark;            \
  }

  FOR_ALL_NODE_PROPERTIES(DEFINE_UNSETTER)

#undef DEFINE_UNSETTER

  void check_register(Register reg) const {
    GUARANTEE((int)reg < (int)Assembler::number_of_registers, "Sanity");
    AZZERT_ONLY_VAR(reg);
  }

  // Debugging support.
  void print_flags(Register reg) const PRODUCT_RETURN;

  Register _src_for[Assembler::number_of_registers];
  Register _pair_for[Assembler::number_of_registers];
  char     _flags[Assembler::number_of_registers];
  int      _transfer_count;

  Register first_allocatable_register() const {
    return Assembler::first_allocatable_register;
  }

  // Find the first Register r > reg where r is allocable.
  Register next_allocatable_register(Register reg) const;
};

#ifndef PRODUCT
// Debugging support.
void TransferGraph::print_flags(Register reg) const {
#if USE_DEBUG_PRINTING
#define PRINT_FLAG_NAME(name)             \
  if (is_ ## name(reg)) {                 \
    tty->print("%s ", #name);             \
  }

  FOR_ALL_NODE_PROPERTIES(PRINT_FLAG_NAME);

  tty->cr();
#endif
}
#endif

// The return value r = next_allocatable_register(reg) is different than
// s = RegisterAllocator::next_for(reg). With ENABLE_ARM_VFP=true, the
// allocatable registers are divided in two disjoint sets (the integer
// regs and the float regs). s and reg always belong to the same set, but
// r may belong to a different set than reg.
//
// The main purpose of this function is to iterate over all the
// allocatable registers without considering whether it float or int.
Assembler::Register
TransferGraph::next_allocatable_register(Assembler::Register reg) const {
#if defined(ARM)
  for (reg = (Register)(reg+1); int(reg)<int(Assembler::number_of_registers);
       reg = (Register)(reg+1)) {
    if (RegisterAllocator::is_allocatable(reg)) {
      return reg;
    }
  }
  return Assembler::first_allocatable_register;
#else
  // IMPL_NOTE: the above doesn't work for x86 case.
  return RegisterAllocator::next_for(reg);
#endif
}

Assembler::Register TransferGraph::get_leaf() const {
  Register reg = first_allocatable_register();

  do {
    if (is_leaf(reg)) {
      return reg;
    }
    reg = next_allocatable_register(reg);
  } while (reg != first_allocatable_register());

  return Assembler::no_reg;
}

inline void TransferGraph::initialize(VirtualStackFrame* source,
                                      VirtualStackFrame* target) {
  FOR_EACH_LOCATION_IN_FRAMES_DO(source, target, src, dst, index)
    if (!src->is_flushed() && src->in_register() &&
        !dst->is_flushed() && dst->in_register()) {
      GUARANTEE(src->is_cached() || src->is_changed(), "Sanity");
      GUARANTEE(dst->is_cached() || dst->is_changed(), "Sanity");
      // Get the type of transfers required to conform this location.
      const RawLocation::Actions actions =
        src->do_conform_to(index, dst, index, RawLocation::NO_ACTIONS);

      GUARANTEE((actions & RawLocation::LOC_STORE) == 0,
                "Must have been performed already");

      if (actions & RawLocation::REG_STORE) {
        // make sure the types match
        GUARANTEE(src->stack_type() == dst->stack_type(), "types must match");
        GUARANTEE(dst->in_register(), "Must be in register");

        Value src_value(src, index);
        Value dst_value(dst, index);

        const Assembler::Register lo_src_reg = src_value.lo_register();
        const Assembler::Register lo_dst_reg = dst_value.lo_register();

        // NOTE: we install a transfer link even if src_reg and dst_reg
        // are the same, introducing a loop in the graph. A loop indicates
        // that the current value must stay in this register after the
        // merge.
        GUARANTEE(src_for(lo_dst_reg) == Assembler::no_reg, "Sanity");

        install_link(lo_src_reg, lo_dst_reg);

        if (src->is_changed() && !is_changed(lo_src_reg)) {
          set_changed(lo_src_reg);
        }

        if (src_value.use_two_registers()) {
          const Assembler::Register hi_src_reg = src_value.hi_register();
          const Assembler::Register hi_dst_reg = dst_value.hi_register();

          // Record paired registers.
          {
            // NOTE: we record paired registers only if there is at least one
            // register->register transfer link for them. It is sufficient as
            // we need to know pairedness of a register only if there is
            // at least one register->register transfer link for it.
            GUARANTEE(lo_src_reg != hi_src_reg, "Sanity");
            GUARANTEE(lo_dst_reg != hi_dst_reg, "Sanity");
            GUARANTEE((pair_for(lo_src_reg) == Assembler::no_reg &&
                       pair_for(hi_src_reg) == Assembler::no_reg) ||
                      (pair_for(lo_src_reg) == hi_src_reg &&
                       pair_for(hi_src_reg) == lo_src_reg), "Sanity");

            record_pair(lo_src_reg, hi_src_reg);
          }

          GUARANTEE(src_for(hi_dst_reg) == Assembler::no_reg, "Sanity");

          install_link(hi_src_reg, hi_dst_reg);

          if (src->is_changed() && !is_changed(hi_src_reg)) {
            set_changed(hi_src_reg);
          }
        }
      }
    }
  FOR_EACH_LOCATION_DONE(src, dst, index);
}

inline void TransferGraph::markup() {
#ifdef GBA
  static
#endif
  Register dst_for[Assembler::number_of_registers];

  Register base_reg = first_allocatable_register();

  do {
    GUARANTEE(!is_visited(base_reg), "Sanity");

    if (mark(base_reg) == zero_mark) {
      AZZERT_ONLY(jvm_memset(dst_for, -1,
                  sizeof(Register) * Assembler::number_of_registers));

      Register reg = base_reg;
      // Mark for nodes visited in this iteration.
      Mark new_mark = zero_mark;

      set_leaf(base_reg);
      dst_for[base_reg] = Assembler::no_reg;

      do {
        set_visited(reg);
        const Register src_reg = src_for(reg);

        if (src_reg != Assembler::no_reg) {
          // We have an outgoing transfer reg->src_reg, so clear the leaf_mark.
          unset_leaf(src_reg);
        }

        if (src_reg == Assembler::no_reg || src_reg == reg) {
          // Root. Mark visited nodes as a tree.
          new_mark = tree_mark;
          set_root(reg);
        } else {
          // If src_reg has a closed_cycle_mark, reg belongs to a tree
          // attached to a cycle. So this is no longer a closed cycle,
          // but a open cycle.
          // To properly mark this cycle as a open cycle, we need to
          // go round it, so we clear closed_cycle_mark and proceed.
          if (is_closed_cycle(src_reg)) {
            unset_closed_cycle(src_reg);
            unset_root(src_reg);
          }

          const Mark src_mark = mark(src_reg);

          if (src_mark != zero_mark) {
            GUARANTEE(src_mark == open_cycle_mark ||
                      src_mark == tree_mark, "Sanity");
            // Found a tree attached to a open cycle or a tree.
            if (src_mark == open_cycle_mark) {
              // A tree attached to a open cycle root should be marked as
              // a tree, since we need to traverse it in a non-standard way.
              new_mark = is_root(src_reg) ? tree_mark : open_cycle_mark;
            } else {
              new_mark = tree_mark;
            }
          } else if (is_visited(src_reg)) {
            if (src_reg == base_reg) {
              // Found a cycle. Until we find a tree attached to it, we mark it
              // as a closed cycle.
              new_mark = closed_cycle_mark;
              set_root(base_reg);
            } else {
              // Found a cycle with an attached tree.
              // Mark it as a open cycle.
              // The root of a open cycle should be a direct descendant of
              // any node in the loop, that is not in the loop itself.
              // In this case it is exactly dst_for[src_reg].
              const Register root = dst_for[src_reg];
              GUARANTEE(root != Assembler::no_reg, "Sanity");
              GUARANTEE(src_for(root) == src_reg, "Sanity");
              GUARANTEE(is_visited(root), "Sanity");
              GUARANTEE(mark(root) == zero_mark, "Sanity");

              // At this point we mark the open cycle and mark the root.
              // We will shift it after all closed cycles are processed.
              // We have to do it this way, since shifting may remove
              // the only leaf register needed for processing of closed
              // cycles.
              set_root(root);

              new_mark = open_cycle_mark;
            }
          } else {
            dst_for[src_reg] = reg;
            reg = src_reg;
          }
        }
      } while (new_mark == zero_mark);

      do {
        unset_visited(reg);
        set_mark(reg, new_mark);
        // A tree attached to a open cycle root should be marked as a tree,
        // since we need to traverse it in a non-standard way.
        if (new_mark == open_cycle_mark && is_root(reg)) {
          new_mark = tree_mark;
        }
        reg = dst_for[reg];
      } while (reg != Assembler::no_reg);
    }

    base_reg = next_allocatable_register(base_reg);
  } while (base_reg != first_allocatable_register());
}

void TransferGraph::remove_link_from_closed_cycle(const Register src,
                                                    const Register dst) {
  GUARANTEE((is_closed_cycle(src) && is_closed_cycle(dst)) ||
            (is_tree(src) && is_root(dst) && src == dst),
            "Must be a loop or a closed cycle");
  GUARANTEE(src_for(dst) == src, "Sanity");

  uninstall_link(src, dst);

  // After this link is removed, this component becomes a tree,
  // so if this component was a closed cycle we need to mark it as a tree.
  if (src != dst) {
    Register tree_reg = src;
    do {
      unset_root(tree_reg);
      unset_closed_cycle(tree_reg);
      set_tree(tree_reg);
      tree_reg = src_for(tree_reg);
    } while (tree_reg != Assembler::no_reg);

    // The reg becomes a root, since it doesn't have ingoing transfers.
    set_root(dst);
  } else {
    GUARANTEE(is_tree(src) && is_root(src), "This was a loop");
  }

  // The spill_reg becomes a leaf, since it doesn't have any outgoing transfers.
  set_leaf(src);
}

inline Assembler::Register TransferGraph::get_spare_register() {
  Register spare_reg = Assembler::no_reg;

  if (RegisterAllocator::has_free(1)) {
    spare_reg = RegisterAllocator::allocate();
    RegisterAllocator::dereference(spare_reg);
  } else {
    spare_reg = get_leaf();
    if (spare_reg == Assembler::no_reg) {
      // We don't have any spare register, so we need to spill one.
      //
      // We can get here only if all components of the transfer graph are
      // closed cycles or loops (no attached trees), so
      // 1.Each register is mapped to exactly one location in the source frame.
      // 2.Whatever register we spill we will need exactly one additional load.
      //
      // Selection criteria is based on three facts:
      // 1.Spilling a register mapped to a changed location requires memory
      // store, while for a cached location it is a noop.
      // 2.Spilling of a paired register requires spilling its pair as well.
      //
      // So we choose a register the selection criteria is as follows:
      // 1.An unpaired register mapped to a cached location (weight 0).
      // 2.An unpaired register mapped to a changed location (weight 1).
      // 3.A paired register mapped to a cached location (weight 2).
      // 4.A paired register mapped to a changed location (weight 3).

      Assembler::Register reg = first_allocatable_register();
      Assembler::Register spill_reg = Assembler::no_reg;
      int spilling_weight = 4;

      do {
        GUARANTEE(is_closed_cycle(reg) || (is_tree(reg) && src_for(reg) == reg), "Sanity");

        int current_spilling_weight = 0;

        if (pair_for(reg) != Assembler::no_reg) {
          current_spilling_weight += 2;
        }

        if (current_spilling_weight < spilling_weight && is_changed(reg)) {
          current_spilling_weight++;
        }

        if (current_spilling_weight < spilling_weight) {
          spill_reg = reg;
          if (current_spilling_weight == 0) {
            break;
          }
          spilling_weight = current_spilling_weight;
        }

        reg = next_allocatable_register(reg);
      } while (reg != first_allocatable_register());

      GUARANTEE(spill_reg != Assembler::no_reg, "Sanity");

      // Remove all outgoing transfer links for the selected register and its
      // pair (if any). The transfers for the removed links will be
      // automatically performed on the third stage.
      //
      // Each register belongs to a closed cycle in this branch, so it must
      // have exactly one ingoing link.

      Register dst_spill_reg = first_allocatable_register();

      do {
        dst_spill_reg = next_allocatable_register(dst_spill_reg);
      } while (src_for(dst_spill_reg) != spill_reg &&
               dst_spill_reg != first_allocatable_register());

      remove_link_from_closed_cycle(spill_reg, dst_spill_reg);

      const Register pair_spill_reg = pair_for(spill_reg);

      if (pair_spill_reg != Assembler::no_reg) {
        // Instead of looking for the destination of pair_spill_reg we can take
        // a pair of the destination of its pair.
        const Register pair_dst_spill_reg = pair_for(dst_spill_reg);
        GUARANTEE(pair_dst_spill_reg != Assembler::no_reg, "Sanity");
        GUARANTEE(src_for(pair_dst_spill_reg) == pair_spill_reg, "Sanity");

        remove_link_from_closed_cycle(pair_spill_reg, pair_dst_spill_reg);
      }

      RegisterAllocator::spill(spill_reg);

      spare_reg = spill_reg;
    }
  }

  return spare_reg;
}

void TransferGraph::generate_move(const Register dst_reg,
                                         const Register src_reg) const {
  GUARANTEE(src_reg != Assembler::no_reg &&
            dst_reg != Assembler::no_reg, "Sanity");
  GUARANTEE(src_reg != dst_reg, "No loops allowed");

  Compiler::code_generator()->move(dst_reg, src_reg);
}

void TransferGraph::generate_transfer_operations(const Register src_reg,
                                                 const Mark reg_mark) {
  Register dst_reg = first_allocatable_register();

  do {
    if (src_for(dst_reg) == src_reg && mark(dst_reg) == reg_mark) {

      uninstall_link(src_reg, dst_reg);

      generate_transfer_operations(dst_reg, reg_mark);

      generate_move(dst_reg, src_reg);
    }

    dst_reg = next_allocatable_register(dst_reg);
  } while (_transfer_count > 0 &&
           dst_reg != first_allocatable_register());
}

inline void TransferGraph::process_closed_cycles() {
  Register base_reg = first_allocatable_register();

  Register closed_cycle_root = Assembler::no_reg;

  // Look if the graph has any closed cycles.
  do {
    if (is_closed_cycle(base_reg) && is_root(base_reg)) {
      closed_cycle_root = base_reg;
      break;
    }

    base_reg = next_allocatable_register(base_reg);
  } while (base_reg != first_allocatable_register());

  // No closed cycles in the graph, return.
  if (closed_cycle_root == Assembler::no_reg) {
    return;
  }

  const Register spare_reg = get_spare_register();
  GUARANTEE(spare_reg != Assembler::no_reg, "Sanity");
  GUARANTEE((is_tree(spare_reg) || is_open_cycle(spare_reg)) &&
            is_leaf(spare_reg),
            "Can only be a leaf of a tree or a open cycle");

  const bool spare_reg_is_root = is_root(spare_reg);
  const Mark spare_reg_mark = mark(spare_reg);

  // We are going to plug the spare register into a closed cycle,
  // so mark it as such.
  if (spare_reg_is_root) {
    unset_root(spare_reg);
  }
  unset_leaf(spare_reg);
  set_mark(spare_reg, closed_cycle_mark);

  const Register src_spare_reg = src_for(spare_reg);

  // Handle the case when the selected spare register has an ingoing transfer
  // link. We save the source register here and restore it after all closed
  // cycles are processed.
  if (src_spare_reg != Assembler::no_reg) {
    // Remove the ingoing link for the spare register, as it will be
    // damaged anyway.
    // We will reinstall it after we process all closed cycles.
    uninstall_link(src_spare_reg, spare_reg);
  }

  // Start with the found closed cycle root.
  base_reg = closed_cycle_root;

  do {
    if (is_closed_cycle(base_reg) && is_root(base_reg)) {
      // Replace src_reg->base_reg transfer with two
      // src_reg->spare_reg and spare_reg->base_reg.
      const Assembler::Register src_reg = src_for(base_reg);

      GUARANTEE(src_reg != Assembler::no_reg, "This is a cycle");
      GUARANTEE(spare_reg != Assembler::no_reg, "Must have one");

      uninstall_link(src_reg, base_reg);

      GUARANTEE(src_for(spare_reg) == Assembler::no_reg, "Sanity");

      install_link(src_reg, spare_reg);
      install_link(spare_reg, base_reg);

      // Traverse the graph from the space_reg and generate code.
      generate_transfer_operations(spare_reg, closed_cycle_mark);
    }

    base_reg = next_allocatable_register(base_reg);
  } while (base_reg != first_allocatable_register());

  // Restore the original marks.
  unset_closed_cycle(spare_reg);
  set_mark(spare_reg, spare_reg_mark);
  set_leaf(spare_reg);
  if (spare_reg_is_root) {
    set_root(spare_reg);
  }

  if (src_spare_reg != Assembler::no_reg) {
    GUARANTEE(spare_reg != Assembler::no_reg, "Sanity");

    // Reinstall the ingoing link to the spare register.
    install_link(src_spare_reg, spare_reg);
  }
}

// Shift all open cycles.
inline void TransferGraph::shift_open_cycles() {
  Register base_reg = first_allocatable_register();

  // Find all roots of open cycles.
  do {
    if (is_open_cycle(base_reg) && is_root(base_reg)) {
      const Register root_reg = base_reg;

      const Register src_root_reg = src_for(root_reg);

      GUARANTEE(src_root_reg != Assembler::no_reg &&
                is_open_cycle(src_root_reg) && !is_root(src_root_reg) &&
                !is_leaf(src_root_reg), "Sanity");

      Register cycle_reg = src_root_reg;
      Register src_cycle_reg = src_for(src_root_reg);

      GUARANTEE(src_cycle_reg != Assembler::no_reg &&
                is_open_cycle(src_cycle_reg) && !is_root(src_cycle_reg) &&
                !is_leaf(src_cycle_reg), "Sanity");

      // Locate the destination of link to be shifted - it is the other
      // destination of the source of root_reg.
      do {
        cycle_reg = src_cycle_reg;
        src_cycle_reg = src_for(cycle_reg);
        GUARANTEE(src_cycle_reg != Assembler::no_reg &&
                  is_open_cycle(src_cycle_reg) && !is_root(src_cycle_reg) &&
                  !is_leaf(src_cycle_reg), "Sanity");
      } while (src_cycle_reg != src_root_reg);

      // Shift the cycle by replacing src_cycle_reg->cycle_reg link with
      // root_reg->cycle_reg link.
      install_link(root_reg, cycle_reg);

      // Since the root_reg now has an outgoing link, it can no longer be a leaf.
      unset_leaf(root_reg);
    }

    base_reg = next_allocatable_register(base_reg);
  } while (base_reg != first_allocatable_register());
}

inline void TransferGraph::process_trees_and_open_cycles() {
  Register base_reg = first_allocatable_register();

  do {
    if (is_root(base_reg)) {
      if (is_tree(base_reg)) {
        // Root can have a loop, remove it here as we don't need to generate
        // any transfer code for loops.
        if (src_for(base_reg) == base_reg) {
          uninstall_link(base_reg, base_reg);
        }
        generate_transfer_operations(base_reg, tree_mark);
      } else if (is_open_cycle(base_reg)) {
        // The open cycle must already be shifted by this point.
        // First process a tree attached to this root if any.
        generate_transfer_operations(base_reg, tree_mark);

        // Then process a open cycle.
        generate_transfer_operations(base_reg, open_cycle_mark);
      }
    }

    base_reg = next_allocatable_register(base_reg);
  } while (base_reg != first_allocatable_register());
}


inline void VirtualStackFrame::conform_to_phase_two(VirtualStackFrame* other) {
  REGISTER_REFERENCES_CHECKER;

  TransferGraph graph(this, other);

  graph.markup();

  graph.process_closed_cycles();

  graph.shift_open_cycles();

  graph.process_trees_and_open_cycles();
}

inline void VirtualStackFrame::conform_to_phase_three(VirtualStackFrame* other) {
  REGISTER_REFERENCES_CHECKER;

  FOR_EACH_LOCATION_DO(src, dst, index)
    if (dst->is_flushed() || !dst->is_immediate()) {
      // Process memory->register transfers.
      const RawLocation::Actions remaining_actions =
        src->do_conform_to(index, dst, index, RawLocation::LOC_LOAD);

      // Process immediate->register transfers.
      if ((remaining_actions & RawLocation::REG_STORE) && src->is_immediate()) {
        Value src_value(src, index);
        Value dst_value(dst, index);

        // do the register store
        Compiler::code_generator()->move(dst_value, src_value);
      }
    }
  FOR_EACH_LOCATION_DONE(src, dst, index);
}

#ifndef PRODUCT
bool VirtualStackFrame::can_do_optimized_merge(VirtualStackFrame* other) {
  // Check for the limitation of the optimized merge algorithm:
  // it requires that if a register R1 is mapped to a long location in pair
  // with some other register R2, then all other locations mapped to R1
  // are also long locations paired with R2.
  {
    Assembler::Register pair_for[Assembler::number_of_registers];

    jvm_memset(pair_for, -1, sizeof pair_for);

    FOR_EACH_LOCATION_DO(src, dst, index)
      if (!src->is_flushed() && src->in_register()) {
        Value src_value(src, index);

        const Assembler::Register lo_reg = src_value.lo_register();

#if ENABLE_ARM_VFP
        // IMPL NOTE: Don't optimize if a frame contains a vfp
        // register.  Need to add support for this later.
        if (lo_reg > Assembler::number_of_gp_registers) {
          return false;
        }
#endif

        GUARANTEE(lo_reg >= 0 &&
                  (int)lo_reg < (int)Assembler::number_of_registers,
                  "Invalid register");

        if (src_value.use_two_registers()) {
          const Assembler::Register hi_reg = src_value.hi_register();

          const Assembler::Register lo_pair_reg = pair_for[lo_reg];

          if (lo_pair_reg != Assembler::no_reg && lo_pair_reg != hi_reg) {
            return false;
          }

          pair_for[lo_reg] = hi_reg;

          const Assembler::Register hi_pair_reg = pair_for[hi_reg];

          if (hi_pair_reg != Assembler::no_reg && hi_pair_reg != lo_reg) {
            return false;
          }

          pair_for[hi_reg] = lo_reg;
        } else {
          const Assembler::Register pair_reg = pair_for[lo_reg];

          if (pair_reg != Assembler::no_reg && pair_reg != lo_reg) {
            return false;
          }

          pair_for[lo_reg] = lo_reg;
        }
      }
    FOR_EACH_LOCATION_DONE(src, dst, index);
  }

  return true;
}
#endif

void VirtualStackFrame::conform_to_optimized_impl(VirtualStackFrame* other) {
  conform_to_prologue(other);

  const bool reg_reg_transfer_found = conform_to_phase_one(other);

  if (reg_reg_transfer_found) {
    conform_to_phase_two(other);
  }

  conform_to_phase_three(other);

  conform_to_epilogue(other);
}
#endif // PRODUCT

#ifndef PRODUCT
void VirtualStackFrame::verify_conform_to(VirtualStackFrame* other) {
  if (!VerifyVSFMerge) {
    return;
  }

#if USE_DEBUG_PRINTING
  // If VerboseVsfMerge is set, print source and target frame,
  // then print compiled code during merge.
  if (VerboseVSFMerge) {
    tty->print_cr("VSF merge:");
    tty->print("Source: ");
    this->print();
    tty->print("Target: ");
    other->print();
  }
#endif

#if ENABLE_VSF_MERGE_TEST
  {
#if ENABLE_TTY_TRACE
    TemporaryModifyGlobal print(&PrintCompiledCodeAsYouGo, VerboseVSFMerge);
#endif

    SETUP_ERROR_CHECKER_ARG;
    VSFMergeTest::verify_merge(this, other JVM_CHECK);
  }
#endif
}
#endif // PRODUCT

void VirtualStackFrame::commit_changes(Assembler::Register reg) {
  AllocationDisabler allocation_not_allowed_in_this_function;

  RawLocation *raw_location = raw_location_at(0);
  RawLocation *end  = raw_location_end(raw_location);
  int index = 0;

  while (raw_location < end) {
    raw_location->write_changes(reg, index);
    if (raw_location->is_two_word()) {
      index += 2;
      raw_location += 2;
    } else {
      index ++;
      raw_location += 1;
    }
  }
}

void VirtualStackFrame::spill_register(Assembler::Register reg) {
  AllocationDisabler allocation_not_allowed_in_this_function;

  RawLocation *raw_location = raw_location_at(0);
  RawLocation *end  = raw_location_end(raw_location);
  int index = 0;

  while (raw_location < end) {
    raw_location->spill_register(reg, index);
    if (raw_location->is_two_word()) {
      index += 2;
      raw_location += 2;
    } else {
      index ++;
      raw_location += 1;
    }
  }

  clear_literal(reg);

  //remember array length: if the base_register() == length_register()
  //the statue is error and should be obsolete.
  if (bound_mask() != 0) {
    if (length_register() == reg || base_register() == reg) {
      clear_bound();
    }
  }

}

// Modify a frame so that it doesn't use a particular register.
// If it >>does<< use that register, then either spill it or move it to
// an unallocated register.
void VirtualStackFrame::unuse_register(Assembler::Register reg) {
  bool used = false;
  Assembler::Register new_reg = Assembler::no_reg;
  clear_literal(reg);

  //remember array length
  if (bound_mask() != 0) {
    if (length_register() == reg || base_register() == reg) {
      clear_bound();
    }
  }

  for (VSFStream s(this); !s.eos(); s.next()) {
    if (is_mapped_by(s.index(), reg)) {
      if (!used) {
        // Our first time seeing this register
        used = true;
        if (RegisterAllocator::has_free(1)) {
          new_reg = RegisterAllocator::allocate();
          code_generator()->move(new_reg, reg);

          RegisterAllocator::move_notation(reg, new_reg);

        }
      }
      // Either spill the location, or modify to indicate that it is in
      // a different location
      Location location(this, s.index());
      if (new_reg == Assembler::no_reg) {
        location.flush();
   //cse
        RegisterAllocator::wipe_notation_of(reg);
      } else {
        location.change_register(new_reg, reg);
      }
    }
  }
  if (new_reg != Assembler::no_reg) {
    RegisterAllocator::dereference(new_reg);
  }
}

void VirtualStackFrame::pop(BasicType kind) {
  if (is_two_word(kind)) {
    pop2();
  } else {
    pop();
  }
}

void VirtualStackFrame::pop() {
#ifdef AZZERT
  // Use the virtual stack pointer as the location.
  int index = virtual_stack_pointer();

  // Pop the value.
  Location l(this, index);
  GUARANTEE(!l.is_two_word() && l.type() != T_ILLEGAL, "sanity check");
#endif
  decrement_virtual_stack_pointer();
}

void VirtualStackFrame::pop2() {
  decrement_virtual_stack_pointer();
  decrement_virtual_stack_pointer();
}

BasicType VirtualStackFrame::expression_stack_type(int index) {
  int n = virtual_stack_pointer() - index;
  Location l(this, n);
  GUARANTEE(!l.is_two_word(), "sanity check");
  BasicType t = l.type();
  if (t != T_ILLEGAL) {
    return t;
  }

  Location f(this, n - 1);
  return f.type();
}

inline void VirtualStackFrame::stack_2_1_to_1_2(void) {
  // Form: ..., value2, value1   => ..., value1, value2
  Value v1(expression_stack_type(0));
  pop(v1);

  Value v2(expression_stack_type(0));
  pop(v2);

  // Push the results
  push(v1);
  push(v2);
}

void VirtualStackFrame::stack_1_to_1_1() {
  // Form: ..., value1, value1   => ..., value1, value1

  // create a value for the top stack location
  Value v(expression_stack_type(0));

  // pop the value from the stack and push it twice
  pop(v);
  push(v);
  push(v);
}

inline void VirtualStackFrame::stack_2_1_to_2_1_2_1(void) {
  // Form: ..., value2, value1   => ..., value2, value1, value2. value1
  Value v1(expression_stack_type(0));
  pop(v1);

  Value v2(expression_stack_type(0));
  pop(v2);

  // Push the results
  push(v2);
  push(v1);
  push(v2);
  push(v1);
}

void VirtualStackFrame::stack_2_1_to_1_2_1() {
  // Form: ..., value2, value1   => ..., value1, value2. value1

  // Pop value1
  Value v1(expression_stack_type(0));
  pop(v1);

  // Pop value2
  Value v2(expression_stack_type(0));
  pop(v2);

  // Push the results
  push(v1);
  push(v2);
  push(v1);
}

void VirtualStackFrame::stack_3_2_1_to_1_3_2_1() {
  // Form: ..., value3, value2, value1   => ..., value1, value3, value2. value1
  Value v1(expression_stack_type(0));
  pop(v1);

  Value v2(expression_stack_type(0));
  pop(v2);

  Value v3(expression_stack_type(0));
  pop(v3);

  // Push the results
  push(v1);
  push(v3);
  push(v2);
  push(v1);
}

void VirtualStackFrame::stack_3_2_1_to_2_1_3_2_1() {
  // Form 1: ..., value3, value2, value1   =>
  //   ..., value2, value1, value3, value2. value1
  Value v1(expression_stack_type(0));
  pop(v1);

  Value v2(expression_stack_type(0));
  pop(v2);

  Value v3(expression_stack_type(0));
  pop(v3);

  // Push the results
  push(v2);
  push(v1);
  push(v3);
  push(v2);
  push(v1);
}

inline void VirtualStackFrame::stack_4_3_2_1_to_2_1_4_3_2_1(void) {
  // Form: ..., value4, value3, value2, value1
  //    => ..., value2, value1, value4, value3, value2, value1
  Value v1(expression_stack_type(0));
  pop(v1);

  Value v2(expression_stack_type(0));
  pop(v2);

  Value v3(expression_stack_type(0));
  pop(v3);

  Value v4(expression_stack_type(0));
  pop(v4);

  // Push the results
  push(v2);
  push(v1);
  push(v4);
  push(v3);
  push(v2);
  push(v1);
}

void VirtualStackFrame::swap() {
  stack_2_1_to_1_2();
}

void VirtualStackFrame::dup() {
  stack_1_to_1_1();
}

void VirtualStackFrame::dup2() {
  Location l1(this, virtual_stack_pointer());
  if (is_two_word(expression_stack_type(0))) {
    stack_1_to_1_1();
  } else {
    stack_2_1_to_2_1_2_1();
  }
}

void VirtualStackFrame::dup_x1() {
  stack_2_1_to_1_2_1();
}

void VirtualStackFrame::dup_x2() {
  if (is_two_word(expression_stack_type(1))) {
    stack_2_1_to_1_2_1();
  } else {
    stack_3_2_1_to_1_3_2_1();
  }
}

void VirtualStackFrame::dup2_x1() {
  if (is_two_word(expression_stack_type(0))) {
    stack_2_1_to_1_2_1();
  } else {
    stack_3_2_1_to_2_1_3_2_1();
  }
}

void VirtualStackFrame::dup2_x2() {
  if (is_two_word(expression_stack_type(0))) {
    if (is_two_word(expression_stack_type(2))) {
      stack_2_1_to_1_2_1();
    } else {
      stack_3_2_1_to_1_3_2_1();
    }
  } else {
    if (is_two_word(expression_stack_type(2))) {
      stack_3_2_1_to_2_1_3_2_1();
    } else {
      stack_4_3_2_1_to_2_1_4_3_2_1();
    }
  }
}

void VirtualStackFrame::value_at(Value& result, int index) {
  AllocationDisabler allocation_not_allowed_in_this_function;

  RawLocation *raw_location = raw_location_at(index);
  raw_location->read_value(result, index);
}

void VirtualStackFrame::value_at(ExtendedValue& result, int index) {
  AllocationDisabler allocation_not_allowed_in_this_function;

  RawLocation *raw_location = raw_location_at(index);
  if (raw_location->is_flushed()) {
    result.set_index(index, raw_location->type());
    result.set_flags(raw_location->flags());
  } else {
    result.is_value(raw_location->type());
    raw_location->read_value(result.value(), index);
  }
}

void VirtualStackFrame::value_at_put(int index, const Value& value) {
  AllocationDisabler allocation_not_allowed_in_this_function;

  RawLocation *raw_location = raw_location_at(index);
  if (index > 0) {
    RawLocation *previous = raw_location - 1;
    if (previous->is_two_word()) {
      // We're wiping out the second word of a long.  So mark
      // the whole thing as no longer relevant.
      previous->mark_as_illegal();
      raw_location->mark_as_illegal();
    }
  }

  raw_location->write_value(value);
  //Remember array length check
  //clear array length checked tag if the value
  //is modified.


  if (index < method()->max_locals()) {
    if (raw_location->type() == T_ARRAY ) {
      clear_bound();
    }
    raw_location->set_is_not_index_checked();
    raw_location->set_is_first_time_access();

  }


}

void VirtualStackFrame::pop(Value& result) {
  // Use the virtual stack pointer as the location.
  bool double_word = result.is_two_word();
  int index = virtual_stack_pointer();

  pop_tag();

  // Adjust the virtual stack pointer.
  decrement_virtual_stack_pointer();
  if (double_word) {
    decrement_virtual_stack_pointer();
    index --;
  }

  AllocationDisabler allocation_not_allowed;

  // Load the value.
  RawLocation *raw_location = raw_location_at(index);
  raw_location->read_value(result, index);

  // Flush the location.
  raw_location->mark_as_flushed();
}

void VirtualStackFrame::push(const Value& value) {
  bool double_word = value.is_two_word();
  // Adjust the virtual stack pointer.
  increment_virtual_stack_pointer();

  push_tag();

  if (double_word) increment_virtual_stack_pointer();

  // Put the value as the new top of stack.
  value_at_put(virtual_stack_pointer() - (double_word ? 1 : 0), value);
}

// This function is used to determine whether the receiver needs a null check
// without loading the receiver value into a register.
jubyte VirtualStackFrame::receiver_flags(const int size_of_parameters) const {
  const int location = virtual_stack_pointer() - size_of_parameters + 1;
  const RawLocation* raw_location = raw_location_at( location );
  GUARANTEE(raw_location->stack_type() == T_OBJECT, "sanity");

  return raw_location->flags();
}

void VirtualStackFrame::set_value_must_be_nonnull(Value &value) {
  AllocationDisabler allocation_not_allowed_in_this_function;
  GUARANTEE(value.in_register() && (value.stack_type() == T_OBJECT),
            "Guarantee");
  value.set_must_be_nonnull();

  register RawLocation *raw_location = raw_location_at(0);
  register RawLocation *end  = raw_location_end(raw_location);
  int index = 0;

  while (raw_location < end) {
    BasicType type = raw_location->stack_type();
    if (type == T_OBJECT && !raw_location->is_flushed() &&
        raw_location->in_register()) {
      if( Value(raw_location, index).lo_register() == value.lo_register()) {
        RawLocation::Status old_status = raw_location->status();
        value.set_must_be_nonnull();
        raw_location->write_value(value);
        raw_location->set_status(old_status);
      }
    }

    if (is_two_word(type)) {
      index += 2;
      raw_location += 2;
    } else {
      index ++;
      raw_location += 1;
    }
  }
}

void VirtualStackFrame::set_value_has_known_min_length(Value &value,
                                                       int length)
{
  GUARANTEE(value.in_register() && (value.stack_type() == T_OBJECT),
            "Guarantee");
  value.set_has_known_min_length(length);

  for (VSFStream s(this); !s.eos(); s.next()) {
    AllocationDisabler allocation_not_allowed_in_this_block;
    int index = s.index();
    RawLocation *l = raw_location_at(index);

    if (l->stack_type() == T_OBJECT && !l->is_flushed() && l->in_register()) {
      if (Value(l, index).lo_register() == value.lo_register()) {
        RawLocation::Status old_status = l->status();
        value.set_has_known_min_length(length);
        l->write_value(value);
        l->set_status(old_status);
      }
    }
  }
}

#if ENABLE_COMPILER_TYPE_INFO
void VirtualStackFrame::set_value_class(Value &value, JavaClass * java_class) {
  AllocationDisabler allocation_not_allowed_in_this_function;
  GUARANTEE(value.in_register() && value.stack_type() == T_OBJECT, "Sanity");
  const jushort class_id = java_class->class_id();
  const bool is_final_type = java_class->is_final_type();

  value.set_class_id(class_id);
  if (is_final_type) {
    value.set_is_exact_type();
  }

  register RawLocation *raw_location = raw_location_at(0);
  register RawLocation *end  = raw_location_end(raw_location);
  int index = 0;
  const Assembler::Register value_register = value.lo_register();

  while (raw_location < end) {
    BasicType type = raw_location->stack_type();
    if (type == T_OBJECT && !raw_location->is_flushed() && 
        raw_location->in_register() && 
        raw_location->get_register() == value_register) {
      raw_location->set_class_id(class_id);
      if (is_final_type) {
        raw_location->set_is_exact_type();
      }
    }

    if (is_two_word(type)) {
      index += 2;
      raw_location += 2;
    } else {
      index ++;
      raw_location += 1;
    }
  }
}
#endif

#if USE_COMPILER_LITERALS_MAP
Assembler::Register VirtualStackFrame::get_literal(int imm32,
                                                   LiteralAccessor& la) {
  LiteralElementStream les(this);
  for ( ; !les.eos() ; les.next()) {
    if (les.value() == imm32) {
        return les.reg();
    }
  }
  Assembler::Register reg = RegisterAllocator::allocate();
  // We can use existing literals to help us create the new one
#if ARM && !ENABLE_THUMB_COMPILER
  code_generator()->mov_imm(reg, imm32, &la);
#else
  // IMPL_NOTE: change the reference to a pointer for SH and THUMB compilers
  // as well, which generates shorter code. See Macros::arith_imm()
  // in Assembler_arm.cpp for more info.
  code_generator()->mov_imm(reg, imm32, la);
#endif

  // Don't mark the literal as "available" until >>after<< the above move
  set_has_literal_value(reg, imm32);
  RegisterAllocator::dereference(reg);
  return reg;
}

void VirtualStackFrame::clear_literals(void) {
#if ENABLE_ARM_VFP
  _literals_mask[ 0 ] = 0;
  _literals_mask[ 1 ] = 0;
#endif  
  jvm_memset(_literals_map, 0, sizeof _literals_map);
}

bool VirtualStackFrame::has_no_literals(void) const{
#if ENABLE_ARM_VFP
  if( _literals_mask[0] | _literals_mask[1] ) {
    return false;
  }
#endif  
  for( int i = 0; i < Assembler::number_of_registers; i++ ) {
    if (get_literal(Assembler::Register(i))) { 
      return false;
    }
  }
  return true;
}
#endif

#if ENABLE_ARM_VFP
Assembler::Register VirtualStackFrame::find_zero(void) const {
  const jint* masks = literals_mask();
  int reg = 0;
  if( masks[0] == 0 ) {
    masks++;
    reg += BitsPerInt;
  }
  for( jint mask = *masks; mask; mask >>= 1 ) {
    if( mask & 1 ) {
      return Assembler::Register(reg);
    }
    reg++;
  }
  return Assembler::no_reg;
}

Assembler::Register VirtualStackFrame::find_non_NaN(void) const {
  GUARANTEE( find_zero() == Assembler::no_reg, "must search for zeroes first" );
  // We prefer NaNs in VFP registers, so do the backward search
  Assembler::Register reg = Assembler::s31;
  do {
    const jint imm32 = get_literal(Assembler::Register(reg));
    if (is_non_NaN(imm32)) {
      return reg;
    }
    reg = Assembler::Register( reg - 1 );
  } while( reg >= Assembler::r0 );
  return Assembler::no_reg;
}

Assembler::Register VirtualStackFrame::find_double_non_NaN(void) const {
  // We prefer NaNs in VFP registers, so do the backward search
  int reg = Assembler::s31;
  do {
    const jint imm32 = get_literal(Assembler::Register(reg));    
    if (is_non_NaN(imm32)) {
      return Assembler::Register(reg - 1);
    }
    reg -= 2;
  } while (reg >= Assembler::s1);
  return Assembler::no_reg;
}

Assembler::Register VirtualStackFrame::find_double_vfp_literal(const jint lo, const jint hi) const {
  const jint* literals_map_base = literals_map();
  int reg = Assembler::s0;
  do {
    if (literals_map_base[reg] == lo && literals_map_base[reg + 1] == hi) {
      return Assembler::Register(reg);
    }
    reg += 2;
  } while (reg < Assembler::s31);
  return Assembler::no_reg;
}
#endif    


#if ENABLE_REMEMBER_ARRAY_CHECK && \
     ENABLE_REMEMBER_ARRAY_LENGTH && ENABLE_NPCE
bool VirtualStackFrame::is_value_must_be_index_checked(
                                 Assembler::Register reg, Value &value) {
  AllocationDisabler allocation_not_allowed_in_this_function;
  GUARANTEE(value.in_register() && (value.stack_type() == T_INT ||
               value.stack_type() == T_LONG ), "Guarantee");
  if (bound_mask() == 0 ||  reg != length_register()) {
    return false;
  }

  //cse
  if(!value.is_two_word() &&
    RegisterAllocator::is_checked_before(value.lo_register())) {
    VERBOSE_CSE(("reg %s is mark as index checked ",
          Disassembler::reg_name(value.lo_register())));

    return true;
  }

  int index = 0;
  int checked_index = bound_index();
  register RawLocation *raw_location = raw_location_at(checked_index);
  if ( checked_index !=0 ) {
    if ( !raw_location->is_flushed() && raw_location->in_register() &&
      raw_location->flags() & Value::F_HAS_INDEX_CHECKED &&
      Value(raw_location, checked_index).lo_register() == value.lo_register() ) {
      return true;
    }
  }

  return false;
}

void VirtualStackFrame::set_value_must_be_index_checked(
                                    Assembler::Register reg, Value &value) {
  AllocationDisabler allocation_not_allowed_in_this_function;
  GUARANTEE(value.in_register() && (value.stack_type() == T_INT ||
         value.stack_type() == T_LONG ), "Guarantee");
  if (bound_mask() == 0 ||  reg != length_register()) {
    return ;
  }

  value.set_must_be_index_checked();

#if ENABLE_CSE
  if ( !value.is_two_word() ) {
    VERBOSE_CSE(("reg %s is index checked before ",
          Disassembler::reg_name(value.lo_register())));
     RegisterAllocator::set_checked_before(value.lo_register());
  }
#endif


  int checked_index = bound_index();
  register RawLocation *raw_location = raw_location_at(checked_index);
  //return if an array length index has been cached.
  if ( checked_index !=0 ) {
    if ( !raw_location->is_flushed() && raw_location->in_register() &&
         raw_location->flags() & Value::F_HAS_INDEX_CHECKED ) {
      return ;
    }
  }

  int index = 0;
  raw_location = raw_location_at(0);
  register RawLocation *end  = raw_location_end(raw_location);
  while (raw_location < end) {
    BasicType type = raw_location->stack_type();
    if ( index > max_value_of_index ) {
      return ;
    }
    if ((type == T_INT ||type == T_LONG ) && !raw_location->is_flushed() &&
        raw_location->in_register()) {
      if ( Value(raw_location, index).lo_register() == value.lo_register()) {
        RawLocation::Status old_status = raw_location->status();
        value.set_must_be_index_checked();
        raw_location->write_value(value);
        raw_location->set_status(old_status);
  set_boundary_index(index);
        return ;
      }
    }
    if (is_two_word(type)) {
      index += 2;
      raw_location += 2;
    } else {
      index ++;
      raw_location += 1;
    }
  }

  return ;
}


#endif

#if ENABLE_REMEMBER_ARRAY_LENGTH
//Following functions is added to the class VirtualStackFrame.
//Because we added an integer _bound_mask to the structure VirtualStackFrameDesc,
//VM need these functions to be called to manipulate the new virtual stack frame.

Assembler::Register VirtualStackFrame::cached_array_length(Assembler::Register array_base,
                                                 bool first_time,
                                                 Assembler::Condition cond) {
  if (is_cached_array_bound_of(array_base)) {
    if (first_time) {
      //The variable references another array
      //we must clean the record to cache the new array length
      clear_bound();
    } else {
      return length_register();
    }
  }

  Assembler::Register length;
  {
   Value result(T_INT);
   Value array(T_ARRAY);
   array.set_register(array_base);
   RegisterAllocator::reference(array_base);
   SETUP_ERROR_CHECKER_ARG;
   Compiler::code_generator()->load_from_object(result, array,
                                     Array::length_offset(), false JVM_NO_CHECK);
   length = result.lo_register();
  }

  bool is_local = false;
  //We do the setting more careful.
  //if the value isn't a local variable
  //we won't do the cache.
  if(bound_flag() < 1) {

    is_local = set_is_not_first_time_access(array_base);

    if (is_local) {
      //clear old bound status
      clear_bound();
      set_boundary_value(length, array_base);
    }
  }

  GUARANTEE(!is_local || is_bound_mask_valid(),
    "Only length of array in local variable could be cached.");

  return length;
}

jint VirtualStackFrame::bound_index(void) const {
  return (bound_mask() & index_bits);
}

Assembler::Register VirtualStackFrame::length_register(void) const {
  return Assembler::as_register((bound_mask() & bound_bits) >> bound_shift );
}

Assembler::Register VirtualStackFrame::base_register(void) const {
  return Assembler::as_register((bound_mask() & base_bits) >> base_shift);
}

int VirtualStackFrame::bound_flag(void) const {
  return (bound_mask() & flag_bits )>> flag_shift;
}

void VirtualStackFrame::clear_bound(void) {
  clear_must_be_index_checked_status_of_values();
  RegisterAllocator::clear_check_status();
  set_bound_mask(0);
}

#if  ENABLE_REMEMBER_ARRAY_CHECK && ENABLE_NPCE
void VirtualStackFrame::clear_must_be_index_checked_status_of_values(void) {
  AllocationDisabler allocation_not_allowed;
  register RawLocation *raw_location = raw_location_at(0);
  register RawLocation *end  = raw_location_end(raw_location);
  if ( is_bound_mask_valid()) {
    while (raw_location < end) {
      raw_location->set_flags(raw_location->flags()
          & ~Value::F_HAS_INDEX_CHECKED);
      if (raw_location->is_two_word()) {
        raw_location ++;
      }
      raw_location ++;
    }
  }
}
#endif

bool VirtualStackFrame::is_cached_array_bound_of(Assembler::Register base_reg) const {
  if (is_bound_mask_valid() &&  base_register() == base_reg) {
    return true;
  }
  return false;
}

bool VirtualStackFrame::is_bound_mask_valid(void) const {

  if (bound_mask() != 0 ) {
    return true;
  }
  return false;
}

void VirtualStackFrame::set_boundary_flag(void) {
  set_bound_mask((1)<<flag_shift | bound_mask());
}

//Checked wheather a register is used by one value in current
//VF.
bool VirtualStackFrame::is_allocated(const Assembler::Register reg) const {
  AllocationDisabler allocation_not_allowed;

  const RawLocation* raw_location = raw_location_at(0);
  const RawLocation* end  = raw_location_end(raw_location);

  for( ; raw_location < end; raw_location ++ ) {
    if (raw_location->in_register()) {
      if (raw_location->value() == (jint)reg) {
        return true;
      }
      if (raw_location->is_two_word()) {
        raw_location ++;
        if (raw_location->value() == (jint)reg) {
          return true;
        }
      }
    } else {
      if (raw_location->is_two_word()) {
        raw_location ++;
      }
    }
  }

#if USE_COMPILER_LITERALS_MAP
  if (has_literal(reg)) {
    return true;
  }
#endif

  return false;
}

//free the cached register for register_allocator in case there's no
//other free registers
Assembler::Register VirtualStackFrame::free_length_register() {
   Assembler::Register result = Assembler::no_reg;
   if (length_register() != 0 &&
        bound_flag() == 0 ) {
      result = length_register();
      clear_bound();
   }
   return result;
}


//reload cached array length in register for OSR stub or TimerTick Stub(non merge conformance)
void VirtualStackFrame::recache_array_length(Assembler::Register array_base,
                    Assembler::Register array_length) {
  if(is_allocated(array_base))
 {
    Value array(T_ARRAY);
    Value length(T_INT);
    array.set_register(array_base);
    length.set_register(array_length);
    RegisterAllocator::reference(array_base);
    RegisterAllocator::reference(array_length);


    //tell the code generator not to assign register for length.
    length.set_is_not_first_time_access();

    SETUP_ERROR_CHECKER_ARG;
    Compiler::code_generator()->load_from_object(length,
                                            array,
                                            Array::length_offset(),
                                            false JVM_NO_CHECK);
  }
}

void VirtualStackFrame::set_boundary_index(jint index) {
   set_bound_mask(( bound_mask() & (~index_bits)) |index);
}

void VirtualStackFrame::set_boundary_value(Assembler::Register bound_reg,
                                              Assembler::Register base_reg) {
  set_bound_mask( base_reg << base_shift | bound_reg << bound_shift );
  set_boundary_flag();
}

bool
VirtualStackFrame::set_is_not_first_time_access(Assembler::Register reg) {
  AllocationDisabler allocation_not_allowed_in_this_function;
  RawLocation *raw_location = raw_location_at(0);
  RawLocation *end  = raw_location_end(raw_location);

  int index = 0;
  while (raw_location < end) {
    if ( raw_location->set_is_not_first_time_access(reg ) ) {
      if ( method()->is_local(index)) {
        return true;
      } else {
        return false;
      }
    }
    if (raw_location->is_two_word()) {
      index += 2;
      raw_location += 2;
    } else {
      index ++;
      raw_location += 1;
    }
  }
  return false;
}

#if ENABLE_REMEMBER_ARRAY_CHECK && ENABLE_NPCE
bool VirtualStackFrame::try_to_set_must_be_index_checked(Assembler::Register length, Value& index) {
  if (!index.is_immediate() &&  index.stack_type() == T_INT) {
    if ( is_value_must_be_index_checked( length, index)) {
      COMPILER_PRINT_AS_YOU_GO(("Omit a array length checking"));
      return true;
    } else {
      set_value_must_be_index_checked( length, index);
    }
  }
  return false;
}
#endif

Assembler::Register VirtualStackFrame::try_to_free_length_register() {
    //This make the allocator allocate bound register until all other
    // registers are in use.
    Assembler::Register reg = Compiler::current()->frame()->free_length_register();
    if ( reg != Assembler::no_reg ) {
      RegisterAllocator::reference(reg);

      //cse
      RegisterAllocator::wipe_notation_of(reg);

      return reg;
    }
    return Assembler::no_reg;
}
#endif //ENABLE_REMEMBER_ARRAY_LENGTH

#if USE_COMPILER_LITERALS_MAP
void LiteralElementStream::next(void) {
  int index;
  for (index = _index; ++index < Assembler::number_of_registers;) {
    if (_vsf->has_literal(Assembler::Register(index))) {
      break;
    }
  }
  _index = index;
}
#endif


#ifndef PRODUCT

void VirtualStackFrame::dump(bool as_comment) {
#if USE_DEBUG_PRINTING
  char str[1024];
  char *p = str;
  bool first = true;
  *p++ = '{';
  if (Verbose) {
    jvm_sprintf(p, " (vsp=%d,sp=%d) ", virtual_stack_pointer(), stack_pointer());
    p += jvm_strlen(p);
  }

#if ENABLE_CSE
  jvm_sprintf(p, "cse_tag=[0x%08x]", (jint)_cse_tag);
  p += jvm_strlen(p);
  //dump register notation
  RegisterAllocator::dump_notation();
#endif

  for (VSFStream s(this); !s.eos(); s.next()) {
    if ((p - str) > sizeof(str) - 50) {
      // We're threatening to overrun the buffer
      jvm_sprintf(p, "...");
      p += jvm_strlen(p);
      break;
    }
    if (!first) {
        jvm_sprintf(p, ", "); p += 2;
    }
    first = false;
    if (method()->is_local(s.index())) {
      jvm_sprintf(p, "L%d = ", s.index());
    } else {
      jvm_sprintf(p, "E%d = ", virtual_stack_pointer() - s.index());
    }
    p += jvm_strlen(p);
    Location location(this, s.index());
    if (location.is_flushed()) {
      jvm_sprintf(p, "mem"); p += jvm_strlen(p);
    } else {
      const Value value( &location );
      if (value.in_register()) {
        if (value.use_two_registers()) {
          Assembler::Register reg_hi = value.hi_register();
          Assembler::Register reg_lo = value.lo_register();
#if USE_COMPILER_LITERALS_MAP
#if ENABLE_ARM_VFP
          if( value.type() == T_DOUBLE ) {
            Disassembler::vfp_reg_name( 'd', reg_lo - Assembler::s0, p );
          } else
#endif
          {
            jvm_sprintf(p, "%s:%s", Disassembler::reg_name(reg_hi),
                                    Disassembler::reg_name(reg_lo));
          }
#else
          jvm_sprintf(p, "%s:%s", Assembler::name_for_long_register(reg_hi),
                              Assembler::name_for_long_register(reg_lo));
#endif // USE_COMPILER_LITERALS_MAP
          p += jvm_strlen(p);
        } else {
          Assembler::Register reg = value.lo_register();
#if USE_COMPILER_LITERALS_MAP
#if ENABLE_ARM_VFP
          if( value.type() == T_FLOAT ) {
            Disassembler::vfp_reg_name( 's', reg - Assembler::s0, p );            
          } else
#endif
          {
            jvm_sprintf(p, "%s", Disassembler::reg_name(reg));
          }
          p += jvm_strlen(p);
#else
          jvm_sprintf(p, "%s", Assembler::name_for_long_register(reg));
          p += jvm_strlen(p);
          if (value.type() == T_FLOAT || value.type() == T_DOUBLE) {
            const FPURegisterMap& fpu_map = fpu_register_map();
            jvm_sprintf(p, " ST(%d)", fpu_map.index_for(reg));
            p += jvm_strlen(p);
          }
#endif // USE_COMPILER_LITERALS_MAP
        }
      } else if (!value.is_present()) {
        jvm_sprintf(p, "XX"); p += jvm_strlen(p);
      } else {
        switch (value.type()) {
          case T_BOOLEAN: // fall-through
          case T_BYTE   : // fall-through
          case T_CHAR   : // fall-through
          case T_SHORT  : // fall-through
          case T_INT    :
            jvm_sprintf(p, "%d", value.as_int());
            break;
          case T_LONG   :
            jvm_sprintf(p, "%lld", value.as_long());
            break;
#if ENABLE_FLOAT
          case T_FLOAT  :
            // take care of promotion
            jvm_sprintf(p, "%f", jvm_f2d(value.as_float()));
            break;
          case T_DOUBLE :
            jvm_sprintf(p, "%f", value.as_double());
            break;
#endif
          case T_OBJECT :
            GUARANTEE(value.must_be_null(), "immediate object");
            jvm_sprintf(p, "null");
            break;
          default       :
            SHOULD_NOT_REACH_HERE();
            break;
        }
        p += jvm_strlen(p);
      }

      if (location.is_cached()) {
        jvm_sprintf(p, " & mem");
        p += jvm_strlen(p);
      }
    }

    switch (location.type()) {
      case T_BOOLEAN: // fall-through
      case T_BYTE   : // fall-through
      case T_CHAR   : // fall-through
      case T_SHORT  : // fall-through
      case T_INT    : jvm_sprintf(p, " (int)");
                      break;
      case T_LONG   : jvm_sprintf(p, " (long)");
                      break;
      case T_FLOAT  : jvm_sprintf(p, " (float)");
                      break;
      case T_OBJECT : 
#if ENABLE_COMPILER_TYPE_INFO
      {
        JavaClass::Raw java_class = 
          Universe::class_from_id(location.class_id());
        GUARANTEE(java_class.not_null(), "Sanity");
        Symbol::Raw class_name = java_class().name();
        *p++ = ' ';
        *p++ = '(';
        jvm_memcpy(p, class_name().utf8_data(), class_name().length());
        p += class_name().length();
        jvm_sprintf(p, "%s%s)",
                    (location.must_be_null() ? ":null" :
                     location.must_be_nonnull() ? ":nonnull" : ""),
                    (location.not_on_heap() ? "!" : ""));
      }
#else
                   jvm_sprintf(p, " (%s%s)",
                              (location.must_be_null() ? "null" :
                               location.is_string() ? "string" :
                               location.is_string_array() ? "string[]" :
                               location.is_object_array() ? "object[]" :
                               location.must_be_nonnull() ? "nonnull" : "obj"
                              ),
                              (location.not_on_heap() ? "!" : ""));
#endif
                      if (location.has_known_min_length()) {
                        p += jvm_strlen(p);
                        jvm_sprintf(p, "[%d]", location.length());
                      }
                      break;
      case T_DOUBLE : jvm_sprintf(p, " (double)");
                      break;
      case T_ILLEGAL: break;
      default       : SHOULD_NOT_REACH_HERE();
                      break;
    }
    p += jvm_strlen(p);
  }

#if USE_COMPILER_LITERALS_MAP
  {
    // Print any literals
    LiteralElementStream les(this);
    for ( ; !les.eos() ; les.next()) {
      if ((p - str) > sizeof(str) - 50) {
        // We're threatening to overrun the buffer
        jvm_sprintf(p, "...");
        p += jvm_strlen(p);
        break;
      }
#if ENABLE_ARM_VFP
      Assembler::Register r = les.reg();
      char prefix = 'r';
      if (r >= Assembler::s0) {
        r = Assembler::Register(r - Assembler::s0);
        prefix = 's';
      }
      jvm_sprintf(p, ", %c%d = %d", prefix, r, les.value());
#else
      jvm_sprintf(p, ", r%d = %d", les.reg(), les.value());
#endif      
      p += jvm_strlen(p);
    }
  }
#endif

  *p++ = '}';
  *p = 0;
  if (as_comment) {
    code_generator()->comment(str);
  } else {
    tty->print_cr(str);
  }
#endif // USE_DEBUG_PRINTING
}

void VirtualStackFrame::print() {
#if USE_DEBUG_PRINTING
  dump(false);
#endif
}

void RawLocationData::print_on(Stream *st) {
#if USE_DEBUG_PRINTING
  st->print_cr("type   = %s", name_for((BasicType)_type));
  switch (_where) {
  case Value::T_NOWHERE:   st->print_cr("where  = nowhere");   break;
  case Value::T_IMMEDIATE: st->print_cr("where  = immediate"); break;
  case Value::T_REGISTER:  st->print_cr("where  = register");  break;
  default:                 st->print_cr("where  = ?? (%d)", _where);
  }
  st->print_cr("status = %d", _status);
  st->print_cr("flags  = %d", _flags);
  st->print_cr("value  = %d", _value);
  st->print_cr("length = %d", _length);
#endif
}

void RawLocationData::p() {
#if USE_DEBUG_PRINTING
  print_on(tty);
#endif
}

#endif // !PRODUCT

void PreserveVirtualStackFrameState::save(JVM_SINGLE_ARG_TRAPS) {
  _saved_frame = Compiler::current()->get_cached_preserved_frame();
  if( _saved_frame == NULL ) {
    _saved_frame = frame()->clone(JVM_SINGLE_ARG_ZCHECK(_saved_frame));
  } else {
    frame()->copy_to( _saved_frame );
  }

#if NOT_CURRENTLY_USED
  // remove immediate information from the saved frame to make merging possible
  for (VSFStream s(saved_frame()); !s.eos(); s.next()) {
    Location loc(saved_frame(), s.index());
    if (loc.is_immediate()) {
      loc.mark_as_flushed();
    }
    loc.clear_flags();
#if ENABLE_COMPILER_TYPE_INFO
    loc.reset_class_id();
#endif
  }
#endif

  // More immediates that need to be flushed
  saved_frame()->clear_literals();
  // flush the frame
  frame()->flush(JVM_SINGLE_ARG_CHECK);
}

void PreserveVirtualStackFrameState::restore( void ) {
  // If we got outofmemory above when calling clone() we don't have a
  // saved frame and we have a pending exception so we need to just return
  JVM_DELAYED_CHECK;
#ifdef AZZERT
  for (VSFStream s(frame()); !s.eos(); s.next()) {
    Location loc(frame(), s.index());
    GUARANTEE(loc.is_flushed(),
              "cannot change frame across virtual stack frame preserves");
  }
#endif

#if USE_COMPILER_FPU_MAP
  // remove the contents in the FPU
  frame()->fpu_register_map().reset();
#endif

  frame()->conform_to(saved_frame());
  Compiler::current()->set_cached_preserved_frame(saved_frame());

  // Make sure we don't use it anymore
  AZZERT_ONLY(_saved_frame = NULL);
}

#if ENABLE_COMPRESSED_VSF
static inline void vsf_flush_do(address* regs, OopDesc* method, jushort* pool) {
  juint reg_list = *--pool;
  int sp_delta = (reg_list >> Relocation::sp_shift) & Relocation::max_sp_delta;
  if (sp_delta == Relocation::max_sp_delta) {
    sp_delta = *--pool;
  }
  if (reg_list & Relocation::sp_negative) {
    regs[Assembler::jsp] -= sp_delta * BytesPerWord;
  } else {
    regs[Assembler::jsp] += sp_delta * BytesPerWord;
  }

  Method::Raw m = method;
  const address fp = regs[Assembler::fp];
  const int adjustment = JavaStackDirection * (1 - m().max_locals());
  address *locals, *expr;

  locals = (address*)(fp + JavaFrame::end_of_locals_offset()) + adjustment;
  expr = m().uses_monitors() ?
    *(address**)(fp + JavaFrame::stack_bottom_pointer_offset()) + adjustment :
    (address*)(fp + JavaFrame::empty_stack_offset()) + adjustment;
  
  for (reg_list &= Relocation::reg_mask; reg_list != 0; reg_list >>= 1) {
    if (reg_list & 1) {
      do {
        int location = *--pool & Relocation::location_mask;
        if (*pool & Relocation::extended_mask) {
          location = (location << Relocation::location_width) |
                     (*--pool & Relocation::location_mask);
        }
        if (m().is_local(location)) {
          locals[location * JavaStackDirection] = *regs;
        } else {
          expr[location * JavaStackDirection] = *regs;
        }
      } while (!(*pool & Relocation::last_mask));
    }
    regs++;
  }
}

extern "C" void vsf_flush(address* regs) {
  const address pc = regs[Assembler::pc] + 4;
  *(address*)(regs[Assembler::fp] + JavaFrame::saved_pc_offset()) = pc;
  CompiledMethod::Raw cm = JavaFrame::find_compiled_method(pc);
  const int offset = pc - cm().entry();

  for (RelocationReader stream(&cm); !stream.at_end(); stream.advance()) {
    if (stream.code_offset() == offset &&
        stream.kind() == Relocation::compressed_vsf_type) {
      vsf_flush_do(regs, cm().method(), stream.current_address());
      return;
    }
  }
  SHOULD_NOT_REACH_HERE();
}

static inline void vsf_restore_do(address* regs, OopDesc* method, jushort* pool) {
  juint reg_list = *--pool;
  int sp_delta = (reg_list >> Relocation::sp_shift) & Relocation::max_sp_delta;
  if (sp_delta == Relocation::max_sp_delta) {
    sp_delta = *--pool;
  }
  if (reg_list & Relocation::sp_negative) {
    regs[Assembler::jsp] += sp_delta * BytesPerWord;
  } else {
    regs[Assembler::jsp] -= sp_delta * BytesPerWord;
  }

  Method::Raw m = method;
  const address fp = regs[Assembler::fp];
  const int adjustment = JavaStackDirection * (1 - m().max_locals());
  address *locals, *expr;

  locals = (address*)(fp + JavaFrame::end_of_locals_offset()) + adjustment;
  expr = m().uses_monitors() ?
    *(address**)(fp + JavaFrame::stack_bottom_pointer_offset()) + adjustment :
    (address*)(fp + JavaFrame::empty_stack_offset()) + adjustment;
  
  for (reg_list &= Relocation::reg_mask; reg_list != 0; reg_list >>= 1) {
    if (reg_list & 1) {
      do {
        int location = *--pool & Relocation::location_mask;
        if (*pool & Relocation::extended_mask) {
          location = (location << Relocation::location_width) |
                     (*--pool & Relocation::location_mask);
        }
        if (m().is_local(location)) {
          *regs = locals[location * JavaStackDirection];
        } else {
          *regs = expr[location * JavaStackDirection];
        }
      } while (!(*pool & Relocation::last_mask));
    }
    regs++;
  }
}

extern "C" void vsf_restore(address* regs) {
  const address pc =
    *(address*)(regs[Assembler::fp] + JavaFrame::saved_pc_offset());
  regs[Assembler::pc] = pc;
  CompiledMethod::Raw cm = JavaFrame::find_compiled_method(pc);
  const int offset = pc - cm().entry();

  for (RelocationReader stream(&cm); !stream.at_end(); stream.advance()) {
    if (stream.code_offset() == offset &&
        stream.kind() == Relocation::compressed_vsf_type) {
      vsf_restore_do(regs, cm().method(), stream.current_address());
      return;
    }
  }
  SHOULD_NOT_REACH_HERE();
}
#endif

#ifdef ABORT_CSE_TRACKING
#undef ABORT_CSE_TRACKING
#endif
#endif
