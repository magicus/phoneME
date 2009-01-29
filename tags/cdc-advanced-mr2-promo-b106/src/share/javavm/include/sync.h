/*
 * @(#)sync.h	1.62 06/10/10
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

#ifndef _INCLUDED_SYNC_H
#define _INCLUDED_SYNC_H

#include "javavm/include/defs.h"
#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/sync.h"

#if CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK
#define CVM_MICROLOCK_UNLOCKED  0x00
#define CVM_MICROLOCK_LOCKED    0xff
#endif

#ifndef _ASM

/*
 * Global micro locks
 */

typedef enum {
    CVM_CODE_MICROLOCK,
#ifdef CVM_CLASSLOADING
    CVM_CONSTANTPOOL_MICROLOCK,
#endif
#ifdef CVM_JIT
    CVM_COMPILEFLAGS_MICROLOCK,
#ifdef CVM_CCM_COLLECT_STATS
    CVM_CCM_STATS_MICROLOCK,
#endif /* CVM_CCM_COLLECT_STATS */
#endif /* CVM_JIT */
#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
    CVM_TOOLS_MICROLOCK,
#endif
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)
    CVM_GC_LOCKER_MICROLOCK,
#endif
#if defined(CVM_TRACE_ENABLED)
    CVM_METHOD_TRACE_MICROLOCK,
#endif
    CVM_ACCESS_VOLATILE_MICROLOCK,
    CVM_NUM_SYS_MICROLOCKS /* This enum should always be the last one. */
} CVMSysMicroLock;

void
CVMsysMicroLock(CVMExecEnv *ee, CVMSysMicroLock lock);
void
CVMsysMicroUnlock(CVMExecEnv *ee, CVMSysMicroLock lock);
void
CVMsysMicroLockAll(CVMExecEnv *ee);
void
CVMsysMicroUnlockAll(CVMExecEnv *ee);

#ifndef CVM_DEBUG
#define CVMsysMicroLock(ee, l)   {(void)(ee); CVMsysMicroLockImpl((l));}
#define CVMsysMicroUnlock(ee, l) {(void)(ee); CVMsysMicroUnlockImpl((l));}
#endif

#define CVMsysMicroLockImpl(l) \
    CVMmicrolockLock(&CVMglobals.sysMicroLock[(l)])
#define CVMsysMicroUnlockImpl(l) \
    CVMmicrolockUnlock(&CVMglobals.sysMicroLock[(l)])

/*********************************************
 * Various CVM_MICROLOCK_TYPE implementations.
 *********************************************/

#if CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SCHEDLOCK

/*
 * CVM_MICROLOCK_SCHEDLOCK
 */
struct CVMMicroLock { CVMUint8 unused; };
#define CVMmicrolockInit(m)	   (CVM_TRUE)
#define CVMmicrolockDestroy(m)
#define CVMmicrolockLock(m)        CVMschedLock()
#define CVMmicrolockUnlock(m)      CVMschedUnlock()

#elif CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK

/*
 * CVM_MICROLOCK_SWAP_SPINLOCK
 */

struct CVMMicroLock
{
    volatile CVMAddr lockWord;
};

extern void CVMmicrolockLockImpl(CVMMicroLock *m);

#define CVMmicrolockLock(m) {                                          \
    CVMAddr                                                            \
    oldWord = CVMatomicSwap(&(m)->lockWord, CVM_MICROLOCK_LOCKED);     \
    if (oldWord != CVM_MICROLOCK_UNLOCKED) {                           \
        CVMmicrolockLockImpl(m);                                       \
    }                                                                  \
}
#define CVMmicrolockUnlock(m) \
    CVMvolatileStore(&(m)->lockWord, CVM_MICROLOCK_UNLOCKED)

#elif CVM_MICROLOCK_TYPE == CVM_MICROLOCK_DEFAULT

/*
 * CVM_MICROLOCK_DEFAULT
 */
/* Microlocks will be implemented using CVMMutex. */
#define CVMMicroLock            CVMMutex
#define CVMmicrolockInit        CVMmutexInit
#define CVMmicrolockDestroy     CVMmutexDestroy
#define CVMmicrolockLock        CVMmutexLock
#define CVMmicrolockUnlock      CVMmutexUnlock

#elif CVM_MICROLOCK_TYPE == CVM_MICROLOCK_PLATFORM_SPECIFIC

/*
 * CVM_MICROLOCK_PLATFORM_SPECIFIC
 */
/* Platform will supply implementation */
#else

#error "Invalid CVM_MICROLOCK_TYPE"

#endif /* CVM_MICROLOCK_TYPE */

#define CVM_ACCESS_VOLATILE_LOCK(ee) \
    CVMsysMicroLock(ee, CVM_ACCESS_VOLATILE_MICROLOCK)

#define CVM_ACCESS_VOLATILE_UNLOCK(ee) \
    CVMsysMicroUnlock(ee, CVM_ACCESS_VOLATILE_MICROLOCK)

/*
 * Reentrant mutexes
 */

struct CVMReentrantMutex {
    CVMExecEnv *owner;
    CVMUint32 count;
    CVMMutex mutex;
};

extern CVMBool CVMreentrantMutexInit   (CVMReentrantMutex *rm,
					CVMExecEnv *owner, CVMUint32 count);
extern void    CVMreentrantMutexDestroy(CVMReentrantMutex *rm);
extern CVMBool CVMreentrantMutexTryLock(CVMExecEnv *ee, CVMReentrantMutex *rm);
extern void    CVMreentrantMutexLock   (CVMExecEnv *ee, CVMReentrantMutex *rm);
extern void    CVMreentrantMutexUnlock (CVMExecEnv *ee, CVMReentrantMutex *rm);
/*
 * CVM_FALSE is returned to indicate that the wait was interrupted,
 * otherwise CVM_TRUE is returned.
 */
extern CVMBool CVMreentrantMutexWait   (CVMExecEnv *ee, CVMReentrantMutex *rm,
					CVMCondVar *c, CVMInt64 millis);

/* Returns TRUE if the execution environment EE currently owns the
   Reentrant Mutex RM */
extern CVMBool CVMreentrantMutexIAmOwner(CVMExecEnv *ee, CVMReentrantMutex *rm);
/* This macro returns an (CVMMutex *) */
#define CVMreentrantMutexGetMutex(/* CVMReentrantMutex * */ m) (&((m)->mutex))
#define CVMreentrantMutexOwner(/* CVMReentrantMutex * */ m) ((m)->owner)
#define CVMreentrantMutexCount(/* CVMReentrantMutex * */ m) ((m)->count)
#ifdef CVM_THREAD_BOOST
#define CVMreentrantMutexAcquire(ee, m) {			\
	CVMBool success = CVMmutexTryLock(&(m)->mutex);	\
	CVMassert(success), (void)success;			\
	CVMassert((m)->owner == (ee));				\
	CVMassert((m)->count > 0);				\
}
#else
#define CVMreentrantMutexSetOwner(ee, m, targetEE)	\
	CVMmutexSetOwner(CVMexecEnv2threadID((ee)),	\
	    &(m)->mutex, CVMexecEnv2threadID((targetEE)))
#endif

#define CVMreentrantMutexLockUnsafe(ee, rm)	\
{						\
    CVMassert(CVMD_isgcUnsafe(ee));		\
    if ((rm)->owner == ee) {			\
        ++(rm)->count;				\
    } else {					\
	CVMD_gcSafeExec((ee), {			\
	    CVMmutexLock(&(rm)->mutex);		\
	});					\
        (rm)->owner = ee;			\
        (rm)->count = 1;			\
    }						\
}

/*
 * Deadlock-detecting VM mutexes
 */

struct CVMSysMutex {
    const char *name;
    CVMUint8 rank;
    CVMReentrantMutex rmutex;
#ifdef CVM_DEBUG
    CVMSysMutex *nextOwned;
#endif
};

#define CVM_SYSMUTEX_MAX_RANK  255

extern CVMBool    CVMsysMutexInit   (CVMSysMutex* m,
				     const char *name,	/* client managed */
				     CVMUint8 rank);	/* deadlock detection */
extern void       CVMsysMutexDestroy(CVMSysMutex* m);

extern CVMBool    CVMsysMutexTryLock(CVMExecEnv *ee, CVMSysMutex* m);
extern void       CVMsysMutexLock   (CVMExecEnv *ee, CVMSysMutex* m);
extern void       CVMsysMutexUnlock (CVMExecEnv *ee, CVMSysMutex* m);
/*
 * CVM_FALSE is returned to indicate that the wait was interrupted,
 * otherwise CVM_TRUE is returned.
 */
extern CVMBool	  CVMsysMutexWait   (CVMExecEnv *ee, CVMSysMutex* m,
				     CVMCondVar *c, CVMInt64 millis);

/* This macro returns a (CVMReentrantMutex *) */
#define CVMsysMutexGetReentrantMutex(/* CVMSysMutex * */ m) \
	(&((m)->rmutex))
#define CVMsysMutexOwner(m) \
	CVMreentrantMutexOwner(&(m)->rmutex)
#define CVMsysMutexCount(m) \
	CVMreentrantMutexCount(&(m)->rmutex)
#define CVMsysMutexIAmOwner(ee, m) \
	(CVMsysMutexOwner((m)) == (ee))

/*
 * "raw" object-less monitor
 */

typedef enum {
    CVM_WAIT_OK,
    CVM_WAIT_INTERRUPTED,
    CVM_WAIT_NOT_OWNER
} CVMWaitStatus;

struct CVMSysMonitor {
    CVMReentrantMutex rmutex;
    CVMCondVar condvar;
};

extern CVMBool
CVMsysMonitorInit(CVMSysMonitor* m, CVMExecEnv *owner, CVMUint32 count);

extern void
CVMsysMonitorDestroy(CVMSysMonitor* m);

extern void
CVMsysMonitorEnter(CVMExecEnv *ee, CVMSysMonitor* m);

extern void
CVMsysMonitorExit(CVMExecEnv *ee, CVMSysMonitor* m);

extern CVMBool
CVMsysMonitorNotify(CVMExecEnv *ee, CVMSysMonitor* m);

extern CVMBool
CVMsysMonitorNotifyAll(CVMExecEnv *ee, CVMSysMonitor* m);

extern CVMWaitStatus
CVMsysMonitorWait(CVMExecEnv *ee, CVMSysMonitor* m, CVMInt64 millis);

#define CVMsysMonitorOwner(m) CVMreentrantMutexOwner(&(m)->rmutex)
#define CVMsysMonitorCount(m) CVMreentrantMutexCount(&(m)->rmutex)

#ifdef CVM_THREAD_BOOST
#define CVMsysMonitorAcquireMutex(ee, m) \
	CVMreentrantMutexAcquire((ee), &(m)->rmutex)
#else
#define CVMsysMonitorSetMutexOwner(ee, m, o) \
	CVMreentrantMutexSetOwner((ee), &(m)->rmutex, (o))
#endif

#define CVMsysMonitorTryEnter(ee, m)			\
	CVMreentrantMutexTryLock((ee), &(m)->rmutex)

#define CVMsysMonitorEnterUnsafe(ee, m)			\
{							\
    CVMreentrantMutexLockUnsafe((ee), &(m)->rmutex);	\
}

#define CVMsysMonitorEnterSafe(ee, m)			\
{							\
    CVMassert(CVMD_isgcSafe((ee)));			\
    CVMreentrantMutexLock((ee), &(m)->rmutex);		\
}

#define CVMsysMonitorEnter(ee, m)	CVMsysMonitorEnterSafe((ee), (m))

#define CVMsysMonitorExit(ee, m)			\
{							\
    CVMreentrantMutexUnlock((ee), &(m)->rmutex);	\
}

/*===========================================================================*/

#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
/* CVM lock type necessary for JVMPI/JVMTI: */
enum {
    CVM_LOCKTYPE_UNKNOWN    = 0, /* Reserved to indicate an illegal type. */
    CVM_LOCKTYPE_OBJ_MONITOR,
    CVM_LOCKTYPE_NAMED_SYSMONITOR
};
#endif

struct CVMProfiledMonitor {
    CVMSysMonitor _super;
#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
    CVMUint8 type;
    CVMUint32 contentionCount;
    CVMProfiledMonitor *next;
    CVMProfiledMonitor **previousPtr;
#endif
};

#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
/* Purpose: Constructor. */
extern CVMBool
CVMprofiledMonitorInit(CVMProfiledMonitor *self, CVMExecEnv *owner,
                       CVMUint32 count, CVMUint8 lockType);
#else
/* inline CVMBool CVMprofiledMonitorInit(CVMProfiledMonitor *self,
            CVMExecEnv *owner, CVMUint32 count)); */
#define CVMprofiledMonitorInit(self, owner, count, lockType) \
    CVMsysMonitorInit(&(self)->_super, (owner), (count))
#endif

/* Purpose: Destructor. */
/* inline void CVMprofiledMonitorDestroy(CVMProfiledMonitor *self); */
#define CVMprofiledMonitorDestroy(self) \
    CVMsysMonitorDestroy(&(self)->_super)

#if defined(CVM_JVMPI) || defined(CVM_JVMTI)

/* Purpose: Gets the type of the CVMProfiledMonitor. */
/* inline CVMUint8 CVMprofiledMonitorGetType(CVMProfiledMonitor *self); */
#define CVMprofiledMonitorGetType(self) \
    ((self)->type)

/* Purpose: Sets the type of the CVMProfiledMonitor. */
/* inline void CVMprofiledMonitorGetType(CVMProfiledMonitor *self,
            CVMUint8 type); */
#define CVMprofiledMonitorSetType(self, _type) \
    ((self)->type = (_type))

#endif /* CVM_JVMPI */

/* Purpose: Enters the monitor. */
/* inline void CVMprofiledMonitorEnter(CVMProfiledMonitor *self,
            CVMExecEnv *currentEE); */
#define CVMprofiledMonitorEnter(self, currentEE, isRaw)	\
    CVMprofiledMonitorEnterSafe((self), (currentEE), (isRaw))

/* Purpose: Exits the monitor. */
#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
extern void
CVMprofiledMonitorExit(CVMProfiledMonitor *self, CVMExecEnv *currentEE);
#else /* !CVM_JVMPI */
/* inline void CVMprofiledMonitorExit(CVMProfiledMonitor *self,
            CVMExecEnv *currentEE); */
#define CVMprofiledMonitorExit(self, currentEE) \
    CVMsysMonitorExit((currentEE), (&(self)->_super))
#endif /* CVM_JVMPI */

/* Purpose: Notifies the next thread waiting on this monitor. */
/* Returns: If the current_ee passed in is not the owner of the monitor,
            CVM_FALSE will be returned.  Else, returns CVM_TRUE. */
/* inline CVMBool CVMprofiledMonitorNotify(CVMProfiledMonitor *self,
            CVMExecEnv *currentEE); */
#define CVMprofiledMonitorNotify(self, currentEE) \
    CVMsysMonitorNotify((currentEE), (&(self)->_super))

/* Purpose: Notifies all threads waiting on this monitor. */
/* Returns: If the current_ee passed in is not the owner of the monitor,
            CVM_FALSE will be returned.  Else, returns CVM_TRUE. */
/* inline CVMBool CVMprofiledMonitorNotifyAll(CVMProfiledMonitor *self,
            CVMExecEnv *currentEE); */
#define CVMprofiledMonitorNotifyAll(self, currentEE) \
    CVMsysMonitorNotifyAll((currentEE), (&(self)->_super))

/* Purpose: Wait for the specified time period (in milliseconds).  If the
            period is 0, then wait forever. */
/* Returns: CVM_WAIT_NOT_OWNER if current_ee is not the owner of the monitor,
            CVM_WAIT_OK if the thread was woken up by a timeout, and
            CVM_WAIT_INTERRUPTED is the thread was woken up by a notify. */
/* NOTE:    Thread must be in a GC safe state before calling this. */
#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
extern CVMWaitStatus
CVMprofiledMonitorWait(CVMProfiledMonitor *self, CVMExecEnv *currentEE,
                       CVMInt64 millis);
#else /* !CVM_JVMPI */
/* inline CVMWaitStatus CVMprofiledMonitorWait(CVMProfiledMonitor *self,
            CVMExecEnv *currentEE, CVMInt64 millis); */
#define CVMprofiledMonitorWait(self, currentEE, millis) \
    CVMsysMonitorWait((currentEE), (&(self)->_super), (millis))
#endif /* CVM_JVMPI */

/* Purpose: Gets the current owner ee of this monitor. */
/* inline CVMExecEnv *CVMprofiledMonitorGetOwner(CVMProfiledMonitor *self); */
#define CVMprofiledMonitorGetOwner(self) \
    CVMsysMonitorOwner(&(self)->_super)

/* Purpose: Gets the current lock count of this monitor. */
/* inline CVMUint32 CVMprofiledMonitorGetCount(CVMProfiledMonitor *self); */
#define CVMprofiledMonitorGetCount(self) \
    CVMsysMonitorCount(&(self)->_super)

#ifdef CVM_THREAD_BOOST
/* Purpose: Acquire the underlying mutex of this monitor, ignoring
            owner and entry count. */
/* inline CVMUint32 CVMprofiledMonitorAcquireMutex(CVMExecEnv *currentEE,
	    CVMProfiledMonitor *pmon); */
#define CVMprofiledMonitorAcquireMutex(currentEE, pmon) \
    CVMsysMonitorAcquireMutex((currentEE), (&(pmon)->_super))
#else
/* Purpose: Sets the current owner threadID in the underlying mutex of this
            monitor. */
/* inline CVMUint32 CVMprofiledMonitorSetMutexOwner(CVMProfiledMonitor *self,
            CVMExecEnv *currentEE, CVMExecEnv *ownerEE); */
#define CVMprofiledMonitorSetMutexOwner(self, currentEE, ownerEE) \
    CVMsysMonitorSetMutexOwner((currentEE), (&(self)->_super), (ownerEE))
#endif

/* Purpose: Tries to enter the monitor. */
/* inline CVMBool CVMprofiledMonitorTryEnter(CVMProfiledMonitor *self,
            CVMExecEnv *currentEE); */
#define CVMprofiledMonitorTryEnter(self, currentEE) \
    CVMsysMonitorTryEnter((currentEE), (&(self)->_super))

/* Purpose: Enters monitor while thread is in a GC unsafe state. */
#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
extern void
CVMprofiledMonitorEnterUnsafe(CVMProfiledMonitor *self,
			      CVMExecEnv *currentEE, CVMBool doPost);
#ifdef CVM_THREAD_BOOST
extern void
CVMprofiledMonitorPreContendedEnterUnsafe(CVMProfiledMonitor *self,
    CVMExecEnv *currentEE);
extern void
CVMprofiledMonitorContendedEnterUnsafe(CVMProfiledMonitor *self,
    CVMExecEnv *currentEE);
#endif
#else /* !CVM_JVMPI */
/* inline void CVMprofiledMonitorEnterUnsafe(CVMProfiledMonitor *self,
            CVMExecEnv *currentEE); */
#define CVMprofiledMonitorEnterUnsafe(self, currentEE, doPost)		\
    CVMsysMonitorEnterUnsafe((currentEE), (&(self)->_super))
#ifdef CVM_THREAD_BOOST
#define CVMprofiledMonitorPreContendedEnterUnsafe(self, currentEE)
#define CVMprofiledMonitorContendedEnterUnsafe(self, currentEE) \
    CVMsysMonitorEnterUnsafe((currentEE), (&(self)->_super))
#endif
#endif /* CVM_JVMPI */

/* Purpose: Enters monitor while thread is in a GC safe state. */
#if defined(CVM_JVMPI) || defined(CVM_JVMTI)
extern void
CVMprofiledMonitorEnterSafe(CVMProfiledMonitor *self, CVMExecEnv *currentEE, CVMBool doPost);
#else /* !CVM_JVMPI */
/* inline void CVMprofiledMonitorEnterSafe(CVMProfiledMonitor *self,
            CVMExecEnv *currentEE); */
#define CVMprofiledMonitorEnterSafe(self, currentEE, doPost)		\
    CVMsysMonitorEnterSafe((currentEE), (&(self)->_super))
#endif /* CVM_JVMPI */

/*===========================================================================*/

/*
 * GC support
 */

extern void
CVMdeleteUnreferencedMonitors(CVMExecEnv *ee,
			      CVMRefLivenessQueryFunc isLive,
			      void* isLiveData,
			      CVMRefCallbackFunc transitiveScanner,
			      void* transitiveScannerData);


extern void
CVMscanMonitors(CVMExecEnv *ee, CVMRefCallbackFunc refCallback, void* data);

extern CVMBool
CVMeeSyncInit(CVMExecEnv *ee);
extern void
CVMeeSyncDestroy(CVMExecEnv *ee);
extern CVMBool
CVMeeSyncInitGlobal(CVMExecEnv *ee, CVMGlobalState *gs);
extern void
CVMeeSyncDestroyGlobal(CVMExecEnv *ee, CVMGlobalState *gs);

/*
 * Support for debug stubs.
 */

#if defined(CVM_PORTING_DEBUG_STUBS) && !defined(CVM_PORTING_DEBUG_IMPL)

#undef CVMmutexInit
#define CVMmutexInit(m) \
        CVMmutexInitStub((m))
CVMBool CVMmutexInitStub(CVMMutex* m);

#undef CVMmutexDestroy
#define CVMmutexDestroy(m)      \
        CVMmutexDestroyStub((m))
CVMBool CVMmutexDestroyStub(CVMMutex* m);

#undef CVMcondvarInit
#define CVMcondvarInit(c,m)     \
        CVMcondvarInitStub((c),(m))
CVMBool CVMcondvarInitStub(CVMCondVar* c, CVMMutex* m);

#undef CVMcondvarDestroy
#define CVMcondvarDestroy(c)    \
        CVMcondvarDestroyStub((c))
CVMBool CVMcondvarDestroyStub(CVMCondVar* c);

#endif

#endif /* !_ASM */
#endif /* _INCLUDED_SYNC_H */
