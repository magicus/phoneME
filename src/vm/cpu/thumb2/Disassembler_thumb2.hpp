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
#if ENABLE_COMPILER
 private:
  static const char* _unresolved_label_name;
 public:
  static void set_reference_to_unresolved_label( const char* name = "???" ) {
    if( PrintCompiledCodeAsYouGo ) {
      _unresolved_label_name = name;
    }
  }
#endif

#include "../arm/Disassembler_armthumb.hpp"
 private:
  // instruction fields
  static jushort bit(const jushort instr, const int i) {
    return (instr >> i) & 1;
  }

  static Register reg_field( const jushort instr, const int shift = 0 ) {
    return as_register((instr >> shift) & 0x7);
  }

  static Register reg_field_w( const juint instr, const int shift = 0 ) {
    return as_register((instr >> shift) & 0x0f);
  }

  // disassembler
  void emit_unknown(const jushort instr);
  void emit_unknown(const jushort hw1, const jushort hw2 );
  void emit_register_list(short instr);
  bool emit_GUARANTEE(bool check, const char* comment);
  void emit_target_offset(const int offset, const int instr_offset);

#if ENABLE_ARM_VFP
  static char precision         (const jushort hw1, const jushort hw2);

  static jushort rm_field       (const jushort hw1, const jushort hw2);
  static jushort rn_field       (const jushort hw1, const jushort hw2);
  static jushort rd_field       (const jushort hw1, const jushort hw2);
  static jushort m_bit          (const jushort hw1, const jushort hw2);
  static jushort n_bit          (const jushort hw1, const jushort hw2);
  static jushort d_bit          (const jushort hw1, const jushort hw2);
  static jushort l_bit          (const jushort hw1, const jushort hw2);
  static jushort fm_field       (const jushort hw1, const jushort hw2);
  static jushort fn_field       (const jushort hw1, const jushort hw2);
  static jushort fd_field       (const jushort hw1, const jushort hw2);
#endif
  void disasm_v6t2_coproc       (const jushort hw1, const jushort hw2,
                                 const int instr_offset = NO_OFFSET);

  void print_gp_name(int imm);
  const char *find_gp_name(int imm);
  int decode_imm(unsigned int p_value);
  void disasm_v6t2_data_processing(short instr, short hw2);
  void disasm_v6t2_data_processing_no_imm(short instr, short hw2);
  void disasm_v6t2_data_load_store_single(short instr, short hw2);
  void disasm_v6t2_data_load_store_double_and_exclusive(short instr, short hw2);
  void disasm_v6t2_data_load_store_multiple(short instr, short hw2);
  void disasm_v6t2_branches_and_misc(short instr, short hw2);

  int  disasm_internal  (const short* addr, short instr,
                                        const int instr_offset = NO_OFFSET);
  void disasm_new16bit  (const short* addr, short instr,
                                        const int instr_offset = NO_OFFSET);
  void disasm_32bit     (const short* addr, const short instr,
                                        const int instr_offset = NO_OFFSET);
  void start_thumb2(const int num_bits, const short* addr, const jushort instr);
  void end_thumb2();
  void comments( const short* addr, int num_half_words );
};

#endif // PRODUCT
