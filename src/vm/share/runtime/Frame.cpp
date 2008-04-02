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

# include "incls/_precompiled.incl"
# include "incls/_Frame.cpp.incl"

extern "C" {
  void interpreter_deoptimization_entry();
  void invoke3_deoptimization_entry_0();
  void invoke3_deoptimization_entry_1();
  void invoke3_deoptimization_entry_2();
  void invoke5_deoptimization_entry_0();
  void invoke5_deoptimization_entry_1();
  void invoke5_deoptimization_entry_2();

  void invoke3_deoptimization_entry_3();
  void invoke3_deoptimization_entry_4();
  void invoke5_deoptimization_entry_3();
  void invoke5_deoptimization_entry_4();
}

bool Frame::_in_gc_state = false;

Frame* Frame::_last_frame = NULL;

#define STACK_LOCK_SIZE (StackLock::size() + 4)

void Frame::init(Thread* thread) {
  // Setup frame and stack pointers.
  GUARANTEE(thread->last_java_fp() != NULL && thread->last_java_sp() != NULL,
            "Last java fp and sp should be set");
  address  fp      = thread->last_java_fp();
  address  sp      = thread->last_java_sp();
  address* pc_addr = (address*)(sp + JavaStackDirection * BytesPerWord);
  set_values(thread, thread->stack_base(), pc_addr, sp, fp);

  // Add to the linked list of all frames
  push_frame();
}

void Frame::fixup_all_frames() { 
  // Run though the linked list of all frames and fix up those that point to
  // a thread that has been moved
  for (Frame* frame = _last_frame; frame != NULL; frame = frame->_previous) {
    frame->fixup_frame();
  }
}

void Frame::fixup_frame() {
  // If the stack base has moved, update all the fields appropriately.
  address old_stack_base = _stack_base;
  address new_stack_base = _thread->stack_base();
  int delta = new_stack_base - old_stack_base;
  if (delta != 0) {
    _fp += delta;
    _sp += delta;
    _pc_addr += delta;
    _stack_base = new_stack_base;
  }
}

ReturnOop JavaFrame::generate_stack_map(int& map_length) {
  AllocationDisabler no_allocation_allowed;

#if ENABLE_COMPILER
  if (is_compiled_frame()) {
    return generate_compiled_method_stack_map(map_length);
  } 
#endif

#ifndef AZZERT
  // In debug builds, we compare the stack map produced by StackmapGenerator
  // with the value in the tags, and make sure that the two are compatible.
  if (TaggedJavaStack) { 
    return generate_tagged_method_stack_map(map_length);
  } 
#endif

  Method::Raw m = method();
  StackmapList::Raw method_stackmaps = StackmapGenerator::find_verifier_map(&m);
  StackmapGenerator gen(&m);
  TypeArray::Raw map = gen.generate_stackmap(&method_stackmaps, bci());
  map_length = gen.abstract_stack_top();
  jint llength = local_length();
  jint elength = expression_length();

  if (bci_with_flags() & exception_frame_flag) {
    GUARANTEE(elength == 1, "Must have exactly one item on the stack");
    GUARANTEE(llength == m().max_locals(), "Frame has normal locals");
    map_length = llength + 1;
    map().ubyte_at_put(llength, obj_tag);
  } if (bci_with_flags() & overflow_frame_flag) {
    map_length = llength + 1;
    map().ubyte_at_put(llength, int_tag);
  } else { 
    GUARANTEE(llength == m().max_locals(), "Frame has normal locals");
    GUARANTEE(llength + elength <= map_length, "Sanity");
    // The generated stack map has too many items on the stack if we are in the
    // middle of an invoker.
    map_length = llength + elength;
  }

#ifdef AZZERT
  if (TaggedJavaStack) { 
    for (int i = map_length; --i >= 0; ) { 
      StackValue* sv = i < llength ? local_at(i, llength)
                                   : expression_at(map_length - i - 1, elength);
      Tag expected_tag =(Tag) map().byte_at(i);
      Tag actual_tag = (Tag)sv->tag();
      if (actual_tag != expected_tag && expected_tag == obj_tag) {
        GUARANTEE(i < llength, "Can't have tag conflict on the stack");
      }
    }
  }
#endif
  return map().obj();
}

#if ENABLE_COMPILER

ReturnOop JavaFrame::generate_compiled_method_stack_map(int& map_length) {
  GUARANTEE(!ENABLE_C_INTERPRETER, 
            "C interpreter should never execute compiled methods");
  TypeArray::Raw map = Universe::gc_block_stackmap();

#if ENABLE_EMBEDDED_CALLINFO
  CallInfo* call_info = cooked_call_info();
  GUARANTEE(call_info->in_compiled_code(), "Must be compiled method");

  bool compact_tags = (call_info->is_compact_format());
#endif // ENABLE_EMBEDDED_CALLINFO
  jint llength = local_length();
  jint elength = expression_length();

  map_length = llength + elength;

#if ENABLE_EMBEDDED_CALLINFO
  Tag (CallInfo::*tagger)(int index);
  int end_index;
  if (compact_tags) {
    end_index = CallInfo::format1_tag_width;
    tagger = &CallInfo::compact_tag_at;
  } else {
    end_index = llength + elength;
    tagger = &CallInfo::extended_tag_at;
  }
  int first_stack_index = end_index - elength;
  int first_index = first_stack_index - llength;

  for (int tag_index = end_index - 1; tag_index >= first_index; tag_index--) {
#if ENABLE_THUMB_VM && defined(__GNUC__)
    // For some reason GCC 3.0.x couldn't compile the function pointer
    // call correctly.
    Tag stack_tag;
    if (compact_tags) {
      stack_tag = call_info->compact_tag_at(tag_index);
    } else {
      stack_tag = call_info->extended_tag_at(tag_index);
    }
#else
    Tag stack_tag = (call_info->*tagger)(tag_index);  
#endif
    if (tag_index >= first_stack_index) {
      int expression_index = end_index - tag_index - 1;
      int map_index = map_length - expression_index - 1;
#ifdef AZZERT
      int map_index2 = map_length - end_index + tag_index;
      GUARANTEE(map_index == map_index2, "Arithmetic!");
#endif
      map().byte_at_put(map_index, (jbyte)stack_tag);
    } else { 
      int local_index = tag_index - first_index;
      map().byte_at_put(local_index, (jbyte)stack_tag);
    }
  }
#endif // ENABLE_EMBEDDED_CALLINFO

#if ENABLE_APPENDED_CALLINFO
  {
    AllocationDisabler no_allocation_allowed;

    const address frame_pc = cooked_pc();

    CompiledMethod::Raw cm = find_compiled_method(frame_pc);

    CallInfoRecord callinfo(&cm, frame_pc);

    for (int i = 0; i < map_length; i++) {
#if ENABLE_EMBEDDED_CALLINFO
      // Check that embedded and appended callinfo produce the same stack map.
      GUARANTEE((map().byte_at(i) == obj_tag) 
                == callinfo.oop_tag_at(i), "Stack maps must be the same");
#endif // ENABLE_EMBEDDED_CALLINFO

      const Tag tag = callinfo.oop_tag_at(i) ? obj_tag : int_tag;
      map().byte_at_put(i, (jbyte)tag);
    }
  }
#endif // ENABLE_APPENDED_CALLINFO

  return map().obj();
}
#endif


ReturnOop JavaFrame::generate_tagged_method_stack_map(int& map_length) { 
#if TaggedJavaStack
  GUARANTEE(TaggedJavaStack, "Can't get tags if there aren't any!");
  TypeArray::Raw map = Universe::gc_block_stackmap();

  jint llength = local_length();
  jint elength = expression_length();

  map_length = llength + elength;
    
  for (int i = map_length; --i >= 0; ) { 
    StackValue* sv = i < llength ? local_at(i, llength)
                                 : expression_at(map_length - i - 1, elength);
    map().byte_at_put(i, sv->tag());
  }
  return map().obj();
#else
  (void)map_length;
  return NULL;
#endif
}


#if ENABLE_COMPILER
#if ENABLE_APPENDED_CALLINFO

address JavaFrame::cooked_pc( void ) const {
  address frame_pc = pc();

  return in_gc_state() && is_compiled_frame()
    ? DERIVED(address, frame_pc, raw_compiled_method())
    : frame_pc;
}

CompiledMethodDesc* 
JavaFrame::find_compiled_method( const address frame_pc ) {
  CompiledMethod::Raw cm = CompiledMethodCache::item_for_address(frame_pc);
  
  if (cm.is_null()) {
    cm = ROM::compiled_method_from_address(frame_pc);
  }
  GUARANTEE(cm.not_null(), "Sanity");

  // If we use a CompilerArea, during compaction of the compiler area
  // result->_klass contains the offset.
/*  GUARANTEE((ObjectHeap::is_gc_active() && CompilerAreaPercentage != 0) ||
            (frame_pc >= (address)cm.obj() && 
             frame_pc <  (address)cm.obj() + cm().object_size()), "Sanity");*/
#if ENABLE_INLINE || ENABLE_TRAMPOLINE
  jint offset = DISTANCE(cm().entry(), frame_pc);
  CompiledMethodDesc* method_desc = (CompiledMethodDesc*)cm().obj();
  jint code_size = method_desc->code_size();
  if ( code_size < offset && offset > 0 ) {
        //this is a unlinked method which is still be used
        method_desc = ObjectHeap::method_contains_instruction_of(frame_pc);
        CompiledMethod::Raw ncm = method_desc;
        return method_desc;
  }
#endif  
  return (CompiledMethodDesc*)cm.obj();
}

bool JavaFrame::in_compiled_code( const address frame_pc ) {
  return CompiledMethodCache::has_index(((const CompiledMethodDesc*)frame_pc))
    || ROM::in_any_loaded_bundle_of_current_task((const OopDesc*)frame_pc);
}

#endif // ENABLE_APPENDED_CALLINFO

void JavaFrame::fill_in_compiled_frame() {
  // Warning: This takes place during GC, so do not create handles

  // Fill in the compiled method pointer.
#if ENABLE_APPENDED_CALLINFO
  set_raw_compiled_method(find_compiled_method(cooked_pc()));

#if ENABLE_EMBEDDED_CALLINFO
  // Check that appended callinfo is consistent with emdedded callinfo.
  GUARANTEE(raw_compiled_method() == cooked_call_info()->compiled_method(),
            "Consistency check");
#endif // ENABLE_EMBEDDED_CALLINFO

#else // ENABLE_APPENDED_CALLINFO

  set_raw_compiled_method(cooked_call_info()->compiled_method());

#endif // ENABLE_APPENDED_CALLINFO

  GUARANTEE(raw_compiled_method()->is_compiled_method(),
            "must have a compiled method by now")

  // Fill in the locals pointer.
  *(address*)(fp() + locals_pointer_offset()) = calculated_locals_pointer();

  // Fill in stack bottom pointer
  Method::Raw m = raw_compiled_method()->method();
  const AccessFlags flags = m().access_flags();
  if (!flags.is_synchronized() && !flags.has_monitor_bytecodes() ) {
    set_empty_stack_bottom_pointer();
  }
}

void JavaFrame::set_empty_stack_bottom_pointer() {
  *(address*) (fp() + stack_bottom_pointer_offset()) =
                         fp() + empty_stack_offset();
}

void JavaFrame::deoptimize() {
  Method::Raw no_method;
  deoptimize(&no_method);
}

void JavaFrame::deoptimize(const Method * callee) {
  // Deoptimizing interpreted frames is trivial - simply return.
  if (!is_compiled_frame()) { 
    return;
  }

  // Fill in the compiled frame.
  if (!in_gc_state()) { 
    fill_in_compiled_frame();
  }

  // Put in the appropriate tags
  if (TaggedJavaStack) { 
    jint llength = local_length();
    jint map_length, i;
    TypeArray::Raw map = generate_compiled_method_stack_map(map_length);

    for (i = map_length; --i >= 0; ) { 
      Tag tag = (Tag)map().ubyte_at(i);
      StackValue* sv = i < llength ? local_at(i, llength)
                                   : expression_at(map_length - i - 1);
      sv->set_tag(tag);
    }
#ifdef AZZERT
    verify();
#endif
  }

  // Get the method and the bytecode index from the frame.
  Method::Raw method = this->method();
  int bci = this->bci();

#if 0 && USE_INDIRECT_EXECUTION_SENSOR_UPDATE
  // IMPL_NOTE: if this is ncessary here?
  CompiledMethod::Raw cm = method().compiled_code();
  method().set_compiled_execution_entry(cm().entry());
#endif

  // Overwrite the compiled method in the frame with the method.
  set_raw_method((MethodDesc*) method.obj());

  if (in_gc_state()) { 
    // Set the appropriate bci.
    set_raw_bcp((address)bci);
  } else { 
    // Set the appropriate bcp for this function
    set_raw_bcp(bci + method().code_base());
    // Set the cpool entry
    set_cpool(DERIVED(address, method().constants(),
                      ConstantPool::base_offset()));
  }

  address deoptimization_pc;
  if (callee->is_null()) { 
    deoptimization_pc = (address) interpreter_deoptimization_entry;
  } else { 
    GUARANTEE(callee->not_null(), "Sanity");
    static const address invoke3_deoptimization_entries[] = {
      (address)invoke3_deoptimization_entry_0,
      (address)invoke3_deoptimization_entry_1,
      (address)invoke3_deoptimization_entry_2,
      (address)invoke3_deoptimization_entry_3,
      (address)invoke3_deoptimization_entry_4,
    };
    static const address invoke5_deoptimization_entries[] = {
      (address)invoke5_deoptimization_entry_0,
      (address)invoke5_deoptimization_entry_1,
      (address)invoke5_deoptimization_entry_2,
      (address)invoke5_deoptimization_entry_3,
      (address)invoke5_deoptimization_entry_4,
    };

    const Bytecodes::Code bc = method().bytecode_at(bci);
    GUARANTEE(bc == Bytecodes::_invokevirtual || 
              bc == Bytecodes::_invokespecial || 
              bc == Bytecodes::_invokestatic  ||
              bc == Bytecodes::_invokeinterface ||
              bc == Bytecodes::_fast_invokevirtual ||
              bc == Bytecodes::_fast_invokestatic || 
              bc == Bytecodes::_fast_init_invokestatic || 
              bc == Bytecodes::_fast_invokeinterface || 
              bc == Bytecodes::_fast_invokevirtual_final || 
              bc == Bytecodes::_fast_invokespecial, "Must be invoker");
    GUARANTEE(Bytecodes::length_for(bc) == 3 ||
              Bytecodes::length_for(bc) == 5, "Sanity check");
    int ret_type = callee->return_type();
    GUARANTEE(0 <= ret_type && ret_type <= 4, "sanity");
    deoptimization_pc = Bytecodes::length_for(bc) == 3
                            ? invoke3_deoptimization_entries[ret_type]
                            : invoke5_deoptimization_entries[ret_type];
  }
  set_pc(deoptimization_pc);
}

#if ENABLE_JAVA_DEBUGGER
void JavaFrame::deoptimize_and_continue(bool adjust_bci) {
  // called by the debugger code to continue on in the interpreter
  UsingFastOops fast_oops;

  deoptimize();
  Method::Fast method = this->method();
  Bytecodes::Code code = method().bytecode_at(this->bci());
  if (adjust_bci && (code == Bytecodes::_invokevirtual ||
      code == Bytecodes::_invokespecial ||
      code == Bytecodes::_invokestatic  ||
      code == Bytecodes::_invokeinterface ||
      (code >= Bytecodes::_fast_invokevirtual &&
       code <= Bytecodes::_fast_invokespecial))) {
    set_raw_bcp(raw_bcp() + Bytecodes::length_for(code));
  }
}
#endif

bool JavaFrame::is_compiled_frame( void ) const {
#if ENABLE_C_INTERPRETER
  return false;
#else
  if (in_gc_state()) { 
    // In gc state, the high bit indicates a compiled frame
    return ((jint)raw_bcp() & compiled_frame_flag) != 0;
  } else {
#if ENABLE_APPENDED_CALLINFO

#if ENABLE_EMBEDDED_CALLINFO
    // Check that appended callinfo is consistent with embedded callinfo.
    GUARANTEE(raw_call_info()->in_compiled_code() == in_compiled_code(pc()), 
              "Consistency check");
#endif // ENABLE_EMBEDDED_CALLINFO
    return in_compiled_code(pc());

#else // ENABLE_APPENDED_CALLINFO

    return raw_call_info()->in_compiled_code();

#endif // ENABLE_APPENDED_CALLINFO
  }
#endif // ENABLE_C_INTERPRETER
}

#if ENABLE_CODE_PATCHING
bool JavaFrame::is_heap_compiled_frame() const {
  GUARANTEE(ENABLE_APPENDED_CALLINFO, "Is not supported for embeded callinfo");
  GUARANTEE(!in_gc_state(), "Wrong state");

  return CompiledMethodCache::has_index((const CompiledMethodDesc*)pc());
}
#endif // ENABLE_CODE_PATCHING

#endif // ENABLE_COMPILER

#if ENABLE_EMBEDDED_CALLINFO
CallInfo* JavaFrame::cooked_call_info( void ) const {
  CallInfo* call_info = raw_call_info();

  return in_gc_state() && is_compiled_frame()
     ? DERIVED(CallInfo*, call_info, raw_compiled_method())
     : call_info;
}
#endif // ENABLE_EMBEDDED_CALLINFO

ReturnOop JavaFrame::method() const {
#if ENABLE_COMPILER
  if (is_compiled_frame()) {
    CompiledMethod::Raw cm = compiled_method();
    return cm().method();
  }
#endif
  GUARANTEE(in_gc_state() || raw_method()->is_method(), "Sanity check");
  return raw_method(); 
}

jint JavaFrame::local_length() const {
  const address caller_sp   = this->caller_sp();
  const address after_last_local = fp() + end_of_locals_offset();
  return (after_last_local - caller_sp) * JavaStackDirection
                      / BytesPerStackElement;
}

address JavaFrame::locals_pointer() const {
#if ENABLE_COMPILER
  if (!in_gc_state() && is_compiled_frame()){
    return calculated_locals_pointer();
  } 
#endif
  return *(address*) (fp() + locals_pointer_offset());
}

#if ENABLE_COMPILER

address JavaFrame::calculated_locals_pointer() const {
  CompiledMethod::Raw cm = compiled_method();
  Method::Raw method = cm().method();
  address caller_sp = fp() + end_of_locals_offset()
       - method().max_locals() * JavaStackDirection * BytesPerStackElement;
  address locals_pointer = caller_sp + arg_offset_from_sp(-1);
  return locals_pointer;
}


#endif

StackValue* JavaFrame::local_at(int index, int length) const {
  GUARANTEE(index < (length > 0 ? length : local_length()),
            "Index out of bounds");
  (void)length;
  return (StackValue*) (locals_pointer()
                         + index * JavaStackDirection * BytesPerStackElement);
}

StackValue* JavaFrame::expression_at(int index, int length) {
  GUARANTEE(index < (length > 0 ? length : expression_length()),
            "Index out of bounds");
  (void)length;
  address stack_address_top = sp();
  int offset = arg_offset_from_sp(index);
  return (StackValue*) (stack_address_top + offset);
}

jint JavaFrame::expression_length() {
  address stack_pointer       = sp();
  address empty_stack_pointer = stack_bottom_pointer();
  return JavaStackDirection * (stack_pointer - empty_stack_pointer)/
                                       BytesPerStackElement;
}

StackLock* JavaFrame::stack_lock_at(int index, int length) {
  GUARANTEE(index < (length > 0 ? length : stack_lock_length()), 
            "Index out of bounds");
  (void)length;
  address stack_lock_base = fp() + JavaFrame::first_stack_lock_offset();
  return (StackLock*)
      (stack_lock_base + JavaStackDirection * index);
}

jint JavaFrame::stack_lock_length() {
  address stack_lock_top  = stack_bottom_pointer();
  address stack_lock_base = (address) (fp() + empty_stack_offset());
  return JavaStackDirection * (stack_lock_top - stack_lock_base);
}

address JavaFrame::stack_bottom_pointer() {
#if ENABLE_COMPILER
  // Lazily computed for compiled frames in normal mode
  if (!in_gc_state() && is_compiled_frame()) {
    Method::Raw m = method();
    if (!m().uses_monitors()) {
      set_empty_stack_bottom_pointer();
    }
  }
#endif
  return *(address*) (fp() + stack_bottom_pointer_offset());
}

/*
void JavaFrame::caller_is(Frame& result) {
  address  caller_fp      = *(address*) (fp() + caller_fp_offset());
  address  caller_sp      = this->caller_sp();
  address* caller_pc_addr = (address*) (fp() + return_address_offset());
  result.set_values(_thread, _stack_base, caller_pc_addr, caller_sp , caller_fp);
}
*/

void JavaFrame::relocate_internal_pointers(int delta, bool do_locks) {
  address caller_fp    = *(address*)(fp() + caller_fp_offset());
  address locals       = *(address*)(fp() + locals_pointer_offset());
  address stack_bottom = *(address*)(fp() + stack_bottom_pointer_offset());
  if (do_locks) {
    // called as a result of grow execution stack
    jint stack_lock_len = stack_lock_length();
    for (int i = 0; i < stack_lock_len; i += STACK_LOCK_SIZE) {
      // Pass in length to make out-of-bounds index check work
      StackLock* lock = stack_lock_at(i, stack_lock_len);
      lock->relocate_internal_pointers(delta);
    }
  }

  *(address*)(fp() + caller_fp_offset())            = caller_fp    + delta;
  *(address*)(fp() + locals_pointer_offset())       = locals       + delta;
  *(address*)(fp() + stack_bottom_pointer_offset()) = stack_bottom + delta;
}

/*
void JavaFrame::relocate_starting_frame_pointers(Thread *thread, int delta) {
}
*/

jint JavaFrame::bci_with_flags() const { 
  if (in_gc_state()) { 
    return ((jint)raw_bcp());
  }  
#if ENABLE_COMPILER
  if (is_compiled_frame()) {
    // Compiled frames don't have flags

#if ENABLE_APPENDED_CALLINFO
    const address frame_pc = cooked_pc();
    CompiledMethod::Raw compiled_method = find_compiled_method(frame_pc);
    CallInfoRecord callinfo(&compiled_method, frame_pc);
    const jint bci = callinfo.bci();

#if ENABLE_EMBEDDED_CALLINFO
    // Check that appended callinfo is consistent with embedded callinfo.
    GUARANTEE(raw_call_info()->bci() == bci, "Consistency check");
#endif // ENABLE_EMBEDDED_CALLINFO

    return bci;
#else // ENABLE_APPENDED_CALLINFO
    return raw_call_info()->bci();
#endif // ENABLE_APPENDED_CALLINFO
  } 
#endif
  Method::Raw m = method();
  return raw_bcp() - m().code_base();
}

void JavaFrame::oops_do(void do_oop(OopDesc**)) {
  GUARANTEE(in_gc_state(), "We can only execute oops_do in gc state");

  // Compute length info before visiting method
  jint llength = local_length();
  jint slength = stack_lock_length();
  // Expression stack
  jint map_length, i;

  TypeArray::Raw map = generate_stack_map(map_length);

  // Visit method >>after<< we generate the stack map
  OopDesc** method_address = (OopDesc**) (fp() + method_offset());
  do_oop(method_address);

  // Locals and expression stack
  for (i = map_length; --i >= 0; ) { 
    if (map().byte_at(i) == obj_tag) { 
      StackValue* sv = i < llength ? local_at(i, llength)
                                   : expression_at(map_length - i - 1);
      do_oop(sv->obj_addr());
    }
  }

  // Stack locks
  for (i = slength - STACK_LOCK_SIZE; i >= 0; i-= STACK_LOCK_SIZE) {
    // Pass in length to make out-of-bounds index check work
    StackLock* lock = stack_lock_at(i, slength);
    lock->oops_do(do_oop);
    do_oop(lock->owner_address());
  }
}

void JavaFrame::gc_prologue(void do_oop(OopDesc**)) {
#if ENABLE_COMPILER && !ENABLE_C_INTERPRETER

#if ENABLE_EMBEDDED_CALLINFO
  CallInfo *call_info = raw_call_info();
  (void)call_info;
#endif // ENABLE_EMBEDDED_CALLINFO

#if ENABLE_APPENDED_CALLINFO
#if ENABLE_EMBEDDED_CALLINFO
  // Check that appended callinfo is consistent with embedded callinfo.
  GUARANTEE(call_info->in_compiled_code() == in_compiled_code(pc()), 
            "Consistency check");
#endif // ENABLE_EMBEDDED_CALLINFO
  const address frame_pc = pc();
  if (in_compiled_code(frame_pc)) {
    CompiledMethodDesc* cm = find_compiled_method(frame_pc);
#if ENABLE_EMBEDDED_CALLINFO
    // Check that appended callinfo is consistent with embedded callinfo.
    GUARANTEE(cm == call_info->compiled_method(), 
              "Consistency check");
#endif // ENABLE_EMBEDDED_CALLINFO
    // Make pc for the compiled frame relative to compiled method start
    int delta = DISTANCE(cm, frame_pc);
#else // ENABLE_APPENDED_CALLINFO

  if (call_info->in_compiled_code()) {
    CompiledMethodDesc* cm = call_info->compiled_method();
    // Make pc for the compiled frame relative to compiled method start
    int delta = DISTANCE(cm, pc());
#endif // ENABLE_APPENDED_CALLINFO

    set_raw_compiled_method(cm);
    set_raw_pc((address)delta);

#if ENABLE_APPENDED_CALLINFO
    CompiledMethod::Raw compiled_method(cm);
    CallInfoRecord callinfo(&compiled_method, frame_pc);
    const jint bci = callinfo.bci();
#if ENABLE_EMBEDDED_CALLINFO
    // Check that appended callinfo is consistent with embedded callinfo.
    GUARANTEE(bci == call_info->bci(), "Consistency check");
#endif // ENABLE_EMBEDDED_CALLINFO    
    set_raw_bcp((address)( bci + compiled_frame_flag));
#else // ENABLE_APPENDED_CALLINFO
    set_raw_bcp((address)( call_info->bci() + compiled_frame_flag));
#endif // ENABLE_APPENDED_CALLINFO

    GUARANTEE(delta >= 0 && (juint)delta < cm->object_size(), "Sanity");
    // Since GC_state has already been set to true, the following must occur
    // >>after<< we have finished modifying our pc()
    fill_in_compiled_frame();
  } else 
#endif
  {
    MethodDesc* rm     = raw_method();
    // Make bci relative to method code start.
    // But be sure we accurately remember any flags passed to us.
    address frame_bcp  = raw_bcp();
    jint    frame_bci  = DISTANCE(rm, frame_bcp);
    jint    actual_bci = frame_bci & actual_bci_mask;
    address actual_bcp = DERIVED(address, rm, actual_bci);
    jint    flags      = (int)frame_bcp - (int)actual_bcp;
    GUARANTEE(actual_bci >= Method::base_offset(), "Sanity check");
    GUARANTEE((juint)actual_bci < rm->object_size(), 
              "Must be within appropriate limits");
    set_raw_bcp((address)(flags + (actual_bci - Method::base_offset())));
    // This is just to make sure that we really do fix it up after the GC.
    AZZERT_ONLY(set_cpool((address)0xdeadc0de));
  }

  // Reverse headers for locked objects
  jint stack_lock_len = stack_lock_length();
  for (int i = 0; i < stack_lock_len; i += STACK_LOCK_SIZE) {
    StackLock* lock = stack_lock_at(i);
    lock->reverse_locked_header();
  }
  oops_do(do_oop);
}

void JavaFrame::gc_epilogue(void) {
  // Restore headers for locked objects
  jint slength = stack_lock_length();
  for (int i = 0; i < slength; i += STACK_LOCK_SIZE) {
    StackLock* lock = stack_lock_at(i);
    lock->restore_locked_header();
  }

#if ENABLE_COMPILER && !ENABLE_C_INTERPRETER
  if (is_compiled_frame()) {
    // Make pc for compiled frame absolute again
#if ENABLE_APPENDED_CALLINFO
#if ENABLE_EMBEDDED_CALLINFO
    GUARANTEE(cooked_call_info()->in_compiled_code() 
              == in_compiled_code(cooked_pc()), "Sanity check");
#endif // ENABLE_EMBEDDED_CALLINFO
    GUARANTEE(in_compiled_code(cooked_pc()), "Sanity check");
#else // ENABLE_APPENDED_CALLINFO
    GUARANTEE(cooked_call_info()->in_compiled_code(), "Sanity check");
#endif // ENABLE_APPENDED_CALLINFO
    GUARANTEE(raw_compiled_method()->is_compiled_method(), "sanity check");
    GUARANTEE((juint)raw_bcp() & compiled_frame_flag,  "Quick check works");
    set_raw_pc(DERIVED(address, raw_compiled_method(), pc()));
  } else 
#endif
  {
    Method::Raw method = raw_method();
    int     saved_bcp  = (int)raw_bcp();
    int     flags      = saved_bcp & ~actual_bci_mask;
    int     actual_bci = (saved_bcp & actual_bci_mask) + Method::base_offset();
    address actual_bcp = DERIVED(address, method.obj(), actual_bci);
    address frame_bcp  = (address)((int)actual_bcp + flags);
    set_raw_bcp(frame_bcp);

    set_cpool(DERIVED(address, 
                      method().constants(), ConstantPool::base_offset()));
  }
}

/*
void EntryFrame::caller_is(Frame& result) const {
  address caller_fp       = *(address*)(fp() + stored_last_fp_offset());
  address caller_sp       = *(address*)(fp() + stored_last_sp_offset());
  address* caller_pc_addr =  (address*)(caller_sp + JavaStackDirection * (int)sizeof(jint));
  result.set_values(_thread, _stack_base, caller_pc_addr, caller_sp , caller_fp);
}
*/
void EntryFrame::relocate_internal_pointers(int delta) {
  address stored_fp   = *(address*) (fp() + stored_last_fp_offset());
  address stored_sp   = *(address*) (fp() + stored_last_sp_offset());
  if (stored_fp != NULL) {
    GUARANTEE(stored_sp != NULL, "Sanity check");
    *(address*)(fp() + stored_last_fp_offset()) = stored_fp + delta;
    *(address*)(fp() + stored_last_sp_offset()) = stored_sp + delta;
  } else {
    GUARANTEE(stored_sp == NULL, "Sanity check");
  }
}

void EntryFrame::oops_do(void do_oop(OopDesc**)) {
  GUARANTEE(fp() + EntryFrame::empty_stack_offset() == sp(), 
            "No stack elements");
  do_oop((OopDesc**)(fp() + EntryFrame::stored_obj_value_offset()));
  do_oop((OopDesc**)(fp() + EntryFrame::pending_exception_offset()));
  do_oop((OopDesc**)(fp() + EntryFrame::pending_activation_offset()));
}


bool
JavaFrame::find_exception_frame(Thread* thread,
                                JavaOop* original_exception JVM_TRAPS) {
  UsingFastOops fast_oops;
  JavaOop::Fast   exception = original_exception->obj();
  JavaClass::Fast exception_class = exception.blueprint();
  bool killing_thread = false;
  bool sticky_exception;

  jint bci;
  jint exception_bci = -1;
  Method::Fast method;

  Frame frame(*this);
  if (exception.equals(Universe::string_class())) {
    killing_thread = true;
  } 
#if ENABLE_ISOLATES
  if (exception_class.equals(Universe::isolate_class())) {
    killing_thread = true;
  } 
#endif
  sticky_exception = killing_thread;

  if (_debugger_active) {
    if (!sticky_exception) {
      handle_exception_info(thread);
    }
  }

  if (frame.is_java_frame() &&
        (frame.as_JavaFrame().bci_with_flags() & overflow_frame_flag) != 0) {
    // There is a 'fake' frame constructed in
    // interpreter_grow_stack.
    // We need to the frame because we don't want to find
    // a handler in this frame since we don't have any space for locals
    // allocated.
    frame.as_JavaFrame().caller_is(frame);
  }

  for(;;) { 
    if (frame.is_entry_frame()) {
      if (frame.as_EntryFrame().is_first_frame()) {
        // We cannot go before the entry frame
        bci = -1; 
        break;
      } else {
        // These never have exception handlers.  Just ignore them
        frame.as_EntryFrame().caller_is(frame);
      }
    } else {
      bool monitor_error = false;
      method = frame.as_JavaFrame().method();
      bci = -1;
      if (!killing_thread) { 
        exception_bci = frame.as_JavaFrame().bci();
        // We just ignore any exceptions, and return -1 if that happens
        bci = method().exception_handler_bci_for(exception_class(),
                                                 exception_bci JVM_CHECK_0);
      }
      if (bci >= 0) { 
        break;
      }
      int slength = frame.as_JavaFrame().stack_lock_length();
      // Synchronized methods that are not in the middle of processing a
      // stack overflow (so that they can build their own frame) must have
      // a locked monitor in slot 0.
      if (method().access_flags().is_synchronized() && 
         ((frame.as_JavaFrame().bci_with_flags() & overflow_frame_flag) == 0)) {
        if (slength == 0) {
          monitor_error = true;
        } else { 
          StackLock* lock = frame.as_JavaFrame().stack_lock_at(0, slength);
          if (lock->owner() == NULL) { 
            monitor_error = true;
          } else { 
            if (TraceThreadsExcessive) {
              TTY_TRACE_CR(("unlock: thread 0x%x", thread->obj()));
            }
            Synchronizer::unlock_stack_lock(lock);
          }
        }
      }
      for (int i = 0; i < slength; i += STACK_LOCK_SIZE) {
        StackLock* lock = frame.as_JavaFrame().stack_lock_at(i, slength);
        if (lock->owner() != NULL) {
          monitor_error = true;
          Synchronizer::unlock_stack_lock(lock);
        }
      }
      if (monitor_error && !sticky_exception) { 
        sticky_exception = true;
        illegal_monitor_state_exception(JVM_SINGLE_ARG_NO_CHECK);
        // Note, we may get a monitor state exception, or we may get an
        // OutOfMemoryError.
        exception = Thread::current_pending_exception();
        exception_class = exception.blueprint();
        Thread::clear_current_pending_exception();
        // We specifically do not go to the caller, but reexecute the loop
        // in this frame.
      } else { 
        frame.as_JavaFrame().caller_is(frame);
      }
    }
  }

  // If the debugger is running we must first call up to the debugger with
  // the stack in its current state.
  if (_debugger_active) {
    if (!sticky_exception) {
      if (bci >= 0) {
        VMEvent::handle_caught_exception(thread, (JavaFrame*)&frame, bci,
                                         &exception);
      } else {
        VMEvent::handle_uncaught_exception(thread, &exception);
      }
    }
  }
  // Update the end of the stack, which always points to the correct value
  // for fp.
  address* stack_pointer = (address*)thread->stack_pointer();
  stack_pointer[0] = frame.fp();
  if (bci >= 0) {
#if ENABLE_COMPILER
    frame.as_JavaFrame().deoptimize();
#endif
    thread->set_last_java_fp(frame.fp());
    // Indicate that the stack is empty.  Assembly code will push the exception
    // if necessary.  (We can't do this since we can't be guaranteed there
    // is space.  What if this is the topmost frame, and it didn't have any
    // stack previously?)
    address empty_stack =
      *(address*)(frame.fp() + JavaFrame::stack_bottom_pointer_offset());
    address one_item_stack =
        empty_stack + JavaStackDirection * BytesPerStackElement;

    thread->set_last_java_sp(one_item_stack);
    // We need to make a new frame object based on the new values in
    // last_java_fp() and last_java_sp().  It will have a different value
    // for pc_addr.
    JavaFrame new_frame(thread);
    new_frame.set_pc((address)interpreter_deoptimization_entry); 
    new_frame.set_raw_bcp(bci + method().code_base());
    GUARANTEE(new_frame.expression_length() == 1, "Proper frame");
    *(new_frame.expression_at(0, 1)->as_oop()) = exception;

#if ENABLE_COMPILER
    new_frame.osr_replace_frame(bci);
#endif
#if ENABLE_WTK_PROFILER
    WTKProfiler::record_exception(thread, &new_frame);
#endif

    GUARANTEE(exception_bci >= 0, 
              "Exception bci must be calculated at this point");
    
    if (bci <= exception_bci) {
      timer_tick();
    }

    return true;
  } else { 
    // This thread is finished.  We only have the final EntryFrame.
    thread->set_last_java_fp(frame.fp());
    thread->set_last_java_sp(frame.fp() + EntryFrame::empty_stack_offset());
    frame.as_EntryFrame().pending_exception() = exception;    
#if ENABLE_WTK_PROFILER
    WTKProfiler::record_exception(thread, &frame);
#endif
    // return null as a signal that this is really the entry frame.
    return false;
  }
}

#if ENABLE_COMPILER

#ifndef PRODUCT
// compiler_tracer is used for setting break points to single-step the
// compiled method. To do that on ARM, set a break point at compiler_tracer
// in your debugger. Whenever compiler_tracer() is called, set a break point
// at the address contained in r0.
extern "C" void compiler_tracer(address pc);
#endif

void JavaFrame::osr_replace_frame(jint bci) {
  UsingFastOops fast_oops;
  Method::Fast m = method();
  CompiledMethod::Fast cm;
  if (m().has_compiled_code()) {
    bool found_osr_entry = false;
    cm = m().compiled_code();
    for (RelocationReader stream(&cm);
            !found_osr_entry && !stream.at_end();
            stream.advance()) {
      if (stream.is_osr_stub()) {
        if (stream.current(1) == bci) {
#if !ENABLE_THUMB_COMPILER
          address ret_adr = cm().entry() + stream.code_offset();
#else
          address ret_adr = cm().entry() + stream.code_offset() + 1;
#endif
          set_pc(ret_adr);
          NOT_PRODUCT(compiler_tracer(ret_adr));
          found_osr_entry = true;
        }
      }
    }
    
#ifndef PRODUCT
    if (found_osr_entry) { 
      if (TraceOSR) {
        tty->print("[+OSR: ");
        m().print_name_on(tty);
        tty->print_cr(" (bci=%d)]", bci);
      }
    } else {
      compiler_tracer(cm().entry());
    }
#endif
  }
}

#endif

#if !defined(PRODUCT)

void StackValue::verify() {
  switch(tag()) {
    case int_tag:
    case float_tag:
    case long_tag:
    case long2_tag:
    case double_tag:
    case double2_tag:
    case ret_tag:
    case uninitialized_tag:
      return;
    case obj_tag: {
      JavaOop::Raw value = as_obj();
      GUARANTEE(value.is_null() || value.is_java_oop(),
                "Must be java object");
      AZZERT_ONLY_VAR(value);
      break;
    }
    default:
      SHOULD_NOT_REACH_HERE();
  }
}

#endif

#if !defined(PRODUCT)

void JavaFrame::verify() {
  // Verify expression stack
  jint elength = expression_length();
  jint llength = local_length();
  int i;
  int map_length;
  TypeArray::Raw map = generate_stack_map(map_length);
  for (i = map_length; --i >= 0; ) { 
    if (map().byte_at(i) == obj_tag) { 
      StackValue* sv = i < llength ? local_at(i, llength)
                                   : expression_at(map_length - i - 1,elength);
      JavaOop::Raw value = sv->as_obj();
      GUARANTEE(value.is_null() || value.is_java_oop(), "Must be java object");
      AZZERT_ONLY_VAR(value);
    }
  }
}

#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

void StackValue::print_on(Stream* st, jint tag) {
  if (is_int(tag)) {
    st->print("(int) %d", as_int());
  } else if (is_long2(tag)) {
    st->print("(long) ");
    st->print(OsMisc_jlong_format_specifier(), as_long());
#if ENABLE_FLOAT
  } else if (is_float(tag)) {
    // promote
    st->print("(float) %f", jvm_f2d(as_float()));
  } else if (is_double2(tag)) {
    st->print("(double) %f", as_double());
#endif
  } else if (is_obj(tag)) {
    Oop obj = as_obj();
    obj.print_value_on(st);
  } else if (is_ret(tag)) {
    st->print("(return bci) %d", as_ret());
  } else if (is_uninitialized(tag)) {
    st->print("(uninitialized)");
  } else {
    st->print("(error in tag [%d])", tag);
  }
  st->cr();
}

void Frame::init(Thread *thread, address guessed_fp) {
  GUARANTEE(ObjectHeap::contains((OopDesc*)guessed_fp), "sanity");

  // Add to the linked list of all frames
  push_frame();
  _is_valid_guessed_frame = false;

  // Guessing doesn't work if we're inside the GC. Please use psgc()
  // instead.
  if (Frame::in_gc_state()) {
    return;
  }

  // (1) Find the object that contains guessed_fp. It must
  //     be an ExecutionStackDesc.
  Oop s = ObjectHeap::slow_object_start((OopDesc**)guessed_fp);
  if (!s.is_execution_stack()) {
    return;
  }

  ExecutionStack stk = s.obj();
  address start, limit;
  if (JavaStackDirection < 0) {
    // i386 style, limit is the end of the ExecutionStack object
    limit = ((address)stk.obj()) + stk.length();
  } else {
    limit = (address)stk.obj();
  }

  guessed_fp = (address) (((jint)guessed_fp) & (~0x03));
  limit      = (address) (((jint)limit)      & (~0x03));
  start      = guessed_fp;

  // (2) Start from the current top of the stack and search downwards.
  //     Look for the first location that contains a plausible FP.
  while (guessed_fp != limit) {
    address fp, sp, pc_addr;

    if (Frame::is_plausible_fp(guessed_fp, start, limit, fp, sp, pc_addr)) {
      _is_valid_guessed_frame = true;

      set_values(thread, thread->stack_base(), (address*)pc_addr, sp, fp);
      return;
    }
    guessed_fp += (-JavaStackDirection) * wordSize;
  }
}

void Frame::print_on(Stream* st, int index, const char* title) {
  st->print("  [%2d] %s", index, title);
  if (PrintLongFrames) {
    st->print(" (pc = 0x%lx, sp = 0x%lx, fp = 0x%lx)", pc(), sp(), fp());
  }
  st->cr();
}

bool Frame::is_plausible_fp(address start, address top, address bottom, 
                            address &fp_ret, address &sp_ret, 
                            address &pc_addr_ret) {
  bool found = false;
  address fp = start;
  address sp = NULL;
  address pc_addr = NULL;

  // In most cases, if we try to print the stack while we're executing
  // bytecodes inside the interpreter, or in compiled methods, we will
  // miss the top-most stack frame. This is because the frame dumping
  // mechanism requires that CallInfo exists for all return address
  // stored in the frames. However, we will have a CallInfo for the
  // top-most method only if it is calling shared_call_vm.

  for (;;) {
    // Check if fp points to a valid EntryFrame
    const address ret_addr = 
        *(address*) (fp + JavaFrame::return_address_offset());
    bool is_entry = (ret_addr == (address)EntryFrame::FakeReturnAddress);
    address last_fp_address = NULL;
    address last_sp = NULL;

    if (is_entry) {
      // This may be an EntryFrame
      const address caller_fp = *(address*) 
          (fp + EntryFrame::stored_last_fp_offset());
      if (caller_fp == NULL) {
        // This is the first entry frame
        // IMPL_NOTE: verify that we're pretty close to the bottom
        if (!found) {
          // This is the only frame on the stack and it's being constructed.
          // Can't print it out yet.
          return false;
        } else {
          return true;
        }
      }

      last_fp_address = fp + EntryFrame::stored_last_fp_offset();
    } else {
      // This may be a valid JavaFrame
      address this_sp = fp + JavaFrame::stack_bottom_pointer_offset();
      if (!is_within_stack_range(this_sp, top, bottom)) {
        return false;
      }

      last_fp_address = fp + JavaFrame::caller_fp_offset();
    }
    
    pc_addr = fp + JavaFrame::return_address_offset();
    last_sp = pc_addr - JavaStackDirection * (int)sizeof(jint);

    if (!is_within_stack_range(pc_addr, top, bottom)) {
      return false;
    }

    if (!is_within_stack_range(last_sp, top, bottom)) {
      return false;
    }

    if ((JavaStackDirection < 0 && last_fp_address >= bottom) ||
        (JavaStackDirection > 0 && last_fp_address <= bottom)) {
      // last_fp's address is not within range
      return false;
    }

    address last_fp = *(address*) last_fp_address;
    if (!is_within_stack_range(last_fp, fp, bottom)) {
      // last_fp is not within range
      return false;
    }

    if ((JavaStackDirection < 0 && last_fp > fp) ||
        (JavaStackDirection > 0 && last_fp < fp)) {
      sp = last_sp;
      fp = last_fp;
      if (!found) {
        fp_ret = fp;
        sp_ret = sp;
        pc_addr_ret = pc_addr;
        found = true;
      }
    } else {
      // last frame is not at the right direction.
      return false;
    }
  }
}

bool Frame::is_valid_guessed_frame() {
  return _is_valid_guessed_frame;
}

bool Frame::is_within_stack_range(address p, address stack_top, 
                                  address stack_bottom) {
  if (JavaStackDirection < 0) {
    GUARANTEE(stack_top <= stack_bottom, "sanity");
    return (stack_top <= p && p <= stack_bottom);
  } else {
    GUARANTEE(stack_top >= stack_bottom, "sanity");
    return (stack_top >= p && p >= stack_bottom);
  }
}

void JavaFrame::print_stack_address(Stream *st, address addr) {
  if (addr == sp()) {
    st->print("   sp=> ");
  } else if (addr == fp()) {
    st->print("   fp=> ");
  } else if (addr == locals_pointer()) {
    st->print("   lp=> ");
  } else {
    st->print("        ");
  }

  st->print_hex8((int)addr);
  st->print(" ");
}

void JavaFrame::print_on(Stream* st, int index) {
  if (PrintLongFrames) { 
    Frame::print_on(st, index, "JavaFrame");
  }
  st->print("  [%2d] ", index);
  Method::Raw m = method();
  m().print_name_on(st, false);
  if (Verbose) {
    st->print(" (");
#if ENABLE_COMPILER
    if (is_compiled_frame()) { 
      CompiledMethod::Raw cm = compiled_method();
      st->print("CompiledMethod* 0x%lx@%d, ", cm.obj(), pc() - cm().entry());
    }
#endif
    st->print("Method* 0x%lx)", m.obj());
  }
  st->fill_to(50);
  st->print(" bci=%d", bci());
  if (is_compiled_frame()) {
    st->print_cr(" (compiled)");
  } else {
    st->cr();
  }

  if (PrintLongFrames || PrintExtraLongFrames) {
    jint map_length;
    TypeArray::Raw map = generate_stack_map(map_length);

    print_expression_stack_on(st, &map, map_length);
    print_stack_locks_on(st);
    if (PrintExtraLongFrames) {
      print_raw_frame_on(st);
    }
    print_locals_on(st, &map);
    if (PrintExtraLongFrames) {
      st->cr();
    }
  }
}

void JavaFrame::print_stack_locks_on(Stream *st) {
  jint slength = stack_lock_length();

  for (int i = slength -  STACK_LOCK_SIZE; i >= 0; i -= STACK_LOCK_SIZE) {
    StackLock *lock = stack_lock_at(i);
    st->print("        SL%d ", i / STACK_LOCK_SIZE);
    lock->print_value_on(st);
    st->cr();

    if (PrintExtraLongFrames) {
      address lock_low = (address)lock;
      address lock_high = (address)lock->owner_address();

      address start = (JavaStackDirection < 0) ? lock_low  : lock_high;
      address stop  = (JavaStackDirection < 0) ? lock_high : lock_low;
      int delta     = -JavaStackDirection * BytesPerWord;

      for (address addr = start; ; addr += delta) {
        const char *fmt = "%-8s";
        print_stack_address(st, addr);
        st->print("SL%d ", i / STACK_LOCK_SIZE);

        int offset = addr - lock_low;
        address value = *(address*)addr;
        if (offset == StackLock::thread_offset()) {
          st->print(fmt, "thread");
          st->print_hex8(value);
        } else if (offset == StackLock::real_java_near_offset()) {
          st->print(fmt, "r_near");
          st->print_hex8(value);
        } else if (offset == StackLock::waiters_offset()) {
          st->print(fmt, "waiters");
          st->print_hex8(value);
        } else if (offset == StackLock::copied_near_offset()) {
          st->print(fmt, "c_near");
          st->print_hex8(value);
        } else if (offset == StackLock::copied_near_offset()
                          + JavaNear::class_info_offset()) {
          st->print(fmt, "cinfo");
          st->print_hex8(value);
        } else if (offset == StackLock::copied_near_offset()
                            + JavaNear::raw_value_offset()) {
          st->print(fmt, "hash");
          st->print_hex8(value);
        } else if (addr == (address)lock->owner_address()) {
          st->print(fmt, "owner");
          st->print_hex8(value);
        } else {
          st->print(fmt, "?");
          st->print_hex8(value);
        }

        st->cr();
        if (addr == stop) {
          break;
        }
      }
    }
  }
}

void JavaFrame::print_raw_frame_on(Stream* st) {
  int min_offset, max_offset;
  get_min_max_offsets(min_offset, max_offset);

  int start = (JavaStackDirection < 0) ? min_offset : max_offset;
  int stop  = (JavaStackDirection < 0) ? max_offset : min_offset;
  int delta = -JavaStackDirection * BytesPerWord;

  for (int offset = start; ; offset += delta) {
    const char *fmt = "%-12s";
    address *addr = (address*)(fp() + offset);
    print_stack_address(st, (address)addr);

    if (offset == stack_bottom_pointer_offset()) {
      st->print(fmt, "stk_btm_ptr");
      st->print_hex8(*addr);
    } else if (offset == bcp_store_offset()) {
      st->print(fmt, "bcp");
      st->print_hex8(*addr);
    } else if (offset == locals_pointer_offset()) {
      st->print(fmt, "locals");
      st->print_hex8(*addr);
    } else if (offset == cpool_offset()) {
      st->print(fmt, "cpool");
      st->print_hex8(*addr);
    } else if (offset == method_offset()) {
      st->print(fmt, "method");
      st->print_hex8(*addr);
    } else if (offset == caller_fp_offset()) {
      st->print(fmt, "caller_fp");
      st->print_hex8(*addr);
    } else if (offset == return_address_offset()) {
      st->print(fmt, "ret_addr");
      st->print_hex8(*addr);
    } else if (offset == stored_int_value1_offset()) { // Used by C interp only
      st->print(fmt, "int1");
      st->print_hex8(*addr);
    } else if (offset == stored_int_value2_offset()) { // Used by C interp only
      st->print(fmt, "int2");
      st->print_hex8(*addr);
    }

    st->cr();
    if (offset == stop) {
      break;
    }
  }
}

void JavaFrame::print_expression_stack_on(Stream* st, TypeArray* map, 
                                          int map_length) {
  jint elength = expression_length();
  for (int i = 0; i < elength; i++) {
    jint tag = map->byte_at(map_length - i - 1);

    StackValue* expr = expression_at(i);
    if (PrintExtraLongFrames) {
      print_stack_address(st, (address)expr);
      st->print("E%-10d ", i);
      st->print_hex8(*(address*)expr);
      st->print(" ");
    } else {
      st->print("        E%d ", i);
    }
    expr->print_on(st, tag);
    if (StackValue::is_big(tag)) {
      i++;
    }
  }
}

void JavaFrame::print_locals_on(Stream* st, TypeArray* map) {
  jint llength = local_length();
  for (int i = llength - 1; i >= 0; i--) {
    jint tag = map->byte_at(i);
    StackValue* local = local_at(i);
    if (StackValue::is_big(tag)) {
      i--;
    }
    if (PrintExtraLongFrames) {
      print_stack_address(st, (address)local);
      st->print("L%-10d ", i);
      st->print_hex8(*(address*)local);
      st->print(" ");
    } else {
      st->print("        L%d ", i);
    }
    local->print_on(st, tag);
  }
}

void JavaFrame::get_min_max_offsets(int& min_offset, int& max_offset) {
  min_offset = 0x7fffffff;
  max_offset = -1;

  get_min_max(min_offset, max_offset, stack_bottom_pointer_offset());
  get_min_max(min_offset, max_offset, bcp_store_offset());
  get_min_max(min_offset, max_offset, locals_pointer_offset());
  get_min_max(min_offset, max_offset, cpool_offset());
  get_min_max(min_offset, max_offset, method_offset());
  get_min_max(min_offset, max_offset, caller_fp_offset());
  get_min_max(min_offset, max_offset, return_address_offset());
}

void Frame::get_min_max(int& min, int& max, int value) {
  if (min > value) {
    min = value;
  }
  if (max < value) {
    max = value;
  }
}

void EntryFrame::print_on(Stream* st, int index) {
  Frame::print_on(st, index, "EntryFrame");
  // Compute length info before visiting expression stack.
  if (PrintExtraLongFrames) {
    print_raw_frame_on(st);
  } else if (PrintLongFrames) {
    UsingFastOops oops;
    Oop::Fast objValue   = stored_obj_value();
    Oop::Fast exception  = pending_exception();
    Oop::Fast activation = pending_activation();
    st->print("       I1 (int) %d\n", stored_int_value1());    
    st->print("       I2 (int) %d\n", stored_int_value2());    
    st->print("      Obj "); objValue.print_value_on(st);   st->cr();
    st->print("      Exc "); exception.print_value_on(st);  st->cr();
    st->print("      Act "); activation.print_value_on(st); st->cr();
  }
}

void EntryFrame::print_raw_frame_on(Stream *st) {
  int min_offset, max_offset;
  get_min_max_offsets(min_offset, max_offset);

  int start = (JavaStackDirection < 0) ? min_offset : max_offset;
  int stop  = (JavaStackDirection < 0) ? max_offset : min_offset;
  int delta = -JavaStackDirection * BytesPerWord;

  for (int offset = start; ; offset += delta) {
    const char *fmt = "%-12s";
    address *addr = (address*)(fp() + offset);

    if (addr == (address*)fp()) {
      st->print("   fp=> ");
    } else {
      st->print("        ");
    }
    st->print_hex8((int)addr);
    st->print(" ");

    if (offset == stored_last_sp_offset()) {
      st->print(fmt, "last_sp");
      st->print_hex8(*addr);
    } else if (offset == stored_last_fp_offset()) {
      st->print(fmt, "last_fp");
      st->print_hex8(*addr);
    } else if (offset == fake_return_address_offset()) {
      st->print(fmt, "fake_ret");
      st->print_hex8(*addr);
    } else if (offset == real_return_address_offset()) {
      st->print(fmt, "real_ret");
      st->print_hex8(*addr);
    } else if (offset == stored_int_value1_offset()) {
      st->print(fmt, "int_val_1");
      st->print_hex8(*addr);
    } else if (offset == stored_int_value2_offset()) {
      st->print(fmt, "int_val_2");
      st->print_hex8(*addr);
    } else if (offset == stored_obj_value_offset()) {
      st->print(fmt, "obj_val");
      st->print_hex8(*addr);
    } else if (offset == pending_exception_offset()) {
      st->print(fmt, "exception");
      st->print_hex8(*addr);
    } else if (offset == pending_activation_offset()) {
      st->print(fmt, "activation");
      st->print_hex8(*addr);

      EntryActivation entry = *((OopDesc**)addr);
      if (entry.not_null()) {
        st->print(" ");
        entry.print_value_on(st);
        st->cr();
        entry.print_list_on(st, 20, 0);
      }
    } else {
      st->print(fmt, "???");
      st->print_hex8(*addr);
    }

    st->cr();
    if (offset == stop) {
      break;
    }
  }
  st->cr();
}

void EntryFrame::get_min_max_offsets(int& min_offset, int& max_offset) {
  min_offset = 0x7fffffff;
  max_offset = -1;

  get_min_max(min_offset, max_offset, stored_last_sp_offset());
  get_min_max(min_offset, max_offset, stored_last_fp_offset());
  get_min_max(min_offset, max_offset, fake_return_address_offset());
  get_min_max(min_offset, max_offset, real_return_address_offset());
  get_min_max(min_offset, max_offset, stored_int_value1_offset());
  get_min_max(min_offset, max_offset, stored_int_value2_offset());
  get_min_max(min_offset, max_offset, stored_obj_value_offset());
  get_min_max(min_offset, max_offset, pending_exception_offset());
  get_min_max(min_offset, max_offset, pending_activation_offset());
}

#endif // !PRODUCT

#undef STACK_LOCK_SIZE
