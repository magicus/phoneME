/*
 *   
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

#if !ENABLE_THUMB_COMPILER && ENABLE_COMPILER

extern "C" { extern address gp_base_label; }

class BinaryAssembler: public Macros {
 private:
  // helpers
  jint offset_at(jint pos) const { 
    return CompiledMethod::base_offset() + pos; 
  }

 public:
  jint code_size() const            { return _code_offset; }

  jint relocation_size() const      { return _relocation.size(); }
  jint free_space() const {
    return (_relocation.current_relocation_offset() + sizeof(jushort)) -
            offset_at(code_size());
  }
  jint code_end_offset() const       { return offset_at(code_size()); }
 
  // check if there's room for a few extra bytes in the compiled method
  bool has_room_for(int bytes) { 
    return free_space() >= bytes + /* slop */8;
  }

  void signal_output_overflow();
  PRODUCT_STATIC void emit_raw(int instr);

  void zero_literal_count() { 
    _unbound_literal_count = 0;
    _code_offset_to_force_literals = 0x7FFFFFFF; // never need to force
    _code_offset_to_desperately_force_literals = 0x7FFFFFFF;
  }

 public:
#if defined(PRODUCT) && !USE_COMPILER_COMMENTS
  static
#else
  virtual
#endif
  void emit(int instr) {
    // emit instruction
#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
    if (PrintCompiledCodeAsYouGo) { 
       Disassembler d(tty);
       tty->print("%d:\t", instance()->_code_offset);
       tty->print("0x%08x\t", instr);
       d.disasm(NULL, instr, instance()->_code_offset);
       tty->cr();
    }
#endif
    emit_raw(instr);
  }

#if defined(PRODUCT) && !USE_COMPILER_COMMENTS
  static
#endif 
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
#if defined(PRODUCT) && !USE_COMPILER_COMMENTS
  static
#endif 
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
  void ldr_big_integer(Register rd, int imm32, Condition cond = al);

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
  PRODUCT_STATIC void ldr_imm_index(Register rd, Register rn, int offset_12=0);

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
    FastOopInStackObj __must_appear_before_fast_objects__;
    TypeArray::Fast   _buffer;
    BinaryAssembler*  _assembler;
    int               _saved_size;
    int               _current;
    int               _length;
  };

  CodeInterleaver*    _interleaver;

#ifndef PRODUCT
  BinaryAssembler* instance() {
    return this;
  }
#else
  static BinaryAssembler* instance() {
    return (BinaryAssembler*)((void*)jvm_fast_globals.compiler_code_generator);
  }
#endif

 public:
  // creation
  BinaryAssembler(CompiledMethod* compiled_method) : 
          _relocation(compiled_method) {
    _relocation.set_assembler(this);
    _compiled_method = compiled_method;
    _code_offset     = 0;
    zero_literal_count();
    CodeInterleaver::initialize(this);
  }

  BinaryAssembler(CompilerState* compiler_state, 
                  CompiledMethod* compiled_method);
  void save_state(CompilerState *compiler_state);
  // accessors
  CompiledMethod* compiled_method() { 
      return _compiled_method; 
  }
  address addr_at(jint pos) const { 
      return (address)(_compiled_method->field_base(offset_at(pos))); 
  }
  int instruction_at(jint pos) const { 
      return *(int*)(_compiled_method->field_base(offset_at(pos))); 
  }
  bool has_overflown_compiled_method() const {
    // Using 8 instead of 0 as defensive programming
    // The shrink operation at the end of compilation will regain the extra
    // space.
    // The extra space ensures that there is always a sentinel at the end of
    // the relocation data and that there is always a null oop that the last
    // relocation entry can address. 
    return free_space() < 8; 
  }
  
  // If compiler_area is enabled, move the relocation data to higher
  // address to make room for more compiled code.
  void ensure_compiled_method_space(int delta = 0);

  // branch support

  // Labels are used to refer to (abstract) machine code locations.
  // They encode a location >= 0 and a state (free, bound, linked).
  // If code-relative locations are used (e.g. offsets from the
  // start of the code, Labels are relocation-transparent, i.e.,
  // the code can be moved around even during code generation (GC).
  class Label {
   public:
    int _encoding;
   private:
    void check(int position) {
      (void)position;
      GUARANTEE(position >= 0, "illegal position"); 
    }
    friend class CodeGenerator;
    friend class CompilationQueueElement;
    friend class Compiler;
    friend class Entry;
    friend class BinaryAssembler;

   public:
    // manipulation
    void unuse() { 
      _encoding = 0; 
    }
    void link_to(int position) { 
      check(position); 
      _encoding = - position - 1; 
    }
    void bind_to(int position) { 
      check(position); 
      _encoding =   position + 1; }

    // accessors
    int  position () const { 
      return abs(_encoding) - 1; 
    } // -1 if free label
    bool is_unused() const { return _encoding == 0; }
    bool is_linked() const { return _encoding <  0; }
    bool is_bound () const { return _encoding >  0; }

    // creation/destruction
    Label()                { unuse(); }
    ~Label()               { /* do nothing for now */ }
  };

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
  class LiteralPoolElementDesc: public MixedOopDesc {
  public:
    LiteralPoolElementDesc *_next;
    OopDesc *_literal_oop;
    jint     _bci;
    jint     _label;
    jint     _literal_int;

    bool is_bound() const {
      return _bci != 0x7fffffff;
    }

    bool matches(const OopDesc* oop, const int imm32) const {
      return oop == _literal_oop && imm32 == _literal_int;
    }
  };

protected:
  class LiteralPoolElement: public MixedOop {
  public:
    HANDLE_DEFINITION(LiteralPoolElement, MixedOop);

    static size_t allocation_size() {
      return align_allocation_size(sizeof(LiteralPoolElementDesc));
    }
    static size_t pointer_count() { return 2; }

  public:
    // To avoid endless lists of friends the static offset computation
    // routines are all public.
    static jint next_offset() {
      return FIELD_OFFSET(LiteralPoolElementDesc, _next);
    }
    static jint literal_oop_offset() {
      return FIELD_OFFSET(LiteralPoolElementDesc, _literal_oop);
    }
    static jint bci_offset() {
      return FIELD_OFFSET(LiteralPoolElementDesc, _bci);
    }
    static jint label_offset() {
      return FIELD_OFFSET(LiteralPoolElementDesc, _label);
    }
    static jint literal_int_offset() {
      return FIELD_OFFSET(LiteralPoolElementDesc, _literal_int);
    }

  private:
    enum {
      // BCI indicating this literal is still unbound.
      not_yet_defined_bci = 0x7fffffff
    };

  public:
    // Note that the bci() field is used as a convenience.
    // If the label is unbound, then bci() == 0x7fffffff
    // If the label is bound, then bci()  is the same as the label's position.

    int bci() const {
      return int_field(bci_offset());
    }
    void set_bci(int i) {
      int_field_put(bci_offset(), i);
    }

    int literal_int() const {
      return int_field(literal_int_offset());
    }
    void set_literal_int(int i) {
      int_field_put(literal_int_offset(), i);
    }

    Label label() const {
      Label L;
      L._encoding = int_field(label_offset());
      return L;
    }

    void set_label(Label& value) {
      int_field_put(label_offset(), value._encoding);
    }

    ReturnOop next() const  {
      return obj_field(next_offset());
    }
    void set_next(Oop* oop) {
      obj_field_put(next_offset(), oop);
    }

    ReturnOop literal_oop() const {
      return obj_field(literal_oop_offset());
    }
    void set_literal_oop(const Oop *oop) {
      obj_field_put(literal_oop_offset(), oop);
    }
    
    bool is_bound() const {
      return ((LiteralPoolElementDesc*)obj())->is_bound();
    }

  public:
    static ReturnOop allocate(const Oop* oop, int imm32 JVM_TRAPS);
      
#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
    void print_value_on(Stream*s);
#endif
  };

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
  void emit_pre_load_item(int ldr_offset, int stub_offset) {
    _relocation.emit(Relocation::pre_load_type, ldr_offset, stub_offset);
  }

  void emit_pre_load_item(int ldr_offset) {
    _relocation.emit(Relocation::pre_load_type, ldr_offset, _code_offset);
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
    _relocation.emit(Relocation::long_branch_type, _code_offset);
  }
  
  void emit_compressed_vsf(VirtualStackFrame* frame) {
    _relocation.emit_vsf(_code_offset, frame);
  }

#if ENABLE_CODE_PATCHING
  void emit_checkpoint_info_record(int instruction_offset,
                                   int stub_position) {
    _relocation.emit_checkpoint_info_record(instruction_offset, 
                                            instruction_at(instruction_offset),
                                            stub_position);
  }
#endif // ENABLE_CODE_PATCHING

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

  void ldr_literal(Register rd, const Oop* oop, int offset, 
                   Condition cond = al);
  void ldr_oop (Register r, const Oop* obj, Condition cond = al);

  // miscellaneous helpers
  void get_thread(Register reg);

  void generate_sentinel() { 
    write_literals(true);
    if (GenerateCompilerAssertions) {
      breakpoint();
    }
    _relocation.emit_sentinel(); 
  }

  void emit_osr_entry(jint bci) { 
    _relocation.emit(Relocation::osr_stub_type, _code_offset, bci); 
  }
  static int ic_check_code_size() { 
    // no inline caches for ARM (yet)
    return 0; 
  } 

#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
  NOT_PRODUCT(virtual) void comment(const char* /*str*/, ...);
#else
  NOT_PRODUCT(virtual) void comment(const char* /*str*/, ...) {}
#endif

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

  ReturnOop find_literal(const Oop* oop, int imm32, int offset JVM_TRAPS);
  void append_literal(LiteralPoolElement *literal);
  void write_literal(LiteralPoolElement *literal);
  void access_literal_pool(Register rd, LiteralPoolElement* literal,
                           Condition cond, bool is_store);

public:
  void write_literals(bool force = false);
  void write_literals_if_desperate(int extra_bytes = 0);

  // We should write out the literal pool at our first convenience
  bool need_to_force_literals() {
    return _code_offset >= _code_offset_to_force_literals;
  }

  // We should write out the literal pool very very soon, even if it
  // generates bad code
  bool desperately_need_to_force_literals(int extra_bytes = 0) {
    return _code_offset + extra_bytes >= _code_offset_to_desperately_force_literals;
  }

  int unbound_literal_count() { return _unbound_literal_count; }

private:
  friend class RelocationWriter;
  friend class CodeInterleaver;
#if ENABLE_INTERNAL_CODE_OPTIMIZER
  friend class Compiler;
#endif

private:
  FastOopInStackObj         __must_appear_before_fast_objects__;
  CompiledMethod*           _compiled_method;
  jint                      _code_offset;
  RelocationWriter          _relocation;
  LiteralPoolElement::Fast  _first_literal;
  LiteralPoolElement::Fast  _first_unbound_literal;
  LiteralPoolElement::Fast  _last_literal;
  int                       _unbound_literal_count;
  int                       _code_offset_to_force_literals;
                            // and to boldly split infinitives
  int                      _code_offset_to_desperately_force_literals;  
};

#if defined(PRODUCT) && !USE_COMPILER_COMMENTS
inline void Assembler::emit(int instr) {
  BinaryAssembler::emit_int(instr);
}
#endif // defined(PRODUCT) && !USE_COMPILER_COMMENTS

#endif /* !ENABLE_THUMB_COMPILER && ENABLE_COMPILER*/
