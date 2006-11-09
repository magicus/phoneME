/*
 * @(#)CodeOptimizer_arm.hpp	1.16 06/03/27 18:46:32
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

#if ENABLE_CODE_OPTIMIZER && !ENABLE_THUMB_COMPILER

#define MAX_INSTRUCTIONS_IN_BLOCK       32

#if ENABLE_INTERNAL_CODE_OPTIMIZER
#define UNDETERMINED_LITERAL_ID  -1
#define NON_LITERAL_ID  -2
#if ENABLE_NPCE
#define ABORT_POINT_ID  -3
#endif
#endif

#define STATUS_PINNED 0x1
#define STATUS_ALIASED 0x2
#define STATUS_WRITEBACK 0x4
#define COND_CONDITIONAL 0x8
#define COND_SETCONDITIONAL 0x10
#define STATUS_EMITTED 0x20
#define STATUS_SCHEDUABLE 0x40

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
    int _literal_id;
#if ENABLE_NPCE
    int _abortable;
#endif
  } _internals;
#endif
  
#ifndef PRODUCT
  bool  _scheduled;
#endif 
 };

#if ENABLE_INTERNAL_CODE_OPTIMIZER

class Bitset : public GlobalObj {
public:
  Bitset() {
    for(int i=0; i<32; i++) _lookup[i] = ~(1<<i);
  }

  void calculate(int size) {
    size++;
    _intReps = (int)(size/32);
    _ext_size = size % 32;
    if(0 != _ext_size) _intReps++;
  }

  int init(JVM_SINGLE_ARG_TRAPS) {
    if (_intReps == 0) {
      return 0;
    }
    OopDesc* oopBits = 
             Universe::new_int_array_in_compiler_area(_intReps JVM_CHECK_0);
    _bits.set_obj(oopBits);
    return 0;
  }

  void set(int index, bool f = true) {
    index++;
    int block_pos = (index-1)/32;
    int bit_pos = (index-1)%32;
    unsigned int block = _bits.int_at(block_pos);
    f ? _bits.int_at_put(block_pos, (1 << bit_pos | block)) :
        _bits.int_at_put(block_pos, (_lookup[bit_pos] & block));
  }
  bool get(int index) { 
    index++;
    unsigned int block = _bits.int_at((index-1)/32);
    return (block >> (index-1)%32) & true;
  }
#else
class Bitset : public GlobalObj {
public:
  Bitset(int size) {
    for (int i=0; i<32; i++) _lookup[i] = ~(1<<i);
      _intReps = (int)(size/32);
      _ext_size = size % 32;
      if (0 != _ext_size) _intReps++;
  }

  int init(JVM_SINGLE_ARG_TRAPS) {
    OopDesc* oopBits = Universe::new_int_array(_intReps JVM_CHECK_0);
    _bits.set_obj(oopBits);
    return 0;
  }

  void set(int index, bool f = true) {
    int block_pos = (index-1)/32;
    int bit_pos = (index-1)%32;
    unsigned int block = _bits.int_at(block_pos);
    f ? _bits.int_at_put(block_pos, (1 << bit_pos | block)) :
        _bits.int_at_put(block_pos, (_lookup[bit_pos] & block));
  }
  bool get(int index) { 
    unsigned int block = _bits.int_at((index-1)/32);
    return (block >> (index-1)%32) & true;
  }
#endif

  ~Bitset() { 
  }
  int  size() { return (_intReps-1)*32 + _ext_size;}

protected :
  unsigned int  _intReps;
  unsigned int  _lookup[32];
  unsigned int  _ext_size;
  TypeArray     _bits;
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
  void  reset(CompiledMethod* cm, int* start, int* end);
  void  update_aoi_stub();
#endif
#if ENABLE_NPCE
  void set_has_exception_table(bool has_exception_table) {
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
  bool detect_exception_table(int offset);
#endif
#if ENABLE_INTERNAL_CODE_OPTIMIZER
#if ENABLE_NPCE
  void update_abort_points(int* ins_start, int* ins_end);
#endif
  void fix_memory_load(OptimizerInstruction* ins_to_fix, int new_index);
  void determine_literal_id(OptimizerInstruction* ins);
  void fix_branch_immediate(OptimizerInstruction* ins, int new_index);
  void fix_pc_immediate_internal(OptimizerInstruction* ins_to_fix, int new_index);
  bool fix_pc_immediate_internal_fast(OptimizerInstruction* ins_to_fix);
#endif
#ifndef PRODUCT
  void dumpCurrentBB();
  void dumpInstructions(OptimizerInstruction** ins, int count);
  void print_ins(OptimizerInstruction* ins);
#endif


 protected :
  OptimizerInstruction  _ins_block_base[MAX_INSTRUCTIONS_IN_BLOCK];

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
  Bitset _not_instructions; //data instruction
  Bitset _scheduablebranchs_or_abortpoint;//extended basic block related instruction
  int _iob_address_tracker;
  int _iob_stub_position;
  int _iob_handler_offset;
#endif
#if ENABLE_NPCE
  bool _has_exception_handler;
#endif
};

#if ENABLE_INTERNAL_CODE_OPTIMIZER
class InternalCodeOptimizer: public StackObj {
 private:
  CodeOptimizer _optimizer;
  static InternalCodeOptimizer* _current;
  int  _unbound_literal_count;
  TypeArray _unbound_literal_tracking_table;
  TypeArray _scheduled_unbound_literal_tracking_table;

#if ENABLE_NPCE
  TypeArray _npe_table;
   int _npe_related_ldrs_ptr;  
#endif    

  TypeArray _iob_table;
   int _iob_related_ldrs_ptr;
  
 public:
  void init_iob_table(int size JVM_TRAPS) {
    if (size == 0) {
      return ;
    }
    OopDesc* oop_iob = 
             Universe::new_int_array_in_compiler_area(size JVM_CHECK);
    _iob_table.set_obj(oop_iob);
    
  }
  
  void record_iob_ldrs(int cur_addr) {
    _iob_table.int_at_put(_iob_related_ldrs_ptr++, cur_addr);
  }

  void update_iob_ldrs(int index, int new_addr) {
    //if new_addr == 0 stand for the ldr instruction is after the cmp,
    //no need to record in relation items.
    _iob_table.int_at_put(index, new_addr);
  }

  int get_index_of_iob_address(unsigned int cur_addr) {
    for (int i =0 ; i < _iob_related_ldrs_ptr ; i ++ ) {
      if (_iob_table.int_at(i)  == cur_addr) {
        return i;
      }
    }
    return -1;
  }
  
#if ENABLE_NPCE
  void init_npe_table(int size  JVM_TRAPS) {
    if (size == 0 ) {
      return ;
    }
    OopDesc* oop_npe = 
             Universe::new_int_array_in_compiler_area(size JVM_CHECK);
    _npe_table.set_obj(oop_npe);
  }

  void record_npe_ldrs(unsigned int cur_addr,
                               unsigned int scheduled_addr) {
    cur_addr = (cur_addr << 16) | scheduled_addr ;
    _npe_table.int_at_put(_npe_related_ldrs_ptr++, cur_addr);
  }

  int get_npe_record_count() {
#ifndef PRODUCT      
    if (OptimizeCompiledCodeVerboseInternal) {
      dump_npe_related_ldrs();
    }
#endif
    return _npe_related_ldrs_ptr;
  }

  int get_index_of_npe_scheduled_address(unsigned int cur_addr ) {
    for ( int i =0 ; i < _npe_related_ldrs_ptr ; i ++ ) {
      if ((_npe_table.int_at(i)>>16) == cur_addr ) {
        return i;
      }
    }
    return -1;
  }

  int get_npe_scheduled_address(int index) {
    return _npe_table.int_at(index) & 0xFFFF;
  }
  
  int get_npe_cur_address(int index) {
    return  _npe_table.int_at(index) >> 16;
  } 
   
  void set_npe_scheduled_address(int index , unsigned int addr) {
    _npe_table.int_at_put(index, _npe_table.int_at(index) & 0xFFFF0000 | addr);
  }

#ifndef PRODUCT
  void dump_npe_related_ldrs() {
  for (int i = 0 ; i < _npe_related_ldrs_ptr ; i++) {
    if (OptimizeCompiledCodeVerboseInternal) {
      TTY_TRACE_CR(("[%d] npe old offset is %d, new offset is %d ",
                    i, _npe_table.int_at( i)>>16,
                    _npe_table.int_at(i)&0xFFFF));
    }
  }
}
#endif

#endif //ENABLE_NPCE

  void init_unbound_literal_tables(int size  JVM_TRAPS) {
#ifndef PRODUCT      
    if (OptimizeCompiledCodeVerboseInternal) {
      TTY_TRACE_CR(("[allocating unbound literal table]=%dWords", size));
    }
#endif     
    if (size == 0 ) {
      return ;
    }
    OopDesc* oop_table = 
             Universe::new_int_array_in_compiler_area(size JVM_CHECK);
             
    _unbound_literal_tracking_table.set_obj(oop_table);
    oop_table = Universe::new_int_array_in_compiler_area(size JVM_CHECK);
    _scheduled_unbound_literal_tracking_table.set_obj(oop_table);
  }

  int get_unbound_literal_count() {
    return _unbound_literal_count;
  }

  void record_unbound_literal_ldr_address( int addr) {
    _unbound_literal_tracking_table.int_at_put(_unbound_literal_count++, addr);
  }

  //mark the latest position(before scheduling) of ldr 
  //acess the literal indexed by literal_id
  void update_unbound_literal_tracking_address(int literal_id,  int addr) {
      _unbound_literal_tracking_table.int_at_put(literal_id, addr);
  }
  
  int get_unbound_literal_address(int literal_id) {
    return _unbound_literal_tracking_table.int_at(literal_id);
  }

  void update_scheduled_unbound_literal_tracking_address(int literal_id,
                                                                    int addr) {
    _scheduled_unbound_literal_tracking_table.int_at_put(literal_id, addr);
  }

  int get_scheduled_unbound_literal_address(int literal_id) {
    return _scheduled_unbound_literal_tracking_table.int_at(literal_id);
  }
 
  int search_unbound_literal( int addr ) {
    for (int i =0 ; i < _unbound_literal_count ; i ++) {
      if (_unbound_literal_tracking_table.int_at(i) ==  addr) {
        return i;
      }
    }
    return -1;
  }

  void dump_unbound_literal_table() {
    for (int i = 0 ; i < _unbound_literal_count ; i++) {
#ifndef PRODUCT
      if (OptimizeCompiledCodeVerboseInternal) {
        TTY_TRACE_CR(("[%d] literal offset is %d ",
                     i, _unbound_literal_tracking_table.int_at( i)));
      }
#endif
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
  
  void StartInternalCodeOptimizer(CompiledMethod* cm, int curren_code_offset) {
    _start_method = cm;
    _start_code_offset = curren_code_offset;
#if ENABLE_NPCE    
    _npe_related_ldrs_ptr=0;
#endif
    _unbound_literal_count = 0;
    _iob_related_ldrs_ptr = 0;
  }   

  bool StopInternalCodeOptimizer( CompiledMethod* cm,
                                  int curren_code_offset JVM_TRAPS );

 public:

 friend class CodeOptimizer;
 
 protected:
 // instruction fields
  static int _start_code_offset;
  static CompiledMethod* _start_method;
  int _stop_code_offset;  
  CompiledMethod* _stop_method;  
};

#endif /*#if ENABLE_INTERNAL_CODE_OPTIMIZER */

#endif /*#if ENABLE_CODE_OPTIMIZER*/
