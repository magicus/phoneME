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
#include "incls/_Main_linux.cpp.incl"
#include <stdio.h>
#include <stdlib.h>

#if ENABLE_PCSL
extern "C" {
#include <pcsl_memory.h>
#include <pcsl_print.h>
}
#endif
/*
 * This file implements the launching of the stand-along VM
 * on the Linux platform.
 */

void JVMSPI_PrintRaw(const char* s) {
#if ENABLE_PCSL
  pcsl_print(s);
#else
  jvm_printf("%s", s);
  jvm_fflush(stdout);
#endif
}

void JVMSPI_Exit(int code) {
  ::jvm_exit(code);
}

#if ENABLE_DYNAMIC_RESTRICTED_PACKAGE
jboolean JVMSPI_IsRestrictedPackage(const char* pkg_name, int name_length) {
  GUARANTEE(UseROM, "sanity");

  // This is just a sample implementation. Please modify it for
  // an actual deployment.
  if (name_length == 8 && strncmp(pkg_name, "com/abcd", 8) == 0) {
    // Don't allow any class in the package of "com.abcd".
    return true;
  }
  if (name_length > 8  && strncmp(pkg_name, "com/abcd/", 9) == 0) {
    // Don't allow any class in a subpackage of "com.abcd".
    return true;
  }
  // Note that classes in an related package, such as com.abcdefg.MyClass, 
  // are allowed.
  return false;
}
#endif

int main(int argc, char **argv) {
  int code = 0;

#if ENABLE_PCSL
  pcsl_mem_initialize(NULL, 0);
#endif

  // Call this before any other Jvm_ functions.
  JVM_Initialize();

  // Ignore arg[0] -- the name of the program.
  argc --;
  argv ++;

  while (true) {
    int n = JVM_ParseOneArg(argc, argv);
    if (n < 0) {
      JVMSPI_DisplayUsage(NULL);
      code = -1;
      goto end;
    } else if (n == 0) {
      break;
    }
    argc -= n;
    argv += n;
  }

  if (JVM_GetConfig(JVM_CONFIG_SLAVE_MODE) == KNI_FALSE) {
    // Run the VM in regular mode -- JVM_Start won't return until
    // the VM completes execution.
    code = JVM_Start(NULL, NULL, argc, argv);
  } else {
    JVM_Start(NULL, NULL, argc, argv);

    for (;;) {
      long timeout = JVM_TimeSlice();
      if (timeout <= -2) {
        break;
      } else {
        int blocked_threads_count;
        JVMSPI_BlockedThreadInfo * blocked_threads;

        blocked_threads = SNI_GetBlockedThreads(&blocked_threads_count);
        JVMSPI_CheckEvents(blocked_threads, blocked_threads_count, timeout);
      }
    }

    code = JVM_CleanUp();
  }

end:
#if ENABLE_PCSL
  pcsl_mem_finalize();
#endif

  return code;
}
