/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#if ENABLE_COMPILER

extern "C" { extern address gp_base_label; }

class BinaryAssembler: public BinaryAssemblerCommon {
 public:
  static void instruction_emitted( void ) {}
  void emit_raw(short instr) { emit_code_short( instr ); }

  NOT_PRODUCT(virtual) void emit(short instr) {
    // emit instruction
    decrease_current_it_depth();
#ifndef PRODUCT
    if (PrintCompiledCodeAsYouGo) {
       Disassembler d(tty);
       tty->print("%d:\t", _code_offset);
       tty->print("0x%04x\t\t", instr & 0xffff);
       d.disasm(&instr, instr, _code_offset);
       tty->cr();
    }
#endif
    emit_raw(instr);
  }

  void emit_int(int instr) {
    // emit 32-bit instruction
#ifndef PRODUCT
    if (PrintCompiledCodeAsYouGo) {
       tty->print("%d:\t", _code_offset);
       tty->print_cr("0x%08x\t", instr);
    }
#endif
    juint w = (juint) instr;
    emit_raw((jushort) w);
    emit_raw((jushort) (w >> 16));
  }

  void emit_w(int instr) {
    decrease_current_it_depth();
#ifndef PRODUCT
    if (PrintCompiledCodeAsYouGo) {
       Disassembler d(tty);

       juint w = (juint)instr;
       jushort hw[2];
       hw[0] = w >> 16;
       hw[1] = w & 0xffff;

       tty->print("%d:\t", _code_offset);
       tty->print("0x%04x 0x%04x\t", hw[0], hw[1]);

       d.disasm((short*)hw, (short)hw[0], _code_offset);
       tty->cr();
    }
#endif
    // See document "ARM DDI 0308A - ARM Architecture Reference Manual,
    // Thumb-2 supplement" section 2.6.
    // IMPL_NOTE: this does not work on big-endian target.
    juint w = (juint) instr;
    emit_raw((jushort) (w >> 16));
    emit_raw((jushort) w);
  }

#if ENABLE_EMBEDDED_CALLINFO
  void emit_ci(CallInfo info) {
    // emit call info
#ifndef PRODUCT
    if (PrintCompiledCodeAsYouGo) {
       tty->print("%d:\t", _code_offset);
       tty->print("0x%08x\t", info.raw());
       info.print(tty);
    }
#endif
    juint w = (juint) info.raw();
    emit_raw((jushort) w);
    emit_raw((jushort) (w >> 16));
  }
#endif // ENABLE_EMBEDDED_CALLINFO

  NOT_PRODUCT(virtual)
  void ldr_big_integer(Register rd, int imm32, Condition cond = al);

  void mov_imm(Register rd, int imm32, LiteralAccessor &la, Condition cond=al);
  void ldr_address(Register rd, address addr, Condition cond = al);

protected:
  class CodeInterleaver {
  public:
    CodeInterleaver(BinaryAssembler* assembler);
    ~CodeInterleaver() {}
    void flush( void ) {
      if( _buffer ) {
        while (emit()) {;}
      }
      _assembler->_interleaver = NULL;
    }
    void start_alternate(JVM_SINGLE_ARG_TRAPS);
    bool emit();

    static void initialize(BinaryAssembler* assembler) {
      assembler->_interleaver = NULL;
    }

  private:
    CompilerIntArray* _buffer;
    BinaryAssembler*  _assembler;
    int               _saved_size;
    int               _current;
    int               _length;
  };

  CodeInterleaver*    _interleaver;

 protected:
  void initialize(OopDesc* compiled_method) {
    BinaryAssemblerCommon::initialize(compiled_method);
    CodeInterleaver::initialize(this);
  }
 public:
  // branch support

  // Labels are used to refer to (abstract) machine code locations.
  // They encode a location >= 0 and a state (free, bound, linked).
  // If code-relative locations are used (e.g. offsets from the
  // start of the code, Labels are relocation-transparent, i.e.,
  // the code can be moved around even during code generation (GC).
  typedef BinaryLabel Label;
  class NearLabel : public Label {};
 private:
  void branch_helper(Label& L, const bool link, const bool near,
                     const Condition cond);
 public:

    // alignment is not used on ARM, but is needed to make
    // CompilationContinuation::compile() platform-independent.
  void bind(Label& L, int alignment = 0);
  void bind_to(Label& L, jint code_offset);

  // void back_patch(Label& L, jint code_offset);

  void b  (Label& L, const Condition cond = al) {
    branch_helper(L, false, false, cond);
  }
  void bl (Label& L, const Condition cond = al) {
    branch_helper(L, true , false, cond);
  }
  void b  (NearLabel& L, const Condition cond = al)  {
    branch_helper(L, false, true, cond);
  }
  void bl (NearLabel& L, const Condition cond = al)  {
    branch_helper(L, true , true, cond);
  }

  void b(CompilationQueueElement* cqe, const Condition cond = al);

  void jmp( Label& L )                            { b(L);   write_literals(); }
  void jmp( CompilationQueueElement* cqe )        { b(cqe); write_literals(); }

  // pc-relative addressing

  void ldr_from(Register rd, LiteralPoolElement* lpe, const Condition cond = al) {
    access_literal_pool(rd, lpe, cond, false);
  }
  void str_to(Register rd, LiteralPoolElement* lpe, const Condition cond = al) {
    access_literal_pool(rd, lpe, cond, true);
  }

  void ldr_literal(Register rd, OopDesc* obj, const int offset,
                   const Condition cond = al);
  void ldr_oop (Register r, Oop* obj, const Condition cond = al);

  // miscellaneous helpers
  void get_thread(Register reg);

  void generate_sentinel() {
    write_literals(true);
#ifdef AZZERT
    if (GenerateCompilerAssertions) {
      breakpoint();
    }
#endif
    emit_sentinel();
  }

  static int ic_check_code_size() {
    // no inline caches for ARM (yet)
    return 0;
  }

  void ldr_using_gp(Register reg, address target, const Condition cond = al) {
    const int offset = target - (address)&gp_base_label;
    it(cond);
    ldr_using_gp(reg, offset);
  }

  void ldr_using_gp(const Register reg, const char name[] ) {
    int offset = find_gp_offset(name);
    GUARANTEE(offset >= 0, "sanity");
    ldr_using_gp(reg, offset);
  }

  void ldr_using_gp(const Register r, const int offset) {
#if ENABLE_ARM_V7
    if (offset >= 0 && has_room_for_imm(offset/4, 5) && r < r8) {
      ldr_r10(r, offset >> 2);
    } else if (has_room_for_imm(offset, 12)) {
      ldr(r, gp, offset);
    } else {
      SHOULD_NOT_REACH_HERE();
    }
#else
    if (r < r8 && has_room_for_imm(offset/4, 5)) {
      ldr_imm5x4(r, gp, offset/4);
    } else {
      mov(r, offset);
      ldr(r, gp, reg(r));
    }
#endif
  }

  int find_gp_offset(const char name[] ) {
    int offset = 1 * sizeof(OopDesc*); // skip the nop bytecode

    static const GPTemplate gp_templates[] = {
      GP_SYMBOLS_DO(DEFINE_GP_POINTER, DEFINE_GP_VALUE)
      {NULL, 0, 0, 0}
    };

    for (const GPTemplate* tmpl = gp_templates; tmpl->name; tmpl++) {
      if (jvm_strcmp(name, tmpl->name) == 0) {
        return offset;
      }
      offset += tmpl->size;
      GUARANTEE((offset % 4) == 0, "must be word aligned");
    }

    return -1;
  }

  void str_using_gp(Register reg, address target, Condition cond = al) {
    int offset = target - (address)&gp_base_label;
    str(reg, gp, offset, cond);
  }

  // By making them virtual, we can create macros that work in both
  // the binary assembler and the source assembler.  See, e.g.
  // oop_write_barrier
#define DEFINE_GP_FOR_BINARY(name, size) \
  NOT_PRODUCT(virtual) void get_ ## name(Register reg, Condition cond = al) { \
     ldr_using_gp(reg, (address)&_ ## name, cond);                 \
  }                                                                \
  NOT_PRODUCT(virtual) void set_ ## name(Register reg, Condition cond = al) { \
     str_using_gp(reg, (address)&_ ## name, cond);                 \
  }                                                                \

  GP_GLOBAL_SYMBOLS_DO(pointers_not_used, DEFINE_GP_FOR_BINARY)

  void set_delayed_literal_write_threshold( const int offset ) {
    const int max_code_offset_to_desperately_force_literals =
      offset - 4 * _unbound_literal_count + _code_offset;

    const int max_code_offset_to_force_literals =
      max_code_offset_to_desperately_force_literals - 0x200;

    if (max_code_offset_to_force_literals < _code_offset_to_force_literals) {
      _code_offset_to_force_literals = max_code_offset_to_force_literals;
    }

    if (max_code_offset_to_desperately_force_literals < _code_offset_to_desperately_force_literals) {
      _code_offset_to_desperately_force_literals = max_code_offset_to_desperately_force_literals;
    }
  }

  LiteralPoolElement* find_literal(OopDesc* obj, const int imm32 JVM_TRAPS);

  void append_literal( LiteralPoolElement* literal );
  void write_literal ( LiteralPoolElement* literal );
  void branch_around_literals( void );

  void access_literal_pool(Register rd, LiteralPoolElement* literal,
                           const Condition cond, const bool is_store);

public:
  void write_literals              ( const bool force = false  );
  void write_literals_if_desperate ( const int extra_bytes = 2 );

private:
  friend class CodeInterleaver;
  friend class Compiler;
};

#endif // ENABLE_COMPILER
