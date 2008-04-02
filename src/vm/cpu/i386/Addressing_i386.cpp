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
#include "incls/_Addressing_i386.cpp.incl"

#if ENABLE_COMPILER

HeapAddress::~HeapAddress() {
  GUARANTEE(!has_address_register(), "address register must be cleared and deallocated");
}

void HeapAddress::write_barrier_prolog() {
  GUARANTEE(stack_type() == T_OBJECT, "write barrier should not be updated for non-object stores");

  // allocate an address register for the write barrier implementation
  set_address_register(RegisterAllocator::allocate());

  // compute the effective address and store it in the address register
  BinaryAssembler::Address address = compute_address_for(lo_offset());
  code_generator()->leal(address_register(), address);
}

void HeapAddress::write_barrier_epilog() {
  GUARANTEE(stack_type() == T_OBJECT, "write barrier should not be updated for non-object stores");
  GUARANTEE(has_address_register(),   "cannot update write barrier without proper register");

  // update the bit vector
  code_generator()->shrl(address_register(), LogBytesPerWord);
  code_generator()->bts(BinaryAssembler::Address((int) _bitvector_base), address_register());

  // dereference the allocated register and clear the cache
  RegisterAllocator::dereference(address_register());
  clear_address_register();
}

BinaryAssembler::Address HeapAddress::address_for(jint address_offset) {
  // use the address register if available
  if (has_address_register()) {
    GUARANTEE(address_offset == lo_offset(), "the address register holds the address for the low offset");
    return BinaryAssembler::Address(address_register());
  }

  // return the computed address
  return compute_address_for(address_offset);
}

BinaryAssembler::Address FieldAddress::compute_address_for(jint address_offset) {
  // [ base + displacement ]
  return BinaryAssembler::Address(object()->lo_register(), address_offset + offset());
}

BinaryAssembler::Address IndexedAddress::compute_address_for(jint address_offset) {
  if (index()->is_immediate()) {
    // [ base + displacement ]
    return BinaryAssembler::Address(array()->lo_register(), address_offset + Array::base_offset() + (index()->as_int() << index_shift()));
  } else {
    // [ base + index * scale + displacement ]
    BinaryAssembler::ScaleFactor scale_factor = (BinaryAssembler::ScaleFactor) index_shift();
    return BinaryAssembler::Address(array()->lo_register(), index()->lo_register(), scale_factor, address_offset + Array::base_offset());
  }
}

BinaryAssembler::Address StackAddress::address_for(jint address_offset) {
  // compute the Intel x86 address for this stack address
  jint offset = compute_base_offset();
  GUARANTEE(address_offset + offset >= 0, "must use non-negative offsets");
  return BinaryAssembler::Address(base(), address_offset + offset);
}

jint LocationAddress::compute_base_offset() {
  // compute the base offset for this location address
  if (is_local()) {
    const int max_locals = Compiler::root()->method()->max_locals();
    // bp + 8 acts like a stack pointer for the locals
    return 2 * sizeof(int)
        + JavaFrame::arg_offset_from_sp(max_locals - index() - 1);
  } else {
    code_generator()->ensure_sufficient_stack_for(index(), type());      
    return JavaFrame::arg_offset_from_sp(frame()->stack_pointer() - index());
  } 
}

bool LocationAddress::is_local_index(jint index) { 
  return Compiler::root()->method()->is_local(index); 
}

#endif
