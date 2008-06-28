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

#ifndef PRODUCT

class Disassembler: public StackObj {
 public:

#if 0
#define DEF(Name, name)                                         \
  typedef Assembler::Name Name;                                 \
  static Name as_##name( const unsigned n ) {                   \
    return Assembler::as_##name( n );                           \
  }                                                             \
  static const char* name##_name ( const Name n );              \
  static const char* name##_name  ( const unsigned n ) {        \
    return name##_name( as_##name( n ) );                       \
  }
  DEF( Condition, condition )
  DEF( Register,  register  )
  DEF( Shift,     shift     )
  DEF( Opcode,    opcode    )
#undef DEF
#endif

  typedef Assembler::Condition  Condition;
  typedef Assembler::Register   Register;
  typedef Assembler::Shift      Shift;
  typedef Assembler::Opcode     Opcode;

  static Condition as_condition( const unsigned n ) {
    return Assembler::as_condition( n );
  }
  static Register as_register( const unsigned n ) {
    return Assembler::as_register( n );
  }
  static Shift as_shift( const unsigned n ) {
    return Assembler::as_shift( n );
  }
  static Opcode as_opcode( const unsigned n ) {
    return Assembler::as_opcode( n );
  }

  // textual representation
  static const char* condition_name  ( const Condition cond   );
  static const char* register_name   ( const Register  reg    );
  static const char* shift_name ( const Shift     shift  );
  static const char* opcode_name( const Opcode    opcode );

  static const char* condition_name  ( const unsigned cond  ) {
    return condition_name( as_condition( cond ) );
  }
  static const char* register_name   ( const unsigned reg   ) {
    return register_name( as_register( reg ) );
  }
  static const char* shift_name ( const unsigned shift ) {
    return shift_name( as_shift( shift ) );
  }
  static const char* opcode_name( const unsigned opcode ) {
    return opcode_name( as_opcode( opcode ) );
  }

 private:
  Stream* _stream;
  static const char* _eol_comments;

  // instruction fields
  static short bit(const short instr, const int i) {
    return (instr >> i) & 1;
  }

  static Register reg_field( short instr, int shift = 0 ) {
    return as_register((instr >> shift) & 0x7);
  }

  static Register reg_field_w( int instr, int shift = 0 ) {
    return as_register((instr >> shift) & 0x0f);
  }

  // disassembler
  void emit_unknown(short instr, const char* comment = NULL);
  void emit_register_list(short instr);
  bool emit_GUARANTEE(bool check, const char* comment);

  void print_gp_name(int imm);
  const char *find_gp_name(int imm);
  int decode_imm(unsigned int p_value);
  void disasm_v6t2_data_processing(short instr, short hw2);
  void disasm_v6t2_data_processing_no_imm(short instr, short hw2);
  void disasm_v6t2_data_load_store_single(short instr, short hw2);
  void disasm_v6t2_data_load_store_double_and_exclusive(short instr, short hw2);
  void disasm_v6t2_data_load_store_multiple(short instr, short hw2);
  void disasm_v6t2_branches_and_misc(short instr, short hw2);
  void disasm_v6t2_coproc(short instr, short hw2);

  int disasm_internal(short* addr, short instr, int instr_offset = NO_OFFSET);
  void disasm_new16bit(short* addr, short instr, int instr_offset = NO_OFFSET);
  void disasm_32bit(short* addr, short instr, int instr_offset = NO_OFFSET);
  void start_thumb2(int num_bits, short* addr, jushort instr);
  void end_thumb2();

#if ENABLE_ARM_VFP
  void emit_vfp_register_list(int instr);
  void emit_vfp_instruction(int instr, int instr_offset);
  void unknown_vfp_instr(int instr);
#endif

 public:
  // creation
  Disassembler(Stream* stream) : _stream(stream) {}

  // accessors
  Stream* stream( void ) const { return _stream; }

  enum { NO_OFFSET = -1 };
  // Returns the number of half-words disassembled
  int disasm(short* addr, short instr, int instr_offset = NO_OFFSET);

#if ENABLE_ARM_VFP
  // type is either 's' (float) or 'd' (double)
  static void vfp_reg_name( const char type, unsigned reg, char buff[] );
#endif

  static void eol_comment( const char* s ) {
    _eol_comments = s;
  }
};

#endif // PRODUCT
