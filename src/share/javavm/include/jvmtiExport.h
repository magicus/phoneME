/*
 * @(#)jvmtiExport.h	1.4 06/10/27
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
   ints are implemented as structs in the porting layer, then this
   implementation will need to be revised. */

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

typedef struct CVMJvmtiThreadNode CVMJvmtiThreadNode;
struct CVMJvmtiThreadNode {
    CVMObjectICell* thread;        /* Global root; always allocated */
    jobject lastDetectedException; /* JNI Global Ref; allocated in
                                      CVMjvmtiPostExceptionEvent */
    jvmtiStartFunction startFunction;  /* for debug threads only */
    const void *startFunctionArg;      /* for debug threads only */

    CVMJvmtiContext *context;
    void *jvmtiPrivateData;    /* JVMTI thread-local data. */
    CVMClassBlock *oldCb;       /* current class being redefined */
    CVMClassBlock *redefineCb;  /* new classblock of redefined class */
    CVMBool startEventSent;
    CVMJvmtiThreadNode *next;
};

typedef struct CVMJvmtiGlobals CVMJvmtiGlobals;
struct CVMJvmtiGlobals {

    /* The following struct is based on JDK HotSpot's _jvmtiExports struct: */
    struct {
	int             fieldAccessCount;
	int             fieldModificationCount;

	jboolean        canGetSourceDebugExtension;
	jboolean        canExamineOrDeoptAnywhere;
	jboolean        canMaintainOriginalMethodOrder;
	jboolean        canPostInterpreterEvents;
	jboolean        canHotswapOrPostBreakpoint;
	jboolean        canModifyAnyClass;
	jboolean        canWalkAnySpace;
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
	jboolean        shouldPostObjectFree;
	jboolean        shouldCleanUpHeapObjects;
	jboolean        shouldPostVmObjectAlloc;    
	jboolean        hasRedefinedAClass;
	jboolean        allDependenciesAreRecorded;
    } exports;

    CVMBool dataDumpRequested;
    CVMBool isEnabled;
    CVMBool isInDebugMode;
    CVMBool debugOptionSet;  /* -Xdebug option passed in */

    /* Are one or more fields being watched?
     * These flags are accessed by the interpreter to determine if
     * jvmti should be notified.
     */
    CVMBool isWatchingFieldAccess;
    CVMBool isWatchingFieldModification;

    /* The following struct isfor storing statics used by the JVMTI
       implementation: */
    struct {
	/* event hooks, etc */
	JavaVM* vm;
	/* Not used: 
        jvmtiEventCallbacks *eventHook;
	JVMTIAllocHook allocHook;
	JVMTIDeallocHook deallocHook; 
	*/
	struct CVMBag* breakpoints;
	struct CVMBag* framePops;
	struct CVMBag* watchedFieldModifications;
	struct CVMBag* watchedFieldAccesses;
	volatile CVMJvmtiThreadNode *threadList;
	CVMJvmtiContext *context;

	/* From jvmtiExport.c */
	jvmtiPhase currentPhase;

	/* Not used:
        jlong      clks_per_sec;
        CVMBool    debuggerConnected = CVM_FALSE;
	CVMUint32  uniqueId = 0x10000;
	*/

	CVMJvmtiMethodNode *nodeByMB[HASH_SLOT_COUNT];

	/* From jvmtiEnv.c: */

	/* Not used:
	CVMJvmtiContext *RetransformableEnvironments;
	CVMJvmtiContext *NonRetransformableEnvironments;
	*/

	/* Used in jvmti_GetThreadInfo(): */
	jfieldID nameID;
	jfieldID priorityID;
	jfieldID daemonID;
	jfieldID groupID;
	jfieldID loaderID;

	/* Used in jvmti_GetThreadGroupInfo(): */
	jfieldID tgParentID;
	jfieldID tgNameID;
	jfieldID tgMaxPriorityID;
	jfieldID tgDaemonID;

	/* Used in jvmti_GetThreadGroupChildren(): */
	jfieldID nthreadsID;
	jfieldID threadsID;
	jfieldID ngroupsID;
	jfieldID groupsID;

	/* For Heap functions in jvmtiEnv.c: */
	CVMJvmtiVisitStack currentStack; /* for Heap functions. */
	CVMJvmtiTagNode *romObjects[HASH_SLOT_COUNT];

	/* From jvmtiDumper.c: */
	CVMJvmtiTagNode *objectsByRef[HASH_SLOT_COUNT];

    } statics;

    /* From jvmtiCapabilities.c: */
    struct {
	/* capabilities which are always potentially available */
	jvmtiCapabilities always;

	/* capabilities which are potentially available during OnLoad */
	jvmtiCapabilities onload;

	/* capabilities which are always potentially available */
	/* but to only one environment */
	jvmtiCapabilities always_solo;

	/* capabilities which are potentially available during OnLoad */
	/* but to only one environment */
	jvmtiCapabilities onload_solo;

	/* remaining capabilities which are always potentially available */
	/* but to only one environment */
	jvmtiCapabilities always_solo_remaining;

	/* remaining capabilities which are potentially available during
	 * OnLoad but to only one environment */
	jvmtiCapabilities onload_solo_remaining;

	/* all capabilities ever acquired */
	jvmtiCapabilities acquired;

    } capabilities;
};

/* The following functions are implemented as macros so that they can be
   inlined:

   NOTE: can* conditions (below) are set at OnLoad and never changed.
 */

void CVMjvmtiSetCanGetSourceDebugExtension(jboolean on);
#define CVMjvmtiSetCanGetSourceDebugExtension(on_) \
    (CVMglobals.jvmti.exports.canGetSourceDebugExtension = (on_))

void CVMjvmtiSetCanExamineOrDeoptAnywhere(jboolean on);
#define CVMjvmtiSetCanExamineOrDeoptAnywhere(on_) \
    (CVMglobals.jvmti.exports.canExamineOrDeoptAnywhere = (on_))

void CVMjvmtiSetCanMaintainOriginalMethodOrder(jboolean on);
#define CVMjvmtiSetCanMaintainOriginalMethodOrder(on_) \
    (CVMglobals.jvmti.exports.canMaintainOriginalMethodOrder = (on_))

void CVMjvmtiSetCanPostInterpreterEvents(jboolean on);
#define CVMjvmtiSetCanPostInterpreterEvents(on_) \
    (CVMglobals.jvmti.exports.canPostInterpreterEvents = (on_))

void CVMjvmtiSetCanHotswapOrPostBreakpoint(jboolean on);
#define CVMjvmtiSetCanHotswapOrPostBreakpoint(on_) \
    (CVMglobals.jvmti.exports.canHotswapOrPostBreakpoint = (on_));

void CVMjvmtiSetCanModifyAnyClass(jboolean on);
#define CVMjvmtiSetCanModifyAnyClass(on_) \
    (CVMglobals.jvmti.exports.canModifyAnyClass = (on_))

void CVMjvmtiSetCanWalkAnySpace(jboolean on);
#define CVMjvmtiSetCanWalkAnySpace(on_) \
    (CVMglobals.jvmti.exports.canWalkAnySpace = (on_))

void CVMjvmtiSetCanAccessLocalVariables(jboolean on);
#define CVMjvmtiSetCanAccessLocalVariables(on_) \
    (CVMglobals.jvmti.exports.canAccessLocalVariables = (on_))

void CVMjvmtiSetCanPostExceptions(jboolean on);
#define CVMjvmtiSetCanPostExceptions(on_) \
    (CVMglobals.jvmti.exports.canPostExceptions = (on_))

void CVMjvmtiSetCanPostBreakpoint(jboolean on);
#define CVMjvmtiSetCanPostBreakpoint(on_) \
    (CVMglobals.jvmti.exports.canPostBreakpoint = (on_))

void CVMjvmtiSetCanPostFieldAccess(jboolean on);
#define CVMjvmtiSetCanPostFieldAccess(on_) \
    (CVMglobals.jvmti.exports.canPostFieldAccess = (on_))

void CVMjvmtiSetCanPostFieldModification(jboolean on);
#define CVMjvmtiSetCanPostFieldModification(on_) \
    (CVMglobals.jvmti.exports.canPostFieldModification = (on_))

void CVMjvmtiSetCanPostMethodEntry(jboolean on);
#define CVMjvmtiSetCanPostMethodEntry(on_) \
    (CVMglobals.jvmti.exports.canPostMethodEntry = (on_))

void CVMjvmtiSetCanPostMethodExit(jboolean on);
#define CVMjvmtiSetCanPostMethodExit(on_) \
    (CVMglobals.jvmti.exports.canPostMethodExit = (on_))

void CVMjvmtiSetCanPopFrame(jboolean on);
#define CVMjvmtiSetCanPopFrame(on_) \
    (CVMglobals.jvmti.exports.canPopFrame = (on_))

void CVMjvmtiSetCanForceEarlyReturn(jboolean on);
#define CVMjvmtiSetCanForceEarlyReturn(on_) \
    (CVMglobals.jvmti.exports.canForceEarlyReturn = (on_))


void CVMjvmtiSetShouldPostSingleStep(jboolean on);
#define CVMjvmtiSetShouldPostSingleStep(on_) \
    (CVMglobals.jvmti.exports.shouldPostSingleStep = (on_))

void CVMjvmtiSetShouldPostFieldAccess(jboolean on);
#define CVMjvmtiSetShouldPostFieldAccess(on_) \
    (CVMglobals.jvmti.exports.shouldPostFieldAccess = (on_))

void CVMjvmtiSetShouldPostFieldModification(jboolean on);
#define CVMjvmtiSetShouldPostFieldModification(on_) \
    (CVMglobals.jvmti.exports.shouldPostFieldModification = (on_))

void CVMjvmtiSetShouldPostClassLoad(jboolean on);
#define CVMjvmtiSetShouldPostClassLoad(on_) \
    (CVMglobals.jvmti.exports.shouldPostClassLoad = (on_))

void CVMjvmtiSetShouldPostClassPrepare(jboolean on);
#define CVMjvmtiSetShouldPostClassPrepare(on_) \
    (CVMglobals.jvmti.exports.shouldPostClassPrepare = (on_))

void CVMjvmtiSetShouldPostClassUnload(jboolean on);
#define CVMjvmtiSetShouldPostClassUnload(on_) \
    (CVMglobals.jvmti.exports.shouldPostClassUnload = (on_))

void CVMjvmtiSetShouldPostClassFileLoadHook(jboolean on);
#define CVMjvmtiSetShouldPostClassFileLoadHook(on_) \
    (CVMglobals.jvmti.exports.shouldPostClassFileLoadHook = (on_))

void CVMjvmtiSetShouldPostNativeMethodBind(jboolean on);
#define CVMjvmtiSetShouldPostNativeMethodBind(on_) \
    (CVMglobals.jvmti.exports.shouldPostNativeMethodBind = (on_))

void CVMjvmtiSetShouldPostCompiledMethodLoad(jboolean on);
#define CVMjvmtiSetShouldPostCompiledMethodLoad(on_) \
    (CVMglobals.jvmti.exports.shouldPostCompiledMethodLoad = (on_))

void CVMjvmtiSetShouldPostCompiledMethodUnload(jboolean on);
#define CVMjvmtiSetShouldPostCompiledMethodUnload(on_) \
    (CVMglobals.jvmti.exports.shouldPostCompiledMethodUnload = (on_))

void CVMjvmtiSetShouldPostDynamicCodeGenerated(jboolean on);
#define CVMjvmtiSetShouldPostDynamicCodeGenerated(on_) \
    (CVMglobals.jvmti.exports.shouldPostDynamicCodeGenerated = (on_))

void CVMjvmtiSetShouldPostMonitorContendedEnter(jboolean on);
#define CVMjvmtiSetShouldPostMonitorContendedEnter(on_) \
    (CVMglobals.jvmti.exports.shouldPostMonitorContendedEnter = (on_))

void CVMjvmtiSetShouldPostMonitorContendedEntered(jboolean on);
#define CVMjvmtiSetShouldPostMonitorContendedEntered(on_) \
    (CVMglobals.jvmti.exports.shouldPostMonitorContendedEntered = (on_))

void CVMjvmtiSetShouldPostMonitorWait(jboolean on);
#define CVMjvmtiSetShouldPostMonitorWait(on_) \
    (CVMglobals.jvmti.exports.shouldPostMonitorWait = (on_))

void CVMjvmtiSetShouldPostMonitorWaited(jboolean on);
#define CVMjvmtiSetShouldPostMonitorWaited(on_) \
    (CVMglobals.jvmti.exports.shouldPostMonitorWaited = (on_))

void CVMjvmtiSetShouldPostGarbageCollectionStart(jboolean on);
#define CVMjvmtiSetShouldPostGarbageCollectionStart(on_) \
    (CVMglobals.jvmti.exports.shouldPostGarbageCollectionStart = (on_))

void CVMjvmtiSetShouldPostGarbageCollectionFinish(jboolean on);
#define CVMjvmtiSetShouldPostGarbageCollectionFinish(on_) \
    (CVMglobals.jvmti.exports.shouldPostGarbageCollectionFinish = (on_))

void CVMjvmtiSetShouldPostDataDump(jboolean on);
#define CVMjvmtiSetShouldPostDataDump(on_) \
    (CVMglobals.jvmti.exports.shouldPostDataDump = (on_))

void CVMjvmtiSetShouldPostObjectFree(jboolean on);
#define CVMjvmtiSetShouldPostObjectFree(on_) \
    (CVMglobals.jvmti.exports.shouldPostObjectFree = (on_))

void CVMjvmtiSetShouldPostVmObjectAlloc(jboolean on);
#define CVMjvmtiSetShouldPostVmObjectAlloc(on_) \
    (CVMglobals.jvmti.exports.shouldPostVmObjectAlloc = (on_))

void CVMjvmtiSetShouldPostThreadLife(jboolean on);
#define CVMjvmtiSetShouldPostThreadLife(on_) \
    (CVMglobals.jvmti.exports.shouldPostThreadLife = (on_))

void CVMjvmtiSetShouldCleanUpHeapObjects(jboolean on);
#define CVMjvmtiSetShouldCleanUpHeapObjects(on_) \
    (CVMglobals.jvmti.exports.shouldCleanUpHeapObjects = (on_))

jlong CVMjvmtiGetThreadEventEnabled(CVMExecEnv *ee);
#define CVMjvmtiGetThreadEventEnabled(ee_) \
    (CVMjvmtiEventEnabled(ee_).enabledBits)

void CVMjvmtiSetShouldPostAnyThreadEvent(CVMExecEnv *ee, jlong enabled);
#define CVMjvmtiSetShouldPostAnyThreadEvent(ee_, enabled_) \
    (CVMjvmtiEventEnabled(ee_).enabledBits = (enabled_))


enum {
    JVMTIVERSIONMASK   = 0x70000000,
    JVMTIVERSIONVALUE  = 0x30000000,
    JVMDIVERSIONVALUE  = 0x20000000
};


/* let JVMTI know that the VM isn't up yet (and JVMOnLoad code isn't running) */
void CVMjvmtiEnterPrimordialPhase();
#define CVMjvmtiEnterPrimordialPhase() \
    (CVMglobals.jvmti.statics.currentPhase = JVMTI_PHASE_PRIMORDIAL)

/* let JVMTI know that the JVMOnLoad code is running */
void CVMjvmtiEnterOnloadPhase();
#define CVMjvmtiEnterOnloadPhase() \
    (CVMglobals.jvmti.statics.currentPhase = JVMTI_PHASE_ONLOAD)

/* let JVMTI know that the VM isn't up yet but JNI is live */
void CVMjvmtiEnterStartPhase();
#define CVMjvmtiEnterStartPhase() \
    (CVMglobals.jvmti.statics.currentPhase = JVMTI_PHASE_START)

/* let JVMTI know that the VM is fully up and running now */
void CVMjvmtiEnterLivePhase();
#define  CVMjvmtiEnterLivePhase() \
    (CVMglobals.jvmti.statics.currentPhase = JVMTI_PHASE_LIVE)

/* let JVMTI know that the VM is dead, dead, dead.. */
void CVMjvmtiEnterDeadPhase();
#define CVMjvmtiEnterDeadPhase() \
    (CVMglobals.jvmti.statics.currentPhase = JVMTI_PHASE_DEAD)

jvmtiPhase CVMjvmtiGetPhase();
#define CVMjvmtiGetPhase() \
    (CVMglobals.jvmti.statics.currentPhase)

/* ------ can_* conditions (below) are set at OnLoad and never changed ------*/

jboolean CVMjvmtiCanGetSourceDebugExtension();
#define CVMjvmtiCanGetSourceDebugExtension() \
    (CVMglobals.jvmti.exports.canGetSourceDebugExtension)

/* BP, expression stack, hotswap, interpOnly, localVar, monitor info */
jboolean CVMjvmtiCanExamineOrDeoptAnywhere();
#define CVMjvmtiCanExamineOrDeoptAnywhere() \
    (CVMglobals.jvmti.exports.canExamineOrDeoptAnywhere)

/* JVMDI spec requires this, does this matter for JVMTI? */
jboolean CVMjvmtiCanMaintainOriginalMethodOrder();
#define CVMjvmtiCanMaintainOriginalMethodOrder() \
    (CVMglobals.jvmti.exports.canMaintainOriginalMethodOrder)

/* any of single-step, method-entry/exit, frame-pop, and field-access /
   modification */
jboolean CVMjvmtiCanPostInterpreterEvents();
#define CVMjvmtiCanPostInterpreterEvents() \
    (CVMglobals.jvmti.exports.canPostInterpreterEvents)

jboolean CVMjvmtiCanHotswapOrPostBreakpoint();
#define CVMjvmtiCanHotswapOrPostBreakpoint() \
    (CVMglobals.jvmti.exports.canHotswapOrPostBreakpoint)

jboolean CVMjvmtiCanModifyAnyClass();
#define CVMjvmtiCanModifyAnyClass() \
    (CVMglobals.jvmti.exports.canModifyAnyClass)

jboolean CVMjvmtiCanWalkAnySpace();
#define CVMjvmtiCanWalkAnySpace() \
    (CVMglobals.jvmti.exports.canWalkAnySpace)

/* can retrieve frames, set/get local variables or hotswap */
jboolean CVMjvmtiCanAccessLocalVariables();
#define CVMjvmtiCanAccessLocalVariables() \
    (CVMglobals.jvmti.exports.canAccessLocalVariables)

/* throw or catch */
jboolean CVMjvmtiCanPostExceptions();
#define CVMjvmtiCanPostExceptions() \
    (CVMglobals.jvmti.exports.canPostExceptions)

jboolean CVMjvmtiCanPostBreakpoint();
#define CVMjvmtiCanPostBreakpoint() \
    (CVMglobals.jvmti.exports.canPostBreakpoint)

jboolean CVMjvmtiCanPostFieldAccess();
#define CVMjvmtiCanPostFieldAccess() \
    (CVMglobals.jvmti.exports.canPostFieldAccess)

jboolean CVMjvmtiCanPostFieldModification();
#define CVMjvmtiCanPostFieldModification() \
    (CVMglobals.jvmti.exports.canPostFieldModification)

jboolean CVMjvmtiCanPostMethodEntry();
#define CVMjvmtiCanPostMethodEntry() \
    (CVMglobals.jvmti.exports.canPostMethodEntry)

jboolean CVMjvmtiCanPostMethodExit();
#define CVMjvmtiCanPostMethodExit() \
    (CVMglobals.jvmti.exports.canPostMethodExit)

jboolean CVMjvmtiCanPopFrame();
#define CVMjvmtiCanPopFrame() \
    (CVMglobals.jvmti.exports.canPopFrame)

jboolean CVMjvmtiCanForceEarlyReturn();
#define CVMjvmtiCanForceEarlyReturn() \
    (CVMglobals.jvmti.exports.canForceEarlyReturn)


/* the below maybe don't have to be (but are for now) fixed conditions here -*/
/* any events can be enabled */
jboolean CVMjvmtiShouldPostThreadLife();
#define CVMjvmtiShouldPostThreadLife() \
    (CVMglobals.jvmti.exports.shouldPostThreadLife)

/* ------ DYNAMIC conditions here ------------ */

jboolean CVMjvmtiShouldPostSingleStep();
#define CVMjvmtiShouldPostSingleStep() \
    (CVMglobals.jvmti.exports.shouldPostSingleStep)

jboolean CVMjvmtiShouldPostFieldAccess();
#define CVMjvmtiShouldPostFieldAccess() \
    (CVMglobals.jvmti.exports.shouldPostFieldAccess)

jboolean CVMjvmtiShouldPostFieldModification();
#define CVMjvmtiShouldPostFieldModification() \
    (CVMglobals.jvmti.exports.shouldPostFieldModification)

jboolean CVMjvmtiShouldPostClassLoad();
#define CVMjvmtiShouldPostClassLoad() \
    (CVMglobals.jvmti.exports.shouldPostClassLoad)

jboolean CVMjvmtiShouldPostClassPrepare();
#define CVMjvmtiShouldPostClassPrepare() \
    (CVMglobals.jvmti.exports.shouldPostClassPrepare)

jboolean CVMjvmtiShouldPostClassUnload();
#define CVMjvmtiShouldPostClassUnload() \
    (CVMglobals.jvmti.exports.shouldPostClassUnload)

jboolean CVMjvmtiShouldPostClassFileLoadHook();
#define CVMjvmtiShouldPostClassFileLoadHook() \
    (CVMglobals.jvmti.exports.shouldPostClassFileLoadHook)

jboolean CVMjvmtiShouldPostNativeMethodBind();
#define CVMjvmtiShouldPostNativeMethodBind() \
    (CVMglobals.jvmti.exports.shouldPostNativeMethodBind)

jboolean CVMjvmtiShouldPostCompiledMethodLoad();
#define CVMjvmtiShouldPostCompiledMethodLoad() \
    (CVMglobals.jvmti.exports.shouldPostCompiledMethodLoad)

jboolean CVMjvmtiShouldPostCompiledMethodUnload();
#define CVMjvmtiShouldPostCompiledMethodUnload() \
    (CVMglobals.jvmti.exports.shouldPostCompiledMethodUnload)

jboolean CVMjvmtiShouldPostDynamicCodeGenerated();
#define CVMjvmtiShouldPostDynamicCodeGenerated() \
    (CVMglobals.jvmti.exports.shouldPostDynamicCodeGenerated)

jboolean CVMjvmtiShouldPostMonitorContendedEnter();
#define CVMjvmtiShouldPostMonitorContendedEnter() \
    (CVMglobals.jvmti.exports.shouldPostMonitorContendedEnter)

jboolean CVMjvmtiShouldPostMonitorContendedEntered();
#define CVMjvmtiShouldPostMonitorContendedEntered() \
    (CVMglobals.jvmti.exports.shouldPostMonitorContendedEntered)

jboolean CVMjvmtiShouldPostMonitorWait();
#define CVMjvmtiShouldPostMonitorWait() \
    (CVMglobals.jvmti.exports.shouldPostMonitorWait)

jboolean CVMjvmtiShouldPostMonitorWaited();
#define CVMjvmtiShouldPostMonitorWaited() \
    (CVMglobals.jvmti.exports.shouldPostMonitorWaited)

jboolean CVMjvmtiShouldPostDataDump();
#define CVMjvmtiShouldPostDataDump() \
    (CVMglobals.jvmti.exports.shouldPostDataDump)

jboolean CVMjvmtiShouldPostGarbageCollectionStart();
#define CVMjvmtiShouldPostGarbageCollectionStart() \
    (CVMglobals.jvmti.exports.shouldPostGarbageCollectionStart)

jboolean CVMjvmtiShouldPostGarbageCollectionFinish();
#define CVMjvmtiShouldPostGarbageCollectionFinish() \
    (CVMglobals.jvmti.exports.shouldPostGarbageCollectionFinish)

jboolean CVMjvmtiShouldPostObjectFree();
#define CVMjvmtiShouldPostObjectFree() \
    (CVMglobals.jvmti.exports.shouldPostObjectFree)

jboolean CVMjvmtiShouldPostVmObjectAlloc();
#define CVMjvmtiShouldPostVmObjectAlloc() \
    (CVMglobals.jvmti.exports.shouldPostVmObjectAlloc)

/* ----------------- */

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


#define CVMjvmtiIsEnabled() \
    (CVMglobals.jvmti.isEnabled)
#define CVMjvmtiSetIsEnabled(isEnabled_) \
    (CVMglobals.jvmti.isEnabled = (isEnabled_))

#define CVMjvmtiHasDebugOptionSet() \
    (CVMglobals.jvmti.debugOptionSet)
#define CVMjvmtiSetDebugOption(debugOptionSet_) \
    (CVMglobals.jvmti.debugOptionSet = (debugOptionSet_))
#define CVMjvmtiIsInDebugMode() \
    (CVMassert((CVMglobals.jvmti.isInDebugMode &&	\
		CVMglobals.jvmti.isEnabled) ||		\
	       !CVMglobals.jvmti.isInDebugMode),	\
     CVMglobals.jvmti.isInDebugMode)
#define CVMjvmtiSetIsInDebugMode(isInDebugMode_) \
    (CVMglobals.jvmti.isInDebugMode = (isInDebugMode_))

#define CVMjvmtiIsWatchingFieldAccess() \
    (CVMglobals.jvmti.isWatchingFieldAccess)
#define CVMjvmtiSetIsWatchingFieldAccess(isWatching_) \
    (CVMglobals.jvmti.isWatchingFieldAccess = (isWatching_))

#define CVMjvmtiIsWatchingFieldModification() \
    (CVMglobals.jvmti.isWatchingFieldModification)
#define CVMjvmtiSetIsWatchingFieldModification(isWatching_) \
    (CVMglobals.jvmti.isWatchingFieldModification = (isWatching_))


enum CVMJvmtiLoadKind {
  JVMTICLASSLOADKINDNORMAL   = 0,
  JVMTICLASSLOADKINDREDEFINE,
  JVMDICLASSLOADKINDRETRANSFORM
};
typedef enum CVMJvmtiLoadKind CVMJvmtiLoadKind;

typedef struct CVMJvmtiLockInfo CVMJvmtiLockInfo;
struct CVMJvmtiLockInfo {
    CVMJvmtiLockInfo *next;
    CVMOwnedMonitor *lock;
};

typedef struct CVMJvmtiExecEnv CVMJvmtiExecEnv;
struct CVMJvmtiExecEnv {
    CVMBool debugEventsEnabled;
    CVMBool jvmtiSingleStepping;
    CVMBool jvmtiNeedFramePop;
    CVMBool jvmtiNeedEarlyReturn;
    CVMBool jvmtiDataDumpRequested;
    CVMBool jvmtiNeedProcessing;
    CVMJvmtiEventEnabled jvmtiUserEventEnabled;
    CVMJvmtiEventEnabled jvmtiEventEnabled;
    jvalue jvmtiEarlyReturnValue;
    CVMUint32 jvmtiEarlyRetOpcode;
    CVMJvmtiLockInfo *jvmtiLockInfoFreelist;

    /* NOTE: The first pass at implementing JVMTI support will have only one
       global environment i.e. CVMJvmtiContext.  Hence, we store the context
       in CVMJvmtiExports instead of here in the thread jvmtiEE.
    */
    /* CVMJvmtiContext *context; */
    void *jvmtiProfilerData;    /* JVMTI Profiler thread-local data. */
};

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

#define CVMjvmtiEventTypeDisable(ee_, eventType_)               \
    (CVMjvmtiEventEnabled(ee_).enabledBits &=                   \
     ~(((jlong)1) << CVMjvmtiEvent2EventBit((eventType_))))

#define CVMjvmtiEventTypeEnable(ee_, eventType_)                \
    (CVMjvmtiEventEnabled(ee_).enabledBits |=                   \
     (((jlong)1) << CVMjvmtiEvent2EventBit((eventType_))))

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
    (CVMjvmtiIsEnabled() && CVMjvmtiDebugEventsEnabled(ee_) &&		\
     ((CVMglobals.jvmti.statics.context->				\
       envEventEnable.eventEnabled.enabledBits &			\
       (((jlong)1) << CVMjvmtiEvent2EventBit(eventType_)))) != 0)

#define CVMjvmtiThreadEventEnabled(ee_, eventType_)			\
    (((ee_) != NULL) && CVMjvmtiIsEnabled() &&                          \
     CVMjvmtiDebugEventsEnabled(ee_) &&                                 \
     ((CVMjvmtiEventEnabled(ee_).enabledBits &                          \
       (((jlong)1) << CVMjvmtiEvent2EventBit(eventType_)))) != 0)


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
    (CVMglobals.jvmti.dataDumpRequested = CVM_TRUE)

/* Purpose: Check if a data dump was requested. */
CVMBool CVMjvmtiDataDumpWasRequested(void);
#define CVMjvmtiDataDumpWasRequested() \
    (CVMglobals.jvmti.dataDumpRequested)

/* Purpose: Clear the pending data dump event request. */
void CVMjvmtiResetDataDumpRequested(void);
#define CVMjvmtiResetDataDumpRequested() \
    (CVMglobals.jvmti.dataDumpRequested = CVM_FALSE)

/* TODO: The CVMjvmtiIsEnabled() check can be eliminated if an obsolete bit can
   be stored in the mb itself. */
#define CVMjvmtiMbIsObsolete(mb)  \
    (CVMjvmtiIsEnabled() && CVMjvmtiMbIsObsoleteX(mb))

/* Gets a jvmtiEnv * JVMTI environment pointer to enable JVMTI work.
 * This function is used by CVMjniGetEnv.
 */
jint CVMjvmtiGetInterface(JavaVM *interfacesVm, void **penv);

/* Initialized and destroys JVMTI global state at VM startu/shutdown: */
void CVMjvmtiInitializeGlobals(CVMJvmtiGlobals *globals);
void CVMjvmtiDestroyGlobals(CVMJvmtiGlobals *globals);

/* Starts up or shuts down JVMTI: */
jvmtiError CVMjvmtiInitialize(JavaVM *vm);
void CVMjvmtiDestroy(CVMJvmtiGlobals *globals);

CVMJvmtiThreadNode *
CVMjvmtiFindThread(CVMExecEnv* ee, CVMObjectICell* thread);

CVMJvmtiThreadNode *
CVMjvmtiInsertThread(CVMExecEnv* ee, CVMObjectICell* thread);

jboolean    CVMjvmtiRemoveThread(CVMExecEnv* ee, CVMObjectICell *thread);

jvmtiError  CVMjvmtiAllocate(jlong size, unsigned char **mem);
jvmtiError  CVMjvmtiDeallocate(unsigned char *mem);
CVMBool     CVMjvmtiClassBeingRedefined(CVMExecEnv *ee, CVMClassBlock *cb);

/* See also CVMjvmtiClassRef2ClassBlock() in jvmtiEnv.h.
   CVMjvmtiClassObject2ClassBlock() takes a direct class object as input while
   CVMjvmtiClassRef2ClassBlock() takes a class ref i.e.  jclass.
*/
CVMClassBlock *CVMjvmtiClassObject2ClassBlock(CVMExecEnv *ee, CVMObject *obj);

void        CVMjvmtiRehash(void);
CVMUint32   CVMjvmtiUniqueID();
void        CVMjvmtiMarkAsObsolete(CVMMethodBlock *oldmb);
CVMBool     CVMjvmtiMbIsObsoleteX(CVMMethodBlock *mb);
CVMConstantPool * CVMjvmtiMbConstantPool(CVMMethodBlock *mb);
CVMBool     CVMjvmtiCheckLockInfo(CVMExecEnv *ee);
void        CVMjvmtiAddLockInfo(CVMExecEnv *ee, CVMObjMonitor *mon,
                                CVMOwnedMonitor *o,
                                CVMBool okToBecomeGCSafe);
void        CVMjvmtiRemoveLockInfo(CVMExecEnv *ee, CVMObjMonitor *mon,
                                   CVMOwnedMonitor *o);
CVMClassBlock* CVMjvmtiGetCurrentRedefinedClass(CVMExecEnv *ee);
void        CVMjvmtiDestroyLockInfo(CVMExecEnv *ee);
#endif   /* CVM_JVMTI */
#endif   /* INCLUDED_JVMTI_EXPORT_H */
