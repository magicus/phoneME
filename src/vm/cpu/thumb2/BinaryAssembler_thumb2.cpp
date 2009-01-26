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
# include "incls/_BinaryAssembler_thumb2.cpp.incl"

#if ENABLE_COMPILER

void BinaryAssembler::branch_helper(Label& L,
                                    const bool link,
                                    const bool is_near,
                                    const Condition cond)
{
  GUARANTEE( !link, "bl is not implemented yet" );

  const int pos = _code_offset;
  const int label_pos = L.position();

  // If the label is not bound, we emit a branch to itself.
  const int offset = L.is_unused() ? 0 : (label_pos - pos);

  GUARANTEE(offset <= 0, "Bound labels must be backward");

  // Emit 16-bit branch only if the label is near or is bound
  // within the limits for 16-bit branch.
  const bool short_branch = is_near ||
    (L.is_bound() && ((offset >= -256 + 4) ||
                      (cond == al && offset >= -2048 + 4)));
#ifndef PRODUCT
  if (!L.is_bound()) {
    Disassembler::set_reference_to_unresolved_label();
  }
#endif
  // Emit branch to itself.
  if (short_branch) {
    b_short(offset, cond);
  } else {
    b_w(offset, cond);
  }

  if (!L.is_bound()) {
    L.link_to(pos);
  }
}

void BinaryAssembler::b(CompilationQueueElement* cqe, const Condition cond){
  Label target = cqe->entry_label();
  branch_helper(target, false, false, cond);
  cqe->set_entry_label(target);
}

void BinaryAssembler::bind(Label& L, int alignment) {
  bind_to(L, _code_offset);
}

void BinaryAssembler::bind_to(Label& L, jint code_offset) {
  // if the code has overflowed the compiled method, we
  // cannot expect to be able to follow the link chain.
  if (L.is_linked() && !has_overflown_compiled_method()) {
    // follow the chain and fixup all instructions -
    // last instruction in chain is referring to itself
    if (PrintCompiledCodeAsYouGo) {
      TTY_TRACE_CR(("**Patching %d to %d", L.position(), code_offset));
    }
    address p = addr_at(L.position());
    address q;

    do {
      q = p;

      const Instruction instr(q);
      switch( instr.kind() ) {
        case Instruction::vfp: {
#if USE_ARM_VFP_LITERALS
          const VFPMemAccess m(q);
          p = m.location();
          m.set_location(addr_at(code_offset));
#else
#ifndef PRODUCT
          if (PrintCompiledCodeAsYouGo) {
            int offset = q - addr_at(0);
            Disassembler d(tty);
            tty->print("**%d:\t", offset);
            tty->print("0x%08x\t", instr.addr());
            d.disasm((short*)q, *(short *)q, offset);
            tty->cr();
            tty->flush();
          }
#endif
          SHOULD_NOT_REACH_HERE();
#endif
          break;
        }
        case Instruction::ldr: {
          const MemAccess m(q);
          p = m.location();
          m.set_location(addr_at(code_offset));
          break;
        }
        default: { // branch, branch_link
          const Branch b(q);
          p = b.target();
          b.set_target(addr_at(code_offset));
          break;
        }
      }
    } while (p != q);
  }
  L.bind_to(code_offset);
}

void BinaryAssembler::access_literal_pool(Register rd,
                                          LiteralPoolElement* literal,
                                          const Condition cond,
                                          const bool is_store) {
  Label L = literal->label();
  const int pos = _code_offset;
  const int target = L.is_unused() ? pos : L.position();
  int offset = target - pos;

  // Generate instruction that loads from the target
  if (is_store) {
    str(rd, pc, offset, cond);
  } else {
    it(cond);
    // Fake load since this allows us to keep -ve offset
    // which is needed during literal chaining. We need
    // to fake because thumb doesn't have a load instruction
    // that can take a -ve offset
#ifndef PRODUCT
    if (GenerateCompilerComments) {
      static char buff[64];
      jvm_sprintf(buff, "load literal, offset=%d", offset);
      Disassembler::eol_comment(buff);
    }
#endif
    if( L.is_bound() ) {
      offset = target - ((pos + 4) & 0xFFFFFFFC);
      GUARANTEE( abs(offset) < 0x1000, "Invalid offset");
#if USE_ARM_VFP_LITERALS
      if( rd > Assembler::r15 ) {
        flds( rd, imm_index5_8x4(pc, offset / 4) );
      } else
#endif
        ldr_pc_imm12_w( rd, offset );
    } else {
      offset /= 2;
      GUARANTEE( abs(offset) < 0x1000, "Invalid offset");
#ifndef PRODUCT
      Disassembler::set_reference_to_unresolved_label();
#endif
#if USE_ARM_VFP_LITERALS
      if( rd > Assembler::r15 ) {
        flds_stub( rd, offset );
      } else
#endif
        ldr_pc_stub( rd, offset );
    }
  }
  if( !L.is_bound() ) {
    L.link_to(pos);
  }
  literal->set_label(L);
}

void BinaryAssembler::ldr_literal(Register rd, OopDesc* obj, const int imm32,
                                  const Condition cond) {
  SETUP_ERROR_CHECKER_ARG;
  LiteralPoolElement* e = find_literal(obj, imm32 JVM_ZCHECK(e) );
  ldr_from(rd, e, cond);
}

void BinaryAssembler::ldr_oop(Register rd, Oop* oop, const Condition cond) {
  if (oop->is_null()) {
    ldr_address(rd, 0, cond);
    return;
  }
#if USE_AOT_COMPILATION
  /*
   * Try to avoid direct reference from AOT-compiled method to Java heap.
   * This allows AOT-compiled methods to stay in TEXT area of ROMImage.cpp.
   */
  if (GenerateROMImage && !ROM::system_contains(oop->obj())) {
    bool load_class = false;
    bool load_near = false;

    if (oop->is_java_class()) {
      load_class = true;
    }
    else if (oop->is_java_near()) {
      JavaClass::Raw klass = oop->klass();
      if (oop->equals(klass().prototypical_near())) {
        load_class = true;
        load_near = true;
      }
    }

    if (load_class) {
      JavaClass::Raw klass;
      if (oop->is_java_near()) {
        klass = oop->klass();
      } else {
        klass = oop;
      }

      int class_id = klass().class_id();
      get_class_list_base(rd, cond);
      ldr(rd, rd, class_id * sizeof(OopDesc*), cond);

      if (load_near) {
        ldr(rd, rd, InstanceClass::prototypical_near_offset(), cond);
      }
      return;
    }
  }
#endif

  ldr_literal(rd, oop->obj(), 0, cond);
}

extern "C" {
  extern address gp_constants[], gp_constants_end[];
}

void BinaryAssembler::ldr_address(Register rd, address target, Condition cond) {
  for (address* addr = gp_constants; addr < gp_constants_end; addr++) {
    if (*addr == target) {
      ldr_using_gp(rd, (address)addr, cond);
      return;
    }
  }
  if (GenerateROMImage) {
    GUARANTEE(target != 0, "Must not be null address");
    ldr_literal(rd, compiled_method()->obj(), (int)target, cond);
  } else {
    ldr_literal(rd, NULL, (int)target, cond);
  }
}

void BinaryAssembler::ldr_big_integer(Register rd, int imm32, Condition cond){
  if (has_room_for_imm(imm32, 16)) {
    it(cond);
    movw_imm12_w(rd, imm32);
  } else {
    it(cond, THEN);
    movw_imm12_w(rd, imm32 & 0xFFFF);
    movt_imm12_w(rd, (juint)imm32 >> 16);
  }
}

inline void BinaryAssembler::append_literal( LiteralPoolElement* literal ) {
  if( _first_literal == NULL ) {
    GUARANTEE(_last_literal == NULL, "No literals");
    GUARANTEE(_first_unbound_literal == NULL, "No unknown literals");
    _first_literal = literal;
  } else {
    _last_literal->set_next(literal);

  }
  _last_literal = literal;
  if( _first_unbound_literal == NULL ) {
    // This is the first literal that hasn't yet been written out.
    _first_unbound_literal = literal;
  }
}

LiteralPoolElement*
BinaryAssembler::find_literal(OopDesc* obj, const int imm32 JVM_TRAPS) {
  enum {
    max_ldr_offset = 1 << 10,   // unsigned for Thumb, signed for VFP
    max_code_size_to_branch_around_literals = 8,
    offset = max_ldr_offset - max_code_size_to_branch_around_literals
  };

  LiteralPoolElement* literal = _first_literal;

  // Search for a bound literal not too far from current instruction
  {
    const int min_offset = _code_offset - offset;
    for( ; literal && literal->is_bound(); literal = literal->next() ) {
      if( literal->_bci >= min_offset && literal->matches(obj, imm32)) {
        return literal;
      }
    }
  }

  // Search for an unbound literal
  {
    int unbound_literal_offset = 0;
    for( ; literal; literal = literal->next() ) {
      if( literal->matches(obj, imm32)) {
        if( unbound_literal_offset < offset ) {
          set_delayed_literal_write_threshold(offset - unbound_literal_offset);
        } else {
          branch_around_literals();
        }
        return literal;
      }
      unbound_literal_offset += 4; // literal size in bytes
    }
  }

  // We need to create a new literal.
  literal = LiteralPoolElement::allocate(obj, imm32 JVM_ZCHECK_0(literal) );

  // Add this literal to the end of the literal pool list
  append_literal(literal);

  enum {
    max_unbound_literal_count = 0x100 - 64    // max size of literals per one bytecode
  };

  _unbound_literal_count++;
  if (_unbound_literal_count >= max_unbound_literal_count) {
    // If we get too many literals, their size might not fit into an
    // immediate.  So we force a cutoff.
    _code_offset_to_force_literals = 0; // force at the next chance
    _code_offset_to_desperately_force_literals = 0;
  } else {
    set_delayed_literal_write_threshold(offset);
  }
  return literal;
}

inline void BinaryAssembler::write_literal( LiteralPoolElement* literal ) {
  int position = _code_offset;
  Oop::Raw oop = literal->literal_oop();
  if (oop.is_null()) {
    emit_int(literal->literal_int());
  } else if (GenerateROMImage && literal->literal_int() != 0) {
    GUARANTEE(oop.equals(compiled_method()), "Special flag");
    emit_relocation(Relocation::compiler_stub_type);
    emit_int(literal->literal_int());
  } else {
#ifndef PRODUCT
    if (PrintCompiledCodeAsYouGo) {
       tty->print("%d:\t", _code_offset);
       oop().print_value_on(tty);
       if (offset != 0) {
         tty->print(" + %d", offset);
       }
       tty->cr();
    }
#endif
    GUARANTEE(literal->literal_int() == 0, "Can't yet handle oop + offset");
    emit_oop(oop.obj());
    emit_int((int)literal->literal_int() + (int)oop.obj()); // inline oop in code
  }

  // Indicate that we know this literal's position in the code
  literal->set_bci(position);

  // Update all places in the code that point to this literal.
  Label label = literal->label();
  bind_to(label, position);
  literal->set_label(label);
}

void BinaryAssembler::write_literals( const bool force ) {
  if( force || need_to_force_literals() ) {
    if( _code_offset & 0x3 ) {
      nop();
    }

    // We generally only want to write out the literals at the end of the
    // method (force = true).  But if our method is starting to get long,
    // we need to write them out more often.
    LiteralPoolElement* literal = _first_unbound_literal;
    for(; literal; literal = literal->next() ) {
      write_literal( literal );
    }
    // Indicate that there are no more not-yet-written literals
    _first_unbound_literal = literal;
    zero_literal_count();
  }
}

void BinaryAssembler::write_literals_if_desperate( const int extra_bytes ) {
  if( desperately_need_to_force_literals( extra_bytes ) ) {
    branch_around_literals();
  }
}

void BinaryAssembler::branch_around_literals( void ) {
  Label skip;
  b( skip );
  write_literals( true );
  bind( skip );
}

void BinaryAssembler::get_thread(Register reg) {
  get_current_thread(reg);
}

BinaryAssembler::CodeInterleaver::CodeInterleaver(BinaryAssembler* assembler){
  GUARANTEE(0, "Interleaver enabled");
  if (PrintCompiledCodeAsYouGo) {
    TTY_TRACE_CR(("***START_CODE_TO_BE_INTERLEAVED***"));
  }
  GUARANTEE(assembler->_interleaver == NULL, "No interleaver active");
  _assembler = assembler;
  _saved_size = assembler->code_size();
  _buffer = NULL;
  _current = _length = 0;
}

void BinaryAssembler::CodeInterleaver::start_alternate(JVM_SINGLE_ARG_TRAPS) {
  GUARANTEE(0, "Interleaver enabled");
  if (PrintCompiledCodeAsYouGo) {
    TTY_TRACE_CR(("***END_CODE_TO_BE_INTERLEAVED***"));
    TTY_TRACE_CR(("***restarting at %d***", _saved_size));
  }

  int old_size = _saved_size;
  int new_size = _assembler->code_size();
  int instructions = (new_size - old_size) >> 2;

  _current = 0;
  _length = instructions;
  _buffer = CompilerIntArray::allocate(instructions JVM_ZCHECK(_buffer));

  jvm_memcpy(_buffer->base(), _assembler->addr_at(old_size), new_size - old_size);
  _assembler->_code_offset = old_size; // Undo the code we've generated
  _assembler->_interleaver = this;
}

bool BinaryAssembler::CodeInterleaver::emit() {
  GUARANTEE(0, "Interleaver enabled");

  // Emit one instruction
  _assembler->_interleaver = NULL; // prevent recursion from emit_raw above
  if (_current < _length) {
    _assembler->emit(_buffer->at(_current));
    _assembler->_interleaver = this;
    _current++;
  }
  if (_current < _length) {
    // Still more to do
    _assembler->_interleaver = this;
    return true;
  }
  return false;
}

void BinaryAssembler::mov_imm(Register rd, int imm, const LiteralAccessor& la,
                              Condition cond) {
  Register rm;

  if (la.has_literal(imm, rm)) {
    it(cond);
    mov(rd, rm);
  } else {
    mov(rd, imm, cond);
  }
}
#endif // ENABLE_COMPILER
