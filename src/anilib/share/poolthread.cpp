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

static PoolThread pool_threads[NUM_POOL_THREADS];
static Os_Event thread_started_event;
static Os_Event thread_finished_event;

jboolean PoolThread_InitializePool()
{
  jboolean status;

  thread_started_event = Os_CreateEvent(&status);
  if (status != KNI_TRUE) {
    return KNI_FALSE;
  }

  thread_finished_event = Os_CreateEvent(&status);
  if (status != KNI_TRUE) {
    Os_DisposeEvent(thread_started_event);
    return KNI_FALSE;
  }

  memset(pool_threads, 0, sizeof(pool_threads));
  return KNI_TRUE;
}

/*
 * This should be called only if PoolThread_InitializePool returns KNI_TRUE
 */
void PoolThread_DisposePool()
{
  Os_DisposeEvent(thread_started_event);
  Os_DisposeEvent(thread_finished_event);
}

PoolThread * PoolThread_Allocate() {
  int i;
  jboolean status;

  for (i=0; i<NUM_POOL_THREADS; i++) {
    PoolThread * pt = &pool_threads[i];

    if (!pt->is_used) {
      if (pt->is_initialized) {
        /*
         * Here's a thread that has been previously used to perform a blocking
         * task, but has been released via PoolThread_release().
         */
        JVM_ASSERT(pt->execute_event_created, "should be already spawned");
        JVM_ASSERT(pt->os_thread_created,     "should be already spawned");
        pt->is_used = KNI_TRUE;
      } else {
        pt->is_initialized = KNI_TRUE;
        pt->is_used = KNI_TRUE;
        pt->is_idle = KNI_TRUE;
        pt->is_terminating = KNI_FALSE;
        pt->parameter_block = NULL;
        pt->parameter_size = 0;
        pt->os_thread_created = KNI_FALSE;
        pt->execute_event_created = KNI_FALSE;
        pt->execute_event = Os_CreateEvent(&status);

        if (status == KNI_FALSE) {
          // Not enough OS resource, try again later
          PoolThread_Free(pt);
          return NULL;
        } else {
          pt->execute_event_created = KNI_TRUE;
        }

        if (PoolThread_Spawn(pt) != KNI_TRUE) {
          // Not enough OS resource, try again later
          PoolThread_Free(pt);
          return NULL;
        }
      }
      /*
       * At this point we have an associated native thread started and
       * blocking on pt->execute_event
       */
      return pt;
    }
  }

  /*
   * All available PoolThread data structures are in use. We need to wait
   * for other ANI method invocation to complete and try again.
   */
  return NULL;
}

int PoolThread_FreeCount() {
  int i, free_count = 0;

  for (i=0; i<NUM_POOL_THREADS; i++) {
    PoolThread * pt = &pool_threads[i];
    if (!pt->is_used) {
      free_count ++;
    }
  }

  return free_count;
}
 
void PoolThread_Free(PoolThread *pt) {
  JVM_ASSERT(pt->is_used, "must already be in use");
  JVM_ASSERT(pt->os_thread_created == KNI_FALSE, "must be single-threaded");

  if (pt->execute_event_created) {
    Os_DisposeEvent(pt->execute_event);
  }
  memset(pt, 0, sizeof(*pt));
}

// This is the start of a pool thread.
static int PoolThread_Run(void* arg) {
  PoolThread *pt = (PoolThread *)arg;
  char parameter_block[ANI_MAX_ALLOCATION_SIZE];

  pt->parameter_block = parameter_block;

  /*
   * Tell PoolThread_Spawn we're started
   */
  Os_SignalEvent(thread_started_event);

  while (!pt->is_terminating) {
    /*
     * Wait for ANI method to call PoolThread_StartExecution()
     */
    Os_WaitForEvent(pt->execute_event);
    if (pt->is_terminating) {
      break;
    }

    (*pt->function)(pt->parameter_block, /*is_blocking*/ KNI_FALSE);

    pt->is_idle = KNI_TRUE;
    pt->function = NULL;
    Os_SignalEvent(thread_finished_event);
  }

  Os_DisposeEvent(pt->execute_event);
  pt->is_used = KNI_FALSE;

  return 0; /* dummy value */
}

jboolean PoolThread_Spawn(PoolThread *pt) {
  jboolean status;
  pt->os_thread = Os_CreateThread(PoolThread_Run, pt, &status);
  if (status == KNI_FALSE) {
    return KNI_FALSE;
  } else {
    pt->os_thread_created = KNI_TRUE;
  }
  Os_WaitForEvent(thread_started_event);
  return KNI_TRUE;
}

void PoolThread_StartExecution(PoolThread *pt) {
  JVM_ASSERT(pt->is_idle, "must not be executing!");
  pt->is_idle = KNI_FALSE;
  Os_SignalEvent(pt->execute_event);
}

void 
PoolThread_WaitForFinishOrTimeout(JVMSPI_BlockedThreadInfo * blocked_threads,
                                  int blocked_threads_count, 
                                  jlong timeout_milli_seconds) {
  int i, j;
  ANI_BlockingInfo *p;

  if (timeout_milli_seconds < 0) {
    /* wait forever */
    Os_WaitForEvent(thread_finished_event);
  } else {
    int retval = Os_WaitForEventOrTimeout(thread_finished_event, 
                                          timeout_milli_seconds);
    if (retval == OS_TIMEOUT) {
      /*
       * We just timed out without any PoolThread finishing.
       */
        return;
    }
  }

  for (i=0; i<NUM_POOL_THREADS; i++) {
    PoolThread * pt = &pool_threads[i];
    if (!pt->is_idle) {
      continue;
    }

    for (j=0; j<blocked_threads_count; j++) {
      JVMSPI_BlockedThreadInfo *info = &blocked_threads[j];
      jint size = info->reentry_data_size;
      p = (ANI_BlockingInfo *)info->reentry_data;
      if ((size == sizeof(*p)) && (p->type == ANI_BLOCK_INFO)) {
        /*
         * OK, we've found a Java thread that's blocked by ANI
         */
        if (pt == p->pt) {
          /*
           * The PoolThread associated with this Java thread has just
           * finished
           */
          SNI_UnblockThread(info->thread_id);
        }
      }
    }
  }
}

void PoolThread_Release(PoolThread *pt) {
  JVM_ASSERT(pt->is_idle, "must not be executing!");
  pt->is_used = KNI_FALSE;
  // IMPL_NOTE: iterate over the list of waiting threads and restart one
}
