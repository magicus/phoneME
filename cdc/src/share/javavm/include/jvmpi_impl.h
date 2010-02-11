/*
 * @(#)jvmpi_impl.h	1.16 06/10/10
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights  
 * Reserved.  Use is subject to license terms.  
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

/*
 * Copyright 2005 Intel Corporation. All rights reserved.  
 */

#ifndef _INCLUDED_JVMPI_IMPL_H
#define _INCLUDED_JVMPI_IMPL_H

/*
 * This header file (originally JDK 1.3 Classic's "vmprofiler.h") defines the
 * internal VM implementation of JVMPI types and functions that the
 * interpreter, garbage collector, etc. should call to notify the profiling
 * agent of events.
 *
 * CVM source code using these functions should #include "jvmpi_impl.h", which
 * itself #includes "export/jvmpi.h".  Client source code (i.e., the profiling
 * agent) should #include "jvmpi.h" as usual.
 */

#ifdef CVM_JVMPI

#include "javavm/include/objects.h"
#include "javavm/include/interpreter.h"
#include "javavm/export/jni.h"
#include "javavm/export/jvmpi.h"

/* VM specific data types and methods used exclusively for the support of
   JVMPI: */

/* ================================================ Class CVMJvmpiEventInfo:
    Purpose: For tracking the status of the JVMPI events.
*/

typedef struct CVMJvmpiEventInfo CVMJvmpiEventInfo;
struct CVMJvmpiEventInfo {
    CVMInt8 eventInfoX[JVMPI_MAX_EVENT_TYPE_VAL + 1];
};

enum { /* For the eventInfoX elements in CVMJvmpiEventInfo: */
    CVMJvmpiEventInfo_NOT_AVAILABLE = -1, /* Cannot be disabled nor enabled. */
    CVMJvmpiEventInfo_DISABLED      = 0,
    CVMJvmpiEventInfo_ENABLED       = 1
};

/* Purpose: Checks to see if the specified event is enabled. */
/* NOTE: Instead of comparing against "== CVMJvmpiEventInfo_ENABLED", we are
        using "> CVMJvmpiEventInfo_DISABLED" for optimization reasons. */
#define CVMjvmpiEventInfoIsEnabled(event) \
    (CVMjvmpiRec()->eventInfo.eventInfoX[event] > CVMJvmpiEventInfo_DISABLED)

/* =================================================== Class CVMJvmpiRecord: */

typedef struct CVMJvmpiRecord CVMJvmpiRecord;
struct CVMJvmpiRecord {
    JVMPI_Interface interfaceX;
    CVMGCLocker gcLocker;
    CVMBool gcWasStarted;
    CVMBool dataDumpRequested;
    CVMJvmpiEventInfo eventInfo; /* Tracks Event Enabled/Disabled status. */
};

/* Purpose: Gets the global CVMJvmpiRecord. */
#define CVMjvmpiRec()   (&CVMglobals.jvmpiRecord)

/*=============================================================================
    Methods for checking if a certain JVMPI event has been enabled by the
    profiler agent for notification by the VM:
    NOTE: These methods are to be called from any VM code which need to check
          if a certain JVMPI event is enabled so as to post the needed JVMPI
          event.
*/

/* Purpose: Checks to see if JVMPI_EVENT_METHOD_ENTRY and/or
            JVMPI_EVENT_METHOD_ENTRY2 is enabled. */
CVMBool CVMjvmpiEventMethodEntryIsEnabled(void);
#define CVMjvmpiEventMethodEntryIsEnabled() \
    (CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_METHOD_ENTRY) || \
     CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_METHOD_ENTRY2))

/* Purpose: Checks to see if JVMPI_EVENT_METHOD_EXIT is enabled. */
CVMBool CVMjvmpiEventMethodExitIsEnabled(void);
#define CVMjvmpiEventMethodExitIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_METHOD_EXIT)

/* Purpose: Checks to see if JVMPI_EVENT_OBJECT_ALLOC is enabled. */
CVMBool CVMjvmpiEventObjectAllocIsEnabled(void);
#define CVMjvmpiEventObjectAllocIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_OBJECT_ALLOC)

/* Purpose: Checks to see if JVMPI_EVENT_OBJECT_FREE is enabled. */
CVMBool CVMjvmpiEventObjectFreeIsEnabled(void);
#define CVMjvmpiEventObjectFreeIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_OBJECT_FREE)

/* Purpose: Checks to see if JVMPI_EVENT_OBJECT_MOVE is enabled. */
CVMBool CVMjvmpiEventObjectMoveIsEnabled(void);
#define CVMjvmpiEventObjectMoveIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_OBJECT_MOVE)

/* Purpose: Checks to see if JVMPI_EVENT_COMPILED_METHOD_LOAD is enabled. */
CVMBool CVMjvmpiEventCompiledMethodLoadIsEnabled(void);
#define CVMjvmpiEventCompiledMethodLoadIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_COMPILED_METHOD_LOAD)

/* Purpose: Checks to see if JVMPI_EVENT_COMPILED_METHOD_UNLOAD is enabled. */
CVMBool CVMjvmpiEventCompiledMethodUnloadIsEnabled(void);
#define CVMjvmpiEventCompiledMethodUnloadIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_COMPILED_METHOD_UNLOAD)

#ifdef CVM_JVMPI_TRACE_INSTRUCTION

/* Purpose: Checks to see if JVMPI_EVENT_INSTRUCTION_START is enabled. */
CVMBool CVMjvmpiEventInstructionStartIsEnabled(void);
#define CVMjvmpiEventInstructionStartIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_INSTRUCTION_START)

#endif /* CVM_JVMPI_TRACE_INSTRUCTION */

/* Purpose: Checks to see if JVMPI_EVENT_THREAD_START is enabled. */
CVMBool CVMjvmpiEventThreadStartIsEnabled(void);
#define CVMjvmpiEventThreadStartIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_THREAD_START)

/* Purpose: Checks to see if JVMPI_EVENT_THREAD_END is enabled. */
CVMBool CVMjvmpiEventThreadEndIsEnabled(void);
#define CVMjvmpiEventThreadEndIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_THREAD_END)

/* Purpose: Checks to see if JVMPI_EVENT_CLASS_LOAD_HOOK is enabled. */
CVMBool CVMjvmpiEventClassLoadHookIsEnabled(void);
#define CVMjvmpiEventClassLoadHookIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_CLASS_LOAD_HOOK)

/* Purpose: Checks to see if JVMPI_EVENT_JNI_GLOBALREF_ALLOC is enabled. */
CVMBool CVMjvmpiEventJNIGlobalrefAllocIsEnabled(void);
#define CVMjvmpiEventJNIGlobalrefAllocIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_JNI_GLOBALREF_ALLOC)

/* Purpose: Checks to see if JVMPI_EVENT_JNI_GLOBALREF_FREE is enabled. */
CVMBool CVMjvmpiEventJNIGlobalrefFreeIsEnabled(void);
#define CVMjvmpiEventJNIGlobalrefFreeIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_JNI_GLOBALREF_FREE)

/* Purpose: Checks to see if JVMPI_EVENT_JNI_WEAK_GLOBALREF_ALLOC is enabled. */
CVMBool CVMjvmpiEventJNIWeakGlobalrefAllocIsEnabled(void);
#define CVMjvmpiEventJNIWeakGlobalrefAllocIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_JNI_WEAK_GLOBALREF_ALLOC)

/* Purpose: Checks to see if JVMPI_EVENT_JNI_WEAK_GLOBALREF_FREE is enabled. */
CVMBool CVMjvmpiEventJNIWeakGlobalrefFreeIsEnabled(void);
#define CVMjvmpiEventJNIWeakGlobalrefFreeIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_JNI_WEAK_GLOBALREF_FREE)

/* Purpose: Checks to see if JVMPI_EVENT_CLASS_LOAD is enabled. */
CVMBool CVMjvmpiEventClassLoadIsEnabled(void);
#define CVMjvmpiEventClassLoadIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_CLASS_LOAD)

/* Purpose: Checks to see if JVMPI_EVENT_CLASS_UNLOAD is enabled. */
CVMBool CVMjvmpiEventClassUnloadIsEnabled(void);
#define CVMjvmpiEventClassUnloadIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_CLASS_UNLOAD)

/* Purpose: Checks to see if JVMPI_EVENT_DATA_DUMP_REQUEST is enabled. */
CVMBool CVMjvmpiEventDataDumpRequestIsEnabled(void);
#define CVMjvmpiEventDataDumpRequestIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_DATA_DUMP_REQUEST)

/* Purpose: Checks to see if JVMPI_EVENT_DATA_RESET_REQUEST is enabled. */
CVMBool CVMjvmpiEventDataResetRequestIsEnabled(void);
#define CVMjvmpiEventDataResetRequestIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_DATA_RESET_REQUEST)

/* Purpose: Checks to see if JVMPI_EVENT_JVM_INIT_DONE is enabled. */
CVMBool CVMjvmpiEventJVMInitDoneIsEnabled(void);
#define CVMjvmpiEventJVMInitDoneIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_JVM_INIT_DONE)

/* Purpose: Checks to see if JVMPI_EVENT_JVM_SHUT_DOWN is enabled. */
CVMBool CVMjvmpiEventJVMShutDownIsEnabled(void);
#define CVMjvmpiEventJVMShutDownIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_JVM_SHUT_DOWN)

/* Purpose: Checks to see if JVMPI_EVENT_ARENA_NEW is enabled. */
CVMBool CVMjvmpiEventArenaNewIsEnabled(void);
#define CVMjvmpiEventArenaNewIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_ARENA_NEW)

/* Purpose: Checks to see if JVMPI_EVENT_ARENA_DELETE is enabled. */
CVMBool CVMjvmpiEventArenaDeleteIsEnabled(void);
#define CVMjvmpiEventArenaDeleteIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_ARENA_DELETE)

/* Purpose: Checks to see if JVMPI_EVENT_MONITOR_CONTENDED_ENTER and/or
            JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER is enabled. */
CVMBool CVMjvmpiEventMonitorContendedEnterIsEnabled(void);
#define CVMjvmpiEventMonitorContendedEnterIsEnabled() \
    (CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_MONITOR_CONTENDED_ENTER) || \
     CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER))

/* Purpose: Checks to see if JVMPI_EVENT_MONITOR_CONTENDED_ENTERED and/or
            JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED is enabled. */
CVMBool CVMjvmpiEventMonitorContendedEnteredIsEnabled(void);
#define CVMjvmpiEventMonitorContendedEnteredIsEnabled() \
    (CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_MONITOR_CONTENDED_ENTERED) || \
     CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED))

/* Purpose: Checks to see if JVMPI_EVENT_MONITOR_CONTENDED_EXIT and/or
            JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT is enabled. */
CVMBool CVMjvmpiEventMonitorContendedExitIsEnabled(void);
#define CVMjvmpiEventMonitorContendedExitIsEnabled() \
    (CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_MONITOR_CONTENDED_EXIT) || \
     CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT))

/* Purpose: Checks to see if JVMPI_EVENT_MONITOR_WAIT is enabled. */
CVMBool CVMjvmpiEventMonitorWaitIsEnabled(void);
#define CVMjvmpiEventMonitorWaitIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_MONITOR_WAIT)

/* Purpose: Checks to see if JVMPI_EVENT_MONITOR_WAITED is enabled. */
CVMBool CVMjvmpiEventMonitorWaitedIsEnabled(void);
#define CVMjvmpiEventMonitorWaitedIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_MONITOR_WAITED)

/* Purpose: Checks to see if JVMPI_EVENT_GC_START is enabled. */
CVMBool CVMjvmpiEventGCStartIsEnabled(void);
#define CVMjvmpiEventGCStartIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_GC_START)

/* Purpose: Checks to see if JVMPI_EVENT_GC_FINISH is enabled. */
CVMBool CVMjvmpiEventGCFinishIsEnabled(void);
#define CVMjvmpiEventGCFinishIsEnabled() \
    CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_GC_FINISH)

/* Purpose: Indicate that we have started a GC cycle (independent of whether
            actual GC'ing has been blocked or not). */
void CVMjvmpiSetGCWasStarted(void);
#define CVMjvmpiSetGCWasStarted() \
    (CVMjvmpiRec()->gcWasStarted = CVM_TRUE)

/* Purpose: Indicate that we have ended a GC cycle which was started. */
void CVMjvmpiResetGCWasStarted(void);
#define CVMjvmpiResetGCWasStarted() \
    (CVMjvmpiRec()->gcWasStarted = CVM_FALSE)

/* Purpose: Check if we have started a GC cycle. */
CVMBool CVMjvmpiGCWasStarted(void);
#define CVMjvmpiGCWasStarted() \
    (CVMjvmpiRec()->gcWasStarted)

/* Purpose: Indicate that a data dump event has been requested. */
void CVMjvmpiSetDataDumpRequested(void);
#define CVMjvmpiSetDataDumpRequested() \
    (CVMjvmpiRec()->dataDumpRequested = CVM_TRUE)

/* Purpose: Clear the pending data dump event request. */
void CVMjvmpiResetDataDumpRequested(void);
#define CVMjvmpiResetDataDumpRequested() \
    (CVMjvmpiRec()->dataDumpRequested = CVM_FALSE)

/* Purpose: Check if a data dump was requested. */
CVMBool CVMjvmpiDataDumpWasRequested(void);
#define CVMjvmpiDataDumpWasRequested() \
    (CVMjvmpiRec()->dataDumpRequested)

/*=============================================================================
    Methods for posting certain JVMPI events to notify the profiler agent that
    the corresponding event has occurred or is about to occur in the VM:
    NOTE: These methods are to be called from any VM code which need to check
          post the appropriate JVMPI event.
*/

/* Purpose: Posts a JVMPI_EVENT_ARENA_NEW event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostArenaNewEvent(CVMUint32 arenaID, const char *name);

/* Purpose: Posts a JVMPI_EVENT_ARENA_DELETE event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostArenaDeleteEvent(CVMUint32 arenaID);

/* Purpose: Posts a JVMPI_EVENT_CLASS_LOAD event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostClassLoadEvent(CVMExecEnv *ee, const CVMClassBlock *cb);

/* Purpose: Posts a JVMPI_CLASS_LOAD_HOOK event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostClassLoadHookEvent(CVMUint8 **buffer, CVMInt32 *bufferLength,
                                    void *(*malloc_f)(unsigned int));

/* Purpose: Posts a JVMPI_EVENT_CLASS_UNLOAD event. */
/* NOTE: Called while GC safe and should be called from within a GC cycle. */
void CVMjvmpiPostClassUnloadEvent(CVMClassBlock *cb);

/* Purpose: Posts a JVMPI_EVENT_COMPILED_METHOD_LOAD event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostCompiledMethodLoadEvent(CVMExecEnv *ee, CVMMethodBlock *mb);

/* Purpose: Posts a JVMPI_EVENT_COMPILED_METHOD_UNLOAD event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostCompiledMethodUnloadEvent(CVMExecEnv *ee, CVMMethodBlock *mb);

/* Purpose: Posts the JVMPI_EVENT_DATA_DUMP_REQUEST event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostDataDumpRequest(void);

/* Purpose: Posts the JVMPI_EVENT_DATA_RESET_REQUEST event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostDataResetRequest(void);

/* Purpose: Posts the JVMPI_EVENT_GC_START event just before GC starts. */
/* NOTE: Called while GC safe before GC thread requests all threads to be
         safe. */
void CVMjvmpiPostGCStartEvent(void);

/* Purpose: Posts the JVMPI_EVENT_GC_FINISH event just after GC finishes. */
/* NOTE: Called while GC safe before GC thread allows all threads to become
         unsafe again. */
void CVMjvmpiPostGCFinishEvent(CVMUint32 used_objs, CVMUint32 used_obj_space,
                               CVMUint32 total_obj_space);

#ifdef CVM_JVMPI_TRACE_INSTRUCTION

/* Purpose: Posts a JVMPI_EVENT_INSTRUCTION_START event. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostInstructionStartEvent(CVMExecEnv *ee, CVMUint8 *pc);

/* Purpose: Posts a JVMPI_EVENT_INSTRUCTION_START event for "if" conditional
            opcodes. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostInstructionStartEvent4If(CVMExecEnv *ee, CVMUint8 *pc,
                                          CVMBool isTrue);

/* Purpose: Posts a JVMPI_EVENT_INSTRUCTION_START event for "tableswitch"
            opcodes. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostInstructionStartEvent4Lookupswitch(CVMExecEnv *ee,
        CVMUint8 *pc, CVMInt32 chosenPairIndex, CVMInt32 pairsTotal);

/* Purpose: Posts a JVMPI_EVENT_INSTRUCTION_START event for "lookupswitch"
            opcodes. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostInstructionStartEvent4Tableswitch(CVMExecEnv *ee,
        CVMUint8 *pc, CVMInt32 key, CVMInt32 low, CVMInt32 hi);

#endif /* CVM_JVMPI_TRACE_INSTRUCTION */

/* Purpose: Posts a JVMPI_EVENT_JNI_GLOBALREF_ALLOC event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostJNIGlobalrefAllocEvent(CVMExecEnv *ee, jobject ref);

/* Purpose: Posts a JVMPI_EVENT_JNI_GLOBALREF_FREE event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostJNIGlobalrefFreeEvent(CVMExecEnv *ee, jobject ref);

/* Purpose: Posts a JVMPI_EVENT_JNI_WEAK_GLOBALREF_ALLOC event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostJNIWeakGlobalrefAllocEvent(CVMExecEnv *ee, jobject ref);

/* Purpose: Posts a JVMPI_EVENT_JNI_WEAK_GLOBALREF_FREE event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostJNIWeakGlobalrefFreeEvent(CVMExecEnv *ee, jobject ref);

/* Purpose: Posts a JVMPI_EVENT_JVM_INIT_DONE event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostJVMInitDoneEvent(void);

/* Purpose: Posts a JVMPI Method Entry event. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostMethodEntryEvent(CVMExecEnv *ee, CVMObjectICell *objICell);

/* Purpose: Posts a JVMPI Method Entry event. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostMethodExitEvent(CVMExecEnv *ee);

/* Purpose: Posts a JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER or
            JVMPI_EVENT_MONITOR_CONTENDED_ENTER event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostMonitorContendedEnterEvent(CVMExecEnv *ee,
                                            CVMProfiledMonitor *pm);

/* Purpose: Posts a JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED or
            JVMPI_EVENT_MONITOR_CONTENDED_ENTERED event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostMonitorContendedEnteredEvent(CVMExecEnv *ee,
                                              CVMProfiledMonitor *pm);

/* Purpose: Posts a JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT or
            JVMPI_EVENT_MONITOR_CONTENDED_EXIT event. */
/* NOTE: Can be called while GC safe or unsafe. */
void CVMjvmpiPostMonitorContendedExitEvent(CVMExecEnv *ee,
                                           CVMProfiledMonitor *pm);

/* Purpose: Posts a JVMPI_EVENT_MONITOR_WAIT event.  Sent when a thread is
            about to wait on an object. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostMonitorWaitEvent(CVMExecEnv *ee, CVMObjectICell *objICell,
                                  jlong millis);

/* Purpose: Posts a JVMPI_EVENT_MONITOR_WAITED event.  Sent when a thread
            finishes waiting on an object. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostMonitorWaitedEvent(CVMExecEnv *ee, CVMObjectICell *objICell);

/* Purpose: Called by GC routines after it allocates an object to post a
            JVMPI_EVENT_OBJECT_ALLOC event. */
/* NOTE: CVMjvmpiPostObjectAllocEvent() will fetch the arena ID for the object
         from the GC itself.  Hence, the prototype does not require the caller
         to pass in the arena ID for the object.  This is done to simplify the
         implementation for RequestEvent() because JVMPI_EVENT_OBJECT_ALLOC
         can be requested and no arena ID is supplied (which makes sense). */
/* NOTE: Called while GC unsafe or while GC is disabled. */
void CVMjvmpiPostObjectAllocEvent(CVMExecEnv *ee, CVMObject *obj);

/* Purpose: Posts an JVMPI_EVENT_OBJECT_FREE for each object which is about to
            be freed. */
/* NOTE: Called while in a GC cycle i.e. while GC safe also. */
void CVMjvmpiPostObjectFreeEvent(CVMObject *obj);

/* Purpose: Posts an JVMPI_EVENT_OBJECT_MORE for each object which has been
            moved. */
/* NOTE: Called while in a GC cycle i.e. while GC safe also. */
void CVMjvmpiPostObjectMoveEvent(CVMUint32 oldArenaID, void *oldobj,
                                 CVMUint32 newArenaID, void *newobj);

/* Purpose: Posts a JVMPI_EVENT_THREAD_START event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostThreadStartEvent(CVMExecEnv *ee, CVMThreadICell *threadICell);

/* Purpose: Posts a JVMPI_EVENT_THREAD_END event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostThreadEndEvent(CVMExecEnv *ee);


/*=============================================================================
    Methods for initializing and finalizing JVMPI to be called by VM code:
*/

/* Purpose: Gets the JVMPI interface.  Called ONLY from JNI's GetEnv(). */
void *CVMjvmpiGetInterface1(void);

/* Purpose: Initializes the JVMPI globals. */
void CVMjvmpiInitializeGlobals(CVMExecEnv *ee, CVMJvmpiRecord *record);

/* Purpose: Destroys the JVMPI globals. */
void CVMjvmpiDestroyGlobals(CVMExecEnv *ee, CVMJvmpiRecord *record);

/* Purpose: Posts startup events required by JVMPI.  This includes defining
            events which transpired prior to the completion of VM
            initialization. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostStartUpEvents(CVMExecEnv *ee);

/* Purpose: Profiler specific initialization, if requested with the
            CVM_JVMPI_PROFILER=<profiler> build option: */
#ifdef CVM_JVMPI_PROFILER
void CVMJVMPIprofilerInit(CVMExecEnv *ee);
#endif

#endif /* CVM_JVMPI */

#endif /* !_INCLUDED_JVMPI_IMPL_H */
