/*
 *    
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
    const size_t callinfo_table_size = CallInfoRecord::table_size(current());
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
  return stream._current_relocation_offset - CompiledMethod::base_offset();
}
#endif

#endif
