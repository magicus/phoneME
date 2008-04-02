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

#if !ENABLE_THUMB_COMPILER && ENABLE_COMPILER

extern "C" { extern address gp_base_label; }

class BinaryAssembler: public BinaryAssemblerCommon {
 public:
  void instruction_emitted( void ) const {
#if !ENABLE_CODE_OPTIMIZER
    CodeInterleaver* cil = _interleaver;
    if (cil != NULL) { 
      cil->emit();
    }
#endif    
  }

  void emit_raw( const int instr ) {
    emit_code_int( instr );
  }

 public:
#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
  virtual
#endif
  void emit(int instr) {
    // emit instruction
#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
    if (PrintCompiledCodeAsYouGo) { 
       Disassembler d(tty);
       tty->print("%d:\t", _code_offset );
       tty->print("0x%08x\t", instr);
       d.disasm(NULL, instr, _code_offset );
       tty->cr();
    }
#endif
    emit_raw(instr);
  }

  void emit_int(int instr) {
    // emit instruction
#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
    if (PrintCompiledCodeAsYouGo) { 
       tty->print("%d:\t", _code_offset);
       tty->print_cr("0x%08x\t", instr);
    }
#endif
    emit_raw(instr);
  }

#if ENABLE_EMBEDDED_CALLINFO
  void emit_ci(CallInfo info) {
    // emit call info
#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
    if (PrintCompiledCodeAsYouGo) { 
       tty->print("%d:\t", _code_offset);
       tty->print("0x%08x\t", info.raw());
       info.print(tty);

    }
#endif
    emit_raw(info.raw());  
  }
#endif // ENABLE_EMBEDDED_CALLINFO

  NOT_PRODUCT(virtual) 
  void ldr_big_integer(Register rd, int imm32, Condition cond = al) {
    ldr_literal(rd, NULL, imm32, cond);
  }

#if ENABLE_ARM_VFP
  void fld_literal(Register rd, int imm32, Condition cond = al);
#endif

  void mov_imm(Register rd, int imm32, Condition cond = al) {
    Macros::mov_imm(rd, imm32, no_CC, cond);
  }
  void mov_imm(Register rd, int imm32, LiteralAccessor* la, Condition cond=al){
    Macros::mov_imm(rd, imm32, la, no_CC, cond);
  }

  void mov_imm(Register rd, address addr, Condition cond = al);

  // generates better C++ code than ldr(rd, imm_index(rn, offset_12))
  static void ldr_imm_index(Register rd, Register rn, int offset_12=0);

public:

  class CodeInterleaver {
    void init_instance(BinaryAssembler* assembler);
  public:
    CodeInterleaver(BinaryAssembler* assembler) {
      // NB. init_instance() is used here to fix CR 6297942.
      init_instance(assembler);
    }
    ~CodeInterleaver() {}

    void flush();
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

 public:
  // creation
  void initialize( OopDesc* compiled_method ) {
    BinaryAssemblerCommon::initialize(compiled_method);
    CodeInterleaver::initialize(this);
  }

  int instruction_at(const jint pos) const { 
    return BinaryAssemblerCommon::int_at(pos); 
  }

  // branch support
  typedef BinaryLabel Label;

#if ENABLE_LOOP_OPTIMIZATION && ARM
public:
  static const Assembler::Register  rn_field(int instr) {
    return Assembler::as_register(instr >> 16 & 0xf);
   }
 
   static const Assembler::Register     rd_field(int instr) {
     return Assembler::as_register(instr >> 12 & 0xf);
   }
 
  static const Assembler::Register      rs_field(int instr)  {
    return Assembler::as_register(instr >>  8 & 0xf);
  }
 
  static const Assembler::Register      rm_field(int instr) {
    return Assembler::as_register(instr & 0xf);
  }
 
  // instruction fields
  static const bool bit(int instr, int i) {
    return (instr >> i & 0x1) == 1;
  }
 
  Assembler::Condition get_reverse_cond(Assembler::Condition cond);
  bool is_jump_instr(int instr, int next_instr, Assembler::Condition& cond, bool& link, bool& op_pc, int& offset);
  void back_branch(Label& L, bool link, Condition cond);
#endif//#if ENABLE_LOOP_OPTIMIZATION && ARM

#if ENABLE_CODE_PATCHING || ENABLE_LOOP_OPTIMIZATION
  int get_instruction(jint pos);
#endif

public:
#if ENABLE_NPCE
  enum {
    no_second_instruction = -1 //the byte code won't emit multi-LDR instructions                                         
  };

  //emit a item into relocation stream. the item contain the address of LDR instr
  //and the address the stub. So the signal handler could jump to stub. The LDR 
  //instr is stored in L.position(). the address of stub is gotten by _code_offset
  
  void emit_null_point_callback_record(Label& L,
           jint offset_of_second_instr_in_words = no_second_instruction);


  //firstly record the position of  LDR/STR instr into Compiler table for the 
  //extend basic block scheduling
  //instruction_offset is offset of memory access instruction from
  //current code_offset.
  //the position of LDR/STR is calculated from _code_offset + (instruction_offset<<2) 
  //secondly if the sub isn't shared, compiler will record the position of LDR instr 
  //into entry label of the exception stub. 
  void record_npe_point(CompilationQueueElement* stub, 
                  int instruction_offset =0, Condition cond = al);


  //check whether the instr indexed by offset is a branch instr
  bool is_branch_instr(jint offset);  
#endif //ENABLE_NPCE

#if ENABLE_INTERNAL_CODE_OPTIMIZER
  enum {
    literal_not_used_in_current_cc = -1
  };

  enum {
    stop_searching = -1,
    branch_is_in_prev_cc = -2,
    no_schedulable_branch = 0, //there's no schedulable branch in current cc.
  };
  //if we move a array load action ahead of the array boundary checking
  //we mark it into the relocation item.
  void emit_pre_load_item(const int ldr_offset, const int stub_offset) {
    emit_relocation(Relocation::pre_load_type, ldr_offset, stub_offset);
  }

  void emit_pre_load_item(const int ldr_offset) {
    emit_pre_load_item(ldr_offset, _code_offset);
  }

  //begin_of_cc represent the start address of the Compilation Continuous
  //give the offset and label return the address of first ldr instruction
  //which access this literal
  int first_instr_of_literal_loading(Label& L, address begin_of_cc);

  //return the offset of branch in the branchs chain of a unbind array 
  //boundary checking stub. the param next is the offset 
  //of the previous branch in the chain
  int next_schedulable_branch(Label L, 
                       address begin_of_cc, int& next);
#endif

  void emit_long_branch() {
    emit_relocation(Relocation::long_branch_type);
  }

#if ENABLE_PAGE_PROTECTION  
  void emit_compressed_vsf(VirtualStackFrame* frame) {
    emit_vsf(frame);
  }
#endif

  void branch(Label& L, bool link, Condition cond);
    // alignment is not used on ARM, but is needed to make
    // CompilationContinuation::compile() platform-independent. 
  void bind(Label& L, int alignment = 0); 
  void bind_to(Label& L, jint code_offset);
  // void back_patch(Label& L, jint code_offset);

  void b  (Label& L, Condition cond = al)         { branch(L, false, cond); }
  void bl (Label& L, Condition cond = al)         { branch(L, true , cond); }
  void bl(address target, Condition cond = al);  
  void b(CompilationQueueElement* cqe, Condition cond = al);

  void jmp(Label& L)                              { b(L); write_literals(); } 
  void jmp(CompilationQueueElement* cqe)          { b(cqe); write_literals(); }

  // pc-relative addressing

  void ldr_from(Register rd, LiteralPoolElement* lpe, Condition cond = al) { 
      access_literal_pool(rd, lpe, cond, false);
  }
  void str_to(Register rd, LiteralPoolElement* lpe, Condition cond = al) { 
      access_literal_pool(rd, lpe, cond, true);
  }

  void ldr_literal(Register rd, OopDesc* obj, int offset, Condition cond = al);
  void ldr_oop (Register r, const Oop* obj, Condition cond = al);

  // miscellaneous helpers
  void get_thread(Register reg);

  void generate_sentinel() { 
    write_literals(true);
    if (GenerateCompilerAssertions) {
      breakpoint();
    }
    emit_sentinel(); 
  }

  static int ic_check_code_size() { 
    // no inline caches for ARM (yet)
    return 0; 
  } 

  void ldr_using_gp(Register reg, address target, Condition cond = al) { 
    int offset = target - (address)&gp_base_label;
    ldr(reg, imm_index(gp, offset), cond);
  }

  void str_using_gp(Register reg, address target, Condition cond = al) { 
    int offset = target - (address)&gp_base_label;
    str(reg, imm_index(gp, offset), cond);
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

  void set_delayed_literal_write_threshold(const int offset) {
    const int max_code_offset_to_desperately_force_literals =
      offset - 4 * _unbound_literal_count + _code_offset;

    const int max_code_offset_to_force_literals =
      max_code_offset_to_desperately_force_literals - 0x500;

    if (max_code_offset_to_force_literals < _code_offset_to_force_literals) {
      _code_offset_to_force_literals = max_code_offset_to_force_literals;
    }

    if (max_code_offset_to_desperately_force_literals < _code_offset_to_desperately_force_literals) {
      _code_offset_to_desperately_force_literals = max_code_offset_to_desperately_force_literals;
    }  
  }

  LiteralPoolElement* find_literal(OopDesc* obj, const int imm32,
                                   int offset JVM_TRAPS);
  void append_literal(LiteralPoolElement* literal);
  void write_literal(LiteralPoolElement* literal);
  void access_literal_pool(Register rd, LiteralPoolElement* literal,
                           Condition cond, bool is_store);
public:
  void write_literals(const bool force = false);
  void write_literals_if_desperate(int extra_bytes = 0);

private:
  friend class CodeInterleaver;
  friend class Compiler;
};

#if defined(PRODUCT) && !USE_COMPILER_COMMENTS
inline void Assembler::emit(int instr) {
  ((BinaryAssembler*)_compiler_code_generator)->emit_int(instr);
}
#endif // defined(PRODUCT) && !USE_COMPILER_COMMENTS

#endif /* !ENABLE_THUMB_COMPILER && ENABLE_COMPILER*/
