/*
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

/*
 * Warning: ENABLE_THUMB_COMPILER=true is not a supported feature
 * This code is included only for reference purposes.
 */

#if ENABLE_THUMB_COMPILER && ENABLE_COMPILER

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

  void emit_raw(int instr);
  void emit_raw(short instr);

  void zero_literal_count() { 
    _unbound_literal_count = 0;
    _code_offset_to_force_literals = 0x7FFFFFFF; // never need to force
    _code_offset_to_desperately_force_literals = 0x7FFFFFFF;
  }

 public:
  NOT_PRODUCT(virtual) void emit(short instr) {
    // emit instruction
#ifndef PRODUCT      
    if (PrintCompiledCodeAsYouGo) { 
       Disassembler d(tty);
       tty->print("%d:\t", _code_offset);
       tty->print("    0x%04x\t", instr);
       d.disasm(NULL, instr, _code_offset);
       tty->cr();
    }
#endif
    emit_raw(instr);
  }

  NOT_PRODUCT(virtual) void emit_int(int instr) {
    // emit 32-bit instruction
    if (PrintCompiledCodeAsYouGo) { 
       tty->print("%d:\t", _code_offset);
       tty->print_cr("0x%08x\t", instr);
    }
    emit_raw(instr);
  }

#if ENABLE_ARM_V6T2
  void emit_w(int instr) {
#ifndef PRODUCT
    if (PrintCompiledCodeAsYouGo) { 
       Disassembler d(tty);
       tty->print("%d:\t", _code_offset);
       tty->print("0x%08x\t", instr);
       //d.disasm(NULL, instr, _code_offset); (IMPL_NOTE: should it be fixed?)
       tty->cr();
    }
#endif
    // See document "ARM DDI 0308A - ARM Architecture Reference Manual,
    // Thumb-2 supplement" section 2.6.
    // IMPL_NOTE: this does not work on big-endian target.
    juint w = (juint)instr;
    emit_raw((int) ((w >> 16) | (w << 16)));
  }
#endif

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
    emit_raw((int)info.raw());  
  }
#endif // ENABLE_EMBEDDED_CALLINFO

  NOT_PRODUCT(virtual) 
  void ldr_big_integer(Register rd, int imm32, Condition cond = al);

  void mov_imm(Register rd, int imm32, Condition cond = al);
  void mov_imm(Register rd, int imm32, LiteralAccessor& la, Condition cond=al);
  void mov_imm(Register rd, address addr, Condition cond = al);
  void mov_reg(Register rd, Register rs, Condition cond = al);


  void rsb(Register rd, Register rm, int imm, Condition cond = al);
  
  // bit manipulations
  void oop_write_barrier(Register dst, const Register tmp1, Register tmp2, 
                         Register tmp3, bool bounds_check);

  void arith(Opcode opcode, Register rd, Register rn, Condition cond = al);
  

protected:

  class CodeInterleaver {
  public:
    CodeInterleaver(BinaryAssembler* assembler);
    ~CodeInterleaver() {}
    void flush() {
      if (_buffer.not_null()) {
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
    FastOopInStackObj __must_appear_before_fast_objects__;
    TypeArray::Fast   _buffer;
    BinaryAssembler*  _assembler;
    int               _saved_size;
    int               _current;
    int               _length;
  };

  CodeInterleaver*    _interleaver;

 public:
  // creation
  BinaryAssembler(CompiledMethod* compiled_method) : 
          _relocation(compiled_method) {
    _relocation.set_assembler(this);
    _compiled_method = compiled_method;
    _code_offset     = 0;
    zero_literal_count();
   _unbound_branch_literal_count = 0;
    CodeInterleaver::initialize(this);
  }

  // support for suspending compilation
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
  void ensure_compiled_method_space();

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
      GUARANTEE(position >= 0, "illegal position"); 
      (void)position;
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

#ifndef PRODUCT
    void print_value_on(Stream*);
    void p();
#endif
  };

  class NearLabel : public Label {};
  
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

    bool matches(OopDesc* oop, int imm32) const {
      return oop == _literal_oop && imm32 == _literal_int;
    }
  };

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

    int label_pos() const {
      return label().position();
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
    void set_literal_oop(Oop *oop) {
      obj_field_put(literal_oop_offset(), oop);
    }
    
    bool is_bound() const {
      return ((LiteralPoolElementDesc*)obj())->is_bound();
    }

  public:
    static ReturnOop allocate(Oop* oop, int imm32 JVM_TRAPS);
    void iterate(OopVisitor* /*visitor*/) PRODUCT_RETURN;
  };

public:
  void branch_helper(Label& L, bool link, bool near, Condition cond);
  void branch_helper(CompilationQueueElement* cqe,
                     bool link, bool near, Condition cond);

    // alignment is not used on ARM, but is needed to make
    // CompilationContinuation::compile() platform-independent. 
  void bind(Label& L, int alignment = 0); 
  void bind_to(Label& L, jint code_offset);

  // void back_patch(Label& L, jint code_offset);

  void b  (Label& L, Condition cond = al) {
     branch_helper(L, false, false, cond);
  }
  void bl (Label& L, Condition cond = al) {
    branch_helper(L, true , false, cond); 
  }
  void b(CompilationQueueElement* cqe, Condition cond = al) {
    branch_helper(cqe, false, false, cond);
  }
                                          
  void b  (NearLabel& L, Condition cond = al)  { 
    branch_helper(L, false, true, cond); 
  }
  void bl (NearLabel& L, Condition cond = al)  { 
    branch_helper(L, true , true, cond); 
  }

  void jmp(Label& L)                              { b(L); write_literals(); } 
  void jmp(CompilationQueueElement* cqe)          { b(cqe); write_literals(); } 

  // pc-relative addressing

  void ldr_from(Register rd, LiteralPoolElement* lpe, Condition cond = al) { 
      access_literal_pool(rd, lpe, cond, false);
  }
  void str_to(Register rd, LiteralPoolElement* lpe, Condition cond = al) { 
      access_literal_pool(rd, lpe, cond, true);
  }

  void ldr_literal(Register rd, Oop* oop, int offset, Condition cond = al);
  void ldr_oop (Register r, Oop* obj, Condition cond = al);

  // miscellaneous helpers
  void get_thread(Register reg);

  void generate_sentinel() { 
    write_literals(true);
#ifdef AZZERT
    breakpoint();
#endif
    _relocation.emit_sentinel(); 
  }

  void emit_osr_entry(jint bci) { 
      _relocation.emit_osr_entry(_code_offset, bci); 
  }
  static int ic_check_code_size() { 
    // no inline caches for ARM (yet)
    return 0; 
  } 

  // debugging
  NOT_PRODUCT(virtual) void comment(const char* /*str*/, ...) PRODUCT_RETURN;

  void ldr_using_gp(Register reg, address target, Condition cond = al) {
    int offset = target - (address)&gp_base_label;
    if (offset >= 0 && has_room_for_imm(offset/4, 5)){
      ldr(reg, gp, offset, cond);
    } else {
      mov_imm(reg, offset);
      ldr_regs(reg, gp, reg);
    }
  }

  void str_using_gp(Register reg, address target, Condition cond = al) { 
    int offset = target - (address)&gp_base_label;
    str(reg, gp, offset, cond);
  }

  // Override load/store to remove conditional instructions
  void ldr(Register rd, Register rn, int offset = 0, Condition cond = al);
  void ldrb(Register rd, Register rn, int offset = 0, Condition cond = al);
  void str(Register rd, Register rn, int offset = 0, Condition cond = al);
  void strb(Register rd, Register rn, int offset = 0, Condition cond = al);

  void breakpoint(Condition cond = al);

  void arith(Opcode opcode, Register rd, Register rn, Register rm,
             CCMode s = no_CC, Condition cond = al);

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

  ReturnOop find_literal(Oop* oop, int offset JVM_TRAPS);

  void append_literal(LiteralPoolElement *literal);
  void append_branch_literal(int branch_pos JVM_TRAPS);
  void write_literal(LiteralPoolElement *literal);
  void access_literal_pool(Register rd, LiteralPoolElement* literal, 
                           Condition cond, bool is_store);

public:
  void write_literals(bool force = false);
  void write_literals_if_desperate();
  void write_value_literals();
  void write_branch_literals();  

  // We should write out the literal pool at our first convenience
  bool need_to_force_literals() { 
    return _code_offset >= _code_offset_to_force_literals;
  }

  // We should write out the literal pool very very soon, even if it
  // generates bad code
  bool desperately_need_to_force_literals() { 
     return _code_offset >= _code_offset_to_desperately_force_literals; 
  }


  bool need_to_force_branch_literals() { 
    return desperately_need_to_force_branch_literals();
  }
  
  bool desperately_need_to_force_branch_literals() { 
    if (unbound_literal_count() >= maximum_unbound_literal_count || 
        _unbound_branch_literal_count >= maximum_unbound_branch_literal_count) {
      return true;
    }
    
    if (_first_unbound_branch_literal.not_null()) {
      int branch_pos = _first_unbound_branch_literal().label_pos();
      return ((_code_offset - branch_pos) >= 200);
    }
    return false;
  }

  int unbound_literal_count() { return _unbound_literal_count; }

private:
  enum { maximum_unbound_literal_count = 10,
         maximum_unbound_branch_literal_count = 10};
  
  void increment_literal_count() {
    _unbound_literal_count++;
    if (_unbound_literal_count == 1) { 
       // If this is the first unbound literal, we need to consider forcing
       // the literal pool if the code grows more than 0xB4 beyond here,
       // since 0x400 is the maximum offset in a ldr
       _code_offset_to_force_literals = _code_offset + 0x200;
       _code_offset_to_desperately_force_literals = _code_offset + 0x3A4;
    } else if (_unbound_literal_count >= maximum_unbound_literal_count) { 
      // If we get too many literals, their size might not fit into an
      // immediate.  So we force a cutoff.
      _code_offset_to_force_literals = 0; // force at the next chance
      _code_offset_to_desperately_force_literals = 0;
    }
  }

private:
  FastOopInStackObj         __must_appear_before_fast_objects__;
  CompiledMethod*           _compiled_method;
  jint                      _code_offset;
  RelocationWriter          _relocation;

  LiteralPoolElement::Fast  _first_literal;
  LiteralPoolElement::Fast  _first_unbound_literal;
  LiteralPoolElement::Fast  _last_literal;
  int                       _unbound_literal_count;
  int                       _unbound_branch_literal_count;
  int                       _code_offset_to_force_literals;
                            // and to boldly split infinitives
  int                       _code_offset_to_desperately_force_literals;
  LiteralPoolElement::Fast  _first_unbound_branch_literal;
  LiteralPoolElement::Fast  _last_unbound_branch_literal;

friend class RelocationWriter;
friend class CodeInterleaver;

};

#endif // ENABLE_THUMB_COMPILER && ENABLE_COMPILER
