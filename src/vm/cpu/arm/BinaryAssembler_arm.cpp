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

#include "incls/_precompiled.incl"

#if ENABLE_COMPILER && !ENABLE_THUMB_COMPILER

#include "incls/_BinaryAssembler_arm.cpp.incl"

// Usage of Labels
//
// free  : label has not been used yet
// bound : label location has been determined, label position is
//          corresponding code offset 
// linked: label location has not been determined yet, label position is code
//          offset of last instruction referring to the label
//
// Label positions are always code offsets in order to be (code) relocation
// transparent.  Linked labels point to a chain of (linked) instructions that
// eventually need to be fixed up to point to the bound label position. Each
// instruction refers to the previous instruction that also refers to the
// same label. The last instruction in the chain (i.e., the first instruction
// to refer to the label) refers to itself.  
// 
// These instructions use pc-relative addressing and thus are relocation
// transparent.  

void BinaryAssembler::branch(Label& L, bool link, Condition cond) {
  const int pos = _code_offset;
  // Default is to have the label branch to itself.  Hence the "-2"
  emit(cond << 28 | 0x5 << 25 | (link ? 0x1FFFFFE : 0x0FFFFFE));
  if (!L.is_unused()) { 
    address addr = addr_at(pos);
    Branch(addr).set_target(addr_at(L.position()));
  } else { 
    // the branch is already to itself, so do nothing
  }
  if (!L.is_bound()) {
    L.link_to(pos);
  }
}

void BinaryAssembler::b(CompilationQueueElement* cqe, Condition cond) { 
  Label target = cqe->entry_label();
  branch(target, false, cond);
  cqe->set_entry_label(target);
}

void BinaryAssembler::bl(address target, Condition cond) { 
#if USE_COMPILER_GLUE_CODE
  GUARANTEE((address(target) >= address(&compiler_glue_code_start) &&
            address(target) < address(&compiler_glue_code_end)),
            "target must be withn glue_code_start and glue_code_end");
      
  int glue_offset = DISTANCE(&compiler_glue_code_start, target);
  address glue = ObjectHeap::glue_code_start() + glue_offset;
  address cur_pc = addr_at(code_size()) + 8;
  int branch_offset = int(glue) - int(cur_pc);

  emit_long_branch();
  compiled_method()->set_has_branch_relocation();
  Assembler::bl(branch_offset, cond);
#else
  SHOULD_NOT_REACH_HERE();
#endif
}

void BinaryAssembler::bind(Label& L, int /*alignment*/) {
  bind_to(L, _code_offset);
}

void BinaryAssembler::bind_to(Label& L, jint code_offset) {
  // if the code has overflowed the compiled method, we
  // cannot expect to be able to follow the link chain.
  if (L.is_linked() && !has_overflown_compiled_method()) {
    // follow the chain and fixup all instructions -
    // last instruction in chain is referring to itself
    COMPILER_PRINT_AS_YOU_GO(("**Patching to %d", code_offset));

    address p = addr_at(L.position());
    address q;
    do {
      q = p;
      Instruction instr(q);
      switch (instr.kind()) {
        case  2:
          { MemAccess m(q);
            p = m.location();
            m.set_location(addr_at(code_offset));
          }
          break;
        case  5:
          { Branch b(q);
            p = b.target();
            b.set_target(addr_at(code_offset));
          }
          break;
#if ENABLE_ARM_VFP
        case  6:
          { VFPMemAccess m(q);
            p = m.location();
            m.set_location(addr_at(code_offset));
          }
          break;
#endif
        default: SHOULD_NOT_REACH_HERE();
      }
#if USE_COMPILER_COMMENTS
      if (PrintCompiledCodeAsYouGo) { 
          int offset = q - addr_at(0);
          Disassembler d(tty);
          tty->print("**%d:\t", offset);
          tty->print("0x%08x\t", instr.addr());
          d.disasm(NULL, *(int *)q, offset);
          tty->cr();
      }
#endif
    } while (p != q);
  }
  L.bind_to(code_offset);
}

void BinaryAssembler::access_literal_pool(Register rd, 
                                          LiteralPoolElement* literal,
                                          Condition cond, bool is_store) {
  Label L = literal->label();

  const int pos = _code_offset;
  const int target = L.is_unused() ? pos : L.position();
  Address2 address = imm_index(pc, target - (pos + 8));
  // Generate instruction that loads from the target
  if (is_store) { 
    str(rd, address, cond);
  } else {
#if ENABLE_ARM_VFP
    if (rd > Assembler::r15) {
      if (L.is_bound()) {
        flds(rd, imm_index5(pc, target - (pos + 8)), cond);
      } else {
        flds_stub(rd, imm_index5_stub(pc, target - (pos + 8)), cond);
      }
    } else
#endif
      ldr(rd, address, cond);
  }
  if (!L.is_bound()) { 
    L.link_to(pos);    
  }
  literal->set_label(L);
}
    
void BinaryAssembler::ldr_literal(Register rd, OopDesc* obj, int imm32,
                                  Condition cond) {
  SETUP_ERROR_CHECKER_ARG;
  enum { max_ldr_offset = 1 << 12 };
  LiteralPoolElement* e = find_literal(obj, imm32, max_ldr_offset JVM_NO_CHECK);
  if( e ) {
    ldr_from(rd, e, cond);
  }
}

void BinaryAssembler::ldr_oop(Register rd, const Oop* oop, Condition cond) {
  if (oop->is_null()) {
    // Java null is 0.
    mov(rd, zero, cond);
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
      int offset = class_id * sizeof(OopDesc*);

      if (offset >= 0x1000) {
        // impossible to use single LDR
        add_imm(rd, rd, offset & ~0xfff, no_CC, cond);
        offset &= 0xfff;
      }
      ldr(rd, imm_index(rd, offset), cond);

      if (load_near) {
        ldr(rd, imm_index(rd, InstanceClass::prototypical_near_offset()), 
            cond);
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

void BinaryAssembler::ldr_imm_index(Register rd, Register rn, int offset_12) {
  ((BinaryAssembler*)_compiler_code_generator)->ldr(rd, imm_index(rn, offset_12));
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

#if ENABLE_ARM_VFP
void BinaryAssembler::fld_literal(Register rd, int imm32, Condition cond) {
  SETUP_ERROR_CHECKER_ARG;
  enum { max_fld_offset = 1 << 8 };
  LiteralPoolElement* e =
    find_literal( NULL, imm32, max_fld_offset JVM_NO_CHECK );
  if( e ) {
    ldr_from(rd, e, cond);
  }
}

LiteralPoolElement* BinaryAssembler::find_literal(OopDesc* obj, const int imm32, 
                                                  int offset JVM_TRAPS)
{
  enum { max_code_size_to_branch_around_literals = 8 };
  offset -= max_code_size_to_branch_around_literals;

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
          write_literals(true);
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
#else  // !ENABLE_ARM_VFP
LiteralPoolElement* BinaryAssembler::find_literal(OopDesc* obj,
                                          const int imm32, int offset JVM_TRAPS)
{
  enum { max_code_size_to_branch_around_literals = 8 };

  offset -= max_code_size_to_branch_around_literals;
  // position is just beyond where a ldr instruction can reach
  const int position = _code_offset - offset;

  {
    for( LiteralPoolElement* ptr = _first_literal; ptr; ptr = ptr->_next) {
      if( ptr->is_bound() && ptr->_bci <= position ) {
        // This literal is too far away to use.  We could discard it
        // but it's not really worth the effort.
        continue;
      }
      if( ptr->matches(obj, imm32) ) {
        return ptr;
      }
    }
  }

  // We need to create a new literal.
  LiteralPoolElement* literal =
    LiteralPoolElement::allocate(obj, imm32 JVM_ZCHECK_0( literal ) );
  // Add this literal to the end of the literal pool list
  append_literal( literal );

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
#endif

void BinaryAssembler::append_literal( LiteralPoolElement* literal ) {
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

void BinaryAssembler::write_literals(const bool force) {
  if (force ||  need_to_force_literals()) { 
    // We generally only want to write out the literals at the end of the 
    // method (force = true).  But if our method is starting to get long, 
    // we need to write them out more often.
    // A literal must be written within 4K of its use.
    LiteralPoolElement* literal = _first_unbound_literal;
    for(; literal; literal = literal->next() ) { 
      write_literal( literal );
    }
    // Indicate that there are no more not-yet-written literals
    _first_unbound_literal = literal;
    zero_literal_count();
  }
}

void BinaryAssembler::write_literals_if_desperate(int extra_bytes) { 
  if (desperately_need_to_force_literals(extra_bytes)) {
    // Don't do any interleaving while writing literals
    CodeInterleaver *old = _interleaver;
    _interleaver = NULL;
    Label skip;
    b(skip);
    write_literals(true);
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
#if USE_COMPILER_COMMENTS
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
    emit_raw(literal->literal_int() + (int)oop.obj()); // inline oop in code
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

void
BinaryAssembler::CodeInterleaver::init_instance(BinaryAssembler* assembler) {
  COMPILER_PRINT_AS_YOU_GO(("***START_CODE_TO_BE_INTERLEAVED***"));
  GUARANTEE(assembler->_interleaver == NULL, "No interleaver active");
  _assembler = assembler;
  _saved_size = assembler->code_size();
  _current = _length = 0;
}

void BinaryAssembler::CodeInterleaver::start_alternate(JVM_SINGLE_ARG_TRAPS) { 
  COMPILER_PRINT_AS_YOU_GO(("***END_CODE_TO_BE_INTERLEAVED***"));
  COMPILER_PRINT_AS_YOU_GO(("***restarting at %d***", _saved_size));

  int old_size = _saved_size;
  int new_size = _assembler->code_size();
  int instructions = (new_size - old_size) >> 2;
   
  _current = 0;
  _length = instructions;
  _buffer = CompilerIntArray::allocate(instructions JVM_ZCHECK(_buffer) );

  jvm_memcpy(_buffer->base(), _assembler->addr_at(old_size), 
                  new_size - old_size);
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

void BinaryAssembler::CodeInterleaver::flush() { 
  if (_buffer) {
    while (emit()) {;}
  }
  _assembler->_interleaver = NULL;
}

#if ENABLE_NPCE
bool BinaryAssembler::is_branch_instr(jint offset) {
    address q = addr_at(offset);
    Instruction instr(q);
    //if the instruction is branch, use old Null Point code
    if (instr.kind() == 5) {
      return true;
  }
  return false;
}
void 
BinaryAssembler::emit_null_point_callback_record(Label& L, 
                                 jint offset_of_second_instr_in_words) {
  jint stub_offset = _code_offset;
  if (L.is_linked() && !has_overflown_compiled_method()) {

    COMPILER_PRINT_AS_YOU_GO(("**update_mapping_table to %d\n", stub_offset));
    COMPILER_PRINT_AS_YOU_GO(("**Label address is  to %d\n", L.position()));

    //if the instruction is branch, use old Null Point code
   if (is_branch_instr(L.position())) {
      COMPILER_PRINT_AS_YOU_GO((
         "special case in NPCE, use cmp instruction, update jump target  \n"));
      bind_to(L,stub_offset);
      return;
    }
    emit_relocation(Relocation::npe_item_type, stub_offset, L.position());
    if (offset_of_second_instr_in_words > 0) {
      emit_relocation(Relocation::npe_item_type, stub_offset,
        L.position() + (offset_of_second_instr_in_words << LogBytesPerWord));
      COMPILER_PRINT_AS_YOU_GO(("**Label address is  to %d",
                       (L.position() + (offset_of_second_instr_in_words<<LogBytesPerWord))));
    }
  }
  if (!L.is_linked() && !has_overflown_compiled_method()) {
    COMPILER_PRINT_AS_YOU_GO(("**Default stub is  to %d\n", stub_offset));
    emit_relocation(Relocation::npe_item_type, stub_offset, 0);
  }
  L.bind_to(stub_offset);
}

void
BinaryAssembler::record_npe_point(CompilationQueueElement* stub, 
                                    int offset_in_instr_nums , Condition cond) { 
  int instruction_offset = _code_offset + (offset_in_instr_nums << LogBytesPerWord);  
#if ENABLE_INTERNAL_CODE_OPTIMIZER
  //we record the instr offset into a table, we will mark them into a 
  //abort point bitmap later for extend basic block scheduling
  //all the LDR instr which may result a null point exception will be recorded
  //into the table. While only instr with exception handle will emit relocaiton
  //item.
  Compiler::current()->record_null_point_exception_inst(instruction_offset);
#endif

  if (stub == NULL ) {
    return;
  }

  //if it's a shared stub for the exception code,
  //we don't record their position into relocation stream
  //later.
  if (((ThrowExceptionStub*)stub)->is_persistent() ) {
    return;
  }
  
  Label target = stub->entry_label();
  if (!target.is_bound()) {
    GUARANTEE(!target.is_linked(), "Non shared stub should not be linked before");
    target.link_to(instruction_offset);
    COMPILER_PRINT_AS_YOU_GO(("**record a NPCE position, offset is %d", 
                            (instruction_offset)));
  } else {
    //stub has been generated
    emit_relocation(Relocation::npe_item_type, target.position(),
                     instruction_offset);
  }
  stub->set_entry_label(target);
}
#endif //ENABLE_NPCE


#if ENABLE_INTERNAL_CODE_OPTIMIZER
#define address_to_offset(address)  (address -addr_at(0))
#ifndef PRODUCT
#define DUMP_INSTRUCTIONS(i, a)         do{  Disassembler disa(tty);\
      if(OptimizeCompiledCodeVerboseInternal) {\
          int offset = i - addr_at(0);\
            tty->print("**%d:\t", offset);\
            tty->print("0x%08x\t", a.addr());\
            disa.disasm(NULL, *(int *)i, offset);\
            tty->cr();};} while(false);
#else
#define DUMP_INSTRUCTIONS(i, a) 
#endif

int BinaryAssembler::first_instr_of_literal_loading(Label& L, 
                                       address  begin_of_cc) {
    if (L.position() < address_to_offset(begin_of_cc)) {
      return literal_not_used_in_current_cc;      
    }

    // follow the chain and fixup all instructions -
    // last instruction in chain is referring to itself
    VERBOSE_SCHEDULING_AS_YOU_GO(
                       ("%d CC begin at %d", L.position(), 
                      address_to_offset(begin_of_cc)));
    address p = addr_at(L.position());
    address q;
    do {
      q = p;
      Instruction instr(q);
      DUMP_INSTRUCTIONS(q, instr);
      //it is memory access ins         
      if (instr.kind() == 2) {
        MemAccess m(q);
        //p go to previous ins
        p = m.location();
        //the head of the chain is out of current compilation continuals and we             
        //has step out of current compilation continuals,
        //so we abort here.
        if( p <  begin_of_cc ) {
          return address_to_offset(p);
        }
      } else {
        SHOULD_NOT_REACH_HERE();
      }
    } while (p != q);
    // the head of the chain is in current compilation continuals
    // and we found the head
    return address_to_offset(q);
}

int BinaryAssembler::next_schedulable_branch(Label L, 
                                  address begin_of_cc, int& next) {
    address p;
    address q;

    if (next !=  no_schedulable_branch) {
      p = addr_at(next);
    } else {
      if(L.position() <address_to_offset(begin_of_cc) ) {
        //next == 0 so label patch address  is no in current CC
        return stop_searching;      
      }
      p = addr_at(L.position());
    }

    {
      q = p;
      Instruction instr(q);
      DUMP_INSTRUCTIONS(q, instr);

      if (instr.kind() == 5) {
        if ( next == no_schedulable_branch) {
          //we are at the tail of the chain
          return address_to_offset(q);
        }
        Branch b(q);
        //b is the prev branch,
        //so we get the next branch from it
        p = b.target();
        //is it point to itself
        if ( address_to_offset(p) == next ) {
          //we reach the head and should stop the searching
          return stop_searching;
        } else if ( p < begin_of_cc ) {
          //the head is in previous cc. we also stop here
          //and put the branch address in param next.
          next = (int) p;
          return branch_is_in_prev_cc;
        } else {
          //we get the next branch, we still
          //in the chain, the chain still in current cc.
          return address_to_offset(p);
        }
        
      } else {
        SHOULD_NOT_REACH_HERE();
      }
    } 
    return stop_searching;
}
#undef address_to_offset
#endif

#if ENABLE_LOOP_OPTIMIZATION
//check whether current instruction is a conditional jump instruction
bool BinaryAssembler::is_jump_instr(int instr, int next_instr, Assembler::Condition& cond,
                                                             bool& link, bool& op_pc, int& offset) {
  cond = Assembler::as_condition(instr >> 28 & 0xf);
  int op = (instr >> 25) & 0x7;
  op_pc = false;
//check whether the instruction may contain pc operation
  if(rd_field(instr) == Assembler::pc || rm_field(instr) == Assembler::pc ||
      rn_field(instr) == Assembler::pc || rs_field(instr) == Assembler::pc) {
    op_pc = true;
  }
#if ENABLE_PAGE_PROTECTION
  // This encoding denotes ldr/str/ldrb/strb rd, [gp, #-offset]
  if ((next_instr & 0xffaf0000) != (0xe5000000 | (Assembler::gp << 16))) {
    return false;
  }
#elif ENABLE_XSCALE_WMMX_TIMER_TICK && !ENABLE_TIMER_THREAD
#define TEXTRCB_R15_ZERO  0xee13f170
  if( next_instr != TEXTRCB_R15_ZERO ) {
    return false;
  }
#else
  int next_cond = Assembler::as_condition(next_instr >> 28 & 0xf);
  int next_offset = next_instr & 0xFFF;
  int next_op =( next_instr >> 26 ) & 0x3;
  int timer_tick_offset = (address)(&_rt_timer_ticks) - (address)&gp_base_label; 
  if( next_cond != 0xE || next_op != 0x1) { 
    return false;
  }
  if(rn_field(next_instr) != Assembler::gp ) {
    return false;
  }
  if( next_offset != timer_tick_offset ) {
    return false;
  }
#endif

  if( op == 5) {
    if (bit(instr, 24)) {
      link = true;
    } else      {
      link = false;
    }
    offset = ((instr & 0xffffff) | (bit(instr, 23) ? 0xff000000 : 0)) << 2; 
    return true;
  }
  return false;
}

//get the reverse condition of current condition
Assembler::Condition BinaryAssembler::get_reverse_cond(Assembler::Condition cond) {
  if((int)cond % 2 == 0) {
    return (Assembler::Condition)(cond + 1);
  } else {
    return (Assembler::Condition)(cond - 1);
  }
}

//call the branch function
void BinaryAssembler::back_branch(Label& L, bool link, Assembler::Condition cond) {
  branch(L, link, cond);
}

#endif // ENABLE_LOOP_OPTIMIZATION

#endif // ENABLE_COMPILER && !ENABLE_THUMB_COMPILER

