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

# include "incls/_precompiled.incl"
# include "incls/_Symbols.cpp.incl"

#if !ROMIZED_PRODUCT

int system_symbols[Symbols::__number_of_system_symbols];

#define VM_SYMBOL_REGULAR(name, string)    {string, false},
#define VM_SYMBOL_SIGNATURE(name, string)  {string, true},

struct SymbolDefinition {
  const char * string;
  bool   is_signature;
};

static const SymbolDefinition definitions[] = {
  VM_SYMBOLS_DO(VM_SYMBOL_REGULAR, VM_SYMBOL_SIGNATURE)
};

void Symbols::initialize(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  Symbol::Fast sym;
  for (int i = 0; i < number_of_system_symbols(); i++) {
    char* string = (char*)definitions[i].string;
    if (definitions[i].is_signature) {
      sym = TypeSymbol::parse(string JVM_CHECK);
    } else {
      sym = SymbolTable::symbol_for(string JVM_CHECK);
    }
    *at(i) = sym;
  }
}

void Symbols::oops_do(void do_oop(OopDesc**)) {
  for (int index = 0; index < number_of_system_symbols(); index++) {
    do_oop((OopDesc**)(&system_symbols[index]));
  }
}
#endif

#ifndef PRODUCT

bool Symbols::is_system_symbol(Symbol* symbol) {
  for (int i = 0; i < number_of_system_symbols(); i++) {
    if (symbol->equals(at(i))) {
      return true;
    }
  }
  return false;
}

#endif
