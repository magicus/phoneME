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
#include <anilib_impl.h>

Os_Event Os_CreateEvent(jboolean *status) {
/*
 * Create an event object, is employed to sync. thread  
 */
  Os_Event  result = NULL; 
  *status = KNI_TRUE;
  return result;
}

void Os_WaitForEvent(Os_Event event) {
/*
 * Wait for specified async. event (signal or time-out interval)
 * In blocking mode
 */
}

void Os_SignalEvent(Os_Event event) {
/*
 * Specify the wait event 
 */
}

jint Os_WaitForEventOrTimeout(Os_Event event, jlong ms) {
/*
 * Tell the event time, either Async. Signal or Timeout Event 
 */
    return OS_SIGNALED;

  
}

void Os_DisposeEvent(Os_Event event) {
/* Reset the event object for the thread */
}

Os_Thread Os_CreateThread(int proc(void *parameter), void *arg,
                          jboolean *status) {
 /* Create a thread */ 
  Os_Thread os_thread = NULL;

  *status = KNI_FALSE; /* succeeded */

  

  return os_thread;
}

void Os_DisposeThread(Os_Thread thread) {

/* Reset the thread object */
}
