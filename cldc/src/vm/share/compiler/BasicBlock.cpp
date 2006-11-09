/*
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
#include "incls/_BasicBlock.cpp.incl"

#if USE_COMPILER_STRUCTURES

/*
 * BasicBlock::compute_branch_entries is used by the Compiler and the
 * ConstantPoolRewriter. We just need to know if a bci bas an entry count
 * of 0, 1, or greater than 1.
 *
 * We need to make sure that very large number of entry counts would not
 * overflow a byte.
 */
#define ADD_BRANCH_ENTRY(array, i) \
    array[(i)] = ((array[(i)] > 10) ? (array[(i)]) : ((jubyte)(array[(i)]+1)))

#if ENABLE_NPCE && ENABLE_INTERNAL_CODE_OPTIMIZER
bool BasicBlock::is_null_point_exception_throwable(Bytecodes::Code code) {
  switch(code) {
    case Bytecodes::_iaload                  :
    case Bytecodes::_laload                  :
    case Bytecodes::_faload                  :
    case Bytecodes::_daload                  :
    case Bytecodes::_aaload                  :
    case Bytecodes::_baload                  :
    case Bytecodes::_caload                  :
    case Bytecodes::_saload                  :
    case Bytecodes::_istore                  :
    case Bytecodes::_lstore                  :
    case Bytecodes::_fstore                  :
    case Bytecodes::_dstore                  :
    case Bytecodes::_astore                  :
    case Bytecodes::_iastore                 :
    case Bytecodes::_lastore                 :
    case Bytecodes::_fastore                 :
    case Bytecodes::_dastore                 :
    case Bytecodes::_aastore                 :
    case Bytecodes::_bastore                 :
    case Bytecodes::_castore                 :
    case Bytecodes::_sastore                 :
    case Bytecodes::_getstatic               :
    case Bytecodes::_putstatic               :
    case Bytecodes::_getfield                :
    case Bytecodes::_putfield                :
    case Bytecodes::_invokevirtual           :
    case Bytecodes::_invokespecial           :
    case Bytecodes::_invokestatic            :
    case Bytecodes::_invokeinterface         :
    case Bytecodes::_arraylength             :
    case Bytecodes::_ifnull                  :
    case Bytecodes::_athrow:
    // VM private bytecodes
    case Bytecodes::_fast_1_putstatic:
    case Bytecodes::_fast_2_putstatic:
    case Bytecodes::_fast_a_putstatic:
    case Bytecodes::_fast_1_getstatic:
    case Bytecodes::_fast_2_getstatic:
    case Bytecodes::_fast_init_1_putstatic:
    case Bytecodes::_fast_init_2_putstatic:
    case Bytecodes::_fast_init_a_putstatic:
    case Bytecodes::_fast_init_1_getstatic:
    case Bytecodes::_fast_init_2_getstatic:
    case Bytecodes::_fast_bputfield:
    case Bytecodes::_fast_sputfield:
    case Bytecodes::_fast_iputfield:
    case Bytecodes::_fast_lputfield:
    case Bytecodes::_fast_fputfield:
    case Bytecodes::_fast_dputfield:
    case Bytecodes::_fast_aputfield:
    case Bytecodes::_fast_bgetfield:
    case Bytecodes::_fast_sgetfield:
    case Bytecodes::_fast_igetfield:
    case Bytecodes::_fast_lgetfield:
    case Bytecodes::_fast_fgetfield:
    case Bytecodes::_fast_dgetfield:
    case Bytecodes::_fast_agetfield:
    case Bytecodes::_fast_cgetfield:     // Not same as fast_sgetfield ecause of sign
    case Bytecodes::_fast_invokevirtual:
    case Bytecodes::_fast_invokestatic:
    case Bytecodes::_fast_init_invokestatic:
    case Bytecodes::_fast_invokeinterface:
    case Bytecodes::_fast_invokevirtual_final:
    case Bytecodes::_fast_invokespecial:
    case Bytecodes::_fast_igetfield_1:         // fast_igetfield,1-byte operand
    case Bytecodes::_fast_agetfield_1:         // fast_agetfield,1-byte operand
    case Bytecodes::_aload_0_fast_agetfield_1: // aload_0 + fast_agetfield_1
    case Bytecodes::_aload_0_fast_igetfield_1: // aload_0 + fast_igetfield_1
    case Bytecodes::_pop_and_npe_if_null: // same as pop and throw NullPointerException if null 
    case Bytecodes::_aload_0_fast_agetfield_4: // aload_0 + fast_agetfield #4
    case Bytecodes::_aload_0_fast_igetfield_4: // aload_0 + fast_igetfield #4
    case Bytecodes::_aload_0_fast_agetfield_8: // aload_0 + fast_agetfield #8
    case Bytecodes::_aload_0_fast_igetfield_8: // aload_0 + fast_igetfield #8
     return true;
    default:
      return false;
   }
}
#endif 

bool BasicBlock::compute_branch_entries(const Method* method, 
                                        TypeArray *entry_counts)
{
  // This is a hot function during compilation. Since it only operates
  // on a few bytecodes, we hand-code the loop rather than 
  // using Method::iterate, which has much higher overhead. Also, this
  // saves footprint because we don't need a C++ vtable for BytecodeClosure.
  AllocationDisabler raw_pointers_used_in_this_function;

  int num_locks = 0;
  bool has_loops = false;
  int codesize = method->code_size();
  register jubyte *bcptr   = (jubyte*)method->code_base();
  register jubyte *bcend   = bcptr + codesize;
  register jubyte *entries = (jubyte*)entry_counts->base_address();
  register int bci = 0;
#if ENABLE_NPCE && ENABLE_CODE_OPTIMIZER
  int exception_count = 0;
#endif
  while (bcptr < bcend) {
    Bytecodes::Code code = (Bytecodes::Code)(*bcptr);
    GUARANTEE(code >= 0, "sanity: unsigned value");

    int length = Bytecodes::length_for(method, bci);
    if (Bytecodes::can_fall_through(code)) {
      ADD_BRANCH_ENTRY(entries, bci+length);
    }

    switch (code) {
    case Bytecodes::_ifeq:
    case Bytecodes::_ifne:
    case Bytecodes::_iflt:
    case Bytecodes::_ifge:
    case Bytecodes::_ifgt:
    case Bytecodes::_ifle:
    case Bytecodes::_if_icmpeq:
    case Bytecodes::_if_icmpne:
    case Bytecodes::_if_icmplt:
    case Bytecodes::_if_icmpge:
    case Bytecodes::_if_icmpgt:
    case Bytecodes::_if_icmple:
    case Bytecodes::_if_acmpeq:
    case Bytecodes::_if_acmpne:
    case Bytecodes::_ifnull:
    case Bytecodes::_ifnonnull:
    case Bytecodes::_goto:
      {
        int dest = bci + (jshort)(Bytes::get_Java_u2(bcptr+1));
        GUARANTEE(dest >= 0 && dest < codesize, "sanity");
        ADD_BRANCH_ENTRY(entries, dest);
        if (dest <= bci) {
          has_loops = true;
        }
      }
      break;
    case Bytecodes::_goto_w:
      {
        int dest = bci + (jint)(Bytes::get_Java_u4(bcptr+1));
        GUARANTEE(dest >= 0 && dest < codesize, "sanity");
        ADD_BRANCH_ENTRY(entries, dest);
        if (dest <= bci) {
          has_loops = true;
        }
      }
      break;
    case Bytecodes::_lookupswitch:
      {
        int table_index  = align_size_up(bci + 1, sizeof(jint));
        int default_dest = bci + method->get_java_switch_int(table_index + 0);
        int num_of_pairs = method->get_java_switch_int(table_index + 4);

        ADD_BRANCH_ENTRY(entries, default_dest);
        for (int i = 0; i < num_of_pairs; i++) {
          int dest =
            bci + method->get_java_switch_int(8 * i + table_index + 12);
          ADD_BRANCH_ENTRY(entries, dest);
          if (dest <= bci) {
            has_loops = true;
          }
        }
      }
      break;
    case Bytecodes::_tableswitch:
      {
        int table_index  = align_size_up(bci + 1, sizeof(jint));
        int default_dest = bci + method->get_java_switch_int(table_index + 0);
        int low          = method->get_java_switch_int(table_index + 4);
        int high         = method->get_java_switch_int(table_index + 8);

        ADD_BRANCH_ENTRY(entries, default_dest);
        for (int i = 0; i < (high - low + 1); i++) {
          int dest =
            bci + method->get_java_switch_int(4 * i + table_index + 12);
          ADD_BRANCH_ENTRY(entries, dest);
          if (dest <= bci) {
            has_loops = true;
          }
        }
      }
      break;
    case Bytecodes::_monitorenter:
      num_locks++;
      break;
    default:
#if ENABLE_NPCE && ENABLE_INTERNAL_CODE_OPTIMIZER
      if (is_null_point_exception_throwable(code)) {
        exception_count ++;
      } 
#endif 
      break;
    }
    bci   += length;
    bcptr += length;
  }
  GUARANTEE(bcptr == bcend, "sanity");
#if ENABLE_COMPILER
  Compiler::set_num_stack_lock_words(num_locks *
                      ((BytesPerWord + StackLock::size()) / sizeof(jobject)));
#endif
#if ENABLE_NPCE && ENABLE_INTERNAL_CODE_OPTIMIZER
  exception_count = exception_count <<1;
  Compiler::set_npe_bytecode_count(exception_count);
#endif
  return has_loops;
}

ReturnOop BasicBlock::compute_entry_counts(Method* method, bool &has_loops
                                           JVM_TRAPS)
{
  UsingFastOops fast_oops; 
  TypeArray::Fast entry_counts;
#if ENABLE_COMPILER
  const bool use_compiler_area = Compiler::is_active();
#else
  const bool use_compiler_area = false;
#endif

  has_loops = false;
  int code_size = method->code_size();
#if ENABLE_COMPILER
  if (use_compiler_area) {
    entry_counts = Universe::new_byte_array_in_compiler_area(code_size
                                                             JVM_CHECK_0);
  } else {
    entry_counts = Universe::new_byte_array(code_size JVM_CHECK_0);
  }
#else
    entry_counts = Universe::new_byte_array(code_size JVM_CHECK_0);
#endif

  // Give the first bytecode an entry, and iterate over the bytecodes.
  add_entry(&entry_counts, 0);
  has_loops |= compute_branch_entries(method, &entry_counts);

  // Add entries for the exception handlers.
  TypeArray::Fast exception_table = method->exception_table();
  GUARANTEE((exception_table().length() % 4) == 0, "Sanity check");
  int len = exception_table().length();
  for (int i = 0; i <len; i+=4) {
    int handler_bci = exception_table().ushort_at(i + 2);
    add_entry(&entry_counts, handler_bci);
    add_entry(&entry_counts, handler_bci);
  }

  // Return the entry counts.
  return entry_counts;
}

#else
// we get here in product build when ENABLE_COMPILER=false and
// ENABLE_MONET=true usually, those advanced optimizatins requiring
// computation of branch targets should be disabled in such a case
ReturnOop BasicBlock::compute_entry_counts(Method* method, bool &has_loops
                                           JVM_TRAPS)
{
  BREAKPOINT;
  return NULL;
}
#endif // USE_COMPILER_STRUCTURES
