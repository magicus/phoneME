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

class ClassParserState: public MixedOop {
 public:
  HANDLE_DEFINITION(ClassParserState, MixedOop);

  DEFINE_ACCESSOR_OBJ(ClassParserState, ClassParserState,next)
  DEFINE_ACCESSOR_OBJ(ClassParserState, Symbol,          class_name)
  DEFINE_ACCESSOR_OBJ(ClassParserState, TypeArray,       buffer)
  DEFINE_ACCESSOR_OBJ(ClassParserState, ConstantPool,    cp)
  DEFINE_ACCESSOR_OBJ(ClassParserState, TypeArray,       interface_indices)
  DEFINE_ACCESSOR_OBJ(ClassParserState, InstanceClass,   result)
                                      
  DEFINE_ACCESSOR_NUM(ClassParserState, int,             stage)
  DEFINE_ACCESSOR_NUM(ClassParserState, int,             super_class_index)
  DEFINE_ACCESSOR_NUM(ClassParserState, int,             access_flags)
  DEFINE_ACCESSOR_NUM(ClassParserState, int,             buffer_pos)
  DEFINE_ACCESSOR_NUM(ClassParserState, int,             major_version)
  DEFINE_ACCESSOR_NUM(ClassParserState, int,             minor_version)

 public:
  // Push new element onto the top of the stack
  void push(ClassParserState *new_top);
  // Push new element onto the top of the stack
  void push(Symbol *new_top_class_name JVM_TRAPS);
  // Remove top element from the stack
  void pop();

  ReturnOop top() {
    GUARANTEE(obj() != NULL, "stack cannot be empty");
    return obj();
  }

  static ReturnOop allocate(Symbol *class_name JVM_TRAPS);
};
