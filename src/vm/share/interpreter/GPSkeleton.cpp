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

// There's no _GPSkeleton.incl because we can't put this file
// to includeDB (otherwise it would be linked into the real VM).
// So we have to a parasite and feed on Oop.cpp ...
#include "incls/_Oop.cpp.incl"

#ifndef KNI_FALSE
// We'd come to here if we're using the -sourceMergerLimit option of
// MakeDeps. In this case, incls/_Oop.cpp.incl would be empty. We have
// to define just a few things we need.
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

#ifndef NULL
#define NULL 0
#endif

#define INTERP_LOG_SIZE 8
#define method_execution_sensor_size 1

typedef int JVMFastGlobals;

#endif

/*
 * This file contains a skeleton for the "GP Table" for the ARM and
 * SH ports. This file is used in the build process of an AOT-enabled
 * ROM generator for these two platforms. Search for GP_TABLE_OBJ inside
 * build/share/jvm.make.
 */

#ifdef ARM

extern "C" {

  unsigned char * _kni_parameter_base;

#if ENABLE_INTERPRETATION_LOG
  OopDesc*        _interpretation_log[INTERP_LOG_SIZE];
  int             _interpretation_log_idx;
#endif

  OopDesc**       _old_generation_end;
  address         _current_stack_limit       = NULL;
  address         _compiler_stack_limit      = NULL;
  int             _rt_timer_ticks            = 0;

  address         _primordial_sp             = NULL;
  OopDesc*        _interned_string_near_addr = NULL;
  OopDesc**       _persistent_handles_addr   = NULL;

#if ENABLE_ISOLATES
  OopDesc*        _current_task;
  // table holding the pointer to the task mirror of all classes for the
  // current task
  address         _mirror_list_base = NULL;

  // where the address of the being initialized marker is kept for the
  // interpreter
  OopDesc*         _task_class_init_marker = NULL;
  OopDesc*         _task_array_class_init_marker = NULL;
#endif

#if ENABLE_JAVA_DEBUGGER
  int             _debugger_active = 0;
#endif

  address         gp_base_label;
  JVMFastGlobals  jvm_fast_globals;
  address         gp_compiler_new_object_ptr;
  address         gp_compiler_new_obj_array_ptr;
  address         gp_compiler_new_type_array_ptr;
  address         gp_shared_monitor_enter_ptr;
  address         gp_shared_monitor_exit_ptr;
  address         gp_compiler_idiv_irem_ptr;
  address         gp_shared_lock_synchronized_method_ptr;
  address         gp_shared_unlock_synchronized_method_ptr;
  address         gp_compiler_throw_NullPointerException_ptr;
  address         gp_compiler_throw_NullPointerException_0_ptr;
  address         gp_compiler_throw_NullPointerException_10_ptr;
  address         gp_compiler_throw_ArrayIndexOutOfBoundsException_ptr;
  address         gp_compiler_throw_ArrayIndexOutOfBoundsException_0_ptr;
  address         gp_compiler_throw_ArrayIndexOutOfBoundsException_10_ptr;
  address         gp_compiler_timer_tick_ptr;

  address         gp_shared_call_vm_ptr;
  address         gp_shared_call_vm_oop_ptr;
  address         gp_shared_call_vm_exception_ptr;

  unsigned char   _method_execution_sensor[method_execution_sensor_size];

  int*            _rom_constant_pool_fast;

  int             gp_constants[1];
  int             gp_constants_end[1];
  int             _gp_bytecode_counter;
  jint            _bytecode_counter;

  int		  _jvm_in_quick_native_method;
  char*		  _jvm_quick_native_exception;

#if ENABLE_THUMB_GP_TABLE
  jlong jvm_ladd(jlong op1, jlong op2) {return (jlong)0;}
  jlong jvm_lsub(jlong op1, jlong op2) {return (jlong)0;}
  jlong jvm_land(jlong op1, jlong op2) {return (jlong)0;}
  jlong jvm_lor (jlong op1, jlong op2) {return (jlong)0;}
  jlong jvm_lxor(jlong op1, jlong op2) {return (jlong)0;}
  jint  jvm_lcmp(jlong op1, jlong op2) {return        0;}
  jlong jvm_lmin(jlong op1, jlong op2) {return (jlong)0;}
  jlong jvm_lmax(jlong op1, jlong op2) {return (jlong)0;}
  jlong jvm_lmul(jlong op1, jlong op2) {return (jlong)0;}
  jlong jvm_lshl(jlong op1, jint op2)  {return (jlong)0;}
  jlong jvm_lshr(jlong op1, jint op2)  {return (jlong)0;}
  jlong jvm_lushr(jlong op1, jint op2) {return (jlong)0;}
#endif
}

#endif
