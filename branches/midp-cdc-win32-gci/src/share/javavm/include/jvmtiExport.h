/*
 * @(#)jvmtiExport.h	1.4 06/10/27
 *
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
 *
 */

#ifndef _INCLUDED_JVMTIEXPORT_H
#define _INCLUDED_JVMTIEXPORT_H

#ifdef CVM_JVMTI

#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/stdlib.h"

#include "javavm/include/defs.h"
#include "javavm/export/jni.h"
#include "javavm/export/jvmti.h"
#include "javavm/include/jvmtiEnv.h"

/* This class contains the JVMTI interface for the rest of CVM. */

/* bits for standard events */
/* TODO: This bit encoding assumes the availability of 64bit integers.  If 64bit
   ints are implemented as structs in the porting layer, then this implementation
   will need to be revised. */

#define CVMjvmtiEvent2EventBit(x)   ((x) - JVMTI_MIN_EVENT_TYPE_VAL)
#define CVMjvmtiEventBit(x)   CVMjvmtiEvent2EventBit(JVMTI_EVENT_##x)

#define SINGLE_STEP_BIT \
    (((jlong)1) << CVMjvmtiEventBit(SINGLE_STEP))
#define	 FRAME_POP_BIT	\
    (((jlong)1) << CVMjvmtiEventBit(FRAME_POP))
#define	  BREAKPOINT_BIT \
    (((jlong)1) << CVMjvmtiEventBit(BREAKPOINT))
#define	  FIELD_ACCESS_BIT \
    (((jlong)1) << CVMjvmtiEventBit(FIELD_ACCESS))
#define	  FIELD_MODIFICATION_BIT \
    (((jlong)1) << CVMjvmtiEventBit(FIELD_MODIFICATION))
#define	  METHOD_ENTRY_BIT \
    (((jlong)1) << CVMjvmtiEventBit(METHOD_ENTRY))
#define	  METHOD_EXIT_BIT \
    (((jlong)1) << CVMjvmtiEventBit(METHOD_EXIT))
#define	  CLASS_FILE_LOAD_HOOK_BIT \
    (((jlong)1) << CVMjvmtiEventBit(CLASS_FILE_LOAD_HOOK))
#define	  NATIVE_METHOD_BIND_BIT \
    (((jlong)1) << CVMjvmtiEventBit(NATIVE_METHOD_BIND))
#define	  VM_START_BIT \
    (((jlong)1) << CVMjvmtiEventBit(VM_START))
#define	  VM_INIT_BIT \
    (((jlong)1) << CVMjvmtiEventBit(VM_INIT))
#define	  VM_DEATH_BIT \
    (((jlong)1) << CVMjvmtiEventBit(VM_DEATH))
#define	  CLASS_LOAD_BIT \
    (((jlong)1) << CVMjvmtiEventBit(CLASS_LOAD))
#define	  CLASS_PREPARE_BIT \
    (((jlong)1) << CVMjvmtiEventBit(CLASS_PREPARE))
#define	  THREAD_START_BIT \
    (((jlong)1) << CVMjvmtiEventBit(THREAD_START))
#define	  THREAD_END_BIT \
    (((jlong)1) << CVMjvmtiEventBit(THREAD_END))
#define	  EXCEPTION_THROW_BIT \
    (((jlong)1) << CVMjvmtiEventBit(EXCEPTION))
#define	  EXCEPTION_CATCH_BIT \
    (((jlong)1) << CVMjvmtiEventBit(EXCEPTION_CATCH))
#define	  MONITOR_CONTENDED_ENTER_BIT \
    (((jlong)1) << CVMjvmtiEventBit(MONITOR_CONTENDED_ENTER))
#define	  MONITOR_CONTENDED_ENTERED_BIT \
    (((jlong)1) << CVMjvmtiEventBit(MONITOR_CONTENDED_ENTERED))
#define	  MONITOR_WAIT_BIT \
    (((jlong)1) << CVMjvmtiEventBit(MONITOR_WAIT))
#define	  MONITOR_WAITED_BIT \
    (((jlong)1) << CVMjvmtiEventBit(MONITOR_WAITED))
#define	  DYNAMIC_CODE_GENERATED_BIT \
    (((jlong)1) << CVMjvmtiEventBit(DYNAMIC_CODE_GENERATED))
#define	  DATA_DUMP_BIT \
    (((jlong)1) << CVMjvmtiEventBit(DATA_DUMP_REQUEST))
#define	  COMPILED_METHOD_LOAD_BIT \
    (((jlong)1) << CVMjvmtiEventBit(COMPILED_METHOD_LOAD))
#define	  COMPILED_METHOD_UNLOAD_BIT \
    (((jlong)1) << CVMjvmtiEventBit(COMPILED_METHOD_UNLOAD))
#define	  GARBAGE_COLLECTION_START_BIT \
    (((jlong)1) << CVMjvmtiEventBit(GARBAGE_COLLECTION_START))
#define	  GARBAGE_COLLECTION_FINISH_BIT \
    (((jlong)1) << CVMjvmtiEventBit(GARBAGE_COLLECTION_FINISH))
#define	  OBJECT_FREE_BIT \
    (((jlong)1) << CVMjvmtiEventBit(OBJECT_FREE))
#define	  VM_OBJECT_ALLOC_BIT \
    (((jlong)1) << CVMjvmtiEventBit(VM_OBJECT_ALLOC))

/* Bit masks for groups of JVMTI events: */

#define	 MONITOR_BITS \
    (MONITOR_CONTENDED_ENTER_BIT | MONITOR_CONTENDED_ENTERED_BIT | \
        MONITOR_WAIT_BIT | MONITOR_WAITED_BIT)
#define	 EXCEPTION_BITS \
    (EXCEPTION_THROW_BIT | EXCEPTION_CATCH_BIT)
#define	 INTERP_EVENT_BITS \
    (SINGLE_STEP_BIT | METHOD_ENTRY_BIT | METHOD_EXIT_BIT | \
        FRAME_POP_BIT | FIELD_ACCESS_BIT | FIELD_MODIFICATION_BIT)
#define	 THREAD_FILTERED_EVENT_BITS \
    (INTERP_EVENT_BITS | EXCEPTION_BITS | MONITOR_BITS | \
        BREAKPOINT_BIT | CLASS_LOAD_BIT | CLASS_PREPARE_BIT | THREAD_END_BIT)
#define NEED_THREAD_LIFE_EVENTS \
    (THREAD_FILTERED_EVENT_BITS | THREAD_START_BIT)
#define	 EARLY_EVENT_BITS \
    (CLASS_FILE_LOAD_HOOK_BIT | VM_START_BIT | VM_INIT_BIT | VM_DEATH_BIT | \
        NATIVE_METHOD_BIND_BIT | THREAD_START_BIT | THREAD_END_BIT | \
        DYNAMIC_CODE_GENERATED_BIT;)


typedef struct {
    int             fieldAccessCount;
    int             fieldModificationCount;

    jboolean        canGetSourceDebugExtension;
    jboolean        canExamineOrDeoptAnywhere;
    jboolean        canMaintainOriginalMethodOrder;
    jboolean        canPostInterpreterEvents;
    jboolean        canHotswapOrPostBreakpoint;
    jboolean        canModifyAnyClass;
    jboolean	    canWalkAnySpace;
    jboolean        canAccessLocalVariables;
    jboolean        canPostExceptions;
    jboolean        canPostBreakpoint;
    jboolean        canPostFieldAccess;
    jboolean        canPostFieldModification;
    jboolean        canPostMethodEntry;
    jboolean        canPostMethodExit;
    jboolean        canPopFrame;
    jboolean        canForceEarlyReturn;

    jboolean        shouldPostSingleStep;
    jboolean        shouldPostFieldAccess;
    jboolean        shouldPostFieldModification;
    jboolean        shouldPostClassLoad;
    jboolean        shouldPostClassPrepare;
    jboolean        shouldPostClassUnload;
    jboolean        shouldPostClassFileLoadHook;
    jboolean        shouldPostNativeMethodBind;
    jboolean        shouldPostCompiledMethodLoad;
    jboolean        shouldPostCompiledMethodUnload;
    jboolean        shouldPostDynamicCodeGenerated;
    jboolean        shouldPostMonitorContendedEnter;
    jboolean        shouldPostMonitorContendedEntered;
    jboolean        shouldPostMonitorWait;
    jboolean        shouldPostMonitorWaited;
    jboolean        shouldPostDataDump;
    jboolean        shouldPostGarbageCollectionStart;
    jboolean        shouldPostGarbageCollectionFinish;
    jboolean        shouldPostThreadLife;
    jboolean	    shouldPostObjectFree;
    jboolean        shouldCleanUpHeapObjects;
    jboolean        shouldPostVmObjectAlloc;    
    jboolean        hasRedefinedAClass;
    jboolean        allDependenciesAreRecorded;
} _jvmtiExports;

extern _jvmtiExports jvmtiExports;

void CVMjvmtiSetCanGetSourceDebugExtension(jboolean on);
void CVMjvmtiSetCanExamineOrDeoptAnywhere(jboolean on);
void CVMjvmtiSetCanMaintainOriginalMethodOrder(jboolean on);
void CVMjvmtiSetCanPostInterpreterEvents(jboolean on);
void CVMjvmtiSetCanHotswapOrPostBreakpoint(jboolean on);
void CVMjvmtiSetCanModifyAnyClass(jboolean on);
void CVMjvmtiSetCanWalkAnySpace(jboolean on);
void CVMjvmtiSetCanAccessLocalVariables(jboolean on);
void CVMjvmtiSetCanPostExceptions(jboolean on);
void CVMjvmtiSetCanPostBreakpoint(jboolean on);
void CVMjvmtiSetCanPostFieldAccess(jboolean on);
void CVMjvmtiSetCanPostFieldModification(jboolean on);
void CVMjvmtiSetCanPostMethodEntry(jboolean on);
void CVMjvmtiSetCanPostMethodExit(jboolean on);
void CVMjvmtiSetCanPopFrame(jboolean on);
void CVMjvmtiSetCanForceEarlyReturn(jboolean on);

void CVMjvmtiSetShouldPostSingleStep(jboolean on);
void CVMjvmtiSetShouldPostFieldAccess(jboolean on);
void CVMjvmtiSetShouldPostFieldModification(jboolean on);
void CVMjvmtiSetShouldPostClassLoad(jboolean on);
void CVMjvmtiSetShouldPostClassPrepare(jboolean on);
void CVMjvmtiSetShouldPostClassUnload(jboolean on);
void CVMjvmtiSetShouldPostClassFileLoadHook(jboolean on);
void CVMjvmtiSetShouldPostNativeMethodBind(jboolean on);
void CVMjvmtiSetShouldPostCompiledMethodLoad(jboolean on);
void CVMjvmtiSetShouldPostCompiledMethodUnload(jboolean on);
void CVMjvmtiSetShouldPostDynamicCodeGenerated(jboolean on);
void CVMjvmtiSetShouldPostMonitorContendedEnter(jboolean on);
void CVMjvmtiSetShouldPostMonitorContendedEntered(jboolean on);
void CVMjvmtiSetShouldPostMonitorWait(jboolean on);
void CVMjvmtiSetShouldPostMonitorWaited(jboolean on);
void CVMjvmtiSetShouldPostGarbageCollectionStart(jboolean on);
void CVMjvmtiSetShouldPostGarbageCollectionFinish(jboolean on);
void CVMjvmtiSetShouldPostDataDump(jboolean on);
void CVMjvmtiSetShouldPostObjectFree(jboolean on);
void CVMjvmtiSetShouldPostVmObjectAlloc(jboolean on);

void CVMjvmtiSetShouldPostThreadLife(jboolean on);
void CVMjvmtiSetShouldCleanUpHeapObjects(jboolean on);
jlong CVMjvmtiGetThreadEventEnabled(CVMExecEnv *ee);
void CVMjvmtiSetShouldPostAnyThreadEvent(CVMExecEnv *ee, jlong enabled);


enum {
  JVMTIVERSIONMASK   = 0x70000000,
  JVMTIVERSIONVALUE  = 0x30000000,
  JVMDIVERSIONVALUE  = 0x20000000
};


/* let JVMTI know that the VM isn't up yet (and JVMOnLoad code isn't running) */
void CVMjvmtiEnterPrimordialPhase();

/* let JVMTI know that the JVMOnLoad code is running */
void CVMjvmtiEnterOnloadPhase();

/* let JVMTI know that the VM isn't up yet but JNI is live */
void CVMjvmtiEnterStartPhase();

/* let JVMTI know that the VM is fully up and running now */
void CVMjvmtiEnterLivePhase();

/* let JVMTI know that the VM is dead, dead, dead.. */
void CVMjvmtiEnterDeadPhase();

jvmtiPhase CVMjvmtiGetPhase();

/* ------ can_* conditions (below) are set at OnLoad and never changed ----------*/

jboolean canGetSourceDebugExtension();

/* BP, expression stack, hotswap, interpOnly, localVar, monitor info */
jboolean canExamineOrDeoptAnywhere();

/* JVMDI spec requires this, does this matter for JVMTI? */
jboolean canMaintainOriginalMethodOrder();

/* any of single-step, method-entry/exit, frame-pop, and field-access/modification */
jboolean jvmtiCanPostInterpreterEvents();

jboolean jvmtiCanHotswapOrPostBreakpoint();

jboolean jvmtiCanModifyAnyClass();

jboolean jvmtiCanWalkAnySpace();

/* can retrieve frames, set/get local variables or hotswap */
jboolean jvmtiCanAccessLocalVariables();

/* throw or catch */
jboolean jvmtiCanPostExceptions();

jboolean jvmtiCanPostBreakpoint();
jboolean jvmtiCanPostFieldAccess();
jboolean jvmtiCanPostFieldModification();
jboolean jvmtiCanPostMethodEntry();
jboolean jvmtiCanPostMethodExit();
jboolean jvmtiCanPopFrame();
jboolean jvmtiCanForceEarlyReturn();


/* the below maybe don't have to be (but are for now) fixed conditions here ----*/
/* any events can be enabled */
jboolean CVMjvmtiShouldPostThreadLife();


/* ------ DYNAMIC conditions here ------------ */

jboolean CVMjvmtiShouldPostSingleStep();
jboolean CVMjvmtiShouldPostFieldAccess();
jboolean CVMjvmtiShouldPostFieldModification();
jboolean CVMjvmtiShouldPostClassLoad();
jboolean CVMjvmtiShouldPostClassPrepare();
jboolean CVMjvmtiShouldPostClassUnload();
jboolean CVMjvmtiShouldPostClassFileLoadHook();
jboolean CVMjvmtiShouldPostNativeMethodBind();
jboolean CVMjvmtiShouldPostCompiledMethodLoad();
jboolean CVMjvmtiShouldPostCompiledMethodUnload();
jboolean CVMjvmtiShouldPostDynamicCodeGenerated();
jboolean CVMjvmtiShouldPostMonitorContendedEnter();
jboolean CVMjvmtiShouldPostMonitorContendedEntered();
jboolean CVMjvmtiShouldPostMonitorWait();
jboolean CVMjvmtiShouldPostMonitorWaited();
jboolean CVMjvmtiShouldPostDataDump();
jboolean CVMjvmtiShouldPostGarbageCollectionStart();
jboolean CVMjvmtiShouldPostGarbageCollectionFinish();
jboolean CVMjvmtiShouldPostObjectFree();
jboolean CVMjvmtiShouldPostVmObjectAlloc();

/* ----------------- */

jboolean isJvmtiVersion(jint version);
jboolean isJvmdiVersion(jint version);
jint getJvmtiInterface(JavaVM *jvm, void **penv, jint version);
  
/* SetNativeMethodPrefix support */
char** getAllNativeMethodPrefixes(int* countPtr);

/* call after CMS has completed referencing processing */
void cmsRefProcessingEpilogue();

typedef struct AttachOperation_ {
  char *args;
} AttachOperation;

/*
 * Definitions for the debugger events and operations.
 * Only for use when CVMJVMTI is defined.
 */


/* This #define is necessary to prevent the declaration of 2 jvmti static
   variables (in jvmti.h) which is not needed in the VM: */
#ifndef NOJVMTIMACROS
#define NOJVMTIMACROS
#endif

#undef JVMTI_LOCK
#undef JVMTI_UNLOCK
#undef JVMTI_IS_LOCKED
#define JVMTI_LOCK(ee)      CVM_JVMTI_LOCK(ee)
#define JVMTI_UNLOCK(ee)    CVM_JVMTI_UNLOCK(ee)
#define JVMTI_IS_LOCKED(ee) CVM_JVMTI_IS_LOCKED(ee)

#define CVM_JVMTI_LOCK(ee)   \
	CVMsysMutexLock(ee, &CVMglobals.jvmtiLock)
#define CVM_JVMTI_UNLOCK(ee) \
	CVMsysMutexUnlock(ee, &CVMglobals.jvmtiLock)
#define CVM_JVMTI_IS_LOCKED(ee)					\
         CVMreentrantMutexIAmOwner(ee,					\
	   CVMsysMutexGetReentrantMutex(&CVMglobals.jvmtiLock))


typedef struct ThreadNode_ {
    CVMObjectICell* thread;        /* Global root; always allocated */
    jobject lastDetectedException; /* JNI Global Ref; allocated in
                                      CVMjvmtiPostExceptionEvent */
    jvmtiStartFunction startFunction;  /* for debug threads only */
    const void *startFunctionArg;      /* for debug threads only */
    JvmtiEnv *env;
    void *jvmtiPrivateData;    /* JVMTI thread-local data. */
    CVMClassBlock *oldCb;       /* current class being redefined */
    CVMClassBlock *redefineCb;  /* new classblock of redefined class */
    CVMBool startEventSent;
    struct ThreadNode_ *next;
} ThreadNode;

typedef struct CVMjvmtiStatics {
  /* event hooks, etc */
  JavaVM* vm;
  jvmtiEventCallbacks *eventHook;
  /*  JVMTIAllocHook allocHook; */
  /*  JVMTIDeallocHook deallocHook; */
  struct CVMBag* breakpoints;
  struct CVMBag* framePops;
  struct CVMBag* watchedFieldModifications;
  struct CVMBag* watchedFieldAccesses;
  volatile ThreadNode *threadList;
  JvmtiEnv *jvmtiEnv;
} JVMTI_Static;

typedef struct CVMJvmtiRecord {
    CVMBool dataDumpRequested;
}CVMJvmtiRecord;

typedef enum {
  JVMTICLASSLOADKINDNORMAL   = 0,
  JVMTICLASSLOADKINDREDEFINE,
  JVMDICLASSLOADKINDRETRANSFORM
}jvmtiLoadKind;

typedef struct jvmtiLockInfo {
    struct jvmtiLockInfo *next;
    CVMOwnedMonitor *lock;
} JvmtiLockInfo;

typedef JvmtiLockInfo CVMjvmtiLockInfo;

typedef struct JvmtiExecEnv {
    CVMBool debugEventsEnabled;
    CVMBool jvmtiSingleStepping;
    CVMBool jvmtiNeedFramePop;
    CVMBool jvmtiNeedEarlyReturn;
    CVMBool jvmtiDataDumpRequested;
    CVMBool jvmtiNeedProcessing;
    JvmtiEventEnabled jvmtiUserEventEnabled;
    JvmtiEventEnabled jvmtiEventEnabled;
    jvalue jvmtiEarlyReturnValue;
    CVMUint32 jvmtiEarlyRetOpcode;
    jvmtiLoadKind jvmtiClassLoadKind;
    JvmtiLockInfo *jvmtiLockInfoFreelist;

    /* NOTE: first pass at JVMTI support has only one global environment */
    /*   JvmtiEnv *JvmtiEnv; */
    void *jvmtiProfilerData;    /* JVMTI Profiler thread-local data. */
} CVMJVMTIExecEnv;

#define CVMjvmtiEnabled()			\
    (CVMglobals.jvmtiEnabled)

#define CVMjvmtiDebuggingFlag()			\
    CVMglobals.jvmtiDebuggingFlag

#define CVMjvmtiSetProcessingCheck(ee_)					\
    if (CVMjvmtiNeedFramePop(ee_) || CVMjvmtiNeedEarlyReturn(ee_) ||	\
	CVMjvmtiSingleStepping(ee_)) {					\
	CVMjvmtiNeedProcessing(ee_) = CVM_TRUE;				\
    } else {								\
	CVMjvmtiNeedProcessing(ee_) = CVM_FALSE;			\
    }

#define CVMjvmtiSingleStepping(ee_)		\
    ((ee_)->jvmtiEE.jvmtiSingleStepping)

#define CVMjvmtiDebugEventsEnabled(ee_)		\
    ((ee_)->jvmtiEE.debugEventsEnabled)

#define CVMjvmtiUserEventEnabled(ee_)		\
    ((ee_)->jvmtiEE.jvmtiUserEventEnabled)

#define CVMjvmtiEventEnabled(ee_)		\
    ((ee_)->jvmtiEE.jvmtiEventEnabled)

#define CVMjvmtiReturnOpcode(ee_)		\
    ((ee_)->jvmtiEE.jvmtiEarlyRetOpcode)

#define CVMjvmtiReturnValue(ee_)		\
    ((ee_)->jvmtiEE.jvmtiEarlyReturnValue)

#define CVMjvmtiNeedFramePop(ee_)		\
    ((ee_)->jvmtiEE.jvmtiNeedFramePop)

#define CVMjvmtiClearFramePop(ee_) {			\
    ((ee_)->jvmtiEE.jvmtiNeedFramePop = CVM_FALSE);	\
    CVMjvmtiSetProcessingCheck(ee_);			\
    }

#define CVMjvmtiNeedEarlyReturn(ee_)		\
    ((ee_)->jvmtiEE.jvmtiNeedEarlyReturn)

#define CVMjvmtiClearEarlyReturn(ee_) {			\
    ((ee_)->jvmtiEE.jvmtiNeedEarlyReturn = CVM_FALSE);	\
    CVMjvmtiSetProcessingCheck(ee_);			\
    }

#define CVMjvmtiNeedProcessing(ee_)		\
    ((ee_)->jvmtiEE.jvmtiNeedProcessing)

#define CVMjvmtiEnvEventEnabled(ee_, eventType_)			\
    (CVMjvmtiEnabled() && CVMjvmtiDebugEventsEnabled(ee_) &&		\
     ((((CVMglobals.jvmtiStatics.jvmtiEnv)->envEventEnable.eventEnabled.enabledBits) & \
       (((jlong)1) << CVMjvmtiEvent2EventBit(eventType_))) != 0))

#define CVMjvmtiThreadEventEnabled(ee_, eventType_)			\
    ((ee_ != NULL) &&							\
     ((CVMjvmtiEventEnabled(ee_).enabledBits &				\
       (((jlong)1) << CVMjvmtiEvent2EventBit(eventType_))) != 0))

#define CVMjvmtiShouldPostObjectFree()		\
    CVMjvmtiEnabled() && jvmtiExports.shouldPostObjectFree

/* Purpose: Indicate that we have started a GC cycle (independent of whether
            actual GC'ing has been blocked or not). */
void CVMjvmtiSetGCWasStarted(void);
#define CVMjvmtiSetGCWasStarted() \
    (CVMjvmtiRec()->gcWasStarted = CVM_TRUE)

/* Purpose: Indicate that we have ended a GC cycle which was started. */
void CVMjvmtiResetGCWasStarted(void);
#define CVMjvmtiResetGCWasStarted() \
    (CVMjvmtiRec()->gcWasStarted = CVM_FALSE)

/* Purpose: Check if we have started a GC cycle. */
CVMBool CVMjvmtiGCWasStarted(void);
#define CVMjvmtiGCWasStarted() \
    (CVMjvmtiRec()->gcWasStarted)

/* Purpose: Gets the global CVMJvmtiRecord. */
#define CVMjvmtiRec()   (&CVMglobals.jvmtiRecord)

#define CVMJVMTI_CHECK_PHASE(x) {	     \
	if (CVMjvmtiGetPhase() != (x)) {     \
	    return JVMTI_ERROR_WRONG_PHASE;  \
	}				     \
    }

#define CVMJVMTI_CHECK_PHASE2(x, y) {	     \
	if (CVMjvmtiGetPhase() != (x) &&     \
	    CVMjvmtiGetPhase() != (y)) {     \
	    return JVMTI_ERROR_WRONG_PHASE;  \
	}				     \
    }
#define CVMJVMTI_CHECK_PHASEV(x) {	     \
	if (CVMjvmtiGetPhase() != (x)) {     \
	    return;			     \
	}				     \
    }

#define CVMJVMTI_CHECK_PHASEV2(x, y) {	     \
	if (CVMjvmtiGetPhase() != (x) &&     \
	    CVMjvmtiGetPhase() != (y)) {     \
	    return;  \
	}				     \
    }

/*
 * This section for use by the interpreter.
 *
 * NOTE: All of these routines are implicitly gc-safe. When the
 * interpreter calls them, the call site must be wrapped in an
 * CVMDGcSafeExec() block.
 */

void CVMjvmtiPostExceptionEvent(CVMExecEnv* ee,
				CVMUint8* pc,
				CVMObjectICell* object);
void CVMjvmtiPostExceptionCatchEvent(CVMExecEnv* ee,
				     CVMUint8* pc,
				     CVMObjectICell* object);
void CVMjvmtiPostSingleStepEvent(CVMExecEnv* ee,
				 CVMUint8* pc);
/** OBJ is expected to be NULL for static fields */
void CVMjvmtiPostFieldAccessEvent(CVMExecEnv* ee,
				  CVMObjectICell* obj,
				  CVMFieldBlock* fb);
/** OBJ is expected to be NULL for static fields */
void CVMjvmtiPostFieldModificationEvent(CVMExecEnv* ee,
					CVMObjectICell* obj,
					CVMFieldBlock* fb,
					       jvalue jval);
void CVMjvmtiPostThreadStartEvent(CVMExecEnv* ee,
				  CVMObjectICell* thread);
void CVMjvmtiPostThreadEndEvent(CVMExecEnv* ee,
				CVMObjectICell* thread);
/* The next two are a bit of a misnomer. If requested, the JVMTI
   reports three types of events to the caller: method entry (for all
   methods), method exit (for all methods), and frame pop (for a
   single frame identified in a previous call to notifyFramePop). The
   "frame push" method below may cause a method entry event to be sent
   to the debugging agent. The "frame pop" method below may cause a
   method exit event, frame pop event, both, or neither to be sent to
   the debugging agent. Also note that
   CVMjvmtiPostFramePopEvent (originally
   notifyDebuggerOfFramePop) checks for a thrown exception
   internally in order to allow the calling code to be sloppy about
   ensuring that events aren't sent if an exception occurred. */
void CVMjvmtiPostFramePushEvent(CVMExecEnv* ee);
void CVMjvmtiPostFramePopEvent(CVMExecEnv* ee, CVMBool isRef,
			       CVMBool isException, jvalue *retValue);
void CVMjvmtiPostClassLoadEvent(CVMExecEnv* ee,
				CVMObjectICell* clazz);
void CVMjvmtiPostClassPrepareEvent(CVMExecEnv* ee,
				   CVMObjectICell* clazz);
void CVMjvmtiPostClassUnloadEvent(CVMExecEnv* ee,
				  CVMObjectICell* clazz);
void CVMjvmtiPostVmStartEvent(CVMExecEnv* ee);
void CVMjvmtiPostVmInitEvent(CVMExecEnv* ee);
void CVMjvmtiPostVmExitEvent(CVMExecEnv* ee);

CVMUint8 CVMjvmtiGetBreakpointOpcode(CVMExecEnv* ee, CVMUint8* pc,
				     CVMBool notify);
CVMBool CVMjvmtiSetBreakpointOpcode(CVMExecEnv* ee, CVMUint8* pc,
				    CVMUint8 opcode);
void CVMjvmtiStaticsInit(struct CVMjvmtiStatics * statics);
void CVMjvmtiStaticsDestroy(struct CVMjvmtiStatics * statics);

void CVMjvmtiPostClassLoadHookEvent(jclass klass,
				    CVMClassLoaderICell *loader,
				    const char *className,
				    jobject protectionDomain,
				    CVMInt32 bufferLength,
				    CVMUint8 *buffer,
				    CVMInt32 *newBufferLength,
				    CVMUint8 **newBuffer);
void CVMjvmtiPostCompiledMethodLoadEvent(CVMExecEnv *ee,
					 CVMMethodBlock *mb);
void CVMjvmtiPostCompiledMethodUnloadEvent(CVMExecEnv *ee,
					   CVMMethodBlock* mb);
void CVMjvmtiPostDataDumpRequest(void);
void CVMjvmtiPostGCStartEvent(void);
void CVMjvmtiPostGCFinishEvent(void);
void CVMjvmtiPostStartUpEvents(CVMExecEnv *ee);
void CVMjvmtiPostNativeMethodBind(CVMExecEnv *ee, CVMMethodBlock *mb,
				  CVMUint8 *nativeCode,
				  CVMUint8 **newNativeCode);
void CVMjvmtiPostMonitorContendedEnterEvent(CVMExecEnv *ee,
					    CVMProfiledMonitor *pm);
void CVMjvmtiPostMonitorContendedEnteredEvent(CVMExecEnv *ee,
                                              CVMProfiledMonitor *pm);
void CVMjvmtiPostMonitorWaitEvent(CVMExecEnv *ee,
				  jobject obj, jlong millis);
void CVMjvmtiPostMonitorWaitedEvent(CVMExecEnv *ee,
				    jobject obj, CVMBool timedout);

void CVMjvmtiPostObjectFreeEvent(CVMObject *obj);

#define CVMjvmtiSetDataDumpRequested() \
    (CVMjvmtiRec()->dataDumpRequested = CVM_TRUE)

/* Purpose: Check if a data dump was requested. */
CVMBool CVMjvmtiDataDumpWasRequested(void);
#define CVMjvmtiDataDumpWasRequested() \
    (CVMjvmtiRec()->dataDumpRequested)

/* Purpose: Clear the pending data dump event request. */
void CVMjvmtiResetDataDumpRequested(void);
#define CVMjvmtiResetDataDumpRequested() \
    (CVMjvmtiRec()->dataDumpRequested = CVM_FALSE)

#define CVMjvmtiMbIsObsolete(mb)  \
    (CVMjvmtiEnabled() && CVMjvmtiMbIsObsoleteX(mb))
/*
 * This function is used by CVMjniGetEnv.
 */
jvmtiInterface_1* CVMjvmtiGetInterface1(JavaVM* interfacesVm);

jint        CVMcreateJvmti(JavaVM *interfacesVm, void **penv);
jvmtiError  CVMinitializeJVMTI();
ThreadNode *CVMjvmtiFindThread(CVMExecEnv* ee, CVMObjectICell* thread);
ThreadNode *CVMjvmtiInsertThread(CVMExecEnv* ee, CVMObjectICell* thread);
jboolean    CVMjvmtiRemoveThread(CVMObjectICell *thread);

jvmtiError  CVMjvmtiAllocate(jlong size, unsigned char **mem);
jvmtiError  CVMjvmtiDeallocate(unsigned char *mem);
CVMBool     CVMjvmtiClassBeingRedefined(CVMExecEnv *ee, CVMClassBlock *cb);
CVMClassBlock *CVMjvmtiClassInstance2ClassBlock(CVMExecEnv *ee,
						CVMObject *obj);
void        CVMjvmtiRehash(void);
CVMUint32   CVMjvmtiUniqueID();
void        CVMjvmtiMarkAsObsolete(CVMMethodBlock *oldmb, CVMConstantPool *cp);
CVMBool     CVMjvmtiMbIsObsoleteX(CVMMethodBlock *mb);
CVMConstantPool * CVMjvmtiMbConstantPool(CVMMethodBlock *mb);
void        CVMjvmtiAddLock(CVMExecEnv *ee, CVMOwnedMonitor *o);
void        CVMjvmtiAddMon(CVMExecEnv *ee, CVMObjMonitor *mon);
void        CVMjvmtiRemoveLock(CVMExecEnv *ee, CVMOwnedMonitor *o);
void        CVMjvmtiRemoveMon(CVMExecEnv *ee, CVMObjMonitor *mon);
CVMClassBlock* CVMjvmtiGetCurrentRedefinedClass(CVMExecEnv *ee);

#endif   /* CVM_JVMTI */
#endif   /* INCLUDED_JVMTI_EXPORT_H */
