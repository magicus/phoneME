/*
 * @(#)threads.h	1.31 06/10/27
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
 *
 */

/*
 * Porting layer for threads.
 */

#ifndef _INCLUDED_PORTING_THREADS_H
#define _INCLUDED_PORTING_THREADS_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/vm-defs.h"

/*
 * Platform must provide definitions for the following opaque struct(s):
 *
 * struct CVMThreadID		Uniquely identifies a thread.
 */

/*
 * Create a new, runnable thread with the given Java priority
 * and C stacksize.  The new thread must call func(arg) upon
 * creation and clean up after itself and exit when func()
 * returns. Returns CVM_TRUE if successful.
 */
extern CVMBool CVMthreadCreate(CVMThreadID *thread,
    CVMSize stackSize, CVMInt32 priority,
    void (*func)(void *), void *arg);

/*
 * Yield to other threads, as defined by Thread.yield().
 */
extern void CVMthreadYield(void);

/*
 * Set the priority of a thread to the given value, mapping the
 * Java priority to an appropriate value.
 */
extern void CVMthreadSetPriority(CVMThreadID *thread, CVMInt32 prio);

/*
 * Suspend a thread, according to the semantics of Thread.suspend()
 * (which has been deprecated) or JVMTI SuspendThread().
 */
extern void CVMthreadSuspend(CVMThreadID *thread);

/*
 * Resume a suspended thread, according to the semantics of Thread.resume()
 * (which has been deprecated) or JVMTI ResumeThread().
 */
extern void CVMthreadResume(CVMThreadID *thread);

/*
 * Associate a CVMThreadID * with the current thread.  The "orphan"
 * flag is set if the thread was not created by CVMthreadCreate().
 * Return CVM_TRUE for success, CVM_FALSE for failure.
 */
extern CVMBool CVMthreadAttach(CVMThreadID *self, CVMBool orphan);

/*
 * Remove the CVMThreadID * from the current thread.
 */
extern void CVMthreadDetach(CVMThreadID *self);

/*
 * Return the CVMThreadID * previously associated with
 * the current thread by CVMthreadAttach(), or NULL if
 * CVMthreadAttach() has not been called yet or
 * CVMthreadDetach() has been called.
 */
extern CVMThreadID * CVMthreadSelf(void);

/*
 * Break the given thread out of a CVMcondvarWait() call,
 * causing that call to return CVM_FALSE, signaling that
 * the wait was interrupted.  If the given thread is not
 * currently in CVMcondvarWait(), the requested is
 * remembered until CVMcondvarWait() is called, which
 * should return immediately with CVM_FALSE.
 */
extern void CVMthreadInterruptWait(CVMThreadID *thread);

/*
 * Return the current interrupted state for the thread.
 * If "clearInterrupted" is true, cancel any previous
 * interrupt requests made by CVMthreadInterruptWait().
 * (Currently, we only set "clearInterrupted" when "thread"
 * is the current thread.)
 */
extern CVMBool CVMthreadIsInterrupted(CVMThreadID *thread,
    CVMBool clearInterrupted);

/*
 * Check for a stack overflow.  Return CVM_TRUE if the stack
 * of the current thread has at least redZone bytes free,
 * CVM_FALSE otherwise.
 */
extern CVMBool CVMthreadStackCheck(CVMThreadID *self, CVMUint32 redZone);

#include CVM_HDR_THREADS_H

#if CVM_THREAD_MIN_C_STACK_SIZE < CVM_REDZONE_ILOOP
#error CVM_THREAD_MIN_C_STACK_SIZE < CVM_REDZONE_ILOOP
#endif

#ifndef CVMthreadSchedHook
#define CVMthreadSchedHook(tid) ((void)0)
#endif

#endif /* _INCLUDED_PORTING_THREADS_H */
