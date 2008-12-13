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

#ifndef PRODUCT

// The Robocop stuff is temporarily disabled. 

#if NOT_CURRENTLY_USED
class RobocopInstruction: public GlobalObj {
 public:
  void must_be_after  (RobocopInstruction* ri);
  void must_be_before (RobocopInstruction* ri);

 private:
  RobocopInstruction(Instruction* instruction, int index);
  ~RobocopInstruction();
  
  int  predecessor_count()           { return _predecessor_count; }
  int  successor_count()             { return _successor_count;   }
  int  index()                       { return _index;             }

  void increment_predecessor_count() { _predecessor_count++; }
  void decrement_predecessor_count() { _predecessor_count--; }
  void increment_successor_count()   { _successor_count++;   }
  void decrement_successor_count()   { _successor_count--;   }

  RobocopInstruction* successor_at      (int index);
  int                 successor_index_at(int index);
  void                successor_at_put  (int index, RobocopInstruction* ri);

  int  adjusts_bcp             ();
  void adjust_bcp_displacement (int adjustment);

  // Emit the robocop instruction by emitting the embedded instruction.
  void emit() { _instruction->emit(); }

  // Instruction and index into robocop template.
  Instruction*          _instruction;
  int                   _index;

  // Number of predecessor instructions.
  int                   _predecessor_count;

  // Successor instructions.
  RobocopInstruction**  _successors;
  int                   _successor_count;

  // Friends.
  friend class RobocopTemplate;
};

class RobocopTemplate {
 public:
  RobocopTemplate(int length);

  // Add an instruction to the robocop template.
  RobocopInstruction* add(Instruction* instruction);

  // Dump the instruction in the template using the given assembler.
  void dump(int preferred);

  // Get the number of different legal linear extensions.
  int  combinations();

 private: 
  int                  _index;
  int                  _length;
  RobocopInstruction** _instruction_table;

  // Adjustment accumulator for allowing the bytecode pointer adjustment to be placed anywhere in the template.
  int                  _esi_adjustment;

  int                  index()  { return _index; }
  int                  length() { return _length; }

  RobocopInstruction*  instruction_at    (int index);
  void                 instruction_at_put(int index, RobocopInstruction* ri);

  int                  combinations(int* predecessor_counts);
  void                 do_dump(int combination);
  void                 do_dump(int* predecessor_counts, int combination);
};
#endif

#endif
