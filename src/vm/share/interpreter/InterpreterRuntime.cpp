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
#include "incls/_InterpreterRuntime.cpp.incl"


extern "C" {
  extern jlong bailout_counters[Bytecodes::number_of_java_codes];
}

extern "C" {

#if ENABLE_ISOLATES
// We use a different entry point for barrier hit by the compiler for
// for debugging right now

  ReturnOop compiled_code_task_barrier(Thread *thread, OopDesc *ic_desc
                                       JVM_TRAPS) {
    return task_barrier(thread, ic_desc JVM_NO_CHECK_AT_BOTTOM);
  }

  // Handling of class initialization barriers
  ReturnOop task_barrier(Thread* /*thread*/, OopDesc *ic_desc JVM_TRAPS) {
    UsingFastOops fast_oops;
    InstanceClass::Fast ic = ic_desc;
    TaskMirrorDesc* tmd = ic().task_mirror_desc();
    if (TaskMirrorDesc::is_initialized_mirror(tmd)){
      return tmd; 
    }

    // Handle cases when the class is not initialized or is being initialized
    tmd = (TaskMirrorDesc *) ic().initialize_for_task(JVM_SINGLE_ARG_CHECK_0);

    // If we are not the initializing thread, we will block in the deferred 
    // invocation of the java.lang.Class.initialize method.
    // When we reach these points, there can be only two cases:
    // 1. the class is fully initialized, 
    // 2. the class is being initialized and the current thread is the
    //    initializer of the class.
    return tmd;
  }

#else

  void initialize_class(Thread* thread, OopDesc* raw_klass JVM_TRAPS) {
    (void)thread;
    UsingFastOops fast_oops;
    InstanceClass::Fast klass = raw_klass;
    GUARANTEE(klass().is_instance_class() && !klass().is_initialized(), 
              "Should not be called for initialized classes");
    klass().initialize(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }

#endif

  void stack_overflow(Thread* thread, address stack_pointer) {
    Thread::stack_overflow(thread, stack_pointer);
  }

  void timer_tick() {
    if (!VerifyOnly) {
#if ENABLE_CODE_PATCHING
      Compiler::unpatch_checkpoints();
#endif
      // No thread switch should happen, especially during Monet + MVM 
      // conversion.
      Thread::timer_tick();
    }
  }

  void lock_stack_lock(Thread* thread, StackLock* stack_lock JVM_TRAPS) {
    Synchronizer::enter(thread, stack_lock JVM_NO_CHECK_AT_BOTTOM);
  }

  // This is called if the object being unlocked is a String.  The 
  // actual object locked is allocated at runtime so that interned
  // strings can be ROMized into the TEXT section.  Also, in the MVM
  // case, interned strings are read-only if in a binary image
  // so we must have a synchronization object that is read/write.
  //  Hence we allocate one when locking the object.
  void unlock_special_stack_lock(Thread* thread, OopDesc *lock_obj JVM_TRAPS) {
    UsingFastOops fastoops;
    StackLock *stack_lock = NULL;

#define STACK_LOCK_SIZE (StackLock::size() + 4)

    // get the object that Java thinks is being unlocked
    JavaOop::Fast obj(lock_obj);
    // find the object that we really used to lock
    obj = Synchronizer::get_lock_object_ref(&obj, thread, true JVM_CHECK);
    GUARANTEE(!obj.is_null(),"Couldn't find lock object");
#ifdef AZZERT
    JavaNear::Raw java_near = obj.klass();
    GUARANTEE(java_near().is_locked(), "object not locked");
#endif
    // get pointer to this stack lock
    JavaFrame fr(thread);
    int stack_lock_len = fr.stack_lock_length();
    int num_locks = stack_lock_len / STACK_LOCK_SIZE;
    for (int i = num_locks - 1; i >= 0; i--) {
      stack_lock = fr.stack_lock_at(i * STACK_LOCK_SIZE, stack_lock_len);
      if (stack_lock->owner() == obj.obj()) {
        break;
      }
      stack_lock = NULL;
    }
    GUARANTEE(stack_lock != NULL, "Couldn't find stack lock for string object");
    Oop::Fast real_near = stack_lock->real_java_near();
    if (real_near.is_null()) {
      // recursive unlock
      stack_lock->clear_owner();
      Synchronizer::release_lock_object_ref(&obj);
      return;
    } else {
      Synchronizer::exit(stack_lock);
      Synchronizer::release_lock_object_ref(&obj);
    }
  }

  void signal_waiters(Thread * /*thread*/, StackLock* stack_lock) {
    Synchronizer::signal_waiters(stack_lock);
  }

  jlong jvm_ldiv(jlong dividend, jlong divisor) {
    GUARANTEE(divisor != 0, "Already check for division by zero");
    return dividend / divisor;
  }

  jlong jvm_lrem(jlong dividend, jlong divisor) {
    GUARANTEE(divisor != 0, "Already check for division by zero");
    return dividend % divisor;
  }

  void array_index_out_of_bounds_exception(JVM_SINGLE_ARG_TRAPS) {
    Throw::array_index_out_of_bounds_exception(empty_message JVM_THROW);
  }

  void null_pointer_exception(JVM_SINGLE_ARG_TRAPS) {
    Throw::null_pointer_exception(empty_message JVM_THROW);
  }

  void illegal_monitor_state_exception(JVM_SINGLE_ARG_TRAPS) {
    Throw::illegal_monitor_state_exception(empty_message JVM_THROW);
  }

  void division_by_zero_exception(JVM_SINGLE_ARG_TRAPS) {
    Throw::arithmetic_exception(idiv_err JVM_THROW);
  }

  void incompatible_class_change_error(JVM_SINGLE_ARG_TRAPS) {
    Throw::incompatible_class_change_error(empty_message JVM_THROW);
  }

  static ReturnOop getException(void (*thrower)(JVM_SINGLE_ARG_TRAPS)
                                JVM_TRAPS) {
    GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "Sanity check");
    thrower(JVM_SINGLE_ARG_NO_CHECK);
    // This is a bit of a temporary fix. We steal the pending exception and move
    // it into the stored exception slot.
    JavaOop::Raw result = Thread::current_pending_exception();
    Thread::clear_current_pending_exception();
    return result;
  }

  ReturnOop get_illegal_monitor_state_exception(JVM_SINGLE_ARG_TRAPS) {
    return getException(illegal_monitor_state_exception 
                        JVM_NO_CHECK_AT_BOTTOM);
  }

  ReturnOop get_array_index_out_of_bounds_exception(JVM_SINGLE_ARG_TRAPS) {
    return getException(array_index_out_of_bounds_exception 
                        JVM_NO_CHECK_AT_BOTTOM);
  }

  ReturnOop  get_null_pointer_exception(JVM_SINGLE_ARG_TRAPS) {
    return getException(null_pointer_exception JVM_NO_CHECK_AT_BOTTOM);
  }

  ReturnOop  get_division_by_zero_exception(JVM_SINGLE_ARG_TRAPS) {
    return getException(division_by_zero_exception JVM_NO_CHECK_AT_BOTTOM);
  }

  ReturnOop get_incompatible_class_change_error(JVM_SINGLE_ARG_TRAPS) {
    return getException(incompatible_class_change_error
                        JVM_NO_CHECK_AT_BOTTOM);
  }

  void arithmetic_exception(JVM_SINGLE_ARG_TRAPS) {
    Throw::arithmetic_exception(empty_message JVM_THROW);
  }

  ReturnOop newobject(JVM_SINGLE_ARG_TRAPS) {
    UsingFastOops fast_oops;
    
    JavaFrame frame(Thread::current());
    Method::Fast method = frame.method();
    const int bci = frame.bci();
    jushort klass_index = method().ushort_at(bci + 1);
    ConstantPool::Fast cp = method().constants();
    InstanceClass::Fast instance_class = cp().klass_at(klass_index JVM_CHECK_0);
    Instance::Fast result;

    if (!instance_class().is_instance_class()
          || instance_class().is_abstract()
          || instance_class().is_interface()) {
      Throw::instantiation(ErrorOnFailure JVM_THROW_0);
    }

    InstanceClass::Fast sender_class = method().holder();
    instance_class().check_access_by(&sender_class, ErrorOnFailure JVM_CHECK_0);

    result = instance_class().new_instance(ErrorOnFailure JVM_CHECK_0);

    // Now that everything above worked and no more exceptions are expected,
    // we can rewrite the byte code.
    // In other words: don't ever move the rewriting above any of
    // the 'JVM_CHECK_0' statements!

    Bytecodes::Code bc = method().bytecode_at(bci);
    if (bc == Bytecodes::_new) {
      if (instance_class().has_finalizer()) {
        // Classes with finalize() method will continue to go slow-case
      } else {
        if (ENABLE_ISOLATES || instance_class().is_initialized()) {
          bc = Bytecodes::_fast_new;
        } else {
          bc = Bytecodes::_fast_init_new;
        }
        if (method().may_be_quickened()) {
          method().bytecode_at_put(bci, bc);
        }
      }
    } else {
      GUARANTEE(bc == Bytecodes::_fast_new || bc == Bytecodes::_fast_init_new,
                "Don't rewrite if the bytecode is already rewritten");
    }

    return result;
  }

#if ENABLE_INTERPRETER_GENERATOR || ENABLE_ROM_GENERATOR
  // Quicken the 'new' bytecode. This method is used only by the Romizer
  void _quicken_new(Method& method, jint bci JVM_TRAPS) {
    jushort klass_index = method.ushort_at(bci + 1);
    ConstantPool cp = method.constants();
    InstanceClass instance_class = cp.klass_at(klass_index JVM_CHECK);

    if (!instance_class.is_instance_class() || instance_class.is_abstract()
           || instance_class.is_interface()) {
      Throw::instantiation(ErrorOnFailure JVM_THROW);
    }

    InstanceClass sender_class = method.holder();
    instance_class.check_access_by(&sender_class, ErrorOnFailure JVM_CHECK);
    
    GUARANTEE(method.bytecode_at(bci) == Bytecodes::_new, "Sanity check");

    if (instance_class.has_finalizer()) {
      // Classes with finalize() method will continue to go slow-case
    } else {
      Bytecodes::Code bc;
      if (ENABLE_ISOLATES || instance_class.is_initialized()) {
        bc = Bytecodes::_fast_new;
      } else {
        bc = Bytecodes::_fast_init_new;
      }
      method.bytecode_at_put(bci, bc);
    }
  }
#endif

  OopDesc* anewarray(JVM_SINGLE_ARG_TRAPS) {
    UsingFastOops fast_oops;
    
    JavaFrame frame(Thread::current());
    Method::Fast method = frame.method();
    const int bci = frame.bci();
    jushort klass_index  = method().ushort_at(bci + 1);
    int length = frame.expression_at(0)->as_int();
    ConstantPool::Fast cp = method().constants();
    JavaClass::Fast klass = cp().klass_at(klass_index JVM_CHECK_0);

    InstanceClass::Fast sender_class = method().holder();
    klass().check_access_by(&sender_class, ErrorOnFailure JVM_CHECK_0);

    // Rewrite the bytecode if necessary.
    const Bytecodes::Code bc = method().bytecode_at(bci);
    if (bc == Bytecodes::_anewarray) {
      if (method().may_be_quickened()) {
        method().bytecode_at_put(bci, Bytecodes::_fast_anewarray);
      }
    } else {
      GUARANTEE(bc == Bytecodes::_fast_anewarray, "Sanity check");
    }
    return Universe::new_obj_array(&klass, length JVM_NO_CHECK_AT_BOTTOM_0);
  }

  OopDesc* _newarray(Thread *thread, BasicType type, int length JVM_TRAPS) {
#ifdef AZZERT
    JavaFrame frame(thread);
    frame.verify();
#else
    (void)thread;
#endif
    
    // Note that this code carefully doesn't create any handles.
    TypeArrayClass* array_class = Universe::as_TypeArrayClass(type);
    return Universe::new_type_array(array_class, length 
                                            JVM_NO_CHECK_AT_BOTTOM_0);
  }
  
#if ENABLE_INTERPRETER_GENERATOR || ENABLE_ROM_GENERATOR
  void _quicken_anewarray(Method& method, jint bci JVM_TRAPS) {
    jushort klass_index  = method.ushort_at(bci + 1);
    ConstantPool cp = method.constants();
    JavaClass klass = cp.klass_at(klass_index JVM_CHECK);

#if USE_BINARY_IMAGE_GENERATOR
    if (GenerateROMImage) {
      // if klass_index is in system classes then don't allow binary
      // image to have quickened bytecode since the array_class entry
      // in the system class (at _class_list[klass_index]) will be 
      // 0 (zero) when the system classes are initialized at runtime.
      if (klass_index < ROM::number_of_system_classes()) {
        ROMOptimizer::trace_failed_quicken(&method, &klass JVM_CHECK);
        return;
      }
    }
#endif
      
    InstanceClass sender_class = method.holder();
    klass.check_access_by(&sender_class, ErrorOnFailure JVM_CHECK);
    klass.get_array_class(1 JVM_CHECK);

    GUARANTEE(method.bytecode_at(bci)==Bytecodes::_anewarray, "Sanity check");
    method.bytecode_at_put(bci, Bytecodes::_fast_anewarray);
  }
#endif

  OopDesc* multianewarray(JVM_SINGLE_ARG_TRAPS) {
    UsingFastOops fast_oops;

    JavaFrame frame(Thread::current());
    Method::Fast method  = frame.method();
    const int bci = frame.bci();
    jushort klass_index  = method().get_java_ushort(bci + 1);
    int dimensions = method().ubyte_at(bci + 3);
    ConstantPool::Fast cp = method().constants();
    ArrayClass::Fast array_class = cp().klass_at(klass_index JVM_CHECK_0);
    InstanceClass::Fast sender_class = method().holder();
    Array::Fast result;

    if (array_class.is_obj_array_class()) {
      ObjArrayClass* obj_array_class = (ObjArrayClass*)&array_class;
      obj_array_class->check_access_by(&sender_class,
                                      ErrorOnFailure JVM_CHECK_0);
      result = obj_array_class->multi_allocate(dimensions, &frame JVM_CHECK_0);
    } else {
      TypeArrayClass* type_array_class = (TypeArrayClass*)&array_class;
      result = Universe::new_type_array(type_array_class,
                                        frame.expression_at(0)->as_int()
                                        JVM_CHECK_0);
    }
#ifdef AZZERT
    JavaClass::Fast f = result.blueprint();
    GUARANTEE(f.equals(&array_class), "checking class pointer");
#endif
    return result;
  }

  jint instanceof(JVM_SINGLE_ARG_TRAPS) {    
    UsingFastOops fast_oops;
    JavaFrame frame(Thread::current());
    Method::Fast method = frame.method();
    const int bci = frame.bci();
    jushort index = method().ushort_at(bci + 1);
    ConstantPool::Fast cp = method().constants();
    JavaClass::Fast klass = cp().klass_at(index JVM_CHECK_0);
    Oop::Fast object = frame.expression_at(0)->as_obj();

    // Rewrite the bytecode if necessary.
    const Bytecodes::Code bc = method().bytecode_at(bci);
    if (bc == Bytecodes::_instanceof) {
      if (method().may_be_quickened()) {
        method().bytecode_at_put(bci, Bytecodes::_fast_instanceof);
      }
    } else {
      GUARANTEE(bc == Bytecodes::_fast_instanceof, "Sanity check");
    }

    if (object.is_null()) return false;
    JavaClass::Fast object_klass = object.blueprint();
    return (object_klass().is_subtype_of(&klass) ? 1 : 0);
  }

#if ENABLE_INTERPRETER_GENERATOR || ENABLE_ROM_GENERATOR

  void _quicken_instanceof(Method& method, jint bci JVM_TRAPS) {
    
    jushort index = method.ushort_at(bci + 1);
    ConstantPool cp = method.constants();
    JavaClass klass = cp.klass_at(index JVM_CHECK);

    GUARANTEE(method.bytecode_at(bci)==Bytecodes::_instanceof, "Sanity check");
    method.bytecode_at_put(bci, Bytecodes::_fast_instanceof);
  }
#endif

  void checkcast(JVM_SINGLE_ARG_TRAPS) {
    UsingFastOops fast_oops;
    JavaFrame frame(Thread::current());
    Method::Fast method = frame.method();
    const int bci = frame.bci();
    jushort index = method().ushort_at(bci + 1);
    ConstantPool::Fast cp = method().constants();
    JavaClass::Fast klass = cp().klass_at(index JVM_CHECK);
    Oop::Fast object = frame.expression_at(0)->as_obj();

    // Rewrite the bytecode if necessary.
    const Bytecodes::Code bc = method().bytecode_at(bci);
    if (bc == Bytecodes::_checkcast) {
      if (method().may_be_quickened()) {
        method().bytecode_at_put(bci, Bytecodes::_fast_checkcast);
      }
    } else {
      GUARANTEE(bc == Bytecodes::_fast_checkcast, "Sanity check");
    }

    if (object.is_null()) {
      return;
    }
    JavaClass::Fast object_klass = object.blueprint();
    if (object_klass().is_subtype_of(&klass)) return;
    Throw::throw_exception(Symbols::java_lang_ClassCastException() JVM_THROW);
  }

#if ENABLE_INTERPRETER_GENERATOR || ENABLE_ROM_GENERATOR

  void _quicken_checkcast(Method& method, jint bci JVM_TRAPS) {
    
    jushort index = method.ushort_at(bci + 1);
    ConstantPool cp = method.constants();
    JavaClass klass = cp.klass_at(index JVM_CHECK);

    GUARANTEE(method.bytecode_at(bci)==Bytecodes::_checkcast, "Sanity check");
    method.bytecode_at_put(bci, Bytecodes::_fast_checkcast);
  }
#endif

  void array_store_type_check(JVM_SINGLE_ARG_TRAPS) {
    
    RuntimeFrame frame(Thread::current());
    Oop::Raw   object = frame.obj_at(0);
    Array::Raw array  = frame.obj_at(2);

    GUARANTEE(object.not_null() && array.not_null(), "Sanity check");

    ObjArrayClass::Raw array_klass= array.blueprint();
    JavaClass::Raw element_klass  = array_klass().element_class();
    JavaClass::Raw object_klass   = object.blueprint();

    if (!object_klass().is_subtype_of(&element_klass)) {
      Throw::array_store_exception(empty_message JVM_THROW);
    }
  }

  Bytecodes::Code compute_rewrite_bytecode(Bytecodes::Code base,
                                           bool is_static, bool is_get,
                                           BasicType type) {
    static const jubyte table[] = {
      /* <unused>    =  0 */ 0xff,
      /* <unused>    =  1 */ 0xff,
      /* <unused>    =  2 */ 0xff,
      /* <unused>    =  3 */ 0xff,
      /* T_BOOLEAN   =  4 */ 0,
      /* T_CHAR      =  5 */ 1,
      /* T_FLOAT     =  6 */ 4,
      /* T_DOUBLE    =  7 */ 5,
      /* T_BYTE      =  8 */ 0,
      /* T_SHORT     =  9 */ 1,
      /* T_INT       = 10 */ 2,
      /* T_LONG      = 11 */ 3,
      /* T_OBJECT    = 12 */ 6,
      /* T_ARRAY     = 13 */ 6,
    };

    GUARANTEE(T_BOOLEAN <= type && type <= T_ARRAY, "sanity");
    int offset = table[type];
    if (type == T_CHAR && !is_static && is_get) {
      // Special handling of fast_cgetfield
      offset = 7;
    }

    if (is_static) {
      // There are no byte/short/char fast bytecodes for static fields.
      // Adjust offset to have T_INT as base and convert byte/short/char into
      // int.
      offset -= 2;
      if (offset < 0) {
        offset = 0;
      }
    }
    return (Bytecodes::Code) (base + offset);
  }

  Bytecodes::Code _rewrite_field_bytecode(Method &method,
                                          jint bci, Bytecodes::Code base,
                                          bool is_get JVM_TRAPS) {
    UsingFastOops fast_oops;

    jushort index = method.get_Java_u2_index_at(bci + 1);
    InstanceClass::Fast sender_class = method.holder();
    ConstantPool::Fast cp = method.constants();
    InstanceClass::Fast dummy_declaring_class; // not used
    int offset;

    // Rewriting must happen after class initialization in case class
    // initialization throws an exception.
    BasicType type = cp().field_type_at(index, offset, false, is_get,
                                        &sender_class, &dummy_declaring_class
                                        JVM_CHECK_(Bytecodes::_illegal)); 
    Bytecodes::Code bc = compute_rewrite_bytecode(base, false, is_get, type);
    if (::byte_size_for(type) >= BytesPerWord) {
      GUARANTEE(offset % BytesPerWord == 0, "Must be aligned");
      offset = offset >> LogBytesPerWord;
    }
    GUARANTEE(((juint)offset) <= 0xffff, "sanity");
    if (!ENABLE_NATIVE_ORDER_REWRITING) { 
      method.put_java_ushort(bci + 1, (jushort)offset);
    } else { 
      method.write_u2_index_at(bci + 1, (jushort)offset);
    }
    method.bytecode_at_put(bci, bc);
    return bc;
  }

  Bytecodes::Code rewrite_field_bytecode(Bytecodes::Code base,
                                         bool is_get JVM_TRAPS) {
    UsingFastOops fast_oops;
    JavaFrame frame(Thread::current());
    Method::Fast method = frame.method();

    Bytecodes::Code code = _rewrite_field_bytecode(method(), frame.bci(), base,
                                   is_get JVM_CHECK_(Bytecodes::_illegal));
    return code;
  }

  Bytecodes::Code _rewrite_static_field_bytecode(Method &method,
                                                 jint bci,
                                                 bool is_get, bool do_init
                                                 JVM_TRAPS) {

    UsingFastOops fast_oops;
    jushort index = method.get_java_ushort(bci + 1);

    InstanceClass::Fast sender_class = method.holder();
    ConstantPool::Fast cp = method.constants();
    InstanceClass::Fast declaring_class;
    int offset;

    // Rewriting must happen after class initialization in case class
    // initialization throws an exception.
    BasicType type = cp().field_type_at(index, offset, true, is_get, 
                                        &sender_class, &declaring_class
                                        JVM_CHECK_(Bytecodes::_illegal));
    Bytecodes::Code bc =
        is_get ? Bytecodes::_fast_1_getstatic : Bytecodes::_fast_1_putstatic;
    if (type == T_LONG || type == T_DOUBLE) {
      bc = (Bytecodes::Code)(bc + 1);
    } else if (!is_get && (type == T_OBJECT || type == T_ARRAY)) {
      bc = Bytecodes::_fast_a_putstatic;
    }

    if (do_init) {
      // Initialize the class as entry activation
#if ENABLE_ISOLATES
      task_barrier(Thread::current(), declaring_class.obj()
                   JVM_CHECK_(Bytecodes::_illegal));
#else
      declaring_class().initialize(JVM_SINGLE_ARG_CHECK_(Bytecodes::_illegal));
#endif
    } else {
      GUARANTEE(GenerateROMImage, "This should happen only during romization");
    }

    if (!ENABLE_ISOLATES && !declaring_class().is_initialized() &&
        !sender_class().is_subclass_of(&declaring_class)) {
      // This bytecode can lead to class initialization
      bc = Bytecodes::cast(bc + Bytecodes::_fast_init_1_putstatic -
                                Bytecodes::_fast_1_putstatic);
    }
    
    if (method.may_be_quickened()) {
      method.bytecode_at_put(bci, bc);
    }
    return bc;
  }

  Bytecodes::Code rewrite_static_field_bytecode(bool is_get JVM_TRAPS) {
    UsingFastOops fast_oops;
    JavaFrame frame(Thread::current());
    Method::Fast method = frame.method();
    Bytecodes::Code code = _rewrite_static_field_bytecode(method(),
             frame.bci(), is_get, true JVM_CHECK_(Bytecodes::_illegal));
    return code;
  }

  Bytecodes::Code putstatic(JVM_SINGLE_ARG_TRAPS) {
    Bytecodes::Code code = rewrite_static_field_bytecode(false 
                                   JVM_CHECK_(Bytecodes::_illegal));
    return code;
  }

  Bytecodes::Code getstatic(JVM_SINGLE_ARG_TRAPS) {
    Bytecodes::Code code = rewrite_static_field_bytecode(true 
                           JVM_CHECK_(Bytecodes::_illegal));
    return code;
  }

  Bytecodes::Code putfield(JVM_SINGLE_ARG_TRAPS) {
    Bytecodes::Code code = rewrite_field_bytecode(Bytecodes::_fast_bputfield, 
                           false JVM_CHECK_(Bytecodes::_illegal));
    return code;
  }

  Bytecodes::Code getfield(JVM_SINGLE_ARG_TRAPS) {
    Bytecodes::Code code = rewrite_field_bytecode(Bytecodes::_fast_bgetfield,
                           true JVM_CHECK_(Bytecodes::_illegal));
    return code;
  }

  bool find_exception_frame(Thread* thread, OopDesc* raw_exception JVM_TRAPS) {
    
    UsingFastOops fast_oops;
    JavaOop::Fast exception = raw_exception;
    JavaFrame frame(thread);
    return frame.find_exception_frame(thread, &exception 
                                      JVM_NO_CHECK_AT_BOTTOM_0);
  }

#ifndef PRODUCT
  void check_interpreter_parameter_tags() {
    UsingFastOops fast_oops;
    JavaFrame frame(Thread::current());
    Method::Fast method = frame.method();
    Signature::Fast signature = method().signature();
    for (SignatureStream ss(&signature, method().is_static()); !ss.eos();
              ss.next()) {
      int index = ss.index();
      StackValue* local = frame.local_at(index);

      switch(stack_type_for(ss.type())) {
        case T_INT:
          GUARANTEE(local->tag() == int_tag, "type check");
          break;
        case T_OBJECT:
          GUARANTEE(local->tag() == obj_tag, "type check");
          break;
        case T_LONG:
          GUARANTEE(local->tag() == long_tag, "type check");
          GUARANTEE(frame.local_at(index + 1)->tag() == long2_tag,
                    "type check");
          break;
        case T_FLOAT:
          GUARANTEE(local->tag() == float_tag, "type check");
          break;
        case T_DOUBLE:
          GUARANTEE(local->tag() == double_tag, "type check");
          GUARANTEE(frame.local_at(index + 1)->tag() == double2_tag,
                    "type check");
          break;
        default:
          AZZERT_ONLY_VAR(local);
          SHOULD_NOT_REACH_HERE();
      }
    }
  }

  void internal_stack_tag_exception() {
    UsingFastOops fast_oops;
    // Invalid tag on stack
    JavaFrame frame(Thread::current());
    Method::Fast method = frame.method();
    ConstantPool::Fast cp = method().constants();
    (void)cp;
    BREAKPOINT;
  }

  void breakpoint() {
    BREAKPOINT;
  }

#endif

  Bytecodes::Code _quicken_invokestatic(Method &method, jint bci,
                                        bool do_init JVM_TRAPS) {
    UsingFastOops fast_oops;
    InstanceClass::Fast sender_class = method.holder();
    ConstantPool::Fast cp = method.constants();
    jushort index = method.get_java_ushort(bci + 1);

    bool is_initialized =
        cp().resolve_invoke_static_at(&sender_class, index,
                                      do_init JVM_CHECK_(Bytecodes::_illegal));
    Bytecodes::Code bc = ENABLE_ISOLATES || is_initialized ?
                         Bytecodes::_fast_invokestatic :
                         Bytecodes::_fast_init_invokestatic;
    if (method.may_be_quickened()) {
      method.bytecode_at_put(bci, bc);
    }
    return bc;
  }

  Bytecodes::Code quicken_invokestatic(JVM_SINGLE_ARG_TRAPS) {
    UsingFastOops fast_oops;
    JavaFrame frame(Thread::current());
    Method::Fast method   = frame.method();
    jint bci        = frame.bci();

    Bytecodes::Code code = _quicken_invokestatic(method(), bci, true
                                 JVM_CHECK_(Bytecodes::_illegal));
    return code;
  }

  Bytecodes::Code _quicken(Method &method, jint bci JVM_TRAPS) {
    UsingFastOops fast_oops;

    InstanceClass::Fast sender_class = method.holder();
    ConstantPool::Fast cp = method.constants();
    const Bytecodes::Code bc = method.bytecode_at(bci);
    Bytecodes::Code quicken_bc;

    switch (bc) {
    case Bytecodes::_ldc: {
        jint index = method.ubyte_at(bci+1);
        ConstantTag tag = cp().tag_at(index);
        // Resolve the constant pool entry.
        if (tag.is_unresolved_string()) {
          cp().string_at(index JVM_CHECK_(Bytecodes::_illegal));
        }
        GUARANTEE(   cp().tag_at(index).is_string()
                  || cp().tag_at(index).is_float()
                  || cp().tag_at(index).is_int(), "Sanity check");
#if ENABLE_JAVA_STACK_TAGS
             if (tag.is_int())   quicken_bc = Bytecodes::_fast_ildc;
        else if (tag.is_float()) quicken_bc = Bytecodes::_fast_fldc;
        else                     quicken_bc = Bytecodes::_fast_aldc;
#else
      quicken_bc = Bytecodes::_fast_1_ldc;
#endif
      }
      break;
    case Bytecodes::_ldc_w: {
        jushort index = method.get_java_ushort(bci + 1);

        // Resolve the constant pool entry.
        if (cp().tag_at(index).is_unresolved_string()) {
          cp().string_at(index JVM_CHECK_(Bytecodes::_illegal));
        }
        GUARANTEE(cp().tag_at(index).is_string()
                  || cp().tag_at(index).is_float()
                  || cp().tag_at(index).is_int(), "Sanity check");
#if ENABLE_JAVA_STACK_TAGS
        ConstantTag tag = cp().tag_at(index);
             if (tag.is_int())   quicken_bc = Bytecodes::_fast_ildc_w;
        else if (tag.is_float()) quicken_bc = Bytecodes::_fast_fldc_w;
        else                     quicken_bc = Bytecodes::_fast_aldc_w;
#else
        quicken_bc = Bytecodes::_fast_1_ldc_w;
#endif

      }
      break;

    case Bytecodes::_ldc2_w: {
        jushort index = method.get_java_ushort(bci + 1);
        // Verify tag
        GUARANTEE(cp().tag_at(index).is_double() ||
                  cp().tag_at(index).is_long(), "Sanity check");
        (void)index;
#if ENABLE_JAVA_STACK_TAGS
        ConstantTag tag = cp().tag_at(index);
        quicken_bc = tag.is_long() ? Bytecodes::_fast_lldc_w
                                   : Bytecodes::_fast_dldc_w;
#else
        quicken_bc = Bytecodes::_fast_2_ldc_w;
#endif
      }
      break;
    case Bytecodes::_invokevirtual: {
        jushort index = method.get_java_ushort(bci + 1);
        bool is_final = cp().resolve_invoke_virtual_at(&sender_class, index
                                             JVM_CHECK_(Bytecodes::_illegal));

#if ENABLE_MONET
        if (GenerateROMImage && !is_final) {
          int vtable_index, class_id;
          cp().resolved_virtual_method_at(index, vtable_index, class_id);
          JavaClass::Raw klass = Universe::class_from_id(class_id);
          ClassInfo::Raw info = klass().class_info();
          Method::Raw real_method = info().vtable_method_at(vtable_index);
          InstanceClass::Raw holder_class = real_method().holder();
          if(!((ROMWriter::_singleton)->_optimizer).is_overridden(&holder_class, 
                                             &real_method))  {
	    if (!real_method().is_abstract()) {
              is_final = true;
              cp().resolved_static_method_at_put(index, &real_method);
#ifndef PRODUCT
              if (TraceRomizer) {
                TTY_TRACE(("Forced invoke final: "));
                real_method().print_name_on(tty);
                TTY_TRACE_CR((""));
              }
#endif
	    }
          }
        }
#endif
        quicken_bc = is_final ? Bytecodes::_fast_invokevirtual_final
                              : Bytecodes::_fast_invokevirtual;
      }
      break;
    case Bytecodes::_invokespecial: {
        jushort index = method.get_java_ushort(bci + 1);
        bool is_init = cp().resolve_invoke_special_at(&sender_class, index
                                              JVM_CHECK_(Bytecodes::_illegal));
        quicken_bc = is_init ? Bytecodes::_fast_invokevirtual_final
                             : Bytecodes::_fast_invokespecial;
      }
      break;

    case Bytecodes::_invokeinterface: {
        jushort index = method.get_java_ushort(bci + 1);
        cp().resolve_invoke_interface_at(&sender_class, index
                                         JVM_CHECK_(Bytecodes::_illegal));
        switch (cp().tag_at(index).value()) {
        case JVM_CONSTANT_ResolvedUncommonInterfaceMethod:
          // This case is actually a java.lang.Object non-final method
          // masquerading as an interface method. E.g.,
          //     aload_0
          //     invokeinterface SomeInterface.hashCode() 1
          quicken_bc = Bytecodes::_fast_invokevirtual;
          break;
        case JVM_CONSTANT_ResolvedFinalUncommonInterfaceMethod:
          // This case is actually a java.lang.Object final method
          // masquerading as an interface method. E.g.,
          //     aload_0
          //     invokeinterface SomeInterface.getClass() 1
          quicken_bc = Bytecodes::_fast_invokevirtual_final;
          break;
#if USE_SOURCE_IMAGE_GENERATOR || ( ENABLE_MONET && !ENABLE_LIB_IMAGES)
        case JVM_CONSTANT_ResolvedStaticMethod:
          // This case is an optimization
          // this method has only one implementation and 
          // GUARANTEED won't have another
          // so we replace invoke interface to _fast_invokevirtual_final
          // of this implementation
          quicken_bc = Bytecodes::_fast_invokevirtual_final;
          break;
        case JVM_CONSTANT_ResolvedBooleanVirtualMethod:
        case JVM_CONSTANT_ResolvedCharVirtualMethod:
        case JVM_CONSTANT_ResolvedFloatVirtualMethod:
        case JVM_CONSTANT_ResolvedDoubleVirtualMethod:
        case JVM_CONSTANT_ResolvedByteVirtualMethod:
        case JVM_CONSTANT_ResolvedShortVirtualMethod:
        case JVM_CONSTANT_ResolvedIntVirtualMethod:
        case JVM_CONSTANT_ResolvedLongVirtualMethod:
        case JVM_CONSTANT_ResolvedObjectVirtualMethod:
        case JVM_CONSTANT_ResolvedArrayVirtualMethod:
        case JVM_CONSTANT_ResolvedVoidVirtualMethod:

          // This case is an optimization
          // this method has only one implementation and 
          // GUARANTEED won't have another
          // so we replace invoke interface to _fast_invokevirtual_final
          // of this implementation
          quicken_bc = Bytecodes::_fast_invokevirtual;
          break;
#endif
        default:
          quicken_bc = Bytecodes::_fast_invokeinterface;
        }

        if (quicken_bc != Bytecodes::_fast_invokeinterface) {
          // invokeinterface takes 4 bytes of operands.
          // operand #3 and #4 don't exist for invokevirtual. Make them
          // into no-ops.
          method.bytecode_at_put_raw(bci + 3, Bytecodes::_nop);
          method.bytecode_at_put_raw(bci + 4, Bytecodes::_nop);
        }
      }
      break;

    default:
      SHOULD_NOT_REACH_HERE();
      return (Bytecodes::Code)0;
    }
    method.bytecode_at_put(bci, quicken_bc);
    return (quicken_bc);
  }

  Bytecodes::Code quicken(JVM_SINGLE_ARG_TRAPS) {
    UsingFastOops fast_oops;
    JavaFrame frame(Thread::current());
    Method::Fast method = frame.method();

    Bytecodes::Code code = _quicken(method(), frame.bci() 
                                    JVM_CHECK_(Bytecodes::_illegal));
    return code;
  }

#if ENABLE_COMPILER

  void deoptimize() {
    // Deoptimize the frame.
    JavaFrame frame(Thread::current());
    AZZERT_ONLY(frame.verify());
    frame.deoptimize();
  }

  void uncommon_trap() {
    // An uncommon trap has occurred in compiled code.
    // Deoptimize the frame.
    JavaFrame frame(Thread::current());
    Method::Raw method = frame.method();
    AZZERT_ONLY(frame.verify());

#if ENABLE_PERFORMANCE_COUNTERS
    jvm_perf_count.uncommon_traps_taken ++;
#endif

    CompiledMethod::Raw trapper = frame.compiled_method();
    CompiledMethod::Raw method_code;
    frame.deoptimize();
    // Unlink the compiled code.
    if (method().has_compiled_code()) {
      method_code = method().compiled_code();
      if (trapper.equals(&method_code)) {
        method().unlink_compiled_code();
        method().set_execution_entry((address) shared_invoke_compiler);
      }
    }

#ifndef PRODUCT
    if (TraceUncommonTrap) {
      TTY_TRACE(("Uncommon_trap: "));
      method().print_name_on(tty);
      TTY_TRACE_CR((""));
    }
#endif

    // Return/continue in the interpreter.
  }

  ReturnOop get_method(Thread* thread, address pc) {
    (void)thread;
    CompiledMethod::Raw cm = JavaFrame::find_compiled_method(pc);
    GUARANTEE(cm.not_null(), "Sanity");
    return cm().method();
  }
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  static long trace_bcs = 0;
#endif

  void trace_bytecode() {
#if !defined(PRODUCT) || ENABLE_TTY_TRACE
    UsingFastOops fast_oops;
    Thread *thread = Thread::current();
    JavaFrame frame(thread);
    Method::Fast method = frame.method();
    int bci = frame.bci();

    if (TraceBytecodesStart <= trace_bcs
        && ((trace_bcs - TraceBytecodesStart) % TraceBytecodesInterval == 0)
        && (TraceBytecodes || TraceBytecodesCompiler)) {
      if (TraceBytecodesVerbose) {
        PrintLongFrames++;
        if (TraceBytecodesVerbose > 1) {
          // Can only be done by the debugger!
          tty->print("#%d: ", trace_bcs);
          ps();
        } else {
          frame.print_on(tty, trace_bcs);
        }
        PrintLongFrames--;
        tty->print("%sbci=%d, fp=0x%x, sp=0x%x, thread=0x%x",
                   (frame.is_compiled_frame() ? "Compiled " : ""),
                   bci, frame.fp(), frame.sp(), thread->obj());
      } else {
        tty->print("[%ld] ", trace_bcs);
        if (frame.is_compiled_frame()) {
          tty->print("Compiled ");
        }
        method().print_value_on(tty);
        tty->print_cr(":%d:", bci);
      }
      method().print_bytecodes(tty, bci);
      tty->cr();
    }
    if (TraceBytecodesStop == trace_bcs) {
      tty->print_cr("Stopped at bytecode [%d]", trace_bcs);
#ifdef UNDER_CE
      DebugBreak();
      TraceBytecodesStop+=15; // intervals
#else
      BREAKPOINT;
#endif
    }
    trace_bcs++;
#endif // !defined(PRODUCT) || ENABLE_TTY_TRACE
  }

#ifndef PRODUCT
  void verify_stack() {
    int i = 0;
    JavaFrame frame(Thread::current());
    Frame fr = frame;
    while (true) {
      if (fr.is_entry_frame()) {
        if (Verbose) {
          tty->print_cr("Skipping entry frame... [%d]", i++);
        }
        if (fr.as_EntryFrame().is_first_frame()) {
          break;
        }
        fr.as_EntryFrame().caller_is(fr);
      } else {
        if (Verbose) {
          tty->print_cr("Verifying java frame... [%d]", i++);
        }
        fr.as_JavaFrame().verify();
        fr.as_JavaFrame().caller_is(fr);
      }
    }
  }
#endif // PRODUCT


#if ENABLE_WTK_PROFILER
void jprof_record_thread_switch() {
  WTKProfiler::record_thread_transition();
}

void jprof_record_method_transition() {
  WTKProfiler::record_method_transition(Thread::current());
}
#endif

}
