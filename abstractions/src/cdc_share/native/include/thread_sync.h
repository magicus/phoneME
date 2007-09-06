/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#ifndef _THREAD_SYNC_H
#define _THREAD_SYNC_H

/**
 * A mutex abstraction.
 */
typedef struct _ThreadMutex *ThreadMutex;

/**
 * A condition variable abstraction.
 */
typedef struct _ThreadCond *ThreadCond;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Error codes.
 */
enum {
    
    /** 
     * Operation wasn't completed because a resource is busy. 
     * (A synchronous call would be blocked)
     */
    THREAD_SYNC_WOULD_BLOCK = 1,
    
    /** Operation was completed with success. */
    THREAD_SYNC_OK = 0,
    
    /** Operation wasn't completed because an unspecified error occured. */
    THREAD_SYNC_FAILURE = -1,
    
    /** Operation wasn't completed because of lack of memory. */
    THREAD_SYNC_OUT_OF_MEMORY = -2,
    
    /** Operation wasn't completed because a timeout is elapsed. */
    THREAD_SYNC_TIMEOUT = -3,

};

/**
 * Creates new mutex.
 * @return new mutex, <code>NULL</code> in case of any error.
 */
ThreadMutex threadMutexCreate();

/**
 * Destroys the mutex.
 * @param mutex the mutex to destroy.
 */
void threadMutexDestroy(ThreadMutex mutex);

/**
 * Acquires the mutex, waits if the mutex is busy.
 * @param mutex the mutex.
 * @retval THREAD_SYNC_OK in case of success 
 * @retval THREAD_SYNC_OUT_OF_MEMORY in case of lack of memory 
 * @retval THREAD_SYNC_FAILURE in case of any other error 
 */
int threadMutexLock(ThreadMutex mutex);

/**
 * Tries to acquire the Mutex, returns immidiately.
 * @param mutex the mutex.
 * @retval THREAD_SYNC_OK in case of success 
 * @retval THREAD_SYNC_OUT_OF_MEMORY in case of lack of memory 
 * @retval THREAD_SYNC_FAILURE in case of any other error 
 * @retval THREAD_SYNC_WOULD_BLOCK if the mutex is acquired by somebody else 
 */
int threadMutexTrylock(ThreadMutex mutex);

/**
 * Frees the mutex.
 * @param mutex the mutex.
 * @retval THREAD_SYNC_OK in case of success 
 * @retval THREAD_SYNC_OUT_OF_MEMORY in case of lack of memory 
 * @retval THREAD_SYNC_FAILURE in case of any other error 
 */
int threadMutexUnlock(ThreadMutex mutex);

/**
 * Creates new condition variable.
 * @param mutex a mutex that will be bound with the condition variable
 * in <code>threadCondWait</code>.
 * @return new condition variable, <code>NULL</code> in case of any error.
 */
ThreadCond threadCondCreate(ThreadMutex mutex);

/**
 * Gets the mutex from the condition variable. This mutex was passed 
 * to <code>threadCondCreate</code>.
 * @param cond the condition variable.
 * @return the mutex, <code>NULL</code> in case of any error.
 */
ThreadMutex threadCondGetMutex(ThreadCond cond);

/**
 * Destroys the condition variable.
 * @param cond the condition variable to destroy.
 */
void threadCondDestroy(ThreadCond cond);

/**
 * Blocks current thread on the condition variable.
 * The mutex that has been provided at creation time (@see threadCondCreate)
 * will be used in this call. See details in POSIX's 
 * <code>pthread_cond_wait</code>. If the <code>millis</code> parameter
 * is not <code>0</code> this method will return 
 * <code>THREAD_SYNC_TIMEOUT</code> when <code>millis</code> milliseconds
 * elapse.
 * @param cond the condition variable.
 * @param millis the timeout in milliseconds
 * @retval THREAD_SYNC_OK in case of success 
 * @retval THREAD_SYNC_OUT_OF_MEMORY in case of lack of memory 
 * @retval THREAD_SYNC_TIMEOUT when <code>millis</code> milliseconds elapsed 
 * @retval THREAD_SYNC_FAILURE in case of any other error 
 */
int threadCondWait(ThreadCond cond, long millis);

/**
 * Unblocks a thread that was blocked on the condition variable by 
 * <code>threadCondWait</code>.
 * See details in POSIX's <code>pthread_cond_signal</code>. Implementation
 * can unblock more than one thread.
 * @param cond the condition variable.
 * @retval THREAD_SYNC_OK in case of success 
 * @retval THREAD_SYNC_OUT_OF_MEMORY in case of lack of memory 
 * @retval THREAD_SYNC_FAILURE in case of any other error 
 */
int threadCondSignal(ThreadCond cond);

/**
 * Unblocks all threads that were blocked on the condition variable by 
 * <code>threadCondWait</code>.
 * See details in POSIX's <code>pthread_cond_broadcast</code>.
 * @param cond the condition variable.
 * @retval THREAD_SYNC_OK in case of success 
 * @retval THREAD_SYNC_OUT_OF_MEMORY in case of lack of memory 
 * @retval THREAD_SYNC_FAILURE in case of any other error 
 */
int threadCondBroadcast(ThreadCond cond);
#ifdef __cplusplus
}
#endif

#endif /* #ifdef _THREAD_SYNC_H */
