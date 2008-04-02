/*
 *
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
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
//CSE
#if ENABLE_CSE
//One notation attached to one register. A notation contains the information about
//the byte codes that the value in the register is calculated from.
class RegisterNotation {
   jint  notation; // high 16bits stand for begin bci of string, low 16bits stand for string length
   jint  locals;//bitmaps of the dependency between the register and local variable from 0~31,
                    //if the register dependency on local variable whose index is larger than 31,
                    //the byte codes expression won't be a CSE candidate.
   jint  constants;//bitmaps of the dependency between register and constant pool index from 0~31.
   jint  array_element_type;//bitmaps of dependency between register and array type.
   jint  type;//the type of the value which is cached in the register
 public:
   RegisterNotation():
    notation(0),locals(0),constants(0),type(0),array_element_type(0)
    {  }
 friend class RegisterAllocator;
 enum  {
  length_bits = 16,
  length_mask = 0xffff,
  index_bits = 16,
  index_mask = 0xffff0000,
  max_local_index = 32,
  max_const_index = 32,
 };
};
#endif
class RegisterAllocator {        
  enum {
    number_of_registers = Assembler::number_of_registers
  };

  static inline CodeGenerator* code_generator ( void ) {
    return _compiler_code_generator;
  }

 public:
  typedef Assembler::Register Register;

  //CSE
#if ENABLE_CSE

  //clean the notation of all register
  static inline void wipe_all_notations() {
    memset(_register_notation_table, 0,
           sizeof(RegisterNotation) * number_of_registers);
    _notation_map = 0;
   _status_checked = 0;
  }

  //clean the check status of all register
  //this is usually called when the bound mark is updated.
  static inline void clear_check_status() {
    _status_checked = 0;
  }

  //set the byte code notation of the register
  static inline void set_notation(const Register reg, const jint offset,
                                  const jint length) {
    _register_notation_table[reg].notation =
        ((offset << RegisterNotation::length_bits) | length);
    _notation_map |= (1<< (jint)reg);
  }

  //move the notation from src register to dest.
  static void move_notation(const Register src, const Register dest) {
    _register_notation_table[dest].notation =
       _register_notation_table[src].notation;
    _register_notation_table[dest].locals =
       _register_notation_table[src].locals;
    _register_notation_table[dest].constants =
       _register_notation_table[src].constants;
    _register_notation_table[dest].type =
       _register_notation_table[src].type;
    _register_notation_table[dest].array_element_type=
       _register_notation_table[src].array_element_type;
    jint mask = (1<<dest);
    _notation_map |= mask;
    if ((_status_checked & (1<<src) ) != 0 ) {
      _status_checked |= mask;
    }
    wipe_notation_of(src);
  }

  //get the notation of the register
  static void get_notation(const Register reg, jint& begin_bci, jint& end_bci) {
    jint notation = _register_notation_table[reg].notation;
    begin_bci = ( notation  >> RegisterNotation::length_bits );
    end_bci = begin_bci + (notation & RegisterNotation::length_mask) -1;
  }

  //get the value type
  static inline jint get_type(const Register reg) {
    return _register_notation_table[reg].type;
  }

  //mark the dependency between reg and locals
  static inline void set_locals(const Register reg, const jint locals) {
    _register_notation_table[reg].locals = locals;
  }

  //mark the dependency between reg and constants
  static inline void set_constants(const Register reg, const jint constants) {
   _register_notation_table[reg].constants = constants;
  }

  //set the type of value stored in the register
  static inline void set_type(const Register reg, const BasicType type) {
    _register_notation_table[reg].type =  type;
  }

  //set the dependency between reg and array type
  static inline void set_array_element_type(const Register reg, const jint type) {
    _register_notation_table[reg].array_element_type=
      type;
  }

  //mark the value stored in the register as checked with the array boundary which
  //is cached in the bound mark.
  static inline void set_check_status(const Register reg) {
  _status_checked |= (1<<reg);
  }

  //check whether the value of the register is array boundary checked before
  static inline jint get_check_status(const Register reg) {
    return (_status_checked & (1<<reg));
  }

  //clean the notation of that register
  static inline void wipe_notation_of(const Register reg) {
    RegisterNotation* notation = &_register_notation_table[reg];
    notation->constants = 0;
    notation->locals = 0;
    notation->notation = 0;
    notation->type = 0;
    jint mask =( 1 << ((jint) reg));
    _notation_map &=  (~mask);
    _status_checked &=  (~mask);
  }

  //wipe all the register notations except the one appears in the bitmap
  static inline void wipe_all_notation_except(const jint bitmap) {
    for (Assembler::Register reg = Assembler::first_register;
         reg <= Assembler::last_register;
         reg = (Assembler::Register) ((int) reg + 1)) {
      jint index = (jint) reg;
      if ((bitmap & (1<< index)) == 0) {
        wipe_notation_of( reg );
      }
    }
    _notation_map &= bitmap;
    _status_checked &= bitmap;
  }

  //check whether the register is notated before.
  static inline bool is_notated() {
    if (_notation_map != 0) {
      return true;
    }
    return false;
  }

  //check the value of the register has been array boundary checked
  //before.
  static inline bool is_checked_before(const Register reg) {
    return is_notated(reg)&& (get_check_status(reg) !=0 );
  }

  //set the value of the register is checked.
  static inline void set_checked_before(const Register reg) {
    if(is_notated(reg)){
      set_check_status(reg);
    };
  }

  //check whether the register is notated
  static inline bool is_notated(const Register reg) {
    if ((_notation_map & (1<<reg)) != 0) {
      return true;
    }
    return false;
  }

  //wipe the notation who is depend on the dirty local
  static void kill_by_locals(const jint local_index);

  //wipe the notation who is depend on the  dirty field
  static void kill_by_fields(const jint constant_index);

  //wipe the notation who is depend on the dirty array access
  //we assume the access of same array element type as the access
  //of same array.
  static void kill_by_array_type(const jint array_element_type);

#ifndef PRODUCT
  static void dump_notation();
  static void dump_notation(Register reg);
#else
  static void dump_notation() {}
  static void dump_notation(Register reg){}
#endif
#else
  static inline void wipe_notation_of(const Register reg) {}
  static void move_notation(const Register reg, const Register new_reg) {}
  static inline void clear_check_status() {}
  static inline bool is_checked_before(const Register reg) {return false;}
  static inline void set_checked_before(const Register reg) {}
#endif // ENABLE_CSE

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

  static bool has_free(int count, Register* next_table,
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

  static Register *_next_register_table;      // CPU-dependent
  static Register *_next_byte_register_table; // CPU-dependent

  // Next round-robin allocation attempt
  static Register _next_allocate;
  static Register _next_byte_allocate;
  static Register _next_float_allocate;

  // Next round-robin spill attempt
  static Register _next_spill;
  static Register _next_byte_spill;
  static Register _next_float_spill;

  //CSE
#if ENABLE_CSE
  //global notation table of each register
  static RegisterNotation _register_notation_table[number_of_registers];

  //bitmap of indicating the notation status of each register
  //for quick access without iteratoring the notation table.
  static jint _notation_map;

  //bitmap of indicating the value store in the register is
  //array length checked with the latest array length caching
  static jint _status_checked;
#endif

  // Allocate any suitable register in range.
  static Register allocate(Register* next_table, Register& next_alloc,
                           Register& next_spill);

  // Allocate any suitable register in range.
  // Returns no_reg if no registers are available without spilling.

  friend class Compiler;
  friend class VSFMergeTest;

  static Register allocate_or_fail(Register* next_table, Register& next);

  // Spill any suitable register in range, and return the chosen register.
  // Returns no_reg if all registers are referenced.
  static Register spill(Register* next_table, Register& next);

#if ENABLE_ARM_VFP
  static Assembler::Register next_vfp_register(Register r, const unsigned step) {
    GUARANTEE(Assembler::is_vfp_register(r), "Register is not a VFP register");
    r = (Register)(r - (Assembler::s0 - step));
    r = (Register)(r & 31);
    r = (Register)(r + Assembler::s0);
    return r;  
  }
#endif

};

#endif
