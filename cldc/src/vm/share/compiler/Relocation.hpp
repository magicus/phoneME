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
//   3 bits indicating the relocation type
//  13 bits indicating the byte offset from the previous relocInfo address

class Relocation: public StackObj {
 public:
  enum Kind {
    oop_type            =  0, // embedded oop
    comment_type        =  1, // comment for embedding in relocation (also
                              // used for padding!)
    comment_or_padding_type =  comment_type, // Ditto
    osr_stub_type       =  2, // stub for osr entry
    compiler_stub_type  =  3, // stub for relative calls/jumps to static code
    rom_oop_type        =  4, // embedded oop, but GC doesn't care
#if ENABLE_NPCE
    npe_item_type      =  5, // npe mapping item;
#endif //ENABLE_NPCE
#if ENABLE_INTERNAL_CODE_OPTIMIZER && ENABLE_CODE_OPTIMIZER
    pre_load_type         =  6,
#endif
#if ENABLE_APPENDED_CALLINFO
    callinfo_type       =  7, // CallInfo record
#endif
    no_relocation       = -1  // Used for specifying no relocation in 
                              // BinaryAssembler
  };

  static bool has_bci(Kind kind) {
    return kind == osr_stub_type;
  }

  enum Constants {
    type_width   =  3,
    offset_width = 13
  };
};

class RelocationStream : public Relocation {
 public:
  jint current_relocation_offset() const {
    return _current_relocation_offset;
  }
  void save_state(CompilerState* compiler_state);
#if ENABLE_INLINE && ARM
  void restore_state(CompilerState* compiler_state);
  void set_compiled_method(CompiledMethod* method);
#endif

 protected:
  RelocationStream(CompiledMethod* compiled_method);
  RelocationStream(CompilerState* compiler_state,
                   CompiledMethod* compiled_method);

  void decrement() {
    _current_relocation_offset -= sizeof(jushort);
  }

  CompiledMethod* _compiled_method;
  jint            _current_relocation_offset;
  jint            _current_code_offset;
  jint            _current_oop_relocation_offset;
  jint            _current_oop_code_offset;
};

inline int sign_extend(int x, int field_length) {
  return (int)x << (BitsPerWord - field_length) >> (BitsPerWord - field_length);
}

class RelocationReader: public RelocationStream {
 private: 
  jushort current(jint offset = 0) const {
    return _compiled_method->ushort_field(_current_relocation_offset -
                                          (offset * sizeof(jushort)));
  }

  void update_current() {
    _current_code_offset += 
      sign_extend(bitfield(current(), 0, offset_width), offset_width);
  }

#if ENABLE_APPENDED_CALLINFO
  void skip_callinfo();
#endif
 public:
  RelocationReader(CompiledMethod* compiled_method)
      : RelocationStream(compiled_method) {
#if ENABLE_APPENDED_CALLINFO
    skip_callinfo();
#endif
    update_current();
  }

  // Tells whether we are at the end of the stream.
  bool at_end() const {
    return current() == 0;
  }

  // Accessors for current relocation pair
  Kind kind() const {
    return (Kind) bitfield(current(), offset_width, type_width);
  }
  int code_offset() {
    return _current_code_offset;
  }

  // Advance to next relocation pair
  void advance();

  jint bci() const {
    return _compiled_method->ushort_field(_current_relocation_offset -
                                          sizeof(jushort));
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
#if ENABLE_NPCE || ( ENABLE_INTERNAL_CODE_OPTIMIZER && ENABLE_CODE_OPTIMIZER )
  jint current_item(int offset){
    return current(offset);
  }
#endif //ENABLE_NPCE

  bool is_oop()            const { return kind() == oop_type;            }
  bool is_comment()        const { return kind() == comment_type;        }
  bool is_osr_stub()       const { return kind() == osr_stub_type;       }
  bool is_compiler_stub()  const { return kind() == compiler_stub_type;  }
  bool is_rom_oop()        const { return kind() == rom_oop_type;        }
#if ENABLE_NPCE
  bool is_npe_item()      const { return kind() == npe_item_type;      }
#endif //ENABLE_NPCE
#if ENABLE_INTERNAL_CODE_OPTIMIZER && ENABLE_CODE_OPTIMIZER
  bool is_pre_load_item()      const { return kind() == pre_load_type;      }
#endif

#ifndef PRODUCT
  Kind kind_at(int pc_offset);
  void print_current(Stream* st, bool verbose=false);
  void print(Stream *st, bool verbose=false);
#endif

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR
  static int code_length(CompiledMethod* cm);
#endif
};

class RelocationWriter: public RelocationStream {
 public:
   RelocationWriter(CompiledMethod* compiled_method)
       : RelocationStream(compiled_method)
   {}

   RelocationWriter(CompilerState* compiler_state,
                    CompiledMethod* compiled_method)
       : RelocationStream(compiler_state, compiled_method)
   {}

   void set_assembler(BinaryAssembler* value);
   
   void emit(Kind kind, jint code_offset);
   void emit_osr_entry(jint code_offset, jint bci);
   void emit_sentinel() { emit_ushort(0); }
   void emit_comment_or_dummy(const char* comment, jint code_offset);
   void emit_comment(const char* comment, jint code_offset)  {
        emit_comment_or_dummy(comment, code_offset);
   }
   void emit_dummy(jint code_offset) {
        emit_comment_or_dummy((const char*)NULL, code_offset);
   }
#if ENABLE_NPCE
   void emit_npe_item(jint ldr_offset, jint code_offset);
#endif //ENABLE_NPCE
#if ENABLE_INTERNAL_CODE_OPTIMIZER && ENABLE_CODE_OPTIMIZER
   void emit_pre_load_item(jint ldr_offset, jint code_offset);
#endif
   // Returns the size of the relocation data in bytes.
   jint size() const {
     return _compiled_method->end_offset()
          - _current_relocation_offset
          - sizeof(jushort);
   }

   // This is called after the compiled method has been expanded in-place
   // (by moving the relocation data to higher address)
   void move(int delta) {
     _current_relocation_offset += delta;
     GUARANTEE(_current_relocation_offset >= 0, "sanity");
     GUARANTEE(_current_relocation_offset < 
               (int)_compiled_method->object_size(), "sanity");
     _current_oop_relocation_offset += delta;
     GUARANTEE(_current_oop_relocation_offset >= 0, "sanity");
     GUARANTEE(_current_oop_relocation_offset < 
               (int)_compiled_method->object_size(), "sanity");
   }
 private:
   void emit_oop(jint code_offset);

   void emit_ushort(jushort value);
   jint compute_embedded_offset(jint code_offset);

   BinaryAssembler* _assembler;
};

#endif
