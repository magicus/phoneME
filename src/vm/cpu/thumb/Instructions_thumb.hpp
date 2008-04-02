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

/*
 * Warning: ENABLE_THUMB_COMPILER=true is not a supported feature
 * This code is included only for reference purposes.
 */

#if ENABLE_THUMB_COMPILER

#if ENABLE_COMPILER

class Instruction: public StackObj {
 private:
  address _addr;

 protected:
  void check_alignment(address addr) const {
    GUARANTEE(((int)addr & 1) == 0, "unaligned addr");
    (void)addr;
  }

 public:
  // creation
  Instruction(address addr)                      { check_alignment(addr); _addr = addr; }

  // accessors
  address addr() const                           { return _addr; }
  short encoding() const                         { return *(short*)addr(); }
  short kind() const                             { return encoding() >> 14 & 0x3; }
  bool bit(int i) const                          { return (encoding() >> i & 0x1) == 1; }

  // manipulation
  void set_encoding(short instr) const { 
    if (!Compiler::current()->code_generator()->has_overflown_compiled_method()) 
      *(short*)_addr = instr; 
  }

  void set_encoding_next(short instr) const { 
    if (!Compiler::current()->code_generator()->has_overflown_compiled_method()) 
      *(((short*)_addr) + 1) = instr; 
  }
};

class MemAccess: public Instruction {
 public:
  MemAccess(address addr) : Instruction(addr) {
    GUARANTEE(!Compiler::current()->code_generator()->has_overflown_compiled_method(),
                 "must be pc-relative load/store")
  }

  int offset() const {
    const int imm8 = encoding() & 0xff;
    // PC relative ldr offsets are multiplied by 4
    // to obtain the intended offset
    return imm8 * 4; 
  }

  void set_offset(int offset) const {
    GUARANTEE(offset >= 0 && abs(offset)/4 < 0x100, "offset too large")
    set_encoding(encoding() & 0xff00 | abs(offset/4));
  }

  address location() const {
    int offset = (encoding() & 0x7FF);
    offset = (offset << 21) >> 21;
    return addr() + (offset * 2);
  }

  void set_location(address loc) const {
    check_alignment(loc);

    int rd = (encoding() >> 11) & 0x7;
    short load_ins = (4 << 12 | 1 << 11 | rd << 8);
    set_encoding(load_ins);
    int offset = (int)loc - ((int)addr() & ~3) - 4;
    GUARANTEE(offset % 4 == 0, "MemAccess: Invalid ldr offset");
    
    set_offset(offset);
  }
};

class Branch: public Instruction {
 public:
  Branch(address addr) : Instruction(addr) {
    GUARANTEE(Compiler::current()->code_generator()->has_overflown_compiled_method()
              || kind() == 3, "must be branch instruction");
  }

  int imm() const {
    short instr = encoding();
    short type = (instr >> 13) & 0x7;

    if (type == 0x6) {
      // Conditional branch -  signed_immed_8
      // SignExtend(immed_8 << 1)
      int offset = (instr & 0xFF) << 1;
      offset = offset << 23 >> 23;
      return offset;
    } else if (type == 0x7) {
      int offset = (instr & 0x7FF) << 1;
      bool hi = (instr & (1 << 11)) == 0;
      if (hi) {
        // unconditional branch - signed_immed_11
        // SignExtend(immed_11 << 1)
        offset = (offset << 20) >> 20;
      }
      return offset;
    }
    
    return (instr & 0x7FF);
  }

  bool is_conditional() const {
    short instr = encoding();
    return (((instr >> 13) & 0x7) == 0x6);
  }
  
  void set_imm(int target) const {
    short instr = encoding();
    short type = (instr >> 13) & 0x7;

    if (type == 0x6) {
      int offset = target << 1;
      offset = offset << 23 >> 23;
      GUARANTEE(offset >= -256 && offset < 255, "b<cond> <target>: Invalid offset");
      GUARANTEE(target < 128 && target >= -128, "b<cond> <target>: Invalid offset");
      set_encoding((instr & 0xFF00) | (target & 0xFF));
      GUARANTEE(imm() < 255 && imm() >= -256, "b<cond> <target>: Invalid offset");
    } else if (type == 0x7) {
      GUARANTEE(target < 2047 && target >= -2048, "b <target>: Invalid offset");
      set_encoding((instr & 0xF800) | (target & 0x7FF));
      GUARANTEE(imm() < 2047 && imm() >= -2048, "b <target>: Invalid offset");
    } else {
      set_encoding(instr & 0xF100 | target);
    }
  }

  address target() const;

  void set_target(address target) const {
    check_alignment(target);
    set_imm((target - addr() - 4) >> 1);
  }

  void emit_bl(address target) const {
    int offset = (target - addr() - 4);
    unsigned int target_hi = (offset >> 12) & 0x7FF;
    unsigned int target_lo = (offset >> 1) & 0x7FF;

    if (offset >= 0) {
      target_hi = (offset >> 12) & 0x7FF;
      target_lo = (offset >> 1) & 0x7FF;
    } else {
      int step = (1 << 12);
      int hi = -((abs(offset) + step - 1) & ~(step - 1));
      target_hi = (hi >> 12) & 0x7FF;
      hi = (((int)target_hi << 12) <<  9) >> 9;
      target_lo = ((abs(hi) - abs(offset)) >> 1) & 0x7FF;      
      GUARANTEE(((juint)hi + ((target_lo & 0x7FF) << 1)) == (juint)offset,
                "bl : invalid offset");
    }
    
    unsigned short bl_hi = (0x7 << 13 | 0x2 << 11 | target_hi);
    unsigned short bl_lo = (0x7 << 13 | 0x3 << 11 | target_lo);
    set_encoding(bl_hi);
    set_encoding_next(bl_lo);
  }
  
};

#endif

#endif /* #if ENABLE_THUMB_COMPILER*/
