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

#if ENABLE_COMPILER && !ENABLE_THUMB_COMPILER

class Instruction: public StackObj {
 private:
  address _addr;

 protected:
  void check_alignment(address addr) const {
    GUARANTEE(((int)addr & 3) == 0, "unaligned addr");
    (void)addr;
  }

 public:
  // creation
  Instruction(address addr)                      { check_alignment(addr); _addr = addr; }

  // accessors
  address addr() const                           { return _addr; }
  int encoding() const                           { return *(int*)addr(); }
  int kind() const                               { return encoding() >> 25 & 0x7; }
  bool bit(int i) const                          { return (encoding() >> i & 0x1) == 1; }

  // manipulation
  void set_encoding(int instr) const             { *(int*)_addr = instr; }
};

class MemAccess: public Instruction {
 public:
  MemAccess(address addr) : Instruction(addr) {
    GUARANTEE(Compiler::current()->code_generator()->has_overflown_compiled_method()
              || (encoding() & 0x0e000000) == 0x04000000, "must be load/store")
  }

  int offset() const {
    const int imm12 = encoding() & 0xfff;
    return bit(23) ? imm12 : -imm12;
  }

  void set_offset(int offset) const {
    GUARANTEE(abs(offset) < 0x1000, "offset too large")
    set_encoding(encoding() & 0xff7ff000 | Assembler::up(offset) << 23 | abs(offset));
  }

  address location() const {
    return addr() + 8 + offset();
  }

  void set_location(address loc) const {
    check_alignment(loc);
    set_offset(loc - addr() - 8);
  }

};

#if ENABLE_ARM_VFP
class VFPMemAccess: public Instruction {
 public:
  VFPMemAccess(address addr) : Instruction(addr) {
  }

  int offset() const {
    const int imm8 = (encoding() & 0xfff) << 2;
    return bit(23) ? imm8 : -imm8;
  }

  void set_offset(int offset) const {
    GUARANTEE(abs(offset) < 0x400, "offset too large");
    // Sets the offset and transforms flds_stub to flds
    set_encoding(encoding() & 0xff7ff000 | Assembler::up(offset) << 23 | 10 << 8 | (abs(offset) >> 2));
  }

  address location() const {
    return addr() + 8 + offset();
  }

  void set_location(address loc) const {
    check_alignment(loc);
    set_offset(loc - addr() - 8);
  }
};
#endif

class Branch: public Instruction {
 public:
  Branch(address addr) : Instruction(addr) {
    GUARANTEE( kind() == 5, "must be branch instruction");
  }

  int imm24() const {
    return encoding() << 8 >> 8; // sign extension
  }

  void set_imm24(int imm24) const {
    // Note: The (int) casts are required to enforce signed comparison using MS EVC++.
    GUARANTEE((int)0xff800000 <= imm24 && imm24 < (int)0x00800000, "imm24 too large");
    set_encoding(encoding() & 0xff000000 | imm24 & 0x00ffffff);
  }

  address target() const {
    return addr() + 8 + (imm24() << 2);
  }

  void set_target(address target) const {
    check_alignment(target);
    set_imm24((target - addr() - 8) >> 2);
  }

  void relocate(int delta) const {
    check_alignment((address)delta);
    set_imm24(imm24() - (delta >> 2));
  }

};

#endif // ENABLE_COMPILER && !ENABLE_THUMB_COMPILER
