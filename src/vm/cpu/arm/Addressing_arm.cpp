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

#include "incls/_precompiled.incl"

#if !ENABLE_THUMB_COMPILER && ENABLE_COMPILER
#include "incls/_Addressing_arm.cpp.incl"

MemoryAddress::~MemoryAddress() {
  // dereference any allocated address registers
  if (has_address_register()) {
      RegisterAllocator::dereference(address_register());
  }
}

Assembler::Address2 MemoryAddress::address_2_for(jint address_offset) {
  if (!has_address_register()) {
    // Try to do direct access
    jint fixed_offset;
    if (has_fixed_offset(fixed_offset)) {
      jint offset = fixed_offset + base_offset() + address_offset;
      if (-(1 << 12) < offset && offset < (1 << 12)) {
        return Assembler::imm_index(fixed_register(), offset);
      }
    }
    create_and_initialize_address_register();
  }
  GUARANTEE(has_address_register(), "We must have address register by now");
  int xbase_offset =            // base_offset or 0
      address_register_includes_base_offset() ? 0 : base_offset();
  if (address_offset == 0 && xbase_offset != 0) { 
      // Update the address_register so that it includes the base_offset
      set_address_register_includes_base_offset();
      return Assembler::imm_index(address_register(), xbase_offset, 
                                  Assembler::pre_indexed);
  } else { 
      return Assembler::imm_index(address_register(), 
                                  address_offset + xbase_offset);
  }
}

Assembler::Address3 MemoryAddress::address_3_for(jint address_offset) {
  if (!has_address_register()) {
    jint fixed_offset;
    if (has_fixed_offset(fixed_offset)) {
      jint offset = fixed_offset + base_offset() + address_offset;
      if (-(1 << 8) < offset && offset < (1 << 8)) {
        return Assembler::imm_index3(fixed_register(), offset);
      }
    }
    // We have to allocate an address register and fill it in
    create_and_initialize_address_register();
  }
  GUARANTEE(has_address_register(), "We must have address register by now");
 
  if (!address_register_includes_base_offset()) {
    address_offset += base_offset();
  }
  // Unlike Address2, we have nothing to gain by using pre_indexed mode.
  // This is not used for modifying objects, longs, or doubles.
  return Assembler::imm_index3(address_register(), address_offset);
}

Assembler::Address5 MemoryAddress::address_5_for(jint address_offset) {
  if (!has_address_register()) {
    // Try to do direct access
    jint fixed_offset;
    if (has_fixed_offset(fixed_offset)) {
      jint offset = fixed_offset + base_offset() + address_offset;
      if (-(1 << 10) < offset && offset < (1 << 10)) {
        return Assembler::imm_index5(fixed_register(), offset);
      }
    }
    create_and_initialize_address_register();
  }
  GUARANTEE(has_address_register(), "We must have address register by now");
  int xbase_offset =            // base_offset or 0
      address_register_includes_base_offset() ? 0 : base_offset();
#if 1
  return Assembler::imm_index5(address_register(), 
                               address_offset + xbase_offset);
#else
  // ??
  if (address_offset == 0 && xbase_offset != 0) { 
      // Update the address_register so that it includes the base_offset
      set_address_register_includes_base_offset();
      return Assembler::imm_index5(address_register(), xbase_offset, 
                                   Assembler::pre_indexed);
  } else { 
      return Assembler::imm_index5(address_register(), 
                                   address_offset + xbase_offset);
  }
#endif
}


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
    code_generator()->add_imm(address_register(), 
                              fixed_register(),
                              fixed_offset + base_offset());
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

  // allocate the necessary temporary registers
  Assembler::Register tmp0;

  GUARANTEE(base_offset() == 0 || address_register_includes_base_offset(), 
            "write_barrier_epilog() must follow address_2_for(0)");

  // This is almost always the last thing we do with an address, so it
  // is okay to steal its temporary register.  This saves us one or two
  // instructions in many cases.
  tmp0 = address_register();
  clear_address_register();

  Assembler::Register tmp1 = RegisterAllocator::allocate();
  Assembler::Register tmp2 = RegisterAllocator::allocate();
  Assembler::Register tmp3 = RegisterAllocator::allocate();
  
  // update the write barrier
  code_generator()->oop_write_barrier(tmp0, tmp1, tmp2, tmp3, false);
  
  // dereference the allocated registers
  RegisterAllocator::dereference(tmp0);
  RegisterAllocator::dereference(tmp1);
  RegisterAllocator::dereference(tmp2);
  RegisterAllocator::dereference(tmp3);
}

bool FieldAddress::has_fixed_offset(jint& fixed_offset) {
  fixed_offset = offset(); 
  return true;
}

Assembler::Register FieldAddress::fixed_register() { 
  return object()->lo_register();
}

FieldAddress::FieldAddress(Value& object, jint offset, BasicType type) : 
      HeapAddress(type), _object(&object), _offset(offset) { }

void FieldAddress::destroy_nonaddress_registers() {
  object()->destroy();
}


IndexedAddress::IndexedAddress(Value& array, Value& index, BasicType type) : 
    HeapAddress(type), _array(&array), _index(&index) { } 

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
    code_generator()->add(address_register(), 
                          fixed_register(),
                          Assembler::imm_shift(index()->lo_register(),
                                               Assembler::lsl, index_shift()));
  }
}

void IndexedAddress::destroy_nonaddress_registers() {
  index()->destroy();
  array()->destroy();
}

LocationAddress::LocationAddress(jint index, BasicType type) : 
     MemoryAddress(type), _index(index) {}

bool LocationAddress::has_fixed_offset(jint& fixed_offset) {
  int base_offset;
  int actual_index;

  if (code_generator()->omit_stack_frame()) {
    // Everything is accessed using jsp
    actual_index = frame()->stack_pointer() - index();
    fixed_offset = JavaFrame::arg_offset_from_sp(actual_index);     
  } else {
    // With a stack frame: locals are accessed using fp
    //                     stacks are accessed using jsp
    if (is_local()) {
      // The offset from the fp that would have it point at the end of the
      // locals block 
      base_offset = JavaFrame::end_of_locals_offset();
      actual_index = Compiler::root()->method()->max_locals() - 1 - index();
    } else {
      if (Assembler::jsp == Assembler::sp) {
        // We need to make sure that we don't put something beyond
        // the current end of stack
        code_generator()->ensure_sufficient_stack_for(index(), type());
      } 
      base_offset = 0;
      actual_index = frame()->stack_pointer() - index();
    }
    fixed_offset = base_offset + JavaFrame::arg_offset_from_sp(actual_index);
  }
  return true;
}
    
Assembler::Register LocationAddress::fixed_register() {
  if (code_generator()->omit_stack_frame()) {
    return Assembler::jsp;
  } else {
    return is_local() ? Assembler::fp : Assembler::jsp;
  }
}

jint LocationAddress::get_fixed_offset() {
  jint fixed_offset;
  if (!has_fixed_offset(fixed_offset)) { 
    SHOULD_NOT_REACH_HERE();
  }
  return fixed_offset;
}

inline bool LocationAddress::is_local() const { 
  return Compiler::root()->method()->is_local(index()); 
}

#endif /*#if !ENABLE_THUMB_COMPILER && ENABLE_COMPILER */
