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

#include "incls/_precompiled.incl"
#include "incls/_Disassembler_arm.cpp.incl"

#if !ENABLE_THUMB_COMPILER

#if !defined(PRODUCT) || USE_COMPILER_DISASSEMBLER

void Disassembler::emit_unknown(int instr, const char* comment) {
  if (GenerateGNUCode) {
    stream()->print(".word\t0x%08x", instr);
  } else {
    stream()->print("DCD\t0x%08x", instr);
  }
  if (comment != NULL) {
    if (GenerateGNUCode && !GenerateROMImage) {
      stream()->print("\t/* %s */", comment);
    } else {
      stream()->print("\t; %s", comment);
    }
  }
}

void Disassembler::emit_imm_offset(bool u, int imm) {
  if (imm != 0) {
    stream()->print(", #");
    if (!u) {
      stream()->put('-');
    }
    stream()->print("%d", imm);
  }
}

void Disassembler::emit_shifted_reg(int instr) {
  // decode shifted register part of an shifter operand or an address
  const Assembler::Shift shift = Assembler::as_shift(instr >> 5 & 0x3);
  stream()->print(reg_name(rm_field(instr)));
  if (bit(instr, 4)) {
    // register shift
    GUARANTEE(!bit(instr, 7), "not a register shift");
    stream()->print(", %s %s", shift_name(shift), reg_name(rs_field(instr)));
  } else {
    // immediate shift
    const int shift_imm = instr >> 7 & 0x1f;
    if (shift_imm == 0) {
      switch (shift) {
        case Assembler::lsl:
          // Nothing to print
          break;
        case Assembler::ror:
          stream()->print(", rrx");
          break;
        default:
          // Treat as a shift by 32
          stream()->print(", %s #32", shift_name(shift));
          break;
      }
    } else {
      stream()->print(", %s #%d", shift_name(shift), shift_imm);
    }
  }
}

void Disassembler::emit_register_list(int instr) {
  stream()->put('{');
  int b = -1;
  bool comma = false;
  // clear upper bits so we can run past last register (see below)!
  instr &= 0xffff;
  for (int i = 0; i <= (Assembler::number_of_registers - Assembler::number_of_float_registers); i++) {
    // run past last register (by 1)!
    // b <  0 => no open range
    // b >= 0 => open range [b, i-1]
    if ((instr & 1 << i) != 0) {
      // register ri included
      if (b < 0) { b = i; } // open a new range
    } else {
      // register ri excluded
      if (b >= 0) {
        // print open range
        if (comma) {
          stream()->print(", ");
        }
        stream()->print(reg_name(Assembler::as_register(b)));
        if (b < i-1) {
            stream()->print("%s%s",
                            (b == i-2 ? ", " : " - "),
                            reg_name(Assembler::as_register(i-1)));
        }
        b = -1; // close range
        comma = true;
      }
    }
  }
  // note: any open range will be closed because we run past the
  // last register and the next bit in the instruction is cleared
  stream()->put('}');
}

void Disassembler::emit_address1(int instr, int instr_offset) {
  // decode shifter operand part of an instruction (addressing mode 1) and
  // print on stream
  (void)instr_offset;
  if (bit(instr, 25)) {
    // rotate immediate
    const int rotate_imm = instr >> 7 & 0x1e; // * 2
    const int imm_8      = instr & 0xff;
    const int imm_32     = _rotr(imm_8, rotate_imm);
    if (rotate_imm == 0 || ((imm_8 & 3) != 0)) {
      // This value is represented using the smallest possible shift
      // Small constants get printed in decimal, others in hex.  Only
      // because it's easier to read
      stream()->print(rotate_imm == 0 ? "#%d" : "#0x%x", imm_32);
    } else {
      // We purposely used a non-standard immediate representation??
      stream()->print("#0x%x, %d\t; #0x%x", imm_8, rotate_imm, imm_32);
    }
  } else {
    // immediate/register shift
    emit_shifted_reg(instr);
  }
}

const char *find_gp_name(bool u, int uoffset) {
  static char buff[100];
  if (!u || uoffset < 0) {
    return NULL;
  }
  int word_index = uoffset/4;

#if !ENABLE_THUMB_GP_TABLE
  if (word_index < 256) {
    Bytecodes::Code bc = (Bytecodes::Code) word_index;
    if (Bytecodes::is_defined(bc)) {
      return Bytecodes::name(bc);
    } else {
      jvm_sprintf(buff, "undefined bytecode 0x%x", word_index);
      return buff;
    }
  }
  uoffset -= 256 * 4;

  if (ENABLE_DISPATCH_TABLE_PADDING) {
    uoffset -= 8 * 4;
  }
#else
  uoffset -= 4; // skip the NOP bytecode handler
#endif

  static const GPTemplate gp_templates[] = {
    GP_SYMBOLS_DO(DEFINE_GP_POINTER, DEFINE_GP_VALUE)
    {NULL, 0, 0, 0}
  };

  for (const GPTemplate* tmpl = gp_templates; tmpl->name; tmpl++) {
    if (uoffset < tmpl->size) {
      if (tmpl->size == 4) {
        return tmpl->name;
      } else {
        jvm_sprintf(buff, "%s[%d]", tmpl->name, uoffset);
        return buff;
      }
    }
    uoffset -= tmpl->size;
  }

  return NULL;
}

void Disassembler::emit_address2(int instr, int instr_offset) {
  // decode address part of a standard load/store instruction
  // (addressing mode 2) and print on stream
  const bool p = bit(instr, 24);
  const bool u = bit(instr, 23);  // add or subtract
  const bool w = bit(instr, 21);
  int uoffset = instr & 0xFFF;
  bool is_imm = false;
  // p = 0, w = 0   => [reg], offset!  post indexed addressing
  // p = 0, w = 1   =>                 doesn't occur
  // p = 1, w = 0   => [reg, offset]   offset addressing
  // p = 1, w = 1   => [reg, offset]!  pre indexed addressing
  stream()->print("[%s", reg_name(rn_field(instr)));
  if (!p) {
    stream()->put(']');
  }
  if (bit(instr, 25)) {
    // register offset
    stream()->print(", %c", u ? '+' : '-');
    emit_shifted_reg(instr);
  } else {
    // immediate offset
    emit_imm_offset(u, uoffset);
    is_imm = true;
  }
  if (p) {
    stream()->put(']');
    if (w) {
      stream()->put('!');
    }
  }
  if (instr_offset != NO_OFFSET && rn_field(instr) == 15) {
    int target = instr_offset + 8 + (u ? uoffset : -uoffset);
    if (GenerateGNUCode && !GenerateROMImage) {
      stream()->print("/* = %d */", target);
    } else {
      stream()->print("; = %d ", target);
    }
  }

  if (rn_field(instr) == Assembler::gp && is_imm) {
    const char * name = find_gp_name(u, uoffset);
    if (name != NULL) {
      while (stream()->position() <= 40) {
        stream()->print(" ");
      }
      if (GenerateGNUCode && !GenerateROMImage) {
        stream()->print("@ %s ", name);
      } else {
        stream()->print("; %s ", name);
      }
    }
  }
}

void Disassembler::emit_address3(int instr, int instr_offset) {
  // decode address part of a miscellaneous load/store instruction
  // (addressing mode 3) and print on stream
  const bool p = bit(instr, 24);
  const bool u = bit(instr, 23);
  const bool w = bit(instr, 21);
  const int uoffset = instr >> 4 & 0xf0 | instr & 0xf;
  stream()->print("[%s", reg_name(rn_field(instr)));
  if (!p) {
    stream()->put(']');
  }
  if (bit(instr, 22)) {
    // immediate offset
    emit_imm_offset(u, uoffset);
  } else {
    // register offset
    stream()->print(", %c", u ? '+' : '-');
    stream()->print(reg_name(rm_field(instr)));
  }
  if (p) {
    stream()->put(']');
    if (w) {
      stream()->put('!');
    }
  }
  if (instr_offset != NO_OFFSET && rn_field(instr) == 15) {
    int target = instr_offset + 8 + (u ? uoffset : -uoffset);
    if (GenerateGNUCode && !GenerateROMImage) {
      stream()->print("/* = %d */", target);
    } else {
      stream()->print("; = %d ", target);
    }
  }
}

void Disassembler::emit_address5(int instr, int instr_offset) {
  const bool p = bit(instr, 24);
  const bool u = bit(instr, 23);
  const bool w = bit(instr, 21);
  int uoffset = (instr & 0xFF) << 2;
  stream()->print("[%s", reg_name(rn_field(instr)));
  if (!p) {
      stream()->put(']');
  }
  if (p || w) {
    emit_imm_offset(u, uoffset);
  }
  if (p) {
    stream()->put(']');
    if (w) {
      stream()->put('!');
    }
  } else {
    if (!w) {
      // coprocessor unindexed mode, whatever that is
      stream()->print(", {%d}", instr & 0xFF);
    }
  }
  if (instr_offset != NO_OFFSET && rn_field(instr) == 15 && (p || w)) {
    int target = instr_offset + 8 + (u ? uoffset : -uoffset);
    if (GenerateGNUCode && !GenerateROMImage) {
      stream()->print("/* = %d */", target);
    } else {
      stream()->print("; = %d ", target);
    }
  }
}

#if ENABLE_ARM_VFP
void Disassembler::emit_vfp_register_list(int instr) {
  int cpnum = (int)rs_field(instr);
  int Fd   = (int)rd_field(instr);
  int D    = bit(instr, 22);
  int freg = Fd << 1 | D;
  int size = instr & 0xFF + freg;

  char type = (cpnum == 10 ? 's': 'd');

  stream()->put('{');
  char buffer [4];
  vfp_reg_name(type, freg++, buffer);
  stream()->print(buffer);

  while (freg < size) {
    stream()->put(',');
    vfp_reg_name(type, freg++, buffer);
    stream()->print(buffer);
  }
  stream()->put('}');
}
#endif

const char* Disassembler::cond_name(Assembler::Condition cond) {
  static const char* cond_names[Assembler::number_of_conditions] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "",   "nv"
  };
  GUARANTEE(int(Assembler::eq) <= int(cond) &&
            int(cond) < int(Assembler::number_of_conditions),
            "illegal condition");
  return cond_names[cond];
}

const char* Disassembler::reg_name(Assembler::Register reg) {
  static const char* const reg_names[] = {
    "r0", "r1", "r2", "r3", "r4" , "r5", "r6", "r7",
    "r8", "r9", "r10", "fp", "r12", "sp", "lr", "pc"
  };
  GUARANTEE( unsigned(reg) < unsigned(sizeof reg_names/sizeof *reg_names),
             "Invalid register" );
  return reg_names[reg];
}

#if ENABLE_ARM_VFP
void Disassembler::vfp_reg_name(const char type, unsigned reg, char buffer[]) {
  GUARANTEE(reg < 32, "Invalid vfp register");
  if( type == 'd' ) {
//    GUARANTEE((reg & 1) == 0, "Invalid vfp double register");
    reg >>= 1;
  }

  jvm_sprintf(buffer, "%c%u", type, reg);
}
#endif


const char* Disassembler::shift_name(Assembler::Shift shift) {
  static const char* shift_names[Assembler::number_of_shifts] = {
    "lsl", "lsr", "asr", "ror"
  };
  GUARANTEE(int(Assembler::lsl) <= int(shift) &&
            int(shift) < int(Assembler::number_of_shifts),
            "illegal shift");
  return shift_names[shift];
}

const char* Disassembler::opcode_name(Assembler::Opcode opcode) {
  static const char* opcode_names[Assembler::number_of_opcodes] = {
    "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
    "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"
  };
  return opcode_names[opcode];
}

void Disassembler::disasm(int* addr, int instr, int instr_offset) {
  // decode instruction and print on stream
  const Assembler::Condition cond = Assembler::as_condition(instr >> 28 & 0xf);
  int op = (instr >> 25) & 0x7;
  if (cond == Assembler::nv) {
    if (op == 6 || (op == 7 && !bit(instr, 24))) {
      // fall through, these are coprocessor instructions
    } else if ((instr & 0xFD70F000) == 0xF550F000) {
      stream()->print("pld\t");
      emit_address2(instr, instr_offset);
      return;
    } else {
      emit_unknown(instr);
      return;
    }
  }
  switch (op) {
    case  0:
    case  1:
      if (   ((instr & 0x0FB0F000) == 0x0320F000)
          || ((instr & 0x0FB0FFF0) == 0x0120F000)) {
        stream()->print("msr%s\t%s_%s%s%s%s, ",
                        cond_name(cond),
                        (bit(instr, 22) ? "spsr" : "cpsr"),
                        bit(instr, 19) ? "c" : "",
                        bit(instr, 18) ? "x" : "",
                        bit(instr, 17) ? "s" : "",
                        bit(instr, 16) ? "f" : "");
        emit_address1(instr, instr_offset);
        break;
      }

      if (   ((instr & 0x0f900010) == 0x01000000)
          || ((instr & 0x0f900090) == 0x01000010)) {
        // This is Table 3-3 in the ARM instruction set manual.  For now,
        // we only care about a small number of instructions
        if ((instr & 0x0FFF0FF0) == 0x016f0f10) {
          stream()->print("clz%s\t%s, %s",
                          cond_name(cond),
                          reg_name(rd_field(instr)),
                          reg_name(rm_field(instr)));
        } else if ((instr & 0x0FFFFFC0) == 0x012fff00) {
          static const char *names[4] = { NULL, "bx", NULL, "blx" };
          const char *name = names[(instr >> 4) & 3];
          if (name == NULL) {
            emit_unknown(instr);
          } else {
            stream()->print("%s%s\t%s",
                            name, cond_name(cond), reg_name(rm_field(instr)));
          }
        } else if ((instr & 0x0FBF0FFF) == 0x010F0000) {
          stream()->print("mrs%s\t%s, %s",
                          cond_name(cond),
                          reg_name(rd_field(instr)),
                          (bit(instr, 22) ? "spsr" : "cpsr"));
        } else {
          emit_unknown(instr);
        }
        break;
      }

      if (!bit(instr, 25) && bit(instr, 7) && bit(instr, 4)) {
        // Table 3.2.  Multiplies and extra load/store instructions
        const int sh = instr >> 5 & 0x3;
        if (sh == 0) {
          // multiply or swap
          if (bit(instr, 24)) {
            // swap
            stream()->print("swp");
            stream()->print(cond_name(cond));
            if (bit(instr, 22)) {
              stream()->put('b');
            }
            stream()->print("\t%s, %s, [%s]",
                            reg_name(rd_field(instr)),
                            reg_name(rm_field(instr)),
                            reg_name(rn_field(instr)));
          } else {
            // multiply
            const bool l = bit(instr, 23);  // long
            const bool u = bit(instr, 22);  // signed or unsigned
            const bool a = bit(instr, 21);  // accumulate or normal
            const bool s = bit(instr, 20);  // set cc bits
            // emit opcode
            if (l) {
              stream()->put(u ? 's' : 'u');
            }
            stream()->print(a ? "mla" : "mul");
            if (l) {
              stream()->put('l');
            }
            stream()->print(cond_name(cond));
            if (s) {
              stream()->put('s');
            }
            // emit operands
            const char* rd = reg_name(rn_field(instr)); // not rd_field!
            const char* rn = reg_name(rd_field(instr)); // not rn_field!
            const char* rs = reg_name(rs_field(instr));
            const char* rm = reg_name(rm_field(instr));
            if (l) {
              stream()->print("\t%s, %s, %s, %s", rn, rd, rm, rs);
            } else {
              stream()->print("\t%s, %s, %s", rd, rm, rs);
              if (a) {
                stream()->print(", %s", rn);
              }
            }
          }
        } else {
          // The specific instruction depends on bit 20 and sh
          const int xsh = sh + (bit(instr, 20) ? 4 : 0);
          stream()->print((xsh == 1 || xsh == 3) ? "str" : "ldr");
          stream()->print(cond_name(cond));
          switch (xsh) {
            case 1: case 5: stream()->print("h" ); break;
            case 2: case 3: stream()->print("d" );  break;
            case 6:         stream()->print("sb"); break;
            case 7:         stream()->print("sh"); break;
            default: SHOULD_NOT_REACH_HERE();
          }
          stream()->print("\t%s, ", reg_name(rd_field(instr)));
          emit_address3(instr, instr_offset);
        }
        break;
      }
      // data processing instructions
      if (instr == 0xe1800000) {
        stream()->print("nop");
        break;
      }
      { const Assembler::Opcode opcode =
            Assembler::as_opcode(instr >> 21 & 0xf);
        const char*  s  = bit(instr, 20) ? "s" : "";
        const char*  rn = reg_name(rn_field(instr));
        const char*  rd = reg_name(rd_field(instr));
        stream()->print("%s%s", opcode_name(opcode), cond_name(cond));
        switch (opcode) {
          case Assembler::_andr: // fall through
          case Assembler::_eor: // fall through
          case Assembler::_sub: // fall through
          case Assembler::_rsb: // fall through
          case Assembler::_add: // fall through
          case Assembler::_adc: // fall through
          case Assembler::_sbc: // fall through
          case Assembler::_rsc: // fall through
          case Assembler::_orr: // fall through
          case Assembler::_bic:
            // <opcode>{<cond>}{s} <rd>, <rn>, <shifter_op>
            stream()->print("%s\t%s, %s, ", s, rd, rn);
            break;
          case Assembler::_tst: // fall through
          case Assembler::_teq: // fall through
          case Assembler::_cmp: // fall through
          case Assembler::_cmn: // fall through
            // <opcode>{<cond>} <rn>, <shifter_op>
            stream()->print("\t%s, ", rn);
            break;
          case Assembler::_mov: // fall through
          case Assembler::_mvn: // fall through
            // <opcode>{<cond>}{s} <rd>, <shifter_op>
            stream()->print("%s\t%s, ", s, rd);
            break;
          default : SHOULD_NOT_REACH_HERE();
        }
        emit_address1(instr, instr_offset);
      }
      break;
    case  3:
      if (bit(instr, 4)) {
        if (cond == Assembler::al) {
          emit_unknown(instr, "breakpoint");
        } else {
          emit_unknown(instr);
        }
        break;
      }
      // fall through!
    case  2:
      {
        // load/store word and unsigned byte instructions
        stream()->print(bit(instr, 20) ? "ldr" : "str");
        stream()->print(cond_name(cond));
        if (bit(instr, 22)) {
          stream()->print("b");
        }
        stream()->print("\t%s, ", reg_name(rd_field(instr)));
        emit_address2(instr, instr_offset);
        break;
      }
    case  4:
      {
        // load/store multiple
        const Assembler::Register rn = rn_field(instr);
        const bool p = bit(instr, 24);
        const bool u = bit(instr, 23);
        const bool l = bit(instr, 20);
        stream()->print(l ? "ldm" : "stm");
        stream()->print(cond_name(cond));
        if (rn == Assembler::sp || rn == Assembler::jsp) {
          // use stack addressing mnemonics
          stream()->put(l == p ? 'e' : 'f');
          stream()->put(l == u ? 'd' : 'a');
        } else {
          // use non-stack addressing mnemonics
          stream()->put(u ? 'i' : 'd');
          stream()->put(p ? 'b' : 'a');
        }
        stream()->print("\t%s", reg_name(rn));
        if (bit(instr, 21)) {
          stream()->put('!');
        }
        stream()->print(", ");
        emit_register_list(instr);
      }
      break;
    case  5:
      // branches
      { const int offset =
            // sign-extend!
            ((instr & 0xffffff) | (bit(instr, 23) ? 0xff000000 : 0)) << 2;
        stream()->put('b');
        if (bit(instr, 24)) {
          stream()->put('l');
        }
        stream()->print("%s\t", cond_name(cond));
        stream()->print("pc + %d\t", offset + 8);
        if (addr != NULL || instr_offset != NO_OFFSET) {
          stream()->print(GenerateGNUCode & !GenerateROMImage ? "/* " : "; ");
          if (addr != NULL && VerbosePointers) {
            stream()->print("=0x%08x ", (int)addr + 8 + offset);
          }
          if (offset != NO_OFFSET) {
            stream()->print("=%d ", instr_offset + 8 + offset);
          }
          if (GenerateGNUCode & !GenerateROMImage) {
            stream()->print("*/");
          }
        }
        break;
      }
    case  6: {
      // Memory to coprocessor register
      int coprocessor = (instr >> 8) & 0xF;
#if ENABLE_XSCALE_WMMX_INSTRUCTIONS
      // To support Xscale WMMX instructions diassembler
      if ( coprocessor == 0 || coprocessor == 1 ) {
        // WMMX instructions using coprocessor 0 & 1
        emit_wmmx_instruction(instr);
        return;
      }
#endif

#if ENABLE_ARM_VFP
      if (coprocessor == 10 || coprocessor == 11) {
        emit_vfp_instruction(instr, instr_offset);
        return;
      }
#endif

      stream()->print("%s%s%s\tp%d, c%d, ",
                      (bit(instr, 20) ? "ldc" : "stc"),
                      cond == Assembler::nv ? "2" : cond_name(cond),
                      (bit(instr, 22) ? "l"   : ""),  // two word?
                      (instr >> 8) & 0xF,             // coprocessor
                      rd_field(instr));
      emit_address5(instr, instr_offset);
      break;
    }

    case  7:
      if (!bit(instr, 24)) {
        // coprocessor instructions.
        int coprocessor = (instr >> 8) & 0xF;
        int opcode2     = (instr >> 5) & 0x7;
#if ENABLE_XSCALE_WMMX_INSTRUCTIONS
        // To support Xscale WMMX instructions diassembler
        if ( coprocessor == 0 || coprocessor == 1 ) {
          // WMMX instructions using coprocessor 0 & 1
          emit_wmmx_instruction(instr);
          return;
        }
#endif

#if ENABLE_ARM_VFP
        if (coprocessor == 10 || coprocessor == 11) {
          emit_vfp_instruction(instr, instr_offset);
          return;
        }
#endif

        if (!bit(instr, 4)) {
          stream()->print("cdp%s\tp%d, %d, c%d, c%d, c%d",
                          cond == Assembler::nv ? "2" : cond_name(cond),
                          coprocessor,
                          (instr >> 20) & 0xF, // opcode1
                          rd_field(instr), rn_field(instr), rm_field(instr));
        } else {
          // mcr, mrc
          stream()->print("%s%s\tp%d, %d, %s, c%d, c%d",
                          (bit(instr, 20) ? "mrc" : "mcr"),
                          cond == Assembler::nv ? "2" : cond_name(cond),
                          coprocessor,
                          (instr >> 21) & 0x7, // opcode1
                          reg_name(rd_field(instr)),
                          rn_field(instr), rm_field(instr));
        }
        stream()->print(", %d", opcode2);
      } else {
        // swi
        stream()->print("swi%s\t0x%06x", cond_name(cond), instr & 0xffffff);
      }
      break;
    default: SHOULD_NOT_REACH_HERE();
  }
}

#if ENABLE_ARM_VFP
void Disassembler::emit_vfp_instruction(int instr, int instr_offset) {
  const Assembler::Condition cond = Assembler::as_condition(instr >> 28 & 0xf);
  int cpnum = (int)rs_field(instr);
  int Fn = (int)rn_field(instr);
  int Fd = (int)rd_field(instr);
  int Fm = (int)rm_field(instr);
  int D  = bit(instr, 22);
  int N  = bit(instr, 7);
  int M  = bit(instr, 5);
  const char type = (cpnum == 10 ? 's': 'd');

  char fn[4];
  char fd[4];
  char fm[4];

  vfp_reg_name(type, (Fd << 1 | D), fd);
  vfp_reg_name(type, (Fn << 1 | N), fn);
  vfp_reg_name(type, (Fm << 1 | M), fm);

  if (((instr >> 24) & 0x0f) == 0x0e && bit(instr, 4) == 0) {
    // Data processing
    int p  = bit(instr, 23);
    int q  = bit(instr, 21);
    int r  = bit(instr, 20);
    int s  = bit(instr, 6);

    const char * op = "UNDEF";
    int pqrs = p << 3 | q << 2 | r << 1 | s;
    switch (pqrs) {
    case 0x00: op = "fmac";  break;
    case 0x01: op = "fnmac"; break;
    case 0x02: op = "fmsc";  break;
    case 0x03: op = "fnmsc"; break;
    case 0x04: op = "fmul";  break;
    case 0x05: op = "fnmul"; break;
    case 0x06: op = "fadd";  break;
    case 0x07: op = "fsub";  break;
    case 0x08: op = "fdiv";  break;
    }

    if (pqrs != 0x0f) {
      stream()->print("%s%c%s\t%s, %s, %s", op, type, cond_name(cond),
                      fd, fn, fm);
    } else {
      static const char * const ops[] = {
        "fcpy",
        "fabs",
        "fneg",
        "fsqrt",
        "??",
        "??",
        "??",
        "??",
        "fcmp",
        "fcmpe",
        "fcmpz",
        "fcmpez",
        "??",
        "??",
        "??",
        "fcvt",  // Special case different instruction for float and double
        "fuito",
        "fsito",
        "??",
        "??",
        "??",
        "??",
        "??",
        "??",
        "ftoui",
        "ftouiz",
        "ftosi",
        "ftosiz"
      };
      const char *op = "??";
      int n = Fn << 1 | N;
      if (n < 28) {
        op = ops[n];
      }
      // Handle special case different instruction for float and double
      if (n == 15) {
  if (type == 'd') {
    vfp_reg_name('s', (Fd << 1 | D), fd);
    vfp_reg_name('d', (Fm << 1 | M), fm);
    stream()->print("fcvtsd%s\t%s, %s", cond_name(cond), fd, fm);
  } else {
    vfp_reg_name('d', (Fd << 1 | D), fd);
    vfp_reg_name('s', (Fm << 1 | M), fm);
    stream()->print("fcvtds%s\t%s, %s", cond_name(cond), fd, fm);
  }
      } else {
        stream()->print("%s%c%s\t%s, %s", op, type, cond_name(cond),
                        fd, fm);
      }
    }
  } else if (((instr >> 24) & 0x0f) == 0x0e && ((instr >> 4) & 0x07) == 0x1) {
    // Single register transfer instructions
    int opcode  = (instr >> 21) & 0x07;
    int L       = bit(instr, 20);
    Register rd = rd_field(instr);
    int Rd      = (int)rd_field(instr);
    const char *vfp_system_reg = "fpscr";
    const char *vfp_reg = "??";

    if ((Fn & 0x1) == 0x1) {
      vfp_reg = vfp_system_reg;
    }

    if (opcode == 0 && cpnum == 10 && L == 0) {
      stream()->print("fmsr%s\t%s, %s", cond_name(cond), fn, reg_name(rd));
    }
    else if (opcode == 0 && cpnum == 10 && L == 1) {
      stream()->print("fmrs%s\t%s, %s", cond_name(cond), reg_name(rd), fn);
    }
    else if (opcode == 7 && cpnum == 10 && L == 1 && N == 0 && Fn == 0x1 && Fd == 0xF) {
      stream()->print("fmstat%s", cond_name(cond));
    }
    else if (opcode == 7 && cpnum == 10 && L == 0 && N == 0) {
      stream()->print("fmxr%s\t%s, %s", cond_name(cond), vfp_reg, reg_name(rd));
    }
    else if (opcode == 7 && cpnum == 10 && L == 1 && N == 0) {
      stream()->print("fmrx%s\t%s, %s", cond_name(cond), reg_name(rd), vfp_reg);
    } else {
      unknown_vfp_instr(instr);
    }
  } else if (((instr >> 21) & 0x62) == 0x62 &&
             ((instr >> 8) & 0x0b) == 0x0b &&
             ((instr >> 4) & 0x07) == 0x1) {
    int L        = bit(instr, 20);
    Register rd  = rd_field(instr);
    Register rn  = rn_field(instr);

    if (cpnum == 11 && L == 0) {
      stream()->print("fmdrr%s\t%s, %s, %s", cond_name(cond), fm, reg_name(rd), reg_name(rn));
    } else if (cpnum == 11 && L == 1) {
      stream()->print("fmrrd%s\t%s, %s, %s", cond_name(cond), reg_name(rd), reg_name(rn), fm);
    } else {
      unknown_vfp_instr(instr);
    }
  } else if (((instr >> 25) & 0x07) == 0x6) {
    // Load and store
    Register rn  = rn_field(instr);
    int P    = bit(instr, 24);
    int U    = bit(instr, 23);
    int W    = bit(instr, 21);
    int L    = bit(instr, 20);
    int puw = P << 2 | U << 1 | W;

    switch (puw) {
      case 2:
      case 3:
      case 5: {
        stream()->print("f%sm", (L ? "ld" : "st"));
        if (rn == Assembler::sp || rn == Assembler::jsp) {
          // use stack addressing mnemonics
          stream()->put(L == P ? 'e' : 'f');
          stream()->put(L == U ? 'd' : 'a');
        } else {
          // use non-stack addressing mnemonics
          stream()->put(U ? 'i' : 'd');
          stream()->put(P ? 'b' : 'a');
        }
        stream()->print("%c", type);
        stream()->print("%s", cond_name(cond));
        stream()->print("\t%s", reg_name(rn));
        if (W) {
          stream()->put('!');
        }
        stream()->print(", ");
        emit_vfp_register_list(instr);
        break;
      }
      case 4:
      case 6: {
        stream()->print("f%s%c%s\t%s, ", (L ? "ld" : "st"), type,
                      cond_name(cond), fd);
        emit_address5(instr, instr_offset);
        break;
      }
      default:
        unknown_vfp_instr(instr);
    }
  } else {
    unknown_vfp_instr(instr);
  }
}

void Disassembler::unknown_vfp_instr(int instr) {
  stream()->print("DCD\t0x%08x", instr);
  GUARANTEE_R(false, "\ngenerated bad VFP instruction");
}
#endif

#endif // !defined(PRODUCT) || USE_COMPILER_DISASSEMBLER

#endif /*#if !ENABLE_THUMB_COMPILER */
