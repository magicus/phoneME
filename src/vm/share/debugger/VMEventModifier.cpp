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
# include "incls/_VMEventModifier.cpp.incl"

#if ENABLE_JAVA_DEBUGGER

// Allocate a new VMEventModifier object

ReturnOop VMEventModifier::allocate_modifier()
{
  SETUP_ERROR_CHECKER_ARG;
  SAVE_CURRENT_EXCEPTION;
  Oop::Raw o = Universe::new_mixed_oop(MixedOopDesc::Type_VMEventModifier,
                                 VMEventModifierDesc::allocation_size(),
                                 VMEventModifierDesc::pointer_count()
                                 JVM_NO_CHECK_AT_BOTTOM);
  RESTORE_CURRENT_EXCEPTION;
  return o;
}


ReturnOop VMEventModifier::new_modifier(PacketInputStream *in,
                                        PacketOutputStream *out, bool& error) {

  UsingFastOops fast_oops;

  jbyte mod_kind = in->read_byte();
  VMEventModifier::Fast newMod;
  Thread::Fast thread;
  newMod = allocate_modifier();
  if (newMod.is_null()) {
    error = true;
    out->set_error(JDWP_Error_OUT_OF_MEMORY);
    return NULL;
  }
  newMod().set_mod_kind(mod_kind);

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("Modifier: modkind = %d", mod_kind);
  }
#endif
  switch(mod_kind) {
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_FieldOnly:
    // unsupported
    in->read_class_id();
    in->read_class_id();
    in->read_int();
    return NULL;
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_Conditional:
    // unsupported
    in->read_int();
    return NULL;
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassExclude:
    // unsupported
    in->read_string();
    return NULL;
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_ThreadOnly:
    newMod().set_thread_id(in->read_thread_id());
    return newMod;
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_Count:
    newMod().set_count(in->read_int());
    return newMod;
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassOnly:
    {
      newMod().set_clazz_id(in->read_class_id());
#ifdef AZZERT
      if (TraceDebugger) {
        tty->print_cr("Modifier: class_id = %d", newMod().clazz_id());
      }
#endif
      return newMod;
    }
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassMatch:
    {
      UsingFastOops fast_oops_2;
      int length;
      SETUP_ERROR_CHECKER_ARG;

      TypeArray::Fast ta = in->read_raw_string(length JVM_NO_CHECK);
      if (ta.is_null()) {
        // Out of memory?? Things will fall apart soon enough,
        // We just punt on this modifier
        return newMod;
      }
      for (int i = length - 1; i >= 0; i--) {
        if (ta().byte_at(i) == '.') {
          ta().byte_at_put(i, '/');
        }
      }
      Symbol::Fast tmpName = Universe::new_symbol(&ta,
                             (utf8)(ta().base_address()), length JVM_NO_CHECK);
      if (tmpName.is_null()) {
        error = true;
        out->set_error(JDWP_Error_OUT_OF_MEMORY);
        return NULL;
      }
#ifdef AZZERT
      if (TraceDebugger) {
        tty->print("Modifier: class match = ");
        tmpName().print_symbol_on(tty);
        tty->cr();
      }
#endif
      ClassMatchModifier::Fast class_match_modifier = newMod().obj();
      class_match_modifier().set_class_match_name(&tmpName);
      return newMod;
    }
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly:
    {
      UsingFastOops fast_oops_3;

      jbyte tag = in->read_byte(); (void)tag;
      LocationModifier::Fast lmod = newMod.obj();
      lmod().set_clazz_id(in->read_class_id());
      if (lmod().clazz_id() == 0) {
        return NULL;        /* Class ID == 0 is invalid */
      }
      lmod().set_method_id(in->read_method_id());
      lmod().set_offset(in->read_long());
#ifdef AZZERT
      if (TraceDebugger) {
        tty->print_cr("Loc: class: 0x%x, method: 0x%x, off: 0x%x",
                      lmod().clazz_id(),
                      (int)(lmod().method_id() & 0xFFFFFFFF),
                      lmod().offset());
      }
#endif
      return lmod;
    }
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_ExceptionOnly:
    {
      UsingFastOops fast_oops_4;

      ExceptionModifier::Fast emod = newMod.obj();
      emod().set_clazz_id(in->read_class_id());
      emod().set_sig_caught(in->read_boolean());
      emod().set_sig_uncaught(in->read_boolean());
      return emod;
    }
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_Step:
    {
      UsingFastOops fast_oops_5;

      int thread_id = in->read_thread_id();
      thread = JavaDebugger::get_thread_by_id(thread_id);
      if (!JavaDebugger::is_valid_thread(&thread) ||
          !thread().last_java_frame_exists()) {
        error = true;
        out->set_error(JDWP_Error_INVALID_THREAD);
        return NULL;
      }

      Frame fr(&thread);
      GUARANTEE(fr.is_java_frame(), "Single step must start at JavaFrame");
      JavaFrame jf = fr.as_JavaFrame();
      LocationModifier::Fast loc = LocationModifier::new_location(&jf);
      if (loc.is_null()) {
        error = true;
        out->set_error(JDWP_Error_OUT_OF_MEMORY);
        return NULL;
      }
#ifdef AZZERT
      if (TraceDebugger) {
        tty->print_cr("StepLoc: thread: 0x%x, class: 0x%x, method: 0x%x, off: 0x%x",
                      thread_id, loc().clazz_id(),
                      (int)(loc().method_id() & 0xFFFFFFFF),
                      loc().offset());
      }
#endif
      loc().set_mod_kind(mod_kind);
      StepModifier::Fast smod = loc.obj();
      smod().set_thread_id(thread_id);
      if (thread.is_null() /* || (thread.state() & THREAD_DEAD) */) {
        /*
         * If you try to single step after suspending because
         * of an uncaught exception event we'll get sent the
         * thread id of the thread that had the exception.
         * That thread is dead though.
         */
        error = true;
        out->set_error(JDWP_Error_INVALID_THREAD);
        return NULL;
      }
      
      smod().set_step_size(in->read_int());
      smod().set_step_depth(in->read_int());
    
      /* query the _proxy_ for next line location */                        
      {
        UsingFastOops fast_oops_6;

        DebuggerEvent d_event;
        d_event.set_event_kind((jbyte)VM_STEPINFO_EVENT);
        VMEvent::stepping_info_request(&d_event, in->transport(),
                                                 &smod);

        /*
         * At this point, the proxy has made sure no more commands
         * from the debugger get sent until we get our line number
         * info from the proxy
         */
        JavaDebugger::set_loop_count(1);
        JavaDebugger::process_command_loop();
        /*
         * Once we get back here, then we know we've gotten the
         * command from the proxy with the line number info put into
         * the StepModifier object in the step_info_event above.
         */

        /* setup the relevant info */
        thread().set_is_stepping(true);
        // set the _debugger_active flag for the interpreter loop
        JavaDebugger::set_stepping(true);

        //        smod = step_info_event().mods();
        //        smod().set_step_target_offset(stepData().step_target_offset());
        smod().unlink_method();

        if (smod().step_depth() == JDWP_StepDepth_OUT) {
          // if stepping out to the caller, we really should unlink any
          // compiled code from here to the caller frame, for now assume
          // one frame up.  It could be re-compiled but we may luck out.
          Method::Fast m;
          Frame caller(jf);
          jf.caller_is(caller);
          if (!caller.is_entry_frame()) {
            m = caller.as_JavaFrame().method();
#if ENABLE_COMPILER
            if (m().has_compiled_code()) {
              m().unlink_compiled_code();
            }
#endif
          }
        }
        smod().set_step_starting_fp(DISTANCE(thread().stack_base(), jf.fp()));
        smod().set_step_starting_offset(jf.bci());
        //        smod().set_dup_current_line_offset(stepData().dup_current_line_offset());
        //        smod().set_post_dup_line_offset(stepData().post_dup_line_offset());
        thread().set_step_info(&smod);
      }
      return smod;
    }
  }
  error = true;
  out->set_error(JDWP_Error_NOT_IMPLEMENTED);
  return NULL;
}

// Return true/false if the given modifier matches 'this'

jboolean VMEventModifier::match(DebuggerEvent *d_event, bool *should_delete) 
{
  *should_delete = false;

  if (d_event == NULL)
    return false;
  switch (mod_kind()) {
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_Conditional:
    /* reserved for future use */
    break;
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_ThreadOnly:
    return thread_match(d_event);
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassOnly:
    return  ((ClassMatchModifier *)this)->class_only(d_event);
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassMatch:
    return ((ClassMatchModifier *)this)->class_match(d_event);

  case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassExclude:
    return !((ClassMatchModifier *)this)->class_match(d_event);
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly:
    return ((LocationModifier *)this)->match(d_event);
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_ExceptionOnly:
    {
      ExceptionModifier *exm = (ExceptionModifier *)(this);
      bool ret = true;
      if (exm->clazz_id() != 0) {
        InstanceClass::Raw clazz1 =
          JavaDebugger::get_object_by_id(d_event->clazz_id());
        InstanceClass::Raw clazz2 =
          JavaDebugger::get_object_by_id(exm->clazz_id());
        if (!clazz1().is_subclass_of(&clazz2)) {
          ret = false;
        }
      }

      ret = ret && ((exm->sig_caught() && d_event->sig_caught()) ||
                    (exm->sig_uncaught() && d_event->sig_uncaught()));

      if (!ret)
        return false;
      break;
    }
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_FieldOnly:
    break;
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_Step:
    return ((StepModifier *)this)->match(d_event);
  case JDWP_EventRequest_Set_Out_modifiers_Modifier_Count:
    {
      int count;
      // just the fact that we have a count modifier means it is active
      if ((count = event_count()) == 1) {
        // we will send this event now
        set_count(0);    // but first turn it off
        return true;
      } else if (count > 1) {
        // decrement and return false
        set_count(--count);
        return false;
      } else {
        // must be <=0, so no event
        *should_delete = true;
        return false;
      }
    }

  }
  return true;
}

jboolean VMEventModifier::thread_match(DebuggerEvent *d_event)
{
  return (thread_id() == d_event->thread_id());
}

jboolean StepModifier::match(DebuggerEvent *d_event) {
  return (thread_id() == d_event->thread_id() &&
          step_size() == d_event->step_size() &&
          step_depth() == d_event->step_depth());
}

void
VMEventModifier::deoptimize_frame(Thread *thread, bool bump_bci)
{
#if ENABLE_COMPILER
  UsingFastOops fast_oops;

  JavaFrame fr(thread);
  if (fr.is_compiled_frame()) {
    UsingFastOops fast_oops_2;
    // since we are stopping in a compiled frame, deoptimize it as we
    // will most likely single step or break here again
    CompiledMethod::Fast method_code, stepper = fr.compiled_method();
    Method::Fast m = fr.method();
    fr.deoptimize_and_continue(bump_bci);
    if (m().has_compiled_code()) {
      method_code = m().compiled_code();
      if (stepper.equals(&method_code)) {
        m().unlink_compiled_code();
      }
    }
  }
#endif
}

void VMEventModifier::deoptimize_method(Method *m) {
#if ENABLE_COMPILER
  // If we insert a breakpoint into a method, that method may be compiled
  // and there may be countless frames in the system that reference
  // that method.  We must find all these frames and deoptimized them so that
  // we hit this breakpoint.
  UsingFastOops fast_oops;
  Thread::Fast thread = Universe::global_threadlist();
  while (thread.not_null() && thread().last_java_fp() != NULL) {
    JavaFrame fr(&thread);
    bool top_frame = true;
    while(true) {
      if (fr.is_entry_frame()) {
        if (fr.as_EntryFrame().is_first_frame()) {
          break;
        }
        top_frame = false;
        fr.as_EntryFrame().caller_is(fr);
      } else {
        UsingFastOops fast_oops2;
        GUARANTEE(fr.is_java_frame(), "Neither JavaFrame or EntryFrame");
        Method::Fast method = fr.method();
        if (method.equals(*m)) {
          // found a reference to this method on some frame, deoptimize it
          if (fr.is_compiled_frame()) {
            // deoptimize essentially restarts the current instruction in this
            // frame when we return to interpreter_deoptimize_entry.
            // However, in our case we can't restart the instruction 
            // since all the stack (with args etc) is popped off.  We really
            // want to continue in the interpreter at the following instruction
            // Hence we bump up the bci by 3 (after checking to make sure
            // it's an invoke we are pointing at).
            fr.deoptimize_and_continue(!top_frame);
          }
        }
        top_frame = false;
        fr.caller_is(fr);
      }
    }
    thread = thread().global_next();
  }
#endif
}

void LocationModifier::unlink_method() {
#if ENABLE_COMPILER
  Method::Raw m = method();
  set_compile_state(true);
  if (m().has_compiled_code()) {
    m().unlink_compiled_code();
    deoptimize_method(&m);
  } else {
    if (m().is_impossible_to_compile()) {
      set_compile_state(false);
    }
  }
  if (!m().is_impossible_to_compile()) {
    m().set_impossible_to_compile();
  }
#endif
}

ReturnOop LocationModifier::new_location(JavaFrame *fr,
                                         LocationModifier *loc)
{
  UsingFastOops fast_oops;

  Method::Fast m = fr->method();
  // we may not be creating a location in a frame of the current thread
  // Therefore we must get the correct class list from the task
  InstanceClass::Fast ic = m().holder(fr->thread());
  return new_location(&ic, &m, fr->bci(), loc);
}

ReturnOop LocationModifier::new_location(InstanceClass *ic,
                                         Method *m, jlong offset,
                                         LocationModifier *location)
{
  UsingFastOops fast_oops;

  LocationModifier::Fast loc;
  if (location == NULL) {
    loc = allocate_modifier();
    if (loc.is_null()) {
      return NULL;
    }
  } else {
    loc = *location;
  }
  loc().set_thread_id(JavaDebugger::get_thread_id_by_ref(Thread::current()));
  loc().set_clazz_id(JavaDebugger::get_object_id_by_ref(ic));
  loc().set_method_id(JavaDebugger::get_method_id(ic,m));
  if (loc().method_id() == -1) {
    // we may have a debug rom breakpoint here.  Try to find the 
    // correct method id
    loc().find_and_set_rom_method_id(m);
  }
  if (loc().method_id() == -1) {
    // couldn't find the method, return NULL
    return NULL;
  }
  loc().set_offset(offset);
  return loc;
}

ReturnOop LocationModifier::method() {
  InstanceClass::Raw clazz = JavaDebugger::get_object_by_id(clazz_id());
  Method::Raw method = JavaDebugger::get_method_by_id(&clazz, method_id());
  return method;
}
 
void LocationModifier::write_null_location(PacketOutputStream *out) {
  out->write_byte(1);
  out->write_int(0);
  out->write_int(0);
  out->write_long(0);
}

void LocationModifier::write(PacketOutputStream *out, jboolean write_tag) {
  // Through the miracle of virtual functions, the write_xxx() functions
  // here will go to BufferedPacketOutputStream if necessary
  jbyte tag_type;

  if (write_tag) {
    if (clazz_id() == 0) {
      out->write_byte(JDWP_TypeTag_CLASS);
    } else {
      JavaClass::Raw jc = JavaDebugger::get_object_by_id(clazz_id());
      tag_type = JavaDebugger::get_jdwp_tagtype(&jc);
      out->write_byte(tag_type);
    }
  }
  out->write_int(clazz_id());
  out->write_long(method_id());
  out->write_long(offset());
}  

jboolean LocationModifier::match(DebuggerEvent *d_event)
{
  if (d_event->clazz_id() == clazz_id() &&
      d_event->method_id() == method_id() &&
      d_event->offset() == offset()) {
    return true;
  }
  return false;
}

ReturnOop LocationModifier::get_dup_rom_method(Method *in_rom_method) {
 
  VMEventStream es;

  VMEvent::Raw ep;
  LocationModifier::Raw  mod;
  Method::Raw sm;
  ep = es.next_by_kind(JDWP_EventKind_BREAKPOINT);
  while(!ep.is_null()) {
    mod = VMEvent::get_modifier(&ep,
        JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
    if (!mod.is_null()) {
      sm = mod().method();
      if (!sm.is_null() && sm.obj() == in_rom_method->obj()) {
        return mod().rom_debug_method();
      }
    }
    ep = es.next_by_kind(JDWP_EventKind_BREAKPOINT);
  }
  return NULL;
}

void LocationModifier::find_and_set_rom_method_id(Method *debug_rom_method) {

  VMEventStream es;

  VMEvent::Raw ep;
  LocationModifier::Raw  mod;
  Method::Raw dm;
  ep = es.next_by_kind(JDWP_EventKind_BREAKPOINT);
  while(!ep.is_null()) {
    mod = VMEvent::get_modifier(&ep,
        JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
    if (!mod.is_null()) {
      dm = mod().rom_debug_method();
      if (!dm.is_null() && dm.obj() == debug_rom_method->obj()) {
        set_method_id(mod().method_id());
        break;
      }
    }
    ep = es.next_by_kind(JDWP_EventKind_BREAKPOINT);
  }
}

extern "C" {
  address get_rom_debug_method(Thread *thread, OopDesc *md) {
    (void)thread; // always passed from interpreter but not used here.
    VMEventStream es;

    Method::Raw m = md;
    VMEvent::Raw ep;
    LocationModifier::Raw  mod;

    ep = es.next_by_kind(JDWP_EventKind_BREAKPOINT);
    while(!ep.is_null()) {
      mod = VMEvent::get_modifier(&ep,
               JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
      if (!mod.is_null() &&
          mod().method_id() == JavaDebugger::get_method_id(&m)) {
        Method::Raw dm = mod().rom_debug_method();
        if (!dm.is_null()) {
          return (address)((char *)dm.obj());
        }
      }
      ep = es.next_by_kind(JDWP_EventKind_BREAKPOINT);
   }
    return (address)md;
  }
}
  
                              

// Install the given opcode into the bytecode stream saving the original 
// into the event modifier.

jboolean LocationModifier::set_method_opcode(Bytecodes::Code new_opcode,
                                             bool is_setting)
{
  UsingFastOops fast_oops;

  Method::Fast m = method();
  jlong offs = offset();
  Bytecodes::Code opcode;
  SETUP_ERROR_CHECKER_ARG;

  if (m.is_null()) {
    // If we are clearing a breakpoint and this was a <clinit> method
    // and we removed <clinit> after the class was intialized then we
    // return since there's nothing to do
    return false;
  }
#ifdef AZZERT
  /* Determine if this is a legal offset 
   * into the bytecode for this method 
   */
  if (!JavaDebugger::is_legal_offset(&m, offs)) {
    return false;
  }
#endif
  if (offs >= m().code_size()) {
    // quick check for out of bounds, could happen if method was 
    // converted to fast_accessor 
    return false;
  }
  if (ROM::system_text_contains(m.obj())) {
    // This method is in ROM so we need to copy it out so that
    // we can modify breakpoints.
    UsingFastOops fast_oops_2;
    Method::Fast dm = get_dup_rom_method(&m);

    if (is_setting) {
      if (dm.is_null()) {
        AccessFlags af = m().access_flags();
        dm = Universe::new_method(m().code_size(), af JVM_CHECK_0);
        jvm_memcpy((char *)dm.obj() + dm().access_flags_offset(),
                   (char *) m.obj() +  m().access_flags_offset(),
                   Method::base_offset() + m().code_size() -
                   m().access_flags_offset());
        // ROM method constants pointer points into ROM text if we 
        // merged pools
        ConstantPool::Raw cp = m().constants();
        dm().set_constants(&cp);
        if (!m().has_no_stackmaps()) {
          Oop::Raw stackmaps = m().stackmaps();
          dm().set_stackmaps(&stackmaps);
        }
        if (!m().has_no_exception_table()) {
          Oop::Raw exception_table = m().exception_table();
          dm().set_exception_table(&exception_table);
        }
#if ENABLE_REFLECTION
        Oop::Raw thrown_exceptions = m().thrown_exceptions();
        dm().set_thrown_exceptions(&thrown_exceptions);
#endif
        set_rom_debug_method(&dm);
      }
      opcode = m().bytecode_at_raw((int)offs);
      dm().bytecode_at_put_raw((int)offs, new_opcode);
      m().set_execution_entry((address)shared_invoke_debug);
      dm().set_execution_entry((address)shared_invoke_debug);
    } else {
      // We are clearing a breakpoint that was set in ROM (actually in
      // a copy of the ROM method)
      dm = rom_debug_method();
      GUARANTEE(!dm.is_null(), "Clearing ROM breakpoint, but method is null!");
      opcode = dm().bytecode_at_raw((int)offs);
      // install new opcode into bytecode stream
      dm().bytecode_at_put_raw((int)offs, new_opcode);
    }
  } else {
    opcode = m().bytecode_at_raw((int)offs);
    // install new opcode into bytecode stream
    m().bytecode_at_put_raw((int)offs, new_opcode);
  }
  if (is_setting) {
    set_save_opcode(opcode);
  }
  return (true);
}

void LocationModifier::save_method_entry() {
  Method::Raw m = method();
  address method_entry = m().execution_entry();
  set_saved_method_entry(method_entry);
}

jboolean StepModifier::find_frame(address starting_fp, JavaFrame *frame)
{
  JavaFrame caller(*frame);
  caller.caller_is(caller);
  while (true) {
    if (caller.is_entry_frame()) {
      EntryFrame e = caller.as_EntryFrame();
      if (e.is_first_frame()) {
        // did not find the frame
        return false;
      }
      e.caller_is(caller);
    } else if (caller.is_java_frame()) {
      JavaFrame jf = caller.as_JavaFrame();
      if (starting_fp == jf.fp()) {
        // found the frame
        return true;
      }
      jf.caller_is(caller);
    }
  }
}

jboolean ClassMatchModifier::class_only(DebuggerEvent *d_event)
{
  if (d_event->clazz_id() == clazz_id()) {
    return true;
  } else {
    return false;
  }
}

// See if the 'current' event being attempted matches the class name in the
// modifier that was set in an event request.

jboolean ClassMatchModifier::class_match(DebuggerEvent *d_event)
{
  UsingFastOops fast_oops;

  JavaClass::Fast clazz = JavaDebugger::get_object_by_id(d_event->clazz_id());
  TypeSymbol::Fast class_name = clazz().name();
  Symbol::Fast mod_name =  class_match_name(); 

  if (mod_name.is_null() || class_name.is_null())
    return true;

  if (clazz().is_obj_array_class()) {
    juint klass_index = class_name().decode_ushort_at(0);
    JavaClass::Raw klass = Universe::class_from_id(klass_index);
    ObjArrayClass::Raw oac = klass.obj();
    klass = oac().element_class();
    class_name = klass().name();
  } else  if (clazz().is_type_array_class()) {
      // just a basic type array, no match here.
      return false;
  }
  // class names in the VM are not necessarily stored with the current OS
  // file separator character, Win32 can use either type of slash

  if (mod_name().byte_at(0) == '*' ||
      mod_name().byte_at(mod_name().length()-1) == '*') {
    // We have a wildcard match.
    int complen = mod_name().length() - 1;
    int req_start = class_name().length() - complen;
    int mod_start = 0;

    if (req_start < 0) {
      return false;
    } else { 
      if (mod_name().byte_at(0) == '*') {
        // suffix match
        // just compare the end of classname
        mod_start = 1;    // point to just past the '*'
      } else {
        req_start = 0;    // prefix match
      }
      return  (jvm_memcmp(mod_name().utf8_data() + mod_start,
                          class_name().utf8_data() + req_start, complen) == 0);
    }
  } else {
    // not a suffix/prefix match so just compare the two.
    return  mod_name().matches(&class_name());
  }
}

void DebuggerEvent::write_as_location(PacketOutputStream *out) {
  jbyte tag_type;

  if (clazz_id() == 0) {
    out->write_byte(JDWP_TypeTag_CLASS);
  } else {
    JavaClass::Raw jc = JavaDebugger::get_object_by_id(clazz_id());
    tag_type = JavaDebugger::get_jdwp_tagtype(&jc);
    out->write_byte(tag_type);
  }
  out->write_int(clazz_id());
  out->write_long(method_id());
  out->write_long(offset());
}

DebuggerEvent::DebuggerEvent(jbyte kind, int thread_id,
                             int clazz_id, jlong method_id,
                             jlong offset) :
  _offset(offset),
  _thread_id(thread_id),
  _clazz_id(clazz_id),
  _method_id(method_id),
  _event_kind(kind)
{
#if ENABLE_ISOLATES
  set_task_id(Thread::current()->task_id());
#endif
}

#endif /* ENABLE_JAVA_DEBUGGER */
