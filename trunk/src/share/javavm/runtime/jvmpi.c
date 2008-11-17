/*
 * @(#)jvmpi.c	1.33 06/10/10
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

#ifdef CVM_JVMPI

#include "javavm/include/porting/time.h"

#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/defs.h"
#include "javavm/include/directmem.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/globals.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/jvmpi_impl.h"
#include "javavm/include/localroots.h"
#include "javavm/include/named_sys_monitor.h"
#include "javavm/include/preloader.h"
#include "javavm/include/stackmaps.h"
#include "javavm/include/stackwalk.h"
#include "javavm/include/sync.h"
#include "javavm/include/utils.h"

#include "javavm/export/jvm.h"

#include "javavm/include/gc/gc_impl.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/endianness.h"

#include "javavm/include/opcodes.h"
#include "generated/offsets/java_lang_Thread.h"
#include "generated/offsets/java_lang_ThreadGroup.h"

#ifdef CVM_INSPECTOR
#include "javavm/include/inspector.h"
#endif

/* IAI-07: Notify JVMPI of compilations and decompilations. */
#ifdef CVM_JIT
#include "javavm/include/jit/jitcodebuffer.h"
#endif

/* NOTE:

    Calling from VM code to the profiler agent:
    ==========================================
    This implementation of JVMPI tries to only invoke the profiler agent when
    the thread is in a GC safe state before.  This is done to minimize the
    chance of the profiler locking up any thread which wants to run a GC
    cycle.  In addition, JVMPI also provides a means to disable GC through the
    use of the CVMGCLocker mechanism.  This mechanism only prevents the GC
    cycle from actually starting.  Unlike having a GC unsafe state, it does not
    block the thread wanting to run a GC cycle.  When GC is disabled, a request
    for a synchronous GC will end up doing nothing in the call to the GC.

    Reason for why JVMPI sometimes uses its own CVMjvmpiGetICellDirect():
    ====================================================================
    GC need to post events (JVMPI_EVENT_OBJECT_FREE and JVMPI_EVENT_OBJECT_MOVE)
    while in a consistent state.  Under such circumstances, the JVMPI
    implementation is not allowed to become GC unsafe in order to call
    CVMID_icellDirect().  CVMjvmpiGetICellDirect() is created to bypass the
    assertion that the thread is GC unsafe.

    It is OK to use CVMjvmpiGetICellDirect() instead of CVMID_icellDirect() in
    the following cases:

    1. The current thread is the GC thread (assumming GC runs in a single
       thread) and a GC cycle is in progress.  Since the current thread is the
       GC thread, there won't be any other thread changing the addresses of
       objects.  Hence it is safe to dereference the ICell without becoming GC
       unsafe first.  Of course, if GC becomes multi-threaded in the future,
       the code will have to be revised to deal with this.

    2. GC has already been disabled by call a call to CVMjvmpiDisableGC()
       previously.  Hence, GC won't be running in another thread and changing
       the addresses of objects.  As a result, it is safe to dereference the
       ICell without becoming GC unsafe first.

    3. The current thread is already in a GC unsafe state.

    4. The current thread is holding the heapLock.  This ensures that GC cannot
       start up in another thread.  Usually, this condition is true when the
       current thread is in the process of allocating an object.

    The condition necessary for safe direct access comprising the above 4
    criteria can be tested by using the CVMjvmpiIsSafeToAccessDirectObject()
    macro.

    Reason for why JVMPI always become GC safe before calling the agent:
    ===================================================================
    When the VM is not in a GC cycle, the profiling agent may choose to call
    JNI functions.  This means that we must ensure that the profiling agent is
    operating in a GC safe state at that point.

    Hence, in every case where JVMPI calls across to the profiling agent, it
    will have to ensure the current thread is in a GC safe state if the VM is
    not in the midst of a GC cycle.  When in the midst of a GC cycle, JVMPI is
    not allowed to change the GC safety state of the current thread.

    GC Safety and Deadlock Issues:
    =============================
    If the current thread is in the midst of a GC cycle, the profiling agent is
    not allowed to become GC unsafe (e.g. by invoking JNI APIs).  There is
    currently no way to enforce this.  Fortunately, the JVMPI specs give us a
    little help here.

    According to the JVMPI specs:
    "Usage Note: 

    It is not safe to invoke JNI functions in arbirtrary JVMPI event handlers.
    JVMPI events may be issued in virtual machine states that are not suitable
    for executing JNI functions. The profiler agent may only invoke JNI
    functions in multithreaded mode (as defined by the JVMPI specification)
    and must take extremely care under these circumstances to avoid race
    conditions, dead locks, and infinite recursions."

    Also according to the JVMPI specs, the VM is deemed to be in thread
    suspended (i.e. single-threaded) mode when issuing GC events.  Hence, we
    can have a little faith that a well-behaved profiler won't make any JNI
    calls when processing received GC events.

    ==========================================================================

    Assumptions:

    1. Availability of a CVMExecEnv:
    ===============================
    It is assumed that the profiler agent won't call into the VM from a thread
    that wasn't created by the VM (this is after all probably why JVMPI
    provides a thread creation mechanism).  In order for the VM to do anything
    useful for the profiler, it will need a CVMExecEnv.  If the profiler agent
    calls upon a JVMPI function from a thread not created by the VM, the VM
    will almost never be able to satisfy the functionality requested anyway.
    Hence, this JVMPI implementation will assume that a CVMExecEnv will always
    be available.

    ==========================================================================

*/

/*=============================================================================
    Prototype of methods which make up the JVMPI Interface:
*/

static void CVMjvmpi_NotifyEvent(JVMPI_Event *event);
static jint CVMjvmpi_EnableEvent(jint event_type, void *arg);
static jint CVMjvmpi_DisableEvent(jint event_type, void *arg);
static jint CVMjvmpi_RequestEvent(jint event_type, void *arg);
static void CVMjvmpi_GetCallTrace(JVMPI_CallTrace *trace, jint depth);
static void CVMjvmpi_ProfilerExit(jint exit_code);

static JVMPI_RawMonitor CVMjvmpi_RawMonitorCreate(char *lock_name);
static void CVMjvmpi_RawMonitorEnter(JVMPI_RawMonitor lock_id);
static void CVMjvmpi_RawMonitorExit(JVMPI_RawMonitor lock_id);
static void CVMjvmpi_RawMonitorWait(JVMPI_RawMonitor lock_id, jlong ms);
static void CVMjvmpi_RawMonitorNotifyAll(JVMPI_RawMonitor lock_id);
static void CVMjvmpi_RawMonitorDestroy(JVMPI_RawMonitor lock_id);

static jlong CVMjvmpi_GetCurrentThreadCpuTime(void);

static void CVMjvmpi_SuspendThread(JNIEnv *env);
static void CVMjvmpi_ResumeThread(JNIEnv *env);
static jint CVMjvmpi_GetThreadStatus(JNIEnv *env);
static jboolean CVMjvmpi_ThreadHasRun(JNIEnv *env);
static jint CVMjvmpi_CreateSystemThread(char *name, jint priority, 
                                        jvmpi_void_function_of_void f);

static void CVMjvmpi_SetThreadLocalStorage(JNIEnv *env, void *ptr);
static void *CVMjvmpi_GetThreadLocalStorage(JNIEnv *env);

static void CVMjvmpi_DisableGC(void);
static void CVMjvmpi_EnableGC(void);
static void CVMjvmpi_RunGC(void);

static jobjectID CVMjvmpi_GetThreadObject(JNIEnv *env);
static jobjectID CVMjvmpi_GetMethodClass(jmethodID mid);

static jobject CVMjvmpi_jobjectID2jobject(jobjectID obj);
static jobjectID CVMjvmpi_jobject2jobjectID(jobject ref);

/*=============================================================================
    Prototype of other PRIVATE methods of the JVMPI implementation:
*/

static jint CVMjvmpiPostHeapDumpEvent(CVMExecEnv *ee, int dumpLevel);
static jint CVMjvmpiPostMonitorDumpEvent(CVMExecEnv *ee);
static jint CVMjvmpiPostObjectDumpEvent(CVMObject *obj);

static void CVMjvmpiPostMonitorContendedExitEvent2(CVMExecEnv *ee,
                                                   CVMProfiledMonitor *mon);

static const CVMClassBlock *
CVMjvmpiClassInstance2ClassBlock(CVMExecEnv *ee, CVMObject *obj);

static void CVMjvmpiDisableAllNotification(CVMExecEnv *ee);
static void CVMjvmpiDisableGC(CVMExecEnv *ee);
static void CVMjvmpiDoCleanup(void);
static void CVMjvmpiEnableGC(CVMExecEnv *ee);
static CVMUint8 CVMjvmpiGetClassType(const CVMClassBlock *cb);
static CVMInt32 CVMjvmpiGetLineNumber(CVMMethodBlock *mb, CVMFrame *frame);
static CVMUint16 CVMjvmpiGetThreadStatus(CVMExecEnv *ee);

static jint
CVMjvmpiPostClassLoadEvent2(CVMExecEnv *ee, const CVMClassBlock *cb,
                            CVMBool isRequestedEvent);

static void
CVMjvmpiPostObjectAllocEvent2(CVMExecEnv *ee, CVMObject *obj,
                              CVMBool isRequestedEvent);
static jint
CVMjvmpiPostThreadStartEvent2(CVMExecEnv *ee, CVMObject *threadObj,
                              CVMBool isRequestedEvent);


/* Purpose: Gets the global JVMPI_Interface. */
#define CVMjvmpiInterface() (&CVMjvmpiRec()->interfaceX)

/* Purpose: Calls the NotifyEvent() in the JVMPI_Interface. */
#define CVMjvmpiNotifyEvent(/* JVMPI_Event * */event) { \
    CVMjvmpiInterface()->NotifyEvent(event);        \
}

/* Purpose: Checks to see if GC is disabled. */
#define CVMjvmpiGCIsDisabled() \
    (CVMgcLockerIsActive(&CVMjvmpiRec()->gcLocker))

/* Purpose: Checks to see if it is safe to access object pointers directly. */
#define CVMjvmpiIsSafeToAccessDirectObject(ee_)       \
    (CVMD_isgcUnsafe(ee_) || CVMgcIsGCThread(ee_) ||  \
     CVMjvmpiGCIsDisabled() || CVMsysMutexIAmOwner(ee_, &CVMglobals.heapLock))

/* Purpose: Gets the direct object pointer for the specified ICell. */
/* NOTE: It is only safe to use this when we are in a GC unsafe state, or GC
         has been disabled using CVMjvmpi_disableGC(), or the current thread
         is the GC thread. */
#define CVMjvmpiGetICellDirect(ee_, icellPtr_) \
    CVMID_icellGetDirectWithAssertion(CVMjvmpiIsSafeToAccessDirectObject(ee_), \
                                      icellPtr_)

/* Purpose: Sets the direct object pointer in the specified ICell. */
/* NOTE: It is only safe to use this when we are in a GC unsafe state, or GC
         has been disabled using CVMjvmpi_disableGC(), or the current thread
         is the GC thread. */
#define CVMjvmpiSetICellDirect(ee_, icellPtr_, directObj_) \
    CVMID_icellSetDirectWithAssertion(CVMjvmpiIsSafeToAccessDirectObject(ee_), \
                                      icellPtr_, directObj_)

/*=============================================================================
    Prototype of CVMJvmpiEventInfo methods:
*/

static void CVMjvmpiEventInfoInit(CVMJvmpiEventInfo *self, CVMExecEnv *ee);
static void CVMjvmpiEventInfoDisable(CVMJvmpiEventInfo *self, CVMExecEnv *ee,
                                     CVMInt32 event);
static void CVMjvmpiEventInfoDisableAll(CVMJvmpiEventInfo *self,
                                        CVMExecEnv *ee);
static void CVMjvmpiEventInfoEnable(CVMJvmpiEventInfo *self, CVMExecEnv *ee,
                                    CVMInt32 event);

/* static CVMBool
CVMjvmpiEventInfoIsNotAvailable(CVMJvmpiEventInfo *self, CVMInt32 event); */
#define CVMjvmpiEventInfoIsNotAvailable(self, event) \
    ((self)->eventInfoX[event] == CVMJvmpiEventInfo_NOT_AVAILABLE)

/*=============================================================================
    CVMJvmpiEventInfo methods:
*/

/* Purpose: Initializer. */
static void CVMjvmpiEventInfoInit(CVMJvmpiEventInfo *self, CVMExecEnv *ee)
{
    CVMsysMicroLock(ee, CVM_TOOLS_MICROLOCK);

    /* Fill in default event info for enabling/disabling event notification: */

    /* Because CVMJvmpiEventInfo_DISABLED is defined to be 0, the
       following can be done more efficiently with a memset:

        int i;
        for (i = 0; i <= JVMPI_MAX_EVENT_TYPE_VAL; i++) {
            info->eventInfoX[i] = CVMJvmpiEventInfo_NOT_APPLICABLE;
        }
    */
    memset(self->eventInfoX, CVMJvmpiEventInfo_NOT_AVAILABLE,
           JVMPI_MAX_EVENT_TYPE_VAL + 1);

#undef CVMjvmpiEventInfoDisableX
#define CVMjvmpiEventInfoDisableX(event) \
    self->eventInfoX[event] = CVMJvmpiEventInfo_DISABLED;

    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_CLASS_LOAD);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_CLASS_UNLOAD);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_CLASS_LOAD_HOOK);
#ifdef CVM_JVMPI_TRACE_INSTRUCTION
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_INSTRUCTION_START);
#endif
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_OBJ_ALLOC);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_OBJ_FREE);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_THREAD_START);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_THREAD_END);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_JNI_GLOBALREF_ALLOC);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_JNI_GLOBALREF_FREE);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_JNI_WEAK_GLOBALREF_ALLOC);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_JNI_WEAK_GLOBALREF_FREE);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_METHOD_ENTRY);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_METHOD_ENTRY2);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_METHOD_EXIT);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_LOAD_COMPILED_METHOD);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_UNLOAD_COMPILED_METHOD);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_JVM_INIT_DONE);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_JVM_SHUT_DOWN);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_DATA_DUMP_REQUEST);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_DATA_RESET_REQUEST);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_OBJ_MOVE);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_ARENA_NEW);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_ARENA_DELETE);

    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_MONITOR_CONTENDED_ENTER);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_MONITOR_CONTENDED_ENTERED);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_MONITOR_CONTENDED_EXIT);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_MONITOR_WAIT);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_MONITOR_WAITED);

    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_GC_START);
    CVMjvmpiEventInfoDisableX(JVMPI_EVENT_GC_FINISH);

#undef CVMjvmpiEventInfoDisableX

    CVMsysMicroUnlock(ee, CVM_TOOLS_MICROLOCK);
}

/* Purpose: Disable notification of the specified Event. */
static void CVMjvmpiEventInfoDisable(CVMJvmpiEventInfo *self, CVMExecEnv *ee,
                                     CVMInt32 event)
{
    CVMsysMicroLock(ee, CVM_TOOLS_MICROLOCK);
    self->eventInfoX[event] = CVMJvmpiEventInfo_DISABLED;
    CVMsysMicroUnlock(ee, CVM_TOOLS_MICROLOCK);
}

/* Purpose: Marks all event notifications as being disabled. */
static void CVMjvmpiEventInfoDisableAll(CVMJvmpiEventInfo *self, CVMExecEnv *ee)
{
    int i;
    CVMsysMicroLock(ee, CVM_TOOLS_MICROLOCK);
    for (i = 0; i <= JVMPI_MAX_EVENT_TYPE_VAL; i++) {
        if (!CVMjvmpiEventInfoIsNotAvailable(self, i)) {
            self->eventInfoX[i] = CVMJvmpiEventInfo_DISABLED;
        }
    }
    CVMsysMicroUnlock(ee, CVM_TOOLS_MICROLOCK);
}

/* Purpose: Enable notification of the specified Event. */
static void CVMjvmpiEventInfoEnable(CVMJvmpiEventInfo *self, CVMExecEnv *ee,
                                    CVMInt32 event)
{
    CVMsysMicroLock(ee, CVM_TOOLS_MICROLOCK);
    self->eventInfoX[event] = CVMJvmpiEventInfo_ENABLED;
    CVMsysMicroUnlock(ee, CVM_TOOLS_MICROLOCK);
}

/*=============================================================================
    Definition of the CVMDumper utility class for generating the dump info
    required for JVMPI support:
*/

/* 
 * Data struture used by the heap dump routines to temporarily hold
 * trace information. For simplicity, we place an upper bound on the 
 * number of threads traced by GC.
 */
#define JVMPI_DUMP_MAX_TRACES 1024

typedef struct CVMDumper CVMDumper;
struct CVMDumper
{
    CVMUint8 *buffer;       /* Beginning of the allocated buffer. */
    CVMUint8 *bufferEnd;    /* End of the allocated buffer. */
    CVMUint8 *ptr;          /* Current write location in buffer. */

    CVMInt32  dumpLevel;    /* level of information in the dump */
    CVMBool isOverflowed;   /* Indicates if a buffer overflow was detected. */

    CVMUint8 *dumpStart;    /* Start of dump. */
    jint *threadStatusList; /* Start or thread status list. */

    CVMUint32 tracesCount;  /* number of threads traced by GC */
    JVMPI_CallTrace traces[JVMPI_DUMP_MAX_TRACES];
};

/* Some of the following are implemented as macros below.  For these, the
   "static" modifier has to omitted to placate the compiler from generating
   warnings.  The modifiers are left here in commented form to indicate that
   these functions should be static if the macros are ever converted back into
   functions: */

static void CVMdumperInit(CVMDumper *self, CVMInt32 dumpLevel);
static void CVMdumperInitBuffer(CVMDumper *self, CVMUint8 *buffer,
                                CVMUint32 bufferSize);
/* static */ CVMBool CVMdumperIsOverflowed(CVMDumper *self);
/* static */ CVMUint32 CVMdumperGetRequiredBufferSize(CVMDumper *self);
/* static */ void CVMdumperSetThreadStatusList(CVMDumper *self,
                                               jint *threadStatusList);
/* static */ void CVMdumperSetDumpStart(CVMDumper *self, CVMUint8 *position);
/* static */ CVMUint8 *CVMdumperGetPosition(CVMDumper *self);
/* static */ void CVMdumperSeekTo(CVMDumper *self, CVMUint8 *position);
/* static */ CVMInt32 CVMdumperGetLevel(CVMDumper *self);

static void *CVMdumperAlloc(CVMDumper *self, CVMUint32 size);
static void CVMdumperWrite(CVMDumper *self, void *srcBuffer, CVMUint32 size);
static void CVMdumperWriteID(CVMDumper *self, void *value);
static void CVMdumperWriteU4(CVMDumper *self, CVMUint32 value);
static void CVMdumperWriteU2(CVMDumper *self, CVMUint16 value);
static void CVMdumperWriteU1(CVMDumper *self, CVMUint8 value);
static void CVMdumperWrite64(CVMDumper *self, CVMJavaVal64 value);

/* static */ void CVMdumperWriteLong(CVMDumper *self, CVMJavaLong value);
/* static */ void CVMdumperWriteDouble(CVMDumper *self, CVMJavaDouble value);
/* static */ void CVMdumperWriteInt(CVMDumper *self, CVMJavaInt value);
/* static */ void CVMdumperWriteFloat(CVMDumper *self, CVMJavaFloat value);
/* static */ void CVMdumperWriteShort(CVMDumper *self, CVMJavaShort value);
/* static */ void CVMdumperWriteChar(CVMDumper *self, CVMJavaChar value);
/* static */ void CVMdumperWriteByte(CVMDumper *self, CVMJavaByte value);
/* static */ void CVMdumperWriteBoolean(CVMDumper *self, CVMJavaBoolean value);

static void CVMdumperDumpThreadTrace(CVMDumper *self, CVMExecEnv *targetEE);
static void CVMdumperDumpRoot(CVMObject **ref, void *data);
static jint CVMdumperDumpInstance(CVMDumper *self, CVMObject *obj);
static jint CVMdumperDumpClass(CVMDumper *self, CVMObject *clazz);
static jint CVMdumperDumpArray(CVMDumper *self, CVMObject *arrObj);
static jint CVMdumperDumpObject(CVMDumper *self, CVMObject *obj);
static void CVMdumperDumpThreadInfo(CVMDumper *self, CVMExecEnv *threadEE);

static jsize CVMdumperComputeInstanceDumpSize(CVMDumper *self, CVMObject *obj);
static jsize CVMdumperComputeClassDumpSize(CVMDumper *self, CVMObject *clazz);
static jsize CVMdumperComputeArrayDumpSize(CVMDumper *self, CVMObject *arrObj);
static jsize CVMdumperComputeObjectDumpSize(CVMDumper *self, CVMObject *obj);

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
static void CVMdumperDumpFastLockInfo(CVMDumper *self, CVMExecEnv *threadEE);
#endif

static void CVMdumperDumpMonitor(CVMDumper *self, CVMProfiledMonitor *mon);
static void CVMdumperDumpThreadsAndMonitors(CVMDumper *dumper, CVMExecEnv *ee);
static CVMBool CVMdumperHeapDumpCallback(CVMObject* obj, CVMClassBlock* cb,
                                         CVMUint32 size, void* data);
static void CVMdumperDumpHeap(CVMDumper *self, CVMExecEnv *ee);

/*=============================================================================
    Methods of the CVMDumper utility class for generating the dump info
    required for JVMPI support:
*/

/* Purpose: Initializes the Dumper object. */
static void CVMdumperInit(CVMDumper *self, CVMInt32 dumpLevel)
{
    memset(self, 0, sizeof(CVMDumper));
    self->dumpLevel = dumpLevel; 
}

/* Purpose: Initializes the Dumper's buffer. */
static void CVMdumperInitBuffer(CVMDumper *self, CVMUint8 *buffer,
                                CVMUint32 bufferSize)
{
    self->buffer = buffer;
    self->bufferEnd = buffer + bufferSize;
    self->ptr = buffer;
    self->dumpStart = buffer;
    self->isOverflowed = CVM_FALSE;
}

/* Purpose: Checks to see if the Dumper has been overflowed. */
#define CVMdumperIsOverflowed(/* CVMDumper * */ self) \
    ((self)->isOverflowed)

/* Purpose: Gets the actual amount of memory needed for the entire dump
            operation. */
#define CVMdumperGetRequiredBufferSize(/* CVMDumper * */ self) \
    (((self)->ptr) - ((self)->buffer))

/* Purpose: Sets the threadStatusList field. */
#define CVMdumperSetThreadStatusList(/* CVMDumper * */ self,         \
                                     /* jint * */ threadStatusList_) \
    ((self)->threadStatusList = threadStatusList_)

/* Purpose: Sets the dumpStart field. */
#define CVMdumperSetDumpStart(/* CVMDumper * */ self,    \
                              /* CVMUint8 * */ position) \
    ((self)->dumpStart = position)

/* Purpose: Gets the current position of the dumper. */
#define CVMdumperGetPosition(/* CVMDumper * */self) \
    ((CVMUint8 *)(self)->ptr)

/* Purpose: Seeks to the specified position. */
#define CVMdumperSeekTo(/* CVMDumper * */self, /* CVMUint8 * */position) \
    ((self)->ptr = (position))

/* Purpose: Gets the level of the dump. */
#define CVMdumperGetLevel(/* CVMDumper * */self) \
    ((self)->dumpLevel)


/* Purpose: Allocate a block of memory from the dump buffer. */
static void *CVMdumperAlloc(CVMDumper *self, CVMUint32 size)
{
    void *result = self->ptr;
    size = CVMpackSizeBy(size, 8);
    self->ptr += size;
    if (self->ptr >= self->bufferEnd) {
        self->isOverflowed = CVM_TRUE;
        return NULL;
    }
    return result;
}

/* Purpose: Dumps an arbitary value. */
static void CVMdumperWrite(CVMDumper *self, void *srcBuffer, CVMUint32 size)
{
    self->ptr += size;
    if (self->ptr >= self->bufferEnd) {
        self->isOverflowed = CVM_TRUE;
    } else {
        memcpy(self->ptr - size, srcBuffer, size);
    }
}

/* Purpose: Dumps a (void *) value. */
static void CVMdumperWriteID(CVMDumper *self, void *value)
{
    CVMdumperWrite((self), &value, sizeof(void *));
}

/* Purpose: Dumps a CVMUint32 value. */
static void CVMdumperWriteU4(CVMDumper *self, CVMUint32 value)
{
#if (CVM_ENDIANNESS == CVM_LITTLE_ENDIAN)
    /* NOTE: According to JDK Classic's implementation, the byte-order expected
        by the profiler is the network byte order i.e. BIG ENDIAN.  Hence, if
        this platform is little endian, then we need to convert the byte-order
        to BIG ENDIAN before passing it to the profiler: */
    {
        /* Convert endianness of value: */
        CVMUint32 newValue =
            ((value & 0x000000FF) << 24) |
            ((value & 0x0000FF00) << 8)  |
            ((value & 0x00FF0000) >> 8)  |
            ((value & 0xFF000000) >> 24);
        value = newValue;
    }
#endif
    CVMdumperWrite(self, &value, sizeof(CVMUint32));
}

/* Purpose: Dumps a CVMUint16 value. */
static void CVMdumperWriteU2(CVMDumper *self, CVMUint16 value)
{
#if (CVM_ENDIANNESS == CVM_LITTLE_ENDIAN)
    /* NOTE: According to JDK Classic's implementation, the byte-order expected
        by the profiler is the network byte order i.e. BIG ENDIAN.  Hence, if
        this platform is little endian, then we need to convert the byte-order
        to BIG ENDIAN before passing it to the profiler: */
    {
        /* Convert endianness of value: */
        CVMUint16 newValue =
            ((value & 0x00FF) << 8) |
            ((value & 0xFF00) >> 8);
        value = newValue;
    }
#endif
    CVMdumperWrite(self, &value, sizeof(CVMUint16));
}

/* Purpose: Dumps a CVMUint8 value. */
static void CVMdumperWriteU1(CVMDumper *self, CVMUint8 value)
{
    CVMdumperWrite(self, &value, sizeof(CVMUint8));
}

/* Purpose: Dumps a CVMJavaVal64 value. */
static void CVMdumperWrite64(CVMDumper *self, CVMJavaVal64 value)
{
    CVMJavaLong highLong = CVMlongShr(value.l, 32);
    CVMUint32 high = CVMlong2Int(highLong);
    CVMUint32 low = CVMlong2Int(value.l);
    CVMdumperWriteU4(self, high);
    CVMdumperWriteU4(self, low);
}

/* Purpose: Dumps a Long value. */
#define CVMdumperWriteLong(/* CVMDumper * */self, /* CVMJavaLong */value_) { \
    CVMJavaVal64 val_;                                                       \
    val_.l = (value_);                                                       \
    CVMdumperWrite64(self, val_);                                            \
}

/* Purpose: Dumps a Double value. */
#define CVMdumperWriteDouble(/* CVMDumper * */self,       \
                             /* CVMJavaDouble */value_) { \
    CVMJavaVal64 val_;                                    \
    val_.d = (value_);                                    \
    CVMdumperWrite64(self, val_);                         \
}

/* Purpose: Dumps an Int value. */
#define CVMdumperWriteInt(/* CVMDumper * */self, /* CVMJavaInt */value_) { \
    CVMJavaInt val_ = (value_);                                            \
    CVMdumperWriteU4(self, (CVMUint32) val_);                              \
}

/* Purpose: Dumps a Float value. */
#define CVMdumperWriteFloat(/* CVMDumper * */self, /* CVMJavaFloat */value_) {\
    CVMJavaVal32 val_;                                              \
    val_.f = (value_);                                              \
    CVMdumperWriteU4(self, (CVMUint32) val_.i);                     \
}

/* Purpose: Dumps a Short value. */
#define CVMdumperWriteShort(/* CVMDumper * */self,      \
                            /* CVMJavaShort */value_) { \
    CVMJavaShort val_ = (value_);                       \
    CVMdumperWriteU2(self, (CVMUint16) val_);           \
}

/* Purpose: Dumps a Char value. */
#define CVMdumperWriteChar(/* CVMDumper * */self, /* CVMJavaChar */value_) { \
    CVMJavaChar val_ = (value_);                                             \
    CVMdumperWriteU2(self, (CVMUint16) val_);                                \
}

/* Purpose: Dumps a Byte value. */
#define CVMdumperWriteByte(/* CVMDumper * */self, /* CVMJavaByte */value_) { \
    CVMJavaByte val_ = (value_);                                             \
    CVMdumperWriteU1(self, (CVMUint8) val_);                                 \
}

/* Purpose: Dumps a Boolean value. */
#define CVMdumperWriteBoolean(/* CVMDumper * */self,        \
                              /* CVMJavaBoolean */value_) { \
    CVMJavaBoolean val_ = (value_);                         \
    CVMdumperWriteU1(self, (CVMUint8) val_);                \
}

/* Purpose: Dumps a trace for the specified thread. */
static void CVMdumperDumpThreadTrace(CVMDumper *self, CVMExecEnv *targetEE)
{
    CVMStack *stack = &targetEE->interpreterStack;
    CVMStackWalkContext context;
    CVMFrame *frame;
    CVMUint32 framesCount = 0;
    JVMPI_CallTrace *trace;

    if (CVMdumperGetLevel(self) == JVMPI_DUMP_LEVEL_0) {
        return;
    }

    /* Get a trace frame: */
    trace = &self->traces[self->tracesCount++];

    /* Bounds check: */
    CVMassert(self->tracesCount <= JVMPI_DUMP_MAX_TRACES);

    /* Walk stack frames and count them: */
    CVMstackwalkInit(stack, &context);
    frame = context.frame;
    while (frame != 0) {
        if (!CVMframeIsTransition(frame) && (frame->mb != NULL)) {
            framesCount++;
        }
        CVMstackwalkPrev(&context);
        frame = context.frame;
    }
    trace->num_frames = framesCount;
    trace->env_id = CVMexecEnv2JniEnv(targetEE);
    trace->num_frames = framesCount;
    /* Allocate memory for the frames dump from the dump buffer: */
    trace->frames = CVMdumperAlloc(self, framesCount * sizeof(JVMPI_CallFrame));

    /* CVMjvmpi_GetCallTrace() will ONLY fill out the trace information in the
       alloted memory above.  It has no knowledge of the CVMDumper and hence
       will not allocate any more memory from it.  Hence, it is safe to skip
       it.  Also, CVMjvmpi_GetCallTrace() always assumes that you have given
       it adequate memory for it to fill (as indicated by framesCount).
       Hence, we must not call it if we don't have the needed memory. */
    if (CVMdumperIsOverflowed(self)) {
        return;
    }
    CVMjvmpi_GetCallTrace(trace, framesCount);
}

/* Purpose: Dumps the specified GC ROOT. */
static void CVMdumperDumpRoot(CVMObject **ref, void *data)
{
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
#endif
    CVMGCProfilingInfo *info = (CVMGCProfilingInfo *)data;
    CVMDumper *self = (CVMDumper *) info->data;
    CVMObjectICell *icell = (CVMObjectICell *) ref;

    if ((icell == NULL) || CVMID_icellIsNull(icell)) {
        return;
    }
    if (CVMdumperGetLevel(self) == JVMPI_DUMP_LEVEL_0) {
        return;
    }

    switch (info->type) {
        case CVMGCRefType_GLOBAL_ROOT:
            CVMdumperWriteU1(self, JVMPI_GC_ROOT_JNI_GLOBAL);
            CVMdumperWriteID(self, (void *) CVMjvmpiGetICellDirect(ee, icell));
            /* Write the address of the globalref: */
            CVMdumperWriteID(self, (void *) *ref);
            break;
        case CVMGCRefType_PRELOADER_STATICS:
        case CVMGCRefType_CLASS_STATICS:
        case CVMGCRefType_LOCAL_ROOTS:
        case CVMGCRefType_UNKNOWN_STACK_FRAME:
        case CVMGCRefType_TRANSITION_FRAME:
            CVMdumperWriteU1(self, JVMPI_GC_ROOT_UNKNOWN);
            CVMdumperWriteID(self, (void *) CVMjvmpiGetICellDirect(ee, icell));
            break;
        case CVMGCRefType_JAVA_FRAME:
        case CVMGCRefType_JNI_FRAME: {
            int i;
            switch (info->type) {
                case CVMGCRefType_JAVA_FRAME:
                    CVMdumperWriteU1(self, JVMPI_GC_ROOT_JAVA_FRAME);
                    break;
                case CVMGCRefType_JNI_FRAME:
                    CVMdumperWriteU1(self, JVMPI_GC_ROOT_JNI_LOCAL);
                    break;
                default:
                    break;
            }
            CVMdumperWriteID(self, (void *) CVMjvmpiGetICellDirect(ee, icell));

            for (i = 0; i < self->tracesCount; i++) {
                JNIEnv *targetEnv = CVMexecEnv2JniEnv(info->u.frame.ee);
                JVMPI_CallTrace *trace = &self->traces[i];
                jint traceDepth = trace->num_frames;
                if (trace->env_id == targetEnv) {
                    CVMdumperWriteID(self, (void *) trace->env_id);
                    CVMdumperWriteU4(self, traceDepth -
                                     (jint)info->u.frame.frameNumber - 1);
                    return;
                }
            }

            /* We should never get here.  But if we do, then we should write
             * out a NULL equivalent. */
            CVMassert(CVM_FALSE);
            CVMdumperWriteID(self, 0);
            CVMdumperWriteU4(self, -1);
            break;
        }
        default:
            /* There should not be any unhandled cases: */
            CVMassert(CVM_FALSE);
    }
}

/* Purpose: Dump an object instance for Level 1 or 2 dumps. */
/* Returns: JVMPI_SUCCESS if successful. */
/* NOTE: The format of the object instance dump is as follows:
        u1        recordType;
        jobjectID objectSelf;
        jobjectID objectClass;
        u4        number_of_bytes_to_follow;
        [values]  instance_field_values[];
   LEVEL 1 dumps excludes all primitive fields.
*/
/* NOTE: Called while GC is disabled or when in GC thread. */
static jint CVMdumperDumpInstance(CVMDumper *self, CVMObject *obj)
{
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
#endif
    CVMClassBlock *cb = CVMobjectGetClass(obj);
    CVMClassBlock *cb0;
    CVMUint32 dumpLevel = CVMdumperGetLevel(self);
    CVMUint32 size = 0;
    CVMUint8 *savedPosition1;
    CVMUint8 *savedPosition2;

    CVMassert(CVMjvmpiGCIsDisabled() || CVMgcIsGCThread(CVMgetEE()));

    CVMdumperWriteU1(self, JVMPI_GC_INSTANCE_DUMP);
    CVMdumperWriteID(self, obj);
    CVMdumperWriteID(self,
        (void *)CVMjvmpiGetICellDirect(ee, CVMcbJavaInstance(cb)));

    /* Save the location for the size to be written later: */
    savedPosition1 = CVMdumperGetPosition(self);
    CVMdumperWriteU4(self, 0);  /* Write a place holder for now. */

    /* Dump the field values: */
    for (cb0 = cb; cb0 != NULL; cb0 = CVMcbSuperclass(cb0)) {
        CVMInt32 i;

        /* NOTE: We're interating through the fields in reverse order.  This is
           intentional because JVMPI list fields in reverse order than that
           which is declared in the ClassFile.  This is especially evident in
           the definition of the JVMPI_GC_INSTANCE_DUMP record where the fields
           of the self class is to preceed that of its super class and so on so
           forth.
        */
        for (i = CVMcbFieldCount(cb0); --i >= 0; ) {
            CVMFieldBlock *fb = CVMcbFieldSlot(cb0, i);
            CVMClassTypeID fieldType;
            if (CVMfbIs(fb, STATIC)) {
                continue;
            }
            fieldType = CVMtypeidGetType(CVMfbNameAndTypeID(fb));
            if (CVMtypeidFieldIsRef(fieldType)) {
                CVMObject *value;
                CVMD_fieldReadRef(obj, CVMfbOffset(fb), value);
                CVMdumperWriteID(self, value);
                size += sizeof(jobjectID);

            } else if (dumpLevel == JVMPI_DUMP_LEVEL_2) {

#undef DUMP_FIELD
#define DUMP_FIELD(size_, type_, wtype_) {               \
    CVMJava##type_ value;                                \
    CVMD_fieldRead##wtype_(obj, CVMfbOffset(fb), value); \
    CVMdumperWrite##type_(self, value);                  \
    size += size_;                                       \
}
                switch (fieldType) {
                    case CVM_TYPEID_LONG: DUMP_FIELD(8, Long, Long); break;
                    case CVM_TYPEID_DOUBLE: DUMP_FIELD(8, Double, Double); break;
                    case CVM_TYPEID_INT: DUMP_FIELD(4, Int, Int); break;
                    case CVM_TYPEID_FLOAT: DUMP_FIELD(4, Float, Float); break;
                    case CVM_TYPEID_SHORT: DUMP_FIELD(2, Short, Int); break;
                    case CVM_TYPEID_CHAR: DUMP_FIELD(2, Char, Int); break;
                    case CVM_TYPEID_BYTE: DUMP_FIELD(1, Byte, Int); break;
                    case CVM_TYPEID_BOOLEAN: DUMP_FIELD(1, Boolean, Int); break;
                }
#undef DUMP_FIELD

            }
        }
    }

    /* Write the actual size of field values: */
    savedPosition2 = CVMdumperGetPosition(self);
    CVMdumperSeekTo(self, savedPosition1);
    CVMdumperWriteU4(self, size);
    CVMdumperSeekTo(self, savedPosition2);

    return JVMPI_SUCCESS;
}

/* Purpose: Dump an instance of java.lang.Class. */
/* Returns: JVMPI_SUCCESS if successful. */
/* NOTE: The format of the class instance dump is as follows:
        u1          recordType;
        jobjectID   class;
        jobjectID   super;
        jobjectID   classloader;
        jobjectID   signers;
        jobjectID   protection_domain;
        jobjectID   class_name; // String object, may be NULL.
        void       *reserved;
        u4          instance_size; // in bytes.
        [jobjectID]* interfaces;
        u2          size_of_constant_pool;
        [u2         constant_pool_index;
         type       type;
         v1]*       value;
        [v1]*       static field values;

   LEVEL 1 dumps excludes all primitive fields.
*/
/* NOTE: Called while GC is disabled or when in GC thread. */
static jint CVMdumperDumpClass(CVMDumper *self, CVMObject *clazz)
{
    CVMExecEnv *ee = CVMgetEE();
    const CVMClassBlock *cb = CVMjvmpiClassInstance2ClassBlock(ee, clazz);
    CVMObject *signer;

    CVMassert(CVMjvmpiGCIsDisabled() || CVMgcIsGCThread(CVMgetEE()));

    /* If the class has not been initialized yet, then we cannot dump it: */
    if (!CVMcbInitializationDoneFlag(ee, cb))
        return JVMPI_FAIL;

    /* %comment l029 */
    signer = NULL;

    CVMdumperWriteU1(self, JVMPI_GC_CLASS_DUMP);
    CVMdumperWriteID(self, clazz);
    {
        CVMClassBlock *superCB = CVMcbSuperclass(cb);
        CVMObject *superObj;
        superObj = (superCB != NULL) ?
                   CVMjvmpiGetICellDirect(ee, CVMcbJavaInstance(superCB)) :
                   NULL;
        CVMdumperWriteID(self, (void *) superObj);
    }
    {
        CVMObjectICell *classloader = CVMcbClassLoader(cb);
        CVMdumperWriteID(self, (classloader != NULL) ?
                                CVMjvmpiGetICellDirect(ee, classloader): NULL);
    }
    CVMdumperWriteID(self, signer);
    {
        CVMObjectICell *protectionDomain = CVMcbProtectionDomain(cb);
        CVMdumperWriteID(self, (protectionDomain != NULL) ?
                         CVMjvmpiGetICellDirect(ee, protectionDomain) : NULL);
    }
    CVMdumperWriteID(self, NULL);  /* Not dumping the name. */
    CVMdumperWriteID(self, NULL);  /* Reserved. */
    CVMdumperWriteU4(self, CVMcbInstanceSize(cb));

    /* Dump the interfaces implemented by this class: */
    {
        CVMUint16 i;
        CVMUint16 count = CVMcbImplementsCount(cb);
        for (i = 0; i < count; i++) {
            CVMClassBlock *interfaceCb = CVMcbInterfacecb(cb, i);
            if (interfaceCb) {
                CVMClassICell *iclazz = CVMcbJavaInstance(interfaceCb);
                CVMObject *interfaceObj =
                    (CVMObject *)CVMjvmpiGetICellDirect(ee, iclazz);
                CVMdumperWriteID(self, interfaceObj);
            } else {
                CVMdumperWriteID(self, NULL);
            }
        }
    }

    /* Dump constant pool info: */
    /* NOTE: CVMcpTypes() is always NULL for ROMized classes.  So we can't get
       the constant pool type for its entries: */
    if (CVMisArrayClass(cb) || CVMcbIsInROM(cb)) {
        CVMdumperWriteU2(self, 0); /* No constant pool in array classes. */
    } else {
        CVMConstantPool *cp = CVMcbConstantPool(cb);
        CVMUint16 i;
        CVMUint16 n = 0;
        CVMUint8 *savedPosition1;
        CVMUint8 *savedPosition2;

        /* Save a place for the number of relevent ConstantPoolEntries: */
        savedPosition1 = CVMdumperGetPosition(self);
        CVMdumperWriteU2(self, 0); /* Write a place holder for now. */

        /* Dump the relevent ConstantPoolEntries: */
        for (i = 1; i < CVMcbConstantPoolCount(cb); i++) {
            if (CVMcpIsResolved(cp, i)) {
                CVMConstantPoolEntryType entryType = CVMcpEntryType(cp, i);
                switch (entryType) {
                    case CVM_CONSTANT_String: {
                        CVMStringICell *str = CVMcpGetStringICell(cp, i);
                        CVMdumperWriteU2(self, i);
                        CVMdumperWriteU1(self, JVMPI_CLASS);
                        CVMdumperWriteID(self,
                                (void *)CVMjvmpiGetICellDirect(ee, str));
                        n++;
                        break;
                    }
                    case CVM_CONSTANT_Class: {
                        CVMClassBlock *classblock = CVMcpGetCb(cp, i);
                        CVMClassICell *clazz = CVMcbJavaInstance(classblock);
                        CVMdumperWriteU2(self, i);
                        CVMdumperWriteU1(self, JVMPI_CLASS);
                        CVMdumperWriteID(self,
                                (void *)CVMjvmpiGetICellDirect(ee, clazz));
                        n++;
                        break;
                    }
                    case CVM_CONSTANT_Long:
                    case CVM_CONSTANT_Double:
                        i++;
                }
            }
        }

        /* Write out the number of relevent ConstantPoolEntries: */
        savedPosition2 = CVMdumperGetPosition(self);
        CVMdumperSeekTo(self, savedPosition1);
        CVMdumperWriteU2(self, n);
        CVMdumperSeekTo(self, savedPosition2);
    }

    /* dump static field values */
    if ((CVMcbFieldCount(cb) > 0) && (CVMcbFields(cb) != NULL)) {
        CVMInt32 dumpLevel = CVMdumperGetLevel(self);
        CVMInt32 i;

        /* NOTE: We're interating through the fields in reverse order.  This is
           intentional because JVMPI list fields in reverse order than that
           which is declared in the ClassFile.  This is especially evident in
           the definition of the JVMPI_GC_INSTANCE_DUMP record where the fields
           of the self class is to preceed that of its super class and so on so
           forth.
        */
        for (i = CVMcbFieldCount(cb); --i >= 0; ) {
            const CVMFieldBlock *fb = CVMcbFieldSlot(cb, i);
            CVMClassTypeID fieldType;

            if (!CVMfbIs(fb, STATIC)) {
                continue;
            }
            fieldType = CVMtypeidGetType(CVMfbNameAndTypeID(fb));
            if (CVMtypeidFieldIsRef(fieldType)) {
                CVMObject *value = (CVMObject *) CVMfbStaticField(ee, fb).raw;
                CVMdumperWriteID(self, value);

            } else if (dumpLevel == JVMPI_DUMP_LEVEL_2) {

#undef DUMP_VALUE
#define DUMP_VALUE(type_, unionField_) {                \
    CVMJava##type_ value;                               \
    value = CVMfbStaticField(ee, fb).unionField_;       \
    CVMdumperWrite##type_(self, value);                 \
}

#undef DUMP_VALUE2
#define DUMP_VALUE2(type_) {                            \
    CVMJava##type_ value;                               \
    value = CVMjvm2##type_(&CVMfbStaticField(ee, fb).raw); \
    CVMdumperWrite##type_(self, value);                 \
}

                switch (fieldType) {
                    case CVM_TYPEID_LONG:    DUMP_VALUE2(Long);      break;
                    case CVM_TYPEID_DOUBLE:  DUMP_VALUE2(Double);    break;
                    case CVM_TYPEID_INT:     DUMP_VALUE(Int,     i); break;
                    case CVM_TYPEID_FLOAT:   DUMP_VALUE(Float,   f); break;
                    case CVM_TYPEID_SHORT:   DUMP_VALUE(Short,   i); break;
                    case CVM_TYPEID_CHAR:    DUMP_VALUE(Char,    i); break;
                    case CVM_TYPEID_BYTE:    DUMP_VALUE(Byte,    i); break;
                    case CVM_TYPEID_BOOLEAN: DUMP_VALUE(Boolean, i); break;
                }
#undef DUMP_VALUE
#undef DUMP_VALUE2

            }
        }
    }
    return JVMPI_SUCCESS;
}

/* Purpose: Dump an array. */
/* Returns: JVMPI_SUCCESS if successful. */
/* NOTE: The format of the primitive aray dump is as follows:
        u1          recordType;
        jobjectID   arrayObj;
        u4          number_of_elements;
        ty          element_type;
        [vi]*       elements[];
   LEVEL 1 dumps excludes all primitive fields.
*/
/* NOTE: The format of the object array dump is as follows:
        u1          recordType;
        jobjectID   arrayObj;
        u4          number_of_elements;
        jobjectID   classObj;
        [jobjectID]* elements[];
*/
/* NOTE: Called while GC is disabled or when in GC thread. */
static jint CVMdumperDumpArray(CVMDumper *self, CVMObject *arrObj)
{
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
#endif
    CVMClassBlock *cb = CVMobjectGetClass(arrObj);
    jint size = CVMD_arrayGetLength((CVMArrayOfAnyType *) arrObj);
    CVMUint8 type = CVMjvmpiGetClassType(cb);

    CVMassert(CVMjvmpiGCIsDisabled() || CVMgcIsGCThread(CVMgetEE()));

    if (type == JVMPI_CLASS) {
        CVMdumperWriteU1(self, JVMPI_GC_OBJ_ARRAY_DUMP);
    } else {
        CVMdumperWriteU1(self, JVMPI_GC_PRIM_ARRAY_DUMP);
    }
    CVMdumperWriteID(self, arrObj);
    CVMdumperWriteU4(self, size);

    if (type == JVMPI_CLASS) {
        jint i;
        CVMdumperWriteID(self,
        (void *)CVMjvmpiGetICellDirect(ee,
                    CVMcbJavaInstance(CVMarrayBaseCb(cb))));
        for (i = 0; i < size; i++) {
            CVMObject *element;
            CVMD_arrayReadRef((CVMArrayOfRef *)arrObj, i, element);
            CVMdumperWriteID(self, element);
        }
    } else {
        CVMdumperWriteU1(self, type);
        if (CVMdumperGetLevel(self) == JVMPI_DUMP_LEVEL_2) {
#undef DUMP_ELEMENTS
#define DUMP_ELEMENTS(type_) {                               \
    jint i;                                                  \
    for (i = 0; i < size; i++) {                             \
        CVMJava##type_ value;                                \
        CVMD_arrayRead##type_(((CVMArrayOf##type_ *)arrObj), \
                              i, value);                     \
        CVMdumperWrite##type_(self, value);                  \
    }                                                        \
}
            switch (type) {
                case JVMPI_BOOLEAN: DUMP_ELEMENTS(Boolean); break;
                case JVMPI_BYTE:    DUMP_ELEMENTS(Byte);    break;
                case JVMPI_SHORT:   DUMP_ELEMENTS(Short);   break;
                case JVMPI_CHAR:    DUMP_ELEMENTS(Char);    break;
                case JVMPI_INT:     DUMP_ELEMENTS(Int);     break;
                case JVMPI_FLOAT:   DUMP_ELEMENTS(Float);   break;
                case JVMPI_LONG:    DUMP_ELEMENTS(Long);    break;
                case JVMPI_DOUBLE:  DUMP_ELEMENTS(Double);  break;
            }
#undef DUMP_ELEMENTS
        }
    }

    return JVMPI_SUCCESS;
}

/* Purpose: Dumps the object info into the buffer specified by the
            DumpContext. */
/* Returns: JVMPI_SUCCESS if successful. */
/* NOTE: Called while GC is disabled or when in GC thread. */
static jint CVMdumperDumpObject(CVMDumper *self, CVMObject *obj)
{
    jint result = JVMPI_SUCCESS;
    CVMClassBlock *cb = CVMobjectGetClass(obj);

    CVMassert(CVMjvmpiGCIsDisabled() || CVMgcIsGCThread(CVMgetEE()));

    /* Do a LEVEL 0 Dump if appropiate: */
    if (CVMdumperGetLevel(self) == JVMPI_DUMP_LEVEL_0) {
        CVMdumperWriteU1(self, CVMjvmpiGetClassType(cb));
        CVMdumperWriteID(self, obj);

    /* Else, do a LEVEL 1 or 2 dump: */
    } else {
        if (CVMisArrayClass(cb)) {
            result = CVMdumperDumpArray(self, obj);
        } else {
            if (cb != (CVMClassBlock*)CVMsystemClass(java_lang_Class)) {
                result = CVMdumperDumpInstance(self, obj);
            } else {
                result = CVMdumperDumpClass(self, obj);
            }
        }
    }
    return result;
}

/* Purpose: Computes the size of a dump of the object instance. */
/* Returns: Size of the dump in bytes. */
/* NOTE: The format of the object instance dump is as follows:
        u1        recordType;
        jobjectID objectSelf;
        jobjectID objectClass;
        u4        number_of_bytes_to_follow;
        [values]  instance_field_values[];
   LEVEL 1 dumps excludes all primitive fields.
*/
/* NOTE: Called while GC is disabled. */
static jsize CVMdumperComputeInstanceDumpSize(CVMDumper *self, CVMObject *obj)
{
    CVMClassBlock *cb = CVMobjectGetClass(obj);
    CVMClassBlock *cb0;
    CVMUint32 dumpLevel = CVMdumperGetLevel(self);
    jsize size;

    CVMassert(CVMjvmpiGCIsDisabled());

    size = /* recordType    */ sizeof(CVMUint8)
         + /* objectID      */ sizeof(jobjectID)
         + /* classObjectID */ sizeof(jobjectID)
         + /* # of bytes    */ sizeof(CVMUint32);

    /* Compute the number of bytes in the field values: */
    for (cb0 = cb; cb0 != NULL; cb0 = CVMcbSuperclass(cb0)) {
        CVMInt32 i;
        for (i = CVMcbFieldCount(cb0); --i >= 0; ) {
            CVMFieldBlock *fb = CVMcbFieldSlot(cb0, i);
            CVMClassTypeID fieldType;
            if (CVMfbIs(fb, STATIC)) {
                continue;
            }
            fieldType = CVMtypeidGetType(CVMfbNameAndTypeID(fb));
            if (CVMtypeidFieldIsRef(fieldType)) {
                size += sizeof(jobjectID);
            } else if (dumpLevel == JVMPI_DUMP_LEVEL_2) {
                switch (fieldType) {
                    case CVM_TYPEID_BOOLEAN:
                    case CVM_TYPEID_BYTE:    size += sizeof(CVMUint8); break;
                    case CVM_TYPEID_CHAR:
                    case CVM_TYPEID_SHORT:   size += sizeof(CVMUint16); break;
                    case CVM_TYPEID_INT:
                    case CVM_TYPEID_FLOAT:   size += sizeof(CVMUint32); break;
                    case CVM_TYPEID_LONG:
                    case CVM_TYPEID_DOUBLE:  size += sizeof(CVMInt64); break;
                }
            }
        }
    }
    return size;
}

/* Purpose: Computes the size of a dump of the class. */
/* Returns: Size of the dump in bytes. */
/* NOTE: The format of the class instance dump is as follows:
        u1          recordType;
        jobjectID   class;
        jobjectID   super;
        jobjectID   classloader;
        jobjectID   signers;
        jobjectID   protection_domain;
        jobjectID   class_name; // String object, may be NULL.
        void       *reserved;
        u4          instance_size; // in bytes.
        [jobjectID]* interfaces;
        u2          size_of_constant_pool;
        [u2         constant_pool_index;
         type       type;
         v1]*       value;
        [v1]*       static field values;

   LEVEL 1 dumps excludes all primitive fields.
*/
/* NOTE: Called while GC is disabled. */
static jsize CVMdumperComputeClassDumpSize(CVMDumper *self, CVMObject *clazz)
{
    CVMExecEnv *ee = CVMgetEE();
    const CVMClassBlock *cb = CVMjvmpiClassInstance2ClassBlock(ee, clazz);
    jsize size;

    CVMassert(CVMjvmpiGCIsDisabled());

    size = /* recordType       */ sizeof(CVMUint8)
         + /* classObjectID    */ sizeof(jobjectID)
         + /* superclassObjID  */ sizeof(jobjectID)
         + /* classloaderObjID */ sizeof(jobjectID)
         + /* signersObjID     */ sizeof(jobjectID)
         + /* protDomainObjID  */ sizeof(jobjectID)
         + /* classnameObjID   */ sizeof(jobjectID)
         + /* reserved         */ sizeof(void *)
         + /* Instance Size    */ sizeof(CVMUint32)
         + /* Interface List   */ 0 /* Added below */
         + /* ConstantPoolSize */ sizeof(CVMUint16)
         + /* ConstantPoolInfo */ 0 /* Added below */
         + /* StaticFields     */ 0 /* Added below */;

    /* Add the size of the interfaces list: */
    size += CVMcbImplementsCount(cb) * sizeof(jobjectID);

    /* Add the size of the constant pool info: */
    /* NOTE: CVMcpTypes() is always NULL for ROMized classes.  So we can't get
       the constant pool type for its entries: */
    if (!CVMisArrayClass(cb) && !CVMcbIsInROM(cb)) {
        CVMConstantPool *cp = CVMcbConstantPool(cb);
        CVMUint16 i;
        CVMUint16 n = 0;

        /* Count the number of relevent ConstantPoolEntries: */
        for (i = 1; i < CVMcbConstantPoolCount(cb); i++) {
            CVMConstantPoolEntryType entryType = CVMcpEntryType(cp, i);
            switch (entryType) {
                case CVM_CONSTANT_String:
                case CVM_CONSTANT_Class:
                    n++;
                    break;
                case CVM_CONSTANT_Long:
                case CVM_CONSTANT_Double:
                    i++;
            }
        }

        size += n * (/* CP Index */ sizeof(CVMUint16)
                  +  /* Type     */ sizeof(CVMUint8)
                  +  /* Value    */ sizeof(jobjectID));

    }

    /* Add the size of the static field values: */
    if ((CVMcbFieldCount(cb) > 0) && (CVMcbFields(cb) != NULL)) {
        CVMInt32 dumpLevel = CVMdumperGetLevel(self);
        CVMInt32 i;

        for (i = CVMcbFieldCount(cb); --i >= 0; ) {
            const CVMFieldBlock *fb = CVMcbFieldSlot(cb, i);
            CVMClassTypeID fieldType;

            if (!CVMfbIs(fb, STATIC)) {
                continue;
            }
            fieldType = CVMtypeidGetType(CVMfbNameAndTypeID(fb));
            if (CVMtypeidFieldIsRef(fieldType)) {
                size += sizeof(jobjectID);
            } else if (dumpLevel == JVMPI_DUMP_LEVEL_2) {
                switch (fieldType) {
                    case CVM_TYPEID_BOOLEAN:
                    case CVM_TYPEID_BYTE:    size += sizeof(CVMUint8); break;
                    case CVM_TYPEID_CHAR:
                    case CVM_TYPEID_SHORT:   size += sizeof(CVMUint16); break;
                    case CVM_TYPEID_INT:
                    case CVM_TYPEID_FLOAT:   size += sizeof(CVMUint32); break;
                    case CVM_TYPEID_LONG:
                    case CVM_TYPEID_DOUBLE:  size += sizeof(CVMInt64); break;
                }
            }
        }
    }
    return size;
}

/* Purpose: Computes the size of a dump of the array. */
/* Returns: Size of the dump in bytes. */
/* NOTE: The format of the primitive aray dump is as follows:
        u1          recordType;
        jobjectID   arrayObjID;
        u4          number_of_elements;
        ty          element_type;
        [vi]*       elements[];
   LEVEL 1 dumps excludes all primitive fields.
*/
/* NOTE: The format of the object array dump is as follows:
        u1          recordtype;
        jobjectID   arrayObj;
        u4          number_of_elements;
        jobjectID   classObj;
        [jobjectID]* elements[];
*/
/* NOTE: Called while GC is disabled. */
static jsize CVMdumperComputeArrayDumpSize(CVMDumper *self, CVMObject *arrObj)
{
    CVMClassBlock *cb = CVMobjectGetClass(arrObj);
    jint length = CVMD_arrayGetLength((CVMArrayOfAnyType *) arrObj);
    jsize size;

    CVMassert(CVMjvmpiGCIsDisabled());

    size = /* recordType    */ sizeof(CVMUint8)
         + /* arrayObjID    */ sizeof(jobjectID)
         + /* # of elements */ sizeof(CVMUint32);

    if (CVMarrayBaseType(cb) == CVM_T_CLASS) {
        size += /* elementType */ sizeof(void *)
              + /* elements    */ (length * sizeof(jobjectID));
    } else {
        size += /* elementType */ sizeof(CVMUint8);
        if (CVMdumperGetLevel(self) == JVMPI_DUMP_LEVEL_2) {
            /* Add size of elements: */
            switch (CVMarrayBaseType(cb)) {
                case CVM_T_BOOLEAN:
                case CVM_T_BYTE:   size += (length * sizeof(CVMUint8)); break;
                case CVM_T_CHAR:
                case CVM_T_SHORT:  size += (length * sizeof(CVMUint16)); break;
                case CVM_T_INT:
                case CVM_T_FLOAT:  size += (length * sizeof(CVMUint32)); break;
                case CVM_T_LONG:
                case CVM_T_DOUBLE: size += (length * sizeof(CVMInt64)); break;
                default:
                    /* We should never get here: */
                    CVMassert(CVM_FALSE);
            }
        }
    }
    return size;
}

/* Purpose: Computes the size of the dump for the object. */
/* Returns: Size of the dump in bytes. */
/* NOTE: Called while GC is disabled. */
static jsize CVMdumperComputeObjectDumpSize(CVMDumper *self, CVMObject *obj)
{
    CVMClassBlock *cb = CVMobjectGetClass(obj);
    jsize size = 0;

    CVMassert(CVMjvmpiGCIsDisabled());

    /* Compute a LEVEL 0 Dump if appropiate: */
    if (CVMdumperGetLevel(self) == JVMPI_DUMP_LEVEL_0) {
        /* Level 0 Object Dump Format:
               u1        objectType;
               jobjectID objectID;
        */
        size = /* objectType */ sizeof(CVMUint8)
             + /* objectID   */ sizeof(jobjectID);

    /* Else, compute a LEVEL 1 or 2 dump: */
    } else {
        if (CVMisArrayClass(cb)) {
            size = CVMdumperComputeArrayDumpSize(self, obj);
        } else {
            if (cb != (CVMClassBlock*)CVMsystemClass(java_lang_Class)) {
                size = CVMdumperComputeInstanceDumpSize(self, obj);
            } else {
                size = CVMdumperComputeClassDumpSize(self, obj);
            }
        }
    }
    return size;
}

/* Purpose: Fill in the dump information for each thread. */
/* NOTE: Cannot go GC safe in here because we're already holding a few
         CVMSysMutexes. */
static void CVMdumperDumpThreadInfo(CVMDumper *self, CVMExecEnv *threadEE)
{
    CVMThreadICell *threadICell = CVMcurrentThreadICell(threadEE);
    if ((threadICell != NULL) && !CVMID_icellIsNull(threadICell)
	&& !threadEE->hasPostedExitEvents)
    {
        CVMdumperDumpThreadTrace(self, threadEE);
    }
}

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE

/* Purpose: Fill in the Fast Lock information for each thread. */
static void CVMdumperDumpFastLockInfo(CVMDumper *self, CVMExecEnv *threadEE)
{
    CVMOwnedMonitor *record = threadEE->objLocksOwned;

    for (; record != NULL; record = record->next) {
        CVMObject *obj = record->object;
        if (CVMhdrBitsSync(obj->hdr.various32) == CVM_LOCKSTATE_LOCKED) {
            /* Write the Monitor type: */
            CVMdumperWriteU1(self, JVMPI_MONITOR_JAVA);

            /* Write the monitor ID i.e. the CVMObject *: */
            CVMdumperWriteID(self, obj);

            /* Indicate the monitor's owner and the number of times this
               monitor has been entered: */
            CVMdumperWriteID(self, (void *)CVMexecEnv2JniEnv(threadEE));
            CVMdumperWriteU4(self, record->count);

            /* There are no other threads waiting to enter on this monitor.
               That's why we have a fast lock in the first place: */
            CVMdumperWriteU4(self, 0);

            /* There are no other threads waiting to be notified bn this
               monitor.  This is because wait() and notify() only operates on
               inflated monitors (not fast locks): */
            CVMdumperWriteU4(self, 0);
        }
    }
}
#endif

/* Purpose: Fill in the dump information for each monitor. */
/* NOTE: Called while GC safe and while all threads are consistent as
         requested by the self thread.  Hence, cannot go GC unsafe in here. */
static void CVMdumperDumpMonitor(CVMDumper *self, CVMProfiledMonitor *pm)
{
#ifdef CVM_DEBUG_ASSERTS /* Used for assertion in CVM_WALK_ALL_THREADS. */
    CVMExecEnv *ee = CVMgetEE();

    CVMassert(CVMD_isgcSafe(ee));
#endif

    switch (CVMprofiledMonitorGetType(pm)) {
        case CVM_LOCKTYPE_OBJ_MONITOR: {
            CVMObjMonitor *mon;

            /* Object Monitor Dump Format:
                u1               recordType;
                jobjectID        objectID;
                ...
            */
            mon = CVMcastProfiledMonitor2ObjMonitor(pm);
	    if (mon->state == CVM_OBJMON_FREE){
		/* This is an unused monitor.
		 * It has no object and no owner.
		 * Skip it altogether.
		 */
		return;
	    }
	    CVMassert(CVMobjMonitorDirectObject(mon) != NULL);
	    CVMdumperWriteU1(self, JVMPI_MONITOR_JAVA);
	    CVMdumperWriteID(self, (void *) CVMobjMonitorDirectObject(mon));
            break;
        }
        case CVM_LOCKTYPE_NAMED_SYSMONITOR: {
            CVMNamedSysMonitor *mon;

            /* Raw Monitor Dump Format:
                u1               recordType;
                char *           rawMonitorName;
                JVMPI_RawMonitor rawMonitorID;
                ...
            */
            mon = CVMcastProfiledMonitor2NamedSysMonitor(pm);
            CVMdumperWriteU1(self, JVMPI_MONITOR_RAW);
            CVMdumperWriteID(self, (void *) CVMnamedSysMonitorGetName(mon));
            CVMdumperWriteID(self, (void *) mon);
            break;
        }
    }

    /* The rest of the Monitor Dump Format:
        ...
        JNIEnv *    ownerThread;
        u4          entryCount;
        u4          numberOfThreadsWaitingToEnter;
        [JNIEnv *]  threadsWaitingToEnter;
        u4          numberOfThreadsWaitingToBeNotified;
        [JNIEnv *]  threadsWaitingToBeNotified;
    */

    /* Dump owner and entryCount info: */
    {
        CVMExecEnv *owner = CVMprofiledMonitorGetOwner(pm);
        if (owner != NULL) {
            /* Indicate that the monitor's owner and the number of times this
               monitor has been entered: */
            CVMdumperWriteID(self, (void *)CVMexecEnv2JniEnv(owner));
            CVMdumperWriteU4(self, CVMprofiledMonitorGetCount(pm));
        } else {
            /* Indicate that this monitor is not owned: */
            CVMdumperWriteID(self, NULL);
            CVMdumperWriteU4(self, 0);
        }
    }

    /* Dump the list of threads waiting to enter the monitor: */
    {
        CVMUint8 *pos1 = CVMdumperGetPosition(self);
        CVMUint8 *pos2;
        CVMUint32 numberOfWaiters = 0;

        CVMdumperWriteU4(self, 0); /* Dump place holder. */
        CVM_WALK_ALL_THREADS(ee, threadEE, {
            if (threadEE->blockingLockEntryMonitor == pm) {
                CVMdumperWriteID(self, (void *)CVMexecEnv2JniEnv(threadEE));
                numberOfWaiters++;
            }
        });
        pos2 = CVMdumperGetPosition(self);
        CVMdumperSeekTo(self, pos1);
        CVMdumperWriteU4(self, numberOfWaiters); /* Dump the actual count. */
        CVMdumperSeekTo(self, pos2);
    }

    /* Dump the list of threads waiting to be notified by the monitor: */
    {
        CVMUint8 *pos1 = CVMdumperGetPosition(self);
        CVMUint8 *pos2;
        CVMUint32 numberOfWaiters = 0;

        CVMdumperWriteU4(self, 0); /* Dump place holder. */
        CVM_WALK_ALL_THREADS(ee, threadEE, {
            if (threadEE->blockingWaitMonitor == pm) {
                CVMdumperWriteID(self, (void *)CVMexecEnv2JniEnv(threadEE));
                numberOfWaiters++;
            }
        });
        pos2 = CVMdumperGetPosition(self);
        CVMdumperSeekTo(self, pos1);
        CVMdumperWriteU4(self, numberOfWaiters); /* Dump the actual count. */
        CVMdumperSeekTo(self, pos2);
    }
}

/* Purpose: Collects the monitor dump info. */
/* NOTE: Only called by CVMjvmpiPostMonitorDumpEvent().
         CVMjvmpiPostMonitorDumpEvent() is responsible for having acquired the
         CVMglobals.threadLock before calling this method.
         CVM_WALK_ALL_THREADS() assumes this is true. */
static void CVMdumperDumpThreadsAndMonitors(CVMDumper *self, CVMExecEnv *ee)
{
    CVMsysMutexLock(ee, &CVMglobals.syncLock);

    /* NOTE: The caller of CVMdumperDumpThreadsAndMonitors() should already
        have acquired the threadLock.  Hence, it is not necessary to acquire
        it again: */
    CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.threadLock));

    /* NOTE: The following must be done in the specified order: */

    /* Step 1: Fill in the stack traces at the top of the Dumper buffer: */

    /* Iterate though all threads and fill in a JVMPI_CallTrace record for
       each thread.  For each of these JVMPI_CallTrace records, fill in
       a list of JVMPI_CallFrame records for each of the stack frames in the
       thread: */
    CVM_WALK_ALL_THREADS(ee, threadEE, {
        CVMdumperDumpThreadInfo(self, threadEE);
    });

    /* Step 2: Fill in the list of thread status info next: */
    {
        CVMUint32 sizeOfList = self->tracesCount * sizeof(jint);
        jint *threadStatusList = (jint *) CVMdumperAlloc(self, sizeOfList);

        /* If we have the needed memory, then fill in the list: */
        if (threadStatusList != NULL) {
            /* Fill in the thread thread status of each thread: */
            int i;
            for (i = 0; i < self->tracesCount; i++) {
                CVMExecEnv *targetEE;
                targetEE = CVMjniEnv2ExecEnv(self->traces[i].env_id);
                threadStatusList[i] = CVMjvmpiGetThreadStatus(targetEE);
            }
        }
        CVMdumperSetThreadStatusList(self, threadStatusList);
    }

    /* Step 3: Fill in the dump info on monitors: */

    /* Mark the beginning of the next set of dump info: */
    CVMdumperSetDumpStart(self, CVMdumperGetPosition(self));

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
    /* Iterate though all threads and fill in Java Object Monitor info for
       fast locks in each thread: */
    CVM_WALK_ALL_THREADS(ee, threadEE, {
        CVMdumperDumpFastLockInfo(self, threadEE);
    });
#endif

    CVMsysMutexLock(ee, &CVMglobals.jvmpiSyncLock);

    /* Iterate over each CVMObjMonitor in the VM and dump them: */
    {
        CVMProfiledMonitor *mon = CVMglobals.objMonitorList;
        while(mon != NULL) {
            CVMdumperDumpMonitor(self, mon);
            mon = mon->next;
        }
    }
    /* Iterate over each Raw Monitor in the VM and dump them: */
    {
        CVMProfiledMonitor *mon = CVMglobals.rawMonitorList;
        while (mon != NULL) {
            CVMdumperDumpMonitor(self, mon);
            mon = mon->next;
        }
    }
    CVMsysMutexUnlock(ee, &CVMglobals.jvmpiSyncLock);

    CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
}

static CVMBool
CVMdumperHeapDumpCallback(CVMObject *obj, CVMClassBlock *cb, CVMUint32 size,
                          void *data)
{
    CVMDumper *self = (CVMDumper *) data;
    CVMdumperDumpObject(self, obj);
    return CVM_TRUE;
}

/* Purpose: Fills in info for a JVMPI_EVENT_HEAP_DUMP event. */
/* NOTE: Based on CVMgcStopTheWorldAndGCSafe(). */
static void CVMdumperDumpHeap(CVMDumper *self, CVMExecEnv *ee)
{
    CVMassert(CVMD_isgcSafe(ee));

    /* And get the rest of the GC locks: */
    CVMlocksForGCAcquire(ee);

    /* Roll all threads to GC-safe points: */
    CVMD_gcBecomeSafeAll(ee);

    /* Iterate though all threads and fill in a JVMPI_CallTrace record for
       each thread.  For each of these JVMPI_CallTrace records, fill in
       a list of JVMPI_CallFrame records for each of the stack frames in the
       thread: */
    CVM_WALK_ALL_THREADS(ee, threadEE, {
        CVMdumperDumpThreadInfo(self, threadEE);
    });

    /* Set the start of the dump after the thread trace info above: */
    CVMdumperSetDumpStart(self, CVMdumperGetPosition(self));

    if (CVMgcEnsureStackmapsForRootScans(ee)) {
        CVMGCOptions gcOpts = {
        /* isUpdatingObjectPointers */ CVM_FALSE,
            /* discoverWeakReferences   */ CVM_FALSE,
            /* isProfilingPass          */ CVM_TRUE
        };

        CVMsysMutexLock(ee, &CVMglobals.threadLock);
        CVMgcScanRoots(ee, &gcOpts, CVMdumperDumpRoot, self);
        CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
    } else {
        /* We should always be able to compute the stackmaps: */
        CVMassert(CVM_FALSE);
    }

    /* Dump every object in the heap: */
    CVMgcimplIterateHeap(ee, CVMdumperHeapDumpCallback, self);

    /* Allow threads to become GC-unsafe again: */
    CVMD_gcAllowUnsafeAll(ee);

    CVMlocksForGCRelease(ee);
}

/*=============================================================================
    PRIVATE Helper and Utility methods of the JVMPI implementation:
*/

/* Purpose: Frees the specified memory if the pointer is not NULL. */
#undef freeIfNotNull
#define freeIfNotNull(x)    \
    if ((x) != NULL) {      \
        free(x);            \
    }

/* Purpose: Gets the ClassBlock from the specified java.lang.Class instance. */
static const CVMClassBlock *
CVMjvmpiClassInstance2ClassBlock(CVMExecEnv *ee, CVMObject *obj)
{
    CVMJavaInt cbAddr;
    CVMClassBlock *cb;

    /* The following condition will have to be true to guarantee that the
       CVMObject *obj that is passed in remains valid for the duration of this
       function, and that we can access the
       CVMoffsetOfjava_lang_Class_classBlockPointer directly below: */
    CVMassert(CVMjvmpiIsSafeToAccessDirectObject(ee));

    CVMD_fieldReadInt(obj,
        CVMoffsetOfjava_lang_Class_classBlockPointer, cbAddr);
    cb = (CVMClassBlock*) cbAddr;
    CVMassert(cb != NULL);

    return cb;
}

/* Purpose: Disables all notification to the profiler. */
static void CVMjvmpiDisableAllNotification(CVMExecEnv *ee)
{
    CVMjvmpiEventInfoDisableAll(&CVMjvmpiRec()->eventInfo, ee);
}

/* Purpose: Disables the GC for JVMPI purposes. */
static void CVMjvmpiDisableGC(CVMExecEnv *ee)
{
    /* By the time we return from this function, we know that GC will not run
       in such a way that will move or delete objects.  A GC cycle which may
       have started already may appear to complete after this call but the
       intergrity of object pointers will not be jeopardized because no actual
       GC'ing will be done.  If actual GC'ing is in progress, then
       CVMgcLockerLock() will wait until it is done before locking out
       GC.  This guarantees that no actual GC activity will run once we exit
       from this function:
    */
    CVMgcLockerLock(&CVMjvmpiRec()->gcLocker, ee);
}

/* Purpose: Enables the GC for JVMPI purposes. */
static void CVMjvmpiEnableGC(CVMExecEnv *ee)
{
    /* CVMgcLockerUnlock() will do nothing if GC is not disabled: */
    CVMgcLockerUnlock(&CVMjvmpiRec()->gcLocker, ee);
}

/* Purpose: Gets the JVMPI type for the specified object. */
static CVMUint8 CVMjvmpiGetClassType(const CVMClassBlock *cb)
{
    CVMInt32 type = JVMPI_NORMAL_OBJECT;
    if (CVMisArrayClass(cb)) {
        switch(CVMarrayBaseType(cb)) {
            case CVM_T_BOOLEAN: type = JVMPI_BOOLEAN;   break;
            case CVM_T_BYTE:    type = JVMPI_BYTE;      break;
            case CVM_T_CHAR:    type = JVMPI_CHAR;      break;
            case CVM_T_SHORT:   type = JVMPI_SHORT;     break;
            case CVM_T_INT:     type = JVMPI_INT;       break;
            case CVM_T_LONG:    type = JVMPI_LONG;      break;
            case CVM_T_FLOAT:   type = JVMPI_FLOAT;     break;
            case CVM_T_DOUBLE:  type = JVMPI_DOUBLE;    break;
            case CVM_T_CLASS:   type = JVMPI_CLASS;     break;
            case CVM_T_VOID:    
            case CVM_T_ERR:
	        CVMassert(CVM_FALSE);
        }
    }
    return type;
}

/* Purpose: Gets the line number for the specified method and frame. */
static CVMInt32 CVMjvmpiGetLineNumber(CVMMethodBlock *mb, CVMFrame *frame)
{
    CVMInt32 lineno = -3;
    if (CVMframeIsJava(frame)) {
        CVMUint8* pc = CVMframePc(frame);
	CVMUint16 pc_offset = (CVMUint16)(pc - CVMmbJavaCode(mb));
        lineno = CVMpc2lineno(mb, pc_offset);
    }
    return lineno;
}

/* Purpose: Gets the execution status of the specified thread. */
static CVMUint16 CVMjvmpiGetThreadStatus(CVMExecEnv *ee)
{
    CVMUint16 status = JVMPI_THREAD_RUNNABLE;
    if (ee->blockingLockEntryMonitor != NULL) {
        status = JVMPI_THREAD_MONITOR_WAIT;
    } else if (ee->blockingWaitMonitor != NULL) {
        status = JVMPI_THREAD_CONDVAR_WAIT;
    }
    return status;
}

/* Purpose: Performs clean-up for the JVMPI operation.  This method is only
            called when the VM exits. */
/* NOTE: Called while GC safe. */
static void CVMjvmpiDoCleanup(void)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMassert(CVMD_isgcSafe(ee));
    if (CVMjvmpiEventJVMShutDownIsEnabled()) {
        JVMPI_Event event;

        event.env_id = CVMexecEnv2JniEnv(ee);
        event.event_type = JVMPI_EVENT_JVM_SHUT_DOWN;
        CVMjvmpiNotifyEvent(&event);
    }
    CVMjvmpiDisableAllNotification(ee);
}

/*=============================================================================
    Methods which make up the JVMPI Interface:
*/

/* Purpose: NotifyEvent() is supposed to be initialized by the profiler agent
            as soon as it gets the JVMPI_Interface structure.  We initialize it
            to this stub function to detect the case where the profiler agent
            fails to initialize it. */
static void CVMjvmpi_NotifyEvent(JVMPI_Event *event)
{
    /* Make sure that NotifyEvent() isn't this stub function.  If we get here,
       then the profiler agent must have failed to initialized NotifyEvent().
       The following assertion is done to get a more intelligible error message
       than CVMassert(CVM_FALSE). */
    CVMassert(CVMjvmpiInterface()->NotifyEvent != CVMjvmpi_NotifyEvent);
}

/* Purpose: Enables event notification. */
static jint CVMjvmpi_EnableEvent(jint event_type, void *arg)
{
    CVMJvmpiEventInfo *info = &CVMjvmpiRec()->eventInfo;
    CVMExecEnv *ee = CVMgetEE();
#ifdef CVM_DEBUG_ASSERTS
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif

    /* event type is greater than the max value for event types */
    if (event_type > JVMPI_MAX_EVENT_TYPE_VAL) {
       return JVMPI_NOT_AVAILABLE;
    }
    if (CVMjvmpiEventInfoIsNotAvailable(info, event_type)) {
        /* Cannot enable/disable this event type. */
        return JVMPI_NOT_AVAILABLE;
    }
    CVMjvmpiEventInfoEnable(info, ee, event_type);

    return JVMPI_SUCCESS;
}

/* Purpose: Disable event notification. */
static jint CVMjvmpi_DisableEvent(jint event_type, void *arg)
{
    CVMJvmpiEventInfo *info = &CVMjvmpiRec()->eventInfo;
    CVMExecEnv *ee = CVMgetEE();
#ifdef CVM_DEBUG_ASSERTS
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif

    /* event type is greater than the max value for event types */
    if (event_type > JVMPI_MAX_EVENT_TYPE_VAL) {
       return JVMPI_NOT_AVAILABLE;
    }
    if (CVMjvmpiEventInfoIsNotAvailable(info, event_type)) {
        return JVMPI_NOT_AVAILABLE;
    }
    CVMjvmpiEventInfoDisable(info, ee, event_type);

    return JVMPI_SUCCESS;
}

/* Purpose: Requests an event from the VM. */
static jint CVMjvmpi_RequestEvent(jint event_type, void *arg)
{
    CVMExecEnv *ee = CVMgetEE();
    jint result = JVMPI_SUCCESS;

    CVMassert(ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));

    switch (event_type) {
    case JVMPI_EVENT_OBJ_ALLOC:
        CVMassert(CVMjvmpiGCIsDisabled());
        CVMjvmpiPostObjectAllocEvent2(ee, (CVMObject *)arg, CVM_TRUE);
        break;
    case JVMPI_EVENT_THREAD_START:
        CVMassert(CVMjvmpiGCIsDisabled());
        if (CVMgcIsGCThread(ee)) {
	    /*
	     * This can happen when, for example, hprof
	     * is assessing the states of all monitors during
	     * shutdown.
	     */
            result = JVMPI_FAIL;
        } else {
            CVMObject *threadObj = (CVMObject *)arg;
#ifdef CVM_INSPECTOR
            CVMjvmpiDisableGC(ee);
            CVMassert(CVMgcIsValidObject(ee, threadObj));
            CVMjvmpiEnableGC(ee);
#endif
            result = CVMjvmpiPostThreadStartEvent2(ee, threadObj, CVM_TRUE);
        }
        break;

    case JVMPI_EVENT_CLASS_LOAD: 
        CVMassert(CVMjvmpiGCIsDisabled());
        {
            const CVMClassBlock *cb;
#ifdef CVM_INSPECTOR
            CVMjvmpiDisableGC(ee);
            CVMassert(CVMgcIsValidObject(ee, (CVMObject *) arg));
            CVMjvmpiEnableGC(ee);
#endif
            cb = CVMjvmpiClassInstance2ClassBlock(ee, (CVMObject *) arg);
            result = CVMjvmpiPostClassLoadEvent2(ee, cb, CVM_TRUE);
        }
        break;

    case JVMPI_EVENT_OBJECT_DUMP:
        CVMassert(CVMjvmpiGCIsDisabled());
        result = CVMjvmpiPostObjectDumpEvent((CVMObject *) arg);
        break;

    case JVMPI_EVENT_HEAP_DUMP: {
        int heap_dump_level;

        if (arg == NULL) {
            heap_dump_level = JVMPI_DUMP_LEVEL_2;
        } else {
            heap_dump_level = ((JVMPI_HeapDumpArg *)arg)->heap_dump_level;
        }

        CVMjvmpiPostHeapDumpEvent(ee, heap_dump_level);
        break;
    }
    case JVMPI_EVENT_MONITOR_DUMP:
        CVMjvmpiPostMonitorDumpEvent(ee);
        break;

    default:
        result = JVMPI_NOT_AVAILABLE;
    }

    return result;
}

/* Purpose: Fills in a stack trace for the specified thread. */
/* NOTE: Called while GC safe.  This function is NOT allowed to become GC
   unsafe because it may be called from the heap dump which forces all threads
   to become consistent.  Hence, calling JNI APIs are also not allowed. */
static void CVMjvmpi_GetCallTrace(JVMPI_CallTrace *trace, jint depth)
{
    JNIEnv *targetJNIEnv = trace->env_id;
    CVMExecEnv *targetEE = CVMjniEnv2ExecEnv(targetJNIEnv);
    CVMStack *stack = &targetEE->interpreterStack;
    CVMStackWalkContext c;
    CVMFrame *frame;
    CVMUint32 framesCount = 0;

#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif
    CVMstackwalkInit(stack, &c);
    frame = c.frame;
    while ((frame != 0) && (framesCount < depth)) {
        CVMMethodBlock *mb = frame->mb;
        if (!CVMframeIsTransition(frame) && (mb != NULL)) {
            /* Fill in the frame info in the trace: */
            JVMPI_CallFrame *jframe = &trace->frames[framesCount];
            jframe->lineno = CVMjvmpiGetLineNumber(mb, frame);
            jframe->method_id = (jmethodID) mb;
            framesCount++;
        }
        /* Get previous frame: */
        CVMstackwalkPrev(&c);
        frame = c.frame;
    }
    trace->num_frames = framesCount;
}


/* Purpose: Called by the profiler agent to inform the VM that the profiler
            wants to exit with the specified exit code. This function causes
            the VM to also exit with the specified exit code. */
/* NOTE: This implementation is based on JDK1.3 Classic VM's implementation. */
static void CVMjvmpi_ProfilerExit(jint exit_code) 
{
    CVMExecEnv *ee = CVMgetEE();
#ifdef CVM_DEBUG_ASSERTS
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif
    CVMjvmpiDisableAllNotification(ee);

    /* %comment l030 */
    CVMexit(exit_code);
}

/* Purpose: Called by the profiler to create a raw monitor. */
/* NOTE: Raw monitors are similar to Java monitors. The difference is that raw
         monitors are not associated with Java objects. 

         It is not safe for the profiler agent to call this function in the
         thread suspended mode because this function may call arbitrary system
         functions such as malloc and block on an internal system library lock.

         If the raw monitor is created with a name beginning with an
         underscore ('_'), then its monitor contention events are not sent to
         the profiler agent.  This is specified in the JVMPI specification
         for RawMonitorCreate().

         This enforced in CVMjvmpiPostMonitorContendedEnterEvent(),
         CVMjvmpiPostMonitorContendedEnteredEvent(), and
         CVMjvmpiPostMonitorContendedExitEvent().
*/
/* NOTE: Called while GC safe. */
static JVMPI_RawMonitor CVMjvmpi_RawMonitorCreate(char *lock_name) {
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif
    return (JVMPI_RawMonitor)CVMnamedSysMonitorInit(lock_name);
}

/* Purpose: Enters the specified monitor. */
/* NOTE: Raw monitors are similar to Java monitors. The difference is that raw
         monitors are not associated with Java objects.

         It is not safe for the profiler agent to call this function in the
         thread suspended mode because the current thread may block on the raw
         monitor already acquired by one of the suspended threads.
*/
static void CVMjvmpi_RawMonitorEnter(JVMPI_RawMonitor lock_id)
{
    CVMNamedSysMonitor *mon = (CVMNamedSysMonitor *) lock_id;
    if (mon != NULL) {
        CVMExecEnv *ee = CVMgetEE();

        CVMassert (ee != NULL);
        CVMassert(CVMD_isgcSafe(ee));
        /* NOTE: We need to use TryEnter() first because we need to be able to
            detect contention cases.  CVMnamedSysMonitorEnter() has been
            tagged exclusively for contended cases: */
        if (CVMnamedSysMonitorTryEnter(mon, ee) == CVM_FALSE) {
            CVMnamedSysMonitorEnter(mon, ee);
        }
    }
}

/* Purpose: Release the specified monitor. */
/* NOTE: Raw monitors are similar to Java monitors. The difference is that raw
         monitors are not associated with Java objects.
*/
static void CVMjvmpi_RawMonitorExit(JVMPI_RawMonitor lock_id)
{
    CVMNamedSysMonitor *mon = (CVMNamedSysMonitor *) lock_id;
    if (mon != NULL) {
        CVMExecEnv *ee = CVMgetEE();
        CVMassert (ee != NULL);
        CVMassert(CVMD_isgcSafe(ee));
        CVMnamedSysMonitorExit(mon, ee);
    }
}

/* Purpose: Wait for notification of specified monitor. 
   NOTE: ms is the timeout period.  Passing 0 for ms causes the thread to wait
         forever.

         Raw monitors are similar to Java monitors. The difference is that raw
         monitors are not associated with Java objects.

         CVM note: if the raw monitor is not owned by the thread executing
         RawMonitorWait(), waiting will not occur.
*/
static void CVMjvmpi_RawMonitorWait(JVMPI_RawMonitor lock_id, jlong ms)
{
    CVMNamedSysMonitor *mon = (CVMNamedSysMonitor *) lock_id;
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
    CVMnamedSysMonitorWait(mon, ee, ms);
}

/* Purpose: Notifies all threads waiting on the specified monitor. */
/* NOTE: Raw monitors are similar to Java monitors. The difference is that raw
         monitors are not associated with Java objects. 

         CVM note: if the raw monitor is not owned by the thread executing
         RawMonitorNotifyAll(), NotifyAll will fail.
*/
static void CVMjvmpi_RawMonitorNotifyAll(JVMPI_RawMonitor lock_id)
{
    CVMNamedSysMonitor *mon = (CVMNamedSysMonitor *) lock_id;
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
    CVMnamedSysMonitorNotifyAll(mon, ee);
}

/* Purpose: Destroys the specified monitor. */
/* NOTE: Raw monitors are similar to Java monitors. The difference is that raw
         monitors are not associated with Java objects. 

         It is not safe for the profiler agent to call this function in the
         thread suspended mode because this function may call arbitrary system
         functions such as free and block on a internal system library lock.
*/
/* NOTE: Called while GC safe. */
static void CVMjvmpi_RawMonitorDestroy(JVMPI_RawMonitor lock_id)
{
    CVMNamedSysMonitor *mon = (CVMNamedSysMonitor *) lock_id;
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif
    if (mon != NULL) {
        CVMnamedSysMonitorDestroy(mon);
    }
}

/* Purpose: This function is supposed to return the amount of time consumed by
            the current thread.  However, the JDK classic VM only returns the
            current global CPU time.  The following implementation is
            compatible with the JDK classic VM's implementation. */
/* Returns: The current thread's CPU time in nanoseconds. */
static jlong CVMjvmpi_GetCurrentThreadCpuTime(void)
{
    jlong time_ms = CVMtimeMillis();
    jlong conversion_factor =  CVMint2Long(1000000);
    jlong time_ns = CVMlongMul(time_ms, conversion_factor);
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif
    return time_ns;
}

/* Purpose: Suspends the specified thread. */
/* NOTE: The JVMPI specs says that java.lang.Thread.resume() cannot be used
         to resume a thread suspended by this method.  However,
         java.lang.Thread.resume() is deprecated in the CDC which CVM works
         with.  Hence, this is not an issue. */
static void CVMjvmpi_SuspendThread(JNIEnv *env)
{
    CVMExecEnv *targetEE = CVMjniEnv2ExecEnv(env);
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif
    CVMthreadSuspend(&targetEE->threadInfo);
}

/* Purpose: Resume the specified thread. */
/* NOTE: The JVMPI specs says that this method cannot be used to resume a
         thread suspended by java.lang.Thread.suspend() .  However,
         java.lang.Thread.suspend() is deprecated in the CDC which CVM works
         with.  Hence, this is not an issue. */
static void CVMjvmpi_ResumeThread(JNIEnv *env)
{
    CVMExecEnv *targetEE = CVMjniEnv2ExecEnv(env);
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif

    /* Reset the hasRun flag of the thread.  When the thread runs, this flag
       will be set again at regular intervals.  This is how we know if the
       thread has run since the last suspension.  See also ThreadHasRun(). */
    CVMeeResetHasRun(targetEE);
    CVMthreadResume(&targetEE->threadInfo);
}

/* Purpose: Gets the execution status of the specified thread. */
static jint CVMjvmpi_GetThreadStatus(JNIEnv *env)
{
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif
    return CVMjvmpiGetThreadStatus(CVMjniEnv2ExecEnv(env));
}

/* Purpose: Checks to see if the specified thread has run between the last
            JVMPI resumption and the current suspension.   It is assumed
            that the thread has been suspended by the profiler agent. */
static jboolean CVMjvmpi_ThreadHasRun(JNIEnv *env)
{
    CVMExecEnv *targetEE = CVMjniEnv2ExecEnv(env);
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif
    /* The hasRun flag of the thread is reset in ResumeThread().  When the
       thread runs, this flag will be set again at regular intervals.  This
       is how we know if the thread has run since the last suspension.  See
       also ResumeThread(). */
    return CVMeeHasRun(targetEE);
}

/* Purpose: Creates a system thread. */
/* NOTE: The following actually creates an instance of java.lang.Thread.
   The only difference is that the thread is marked as a daemon thread.
   This is compliant with the JDK Classic VM's implementation. */
/* NOTE: Called while GC safe. */
static jint CVMjvmpi_CreateSystemThread(char *name, jint priority,
                                        jvmpi_void_function_of_void function)
{
    CVMExecEnv *ee = CVMgetEE();
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    jobject threadObj;
    jclass threadClass;
    jstring threadName;

    CVMassert(ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));

    /* Obtain the correct thread priority: */
    if (priority < JVMPI_MINIMUM_PRIORITY) {
        priority = JVMPI_MINIMUM_PRIORITY;
    } else if (priority > JVMPI_MAXIMUM_PRIORITY) {
        priority = JVMPI_MAXIMUM_PRIORITY;
    }

    /* Instantiate a daemon (i.e. system) thread: */
    threadClass = CVMcbJavaInstance(CVMsystemClass(java_lang_Thread));
    threadName = CVMjniNewStringUTF(env, name);
    threadObj = CVMjniCallStaticObjectMethod(
        env, threadClass,
	CVMglobals.java_lang_Thread_initDaemonThread,
	threadName, priority);

    /* Start the thread: */
    if (threadObj != NULL) {
	JVM_StartSystemThread(env, threadObj, function, NULL);
    }

    /* Make sure that we're successful in starting the thread: */
    if (threadObj == NULL || CVMexceptionOccurred(ee)) {
        CVMclearLocalException(ee);
        CVMclearRemoteException(ee);
        return (jint)JNI_ERR; /* Could not allocate thread: OutOfMemoryError */
    }
    return (jint)JNI_OK;
}

/* Purpose: Sets the thread-local storage.  This thread-local storage is
            usually used by the profiling agent for maintaining per-thread
            data. */
static void CVMjvmpi_SetThreadLocalStorage(JNIEnv *env, void *ptr)
{
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif
    CVMjniEnv2ExecEnv(env)->jvmpiProfilerData = ptr;
}

/* Purpose: Retrieves the previously set thread-local storage.  This thread-
            local storage is usually used by the profiling agent for
            maintaining per-thread data.*/
static void *CVMjvmpi_GetThreadLocalStorage(JNIEnv *env)
{
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
#endif
    return CVMjniEnv2ExecEnv(env)->jvmpiProfilerData;
}

/* Purpose: Disables garbage collection.  DisableGC() & EnableGC() may be
            nested. */
static void CVMjvmpi_DisableGC(void)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
    CVMjvmpiDisableGC(ee);
}

/* Purpose: Enables garbage collection.  DisableGC() & EnableGC() may be
            nested. */
static void CVMjvmpi_EnableGC(void)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
    CVMjvmpiEnableGC(ee);
}

/* Purpose: Called by the profiler to force a garbage collection. */
static void CVMjvmpi_RunGC(void)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMassert (ee != NULL);
    CVMassert(!CVMgcIsGCThread(ee));
    CVMgcRunGC(ee);
}

/* Purpose: Called by the profiler agent to obtain the thread object ID that
            corresponds to a JNIEnv pointer. */
/* NOTE: According to JVMPI specs, the profiler must disable GC before calling
         this function. */
/* NOTE: Called while GC safe. */
static jobjectID CVMjvmpi_GetThreadObject(JNIEnv *env)
{
    CVMObjectICell *threadObj;
    jobjectID thread_id;
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();

    CVMassert(env != NULL);
    CVMassert(ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(CVMjvmpiGCIsDisabled());
#endif
    threadObj = CVMcurrentThreadICell(CVMjniEnv2ExecEnv(env));
    thread_id = (jobjectID) CVMjvmpiGetICellDirect(ee, threadObj);
    return thread_id;
}

/* Purpose: Called by the profiler agent to obtain the object ID of the class
            that defines a method. */
/* NOTE: According to JVMPI specs, the profiler must disable GC before calling
         this function. */
static jobjectID CVMjvmpi_GetMethodClass(jmethodID mid)
{
    CVMMethodBlock *mb = (CVMMethodBlock *) mid;
    CVMObjectICell *clazzICell;
    jobjectID clazzObj;
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();

    CVMassert (mid != NULL);
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(CVMjvmpiGCIsDisabled());
#endif
    clazzICell = CVMcbJavaInstance(CVMmbClassBlock(mb));
    clazzObj = (jobjectID) CVMjvmpiGetICellDirect(ee, clazzICell);
    return clazzObj;
}

/* Purpose: Converts a jobjectID to a jobject. */
/* NOTE: This implementation is based on JDK1.3 Classic VM's implementation. */
/* NOTE: Called while GC safe. */
static jobject CVMjvmpi_jobjectID2jobject(jobjectID obj)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMObjectICell *result_cell;

    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMjvmpiGCIsDisabled());

    result_cell = CVMjniCreateLocalRef(ee);
    CVMjvmpiSetICellDirect(ee, result_cell, (CVMObject *) obj);
    return (jobject) result_cell;
}

/* Purpose: Converts a jobject to a jobjectID. */
/* NOTE: Though it is not explicitly stated in the JVMPI specs, the profiler
         must disable GC before calling this function.  Otherwise, there
         will be no guarantee that the returned jobjectID will be valid. */
/* NOTE: Called while GC safe. */
static jobjectID CVMjvmpi_jobject2jobjectID(jobject ref)
{
    jobjectID object_id;
#ifdef CVM_DEBUG_ASSERTS
    CVMExecEnv *ee = CVMgetEE();

    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(CVMjvmpiGCIsDisabled());
#endif
    object_id = (jobjectID) CVMjvmpiGetICellDirect(ee, ref);
    return object_id;
}

/*=============================================================================
    Methods for posting JVMPI events:
*/

/* Purpose: Posts a JVMPI_EVENT_ARENA_NEW event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostArenaNewEvent(CVMUint32 arenaID, const char *name)
{
    JVMPI_Event event;
    CVMExecEnv *ee = CVMgetEE();

    CVMassert(CVMD_isgcSafe(ee));
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.event_type = JVMPI_EVENT_ARENA_NEW;
    event.u.new_arena.arena_id = (jint) arenaID;
    event.u.new_arena.arena_name = name;
    CVMjvmpiNotifyEvent(&event);
}

/* Purpose: Posts a JVMPI_EVENT_ARENA_DELETE event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostArenaDeleteEvent(CVMUint32 arenaID)
{
    JVMPI_Event event;
    CVMExecEnv *ee = CVMgetEE();

    CVMassert(CVMD_isgcSafe(ee));
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.event_type = JVMPI_EVENT_ARENA_DELETE;
    event.u.delete_arena.arena_id = (jint) arenaID;
    CVMjvmpiNotifyEvent(&event);
}

/* Purpose: Posts a JVMPI_EVENT_CLASS_LOAD event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostClassLoadEvent(CVMExecEnv *ee, const CVMClassBlock *cb)
{
    /* NOTE: CVMjvmpiPostClassLoadEvent2() will assert that the current thread
             is in a GC safe state. */
    CVMjvmpiPostClassLoadEvent2(ee, cb, CVM_FALSE);
}

/* Purpose: Converts '/' to '.' in the specified class name string. */
static void CVMjvmpiClassnameSlash2Dot(char *clazzname)
{
    char *c = clazzname;
    for (; *c != '\0'; c++) {
        if (*c == '/') {
            *c = '.';
        }
    }
}

/* Purpose: Posts a JVMPI_EVENT_CLASS_LOAD event. */
/* NOTE: Called while GC safe. */
static jint
CVMjvmpiPostClassLoadEvent2(CVMExecEnv *ee, const CVMClassBlock *cb,
                            CVMBool isRequestedEvent)
{
    jint result = JVMPI_SUCCESS;
    JVMPI_Event event;
    char *class_name = NULL;
    int num_methods = CVMcbMethodCount(cb);
    int num_fields = CVMcbFieldCount(cb);
    JVMPI_Method *methods = NULL;
    JVMPI_Field *statics = NULL;
    JVMPI_Field *instances = NULL;
    int num_static = 0;
    int num_instance = 0;
    int i_stat = 0;
    int i_inst = 0;
    int i;

#undef failIfNull
#define failIfNull(x)   \
    if ((x) == NULL) {  \
        goto failed;    \
    }

    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(!CVMgcIsGCThread(ee));

    if (!isRequestedEvent && ee->hasPostedExitEvents){
	/* this thread is dead. we really don't want to
	 * post more events for it. Nothing bad happened,
	 * so we return success, but we don't actually post
	 * anything.
	 */
	return JVMPI_SUCCESS;
    }

    /* Compute the number of static and instance fields: */
    for (i = num_fields; --i >= 0; ) {
        const CVMFieldBlock *fb = CVMcbFieldSlot(cb, i);
        if (CVMfbIs(fb, STATIC)) {
            num_static++;
        } else {
            num_instance++;
        }
    }

    /* Allocate buffers to store the static and instance field info, and go
     * get them:
     */
    statics = (JVMPI_Field *) calloc(num_static, sizeof(JVMPI_Field));
    failIfNull(statics);
    instances = (JVMPI_Field *) calloc(num_instance, sizeof(JVMPI_Field));
    failIfNull(instances);

    for (i = num_fields; --i >= 0; ) {
        const CVMFieldBlock *fb = CVMcbFieldSlot(cb, i);
        CVMFieldTypeID tid = CVMfbNameAndTypeID(fb);
        char *name = CVMtypeidFieldNameToAllocatedCString(tid);
        char *sig = CVMtypeidFieldTypeToAllocatedCString(tid);
        failIfNull(name);
        failIfNull(sig);
        if (CVMfbIs(fb, STATIC)) {
            statics[i_stat].field_name = name;
            statics[i_stat].field_signature = sig;
            i_stat++;
        } else {
            instances[i_inst].field_name = name;
            instances[i_inst].field_signature = sig;
            i_inst++;
        }
    }

    /* Get the method info: */
    methods = (JVMPI_Method *) calloc(num_methods, sizeof(JVMPI_Method));
    failIfNull(methods);
    for (i = 0; i < num_methods; i++) {
        const CVMMethodBlock *mb = CVMcbMethodSlot(cb, i);
        CVMMethodTypeID tid = CVMmbNameAndTypeID(mb);
        CVMInt32 start = -1;
        CVMInt32 end = -1;
        char *method_name = CVMtypeidMethodNameToAllocatedCString(tid);
        char *method_sig = CVMtypeidMethodTypeToAllocatedCString(tid);

        failIfNull(method_name);
        failIfNull(method_sig);

        /* CVMmbJmd(mb) can be NULL if the method is a <clinit> which have
           already been executed: */
        if (!CVMmbIs(mb, NATIVE) && !CVMmbIs(mb, ABSTRACT) &&
            (CVMmbJmd(mb) != NULL)) {
            CVMLineNumberEntry *table = CVMmbLineNumberTable(mb);
            CVMUint32 length = CVMmbLineNumberTableLength(mb);

            /* Determine the starting and ending line number: */
            if ((table != NULL) && (length > 0)) {
                CVMInt32 current = -1;
                CVMInt32 i = 0;

                start = table[0].lineNumber;
                end = table[length - 1].lineNumber;
                if (end < start) {
                    end = start;
                }
                for (i = 1; i < length; i++) {
                    current = table[i].lineNumber;
                    if (current < start) {
                        start = current;
                    }
                    if (current > end) {
                        end = current;
                    }
                }
            }
        }

        /* Fill in the info for this method: */
        methods[i].method_name = method_name;
        methods[i].method_signature = method_sig;
        methods[i].method_id = (jmethodID) mb;
        methods[i].start_lineno = start;
        methods[i].end_lineno = end;
    }

    /* Get the class name: */
    class_name = CVMtypeidClassNameToAllocatedCString(CVMcbClassName(cb));
    failIfNull(class_name);

    /* Make some necessary adjustments: */
    CVMjvmpiClassnameSlash2Dot(class_name);
    if (!isRequestedEvent) {
        CVMjvmpiDisableGC(ee);
    }

    /* Fill in the event info: */
    event.event_type = JVMPI_EVENT_CLASS_LOAD;
    if (isRequestedEvent) {
        event.event_type |= JVMPI_REQUESTED_EVENT;
    }
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.class_load.class_name = class_name;
    {
        char *source_name = CVMcbSourceFileName(cb);
        event.u.class_load.source_name = (source_name != NULL) ?
                                         source_name : "";
    }
    event.u.class_load.num_interfaces = CVMcbImplementsCount(cb);
    event.u.class_load.num_methods = num_methods;
    event.u.class_load.methods = methods;
    event.u.class_load.num_static_fields = num_static;
    event.u.class_load.statics = statics;
    event.u.class_load.num_instance_fields = num_instance;
    event.u.class_load.instances = instances;
    event.u.class_load.class_id =
        (jobjectID) CVMjvmpiGetICellDirect(ee, CVMcbJavaInstance(cb));

    /* Post the event: */
    CVMjvmpiNotifyEvent(&event);

    if (!isRequestedEvent) {
        CVMjvmpiEnableGC(ee);
    }

doCleanUpAndExit:

    /* Release the buffers used: */
    freeIfNotNull(class_name);

    /* Release the methods' info: */
    if (methods != NULL) {
        for (i = 0; i < num_methods; i++) {
            freeIfNotNull(methods[i].method_name);
            freeIfNotNull(methods[i].method_signature);
        }
        free(methods);
    }
    /* Release the static fields' info: */
    if (statics != NULL) {
        for (i = 0; i < num_static; i++) {
            freeIfNotNull(statics[i].field_name);
            freeIfNotNull(statics[i].field_signature);
        }
        free(statics);
    }
    /* Release the instance fields' info: */
    if (instances != NULL) {
        for (i = 0; i < num_instance; i++) {
            freeIfNotNull(instances[i].field_name);
            freeIfNotNull(instances[i].field_signature);
        }
        free(instances);
    }
    return result;

failed:
    result = JVMPI_FAIL;
    goto doCleanUpAndExit;

#undef failIfNull

}


/* Purpose: Posts a JVMPI_CLASS_LOAD_HOOK event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostClassLoadHookEvent(CVMUint8 **buffer, CVMInt32 *bufferLength,
                                    void *(*malloc_f)(unsigned int))
{
    CVMExecEnv *ee = CVMgetEE();
    JVMPI_Event event;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(!ee->hasPostedExitEvents);

    /* Fill event info and notify the profiler: */
    event.event_type = JVMPI_EVENT_CLASS_LOAD_HOOK;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.class_load_hook.class_data = *buffer;
    event.u.class_load_hook.class_data_len = *bufferLength;
    event.u.class_load_hook.new_class_data = NULL;
    event.u.class_load_hook.new_class_data_len = 0;
    event.u.class_load_hook.malloc_f = malloc_f;
    CVMjvmpiNotifyEvent(&event);

    *buffer = event.u.class_load_hook.new_class_data;
    *bufferLength = event.u.class_load_hook.new_class_data_len;
}

/* Purpose: Posts a JVMPI_EVENT_CLASS_UNLOAD event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostClassUnloadEvent(CVMClassBlock *cb)
{
    CVMExecEnv *ee = CVMgetEE();
    JVMPI_Event event;

    CVMassert(CVMD_isgcSafe(ee));

    CVMjvmpiDisableGC(ee);
    /* fill event info and notify the profiler */
    event.event_type = JVMPI_EVENT_CLASS_UNLOAD;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.class_unload.class_id =
        (jobjectID) CVMjvmpiGetICellDirect(ee, CVMcbJavaInstance(cb));

    CVMjvmpiNotifyEvent(&event);
    CVMjvmpiEnableGC(ee);
}


/* Purpose: Posts a JVMPI_EVENT_COMPILED_METHOD_LOAD event. */
/* IAI-07: Notify JVMPI of compilations and decompilations. */
void CVMjvmpiPostCompiledMethodLoadEvent(CVMExecEnv *ee, CVMMethodBlock *mb)
{
#ifndef CVM_JIT
    CVMassert(CVM_FALSE);
#else
    JVMPI_Event event;
    CVMCompiledMethodDescriptor *cmd = CVMmbCmd(mb);
    jint table_size = 0;
    JVMPI_Lineno *lineno_table = NULL;

#ifdef CVM_DEBUG_CLASSINFO
    int i;
    CVMCompiledPcMap * maps = CVMcmdCompiledPcMaps(cmd);
    CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
    CVMLineNumberEntry * lineNumberEntry = CVMjmdLineNumberTable(jmd);
    int map_entries;
    jint line_number_tablen;
    int j;

    /* cmd: javapc<->compiled pc */
    map_entries = CVMcmdNumPcMaps(cmd);
    if (map_entries <= 0) {
        goto fillfields;
    }

    /* jmd: startpc<->lineNumber */
    line_number_tablen = CVMjmdLineNumberTableLength(jmd);
    if (line_number_tablen <= 0) {
        goto fillfields;
    }

    /* JVMPI_Lineno: compiled pc<->lineNumber */
    table_size = map_entries;

    lineno_table = calloc(table_size, sizeof(JVMPI_Lineno));
    if (lineno_table == NULL) {
        goto failed;
    }

    for (i = 0; i < table_size; i++) {
        lineno_table[i].offset = maps[i].compiledPc;
        for (j = 0; j < line_number_tablen-1; j++) {
            if (maps[i].javaPc >= lineNumberEntry[j].startpc && 
                maps[i].javaPc < lineNumberEntry[j+1].startpc) {
                break;
            }
        }
        lineno_table[i].lineno = lineNumberEntry[j].lineNumber;
    }

#endif /* CVM_DEBUG_CLASSINFO */

fillfields:
    CVMjvmpiDisableGC(ee);
    /* fill event info and notify the profiler */
    event.event_type = JVMPI_EVENT_LOAD_COMPILED_METHOD;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.compiled_method_load.method_id = (jmethodID)mb;
    event.u.compiled_method_load.code_addr = CVMcmdStartPC(cmd);
    event.u.compiled_method_load.code_size = CVMcmdCodeSize(cmd);
    event.u.compiled_method_load.lineno_table_size = table_size;
    event.u.compiled_method_load.lineno_table = lineno_table;

    CVMjvmpiNotifyEvent(&event);

    CVMjvmpiEnableGC(ee);

doCleanUpAndExit:
    freeIfNotNull(lineno_table);
    return;

failed:
    goto doCleanUpAndExit;
#endif /* CVM_JIT */
}

/* Purpose: Posts a JVMPI_EVENT_COMPILED_METHOD_UNLOAD event. */
/* IAI-07: Notify JVMPI of compilations and decompilations. */
void CVMjvmpiPostCompiledMethodUnloadEvent(CVMExecEnv *ee, CVMMethodBlock* mb)
{
#ifndef CVM_JIT
    CVMassert(CVM_FALSE);
#else
    JVMPI_Event event;

    CVMassert(CVMD_isgcSafe(ee));

    CVMjvmpiDisableGC(ee);
    /* fill event info and notify the profiler */
    event.event_type = JVMPI_EVENT_UNLOAD_COMPILED_METHOD;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.compiled_method_unload.method_id = (jmethodID)mb;
    CVMjvmpiNotifyEvent(&event);
    CVMjvmpiEnableGC(ee);
#endif /* CVM_JIT */
}

/* Purpose: Posts the JVMPI_EVENT_DATA_DUMP_REQUEST event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostDataDumpRequest(void)
{
    CVMExecEnv *ee = CVMgetEE();
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMassert(CVMD_isgcSafe(ee));
    CVMjvmpiResetDataDumpRequested();
    if (CVMjvmpiEventDataDumpRequestIsEnabled()) {
        JVMPI_Event event;
        event.env_id = env;
        event.event_type = JVMPI_EVENT_DATA_DUMP_REQUEST;
        CVMjvmpiNotifyEvent(&event);
    }
}

/* Purpose: Posts the JVMPI_EVENT_DATA_RESET_REQUEST event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostDataResetRequest(void)
{
    CVMExecEnv *ee = CVMgetEE();
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMassert(CVMD_isgcSafe(ee));
    if (CVMjvmpiEventDataResetRequestIsEnabled()) {
        JVMPI_Event event;
        event.env_id = env;
        event.event_type = JVMPI_EVENT_DATA_RESET_REQUEST;
        CVMjvmpiNotifyEvent(&event);
    }
}

/* Purpose: Posts the JVMPI_EVENT_GC_START event just before GC starts. */
/* NOTE: Called while GC safe before GC thread requests all threads to be
         safe. */
void CVMjvmpiPostGCStartEvent(void)
{
    CVMExecEnv *ee = CVMgetEE();
    JVMPI_Event event;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    /* Fill event info and notify the profiler: */
    event.event_type = JVMPI_EVENT_GC_START;
    event.env_id = CVMexecEnv2JniEnv(ee);

    CVMjvmpiNotifyEvent(&event);
}

/* Purpose: Posts the JVMPI_EVENT_GC_FINISH event just after GC finishes. */
/* NOTE: Called while GC safe before GC thread allows all threads to become
         unsafe again. */
void CVMjvmpiPostGCFinishEvent(CVMUint32 used_objs, CVMUint32 used_obj_space,
                               CVMUint32 total_obj_space)
{
    CVMExecEnv *ee = CVMgetEE();
    JVMPI_Event event;

    CVMassert(CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    /* fill event info and notify the profiler */
    event.event_type = JVMPI_EVENT_GC_FINISH;
    event.u.gc_info.used_objects = CVMint2Long(used_objs);
    event.u.gc_info.used_object_space = CVMint2Long(used_obj_space);
    event.u.gc_info.total_object_space = CVMint2Long(total_obj_space);
    event.env_id = CVMexecEnv2JniEnv(ee);

    CVMjvmpiNotifyEvent(&event);
}

/* Purpose: Posts a JVMPI_EVENT_HEAP_DUMP event. */
/* called when the profiler wants a heap dump */
/* NOTE: Called while GC safe. */
static jint CVMjvmpiPostHeapDumpEvent(CVMExecEnv *ee, int dumpLevel)
{
    CVMDumper dumper_;
    CVMDumper *dumper;
    CVMUint32 bufferSize;
    CVMBool done = CVM_FALSE;
    jint result = JVMPI_SUCCESS;
#ifdef CVM_DEBUG_ASSERTS
    CVMInt32 pass = 0;
#endif

    CVMassert(CVMD_isgcSafe(ee));

    dumper = &dumper_;
    CVMdumperInit(dumper, dumpLevel);

    /* Get an estimate of heap dump size: */
    {
        CVMUint32 freeMem = CVMgcimplFreeMemory(ee);
        CVMUint32 totalMem = CVMgcimplTotalMemory(ee);
        bufferSize = (CVMUint32)((totalMem - freeMem) * 1.5);
    }

    /* Lock out the GC: */
    /* NOTE: We won't get any interference from GC because this thread
        holds the heapLock.  This is effectively equivalent to disabling
        GC. */
#ifdef CVM_JIT
    CVMsysMutexLock(ee, &CVMglobals.jitLock);
#endif

    CVMsysMutexLock(ee, &CVMglobals.heapLock);

    while (!done) {

        CVMUint8 *buffer;

#ifdef CVM_DEBUG_ASSERTS
        pass++;
        /* We should never get to pass 3.  Pass 1 is the first attempt.  If
           pass 1 fails, it will compute the needed size.  Hence, pass 2
           should always succeed. */
        CVMassert(pass < 3);
#endif

        /* allocate enough memory to collect the heap dump as we execute
         * in single thread mode during heap dump collection and cannot
         * afford to call malloc 
         */
        buffer = malloc(bufferSize);
        if (buffer == NULL) {
            result = JVMPI_FAIL;    /* Not enough memory to do the dump. */
            goto doCleanUpAndExit;
        }

        CVMdumperInitBuffer(dumper, buffer, bufferSize);

        /* Fill in the dump info: */
        CVMdumperDumpHeap(dumper, ee);

        if (!CVMdumperIsOverflowed(dumper)) {
            /* notify the profiler */
            JVMPI_Event event;
            event.event_type = JVMPI_EVENT_HEAP_DUMP | JVMPI_REQUESTED_EVENT;
            event.u.heap_dump.dump_level = dumpLevel;
            event.u.heap_dump.begin = (char *) dumper->dumpStart;
            event.u.heap_dump.end = (char *) dumper->ptr;
            event.u.heap_dump.num_traces = dumper->tracesCount;
            event.u.heap_dump.traces = dumper->traces;

            CVMjvmpiDisableGC(ee);
            CVMjvmpiNotifyEvent(&event);
            CVMjvmpiEnableGC(ee);
            done = CVM_TRUE;
        }

        /* increase buffer size in case we over shot the allocated size */
        freeIfNotNull(buffer);
        bufferSize = CVMdumperGetRequiredBufferSize(dumper);
    }

doCleanUpAndExit:
    CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
#ifdef CVM_JIT
    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif

    return result;
}

#ifdef CVM_JVMPI_TRACE_INSTRUCTION

typedef enum CVMJvmpiBytecodeType
{
    CVM_JVMPI_BYTECODE_BASIC = 0,
    CVM_JVMPI_BYTECODE_IF,
    CVM_JVMPI_BYTECODE_LOOKUP_SWITCH,
    CVM_JVMPI_BYTECODE_TABLE_SWITCH
} CVMJvmpiBytecodeType;

/* Purpose: Performs the actual work of posting the
            JVMPI_EVENT_INSTRUCTION_START event. */
static void
CVMjvmpiPostInstructionStartEventInternal(CVMExecEnv *ee, CVMUint8 *pc,
                                          CVMJvmpiBytecodeType type,
                                          CVMBool isTrue,
                                          CVMInt32 chosenPairIndex,
                                          CVMInt32 pairsTotal,
                                          CVMInt32 key, CVMInt32 low,
                                          CVMInt32 hi)
{
    JVMPI_Event event;
    CVMMethodBlock *mb;
    CVMFrame *frame;
    CVMassert(ee != NULL);
    CVMassert(CVMD_isgcUnsafe(ee));
    frame = CVMeeGetCurrentFrame(ee);
    CVMassert(frame != NULL);
    if (CVMframeIsTransition(frame) || ee->hasPostedExitEvents) {
        return; /* Transition frames are supposed to be invisible. */
	/* and so are threads that have exited */
    }
    mb = frame->mb;
    if (!mb) {
        return;
    }
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.event_type = JVMPI_EVENT_INSTRUCTION_START;
    {
        CVMJavaMethodDescriptor *jmd = CVMmbJmd(mb);
        CVMInt32 offset = pc - CVMjmdCode(jmd);
        if (CVMjmdFlags(jmd) & CVM_JMD_DID_REWRITE) {
            offset = CVMstackmapReverseMapPC(jmd, offset);
            if (offset < 0) {
                return;
            }
        }
        event.u.instruction.offset = (jint) offset;
    }
    event.u.instruction.method_id = (jmethodID)mb;
    switch (type) {
        case CVM_JVMPI_BYTECODE_IF: {
            event.u.instruction.u.if_info.is_true = (jboolean) isTrue;
            break;
        }
        case CVM_JVMPI_BYTECODE_LOOKUP_SWITCH: {
            event.u.instruction.u.lookupswitch_info.chosen_pair_index =
                (jint) chosenPairIndex;
            event.u.instruction.u.lookupswitch_info.pairs_total =
                (jint)pairsTotal;
            break;
        }
        case CVM_JVMPI_BYTECODE_TABLE_SWITCH: {
            event.u.instruction.u.tableswitch_info.key = (jint) key;
            event.u.instruction.u.tableswitch_info.low = (jint) low;
            event.u.instruction.u.tableswitch_info.hi  = (jint) hi;
            break;
        }
        default:
            CVMassert(type == CVM_JVMPI_BYTECODE_BASIC);
    }
    CVMD_gcSafeExec(ee, {
        CVMjvmpiNotifyEvent(&event);
    });
}

/* Purpose: Posts a JVMPI_EVENT_INSTRUCTION_START event. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostInstructionStartEvent(CVMExecEnv *ee, CVMUint8 *pc)
{
    /* NOTE: CVMjvmpiPostInstructionStartEventInternal() will assert that the
             current thread is GC unsafe. */
    CVMjvmpiPostInstructionStartEventInternal(ee, pc,
        CVM_JVMPI_BYTECODE_BASIC, CVM_FALSE, 0, 0, 0, 0, 0);
}

/* Purpose: Posts a JVMPI_EVENT_INSTRUCTION_START event for "if" conditional
            opcodes. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostInstructionStartEvent4If(CVMExecEnv *ee, CVMUint8 *pc,
                                          CVMBool isTrue)
{
    /* NOTE: CVMjvmpiPostInstructionStartEventInternal() will assert that the
             current thread is GC unsafe. */
    CVMjvmpiPostInstructionStartEventInternal(ee, pc,
        CVM_JVMPI_BYTECODE_IF, isTrue, 0, 0, 0, 0, 0);
}

/* Purpose: Posts a JVMPI_EVENT_INSTRUCTION_START event for "lookupswitch"
            opcodes. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostInstructionStartEvent4Lookupswitch(CVMExecEnv *ee,
            CVMUint8 *pc, CVMInt32 chosenPairIndex, CVMInt32 pairsTotal)
{
    /* NOTE: CVMjvmpiPostInstructionStartEventInternal() will assert that the
             current thread is GC unsafe. */
    CVMjvmpiPostInstructionStartEventInternal(ee, pc,
        CVM_JVMPI_BYTECODE_LOOKUP_SWITCH, CVM_FALSE,
        chosenPairIndex, pairsTotal, 0, 0, 0);
}

/* Purpose: Posts a JVMPI_EVENT_INSTRUCTION_START event for "tableswitch"
            opcodes. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostInstructionStartEvent4Tableswitch(CVMExecEnv *ee,
                    CVMUint8 *pc, CVMInt32 key, CVMInt32 low, CVMInt32 hi)
{
    /* NOTE: CVMjvmpiPostInstructionStartEventInternal() will assert that the
             current thread is GC unsafe. */
    CVMjvmpiPostInstructionStartEventInternal(ee, pc,
        CVM_JVMPI_BYTECODE_TABLE_SWITCH, CVM_FALSE, 0, 0, key, low, hi);
}

#endif /* CVM_JVMPI_TRACE_INSTRUCTION */

/* Purpose: Posts a JVMPI_EVENT_JNI_GLOBALREF_ALLOC event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostJNIGlobalrefAllocEvent(CVMExecEnv *ee, jobject ref)
{
    JVMPI_Event event;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    CVMjvmpiDisableGC(ee);

    /* Fill event info and notify the profiler: */
    event.event_type = JVMPI_EVENT_JNI_GLOBALREF_ALLOC;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.jni_globalref_alloc.obj_id =
        (jobjectID) CVMjvmpiGetICellDirect(ee, ref);
    event.u.jni_globalref_alloc.ref_id = ref;

    CVMjvmpiNotifyEvent(&event);
    CVMjvmpiEnableGC(ee);
}

/* Purpose: Posts a JVMPI_EVENT_JNI_GLOBALREF_FREE event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostJNIGlobalrefFreeEvent(CVMExecEnv *ee, jobject ref)
{
    JVMPI_Event event;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    /* Fill event info and notify the profiler: */
    event.event_type = JVMPI_EVENT_JNI_GLOBALREF_FREE;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.jni_globalref_free.ref_id = ref;

    CVMjvmpiNotifyEvent(&event);
}

/* Purpose: Posts a JVMPI_EVENT_JNI_WEAK_GLOBALREF_ALLOC event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostJNIWeakGlobalrefAllocEvent(CVMExecEnv *ee, jobject ref)
{
    JVMPI_Event event;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    CVMjvmpiDisableGC(ee);

    /* Fill event info and notify the profiler: */
    event.event_type = JVMPI_EVENT_JNI_WEAK_GLOBALREF_ALLOC;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.jni_globalref_alloc.obj_id =
        (jobjectID) CVMjvmpiGetICellDirect(ee, ref);
    event.u.jni_globalref_alloc.ref_id = ref;

    CVMjvmpiNotifyEvent(&event);
    CVMjvmpiEnableGC(ee);
}

/* Purpose: Posts a JVMPI_EVENT_JNI_WEAK_GLOBALREF_FREE event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostJNIWeakGlobalrefFreeEvent(CVMExecEnv *ee, jobject ref)
{
    JVMPI_Event event;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    /* Fill event info and notify the profiler: */
    event.event_type = JVMPI_EVENT_JNI_WEAK_GLOBALREF_FREE;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.jni_globalref_free.ref_id = ref;

    CVMjvmpiNotifyEvent(&event);
}

/* Purpose: Posts a JVMPI_EVENT_JVM_INIT_DONE event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostJVMInitDoneEvent(void)
{
    /* Notify the profiler that VM init is done and it can safely
       do things like thread creation: */
    JVMPI_Event event;
    CVMExecEnv *ee = CVMgetEE();

    CVMassert(CVMD_isgcSafe(ee));
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.event_type = JVMPI_EVENT_JVM_INIT_DONE;
    CVMjvmpiNotifyEvent(&event);
}

/* Purpose: Posts a JVMPI Method Entry event. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostMethodEntryEvent(CVMExecEnv *ee, CVMObjectICell *objICell)
{
    JVMPI_Event event;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMMethodBlock *mb = CVMeeGetCurrentFrame(ee)->mb;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcUnsafe(ee));

    /* Post the event for JVMPI_EVENT_METHOD_ENTRY if necessary: */
    if (ee->hasPostedExitEvents){
	return;
    }
    if (CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_METHOD_ENTRY)) {
        event.event_type = JVMPI_EVENT_METHOD_ENTRY;
        event.env_id = env;
        event.u.method.method_id = (jmethodID)mb;

        CVMD_gcSafeExec(ee, {
            CVMjvmpiNotifyEvent(&event);
        });
    }
    /* Post the event for JVMPI_EVENT_METHOD_ENTRY2 if necessary: */
    if (CVMjvmpiEventInfoIsEnabled(JVMPI_EVENT_METHOD_ENTRY2)) {
        CVMD_gcSafeExec(ee, {
            CVMObject *obj_id;

            CVMjvmpiDisableGC(ee);

            event.event_type = JVMPI_EVENT_METHOD_ENTRY2;
            event.env_id = env;
            event.u.method_entry2.method_id = (jmethodID)mb;
            if (CVMmbIs(mb, STATIC)) {
                obj_id = 0;
            } else {
                obj_id = CVMjvmpiGetICellDirect(ee, objICell);
            }
            event.u.method_entry2.obj_id = (jobjectID) obj_id;

            CVMjvmpiNotifyEvent(&event);
            CVMjvmpiEnableGC(ee);
        });
    }
}

/* Purpose: Posts a JVMPI Method Entry event. */
/* NOTE: Called while GC unsafe. */
void CVMjvmpiPostMethodExitEvent(CVMExecEnv *ee)
{
    JVMPI_Event event;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcUnsafe(ee));

    if (ee->hasPostedExitEvents){
	return;
    }
    /* Fill event info and notify the profiler: */
    event.event_type = JVMPI_EVENT_METHOD_EXIT;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.method.method_id = (jmethodID)CVMeeGetCurrentFrame(ee)->mb;
    CVMD_gcSafeExec(ee, {
        CVMjvmpiNotifyEvent(&event);
    });
}

/* Purpose: Posts a JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER or
            JVMPI_EVENT_MONITOR_CONTENDED_ENTER event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostMonitorContendedEnterEvent(CVMExecEnv *ee,
                                            CVMProfiledMonitor *pm)
{
    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    if (pm->type == CVM_LOCKTYPE_NAMED_SYSMONITOR) {
        JVMPI_Event event;
        CVMNamedSysMonitor *mon = CVMcastProfiledMonitor2NamedSysMonitor(pm);
        const char *name = CVMnamedSysMonitorGetName(mon);

        /* According to the JVMPI spec for RawMonitorCreate(), if a monitor's
           name begins with '_', its monitor contention events are not sent
           to the profiler agent:
        */
        if (name[0] != '_') {
            event.env_id = CVMexecEnv2JniEnv(ee);
            event.event_type = JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER;
            event.u.raw_monitor.name = name;
            event.u.raw_monitor.id = (JVMPI_RawMonitor) mon;
            CVMjvmpiNotifyEvent(&event);
        }

    } else if (pm->type == CVM_LOCKTYPE_OBJ_MONITOR) {
        JVMPI_Event event;
        CVMObjMonitor *mon = CVMcastProfiledMonitor2ObjMonitor(pm);

        CVMjvmpiDisableGC(ee);

        event.env_id = CVMexecEnv2JniEnv(ee);
        event.event_type = JVMPI_EVENT_MONITOR_CONTENDED_ENTER;
        event.u.monitor.object = (jobjectID) mon->obj;
        CVMjvmpiNotifyEvent(&event);

        CVMjvmpiEnableGC(ee);
    }
}

/* Purpose: Posts a JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED or
            JVMPI_EVENT_MONITOR_CONTENDED_ENTERED event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostMonitorContendedEnteredEvent(CVMExecEnv *ee,
                                              CVMProfiledMonitor *pm)
{
    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    if (pm->type == CVM_LOCKTYPE_NAMED_SYSMONITOR) {
        JVMPI_Event event;
        CVMNamedSysMonitor *mon = CVMcastProfiledMonitor2NamedSysMonitor(pm);
        const char *name = CVMnamedSysMonitorGetName(mon);

        /* According to the JVMPI spec for RawMonitorCreate(), if a monitor's
           name begins with '_', its monitor contention events are not sent
           to the profiler agent:
        */
        if (name[0] != '_') {
            event.env_id = CVMexecEnv2JniEnv(ee);
            event.event_type = JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED;
            event.u.raw_monitor.name = name;
            event.u.raw_monitor.id = (JVMPI_RawMonitor) mon;
            CVMjvmpiNotifyEvent(&event);
        }

    } else if (pm->type == CVM_LOCKTYPE_OBJ_MONITOR) {
        JVMPI_Event event;
        CVMObjMonitor *mon = CVMcastProfiledMonitor2ObjMonitor(pm);

        CVMjvmpiDisableGC(ee);

        event.env_id = CVMexecEnv2JniEnv(ee);
        event.event_type = JVMPI_EVENT_MONITOR_CONTENDED_ENTERED;
        event.u.monitor.object = (jobjectID) mon->obj;
        CVMjvmpiNotifyEvent(&event);

        CVMjvmpiEnableGC(ee);
    }
}

/* Purpose: Posts a JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT or
            JVMPI_EVENT_MONITOR_CONTENDED_EXIT event. */
/* NOTE: Can be called while GC safe or unsafe. */
void CVMjvmpiPostMonitorContendedExitEvent(CVMExecEnv *ee,
                                           CVMProfiledMonitor *pm)
{
    CVMassert(!CVMgcIsGCThread(ee));
    if (CVMD_isgcSafe(ee)) {
        CVMjvmpiPostMonitorContendedExitEvent2(ee, pm);
    } else {
        CVMD_gcSafeExec(ee, {
            CVMjvmpiPostMonitorContendedExitEvent2(ee, pm);
        });
    }
}

/* Purpose: Does the work of posting a JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT
            or JVMPI_EVENT_MONITOR_CONTENDED_EXIT event. */
/* NOTE: This is broken out from CVMjvmpiPostMonitorContendedExitEvent() so
         that we can guarantee that it is called while GC safe. */
static void CVMjvmpiPostMonitorContendedExitEvent2(CVMExecEnv *ee,
                                                   CVMProfiledMonitor *pm)
{
    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    if (pm->type == CVM_LOCKTYPE_NAMED_SYSMONITOR) {
        JVMPI_Event event;
        CVMNamedSysMonitor *mon = CVMcastProfiledMonitor2NamedSysMonitor(pm);
        const char *name = CVMnamedSysMonitorGetName(mon);

        /* According to the JVMPI spec for RawMonitorCreate(), if a monitor's
           name begins with '_', its monitor contention events are not sent
           to the profiler agent:
        */
        if (name[0] != '_') {
            event.env_id = CVMexecEnv2JniEnv(ee);
            event.event_type = JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT;
            event.u.raw_monitor.name = name;
            event.u.raw_monitor.id = (JVMPI_RawMonitor) mon;
            CVMjvmpiNotifyEvent(&event);
        }

    } else if (pm->type == CVM_LOCKTYPE_OBJ_MONITOR) {

        JVMPI_Event event;
        CVMObjMonitor *mon = CVMcastProfiledMonitor2ObjMonitor(pm);

        CVMjvmpiDisableGC(ee);

        event.env_id = CVMexecEnv2JniEnv(ee);
        event.event_type = JVMPI_EVENT_MONITOR_CONTENDED_EXIT;
        event.u.monitor.object = (jobjectID) mon->obj;
        CVMjvmpiNotifyEvent(&event);

        CVMjvmpiEnableGC(ee);
    }
}

/* Purpose: Posts a JVMPI_EVENT_MONITOR_DUMP event to dump a snapshot of all
            threads and monitors in the VM.  Called when the profiler requests
            a monitor dump though RequestEvent(). */
/* NOTE: Should be called while GC safe. */
static jint CVMjvmpiPostMonitorDumpEvent(CVMExecEnv *ee)
{
    CVMDumper dumperInstance;
    CVMDumper *dumper;
    unsigned int bufferSize;
    CVMBool done = CVM_FALSE;
    jint result = JVMPI_SUCCESS;
#ifdef CVM_DEBUG_ASSERTS
    CVMInt32 pass = 0;
#endif

    CVMassert(CVMD_isgcSafe(ee));

    dumper = &dumperInstance;
    CVMdumperInit(dumper, JVMPI_DUMP_LEVEL_2);

#ifdef CVM_JIT
    CVMsysMutexLock(ee, &CVMglobals.jitLock);
#endif
    CVMsysMutexLock(ee, &CVMglobals.heapLock);

    /* And get the rest of the GC locks: */
    CVMlocksForGCAcquire(ee);

    /* Roll all threads to GC-safe points. */
    CVMD_gcBecomeSafeAll(ee);

    /* get an estimate of monitor dump size */
    bufferSize = 1024 * 256; /* 256 K to start with */
    while (!done) {

        CVMUint8 *buffer;

#ifdef CVM_DEBUG_ASSERTS
        pass++;
        /* We should never get to pass 3.  Pass 1 is the first attempt.  If
           pass 1 fails, it will compute the needed size.  Hence, pass 2
           should always succeed. */
        CVMassert(pass < 3);
#endif

        /* Allocate the dump buffer before we start: */
        buffer = malloc(bufferSize);
        if (buffer == NULL) {
            result = JVMPI_FAIL; /* Not able to allocate memory for dump. */
            goto doCleanUpAndExit;
        }

        /* Initialize the dump buffer: */
        CVMdumperInitBuffer(dumper, buffer, bufferSize);

        CVMjvmpiDisableGC(ee);

        /* We would have needed to grab the threadLock to make sure threads
           don't terminate.  However, this s already done by
           CVMlocksForGCAcquire() above.  Hence, we don't have to acquire it
           again: */
        CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.threadLock));

        /* Fill in the dump info: */
        CVMdumperDumpThreadsAndMonitors(dumper, ee);

        /* If it appears that we had more than adequate space for the dump,
           then ... */
        if (!CVMdumperIsOverflowed(dumper)) {
            JVMPI_Event event;
            event.event_type = JVMPI_EVENT_MONITOR_DUMP |
                               JVMPI_REQUESTED_EVENT;
            event.env_id = CVMexecEnv2JniEnv(ee);
            event.u.monitor_dump.begin = (char *) dumper->dumpStart;
            event.u.monitor_dump.end = (char *) dumper->ptr;
            event.u.monitor_dump.num_traces = dumper->tracesCount;
            event.u.monitor_dump.traces = dumper->traces;
            event.u.monitor_dump.threads_status = dumper->threadStatusList;
            CVMjvmpiNotifyEvent(&event);
            done = CVM_TRUE;
        }

        /* Would have released the threadLock here if we had needed to grab it
           above: */

        CVMjvmpiEnableGC(ee);

        /* increase buffer size in case we over shot the allocated size */

        /* Release the dump buffer: */
        freeIfNotNull(buffer);

        /* Increase the buffer size estimate for another attempt to dump: */
        bufferSize = CVMdumperGetRequiredBufferSize(dumper);
    }

doCleanUpAndExit:
    /* Allow threads to become GC-unsafe again. */
    CVMD_gcAllowUnsafeAll(ee);

    CVMlocksForGCRelease(ee);

    CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
#ifdef CVM_JIT
    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif

    return result;
}

/* Purpose: Posts a JVMPI_EVENT_MONITOR_WAIT event.  Sent when a thread is
            about to wait on an object. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostMonitorWaitEvent(CVMExecEnv *ee, CVMObjectICell *objICell,
                                  jlong millis)
{
    JVMPI_Event event;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    CVMjvmpiDisableGC(ee);

    event.env_id = CVMexecEnv2JniEnv(ee);
    event.event_type = JVMPI_EVENT_MONITOR_WAIT;
    event.u.monitor_wait.object = (objICell != NULL) ?
        (jobjectID) CVMjvmpiGetICellDirect(ee, objICell) : (jobjectID) NULL;
    event.u.monitor_wait.timeout = millis;
    CVMjvmpiNotifyEvent(&event);

    CVMjvmpiEnableGC(ee);
}

/* Purpose: Posts a JVMPI_EVENT_MONITOR_WAITED event.  Sent when a thread
            finishes waiting on an object. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostMonitorWaitedEvent(CVMExecEnv *ee, CVMObjectICell *objICell)
{
    JVMPI_Event event;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    CVMjvmpiDisableGC(ee);

    event.env_id = CVMexecEnv2JniEnv(ee);
    event.event_type = JVMPI_EVENT_MONITOR_WAITED;
    event.u.monitor_wait.object = (objICell != NULL) ?
        (jobjectID) CVMjvmpiGetICellDirect(ee, objICell) : (jobjectID) NULL;
    CVMjvmpiNotifyEvent(&event);

    CVMjvmpiEnableGC(ee);
}

/* Purpose: Called by GC routines after it allocates an object to post a
            JVMPI_EVENT_OBJECT_ALLOC event. */
/* NOTE: CVMjvmpiPostObjectAllocEvent() will fetch the arena ID for the object
         from the GC itself.  Hence, the prototype does not require the caller
         to pass in the arena ID for the object.  This is done to simplify the
         implementation for RequestEvent() because JVMPI_EVENT_OBJECT_ALLOC
         can be requested and no arena ID is supplied (which makes sense). */
/* NOTE: Object allocation can happen within a GC cycle.  So, this event can
         be posted while GC safe or unsafe. */
void CVMjvmpiPostObjectAllocEvent(CVMExecEnv *ee, CVMObject *obj)
{
    CVMassert(CVMD_isgcUnsafe(ee) || CVMgcIsGCThread(ee));

    CVMjvmpiDisableGC(ee);
    if (CVMD_isgcUnsafe(ee)) {
        CVMD_gcSafeExec(ee, {
            CVMjvmpiPostObjectAllocEvent2(ee, obj, CVM_FALSE);
        });
    } else {
        CVMjvmpiPostObjectAllocEvent2(ee, obj, CVM_FALSE);
    }
    CVMjvmpiEnableGC(ee);
}

/* Purpose: Posts a JVMPI_EVENT_OBJECT_ALLOC event. */
/* NOTE: Called while GC safe. */
static void CVMjvmpiPostObjectAllocEvent2(CVMExecEnv *ee,
                                          CVMObject *obj,
                                          CVMBool isRequestedEvent)
{
    JVMPI_Event event;
    const CVMClassBlock *cb;
    CVMUint8 type;
    CVMUint32 size;

    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(CVMjvmpiGCIsDisabled());

    if (obj == NULL)
        return;

#ifdef CVM_INSPECTOR
    if (!isRequestedEvent) {
        CVMassert(CVMgcIsValidObject(ee, obj));
    }
#endif
    cb = CVMobjectGetClass(obj);
    type = CVMjvmpiGetClassType(cb);
    size = CVMobjectSizeGivenClass(obj, cb);

    /* Fill the event info: */
    event.event_type = JVMPI_EVENT_OBJ_ALLOC;
    if (isRequestedEvent) {
        event.event_type |= JVMPI_REQUESTED_EVENT;
    }
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.obj_alloc.arena_id = (jint) CVMgcGetArenaID(obj);

    /* If the object is an array of refs, then class object of interest is the
       the element type: */
    if (type == JVMPI_CLASS) {
        cb = CVMarrayBaseCb(cb);
    }
    /* Get the class object: */
    if ((type == JVMPI_NORMAL_OBJECT) || (type == JVMPI_CLASS)) {
        CVMObjectICell *clazz = CVMcbJavaInstance(cb);
        CVMObject *clazzObj = CVMjvmpiGetICellDirect(ee, clazz);
        event.u.obj_alloc.class_id = (clazzObj != obj) ?
                                 (jobjectID) clazzObj : NULL;
    } else {
        event.u.obj_alloc.class_id = NULL;
    }

    event.u.obj_alloc.is_array = (jint) type;
    event.u.obj_alloc.size = (jint) size;
    event.u.obj_alloc.obj_id = (jobjectID) obj;

    /* Post the event: */
    CVMjvmpiNotifyEvent(&event);
}

/* Purpose: Posts a JVMPI_EVENT_OBJECT_DUMP to dump the info on the specified
            object. */
/* NOTE: Should be called while GC is disabled. */
/* NOTE: Should be called while in a GC safe state. */
static jint CVMjvmpiPostObjectDumpEvent(CVMObject *obj)
{
    JVMPI_Event event;
    CVMDumper dumper;
    CVMExecEnv *ee = CVMgetEE();
    CVMUint32 bufferSize;
    CVMUint8 *buffer;
    jint result;

    CVMassert(CVMjvmpiGCIsDisabled());
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));

    if (obj == 0) {
        return JVMPI_FAIL;
    }

    /* Get the object dump info: */
    CVMdumperInit(&dumper, JVMPI_DUMP_LEVEL_2);
    bufferSize = CVMdumperComputeObjectDumpSize(&dumper, obj);
    CVMassert(bufferSize != 0);

    buffer = malloc(bufferSize);
    if (buffer == NULL) {
        return JVMPI_FAIL;
    }

    CVMdumperInitBuffer(&dumper, buffer, bufferSize);

    /* If we successfully dumped the object, then post the event: */
    result = CVMdumperDumpObject(&dumper, obj);
    if (result == JVMPI_SUCCESS) {
        /* Fill in and post the event: */
        event.env_id = CVMexecEnv2JniEnv(ee);
        event.event_type = JVMPI_EVENT_OBJECT_DUMP | JVMPI_REQUESTED_EVENT;
        event.u.object_dump.data_len = dumper.ptr - buffer;
        event.u.object_dump.data = (char *) buffer;
        CVMjvmpiNotifyEvent(&event);
    }

    freeIfNotNull(buffer);
    return result;
}

/* Purpose: Posts an JVMPI_EVENT_OBJECT_FREE for each object which is about to
            be freed. */
/* NOTE: Called while in a GC cycle i.e. while GC safe also. */
void CVMjvmpiPostObjectFreeEvent(CVMObject *obj)
{
    CVMExecEnv *ee = CVMgetEE();
    JVMPI_Event event;

    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(CVMgcIsGCThread(ee));

    CVMjvmpiDisableGC(ee);

    /* fill event info and notify the profiler */
    event.event_type = JVMPI_EVENT_OBJ_FREE;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.obj_free.obj_id = (jobjectID) obj;

    CVMjvmpiNotifyEvent(&event);
    CVMjvmpiEnableGC(ee);
}

/* Purpose: Posts an JVMPI_EVENT_OBJECT_MORE for each object which has been
            moved. */
/* NOTE: Called while in a GC cycle i.e. while GC safe also. */
void CVMjvmpiPostObjectMoveEvent(CVMUint32 oldArenaID, void *oldObj,
                                 CVMUint32 newArenaID, void *newObj)
{
    CVMExecEnv *ee = CVMgetEE();
    JVMPI_Event event;

    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(CVMgcIsGCThread(ee));

    CVMjvmpiDisableGC(ee);

    /* fill event info and notify the profiler */
    event.event_type = JVMPI_EVENT_OBJ_MOVE;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.obj_move.obj_id       = (jobjectID) oldObj;
    event.u.obj_move.arena_id     = oldArenaID;
    event.u.obj_move.new_obj_id   = (jobjectID) newObj;
    event.u.obj_move.new_arena_id = newArenaID;

    CVMjvmpiNotifyEvent(&event);

    CVMjvmpiEnableGC(ee);
}

/* Purpose: Posts a JVMPI_EVENT_THREAD_START event. */
/* NOTE: See CVMjvmpiPostThreadStartEvent2() for details. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostThreadStartEvent(CVMExecEnv *ee, CVMThreadICell *threadICell)
{
    /* NOTE: CVMjvmpiPostThreadStartEvent2() will assert that the current
             thread is GC safe. */
    CVMjvmpiDisableGC(ee);
    CVMjvmpiPostThreadStartEvent2(ee, CVMjvmpiGetICellDirect(ee, threadICell),
                                  CVM_FALSE);
    CVMjvmpiEnableGC(ee);
}

/* Purpose: Posts a JVMPI_EVENT_THREAD_START event. */
/* Returns: JVMPI_SUCCESS is successful, else returns JVMPI_FAIL. */
/* NOTE: This is the method which does the real work. */
/* NOTE: Should be called in a GC safe state.  Cannot be called while in a GC
         cycle. */
static jint
CVMjvmpiPostThreadStartEvent2(CVMExecEnv *ee, CVMObject *threadObj,
                              CVMBool isRequestedEvent)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    JVMPI_Event event;
    jint result = JVMPI_SUCCESS;

    CVMassert(CVMjvmpiGCIsDisabled());
    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    {
        CVMExecEnv *targetEE;
        CVMJavaLong eetop;
        CVMObject *threadGroup;
        CVMObject *threadGroupParent = NULL;
        CVMObject *threadName;
        CVMObject *threadGroupName = NULL;
        CVMObject *threadGroupParentName = NULL;

        CVMObject *threadNameStrObj = NULL;
        jchar *threadNameElements = NULL;
        const char *threadNameStr = NULL;
        const char *threadGroupNameStr = NULL;
        const char *threadGroupParentNameStr = NULL;

        CVMD_gcUnsafeExec(ee, {
            /* Get the thread's ee: */
            CVMD_fieldReadLong(threadObj, CVMoffsetOfjava_lang_Thread_eetop,
                                eetop);
            targetEE = (CVMExecEnv *)CVMlong2VoidPtr(eetop);

            /* Get the thread group: */
            CVMD_fieldReadRef(threadObj, CVMoffsetOfjava_lang_Thread_group,
                              threadGroup);

            /* Get the thread group parent: */
            if (threadGroup != NULL) {
                CVMD_fieldReadRef(threadGroup,
                   CVMoffsetOfjava_lang_ThreadGroup_parent, threadGroupParent);
            }

            /* Get the thread's name: */
            CVMD_fieldReadRef(threadObj, CVMoffsetOfjava_lang_Thread_name,
                              threadName);

            /* Get the thread group's name: */
            if (threadGroup != NULL) {
                CVMD_fieldReadRef(threadGroup,
                    CVMoffsetOfjava_lang_ThreadGroup_name, threadGroupName);
            }

            /* Get the thread group parent's name: */
            if (threadGroupParent != NULL) {
                CVMD_fieldReadRef(threadGroupParent,
                    CVMoffsetOfjava_lang_ThreadGroup_name,
                    threadGroupParentName);
            }
        });

        /* Fill event info and notify the profiler: */
        event.event_type = JVMPI_EVENT_THREAD_START;
        if (isRequestedEvent) {
            event.event_type |= JVMPI_REQUESTED_EVENT;
        }
        event.env_id = env;
        event.u.thread_start.thread_id = (jobjectID) threadObj;
        event.u.thread_start.thread_env_id = CVMexecEnv2JniEnv(targetEE);

        /* NOTE: In the following, JNI APIs are used to fetch and create
           strings.  The JNI Apis take object references in the form of
           jobjects i.e. ICells.  The local roots are allocated to provide
           these jobjects.

           After the desired results are attained, the local roots are released
           before we call across to the profiler.  This is done so as to not
           leave a local root frame on the local root stack when we call across
           to the profiler.  This is necessary because the profiler can request
           thread start events which would result in the VM recursing into
           CVMjvmpiPostThreadStartEvent2().  Leaving the local root frame on
           the local root stack may result in a local root stack overflow if
           the execution path recurses through here too often.

           Instead, direct object references are held on the native stack as
           automatic variables.  This can be done without fear of GC moving the
           object without our knowledge because GC has already been disabled
           before this function is called (see assertion above).

           After returning from the profiler, we need to re-allocate some
           local roots in order to provide jobjects to be used with the JNI
           APIs for releasing resources that were allocated in this function.
        */
        CVMID_localrootBegin(ee) {
            CVMID_localrootDeclare(CVMObjectICell, threadCharArrayICell);
            CVMID_localrootDeclare(CVMObjectICell, threadNameICell);
            CVMID_localrootDeclare(CVMObjectICell, threadGroupNameICell);
            CVMID_localrootDeclare(CVMObjectICell, threadGroupParentNameICell);

            /* Fill in the thread's name if available: */
            if (threadName != NULL) {
                jint length;
                CVMD_gcUnsafeExec(ee, {
                    CVMID_icellSetDirect(ee, threadCharArrayICell, threadName);
                });
                threadNameElements = (*env)->GetCharArrayElements(env,
                                     (jcharArray) threadCharArrayICell, NULL);
                if (threadNameElements == NULL) {
                    goto failedInFirstLocalRootBlock;
                }
                length = (*env)->GetArrayLength(env,
                                    (jarray) threadCharArrayICell);

                threadNameICell =
                    (*env)->NewString(env, threadNameElements, length);
                if (threadNameICell == NULL) {
                    (*env)->ExceptionClear(env);
                    goto failedInFirstLocalRootBlock;
                }

                threadNameStrObj = CVMjvmpiGetICellDirect(ee, threadNameICell);
                threadNameStr =
                    (*env)->GetStringUTFChars(env, threadNameICell, NULL);
                if (threadNameStr == NULL) {
                    goto failedInFirstLocalRootBlock;
                }
            }
            /* Fill in the thread group's name if available: */
            if (threadGroupName != NULL) {
                CVMD_gcUnsafeExec(ee, {
                    CVMID_icellSetDirect(ee, threadGroupNameICell,
                                         threadGroupName);
                });
                threadGroupNameStr = (*env)->GetStringUTFChars(env,
                                                threadGroupNameICell, NULL);
                if (threadGroupNameStr == NULL) {
                    goto failedInFirstLocalRootBlock;
                }
            }
            /* Fill in the thread group parent's name if available: */
            if (threadGroupParentName != NULL) {
                CVMD_gcUnsafeExec(ee, {
                    CVMID_icellSetDirect(ee, threadGroupParentNameICell,
                                         threadGroupParentName);
                });
                threadGroupParentNameStr = (*env)->GetStringUTFChars(env,
                                                threadGroupParentNameICell,
                                                NULL);
                if (threadGroupParentNameStr == NULL) {
                    goto failedInFirstLocalRootBlock;
                }
            }
            goto endOfFirstLocalRootBlock;

        failedInFirstLocalRootBlock:
            result = JVMPI_FAIL;
            if (threadGroupNameStr != NULL) {
                (*env)->ReleaseStringUTFChars(env, threadGroupNameICell,
                                              threadGroupNameStr);
            }
            if (threadNameStr != NULL) {
                (*env)->ReleaseStringUTFChars(env, threadNameICell,
                                              threadNameStr);
            }
            if (threadNameElements != NULL) {
                (*env)->ReleaseCharArrayElements(env,
                                (jcharArray) threadCharArrayICell,
                                threadNameElements, JNI_ABORT);
            }

        endOfFirstLocalRootBlock:;

        } CVMID_localrootEnd();

        if (result == JVMPI_FAIL) {
            return JVMPI_FAIL; /* Abort. */
        }

        event.u.thread_start.thread_name = (char *)threadNameStr;
        event.u.thread_start.group_name = (char *)threadGroupNameStr;
        event.u.thread_start.parent_name = (char *)threadGroupParentNameStr;

        /* Post the event: */
        CVMjvmpiNotifyEvent(&event);

        CVMID_localrootBegin(ee) {
            CVMID_localrootDeclare(CVMObjectICell, threadCharArrayICell);
            CVMID_localrootDeclare(CVMObjectICell, threadNameICell);
            CVMID_localrootDeclare(CVMObjectICell, threadGroupNameICell);
            CVMID_localrootDeclare(CVMObjectICell, threadGroupParentNameICell);

            /* Re-establish the ICells: */
            CVMD_gcUnsafeExec(ee, {
                CVMID_icellSetDirect(ee, threadCharArrayICell, threadName);
                CVMID_icellSetDirect(ee, threadNameICell, threadNameStrObj);
                CVMID_icellSetDirect(ee, threadGroupNameICell, threadGroupName);
                CVMID_icellSetDirect(ee, threadGroupParentNameICell,
                                     threadGroupParentName);
            });

            if (threadGroupParentNameStr != NULL) {
                (*env)->ReleaseStringUTFChars(env, threadGroupParentNameICell,
                                              threadGroupParentNameStr);
            }
            if (threadGroupNameStr != NULL) {
                (*env)->ReleaseStringUTFChars(env, threadGroupNameICell,
                                              threadGroupNameStr);
            }
            if (threadNameStr != NULL) {
                (*env)->ReleaseStringUTFChars(env, threadNameICell,
                                              threadNameStr);
            }
            if (threadNameElements != NULL) {
                (*env)->ReleaseCharArrayElements(env,
                                (jcharArray) threadCharArrayICell,
                                threadNameElements, JNI_ABORT);
            }

        } CVMID_localrootEnd();
    }

    return result;
}

/* Purpose: Posts a JVMPI_EVENT_THREAD_END event. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostThreadEndEvent(CVMExecEnv *ee)
{
    JVMPI_Event event;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    CVMjvmpiDisableGC(ee);

    /* Fill event info and notify the profiler: */
    event.event_type = JVMPI_EVENT_THREAD_END;
    event.env_id = CVMexecEnv2JniEnv(ee);
    CVMjvmpiNotifyEvent(&event);

    CVMjvmpiEnableGC(ee);
}

/*=============================================================================
    Methods for initializing and finalizing JVMPI to be called by VM code:
*/

/* Purpose: Sets up the JVMPI & its event notification table. */
static void CVMjvmpiInit(CVMExecEnv *ee)
{
    JVMPI_Interface *intf = CVMjvmpiInterface();

    /* NOTE: By default, all event notifications default to be DISABLED: */
    CVMjvmpiEventInfoInit(&CVMjvmpiRec()->eventInfo, ee);

    /* Initialize the JVMPI_Interface structure: */
    intf->version = JVMPI_VERSION_1;
    intf->NotifyEvent             = CVMjvmpi_NotifyEvent;
    intf->EnableEvent             = CVMjvmpi_EnableEvent;
    intf->DisableEvent            = CVMjvmpi_DisableEvent;
    intf->RequestEvent            = CVMjvmpi_RequestEvent;
    intf->GetCallTrace            = CVMjvmpi_GetCallTrace;
    intf->GetCurrentThreadCpuTime = CVMjvmpi_GetCurrentThreadCpuTime;
    intf->ProfilerExit            = CVMjvmpi_ProfilerExit;
    intf->RawMonitorCreate        = CVMjvmpi_RawMonitorCreate;
    intf->RawMonitorEnter         = CVMjvmpi_RawMonitorEnter;
    intf->RawMonitorExit          = CVMjvmpi_RawMonitorExit;
    intf->RawMonitorWait          = CVMjvmpi_RawMonitorWait;
    intf->RawMonitorNotifyAll     = CVMjvmpi_RawMonitorNotifyAll;
    intf->RawMonitorDestroy       = CVMjvmpi_RawMonitorDestroy;
    intf->SuspendThread           = CVMjvmpi_SuspendThread;
    intf->ResumeThread            = CVMjvmpi_ResumeThread;
    intf->GetThreadStatus         = CVMjvmpi_GetThreadStatus;
    intf->ThreadHasRun            = CVMjvmpi_ThreadHasRun;
    intf->CreateSystemThread      = CVMjvmpi_CreateSystemThread;
    intf->SetThreadLocalStorage   = CVMjvmpi_SetThreadLocalStorage;
    intf->GetThreadLocalStorage   = CVMjvmpi_GetThreadLocalStorage;
    intf->DisableGC               = CVMjvmpi_DisableGC;
    intf->EnableGC                = CVMjvmpi_EnableGC;
    intf->RunGC                   = CVMjvmpi_RunGC;
    intf->GetThreadObject         = CVMjvmpi_GetThreadObject;
    intf->GetMethodClass          = CVMjvmpi_GetMethodClass;
    intf->jobjectID2jobject       = CVMjvmpi_jobjectID2jobject;
    intf->jobject2jobjectID       = CVMjvmpi_jobject2jobjectID;
}

/* Purpose: Returns a pointer to the JVMPI interface structure. */
void *CVMjvmpiGetInterface1(void)
{
    return CVMjvmpiInterface();
}

/* Purpose: Initializes the JVMPI globals. */
void CVMjvmpiInitializeGlobals (CVMExecEnv *ee, CVMJvmpiRecord *record)
{
    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));

    CVMgcLockerInit(&CVMjvmpiRec()->gcLocker);

    /* Setup callback for shutting down properly upon exit: */
    CVMatExit(CVMjvmpiDoCleanup);

    /* Set up event info and the interface */
    CVMjvmpiInit(ee);
}

/* Purpose: Destroys the JVMPI globals. */
void CVMjvmpiDestroyGlobals (CVMExecEnv *ee, CVMJvmpiRecord *record)
{
    CVMgcLockerDestroy(&CVMjvmpiRec()->gcLocker);
}

#ifdef CVM_CLASSLIB_JCOV
/* Purpose: Posts a JVMPI_EVENT_CLASS_LOAD_HOOK for a preloaded class.
   NOTE: This function is used to simulate loading of classes that were
         preloaded.  If exceptions are encountered while doing this, the
         exceptions will be cleared and discarded.
*/
static void
CVMjvmpiPostStartupClassLoadHookEvents(CVMExecEnv *ee,
                                       CVMClassBlock *cb, void *data)
{
    char *classname;
    CVMClassPath *path = (CVMClassPath *)data;
    CVMUint8 *classfile;
    CVMUint32 classfileSize;

    CVMassert(!CVMexceptionOccurred(ee));

    classname = CVMtypeidClassNameToAllocatedCString(CVMcbClassName(cb));
    if (classname == NULL) {
        return;
    }

    classfile = CVMclassLoadSystemClassFileFromPath(ee, classname,
                                                    path, &classfileSize);
    if (CVMexceptionOccurred(ee)) {
        CVMclearLocalException(ee);
        free(classname);
        return;
    }

    if (CVMjvmpiEventClassLoadHookIsEnabled()) {
        CVMUint8 *buffer = (CVMUint8 *)classfile;
        CVMInt32 bufferSize = classfileSize;

        CVMjvmpiPostClassLoadHookEvent(&buffer, &bufferSize,
                                       (void *(*)(unsigned int))&malloc);

        /* If buffer is NULL, then the profiler must have attempted to
           allocate a buffer of its own for instrumentation and failed.  Just
           ignore it.
           If the buffer has changed, then the profiler agent must have
           replaced it with an instrumented version.  We should clean this up
           properly even though we can't use it.
        */
        if ((buffer != NULL) && (buffer != classfile)) {
            free(buffer);
        }
    }

    /* Clean-up resources and return: */
    CVMclassReleaseSystemClassFile(ee, classfile);
    free(classname);
}
#endif

/* Purpose: Posts a JVMPI_EVENT_CLASS_LOAD for a missed class. */
static void
CVMjvmpiPostStartupClassEvents(CVMExecEnv *ee, CVMClassBlock *cb, void *data)
{
    /* If an exception occurred during event notification, then abort: */
    if (CVMexceptionOccurred(ee)) {
        return;
    }

    /* Ignore primitive types: */
    if (CVMcbIs(cb, PRIMITIVE) ) {
        return;
    }

#ifdef CVM_CLASSLIB_JCOV
    if (CVMjvmpiEventClassLoadHookIsEnabled()) {
        CVMjvmpiPostStartupClassLoadHookEvents(ee, cb,
                                               &CVMglobals.bootClassPath);
        /* If an exception occurred during event notification, then abort.
           We cannot go on with more event notifications: */
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }
#endif
    if (CVMjvmpiEventClassLoadIsEnabled()) {
        CVMjvmpiPostClassLoadEvent2(ee, cb, CVM_FALSE);
    }
}

/* Purpose: Posts a JVMPI_EVENT_OBJECT_ALLOC for a missed object. */
static CVMBool
CVMjvmpiPostStartupObjectEvents(CVMObject *obj, CVMClassBlock *cb,
                                CVMUint32 size, void *data)
{
    if (CVMobjectGetClass(obj) != CVMsystemClass(java_lang_Class)) {
        if (CVMjvmpiEventObjectAllocIsEnabled()) {
            CVMExecEnv *ee = (CVMExecEnv *)data;
            CVMjvmpiPostObjectAllocEvent(ee, obj);
            /* If an exception occurred during event notification, then abort.
               We cannot go on with more event notifications: */
            if (CVMexceptionOccurred(ee)) {
                return CVM_FALSE; /* Abort scan. */
            }
        }
    }
    return CVM_TRUE; /* OK to continue scan. */
}

/* Purpose: Posts startup events required by JVMPI.  This includes defining
            events which transpired prior to the completion of VM
            initialization. */
/* NOTE: Called while GC safe. */
void CVMjvmpiPostStartUpEvents(CVMExecEnv *ee)
{
    /* NOTE: It is not trivial to reconstruct the order in which all the
       defining events prior to this point has occurred.  Also, these events
       don't exist for preloaded objects.  The following is a gross
       approximation which will hopefully lessen the need missed events.
       This is a temporary measure until a need for a more comprehensive
       solution is found, or until a way to compute the order is found. */

    CVMassert(CVMD_isgcSafe(ee));

    /* 1. Post the current Thread Start Event first because all events will
          make reference to this thread: */
    if (CVMjvmpiEventThreadStartIsEnabled()) {
        CVMObjectICell *threadICell = CVMcurrentThreadICell(ee);
        CVMjvmpiPostThreadStartEvent(ee, threadICell);
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }

    /* 2. Post GC Arena Events: */
    if (CVMjvmpiEventArenaNewIsEnabled()) {
        CVMgcPostJVMPIArenaNewEvent();
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }

    /* 3. Post Class Load Events: */
    if (CVMjvmpiEventClassLoadIsEnabled()
#ifdef CVM_CLASSLIB_JCOV
        || CVMjvmpiEventClassLoadHookIsEnabled()
#endif
        ) {
        /* Posts a JVMPI_EVENT_CLASS_LOAD event for each ROMized class: */
        CVMclassIterateAllClasses(ee, CVMjvmpiPostStartupClassEvents, 0);
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }

    /* 4. Post Object Allocation Events: */
    if (CVMjvmpiEventObjectAllocIsEnabled()) {
        CVMsysMutexLock(ee, &CVMglobals.heapLock);
        CVMD_gcUnsafeExec(ee, {
            CVMpreloaderIteratePreloadedObjects(ee,
                CVMjvmpiPostStartupObjectEvents, (void *)ee);
            if (!CVMexceptionOccurred(ee)) {
                CVMgcimplIterateHeap(ee, CVMjvmpiPostStartupObjectEvents,
                                     (void *)ee);
            }
        });
        CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }

    /* 5. Post other Thread Start Events: */
    if (CVMjvmpiEventThreadStartIsEnabled()) {
        CVMsysMutexLock(ee, &CVMglobals.heapLock);
        CVMsysMutexLock(ee, &CVMglobals.threadLock);
        CVM_WALK_ALL_THREADS(ee, threadEE, {
            if (ee != threadEE && CVMjvmpiEventThreadStartIsEnabled() &&
                !CVMexceptionOccurred(ee)) {
                CVMObjectICell *threadICell = CVMcurrentThreadICell(threadEE);
                CVMjvmpiPostThreadStartEvent(ee, threadICell);
            }
        });
        CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
        CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }

    /* 5. Post VM Init Event: */
    if (CVMjvmpiEventJVMInitDoneIsEnabled()) {
        CVMjvmpiPostJVMInitDoneEvent();
    }
}

#endif /* CVM_JVMPI */
