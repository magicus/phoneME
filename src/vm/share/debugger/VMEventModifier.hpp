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

/** \class VMEventModifier
    Support for event modifiers.  Event modifiers are used by VMEvent to
    hold information about each event such as which class, method and offset
    to put a breakpoint.  Other modifiers are used for exception events, class
    matching and single stepping
*/

#if ENABLE_JAVA_DEBUGGER

class DebuggerEvent {

  /* A DebuggerEvent is much like a VMEventModifier except it is transient.
   * Once we send the event to the debugger it is not needed.
   * 
   */
public:
  DebuggerEvent() : _offset(0), _thread_id(0), _clazz_id(0), _method_id(0),
  _event_kind(0) {
  }

  DebuggerEvent(jbyte kind, int thread_id, int clazz_id, jlong method_id,
                jlong offset);

  int thread_id() {
    return _thread_id;
  }

  void set_thread_id(int thread_id) {
    _thread_id = thread_id;
  }

  int clazz_id() {
    return _clazz_id;
  }

  void set_clazz_id(int clazz_id) {
    _clazz_id = clazz_id;
  }

  jlong method_id() {
    return _method_id;
  }

  void set_method_id(jlong method_id) {
    _method_id = method_id;
  }

  jlong offset() {
    return _offset;
  }

  void set_offset(jlong offset) {
    _offset = offset;
  }

  jbyte event_kind() {
    return _event_kind;
  }

  void set_event_kind(jbyte kind) {
    _event_kind = kind;
  }

  void set_sig_caught(jboolean val) {
    _sig_caught = val;
  }

  void set_sig_uncaught(jboolean val) {
    _sig_uncaught = val;
  }

  jboolean sig_caught() {
    return _sig_caught;
  }

  jboolean sig_uncaught() {
    return _sig_uncaught;
  }

  int step_size() {
    return _step_size;
  }

  void set_step_size(int size) {
    _step_size = size;
  }

  int step_depth() {
    return _step_depth;
  }

  void set_step_depth(int depth) {
    _step_depth = depth;
  }
  void write_as_location(PacketOutputStream *out);

#if ENABLE_ISOLATES
  void set_task_id(int id) {
    _task_id = id;
  }
#endif

private:
  jlong         _offset;
  jint          _thread_id;
  jint          _clazz_id;
  jlong         _method_id;
  jint          _step_size;
  jint          _step_depth;
#if ENABLE_ISOLATES
  jint          _task_id;
#endif
  jbyte         _event_kind;
  jboolean      _sig_caught;
  jboolean      _sig_uncaught;

};

class VMEventModifier : public MixedOop {

public:

  HANDLE_DEFINITION(VMEventModifier, MixedOop);

  jboolean thread_match(DebuggerEvent *);
  jboolean match(DebuggerEvent *, bool*);
  static ReturnOop allocate_modifier();
  static ReturnOop new_modifier(PacketInputStream *, PacketOutputStream *,
                                bool& error);

  ReturnOop next() {
    return obj_field(next_offset());
  }
  void set_next(Oop *p) {
    obj_field_put(next_offset(), p);
  }
  jbyte mod_kind() {
    return byte_field(mod_kind_offset());
  }
  void set_mod_kind(jbyte val) {
    byte_field_put(mod_kind_offset(), val);
  }
  int thread_id() {
    return int_field(thread_id_offset());
  }
  void set_thread_id(int val) {
    int_field_put(thread_id_offset(), val);
  }
  int clazz_id() {
    return int_field(clazz_id_offset());
  }
  void set_clazz_id(int val) {
    int_field_put(clazz_id_offset(), val);
  }
  int event_count() {
    return int_field(count_offset());
  }
  void set_count(int val) {
    int_field_put(count_offset(), val);
  }
  jboolean compile_state() {
    return bool_field(compile_state_offset());
  }
  void set_compile_state(jboolean val) {
    bool_field_put(compile_state_offset(), val);
  }

  address saved_method_entry() {
    return (address)int_field(saved_method_entry_offset());
  }
  void set_saved_method_entry(address val) {
    int_field_put(saved_method_entry_offset(), (int)val);
  }

  static void deoptimize_frame(Thread *, bool bump_bci = false);

  void deoptimize_method(Method *m);

protected:

  static int next_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _next);
  }
  static int mod_kind_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _mod_kind);
  }
  static int thread_id_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _location_mod._thread_id);
  }
  static int clazz_id_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _location_mod._clazz_id);
  }
  static int count_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _event_count);
  }
  static int method_id_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _location_mod._method_id);
  }
  static int offset_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _location_mod._offset);
  }
  static int save_opcode_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _location_mod._opcode);
  }
  static int sig_caught_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _exception_mod._sig_caught);
  }
  static int sig_uncaught_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _exception_mod._sig_uncaught);
  }
  static int class_name_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _class_name);
  }
  static int step_target_clazz_id_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _singlestep_mod._step_target._clazz_id);
  }
  static int step_target_method_id_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _singlestep_mod._step_target._method_id);
  }
  static int step_target_offset_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _singlestep_mod._step_target._offset);
  }
  static int step_size_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _singlestep_mod._step_size);
  }
  static int step_depth_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _singlestep_mod._step_depth);
  }
  static int step_starting_offset_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _singlestep_mod._step_starting_offset);
  }
  static int dup_current_line_offset_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _singlestep_mod._dup_current_line_offset);
  }
  static int post_dup_line_offset_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _singlestep_mod._post_dup_line_offset);
  }
  static int step_starting_fp_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _singlestep_mod._step_starting_fp);
  }
  static int compile_state_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _compile_state);
  }

  static int rom_debug_method_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _rom_debug_method);
  }

  static int saved_method_entry_offset() {
    return FIELD_OFFSET(VMEventModifierDesc, _saved_method_entry);
  }
};

class LocationModifier : public VMEventModifier {

public:
  HANDLE_DEFINITION(LocationModifier, VMEventModifier);

  static ReturnOop new_location(JavaFrame *fr, LocationModifier *loc = NULL);
  static ReturnOop new_location(InstanceClass *, Method *, jlong offset,
                                LocationModifier *loc = NULL);
  ReturnOop method();

  void write(PacketOutputStream *out, jboolean write_tag = true);
  static void write_null_location(PacketOutputStream *out);
  jboolean set_method_opcode(Bytecodes::Code, bool is_setting);

  jboolean match(DebuggerEvent *d_event);

  jlong method_id() {
    return long_field(method_id_offset());
  }
  void set_method_id(jlong val) {
    long_field_put(method_id_offset(), val);
  }
  jlong offset() {
    return long_field(offset_offset());
  }
  void set_offset(jlong val) {
    long_field_put(offset_offset(), val);
  }
  Bytecodes::Code save_opcode() {
    return (Bytecodes::Code)ubyte_field(save_opcode_offset());
  }
  void set_save_opcode(Bytecodes::Code opcode) {
    byte_field_put(save_opcode_offset(), (jbyte)opcode);
  }

  ReturnOop rom_debug_method() {
    return  obj_field(rom_debug_method_offset());
  }
  void set_rom_debug_method(Oop *value) {
    obj_field_put(rom_debug_method_offset(), value);
  }

  void unlink_method();

  void find_and_set_rom_method_id(Method *rom_method);
  ReturnOop get_dup_rom_method(Method *);

  void save_method_entry();
};

class ExceptionModifier : public LocationModifier {

public:
  HANDLE_DEFINITION(ExceptionModifier, LocationModifier);

  jboolean sig_caught() {
    return bool_field(sig_caught_offset());
  }
  jboolean sig_uncaught() {
    return bool_field(sig_uncaught_offset());
  }
  void set_sig_caught(jboolean val) {
    bool_field_put(sig_caught_offset(), val);
  }
  void set_sig_uncaught(jboolean val) {
    bool_field_put(sig_uncaught_offset(), val);
  }
};

class ClassMatchModifier : public VMEventModifier {

public:
  HANDLE_DEFINITION(ClassMatchModifier, VMEventModifier);

  jboolean class_only(DebuggerEvent *);
  jboolean class_match(DebuggerEvent *);

  void set_class_match_name(Oop *name){
    obj_field_put(class_name_offset(), name);
  }
  ReturnOop class_match_name() {
    return obj_field(class_name_offset());
  }

};

class StepModifier : public LocationModifier {

public:
  HANDLE_DEFINITION(StepModifier, LocationModifier);

  jboolean match(DebuggerEvent *smp);
  static jboolean find_frame(address starting_fp, JavaFrame *);

  int step_target_clazz_id() {
    return int_field(step_target_clazz_id_offset());
  }
  jlong step_target_method_id() {
    return long_field(step_target_method_id_offset());
  }
  julong step_target_offset() {
    return ulong_field(step_target_offset_offset());
  }
  void set_step_target_clazz_id(int val) {
    int_field_put(step_target_clazz_id_offset(), val);
  }
  void set_step_target_method_id(jlong val) {
    long_field_put(step_target_method_id_offset(), val);
  }
  void set_step_target_offset(julong val) {
    ulong_field_put(step_target_offset_offset(), val);
  }
  int step_size() {
    return int_field(step_size_offset());
  }
  int step_depth() {
    return int_field(step_depth_offset());
  }
  void set_step_size(int val) {
    int_field_put(step_size_offset(), val);
  }
  void set_step_depth(int val) {
    int_field_put(step_depth_offset(), val);
  }
  julong dup_current_line_offset() {
    return ulong_field(dup_current_line_offset_offset());
  }
  void set_dup_current_line_offset(julong val) {
    ulong_field_put(dup_current_line_offset_offset(), val);
  }
  julong post_dup_line_offset() {
    return ulong_field(post_dup_line_offset_offset());
  }
  void set_post_dup_line_offset(julong val) {
    ulong_field_put(post_dup_line_offset_offset(), val);
  }
  julong step_starting_offset() {
    return ulong_field(step_starting_offset_offset());
  }
  void set_step_starting_offset(julong val) {
    ulong_field_put(step_starting_offset_offset(), val);
  }
  address step_starting_fp() {
    return (address)int_field(step_starting_fp_offset());
  }
  void set_step_starting_fp(int val) {
    int_field_put(step_starting_fp_offset(), (int)val);
  }

};
#endif
