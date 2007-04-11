/*
 *    
 *
 * Portions Copyright  2000-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

# include "incls/_precompiled.incl"
# include "incls/_Relocation.cpp.incl"

#if ENABLE_COMPILER

RelocationStream::RelocationStream(CompiledMethod* compiled_method) {
  _compiled_method            = compiled_method;
  _current_relocation_offset  = compiled_method->end_offset();
  decrement();
  _current_code_offset        = 0;
  _current_oop_relocation_offset = _current_relocation_offset;
  _current_oop_code_offset    = 0;
}

RelocationStream::RelocationStream(CompilerState* compiler_state, 
                                   CompiledMethod* compiled_method) {
  _compiled_method            = compiled_method;
  _current_relocation_offset  = compiler_state->current_relocation_offset();
  _current_code_offset        = compiler_state->current_code_offset();
  _current_oop_relocation_offset  = compiler_state->current_oop_relocation_offset();
  _current_oop_code_offset        = compiler_state->current_oop_code_offset();
}

void RelocationStream::save_state(CompilerState* compiler_state) {
  compiler_state->set_current_relocation_offset(_current_relocation_offset);
  compiler_state->set_current_code_offset      (_current_code_offset);
  compiler_state->set_current_oop_relocation_offset(_current_oop_relocation_offset);
  compiler_state->set_current_oop_code_offset      (_current_oop_code_offset);
}

#if ENABLE_INLINE
void RelocationStream::restore_state(CompilerState* compiler_state) {
    _current_relocation_offset  = compiler_state->current_relocation_offset();
    _current_code_offset        = compiler_state->current_code_offset();
    _current_oop_relocation_offset  = compiler_state->current_oop_relocation_offset();
    _current_oop_code_offset        = compiler_state->current_oop_code_offset();
}
void RelocationStream::set_compiled_method(CompiledMethod* method) {
   _compiled_method = method;
}
#endif

jint RelocationWriter::compute_embedded_offset(jint code_offset) {
  const int max_embedded_offset = right_n_bits(offset_width - 1);
  const int min_embedded_offset = - max_embedded_offset - 1; 
  int offset = code_offset - _current_code_offset;
  // If the offset cannot fit inside the reloc information we emit
  // extra dummy information with max_embedded_offset as offset
  while (offset > max_embedded_offset) {
    emit_dummy(_current_code_offset + max_embedded_offset);
    offset = code_offset - _current_code_offset;
  }
  while (offset < min_embedded_offset) {
    emit_dummy(_current_code_offset + min_embedded_offset);
    offset = code_offset - _current_code_offset;
  }
  _current_code_offset = code_offset;
  return bitfield(offset, 0, offset_width);
}

void RelocationWriter::set_assembler(BinaryAssembler* value) {
  _assembler = value;
}

#if ENABLE_CODE_PATCHING
void RelocationWriter::emit_checkpoint_info_record(int code_offset,
                       unsigned int original_instruction, 
                       int stub_position) {
#if AZZERT
  for (RelocationReader stream(_compiled_method); !stream.at_end(); 
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

/*
 * oop_type entries are inserted before all others to speed up relocation
 * scanning during GC.
 */
void RelocationWriter::emit_oop(jint code_offset) {
  GUARANTEE(_current_code_offset <= code_offset, "Sanity");
  GUARANTEE(_current_oop_code_offset <= code_offset, "Sanity");
  // If there are any other entries after the last oop_type entry we need to
  // move them to insert this entry.
  const int non_oop_entries_size = 
    _current_oop_relocation_offset - _current_relocation_offset;
  GUARANTEE(non_oop_entries_size >= 0, "Cannot be negative");

  if (non_oop_entries_size > 0) {
    // Move non oop_type entries down and insert a new oop entry.
    const int max_embedded_offset = right_n_bits(offset_width - 1);
    const int min_embedded_offset = - max_embedded_offset - 1; 

    const jushort first_non_oop_entry = 
      _compiled_method->ushort_field(_current_oop_relocation_offset);
    const int first_non_oop_code_offset = 
      _current_oop_code_offset +
      sign_extend(bitfield(first_non_oop_entry, 0, offset_width), offset_width);
    const int code_offset_from_last_oop = code_offset - _current_oop_code_offset;
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

    if (_assembler->has_room_for(move_offset)) {
      address src = (address)_compiled_method->obj() + 
        _current_relocation_offset + sizeof(jushort);
      address dst = src - move_offset;
      jvm_memmove(dst, src, non_oop_entries_size);
    }
    _current_relocation_offset -= move_offset;

    const int saved_relocation_offset = _current_relocation_offset;
    const int saved_code_offset = _current_code_offset;
    _current_relocation_offset = _current_oop_relocation_offset;
    _current_code_offset = _current_oop_code_offset;

    // Emit padding before the new oop_type entry.
    const int offset_before = compute_embedded_offset(code_offset);

    GUARANTEE(_current_relocation_offset == 
              _current_oop_relocation_offset - int(padding_before), "Sanity");    

    // Emit the entry itself.
    emit_ushort((jushort) (((int) oop_type << offset_width) | offset_before));
    
    _current_oop_relocation_offset = _current_relocation_offset;

    // Emit padding after the new oop_type entry.
    const int offset_after = compute_embedded_offset(first_non_oop_code_offset);

    GUARANTEE(_current_relocation_offset == 
              _current_oop_relocation_offset - int(padding_after), "Sanity");    

    // Update the relative offset for first non-oop entry.
    const Kind first_non_oop_entry_kind = 
      (Kind)bitfield(first_non_oop_entry, offset_width, type_width);
    emit_ushort((jushort) (((int) first_non_oop_entry_kind << offset_width) | 
                           offset_after));

    _current_relocation_offset = saved_relocation_offset;
    _current_code_offset = saved_code_offset;
  } else {
    const int offset = compute_embedded_offset(code_offset);
    emit_ushort((jushort) (((int) oop_type << offset_width) | offset));
    _current_oop_relocation_offset = _current_relocation_offset;
  }

  _current_oop_code_offset = code_offset;
}

void RelocationWriter::emit(Kind kind, jint code_offset) {
  GUARANTEE(kind != oop_type, "Use emit_oop() instead");
  const int offset = compute_embedded_offset(code_offset);
  emit_ushort((jushort) (((int) kind << offset_width) | offset));
}

void RelocationWriter::emit_comment(jint code_offset, const char* comment) {
  int len = jvm_strlen(comment);
  int max = (_assembler->free_space() - 8) / sizeof(jushort) - 10;
  if (len > max) {
    len = max;
  }
  if (len < 0) {
    len = 0;
  }

  Compiler::code_generator()->ensure_compiled_method_space(2 * len + 4);
  emit(comment_type, code_offset);
  emit_ushort((jushort)len);
  for (int index = 0; index < len; index++) {
    emit_ushort((jushort) comment[index]);
  }
}

void RelocationWriter::emit_vsf(jint code_offset, VirtualStackFrame* frame) {
  emit(compressed_vsf_type, code_offset);

  int reg_location[Assembler::number_of_registers + 1];
  memset(reg_location, 0, sizeof(reg_location));
  juint reg_list = 0;

  AllocationDisabler allocation_not_allowed;
  RawLocation* loc = frame->raw_location_at(0);
  RawLocation* end  = frame->raw_location_end(loc);

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
    emit_ushort(reg_list | (max_sp_delta << sp_shift));
    emit_ushort((jushort)sp_delta);
  } else if (sp_delta >= 0) {
    // in most cases Virtual SP is very close to Real SP
    emit_ushort(reg_list | (sp_delta << sp_shift));
  } else if (-sp_delta < max_sp_delta) {
    emit_ushort(reg_list | sp_negative | ((-sp_delta) << sp_shift));
  } else {
    emit_ushort(reg_list | sp_negative | (max_sp_delta << sp_shift));
    emit_ushort((jushort)(-sp_delta));
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
  Compiler::code_generator()->ensure_compiled_method_space(2 * items_required);
  jushort* pool = current_address() + 1;
  decrement(items_required);

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

void RelocationWriter::emit_ushort(jushort value) {
//  printf("emit index=%d\n", _current_relocation_offset);
  if (_assembler->has_room_for(sizeof(jushort))) {
    _compiled_method->ushort_field_put(_current_relocation_offset, value);
  } 
  // IMPL_NOTE: This code looks bogus and should be fixed. However, don't
  // just move this decrement() into the "if" block above. Some code may depend on
  // the side-effect where the method is overflown and the offset becomes negative.
  decrement();
}

void RelocationReader::advance() {
  Kind k = kind();
#if ENABLE_APPENDED_CALLINFO
  GUARANTEE(k != callinfo_type, "Must be skipped");
#endif
  if (has_param(k)) {
    decrement(2);
  } else if (k == comment_or_padding_type) {
    decrement(2 + current(1));
#if ENABLE_COMPRESSED_VSF
  } else if (k == compressed_vsf_type) {
    juint reg_list = current(1);
    decrement((reg_list & sp_mask) == sp_mask ? 3 : 2);
    for (reg_list &= reg_mask; reg_list != 0; reg_list >>= 1) {
      if (reg_list & 1) {
        while ((current() & last_mask) == 0) {
          decrement();
        }
        decrement();
      }
    }
#endif
#if ENABLE_CODE_PATCHING
  } else if (k == tick_checkpoint_type) {
    // original instruction
    decrement(2); 
    // offset to stub
    const int end_bit = 1 << (BitsPerShort - 1);
    decrement(!(end_bit & current()) ? 2 : 1);
#endif
  } else {
    decrement();
  }
  update_current();
}

#if ENABLE_APPENDED_CALLINFO
void RelocationReader::skip_callinfo() {
  if (kind() == callinfo_type) {
    const size_t callinfo_table_size = bitfield(current(), 0, offset_width);
    _current_relocation_offset -= align_size_up(callinfo_table_size, sizeof(jushort));
  }
}
#endif // ENABLE_APPENDED_CALLINFO

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

void RelocationReader::print_current(Stream* st, bool verbose) {
#if USE_DEBUG_PRINTING
  if (at_end()) {
    st->print("end");
  } else {
    if (verbose) {
      st->print("%5d: ", _current_relocation_offset);
    } else {
      st->print("  ");
    }
    switch (kind()) {
     case Relocation::oop_type            : st->print("oop");            break;
     case Relocation::comment_type        : st->print("comment");        break;
     case Relocation::osr_stub_type       : st->print("osr stub");       break;
     case Relocation::compiler_stub_type  : st->print("compiler stub");  break;
     case Relocation::rom_oop_type        : st->print("rom oop");        break;
     case Relocation::npe_item_type       : st->print("npe table item"); break;
     case Relocation::pre_load_type       : st->print("pre load  item"); break;
     case Relocation::long_branch_type    : st->print("long branch");    break;
     case Relocation::compressed_vsf_type : st->print("compressed vsf"); break;
     default:                               st->print("<illegal>");      break;
    }
    st->print("@%d", code_offset());

    if (has_param(kind())) {
      st->print(", param=%d", current(1));
    }
  }
  st->cr();
#endif
}

Relocation::Kind RelocationReader::kind_at(int pc_offset) {
  for (RelocationReader stream(_compiled_method); !stream.at_end(); 
          stream.advance()) {
    if (pc_offset == stream.code_offset()) {
      return stream.kind();
    }
  }
  return no_relocation;
}

void RelocationReader::print(Stream* st, bool verbose) {
#if USE_DEBUG_PRINTING
  // A RelocationReader read things in reversed order. Let's
  // print things out in the ascending order for easier viewing.
  int relocation_count = 0;
  for (RelocationReader skipper(_compiled_method); !skipper.at_end(); 
            skipper.advance()) {
    relocation_count ++;
  }

  for (int i=0; i<relocation_count; i++) {
    int j=0;
    for (RelocationReader stream(_compiled_method); !stream.at_end(); 
              stream.advance(), j++) {
      if (j == (relocation_count - i - 1)) {
        stream.print_current(st, verbose);
      }
    }
  }
#endif
}

#endif


#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_TTY_TRACE
int RelocationReader::code_length(CompiledMethod* cm) { 
  RelocationReader stream(cm);
  while (!stream.at_end()) { 
    stream.advance();
  }
  return stream.current_relocation_offset() - CompiledMethod::base_offset();
}
#endif

#endif
