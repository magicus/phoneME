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
#include "incls/_MethodTrap.cpp.incl"

#if ENABLE_METHOD_TRAPS

extern "C" {
  MethodTrapDesc _method_trap[max_traps];

  void interrupt_or_invoke(MethodTrapDesc* trap) {
    SETUP_ERROR_CHECKER_ARG;

    if (trap->for_this_task() && trap->call_count_reached()) {
      int action = trap->action();
      switch (action) {
      case ACTION_STOP_ISOLATE:
#if ENABLE_ISOLATES
        {
          Task::current()->forward_stop(0, Task::OTHER_HALT JVM_CHECK);
          break;
        }
#endif
        // In SVM mode ACTION_STOP_ISOLATE is the same as ACTION_HALT
      case ACTION_EXIT:
        JVM_Stop(0);
        SHOULD_NOT_REACH_HERE();
      case ACTION_BREAKPOINT:
        BREAKPOINT;
        break;
      case ACTION_CALLBACK:
      default:
        JVMSPI_MethodTrap(action, trap->index());
      }
    }
  }
}

/*
 * Initialize memory used by MethodTrapDesc structures.
 * This function is called when the JVM process is intialized
 * (not when the VM is restarted inside the same process).
 */
void MethodTrap::initialize() {
  jvm_memset(_method_trap, 0, sizeof(_method_trap));
}

/*
 * Release memory used by MethodTrapDesc structures.
 * This is called whenever a VM instance quits
 * (but may be restarted again inside the same process).
 */
void MethodTrap::dispose() {
  for (int i = 0; i < max_traps; i++) {
    if (_method_trap[i]._method_name != NULL) {
      OsMemory_free(_method_trap[i]._method_name);
    }
  }
  jvm_memset(_method_trap, 0, sizeof(_method_trap));
}

/*
 * Called after JVM::start() is called just before main() method is runned
 */
void  MethodTrap::activate_initial_traps() {
  SETUP_ERROR_CHECKER_ARG;
  for (int i = 0; i < max_traps; i++) {
    MethodTrapDesc* mt = &_method_trap[i];
    if (mt->_method_name != NULL && mt->for_this_task()) {
      mt->apply(mt->_method_name, NULL JVM_NO_CHECK);
      OsMemory_free(mt->_method_name);
      mt->_method_name = NULL;
    }
  }
  // Ignore exceptions that might be thrown
  Thread::clear_current_pending_exception();
}

MethodTrapDesc* MethodTrap::find_slot() {
  for (int i = 0; i < max_traps; i++) {
    if (_method_trap[i].is_free() && _method_trap[i]._method_name == NULL) {
      return &_method_trap[i];
    }
  }
  return NULL;
}

void MethodTrap::oops_do(void do_oop(OopDesc**)) {
  for (int i = 0; i < max_traps; i++) {
    if (!_method_trap[i].is_free()) {
      if (ObjectHeap::contains(_method_trap[i]._trapped_method)) {
        do_oop((OopDesc**) &_method_trap[i]._trapped_method);
      }
      if (ObjectHeap::contains(_method_trap[i]._handler_method)) {
        do_oop((OopDesc**) &_method_trap[i]._handler_method);
      }
    }
  }
}

void MethodTrap::to_c_string(char* dest, OopDesc* java_string) {
  String::Raw str = java_string;
  int length = str.is_null() ? 0 : str().length();
  for (int i = 0; i < length; i++) {
    dest[i] = (char) str().char_at(i);
  }
  dest[length] = 0;
}

/*
 * NOTE: this function changes the source string
 */
ReturnOop MethodTrapDesc::parse_method_name(char* full_name JVM_TRAPS) {
  UsingFastOops fast_oops;

  // Parse signature part
  Symbol::Fast signature;
  char* p = jvm_strchr(full_name, '(');
  if (p != NULL) {
    signature = TypeSymbol::parse(p JVM_CHECK_0);
    *p = 0;
  }

  // Parse method name part
  p = jvm_strrchr(full_name, '.');
  if (p <= full_name) {
    Throw::throw_exception(Symbols::java_lang_IllegalArgumentException(),
                           invalid_method JVM_THROW_0);
  }
  *p++ = 0;
  Symbol::Fast method_name = SymbolTable::symbol_for(p JVM_CHECK_0);

  // Parse class name part
  InstanceClass::Fast ic = JVM::resolve_class(full_name JVM_CHECK_0);

  // Finally get the requested method
  Method::Fast result = ic().lookup_method(&method_name, &signature);
  if (result.is_null()) {
    Throw::throw_exception(Symbols::java_lang_IllegalArgumentException(),
                           invalid_method JVM_THROW_0);
  }
  return (MethodDesc*) result.obj();
}

int MethodTrapDesc::apply(char* trapped_name, char* handler_name JVM_TRAPS) {
  UsingFastOops fast_oops;
  Method::Fast target = parse_method_name(trapped_name JVM_CHECK_0);
  if (target().get_trap() != NULL) {
    Throw::throw_exception(Symbols::java_lang_IllegalArgumentException(),
                           trap_already_set JVM_THROW_0);
  }

  if (handler_name != NULL) {
    Method::Raw handler = parse_method_name(handler_name JVM_CHECK_0);
    if (handler().size_of_parameters() != target().size_of_parameters() ||
        handler().return_type() != target().return_type())
    {
      Throw::throw_exception(Symbols::java_lang_IllegalArgumentException(),
                             parameter_types_mismatch JVM_THROW_0);
    }
    _handler_method = (MethodDesc*) handler.obj();
  }
  _trapped_method = (MethodDesc*) target.obj();

  int id = index();
  address new_entry = (address)cautious_invoke + id * trap_entry_offset;
  set_old_entry(target().execution_entry());
  target().set_execution_entry(new_entry);
  return id;
}

void MethodTrapDesc::release() {
  Method::Raw m = _trapped_method;
  _trapped_method = NULL;
  _handler_method = NULL;
  m().set_execution_entry(_old_entry);
}

extern "C" {

jint Java_com_sun_cldchi_jvm_MethodTrap_setTrap(JVM_SINGLE_ARG_TRAPS) {
  MethodTrapDesc* mt = MethodTrap::find_slot();
  if (mt == NULL) {
    Throw::throw_exception(Symbols::java_lang_RuntimeException(),
                           no_more_free_slots JVM_THROW_0);   
  }

  char trapped_name[256];
  MethodTrap::to_c_string(trapped_name, GET_PARAMETER_AS_OOP(1));
  mt->set_parameters(KNI_GetParameterAsInt(2), /* call_count */
                     KNI_GetParameterAsInt(3), /* action */
                     KNI_GetParameterAsInt(4)  /* target_task */);
  return mt->apply(trapped_name, NULL JVM_NO_CHECK_AT_BOTTOM);
}

jint Java_com_sun_cldchi_jvm_MethodTrap_setJavaTrap(JVM_SINGLE_ARG_TRAPS) {
  MethodTrapDesc* mt = MethodTrap::find_slot();
  if (mt == NULL) {
    Throw::throw_exception(Symbols::java_lang_RuntimeException(),
                           no_more_free_slots JVM_THROW_0);   
  }

  char trapped_name[256], handler_name[256];
  MethodTrap::to_c_string(trapped_name, GET_PARAMETER_AS_OOP(1));
  MethodTrap::to_c_string(handler_name, GET_PARAMETER_AS_OOP(2));
  mt->set_parameters(0, 0, 0);
  return mt->apply(trapped_name, handler_name JVM_NO_CHECK_AT_BOTTOM);
}

void Java_com_sun_cldchi_jvm_MethodTrap_releaseTrap(JVM_SINGLE_ARG_TRAPS) {
  int index = KNI_GetParameterAsInt(1);
  if (index < 0 || index >= max_traps || _method_trap[index].is_free()) {
    Throw::throw_exception(Symbols::java_lang_IllegalArgumentException(),
                           invalid_trap_handle JVM_THROW);
  }
  _method_trap[index].release();
}

} // extern "C"

#endif
