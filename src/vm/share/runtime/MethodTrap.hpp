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

#if ENABLE_METHOD_TRAPS

const int max_traps = 16;
// Distance (in bytes) between consecutive 'cautious_invoke' entries.
// See caution_invoke implementation.
const int trap_entry_offset = 16;

// JVM trap actions, copied from com.sun.cldchi.jvm.MethodTrap
enum TrapAction {
  ACTION_CALLBACK     =  0,
  ACTION_EXIT         = -1,
  ACTION_STOP_ISOLATE = -2,
  ACTION_BREAKPOINT   = -3
};

class MethodTrapDesc;

extern "C" {
  extern MethodTrapDesc _method_trap[max_traps];

  // Trap entry (native ASM function)
  void cautious_invoke();
  // Interrupt execution if call_count of specified method is reached
  void interrupt_or_invoke(MethodTrapDesc* trap);
}

// Trap descriptor
class MethodTrapDesc {
private:
  char* _method_name;
  MethodDesc* _trapped_method;
  MethodDesc* _handler_method;
  address _old_entry;
  int _call_count;
  int _action;
  int _target_task;

  // Given fully-qualified method name return the corresponding Method object
  static ReturnOop parse_method_name(char* full_name JVM_TRAPS);

public:
  static const int handler_method_offset() {
    return FIELD_OFFSET(MethodTrapDesc, _handler_method);
  }

  static const int old_entry_offset() {
    return FIELD_OFFSET(MethodTrapDesc, _old_entry);
  }

  void set_method_name(char* value) {
    _method_name = value;
  }

  address old_entry() {
    return _old_entry;
  }

  void set_old_entry(address value) {
    _old_entry = value;
  }

  void set_parameters(int call_count, int action, int target_task) {
    _call_count = call_count;
    _action = action;
    _target_task = target_task;
  }
  
  bool call_count_reached() {
    return --_call_count <= 0;
  }

  bool action() {
    return _action;
  }

  bool for_this_task() {
#if ENABLE_ISOLATES
    return _target_task == 0 || _target_task == TaskContext::current_task_id();
#else
    return true;
#endif
  }

  bool is_free() {
    return _trapped_method == NULL;
  }

  int index() {
    return ((address)this - (address)_method_trap) / sizeof(MethodTrapDesc);
  }

  int apply(char* trapped_name, char* handler_name JVM_TRAPS);
  void release();

  friend class MethodTrap;
};

class MethodTrap : public AllStatic {
public:
  // Initialize buffer used by all MethodTrapDesc structures
  static void initialize();
  // Release memory used by all MethodTrapDesc structures
  static void dispose();
  // Set up traps passed via JVM_ParseOneArg()
  static void activate_initial_traps();
  // GC support - needed to update pointers in _method_trap[i]
  static void oops_do(void do_oop(OopDesc**));
  // Find free MethodTrapDesc structure and return pointer to it
  static MethodTrapDesc* find_slot();
  // Convert Java String object to null-terminated array of chars
  static void to_c_string(char* dest, OopDesc* java_string);
};

#endif
