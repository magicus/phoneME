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

// The Assembler class provides minimal functionality to generate
// Wireless MMX instruction. All functions representing machine instructions
// generate exactly one instruction.


#include "incls/_precompiled.incl"

#if !ENABLE_THUMB_COMPILER 

#if !defined(PRODUCT) || USE_COMPILER_DISASSEMBLER

#include "incls/_Disassembler_wmmx.cpp.incl"

#if ENABLE_XSCALE_WMMX_INSTRUCTIONS

const char* Disassembler::wmmx_wreg_name(Assembler::WMMXRegister reg) {
  static const char* reg_names[Assembler::number_of_registers] = {
    "wR0", "wR1", "wR2", "wR3", "wR4", "wR5", "wR6", "wR7",
    "wR8", "wR9", "wR10", "wR11", "wR12", "wR13", "wR14", "wR15"
  };
  GUARANTEE(Assembler::wR0 <= reg && 
            reg < (Assembler::WMMXRegister)(Assembler::number_of_registers),
            "illegal register");
  return reg_names[reg];
}

const char* Disassembler::wmmx_wcgreg_name(Assembler::WCGRegister reg) {
  static const char* reg_names[Assembler::number_of_registers] = {
    "wCID", "wCon", "wCSSF", "wCASF", 
    "reserved", "reserved", "reserved", "reserved",
    "wCGR0", "wCGR1", "wCGR2", "wCGR3", 
    "reserved", "reserved", "reserved", "reserved"
  };
  GUARANTEE(Assembler::wCID <= reg && 
            reg < (Assembler::WCGRegister)(Assembler::number_of_registers),
            "illegal register");
  return reg_names[reg];
}

const char* Disassembler::wmmx_align_name(int ww) {
  static const char* align_names[] = {
    "b", "h", "w", "d"
  };
  GUARANTEE(0 <= ww && ww < 4,
            "illegal data alignment");
  return align_names[ww];
}


// To support Xscale WMMX instructions
void Disassembler::emit_wmmx_instruction(int instr) {
  // decode instruction and print on stream
  const Assembler::Condition cond = 
                   Assembler::as_condition(instr >> 28 & 0xf);
  int op = (instr >> 25) & 0x7;
  switch (op) {    
    case  6: {
      
      int cp_num = (instr >> 8) & 0xF;
      int Opcode2 = (instr >> 4) & 0xF;
      int punwl = (instr >> 20) & 0x1F;
      int wRd = instr & 0xF;
      int wRn = instr & 0xF;
      int RdHi = (instr >> 16) & 0xF;
      int RdLo = (instr >> 12) & 0xF;
      
      if ( punwl == 4 && cp_num == 0 && Opcode2 == 0 ) {
        // TMCRR instruction
        stream()->print("tmcrr%s\t%s, %s, %s", cond_name(cond),
                                wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                                reg_name((Assembler::Register)RdLo), 
                                reg_name((Assembler::Register)RdHi) );
        
      } else {
        if ( punwl == 5 && cp_num == 0 && Opcode2 == 0 ) {
        // TMRRC instruction
          stream()->print("tmrrc%s\t%s, %s, %s", cond_name(cond),
                          reg_name((Assembler::Register)RdLo), 
                          reg_name((Assembler::Register)RdHi),
                          wmmx_wreg_name((Assembler::WMMXRegister)wRn) );
        } else {
          // Load Store Instructions
          int Rn = (instr >> 16) & 0xF;
          int wRd = (instr >> 12) & 0xF;
          int offset_8 = instr & 0xFF;
          const bool p = bit(instr, 24);
          const bool u = bit(instr, 23);
          const bool n = bit(instr, 22);
          const bool w = bit(instr, 21);
          const bool l = bit(instr, 20);
          const bool m = bit(instr, 8);
          if (!n && !m) stream()->print(l ? "wldrb" : "wstrb");
          if (n && !m) stream()->print(l ? "wldrh" : "wstrh");
          if (!n && m) stream()->print(l ? "wldrw" : "wstrw");
          if (n && m) stream()->print(l ? "wldrd" : "wstrd");
          
          if ((instr >> 28 & 0xF) == 0xF) {
            stream()->print("\t%s, ", 
                            wmmx_wcgreg_name((Assembler::WCGRegister)wRd));
          } else {          
            stream()->print("%s\t%s, ", cond_name(cond),
                              wmmx_wreg_name((Assembler::WMMXRegister)wRd ));
          }
          
          if (p && !w) {
            stream()->print("[%s, #", reg_name((Assembler::Register)Rn));
            stream()->print(u ? "+" : "-");
            stream()->print("%d", offset_8 * 4);
            stream()->print("]");
          }
          if (p && w) {
            stream()->print("[%s,#", reg_name((Assembler::Register)Rn));
            stream()->print(u ? "+" : "-");
            stream()->print("%d", offset_8 * 4);
            stream()->print("]!");
          }
          if (!p && w) {
            stream()->print("[%s],#", reg_name((Assembler::Register)Rn));
            stream()->print(u ? "+" : "-");
            stream()->print("%d", offset_8 * 4);
          }
          if (!p && !w) {
            stream()->print("[%s],", reg_name((Assembler::Register)Rn));
            stream()->print("{");
            stream()->print("%d", offset_8);
            stream()->print("}");
          }
        }
      }
      break;
    }
    case  7: {
      int cp_num = (instr >> 8) & 0xF;
      if (!bit(instr, 4)) {
        // Coprocessor Data (CDP) Instructions
        int Opcode1 = (instr >> 20) & 0xF;
        int Opcode2 = (instr >> 5) & 0x7;
        int wRm = instr & 0xF;
        int wRn = (instr >> 16) & 0xF;
        int wRd = (instr >> 12) & 0xF;
        if ( cp_num == 0 ) {
          // WMMX Coprocessor 0 CDP instructions
          switch (Opcode2) {
            case  0: {
              // CDP Misc Type Instructions
              const char* opcode_names[] = {
                 "wor", "wxor", "wand", "wandn", "wavg2" };
              if (Opcode1 < 0x4) {
                  stream()->print("%s", opcode_names[Opcode1]);
              } else {
                // Instruction wavg2, Opcode1 = 1 h 0 r
                if ( bit(Opcode1, 3) ) {
                  if ( !bit(Opcode1, 1) ) {
                    const bool h = bit( Opcode1, 2 );
                    const bool r = bit( Opcode1, 0 );
                    stream()->print("%s", opcode_names[4]);
                    stream()->print(h ? "h" : "b");
                    if (r) {
                      stream()->print("r");
                    }
                  } else {
                    // reserved resource
                    // SHOULD_NOT_REACH_HERE();
                    stream()->print("DCD:%#X",instr);
                  }
                } else {
                  // reserved resource
                  // SHOULD_NOT_REACH_HERE();
                  stream()->print("DCD:%#X",instr);
                }
              }
              stream()->print("%s\t%s, %s, %s",cond_name(cond),
                              wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                              wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                              wmmx_wreg_name((Assembler::WMMXRegister)wRm));
              break;
            }
            case  1: {
              // CDP Align Type Instructions
              if (!bit(Opcode1, 3)) {
                // WALIGNI
                int Imm3 = Opcode1 & 0x7;
                stream()->print("waligni%s\t%s, %s, %s, #%d",cond_name(cond),
                                wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                                wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                                wmmx_wreg_name((Assembler::WMMXRegister)wRm),
                                Imm3 );

              } else {
                if (!bit(Opcode1, 2)) {
                  // WALIGNR
                  int vv = Opcode1 & 0x3;
                  stream()->print("walignr%d%s\t%s, %s, %s", 
                              vv, cond_name(cond),
                              wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                              wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                              wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
                } else {
                  // reserved resource
                  // SHOULD_NOT_REACH_HERE();
                  stream()->print("DCD:%#X",instr);
                }
              }
              break;
            }
            case  2: {
               // CDP Cp0_Shift Type Instructions
              const char* opcode_names[] = {
                   "wsra", "wsll", "wsrl", "wror" };
              int opcode = Opcode1 & 0x3;
              int ww = ( Opcode1 >> 2 ) & 0x3;
              stream()->print("%s%s%s\t%s, %s, %s", opcode_names[opcode],
                              wmmx_align_name(ww),
                              cond_name(cond),
                              wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                              wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                              wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
              break;
            }
            case  3: {
              // CDP Compare Type Instructions
              if ( !bit(Opcode1, 0) ) {
                if ( !bit(Opcode1, 1) ) {
                  // WCMPEQ
                  int ww = ( Opcode1 >> 2 ) & 0x3;
                  stream()->print("wcmpeq%s%s\t%s, %s, %s", 
                           wmmx_align_name(ww),
                           cond_name(cond),
                           wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                           wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                           wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
                } else {
                  // reserved resource
                  // SHOULD_NOT_REACH_HERE();
                  stream()->print("DCD:%#X",instr);
                }
              } else {
                // WCMPGT
                int ww = ( Opcode1 >> 2 ) & 0x3;
                const bool s = bit( Opcode1, 1 );
                stream()->print("wcmpgt");
                stream()->print(s ? "s" : "u");
                stream()->print("%s%s\t%s, %s, %s", 
                        wmmx_align_name(ww),
                        cond_name(cond),
                        wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRm));
              }
              break;
            }
            case  4: {
              // CDP PACK Type Instructions
              int ww = ( (Opcode1 >> 2) & 0x3 );
              int ss = ( Opcode1 & 0x3 );
              if ( ss == 0 || ss == 2 ) {
                // reserved resource
                // SHOULD_NOT_REACH_HERE();
                stream()->print("DCD:%#X",instr);
              } else {
                stream()->print( "wpack%s", wmmx_align_name(ww) );
                if ( ss == 1 ) {
                  stream()->print( "us");
                } else {
                  stream()->print( "ss");
                }
                stream()->print("%s\t%s, %s, %s", 
                        cond_name(cond),
                        wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRm));
              }
              break;
            }
            case  6: {
              // CDP UNPACK Type Instructins
              int ss = (Opcode1 & 0x3);
              int ww = (( Opcode1 >> 2) & 0x3);
              if(ss != 0x3) {
                if(ss == 0x1) {
                  // WUNPCKIH
                  stream()->print("wunpckih%s", wmmx_align_name(ww));
                  stream()->print("%s\t%s, %s, %s", 
                        cond_name(cond),
                        wmmx_wreg_name( (Assembler::WMMXRegister)wRd ), 
                        wmmx_wreg_name( (Assembler::WMMXRegister)wRn ), 
                        wmmx_wreg_name( (Assembler::WMMXRegister)wRm ) );
                }else{
                  // WUNPCKEH
                  const bool s = bit(Opcode1, 1);
                  stream()->print("wunpckeh");
                  stream()->print(s ? "s" : "u");
                  stream()->print("%s", wmmx_align_name(ww));
                  stream()->print("%s\t%s, %s", 
                        cond_name(cond),
                        wmmx_wreg_name( (Assembler::WMMXRegister)wRd ), 
                        wmmx_wreg_name( (Assembler::WMMXRegister)wRn ));
                }
              }else{
                //reserved opcode1 --11
                stream()->print("DCD:%#X",instr);
              }
              break;
            }
            case  7: {
              // CDP UNPACK Type Instructins
              int ss = ( Opcode1 & 0x3 );
              int ww = ( ( Opcode1 >> 2) & 0x3 );
              if(ss != 0x3) {
                if(ss == 0x1) {
                  // WUNPCKIL
                  stream()->print("wunpckil%s", wmmx_align_name(ww));
                  stream()->print("%s\t%s, %s, %s", 
                        cond_name(cond),
                        wmmx_wreg_name( (Assembler::WMMXRegister)wRd ), 
                        wmmx_wreg_name( (Assembler::WMMXRegister)wRn ), 
                        wmmx_wreg_name( (Assembler::WMMXRegister)wRm ) );
                }else{
                  // WUNPCKEL
                  const bool s = bit( Opcode1, 1 );
                  stream()->print( "wunpckel");
                  stream()->print( s ? "s" : "u" );
                  stream()->print( "%s", wmmx_align_name(ww) );
                  stream()->print("%s\t%s, %s", 
                        cond_name(cond),
                        wmmx_wreg_name( (Assembler::WMMXRegister)wRd ), 
                        wmmx_wreg_name( (Assembler::WMMXRegister)wRn ));
                }
              }else{
                //reserved opcode1 --11
                stream()->print("DCD:%#X",instr);
              }
              break;
            }
            default: 
              // SHOULD_NOT_REACH_HERE();
              stream()->print("DCD:%#X",instr);
            }
          } else {
            // WMMX Coprocessor 1 CDP instructions
            switch (Opcode2) {
              case  0: {
                // CDP Multiply Type Instructions
                const bool s = bit(Opcode1, 1);
                const bool bit0 = bit(Opcode1, 0);
                int first_two_bits = ((Opcode1 >> 2) & 0x3);
                if (first_two_bits == 0) {
                  //WMUL
                  stream()->print("wmul");
                  stream()->print(s ? "s" : "u");
                  stream()->print(bit0 ? "m" : "l");
                  stream()->print("%s\t%s, %s, %s", 
                        cond_name(cond),
                        wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
                } else {
                  if ( first_two_bits == 1 ) {
                    //WMAC
                    stream()->print("wmac");
                    stream()->print(s ? "s" : "u");
                    stream()->print(bit0 ? "z" : "");
                    stream()->print("%s\t%s, %s, %s", 
                          cond_name(cond),
                          wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                          wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                          wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
                  } else {
                    if (first_two_bits == 2) {
                      if (!bit(Opcode1, 0)) {
                      //WMADD
                      stream()->print("wmadd");
                      stream()->print(s ? "s" : "u");
                      stream()->print("%s\t%s, %s, %s", 
                          cond_name(cond),
                          wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                          wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                          wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
                      } else {
                        // reserved resource
                        // SHOULD_NOT_REACH_HERE();
                        stream()->print("DCD:%#X",instr);
                      }
                    } else {
                      // reserved resource
                      // SHOULD_NOT_REACH_HERE();
                      stream()->print("DCD:%#X",instr);
                    }
                  }
                }
                break;
              }
              case  1: {
                // CDP Difference Type Instructions
                const bool h = bit(Opcode1, 2);
                const bool z = bit(Opcode1, 0);
                if (!bit(Opcode1, 3) && !bit(Opcode1, 1)) {
                //WSAD
                stream()->print("wsad");
                stream()->print(h ? "h" : "b");
                stream()->print(z ? "z" : "");
                stream()->print("%s\t%s, %s, %s", 
                      cond_name(cond),
                      wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                      wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                      wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
                } else {
                  // reserved resource
                  // SHOULD_NOT_REACH_HERE();
                  stream()->print("DCD:%#X",instr);
                }
                break;
              }
              case  2: {
                // CDP Cp1_Shift Type Instructions
                const char* opcode_names[] = {
                   "wsra", "wsll", "wsrl", "wror"};
                int opcode = Opcode1 & 0x3;
                int ww = ( Opcode1 >> 2 ) & 0x3;
                stream()->print("%s%sg%s\t%s, %s, %s", opcode_names[opcode],
                             wmmx_align_name(ww),
                             cond_name(cond),
                             wmmx_wreg_name( (Assembler::WMMXRegister)wRd ), 
                             wmmx_wreg_name( (Assembler::WMMXRegister)wRn ), 
                             wmmx_wcgreg_name( (Assembler::WCGRegister)wRm )    );
                break;
              }
              case  3: {
                // CDP Max/Min Type Instructions
                const bool s = bit(Opcode1, 1);
                int ww = ( Opcode1 >> 2 ) & 0x3;
                if ( !bit(Opcode1, 0) ) {
                  stream()->print("wmax");
                  stream()->print(s ? "s" : "u");
                  stream()->print("%s%s\t%s, %s, %s", 
                        wmmx_align_name(ww),
                        cond_name(cond),
                        wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
                } else {
                  stream()->print("wmin");
                  stream()->print(s ? "s" : "u");
                  stream()->print("%s%s\t%s, %s, %s", 
                        wmmx_align_name(ww),
                        cond_name(cond),
                        wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
                }
                break;
              }
              case  4: {
                // CDP Arithmetic Type Instruction - WADD
                int ww = ( (Opcode1 >> 2) & 0x3 );
                int ss = ( Opcode1 & 0x3 );
                stream()->print("wadd%s", wmmx_align_name(ww));
                if (ss == 1) {
                  stream()->print("us");
                } else {
                  stream()->print("ss");
                }
                stream()->print("%s\t%s, %s, %s", 
                        cond_name(cond),
                        wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
                break;
              }
              case  5: {
                // CDP Arithmetic Type Instruction - WSUB
                int ww = ( (Opcode1 >> 2) & 0x3 );
                int ss = ( Opcode1 & 0x3 );
                stream()->print("wsub%s", wmmx_align_name(ww));
                if ( ss == 1 ) {
                  stream()->print("us");
                } else {
                  stream()->print("ss");
                }
                stream()->print("%s\t%s, %s, %s", 
                        cond_name(cond),
                        wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
                break;
              }
              case  6: {
                // CDP Arithmetic Type Instruction - WACC
                int ww = ( (Opcode1 >> 2) & 0x3 );
                int last_two_bits = ( Opcode1 & 0x3 );
                if (last_two_bits == 0) { 
                  stream()->print("wacc%s", wmmx_align_name(ww));
                  stream()->print("%s\t%s, %s, %s", 
                        cond_name(cond),
                        wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                        wmmx_wreg_name((Assembler::WMMXRegister)wRm) );
                } else {
                  // reserved resource
                  // SHOULD_NOT_REACH_HERE();
                  stream()->print("DCD:%#X",instr);
                }  
                break;
              }
              case  7: {
                // CDP Shuffle Type Instructions
                int Imm8 = ((Opcode1 & 0xF)<< 4) | (instr & 0xF) ;
                stream()->print("wshufh%s\t%s, %s, #%d",cond_name(cond),
                                wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                                wmmx_wreg_name((Assembler::WMMXRegister)wRn), 
                                Imm8);
                break;
              }
              default: 
                // SHOULD_NOT_REACH_HERE();
                stream()->print("DCD:%#X",instr);
            }
          }
        } else { 
          // Transfers to & from Coprocessor Register (MCR, MRC)
          int bit20 = ( instr >> 20 ) & 0x1;
          if( bit20 == 0x0 ){  
          int Opcode1 = ( instr >> 21 ) & 0x3 ;
          const char *opcode_names[] = { "tmcr", "tmia", "tbcst", "tinsr" };
          stream()->print("%s",opcode_names[Opcode1]);
          
          //TMIA
          if( Opcode1 == 1) {
        
          //this instrcution has different format
          int Opcode2 = ( instr >> 16  ) & 0xF ;
          int Rm = instr & 0xF;
          int wRd = (instr >> 5) & 0xF;
          int Rs = (instr >> 12) & 0xF;
          int hh = (Opcode2 >> 2) & 0x3; 

          if(hh != 0x1) {
            switch (hh) {
              case 2: {
                stream()->print("ph");
                break;
              }
              case 3: {
                int ll = Opcode2 & 0x3;
                const bool x = bit(ll, 0);
                stream()->print(x ? "t" : "b");
                const bool y = bit(ll, 1);
                stream()->print(y ? "t" : "b");
                break;
              }
              default: ;
            }
            stream()->print("%s\t%s, %s, %s",
                    cond_name(cond), 
                    wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                    reg_name((Assembler::Register)Rm), 
                    reg_name((Assembler::Register)Rs) );
          } else {
            stream()->print("DCD:%#X", instr);
          }
        } else {
          int cp_num = ( instr >> 8 ) & 0xF ;
          int Opcode2 = ( instr >> 5  ) & 0x7 ;
          int Rn = (instr >> 12) & 0xF;
          int wRd = (instr >> 16) & 0xF;
          int wRm = instr & 0xF;
          int ww = ( Opcode2 >> 1 ) & 0x3 ;
          bool reserved = false;
          // Catch the reserved opcode1 opcode2 cp_num wRm.
          if( cp_num == 0x1 ) {
            if(bit(Opcode1, 2) ||
               bit(Opcode1, 1) ||
               bit(Opcode1, 0) ||
               bit(Opcode2, 2) ||
               bit(Opcode2, 1) ||
               bit(Opcode2, 0) ||
               bit(wRm, 3) ||
               bit(wRm, 2) ||
               bit(wRm, 1) || bit(wRm, 0)) {
              reserved = true;
            }
          } else if ( cp_num == 0x0 ) {
            if( ( Opcode1 == 0x2 && bit( Opcode2, 0 ) ) ||
                ( Opcode1 == 0x3 && bit( Opcode2, 0 ) ) ||
                 bit(Opcode1, 2)) {
              reserved = true;
          } 
        } 
        if( ! reserved ){
          switch( Opcode1 ) {
            //TMCR
            case 0: {
              stream()->print("%s\t%s, %s", cond_name(cond),
                      wmmx_wcgreg_name((Assembler::WCGRegister)wRd), 
                      reg_name((Assembler::Register)Rn));
              break;
            }
            //TBCST
            case 2: {
              stream()->print("%s%s\t%s, %s",
                   wmmx_align_name(ww),
                   cond_name(cond),
                   wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                   reg_name((Assembler::Register)Rn));
              break;
            }
            //TINSR
            case 3: {
              int offset = wRm & 0x7 ;
              stream()->print("%s%s\t%s, %s, #%d", 
                        wmmx_align_name(ww),
                        cond_name(cond),
                        wmmx_wreg_name((Assembler::WMMXRegister)wRd), 
                        reg_name((Assembler::Register)Rn), 
                        offset);
              break;
            } 
            default: 
              // SHOULD_NOT_REACH_HERE();
              stream()->print("DCD:%#X",instr);
          }
        } else {
          // SHOULD_NOT_REACH_HERE();
          stream()->print("DCD:%#X",instr);
        } 
      } 
    }else{
      //Transfers from Coprocessor Register
      int Opcode1 = ( instr >> 21) & 0x7;
      int Opcode2 = ( instr >> 5) & 0x7;
      int wRm = instr & 0xF;
      int cp_num = ( instr >> 8 ) & 0xF ;
      int ww = ( Opcode1 >> 1 ) & 0x3;
      bool reserved = false;
        
      // Catch the reserved opcode1 opcode2 cp_num wRm.
      if(cp_num == 0x1 ){
        if( ( Opcode2 == 0 && ( bit( Opcode1, 0) || 
              bit( Opcode1, 1) || bit( Opcode1, 1) ) ) ||
            ( Opcode2 == 0 && Opcode1 ==0 && ( bit( wRm, 0) || 
              bit( wRm, 1) || bit( wRm, 2) || bit( wRm, 3) ) ) ||
            ( Opcode2 == 1 && bit ( Opcode1, 0) ) ||
            ( Opcode2 == 2 && bit ( Opcode1, 0) ) ||
            ( Opcode2 == 3 && bit ( Opcode1, 0) ) ||
            bit ( Opcode2, 2)) {
          reserved = true;
        }
      }else if( cp_num == 0x0 ){
        if( ( Opcode2 == 0x1 && bit( Opcode1, 0)  ) ||
            bit( Opcode2, 2) ||
            ( bit( Opcode2, 1) && Opcode2 != 3)
               ) {
          reserved = true;
        } 
      }
      if ( ! reserved ) { 
        switch( Opcode2 ) {
          //TMRC
          case  0: {
            int wCx = ( instr >> 16 ) & 0xF;
            int Rd = ( instr >> 12 ) & 0xF;
            stream()->print("tmrc%s\t%s, %s",
              cond_name(cond),
              reg_name((Assembler::Register)Rd), 
              wmmx_wcgreg_name((Assembler::WCGRegister)wCx));
            break;
          }
          //TMOVMSK and TANDC
          case  1: {
            int wRn = ( instr >> 16 ) & 0xF;
            int Rd = ( instr >> 12 ) & 0xF;
            if( cp_num == 0x0 ) {
              stream()->print("tmovmsk%s%s\t%s, %s",
              wmmx_align_name(ww),
              cond_name(cond),
              reg_name((Assembler::Register)Rd), 
                     wmmx_wreg_name((Assembler::WMMXRegister)wRn));
            } else if (cp_num == 0x1) {
              stream()->print("tandc%s%s\tr15",
                        wmmx_align_name(ww),
                        cond_name(cond));  
            }
            break;
          }
          //TORC
          case  2: {
            stream()->print("torc%s%s\tr15",
                      wmmx_align_name(ww),
                      cond_name(cond));  
            break;
          }
          //TEXTRC and TEXTRM
          case  3: {
            int wRn = ( instr >> 16 ) & 0xF;
            int Rd = ( instr >> 12 ) & 0xF;
            int offset = wRm & 0x7;
            if( cp_num == 0x1 ) {
              stream()->print("textrc%s%s\tr15, #%d",
                        wmmx_align_name(ww),
                        cond_name(cond),
                        offset);
            }else if(cp_num == 0x0) {
              stream()->print("textrm%s%s%s\t%s, %s, #%d",
                        (bit(wRm,3)?"s":"u"),
                        wmmx_align_name(ww),
                        cond_name(cond),
                        reg_name( (Assembler::Register)Rd ), 
                        wmmx_wreg_name( (Assembler::WMMXRegister)wRn ),
                        offset);
            }
            break;
          }
          default:
            // SHOULD_NOT_REACH_HERE();
            stream()->print("DCD:%#X",instr);
            }
          } else {
            // SHOULD_NOT_REACH_HERE();
            stream()->print("DCD:%#X",instr);
          }
        }//End of Transfers from Coprocessor Register  
      }
      break;
    }
    default: 
      // SHOULD_NOT_REACH_HERE();
      stream()->print("DCD:%#X",instr);
  }
}
#endif // ENABLE_XSCALE_WMMX_INSTRUCTIONS

#endif // !defined(PRODUCT) || USE_COMPILER_DISASSEMBLER

#endif /*#if !ENABLE_THUMB_COMPILER */
