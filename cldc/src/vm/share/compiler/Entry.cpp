/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
# include "incls/_Entry.cpp.incl"

#if ENABLE_COMPILER

ReturnOop Entry::allocate(jint bci, VirtualStackFrame* frame, 
                          BinaryAssembler::Label& label, jint code_size 
                          JVM_TRAPS) {
  UsingFastOops fast_oops;
  // Clone the virtual stack frame by default.
  VirtualStackFrame::Fast frame_clone = frame->clone(JVM_SINGLE_ARG_CHECK_0);

  // Allocate the new entry.
  Entry::Fast entry = Universe::new_mixed_oop_in_compiler_area(
                                              MixedOopDesc::Type_Entry,
                                              EntryDesc::allocation_size(),
                                              EntryDesc::pointer_count()
                                              JVM_OZCHECK(entry));
  // Fill out instance fields.
  entry().set_bci(bci);
  entry().set_frame(&frame_clone);
  entry().set_label(label);
  entry().set_code_size(code_size);

  // Return the entry.
  return entry;
}

#endif
