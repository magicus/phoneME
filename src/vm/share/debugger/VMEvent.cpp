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
# include "incls/_VMEvent.cpp.incl"

#if ENABLE_JAVA_DEBUGGER

int VMEvent::_debugger_notify_list;

// convert event kinds to a bit for quick testing
int VMEvent::JDWP_eventkind_to_dbg_eventkind(jbyte eventType)
{
  int dbg_eventType;

  switch (eventType) {
  case JDWP_EventKind_BREAKPOINT:
    dbg_eventType = Dbg_EventKind_BREAKPOINT;
    break;
  case JDWP_EventKind_SINGLE_STEP:
    dbg_eventType = Dbg_EventKind_SINGLE_STEP;
    break;
  case JDWP_EventKind_THREAD_START:
    dbg_eventType = Dbg_EventKind_THREAD_START;
    break;
  case JDWP_EventKind_THREAD_END:
    dbg_eventType = Dbg_EventKind_THREAD_END;
    break;
  case JDWP_EventKind_CLASS_PREPARE:
    dbg_eventType = Dbg_EventKind_CLASS_PREPARE;
    break;
  case JDWP_EventKind_CLASS_LOAD:
    dbg_eventType = Dbg_EventKind_CLASS_LOAD;
    break;
  case JDWP_EventKind_CLASS_UNLOAD:
    dbg_eventType = Dbg_EventKind_CLASS_UNLOAD;
    break;
  case JDWP_EventKind_VM_INIT:
    dbg_eventType = Dbg_EventKind_VM_INIT;
    break;
  case JDWP_EventKind_VM_DEATH:
    dbg_eventType = Dbg_EventKind_VM_DEATH;
    break;
  case JDWP_EventKind_EXCEPTION_CATCH:
    dbg_eventType = Dbg_EventKind_EXCEPTION_CATCH;
    break;
  case JDWP_EventKind_USER_DEFINED:
    dbg_eventType = Dbg_EventKind_USER_DEFINED;
    break;
  case JDWP_EventKind_FRAME_POP:
    dbg_eventType = Dbg_EventKind_FRAME_POP;
    break;
  case JDWP_EventKind_EXCEPTION:
    dbg_eventType = Dbg_EventKind_EXCEPTION;
    break;
  default:
    return 0;
  }
  return dbg_eventType;
}

ReturnOop VMEvent::find_event(jbyte kind) {
  VMEventStream es;

  return es.next_by_kind(kind);
}

ReturnOop VMEvent::find_breakpoint_event(LocationModifier *lmp, int task_id) {
  return find_breakpoint_event(lmp->clazz_id(), lmp->method_id(),
                               lmp->offset(), task_id);
}

ReturnOop VMEvent::find_breakpoint_event(int class_id, jlong method_id,
                                         jlong offset, int task_id)
{
  // May be called during GC via GenerateStackMap::run().

  VMEventStream es;
  VMEvent::Raw ep;
  LocationModifier::Raw  loc;

  while (!es.at_end()) {
    ep = es.next_by_kind(JDWP_EventKind_BREAKPOINT);
    if (ep.is_null()) {
      return NULL;
    }
#if ENABLE_ISOLATES
    // If task_id == -1 && ep.task_id == -1 then we don't care
    // which task this is we check the event otherwise we skip it
    if (task_id != -1 && ep().task_id() != -1 &&
        (ep().task_id() != task_id)) {
      continue;
    }
#else
    (void)task_id;
#endif
    loc = get_modifier(&ep,
        JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
    if (loc.not_null()) {
      if (loc().clazz_id() == class_id &&
          loc().method_id() == method_id && 
          loc().offset() == offset) {
        return ep;
      }
    }
  }
  return (ReturnOop)NULL;
}

// Given a modifier type, find the VMEventModifier that matches in this
// particular event

ReturnOop VMEvent::get_modifier(VMEvent *ep, jbyte type)
{
  VMEventModifier::Raw mod;
  for (mod = ep->mods(); !mod.is_null(); mod = mod().next()) {
    if (mod().mod_kind() == type) {
      return mod;
    }
  }
  return NULL;
}

// Find an event in our list of events that satisfies all the modifiers
// passed in via the VMEventModifier object
// Return a list of events that match.  If no match return NULL

ReturnOop VMEvent::get_event_request(DebuggerEvent *d_event,
    int &event_count, jbyte &suspend_policy)
{
  VMEventStream es;

  UsingFastOops fast_oops;

  VMEvent::Fast ep, epp;
  VMEventModifier::Fast em;
  bool matched;

  jbyte kind = d_event->event_kind();

  event_count = 0;
  suspend_policy = 0;
  ep = es.next_by_kind(kind);
  while(!ep.is_null()) {
    bool should_delete = false;
    // check modifiers
#if ENABLE_ISOLATES
    // Check to see what task this event was requested on
    if (ep().task_id() != -1 &&
        (Thread::current()->task_id() != ep().task_id())) {
      ep = es.next_by_kind(kind);
      continue;
    }
#endif
    em = ep().mods();
    // we need to check the modifiers to see if we send this event
    matched = true;
    do {
      if (em.is_null()) {
        break;
      }
      if (!em().match(d_event, &should_delete)) {
        matched = false;
        break;
      }
      em = em().next();
    } while(em.not_null());
    if (matched) {
      // Found a matching event, join it to the list of events to send
      ep().set_send_next(&epp);
      epp = ep;
      event_count++;
      if (ep().suspend_policy() > suspend_policy) {
        suspend_policy = ep().suspend_policy();
      }
    }
    if (should_delete) {
      clear_event_request(&ep);
    }
    ep = es.next_by_kind(kind);
  }
  return epp;
}

// called during verification if we hit a breakpoint opcode.
// Return the original opcode to caller

Bytecodes::Code
VMEvent::get_verifier_breakpoint_opcode(const Method* this_method, int bci) {
  UsingFastOops fast_oops;

  InstanceClass::Fast cl;
  // We may get called from Method::check_bytecodes() and the holder
  // hasn't been set yet.  Check for 0xFFFF  class id which is illegal
  if (this_method->holder_id() == 0xFFFF) {
    // No class has been set yet, just return the breakpoint opcode
    return Bytecodes::_breakpoint;
  }


  cl = this_method->holder();   /* point to class of method */
  // We may get called during GC when stackmaps are generated and we are
  // iterating over bytecodes.  So, we can't allocate.

  VMEvent::Raw ep =
    find_breakpoint_event(JavaDebugger::get_object_id_by_ref(&cl),
                          JavaDebugger::get_method_id(this_method),
                          (jlong)bci, -1);
  if (ep.is_null()) {
    return Bytecodes::_illegal;
  }
  LocationModifier::Raw location = get_modifier(&ep,
        JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
  GUARANTEE(!location.is_null(), "No location modifier for event");
  return(location().save_opcode());
}

// Replace the opcode in the breakpoint event with the new fast bytecode

void VMEvent::replace_event_opcode(Method *method, Bytecodes::Code bcode,
                                        int bci)
{
  UsingFastOops fast_oops;

  InstanceClass::Fast clazz = method->holder();
  // find this breakpoint
  VMEvent::Raw ep =
    find_breakpoint_event(JavaDebugger::get_object_id_by_ref(&clazz),
                          JavaDebugger::get_method_id(method),
                          (jlong)bci, -1);

  if (ep.is_null()) {
    // if we didn't find the breakpoint event, then just replace the bytecode
    method->bytecode_at_put_raw(bci, bcode);
  } else {
    // Set the saved opcode to the new fast opcode
    LocationModifier::Raw location = get_modifier(&ep,
        JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
    GUARANTEE(!location.is_null(), "No location modifier for event");
    location().set_save_opcode(bcode);
  }
  return;
}

void VMEvent::remove_event_request(VMEvent *ep)
{

  VMEvent::Raw eph = Universe::vmevent_request_head();
  VMEvent::Raw epPrev;

  if (ep->is_null()) {
    return;
  }
  if (ep->equals(&eph)) {
    // remove head of list
    *Universe::vmevent_request_head() = eph().next();
    return;
  }
  epPrev = eph;
  eph = eph().next();
  while (eph.not_null()) {
    if (ep->equals(&eph)) {
      VMEvent::Raw epnext = eph().next();
      epPrev().set_next(&epnext);
      return;
    }
    epPrev = eph;
    eph = eph().next();
  }
}

void VMEvent::class_prepare_event(JavaClass *jc)
{

  if (Universe::is_bootstrapping() || !JavaDebugger::is_debugger_option_on()) {
    return;
  } else {
    if (_debugger_active) {
      // create the event 
      DebuggerEvent d_event(JDWP_EventKind_CLASS_PREPARE,
                            JavaDebugger::get_thread_id_by_ref(Thread::current()),
                            JavaDebugger::get_object_id_by_ref(jc), 0, 0);
      send_event(&d_event);
    }
  }
}

void VMEvent::thread_event(Thread *t, bool is_start)
{
  UsingFastOops fast_oops;

  GUARANTEE(_debugger_active, "sending thread event, debugger not active");
  jbyte kind =
    is_start ? JDWP_EventKind_THREAD_START: JDWP_EventKind_THREAD_DEATH;
  DebuggerEvent d_event(kind, JavaDebugger::get_thread_id_by_ref(t), 0, 0, 0);
  send_event(&d_event);
}

// Called from interpreter to handle a breakpoint opcode.
// Returns the original bytecode so that the interpreter can dispatch to it

extern "C" {
  Bytecodes::Code  handle_breakpoint(Thread *thread)
  {
    UsingFastOops fast_oops;

    Bytecodes::Code opcode;
    VMEvent::Fast ep;
    LocationModifier::Fast break_location;
    JavaFrame fr(thread);
    Method::Fast m = fr.method();
    InstanceClass::Fast ic = m().holder();
#ifdef AZZERT
    if (TraceDebugger) {
      tty->print_cr("handle_breakpoint: task id = %d", Thread::current()->task_id());
    }
#endif
    ep = VMEvent::find_breakpoint_event(JavaDebugger::get_object_id_by_ref(&ic),
                                        JavaDebugger::get_method_id(&m),
                                        (jlong)fr.bci(),
                                        Thread::current()->task_id());
    if (ep.not_null()) {
      break_location = VMEvent::get_modifier(&ep,
          JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
      GUARANTEE(!break_location.is_null(), "No location modifier for event");
      opcode = break_location().save_opcode();
      if (Thread::current()->status() & THREAD_JUST_BORN) {
        Thread* t = Thread::current();
        // JDB has a problem if it gets the thread event *after* the breakpoint
        t->set_status(t->status() & ~THREAD_JUST_BORN);
        VMEvent::thread_event(t, true);
      }
      DebuggerEvent d_event(JDWP_EventKind_BREAKPOINT,
                            JavaDebugger::get_thread_id_by_ref(Thread::current()),
                            JavaDebugger::get_object_id_by_ref(&ic),
                            JavaDebugger::get_method_id(&ic, &m),
                            (jlong)fr.bci());
      VMEvent::send_event(&d_event);
      return opcode;
    } else {
      GUARANTEE(false, "Could not find original opcode");
      // Help!
      return Bytecodes::_illegal;
    }
  }
}

// called to handle a single step check
/*========================================================================
 * Function:        handle_single_tep()
 * Overview:        determines when a stepping event has occurred
 *                  stepping can be in, over, or out.
 *                  
 * Interface:
 *  parameters:     
 *  returns:        
 *
 * Notes:
 *    How single stepping works
    Assume you are sitting at a breakpoint in some code and you
    decide to single step.  The debugger issues an event request for
    a single step event.  The VM gets this request in create_event_request()
    and sends a vendor specific command to the debug agent to obtain 
    information as to where the next line may be. The proxy returns
    with three pieces of info (see JavaDebugger::vendor_get_stepping_info()).
     1) the offset of the next line that might be executed.  Note
        that the debug agent doesn't really know if the next line will
        be executed as there could be a goto type of instruction in the
        bytecode.
     2) The offset of code that has the same line number as the above
        offset (or -1 if no duplicate).  You would only get a duplicate
        if there is a for loop or while loop (as far as I know).
     3) If there is a duplicate offset, then this offset is the offset of
        the line after the above duplicate offset.  Here's an example.

    Line #
     1            public void run() {
     2                for (int j = 0; j < 2; j++) {
     3                    int k = j;
     4                }
     5           }

    Code for Method void run()
    Offset
     0     iconst_0
     1     istore_1
     2     goto 10
     5     iload_1
     6     istore_2
     7     iinc 1 1
    10     iload_1
    11     iconst_2
    12     if_icmplt 5
    15     return

    Line numbers for method void run()

       line #: code offset

       line 2: 0
       line 3: 5
       line 2: 7
       line 5: 15

    You set a breakpoint at line 2 and run the program and hit the
    breakpoint.  Now you single step.  The debugger requests a single
    step event.  The VM calls back to the debug agent requesting stepping
    info.  The proxy responds with this info:
        target offset  5
            duplicate offset  7
            offset of line after duplicate  15
    As you can see from the above line number table, line 2 corresponds
    to two offsets.  Offset 0 is the start of the for loop and offset 7
    is the increment and test part of the loop.
      All this info is stored in the StepModifier class which is pointed to
    by the Thread class and we return back to the interpreter loop.  At 
    the top of the loop we call back to this file to the 
    handle_single_tep() function (below).  We obtain the current frame
    pointer and the current offset into the method code. We then
    start testing to see if we need to send a single step event to
    the debugger.  If the frame is the same as the one when we started this
    single step process and we are not doing a step OUT type of single
    step, we check to see if the current offset has reached the 
    target offset.  If so then send the event.  If not then we check to
    see if the current offset is less than the starting offset.  If so
    then we must have jumped back somehow so we send an event.  Next
    we test if the current offset is > the target offset.  If so we
    check to see if the dup_current_line_offset or post_dup_line_offset
    is -1. If so then there is no duplicate line info and this isn't a for or 
    while loop so we just send the event.  If there is dup line info
    then we check to see if the current offset is >= the
    dup_current_line_offset.
    If it is then we may be at the end of the loop or we may have gone
    beyond the end of the loop to the next line.  We check to see if
    the current offset >= post_dup_line_offset and if so then we've gone out
    of the loop so send an event.  Otherwise we haven't left the loop and
    we just continue interpreting and repeat the whole process.
    Eventually we reach the offset 5 (which was our target offset) and send
    the single step event.
    If we single step again we won't have any duplicate line info 
    (since line 3 has only one entry in the table) so we simply send
    an event when we reach offset 7.  Now we are stopped at the end of the
    loop although the debugger is correctly placing the cursor at line
    2 since offset 7 corresponds to line 2.  When we single step now, cldc_vm
    will get the stepping info from the debug agent.  Since this is
    a duplicate line (line 2 is at offset 0 and offset 7) we will get
    duplicate offset information back.  If you look at the code in
    kdp/classparser/attributes/LineNumberTableAttribute.java for the
    method getNextExecutableLineCodeIndex() you notice that if there is
    a duplicate entry we check to see if the duplicate offset (in this
    case offset 0 since we are at offset 7) is before or after the 
    current offset.  We always return the lesser of the two, in this 
    case we return offset 5 as the target since it's the next line after
    the line that corresponds to offset 0. So the next time we step we
    will step back to line 3 which is what we expect.  However, if the
    loop test terminates the loop then we will not go to line 3 offset 5
    but rather to offset 15 line 5.  This will be caught by the test that
    looks for the current offset >= post_dup_line_offset and we will send the
    event with the offset of 15.

    If the frame has changed then we do testing to determine if we've 
    stepped into a new function or popped back up to the caller.  Depending
    on the type of step (step OVER or step INTO or step OUT) we 
    determine whether or not we need to send an event.

 *=======================================================================*/

extern "C" {
  void handle_single_step(Thread *thread)
  {

    UsingFastOops fast_oops;

    julong offset;
    bool sendEvent = false;

    /* if the proxy has returned -1 it means that we have reached
     * the end of a function and we must look up stack for the
     * calling function
     *
     */
    JavaFrame frame(thread);
    GUARANTEE(frame.is_java_frame(), "single step not in JavaFrame!");
#if !ENABLE_ROM_JAVA_DEBUGGER    
    // Don't allow step events if we are in a ROM method
    GUARANTEE(UseROM, "Sanity");
    Method::Raw m = frame.method();    
    if (ROM::in_any_loaded_bundle(m.obj())) {
      return;
    }
#endif
    offset = frame.bci();
    StepModifier::Fast sm = thread->step_info();
    if (sm.is_null()) {
      // Huh?  this can't be.  Turn off single stepping for this thread 
      thread->set_is_stepping(false);
      return;
    }
    address starting_fp = DERIVED(address, sm().step_starting_fp(),
                                  thread->stack_base());
    if (sm().step_target_offset() == -1) {
        /*
         * Wait for the frame to change and that will mean we popped up to
         * the calling frame. (Or we called down to another frame, maybe...
         * should look into that possibility)
         */
      if (starting_fp != frame.fp()) {
        sendEvent = true;
        /* the frame has changed but we don't know whether we
         * have gone into a function or gone up one.
         * if we look up the call stack and find the frame 
         * that originally set the request then we stepped
         * into a function, otherwise we stepped out 
         */
        if (sm().step_depth() == JDWP_StepDepth_OVER ||
            sm().step_depth() == JDWP_StepDepth_OUT) {
          if (StepModifier::find_frame(starting_fp, &frame)) {
            sendEvent = false;
          }
        }
      }
    } else {
        switch (sm().step_size()) {

        case JDWP_StepSize_LINE:
            /* stepping by line can have two effects: we stay in the same 
             * frame or the frame changes
             */
          if (starting_fp == frame.fp()) {
            /* if the frame is the same and we are not running to the
             * end of a function we need to see if we have reached
             * the location we are looking for
             */
            if (sm().step_depth() != JDWP_StepDepth_OUT) {
              if (offset == sm().step_target_offset()) {
                /* we reached the exact offset we were looking for */
                sendEvent = true;
                break;
              } else if (offset < sm().step_starting_offset()) {
                /* we ended up before the offset were the stepping
                 * request was initiated. we jumped back
                 */
                sendEvent = true;
                break;
              } else if (offset > sm().step_target_offset() &&
                         offset > sm().step_starting_offset()) {
                if (sm().dup_current_line_offset() == -1) {
                  /* We stepped beyond the target and there's no
                   * duplicate offset information
                   * (like there would be in a for or while loop).
                   * Most likely a break in a switch case.
                   * However, we checked to see if our starting offset
                   * is equal to our current, if so don't issue
                   * an event otherwise we'll be in an infinite loop.
                   */
                  sendEvent = true;
                  break;
                } else if (offset >= sm().dup_current_line_offset()) {
                  /*
                   * We are after the location we were looking for;
                   * we jumped forward.  However, we may be at the end
                   * of a for/while loop where we do the loop
                   * test.  We check to see if we are in the line after
                   * the for/do end line.  If not, we just continue.
                   */
                  if (sm().post_dup_line_offset() != -1) {
                    /*
                     * There is a line after the duplicate line
                     * so check to see if we've past it.  If 
                     * post_dup_line_offset == -1 then there is no line
                     * after the duplicate so we continue until we
                     * either jump back or return from this method
                     */
                    if (offset >= sm().post_dup_line_offset()) {
                      /* we were beyond the end of the loop */
                      sendEvent = true;
                      break;
                    }
                  }
                }
              }
            }
          } else {
            /* the frame has changed but we don't know whether we
             * have gone into a function or gone up one.
             * if we look up the call stack and find the frame 
             * that originally set the request then we stepped
             * into a function, otherwise we stepped out 
             */
            sendEvent = true;
            if (sm().step_depth() == JDWP_StepDepth_OVER ||
                sm().step_depth() == JDWP_StepDepth_OUT) {
              if (StepModifier::find_frame(starting_fp, &frame)) {
                sendEvent = false;
              }
            }
          }

          break;

          /* stepping by bytecode */
        case JDWP_StepSize_MIN:
            
          if (sm().step_depth() == JDWP_StepDepth_OVER) {
            // if we are stepping by bytecode then if we are in the
            // same frame as we started in, send the event.
            // if we are not in the same frame then see if we are deeper in the
            // call stack.  If so then don't send the event since we are in
            // Step Over mode.
            if (starting_fp == frame.fp()) {
              sendEvent = true;
            } else {
              sendEvent = true;
              /* If we are deeper in the call stack do not break... */
              if (StepModifier::find_frame(starting_fp, &frame)) {
                sendEvent = false;
              }
            }
          } else if (sm().step_depth() == JDWP_StepDepth_OUT) {
            // in this case, we want to send the event only if we've popped
            // up one frame (i.e. to the caller of the starting frame)
            if (starting_fp != frame.fp()) {
              sendEvent = true;
              /* If we are deeper in the call stack do not break... */
              if (StepModifier::find_frame(starting_fp, &frame)) {
                sendEvent = false;
              }
            }
          } else {
            sendEvent = true;
          }
          break;
        }
    }

    if (sendEvent) {
      UsingFastOops fast_oops_2;

      Method::Fast m = frame.method();
      InstanceClass::Fast ic = m().holder();
      if (m().bytecode_at_raw(frame.bci()) == Bytecodes::_breakpoint) {
        // Don't single step into a breakpoint
        return;
      }
      if (JavaDebugger::get_method_id(&ic, &m) == -1) {
        // Most likely this method was removed from the ROM because the
        // ROM was built without MakeROMDebuggable.  There's no sense in
        // sending an event since we can't identify the method to the 
        // debugger
        return;
      }
      DebuggerEvent d_event(JDWP_EventKind_SINGLE_STEP,
                            JavaDebugger::get_thread_id_by_ref(Thread::current()),
                            JavaDebugger::get_object_id_by_ref(&ic),
                            JavaDebugger::get_method_id(&ic, &m),
                            (jlong)frame.bci());
      d_event.set_step_size(sm().step_size());
      d_event.set_step_depth(sm().step_depth());
      VMEvent::send_event(&d_event);
      return;
    } else {
      return;
    }
  }
}

// Allocate a new VMEvent object

ReturnOop VMEvent::create_vm_event_request()
{
  UsingFastOops fast_oops;

  SETUP_ERROR_CHECKER_ARG;
  VMEvent::Fast ep;

  SAVE_CURRENT_EXCEPTION;
  ep = VMEvent::allocate(JVM_SINGLE_ARG_NO_CHECK);
  RESTORE_CURRENT_EXCEPTION;
  if (ep.is_null()) {
    return ep;
  }
  ep().set_event_id(JavaDebugger::next_seq_num());
  return ep;
}

// Create a new event based on the passed in eventrequest information

void VMEvent::create_event_request(PacketInputStream *in, 
                                   PacketOutputStream *out, jbyte event_kind)
{

  UsingFastOops fast_oops;
  VMEvent::Fast ep;
  int i;
  bool error = false;
  VMEventModifier::Fast current_modifier_slot, new_modifier, event_modifier;

  {
    TaskAllocationContext tmp(SYSTEM_TASK);
    ep = create_vm_event_request();
  }
    
  ep().set_event_kind(event_kind);
  ep().set_suspend_policy(in->read_byte());
  ep().set_num_modifiers(in->read_int());
  Transport *transport = in->transport();
  ep().set_transport(transport);
#if ENABLE_ISOLATES
  ep().set_task_id(transport->task_id());
  TaskGCContext tmp(transport->task_id());
#endif
  for (i=0; i < ep().num_modifiers(); i++) {

    {
      TaskAllocationContext tmp(SYSTEM_TASK);
      new_modifier = VMEventModifier::new_modifier(in, out, error);
    }

    if (error) {
      // some sort of error happened
      if (out->error() != 0) {
        out->send_packet();
      }
      return;
    }
    if (new_modifier.is_null()) {
      // Most likely we don't support this modifier
      continue;
    }
    if (event_kind == JDWP_EventKind_BREAKPOINT &&
        new_modifier().mod_kind() ==
        JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly) {
      UsingFastOops fastoops2;
      LocationModifier *lmp = (LocationModifier *)&new_modifier;
      VMEvent::Fast epb = VMEvent::find_breakpoint_event(lmp,
                                                         transport->task_id());
      if (epb.not_null()) {
        out->write_int(epb().event_id());
        out->send_packet();
        //        out->send_error(JDWP_Error_NOT_IMPLEMENTED);
        // there's a breakpoint here already, don't bother installing another
        return;
      }
#if ENABLE_ISOLATES
      InstanceClass::Raw ic = JavaDebugger::get_object_by_id(new_modifier().clazz_id());
      if (ic().class_id() < ROM::number_of_system_classes()) {
        // breakpoint in system class, allow in any task
        ep().set_task_id(-1);
      }
      // See if any other task already has a breakpoint here.
      epb = find_breakpoint_event(lmp);
      if (!epb.is_null()) {
        // Has to be some other task since we haven't linked in this event
        GUARANTEE(epb().task_id() != transport->task_id(),
                  "Breakpoint already inserted");
        LocationModifier::Raw lmod = get_modifier(&epb,
                   JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
        GUARANTEE(!lmod.is_null(),"Breakpoint event has no location modifier");
        lmp->set_save_opcode(lmod().save_opcode());
      } else {
#endif
        /* Return an error back to the debugger if 
         * the breakpoint could not be installed.
         */
        lmp->unlink_method();
        lmp->save_method_entry();
        if (lmp->set_method_opcode(Bytecodes::_breakpoint, true) ==
            false){
          out->send_error(JDWP_Error_INVALID_LOCATION);
          return;
        }
#if ENABLE_ISOLATES
      }
#endif
    }
    // insert the mod at the end of the list of modifiers
    event_modifier = ep().mods();
    if (event_modifier.is_null()) {
      ep().set_mods(&new_modifier);
      current_modifier_slot = ep().mods();
    } else {
      current_modifier_slot().set_next(&new_modifier);
      current_modifier_slot = new_modifier;
    }
  }
#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("Create Event: xprt = 0x%x, ep = 0x%lx, id = 0x%x, kind = 0x%lx, suspend = 0x%lx, num mods = 0x%lx",
                  transport->obj(),
                  ep.obj(), 
                  ep().event_id(),
                  ep().event_kind(), 
                  ep().suspend_policy(), 
                  ep().num_modifiers());
  }
#endif
  ep().set_next((VMEvent *)Universe::vmevent_request_head());
  *Universe::vmevent_request_head() = ep;
  out->write_int(ep().event_id());
  out->send_packet();
  VMEvent::add_notification(JDWP_eventkind_to_dbg_eventkind(event_kind));
}

// Debugger is requesting to receive an event

void 
VMEvent::event_request_set(PacketInputStream *in, PacketOutputStream *out)
{
    jbyte eventType;

#ifdef AZZERT
    if (TraceDebugger) {
      tty->print_cr("Event Set");
    }
#endif

    eventType = in->read_byte();

    if (JDWP_eventkind_to_dbg_eventkind(eventType) == 0) {
      // JBuilder2006 just absolutely fails if we send this error
      // We just return an ID and carry on.
      //      out->send_error(JDWP_Error_INVALID_METHODID);
      out->write_int(JavaDebugger::next_seq_num());
      out->send_packet();
      return;
    }
    create_event_request(in, out, eventType);
}

void
VMEvent::clear_impossible_to_compile(LocationModifier *mod, VMEvent *ep)
{
  UsingFastOops fast_oops;

  // If we are using the compiler then we should reset the
  // impossible_to_compile flag for this method (and potentially one frame
  // up if it's a single step).
  // We also check the previous state of the method, if it was
  // "impossible_to_compile" we don't reset the flag.

  InstanceClass::Fast clazz;
  Method::Fast method, callerMethod;
  LocationModifier::Fast thisMod;

  Method::Fast m = mod->method();
  //  if (!m().has_compiled_code()) {
    // if method does not have compiled code then just return
  //    return;
  //  }
  if (m.is_null()) {
    // Method was removed.  Most likely it was a <clinit> method
    return;
  }
  VMEvent::Fast epm;
  VMEventStream es;
  bool found_one = false;
  while (!es.at_end()) {
    epm = es.next();
    if ((epm().event_kind() == JDWP_EventKind_BREAKPOINT ||
         epm().event_kind() == JDWP_EventKind_SINGLE_STEP) &&
        (ep == NULL || !epm.equals(ep))) {
      thisMod = get_modifier(&epm,
         JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
      if (thisMod.not_null()) {
        method = thisMod().method();
        if (method.equals(&m)) {
          // another breakpoint in this method, keep impossible_to_compile set
          found_one = true;
          break;
        }
      }
    }
  }
  if (!found_one) {
    // we must have looped through the whole list and not found another
    // breakpoint in this method so clear the impossible_to_compile flag
    if (mod->rom_debug_method() != NULL) {
      // This method is in ROM, let's check all method pointers on the java
      // stack to see if any of them point to this rom_debug_method.
      // We may be called as a result of JVM::cleanup(). Thread doesn't have
      // a stack in that case.
      Thread *thread = Thread::current();
      if (thread->last_java_fp() != NULL && thread->last_java_sp() != NULL) {
        Frame fr(Thread::current());
        while (true) {
          if (fr.is_entry_frame()) {
            EntryFrame e = fr.as_EntryFrame();
            if (e.is_first_frame()) {
              break;
            }
            e.caller_is(fr);
          } else if (fr.is_java_frame()) {
            JavaFrame jf = fr.as_JavaFrame();
            if (jf.method() == mod->rom_debug_method()) {
              MethodDesc *md = (MethodDesc *)mod->method();
              // fix up the stored bcp in this frame
              int bci = jf.bci_with_flags();
              jf.set_raw_method(md);
              Method::Raw m = jf.method();
              jf.set_raw_bcp((address)(bci + m().code_base()));
            }
            jf.caller_is(fr);
          }
        }
      }
    }
    // We also check the previous state of the method, if it was
    // "impossible_to_compile" we don't reset the flag.
    if (mod->compile_state() == true) {
      // Method was compilable so set entry to default
      m().set_default_entry(false);
    } else {
      if (ep->event_kind() == JDWP_EventKind_BREAKPOINT) {
        // May have been a special native method like String.charAt.
        // Just replace the entry with what we had saved earlier
        GUARANTEE(!ObjectHeap::contains((OopDesc*)mod->saved_method_entry()),
                  "ROM method entry is in heap");
        m().variable_part()->set_execution_entry(mod->saved_method_entry());
      }
    }
  }
}

// Remove this event from our list of requested events

void VMEvent::clear_event_request(VMEvent *ep)
{
  LocationModifier::Raw mod;
  Thread::Raw thread;

  VMEvent::remove_event_request(ep);
#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("VMEvent: clear 0x%x, id 0x%x", (int)ep->obj(), ep->event_id());
  }
#endif
  switch (ep->event_kind()) {
  case JDWP_EventKind_BREAKPOINT:
    mod = get_modifier(ep,
        JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
    GUARANTEE(!mod.is_null(), "No location modifier for breakpoint event");
    if (!mod.is_null()) {
#if ENABLE_ISOLATES
      // See if any other task already has a breakpoint here.
      VMEvent::Raw epb = find_breakpoint_event(&mod);
      if (!epb.is_null()) {
        // Has to be some other task since we have unlinked this event
        GUARANTEE(epb().task_id() != ep->task_id(),
                  "Duplicate breakpoint event for this task");
      } else {
#endif
        mod().set_method_opcode(mod().save_opcode(), false);
#if ENABLE_ISOLATES
      }
#endif
    }
    break;
  case JDWP_EventKind_SINGLE_STEP:
    mod = get_modifier(ep,
        JDWP_EventRequest_Set_Out_modifiers_Modifier_Step);
    if (mod.not_null()) {
      thread = JavaDebugger::get_thread_by_id(mod().thread_id());
      if (thread.not_null())
        thread().set_is_stepping(false);
      JavaDebugger::set_stepping(false);
    }
    break;
  }
  // see if there are any other breakpoints in this method.  If no other 
  // breakpoints and the compiler is active then clear the
  // impossible_to_compile flag.
  if (mod.not_null() && (ep->event_kind() == JDWP_EventKind_BREAKPOINT ||
         ep->event_kind() == JDWP_EventKind_SINGLE_STEP)) {
    clear_impossible_to_compile(&mod, ep);
  }
}

// Debugger wants to remove this event request

void 
VMEvent::event_request_clear(PacketInputStream *in, 
                   PacketOutputStream *out)
{
  jbyte eventType = in->read_byte();
  jint eventID = in->read_int();
  VMEvent::Raw ep;
  
#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("Event Clear");
  }
#endif

  eventType = 0;      /* satisfy GCC */
  VMEventStream es;

  while(!es.at_end()) {
    ep = es.next();
    if (ep().event_id() == eventID) {
      clear_event_request(&ep);
    }
  }
  out->send_packet();
}

void 
VMEvent::event_request_clear_all_breakpoints(PacketInputStream *in, 
                                 PacketOutputStream *out)
{

  clear_all_breakpoints(in->transport());
  out->send_packet();
}

void
VMEvent::clear_all_events(Transport *t)
{

  VMEventStream es;
  UsingFastOops fast_oops;

  VMEvent::Fast ep;

  ep = es.next_by_task(t->task_id());
  while (!ep.is_null()) {
    clear_event_request(&ep);
    ep = es.next_by_task(t->task_id());
  }
}

void
VMEvent::clear_all_breakpoints(Transport *t)
{

  VMEventStream es;
  UsingFastOops fast_oops;

  VMEvent::Fast ep;

  ep = es.next_by_kind(JDWP_EventKind_BREAKPOINT, t->task_id());
  while (!ep.is_null()) {
    clear_event_request(&ep);
    ep = es.next_by_kind(JDWP_EventKind_BREAKPOINT, t->task_id());
  }
}

//  Set the notification flags to only process those events whose bits are set

void VMEvent::set_notification(int n)
{
  _debugger_notify_list = n;
}

// Enable an event

void VMEvent::add_notification(int n)
{
  _debugger_notify_list |= n;
}

// Send the initialization event to the debugger

void VMEvent::vminit(Transport *t)
{
  int reqID = PacketStream::unique_id() /* side-effects*/;
  (void)reqID;
  int threadID = JavaDebugger::get_thread_id_by_ref(Thread::current());
  bool is_suspend = true;

  PacketOutputStream out(t, JDWP_EVENT_VMINIT_LEN,
                         JDWP_COMMAND_SET(Event),
                         JDWP_COMMAND(Event, Composite));

  if ((is_suspend = JavaDebugger::is_suspend()) != false) {
    // Suspend all threads by default
    DEBUGGER_EVENT(("VM Suspend All"));
    out.write_byte(JDWP_SuspendPolicy_ALL);
  } else {
    // Suspend no threads
    DEBUGGER_EVENT(("VM Suspend None"));
    out.write_byte(JDWP_SuspendPolicy_NONE);
  }
  out.write_int(1);
  out.write_byte(JDWP_EventKind_VM_START);
  out.write_int(0);

  out.write_int(threadID);
  out.send_packet();

  if (is_suspend) {
    int task_id = -1;
#if ENABLE_ISOLATES
    task_id = Task::current_id();
#endif
    // Use suspend policy which suspends all threads by default
    JavaDebugger::process_suspend_policy(JDWP_SuspendPolicy_ALL,
                                         task_id,
                                         true);
  } else {
    // Use suspend policy which suspends no threads
    DEBUGGER_EVENT(("VM SuspendPolicy not specified"));
    JavaDebugger::process_suspend_policy(JDWP_SuspendPolicy_NONE,
                                         -1,
                                         true);
  }
  if (_debugger_active) {
    JavaDebugger::send_all_class_prepares();
  }
}

// The VM is being shutdown

void VMEvent::vmdeath(Transport *transport)
{
  //  check_notify_wanted(Dbg_EventKind_VM_DEATH);
  PacketOutputStream out(transport, JDWP_EVENT_VMDEATH_LEN,
                         JDWP_COMMAND_SET(Event),
                         JDWP_COMMAND(Event, Composite));
  DEBUGGER_EVENT(("VM Death"));
  out.write_byte(JDWP_SuspendPolicy_ALL);
  out.write_int(1);
  out.write_byte(JDWP_EventKind_VM_DEATH);
  out.write_int(0); /* spec says always zero */
  out.send_packet();
}

// Send an event on method entry (unsupported at this time)

void set_event_method_entry()
{
    check_notify_wanted(Dbg_EventKind_METHOD_ENTRY);
}

// Send an event on method exit (unsupported at this time)

void set_event_method_exit()
{
    check_notify_wanted(Dbg_EventKind_METHOD_EXIT);
}

// Send an event on field access (unsupported at this time)

void set_event_field_access()
{
    check_notify_wanted(Dbg_EventKind_FIELD_ACCESS);
}

// Send an event on field modification (unsupported at this time)

void set_event_field_modification()
{
    check_notify_wanted(Dbg_EventKind_FIELD_MODIFICATION);
}

// Send an event on frame pop (unsupported at this time)

void set_event_frame_pop()
{
    check_notify_wanted(Dbg_EventKind_FRAME_POP);
}

extern "C" {
void handle_exception_info(Thread *thread) {
  UsingFastOops fast_oops;

  JavaFrame throw_frame(thread);
  VMEvent::Fast info_event =
    VMEvent::find_event((jbyte)VM_EXCEPTION_INFO_EVENT);
  if (info_event.not_null()) {
    VMEvent::remove_event_request(&info_event);
  }
  info_event = VMEvent::create_vm_event_request();
  LocationModifier::Fast loc = LocationModifier::new_location(&throw_frame);
  if (info_event.is_null() || loc.is_null()) {
    // punt, out of memory or we couldn't find this location
    return;
  }
  info_event().set_event_kind((jbyte)VM_EXCEPTION_INFO_EVENT);
  loc().set_mod_kind(JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
  info_event().set_mods(&loc);
  info_event().set_next((VMEvent *)Universe::vmevent_request_head());
  *Universe::vmevent_request_head() = info_event;
}
}

void VMEvent::handle_uncaught_exception(Thread *THREAD, JavaOop *exception)
{
  (void)THREAD; // IMPL_NOTE: do we need this param?
  // Called with an entry frame at the top of the stack
  UsingFastOops fast_oops;

  Throwable::Fast exc = exception;
  InstanceClass::Fast exception_class = exc().blueprint();
  DebuggerEvent d_event(JDWP_EventKind_EXCEPTION,
                        JavaDebugger::get_thread_id_by_ref(Thread::current()),
                        JavaDebugger::get_object_id_by_ref(&exception_class), 0, 0);
  d_event.set_sig_uncaught(true);
  //  THREAD->set_status(THREAD_DEAD);
  VMEvent::exception_event(&exc, NULL, &d_event, 0);
}

void VMEvent::handle_caught_exception(Thread *thread, JavaFrame *catch_frame,
                                      int bci, JavaOop *exception)
{
  (void)thread; // IMPL_NOTE: do we need this param?
  UsingFastOops fast_oops;
  Throwable::Fast exc = exception;
  InstanceClass::Fast exception_class = exc().blueprint();
  Method::Fast catch_method = catch_frame->method();
  DebuggerEvent d_event(JDWP_EventKind_EXCEPTION,
                        JavaDebugger::get_thread_id_by_ref(thread),
                        JavaDebugger::get_object_id_by_ref(&exception_class),
                        JavaDebugger::get_method_id(&catch_method),
                        (jlong)catch_frame->bci());
  d_event.set_sig_caught(true);
  VMEvent::exception_event(&exc, catch_frame, &d_event, bci);
}

// Send an event on an exception

void VMEvent::exception_event(Throwable *exception,
                                   JavaFrame *catch_frame,
                                   DebuggerEvent *d_event, int catch_offset)
{
  check_notify_wanted(Dbg_EventKind_EXCEPTION);

  UsingFastOops fast_oops;

  VMEvent::Fast ep, ep_2;
  jlong throw_offset = 0;
  int event_count = 0;
  jbyte suspend_policy = JDWP_SuspendPolicy_NONE;
  InstanceClass::Fast ic;
  LocationModifier::Fast location;
  Method::Fast catch_method;
  Method::Fast throw_method;
  int data_len = JDWP_EVENT_LEN;

  ep = ep_2 = get_event_request(d_event, event_count, suspend_policy);
  if (ep.is_null()) {
    return;
  }
  // Flush any packets waiting in the queue.  This helps avoid a race
  // condition where we may have a resume command in the queue for a
  // previous event, we send this event, process the resume command
  // out of order
  JavaDebugger::dispatch(0);

  // Calculate packet length
  data_len += (JDWP_EVENT_EXCEPTION_LEN * event_count);

  Transport::Fast transport = ep().transport();
  PacketOutputStream out(&transport, data_len, JDWP_COMMAND_SET(Event),
                         JDWP_COMMAND(Event, Composite));
  // Create a buffered output stream so we can asynchronously send an error
  // Calculate the size based on half of the items being 'longs'
  UsingFastOops fast_oops_2;

  Thread::Fast thread = JavaDebugger::get_thread_by_id(d_event->thread_id());
  VMEventModifier::deoptimize_frame(&thread, true);


  VMEvent::Fast info_event = find_event((jbyte)VM_EXCEPTION_INFO_EVENT);
  if (!info_event.is_null()) {
    location = get_modifier(&info_event,
        JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
    GUARANTEE(!location.is_null(), "No location modifier in info event");
    throw_method = location().method();
    throw_offset = location().offset();
    remove_event_request(&info_event);
  } else {
    UsingFastOops fast_oops_3;
    ObjArray::Fast trace, methods;
    TypeArray::Fast offsets;
    trace = exception->backtrace();
    if (!trace.is_null()) {
      methods = trace().obj_at(0);
      offsets = trace().obj_at(1);
      if (!methods.is_null() && !offsets.is_null()) {
        throw_method = methods().obj_at(0);
        throw_offset = (jlong)(offsets().int_at(0));
      }
    }
  }

  DEBUGGER_EVENT(("Exception"));
  out.write_byte(suspend_policy);
  out.write_int(event_count);
  while (ep.not_null()) {
    out.write_byte(JDWP_EventKind_EXCEPTION);        
    out.write_int(ep().event_id());

    // thread with exception
    out.write_int(d_event->thread_id());

    // location of exception throw
    if (throw_method.not_null()) {
      ic = throw_method().holder();
    }
    DebuggerEvent throw_event(JDWP_EventKind_EXCEPTION,
                              0, // don't need thread
                              JavaDebugger::get_object_id_by_ref(&ic),
                              JavaDebugger::get_method_id(&ic, &throw_method),
                              (jlong)throw_offset);
    throw_event.write_as_location(&out);

    // thrown exception 
    out.write_byte('L');
    out.write_object(exception);

    // location of catch, or 0 if not caught

    if (catch_frame == NULL) {
      LocationModifier::write_null_location(&out);
    } else {
      catch_method = catch_frame->method();
      ic = catch_method().holder();
      DebuggerEvent catch_event(JDWP_EventKind_EXCEPTION,
                                0, // don't need thread
                                JavaDebugger::get_object_id_by_ref(&ic),
                                JavaDebugger::get_method_id(&ic, &catch_method),
                                (jlong)catch_offset);
      catch_event.write_as_location(&out);
    }
    ep = ep().send_next();
  }
  out.send_packet();
  JavaDebugger::process_suspend_policy(suspend_policy, &thread,
                                      true);
}

// Pseudo event sent to debug agent.
//  Agent sends a command back to the VM with single stepping info

void VMEvent::stepping_info_request(DebuggerEvent *d_event,
                                    Transport *t,
                                    LocationModifier *loc)
{
  (void)d_event; // IMPL_NOTE: do we need this param?
  UsingFastOops fastoops;
  PacketOutputStream out(t, JDWP_COMMAND_STEPPING_INFO_LEN,
                         KVM_CMDSET, KVM_STEPPING_EVENT_COMMAND);

  DEBUGGER_EVENT(("Need Stepping Info"));

  // IMPL_NOTE: GC problem ?? I don't think so as req is a handle and the address of 
  // req is valid in the previous stack frame which stays in existence until
  // the agent sends the info back
  out.write_int((jint)(loc));
  loc->write(&out, false);
  out.send_packet();
}

// Main function for sending events to debugger

void VMEvent::send_event(DebuggerEvent *d_event) {
  check_notify_wanted(JDWP_eventkind_to_dbg_eventkind(d_event->event_kind()));

  UsingFastOops fast_oops;

  VMEvent::Fast ep, ep_2;
  JavaClass::Fast jc;
  int event_count = 0;
  jbyte suspend_policy = JDWP_SuspendPolicy_NONE;
  int data_len = JDWP_EVENT_LEN;

  ep = ep_2 = get_event_request(d_event, event_count, suspend_policy);
  if (ep.is_null()) {
    return;
  }

  // Flush any packets waiting in the queue.  This helps avoid a race
  // condition where we may have a resume command in the queue for a
  // previous event, we send this event, process the resume command
  // out of order
  JavaDebugger::dispatch(0);

  // Calculate packet length
  while (ep_2.not_null()) {
    switch(ep_2().event_kind()) {
    case JDWP_EventKind_SINGLE_STEP:
    case JDWP_EventKind_BREAKPOINT:
      data_len += JDWP_EVENT_BREAK_LEN;
      break;
    case JDWP_EventKind_THREAD_START:
    case JDWP_EventKind_THREAD_DEATH:
      data_len += JDWP_EVENT_THREAD_LEN;
      break;
    case JDWP_EventKind_CLASS_PREPARE:
    case JDWP_EventKind_CLASS_LOAD:
      jc = JavaDebugger::get_object_by_id(d_event->clazz_id());
      data_len += (9 + class_prepare_info_length(&jc));
      break;
    case JDWP_EventKind_CLASS_UNLOAD:
      jc = JavaDebugger::get_object_by_id(d_event->clazz_id());
      data_len += (5 + PacketOutputStream::get_class_name_length(&jc));
      break;
    }
    ep_2 = ep_2().send_next();
  }

  Transport::Fast transport = ep().transport();
  PacketOutputStream out(&transport, data_len, JDWP_COMMAND_SET(Event), 
                         JDWP_COMMAND(Event, Composite));

  UsingFastOops fast_oops_2;
  Thread::Fast thread = JavaDebugger::get_thread_by_id(d_event->thread_id());
  if (thread.is_null()) {
    // Some events were created before any threads were ready, so just
    // use the current thread
    thread = Thread::current();
  }

  DEBUGGER_EVENT(("VM Event xprt=0x%x, thrd=0x%x, id=0x%x, kind=0x%x, susp.=0x%x",
                  transport.obj(), d_event->thread_id(),
                  ep().event_id(), ep().event_kind(), suspend_policy));

  out.write_byte(suspend_policy);
  out.write_int(event_count);
  while (ep.not_null()) {
    //    transport = ep().transport();
    //    out.set_transport(&transport);
    out.write_byte(ep().event_kind());
    out.write_int(ep().event_id());
    switch(ep().event_kind()) {
    case JDWP_EventKind_SINGLE_STEP:
    case JDWP_EventKind_BREAKPOINT:
      {
        UsingFastOops fast_oops_3;

        out.write_int(d_event->thread_id());
        DEBUGGER_EVENT(("Event clss=0x%x, mthd=0x%x, off=0x%x",
                        d_event->clazz_id(),
                        (int)(d_event->method_id() & 0xFFFFFFFF),
                        d_event->offset()));
#ifdef AZZERT
        if (TraceDebugger) {
          InstanceClass::Raw ic = JavaDebugger::get_object_by_id(d_event->clazz_id());
          Method::Raw m = JavaDebugger::get_method_by_id(&ic, d_event->method_id());
          m().print_name_on(tty);
          tty->cr();
        }
#endif
        d_event->write_as_location(&out);
        // IMPL_NOTE: I don't think we can ever get here with a compiled frame
        VMEventModifier::deoptimize_frame(&thread);
        break;
      }
    case JDWP_EventKind_THREAD_START:
    case JDWP_EventKind_THREAD_DEATH:
      out.write_int(d_event->thread_id());
      break;
    case JDWP_EventKind_CLASS_PREPARE:
    case JDWP_EventKind_CLASS_LOAD:
      {
        out.write_int(d_event->thread_id());
        DEBUGGER_EVENT(("Event clss=0x%x", d_event->clazz_id()));
        jc = JavaDebugger::get_object_by_id(d_event->clazz_id());
#ifdef AZZERT
        if (TraceDebugger) {
            jc().print_name_on(tty);
            tty->cr();
        }
#endif
        out.write_class_prepare_info(&jc);
        break;
      }
    case JDWP_EventKind_CLASS_UNLOAD:
      {
        jc = JavaDebugger::get_object_by_id(d_event->clazz_id());
        out.write_class_name(&jc);
        break;
      }
    }
    ep = ep().send_next();
  }
  out.send_packet();
  JavaDebugger::process_suspend_policy(suspend_policy, &thread, false);
}


void *VMEvent::event_request_cmds[] = { 
  (void *)0x3
  ,(void *)event_request_set
  ,(void *)event_request_clear
  ,(void *)event_request_clear_all_breakpoints};

#else //ENABLE_JAVA_DEBUGGER
extern "C" {
  void handle_single_step(Thread*) {}
  void handle_exception_info(Thread*) {}
}
#endif
