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

#include "incls/_precompiled.incl"
#include "incls/_RegisterAllocator.cpp.incl"

#if ENABLE_COMPILER
const Assembler::Register* RegisterAllocator::_next_register_table;
const Assembler::Register* RegisterAllocator::_next_byte_register_table;

Assembler::Register RegisterAllocator::_next_allocate;
Assembler::Register RegisterAllocator::_next_byte_allocate;
Assembler::Register RegisterAllocator::_next_float_allocate;

Assembler::Register RegisterAllocator::_next_spill;
Assembler::Register RegisterAllocator::_next_byte_spill;
Assembler::Register RegisterAllocator::_next_float_spill;

bool RegisterAllocator::is_mapping_something(const Assembler::Register reg) {
  const CompilerState* state = _compiler_state;
  return state->frame()->is_mapping_something(reg) ||
         ( state->conforming_frame() != NULL &&
           state->conforming_frame()->is_mapping_something(reg) );
}

Assembler::Register RegisterAllocator::allocate( void ) {
  return allocate(_next_register_table, _next_allocate, _next_spill);
}

#ifndef ARM
Assembler::Register RegisterAllocator::allocate_byte_register( void ) {
  return allocate(_next_byte_register_table, _next_byte_allocate,
                  _next_byte_spill);
}
#endif


Assembler::Register RegisterAllocator::allocate_float_register() {
#if ENABLE_ARM_VFP
#if 0
  // This change improve the caching of locals in registers.
  // But the more value that are cached in registers the more
  // values need to be written into memory at method call.
  // Find the best fit
  {
    const Register start = Register(_next_float_allocate & ~1);
    Register next = next_vfp_register(start, (Assembler::number_of_float_registers - 8));
    do {
      const bool f0 = !is_referenced(Register(next+0)) && !is_mapping_something(Register(next+0));
      const bool f1 = !is_referenced(Register(next+1)) && !is_mapping_something(Register(next+1));
      if ((f0 + f1) == 1) {
      Register r = next;
      if ( f0 == false) {
        r = Register(next + 1);
        }

       reference(r);

       return r;
      }
      next = next_vfp_register(next, 2);
    } while (next != start);
  }
#endif

  // Find a free register
  {
    const Register start = _next_float_allocate;
    Register next = start;

    do {
      if( !is_referenced(next) && !is_mapping_something(next)) {

        reference(next);

        _next_float_allocate = next_vfp_register(next, 1);
        return next;
      }
      next = next_vfp_register(next, 1);
    } while (next != start);
  }

  // Nothing free spill registers
  {
    const Register start = _next_float_spill;
    Register next = start;
    do {
      if (is_referenced(next)) {
        continue;
      }
#if ENABLE_INLINE
      if (Compiler::current()->conforming_frame() &&
          Compiler::current()->conforming_frame()->is_mapping_something(next)) {
        continue;
      }
#endif // ENABLE_INLINE
      spill(next);

      reference(next);
      _next_float_spill = next_vfp_register(next, 1);
      return next;

    } while ((next = next_vfp_register(next, 1)) != start);
  }
  GUARANTEE(false, "Cannot allocate VFP registers for a float");
  return Assembler::no_reg;
#else  // !ENABLE_ARM_VFP
  return allocate(_next_register_table, _next_float_allocate, _next_float_spill);
#endif // ENABLE_ARM_VFP
}

#if ENABLE_ARM_VFP
Assembler::Register RegisterAllocator::allocate_double_register() {
  {
    const Register start = Register( next_vfp_register( _next_float_allocate, 1) & ~1);
    Register next = start;

    do {
      if (!is_referenced(Register(next + 0)) &&
        !is_referenced(Register(next + 1)) &&
          !is_mapping_something(Register(next + 0)) &&
        !is_mapping_something(Register(next + 1))) {

      reference(Register(next + 0));
        reference(Register(next + 1));

        _next_float_allocate = next_vfp_register(next, 2);
        return next;
      }
      next = next_vfp_register(next, 2);
    } while (next != start);
  }

#if 0
  // This change improve the caching of locals in registers.
  // But the more value that are cached in registers the more
  // values need to be written into memory at method call.
  // Try and find a half filled register pair
  {
    const Register start = Register((_next_float_spill) & ~1);
    Register next = next_vfp_register(start, 2);
    Register end = next_vfp_register(start, 8);
    do {
      if (is_referenced(Register(next+0)) || is_referenced(Register(next+1))) {
        continue;
      }
      const bool f0 = is_mapping_something(Register(next+0));
      const bool f1 = is_mapping_something(Register(next+1));

      if ((f0 + f1) == 1) {
        spill( f0 ? next : Register( next + 1) );

        reference(Register(next + 0));
        reference(Register(next + 1));
        _next_float_spill = next_vfp_register(next, 2);
      return next;
      }
      next = next_vfp_register(next, 2);
    } while (next != end);
  }
#endif

  // Nothing free spill registers
  {
    const Register start = Register( next_vfp_register( _next_float_spill, 1) & ~1);
    Register next = start;
    do {
      if (is_referenced(Register(next+0)) || is_referenced(Register(next+1))) {
        continue;
      }
#if ENABLE_INLINE
      {
        const VirtualStackFrame* frame = Compiler::current()->conforming_frame();
        if( frame &&
            (frame->is_mapping_something(Register(next+0)) ||
             frame->is_mapping_something(Register(next+1)))) {
          continue;
        }
      }
#endif
      spill(Register(next+0));
      spill(Register(next+1));

      reference(Register(next+0));
      reference(Register(next+1));
      _next_float_spill = next_vfp_register(next, 2);
      return next;

    } while ((next = next_vfp_register(next, 2)) != start);
  }
  GUARANTEE(false, "Cannot allocate VFP registers for a double");
  return Assembler::no_reg;
}
#endif // ENABLE_ARM_VFP

Assembler::Register
RegisterAllocator::allocate(const Assembler::Register* next_table,
    Assembler::Register& next_alloc, Assembler::Register& next_spill) {
  Register reg = allocate_or_fail(next_table, next_alloc);
  if (reg == Assembler::no_reg) {
    reg = Compiler::current()->frame()->try_to_free_length_register();
    if ( reg != Assembler::no_reg) {
      return reg;
    }
    // Spill any suitable register.
    reg = spill(next_table, next_spill);
    // Make sure we got ourselves a proper register.
    GUARANTEE(reg != Assembler::no_reg, "Cannot allocate register when all registers are referenced");
    // Setup a reference to the newly allocated register.
    reference(reg);
  }

  // Return the register.
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

#define REFERENCE_AND_RETURN(reg)         reference(reg);\
        return reg;
Assembler::Register
RegisterAllocator::allocate_or_fail(const Assembler::Register* next_table,
                                    Assembler::Register& next) {
  // Use a round-robin strategy to allocate the registers.
  const Register current = next;
  do {
    next = next_table[next];
    // Check to see whether or not the register is available for allocation.
    if (!is_referenced(next) && !is_mapping_something(next)) {
      // Setup a reference to the newly allocated register.
      REFERENCE_AND_RETURN(next);
    }
  } while(next != current);
  // Couldn't allocate a register without spilling.

  return Assembler::no_reg;
}
#undef REFERENCE_AND_RETURN

Assembler::Register
RegisterAllocator::spill(const Assembler::Register* next_table,
                         Assembler::Register& next) {
  // Use a round-robin strategy to spill the registers.
  const Register current = next;
  do {
    next = next_table[next];
    // Check to see whether or not the register is available for spilling.
    if (!is_referenced(next)) {
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
                                 const Assembler::Register* next_table,
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

#ifndef PRODUCT
void RegisterAllocator::print( void ) {
#if USE_DEBUG_PRINTING
  for( Assembler::Register reg = Assembler::first_register;
       reg <= Assembler::last_register;
       reg = Assembler::Register(reg + 1) ) {
    if (is_referenced(reg)) {
      TTY_TRACE_CR(("register_references[%s] = %d",
                    register_name(reg), references(reg)));
    }
  }
#endif
}

void RegisterAllocator::guarantee_all_free( void ) {
  for( Assembler::Register reg = Assembler::first_register;
       reg <= Assembler::last_register;
       reg = Assembler::Register(reg + 1) ) {
    if (is_referenced(reg)) {
      TTY_TRACE_CR(("register %s has not been released:", register_name(reg) ));
      print();
      SHOULD_NOT_REACH_HERE();
    }
  }
}

#endif // !PRODUCT

#endif
