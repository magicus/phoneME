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

#ifndef VFP_COND
  #error "VFP_COND must be defined"
#endif

   // sn <- rd
   void fmsr(Register sn, Register rd VFP_COND
     jint n = sn - s0;
     jint Fn = n >> 1;   // top 4 bits
     jint N  = n & 0x01; // bottom bit
     VFP_EMIT(cond << 28 | 0xe0 << 20 | Fn << 16 | rd << 12 |
              0x0a << 8 | N << 7 | 0x10);
   }
   // rd <- sn
   void fmrs(Register rd, Register sn VFP_COND
     jint n = sn - s0;
     jint Fn = n >> 1;   // top 4 bits
     jint N  = n & 0x01; // bottom bit
     VFP_EMIT(cond << 28 | 0xe1 << 20 | Fn << 16 | rd << 12 |
              0x0a << 8 | N << 7 | 0x10);
   }
   // rd <- system reg
   void fmrx(Register rd, VFPSystemRegister reg VFP_COND
     VFP_EMIT(cond << 28 | 0xef << 20 | reg << 16 | rd << 12 |
              0x0a << 8 | 0x10);
   }
   // system reg <- rd
   void fmxr(VFPSystemRegister reg, Register rd VFP_COND
     VFP_EMIT(cond << 28 | 0xee << 20 | reg << 16 | rd << 12 |
              0x0a << 8 | 0x10);
   }

   // dm <- rd rn
   void fmdrr(Register sm, Register rd, Register rn VFP_COND
     juint m = sm - s0;
     jint  Fm = m >> 1;    // top 4 bits
     jint  M  = m & 0x01;  // bottom bits
     VFP_EMIT(cond << 28 | 0xc4 << 20 | rn << 16 | rd << 12 |
              0x0b << 8 | M << 5 | 0x01 << 4 | Fm);
   }
   // rd rn <- dm
   void fmrrd(Register rd, Register rn, Register sm VFP_COND
     juint m = sm - s0;
     jint  Fm = m >> 1;    // top 4 bits
     jint  M  = m & 0x01;  // bottom bits
     VFP_EMIT(cond << 28 | 0xc5 << 20 | rn << 16 | rd << 12 |
              0x0b << 8 | M << 5 | 0x01 << 4 | Fm);
   }

   void fmstat( SINGLE_ARG_VFP_COND
     VFP_EMIT(cond << 28 | 0xef << 20 | 0x01 << 16 | 0x0f << 12 |
              0x0a << 8 | 0x10);
   }

#define F(mnemonic, Fn, N, cpnum) \
  void mnemonic(Register sd, Register sm VFP_COND \
     jint d = sd - s0; \
     jint m = sm - s0; \
     jint Fd = d >> 1;   /* top 4 bits */ \
     jint Fm = m >> 1;   /* top 4 bits */ \
     jint D  = d & 0x01; /* bottom bit */ \
     jint M  = m & 0x01; /* bottom bit */ \
     \
     VFP_EMIT(cond << 28 | 0x1d << 23 | D << 22 | 3 << 20 | Fn << 16 |\
              Fd << 12 | cpnum << 8 | N << 7 | 1 << 6 | M << 5 | Fm); \
   }
   F(fcpys,   0x0, 0, 10) // sd =  sm
   F(fabss,   0x0, 1, 10) // sd =  abs(sm)
   F(fnegs,   0x1, 0, 10) // sd = -sm
   F(fsqrts,  0x1, 1, 10) // sd =  sqrt(sm)
   F(fcmps,   0x4, 0, 10)
   F(fcmpes,  0x4, 1, 10)
   F(fcmpzs,  0x5, 0, 10)
   F(fcmpezs, 0x5, 1, 10)
   F(fcvtds,  0x7, 1, 10)
   F(fuitos,  0x8, 0, 10)
   F(fsitos,  0x8, 1, 10)
   F(ftouis,  0xc, 0, 10)
   F(ftouizs, 0xc, 1, 10)
   F(ftosis,  0xd, 0, 10)
   F(ftosizs, 0xd, 1, 10)

   F(fcpyd,   0x0, 0, 11) // sd =  sm
   F(fabsd,   0x0, 1, 11) // sd =  abs(sm)
   F(fnegd,   0x1, 0, 11) // sd = -sm
   F(fsqrtd,  0x1, 1, 11) // sd =  sqrt(sm)
   F(fcmpd,   0x4, 0, 11)
   F(fcmped,  0x4, 1, 11)
   F(fcmpzd,  0x5, 0, 11)
   F(fcmpezd, 0x5, 1, 11)
   F(fcvtsd,  0x7, 1, 11)
   F(fuitod,  0x8, 0, 11)
   F(fsitod,  0x8, 1, 11)
   F(ftouid,  0xc, 0, 11)
   F(ftouizd, 0xc, 1, 11)
   F(ftosid,  0xd, 0, 11)
   F(ftosizd, 0xd, 1, 11)

#undef F

#define F(mnemonic, p, q, r, s, cpnum) \
   void mnemonic(Register sd, Register sn, Register sm VFP_COND \
     sd = Register(sd - s0); \
     sm = Register(sm - s0); \
     sn = Register(sn - s0); \
     jint Fd = sd >> 1;   /* top 4 bits */ \
     jint Fm = sm >> 1;   /* top 4 bits */ \
     jint Fn = sn >> 1;   /* top 4 bits */ \
     jint D  = sd & 0x01; /* bottom bit */ \
     jint M  = sm & 0x01; /* bottom bit */ \
     jint N  = sn & 0x01; /* bottom bit */ \
     \
     VFP_EMIT(cond << 28 | 0x0e << 24 | p << 23 | D << 22 | q << 21 | \
              r << 20 | Fn << 16 |  \
              Fd << 12 | cpnum << 8 | N << 7 | s << 6 | M << 5 | Fm); \
   }
   F(fmacs,  0, 0, 0, 0, 10)
   F(fnmacs, 0, 0, 0, 1, 10)
   F(fmscs,  0, 0, 1, 0, 10)
   F(fnmscs, 0, 0, 1, 1, 10)
   F(fmuls,  0, 1, 0, 0, 10)
   F(fnmuls, 0, 1, 0, 1, 10)
   F(fadds,  0, 1, 1, 0, 10)
   F(fsubs,  0, 1, 1, 1, 10)
   F(fdivs,  1, 0, 0, 0, 10)

   F(fmacd,  0, 0, 0, 0, 11)
   F(fnmacd, 0, 0, 0, 1, 11)
   F(fmscd,  0, 0, 1, 0, 11)
   F(fnmscd, 0, 0, 1, 1, 11)
   F(fmuld,  0, 1, 0, 0, 11)
   F(fnmuld, 0, 1, 0, 1, 11)
   F(faddd,  0, 1, 1, 0, 11)
   F(fsubd,  0, 1, 1, 1, 11)
   F(fdivd,  1, 0, 0, 0, 11)
#undef F

  // Place holder for flds to use a 12 bit offset during compilation.
  // The place holder will be replaced by flsd when the literal is bound.
  // This way prevents us from using fldd to access the literal pool
  enum Address5_stub {
    forceaddress5_stub=0x10000000  // force Address3 to be int size
  };

  static Address5_stub imm_index5_stub(Register rn, int offset_12) {
    GUARANTEE(offset_12 % 4 == 0, "Offset must be multiple of 4");
    check_imm(abs(offset_12 >> 2), 12);
    return (Address5_stub)(offset | (up(offset_12) << 23) | rn << 16 | abs(offset_12>>2));
  }

#define F(mnemonic, L, cpnum) \
   void mnemonic(Register sd, Address5 address5 VFP_COND \
     sd = Register(sd - s0); \
     jint Fd = sd >> 1;   /* top 4 bits */ \
     jint D  = sd & 0x01; /* bottom bit */ \
     \
     VFP_EMIT(cond << 28 | 0x06 << 25 | 1 << 24 | D << 22 | \
              L << 20 | Fd << 12 | cpnum << 8 | address5); \
   }

   F(flds, 1, 10)
   F(fsts, 0, 10)
   F(fldd, 1, 11)
   F(fstd, 0, 11)
#undef F

#define F(mnemonic, P, U, L, cpnum) \
   void mnemonic(Register rn, Register beg, int size, WritebackMode w = no_writeback VFP_COND \
     beg = Register(beg - s0); \
     jint Fd = beg >> 1;   /* top 4 bits */ \
     jint D  = beg & 0x01; /* bottom bit */ \
     \
     GUARANTEE(size != 0 && beg >= 0 && beg+size <= number_of_float_registers,  \
               "Invalid Register List"); \
     VFP_EMIT(cond << 28 | 0x06 << 25 | P << 24 | U << 23 | D << 22 | w << 21 |\
              L << 20 | rn << 16 | Fd << 12 | cpnum << 8 | size); \
   }

   // Non stack                // stack
   F(fldmiad, 0, 1, 1, 11)   F(fldmfdd, 0, 1, 1, 11)
   F(fldmias, 0, 1, 1, 10)   F(fldmfds, 0, 1, 1, 10)
   F(fldmdbd, 1, 0, 1, 11)   F(fldmead, 1, 0, 1, 11)
   F(fldmdbs, 1, 0, 1, 10)   F(fldmeas, 1, 0, 1, 10)
   F(fstmiad, 0, 1, 0, 11)   F(fstmead, 0, 1, 0, 11)
   F(fstmias, 0, 1, 0, 10)   F(fstmeas, 0, 1, 0, 10)
   F(fstmdbd, 1, 0, 0, 11)   F(fstmfdd, 1, 0, 0, 11)
   F(fstmdbs, 1, 0, 0, 10)   F(fstmfds, 1, 0, 0, 10)
#undef F

#undef VFP_COND
#undef SINGLE_ARG_VFP_COND
#undef VFP_EMIT
