/*
 *
 * Portions Copyright  2003-2006 Sun Microsystems, Inc. All Rights Reserved.
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

void RelocationWriter::emit(Kind kind, jint code_offset) {
  if (kind == oop_type) {
    emit_oop(code_offset);
  } else {
    int offset = compute_embedded_offset(code_offset);
    emit_ushort((jushort) (((int) kind << offset_width) | offset));
  }
}

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

void RelocationWriter::emit_osr_entry(jint code_offset, jint bci) {
  emit(osr_stub_type, code_offset);
  emit_ushort((jushort) bci);
}

void RelocationWriter::emit_comment_or_dummy(const char* comment, jint code_offset) {
  emit(comment_type, code_offset);
  int len;
  if (comment == NULL) {
    // We want to emit a dummy
    len = 0;
  } else {
    // We're really emitting comments. Try not to overflow.
    len = jvm_strlen(comment);
    int max = (_assembler->free_space() - 8) / sizeof(jushort) - 10;
    if (len > max) {
      len = max;
    }
    if (len < 0) {
      len = 0;
    }
  }
  emit_ushort((jushort)len);
  for (int index = 0; index < len; index++) {
    emit_ushort((jushort) comment[index]);
  }
}
#if ENABLE_NPCE
void  RelocationWriter::emit_npe_item(jint ldr_offset, jint code_offset){
  emit(npe_item_type, code_offset);
  emit_ushort((jshort)ldr_offset);
}
#endif

#if ENABLE_INTERNAL_CODE_OPTIMIZER && ENABLE_CODE_OPTIMIZER
void  RelocationWriter::emit_pre_load_item(jint ldr_offset, jint code_offset){
  emit(pre_load_type, code_offset);
  emit_ushort((jshort)ldr_offset);
}
#endif 

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
  decrement();
  if (has_bci(k)) {
    decrement();
  }
  else if (k == comment_or_padding_type) {
    jint len = current();
    decrement();
    for (int index = 0; index < len; index++) {
      decrement();
    }
  }
#if ENABLE_NPCE
  else if (k == npe_item_type) {
    decrement(); //ldr offset
  }
#endif //ENABLE_NPCE
#if ENABLE_INTERNAL_CODE_OPTIMIZER && ENABLE_CODE_OPTIMIZER
  else if (k == pre_load_type ) {
    decrement(); //ldr offset
  }
#endif //ENABLE_INTERNAL_CODE_OPTIMIZER
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

#ifndef PRODUCT

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
     case Relocation::oop_type            : st->print("oop");           break;
     case Relocation::comment_type        : st->print("comment");       break;
     case Relocation::osr_stub_type       : st->print("osr stub");      break;
     case Relocation::compiler_stub_type  : st->print("compiler stub"); break;
     case Relocation::rom_oop_type        : st->print("rom oop");       break;
#if ENABLE_NPCE
     case Relocation::npe_item_type       : st->print("npe table item");break;
#endif //ENABLE_NPCE
#if ENABLE_INTERNAL_CODE_OPTIMIZER && ENABLE_CODE_OPTIMIZER
     case Relocation::pre_load_type       : st->print("pre load  item");break;
#endif //ENABLE_INTERNAL_CODE_OPTIMIZER

     default:                               st->print("<illegal>");     break;
    }
    st->print("@%d", code_offset());

    if (has_bci(kind())) {
      st->print(", bci=%d", bci());
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


#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR
int RelocationReader::code_length(CompiledMethod* cm) { 
  RelocationReader stream(cm);
  while (!stream.at_end()) { 
    stream.advance();
  }
  return stream.current_relocation_offset() - CompiledMethod::base_offset();
}
#endif

#endif
