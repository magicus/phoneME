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

// Labels are used to refer to (abstract) machine code locations.
// They encode a location >= 0 and a state (free, bound, linked).
// If code-relative locations are used (e.g. offsets from the
// start of the code, Labels are relocation-transparent, i.e.,
// the code can be moved around even during code generation (GC).

// Usage of Labels
//
// free  : label has not been used yet
// bound : label location has been determined, label position is
//          corresponding code offset 
// linked: label location has not been determined yet, label position is code
//          offset of last instruction referring to the label
//
// Label positions are always code offsets in order to be (code) relocation
// transparent.  Linked labels point to a chain of (linked) instructions that
// eventually need to be fixed up to point to the bound label position. Each
// instruction refers to the previous instruction that also refers to the
// same label. The last instruction in the chain (i.e., the first instruction
// to refer to the label) refers to itself.  
// 
// These instructions use pc-relative addressing and thus are relocation
// transparent.

class BinaryLabel {
  int _encoding;

  void check(const int position) const {
    (void)position;
    GUARANTEE(position >= 0, "illegal position"); 
  }

 public:
  int encoding ( void ) const { return _encoding; }

  // manipulation
  void unuse( void ) { _encoding = 0; }
  void link_to(const int position) { 
    check(position); 
    _encoding = - position - 1; 
  }
  void bind_to(const int position) { 
    check(position); 
    _encoding = position + 1; }

  // accessors
  static int position( const int encoding ) {
    return abs(encoding) - 1; 
  } // -1 if free label
  int position( void ) const { 
    return position( _encoding ); 
  }

  bool is_unused( void ) const { return _encoding == 0; }
  bool is_linked( void ) const { return _encoding <  0; }
  bool is_bound ( void ) const { return _encoding >  0; }

  // creation/destruction
  BinaryLabel( const int value = 0 ): _encoding( value ) {}
 ~BinaryLabel( void ) { /* do nothing for now */ }

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void print_value_on ( Stream* ) const;
  void p( void ) const;
#endif
  
  friend class CompilationQueueElement;
  friend class Compiler;
  friend class Entry;
};

#endif
