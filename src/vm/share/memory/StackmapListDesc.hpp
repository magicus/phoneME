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

class StackmapListDesc: public OopDesc {
 private:
  static jint header_size() { return sizeof(StackmapListDesc); }

  // Computes the allocation size
  static size_t allocation_size(int length) { 
    // each stackmap entry consists of a data integer/pointer
    return align_allocation_size(header_size() + length * sizeof(int));
  }
  
 public:
  // GC support.
  void variable_oops_do(void do_oop(OopDesc**));

  // Returns the object size
  size_t object_size() { return allocation_size(_entry_count); }

  // Returns if the given stackmap conforms to a shortmap format
  static bool is_shortmap_type(juint locals, juint stack_height, juint bci);
  
 private:
  // Initializes the object after allocation
  void initialize(OopDesc* klass, jint size);

  // Offset of the first stackmap entry
  unsigned int* stackmap_entries_base() {
    return ((unsigned int*) this) + (header_size()/sizeof(int));
  }

  // Number of stackmaps stored in this entry
  unsigned int _entry_count;

  // the stackmap entries for all available bci offsets
  // starting from here  

  /* layout of stackmap entries
   * map_integer|map_integer| ....
   * where map_integer in the case of a shortmap
   * is split as:
   * hi-16 bits for shortmap
   * lo-16 bits ==> map-type     :  1
   *                stack-height :  2
   *                locals       :  4
   *                bci          :  9
   * lowest bit indicates type(short/long map)
   * 
   * In the case of longmap, the entire integer is
   * a pointer to a TypeArray<short> object
   *
   */
public:
  static const jushort SHORT_ENTRY_FLAG;
  static const jushort SHORT_ENTRY_STACK_HEIGHT_MASK;
  static const jushort SHORT_ENTRY_STACK_HEIGHT_MAX;
  static const jushort SHORT_ENTRY_LOCALS_MAX; 
  static const jushort SHORT_STACKMAP_BITS_MAX;
  static const jushort SHORT_ENTRY_BCI_MAX;
  
  friend class StackmapList;
  friend class Universe;
  friend class OopDesc;

};
