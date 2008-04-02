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
#include "incls/_LiteralPoolElement.cpp.incl"

#if USE_LITERAL_POOL
#if !defined(PRODUCT) || USE_COMPILER_COMMENTS

void LiteralPoolElement::print_value_on( Stream* s ) const {
#if USE_COMPILER_COMMENTS
  s->print("<LiteralPoolElement: ");
  if (is_bound()) { 
    s->print("bci=%d, ", bci());
  } else { 
    s->print("unbound, ", bci());
  }
  Oop::Raw oop = literal_oop();
  int imm32     = literal_int();
  if (oop.is_null()) { 
    s->print("imm32=%d >", imm32);
  } else { 
    s->print("immoop=[");
    oop.print_value_on(s);
    if (imm32 != 0) { 
      s->print(" + %d", imm32);
    }
    s->print("] >");
  }
#endif
}

// This method is called by MixedOop::iterate() after iterating the
// header part of MixedOop
void LiteralPoolElement::iterate(OopVisitor* visitor) {
#if 0 // IMPL_NOTE: need to revisit        
  if (literal_oop() != NULL) {
    NamedField id("oop", true);
    visitor->do_oop(&id, literal_oop_offset(), true);
  }
  if (is_bound()) { 
    NamedField id("bound bci", true);
    visitor->do_int(&id, bci_offset(), true);
  }
  if (literal_oop() == NULL) { 
    NamedField id("imm32", true);
    visitor->do_int(&id, literal_int_offset(), true);
  }
  {
    NamedField id("label", true);
    visitor->do_int(&id, label_offset(), true);
  }

  // IMPL_NOTE: use OopPrinter API
  {
    BinaryLabel lab = label();
    lab.print_value_on(tty);
  }
#endif
}

#endif

#endif
