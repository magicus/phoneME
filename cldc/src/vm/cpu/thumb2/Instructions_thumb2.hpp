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

#if ENABLE_COMPILER
class Instruction: StackObj {
 private:
  address _addr;

 protected:
  static void check_overflow( void ) {
    GUARANTEE(!CodeGenerator::current()->has_overflown_compiled_method(),
              "compiled method overflow" );
  }
  void check_alignment(const address addr) const {
    GUARANTEE(((int)addr & 1) == 0, "unaligned addr");
  }
  void check_kind( const short expected_kind, const char msg[] ) {
    GUARANTEE( kind() == expected_kind, msg );
  }

 public:
  // creation
  Instruction( const address addr ) {
    check_overflow();
    check_alignment( addr );
    _addr = addr;
  }

  enum Kind {
    vfp = 0,
    ldr = 1
  };

  // accessors
  address addr( void ) const {
    check_overflow();
    return _addr;
  }
  short* encoding( const int i ) const {
    return ((short*) addr()) + i;
  }
  short encoding( void ) const {
    return *encoding(0);
  }
  short encoding_next( void ) const {
    return *encoding(1);
  }

  short kind( void ) const {
    return (encoding() >> 14) & 0x3;
  }
  bool bit(const int i) const {
    return ((encoding() >> i) & 0x1) == 1;
  }

  // manipulation
  void set_encoding( const short instr ) const {
    *encoding(0) = instr;
  }
  void set_encoding_next( const short instr ) const {
    *encoding(1) = instr;
  }
};

class MemAccess: Instruction {
 public:
  MemAccess( const address addr ): Instruction( addr ) {
    check_kind( ldr, "must be pc-relative load");
  }

  int offset( void ) const {
    const int imm8 = encoding() & 0xff;
    // PC relative ldr offsets are multiplied by 4
    // to obtain the intended offset
    return imm8 * 4;
  }

  void set_offset( const int offset ) const {
    GUARANTEE(offset >= 0 && abs(offset)/4 < 0x100, "offset too large")
    set_encoding(encoding() & 0xff00 | abs(offset/4));
  }

  address location( void ) const {
    int offset = (encoding() & 0x7FF);
    offset = (offset << 21) >> 21;
    return addr() + (offset * 2);
  }

  int reg ( void ) const { return (encoding() >> 11) & 0x7; }

  void set_location( const address loc ) const {
    check_alignment(loc);

    const int rd = reg();
    const short load_ins = (4 << 12 | 1 << 11 | rd << 8);
    set_encoding(load_ins);

    const int offset = (int)loc - ((int)addr() & ~3) - 4;
    GUARANTEE(offset % 4 == 0, "MemAccess: Invalid ldr offset");
    set_offset(offset);
  }
};

#if USE_ARM_VFP_LITERALS
class VFPMemAccess: Instruction {
 public:
  VFPMemAccess( const address addr ): Instruction( addr ) {
    check_kind( 0, "must be pc-relative flds" );
  }

  int offset( void ) const {
    return encoding() & 0x7FF;
  }
  address location( void ) const {
    return addr() + 2*offset();
  }
  int reg ( void ) const {
    const int freg = encoding_next();
    GUARANTEE( freg < 32, "wrong freg" );
    return freg;
  }

  void set_offset( const int offset ) const {
    GUARANTEE( abs(offset) < 0x400, "offset too large" );

    // Sets the offset and transforms flds_stub to flds
    const int freg = reg();
    set_encoding( 0xFD1F | (freg & 1) << 6 | Assembler::up(offset) << 7 );
    set_encoding_next( 0x0A00 | (freg >> 1) << 12 | (abs(offset) >> 2) );
  }
  void set_location( const address loc ) const {
    check_alignment( loc );
    set_offset( loc - addr() - 4 );
  }
};
#endif

class Branch: Instruction {
 public:
  Branch( const address addr ): Instruction( addr ) {
    GUARANTEE( (kind() & 2) == 2, "must be branch instruction" );
  }

  int imm( void ) const {
    const short instr = encoding();
    const short type = (instr >> 13) & 0x7;

    if (type == 0x6) {
      // Conditional branch -  signed_immed_8
      // SignExtend(immed_8 << 1)
      int offset = (instr & 0xFF) << 1;
      offset = (offset << 23) >> 23;
      return offset;
    } else if (type == 0x7) {
      int offset = (instr & 0x7FF) << 1;
      if( ((instr >> 11) & 1 ) == 0 ) {
        // unconditional branch - signed_immed_11
        // SignExtend(immed_11 << 1)
        offset = (offset << 20) >> 20;
      }
      return offset;
    }

    return instr & 0x7FF;
  }

  bool is_conditional( void ) const {
    const short instr = encoding();
    if (is_long_encoding(instr)) {
      const short instr_next = encoding_next();
      return (((instr_next >> 12) & 0x1) == 0);
    } else {
      return (((instr >> 13) & 0x7) == 0x6);
    }
  }

  int condition ( void ) const {
    GUARANTEE( is_conditional(), "sanity check" );
    const int shift = is_long() ? 6 : 8;
    return (encoding() >> shift) & 0xF;
  }

  void set_imm(const int target) const;

  address target( void ) const;

  void set_target(const address target) const {
    check_alignment(target);
    set_imm((target - addr() - 4) >> 1);
  }

  bool is_long( void ) const {
    const short instr = encoding();
    return is_long_encoding(instr);
  }

 private:
  static bool is_long_encoding(const short instr) {
    return (((instr >> 12) & 0xF) == 0xF);
  }
};
#endif
