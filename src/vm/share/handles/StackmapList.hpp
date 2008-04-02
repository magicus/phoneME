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

class StackmapList: public Oop {
 public:
  HANDLE_DEFINITION(StackmapList, Oop);
  ~StackmapList() {}

 public:
  // To avoid endless lists of friends the static offset computation
  // routines are all public.
  static int entry_count_offset() {
    return FIELD_OFFSET(StackmapListDesc, _entry_count);
  }

  void print_value_on(Stream* /*st*/) PRODUCT_RETURN;
  void print_entry_on(Stream* /*st*/, int /*index*/, bool /*new_lines*/)
                      PRODUCT_RETURN;
  void iterate(OopVisitor* /*visitor*/) PRODUCT_RETURN;
  static void iterate_oopmaps(oopmaps_doer /*do_map*/, void* /*param*/)
                      PRODUCT_RETURN;

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif
  
  static int entry_stat_offset(int index) {
    return entry_count_offset() + sizeof(int) * (index + 1);
  }

 private:
  inline bool is_valid(int index) {
    juint entry = uint_field(entry_stat_offset(index));
    return (entry != 0);
  }

 public:

  int entry_count() {
    return int_field(entry_count_offset());
  }
    
  inline bool is_short_map(int index) { 
    juint entry = uint_field(entry_stat_offset(index));
    return (bool)((entry & StackmapListDesc::SHORT_ENTRY_FLAG));
  }

  unsigned short get_short_map(unsigned int index) { 
    juint entry = uint_field(entry_stat_offset(index));
    return (unsigned short)(entry >> 16);
  }

  void set_short_map(int index, unsigned short bitmap) { 
    GUARANTEE(index < entry_count(),
              "StackmapList: Illegal attempt to expand entry");
    juint entry = uint_field(entry_stat_offset(index));
    entry |= StackmapListDesc::SHORT_ENTRY_FLAG; 
    entry &= 0xFFFF;
    entry |= (bitmap << 16);
    uint_field_put(entry_stat_offset(index), entry);
    GUARANTEE(is_short_map(index), "StackmapList:Invalid shortmap entry");
    GUARANTEE(get_short_map(index) == bitmap, 
              "StackmapList:Invalid short map set");
  }

  ReturnOop get_long_map(int index) { 
    return obj_field(entry_stat_offset(index)); 
  }

  void set_long_map(int index, TypeArray* bitmap) { 
    GUARANTEE(index < entry_count(),
              "StackmapList: Illegal attempt to expand entry");
    obj_field_put(entry_stat_offset(index), bitmap);
    GUARANTEE(!is_short_map(index), "StackmapList:Invalid long map");
    GUARANTEE(get_long_map(index) == bitmap->obj(), 
              "StackmapList:Invalid long map set");
  }

  void update(int index, jushort count, jushort height, jushort bci);

  unsigned short get_stack_height(int index) {
    if (is_short_map(index)) {
      juint entry = uint_field(entry_stat_offset(index));
      return (unsigned short)((entry >> 1) & 
                              StackmapListDesc::SHORT_ENTRY_STACK_HEIGHT_MASK);
    }else {
      TypeArray::Raw longmap = obj_field(entry_stat_offset(index)); 
      return longmap().ushort_at(1);
    }
  }

  void set_stack_height(int index, unsigned short height) {
    if (is_short_map(index)) {
      juint entry = uint_field(entry_stat_offset(index));
      GUARANTEE(height <= 0x3, "Invalid map type chosen in set_stack_height");
      entry |= (height & StackmapListDesc::SHORT_ENTRY_STACK_HEIGHT_MASK) << 1;
      uint_field_put(entry_stat_offset(index), entry);
      GUARANTEE(get_stack_height(index) == height,
                "StackmapList:Invalid short stack height");
    } else {
      TypeArray longmap(obj_field(entry_stat_offset(index))); 
      longmap.ushort_at_put(1, height);
      GUARANTEE(get_stack_height(index) == height,
                "StackmapList:Invalid long map stack height");
    }
  }

  unsigned short get_locals(int index) { 
    if (is_short_map(index)) {
      juint entry = uint_field(entry_stat_offset(index));
      // knock out stackmap and bci
      entry &= 0x7F;
      return (unsigned short)(entry >> 3);
    } else {
      TypeArray::Raw longmap = obj_field(entry_stat_offset(index)); 
      return longmap().ushort_at(0);
    }
  }

  void set_locals(int index, unsigned short count) {
    if (is_short_map(index)) {
      juint entry = uint_field(entry_stat_offset(index));
      GUARANTEE(count <= 0xF, "Invalid map type chosen in set_locals");
      entry |= (count << 3);
      uint_field_put(entry_stat_offset(index), entry);
      GUARANTEE(get_locals(index) == count,
                "StackmapList:Invalid short locals");    
    } else {
      TypeArray longmap(obj_field(entry_stat_offset(index))); 
      longmap.ushort_at_put(0, count);
      GUARANTEE(get_locals(index) == count, 
                "StackmapList:Invalid long map locals");
    }
  } 

  unsigned short get_bci(int index) { 
    if (is_short_map(index)) {
      juint entry = uint_field(entry_stat_offset(index));
      // knock out stackmap
      entry &= 0xFFFF;
      return (unsigned short)(entry >> 7);  
    }else {
      TypeArray::Raw longmap = obj_field(entry_stat_offset(index)); 
      return longmap().ushort_at(2);
    }
  }

  void set_bci(int index, unsigned short bci) {
    if (is_short_map(index)) {
      juint entry = uint_field(entry_stat_offset(index));
      // clear out existing bci
      entry &= 0xFFFF007F;
      // set new bci
      entry |= (bci << 7);
      uint_field_put(entry_stat_offset(index), entry);
      GUARANTEE(get_bci(index) == bci, "StackmapList:set_bci failed");
    }  else {
      TypeArray longmap(obj_field(entry_stat_offset(index))); 
      longmap.ushort_at_put(2, bci);
      GUARANTEE(get_bci(index) == bci, "StackmapList:Invalid long map bci");
    }
  }

  static int header_size() {
    return sizeof(StackmapListDesc);
  }

 private:
  void print_bitmap_internal(Stream* st, unsigned int bitmap, int index, 
                             int count, bool new_lines);
};
