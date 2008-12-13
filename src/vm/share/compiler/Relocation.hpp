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

#if ENABLE_COMPILER

// A RelocationReader iterates through the relocation information of a
// CompiledMethod.
// Usage:
//   for (RelocationReader stream(compiled_method); !stream.at_end(); 
//        stream.advance()) {
//     switch (stream.type()) {
//      case RelocationReader::oop_type:
//     }
//   }
//
// Relocation information is represented as a 0 terminated string of 16 bits:
//   4 bits indicating the relocation type
//  12 bits indicating the byte offset from the previous relocInfo address

class Relocation {
 public:
  enum Kind {
    oop_type = 0,         // embedded oop
    comment_type,         // code comment (also used for padding)
    comment_or_padding_type =  comment_type,
    osr_stub_type,        // stub for osr entry
    compiler_stub_type,   // stub for relative calls/jumps to static code
    rom_oop_type,         // embedded oop, but GC doesn't care
    npe_item_type,        // ENABLE_NPCE
    pre_load_type,        // ENABLE_INTERNAL_CODE_OPTIMIZER
    long_branch_type,     // USE_COMPILER_GLUE_CODE
    compressed_vsf_type,  // ENABLE_COMPRESSED_VSF
    callinfo_type,        // ENABLE_APPENDED_CALLINFO
    tick_checkpoint_type, // ENABLE_CODE_PATCHING
    no_relocation = -1    // Used for specifying no relocation in BinaryAssembler
  };

  static bool has_param(Kind kind) {
    return kind == osr_stub_type ||
           kind == npe_item_type ||
           kind == pre_load_type;
  }

  enum Constants {
    type_width   = 4,
    offset_width = 12
  };

  enum VSFConstants {
    extended_mask    = 1 << 12,
    last_mask        = 1 << 13,
    location_width   = 12,
    max_location     = 1 << location_width,
    location_mask    = ~(extended_mask | last_mask),
    max_sp_delta     = 3,
    sp_shift         = 5,
    sp_mask          = max_sp_delta << sp_shift,
    sp_negative      = 1 << 15,
    reg_mask         = ~(sp_mask | sp_negative)
  };

};

class RelocationStream : public Relocation {
 protected:
  RelocationStream(void) {}

  void initialize(CompiledMethod* compiled_method) {
    _current_relocation_offset  = compiled_method->end_offset();
    decrement();
    _current_code_offset        = 0;
    _current_oop_relocation_offset = _current_relocation_offset;
    _current_oop_code_offset    = 0;
  }

  void decrement(const jint items = 1) {
    _current_relocation_offset -= items * sizeof(jushort);
  }

  jushort* current_address( OopDesc* method ) const {
    return method->ushort_field_addr(_current_relocation_offset);
  }

  jint _current_relocation_offset;
  jint _current_code_offset;
  jint _current_oop_relocation_offset;
  jint _current_oop_code_offset;

  friend class BinaryAssemblerCommon;
};

inline int sign_extend(int x, int field_length) {
  return (int)x << (BitsPerWord - field_length) >> (BitsPerWord - field_length);
}

class RelocationReader: public RelocationStream {
 private: 
  CompiledMethod* _compiled_method;

  void update_current( void ) {
    _current_code_offset += 
      sign_extend(bitfield(current(), 0, offset_width), offset_width);
  }

#if ENABLE_APPENDED_CALLINFO
  void skip_callinfo();
#endif
 public:
  RelocationReader(CompiledMethod* compiled_method) {
    _compiled_method = compiled_method;
    initialize( compiled_method );
#if ENABLE_APPENDED_CALLINFO
    skip_callinfo();
#endif
    update_current();
  }

#if ENABLE_CODE_PATCHING
  jint current_offset(jint offset) {
    return _current_relocation_offset - (offset * sizeof(jushort));
  }
#endif

  // Tells whether we are at the end of the stream.
  bool at_end( void ) const {
    return current() == 0;
  }

  // Accessors for current relocation pair
  Kind kind( void ) const {
    return (Kind) bitfield(current(), offset_width, type_width);
  }
  int code_offset( void ) {
    return _current_code_offset;
  }

  // Advance to next relocation pair
  void advance( void );

  jushort current(const jint offset = 0) const {
    return current_address( _compiled_method->obj() )[-offset];
  }

  void print_comment_on(Stream* st) {
#ifndef PRODUCT
    GUARANTEE(kind() == comment_type, 
              "cannot print string from non-comment type");
    jint len = current(1);
    for (int index = 0; index < len; index++) {
      st->print("%c", current(2 + index));
    }
#else
    (void) st;
#endif
  }

  bool is_oop()             const { return kind() == oop_type;             }
  bool is_comment()         const { return kind() == comment_type;         }
  bool is_osr_stub()        const { return kind() == osr_stub_type;        }
  bool is_compiler_stub()   const { return kind() == compiler_stub_type;   }
  bool is_rom_oop()         const { return kind() == rom_oop_type;         }
  bool is_npe_item()        const { return kind() == npe_item_type;        }
  bool is_pre_load_item()   const { return kind() == pre_load_type;        }
  bool is_long_branch()     const { return kind() == long_branch_type;     }
  bool is_compressed_vsf()  const { return kind() == compressed_vsf_type;  }
  bool is_checkpoint_info() const { return kind() == tick_checkpoint_type; }

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  Kind kind_at(int pc_offset);
  void print_current(Stream* st, bool verbose=false);
  void print(Stream *st, bool verbose=false);
#endif

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_TTY_TRACE
  static int code_length(CompiledMethod* cm);
#endif
};

#endif
