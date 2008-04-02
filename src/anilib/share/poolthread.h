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
 * PoolThread - Native thread pool used by ANI.
 * (See ani.h for more details on using ANI)
 */

#ifndef _POOL_THREAD_H_
#define _POOL_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Change this if your system allows more/fewer pool threads */
#define NUM_POOL_THREADS 16

typedef struct _PoolThread {
  jboolean is_initialized;  /* Has a native thread be created? */
  jboolean is_used;         /* Is it associated with an ANI method call? */
  jboolean is_idle;         /* Is native thread doing something */
  jboolean is_terminating;
  Os_Thread os_thread;
  jboolean os_thread_created;
  Os_Event execute_event;
  jboolean execute_event_created;
  void *parameter_block;
  size_t parameter_size;
  ANI_AsynchronousFunction function;
} PoolThread;

extern jboolean PoolThread_InitializePool();
extern void     PoolThread_DisposePool();
extern int      PoolThread_FreeCount();

extern PoolThread * PoolThread_Allocate();
extern void PoolThread_Free(PoolThread *pt);
extern jboolean PoolThread_Spawn(PoolThread *pt);
extern void PoolThread_WaitForFinishOrTimeout(
                         JVMSPI_BlockedThreadInfo * blocked_threads,
                         int blocked_threads_count, 
                         jlong timeout_milli_seconds);
extern void PoolThread_Release(PoolThread *pt);
extern void PoolThread_StartExecution(PoolThread *pt);

#ifdef __cplusplus
}
#endif

#endif /* _POOL_THREAD_H_ */
