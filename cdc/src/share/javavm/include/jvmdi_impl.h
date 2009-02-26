/*
 * @(#)jvmdi_impl.h	1.18 06/10/10
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

#ifndef _INCLUDED_JVMDI_IMPL_H
#define _INCLUDED_JVMDI_IMPL_H

/*
 * This header file (originally JDK 1.2's "breakpoints.h") defines the
 * JVMDI functions the interpreter, garbage collector, etc. call to
 * notify the debugging agent of events.
 *
 * CVM source code using these functions should #include
 * "jvmdi_impl.h", which itself #includes "export/jvmdi.h". Client
 * source code (i.e., the debugging agent) should #include "jvmdi.h"
 * as usual.
 */

/*
 * Definitions for the debugger events and operations.
 * Only for use when CVM_JVMDI is defined.
 */

#ifdef CVM_JVMDI

#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/stdlib.h"

#include "javavm/include/defs.h"
#include "javavm/export/jni.h"

/* This #define is necessary to prevent the declaration of 2 jvmdi static
   variables (in jvmdi.h) which is not needed in the VM: */
#ifndef NO_JVMDI_MACROS
#define NO_JVMDI_MACROS
#endif
#include "javavm/export/jvmdi.h"

#define CVM_DEBUGGER_LOCK(ee)   \
	CVMsysMutexLock(ee, &CVMglobals.debuggerLock)
#define CVM_DEBUGGER_UNLOCK(ee) \
	CVMsysMutexUnlock(ee, &CVMglobals.debuggerLock)
#define CVM_DEBUGGER_IS_LOCKED(ee)					\
         CVMreentrantMutexIAmOwner(ee,					\
	   CVMsysMutexGetReentrantMutex(&CVMglobals.debuggerLock))

/*
 * JVMDI globally defined static variables.
 * jvmdiInitialized tells whether JVMDI initialization is done once
 *                  before anything that accesses event structures.
 */
typedef struct ThreadNode_ {
    CVMObjectICell* thread;        /* Global root; always allocated */
    jobject lastDetectedException; /* JNI Global Ref; allocated in
                                      CVMjvmdiNotifyDebuggerOfException */
    jboolean eventEnabled[JVMDI_MAX_EVENT_TYPE_VAL+1];
    JVMDI_StartFunction startFunction;  /* for debug threads only */
    void *startFunctionArg;             /* for debug threads only */
    struct ThreadNode_ *next;
} ThreadNode;

typedef struct CVMjvmdiStatics {
    /* event hooks, etc */
    CVMBool jvmdiInitialized;
    JavaVM* vm;
    JVMDI_EventHook eventHook;
    JVMDI_AllocHook allocHook;
    JVMDI_DeallocHook deallocHook;
    struct CVMBag* breakpoints;
    struct CVMBag* framePops;
    struct CVMBag* watchedFieldModifications;
    struct CVMBag* watchedFieldAccesses;
    volatile ThreadNode *threadList;
    CVMUint32 eventEnable[JVMDI_MAX_EVENT_TYPE_VAL+1];
} JVMDI_Static;

/*
 * This section for use by the interpreter.
 *
 * NOTE: All of these routines are implicitly gc-safe. When the
 * interpreter calls them, the call site must be wrapped in an
 * CVMD_gcSafeExec() block.
 */

extern void CVMjvmdiNotifyDebuggerOfException(CVMExecEnv* ee,
					      CVMUint8* pc,
					      CVMObjectICell* object);
extern void CVMjvmdiNotifyDebuggerOfExceptionCatch(CVMExecEnv* ee,
						   CVMUint8* pc,
						   CVMObjectICell* object);
extern void CVMjvmdiNotifyDebuggerOfSingleStep(CVMExecEnv* ee,
					       CVMUint8* pc);
/** OBJ is expected to be NULL for static fields */
extern void CVMjvmdiNotifyDebuggerOfFieldAccess(CVMExecEnv* ee,
						CVMObjectICell* obj,
						CVMFieldBlock* fb);
/** OBJ is expected to be NULL for static fields */
extern void CVMjvmdiNotifyDebuggerOfFieldModification(CVMExecEnv* ee,
						      CVMObjectICell* obj,
						      CVMFieldBlock* fb,
						      jvalue jval);
extern void CVMjvmdiNotifyDebuggerOfThreadStart(CVMExecEnv* ee,
						CVMObjectICell* thread);
extern void CVMjvmdiNotifyDebuggerOfThreadEnd(CVMExecEnv* ee,
					      CVMObjectICell* thread);
/* The next two are a bit of a misnomer. If requested, the JVMDI
   reports three types of events to the caller: method entry (for all
   methods), method exit (for all methods), and frame pop (for a
   single frame identified in a previous call to notifyFramePop). The
   "frame push" method below may cause a method entry event to be sent
   to the debugging agent. The "frame pop" method below may cause a
   method exit event, frame pop event, both, or neither to be sent to
   the debugging agent. Also note that
   CVMjvmdiNotifyDebuggerOfFramePop (originally
   notify_debugger_of_frame_pop) checks for a thrown exception
   internally in order to allow the calling code to be sloppy about
   ensuring that events aren't sent if an exception occurred. */
extern void CVMjvmdiNotifyDebuggerOfFramePush(CVMExecEnv* ee);
extern void CVMjvmdiNotifyDebuggerOfFramePop(CVMExecEnv* ee);
extern void CVMjvmdiNotifyDebuggerOfClassLoad(CVMExecEnv* ee,
					      CVMObjectICell* clazz);
extern void CVMjvmdiNotifyDebuggerOfClassPrepare(CVMExecEnv* ee,
						 CVMObjectICell* clazz);
extern void CVMjvmdiNotifyDebuggerOfClassUnload(CVMExecEnv* ee,
						CVMObjectICell* clazz);
extern void CVMjvmdiNotifyDebuggerOfVmInit(CVMExecEnv* ee);
extern void CVMjvmdiNotifyDebuggerOfVmExit(CVMExecEnv* ee);

extern CVMUint8 CVMjvmdiGetBreakpointOpcode(CVMExecEnv* ee, CVMUint8* pc,
					    CVMBool notify);
extern CVMBool CVMjvmdiSetBreakpointOpcode(CVMExecEnv* ee, CVMUint8* pc,
					   CVMUint8 opcode);
extern void CVMjvmdiStaticsInit(struct CVMjvmdiStatics * statics);
extern void CVMjvmdiStaticsDestroy(struct CVMjvmdiStatics * statics);

#define CVMjvmdiEventsEnabled()	\
    (CVMglobals.jvmdiStatics.eventHook != NULL)

#define CVMjvmdiThreadEventsEnabled(ee)	\
    ((ee)->debugEventsEnabled && CVMglobals.jvmdiStatics.eventHook != NULL)

/*
 * This function is used by CVMjniGetEnv.
 */
extern JVMDI_Interface_1* CVMjvmdiGetInterface1(JavaVM* interfaces_vm);

#endif /* CVM_JVMDI */

#endif /* !_INCLUDED_JVMDI_IMPL_H */
