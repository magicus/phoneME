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

/** \class VMEvent
    Handles the processing of event requests from the debugger and the sending
    of events to the debugger.  Maintains a linked list of event requests

*/


class VMEvent : public MixedOop {
public:
  HANDLE_DEFINITION(VMEvent, MixedOop);

#if ENABLE_JAVA_DEBUGGER
  static void class_prepare_event(JavaClass *);
  static void thread_event(Thread *, bool);
  static jint JDWP_eventkind_to_dbg_eventkind(jbyte);

  static ReturnOop get_modifier(VMEvent *, jbyte);
  static ReturnOop find_event(jbyte);
  static ReturnOop find_breakpoint_event(LocationModifier *, int task_id = -1);
  static ReturnOop find_breakpoint_event(int class_id, jlong method_id,
                                         jlong offset, int task_id);
  static Bytecodes::Code get_verifier_breakpoint_opcode(const Method *, int);
  static void replace_event_opcode(Method *, Bytecodes::Code, int);
  static void add_notification(int);
  static void set_notification(int);
  static void vminit(Transport *t);
  static void vmdeath(Transport *t);
  static void handle_uncaught_exception(Thread *, JavaOop *);
  static void handle_caught_exception(Thread *, JavaFrame *, int bci,
                                      JavaOop *);
  static void handle_exception_info(Thread *);
  static void exception_event(Throwable *, JavaFrame *,
                                   DebuggerEvent *, int);
  static void stepping_info_request(DebuggerEvent *,
                                    Transport *t, LocationModifier *);
  static void remove_event_request(VMEvent *);
  static ReturnOop create_vm_event_request();
  static void create_event_request(PacketInputStream *, PacketOutputStream *,
                                   jbyte);
  static void event_request_set(PacketInputStream *, PacketOutputStream *);
  static void clear_event_request(VMEvent *);
  static void event_request_clear(PacketInputStream *, PacketOutputStream *);
  static void clear_all_breakpoints(Transport *t);
  static void clear_all_events(Transport *t);
  static void event_request_clear_all_breakpoints(PacketInputStream *,
                                                  PacketOutputStream *);
private:
  static ReturnOop get_event_request(DebuggerEvent *, int &, jbyte &);
  static int class_prepare_info_length(JavaClass *clazz) {
    return PacketOutputStream::get_class_name_length(clazz) + 13;
  }

public:

  static void send_event(DebuggerEvent *d_event);

  jbyte event_kind() {
    return byte_field(event_kind_offset());
  }
  void set_event_kind(jbyte val) {
    byte_field_put(event_kind_offset(), val);
  }

  jbyte suspend_policy() {
    return byte_field(suspend_policy_offset());
  }
  void set_suspend_policy(jbyte val) {
    byte_field_put(suspend_policy_offset(), val);
  }

  int event_id() {
    return int_field(event_id_offset());
  }
  void set_event_id(int val) {
    int_field_put(event_id_offset(), val);
  }

  ReturnOop next() {
    return obj_field(next_offset());
  }
  void set_next(Oop *p) {
    obj_field_put(next_offset(), p);
  }

  ReturnOop send_next() {
    return obj_field(sendnext_offset());
  }
  void set_send_next(Oop *p) {
    obj_field_put(sendnext_offset(), p);
  }

  ReturnOop queue_next() {
    return obj_field(queue_next_offset());
  }
  void set_queue_next(Oop *p) {
    obj_field_put(queue_next_offset(), p);
  }

  ReturnOop mods() {
    return obj_field(mods_offset());
  }
  void set_mods(Oop *mod) {
    obj_field_put(mods_offset(), mod);
  }
  void clear_mods() {
    obj_field_clear(mods_offset());
  }
  int num_modifiers() {
    return int_field(num_modifiers_offset());
  }
  void set_num_modifiers(int val) {
    int_field_put(num_modifiers_offset(), val);
  }

  ReturnOop transport() {
    return obj_field(transport_offset());
  }
  void set_transport(Oop *t) {
    obj_field_put(transport_offset(), t);
  }

  int task_id() {
    return int_field(task_id_offset());
  }
  void set_task_id(int value) {
    int_field_put(task_id_offset(), value);
  }

  static int _debugger_notify_list;
  static void *event_request_cmds[];

protected:
  static int next_offset() {
    return FIELD_OFFSET(VMEventDesc, _next);
  }
  static int sendnext_offset() {
    return FIELD_OFFSET(VMEventDesc, _send_next);
  }
  static int event_kind_offset() {
    return FIELD_OFFSET(VMEventDesc, _event_kind);
  }
  static int suspend_policy_offset() {
    return FIELD_OFFSET(VMEventDesc, _suspend_policy);
  }
  static int mods_offset() {
    return FIELD_OFFSET(VMEventDesc, _mods);
  }
  static int num_modifiers_offset() {
    return FIELD_OFFSET(VMEventDesc, _num_modifiers);
  }
  static int event_id_offset() {
    return FIELD_OFFSET(VMEventDesc, _event_id);
  }

  static int queue_next_offset() {
    return FIELD_OFFSET(VMEventDesc, _queue_next);
  }

  static int transport_offset() {
    return FIELD_OFFSET(VMEventDesc, _transport);
  }

  static int task_id_offset() {
    return FIELD_OFFSET(VMEventDesc, _task_id);
  }

private:
  static void clear_impossible_to_compile(LocationModifier *, VMEvent *);

  static ReturnOop allocate(JVM_SINGLE_ARG_TRAPS) {
    return Universe::new_mixed_oop(MixedOopDesc::Type_VMEvent,
                                   VMEventDesc::allocation_size(),
                                   VMEventDesc::pointer_count()
                                   JVM_NO_CHECK_AT_BOTTOM);
  }
#else
public:
  static void class_prepare_event(JavaClass *){}
  static void thread_event(Thread *, bool) {}
  static void handle_uncaught_exception(Thread *, JavaOop *) {}
  static void handle_caught_exception(Thread *, JavaFrame *,
                                       int, JavaOop *) {}

  static Bytecodes::Code get_verifier_breakpoint_opcode(const Method *, int) {
    return Bytecodes::_breakpoint;
  }
  static void replace_event_opcode(Method *, Bytecodes::Code, int) {}
#endif
};

#if ENABLE_JAVA_DEBUGGER


class VMEventStream {
public:
  VMEvent _event;
  bool _end_events;

  VMEventStream() {
    _event = Universe::vmevent_request_head();
    if (_event.is_null()) {
      _end_events = true;
    } else {
      _end_events = false;
    }
  }

  ReturnOop next() {
    VMEvent::Raw e;
    e = _event;
    _event = _event.next();
    if (_event.is_null()) {
      _end_events = true;
    }
    return e;
  }

  ReturnOop next_by_task(int task_id) {
    while(!_end_events) {
      VMEvent::Raw e = next();
      if ((task_id == -1 || e().task_id() == task_id)) {
        return e;
      }
    }
    return NULL;
  }

  ReturnOop next_by_kind(jbyte kind, int task_id = -1) {
    while(!_end_events) {
      VMEvent::Raw e = next();
      if (e().event_kind() == kind &&
          (task_id == -1 || e().task_id() == task_id)) {
        return e;
      }
    }
    return NULL;
  }
  bool at_end() {
    return _end_events;
  }
};
#else

#endif
