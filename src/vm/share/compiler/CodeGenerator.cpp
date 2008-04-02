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
#include "incls/_CodeGenerator.cpp.incl"

#if ENABLE_COMPILER

void CodeGenerator::load_from_location(Value& result, jint index, 
                                       Condition cond) {
  const BasicType type = result.stack_type();
  LocationAddress address(index, type);
  load_from_address(result, type, address, cond);
}

void CodeGenerator::store_to_location(Value& value, jint index) {
  const BasicType type = value.stack_type();
  LocationAddress address(index, type);
  store_to_address(value, type, address);
}

void CodeGenerator::flush_frame(JVM_SINGLE_ARG_TRAPS) {
  frame()->flush(JVM_SINGLE_ARG_NO_CHECK);
}

#ifndef ARM
// The code below works for ARM, but it has its own version to attempt
// to minimize register stalls

void CodeGenerator::load_from_object(Value& result, Value& object, jint offset,
                                     bool null_check JVM_TRAPS) {
  if (null_check) {
    maybe_null_check(object JVM_CHECK);
  }
  FieldAddress address(object, offset, result.type());
  load_from_address(result, result.type(), address);
}
#endif

void CodeGenerator::store_to_object(Value& value, Value& object, jint offset,
                                    bool null_check JVM_TRAPS) {
  const BasicType type = value.type();
  if (null_check) {
#if ENABLE_NPCE
    if(need_null_check(object)){
      maybe_null_check_by_npce(object, false, true, type JVM_CHECK);
      FieldAddress address(object, offset, type);
      store_to_address_and_record_offset_of_exception_instr(value, type, address);
      return;
    }
#else
#if ENABLE_ARM_V7
    bool could_use_xenon_features = 
      is_inline_exception_allowed(ThrowExceptionStub::rte_null_pointer JVM_CHECK);
    if (!could_use_xenon_features)
#endif
      maybe_null_check(object JVM_CHECK);
#endif
  }
  FieldAddress address(object, offset, type);
  store_to_address(value, type, address);
}
#if !ENABLE_ARM_V7
//there is own version which uses (ld|st)r(*)2 instructions
void CodeGenerator::load_from_array(Value& result, Value& array, Value& index) {
  const BasicType type = result.type();
  IndexedAddress address(array, index, type);
  load_from_address(result, type, address);
}

void CodeGenerator::store_to_array(Value& value, Value& array, Value& index) {
  const BasicType type = value.type();
  IndexedAddress address(array, index, type);
  store_to_address(value, type, address);
}
#endif
void CodeGenerator::check_free_space(JVM_SINGLE_ARG_TRAPS) {
  if (has_overflown_compiled_method()) {
    Throw::out_of_memory_error(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
}

void CodeGenerator::ensure_sufficient_stack_for(int index, BasicType kind) {
  int adjusted_index = index + (is_two_word(kind) ? 1 : 0);
  const int max_execution_stack_count = 
    Compiler::root()->method()->max_execution_stack_count();

  int inliner_stack_count = 2;
#if ENABLE_INLINE
  inliner_stack_count += 3;
#endif

  int max_index = max_execution_stack_count + inliner_stack_count -
    Compiler::current()->num_stack_lock_words() - 1;
  GUARANTEE(adjusted_index <= max_index || 
            method()->is_native() || method()->is_abstract(), 
            "max index should be max");
  if (adjusted_index > frame()->stack_pointer()) {
    frame()->set_stack_pointer(max_index);
  }
}

// Use this for instructions that have not been implemented; the compiled
// code will not be recompiled again and again, but we switch execution to
// interpreter in the middle of the compiled method 
void CodeGenerator::go_to_interpreter(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_COMMENT(("Continue in the interpreter"));

#if ENABLE_INLINE
  if (Compiler::is_inlining()) {
    // Cannot deoptimize when the frame is omitted.
    // Must abort current compilation discarding any generated code.
    // The current method is marked as impossible to compile to prevent  
    // inlining on the next compilation attempt for the caller
    Compiler::abort_active_compilation(true JVM_THROW);
  }
#endif

  if (omit_stack_frame()) {
    // Cannot deoptimize when the frame is omitted.
    // Must abort current compilation discarding any generated code.
    Compiler::abort_active_compilation(false JVM_THROW);
  }

  // Terminate the current compilation string.
  Compiler::closure()->terminate_compilation();

  // Flush the frame.
  flush_frame(JVM_SINGLE_ARG_CHECK);

  // Call the deoptimize VM routine.
  call_vm((address) deoptimize, T_VOID JVM_NO_CHECK_AT_BOTTOM);
}

// Use this for code that cannot be composed now, but may be compilable after
// a recompilation (e.g., unloaded classes etc.)
void CodeGenerator::uncommon_trap(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_COMMENT(("Uncommon trap"));

  if (GenerateROMImage) {
    // We can't deal with uncommon traps in precompiled code.
    Compiler::abort_active_compilation(false JVM_THROW);
  }

  if (omit_stack_frame()) {
    // Cannot deoptimize when the frame is omitted.
    // Must abort current compilation discarding any generated code.
    Compiler::abort_active_compilation(false JVM_THROW);
  }

#if ENABLE_INLINE
  if (Compiler::is_inlining()) {
    // Cannot deoptimize when the frame is omitted.
    // Must abort current compilation discarding any generated code.
    // 
    // NOTE: if there is an inlinable callee that generates an 
    // uncommon trap when compiled and this callee is never invoked, the
    // caller will repeatedly be compiled.
    // We mark the callee as impossible-to-compile to prevent further 
    // attempts to inline it. This can degrade performance if the callee is 
    // a hot method.
    Compiler::abort_active_compilation(true JVM_THROW);
  }
#endif

#if ENABLE_PERFORMANCE_COUNTERS
  jvm_perf_count.uncommon_traps_generated ++;
#endif

  // Terminate the current compilation string.
  Compiler::closure()->terminate_compilation();

  // Flush the frame.
  flush_frame(JVM_SINGLE_ARG_CHECK);

  // Call the uncommon trap VM routine.
  call_vm((address) ::uncommon_trap, T_VOID JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::osr_entry(bool force JVM_TRAPS) {
  GUARANTEE(!Compiler::is_inlining(),
            "OSR stubs not supported for inlined methods");

  if (!omit_stack_frame() &&
      !GenerateROMImage && (force || (!Compiler::is_in_loop()))) {
    // Make sure it's possible to conform to this entry.
    frame()->conformance_entry(false);

    //cse
    frame()->wipe_notation_for_osr_entry();
        
    COMPILER_COMMENT(("OSR entry"));
    Label osr_entry;
    bind(osr_entry);

    OSRStub* stub = OSRStub::allocate( bci(), osr_entry JVM_NO_CHECK );
    if( stub ) {
      stub->insert();
    }
  }
}

inline bool CodeGenerator::is_commutative( const BytecodeClosure::binary_op op) {
  return ((0
  #define COMMUTATIVE( name ) | (1 << BytecodeClosure::bin_##name)
    COMMUTATIVE( add )
    COMMUTATIVE( mul )
    COMMUTATIVE( and )
    COMMUTATIVE( or  )
    COMMUTATIVE( xor )
    COMMUTATIVE( min )
    COMMUTATIVE( max )
  #undef COMMUTATIVE
    ) >> op ) & 1;
}

inline bool CodeGenerator::is_reversible( const BytecodeClosure::binary_op op ) {
  return ((0
  #define REVERSIBLE( name ) | (1 << BytecodeClosure::bin_##name)
    REVERSIBLE( add )
    REVERSIBLE( sub )
    REVERSIBLE( mul )
    REVERSIBLE( and )
    REVERSIBLE( or  )
    REVERSIBLE( xor )
    REVERSIBLE( min )
    REVERSIBLE( max )
  #undef REVERSIBLE
    ) >> op ) & 1;
}

BytecodeClosure::binary_op CodeGenerator::reverse_operation( const BytecodeClosure::binary_op op) {
  GUARANTEE(is_reversible(op), "sanity check");
  if (is_commutative(op)) return op;
  GUARANTEE(op == BytecodeClosure::bin_sub, "only possibility left");
  return BytecodeClosure::bin_rsb;
}

void CodeGenerator::int_unary(Value& result, Value& op1, 
                              BytecodeClosure::unary_op op JVM_TRAPS) {
  if (op1.is_immediate()) {
    int_constant_fold(result, op1, op JVM_NO_CHECK_AT_BOTTOM);
  } else {
    int_unary_do(result, op1, op JVM_NO_CHECK_AT_BOTTOM);
  }
}

void CodeGenerator::int_binary(Value& result, Value& op1, Value& op2, 
                               BytecodeClosure::binary_op op JVM_TRAPS) {
  if (op2.is_immediate()) {
    if (op1.is_immediate()) {
      // both operands are immediate: do constant folding
      int_constant_fold(result, op1, op2, op JVM_NO_CHECK_AT_BOTTOM);
    } else {
      int_binary_do(result, op1, op2, op JVM_NO_CHECK_AT_BOTTOM);
    }
  } else if (op1.is_immediate() && is_reversible(op)) {
    int_binary_do(result, op2, op1, reverse_operation(op) JVM_NO_CHECK_AT_BOTTOM);
  } else {
    if (op1.is_immediate()) op1.materialize();
    int_binary_do(result, op1, op2, op JVM_NO_CHECK_AT_BOTTOM);
  }
}

void CodeGenerator::long_unary(Value& result, Value& op1,
                               BytecodeClosure::unary_op op JVM_TRAPS) {
  if (op1.is_immediate()) {
    long_constant_fold(result, op1, op JVM_NO_CHECK_AT_BOTTOM);
  } else {
    long_unary_do(result, op1, op JVM_NO_CHECK_AT_BOTTOM);
  }
}

void CodeGenerator::long_binary(Value& result, Value& op1, Value& op2,
                                BytecodeClosure::binary_op op JVM_TRAPS) {
  if (op2.is_immediate()) {
    if (op1.is_immediate()) {
      // both operands are immediate: do constant folding
      long_constant_fold(result, op1, op2, op JVM_NO_CHECK_AT_BOTTOM);
    } else {
      long_binary_do(result, op1, op2, op JVM_NO_CHECK_AT_BOTTOM);
    }
  } else if (op1.is_immediate() && is_reversible(op)) {
    long_binary_do(result, op2, op1,
                   reverse_operation(op) JVM_NO_CHECK_AT_BOTTOM);
  } else {
    if (op1.is_immediate()) op1.materialize();
    long_binary_do(result, op1, op2, op JVM_NO_CHECK_AT_BOTTOM);
  }
}

#if ENABLE_FLOAT
void CodeGenerator::float_unary(Value& result, Value& op1,
                               BytecodeClosure::unary_op op JVM_TRAPS) {
  if (op1.is_immediate()) {
    float_constant_fold(result, op1, op JVM_NO_CHECK_AT_BOTTOM);
  } else {
    float_unary_do(result, op1, op JVM_NO_CHECK_AT_BOTTOM);
  }
}

void CodeGenerator::float_binary(Value& result, Value& op1, Value& op2,
                                BytecodeClosure::binary_op op JVM_TRAPS) {
  // We don't do constant folding for binary floating operations
  float_binary_do(result, op1, op2, op JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::double_unary(Value& result, Value& op1,
                               BytecodeClosure::unary_op op JVM_TRAPS) {
  if (op1.is_immediate()) {
    double_constant_fold(result, op1, op JVM_NO_CHECK_AT_BOTTOM);
  } else {
    double_unary_do(result, op1, op JVM_NO_CHECK_AT_BOTTOM);
  }
}

void CodeGenerator::double_binary(Value& result, Value& op1, Value& op2,
                                BytecodeClosure::binary_op op JVM_TRAPS) {
  // We don't do constant folding for binary floating operations
  double_binary_do(result, op1, op2, op JVM_NO_CHECK_AT_BOTTOM);
}
#endif

void CodeGenerator::int_constant_fold(Value& result, Value& op1, Value& op2,
                                      BytecodeClosure::binary_op op JVM_TRAPS) {
  GUARANTEE(op1.is_immediate() && op2.is_immediate(),
            "Both operands must be immediate to do constant folding");

  jint result_imm;
  jint op1_imm = op1.as_int();
  jint op2_imm = op2.as_int();

  switch (op) {
    case BytecodeClosure::bin_sub  : 
      result_imm = op1_imm - op2_imm;
      break;
    case BytecodeClosure::bin_add  : 
      result_imm = op1_imm + op2_imm;
      break;
    case BytecodeClosure::bin_and  : 
      result_imm = op1_imm & op2_imm;
      break;
    case BytecodeClosure::bin_xor  : 
      result_imm = op1_imm ^ op2_imm;
      break;
    case BytecodeClosure::bin_or   : 
      result_imm = op1_imm | op2_imm;
      break;
    case BytecodeClosure::bin_shr  : 
      result_imm = op1_imm >> (op2_imm & 0x1f);
      break;
    case BytecodeClosure::bin_shl  : 
      result_imm = op1_imm << (op2_imm & 0x1f);
      break;
    case BytecodeClosure::bin_ushr : 
      result_imm = (juint)op1_imm >> (op2_imm & 0x1f);
      break;
    case BytecodeClosure::bin_mul  : 
      result_imm = op1_imm * op2_imm;
      break;
    case BytecodeClosure::bin_max  : 
      result_imm = max(op1_imm,op2_imm);
      break;
    case BytecodeClosure::bin_min  : 
      result_imm = min(op1_imm,op2_imm);
      break;
    case BytecodeClosure::bin_div  : 
      { 
        if (op2_imm == 0) {
          op1.materialize();
          int_binary_do(result, op1, op2, op JVM_CHECK);
          return;
        }
        if ((unsigned int)op1_imm == 0x80000000 && op2_imm == -1) {
          result_imm = op1_imm;
        } else {
          result_imm = op1_imm / op2_imm;
        }
        break;
      }

    case BytecodeClosure::bin_rem  :
    {
        if (op2_imm == 0) {
          op1.materialize();
          int_binary_do(result, op1, op2, op JVM_CHECK);
          return;
        }
        if ((unsigned int)op1_imm == 0x80000000 && op2_imm == -1) {
          result_imm = 0;
        } else {
          result_imm = op1_imm % op2_imm;
        }
        break;
      }
    default : 
      /* to shut comiler up */
      result_imm = 0;
      SHOULD_NOT_REACH_HERE();        
      break;
  }
  result.set_int(result_imm);
}

void CodeGenerator::int_constant_fold(Value& result, Value& op1,
                                      const BytecodeClosure::unary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  GUARANTEE(op1.is_immediate(), "Must be immediate to do constant folding");
  jint result_imm;
  jint op1_imm = op1.as_int();

  switch (op) {
    case BytecodeClosure::una_neg  : 
      result_imm = -op1_imm;
      break;
    case BytecodeClosure::una_abs  : 
      result_imm = op1_imm < 0 ? -op1_imm : op1_imm;
      break;
    default : 
      SHOULD_NOT_REACH_HERE();        
      /* to shut comiler up */
      return;
  }
  result.set_int(result_imm);
}

void CodeGenerator::long_constant_fold(Value& result, Value& op1, Value& op2,
                                       const BytecodeClosure::binary_op op JVM_TRAPS) {
  GUARANTEE(op1.is_immediate() && op2.is_immediate(),
            "Both operands must be immediate to do constant folding");
  
  jlong result_imm;
  jlong op1_imm = op1.as_long();
  jlong op2_imm = op2.type() == T_LONG ? op2.as_long() : 0;

  switch (op) {
    case BytecodeClosure::bin_sub  :
      result_imm = op1_imm - op2_imm;
      break;
    case BytecodeClosure::bin_add  :
      result_imm = op1_imm + op2_imm;
      break;
    case BytecodeClosure::bin_and  :
      result_imm = op1_imm & op2_imm;
      break;
    case BytecodeClosure::bin_xor  :
      result_imm = op1_imm ^ op2_imm;
      break;
    case BytecodeClosure::bin_or   :
      result_imm = op1_imm | op2_imm;
      break;
    case BytecodeClosure::bin_shr  :
      result_imm = op1_imm >> (op2.as_int() & 0x3f);
      break;
    case BytecodeClosure::bin_shl  :
      result_imm = op1_imm << (op2.as_int() & 0x3f);
      break;
    case BytecodeClosure::bin_ushr :
      result_imm = (julong)op1_imm >> (op2.as_int() & 0x3f);
      break;
    case BytecodeClosure::bin_mul  :
      result_imm = op1_imm * op2_imm;
      break;
    case BytecodeClosure::bin_max  :
      result_imm = op1_imm > op2_imm ? op1_imm : op2_imm;
      break;
    case BytecodeClosure::bin_min  :
      result_imm = op1_imm < op2_imm ? op1_imm : op2_imm;
      break;
    case BytecodeClosure::bin_div  : 
      { 
        if (op2_imm == 0) {
          op1.materialize();
          long_binary_do(result, op1, op2, op JVM_CHECK);
          return;
        }
        if ((jlong)op1_imm == min_jlong && op2_imm == -1) {
          result_imm = op1_imm;
        } else {
          result_imm = op1_imm / op2_imm;
        }
        break;
      }

    case BytecodeClosure::bin_rem  :
      { 
        if (op2.as_long() == 0) {
          op1.materialize();
          long_binary_do(result, op1, op2, op JVM_CHECK);
          return;
        }
        if ((jlong)op1_imm == min_jlong && op2.as_long() == -1) {
          result_imm = 0;
        } else {
          result_imm = op1_imm % op2.as_long();
        }
        break;
      }

    default : 
      SHOULD_NOT_REACH_HERE();
      /* to shut comiler up */
      return;
  }

  result.set_long(result_imm);
}

void CodeGenerator::long_constant_fold(Value& result, Value& op1, 
                                       const BytecodeClosure::unary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  GUARANTEE(op1.is_immediate(), 
            "Operand must be immediate to do constant folding");
  jlong result_imm = 0; /* IMPL_NOTE: -Wuninitialized (sh3) */
  jlong op1_imm = op1.as_long();

  switch (op) {
    case BytecodeClosure::una_neg  : 
      result_imm = -op1_imm;
      break;
    case BytecodeClosure::una_abs  : 
      result_imm = op1_imm < 0 ? -op1_imm : op1_imm;
      break;
    default : 
      SHOULD_NOT_REACH_HERE();        
      /* to shut comiler up */
      return;
  }
  result.set_long(result_imm);
}

#if ENABLE_FLOAT
void CodeGenerator::float_constant_fold(Value& result, Value& op1, 
                                        const BytecodeClosure::unary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  GUARANTEE(op1.is_immediate(), 
            "Operand must be immediate to do constant folding");
  GUARANTEE(op == BytecodeClosure::una_neg || op == BytecodeClosure::una_abs,
            "Sanity");

  if (op == BytecodeClosure::una_neg) { 
    result.set_raw_int(op1.as_raw_int() ^ 0x80000000);
  } else { 
    result.set_raw_int(op1.as_raw_int() & ~0x80000000);
  }
}

void CodeGenerator::double_constant_fold(Value& result, Value& op1, 
                                         const BytecodeClosure::unary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  GUARANTEE(op1.is_immediate(), 
            "Operand must be immediate to do constant folding");
  GUARANTEE(op == BytecodeClosure::una_neg || op == BytecodeClosure::una_abs,
            "Sanity");
  const jlong flipper = (MSW_FIRST_FOR_DOUBLE != MSW_FIRST_FOR_LONG)
         // Longs and doubles have opposite endianness
         ? ((jlong) 1) << 31
         // Longs and doubles have the same endianness
         : ((jlong) 1) << 63;
  if (op == BytecodeClosure::una_neg) { 
    result.set_raw_long(op1.as_raw_long() ^ flipper);
  } else { 
    result.set_raw_long(op1.as_raw_long() & ~flipper);
  }
}
#endif


static bool compare( const BytecodeClosure::cond_op condition, const int x, const int y ) {
  switch (condition) {
    case BytecodeClosure::null:
    case BytecodeClosure::eq: return x == y;
    case BytecodeClosure::nonnull:
    case BytecodeClosure::ne: return x != y;
    case BytecodeClosure::lt: return x <  y;
    case BytecodeClosure::le: return x <= y;
    case BytecodeClosure::gt: return x >  y;
    case BytecodeClosure::ge: return x >= y;
  }
  SHOULD_NOT_REACH_HERE();
  return false;
}

void CodeGenerator::branch(int destination JVM_TRAPS) {
  COMPILER_COMMENT(("Branching to location %d", destination));

  Compiler::closure()->set_next_bytecode_index(destination);
  if (destination <= Compiler::bci()) {
     check_timer_tick(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
}


void CodeGenerator::branch_if(BytecodeClosure::cond_op condition,
       int destination, Value& op1, Value& op2, const bool flags_set JVM_TRAPS)
{  
  if (op1.is_immediate()) {
    if (op2.is_immediate()) {
      if (compare(condition, op1.as_int(), op2.as_int())) {
        branch(destination JVM_NO_CHECK_AT_BOTTOM);
      } else { 
        // elide since it is a nop
      }
    } else { 
      branch_if(BytecodeClosure::reverse(condition), destination,
                op2, op1, flags_set JVM_NO_CHECK_AT_BOTTOM);
    }
  } else { 
    if( OptimizeForwardBranches ) {
      const int next_bci = Compiler::closure()->next_bytecode_index();
      if( destination > next_bci && destination - next_bci < 15 ) {
        const bool opt = forward_branch_optimize(next_bci, condition,
                                        destination, op1, op2 JVM_CHECK);
        if (opt) {
          return;
        }
      }
    }
#if ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS
    // Zero comparison may alter carry flag
    // 'eq' and 'ne' are not sensitive to carry
    // 'lt' and 'ge' must be tranlated to 's' and 'ns'
    // 'gt' and 'le" cannot be translated
    if( !flags_set || condition == BytecodeClosure::gt
                   || condition == BytecodeClosure::le ) {
      cmp_values(op1, op2, condition);
    } else {
      switch( condition ) {
        case BytecodeClosure::lt:
          condition = BytecodeClosure::negative; break;
        case BytecodeClosure::ge:
          condition = BytecodeClosure::positive; break;
      }
    }
#else
    cmp_values(op1, op2, condition);
#endif
    conditional_jump(condition, destination, true JVM_NO_CHECK_AT_BOTTOM);
  }
}

bool CodeGenerator::is_inline_exception_allowed(int rte JVM_TRAPS) {
  Method* m = Compiler::root()->method();
  const bool has_monitors = m->access_flags().is_synchronized() ||
                            m->access_flags().has_monitor_bytecodes();
  if (has_monitors) {
    return false;
  }

  if (m->exception_table() == NULL) {
    // This covers most of the cases
    return true;
  }

  GUARANTEE(rte == ThrowExceptionStub::rte_null_pointer || 
            rte == ThrowExceptionStub::rte_array_index_out_of_bounds,"sanity");

  const int handler_bci = m->exception_handler_bci_for(
    ThrowExceptionStub::exception_class(rte), bci() JVM_NO_CHECK);
  return handler_bci == -1;
}

void CodeGenerator::maybe_null_check(Value& value JVM_TRAPS) {
  if (need_null_check(value)) {
    null_check(value JVM_CHECK);
    // Any other location using the same register can also have its
    // nonnull indication set
    if (!value.must_be_null()) {
      frame()->set_value_must_be_nonnull(value);
    }
  }
}

#if ENABLE_NPCE
void CodeGenerator::maybe_null_check_by_npce(Value& value, bool fakeldr, bool is_quick, BasicType type_of_data JVM_TRAPS) {
  if (need_null_check(value)) {
    null_check_by_npce(value, fakeldr, is_quick, type_of_data JVM_CHECK);
    // Any other location using the same register can also have its
    // nonnull indication set
    frame()->set_value_must_be_nonnull(value);
  }
}
#endif //ENABLE_NPCE

void CodeGenerator::move(Value& dst, ExtendedValue& src, Condition cond) {
  if (src.is_value()) {
    // XValue represents a Value
    move(dst, src.value(), cond); 
  } else if (src.is_oop()) {
    // XValue represents an Oop
    move(dst, src.oop(), cond);
  } else {
    // XValue represents a local still in memory
    load_from_location(dst, src.index(), cond);
  }
}

void CodeGenerator::branch_if_do(BytecodeClosure::cond_op condition,
                                 Value& op1, Value& op2, 
                                 int destination JVM_TRAPS) {
  cmp_values(op1, op2, condition);
  conditional_jump(condition, destination, true JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::conditional_jump(const BytecodeClosure::cond_op condition,
                                     const int destination,
                                     const bool assume_backward_jumps_are_taken
                                     JVM_TRAPS) {
  if ((assume_backward_jumps_are_taken && destination <= bci()) ||
      Compiler::current()->is_branch_taken(bci())) {
    Label fall_through;
    conditional_jump_do(BytecodeClosure::negate(condition), fall_through);
    COMPILER_COMMENT(("Creating continuation for fallthrough to bci = %d",
                      next_bci() ));
    CompilationContinuation::insert(
                      next_bci(), fall_through JVM_CHECK);
    branch(destination JVM_NO_CHECK_AT_BOTTOM);
  } else {
    Label branch_taken;
    conditional_jump_do(condition, branch_taken);
    COMPILER_COMMENT(("Creating continuation for target bci = %d", 
                      destination));
    CompilationContinuation::insert(destination,
                                    branch_taken JVM_NO_CHECK_AT_BOTTOM);
  }
}

#if ENABLE_APPENDED_CALLINFO
void CodeGenerator::append_callinfo_record(const int code_offset
                                           JVM_TRAPS) {
  const int number_of_tags = frame()->virtual_stack_pointer() + 1;
  const int root_bci = Compiler::root()->compiler_bci();

  callinfo_writer()->start_record(code_offset, root_bci, 
                                number_of_tags JVM_CHECK);
  frame()->fill_callinfo_record(callinfo_writer());
  callinfo_writer()->commit_record();

#ifdef AZZERT
  // Read back and verify this record.
  const CompiledMethod * const cm = compiled_method();
  CallInfoRecord callinfo(cm, (address)cm->entry() + code_offset);
  GUARANTEE(callinfo.bci() == root_bci, "Sanity");
#endif
}
#endif // ENABLE_APPENDED_CALLINFO

OopDesc* CodeGenerator::finish( void ) {
  {
    // Insert terminating sentinel.
    COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(sentinel);
    generate_sentinel();
  }

#if ENABLE_APPENDED_CALLINFO
  _callinfo_writer.commit_table();
#endif
  // Shrink compiled method object to correct size
  const jint code_size = this->code_size();
  const jint relocation_size = this->relocation_size();
  INCREMENT_COMPILER_PERFORMANCE_COUNTER(relocation, relocation_size);

  CompiledMethod::Raw cm( compiled_method() );
  cm().shrink(code_size, relocation_size);
#if ENABLE_TTY_TRACE
  if( Verbose || PrintCompilation ) {
    if (VerbosePointers) {
      TTY_TRACE_CR((" [compiled method (0x%x) size=%d, code=%d, reloc=%d, "
                    "entry=0x%x]",
        cm().obj(), cm().object_size(), code_size, relocation_size,
        cm().entry() ));
    } else {
      TTY_TRACE_CR((" [compiled method size=%d, code=%d, reloc=%d]",
        cm().object_size(), code_size, relocation_size ));
    }
  }

  if( PrintCompiledCode && OptimizeCompiledCodeVerbose ) {
    tty->print_cr("Before Optimization:");
    cm().print_code_on(tty);
  }
#endif  
  return cm;
}

class ForwardBranchOptimizer : public BytecodeClosure {
  CodeGenerator* _cg;
  enum State {
    Start, Abort,
    Load, LoadEtc, LoadEtcGoto, LoadEtcReturn,  LoadEtcStopLoadEtcDone,
    // Special states that indicate success
    Iinc, Goto
  };
  State         _state, _next_state;
  int           _bci,   _next_bci, _final_bci, _iinc_index, _iinc_delta;
  int           _raw_goto_target;
  int           _etc_true_start, _etc_false_start, _etc_length;
  BasicType     _load_type;
  ExtendedValue  _load_false, _load_true;

public:
  bool run(Method* method, CodeGenerator* cg,
              int start_bci,
              BytecodeClosure::cond_op condition,
              int destination, Value& op1, Value& op2 JVM_TRAPS);

      // Loads & stores
  virtual void push_int   (jint    value JVM_TRAPS);
  virtual void push_long  (jlong   value JVM_TRAPS);
#if ENABLE_FLOAT
  virtual void push_float (jfloat  value JVM_TRAPS);
  virtual void push_double(jdouble value JVM_TRAPS);
#endif
  virtual void push_obj   (Oop* value JVM_TRAPS);
  virtual void load_local(BasicType kind, int index JVM_TRAPS);
  virtual void store_local(BasicType kind, int index JVM_TRAPS);
  virtual void increment_local_int(int index, jint offset JVM_TRAPS);
  virtual void branch(int dest JVM_TRAPS);
  virtual void return_op(BasicType kind JVM_TRAPS);

  // Stack operations
  virtual void nop(JVM_SINGLE_ARG_TRAPS) { 
    JVM_IGNORE_TRAPS;
    simple_instruction(false); 
  }
  virtual void pop(JVM_SINGLE_ARG_TRAPS) { 
    JVM_IGNORE_TRAPS;
    simple_instruction(false); 
  }
  virtual void pop2(JVM_SINGLE_ARG_TRAPS) { 
    JVM_IGNORE_TRAPS;
    simple_instruction(false); 
  }
  virtual void dup(JVM_SINGLE_ARG_TRAPS) { 
    JVM_IGNORE_TRAPS;
    simple_instruction(false); 
  }
  virtual void dup2(JVM_SINGLE_ARG_TRAPS) { 
    JVM_IGNORE_TRAPS;
    simple_instruction(false); 
  }
  virtual void dup_x1(JVM_SINGLE_ARG_TRAPS) { 
    JVM_IGNORE_TRAPS;
    simple_instruction(false); 
  }
  virtual void dup2_x1(JVM_SINGLE_ARG_TRAPS) { 
    JVM_IGNORE_TRAPS;
    simple_instruction(false); 
  }
  virtual void dup_x2(JVM_SINGLE_ARG_TRAPS) { 
    JVM_IGNORE_TRAPS;
    simple_instruction(false); 
  }
  virtual void dup2_x2(JVM_SINGLE_ARG_TRAPS) { 
    JVM_IGNORE_TRAPS;
    simple_instruction(false); 
  }
  virtual void swap(JVM_SINGLE_ARG_TRAPS) { 
    JVM_IGNORE_TRAPS;
    simple_instruction(false); 
  }

  // Array operations
  virtual void array_length(JVM_SINGLE_ARG_TRAPS)
      { simple_instruction(true); JVM_IGNORE_TRAPS; }
  virtual void load_array(BasicType /*kind*/ JVM_TRAPS)
      { simple_instruction(true); JVM_IGNORE_TRAPS; }
  virtual void store_array(BasicType /*kind*/ JVM_TRAPS)
      { simple_instruction(true); JVM_IGNORE_TRAPS; }
  // get/set for dynamic class loading
  virtual void get_field(int /*index*/ JVM_TRAPS)
      { simple_instruction(true); JVM_IGNORE_TRAPS; }
  virtual void put_field(int /*index*/ JVM_TRAPS)
      { simple_instruction(true); JVM_IGNORE_TRAPS; }
  // Object and array allocation
  virtual void new_object(int /*index*/ JVM_TRAPS)
      { simple_instruction(true); JVM_IGNORE_TRAPS; }
  virtual void new_basic_array(int /*type*/ JVM_TRAPS)
      { simple_instruction(true); JVM_IGNORE_TRAPS; }
  virtual void new_object_array(int /*index*/ JVM_TRAPS)
      { simple_instruction(true); JVM_IGNORE_TRAPS; }
  virtual void new_multi_array(int /*index*/, int /*num_of_dims*/ JVM_TRAPS) 
      { simple_instruction(true); JVM_IGNORE_TRAPS; }
  virtual void fast_get_field(BasicType /*field_type*/, int /*field_offset*/
                              JVM_TRAPS) 
      { simple_instruction(true); JVM_IGNORE_TRAPS; }
  virtual void fast_put_field(BasicType /*field_type*/, int /*field_offset*/
                              JVM_TRAPS) 
      { simple_instruction(true); JVM_IGNORE_TRAPS; }

  // Unary arithmetic operations
  virtual void neg(BasicType /*kind*/ JVM_TRAPS)
      { simple_instruction(false); JVM_IGNORE_TRAPS; }

  // Conversion operations
  virtual void convert(BasicType /*from*/, BasicType /*to*/ JVM_TRAPS)
      { simple_instruction(false); JVM_IGNORE_TRAPS; }

  virtual void binary(BasicType kind, binary_op op JVM_TRAPS) { 
    JVM_IGNORE_TRAPS;
    bool can_error =
        (kind == T_INT || kind == T_LONG) && (op == bin_div || op == bin_rem);
    simple_instruction(can_error);
  }

  virtual void aload_0_fast_get_field_1(BasicType /*field_type*/ JVM_TRAPS) {
    // Do nothing. The forward branch will be aborted.
    JVM_IGNORE_TRAPS;
  }
#if !ENABLE_CPU_VARIANT
  virtual void aload_0_fast_get_field_n(int /*bytecode*/ JVM_TRAPS) {
    // Do nothing. The forward branch will be aborted.
    JVM_IGNORE_TRAPS;
  }
#endif //!ENABLE_CPU_VARIANT
  
private:  
  void simple_instruction(bool has_exception);
  ExtendedValue* push_simple(BasicType kind);
  bool verify_duplication();
#if USE_COMPILER_COMMENTS
  void show_comments(int /*bci*/, int /*end*/);
#else
  void show_comments(int /*bci*/, int /*end*/) {}
#endif
};

bool CodeGenerator::forward_branch_optimize(int next_bci,
                                            BytecodeClosure::cond_op condition,
                                            int destination,
                                            Value& op1, Value& op2 JVM_TRAPS) {
  ForwardBranchOptimizer opt;
  bool result = opt.run(method(), this, next_bci,
                        condition, destination, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
  return result;
}

bool ForwardBranchOptimizer::run(Method* method, CodeGenerator* cg,
              int start_bci,
              BytecodeClosure::cond_op condition,
              int destination, Value& op1, Value& op2 JVM_TRAPS) {
  BytecodeClosure::initialize(method);
  int bci = start_bci;
  _final_bci = destination;
  _state = Start;
  _cg    = cg;

  for (int i = 0; bci < _final_bci; i++) {
    _next_bci = method->next_bci(bci);
    _next_state = Abort;
    const Bytecodes::Code code = method->bytecode_at(bci);
    method->iterate_bytecode(bci, this, code JVM_CHECK_0);
    bci = _next_bci;
    if ((_state = _next_state) == Abort) {
      return false;
    }
  }

  BytecodeCompileClosure *bcc = Compiler::closure();
  VirtualStackFrame* frame = cg->frame();

  switch(_state) {
    case Start:
      show_comments(start_bci, _final_bci);
      // ignoring branch to next instruction
      return true;

    case LoadEtcStopLoadEtcDone: {
      show_comments(start_bci, _final_bci);
      Value result(_load_type);
      if (_load_true.is_value() && _load_true.value().in_register()) {
        cg->if_then_else(result, condition,
                         op1, op2, _load_true, _load_false JVM_CHECK_0);
      } else {
        cg->if_then_else(result, BytecodeClosure::negate(condition), 
                         op1, op2, _load_false, _load_true JVM_CHECK_0);
      }

      result.set_flags((jubyte)(_load_false.flags() & _load_true.flags()));

      frame->push(result);
      bcc->set_next_bytecode_index(_etc_true_start);
      return true;
    }                          


    case Goto: {
      // Branch is over a single "goto"
      show_comments(start_bci, _final_bci);
      cg->branch_if_do(BytecodeClosure::negate(condition), op1, op2,
                       _raw_goto_target JVM_CHECK_0);
      bcc->set_next_bytecode_index(_final_bci);
      return true;
    }

    case Iinc: {
      // Branch is over a single "iinc"
      show_comments(start_bci, _final_bci);
      Value index_value(T_INT);
      Value result(T_INT);
      frame->value_at(index_value, _iinc_index);
      frame->clear(_iinc_index);
      // Increment index_value if the condition is false.
      cg->if_iinc(result, BytecodeClosure::negate(condition), op1, op2, 
                  index_value, _iinc_delta JVM_CHECK_0);
      frame->value_at_put(_iinc_index, result);
      bcc->set_next_bytecode_index(_final_bci);
      return true;
    }

    default:
      // Not in an optimizing state.
      return false;
  }
}

void ForwardBranchOptimizer::push_int   (jint    value JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  ExtendedValue* current_load = push_simple(T_INT);
  if (current_load != NULL) { 
    current_load->is_value(T_INT);
    current_load->value().set_int(value);
  }
}

void ForwardBranchOptimizer::push_long  (jlong   value JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  ExtendedValue* current_load = push_simple(T_LONG);
  if (current_load != NULL) { 
    current_load->is_value(T_LONG);
    current_load->value().set_long(value);
  }
}

#if ENABLE_FLOAT
void ForwardBranchOptimizer::push_float (jfloat  value JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  ExtendedValue* current_load = push_simple(T_FLOAT);
  if (current_load != NULL) { 
    current_load->is_value(T_FLOAT);
    current_load->value().set_float(value);
  }
}

void ForwardBranchOptimizer::push_double(jdouble value JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  ExtendedValue* current_load = push_simple(T_DOUBLE);
  if (current_load != NULL) { 
    current_load->is_value(T_DOUBLE);
    current_load->value().set_double(value);
  }
}
#endif

void ForwardBranchOptimizer::push_obj   (Oop* value JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  ExtendedValue* current_load = push_simple(T_OBJECT);
  if (current_load != NULL) { 
    current_load->set_obj(value);
  }
}

void ForwardBranchOptimizer::load_local(BasicType kind, int index JVM_TRAPS)  { 
  JVM_IGNORE_TRAPS;
  ExtendedValue* current_load = push_simple(kind);
  if (current_load != NULL) { 
    VirtualStackFrame* frame = _cg->frame();
    frame->value_at(*current_load, Compiler::current_local_base() + index);
  }
}


ExtendedValue* ForwardBranchOptimizer::push_simple(BasicType kind) {
  switch(_state) {
    case Start:
      // This is the first instruction of the sequence
      _load_type = kind;
      _next_state = Load;
      _etc_false_start = _next_bci;
      // Indicate that this is the result for the "false" branch
      return &_load_false;

    case LoadEtcGoto:
    case LoadEtcReturn:
      // The first instruction after a goto/return
        _etc_true_start = _next_bci;
      if (kind == _load_type && verify_duplication() && 
         (_state == LoadEtcReturn || ((_final_bci - _next_bci) == _etc_length))) {
        // This is the same type of load, and the rest of our code precisely
        // matches what we have before
        _next_state = LoadEtcStopLoadEtcDone;
        // And we successfully optimized
        _next_bci = _final_bci = _etc_true_start + _etc_length;
        // Indicate that this is the result for the "false" branch
        return &_load_true;
      } else {
        // Abort
        return NULL;
      }
    default:
      simple_instruction(false);
      return NULL;
  }
}

void ForwardBranchOptimizer::store_local(BasicType /*kind*/, int index JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  if (_state == Load && _next_bci == _final_bci) { 
    // Optimize Load + Store as if it were an if-then-else
    VirtualStackFrame* frame = _cg->frame();
    // TRUE:   local = local
    frame->value_at(_load_true, Compiler::current_local_base() + index);
    frame->clear(index);
    _etc_true_start = bci();
    // We fix things up so we look like if-then-else
    _next_state  = LoadEtcStopLoadEtcDone;
  } else { 
    simple_instruction(false);
  }
}

void ForwardBranchOptimizer::increment_local_int(int index, jint offset JVM_TRAPS){ 
  JVM_IGNORE_TRAPS;
  if (_state == Start && _next_bci == _final_bci) { 
    // Optimize this if it is the sole instruction
    _next_state = Iinc;
    _iinc_index = Compiler::current_local_base() + index;
    _iinc_delta = offset;
  } else {
    simple_instruction(false);
  }
}

void ForwardBranchOptimizer::simple_instruction(bool has_exception) { 
  switch(_state) {
    case Load: case LoadEtc:
      if (has_exception) {
        TypeArray::Raw exception_table = method()->exception_table();
        if (exception_table().length() == 0) { 
          _next_state = LoadEtc;
        }
      } else { 
        _next_state = LoadEtc;
      }
      break;
    default:
      break;
  }
}

void ForwardBranchOptimizer::branch(int dest JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  if (_next_bci == _final_bci && dest > _next_bci) {
    _final_bci = dest;
    switch(_state) {
      case Start:            
        _next_state = Goto; 
        _raw_goto_target = dest;
        _final_bci = _next_bci;
        break;
      case Load: case LoadEtc:
        _etc_length = bci() - _etc_false_start;
        _next_state = LoadEtcGoto;
        break;
      default: 
        ;
    }
  }
}

void ForwardBranchOptimizer::return_op(BasicType /*kind*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  if (_next_bci == _final_bci) {
    switch(_state) { 
      case Load: case LoadEtc:
        _next_state = LoadEtcReturn;
        _final_bci = method_size();
        _etc_length = _next_bci - _etc_false_start;
        break;
    }
  }
}  

bool ForwardBranchOptimizer::verify_duplication() { 
  Method* m = method();
  int start1 = _etc_false_start;
  int start2 = _etc_true_start;
  int length = _etc_length;
  if (start2 + length > m->code_size()) {
    return false;
  }
  for (int i = 0; i < _etc_length; i++) {
    if (m->ubyte_at(start1 + i) != m->ubyte_at(start2 + i)) { 
      return false;
    }
  }
  return true;
}


#if USE_COMPILER_COMMENTS
void ForwardBranchOptimizer::show_comments(int bci, int end) {
  if (GenerateCompilerComments) {
    COMPILER_COMMENT(("Optimization of forward branch"));
    while (bci < end) { 
      FixedArrayOutputStream output;
      method()->print_bytecodes(&output, bci);
      _cg->comment(output.array());
      bci = next_bci(bci);
    }
  }
}
#endif

#endif
