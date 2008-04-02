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

#if ENABLE_INTERPRETER_GENERATOR && !ENABLE_THUMB_COMPILER

#ifndef PRODUCT

class SourceMacros: public SourceAssembler {
 public:
  SourceMacros(Stream* output): SourceAssembler(output) {}

  // debugging support
  void dump_state    (const char* msg = NULL, Condition cond = al);

  void report_warning(const char* msg = NULL, Condition cond = al);
  void report_fatal  (const char* msg = NULL, Condition cond = al);

  void verify_gp_register(Register tmp);
  void verify_cpool_register(Register tmp);
  void debug_on_variable(const char* var_name, Condition cond, Register tmp);
  void zero_region(Register item, Register size,
                   Register tmp1, Register tmp2,
                   int header_size, bool size_includes_header,  bool size_is_in_bytes);

  void verify_tag(Register reg, Tag tag);

  // typed stack manipulation
  enum State { 
      // tos is cached in register.  jsp points to top-most item on stack
      tos_in_regs, 
      // no values in register.  jsp points to top-most item on stack
      tos_on_stack, 
      // Indeterminate state of the interpreter in between bytecodes.  
      // Stack may be "full" or empty".  Top element may be on stack or in
      // register
      tos_interpreter_basic 
  };

  // support for locals
  Address2 local_addr_at(int      index, int word_offset);
  Address2 local_addr_at(Register index                 );

  void     get_local_addr(Register result, int      index);
  void     get_local_addr(Register result, Register index);

  // support for bytecode access
  void ldrb_at_bcp (Register reg,               int offset);

  void ldrh_at_bcp_A (Register reg, Register tmp, int offset);
  void ldrh_at_bcp_B (Register reg, Register tmp, int offset);

  enum { addr_cpool_index,  // return the address of this cpool entry in "addr"
         addr_dont_care     // return whatever. . . .
  };
  void ldr_cp_entry_A(Register dst, Register addr,
                    int addr_result = addr_dont_care, int size = 2,
                    int offset = 1);

  void ldr_cp_entry_B(Register dst, Register addr,
                      int addr_result = addr_dont_care, int size = 2,
                      int offset = 1);

  void ldr_class_from_index_and_base(Register dst, Register index,
                                     Register base, Condition cond = al);

  // elaborate loads; values must be aligned
  enum ScaleFactor {
    no_scale = -1,
    times_1  =  0,
    times_2  =  1,
    times_4  =  2,
    times_8  =  3
  };

  void ldr_receiver(Register dst, Register count);

  void ldr_indexed (Register dst, Register base, Register index,
                    ScaleFactor scale_shift, int offset_12);
  void ldrb_indexed(Register dst, Register base, Register index,
                    ScaleFactor scale_shift, int offset_12);
  void ldrh_indexed(Register dst, Register base, Register index,
                    ScaleFactor scale_shift, int offset_8);

  // load byte swapped
  void ldr_bswap(Register dst, Register src, Register tmp, int offset);

  // various helpers
  void set_tag(Tag tag, Register reg=tos_tag);
  void set_tags(Tag tag, Register reg1=tos_tag, Register reg2=tmp1);
  void set_return_type(BasicType type, Condition cond = al);

  // untyped stack manipulation
  void push(Register reg, Condition cond = al,
            StackTypeMode type = full);
  void push(Address4 set, Condition cond = al, 
            StackTypeMode type = full);
  
  void pop (Register reg, Condition cond = al, 
            StackTypeMode type = full);
  void pop (Address4 set, Condition cond = al, 
            StackTypeMode type = full);

  // Entering and exiting a template
  void set_stack_state_to(State state, Condition cond = al);
  void restore_stack_state_from(State state, Condition cond = al);
  void dispatch(State);

  void pop_arguments(int count);
  void push_results(int count, Address4 result1=tos,   Address4 result2=tmp01, 
                               Address4 result3=tmp23, Address4 result4=tmp45,
                               Address4 result5=emptySet,
                               Address4 result6=emptySet);
  void dispatch(int count, Address4 result1=tos,   Address4 result2=tmp01, 
                           Address4 result3=tmp23, Address4 result4=tmp45, 
                           Address4 result5=emptySet,
                           Address4 result6=emptySet);

  void simple_c_bytecode(const char *name,
                         BasicType result,
                         BasicType arg1, BasicType arg2 = T_VOID,
                         bool commutative = false);

  void prefetch(int step);
  void prefetch(Register step);

  void simple_c_setup_arguments(BasicType arg1, BasicType arg2 = T_VOID,
                                bool commutative = false);
  void simple_c_store_result(BasicType result);
#if USE_FP_RESULT_IN_VFP_REGISTER
  void pop_fp_result(BasicType type);
#endif

  Address2 bytecode_impl(Register reg) { 
#if !ENABLE_THUMB_GP_TABLE
    return add_index(gp, reg, lsl, 2);
#else
    return sub_index(gp, reg, lsl, 2);
#endif
  }

  // support for getting the current thread
  void get_thread(Register thread);
  void get_thread_handle(Register thread);

  // support for call from interpreter
  void call_from_interpreter(Register routine, int offset = 0);
  void call_from_interpreter(const char* routine, bool target_fixes_lr = false);

  // support for accessing frame data
  void get_bottom(Register reg, Condition cond=al);
  void get_method(Register reg, Condition cond=al);

  // support for accessing constants pool
  void get_cpool(Register reg, bool method_in_r0=false);

  void save_interpreter_state();
  void restore_interpreter_state(bool include_bcp = true);

  // support for calling the VM runtime system
  void interpreter_call_vm(const char *name, BasicType return_value_type,
                           bool save_interpreter_state = true);

  // Support for profiling
  void wtk_profile_quick_call(int param_size = -1);

  // stack handling
  void check_timer_tick();

  void fast_c_call(Label name, Register preserved = no_reg);

  // locking, unlocking support
  void unlock_activation ();

  // Record recently invoked interpreted method
#if ENABLE_INTERPRETATION_LOG
  void update_interpretation_log();
#else
  void update_interpretation_log() {}
#endif

#if ENABLE_ISOLATES
  void get_mirror_from_clinit_list(Register tm, Register klass, Register temp);
#else
  void initialize_class_when_needed(Register dst, Register tmp1, Register tmp2,
                                    Label& restart, int args_from_stack = 0);
#endif

 protected:
  // the maximum array length that is safe to allocate with having
  // to worry about wrap-arounds.
  enum {
    maximum_safe_array_length = (1 << 20)
  };

  void interpreter_call_shared_call_vm(BasicType return_value_type);
  void goto_shared_call_vm(BasicType return_value_type);

  void return_from_invoker(int prefetch_size, int result_type);
  void generate_call(Register entry, Label& label, int result_type, 
                     int prefetch_size,  char* deoptimization_entry_name);
  void invoke_method(Register method, Register entry, Register tmp,
                     int prefetch_size, char *deoptimization_entry_name);
  void swap_mask(Register msk);
  void swap_bytes(Register res, Register tmp, Register msk);
  void get_method_parameter_size(Register result, Register method);
};

#endif // PRODUCT

#endif /*#if ENABLE_INTERPRETER_GENERATOR*/
