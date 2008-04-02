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

#if USE_LITERAL_POOL
class LiteralPoolElementDesc: public MixedOopDesc {
public:
  // BCI indicating this literal is still unbound.
  enum { not_yet_defined_bci = 0x7fffffff };
        
  OopDesc*                      _literal_oop;
  LiteralPoolElementDesc*       _next;
  jint                          _bci;
  jint                          _label;
  jint                          _literal_int;

  // Note that the bci() field is used as a convenience.
  // If the label is unbound, then bci() == 0x7fffffff
  // If the label is bound, then bci()  is the same as the label's position.

  static int pointer_count( void ) {
    return 1;
  }
  static size_t allocation_size( void ) { 
    return align_allocation_size( sizeof( LiteralPoolElementDesc ) );
  }

  bool is_bound( void ) const {
    return _bci != not_yet_defined_bci;
  }

  bool matches(const OopDesc* oop, const jint imm32) const {
    return oop == _literal_oop && imm32 == _literal_int;
  }

  friend class LiteralPoolElement;
};
#endif
