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

 
  // The wireless MMX SIMD Data Registers in CP0
  enum WMMXRegister {
    wR0,   wR1,   wR2,   wR3, 
    wR4,   wR5,   wR6,   wR7, 
    wR8,   wR9,   wR10,  wR11, 
    wR12,  wR13,  wR14,  wR15
  };

  // The wireless MMX coprocessor control&status register in CP1
  enum WCGRegister {
    wCID=0,
    wCon=1,
    wCSSF=2,
    wCASF=3,
    // 4-7 is reserved
    wCGR0=8, 
    wCGR1=9,
    wCGR2=10, 
    wCGR3=11
    // 12-15 is reserved
  };

// Coprocessor Data (CDP) Instructions, type = Misc: cp_num = p0, opcode2 = 0, 
  // Opcode1 = 0 
  void wor(WMMXRegister wRd, WMMXRegister wRn, 
           WMMXRegister wRm, Condition cond = al) {
      cdp(p0, 0, (CRegister)wRd, (CRegister)wRn, (CRegister)wRm, 0, cond);
  }
  // Opcode1 = 1 
  void wxor(WMMXRegister wRd, WMMXRegister wRn, 
            WMMXRegister wRm, Condition cond = al) {
      cdp(p0, 1, (CRegister)wRd, (CRegister)wRn, (CRegister)wRm, 0, cond);
  }
  // Opcode1 = 2 
  void wand(WMMXRegister wRd, WMMXRegister wRn, 
            WMMXRegister wRm, Condition cond = al) {
      cdp(p0, 2, (CRegister)wRd, (CRegister)wRn, (CRegister)wRm, 0, cond);
  }
  // Opcode1 = 3 
  void wandn(WMMXRegister wRd, WMMXRegister wRn, 
             WMMXRegister wRm, Condition cond = al) {
      cdp(p0, 3, (CRegister)wRd, (CRegister)wRn, (CRegister)wRm, 0, cond);
  }
  // Opcode1 = 1 h 0 r
  #define F(mnemonic, h, r)                                         \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,                 \
                           WMMXRegister wRm, Condition cond = al) { \
    cdp(p0, ( 0x8 | (h*0x4) | r ), (CRegister)wRd, (CRegister)wRn,  \
         (CRegister)wRm, 0, cond);                                  \
  }                                                           
  F(wavg2b, 0, 0)
  F(wavg2h, 1, 0)
  F(wavg2br, 0, 1)
  F(wavg2hr, 1, 1)
  #undef F

// Coprocessor Data (CDP) Instructions, type = Align:cp_num = p0,opcode2 = 001
  // Opcode1 = 0 v v v
  void waligni(WMMXRegister wRd, WMMXRegister wRn, 
               WMMXRegister wRm, int Imm3, Condition cond = al) {
      Imm3 = Imm3 & 0x7;
      cdp(p0, Imm3, (CRegister)wRd, (CRegister)wRn, (CRegister)wRm, 1, cond);
  }
  // Opcode1 = 1 0 v v
  #define F(mnemonic, vv)                                             \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,                   \
                WMMXRegister wRm, Condition cond = al) {              \
    cdp(p0, (0x8 | vv), (CRegister)wRd, (CRegister)wRn,               \
         (CRegister)wRm, 1, cond);                                    \
  }                                                           
  F(walignr0, 0)
  F(walignr1, 1)
  F(walignr2, 2)
  F(walignr3, 3)
  #undef F

// Coprocessor Data (CDP) Instructions, type = Shift: opcode2 = 010 
  // Opcode1 = w w 0 0, cp_num = p0 & p1
  #define F(mnemonic, ww)                                             \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,                   \
                           WMMXRegister wRm, Condition cond = al) {   \
    cdp(p0, 0x4*ww, (CRegister)wRd, (CRegister)wRn,                   \
         (CRegister)wRm, 2, cond);                                    \
  }                                                           
  F(wsrah, 1)
  F(wsraw, 2)  
  F(wsrad, 3)  
  #undef F
  #define F(mnemonic, ww)                                             \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,                   \
                WCGRegister wCGRn, Condition cond = al) {             \
    cdp(p1, 0x4*ww, (CRegister)wRd,                                   \
         (CRegister)wRn, (CRegister)wCGRn, 2, cond);                  \
  }                                                           
  F(wsrahg, 1)
  F(wsrawg, 2)  
  F(wsradg, 3)  
  #undef F
  // Opcode1 = w w 0 1, cp_num = p0 & p1
  #define F(mnemonic, ww)                                            \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,                  \
                WMMXRegister wRm, Condition cond = al) {             \
    cdp(p0, ((0x4*ww)|1), (CRegister)wRd, (CRegister)wRn,            \
         (CRegister)wRm, 2, cond);                                   \
  }                                                           
  F(wsllh, 1)
  F(wsllw, 2)  
  F(wslld, 3)  
  #undef F
  #define F(mnemonic, ww)                                           \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,                 \
                WCGRegister wCGRn, Condition cond = al) {           \
    cdp(p1, ((0x4*ww)|1), (CRegister)wRd, (CRegister)wRn,           \
         (CRegister)wCGRn, 2, cond);                                \
  }                                                           
  F(wsllhg, 1)
  F(wsllwg, 2)  
  F(wslldg, 3)  
  #undef F
  // Opcode1 = w w 1 0, cp_num = p0 & p1
  #define F(mnemonic, ww)                                      \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,            \
                WMMXRegister wRm, Condition cond = al) {       \
    cdp(p0, ((0x4*ww) | 2), (CRegister)wRd, (CRegister)wRn,    \
         (CRegister)wRm, 2, cond);                             \
  }                                                           
  F(wsrlh, 1)
  F(wsrlw, 2)  
  F(wsrld, 3)  
  #undef F
  #define F(mnemonic, ww)                                      \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,            \
                WCGRegister wCGRn, Condition cond = al) {      \
    cdp(p1, ((0x4*ww) | 2), (CRegister)wRd, (CRegister)wRn,    \
         (CRegister)wCGRn, 2, cond);                           \
  }                                                           
  F(wsrlhg, 1)
  F(wsrlwg, 2)  
  F(wsrldg, 3)  
  #undef F
  // Opcode1 = w w 1 1, cp_num = p0 & p1
  #define F(mnemonic, ww)                                     \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,           \
                WMMXRegister wRm, Condition cond = al) {      \
    cdp(p0, ((0x4*ww) | 3), (CRegister)wRd, (CRegister)wRn,   \
         (CRegister)wRm, 2, cond);                            \
  }                                                           
  F(wrorh, 1)
  F(wrorw, 2)  
  F(wrord, 3)  
  #undef F
  #define F(mnemonic, ww)                                     \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,           \
                WCGRegister wCGRn, Condition cond = al) {     \
    cdp(p1, ((0x4*ww) | 3), (CRegister)wRd, (CRegister)wRn,   \
         (CRegister)wCGRn, 2, cond);                          \
  }                                                           
  F(wrorhg, 1)
  F(wrorwg, 2)  
  F(wrordg, 3)  
  #undef F

// Coprocessor Data (CDP) Instructions 
// type = Compare: cp_num = p0, opcode2 = 011
  // Opcode1 = w w 0 0 
  #define F(mnemonic, ww)                                     \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,           \
                WMMXRegister wRm, Condition cond = al) {      \
    cdp(p0, 0x4*ww, (CRegister)wRd, (CRegister)wRn,           \
         (CRegister)wRm, 3, cond);                            \
  }                                                           
  F(wcmpeqb, 0)
  F(wcmpeqh, 1)  
  F(wcmpeqw, 2)  
  #undef F
  // Opcode1 = w w s 1 
  #define F(mnemonic, s, ww)                                  \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,           \
                WMMXRegister wRm, Condition cond = al) {      \
    cdp(p0, ((0x4*ww)|(0x2*s)|1), (CRegister)wRd,             \
         (CRegister)wRn, (CRegister)wRm, 3, cond);            \
  }                                                           
  F(wcmpgtub, 0, 0)
  F(wcmpgtuh, 0, 1)  
  F(wcmpgtuw, 0, 2)  
  F(wcmpgtsb, 1, 0)
  F(wcmpgtsh, 1, 1)  
  F(wcmpgtsw, 1, 2)  
  #undef F

// Coprocessor Data (CDP) Instructions
// type = PACK: cp_num = p0, opcode2 = 1 0 0
  // Opcode1 = w w s s 
  #define F(mnemonic, ss, ww)                              \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,        \
                WMMXRegister wRm, Condition cond = al) {   \
    cdp(p0, ((0x4*ww)|ss), (CRegister)wRd,                 \
         (CRegister)wRn, (CRegister)wRm, 0x4, cond);       \
  }                                                           
  F(wpackhus, 1, 1)
  F(wpackwus, 1, 2)
  F(wpackdus, 1, 3)    
  F(wpackhss, 3, 1)
  F(wpackwss, 3, 2)
  F(wpackdss, 3, 3)    
  #undef F

// Coprocessor Data (CDP) Instructions 
// type = UPACK: cp_num=p0, opcode2=110 and 111
  // Opcode2 = 1 1 0, Opcode1 = w w s 0 
  #define F(mnemonic, s, ww)                                     \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,              \
                Condition cond = al) {                           \
    cdp(p0, ((0x4*ww)|(0x2*s)), (CRegister)wRd, (CRegister)wRn,  \
         (CRegister)0, 0x6, cond);                               \
  }                                                           
  F(wunpackehub, 0, 0)
  F(wunpackehuh, 0, 1)
  F(wunpackehuw, 0, 2)
  F(wunpackehsb, 1, 0)
  F(wunpackehsh, 1, 1)
  F(wunpackehsw, 1, 2)
  #undef F
  // Opcode2 = 1 1 0, Opcode1 = w w 0 1 
  #define F(mnemonic, ww)                                  \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,        \
                WMMXRegister wRm, Condition cond = al) {   \
    cdp(p0, ((0x4*ww)|1), (CRegister)wRd, (CRegister)wRn,  \
         (CRegister)wRm, 0x6, cond);                       \
  }                                                           
  F(wunpackihb, 0)
  F(wunpackihh, 1)
  F(wunpackihw, 2)
  #undef F
  // Opcode2 = 1 1 1, Opcode1 = w w s 0  
  #define F(mnemonic, s, ww)                                     \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,              \
                Condition cond = al) {                           \
    cdp(p0, ((0x4*ww)|(0x2*s)), (CRegister)wRd, (CRegister)wRn,  \
        (CRegister)0, 0x7, cond );                               \
  }                                                           
  F(wunpackelub, 0, 0)
  F(wunpackeluh, 0, 1)
  F(wunpackeluw, 0, 2)
  F(wunpackelsb, 1, 0)
  F(wunpackelsh, 1, 1)
  F(wunpackelsw, 1, 2)
  #undef F
  // Opcode2 = 1 1 1, Opcode1 = w w 0 1
  #define F(mnemonic, ww)                                   \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,         \
                WMMXRegister wRm, Condition cond = al) {    \
    cdp(p0, ((0x4*ww)|1), (CRegister)wRd, (CRegister)wRn,   \
         (CRegister)wRm, 0x7, cond);                        \
  }                                                           
  F(wunpackilb, 0)
  F(wunpackilh, 1)
  F(wunpackilw, 2)
  #undef F


// Coprocessor Data (CDP) Instructions 
// type = Multiply: cp_num=p1, opcode2=000
  
  // Opcode1 = 0 0 s f
  #define F(mnemonic, s, f)                               \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,       \
                WMMXRegister wRm, Condition cond = al) {  \
    cdp(p1, ((0x2*s)|f), (CRegister)wRd, (CRegister)wRn,  \
         (CRegister)wRm, 0, cond);                        \
  }                                                           
  F(wmulul, 0, 0)
  F(wmulum, 0, 1)
  F(wmulsl, 1, 0)
  F(wmulsm, 1, 1)
  #undef F
  // Opcode1 = 0 1 s z
  #define F(mnemonic, s, z)                                   \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,           \
                WMMXRegister wRm, Condition cond = al) {      \
    cdp(p1, (0x4|(0x2*s)|z), (CRegister)wRd, (CRegister)wRn,  \
         (CRegister)wRm, 0, cond);                            \
  }                                                           
  F(wmacu,  0, 0)
  F(wmacuz, 0, 1)  
  F(wmacs,  1, 0)
  F(wmacsz, 1, 1)  
  #undef F

  // Opcode1 = 1 0 s 0
  #define F(mnemonic, s)                                    \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,         \
                WMMXRegister wRm, Condition cond = al) {    \
    cdp(p1, 0x8|(0x2*s), (CRegister)wRd, (CRegister)wRn,    \
         (CRegister)wRm, 0, cond);                          \
  }                                                           
  F(wmaddu, 0)
  F(wmadds, 1)  
  #undef F

// Coprocessor Data (CDP) Instructions
// type = Difference: cp_num=p1, opcode2=001
  // Opcode1 = 0 h 0 z
  #define F(mnemonic, h, z)                                 \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,         \
                WMMXRegister wRm, Condition cond = al) {    \
    cdp(p1, (0x4*h)|z, (CRegister)wRd, (CRegister)wRn,      \
         (CRegister)wRm, 1, cond);                          \
  }                                                           
  F(wsadb,  0, 0)
  F(wsadbz, 0, 1)  
  F(wsadh,  1, 0)
  F(wsadhz, 1, 1)  
  #undef F

// Coprocessor Data (CDP) Instructions
// type = Max/Min: cp_num=p1, opcode2=011
  // Opcode1 = w w s 0
  #define F(mnemonic, ww, s)                                 \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,          \
                WMMXRegister wRm, Condition cond = al) {     \
    cdp(p1, (0x4*ww)|(0x2*s), (CRegister)wRd, (CRegister)wRn,\
         (CRegister)wRm, 3, cond);                           \
  }                                                           
  F(wmaxub, 0, 0)
  F(wmaxuh, 1, 0)  
  F(wmaxuw, 2, 0)    
  F(wmaxsb, 0, 1)
  F(wmaxsh, 1, 1)  
  F(wmaxsw, 2, 1)    
  #undef F
  // Opcode1 = w w s 1
  #define F(mnemonic, ww, s)                                     \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,              \
                WMMXRegister wRm, Condition cond = al) {         \
    cdp(p1, (0x4*ww)|(0x2*s)|1, (CRegister)wRd, (CRegister)wRn,  \
        (CRegister)wRm, 3, cond );                               \
  }                                                           
  F(wminub, 0, 0)
  F(wminuh, 1, 0)  
  F(wminuw, 2, 0)    
  F(wminsb, 0, 1)
  F(wminsh, 1, 1)  
  F(wminsw, 2, 1)    
  #undef F

// Coprocessor Data (CDP) Instructions
// type = Arithmetic: cp_num=p1, opcode2=100, 101, 110
  // Opcode1 = w w s s, Opcode2 = 1 0 0
  #define F(mnemonic, ww, ss)                             \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,       \
                WMMXRegister wRm, Condition cond = al) {  \
    cdp(p1, (0x4*ww)|ss, (CRegister)wRd, (CRegister)wRn,  \
         (CRegister)wRm, 0x4, cond);                      \
  }                                                           
  F(waddbus, 0, 1)
  F(waddhus, 1, 1)
  F(waddwus, 2, 1)
  F(waddbss, 0, 3)
  F(waddhss, 1, 3)
  F(waddwss, 2, 3)
  #undef F
  // Opcode1 = w w s s, Opcode2 = 1 0 1
  #define F(mnemonic, ww, ss)                              \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,        \
                WMMXRegister wRm, Condition cond = al) {   \
    cdp(p1, (0x4*ww)|ss, (CRegister)wRd, (CRegister)wRn,   \
        (CRegister)wRm, 0x5, cond );                       \
  }                                                           
  F(wsubbus, 0, 1)
  F(wsubhus, 1, 1)
  F(wsubwus, 2, 1)
  F(wsubbss, 0, 3)
  F(wsubhss, 1, 3)
  F(wsubwss, 2, 3)
  #undef F
  // Opcode1 = w w 0 0, Opcode2 = 1 1 0
  #define F(mnemonic, ww)                                 \
  void mnemonic(WMMXRegister wRd, WMMXRegister wRn,       \
                WMMXRegister wRm, Condition cond = al) {  \
    cdp(p1, 0x4*ww, (CRegister)wRd, (CRegister)wRn,       \
         (CRegister)wRm, 0x6, cond);                      \
  }                                                           
  F(waccb, 0)
  F(wacch, 1)
  F(waccw, 2)   
  #undef F
  
// Coprocessor Data (CDP) Instructions 
// type = Shuffle: cp_num=p1, opcode2=111
  // Opcode1 = d d c c
  void wshufh(WMMXRegister wRd, WMMXRegister wRn, WMMXRegister wRm, 
              int Imm8, Condition cond = al) {
      int bbaa, ddcc;
      bbaa = Imm8 & 0xF;
      ddcc = (Imm8 & 0xF0) >> 0x4;
      cdp(p1, ddcc, (CRegister)wRd, (CRegister)wRn, 
           (CRegister)bbaa, 0x7, cond);
  }

// Transfers to Coprocessor Register (MCR) Instructions
  // Opcode1 = 000, Opcode2 = 000, cp_num = 0001, wRm = 0000
  void tmcr(WCGRegister wCx, Register Rn, Condition cond = al) {
      mcr(p1, 0, Rn, (CRegister)wCx, (CRegister)0, 0x0, cond);
  }
  // Opcode1 = 001, Opcode2 = 0000, cp_num = 0000
  void tmia(WMMXRegister wRd, Register Rm, Register Rs, Condition cond = al) {
      emit((cond << 28 ) | (0xE << 24) | (1 << 21) | 
           (Rs << 12) | (wRd << 5 ) | (1<<4) | Rm);
  }
  // Opcode1 = 001, Opcode2 = 1000, cp_num = 0000
  void tmiaph(WMMXRegister wRd, Register Rm, 
              Register Rs, Condition cond = al) {
      emit((cond << 28 ) | (0xE << 24) | (1 << 21) | 
            (0x8 << 16) | (Rs << 12) | (wRd << 5 ) | (1<<4) | Rm);
  }
  // Opcode1 = 001, Opcode2 = 11xy, cp_num = 0000
  #define F(mnemonic, x, y)                                       \
  void mnemonic(WMMXRegister wRd, Register Rm, Register Rs,       \
                Condition cond = al) {                            \
    emit((cond << 28 ) | (0xE << 24) | (1 << 21) | (0x3 << 18) |  \
         (x << 17) | (y << 16) | (Rs << 12) | (wRd << 5 ) |       \
         (1<<4) | Rm);                                            \
  }                                                           
  F(tmiabb, 0, 0)
  F(tmiatb, 1, 0)
  F(tmiabt, 0, 1)
  F(tmiatt, 1, 1)      
  #undef F
  // Opcode1 = 010, Opcode2 = ww0, cp_num = 0000, wRm = 0000
  #define F(mnemonic, ww)                                              \
  void mnemonic(WMMXRegister wRd, Register Rn, Condition cond = al) {  \
    mcr(p0, 0x2, Rn, (CRegister)wRd, (CRegister)0, 0x2*ww, cond);      \
  }                                                           
  F(tbcstb, 0)
  F(tbcsth, 1)
  F(tbcstw, 2)  
  #undef F
  // Opcode1 = 011, Opcode2 = ww0, cp_num = 0000, wRm = 0bbb
  #define F(mnemonic, ww)                              \
  void mnemonic(WMMXRegister wRd, Register Rn,         \
                int Imm3, Condition cond = al) {       \
    Imm3 = (Imm3&0x7);                                 \
    mcr(p0, 0x3, Rn, (CRegister)wRd,                   \
        (CRegister)Imm3, 0x2*ww, cond);                \
  }                                                           
  F(tinsrb, 0)
  F(tinsrh, 1)
  F(tinsrw, 2)  
  #undef F

// Transfers to Coprocessor Register (MCRR) Instructions
  // opcode2 = 0000, cp_num = 0000
  void tmcrr(WMMXRegister wRd, Register RdLo, 
             Register RdHi, Condition cond = al) {
      emit((cond << 28 ) | (0xC4 << 20) | 
           (RdHi << 16) | (RdLo << 12) | wRd);
  }

// Transfers from Coprocessor Register (MRC) Instructions
  // opcode1 = 000, opcode2 = 000, cp_num = 0001, wRm = 0000
  void tmrc(Register Rd, WCGRegister wCx, Condition cond = al) {
      mrc(p1, 0, Rd, (CRegister)wCx, (CRegister)0, 0x0, cond);
  }
  // opcode1 = ww0, opcode2 = 001, cp_num = 0000, wRm = 0000
  #define F(mnemonic, ww)                                             \
  void mnemonic(Register Rd, WMMXRegister wRn, Condition cond = al) { \
    mrc(p0, 0x2*ww, Rd, (CRegister)wRn, (CRegister)0, 1, cond);       \
  }                                                           
  F(tmovmskb, 0)
  F(tmovmskh, 1)
  F(tmovmskw, 2)  
  #undef F
  // opcode1 = ww0, opcode2 = 001, cp_num = 0001, wRm = 0000  
  #define F(mnemonic, ww)                                        \
  void mnemonic(Register Rd = r15, Condition cond = al) {        \
    mrc(p1, 0x2*ww, Rd, (CRegister)3, (CRegister)0, 1, cond);    \
  }                                                           
  F(tandcb, 0)
  F(tandch, 1)
  F(tandcw, 2)  
  #undef F
  // opcode1 = ww0, opcode2 = 010, cp_num = 0001, wRm = 0000  
  #define F(mnemonic, ww)                                        \
  void mnemonic(Register Rd = r15, Condition cond = al) {        \
    mrc(p1, 0x2*ww, Rd, (CRegister)3, (CRegister)0, 2, cond);    \
  }                                                           
  F(torcb, 0)
  F(torch, 1)
  F(torcw, 2)  
  #undef F
  // opcode1 = ww0, opcode2 = 011, cp_num = 0001, wRm = 0bbb  
  #define F(mnemonic, ww)                                          \
  void mnemonic(int Imm3, Condition cond = al) {                   \
    Imm3 = (Imm3&0x7);                                             \
    mrc( p1, 0x2*ww, r15, (CRegister)3, (CRegister)Imm3, 3, cond );\
  }                                                           
  F(textrcb, 0)
  F(textrch, 1)
  F(textrcw, 2)  
  #undef F
  // opcode1 = ww0, opcode2 = 011, cp_num = 0000, wRm = 0bbb  
  #define F(mnemonic, s, ww)                         \
  void mnemonic(Register Rd, WMMXRegister wRn,       \
                int Imm3, Condition cond = al) {     \
    Imm3 = (Imm3&0x7);                               \
    mrc(p0, 0x2*ww, Rd, (CRegister)wRn,              \
         (CRegister)(Imm3|(0x8*s)), 3, cond);        \
  }                                                           
  F(textrmub, 0, 0)
  F(textrmuh, 0, 1)
  F(textrmuw, 0, 2)  
  F(textrmsb, 1, 0)
  F(textrmsh, 1, 1)
  F(textrmsw, 1, 2)  
  #undef F

// Transfers from Coprocessor Register (MRRC) Instructions
  // opcode2 = 0000, cp_num = 0000
  void tmrrc(Register RdLo, Register RdHi, 
             WMMXRegister wRn, Condition cond = al) {
      emit((cond << 28 ) | (0xC5 << 20) | (RdHi << 16) | (RdLo << 12) | wRn);
  }

// Load/Store to Wireless MMX technology Data Registers
  // WLDR  
  #define F(mnemonic, N, M, Shift)                                  \
  void mnemonic(WMMXRegister wRd, Register Rn, int offset_8 = 0,    \
                Mode mode = offset, Condition cond = al) {          \
    if(mode == post_indexed) {                                      \
      mode = (Mode)(1 << 21);                                       \
    }                                                               \
    emit( (cond << 28) | (6 << 25) | mode | (up(offset_8) << 23) |  \
          (N << 22) | (1 << 20) | (Rn << 16) | (wRd << 12) |        \
          (M << 8) | abs(offset_8>>Shift) & 0xff);                  \
  }                                                           
  F(wldrb, 0, 0, 0)
  F(wldrh, 1, 0, 0)  
  F(wldrw, 0, 1, 2)
  F(wldrd, 1, 1, 2)  
  #undef F

  // WLDRW wCx
  #define F(mnemonic, N, M, Shift)                                  \
  void mnemonic(WCGRegister wCx, Register Rn, int offset_8 = 0,     \
                Mode mode = offset, Condition cond = al) {          \
    if(mode == post_indexed) {                                      \
      mode = (Mode)(1 << 21);                                       \
    }                                                               \
    emit((cond << 28) | (6 << 25) | mode | (up(offset_8) << 23) |   \
         (N << 22) | (1 << 20) | (Rn << 16) | (wCx << 12) |         \
         (M << 8) | abs(offset_8>>Shift) & 0xff);                   \
  }                                                           
  F(wldrw, 0, 1, 2)
  #undef F

  // WSTR  
  #define F(mnemonic, N, M, Shift)                                  \
  void mnemonic(WMMXRegister wRm, Register Rn, int offset_8 = 0,    \
                Mode mode = offset, Condition cond = al) {          \
    if(mode == post_indexed) {                                      \
      mode = (Mode)(1 << 21);                                       \
    }                                                               \
    emit((cond << 28) | (6 << 25) | mode | (up(offset_8) << 23) |   \
         (N << 22) | (Rn << 16) | (wRm << 12) |                     \
         (M << 8) | abs(offset_8>>Shift) & 0xff);                   \
  }                                                           
  F(wstrb, 0, 0, 0)
  F(wstrh, 1, 0, 0)  
  F(wstrw, 0, 1, 2)
  F(wstrd, 1, 1, 2)  
  #undef F

  // WSTRW wCx
  #define F(mnemonic, N, M, Shift)                                  \
  void mnemonic(WCGRegister wCx, Register Rn, int offset_8 = 0,     \
                Mode mode = offset, Condition cond = al) {          \
    if(mode == post_indexed) {                                      \
      mode = (Mode)(1 << 21);                                       \
    }                                                               \
    emit((cond << 28) | (6 << 25) | mode | (up(offset_8) << 23) |   \
         (N << 22) | (Rn << 16) | (wCx << 12) |                     \
         (M << 8) | abs(offset_8>>Shift) & 0xff);                   \
  }                                                           
  F(wstrw, 0, 1, 2)
  #undef F
