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

#if ENABLE_COMPILER

#include "incls/_CompilationQueue_thumb.cpp.incl"

void OSRStub::emit_osr_entry_and_callinfo(CodeGenerator *gen JVM_TRAPS) {
#if ENABLE_EMBEDDED_CALLINFO
  // Make sure the callinfo is on a word boundary
  if ((gen->code_size() & 0x3) != 0) {
    gen->nop();
  }
#else
  // Callinfo record is bound to the beginning of the OSR stub.
  // If the previous element of the compilation queue has callinfo record
  // bound to the last instruction, these two records will conflict as
  // they are bound to the same code offset.
  NOT_PRODUCT(gen->comment("Padding to separate callinfo records"));
  gen->nop();
#endif   
  gen->write_call_info(0 JVM_CHECK);
  gen->emit_osr_entry(bci());
}

#endif
