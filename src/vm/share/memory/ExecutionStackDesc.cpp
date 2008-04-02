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
# include "incls/_ExecutionStackDesc.cpp.incl"

ExecutionStackDesc* ExecutionStackDesc::_stack_list;  // used only by GC;

void ExecutionStackDesc::relocate_all_pointers(int delta,
                                               void updater(OopDesc**)) {
  OopDesc*    thread = _thread;
  Thread*     thread_handle = (Thread*)&thread;
  this->variable_oops_do(updater); 
  // Call relocate_internal_pointers, letting it know about the value of
  // thread >>before<< it was updated
  this->relocate_internal_pointers(delta, thread_handle);
}

// This method is called before compaction: <this> is still at the old
// location, but we should update all internal pointers to point to
// the new locations of their destinations.
void ExecutionStackDesc::relocate_internal_pointers(int delta,
                                                    Thread* thread, 
                                                    bool do_locks) {
  // _thread == NULL means that this is an execution stack that has been
  // discarded because we've replaced it with a larger stack.  No one should
  // point to an execution stack except for its thread.
  GUARANTEE(_thread != NULL, "Should not relocate dead execution stack!");

  if (thread->last_java_frame_exists()) {
    // Iterate through all frames on this stack.
    Frame fr(thread);
    while (true) {
      if (fr.is_entry_frame()) {
        EntryFrame copy(fr.as_EntryFrame());
        // Get caller before relocating internal pointers.
        bool is_first_frame = copy.is_first_frame();
        if (!is_first_frame) {
          copy.caller_is(fr);
        }
        copy.relocate_internal_pointers(delta);
        if (is_first_frame) {
          break;
        }
      } else {
        JavaFrame copy(fr.as_JavaFrame());
        // Get caller before relocating internal pointers.
        copy.caller_is(fr);
        copy.relocate_internal_pointers(delta, do_locks);
      }
    }

    JavaFrame::relocate_starting_frame_pointers(thread, delta);

    // Relocate this stack's thread's last_java_sp and last_java_fp
    thread->set_last_java_sp(thread->last_java_sp() + delta);
    thread->set_last_java_fp(thread->last_java_fp() + delta);
  }

  // Relocate this stack's thread's saved frame pointer,
  // stack_pointer, and stack_limit.
  jint* stored_fp_addr = (jint*)thread->stack_pointer();
  if (*stored_fp_addr != 0) {
    *stored_fp_addr = *stored_fp_addr + delta;
  }
  thread->set_stack_pointer(thread->stack_pointer() + delta);
  thread->set_stack_limit(thread->stack_limit() + delta);

  if (contains(_kni_parameter_base)) {
    _kni_parameter_base += delta;
  }

  // That should do it. All the other stuff on the stack is taken care
  // of in Thread::oops_do().
}

// this is method of the great fun: we update list of thread stacks 
// with value in old_stack, by replacing pointers to old_stack
// with pointers to new_stack and also fixing new_stack->_next_stack
// to actually point where it should point to
void ExecutionStackDesc::update_list(ExecutionStackDesc* old_stack,
				     ExecutionStackDesc* old_stack_next,
				     ExecutionStackDesc* new_stack) {
  ExecutionStackDesc *this_stack = _stack_list;

  // if replacing list head - give it a special treatment
  if (old_stack == _stack_list) {
    _stack_list = new_stack;
    new_stack->_next_stack = old_stack_next;
    return;
  }
  
  // find element just before one we want to replace and patch here
  while (this_stack != NULL) {
    ExecutionStackDesc *next_stack = this_stack->_next_stack;

    if (next_stack == old_stack) {      
      this_stack->_next_stack = new_stack;
      new_stack->_next_stack = old_stack_next;
      break;
    }
    this_stack = next_stack;
  }
  
  GUARANTEE(new_stack->_next_stack == old_stack_next, "must be updated");
}
