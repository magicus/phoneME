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

/*
 * This file contains stubs for all the symbols that are generated
 * by the interpreter loop generator.
 */

#include "incls/_precompiled.incl"
#include "incls/_CompleteInclude2.incl"

#ifndef KNI_FALSE
// We'd come to here if we're using the -sourceMergerLimit option of
// MakeDeps. In this case, incls/_Oop.cpp.incl would be empty. We have
// to define just a few things we need.
typedef void Oop;
typedef void OopDesc;
typedef void TypeArrayClass;
typedef void Thread;
typedef unsigned char jubyte;
typedef int jint;
typedef void * address;

#ifdef _MSC_VER
typedef __int64 jlong;
#else
typedef long long jlong;
#endif

#endif

#ifndef NULL
#define NULL 0
#endif

extern "C" {

void shared_invoke_debug()                         {}
void shared_invoke_compiler()                      {}
void shared_call_vm()                              {}
void shared_call_vm_oop()                          {}
void shared_call_vm_exception()                    {}
void call_on_primordial_stack(void (*)(void))      {}
void primordial_to_current_thread()                {}
void current_thread_to_primordial()                {}
void shared_fast_getbyte_accessor()                {}
void shared_fast_getshort_accessor()               {}
void shared_fast_getchar_accessor()                {}
void shared_fast_getint_accessor()                 {}
void shared_fast_getlong_accessor()                {}
void shared_fast_getbyte_static_accessor()         {}
void shared_fast_getshort_static_accessor()        {}
void shared_fast_getchar_static_accessor()         {}
void shared_fast_getint_static_accessor()          {}
void shared_fast_getlong_static_accessor()         {}
void shared_fast_getfloat_accessor()               {}
void shared_fast_getdouble_accessor()              {}
void shared_fast_getfloat_static_accessor()        {}
void shared_fast_getdouble_static_accessor()       {}
void fixed_interpreter_method_entry()              {}
void fixed_interpreter_fast_method_entry_0()       {}
void fixed_interpreter_fast_method_entry_1()       {}
void fixed_interpreter_fast_method_entry_2()       {}
void fixed_interpreter_fast_method_entry_3()       {}
void fixed_interpreter_fast_method_entry_4()       {}
void invoke_pending_entries(Thread* THREAD)        {}
void start_lightweight_thread_asm()                {}
void interpreter_timer_tick()                      {}
void interpreter_grow_stack()                      {}
void interpreter_method_entry()                    {}
void interpreter_fast_method_entry_0()             {}
void interpreter_fast_method_entry_1()             {}
void interpreter_fast_method_entry_2()             {}
void interpreter_fast_method_entry_3()             {}
void interpreter_fast_method_entry_4()             {}
void quick_void_native_method_entry()              {}
void quick_int_native_method_entry()               {}
void quick_obj_native_method_entry()               {}
void interpreter_dispatch_table()                  {}
void interpreter_deoptimization_entry()            {}
void invoke3_deoptimization_entry_0()              {}
void invoke3_deoptimization_entry_1()              {}
void invoke3_deoptimization_entry_2()              {}
void invoke3_deoptimization_entry_3()              {}
void invoke3_deoptimization_entry_4()              {}
void invoke5_deoptimization_entry_0()              {}
void invoke5_deoptimization_entry_1()              {}
void invoke5_deoptimization_entry_2()              {}
void invoke5_deoptimization_entry_3()              {}
void invoke5_deoptimization_entry_4()              {}
void indirect_execution_sensor_update()            {}
void compiler_rethrow_exception()                  {}
void compiler_new_object()                         {}
void compiler_new_obj_array()                      {}
void compiler_new_type_array()                     {}
void compiler_idiv_irem()                          {}
void compiler_remove_patch()                       {}
void compiler_timer_tick()                         {}
void shared_monitor_enter()                        {}
void shared_monitor_exit()                         {}
#if USE_COMPILER_GLUE_CODE
void compiler_glue_code_start()                    {}
void compiler_glue_code_end()                      {}
#endif
#if ENABLE_COMPRESSED_VSF
void compiler_callvm_stubs_start()                 {}
void compiler_callvm_stubs_end()                   {}
#endif

void compiler_invoke_static()                      {}
void compiler_unlinked_static_method()             {}
void compiler_invoke_virtual_uninitialized()       {}
void compiler_unlinked_virtual_method()            {}
void compiler_invoke_virtual_megamorphic()         {}
void compiler_inline_cache_miss()                  {}
void compiler_throw_NullPointerException()         {}
void compiler_throw_ArrayIndexOutOfBoundsException() {}

void shared_lock_synchronized_method()             {}
void shared_unlock_synchronized_method()           {}

void native_math_sin_entry()                       {}
void native_math_cos_entry()                       {}
void native_math_tan_entry()                       {}
void native_math_sqrt_entry()                      {}
void native_math_ceil_entry()                      {}
void native_math_floor_entry()                     {}

#if ENABLE_CLDC_111

void native_math_asin_entry()                      {}
void native_math_acos_entry()                      {}
void native_math_atan_entry()                      {}
void native_math_atan2_entry()                     {}

#endif // ENABLE_CLDC_111

void native_string_charAt_entry()                  {}
void native_string_init_entry()                    {}
void native_string_equals_entry()                  {}
void native_string_indexof_string_entry()          {}
void native_string_indexof0_string_entry()         {}
void native_string_indexof_entry()                 {}
void native_string_indexof0_entry()                {}
void native_string_compareTo_entry()               {}
void native_string_startsWith_entry()              {}
void native_string_startsWith0_entry()             {}
void native_string_endsWith_entry()                {}
void native_string_substringI_entry()              {}
void native_string_substringII_entry()             {}
void native_system_arraycopy_entry()               {}
void native_jvm_unchecked_byte_arraycopy_entry()   {}
void native_jvm_unchecked_char_arraycopy_entry()   {}
void native_jvm_unchecked_int_arraycopy_entry()    {}
void native_jvm_unchecked_long_arraycopy_entry()   {}
void native_jvm_unchecked_obj_arraycopy_entry()    {}

void native_vector_elementAt_entry()               {}
void native_vector_addElement_entry()              {}
void native_stringbuffer_append_entry()            {}
void native_integer_toString_entry()               {}
#if ENABLE_JAVA_DEBUGGER
void shared_call_vm_oop_return()                   {}
#endif

#if !defined(PRODUCT) || USE_DEBUG_PRINTING
jlong interpreter_bytecode_counters[1];
jlong interpreter_pair_counters[1];
#endif

#if ENABLE_METHOD_TRAPS
void cautious_invoke() {}
#endif

#if ENABLE_REFLECTION || ENABLE_JAVA_DEBUGGER
void entry_return_void()   {}
void entry_return_word()   {}
void entry_return_long()   {}
void entry_return_float()  {}
void entry_return_double() {}
void entry_return_object() {}
#endif

jint assembler_loop_type;
jint _bytecode_counter = 0;

unsigned char _protected_page[1];

#if ENABLE_CPU_VARIANT
void initialize_cpu_variant() {}
void enable_cpu_variant() {}
void disable_cpu_variant() {}
#endif

#if ARM || HITACHI_SH
jint compiler_patched_code[1];
jint compiler_unpatched_code[1];

address   _current_stack_limit;
address   _compiler_stack_limit;
int       _rt_timer_ticks;
address   _primordial_sp;

OopDesc** _persistent_handles_addr;
OopDesc*  _interpretation_log[1];
int       _interpretation_log_idx;

unsigned char _method_execution_sensor[1];

OopDesc** _old_generation_end;
OopDesc*  _interned_string_near_addr;
OopDesc*  _task_class_init_marker;
OopDesc*  _current_task;

#if ENABLE_JAVA_DEBUGGER
int       _debugger_active;
#endif
int       _current_task_id_as_offset;
int*      _rom_constant_pool_fast;

unsigned char * _kni_parameter_base;

int gp_constants[1];
int gp_constants_end[1];

int _gp_bytecode_counter;
int _gp_misc[1];

int  _jvm_in_quick_native_method;
char*  _jvm_quick_native_exception;

#endif

#if ARM

address gp_base_label;
address gp_compiler_new_object_ptr;
address gp_compiler_new_obj_array_ptr;
address gp_compiler_new_type_array_ptr;
address gp_shared_monitor_enter_ptr;
address gp_shared_monitor_exit_ptr;
address gp_compiler_idiv_irem_ptr;
address gp_compiler_checkcast_ptr;
address gp_compiler_instanceof_ptr;
address gp_shared_lock_synchronized_method_ptr;
address gp_shared_unlock_synchronized_method_ptr;
address gp_compiler_throw_NullPointerException_ptr;
address gp_compiler_throw_NullPointerException_0_ptr;
address gp_compiler_throw_NullPointerException_10_ptr;
address gp_compiler_throw_ArrayIndexOutOfBoundsException_ptr;
address gp_compiler_throw_ArrayIndexOutOfBoundsException_0_ptr;
address gp_compiler_throw_ArrayIndexOutOfBoundsException_10_ptr;
address gp_compiler_timer_tick_ptr;
address gp_interpreter_method_entry_ptr;
address gp_shared_call_vm_ptr;
address gp_shared_call_vm_oop_ptr;
address gp_shared_call_vm_exception_ptr;

#if ENABLE_FLOAT
void   jvm_set_vfp_fast_mode()      {}
int    jvm_fcmpl(float, float)      { return 0; }
int    jvm_fcmpg(float, float)      { return 0; }
int    jvm_dcmpl(double, double)    { return 0; }
int    jvm_dcmpg(double, double)    { return 0; }
float  jvm_i2f(int)                 { return 0; }
double jvm_i2d(int)                 { return 0; }
int    jvm_f2i(float)               { return 0; }
double jvm_f2d(float)               { return 0; }
jlong  jvm_f2l(float)               { return 0; }
float  jvm_l2f(jlong)               { return 0; }
double jvm_l2d(jlong)               { return 0; }
jlong  jvm_d2l(double)              { return 0; }
int    jvm_d2i(double)              { return 0; }
float  jvm_d2f(double)              { return 0; }

float jvm_fadd(float x, float y)    { return 0; }
float jvm_fsub(float x, float y)    { return 0; }
float jvm_fmul(float x, float y)    { return 0; }
float jvm_fdiv(float x, float y)    { return 0; }
float jvm_frem(float x, float y)    { return 0; }

double jvm_dadd(double x, double y) { return 0; }
double jvm_dsub(double x, double y) { return 0; }
double jvm_dmul(double x, double y) { return 0; }
double jvm_ddiv(double x, double y) { return 0; }
double jvm_drem(double x, double y) { return 0; }

#endif

void arm_flush_icache(address start, int size) { }

// set timer_tick from WMMX wCASF register
void wmmx_set_timer_tick() { }
// clear timer_tick from WMMX wCASF register
void wmmx_clear_timer_tick() { }

#endif /* ARM */


#if HITACHI_SH
address gp_base_label;

void shared_idiv() {}
void shared_irem() {}

void sh_flush_icache(address start, int size) { }

#endif

// Must be in-sync with the declaration inside GlobalDefinitions.hpp
JVMFastGlobals jvm_fast_globals;

#if ENABLE_FAST_MEM_ROUTINES || defined(USE_LIBC_GLUE)
void* jvm_memcpy(void *dest, const void *src, int n) {
  return (void*)memcpy(dest, src, n);
}
int jvm_memcmp(const void *s1, const void *s2, int n) {
  return (int)memcmp(s1, s2, n);
}
#endif

#if ENABLE_ARM_VFP
void vfp_redo() {}
void vfp_fcmp_redo() {}
void vfp_double_redo() {}
void vfp_dcmp_redo() {}
#endif

} // extern "C"
