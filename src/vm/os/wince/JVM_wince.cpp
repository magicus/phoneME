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

/*
 * JVM_wince.cpp: PocketPC-specific VM startup and 
 *                  shutdown routines.
 *
 * This file provides PocketPC-specific virtual machine
 * startup and shutdown routines.  Refer to file 
 * "/src/vm/share/runtime/JVM.hpp" and the Porting
 * Guide for details.
 */

#include "incls/_precompiled.incl"
#include "incls/_JVM_wince.cpp.incl"

extern "C" {
  // tells us what type of assembler loop we have
  // 0 = interpreter stub
  // 1 = debug
  // 2 = product
  extern int assembler_loop_type;
}

int _system_address = 0;

extern "C" int JVM_Start(const JvmPathChar *classpath, char *main_class, 
                         int argc, char **argv) {

#ifdef ARM
  GUARANTEE(assembler_loop_type > 0, "Bad interpreter loop");
#endif

#ifndef ARM
  // this is only when running under EVC debugger
  if (EVCDebug || 1) {
    _system_address = ((int)&argc) & 0xFF000000;
  }
#endif

  JVM::set_arguments(classpath, main_class, argc, argv);
  const int result = JVM::start();
  Arguments::finalize();
  return result;
}
