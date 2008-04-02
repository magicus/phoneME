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

// An Array is the abstract class for all array handles.

class Array: public JavaOop {
 public:
  HANDLE_DEFINITION(Array, JavaOop);
  ~Array() {}
  // To avoid endless lists of friends the static offset computation routines are all public.
  static int base_offset()   { return ArrayDesc::header_size(); } 
  static int length_offset() { return FIELD_OFFSET(ArrayDesc, _length); }
  
  // Returns the length
  jint length() const { return int_field(length_offset()); }

  // Tells whether index is within bounds.
  bool is_within_bounds(int index) const {
    return (0 <= index) && (index < length());
  }

  ArrayDesc* array() { return (ArrayDesc*) obj(); };

  // The address of the first array element.
  address base_address(void) const {
    return ((address)obj()) + base_offset();
  }

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif
  
#if ENABLE_REFLECTION
  ReturnOop shrink(int new_length);
#endif

private:
  void set_length(int length) {
    int_field_put(length_offset(), length);
  }
friend class ConstantPoolRewriter;
friend class ROMTools;
friend class ROMOptimizer;
};
