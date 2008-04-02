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

# include "incls/_precompiled.incl"
# include "incls/_StackmapList.cpp.incl"

void StackmapList::update(int index, jushort count, jushort height, jushort bci)
{
  if (is_short_map(index)) {
    juint entry = uint_field(entry_stat_offset(index));
    GUARANTEE(height <= 0x3, "Invalid map type chosen in set_stack_height");
    GUARANTEE(count <= 0xF, "Invalid map type chosen in set_locals");

    entry |= ((height & StackmapListDesc::SHORT_ENTRY_STACK_HEIGHT_MASK) << 1) |
             (count << 3);

    // clear out existing bci (IMPL_NOTE: is this necessary?)
    entry &= 0xFFFF007F;
    // set new bci
    entry |= (bci << 7);

    uint_field_put(entry_stat_offset(index), entry);

    GUARANTEE(get_stack_height(index) == height,
              "StackmapList:Invalid short stack height");
    GUARANTEE(get_locals(index) == count,
              "StackmapList:Invalid short locals");    
    GUARANTEE(get_bci(index) == bci, "StackmapList:set_bci failed");
  } else {
    TypeArray::Raw longmap(obj_field(entry_stat_offset(index))); 
    longmap().ushort_at_put(0, count);
    longmap().ushort_at_put(1, height);
    longmap().ushort_at_put(2, bci);

    GUARANTEE(get_stack_height(index) == height,
              "StackmapList:Invalid long map stack height");
    GUARANTEE(get_locals(index) == count, 
              "StackmapList:Invalid long map locals");
    GUARANTEE(get_bci(index) == bci, "StackmapList:Invalid long map bci");
  }
}

#ifndef PRODUCT

void StackmapList::print_bitmap_internal(Stream* st, unsigned int bitmap, 
                                         int index, int count,
                                         bool new_lines) {
#if USE_DEBUG_PRINTING
  for (int i = 0; i < count; i++) {
    if (bitmap & 0x1) {
      st->print("[%d] REF", index++);
    } else {
      st->print("[%d] VAL", index++);
    }
    bitmap >>= 1;
    if (new_lines) {
      st->cr();
    } else {
      st->print(" ");
    }
  }
  return;
#endif
}

void StackmapList::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
  int entries = entry_count();
  
  for (int i = 0; i < entries; i++){
    // the stackmap entries may not be initialized yet. This 
    // would happen when print_value_on is called during the 
    // course of stackmap compression during class verification
    if (!is_valid(i)) {
      continue;
    }
    print_entry_on(st, i, true);
  }
#endif
}

void StackmapList::print_entry_on(Stream *st, int i, bool new_lines) {
#if USE_DEBUG_PRINTING
  int stack_count = get_stack_height(i);
  int locals_count = get_locals(i);
  
  if (is_short_map(i)) {
    unsigned int bitmap = get_short_map(i);
    st->print("Compressed Short Stackmap: locals=%d stack_height=%d bci=%d",
              locals_count, stack_count, get_bci(i));
    if (new_lines) {
      st->cr();
    } else {
      st->print(" ");
    }
    print_bitmap_internal(st, bitmap, 0, stack_count + locals_count, new_lines);
  } else {
    UsingFastOops fast_oops;
    TypeArray::Fast bitmap = get_long_map(i);
    const int bits_per_entry = 8 * sizeof(unsigned short);
    int array_max =
        ((stack_count + locals_count + bits_per_entry - 1)/bits_per_entry) + 3;
    unsigned int bmp_bits;
    int index = 0;

    st->print("Compressed Long Stackmap: locals=%d stack_height=%d bci=%d",
              locals_count, stack_count, get_bci(i));
    if (new_lines) {
      st->cr();
    } else {
      st->print(" ");
    }
    int todo = stack_count + locals_count;
    for (int i = 3; i < array_max; i++) {
      bmp_bits = bitmap().ushort_at(i);
      print_bitmap_internal(st, bmp_bits, index,
                            min(todo, bits_per_entry), new_lines);
      todo -= bits_per_entry;
      index += bits_per_entry;
    }
  }
#endif
}

void StackmapList::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  int entries = entry_count();
  NamedField id("entry_count", true);
  visitor->do_uint(&id, entry_count_offset(), true);

  for (int i = 0; i < entries; i++){
    if (is_short_map(i)) {
      {
        NamedField id("short_map", true);
        visitor->do_int(&id, entry_stat_offset(i), true);
      }
    } else {
      {
        NamedField id("long_map", true);
        visitor->do_oop(&id, entry_stat_offset(i), true);
      }
    }
  }
#endif
}

void StackmapList::iterate_oopmaps(oopmaps_doer do_map, void *param) {
#if USE_OOP_VISITOR
  OOPMAP_ENTRY_4(do_map, param, T_INT, entry_count);
#endif
}

#endif // PRODUCT

#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int StackmapList::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;
  juint entries = (juint)entry_count();
  
  // make sure there's enough space in the field map
  // since the stackmap entry is a special object that
  // embeds a *variable* number of fields
  if ((2*entries + 1) > (juint)field_map->length()) {
    return 2*entries + 1;
  }
  // Obj Near
  map_index = Near::generate_fieldmap(field_map);
  
   // _entry_count
  field_map->byte_at_put(map_index++, T_INT);
 
  for (unsigned int i = 0 ; i < entries; i++) {
    if (!is_short_map(i)) {
      // the fields is a pointer to a TypeArray object
      field_map->byte_at_put(map_index++, T_OBJECT);
    } else {
      field_map->byte_at_put(map_index++, T_INT);
    }
  }

  return map_index;
}

#endif
