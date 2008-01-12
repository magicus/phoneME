/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
# include "incls/_BinaryAssemblerCommon.cpp.incl"

#if ENABLE_COMPILER

void BinaryAssemblerCommon::ensure_compiled_method_space( int delta ) {
  delta += 256;
  if (!has_room_for(delta)) {
    delta = align_allocation_size(delta + (1024 - 256));
    if( compiled_method()->expand_compiled_code_space(delta,
                                                      relocation_size())) {
      _relocation.move(delta);
    }
  }
}

#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
void BinaryAssemblerCommon::comment(const char* fmt, ...) {
  JVM_VSNPRINTF_TO_BUFFER(fmt, buffer, 1024);

  if (PrintCompiledCodeAsYouGo) {
    tty->print_cr(";; %s", buffer);
  } else if (GenerateCompilerComments) {
    _relocation.emit_comment(_code_offset, buffer);
  }
}
#endif // !defined(PRODUCT) || USE_COMPILER_COMMENTS

#endif
