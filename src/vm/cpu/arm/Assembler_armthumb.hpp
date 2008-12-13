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
 * This is the part of the Assembler class that's shared between the ARM
 * and THUMB compilers
 */

// GP_ASM: the entry in the GP table is defined in the assembler loop
// GP_C:   the entry in the GP table is defined in C code.
#define GP_ASM 1
#define GP_C   2

#if ENABLE_ISOLATES
  #define FOR_ALL_ISOLATE_GP_GLOBALS(func) \
      func(current_task,            sizeof(OopDesc*)) \
      func(task_class_init_marker,  sizeof(OopDesc*))
#else
  #define FOR_ALL_ISOLATE_GP_GLOBALS(func)
#endif

#if ENABLE_JAVA_DEBUGGER
  #define FOR_ALL_DEBUGGER_GP_GLOBALS(func) \
      func(debugger_active, sizeof(int))
#else
  #define FOR_ALL_DEBUGGER_GP_GLOBALS(func)
#endif

  #define DECLARE_FAST_GLOBAL_FOR_ARM(func, type, name) \
     func(name, sizeof(OopDesc*))

  #define GP_GLOBAL_SYMBOLS_DO_PART1(pointer, value) \
     value(current_stack_limit,           sizeof(OopDesc*))                   \
     value(compiler_stack_limit,          sizeof(OopDesc*))                   \
     value(rt_timer_ticks,                sizeof(int))

  #define GP_GLOBAL_SYMBOLS_DO_PART2(pointer, value) \
     FORALL_JVM_FAST_GLOBALS(DECLARE_FAST_GLOBAL_FOR_ARM, value) \
     \
     value(interpretation_log,            sizeof(OopDesc*) * INTERP_LOG_SIZE) \
     value(interpretation_log_idx,        sizeof(int))                        \
     value(primordial_sp,                 sizeof(OopDesc*))                   \
     value(old_generation_end,            sizeof(OopDesc*))                   \
     value(bytecode_counter,              sizeof(OopDesc*))                   \
     value(kni_parameter_base,            sizeof(OopDesc*))                   \
     value(rom_constant_pool_fast,        sizeof(int))                        \
     value(gp_bytecode_counter,           sizeof(int) )                       \
     value(jvm_in_quick_native_method,    sizeof(int) )                       \
     value(jvm_quick_native_exception,    sizeof(int) )                       \
     value(method_execution_sensor,       method_execution_sensor_size)       \
     value(interned_string_near_addr,     sizeof(OopDesc*))                   \
     value(persistent_handles_addr,       sizeof(OopDesc*))                   \
     FOR_ALL_ISOLATE_GP_GLOBALS(value)                                        \
     FOR_ALL_DEBUGGER_GP_GLOBALS(value)

#define GP_GLOBAL_SYMBOLS_DO(pointer, value) \
        GP_GLOBAL_SYMBOLS_DO_PART1(pointer, value) \
        GP_GLOBAL_SYMBOLS_DO_PART2(pointer, value)

//----------------------------------------------------------------------
// Functions that need to be called by JIT code, using something like
//     ldr pc, [r5, #xxxx]
//----------------------------------------------------------------------

#if !ENABLE_COMPILER
#define GP_COMPILER_SYMBOLS_DO_GENERIC(pointer, value)
#else
#define GP_COMPILER_SYMBOLS_DO_GENERIC(pointer, value) \
  pointer(GP_ASM, compiler_new_object) \
  pointer(GP_ASM, compiler_new_type_array) \
  pointer(GP_ASM, compiler_timer_tick) \
  pointer(GP_ASM, shared_monitor_enter) \
  pointer(GP_ASM, shared_monitor_exit) \
  pointer(GP_ASM, shared_lock_synchronized_method) \
  pointer(GP_ASM, shared_unlock_synchronized_method) \
  \
  pointer(GP_ASM, shared_call_vm_oop) \
  pointer(GP_ASM, shared_call_vm_exception) \
  pointer(GP_ASM, shared_call_vm) \
  pointer(GP_ASM, interpreter_method_entry) \
  pointer(GP_ASM, compiler_new_obj_array) \
  pointer(GP_ASM, compiler_idiv_irem) \
  pointer(GP_ASM, compiler_checkcast) \
  pointer(GP_ASM, compiler_instanceof) \
  \
  pointer(GP_ASM, compiler_throw_NullPointerException) \
  pointer(GP_ASM, compiler_throw_NullPointerException_0) \
  pointer(GP_ASM, compiler_throw_NullPointerException_1) \
  pointer(GP_ASM, compiler_throw_NullPointerException_2) \
  pointer(GP_ASM, compiler_throw_NullPointerException_3) \
  pointer(GP_ASM, compiler_throw_NullPointerException_4) \
  pointer(GP_ASM, compiler_throw_NullPointerException_5) \
  pointer(GP_ASM, compiler_throw_NullPointerException_6) \
  pointer(GP_ASM, compiler_throw_NullPointerException_7) \
  pointer(GP_ASM, compiler_throw_NullPointerException_8) \
  pointer(GP_ASM, compiler_throw_NullPointerException_9) \
  pointer(GP_ASM, compiler_throw_NullPointerException_10) \
  \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException) \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException_0) \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException_1) \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException_2) \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException_3) \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException_4) \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException_5) \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException_6) \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException_7) \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException_8) \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException_9) \
  pointer(GP_ASM, compiler_throw_ArrayIndexOutOfBoundsException_10)
#endif

#define GP_COMPILER_SYMBOLS_DO(pointer, value) \
        GP_COMPILER_SYMBOLS_DO_GENERIC(pointer, value)

//----------------------------------------------------------------------
// Constants that are implemented in C code but need to be statically
// referenced by the interpreter, like
//     ldr r0, =stack_overflow
//----------------------------------------------------------------------

#define GP_INTERPRETER_SYMBOLS_DO_GENERIC(pointer, value) \
      pointer(GP_C, stack_overflow) \
      pointer(GP_C, timer_tick) \
      pointer(GP_C, lock_stack_lock) \
      pointer(GP_C, signal_waiters) \
      pointer(GP_C, array_index_out_of_bounds_exception) \
      pointer(GP_C, get_array_index_out_of_bounds_exception) \
      pointer(GP_C, null_pointer_exception) \
      pointer(GP_C, get_null_pointer_exception) \
      pointer(GP_C, illegal_monitor_state_exception) \
      pointer(GP_C, get_illegal_monitor_state_exception) \
      pointer(GP_C, division_by_zero_exception) \
      pointer(GP_C, get_division_by_zero_exception) \
      pointer(GP_C, get_incompatible_class_change_error) \
      pointer(GP_C, multianewarray) \
      pointer(GP_C, instanceof) \
      pointer(GP_C, _instanceof) \
      pointer(GP_C, checkcast) \
      pointer(GP_C, array_store_type_check)

#if ENABLE_WTK_PROFILER
#define GP_INTERPRETER_SYMBOLS_DO_PROFILER(pointer, value) \
      pointer(GP_C, jprof_record_method_transition)
#else
#define GP_INTERPRETER_SYMBOLS_DO_PROFILER(pointer, value)
#endif

#if ENABLE_ISOLATES
#define GP_INTERPRETER_SYMBOLS_DO_ISOLATE(pointer, value) \
      pointer(GP_C, compiled_code_task_barrier)
#else
#define GP_INTERPRETER_SYMBOLS_DO_ISOLATE(pointer, value)
#endif

#if (ENABLE_THUMB_COMPILER || ENABLE_THUMB_REGISTER_MAPPING)
#define GP_INTERPRETER_SYMBOLS_DO_THUMB(pointer, value) \
  pointer(GP_ASM, indirect_execution_sensor_update)
#else
#define GP_INTERPRETER_SYMBOLS_DO_THUMB(pointer, value)
#endif

#define GP_INTERPRETER_SYMBOLS_DO(pointer, value) \
        GP_INTERPRETER_SYMBOLS_DO_THUMB(pointer, value) \
        GP_INTERPRETER_SYMBOLS_DO_GENERIC(pointer, value) \
        GP_INTERPRETER_SYMBOLS_DO_PROFILER(pointer, value) \
        GP_INTERPRETER_SYMBOLS_DO_ISOLATE(pointer, value)

//----------------------------------------------------------------------
// Float constants that are somehow referenced by the compiler.
// IMPL_NOTE: how exactly?
//----------------------------------------------------------------------

#define GP_COMPILER_FLOAT_SYMBOLS_DO_GENERIC(pointer, value) \
      pointer(GP_C, jvm_ldiv) \
      pointer(GP_C, jvm_lrem)

#if ENABLE_THUMB_GP_TABLE
#define GP_COMPILER_FLOAT_SYMBOLS_DO_THUMB_COMPILER(pointer, value) \
      pointer(GP_C, jvm_ladd) \
      pointer(GP_C, jvm_lsub) \
      pointer(GP_C, jvm_land) \
      pointer(GP_C, jvm_lor)  \
      pointer(GP_C, jvm_lxor) \
      pointer(GP_C, jvm_lcmp) \
      pointer(GP_C, jvm_lmin) \
      pointer(GP_C, jvm_lmax) \
      pointer(GP_C, jvm_lmul) \
      pointer(GP_C, jvm_lshl) \
      pointer(GP_C, jvm_lshr) \
      pointer(GP_C, jvm_lushr)
#else
#define GP_COMPILER_FLOAT_SYMBOLS_DO_THUMB_COMPILER(pointer, value)
#endif

#define GP_COMPILER_FLOAT_SYMBOLS_DO(pointer, value) \
        GP_COMPILER_FLOAT_SYMBOLS_DO_GENERIC(pointer, value) \
        GP_COMPILER_FLOAT_SYMBOLS_DO_THUMB_COMPILER(pointer, value)

//----------------------------------------------------------------------
// Constants that are somehow referenced by the compiler.
// IMPL_NOTE: how exactly?
//----------------------------------------------------------------------

#if ENABLE_FAST_MEM_ROUTINES
#define GP_COMPILER_SYMBOLS2_MEM_ROUTINES(pointer, value) \
      pointer(GP_ASM, jvm_memcpy)
#else
#define GP_COMPILER_SYMBOLS2_MEM_ROUTINES(pointer, value) \
      pointer(GP_C, jvm_memcpy)
#endif

#if !ENABLE_COMPILER
#define GP_COMPILER_SYMBOLS2_DO_GENERIC(pointer, value)
#else
#define GP_COMPILER_SYMBOLS2_DO_GENERIC(pointer, value) \
      GP_COMPILER_SYMBOLS2_MEM_ROUTINES(pointer, value) \
      pointer(GP_C, deoptimize) \
      pointer(GP_C, uncommon_trap) \
      pointer(GP_C, get_method)    \
      pointer(GP_C, trace_bytecode) 
#endif

#if !ENABLE_ISOLATES && ENABLE_COMPILER
#define GP_COMPILER_SYMBOLS2_DO_SVM(pointer, value) \
      pointer(GP_C, initialize_class)
#else
#define GP_COMPILER_SYMBOLS2_DO_SVM(pointer, value)
#endif

#define GP_COMPILER_SYMBOLS2_DO(pointer, value) \
        GP_COMPILER_SYMBOLS2_DO_GENERIC(pointer, value) \
        GP_COMPILER_SYMBOLS2_DO_SVM(pointer, value)

//----------------------------------------------------------------------
// Internal constants. Note that it's split in 3 parts in that order to
// generate the same code as GPTableGenerator_arm.cpp version 1.12. Need
// to clean up later.
// IMPL_NOTE: what exactly is this for?
//----------------------------------------------------------------------

#define GP_INTERNAL_SYMBOLS1_DO(pointer, value) \
    pointer(GP_ASM, native_system_arraycopy_entry) \
    pointer(GP_ASM, native_jvm_unchecked_byte_arraycopy_entry) \
    pointer(GP_ASM, native_jvm_unchecked_char_arraycopy_entry) \
    pointer(GP_ASM, native_jvm_unchecked_int_arraycopy_entry) \
    pointer(GP_ASM, native_jvm_unchecked_obj_arraycopy_entry) \
    pointer(GP_ASM, native_string_indexof_entry) \
    pointer(GP_ASM, native_string_indexof0_entry)

#if ENABLE_SOFT_FLOAT && ENABLE_FLOAT
#define GP_INTERNAL_SYMBOLS2_DO(pointer, value) \
    pointer(GP_ASM, jvm_fadd) \
    pointer(GP_ASM, jvm_fsub) \
    pointer(GP_ASM, jvm_fmul) \
    pointer(GP_ASM, jvm_fdiv) \
    pointer(GP_ASM, jvm_frem) \
    pointer(GP_ASM, jvm_dadd) \
    pointer(GP_ASM, jvm_dsub) \
    pointer(GP_ASM, jvm_dmul) \
    pointer(GP_ASM, jvm_ddiv) \
    pointer(GP_ASM, jvm_drem) \
    pointer(GP_ASM, jvm_fcmpl) \
    pointer(GP_ASM, jvm_fcmpg) \
    pointer(GP_ASM, jvm_dcmpl) \
    pointer(GP_ASM, jvm_dcmpg) \
    pointer(GP_ASM, jvm_i2d) \
    pointer(GP_ASM, jvm_i2f) \
    pointer(GP_ASM, jvm_l2d) \
    pointer(GP_ASM, jvm_l2f) \
    pointer(GP_ASM, jvm_f2i) \
    pointer(GP_ASM, jvm_f2l) \
    pointer(GP_ASM, jvm_f2d) \
    pointer(GP_ASM, jvm_d2i) \
    pointer(GP_ASM, jvm_d2l) \
    pointer(GP_ASM, jvm_d2f)
#else
#define GP_INTERNAL_SYMBOLS2_DO(pointer, value)
#endif

#if ENABLE_COMPILER
#define GP_INTERNAL_SYMBOLS3_DO(pointer, value) \
    pointer(GP_ASM, fixed_interpreter_method_entry) \
    pointer(GP_ASM, fixed_interpreter_fast_method_entry_0) \
    pointer(GP_ASM, fixed_interpreter_fast_method_entry_1) \
    pointer(GP_ASM, fixed_interpreter_fast_method_entry_2) \
    pointer(GP_ASM, fixed_interpreter_fast_method_entry_3) \
    pointer(GP_ASM, fixed_interpreter_fast_method_entry_4) \
    pointer(GP_ASM, _newobject) \
    pointer(GP_ASM, interpreter_throw_NullPointerException) \
    pointer(GP_ASM, interpreter_throw_NullPointerException_tos_cached) \
    pointer(GP_ASM, interpreter_throw_IncompatibleClassChangeError) \
    pointer(GP_ASM, interpreter_grow_stack) \
    pointer(GP_ASM, interpreter_throw_ArrayIndexOutOfBoundsException)
#else
#define GP_INTERNAL_SYMBOLS3_DO(pointer, value) \
    pointer(GP_ASM, fixed_interpreter_fast_method_entry_0) \
    pointer(GP_ASM, fixed_interpreter_fast_method_entry_1) \
    pointer(GP_ASM, fixed_interpreter_fast_method_entry_2) \
    pointer(GP_ASM, fixed_interpreter_fast_method_entry_3) \
    pointer(GP_ASM, fixed_interpreter_fast_method_entry_4) \
    pointer(GP_ASM, _newobject) \
    pointer(GP_ASM, interpreter_throw_NullPointerException) \
    pointer(GP_ASM, interpreter_throw_NullPointerException_tos_cached) \
    pointer(GP_ASM, interpreter_throw_IncompatibleClassChangeError) \
    pointer(GP_ASM, interpreter_grow_stack) \
    pointer(GP_ASM, interpreter_throw_ArrayIndexOutOfBoundsException)

#endif

#define GP_INTERNAL_SYMBOLS_DO(pointer, value) \
        GP_INTERNAL_SYMBOLS1_DO(pointer, value) \
        GP_INTERNAL_SYMBOLS2_DO(pointer, value) \
        GP_INTERNAL_SYMBOLS3_DO(pointer, value)

// Use GP_SYMBOLS_DO() to iterate over all symbols that may be
// accessed by the compiler using ldr_using_gp()

#if (ENABLE_THUMB_COMPILER || ENABLE_THUMB_REGISTER_MAPPING)
#define GP_SYMBOLS_DO(pointer, value) \
        GP_INTERPRETER_SYMBOLS_DO(pointer, value) \
        GP_GLOBAL_SYMBOLS_DO_PART1(pointer, value) \
        GP_COMPILER_SYMBOLS_DO(pointer, value) \
        GP_GLOBAL_SYMBOLS_DO_PART2(pointer, value) \
        GP_COMPILER_FLOAT_SYMBOLS_DO(pointer, value) \
        GP_COMPILER_SYMBOLS2_DO(pointer, value) \
        GP_INTERNAL_SYMBOLS_DO(pointer, value)
#else
#define GP_SYMBOLS_DO(pointer, value) \
        GP_GLOBAL_SYMBOLS_DO(pointer, value) \
        GP_COMPILER_SYMBOLS_DO(pointer, value) \
        GP_INTERPRETER_SYMBOLS_DO(pointer, value) \
        GP_COMPILER_FLOAT_SYMBOLS_DO(pointer, value) \
        GP_COMPILER_SYMBOLS2_DO(pointer, value) \
        GP_INTERNAL_SYMBOLS_DO(pointer, value)
#endif

struct GPTemplate {
  const char *name;
  bool  is_pointer;
  bool  is_asm;
  int   size;
};

#define DEFINE_GP_POINTER(type, name) {STR(name), 1, (type == GP_ASM), 4},
#define DEFINE_GP_VALUE(name, size)   {STR(name), 0, 0, size},

#define DECLARE_GP_PTR_FOR_POINTER(type, name) \
        extern address gp_ ## name ## _ptr;
#define DECLARE_GP_PTR_FOR_VALUE(type, name)

extern "C" {
  extern address gp_base_label;
  GP_SYMBOLS_DO(DECLARE_GP_PTR_FOR_POINTER, DECLARE_GP_PTR_FOR_VALUE)
}
