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

// ArrayDesc is abstract super class for all arrays.

class ArrayDesc: public JavaDesc {
 protected:
  // Number of elements in the array
  int _length;
  static jint header_size() { return sizeof(ArrayDesc); }

 private:
  // Computes the allocation size
  static size_t allocation_size(int length, int scale) {
    GUARANTEE(length >= 0, "Cannot allocate array of negative length");
    return align_allocation_size(header_size() + length * scale);
  }

 
 public:
  // Returns the size of the object
  size_t object_size(int scale) {
    return allocation_size(_length, scale);
  }

  void *data() {
    return (char *) this + header_size();
  }

 private:
  // Initializes the object after allocation
  void initialize(OopDesc* klass, int length) {
    OopDesc::initialize(klass);
    _length = length;
  }

  void reinitialize(OopDesc* klass, int length) {
    OopDesc::reinitialize(klass);
    _length = length;
  }

  friend class Array;
  friend class ObjectHeap;
  friend class Universe;
  friend class Inflater;
#if ENABLE_COMPILER
  friend class CodeGenerator;
#endif
  friend class FarClassDesc;
#if ENABLE_JAVA_DEBUGGER
  friend class JavaDebugger;
#endif
#if ENABLE_OOP_TAG
  friend class Task;
#endif
};

class TypeArrayDesc: public ArrayDesc {};
