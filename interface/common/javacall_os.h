/*
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

#ifndef __JAVACALL_OS_H
#define __JAVACALL_OS_H


#include "javacall_defs.h"


/**
 * A mutex abstraction.
 */
typedef struct _javacall_mutex *javacall_mutex;

/**
 * A condition variable abstraction.
 */
typedef struct _javacall_cond *javacall_cond;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the OS structure.
 * This is where timers and threads get started for the first
 * real_time_tick event, and where signal handlers and other I/O
 * initialization should occur.
 *
 */
void javacall_os_initialize(void);

/**
 * Performs a clean-up of all threads and other OS related activity
 * to allow for a clean and complete restart.  This should undo
 * all the work that initialize does.
 */
void javacall_os_dispose();

/** 
 * javacall_os_flush_icache is used, for example, to flush any caches used by a
 * code segment that is deoptimized or moved during a garbage collection.
 * flush at least [address, address + size] (may flush complete icache).
 *
 * @param address   Start address to flush
 * @param size      Size to flush
 */
void javacall_os_flush_icache(unsigned char* address, int size);

/**
 * Creates new mutex.
 * @return new mutex, <code>NULL</code> in case of any error.
 */
javacall_mutex javacall_os_mutex_create();

/**
 * Destroys the mutex.
 * @param mutex the mutex to destroy.
 */
void javacall_os_mutex_destroy(javacall_mutex mutex);

/**
 * Acquires the mutex, waits if the mutex is busy.
 * @param mutex the mutex.
 * @retval JAVACALL_OK in case of success 
 * @retval JAVACALL_OUT_OF_MEMORY in case of lack of memory 
 * @retval JAVACALL_FAIL in case of any other error 
 */
javacall_result javacall_os_mutex_lock(javacall_mutex mutex);

/**
 * Tries to acquire the Mutex, returns immidiately.
 * @param mutex the mutex.
 * @retval JAVACALL_OK in case of success 
 * @retval JAVACALL_OUT_OF_MEMORY in case of lack of memory 
 * @retval JAVACALL_FAIL in case of any other error 
 * @retval JAVACALL_WOULD_BLOCK if the mutex is acquired by somebody else 
 */
javacall_result javacall_os_mutex_try_lock(javacall_mutex mutex);

/**
 * Frees the mutex.
 * @param mutex the mutex.
 * @retval JAVACALL_OK in case of success 
 * @retval JAVACALL_OUT_OF_MEMORY in case of lack of memory 
 * @retval JAVACALL_FAIL in case of any other error 
 */
javacall_result javacall_os_mutex_unlock(javacall_mutex mutex);

/**
 * Creates new condition variable.
 * @param mutex a mutex that will be bound with the condition variable
 * in <code>javacall_os_cond_wait</code>.
 * @return new condition variable, <code>NULL</code> in case of any error.
 */
javacall_cond javacall_os_cond_create(javacall_mutex mutex);

/**
 * Gets the mutex from the condition variable. This mutex was passed 
 * to <code>javacall_os_cond_create</code>.
 * @param cond the condition variable.
 * @return the mutex, <code>NULL</code> in case of any error.
 */
javacall_mutex javacall_os_cond_get_mutex(javacall_cond cond);

/**
 * Destroys the condition variable.
 * @param cond the condition variable to destroy.
 */
void javacall_os_cond_destroy(javacall_cond cond);

/**
 * Blocks current thread on the condition variable.
 * The mutex that has been provided at creation time
 * (@see javacall_os_cond_create)
 * will be used in this call. See details in POSIX's 
 * <code>pthread_cond_wait</code>. If the <code>millis</code> parameter
 * is not <code>0</code> this method will return 
 * <code>JAVACALL_TIMEOUT</code> when <code>millis</code> milliseconds
 * elapse.
 * @param cond the condition variable.
 * @param millis the timeout in milliseconds
 * @retval JAVACALL_OK in case of success 
 * @retval JAVACALL_OUT_OF_MEMORY in case of lack of memory 
 * @retval JAVACALL_TIMEOUT when <code>millis</code> milliseconds elapsed 
 * @retval JAVACALL_FAIL in case of any other error 
 */
javacall_result javacall_os_cond_wait(javacall_cond cond, long millis);

/**
 * Unblocks a thread that was blocked on the condition variable by 
 * <code>javacall_os_cond_wait</code>.
 * See details in POSIX's <code>pthread_cond_signal</code>. Implementation
 * can unblock more than one thread.
 * @param cond the condition variable.
 * @retval JAVACALL_OK in case of success 
 * @retval JAVACALL_OUT_OF_MEMORY in case of lack of memory 
 * @retval JAVACALL_FAIL in case of any other error 
 */
javacall_result javacall_os_cond_signal(javacall_cond cond);

/**
 * Unblocks all threads that were blocked on the condition variable by 
 * <code>javacall_os_cond_wait</code>.
 * See details in POSIX's <code>pthread_cond_broadcast</code>.
 * @param cond the condition variable.
 * @retval JAVACALL_OK in case of success 
 * @retval JAVACALL_OUT_OF_MEMORY in case of lack of memory 
 * @retval JAVACALL_FAIL in case of any other error 
 */
javacall_result javacall_os_cond_broadcast(javacall_cond cond);


#ifdef __cplusplus
}
#endif

#endif

