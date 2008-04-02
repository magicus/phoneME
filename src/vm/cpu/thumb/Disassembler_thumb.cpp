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

#include "incls/_precompiled.incl"

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
#include "incls/_Disassembler_thumb.cpp.incl"

const char *Disassembler::_eol_comments = NULL;
Assembler::Register Disassembler::_last_imm_register = Assembler::no_reg;
int Disassembler::_last_imm_offset;
int Disassembler::_last_imm;

void Disassembler::emit_unknown(short instr, const char* comment) {
  if (GenerateGNUCode) { 
    stream()->print(".word\t0x%04x", (unsigned short)instr);
    if (comment != NULL) { 
      stream()->print("\t/* %s */", comment);
    }
  } else {
    stream()->print("DCD\t0x%04x", (unsigned short)instr);
    if (comment != NULL) { 
      stream()->print("\t; %s", comment);
    }
  }
}

void Disassembler::emit_register_list(short instr) {
  stream()->put('{');
  int b = -1;
  bool comma = false;
  // clear upper bits so we can run past last register (see below)!
  instr &= 0xff; 
  for (int i = 0; i <= Assembler::number_of_registers; i++) { 
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

const char* Disassembler::cond_name(Assembler::Condition cond) {
  static const char* cond_names[Assembler::number_of_conditions] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "",   "nv"
  };
  GUARANTEE(Assembler::eq <= cond && cond < Assembler::number_of_conditions,
            "illegal condition");
  return cond_names[cond];
}

const char* Disassembler::reg_name(Assembler::Register reg) {
  static const char* reg_names[Assembler::number_of_registers] = {
    "r0", "r1", "r2", "r3", "fp" , "gp", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc"
  };
  GUARANTEE(Assembler::r0 <= reg && reg < Assembler::number_of_registers,
            "illegal register");
  return reg_names[reg];
}

const char* Disassembler::shift_name(Assembler::Shift shift) {
  static const char* shift_names[Assembler::number_of_shifts] = {
    "lsl", "lsr", "asr", "ror"
  };
  GUARANTEE(Assembler::lsl_shift <= shift && shift < Assembler::number_of_shifts,
            "illegal shift");
  return shift_names[shift];
}

const char* Disassembler::opcode_name(Assembler::Opcode opcode) {
  static const char* opcode_names[Assembler::number_of_opcodes] = {
    "and", "eor", "lsl", "lsr", "asr", "adc", "sbc", "ror",
    "tst", "neg", "cmp", "cmn", "orr", "mul", "bic", "mvn",
    "add", "sub", "mov"
  };
  return opcode_names[opcode];
}

const char *Disassembler::find_gp_name(int uoffset) {
  static char buff[100];
  if (uoffset < 0) {
    return NULL;
  }

  if (uoffset < 4) {
    return "bc_impl_nop";
  }
  uoffset -= 4;

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

void Disassembler::print_gp_name(int imm) {
   const char *name = find_gp_name(imm);
   if (name != NULL) {
     if (GenerateGNUCode && !GenerateROMImage) { 
       stream()->print("/* = %s */", name);
     } else { 
       stream()->print("; = %s ", name);
     }
   }
}

int Disassembler::disasm(short* addr, short instr, int instr_offset) {
  // decode instruction and print on stream
  int op = (instr >> 13) & 0x7;
  int num_half_words = 1;

  switch (op) {
    case 0:    
    {
      bool is_add_sub = (bit(instr, 12) && bit(instr, 11));
      const char*  rn = reg_name(reg_field(instr, 3));
      const char*  rd = reg_name(reg_field(instr));

      if (!is_add_sub) {
        // Shift by immediate
        // lsl | lsr | asr <Rd>, <Rm>, #<immed_5>
        const Assembler::Opcode opcode =
            Assembler::as_opcode((instr >> 11 & 0x3) + 2);
        int imm = ((instr >> 6) & 0x1F);
        stream()->print("%s", opcode_name(opcode));
        stream()->print("\t%s, %s, #%d", rd, rn, imm);
      } else {
        // add/sub register/immediate
        bool is_sub = bit(instr, 9);
        const char *name = is_sub ? "sub" : "add";
        if (!bit(instr, 10)) {
          // add|sub <Rd>, <Rm>, <Rn>
          const char* rm = reg_name(reg_field(instr, 6));
          stream()->print("%s\t%s, %s, %s", name, rd, rn, rm);
        } else {
          // add|sub <Rd>, <Rn>, #<3-bit imme.>
          int imm = ((instr >> 6) & 0x7);
          if (!is_sub && imm == 0) {
            stream()->print("mov\t%s, %s", rd, rn);
          } else {
            stream()->print("%s\t%s, %s, #%d", name, rd, rn, imm);
          }
        }
      }
    }
    break;

    case 1:
    {
      // add|sub|cmp|mov <Rd/Rn>, #<8-bit imme>
      const int opcode = (instr >> 11) & 0x3;
      const char* rd = reg_name(reg_field(instr, 8));
      switch (opcode) {
        case 0: 
          stream()->print("mov\t%s, #%d", rd, (instr & 0xFF));
          _last_imm = (instr & 0xFF);
          _last_imm_offset = instr_offset;
          _last_imm_register = reg_field(instr, 8);
          break;
        case 1: 
          stream()->print("cmp\t%s, #%d", rd, (instr & 0xFF));
          break;
        case 2: 
          stream()->print("add\t%s, #%d", rd, (instr & 0xFF));
          break;
        case 3: 
          stream()->print("sub\t%s, #%d", rd, (instr & 0xFF));
          break;
        default: 
          SHOULD_NOT_REACH_HERE();
      }
    }
    break;
      
    case 2:
    {
      if (((instr >> 11) & 0x1F) == 0x9) {
        // ldr Rd, [PC, #<immed_8> * 4]
        const char* rd = reg_name(reg_field(instr, 8));
        stream()->print("ldr\t%s, [pc, #%d]", rd, 
                        (instr & 0xFF) * 4);
        int target = (instr_offset & ~3) + 4 + ((instr & 0xFF) * 4);
        if (GenerateGNUCode) { 
          stream()->print("/" "* = %d ", target);
        } else { 
          stream()->print("; = %d ", target);
        }
        if (ObjectHeap::contains_moveable((OopDesc*)addr)) {
          int *value_addr = DERIVED(int*, addr, target - instr_offset);
          stream()->print("[0x%08x] = 0x%08x", value_addr, *value_addr);
          // Also print it as an object if applicable.
        }
        if (GenerateGNUCode) {
          stream()->print(" */");
        }
      } else if (bit(instr, 12)) {
        // ldr|str <Rd>, [<Rn>, <Rm>]
        const char* rd = reg_name(reg_field(instr, 0));
        const char* rn = reg_name(reg_field(instr, 3));
        const char* rm = reg_name(reg_field(instr, 6));
        const int opcode = (instr >> 9) & 0x7;
        switch(opcode) {
          case 0: stream()->print("str"); break;
          case 1: stream()->print("strh"); break;
          case 2: stream()->print("strb"); break;
          case 3: stream()->print("ldrsb"); break;
          case 4: stream()->print("ldr"); break;
          case 5: stream()->print("ldrh"); break;
          case 6: stream()->print("ldrb"); break;
          case 7: stream()->print("ldrsh"); break;
        }
        stream()->print("\t%s, [%s, %s]", rd, rn, rm);
        if (rn == reg_name(Assembler::gp)) {
          if (_last_imm_register == reg_field(instr, 6) && 
              _last_imm_offset == instr_offset - 2) {
            print_gp_name(_last_imm);
          }
        }
      } else if (bit(instr, 10)) {
        int opcode = (instr >> 8) & 0x3;
        Assembler::Register reg_rd = 
               Assembler::as_register((instr       & 0x7) |
                                      (instr >> 4) & 0x8);
        Assembler::Register reg_rm =
               Assembler::as_register((instr >> 3) & 0xF);

        const char* rd = reg_name(reg_rd);
        const char* rm = reg_name(reg_rm);      
        
        switch (opcode) {
          case 0: 
            stream()->print("add\t%s, %s", rd, rm);
            break;
          case 1: 
            stream()->print("cmp\t%s, %s", rd, rm);
            break;
          case 2: 
            stream()->print("mov\t%s, %s", rd, rm);
            break;
          case 3:
            stream()->print("bx\t%s", rm);
            break;
          default: 
            SHOULD_NOT_REACH_HERE();
        }
      } else {
          const Assembler::Opcode opcode =
            Assembler::as_opcode(instr >> 6 & 0xf);
          const char* rd = reg_name(reg_field(instr, 0));
          const char*  rn = reg_name(reg_field(instr, 3));
          stream()->print("%s\t%s, %s", opcode_name(opcode), rd, rn);
        }
        break;
    }
    
    case 3:    
    {
       // ldrb|strb|ldr|str <Rd>, [<Rn>, #<immed_5> * 4]
       const char*  rn = reg_name(reg_field(instr, 3));
       const char*  rd = reg_name(reg_field(instr));
       const bool byte_op = bit(instr,12);
       const int imm_shift = byte_op ? 0 : 2;
       if (bit(instr, 11)) {
         stream()->print(byte_op ? "ldrb" : "ldr");
       } else {
         stream()->print(byte_op ? "strb" : "str");
       }
       stream()->print("\t%s, [%s", rd, rn);
       int imm = ((instr >> 6) & 0x1F) << imm_shift;
       if (imm == 0) {
         stream()->print("]");
       }
       else {
         stream()->print(", #%d]", imm);
         if (rn == reg_name(Assembler::gp)) {
           print_gp_name(imm);
         }
       }
       break;
    }

    case 4:
    {
      const char*  rn = reg_name(reg_field(instr, 3));
      const char*  rd = reg_name(reg_field(instr));
      int imm = 0;
      if (!bit(instr, 12)) {
        // ldrh|strh <Rd>, [<Rn>, #<immed_5> * 4]
        stream()->print(bit(instr,11) ? "ldrh" : "strh");
        stream()->print("\t%s, [%s", rd, rn);
        imm = ((instr >> 6) & 0x1F) * 2;
      } else {
        // ldr|str <Rd>, [SP, #<immed_5> * 4]
        stream()->print(bit(instr,11) ? "ldr" : "str");
        stream()->print("\t%s, [sp", rd, rn);
        imm = (instr & 0xFF);
      }
      if (imm == 0) {
        stream()->print("]");
      } else {
        stream()->print(", #%d]", imm);
      }
      break;
    }
    
    case 5:
    {
       const char*  rd = reg_name(reg_field(instr, 8));
      if (!bit(instr, 12)) {
        // add <Rd>, PC|SP, #<immed_8> * 4
        stream()->print("add\t%s, %s, #%d", rd, (bit(instr,11) ? "sp" : "pc"),
                          (instr & 0xFF) * 4);
        int target = (instr_offset & ~3) + 4 + ((instr & 0xFF) * 4);
        if (GenerateGNUCode) { 
          stream()->print("/* = %d */", target);
        } else { 
          stream()->print("; = %d ", target);
        }
      } else {
        // Miscellaneous - TO DO
        emit_unknown(instr);
      }
      break;
    }
    
    case  6:
    {
      int type_bits = (instr >> 8) & 0xF;
      if (bit(instr, 12) == 0) {
        // ldmia|stmia <Rd>!, <registers>
        const Assembler::Register rn = reg_field(instr, 8);
        const bool l = bit(instr, 11);
        stream()->print(l ? "ldmia" : "stmia");
        stream()->print("\t%s!", reg_name(rn));
        stream()->print(", ");
        emit_register_list(instr);
      } else if (type_bits == 0xF) {
        // swi <immed_8>
        stream()->print("swi\t0x%06x", (instr & 0xFF));
      } else if (type_bits == 0xE){
        // Unknown instruction
        emit_unknown(instr);
      } else {
        // b<cond> <signed_immed_8>
        Assembler::Condition cond = 
               Assembler::as_condition((instr >> 8) & 0xF);
        int offset =
             ((instr & 0xFF) | (bit(instr, 7) ? 0xffffff00 : 0)) << 1;  

        stream()->print("b%s\t", cond_name(cond));
        int target = offset + 4;
        const char *sign = (target >= 0) ? "+" : "-";
        stream()->print("pc %s %d\t", sign, abs(target));
        if (addr != NULL || instr_offset != NO_OFFSET) { 
          stream()->print(GenerateGNUCode ? "/* " : "; ");
          if (addr != NULL && VerbosePointers) { 
            stream()->print("=0x%08x ", (int)addr + 4 + offset);
          }
          if (offset != NO_OFFSET) { 
            stream()->print("=%d ", instr_offset + 4 + offset);
          }
          if (GenerateGNUCode) { 
            stream()->print("*/");
          }
        }
      }
      break;
    }
    
    case 7:
    {
      int type_bits = (instr >> 11) & 0x3;
      if (type_bits == 0 || type_bits == 2) {
        // b <#signed immed_11>
        int offset = 0;
        char *opcode;
        if ( type_bits == 0) {
          offset = ((instr & 0x7FF) | (bit(instr, 10) ? 0xfffff800 : 0)) << 1; 
          opcode = (char*)"b";
        } else {
          offset = (instr & 0x7FF);
          offset = ((offset << 12) << 9) >> 9;
          offset += ((*(addr + 1) & 0x7FF)) << 1;
          opcode = (char*)"bl";
        }

        int target = offset + 4;
        const char *sign = (target >= 0) ? "+" : "-";
        stream()->print("%s\tpc %s %d\t", opcode, sign, abs(target));

        if (addr != NULL || instr_offset != NO_OFFSET) { 
          stream()->print(GenerateGNUCode ? "/* " : "; ");
          if (addr != NULL && VerbosePointers) { 
            stream()->print("=0x%04x ", (int)addr + 4 + offset);
          }
          if (offset != NO_OFFSET) { 
            stream()->print("=%d ", instr_offset + 4 + offset);
          }
          if (GenerateGNUCode) { 
            stream()->print("*/");
          }
        }
      } else if (type_bits == 3) {
          stream()->print(GenerateGNUCode ? 
                          "\t/* bl suffix */ " : "\t; bl suffix ");
      } else {
        emit_unknown(instr);
      }
      break;
    }
    
    default: SHOULD_NOT_REACH_HERE();
  }

  if (_eol_comments) {
    stream()->print(GenerateGNUCode ? " /* " : " ; ");
    tty->print(" %s", _eol_comments);
    if (GenerateGNUCode) { 
      stream()->print("*/");
    }
    _eol_comments = NULL;
  }

  if (_last_imm_offset != instr_offset) {
    _last_imm_register = Assembler::no_reg;
  }

  return num_half_words;
}

#endif // !defined(PRODUCT) || ENABLE_TTY_TRACE
