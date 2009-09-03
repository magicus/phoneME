/*
 * @(#)sync.h	1.24 06/10/10
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

#ifndef _INCLUDED_PORTING_SYNC_H
#define _INCLUDED_PORTING_SYNC_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/vm-defs.h"

/*
 * The platform defines the CVM_MICROLOCK_TYPE variable to be
 * one of the following.
 */

#define CVM_MICROLOCK_DEFAULT		-1
#define CVM_MICROLOCK_SCHEDLOCK		1
#define CVM_MICROLOCK_SWAP_SPINLOCK	2
#define CVM_MICROLOCK_PLATFORM_SPECIFIC	3

/*
 * The platform defines the CVM_FASTLOCK_TYPE variable to be
 * one of the following.
 */

#define CVM_FASTLOCK_NONE		-1
#define CVM_FASTLOCK_ATOMICOPS		1
#define CVM_FASTLOCK_MICROLOCK		3

#ifndef _ASM

#ifdef CVM_ADV_SPINLOCK
/*
 * If CVM_ADV_SPINLOCK is #defined, then CVM_ADV_ATOMIC_SWAP must also
 * be defined, and the platform must supply CVMvolatileStore and
 * CVMspinlockYield.
 */

/*
 * Yield the processor to another thread, so a later attempt can be made
 * to grab the microlock.
 */
extern void CVMspinlockYield();

/*
 * CVMvolatileStore is used to unlock the microlock. Conceptually
 * it is simply a store of new_value into *addr. However, it must be done
 * in such a way as to guarantee that the compiler doesn't shuffle
 * instructions around it. Thus an assembler inline function is normally
 * used. It also must provide a membar on MP systems.
 */
extern void CVMvolatileStore(volatile CVMAddr *addr, CVMAddr new_value);

#endif /* CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK */

/*
 * If CVM_MICROLOCK_TYPE == CVM_MICROLOCK_PLATFORM_SPECIFIC, then the
 * platform must provide definitions for the following microlock
 * related struct and APIs. Otherwise default versions will be
 * provided based on the setting of CVM_MICROLOCK_TYPE.
 */

/*
 * opaque struct for microlocks.
 *
 * struct CVMMicroLock
 */

/*
 * Initialize the given CVMMicroLock, allocating additional resources if
 * necessary.
 */
extern CVMBool CVMmicrolockInit   (CVMMicroLock* m);

/*
 * Destroy the given CVMMicroLock, releasing any resources that were
 * previously allocated by CVMmicrolockInit.
 */
extern void    CVMmicrolockDestroy(CVMMicroLock* m);

/*
 * Acquire a lock for the given microlock.  This call may block.
 * A thread will never attempt to acquire the same microlock
 * more than once, so a counter to keep track of recursive
 * entry is not required.
 */
extern void    CVMmicrolockLock   (CVMMicroLock* m);

/*
 * Release a lock previously acquired by CVMmicrolockLock.
 */
extern void    CVMmicrolockUnlock (CVMMicroLock* m);

/*
 * Platform must provide definitions for the following opaque structs:
 *
 * struct CVMMutex
 * struct CVMCondVar
 *
 */

/*
 * Initialize the given CVMMutex, allocating additional resources if necessary.
 */
extern CVMBool CVMmutexInit   (CVMMutex* m);

/*
 * Destroy the given CVMMutex, releasing any resources that were
 * previously allocated by CVMmutexInit.
 */
extern void    CVMmutexDestroy(CVMMutex* m);

/*
 * Attempt to acquire a lock for the given mutex.  This call must return
 * immediately and never block. CVM_TRUE is returned if the lock was
 * acquired, otherwise CVM_FALSE is returned.
 */
extern CVMBool CVMmutexTryLock(CVMMutex* m);

/*
 * Acquire a lock for the given mutex.  This call may block.
 * A thread will never attempt to acquire the same mutex
 * more than once, so a counter to keep track of recursive
 * entry is not required.
 */
extern void    CVMmutexLock   (CVMMutex* m);

/*
 * Release a lock previously acquired by CVMmutexLock.
 */
extern void    CVMmutexUnlock (CVMMutex* m);

#ifdef CVM_ADV_MUTEX_SET_OWNER
/*
 * Lock an unlocked mutex on behalf of another thread.
 * The result will be identical to the thread having
 * called CVMmutexLock itself.
 */
extern void    CVMmutexSetOwner(CVMThreadID *self, CVMMutex* m,
				CVMThreadID *ti);
#endif

#ifdef CVM_ADV_THREAD_BOOST

/*
 * Support for thread priority boosting.  This is an alternative
 * solution for lazy monitor inflation due to first contention
 * if CVMmutexSetOwner is not available.  All of the calls are
 * synchronized by the caller except CVMthreadBoostAndWait, so
 * calls will be serialized for any given boost record.
 *
 * NOTE: These APIs are only used to facilitate monitor inflation.
 */

/*
 * Initialize boost record.
 */
extern CVMBool CVMthreadBoostInit(CVMThreadBoostRecord *b);

/*
 * Destroy boost record.
 */
extern void CVMthreadBoostDestroy(CVMThreadBoostRecord *b);

/*
 * Sets up boosting info in preparation for CVMthreadBoostAndWait()
 * i.e adds a booster (synchronized).  Returns queue for boost-and-wait call.
 * NOTE: CVMthreadAddBooster() is protected by the VM syncLock. 
 */
extern CVMThreadBoostQueue *CVMthreadAddBooster(CVMThreadID *self,
					        CVMThreadBoostRecord *b,
					        CVMThreadID *target);
/*
 * Boosts the priority of the target thread who is supposed to own the
 * newly inflated monitor.  After that, the current thread will block until
 * the target thread cancels the boost on the monitor via
 * CVMthreadCancelBoost().  The target thread is in recorded in the boost
 * record if necessary (implementation dependent).
 * NOTE: Not protected by the VM syncLock because this function can block.
 */
extern void    CVMthreadBoostAndWait(CVMThreadID *self,
				     CVMThreadBoostRecord *b,
				     CVMThreadBoostQueue *q);

/* Cancels the boost that was previously put on this boost record which
   boosted the priority of the current thread.  This cancellation allows the
   current thread to be restored to its original priority value before the
   boost operation.
   NOTE: CVMthreadCancelBoost() is protected by the VM syncLock.
*/
extern void    CVMthreadCancelBoost(CVMThreadID *self,
				    CVMThreadBoostRecord *b);
#endif

/*
 * Initialize the given CVMCondVar, allocating additional resources if
 * necessary.  The CVMMutex that will be used by any future calls
 * to CVMcondvarWait is also provided, for those platforms that
 * require the mutex for proper condition variable initialization.
 */
extern CVMBool CVMcondvarInit   (CVMCondVar *c, CVMMutex* m);
/*
 * Destroy the given CVMCondVar, releasing any resources that were
 * previously allocated by CVMcondvarInit.
 */
extern void    CVMcondvarDestroy(CVMCondVar *c);

/*
 * Perform a timed wait on a condition variable.  The mutex is
 * unlocked during while the thread blocks and locked again
 * when it wakes up.  CVM_FALSE is returned if the operation
 * is interrupted by CVMthreadInterruptWait.  This function
 * is used to implement Object.wait, therefore the atomicity of 
 * unlocking the lock and checking for notification or an
 * interrupt must follow the rules described by Object.wait.
 * A timeout of 0ms is treated as infinity, or "wait forever".
 */
extern CVMBool CVMcondvarWait     (CVMCondVar* c, CVMMutex* m,
				   CVMJavaLong millis);
/*
 * Wakeup a thread waiting on the given CVMCondVar, as defined
 * by the semantics of Object.notify.
 */
extern void    CVMcondvarNotify   (CVMCondVar* c);
/*
 * Wakeup all threads waiting on the given CVMCondVar, as defined
 * by the semantics of Object.notifyAll.
 */
extern void    CVMcondvarNotifyAll(CVMCondVar* c);

/*
 * Take advantage of other advanced features of the platform
 */

#ifdef CVM_ADV_SCHEDLOCK
/*
 * Lock the scheduler.  The current thread will remain running
 * and no other threads in this VM instance can become runnable
 * until the schedular lock is released.
 */
extern void CVMschedLock(void);
/*
 * Release the scheduler lock acquired by CVMschedLock.  Normal
 * thread scheduling will resume.
 */
extern void CVMschedUnlock(void);
#endif

#ifdef CVM_ADV_ATOMIC_CMPANDSWAP
/*
 * Conditionally set a word in memory, returning the previous
 * value.  The new value "nnew" is written to address "addr" only
 * if the current value matches the value given by "old".
 */
/*
 * make CVM ready to run on 64 bit platforms
 *
 * CVMatomicCompareAndSwap() is used in runtime/objsync.c
 * with CVMatomicCompareAndSwap(&obj->hdr.various32, nbits, obits);
 * Because the member various32 can hold a native pointer
 * the type of the function arguments addr, new and old has to be changed
 * to CVMAddr
 */
extern CVMAddr CVMatomicCompareAndSwap(volatile CVMAddr *addr,
	CVMAddr nnew, CVMAddr old);
#endif

#ifdef CVM_ADV_ATOMIC_SWAP
/*
 * Atomically set a word in memory, returning the previous
 * value.
 */
/*
 * make CVM ready to run on 64 bit platforms
 *
 * CVMatomicSwap() is used in runtime/objsync.c
 * with CVMatomicSwap(&(obj)->hdr.various32, CVM_LOCKSTATE_LOCKED)
 * Because the member various32 can hold a native pointer
 * the type of the function arguments addr and new has to be changed
 * to CVMAddr
 */
extern CVMAddr CVMatomicSwap(volatile CVMAddr *addr, CVMAddr nnew);
#endif

#ifdef CVM_MP_SAFE
/*
 * Make sure past writes are visible to all threads.
 */
extern void    CVMmemoryBarrier();
#endif

#endif /* !_ASM */

#include CVM_HDR_SYNC_H

/*
 * Sanity checks
 */
#ifndef CVM_MICROLOCK_TYPE
#define CVM_MICROLOCK_TYPE CVM_MICROLOCK_DEFAULT
#elif CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SCHEDLOCK
#ifndef CVM_ADV_SCHEDLOCK
#error CVM_MICROLOCK_SCHEDLOCK but not CVM_ADV_SCHEDLOCK
#endif
#elif CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK
#ifndef CVM_ADV_SPINLOCK
#error CVM_MICROLOCK_SWAP_SPINLOCK but not CVM_ADV_SPINLOCK
#endif
#ifndef CVM_ADV_ATOMIC_SWAP
#error CVM_MICROLOCK_SWAP_SPINLOCK but not ADV_ATOMIC_SWAP
#endif
#elif CVM_MICROLOCK_TYPE == CVM_MICROLOCK_DEFAULT
#elif CVM_MICROLOCK_TYPE == CVM_MICROLOCK_PLATFORM_SPECIFIC
#else
#error Unknown CVM_MICROLOCK_TYPE
#endif

#ifndef CVM_FASTLOCK_TYPE
#define CVM_FASTLOCK_TYPE CVM_FASTLOCK_NONE
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS
#ifndef CVM_ADV_ATOMIC_CMPANDSWAP
#error CVM_FASTLOCK_ATOMICOPS but not CVM_ADV_ATOMIC_CMPANDSWAP
#endif
#ifndef CVM_ADV_ATOMIC_SWAP
#error CVM_FASTLOCK_ATOMICOPS but not CVM_ADV_ATOMIC_SWAP
#endif
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_NONE
#else
#error Unknown CVM_FASTLOCK_TYPE
#endif

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
#if !(defined(CVM_ADV_MUTEX_SET_OWNER) || defined(CVM_ADV_THREAD_BOOST))
#error Must define CVM_ADV_MUTEX_SET_OWNER or CVM_ADV_THREAD_BOOST for fast locking
#endif
#if defined(CVM_ADV_THREAD_BOOST) && !defined(CVM_ADV_MUTEX_SET_OWNER)
#define CVM_THREAD_BOOST
#endif
#endif

#endif /* _INCLUDED_PORTING_SYNC_H */
