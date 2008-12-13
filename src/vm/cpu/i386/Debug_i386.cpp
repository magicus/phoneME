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
# include "incls/_Debug_i386.cpp.incl"

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

void ps() {
  Thread* current = Thread::current();

  if (current == NULL) {
    tty->print_cr("cannot print stack (no current thread)");
  } else if (Frame::in_gc_state()) {
    psgc();
  } else if (current->last_java_frame_exists()) {
    current->trace_stack(tty);
  } else if (ObjectHeap::contains((OopDesc*)&current)) {
    psg((address)&current);
  } else {
    tty->print_cr("cannot print stack (not executing in VM)");
  }
}

#endif
