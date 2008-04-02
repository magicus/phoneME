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

# include "incls/_precompiled.incl"
# include "incls/_Method.cpp.incl"

HANDLE_CHECK(Method, is_method())

int Method::vtable_index() const {
  // Retrieve the vtbale index from the vtable by searching
  InstanceClass::Raw klass = holder();
  ClassInfoDesc *info = (ClassInfoDesc*) klass().class_info();

  OopDesc *this_obj = obj();
  OopDesc **base = info->vtable_base();
  int len = info->_vtable_length;
  for (int index = 0; index < len; index++) {
    if (this_obj != *base) {
      base ++;
    } else {
      return index;
    }
  }

  return -1;
}

#if ENABLE_COMPILER

inline bool Method::resume_compilation(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_ISOLATES
  // IMPL_NOTE: It is not usual to allocate anything in general Heap
  // (not CompilerArea) during compilation, but in this case only
  // a few OopCons objects can be created, and that is considered to be OK
  TaskAllocationContext tmp( _compiler_code_generator->task_id() );
#endif
  bool status = (bool)Compiler::resume_compilation(this JVM_MUST_SUCCEED);
  return status;
}

bool Method::compile(int active_bci, bool resume JVM_TRAPS) {
  if (resume) {
    return resume_compilation(JVM_SINGLE_ARG_MUST_SUCCEED);
  }
  if( Universe::before_main() ) {
    return false;
  }

  if (is_impossible_to_compile()) {
    return false;
  }
  if (code_size() == 0 || is_abstract()) {
    set_impossible_to_compile();
    return false;
  }

  if (has_compiled_code()) {
    return true;
  }

  if (code_size() > MaxMethodToCompile && !GenerateROMImage) {
    if (Verbose) {
      TTY_TRACE(("Method too big to compile: "));
      print_name_on_tty();
      TTY_TRACE_CR((" [%d]", code_size()));
    }
    set_impossible_to_compile();
    return false;
  }

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  if (Arguments::must_check_CompileOnly()) {
    UsingFastOops internal;
    bool dummy;
    Symbol::Fast methodName = get_original_name(dummy);
    InstanceClass::Fast ic = holder();
    Symbol::Fast className = ic().original_name();
    bool is_match = Arguments::must_compile_method(&className, &methodName
                                                   JVM_MUST_SUCCEED);
    if (!is_match) {
      set_impossible_to_compile();
      return false;
    }
  }
  if (Verbose) {
    tty->print("compiling method: (0x%x), ", (int)obj());
    print_name_on(tty);
    tty->print_cr("");
  }
#endif // PRODUCT

  bool status = (bool)Compiler::compile(this, active_bci JVM_MUST_SUCCEED);
  return status;
}
#endif

ReturnOop Method::exception_table() const {
  ReturnOop result;

  if (has_no_exception_table()) {
    result = NULL;
  } else {
    result = obj_field(exception_table_offset());
  }

  if (result == NULL) {
    result = Universe::empty_short_array()->obj();
  }

  return result;
}

ReturnOop Method::compiled_code_unchecked() const {
  int offset = CompiledMethod::entry_offset();
#if ENABLE_THUMB_COMPILER
  // The low bit is set to 0x1 so that BX will automatically switch into
  // THUMB mode.
  offset += 1;
#endif

  ReturnOop result =
      DERIVED(ReturnOop, ((MethodDesc*)obj())->execution_entry(), -offset);

#if USE_INDIRECT_EXECUTION_SENSOR_UPDATE
  GUARANTEE(ENABLE_THUMB_COMPILER, "sanity")
  if ((((int)result) & 0x02) != 0) {
    result = DERIVED(ReturnOop, result,
                     - CompiledMethodDesc::NUMBER_OF_EXECUTION_SENSOR_BYTES);
    GUARANTEE_R(result->is_compiled_method(), "sanity");
  }
#endif

  return result;
}


ReturnOop Method::constants() const {
  // only when actually running, not ROMizing
#if ROMIZING
  if (UseROM && has_compressed_header()) {
    return (ReturnOop)_rom_constant_pool;
  }
#endif

#if defined(AZZERT) && NOT_CURRENTLY_USED
  // IMPL_NOTE: currently this doesn't work during task termination
  if (!GenerateROMImage && (holder_id() != 0xFFFF) && !is_abstract()) {
    InstanceClass::Raw holder_class = holder();
    ConstantPool::Raw class_constants = holder_class().constants();
    ConstantPool::Raw method_constants = obj_field(constants_offset());
    GUARANTEE(class_constants().equals(method_constants()), "Sanity");
  }
#endif
  return obj_field(constants_offset());
}

ReturnOop Method::stackmaps() const {
#if ROMIZING
  if (UseROM && has_no_stackmaps()) {
    return NULL;
  }
#endif
  juint n = uint_field(stackmaps_offset());
  if ((n & 0x01) != 0) {
    // This is a special encoding of stackmaps created by the source
    // romizer to save system ROM footprint. See ROMWriter::stream_method()
    StackmapList::Raw sm = Universe::inlined_stackmaps()->obj();
    sm().uint_field_put(StackmapList::header_size(), (n >> 1));
    GUARANTEE(sm().entry_count() == 1, "sanity");
    GUARANTEE(sm().is_short_map(0), "sanity");
    return sm.obj();
  }
  return (ReturnOop)n;
}

ReturnOop Method::name() const {
  return raw_constants_base()[name_index()];
}

ReturnOop Method::signature() const {
  return raw_constants_base()[signature_index()];
}

jint Method::exception_handler_bci_for(OopDesc* exception_klass,
                                       jint bci JVM_TRAPS) {
  UsingFastOops fast_oops;
  JavaClass::Fast    exception_class = exception_klass;
  ConstantPool::Fast cp = constants();
  TypeArray::Fast    et = exception_table();
  GUARANTEE((et().length() % 4) == 0, "Sanity check");

  for (int i = 0; i < et().length(); i+=4) {
    // Check that the exception occurred within the handler region.
    if (bci >= et().ushort_at(i) && bci < et().ushort_at(i + 1)) {
      // If the catch type index is 0 this handler will catch any type.
      if (et().ushort_at(i + 3) == 0) {
        return et().ushort_at(i + 2);
      }
      // Check that the class of the exception is a subclass of the catch type.
      JavaClass::Raw catch_type =
        cp().klass_at(et().ushort_at(i + 3) JVM_CHECK_(-1));
      if (catch_type.equals(exception_class) ||
          exception_class().is_subtype_of(&catch_type)) {
        return et().ushort_at(i + 2);
      }
    }
  }
  return -1;
}

// Does *any* handler of *any* exception type in this method cover the
// given bci?
bool Method::exception_handler_exists_for(jint bci) {
  AllocationDisabler raw_pointers_used_in_this_function;

  TypeArray::Raw et = exception_table();
  GUARANTEE((et().length() % 4) == 0, "Sanity check");

  jushort *ptr = et().ushort_base_address();
  jushort *end = ptr + et().length();

  for (; ptr < end; ptr += 4) {
    // Check that the exception occurred within the handler region.
    if (bci >= ptr[0] && bci < ptr[1]) {
      return true;
    }
  }
  return false;
}

jint Method::exception_handler_if_first_is_any(jint bci) {
  AllocationDisabler raw_pointers_used_in_this_function;

  TypeArray::Raw et = exception_table();
  GUARANTEE((et().length() % 4) == 0, "Sanity check");

  jushort *ptr = et().ushort_base_address();
  jushort *end = ptr + et().length();

  for (; ptr < end; ptr += 4) {
    // Check that the exception occurred within the handler region.
    if (bci >= ptr[0] && bci < ptr[1]) {
      // If the catch type index is 0 this handler will catch any type.
      if (ptr[3] == 0) {
        return ptr[2];
      } else {
        return -1;
      }
    }
  }
  return -1;
}

jushort Method::get_Java_u2_index_at(jint bci) {
  return Bytes::get_Java_u2((address)field_base(bc_offset_for(bci)));
}

void Method::write_u2_index_at(jint bci, jushort index) {
  Bytes::put_native_u2((address)field_base(bc_offset_for(bci)), index);
}

// Called by compiler to tell if this method may be inlined
bool Method::is_fast_get_accessor() const {
  const address entry = execution_entry();
  if (is_static()) {
    return (entry == (address)shared_fast_getint_static_accessor)   ||
           (entry == (address)shared_fast_getshort_static_accessor) ||
           (entry == (address)shared_fast_getbyte_static_accessor)  ||
           (entry == (address)shared_fast_getchar_static_accessor)  ||
           (entry == (address)shared_fast_getlong_static_accessor);
  } else {
    return (entry == (address)shared_fast_getint_accessor)          ||
           (entry == (address)shared_fast_getshort_accessor)        ||
           (entry == (address)shared_fast_getbyte_accessor)         ||
           (entry == (address)shared_fast_getchar_accessor)         ||
           (entry == (address)shared_fast_getlong_accessor);
  }
}

bool
Method::may_convert_to_fast_get_accessor(BasicType& type, int& offset JVM_TRAPS)
{
  // Check obvious requirements
  if (MakeROMDebuggable) {
    return false; // during romization, if this image should be debuggable
                  // then don't optimize it
  }
  if (_debugger_active) {
    return false; // Else can't single step into these methods
  }
  if (uses_monitors()) {
    return false; // must not be synchronized!
  }
  if (size_of_parameters() != 1) {
    return false;
  }
  if (code_size() != 5) {
    return false; // wrong code size
  }

  // Check the bytecodes
  if (bytecode_at(0) != Bytecodes::_aload_0) {
    return false;
  }
  if (bytecode_at(1) != Bytecodes::_getfield) {
    return false;
  }
  if (bytecode_at(4) < Bytecodes::_ireturn
      || Bytecodes::_areturn < bytecode_at(4)){
    return false;
  }

  jushort index = get_Java_u2_index_at(2);
  return try_resolve_field_access(index, type, offset,
                                  /*is_static*/false, /*is_get*/true
                                  JVM_NO_CHECK_AT_BOTTOM);
}

#if ENABLE_COMPILER && ENABLE_INLINE
bool Method::bytecode_inline_filter(bool& has_field_get, 
                                    int& index JVM_TRAPS) const {
  AllocationDisabler raw_pointers_used_in_this_function;
  has_field_get = false;
  int has_one_return = 0;
  register jubyte *bcptr = (jubyte*)code_base();
  register jubyte *bcend = bcptr + code_size();
  register int bci = 0;
  while (bcptr < bcend) {
    const Bytecodes::Code code = (Bytecodes::Code)(*bcptr);
    GUARANTEE(code >= 0, "sanity: unsigned value");
    const int length = bytecode_length_for(bci);

    switch (code) {
    // byte code may throw exception
      case Bytecodes::_athrow:
      case Bytecodes::_goto_w:
      case Bytecodes::_jsr:
      case Bytecodes::_jsr_w:
      case Bytecodes::_breakpoint:
      case Bytecodes::_aconst_null:
      case Bytecodes::_ldc:
      case Bytecodes::_ldc_w:
      case Bytecodes::_ldc2_w:
      case Bytecodes::_aload_1:
      case Bytecodes::_aload_2:
      case Bytecodes::_aload_3:
      case Bytecodes::_iaload:
      case Bytecodes::_laload:
      case Bytecodes::_faload:
      case Bytecodes::_daload:
      case Bytecodes::_aaload:
      case Bytecodes::_baload:
      case Bytecodes::_caload:
      case Bytecodes::_saload:
      case Bytecodes::_new:
      case Bytecodes::_newarray:
      case Bytecodes::_anewarray:
      case Bytecodes::_arraylength:
      case Bytecodes::_multianewarray:
      case Bytecodes::_checkcast:
      case Bytecodes::_instanceof:     
      case Bytecodes::_putstatic:
      case Bytecodes::_putfield:
      case Bytecodes::_getstatic:
      case Bytecodes::_ladd:
      case Bytecodes::_fadd:
      case Bytecodes::_dadd:
      case Bytecodes::_lsub:
      case Bytecodes::_fsub:
      case Bytecodes::_dsub:
      case Bytecodes::_lmul:
      case Bytecodes::_fmul:
      case Bytecodes::_dmul:
      case Bytecodes::_idiv:
      case Bytecodes::_ldiv:
      case Bytecodes::_fdiv:
      case Bytecodes::_ddiv:
      case Bytecodes::_irem:
      case Bytecodes::_lrem:
      case Bytecodes::_frem:
      case Bytecodes::_drem:
      case Bytecodes::_fast_1_ldc:
      case Bytecodes::_fast_1_ldc_w:
      case Bytecodes::_fast_2_ldc_w:

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
      case Bytecodes::_fast_lgetfield:
      case Bytecodes::_fast_fgetfield:
      case Bytecodes::_fast_dgetfield:
      case Bytecodes::_fast_agetfield:
      case Bytecodes::_fast_cgetfield:
      case Bytecodes::_fast_new:
      case Bytecodes::_fast_init_new:
      case Bytecodes::_fast_anewarray:
      case Bytecodes::_fast_checkcast:
      case Bytecodes::_fast_instanceof:
      case Bytecodes::_fast_igetfield_1:
      case Bytecodes::_fast_agetfield_1:
      case Bytecodes::_aload_0_fast_agetfield_1:
      case Bytecodes::_aload_0_fast_igetfield_1:
#if !ENABLE_CPU_VARIANT
      case Bytecodes::_aload_0_fast_agetfield_4:
      case Bytecodes::_aload_0_fast_igetfield_4:
      case Bytecodes::_aload_0_fast_agetfield_8:
      case Bytecodes::_aload_0_fast_igetfield_8:
#endif //!ENABLE_CPU_VARIANT
   //is leaf method
      case Bytecodes::_invokevirtual:
      case Bytecodes::_invokespecial:
      case Bytecodes::_invokestatic:
      case Bytecodes::_invokeinterface:
      case Bytecodes::_fast_invokevirtual:
      case Bytecodes::_fast_invokestatic:
      case Bytecodes::_fast_init_invokestatic:
      case Bytecodes::_fast_invokeinterface:
      case Bytecodes::_fast_invokenative:
      case Bytecodes::_fast_invokevirtual_final:
      case Bytecodes::_fast_invokespecial:
   // i386 port can go to interpreter for FP instructions 
   // IMPL_NOTE: need to check inlining per platform
      case Bytecodes::_i2f:
      case Bytecodes::_i2d:
      case Bytecodes::_l2f: 
      case Bytecodes::_l2d:
      case Bytecodes::_f2i: 
      case Bytecodes::_f2l:
      case Bytecodes::_f2d:
      case Bytecodes::_d2i: 
      case Bytecodes::_d2l:
      case Bytecodes::_d2f:
        return false;
      case Bytecodes::_getfield:
        has_field_get=true;
        index = get_java_ushort(bci + 1);
        break;
      default:
        break;
    }
    bci   += length;
    bcptr += length;
  }
  GUARANTEE(bcptr == bcend, "sanity");
  return true;
}

bool Method::bytecode_inline_prepass(Attributes& attributes 
                                     JVM_TRAPS) const {
  if (is_native()) {
    return false;
  }

  if (is_impossible_to_compile()) {
    return false;
  }

  if (uses_monitors()) {
    return false;
  }

  if (!is_leaf()) {
    return false;
  }

  if (_debugger_active) {
    return false; // Else can't single step into these methods
  }

  if (code_size() > 13 || code_size() <= 1) {
    return false;
  }

  if (size_of_parameters() > 3) {
    return false;
  }

  if (is_quick_native() || is_fast_get_accessor()) {
    return false;
  }

  if (size_of_parameters() != max_locals()) {
    return false;
  }

  if (uses_monitors()) {
    return false; // must not be synchronized!
  }

  if (is_object_initializer()) return false;

  if (match(Symbols::object_initializer_name(), Symbols::void_signature())) {
    return false;
  }

  TypeArray::Raw my_exception_table = exception_table();
  if (my_exception_table().length() != 0) {
    return false;
  }

  UsingFastOops fast_oops;
  InstanceClass::Fast holder_class = holder();
  if ((&holder_class)->is_subclass_of(Universe::throwable_class())) {
    return false;
  }

  if ((&holder_class)->equals(Universe::throwable_class())) {
    return false;
  }

  bool has_field_get = false;
  int index = 0;

  compute_attributes(attributes JVM_CHECK_0);

  // Note: we don't emit OSR stubs for inlined methods, 
  // so inlining a method with a loop can degrade performance
  if (attributes.has_loops) {
    return false;
  }

  if (!attributes.bytecodes_allow_inlining) {
    return false;
  }

  if (!InlineIfExceptions && attributes.can_throw_exceptions) {
    return false;
  }

  return true;
}
#endif

#if ENABLE_COMPILER && ENABLE_INLINE
// Returns if a method can be shared between tasks
bool Method::is_shared() const {
#if ENABLE_ISOLATES && USE_BINARY_IMAGE_LOADER
  FOREACH_BINARY_IMAGE_IN_CURRENT_TASK(bundle)
    if (bundle->text_contains(obj())) {
      return bundle->is_sharable();
    }
  ENDEACH_BINARY_IMAGE_IN_CURRENT_TASK;
#endif
  return ENABLE_ISOLATES && ROM::is_rom_method(obj());
}

ReturnOop Method::find_callee_record() const {
  AllocationDisabler raw_pointers_used_in_this_function;

  OopCons::Raw table = Task::current()->direct_callers();
  while (table.not_null()) {
    OopCons::Raw record = table().oop();
    GUARANTEE(record.not_null(), "Record must not be null");
    Method::Raw method = record().oop();
    if (this->equals(&method)) {
      return record.obj();
    }
    table = table().next();
  }

  return NULL;
}

void Method::add_direct_caller(const Method * caller JVM_TRAPS) const {
  GUARANTEE(vtable_index() >= 0, "Mo need to track non-virtual methods");
  UsingFastOops fast_oops;
  OopCons::Fast record = find_callee_record();
  OopCons::Fast head;
  OopCons::Fast list;
  OopCons::Fast table;

  if (record.not_null()) {
    head = record().next();

    // Avoid adding the same caller twice
    {
      list = head;
      while (list.not_null()) {
        Method::Raw method = list().oop();
        if (caller->equals(&method)) {
          return;
        }
        list = list().next();
      }
    }

    head = OopCons::cons_up(caller, &head JVM_CHECK);

    record().set_next(&head);
  } else {
    // Note: just reusing null 'record' handle here
    list = OopCons::cons_up(caller, &record JVM_CHECK);

    record = OopCons::cons_up(this, &list JVM_CHECK);

    table = Task::current()->direct_callers();
    table = OopCons::cons_up(&record, &table JVM_CHECK);
    Task::current()->set_direct_callers(&table);
  }
}

void Method::unlink_direct_callers() const {
  GUARANTEE(vtable_index() >= 0, "No need to track non-virtual methods");

  AllocationDisabler raw_pointers_used_in_this_function;

  Method::DirectCallerStream reader(this);

  if (!reader.has_next()) {
    // No direct callers
    return;
  }

  // Iterate over all threads of the current task and deoptimize all frames 
  // with compiled methods of direct callers from stack
  {
    ForAllThreads( thread ) {
#if ENABLE_ISOLATES
      if (thread().task_id() != Task::current_id()) {
        continue;
      }
#endif
      Method::Raw callee;
      for( Frame frame( &thread() );; ) {
        if( frame.is_entry_frame() ) {
          if( frame.as_EntryFrame().is_first_frame() ) break;
          frame.as_EntryFrame().caller_is( frame );
          callee.set_null();
        } else {
          if( frame.as_JavaFrame().is_compiled_frame() ) {
            CompiledMethod::Raw frame_cm = 
              frame.as_JavaFrame().compiled_method();
            
            Method::DirectCallerStream reader(this);
            
            while (reader.has_next()) {
              Method::Raw direct_caller = reader.next();
              if (direct_caller().has_compiled_code()) {            
                CompiledMethod::Raw caller_cm = 
                  direct_caller().compiled_code();
                if (caller_cm.equals(&frame_cm)) {
                  if (TraceMethodInlining) {
                    tty->print("    Deoptimized %stopmost frame: ",
                               callee.not_null() ? "non-" : "");
                    direct_caller().print_name_on_tty();
                    tty->cr();
                  }
                  frame.as_JavaFrame().deoptimize(&callee);

                  break;
                }
              }
            }
          }
          callee = frame.as_JavaFrame().method();
          frame.as_JavaFrame().caller_is(frame);
        }
      }
    }
  }

  // Compiler should not initialize any classes.
  // If we get here we should track current compilation as well.
  GUARANTEE(!Compiler::is_active(), 
            "Should not get here during compilation");

  // Unlink and remove compiled code for all direct callers
  {
    Method::Raw current_compiling;

    const CodeGenerator* gen = _compiler_code_generator;
    if( gen ) {
      current_compiling = gen->compiled_method()->method();
    }

    GUARANTEE(reader.has_next(), "Must have one");

    while (reader.has_next()) {
      Method::Raw direct_caller = reader.next();

      if (direct_caller().has_compiled_code()) {
        direct_caller().unlink_compiled_code();
        
        if (TraceMethodInlining) {
          tty->print("    Decompiled method: ");
          direct_caller().print_name_on_tty();
          tty->cr();
        }
      } else if (current_compiling.equals(&direct_caller)) {
        Compiler::abort_suspended_compilation();

        if (TraceMethodInlining) {
          tty->print("    Compilation aborted: ");
          direct_caller().print_name_on_tty();
          tty->cr();
        }
      }
    }
  }

  // Remove all direct callers of this method from the table
  {
    OopCons::Raw prev;
    OopCons::Raw table = Task::current()->direct_callers();
    while (table.not_null()) {
      OopCons::Raw record = table().oop();
      GUARANTEE(record.not_null(), "Record must not be null");
      Method::Raw method = record().oop();
      if (this->equals(&method)) {
        OopCons::Raw next = table().next();
        if (prev.not_null()) {
          prev().set_next(&next);
        } else {
          Task::current()->set_direct_callers(&next);
        }
        return;
      }
      prev = table;
      table = table().next();
    }
  }
}
#endif

bool Method::try_resolve_field_access(int index, BasicType& type,
                                      int& offset, bool is_static,
                                      bool is_get JVM_TRAPS) {
  UsingFastOops fast_oops;
  InstanceClass::Fast sender_class = holder();
  ConstantPool::Fast cp = constants();
  InstanceClass::Fast dummy_declaring_class;

#if ENABLE_INLINE
  // When inlining, the callee may belong to not yet verified class,
  // we should not resolve any fields in its constant pool.
  if (!sender_class().is_verified()) {
    return false;
  }
#endif

  ConstantPool::suspend_class_loading();
  {
    type = cp().field_type_at(index, offset, is_static, is_get,
                              &sender_class, &dummy_declaring_class
                              JVM_NO_CHECK);
  }
  ConstantPool::resume_class_loading();

  if (CURRENT_HAS_PENDING_EXCEPTION) {
    // This method refers to an invalid field, or a field in a class that
    // has not yet been loaded.
    Thread::clear_current_pending_exception();
    return false;
  }

  return true;
}

bool Method::is_object_initializer() const {
  return Symbols::object_initializer_name()->obj() == name();
}

void Method::check_access_by(InstanceClass* sender_class,
                             InstanceClass* static_receiver_class,
                             FailureMode fail_mode JVM_TRAPS) {

  if (can_access_by(sender_class, static_receiver_class)) {
    return;
  }
  Throw::illegal_access(fail_mode JVM_NO_CHECK_AT_BOTTOM);
}

bool Method::can_access_by(InstanceClass* sender_class,
                           InstanceClass* static_receiver_class) {
  if (is_public()) {
    return true;
  }
  InstanceClass::Raw holder_class = holder();
  if (holder_class.equals(sender_class)) {
    return true;
  }
  if (!is_private()) {
    if (holder_class().is_same_class_package(sender_class)) {
      return true;
    }
    if (is_protected()) {
      if (sender_class->is_subclass_of(&holder_class)) {
        if (static_receiver_class->equals(sender_class) ||
            static_receiver_class->is_subclass_of(sender_class) ||
            sender_class->is_subclass_of(static_receiver_class)) {
          return true;
        }
      }
    }
  }

  return false;
}

bool Method::match(Symbol* name, Symbol* signature) const {
  return ((MethodDesc*)obj())->match(name->obj(), signature->obj());
}

int Method::itable_index() const {
  InstanceClass::Raw ic = holder();
  ObjArray::Raw methods = ic().methods();
  for (int index = 0; index < methods().length(); index++) {
    if (this->equals(methods().obj_at(index))) {
      return index;
    }
  }
  SHOULD_NOT_REACH_HERE();
  return -1;
}

bool Method::is_vanilla_constructor() const {
  AllocationDisabler raw_pointers_used_in_this_function;

  // Returns true if this is a vanilla constructor, i.e. an "<init>" "()V"
  // method which only calls the superclass vanilla constructor and possibly
  // does stores of zero constants to local fields:
  //
  //   aload_0
  //   invokespecial
  //   indexbyte1
  //   indexbyte2
  //
  // followed by an (optional) sequence of:
  //
  //   aload_0
  //   aconst_null / iconst_0 / fconst_0 / dconst_0
  //   putfield
  //   indexbyte1
  //   indexbyte2
  //
  // followed by:
  //
  //   return
  GUARANTEE(match(Symbols::object_initializer_name(), Symbols::void_signature()),  "Should only be called for default constructors");

  int size = code_size();
  // Check if size match
  if (size == 0 || size % 5 != 0) {
    return false;
  }
  jubyte *code = (jubyte*)code_base();
  jubyte *lastcode = code + size - 1;

  if ( code[0]     != Bytecodes::_aload_0
    || code[1]     != Bytecodes::_invokespecial
    || lastcode[0] != Bytecodes::_return) {
    // Does not call superclass default constructor
    return false;
  }
  // Check optional sequence
  for (code += 4; code < lastcode; code += 5) {
    if (code[0] != Bytecodes::_aload_0) {
      return false;
    }
    if (!Bytecodes::is_zero_const(Bytecodes::cast(code[1]))) {
      return false;
    }
    if (code[2] != Bytecodes::_putfield) {
      return false;
    }
  }
  return true;
}

// size of parameters, in # of words (longs count as two words)
jushort Method::compute_size_of_parameters() {
  Signature::Raw sig = signature();

  int param_size = sig().parameter_word_size(is_static());
  GUARANTEE(param_size >= 0 && param_size <= SIZE_OF_PARAMETERS_MASK, 
    "check for overflow");

  int ret_size = word_size_for(sig().return_type(true));
  GUARANTEE(0 <= ret_size && ret_size <= 2, "sanity");

  int method_attributes = param_size;

  if (ret_size != 0) {
#if USE_FP_RESULT_IN_VFP_REGISTER
    if (!is_native() && 
        ((sig().return_type(true) == T_FLOAT) || 
         (sig().return_type(true) == T_DOUBLE))) {
      if (ret_size == 1) {      
        method_attributes |= (FP_SINGLE << RESULT_STORAGE_TYPE_SHIFT);
      } else {
        method_attributes |= (FP_DOUBLE << RESULT_STORAGE_TYPE_SHIFT);      
      }
    } else 
#endif
    {
      if (ret_size == 1) {      
        method_attributes |= (SINGLE << RESULT_STORAGE_TYPE_SHIFT);
      } else {
        method_attributes |= (DOUBLE << RESULT_STORAGE_TYPE_SHIFT);      
      }
    }
  }

  GUARANTEE(0 <= method_attributes && method_attributes <= 0xffff, "sanity");
  set_method_attributes(method_attributes);

  return (jushort)param_size;
}


/**
 * (1) Check to make sure this method contains only valid bytecodes.
 * (2) Fills in the HAS_INVOKE_BYTECODES and HAS_MONITOR_BYTECODES flags.
 */
void Method::check_bytecodes(JVM_SINGLE_ARG_TRAPS) {
  if (is_native() || is_abstract()) {
    // We've generated the code ourselves.  Nothing to check.
    return;
  }

  {
    // This is a hot loop, so we're using raw pointers here to help C++
    // compiler generate better code. If a GC happens we're
    // in deep trouble!

    AllocationDisabler raw_pointers_used_in_this_block;

    bool has_monitor_bytecodes = false;
    bool has_invoke_bytecodes  = false;
    int codesize = code_size();
    jubyte *bcptr = (jubyte*)code_base();
    jubyte *bcstart = bcptr;
    jubyte *bcend = bcptr + codesize;
    int bci;
    unsigned int numlocks = 0;

    // This is a hot loop used during class loading. the conditions checked
    // inside this loop are carefully ordered so that most bytecodes can
    // be processed
    for (bci = 0; bcptr < bcend;) {
      Bytecodes::Code code = (Bytecodes::Code)(*bcptr);
      int bytecode_length;

      GUARANTEE(code >= 0, "sanity: unsigned value");

      if (code < Bytecodes::_tableswitch) {
        // This is the fast path in this loop. Most valid bytecodes would
        // hit this branch. Note we don't update bci here.
        if ((bytecode_length = Bytecodes::length_for(code)) < 0) {
          // bytecode not defined
          goto error;
        }

        // only the tableswitch and lookupswitch bytecodes have len=0
        GUARANTEE(bytecode_length > 0,
                  "special bytecode not handled in this block");

        // make sure we don't mis-use an out-of-sync variable.
        AZZERT_ONLY(bci = -12345678);

        bcptr += bytecode_length;
        continue;
      }

      // This is the slow path. We need to sync up bci with bcptr
      bci = (bcptr - bcstart);

      if (code >= Bytecodes::_breakpoint ||!Bytecodes::is_defined(code)) {
        // User code should never have breakpoint or any of the fast bytecodes
        goto error;
      }
      else if (code == Bytecodes::_wide) {  // 0xc4
        code = bytecode_at(bci + 1);
        if (!Bytecodes::is_defined(code)) {
          goto error;
        }
        if ((bytecode_length = Bytecodes::wide_length_for(code)) == 0) {
          goto error_wide;
        }
      }
      else {
        switch (code) {
        case Bytecodes::_monitorenter:     // 0xc2
          numlocks++;
        case Bytecodes::_monitorexit:      // 0xc3
          has_monitor_bytecodes = true;
          break;
        case Bytecodes::_invokeinterface:  // 0xb9
        case Bytecodes::_invokestatic:     // 0xb8
        case Bytecodes::_invokespecial:    // 0xb7
        case Bytecodes::_invokevirtual:    // 0xb6
          has_invoke_bytecodes = true;
          break;
        case Bytecodes::_lookupswitch:     // 0xab
        case Bytecodes::_tableswitch:      // 0xaa
          {
            int a_bci = align_size_up(bci + 1, wordSize);
            //CR6538939: only zero-byte padding is allowed
            for (int offset = bci; ++offset < a_bci; ) {
              if ((jubyte)bytecode_at_raw(offset) != 0) {
                goto error;
              }
            }
            int fields;
            if (code == Bytecodes::_tableswitch) {
              int raw_lo = get_native_aligned_int(a_bci + wordSize);
              int raw_hi = get_native_aligned_int(a_bci + 2 * wordSize);
              if (Bytes::is_Java_byte_ordering_different()) {
                raw_hi = Bytes::swap_u4(raw_hi);
                raw_lo = Bytes::swap_u4(raw_lo);
              }
              fields = 3 + raw_hi - raw_lo + 1;
            } else {
              int raw_npairs = get_native_aligned_int(a_bci + wordSize);
              if (Bytes::is_Java_byte_ordering_different()) {
                raw_npairs = Bytes::swap_u4(raw_npairs);
              }
              fields = 2 + 2 * raw_npairs;
            }
            int end = a_bci + fields * wordSize;
            // fields can look negative if is is really large.
            if (end >= codesize || fields < 0 || fields > codesize) {
              goto error;
            }
            if (Bytes::is_Java_byte_ordering_different()
                  && ENABLE_NATIVE_ORDER_REWRITING) {
              for (int offset = a_bci; offset < end; offset += wordSize) {
                int old = get_native_aligned_int(offset);
                put_native_aligned_int(offset, Bytes::swap_u4(old));
              }
            }
          }
          break;
        }
        if ((bytecode_length = bytecode_length_for(bci)) == -1) {
          goto error;
        }
      }
      bci += bytecode_length;
      bcptr += bytecode_length;
    }

    if (bci > codesize) {
      goto error;
    }

    AccessFlags flags = access_flags();
    if (has_monitor_bytecodes) {
      GUARANTEE((numlocks *
                 ((BytesPerWord + StackLock::size()) / sizeof(jobject)) +
                (unsigned int)max_execution_stack_count()) < 0xffff,
                "max lock size + max stack too big for unsigned short");
      if ((numlocks * ((BytesPerWord + StackLock::size()) / sizeof(jobject)) +
                       (unsigned int)max_execution_stack_count()) > 0xffff) {
        // Too big to fit into upper end of unsigned short
        Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW);
      }
      set_max_execution_stack_count((jushort)(numlocks *
                      ((BytesPerWord + StackLock::size()) / sizeof(jobject)) +
                      (unsigned int)max_execution_stack_count()));
      flags.set_has_monitor_bytecodes();
    }
    if (has_invoke_bytecodes) {
      flags.set_has_invoke_bytecodes();
    }
    set_access_flags(flags);
  }
  return;

error:
  Throw::class_format_error(illegal_code JVM_THROW);

error_wide:
  Throw::class_format_error(illegal_wide_code JVM_THROW);
}

#if ENABLE_ROM_GENERATOR
ReturnOop Method::create_other_endianness(JVM_SINGLE_ARG_TRAPS) {
  int object_size = this->object_size();
  OopDesc* raw_result = ObjectHeap::allocate(object_size JVM_ZCHECK_0(raw_result));
  jvm_memcpy(raw_result, this->obj(), object_size);
  Method::Raw result = raw_result;

  int bytecode_length;
  for (jushort bci = 0; bci != code_size(); bci += bytecode_length) {
    Bytecodes::Code code = bytecode_at(bci);
    if (code == Bytecodes::_wide) {
      code = bytecode_at(bci + 1);
      bytecode_length = Bytecodes::wide_length_for(code);
    } else {
      bytecode_length = Bytecodes::length_for(code);
      switch(code) {
        case Bytecodes::_tableswitch:
        case Bytecodes::_lookupswitch: {
          int a_bci = align_size_up(bci + 1, wordSize);
          int fields;
          if (code == Bytecodes::_tableswitch) {
            int low  = get_java_switch_int(a_bci + 4);
            int high = get_java_switch_int(a_bci + 8);
            fields = 3 + high - low + 1;
          } else {
            int num_of_pairs  = get_java_switch_int(a_bci + 4);
            fields = 2 + 2 * num_of_pairs;
          }
          int end = a_bci + fields * wordSize;
          for (int offset = a_bci; offset < end; offset += wordSize) {
            int old = get_native_aligned_int(offset);
            result().put_native_aligned_int(offset, Bytes::swap_u4(old));
          }
          bytecode_length = end - bci;
          break;
        }
#if !ENABLE_CPU_VARIANT
        case Bytecodes::_init_static_array: {
          bytecode_length = Bytecodes::wide_length_for(this, bci, code);
          int size_factor = 1 << get_ubyte(bci + 1);
          int size = get_native_ushort(bci + 2);
          result().put_native_ushort(bci + 2, Bytes::swap_u2(size));
          int i;
          switch(size_factor) {
            case(1)://do nothing
              break;
            case(2):
              for (i= 0; i < size; i++) {
                int old = get_native_ushort(bci + 4 + 2*i);
                result().put_native_ushort(bci + 4 + 2*i, Bytes::swap_u2(old));
              }
              break;
           case(4):
              for (i= 0; i < size; i++) {
                int old = get_native_uint(bci + 4 + 4*i);
                result().put_native_uint(bci + 4 + 4*i, Bytes::swap_u4(old));
              }
              break;
           case(8):
              for (i= 0; i < size; i++) {
                int old1 = get_native_uint(bci + 4 + 8*i);
                int old2 = get_native_uint(bci + 8 + 8*i);
                result().put_native_uint(bci + 4 + 8*i, Bytes::swap_u4(old2));
                result().put_native_uint(bci + 8 + 8*i, Bytes::swap_u4(old1));
              }
              break;
           default:
             SHOULD_NOT_REACH_HERE();
          }
          break;
        }
#endif //!ENABLE_CPU_VARIANT
        case Bytecodes::_fast_bputfield: // fall through
        case Bytecodes::_fast_sputfield: // fall through
        case Bytecodes::_fast_iputfield: // fall through
        case Bytecodes::_fast_lputfield: // fall through
        case Bytecodes::_fast_fputfield: // fall through
        case Bytecodes::_fast_dputfield: // fall through
        case Bytecodes::_fast_aputfield:
        case Bytecodes::_fast_bgetfield: // fall through
        case Bytecodes::_fast_sgetfield: // fall through
        case Bytecodes::_fast_igetfield: // fall through
        case Bytecodes::_fast_lgetfield: // fall through
        case Bytecodes::_fast_fgetfield: // fall through
        case Bytecodes::_fast_dgetfield: // fall through
        case Bytecodes::_fast_agetfield: // fall through
        case Bytecodes::_fast_cgetfield: {
          int old = get_native_ushort(bci + 1);
          result().put_native_ushort(bci+1, Bytes::swap_u2(old));
          break;
        }
      }
    }
  }
  return result;
}
#endif

#if !ROMIZED_PRODUCT
void Method::set_fixed_entry(address entry) {
  set_execution_entry(entry);
  GUARANTEE(is_impossible_to_compile(), "sanity");
}
#endif

void Method::set_impossible_to_compile() {
  set_fixed_interpreter_entry();
}

void Method::iterate(int begin, int end, BytecodeClosure* blk JVM_TRAPS) {
  int bci = begin;
  for(; bci < end; bci = next_bci(bci) ) {
    // Note: we may potentially iterate a bytecode whose end is past <end>,
    // but this would be caught by the illegal_code() check below. That
    // should make the bytecode verifier happy.
    iterate_bytecode(bci, blk, bytecode_at(bci) JVM_CHECK);
  }

  if (bci > end) {
    blk->illegal_code(JVM_SINGLE_ARG_THROW);
  }
}

#if ENABLE_CPU_VARIANT && ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
void Method::fix_long_operations() {  
  //-1 below is small performance optimizations 
  //we couldn't call get_ubyte(bci+1) for the last bytecode
  //but also we don't have to do it.
  const int method_size = code_size() - 1;
  for( int bci = 0; bci < method_size; bci = next_bci( bci ) ) {
    const jubyte idx = get_ubyte(bci+1);
    if( idx != 0xff) continue;
    Bytecodes::Code new_code;
    switch( bytecode_at(bci) ) {
      default: continue;
      #define REPLACE_BYTECODE(code)\
        case Bytecodes::_##code: new_code = Bytecodes::_##code##_safe; break
      REPLACE_BYTECODE(lload );
      REPLACE_BYTECODE(lstore);
#if ENABLE_FLOAT
      REPLACE_BYTECODE(dload );
      REPLACE_BYTECODE(dstore);
#endif
      #undef REPLACE_BYTECODE
    }
    bytecode_at_put_raw(bci, new_code);
  }
}
#endif 

void Method::iterate_push_constant_1(int i, BytecodeClosure* blk JVM_TRAPS) {
  UsingFastOops fast_oops;
  ConstantPool::Fast c = constants();
  if (i >= c().length()) {
    blk->illegal_code(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return;
  }
  jubyte tag = c().tag_value_at(i);
  if (ConstantTag::is_int(tag)) {
    blk->push_int(c().int_at(i) JVM_NO_CHECK_AT_BOTTOM);
  } else if (ConstantTag::is_float(tag))  {
    blk->push_float(c().float_at(i) JVM_NO_CHECK_AT_BOTTOM);
  } else if (ConstantTag::is_string(tag) ||
             ConstantTag::is_unresolved_string(tag)) {
    UsingFastOops internal;
    Oop::Fast string = c().string_at(i JVM_CHECK);
    blk->push_obj(&string JVM_NO_CHECK_AT_BOTTOM);
  } else {
    blk->illegal_code(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
}

void Method::iterate_push_constant_2(int i, BytecodeClosure* blk JVM_TRAPS) {
  UsingFastOops fast_oops;
  ConstantPool::Fast c = constants();
  if (i + 1 >= c().length()) {
    blk->illegal_code(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return;
  }
  jubyte tag = c().tag_value_at(i);
  if (ConstantTag::is_long(tag)) {
    blk->push_long(c().long_at(i) JVM_NO_CHECK_AT_BOTTOM);
  } else if (ConstantTag::is_double(tag)) {
    blk->push_double(c().double_at(i) JVM_NO_CHECK_AT_BOTTOM);
  } else {
    blk->illegal_code(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
}

void Method::iterate_bytecode(int bci, BytecodeClosure* blk,
                              Bytecodes::Code code JVM_TRAPS)
{
  static const BasicType array_op_types[]  = {
    T_INT,  T_LONG,  T_FLOAT,  T_DOUBLE, T_OBJECT, T_BYTE,   T_CHAR,   T_SHORT
  };
  static const BasicType field_op_types[]  = {
    T_BYTE, T_SHORT, T_INT,    T_LONG,   T_FLOAT,  T_DOUBLE, T_OBJECT, T_CHAR
  };

  blk->set_bytecode(bci);
  blk->bytecode_prolog(JVM_SINGLE_ARG_CHECK);

  /*
   * The bytecodes are sorted in reversed order of frequency, so
   * that it can generate the best code sequence on Thumb (which has
   * a limit of 1024 in immediate branch offset. The ordering has no effect
   * on ARM.
   */

  switch (code) {

    case Bytecodes::_ldc_w:
      iterate_push_constant_1(get_java_ushort(bci+1), blk JVM_NO_CHECK);
      break;
    case Bytecodes::_ldc2_w:
      iterate_push_constant_2(get_java_ushort(bci+1), blk JVM_NO_CHECK);
      break;

    case Bytecodes::_aconst_null: {
      Oop::Raw obj;
      blk->push_obj(&obj JVM_NO_CHECK);
      break;
    }

    case Bytecodes::_iconst_m1        : // fall
    case Bytecodes::_iconst_2         : // fall
    case Bytecodes::_iconst_3         : // fall
    case Bytecodes::_iconst_4         : // fall
    case Bytecodes::_iconst_5         :
      blk->push_int(code - Bytecodes::_iconst_0 JVM_NO_CHECK);
      break;

    case Bytecodes::_lconst_0         :
      blk->push_long(0 JVM_NO_CHECK);
      break;
    case Bytecodes::_lconst_1         :
      blk->push_long(1 JVM_NO_CHECK);
      break;
    case Bytecodes::_fconst_0         :
      blk->push_float(0.0F JVM_NO_CHECK);
      break;
    case Bytecodes::_fconst_1         :
      blk->push_float(1.0F JVM_NO_CHECK);
      break;
    case Bytecodes::_fconst_2         :
      blk->push_float(2.0F JVM_NO_CHECK);
      break;
    case Bytecodes::_dconst_0         :
      blk->push_double(0.0 JVM_NO_CHECK);
      break;
    case Bytecodes::_dconst_1         :
      blk->push_double(1.0 JVM_NO_CHECK);
      break;

    case Bytecodes::_iload_0          : // fall
    case Bytecodes::_iload_1          : // fall
    case Bytecodes::_iload_2          : // fall
    case Bytecodes::_iload_3          :
      blk->load_local(T_INT, code - Bytecodes::_iload_0 JVM_NO_CHECK);
      break;

    case Bytecodes::_lload            :
#if ENABLE_CPU_VARIANT && ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
    case Bytecodes::_lload_safe       :
#endif
      blk->load_local(T_LONG, get_ubyte(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_lload_0          : // fall
    case Bytecodes::_lload_1          : // fall
    case Bytecodes::_lload_2          : // fall
    case Bytecodes::_lload_3          :
      blk->load_local(T_LONG, code - Bytecodes::_lload_0 JVM_NO_CHECK);
      break;

    case Bytecodes::_fload            :
      blk->load_local(T_FLOAT, get_ubyte(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_fload_0          : // fall
    case Bytecodes::_fload_1          : // fall
    case Bytecodes::_fload_2          : // fall
    case Bytecodes::_fload_3          :
      blk->load_local(T_FLOAT, code - Bytecodes::_fload_0 JVM_NO_CHECK);
      break;

    case Bytecodes::_dload            :
#if ENABLE_CPU_VARIANT && ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
    case Bytecodes::_dload_safe       :
#endif
      blk->load_local(T_DOUBLE, get_ubyte(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_dload_0          : // fall
    case Bytecodes::_dload_1          : // fall
    case Bytecodes::_dload_2          : // fall
    case Bytecodes::_dload_3          :
      blk->load_local(T_DOUBLE, code - Bytecodes::_dload_0 JVM_NO_CHECK);
      break;

    case Bytecodes::_aload_2          : // fall
    case Bytecodes::_aload_3          :
      blk->load_local(T_OBJECT, code - Bytecodes::_aload_0 JVM_NO_CHECK);
      break;

    case Bytecodes::_istore_0         : // fall
    case Bytecodes::_istore_1         : // fall
    case Bytecodes::_istore_2         : // fall
    case Bytecodes::_istore_3         :
      blk->store_local(T_INT, code - Bytecodes::_istore_0 JVM_NO_CHECK);
      break;

    case Bytecodes::_lstore           :
#if ENABLE_CPU_VARIANT && ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
    case Bytecodes::_lstore_safe      :
#endif
      blk->store_local(T_LONG, get_ubyte(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_lstore_0         : // fall
    case Bytecodes::_lstore_1         : // fall
    case Bytecodes::_lstore_2         : // fall
    case Bytecodes::_lstore_3         :
      blk->store_local(T_LONG, code - Bytecodes::_lstore_0 JVM_NO_CHECK);
      break;

    case Bytecodes::_fstore           :
      blk->store_local(T_FLOAT, get_ubyte(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_fstore_0         : // fall
    case Bytecodes::_fstore_1         : // fall
    case Bytecodes::_fstore_2         : // fall
    case Bytecodes::_fstore_3         :
      blk->store_local(T_FLOAT, code - Bytecodes::_fstore_0 JVM_NO_CHECK);
      break;

    case Bytecodes::_dstore           :
#if ENABLE_CPU_VARIANT && ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
    case Bytecodes::_dstore_safe      :
#endif
      blk->store_local(T_DOUBLE, get_ubyte(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_dstore_0         : // fall
    case Bytecodes::_dstore_1         : // fall
    case Bytecodes::_dstore_2         : // fall
    case Bytecodes::_dstore_3         :
      blk->store_local(T_DOUBLE, code - Bytecodes::_dstore_0 JVM_NO_CHECK);
      break;

    case Bytecodes::_astore           :
      blk->store_local(T_OBJECT, get_ubyte(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_astore_0         : // fall
    case Bytecodes::_astore_1         : // fall
    case Bytecodes::_astore_2         : // fall
    case Bytecodes::_astore_3         :
      blk->store_local(T_OBJECT, code - Bytecodes::_astore_0 JVM_NO_CHECK);
      break;

    case Bytecodes::_iinc: {
      const int index = get_ubyte(bci+1);
      const jint offset = get_byte(bci+2);
      blk->increment_local_int(index, offset JVM_NO_CHECK);
      break;
    }

    case Bytecodes::_newarray:
      blk->new_basic_array(get_ubyte(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_anewarray:
      blk->new_object_array(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;

    case Bytecodes::_multianewarray: {
      const int klass_index = get_java_ushort(bci + 1);
      const int nof_dims = get_ubyte(bci+3);
      blk->new_multi_array(klass_index, nof_dims JVM_NO_CHECK);
      break;
    }

    case Bytecodes::_arraylength     :
      blk->array_length(JVM_SINGLE_ARG_NO_CHECK);
      break;

    case Bytecodes::_iaload          : // fall
    case Bytecodes::_laload          : // fall
    case Bytecodes::_faload          : // fall
    case Bytecodes::_daload          : // fall
    case Bytecodes::_aaload          : // fall
    case Bytecodes::_baload          : // fall
    case Bytecodes::_caload          : // fall
    case Bytecodes::_saload          :
      blk->load_array(array_op_types[code - Bytecodes::_iaload] JVM_NO_CHECK);
      break;

    case Bytecodes::_lastore         : // fall
    case Bytecodes::_fastore         : // fall
    case Bytecodes::_dastore         : // fall
    case Bytecodes::_aastore         : // fall
    case Bytecodes::_bastore         : // fall
    case Bytecodes::_castore         : // fall
    case Bytecodes::_sastore         :
      blk->store_array(array_op_types[code - Bytecodes::_iastore] JVM_NO_CHECK);
      break;

    case Bytecodes::_nop             :
      blk->nop(JVM_SINGLE_ARG_NO_CHECK);
      break;
    case Bytecodes::_pop             :
      blk->pop(JVM_SINGLE_ARG_NO_CHECK);
      break;
    case Bytecodes::_pop_and_npe_if_null:
      blk->pop_and_npe_if_null(JVM_SINGLE_ARG_NO_CHECK);
      break;
#if !ENABLE_CPU_VARIANT
    case Bytecodes::_init_static_array:
      blk->init_static_array(JVM_SINGLE_ARG_NO_CHECK);
      break;
#endif
    case Bytecodes::_pop2            :
      blk->pop2(JVM_SINGLE_ARG_NO_CHECK);
      break;
    case Bytecodes::_dup2            :
      blk->dup2(JVM_SINGLE_ARG_NO_CHECK);
      break;
    case Bytecodes::_dup_x1          :
      blk->dup_x1(JVM_SINGLE_ARG_NO_CHECK);
      break;
    case Bytecodes::_dup2_x1         :
      blk->dup2_x1(JVM_SINGLE_ARG_NO_CHECK);
      break;
    case Bytecodes::_dup_x2          :
      blk->dup_x2(JVM_SINGLE_ARG_NO_CHECK);
      break;
    case Bytecodes::_dup2_x2         :
      blk->dup2_x2(JVM_SINGLE_ARG_NO_CHECK);
      break;
    case Bytecodes::_swap            :
      blk->swap (JVM_SINGLE_ARG_NO_CHECK);
      break;

    case Bytecodes::_ladd:
    case Bytecodes::_fadd:
    case Bytecodes::_dadd:
    case Bytecodes::_isub:
    case Bytecodes::_lsub:
    case Bytecodes::_fsub:
    case Bytecodes::_dsub:
    case Bytecodes::_imul:
    case Bytecodes::_lmul:
    case Bytecodes::_fmul:
    case Bytecodes::_dmul:
    case Bytecodes::_idiv:
    case Bytecodes::_ldiv:
    case Bytecodes::_fdiv:
    case Bytecodes::_ddiv:
    case Bytecodes::_irem:
    case Bytecodes::_lrem:
    case Bytecodes::_frem:
    case Bytecodes::_drem:
    case Bytecodes::_ishl:
    case Bytecodes::_ishr:
    case Bytecodes::_iushr:
    case Bytecodes::_lshl:
    case Bytecodes::_lshr:
    case Bytecodes::_lushr:
    case Bytecodes::_iand:
    case Bytecodes::_land:
    case Bytecodes::_ior:
    case Bytecodes::_lor:
    case Bytecodes::_ixor:
    case Bytecodes::_lxor: {
      #define BINARY_OP_TYPE(a, b, c, d) c, d
      static const jubyte binary_op_types[] = {
        BINARY_OP_TYPE(0x60, iadd,  T_INT,     BytecodeClosure::bin_add),
        BINARY_OP_TYPE(0x61, ladd,  T_LONG,    BytecodeClosure::bin_add),
        BINARY_OP_TYPE(0x62, fadd,  T_FLOAT,   BytecodeClosure::bin_add),
        BINARY_OP_TYPE(0x63, dadd,  T_DOUBLE,  BytecodeClosure::bin_add),
        BINARY_OP_TYPE(0x64, isub,  T_INT,     BytecodeClosure::bin_sub),
        BINARY_OP_TYPE(0x65, lsub,  T_LONG,    BytecodeClosure::bin_sub),
        BINARY_OP_TYPE(0x66, fsub,  T_FLOAT,   BytecodeClosure::bin_sub),
        BINARY_OP_TYPE(0x67, dsub,  T_DOUBLE,  BytecodeClosure::bin_sub),
        BINARY_OP_TYPE(0x68, imul,  T_INT,     BytecodeClosure::bin_mul),
        BINARY_OP_TYPE(0x69, lmul,  T_LONG,    BytecodeClosure::bin_mul),
        BINARY_OP_TYPE(0x6a, fmul,  T_FLOAT,   BytecodeClosure::bin_mul),
        BINARY_OP_TYPE(0x6b, dmul,  T_DOUBLE,  BytecodeClosure::bin_mul),
        BINARY_OP_TYPE(0x6c, idiv,  T_INT,     BytecodeClosure::bin_div),
        BINARY_OP_TYPE(0x6d, ldiv,  T_LONG,    BytecodeClosure::bin_div),
        BINARY_OP_TYPE(0x6e, fdiv,  T_FLOAT,   BytecodeClosure::bin_div),
        BINARY_OP_TYPE(0x6f, ddiv,  T_DOUBLE,  BytecodeClosure::bin_div),
        BINARY_OP_TYPE(0x70, irem,  T_INT,     BytecodeClosure::bin_rem),
        BINARY_OP_TYPE(0x71, lrem,  T_LONG,    BytecodeClosure::bin_rem),
        BINARY_OP_TYPE(0x72, frem,  T_FLOAT,   BytecodeClosure::bin_rem),
        BINARY_OP_TYPE(0x73, drem,  T_DOUBLE,  BytecodeClosure::bin_rem),
        BINARY_OP_TYPE(0x74, ineg,  T_ILLEGAL, 0),
        BINARY_OP_TYPE(0x75, lneg,  T_ILLEGAL, 0),
        BINARY_OP_TYPE(0x76, fneg,  T_ILLEGAL, 0),
        BINARY_OP_TYPE(0x77, dneg,  T_ILLEGAL, 0),
        BINARY_OP_TYPE(0x78, ishl,  T_INT,     BytecodeClosure::bin_shl),
        BINARY_OP_TYPE(0x79, lshl,  T_LONG,    BytecodeClosure::bin_shl),
        BINARY_OP_TYPE(0x7a, ishr,  T_INT,     BytecodeClosure::bin_shr),
        BINARY_OP_TYPE(0x7b, lshr,  T_LONG,    BytecodeClosure::bin_shr),
        BINARY_OP_TYPE(0x7c, iushr, T_INT,     BytecodeClosure::bin_ushr),
        BINARY_OP_TYPE(0x7d, lushr, T_LONG,    BytecodeClosure::bin_ushr),
        BINARY_OP_TYPE(0x7e, iand,  T_INT,     BytecodeClosure::bin_and),
        BINARY_OP_TYPE(0x7f, land,  T_LONG,    BytecodeClosure::bin_and),
        BINARY_OP_TYPE(0x80, ior,   T_INT,     BytecodeClosure::bin_or),
        BINARY_OP_TYPE(0x81, lor,   T_LONG,    BytecodeClosure::bin_or),
        BINARY_OP_TYPE(0x82, ixor,  T_INT,     BytecodeClosure::bin_xor),
        BINARY_OP_TYPE(0x83, lxor,  T_LONG,    BytecodeClosure::bin_xor)
      };
      #undef BINARY_OP_TYPE

      const int i = (code - Bytecodes::_iadd) * 2;
      blk->binary(BasicType(binary_op_types[i]),
                  BytecodeClosure::binary_op(binary_op_types[i+1])
                  JVM_NO_CHECK);
      break;
    }

    case Bytecodes::_ineg:
    case Bytecodes::_lneg:
    case Bytecodes::_fneg:
    case Bytecodes::_dneg: {
      static const jubyte table[] = {T_INT, T_LONG, T_FLOAT, T_DOUBLE};
      BasicType type = (BasicType)table[code - Bytecodes::_ineg];
      blk->neg(type JVM_NO_CHECK);
      break;
    }

    case Bytecodes::_i2l:
    case Bytecodes::_i2f:
    case Bytecodes::_i2d:
    case Bytecodes::_l2i:
    case Bytecodes::_l2f:
    case Bytecodes::_l2d:
    case Bytecodes::_f2i:
    case Bytecodes::_f2l:
    case Bytecodes::_f2d:
    case Bytecodes::_d2i:
    case Bytecodes::_d2l:
    case Bytecodes::_d2f:
    case Bytecodes::_i2b:
    case Bytecodes::_i2c:
    case Bytecodes::_i2s: {
      #define CONVERT_OP_TYPE(a, b, c, d) c, d
      static const jubyte convert_op_types[] = {
        CONVERT_OP_TYPE(0x85, i2l, T_INT,    T_LONG),
        CONVERT_OP_TYPE(0x86, i2f, T_INT,    T_FLOAT),
        CONVERT_OP_TYPE(0x87, i2d, T_INT,    T_DOUBLE),
        CONVERT_OP_TYPE(0x88, l2i, T_LONG,   T_INT),
        CONVERT_OP_TYPE(0x89, l2f, T_LONG,   T_FLOAT),
        CONVERT_OP_TYPE(0x8a, l2d, T_LONG,   T_DOUBLE),
        CONVERT_OP_TYPE(0x8b, f2i, T_FLOAT,  T_INT),
        CONVERT_OP_TYPE(0x8c, f2l, T_FLOAT,  T_LONG),
        CONVERT_OP_TYPE(0x8d, f2d, T_FLOAT,  T_DOUBLE),
        CONVERT_OP_TYPE(0x8e, d2i, T_DOUBLE, T_INT),
        CONVERT_OP_TYPE(0x8f, d2l, T_DOUBLE, T_LONG),
        CONVERT_OP_TYPE(0x90, d2f, T_DOUBLE, T_FLOAT),
        CONVERT_OP_TYPE(0x91, i2b, T_INT,    T_BYTE),
        CONVERT_OP_TYPE(0x92, i2c, T_INT,    T_CHAR),
        CONVERT_OP_TYPE(0x93, i2s, T_INT,    T_SHORT),
      };
      #undef CONVERT_OP_TYPE

      const int i = (code - Bytecodes::_i2l) * 2;
      blk->convert((BasicType)convert_op_types[i],
                   (BasicType)convert_op_types[i+1] JVM_NO_CHECK);
      break;
    }

    case Bytecodes::_ifeq : // 153
    case Bytecodes::_ifne : // 154
    case Bytecodes::_iflt : // 155
    case Bytecodes::_ifge : // 156
    case Bytecodes::_ifgt : // 157
    case Bytecodes::_ifle : // 158
      {
        const BytecodeClosure::cond_op op = BytecodeClosure::cond_op
          (code - Bytecodes::_ifeq + BytecodeClosure::eq);
        blk->branch_if(op, branch_destination(bci) JVM_NO_CHECK);
      }
      break;

    case Bytecodes::_ifnull   :
    case Bytecodes::_ifnonnull:
      {
        const BytecodeClosure::cond_op op = BytecodeClosure::cond_op
          (code - Bytecodes::_ifnull + BytecodeClosure::null);
        blk->branch_if( op, bci + get_java_short(bci+1) JVM_NO_CHECK );
      }
      break;

    case Bytecodes::_if_icmpeq:  // 159
    case Bytecodes::_if_icmpne:  // 160
    case Bytecodes::_if_icmplt:  // 161
    case Bytecodes::_if_icmpge:  // 162
    case Bytecodes::_if_icmpgt:  // 163
    case Bytecodes::_if_icmple:  // 164
      {
        const BytecodeClosure::cond_op op = BytecodeClosure::cond_op
          (code - Bytecodes::_if_icmpeq + BytecodeClosure::eq);
        blk->branch_if_icmp(op, branch_destination(bci) JVM_NO_CHECK);
      }
      break;

    case Bytecodes::_lcmp:  // 148
    case Bytecodes::_fcmpl: // 149
    case Bytecodes::_fcmpg: // 150
    case Bytecodes::_dcmpl: // 151
    case Bytecodes::_dcmpg: // 152
      {
        static const jubyte table[] = {
          T_LONG,   BytecodeClosure::eq/* ignored */,
          T_FLOAT,  BytecodeClosure::lt,
          T_FLOAT,  BytecodeClosure::gt,
          T_DOUBLE, BytecodeClosure::lt,
          T_DOUBLE, BytecodeClosure::gt,
        };
        const int i = (code - Bytecodes::_lcmp) * 2;
        blk->compare(BasicType(table[i]),
                     BytecodeClosure::cond_op(table[i+1]) JVM_NO_CHECK);
      }
      break;

    case Bytecodes::_if_acmpeq:
    case Bytecodes::_if_acmpne: {
      const BytecodeClosure::cond_op op = BytecodeClosure::cond_op
        (code - Bytecodes::_if_acmpeq + BytecodeClosure::eq);
      blk->branch_if_acmp(op, branch_destination(bci) JVM_NO_CHECK);
      break;
    }
    case Bytecodes::_goto_w          :
      blk->branch(bci + get_java_int(bci+1) JVM_NO_CHECK);
      break;

    case Bytecodes::_ireturn:
    case Bytecodes::_lreturn:
    case Bytecodes::_freturn:
    case Bytecodes::_dreturn:
    case Bytecodes::_areturn:
    case Bytecodes::_return: {
      static const jubyte table[] = {
        T_INT, T_LONG, T_FLOAT, T_DOUBLE, T_OBJECT, T_VOID
      };
      blk->return_op(BasicType(table[code-Bytecodes::_ireturn]) JVM_NO_CHECK);
      break;
    }
    case Bytecodes::_putfield:
      blk->put_field(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_putstatic:
      blk->put_static(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_getstatic:
      blk->get_static(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_invokestatic:
      blk->invoke_static(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_fast_invokespecial:
      blk->fast_invoke_special(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;

    case Bytecodes::_invokeinterface: {
      const int i = get_java_ushort(bci+1);
      const int n = get_ubyte(bci+3);
      blk->invoke_interface(i, n JVM_NO_CHECK);
      break;
    }
    case Bytecodes::_athrow:
      blk->throw_exception(JVM_SINGLE_ARG_NO_CHECK);
      break;
    case Bytecodes::_new:
      blk->new_object(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_checkcast:
      blk->check_cast(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_instanceof:
      blk->instance_of(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;
    case Bytecodes::_monitorenter:
      blk->monitor_enter(JVM_SINGLE_ARG_NO_CHECK);
      break;
    case Bytecodes::_monitorexit:
      blk->monitor_exit(JVM_SINGLE_ARG_NO_CHECK);
      break;

#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_ildc: // fall through
    case Bytecodes::_fast_fldc: // fall through
    case Bytecodes::_fast_aldc: // fall through
#else
    case Bytecodes::_fast_1_ldc:
#endif
      iterate_push_constant_1(get_ubyte(bci+1), blk JVM_NO_CHECK);
      break;

#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_ildc_w: // fall through
    case Bytecodes::_fast_fldc_w: // fall through
    case Bytecodes::_fast_aldc_w:
#else
    case Bytecodes::_fast_1_ldc_w:
#endif
      iterate_push_constant_1(get_java_ushort(bci+1), blk JVM_NO_CHECK);
      break;

#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_lldc_w: // fall through
    case Bytecodes::_fast_dldc_w:
#else
    case Bytecodes::_fast_2_ldc_w:
#endif
      iterate_push_constant_2(get_java_ushort(bci+1), blk JVM_NO_CHECK);
      break;

#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_iputstatic: // fall through
    case Bytecodes::_fast_lputstatic: // fall through
    case Bytecodes::_fast_fputstatic: // fall through
    case Bytecodes::_fast_dputstatic: // fall through
    case Bytecodes::_fast_aputstatic: // fall through
#else
    case Bytecodes::_fast_1_putstatic:      // fall through
    case Bytecodes::_fast_2_putstatic:      // fall through
    case Bytecodes::_fast_a_putstatic:      // fall through
    case Bytecodes::_fast_init_1_putstatic: // fall through
    case Bytecodes::_fast_init_2_putstatic: // fall through
    case Bytecodes::_fast_init_a_putstatic: // fall through
#endif
      blk->put_static(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;

#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_igetstatic: // fall through
    case Bytecodes::_fast_lgetstatic: // fall through
    case Bytecodes::_fast_fgetstatic: // fall through
    case Bytecodes::_fast_dgetstatic: // fall through
    case Bytecodes::_fast_agetstatic: // fall through
#else
    case Bytecodes::_fast_1_getstatic:      // fall through
    case Bytecodes::_fast_2_getstatic:      // fall through
    case Bytecodes::_fast_init_1_getstatic: // fall through
    case Bytecodes::_fast_init_2_getstatic: // fall through
#endif
      blk->get_static(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;

    case Bytecodes::_fast_bputfield: // fall through
    case Bytecodes::_fast_sputfield: // fall through
    case Bytecodes::_fast_iputfield: // fall through
    case Bytecodes::_fast_lputfield: // fall through
    case Bytecodes::_fast_fputfield: // fall through
    case Bytecodes::_fast_dputfield: // fall through
    case Bytecodes::_fast_aputfield: {
      BasicType type = field_op_types[code - Bytecodes::_fast_bputfield];
      int offset = ENABLE_NATIVE_ORDER_REWRITING ? get_native_ushort(bci+1)
                                                 : get_java_ushort(bci+1);
      if (::byte_size_for(type) >= BytesPerWord) {
        offset *= BytesPerWord;
      }
      blk->fast_put_field(type, offset JVM_NO_CHECK);
      break;
    }

    case Bytecodes::_fast_bgetfield: // fall through
    case Bytecodes::_fast_sgetfield: // fall through
    case Bytecodes::_fast_igetfield: // fall through
    case Bytecodes::_fast_lgetfield: // fall through
    case Bytecodes::_fast_fgetfield: // fall through
    case Bytecodes::_fast_dgetfield: // fall through
    case Bytecodes::_fast_agetfield: // fall through
    case Bytecodes::_fast_cgetfield: {
      BasicType type = field_op_types[code - Bytecodes::_fast_bgetfield];
      int offset = ENABLE_NATIVE_ORDER_REWRITING ? get_native_ushort(bci+1)
                                                 : get_java_ushort(bci+1);
      if (byte_size_for(type) >= BytesPerWord) {
        offset *= BytesPerWord;
      }
      blk->fast_get_field(type, offset JVM_NO_CHECK);
      break;
    }
    case Bytecodes::_fast_igetfield_1: {
      int offset = get_ubyte(bci+1) * BytesPerWord;
      blk->fast_get_field(T_INT, offset JVM_NO_CHECK);
      break;
    }
    case Bytecodes::_fast_agetfield_1: {
      int offset = get_ubyte(bci+1) * BytesPerWord;
      blk->fast_get_field(T_OBJECT, offset JVM_NO_CHECK);
      break;
    }
    case Bytecodes::_aload_0_fast_igetfield_1: {
      blk->aload_0_fast_get_field_1(T_INT JVM_NO_CHECK);
      break;
    }
    case Bytecodes::_aload_0_fast_agetfield_1: {
      blk->aload_0_fast_get_field_1(T_OBJECT JVM_NO_CHECK);
      break;
    }
#if !ENABLE_CPU_VARIANT  
    case Bytecodes::_aload_0_fast_igetfield_4:
    case Bytecodes::_aload_0_fast_agetfield_4:
    case Bytecodes::_aload_0_fast_igetfield_8:
    case Bytecodes::_aload_0_fast_agetfield_8:
      blk->aload_0_fast_get_field_n(code JVM_NO_CHECK);
      break;
#endif //!ENABLE_CPU_VARIANT  
    case Bytecodes::_fast_invokevirtual:
      blk->fast_invoke_virtual(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;

    case Bytecodes::_fast_invokestatic:
    case Bytecodes::_fast_init_invokestatic:
      blk->invoke_static(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;

    case Bytecodes::_fast_invokeinterface:
      {
        int i = get_java_ushort(bci+1);
        int n = get_ubyte(bci+3);
        blk->invoke_interface(i, n JVM_NO_CHECK);
      }
      break;

    case Bytecodes::_fast_invokenative:
      {
        BasicType type = (BasicType)get_ubyte(bci+1);
        blk->invoke_native(type, get_native_code() JVM_CHECK);
        blk->return_op(type JVM_NO_CHECK);
      }
      break;

    case Bytecodes::_fast_new:
    case Bytecodes::_fast_init_new:
      blk->new_object(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;

    case Bytecodes::_fast_anewarray:
      blk->new_object_array(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;

    case Bytecodes::_fast_checkcast:
      blk->check_cast(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;

    case Bytecodes::_fast_instanceof:
      blk->instance_of(get_java_ushort(bci+1) JVM_NO_CHECK);
      break;

  // Most common bytecodes
  case Bytecodes::_ldc:
    iterate_push_constant_1(get_ubyte(bci+1), blk JVM_NO_CHECK);
    break;

  case Bytecodes::_iadd:
    blk->binary(T_INT, BytecodeClosure::bin_add JVM_NO_CHECK);
    break;

  case Bytecodes::_invokespecial:
    blk->invoke_special(get_java_ushort(bci+1) JVM_NO_CHECK);
    break;

  case Bytecodes::_goto:
    blk->branch(branch_destination(bci) JVM_NO_CHECK);
    break;

  case Bytecodes::_sipush:
    blk->push_int(get_java_short(bci+1) JVM_NO_CHECK);
    break;

  case Bytecodes::_istore:
    blk->store_local(T_INT, get_ubyte(bci+1) JVM_NO_CHECK);
    break;

  case Bytecodes::_fast_invokevirtual_final:
    blk->fast_invoke_virtual_final(get_java_ushort(bci+1) JVM_NO_CHECK);
    break;

  case Bytecodes::_iconst_0:
    blk->push_int(0 JVM_NO_CHECK);
    break;

  case Bytecodes::_iconst_1:
    blk->push_int(1 JVM_NO_CHECK);
    break;

  case Bytecodes::_aload_1:
    blk->load_local(T_OBJECT, 1 JVM_NO_CHECK);
    break;

  case Bytecodes::_aload:
    blk->load_local(T_OBJECT, get_ubyte(bci+1) JVM_NO_CHECK);
    break;

  case Bytecodes::_invokevirtual:
    blk->invoke_virtual(get_java_ushort(bci+1) JVM_NO_CHECK);
    break;

  case Bytecodes::_getfield:
    blk->get_field(get_java_ushort(bci+1) JVM_NO_CHECK);
    break;

  case Bytecodes::_iload:
    blk->load_local(T_INT, get_ubyte(bci+1) JVM_NO_CHECK);
    break;

  case Bytecodes::_aload_0:
    blk->load_local(T_OBJECT, 0 JVM_NO_CHECK);
    break;

  case Bytecodes::_iastore:
    blk->store_array(T_INT JVM_NO_CHECK);
    break;

  case Bytecodes::_dup:
    blk->dup(JVM_SINGLE_ARG_NO_CHECK);
    break;

  case Bytecodes::_bipush:
    blk->push_int(get_byte(bci+1) JVM_NO_CHECK);
    break;

  case Bytecodes::_tableswitch:
    {
      const int aligned_index = align_size_up(bci + 1, sizeof(jint));
      const int default_dest  = bci + get_java_switch_int(aligned_index + 0);
      const int low           = get_java_switch_int(aligned_index + 4);
      const int high          = get_java_switch_int(aligned_index + 8);
      blk->table_switch(aligned_index, default_dest, low, high JVM_NO_CHECK);
    }
    break;

  case Bytecodes::_lookupswitch:
    {
      const int aligned_index = align_size_up(bci + 1, sizeof(jint));
      const int default_dest  = bci + get_java_switch_int(aligned_index + 0);
      const int num_of_pairs  = get_java_switch_int(aligned_index + 4);
      blk->lookup_switch(aligned_index, default_dest,num_of_pairs JVM_NO_CHECK);
    }
    break;

  case Bytecodes::_wide:
    {
      static const BasicType local_op_types[]  = {
        T_INT,  T_LONG,  T_FLOAT,  T_DOUBLE, T_OBJECT
      };

      code = bytecode_at(bci + 1);
      switch (code) {

      case Bytecodes::_iinc :
        {
          const jint index  = get_java_ushort(bci+2);
          const jint offset = get_java_short(bci+4);
          blk->increment_local_int(index, offset JVM_NO_CHECK);
        }
        break;

      case Bytecodes::_iload : // fall
      case Bytecodes::_lload : // fall
      case Bytecodes::_fload : // fall
      case Bytecodes::_dload : // fall
      case Bytecodes::_aload :
#if ENABLE_CPU_VARIANT && ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
      case Bytecodes::_lload_safe: // fall
      case Bytecodes::_dload_safe: // fall
#endif
        blk->load_local(local_op_types[code - Bytecodes::_iload],
        get_java_ushort(bci+2) JVM_NO_CHECK);
        break;

      case Bytecodes::_istore: // fall
      case Bytecodes::_lstore: // fall
      case Bytecodes::_fstore: // fall
      case Bytecodes::_dstore: // fall
      case Bytecodes::_astore:
#if ENABLE_CPU_VARIANT && ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
      case Bytecodes::_lstore_safe: // fall
      case Bytecodes::_dstore_safe: // fall
#endif
        blk->store_local(local_op_types[code - Bytecodes::_istore],
        get_java_ushort(bci+2) JVM_NO_CHECK);
        break;

      default:
        blk->illegal_code(JVM_SINGLE_ARG_NO_CHECK);
        break;
      }
    }
    break;
  default:
    blk->illegal_code(JVM_SINGLE_ARG_NO_CHECK);
    break;
  }

  // JVM_NO_CHECK is used instead of JVM_CHECK in the above switch statement.
  // For that reason an explicit exception check is needed here.
  if (CURRENT_HAS_PENDING_EXCEPTION) {
    blk->handle_exception(JVM_SINGLE_ARG_CHECK);
  }

  blk->bytecode_epilog(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void Method::set_default_entry(bool check_impossible_flag) {
#if ENABLE_COMPILER
#if USE_BINARY_IMAGE_GENERATOR
  if (!GenerateROMImage)
#endif
  {
    if (UseCompiler && !MixedMode &&
        (!check_impossible_flag || !is_impossible_to_compile())) {
      set_execution_entry((address) shared_invoke_compiler);
      return;
    }
  }
#endif
  set_default_interpreter_entry();
}

void Method::update_rom_default_entries() {
#if ENABLE_COMPILER
  if (!UseCompiler || MixedMode) {
    return;
  }

  MethodVariablePart *ptr =
    (MethodVariablePart *)&_rom_method_variable_parts[0];
  MethodVariablePart *end =
    (MethodVariablePart *) (((int)ptr) + _rom_method_variable_parts_size);

  address compile_entry = (address)shared_invoke_compiler;

  while (ptr < end) {
    address entry = ptr->execution_entry();
    if ((entry == (address)interpreter_fast_method_entry_0) ||
        (entry == (address)interpreter_fast_method_entry_1) ||
        (entry == (address)interpreter_fast_method_entry_2) ||
        (entry == (address)interpreter_fast_method_entry_3) ||
        (entry == (address)interpreter_fast_method_entry_4) ||
        (entry == (address)interpreter_method_entry)) {
      ptr->set_execution_entry(compile_entry);
    }

    ptr ++;
  }
#endif
}

void Method::set_default_interpreter_entry() {
  static const address table[] = {
    (address)interpreter_fast_method_entry_0,
    (address)interpreter_fast_method_entry_1,
    (address)interpreter_fast_method_entry_2,
    (address)interpreter_fast_method_entry_3,
    (address)interpreter_fast_method_entry_4,
    (address)interpreter_method_entry,
  };
  set_interpreter_entry_with_table(table);
}

void Method::set_fixed_interpreter_entry() {
#if ENABLE_COMPILER
  static const address table[] = {
    (address)fixed_interpreter_fast_method_entry_0,
    (address)fixed_interpreter_fast_method_entry_1,
    (address)fixed_interpreter_fast_method_entry_2,
    (address)fixed_interpreter_fast_method_entry_3,
    (address)fixed_interpreter_fast_method_entry_4,
    (address)fixed_interpreter_method_entry,
  };
  set_interpreter_entry_with_table(table);
#endif
}

void Method::set_interpreter_entry_with_table(const address *table) {
  address entry = table[5];
  if (!access_flags().is_synchronized()) {
    int size = max_locals() - size_of_parameters();
    if (((juint)size) <= 4) {
      entry = table[size];
    }
  }

  set_execution_entry(entry);
}

#if USE_COMPILER_STRUCTURES

void Method::unlink_compiled_code() {
  set_default_entry(true);
}

inline void Method::add_entry( jubyte counts[], const int bci, const int inc ) {
  // We need to make sure that very large number of entry counts would not
  // overflow a byte.
  jubyte count = counts[ bci ] + inc;

  enum { MaxCount = 10 };
  if( count > MaxCount ) {
    count = MaxCount;
  }
  counts[ bci ] = count;
}

#if !(ENABLE_COMPILER && ENABLE_ROM_GENERATOR)
inline
#endif
void Method::add_exception_table_entries( jubyte counts[] ) const {
  TypeArray::Raw exception_table = this->exception_table();
  GUARANTEE((exception_table().length() % 4) == 0, "Sanity check");
  const int len = exception_table().length();
  for( int i = 0; i < len; i += 4 ) {
    const int handler_bci = exception_table().ushort_at(i + 2);
    add_entry(counts, handler_bci, 2);
  }
}

#define ADD_BRANCH_ENTRY(offset)                        \
  const int dest = bci + offset;                        \
  GUARANTEE(dest >= 0 && dest < codesize, "sanity");    \
  add_entry( entry_counts, dest )

#define ADD_BACK_BRANCH_ENTRY(offset) ADD_BRANCH_ENTRY(offset); \
  if(dest <= bci) { has_loops = true; }

#if ENABLE_COMPILER

void Method::compute_attributes(Attributes& attributes JVM_TRAPS) const {
  GUARANTEE( Compiler::is_active(), "Sanity" );

  const int codesize = code_size();
  CompilerByteArray* entry_count_array =
    CompilerByteArray::allocate( codesize JVM_ZCHECK( entry_count_array ) );
  CompilerByteArray* bci_flags_array =
    CompilerByteArray::allocate( codesize JVM_ZCHECK( bci_flags_array ) );

  {
    // This is a hot function during compilation. Since it only operates
    // on a few bytecodes, we hand-code the loop rather than 
    // using Method::iterate, which has much higher overhead. Also, this
    // saves footprint because we don't need a C++ vtable for BytecodeClosure.
    AllocationDisabler raw_pointers_used_in_this_function;

    jubyte* entry_counts = entry_count_array->base();
    jubyte* bci_flags    = bci_flags_array  ->base();

    // Give the first bytecode an entry, and iterate over the bytecodes.
    entry_counts[0] = 1;
    {
      int num_locks = 0;
      bool has_loops = false;
      int exception_count = 0;
      const jubyte* codebase = (jubyte*)code_base();
      int bci = 0;
      jushort accumulated_flags = 0;
      int branch_bci = -1;

      while( bci < codesize ) {
        const jubyte* const bcptr = codebase + bci;
        const Bytecodes::Code code = Bytecodes::Code(*bcptr);
        GUARANTEE(code >= 0, "sanity: unsigned value");

        const int length = bytecode_length_for(bci);

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
            GUARANTEE(Bytecodes::can_fall_through(code), 
                      "Conditional branches can fall through");
            branch_bci = bci;
            // Fall through
          case Bytecodes::_goto: {
            ADD_BACK_BRANCH_ENTRY(jshort(Bytes::get_Java_u2(address(bcptr+1))));
          } break;
          case Bytecodes::_goto_w: {
            ADD_BACK_BRANCH_ENTRY(int(Bytes::get_Java_u4(address(bcptr+1))));
          } break;
          case Bytecodes::_lookupswitch: {
            const int table_index  = align_size_up(bci + 1, sizeof(jint));
            ADD_BRANCH_ENTRY( get_java_switch_int(table_index + 0) );

            const int num_of_pairs = get_java_switch_int(table_index + 4);
            for( int i = 0; i < num_of_pairs; i++ ) {
              ADD_BACK_BRANCH_ENTRY( get_java_switch_int(8 * i + table_index + 12) );
            }
          } break;
          case Bytecodes::_tableswitch: {
            const int table_index  = align_size_up(bci + 1, sizeof(jint));
            ADD_BRANCH_ENTRY( get_java_switch_int(table_index + 0) );

            const int size = get_java_switch_int(table_index + 8) -
                             get_java_switch_int(table_index + 4);
            for (int i = 0; i <= size; i++) {
              ADD_BACK_BRANCH_ENTRY( get_java_switch_int(4 * i + table_index + 12) );
            }
          } break;
          case Bytecodes::_monitorenter:
            num_locks++;
            break;
          case Bytecodes::_athrow:
            if (branch_bci >= 0) {
              GUARANTEE(branch_bci < codesize, "Sanity");
              bci_flags[branch_bci] |= bci_branch_taken;
            }
            // Fall through
          default:
#if ENABLE_NPCE && ENABLE_INTERNAL_CODE_OPTIMIZER
            if( Bytecodes::is_null_point_exception_throwable(code) ) {
              exception_count++;
            } 
#endif 
            break;
        }

        {
          const jushort flags = Bytecodes::get_flags(code);
          accumulated_flags |= flags;
          if( Bytecodes::can_fall_through_flags(flags) ) {
            ADD_BRANCH_ENTRY(length);
          } else {
            branch_bci = -1;
          }
        }

        bci += length;
      }

      GUARANTEE(bci == codesize, "Sanity");
      add_exception_table_entries( entry_counts );

      attributes.entry_counts = entry_count_array;
      attributes.bci_flags = bci_flags_array;
      attributes.has_loops = has_loops;
      attributes.can_throw_exceptions = 
        Bytecodes::can_throw_exceptions_flags(accumulated_flags);
#if ENABLE_INLINE
      attributes.bytecodes_allow_inlining = 
        Bytecodes::allow_inlining_flags(accumulated_flags);
#endif
      attributes.num_locks = num_locks;
      attributes.num_bytecodes_can_throw_npe = exception_count;  
    }
  }
}

#endif // ENABLE_COMPILER

#if ENABLE_ROM_GENERATOR

#undef ADD_BACK_BRANCH_ENTRY
#define ADD_BACK_BRANCH_ENTRY(offset) ADD_BRANCH_ENTRY(offset);

OopDesc* Method::compute_entry_counts(JVM_SINGLE_ARG_TRAPS) const {
  const int codesize = code_size();
  TypeArray::Raw entry_count_array =
    Universe::new_byte_array(codesize JVM_OZCHECK(entry_count_array));

  AllocationDisabler raw_pointers_used_in_this_function;
  jubyte* entry_counts = entry_count_array().base_address();

  // Give the first bytecode an entry, and iterate over the bytecodes.
  entry_counts[0] = 1;
  {
    const jubyte* codebase = (jubyte*)code_base();
    int bci = 0;
    int branch_bci = -1;

    while( bci < codesize ) {
      const jubyte* const bcptr = codebase + bci;
      const Bytecodes::Code code = Bytecodes::Code(*bcptr);
      GUARANTEE(code >= 0, "sanity: unsigned value");

      const int length = bytecode_length_for(bci);

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
          GUARANTEE(Bytecodes::can_fall_through(code), 
                    "Conditional branches can fall through");
          branch_bci = bci;
          // Fall through
        case Bytecodes::_goto: {
          ADD_BACK_BRANCH_ENTRY(jshort(Bytes::get_Java_u2(address(bcptr+1))));
        } break;
        case Bytecodes::_goto_w: {
          ADD_BACK_BRANCH_ENTRY(int(Bytes::get_Java_u4(address(bcptr+1))));
        } break;
        case Bytecodes::_lookupswitch: {
          const int table_index  = align_size_up(bci + 1, sizeof(jint));
          ADD_BRANCH_ENTRY( get_java_switch_int(table_index + 0) );

          const int num_of_pairs = get_java_switch_int(table_index + 4);
          for( int i = 0; i < num_of_pairs; i++ ) {
            ADD_BACK_BRANCH_ENTRY( get_java_switch_int(8 * i + table_index + 12) );
          }
        } break;
        case Bytecodes::_tableswitch: {
          const int table_index  = align_size_up(bci + 1, sizeof(jint));
          ADD_BRANCH_ENTRY( get_java_switch_int(table_index + 0) );

          const int size = get_java_switch_int(table_index + 8) -
                           get_java_switch_int(table_index + 4);
          for (int i = 0; i <= size; i++) {
            ADD_BACK_BRANCH_ENTRY( get_java_switch_int(4 * i + table_index + 12) );
          }
        } break;
      }

      if( Bytecodes::can_fall_through_flags( Bytecodes::get_flags(code) ) ) {
        ADD_BRANCH_ENTRY(length);
      } else {
        branch_bci = -1;
      }

      bci += length;
    }

    GUARANTEE(bci == codesize, "Sanity");
  }
  add_exception_table_entries( entry_counts );
  return entry_count_array;
}
#endif



#undef ADD_BRANCH_ENTRY
#undef ADD_BACK_BRANCH_ENTRY
#endif

bool Method::is_impossible_to_compile() const {
  const address entry = execution_entry();

  if ((entry == (address)interpreter_fast_method_entry_0) ||
      (entry == (address)interpreter_fast_method_entry_1) ||
      (entry == (address)interpreter_fast_method_entry_2) ||
      (entry == (address)interpreter_fast_method_entry_3) ||
      (entry == (address)interpreter_fast_method_entry_4) ||
      (entry == (address)interpreter_method_entry)
#if ENABLE_COMPILER
      || (entry == (address)shared_invoke_compiler)
#endif
      ) {
    return false;
  }
  else if (ObjectHeap::contains((OopDesc*)execution_entry())) {
    return false;
  }
  else {
    return true;
  }
}

void Method::set_fast_accessor_entry(JVM_SINGLE_ARG_TRAPS) {
  static const address accessors[] = {
    /* T_BOOLEAN 4 */ (address) shared_fast_getbyte_accessor,
    /* T_CHAR    5 */ (address) shared_fast_getchar_accessor,
    /* T_FLOAT   6 */ (address) shared_fast_getfloat_accessor,
    /* T_DOUBLE  7 */ (address) shared_fast_getdouble_accessor,
    /* T_BYTE    8 */ (address) shared_fast_getbyte_accessor,
    /* T_SHORT   9 */ (address) shared_fast_getshort_accessor,
    /* T_INT    10 */ (address) shared_fast_getint_accessor,
    /* T_LONG   11 */ (address) shared_fast_getlong_accessor,
    /* T_OBJECT 12 */ (address) shared_fast_getint_accessor,
    /* T_ARRAY  13 */ (address) shared_fast_getint_accessor,
  };
  static const address static_accessors[] = {
    /* T_BOOLEAN 4 */ (address) shared_fast_getbyte_static_accessor,
    /* T_CHAR    5 */ (address) shared_fast_getchar_static_accessor,
    /* T_FLOAT   6 */ (address) shared_fast_getfloat_static_accessor,
    /* T_DOUBLE  7 */ (address) shared_fast_getdouble_static_accessor,
    /* T_BYTE    8 */ (address) shared_fast_getbyte_static_accessor,
    /* T_SHORT   9 */ (address) shared_fast_getshort_static_accessor,
    /* T_INT    10 */ (address) shared_fast_getint_static_accessor,
    /* T_LONG   11 */ (address) shared_fast_getlong_static_accessor,
    /* T_OBJECT 12 */ (address) shared_fast_getint_static_accessor,
    /* T_ARRAY  13 */ (address) shared_fast_getint_static_accessor,
  };

  BasicType type;
  int offset;
  bool ok = may_convert_to_fast_get_accessor(type, offset JVM_MUST_SUCCEED);

  if (ok && offset <= 0x7fff) {
    GUARANTEE(T_BOOLEAN <= type && type <= T_ARRAY, "sanity");
    const address* table = is_static() ? static_accessors : accessors;
    set_fast_accessor_offset((jushort)offset);
    set_fast_accessor_type(type);
    set_execution_entry(table[type - T_BOOLEAN]);
  }
}

jushort Method::ushort_at(jint bci) {
  return Bytes::get_Java_u2((address)field_base(bc_offset_for(bci)));
}

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_TTY_TRACE
// Is this method overloaded in its holder class?
bool Method::is_overloaded() const {
  InstanceClass::Raw h = holder();
  ObjArray::Raw methods = h().methods();
  bool dummy;
  int i;

  for (i=0; i<methods().length(); i++) {
    Method::Raw m = methods().obj_at(i);
    if (m.not_null() && !m.equals(this)) {
      Symbol::Raw my_name = get_original_name(dummy);
      Symbol::Raw other_name = m().get_original_name(dummy);
      if (my_name.equals(&other_name)) {
        return true;
      }
    }
  }

  // In the case of romized classes, virtual methods would be
  // removed from methods() list and present only inside the vtable
  ClassInfo::Raw info = h().class_info();
  int len = h().vtable_length();

  for (i=0; i<len; i++) {
    Method::Raw m = info().vtable_method_at(i);
    if (m.not_null() && !m.equals(this)) {
      Symbol::Raw my_name = get_original_name(dummy);
      Symbol::Raw other_name = m().get_original_name(dummy);
      if (my_name.equals(&other_name)) {
        return true;
      }
    }
  }

  return false;
}
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

void Method::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop::iterate(visitor);

  {
    if (has_compressed_header()) {
      NamedField id("constants (skipped)", true);
      id.set_hex_output(true);
      id.set_is_pointer(true);
      visitor->do_uint(&id, constants_offset(), true);
    } else {
      NamedField id("constants", true);
      visitor->do_oop(&id, constants_offset(), true);
    }
  }

  {
    if (has_no_exception_table()) {
      NamedField id("exception_table (skipped)", true);
      id.set_hex_output(true);
      id.set_is_pointer(true);
      visitor->do_uint(&id, exception_table_offset(), true);
    } else {
      NamedField id("exception_table", true);
      visitor->do_oop(&id, exception_table_offset(), true);
    }
  }

  {
    if (has_no_stackmaps()) {
      NamedField id("stackmaps (skipped)", true);
      id.set_hex_output(true);
      id.set_is_pointer(true);
      visitor->do_uint(&id, stackmaps_offset(), true);
    } else {
      NamedField id("stackmaps", true);
      visitor->do_oop(&id, stackmaps_offset(), true);
    }
  }

  { // execution entry
    NamedField id("execution entry", true);
    id.set_hex_output(true);
    id.set_is_pointer(true);
    visitor->do_uint(&id, heap_execution_entry_offset(), true);
  }

#if ENABLE_ROM_JAVA_DEBUGGER
  { // line number table
    NamedField id("line number table", true);
    id.set_hex_output(true);
    id.set_is_pointer(true);
    visitor->do_uint(&id, line_var_table_offset(), true);
  }
#endif
  {
    NamedField id("variable info", true);
    id.set_hex_output(true);
    id.set_is_pointer(true);
    visitor->do_uint(&id, variable_part_offset(), true);
  }

#if ENABLE_REFLECTION
  {
    NamedField id("thrown_exceptions", true);
    visitor->do_oop(&id, thrown_exceptions_offset(), true);
  }
#endif

  { // access flags
    char buff[1024];
    access_flags().print_to_buffer(buff, AccessFlags::METHOD_FLAGS);
    visitor->do_comment(buff);
    NamedField id("access_flags", true);
    id.set_hex_output(true);
    visitor->do_ushort(&id, access_flags_offset(), true);
  }

  {
    NamedField id("holder_id", true);
    visitor->do_ushort(&id, holder_id_offset(), true);
  }

  if (is_quick_native()) {
    NamedField id("quick_native_code", true);
    id.set_hex_output(true);
    id.set_is_pointer(true);
    visitor->do_uint(&id, quick_native_code_offset(), true);
  } else if (is_fast_get_accessor()) {
    {
      NamedField id("fast_accessor_type", true);
      visitor->do_ushort(&id, fast_accessor_type_offset(), true);
    }
    {
      NamedField id("fast_accessor_offset", true);
      visitor->do_ushort(&id, fast_accessor_offset_offset(), true);
    }
  } else {
    {
      NamedField id("max_execution_stack_count", true);
      visitor->do_ushort(&id, max_execution_stack_count_offset(), true);
    }
    {
      NamedField id("max_locals", true);
      visitor->do_ushort(&id, max_locals_offset(), true);
    }
  }

  {
    NamedField id("size_of_parameters_and_return_type", true);
    visitor->do_ushort(&id, method_attributes_offset(), true);
  }

  {
    NamedField id("name_index", true);
    visitor->do_ushort(&id, name_index_offset(), true);
  }

  {
    NamedField id("signature_index", true);
    visitor->do_ushort(&id, signature_index_offset(), true);
  }

  {
    NamedField id("code_size", true);
    visitor->do_ushort(&id, code_size_offset(), true);
  }

#if ENABLE_JVMPI_PROFILE
  {
    NamedField id("method_id", true);
    visitor->do_uint(&id, method_id_offset(), true);
  }
#endif  

#endif
}

void Method::print_name_on(Stream* st, bool long_output) const {
#if ENABLE_TTY_TRACE
  bool renamed;
  const TaskGCContext tmp(obj());
  Signature::Raw sig = signature();
  Symbol::Raw n = get_original_name(renamed);

  if (long_output) {
    st->print("%s ", is_static() ? "static" : "virtual");
    sig().print_return_type_on(st);
    st->print(" ");
  }

  if (tmp.valid() && holder_id() != 0xffff) {
    InstanceClass::Raw h = holder();
    h().print_name_on(st);
  } else {
    st->print("<No Holder Name>, ID= 0x%x", holder_id());
  }
  st->print(".");
  n().print_symbol_on(st);

#if ENABLE_ISOLATES
  if (long_output) {
#else
  if (long_output || is_overloaded()) {
#endif
    sig().print_parameters_on(st);
  }
#endif
}
#endif //PRODUCT

#if !defined(PRODUCT) || ENABLE_PERFORMANCE_COUNTERS || ENABLE_TTY_TRACE
// Print name to a 0-terminated char buffer. max_length includes the trailing 0
void Method::print_name_to(char *buffer, int max_length) {
#if USE_DEBUG_PRINTING
  bool dummy;
  const TaskGCContext tmp( obj() );

  // (1) Copy the class name
  int i, avail;
  char * p = buffer;
  InstanceClass klass = holder();
  Symbol class_name = klass.original_name();

  avail = max_length - (p - buffer) - 1;
  int class_name_len = class_name.length();
  if (class_name_len > avail) {
      class_name_len = avail;
  }
  for (i=0; i<class_name_len; i++) {
    *p++ = (char)class_name.byte_at(i);
  }

  // (2) A "."
  avail = max_length - (p - buffer) - 1;
  if (avail > 0) {
    *p++ = '.';
  }

  // (3) Copy the method name
  Symbol method_name = get_original_name(dummy);
  int method_name_len = method_name.length();
  avail = max_length - (p - buffer) - 1;
  if (method_name_len > avail) {
      method_name_len = avail;
  }
  for (i=0; i<method_name_len; i++) {
    *p++ = (char)method_name.byte_at(i);
  }

  // (4) Copy the signature
  ByteArrayOutputStream baos;
  Signature sig = signature();
  sig.print_decoded_on(&baos);
  char *q = baos.get_chars();
  int signature_len = jvm_strlen(q);
  avail = max_length - (p - buffer) - 1;

  if (signature_len > avail) {
      signature_len = avail;
  }
  for (i=0; i<signature_len; i++) {
    *p++ = *q++;
  }
  *p = 0;

  GUARANTEE((p - buffer) <= max_length, "buffer overflow");
#else
  (void)buffer; (void)max_length;
#endif
}
#endif

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR || \
        ENABLE_PERFORMANCE_COUNTERS ||ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
ReturnOop Method::get_original_name(bool& renamed) const {
  Symbol::Raw n = name();

  if (n().equals(Symbols::unknown())) {
    renamed = true;
    Symbol::Raw n2 = ROM::get_original_method_name(this);
    if (!n2.is_null()) {
      return n2.obj();
    }
  } else {
    renamed = false;
  }

  return n.obj();
}
#endif

#if ENABLE_ROM_GENERATOR

// generate a map of all the field types in this object
int Method::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;

  //klass
  map_index = Near::generate_fieldmap(field_map);

  //constants
  field_map->byte_at_put(map_index++, T_OBJECT);
  //exception table
  field_map->byte_at_put(map_index++, T_OBJECT);
  //stackmaps
  field_map->byte_at_put(map_index++, T_OBJECT);

  //execution entry
  field_map->byte_at_put(map_index++, T_SYMBOLIC);
#if ENABLE_ROM_JAVA_DEBUGGER
  //line number table
  field_map->byte_at_put(map_index++, T_OBJECT);
#endif
  //variable info
  field_map->byte_at_put(map_index++, T_SYMBOLIC);
#if ENABLE_REFLECTION
  //thrown exceptions
  field_map->byte_at_put(map_index++, T_OBJECT);
#endif
  //access_flags
  field_map->byte_at_put(map_index++, T_SHORT);
  //holder_id
  field_map->byte_at_put(map_index++, T_SHORT);
  if (!is_quick_native()) {
    //max_execution_stack_count
    field_map->byte_at_put(map_index++, T_SHORT);
    //max_locals
    field_map->byte_at_put(map_index++, T_SHORT);
  } else {
    // quick_native_code
    field_map->byte_at_put(map_index++, T_SYMBOLIC);
  }
  //size_of_parameters
  field_map->byte_at_put(map_index++, T_SHORT);
  //name_index
  field_map->byte_at_put(map_index++, T_SHORT);
  //signature_index
  field_map->byte_at_put(map_index++, T_SHORT);
  //code_size
  field_map->byte_at_put(map_index++, T_SHORT);

#if ENABLE_JVMPI_PROFILE
  //method_id
  field_map->byte_at_put(map_index++, T_INT);
#endif  

  // Add special types for all bytecodes
  jint code_map_size = align_size_up(code_size(), BytesPerWord) / BytesPerWord;
  if (map_index + code_map_size > field_map->length()) {
    return map_index + code_map_size;
  }

  for (int i = 0; i < code_map_size; i++) {
    field_map->byte_at_put(map_index++, T_SYMBOLIC);
  }
  return map_index;
}

#endif /* #if ENABLE_ROM_GENERATOR*/

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

void Method::print_bytecodes(Stream* st, int start, int end, bool include_nl,
                             bool verbose) {
#if USE_DEBUG_PRINTING
  SETUP_ERROR_CHECKER_ARG;
  BytecodePrintClosure bp(st, include_nl, verbose);
  bp.initialize(this);
  iterate(start, end, &bp JVM_NO_CHECK_AT_BOTTOM);
#endif
}

void Method::iterate_oopmaps(oopmaps_doer do_map, void* param) {
#if USE_OOP_VISITOR
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, constants);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, exception_table);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, stackmaps);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    heap_execution_entry);
#if ENABLE_ROM_JAVA_DEBUGGER
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, line_var_table);
#endif
  OOPMAP_ENTRY_4(do_map, param, T_INT,    variable_part);
#if ENABLE_REFLECTION
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, thrown_exceptions);
#endif
  OOPMAP_ENTRY_4(do_map, param, T_SHORT,  access_flags);
  OOPMAP_ENTRY_4(do_map, param, T_SHORT,  holder_id);
  OOPMAP_ENTRY_4(do_map, param, T_SHORT,  max_execution_stack_count);
  OOPMAP_ENTRY_4(do_map, param, T_SHORT,  max_locals);
  OOPMAP_ENTRY_4(do_map, param, T_SHORT,  method_attributes);
  OOPMAP_ENTRY_4(do_map, param, T_SHORT,  name_index);
  OOPMAP_ENTRY_4(do_map, param, T_SHORT,  signature_index);
  OOPMAP_ENTRY_4(do_map, param, T_SHORT,  code_size);

#if ENABLE_JVMPI_PROFILE
  OOPMAP_ENTRY_4(do_map, param, T_INT,  method_id);
#endif

  // alias OOPMAP_ENTRY_4(do_map, param, T_INT,    quick_native_code);
  // alias OOPMAP_ENTRY_4(do_map, param, T_SHORT,  fast_accessor_offset);
  // alias OOPMAP_ENTRY_4(do_map, param, T_SHORT,  fast_accessor_type);
  // alias OOPMAP_ENTRY_4(do_map, param, T_INT,    native_code);
#endif
}
#endif

#if ENABLE_CSE
bool Method::is_snippet_can_be_elminate(jint begin_bci, jint end_bci, int& local_mask, int& constant_mask, 
                           int& array_type_mask) {
 AllocationDisabler raw_pointers_used_in_this_function;
  register int bci = begin_bci;
  int local_index, constant_index, array_type;
  while (bci  <=  end_bci) {
    local_index = 0;
    constant_index = 0;
    Bytecodes::Code code = bytecode_at(bci);
    
    int length = Bytecodes::length_for(this, bci);
    if (length == 0) {
      return false;
    }

  /*
   * The bytecodes are sorted in reversed order of frequency, so
   * that it can generate the best code sequence on Thumb (which has
   * a limit of 1024 in immediate branch offset. The ordering has no effect
   * on ARM.
   */

  switch (code) {

    case Bytecodes::_iload_0          : // fall
    case Bytecodes::_iload_1          : // fall
    case Bytecodes::_iload_2          : // fall
    case Bytecodes::_iload_3          :
        local_index = code - Bytecodes::_iload_0;
        break;

    case Bytecodes::_lload            :
#if ENABLE_CPU_VARIANT && ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
    case Bytecodes::_lload_safe       :
#endif
        local_index = get_ubyte(bci+1);
        break;
    case Bytecodes::_lload_0          : // fall
    case Bytecodes::_lload_1          : // fall
    case Bytecodes::_lload_2          : // fall
    case Bytecodes::_lload_3          :
        local_index = code - Bytecodes::_lload_0;
        break;

    case Bytecodes::_fload            :
    case Bytecodes::_fload_0          : // fall
    case Bytecodes::_fload_1          : // fall
    case Bytecodes::_fload_2          : // fall
    case Bytecodes::_fload_3          :
    case Bytecodes::_dload            :
#if ENABLE_CPU_VARIANT && ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
    case Bytecodes::_dload_safe       :
#endif
    case Bytecodes::_dload_0          : // fall
    case Bytecodes::_dload_1          : // fall
    case Bytecodes::_dload_2          : // fall
    case Bytecodes::_dload_3          :
        return false;
    case Bytecodes::_aload_2          : // fall
    case Bytecodes::_aload_3          :
        local_index = code - Bytecodes::_aload_0;        
        break;
    case Bytecodes::_dup:
    case Bytecodes::_dup_x1:
    case Bytecodes::_dup_x2:
    case Bytecodes::_dup2:
    case Bytecodes::_dup2_x1:
    case Bytecodes::_dup2_x2:
    case Bytecodes::_iinc:
    case Bytecodes::_newarray:
    case Bytecodes::_anewarray:
    case Bytecodes::_multianewarray:
    case Bytecodes::_pop_and_npe_if_null:
#if !ENABLE_CPU_VARIANT
    case Bytecodes::_init_static_array:
#endif //!ENABLE_CPU_VARIANT
    case Bytecodes::_ifeq : // 153
    case Bytecodes::_ifne : // 154
    case Bytecodes::_iflt : // 155
    case Bytecodes::_ifge : // 156
    case Bytecodes::_ifgt : // 157
    case Bytecodes::_ifle : // 158
    case Bytecodes::_ifnull          :
    case Bytecodes::_ifnonnull       :
    case Bytecodes::_if_icmpeq:  // 159
    case Bytecodes::_if_icmpne:  // 160
    case Bytecodes::_if_icmplt:  // 161
    case Bytecodes::_if_icmpge:  // 162
    case Bytecodes::_if_icmpgt:  // 163
    case Bytecodes::_if_icmple:  // 164
    case Bytecodes::_if_acmpeq       :
    case Bytecodes::_if_acmpne       :
    case Bytecodes::_goto_w          :
    case Bytecodes::_ireturn:
    case Bytecodes::_lreturn:
    case Bytecodes::_freturn:
    case Bytecodes::_dreturn:
    case Bytecodes::_areturn:
    case Bytecodes::_return:
        return false;
    case Bytecodes::_getstatic:
      constant_index = get_java_ushort(bci+1);
      break;
    case Bytecodes::_invokestatic:
    case Bytecodes::_fast_invokespecial:
    case Bytecodes::_invokeinterface:
    case Bytecodes::_athrow:
    case Bytecodes::_new:
    case Bytecodes::_monitorenter:
    case Bytecodes::_monitorexit:
      return false;

#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_igetstatic: // fall through
    case Bytecodes::_fast_lgetstatic: // fall through
    case Bytecodes::_fast_fgetstatic: // fall through
    case Bytecodes::_fast_dgetstatic: // fall through
    case Bytecodes::_fast_agetstatic: // fall through
#else 
    case Bytecodes::_fast_1_getstatic: // fall through
    case Bytecodes::_fast_2_getstatic: // fall through
#endif
      constant_index = get_java_ushort(bci+1);
      break;

    case Bytecodes::_fast_bgetfield: // fall through
    case Bytecodes::_fast_sgetfield: // fall through
    case Bytecodes::_fast_igetfield: // fall through
    case Bytecodes::_fast_lgetfield: // fall through
    case Bytecodes::_fast_fgetfield: // fall through
    case Bytecodes::_fast_dgetfield: // fall through
    case Bytecodes::_fast_agetfield: // fall through
    case Bytecodes::_fast_cgetfield: {
      int offset = ENABLE_NATIVE_ORDER_REWRITING ? get_native_ushort(bci+1)
                                                 : get_java_ushort(bci+1);
      constant_index = offset ;
      break;
    }
    case Bytecodes::_fast_igetfield_1: {
      constant_index = get_ubyte(bci+1) ;
      break;
    }
    case Bytecodes::_fast_agetfield_1: {
      constant_index = get_ubyte(bci+1) ;
      break;
    }
    case Bytecodes::_iaload          : // fall
    case Bytecodes::_laload          : // fall
    case Bytecodes::_faload          : // fall
    case Bytecodes::_daload          : // fall
    case Bytecodes::_aaload          : // fall
    case Bytecodes::_baload          : // fall
    case Bytecodes::_caload          : // fall
    case Bytecodes::_saload          : {
        array_type = code - Bytecodes::_iaload;
        array_type_mask |= ( 1 << array_type);
        break;
    }
    case Bytecodes::_aload_0_fast_igetfield_1:
    case Bytecodes::_aload_0_fast_agetfield_1: {
      constant_index = get_ubyte(bci+1) ;
      local_index = 0;
      break;
    }
#if !ENABLE_CPU_VARIANT  
    case Bytecodes::_aload_0_fast_agetfield_4:
    case Bytecodes::_aload_0_fast_igetfield_4: {
      constant_index = 4;
      local_index = 0;
      break;
    }
    case Bytecodes::_aload_0_fast_agetfield_8:
    case Bytecodes::_aload_0_fast_igetfield_8: {
      constant_index = 8;
      local_index = 0;
      break;
    }
#endif        
   case Bytecodes::_fast_init_1_putstatic:
   case Bytecodes::_fast_init_2_putstatic:
   case Bytecodes::_fast_init_a_putstatic:
   case Bytecodes::_fast_init_1_getstatic:
   case Bytecodes::_fast_init_2_getstatic:
   case Bytecodes::_fast_init_invokestatic:
   case Bytecodes::_fast_init_new: {
        return true;
   }
    case Bytecodes::_fast_invokevirtual:
    case Bytecodes::_fast_invokestatic:
    case Bytecodes::_fast_invokeinterface:
    case Bytecodes::_fast_invokenative:
    case Bytecodes::_fast_new:
    case Bytecodes::_fast_anewarray:
      return false;
  // Most common bytecodes
  case Bytecodes::_invokespecial:
  case Bytecodes::_goto:
  case Bytecodes::_fast_invokevirtual_final:
    return false;
  case Bytecodes::_aload_1:
    local_index = 1;
    break;

  case Bytecodes::_aload:
    local_index = get_ubyte(bci+1);
    break;
  case Bytecodes::_invokevirtual:
    return false;
  case Bytecodes::_getfield:
    constant_index = get_java_ushort(bci+1);
    break;

  case Bytecodes::_iload:
    local_index = get_ubyte(bci+1);
    break;

  case Bytecodes::_aload_0:
    local_index = 0;
    break;

 case Bytecodes::_tableswitch:
 case Bytecodes::_lookupswitch:
 case Bytecodes::_wide:
    return false;
  }

  bci += length;

  //we can't track more compilicated case
  //so abort here.
  if ( local_index > 31 || constant_index > 31 ) {
    
    return false;
  }
  local_mask |= (1<< local_index);  
  constant_mask |= (1<<constant_index);
  
  }

  return true;
  
}

 bool Method::compare_bytecode(const jint begin_bci, const jint end_bci, jint cur_bci, jint& next_bci) {
  int length = Bytecodes::length_for(this, end_bci);
  length += end_bci - begin_bci;
  jubyte *bcptr = (jubyte*)code_base();
  if( jvm_memcmp( bcptr + begin_bci, bcptr + cur_bci, length) == 0 ) {
    next_bci = cur_bci + end_bci - begin_bci;
    return true;
  }
  return false;
 }


void Method::wipe_out_dirty_recorded_snippet(int bci, Bytecodes::Code code) {
  switch (code) {
    case Bytecodes::_istore_0         : // fall
    case Bytecodes::_istore_1         : // fall
    case Bytecodes::_istore_2         : // fall
    case Bytecodes::_istore_3         :
        RegisterAllocator::kill_by_locals(code - Bytecodes::_istore_0);
        break;

    case Bytecodes::_lstore           :
#if ENABLE_CPU_VARIANT && ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
    case Bytecodes::_lstore_safe      :
#endif
        RegisterAllocator::kill_by_locals(get_ubyte(bci+1));
        break;
    case Bytecodes::_lstore_0         : // fall
    case Bytecodes::_lstore_1         : // fall
    case Bytecodes::_lstore_2         : // fall
    case Bytecodes::_lstore_3         :
        RegisterAllocator::kill_by_locals(code - Bytecodes::_lstore_0);
        break;


    case Bytecodes::_astore           :
        RegisterAllocator::kill_by_locals(get_ubyte(bci+1));
        break;
    case Bytecodes::_astore_0         : // fall
    case Bytecodes::_astore_1         : // fall
    case Bytecodes::_astore_2         : // fall
    case Bytecodes::_astore_3         :
        RegisterAllocator::kill_by_locals(code - Bytecodes::_astore_0);
        break;

    case Bytecodes::_iinc:
      {
        RegisterAllocator::kill_by_locals(get_ubyte(bci+1));
      }
      break;
    case Bytecodes::_iastore          : // fall
    case Bytecodes::_lastore          : // fall
    case Bytecodes::_fastore          : // fall
    case Bytecodes::_dastore         : // fall
    case Bytecodes::_aastore         : // fall
    case Bytecodes::_bastore         : // fall
    case Bytecodes::_castore         : // fall
    case Bytecodes::_sastore         :
        
        RegisterAllocator::kill_by_array_type((1<<(code - Bytecodes::_iastore)));
        break;


    case Bytecodes::_putfield:
      RegisterAllocator::kill_by_fields(get_java_ushort(bci+1));
      break;
    case Bytecodes::_putstatic:
      RegisterAllocator::kill_by_fields(get_java_ushort(bci+1));
      break;



#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_iputstatic: // fall through
    case Bytecodes::_fast_lputstatic: // fall through
    case Bytecodes::_fast_fputstatic: // fall through
    case Bytecodes::_fast_dputstatic: // fall through
    case Bytecodes::_fast_aputstatic: // fall through
#else 
    case Bytecodes::_fast_1_putstatic: // fall through
    case Bytecodes::_fast_2_putstatic: // fall through
    case Bytecodes::_fast_a_putstatic: // fall through
#endif
      RegisterAllocator::kill_by_fields(get_java_ushort(bci+1));
      break;


    case Bytecodes::_fast_bputfield: // fall through
    case Bytecodes::_fast_sputfield: // fall through
    case Bytecodes::_fast_iputfield: // fall through
    case Bytecodes::_fast_lputfield: // fall through
    case Bytecodes::_fast_fputfield: // fall through
    case Bytecodes::_fast_dputfield: // fall through
    case Bytecodes::_fast_aputfield: {
      int offset = ENABLE_NATIVE_ORDER_REWRITING ? get_native_ushort(bci+1)
                                                 : get_java_ushort(bci+1);
      RegisterAllocator::kill_by_fields(offset);
      break;
    }



  case Bytecodes::_istore:
    RegisterAllocator::kill_by_locals(get_ubyte(bci+1));
    break;

  default:
    break;
  }
}


#endif
