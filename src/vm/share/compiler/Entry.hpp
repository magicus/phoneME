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

class Entry: public CompilerObject {
  VirtualStackFrame* _frame;
  int                _bci;
  int                _label;
  int                _code_size;

public:
  static Entry* allocate(jint bci, VirtualStackFrame* frame, 
                 const BinaryAssembler::Label label, jint code_size JVM_TRAPS)
  { // Clone the virtual stack frame by default.
    VirtualStackFrame* frame_clone =
      frame->clone(JVM_SINGLE_ARG_ZCHECK_0(frame_clone));

    Entry* entry = COMPILER_OBJECT_ALLOCATE(Entry);
    if( entry ) {
      entry->set_bci( bci );
      entry->set_frame( frame_clone );
      entry->set_label( label );
      entry->set_code_size( code_size );
    }
    return entry;
  }

  // Field accessors
  jint bci( void ) const { return _bci; }
  void set_bci( const jint value ) { _bci = value; }

  VirtualStackFrame* frame( void ) const { return _frame; }
  void set_frame(VirtualStackFrame* value) { _frame = value; }

  BinaryAssembler::Label label( void ) const {
    BinaryAssembler::Label L;
    L._encoding = _label;
    return L;
  }
  void set_label(const BinaryAssembler::Label value) {
    _label = value._encoding;
  }

  jint code_size( void ) const { return _code_size; }
  void set_code_size(const jint value) { _code_size = value; }
};

class EntryArray: CompilerPointerArray {
public:
  typedef Entry* element_type;
  typedef EntryArray array_type;

  const element_type* base( void ) const {
    return (const element_type*) CompilerPointerArray::base();
  }
  element_type* base( void ) {
    return (element_type*) CompilerPointerArray::base();
  }

  const element_type& at ( const int i ) const {   
    return (const element_type&) CompilerPointerArray::at( i );
  }
  element_type& at ( const int i ) {   
    return (element_type&) CompilerPointerArray::at( i );
  }
  void at_put ( const int i, const element_type val ) {   
    CompilerPointerArray::at_put( i, val );
  }

  static array_type* allocate( const int n JVM_TRAPS ) {
    return (array_type*) CompilerPointerArray::allocate( n JVM_NO_CHECK );
  }
};

#endif /* ENABLE_COMPILER */
