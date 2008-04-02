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
 * Linux implementation of OS calls required by platform-independent
 * parts of ANILib.
 */

#include <anilib_impl.h>

Os_Event Os_CreateEvent(jboolean *status) {
  Os_Event event = (Os_Event) malloc(sizeof(Os_EventStruct));

  pthread_cond_t temp_cond = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t temp_mux = PTHREAD_MUTEX_INITIALIZER;

  event->signaled      = 0;
  event->condition     = temp_cond;
  event->mutex         = temp_mux;

  pthread_cond_init(&event->condition, NULL);
  pthread_mutex_init(&event->mutex, NULL);

  /* IMPL_NOTE: check for error conditions */
  *status = KNI_TRUE;
  return event;
}

void Os_WaitForEvent(Os_Event event) {
  pthread_mutex_lock(&event->mutex);
  while (event->signaled == 0) {
    pthread_cond_wait(&event->condition, &event->mutex);
  }
  event->signaled--;
  pthread_mutex_unlock(&event->mutex);
}

void Os_SignalEvent(Os_Event event) {
  pthread_mutex_lock(&event->mutex);
  event->signaled++;
  pthread_cond_signal(&event->condition);
  pthread_mutex_unlock(&event->mutex);
}

jint Os_WaitForEventOrTimeout(Os_Event event, jlong ms) {
  int result = 0;
  struct timeval now = {0,0};
  struct timespec timeout = {0,0};

  if (ms < 0) {
    /* wait forever */
    Os_WaitForEvent(event);
    return OS_SIGNALED;
  }

  pthread_mutex_lock(&event->mutex);
  gettimeofday(&now, NULL);

  if (ms == 0) {
    timeout.tv_sec = now.tv_sec;
    timeout.tv_nsec = now.tv_usec * 1000;  

    if (ms >= 1000) {
      timeout.tv_sec += ((signed long) (((ms) >> 32) & 0xffffffff));
      timeout.tv_sec += ((signed long) (((ms) & 0xffffffff))) / 1000;
      ms -= (ms/1000)*1000;
    }
    timeout.tv_nsec += ((int)ms * 1000000);
  } else {
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;
  }

  if (event->signaled == 0) {
    result =
      pthread_cond_timedwait(&event->condition, &event->mutex, &timeout);
  }

  if (result != ETIMEDOUT) {
    event->signaled--;
  }

  pthread_mutex_unlock(&event->mutex);
  if (result == ETIMEDOUT) {
    return OS_TIMEOUT;
  } else {
    return OS_SIGNALED;
  }
}

void Os_DisposeEvent(Os_Event event) {
  pthread_mutex_destroy(&event->mutex);
  pthread_cond_destroy(&event->condition);

  free((void*)event);
}

Os_Thread Os_CreateThread(int proc(void *parameter), void *arg, 
                          jboolean *status) {
  pthread_attr_t attr;
  pthread_t os_thread;
  void* (*routine)(void*) = (void*(*)(void *))proc;

  if (pthread_attr_init(&attr) != 0) {
    *status = KNI_FALSE;
    return 0;
  }

  if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
    *status = KNI_FALSE;
    return 0;
  }

  if (pthread_create(&os_thread, &attr, routine, arg) != 0) {
    *status = KNI_FALSE;
    return 0;
  } else {
    *status = KNI_TRUE;
    return os_thread;
  }
}

void Os_DisposeThread(Os_Thread thread) {
  /* IMPL_NOTE: nothing to do? */
}
