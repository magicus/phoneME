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
 * Linux declarations of OS calls required by platform-independent
 * parts of ANILib.
 */

#ifndef _OS_PORT_H_
#define _OS_PORT_H_

#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int signaled;
  pthread_cond_t condition;
  pthread_mutex_t mutex;
} Os_EventStruct;

typedef Os_EventStruct * Os_Event;
typedef pthread_t        Os_Thread;

extern Os_Event Os_CreateEvent(jboolean *status);
extern void Os_WaitForEvent(Os_Event event);
extern jint Os_WaitForEventOrTimeout(Os_Event event, jlong ms);
extern void Os_SignalEvent(Os_Event event);
extern void Os_DisposeEvent(Os_Event event);

extern Os_Thread Os_CreateThread(int proc(void *parameter), void *arg, 
                                 jboolean *status);
extern void Os_DisposeThread(Os_Thread thread);

extern void Os_DisposeEvent(Os_Event event);

#ifdef __cplusplus
}
#endif

#endif /* _OS_PORT_H_ */
