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
# include "incls/_ThreadObj.cpp.incl"

HANDLE_CHECK(ThreadObj, !is_jvm_thread())

bool ThreadObj::is_unstarted() {
  Thread::Raw t = thread();
  return t().is_null() && !is_terminated();
}

bool ThreadObj::is_alive() {
  Thread::Raw t = thread();

  if (TraceThreadEvents) {
    TTY_TRACE_CR(("ThreadObj 0x%x has Thread 0x%x, status %s", 
                  obj(), t.obj(), is_terminated() ? "TERMINATED" : "ALIVE"));
  }

  return !t().is_null() && !is_stillborn() && !is_terminated();
}

#ifndef PRODUCT

void ThreadObj::print_value_on(Stream* st) {
  st->print("Thread object for ");
  if (is_unstarted()) {
    tty->print("unstarted thread");
  } else if (is_stillborn()) {
    tty->print("stillborn thread");
  } else if (is_terminated()) {
    tty->print("terminated thread");
  } else {
    Thread t = thread();
    t.print_value_on(st);
  }
}

void ThreadObj::verify_fields() {
  UsingFastOops fast_oops;
  InstanceClass::Fast ic = Universe::thread_class();
  ic().verify_instance_field("priority",      "I",
                              priority_offset());
  ic().verify_instance_field("vm_thread",     "Ljava/lang/Object;",
                              thread_offset());
  ic().verify_instance_field("is_terminated", "I",
                              is_terminated_offset());
  ic().verify_instance_field("is_stillborn",  "I",
                              is_stillborn_offset());
#if ENABLE_CLDC_11
  ic().verify_instance_field("name",          "[C",
                              name_offset());
#endif
}

#endif
