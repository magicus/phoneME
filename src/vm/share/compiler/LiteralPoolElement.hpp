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

#if USE_LITERAL_POOL

class LiteralPoolElement: public MixedOop {
public:
  HANDLE_DEFINITION(LiteralPoolElement, MixedOop);

  enum { not_yet_defined_bci = LiteralPoolElementDesc::not_yet_defined_bci };

  static size_t allocation_size( void ) {
    return LiteralPoolElementDesc::allocation_size();
  }
  static size_t pointer_count( void ) {
    return LiteralPoolElementDesc::pointer_count();
  }        

public:
  // To avoid endless lists of friends the static offset computation
  // routines are all public.
  static jint literal_oop_offset() {
    return FIELD_OFFSET(LiteralPoolElementDesc, _literal_oop);
  }
  static jint next_offset() {
    return FIELD_OFFSET(LiteralPoolElementDesc, _next);
  }
  static jint bci_offset() {
    return FIELD_OFFSET(LiteralPoolElementDesc, _bci);
  }
  static jint label_offset() {
    return FIELD_OFFSET(LiteralPoolElementDesc, _label);
  }
  static jint literal_int_offset() {
    return FIELD_OFFSET(LiteralPoolElementDesc, _literal_int);
  }

  int bci( void ) const { return int_field(bci_offset()); }
  void set_bci(const int i) { int_field_put(bci_offset(), i); }

  int literal_int( void ) const { return int_field(literal_int_offset()); }
  void set_literal_int(const int i) { int_field_put(literal_int_offset(), i); }

  BinaryLabel label( void ) const {
    const int label_value = int_field(label_offset());
    const BinaryLabel l( label_value );
    return l;
  }
  int label_position( void ) const {
    return BinaryLabel::position( int_field(label_offset()) );
  }

  void set_label(const BinaryLabel value) {
    int_field_put(label_offset(), value.encoding() );
  }

  ReturnOop next( void ) const  {
    return (ReturnOop)int_field(next_offset());
  }
  void set_next(OopDesc* p) {
    int_field_put(next_offset(), int(p));
  }
  void set_next(const Oop* oop) {
    set_next( oop->obj() );
  }

  ReturnOop literal_oop() const {
    return obj_field(literal_oop_offset());
  }
  void set_literal_oop(OopDesc* p) {
    obj_field_put(literal_oop_offset(), p);
  }
  void set_literal_oop(const Oop *oop) {
    set_literal_oop( oop->obj() );
  }
  
  bool is_bound() const {
    return ((LiteralPoolElementDesc*)obj())->is_bound();
  }

public:
  static ReturnOop allocate(const Oop* oop, int imm32 JVM_TRAPS);
    
#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
  void print_value_on(Stream*s) const;

  void iterate(OopVisitor* /*visitor*/);
#endif
};

#endif
