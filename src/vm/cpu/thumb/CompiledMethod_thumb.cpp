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

/*
 * Warning: ENABLE_THUMB_COMPILER=true is not a supported feature
 * This code is included only for reference purposes.
 */


# include "incls/_precompiled.incl"
# include "incls/_CompiledMethod_thumb.cpp.incl"

#if ENABLE_COMPILER

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

static void decode_instruction(Stream* st, short* addr, int offset) {
  Disassembler(st).disasm(addr, *addr, offset);
}

void CompiledMethod::print_code_on(Stream* st) {
  int end_offset = RelocationReader::code_length(this);
  for (int offset = 0; offset < end_offset; offset += 2) { 
    short* instruction_start = (short*) (entry() + offset);

    print_comment_for(offset, st);

    bool is_oop = false;
    for (RelocationReader stream(this); !stream.at_end(); stream.advance()) {
      if (stream.code_offset() == offset) { 
        if (stream.is_oop() || stream.is_rom_oop()) { 
          is_oop = true;
          break;
        }
      }
    }
    st->print(" %4d: ", offset);
    if (VerbosePointers) {
      st->print("0x%08x: ", instruction_start);
    }
    if (!is_oop) { 
      decode_instruction(st, instruction_start, offset);
    } else { 
      GUARANTEE(((int)instruction_start & 0x3) == 0, 
                "Disassembler: Invalid embedded Oop");
      Oop o = (OopDesc*)*(int*)instruction_start;
      if (VerbosePointers) {
        st->print("0x%08x:", *(int*)instruction_start);
      }
      o.print_value_on(st);
      offset += 2;
    }
    st->cr();
  }
}

#endif // !defined(PRODUCT) || ENABLE_TTY_TRACE
#endif // ENABLE_COMPILER

#if ENABLE_ROM_GENERATOR
// generate a map of all the field types in this object
int CompiledMethod::generate_fieldmap(TypeArray* field_map) {
#if !ENABLE_COMPILER
  SHOULD_NOT_REACH_HERE();
  return 0;
#else
  int map_index = 0;
  int max_length = size() + 3;
  if (field_map->length() < max_length) {
    // Treat everything as a byte after the header
    return max_length;
  }
  // Generic object near
  map_index = Near::generate_fieldmap(field_map);

  // flags_and_size
  field_map->byte_at_put(map_index++, T_INT);
  // method
  field_map->byte_at_put(map_index++, T_OBJECT);

  int offset;
  int end_offset = RelocationReader::code_length(this);
  for (offset = 0; offset < end_offset; offset += 4) { 
    int* instruction_start = (int *)(entry() + offset);
    bool is_oop = false;
    bool is_address = false;
    for (RelocationReader stream(this); !stream.at_end(); stream.advance()) {
      if (stream.code_offset() == offset) { 
        if (stream.is_oop() || stream.is_rom_oop()) { 
          is_oop = true;
          break;
        } else if (stream.is_compiler_stub()) { 
          GUARANTEE(GenerateROMImage, "Must be generating ROM Image");
          is_address = true;
          break;
        }
      }
    }
    if (is_oop) { 
        // field_map->int_at_put(map_index++, T_INT);
      field_map->byte_at_put(map_index++, T_OBJECT);
    } else if (is_address) { 
      field_map->byte_at_put(map_index++, T_SYMBOLIC);
    } else { 
      field_map->byte_at_put(map_index++, T_INT);
    }
  }
  // Now deal with the relocation table
  for (; offset < size(); offset += 2) {
    field_map->byte_at_put(map_index++, T_SHORT);
  }
  if ((offset % 4) != 0) {
    // IMPL_NOTE: why will this happen? Anyway, let's just add a Short to be sure.
    field_map->byte_at_put(map_index++, T_SHORT);
  }
  return map_index;
#endif
}
#endif // ENABLE_ROM_GENERATOR
