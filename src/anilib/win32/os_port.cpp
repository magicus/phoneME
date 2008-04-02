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

/**
 * Win32 implementation of OS calls required by platform-independent
 * parts of ANILib.
 */

#include "incls/_precompiled.incl"
#include "anilib_impl.h"

Os_Event Os_CreateEvent(jboolean *status) {
  HANDLE result = CreateEvent(NULL, FALSE, FALSE, NULL);
  JVM_ASSERT(result != NULL, "Please don't fail!");
  *status = KNI_TRUE;
  return result;
}

void Os_WaitForEvent(Os_Event event) {
  int code = WaitForSingleObject(event, INFINITE);
  JVM_ASSERT(code == WAIT_OBJECT_0, "sanity");
}

void Os_SignalEvent(Os_Event event) {
  int code = SetEvent(event);
  JVM_ASSERT(code != 0, "sanity");
}

jint Os_WaitForEventOrTimeout(Os_Event event, jlong ms) {
  // win32 WaitForSingleObject takes 32-bit unsigned argument
  // Should really do loop here if sleeping more than 49 days
  DWORD duration = (DWORD)(ms & 0x7fffffff);
  int code;
  if ((code = WaitForSingleObject(event, duration)) == WAIT_TIMEOUT) {
    return OS_TIMEOUT;
  } else {
    JVM_ASSERT(code == WAIT_OBJECT_0, "sanity");
    return OS_SIGNALED;
  }
}

void Os_DisposeEvent(Os_Event event) {
  CloseHandle(event);
}

Os_Thread Os_CreateThread(int proc(void *parameter), void *arg,
                          jboolean *status) {
  DWORD unused;
  HANDLE os_thread;
  os_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) proc,
                           arg, CREATE_SUSPENDED, &unused);
  if (os_thread != NULL) {
    ResumeThread(os_thread);
    *status = KNI_TRUE; /* succeeded */
  } else {
    *status = KNI_FALSE; /* succeeded */
  }

  return os_thread;
}

void Os_DisposeThread(Os_Thread thread) {
  if (thread != NULL) {
    CloseHandle(thread);
  }
}
