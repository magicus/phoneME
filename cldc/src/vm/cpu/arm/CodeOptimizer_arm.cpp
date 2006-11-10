/*
 * @(#)CodeOptimizer_arm.cpp	1.23 06/04/05 09:11:30 
 *
 * Portions Copyright  2003-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#if ENABLE_CODE_OPTIMIZER && !ENABLE_THUMB_COMPILER
#include "incls/_CodeOptimizer_arm.cpp.incl"

OptimizerInstruction::OptimizerInstruction(OptimizerInstruction& copy) {
  jvm_memcpy(this, &copy, sizeof(OptimizerInstruction));
}

void OptimizerInstruction::init(int* ins_raw) {
  memset(this, 0, sizeof(OptimizerInstruction));
#if ENABLE_INTERNAL_CODE_OPTIMIZER && ENABLE_CODE_OPTIMIZER
  _internals._literal_id = UNDETERMINED_LITERAL_ID;
#endif
  _raw = ins_raw;
}

int OptimizerInstruction::issue_latency() {
  return 1;
}

#if ENABLE_INTERNAL_CODE_OPTIMIZER
int OptimizerInstruction::result_latency() {
  int opcode;
  int latency = 0;
  latency = latency_dty[_type];
  if (_status & STATUS_WRITEBACK ) latency++;
  if (_type == stm || _type == ldm) {
    latency += _operand_count;
  }
  return latency;
}
#else
int OptimizerInstruction::result_latency() {
  int opcode;
  int latency = 0;
  if (_status & STATUS_WRITEBACK ) latency++;
  switch (_type) {
    case data:
    case str:
      latency=latency+1;
      break;
    case ldr:
      latency += 3;
      break;
    case ldm:
      latency += (2 + _operand_count);
      break;
    case stm:
      latency += (2 + _operand_count);
      break;
    case semaphore:
      latency += 5;
      break;
    case mult:
      latency += 3;
      break;
  }
  return latency;
}
#endif

#ifndef PRODUCT
void OptimizerInstruction::print() {
  Disassembler(tty).disasm(NULL, *_raw, -1);
  tty->cr();
}
#endif

#define DELAY_CMP(inst_a, inst_b) \
    (inst_a->_delay > inst_b->_delay ? 1 : 0)

static void isort(OptimizerInstruction** inst, int lo, int hi) {
  OptimizerInstruction* i_tmp;
  int i, j;
  
  for(i = lo+1; i <= hi; i++) {
    i_tmp = inst[i];
    j = i-1;
    while(j >= lo && DELAY_CMP(inst[j], i_tmp)) {
      inst[j+1] = inst[j];
      j--;
    }
    inst[j+1] = i_tmp;
  }
}

#define REGISTER_PC     0x8000
#define REGISTER_LR     0x4000
#define REGISTER_SP     0x2000
#define REGISTER_FP     0x800
#define REGISTER_GP     0x20

#define RESULT_USED_IN_NEXT(inst_first, inst_next) \
    (inst_first->_results & inst_next->_operands)

#if ENABLE_INTERNAL_CODE_OPTIMIZER
extern "C" {
  extern address gp_base_label;
  extern address gp_compiler_throw_ArrayIndexOutOfBoundsException_0_ptr;
}

CodeOptimizer::CodeOptimizer() {}
void  CodeOptimizer::update_aoi_stub() {
  if(_iob_stub_position == -1 && _iob_address_tracker != -1) {
    Compiler::current()->update_aoi_stub(_iob_address_tracker - (int)method->entry());
  }
}
void  CodeOptimizer::reset(CompiledMethod* cm, int* start, int* end) {
  int size = end - start;
  method = cm;
  _branch_targets.calculate(size);
  _pinned_entries.calculate(size);
  _not_instructions.calculate(size);
  _scheduablebranchs_or_abortpoint.calculate( size);
  _ins_raw_start = start;
  _ins_raw_end = end;
#ifndef PRODUCT  
  if (OptimizeCompiledCodeVerboseInternal) {
    TTY_TRACE_CR(("\t\t[CompilationContinuation size]= %d Word",size));
  }
#endif
  _ins_block_top = _ins_block_base + MAX_INSTRUCTIONS_IN_BLOCK;
  _ins_block_next = _ins_block_base;

  // Initialize the instructions
  for (int i = 0; i < MAX_INSTRUCTIONS_IN_BLOCK; i++) {
    _ins_block_base[i].init(NULL);
  }
  _iob_handler_offset =(int) ((long) &gp_compiler_throw_ArrayIndexOutOfBoundsException_0_ptr -
                                (long) &gp_base_label);
  _iob_address_tracker = -1;
  _iob_stub_position = Compiler::current()->aoi_stub_position();
  if (_iob_stub_position != -1) {
    _iob_stub_position = _iob_stub_position + (int) method->entry();
  }
}
#else
CodeOptimizer::CodeOptimizer(CompiledMethod* cm, int* start, int* end) : 
               _branch_targets((end-start)), _pinned_entries((end-start)) {
  method = cm;
  _ins_raw_start = start;
  _ins_raw_end = end;

  _ins_block_top = _ins_block_base + MAX_INSTRUCTIONS_IN_BLOCK;
  _ins_block_next = _ins_block_base;

  // Initialize the instructions
  for (int i = 0; i < MAX_INSTRUCTIONS_IN_BLOCK; i++) {
    _ins_block_base[i].init(NULL);
  }
}
#endif

CodeOptimizer::~CodeOptimizer() {
}

bool CodeOptimizer::depends_on_ins(OptimizerInstruction* ins_first,
                                   OptimizerInstruction* ins_next) {
  int index = 0;

  // doesn't support moves over these type of instructions
  if ( (ins_first->_status | ins_next->_status) & STATUS_PINNED) {
    return true;
  }
  
  if( (ins_first->_status & ins_next->_status ) & COND_SETCONDITIONAL) {
    return true;
  }

  if( ( ins_first->_status & COND_SETCONDITIONAL) && 
      ( ins_next->_status & COND_CONDITIONAL)) {
    return true;
  }

  if( ( ins_first->_status & COND_CONDITIONAL) && 
      ( ins_next->_status & COND_SETCONDITIONAL)) {
    return true;
  }

#if  ENABLE_INTERNAL_CODE_OPTIMIZER  
  if( (ins_first->_status & STATUS_SCHEDUABLE)  &&
      ins_next->_type == OptimizerInstruction::str  ) {
    return true;
  }
  if( (ins_next->_status & STATUS_SCHEDUABLE ) &&
      ins_first->_type == OptimizerInstruction::str  ) {
    return true;
  }
#endif

#if  ENABLE_INTERNAL_CODE_OPTIMIZER  
// 0xE860 = 0b 1010 1001 1110 0000
// r15 (pc) r13 (sp) r11 (fp)  r6 (jsp) r5(global pool)
//#define SPECIFIC_MEMORY_OPT 0xA860 
#define SPECIFIC_MEMORY_OPT 0x0860 

  
  bool writeback = ( ins_first->_status | ins_next->_status ) & 
                                                             STATUS_WRITEBACK;
  bool checkImm = 
       ( (ins_first->_type == OptimizerInstruction::str && 
          ins_next->_type  == OptimizerInstruction::ldr) );

  // RAW
  if(ins_first->_results & ins_next->_operands) {
    if (checkImm && !writeback) {
      if (ins_first->_imm == ins_next->_imm) return true;
    } else {
      return true;
    }
  } else {
    if( checkImm && 
        (ins_first->_results & ~SPECIFIC_MEMORY_OPT) && 
        (ins_next->_operands & ~SPECIFIC_MEMORY_OPT) ) {
      return true;
    }
  }

  checkImm = ( (ins_first->_type == OptimizerInstruction::ldr) && 
               (ins_next->_type  == OptimizerInstruction::str) );

  // WAR
  if(ins_next->_results & ins_first->_operands) {
    if (checkImm && !writeback) {
      if (ins_first->_imm == ins_next->_imm) return true;
    } else {
      return true;
    }
  } else {
    if( checkImm && (ins_next->_results & ~SPECIFIC_MEMORY_OPT) && 
        (ins_first->_operands & ~SPECIFIC_MEMORY_OPT) ) {
      return true;
    }
  }


  checkImm = ( ins_first->_type == OptimizerInstruction::str && 
               ins_next->_type == OptimizerInstruction::str);

  // WAW
  if(ins_next->_results & ins_first->_results) {
    if (checkImm && !writeback) {
      if (ins_first->_imm == ins_next->_imm) return true;
    } else {
      return true;
    }
  } else {
    if( checkImm && (ins_next->_results&~SPECIFIC_MEMORY_OPT) && 
        (ins_first->_results&~SPECIFIC_MEMORY_OPT) ) {
      return true;
  }
  }
#else

  bool writeback =( ins_first->_status | ins_next->_status ) & 
                                   STATUS_WRITEBACK;
  bool checkImm = 
       ( (ins_first->_type == OptimizerInstruction::str && 
          ins_next->_type  == OptimizerInstruction::ldr) ||
         (ins_first->_type == OptimizerInstruction::str && 
         ins_next->_type  == OptimizerInstruction::str) );

  // RAW
  if(ins_first->_results & ins_next->_operands) {
    if (checkImm && !writeback) {
      if (ins_first->_imm == ins_next->_imm) return true;
    } else {
      return true;
    }
  }

  checkImm = ( (ins_first->_type == OptimizerInstruction::ldr) && 
               (ins_next->_type  == OptimizerInstruction::str)  ||
               (ins_first->_type == OptimizerInstruction::str && 
               ins_next->_type  == OptimizerInstruction::str) );

  // WAR
  if(ins_next->_results & ins_first->_operands) {
    if (checkImm && !writeback) {
      if (ins_first->_imm == ins_next->_imm) return true;
    } else {
      return true;
    }
  }

  checkImm = ( ins_first->_type == OptimizerInstruction::str && 
               ins_next->_type == OptimizerInstruction::str);
  // WAW
  if(ins_next->_results & ins_first->_results) {
    if (checkImm && !writeback) {
      if (ins_first->_imm == ins_next->_imm) return true;
    } else {
      return true;
    }
  }
  
#endif //ENABLE_INTERNAL_CODE_OPTIMIZER

  return false;
}

bool CodeOptimizer::is_end_of_block(OptimizerInstruction* ins, int offset) {
  GUARANTEE(ins != NULL, "Invalid instruction");

#if ENABLE_INTERNAL_CODE_OPTIMIZER
  if ( ExtendBasicBlock ) {
    if ( ins->_status & STATUS_SCHEDUABLE ) {
      return false;
    }
  }
#endif

  if ( ins->_type == OptimizerInstruction::branch || 
       ins->_type == OptimizerInstruction::swi ) {
    return true;
  }

  // Check for ldr pc or mov pc
  if (ins->_results & REGISTER_PC) {
#if  ENABLE_INTERNAL_CODE_OPTIMIZER
    if ( ExtendBasicBlock ) {
      unsigned int offset =(unsigned int) ((int) ins->_imm - _iob_handler_offset);
      if (ins->_operands & REGISTER_GP && offset <= 10 * BytesPerWord ) {
        ins->_status |= STATUS_SCHEDUABLE;
        return false;
      }
    }
#endif
    return true;
  } else { 
    return false;
  }
}

#if ENABLE_NPCE
//read the npe related ldr/str from 
//relocation stream.
bool CodeOptimizer::detect_exception_table(int offset) {
  int table_item;
  int address;
  for (RelocationReader stream(method); !stream.at_end(); stream.advance()) {
    if (stream.is_npe_item()) {
      if (stream.current_item(1) == 4*offset) {
        return true; 
      }
    } 
  } 
  return false;
}
#endif //ENABLE_NPCE

void CodeOptimizer::determine_pin_status( OptimizerInstruction* ins,
                                          int offset) {
  ins->_status &= ~STATUS_PINNED;


#if ENABLE_NPCE && ENABLE_INTERNAL_CODE_OPTIMIZER
  //handler npe cases
  if ((ins->_type == OptimizerInstruction::ldr ||
         ins->_type == OptimizerInstruction::str) && 
         _scheduablebranchs_or_abortpoint.get(offset)) {
    //pinned all npe and array index related
    //schedule if the method has exception handler.
    if (_has_exception_handler) {
      ins->_status |= STATUS_PINNED;
      return;
    }
#ifndef PRODUCT        
    if (OptimizeCompiledCodeVerboseInternal) {
      TTY_TRACE(("abort point "));
      Disassembler(tty).disasm(NULL, *( ins->_raw), -1);
      TTY_TRACE_CR((""));
    }
#endif
    ins->put_result(Assembler::jsp);
    ins->_internals._abortable = ABORT_POINT_ID;
  }
#endif

#if ENABLE_INTERNAL_CODE_OPTIMIZER 
  //decoder will take some data of call info as instructions.
  //we mark those no-instructions address in _not_instructions
  //and pinned them.
  if (_not_instructions.get(offset)) {
    ins->_internals._literal_id  = NON_LITERAL_ID;
    ins->_status |= STATUS_PINNED;
    return;
  }

  //if is a inline array out of boundary exception thrower
  //mark them to create exteded basic block
  if ( (ins->_type ==OptimizerInstruction::ldr) && 
       (ins->_results & REGISTER_PC) && 
      (ins->_operands & REGISTER_GP) ) {
    unsigned int offset =(unsigned int)((int) ins->_imm - _iob_handler_offset);
    if ( offset <= 10 * BytesPerWord ) {
      ins->_status |= STATUS_SCHEDUABLE;
      ins->put_result(Assembler::jsp);
      return;
    }
  //if a standard array out of boundary exception thrower  
  //mark them to create exteded basic block
  } else if (ins->_type ==OptimizerInstruction::branch && 
       _scheduablebranchs_or_abortpoint.get(offset)) {
    ins->_status |= STATUS_SCHEDUABLE;
    ins->put_result(Assembler::jsp);
    return;
  }
#else
  if (ins->_type == OptimizerInstruction::str) {
    unsigned short imm_register = ins->_results;
    if ((imm_register & ~REGISTER_FP) &&
       (imm_register & ~REGISTER_SP) &&
       (imm_register & ~REGISTER_GP)) {
      ins->_status |= ( STATUS_PINNED | STATUS_ALIASED);
      return;
    }
  }
#endif // ENABLE_INTERNAL_CODE_OPTIMIZER

  // Check if the instruction is an osr entry
  if (_pinned_entries.get(offset)) {
    ins->_status |= ( STATUS_PINNED | STATUS_ALIASED);
    return;
  }

  // remove cmp instruction from no-move list
  if (ins->_type == OptimizerInstruction::ldm ||
      ins->_type == OptimizerInstruction::stm || 
      ins->_type == OptimizerInstruction::unknown ||
      ins->_type == OptimizerInstruction::mult ||
      ins->_type == OptimizerInstruction::branch ||      
      ins->_type == OptimizerInstruction::data_carry) {
      ins->_status |= STATUS_PINNED;
    return;
  }

  // shift ror/rrx 
  if (ins->_shift >= 2) {
    ins->_status |= STATUS_PINNED;
    return;
  }

  // Pin if uses pc
  if (ins->_results & REGISTER_PC) {
    ins->_status |= STATUS_PINNED;
    return;
  }

  if (ins->_operands & REGISTER_PC) {
    if (ins->_type != OptimizerInstruction::ldr ||
       ins->_imm > 3976 || ins->_imm < -3976) {
       ins->_status |= STATUS_PINNED;      
    }
    return;
  }

  if (ins->_type == OptimizerInstruction::ldr ||
      ins->_type == OptimizerInstruction::str ) {
#if ENABLE_NPCE && \
     !ENABLE_INTERNAL_CODE_OPTIMIZER 
   if ( detect_exception_table(offset)) {
      ins->_status |= STATUS_PINNED;
      return;
    }
#endif
  }
  return;
}

int* CodeOptimizer::unpack_block(int* ins_start, int* ins_end,
                                 int  offset) {
  _ins_block_next = _ins_block_base;

  // filter out single instruction blocks
  _ins_block_next->init(ins_start);
  unpack_instruction(_ins_block_next);
  determine_pin_status(_ins_block_next, offset);
#if ENABLE_INTERNAL_CODE_OPTIMIZER
#ifndef PRODUCT  
  if (OptimizeCompiledCodeVerboseInternal) {
    TTY_TRACE_CR(("\t[BB Head]=0x%08x",(int)_ins_block_next->_raw));
  }
#endif  
  determine_literal_id(_ins_block_next);
#endif//ENABLE_INTERNAL_CODE_OPTIMIZER

  if (is_end_of_block(_ins_block_next, offset)) {
    return ins_start;
  } else {
    ++_ins_block_next;
    ++offset;
  }

  // Currently supports only basic blocks of max size 30 instructions
  for (int* ins_curr = ins_start + 1; ins_curr <= ins_end; ins_curr++, offset++) {
    _ins_block_next->init(ins_curr);
    unpack_instruction(_ins_block_next);
    determine_pin_status(_ins_block_next, offset);

#if ENABLE_INTERNAL_CODE_OPTIMIZER
    determine_literal_id(_ins_block_next);
#else
    // the first instruction of method shouldn't be
    // moved because of _method_execution_sensor
    if ( ins_start == _ins_raw_start ) {
      _ins_block_next->_status |= STATUS_PINNED;
    }
#endif    

    // Check for branch target as well
    if ( _branch_targets.get(offset)) {
      return ins_curr - 1;
    }

    // Check for the end of block, included mov pc, rx into basic block
    if (is_end_of_block(_ins_block_next, offset)) {
      if ( _ins_block_next->_type == OptimizerInstruction::branch || 
            _ins_block_next->_type == OptimizerInstruction::swi ||
            _ins_block_next->_type == OptimizerInstruction::ldr) {
      return ins_curr - 1;
      } else {
        _ins_block_next++;
        return ins_curr;
      }
    }

    if (_ins_block_next+1 == _ins_block_top) {
      _ins_block_next++;
      return ins_curr;
    }

    ++_ins_block_next;
  }
  return ins_end;
}

#if ENABLE_INTERNAL_CODE_OPTIMIZER
//if this is a instruction to load constant,
//we assign a literal id to the ldr instruction
//to  track the link.
void CodeOptimizer::determine_literal_id(OptimizerInstruction* ins) {
  if ((ins->_type == OptimizerInstruction::ldr) &&
      (ins->_operands & REGISTER_PC) &&
      (ins->_internals._literal_id  == UNDETERMINED_LITERAL_ID) ) {
    InternalCodeOptimizer* ico = Compiler::current()->optimizer();
#ifndef PRODUCT        
    int old_literal_id= ins->_internals._literal_id ;
#endif
    int offset =(int) ins->_raw -(int) method->entry();

    //old_offset is the offset of ldr instruction accessing
    //same constant and pointed by current instruction
    // ldr rx, [pc, #xx]
    //  |
    // ldr rxx, [pc, #xx]
    int old_offset = offset +  (ins->_imm + 8);
    int id = ico->search_unbound_literal( old_offset);
    ins->_internals._literal_id  = id;
    if (id != UNDETERMINED_LITERAL_ID) {
      ico->update_unbound_literal_tracking_address(id, offset);
      if (ico->get_scheduled_unbound_literal_address(id) == 0) {
        //if the ldr link is start from previous CC
        //we record the offset and recreat link based on 
        //this fixed offset(old_offset)
        //otherwise we need to tracker the link head
        //durning scheduling
        if (old_offset < ico->_start_code_offset) {
          ico->update_scheduled_unbound_literal_tracking_address(id, 
                                                        old_offset);
        }
      }
    }
#ifndef PRODUCT      
    if (OptimizeCompiledCodeVerboseInternal) {
      TTY_TRACE(("\t\tdetermin_literal_id[%d]\n\t\t\t", offset));
      Disassembler(tty).disasm(NULL, *( ins->_raw), -1);
      TTY_TRACE_CR((""));
      TTY_TRACE_CR(("\t\t\tliteral_id[%d]=>[%d]", old_literal_id, 
                                   ins->_internals._literal_id ));
      TTY_TRACE_CR(("\t\t\t[pre offset=%d]<=[my offset=%d]", old_offset, 
                                                               offset));
    }
#endif        
  }
}
#endif // ENABLE_INTERNAL_CODE_OPTIMIZER

void CodeOptimizer::update_branch_target(OptimizerInstruction* ins, 
                                        int* ins_start, 
                                        int* ins_end) {
  if (ins->_type == OptimizerInstruction::branch) {
   // TTY_TRACE_CR(("\t\t[0%d] Unpatched BL", 
   //                (int)ins->_raw-(int)method->entry()));
     
   // Disassembler(tty).disasm(NULL, *( ins->_raw), -1);
    int offset = ((*ins->_raw & 0xffffff) | 
                 (bit(*ins->_raw, 23) ? 0xff000000 : 0)) << 2; 
#if ENABLE_INTERNAL_CODE_OPTIMIZER
   /*
    //This code will slow the eembc kxml
    int target =(int) ins->_raw  + offset + 8;
    if (target < (int) method->entry() ||  
        target > (int) ins_end) {
      return; 
    }  
    int encode = *(int*)target;
    if (((encode >> 25) & 0x7) == 5) {
    */
    if (offset == -8) {
      //this must be branch hasn't been patched
      //if the offset pointed ot other un-patched 
      //instruction, we can't mark them as branh target also.
      //changed, but need performance is not good.
      //so keep the old.
#ifndef PRODUCT
      if (OptimizeCompiledCodeVerboseInternal) {
        TTY_TRACE_CR(("\t\t[0%d] Unpatched BL", 
                (int)ins->_raw-(int)method->entry()));
      }
#endif
      return;
    }
#endif
    int* targetPc = (int*) ((int)ins->_raw + 8 + offset);
    if (targetPc >= ins_start && targetPc <= ins_end) {
      _branch_targets.set(abs(targetPc - ins_start));
    }
#if ENABLE_INTERNAL_CODE_OPTIMIZER    
    else if (_iob_stub_position != -1) {
      //if target is index out of boundary stub,
      //we mark them as schedulable branch
      //for later extended basic block work
      if (((int) targetPc-(int) method->entry()) == _iob_stub_position) {
        if (OptimizeCompiledCodeVerboseInternal) {        
          TTY_TRACE_CR(("Found a bound schedulable branch %d", 
              abs(ins->_raw - ins_start)<<2));
        }
        _scheduablebranchs_or_abortpoint.set(abs(ins->_raw - ins_start));
      };
    }
#endif      
  }
  return;
}

void CodeOptimizer::update_data_sites(OptimizerInstruction* ins,
                                      int* ins_start,
                                      int* ins_end) {
  if (ins->_type == OptimizerInstruction::data &&
      (ins->_results & REGISTER_LR) &&
      (ins->_operands & REGISTER_PC)) {
    // covers add lr, pc, #<immed>
    // followed by jump and call data
    int* targetPc = (int*) ((int)ins->_raw + 8 + ins->_imm);
    if (targetPc >= ins_start && targetPc <= ins_end) {
      // Pin all instructions between this and the target
      int i = abs(ins->_raw - ins_start);
#if ENABLE_INTERNAL_CODE_OPTIMIZER      
      // this address has identied as data
      // skip the follow code.
      if ( _not_instructions.get(i) ) {
        return ;
      }
#endif      
      for (; i < abs(targetPc - ins_start); i++) {
        _pinned_entries.set(i);
#if ENABLE_INTERNAL_CODE_OPTIMIZER
        _not_instructions.set(i);
#endif
      }
    }
  } else if (ins->_type == OptimizerInstruction::ldr && 
             ins->_operands & REGISTER_PC) {
    // covers literals that are embedded in code
#if !ENABLE_INTERNAL_CODE_OPTIMIZER
    //in internal code optimizer, this will be handler by fix_memory load 
    //function
    int* targetPc = (int*) ((int)ins->_raw + 8 + ins->_imm);
    if (targetPc >= ins_start && targetPc <= ins_end) {
      _pinned_entries.set(abs(targetPc - ins_start));
    }
#endif
  }
  return;
}

#if ENABLE_NPCE && ENABLE_INTERNAL_CODE_OPTIMIZER
//mark the npe relation instuction
//since them may result branch
//and should maintain jsp for these instruction.
void CodeOptimizer::update_abort_points(int* ins_start,
                                                  int* ins_end) {
  int table_size = Compiler::current()->
       code_generator()->npe_index();
  int index;
  int start_offset = (int) ins_start - (int) method->entry();
  for ( int i = 0; i < table_size ; i++) {
    index = ( Compiler::npe_address( i ) - start_offset ) >> 2;
    _scheduablebranchs_or_abortpoint.set(index);
  }
}
#endif

void CodeOptimizer::update_instruction_info_tables(int* ins_start, 
                                                   int* ins_end) {
  OptimizerInstruction ins;
  for (int* ins_curr = ins_start; ins_curr < ins_end; ins_curr++) {
    ins.init(ins_curr);
    unpack_instruction(&ins);
    update_branch_target(&ins, ins_start, ins_end);
    update_data_sites(&ins, ins_start, ins_end);
  }
}

void CodeOptimizer::determine_osr_entry(int* ins_start, int* ins_end) {

#if ENABLE_INTERNAL_CODE_OPTIMIZER 
  Compiler *current_compiler = Compiler::current();
  InternalCodeOptimizer* internal_optimizer = current_compiler->optimizer();
  BinaryAssembler::Label label;
  //Get schedulable branch instruction
  {
    int next = 0;
    int prev = 0;
    while ((next = 
    current_compiler->next_schedulable_branch( ins_start, prev, &label)) > 0) {
      _scheduablebranchs_or_abortpoint.set(
        (next -internal_optimizer->_start_code_offset)/4);

      _branch_targets.set(
      (next -internal_optimizer->_start_code_offset)/4, false);
      prev = next;
    }
    //if next = -2 
    //the index out of boundary linker head is in previous 
    //CC.
    if (next != -1) {
      _iob_address_tracker = prev ;
    } 
  }

  //Pin OSR entries and Entry Frames
  current_compiler->begin_pinned_entry_search();
  label = current_compiler->get_next_pinned_entry();
  while (!label.is_unused()) {
#ifndef PRODUCT   
    if (OptimizeCompiledCodeVerboseInternal) {
      TTY_TRACE(("\t***OSR Entry offset=%d,",label.position())); 
    }
#endif
    if (label.position() >= internal_optimizer->_start_code_offset ) {
#ifndef PRODUCT   
      if (OptimizeCompiledCodeVerboseInternal) {
        TTY_TRACE_CR( ("pinned at bitmap [%d]",
          ((label.position() - internal_optimizer->_start_code_offset)/4)));
      }
#endif//PRODUCT
      _pinned_entries.set(
        (label.position() - internal_optimizer->_start_code_offset)/4);
    }
    label = current_compiler->get_next_pinned_entry();
  }

  //create literal data structure
  current_compiler->begin_bound_literal_search();
  label = current_compiler->get_next_bound_literal();
  while (!label.is_unused()) {
#ifndef PRODUCT   
    if (PrintCompiledCodeAsYouGo) {
      TTY_TRACE_CR(("\tBound literal Offset is %d", label.position())); 
    }
#endif//PRODUCT
    if (label.position() >= internal_optimizer->_start_code_offset ) {
#ifndef   PRODUCT   
      if (PrintCompiledCodeAsYouGo) {
        TTY_TRACE(("\tBound literal Pinned %d \t", label.position()));
        TTY_TRACE_CR(("set bitmap at %d \t",
            ((label.position()-internal_optimizer->_start_code_offset)/4)));
      }
#endif//PRODUCT
      _not_instructions.set( (label.position() - 
              internal_optimizer->_start_code_offset)/4 );
    }
    label = current_compiler->get_next_bound_literal();
  }
#else
  for (RelocationReader stream(method); !stream.at_end(); stream.advance()) {
    if (stream.is_osr_stub()) {        
      _pinned_entries.set(stream.code_offset()/4);      
    }    
  }
#endif//ENABLE_INTERNAL_CODE_OPTIMIZER
}

#if ENABLE_INTERNAL_CODE_OPTIMIZER  
void CodeOptimizer::fix_memory_load(OptimizerInstruction* ins_to_fix,
                                         int new_index) {
  int raw_index = 0;
  InternalCodeOptimizer* ico = Compiler::current()->optimizer();

  if (ins_to_fix->_shift != 0) {
    return;
  }
  
  for (OptimizerInstruction* curr = _ins_block_base;
        curr < _ins_block_next; curr++, raw_index++) {
    if (curr == ins_to_fix) {
      break;
    }
  }
        
  if (raw_index == new_index) {
    return;
  }

  //previous instruction
  int i = raw_index - 1;
  
#if ENABLE_NPCE      
  short instruction_index_before_schedule = 
          (short) ( ins_to_fix->_raw - (int *) ico->_stop_method->entry());
  short instruction_index_after_schedule =
          (short)( instruction_index_before_schedule +  new_index - raw_index);
  int index;
#endif

  bool need_item = true;
  if ( i >= 0 ) {
    //previous instruction is in BB
    if ( ( _ins_block_base[i]._status & 
              (STATUS_EMITTED | STATUS_SCHEDUABLE) ) == 
              (STATUS_EMITTED | STATUS_SCHEDUABLE) ) {
      //if previous is scheduable branch and has been emitted
      //so cmp of array boundary has happened

      
      int load_offset = ins_to_fix->_raw - (int *)method->entry() + 
                                             new_index - raw_index;
#if ENABLE_NPCE        
      int j = raw_index - 3 ;
      if (  j >= 0  &&  !( _ins_block_base[j]._status & STATUS_EMITTED ) &&
            _ins_block_base[j]._internals._abortable == ABORT_POINT_ID) { 
        // if this is the first array access, which mean a npe may happend
        // when acess array length.
        // ldr -> raw_index -3 
        // cmp
        // b
        // ldr
        int possible_abort_point = instruction_index_before_schedule - 3;
#ifndef PRODUCT
        if (PrintCompiledCodeAsYouGo) {
          TTY_TRACE_CR(("abort a array bound check item at %d", 
                                                               load_offset <<2));
        }
#endif

        //in this case, 
        //error memory access
        //should be taken as npe 
        need_item = false;

        index = ico->get_index_of_npe_scheduled_address( 
                   possible_abort_point);
        if (index != -1) {
          // in this case
          // array element load is scheduled 
          // ahead of array length acess
          // we need update the npe information 
          // to handle this
          ico->set_npe_scheduled_address( index,  
                  instruction_index_after_schedule);
          //mark the npe related ldr has processed
          //so later he will not cleanup the updating.
          _ins_block_base[j]._status |= STATUS_EMITTED;
        }
      }
#endif

      if (need_item) {
        Compiler::current()->code_generator()->
               emit_pre_load_item( load_offset<<2, ico->_stop_code_offset);
      }
    }
  }
  
#if ENABLE_NPCE
  if (ico->get_npe_record_count() == 0) {
    return;
  }
  
  index = ico->get_index_of_npe_scheduled_address( 
                       instruction_index_before_schedule);
          
#ifndef PRODUCT
  if (PrintCompiledCodeAsYouGo) {
    tty->print( "\t\tNPE LDR old=[%d] ",
                instruction_index_before_schedule<<2 );
    tty->print_cr("new=[%d] ", instruction_index_after_schedule<<2);
    tty->print_cr("[table index]=%d \n", index);
    tty->print_cr("\t\t");
    Disassembler(tty).disasm(NULL,*( ins_to_fix->_raw), -1);
    tty->cr();
  }
#endif  
  if (index != -1  && !(ins_to_fix->_status & STATUS_EMITTED)) {
    ico->set_npe_scheduled_address( index,  instruction_index_after_schedule);
  }
#endif
}
#endif // ENABLE_INTERNAL_CODE_OPTIMIZER


#if ENABLE_INTERNAL_CODE_OPTIMIZER
bool InternalCodeOptimizer::StopInternalCodeOptimizer(CompiledMethod* cm,
                                        int curren_code_offset JVM_TRAPS){
  int i, code_length;
  int* code_begin;
  int* code_end;
    
  Compiler *compiler = Compiler::current();
    
  _stop_method = cm;
  _stop_code_offset = curren_code_offset - BytesPerWord;
  if ( _start_method == _stop_method && 
       _stop_code_offset >= _start_code_offset ) {
    // begin the code optimizing
    code_length = (_stop_code_offset - _start_code_offset) >> 2;
    code_begin = (int *)cm->entry() + (_start_code_offset>>2);
    code_end = code_begin + code_length;

    ObjectHeap::save_compiler_area_top_fast();
#ifndef PRODUCT
    if (OptimizeCompiledCodeVerboseInternal) {
      TTY_TRACE_CR(("****Internal optimization start"));
    }
#endif

#if ENABLE_NPCE    
    _npe_related_ldrs_ptr = 0;
#endif
    _unbound_literal_count = 0;
    _iob_related_ldrs_ptr = 0;
    //don't skip any un-schedulable instructions 
    //they may need to patch literal.
#if ENABLE_NPCE      
    init_npe_table(ThrowExceptionStub::npe_count  JVM_CHECK_0);
#endif

    init_unbound_literal_tables(
           compiler->get_unbound_literal_count() JVM_CHECK_0);

#ifndef PRODUCT      
    if (OptimizeCompiledCodeVerboseInternal) {          
      TTY_TRACE_CR(( "\t[ub_literal_count]=%d",
                     compiler->get_unbound_literal_count()) );
    }
#endif      

#ifndef PRODUCT                  
    if (OptimizeCompiledCodeVerboseInternal) {
      TTY_TRACE_CR(("\tAccesses of unbound literal"));
    }
#endif

    compiler->get_first_literal_ldrs(code_begin);

#if ENABLE_NPCE    
#ifndef PRODUCT                  
    if (OptimizeCompiledCodeVerboseInternal)
      TTY_TRACE_CR(("\tNPE ldr instructions"));
#endif
    compiler->get_npe_instructions(_start_code_offset);     
#endif

    _optimizer.reset(cm, code_begin, code_end);

#if ENABLE_NPCE
    //if has exception handler
    //don't do extend basic block scheduling
    Method::Raw cur_method = cm->method();
    if ( !cur_method().has_no_exception_table()  ) {
      _optimizer.set_has_exception_table(true);
    }
#endif

    _optimizer.optimize_code(JVM_SINGLE_ARG_CHECK_0);

#if ENABLE_NPCE       
    compiler->patch_null_check_stubs();
#endif

#ifndef PRODUCT
    if (OptimizeCompiledCodeVerboseInternal) {
      dump_unbound_literal_table();
    }
#endif    

    compiler->patch_unbound_literal(_start_code_offset);
    _optimizer.update_aoi_stub();
    ObjectHeap::update_compiler_area_top_fast();

#ifndef PRODUCT
    if (OptimizeCompiledCodeVerboseInternal) {
      TTY_TRACE_CR(("****Internal optimization finish"));
    }
#endif

  } else {
#ifndef PRODUCT
    if (OptimizeCompiledCodeVerboseInternal) {
      if ( _start_method != _stop_method ) {
        TTY_TRACE_CR(("Can't optimization code due to method switching!"));
      }
    }
#endif
  }

  return 0;
}

//code handler constant load in internal code scheduling
void 
CodeOptimizer::fix_pc_immediate_internal( OptimizerInstruction* ins_to_fix,
                                          int new_index) {
  // determine the location of the instruction in the original stream
  int raw_index = 0;
  
  if (ins_to_fix->_internals._literal_id  == NON_LITERAL_ID) {
    return;
  }
  if (ins_to_fix->_shift != 0) {
    return;
  }
  
  for (OptimizerInstruction* curr = _ins_block_base;
        curr < _ins_block_next; curr++, raw_index++) {
    if (curr == ins_to_fix) {
      break;
    }
  }

#ifndef PRODUCT
  int old_instruction=0;
  if (OptimizeCompiledCodeVerboseInternal) {
    old_instruction = *(ins_to_fix->_raw);
  }
#endif

  InternalCodeOptimizer* ico = Compiler::current()->optimizer();
  int old_code_offset = ins_to_fix->_raw - (int *) ico->_stop_method->entry();
  int new_code_offset  = old_code_offset +  new_index - raw_index;
  int new_imm = ins_to_fix->_imm + (raw_index - new_index)*4 ;
#ifndef PRODUCT  
  if (OptimizeCompiledCodeVerboseInternal) {
    TTY_TRACE_CR(("\t\t fix_pc_immediate_internal"));
    TTY_TRACE_CR(("\t\t\tindex old[%d]=>new[%d], ", raw_index, new_index));
    TTY_TRACE_CR( ("\t\t\toffset old[%d]=>new[%d], ",
                  old_code_offset<<2, new_code_offset<<2));
  }
#endif  
  if ( ins_to_fix->_internals._literal_id  > UNDETERMINED_LITERAL_ID) {
    int prev_emit_offset = 
         ico->get_scheduled_unbound_literal_address(
                                 ins_to_fix->_internals._literal_id );
    ico->update_scheduled_unbound_literal_tracking_address(
                ins_to_fix->_internals._literal_id , new_code_offset<<2);     
    if (prev_emit_offset != 0) {
      new_imm = prev_emit_offset - (new_code_offset<<2 ) - 8;
    } else { 
      //_literal_id related constant access instructions has not be 
      //emitted before.we are the first one.
      if ( ins_to_fix->_imm == -8) {
        return;
      } else {
        //only fix new_imm when the previous one  is in current CC
        //otherwise, new_imm = change of current instruction + old _imm.
        if ( ((old_code_offset <<  2) + ins_to_fix->_imm + 8 ) >= 
              ico->_start_code_offset ) {
          new_imm = -8;
        }
      }
    }
  }

#ifndef PRODUCT  
  if (OptimizeCompiledCodeVerboseInternal) {  
    TTY_TRACE_CR(("\t\t\timm  old[%d]=>new[%d], ",  
         ins_to_fix->_imm, new_imm));
  }
#endif  

  int new_raw_ins = *ins_to_fix->_raw;
  new_raw_ins &= 0xFFFFF000;

  // Make sure sign bit for the offset
  // is setup properly
  if (new_imm < 0) {
    new_imm = -new_imm;
    new_raw_ins &= ~0x800000;
  } else {
    new_raw_ins |= 0x800000;
  }
  new_raw_ins |= (new_imm & 0xFFF);
  *ins_to_fix->_raw = new_raw_ins;
#ifndef PRODUCT 
  if (OptimizeCompiledCodeVerboseInternal) {
    TTY_TRACE(("\t\t\t "));
    Disassembler(tty).disasm(NULL, old_instruction, -1);
    TTY_TRACE((" =>  "));
    Disassembler(tty).disasm(NULL, *(ins_to_fix->_raw), -1);
    TTY_TRACE_CR((""));
  }
#endif

  return;
}

// handle instruction not be scheduled.
bool 
CodeOptimizer::fix_pc_immediate_internal_fast(
                          OptimizerInstruction* ins_to_fix) {
  // determine the location of the instruction in the original stream
  if (ins_to_fix->_internals._literal_id  == NON_LITERAL_ID) {
    return false;
  }
  if (ins_to_fix->_shift != 0) {
    return false;
  }

#ifndef PRODUCT
  int old_instruction = 0;
  if (OptimizeCompiledCodeVerboseInternal) {
    old_instruction = *(ins_to_fix->_raw);
  }
#endif

  InternalCodeOptimizer* ico = Compiler::current()->optimizer();
  int old_code_offset = ins_to_fix->_raw - (int *) ico->_stop_method->entry();
  int new_imm = ins_to_fix->_imm;

#ifndef PRODUCT
  if (OptimizeCompiledCodeVerboseInternal) {
    TTY_TRACE_CR(("\t\t fix_pc_immediate_internal_fast"));
    TTY_TRACE_CR( ("\t\t\toffset old[%d]=>new[%d]",
                  old_code_offset<<2, old_code_offset<<2));
  }
#endif  

  if (ins_to_fix->_internals._literal_id  > UNDETERMINED_LITERAL_ID) {
    int prev_emit_offset = 
           ico->get_scheduled_unbound_literal_address(
              ins_to_fix->_internals._literal_id );
    ico->update_scheduled_unbound_literal_tracking_address(
              ins_to_fix->_internals._literal_id , old_code_offset<<2);     
    if (prev_emit_offset != 0) {
      new_imm = prev_emit_offset - (old_code_offset<<2 ) - 8;
    } else {
      if ( ins_to_fix->_imm == -8) {
        return false;
      } else {
        new_imm = -8;
      }
    }
  }
#ifndef PRODUCT  
  if (OptimizeCompiledCodeVerboseInternal) {  
    TTY_TRACE_CR(("\t\t\timm  old[%d]=>new[%d], ",  ins_to_fix->_imm, new_imm));
  }
#endif  
  int new_raw_ins = *ins_to_fix->_raw;
  new_raw_ins &= 0xFFFFF000;

  // Make sure sign bit for the offset
  // is setup properly
  if (new_imm < 0) {
    new_imm = -new_imm;
    new_raw_ins &= ~0x800000;
  } else {
    new_raw_ins |= 0x800000;
  }  

  new_raw_ins |= (new_imm & 0xFFF);
  *ins_to_fix->_raw = new_raw_ins;

#ifndef PRODUCT 
  if (OptimizeCompiledCodeVerboseInternal) {
    TTY_TRACE(("\t\t\t "));
    Disassembler(tty).disasm(0, old_instruction, -1);
    TTY_TRACE(("=>  "));
    Disassembler(tty).disasm(0, *(ins_to_fix->_raw), -1);
    TTY_TRACE_CR((""));
  }
#endif
  return true;
}

void 
CodeOptimizer::fix_branch_immediate( OptimizerInstruction* ins_to_fix, 
                                                             int new_index ) {
  // determine the location of the instruction in the original stream
  int raw_index = 0;
  if (ins_to_fix->_shift != 0) {
    return;
  }
  
  for (OptimizerInstruction* curr = _ins_block_base;
       curr < _ins_block_next; curr++, raw_index++) {
    if (curr == ins_to_fix) {
      break;
    }
  }
       
  int new_position =((int) ins_to_fix->_raw + (new_index<<2) - (raw_index<<2));
  int imm24;
  int new_raw;
  
  if (_iob_stub_position != -1) {
    imm24 = (_iob_stub_position - new_position - 8) >> 2;
  } else {
     if (_iob_address_tracker == -1) {
       imm24 = (-8)>> 2;
     } else {
       imm24 = ( _iob_address_tracker - new_position - 8) >> 2;
     }
     _iob_address_tracker = new_position;
  }

  new_raw = *ins_to_fix->_raw;
  new_raw = new_raw & 0xff000000 | imm24 & 0x00ffffff;
  *ins_to_fix->_raw = new_raw;
  return;
}

#endif //ENABLE_INTERNAL_CODE_OPTIMIZER

void CodeOptimizer::fix_pc_immediate( OptimizerInstruction* ins_to_fix,
                                      int new_index) {
  // determine the location of the instruction in the original stream
  int raw_index = 0;
  
  if (ins_to_fix->_shift != 0) {
    return;
  }
  
  for( OptimizerInstruction* curr = _ins_block_base;
       curr < _ins_block_next; curr++, raw_index++) {
    if (curr == ins_to_fix) {
      break;
    }
  }
  
  if (raw_index == new_index) {
    return;
  }

  int new_imm = ins_to_fix->_imm + (raw_index - new_index) * 4;
  int new_raw_ins = *ins_to_fix->_raw;
  new_raw_ins &= 0xFFFFF000;

  // Make sure sign bit for the offset
  // is setup properly
  if (new_imm < 0) {
    new_imm = -new_imm;
    new_raw_ins &= ~0x800000;
  } else {
    new_raw_ins |= 0x800000;
  }

  new_raw_ins |= (new_imm & 0xFFF);
  *ins_to_fix->_raw = new_raw_ins;
  return;
}

bool CodeOptimizer::build_dependency_graph(int block_size) {
  int i,j,m;
  OptimizerInstruction *temp_p, *temp_c;
  OptimizerInstruction* pinned_instructions[MAX_INSTRUCTIONS_IN_BLOCK];
  bool child_uses_result;
  int num_pinned = 0;
  bool can_be_optimized = false;
  bool no_literal_ldr  = true;
  OptimizerInstruction* insBlockCurr = _ins_block_base;

  for(i=0; i<block_size; i++) {
    temp_p = insBlockCurr+i;
    child_uses_result = false; 

    if( temp_p->_status & STATUS_PINNED) {
      pinned_instructions[num_pinned++] = temp_p;
    }
     
    for(j=i+1; j<block_size; j++) {
      temp_c = insBlockCurr+j;

      //If parent is pinned, all instructions below it are its children
      if( ( temp_p->_status & 
          ( STATUS_PINNED | STATUS_ALIASED) ) == 
          STATUS_PINNED ) {
        temp_p->_children[temp_p->_num_children++] = temp_c;
      } else if (depends_on_ins(temp_p, temp_c)) {
        temp_c->_parents[temp_c->_num_parents++] = temp_p;
        temp_p->_children[temp_p->_num_children++] = temp_c;
      }

      /* One time only set up of the following for the current child: */
      /*     - parent relationship for all pinned instructions        */
      if(temp_c - temp_p  == 1) {
        //Add all pinned instructions as parents of this child
        for(m=0; m<num_pinned; m++) {
          if (!(pinned_instructions[m]->_status & STATUS_ALIASED) ) {
            temp_c->_parents[temp_c->_num_parents++] = pinned_instructions[m];
          }
        }
      }
    }  
  }

  OptimizerInstruction* nodes_instructions[MAX_INSTRUCTIONS_IN_BLOCK];
  int number_nodes = 0;
  // found the leaf instructions & nodes instructions
  for(i=0; i<block_size; i++) {
    temp_p = insBlockCurr+i;
    if( temp_p->_num_children == 0 ) {
      temp_p->_delay = 1;
    } else {
      nodes_instructions[number_nodes++] = temp_p;
      temp_p->_delay = 0;
    }
  }

  int max_delay, instruction_delay, result_latency;
  bool is_ready;

  // calculate the _delay of nodes instructions
  while( number_nodes > 0 ) {
    for( i=0; i < number_nodes; i++ ) {
      temp_p = nodes_instructions[i];
      is_ready = true;

      for(j=0; j<temp_p->_num_children; j++) {
        temp_c = temp_p->_children[j];
        if( temp_c->_delay == 0 ) {
          // it means that it is also node instruction,
          // and hasn't got the delay value
          is_ready = false;
        }       
      }

      max_delay = 2;
      result_latency = temp_p->result_latency();
    
      if( is_ready) {
        for(j=0; j<temp_p->_num_children; j++) {
          temp_c = temp_p->_children[j];
          if( RESULT_USED_IN_NEXT(temp_p, temp_c)) {
            instruction_delay = temp_c->_delay + result_latency;
            if( temp_c->_shift ) instruction_delay++;          
              if( result_latency > 1 ) {
                can_be_optimized = true;
              }
            } else {
              instruction_delay = temp_c->_delay + 1;
            }
            if( instruction_delay > max_delay ) {
              max_delay = instruction_delay;
            }
          }
          temp_p->_delay = max_delay;
        }
      }  
      // update the notes instrucitons
      number_nodes = 0;
      for(i=0; i<block_size; i++) {
        temp_p = insBlockCurr+i;
        if( temp_p->_delay == 0 ) {
          nodes_instructions[number_nodes++] = temp_p;
        }
      }
    }

#if ENABLE_INTERNAL_CODE_OPTIMIZER
    //add for fix pc related ldr
    if (!can_be_optimized) {
#ifndef PRODUCT
      if (OptimizeCompiledCodeVerboseInternal) {
        TTY_TRACE_CR( (
"\tEnter block can't be optimized\n\t\tsize=%d\n\t\tunbound literal count=%d",
                      block_size, 
                      InternalCodeOptimizer::current()->get_unbound_literal_count()) );
      }
#endif    
    if (Compiler::current()->optimizer()->get_unbound_literal_count() != 0) {
      insBlockCurr = _ins_block_base;
      for (i=0; i<block_size; i++) {
        temp_p = insBlockCurr+i;
#ifndef PRODUCT        
        if (OptimizeCompiledCodeVerboseInternal) {
          TTY_TRACE(("\t\t"));
          Disassembler(tty).disasm(NULL, 
             *temp_p->_raw,(int) temp_p->_raw - (int)method->entry());
          TTY_TRACE_CR((""));
        }
#endif
        if ((temp_p->_operands & REGISTER_PC) &&
          (temp_p->_type == OptimizerInstruction::ldr)) {
          if (fix_pc_immediate_internal_fast(temp_p)) {
#ifndef PRODUCT        
            if (OptimizeCompiledCodeVerboseInternal) {
              TTY_TRACE_CR(("\t\tpatch instruction at  0x%08x",
                            (_ins_block_base->_raw+i)) );
            }
#endif            
            *(_ins_block_base->_raw+i) = *(temp_p->_raw);
          }
        }
      }
    }
  }
#endif//ENABLE_INTERNAL_CODE_OPTIMIZER
  return can_be_optimized;
}

bool CodeOptimizer::reorganize() {
  int  ready_count = 0;
  int  ready_index = 0;
  int  exec_count = 0;
  int  exec_index = 0;
  int currentExecuteTime = 0;
  int totalOriginalExecuteTime = 0;
  int result_latency;

  OptimizerInstruction* ready[MAX_INSTRUCTIONS_IN_BLOCK];
  OptimizerInstruction* executing[MAX_INSTRUCTIONS_IN_BLOCK];

  _schedule_index = 0;

//dump basic block before scheduling
#ifndef PRODUCT
  if (OptimizeCompiledCodeVerbose) {
    tty->cr();
    tty->print_cr("Before scheduling:");
    dumpCurrentBB();
    tty->cr();
  }
#endif

#ifndef PRODUCT
  if (OptimizeCompiledCodeVerbose) {
    // calculate the cycles number for original code
    for (OptimizerInstruction* curr = _ins_block_base; 
          curr < _ins_block_next; curr++) {
      curr->executeTime = 0;
    }
    for (OptimizerInstruction* curr = _ins_block_base;
          curr < _ins_block_next; curr++) {
      totalOriginalExecuteTime++;    
      if (totalOriginalExecuteTime < curr->executeTime ) {
        totalOriginalExecuteTime = curr->executeTime;
      }
      result_latency = curr->result_latency();
      for (int j = 0 ; j < curr->_num_children; j++) {
        OptimizerInstruction* child = curr->_children[j];
        // update the instruction executeTime
        if (result_latency > 1 ) {
          if (RESULT_USED_IN_NEXT(curr, child)) {
            int latest_executeTime = totalOriginalExecuteTime + result_latency;
            if ((child->_shift) ) latest_executeTime++;          
            if (child->executeTime < latest_executeTime ) {
              child->executeTime = latest_executeTime;
            }
          }
        }
      }
    }
  }  
#endif  

  for (OptimizerInstruction* curr = _ins_block_base;
        curr < _ins_block_next; curr++) {
    curr->executeTime = 0;
    if (curr->_num_parents == 0) {
      ready[ready_count++] = curr;
    }
  }

  if ( _ins_block_base->_raw != _ins_raw_start) {
    // consider the instruction ahead of Basic Block
    // Only for LDR instruction for overhead issue
    int *AheadInstruction = (_ins_block_base->_raw)-1;
    int InstructionValue = *AheadInstruction;
    // Load & Store word, half-word, signed & unsigned bytes
    if(  ( ((InstructionValue >> 26) & 0x3) == 1 ) || 
          ( ((InstructionValue >> 25) & 0x7) == 0 )  ) {
      // Only process Load instruction
      if( ((InstructionValue >> 20) & 0x1) == 1 ) {
        Assembler::Register ResultRegister = rd_field(InstructionValue);
        short Result_Bit = (1 << ResultRegister);
        for (ready_index = ready_count - 1; ready_index >= 0; ready_index--) {
          OptimizerInstruction* ready_ins = ready[ready_index];
          if ( ( (ready_ins->_operands) & Result_Bit ) ||
                ( (ready_ins->_results) & Result_Bit) ) {
            ready_ins->executeTime = 3;
          }
        }
      }
    }
  }

  while (ready_count > 0) {
    // default instruction use one cycle for execution
    currentExecuteTime++;
    // sort by _delay
    isort(ready, 0, ready_count - 1);

    // Get the instruction without penalty
    for (ready_index = ready_count - 1; ready_index >= 0; ready_index--) {
      OptimizerInstruction* ready_ins = ready[ready_index];
        if( currentExecuteTime >= ready_ins->executeTime ) break;
      }

    // all instructions in ready list have penalty
    if (ready_index < 0) {
      int min_executeTime = 10000;
      // choose one with smallest penalty
      for( int i = 0; i < ready_count ; i++ ) {
        if( ready[i]->executeTime < min_executeTime ) {
          min_executeTime = ready[i]->executeTime;
        }
      }
      for (ready_index = ready_count - 1; ready_index >= 0; ready_index--) { 
        if( min_executeTime == ready[ready_index]->executeTime ) { 
          break;
        }
      }
    }

    OptimizerInstruction* last_scheduled = ready[ready_index];
    if ( last_scheduled->_operands & REGISTER_PC &&
         last_scheduled->_type == OptimizerInstruction::ldr) {
      // fix up PC immediate
#if ENABLE_INTERNAL_CODE_OPTIMIZER        
      fix_pc_immediate_internal(last_scheduled, _schedule_index);
#else
      fix_pc_immediate(last_scheduled, _schedule_index);
#endif
    }
#if ENABLE_INTERNAL_CODE_OPTIMIZER 
    else if ( !(last_scheduled->_status & ( STATUS_PINNED | 
                                                        STATUS_SCHEDUABLE))  &&
                 (last_scheduled->_type == OptimizerInstruction::ldr || 
                  last_scheduled->_type == OptimizerInstruction::str) ) {
      //handle npe and array load            
      fix_memory_load(last_scheduled, _schedule_index);
    }
#endif
#if ENABLE_INTERNAL_CODE_OPTIMIZER
    else if (((last_scheduled->_status & 
                        ( STATUS_PINNED | STATUS_SCHEDUABLE)) == 
                        STATUS_SCHEDUABLE  ) && 
                last_scheduled->_type == OptimizerInstruction::branch) {
      //handle un-patched branch instruction
      fix_branch_immediate(last_scheduled, _schedule_index);
    }
#endif
    _schedule[_schedule_index++] = *last_scheduled->_raw;
#if ENABLE_INTERNAL_CODE_OPTIMIZER
    last_scheduled->_status |= STATUS_EMITTED;
#endif

#ifndef PRODUCT
    last_scheduled->_scheduled = true;
#endif
    result_latency = last_scheduled->result_latency();

    // remove scheduled instruction from the ready list
    if (ready_index != ready_count - 1) {
      jvm_memmove(ready+ready_index, ready+ready_index+1, 
              sizeof(OptimizerInstruction*) * (ready_count - ready_index));
    }
    ready_count--;
 
    // update the currentExecuteTime (cycles)
    if(  currentExecuteTime < last_scheduled->executeTime  ) {
      currentExecuteTime = last_scheduled->executeTime;
    }
    
    // add the children of the scheduled instruction to the ready list
    for (int j = 0 ; j < last_scheduled->_num_children; j++) {
      OptimizerInstruction* child = last_scheduled->_children[j];
      child->_num_parents_scheduled++;
      if (child->_num_parents_scheduled == child->_num_parents) {
        ready[ready_count++] = child;
      }
      // update the instruction executeTime
      if( result_latency > 1 ) {
        if( RESULT_USED_IN_NEXT(last_scheduled, child)) {
          int latest_executeTime = currentExecuteTime + result_latency;
          if( (child->_shift)  ) latest_executeTime++;     

          if( child->executeTime < latest_executeTime ) {
            child->executeTime = latest_executeTime;
          }
        }
      }
    }
  }
#ifndef PRODUCT
  for ( OptimizerInstruction* ins_curr = _ins_block_base;
        ins_curr < _ins_block_next; ins_curr++) {
    GUARANTEE( ins_curr->_scheduled,
               "OptimizerInstruction scheduled incorrectly" );
  }
#endif

  //dump schedule
#ifndef PRODUCT
  if (OptimizeCompiledCodeVerbose) {
    int block_size;
    block_size = _ins_block_next - _ins_block_base;
    tty->print_cr("block_size=%d", block_size);
    if( totalOriginalExecuteTime < currentExecuteTime ) {
      tty->print_cr("unhappy case:");
    } else if ( currentExecuteTime == totalOriginalExecuteTime ) {
      if( currentExecuteTime == _ins_block_next - _ins_block_base ) {
        tty->print("no penalty case and ");
      }
      tty->print_cr("unchanged case:");
    } else {
      if( currentExecuteTime == _ins_block_next - _ins_block_base ) {
        tty->print_cr("perfect case:");
      } else {
        tty->print_cr("improved case:"); 
      }
    }
    tty->cr();
    tty->print_cr("After scheduling:");
    tty->print_cr( "old cycles:%d; new cycles:%d",
                   totalOriginalExecuteTime,currentExecuteTime );
    for (int i = 0 ; i < _schedule_index; i++) {
      tty->print("%d:\t0x%08x\t", (i<<2) +(int)_ins_block_base->_raw - (int)method->entry() , _schedule[i]);
      Disassembler(tty).disasm( &_schedule[i],
                                _schedule[i],
                                ( i +(int)_ins_block_base->_raw - (int)method->entry()));
      tty->cr();
    }
    tty->cr();
    tty->print_cr("*******************");
  }
#endif
  return true;
}


void CodeOptimizer::update_compiled_code() {
  int* ins_curr = _ins_block_base->_raw;

  for (int i = 0; i < _schedule_index; i++, ins_curr++) {
    *ins_curr = _schedule[i];
  }
  return;
}


bool CodeOptimizer::optimize_code(JVM_SINGLE_ARG_TRAPS) {
  int result = 0;

  // split the code into basic blocks and reorganize
  int* ins_curr = _ins_raw_start;
  int* ins_block_end = _ins_raw_start;

  // initialize Bitset
  _branch_targets.init(JVM_SINGLE_ARG_CHECK_0);
  _pinned_entries.init(JVM_SINGLE_ARG_CHECK_0);
#if ENABLE_INTERNAL_CODE_OPTIMIZER
  _not_instructions.init(JVM_SINGLE_ARG_CHECK_0);
  _scheduablebranchs_or_abortpoint.init(JVM_SINGLE_ARG_CHECK_0);
#if ENABLE_NPCE
   update_abort_points(_ins_raw_start, _ins_raw_end);
#endif

#endif
  //_unbound_literal_ldrs.init(JVM_SINGLE_ARG_CHECK_0);
  // determine literals and callinfo sites in code
  // as well as branch targets
  update_instruction_info_tables(_ins_raw_start, _ins_raw_end);
  // determine entry points 
  determine_osr_entry(_ins_raw_start, _ins_raw_end);

  while (ins_curr < _ins_raw_end) {
    ins_block_end =
          unpack_block(ins_curr, _ins_raw_end, (ins_curr - _ins_raw_start));
#if ENABLE_INTERNAL_CODE_OPTIMIZER
#ifndef PRODUCT
    if (OptimizeCompiledCodeVerboseInternal) {
      TTY_TRACE_CR(("\t\tins_block_end=[0x%08x], ins_cur=[0x%08x]",ins_block_end, ins_curr));
    }
#endif  
#endif

    if (build_dependency_graph(ins_block_end - ins_curr + 1)) {
      reorganize();
      update_compiled_code();
    }
#if ENABLE_INTERNAL_CODE_OPTIMIZER
#ifndef PRODUCT
    if (OptimizeCompiledCodeVerboseInternal) {
      TTY_TRACE_CR( ("\tis BB end a branch target  %s",
                    _branch_targets.get(ins_block_end - _ins_raw_start)==0?"false":"true"));
    }
#endif  
#endif
    if (_branch_targets.get(ins_block_end - _ins_raw_start)) {
      ins_curr = ins_block_end + 1;
    } else {
#if ENABLE_INTERNAL_CODE_OPTIMIZER
      //fix for constant access
      ins_curr = ins_block_end + 1;
      if (ins_curr != (_ins_raw_end + 1)  && 
        !_not_instructions.get( ins_curr - _ins_raw_start)) {
        bool found =false;
        OptimizerInstruction op;
        for (OptimizerInstruction* curr = _ins_block_base;
              curr < _ins_block_top; curr++) {
          if (curr->_raw == ins_curr) {
            found = true;
            op = *curr;
#ifndef PRODUCT
            if(OptimizeCompiledCodeVerboseInternal) {
              TTY_TRACE_CR(("\t\t[0x%08x] decoded",(int)ins_curr));
            }
#endif  
            break;
          }
        }
        if (!found) {
          op.init( ins_curr);
          unpack_instruction(&op);
          determine_literal_id(&op);
        }
        if (op._operands & REGISTER_PC &&
          op._type == OptimizerInstruction::ldr) {
          if (fix_pc_immediate_internal_fast(&op)) {
#ifndef PRODUCT
            if (OptimizeCompiledCodeVerboseInternal) {
              tty->print( "literal[%d]=%s",
                      op._internals._literal_id , found?"true":"false");
              tty->print( " patching among BB [0x%08x] 0x%08x=>0x%08x\n",
                      (int)(ins_curr),*(ins_curr),*(op._raw) );
            }
#endif  
            *(ins_curr) = *(op._raw);
          }
        }
      }
#endif//ENABLE_INTERNAL_CODE_OPTIMIZER
      ins_curr = ins_block_end + 2; // skip over the branch/pc load
    }
  }
  return result;
}

void CodeOptimizer::get_shifted_reg(OptimizerInstruction* ins) {
  // decode _shifted register part of an shifter operand or an address
  const Assembler::Shift shift = Assembler::as_shift(*ins->_raw >> 5 & 0x3);
  ins->put_operand(rm_field(*ins->_raw));
  ins->_shift = 1;

  if (bit(*ins->_raw, 4)) {
    // register shift
    GUARANTEE(!bit(*ins->_raw, 7), "not a register shift");
    ins->put_operand(rs_field(*ins->_raw));
  } else {
    int imm_shift = *ins->_raw>>7 & 0x1f;
    if (imm_shift == 0) {
      if (shift == Assembler::ror) {
        ins->_status |= COND_SETCONDITIONAL;
      } else {
        ins->_shift = 0;
      }
    }
  } 
}

void CodeOptimizer::unpack_address1(OptimizerInstruction* ins) {
  // decode shifter operand part of an ins (addressing mode 1)
  if (bit(*ins->_raw, 25)) {
    // rotate immediate
    const int rotate_imm = *ins->_raw >> 7 & 0x1e; // * 2
    const int imm_8    = *ins->_raw & 0xff;
    const int imm_32   = _rotr(imm_8, rotate_imm);
    ins->_imm = imm_32;
  } else {
    // immediate/register shift
    get_shifted_reg(ins);
  }
}

void CodeOptimizer::unpack_address2(OptimizerInstruction* ins) {
  //decode address part of a standard load/store *ins->_raw (addressing mode 2)
  const bool p = bit(*ins->_raw, 24);
  const bool u = bit(*ins->_raw, 23);
  const bool w = bit(*ins->_raw, 21);
  int imm = *ins->_raw & 0xFFF;
  Assembler::Register register_rn = rn_field(*ins->_raw);

  if (ins->_type == OptimizerInstruction::str) {
    if (rn_field(*ins->_raw) != Assembler::r15) {
      ins->put_result(rn_field(*ins->_raw));
    }
//     It is a bug.
//    ins->put_operand(rn_field(*ins->_raw));
  } else {
    ins->put_operand(rn_field(*ins->_raw));
  }
  if (bit(*ins->_raw, 25)) {
    // register offset
    get_shifted_reg(ins);
  } else {
    // immediate offset
    if (!u) {
      ins->_imm = -imm;
    } else {
      ins->_imm = imm;
    }
  }
  if ((p && w) || (!p && !w)) {
    ins->_status |= STATUS_WRITEBACK;
    ins->put_result(register_rn);
  }
}

void CodeOptimizer::unpack_address3(OptimizerInstruction* ins) {
  // decode address part of a miscellaneous load/store instructions
  // (addressing mode 3)
  const bool p = bit(*ins->_raw, 24);
  const bool u = bit(*ins->_raw, 23);
  const bool w = bit(*ins->_raw, 21);
  const int imm = *ins->_raw >> 4 & 0xf0 | *ins->_raw & 0xf;
  Assembler::Register register_rn = rn_field(*ins->_raw);

  if (ins->_type == OptimizerInstruction::str) {
    ins->put_result(rn_field(*ins->_raw));
  } else {
    ins->put_operand(rn_field(*ins->_raw));
  }
  if (bit(*ins->_raw, 22)) {
    // immediate offset
    if (!u) {
      ins->_imm = -imm;
    } else {
      ins->_imm = imm;
    }
  } else {
    // register offset
    ins->put_operand(rm_field(*ins->_raw));
  }
  if (p && w) {
    ins->_status |= STATUS_WRITEBACK;
    ins->put_result(register_rn);
  }
}

bool CodeOptimizer::unpack_instruction(OptimizerInstruction* ins) {
  // decode *ins->_raw and store operands and immediates
  const Assembler::Condition cond = 
            Assembler::as_condition(*ins->_raw >> 28 & 0xf);
  if (cond == Assembler::nv) { 
    // These are just special instructions that we can ignore
    return false;
  }
  if (cond != Assembler::al) 
    ins->_status |= COND_CONDITIONAL;
    ins->_status &= ~COND_SETCONDITIONAL;

  switch (*ins->_raw >> 25 & 0x7) {
    case  0:
      if ((*ins->_raw & 0x0f900010) == 0x01000000) {
        // miscellaneous instructions
        break;
      }
      if (bit(*ins->_raw, 7) && bit(*ins->_raw, 4)) {
        // multiplies and extra load/store *ins->_rawuctions
        const int sh = *ins->_raw >> 5 & 0x3;
        if (sh == 0) {
          // multiply or swap
          if (bit(*ins->_raw, 24)) {
            // swap
            ins->_type = OptimizerInstruction::semaphore;
            ins->put_result(rd_field(*ins->_raw)); // destination
            ins->put_operand(rm_field(*ins->_raw));
            ins->put_operand(rn_field(*ins->_raw));
          } else {
            // multiply
            const bool l = bit(*ins->_raw, 23);
            const bool u = bit(*ins->_raw, 22);
            const bool a = bit(*ins->_raw, 21);
            const bool s = bit(*ins->_raw, 20);
            if (s)
              ins->_status |= COND_SETCONDITIONAL;
            ins->_type = OptimizerInstruction::mult;
            ins->put_result(rd_field(*ins->_raw));
            if (l || a) {
              ins->put_operand(rn_field(*ins->_raw));
            }
            ins->put_operand(rs_field(*ins->_raw));
            ins->put_operand(rm_field(*ins->_raw));
          }
        } else {
          // load/store
          if (bit(*ins->_raw, 20)) {
            ins->_type = OptimizerInstruction::ldr;
            ins->put_result(rd_field(*ins->_raw));
          } else {
            ins->_type = OptimizerInstruction::str;
            ins->put_operand(rd_field(*ins->_raw));
          }
          unpack_address3(ins);
        }
        break;
      }
      // fall through!
    case  1:
      // data processing instructions
      if (*ins->_raw == 0xe1800000) { 
        // instruction: nop
        break;
      }
      { 
        if (bit(*ins->_raw, 20))
              ins->_status |= COND_SETCONDITIONAL;
        const Assembler::Opcode opcode = 
                      Assembler::as_opcode(*ins->_raw >> 21 & 0xf);

        switch (opcode) {
          case Assembler::_adc: // fall through
          case Assembler::_sbc: // fall through
          case Assembler::_rsc: // fall through
            ins->put_result(rd_field(*ins->_raw));
            ins->put_operand(rn_field(*ins->_raw));
            ins->_type = OptimizerInstruction::data;
            ins->_status |= COND_SETCONDITIONAL;
            
            break;
          case Assembler::_tst: // fall through
          case Assembler::_teq: // fall through
          case Assembler::_cmp: // fall through
          case Assembler::_cmn: // fall through
            // <opcode>{<cond>} <rn>, <shifter_op>
            ins->put_operand(rn_field(*ins->_raw));
            ins->_type = OptimizerInstruction::cmp;
            break; 
          case Assembler::_mov: // fall through
          case Assembler::_mvn: // fall through
            // <opcode>{<cond>}{s} <rd>, <shifter_op>
            ins->put_result(rd_field(*ins->_raw));
            if (bit(*ins->_raw, 20)) {
                ins->_status |= COND_SETCONDITIONAL;
            }
            ins->_type = OptimizerInstruction::data;
            break;
          default :
            ins->put_result(rd_field(*ins->_raw));
            ins->put_operand(rn_field(*ins->_raw));
            if (bit(*ins->_raw, 20)) {
                  ins->_status |= COND_SETCONDITIONAL;
            }
            ins->_type = OptimizerInstruction::data;
        }
        unpack_address1(ins);
      }
      break;
    case  3:
      if (bit(*ins->_raw, 4)) {
        break;
      }
      // fall through!
    case  2:
      // load/store word and unsigned byte *ins->_rawuctions
      { 
        if (bit(*ins->_raw, 20)) {
          ins->_type = OptimizerInstruction::ldr;
          ins->put_result(rd_field(*ins->_raw));
        } else {
          ins->_type = OptimizerInstruction::str;
          ins->put_operand(rd_field(*ins->_raw));
        }
        unpack_address2(ins);
      }
      break;
    case  4:
      { // load/store multiple
        const bool p = bit(*ins->_raw, 24);
        const bool u = bit(*ins->_raw, 23);
        const bool l = bit(*ins->_raw, 20);
        if (l) {
          ins->_type = OptimizerInstruction::ldm;
        } else {
          ins->_type = OptimizerInstruction::stm;
        }
      }
      break;
    case  5:
      // branches
      ins->_type = OptimizerInstruction::branch;
      break;
    case  6:  break;
    case  7:
      if (bit(*ins->_raw, 24)) {
        // swi
        ins->_type = OptimizerInstruction::swi;
      }  
      break;
    default: SHOULD_NOT_REACH_HERE();
  }
  return true;
}

// Debug helper routines
#ifndef PRODUCT

const char* CodeOptimizer::opcodetype_name(
                 OptimizerInstruction::OpcodeType type ) {

  static const char* op_names[OptimizerInstruction::number_of_opcodetypes] = {
    "unknown", "data", "data_carry", "ldr", "str", "ldm", 
    "stm", "semaphore", "mult", "branch", "swi", "cmp"
  };

  return op_names[type];
}

const char* CodeOptimizer::reg_name(Assembler::Register reg) {
  static const char* reg_names[Assembler::number_of_registers] = {
    "r0", "r1", "r2", "r3", "r4" , "r5", "r6", "r7",
    "r8", "r9", "r10", "fp", "r12", "sp", "lr", "pc"
  };
  GUARANTEE( Assembler::r0 <= reg && 
             reg < Assembler::number_of_registers,
             "illegal register" );

  return reg_names[reg];
}

void CodeOptimizer::print_ins(OptimizerInstruction* ins) {
  unsigned short s;
  tty->print(opcodetype_name(ins->_type));
  tty->print(" ");

  for (s = 0 ; s < Assembler::number_of_registers; s++) {
    if(ins->_results & (1 << s)) {
      tty->print(reg_name((Assembler::Register)s));
      tty->print(",");
    }
  }
  tty->print(" ");
  for (s = 0 ; s < Assembler::number_of_registers; s++) {
    if(ins->_operands & (1 << s)) {
      tty->print(reg_name((Assembler::Register)s));
      tty->print(",");
    }
  }
  if (ins->_imm != 0) {
    tty->print(" #%d  ; #0x%x", ins->_imm, ins->_imm);
  }
  tty->print_cr("");
}

void CodeOptimizer::dumpCurrentBB() {
  for ( OptimizerInstruction* curr = _ins_block_base;
        curr < _ins_block_next; curr++) {
    tty->print("%d:\t0x%08x\t",((int)curr->_raw-(int)method->entry()), *curr->_raw);
    Disassembler(tty).disasm(NULL, *curr->_raw, -1);
    tty->cr();
  }
}

void CodeOptimizer::dumpInstructions(OptimizerInstruction** ins, int count) {
  for (int i = 0; i < count; i++) {
    Disassembler(tty).disasm(NULL, *ins[i]->_raw, -1);
    tty->cr();
  }
}

#endif /* #ifndef PRODUCT*/

#endif /*#if ENABLE_CODE_OPTIMIZER*/
