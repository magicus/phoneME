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
 * JVM_generic.cpp: Generic-specific VM startup and
 *                  shutdown routines.
 *
 * This file provides Generic-specific virtual machine
 * startup and shutdown routines.  Refer to file
 * "/src/vm/share/runtime/JVM.hpp" and the Porting
 * Guide for details.
 */

#include "incls/_precompiled.incl"
#include "incls/_JVM_generic.cpp.incl"

static int executeVM( void ) {
  const int result = JVM::start();
  Arguments::finalize();
  return result;
}

extern "C" int JVM_Start(const char *classpath, char *main_class, int argc,
                         char **argv) {
  JVM::set_arguments(classpath, main_class, argc, argv);
  return executeVM();
}

extern "C" int JVM_Start2(const char *classpath, char *main_class, int argc,
                          jchar **u_argv) {
  JVM::set_arguments2(classpath, main_class, argc, NULL, u_argv, true);
  return executeVM();
}
