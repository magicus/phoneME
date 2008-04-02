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
# include "incls/_Buffer.cpp.incl"

#if NOT_CURRENTLY_USED

HANDLE_IMPLEMENTATION_WITH_DISPOSING_FAST(Buffer)

ReturnOop Buffer::allocate(unsigned int length, 
                           JvmAllocProc alloc_proc JVM_TRAPS) {
  if (alloc_proc != NULL) {
    OopDesc* oop_desc = (OopDesc*)alloc_proc(sizeof(ArrayDesc) +
                                             length * sizeof(jubyte));
    if (oop_desc == NULL) {
      Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
    }
    // Must zero-inititialize the allocated block to match 
    // ObjectHeap::allocate behavior.
    jvm_memset((char*)oop_desc + sizeof(ArrayDesc), 0, 
               length * sizeof(jubyte));
    *oop_desc->int_field_addr(Array::length_offset()) = length;
    return oop_desc;
  } else {
    return Universe::new_byte_array(length JVM_CHECK_0);
  }
}

#endif
