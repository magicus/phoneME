/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

# include "incls/_precompiled.incl"
# include "incls/_Disassembler_thumb2.cpp.incl"

#ifndef PRODUCT
const char* Disassembler::_eol_comments;
#if ENABLE_COMPILER
const char* Disassembler::_unresolved_label_name;
#endif

int Disassembler::decode_imm(unsigned int p_value) {
  GUARANTEE(!(p_value & ~0xFFF), "sanity");
  if (p_value < 256) {
    return p_value;
  }
  unsigned int code = (p_value & 0xF00) >> 8;
  unsigned int first_8_bits = p_value & 0xFF;

  if (code == 1) {
    return first_8_bits | (first_8_bits << 16);
  } else if (code == 2) {
    return (first_8_bits << 8) | (first_8_bits << 24);
  } else if (code == 3) {
    return first_8_bits | (first_8_bits << 8) | (first_8_bits << 16) | (first_8_bits << 24);
  }
  first_8_bits = (p_value & 0x7F) | 0x80;
  int shift_num = (p_value & 0xF80) >> 7;
  return ((unsigned int)(first_8_bits) >> shift_num) | (first_8_bits<<(32 - shift_num));
}

void Disassembler::emit_unknown(const unsigned short instr) {
  stream()->print("%s\t0x%04x", GenerateGNUCode ? ".word" : "DCD", instr );
}
void Disassembler::emit_unknown(const unsigned short hw1, const unsigned short hw2 ) {
  stream()->print("%s\t0x%04x, 0x%04x", GenerateGNUCode ? ".word" : "DCD", hw1, hw2 );
}

bool Disassembler::emit_GUARANTEE(bool check, const char* comment) {
  if (check) return false;
  stream()->print("failed validating instruction");
  return true;
}

void Disassembler::emit_register_list(short instr) {
  stream()->put('{');
  int b = -1;
  bool comma = false;
  // clear upper bits so we can run past last register (see below)!
  instr &= 0xff;
  for (int i = 0; i <= Assembler::number_of_gp_registers; i++) {
    // run past last register (by 1)!
    // b <  0 => no open range
    // b >= 0 => open range [b, i-1]
    if ((instr & 1 << i) != 0) {
      // register ri included
      if (b < 0) {
        b = i; // open a new range
      }
    } else {
      // register ri excluded
      if (b >= 0) {
        // print open range
        if (comma) {
          stream()->print(", ");
        }
        stream()->print( register_name( b ) );
        if (b < i-1) {
          stream()->print("%s%s",
                          (b == i-2 ? ", " : " - "),
                          register_name( i-1 ) );
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

void Disassembler::emit_target_offset(const int offset, const int instr_offset){
  const int target = (instr_offset & ~3) + 4 + offset;
  stream()->print( GenerateGNUCode ? "/* = %d */" : "; = %d ", target);
}

const char* Disassembler::condition_name(const Assembler::Condition cond) {
  static const char* cond_names[Assembler::number_of_conditions] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "",   "nv"
  };
  return cond_names[cond];
}

const char* Disassembler::register_name(const Assembler::Register reg) {
  static const char* const reg_names_a[Assembler::number_of_registers] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc"
  };
  static const char* const reg_names_b[Assembler::number_of_registers] = {
#if ENABLE_ARM_V7
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "jsp", "gp", "fp", "r12", "sp", "lr", "pc"
#else
    "r0", "r1", "r2", "r3", "fp" , "gp", "jsp", "r7",
    "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc"
#endif
  };
  return (GenerateAssemblyCode ? reg_names_a : reg_names_b) [reg];
}

#if ENABLE_ARM_VFP
inline char Disassembler::precision(const jushort /*hw1*/, const jushort hw2 ){
  return (hw2 & (1 << 8)) ? 'd' : 's';
}
inline jushort Disassembler::rm_field(const jushort /*hw1*/, const jushort hw2){
  return hw2 & 0xf;
}
inline jushort Disassembler::rn_field(const jushort hw1, const jushort/*hw2*/){
  return hw1 & 0x0f;
}
inline jushort Disassembler::rd_field(const jushort /*hw1*/, const jushort hw2){
  return (hw2 >> 12) & 0x0f;
}
inline jushort Disassembler::m_bit   (const jushort /*hw1*/, const jushort hw2){
  return bit(hw2, 5);
}
inline jushort Disassembler::n_bit(const jushort /*hw1*/, const jushort hw2) {
  return bit(hw2, 7);
}
inline jushort Disassembler::d_bit(const jushort hw1, const jushort /*hw2*/) {
  return bit(hw1, 6);
}
inline jushort Disassembler::l_bit(const jushort hw1, const jushort /*hw2*/) {
  return bit(hw1, 4);
}
inline jushort Disassembler::fm_field(const jushort hw1, const jushort hw2) {
  return rm_field(hw1, hw2) << 1 | m_bit(hw1, hw2);
}
inline jushort Disassembler::fn_field(const jushort hw1, const jushort hw2) {
  return rn_field(hw1, hw2) << 1 | n_bit(hw1, hw2);
}
inline jushort Disassembler::fd_field(const jushort hw1, const jushort hw2) {
  return rd_field(hw1, hw2) << 1 | d_bit(hw1, hw2);
}

void Disassembler::vfp_reg_name(const char type, unsigned reg, char buffer[]) {
  GUARANTEE(reg < 32, "Invalid vfp register");
  if( type == 'd' ) {
//    GUARANTEE((reg & 1) == 0, "Invalid vfp double register");
    reg >>= 1;
  }

  jvm_sprintf(buffer, "%c%u", type, reg);
}
#endif

inline void Disassembler::disasm_v6t2_coproc( const jushort hw1,
                                              const jushort hw2,
                                              const int instr_offset )
{
#if ENABLE_ARM_VFP
  const jushort fn = fn_field( hw1, hw2 );
  const jushort fm = fm_field( hw1, hw2 );
  const jushort fd = fd_field( hw1, hw2 );
  const char type  = precision( hw1, hw2 );

  char fn_name[4];
  char fm_name[4];
  char fd_name[4];

  vfp_reg_name( type, fn, fn_name );
  vfp_reg_name( type, fm, fm_name );
  vfp_reg_name( type, fd, fd_name );

  const jushort L = l_bit(hw1, hw2);

  if( ((hw1 >> 8) & 0x0f) == 0x0e ) {
    if( bit(hw2, 4) == 0 ) {            // Data processing
      const jushort p = bit(hw1, 7);
      const jushort q = bit(hw1, 5);
      const jushort r = bit(hw1, 4);
      const jushort s = bit(hw2, 6);
      const jushort pqrs = p << 3 | q << 2 | r << 1 | s;

      if( pqrs != 0x0f ) {
        const char* op;
        switch( pqrs ) {
          case 0x00: op = "fmac";  break;
          case 0x01: op = "fnmac"; break;
          case 0x02: op = "fmsc";  break;
          case 0x03: op = "fnmsc"; break;
          case 0x04: op = "fmul";  break;
          case 0x05: op = "fnmul"; break;
          case 0x06: op = "fadd";  break;
          case 0x07: op = "fsub";  break;
          case 0x08: op = "fdiv";  break;
          default:   op = "VFP_UNDEF";
        }
        stream()->print("%s%c\t%s, %s, %s", op, type, fd_name, fn_name, fm_name);
      } else if( fn == 15 ) {
        const char fd_type = type == 'd' ? 's' : 'd';
        vfp_reg_name(fd_type, fd, fd_name);
        stream()->print("fcvt%c%c\t%s, %s", fd_type, type, fd_name, fm_name);
      } else {
        const char* op;
        switch( fn ) {
          case 0x00: op = "fcpy";   break;
          case 0x01: op = "fabs";   break;
          case 0x02: op = "fneg";   break;
          case 0x03: op = "fsqrt";  break;
          case 0x08: op = "fcmp";   break;
          case 0x09: op = "fcmpe";  break;
          case 0x0a: op = "fcmpz";  break;
          case 0x0b: op = "fcmpez"; break;
          case 0x10: op = "fuito";  break;
          case 0x11: op = "fsito";  break;
          case 0x18: op = "ftoui";  break;
          case 0x19: op = "ftouiz"; break;
          case 0x1a: op = "ftosi";  break;
          case 0x1b: op = "ftosiz"; break;
          default:   op = "VFP_UNDEF";
        }
        stream()->print("%s%c\t%s, %s", op, type, fd_name, fm_name);
      }
      return;
    }
    if( type == 's' && ((hw2 >> 4) & 0x07) == 1 ) {     // Single register transfer
      const jushort rd     = rd_field(hw1, hw2);
      const char* rd_name  = register_name( rd );
      const jushort opcode = (hw1 >> 5) & 0x07;

      if( opcode == 0 ) {
        if( L == 0 ) {
          stream()->print("fmsr\t%s, %s", fn_name, rd_name);
        } else {
          stream()->print("fmrs\t%s, %s", rd_name, fn_name);
        }
        return;
      }
      if( opcode == 7 && n_bit(hw1, hw2) == 0 ) {
        const char* vfp_sysreg = fn == 2 ? "fpscr" : "UNDEF_VFP_SYSREG";
        if( L == 0 ) {
          stream()->print("fmxr\t%s, %s", vfp_sysreg, rd_name);
        } else if( fn == 2 && rd == 0x0f ) {
          stream()->print("fmstat");
        } else {
          stream()->print("fmrx\t%s, %s", rd_name, vfp_sysreg);
        }
        return;
      }
    }
  }
  if( ((hw1 >> 9) & 0x07) == 0x6 ) {
    if( ((hw1 >> 5) & 0x0f) == 0x2 && ((hw2 >> 4) & 0xff) == 0xb1 ) {
      const char* rd_name = register_name( rd_field(hw1, hw2) );
      const char* rn_name = register_name( rn_field(hw1, hw2) );
      if( L == 0 ) {
        stream()->print("fmdrr\t%s, %s, %s", fm_name, rd_name, rn_name);
      } else {
        stream()->print("fmrrd\t%s, %s, %s", rd_name, rn_name, fm_name);
      }
      return;
    }

    const jushort rn = rn_field(hw1, hw2);
    const char* rn_name = register_name(rn);

    const jushort P = bit(hw1, 8);
    const jushort U = bit(hw1, 7);
    const jushort W = bit(hw1, 5);
    const jushort puw = P << 2 | U << 1 | W;

    switch( puw ) {
      case 2: case 3: case 5: {
        char suffix1, suffix2;
        if( rn == Assembler::sp || rn == Assembler::jsp ) {
          // use stack addressing mnemonics
          suffix1 = L == P ? 'e' : 'f';
          suffix2 = L == U ? 'd' : 'a';
        } else {
          // use non-stack addressing mnemonics
          suffix1 = U ? 'i' : 'd';
          suffix2 = P ? 'b' : 'a';
        }
        stream()->print("f%sm%c%c%c\t%s", (L ? "ld" : "st"), suffix1, suffix2,
                        type, rn_name);
        if( W ) {
          stream()->put('!');
        }

        stream()->print(", {");
        for( int freg = fd, end = (hw2 & 0xFF) + fd;; stream()->put(',') ) {
          char buffer [4];
          vfp_reg_name(type, freg, buffer);
          stream()->print(buffer);
          if( ++freg >= end ) break;
        }
        stream()->put('}');
        return;
      }
      case 4: case 6: {
        stream()->print("f%s%c\t%s, [%s", (L ? "ld" : "st"), type,
                        fd_name, rn_name);
        int offset = (hw2 & 0xff) << 2;
        if( !U ) {
          offset = -offset;
        }
        if( offset ) {
          stream()->print(", #%d", offset);
        }
        stream()->put(']');

        if( instr_offset != NO_OFFSET && rn == Assembler::pc ) {
          emit_target_offset( instr_offset, offset );
        }
        return;
      }
    }
  }
#endif
  if( hw2 & 0x10 ) {
    // mcr.w, mrc.w
    stream()->print("%s.w\tp%d, %d, %s, c%d, c%d, %d",
                    (hw1 & 0x10) ? "mrc" : "mcr", (hw2 >> 8) & 15,
                    (hw1 >> 5) & 7,
                    register_name( hw2 >> 12 ),
                    hw1 & 15, hw2 & 15, (hw2 >> 5) & 7);
  } else {
    emit_unknown( hw1, hw2 );
    UNIMPLEMENTED(); // other instructions
  }
}

const char* Disassembler::shift_name(const Assembler::Shift shift) {
  static const char* shift_names[Assembler::number_of_shifts] = {
    "lsl", "lsr", "asr", "ror"
  };
  GUARANTEE(unsigned(shift) < Assembler::number_of_shifts, "illegal shift");
  return shift_names[shift];
}

const char* Disassembler::opcode_name(const Assembler::Opcode opcode) {
  static const char* const opcode_names[Assembler::number_of_opcodes] = {
    "and", "eor", "lsl", "lsr", "asr", "adc", "sbc", "ror",
    "tst", "neg", "cmp", "cmn", "orr", "mul", "bic", "mvn",
    "add", "sub", "mov"
  };
  static const char* const opcode_names_s[Assembler::number_of_opcodes] = {
    "ands", "eors", "lsls", "lsrs", "asrs", "adcs", "sbcs", "rors",
    "tst",  "negs", "cmp",  "cmn",  "orrs", "muls", "bics", "mvns",
    "adds", "subs", "movs"
  };
  GUARANTEE(unsigned(opcode) < Assembler::number_of_opcodes, "illegal opcode");
  return (GenerateGNUCode ? opcode_names : opcode_names_s) [opcode];
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
    const int size = tmpl->size;
    if (uoffset < size) {
      if (size == 4) {
        return tmpl->name;
      } else {
        jvm_sprintf(buff, "%s[%d]", tmpl->name, uoffset);
        return buff;
      }
    }
    uoffset -= size;
  }

  return NULL;
}

void Disassembler::print_gp_name(int imm) {
   const char* name = find_gp_name(imm);
   if( name != NULL ) {
     const char* const format =
       GenerateGNUCode && !GenerateROMImage ? " @ = %s" : "; = %s ";
     stream()->print(format, name);
   }
}

inline void Disassembler::comments( const short* addr, int num_half_words ) {
  if( GenerateAssemblyCode || _eol_comments ) {
    while( stream()->position() <= 40 ) {
      stream()->print(" ");
    }
    stream()->print(GenerateGNUCode ? " /" "*" : " ;");

    if (GenerateAssemblyCode) {
      do {
        stream()->print(" 0x%04x", (unsigned short)*addr++);
      } while (--num_half_words);
    }

    if (_eol_comments) {
      stream()->print(" %s", _eol_comments);
      _eol_comments = NULL;
    }

    if( GenerateGNUCode ) {
      stream()->print(" */");
    }
  }
}

void Disassembler::disasm_new16bit(const short* addr, short instr,
                                   const int instr_offset)
{
  static const char* const hint[] = {"nop", "yield", "wfe", "wfi", "sev"};

  start_thumb2(16, addr, instr);

  if (instr & (1 << 10)) { //IT or hints
    if (((instr >> 8) & 0xFF) != 0xBF) {
      stream()->print("DCW 0x%04x", instr & 0xffff);
    } else if (instr & (0xF)) { //IT
      // There are some troubles with IT:
      // Assembler requires us to make each instruction in IT block
      // explicitly conditional, but with current implementation of
      // disassembler it's not easy to do so -
      // let's just print HEX codes instead
      stream()->print("DCW 0x%04x ; ", instr & 0xffff);
      int cond = (instr >> 4) & 0xF;
      stream()->print("IT");
      while (instr & 7) {
        stream()->print("%c", ((instr >> 3) ^ cond) & 1 ? 'E' : 'T');
        instr <<= 1;
      }
      stream()->print(" %s", condition_name( cond ) );

    } else { //hint
      int hint_num = (instr >> 4) & 0xF;
      stream()->print("%s", hint[hint_num]);
    }
  } else { //CZB
    Register rn = as_register(instr & 0x7);
    const char* cond= condition_name( (instr >> 11) & 0x1 );
    int offset = ((instr & (0x1 << 9)) >> 3) | ((instr & (0x1F << 3)) >> 2);
    offset += 4;
    stream()->print("czb%s %s %d", cond, register_name(rn), offset);
  }

  end_thumb2();
}

inline void Disassembler::disasm_v6t2_data_load_store_double_and_exclusive(
                                short instr, short hw2)
{
  const char* rn   = register_name(reg_field_w(instr));
  const char* rxf  = register_name(reg_field_w(hw2, 12));
  const char* rxf2 = register_name(reg_field_w(hw2, 8));
  int imm8 = (hw2 & 0xFF) << 2;
  const char* singess[] = {"-", ""};
  const char* sizes[] = {"b", "h", "error", "d"};
  if ((instr >> 8) & 0x1) { //ldrd/strd pre_idx
    const char* wb[] = {"", "!"};
    const char* code[] = {"strd", "ldrd"};
    stream()->print("%s.w\t%s, %s, [%s, #%s%d]%s", code[(instr >> 4) & 0x1],
        rxf, rxf2, rn, singess[(instr >> 7) & 0x1], imm8, wb[(instr >> 5) & 0x1]);
  } else if ((instr >> 5) & 0x1) { //ldrd/strd post_idx
    const char* code[] = {"strd", "ldrd"};
    stream()->print("%s.w\t%s, %s, [%s], #%s%d", code[(instr >> 4) & 0x1],
        rxf, rxf2, rn, singess[(instr >> 7) & 0x1], imm8);
  } else if ((instr >> 7) & 0x1) { //load/store exclusive byte and etc
    const char *rm = register_name(reg_field_w(hw2));
    int op = (hw2 >> 4) & 0xF;
    if (op & 0x4) { //ldrex*
      const char* code[] = {"str", "ldr"};
      if (!(op & 0x3)) {
        rxf2 = "";
      }
      stream()->print("%sex%s.w\t%s, %s, [%s]", code[(instr >> 4) & 0x1],
        sizes[op & 0x3], rxf, rxf2, rn);
    } else {//tbb, tbh
      const char* lsl[] = {"", ", lsl #1"};
      stream()->print("tb%s.w\t[%s, %s%s]",
        sizes[op & 0x1], rn, rm, lsl[op & 0x1]);
    }
  } else {//ldrex/strex
    const char* code[] = {"strex", "ldrex"};
    stream()->print("%s.w\t%s, %s, [%s, #%d]", code[(instr >> 4) & 0x1],
        rxf2, rxf, rn, imm8);
  }
}

inline void Disassembler::disasm_v6t2_data_load_store_multiple(
                                short instr, short hw2)
{
  int u = (instr >> 8) & 0x1;
  int v = (instr >> 7) & 0x1;
  int l = (instr >> 4) & 0x1;
  int w = (instr >> 5) & 0x1;
  const char *rn = register_name(reg_field_w(instr));
  static const char* const mode[] = {"ia", "db"};
  static const char* const wb[] = {"", "!"};
  if (u == v) {
    if (l) { //rfe
      stream()->print("rfe%s.w\t%s%s", mode[u], rn, wb[w]);
    } else { //srs
      int mode13 = hw2 & 0xF;
      stream()->print("srs%s.w\t#%d%s", mode[u], mode13, wb[w]);
    }
  } else { //stm/ldm
    const char* code[] = {"stm", "ldm"};
    stream()->print("%s%s.w\t%s%s, ", code[l], mode[u], rn, wb[w]);
    emit_register_list(hw2);
  }
}

inline void Disassembler::disasm_v6t2_data_load_store_single(
                                short instr, short hw2)
{
  Assembler::Register rn_reg = reg_field_w(instr);
  Assembler::Register rxf_reg = reg_field_w(reg_field_w(hw2, 12));
  const char* rn = register_name(rn_reg);
  const char* rxf = register_name(rxf_reg);
  int size = (instr >> 5) & 0x3;
  bool load = (instr >> 4) & 0x1;
  int S = (instr >> 8) & 0x1;
  int U = (instr >> 7) & 0x1;
  static const char* const sign_suff[] = {"", "s"};
  static const char* const size_suff[] = {"b", "h", "", "error3"};
  int imm8 = hw2 & 0xFF;
  int imm12 = hw2 & 0xFFF;
  if (load) {
    if ((instr & 0xF) == 0xF) { //pc +- imm12
      stream()->print("ldr%s%s.w\t%s, [pc, #%c%d]",
        sign_suff[S], size_suff[size], rxf, "-+"[S], imm12);
      return;
    }
    if (U == 1) {
      stream()->print("ldr%s%s.w\t%s, [%s, #%d]",
        sign_suff[S], size_suff[size], rxf, rn, imm12);
      if (rn_reg == Assembler::gp) {
         print_gp_name(imm12);
      }
      return;
    }
    int subcode = (hw2 >> 8) & 0xF;
    if (subcode == 0xC) {//rn - imm8
      stream()->print("ldr%s%s.w\t%s, [%s, #-%d]",
        sign_suff[S], size_suff[size], rxf, rn, imm8);
    } else if (subcode == 0x9) {//post_idx_neg
      stream()->print("ldr%s%s.w\t%s, [%s], #-%d",
        sign_suff[S], size_suff[size], rxf, rn, imm8);
    } else if (subcode == 0xB) {//post_idx_pos
      stream()->print("ldr%s%s.w\t%s, [%s], #%d",
        sign_suff[S], size_suff[size], rxf, rn, imm8);
    } else if (subcode == 0xD) {//pre_idx_neg
      stream()->print("ldr%s%s.w\t%s, [%s, #-%d]!",
        sign_suff[S], size_suff[size], rxf, rn, imm8);
    } else if (subcode == 0xF) {//pre_idx_pos
      stream()->print("ldr%s%s.w\t%s, [%s, #%d]!",
        sign_suff[S], size_suff[size], rxf, rn, imm8);
    } else if (subcode == 0x0) {//shifted register
      int shift = (hw2 >> 4) & 0x3;
      const char *rm = register_name(reg_field_w(hw2));
      if (shift) {
        stream()->print("ldr%s%s.w\t%s, [%s, %s, lsl #%d]",
          sign_suff[S], size_suff[size], rxf, rn, rm, shift);
      } else {
        stream()->print("ldr%s%s.w\t%s, [%s, %s]",
          sign_suff[S], size_suff[size], rxf, rn, rm);
      }
    } else if (subcode == 0xE) {//user privillege
      stream()->print("ldr%s%st.w\t%s, [%s, #%d]",
        sign_suff[S], size_suff[size], rxf, rn, imm8);
    }
  } else {
    if (U == 1) {
      stream()->print("str%s.w\t%s, [%s, #%d]",
        size_suff[size], rxf, rn, imm12);
      return;
    }
    int subcode = (hw2 >> 8) & 0xF;
    if (subcode == 0xC) {//rn - imm8
      stream()->print("str%s.w\t%s, [%s, #-%d]",
        size_suff[size], rxf, rn, imm8);
    } else if (subcode == 0x9) {//post_idx_neg
      stream()->print("str%s.w\t%s, [%s], #-%d",
        size_suff[size], rxf, rn, imm8);
    } else if (subcode == 0xB) {//post_idx_pos
      stream()->print("str%s.w\t%s, [%s], #%d",
        size_suff[size], rxf, rn, imm8);
    } else if (subcode == 0xD) {//pre_idx_neg
      stream()->print("str%s.w\t%s, [%s, #-%d]!",
        size_suff[size], rxf, rn, imm8);
    } else if (subcode == 0xF) {//pre_idx_pos
      stream()->print("str%s.w\t%s, [%s, #%d]!",
        size_suff[size], rxf, rn, imm8);
    } else if (subcode == 0x0) {//shifted register
      int shift = (hw2 >> 4) & 0x3;
      const char *rm = register_name(reg_field_w(hw2));
      if (shift) {
        stream()->print("str%s.w\t%s, [%s, %s, lsl #%d]",
          size_suff[size], rxf, rn, rm, shift);
      } else {
        stream()->print("str%s.w\t%s, [%s, %s]",
          size_suff[size], rxf, rn, rm);
      }
    } else if (subcode == 0xE) {//user privillege
      stream()->print("str%st.w\t%s, [%s, #%d]",
        size_suff[size], rxf, rn, imm8);
    }
  }
}

inline void Disassembler::disasm_v6t2_data_processing_no_imm(short instr,
                                                             short hw2) {
  const char *rn = register_name(reg_field_w(instr));
  const char *rd = register_name(reg_field_w(hw2, 8));
  const char *rd_low = register_name(reg_field_w(hw2, 12));
  const char *rm = register_name(reg_field_w(hw2));
  static const char *s_suffix[] = {"", "s", "<error>"};
  int op = (instr >> 4) & 0x7;
  int S = (instr >> 4) & 0x1;

  if (((instr >> 12) & 0x1) == 0) { //data processing constant shift
    static const char* opcode_w_names[] = {
    "and", "bic", "orr", "orn", "eor", "error:0101", "error:0110", "error:0111",
    "add", "error:1001", "adc", "sbc", "error:1100", "sub", "rsb", "error:1111"
    };
    op = (instr >> 5) & 0xF;
    const char *code = opcode_w_names[op];
    if (op == 2 && (instr & 0x0f) == 0x0f) {
      stream()->print("mov%s.w\t%s, %s", s_suffix[S], rd, rm);
    } else if (op == 3 && (instr & 0x0f) == 0x0f) {
      stream()->print("mvn%s.w\t%s, %s", s_suffix[S], rd, rm);
    } else if ((op == 0x0d) && (reg_field_w(hw2, 8) == Assembler::pc) && (S == 1)) {
      stream()->print("cmp.w\t%s, %s", rn, rm);
    } else {
      stream()->print("%s%s.w\t%s, %s, %s", code, s_suffix[S], rd, rn, rm);
    }
    int shift_value = ((hw2 >> 6) & 0x3) | ((hw2 >> 10) & (0x7 << 2)) ;
    int shift_type = (hw2 >> 4) & 0x3;
    if (shift_value == 0) {
      static const char* shifts32[4] = {"", ", lsr #32", ", asr #32", ", rrx"};
      stream()->print(shifts32[shift_type]);
    } else {
      stream()->print(", %s #%d", shift_name((Assembler::Shift)shift_type),
                                 shift_value);
    }
    return;
  }

  int b8 = ((instr >> 8) & 0x1);
  int b7 = ((instr >> 7) & 0x1);
  int b2_7 = ((hw2 >> 7) & 0x1);
  if (!b8 && !b7 && !b2_7) { //register controlled shift
    if (emit_GUARANTEE(((hw2 >> 4) & 0x7) == 0, "op2 could be only zero")) return;
    if (emit_GUARANTEE(((hw2 >> 12) & 0xF) == 0xF, "see encoding")) return;
    const char *shift = shift_name((Assembler::Shift)(op >> 1));
    stream()->print("%s%s.w\t%s, %s, %s", shift, s_suffix[S], rd, rn, rm);
  } else if (!b8 && !b7 && b2_7) { //sign or zero extension
    const char* signess = (op & 0x1) ? "uxt" : "sxt";
    const char* code = "";
    const char* prefix  = "a";
    op &= 0x6;
    if (op == 4) {
      code = "b";
    } else if (op == 2) {
      code = "16";
    } else if (op == 0) {
      code = "h";
    }
    if ((instr & 0xF) == 0xF) {
      prefix  = "";
      rn = "";
    }
    int ror = ((hw2 >> 4) & 0x3) << 3;
    stream()->print("%s%s%s.w\t%s, %s, %s, %s, ror #%d",
                    signess, prefix, code, rd, rn, rm, ror);
  } else if (!b8 && b7 && b2_7) { //other trhee reg processig data
    int op2 = ((hw2 >> 4) & 0x7);
    if (emit_GUARANTEE(op2 < 4, "see encoding")) return ;
    if (emit_GUARANTEE(op  < 4, "see encoding")) return ;
    if (op == 0) {
      const char* intrs[] = {"qadd", "qdadd", "qsub", "qdsub"};
      stream()->print("%s.w\t%s, %s, %s", intrs[op2], rd, rn, rm);
    } else if (op == 1) {
      const char* intrs[] = {"rev", "rev16", "rbit", "revsh"};
      stream()->print("%s.w\t%s, %s", intrs[op2], rd, rm);
    } else if (op == 2) {
      if (emit_GUARANTEE(op2 == 0, "see encoding")) return ;
      stream()->print("sel.w\t%s, %s, %s", rd, rn, rm);
    } else { //op == 3
      stream()->print("clz.w\t%s, %s", rd, rm);
    }
  } else if (!b8 && b7 && !b2_7) { //SIMD add or substract
    int prefix = ((hw2 >> 4) & 0x7);
    const char* prefixes[] = {"s", "q", "sh", "error3", "u", "uq", "uh", "error7"};
    const char* instres[] = {"add8", "add16", "addsubx", "error3", "sub8", "sub16", "subaddx", "error7"};
    stream()->print("%s%s.w\t%s, %s, %s",
                    prefixes[prefix], instres[op], rd, rn, rm);
  } else if (b8 && b7) { //64bit mult
    int op2 = (hw2 >> 4) & 0xF;
    const char* suffix[] = {"", "r"};
    if (op == 0) {
      if (emit_GUARANTEE(op2 == 0, "see encoding")) return ;
      stream()->print("smull%s.w\t%s, %s, %s, %s", suffix[op2], rd_low, rd, rn, rm);
    } else if (op == 1) {
      if (emit_GUARANTEE(op2 == 0xF, "see encoding")) return ;
      stream()->print("sdiv.w\t%s, %s, %s", rd, rn, rm);
    } else if (op == 2) {
      if (emit_GUARANTEE(op2 == 0, "see encoding")) return ;
      stream()->print("umull%s.w\t%s, %s, %s, %s", suffix[op2], rd_low, rd, rn, rm);
    } else if (op == 3) {
      if (emit_GUARANTEE(op2 == 0xF, "see encoding")) return ;
      stream()->print("udiv.w\t%s, %s, %s", rd, rn, rm);
    } else if (op2 == 0) {
      if (op == 4) {
        stream()->print("smlal.w\t%s, %s, %s, %s", rd_low, rd, rn, rm);
      } else if (op == 6) {
        stream()->print("umlal.w\t%s, %s, %s, %s", rd_low, rd, rn, rm);
      } else {
        SHOULD_NOT_REACH_HERE();
      }
    } else if (op == 4) {
      if (op2 & 0x4) {//smlald
        const char* suffix[] = {"", "x"};
        stream()->print("smlald%s.w\t%s, %s, %s, %s", suffix[op2 & 0x1], rd_low, rd, rn, rm);
      } else {//smlal<x><y>
        const char* suffix[] = {"bb", "bt", "tb", "tt"};
        stream()->print("smlal%s.w\t%s, %s, %s, %s", suffix[op2 & 0x3], rd_low, rd, rn, rm);
      }
    } else if (op == 5) {
      if (emit_GUARANTEE((op2 & 0xC) == 0xC, "see encoding")) return ;
      const char* suffix[] = {"", "x"};
      stream()->print("smlsld%s.w\t%s, %s, %s, %s", suffix[op2 & 0x1], rd_low, rd, rn, rm);
    } else {
      SHOULD_NOT_REACH_HERE();
    }
  } else if (b8 && !b7) { //32bit mult
    int op2 = (hw2 >> 4) & 0xF;
    int rac = (hw2 >> 12) & 0xF;
    const char* rc = (rac == 0xF) ? "" : register_name(reg_field_w(hw2, 12));
    if (op == 7) {//usad{8}
      if (rac == 0xF) {
        stream()->print("usad8.w\t%s, %s, %s",
                        rd, rn, rm);
      } else {
        stream()->print("usada8.w\t%s, %s, %s %s",
                        rd, rn, rm, register_name(reg_field_w(hw2, 12)));
      }
    } else if (op == 0) {
      if (!op2 && rac == 0xF) {
        stream()->print("mul.w\t%s, %s, %s", rd, rn, rm);
      } else {
        stream()->print("%s.w\t%s, %s, %s, %s", (op2 ? "mls" : "mla"), rd, rn, rm, rc);
      }
    } else if (op == 1) { //sml(a|s)<x><y>
      const char* code = (rac == 0xF) ? "mul" : "mla";
      const char* suffix[] = {"bb", "bt", "tb", "tt"};
      stream()->print("s%s%s.w\t%s, %s, %s", code, suffix[op2], rd, rn, rm, rc);
    } else if (op == 2) { //sm(l|u)ad<y>
      const char* code = (rac == 0xF) ? "u" : "l";
      const char* suffix[] = {"", "X"};
      stream()->print("sm%sad%s.w\t%s, %s, %s", code, suffix[op2], rd, rn, rm, rc);
    } else if (op == 3) { //sm(l|u)ad<y>
      const char* code = (rac == 0xF) ? "ul" : "la";
      const char* suffix[] = {"b", "t"};
      stream()->print("sm%sw%s.w\t%s, %s, %s", code, suffix[op2], rd, rn, rm, rc);
    } else if (op == 4) { //sm(u|l)sd>
      const char* code = (rac == 0xF) ? "u" : "l";
      const char* suffix[] = {"", "x"};
      stream()->print("sm%sds%s.w\t%s, %s, %s", code, suffix[op2], rd, rn, rm, rc);
    } else if (op == 5) { //smm(ul|la)
      const char* code = (rac == 0xF) ? "u" : "l";
      const char* suffix[] = {"", "r"};
      stream()->print("sm%sds%s.w\t%s, %s, %s", code, suffix[op2], rd, rn, rm, rc);
    } else if (op == 6) { //smmls
      const char* suffix[] = {"", "r"};
      stream()->print("smmls%s.w\t%s, %s, %s", suffix[op2], rd, rn, rm, rc);
    }
  }

  return;
}


inline void Disassembler::disasm_32bit(const short* addr, const short instr,
                                       const int instr_offset) {
  start_thumb2(32, addr, instr);

  jushort hw2 = (jushort)addr[1];

  if (((instr >> 11) & 0x3) == 2) {
    if ((hw2 & (1 << 15))) {
      disasm_v6t2_branches_and_misc(instr, hw2);
    } else {
      disasm_v6t2_data_processing(instr, hw2);
    }
  } else if (((instr >> 9) & 0x7) == 0x5) {//data processing no imm
    disasm_v6t2_data_processing_no_imm(instr, hw2);
  } else if (((instr >> 9) & 0xF) == 0xC) { //load and store single data items
    disasm_v6t2_data_load_store_single(instr, hw2);
/*  } else if (((instr >> 9) & 0x7F) == 0x7C) {
    int size = (instr >> 5) & 0x03;
    Assembler::Register rn  = reg(instr & 0x0f);
    Assembler::Register rxf = reg((hw2 >> 12) & 0x0f);
    int imm;
    const char *name = bit(instr, 4) ? "ldr" : "str";

    // load and store single data items
    if (rn == Assembler::pc) {
      // PC+-imm12
      if (bit(instr, 7)) { // +imm12
        imm = 0;
      } else { // -imm12
        imm = (0xffffffff >> 12) << 12;
      }
      imm |= (hw2 & 0xfff);
    } else if (bit(instr, 7)) {
      // Rn +imm12
      imm = hw2 & 0xfff;
    } else {
      imm = 999999; // IMPL_NOTE: consider whether it should be fixed.
    }

    static const char *suffix[] = {"b", "h", "", "<error>"};
    stream()->print("%s%s.w\t%s, [%s, #%d]", name, suffix[size],
                    register_name(rxf), register_name(rn), imm);*/
  } else if (((instr >> 9) & 0xF) == 0x4) {
    if (bit(instr, 6)) {
      disasm_v6t2_data_load_store_double_and_exclusive(instr, hw2);
    } else {
      disasm_v6t2_data_load_store_multiple(instr, hw2);
    }
  } else if ((instr & 0xec00) == 0xec00) {
    disasm_v6t2_coproc(instr, hw2, instr_offset);
  }

  end_thumb2();
}

void Disassembler::disasm_v6t2_data_processing(short instr, short hw2) {
  static const char* const opcode_w_names[] = {
    "and", "bic", "orr", "orn", "eor", "error:0101", "error:0110","error:0111",
    "add", "error:1001", "adc", "sbc", "error:1100", "sub", "rsb","error:1111"
  };
  static const char* const s_suffix[] = {"", "s", "<error>"};
  const char *rn = register_name(reg_field_w(instr));
  const char *rd = register_name(reg_field_w(hw2, 8));
  const char* op_code_name = "";

  if (((instr >> 9) & 0x1) == 0) { // encoded 12 bit imm
    int opcode_value = (instr >> 5) & 0xF;
    const char *code = opcode_w_names[opcode_value];
    int S = (instr >> 4) & 0x1;
    int imm12 = hw2 & 0xFF | ((hw2 >> 4) & (0x7 << 8)) | ((instr << 1) & (0x1 << 11));
    imm12 = decode_imm(imm12);
    if ((((hw2 >> 8) & 0xF) ^ 0xF) == 0) {//clash in cmn, cmp, teq, tst
      if (S) {
        if (opcode_value == 0x0) {
          code = "tst";
          S = 0;
          rd = NULL;
        } else if (opcode_value == 0x4) {
          code = "teq";
          S = 0;
          rd = NULL;
        } else if (opcode_value == 0x8) {
          code = "cmn";
          S = 0;
          rd = NULL;
        } else if (opcode_value == 0xD) {
          code = "cmp";
          S = 0;
          rd = NULL;
        }
      }
    } else if (((instr & 0xF) ^ 0xF) == 0) {//clash in cmn, cmp, teq, tst
      if (opcode_value == 0x2) {
        code = "mov";
        rn = NULL;
      } else if (opcode_value == 0x3) {
        code = "mvn";
        rn = NULL;
      }
    }
    stream()->print("%s%s.w\t", code, s_suffix[S]);
    if (rd) {
      stream()->print("%s, ", rd);
    }
    if (rn) {
      stream()->print("%s, ", rn);
    }
    stream()->print("#%d", imm12);
  } else {
    if (((instr >> 8) & 0x1) == 0) {
      int code = ((instr >> 4) & 0xB);
      int imm12 = hw2 & 0xFF | ((hw2 >> 4) & (0x7 << 8)) | ((instr << 1) & (0x1 << 11));
      if (((instr >> 6) & 0x1) == 0) {// 12 bit plain imm
        if (code == 0) {
          op_code_name = "addw";
        } else if (code == 0xA) {
          op_code_name = "subw";
        } else {
          op_code_name = "error";
          stream()->print("error: encoded values are: %d %d\n", instr, hw2);
          SHOULD_NOT_REACH_HERE();
        }
        stream()->print("%s.w\t%s, %s, #%d", op_code_name, rd, rn, imm12);
      } else {// 16 bit plain imm
        int imm16 = imm12 | ((instr & 0xF) << 12);
        if (code == 0) {
          op_code_name = "movw";
        } else if (code == 0x8) {
          op_code_name = "movt";
        } else {
          op_code_name = "error";
          stream()->print("error: encoded values are: %d %d\n", instr, hw2);
          SHOULD_NOT_REACH_HERE();
        }
        stream()->print("%s.w\t%s, #%d", op_code_name, rd, imm16);
      }
    } else {
      if (((instr >> 4) & 0x1) == 0) {
        int op = (instr >> 5) & 0x7;
        int val1 = ((hw2 >> 6) & 0x3) | ((hw2 >> 10) & 0x7);
        int val2 = (hw2  & 0x1F);
        const char* sixteen = "";
        const char* shift;
        if (op == 3) { //bfc/bfi
          if (((instr & 0xF) ^ 0xF) == 0) {
            op_code_name = "bfc";
            rn = "";
          } else {
            op_code_name = "bfi";
          }
          stream()->print("%s.w\t%s, %s, #%d, #%d",
                               op_code_name, rd, rn, val1, val2 - val1 + 1);
        } else if (op < 7) {
          const char* signess = (op > 3) ? "u" : "s";
          if ((op & 0x3) == 2) { //bfx
            op_code_name = "bfx";
            stream()->print("%s%s.w\t%s, %s, #%d, #%d",
                        signess, op_code_name, rd, rn, val1, val2 - val1 + 1);
            return;
          } else if ((op & 0x3) == 0) { //ssat lsl only
            op_code_name = "ssat";
            shift = "lsl";
          } else { //ssat asr or ssat16
            op_code_name = "ssat";
            if (val2 == 0) {
              sixteen = "16";
              shift = "";
            } else {
              shift = "asr";
            }
          }
          stream()->print("%s%s%s.w\t%s, #%d, %s, %s #%d",
                     signess, op_code_name, sixteen, rd, val2 + 1, rn, shift,
                     val1);

        } else {
          SHOULD_NOT_REACH_HERE();
        }
      } else {
        stream()->print("reserved code space!\n");
        SHOULD_NOT_REACH_HERE();
      }
    }
  }
}

void Disassembler::disasm_v6t2_branches_and_misc(short instr, short hw2) {
  int code1 = (hw2 >> 12) & 0xD;
  int S = (instr >> 10) & 0x1;
  int cond = (instr >> 6) & 0xF;
  const char *rn = register_name(reg_field_w(instr));
  if (code1 > 0x8) { //branches
    int I1 = (hw2 >> 13) & 0x1;
    int I2 = (hw2 >> 11) & 0x1;
    int offset = ((hw2 & 0x7FF) << 1) | ((instr & 0x3FF) << 12) |
      ((I1 == S ? 1 : 0) << 23) | ((I2 == S ? 1 : 0) << 22) | S << 24;
    offset = offset << 7 >> 7;

    const char* name = "undefined";
    switch (code1) {
      case 0x9: name = "b";   break;
      case 0xD: name = "bl";  break;
      case 0xC: name = "blx"; break;
    }
    stream()->print("%s.w\t%d", name, offset);
  } else if (cond < 0xE) {//conditional branch
    int offset = ((hw2 & 0x7FF) << 1) | ((instr & 0x3F) << 12) |
     (hw2 & (1 << 13)) << 5 | (hw2 & (1 << 11)) << 8 | S << 20;
    offset = offset << 11 >> 11;
    stream()->print("b%s.w\t%d", condition_name(cond), offset);
  } else if (S == 1) {//smi
    if (emit_GUARANTEE((hw2 & 0xFFF0) == 0xF7F0, "see encoding")) return;
    int imm16 = (instr & 0xF) | (hw2 & 0x0FF0) | ((hw2 & 0xF) << 12);
    stream()->print("smi.w\t%d", imm16);
  } else if (!((instr >> 6) & 0x3)) {//msr
    const char* psr[] = {"cpsr", "spsr"};
    const char* c = ((hw2 >> 8) & 0x1) ? "c":"";
    const char* x = ((hw2 >> 8) & 0x1) ? "x":"";
    const char* s = ((hw2 >> 8) & 0x1) ? "s":"";
    const char* f = ((hw2 >> 8) & 0x1) ? "f":"";
    stream()->print("msr.w\t%s_%s%s%s%s, %s", psr[(instr >> 6) & 0x1], c, x, s, f, rn);
  } else if (((instr >> 5) & 0x3) == 0x1) {
    if (((instr >> 4) & 0x1)) { //sco
      int op = (hw2 >> 4) & 0xF;
      const char* codes[] = {"error0", "error1", "clrex", "error3", "dsb", "dmb", "isb"};
      stream()->print("%s.w\t", codes[op]);
    } else {//
      if ((hw2 >> 8) & 0x7) {//cps
        int imod = (hw2 >> 9) & 0x3;
        int m = (hw2 >> 8) & 0x1;
        int mode = hw2 & 0x1F;
        if (imod == 0) {
          stream()->print("cps.w\t#%d", mode);
        } else {
          const char* imodes[] = {"error0", "error1", "ie", "id"};
          const char* a = ((hw2 >> 5) & 0x1) ? "a":"";
          const char* i = ((hw2 >> 6) & 0x1) ? "i":"";
          const char* f = ((hw2 >> 7) & 0x1) ? "f":"";
          if (m) {
            stream()->print("cps%s.w\t %s%s%s, #%d", imodes[imod], a, i, f, mode);
          } else {
            stream()->print("cps%s.w\t %s%s%s", imodes[imod], a, i, f);
          }
        }
      } else {//hints
        int hint = hw2 & 0xFF;
        if (emit_GUARANTEE(hint < 5, "see encoding")) return;
        const char* hints[] = {"nop", "yield", "wfe", "wfi", "sev"};
        stream()->print("%s.w", hints[hint]);
      }
    }
  } else if (((instr >> 5) & 0x3) == 0x2) {
    if ((instr >> 4) & 0x1) {
      int imm8 = hw2 & 0xFF;
      stream()->print("subs.w pc, lr, #%d", imm8);
    } else {
      stream()->print("bxj.w %s", rn);
    }
  } else if (((instr >> 5) & 0x3) == 0x3) {
    const char* rn = register_name(reg_field_w(hw2, 8));
    const char* psr[] = {"cpsr", "spsr"};
    stream()->print("mrs.w %s %s", rn, psr[(instr >> 4) & 0x1]);
  }
}

void Disassembler::start_thumb2(const int num_bits,
                                const short* addr,
                                const jushort instr) {
  if (GenerateAssemblyCode) {
    if (GenerateGNUCode) {
      // We don't have an assembler that can handle T2 opcode yet, so let's
      // jump the hex numbers instead.
      if (num_bits == 16) {
        stream()->print(".short\t0x%04x\n/" "*\t", instr & 0xffff);
      } else {
        jushort hw2 = (jushort)addr[1];
        stream()->print(".short\t0x%04x\n\t.short\t0x%04x\n/" "*\t",
                        instr & 0xffff, hw2 & 0xffff);
      }
    } else {
      // RVDS supports T2 assembler - it's OK to print symbolic notation
    }
  }
}

void Disassembler::end_thumb2() {
  if (GenerateAssemblyCode) {
    if (GenerateGNUCode) {
      stream()->print(" *" "/");
    } else {
      // Nothing needed here
    }
  }
}

inline int Disassembler::disasm_internal(const short* addr, short instr,
                                         const int instr_offset)
{ // decode instruction and print on stream
#if ENABLE_COMPILER
  if( _unresolved_label_name ) {
    const char* unresolved_label_name = _unresolved_label_name;
    _unresolved_label_name = NULL;

    const Instruction instr((address)addr);
    switch( instr.kind() ) {
      case Instruction::vfp: {
#if USE_ARM_VFP_LITERALS
        const VFPMemAccess m((address)addr);
        char buffer[4];
        vfp_reg_name( 's', m.reg(), buffer );
        stream()->print("flds\t%s, [%s]", buffer, unresolved_label_name );
#else
        SHOULD_NOT_REACH_HERE();
#endif
        return 2;
      }
      case Instruction::ldr: {
        const MemAccess m( (address)addr );
        const char* rd = register_name( m.reg() );
        stream()->print("ldr\t%s, [pc + %s]", rd, unresolved_label_name );
        return 1;
      }
      default: { // branch, branch_link
        const Branch b( (address)addr );
        const char* suffix = "";
        if( b.is_long() ) {
          suffix = ".w";
        }
        const char* cond = "";
        if( b.is_conditional() ) {
          cond = condition_name( b.condition() );
        }
        stream()->print("b%s%s\t%s", cond, suffix, unresolved_label_name );
        return b.is_long() ? 2 : 1;
      }
    }
  }
#endif
#if ENABLE_ARM_V7
  if (((instr >> 12) & 0xF) == 0xC) { //new xenon instructions
    start_thumb2(16, addr, instr);

    if (!bit(instr, 11)) {
      int hdl_id = instr & 0x1F;
      if (bit(instr, 10)) { //hblp
        int imm = (instr >> 5) & 0x1F;
        stream()->print("hblp.w #%d, #%d", imm, hdl_id);
      } else if (bit(instr, 9)) { //hb{l}
        const char* l = bit(instr, 8) ? "l" : "";
        hdl_id = instr & 0xFF;
        stream()->print("hb%s.w #%d", l, hdl_id);
      } else if (!bit(instr, 8)) { //hbp
        int imm = (instr >> 5) & 0x7;
        stream()->print("hbp.w #%d, #%d", imm, hdl_id);
      } else {
        emit_unknown(instr);
      }
    } else {
      int subcode = (instr >> 9) & 0x3;
      const char*  rn = register_name( (instr >> 3) & 0x7 );
      const char*  rd = register_name( instr & 0x7 );
      if (subcode == 0) { //ldr_neg
        int imm = ((instr >> 6) & 0x7) << 2;
        stream()->print("ldr.neg %s, [%s, #-%d]", rd, rn, imm);
      } else if (subcode == 1) { //
        if (bit(instr, 8)) { //ldr[r10]
          int imm5 = ((instr >> 3) & 0x1F) << 2;
          stream()->print("ldr.r10\t%s, [%s, #%d]",
                          rd, register_name(Assembler::r10), imm5);
          if (Assembler::gp == Assembler::r10) {
            print_gp_name(imm5);
          }
        } else { //chka
          int rn_num = (instr & 0x7) | ((instr >> 4) & 0x8);
          rd = register_name( rn_num );
          stream()->print("chka\t%s, %s", rd, rn);
        }
      } else {//ldr/str[r9]
        int imm6 = ((instr >> 3) & 0x3F) << 2;
        const char* code = bit(instr, 9) ? "str.r9": "ldr.r9";
        stream()->print("%s\t%s, [%s, #%d]", code, rd,
                        register_name(Assembler::r9), imm6);
      }
    }

    end_thumb2();
    return 1;
  }

  jushort hw2 = (jushort)addr[1];
  if (jushort(instr) == 0xF3BF && (hw2 & ~0x10) == 0x870F) {
    start_thumb2(32, addr, instr);
    stream()->print("%s", bit(hw2, 4) ? "enterx" : "leavex");
    end_thumb2();
    return 2;
  }
#endif

  // New thumb-2 16-bit instr
  if ((((instr >> 8) & 0xFF) == 0xBF) || ((((instr >> 8) & ~0xA)  ^ 0xB1) == 0)) {
    disasm_new16bit(addr, instr, instr_offset);
    return 1;
  }

  int op32 = (instr >> 11) & 0x1F;
  if ((op32 & 0x1c) == 0x1c && (op32 & 0x03) != 0) {
    // Bits [15:11] is 0b111xx, where xx is not 00: this is 32-bit instr
    disasm_32bit(addr, instr, instr_offset);
    return 2;
  }

  // ARM_V5T Thumb 16-bit instructions
  int op = (instr >> 13) & 0x7;
  int num_half_words = 1;
  switch (op) {
    case 0: {
      bool is_add_sub = (bit(instr, 12) && bit(instr, 11));
      const char*  rn = register_name(reg_field(instr, 3));
      const char*  rd = register_name(reg_field(instr));

      if (!is_add_sub) {
        // Shift by immediate
        // lsl | lsr | asr <Rd>, <Rm>, #<immed_5>
        const Assembler::Opcode opcode =
            Assembler::as_opcode((instr >> 11 & 0x3) + 2);
        stream()->print("%s", opcode_name(opcode));
        stream()->print("\t%s, %s, #%d", rd, rn, ((instr >> 6) & 0x1F));
      } else {
        // add/sub register/immediate
        const char* s_suffix = GenerateGNUCode ? "" : "s";
        stream()->print("%s%s", ((bit(instr, 9) ? "sub" : "add")), s_suffix);
        if (!bit(instr, 10)) {
          // add|sub <Rd>,   <Rm>, <Rn>
          const char*  rm = register_name(reg_field(instr, 6));
          stream()->print("\t%s, %s, %s", rd, rn, rm);
        } else {
          // add|sub <Rd>, <Rn>, #<3-bit imme.>
          stream()->print("\t%s, %s, #%d", rd, rn, ((instr >> 6) & 0x7));
        }
      }
      break;
    }
    case 1: {
      // add|sub|cmp|mov <Rd/Rn>, #<8-bit imme>
      static const char* const ops [] = {
        "mov", "cmp", "add", "sub"
      };
      const char* op = ops[ (instr >> 11) & 0x3 ];
      const char* rd = register_name(reg_field(instr, 8));
      const char* s_suffix = GenerateGNUCode ? "" : "s";
      stream()->print("%s%s\t%s, #%d", op, s_suffix, rd, (instr & 0xFF));
      break;
    }
    case 2: {
      if (((instr >> 11) & 0x1F) == 0x9) {
        // ldr Rd, [PC, #<immed_8> * 4]
        const char* rd = register_name(reg_field(instr, 8));
        const unsigned offset = (instr & 0xFF) * 4;
        stream()->print("ldr\t%s, [pc, #%u]", rd, offset);
        emit_target_offset(instr_offset, offset);
      } else if (bit(instr, 12)) {
        // ldr|str <Rd>, [<Rn>, <Rm>]
        static const char* const ops[] = {
          "str", "strh", "strb", "ldrsb",
          "ldr", "ldrh", "ldrb", "ldrsh"
        };
        const char* op = ops[ (instr >> 9) & 0x7 ];
        const char* rd = register_name(reg_field(instr, 0));
        const char* rn = register_name(reg_field(instr, 3));
        const char* rm = register_name(reg_field(instr, 6));
        stream()->print("%s\t%s, [%s, %s]", op, rd, rn, rm);
      } else if (bit(instr, 10)) {
        int opcode = (instr >> 8) & 0x3;
        Register reg_rd = as_register((instr       & 0x7) | (instr >> 4) & 0x8);
        Register reg_rm = as_register((instr >> 3) & 0xF);

        const char* rd = register_name( reg_rd );
        const char* rm = register_name( reg_rm );

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
          if (bit(instr, 7)) {
            stream()->print("blx\t%s", rm);
          } else {
            stream()->print("bx\t%s", rm);
          }
          break;
        default:
          SHOULD_NOT_REACH_HERE();
        }
      } else {
        const Assembler::Opcode opcode = Assembler::as_opcode(instr >> 6 & 0xf);
        const char* rd = register_name(reg_field(instr, 0));
        const char* rn = register_name(reg_field(instr, 3));
        stream()->print("%s\t%s, %s", opcode_name(opcode), rd, rn);
      }
      break;
    }
    case 3: {
      // ldrb|strb|ldr|str <Rd>, [<Rn>, #<immed_5> * 4]
      const Register rn_reg = reg_field( instr, 3);
      const Register rd_reg = reg_field( instr );
      const char* rn = register_name( rn_reg );
      const char* rd = register_name( rd_reg );
      const short opcode = (instr >> 11) & 3;
      static const char* const ops [] = {
        "str", "ldr", "strb", "ldrb"
      };
      stream()->print("%s\t%s, [%s", ops[opcode], rd, rn);
      int imm = ((instr >> 6) & 0x1F);
      if (imm == 0) {
        stream()->print("]");
      } else {
        if( !(opcode & 2) ) {
          imm <<= 2;
        }
        stream()->print(", #%d]", imm);
        if( rn_reg == Assembler::gp ) {
          print_gp_name(imm);
        }
      }
      break;
    }
  case 4:
    {
      const char* rn = register_name(reg_field(instr, 3));
      const char* rd = register_name(reg_field(instr));
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
      const char*  rd = register_name(reg_field(instr, 8));
      if (!bit(instr, 12)) {
        // add <Rd>, PC|SP, #<immed_8> * 4
        stream()->print("add\t%s, %s,#%d", rd, (bit(instr,11) ? "sp" : "pc"),
                          (instr & 0xFF) * 4);
        int target = (instr_offset & ~3) + 4 + ((instr & 0xFF) * 4);
        if (GenerateGNUCode) {
          stream()->print("/* = %d */", target);
        } else {
          stream()->print("; = %d ", target);
        }
      } else if (((instr >> 9) & 0x7A) == 0x5A) {
        short range = instr & 0xFF;
        stream()->print("%s\t", (bit(instr,11) ? "pop" : "push"));
        emit_register_list(range);
        if (bit(instr, 8)) {
          if (range != 0) {
            stream()->print(", ");
          }
          stream()->print("lr");
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
        const Register rn = reg_field(instr, 8);
        const bool l = bit(instr, 11);
        stream()->print(l ? "ldmia" : "stmia");
        stream()->print("\t%s!", register_name(rn));
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
        const int offset =
             ((instr & 0xFF) | (bit(instr, 7) ? 0xffffff00 : 0)) << 1;

        stream()->print("b%s\t", condition_name((instr >> 8) & 0xF));
        stream()->print("pc + %d\t", offset + 4);
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

        if ( type_bits == 0) {
          offset = ((instr & 0x7FF) | (bit(instr, 10) ? 0xfffff800 : 0)) << 1;
          stream()->print("b\tpc + %d\t", offset + 4);
        } else {
          offset = (instr & 0x7FF);
          offset = ((offset << 12) << 9) >> 9;
          offset += ((*(addr + 1) & 0x7FF)) << 1;
          stream()->print("bl\tpc + %d\t", offset + 4);
        }

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

  return num_half_words;
}

int Disassembler::disasm(short* addr, short instr, int instr_offset) {
  const int num_half_words = disasm_internal(addr, instr, instr_offset);
  comments( addr, num_half_words );
#if ENABLE_COMPILER
  _unresolved_label_name = NULL;
#endif
  return num_half_words;
}
#endif // PRODUCT
