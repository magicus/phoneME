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
# include "incls/_BinaryAssemblerCommon.cpp.incl"

#if ENABLE_COMPILER

#define IMPLEMENT_CODE_EMITTER(type)\
void BinaryAssemblerCommon::emit_code_##type(const unsigned value) {        \
  const int code_offset = _code_offset;                                     \
  _code_offset = code_offset + sizeof(j##type);                             \
  if( has_room_for(code_offset, sizeof(j##type)) ) {                        \
    type##_at_put(code_offset, value);                                      \
    assembler()->instruction_emitted();                                     \
  }                                                                         \
}

BINARY_CODE_ACCESSOR_DO(IMPLEMENT_CODE_EMITTER)
#undef IMPLEMENT_CODE_EMITTER

void BinaryAssemblerCommon::emit_relocation_ushort(const unsigned value) {
  if( has_room_for(sizeof(jushort)) ) {
    compiled_method()->ushort_field_put( current_relocation_offset(), value);
  } 
  // IMPL_NOTE: Don't move this decrement() into the "if" block above.
  // When code buffer overflows, no data should be written
  // but relocation stream must continue to advance for the overflow detection
  _relocation.decrement();
}

jint BinaryAssemblerCommon::compute_embedded_offset(const jint code_offset) {
  const int max_embedded_offset = right_n_bits(offset_width - 1);
  const int min_embedded_offset = - max_embedded_offset - 1; 
  int offset = code_offset - _relocation._current_code_offset;
  // If the offset cannot fit inside the reloc information we emit
  // extra dummy information with max_embedded_offset as offset
  while (offset > max_embedded_offset) {
    emit_dummy(current_code_offset() + max_embedded_offset);
    offset = code_offset - current_code_offset();
  }
  while (offset < min_embedded_offset) {
    emit_dummy(current_code_offset() + min_embedded_offset);
    offset = code_offset - current_code_offset();
  }
  set_current_code_offset( code_offset );
  return bitfield(offset, 0, offset_width);
}

void BinaryAssemblerCommon::emit_relocation(const Relocation::Kind kind,
                                            const jint code_offset) {
  GUARANTEE(kind != Relocation::oop_type, "Use emit_oop() instead");
  const int offset = compute_embedded_offset(code_offset);
  emit_relocation_ushort( ((int) kind << offset_width) | offset );
}

void BinaryAssemblerCommon::emit_dummy(const jint code_offset) {
  emit_relocation(Relocation::comment_type, code_offset);
  emit_relocation_ushort(0);
}

/*
 * oop_type entries are inserted before all others to speed up relocation
 * scanning during GC.
 */
void BinaryAssemblerCommon::emit_relocation_oop( void ) {
  const jint code_offset = _code_offset;
  GUARANTEE( current_code_offset    () <= code_offset, "Sanity");
  GUARANTEE( current_oop_code_offset() <= code_offset, "Sanity");
  // If there are any other entries after the last oop_type entry we need to
  // move them to insert this entry.
  const int non_oop_entries_size = 
    current_oop_relocation_offset() - current_relocation_offset();
  GUARANTEE(non_oop_entries_size >= 0, "Cannot be negative");

  if (non_oop_entries_size > 0) {
    // Move non oop_type entries down and insert a new oop entry.
    const int max_embedded_offset = right_n_bits(offset_width - 1);
    const int min_embedded_offset = - max_embedded_offset - 1; 

    const jushort first_non_oop_entry = 
      compiled_method()->ushort_field(current_oop_relocation_offset());
    const int first_non_oop_code_offset = 
      current_oop_code_offset() +
      sign_extend(bitfield(first_non_oop_entry, 0, offset_width), offset_width);
    const int code_offset_from_last_oop = code_offset - current_oop_code_offset();
    const int code_offset_to_first_non_oop = 
      first_non_oop_code_offset - code_offset;

    GUARANTEE(code_offset_to_first_non_oop <= 0, "Cannot be positive");

    // In case if offsets do not fit in the 13-bit field we emit additional
    // padding entries (see compute_embedded_offset()). The size of a padding
    // entry is 4 bytes.
    const size_t padding_before = 
      (code_offset_from_last_oop - 1) / max_embedded_offset * 2 * sizeof(jushort);
    const size_t padding_after = 
      (code_offset_to_first_non_oop + 1) / min_embedded_offset * 2 * sizeof(jushort);
    // The total move offset is padding before the entry plus padding after the
    // entry plus the size of the entry itself (2 bytes).
    const size_t move_offset = 
      padding_before + padding_after + sizeof(jushort);

    if( has_room_for(move_offset) ) {
      address src = (address)(compiled_method()->obj()) + 
        current_relocation_offset() + sizeof(jushort);
      address dst = src - move_offset;
      jvm_memmove(dst, src, non_oop_entries_size);
    }
    set_current_relocation_offset( current_relocation_offset() - move_offset );

    const int saved_relocation_offset = current_relocation_offset();
    const int saved_code_offset = current_code_offset();
    set_current_relocation_offset( current_oop_relocation_offset() );
    set_current_code_offset( current_oop_code_offset() );

    // Emit padding before the new oop_type entry.
    const int offset_before = compute_embedded_offset(code_offset);

    GUARANTEE( current_relocation_offset() == 
               current_oop_relocation_offset() - int(padding_before), "Sanity");    

    // Emit the entry itself.
    emit_relocation_ushort(
      ((int) Relocation::oop_type << offset_width) | offset_before);
    
    set_current_oop_relocation_offset( current_relocation_offset() );

    // Emit padding after the new oop_type entry.
    const int offset_after = compute_embedded_offset(first_non_oop_code_offset);

    GUARANTEE( current_relocation_offset() == 
               current_oop_relocation_offset() - int(padding_after), "Sanity");    

    // Update the relative offset for first non-oop entry.
    const Relocation::Kind first_non_oop_entry_kind = 
      (Relocation::Kind)bitfield(first_non_oop_entry, offset_width, type_width);
    emit_relocation_ushort(
      ((int) first_non_oop_entry_kind << offset_width) | offset_after);
    set_current_relocation_offset( saved_relocation_offset );
    set_current_code_offset( saved_code_offset );
  } else {
    const int offset = compute_embedded_offset(code_offset);
    emit_relocation_ushort( ((int) Relocation::oop_type << offset_width) | offset );
    set_current_oop_relocation_offset( current_relocation_offset() );
  }

  set_current_oop_code_offset( code_offset );
}

#if ENABLE_PAGE_PROTECTION
void BinaryAssemblerCommon::emit_vsf(const VirtualStackFrame* frame) {
  enum {
    extended_mask  = Relocation::extended_mask,
    last_mask      = Relocation::last_mask,
    location_mask  = Relocation::location_mask,
    location_width = Relocation::location_width,
    max_location   = Relocation::max_location,
    max_sp_delta   = Relocation::max_sp_delta,
    sp_negative    = Relocation::sp_negative,
    sp_shift       = Relocation::sp_shift
  };

  emit_relocation(Relocation::compressed_vsf_type);

  int reg_location[Assembler::number_of_registers + 1];
  memset(reg_location, 0, sizeof(reg_location));
  juint reg_list = 0;

  const RawLocation* loc = frame->raw_location_at(0);
  const RawLocation* end = frame->raw_location_end(loc);

  // 1st pass: create mapped register list and count mapped locations
  int index = 0;
  bool second_word = false;
  for (; loc < end; loc++, index++) {
    if (second_word || !loc->is_flushed() && loc->in_register()) {
      const juint reg_no = loc->value();
      reg_location[reg_no + 1] -= (index < max_location ? 1 : 2);
      reg_list |= (1 << reg_no);
      second_word = loc->is_two_word();
    }
  }

  // Emit list of mapped registers with encoded SP difference
  const int sp_delta = frame->stack_pointer() - frame->virtual_stack_pointer();
  if (sp_delta >= max_sp_delta) {
    emit_relocation_ushort( reg_list | (max_sp_delta << sp_shift) );
    emit_relocation_ushort( sp_delta );
  } else if (sp_delta >= 0) {
    // in most cases Virtual SP is very close to Real SP
    emit_relocation_ushort( reg_list | (sp_delta << sp_shift));
  } else if (-sp_delta < max_sp_delta) {
    emit_relocation_ushort( reg_list | sp_negative | ((-sp_delta) << sp_shift));
  } else {
    emit_relocation_ushort( reg_list | sp_negative | (max_sp_delta << sp_shift) );
    emit_relocation_ushort( -sp_delta );
  }
  if (reg_list == 0) {
    // optimization for the case when there are no mapped registers
    return;
  }

  // Calculate offsets in the map for each register data
  int i;
  for (i = 0; i < Assembler::number_of_registers; i++) {
    reg_location[i + 1] += reg_location[i];
  }

  // Signal overflow if there's not enough space in compiled code
  const int items_required = - reg_location[Assembler::number_of_registers];
  ensure_compiled_method_space(2 * items_required);
  jushort* pool = _relocation.current_address(_compiled_method) + 1;
  _relocation.decrement(items_required);

  // 2nd pass: fill in the map
  index = 0;
  GUARANTEE(second_word == false, "Sanity");
  for (loc = frame->raw_location_at(0); loc < end; loc++, index++) {
    if (second_word || !loc->is_flushed() && loc->in_register()) {
      const juint reg_no = loc->value();
      int offset = --reg_location[reg_no];
      if (index < max_location) {
        pool[offset] = index;
      } else {
        pool[offset] = extended_mask | (index >> location_width);
        pool[offset - 1] = index & location_mask;
        --reg_location[reg_no];
      }
      second_word = loc->is_two_word();
    }
  }

  // Mark the last location index for each mapped register
  for (i = 0; i < Assembler::number_of_registers; i++) {
    if (reg_list & (1 << i)) {
      pool[reg_location[i]] |= last_mask;
    }
  }
}
#endif

#if ENABLE_CODE_PATCHING
void BinaryAssemblerCommon::::emit_checkpoint_info_record(int code_offset,
                       unsigned int original_instruction, 
                       int stub_position) {
#if AZZERT
  for( RelocationReader stream(_compiled_method); !stream.at_end(); 
       stream.advance()) {
    GUARANTEE(stream.code_offset() != code_offset || 
              stream.kind() == comment_type, 
      "Trying to patch relocatable instruction");
  }
#endif

  emit(Relocation::tick_checkpoint_type, code_offset);
  emit_ushort(bitfield(original_instruction, BitsPerShort, BitsPerShort)); 
  emit_ushort(bitfield(original_instruction, 0,            BitsPerShort));        

  const unsigned int uvalue = (unsigned int) stub_position;
  const unsigned int end_flag = 1 << (BitsPerShort - 1);
  const unsigned int max_value = end_flag - 1; 

  if (uvalue > max_value) {
    GUARANTEE(uvalue < 0x3FFFFFFF, "Sanity");
    emit_ushort(bitfield(uvalue, 0, BitsPerShort - 1));
    emit_ushort(
      bitfield(uvalue, BitsPerShort - 1, BitsPerShort - 1) | end_flag);
  } else {
    emit_ushort(bitfield(uvalue, 0, BitsPerShort - 1) | end_flag);
  }
}
#endif // ENABLE_CODE_PATCHING

void BinaryAssemblerCommon::ensure_compiled_method_space( int delta ) {
  delta += 256;
  if (!has_room_for(delta)) {
    delta = align_allocation_size(delta + (1024 - 256));
    if( compiled_method()->expand_compiled_code_space(delta,
                                                      relocation_size())) {
      _relocation._current_relocation_offset += delta;
      GUARANTEE(_relocation._current_relocation_offset >= 0, "sanity");
      GUARANTEE(_relocation._current_relocation_offset < 
              (int)compiled_method()->object_size(), "sanity");
      _relocation._current_oop_relocation_offset += delta;
      GUARANTEE(_relocation._current_oop_relocation_offset >= 0, "sanity");
      GUARANTEE(_relocation._current_oop_relocation_offset < 
              (int)compiled_method()->object_size(), "sanity");
    }
  }
}

#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
void BinaryAssemblerCommon::comment(const char* fmt, ...) {
  JVM_VSNPRINTF_TO_BUFFER(fmt, buffer, 1024);

  if (PrintCompiledCodeAsYouGo) {
    tty->print_cr(";; %s", buffer);
  } else if (GenerateCompilerComments) {
    int len = jvm_strlen(buffer);
    const int max = (free_space() - 8) / sizeof(jushort) - 10;
    if (len > max) {
      len = max;
    }
    if (len < 0) {
      len = 0;
    }

    ensure_compiled_method_space(2*len + 4);
    emit_relocation(Relocation::comment_type);
    emit_relocation_ushort( len );
    for( int index = 0; index < len; index++ ) {
      emit_relocation_ushort( buffer[index] );
    }
  }
}
#endif // !defined(PRODUCT) || USE_COMPILER_COMMENTS

#endif
