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

#if ENABLE_COMPILER

class FPURegisterMap {
 public:
  typedef Assembler::Register Register;

 private:
  enum {
    size = Assembler::number_of_float_registers,
    item_bits = 4,

    empty_stack = 0,
    item_base = Assembler::fp0 - 1,   // 0 is undef value
    item_mask = (1 << item_bits) - 1,
    last_item_mask = item_mask << (item_bits*(size-1))
  };

  unsigned _stack;

  static unsigned encode ( const Register reg ) {
    GUARANTEE( Assembler::fp0 <= reg && reg <= Assembler::fp7, "Sanity" );
    return unsigned(reg) - item_base;
  }

  static Register decode ( const unsigned value ) {
    GUARANTEE( value != empty_stack, "FPU Register stack underflow" );
    return Register( (value & item_mask) + item_base );
  }

 public:
  void reset( void ) { _stack = empty_stack; }

  void clear( void );

  bool is_empty( void ) const { return _stack == empty_stack;  }

  Register top_of_stack_register( void ) const {
    return decode( _stack );
  }
  bool is_top_of_stack( const Register reg ) const {
    return top_of_stack_register() == reg;
  }

  bool is_unused( const Register reg) const {
    return !is_on_stack(reg);
  }

  unsigned is_on_stack(const Register reg) const ;

  void push( const Register reg ) {
    GUARANTEE( !(_stack & last_item_mask) , "FPU Register stack overflow" );
    _stack = encode( reg ) | (_stack << item_bits);
  }

  void pop( void ) {
    GUARANTEE( !is_empty(), "FPU Register stack underflow" );
    _stack >>= item_bits;
  }

  void pop( const Register reg ) {
    GUARANTEE(top_of_stack_register() == reg,
              "Can only pop register at top of stack");
    pop();
  }

  int index_for( const Register reg ) const;

  Register register_for( const int index ) const {
    GUARANTEE( (_stack >> (index*item_bits)) != 0, "Index out of bounds");
    return decode( _stack >> (index*item_bits) );
  }

  int swap_with_top(const Register reg) {
    const unsigned encoded_reg = encode( reg );
    // Swaps reg with top of stack and returns index (before swap) of reg.
    int i = 0;
    for( unsigned x = _stack; (x & item_mask) != encoded_reg; x >>= item_bits ) {
      GUARANTEE( x != empty_stack, "Sanity" );
      i++;
    }
    const int shift = i * item_bits;
    _stack = ((_stack &~item_mask) & ~(item_mask << shift)) | encoded_reg |
             ((_stack & item_mask) << shift );
    return i;
  }

#ifndef PRODUCT
  bool is_clearable( void ) const;
  void dump( const bool as_comment) const;
#endif
};

#endif
