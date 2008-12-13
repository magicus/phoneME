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

#if USE_LITERAL_POOL
class LiteralPoolElement: public CompilerObject {
public:
  // BCI indicating this literal is still unbound.
  enum { not_yet_defined_bci = 0x7fffffff };
        
  OopDesc*              _literal_oop;
  LiteralPoolElement*   _next;
  jint                  _bci;
  jint                  _label;
  jint                  _literal_int;

  // Note that the bci() field is used as a convenience.
  // If the label is unbound, then bci() == 0x7fffffff
  // If the label is bound, then bci()  is the same as the label's position.

  bool is_bound( void ) const {
    return _bci != not_yet_defined_bci;
  }

  bool matches(const OopDesc* oop, const jint imm32) const {
    return oop == _literal_oop && imm32 == _literal_int;
  }

  int bci( void ) const { return _bci; }
  void set_bci( const int i ) { _bci = i; }

  int literal_int( void ) const { return _literal_int; }
  void set_literal_int(const int i) { _literal_int = i; }

  BinaryLabel label( void ) const {
    const BinaryLabel l( _label );
    return l;
  }
  int label_position( void ) const {
    return BinaryLabel::position( _label );
  }
  void set_label(const BinaryLabel value) {
    _label = value.encoding();
  }

  LiteralPoolElement* next( void ) const  {
    return _next;
  }
  void set_next(LiteralPoolElement* p) {
    _next = p;
  }

  ReturnOop literal_oop( void ) const {
    return _literal_oop;
  }
  void set_literal_oop(OopDesc* p) {
    _literal_oop = p;
  }
  void set_literal_oop(const Oop *oop) {
    set_literal_oop( oop->obj() );
  }

  void oops_do( void do_oop(OopDesc**) ) {
    do_oop( &_literal_oop );
  }
  
  static LiteralPoolElement* allocate(OopDesc* obj, int imm32 JVM_TRAPS) {
    LiteralPoolElement* literal = COMPILER_OBJECT_ALLOCATE(LiteralPoolElement);
    if( literal ) {
      literal->set_literal_int(imm32);
      literal->set_literal_oop(obj);
      literal->set_bci(not_yet_defined_bci);
    }
    return literal;
  }
    
#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
  void print_value_on(Stream*s) const;

  void iterate(OopVisitor* /*visitor*/);
#endif
};

#endif
