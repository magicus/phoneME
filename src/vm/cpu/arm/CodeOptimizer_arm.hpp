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

#if ENABLE_CODE_OPTIMIZER && !ENABLE_THUMB_COMPILER

#define MAX_INSTRUCTIONS_IN_BLOCK       32

#if ENABLE_INTERNAL_CODE_OPTIMIZER
enum {
  //the insr access literal, but the index of literal isn't 
  //set yet, this value is to make sure the ins will only
  //be processed by determine_literal_id() once.
  undetermined_literal = -1,
  //the instr does not access literal
  not_access_literal = -2,
  //the instr represented by current bitmap point
  //can throw exception
  abort_point = -3,
};
#endif

enum {
 status_pinned = 0x1,
 status_aliased = 0x2,
 status_writeback = 0x4,
 cond_conditional = 0x8,
 cond_setconditional = 0x10,
 status_emitted = 0x20,
 status_scheduable = 0x40,
};

class OptimizerInstruction : public StackObj {
 friend class CodeOptimizer;

 public:
  OptimizerInstruction() : _results(0), _operands(0), _operand_count(0), 
                  _type(unknown),_status(0), _shift(0), _delay(0),
                  _num_children(0), _num_parents(0)
#if ENABLE_INTERL_INTERNAL_CODE_OPTIMIZER
                   ,_internals._literal_id (UNDETERMINED_LITERAL_ID)
#endif
  { }

  OptimizerInstruction(OptimizerInstruction& copy);
  ~OptimizerInstruction() {}

  void init(int* ins_raw);
 
  void put_operand(const Assembler::Register reg) {
    _operand_count++;    
    _operands = ((1 << reg) | _operands);
  }
  void put_result(const Assembler::Register reg) {
    _results = ((1 << reg) | _results);
  }
#if ENABLE_INTERNAL_CODE_OPTIMIZER
  //latency table of each kind of instr
  static int latency_dty[]; 
#endif  
  int issue_latency();
  int result_latency();
 
  void print();

  enum OpcodeType {
    unknown, data, data_carry, ldr, str, ldm, stm, semaphore, mult, 
    branch, swi, cmp,
    number_of_opcodetypes
  };

  OptimizerInstruction* _parents[MAX_INSTRUCTIONS_IN_BLOCK];
  OptimizerInstruction* _children[MAX_INSTRUCTIONS_IN_BLOCK];
  short _num_parents;
  short _num_parents_scheduled;
  short _num_children;
  short _delay;
  unsigned short _results;
  unsigned short _operands;

 protected:
  short _shift;
  short _operand_count;

  int   _imm;
  int*  _raw;
 
  OpcodeType  _type;

  int executeTime;
  int _status;
#if ENABLE_INTERNAL_CODE_OPTIMIZER
  union {
    //the index of the literal used by current instr
    int _literal_id;
#if ENABLE_NPCE
    //can the instr throw exception
    int _abortable;
#endif
  } _internals;
#endif
  
#ifndef PRODUCT
  bool  _scheduled;
#endif 
 };

class Bitset : public GlobalObj {
#if ENABLE_INTERNAL_CODE_OPTIMIZER
  CompilerIntArray* _bits;
  int at( const int i ) const {
    return _bits->at( i );
  }
  void at_put( const int i, const int value ) const {
    return _bits->at_put( i, value );
  }
#else
  TypeArray     _bits;
  int at( const int i ) const {
    return _bits.int_at( i );
  }
  void at_put( const int i, const int value ) const {
    return _bits.at_put( i, value );
  }
#endif
  unsigned int _size;

  static int block_index( const unsigned i ) {
    return i >> 5;
  }
  static int bit_index( const unsigned i ) {
    return i & 31;
  }
public:
  Bitset( void ) {}
 ~Bitset( void ) {}

  void calculate( const unsigned size ) {
  //  _size = block_index( size+31 );
    _size = block_index( size+32 );     // Workaround for the bug
  }

  void init(JVM_SINGLE_ARG_TRAPS) {   
    if( _size ) {
#if ENABLE_INTERNAL_CODE_OPTIMIZER
      _bits = CompilerIntArray::allocate( _size JVM_ZCHECK( _bits ) );
#else
      _bits = Universe::new_int_array( _size JVM_NO_CHECK);
#endif
    }
  }

  int get( const int i ) const { 
    return ( at(block_index(i)) >> bit_index(i) ) & 1;
  }
  void set( const int i ) const {
    const int block_pos = block_index( i );
    const unsigned block = at( block_pos ) | (1 << bit_index( i ));
    at_put( block_pos, block );
  }
  void clear( const int i ) const {
    const int block_pos = block_index( i );
    const unsigned block = at( block_pos ) &~(1 << bit_index( i ));
    at_put( block_pos, block );
  }
};

class CodeOptimizer: public StackObj {

 public:

#if ENABLE_INTERNAL_CODE_OPTIMIZER
  CodeOptimizer::CodeOptimizer();
#else
  CodeOptimizer(CompiledMethod* cm, int* start, int* end);
#endif
  ~CodeOptimizer();
   
 public:
  bool optimize_code(JVM_SINGLE_ARG_TRAPS);
#if ENABLE_INTERNAL_CODE_OPTIMIZER

  //reset the Code Optimizer status variable for the reuse 
  //of a pre-allocated Code Optimizer object.
  void  reset(CompiledMethod* cm, int* start, int* end);

  //update the entry label of unbind shared index check stub
  void  update_shared_index_check_stub();

  //check whether the method have exception table.
  void set_has_exception_handler(bool has_exception_table) {
    _has_exception_handler = has_exception_table;
  }

#endif

 protected:
  bool CodeOptimizer::reorganize();

 // instruction fields
  const bool        bit(int instr, int i) { return (instr >> i & 0x1) == 1; }
  const Assembler::Register  rn_field(int instr) { 
    return Assembler::as_register(instr >> 16 & 0xf); 
  }
  const Assembler::Register  rd_field(int instr) { 
    return Assembler::as_register(instr >> 12 & 0xf); 
  }
  const Assembler::Register  rs_field(int instr) { 
    return Assembler::as_register(instr >>  8 & 0xf); 
  }
  const Assembler::Register  rm_field(int instr) { 
    return Assembler::as_register(instr    & 0xf); 
  }

  const char* opcodetype_name(OptimizerInstruction::OpcodeType type);
  const char* reg_name(Assembler::Register reg);

  bool depends_on_ins(OptimizerInstruction* ins_block_curr, 
                      OptimizerInstruction* ins_could_interlock);
bool CodeOptimizer::depends_of_memory_on_ins(OptimizerInstruction* ins_first,
                                   OptimizerInstruction* ins_next);
  bool build_dependency_graph(int block_size);
  void update_compiled_code();

  void get_shifted_reg(OptimizerInstruction* ins);

  void unpack_address1(OptimizerInstruction* ins);
  void unpack_address2(OptimizerInstruction* ins);
  void unpack_address3(OptimizerInstruction* ins);

  bool unpack_instruction(OptimizerInstruction* ins);
  void determine_pin_status(OptimizerInstruction* ins, int offset);

  bool is_end_of_block(OptimizerInstruction* ins, int offset);
  int* unpack_block(int* ins_start, int* ins_end, int offset);
  bool can_reorganize_block();

  void update_instruction_info_tables(int* ins_start, int* ins_end);
  void update_branch_target(OptimizerInstruction* ins,
                            int* ins_start, int* ins_end);
  void update_data_sites(OptimizerInstruction* ins,
                         int* ins_start, int* ins_end);
  void determine_osr_entry(int* ins_start, int* ins_end);
  
  void fix_pc_immediate(OptimizerInstruction* ins_to_fix, int new_index);
#if ENABLE_NPCE
  //read the npe related ldr/str from 
  //relocation stream.
  bool detect_exception_table(int offset);
#endif
#if ENABLE_INTERNAL_CODE_OPTIMIZER
#if ENABLE_NPCE
  //mark the npe relation instuction
  //since them may result branch
  //and should maintain jsp for these instruction.
  void update_abort_points(int* ins_start, int* ins_end);
#endif

  //Get schedulable branch instruction
  //      < -- | pointed to itself
  //      b -- |
  //     ^
  //     | 
  //     b <--- pointed by the unbinded stub label
  //
  //
  void determine_schedulable_branch(int* ins_start, int* ins_end);

  //mark out the data entry created from literal access,
  //scheduler will skip them.
  void determine_bound_literal(int* ins_start, int* ins_end);

  // 1. record the new offset of npe_with_stub ins into the table.
  // 2. if the array load inst is scheduled ahead of arry null point check,
  //the scheduler will record the address of array load into null point 
  //exception related instrs table.
  void fix_memory_load(OptimizerInstruction* ins_to_fix, int new_index);
  
  //if this is a instruction to access literal,
  //we assign a literal id to the ldr instruction
  //to  track the link.
  void determine_literal_id(OptimizerInstruction* ins);

  //update the its targe offset after scheduling a branch instruction 
  void fix_branch_immediate(OptimizerInstruction* ins, int new_index);

  //like fix_pc_immediate, but simplified for an instr unscheduled.
  //fix_pc_immediate_for_ins_outside_block and 
  //fix_pc_immediate_for_ins_unscheduled will invoke it as
  //the fix function for specific instr.
  bool fix_pc_immediate_fast(OptimizerInstruction* ins_to_fix);

  //either fix_pc_immediate_for_ins_outside_block or fix_pc_immediate_for_ins_unscheduled
  //must be called for ins not processed by reorganize(), in order to maintain the validity of 
  //score board.
 
  //fix the immediate of instructions accessing litreal not included in a basic block
  void fix_pc_immediate_for_ins_outside_block(
        int* ins_curr, int* ins_block_end);

  //fix the immediate of instructions accessing literal  in a basic block which can be optimized
  void fix_pc_immediate_for_ins_unscheduled(int block_size);
#else
  void determine_schedulable_branch(int* ins_start, int* ins_end) {
  }
  void determine_bound_literal(int* ins_start, int* ins_end) {}
  void determine_literal_id(OptimizerInstruction* ins) {}
  void fix_pc_immediate_for_ins_outside_block(
        int* ins_curr, int* ins_block_end){}
  void fix_pc_immediate_for_ins_unscheduled(int block_size) {}
#endif
#ifndef PRODUCT
  void dumpCurrentBB();
  void dumpInstructions(OptimizerInstruction** ins, int count);
  void print_ins(OptimizerInstruction* ins);
#endif


 protected :
  OptimizerInstruction _ins_block_base[MAX_INSTRUCTIONS_IN_BLOCK];

  int _schedule[MAX_INSTRUCTIONS_IN_BLOCK];
  int _schedule_index;

  OptimizerInstruction* _ins_block_top;
  OptimizerInstruction* _ins_block_next;

  int* _ins_raw_start;
  int* _ins_raw_end;
  CompiledMethod* method;

  Bitset   _branch_targets;
  Bitset   _pinned_entries;

#if ENABLE_INTERNAL_CODE_OPTIMIZER
  enum {
    no_branch_to_index_check_stub = -1,
    not_bound = -1
  };
  //data emitted in the jitted
  Bitset _data_entries; 
  //extended basic block related instruction
  //we only take branch to a shared index 
  //check stub as scheduable.
  //since the abort point are mem access instr.
  //so it could shared the bitmap with schedulable branch.
  Bitset _scheduable_branchs_and_abortpoint;
  //emitted branch to index check stub
  int _address_of_last_emitted_branch_to_index_check_stub;
  //address of emitted index check stub
  int _address_of_shared_emitted_index_check_stub;
  //offset from gp base of global index check handler
  int _gp_offset_of_index_check_handler;
  bool _has_exception_handler;
#endif

};

#if ENABLE_INTERNAL_CODE_OPTIMIZER
class InternalCodeOptimizer: public StackObj {
 private:
  CodeOptimizer _optimizer; //current code scheduler
  static InternalCodeOptimizer* _current;//this
  int  _unbound_literal_count;
  //score board record the offset of latest ldr ins accessing each literal before scheduling.
  CompilerIntArray* _score_board_of_unbound_literal_access_ins_before_scheduling;
  //score board record the offset of latest ldr ins accessing each literal after scheduling
  //current ins could calculate its offset_imm based on the value on the score board.
  CompilerIntArray* _score_board_of_unbound_literal_access_ins_after_scheduling;

#if ENABLE_NPCE
  //table record the position before and after scheduling of a 
  //LDR instruction pointed by a NullCheckStub.
  //
  //
  CompilerIntArray* _npe_ins_with_stub_table;
  int _npe_ins_with_stub_counter;//table index  
#endif    
 public:
  
#if ENABLE_NPCE
  //allocate the table to hold the null point related LDR
  //instrs. The allocated size is the counter of all the LDR 
  //throwing null point exception. But the scheduler will
  //only record the LDR associated with null check stub into 
  //the table later.
  void init_npe_ins_with_stub_table(const int size JVM_TRAPS) {
    CompilerIntArray* p = NULL;
    if( size ) {
      p = CompilerIntArray::allocate( size JVM_NO_CHECK );
    }
    _npe_ins_with_stub_table = p;
  }

  enum {
    cur_offset_width = 16, //offset before scheduling
    cur_offset_mask = 0xFFFF0000,
    scheduled_offset_width = 16,
    scheduled_offset_mask = 0xFFFF,
  };
  
  //record the LDR associated with null check stub into 
  //the table later. The table is a int array. The high 16bits
  //of each element is the instr offset before scheduling.
  //the low 16bits is the offset after scheduling.
  void record_npe_ins_with_stub(unsigned int cur_offset,
                               unsigned int scheduled_offset) {
    cur_offset = (cur_offset << scheduled_offset_width) |
                          scheduled_offset ;
    _npe_ins_with_stub_table->at_put(_npe_ins_with_stub_counter++, cur_offset);
  }

  int record_count_of_npe_ins_with_stub() {
#ifndef PRODUCT      
    if (OptimizeCompiledCodeVerboseInternal) {
      dump_npe_related_ldrs();
    }
#endif
    return _npe_ins_with_stub_counter;
  }

  //get the table index of the record whose scheduled offset equal 
  //offset param
  int index_of_scheduled_npe_ins_with_stub(unsigned offset ) const {
    for ( int i = 0 ; i < _npe_ins_with_stub_counter ; i ++ ) {
      if ((_npe_ins_with_stub_table->at(i)>>scheduled_offset_width) == offset ) {
        return i;
      }
    }
    return -1;
  }

  //get the scheduled npe instr offset index by index param
  int scheduled_offset_of_npe_ins_with_stub(int index) const {
    return _npe_ins_with_stub_table->at(index) & scheduled_offset_mask;
  }

  //get the offset of npe instrs before scheduling
  int offset_of_npe_ins_with_stub(int index) const {
    return _npe_ins_with_stub_table->at(index) >> scheduled_offset_width;
  } 
   
  void set_scheduled_offset_of_npe_ins_with_stub(int index , unsigned int offset) const {
    _npe_ins_with_stub_table->at_put(index, _npe_ins_with_stub_table->at(index) & cur_offset_mask | offset);
  }

#ifndef PRODUCT
  void dump_npe_related_ldrs( void ) const {
  for (int i = 0 ; i < _npe_ins_with_stub_counter ; i++) {
    VERBOSE_SCHEDULING_AS_YOU_GO(("[%d] npe old offset is %d, new offset is %d ",
                    i, _npe_ins_with_stub_table->at(i) >> scheduled_offset_width,
                    _npe_ins_with_stub_table->at(i) & scheduled_offset_mask));
    }
  }
#endif
#endif //ENABLE_NPCE

  //allocate a table to record the unbound literals
  void init_unbound_literal_tables(const int size JVM_TRAPS) {
    VERBOSE_SCHEDULING_AS_YOU_GO(("[allocating unbound literal table]=%dWords", size));
    if (size == 0 ) {
      return ;
    }

    {
      CompilerIntArray* p = CompilerIntArray::allocate( size JVM_ZCHECK( p ) );
      _score_board_of_unbound_literal_access_ins_before_scheduling = p;
    }

    {
      CompilerIntArray* p = CompilerIntArray::allocate( size JVM_NO_CHECK );
      _score_board_of_unbound_literal_access_ins_after_scheduling = p;
    }
  }

  int get_unbound_literal_count( void ) const  {
    return _unbound_literal_count;
  }

  //record the offset of instrs access literals.
  //for literal with multi-access, scheduler only records the earliest one
  void record_offset_of_unbound_literal_access_ins( int offset ) {
    _score_board_of_unbound_literal_access_ins_before_scheduling->at_put(_unbound_literal_count++, offset);
  }

  //mark the latest position(before scheduling) of ldr 
  //acess the literal indexed by literal_id
  void update_offset_of_unbound_literal_access_ins(int literal_id, int offset) const {
      _score_board_of_unbound_literal_access_ins_before_scheduling->at_put(literal_id, offset);
  }

  //record the offset of latest emitted literal access ins associated with the 
  //literal indexed by literal_id
  void update_offset_of_scheduled_unbound_literal_access_ins( int literal_id,
                                                              int offset) const {
    _score_board_of_unbound_literal_access_ins_after_scheduling->at_put(literal_id, offset);
  }

  //get the offset of latest emitted literal access ins associated with the 
  //literal indexed by literal_id
  int offset_of_scheduled_unbound_literal_access_ins(int literal_id) const {
    return _score_board_of_unbound_literal_access_ins_after_scheduling->at(literal_id);
  }

   //get the literal_id whose latest position(before scheduling) equal param 
   //offset
  int index_of_unbound_literal_access_ins( int offset ) const {
    for (int i =0 ; i < _unbound_literal_count ; i ++) {
      if (_score_board_of_unbound_literal_access_ins_before_scheduling->at(i) == offset) {
        return i;
      }
    }
    return -1;
  }

  void dump_unbound_literal_table( void ) const {
    for (int i = 0 ; i < _unbound_literal_count ; i++) {
      VERBOSE_SCHEDULING_AS_YOU_GO(("[%d] literal offset is %d ",
        _score_board_of_unbound_literal_access_ins_before_scheduling->at( i )));
    }
  }
  
  InternalCodeOptimizer() {
    _current = this;
    
  }
  
  ~InternalCodeOptimizer() {
    _current = NULL;
  }
 
  static InternalCodeOptimizer* current() {
    return _current;
  }

  //record the begin address of current cc
  void prepare_for_scheduling_of_current_cc(CompiledMethod* cm, int current_code_offset) {
    _start_method = cm;
    _start_code_offset = current_code_offset;
  }   

  //scheduling the instruction in current cc. 
  bool schedule_current_cc( CompiledMethod* cm,
                                  int current_code_offset JVM_TRAPS );

 public:

 friend class CodeOptimizer;
 
 protected:
  // begin offset of current cc
  static int _start_code_offset;
  static CompiledMethod* _start_method;
  // end offset  of current cc
  int _stop_code_offset;  
  CompiledMethod* _stop_method;  
};

#endif /*#if ENABLE_INTERNAL_CODE_OPTIMIZER */

#endif /*#if ENABLE_CODE_OPTIMIZER*/
