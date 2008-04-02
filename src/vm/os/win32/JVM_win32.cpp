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
 * JVM_win32.cpp: Win32-specific VM startup and
 *                  shutdown routines.
 *
 * This file provides Win32-specific virtual machine
 * startup and shutdown routines.  Refer to file
 * "/src/vm/share/runtime/JVM.hpp" and the Porting
 * Guide for details.
 */

#include "incls/_precompiled.incl"
#include "incls/_JVM_win32.cpp.incl"

#ifdef CYGWIN
#define __try
#define __except(e)
// IMPL_NOTE: consider whether it is needed here 
#ifndef SIGBREAK
#define SIGBREAK 21
#endif
#endif

#ifndef PRODUCT

#include <signal.h>

static bool printing_stack = false;

// Type Ctrl-break (there's a "break" key on your PC keyboard)
// to dump stack trace of all Java threads
static void SigBreakHandler(int junk) {
  if (!printing_stack) {
    UNIMPLEMENTED();
    printing_stack = 1;
    pss();
    printing_stack = 0;
  }
  signal(SIGBREAK, SigBreakHandler);
}

#define EXCEPTION_CASE(c) case c: name = #c;
#define PRINT_REGISTER(x) \
    printf("Register %6s = 0x%8x\n", #x, exptr->ContextRecord->x)

#endif // !PRODUCT

extern "C" void interpreter_timer_tick();
extern "C" void compiler_timer_tick();

int CheckException(int n_except, LPEXCEPTION_POINTERS exptr) {
#if ENABLE_PAGE_PROTECTION
  if (n_except == STATUS_ACCESS_VIOLATION) {
    const juint offset =
      (address)exptr->ExceptionRecord->ExceptionInformation[1]-_protected_page;
    if (TracePageAccess) {
      TTY_TRACE_CR(("ACCESS_VIOLATION signaled: offset = %d", offset));
    }
    switch (offset) {
    case COMPILER_TIMER_TICK_SLOT:
      exptr->ContextRecord->Eip = (DWORD)compiler_timer_tick;
      return EXCEPTION_CONTINUE_EXECUTION;
    case INTERPRETER_TIMER_TICK_SLOT:
      exptr->ContextRecord->Eip = (DWORD)interpreter_timer_tick;
      return EXCEPTION_CONTINUE_EXECUTION;
    }
  }
#endif

#ifndef PRODUCT
  // If we have error inside a pp() call, last_raw_handle, etc, need to be
  // restored.
  DebugHandleMarker::restore();

  char * name = "unknown";
  switch (n_except) {
    EXCEPTION_CASE(STATUS_ACCESS_VIOLATION)               break;
    EXCEPTION_CASE(STATUS_BREAKPOINT)                     break;
    EXCEPTION_CASE(STATUS_DATATYPE_MISALIGNMENT)          break;
    EXCEPTION_CASE(STATUS_ILLEGAL_INSTRUCTION)            break;
    EXCEPTION_CASE(STATUS_PRIVILEGED_INSTRUCTION)         break;
    EXCEPTION_CASE(STATUS_INTEGER_DIVIDE_BY_ZERO)         break;
    EXCEPTION_CASE(STATUS_INTEGER_OVERFLOW)               break;
    EXCEPTION_CASE(STATUS_SINGLE_STEP)                    break;
  }
  tty->print_cr("** Unhandled exception 0x%x (%s) at 0x%x **",
                n_except, name, exptr->ExceptionRecord->ExceptionAddress);

  if (n_except ==  STATUS_ACCESS_VIOLATION) {
    tty->print_cr("access violation address = 0x%x",
                  exptr->ExceptionRecord->ExceptionInformation[1]);
  }

  PRINT_REGISTER(SegCs);
  PRINT_REGISTER(SegEs);
  PRINT_REGISTER(SegDs);
  PRINT_REGISTER(SegGs);
  PRINT_REGISTER(SegFs);
  PRINT_REGISTER(SegSs);
  PRINT_REGISTER(Eax);
  PRINT_REGISTER(Ebx);
  PRINT_REGISTER(Ecx);
  PRINT_REGISTER(Edx);
  PRINT_REGISTER(Edi);
  PRINT_REGISTER(Esi);
  PRINT_REGISTER(Ebp);
  PRINT_REGISTER(Esp);
  PRINT_REGISTER(Eip);
  PRINT_REGISTER(EFlags);

  if (!printing_stack && PrintStackTraceOnCrash) {
    printing_stack = 1;
    __try {
      pss();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {;}
  }

  if (ExitOnCrash) {
    printf("Exiting from crash ...\n");
    ::exit(99);
  }
#endif

  return EXCEPTION_CONTINUE_SEARCH;
}

static int executeVM( void ) {
#ifndef PRODUCT
  signal(SIGBREAK, SigBreakHandler);
#endif

  if (SlaveMode) {
    // IMPL_NOTE: ExecutionLoops not supported in SlaveMode
    ExecutionLoops = 1;
  }
  
  int result = 0;
  for (int i = 0; i < ExecutionLoops; i++) {
    if (Verbose) {
      tty->print_cr("\t***Starting VM***");
    }
    __try {
      result = JVM::start();
    }
    __except (CheckException(GetExceptionCode(), GetExceptionInformation())) {
      ;
    }
  }
  Arguments::finalize();
  return result;
}

extern "C" int JVM_Start(const JvmPathChar *classpath, char *main_class,
                         int argc, char **argv) {
  JVM::set_arguments(classpath, main_class, argc, argv);
  return executeVM();
}

extern "C" int JVM_Start2(const JvmPathChar *classpath, char *main_class,
                          int argc, jchar **u_argv) {
  JVM::set_arguments2(classpath, main_class, argc, NULL, u_argv, true);
  return executeVM();
}
