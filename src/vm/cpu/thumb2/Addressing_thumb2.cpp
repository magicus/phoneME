/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

# include "incls/_precompiled.incl"
# include "incls/_Addressing_thumb2.cpp.incl"

#if ENABLE_COMPILER

MemoryAddress::~MemoryAddress() {
  // dereference any allocated address registers
  if (has_address_register()) {
      RegisterAllocator::dereference(address_register());
  }
}

void MemoryAddress::prepare_preindexed_address(jint address_offset,
                                               Assembler::Register& reg,
                                               jint& offset){
  offset = 0;

  if (!has_address_register()) {
    // Try to do direct access
    jint fixed_offset;
    if (has_fixed_offset(fixed_offset)) {
      offset = fixed_offset + base_offset() + address_offset;
      reg = fixed_register();
      return;
    }
    create_and_initialize_address_register();
  }

  GUARANTEE(has_address_register(), "We must have address register by now");

  reg = address_register();

  int xbase_offset =            // base_offset or 0
    address_register_includes_base_offset() ? 0 : base_offset();
  if (address_offset == 0 && xbase_offset != 0) {
    // Update the address_register so that it includes the base_offset
    set_address_register_includes_base_offset();
    code_generator()->add(address_register(), address_register(), xbase_offset);
    offset = 0;
  } else {
    offset = (address_offset + xbase_offset);
  }
}

void MemoryAddress::prepare_indexed_address(jint address_offset,
                                            Assembler::Register& reg,
                                            jint& offset){
  if (!has_address_register()) {
    jint fixed_offset;
    if (has_fixed_offset(fixed_offset)) {
      offset = fixed_offset + base_offset() + address_offset;
      reg = fixed_register();
      return;
    }
    // We have to allocate an address register and fill it in
    create_and_initialize_address_register();
  }
  GUARANTEE(has_address_register(), "We must have address register by now");

  offset = address_offset;

  if (!address_register_includes_base_offset()) {
    offset += base_offset();
  }

  reg = address_register();
  return;
}

#if ENABLE_ARM_VFP
Assembler::Address5 MemoryAddress::address_5_for(jint address_offset) {
  if (!has_address_register()) {
    // Try to do direct access
    jint fixed_offset;
    if (has_fixed_offset(fixed_offset)) {
      const jint offset = fixed_offset + base_offset() + address_offset;
      if (-(1 << 10) < offset && offset < (1 << 10)) {
        return Assembler::imm_index5(fixed_register(), offset);
      }
    }
    create_and_initialize_address_register();
  }
  GUARANTEE(has_address_register(), "We must have address register by now");
  const int xbase_offset =            // base_offset or 0
    address_register_includes_base_offset() ? 0 : base_offset();
  return Assembler::imm_index5(address_register(),
                               address_offset + xbase_offset);
}
#endif

void MemoryAddress::create_and_initialize_address_register() {
  // We have to allocate an address register and fill it in
  set_address_register(RegisterAllocator::allocate());
  // fill in the address register
  fill_in_address_register();
  // Remove all values except for the ones we need
  destroy_nonaddress_registers();
}

void MemoryAddress::fill_in_address_register() {
  // In all cases exception for variable arrays indices, we are looking at
  // at fixed offset into the object.
  jint fixed_offset;
  if (has_fixed_offset(fixed_offset)) {
    code_generator()->mov(address_register(), fixed_offset + base_offset());
    code_generator()->add(address_register(), fixed_register(),
                          address_register());
    set_address_register_includes_base_offset();
  } else {
    // This is a virtual method, and in this case, we better be calling
    // an overriding definition.
    SHOULD_NOT_REACH_HERE();
  }
}


void HeapAddress::write_barrier_prolog() {
  // We must have an address register
  if (!has_address_register()) {
    create_and_initialize_address_register();
  }
}

void HeapAddress::write_barrier_epilog() {
  GUARANTEE(has_address_register(),
            "write barrier must have an address register");

  GUARANTEE(base_offset() == 0 || address_register_includes_base_offset() ,
            "write_barrier_epilog() must follow address_2_for(0)");

  // This is almost always the last thing we do with an address, so it
  // is okay to steal its temporary register.  This saves us one or two
  // instructions in many cases.
  Assembler::Register dst = address_register();
  clear_address_register();
#if ENABLE_ARM_V7
  if (UseHandlers) {
    if (RegisterAllocator::references(dst) > 1) {
      Assembler::Register tmp = RegisterAllocator::allocate();
      code_generator()->mov(tmp, dst);
      code_generator()->hbl(CodeGenerator::write_barrier_handler_r0 + (int)tmp);
      RegisterAllocator::dereference(tmp);
    } else {
      code_generator()->hbl(CodeGenerator::write_barrier_handler_r0 + (int)dst);
    }
  } else
#endif
  {
    Assembler::Register tmp1 = RegisterAllocator::allocate();
    Assembler::Register tmp2 = RegisterAllocator::allocate();
    Assembler::Register tmp3 = Assembler::r12;
    code_generator()->oop_write_barrier(dst, tmp1, tmp2, tmp3, false);
    RegisterAllocator::dereference(tmp1);
    RegisterAllocator::dereference(tmp2);
  }
  RegisterAllocator::dereference(dst);
}

bool FieldAddress::has_fixed_offset(jint& fixed_offset) {
  fixed_offset = offset();
  return true;
}

Assembler::Register FieldAddress::fixed_register() {
  return object()->lo_register();
}

void FieldAddress::destroy_nonaddress_registers() {
  object()->destroy();
}


bool IndexedAddress::has_fixed_offset(jint& fixed_offset) {
  if (index()->is_immediate()) {
    fixed_offset = (index()->as_int() << index_shift());
    return true;
  } else {
    return false;
  }
}

Assembler::Register IndexedAddress::fixed_register() {
  return array()->lo_register();
}

void IndexedAddress::fill_in_address_register() {
  if (index()->is_immediate()) {
    MemoryAddress::fill_in_address_register();
  } else {
    if (index_shift() != 0) {
      code_generator()->lsl_imm5(address_register(), index()->lo_register(),
                                 index_shift());
      code_generator()->add(address_register(), fixed_register(),
                                 address_register());
    } else {
      code_generator()->add(address_register(), fixed_register(),
                                 index()->lo_register());
    }
  }
}

void IndexedAddress::destroy_nonaddress_registers() {
  index()->destroy();
  array()->destroy();
}

inline bool LocationAddress::is_local( void ) const {
  return code_generator()->method()->is_local(index());
}


bool LocationAddress::has_fixed_offset(jint& fixed_offset) {
  int base_offset;
  int actual_index;

  CodeGenerator* const gen = code_generator();

#if ENABLE_ARM_V7
  if (is_local()) {
    // +--+--+--+---------+--+--+--+
    // |L0|L1|L2|frame    |E0|E1|E2|
    // +--+--+--+---------+--+--+--+
    //                           ^- frame()->stack_pointer()
    //
    // Note: in the example above, frame()->stack_pointer() == 5.
    //
    int offset_from_jsp =
        (gen->frame()->stack_pointer() - index()) * BytesPerStackElement +
        JavaFrame::frame_desc_size();
    if (!ENABLE_INCREASING_JAVA_STACK && ENABLE_FULL_STACK &&
        !gen->method()->access_flags().is_synchronized() &&
        !gen->method()->access_flags().has_monitor_bytecodes() &&
        offset_from_jsp < 0xff) {
      // We will try to use jsp (r9) to access the local if possible, so that
      // it can be done with one 16-bit instruction. We don't do it for
      // synchronization methods, which may have a variable frame size.
      fixed_offset = offset_from_jsp;
      _fixed_register = Assembler::jsp;
      return true;
    }
  }
#endif

  if (is_local()) {
    // The offset from the fp that would have it point at the end of the
    // locals block
    base_offset = JavaFrame::end_of_locals_offset();
    actual_index = gen->method()->max_locals() - 1 - index();
    _fixed_register = Assembler::fp;
  } else {
    // We need to make sure that we don't put something beyond
    // the current end of stack
    gen->ensure_sufficient_stack_for(index(), type());

    base_offset = 0;
    actual_index = gen->frame()->stack_pointer() - index();
    _fixed_register = Assembler::jsp;
  }
  fixed_offset = base_offset + JavaFrame::arg_offset_from_sp(actual_index);
  return true;
}

Assembler::Register LocationAddress::fixed_register() {
  GUARANTEE(_fixed_register != Assembler::no_reg, "must be initialized");
  return _fixed_register;
}

jint LocationAddress::get_fixed_offset() {
  jint fixed_offset;
  if (has_fixed_offset(fixed_offset)) {
    return fixed_offset;
  } else {
    SHOULD_NOT_REACH_HERE();
    return 0;
  }
}

#endif
