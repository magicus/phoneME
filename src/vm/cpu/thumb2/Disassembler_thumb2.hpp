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
 private:
  Stream* _stream;
  static char *_eol_comments;

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
  Assembler::Register reg(int n) {
    GUARANTEE(n >= 0 && n < Assembler::number_of_registers, "sanity");
    return (Assembler::Register)n;
  }
  void start_thumb2(int num_bits, short* addr, jushort instr);
  void end_thumb2();
 public:
  // creation
  Disassembler(Stream* stream) : _stream(stream) {}
  
  // accessors
  Stream* stream() const { return _stream; }
  
  enum { NO_OFFSET = -1 };
  // Returns the number of half-words disassembled
  int disasm(short* addr, short instr, int instr_offset = NO_OFFSET);

  // textual representation
  static const char* cond_name  (Assembler::Condition cond  );
  static const char* reg_name   (Assembler::Register  reg   );
  static const char* shift_name (Assembler::Shift     shift );
  static const char* opcode_name(Assembler::Opcode    opcode);

  static void eol_comment(char *s) {
    _eol_comments = s;
  }
};

#endif // PRODUCT
