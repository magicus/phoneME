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

/** @file JavaDebugger.hpp
    @brief Main interface class to the external debugger.

    The JavaDebugger class is used to interface with the external debug proxy.
    This file is responsible for reading commands from the underlying
    transport and calling the appropriate function via a function table.
    This class also manages the object <-> ID mapping that is used to map
    VM objects to debugger ID values.
*/

#if ENABLE_JAVA_DEBUGGER

typedef void (*command_handler)(PacketInputStream *, PacketOutputStream *);

#define DEBUGGER_EVENT(x)            \
  if (TraceDebugger) {               \
    tty->print("DEBUGGER EVENT: ");  \
    tty->print_cr x;                 \
  }

// Bit fields for the _debugger_active flag
#define DEBUGGER_ACTIVE 1
#define DEBUGGER_STEPPING 0x80000000

#endif

class JavaDebugger {
#if ENABLE_JAVA_DEBUGGER
public:

  /** Initialize the transport between VM and proxy.
      Usually called by the VM initialization code with 
      @param t pointing to the Transport.
   */
  static  bool sync_debugger(Transport *t);
  /** Close the underlying Transport.
   */
  static void close_java_debugger(Transport *t);
  /** Poll for a debugger command.
      Loops forever calling dispatch routine with a timeout of -1 (forever).
      Breaks out when someone calls JavaDebugger::set_loop_count().
   */
  static void process_command_loop();
  /** Main command dispatch function.
      Called from various points in the code to ensure that we can always
      get a command from the Transport.  Called with a timeout value that
      is passed to the underlying Transport.
      @param timeout - if -1 then wait forever, if 0 then just poll.  
   */
  static bool dispatch(int timeout);

  /** Set current Transport to t
      @param t
   */

  static bool initialize_java_debugger_main(JVM_SINGLE_ARG_TRAPS);
  static void initialize_java_debugger_task(JVM_SINGLE_ARG_TRAPS);
  static bool initialize_java_debugger(JVM_SINGLE_ARG_TRAPS);

  static void set_debugger_option_on(bool is_on)
  {
    // arguments passed in to VM to enable debugger
    _debugger_option_on = is_on;
  }
  static void set_debug_isolate_option_on(bool is_on)
  {
    // arguments passed in to VM to enable debugger
    _debug_isolate_option_on = is_on;
  }
  static void set_debug_main_option_on(bool is_on)
  {
    // arguments passed in to VM to enable debugger
    _debug_main_option_on = is_on;
  }
  static bool is_debugger_option_on()
  {
    return _debugger_option_on;
  }
  static bool is_debug_isolate_option_on()
  {
    return _debug_isolate_option_on;
  }
  static bool is_debug_main_option_on()
  {
    return _debug_main_option_on;
  }
  static void set_suspend(bool state) {
    _suspend_option = state;
  }
  static bool is_suspend() {
    return _suspend_option;
  }
  static void set_stepping(bool is_stepping) {
    if (is_stepping) {
      _debugger_active |= DEBUGGER_STEPPING;
    } else {
      _debugger_active &= ~DEBUGGER_STEPPING;
    }
  }

private:
  static jboolean _resume_ok;
  static bool _suspend_option;

  // object ID functions
public:
  static int next_seq_num();
  static void release_id(int);
  static ReturnOop get_object_by_id(int objectID);
  static int get_object_id_by_ref(Oop *p);
  static ReturnOop get_thread_by_id(int objectID);
  static int get_thread_id_by_ref(Thread *t);
  static int get_object_id_by_ref_nocreate(Oop *p);
  static int get_method_index(const Method *);
  static jlong get_method_id(const Method *);
  static jlong get_method_id(InstanceClass *, const Method *);
  static ReturnOop get_method_by_id(InstanceClass *, jlong);
private:
  static int _next_seq_num;
  static int object_hash_code(Oop *);
  static int hash_id(jint);
  static ReturnOop compress_chain(RefNode *node, bool by_ref);
  // conversions to JDWP types of data
public:
  static jbyte get_jdwp_tagtype(JavaClass *);
  static jint get_jdwp_class_status(JavaClass *);
  static jbyte get_tag_from_type(jint type);
  static jbyte get_jdwp_tag(Oop *p);

  static bool is_valid_thread(Thread *t) {
    return (t->not_null());
  }

  static void process_suspend_policy(jbyte policy, Thread *thread, 
                                     jboolean forceWait) {
    process_suspend_policy(policy, thread, -1, forceWait);
  }
  static void process_suspend_policy(jbyte policy, int task_id, 
                                     jboolean forceWait) {
    process_suspend_policy(policy, NULL, task_id, forceWait);
  }

  // commands handled in JavaDebugger
  static void nop(COMMAND_ARGS);
  static void vendor_hand_shake(COMMAND_ARGS);
  static void vendor_get_stepping_info(COMMAND_ARGS);
  static void get_line_num_table(COMMAND_ARGS);
  static void get_local_var_table(COMMAND_ARGS);
private:
  static void *arraytype_cmds[];
  static void *field_cmds[];
  static void *method_cmds[];
  static void *threadgroupreference_cmds[];
  static void *event_request_cmds[];
  static void *classloader_reference_cmds[];
  static void *vendor_commands[];
  static void **func_array[];

  // misc support routines
public:
  static void send_all_class_prepares();
  static char *unicode2utf(TypeArray *, char *, int);
  static int unicode2utfstrlen(TypeArray *);
  static void set_loop_count(int);
  static int get_loop_count();
#ifdef AZZERT
  static bool is_legal_offset(Method *m, jlong offset);
  static void print_class(JavaClass *ic);
  static void print_field(Field *f);
#endif

private:
  static int _loop_count;
  static bool _debugger_option_on;
  static bool _debug_isolate_option_on;
  static bool _debug_main_option_on;

  static void process_suspend_policy(jbyte policy, Thread *thread, 
                                     int task_id, jboolean forceWait);
  // GC support
public:
  static void clear_and_release_map_entry(OopDesc**, RefNodeDesc *, int);
  static void flush_refnodes();
  static void rehash();

  enum {
    HASH_SLOT_SIZE = 127
  };
#else
public:
  static void set_stepping(bool /*is_stepping*/) {}
  static void dispatch(int /*timeout*/) {}
  static bool is_debugger_option_on()
  {
    return false;
  }

#endif
};
