/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#if ENABLE_THUMB_COMPILER
#if !defined(PRODUCT) || ENABLE_TTY_TRACE


/*
 * Warning: ENABLE_THUMB_COMPILER=true is not a supported feature
 * This code is included only for reference purposes.
 */

class Disassembler: public StackObj {
 public:
  typedef Assembler::Register   Register;
  typedef Assembler::Condition  Condition;
  typedef Assembler::Shift      Shift;
  typedef Assembler::Opcode     Opcode;

  static Register as_register( const unsigned n ) {
    return Assembler::as_register( n );
  }
  static Condition as_condition( const unsigned n ) {
    return Assembler::as_condition( n );
  }
  static Shift as_shift( const unsigned n ) {
    return Assembler::as_shift( n );
  }
  static Opcode as_opcode( const unsigned n ) {
    return Assembler::as_opcode( n );
  }

  // textual representation
  static const char* condition_name     (const Assembler::Condition cond  );
  static const char* shift_name         (const Assembler::Shift     shift );
  static const char* opcode_name        (const Assembler::Opcode    opcode);
  static const char* register_name      (const Assembler::Register  reg   );

  static const char* condition_name ( const unsigned cond ) {
    return condition_name( as_condition( cond ) );
  }
  static const char* shift_name ( const unsigned shift ) {
    return shift_name( as_shift( shift ) );
  }
  static const char* opcode_name ( const unsigned opcode) {
    return opcode_name( as_opcode( opcode ) );
  }
  static const char* register_name ( const unsigned reg ) {
    return register_name( as_register( reg ) );
  }

  // creation
  Disassembler(Stream* stream) : _stream(stream) {}

  // accessors
  Stream* stream() const { return _stream; }

  enum { NO_OFFSET = -1 };
  // Returns the number of half-words disassembled
  int disasm(short* addr, short instr, int instr_offset = NO_OFFSET);

  static void eol_comment(const char *s) {
    _eol_comments = s;
  }

 private:
  Stream* _stream;
  static const char *_eol_comments;
  static Assembler::Register _last_imm_register;
  static int _last_imm_offset;
  static int _last_imm;

  // instruction fields
  static const bool bit(short instr, int i) {
    return (instr >> i & 0x1) == 1;
  }

  static const Assembler::Register reg_field(short instr, int shift = 0) {
    return Assembler::as_register((instr >> shift) & 0x7);
  }

  static const Assembler::Register reg_field_w(int instr, int shift = 0) {
    return Assembler::as_register((instr >> shift) & 0x0f);
  }

  // disassembler
  void emit_unknown(short instr, const char* comment = NULL);
  void emit_register_list(short instr);

  void print_gp_name(int imm);
  const char *find_gp_name(int imm);
};

#endif // !defined(PRODUCT) || ENABLE_TTY_TRACE

#endif /*#if ENABLE_THUMB_COMPILER*/
