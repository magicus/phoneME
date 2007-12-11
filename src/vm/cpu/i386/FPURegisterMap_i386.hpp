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

#if ENABLE_COMPILER

class FPURegisterMap {
 private:
  enum { size = Assembler::number_of_float_registers };
  signed char _stack[ size ];
  signed char _top;
 public:
  typedef Assembler::Register Register;

  void reset( void ) {
    set_top_of_stack( 0 );
  }

  void clear( void );

  bool is_empty( void ) const {
    return top_of_stack() == 0;
  }
  Register top_of_stack_register( void ) const {
    return register_at( top_of_stack() );
  }
  bool is_top_of_stack(const Register reg) const {
    return _top > 0 && top_of_stack_register() == reg;
  }

  bool is_unused( const Assembler::Register reg) const {
    return !is_on_stack(reg);
  }

  bool is_on_stack(const Assembler::Register reg) const ;
  bool is_clearable( void ) const;

  void push( const Assembler::Register reg ) {
    GUARANTEE(top_of_stack() < size, "FPU Register stack overflow");

    set_top_of_stack( top_of_stack() + 1 );
    set_register_at( top_of_stack(), reg );
  }

  void pop( void ) {
    GUARANTEE( top_of_stack() > 0, "FPU Register stack underflow" );
    set_top_of_stack(top_of_stack() - 1);
  }

  void pop( const Assembler::Register reg ) {
    GUARANTEE(top_of_stack_register() == reg, "Can only pop register at top of stack");
    pop();
  }

  int index_for( const Assembler::Register reg) const;

  Assembler::Register register_for(int index) const {
    GUARANTEE(0 < index && index <= top_of_stack(), "Index out of bounds");

    return register_at(top_of_stack() - index);
  }

  int swap_with_top(const Assembler::Register reg);

#ifndef PRODUCT
  void dump( const bool as_comment) const;
#endif

 private:
  int top_of_stack( void ) const {
    return _top;
  }
  void set_top_of_stack( const int index ) {
    _top = index;
  }
  Register register_at ( const int index) const {
    GUARANTEE( unsigned( index - 1 ) < size, "FPU stack index out of bounds" );
    return Register(_stack[index-1]);
  }
  void set_register_at ( const int index, const Register reg ) {
    GUARANTEE( unsigned(index - 1) < size, "FPU stack index out of bounds" );
    GUARANTEE( Assembler::fp0 <= reg && reg <= Assembler::fp7, "Sanity" );
    _stack[ index-1 ] = (signed char)reg;
  }
};

#endif
