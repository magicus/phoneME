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

// This disassembler has been copied from the x86 Solaris source code.
// It is C code that has been minimally modified to fit into a C++ world.
// Since this code is purely for debugging purposes, making it look pretty is
// not a high priority.


#include "incls/_precompiled.incl"

#if ENABLE_COMPILER

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

#include "incls/_Disassembler_i386.cpp.incl"

#define        NCPS    15    // number of chars per symbol 


// This is the structure that will be used for storing all the
//   op code information.  The structure values themselves are in '_tbls.c'.

struct    instable {
  char         name[NCPS];
  const struct instable *indirect;    // for decode op codes
  unsigned     adr_mode;
  int          suffix;
};

enum {
    UNKNOWN,
    MRw,
    IMlw,
    IMw,
    IR,
    OA,
    AO,
    Mv,
    Mw,
    Mem,
    Reg,
    RA,
    MR,
    IA,
    MA,
    SD,
    AD,
    SA,
    Disp,
    Disp8,
    INM,
    Imm,
    Port,
    PortV,
    DSHIFT,                     // for double shift that has an 8-bit immediate
    Discard,
    OVERRIDE,
    GO_ON,
    IMUL,                       // for 186 iimul instr` 
    MvI,                        // for 186 logicals
    ENTER,                      // for 186 enter instr 
    RMw,                        
    Imm8,                       // for push immediate byte
    Float1,                     // for 287 instructions
    Float2,                     // for 287 instructions
    DM,                         // 16-bit data
    AM,                         // 16-bit addr
    MIb,                        // for 386 logicals
    PREFIX,                     // an instruction prefix like REP, LOCK
    INT3,                       // The int 3 instruction
    DSHIFTcl,                   // for double shift that implicitly uses %cl
    CWD,                        // cwd and variants
    CBW,                        // cbw, etc
    RET,                        // single immediate 16-bit operand
    MOVZ,                       // for movs and movz, etc
    XADDB,                      // for xaddb
};


class x86_disassembler {
public:
  x86_disassembler(address addr, DisassemblerEnv* env) {
    _curloc = addr;
    _env = env;
    _result = env->buffer();
  }
  void disassemble(void);
  address curloc() { return _curloc; }
private:
  // mode indicates a single register with no displacement 
  enum {
    REG_ONLY = 3,
    LONGOPERAND = 1,
    OPLEN = 256                 // maximum length of a single operand
  };
  enum { 
    EAX = 0,
    ECX=  1,
    EDX=  2,
    EBX=  3,
    ESP=  4,
    EBP=  5,
    ESI=  6,
    EDI=  7
  };
  enum {
    UNKNOWN,
    MRw,
    IMlw,
    IMw,
    IR,
    OA,
    AO,
    Mv,
    Mw,
    Mem,
    Reg,
    RA,
    MR,
    IA,
    MA,
    SD,
    AD,
    SA,
    Disp,
    Disp8,
    INM,
    Imm,
    Port,
    PortV,
    DSHIFT,                     // for double shift that has an 8-bit immediate
    Discard,
    OVERRIDE,
    GO_ON,
    IMUL,                       // for 186 iimul instr` 
    MvI,                        // for 186 logicals
    ENTER,                      // for 186 enter instr 
    RMw,                        
    Imm8,                       // for push immediate byte
    Float1,                     // for 287 instructions
    Float2,                     // for 287 instructions
    DM,                         // 16-bit data
    AM,                         // 16-bit addr
    MIb,                        // for 386 logicals
    PREFIX,                     // an instruction prefix like REP, LOCK
    INT3,                       // The int 3 instruction
    DSHIFTcl,                   // for double shift that implicitly uses %cl
    CWD,                        // cwd and variants
    CBW,                        // cbw, etc
    RET,                        // single immediate 16-bit operand
    MOVZ,                       // for movs and movz, etc
    XADDB,                      // for xaddb
  };

  char  operand[3][OPLEN];      // to store operands as they appear
  const char *overreg;          // save the segment override register if any    
  int   data16;                 // 16- or 32-bit data 
  int   addr16;                 // 16- or 32-bit addressing 

  int WBIT(int x)  { return x & 0x1; }     // to get w bit
  int REGNO(int x) { return x & 0x7; }     // to get 3-bit register
  int VBIT(int x)  { return ((x)>>1 & 0x1); }  

  int OPSIZE(int data16, int wbit) { 
    return (wbit) ? ((data16) ? 2:4) : 1; 
  }

  char *_result;

  jubyte getbyte(void) {   return *_curloc++; }

  void get_modrm_byte(unsigned& mode, unsigned& reg, unsigned& r_m);
  void check_override(int opindex);
  void imm_data(int size, int opindex, bool comment = false);
  void get_opcode(unsigned &high, unsigned &low);
  void bad_opcode();
  void displacement(int no_bytes, bool signed_value, int opindex, long& value);
  void get_operand(unsigned mode, unsigned r_m, int wbit, int opindex);
  void get_bytes(int no_bytes, char *destbuf, long& destvalue);

  address    _curloc;
  DisassemblerEnv* _env;

  static const char *const REG16[8][2];
  static const char *const REG32[8][2];
  static const char dispsize16[8][4]; 
  static const char dispsize32[8][4]; 
  static const char *const regname16[4][8];
  static const char *const regname32[4][8];
  static const char *const scale_factor[4];
  static const char *const indexname[8];

  static const struct instable distable[16][16];

  static const struct instable op0FBA[8];
  static const struct instable op0FC8[4];
  static const struct instable op0F[16][16];
  static const struct instable op80[8];
  static const struct instable op81[8];
  static const struct instable op82[8];
  static const struct instable op83[8];
  static const struct instable opC0[8];
  static const struct instable opC1[8];
  static const struct instable opD0[8];
  static const struct instable opD1[8];
  static const struct instable opD2[8];
  static const struct instable opD3[8];
  static const struct instable opF6[8];
  static const struct instable opF7[8];
  static const struct instable opFE[8];
  static const struct instable opFF[8];

  static const struct instable opFP1n2[8][8];
  static const struct instable opFP3[8][8];
  static const struct instable opFP4[4][8];
  static const struct instable opFP5[8];
  static const struct instable opFP6[8];
  static const struct instable opFP7[8];
};

address disasm(address addr, DisassemblerEnv* env) {
  x86_disassembler dis(addr, env);
  dis.disassemble();

  char *buffer = env->buffer();
  char *cp =     buffer + strlen(buffer);
  while (cp-- > buffer && *cp == ' ') { 
    *cp = '\0';
  }
  return dis.curloc();
}


void
x86_disassembler::disassemble()
{
  const struct instable *dp;
  int wbit, vbit;
  unsigned mode, reg, r_m;

  // nibbles of the opcode
  unsigned opcode1, opcode2, opcode3, opcode4, opcode5;
  long    lngval;
  const char *reg_name = NULL;
  int got_modrm_byte;
  char mnemonic[OPLEN];
  // number of bytes of opcode - used to get wbit 
  char opcode_bytes;

  mnemonic[0] = '\0';
  _result[0] = '\0';
  operand[0][0] = '\0';
  operand[1][0] = '\0';
  operand[2][0] = '\0';
  addr16 = 0;
  data16 = 0;
  overreg = NULL;
  opcode_bytes = 0;

  for (;;) {
    get_opcode(opcode1, opcode2);
    dp = &distable[opcode1][opcode2];

    if (dp->adr_mode == PREFIX) {
      strcat(mnemonic, dp->name);
    } else if (dp->adr_mode == AM) {
      addr16 = !addr16;
    } else if (dp->adr_mode == DM) {
      data16 = !data16;
    } else if (dp->adr_mode == OVERRIDE) {
      overreg = dp->name;
    } else {
      break;
    }
  }

  // some 386 instructions have 2 bytes of opcode before the mod_r/m
  // byte so we need to perform a table indirection.
  if (dp->indirect == (struct instable *)op0F) {
    get_opcode(opcode4, opcode5);
    opcode_bytes = 2;
    if ((opcode4 == 0xc) && (opcode5 >= 0x8)) {
      dp = &op0FC8[0];
    } else { 
      dp = &op0F[opcode4][opcode5];
    }
  }
  
  got_modrm_byte = 0;
  if (dp->indirect != NULL) {
    // This must have been an opcode for which several
    // instructions exist.  The opcode3 field further decodes the instruction.
    got_modrm_byte = 1;
    get_modrm_byte(mode, opcode3, r_m);

    // decode 287 instructions (D8-DF) from opcodeN
    if (opcode1 == 0xD && opcode2 >= 0x8) {
      if (opcode2 == 0xB && mode == 0x3 && opcode3 == 4) {
        dp = &opFP5[r_m];
      } else if (opcode2 == 0xA && mode == 0x3 && opcode3 < 4){
        dp = &opFP7[opcode3];
      } else if (opcode2 == 0xB && mode == 0x3){
        dp = &opFP6[opcode3];
      } else if (opcode2 == 0x9 && mode == 0x3 && opcode3 >= 4) {
        dp = &opFP4[opcode3-4][r_m];
      } else if (mode == 0x3) {
        dp = &opFP3[opcode2-8][opcode3];
      } else {
        dp = &opFP1n2[opcode2-8][opcode3];
      }
    } else {
      dp = dp -> indirect + opcode3;
    }
  }

  if (dp->indirect != NULL) {
    bad_opcode();
    return;
  }

  // print the mnemonic
  if (dp->adr_mode != CBW && dp->adr_mode != CWD) {
    strcat(mnemonic, dp -> name);
    if (dp->suffix) {
      strcat(mnemonic, (data16? "w" : "l"));
    }
    sprintf(_result, (strlen(mnemonic) < (unsigned int)8 ?
                   "%-8s" : "%-8s "), mnemonic);
  }

  // Each instruction has a particular instruction syntax format
  // stored in the disassembly tables.  The assignment of formats
  //  to instructions was made by the author.  Individual formats
  //  are explained as they are encountered in the following
  // switch construct.

  switch (dp -> adr_mode) {
  case MOVZ:
    //
    // movsbl movsbw (0x0FBE) or movswl (0x0FBF)
    // movzbl movzbw (0x0FB6) or mobzwl (0x0FB7)
    // wbit lives in 2nd byte, note that operands are different sized

    // Get second operand first so data16 can be destroyed
    if (!got_modrm_byte) {
      get_modrm_byte(mode, reg, r_m);
    }
    reg_name = (data16 ? REG16 : REG32)[reg][LONGOPERAND];

    wbit = WBIT(opcode5);
    data16 = 1;
    get_operand(mode, r_m, wbit, 0);
    sprintf(_result, "%s%s, %s", _result, operand[0], reg_name);
    return;

  case IMUL:
    // imul instruction, with either 8-bit or longer immediate
    if (!got_modrm_byte) { 
      get_modrm_byte(mode, reg, r_m);
    }
    get_operand(mode, r_m, LONGOPERAND, 1);

    // opcode 0x6B for byte, sign-extended displacement, 0x69 for word(s)
    imm_data(OPSIZE(data16, opcode2 == 0x9), 0);
    reg_name = (data16 ? REG16 : REG32) [reg][LONGOPERAND];
    sprintf(_result, "%s%s, %s, %s", _result, operand[0], operand[1], reg_name);
    return;
    
  case MRw:
    // memory or register operand to register, with 'w' bit  
    wbit = WBIT(opcode2);
    if (!got_modrm_byte) {
      get_modrm_byte(mode, reg, r_m);
    } 
    get_operand(mode, r_m, wbit, 0);
    reg_name = (data16 ? REG16 : REG32)[reg][wbit];
    sprintf(_result, "%s%s, %s", _result, operand[0], reg_name);
    return;

  case RMw:
    // register to memory or register operand, with 'w' bit   
    // arpl happens to fit here also because it is odd 
    if (opcode_bytes == 2) { 
      wbit = WBIT(opcode5);
    } else { 
      wbit = WBIT(opcode2);
    }
    if (!got_modrm_byte) {
      get_modrm_byte(mode, reg, r_m);
    }
    get_operand(mode, r_m, wbit, 0);
    reg_name = (data16 ? REG16 : REG32) [reg][wbit];
    sprintf(_result, "%s%s, %s", _result, reg_name, operand[0]);
    return;

  case XADDB:
    // special case for the Pentium xaddb instruction 
    wbit = 0;
    if (!got_modrm_byte) {
      get_modrm_byte(mode, reg, r_m);
    }
    get_operand(mode, r_m, wbit, 0);
    reg_name = (data16 ? REG16 : REG32)[reg][wbit];
    sprintf(_result, "%s%s, %s", _result, reg_name, operand[0]);
    return;
    
  case DSHIFT:
    // Double shift. Has immediate operand specifying the shift. 
    get_modrm_byte(mode, reg, r_m);
    get_operand(mode, r_m, LONGOPERAND, 1);
    reg_name = (data16 ? REG16 : REG32)[reg][LONGOPERAND];
    imm_data(1, 0);
    sprintf(_result, "%s%s, %s, %s", _result, operand[0], reg_name, operand[1]);
    return;

  case DSHIFTcl:
    // Double shift, using implicit cl
    get_modrm_byte(mode, reg, r_m);
    get_operand(mode, r_m, LONGOPERAND, 0);
    reg_name = (data16 ? REG16 : REG32)[reg][LONGOPERAND];
    sprintf(_result, "%s%s, %s", _result, reg_name, operand[0]);
    return;

  case IMlw:
    // immediate to memory or register operand 
    wbit = WBIT(opcode2);
    get_operand(mode, r_m, wbit, 1);

    // A long immediate is expected for opcode 0x81, not 0x80 nor 0x83
    imm_data(OPSIZE(data16, opcode2 == 1), 0, true);
    sprintf(_result, "%s%s, %s", _result, operand[0], operand[1]);
    return;

  case IMw:
    // immediate to memory or register operand with the 'w' bit present
    wbit = WBIT(opcode2);
    if (!got_modrm_byte) {
      get_modrm_byte(mode, reg, r_m);
    }
    get_operand(mode, r_m, wbit, 1);
    imm_data(OPSIZE(data16, wbit), 0, true);
    sprintf(_result, "%s%s, %s", _result, operand[0], operand[1]);
    return;
    
  case IR: 
    // immediate to register with register in low 3 bits  of op code  
    wbit = opcode2 >>3 & 0x1;
    reg = REGNO(opcode2);
    reg_name = (data16 ? REG16 : REG32)[reg][wbit];
    imm_data(OPSIZE(data16, wbit), 0, true);
    sprintf(_result, "%s%s, %s", _result, operand[0], reg_name);
    return;

  case OA:
    // memory operand to accumulator  
    wbit = WBIT(opcode2);
    displacement(OPSIZE(addr16, LONGOPERAND), false, 0, lngval);
    reg_name = (data16 ? REG16 : REG32)[0][wbit];
    sprintf(_result, "%s%s, %s", _result, operand[0], reg_name);
    return;

  case AO:
    // accumulator to memory operand  
    wbit = WBIT(opcode2);
    displacement(OPSIZE(addr16, LONGOPERAND), false, 0, lngval);
    reg_name = (addr16 ? REG16 : REG32)[0][wbit];
    sprintf(_result, "%s%s, %s", _result, reg_name, operand[0]);
    return;

  case Mv:
    // rotate or shift instructions, which may shift by 1 or
    // consult the cl register, depending on the 'v' bit

    vbit = VBIT(opcode2);
    wbit = WBIT(opcode2);
    get_operand(mode, r_m, wbit, 0);

    reg_name = vbit ? "%cl" : "$1";
    sprintf(_result, "%s%s, %s", _result, reg_name, operand[0]);
    return;

  case MvI:
    // immediate rotate or shift instructions, which may or
    // may not consult the cl register, depending on the 'v' bit
    vbit = VBIT(opcode2);
    wbit = WBIT(opcode2);
    get_operand(mode, r_m, wbit, 0);
    imm_data(1, 1);
    reg_name = vbit ? "%cl, " : "";
    sprintf(_result, "%s%s,%s%s", _result, operand[1], reg_name, operand[0]);
    return;
    
  case MIb:
    get_operand(mode, r_m, LONGOPERAND, 0);
    imm_data(1, 1);
    sprintf(_result, "%s%s, %s", _result, operand[1], operand[0]);
    return;

  case Mw:
    // single memory or register operand with 'w' bit present 
    wbit = WBIT(opcode2);
    get_operand(mode, r_m, wbit, 0);
    sprintf(_result, "%s%s", _result, operand[0]);
    return;

  case Mem:
    // single memory or register operand 
    if (!got_modrm_byte) {
      get_modrm_byte(mode, reg, r_m);
    }
    get_operand(mode, r_m, LONGOPERAND, 0);
    sprintf(_result, "%s%s", _result, operand[0]);
    return;

  case Reg:
    // single register operand with register in the low 3 bits of op code
    if (opcode_bytes == 2) {
      reg = REGNO(opcode5);
    } else { 
      reg = REGNO(opcode2);
    }
    reg_name = (data16 ? REG16 : REG32)[reg][LONGOPERAND];
    sprintf(_result, "%s%s", _result, reg_name);
    return;

  case RA: {
    // register to accumulator with register in the low 3  bits of op code,
    // xchg instructions
    reg = REGNO(opcode2);
    reg_name = (data16 ? REG16 : REG32)[reg][LONGOPERAND];
    const char *eax_name = (data16 ? REG16 : REG32)[EAX][LONGOPERAND];
    sprintf(_result, "%s%s, %s", _result, reg_name, eax_name);
    return;
  }

  case MR:
    // memory or register operand to register 
    if (!got_modrm_byte) { 
      get_modrm_byte(mode, reg, r_m);
    }
    get_operand(mode, r_m, LONGOPERAND, 0);
    reg_name = (data16 ? REG16 : REG32)[reg][LONGOPERAND];
    sprintf(_result, "%s%s, %s", _result, operand[0], reg_name);
    return;

  case IA: {
    // immediate operand to accumulator 
    int no_bytes = OPSIZE(data16, WBIT(opcode2));
    switch (no_bytes) {
      case 1: reg_name = "%al"; break;
      case 2: reg_name = "%ax"; break;
      case 4: reg_name = "%eax"; break;
    }
    imm_data(no_bytes, 0, true);
    sprintf(_result, "%s%s, %s", _result, operand[0], reg_name);
    return;
  }

  case MA:
    // memory or register operand to accumulator
    wbit = WBIT(opcode2);
    get_operand(mode, r_m, wbit, 0);
    reg_name = (data16 ? REG16 : REG32) [0][wbit];
    sprintf(_result, "%s%s, %s", _result, operand[0], reg_name);
    return;

  case SD:
    // si register to di register
    check_override(0);
    sprintf(_result, "%s%s(%%%ssi), (%%%sdi)",
            _result, operand[0], addr16? "" : "e", addr16? "" : "e");
    return;

  case AD:
    //  accumulator to di register  
    wbit = WBIT(opcode2);
    check_override(0);
    reg_name = (data16 ? REG16 : REG32) [0][wbit];
    sprintf(_result, "%s%s, %s(%%%sdi)", _result,
            reg_name, operand[0], addr16? "" : "e");
    return;
    
  case SA:
    //  si register to accumulator 
    wbit = WBIT(opcode2);
    check_override(0);
    reg_name = (addr16 ? REG16 : REG32) [0][wbit];
    sprintf(_result, "%s%s(%%%ssi), %s", _result,
            operand[0], addr16? "" : "e", reg_name);
    return;
    
  case Disp:
  case Disp8: { 
    address here = _curloc;
    int size = OPSIZE(data16, dp->adr_mode == Disp);
    operand[0][0] = '$';
    get_bytes(size, operand[0], lngval);
    address target = _curloc + lngval;
    int code_offset = _env->code_offset(target);
    if (code_offset != -1) { 
      sprintf(operand[0], "L%d", code_offset);
    } else {
      sprintf(operand[0], "$%#lx", (long)target);
      _env->comment_on_immediate(here, (long)target);
    }
    sprintf(_result, "%s%s", _result, operand[0]);
    return;
  }

  case INM:
    // indirect to memory or register operand 
    get_operand(mode, r_m, LONGOPERAND, 0);
    sprintf(_result, "%s*%s", _result, operand[0]);
    return;

  case Imm:
  case Imm8:
    // single 32/16 bit immediate operand 
    // single 8 bit immediate operand 
    imm_data(OPSIZE(data16, dp->adr_mode == Imm), 0, true);
    sprintf(_result, "%s%s", _result, operand[0]);
    return;

  case ENTER:
    imm_data(2, 0);
    imm_data(1, 1);
    sprintf(_result, "%s%s, %s", _result, operand[0], operand[1]);
    return;

  case RET:
    // 16-bit immediate operand
    imm_data(2, 0);
    sprintf(_result, "%s%s", _result, operand[0]);
    return;

  case Port:
    //  single 8 bit port operand 
    check_override(0);
    imm_data(1, 0);
    sprintf(_result, "%s%s", _result, operand[0]);
    return;

  case PortV:
    // single operand, dx register (variable port instruction)
    check_override(0);
    sprintf(_result, "%s%s(%%dx)", _result, operand[0]);
    return;

  case INT3:
    sprintf(_result, "%s$0x3", _result);
    return;

  case Discard:
    // an unused byte must be discarded
    getbyte();
    return;
    
  case CBW:
    strcat(_result, data16 ? "cbtw" : "cwtl");
    return;

  case CWD:
    strcat(_result, data16 ? "cwtd" : "cltd");
    return;

  case GO_ON:
    // no disassembly, the mnemonic was all there was
    return;

  case Float1:
    sprintf(_result, "%s%%st(%1d)", _result, r_m);
    return;

  case Float2:
    if (opcode2 >> 2 & 0x1) {
      sprintf(_result, "%s%%st, %%st(%1d)", _result, r_m);
    } else {
      sprintf(_result, "%s%%st(%1d), %%st", _result, r_m);
    }
    return;

  case AM:
  case DM:
  case OVERRIDE:
  case PREFIX:
  case UNKNOWN:
    bad_opcode();
    return;

  default:
    SHOULD_NOT_REACH_HERE();
  } // end of switch
}


//
//  void get_modrm_byte (mode, reg, r_m)
//  Get the byte following the op code and separate it into the
//  mode, register, and r/m fields.
//  Scale-Index-Bytes have a similar format.

void
x86_disassembler::get_modrm_byte(unsigned& mode, unsigned& reg, unsigned& r_m)
{
  jubyte curbyte = getbyte();

  r_m = curbyte & 0x7; /* r/m field from curbyte */
  reg = curbyte >> 3 & 0x7; /* register field from curbyte */
  mode = curbyte >> 6 & 0x3; /* mode field from curbyte */
}

//    void check_override (opindex)
//
//    Check to see if there is a segment override prefix pending.
//    If so, print it in the current 'operand' location and set
//    the override flag back to false.


 void
   x86_disassembler::check_override(int opindex)
{
  if (overreg) {
    strcpy(operand[opindex], overreg);
  }
  overreg = NULL;
}


//  void displacement (no_bytes, opindex, value)
//
//    Get and print in the 'operand' array a one, two or four
//    byte displacement from a register.

void
x86_disassembler::displacement(int no_bytes, bool signed_value,
                               int opindex, long& value)
{
  char    temp[(NCPS*2)+1];
  long    val;

  get_bytes(no_bytes, temp, value);
  val = value;
  check_override(opindex);
  if (signed_value) {
    if (val >= -255 && val <= 255) { 
      sprintf(temp, "%ld", val);
    } else if (val >= 0 || val < 0xfff00000) {
      sprintf(temp, "%#lx", val);
    }  else {
      sprintf(temp, "-%#lx", -val);
    }
  }
  strcat(operand[opindex], temp);
}

void
x86_disassembler::get_operand(unsigned mode, unsigned r_m, int wbit, int opindex)
{
  int dispsize;                 // size of displacement in bytes 
  long dispvalue;               // value of the displacement 
  const char *resultreg;        // representation of index(es) 
  char *format;                 // output format of result 
  int s_i_b;                    // flag presence of scale-index-byte 
  unsigned ss;                  // scale-factor from opcode 
  unsigned index;               // index register number 
  unsigned base;                // base register number 
  char indexbuffer[16];         // char representation of index(es) 

  // if symbolic representation, skip override prefix, if any 
  check_override(opindex);

  // check for the presence of the s-i-b byte 
  if (r_m == ESP && mode != REG_ONLY && !addr16) {
    s_i_b = true;
    get_modrm_byte(ss, index, base);
  } else {
    s_i_b = false;
  }
  dispsize = (addr16 ? dispsize16 : dispsize32) [r_m][mode];
  
  if (s_i_b && mode == 0 && base == EBP) {
    dispsize = 4;
  }

  if (dispsize != 0) {
    displacement(dispsize, (s_i_b || mode), opindex, dispvalue);
  }

  if (s_i_b) {
    const char *basereg = regname32[mode][base];
    if (ss) {
      sprintf(indexbuffer, "%s%s,%s", basereg, indexname[index],
              scale_factor[ss]);
    } else {
      sprintf(indexbuffer, "%s%s", basereg, indexname[index]);
    }
    resultreg = indexbuffer;
    format = "%s(%s)";
  } else {
    // no s-i-b 
    if (mode == REG_ONLY) {
      format = "%s%s";
      resultreg = (data16 ? REG16 : REG32) [r_m][wbit];
    } else { /* Modes 00, 01, or 10 */
      resultreg = (addr16 ? regname16 :regname32) [mode][r_m];
      if (r_m == EBP && mode == 0) { /* displacement only */
        format = "%s";
      } else {
        // Modes 00, 01, or 10, not displacement only, and no s-i-b
        format = "%s(%s)";
      }
    }
  }
  sprintf(operand[opindex], format, operand[opindex], resultreg);
}

// get_bytes() reads no_bytes from a file and converts them into destbuf.
// A sign-extended value is placed into destvalue if it is non-null.

void
x86_disassembler::get_bytes(int no_bytes, char *destbuf, long& destvalue)
{
  int i;
  long shiftbuf = 0;
  int not_signed;

  for (i = 0; i < no_bytes; i++) {
    jubyte curbyte = getbyte();
    shiftbuf |= (long)curbyte << (8 * i);
  }
  
  switch (no_bytes) {
    case 1:
      if (shiftbuf & 0x80)
        shiftbuf |= 0xffffff00ul;
      break;
    case 2:
      if (shiftbuf & 0x8000)
        shiftbuf |= 0xffff0000ul;
      break;
  }
  destvalue = shiftbuf;
  if (shiftbuf & 0x80000000) {
    not_signed = 0;
    if ((strncmp(_result, "or", 2) == 0) ||
        (strncmp(_result, "and", 3) == 0) ||
        (strncmp(_result, "xor", 3) == 0) ||
        (strncmp(_result, "test", 4) == 0) ||
        (strncmp(_result, "in", 2) == 0) ||
        (strncmp(_result, "out", 3) == 0) ||
        (strncmp(_result, "lcall", 5) == 0) ||
        (strncmp(_result, "ljmp", 4) == 0) ||
        ((_result[0] == 'r') &&   // rotate/shift 
         ((_result[1] == 'c') || (_result[1] == 'o'))) ||
        ((_result[0] == 's') &&
         ((_result[1] == 'a') || (_result[1] == 'h'))) ||
        ((_result[0] == 'p') && (_result[1] == 's') &&
         ((_result[2] == 'r') || (_result[2] == 'l')))) {
      if (no_bytes == 1)
        shiftbuf &= 0xff;
      else if (no_bytes == 2)
        shiftbuf &= 0xffff;
      not_signed = 1;
    } else if (shiftbuf < 0xfff00000) {
      // don't negate kernel and kadb addresses 
      not_signed = 1;
    }
    if (not_signed == 0) {
      shiftbuf = -shiftbuf;
      *destbuf++ = '-';
    }
  }
  if (shiftbuf >= 0 && shiftbuf < 256) { 
    sprintf(destbuf, "%ld", shiftbuf);
  } else { 
    sprintf(destbuf, "%#lx", shiftbuf);
  }
}


//    void imm_data (no_bytes, opindex)
//
//    Determine if 1, 2 or 4 bytes of immediate data are needed, then
//    get and print them.

void
x86_disassembler::imm_data(int no_bytes, int opindex, bool comment)
{
  long value;
  int len = strlen(operand[opindex]);
  address here = _curloc;

  operand[opindex][len] = '$';
  get_bytes(no_bytes, &operand[opindex][len+1], value);
  if (no_bytes == 4 && comment) {
    _env->comment_on_immediate(here, value);
  }
}


//   get_opcode (high, low)
//   Get the next byte and separate the op code into the high and
//   low nibbles.

void
x86_disassembler::get_opcode(unsigned& high, unsigned& low)
{
  jubyte curbyte = getbyte();
  low = curbyte & 0xf;          // ----xxxx low 4 bits 
  high = curbyte >> 4 & 0xf;    // xxxx---- bits 7 to 4 
}

/*     bad_opcode    */
/*     print message and try to recover */

void
x86_disassembler::bad_opcode(void)
{
  strcpy(_result, "***ERROR--unknown op code***");
}

const char *const x86_disassembler::REG16[8][2] = {
  /* w bit            0           1        */
  /* reg bits */
  /* 000  */      { "%al",       "%ax" },
  /* 001  */      { "%cl",       "%cx" },
  /* 010  */      { "%dl",       "%dx" },
  /* 011  */      { "%bl",       "%bx" },
  /* 100  */      { "%ah",       "%sp" },
  /* 101  */      { "%ch",       "%bp" },
  /* 110  */      { "%dh",       "%si" },
  /* 111  */      { "%bh",       "%di" },
};

const char *const x86_disassembler::REG32[8][2] = {
    /* w bit        0        1        */
    /* reg bits */
    /* 000  */        { "%al",        "%eax" },
    /* 001  */        { "%cl",        "%ecx" },
    /* 010  */        { "%dl",        "%edx" },
    /* 011  */        { "%bl",        "%ebx" },
    /* 100  */        { "%ah",        "%esp" },
    /* 101  */        { "%ch",        "%ebp" },
    /* 110  */        { "%dh",        "%esi" },
    /* 111  */        { "%bh",        "%edi" }
};


const char x86_disassembler::dispsize16[8][4] = {
  /* mod        00    01    10    11 */
  /* r/m */
  /* 000 */    0,    1,    2,    0,
  /* 001 */    0,    1,    2,    0,
  /* 010 */    0,    1,    2,    0,
  /* 011 */    0,    1,    2,    0,
  /* 100 */    0,    1,    2,    0,
  /* 101 */    0,    1,    2,    0,
  /* 110 */    2,    1,    2,    0,
  /* 111 */    0,    1,    2,    0
};


const char x86_disassembler::dispsize32[8][4] = {
  /* mod        00    01    10    11 */
  /* r/m */
  /* 000 */    0,    1,    4,    0,
  /* 001 */    0,    1,    4,    0,
  /* 010 */    0,    1,    4,    0,
  /* 011 */    0,    1,    4,    0,
  /* 100 */    0,    1,    4,    0,
  /* 101 */    4,    1,    4,    0,
  /* 110 */    0,    1,    4,    0,
  /* 111 */    0,    1,    4,    0
};


const char *const x86_disassembler::regname16[4][8] = {
/*reg  000        001        010        011        100    101   110     111 */
/*mod*/
/*00*/ "%bx,%si", "%bx,%di", "%bp,%si", "%bp,%di", "%si", "%di", "",    "%bx",
/*01*/ "%bx,%si", "%bx,%di", "%bp,%si", "%bp,%di", "%si", "%di", "%bp", "%bx",
/*10*/ "%bx,%si", "%bx,%di", "%bp,%si", "%bp,%di", "%si", "%di", "%bp", "%bx",
/*11*/ "%ax",     "%cx",     "%dx",     "%bx",     "%sp", "%bp", "%si", "%di"
};


const char *const x86_disassembler::regname32[4][8] = {
/*reg   000     001     010     011     100     101     110     111 */
/*mod*/
/*00 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "",     "%esi", "%edi",
/*01 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi",
/*10 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi",
/*11 */ "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi"
};

const char *const x86_disassembler::scale_factor[4] = {
    "1", "2", "4", "8"
};

const char *const x86_disassembler::indexname[8] = {
    ",%eax",
    ",%ecx",
    ",%edx",
    ",%ebx",
    "",
    ",%ebp",
    ",%esi",
    ",%edi"
};


#define        INVALID    {"",NULL,UNKNOWN,0}

//  Main decode table for the op codes.  The first two nibbles
//  will be used as an index into the table.  If there is a
//  a need to further decode an instruction, the array to be
//  referenced is indicated with the other two entries being
//  empty.

const struct instable x86_disassembler::distable[16][16] = {
    {
/* [0,0] */    {"addb",NULL,RMw,0},
               {"add",NULL,RMw,1},
               {"addb",NULL,MRw,0},
               {"add",NULL,MRw,1},
/* [0,4] */    {"addb",NULL,IA,0},
               {"add",NULL,IA,1},
               INVALID, // {"push",NULL,SEG,1},
               INVALID, // {"pop",NULL,SEG,1},
/* [0,8] */    {"orb",NULL,RMw,0},
               {"or",NULL,RMw,1},
               {"orb",NULL,MRw,0},
               {"or",NULL,MRw,1},
/* [0,C] */    {"orb",NULL,IA,0},
               {"or",NULL,IA,1},
               INVALID, // {"push",NULL,SEG,1},
               {"",(const struct instable*)op0F,UNKNOWN,0}   // Check Unknown
       },
       {
/* [1,0] */    {"adcb",NULL,RMw,0},
               {"adc",NULL,RMw,1},
               {"adcb",NULL,MRw,0},
               {"adc",NULL,MRw,1},
/* [1,4] */    {"adcb",NULL,IA,0},
               {"adc",NULL,IA,1},
               INVALID, // {"push",NULL,SEG,1},
               INVALID, // {"pop",NULL,SEG,1},
/* [1,8] */    {"sbbb",NULL,RMw,0},
               {"sbb",NULL,RMw,1},
               {"sbbb",NULL,MRw,0},
               {"sbb",NULL,MRw,1},
/* [1,C] */    {"sbbb",NULL,IA,0},
               {"sbb",NULL,IA,1},
               INVALID, // {"push",NULL,SEG,1},
               INVALID, // {"pop",NULL,SEG,1},
             },
             {
/* [2,0] */    {"andb",NULL,RMw,0},
               {"and",NULL,RMw,1},
               {"andb",NULL,MRw,0},
               {"and",NULL,MRw,1},
 /* [2,4] */   {"andb",NULL,IA,0},
               {"and",NULL,IA,1},
               {"%es:",NULL,OVERRIDE,0},
               INVALID, // {"daa",NULL,GO_ON,0},
/* [2,8] */    {"subb",NULL,RMw,0},
               {"sub",NULL,RMw,1},
               {"subb",NULL,MRw,0},
               {"sub",NULL,MRw,1},
/* [2,C] */    {"subb",NULL,IA,0},
               {"sub",NULL,IA,1},
               {"%cs:",NULL,OVERRIDE,0},
               INVALID, // {"das",NULL,GO_ON,0},
             }, 
             {
/* [3,0] */    {"xorb",NULL,RMw,0},
               {"xor",NULL,RMw,1},
               {"xorb",NULL,MRw,0},
               {"xor",NULL,MRw,1},
/* [3,4] */    {"xorb",NULL,IA,0},
               {"xor",NULL,IA,1},
               {"%ss:",NULL,OVERRIDE,0},
               INVALID, // {"aaa",NULL,GO_ON,0},
/* [3,8] */    {"cmpb",NULL,RMw,0},
               {"cmp",NULL,RMw,1},
               {"cmpb",NULL,MRw,0},
               {"cmp",NULL,MRw,1},
/* [3,C] */    {"cmpb",NULL,IA,0},
               {"cmp",NULL,IA,1},
               {"%ds:",NULL,OVERRIDE,0},
               INVALID, // {"aas",NULL,GO_ON,0},
             },
             {
/* [4,0] */    {"inc",NULL,Reg,1},
               {"inc",NULL,Reg,1},
               {"inc",NULL,Reg,1},
               {"inc",NULL,Reg,1},
/* [4,4] */    {"inc",NULL,Reg,1},
               {"inc",NULL,Reg,1},
               {"inc",NULL,Reg,1},
               {"inc",NULL,Reg,1},
/* [4,8] */    {"dec",NULL,Reg,1},
               {"dec",NULL,Reg,1},
               {"dec",NULL,Reg,1},
               {"dec",NULL,Reg,1},
/* [4,C] */    {"dec",NULL,Reg,1},
               {"dec",NULL,Reg,1},
               {"dec",NULL,Reg,1},
               {"dec",NULL,Reg,1},
             },
             {
/* [5,0] */    {"push",NULL,Reg,1},
               {"push",NULL,Reg,1},
               {"push",NULL,Reg,1},
               {"push",NULL,Reg,1},
/* [5,4] */    {"push",NULL,Reg,1},
               {"push",NULL,Reg,1},
               {"push",NULL,Reg,1},
               {"push",NULL,Reg,1},
/* [5,8] */    {"pop",NULL,Reg,1},
               {"pop",NULL,Reg,1},
               {"pop",NULL,Reg,1},
               {"pop",NULL,Reg,1},
/* [5,C] */    {"pop",NULL,Reg,1},
               {"pop",NULL,Reg,1},
               {"pop",NULL,Reg,1},
               {"pop",NULL,Reg,1},
             },
             {
/* [6,0] */    {"pusha",NULL,GO_ON,1},
               {"popa",NULL,GO_ON,1},
               {"bound",NULL,MR,1},
               INVALID, // {"arpl",NULL,RMw,0},
/* [6,4] */    {"%fs:",NULL,OVERRIDE,0},
               {"%gs:",NULL,OVERRIDE,0},
               {"data16",NULL,DM,0},
               {"addr16",NULL,AM,0},
/* [6,8] */    {"push",NULL,Imm,1},
               {"imul",NULL,IMUL,1},
               {"push",NULL,Imm8,1},
               {"imul",NULL,IMUL,1},
/* [6,C] */    {"insb",NULL,GO_ON,0},
               {"ins",NULL,GO_ON,1},
               {"outsb",NULL,GO_ON,0},
               {"outs",NULL,GO_ON,1},
             }, 
             {
/* [7,0] */    {"jo",NULL,Disp8,0},
               {"jno",NULL,Disp8,0},
               {"jb",NULL,Disp8,0},
               {"jae",NULL,Disp8,0},
/* [7,4] */    {"je",NULL,Disp8,0},
               {"jne",NULL,Disp8,0},
               {"jbe",NULL,Disp8,0},
               {"ja",NULL,Disp8,0},
/* [7,8] */    {"js",NULL,Disp8,0},
               {"jns",NULL,Disp8,0},
               {"jp",NULL,Disp8,0},
               {"jnp",NULL,Disp8,0},
/* [7,C] */    {"jl",NULL,Disp8,0},
               {"jge",NULL,Disp8,0},
               {"jle",NULL,Disp8,0},
               {"jg",NULL,Disp8,0},
             },
             {
/* [8,0] */    {"",(const struct instable*)op80,UNKNOWN,0},
               {"",(const struct instable*)op81,UNKNOWN,0},
               {"",(const struct instable*)op82,UNKNOWN,0},
               {"",(const struct instable*)op83,UNKNOWN,0},
/* [8,4] */    {"testb",NULL,RMw,0},
               {"test",NULL,RMw,1},
               {"xchgb",NULL,RMw,0},
               {"xchg",NULL,RMw,1},
/* [8,8] */    {"movb",NULL,RMw,0},
               {"mov",NULL,RMw,1},
               {"movb",NULL,MRw,0},
               {"mov",NULL,MRw,1},
/* [8,C] */    INVALID, // {"movw",NULL,SM,0},
               {"lea",NULL,MR,1},
               INVALID, // {"movw",NULL,MS,0},
               {"pop",NULL,Mem,1},
             },
             {
/* [9,0] */    {"nop",NULL,GO_ON,0},
               {"xchg",NULL,RA,1},
               {"xchg",NULL,RA,1},
               {"xchg",NULL,RA,1},
/* [9,4] */    {"xchg",NULL,RA,1},
               {"xchg",NULL,RA,1},
               {"xchg",NULL,RA,1},
               {"xchg",NULL,RA,1},
/* [9,8] */    {"",NULL,CBW,0},
               {"",NULL,CWD,0},
               INVALID,
               {"fwait",NULL,GO_ON,0},
/* [9,C] */    {"pushf",NULL,GO_ON,1},
               {"popf",NULL,GO_ON,1},
               {"sahf",NULL,GO_ON,0},
               {"lahf",NULL,GO_ON,0},
             },
             {
/* [A,0] */    {"movb",NULL,OA,0},
               {"mov",NULL,OA,1},
               {"movb",NULL,AO,0},
               {"mov",NULL,AO,1},
/* [A,4] */    {"movsb",NULL,SD,0},
               {"movs",NULL,SD,1},
               {"cmpsb",NULL,SD,0},
               {"cmps",NULL,SD,1},
/* [A,8] */    {"testb",NULL,IA,0},
               {"test",NULL,IA,1},
               {"stosb",NULL,AD,0},
               {"stos",NULL,AD,1},
/* [A,C] */    {"lodsb",NULL,SA,0},
               {"lods",NULL,SA,1},
               {"scasb",NULL,AD,0},
               {"scas",NULL,AD,1},
             },
             {
/* [B,0] */    {"movb",NULL,IR,0},
               {"movb",NULL,IR,0},
               {"movb",NULL,IR,0},
               {"movb",NULL,IR,0},
/* [B,4] */    {"movb",NULL,IR,0},
               {"movb",NULL,IR,0},
               {"movb",NULL,IR,0},
               {"movb",NULL,IR,0},
/* [B,8] */    {"mov",NULL,IR,1},
               {"mov",NULL,IR,1},
               {"mov",NULL,IR,1},
               {"mov",NULL,IR,1},
/* [B,C] */    {"mov",NULL,IR,1},
               {"mov",NULL,IR,1},
               {"mov",NULL,IR,1},
               {"mov",NULL,IR,1},
             },
             {
/* [C,0] */    {"",(const struct instable*)opC0,UNKNOWN,0},
               {"",(const struct instable*)opC1,UNKNOWN,0},
               {"ret",NULL,RET,0},
               {"ret",NULL,GO_ON,0},
/* [C,4] */    INVALID, // {"les",NULL,MR,0},
               INVALID, // {"lds",NULL,MR,0},
               {"movb",NULL,IMw,0},
               {"mov",NULL,IMw,1},
/* [C,8] */    {"enter",NULL,ENTER,0},
               {"leave",NULL,GO_ON,0},
               {"lret",NULL,RET,0},
               {"lret",NULL,GO_ON,0},
/* [C,C] */    {"int",NULL,INT3,0},
               {"int",NULL,Imm8,0},
               INVALID, // {"into",NULL,GO_ON,0},
               INVALID, // {"iret",NULL,GO_ON,0},
             },
             {
/* [D,0] */    {"", (const struct instable*)opD0,UNKNOWN,0},
               {"", (const struct instable*)opD1,UNKNOWN,0},
               {"", (const struct instable*)opD2,UNKNOWN,0},
               {"", (const struct instable*)opD3,UNKNOWN,0},
/* [D,4] */    INVALID, // {"aam",NULL,Discard,0},
               INVALID, // {"aad",NULL,Discard,0},
               {"falc",NULL,GO_ON,0},
               {"xlat",NULL,GO_ON,0},
/* [D,8] */    {"", (const struct instable*)opFP1n2,UNKNOWN,0},
               {"", (const struct instable*)opFP1n2,UNKNOWN,0},
               {"", (const struct instable*)opFP1n2,UNKNOWN,0},
               {"", (const struct instable*)opFP1n2,UNKNOWN,0},
/* [D,C] */    {"", (const struct instable*)opFP1n2,UNKNOWN,0},
               {"", (const struct instable*)opFP1n2,UNKNOWN,0},
               {"", (const struct instable*)opFP1n2,UNKNOWN,0},
               {"", (const struct instable*)opFP1n2,UNKNOWN,0},
             },
             {
/* [E,0] */    {"loopnz",NULL,Disp8,0},
               {"loopz",NULL,Disp8,0},
               {"loop",NULL,Disp8,0},
               {"jcxz",NULL,Disp8,0},
/* [E,4] */    {"inb",NULL,Port,0},
               {"in",NULL,Port,1},
               {"outb",NULL,Port,0},
               {"out",NULL,Port,1},
/* [E,8] */    {"call",NULL,Disp,0},
               {"jmp",NULL,Disp,0},
               INVALID,
               {"jmp",NULL,Disp8,0},
/* [E,C] */    {"inb",NULL,PortV,0},
               {"in",NULL,PortV,1},
               {"outb",NULL,PortV,0},
               {"out",NULL,PortV,1},
             },
             {
/* [F,0] */    {"lock ",NULL,PREFIX,0},
               INVALID,
               {"repnz ",NULL,PREFIX,0},
               {"repz ",NULL,PREFIX,0},
/* [F,4] */    {"hlt",NULL,GO_ON,0},
               {"cmc",NULL,GO_ON,0},
               {"",opF6,UNKNOWN,0},
               {"",opF7,UNKNOWN,0},
/* [F,8] */    {"clc",NULL,GO_ON,0},
               {"stc",NULL,GO_ON,0},
               INVALID, // {"cli",NULL,GO_ON,0},
               INVALID, // {"sti",NULL,GO_ON,0},
/* [F,C] */    {"cld",NULL,GO_ON,0},
               {"std",NULL,GO_ON,0},
               {"",opFE,UNKNOWN,0},
               {"",opFF,UNKNOWN,0},
             }
};



const struct instable x86_disassembler::op0FBA[8] = {
/*  [0]  */    INVALID,
               INVALID,
               INVALID,
               INVALID,
               
/*  [4]  */    {"bt",NULL,MIb,1},
               {"bts",NULL,MIb,1},
               {"btr",NULL,MIb,1},
               {"btc",NULL,MIb,1},
};

/*
 *    Decode table for 0x0FC8 opcode -- 486 bswap instruction
 *
 * bit pattern: 0000 1111 1100 1reg
 */
const struct instable x86_disassembler::op0FC8[4] = {
/*  [0]  */    {"bswap",NULL,Reg,0},
               INVALID,
               INVALID,
               INVALID,
};

/*
 *    Decode table for 0x0F opcodes
 */

const struct instable x86_disassembler::op0F[16][16] = {
    {
/*  [00]  */   INVALID,
               INVALID,
               INVALID, // {"lar",NULL,MR,0},
               INVALID, // {"lsl",NULL,MR,0},
/*  [04]  */   INVALID,
               INVALID,
               INVALID, // {"clts",NULL,GO_ON,0},
               INVALID,
/*  [08]  */   INVALID, // {"invd",NULL,GO_ON,0},
               INVALID, // {"wbinvd",NULL,GO_ON,0},
               INVALID,
               INVALID, // {"ud2",NULL,GO_INVALID, // ON,0},
/*  [0C]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    },
    {
/*  [10]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [14]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [18]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [1C]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    },
    {
/*  [20]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [24]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [28]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [2C]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    },
    {
/*  [30]  */   INVALID, // {"wrmsr",NULL,GO_ON,0},
               INVALID, // {"rdtsc",NULL,GO_ON,0},
               INVALID, // {"rdmsr",NULL,GO_ON,0},
               INVALID, // {"rdpmc",NULL,GO_ON,0},
/*  [34]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [38]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [3C]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    },         
    {          
/*  [40]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [44]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [48]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [4C]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    },         
    {          
/*  [50]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [54]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [58]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [5C]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    },         
    {          
/*  [60]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [64]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [68]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [6C]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    },         
    {          
/*  [70]  */   INVALID,
               {"psrXXX",NULL,MR,0},
               {"psrXXX",NULL,MR,0},
               {"psrXXX",NULL,MR,0},
/*  [74]  */   INVALID,
               INVALID,
               INVALID,
               {"emms",NULL,GO_ON,0},
/*  [78]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [7C]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    },
    {
/*  [80]  */   {"jo",NULL,Disp,0},
               {"jno",NULL,Disp,0},
               {"jb",NULL,Disp,0},
               {"jae",NULL,Disp,0},
/*  [84]  */   {"je",NULL,Disp,0},
               {"jne",NULL,Disp,0},
               {"jbe",NULL,Disp,0},
               {"ja",NULL,Disp,0},
/*  [88]  */   {"js",NULL,Disp,0},
               {"jns",NULL,Disp,0},
               {"jp",NULL,Disp,0},
               {"jnp",NULL,Disp,0},
/*  [8C]  */   {"jl",NULL,Disp,0},
               {"jge",NULL,Disp,0},
               {"jle",NULL,Disp,0},
               {"jg",NULL,Disp,0},
    },         
    {          
/*  [90]  */   {"seto",NULL,Mem,0},
               {"setno",NULL,Mem,0},
               {"setb",NULL,Mem,0},
               {"setae",NULL,Mem,0},
/*  [94]  */   {"sete",NULL,Mem,0},
               {"setne",NULL,Mem,0},
               {"setbe",NULL,Mem,0},
               {"seta",NULL,Mem,0},
/*  [98]  */   {"sets",NULL,Mem,0},
               {"setns",NULL,Mem,0},
               {"setp",NULL,Mem,0},
               {"setnp",NULL,Mem,0},
/*  [9C]  */   {"setl",NULL,Mem,0},
               {"setge",NULL,Mem,0},
               {"setle",NULL,Mem,0},
               {"setg",NULL,Mem,0},
    },         
    {          
/*  [A0]  */   INVALID, // {"push",NULL,LSEG,1},
               INVALID, // {"pop",NULL,LSEG,1},
               INVALID, // {"cpuid",NULL,GO_ON,0},
               {"bt",NULL,RMw,1},
/*  [A4]  */   {"shld",NULL,DSHIFT,1},
               {"shld",NULL,DSHIFTcl,1},
               INVALID,
               INVALID,
/*  [A8]  */   INVALID, // {"push",NULL,LSEG,1},
               INVALID, // {"pop",NULL,LSEG,1},
               INVALID, // {"rsm",NULL,GO_ON,0},
               {"bts",NULL,RMw,1},
/*  [AC]  */   {"shrd",NULL,DSHIFT,1},
               {"shrd",NULL,DSHIFTcl,1},
               INVALID,
               {"imul",NULL,MRw,1},
    },         
    {          
/*  [B0]  */   {"cmpxchgb",NULL,RMw,0},
               {"cmpxchg",NULL,RMw,1},
               INVALID, // {"lss",NULL,MR,0},
               {"btr",NULL,RMw,1},
/*  [B4]  */   INVALID, // {"lfs",NULL,MR,0},
               INVALID, // {"lgs",NULL,MR,0},
               {"movzb",NULL,MOVZ,1},
               {"movzwl",NULL,MOVZ,0},
/*  [B8]  */   INVALID,
               INVALID,
               {"",op0FBA,UNKNOWN,0},
               {"btc",NULL,RMw,1},
/*  [BC]  */   {"bsf",NULL,MRw,1},
               {"bsr",NULL,MRw,1},
               {"movsb",NULL,MOVZ,1},
               {"movswl",NULL,MOVZ,0},
    },         
    {          
/*  [C0]  */   {"xaddb",NULL,XADDB,0},
               {"xadd",NULL,RMw,1},
               INVALID,
               INVALID,
/*  [C4]  */   INVALID,
               INVALID,
               INVALID,
               {"cmpxchg8b",NULL,Mem,0},
/*  [C8]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [CC]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    },         
    {          
/*  [D0]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [D4]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [D8]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [DC]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    },         
    {          
/*  [E0]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [E4]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [E8]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [EC]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    },         
    {          
/*  [F0]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [F4]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [F8]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [FC]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
    }
};


/*
 *    Decode table for 0x80 opcodes
 */

const struct instable x86_disassembler::op80[8] = {
/*  [0]  */    {"addb",NULL,IMlw,0},
               {"orb",NULL,IMw,0},
               {"adcb",NULL,IMlw,0},
               {"sbbb",NULL,IMlw,0},
/*  [4]  */    {"andb",NULL,IMw,0},
               {"subb",NULL,IMlw,0},
               {"xorb",NULL,IMw,0},
               {"cmpb",NULL,IMlw,0},
};


const struct instable x86_disassembler::op81[8] = {
/*  [0]  */    {"add",NULL,IMlw,1},
               {"or",NULL,IMw,1},
               {"adc",NULL,IMlw,1},
               {"sbb",NULL,IMlw,1},
/*  [4]  */    {"and",NULL,IMw,1},
               {"sub",NULL,IMlw,1},
               {"xor",NULL,IMw,1},
               {"cmp",NULL,IMlw,1},
};


const struct instable x86_disassembler::op82[8] = {
/*  [0]  */    {"addb",NULL,IMlw,0},
               {"orb",NULL,IMlw,0},
               {"adcb",NULL,IMlw,0},
               {"sbbb",NULL,IMlw,0},
/*  [4]  */    {"andb",NULL,IMlw,0},
               {"subb",NULL,IMlw,0},
               {"xorb",NULL,IMlw,0},
               {"cmpb",NULL,IMlw,0},
};

const struct instable x86_disassembler::op83[8] = {
/*  [0]  */    {"add",NULL,IMlw,1},
               {"or",NULL,IMlw,1},
               {"adc",NULL,IMlw,1},
               {"sbb",NULL,IMlw,1},
/*  [4]  */    {"and",NULL,IMlw,1},
               {"sub",NULL,IMlw,1},
               {"xor",NULL,IMlw,1},
               {"cmp",NULL,IMlw,1},
};

const struct instable x86_disassembler::opC0[8] = {
/*  [0]  */    {"rolb",NULL,MvI,0},
               {"rorb",NULL,MvI,0},
               {"rclb",NULL,MvI,0},
               {"rcrb",NULL,MvI,0},
/*  [4]  */    {"shlb",NULL,MvI,0},
               {"shrb",NULL,MvI,0},
               INVALID,
               {"sarb",NULL,MvI,0},
};

const struct instable x86_disassembler::opD0[8] = {
/*  [0]  */    {"rolb",NULL,Mv,0},
               {"rorb",NULL,Mv,0},
               {"rclb",NULL,Mv,0},
               {"rcrb",NULL,Mv,0},
/*  [4]  */    {"shlb",NULL,Mv,0},
               {"shrb",NULL,Mv,0},
               {"salb",NULL,Mv,0},
               {"sarb",NULL,Mv,0},
};

const struct instable x86_disassembler::opC1[8] = {
/*  [0]  */    {"rol",NULL,MvI,1},
               {"ror",NULL,MvI,1},
               {"rcl",NULL,MvI,1},
               {"rcr",NULL,MvI,1},
/*  [4]  */    {"shl",NULL,MvI,1},
               {"shr",NULL,MvI,1},
               {"sal",NULL,MvI,1},
               {"sar",NULL,MvI,1},
};


const struct instable x86_disassembler::opD1[8] = {
/*  [0]  */    {"rol",NULL,Mv,1},
               {"ror",NULL,Mv,1},
               {"rcl",NULL,Mv,1},
               {"rcr",NULL,Mv,1},
/*  [4]  */    {"shl",NULL,Mv,1},
               {"shr",NULL,Mv,1},
               {"sal",NULL,Mv,1},
               {"sar",NULL,Mv,1},
};


const struct instable x86_disassembler::opD2[8] = {
/*  [0]  */    {"rolb",NULL,Mv,0},
               {"rorb",NULL,Mv,0},
               {"rclb",NULL,Mv,0},
               {"rcrb",NULL,Mv,0},
/*  [4]  */    {"shlb",NULL,Mv,0},
               {"shrb",NULL,Mv,0},
               {"salb",NULL,Mv,0},
               {"sarb",NULL,Mv,0},
};

const struct instable x86_disassembler::opD3[8] = {
/*  [0]  */    {"rol",NULL,Mv,1},
               {"ror",NULL,Mv,1},
               {"rcl",NULL,Mv,1},
               {"rcr",NULL,Mv,1},
/*  [4]  */    {"shl",NULL,Mv,1},
               {"shr",NULL,Mv,1},
               {"salb",NULL,Mv,1},
               {"sar",NULL,Mv,1},
};


const struct instable x86_disassembler::opF6[8] = {
/*  [0]  */    {"testb",NULL,IMw,0},
               {"testb",NULL,IMw,0},
               {"notb",NULL,Mw,0},
               {"negb",NULL,Mw,0},
/*  [4]  */    {"mulb",NULL,MA,0},
               {"imulb",NULL,MA,0},
               {"divb",NULL,MA,0},
               {"idivb",NULL,MA,0},
};


const struct instable x86_disassembler::opF7[8] = {
/*  [0]  */    {"test",NULL,IMw,1},
               {"test",NULL,IMw,1},
               {"not",NULL,Mw,1},
               {"neg",NULL,Mw,1},
/*  [4]  */    {"mul",NULL,MA,1},
               {"imul",NULL,MA,1},
               {"div",NULL,MA,1},
               {"idiv",NULL,MA,1},
};


const struct instable x86_disassembler::opFE[8] = {
/*  [0]  */    {"incb",NULL,Mw,0},
               {"decb",NULL,Mw,0},
               INVALID,
               INVALID,
/*  [4]  */    INVALID,
               INVALID,
               INVALID,
               INVALID,
};

const struct instable x86_disassembler::opFF[8] = {
/*  [0]  */    {"inc",NULL,Mw,1},
               {"dec",NULL,Mw,1},
               {"call",NULL,INM,0},
               INVALID, // {"lcall",NULL,INM,0},
/*  [4]  */    {"jmp",NULL,INM,0},
               INVALID, // {"ljmp",NULL,INM,0},
               {"push",NULL,Mem,1},
               INVALID,
};

/* for 287 instructions, which are a mess to decode */
const struct instable x86_disassembler::opFP1n2[8][8] = {
/* bit pattern:    1101 1xxx MODxx xR/Mem */
    {
/*  [0,0] */   {"fadds",NULL,Mem,0},
               {"fmuls",NULL,Mem,0},
               {"fcoms",NULL,Mem,0},
               {"fcomps",NULL,Mem,0},
/*  [0,4] */   {"fsubs",NULL,Mem,0},
               {"fsubrs",NULL,Mem,0},
               {"fdivs",NULL,Mem,0},
               {"fdivrs",NULL,Mem,0},
    },         
    {          
/*  [1,0]  */   {"flds",NULL,Mem,0},
               INVALID,
               {"fsts",NULL,Mem,0},
               {"fstps",NULL,Mem,0},
/*  [1,4]  */  {"fldenv",NULL,Mem,0},
               {"fldcw",NULL,Mem,0},
               {"fnstenv",NULL,Mem,0},
               {"fnstcw",NULL,Mem,0},
    },         
    {          
/*  [2,0]  */  {"fiaddl",NULL,Mem,0},
               {"fimull",NULL,Mem,0},
               {"ficoml",NULL,Mem,0},
               {"ficompl",NULL,Mem,0},
/*  [2,4]  */  {"fisubl",NULL,Mem,0},
               {"fisubrl",NULL,Mem,0},
               {"fidivl",NULL,Mem,0},
               {"fidivrl",NULL,Mem,0},
    },         
    {          
/*  [3,0]  */   {"fildl",NULL,Mem,0},
               INVALID,
               {"fistl",NULL,Mem,0},
               {"fistpl",NULL,Mem,0},
/*  [3,4]  */   INVALID,
               {"fldt",NULL,Mem,0},
               INVALID,
               {"fstpt",NULL,Mem,0},
    },         
    {          
               
/*  [4,0]  */  {"faddl",NULL,Mem,0},
               {"fmull",NULL,Mem,0},
               {"fcoml",NULL,Mem,0},
               {"fcompl",NULL,Mem,0},
               
/*  [4,1]  */  {"fsubl",NULL,Mem,0},
               {"fsubrl",NULL,Mem,0},
               {"fdivl",NULL,Mem,0},
               {"fdivrl",NULL,Mem,0},
               
    },         
    {          
/*  [5,0]  */  {"fldl",NULL,Mem,0},
               INVALID,
               {"fstl",NULL,Mem,0},
               {"fstpl",NULL,Mem,0},
               
/*  [5,4]  */  {"frstor",NULL,Mem,0},
               INVALID,
               {"fnsave",NULL,Mem,0},
               {"fnstsw",NULL,Mem,0},
               
    },         
    {          
/*  [6,0]  */  {"fiadd",NULL,Mem,0},
               {"fimul",NULL,Mem,0},
               {"ficom",NULL,Mem,0},
               {"ficomp",NULL,Mem,0},
               
/*  [6,4]  */  {"fisub",NULL,Mem,0},
               {"fisubr",NULL,Mem,0},
               {"fidiv",NULL,Mem,0},
               {"fidivr",NULL,Mem,0},
               
    },         
    {          
/*  [7,0]  */  {"fild",NULL,Mem,0},
               INVALID,
               {"fist",NULL,Mem,0},
               {"fistp",NULL,Mem,0},
               
/*  [7,4]  */  {"fbld",NULL,Mem,0},
               {"fildll",NULL,Mem,0},
               {"fbstp",NULL,Mem,0},
               {"fistpll",NULL,Mem,0},
    }
};

const struct instable x86_disassembler::opFP3[8][8] = {
/* bit  pattern:    1101 1xxx 11xx xREG */
    {
/*  [0,0]  */  {"fadd",NULL,Float2,0},
               {"fmul",NULL,Float2,0},
               {"fcom",NULL,Float1,0},
               {"fcomp",NULL,Float1,0},
                
/*  [0,4]  */  {"fsub",NULL,Float2,0},
               {"fsubr",NULL,Float2,0},
               {"fdiv",NULL,Float2,0},
               {"fdivr",NULL,Float2,0},
    },
    {
/*  [1,0]  */  {"fld",NULL,Float1,0},
               {"fxch",NULL,Float1,0},
               {"fnop",NULL,GO_ON,0},
               {"fstp",NULL,Float1,0},
                
/*  [1,4]  */  INVALID,
               INVALID,
               INVALID,
               INVALID,
    },
    {
/*  [2,0]  */  INVALID,
               INVALID,
               INVALID,
               INVALID,
                
/*  [2,4]  */  INVALID,
               {"fucompp",NULL,GO_ON,0},
               INVALID,
               INVALID,
    },         
    {          
/*  [3,0]  */  INVALID,
               INVALID,
               INVALID,
               INVALID,
               
/*  [3,4]  */  INVALID,
               INVALID,
               INVALID,
               INVALID,
    },         
    {          
/*  [4,0]  */  {"fadd",NULL,Float2,0},
               {"fmul",NULL,Float2,0},
               {"fcom",NULL,Float1,0},
               {"fcomp",NULL,Float1,0},
               
/*  [4,4]  */  {"fsub",NULL,Float2,0},
               {"fsubr",NULL,Float2,0},
               {"fdiv",NULL,Float2,0},
               {"fdivr",NULL,Float2,0},
    },         
    {          
/*  [5,0]  */  {"ffree",NULL,Float1,0},
               {"fxch",NULL,Float1,0},
               {"fst",NULL,Float1,0},
               {"fstp",NULL,Float1,0},
               
/*  [5,4]  */  {"fucom",NULL,Float1,0},
               {"fucomp",NULL,Float1,0},
               INVALID,
               INVALID,
    },         
    {          
/*  [6,0]  */  {"faddp",NULL,Float2,0},
               {"fmulp",NULL,Float2,0},
               {"fcomp",NULL,Float1,0},
               {"fcompp",NULL,GO_ON,0},
               
/*  [6,4]  */  {"fsubp",NULL,Float2,0},
               {"fsubrp",NULL,Float2,0},
               {"fdivp",NULL,Float2,0},
               {"fdivrp",NULL,Float2,0},
    },         
    {          
/*  [7,0]  */  {"ffree",NULL,Float1,0},
               {"fxch",NULL,Float1,0},
               {"fstp",NULL,Float1,0},
               {"fstp",NULL,Float1,0},
               
/*  [7,4]  */  {"fstsw",NULL,Mem,0},
               {"fucomip",NULL,Float1,0},
               {"fcomip",NULL,Float1,0},
               INVALID,
    }
};

const struct instable x86_disassembler::opFP4[4][8] = {
/* bit pattern:    1101 1001 111x xxxx */
    {
/*  [0,0]  */  {"fchs",NULL,GO_ON,0},
               {"fabs",NULL,GO_ON,0},
               INVALID,
               INVALID,
               
/*  [0,4]  */  {"ftst",NULL,GO_ON,0},
               {"fxam",NULL,GO_ON,0},
               {"ftstp",NULL,GO_ON,0},
               INVALID,
    },         
    {          
/*  [1,0]  */  {"fld1",NULL,GO_ON,0},
               {"fldl2t",NULL,GO_ON,0},
               {"fldl2e",NULL,GO_ON,0},
               {"fldpi",NULL,GO_ON,0},
               
/*  [1,4]  */  {"fldlg2",NULL,GO_ON,0},
               {"fldln2",NULL,GO_ON,0},
               {"fldz",NULL,GO_ON,0},
               INVALID,
    },         
    {          
               
/*  [2,0]  */  {"f2xm1",NULL,GO_ON,0},
               {"fyl2x",NULL,GO_ON,0},
               {"fptan",NULL,GO_ON,0},
               {"fpatan",NULL,GO_ON,0},
               
/*  [2,4]  */  {"fxtract",NULL,GO_ON,0},
               {"fprem1",NULL,GO_ON,0},
               {"fdecstp",NULL,GO_ON,0},
               {"fincstp",NULL,GO_ON,0},
               
    },         
    {          
/*  [3,0]  */  {"fprem",NULL,GO_ON,0},
               {"fyl2xp1",NULL,GO_ON,0},
               {"fsqrt",NULL,GO_ON,0},
               {"fsincos",NULL,GO_ON,0},
               
/*  [3,4]  */  {"frndint",NULL,GO_ON,0},
               {"fscale",NULL,GO_ON,0},
               {"fsin",NULL,GO_ON,0},
               {"fcos",NULL,GO_ON,0},
    }
};

const struct instable x86_disassembler::opFP5[8] = {
/* bit pattern:    1101 1011 111x xxxx */
/*  [0]  */    {"feni",NULL,GO_ON,0},
               {"fdisi",NULL,GO_ON,0},
               {"fnclex",NULL,GO_ON,0},
               {"fninit",NULL,GO_ON,0},
                
/*  [4]  */    {"fsetpm",NULL,GO_ON,0},
               {"frstpm",NULL,GO_ON,0},
               INVALID,
               INVALID,
};

const struct instable x86_disassembler::opFP6[8] = {
/* bit pattern:    1101 1011 11yy yxxx */
/*  [00]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [04]  */   INVALID,
               {"fucomi",NULL,Float1,0},
               {"fcomi",NULL,Float1,0},
                INVALID,
};

const struct instable x86_disassembler::opFP7[8] = {
/* bit pattern:    1101 1010 11yy yxxx */
/*  [00]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
/*  [04]  */   INVALID,
               INVALID,
               INVALID,
               INVALID,
};

#endif // !defined(PRODUCT) || ENABLE_TTY_TRACE

#endif // ENABLE_COMPILER
