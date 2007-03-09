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

#if ENABLE_COMPILER

class FPURegisterMap: public TypeArray {
 public:
  HANDLE_DEFINITION(FPURegisterMap, TypeArray);

  void reset();
  void clear();

  bool is_empty()                             { return top_of_stack() == 0;         }
  Assembler::Register top_of_stack_register() { return register_at(top_of_stack()); }

  bool is_top_of_stack(Assembler::Register reg) {
    return (top_of_stack_register() == reg);
  }

  bool is_unused(Assembler::Register reg) { return !is_on_stack(reg); }
  bool is_on_stack(Assembler::Register reg);
  bool is_clearable();

  void push(Assembler::Register reg) {
    GUARANTEE(top_of_stack() < Assembler::number_of_float_registers, "FPU Register stack overflow");

    set_top_of_stack(top_of_stack() + 1);
    set_register_at(top_of_stack(), reg);
  }

  void pop() {
    GUARANTEE(top_of_stack() > 0, "FPU Register stack underflow");

    set_register_at(top_of_stack(), Assembler::no_reg);
    set_top_of_stack(top_of_stack() - 1);
  }

  void pop(Assembler::Register reg) {
    GUARANTEE(register_at(top_of_stack()) == reg, "Can only pop register at top of stack");
    pop();
  }

  int index_for(Assembler::Register reg);

  Assembler::Register register_for(int index) {
    GUARANTEE(0 < index && index <= top_of_stack(), "Index out of bounds");

    return register_at(top_of_stack() - index);
  }

  int swap_with_top(Assembler::Register reg);

#ifndef PRODUCT
  void dump(bool as_comment);
#endif

 private:
  int top_of_stack() const                                 { return int_at(0);                          }
  void set_top_of_stack(int index)                         { int_at_put(0, index);                      }
  Assembler::Register register_at(int index) const         { return (Assembler::Register)int_at(index); }
  void set_register_at(int index, Assembler::Register reg) { int_at_put(index, (jint)reg);              }
};

#endif
