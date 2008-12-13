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

#ifndef PRODUCT

#if defined(LINUX) || defined (CYGWIN)
#define USE_INVERTED_ORDER 1
#endif

class SourceAssembler: public Assembler {
 public:
  class Label
  {
   public:
    // Constructor.
    Label() : _id(next_id()) { }

    // Accessor for id.
    int id() const { return _id; }

   private:
    int        _id;

    // Static method for getting the next available label id.
    static int _next_id;
    static int  next_id() { return _next_id++; }
  };

  class Constant {
   public:
    enum ConstantType {
      immediate_constant         = 0,
      reference_constant         = 1,
      label_constant             = 2,
    };

    // Constructors.
    explicit Constant(int immediate) :
      _type(immediate_constant), _immediate(immediate), _isproc(false)
    { }
    explicit Constant(const char* reference, int offset = 0) :
      _type(reference_constant), _immediate(offset), _reference(reference),
      _isproc(false)
    { }
    explicit Constant(const char* reference, bool is_proc, int offset) :
      _type(reference_constant), _immediate(offset), _reference(reference),
      _isproc(is_proc)
    { }
    explicit Constant(const Label& label, int offset = 0) :
      _type(label_constant), _immediate(offset), _label(label), _isproc(false)
    { }

    // Type accessors.
    bool is_immediate() const { return type() == immediate_constant; }
    bool is_reference() const { return type() == reference_constant; }
    bool is_label()     const { return type() == label_constant;     }

    // Accessors.
    ConstantType type()      const { return _type; }
    const int    immediate() const {
      GUARANTEE(is_immediate(), "Must be immediate");
      return _immediate;
    }
    const char*  reference() const {
      GUARANTEE(is_reference(), "Must be reference");
      return _reference;
    }
    const int    offset()     const {
      GUARANTEE(!is_immediate(), "Must not be immediate");
      return _immediate;
    }
    const Label& label()     const {
        GUARANTEE(is_label(), "Must be label");
        return _label;
    }
    const bool is_proc()      const { return _isproc;     }

   private:
    ConstantType  _type;
    const int     _immediate;
    const char*   _reference;
    const Label   _label;
    const bool    _isproc;
  };

  class Address {
   public:
    Address(const Constant& displacement) :
      _displacement(displacement), _base(no_reg), _index(no_reg), _scale(no_scale)
    { }
    Address(const Register base, const Constant& displacement = Constant(0)) :
      _displacement(displacement), _base(base), _index(no_reg), _scale(no_scale)
    { }
    Address(const Register base, const Register index, ScaleFactor scale, const Constant& displacement = Constant(0)) :
      _displacement(displacement), _base(base), _index(index), _scale(scale)
    { }

    // Accessors.
    const Register     base        (void) const { return _base;         }
    const Register     index       (void) const { return _index;        }
    const ScaleFactor  scale       (void) const { return _scale;        }
    const Constant&    displacement(void) const { return _displacement; }

   private:
    const Register     _base;
    const Register     _index;
    const ScaleFactor  _scale;
    const Constant     _displacement;
  };

  enum OperandSize
  {
     byte_operand,
     word_operand,
     long_operand,
     very_long_operand,
     very_very_long_operand,
  };

  #define INSTRUCTION_0(name, format)                         \
    void name() {                                             \
      emit_instruction(format);                               \
    }

  #define INSTRUCTION_1(name, format, t1)                     \
    void name(const t1& p1) {                                 \
      emit_instruction(format, &p1);                          \
    }
#ifdef USE_INVERTED_ORDER
  #define INSTRUCTION_2(name, format, t1, t2)                 \
    void name(const t1& p1, const t2& p2) {                   \
      emit_instruction(format, &p2, &p1); /* p1,p2 switched*/ \
    }
#else
  #define INSTRUCTION_2(name, format, t1, t2)                 \
    void name(const t1& p1, const t2& p2) {                   \
      emit_instruction(format, &p1, &p2);                     \
    }
#endif

  #define INSTRUCTION_3(name, format, t1, t2, t3)             \
    void name(const t1& p1, const t2& p2, const t3& p3) {     \
      emit_instruction(format, &p1, &p2, &p3);                \
    }

 public:
  // Constructor and deconstructor for the source assembler.
  SourceAssembler (Stream* output);
 ~SourceAssembler ();

  // Start and stop the assembler.
  void start();
  void stop ();

 public:
  // Start and stop segments in the assembler code.
  void start_code_segment ();
  void start_data_segment ();
  void stop_code_segment  ();
  void stop_data_segment  ();

  // Stack operations.
#ifdef USE_INVERTED_ORDER
  INSTRUCTION_0(pushal , "pushal" );
  INSTRUCTION_0(popal  , "popal"  );
  INSTRUCTION_0(pushfl , "pushfl" );
  INSTRUCTION_0(popfl  , "popfl"  );

  INSTRUCTION_1(pushl  , "pushl %c"  , Constant );
  INSTRUCTION_1(pushl  , "pushl %lr" , Register );
  INSTRUCTION_1(pushl  , "pushl %la" , Address  );

  INSTRUCTION_1(popl   , "popl %lr"  , Register );
  INSTRUCTION_1(popl   , "popl %la"  , Address  );

  // Ordinary moves.
  INSTRUCTION_2(movb   , "movb %br, %ba"   , Register, Address );
  INSTRUCTION_2(movb   , "movb %br, %br"   , Register, Register);
  INSTRUCTION_2(movb   , "movb %br, %c"    , Register, Constant);
  INSTRUCTION_2(movb   , "movb %ba, %br"   , Address , Register);
  INSTRUCTION_2(movb   , "movb %ba, %c"    , Address , Constant);

  INSTRUCTION_2(movw   , "movw %wr, %wa"   , Register, Address );
  INSTRUCTION_2(movw   , "movw %wr, %wr"   , Register, Register);
  INSTRUCTION_2(movw   , "movw %wr, %c"    , Register, Constant);
  INSTRUCTION_2(movw   , "movw %wa, %wr"   , Address , Register);
  INSTRUCTION_2(movw   , "movw  %wa, %c"    , Address , Constant);

  INSTRUCTION_2(movl   , "movl %lr, %la"   , Register, Address );
  INSTRUCTION_2(movl   , "movl %lr, %lr"   , Register, Register);
  INSTRUCTION_2(movl   , "movl %lr, %c"    , Register, Constant);
  INSTRUCTION_2(movl   , "movl %la, %lr"   , Address , Register);
  INSTRUCTION_2(movl   , "movl %la, %c"    , Address , Constant);

  // Moves with sign extension.
  INSTRUCTION_2(movsxb , "movsbl %lr, %br" , Register, Register);
  INSTRUCTION_2(movsxb , "movsbl %lr, %ba" , Register, Address );

  INSTRUCTION_2(movsxw , "movswl %lr, %wr" , Register, Register);
  INSTRUCTION_2(movsxw , "movswl %lr, %wa" , Register, Address );

  // Moves with zero extension.
  INSTRUCTION_2(movzxb , "movzbl %lr, %br" , Register, Register);
  INSTRUCTION_2(movzxb , "movzbl %lr, %ba" , Register, Address );

  INSTRUCTION_2(movzxw , "movzwl %lr, %wr" , Register, Register);
  INSTRUCTION_2(movzxw , "movzwl %lr, %wa" , Register, Address );
#else
  INSTRUCTION_0(pushal , "pushad" );
  INSTRUCTION_0(popal  , "popad"  );
  INSTRUCTION_0(pushfl , "pushfd" );
  INSTRUCTION_0(popfl  , "popfd"  );

  INSTRUCTION_1(pushl  , "push %c"  , Constant );
  INSTRUCTION_1(pushl  , "push %lr" , Register );
  INSTRUCTION_1(pushl  , "push %la" , Address  );
  INSTRUCTION_1(popl   , "pop %lr"  , Register );
  INSTRUCTION_1(popl   , "pop %la"  , Address  );

  // Ordinary moves.
  INSTRUCTION_2(movb   , "mov %br, %ba"   , Register, Address );
  INSTRUCTION_2(movb   , "mov %br, %br"   , Register, Register);
  INSTRUCTION_2(movb   , "mov %br, %c"    , Register, Constant);
  INSTRUCTION_2(movb   , "mov %ba, %br"   , Address , Register);
  INSTRUCTION_2(movb   , "mov %ba, %c"    , Address , Constant);

  INSTRUCTION_2(movw   , "mov %wr, %wa"   , Register, Address );
  INSTRUCTION_2(movw   , "mov %wr, %wr"   , Register, Register);
  INSTRUCTION_2(movw   , "mov %wr, %c"    , Register, Constant);
  INSTRUCTION_2(movw   , "mov %wa, %wr"   , Address , Register);
  INSTRUCTION_2(movw   , "mov %wa, %c"    , Address , Constant);

  INSTRUCTION_2(movl   , "mov %lr, %la"   , Register, Address );
  INSTRUCTION_2(movl   , "mov %lr, %lr"   , Register, Register);
  INSTRUCTION_2(movl   , "mov %lr, %c"    , Register, Constant);
  INSTRUCTION_2(movl   , "mov %la, %lr"   , Address , Register);
  INSTRUCTION_2(movl   , "mov %la, %c"    , Address , Constant);

  // Moves with sign extension.
  INSTRUCTION_2(movsxb , "movsx %lr, %br" , Register, Register);
  INSTRUCTION_2(movsxb , "movsx %lr, %ba" , Register, Address );

  INSTRUCTION_2(movsxw , "movsx %lr, %wr" , Register, Register);
  INSTRUCTION_2(movsxw , "movsx %lr, %wa" , Register, Address );

  // Moves with zero extension.
  INSTRUCTION_2(movzxb , "movzx %lr, %br" , Register, Register);
  INSTRUCTION_2(movzxb , "movzx %lr, %ba" , Register, Address );

  INSTRUCTION_2(movzxw , "movzx %lr, %wr" , Register, Register);
  INSTRUCTION_2(movzxw , "movzx %lr, %wa" , Register, Address );
#endif

  // Ror
  INSTRUCTION_2(ror, "ror %lr, %c", Register, Constant);

  // Xchg
  INSTRUCTION_2(xchg, "xchg %br, %br", Register, Register);

  // Miscellaneous without operands.
#ifdef USE_INVERTED_ORDER
  INSTRUCTION_0(cdql   , "cltd"   );
#else
  INSTRUCTION_0(cdql   , "cdq"   );
#endif
  INSTRUCTION_0(int3   , "int 3" );
  INSTRUCTION_0(hlt    , "hlt"   );
  INSTRUCTION_0(lock   , "lock"  );
  INSTRUCTION_0(nop    , "nop"   );
  INSTRUCTION_0(ret    , "ret"   );
  INSTRUCTION_0(sahf   , "sahf"  );

  // Miscellaneous with one operand.
  INSTRUCTION_1(bswap  , "bswap %lr"      , Register);

  INSTRUCTION_1(call   , "call %c"        , Constant);
  INSTRUCTION_1(call   , "call %lr"       , Register);
  INSTRUCTION_1(call   , "call %la"       , Address );

#ifdef USE_INVERTED_ORDER
  INSTRUCTION_1(decl   , "decl %lr"        , Register);
  INSTRUCTION_1(decl   , "decl %la"        , Address );

  INSTRUCTION_1(idivl  , "idivl %lr"       , Register);
  INSTRUCTION_1(idivl  , "idivl %la"       , Address );

  INSTRUCTION_1(imull  , "imull %lr"       , Register);
  INSTRUCTION_1(imull  , "imull %la"       , Address );

  INSTRUCTION_1(incl   , "incl %lr"        , Register);
  INSTRUCTION_1(incl   , "incl %la"        , Address );

  INSTRUCTION_1(jmp    , "jmp %c"         , Constant);
  INSTRUCTION_1(jmp    , "jmp %lr"        , Register);
  INSTRUCTION_1(jmp    , "jmp %la"        , Address );

  INSTRUCTION_1(loop   , "loop %c"        , Constant);

  INSTRUCTION_1(mull   , "mull %lr"        , Register);
  INSTRUCTION_1(mull   , "mull %la"        , Address );

  INSTRUCTION_1(negl   , "negl %lr"        , Register);
  INSTRUCTION_1(negl   , "negl %la"        , Address );

  INSTRUCTION_1(notl   , "notl %lr"        , Register);
  INSTRUCTION_1(notl   , "notl %la"        , Address );

  INSTRUCTION_1(ret    , "ret %c"         , Constant);

  INSTRUCTION_1(sarl   , "sarl %lr, cl"    , Register);
  INSTRUCTION_1(sarl   , "sarl %la, cl"    , Address );

  INSTRUCTION_1(shrl   , "shrl %lr, cl"    , Register);
  INSTRUCTION_1(shrl   , "shrl %la, cl"    , Address );

  INSTRUCTION_1(shll   , "shll %lr, cl"    , Register);
  INSTRUCTION_1(shll   , "shll %la, cl"    , Address );

  // Miscellaneous with two operands.
  INSTRUCTION_2(adcl   , "adcl %lr, %c"    , Register, Constant);
  INSTRUCTION_2(adcl   , "adcl %lr, %lr"   , Register, Register);
  INSTRUCTION_2(adcl   , "adcl %lr, %la"   , Register, Address );
  INSTRUCTION_2(adcl   , "adcl %la, %c"    , Address , Constant);
  INSTRUCTION_2(adcl   , "adcl %la, %lr"   , Address , Register);

  INSTRUCTION_2(addl   , "addl %lr, %c"    , Register, Constant);
  INSTRUCTION_2(addl   , "addl %lr, %lr"   , Register, Register);
  INSTRUCTION_2(addl   , "addl %lr, %la"   , Register, Address );
  INSTRUCTION_2(addl   , "addl %la, %c"    , Address , Constant);
  INSTRUCTION_2(addl   , "addl %la, %lr"   , Address , Register);

  INSTRUCTION_2(andl   , "andl %lr, %c"    , Register, Constant);
  INSTRUCTION_2(andl   , "andl %lr, %lr"   , Register, Register);
  INSTRUCTION_2(andl   , "andl %lr, %la"   , Register, Address );
  INSTRUCTION_2(andl   , "andl %la, %c"    , Address , Constant);
  INSTRUCTION_2(andl   , "andl %la, %lr"   , Address , Register);

  INSTRUCTION_2(cmpl   , "cmpl %lr, %c"    , Register, Constant);
  INSTRUCTION_2(cmpl   , "cmpl %lr, %lr"   , Register, Register);
  INSTRUCTION_2(cmpl   , "cmpl %lr, %la"   , Register, Address );
  INSTRUCTION_2(cmpl   , "cmpl %la, %c"    , Address , Constant);
  INSTRUCTION_2(cmpl   , "cmpl %la, %lr"   , Address , Register);

  INSTRUCTION_2(cmpw   , "cmpw %wr, %c"    , Register, Constant);
  INSTRUCTION_2(cmpw   , "cmpw %wr, %wr"   , Register, Register);
  INSTRUCTION_2(cmpw   , "cmpw %wr, %wa"   , Register, Address );
  INSTRUCTION_2(cmpw   , "cmpw %wa, %c"    , Address , Constant);
  INSTRUCTION_2(cmpw   , "cmpw %wa, %wr"   , Address , Register);

  INSTRUCTION_2(cmpb   , "cmpb %br, %c"    , Register, Constant);
  INSTRUCTION_2(cmpb   , "cmpb %br, %br"   , Register, Register);
  INSTRUCTION_2(cmpb   , "cmpb %br, %ba"   , Register, Address );
  INSTRUCTION_2(cmpb   , "cmpb %ba, %c"    , Address , Constant);
  INSTRUCTION_2(cmpb   , "cmpb %ba, %br"   , Address , Register);

  INSTRUCTION_2(imull  , "imull %lr, %c"   , Register, Constant);
  INSTRUCTION_2(imull  , "imull %lr, %lr"  , Register, Register);
  INSTRUCTION_2(imull  , "imull %lr, %la"  , Register, Address );

  INSTRUCTION_2(leal   , "leal %lr, %la"   , Register, Address );

  INSTRUCTION_2(orl    , "orl %lr, %c"     , Register, Constant);
  INSTRUCTION_2(orl    , "orl %lr, %lr"    , Register, Register);
  INSTRUCTION_2(orl    , "orl %lr, %la"    , Register, Address );
  INSTRUCTION_2(orl    , "orl %la, %c"     , Address , Constant);
  INSTRUCTION_2(orl    , "orl %la, %lr"    , Address , Register);

  INSTRUCTION_2(orw    , "orw %wr, %c"     , Register, Constant);
  INSTRUCTION_2(orw    , "orw %wr, %wr"    , Register, Register);
  INSTRUCTION_2(orw    , "orw %wr, %wa"    , Register, Address );
  INSTRUCTION_2(orw    , "orw %wa, %c"     , Address , Constant);
  INSTRUCTION_2(orw    , "orw %wa, %wr"    , Address , Register);

  INSTRUCTION_2(sarl   , "sarl %lr, %c"    , Register, Constant);
  INSTRUCTION_2(sarl   , "sarl %la, %c"    , Address , Constant);

  INSTRUCTION_2(shrl   , "shrl %lr, %c"    , Register, Constant);
  INSTRUCTION_2(shrl   , "shrl %la, %c"    , Address , Constant);

  INSTRUCTION_2(shll   , "shll %lr, %c"    , Register, Constant);
  INSTRUCTION_2(shll   , "shll %la, %c"    , Address , Constant);

  INSTRUCTION_2(shrdl  , "shrd %lr, %lr" , Register, Register);
  INSTRUCTION_2(shldl  , "shld %lr, %lr" , Register, Register);

  INSTRUCTION_2(sbbl   , "sbbl %lr, %c"    , Register, Constant);
  INSTRUCTION_2(sbbl   , "sbbl %lr, %lr"   , Register, Register);
  INSTRUCTION_2(sbbl   , "sbbl %lr, %la"   , Register, Address );
  INSTRUCTION_2(sbbl   , "sbbl %la, %c"    , Address , Constant);
  INSTRUCTION_2(sbbl   , "sbbl %la, %lr"   , Address , Register);

  INSTRUCTION_2(subl   , "subl %lr, %c"    , Register, Constant);
  INSTRUCTION_2(subl   , "subl %lr, %lr"   , Register, Register);
  INSTRUCTION_2(subl   , "subl %lr, %la"   , Register, Address );
  INSTRUCTION_2(subl   , "subl %la, %c"    , Address , Constant);
  INSTRUCTION_2(subl   , "subl %la, %lr"   , Address , Register);

  INSTRUCTION_2(subw   , "subw %wr, %wa"   , Register, Address );

  INSTRUCTION_2(testb  , "testb %br, %c"   , Register, Constant);
  INSTRUCTION_2(testb  , "testb %br, %br"  , Register, Register);
  INSTRUCTION_2(testb  , "testb %br, %ba"  , Register, Address );
  INSTRUCTION_2(testb  , "testb %ba, %c"   , Address , Constant);
  INSTRUCTION_2(testb  , "testb %ba, %br"  , Address , Register);

  INSTRUCTION_2(testl  , "testl %lr, %c"   , Register, Constant);
  INSTRUCTION_2(testl  , "testl %lr, %lr"  , Register, Register);
  INSTRUCTION_2(testl  , "testl %lr, %la"  , Register, Address );
  INSTRUCTION_2(testl  , "testl %la, %c"   , Address , Constant);
  INSTRUCTION_2(testl  , "testl %la, %lr"  , Address , Register);

  INSTRUCTION_2(xorl   , "xorl %lr, %c"    , Register, Constant);
  INSTRUCTION_2(xorl   , "xorl %lr, %lr"   , Register, Register);
  INSTRUCTION_2(xorl   , "xorl %lr, %la"   , Register, Address );
  INSTRUCTION_2(xorl   , "xorl %la, %c"    , Address , Constant);
  INSTRUCTION_2(xorl   , "xorl %la, %lr"   , Address , Register);

  INSTRUCTION_2(btl    , "btl %lr, %lr"    , Register, Register);

  INSTRUCTION_2(btsl   , "btsl %lr, %lr"   , Register, Register);
  INSTRUCTION_2(btsl   , "btsl %la, %lr"   , Address , Register);
#else
  INSTRUCTION_1(decl   , "dec %lr"        , Register);
  INSTRUCTION_1(decl   , "dec %la"        , Address );

  INSTRUCTION_1(idivl  , "idiv %lr"       , Register);
  INSTRUCTION_1(idivl  , "idiv %la"       , Address );

  INSTRUCTION_1(imull  , "imul %lr"       , Register);
  INSTRUCTION_1(imull  , "imul %la"       , Address );

  INSTRUCTION_1(incl   , "inc %lr"        , Register);
  INSTRUCTION_1(incl   , "inc %la"        , Address );

  INSTRUCTION_1(jmp    , "jmp %c"         , Constant);
  INSTRUCTION_1(jmp    , "jmp %lr"        , Register);
  INSTRUCTION_1(jmp    , "jmp %la"        , Address );

  INSTRUCTION_1(loop   , "loop %c"        , Constant);

  INSTRUCTION_1(mull   , "mul %lr"        , Register);
  INSTRUCTION_1(mull   , "mul %la"        , Address );

  INSTRUCTION_1(negl   , "neg %lr"        , Register);
  INSTRUCTION_1(negl   , "neg %la"        , Address );

  INSTRUCTION_1(notl   , "not %lr"        , Register);
  INSTRUCTION_1(notl   , "not %la"        , Address );

  INSTRUCTION_1(ret    , "ret %c"         , Constant);

  INSTRUCTION_1(sarl   , "sar %lr, cl"    , Register);
  INSTRUCTION_1(sarl   , "sar %la, cl"    , Address );

  INSTRUCTION_1(shrl   , "shr %lr, cl"    , Register);
  INSTRUCTION_1(shrl   , "shr %la, cl"    , Address );

  INSTRUCTION_1(shll   , "shl %lr, cl"    , Register);
  INSTRUCTION_1(shll   , "shl %la, cl"    , Address );

  // Miscellaneous with two operands.
  INSTRUCTION_2(adcl   , "adc %lr, %c"    , Register, Constant);
  INSTRUCTION_2(adcl   , "adc %lr, %lr"   , Register, Register);
  INSTRUCTION_2(adcl   , "adc %lr, %la"   , Register, Address );
  INSTRUCTION_2(adcl   , "adc %la, %c"    , Address , Constant);
  INSTRUCTION_2(adcl   , "adc %la, %lr"   , Address , Register);

  INSTRUCTION_2(addl   , "add %lr, %c"    , Register, Constant);
  INSTRUCTION_2(addl   , "add %lr, %lr"   , Register, Register);
  INSTRUCTION_2(addl   , "add %lr, %la"   , Register, Address );
  INSTRUCTION_2(addl   , "add %la, %c"    , Address , Constant);
  INSTRUCTION_2(addl   , "add %la, %lr"   , Address , Register);

  INSTRUCTION_2(andl   , "and %lr, %c"    , Register, Constant);
  INSTRUCTION_2(andl   , "and %lr, %lr"   , Register, Register);
  INSTRUCTION_2(andl   , "and %lr, %la"   , Register, Address );
  INSTRUCTION_2(andl   , "and %la, %c"    , Address , Constant);
  INSTRUCTION_2(andl   , "and %la, %lr"   , Address , Register);

  INSTRUCTION_2(cmpl   , "cmp %lr, %c"    , Register, Constant);
  INSTRUCTION_2(cmpl   , "cmp %lr, %lr"   , Register, Register);
  INSTRUCTION_2(cmpl   , "cmp %lr, %la"   , Register, Address );
  INSTRUCTION_2(cmpl   , "cmp %la, %c"    , Address , Constant);
  INSTRUCTION_2(cmpl   , "cmp %la, %lr"   , Address , Register);

  INSTRUCTION_2(cmpw   , "cmp %wr, %c"    , Register, Constant);
  INSTRUCTION_2(cmpw   , "cmp %wr, %wr"   , Register, Register);
  INSTRUCTION_2(cmpw   , "cmp %wr, %wa"   , Register, Address );
  INSTRUCTION_2(cmpw   , "cmp %wa, %c"    , Address , Constant);
  INSTRUCTION_2(cmpw   , "cmp %wa, %wr"   , Address , Register);

  INSTRUCTION_2(cmpb   , "cmp %br, %c"    , Register, Constant);
  INSTRUCTION_2(cmpb   , "cmp %br, %br"   , Register, Register);
  INSTRUCTION_2(cmpb   , "cmp %br, %ba"   , Register, Address );
  INSTRUCTION_2(cmpb   , "cmp %ba, %c"    , Address , Constant);
  INSTRUCTION_2(cmpb   , "cmp %ba, %br"   , Address , Register);

  INSTRUCTION_2(imull  , "imul %lr, %c"   , Register, Constant);
  INSTRUCTION_2(imull  , "imul %lr, %lr"  , Register, Register);
  INSTRUCTION_2(imull  , "imul %lr, %la"  , Register, Address );

  INSTRUCTION_2(leal   , "lea %lr, %la"   , Register, Address );

  INSTRUCTION_2(orl    , "or %lr, %c"     , Register, Constant);
  INSTRUCTION_2(orl    , "or %lr, %lr"    , Register, Register);
  INSTRUCTION_2(orl    , "or %lr, %la"    , Register, Address );
  INSTRUCTION_2(orl    , "or %la, %c"     , Address , Constant);
  INSTRUCTION_2(orl    , "or %la, %lr"    , Address , Register);

  INSTRUCTION_2(orw    , "or %wr, %c"     , Register, Constant);
  INSTRUCTION_2(orw    , "or %wr, %wr"    , Register, Register);
  INSTRUCTION_2(orw    , "or %wr, %wa"    , Register, Address );
  INSTRUCTION_2(orw    , "or %wa, %c"     , Address , Constant);
  INSTRUCTION_2(orw    , "or %wa, %wr"    , Address , Register);

  INSTRUCTION_2(sarl   , "sar %lr, %c"    , Register, Constant);
  INSTRUCTION_2(sarl   , "sar %la, %c"    , Address , Constant);

  INSTRUCTION_2(shrl   , "shr %lr, %c"    , Register, Constant);
  INSTRUCTION_2(shrl   , "shr %la, %c"    , Address , Constant);

  INSTRUCTION_2(shll   , "shl %lr, %c"    , Register, Constant);
  INSTRUCTION_2(shll   , "shl %la, %c"    , Address , Constant);

  INSTRUCTION_2(shrdl  , "shrd %lr, %lr, cl" , Register, Register);
  INSTRUCTION_2(shldl  , "shld %lr, %lr, cl" , Register, Register);

  INSTRUCTION_2(sbbl   , "sbb %lr, %c"    , Register, Constant);
  INSTRUCTION_2(sbbl   , "sbb %lr, %lr"   , Register, Register);
  INSTRUCTION_2(sbbl   , "sbb %lr, %la"   , Register, Address );
  INSTRUCTION_2(sbbl   , "sbb %la, %c"    , Address , Constant);
  INSTRUCTION_2(sbbl   , "sbb %la, %lr"   , Address , Register);

  INSTRUCTION_2(subl   , "sub %lr, %c"    , Register, Constant);
  INSTRUCTION_2(subl   , "sub %lr, %lr"   , Register, Register);
  INSTRUCTION_2(subl   , "sub %lr, %la"   , Register, Address );
  INSTRUCTION_2(subl   , "sub %la, %c"    , Address , Constant);
  INSTRUCTION_2(subl   , "sub %la, %lr"   , Address , Register);

  INSTRUCTION_2(subw   , "sub %wr, %wa"   , Register, Address );

  INSTRUCTION_2(testb  , "test %br, %c"   , Register, Constant);
  INSTRUCTION_2(testb  , "test %br, %br"  , Register, Register);
  INSTRUCTION_2(testb  , "test %br, %ba"  , Register, Address );
  INSTRUCTION_2(testb  , "test %ba, %c"   , Address , Constant);
  INSTRUCTION_2(testb  , "test %ba, %br"  , Address , Register);

  INSTRUCTION_2(testl  , "test %lr, %c"   , Register, Constant);
  INSTRUCTION_2(testl  , "test %lr, %lr"  , Register, Register);
  INSTRUCTION_2(testl  , "test %lr, %la"  , Register, Address );
  INSTRUCTION_2(testl  , "test %la, %c"   , Address , Constant);
  INSTRUCTION_2(testl  , "test %la, %lr"  , Address , Register);

  INSTRUCTION_2(xorl   , "xor %lr, %c"    , Register, Constant);
  INSTRUCTION_2(xorl   , "xor %lr, %lr"   , Register, Register);
  INSTRUCTION_2(xorl   , "xor %lr, %la"   , Register, Address );
  INSTRUCTION_2(xorl   , "xor %la, %c"    , Address , Constant);
  INSTRUCTION_2(xorl   , "xor %la, %lr"   , Address , Register);

  INSTRUCTION_2(btl    , "bt %lr, %lr"    , Register, Register);

  INSTRUCTION_2(btsl   , "bts %lr, %lr"   , Register, Register);
  INSTRUCTION_2(btsl   , "bts %la, %lr"   , Address , Register);
#endif

  // Miscellaneous with three operands.
  INSTRUCTION_3(imull  , "imul %lr, %lr, %c" , Register, Register, Constant);
  INSTRUCTION_3(imull  , "imul %lr, %la, %c" , Register, Address , Constant);

  // Zero-operand floating point operations.
  INSTRUCTION_0(fchs     , "fchs"       );
  INSTRUCTION_0(fxch     , "fxch"       );
#ifdef USE_INVERTED_ORDER
  /* we use fsubrp and fdivrp to workaround syntax in X86 as:
   * it incorrectly swaps operands for some obscure compatibility reasons
   * with AT&T ASM
   */
  INSTRUCTION_0(faddp    , "faddp"       );
  INSTRUCTION_0(fsubp    , "fsubrp"       );
  INSTRUCTION_0(fmulp    , "fmulp"       );
  INSTRUCTION_0(fdivp    , "fdivrp"       );
#else
  INSTRUCTION_0(faddp    , "fadd"       );
  INSTRUCTION_0(fsubp    , "fsub"       );
  INSTRUCTION_0(fmulp    , "fmul"       );
  INSTRUCTION_0(fdivp    , "fdiv"       );
#endif
  INSTRUCTION_0(fsin     , "fsin"       );
  INSTRUCTION_0(fcos     , "fcos"       );
  INSTRUCTION_0(fsqrt    , "fsqrt"      );
  INSTRUCTION_0(fprem    , "fprem"      );
  INSTRUCTION_0(fwait    , "fwait"      );
  INSTRUCTION_0(fincstp  , "fincstp"    );
  INSTRUCTION_0(fcompp   , "fcompp"     );
  INSTRUCTION_0(fld1     , "fld1"       );
  INSTRUCTION_0(fyl2x    , "fyl2x"      );
  INSTRUCTION_0(fldl2e   , "fldl2e"     );
  INSTRUCTION_0(f2xm1    , "f2xm1"      );
  INSTRUCTION_0(frndint  , "frndint"    );
  INSTRUCTION_0(fscale   , "fscale"     );

  // One-operand floating point operations.
#ifdef USE_INVERTED_ORDER
  INSTRUCTION_1(fld_f  , "flds %la"   , Address);
  INSTRUCTION_1(fld_d  , "fldl %va"   , Address);
  INSTRUCTION_1(fld_t  , "fldt %ta"   , Address);

  INSTRUCTION_1(fild_f , "fildl %la"   , Address);
  INSTRUCTION_1(fild_d , "fildll %va"  , Address);

  INSTRUCTION_1(fstp_f , "fstps %la"  , Address);
  INSTRUCTION_1(fstp_d , "fstpl %va"  , Address);
  INSTRUCTION_1(fstp_t , "fstpt %ta"  , Address);

  INSTRUCTION_1(fistp_f, "fistpl %la"   , Address);
  INSTRUCTION_1(fistp_d, "fistpll %va"  , Address);
#else
  INSTRUCTION_1(fld_f  , "fld %la"    , Address);
  INSTRUCTION_1(fld_d  , "fld %va"    , Address);
  INSTRUCTION_1(fld_t  , "fld %ta"    , Address);

  INSTRUCTION_1(fild_f , "fild %la"   , Address);
  INSTRUCTION_1(fild_d , "fild %va"   , Address);

  INSTRUCTION_1(fstp_f , "fstp %la"   , Address);
  INSTRUCTION_1(fstp_d , "fstp %va"   , Address);
  INSTRUCTION_1(fstp_t , "fstp %ta"   , Address);

  INSTRUCTION_1(fistp_f, "fistp %la"  , Address);
  INSTRUCTION_1(fistp_d, "fistp %va"  , Address);
#endif

  INSTRUCTION_1(fstcw  , "fstcw %wa"  , Address);
  INSTRUCTION_1(fldcw  , "fldcw %wa"  , Address);

  INSTRUCTION_1(fnstsw , "fnstsw %wr" , Register);

  // We should consider adding an integer argument (or two) to the following routines.
#ifdef USE_INVERTED_ORDER
  INSTRUCTION_0(ffree_st0    , "ffree %st(0)"       );
  INSTRUCTION_0(fld_st0      , "fld %st(0)"         );
  INSTRUCTION_0(fsub_st0_st1 , "fsub %st(0), %st(1)");
  INSTRUCTION_0(fstp_st1     , "fstp %st(1)"        );
#else
  INSTRUCTION_0(ffree_st0    , "ffree ST(0)"      );
  INSTRUCTION_0(fld_st0      , "fld ST(0)"        );
  INSTRUCTION_0(fsub_st0_st1 , "fsub ST(0), ST(1)");
  INSTRUCTION_0(fstp_st1     , "fstp ST(1)"       );
#endif
  #undef INSTRUCTION_0
  #undef INSTRUCTION_1
  #undef INSTRUCTION_2
  #undef INSTRUCTION_3

  // Conditional jumps.
  void jcc(Condition condition, const Constant& cst);

  // Define embedded call info.
  void define_call_info();

  // Insert alignment constraint.
  void align(int alignment);

  // Bind the given label to the current position.
  void bind(const Label& label, int alignment = 0);

  // Make a public entry.
  void entry(const char* name, int alignment = 16);
  void alt_entry(const char* name, int alignment = 0);
  void entry_end();

  // Make a public entry that can be linked by a ROM image.
  void rom_linkable_entry(const char* name, int alignment = 16);
  void rom_linkable_entry_end();

  // Support for defining data cells.
  void define_byte(const Constant& cst, int dups = 1, const char* name = 0) { emit_data(byte_operand, cst, dups, name); }
  void define_word(const Constant& cst, int dups = 1, const char* name = 0) { emit_data(word_operand, cst, dups, name); }
  void define_long(const Constant& cst, int dups = 1, const char* name = 0) { emit_data(long_operand, cst, dups, name); }

  void define_byte_variable(const char* c_type, const char* name,
                            const Constant& cst) {
    emit_variable(c_type, byte_operand, name, cst);
  }
  void define_word_variable(const char* c_type, const char* name,
                            const Constant& cst) {
    emit_variable(c_type, word_operand, name, cst);
  }
  void define_long_variable(const char* c_type, const char* name,
                            const Constant& cst) {
    emit_variable(c_type, long_operand, name, cst);
  }

  void define_array_begin(const char* c_type, const char* name, int size = -1);
  void define_byte_element(const Constant &cst) {
    emit_array_element(byte_operand, cst);
  }
  void define_word_element(const Constant &cst) {
    emit_array_element(word_operand, cst);
  }
  void define_long_element(const Constant &cst) {
    emit_array_element(long_operand, cst);
  }
  void define_array_end();

  // We assume there can be only one global instance of each struct type.
  void define_struct_begin(const char* struct_type_name,
                           const char* struct_name);
  void define_struct_field(const char* struct_name,
                           const char* c_type,
                           const char* name,
                           OperandSize size);
  void define_struct_end(const char* struct_name);

  // Support for commenting generated code.
  void comment          (const char* format, ...);
  void comment_section  (const char* format, ...);

  // Public method for emitting instructions.
  void emit_instruction (const char* format, ...);

  // Public method for emitting byte constants in text segment.  This
  // is the only way to emit data in text that VC++ inline asm supports.
  void emit_byte        (const Constant& cst);

  // A reference to an extern "C" global function --> some compilers (Cygwin)
  // requires a "_" prefix. See the +AddExternCUnderscore flag
  const char * extern_c_prefix();

  void emit_data_name(const char* c_type, const char* name);

 private:
  // The output stream.
  Stream* _output;

  // Are we generating code inside a function or not
  bool _inside_entry;

  // Helper function for negating conditions.
  Condition negate_cc   (Condition condition);

  // Helper function for emitting strings.
  void emit             (const char* format, ...);

  // Helper function for emitting data cells.
  void emit_data        (OperandSize size, const Constant& cst, int dups, const char* name);

  void emit_data_value(OperandSize size, const Constant& cst);
  void emit_variable(const char* c_type, OperandSize size,
                     const char* name, const Constant& cst);
  void emit_array_element(OperandSize size, const Constant &cst);

  // Helper routines for emitting instructions.
  void emit_instruction (const char* format, va_list arguments);

  // Helper function for emitting externals.
  bool emit_externals   (const char* format, va_list arguments);

  // Helper functions for emitting registers.
  void emit_register    (const Register reg, OperandSize size);

  // Helper function for emitting addresses.
  void emit_address     (const Address&  adr, OperandSize size);

  // Helper function for emitting constants.
  void emit_constant    (const Constant& cst);
  void emit_constant2    (const Constant& cst);
  void emit_constant_displacement    (const Constant& cst);

  // Helper function for conditions.
  void emit_cc          (Condition condition);

  // Helper function for emitting comments.
  void emit_comment     (const char* format, ...);
  void emit_comment     (const char* format, va_list arguments);

  char* reverse_format  (const char* format);

  static bool is_jvm_fast_global(const char * const reference);

  enum output_segment {
    NO_SEGMENT,
    CODE_SEGMENT,
    DATA_SEGMENT,
  };

  enum output_segment _current_segment;
};

#endif
