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

#include "incls/_precompiled.incl"

#ifndef PRODUCT
#include "incls/_Robocop.cpp.incl"

#if NOT_CURRENTLY_USED
void RobocopInstruction::must_be_after(RobocopInstruction* ri) {
  ri->must_be_before(this);
}

void RobocopInstruction::must_be_before(RobocopInstruction* ri) {
  // Allocate room for the new successor relation.
  if (successor_count() == 0) {
    _successors = (RobocopInstruction**) malloc(sizeof(RobocopInstruction*));
  } else {
    _successors = (RobocopInstruction**) realloc(_successors, (successor_count() + 1) * sizeof(RobocopInstruction*));
  }
  increment_successor_count();

  // Insert the instruction into the list of successors.
  successor_at_put(successor_count() - 1, ri);

  // Maintain predecessor count for each instruction.
  ri->increment_predecessor_count();
}

RobocopInstruction::RobocopInstruction(Instruction* instruction, int index) : 
  _instruction(instruction), _index(index)
{
  // Reset the successor and predecessor counts.
  _successor_count = 0;
  _predecessor_count = 0;
}

RobocopInstruction::~RobocopInstruction() {
  free(_successors);
}

void RobocopInstruction::adjust_bcp_displacement(int adjustment) {
  // IMPL_NOTE: Implement this.
/*
  if (_dst_a.base_is(reg)) _dst_a.adjust_displacement(adjustment);
  if (_src_a.base_is(reg)) _src_a.adjust_displacement(adjustment);
*/
}

int RobocopInstruction::adjusts_bcp() {
  if (jvm_strcmp(_instruction->format(), "inc %lr") == 0) {
    InstructionRegister* ir = (InstructionRegister*) _instruction;
    if (ir->_1 == esi) return 1;
  }
  if (jvm_strcmp(_instruction->format(), "add %lr,%lc") == 0) {
    InstructionRegisterConstant* irc = (InstructionRegisterConstant*) _instruction;
    if (irc->_1 == esi && irc->_2.is_immediate()) {
      return irc->_2.immediate();
    }
  }

  if (jvm_strcmp(_instruction->format(), "dec %lr") == 0) {
    InstructionRegister* ir = (InstructionRegister*) _instruction;
    if (ir->_1 == esi) return -1;
  }
  if (jvm_strcmp(_instruction->format(), "sub %lr,%lc") == 0) {
    InstructionRegisterConstant* irc = (InstructionRegisterConstant*) _instruction;
    if (irc->_1 == esi && irc->_2.is_immediate()) {
      return -irc->_2.immediate();
    }
  }

  return 0;
}

RobocopInstruction* RobocopInstruction::successor_at(int index) {
  assert(index >= 0 && index < successor_count(), "Index out of bounds");
  return _successors[index];
}

int RobocopInstruction::successor_index_at(int index) {
  assert(index >= 0 && index < successor_count(), "Index out of bounds");
  return _successors[index]->index();
}

void RobocopInstruction::successor_at_put(int index, RobocopInstruction* ri) {
  assert(index >= 0 && index < successor_count(), "Index out of bounds");
  _successors[index] = ri;    
}

RobocopTemplate::RobocopTemplate(int length) {
  assert(length >= 1, "Cannot allocate robocop template with a non-positive length");
  _index             = 0;
  _length            = length;
  _instruction_table = (RobocopInstruction**) malloc(length * sizeof(RobocopInstruction*));
}

RobocopInstruction* RobocopTemplate::instruction_at(int index) {
  assert(index >= 0 && index < length(), "Index out of bounds");
  return _instruction_table[index];
}

void RobocopTemplate::instruction_at_put(int index, RobocopInstruction* ri) {
  assert(index >= 0 && index < length(), "Index out of bounds");
  _instruction_table[index] = ri; 
}

void RobocopTemplate::dump(int preferred) {
  Generator::assembler()->comment("Robocop generated assembly sequence follows");
  if (CountLinearExtensions) {
    if (BytecodeNumber == (int) TemplateTable::current_template()->bytecode()) {
      tty->print_cr("Number of linear extensions for bytecode %d (%s): %d", 
                    (int) TemplateTable::current_template()->bytecode(), 
                    Bytecodes::name(TemplateTable::current_template()->bytecode()), 
                    combinations());
    }
    do_dump(preferred);
  } else {
    if (BytecodeNumber == (int) TemplateTable::current_template()->bytecode()) {
      do_dump(UseLinearExtension);
    } else {
      do_dump(preferred);
    }
  }
}

void RobocopTemplate::do_dump(int combination) {
  if (Verbose) {
    tty->print_cr("Dumping code for bytecode %d (%s) using linear extension %d/%d",
                  (int) TemplateTable::current_template()->bytecode(), 
                  Bytecodes::name(TemplateTable::current_template()->bytecode()),
                  combination,
                  combinations());   
  }
  int* predecessor_counts = (int*) malloc(sizeof(int) * _index);
  for (int i = 0; i < _index; i++) {
    predecessor_counts[i] = instruction_at(i)->predecessor_count();
  }
  _esi_adjustment = 0;
  do_dump(predecessor_counts, combination);
  free(predecessor_counts);
}

void RobocopTemplate::do_dump(int* predecessor_counts, int combination) {
  int lookfor_comb = combination;
  int* my_predecessor_counts = (int*) malloc(sizeof(int) * _index);
  for (int i = 0; i < _index; i++) {
    if (predecessor_counts[i] == 0) {
      jvm_memcpy(my_predecessor_counts, predecessor_counts, sizeof(int) * _index);
      my_predecessor_counts[i]--;
      for (int k = 0; k < instruction_at(i)->successor_count(); k++) {
        my_predecessor_counts[instruction_at(i)->successor_index_at(k)]--;
      }
      int combs = combinations(my_predecessor_counts);
      if (lookfor_comb <= combs) {
        int delta = 0; 
        for (int j = 0; j < i; j++) {
          delta += instruction_at(j)->adjusts_bcp();
        }
        delta -= _esi_adjustment;
        instruction_at(i)->adjust_bcp_displacement(delta);
        instruction_at(i)->emit();
        instruction_at(i)->adjust_bcp_displacement(-delta);
        _esi_adjustment += instruction_at(i)->adjusts_bcp();
        do_dump(my_predecessor_counts, lookfor_comb);
        free(my_predecessor_counts);
        return;
      } else {
        lookfor_comb -= combs;
      }
    }
  }
  free(my_predecessor_counts);
}

int RobocopTemplate::combinations() {
  int* predecessor_counts = (int*) malloc(sizeof(int) * _index);
  for (int i = 0; i < _index; i++) {
    predecessor_counts[i] = instruction_at(i)->predecessor_count();
  }
  int comb = combinations(predecessor_counts);
  free(predecessor_counts);
  return comb;
}

int RobocopTemplate::combinations(int* predecessor_counts) {
  int* my_predecessor_counts = (int*) malloc(sizeof(int) * _index);
  int comb = 0;
  for (int i = 0; i < _index; i++) {
    if (predecessor_counts[i] == 0) {
      jvm_memcpy(my_predecessor_counts, predecessor_counts, _index * sizeof(int));
      my_predecessor_counts[i]--;
      for (int j = 0; j < instruction_at(i)->successor_count(); j++) {
        my_predecessor_counts[instruction_at(i)->successor_index_at(j)]--;
      }
      comb += combinations(my_predecessor_counts);
    }
  }    
  free(my_predecessor_counts);
  return (comb == 0 ? 1 : comb);
}

RobocopInstruction* RobocopTemplate::add(Instruction* instruction) {
  assert(_index < _length, "Index out of bounds");
  instruction_at_put(_index++, new RobocopInstruction(instruction, _index));
  return instruction_at(_index - 1);
}
#endif

#endif
