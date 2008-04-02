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
#include "incls/_VirtualStackFrame_arm.cpp.incl"

#if ENABLE_COMPILER

bool VirtualStackFrame::flush_quick() {
  if (ENABLE_JAVA_STACK_TAGS) {
    return false;
  }

  int current_stack_ptr = stack_pointer();
  int virtual_stack_ptr = virtual_stack_pointer();

  if (current_stack_ptr >= virtual_stack_ptr) {
    // Not possible to use writeback addressing modes
    return false;
  }

  // This happens quite often, especially if we are making a series of
  // calls like:
  //
  //    iload_0
  //    invokestatic  void foo(int);
  //    iload_0
  //    iload_1
  //    invokestatic  void bar(int, int);
  //
  // During the second call, the top two VSF locations are in register
  // and must be flushed. The other VSF locations are already flushed
  // because of the first call. So (assuming we have downward-growing
  // full stack) we can use two pre-decrement STRs to store the top two
  // locations.
  //
  // IMPL_NOTE: consider using STM to save code footprint. However, doing
  // so might be slower than using individual STRs on Xscale

  {
    AllocationDisabler allocation_not_allowed;
    RawLocation *raw_location = raw_location_at(0);
    RawLocation *end = raw_location_end(raw_location);
    int index = 0;

    while (raw_location < end) {
      BasicType type = raw_location->type();
      if (is_two_word(type)) {
        return false; // uncommon. Just bail out.
      }
#if ENABLE_ARM_VFP
      if (type == T_FLOAT) {
        return false; // TEMP: fsts supports pre-indexing as well
      }
#endif
      bool changed = raw_location->is_changed();
      if (index <= current_stack_ptr) {
        if (changed) {
          return false;
        }
      } else {
        if (!changed) {
          return false;
        }
        if (!raw_location->in_register()) {
          return false;
        }
      }

      index ++;
      raw_location++;
    }
  }

  // Is this necessary?
  code_generator()->write_literals_if_desperate();

  {
    AllocationDisabler allocation_not_allowed;
    const Assembler::Register jsp = Assembler::jsp;
    const int offset = BytesPerStackElement * JavaStackDirection;

    RawLocation *raw_location = raw_location_at(0);
    RawLocation *end = raw_location_end(raw_location);
    int index = 0;

    while (raw_location < end) {
      if (index <= current_stack_ptr) {
        raw_location->mark_as_flushed();
      } else {
        Assembler::Register reg = raw_location->get_register();
        Assembler::Address2 addr;

        addr = Assembler::imm_index(jsp, offset, Assembler::pre_indexed);

        code_generator()->str(reg, addr);
        raw_location->mark_as_flushed();
      }
      index ++;
      raw_location++;
    }
  }

  set_real_stack_pointer(virtual_stack_ptr);
  return true;
}

#ifndef PRODUCT
void VirtualStackFrame::dump_fp_registers(const bool as_comment) const {
  // no floating point registers to dump
  (void)as_comment;
}
#endif // !PRODUCT

#endif // ENABLE_COMPILER
