/*
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
#include "incls/_BinaryAssembler_i386.cpp.incl"

#if ENABLE_COMPILER

inline FPURegisterMap& BinaryAssembler::fpu_register_map( void ) {
  return Compiler::frame()->fpu_register_map();
}

inline jint BinaryAssembler::long_at(const int position) const {
  return BinaryAssemblerCommon::int_at(position);
}
inline void BinaryAssembler::word_at_put(const int position,
                                         const jshort value) const {
  BinaryAssemblerCommon::short_at_put(position, value);
}
inline void BinaryAssembler::long_at_put(const int position,
                                         const jint value) const {
  BinaryAssemblerCommon::int_at_put(position, value);
}

bool BinaryAssembler::is_signed_byte(const int data) {
  return -0x80 <= data && data < 0x80;
}
bool BinaryAssembler::is_unsigned_byte(const int data) {
  return 0 <= data && data <= 0xff;
}

class DisassemblerInfo {
private:
  NOT_PRODUCT(BinaryAssembler *_ba;)
  NOT_PRODUCT(jint _start_code_offset;)
  NOT_PRODUCT(DisassemblerInfo *_previous;)
  NOT_PRODUCT(static DisassemblerInfo *_current;)
public:
  DisassemblerInfo(BinaryAssembler *ba) PRODUCT_RETURN;
  NOT_PRODUCT(~DisassemblerInfo();)
};

#ifndef PRODUCT

DisassemblerInfo::DisassemblerInfo(BinaryAssembler *ba) {
  _ba = ba;
  _start_code_offset = ba->code_size();
  _previous = _current;
  _current = this;
}

DisassemblerInfo::~DisassemblerInfo() {
  jint start = _start_code_offset;
  jint end = _ba->code_size();
  if (PrintCompiledCodeAsYouGo) {
    _ba->compiled_method()->print_code_on(tty, start, end);
  }
  // Some of the FP emitters call other FP functions.  We don't try to
  // sort it all out.  Rather, if this is a recursive creation of a 
  // Disassembler info, we make sure our parent doesn't reprint what
  // we've already disassembled
  if (_previous != NULL) {
    _previous->_start_code_offset = end;
  }
  _current = _previous;
}

DisassemblerInfo* DisassemblerInfo::_current = NULL;
#endif

void BinaryAssembler::generic_binary_1(Register dst, Register src,
                                       generic_binary_op_1 op) { 
  emit_arith(0x03 + op * 8, 0xC0, dst, src);
}

void BinaryAssembler::generic_binary_1(Register dst, int imm32,
                                       generic_binary_op_1 op) { 
  emit_arith(0x81, 0xC0 + op * 8, dst, imm32);
}

// Move instructions.
void BinaryAssembler::movl(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x8B);
  emit_operand(dst, src);
}  

void BinaryAssembler::movl(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x8B);
  emit_byte(0xC0 | (dst << 3) | src);
}

void BinaryAssembler::movl(Register dst, int imm32) {
  DisassemblerInfo print_me(this);
  emit_byte(0xB8 | dst);
  emit_long(imm32);
}

void BinaryAssembler::movl(Register dst, const Oop* oop) {
  DisassemblerInfo print_me(this);
  emit_byte(0xB8 | dst);
  // write relocation information for the oop
  emit_oop( oop->obj() );
  emit_long((int) oop->obj());
}

void BinaryAssembler::movl(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x89);
  emit_operand(src, dst);
}

void BinaryAssembler::movl(const Address& dst, int imm32) {
  DisassemblerInfo print_me(this);
  emit_byte(0xC7);
  emit_operand(eax, dst);
  emit_long(imm32);
}

void BinaryAssembler::movl(const Address& dst, const Oop* oop) {
  DisassemblerInfo print_me(this);
  emit_byte(0xC7);
  emit_operand(eax, dst);
  emit_oop(oop->obj());
  emit_long((int) oop->obj());
}

void BinaryAssembler::movb(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  GUARANTEE(is_valid_byte_register(dst), "Must have byte register"); 
  emit_byte(0x8A);
  emit_operand(dst, src);
}

void BinaryAssembler::movb(const Address& dst, int imm8) {
  DisassemblerInfo print_me(this);
  emit_byte(0xC6);
  emit_operand(eax, dst);
  emit_byte(imm8);
}

void BinaryAssembler::movb(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  GUARANTEE(is_valid_byte_register(src), "Must have byte register"); 
  emit_byte(0x88);
  emit_operand(src, dst);
}
  
void BinaryAssembler::movw(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x66);
  emit_byte(0x8B);
  emit_operand(dst, src);
}

void BinaryAssembler::movw(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x66);
  emit_byte(0x89);
  emit_operand(src, dst);
}

void BinaryAssembler::movw(const Address& dst, int imm16) {
  DisassemblerInfo print_me(this);
  emit_byte(0x66);
  emit_byte(0xC7);
  emit_operand(eax, dst);
  emit_word(imm16);
}

// Move with zero/sign extension.
void BinaryAssembler::movsxb(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x0F);
  emit_byte(0xBE);
  emit_operand(dst, src);
}

void BinaryAssembler::movsxb(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  GUARANTEE(is_valid_byte_register(src), "Must have byte register"); 
  emit_byte(0x0F);
  emit_byte(0xBE);
  emit_byte(0xC0 | (dst << 3) | src);
}

void BinaryAssembler::movsxw(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x0F);
  emit_byte(0xBF);
  emit_operand(dst, src);
}

void BinaryAssembler::movsxw(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x0F);
  emit_byte(0xBF);
  emit_byte(0xC0 | (dst << 3) | src);
}
  
void BinaryAssembler::movzxb(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x0F);
  emit_byte(0xB6);
  emit_operand(dst, src);
}

void BinaryAssembler::movzxb(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  GUARANTEE(is_valid_byte_register(src), "Must have byte register"); 
  emit_byte(0x0F);
  emit_byte(0xB6);
  emit_byte(0xC0 | (dst << 3) | src);
}

void BinaryAssembler::movzxw(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x0F);
  emit_byte(0xB7);
  emit_operand(dst, src);
}
  
void BinaryAssembler::movzxw(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x0F);
  emit_byte(0xB7);
  emit_byte(0xC0 | (dst << 3) | src);
}

// Exchange instruction.
void BinaryAssembler::xchgl(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x87);
  emit_operand(dst, src);
}  

// Arithmetic instructions.
void BinaryAssembler::addl(const Address& dst, int imm32) {
  DisassemblerInfo print_me(this);
  if (is_unsigned_byte(imm32)) {
    emit_byte(0x83);
    emit_operand(eax, dst);
    emit_byte(imm32 & 0xFF);
  } else {
    emit_byte(0x81);
    emit_operand(eax, dst);
    emit_long(imm32);
  }
}

void BinaryAssembler::addl(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x01);
  emit_operand(src, dst);
}

void BinaryAssembler::addl(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x03);
  emit_operand(dst, src);
}

void BinaryAssembler::adcl(const Address& dst, int imm32) {
  DisassemblerInfo print_me(this);
  if (is_unsigned_byte(imm32)) {
    emit_byte(0x83);
    emit_operand(edx, dst);
    emit_byte(imm32 & 0xFF);
  } else {
    emit_byte(0x81);
    emit_operand(edx, dst);
    emit_long(imm32);
  }
}

void BinaryAssembler::adcl(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x13);
  emit_operand(src, dst);
}

void BinaryAssembler::adcl(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x13);
  emit_operand(dst, src);
}

void BinaryAssembler::imull(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x0F);
  emit_byte(0xAF);
  emit_byte(0xC0 | dst << 3 | src);
}

void BinaryAssembler::imull(Register dst, Register src, int imm32) {
  DisassemblerInfo print_me(this);
  if (is_signed_byte(imm32)) {
    emit_byte(0x6B);
    emit_byte(0xC0 | dst << 3 | src);
    emit_byte(imm32);
  } else {
    emit_byte(0x69);
    emit_byte(0xC0 | dst << 3 | src);
    emit_long(imm32);
  }  
}

void BinaryAssembler::imull(Register dst, int imm32) {
  imull(dst, dst, imm32);
}

void BinaryAssembler::imull(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x0F);
  emit_byte(0xAF);
  emit_operand(dst, src);
}

void BinaryAssembler::idivl(Register divisor) {
  DisassemblerInfo print_me(this);
  emit_byte(0xF7);
  emit_operand(edi, divisor);
}

void BinaryAssembler::subl(const Address& dst, int imm32) {
  DisassemblerInfo print_me(this);
  if (is_unsigned_byte(imm32)) {
    emit_byte(0x83);
    emit_operand(ebp, dst);
    emit_byte(imm32 & 0xFF);
  } else {
    emit_byte(0x81);
    emit_operand(ebp, dst);
    emit_long(imm32);
  }
}

void BinaryAssembler::subl(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x29);
  emit_operand(src, dst);
}

void BinaryAssembler::subl(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x2B);
  emit_operand(dst, src);
}
void BinaryAssembler::sbbl(Register dst, int imm32) {
  DisassemblerInfo print_me(this);
  emit_arith(0x81, 0xD8, dst, imm32);
}

void BinaryAssembler::sbbl(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_arith(0x1B, 0xC0, dst, src);
}

void BinaryAssembler::negl(const Address& dst) {
  DisassemblerInfo print_me(this);
  emit_byte(0xF7);
  emit_operand(ebx, dst);
}

void BinaryAssembler::negl(Register dst) {
  DisassemblerInfo print_me(this);
  emit_byte(0xF7);
  emit_byte(0xD8 | dst);
}

void BinaryAssembler::incl(Register dst) {
  DisassemblerInfo print_me(this);
  emit_byte(0x40 | dst);
}

void BinaryAssembler::incl(const Address& dst) {
  DisassemblerInfo print_me(this);
  emit_byte(0xFF);
  emit_operand(eax, dst);
}

void BinaryAssembler::decl(Register dst) {
  DisassemblerInfo print_me(this);
  emit_byte(0x48 | dst);
}

void BinaryAssembler::cdql() {
  DisassemblerInfo print_me(this);
  emit_byte(0x99);
} 

// Logical operations.
void BinaryAssembler::andl(const Address& dst, int imm32) {
  DisassemblerInfo print_me(this);
  if (is_unsigned_byte(imm32)) {
    emit_byte(0x83);
    emit_operand(esp, dst);
    emit_byte(imm32 & 0xFF);
  } else {
    emit_byte(0x81);
    emit_operand(esp, dst);
    emit_long(imm32);
  }
}

void BinaryAssembler::andl(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x21);
  emit_operand(src, dst);
}

void BinaryAssembler::andl(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x23);
  emit_operand(dst, src);
}

void BinaryAssembler::orl(const Address& dst, int imm32) {
  DisassemblerInfo print_me(this);
  if (is_unsigned_byte(imm32)) {
    emit_byte(0x83);
    emit_operand(ecx, dst);
    emit_byte(imm32 & 0xFF);
  } else {
    emit_byte(0x81);
    emit_operand(ecx, dst);
    emit_long(imm32);
  }
}

void BinaryAssembler::orl(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x09);
  emit_operand(src, dst);
}

void BinaryAssembler::orl(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x0B);
  emit_operand(dst, src);
}
void BinaryAssembler::xorl(const Address& dst, int imm32) {
  DisassemblerInfo print_me(this);
  if (is_unsigned_byte(imm32)) {
    emit_byte(0x83);
    emit_operand(esi, dst);
    emit_byte(imm32 & 0xFF);
  } else {
    emit_byte(0x81);
    emit_operand(esi, dst);
    emit_long(imm32);
  }
}

void BinaryAssembler::xorl(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x31);
  emit_operand(src, dst);
}

void BinaryAssembler::xorl(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x33);
  emit_operand(dst, src);
}

void BinaryAssembler::notl(Register dst) {
  DisassemblerInfo print_me(this);
  emit_byte(0xF7);
  emit_byte(0xD0 | dst);
}

// Bit vector operations

void BinaryAssembler::bts(const Address& bit_base, Register bit_offset) {
  DisassemblerInfo print_me(this);
  emit_byte(0x0F);
  emit_byte(0xAB);
  emit_operand(bit_offset, bit_base);
}

// Stack manipulation operations.
void BinaryAssembler::pushal() {
  DisassemblerInfo print_me(this);
  emit_byte(0x60);
}

void BinaryAssembler::pushl(Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x50 | src);  
}

void BinaryAssembler::pushl(const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0xFF);
  emit_operand(esi, src);  
}

void BinaryAssembler::pushl(int imm32) {
  DisassemblerInfo print_me(this);
  if (is_signed_byte(imm32)) {
    emit_byte(0x6A);
    emit_byte(imm32);
  } else {
    emit_byte(0x68);
    emit_long(imm32);  
  }
}

#if NOT_CURRENTLY_USED

void BinaryAssembler::pushl(Oop* oop) {
  DisassemblerInfo print_me(this);
  emit_byte(0x68);
  emit_oop(oop->obj());
  emit_long((int) oop->obj());  
}

#endif

void BinaryAssembler::popal() {
  DisassemblerInfo print_me(this);
  emit_byte(0x61);
}

void BinaryAssembler::popl(Register dst) {
  DisassemblerInfo print_me(this);
  emit_byte(0x58 | dst);  
}

void BinaryAssembler::popl(const Address& dst) {
  DisassemblerInfo print_me(this);
  emit_byte(0x8F);
  emit_operand(eax, dst);
}

// Load effective address instructions.
void BinaryAssembler::leal(Register dst, const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x8D);
  emit_operand(dst, src);
}

// Test instructions..
void BinaryAssembler::testb(Register dst, int imm8) {
  DisassemblerInfo print_me(this);
  emit_arith_b(0xF6, 0xC0, dst, imm8);
}

void BinaryAssembler::testl(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_arith(0x85, 0xC0, dst, src);
}

void BinaryAssembler::testl(Register dst, int imm32) {
  DisassemblerInfo print_me(this);
  if (dst == eax) {
    emit_byte(0xA9);
  } else {
    emit_byte(0xF7);
    emit_byte(0xC0 | dst);
  }
  emit_long(imm32);
}

void BinaryAssembler::testl(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x85);
  emit_operand(src, dst);
}

void BinaryAssembler::testl(const Address& dst, int imm32) {
  DisassemblerInfo print_me(this);
  emit_byte(0xF7);
  emit_operand(eax, dst);
  emit_long(imm32);
}

// Compare instructions.
void BinaryAssembler::cmpl(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_arith(0x3B, 0xC0, dst, src);
}

void BinaryAssembler::cmpl(Register dst, int imm32) {
  DisassemblerInfo print_me(this);
  emit_arith(0x81, 0xF8, dst, imm32);
}

void BinaryAssembler::cmpl(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  emit_byte(0x39);
  emit_operand(src, dst);
}

void BinaryAssembler::cmpl(const Address& dst, int imm32) {
  DisassemblerInfo print_me(this);
  emit_byte(0x81);
  emit_operand(edi, dst);
  emit_long(imm32);
}

// Shift instructions.
void BinaryAssembler::shll(Register dst, int imm8) {
  DisassemblerInfo print_me(this);
  GUARANTEE(1 <= imm8 && imm8 <= 32, "Illegal shift count");
  if (imm8 == 1) {
    emit_byte(0xD1);
    emit_byte(0xE0 | dst);
  } else {
    emit_byte(0xC1);
    emit_byte(0xE0 | dst);
    emit_byte(imm8);
  }
}

void BinaryAssembler::shll(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  GUARANTEE(src == ecx, "Shift register must be ecx");
  emit_byte(0xD3);
  emit_byte(0xE0 | dst);
}

void BinaryAssembler::shll(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  GUARANTEE(src == ecx, "Shift register must be ecx");
  emit_byte(0xD3);
  emit_operand(esp, dst);
}

void BinaryAssembler::shrl(Register dst, int imm8) {
  DisassemblerInfo print_me(this);
  GUARANTEE(1 <= imm8 && imm8 <= 32, "Illegal shift count");
  if (imm8 == 1) {
    emit_byte(0xD1);
    emit_byte(0xE8 | dst);
  } else {
    emit_byte(0xC1);
    emit_byte(0xE8 | dst);
    emit_byte(imm8);
  }
}

void BinaryAssembler::shrl(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  GUARANTEE(src == ecx, "Shift register must be ecx");
  emit_byte(0xD3);
  emit_byte(0xE8 | dst);
}

void BinaryAssembler::shrl(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  GUARANTEE(src == ecx, "Shift register must be ecx");
  emit_byte(0xD3);
  emit_operand(ebp, dst);
}

void BinaryAssembler::sarl(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  GUARANTEE(src == ecx, "Shift register must be ecx");
  emit_byte(0xD3);
  emit_byte(0xF8 | dst);
}

void BinaryAssembler::sarl(const Address& dst, Register src) {
  DisassemblerInfo print_me(this);
  GUARANTEE(src == ecx, "Shift register must be ecx");
  emit_byte(0xD3);
  emit_operand(edi, dst);
}

void BinaryAssembler::sarl(Register dst, int imm8) {
  DisassemblerInfo print_me(this);
  GUARANTEE(1 <= imm8 && imm8 <= 32, "Illegal shift count");
  if (imm8 == 1) {
    emit_byte(0xD1);
    emit_byte(0xF8 | dst);
  } else {
    emit_byte(0xC1);
    emit_byte(0xF8 | dst);
    emit_byte(imm8);
  }
}

// Jump instructions.
void BinaryAssembler::jmp(Label& L) {
  DisassemblerInfo print_me(this);
  if (L.is_bound()) {
    // If it's a backwards branch, see if we can use the short form
    const int short_length = 2;  // opcode + offset
    int short_offset = L.position() - (_code_offset + short_length);
    if (is_signed_byte(short_offset)) {
      emit_byte(0xEB); 
      emit_byte(short_offset);
      return;
    }
  }
  emit_byte(0xE9);
  emit_displacement(L);
}

void BinaryAssembler::jmp(NearLabel& L) {
  DisassemblerInfo print_me(this);
  emit_byte(0xEB);
  emit_displacement(L);
}

void BinaryAssembler::jmp(CompilationQueueElement* cqe) { 
  Label target = cqe->entry_label();
  jmp(target);
  cqe->set_entry_label(target);
}

// Conditional jump.
void BinaryAssembler::jcc(Condition condition, Label& L) {
  DisassemblerInfo print_me(this);
  GUARANTEE((0 <= condition) && (condition < 16), "Illegal condition");
  if (L.is_bound()) {
    const int short_length = 2;  // opcode + offset
    int short_offset = L.position() - (_code_offset + short_length);
    if (is_signed_byte(short_offset)) {
      emit_byte(0x70 | condition); 
      emit_byte(short_offset);
      return;
    }
  }
  emit_byte(0x0F);
  emit_byte(0x80 | condition);
  emit_displacement(L);
}

void BinaryAssembler::jcc(Condition condition, NearLabel& L) {
  DisassemblerInfo print_me(this);
  GUARANTEE((0 <= condition) && (condition < 16), "Illegal condition");
  emit_byte(0x70 | condition); 
  emit_displacement(L);
}

void BinaryAssembler::jcc(Condition condition, CompilationQueueElement* cqe) { 
  Label target = cqe->entry_label();
  jcc(condition, target);
  cqe->set_entry_label(target);
}


#if NOT_CURRENTLY_USED
void BinaryAssembler::jcc(Condition condition, address entry) {
  DisassemblerInfo print_me(this);
  GUARANTEE((0 <= condition) && (condition < 16), "Illegal condition");
  emit_byte(0x0F);
  emit_byte(0x80 | condition);
  int offs = (int) entry - (_code_offset + sizeof(int) + (int) compiled_method()->entry());
  _relocation.emit(Relocation::compiler_stub_type, _code_offset);
  emit_long(offs);
}
#endif

void BinaryAssembler::call_jmp(Register reg, int opcode2) {
  DisassemblerInfo print_me(this);
  emit_byte(0xFF);
  emit_byte(opcode2 | reg);
}

void BinaryAssembler::call_jmp(address entry, int opcode) {
  DisassemblerInfo print_me(this);
  emit_byte(opcode);
  int offs = (int) entry - 
          (_code_offset + sizeof(int) + (int) compiled_method()->entry());
  emit_relocation(Relocation::compiler_stub_type);
  emit_long(offs);
}

// Floating-point operations.
void BinaryAssembler::fld(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map();
  if( fpu_map.is_on_stack(dst) ) {
    fpop(dst);
  }

  emit_farith(0xD9, 0xC0, fpu_map.index_for(src));
  fpu_map.push(dst);
}

void BinaryAssembler::fstp(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map();
  GUARANTEE(fpu_map.is_top_of_stack(src), 
            "Source register must be on top of FPU stack");
  GUARANTEE(!fpu_map.is_top_of_stack(dst), 
            "Destination register must not be on top of FPU stack");
  emit_farith(0xDD, 0xD8, fpu_map.index_for(src));
  fpu_map.pop(src);
}

void BinaryAssembler::fld1(Register dst) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  if( fpu_map.is_on_stack(dst) ) {
    fpop(dst);
  }

  emit_byte(0xD9);
  emit_byte(0xE8);
  fpu_map.push(dst);
}

void BinaryAssembler::fldz(Register dst) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  if( fpu_map.is_on_stack(dst)) {
    fpop(dst);
  }

  emit_byte(0xD9);
  emit_byte(0xEE);
  fpu_map.push(dst);
}

void BinaryAssembler::fld_s(Register dst, const Address& adr) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  if( fpu_map.is_on_stack(dst) ) {
    fpop(dst);
  }

  emit_byte(0xD9);
  emit_operand(eax, adr);
  fpu_map.push(dst);
}

void BinaryAssembler::fld_d(Register dst, const Address& adr) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  if( fpu_map.is_on_stack(dst) ) {
    fpop(dst);
  }

  emit_byte(0xDD);
  emit_operand(eax, adr);
  fpu_map.push(dst);
}

void BinaryAssembler::fstp_s(const Address& adr, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack(src),
            "Source register must be on top of FPU stack");
  emit_byte(0xD9);
  emit_operand(ebx, adr);
  fpu_map.pop(src);
}

void BinaryAssembler::fstp_d(const Address& adr, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE(fpu_map.is_top_of_stack(src), 
            "Source register must be on top of FPU stack");
  emit_byte(0xDD);
  emit_operand(ebx, adr);
  fpu_map.pop(src);
}

void BinaryAssembler::fild_s(Register dst, const Address& adr) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  if( fpu_map.is_on_stack(dst) ) {
    fpop(dst);
  }

  emit_byte(0xDB);
  emit_operand(eax, adr);
  fpu_map.push(dst);
}

void BinaryAssembler::fild_d(Register dst, const Address& adr) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  if( fpu_map.is_on_stack(dst) ) {
    fpop(dst);
  }

  emit_byte(0xDF);
  emit_operand(ebp, adr);
  fpu_map.push(dst);
}

void BinaryAssembler::fistp_s(const Address& adr, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack(src), 
             "Source register must be on top of FPU stack");
  emit_byte(0xDB);
  emit_operand(ebx, adr);
  fpu_map.pop(src);
}

void BinaryAssembler::fistp_d(const Address& adr, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack( src ), 
             "Source register must be on top of FPU stack");
  emit_byte(0xDF);
  emit_operand(edi, adr);
  fpu_map.pop(src);
}

void BinaryAssembler::faddp(Register dst, Register src) {
  DisassemblerInfo print_me(this); 
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack(src), 
             "Source register must be on top of FPU stack");
  GUARANTEE(!fpu_map.is_top_of_stack(dst), 
             "Destination register must not be on top of FPU stack");
  emit_farith(0xDE, 0xC0, fpu_map.index_for(dst));
  fpu_map.pop(src);
}

void BinaryAssembler::fsubp(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack(src), 
             "Source register must be on top of FPU stack");
  GUARANTEE(!fpu_map.is_top_of_stack(dst),
             "Destination register must not be on top of FPU stack");
  emit_farith(0xDE, 0xE8, fpu_map.index_for(dst));
  fpu_map.pop(src);
}

void BinaryAssembler::fsubrp(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack(src), 
             "Source register must be on top of FPU stack");
  GUARANTEE( !fpu_map.is_top_of_stack(dst),
             "Destination register must not be on top of FPU stack");
  emit_farith(0xDE, 0xE0, fpu_map.index_for(dst));
  fpu_map.pop(src);
}

void BinaryAssembler::fmulp(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack(src), 
             "Source register must be on top of FPU stack");
  GUARANTEE( !fpu_map.is_top_of_stack(dst),
             "Destination register must not be on top of FPU stack");
  emit_farith(0xDE, 0xC8, fpu_map.index_for(dst));
  fpu_map.pop(src);
}

void BinaryAssembler::fdivp(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack(src), 
             "Source register must be on top of FPU stack" );
  GUARANTEE( !fpu_map.is_top_of_stack(dst),
             "Destination register must not be on top of FPU stack");
  emit_farith(0xDE, 0xF8, fpu_map.index_for(dst));
  fpu_map.pop(src);
}

void BinaryAssembler::fdivrp(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack(src), 
             "Source register must be on top of FPU stack" );
  GUARANTEE(!fpu_map.is_top_of_stack(dst),
             "Destination register must not be on top of FPU stack");
  emit_farith( 0xDE, 0xF0, fpu_map.index_for(dst) );
  fpu_map.pop(src);
}

void BinaryAssembler::fremp(Register src, Register dst) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  bool must_save_eax = Compiler::frame()->is_mapping_something(eax);

  GUARANTEE(fpu_map.is_top_of_stack(src),
            "Destination register must be on top of FPU stack");
  GUARANTEE(fpu_map.index_for(dst) == 1, 
  "Source register must be directly below top of FPU stack");

  if (must_save_eax) pushl(eax);
  NearLabel L;
  bind(L);
  fprem();
  fstsw_ax();
  sahf();
  jcc(parity, L);
  fstp(dst, src);
  if (must_save_eax) popl(eax);
}

void BinaryAssembler::fprem() {
  UsingFastOops fast_oops;
  DisassemblerInfo print_me(this);
  emit_byte(0xD9);
  emit_byte(0xF8);
}

void BinaryAssembler::fchs(Register dst) {
  DisassemblerInfo print_me(this);
  const FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack(dst),
             "Destination register must be on top of FPU stack");
  emit_byte(0xD9);
  emit_byte(0xE0);
}

void BinaryAssembler::fabs(Register dst) {
  DisassemblerInfo print_me(this);
  const FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack(dst),
             "Destination register must be on top of FPU stack");
  emit_byte(0xD9);
  emit_byte(0xE1);
}

void BinaryAssembler::fxch(Register dst) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE(!fpu_map.is_top_of_stack(dst),
            "Destination register must not be on top of FPU stack");
  emit_farith(0xD9, 0xC8, fpu_map.swap_with_top(dst));
}

void BinaryAssembler::fpop(Register dst) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  if( !fpu_map.is_top_of_stack(dst) ) {
    fxch(dst);
  }
  ffree(dst);
  emit_byte(0xD9); // fincstp
  emit_byte(0xF7);
  fpu_map.pop(dst);
}

void BinaryAssembler::ffree(Register dst) {
  DisassemblerInfo print_me(this);
  const FPURegisterMap& fpu_map = fpu_register_map(); 
  emit_farith( 0xDD, 0xC0, fpu_map.index_for(dst) );
}

void BinaryAssembler::fucomip(Register dst, Register src) {
  DisassemblerInfo print_me(this);
  FPURegisterMap& fpu_map = fpu_register_map(); 
  GUARANTEE( fpu_map.is_top_of_stack(src), 
             "Source register must be on top of FPU stack");
  GUARANTEE( !fpu_map.is_top_of_stack(dst),
             "Destination register must not be on top of FPU stack");
  emit_farith( 0xDF, 0xE8, fpu_map.index_for( dst ) );
  fpu_map.pop(src);
}

void BinaryAssembler::fwait() {
  UsingFastOops fast_oops;
  DisassemblerInfo print_me(this);
  emit_byte(0x9B);
}

void BinaryAssembler::finit() {
  DisassemblerInfo print_me(this);
  emit_byte(0x9B); // fwait
  emit_byte(0xDB); // finit
  emit_byte(0xE3);
  fpu_register_map().reset();
}

void BinaryAssembler::fstsw_ax() {
  UsingFastOops fast_oops;
  DisassemblerInfo print_me(this);
  emit_byte(0x9B); // fwait
  emit_byte(0xDF); // fnstsw_ax
  emit_byte(0xE0);
}

// Miscellaneous instructions.
void BinaryAssembler::sahf() {
  DisassemblerInfo print_me(this);
  emit_byte(0x9E);
}

void BinaryAssembler::int3() {
  DisassemblerInfo print_me(this);
  emit_byte(0xCC);
}

void BinaryAssembler::nop() {
  DisassemblerInfo print_me(this);
  emit_byte(0x90);
}

void BinaryAssembler::ret(int k) {
  DisassemblerInfo print_me(this);
  if (k == 0) {
    emit_byte(0xC3);
  } else {
    emit_byte(0xC2);
    emit_word(k);
  }
}

void BinaryAssembler::hlt() {
  DisassemblerInfo print_me(this);
  emit_byte(0xF4);
}

// Instruction only used for debugging/printing

#ifndef PRODUCT

void BinaryAssembler::pushad() {
  DisassemblerInfo print_me(this);
  emit_byte(0x60);
}

void BinaryAssembler::popad() {
  DisassemblerInfo print_me(this);
  emit_byte(0x61);
}

void BinaryAssembler::pushfd() {
  DisassemblerInfo print_me(this);
  emit_byte(0x9C);
}

void BinaryAssembler::popfd() {
  DisassemblerInfo print_me(this);
  emit_byte(0x9D);
}

void BinaryAssembler::fnsave(const Address& dst) {
  DisassemblerInfo print_me(this);
  emit_byte(0xDD);
  emit_operand(esi, dst);
}

void BinaryAssembler::frstor(const Address& src) {
  DisassemblerInfo print_me(this);
  emit_byte(0xDD);
  emit_operand(esp, src);
}

#endif // PRODUCT

// Helpers routines
void  BinaryAssembler::emit_displacement(Label& L) {
  int pos = _code_offset;
  if (L.is_unused()) {
    // This make it look like a branch to itself
    emit_long(-4);
  } else {
    emit_long(L.position() - (pos + 4));
  }
  if (!L.is_bound()) {
    L.link_to(pos);
  }
}

void  BinaryAssembler::emit_displacement(NearLabel& L) {
  int pos = _code_offset;
  if (L.is_unused()) {
    // This makes it look like a branch to itself
    emit_byte(-1);
  } else {
    emit_byte(L.position() - (pos + 1));
  }

  if (!L.is_bound()) {
    L.link_to(pos);
  }

}

void BinaryAssembler::bind(Label& L, int alignment) {
  if (alignment > 0) {
    int adjustment = code_size() % alignment;
    if (adjustment > 0) {
      for (int i = 0; i < alignment - adjustment; i++) nop();
    }
  }
  bind_to(L, _code_offset);
}

void BinaryAssembler::bind_to(Label& L, jint code_offset) {
  // if the code has overflowed the compiled method, we
  // cannot expect to be able to follow the link chain.
  if (L.is_unbound() && !has_overflown_compiled_method()) {
    int p = L.position();
    int q;
    do {
      // Modify the instruction at offset q.  Set p to next offset
      q = p;
      p = q + 4 + long_at(q);
      GUARANTEE(p <= q, "Offsets must be decreasing");
      long_at_put(q, code_offset - (q + 4));
    } while (p != q);
  }
  L.bind_to(code_offset);
}

void BinaryAssembler::bind_to(NearLabel& L, jint code_offset) {
  // if the code has overflowed the compiled method, we
  // cannot expect to be able to follow the link chain.
  if (L.is_unbound() && !has_overflown_compiled_method()) {
    int p = L.position();
    int q;
    do {
      // Modify the instruction at offset q.  Set p to next offset
      q = p;
      p = q + 1 + byte_at(q);
      GUARANTEE(p <= q, "Offsets must be decreasing");
      GUARANTEE(is_signed_byte(code_offset - (q + 1)),
                "Offset must fit in byte");
      byte_at_put(q, code_offset - (q + 1));
    } while (p != q);
  }
  L.bind_to(code_offset);
}

void BinaryAssembler::get_thread(Register dst) {
  movl(dst, Address((int) &_current_thread));
}

void BinaryAssembler::emit_operand(Register reg, Register base, Register index,
                                   const ScaleFactor scale, int disp,
                                   Relocation::Kind reloc) { 
  if (is_valid_int_register(base)) {
    if (is_valid_int_register(index)) {
      GUARANTEE(scale != no_scale, "inconsistent address");
      // [base + index*scale + disp]
      if (disp == 0 && reloc == Relocation::no_relocation) {
        // [base + index*scale]
        // [00 reg 100][ss index base]
        GUARANTEE(index != esp && base != ebp, "illegal addressing mode");
        emit_byte(0x04 | reg << 3);
        emit_byte(scale << 6 | index << 3 | base);
      } else if (is_signed_byte(disp) && reloc == Relocation::no_relocation) {
        // [base + index*scale + imm8]
        // [01 reg 100][ss index base] imm8
        GUARANTEE(index != esp, "illegal addressing mode");
        emit_byte(0x44 | reg << 3);
        emit_byte(scale << 6 | index << 3 | base);
        emit_byte(disp & 0xFF);
      } else {
        // [base + index*scale + imm32]
        // [10 reg 100][ss index base] imm32
    GUARANTEE(index != esp, "illegal addressing mode");
        emit_byte(0x84 | reg << 3);
        emit_byte(scale << 6 | index << 3 | base);
    emit_data(disp, reloc);
      }
    } else if (base == esp) {
      // [esp + disp]
      if (disp == 0 && reloc == Relocation::no_relocation) {
        // [esp]
        // [00 reg 100][00 100 100]
        emit_byte(0x04 | reg << 3);
        emit_byte(0x24);
      } else if (is_signed_byte(disp) && reloc == Relocation::no_relocation) {
        // [esp + imm8]
        // [01 reg 100][00 100 100] imm8
        emit_byte(0x44 | reg << 3);
        emit_byte(0x24);
        emit_byte(disp & 0xFF);
      } else {
        // [esp + imm32]
        // [10 reg 100][00 100 100] imm32
        emit_byte(0x84 | reg << 3);
        emit_byte(0x24);
        emit_data(disp, reloc);
      }
    } else {
      // [base + disp]
      GUARANTEE(base != esp, "illegal addressing mode");
      if (disp == 0 && reloc == Relocation::no_relocation && base != ebp) {
        // [base]
        // [00 reg base]
        GUARANTEE(base != ebp, "illegal addressing mode");
        emit_byte(0x00 | reg << 3 | base);
      } else if (is_signed_byte(disp) && reloc == Relocation::no_relocation) {
        // [base + imm8]
        // [01 reg base] imm8
        emit_byte(0x40 | reg << 3 | base);
        emit_byte(disp & 0xFF);
      } else {
        // [base + imm32]
        // [10 reg base] imm32
        emit_byte(0x80 | reg << 3 | base);
        emit_data(disp, reloc);
      }
    }
  } else {
    if (is_valid_int_register(index)) {
      GUARANTEE(scale != no_scale, "inconsistent address");
      // [index*scale + disp]
      // [00 reg 100][ss index 101] imm32
      GUARANTEE(index != esp, "illegal addressing mode");
      emit_byte(0x04 | reg << 3);
      emit_byte(scale << 6 | index << 3 | 0x05);
      emit_data(disp, reloc);
    } else {
      // [disp]
      // [00 reg 101] imm32
      emit_byte(0x05 | reg << 3);
      emit_data(disp, reloc);
    }
  }
}

inline void BinaryAssembler::emit_operand(Register reg, const Address& adr,
                                          Relocation::Kind reloc) { 
  emit_operand(reg, adr._base, adr._index, adr._scale, adr._disp, reloc);
}

void BinaryAssembler::emit_arith(int op1, int op2, Register dst, int imm32) {
  DisassemblerInfo print_me(this);

  GUARANTEE(is_unsigned_byte(op1) && is_unsigned_byte(op2), "Wrong opcode");
  GUARANTEE((op1 & 0x01) == 1, "Should be 32-bit operation");
  GUARANTEE((op1 & 0x02) == 0, "Sign-extension bit should not be set");

  if (is_signed_byte(imm32)) {
    emit_byte(op1 | 0x02); // Set sign bit
    emit_byte(op2 | dst);
    emit_byte(imm32 & 0xFF);
  } else {
    emit_byte(op1);
    emit_byte(op2 | dst);
    emit_long(imm32);
  }
}

void BinaryAssembler::emit_arith(int op1, int op2, Register dst, Register src) {
  DisassemblerInfo print_me(this);
  GUARANTEE(is_unsigned_byte(op1) && is_unsigned_byte(op2), "Wrong opcode");
  emit_byte(op1);
  emit_byte(op2 | dst << 3 | src);
}

void BinaryAssembler::emit_arith_b(int op1, int op2, Register dst, int imm8) {
  DisassemblerInfo print_me(this);
  GUARANTEE(is_valid_byte_register(dst), "Must have byte register");
  GUARANTEE(is_unsigned_byte(op1) && is_unsigned_byte(op2), "Wrong opcode");
  GUARANTEE(is_unsigned_byte(imm8), "Not a byte");
  GUARANTEE((op1 & 0x01) == 0, "Should be 8-bit operation");
  emit_byte(op1);
  emit_byte(op2 | dst);
  emit_byte(imm8);
}

void BinaryAssembler::emit_farith(int op1, int op2, int stack_offset) {
  DisassemblerInfo print_me(this);
  GUARANTEE(is_unsigned_byte(op1) && is_unsigned_byte(op2), "Wrong opcode");
  GUARANTEE(0 <= stack_offset && stack_offset < 8, "Illegal fpu stack offset");
  emit_byte(op1);
  emit_byte(op2 + stack_offset);
}

void BinaryAssembler::emit_data(int data, Relocation::Kind reloc) {
  // We should add support for relocation here.
  if (reloc != Relocation::no_relocation) {
    // Emit relocation information
    emit_relocation(reloc);
  }
  emit_long(data);
}

#endif
