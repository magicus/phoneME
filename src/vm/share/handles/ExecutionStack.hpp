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

class ExecutionStack: public Oop { 
 public:
  HANDLE_DEFINITION_CHECK(ExecutionStack, Oop);

  static int length_offset() { 
    return FIELD_OFFSET(ExecutionStackDesc, _length); 
  }
  static int thread_offset() { 
    return FIELD_OFFSET(ExecutionStackDesc, _thread); 
  }

 public:
  // Returns the length
  jint length() const { return int_field(length_offset()); }

  // thread accessors
  ReturnOop thread() const { 
    return obj_field(thread_offset()); 
  }
  void set_thread(Thread* value) { 
    obj_field_put(thread_offset(), (Oop*) value); 
  }
  void clear_thread() { obj_field_clear(thread_offset()); }
 
  ReturnOop shrink_in_place(jint new_length);

  // Printing
#ifndef PRODUCT
  void iterate(OopVisitor* visitor);
  void print_value_on(Stream* st);
#endif

private:
  void set_length(jint length) { int_field_put(length_offset(), length); }

};
