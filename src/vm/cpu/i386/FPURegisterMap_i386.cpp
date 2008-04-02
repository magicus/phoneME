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
#include "incls/_FPURegisterMap_i386.cpp.incl"

#if ENABLE_COMPILER

void FPURegisterMap::clear( void ) {
  unsigned x = _stack;
  for( ; x != empty_stack; x >>= item_bits ) {
    Compiler::code_generator()->ffree( decode( x ) );
  }
  _stack = x;
}

unsigned FPURegisterMap::is_on_stack( const Assembler::Register reg ) const {
  const unsigned encoded_reg = encode( reg );
  unsigned x = _stack;
  for( ; x != empty_stack; x >>= item_bits ) {
    if( (x & item_mask) == encoded_reg ) break;
  }
  return x;
}

int FPURegisterMap::index_for(const Assembler::Register reg) const {
  const unsigned encoded_reg = encode( reg );
  int i = 0;
  for( unsigned x = _stack; (x & item_mask) != encoded_reg; x >>= item_bits ) {
    GUARANTEE( x != empty_stack, "Sanity" );
    i++;
  }
  return i;
}

#ifndef PRODUCT
bool FPURegisterMap::is_clearable( void ) const {
  for( unsigned x = _stack; x != empty_stack; x >>= item_bits ) {
    if( RegisterAllocator::is_mapping_something( decode( x ) ) ) return false;
  }
  return true;
}

void FPURegisterMap::dump( const bool as_comment ) const {
  char str[1024];
  sprintf(str, "FPU: [");
  for( unsigned x = _stack; x != empty_stack; x >>= item_bits ) {
    const Register reg = decode( x );
    sprintf(str, "%s%s%s, ", str, Assembler::name_for_long_register( reg ),
                             RegisterAllocator::is_referenced( reg ) ? " (referenced)" : "");
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
