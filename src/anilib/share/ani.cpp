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
#include "anilib_impl.h"

static jint waiter_count;

void ANI_Initialize() {
  PoolThread_InitializePool();
  waiter_count = 0;
}

void ANI_Dispose() {
  PoolThread_DisposePool();
}

jboolean ANI_Start() {
  ANI_BlockingInfo * p = (ANI_BlockingInfo*)SNI_GetReentryData(NULL);
  if (p == NULL) {
    /*
     * Called for the first time for this ANI method invocation. We need to
     * allocate a PoolThread for this invocation
     */
    p = (ANI_BlockingInfo*)SNI_AllocateReentryData(sizeof(ANI_BlockingInfo));
    if (p == NULL) {
      // we cannot allocate buffer, so refuse to start
      return KNI_FALSE;
    } 
    p->type = ANI_BLOCK_INFO;
    p->pt = PoolThread_Allocate();
    p->parameter_block_allocated = KNI_FALSE;
  } else {
    if (p->pt == NULL) {
      /* This invocation had call ANI_Wait() before. Let's try again to
       * allocate a thread */
      p->pt = PoolThread_Allocate();
      p->parameter_block_allocated = KNI_FALSE;
    }
  }

  /*
   * This thread is not considered blocked until it calls ANI_BlockThread()
   */
  p->is_blocked = KNI_FALSE;

  if (p->pt == NULL) {
    /*
     * The caller needs to call ANI_Wait. The method invocation will 
     * be retried when a new poolthread becomes available.
     */
    return KNI_FALSE;
  } else {
    return KNI_TRUE;
  }
}

void ANI_Wait() {
#ifdef AZZERT
  ANI_BlockingInfo * p = (ANI_BlockingInfo*)SNI_GetReentryData(NULL);
  JVM_ASSERT(p != NULL, "ReentryData should have been allocated");
  JVM_ASSERT(p->pt == NULL, "sanity");
#endif

  waiter_count ++;
  SNI_BlockThread();
}

void* ANI_AllocateParameterBlock(size_t parameter_size) {
  ANI_BlockingInfo * p = (ANI_BlockingInfo*)SNI_GetReentryData(NULL);
  JVM_ASSERT(p != NULL, "ReentryData should have been allocated");

  if (parameter_size > ANI_MAX_ALLOCATION_SIZE) {
    return NULL;
  }

  p->pt->parameter_size = parameter_size;
  p->parameter_block_allocated = KNI_TRUE;
  return p->pt->parameter_block;
}

void* ANI_GetParameterBlock(size_t *parameter_size) {
  ANI_BlockingInfo * p = (ANI_BlockingInfo*)SNI_GetReentryData(NULL);
  JVM_ASSERT(p != NULL, "ReentryData should have been allocated");

  if (!p->parameter_block_allocated) {
    return NULL;
  }

  if (parameter_size != NULL) {
    *parameter_size = p->pt->parameter_size;
  }
  return p->pt->parameter_block;
}

jboolean
ANI_UseFunction(ANI_AsynchronousFunction function, jboolean try_non_blocking)
{
  ANI_BlockingInfo * p = (ANI_BlockingInfo*)SNI_GetReentryData(NULL);
  JVM_ASSERT(p != NULL, "ReentryData should have been allocated");

  p->pt->function = function;
  if (try_non_blocking) {
    return (*function)(p->pt->parameter_block, /*is_blocking*/ KNI_FALSE);
  } else {
    return KNI_FALSE;
  }
}

void ANI_BlockThread() {
  ANI_BlockingInfo * p = (ANI_BlockingInfo*)SNI_GetReentryData(NULL);
  JVM_ASSERT(p != NULL, "ReentryData should have been allocated");

  p->is_blocked = KNI_TRUE;
  PoolThread_StartExecution(p->pt);
}

void ANI_End() {
  ANI_BlockingInfo * p = (ANI_BlockingInfo*)SNI_GetReentryData(NULL);
  JVM_ASSERT(p != NULL, "ReentryData should have been allocated");

  if (p->is_blocked) {
    SNI_BlockThread();
  } else {
    PoolThread_Release(p->pt);
  }
}

void ANI_WaitForThreadUnblocking(JVMSPI_BlockedThreadInfo * blocked_threads,
                                 int blocked_threads_count, 
                                 jlong timeout_milli_seconds) {
  int i, free_count;
  ANI_BlockingInfo *p;

  if (waiter_count > 0 && (free_count = PoolThread_FreeCount()) > 0) {
    JVM_ASSERT(blocked_threads_count > 0, "sanity");

    for (i=0; i<blocked_threads_count; i++) {
      JVMSPI_BlockedThreadInfo *info = &blocked_threads[i];
      jint size = info->reentry_data_size;
      p = (ANI_BlockingInfo *)info->reentry_data;

      if ((size == sizeof(*p)) && (p->type == ANI_BLOCK_INFO)) {
        /*
         * OK, we've found a Java thread that's blocked by ANI
         */
        if (p->pt == NULL) {
          /*
           * This thread called ANI_Wait() because no PoolThread were
           * available at the time. Let's try again.
           */
          SNI_UnblockThread(info->thread_id);
          waiter_count --;
          free_count --;
          JVMSPI_PrintRaw(".");
        }
      }
      if (free_count <= 0) {
        break;
      }
    }

    timeout_milli_seconds = 0;
  }
  PoolThread_WaitForFinishOrTimeout(blocked_threads, blocked_threads_count,
                                    timeout_milli_seconds);
}
