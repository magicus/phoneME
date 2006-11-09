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

#include "incls/_precompiled.incl"
#include "incls/_RegisterAllocator.cpp.incl"

#if ENABLE_COMPILER
Assembler::Register*RegisterAllocator::_next_register_table      = NULL;
Assembler::Register*RegisterAllocator::_next_byte_register_table = NULL;

Assembler::Register RegisterAllocator::_next_allocate;
Assembler::Register RegisterAllocator::_next_byte_allocate;
Assembler::Register RegisterAllocator::_next_float_allocate;

Assembler::Register RegisterAllocator::_next_spill;
Assembler::Register RegisterAllocator::_next_byte_spill;
Assembler::Register RegisterAllocator::_next_float_spill;

bool RegisterAllocator::is_mapping_something(Assembler::Register reg) {
  return Compiler::current()->frame()->is_mapping_something(reg) ||
    (Compiler::current()->conforming_frame()->not_null() &&
     Compiler::current()->conforming_frame()->is_mapping_something(reg));
}

Assembler::Register RegisterAllocator::allocate() {
  return allocate(_next_register_table, _next_allocate, _next_spill);
}

#ifndef ARM
Assembler::Register RegisterAllocator::allocate_byte_register() {
  return allocate(_next_byte_register_table, _next_byte_allocate, 
                  _next_byte_spill);
}

Assembler::Register RegisterAllocator::allocate_float_register() {
  return allocate(_next_register_table, _next_float_allocate,_next_float_spill);
}
#endif // !ARM

Assembler::Register 
RegisterAllocator::allocate(Assembler::Register* next_table,
    Assembler::Register& next_alloc, Assembler::Register& next_spill) {
  Register reg = allocate_or_fail(next_table, next_alloc);
  if (reg == Assembler::no_reg) {
#if ENABLE_REMEMBER_ARRAY_LENGTH && ARM
    //This make the allocate allocate bound register until all other 
    // registers are in use. 
    if (Compiler::current()->frame()->get_bound_register() != 0 &&
        Compiler::current()->frame()->get_bound_access_count() == 0 ) {
      reg = Compiler::current()->frame()->get_bound_register();
      Compiler::current()->frame()->set_bound_mask(0);
      reference(reg);
#if ENABLE_CSE
      clear_notation(reg);
#endif

      return reg;
    }
#endif    
    // Spill any suitable register.
    reg = spill(next_table, next_spill);
    // Make sure we got ourselves a proper register.
    GUARANTEE(reg != Assembler::no_reg, "Cannot allocate register when all registers are referenced");
    // Setup a reference to the newly allocated register.
    reference(reg);
  }
  // Return the register.
#if ENABLE_CSE
      clear_notation(reg);
#endif

  return reg;
}

Assembler::Register RegisterAllocator::allocate_or_fail() {
  return allocate_or_fail(_next_register_table, _next_allocate);
}

Assembler::Register RegisterAllocator::allocate(Assembler::Register reg) {
  // For now we allow registers that aren't handled by the register allocator
  // to be allocated. This might seem a bit strange.
  if (reg < (Assembler::Register)Assembler::number_of_registers) {
    GUARANTEE(!is_referenced(reg), "Cannot allocate referenced register");
    // Setup a reference to the newly allocated register.
    reference(reg);
    // Spill the register.
    spill(reg);
#if ENABLE_CSE
    clear_notation(reg);
#endif
    
  }
  // Return the register.
  return reg;
}

void RegisterAllocator::spill(Assembler::Register reg) {
  if (is_mapping_something(reg)) {
    // Spill register into memory.
    Compiler::current()->frame()->spill_register(reg);
  }
}

Assembler::Register RegisterAllocator::allocate_or_fail(Assembler::Register* next_table, Assembler::Register& next) {
 
#if ENABLE_CSE
  Register second = Assembler::no_reg; 
#endif

  // Use a round-robin strategy to allocate the registers.
  const Register current = next;
  do {
    next = next_table[next];
    // Check to see whether or not the register is available for allocation.
    if (!is_referenced(next) && !is_mapping_something(next)) {
      // Setup a reference to the newly allocated register.

#if ENABLE_CSE      
      if (!is_notated(next)) {
#endif

      reference(next);
      // Return the register.
      return next;
      
#if ENABLE_CSE
      } else if ( second == Assembler::no_reg) {
        second = next;
      }
#endif      

    } 
  } while(next != current);
  // Couldn't allocate a register without spilling.

#if ENABLE_CSE
  if ( second != Assembler::no_reg ) {
    next = second;
    reference(next);
    return next;
  }
#endif

  return Assembler::no_reg;
}

Assembler::Register RegisterAllocator::spill(Assembler::Register* next_table, Assembler::Register& next) {
  // Use a round-robin strategy to spill the registers.
  const Register current = next;
  do {
    next = next_table[next];
    // Check to see whether or not the register is available for spilling.
#if ENABLE_INLINE && ARM
    if ((!is_referenced(next)) && !(Compiler::current()->conforming_frame()->not_null() &&
     Compiler::current()->conforming_frame()->is_mapping_something(next)) ){
#else
    if (!is_referenced(next)) {
#endif
      // Spill the register.
      spill(next);
      // Return the register.
      return next;
    } 
  } while(next != current);
  // Couldn't allocate a register without spilling.
  return Assembler::no_reg;
}

bool RegisterAllocator::has_free(int count, 
                                 Assembler::Register* next_table,
                                 Assembler::Register next, bool spill) {
  const Register current = next;
  do {
    next = next_table[next];
    // Check to see whether or not the register is available for allocation.
    if (!is_referenced(next) && (spill || !is_mapping_something(next))) {
      count--;
      if (count == 0) return true;
    } 
  } while(next != current);
  // Couldn't allocate a register without spilling.
  return false;
}

#if ENABLE_CSE
void RegisterAllocator::kill_locals(const jint local_index) {
  const RegisterNotation *table = _register_notation_table;
  if ( local_index < RegisterNotation::max_local_index ) {
    jint mask = (1<<local_index);
    for (Assembler::Register reg = Assembler::first_register;
         reg <= Assembler::last_register;
         reg = (Assembler::Register) ((int) reg + 1)) {
      if ((_notation_map & (1<<reg)) && 
          (table[reg].locals & mask) !=0) {
        COMPILER_COMMENT(("kill notation[%s] by local_index%d", 
                          Disassembler::reg_name(reg), local_index));
        clear_notation(reg);
      } 
    }
  }
}

void RegisterAllocator::kill_constant(const jint constant_index) {
  const RegisterNotation *table = _register_notation_table;
  if ( constant_index < RegisterNotation::max_const_index ) {
    jint mask = (1<<constant_index);
    for (Assembler::Register reg = Assembler::first_register;
         reg <= Assembler::last_register;
         reg = (Assembler::Register) ((int) reg + 1)) {
      if ((_notation_map & (1<<reg) ) &&
          (table[reg].constants & mask) !=0) {
        COMPILER_COMMENT(("kill notation[%s] by put field%d", 
                          Disassembler::reg_name(reg), constant_index));
        clear_notation(reg);
      }
    }
  }
}

void RegisterAllocator::kill_array_type(const jint array_element_type) {
  const RegisterNotation *table = _register_notation_table;
  if (array_element_type != 0) {
    for (Assembler::Register reg = Assembler::first_register;
         reg <= Assembler::last_register;
         reg = (Assembler::Register) ((int) reg + 1)) {
      if ((_notation_map & (1<<reg)) && 
          (table[reg].array_element_type & array_element_type) != 0 ) {
        COMPILER_COMMENT(("kill notation[%s] by array store%d", 
                          Disassembler::reg_name(reg), array_element_type));
        clear_notation(reg);
      }
    }
  }
}
#endif


#ifndef PRODUCT

#if ENABLE_CSE
void RegisterAllocator::dump_notation() {
  for (Assembler::Register reg = Assembler::first_register;
    reg <= Assembler::last_register;
    reg = (Assembler::Register) ((int) reg + 1)) {
    dump_notation(reg);  
  }
}

void RegisterAllocator::dump_notation(const Register reg){
  const RegisterNotation *table = _register_notation_table;
  jint cur_notation;
  cur_notation = table[reg].notation;
  if (cur_notation != 0) {
    jint offset = ( cur_notation >> 16 ) & 0xffff ;
    jint length = cur_notation & 0xffff ; 
    COMPILER_COMMENT(("notation[%s]", Disassembler::reg_name(reg)));  
    COMPILER_COMMENT(("    bci start  = %d, length = %d", offset, length));
    COMPILER_COMMENT(("    local deps = 0x%08x", table[reg].locals));
    COMPILER_COMMENT(("    field deps = 0x%08x", table[reg].constants));
    COMPILER_COMMENT(("    array deps = 0x%08x", table[reg].array_element_type));
    COMPILER_COMMENT(("    value type = 0x%08x", table[reg].type));
    COMPILER_COMMENT(("    index      =  %s",   (_status_checked & (1<<reg))!=0 ?
                                                "checked":"not checked"));
  }
}
#endif

void RegisterAllocator::print() {
#if USE_DEBUG_PRINTING
  for (Assembler::Register reg = Assembler::first_register;
       reg <= Assembler::last_register;
       reg = (Assembler::Register) ((int) reg + 1)) {
    if (is_referenced(reg)) {
#if ARM || defined(HITACHI_SH)
      const char* name = Disassembler::reg_name(reg);
#else
      const char* name = Assembler::name_for_long_register(reg);
#endif
      TTY_TRACE_CR(("register_references[%s] = %d", name, references(reg)));
    }    
  }
#endif
}

void RegisterAllocator::guarantee_all_free() {
  for (Assembler::Register reg = Assembler::first_register;
       reg <= Assembler::last_register;
       reg = (Assembler::Register) ((int) reg + 1)) {
    if (is_referenced(reg)) {
#if ARM || defined(HITACHI_SH)
      const char* name = Disassembler::reg_name(reg);
#else
      const char* name = Assembler::name_for_long_register(reg);
#endif
      TTY_TRACE_CR(("register %s has not been released:", name));
      print();
      SHOULD_NOT_REACH_HERE();
    }
  }
}

#endif // !PRODUCT

#endif
