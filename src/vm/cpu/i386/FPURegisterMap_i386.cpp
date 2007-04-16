/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
#include "incls/_FPURegisterMap_i386.cpp.incl"

#if ENABLE_COMPILER

void FPURegisterMap::reset() {
  for (int i = Assembler::number_of_float_registers - 1; i > 0; i--) {
    set_register_at(i, Assembler::no_reg);
  }
  set_top_of_stack(0);
}

void FPURegisterMap::clear() {
  for (int i = top_of_stack(); i > 0; i--) {
    Compiler::code_generator()->ffree(register_at(i));
  }
  reset();
}

bool FPURegisterMap::is_on_stack(Assembler::Register reg) {
  for (int i = top_of_stack(); i > 0; i--) {
    if (register_at(i) == reg) return true;
  }
  return false;
}

bool FPURegisterMap::is_clearable() {
  for (int i = top_of_stack(); i > 0; i--) {
    if (RegisterAllocator::is_mapping_something(register_at(i))) return false;
  }
  return true;
}

int FPURegisterMap::index_for(Assembler::Register reg) {
  // Returns FPU stack index of reg (NOT index into _register_map[]!).
  for (int i = top_of_stack(); i > 0; i--) {
    if (register_at(i) == reg) return top_of_stack() - i;
  }
  SHOULD_NOT_REACH_HERE();
  return -1;
}

int FPURegisterMap::swap_with_top(Assembler::Register reg) {
  // Swaps reg with top of stack and returns index (before swap) of reg.
  for (int i = top_of_stack(); i > 0; i--) {
    if (register_at(i) == reg) {
      set_register_at(i, register_at(top_of_stack()));
      set_register_at(top_of_stack(), reg);
      return top_of_stack() - i;
    }
  }
  SHOULD_NOT_REACH_HERE();
  return -1;
}

#ifndef PRODUCT
void FPURegisterMap::dump(bool as_comment) {
  char str[1024];
  sprintf(str, "FPU: [");
  for (int i = top_of_stack(); i > 0; i--) {
    sprintf(str, "%s%s%s, ", str, Assembler::name_for_long_register(register_at(i)),
                             RegisterAllocator::is_referenced(register_at(i)) ? " (referenced)" : "");
  }
  sprintf(str, "%s]", str);
  if (as_comment) {
    Compiler::code_generator()->comment(str);
    return;
  }
  tty->print(str);
}
#endif

#endif
