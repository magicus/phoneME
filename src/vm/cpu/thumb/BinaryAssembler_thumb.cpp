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

#include "incls/_precompiled.incl"

#if ENABLE_THUMB_COMPILER && ENABLE_COMPILER

#include "incls/_BinaryAssembler_thumb.cpp.incl"

void BinaryAssembler::branch_helper(Label& L, bool link, bool is_near,
                                    Condition cond) {
  (void)link; // IMPL_NOTE -- is this needed?
  if (cond == nv) {
    return;
  }

  int pos = _code_offset;
  bool long_branch = true;
  int label_pos = L.position();

  if (!is_near) {
    if (!L.is_unused()) {
      // address addr = addr_at(pos);
      if (pos - label_pos < 2046) {
        long_branch = false;
      }
    } else {
      Method::Raw m = compiled_method()->method();
      if (m().code_size() < 50) {
        long_branch = false;
      }
    }
  }

  if (cond != al) {
    if (!L.is_unused()) {
      // backward conditional branch, cant
      // use trampoline branch
      int branch_distance = pos - label_pos + 4;
      if (branch_distance >= -256 && branch_distance < 255) {
        emit(13 << 12 | cond << 8);
      } else {
        emit(13 << 12 | not_cond(cond) << 8 |
               (long_branch ? 0x1 : 0)) ;
        pos = _code_offset;
        emit((short)(7 << 13 | 0x7FE));
        if (long_branch) {
          nop();
        }
      }      
    } else {
      // forward conditional branch
      emit(13 << 12 | cond << 8 | 0xFE) ;
    }
  } else {
    // unconditional branch
    pos = _code_offset;
    emit((short)(7 << 13 | 0x7FE));
    if (long_branch) {
      nop();
    }
  }
      
  if (!L.is_unused()) { 
    // since the branch target
    // is known emit it now
    address addr = addr_at(pos);
    if (long_branch) {
      Branch(addr).emit_bl(addr_at(label_pos));
    } else {
      Branch(addr).set_target(addr_at(label_pos));
    }
  } else { 
    // if the branch is a conditional branch which has a
    // limited target range, add a BranchLiteralElement
    // so that a trampoline branch can be emitted later
    // to bridge the conditional branch to its target
    // otherwise do nothing
    if (cond != al && !is_near) {
      SETUP_ERROR_CHECKER_ARG;
      append_branch_literal(pos JVM_CHECK);
    }
  }  

  if (!L.is_bound()) {
    L.link_to(pos);
  }
}

void BinaryAssembler::branch_helper(CompilationQueueElement* cqe,
                                    bool link, bool is_near, Condition cond) {
  Label target = cqe->entry_label();
  branch_helper(target, link, is_near, cond);
  cqe->set_entry_label(target);
}    

void BinaryAssembler::bind(Label& L, int alignment) {
  (void)alignment; // IMPL_NOTE: is this needed?
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
    int instr_kind;
    
    do {
      q = p;

      Instruction instr(q);
      instr_kind = instr.kind();
       
      switch (instr_kind) {
        case  1:
        {
          MemAccess m(q);
          p = m.location();
          m.set_location(addr_at(code_offset));
    if (PrintCompiledCodeAsYouGo) {
      TTY_TRACE_CR(("MEM ACCESS "));
    }
          break;
        }
        case  3:
          { Branch b(q);
            int offset = q - addr_at(0);
            p = b.target();
 
            // Note the branch's target is used when we would like
            // to use a trampoline to jump to a farther target
            if (p == q) {
              int branch_distance = code_offset - offset - 4;
              if (b.is_conditional()) {
                 if ((branch_distance > 255) || 
                     (branch_distance < -256)) {
                   // Early branch resolution
                   GUARANTEE(code_offset + 4 < _code_offset, 
                             "Branch chaining failed");
                   b.set_target(addr_at(_code_offset+2));
                   emit((short)(7 << 13 | 1));
                   int new_branch_pos = _code_offset;
                   int new_branch_distance = code_offset - new_branch_pos - 4;
                   emit((short)(7 << 13 | 0x7FE));
                   nop();
                   Branch long_branch(addr_at(new_branch_pos));
                   if ((new_branch_distance >= 2046) || 
                       (new_branch_distance <= -2048)) {
                     long_branch.emit_bl(addr_at(code_offset));
                   } else {
                     long_branch.set_target(addr_at(code_offset));
                   }
                 } else {
                   b.set_target(addr_at(code_offset));
                 }
              } else if ((branch_distance >= 2046) || 
                  (branch_distance <= -2048)) {
                // emit bl <target> instead b <target>
                b.emit_bl(addr_at(code_offset));
              } else {
                b.set_target(addr_at(code_offset));
              }

#ifndef PRODUCT      
              if (PrintCompiledCodeAsYouGo) {
                int offset = q - addr_at(0);
                Disassembler d(tty);
                tty->print("%d:\t0x%08x\t", offset, *(short *)q);
                d.disasm((short*)q, *(short *)q, offset);
                tty->cr();
                tty->flush();
              }
#endif
            }
          }
          break;
        default: 
        {
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
        }
      }
    } while (p != q);
  }
  L.bind_to(code_offset);
}

void BinaryAssembler::access_literal_pool(Register rd, 
                                          LiteralPoolElement* literal,
                                          Condition cond, bool is_store) {
  Label L = literal->label();
  int pos = _code_offset;

  const int target = L.is_unused() ? pos : L.position();
  // Generate instruction that loads from the target
  if (is_store) {
    str(rd, pc, target - pos, cond);
  } else {
    // Fake load since this allows us to keep -ve offset
    // which is needed during literal chaining. We need
    // to fake because thumb doesn't have a load instruction 
    // that can take a -ve offset
    int offset = (target - pos) / 2;
#ifndef PRODUCT
    if (GenerateCompilerComments) {
      static char buff[64];
      jvm_sprintf(buff, "load literal, offset=%d", offset);
      Disassembler::eol_comment(buff);
    }
#endif
    GUARANTEE(abs(offset) < 0x1000, "Invalid offset");
    emit((short)(0x1 << 14 | rd << 11 | (offset & 0x7FF)));
  }
  if (!L.is_bound()) { 
    L.link_to(pos);    
  }
  literal->set_label(L);
}
    
void BinaryAssembler::ldr_literal(Register rd, OopDesc* obj, int imm32,
                                  Condition cond) {
  SETUP_ERROR_CHECKER_ARG;
  LiteralPoolElement* e = find_literal(obj, imm32 JVM_NO_CHECK);
  if( e ) {
    ldr_from(rd, e, cond);
  }
}

void BinaryAssembler::ldr_oop(Register rd, const Oop* oop, Condition cond) {
  if (oop->is_null()) {
    // Java null is 0.
    NearLabel skip_mov;
    b(skip_mov, not_cond(cond));
    mov_imm(rd, zero);
    bind(skip_mov);
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
      NearLabel skip_mov;
      b(skip_mov, not_cond(cond));

      JavaClass::Raw klass;
      if (oop->is_java_near()) {
        klass = oop->klass();
      } else {
        klass = oop;
      }

      int class_id = klass().class_id();
      get_class_list_base(rd, always);
      int offset = class_id * sizeof(OopDesc*);

      RegisterAllocator::reference(rd);
      GUARANTEE((offset & WordAlignmentMask) == 0, 
                "Offset must be word-aligned");
      ldr(rd, rd, offset);

      if (load_near) {
        ldr(rd, rd, InstanceClass::prototypical_near_offset());
      }
      RegisterAllocator::dereference(rd);

      bind(skip_mov);
      return;
    }
  }
#endif

  ldr_literal(rd, oop->obj(), 0, cond);
}

extern "C" { 
  extern address gp_constants[], gp_constants_end[];
}

void BinaryAssembler::mov_imm(Register rd, address target, Condition cond) {
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
  ldr_literal(rd, NULL, imm32, cond);
}    

LiteralPoolElement* BinaryAssembler::find_literal(OopDesc* obj, int imm32 JVM_TRAPS)
{
  LiteralPoolElement* literal = _first_literal;
  for(; literal; literal = literal->next() ) {
    if( literal->is_bound() ) { 
      // This literal is too far away to use.  We could discard it
      // but it's not really worth the effort.
      continue;
    }
    if( literal->matches(obj, imm32)) { 
      return literal;
    }
  }

  GUARANTEE( obj || imm32 != 0, "Invalid literal");
  
  literal = LiteralPoolElement::allocate(obj, imm32 JVM_NO_CHECK);
  // Add this literal to the end of the literal pool list

  append_literal( literal );
  return literal;
}

void BinaryAssembler::append_branch_literal(int branch_pos JVM_TRAPS) {
  LiteralPoolElement* literal =
    LiteralPoolElement::allocate(NULL, branch_pos JVM_ZCHECK(literal) );
  // Add this literal to the end of the literal pool list
  if( _first_unbound_branch_literal == NULL ) {
    // This is the first branch bridge that hasn't yet been written out.
    _first_unbound_branch_literal = literal;
  } else {
    _last_unbound_branch_literal->set_next(literal);    
  }
  _last_unbound_branch_literal = literal;
  _unbound_branch_literal_count++;
  
  Label L = literal->label();
  if (!L.is_bound()) { 
    L.link_to(branch_pos);    
  }
  literal->set_label(L);
}  

void BinaryAssembler::append_literal(LiteralPoolElement* literal) {
  if( !_first_literal ) {
    GUARANTEE(_last_literal == NULL, "No literals");
    GUARANTEE(_first_unbound_literal == NULL, "No unknown literals");
    _first_literal = literal;
  } else { 
    _last_literal->set_next(literal);
  }
  _last_literal = literal;
  if( !_first_unbound_literal ) {
    // This is the first literal that hasn't yet been written out.
    _first_unbound_literal = literal;
  }
  increment_literal_count();
}


void BinaryAssembler::write_value_literals() {
  // We generally only want to write out the literals at the end of the 
  // method (force = true).  But if our method is starting to get long, 
  // we need to write them out more often.
  // A literal must be written within 4K of its use.
  if ((_code_offset & 0x3) != 0) {
    nop();
  }
    
  LiteralPoolElement* literal = _first_unbound_literal; 
  for(; literal; literal = literal->next()) { 
    write_literal( literal );
  }
  // Indicate that there are no more not-yet-written literals
  _first_unbound_literal = literal;
  zero_literal_count();
}

void BinaryAssembler::write_branch_literals() {
  if ((_code_offset & 0x3) != 0) {
    nop(); // IMPL_NOTE: this doesn't seem necessary
  }
    
  LiteralPoolElement* literal = _first_unbound_branch_literal; 
  for (; literal; literal = literal->next() ) { 
    // Emit a long branch to serve as a trampoline point
    // for another branch. This will patched to the right
    // target when the target code is emitted
    Label branch_label = literal->label();
    address branch_instr_addr = addr_at(branch_label.position());
    Branch b(branch_instr_addr);
    int position = _code_offset;

      // Some branches that we speculate could need a trampoline
      // could get resolved with a shorter branch. So ignore them
      // and save space

    // Check if branching on to itself
    if (b.target() == branch_instr_addr) { 
      // This means this conditional branch has not been resolved yet 
      //  and therefore a trampoline branch is required.
      emit((short)(7 << 13 | 0x7FE));
      nop(); // IMPL_NOTE: this doesn't seem necessary

      // Indicate that we know this literal's position in the code
      literal->set_bci(position);

      // Update all places in the code that point to this literal.  
      // This would make the conditional branch target the trampoline
      // branch that we just emitted above. The trampoline branch would
      // then later resolve to the right target
      Label label = literal->label();
      bind_to(label, position);
      GUARANTEE(b.target() == addr_at(position), "Branch optimization failed");
      
      literal->set_label(label);
    }
  }

  // Indicate that there are no more not-yet-written literals
  _first_unbound_branch_literal = literal;
  _unbound_branch_literal_count = 0;
}

void BinaryAssembler::write_literals(bool force) {
  bool will_write_value_literals = need_to_force_literals();
  
  if (force ||  will_write_value_literals ||
      need_to_force_branch_literals()) { 
    write_branch_literals();
  }

  if (force ||  will_write_value_literals) { 
    write_value_literals();
  }
}

void BinaryAssembler::write_literals_if_desperate() { 
  // Check if any branch literals needs to be written
  if (desperately_need_to_force_branch_literals()) {
    // force write all eligible branch literals
    CodeInterleaver *old = _interleaver;
    _interleaver = NULL;
    NearLabel skip;
    b(skip);
    write_branch_literals();
    bind(skip);
    _interleaver = old;
  }
   
  if (desperately_need_to_force_literals()) {
    CodeInterleaver *old = _interleaver;
    _interleaver = NULL;
    NearLabel skip;
    b(skip);
    write_value_literals();
    bind(skip);
    _interleaver = old;
  }
}

void BinaryAssembler::write_literal(LiteralPoolElement* literal) {
  int position = _code_offset;
  Oop::Raw oop = literal->literal_oop();
  if (oop.is_null()) {
    emit_int(literal->literal_int());
  } else if (GenerateROMImage && literal->literal_int() != 0) {
    GUARANTEE(oop.equals(compiled_method()), "Special flag");
    emit_relocation(Relocation::compiler_stub_type);
    emit_raw(literal->literal_int());
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
    emit_oop( oop.obj() );
    emit_raw((int)literal->literal_int() + (int)oop.obj()); // inline oop in code
  }

  // Indicate that we know this literal's position in the code
  literal->set_bci(position);

  // Update all places in the code that point to this literal.  
  Label label = literal->label();
  bind_to(label, position);
  literal->set_label(label);
}

void BinaryAssembler::get_thread(Register reg) {
  get_current_thread(reg);
}  

BinaryAssembler::CodeInterleaver::CodeInterleaver(BinaryAssembler* assembler){
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
  if (PrintCompiledCodeAsYouGo) { 
    TTY_TRACE_CR(("***END_CODE_TO_BE_INTERLEAVED***"));
    TTY_TRACE_CR(("***restarting at %d***", _saved_size));
  }

  int old_size = _saved_size;
  int new_size = _assembler->code_size();
  int instructions = (new_size - old_size) >> 1;
   
  _current = 0;
  _length = instructions;
  _buffer = CompilerShortArray::allocate(instructions JVM_ZCHECK(_buffer));

  jvm_memcpy(_buffer->base(), _assembler->addr_at(old_size), new_size - old_size);
  _assembler->_code_offset = old_size; // Undo the code we've generated
  _assembler->_interleaver = this;
}

bool BinaryAssembler::CodeInterleaver::emit() { 
  
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

void BinaryAssembler::ldr(Register rd, Register rn, int offset, Condition cond)
{
  if (rn == Assembler::pc) {
    GUARANTEE((_code_offset & 0x3) == 0, "Misaligned pc load");
  }
  
  if (cond != al) {
    NearLabel skip_ldr;
    b(skip_ldr, not_cond(cond));
    Assembler::ldr(rd, rn, offset);
    bind(skip_ldr);
  } else {
    Assembler::ldr(rd, rn, offset);
  }
}

void BinaryAssembler::str(Register rd, Register rn, int offset, Condition cond) {
  if (cond != al) {
    NearLabel skip_str;
    b(skip_str, not_cond(cond));
    Assembler::str(rd, rn, offset);
    bind(skip_str);
  } else {
    Assembler::str(rd, rn, offset);
  }
}
void BinaryAssembler::ldrb(Register rd, Register rn, int offset, Condition cond) {
  if (cond != al) {
    NearLabel skip_ldr;
    b(skip_ldr, not_cond(cond));
    Assembler::ldrb(rd, rn, offset);
    bind(skip_ldr);
  } else {
    Assembler::ldrb(rd, rn, offset);
  }
}

void BinaryAssembler::strb(Register rd, Register rn, int offset, Condition cond) {
  if (cond != al) {
    NearLabel skip_str;
    b(skip_str, not_cond(cond));
    Assembler::strb(rd, rn, offset);
    bind(skip_str);
  } else {
    Assembler::strb(rd, rn, offset);
  }
}

void BinaryAssembler::breakpoint(Condition cond) {
  // IMPL_NOTE: we need to make this portable on platforms that may
  // use a different method of breakpoints. Make sure the fix works
  // with cross compilation -- remember the loop generator is compiled
  // with the host compiler!
  if (cond == al) {
    Assembler::breakpoint();
  } else {
    NearLabel skip_swi;
    b(skip_swi, not_cond(cond));
    Assembler::breakpoint();      
    bind(skip_swi);
  }    
}

void BinaryAssembler::arith(Opcode opcode, Register rd, Register rn,
                            Condition cond) {
  if (cond == al) {
    Assembler::arith(opcode, rd, rn);
  } else {
    NearLabel skip_arith;
    b(skip_arith, not_cond(cond));
    Assembler::arith(opcode, rd, rn);
    bind(skip_arith);
  }      
}

void BinaryAssembler::mov_reg(Register rd, Register rs, Condition cond) {
  if (cond == al) {
    Assembler::mov_reg(rd, rs);
  } else {
    NearLabel skip_mov;
    b(skip_mov, not_cond(cond));
    Assembler::mov_reg(rd, rs);
    bind(skip_mov);
  }
}

void BinaryAssembler::mov_imm(Register rd, int imm32, Condition cond) {
  if (cond == al) {
    Assembler::mov_imm(rd, imm32);
  } else {
    NearLabel skip_mov;
    b(skip_mov, not_cond(cond));
    Assembler::mov_imm(rd, imm32);
    bind(skip_mov);
  }
}

int is_multiple(int imm32) {
  int val = imm32;
  int result = 0;
  if((val % 256) == 0) {
    result = val/256;
  }
  return (val % 256) ? 0 : imm32/256;
}

void BinaryAssembler::mov_imm(Register rd, int imm32, LiteralAccessor& la, 
                              Condition cond) {
  Register rm;

  // Try to avoid loading a literal if possible.  Look for
  // opportunities to just mov immediate values (or mov and
  // shift them)
  if(imm32 <= 256) {
    mov_imm(rd, imm32, cond);
    return;
  } 
  
/*
  //This seems to break a few random cases.  Disable for now.
  else if(cond == al && (imm32 % 256 == 0)) {
    int shift_amt = 0;
    int optimizable = 1;
    switch(imm32) {
      case(512):
        shift_amt = 1;
        break;
      case(1024):
        shift_amt = 2;
        break;
      case(2048):
        shift_amt = 3;
        break;
      case(4096):
        shift_amt = 4;
        break;
      default:
        optimizable = 0;
    }
    if(optimizable) {
      mov_imm(rd, 0x1);
      lsl_imm(rd, rd, 8 + shift_amt);
      return;
    }
  }
*/
 
  if (la.has_literal(imm32, rm)) {
    mov_reg(rd, rm, cond);
  } else {
    mov_imm(rd, imm32, cond);
  }
}

void BinaryAssembler::rsb(Register rd, Register rm, int imm, Condition cond) {
  if (cond == al) {
    Macros::rsb(rd, rm, imm);
  } else {
    NearLabel skip_rsb;
    b(skip_rsb, not_cond(cond));
    Macros::rsb(rd, rm, imm);
    bind(skip_rsb);
  }  
}

void BinaryAssembler::oop_write_barrier(Register dst, Register tmp1, Register tmp2,
                               Register tmp3, bool range_check) {
  Register base            = tmp1;
  Register bit_offset      = dst;
  Register tmp             = tmp3;
  Condition cond           = al;

  NearLabel skip_oop_barrier;

  GUARANTEE(dst != tmp1 && dst != tmp2 && dst != tmp2 && 
            tmp1 != tmp2 && tmp1 != tmp3 && tmp2 != tmp3, " Invalid barrier registers");
  
  NOT_PRODUCT(comment("oop_write_barrier"));
  if (range_check) {
    get_heap_start(tmp1);
    get_heap_top(tmp2);
    add(tmp2, tmp2, one);
    cmp(dst, reg(tmp1));

    Label skip_cmp;
    b(skip_cmp, not_cond(hs));
    cmp(tmp2, reg(dst));
    bind(skip_cmp);
    cond = hs;
 
    b(skip_oop_barrier, not_cond(cond));
    get_bitvector_base(base);
    // bit_offset starting from base
    lsr_imm(bit_offset, dst, LogBytesPerWord);
  } else {
    get_bitvector_base(base);
    // bit_offset starting from base
    lsr_imm(bit_offset, dst, LogBytesPerWord);
  }
    
  if (HARDWARE_LITTLE_ENDIAN) {
    const int rem_mask = right_n_bits(LogBitsPerByte);
    Register byte_value = tmp2;
    Register byte_bit_offset = bit_offset;

    // Get the byte, update base 
    lsr_imm(byte_value, bit_offset, LogBitsPerByte); //using byte_value as scratch
    add_regs(base, base, byte_value);
    ldrb(byte_value, base);
    // Compute the bit offset (dst % bitsPerByte) inside the mask
    mov_imm(tmp, rem_mask);
    andr(byte_bit_offset, tmp);
    mov_imm(tmp, one);
    lsl(tmp, byte_bit_offset);
    orr(byte_value, tmp);
    strb(byte_value, base);
  } else {
    const int rem_mask = right_n_bits(LogBitsPerWord);
    Register word_value = tmp2;
    Register word_bit_offset = bit_offset;

    // With big-endian, we must set a word at a time, so that we match
    // the behavior of ObjectHeap_arm.cpp.
    lsr_imm(dst, dst, LogBitsPerWord);
    lsl_imm(tmp, dst, LogBytesPerWord);
    ldr(word_value, base, tmp);

    mov_imm(tmp, rem_mask);
    andr(word_bit_offset, tmp);
    mov_imm(tmp, one);
    lsl(tmp, word_bit_offset);
    orr(word_value, tmp);
    str(word_value, base);
  }

  if (range_check) {
    bind(skip_oop_barrier);
  }
}

#endif
