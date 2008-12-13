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

 private:
  Stream* _stream;

#if ENABLE_ARM_VFP
  void emit_vfp_register_list(int instr);
  void emit_vfp_instruction(int instr, int instr_offset);
  void unknown_vfp_instr(int instr);
#endif

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
  Stream* stream( void ) const { return _stream; }

  enum { NO_OFFSET = -1 };

#if ENABLE_THUMB_COMPILER
 private:
  static const char* _eol_comments;

 public:
  static void eol_comment( const char* s ) {
    _eol_comments = s;
  }
  // Returns the number of half-words disassembled
  int disasm(short* addr, short instr, int instr_offset = NO_OFFSET);
#else
  void disasm(int* addr, int instr, int instr_offset = NO_OFFSET);
#endif

#if ENABLE_ARM_VFP
  // type is either 's' (float) or 'd' (double)
  static void vfp_reg_name(const char type, unsigned reg, char buff[]);
#endif
