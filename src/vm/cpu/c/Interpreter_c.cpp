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
#include "incls/_Interpreter_c.cpp.incl"
#include <stdarg.h>
#include <setjmp.h>

extern "C" {

  // external routines used when quickening
  extern Bytecodes::Code quicken(Thread* THREAD);
  extern Bytecodes::Code quicken_invokestatic(Thread* THREAD);
  extern Bytecodes::Code putfield(Thread* THREAD);
  extern Bytecodes::Code getfield(Thread* THREAD);
  extern Bytecodes::Code putstatic(Thread* THREAD);
  extern Bytecodes::Code getstatic(Thread* THREAD);
  extern Bytecodes::Code handle_breakpoint(Thread *thread);
  extern void handle_single_step(Thread *thread);

  // external interpreter runtime routines, see InterpreterRuntime_<arch>.cpp
  // and  InterpreterRuntime.cpp
  extern OopDesc*        anewarray(Thread *THREAD);
  extern OopDesc*        _newarray(Thread *THREAD, BasicType type, jint len);
  extern void            signal_waiters(Thread *THREAD,
                                        StackLock* stack_lock);
  extern void            lock_stack_lock(Thread* THREAD,
                                         StackLock* stack_lock JVM_TRAPS);
  extern ReturnOop       find_exception_frame(Thread* THREAD,
                                              OopDesc* raw_exception);
  extern void            unlock_special_stack_lock(Thread* THREAD,
                                                   OopDesc *lock_obj);

  // init FPU
  extern void InitFPU();

  // prototypes of C linkage function goes here
  static bool shared_call_vm_internal(address entry_point,
                                      address return_point,
                                      BasicType return_value_type,
                                      jint num_args,
                                      ...);
  static void shared_entry(address return_point);

  // global defines
#define WIDE_OFFSET       255

  // types
  typedef void       (*func_t)();
  typedef void       (*func1_t)(jint arg);
  typedef jlong      (*func2_t)(jint arg1, jint arg2);
  typedef void       (*void_func_t)(Thread* THREAD, ...);
  typedef jint       (*int_func_t)(Thread* THREAD, ...);
  typedef jlong      (*long_func_t)(Thread* THREAD, ...);
  typedef jfloat     (*float_func_t)(Thread* THREAD, ...);
  typedef jdouble    (*double_func_t)(Thread* THREAD, ...);
  typedef jboolean   (*boolean_func_t)(Thread* THREAD, ...);
  typedef jchar      (*char_func_t)(Thread* THREAD, ...);
  typedef jshort     (*short_func_t)(Thread* THREAD, ...);
  typedef jobject    (*object_func_t)(Thread* THREAD, ...);
  typedef OopDesc*   (*oop_func_t)(Thread* THREAD, ...);

  typedef union {
    jbyte   byte_val;
    jshort  short_val;
    jint    int_val;
    jlong   long_val;
    jfloat  float_val;
    jdouble double_val;
    address object_val;
  } basic_value;

  /* global VM pointers */
  static address  g_jfp;     /* Java frame pointer   */
  static address  g_jsp;     /* Java stack pointer   */
  static address  g_jpc;     /* Java program counter */
  static address  g_jlocals; /* current Java method locals pointer */

  /* used to return from interpreter via longjmp */
  static jmp_buf interpreter_env;
  static bool inside_interpreter;

  /* bytecodes dispatch table */
  static func_t interpreter_dispatch_table[256+WIDE_OFFSET];

  // has_Interpreter, has_FloatingPoint, has_TraceBytecodes
  jint assembler_loop_type = 0x1 + 0x40 + 0x4;

#if !defined(PRODUCT) || USE_DEBUG_PRINTING
  jlong interpreter_pair_counters[Bytecodes::number_of_java_codes *
                                  Bytecodes::number_of_java_codes];
  jlong interpreter_bytecode_counters[Bytecodes::number_of_java_codes];
#endif

  // bytecode stream accessors
#define GET_BYTE(x) (*(jubyte*)(g_jpc + (x) + 1))
#define GET_SIGNED_BYTE(x) (*(jbyte*)(g_jpc + (x) + 1))
#define GET_SHORT(x) \
         (jushort)(((jint)(GET_BYTE(x)) << 8) | (jint)GET_BYTE(x+1))
#define GET_SIGNED_SHORT(x) \
         (jshort) (((jint)(GET_BYTE(x)) << 8) | (jint)GET_BYTE(x+1))

//  ENABLE_NATIVE_ORDER_REWRITING  HARDWARE_LITTLE_ENDIAN   little-endian
//  ENABLE_NATIVE_ORDER_REWRITING !HARDWARE_LITTLE_ENDIAN   big-endian
// !ENABLE_NATIVE_ORDER_REWRITING  HARDWARE_LITTLE_ENDIAN   big-endian
// !ENABLE_NATIVE_ORDER_REWRITING !HARDWARE_LITTLE_ENDIAN   big-endian
#if HARDWARE_LITTLE_ENDIAN && ENABLE_NATIVE_ORDER_REWRITING
#define GET_SHORT_NATIVE(x) \
         (((jint)(GET_BYTE(x+1)) << 8) | (jint)(GET_BYTE(x)) )
#else
#define GET_SHORT_NATIVE(x) \
         ((((GET_BYTE(x))) << 8) | GET_BYTE(x+1))
#endif

#define GET_INT(x)  \
  ( ((jint)(GET_BYTE(x))    << 24)  | ((jint)(GET_BYTE(x+1)) << 16) | \
    ((jint)(GET_BYTE(x+2))  <<  8)  | ((jint)(GET_BYTE(x+3))) )
#define ADVANCE(x)           (g_jpc += (x))
#define ADVANCE_FOR_RETURN() ADVANCE((int)(GET_FRAME(return_advance)))

#define NULL_CHECK(obj) \
    if (obj == NULL) {                           \
      interpreter_throw_NullPointerException();  \
      return;                                    \
    }

#define BOUNDS_CHECK(obj, idx) \
    if (obj == NULL) {                                    \
      interpreter_throw_NullPointerException();           \
      return false;                                       \
    }                                                     \
    if (idx < 0 || idx >= GET_ARRAY_LENGTH(obj)) {        \
      interpreter_throw_ArrayIndexOutOfBoundsException(); \
      return false;                                       \
    }

  // method locals accessors
#define GET_LOCAL(n) *((jint*)g_jlocals - (n))
#define GET_LOCAL_LOW(n) *((jint*)g_jlocals - (n))
#define GET_LOCAL_HIGH(n) *((jint*)g_jlocals - (n) - 1)

#define SET_LOCAL(n, val) *((jint*)g_jlocals - (n)) = (val)
#define SET_LOCAL_LOW(n, val) *((jint*)g_jlocals - (n)) = (val)
#define SET_LOCAL_HIGH(n, val) *((jint*)g_jlocals - (n) - 1) = (val)

  // frame accessors
#define FRAME_OFFSET(name) (JavaFrame::name##_offset())
#define ENTRY_FRAME_OFFSET(name) (EntryFrame::name##_offset())
#define GET_FRAME(name) *(address*)(g_jfp + FRAME_OFFSET(name))
#define GET_FRAME_OFFSET(name, offset) \
        *(address*)(g_jfp + FRAME_OFFSET(name) + offset)
#define SET_FRAME(name, value) \
        *(address*)(g_jfp + FRAME_OFFSET(name)) = (value)
#define SET_FRAME_OFFSET(name, value, offset) \
        *(address*)(g_jfp + FRAME_OFFSET(name) + offset) = (value)
#define GET_ENTRY_FRAME(name) *(address*)(g_jfp + ENTRY_FRAME_OFFSET(name))
#define SET_ENTRY_FRAME(name, value) \
        *(jint*)(g_jfp + ENTRY_FRAME_OFFSET(name)) = (value)

  // Aliases for slots in the JavaFrame that are specific to the
  // C interpreter loop
  //
  // return_advance -- advance the g_jpc of the caller method by this amount
  //                   when executing a Xreturn bytecode.
#define return_advance stored_int_value1

  // array accessors
#define GET_ARRAY_LENGTH(ref) *((jint*)((ref)+Array::length_offset()))
#define SET_ARRAY_LENGTH(ref, len) \
           *((jint*)((ref)+Array::length_offset())) = (len)
#define GET_ARRAY_ELEMENT(ref, n, type) \
           *((type*)((ref)+Array::base_offset()) + (n))
#define SET_ARRAY_ELEMENT(ref, n, type, val) \
           *((type*)((ref)+Array::base_offset()) + (n)) = (val)

  // thread accessors
#define THREAD_OFFSET(name) Thread::name##_offset()
#define GET_THREAD_INT(name)                                    \
       *(jint*)((address)_current_thread + THREAD_OFFSET(name))
#define SET_THREAD_INT(name, value)                             \
       *(jint*)((address)_current_thread + THREAD_OFFSET(name)) = value

  // entry access
#define ENTRY_OFFSET(name) EntryActivation::name##_offset()
#define GET_ENTRY(entry, name)                                    \
       *(jint*)((address)entry + ENTRY_OFFSET(name))

  // stacklock access
#define STACKLOCK_OFFSET(name) StackLock::name##_offset()
#define GET_STACKLOCK(lock, name) *(address*)(lock + STACKLOCK_OFFSET(name))
#define GET_STACKLOCK_OFFSET(lock, name, offset) \
      *(address*)(lock + STACKLOCK_OFFSET(name) + offset)
#define SET_STACKLOCK(lock, name, value) \
        *(address*)(lock + STACKLOCK_OFFSET(name)) = (value)
#define SET_STACKLOCK_OFFSET(lock, name, offset, value) \
        *(address*)(lock + STACKLOCK_OFFSET(name) + offset) = (value)

  // near access
#define NEAR_OFFSET(name) JavaNear::name##_offset()
#define GET_NEAR(near, name) *(address*)(near + NEAR_OFFSET(name))
#define GET_NEAR_OFFSET(near, name, offset) \
        *(address*)(near + NEAR_OFFSET(name) + offset)

// argument for the native functions calls - current thread pointer
#define NATIVE_ARG ((Thread*)&_current_thread)

// auxiliary macros
#define FUNC_UNUSED(x)                                          \
    void x() {}

#define FUNC_UNIMPLEMENTED(x)                                   \
    void x() {                                                  \
      tty->print_cr("NOT IMPLEMENTED: %s", #x);                 \
      BREAKPOINT;                                               \
    }

#define UNIMPL(x)                                               \
    tty->print_cr("UNIMPLEMENTED BYTECODE: %s\n", #x);          \
    BREAKPOINT;

// Interpreter entrance/exit
enum {
  JMP_ENTER = 0,
  JMP_THREAD_EXIT,
  JMP_STOPPED_MANUALLY,
  JMP_CONTEXT_CHANGED
};

#define BREAK_INTERPRETER_LOOP(reason)                                \
    GUARANTEE(inside_interpreter, "Must be inside interpreter loop"); \
    longjmp(interpreter_env, reason)

  // useful inlines
  static inline address arg_address_from_sp(int index) {
    return (g_jsp + BytesPerWord * index);
  }

  static inline jint int_from_sp(int index) {
    return *(jint*)arg_address_from_sp(index);
  }

  static inline address address_from_sp(int index) {
    return *(address*)arg_address_from_sp(index);
  }

  static inline jushort first_ushort_from_cpool(address cpool, int index) {
#if HARDWARE_LITTLE_ENDIAN
    cpool += index * 4;
    return (*cpool | ((jushort)*(cpool + 1)) << 8);
#else
    cpool += index * 4 + 2;
    return (((jushort)(*cpool)) << 8 | *(cpool+1));
#endif
  }

  static inline jushort second_ushort_from_cpool(address cpool, int index) {
#if HARDWARE_LITTLE_ENDIAN
    cpool += index * 4 + 2;
    return (*cpool | ((jushort)*(cpool + 1)) << 8);
#else
    cpool += index * 4;
    return (((jushort)(*cpool)) << 8 | *(cpool+1));
#endif
  }

  static inline address get_class_by_id(jushort id) {
    return *(address*)(_class_list_base + id*4);
  }

  static inline address get_method_from_ci(address ci, jint idx) {
    return *(address*)(ci + idx * 4 + ClassInfoDesc::header_size());
  }

  static inline address get_method_from_vtable(address klazz, jint idx) {
    address ci = *(address*)(klazz + JavaNear::class_info_offset());
    return get_method_from_ci(ci, idx);
  }

  static inline jushort get_num_params(address method) {
    jushort value =
      *(jushort*)(method+Method::method_attributes_offset());
    value &= Method::SIZE_OF_PARAMETERS_MASK;
    return value;
  }

  static inline jushort get_access_flags(address method) {
    return *(jushort*)(method+Method::access_flags_offset());
  }

  static inline jushort get_max_locals(address method) {
    return *(jushort*)(method + Method::max_locals_offset());
  }

  static inline jushort get_max_stack(address method) {
    return *(jushort*)(method + Method::max_execution_stack_count_offset());
  }

  static inline jushort get_holder_id(address method) {
    return *(jushort*)(method + Method::holder_id_offset());
  }

  static inline address get_quick_native(address method) {
    return *(address*)(method + Method::quick_native_code_offset());
  }

  static inline jint get_from_cpool(address cpool, jushort index) {
    return *(jint*)(cpool + index * 4);
  }

  static inline void write_barrier(address addr) {
    ObjectHeap::set_bit_for((OopDesc**)addr);
  }

  static inline void call_from_interpreter(address addr, int offset) {
    func_t f = (func_t)(addr + offset);
    f();
    // in ASM interpreters we put call info right after function call
    // we have no need for that in C interpreter
  }

  static inline address get_cpool(address method) {
    address rv;

    if (get_access_flags(method) & JVM_ACC_HAS_COMPRESSED_HEADER)  {
      rv = (address)_rom_constant_pool;
    } else {
      rv = *(address*)(method + Method::constants_offset());
    }
    return (rv + ConstantPool::base_offset());
  }

  // stack accessors
  static inline jint POP() {
    jint value = *(jint*)g_jsp;
    g_jsp += sizeof(jint);
    return value;
  }

  static inline address OBJ_POP() {
    address value = *(address*)g_jsp;
    g_jsp += sizeof(address);
    return value;
  }

  static inline jfloat FLOAT_POP() {
    jfloat value = *(jfloat*)g_jsp;
    g_jsp += sizeof(jfloat);
    return value;
  }

  static inline void PUSH(jint v) {
    g_jsp -= sizeof(jint);
    *(jint*)g_jsp = v;
  }

  static inline void OBJ_PUSH(address v) {
    g_jsp -= sizeof(address);
    *(address*)g_jsp = v;
  }

  static inline void FLOAT_PUSH(jfloat v) {
    g_jsp -= sizeof(jfloat);
    *(jfloat*)g_jsp = v;
  }

  static inline jlong LONG_POP() {
    jlong_accessor acc;
    acc.words[0] = POP();
    acc.words[1] = POP();
    return acc.long_value;
  }

  static inline void LONG_PUSH(jlong val) {
    jlong_accessor acc;
    acc.long_value = val;
    PUSH(acc.words[1]);
    PUSH(acc.words[0]);
  }

  static inline jdouble DOUBLE_POP() {
     jdouble_accessor acc;
     acc.words[0] = POP();
     acc.words[1] = POP();
     return acc.double_value;
  }

  static inline void DOUBLE_PUSH(jdouble val) {
    jdouble_accessor acc;
    acc.double_value = val;
    PUSH(acc.words[1]);
    PUSH(acc.words[0]);
  }

  static inline jint PEEK(jint n)    {
    return *((jint*)g_jsp + n);
  }

  static inline address OBJ_PEEK(jint n) {
    return *((address*)g_jsp + n);
  }

  static inline jint int_from_addr(address addr) {
    return *(jint*)addr;
  }

  static inline jlong long_from_addr(address addr) {
    jlong_accessor acc;
    acc.words[0] = *(jint*)addr;
    acc.words[1] = *(jint*)(addr+4);
    return acc.long_value;
  }

  static inline void long_to_addr(address addr, jlong val) {
    jlong_accessor acc;
    acc.long_value = val;
    *(jint*)addr = acc.words[0];
    *(jint*)(addr+4) = acc.words[1];
  }

  static inline jdouble double_from_addr(address addr) {
    jdouble_accessor acc;
    acc.words[0] = *(jint*)addr;
    acc.words[1] = *(jint*)(addr+4);
    return acc.double_value;
  }

  static inline void double_to_addr(address addr, jdouble val) {
    jdouble_accessor acc;
    acc.double_value = val;
    *(jint*)addr = acc.words[0];
    *(jint*)(addr+4) = acc.words[1];
  }

  static inline address callee_method() {
    return (address)GET_THREAD_INT(obj_value);
  }

  static inline void set_callee_method(address method) {
    SET_THREAD_INT(obj_value, (int)method);
    write_barrier((address)_current_thread + Thread::obj_value_offset());
  }

  static inline bool interpreter_call_vm(address callback, BasicType rv_type) {
    return shared_call_vm_internal(callback, NULL, rv_type, 0);
  }

  static inline bool interpreter_call_vm_1(address callback, BasicType rv_type,
                                           jint arg1) {
    return shared_call_vm_internal(callback, NULL, rv_type, 1, arg1);
  }

  static inline bool interpreter_call_vm_2(address callback, BasicType rv_type,
                                           jint arg1, jint arg2) {
    return shared_call_vm_internal(callback, NULL, rv_type, 2, arg1, arg2);
  }

  static inline void check_timer_tick() {
#if ENABLE_PAGE_PROTECTION
    // use g_jpc to prevent compiler from optimizing this memory access
    _protected_page[INTERPRETER_TIMER_TICK_SLOT] = (int)g_jpc;
#else
    if (_rt_timer_ticks > 0) {
      interpreter_call_vm_1((address)&timer_tick, T_VOID, (jint)NATIVE_ARG);
    }
#endif
  }

  static inline void method_transition() {
#if ENABLE_WTK_PROFILER
    if (UseExactProfiler) {
      interpreter_call_vm((address)&jprof_record_method_transition, T_VOID);
    }
#endif
  }

  // invoker_size: size of the invoke bytecode in caller method
  static void invoke_java_method(address method, int invoker_size) {
    if (Verbose) {
      Method::Raw m = (ReturnOop)method;
      m().print_name_on_tty();
      tty->cr();
    }

    // store old values to be restored by return bytecode
    // only if called from another Java frame - otherwise frame is EntryFrame
    // and we can overwrite useful fields
    if (invoker_size) {
      SET_FRAME(locals_pointer, g_jlocals);
      SET_FRAME(bcp_store, g_jpc);
      SET_FRAME(return_advance, (address)invoker_size);
    }

    address exec_entry =
      *(address*)(method + Method::variable_part_offset());
    // double indirection to keep in-heap footprint of ROMized methods
    // minimal
    exec_entry = *(address*)exec_entry;

    // store method in obj_value field of current thread
    set_callee_method(method);

    // and invoke it
    call_from_interpreter(exec_entry, 0);
  }

#if ENABLE_FLOAT
  void native_math_sin_entry() {
    jdouble x = DOUBLE_POP();
    DOUBLE_PUSH(jvm_fplib_sin(x));
    ADVANCE_FOR_RETURN();
  }

  void native_math_cos_entry() {
    jdouble x = DOUBLE_POP();
    DOUBLE_PUSH(jvm_fplib_cos(x));
    ADVANCE_FOR_RETURN();
  }

  void native_math_tan_entry() {
    jdouble x = DOUBLE_POP();
    DOUBLE_PUSH(jvm_fplib_tan(x));
    ADVANCE_FOR_RETURN();
  }

  void native_math_sqrt_entry() {
    jdouble x = DOUBLE_POP();
    DOUBLE_PUSH(ieee754_sqrt(x));
    ADVANCE_FOR_RETURN();
  }

  void native_math_ceil_entry() {
    jdouble x = DOUBLE_POP();
    DOUBLE_PUSH(jvm_fplib_ceil(x));
    ADVANCE_FOR_RETURN();
  }

  void native_math_floor_entry() {
    jdouble x = DOUBLE_POP();
    DOUBLE_PUSH(jvm_fplib_floor(x));
    ADVANCE_FOR_RETURN();
  }

#if ENABLE_CLDC_111

  void native_math_asin_entry() {
    jdouble x = DOUBLE_POP();
    DOUBLE_PUSH(jvm_fplib_asin(x));
    ADVANCE_FOR_RETURN();
  }

  void native_math_acos_entry() {
    jdouble x = DOUBLE_POP();
    DOUBLE_PUSH(jvm_fplib_acos(x));
    ADVANCE_FOR_RETURN();
  }

  void native_math_atan_entry() {
    jdouble x = DOUBLE_POP();
    DOUBLE_PUSH(jvm_fplib_atan(x));
    ADVANCE_FOR_RETURN();
  }

  void native_math_atan2_entry() {
    jdouble y = DOUBLE_POP();
    jdouble x = DOUBLE_POP();
    DOUBLE_PUSH(jvm_fplib_atan2(x, y));
    ADVANCE_FOR_RETURN();
  }

#endif // ENABLE_CLDC_111

#endif // ENABLE_FLOAT

  void native_string_indexof_entry() {
    // we have no generic implementation, so do it here
    int fromIndex = POP();
    int ch = POP();
    address obj = OBJ_POP();

    if (ch > 0xffff) {
      PUSH(-1);
      ADVANCE_FOR_RETURN();
      return;
    }

    if (fromIndex < 0) {
      fromIndex = 0;
    }

    jint count = *(jint*)(obj + String::count_offset());

    if (fromIndex >= count) {
      PUSH(-1);
      ADVANCE_FOR_RETURN();
      return;
    }

    jint offset = *(jint*)(obj + String::offset_offset());
    int max = offset + count;
    jchar* v = (jchar*)
      (*(address*)(obj + String::value_offset()) + Array::base_offset());

    for (int i = offset + fromIndex ; i < max ; i++) {
      if (v[i] == ch) {
        PUSH(i - offset);
        ADVANCE_FOR_RETURN();
        return;
      }
    }

    PUSH(-1);
    ADVANCE_FOR_RETURN();
  }

  void native_string_init_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_string_equals_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_string_indexof0_entry() {
    // Push zero for fromIndex
    PUSH(0);

    native_string_indexof_entry();
  }

  void native_string_compareTo_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_string_charAt_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_string_indexof0_string_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_string_indexof_string_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_string_startsWith_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_string_startsWith0_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_string_endsWith_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_string_substringI_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_string_substringII_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_system_arraycopy_entry() {
    // call generic native implementation
    interpreter_method_entry();
  }

  void native_jvm_unchecked_byte_arraycopy_entry() {
    // call generic native implementation
    interpreter_method_entry();
  }

  void native_jvm_unchecked_char_arraycopy_entry() {
    // call generic native implementation
    interpreter_method_entry();
  }

  void native_jvm_unchecked_int_arraycopy_entry() {
    // call generic native implementation
    interpreter_method_entry();
  }

  void native_jvm_unchecked_long_arraycopy_entry() {
    // call generic native implementation
    interpreter_method_entry();
  }

  void native_jvm_unchecked_obj_arraycopy_entry() {
    // call generic native implementation
    interpreter_method_entry();
  }

  void native_vector_elementAt_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_vector_addElement_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_stringbuffer_append_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  void native_integer_toString_entry() {
    // call generic implementation
    interpreter_method_entry();
  }

  // those shouldn't be used, instead *_internal()
  // will work, as we need different method signature
  FUNC_UNIMPLEMENTED(shared_lock_synchronized_method);
  FUNC_UNIMPLEMENTED(shared_unlock_synchronized_method);
  FUNC_UNIMPLEMENTED(shared_call_vm);
  FUNC_UNIMPLEMENTED(shared_call_vm_oop);
  FUNC_UNIMPLEMENTED(shared_call_vm_exception);
  FUNC_UNIMPLEMENTED(interpreter_grow_stack);
  FUNC_UNIMPLEMENTED(shared_monitor_enter);
  FUNC_UNIMPLEMENTED(shared_monitor_exit);

  // compiler-related -  irrelevant for C interpreter
  FUNC_UNIMPLEMENTED(indirect_execution_sensor_update);
  FUNC_UNIMPLEMENTED(compiler_new_obj_array);
  FUNC_UNIMPLEMENTED(compiler_new_type_array);
  FUNC_UNIMPLEMENTED(compiler_new_object);
  FUNC_UNIMPLEMENTED(compiler_idiv_irem);
  FUNC_UNIMPLEMENTED(compiler_checkcast);
  FUNC_UNIMPLEMENTED(compiler_instanceof);
  FUNC_UNIMPLEMENTED(compiler_glue_code_start);
  FUNC_UNIMPLEMENTED(compiler_glue_code_end);
  FUNC_UNIMPLEMENTED(compiler_callvm_stubs_start);
  FUNC_UNIMPLEMENTED(compiler_callvm_stubs_end);

  FUNC_UNIMPLEMENTED(compiler_remove_patch);
  FUNC_UNIMPLEMENTED(compiler_timer_tick);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException_0);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException_1);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException_2);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException_3);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException_4);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException_5);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException_6);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException_7);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException_8);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException_9);
  FUNC_UNIMPLEMENTED(compiler_throw_NullPointerException_10);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException_0);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException_1);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException_2);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException_3);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException_4);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException_5);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException_6);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException_7);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException_8);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException_9);
  FUNC_UNIMPLEMENTED(compiler_throw_ArrayIndexOutOfBoundsException_10);
  FUNC_UNIMPLEMENTED(invoke3_deoptimization_entry_0)
  FUNC_UNIMPLEMENTED(invoke3_deoptimization_entry_1)
  FUNC_UNIMPLEMENTED(invoke3_deoptimization_entry_2)
  FUNC_UNIMPLEMENTED(invoke3_deoptimization_entry_3)
  FUNC_UNIMPLEMENTED(invoke3_deoptimization_entry_4)
  FUNC_UNIMPLEMENTED(invoke5_deoptimization_entry_0)
  FUNC_UNIMPLEMENTED(invoke5_deoptimization_entry_1)
  FUNC_UNIMPLEMENTED(invoke5_deoptimization_entry_2)
  FUNC_UNIMPLEMENTED(invoke5_deoptimization_entry_3)
  FUNC_UNIMPLEMENTED(invoke5_deoptimization_entry_4)

  // called but actually should do nothing
  FUNC_UNUSED(interpreter_deoptimization_entry)

  // maybe needs implementation
  FUNC_UNIMPLEMENTED(shared_invoke_debug);

#if ENABLE_METHOD_TRAPS
  FUNC_UNIMPLEMENTED(cautious_invoke);
#endif

#if !CROSS_GENERATOR
  jint _bytecode_counter;
#endif

  static bool lock_object(address object, address lock) {

    // Store the object in the stack lock and lock it
    *(address*)(lock + StackLock::size()) = object;

    // Get the near object
    address near_obj = *(address*)object;
    address tmp1;

    // see if it's an interned string
    if  (near_obj == (address)_interned_string_near_addr) {
      // go slow path
      goto slow;
    }

    // object already locked?
    if ((jint)GET_NEAR(near_obj, raw_value) & 0x1) {
      // maybe slow case

      // thread of object's lock
      tmp1 = *(address*)(near_obj + StackLock::thread_offset() -
                         StackLock::copied_near_offset());

      // is this a recursive lock?
      if ((address)_current_thread == tmp1) {
        SET_STACKLOCK(lock, real_java_near, 0);
        return false;
      }

      // slow_case
    slow:
      // clear our near for GC pleasure
      SET_STACKLOCK(lock, real_java_near, 0);
      return shared_call_vm_internal((address)&lock_stack_lock, NULL,
                                     T_VOID, 1, lock);
    }

    // We have the fast case

    // Clear waiters
    SET_STACKLOCK(lock, waiters, 0);

    // Update the near pointer in the object
    *(address*)object = lock + StackLock::copied_near_offset();

    // fill in stacklock fields
    SET_STACKLOCK(lock, real_java_near, near_obj);
    SET_STACKLOCK(lock, thread, (address)_current_thread);

    // Copy the locked near object to the stack
    SET_STACKLOCK_OFFSET(lock, copied_near, 0,
                         *(address*)(near_obj + 0));
    SET_STACKLOCK_OFFSET(lock, copied_near, 4,
                         *(address*)(near_obj+4));
    // set lock bit
    jint raw_value =  *(jint*)(near_obj+8) | 0x1;
    SET_STACKLOCK_OFFSET(lock, copied_near, 8,
                         (address)raw_value);
    return false;
  }

  static bool unlock_object(address object, address lock) {
    // Get near object
    address tmp1 =  *(address*)object;
    if (tmp1 == (address)_interned_string_near_addr) {
       return shared_call_vm_internal((address)&unlock_special_stack_lock, NULL,
                                      T_VOID, 1, object);
    }

    // Get the real java near pointer from the stack lock
    tmp1 = GET_STACKLOCK(lock, real_java_near);

    // Zero out the object field so that lock is free
    *(address*)(lock + StackLock::size()) = NULL;

    // Is this the reentrant case
    if (tmp1 == NULL) {
      return false;
    }

    // Set the object near pointer
    // tmp1 contains the real java near
    *(address*)object = tmp1;

    // The last argument can be false if we separate out the case of
    // unlocking synchronized methods into a separate case. There are
    // no synchronized methods in String.

    // Oop write barrier is needed in case new near object has been assigned
    // during locking.  But this happens so rarely that it is worth our while to
    // to do a quick test to avoid it.
    if (tmp1 >= object) {
      write_barrier(object);
    }

    address tmp0 = GET_STACKLOCK_OFFSET(lock, copied_near,
                                        JavaNear::raw_value_offset());

    // Check for waiters
    if (((jint)tmp0 & 2) == 0) {
      return false;
    }

    // Call the runtime system to signal the waiters
    return shared_call_vm_internal((address)&signal_waiters, NULL,
                                   T_VOID, 1, lock);
  }

  static bool lock_synchronized_method_internal(address object) {
    // Allocate space for the monitor
    g_jsp -=  StackLock::size() + BytesPerWord;
    SET_FRAME(stack_bottom_pointer, g_jsp);
    address lock = g_jsp; // pointer to the stacklock
    return lock_object(object, lock);
  }

  static bool unlock_synchronized_method_internal() {
    // get address of last lock
    address object = GET_FRAME_OFFSET(first_stack_lock, StackLock::size());
    address lock = g_jfp + JavaFrame::first_stack_lock_offset();

    // IMPL_NOTE: shouldn't it be lock?
    if (!object) {
      // lock better be locked
      return interpreter_call_vm((address)&illegal_monitor_state_exception,
                                 T_VOID);
    }

    return unlock_object(object, lock);
  }

  static bool unlock_activation() {
    // unlock_activation
    address lock = GET_FRAME(stack_bottom_pointer);

    // See if there are any locks on this frame
    if (lock == g_jfp + JavaFrame::empty_stack_offset()) {
      return false;
    }

    // check if the method is synchronized
    jushort flagz = get_access_flags(GET_FRAME(method));

    if (flagz & JVM_ACC_SYNCHRONIZED) {
      if (unlock_synchronized_method_internal()) {
        return true;
      }

      // reread locals as could be moved by call outside interpreter
      lock = GET_FRAME(stack_bottom_pointer);
    }

    // Check that all stack locks have been unlocked
    // lock is pointing at first word of stack lock

    address tmp1 = g_jfp + JavaFrame::stack_bottom_pointer_offset();

    // We know that there is at least one stack lock, or we wouldn't be here in
    // the first place!
    do {
      GUARANTEE(lock <= tmp1, "sanity");
      if (*(address*)(lock + StackLock::size()) != NULL) {
        return interpreter_call_vm((address)&illegal_monitor_state_exception,
                                   T_VOID);
      }
      lock += StackLock::size() + BytesPerWord;
    } while (lock != tmp1);

    return false;
  }

  static address allocate_monitor() {
    // Set lock to old expression stack bottom
    address lock = GET_FRAME(stack_bottom_pointer);

    // Move expression stack top and bottom
    jint len = StackLock::size() + 4;
    g_jsp -= len;
    lock -= len;
    SET_FRAME(stack_bottom_pointer, lock);

    // Move the expression stack contents
    //jvm_memmove(lock, lock + len, g_jsp - lock);
    address tmp1 = g_jsp;
    while (tmp1 != lock) {
      *(jint*)tmp1 = *(jint*)(tmp1 + len);
      tmp1 += 4;
    }

    return lock;
  }

  static bool monitor_enter_internal(address object) {
    GUARANTEE(sizeof(JavaNearDesc) == 3 * BytesPerWord, "Sanity");
    GUARANTEE(JavaNear::raw_value_offset() == 2 * BytesPerWord, "sanity");

    address tmp0 = GET_FRAME(stack_bottom_pointer);
    address tmp1 = g_jfp + JavaFrame::stack_bottom_pointer_offset();
    address lock = NULL;

    // Find free slot in monitor block
    while (tmp0 != tmp1) {
      // Start the loop by checking if the current stack lock is empty
      address tmp2 = *(address*)(tmp0 + StackLock::size());

      if (tmp2 == 0) {
        lock = tmp0;
      }

      // Check to see if the object in the current stack lock is equal
      // to the object from the stack
      if (tmp2 == object) {
        break;
      }

      tmp0 += BytesPerWord + StackLock::size();
    }

    if (!lock) {
      lock = allocate_monitor();
    }

    return lock_object(object, lock);
  }

  static bool monitor_exit_internal(address object) {
    if (*(address*)object == (address)_interned_string_near_addr) {
      // IMPL_NOTE: we already check for _interned_strings in unlock_object()
      return shared_call_vm_internal((address)&unlock_special_stack_lock, NULL,
                                     T_VOID, 1, object);
    }

    address lock = GET_FRAME(stack_bottom_pointer);
    // address of lock just beyond the first
    address tmp1 = g_jfp + JavaFrame::pre_first_stack_lock_offset();
    //JavaFrame::stack_bottom_pointer_offset();

    // loop to find correct monitor
    bool found = false;
    // at the end of the monitor block?
    while (lock != tmp1) {
      // Is monitor pointed at by lock the right one?

      // get object field
      address tmp0 = *(address*)(lock + StackLock::size());

      // is this the right object?
      if (object == tmp0) {
        found = true;
        break;
      }

      // Go to next lock
      lock += BytesPerWord + StackLock::size();
    }

    if (!found) {
      return interpreter_call_vm((address)&illegal_monitor_state_exception,
                                 T_VOID);
    }

    return unlock_object(object, lock);
  }

  static void invoke_native_entry(address entry_point,
                                  BasicType return_value_type,
                                  jint num_args,
                                  va_list ap,
                                  jint &r1, jint &r2)
  {
    jint arg1, arg2;
    switch (return_value_type) {
    // IMPL_NOTE: consider about other int types, like short, byte
    case T_INT:
      {
        int_func_t f = (int_func_t)entry_point;

        switch (num_args) {
        case 0:
          r1 = f(NATIVE_ARG);
          break;
        case 1:
          arg1 = va_arg(ap, jint);
          r1 = f(NATIVE_ARG, arg1);
          break;
        case 2:
          arg1 = va_arg(ap, jint);
          arg2 = va_arg(ap, jint);
          r1 = f(NATIVE_ARG, arg1, arg2);
          break;
        default:
          SHOULD_NOT_REACH_HERE();
        }
      }
      break;
    case T_LONG:
      {
        long_func_t f = (long_func_t)entry_point;
        jlong_accessor acc;

        switch (num_args) {
        case 0:
          acc.long_value = f(NATIVE_ARG);
          break;
        case 1:
          arg1 = va_arg(ap, jint);
          acc.long_value = f(NATIVE_ARG, arg1);
          break;
        case 2:
          arg1 = va_arg(ap, jint);
          arg2 = va_arg(ap, jint);
          acc.long_value = f(NATIVE_ARG, arg1, arg2);
          break;
        default:
          SHOULD_NOT_REACH_HERE();
          return;
        }

        r1 = acc.words[0];
        r2 = acc.words[1];
      }
      break;
#if ENABLE_FLOAT
    case T_FLOAT:
      {
        float_func_t f = (float_func_t)entry_point;
        jfloat val;
        switch (num_args) {
        case 0:
          val = f(NATIVE_ARG);
          break;
        case 1:
          arg1 = va_arg(ap, jint);
          val = f(NATIVE_ARG, arg1);
          break;
        case 2:
          arg1 = va_arg(ap, jint);
          arg2 = va_arg(ap, jint);
          val = f(NATIVE_ARG, arg1, arg2);
          break;
        default:
          SHOULD_NOT_REACH_HERE();
          return;
        }
        r1 = float_bits(val);
      }
      break;
    case T_DOUBLE:
      {
        double_func_t f = (double_func_t)entry_point;
        jdouble_accessor acc;

        switch (num_args) {
        case 0:
          acc.double_value = f(NATIVE_ARG);
          break;
        case 1:
          arg1 = va_arg(ap, jint);
          acc.double_value = f(NATIVE_ARG, arg1);
          break;
        case 2:
          arg1 = va_arg(ap, jint);
          arg2 = va_arg(ap, jint);
          acc.double_value = f(NATIVE_ARG, arg1, arg2);
          break;
        default:
          SHOULD_NOT_REACH_HERE();
          return;
        }
        r1 = acc.words[0];
        r2 = acc.words[1];
      }
      break;
#endif // ENABLE_FLOAT
    case T_OBJECT:
    case T_ARRAY:
      {
        oop_func_t f = (oop_func_t)entry_point;
        switch (num_args) {
        case 0:
          r1 = (jint)f(NATIVE_ARG);
          break;
        case 1:
          arg1 = va_arg(ap, jint);
          r1 = (jint)f(NATIVE_ARG, arg1);
          break;
        case 2:
          arg1 = va_arg(ap, jint);
          arg2 = va_arg(ap, jint);
          r1 = (jint)f(NATIVE_ARG, arg1, arg2);
          break;
        default:
          SHOULD_NOT_REACH_HERE();
        }
      }
      break;
    case T_VOID:
      {
        void_func_t f = (void_func_t)entry_point;

        switch (num_args) {
        case 0:
          f(NATIVE_ARG);
          break;
        case 1:
          arg1 = va_arg(ap, jint);
          f(NATIVE_ARG, arg1);
          break;
        case 2:
          arg1 = va_arg(ap, jint);
          arg2 = va_arg(ap, jint);
          f(NATIVE_ARG, arg1, arg2);
          break;
        default:
          SHOULD_NOT_REACH_HERE();
        }
      }
      break;
    default:
      SHOULD_NOT_REACH_HERE();
    }
  }

  static bool resume_thread() {
    // restore values from current thread structure
    g_jsp = (address)GET_THREAD_INT(stack_pointer);
    g_jfp = OBJ_POP();
    // remove dummy
    POP();
    address return_point = OBJ_POP();

    // check for pending activations
    address pentry = (address)GET_THREAD_INT(pending_entries);
    if (pentry) {
      if (GET_THREAD_INT(status) & THREAD_TERMINATING) {
        SET_THREAD_INT(pending_entries, 0);
        BREAK_INTERPRETER_LOOP(JMP_THREAD_EXIT);
      } else {
        jint max_len = GET_ENTRY(pentry, length);
        while ((pentry = (address)GET_ENTRY(pentry, next)) != NULL) {
          jint len = GET_ENTRY(pentry, length);
          if (len > max_len) {
            max_len = len;
          }
        }
        max_len = max_len * 8 + StackLock::size() + 4;
        // stack grows to smaller addresses
        address new_stack_ptr = (address)GET_THREAD_INT(stack_pointer) - max_len;
        // increase stack size
        if (new_stack_ptr < _current_stack_limit) {
          if (interpreter_call_vm_1((address)&stack_overflow, T_VOID,
                                    (jint)new_stack_ptr)) {
            return true;
          }
        }
        // invoke the pending activation
        shared_entry(return_point);
        return true;
      }
    }

    // read data from frame
    g_jpc = GET_FRAME(bcp_store);
    g_jlocals = GET_FRAME(locals_pointer);

    if (return_point != NULL) {
      ((func_t)(return_point))();
    }

    // reset last java frame in thread
    SET_THREAD_INT(last_java_sp, 0);
    SET_THREAD_INT(last_java_fp, 0);

    if (CURRENT_HAS_PENDING_EXCEPTION) {
      OopDesc* exception =_current_pending_exception;
      _current_pending_exception = NULL;
      shared_call_vm_internal(NULL, NULL, T_ILLEGAL, 1, (jint)exception);
      return true;
    }
    return false;
  }

  static bool shared_call_vm_internal(address entry_point,
                                      address return_point,
                                      BasicType return_value_type,
                                      jint num_args,
                                      ...)
  {
    // Save last java stack pointer and last java frame pointer in thread
    SET_THREAD_INT(last_java_fp, (jint)g_jfp);
    SET_THREAD_INT(last_java_sp, (jint)g_jsp);
    // sync up other globals
    SET_FRAME(bcp_store, g_jpc);
    SET_FRAME(locals_pointer, g_jlocals);

    // dummies to keep stack layout
    // should be return address of our caller
    OBJ_PUSH(return_point);

    // dummy location, in case stack size is 0, and we need an exception
    if (return_value_type == T_ILLEGAL) {
      // If the top-most frame catches the exception, we need to make sure that
      // there is sufficient space for us to expand its expression stack to
      // hold one element.
      PUSH(0);
    }
    // should be return address into this function
    PUSH(0xDEADBEEF);
    // fp
    OBJ_PUSH(g_jfp);
    SET_THREAD_INT(stack_pointer, (jint)g_jsp);

    bool is_oop = (stack_type_for(return_value_type) == T_OBJECT);

    jint r1 = 0, r2 = 0;
    va_list ap;
    va_start(ap, num_args);

    if (return_value_type == T_ILLEGAL) {
      GUARANTEE(num_args == 1, "Only 1 argument allowed here");
      OopDesc* raw_exc =  (OopDesc*)va_arg(ap, jint);

      // this call also fills in last_java_fp and last_java_sp in
      // current thread to the frame of handler
      r1 = (jint)find_exception_frame(NATIVE_ARG, raw_exc);

      g_jsp = (address)GET_THREAD_INT(last_java_sp);
      g_jfp = (address)GET_THREAD_INT(last_java_fp);

      // A return value of zero means the initial entry frame
      if (r1 == 0) {
        va_end(ap);

        // exception is stored in frame
        _current_pending_exception =
          (OopDesc*)GET_ENTRY_FRAME(pending_exception);

        BREAK_INTERPRETER_LOOP(JMP_THREAD_EXIT);
      } else {
        // update Java stack with new g_jfp, so that resume_thread() will
        // proceed with exception handler in the right Java frame
        PUSH(0);
        PUSH(0xDEADBEEF);
        OBJ_PUSH(g_jfp);
        SET_THREAD_INT(stack_pointer, (jint)g_jsp);
      }
    } else {
      // Call the native entry
      invoke_native_entry(entry_point, return_value_type, num_args,
                          ap, r1, r2);
    }

    va_end(ap);

    // save the return values in the thread
    if (is_oop) {
      SET_THREAD_INT(obj_value, r1);
      write_barrier((address)_current_thread + Thread::obj_value_offset());
    } else {
      SET_THREAD_INT(int1_value, r1);
      SET_THREAD_INT(int2_value, r2);
    }

    switch_thread(NATIVE_ARG);
    return resume_thread();
  }

  static void shared_entry(address return_point) {
    // make space on stack for EntryFrame
    g_jsp -=  EntryFrame::frame_desc_size();
    g_jfp = g_jsp - EntryFrame::empty_stack_offset();

    // stored data in frame
    SET_ENTRY_FRAME(pending_exception,   (jint)_current_pending_exception);
    SET_ENTRY_FRAME(stored_last_sp,      GET_THREAD_INT(last_java_sp));
    SET_ENTRY_FRAME(stored_last_fp,      GET_THREAD_INT(last_java_fp));
    SET_ENTRY_FRAME(fake_return_address, EntryFrame::FakeReturnAddress);
    SET_ENTRY_FRAME(real_return_address, (jint)return_point);
    SET_ENTRY_FRAME(stored_obj_value,    GET_THREAD_INT(obj_value));
    SET_ENTRY_FRAME(stored_int_value1,   GET_THREAD_INT(int1_value));
    SET_ENTRY_FRAME(stored_int_value2,   GET_THREAD_INT(int2_value));

    // reset last java frame in thread
    SET_THREAD_INT(last_java_sp, 0);
    SET_THREAD_INT(last_java_fp, 0);

    // Get the pending exception and (first) pending activation from the thread
    address pending_entry = (address)GET_THREAD_INT(pending_entries);

    // clear the pending activation and pending exception thread field
    SET_THREAD_INT(pending_entries, 0);

    _current_pending_exception = 0;

    jint entry = GET_ENTRY(pending_entry, next);
    jint len = GET_ENTRY(pending_entry, length);

    // save next entry
    SET_ENTRY_FRAME(pending_activation, entry);

    address header = pending_entry  + EntryActivationDesc::header_size();

    // put values on stack, note skipping of tags
    for (int i = 0; i < len; i++) {
      PUSH(*((jint*)header + 2 * i));
    }

    // find Java method
    address method = (address)GET_ENTRY(pending_entry, method);
    // invoke it
    invoke_java_method(method, 0);
  }

  static void compile_current_method() {
#if ENABLE_COMPILER
    SETUP_ERROR_CHECKER_ARG;
    Method m = (OopDesc*)callee_method();
    m.compile(0, false JVM_CHECK);
#endif
  }

  bool interpreter_grow_stack_internal(address method,
                                       address new_stack_ptr) {
    // please note that here we have simplification over ASM version:
    // as we aren't afraid to be interrupted by system interrupt
    // leaving stack in inconsistent way, we can easily shift FP at the
    // very beginning
    address old_jfp = g_jfp;
    // Reserve space on stack for frame
    g_jsp -= JavaFrame::frame_desc_size();

    // make new frame
    g_jfp = g_jsp - JavaFrame::empty_stack_offset();

    // Fill in fields of the frame
    SET_FRAME(stack_bottom_pointer, g_jsp);
    SET_FRAME(locals_pointer, g_jlocals);
    SET_FRAME(method, method);
    SET_FRAME(cpool, get_cpool(method));
    SET_FRAME(caller_fp, old_jfp);
    SET_FRAME(return_address, 0);

    // Create a fake bcp with a new flag set.
    g_jpc =  (address)
      (((juint)method + Method::base_offset()) + JavaFrame::overflow_frame_flag);

    if (interpreter_call_vm_1((address)stack_overflow, T_VOID,
                              (jint)new_stack_ptr)) {
      return true;
    }

    g_jlocals = GET_FRAME(locals_pointer);
    g_jsp = g_jfp + JavaFrame::end_of_locals_offset();
    g_jfp = GET_FRAME(caller_fp);
    return false;
  }

  // This function is to be used with ENABLE_PAGE_PROTECTION=true
  void interpreter_timer_tick() {
    // Save current state like shared_call_vm does
    SET_THREAD_INT(last_java_fp, (jint)g_jfp);
    SET_THREAD_INT(last_java_sp, (jint)g_jsp);
    SET_FRAME(bcp_store, g_jpc);
    SET_FRAME(locals_pointer, g_jlocals);
    OBJ_PUSH(NULL);
    PUSH(0xDEADBEEF);
    OBJ_PUSH(g_jfp);
    SET_THREAD_INT(stack_pointer, (jint)g_jsp);
    // Call the native routine
    timer_tick();
    switch_thread(NATIVE_ARG);
    // interpreter_timer_tick is called from a signal handler
    // in the context of another function. We can't just return from here,
    // but we may use longjmp to restore primordial context.
    BREAK_INTERPRETER_LOOP(JMP_CONTEXT_CHANGED);
  }

  static jint quick_native_throw(Thread *thread, const char* name,
    const char* message) {
    (void)thread;
    return KNI_ThrowNew(name, message);
  }

  static void quick_native_method_entry(BasicType return_type) {
    address method = callee_method();

    GUARANTEE((get_access_flags(method) & JVM_ACC_SYNCHRONIZED) == 0,
              "QuickNative methods must not be synchronized");

    // Get the method parameter size
    jushort num_params = get_num_params(method);

    // Point _kni_parameter_base to the first parameter
    address locals = g_jsp +
      num_params * BytesPerStackElement + JavaFrame::arg_offset_from_sp(-1);

    // Set space for fake parameter for static method (KNI-ism)
    if (get_access_flags(method) & JVM_ACC_STATIC) {
      locals += BytesPerStackElement;
    }
    _kni_parameter_base = locals;

    // Get the native method pointer from the bytecode
    address native_ptr = get_quick_native(method);

    _jvm_in_quick_native_method = 1;
    jint r1, r2;
    invoke_native_entry(native_ptr, return_type, 0, NULL, r1, r2);
    _jvm_in_quick_native_method = 0;

    if (_jvm_quick_native_exception != NULL) {
      interpreter_call_vm_2((address)&quick_native_throw, T_INT,
        (jint)_jvm_quick_native_exception, 0);
      _jvm_quick_native_exception = NULL;
      return;
    }

    // remove parameters from stack
    g_jsp +=  num_params * BytesPerStackElement;

    switch (return_type) {
    case T_INT:
    case T_OBJECT:
      PUSH(r1);
      break;
    case T_VOID:
      // do nothing
      break;
    default:
      SHOULD_NOT_REACH_HERE();
    }
    ADVANCE_FOR_RETURN();
  }

  void quick_void_native_method_entry() {
    quick_native_method_entry(T_VOID);
  }

  void quick_int_native_method_entry() {
    quick_native_method_entry(T_INT);
  }

  void quick_obj_native_method_entry() {
    quick_native_method_entry(T_OBJECT);
  }

#if ENABLE_ISOLATES

  extern void thread_task_cleanup();

  static inline address get_current_task() {
    return (address)_current_task;
  }

  static inline address get_mirror_by_id(jushort id) {
    return *((address*)_mirror_list_base + id);
  }
  static inline address get_cib_marker() {
    return (address)_task_class_init_marker;
  }

  static inline void reset_bits(address& addr, int mask) {
    addr = (address)((int)addr & ~mask);
  }

  static inline address get_real_mirror(address obj) {
    return *(address*)(obj + TaskMirror::real_java_mirror_offset());
  }

  static inline address get_clinit_list() {
    return *(address*)(get_current_task() + Task::clinit_list_offset());
  }

  address get_mirror_from_clinit_list(address clazz) {
    address tmp = get_clinit_list();
    while (clazz != *(address*)(tmp + TaskMirror::containing_class_offset())) {
      tmp = *(address*)(tmp + TaskMirror::next_in_clinit_list_offset());
      GUARANTEE(tmp != NULL, "sanity");
    }
    return tmp;
  }

  static inline bool get_method_task_mirror(address method,
                                            address& real_mirror) {
    address clazz = NULL;
    jushort id = get_holder_id(method);
    address task_mirror = get_mirror_by_id(id);
    if (task_mirror != get_cib_marker()) {
      real_mirror = get_real_mirror(task_mirror);
      return false;
    }
    clazz = get_class_by_id(id);
    if (interpreter_call_vm_1((address)task_barrier, T_OBJECT, (jint)clazz)) {
      return true;
    }
    task_mirror = (address)GET_THREAD_INT(obj_value);
    GUARANTEE(task_mirror != 0, "Task mirror is zero");
    real_mirror = get_real_mirror(task_mirror);
    return false;
  }

#else

  static inline bool is_initialized_class(address klass) {
    address java_mirror = *(address*)(klass + JavaClass::java_mirror_offset());
    jint status = *(jint*)(java_mirror + JavaClassObj::status_offset());
    if (status & JavaClassObj::INITIALIZED) {
      return true;
    }
    if (status & JavaClassObj::IN_PROGRESS) {
      address this_thread = (address)GET_THREAD_INT(thread_obj);
      address init_thread = *(address*)(java_mirror +
                                        JavaClassObj::thread_offset());
      return this_thread == init_thread;
    }
    return false;
  }

#endif // ENABLE_ISOLATES

  void interpreter_method_entry() {
    address method = callee_method();

    // Get the method parameter size
    jushort num_params = get_num_params(method);

    // setup locals
    g_jlocals = g_jsp +
      num_params * BytesPerStackElement + JavaFrame::arg_offset_from_sp(-1);

    // stack overflow check
    address new_stack_ptr =  g_jsp - get_max_stack(method) * 4;

    if (new_stack_ptr < _current_stack_limit) {
      if (interpreter_grow_stack_internal(method, new_stack_ptr)) {
        return;
      }
      // IMPL_NOTE: reread callee from thread, as may be moved by GC
      method = callee_method();
    }

    // get the size of the locals
    jushort max_locals = get_max_locals(method);

    // reserve space for locals
    g_jsp -= (max_locals - num_params) * BytesPerStackElement;
    // Reserve space on stack for frame
    g_jsp -= JavaFrame::frame_desc_size();

    // Set the Frame pointer
    address old_jfp = g_jfp;

    // set frame pointer
    g_jfp = g_jsp - JavaFrame::empty_stack_offset();

    // Fill in fields of the frame
    SET_FRAME(stored_int_value1, 0x0);
    SET_FRAME(stored_int_value2, 0x0);
    SET_FRAME(stack_bottom_pointer, g_jsp);
    SET_FRAME(locals_pointer, g_jlocals);
    SET_FRAME(method, method);
    SET_FRAME(cpool, get_cpool(method));
    SET_FRAME(caller_fp, old_jfp);
    SET_FRAME(return_address, g_jpc);

    // set bcp
    g_jpc = method + Method::base_offset();

    // check if the method is synchronized
    jushort flagz = get_access_flags(method);

    if (flagz & JVM_ACC_SYNCHRONIZED) {
      address obj = NULL;
      if (flagz & JVM_ACC_STATIC) {
#if ENABLE_ISOLATES
        if (get_method_task_mirror(method, obj)) {
          return;
        }
#else
        // Static method - synchronize on the mirror of the holder of
        // constant pool of current method
        jushort id = get_holder_id(method);
        obj = get_class_by_id(id);
        obj = *(address*)(obj + JavaClass::java_mirror_offset());
#endif
      } else {
        // Synchronize on local 0
        obj = (address)GET_LOCAL(0);
      }
      if (lock_synchronized_method_internal(obj)) {
        return;
      }
    }

    method_transition();
  }

  static void dispatch_return_point() {
    if (!CURRENT_HAS_PENDING_EXCEPTION) {
      interpreter_dispatch_table[GET_THREAD_INT(int1_value)]();
    }
  }

  static inline void interpreter_call_vm_dispatch(address callback,
                                                  BasicType rv_type) {
    GUARANTEE(rv_type == T_INT, "Only ints allowed here");
    // Call the shared call vm and disregard any return value
    shared_call_vm_internal(callback, (address)&dispatch_return_point,
                            T_INT, 0);
  }

  static inline void interpreter_call_vm_redo(address callback, BasicType rv_type) {
    // Call the shared call vm and disregard any return value
    shared_call_vm_internal(callback, NULL, rv_type, 0);
#if ENABLE_JAVA_DEBUGGER
    if (_debugger_active != 0) {
      if (*g_jpc == Bytecodes::_breakpoint) {
        interpreter_dispatch_table[GET_THREAD_INT(int1_value)]();
      }
    }
#endif
  }

  FUNC_UNIMPLEMENTED(interpreter_throw_NullPointerException_tos_cached)

  void interpreter_throw_NullPointerException() {
    interpreter_call_vm((address)&null_pointer_exception, T_VOID);
  }

  void interpreter_throw_ArrayIndexOutOfBoundsException() {
    interpreter_call_vm((address)&array_index_out_of_bounds_exception, T_VOID);
  }

  void interpreter_throw_IncompatibleClassChangeError() {
    interpreter_call_vm((address)&incompatible_class_change_error, T_VOID);

  }

  void interpreter_throw_ArithmeticException() {
    interpreter_call_vm((address)&arithmetic_exception, T_VOID);
  }

  static inline bool type_check(address ref, address obj, jint idx) {
    if (!obj) {
      return true;
    }
    OBJ_PUSH(ref);
    PUSH(idx);
    OBJ_PUSH(obj);
    if (!interpreter_call_vm((address)&array_store_type_check, T_VOID)) {
      // clear arguments from stack
      g_jsp += 3 * sizeof(jint);
      return true;
    }
    return false;
  }

  // Shared fast accessors

  static jushort get_fast_accessor_offset() {
    // No frame is constructed for fast accessors
    address method = callee_method();
    return *(jushort*)(method + Method::fast_accessor_offset_offset());
  }

  void shared_fast_getbyte_accessor() {
    PUSH(*(jbyte*)(OBJ_POP() + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getshort_accessor() {
    PUSH(*(jshort*)(OBJ_POP() + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getchar_accessor() {
    PUSH(*(jushort*)(OBJ_POP() + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getint_accessor() {
    PUSH(*(jint*)(OBJ_POP() + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getlong_accessor() {
    LONG_PUSH(long_from_addr(OBJ_POP() + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getbyte_static_accessor() {
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(*(jbyte*)(obj + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getshort_static_accessor() {
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(*(jshort*)(obj + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getchar_static_accessor() {
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(*(jushort*)(obj + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getint_static_accessor() {
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(*(jint*)(obj + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getlong_static_accessor() {
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    LONG_PUSH(long_from_addr(obj + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getfloat_accessor() {
    FLOAT_PUSH(*(jfloat*)(OBJ_POP() + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getdouble_accessor() {
    DOUBLE_PUSH(double_from_addr(OBJ_POP() + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getfloat_static_accessor() {
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    FLOAT_PUSH(*(jfloat*)(obj + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  void shared_fast_getdouble_static_accessor() {
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    DOUBLE_PUSH(double_from_addr(obj + get_fast_accessor_offset()));
    ADVANCE_FOR_RETURN();
  }

  // Fixed method entries

  void shared_invoke_compiler() {
    shared_call_vm_internal((address)&compile_current_method, NULL, T_VOID, 0);
    interpreter_method_entry();
  }

  void fixed_interpreter_method_entry() {
    interpreter_method_entry();
  }

  void fixed_interpreter_fast_method_entry_0() {
    interpreter_method_entry();
  }

  void fixed_interpreter_fast_method_entry_1() {
    interpreter_method_entry();
  }

  void fixed_interpreter_fast_method_entry_2() {
    interpreter_method_entry();
  }

  void fixed_interpreter_fast_method_entry_3() {
    interpreter_method_entry();
  }

  void fixed_interpreter_fast_method_entry_4() {
    interpreter_method_entry();
  }

  // Fast method entries

  void interpreter_fast_method_entry_0() {
    interpreter_method_entry();
  }

  void interpreter_fast_method_entry_1() {
    interpreter_method_entry();
  }

  void interpreter_fast_method_entry_2() {
    interpreter_method_entry();
  }

  void interpreter_fast_method_entry_3() {
    interpreter_method_entry();
  }

  void interpreter_fast_method_entry_4() {
    interpreter_method_entry();
  }

  static bool array_load(BasicType type) {
    jint    idx = POP();
    address ref = OBJ_POP();
    BOUNDS_CHECK(ref, idx);

    switch (type) {
    case T_BYTE    :
      {
        jint v = GET_ARRAY_ELEMENT(ref, idx, jbyte);
        PUSH(v);
      }
      break;
    case T_CHAR    :
      {
        jint v = GET_ARRAY_ELEMENT(ref, idx, jchar);
        PUSH(v);
      }
      break;
    case T_SHORT   :
      {
        jint v = GET_ARRAY_ELEMENT(ref, idx, jshort);
        PUSH(v);
      }
      break;
    case T_INT     :
      {
        jint v = GET_ARRAY_ELEMENT(ref, idx, jint);
        PUSH(v);
      }
      break;
    case T_LONG    :
      {
        jlong v = long_from_addr(ref + Array::base_offset() + idx*8);
        LONG_PUSH(v);
      }
      break;
    case T_FLOAT   :
      {
        jfloat v = GET_ARRAY_ELEMENT(ref, idx, jfloat);
        FLOAT_PUSH(v);
      }
      break;
    case T_DOUBLE  :
      {
        jdouble v = double_from_addr(ref + Array::base_offset() + idx*8);
        DOUBLE_PUSH(v);
      }
      break;
    case T_OBJECT  :
      {
        jint v = GET_ARRAY_ELEMENT(ref, idx, jint);
        PUSH(v);
      }
      break;
    default        :
      SHOULD_NOT_REACH_HERE();
    }

    return true;
  }

  static bool array_store(BasicType type) {
    switch (type) {
    case T_BYTE    :
      {
        jbyte    val = POP();
        jint     idx = POP();
        address  ref = OBJ_POP();
        BOUNDS_CHECK(ref, idx);
        SET_ARRAY_ELEMENT(ref, idx, jbyte, val);
      }
      break;
    case T_CHAR    :
      {
        jushort val = POP();
        jint    idx  = POP();
        address ref  = OBJ_POP();
        BOUNDS_CHECK(ref, idx);
        SET_ARRAY_ELEMENT(ref, idx, jushort, val);
      }
      break;
    case T_SHORT   :
      {
        jshort   val = POP();
        jint     idx = POP();
        address  ref = OBJ_POP();
        BOUNDS_CHECK(ref, idx);
        SET_ARRAY_ELEMENT(ref, idx, jshort, val);
      }
      break;
    case T_INT     :
      {
        jint    val = POP();
        jint    idx = POP();
        address ref = OBJ_POP();
        BOUNDS_CHECK(ref, idx);
        SET_ARRAY_ELEMENT(ref, idx, jint, val);
      }
      break;
    case T_OBJECT  :
      {
        address val = OBJ_POP();
        jint    idx = POP();
        address ref = OBJ_POP();
        BOUNDS_CHECK(ref, idx);
        if (!type_check(ref, val, idx)) return false;

        write_barrier(ref + Array::base_offset() + idx * sizeof(jint));
        SET_ARRAY_ELEMENT(ref, idx, address, val);
      }
      break;
    case T_LONG    :
      {
        jlong   val  = LONG_POP();
        jint    idx  = POP();
        address ref  = OBJ_POP();
        BOUNDS_CHECK(ref, idx);
        long_to_addr(ref + Array::base_offset() + idx*8, val);
      }
      break;
    case T_FLOAT   :
      {
        jfloat  val  = FLOAT_POP();
        jint    idx  = POP();
        address ref  = OBJ_POP();
        BOUNDS_CHECK(ref, idx);
        SET_ARRAY_ELEMENT(ref, idx, jfloat, val);
      }
      break;
    case T_DOUBLE  :
      {
        jdouble val  = DOUBLE_POP();
        jint    idx  = POP();
        address ref  = OBJ_POP();
        BOUNDS_CHECK(ref, idx);
        double_to_addr(ref + Array::base_offset() + idx*8, val);
      }
      break;
    default        :
      SHOULD_NOT_REACH_HERE();
    }

    return true;
  }

  static inline void iload(int n) {
    PUSH(GET_LOCAL(n));
  }

  static inline void lload(int n) {
    jlong_accessor acc;
    acc.words[1] = GET_LOCAL_LOW(n);
    acc.words[0] = GET_LOCAL_HIGH(n);
    LONG_PUSH(acc.long_value);
  }

  static inline void fload(int n) {
    PUSH(GET_LOCAL(n));
  }

  static inline void aload(int n) {
    PUSH(GET_LOCAL(n));
  }

  static inline void dload(int n) {
    PUSH(GET_LOCAL_LOW(n));
    PUSH(GET_LOCAL_HIGH(n));
  }

  static inline void istore(int n) {
    SET_LOCAL(n, POP());
  }

  static inline void lstore(int n) {
    jlong_accessor acc;
    acc.long_value = LONG_POP();

    SET_LOCAL_LOW(n,  acc.words[1]);
    SET_LOCAL_HIGH(n, acc.words[0]);
  }

  static inline void fstore(int n) {
    SET_LOCAL(n, POP());
  }

  static inline void astore(int n) {
    SET_LOCAL(n, POP());
  }

  static inline void dstore(int n) {
    jdouble_accessor acc;
    acc.double_value = DOUBLE_POP();

    SET_LOCAL_LOW(n,  acc.words[1]);
    SET_LOCAL_HIGH(n, acc.words[0]);
  }

  static inline void branch(bool cond) {
    if (cond) {
      g_jpc += GET_SIGNED_SHORT(0);
      // only check ticks on taken branches
      check_timer_tick();
    } else {
      ADVANCE(3);
    }
  }

  static address get_static_field_offset() {
    address cpool = GET_FRAME(cpool);
    jushort idx = GET_SHORT(0);
    jushort class_id = first_ushort_from_cpool(cpool, idx);
#if ENABLE_ISOLATES
    address obj = get_mirror_by_id(class_id);
    if (obj == get_cib_marker()) {
      address klass = get_class_by_id(class_id);
      if (interpreter_call_vm_1((address)task_barrier, T_OBJECT, (jint)klass)) {
        return NULL;
      }
      obj = (address)GET_THREAD_INT(obj_value);
      cpool = GET_FRAME(cpool);
    }
#else
    address obj = get_class_by_id(class_id);
    if (!is_initialized_class(obj)) {
      if (interpreter_call_vm_1((address)initialize_class, T_VOID, (jint)obj)) {
        return NULL;
      }
      GUARANTEE(cpool == GET_FRAME(cpool), "No GC should have happened");
    }
#endif
    return obj + second_ushort_from_cpool(cpool, idx); // offset
  }

  static void return_internal(BasicType type) {
    basic_value rv;

    if (unlock_activation()) {
      return;
    }

    switch(type) {
    case T_OBJECT :
    case T_INT    :
    case T_FLOAT  :
      rv.int_val =  int_from_sp(0);
      break;
    case T_LONG   :
      rv.long_val = jlong_from_low_high(int_from_sp(0), int_from_sp(1));
      break;
    case T_DOUBLE :
      rv.double_val = jdouble_from_low_high(int_from_sp(0), int_from_sp(1));
      break;
    case T_VOID   :
      break;
    default:
      SHOULD_NOT_REACH_HERE();
    }

    g_jlocals = GET_FRAME(locals_pointer);

    // go to previous frame
    g_jfp = GET_FRAME(caller_fp);

    g_jsp = g_jlocals - JavaFrame::arg_offset_from_sp(-1);

    method_transition();

    // we returned to EntryFrame
    if (GET_FRAME(return_address) == (address)EntryFrame::FakeReturnAddress) {
      address pending_entry = GET_ENTRY_FRAME(pending_activation);
      if (pending_entry != NULL) {
        jint entry = GET_ENTRY(pending_entry, next);
        jint len = GET_ENTRY(pending_entry, length);
        // save next entry
        SET_ENTRY_FRAME(pending_activation, entry);

        address header = pending_entry  + EntryActivationDesc::header_size();
        // put values on stack, note skipping of tags
        for (int i = 0; i < len; i++) {
          PUSH(*((jint*)header + 2 * i));
        }
        // find Java method
        address method = (address)GET_ENTRY(pending_entry, method);
        // invoke it
        invoke_java_method(method, 0);
        return;
      }

      // restore pending exception
      _current_pending_exception =
        (OopDesc*)GET_ENTRY_FRAME(pending_exception);

      SET_THREAD_INT(obj_value, (jint)GET_ENTRY_FRAME(stored_obj_value));
      // IMPL_NOTE: write_barrier((address)_current_thread + Thread::obj_value_offset());
      SET_THREAD_INT(int1_value, (jint)GET_ENTRY_FRAME(stored_int_value1));
      SET_THREAD_INT(int2_value, (jint)GET_ENTRY_FRAME(stored_int_value2));

      func_t stored_return_point = (func_t)GET_ENTRY_FRAME(real_return_address);

      // Pop the entry frame off the stack
      address old_jsp = GET_ENTRY_FRAME(stored_last_sp);
      address old_jfp = GET_ENTRY_FRAME(stored_last_fp);
      g_jsp = g_jfp +
        EntryFrame::empty_stack_offset() + EntryFrame::frame_desc_size();
      g_jfp = old_jfp;
      SET_THREAD_INT(last_java_sp, (jint)old_jsp);
      SET_THREAD_INT(last_java_fp, (jint)old_jfp);

      if (g_jfp == NULL) {
        BREAK_INTERPRETER_LOOP(JMP_THREAD_EXIT);
      }

      g_jpc = GET_FRAME(bcp_store);
      g_jlocals = GET_FRAME(locals_pointer);

      if (CURRENT_HAS_PENDING_EXCEPTION) {
        OopDesc* exception =_current_pending_exception;
        _current_pending_exception = NULL;
        shared_call_vm_internal(NULL, NULL, T_ILLEGAL, 1, (jint)exception);
        return;
      }

      if (stored_return_point != NULL) {
        stored_return_point();
      }
      return;
    }
    // restore previous JPC and locals
    g_jpc = GET_FRAME(bcp_store) + (int)(GET_FRAME(return_advance));
    g_jlocals = GET_FRAME(locals_pointer);

    switch(type) {
    case T_INT    : PUSH(rv.int_val);              break;
    case T_LONG   : LONG_PUSH(rv.long_val);        break;
    case T_FLOAT  : FLOAT_PUSH(rv.float_val);      break;
    case T_DOUBLE : DOUBLE_PUSH(rv.double_val);    break;
    case T_OBJECT : OBJ_PUSH(rv.object_val);       break;
    case T_VOID   : break;
    }
  }

  static void fast_ldc(BasicType type, bool wide) {
    jushort idx = wide ? GET_SHORT(0) : GET_BYTE(0);
    // Get constant pool of current method
    address cpool = GET_FRAME(cpool);

    // Push value on the stack
    switch (type) {
    case T_INT    :
    case T_FLOAT  :
    case T_OBJECT :
      {
        jint val = get_from_cpool(cpool, idx);
        PUSH(val);
      }
      break;
    case T_LONG   :
      {
        jlong val = long_from_addr(cpool + idx * 4);
        LONG_PUSH(val);
      }
      break;
    case T_DOUBLE:
      {
        jdouble val = double_from_addr(cpool + idx * 4);
        DOUBLE_PUSH(val);
      }
      break;
    default       :
      SHOULD_NOT_REACH_HERE();
    }

    ADVANCE(wide ? 3 : 2);
  }

  static void fast_invoke_internal(bool has_fixed_target_method,
                                   bool must_do_null_check, int invoker_size) {
    address method = NULL;
    // Get constant pool index
    jushort index = GET_SHORT(0);
    // Get constant pool of current method
    address cpool = GET_FRAME(cpool);

    if (has_fixed_target_method) {
      // Get method from resolved constant pool entry
      method = (address)get_from_cpool(cpool, index);
      if (must_do_null_check) {
        // Get the number of parameters from the method
        jushort num_params = get_num_params(method);
        // Get receiver object
        address receiver = OBJ_PEEK(num_params - 1);
        NULL_CHECK(receiver);
      } else {
        // Static method, can lead to class initialization
        jushort class_id = get_holder_id(method);
#if ENABLE_ISOLATES
        address mirror = get_mirror_by_id(class_id);
        if (mirror == get_cib_marker()) {
          address klass = get_class_by_id(class_id);
          if (interpreter_call_vm_1((address)task_barrier, T_OBJECT, (jint)klass)) {
            return;
          }
          cpool = GET_FRAME(cpool);
          method = (address)get_from_cpool(cpool, index);
        }
#else
        address klass = get_class_by_id(class_id);
        if (!is_initialized_class(klass)) {
          if (interpreter_call_vm_1((address)initialize_class, T_VOID, (jint)klass)) {
            return;
          }
          GUARANTEE(cpool == GET_FRAME(cpool), "No GC should have happened");
        }
#endif
      }
    } else {
      // Get vtable index from constant pool entry
      jushort vindex = first_ushort_from_cpool(cpool, index);
      // Get the class id from constant pool entry
      jushort klazz_id = second_ushort_from_cpool(cpool, index);
      // Get class by its id
      address klazz = get_class_by_id(klazz_id);
      // Get the ClassInfo
      address ci = *(address*)(klazz + JavaClass::class_info_offset());
      // Get method from vtable of the ClassInfo
      method = get_method_from_ci(ci, vindex);
      // Get the number of parameters from method
      jushort num_params = get_num_params(method);
       // Get receiver object and perform the null check
      address receiver = OBJ_PEEK(num_params - 1);
      NULL_CHECK(receiver);

      // Get prototype of receiver object
      receiver = *(address*)receiver;    // java near
      // Get method from vtable of the class
      method = get_method_from_vtable(receiver, vindex);
      GUARANTEE(method != NULL, "must be in the vtable");
    }

    // Call method
    invoke_java_method(method, invoker_size);
  }

  /* bytecodes implementation follows */

#define START_BYTECODES
#define END_BYTECODES
#define BYTECODE_IMPL_NO_STEP(x) static void bc_impl_##x() {
#if ENABLE_JAVA_DEBUGGER
#define BYTECODE_IMPL(x) static void bc_impl_##x() {            \
  if (_debugger_active & DEBUGGER_STEPPING) {                   \
    interpreter_call_vm((address)&handle_single_step, T_VOID);  \
  }
#else
#define BYTECODE_IMPL(x) static void bc_impl_##x() {
#endif
#define BYTECODE_IMPL_END }

  START_BYTECODES

  BYTECODE_IMPL(nop)
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aconst_null)
    PUSH(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iconst_m1)
    PUSH(-1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iconst_0)
    PUSH(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iconst_1)
    PUSH(1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iconst_2)
    PUSH(2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iconst_3)
    PUSH(3);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iconst_4)
    PUSH(4);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iconst_5)
    PUSH(5);
    ADVANCE(1);
   BYTECODE_IMPL_END

  BYTECODE_IMPL(lconst_0)
    LONG_PUSH(0);
    ADVANCE(1);
   BYTECODE_IMPL_END

  BYTECODE_IMPL(lconst_1)
    LONG_PUSH(1);
    ADVANCE(1);
   BYTECODE_IMPL_END

  BYTECODE_IMPL(fconst_0)
    PUSH(0);
    ADVANCE(1);
   BYTECODE_IMPL_END

  BYTECODE_IMPL(fconst_1)
    PUSH(1065353216);
    ADVANCE(1);
   BYTECODE_IMPL_END

  BYTECODE_IMPL(fconst_2)
    PUSH(1073741824);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dconst_0)
    DOUBLE_PUSH(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dconst_1)
#if HARDWARE_LITTLE_ENDIAN
    PUSH(1072693248);
    PUSH(0);
#else
    PUSH(0);
    PUSH(1072693248);
#endif
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(bipush)
    PUSH(GET_SIGNED_BYTE(0));
    ADVANCE(2);
   BYTECODE_IMPL_END

  BYTECODE_IMPL(sipush)
    PUSH(GET_SIGNED_SHORT(0));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(ldc)
    interpreter_call_vm_redo((address)&quicken, T_INT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(ldc_w)
    interpreter_call_vm_redo((address)&quicken, T_INT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(ldc2_w)
    interpreter_call_vm_redo((address)&quicken, T_INT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iload)
    iload(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iload_wide)
    iload(GET_SHORT(0));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lload)
    lload(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lload_wide)
    lload(GET_SHORT(0));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fload)
    fload(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fload_wide)
    fload(GET_SHORT(0));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dload)
    dload(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dload_wide)
    dload(GET_SHORT(0));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aload)
    aload(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aload_wide)
    aload(GET_SHORT(0));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iload_0)
    iload(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iload_1)
    iload(1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iload_2)
    iload(2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iload_3)
    iload(3);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lload_0)
    lload(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lload_1)
    lload(1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lload_2)
    lload(2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lload_3)
    lload(3);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fload_0)
    fload(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fload_1)
    fload(1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fload_2)
    fload(2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fload_3)
    fload(3);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dload_0)
    dload(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dload_1)
    dload(1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dload_2)
    dload(2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dload_3)
    dload(3);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aload_0)
    aload(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aload_1)
    aload(1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aload_2)
    aload(2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aload_3)
    aload(3);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iaload)
    if (array_load(T_INT)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(laload)
    if (array_load(T_LONG)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(faload)
    if (array_load(T_FLOAT)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(daload)
    if (array_load(T_DOUBLE)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aaload)
    if (array_load(T_OBJECT)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(baload)
    if (array_load(T_BYTE)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(caload)
    if (array_load(T_CHAR)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(saload)
    if (array_load(T_SHORT)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(istore)
    istore(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(istore_wide)
    istore(GET_SHORT(0));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lstore)
    lstore(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lstore_wide)
    lstore(GET_SHORT(0));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fstore)
    fstore(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fstore_wide)
    fstore(GET_SHORT(0));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dstore)
    dstore(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dstore_wide)
    dstore(GET_SHORT(0));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(astore)
    astore(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(astore_wide)
    astore(GET_SHORT(0));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(istore_0)
    istore(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(istore_1)
    istore(1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(istore_2)
    istore(2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(istore_3)
    istore(3);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lstore_0)
    lstore(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lstore_1)
    lstore(1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lstore_2)
    lstore(2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lstore_3)
    lstore(3);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fstore_0)
    fstore(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fstore_1)
    fstore(1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fstore_2)
    fstore(2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fstore_3)
    fstore(3);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dstore_0)
    dstore(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dstore_1)
    dstore(1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dstore_2)
    dstore(2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dstore_3)
    dstore(3);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(astore_0)
    astore(0);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(astore_1)
    astore(1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(astore_2)
    astore(2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(astore_3)
    astore(3);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iastore)
    if (array_store(T_INT)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lastore)
    if (array_store(T_LONG)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fastore)
    if (array_store(T_FLOAT)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dastore)
    if (array_store(T_DOUBLE)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aastore)
    if (array_store(T_OBJECT)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(bastore)
    if (array_store(T_BYTE)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(castore)
    if (array_store(T_CHAR)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(sastore)
    if (array_store(T_SHORT)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(pop)
    POP();
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(pop2)
    POP();
    POP();
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dup)
    jint v = PEEK(0);
    PUSH(v);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dup_x1)
    jint val1 = POP();
    jint val2 = POP();
    PUSH(val1);
    PUSH(val2);
    PUSH(val1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dup_x2)
    jint val1 = POP();
    jint val2 = POP();
    jint val3 = POP();
    PUSH(val1);
    PUSH(val3);
    PUSH(val2);
    PUSH(val1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dup2)
    jint val1 = POP();
    jint val2 = POP();
    PUSH(val2);
    PUSH(val1);
    PUSH(val2);
    PUSH(val1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dup2_x1)
    jint val1 = POP();
    jint val2 = POP();
    jint val3 = POP();
    PUSH(val2);
    PUSH(val1);
    PUSH(val3);
    PUSH(val2);
    PUSH(val1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dup2_x2)
    jint val1 = POP();
    jint val2 = POP();
    jint val3 = POP();
    jint val4 = POP();
    PUSH(val2);
    PUSH(val1);
    PUSH(val4);
    PUSH(val3);
    PUSH(val2);
    PUSH(val1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(swap)
    jint val1 = POP();
    jint val2 = POP();
    PUSH(val1);
    PUSH(val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iadd)
    jint val1 = POP();
    jint val2 = POP();
    PUSH(val1 + val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ladd)
    jlong val1 = LONG_POP();
    jlong val2 = LONG_POP();
    LONG_PUSH(val1 + val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

#if ENABLE_FLOAT
  BYTECODE_IMPL(fadd)
    jfloat val1 = FLOAT_POP();
    jfloat val2 = FLOAT_POP();
    FLOAT_PUSH(jvm_fadd(val1, val2));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dadd)
    jdouble val1 = DOUBLE_POP();
    jdouble val2 = DOUBLE_POP();
    DOUBLE_PUSH(jvm_dadd(val1, val2));
    ADVANCE(1);
  BYTECODE_IMPL_END
#endif

  BYTECODE_IMPL(isub)
    jint val2 = POP();
    jint val1 = POP();
    PUSH(val1 - val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lsub)
    jlong val2 = LONG_POP();
    jlong val1 = LONG_POP();
    LONG_PUSH(val1 - val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

#if ENABLE_FLOAT
  BYTECODE_IMPL(fsub)
    jfloat val2 = FLOAT_POP();
    jfloat val1 = FLOAT_POP();
    FLOAT_PUSH(jvm_fsub(val1, val2));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dsub)
    jdouble val2 = DOUBLE_POP();
    jdouble val1 = DOUBLE_POP();
    DOUBLE_PUSH(jvm_dsub(val1, val2));
    ADVANCE(1);
  BYTECODE_IMPL_END
#endif

  BYTECODE_IMPL(imul)
    jint val2 = POP();
    jint val1 = POP();
    PUSH(val1 * val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lmul)
    jlong val2 = LONG_POP();
    jlong val1 = LONG_POP();
    LONG_PUSH(val1 * val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fmul)
    jfloat val2 = FLOAT_POP();
    jfloat val1 = FLOAT_POP();
    FLOAT_PUSH(jvm_fmul(val1, val2));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dmul)
    jdouble val1 = DOUBLE_POP();
    jdouble val2 = DOUBLE_POP();
    DOUBLE_PUSH(jvm_dmul(val1, val2));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(idiv)
    jint val2 = POP();
    jint val1 = POP();
    if (val2 == 0) {
      interpreter_throw_ArithmeticException();
      return;
    }
    jint rv = val1;
    if (val1 != 0x80000000 || val2 != -1) {
      rv  /=  val2;
    }
    PUSH(rv);

    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ldiv)
    jlong val2 = LONG_POP();
    jlong val1 = LONG_POP();
    if (val2 == 0) {
      interpreter_throw_ArithmeticException();
      return;
    }
    LONG_PUSH(val1 / val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fdiv)
    jfloat val2 = FLOAT_POP();
    jfloat val1 = FLOAT_POP();
    FLOAT_PUSH(jvm_fdiv(val1, val2));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ddiv)
    jdouble val2 = DOUBLE_POP();
    jdouble val1 = DOUBLE_POP();
    DOUBLE_PUSH(jvm_ddiv(val1, val2));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(irem)
    jint val2 = POP();
    jint val1 = POP();
    if (val2 == 0) {
      interpreter_throw_ArithmeticException();
      return;
    }
    if (val1 == 0x80000000 && val2 == -1) {
      PUSH(val1 % 1);
    } else {
      PUSH(val1 % val2);
    }
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lrem)
    jlong val2 = LONG_POP();
    jlong val1 = LONG_POP();
    if (val2 == 0) {
      interpreter_throw_ArithmeticException();
      return;
    }
    LONG_PUSH(val1 % val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

#if ENABLE_FLOAT
  BYTECODE_IMPL(frem)
    jfloat val2 = FLOAT_POP();
    jfloat val1 = FLOAT_POP();
    FLOAT_PUSH(jvm_frem(val1, val2));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(drem)
    jdouble val2 = DOUBLE_POP();
    jdouble val1 = DOUBLE_POP();
    DOUBLE_PUSH(jvm_drem(val1, val2));
    ADVANCE(1);
  BYTECODE_IMPL_END
#endif

  BYTECODE_IMPL(ineg)
    jint val = POP();
    PUSH(-val);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lneg)
    jlong val = LONG_POP();
    LONG_PUSH(-val);
    ADVANCE(1);
  BYTECODE_IMPL_END

#if ENABLE_FLOAT
  BYTECODE_IMPL(fneg)
    jfloat val = FLOAT_POP();
    FLOAT_PUSH(jvm_fmul(val, -1.0f));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dneg)
    jdouble val = DOUBLE_POP();
    DOUBLE_PUSH(jvm_dneg(val));
    ADVANCE(1);
  BYTECODE_IMPL_END
#endif

  BYTECODE_IMPL(ishl)
    jint val2 = POP();
    jint val1 = POP();
    PUSH(val1 << (val2 & 0x1f));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lshl)
    jint  val2 = POP();
    jlong val1 = LONG_POP();
    LONG_PUSH(val1 << (val2 & 0x3f));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ishr)
    jint val2 = POP();
    jint val1 = POP();
    PUSH(val1 >> (val2 & 0x1f));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lshr)
    jint  val2 = POP();
    jlong val1 = LONG_POP();
    LONG_PUSH(val1 >> (val2 & 0x3f));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iushr)
    jint val2 = POP() & 0x1f;
    jint val1 = POP();
    val1 = (juint)val1 >> val2;
    PUSH(val1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lushr)
    jint  val2 = POP() & 0x3f;
    jlong val1 = LONG_POP();
    val1 = (julong)val1 >> val2;
    LONG_PUSH(val1);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iand)
    jint val2 = POP();
    jint val1 = POP();
    PUSH(val1 & val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(land)
    jlong val2 = LONG_POP();
    jlong val1 = LONG_POP();
    LONG_PUSH(val1 & val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ior)
    jint val2 = POP();
    jint val1 = POP();
    PUSH(val1 | val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lor)
    jlong val2 = LONG_POP();
    jlong val1 = LONG_POP();
    LONG_PUSH(val1 | val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ixor)
    jint val2 = POP();
    jint val1 = POP();
    PUSH(val1 ^ val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lxor)
    jlong val2 = LONG_POP();
    jlong val1 = LONG_POP();
    LONG_PUSH(val1 ^ val2);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iinc)
    jint n   = GET_BYTE(0);
    jint inc = GET_SIGNED_BYTE(1);
    jint val = GET_LOCAL(n);
    SET_LOCAL(n, val + inc);
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iinc_wide)
    jint   n   = GET_SHORT(0);
    jint   inc = GET_SIGNED_SHORT(2);
    jint   val = GET_LOCAL(n);
    SET_LOCAL(n, val + inc);
    ADVANCE(5);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(i2l)
    jlong val = POP();
    LONG_PUSH(val);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(i2f)
    FLOAT_PUSH(jvm_i2f(POP()));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(i2d)
    DOUBLE_PUSH(jvm_i2d(POP()));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(l2i)
    jint val = (jint)LONG_POP();
    PUSH(val);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(l2f)
    FLOAT_PUSH(jvm_l2f(LONG_POP()));
    ADVANCE(1);
  BYTECODE_IMPL_END

#if ENABLE_FLOAT
  BYTECODE_IMPL(l2d)
    DOUBLE_PUSH(jvm_l2d(LONG_POP()));
    ADVANCE(1);
  BYTECODE_IMPL_END
#endif

  BYTECODE_IMPL(f2i)
    PUSH(jvm_f2i(FLOAT_POP()));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(f2l)
    LONG_PUSH(jvm_f2l(FLOAT_POP()));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(f2d)
    DOUBLE_PUSH(jvm_f2d(FLOAT_POP()));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(d2i)
    PUSH(jvm_d2i(DOUBLE_POP()));
    ADVANCE(1);
  BYTECODE_IMPL_END

#if ENABLE_FLOAT
  BYTECODE_IMPL(d2l)
    LONG_PUSH(jvm_d2l(DOUBLE_POP()));
    ADVANCE(1);
  BYTECODE_IMPL_END
#endif

  BYTECODE_IMPL(d2f)
    FLOAT_PUSH(jvm_d2f(DOUBLE_POP()));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(i2b)
    jbyte val = (jbyte)POP();
    PUSH(val);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(i2c)
    jchar val = (jchar)POP();
    PUSH(val);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(i2s)
    jshort val = (jshort)POP();
    PUSH(val);
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lcmp)
    jlong val2 = LONG_POP();
    jlong val1 = LONG_POP();

    jint rv = val1 < val2 ? -1 : val1 == val2 ? 0 : 1;
    PUSH(rv);

    ADVANCE(1);
  BYTECODE_IMPL_END

#if ENABLE_FLOAT
  BYTECODE_IMPL(fcmpl)
    jfloat rval = FLOAT_POP();
    jfloat lval = FLOAT_POP();

    PUSH(jvm_fcmpl(lval, rval));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fcmpg)
    jfloat rval = FLOAT_POP();
    jfloat lval = FLOAT_POP();

    PUSH(jvm_fcmpg(lval, rval));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dcmpl)
    jdouble rval = DOUBLE_POP();
    jdouble lval = DOUBLE_POP();

    PUSH(jvm_dcmpl(lval, rval));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dcmpg)
    jdouble rval = DOUBLE_POP();
    jdouble lval = DOUBLE_POP();

    PUSH(jvm_dcmpg(lval, rval));
    ADVANCE(1);
  BYTECODE_IMPL_END
#endif

  BYTECODE_IMPL(ifeq)
    jint val = POP();
    branch(val == 0);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ifne)
    jint val = POP();
    branch(val != 0);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(iflt)
    jint val = POP();
    branch(val < 0);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ifge)
    jint val = POP();
    branch(val >= 0);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ifgt)
    jint val = POP();
    branch(val > 0);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ifle)
    jint val = POP();
    branch(val <= 0);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(if_icmpeq)
    jint val2 = POP();
    jint val1 = POP();
    branch(val1 == val2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(if_icmpne)
    jint val2 = POP();
    jint val1 = POP();
    branch(val1 != val2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(if_icmplt)
    jint val2 = POP();
    jint val1 = POP();
    branch(val1 < val2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(if_icmpge)
    jint val2 = POP();
    jint val1 = POP();
    branch(val1 >= val2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(if_icmpgt)
    jint val2 = POP();
    jint val1 = POP();
    branch(val1 > val2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(if_icmple)
    jint val2 = POP();
    jint val1 = POP();
    branch(val1 <= val2);
   BYTECODE_IMPL_END

  BYTECODE_IMPL(if_acmpeq)
    jint val2 = POP();
    jint val1 = POP();
    branch(val1 == val2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(if_acmpne)
    jint val2 = POP();
    jint val1 = POP();
    branch(val1 != val2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(goto)
    branch(true);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(tableswitch)
    jint index = POP();
    // method entry point is 4-byte aligned, jpc points to
    // tableswitch bytecode
    address aligned_jpc = (address)((jint)(g_jpc + 1 + 3) & ~3);
    // get default target
    jint    target = int_from_addr(aligned_jpc);
    jint    low    = int_from_addr(aligned_jpc + 4);
    jint    high   = int_from_addr(aligned_jpc + 8);
    if (index >= low && index <= high) {
      target = int_from_addr(aligned_jpc + 12 + (index - low) * 4);
    }
    g_jpc += target;
    check_timer_tick();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lookupswitch)
    jint key = POP();
    // method entry point is 4-byte aligned, jpc points to
    // lookupswitch bytecode
    address aligned_jpc = (address)((jint)(g_jpc + 1 + 3) & ~3);
    // get default target
    jint    target = int_from_addr(aligned_jpc);
    jint    npairs = int_from_addr(aligned_jpc + 4);

    while (npairs-- > 0) {
      aligned_jpc += 8;
      if (int_from_addr(aligned_jpc) == key) {
        target = int_from_addr(aligned_jpc + 4);
        break;
      }
    }
    // branch to target offset
    g_jpc += target;
    check_timer_tick();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ireturn)
    return_internal(T_INT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lreturn)
    return_internal(T_LONG);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(freturn)
    return_internal(T_FLOAT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dreturn)
    return_internal(T_DOUBLE);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(areturn)
    return_internal(T_OBJECT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(return)
    return_internal(T_VOID);
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(getstatic)
    interpreter_call_vm_dispatch((address)&getstatic, T_INT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(putstatic)
    interpreter_call_vm_dispatch((address)&putstatic, T_INT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(getfield)
    interpreter_call_vm_redo((address)&getfield, T_INT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(putfield)
    interpreter_call_vm_redo((address)&putfield, T_INT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(invokevirtual)
    interpreter_call_vm_redo((address)&quicken, T_INT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(invokespecial)
    interpreter_call_vm_redo((address)&quicken, T_INT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(invokestatic)
    interpreter_call_vm_dispatch((address)&quicken_invokestatic, T_INT);
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(invokeinterface)
    interpreter_call_vm_redo((address)&quicken, T_INT);
  BYTECODE_IMPL_END

  static void new_return_point() {
    PUSH(GET_THREAD_INT(obj_value));
    ADVANCE(3);
  }

  BYTECODE_IMPL(new)
    shared_call_vm_internal((address)&newobject, (address)&new_return_point,
                            T_OBJECT, 0);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(newarray)
    jint type = GET_BYTE(0);
    jint len = POP();

    // actually newarray is different on x86 and everything else,
    // as x86 C code reads it from Java stack, and ARM, SH and C does it
    // in interpreter
    if (!interpreter_call_vm_2((address)&_newarray, T_ARRAY, type, len)) {
      // put returned value on stack
      PUSH(GET_THREAD_INT(obj_value));
      ADVANCE(2);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(anewarray)
    if (!interpreter_call_vm((address)&anewarray, T_ARRAY)) {
      // remove length from stack
      POP();
      // put returned value on stack
      PUSH(GET_THREAD_INT(obj_value));
      ADVANCE(3);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(arraylength)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(GET_ARRAY_LENGTH(obj));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(athrow)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    shared_call_vm_internal(NULL, NULL, T_ILLEGAL, 1, obj);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(checkcast)
    if (!interpreter_call_vm((address)&checkcast, T_VOID)) {
      // checkcast can throw an exception
      ADVANCE(3);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(instanceof)
    // instanceof can throw an exception, like in
    // vm.instr.instanceofX.instanceof012.instanceof01201m1_1.instanceof01201m1
    // when we're using invalid class index in Java file
    if (interpreter_call_vm((address)&instanceof, T_INT)) {
      return;
    }

    // remove object from stack
    POP();

    // put returned value on stack
    PUSH(GET_THREAD_INT(int1_value));

    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(monitorenter)
    // Get the object to lock from the stack
    address obj = OBJ_POP();
    NULL_CHECK(obj);

    // IMPL_NOTE: Increment the bytecode pointer before locking to make
    // asynchronous exceptions work???
    ADVANCE(1);
    monitor_enter_internal(obj);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(monitorexit)
    // Get the object to unlock from the stack
    address obj = OBJ_POP();
    NULL_CHECK(obj);

    if (!monitor_exit_internal(obj)) {
      ADVANCE(1);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(wide)
    ADVANCE(1);
    interpreter_dispatch_table[(int)*g_jpc + WIDE_OFFSET]();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(multianewarray)
    if (!interpreter_call_vm((address)&multianewarray, T_ARRAY)) {
      // remove parameters
      g_jsp += GET_BYTE(2) * BytesPerStackElement;
      // put returned value on stack
      PUSH(GET_THREAD_INT(obj_value));
      ADVANCE(4);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ifnull)
    jint val = POP();
    branch(val == 0);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(ifnonnull)
    jint val = POP();
    branch(val != 0);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(goto_w)
    jint offset = GET_INT(0);
    g_jpc += offset;
  BYTECODE_IMPL_END

  BYTECODE_IMPL_NO_STEP(breakpoint)
#if ENABLE_JAVA_DEBUGGER
    interpreter_call_vm((address)&handle_breakpoint, T_INT);
    interpreter_dispatch_table[GET_THREAD_INT(int1_value)]();
#else
    UNIMPL(breakpoint);
#endif
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_1_ldc)
    fast_ldc(T_INT, false);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_1_ldc_w)
    fast_ldc(T_INT, true);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_2_ldc_w)
    fast_ldc(T_LONG, true);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_1_putstatic)
    address addr = get_static_field_offset();
    if (addr != NULL) {
      *(jint*)addr = POP();
      ADVANCE(3);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_2_putstatic)
    address addr = get_static_field_offset();
    if (addr != NULL) {
      long_to_addr(addr, LONG_POP());
      ADVANCE(3);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_a_putstatic)
    address addr = get_static_field_offset();
    if (addr != NULL) {
      *(address*)addr = OBJ_POP();
      write_barrier(addr);
      ADVANCE(3);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_1_getstatic)
    address addr = get_static_field_offset();
    if (addr != NULL) {
      PUSH(*(jint*)addr);
      ADVANCE(3);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_2_getstatic)
    address addr = get_static_field_offset();
    if (addr != NULL) {
      LONG_PUSH(long_from_addr(addr));
      ADVANCE(3);
    }
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_init_1_putstatic)
    bc_impl_fast_1_putstatic();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_init_2_putstatic)
    bc_impl_fast_2_putstatic();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_init_a_putstatic)
    bc_impl_fast_a_putstatic();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_init_1_getstatic)
    bc_impl_fast_1_getstatic();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_init_2_getstatic)
    bc_impl_fast_2_getstatic();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_bputfield)
    jbyte value = POP();
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    *(jbyte*)(obj + GET_SHORT_NATIVE(0)) = value;
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_sputfield)
    jshort value = POP();
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    *(jshort*)(obj + GET_SHORT_NATIVE(0)) = value;
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_iputfield)
    jint value = POP();
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    *(jint*)(obj + GET_SHORT_NATIVE(0) * 4) = value;
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_lputfield)
    jlong value = LONG_POP();
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    long_to_addr(obj + GET_SHORT_NATIVE(0) * 4, value);
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_fputfield)
    jfloat value = FLOAT_POP();
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    *(jfloat*)(obj + GET_SHORT_NATIVE(0) * 4) = value;
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_dputfield)
    jdouble value = DOUBLE_POP();
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    double_to_addr(obj + GET_SHORT_NATIVE(0) * 4, value);
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_aputfield)
    address value = OBJ_POP();
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    obj += GET_SHORT_NATIVE(0) * 4;
    *(address*)obj = value;
    write_barrier(obj);
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_bgetfield)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(*(jbyte*)(obj + GET_SHORT_NATIVE(0)));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_sgetfield)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(*(jshort*)(obj + GET_SHORT_NATIVE(0)));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_cgetfield)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(*(jushort*)(obj + GET_SHORT_NATIVE(0)));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_igetfield)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(*(jint*)(obj + GET_SHORT_NATIVE(0) * 4));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_lgetfield)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    LONG_PUSH(long_from_addr(obj + GET_SHORT_NATIVE(0) * 4));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_fgetfield)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    FLOAT_PUSH(*(jfloat*)(obj + GET_SHORT_NATIVE(0) * 4));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_dgetfield)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    DOUBLE_PUSH(double_from_addr(obj + GET_SHORT_NATIVE(0) * 4));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_agetfield)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    OBJ_PUSH(*(address*)(obj + GET_SHORT_NATIVE(0) * 4));
    ADVANCE(3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_invokevirtual)
    fast_invoke_internal(false, true, 3);
   BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_invokestatic)
    fast_invoke_internal(true, false, 3);
   BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_init_invokestatic)
    fast_invoke_internal(true, false, 3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_invokeinterface)
    // read arguments
    jushort index = GET_SHORT(0);
    jubyte  num_params = GET_BYTE(2);

    // Get constant pool of current method
    address cpool = GET_FRAME(cpool);

    // Get the method index and class id from resolved constant pool entry
    jushort method_index =  first_ushort_from_cpool(cpool, index);
    jushort class_id = second_ushort_from_cpool(cpool, index);

    // Get receiver object and perform the null check
    address receiver = OBJ_PEEK(num_params - 1);
    NULL_CHECK(receiver);

    // Get class of receiver object
    receiver = *(address*)receiver;
    receiver = *(address*)receiver;

    // Get the itable from the class of the receiver object
    // Get the ClassInfo
    address ci = *(address*)(receiver + JavaClass::class_info_offset());
    // get length of vtable and itable
    jushort vlength = *(jushort*)(ci + ClassInfo::vtable_length_offset());
    jint ilength = *(jushort*)(ci + ClassInfo::itable_length_offset());
    // start of itable
    address itable = ci + ClassInfoDesc::header_size() + vlength * 4;

    // Lookup interface method table by linear search
    for (itable = ci + ClassInfoDesc::header_size() + vlength*4; ; ilength--){
      // IMPL_NOTE: or < 0
      if (ilength <= 0) {
       interpreter_throw_IncompatibleClassChangeError();
       return;
      }

      // found
      if (class_id == int_from_addr(itable)) {
        break;
      }

      itable += 8;
    }

    // method table of the receiver class
    address table = int_from_addr(itable + 4) + ci;
    address method = *(address*)(table + method_index * 4);
    invoke_java_method(method, 5);
  BYTECODE_IMPL_END

  static void bc_impl_fast_invokenative();

  static void invokenative_return_point() {
    if (GET_THREAD_INT(async_redo)) {
      // Clear Thread.async_redo so that we won't loop indefinitely
      SET_THREAD_INT(async_redo, 0);
      bc_impl_fast_invokenative();
      return;
    }

    // Clear async_info (even if the methods was never redone)
    SET_THREAD_INT(async_info, 0);

    // Clear async_info (even if the methods was never redone)
    SET_THREAD_INT(async_info, 0);

    switch (GET_BYTE(0)) {
      case T_INT:
        PUSH(GET_THREAD_INT(int1_value));
        bc_impl_ireturn();
        break;

      case T_FLOAT:
        PUSH(GET_THREAD_INT(int1_value));
        bc_impl_freturn();
        break;

      case T_VOID:
        bc_impl_return();
        break;

      case T_LONG:
        PUSH(GET_THREAD_INT(int2_value));
        PUSH(GET_THREAD_INT(int1_value));
        bc_impl_lreturn();
        break;

      case T_DOUBLE:
        PUSH(GET_THREAD_INT(int2_value));
        PUSH(GET_THREAD_INT(int1_value));
        bc_impl_dreturn();
        break;

      case T_OBJECT:
        PUSH(GET_THREAD_INT(obj_value));
        bc_impl_areturn();
        break;

      default:
        SHOULD_NOT_REACH_HERE();
    }
  }

  BYTECODE_IMPL(fast_invokenative)
    address method = callee_method();
    // Point _kni_parameter_base to the first parameter
    address locals = g_jlocals;

    // Set space for fake parameter for static method (KNI-ism)
    if (get_access_flags(method) & JVM_ACC_STATIC) {
      locals += 4;
    }
    _kni_parameter_base = locals;

    // Get the native method pointer from the bytecode
    address native_ptr =
      *(address*)(g_jpc + Method::native_code_offset_from_bcp());

    method_transition();

    shared_call_vm_internal(native_ptr, (address)&invokenative_return_point,
                            (BasicType)GET_BYTE(0), 0);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_new)
    bc_impl_new();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_init_new)
    bc_impl_new();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_anewarray)
    bc_impl_anewarray();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_checkcast)
    bc_impl_checkcast();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_instanceof)
    bc_impl_instanceof();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_invokevirtual_final)
    fast_invoke_internal(true, true, 3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_invokespecial)
    // Get constant pool index
    jushort idx = GET_SHORT(0);
    // Get constant pool of current method
    address cpool = GET_FRAME(cpool);
    // Get vtable index from constant pool entry
    jushort vindex = first_ushort_from_cpool(cpool, idx);
    // Get the class id from constant pool entry
    jushort klazz_id = second_ushort_from_cpool(cpool, idx);
    // Get class by its id
    address klazz = get_class_by_id(klazz_id);
    // Get the ClassInfo
    address ci = *(address*)(klazz + JavaClass::class_info_offset());
    // Get method from vtable of the ClassInfo
    address method = get_method_from_ci(ci, vindex);
    // Get the number of parameters from method
    jushort num_params = get_num_params(method);
    // Get receiver object and perform the null check
    address receiver = OBJ_PEEK(num_params - 1);
    NULL_CHECK(receiver);

    invoke_java_method(method, 3);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_igetfield_1)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(*(jint*)(obj + GET_BYTE(0) * 4));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(fast_agetfield_1)
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    OBJ_PUSH(*(address*)(obj + GET_BYTE(0) * 4));
    ADVANCE(2);
  BYTECODE_IMPL_END

#if !ENABLE_CPU_VARIANT

  BYTECODE_IMPL(aload_0_fast_igetfield_1)
    aload(0);
    bc_impl_fast_igetfield_1();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aload_0_fast_igetfield_4)
    aload(0);
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(*(jint*)(obj + 4));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aload_0_fast_igetfield_8)
    aload(0);
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    PUSH(*(jint*)(obj + 8));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aload_0_fast_agetfield_1)
    aload(0);
    bc_impl_fast_agetfield_1();
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aload_0_fast_agetfield_4)
    aload(0);
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    OBJ_PUSH(*(address*)(obj + 4));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(aload_0_fast_agetfield_8)
    aload(0);
    address obj = OBJ_POP();
    NULL_CHECK(obj);
    OBJ_PUSH(*(address*)(obj + 8));
    ADVANCE(1);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(init_static_array)
    address ref = OBJ_PEEK(0);
    NULL_CHECK(ref);
    int size_factor = 1 << (int)GET_BYTE(0);
    int count = (jushort)GET_SHORT_NATIVE(1);
    int len = GET_ARRAY_LENGTH(ref);
    if (count < 0 || count > len) {
      interpreter_throw_ArrayIndexOutOfBoundsException();
      return;
    }
    jvm_memcpy(ref + Array::base_offset(), (g_jpc + 4), count * size_factor);
    ADVANCE(4 + size_factor * count);
  BYTECODE_IMPL_END

#elif ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND

  BYTECODE_IMPL(lload_safe)
    lload(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(lstore_safe)
    lstore(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dload_safe)
    dload(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

  BYTECODE_IMPL(dstore_safe)
    dstore(GET_BYTE(0));
    ADVANCE(2);
  BYTECODE_IMPL_END

#endif

  BYTECODE_IMPL(pop_and_npe_if_null)
    NULL_CHECK(OBJ_POP());
    ADVANCE(1);
  BYTECODE_IMPL_END

  END_BYTECODES

  void undef_bc() {
    tty->print_cr("Undefined bytecode hit: 0x%x at %x", *g_jpc, g_jpc);
    BREAKPOINT;
  }

} /* of extern "C" */


#define DEF_BC(name)               \
    interpreter_dispatch_table[Bytecodes::_##name] = &bc_impl_##name
#define DEF_BC_WIDE(name)          \
    interpreter_dispatch_table[Bytecodes::_##name + WIDE_OFFSET] = \
       &bc_impl_##name##_wide
#if ENABLE_FLOAT
#define DEF_BC_FLOAT(name)          \
    interpreter_dispatch_table[Bytecodes::_##name] = \
       &bc_impl_##name
#else
#define DEF_BC_FLOAT(name)
#endif

static void init_dispatch_table() {
  DEF_BC(nop);
  DEF_BC(aconst_null);
  DEF_BC(iconst_m1);
  DEF_BC(iconst_0);
  DEF_BC(iconst_1);
  DEF_BC(iconst_2);
  DEF_BC(iconst_3);
  DEF_BC(iconst_4);
  DEF_BC(iconst_5);
  DEF_BC(lconst_0);
  DEF_BC(lconst_1);
  DEF_BC(fconst_0);
  DEF_BC(fconst_1);
  DEF_BC(fconst_2);
  DEF_BC(dconst_0);
  DEF_BC(dconst_1);
  DEF_BC(bipush);
  DEF_BC(sipush);
  DEF_BC(ldc);
  DEF_BC(ldc_w);
  DEF_BC(ldc2_w);
  DEF_BC(iload);
  DEF_BC_WIDE(iload);
  DEF_BC(lload);
  DEF_BC_WIDE(lload);
  DEF_BC(fload);
  DEF_BC_WIDE(fload);
  DEF_BC(dload);
  DEF_BC_WIDE(dload);
  DEF_BC(aload);
  DEF_BC_WIDE(aload);
  DEF_BC(iload_0);
  DEF_BC(iload_1);
  DEF_BC(iload_2);
  DEF_BC(iload_3);
  DEF_BC(lload_0);
  DEF_BC(lload_1);
  DEF_BC(lload_2);
  DEF_BC(lload_3);
  DEF_BC(fload_0);
  DEF_BC(fload_1);
  DEF_BC(fload_2);
  DEF_BC(fload_3);
  DEF_BC(dload_0);
  DEF_BC(dload_1);
  DEF_BC(dload_2);
  DEF_BC(dload_3);
  DEF_BC(aload_0);
  DEF_BC(aload_1);
  DEF_BC(aload_2);
  DEF_BC(aload_3);
  DEF_BC(iaload);
  DEF_BC(laload);
  DEF_BC(faload);
  DEF_BC(daload);
  DEF_BC(aaload);
  DEF_BC(baload);
  DEF_BC(caload);
  DEF_BC(saload);
  DEF_BC(istore);
  DEF_BC_WIDE(istore);
  DEF_BC(lstore);
  DEF_BC_WIDE(lstore);
  DEF_BC(fstore);
  DEF_BC_WIDE(fstore);
  DEF_BC(dstore);
  DEF_BC_WIDE(dstore);
  DEF_BC(astore);
  DEF_BC_WIDE(astore);
  DEF_BC(istore_0);
  DEF_BC(istore_1);
  DEF_BC(istore_2);
  DEF_BC(istore_3);
  DEF_BC(lstore_0);
  DEF_BC(lstore_1);
  DEF_BC(lstore_2);
  DEF_BC(lstore_3);
  DEF_BC(fstore_0);
  DEF_BC(fstore_1);
  DEF_BC(fstore_2);
  DEF_BC(fstore_3);
  DEF_BC(dstore_0);
  DEF_BC(dstore_1);
  DEF_BC(dstore_2);
  DEF_BC(dstore_3);
  DEF_BC(astore_0);
  DEF_BC(astore_1);
  DEF_BC(astore_2);
  DEF_BC(astore_3);
  DEF_BC(iastore);
  DEF_BC(lastore);
  DEF_BC(fastore);
  DEF_BC(dastore);
  DEF_BC(aastore);
  DEF_BC(bastore);
  DEF_BC(castore);
  DEF_BC(sastore);
  DEF_BC(pop);
  DEF_BC(pop2);
  DEF_BC(dup);
  DEF_BC(dup_x1);
  DEF_BC(dup_x2);
  DEF_BC(dup2);
  DEF_BC(dup2_x1);
  DEF_BC(dup2_x2);
  DEF_BC(swap);
  DEF_BC(iadd);
  DEF_BC(ladd);
  DEF_BC_FLOAT(fadd);
  DEF_BC_FLOAT(dadd);
  DEF_BC(isub);
  DEF_BC(lsub);
  DEF_BC_FLOAT(fsub);
  DEF_BC_FLOAT(dsub);
  DEF_BC(imul);
  DEF_BC(lmul);
  DEF_BC(fmul);
  DEF_BC(dmul);
  DEF_BC(idiv);
  DEF_BC(ldiv);
  DEF_BC(fdiv);
  DEF_BC(ddiv);
  DEF_BC(irem);
  DEF_BC(lrem);
  DEF_BC_FLOAT(frem);
  DEF_BC_FLOAT(drem);
  DEF_BC(ineg);
  DEF_BC(lneg);
  DEF_BC_FLOAT(fneg);
  DEF_BC_FLOAT(dneg);
  DEF_BC(ishl);
  DEF_BC(lshl);
  DEF_BC(ishr);
  DEF_BC(lshr);
  DEF_BC(iushr);
  DEF_BC(lushr);
  DEF_BC(iand);
  DEF_BC(land);
  DEF_BC(ior);
  DEF_BC(lor);
  DEF_BC(ixor);
  DEF_BC(lxor);
  DEF_BC(iinc);
  DEF_BC_WIDE(iinc);
  DEF_BC(i2l);
  DEF_BC(i2f);
  DEF_BC(i2d);
  DEF_BC(l2i);
  DEF_BC(l2f);
  DEF_BC_FLOAT(l2d);
  DEF_BC(f2i);
  DEF_BC(f2l);
  DEF_BC(f2d);
  DEF_BC(d2i);
  DEF_BC_FLOAT(d2l);
  DEF_BC(d2f);
  DEF_BC(i2b);
  DEF_BC(i2c);
  DEF_BC(i2s);
  DEF_BC(lcmp);
  DEF_BC_FLOAT(fcmpl);
  DEF_BC_FLOAT(fcmpg);
  DEF_BC_FLOAT(dcmpl);
  DEF_BC_FLOAT(dcmpg);
  DEF_BC(ifeq);
  DEF_BC(ifne);
  DEF_BC(iflt);
  DEF_BC(ifge);
  DEF_BC(ifgt);
  DEF_BC(ifle);
  DEF_BC(if_icmpeq);
  DEF_BC(if_icmpne);
  DEF_BC(if_icmplt);
  DEF_BC(if_icmpge);
  DEF_BC(if_icmpgt);
  DEF_BC(if_icmple);
  DEF_BC(if_acmpeq);
  DEF_BC(if_acmpne);
  DEF_BC(goto);
  DEF_BC(tableswitch);
  DEF_BC(lookupswitch);
  DEF_BC(ireturn);
  DEF_BC(lreturn);
  DEF_BC(freturn);
  DEF_BC(dreturn);
  DEF_BC(areturn);
  DEF_BC(return);
  DEF_BC(getstatic);
  DEF_BC(putstatic);
  DEF_BC(getfield);
  DEF_BC(putfield);
  DEF_BC(invokevirtual);
  DEF_BC(invokespecial);
  DEF_BC(invokestatic);
  DEF_BC(invokeinterface);
  DEF_BC(new);
  DEF_BC(newarray);
  DEF_BC(newarray);
  DEF_BC(arraylength);
  DEF_BC(athrow);
  DEF_BC(checkcast);
  DEF_BC(instanceof);
  DEF_BC(monitorenter);
  DEF_BC(monitorexit);
  DEF_BC(wide);
  DEF_BC(anewarray);
  DEF_BC(multianewarray);
  DEF_BC(ifnull);
  DEF_BC(ifnonnull);
  DEF_BC(goto_w);
  DEF_BC(breakpoint);
  DEF_BC(fast_1_ldc);
  DEF_BC(fast_1_ldc_w);
  DEF_BC(fast_2_ldc_w);
  DEF_BC(fast_1_putstatic);
  DEF_BC(fast_2_putstatic);
  DEF_BC(fast_a_putstatic);
  DEF_BC(fast_1_getstatic);
  DEF_BC(fast_2_getstatic);
  DEF_BC(fast_bputfield);
  DEF_BC(fast_sputfield);
  DEF_BC(fast_iputfield);
  DEF_BC(fast_lputfield);
  DEF_BC(fast_fputfield);
  DEF_BC(fast_dputfield);
  DEF_BC(fast_aputfield);
  DEF_BC(fast_bgetfield);
  DEF_BC(fast_sgetfield);
  DEF_BC(fast_igetfield);
  DEF_BC(fast_lgetfield);
  DEF_BC(fast_fgetfield);
  DEF_BC(fast_dgetfield);
  DEF_BC(fast_agetfield);
  DEF_BC(fast_cgetfield);
  DEF_BC(fast_invokevirtual);
  DEF_BC(fast_invokestatic);
  DEF_BC(fast_invokeinterface);
  DEF_BC(fast_invokenative);
  DEF_BC(fast_new);
  DEF_BC(fast_anewarray);
  DEF_BC(fast_checkcast);
  DEF_BC(fast_instanceof);
  DEF_BC(fast_invokevirtual_final);
  DEF_BC(fast_invokespecial);
  DEF_BC(fast_invokespecial);
  DEF_BC(fast_igetfield_1);
  DEF_BC(fast_agetfield_1);
#if !ENABLE_CPU_VARIANT
  DEF_BC(aload_0_fast_igetfield_1);
  DEF_BC(aload_0_fast_igetfield_4);
  DEF_BC(aload_0_fast_igetfield_8);
  DEF_BC(aload_0_fast_agetfield_1);
  DEF_BC(aload_0_fast_agetfield_4);
  DEF_BC(aload_0_fast_agetfield_8);
  DEF_BC(init_static_array);
#elif ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
  //used to replace ordinary bytecodes for some versions of JAZELLE 
  DEF_BC(lload_safe);
  DEF_BC(lstore_safe);
  DEF_BC(dload_safe);
  DEF_BC(dstore_safe);
#endif 
  DEF_BC(pop_and_npe_if_null);
  DEF_BC(fast_init_1_putstatic);
  DEF_BC(fast_init_2_putstatic);
  DEF_BC(fast_init_a_putstatic);
  DEF_BC(fast_init_1_getstatic);
  DEF_BC(fast_init_2_getstatic);
  DEF_BC(fast_init_invokestatic);
  DEF_BC(fast_init_new);
}
#undef DEF_BC
#undef DEF_BC_WIDE

// we couldn't use tty here, as it could be not initialized yet
// on all target platforms for C interpreter fprtinf(stderr, ...)
// is acceptable
#define MY_GUARANTEE(cond, str) \
    if (!(cond)) {              \
       fprintf(stderr, ">>>>>> C INTERPRETER: %s\n", str);      \
       BREAKPOINT;              \
    }

static void init() {
  // put all built-in limitations here
  MY_GUARANTEE(!TaggedJavaStack, "tagged stack not supported");
  MY_GUARANTEE(JavaStackDirection < 0, "Cannot handle forward stacks");

  inside_interpreter = false;

#if ENABLE_FLOAT
  // initialize FPU unit if needed
  InitFPU();
#endif

  // make it clean
  for (int i=0; i < ARRAY_SIZE(interpreter_dispatch_table); i++) {
    interpreter_dispatch_table[i] = &undef_bc;
  }
  // init bytecodes dispatch table
  init_dispatch_table();
}
#undef MY_GUARANTEE

// interpreter
static void Interpret() {
  // Start a new thread or continue in another existing thread
  // after thread termination.
  // NOTE that it also can invoke longjmp, so it must be called after setjmp
  resume_thread();
  // process bytecodes in the infinite loop
  if (TraceBytecodes) {
    for (;;) {
      interpreter_call_vm((address)&trace_bytecode, T_VOID);
      interpreter_dispatch_table[*g_jpc]();
    }
  } else {
    for (;;) {
      interpreter_dispatch_table[*g_jpc]();
    }
  }
}

void primordial_to_current_thread() {
  // Mark that primordial_to_current_thread() was called.
  // JVM::stop() checks this variable and calls current_thread_to_primordial()
  _primordial_sp = (address) 1;
  do {
    // We use longjmp to exit from interpreter;
    // this helps us to avoid additional checks on every bytecode execution.
    // WARNING: no stack objects are allowed in interpreter implementation
    int rv = setjmp(interpreter_env);
    if (rv == JMP_ENTER) {
      GUARANTEE(!inside_interpreter, "Cannot enter interpreter recursively");
      inside_interpreter = true;
      Interpret();
      SHOULD_NOT_REACH_HERE(); // Interpret() can exit only by longjmp
    }
    inside_interpreter = false;

    if (rv == JMP_CONTEXT_CHANGED) {
      continue;
    } else if (rv == JMP_STOPPED_MANUALLY || Universe::is_stopping()) {
      break;
    }
    GUARANTEE(rv == JMP_THREAD_EXIT, "no other values");

    if (CURRENT_HAS_PENDING_EXCEPTION) {
      Thread::lightweight_thread_uncaught_exception();
    }
    if (!TestCompiler) {
      Thread::finish();
      force_terminated(Thread::current());
    }
#if ENABLE_ISOLATES
    thread_task_cleanup();
#endif
    Thread::lightweight_thread_exit();
  } while (Scheduler::get_next_runnable_thread()->not_null());
}

void current_thread_to_primordial() {
  BREAK_INTERPRETER_LOOP(JMP_STOPPED_MANUALLY);
}

void call_on_primordial_stack(void (*)(void)) {
  SHOULD_NOT_REACH_HERE();
}

void invoke_pending_entries(Thread* thread) {
  SHOULD_NOT_REACH_HERE();
}

// simple way to do global initialization
class Initer {
public:
  Initer() {
    // IMPL_NOTE: consider whether this should be fixed
    init();
  }
  static Initer me;
};

Initer Initer::me;

typedef struct JavaFrameDebug {
  int stack_bottom_pointer;
  int stored_int_value1;
  int stored_int_value2;
  int bcp_store;
  int locals_pointer;
  int cpool;
  int method;
  int caller_fp;
  int return_address;
} JavaFrameDebug;

JavaFrameDebug * jfp_debug() {
  return (JavaFrameDebug*)( ((int)g_jfp) +
                            JavaFrame::stack_bottom_pointer_offset() );
}

#if ENABLE_COMPILER
bool __is_arm_compiler_active() {
  return Compiler::is_active();
}
#endif

#if ENABLE_CPU_VARIANT
extern "C" {
  void initialize_cpu_variant() {}
  void enable_cpu_variant() {}
  void disable_cpu_variant() {}

  void bytecode_dispatch_0x0FE() {}
  void bytecode_dispatch_0x0FF() {}
  void bytecode_dispatch_0x100() {}
  void bytecode_dispatch_0x101() {}
  void bytecode_dispatch_0x102() {}
  void bytecode_dispatch_0x103() {}
  void bytecode_dispatch_0x104() {}
  void bytecode_dispatch_0x105() {}
}
#endif

#if ENABLE_REFLECTION || ENABLE_JAVA_DEBUGGER
  void entry_return_void()   {}
  void entry_return_word()   {}
  void entry_return_long()   {}
  void entry_return_float()  {}
  void entry_return_double() {}
  void entry_return_object() {}
#endif
#if  ENABLE_JAVA_DEBUGGER
  void shared_call_vm_oop_return() {}
#endif

extern "C" {
  void wmmx_set_timer_tick() { }
  void wmmx_clear_timer_tick() { }

  // fast globals can be defined in ASM loop, for AOT romizer
#if !CROSS_GENERATOR
  JVMFastGlobals  jvm_fast_globals;
#endif
  void * _current_thread_addr = &jvm_fast_globals.current_thread;

#if ENABLE_FAST_MEM_ROUTINES || defined(USE_LIBC_GLUE)
void* jvm_memcpy(void *dest, const void *src, int n) {
  return (void*)memcpy(dest, src, n);
}
int jvm_memcmp(const void *s1, const void *s2, int n) {
  return (int)memcmp(s1, s2, n);
}
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
// These are to allow stack walking, e.g. ps(), even when executing bytecodes
bool update_java_pointers() {
  if (g_jfp == NULL || g_jsp == NULL) {
    return false;
  }
  const int bci = g_jpc - (GET_FRAME(method) + Method::base_offset());
  if (bci < 0 || bci >= 65536) {
    return false;
  }
  SET_THREAD_INT(last_java_fp, (jint)g_jfp);
  SET_THREAD_INT(last_java_sp, (jint)g_jsp);
  SET_FRAME(bcp_store, g_jpc);
  SET_FRAME(locals_pointer, g_jlocals);
  return true;
}

void revert_java_pointers() {
  SET_THREAD_INT(last_java_fp, 0);
  SET_THREAD_INT(last_java_sp, 0);
}
#endif

#if ENABLE_ARM_VFP
void vfp_redo() {}
void vfp_fcmp_redo() {}
void vfp_double_redo() {}
void vfp_dcmp_redo() {}
#endif

#if ENABLE_PAGE_PROTECTION
// Take care of page-boundary alignment of _protected_page variable
#ifdef __GNUC__
#define ALIGN(x) __attribute__ ((aligned (x)))
unsigned char _protected_page[PROTECTED_PAGE_SIZE] ALIGN(4096);
#else 
unsigned char _dummy1[PROTECTED_PAGE_SIZE];
unsigned char _protected_page[PROTECTED_PAGE_SIZE];
unsigned char _dummy2[PROTECTED_PAGE_SIZE];
#endif // __GNUC__
#endif // ENABLE_PAGE_PROTECTION

} // extern "C"
