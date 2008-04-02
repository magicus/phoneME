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

#if ENABLE_COMPILER

class BinaryAssembler: public BinaryAssemblerCommon {
 public:
  class InternalLabel {
   public:
    InternalLabel  ()                { unuse(); }

    bool is_bound   () const { return _encoding <  0; }
    bool is_unbound () const { return _encoding >  0; }
    bool is_unused  () const { return _encoding == 0; }

    int  position   () const { 
      if (_encoding < 0) return - _encoding - 1;
      if (_encoding > 0) return   _encoding - 1;
      return 0;
    }

    void bind_to (int position) {
      GUARANTEE(position >= 0, "Illegal position");
      _encoding = - position - 1;
    }

    void link_to (int position) {
      GUARANTEE(position >= 0, "Illegal position");
      _encoding =   position + 1;
    }

    void unuse()                { _encoding = 0; }

   public:
    // _encoding encodes both the binding state (via its sign)
    // and the binding position (via its value) of a label.
    //
    // _encoding <  0  Bound label, 
    //                 position() returns the target (jump) position. 
    // _encoding == 0  Unused label.
    // _encoding >  0  Unbound label, 
    //                 position() returns the last displacement in the chain.

    int _encoding;  
   private:
    friend class Compiler;
    friend class Entry;
  };

  class Label : public InternalLabel { };

  class NearLabel : public InternalLabel { };

  class Address {
   public:
    Address(int disp) : 
      _disp  (disp),
      _base  (no_reg),
      _index (no_reg)
    { }

    Address(Register base, int disp = 0) :
      _disp  (disp),
      _base  (base),
      _index (no_reg)
    { }

    Address(Register base, Register index, const ScaleFactor& scale, int disp = 0) :
      _disp  (disp),
      _base  (base),
      _index (index),
      _scale (scale)
    { }

   private:
    Register     _base;
    Register     _index;
    ScaleFactor  _scale;
    int          _disp;

    friend class BinaryAssembler;
    friend class CodeGenerator;
  };

#if ENABLE_COMPILER

  enum generic_binary_op_1 { 
    bin_illegal = -1, bin_add = 0, bin_or = 1, bin_adc = 2, bin_and = 4, 
    bin_sub = 5, bin_xor = 6
  };
  void generic_binary_1(Register dst, Register src, generic_binary_op_1 op);
  void generic_binary_1(Register dst, int imm32, generic_binary_op_1 op);

  // Move instructions.
  void movl   (Register dst, const Address& src);
  void movl   (Register dst, Register src);
  void movl   (Register dst, int imm32);
  void movl   (Register dst, const Oop* oop);
  
  // alias for platform independant code
  void mov    (Register dst, Register src) { movl(dst, src); }

  void movl   (const Address& dst, Register src);
  void movl   (const Address& dst, int imm32);
  void movl   (const Address& dst, const Oop* oop);

  void movb   (Register dst, const Address& src);
  void movb   (const Address& dst, int imm8);
  void movb   (const Address& dst, Register src);

  void movw   (Register dst, const Address& src);
  void movw   (const Address& dst, Register src);
  void movw   (const Address& dst, int imm16);
  
  // Move with zero/sign extension.
  void movsxb (Register dst, const Address& src);
  void movsxb (Register dst, Register src);
  void movsxw (Register dst, const Address& src);
  void movsxw (Register dst, Register src);
  
  void movzxb (Register dst, const Address& src);
  void movzxb (Register dst, Register src);
  void movzxw (Register dst, const Address& src);  
  void movzxw (Register dst, Register src);

  // Exchange contents of src and dst.
  void xchgl  (Register dst, const Address& src);

  // Arithmetic instructions.
  void addl   (const Address& dst, int imm32);
  void addl   (const Address& dst, Register src);
  void addl   (Register dst, int imm32) {
    generic_binary_1(dst, imm32, bin_add);
  }
  void addl   (Register dst, const Address& src);
  void addl   (Register dst, Register src) {
    generic_binary_1(dst, src, bin_add);
  }

  void adcl   (const Address& dst, int imm32);
  void adcl   (const Address& dst, Register src);
  void adcl   (Register dst, int imm32) {
    generic_binary_1(dst, imm32, bin_adc);
  }
  void adcl   (Register dst, const Address& src);
  void adcl   (Register dst, Register src) {
    generic_binary_1(dst, src, bin_adc);
  }

  void imull  (Register dst, Register src);
  void imull  (Register dst, Register src, int imm32);
  void imull  (Register dst, int imm32);
  void imull  (Register dst, const Address& src);

  void idivl  (Register divisor);

  void subl   (const Address& dst, int imm32);
  void subl   (const Address& dst, Register src);
  void subl   (Register dst, int imm32) {
    generic_binary_1(dst, imm32, bin_sub);
  }
  void subl   (Register dst, const Address& src);
  void subl   (Register dst, Register src) {
    generic_binary_1(dst, src, bin_sub);
  }

  void sbbl   (Register dst, int imm32);
  void sbbl   (Register dst, Register src);

  void negl   (const Address& dst);
  void negl   (Register dst);

  void incl   (Register dst);
  void incl   (const Address& dst);

  void decl   (Register dst);

  void cdql   ();

  // Logical operations.
  void andl   (const Address& dst, int imm32);
  void andl   (const Address& dst, Register src);
  void andl   (Register dst, int imm32) {
    generic_binary_1(dst, imm32, bin_and);
  }
  void andl   (Register dst, const Address& src);
  void andl   (Register dst, Register src) {
    generic_binary_1(dst, src, bin_and);
  }

  void orl    (const Address& dst, int imm32);
  void orl    (const Address& dst, Register src);
  void orl    (Register dst, int imm32) {
    generic_binary_1(dst, imm32, bin_or);
  }
  void orl    (Register dst, const Address& src);
  void orl    (Register dst, Register src) {
    generic_binary_1(dst, src, bin_or);
  }

  void xorl   (const Address& dst, int imm32);
  void xorl   (const Address& dst, Register src);
  void xorl   (Register dst, int imm32) {
    generic_binary_1(dst, imm32, bin_xor);
  }
  void xorl   (Register dst, const Address& src);
  void xorl   (Register dst, Register src) {
    generic_binary_1(dst, src, bin_xor);
  }

  void notl   (Register dst);

  // Bit vector operations
  void bts    (const Address& bit_base, Register bit_offset);

  // Stack manipulation instructions.
  void pushal ();
  void pushl  (Register src);
  void pushl  (const Address& adr);
  void pushl  (int imm32);
  void pushb  (int imm8);
  void pushl  (Oop* oop);

  void popal  ();
  void popl   (Register dst);
  void popl   (const Address& dst);

  // Load effective address instructions.
  void leal   (Register dst, const Address& src);

  // Test instructions.
  void testb  (Register dst, int imm8);

  void testl  (Register dst, Register src);
  void testl  (Register dst, int imm32);

  void testl  (const Address&  dst, Register src);
  void testl  (const Address&  dst, int imm32);

  // Compare instructions.
  void cmpl   (Register dst, Register src);
  void cmpl   (Register dst, int imm32);

  void cmpl   (const Address&  dst, Register src);
  void cmpl   (const Address&  dst, int imm32);

  // Shift instructions.
  void shll   (Register dst, int imm8);
  void shll   (Register dst, Register src);
  void shll   (const Address&  dst, Register src);

  void shrl   (Register dst, int imm8);
  void shrl   (Register dst, Register src);
  void shrl   (const Address&  dst, Register src);

  void sarl   (Register dst, Register src);
  void sarl   (const Address&  dst, Register src);
  void sarl   (Register dst, int imm8);

  // Calls.
  void call(Register reg)   { call_jmp(reg,   0xD0); }
  void call(address entry)  { call_jmp(entry, 0xE8); }

  void call_jmp(Register reg,  int opcode2);
  void call_jmp(address entry, int opcode);    

  // Jump instructions.
  void jmp    (Register reg) { call_jmp(reg,   0xE0); }
  void jmp    (address entry){ call_jmp(entry, 0xE9); }
  void jmp    (Label& L);
  void jmp    (NearLabel& L);
  void jmp    (CompilationQueueElement* cqe);

  // Conditional jump.
  void jcc    (Condition condition, Label& L);
  void jcc    (Condition condition, NearLabel& L);
  void jcc    (Condition condition, CompilationQueueElement* cqe);

  // Floating-point operations.
  void fld(Register dst, Register src);
  void fstp(Register dst, Register src);

  void fld1(Register dst);
  void fldz(Register dst);

  void fld_s(Register dst, const Address& adr);
  void fld_d(Register dst, const Address& adr);

  void fstp_s(const Address& adr, Register src);
  void fstp_d(const Address& adr, Register src);

  void fild_s(Register dst, const Address& adr);
  void fild_d(Register dst, const Address& adr);

  void fistp_s(const Address& adr, Register src);
  void fistp_d(const Address& adr, Register src);

  void fabs(Register dst);
  void fchs(Register dst);

  void faddp(Register dst, Register src);
  void fsubp(Register dst, Register src);
  void fsubrp(Register dst, Register src);
  void fmulp(Register dst, Register src);
  void fdivp(Register dst, Register src);
  void fdivrp(Register dst, Register src);
  void fremp(Register src, Register dst);
  void fprem();

  void fxch(Register dst);
  void fpop(Register dst); 
  void ffree(Register dst);

  void fucomip(Register dst, Register src);
  void fwait();
  void finit();

  void fldcw(const Address& src);
  void fnstcw(const Address& src);
  void fstsw_ax();

  // Miscellaneous instructions.
  void sahf   ();
  void int3   ();
  void breakpoint() { int3(); }
  void nop    ();
  void ret    (int k = 0);
  void hlt    ();

  // Instruction only used for debugging/printing
#ifndef PRODUCT
  void pushad();
  void popad();
  void pushfd();
  void popfd();
  void fnsave(const Address& dst);
  void frstor(const Address& src);
#endif // PRODUCT

  // Helpers routines
  void bind       (Label& L, int alignment = 0);
  void bind_to    (Label& L, jint code_offset);
  void bind       (NearLabel& L) { bind_to(L, _code_offset); }
  void bind_to    (NearLabel& L, int code_offset);

  void get_thread (Register dst);

  static void instruction_emitted( void ) {}

  void emit_byte(const jint value)  { emit_code_byte ( value ); }
  void emit_word(const jint value)  { emit_code_short( value ); }
  void emit_long(const jint value)  { emit_code_int  ( value ); }

  void emit_displacement(Label& L);
  void emit_displacement(NearLabel& L);

  // Helper functions for groups of instructions

  void emit_operand(Register reg, Register base, Register index,
        const ScaleFactor scale, int disp,
                    Relocation::Kind reloc);
  inline void emit_operand(Register reg, const Address& adr,
         Relocation::Kind reloc = Relocation::no_relocation);

  void emit_arith   (int op1, int op2, Register dst, int imm32);
  void emit_arith   (int op1, int op2, Register dst, Register src);
  void emit_arith_b (int op1, int op2, Register dst, int imm8);

  void emit_farith  (int op1, int op2, int stack_offset);

  void emit_data(int data,
                 Relocation::Kind reloc = Relocation::no_relocation);

  // Returns the code size in bytes

  void generate_sentinel() {
    hlt();
    emit_sentinel();     
  }

 private:
  static FPURegisterMap& fpu_register_map( void );

  jint  long_at    (const int position) const;
  void  word_at_put(const int position, const jshort value) const;
  void  long_at_put(const int position, const jint value) const;

  static bool is_signed_byte  ( const int data );
  static bool is_unsigned_byte( const int data );
#endif
};

#endif // ENABLE_COMPILER
