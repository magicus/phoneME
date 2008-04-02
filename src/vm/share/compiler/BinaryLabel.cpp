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
# include "incls/_BinaryLabel.cpp.incl"

#if ENABLE_COMPILER && (!defined( PRODUCT ) || ENABLE_TTY_TRACE)
void BinaryLabel::print_value_on(Stream* s) const {
  s->print("encoding = %d (0x%x)", _encoding, _encoding);
  if (is_unused()) {
    s->print_cr(" unused");
  } else {
    const int pos = position();
    const address p = Compiler::code_generator()->addr_at(pos);
    if (is_linked()) {
      s->print_cr(" linked, pos=%d, addr=0x%x", pos, p);
    } else {
      GUARANTEE(is_bound(), "sanity");
      s->print_cr(" bound, pos=%d, addr=0x%x",  pos, p );
    }
  }
}
void BinaryLabel::p( void ) const {
  print_value_on(tty);
}
#endif
