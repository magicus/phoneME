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

// This file contains the shared part of the source and binary assembler.
// Ideally, this structure should match the structure for the ARM port.

#if ENABLE_COMPILER
class Assembler: public CompilerObject {
#else
class Assembler: public GlobalObj {
#endif
 public:
  enum Condition {
    // Order is relevant!
    overflow,
    no_overflow,
    below,
    above_equal,
    equal,
    not_equal,
    below_equal,
    above,
    negative,
    positive,
    parity,
    no_parity,
    less,
    greater_equal,
    less_equal,
    greater,

    number_of_conditions,

    // Alternative names
    zero        = equal,
    not_zero    = not_equal,
    carry_set   = below,
    carry_clear = above_equal,

    // pseudo condition, used for compatibility with ARM.
    always = number_of_conditions
  };

  enum Register {
    // Integer registers
    eax    =  0,
    ecx    =  1,
    edx    =  2,
    ebx    =  3,
    esp    =  4,
    ebp    =  5,
    esi    =  6,
    edi    =  7,

    // Fake floating point registers used by compiler
    fp0    =  8,
    fp1    =  9,
    fp2    = 10,
    fp3    = 11,
    fp4    = 12,
    fp5    = 13,
    fp6    = 14,
    fp7    = 15,

    // Byte registers
    ah,
    ch,
    dh,
    bh,
    al,
    cl,
    dl,
    bl,

    // for platform-independant code
    return_register = eax,
    stack_lock_register = edx,

    // Useful constants
    no_reg = -1,

    first_register = eax,
//    last_register  = fp7,
    last_register = bl,

    first_allocatable_register = eax,
  };

  enum {
    first_int_register   = eax,
    last_int_register    = edi,

//    first_byte_register  = eax,
//    last_byte_register   = ebx,
    first_byte_register  = ah,
    last_byte_register   = bl,

    first_float_register = fp0,
    last_float_register  = fp7,

    number_of_float_registers = last_float_register - first_float_register + 1,
//    number_of_registers      = last_float_register + 1
    number_of_registers      = last_byte_register + 1
  };

  enum {
    instruction_alignment = 1,
  };

  // Test that a given register is indeed valid.
  static bool is_valid_int_register(Register reg) {
    return (reg >= (Register)first_int_register  && 
            reg <= (Register)last_int_register);
  }
  static bool is_valid_byte_register(Register reg) {
    return (reg >= (Register)eax && 
            reg <= (Register)ebx || reg >= (Register)first_byte_register && 
            reg <= (Register)last_byte_register);
  }
  static bool is_valid_register(Register reg) {
    return reg >= first_register      && reg <= last_register;
  }

  static Register register_from_encoding(int encoding) { return (Register) encoding; }

  // for platform-independant code
  static Register reg(Register r)               { return r; }

#ifndef PRODUCT
  // Name accessors.
  static const char* name_for_byte_register(const Register reg);
  static const char* name_for_work_register(const Register reg);
  static const char* name_for_long_register(const Register reg);
#endif

  enum ScaleFactor {
    no_scale = -1,
    times_1  =  0,
    times_2  =  1,
    times_4  =  2,
    times_8  =  3
  };
};

class Macros: public Assembler {
};
