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

#if ENABLE_COMPILER

class Instruction: public StackObj {
private:
  address _addr;

public:
  Instruction(address addr) {
    _addr = addr;
  }

  address addr()                const { return _addr; }
  
  jubyte  byte_at  (int offset) const { return *(jbyte*)  (_addr + offset); }
  jushort short_at (int offset) const { return *(jshort*) (_addr + offset); }
  juint   int_at   (int offset) const { return *(jint*)   (_addr + offset); }
  jubyte  ubyte_at (int offset) const { return *(jubyte*) (_addr + offset); }
  jushort ushort_at(int offset) const { return *(jushort*)(_addr + offset); }
  juint   uint_at  (int offset) const { return *(juint*)  (_addr + offset); }

  void byte_at_put (int offset, jbyte  value) { *(jbyte*) (_addr + offset) = value; }
  void short_at_put(int offset, jshort value) { *(jshort*)(_addr + offset) = value; }
  void int_at_put  (int offset, jint   value) { *(jint*)  (_addr + offset) = value; }

};

class MemAccess: public Instruction {
public:
  MemAccess(address addr) : Instruction(addr) {
    GUARANTEE(ubyte_at(0) >= 0x88 && ubyte_at(0) <= 0x8b, "MOV instruction");
  }

  int offset() const {
    GUARANTEE((ubyte_at(1) & 0xc7) == 0x05, "Simple 32-bit displacement");
    return int_at(2);
  }

  void set_offset(int offset) {
    GUARANTEE((ubyte_at(1) & 0xc7) == 0x05, "Simple 32-bit displacement");
    int_at_put(2, offset);
  }

};

#endif // ENABLE_COMPILER
