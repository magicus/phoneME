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
# include "incls/_CompiledMethodDesc_i386.cpp.incl"

#if ENABLE_COMPILER

void CompiledMethodDesc::update_relative_offsets(int delta) {
  // Create a fake handle (only used during GC).
  // Note that this temporary handle will not be visited by GC.
  CompiledMethodDesc* cm_desc = this;
  CompiledMethod* cm = (CompiledMethod*) &cm_desc;
  // Iterate over all relative offsets to static code in generated code
  for (RelocationReader stream(cm); !stream.at_end(); stream.advance()) {
    if (stream.is_compiler_stub()) {
      // Relocate relative jump/call to static code.
      int* offset = (int*)(cm->entry() + stream.code_offset());
#ifdef AZZERT
      OopDesc* target =
          (OopDesc*)(*offset + (stream.code_offset() + (int)cm->entry()));
      GUARANTEE(!ObjectHeap::contains_moveable(target), "Insanity check");
#endif
      *offset = *offset - delta;
    }
  }
}
#endif

#if USE_COMPILER_STRUCTURES
int CompiledMethodDesc::get_cache_index ( void ) const {
  GUARANTEE( *(const unsigned short*) (this+1) == 0x05C6,
    "compiled method code must start with method_execution_sensor update" );
  return ( *(const int*) ( ((unsigned char*)(this+1))+2 ) ) - (int) _method_execution_sensor;
}

void CompiledMethodDesc::set_cache_index ( const int i ) {
  GUARANTEE(i>=0 && i<0xffff, "cache sanity");
  GUARANTEE( *(const unsigned short*) (this+1) == 0x05C6,
    "compiled method code must start with method_execution_sensor update" );
  *(int*)( ((unsigned char*)(this+1))+2 ) = (int)(_method_execution_sensor+i);
}

#endif
