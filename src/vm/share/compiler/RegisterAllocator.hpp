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

#if ENABLE_COMPILER

class RegisterAllocator {
  enum {
    number_of_registers = Assembler::number_of_registers
  };

  static inline CodeGenerator* code_generator ( void ) {
    return (CodeGenerator*)_compiler_state;
  }

 public:
  typedef Assembler::Register Register;

  // Allocate a free 32-bit register.
  static Register allocate();

  // Allocate a free 32-bit register without spilling.
  static Register allocate_or_fail();

  // Allocate a particular 32-bit register.
  static Register allocate(const Register reg);

  // Allocate a free byte register.
  static Register allocate_byte_register(void);

  // Allocate a free float register.
  static Register allocate_float_register(void);

#if ENABLE_ARM_VFP
  // Allocate a free float register.
  static Register allocate_double_register(void);
#endif

  // Spill a specific register with the given encoding.
  static void spill(const Register reg);

  // Get the number of references for a specific register.
  static int references(const Register reg) {
    return _register_references[reg];
  }

  // Accessors for updating reference counts.
  static void reference(const Register reg) {
    _register_references[reg]++;
  }
  static void dereference(const Register reg) {
#if defined(ARM) || defined(HITACHI_SH)
    GUARANTEE(reg < Register(number_of_registers), "sanity");
#else
    // IMPL_NOTE: This check might be required on x86 platform (at least
    // once upon a time) because of existence of half- and
    // byte-registers. Consider whether this is needed anymore.
    if (reg >= Register(number_of_registers)) {
      return;
    }
#endif
    GUARANTEE(is_referenced(reg), "Dereferencing unreferenced register");
    _register_references[reg]--;
  }

  // Check if a given register is referenced or mapping something.
  static bool is_referenced(const Register reg) {
    return references(reg) > 0;
  }
  static bool is_mapping_something(const Register reg);

  // Initialize the register allocator.
  static void initialize(void);

  // Debugging support
  static void print() PRODUCT_RETURN;
  static void guarantee_all_free() PRODUCT_RETURN;

  static bool has_free(int count, bool spill = false) {
      return has_free(count, _next_register_table, _next_allocate, spill);
  }

  static bool has_free(int count, const Register* next_table,
                                            Register next, bool spill = false);

  static Register next_for(Register reg) {
    return _next_register_table[reg];
  }
  static bool is_allocatable(Register reg) {
    return _next_register_table[reg] != Assembler::no_reg;
  }
 private:
  // Reference counts for registers.
  static int _register_references[number_of_registers];
  static void initialize_register_references( void ) {
    memset( _register_references, 0, sizeof _register_references );
  }

  static const Register* _next_register_table;      // CPU-dependent
  static const Register* _next_byte_register_table; // CPU-dependent

  // Next round-robin allocation attempt
  static Register _next_allocate;
  static Register _next_byte_allocate;
  static Register _next_float_allocate;

  // Next round-robin spill attempt
  static Register _next_spill;
  static Register _next_byte_spill;
  static Register _next_float_spill;

  // Allocate any suitable register in range.
  static Register allocate(const Register* next_table, Register& next_alloc,
                           Register& next_spill);

  // Allocate any suitable register in range.
  // Returns no_reg if no registers are available without spilling.

  friend class Compiler;
  friend class VSFMergeTest;

  static Register allocate_or_fail(const Register* next_table, Register& next);

  // Spill any suitable register in range, and return the chosen register.
  // Returns no_reg if all registers are referenced.
  static Register spill(const Register* next_table, Register& next);

#if ENABLE_ARM_VFP
  static Assembler::Register next_vfp_register(Register r, const unsigned step) {
    GUARANTEE(Assembler::is_vfp_register(r), "Register is not a VFP register");
    r = (Register)(r - (Assembler::s0 - step));
    r = (Register)(r & 31);
    r = (Register)(r + Assembler::s0);
    return r;
  }
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  static const char* register_name( const Assembler::Register reg ) {
#if ARM || defined(HITACHI_SH)
    return Disassembler::register_name(reg);
#else
    return Assembler::name_for_long_register(reg);
#endif
  }
#endif
};

#endif
