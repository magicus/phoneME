/*
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

/*
 * OsMisc_win32.cpp:
 */

#include "incls/_precompiled.incl"
#include "incls/_OsMisc_win32.cpp.incl"

#ifdef __cplusplus
extern "C" {
#endif

static HANDLE thread_process;

const JvmPathChar *OsMisc_get_classpath() {
  // If classpath is allocated, just keep it during the entire lifetime
  // of the VM (even if VM is restarted). Environment variables won't be
  // changed during VM execution so classpath won't change.
  static JvmPathChar* classpath = NULL;
  if (classpath == NULL) {
    char *ascii;
    if ((ascii = getenv("CLASSPATH")) != NULL) {
      int len = strlen(ascii);
      classpath = (JvmPathChar*)malloc((len+1) * sizeof(JvmPathChar));
      for (int i=0; i<len; i++) {
        classpath[i] = (JvmPathChar)ascii[i];
      }
      classpath[len] = 0;
    }
  }

  return classpath;
}

void OsMisc_set_process(HANDLE process) {
  thread_process = process;
}

void OsMisc_flush_icache(address start, int size) {
  // Note: On a single-CPU x86 machine, this may not be required.
  GUARANTEE(start == NULL || size > 0, "illegal argument combination");
  FlushInstructionCache(thread_process, start, size);
}

#ifndef PRODUCT

const char *OsMisc_jlong_format_specifier() {
  return "%I64d";
}

const char *OsMisc_julong_format_specifier() {
  return "%I64u";
}

#endif // PRODUCT

#ifdef __cplusplus
}
#endif
