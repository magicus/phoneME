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

// the generic address is a platform independant abstraction over addresses 
class GenericAddress: public StackObj {
 public:
  GenericAddress(BasicType type) : _type(type) { } 

  // virtuals for write barrier implementation
  virtual void write_barrier_prolog() { /* by default there's no write barrier */ }
  virtual void write_barrier_epilog() { /* by default there's no write barrier */ }

 protected:
  static inline VirtualStackFrame* frame ( void );

  static inline CodeGenerator* code_generator ( void ) {
    return _compiler_code_generator;
  }
  static inline Method* method ( void ) {
    return _compiler_method;
  }

  // default offsets to the low and high parts of the generic address
  virtual jint       lo_offset()      const { return 0 * BytesPerWord; }
  virtual jint       hi_offset()      const { return 1 * BytesPerWord; }

  // accessor for the type of this abstract address
  BasicType          type()           const { return _type; }
  BasicType          stack_type()     const { return stack_type_for(type()); }

  // checks if this abstract address is addressing two words
  bool               is_two_word()    const { return ::is_two_word(type()); }

 private:
  BasicType         _type; 
};

#endif
