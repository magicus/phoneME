/*
 * @(#)jvmdi.c	1.140 06/10/10
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
 * Breakpoints, single-stepping and debugger event notification.
 * Debugger event notification/handling includes breakpoint, single-step
 * and exceptions.
 */

#ifdef CVM_JVMDI

#include "javavm/include/porting/ansi/stdarg.h"
#include "javavm/include/defs.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/localroots.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/signature.h"
#include "javavm/include/globals.h"
#include "javavm/include/bag.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/named_sys_monitor.h"
#include "javavm/include/opcodes.h"
#include "generated/offsets/java_lang_Thread.h"
#include "generated/jni/java_lang_reflect_Modifier.h"
#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"

#include "javavm/include/jvmdi_impl.h"

#ifdef CVM_HW
#include "include/hw.h"
#endif

/* %comment: k001 */


/* (This is unused in CVM -- only used in unimplemented GetBytecodes) */
/* Defined in inline.c thru an include of "opcodes.length". This is dangerous
   as is. There should be a header file with this extern shared by both .c files */
/* extern const short opcode_length[256]; */

#undef DEBUGGER_LOCK
#undef DEBUGGER_UNLOCK
#undef DEBUGGER_IS_LOCKED
#define DEBUGGER_LOCK(ee)      CVM_DEBUGGER_LOCK(ee)
#define DEBUGGER_UNLOCK(ee)    CVM_DEBUGGER_UNLOCK(ee)
#define DEBUGGER_IS_LOCKED(ee) CVM_DEBUGGER_IS_LOCKED(ee)

/* Convenience macros */
#define CVM_THREAD_LOCK(ee)   CVMsysMutexLock(ee, &CVMglobals.threadLock)
#define CVM_THREAD_UNLOCK(ee) CVMsysMutexUnlock(ee, &CVMglobals.threadLock)

/* NOTE: "Frame pop sentinels" removed in CVM version of JVMDI
   because we don't have two return PC slots and mangling the only
   available one is incorrect if an exception is thrown. See
   CVMjvmdiNotifyDebuggerOfFramePop and jvmdi_NotifyFramePop below. */

/* #define FRAME_POP_SENTINEL ((unsigned char *)1) */

#define INITIAL_BAG_SIZE 4

struct bkpt {
    CVMUint8* pc;      /* key - must be first */
    CVMUint8 opcode;   /* opcode to restore */
    jobject classRef;       /* Prevents enclosing class from being gc'ed */
};

struct fpop {
    CVMFrame* frame;       /* key - must be first */
    /* CVMUint8* returnpc; */ /* Was used for PC mangling in JDK version.
				 Now just indicates set membership. */
};

struct fieldWatch {
    CVMFieldBlock* fb;  /* field to watch; key - must be first */
    jclass classRef;        /* Prevents enclosing class from being gc'ed */
};

/* additional local frames to push to cover usage by client debugger */
#define LOCAL_FRAME_SLOP 10

#define JVMDI_EVENT_GLOBAL_MASK 0xf0000000
#define JVMDI_EVENT_THREAD_MASK 0x7fffffff

static void enableAllEvents(jint eventType, jboolean enabled) {
    if (enabled) {
        CVMglobals.jvmdiStatics.eventEnable[eventType] |=  
	    JVMDI_EVENT_GLOBAL_MASK;
    } else {
        CVMglobals.jvmdiStatics.eventEnable[eventType] &=  
	    ~JVMDI_EVENT_GLOBAL_MASK;
    }
}

static void enableThreadEvents(CVMExecEnv* ee, ThreadNode *node, 
                               jint eventType, jboolean enabled) {
    CVMUint32 count;

    CVMassert(DEBUGGER_IS_LOCKED(ee));

    /*
     * If state is changing, update the global eventEnable word
     * for this thread and the per-thread boolean flag.
     */
    if (node->eventEnabled[eventType] != enabled) {
        node->eventEnabled[eventType] = enabled;

        count = CVMglobals.jvmdiStatics.eventEnable[eventType] & 
	    JVMDI_EVENT_THREAD_MASK;
        if (enabled) {
            count++;
        } else {
            count--;
        }
        CVMglobals.jvmdiStatics.eventEnable[eventType] = 
            (JVMDI_EVENT_GLOBAL_MASK & 
	     CVMglobals.jvmdiStatics.eventEnable[eventType]) |
            (JVMDI_EVENT_THREAD_MASK & count);
    }
}

static jboolean threadEnabled(jint eventType, ThreadNode *node) {
    return (node == NULL) ? JNI_FALSE : node->eventEnabled[eventType];
}

#define GLOBALLY_ENABLED(eventType)  (CVMglobals.jvmdiStatics.eventEnable[eventType] & JVMDI_EVENT_GLOBAL_MASK)

/*
 * This macro is used in notify_debugger_* to determine whether a JVMDI 
 * event should be generated. Note that this takes an expression for the thread node
 * which we should avoid evaluating twice. That's why threadEnabled above
 * is a function, not a macro
 *
 * Also the first eventEnable[eventType] is redundant, but it serves as
 * a quick filter of events that are completely unreported.
 */
#define MUST_NOTIFY(eventType, threadNodeExpr) \
             (CVMglobals.jvmdiStatics.eventEnable[eventType] && \
              CVMglobals.jvmdiStatics.eventHook && \
              ( GLOBALLY_ENABLED(eventType) || \
                threadEnabled(eventType, threadNodeExpr)))

/* forward defs */
static jvmdiError JNICALL
jvmdi_Allocate(jlong size, jbyte** memPtr);
static jvmdiError JNICALL
jvmdi_Deallocate(jbyte* mem);
#ifdef JDK12
static void
handleExit(void);
#endif

static ThreadNode *findThread(CVMExecEnv* ee, CVMObjectICell* thread);
static ThreadNode *insertThread(CVMExecEnv* ee, CVMObjectICell* thread);

/*
 * Initialize JVMDI - if and only if it hasn't been initialized.
 * Must be called before anything that accesses event structures.
 */
static jvmdiError
initializeJVMDI() 
{
    CVMExecEnv *ee = CVMgetEE();
    CVMBool haveFailure = CVM_FALSE;

    /* %comment: k003 */
    if (CVMglobals.jvmdiStatics.jvmdiInitialized) {
	return JVMDI_ERROR_NONE;
    }

#ifdef JDK12
    CVMatExit(handleExit);
#endif

    CVMglobals.jvmdiStatics.breakpoints = 
        CVMbagCreateBag(sizeof(struct bkpt), INITIAL_BAG_SIZE);
    CVMglobals.jvmdiStatics.framePops = CVMbagCreateBag(
	sizeof(struct fpop), INITIAL_BAG_SIZE);
    CVMglobals.jvmdiStatics.watchedFieldModifications = 
        CVMbagCreateBag(sizeof(struct fieldWatch), INITIAL_BAG_SIZE);
    CVMglobals.jvmdiStatics.watchedFieldAccesses = 
        CVMbagCreateBag(sizeof(struct fieldWatch), INITIAL_BAG_SIZE);
    if (CVMglobals.jvmdiStatics.breakpoints == NULL || 
	CVMglobals.jvmdiStatics.framePops == NULL || 
	CVMglobals.jvmdiStatics.watchedFieldModifications == NULL || 
        CVMglobals.jvmdiStatics.watchedFieldAccesses == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    /*
     * Setup the events to be enabled/disabled on startup. All events
     * are enabled except single step, exception catch, method entry/exit.
     */
    enableAllEvents(JVMDI_EVENT_THREAD_START, JNI_TRUE);
    enableAllEvents(JVMDI_EVENT_THREAD_END, JNI_TRUE);
    enableAllEvents(JVMDI_EVENT_CLASS_LOAD, JNI_TRUE);
    enableAllEvents(JVMDI_EVENT_CLASS_PREPARE, JNI_TRUE);
    enableAllEvents(JVMDI_EVENT_CLASS_UNLOAD, JNI_TRUE);
    enableAllEvents(JVMDI_EVENT_FIELD_ACCESS, JNI_TRUE);
    enableAllEvents(JVMDI_EVENT_FIELD_MODIFICATION, JNI_TRUE);
    enableAllEvents(JVMDI_EVENT_BREAKPOINT, JNI_TRUE);
    enableAllEvents(JVMDI_EVENT_FRAME_POP, JNI_TRUE);
    enableAllEvents(JVMDI_EVENT_EXCEPTION, JNI_TRUE);
    enableAllEvents(JVMDI_EVENT_USER_DEFINED, JNI_TRUE);

    /* NOTE: We must not trigger a GC while holding the thread lock.
       We are safe here because insertThread() can allocate global roots
       which can cause expansion of the global root stack, but not cause
       a GC. */
    CVM_THREAD_LOCK(ee);

    /* Log all thread that were created prior to JVMDI's initialization: */
    /* NOTE: We are only logging the pre-existing threads into the JVMDI
       threads list.  We don't send currently send JVMDI_EVENT_THREAD_START
       for these threads that started before JVMDI was initialized. */
    CVM_WALK_ALL_THREADS(ee, currentEE, {
        jthread thread = CVMcurrentThreadICell(currentEE);
        if (!haveFailure && !CVMID_icellIsNull(thread)) {
            ThreadNode *node = findThread(ee, thread);
            if (node == NULL) {
                node = insertThread(ee, thread);
                if (node == NULL) {
                    haveFailure = CVM_TRUE;
                }
            }
        }
    });

    CVM_THREAD_UNLOCK(ee);

    /* Abort if we detected a failure while trying to log threads: */
    if (haveFailure) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }

    CVMglobals.jvmdiStatics.jvmdiInitialized = CVM_TRUE;

    return JVMDI_ERROR_NONE;
}

/*
 * These functions maintain the linked list of currently running threads. 
 */
static ThreadNode *
findThread(CVMExecEnv* ee, CVMObjectICell* thread) {
    ThreadNode *node;
    CVMBool thrEq;

    DEBUGGER_LOCK(ee);

    /* cast away volatility */
    node = (ThreadNode *)CVMglobals.jvmdiStatics.threadList;  
    while (node != NULL) {
	CVMID_icellSameObject(ee, node->thread, thread, thrEq);
	if (thrEq) {
            break;
        }
        node = node->next;
    }
    DEBUGGER_UNLOCK(ee);

    return node;
}

static ThreadNode *
insertThread(CVMExecEnv* ee, CVMObjectICell* thread) {
    ThreadNode *node;

    /* NOTE: you could move the locking and unlocking inside the if
       clause in such a way as to avoid the problem with seizing both
       the debugger and global root lock at the same time, but it
       wouldn't solve the problem of removeThread, which is harder,
       nor the (potential) problem of the several routines which
       perform indirect memory accesses while holding the debugger
       lock; if there was ever a lock associated
       with those accesses there would be a problem. */
    DEBUGGER_LOCK(ee);

    node = (ThreadNode *)malloc(sizeof(*node));
    if (node != NULL) {
	node->thread = CVMID_getGlobalRoot(ee);
	if (node->thread == NULL) {
	    goto fail0;
	}
        node->lastDetectedException = CVMID_getGlobalRoot(ee);
	if (node->lastDetectedException == NULL) {
	    goto fail1;
	}

	CVMID_icellAssign(ee, node->thread, thread);

        memset(node->eventEnabled, 0, sizeof(node->eventEnabled));
        node->startFunction = NULL;
        node->startFunctionArg = NULL;
	/* cast away volatility */
        node->next = (ThreadNode *)CVMglobals.jvmdiStatics.threadList; 
        CVMglobals.jvmdiStatics.threadList = node;
    } 

 unlock:
    DEBUGGER_UNLOCK(ee);

    return node;

fail1:
    CVMID_freeGlobalRoot(ee, node->lastDetectedException);
fail0:
    free(node);
    node = NULL;
    goto unlock;
}

static jboolean 
removeThread(CVMObjectICell* thread) {
    ThreadNode *previous = NULL;
    ThreadNode *node; 
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMBool thrEq;
    jboolean rc = JNI_FALSE;

    DEBUGGER_LOCK(ee);

    /* cast away volatility */
    node = (ThreadNode *)CVMglobals.jvmdiStatics.threadList;  
    while (node != NULL) {
	CVMID_icellSameObject(ee, node->thread, thread, thrEq);
        if (thrEq) {
            int i;
            if (previous == NULL) {
                CVMglobals.jvmdiStatics.threadList = node->next;
            } else {
                previous->next = node->next;
            }
            for (i = 0; i <= JVMDI_MAX_EVENT_TYPE_VAL; i++) {
                enableThreadEvents(ee, node, i, JNI_FALSE);
            }
	    CVMID_freeGlobalRoot(ee, node->thread);
	    (*env)->DeleteGlobalRef(env, node->lastDetectedException);

            free((void *)node);
            rc = JNI_TRUE;
            break;
        }
        previous = node;
        node = node->next;
    }
    DEBUGGER_UNLOCK(ee);

    return rc;
}


static void
reportException(CVMExecEnv* ee, CVMUint8 *pc,
                CVMObjectICell* object, CVMFrame* frame) {
    JVMDI_Event event;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMMethodBlock* mb = frame->mb;
    CVMClassBlock* exceptionCb;
    CVMJavaLong sz;
    CVMFrameIterator iter;

    if (mb == NULL) {
        return;
    }

    /* NOTE: MUST BE A JAVA METHOD */
    CVMassert(!CVMmbIs(mb, NATIVE));

    CVMID_objectGetClass(ee, object, exceptionCb);

    event.kind = JVMDI_EVENT_EXCEPTION;
    event.u.exception.thread =
	(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
    event.u.exception.clazz =
	(*env)->NewLocalRef(env, CVMcbJavaInstance(CVMmbClassBlock(mb)));
    event.u.exception.method = (jmethodID)mb;
    sz = CVMint2Long(pc - CVMmbJavaCode(mb));
    event.u.exception.location = sz;
    event.u.exception.exception = (*env)->NewLocalRef(env, object);
    event.u.exception.catch_clazz = 0;
    event.u.exception.catch_method = 0;
    event.u.exception.catch_location = CVMlongConstZero();

    /* walk up the stack to see if this exception is caught anywhere. */

    CVMframeIterateInit(&iter, frame);

    while (CVMframeIterateNextSpecial(&iter, CVM_FALSE)) {
	/* %comment: k004 */
	/* skip transition frames, which can't catch exceptions */
        if (CVMframeIterateCanHaveJavaCatchClause(&iter)) {
	    CVMMethodBlock* cmb = frame->mb;
	    /* %comment: k005 */
	    if (cmb != NULL) {
		CVMUint8* pc = CVMframeIterateGetJavaPc(&iter);
		CVMUint8* cpc =
		    CVMgcSafeFindPCForException(ee, &iter,
						exceptionCb,
						pc);
		if (cpc != NULL) {
		    /* NOTE: MUST BE A JAVA METHOD */
		    CVMassert(!CVMmbIs(cmb, NATIVE));
		    event.u.exception.catch_clazz = (jclass)
			(*env)->NewLocalRef(env,
			    CVMcbJavaInstance(CVMmbClassBlock(cmb)));
		    event.u.exception.catch_method = cmb;
		    sz = CVMint2Long(cpc - CVMmbJavaCode(cmb));
		    event.u.exception.catch_location = sz;
		    break;
		}
	    }
	}
    }

    CVMglobals.jvmdiStatics.eventHook(env, &event);
}

/* %comment: k006 */
void
CVMjvmdiNotifyDebuggerOfException(CVMExecEnv* ee, CVMUint8 *pc, CVMObjectICell* object)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    ThreadNode *threadNode;
    CVMBool exceptionsEqual = CVM_FALSE;

    /* This check is probably unnecessary */
    if (CVMcurrentThreadICell(ee) == NULL) {
        return;
    }

    threadNode = findThread(ee, CVMcurrentThreadICell(ee));
    if (threadNode == NULL) {
        /* ignore any exceptions before thread start */
        return;
    }

    CVMID_icellSameObject(ee, threadNode->lastDetectedException,
			  object,
			  exceptionsEqual);

    if (!exceptionsEqual) {
        /* Get current frame before another one is pushed */
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);

        if ((*env)->PushLocalFrame(env, 5+LOCAL_FRAME_SLOP) < 0) {
            return;
        }
        
        if (MUST_NOTIFY(JVMDI_EVENT_EXCEPTION, threadNode)) {
            jobject pendingException = (*env)->ExceptionOccurred(env);

            /*
             * Save the pending exception so it does not get
             * overwritten if CVMgcSafeFindPCForException() throws an
             * exception. Also, in classloading builds,
             * CVMgcSafeFindPCForException may cause loading of the
             * exception class, which code expects no exception to
             * have occurred upon entry.
	     */

            (*env)->ExceptionClear(env);

            reportException(ee, pc, pendingException, frame);

            /*
             * Restore the exception state
             */
            if (pendingException != NULL) {
                (*env)->Throw(env, pendingException);
            } else {
                (*env)->ExceptionClear(env);
            }
	}

        /*
         * This needs to be a global ref; otherwise, the detected
         * exception could be cleared and collected in a native method
         * causing later comparisons to be invalid.
         */
	CVMID_icellAssign(ee, threadNode->lastDetectedException, object);

        (*env)->PopLocalFrame(env, 0);
    }
}

/*
 * This function is called by the interpreter whenever:
 * 1) The interpreter catches an exception, or
 * 2) The interpreter detects that a native method has returned
 *    without an exception set (i.e. the native method *may* have
 *    cleared an exception; cannot tell for sure)
 *
 * This function performs 2 tasks. It removes any exception recorded
 * as the last detected exception by notify_debugger_of_exception. It
 * also reports an event to the JVMDI client. (2) above implies that 
 * we need to make sure that an exception was actually cleared before
 * reporting the event. This 
 * can be done by checking whether there is currently a last detected
 * detected exception value saved for this thread.
 */
void
CVMjvmdiNotifyDebuggerOfExceptionCatch(CVMExecEnv* ee, CVMUint8 *pc, CVMObjectICell* object)
{
    ThreadNode *threadNode;
    CVMJavaLong sz;
    
    /* This check is probably unnecessary */
    if (CVMcurrentThreadICell(ee) == NULL) {
        return;
    }

    threadNode = findThread(ee, CVMcurrentThreadICell(ee));
    if (threadNode == NULL) {
        /* ignore any exceptions before thread start */
        return;
    }

    if (MUST_NOTIFY(JVMDI_EVENT_EXCEPTION_CATCH, threadNode)) {
        JVMDI_Event event;
        JNIEnv* env = CVMexecEnv2JniEnv(ee);
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	CVMMethodBlock* mb = frame->mb;

        if (mb == NULL) {
            return;
        }

	/* NOTE: MUST BE A JAVA METHOD */
	CVMassert(!CVMmbIs(mb, NATIVE));

        /*
         * Report the caught exception if it is being caught in Java code
         * or if it was caught in native code after its throw was reported 
         * earlier.
         */
        if ((object != NULL) || 
	    !CVMID_icellIsNull(threadNode->lastDetectedException)) {
            
            if ((*env)->PushLocalFrame(env, 3+LOCAL_FRAME_SLOP) < 0) {
                return;
            }
            
            event.kind = JVMDI_EVENT_EXCEPTION_CATCH;
            event.u.exception_catch.thread =
		(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
            event.u.exception_catch.clazz =
		(*env)->NewLocalRef(env,
				    CVMcbJavaInstance(CVMmbClassBlock(mb)));
            event.u.exception_catch.method = mb;
	    sz = CVMint2Long(pc - CVMmbJavaCode(mb));
            event.u.exception_catch.location = sz;
            if (object != NULL) {
                event.u.exception_catch.exception =
		    (*env)->NewLocalRef(env, object);
            } else {
                event.u.exception_catch.exception = NULL;
            }
    
            CVMglobals.jvmdiStatics.eventHook(env, &event);
            (*env)->PopLocalFrame(env, 0);
        }
    }
    CVMID_icellSetNull(threadNode->lastDetectedException);
}

void
CVMjvmdiNotifyDebuggerOfSingleStep(CVMExecEnv* ee, CVMUint8 *pc)
{
    /*
     * The interpreter notifies us only for threads that have stepping enabled,
     * so we don't do any checks of the global or per-thread event
     * enable flags here.
     */

    if (CVMglobals.jvmdiStatics.eventHook) {
        JVMDI_Event event;
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	CVMMethodBlock* mb = frame->mb;
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	CVMJavaLong sz;

	if (CVMframeIsTransition(frame) || mb == NULL) {
	    return;
	}

	/* NOTE: MUST BE A JAVA METHOD */
	CVMassert(!CVMmbIs(mb, NATIVE));

	if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	    return;
        }
	event.kind = JVMDI_EVENT_SINGLE_STEP;
	event.u.single_step.thread =
	    (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
	event.u.single_step.clazz =
	    (*env)->NewLocalRef(env,
				CVMcbJavaInstance(CVMmbClassBlock(mb)));
	event.u.single_step.method = mb;
	sz = CVMint2Long(pc - CVMmbJavaCode(mb));
	event.u.single_step.location = sz;

	CVMglobals.jvmdiStatics.eventHook(env, &event);

	(*env)->PopLocalFrame(env, 0);
    }
}

static void
notify_debugger_of_breakpoint(CVMExecEnv* ee, CVMUint8 *pc)
{
    if (MUST_NOTIFY(JVMDI_EVENT_BREAKPOINT,
		    findThread(ee, CVMcurrentThreadICell(ee)))) {
        JVMDI_Event event;
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	CVMMethodBlock* mb = frame->mb;
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	CVMJavaLong sz;

	if (mb == NULL) {
	    return;
	}
	/* NOTE: MUST BE A JAVA METHOD */
	CVMassert(!CVMmbIs(mb, NATIVE));
	if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	    return;
        }
	event.kind = JVMDI_EVENT_BREAKPOINT;
	event.u.breakpoint.thread =
	    (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
	event.u.breakpoint.clazz =
	    (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMmbClassBlock(mb)));
	event.u.breakpoint.method = mb;
	sz = CVMint2Long(pc - CVMmbJavaCode(mb));
	event.u.breakpoint.location = sz;

	CVMglobals.jvmdiStatics.eventHook(env, &event);

	(*env)->PopLocalFrame(env, 0);
    }
}

void
CVMjvmdiNotifyDebuggerOfThreadStart(CVMExecEnv* ee, CVMObjectICell* thread)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    /*
     * Look for existing thread info for this thread. If there is 
     * a ThreadNode already, it just means that it's a debugger thread
     * started by jvmdi_RunDebugThread; if not, we create the ThreadNode
     * here. 
     */
    ThreadNode *node = findThread(ee, thread);
    if (node == NULL) {
        node = insertThread(ee, thread);
        if (node == NULL) {
            (*env)->FatalError(env, "internal allocation error in JVMDI");
        }
    }

    if (CVMglobals.jvmdiStatics.eventHook && 
	CVMglobals.jvmdiStatics.eventEnable[JVMDI_EVENT_THREAD_START]) {
        JVMDI_Event event;

        if ((*env)->PushLocalFrame(env, 1+LOCAL_FRAME_SLOP) < 0) {
            return;
        }
        event.kind = JVMDI_EVENT_THREAD_START;
        event.u.thread_change.thread =
	    (*env)->NewLocalRef(env, thread);

        CVMglobals.jvmdiStatics.eventHook(env, &event);

        (*env)->PopLocalFrame(env, 0);
    }
}

void
CVMjvmdiNotifyDebuggerOfThreadEnd(CVMExecEnv* ee, CVMObjectICell* thread)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    if (MUST_NOTIFY(JVMDI_EVENT_THREAD_END, findThread(ee, thread))) {
        JVMDI_Event event;

        if ((*env)->PushLocalFrame(env, 1+LOCAL_FRAME_SLOP) < 0) {
            goto forgetThread;
        }
        event.kind = JVMDI_EVENT_THREAD_END;
        event.u.thread_change.thread =
	    (*env)->NewLocalRef(env, thread);

        CVMglobals.jvmdiStatics.eventHook(env, &event);

        (*env)->PopLocalFrame(env, 0);
    }

forgetThread:
    if (removeThread(thread) == JNI_FALSE) {
        (*env)->FatalError(env, "internal error in JVMDI (ending unstarted thread)");
    }
}

void
CVMjvmdiNotifyDebuggerOfFieldAccess(CVMExecEnv* ee, CVMObjectICell* obj,
				    CVMFieldBlock* fb)
{
    struct fieldWatch *fwfb;

    DEBUGGER_LOCK(ee);
    fwfb = CVMbagFind(CVMglobals.jvmdiStatics.watchedFieldAccesses, fb);
    DEBUGGER_UNLOCK(ee);          

    if (fwfb != NULL &&
	MUST_NOTIFY(JVMDI_EVENT_FIELD_ACCESS, 
		    findThread(ee, CVMcurrentThreadICell(ee)))) {
        JVMDI_Event event;
        CVMFrame* frame = CVMeeGetCurrentFrame(ee);
        CVMMethodBlock* mb;
        jlocation location;
        JNIEnv* env = CVMexecEnv2JniEnv(ee);

        /* We may have come through native code - remove any
         * empty frames.
         */

	/* %comment: k007 */

        frame = CVMgetCallerFrame(frame, 0);
        if (frame == NULL || (mb = frame->mb) == NULL) {
            return;
        }
	if (CVMframeIsInterpreter(frame)) {
	    CVMInterpreterFrame* interpFrame = CVMgetInterpreterFrame(frame);
	    location = CVMint2Long(CVMframePc(interpFrame) - CVMmbJavaCode(mb));
	} else {
            location = CVMint2Long(-1);
	}

        if ((*env)->PushLocalFrame(env, 4+LOCAL_FRAME_SLOP) < 0) {
            return;
        }
        event.kind = JVMDI_EVENT_FIELD_ACCESS;
        event.u.field_access.thread =
	    (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
        event.u.field_access.clazz =
	    (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMmbClassBlock(mb)));
        event.u.field_access.method = mb;
        event.u.field_access.location = location;
        event.u.field_access.field_clazz =
	    (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMfbClassBlock(fb)));
        event.u.field_access.field = fb;
        event.u.field_access.object =  obj == NULL? 
                                                NULL : 
                                                (*env)->NewLocalRef(env, obj);

        CVMglobals.jvmdiStatics.eventHook(env, &event);

        (*env)->PopLocalFrame(env, 0);
    }
}

void
CVMjvmdiNotifyDebuggerOfFieldModification(CVMExecEnv* ee, CVMObjectICell* obj,
					  CVMFieldBlock* fb, jvalue jval)
{
    struct fieldWatch *fwfb;

    DEBUGGER_LOCK(ee);
    fwfb = CVMbagFind(CVMglobals.jvmdiStatics.watchedFieldModifications, fb);
    DEBUGGER_UNLOCK(ee);          

    if (fwfb != NULL &&
	MUST_NOTIFY(JVMDI_EVENT_FIELD_MODIFICATION, 
		    findThread(ee, CVMcurrentThreadICell(ee)))) {
        JVMDI_Event event;
        CVMFrame* frame = CVMeeGetCurrentFrame(ee);
        CVMMethodBlock* mb;
        jlocation location;
        JNIEnv* env = CVMexecEnv2JniEnv(ee);
	CVMFieldTypeID tid = CVMfbNameAndTypeID(fb);
	char sig_type;

	if (CVMtypeidIsPrimitive(tid)) {
	    sig_type = CVMterseTypePrimitiveSignatures[CVMtypeidGetType(tid)];
	} else if (CVMtypeidIsArray(tid)) {
	    sig_type = CVM_SIGNATURE_ARRAY;
	} else {
	    sig_type = CVM_SIGNATURE_CLASS;
	}

        /* We may have come through native code - remove any
         * empty frames.
         */

	/* %comment: k007 */

        frame = CVMgetCallerFrame(frame, 0);
        if (frame == NULL || (mb = frame->mb) == NULL) {
            return;
        }
	if (CVMframeIsInterpreter(frame)) {
	    CVMInterpreterFrame* interpFrame = CVMgetInterpreterFrame(frame);
	    location = CVMint2Long(CVMframePc(interpFrame) - CVMmbJavaCode(mb));
	} else {
            location = CVMint2Long(-1);
	}

        if ((*env)->PushLocalFrame(env, 5+LOCAL_FRAME_SLOP) < 0) {
            return;
        }
        if ((sig_type == CVM_SIGNATURE_CLASS) ||
	    (sig_type == CVM_SIGNATURE_ARRAY)) {
            jval.l = (*env)->NewLocalRef(env, jval.l);
        }
        event.kind = JVMDI_EVENT_FIELD_MODIFICATION;
        event.u.field_modification.thread =
	    (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
        event.u.field_modification.clazz =
	    (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMmbClassBlock(mb)));
        event.u.field_modification.method = mb;
        event.u.field_modification.location = location;
        event.u.field_modification.field_clazz =
	    (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMfbClassBlock(fb)));
        event.u.field_modification.field = fb;
        event.u.field_modification.object = (obj == NULL? 
					     NULL : 
					     (*env)->NewLocalRef(env, obj));
        event.u.field_modification.signature_type = sig_type;
        event.u.field_modification.new_value = jval;

        CVMglobals.jvmdiStatics.eventHook(env, &event);

        (*env)->PopLocalFrame(env, 0);
    }
}

void
CVMjvmdiNotifyDebuggerOfClassLoad(CVMExecEnv* ee, CVMObjectICell* clazz)
{
    if (MUST_NOTIFY(JVMDI_EVENT_CLASS_LOAD,
	            findThread(ee, CVMcurrentThreadICell(ee)))) {
        JVMDI_Event event;
        JNIEnv *env = CVMexecEnv2JniEnv(ee);

        if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
            return;
        }
        event.kind = JVMDI_EVENT_CLASS_LOAD;
        event.u.class_event.thread =
	    (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
        event.u.class_event.clazz =
	    (*env)->NewLocalRef(env, clazz);

        CVMglobals.jvmdiStatics.eventHook(env, &event);

        (*env)->PopLocalFrame(env, 0);
    }
}

void
CVMjvmdiNotifyDebuggerOfClassPrepare(CVMExecEnv* ee, CVMObjectICell* clazz)
{
    if (MUST_NOTIFY(JVMDI_EVENT_CLASS_PREPARE,
		    findThread(ee, CVMcurrentThreadICell(ee)))) {
        JVMDI_Event event;
        JNIEnv *env = CVMexecEnv2JniEnv(ee);

        if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
            return;
        }
        event.kind = JVMDI_EVENT_CLASS_PREPARE;
        event.u.class_event.thread =
	    (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
        event.u.class_event.clazz =
	    (*env)->NewLocalRef(env, clazz);

        CVMglobals.jvmdiStatics.eventHook(env, &event);

        (*env)->PopLocalFrame(env, 0);
    }
}

void
CVMjvmdiNotifyDebuggerOfClassUnload(CVMExecEnv* ee, CVMObjectICell* clazz)
{
    if (CVMglobals.jvmdiStatics.eventHook && 
	CVMglobals.jvmdiStatics.eventEnable[JVMDI_EVENT_CLASS_UNLOAD]) {
        JVMDI_Event event;
        JNIEnv *env = CVMexecEnv2JniEnv(ee);

        if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
            return;
        }
        event.kind = JVMDI_EVENT_CLASS_UNLOAD;
        event.u.class_event.thread =
	    (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
        event.u.class_event.clazz =
	    (*env)->NewLocalRef(env, clazz);

        CVMglobals.jvmdiStatics.eventHook(env, &event);

        (*env)->PopLocalFrame(env, 0);
    }
}

static void
reportFrameEvent(CVMExecEnv* ee, jint kind) 
{
    JVMDI_Event event;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMObjectICell* thread = CVMcurrentThreadICell(ee);
    CVMFrame* frame = CVMeeGetCurrentFrame(ee);
    CVMMethodBlock* mb = frame->mb;

    if (mb == NULL) {
        return;
    }

    if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
        return;
    }
    event.kind = kind;
    event.u.frame.thread = (*env)->NewLocalRef(env, thread);
    event.u.frame.clazz =
	(*env)->NewLocalRef(env, CVMcbJavaInstance(CVMmbClassBlock(mb)));
    event.u.frame.method = mb;
    /* NOTE: jframeID and CVMFrame* must be compatible */
    event.u.frame.frame = (jframeID)frame;

    CVMglobals.jvmdiStatics.eventHook(env, &event);

    (*env)->PopLocalFrame(env, 0);
}

void
CVMjvmdiNotifyDebuggerOfFramePop(CVMExecEnv* ee)
{
    CVMFrame* frame = CVMeeGetCurrentFrame(ee);
    CVMFrame* framePrev;

    /* %comment: k008 */

    if (frame == NULL) {
	return;
    }

    framePrev = CVMframePrev(frame);

    if (framePrev == NULL) {
	return;
    }

    /* NOTE:  This is needed by the code below, but wasn't
       present in the 1.2 sources */
    if (!CVMframeIsInterpreter(framePrev)) {
	return;
    }

    /* NOTE: the JDK has two slots for the return PC; one is the
       "lastpc" and one is the "returnpc". The JDK's version of the
       JVMDI inserts a "frame pop sentinel" as the returnpc in
       jvmdi_NotifyFramePop. The munging of the return PC is
       unnecessary and is merely an optimization to avoid performing a
       hash table lookup each time this function is called by the
       interpreter loop. Unfortunately, in CVM this optimization is
       also incorrect. We only have one return PC slot, and if an
       exception is thrown the fillInStackTrace code will not know
       that it may have to undo these sentinels, or how to do so.

       We could get around this by setting a bit in CVMFrame; since
       pointers on a 32-bit architecture are 4-byte aligned, we could
       use the bit just above the "special handling bit". However, for
       now, we're going to do the slow hashtable lookup each time. */
    {
        struct fpop *fp = NULL;
	CVMBool gotFramePop = CVM_FALSE;

        DEBUGGER_LOCK(ee);
	/* It seems that this gets called before JVMDI is initialized */
	if (CVMglobals.jvmdiStatics.framePops != NULL) {
	    fp = CVMbagFind(CVMglobals.jvmdiStatics.framePops, frame);
	}

	if (fp != NULL) {
	    /* Found a frame pop */
	    /* fp now points to randomness */
            CVMbagDelete(CVMglobals.jvmdiStatics.framePops, fp); 
	    gotFramePop = CVM_TRUE;
	}
        DEBUGGER_UNLOCK(ee);

	if (gotFramePop) {
	    if (!CVMexceptionOccurred(ee) &&
		MUST_NOTIFY(JVMDI_EVENT_FRAME_POP,
			    findThread(ee, CVMcurrentThreadICell(ee)))) {
                reportFrameEvent(ee, JVMDI_EVENT_FRAME_POP);
	    }
	}
    }

    if (!CVMexceptionOccurred(ee) && 
        MUST_NOTIFY(JVMDI_EVENT_METHOD_EXIT,
		    findThread(ee, CVMcurrentThreadICell(ee)))) {
        reportFrameEvent(ee, JVMDI_EVENT_METHOD_EXIT);
    }
}

void
CVMjvmdiNotifyDebuggerOfFramePush(CVMExecEnv* ee)
{
    if (MUST_NOTIFY(JVMDI_EVENT_METHOD_ENTRY,
		    findThread(ee, CVMcurrentThreadICell(ee)))) {
        reportFrameEvent(ee, JVMDI_EVENT_METHOD_ENTRY);
    }
}

/*
 * This function is called at the end of the VM initialization process.
 * It triggers the sending of an event to the JVMDI client. At this point
 * The JVMDI client is free to use all of JNI and JVMDI.
 */
void
CVMjvmdiNotifyDebuggerOfVmInit(CVMExecEnv* ee)
{    
    if (CVMglobals.jvmdiStatics.eventHook) {
        JVMDI_Event event;
	JNIEnv *env = CVMexecEnv2JniEnv(ee);

	if ((*env)->PushLocalFrame(env, 0+LOCAL_FRAME_SLOP) < 0) {
	    return;
        }
	event.kind = JVMDI_EVENT_VM_INIT;
	CVMglobals.jvmdiStatics.eventHook(env, &event);

	(*env)->PopLocalFrame(env, 0);
    }
}

void
CVMjvmdiNotifyDebuggerOfVmExit(CVMExecEnv* ee)
{    
    if (CVMglobals.jvmdiStatics.eventHook) {
        JVMDI_Event event;
	JNIEnv *env = CVMexecEnv2JniEnv(ee);

	event.kind = JVMDI_EVENT_VM_DEATH;

	if ((*env)->PushLocalFrame(env, 0+LOCAL_FRAME_SLOP) < 0) {
	    return;
        }
	CVMglobals.jvmdiStatics.eventHook(env, &event);

	(*env)->PopLocalFrame(env, 0);
    }
}

#ifdef JDK12

static void
handleExit(void)
{
    CVMjvmdiNotifyDebuggerOfVmExit(CVMgetEE());
}

#endif

/**
 * Return the underlying opcode at the specified address.
 * Notify of the breakpoint, if it is still there and 'notify' is true.
 * For use outside jvmdi.
 */
CVMUint8
CVMjvmdiGetBreakpointOpcode(CVMExecEnv* ee, CVMUint8 *pc, CVMBool notify)
{
    struct bkpt *bp;
    int rv;

    DEBUGGER_LOCK(ee);
    bp = CVMbagFind(CVMglobals.jvmdiStatics.breakpoints, pc);
    if (bp == NULL) {
	rv = *pc;  
    } else {
        rv = bp->opcode;
    }
    DEBUGGER_UNLOCK(ee);
    
    CVMassert(rv != opc_breakpoint);

    if (notify && bp != NULL) {
        notify_debugger_of_breakpoint(ee, pc);
    }
    return rv;
}

/* The opcode at the breakpoint has changed. For example, it
 * has been quickened. Update the opcode stored in the breakpoint table.
 */
CVMBool
CVMjvmdiSetBreakpointOpcode(CVMExecEnv* ee, CVMUint8 *pc, CVMUint8 opcode)
{
    struct bkpt *bp;

    CVMassert(DEBUGGER_IS_LOCKED(ee));
    bp = CVMbagFind(CVMglobals.jvmdiStatics.breakpoints, pc);
    CVMassert(bp != NULL);
    bp->opcode = opcode;
    return CVM_TRUE;
}

/* clean up breakpoint - doesn't actually remove it. 
 * lock must be held */
static CVMBool
clear_bkpt(JNIEnv *env, struct bkpt *bp)
{
    CVMassert(*(bp->pc) == opc_breakpoint);
    *(bp->pc) = (CVMUint8)(bp->opcode);
#ifdef CVM_HW
    CVMhwFlushCache(bp->pc, bp->pc + 1);
#endif
    
    /* 
     * De-reference the enclosing class so that it's GC 
     * is no longer prevented by this breakpoint. 
     */
    (*env)->DeleteGlobalRef(env, bp->classRef);
    return CVM_TRUE;
}

/* Exported JVMDI interface begins here */

#define NULL_CHECK(_ptr, _error) { if ((_ptr) == NULL) return (_error); }
#define NOT_NULL(ptr)   { if (ptr == NULL) return JVMDI_ERROR_NULL_POINTER; }
#define NOT_NULL2(ptr1,ptr2)   { if (ptr1 == NULL || ptr2 == NULL) \
                                      return JVMDI_ERROR_NULL_POINTER; }

#define ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(ptr, error_code) \
    { if (NULL == (ptr)) return (error_code); }

#define ASSERT_NOT_NULL2_ELSE_EXIT_WITH_ERROR(ptr1, ptr2, error_code) \
    { if (NULL == (ptr1) || NULL == (ptr2)) return (error_code); }

#define THREAD_OK(ee)  { if (ee == NULL) \
				     return JVMDI_ERROR_UNATTACHED_THREAD; }

#define ALLOC(size, where) { jvmdiError allocErr = \
 jvmdi_Allocate(size, (jbyte**)where); \
 if (allocErr != JVMDI_ERROR_NONE) return allocErr; }

#define ALLOC_WITH_CLEANUP_IF_FAILED(size, where, cleanup) {    \
    jvmdiError allocErr = jvmdi_Allocate(size, (jbyte**)where); \
    if (allocErr != JVMDI_ERROR_NONE) {                         \
        cleanup;                                                \
        return allocErr;                                        \
    }                                                           \
}


static CVMClassBlock *object2class(CVMExecEnv *ee, jclass clazz)
{
    CVMClassBlock *cb = NULL;

    CVMD_gcUnsafeExec(ee, {
	if (clazz != NULL) {
	    CVMClassBlock *ccb;
	    CVMObject* directObj = CVMID_icellDirect(ee, clazz);
	    ccb = CVMobjectGetClass(directObj);
	    if (ccb == CVMsystemClass(java_lang_Class)) {
		cb = CVMgcUnsafeClassRef2ClassBlock(ee, clazz);
	    }
	}
    });

    return cb;
}

#define VALID_CLASS(cb) { if ((cb)==NULL) \
          return JVMDI_ERROR_INVALID_CLASS; }

#define VALID_OBJECT(o) { if ((o)==NULL) \
          return JVMDI_ERROR_INVALID_OBJECT; }

#define DEBUG_ENABLED() { if (!CVMglobals.jvmdiDebuggingEnabled) return JVMDI_ERROR_ACCESS_DENIED; }

  /*
   *  Threads
   */

static CVMExecEnv*
jthreadToExecEnv(CVMExecEnv* ee, jthread thread)
{
    CVMJavaLong eetop;

    if ((thread == NULL) ||
	CVMID_icellIsNull(thread)) {
	return NULL;
    }

#if 0
{
    CVMBool currentThread;
    CVMID_icellSameObject(ee, thread, CVMcurrentThreadICell(ee), currentThread);
    if (currentThread) {
	return ee;
    }
}
#endif

    CVMID_fieldReadLong(ee, (CVMObjectICell*) thread,
			CVMoffsetOfjava_lang_Thread_eetop,
			eetop);
    return (CVMExecEnv*)CVMlong2VoidPtr(eetop);
}

/* Return via "threadsPtr" all threads in this VM
 * ("threadsCountPtr" returns the number of such threads).
 * Memory for this table is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Note: threads in the array are JNI global references and must
 * be explicitly managed.
 * Errors: JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetAllThreads(jint *threadsCountPtr, jthread **threadsPtr)
{
    jvmdiError rc = JVMDI_ERROR_NONE;
    CVMExecEnv* ee = CVMgetEE();
    CVMJavaLong sz;
    CVMInt32 i;

    DEBUG_ENABLED();
    NOT_NULL(threadsCountPtr);
    THREAD_OK(ee);

    /* NOTE: We must not trigger a GC while holding the thread lock.
       We are safe here because allocating global roots can cause
       expansion of the global root stack, but not a GC. */
    CVM_THREAD_LOCK(ee);

    /* count the threads */
    *threadsCountPtr = 0;
    CVM_WALK_ALL_THREADS(ee, currentEE, {
        CVMassert(CVMcurrentThreadICell(currentEE) != NULL);
        if (!CVMID_icellIsNull(CVMcurrentThreadICell(currentEE))) {
            (*threadsCountPtr)++;
        }
    });

    sz = CVMint2Long(*threadsCountPtr * sizeof(jthread));
    rc = jvmdi_Allocate(sz, (jbyte**)threadsPtr);
    if (rc == JVMDI_ERROR_NONE) {

        jthread *tempPtr = *threadsPtr;
	i = 0;
        /* fill in the threads */
	CVM_WALK_ALL_THREADS(ee, currentEE, {
            if (!CVMID_icellIsNull(CVMcurrentThreadICell(currentEE))) {
                tempPtr[i] = CVMID_getGlobalRoot(ee);
                if (tempPtr[i] == NULL) {
                    /* %comment: k010 */
                    rc = JVMDI_ERROR_OUT_OF_MEMORY;
                    break; /* %comment: k011 */
                }
                CVMID_icellAssign(ee, tempPtr[i],
                                  CVMcurrentThreadICell(currentEE));
                i++;
            }
	});
    }

    CVM_THREAD_UNLOCK(ee);
    return rc;
}


/* Return via "threadStatusPtr" the current status of the thread and 
 * via "suspendStatusPtr" information on suspension. If the thread
 * has been suspended, bits in the suspendStatus will be set, and the 
 * thread status indicates the pre-suspend status of the thread.
 * JVMDI_THREAD_STATUS_* flags are used to identify thread and 
 * suspend status.
 * Errors: JVMDI_ERROR_INVALID_THREAD, JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetThreadStatus(jthread threadObj,
                      jint *threadStatusPtr, jint *suspendStatusPtr)
{
    /* %comment: k029 */
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv* targetEE;
    /* NOTE:  Added this lock. This used to be a SYSTHREAD call,
       which I believe is unsafe unless you have the thread queue
       lock, because the thread might exit after you get a handle to
       its EE. (Must file bug) */
    CVM_THREAD_LOCK(ee);
    /* %comment: k035 */
    targetEE = jthreadToExecEnv(ee, threadObj);
    if (targetEE == NULL) {
	*threadStatusPtr = JVMDI_THREAD_STATUS_ZOMBIE;
	*suspendStatusPtr = 0;
    } else {
	*threadStatusPtr = JVMDI_THREAD_STATUS_RUNNING;
	*suspendStatusPtr = 
	    (targetEE->threadState & CVM_THREAD_SUSPENDED) ?
	    JVMDI_SUSPEND_STATUS_SUSPENDED : 0;
    }
    CVM_THREAD_UNLOCK(ee);
    return JVMDI_ERROR_NONE;

}

/* Suspend the specified thread.
 * Errors: JVMDI_ERROR_INVALID_THREAD, JVMDI_ERROR_THREAD_SUSPENDED,
 * JVMDI_ERROR_VM_DEAD 
 */
static jvmdiError JNICALL
jvmdi_SuspendThread(jthread threadObj)
{
    jvmdiError rc = JVMDI_ERROR_NONE;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv* env;
    /* NOTE that this looks unsafe. However, later code will get the
       EE out of the Java thread object again, this time under
       protection of the thread lock, before doing any real work with
       the EE. */
    CVMExecEnv* target = jthreadToExecEnv(ee, threadObj);

    DEBUG_ENABLED();
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    if (target == ee) {
        /*
         * Suspending self. Don't need to check for previous
         * suspension.  Shouldn't grab any locks here. (NOTE: JDK's
         * JVM_SuspendThread recomputes whether it's suspending itself
         * down in JDK's threads.c)
         */
        JVM_SuspendThread(env, (jobject)threadObj);
        rc = JVMDI_ERROR_NONE;
    } else {
#if 0
	CVMconsolePrintf("jvmdi_SuspendThread: suspending thread \"");
	CVMprintThreadName(env, threadObj);
	CVMconsolePrintf("\"\n");
#endif

        /*
         * Suspending another thread. If it's already suspended 
         * report an error.
         */

	/*
	 * NOTE: this relies on the fact that the system mutexes
	 * grabbed by CVMlocksForThreadSuspendAcquire are
	 * reentrant. See jvm.c, JVM_SuspendThread, which makes a
	 * (redundant) call to the same routine.
	 *
	 * NOTE: We really probably only need the threadLock, but since
	 * we have to hold onto it until after JVM_SuspendThread() is
	 * done, and since JVM_SuspendThread() grabs all the locks,
	 * we need to make sure we at least grab all locks with a lower
	 * rank than threadLock in order to avoid a rank error. Calling
	 * CVMlocksForThreadSuspendAcquire() is the easiest way to do this.
	 */
	CVMlocksForThreadSuspendAcquire(ee);

        /*
         * Now that we're locked down, re-fetch the sys thread so that 
         * we can be sure it hasn't gone away.
         */
	target = jthreadToExecEnv(ee, threadObj);
        if (target == NULL) {
            /*
             * It has finished running and is a zombie
             */
            rc = JVMDI_ERROR_INVALID_THREAD;
        } else {
	    if (target->threadState & CVM_THREAD_SUSPENDED) {
                rc = JVMDI_ERROR_THREAD_SUSPENDED;
            } else {
                JVM_SuspendThread(env, (jobject)threadObj);
                rc = JVMDI_ERROR_NONE;
            }
        }
	CVMlocksForThreadSuspendRelease(ee);
    }
    return rc;
}

/* Resume the specified thread.
 * Errors: JVMDI_ERROR_INVALID_THREAD, JVMDI_ERROR_THREAD_NOT_SUSPENDED, 
 * JVMDI_ERROR_INVALID_TYPESTATE,
 * JVMDI_ERROR_VM_DEAD
 */
static jvmdiError JNICALL
jvmdi_ResumeThread(jthread threadObj)
{
    jvmdiError rc = JVMDI_ERROR_THREAD_NOT_SUSPENDED;
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv* target;
    JNIEnv* env;

    DEBUG_ENABLED();
    
    env = CVMexecEnv2JniEnv(ee);

    CVM_THREAD_LOCK(ee);

    target = jthreadToExecEnv(ee, threadObj);
    if (target != NULL) {
	if (target->threadState & CVM_THREAD_SUSPENDED) {
	    JVM_ResumeThread(env, (jobject) threadObj);
	    rc = JVMDI_ERROR_NONE;
	}
    }

    CVM_THREAD_UNLOCK(ee);

    return rc;

#if 0
    /* JDK 1.2 code */
    jvmdiError rc;
    int sysState;
    Hjava_lang_Thread *thread;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    

    DEBUG_ENABLED();
    THREAD_OK(ee);
    QUEUE_LOCK(ee);

    env = CVMexecEnv2JniEnv(ee);

    thread = (Hjava_lang_Thread *)DeRef(env, threadObj);

    sysState = sysThreadGetStatus(SYSTHREAD(thread), NULL);
    if (sysState & SYS_THREAD_SUSPENDED) {
        JVM_ResumeThread(env, (jobject)threadObj);
        rc = JVMDI_ERROR_NONE;
    } else {
        rc = JVMDI_ERROR_THREAD_NOT_SUSPENDED;
    }
    QUEUE_UNLOCK(ee);
    return rc;
#endif
}  

/* 
 * Kill the specified thread - generate the specified exception.
 * For the default stop() pass in an instance of ThreadDeath. 
 * JVMDI equivalent of Thread.stop().
 * Errors: JVMDI_ERROR_INVALID_THREAD
 */
static jvmdiError JNICALL
jvmdi_StopThread(jthread thread, jobject exception) {
    /* %comment: k031 */

    return JVMDI_ERROR_NONE;

}  

/* 
 * Interrupt the specified thread. 
 * JVMDI equivalent of Thread.interrupt().
 * Errors: JVMDI_ERROR_INVALID_THREAD
 */
static jvmdiError JNICALL
jvmdi_InterruptThread(jthread thread) {
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    

    DEBUG_ENABLED();
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    JVM_Interrupt(env, thread);
    return JVMDI_ERROR_NONE;
}  

/* %comment: k033 */


static jfieldID 
getFieldID(JNIEnv *env, jclass clazz, char *name, char *type)
{
    jfieldID id = (*env)->GetFieldID(env, clazz, name, type);
    jthrowable exc = (*env)->ExceptionOccurred(env);
    if (exc) {
            (*env)->ExceptionDescribe(env);
            (*env)->FatalError(env, "internal error in JVMDI");
    }
    return id;
}

/* count UTF length of Unicode chars */
static int 
lengthCharsUTF(jint uniLen, jchar *uniChars)  
{
    int i;
    int utfLen = 0;
    jchar *uniP = uniChars;

    for (i = 0 ; i < uniLen ; i++) {
        int c = *uniP++;
        if ((c >= 0x0001) && (c <= 0x007F)) {
            utfLen++;
        } else if (c > 0x07FF) {
            utfLen += 3;
        } else {
            utfLen += 2;
        }
    }
    return utfLen;
}

/* convert Unicode to UTF */
static void 
setBytesCharsUTF(jint uniLen, jchar *uniChars, char *utfBytes)  
{
    int i;
    jchar *uniP = uniChars;
    char *utfP = utfBytes;

    for (i = 0 ; i < uniLen ; i++) {
        int c = *uniP++;
        if ((c >= 0x0001) && (c <= 0x007F)) {
            *utfP++ = c;
        } else if (c > 0x07FF) {
            *utfP++ = (0xE0 | ((c >> 12) & 0x0F));
            *utfP++ = (0x80 | ((c >>  6) & 0x3F));
            *utfP++ = (0x80 | ((c >>  0) & 0x3F));
        } else {
            *utfP++ = (0xC0 | ((c >>  6) & 0x1F));
            *utfP++ = (0x80 | ((c >>  0) & 0x3F));
        }
    }
    *utfP++ = 0; /* terminating zero */
}


/* Return via "infoPtr" thread info.
 * Memory for the name string is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Note: returned objects are allocated globally and must be explictly
 * freed with DeleteGlobalRef.
 * Errors: JVMDI_ERROR_INVALID_THREAD, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetThreadInfo(jthread thread, JVMDI_thread_info *infoPtr) 
{
    /* %comment: k034 */

    static jfieldID nameID = 0;
    static jfieldID priorityID;
    static jfieldID daemonID;
    static jfieldID groupID;
    static jfieldID loaderID;
    jcharArray nmObj;
    jobject groupObj;
    jobject loaderObj;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    
    
    DEBUG_ENABLED();
    NOT_NULL(infoPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);
    if (nameID == 0) {
#if 0
	/* This doesn't work for subclasses of Thread, because
	   we are asking for private fields */
        jclass threadClass = (*env)->GetObjectClass(env, thread);
#else
	jclass threadClass =
	    CVMcbJavaInstance(CVMsystemClass(java_lang_Thread));
#endif
        nameID = getFieldID(env, threadClass, "name", "[C");
        priorityID = getFieldID(env, threadClass, "priority", "I");
        daemonID = getFieldID(env, threadClass, "daemon", "Z");
        groupID = getFieldID(env, threadClass, "group", 
                             "Ljava/lang/ThreadGroup;");
        loaderID = getFieldID(env, threadClass, "contextClassLoader", 
                              "Ljava/lang/ClassLoader;");
    }
    /* NOTE that when we use the JNI we don't want our accesses
       recorded, so we bypass the instrumented JNI vector and call the
       implementation directly. */
    nmObj = (jcharArray)(CVMjniGetObjectField(env, thread, nameID));
    infoPtr->priority = CVMjniGetIntField(env, thread, priorityID);
    infoPtr->is_daemon = CVMjniGetBooleanField(env, thread, daemonID);
    groupObj = CVMjniGetObjectField(env, thread, groupID);
    infoPtr->thread_group = groupObj == NULL ? NULL :
                    (jthreadGroup)((*env)->NewGlobalRef(env, groupObj));
    loaderObj = CVMjniGetObjectField(env, thread, loaderID);
    infoPtr->context_class_loader = loaderObj == NULL ? NULL :
                    (*env)->NewGlobalRef(env, loaderObj);

    {   /* copy the thread name */
        jint uniLen = (*env)->GetArrayLength(env, nmObj);
        jchar *uniChars = (*env)->GetCharArrayElements(env, nmObj, NULL);
        jint utfLen = lengthCharsUTF(uniLen, uniChars);
	CVMJavaLong sz = CVMint2Long(utfLen+1);
        ALLOC(sz, &(infoPtr->name));
        setBytesCharsUTF(uniLen, uniChars, infoPtr->name);
        (*env)->ReleaseCharArrayElements(env, nmObj, uniChars, JNI_ABORT);
    }
   
    return JVMDI_ERROR_NONE;
}
    
/* NOTE: We (will) implement fast locking, so we don't implement
   GetOwnedMonitorInfo or GetCurrentContendedMonitor. */

#ifndef FAST_MONITOR
# define FAST_MONITOR
#endif

/*-begin-#ifdef FAST_MONITOR-------------------------------------------------*/
#ifndef FAST_MONITOR

typedef struct {
    JNIEnv *env;
    jvmdiError error;
    sys_thread_t *sysThread;
    int count;
    jobject *monIter;
} MonitorEnumArg;

static void 
deleteRefArray(JNIEnv *env, jobject *objects, int count) 
{
    int i;
    for (i = 0; i < count; i++) {
        (*env)->DeleteGlobalRef(env, objects[i]);
    }
}

static void 
ownedMonitorCountHelper(monitor_t *monitor, void *arg)
{
    MonitorEnumArg *monArg = arg;
    monArg->error = JVMDI_ERROR_NONE;

    /*
     * Count the monitor as owned if its owner thread is the
     * right one and if it is a Java monitor.
     */
    if ((sysMonitorOwner(sysmon(monitor)) == monArg->sysThread) &&
        (lookupJavaMonitor(sysmon(monitor)) != NULL)) {
        monArg->count++;
    }
}

static void 
ownedMonitorHelper(monitor_t *monitor, void *arg) 
{
    /* %comment: k032 */
    MonitorEnumArg *monArg = arg;
    monArg->error = JVMDI_ERROR_NONE;

    if (sysMonitorOwner(sysmon(monitor)) == monArg->sysThread) {
        CVMObjectICell* handle = lookupJavaMonitor(sysmon(monitor));
        if (handle) {
            jobject object;
            JNIEnv *env = monArg->env;
    
            if ((*env)->PushLocalFrame(env, 1) < 0) {
                monArg->error = JVMDI_ERROR_OUT_OF_MEMORY;
                return;
            }
            object = MkRefLocal(env, handle);
            *monArg->monIter = (jobject)((*env)->NewGlobalRef(env, object));
            (*env)->PopLocalFrame(env, 0);
    
            if (*monArg->monIter == 0) {
                monArg->error = JVMDI_ERROR_OUT_OF_MEMORY;
                return;
            }
            monArg->count++;
            monArg->monIter++;
        }
    }
}

#endif /* FAST_MONITOR */
/*-end-#ifdef FAST_MONITOR---------------------------------------------------*/
  
/* Return via "infoPtr" monitor ownership info for the given thread.
 * The thread must be suspended before calling this function.
 * Memory for the owned monitor array is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Note: returned objects are allocated globally and must be explictly
 * freed with DeleteGlobalRef.
 * Errors: JVMDI_ERROR_INVALID_THREAD, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OUT_OF_MEMORY, JVMDI_ERROR_THREAD_NOT_SUSPENDED
 */
static jvmdiError JNICALL
jvmdi_GetOwnedMonitorInfo(jthread threadObj, JVMDI_owned_monitor_info *infoPtr) 
{
#ifdef FAST_MONITOR
    return JVMDI_ERROR_NOT_IMPLEMENTED;
#else
    jvmdiError rc = JVMDI_ERROR_NONE;
    int error;
    MonitorEnumArg arg;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    
    Hjava_lang_Thread *thread;

    DEBUG_ENABLED();
    THREAD_OK(ee);
    NOT_NULL(infoPtr);

    ee = SysThread2EE(self);
    env = CVMexecEnv2JniEnv(ee);
    thread  = (Hjava_lang_Thread *)DeRef(env, threadObj);

    /* count the owned monitors */
    arg.env = env;
    arg.sysThread = SYSTHREAD(thread);
    arg.count = 0;

    CACHE_LOCK(self);
    monitorEnumerate(ownedMonitorCountHelper, &arg);
    CACHE_UNLOCK(self);

    infoPtr->owned_monitor_count = arg.count;
    if (arg.count == 0) {
        infoPtr->owned_monitors = NULL;
    } else {
	CVMJavaLong sz = CVMint2Long(arg.count * sizeof(jobject));
        ALLOC(sz, &infoPtr->owned_monitors);
    
        /* fill in the owned monitors */
        arg.count = 0;
        arg.monIter = infoPtr->owned_monitors;
        CACHE_LOCK(self);
        monitorEnumerate(ownedMonitorHelper, &arg);
        CACHE_UNLOCK(self);
    
        /*
         * If the counts don't match, something bad happened. Either
         * there was an error in the helper or threads are running and have
         * changed monitor states. In either case an error should be returned here.
         */
        if (infoPtr->owned_monitor_count != arg.count) {
            deleteRefArray(env, infoPtr->owned_monitors, arg.count);
            jvmdi_Deallocate((jbyte *)infoPtr->owned_monitors);
            if (arg.error != JVMDI_ERROR_NONE) {
                return JVMDI_ERROR_OUT_OF_MEMORY;
            } else {
                /* Monitors changed ==> threads were running */
                return JVMDI_ERROR_THREAD_NOT_SUSPENDED;
            }
        } 
    }

    return JVMDI_ERROR_NONE;
#endif /* FAST_MONITOR */
}

static jvmdiError JNICALL
jvmdi_GetCurrentContendedMonitor(jthread threadObj, jobject *monitorPtr) 
{
#ifdef FAST_MONITOR
    return JVMDI_ERROR_NOT_IMPLEMENTED;
#else
    Hjava_lang_Thread *thread;
    int sysState;
    sys_mon_t *sysMon;
    CVMExecEnv *ee = CVMgetEE();
    JNIEnv *env;
    CVMObjectICell* handle;
    jobject monitor;

    DEBUG_ENABLED();
    NOT_NULL(monitorPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);
    thread  = (Hjava_lang_Thread *)DeRef(CVMexecEnv2JniEnv(EE()), threadObj);

    if (thread == 0) {
        return JVMDI_ERROR_INVALID_THREAD;
    } 
    
    if (SYSTHREAD(thread) == 0) {
        /* Zombies are not waiting on monitors */
        *monitorPtr = NULL;
        return JVMDI_ERROR_NONE;
    }
     
    QUEUE_LOCK(self);   /* Prevent anyone from resuming in the middle */
    sysState = sysThreadGetStatus(SYSTHREAD(thread), &sysMon);
    QUEUE_UNLOCK(self);
    if ((sysState & SYS_THREAD_SUSPENDED) == 0) {
        return JVMDI_ERROR_THREAD_NOT_SUSPENDED;
    }

    if (sysMon == NULL) {
        /* Not waiting on any monitor */
        *monitorPtr = NULL;
        return JVMDI_ERROR_NONE;
    }

    /*
     * Attempt to find the java object associated with the monitor 
     */
    handle = lookupJavaMonitor(sysMon);
    if (handle == NULL) {
        /* Waiting on an internal monitor of some kind */
        *monitorPtr = NULL;
        return JVMDI_ERROR_NONE;
    }

    if ((*env)->PushLocalFrame(env, 1) < 0) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    monitor = MkRefLocal(env, handle);
    monitor = (*env)->NewGlobalRef(env, monitor);
    (*env)->PopLocalFrame(env, 0);
    if (monitor == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    *monitorPtr = monitor;
    return JVMDI_ERROR_NONE;
#endif /* FAST_MONITOR */
}



  /*
   *  Thread Groups
   */

/* Return in 'groups' an array of top-level thread groups (parentless 
 * thread groups).
 * Note: returned objects are allocated globally and must be explictly
 * freed with DeleteGlobalRef.
 * Errors: JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetTopThreadGroups(jint *groupCountPtr, 
                         jthreadGroup **groupsPtr)
{
    jobject sysgrp;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    
    CVMJavaLong sz;

    DEBUG_ENABLED();
    NOT_NULL2(groupCountPtr, groupsPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    /* We only have the one top group */
    sz = CVMint2Long(sizeof(jthreadGroup));
    ALLOC(sz, groupsPtr);
    *groupCountPtr = 1;

    /* Obtain the system ThreadGroup object from a static field of 
     * Thread class */
    if ((*env)->PushLocalFrame(env, 1) < 0) {
	return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    {
	jclass threadClass =
	    CVMcbJavaInstance(CVMsystemClass(java_lang_Thread));
	jobject systemThreadGroup = 
	    CVMjniGetStaticObjectField(env, threadClass,
		CVMjniGetStaticFieldID(env, threadClass,
		    "systemThreadGroup", "Ljava/lang/ThreadGroup;"));

	CVMassert(systemThreadGroup != NULL);

	sysgrp = (*env)->NewGlobalRef(env, systemThreadGroup);
    }

    if (sysgrp == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    **groupsPtr = (jthreadGroup) sysgrp;
   
    return JVMDI_ERROR_NONE;
}

/* Return via "infoPtr" thread group info.
 * Memory for the name string is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Note: returned objects are allocated globally and must be explictly
 * freed with DeleteGlobalRef.
 * Errors: JVMDI_ERROR_INVALID_THREAD_GROUP, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetThreadGroupInfo(jthreadGroup group, 
                         JVMDI_thread_group_info *infoPtr) 
{
    static jfieldID parentID;
    static jfieldID nameID = 0;
    static jfieldID maxPriorityID;
    static jfieldID daemonID;
    jstring nmString;
    jobject parentObj;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    
    
    DEBUG_ENABLED();
    NOT_NULL(infoPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);
    /* Needed because CVMjniGetObjectField allocates local refs
       internally */
    if ((*env)->PushLocalFrame(env, 5) < 0) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    if (nameID == 0) {
#if 0
	/* This doesn't work for subclasses of ThreadGroup, because
	   we are asking for private fields */
        jclass tgClass = (*env)->GetObjectClass(env, group);
#else
	jclass tgClass =
	    CVMcbJavaInstance(CVMsystemClass(java_lang_ThreadGroup));
#endif
        parentID = getFieldID(env, tgClass, "parent", 
                             "Ljava/lang/ThreadGroup;");
        nameID = getFieldID(env, tgClass, "name", 
                            "Ljava/lang/String;");
        maxPriorityID = getFieldID(env, tgClass, "maxPriority", "I");
        daemonID = getFieldID(env, tgClass, "daemon", "Z");
    }
    /* NOTE that when we use the JNI we don't want our accesses
       recorded, so we bypass the instrumented JNI vector and call the
       implementation directly. */
    parentObj = CVMjniGetObjectField(env, group, parentID);
    infoPtr->parent = parentObj == NULL? NULL : 
                   (jthreadGroup)((*env)->NewGlobalRef(env, parentObj));
    nmString = (jstring)(CVMjniGetObjectField(env, group, nameID));
    infoPtr->max_priority = CVMjniGetIntField(env, group, 
                                                      maxPriorityID);
    infoPtr->is_daemon = CVMjniGetBooleanField(env, group, daemonID);

    {   /* copy the thread group name */
        jint nmLen = (*env)->GetStringUTFLength(env, nmString);
        const char *nmValue = (*env)->GetStringUTFChars(env, 
                                                         nmString, NULL);
	CVMJavaLong sz = CVMint2Long(nmLen+1);
        ALLOC(sz, &(infoPtr->name));
        strcpy(infoPtr->name, nmValue);
        (*env)->ReleaseStringUTFChars(env, nmString, nmValue);
    }
   
    (*env)->PopLocalFrame(env, 0);
    return JVMDI_ERROR_NONE;
}
    
static jvmdiError 
objectArrayToArrayOfObject(JNIEnv *env, jint cnt, jobject **destPtr,
                           jobjectArray arr) {
    jvmdiError rc;
    CVMJavaLong sz = CVMint2Long(cnt * sizeof(jobject));

    /* allocate the destination array */
    rc = jvmdi_Allocate(sz, (jbyte**)destPtr);

    /* fill-in the destination array */
    if (rc == JVMDI_ERROR_NONE) {
        int inx;
        jobject *rp = *destPtr;
        for (inx = 0; inx < cnt; inx++) {
            jobject obj = (*env)->NewGlobalRef(env, 
                          (*env)->GetObjectArrayElement(env, arr, inx));
            if (obj == NULL) {
                rc = JVMDI_ERROR_OUT_OF_MEMORY;
                break;
            }
            *rp++ = obj;
        }

        /* clean up partial array after any error */
        if (rc != JVMDI_ERROR_NONE) {
            for (rp--; rp >= *destPtr; rp--) {
                (*env)->DeleteGlobalRef(env, *rp);
            }
            jvmdi_Deallocate((jbyte *)*destPtr);
	    *destPtr = NULL;
        }            
    }
    return rc;
}

/* Return via "threadCountPtr" the number of threads in this thread group.
 * Return via "threadsPtr" the threads in this thread group.
 * Return via "groupCountPtr" the number of thread groups in this thread group.
 * Return via "groupsPtr" the thread groups in this thread group.
 * Note: returned objects are allocated globally and must be explictly
 * freed with DeleteGlobalRef.
 * Errors: JVMDI_ERROR_INVALID_THREAD_GROUP, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetThreadGroupChildren(jthreadGroup group, 
                             jint *threadCountPtr, jthread **threadsPtr,
                             jint *groupCountPtr, jthreadGroup **groupsPtr) {
    /* %comment: k034 */

    static jfieldID nthreadsID = 0;
    static jfieldID threadsID;
    static jfieldID ngroupsID;
    static jfieldID groupsID;
    jvmdiError rc;
    jint nthreads;
    jobject threads;
    jint ngroups;
    jobject groups;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    
    
    DEBUG_ENABLED();
    NOT_NULL2(threadCountPtr, threadsPtr);
    NOT_NULL2(groupCountPtr, groupsPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);
    /* Needed because CVMjniGetObjectField allocates local refs
       internally */
    if ((*env)->PushLocalFrame(env, 5) < 0) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    if (nthreadsID == 0) {
#if 0
	/* This doesn't work for subclasses of ThreadGroup, because
	   we are asking for private fields */
        jclass tgClass = (*env)->GetObjectClass(env, group);
#else
	jclass tgClass =
	    CVMcbJavaInstance(CVMsystemClass(java_lang_ThreadGroup));
#endif
        nthreadsID = getFieldID(env, tgClass, "nthreads", "I");
        threadsID = getFieldID(env, tgClass, "threads", 
                             "[Ljava/lang/Thread;");
        ngroupsID = getFieldID(env, tgClass, "ngroups", "I");
        groupsID = getFieldID(env, tgClass, "groups", 
                             "[Ljava/lang/ThreadGroup;");
    }
    /* NOTE that when we use the JNI we don't want our accesses
       recorded, so we bypass the instrumented JNI vector and call the
       implementation directly. */
    nthreads = CVMjniGetIntField(env, group, nthreadsID);
    threads = CVMjniGetObjectField(env, group, threadsID);
    ngroups = CVMjniGetIntField(env, group, ngroupsID);
    groups = CVMjniGetObjectField(env, group, groupsID);

    rc = objectArrayToArrayOfObject(env, nthreads, threadsPtr, threads);
    if (rc == JVMDI_ERROR_NONE) {
        rc = objectArrayToArrayOfObject(env, ngroups, groupsPtr, groups);

        /* deallocate all of threads list on error */
        if (rc != JVMDI_ERROR_NONE) {
            int inx;
            jthread *rp = *threadsPtr;
            for (inx = 0; inx < nthreads; inx++) {
                (*env)->DeleteGlobalRef(env, *rp++); 
            }
            jvmdi_Deallocate((jbyte *)*threadsPtr);
        } else {
            *threadCountPtr = nthreads;
            *groupCountPtr = ngroups;
        }
    }
    (*env)->PopLocalFrame(env, 0);
    return rc;
}

  /*
   *  Stack access
   */

/* Return via "countPtr" the number of frames in the thread's call stack.
 * Errors: JVMDI_ERROR_INVALID_THREAD,
 * JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetFrameCount(jthread thread, jint* countPtr) 
{
    /* NOTE that we do not seize the thread lock, because this
       function requires that the target thead be suspended, which
       means it can't exit out from under us once we've gotten a
       pointer to its EE. Once we can query a thread's status, we can
       put in an assert. */
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv* thread_ee;
    CVMFrame* frame;
    jint count = 0;

    DEBUG_ENABLED();

    thread_ee = jthreadToExecEnv(ee, thread);

    if (thread_ee == 0) {
	return JVMDI_ERROR_INVALID_THREAD;
    }

    CVMassert(thread_ee->threadState & CVM_THREAD_SUSPENDED);
    
    NOT_NULL(countPtr);

    frame = CVMeeGetCurrentFrame(thread_ee);
    while (frame != NULL) {
        /* skip pseudo frames */
        if (frame->mb != 0) {
            count++;
	}
        frame = CVMframePrev(frame);
    }

    *countPtr = count;

    return JVMDI_ERROR_NONE;
}    

/* Return via "framePtr" the frame ID for the current stack frame 
 * of this thread.  Thread must be suspended. 
 * Errors: JVMDI_ERROR_NO_MORE_FRAMES, JVMDI_ERROR_INVALID_THREAD,
 * JVMDI_ERROR_THREAD_NOT_SUSPENDED, JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetCurrentFrame(jthread thread, jframeID *framePtr) 
{
    /* NOTE that we do not seize the thread lock, because this
       function requires that the target thead be suspended, which
       means it can't exit out from under us once we've gotten a
       pointer to its EE. Once we can query a thread's status, we can
       put in an assert. */
    CVMExecEnv* thread_ee;
    CVMFrame* frame;

    DEBUG_ENABLED();

    thread_ee = jthreadToExecEnv(CVMgetEE(), thread);

    if (thread_ee == NULL) {
	/* Thread hasn't any state yet. */
	return JVMDI_ERROR_INVALID_THREAD;
    }
    NOT_NULL(framePtr);
    /* TO DO: check for not suspended */
    frame = CVMeeGetCurrentFrame(thread_ee);

    frame = CVMgetCallerFrameSpecial(frame, 0, CVM_FALSE);

    if (frame == NULL) {
        return JVMDI_ERROR_NO_MORE_FRAMES;
    }

    *framePtr = (jframeID)frame;
    return JVMDI_ERROR_NONE;
}

/* Return via "framePtr" the frame ID for the stack frame that called 
 * the specified frame.
 * Errors: JVMDI_ERROR_NO_MORE_FRAMES, JVMDI_ERROR_INVALID_FRAMEID,
 * JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetCallerFrame(jframeID called, jframeID *framePtr) 
{
    CVMFrame* frame = (CVMFrame*) called;

    DEBUG_ENABLED();
    NOT_NULL(framePtr);
    if (frame == NULL) {
        return JVMDI_ERROR_INVALID_FRAMEID;
    }

    frame = CVMgetCallerFrameSpecial(frame, 1, CVM_FALSE);

    if (frame == NULL) {
	return JVMDI_ERROR_NO_MORE_FRAMES;
    }

    *framePtr = (jframeID)frame;
    return JVMDI_ERROR_NONE;
}

/* Return via "classPtr" and "methodPtr" the active method in the 
 * specified frame. Return via "locationPtr" the index of the
 * currently executing instruction within the specified frame,
 * return negative one if location not available.
 * Note: class returned via 'classPtr' is allocated locally and must
 * be explicitly managed.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_NULL_POINTER
 * JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetFrameLocation(jframeID jframe,
                       jclass *classPtr, jmethodID *methodPtr,
                       jlocation *locationPtr)
{
    CVMFrame* frame = (CVMFrame*) jframe;
    CVMMethodBlock* mb = frame->mb;
    jobject clazz;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    

    DEBUG_ENABLED();
    NOT_NULL2(classPtr, methodPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    if (mb == NULL) {
        return JVMDI_ERROR_INVALID_FRAMEID;
    }
    if ((*env)->PushLocalFrame(env, 1) < 0) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }

    clazz = (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMmbClassBlock(mb)));
    *classPtr = (jclass)((*env)->NewGlobalRef(env, clazz));
    (*env)->PopLocalFrame(env, 0);
    *methodPtr = (jmethodID)mb;

    /* The jmd for <clinit> is freed after it is run. Can't take
       any chances. */
    if (CVMmbIs(mb, NATIVE) ||
	(CVMmbJmd(mb) == NULL)) {
        *locationPtr = CVMint2Long(-1);
    } else {
        *locationPtr = CVMint2Long(CVMframePc(frame) - CVMmbJavaCode(mb));
    }
    return JVMDI_ERROR_NONE;
}  			    

/* Send a JVMDI_EVENT_FRAME_POP event when the specified frame is
 * popped from the stack.  The event is sent after the pop has occurred.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_NATIVE_FRAME
 */
static jvmdiError JNICALL
jvmdi_NotifyFramePop(jframeID jframe)
{
    CVMFrame* frame = (CVMFrame*) jframe;
    CVMFrame* framePrev;
    CVMMethodBlock* mb = frame->mb;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();

    DEBUG_ENABLED();
    THREAD_OK(ee);

    if (mb == NULL) {
        return JVMDI_ERROR_INVALID_FRAMEID;
    }
    if (CVMmbIs(mb, NATIVE)) {
        return JVMDI_ERROR_OPAQUE_FRAME;
    }
    if (!CVMframeIsInterpreter(frame)) {
	return JVMDI_ERROR_OPAQUE_FRAME;
    }

    /*
     * Frame pop notification operates on the calling frame, so 
     * make sure it exists.
     */
    framePrev = CVMframePrev(frame);
    if (framePrev == NULL) {
        /* First frame (no previous) must be opaque */
        err = JVMDI_ERROR_OPAQUE_FRAME;
    } else {
	/* This is needed by the code below, but wasn't
           present in the 1.2 sources */
	if (!CVMframeIsInterpreter(framePrev)) {
	    return JVMDI_ERROR_OPAQUE_FRAME;
	}

        DEBUGGER_LOCK(ee);
        /* check if one already exists */
        if (CVMbagFind(CVMglobals.jvmdiStatics.framePops, 
	    frame) != NULL) {    
            err = JVMDI_ERROR_DUPLICATE;
        } else {
            /* allocate space for it */
            struct fpop *fp = CVMbagAdd(CVMglobals.jvmdiStatics.framePops);                     
            if (fp == NULL) {
                err = JVMDI_ERROR_OUT_OF_MEMORY;
            } else {
		/* Note that "frame pop sentinel" (i.e., returnpc
                   mangling) code removed in CVM. The JDK can do this
                   because it has two returnpc slots per frame. See
                   CVMjvmdiNotifyDebuggerOfFramePop for possible
                   optimization/solution. */
                fp->frame = frame;
                err = JVMDI_ERROR_NONE;
            }
        }
        DEBUGGER_UNLOCK(ee);          
    }

    return err;
}


  /*
   *  Local variables
   */

/* Return via "siPtr" the stack item at the specified slot.  Errors:
 * JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_INVALID_SLOT,
 * JVMDI_ERROR_TYPE_MISMATCH, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OPAQUE_FRAME. Must be called from GC-unsafe code and is
 * guaranteed not to become GC-safe internally. (Stack pointers are
 * invalid across GC-safe points.)
 */
static jvmdiError
jvmdiGCUnsafeGetSlot(jframeID jframe, jint slot, CVMJavaVal32 **siPtr)
{
    CVMFrame* frame = (CVMFrame*) jframe;
    CVMSlotVal32* vars = NULL;
    CVMMethodBlock* mb;

    if (!CVMframeIsInterpreter(frame)) {
        return JVMDI_ERROR_OPAQUE_FRAME;
    }
    mb = frame->mb;
    vars = CVMframeLocals(frame);
    if (mb == NULL || vars == NULL) {
        return JVMDI_ERROR_INVALID_FRAMEID;
    }
    if (CVMmbIs(mb, NATIVE)) {
        return JVMDI_ERROR_OPAQUE_FRAME;
    }
    if (slot >= CVMmbMaxLocals(mb)) {
        return JVMDI_ERROR_INVALID_SLOT;
    }
    *siPtr = &vars[slot].j;
    return JVMDI_ERROR_NONE;
}

/* Return via "valuePtr" the value of the local variable at the
 * specified slot.
 * Note: returned objects are allocated globally and must be explictly
 * freed with DeleteGlobalRef.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_INVALID_SLOT,
 * JVMDI_ERROR_TYPE_MISMATCH, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OPAQUE_FRAME, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetLocalObject(jframeID frame, jint slot, 
		     jobject *valuePtr)
{
    CVMJavaVal32* si;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    

    DEBUG_ENABLED();
    NOT_NULL(valuePtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    CVMID_localrootBegin(ee); {
	CVMID_localrootDeclare(CVMObjectICell, obj);

	CVMD_gcUnsafeExec(ee, {
	    err = jvmdiGCUnsafeGetSlot(frame, slot, &si);
	    if (err == JVMDI_ERROR_NONE) {
		CVMID_icellAssignDirect(ee, obj, &si->r);
	    }
	});
	if (err == JVMDI_ERROR_NONE) {
	    /* NOTE: at this point, "obj" could contain a null direct
               object. We must not allow these to "escape" to the rest
               of the system. */
	    /* %comment: k012 */
	    if (!CVMID_icellIsNull(obj)) {
		*valuePtr = (*env)->NewGlobalRef(env, obj);
		if (*valuePtr == NULL) {
		    err = JVMDI_ERROR_OUT_OF_MEMORY;
		}
	    } else {
		*valuePtr = NULL;
	    }
	}
    } CVMID_localrootEnd();

    return err;
}

/* Return via "valuePtr" the value of the local variable at the
 * specified slot.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_INVALID_SLOT,
 * JVMDI_ERROR_TYPE_MISMATCH, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OPAQUE_FRAME
 */
static jvmdiError JNICALL
jvmdi_GetLocalInt(jframeID frame, jint slot, 
		  jint *valuePtr)
{
    CVMJavaVal32* si;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();

    DEBUG_ENABLED();
    NOT_NULL(valuePtr);

    CVMD_gcUnsafeExec(ee, {
	err = jvmdiGCUnsafeGetSlot(frame, slot, &si);
    });

    if (err == JVMDI_ERROR_NONE) {
        *valuePtr = (jint)si->i;
    }
    return err;
}

/* Return via "valuePtr" the value of the local variable at the
 * specified slot.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_INVALID_SLOT,
 * JVMDI_ERROR_TYPE_MISMATCH, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OPAQUE_FRAME
 */
static jvmdiError JNICALL
jvmdi_GetLocalLong(jframeID frame, jint slot, 
		   jlong *valuePtr)
{
    CVMJavaVal32* si;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();

    DEBUG_ENABLED();
    NOT_NULL(valuePtr);

    CVMD_gcUnsafeExec(ee, {
	err = jvmdiGCUnsafeGetSlot(frame, slot, &si);
    });

    if (err == JVMDI_ERROR_NONE) {
	*valuePtr = CVMjvm2Long(&si->raw);
    }
    return err;
}

/* Return via "valuePtr" the value of the local variable at the
 * specified slot.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_INVALID_SLOT,
 * JVMDI_ERROR_TYPE_MISMATCH, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OPAQUE_FRAME
 */
static jvmdiError JNICALL
jvmdi_GetLocalFloat(jframeID frame, jint slot, 
		    jfloat *valuePtr)
{
    CVMJavaVal32* si;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();

    DEBUG_ENABLED();
    NOT_NULL(valuePtr);

    CVMD_gcUnsafeExec(ee, {
	err = jvmdiGCUnsafeGetSlot(frame, slot, &si);
    });

    if (err == JVMDI_ERROR_NONE) {
        *valuePtr = (jfloat)si->f;
    }
    return err;
}

/* Return via "valuePtr" the value of the local variable at the
 * specified slot.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_INVALID_SLOT,
 * JVMDI_ERROR_TYPE_MISMATCH, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OPAQUE_FRAME
 */
static jvmdiError JNICALL
jvmdi_GetLocalDouble(jframeID frame, jint slot, 
		     jdouble *valuePtr)
{
    CVMJavaVal32* si;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();

    DEBUG_ENABLED();
    NOT_NULL(valuePtr);

    CVMD_gcUnsafeExec(ee, {
	err = jvmdiGCUnsafeGetSlot(frame, slot, &si);
    });

    if (err == JVMDI_ERROR_NONE) {
        *valuePtr = CVMjvm2Double(&si->raw);
    }
    return err;
}

/* Set the value of the local variable at the specified slot.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_INVALID_SLOT,
 * JVMDI_ERROR_TYPE_MISMATCH, JVMDI_ERROR_OPAQUE_FRAME
 */
static jvmdiError JNICALL
jvmdi_SetLocalObject(jframeID frame, jint slot, jobject value)
{
    CVMJavaVal32* si;
    CVMExecEnv* ee = CVMgetEE();
    jvmdiError err;

    DEBUG_ENABLED();

    CVMD_gcUnsafeExec(ee, {
	err = jvmdiGCUnsafeGetSlot(frame, slot, &si);
	
	if (err == JVMDI_ERROR_NONE) {
	    if (value != NULL) {
		CVMID_icellAssignDirect(ee, &si->r, value);
	    } else {
		CVMID_icellSetNull(&si->r);
	    }
	}
    });
    return err;
}

/* Set the value of the local variable at the specified slot.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_INVALID_SLOT,
 * JVMDI_ERROR_TYPE_MISMATCH, JVMDI_ERROR_OPAQUE_FRAME
 */
static jvmdiError JNICALL
jvmdi_SetLocalInt(jframeID frame, jint slot, jint value)
{
    CVMJavaVal32* si;
    CVMExecEnv* ee = CVMgetEE();
    jvmdiError err;

    DEBUG_ENABLED();

    CVMD_gcUnsafeExec(ee, {
	err = jvmdiGCUnsafeGetSlot(frame, slot, &si);
	if (err == JVMDI_ERROR_NONE) {
	    si->i = (CVMJavaInt)value;
	}
    });

    return err;
}

/* Set the value of the local variable at the specified slot.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_INVALID_SLOT,
 * JVMDI_ERROR_TYPE_MISMATCH, JVMDI_ERROR_OPAQUE_FRAME
 */
static jvmdiError JNICALL
jvmdi_SetLocalLong(jframeID frame, jint slot, jlong value)
{
    CVMJavaVal32* si;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();

    DEBUG_ENABLED();

    CVMD_gcUnsafeExec(ee, {
	err = jvmdiGCUnsafeGetSlot(frame, slot, &si);
	if (err == JVMDI_ERROR_NONE) {
	    CVMlong2Jvm(&si->raw, value);
	}
    });

    return err;    
}

/* Set the value of the local variable at the specified slot.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_INVALID_SLOT,
 * JVMDI_ERROR_TYPE_MISMATCH, JVMDI_ERROR_OPAQUE_FRAME
 */
static jvmdiError JNICALL
jvmdi_SetLocalFloat(jframeID frame, jint slot, jfloat value)
{
    CVMJavaVal32* si;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();

    DEBUG_ENABLED();

    CVMD_gcUnsafeExec(ee, {
	err = jvmdiGCUnsafeGetSlot(frame, slot, &si);
	if (err == JVMDI_ERROR_NONE) {
	    si->f = value;
	}
    });
    return err;
}

/* Set the value of the local variable at the specified slot.
 * Errors: JVMDI_ERROR_INVALID_FRAMEID, JVMDI_ERROR_INVALID_SLOT,
 * JVMDI_ERROR_TYPE_MISMATCH, JVMDI_ERROR_OPAQUE_FRAME
 */
static jvmdiError JNICALL
jvmdi_SetLocalDouble(jframeID frame, jint slot, jdouble value)
{
    CVMJavaVal32* si;
    jvmdiError err = jvmdiGCUnsafeGetSlot(frame, slot, &si);
    CVMExecEnv* ee = CVMgetEE();

    DEBUG_ENABLED();

    CVMD_gcUnsafeExec(ee, {
	err = jvmdiGCUnsafeGetSlot(frame, slot, &si);
	if (err == JVMDI_ERROR_NONE) {
	    CVMdouble2Jvm(&si->raw, value);
	}
    });
    return err;    
}

  /*
   *  Breakpoints
   */

/* Set a breakpoint.
 */
static jvmdiError JNICALL
jvmdi_SetBreakpoint(jclass clazz, jmethodID method, 
                    jlocation bci)
{
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMUint8* code;
    CVMInt32 bciInt;
    CVMUint8* pc;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();
    CVMJavaLong sz;
    jvmdiError initError = initializeJVMDI();

    DEBUG_ENABLED();
    THREAD_OK(ee);

    if (initError != JVMDI_ERROR_NONE) {
	return initError;
    }


    if (CVMmbIs(mb, NATIVE)) {
        return JVMDI_ERROR_INVALID_METHODID;
    }

    /* The jmd for <clinit> gets freed after it executes. Can't take
       any chances. */
    if (CVMmbJmd(mb) == NULL) {
        return JVMDI_ERROR_INVALID_METHODID;
    }

    code = CVMmbJavaCode(mb);
    CVMassert(code != NULL);

    bciInt = CVMlong2Int(bci);
    pc = code + bciInt;

    sz = CVMint2Long(CVMmbCodeLength(mb));
    if (CVMlongLtz(bci) || CVMlongGe(bci,sz)) {
        return JVMDI_ERROR_INVALID_LOCATION;
    }

    DEBUGGER_LOCK(ee);
    if (CVMbagFind(CVMglobals.jvmdiStatics.breakpoints, pc) == NULL) {
        JNIEnv *env = CVMexecEnv2JniEnv(CVMgetEE());
        jobject classRef = (*env)->NewGlobalRef(env, clazz);

        if (classRef == NULL) {
            err = JVMDI_ERROR_OUT_OF_MEMORY;
        } else {
            struct bkpt *bp = CVMbagAdd(CVMglobals.jvmdiStatics.breakpoints);
            if (bp == NULL) {
                (*env)->DeleteGlobalRef(env, classRef);
                err = JVMDI_ERROR_OUT_OF_MEMORY;
            } else {
                bp->pc = pc;
                bp->opcode = *pc;
                /* Keep a reference to the class to prevent gc */
                bp->classRef = classRef;
                *pc = opc_breakpoint;
#ifdef CVM_HW
                CVMhwFlushCache(pc, pc + 1);
#endif
                err = JVMDI_ERROR_NONE;
            }
        }
    } else {
        err = JVMDI_ERROR_DUPLICATE;
    }
    DEBUGGER_UNLOCK(ee);    

    return err;
}

/* Clear a breakpoint.
 */
static jvmdiError JNICALL
jvmdi_ClearBreakpoint(jclass clazz, jmethodID method, 
                      jlocation bci)
{
    struct bkpt *bp;
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMUint8* code;
    CVMInt32 bciInt;
    CVMUint8* pc;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    
    CVMJavaLong sz;
    jvmdiError initError = initializeJVMDI();

    DEBUG_ENABLED();
    THREAD_OK(ee);

    if (initError != JVMDI_ERROR_NONE) {
	return initError;
    }

    if (CVMmbIs(mb, NATIVE)) {
        return JVMDI_ERROR_INVALID_METHODID;
    }

    /* The jmd for <clinit> gets freed after it executes. Can't take
       any chances. (NOTE: this is probably a bug because you can
       never clear out a breakpoint set in a <clinit> method after
       it's run.) */
    if (CVMmbJmd(mb) == NULL) {
        return JVMDI_ERROR_INVALID_METHODID;
    }

    code = CVMmbJavaCode(mb);
    CVMassert(code != NULL);

    bciInt = CVMlong2Int(bci);
    pc = code + bciInt;

    env = CVMexecEnv2JniEnv(ee);

    sz = CVMint2Long(CVMmbCodeLength(mb));
    if (CVMlongLtz(bci) || CVMlongGe(bci,sz)) {
        return JVMDI_ERROR_INVALID_LOCATION;
    }

    DEBUGGER_LOCK(ee);
    bp = CVMbagFind(CVMglobals.jvmdiStatics.breakpoints, pc);
    if (bp == NULL) {
        err = JVMDI_ERROR_NOT_FOUND;
    } else {
        clear_bkpt(env, bp);
        CVMbagDelete(CVMglobals.jvmdiStatics.breakpoints, bp);
        err = JVMDI_ERROR_NONE;
    }
    DEBUGGER_UNLOCK(ee);

    return err;
}

static CVMBool
clearAllBreakpointsHelper(void *item, void *arg)
{
    return clear_bkpt((JNIEnv*) arg, (struct bkpt*) item);
}

/* Clear all breakpoints.
 * Errors: JVMDI_ERROR_VM_DEAD
 */
static jvmdiError JNICALL
jvmdi_ClearAllBreakpoints()
{
    CVMExecEnv* ee = CVMgetEE();
    jvmdiError initError = initializeJVMDI();

    DEBUG_ENABLED();
    THREAD_OK(ee);

    if (initError != JVMDI_ERROR_NONE) {
	return initError;
    }

    DEBUGGER_LOCK(ee);
    {
        CVMExecEnv* ee = CVMgetEE();
        JNIEnv *env = CVMexecEnv2JniEnv(ee);
        CVMbagEnumerateOver(CVMglobals.jvmdiStatics.breakpoints, 
			    clearAllBreakpointsHelper, 
			    (void *)env);
        CVMbagDeleteAll(CVMglobals.jvmdiStatics.breakpoints);
    }
    DEBUGGER_UNLOCK(ee);          
        
    return JVMDI_ERROR_NONE;
}


  /*
   *  Field Watching
   */

/* Set a field watch.
 */
static jvmdiError
setFieldWatch(jclass clazz, jfieldID field, 
              struct CVMBag* watchedFields, CVMBool *ptrWatching)
{
    CVMFieldBlock* fb = (CVMFieldBlock*) field;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();
    jvmdiError initError = initializeJVMDI();

    DEBUG_ENABLED();
    THREAD_OK(ee);

    if (initError != JVMDI_ERROR_NONE) {
	return initError;
    }

    DEBUGGER_LOCK(ee);
    if (CVMbagFind(watchedFields, fb) == NULL) {
        JNIEnv *env = CVMexecEnv2JniEnv(ee);
        jobject classRef = (*env)->NewGlobalRef(env, clazz);

        if (classRef == NULL) {
            err = JVMDI_ERROR_OUT_OF_MEMORY;
        } else {
            struct fieldWatch *fw = CVMbagAdd(watchedFields);
            if (fw == NULL) {
                (*env)->DeleteGlobalRef(env, classRef);
                err = JVMDI_ERROR_OUT_OF_MEMORY;
            } else {
                fw->fb = fb;
                /* Keep a reference to the class to prevent gc */
                fw->classRef = classRef;
                /* reset the global flag */
                *ptrWatching = CVM_TRUE;
                err = JVMDI_ERROR_NONE;
            }
	}
    } else {
        err = JVMDI_ERROR_DUPLICATE;
    }
    DEBUGGER_UNLOCK(ee);    

    return err;
}

/* Clear a field watch.
 */
static jvmdiError
clearFieldWatch(jclass clazz, jfieldID field, 
                struct CVMBag* watchedFields, CVMBool *ptrWatching)
{
    CVMFieldBlock* fb = (CVMFieldBlock*) field;
    jvmdiError err;
    CVMExecEnv* ee = CVMgetEE();
    struct fieldWatch *fw;
    jvmdiError initError = initializeJVMDI();

    DEBUG_ENABLED();
    THREAD_OK(ee);

    if (initError != JVMDI_ERROR_NONE) {
	return initError;
    }

    DEBUGGER_LOCK(ee);
    fw = CVMbagFind(watchedFields, fb);
    if (fw != NULL) {
        JNIEnv *env = CVMexecEnv2JniEnv(ee);

        /* 
         * De-reference the enclosing class so that it's 
         * GC is no longer prevented by
         * this field watch. 
         */
        (*env)->DeleteGlobalRef(env, fw->classRef);
        /* delete it from list */
        CVMbagDelete(watchedFields, fw);
        /* reset the global flag */
        *ptrWatching = CVMbagSize(watchedFields) > 0;
        err = JVMDI_ERROR_NONE;
    } else {
        err = JVMDI_ERROR_NOT_FOUND;
    }
    DEBUGGER_UNLOCK(ee);

    return err;
}

/* Set a field modification watch.
 */
static jvmdiError JNICALL
jvmdi_SetFieldModificationWatch(jclass clazz, jfieldID field)
{
    return setFieldWatch(clazz, field, 
	CVMglobals.jvmdiStatics.watchedFieldModifications, 
        &CVMglobals.jvmdiWatchingFieldModification);
}

/* Clear a field modification watch.
 */
static jvmdiError JNICALL
jvmdi_ClearFieldModificationWatch(jclass clazz, jfieldID field)
{
    return clearFieldWatch(clazz, field, 
	CVMglobals.jvmdiStatics.watchedFieldModifications, 
        &CVMglobals.jvmdiWatchingFieldModification);
}

/* Set a field access watch.
 */
static jvmdiError JNICALL
jvmdi_SetFieldAccessWatch(jclass clazz, jfieldID field)
{
    return setFieldWatch(clazz, field, 
	CVMglobals.jvmdiStatics.watchedFieldAccesses, 
        &CVMglobals.jvmdiWatchingFieldAccess);
}

/* Clear a field access watch.
 */
static jvmdiError JNICALL
jvmdi_ClearFieldAccessWatch(jclass clazz, jfieldID field)
{
    return clearFieldWatch(clazz, field, 
	CVMglobals.jvmdiStatics.watchedFieldAccesses, 
        &CVMglobals.jvmdiWatchingFieldAccess);
}


  /*
   *  Events
   */

/* Set the routine which will handle in-coming events.
 * Passing NULL as hook unsets hook.
 */
static jvmdiError JNICALL
jvmdi_SetEventHook(JVMDI_EventHook hook)
{
    jvmdiError initError = initializeJVMDI();
    
    DEBUG_ENABLED();
    THREAD_OK(CVMgetEE());

    if (initError != JVMDI_ERROR_NONE) {
	return initError;
    }

    CVMglobals.jvmdiStatics.eventHook = hook;
    return JVMDI_ERROR_NONE;
}

/* single-step support */

static void set_single_step_thread(CVMExecEnv* ee,
				   CVMObjectICell* threadICell,
				   CVMBool shouldStep) 
{
#if 0
    CVMJavaInt wasSteppingInt;
    CVMBool wasStepping;
    CVMJavaInt shouldStepInt;
#endif
    CVMExecEnv *targetEE;
    
    targetEE = jthreadToExecEnv(ee, threadICell);

    CVMassert(targetEE != NULL);

    targetEE->jvmdiSingleStepping = shouldStep;

#if 0
    CVMID_fieldReadInt(ee, threadICell,
		       CVMoffsetOfjava_lang_Thread_single_step,
		       wasSteppingInt);
    wasStepping = (CVMBool) wasSteppingInt;
    shouldStepInt = (CVMJavaInt) shouldStep;
    CVMID_fieldWriteInt(ee, threadICell,
			CVMoffsetOfjava_lang_Thread_single_step,
			shouldStepInt);
    /* %comment: k013 */

    if (wasStepping != shouldStep) {
        if (shouldStep) {
            CVMglobals.jvmdiSingleStepping++;
        } else if (CVMglobals.jvmdiSingleStepping > 0) {
            CVMglobals.jvmdiSingleStepping--;
        }
    }
#endif
}

static jvmdiError JNICALL
jvmdi_SetEventNotificationMode(jint mode, jint eventType, jthread thread, ...)
{
    jboolean isEnabled = (mode == JVMDI_ENABLE);
    jvmdiError rc = JVMDI_ERROR_NONE;
    va_list argPtr;
    jvmdiError initError = initializeJVMDI();
    
    if (initError != JVMDI_ERROR_NONE) {
        return initError;
    }

    /* Currently, we don't accept any additional arguments. */
    va_start(argPtr, thread);
    va_end(argPtr);

    DEBUG_ENABLED();
    
    if ((eventType < 0) || (eventType > JVMDI_MAX_EVENT_TYPE_VAL)) {
        return JVMDI_ERROR_INVALID_EVENT_TYPE;
    }

    if (thread != NULL) {
        /*
         * These events cannot be controlled on a per-thread basis
         */
        if ((eventType == JVMDI_EVENT_VM_INIT) ||
            (eventType == JVMDI_EVENT_VM_DEATH) ||
            (eventType == JVMDI_EVENT_THREAD_START) ||
            (eventType == JVMDI_EVENT_CLASS_UNLOAD)) {
            return JVMDI_ERROR_INVALID_THREAD;
        }
    }

    if (eventType == JVMDI_EVENT_SINGLE_STEP) {
        if (thread == NULL) {
            rc = JVMDI_ERROR_INVALID_THREAD;
        } else {
	    CVMExecEnv* ee = CVMgetEE();
	    DEBUGGER_LOCK(ee);
            set_single_step_thread(ee, thread, isEnabled);
	    DEBUGGER_UNLOCK(ee);
        }
    } else if (thread == NULL) {
        enableAllEvents(eventType, isEnabled);
    } else {
	CVMExecEnv* ee = CVMgetEE();
        ThreadNode *node;

        DEBUGGER_LOCK(ee);

        node = findThread(ee, thread);
        if (node == NULL) {
            rc = JVMDI_ERROR_INVALID_THREAD;
        } else {
            enableThreadEvents(ee, node, eventType, isEnabled);
        }

        DEBUGGER_UNLOCK(ee);
    }

    return rc;
}


  /*
   *  Memory allocation
   */

/* Set the routines which will perform allocation and deallocation.
 * Passing NULL in both reverts to default allocator.
 */
static jvmdiError JNICALL
jvmdi_SetAllocationHooks(JVMDI_AllocHook ahook, JVMDI_DeallocHook dhook) 
{
    DEBUG_ENABLED();

    CVMglobals.jvmdiStatics.allocHook = ahook;
    CVMglobals.jvmdiStatics.deallocHook = dhook;
    return JVMDI_ERROR_NONE;
}


/* Allocate an area of memory of "size" bytes.  Return via "memPtr" 
 * a pointer to the beginning of this area. Use the allocator set
 * by JVMDI_SetAllocationHooks().
 * Errors: JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_Allocate(jlong size, jbyte** memPtr) 
{
    jbyte *mem;
    CVMJavaInt intSize;
    JVMDI_AllocHook alloc_hook = CVMglobals.jvmdiStatics.allocHook; 
    
    DEBUG_ENABLED();
    NOT_NULL(memPtr);
    if (alloc_hook != NULL) {
      return alloc_hook(size, memPtr);
    }
    /* %comment: k014 */
    intSize = CVMlong2Int(size);
    mem = (jbyte *)malloc(intSize);
    if (mem == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    *memPtr = mem;
    return JVMDI_ERROR_NONE;
}

/* Deallocate an area of memory of "mem" which was allocated by the
 * allocator set by JVMDI_SetAllocationHooks(). Use the deallocator set
 * by JVMDI_SetAllocationHooks().
 * Errors: JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_Deallocate(jbyte* mem) 
{
    JVMDI_DeallocHook deallocHook = CVMglobals.jvmdiStatics.deallocHook;

    DEBUG_ENABLED();
    NOT_NULL(mem);
    if (deallocHook != NULL) {
      return deallocHook(mem);
    }
    free(mem);
    return JVMDI_ERROR_NONE;
}


  /*
   *  Class access
   */

/* Return via "sigPtr" the class's signature (UTF8).  Form
 * is a JNI signature.  Examples:  "Ljava/io/File;"
 * "[Ljava/util/Date;" (an array of dates).
 * Memory for this string is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().  
 * Errors: JVMDI_ERROR_INVALID_CLASS, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetClassSignature(jclass clazz, char **sigPtr) 
{
    char *name;
    int len;
    CVMJavaLong longLen;
    CVMClassBlock* cb;

    DEBUG_ENABLED();

    cb = object2class(CVMgetEE(), clazz);
    VALID_CLASS(cb);

    if (!CVMcbIs(cb, PRIMITIVE)) {
	name = CVMtypeidClassNameToAllocatedCString(CVMcbClassName(cb));
	len = strlen(name);
	if (name[0] == '[') { /* arrays are already in signature form */
	    longLen = CVMint2Long(len+1);
            ALLOC_WITH_CLEANUP_IF_FAILED(longLen, sigPtr, {
                free (name);
            });
	    strcpy(*sigPtr, name);
	} else {
	    char *sig;
	    longLen = CVMint2Long(len+3);
            ALLOC_WITH_CLEANUP_IF_FAILED(longLen, sigPtr, {
                free (name);
            });
	    sig = *sigPtr;
	    sig[0] = 'L';
	    strcpy(sig+1, name);
	    sig[len+1] = ';';
	    sig[len+2] = 0;
	}        
	free(name);
    } else {
	char *sig;
	longLen = CVMint2Long(2);
	ALLOC(longLen, sigPtr);
	sig = *sigPtr;
	sig[0] = CVMbasicTypeSignatures[CVMcbBasicTypeCode(cb)];
	sig[1] = 0;
    }
    return JVMDI_ERROR_NONE;
}

/* Return via "statePtr" the current state bits of this class.
 * Errors: JVMDI_ERROR_INVALID_CLASS, JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetClassStatus(jclass clazz, jint *statePtr) {
    jint state = 0;
    CVMClassBlock* cb;
    CVMExecEnv *ee = CVMgetEE();

    DEBUG_ENABLED();
    NOT_NULL(statePtr);

    cb = object2class(ee, clazz);
    VALID_CLASS(cb);

    if (CVMcbCheckRuntimeFlag(cb, VERIFIED)) {
        state |= JVMDI_CLASS_STATUS_VERIFIED;
    }

    if (CVMcbCheckRuntimeFlag(cb, LINKED)) {
        state |= JVMDI_CLASS_STATUS_PREPARED;
    }

    if (CVMcbInitializationDoneFlag(ee, cb)) {
        state |= JVMDI_CLASS_STATUS_INITIALIZED;
    }

    if (CVMcbCheckErrorFlag(ee, cb)) {
        state |= JVMDI_CLASS_STATUS_ERROR;
    }

    *statePtr = state;
    return JVMDI_ERROR_NONE;
}

/* Return via "sourceNamePtr" the class's source path (UTF8).
 * Memory for this string is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Errors: JVMDI_ERROR_INVALID_CLASS, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OUT_OF_MEMORY, JVMDI_ERROR_ABSENT_INFORMATION
 */
static jvmdiError JNICALL
jvmdi_GetSourceFileName(jclass clazz, char **sourceNamePtr) 
{
    char *srcName;
    CVMClassBlock* cb;
    CVMJavaLong sz;

    DEBUG_ENABLED();

    cb = object2class(CVMgetEE(), clazz);
    VALID_CLASS(cb);
    srcName = CVMcbSourceFileName(cb);
    if (srcName == NULL) {
        return JVMDI_ERROR_ABSENT_INFORMATION;
    }
    sz = CVMint2Long(strlen(srcName)+1);
    ALLOC(sz, sourceNamePtr);
    strcpy(*sourceNamePtr, srcName);
    return JVMDI_ERROR_NONE;
}

/* Return via "modifiersPtr" the modifiers of this class.
 * See JVM Spec "access_flags".
 * Errors: JVMDI_ERROR_INVALID_CLASS, JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetClassModifiers(jclass clazz, jint *modifiersPtr) 
{
    CVMExecEnv *ee = CVMgetEE();

    DEBUG_ENABLED();
    NOT_NULL(modifiersPtr);
    THREAD_OK(ee);

    *modifiersPtr = JVM_GetClassModifiers(CVMexecEnv2JniEnv(ee),
					  clazz);
    return JVMDI_ERROR_NONE;
}

/* Return via "methodsPtr" a list of the class's methods and 
 * constructors ("methodCountPtr" returns the number of methods and
 * constructors).
 * Memory for this list is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Inherited methods are not included.
 * Errors: JVMDI_ERROR_INVALID_CLASS, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetClassMethods(jclass clazz, 
                      jint *methodCountPtr, jmethodID **methodsPtr) 
{
    jint methods_count;
    jmethodID *mids;
    int i;
    jint state;
    CVMClassBlock* cb; 
    CVMJavaLong sz;

    DEBUG_ENABLED();
    NOT_NULL2(methodCountPtr, methodsPtr);

    cb = object2class(CVMgetEE(), clazz);
    VALID_CLASS(cb);

    jvmdi_GetClassStatus(clazz, &state);
    if (!(state & JVMDI_CLASS_STATUS_PREPARED)) {
        return JVMDI_ERROR_CLASS_NOT_PREPARED;
    }

    /* %comment: k015 */
    methods_count = CVMcbMethodCount(cb);
    *methodCountPtr = methods_count;
    sz = CVMint2Long(methods_count*sizeof(jmethodID));
    ALLOC(sz, methodsPtr);
    mids = *methodsPtr;
    for (i = 0; i < methods_count; ++i) {
        mids[i] = CVMcbMethodSlot(cb, i);
    }
    return JVMDI_ERROR_NONE;
}

/* Return via "fieldsPtr" a list of the class's fields ("fieldCountPtr" 
 * returns the number of fields).
 * Memory for this list is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Inherited fields are not included.
 * Errors: JVMDI_ERROR_INVALID_CLASS, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetClassFields(jclass clazz, 
                     jint *fieldCountPtr, jfieldID **fieldsPtr) 
{
    jint fields_count;
    jfieldID *fids;
    int i;
    jint state;
    CVMClassBlock* cb;
    CVMJavaLong sz;

    DEBUG_ENABLED();
    NOT_NULL2(fieldCountPtr, fieldsPtr);

    cb = object2class(CVMgetEE(), clazz);
    VALID_CLASS(cb);
    
    jvmdi_GetClassStatus(clazz, &state);
    if (!(state & JVMDI_CLASS_STATUS_PREPARED)) {
        return JVMDI_ERROR_CLASS_NOT_PREPARED;
    }

    fields_count = CVMcbFieldCount(cb);
    *fieldCountPtr = fields_count;
    sz = CVMint2Long(fields_count*sizeof(jfieldID));
    ALLOC(sz, fieldsPtr);
    fids = *fieldsPtr;
    for (i = 0; i < fields_count; ++i) {
        fids[i] = CVMcbFieldSlot(cb, i);
    }
    return JVMDI_ERROR_NONE;
}


/* Return via "interfacesPtr" a list of the interfaces this class
 * declares ("interfaceCountPtr" returns the number of such interfaces).  
 * Memory for this list is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Note: classes in the list are a JNI global reference and must
 * be explicitly managed.
 * Errors: JVMDI_ERROR_INVALID_CLASS, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetImplementedInterfaces(jclass clazz,
                               jint *interfaceCountPtr, 
                               jclass **interfacesPtr) 
{
    jint interfaces_count;
    int i;
    /* %comment: k016 */
    jclass *imps;
    jint state;
    CVMClassBlock* cb;   
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    
    CVMJavaLong sz;

    DEBUG_ENABLED();
    NOT_NULL2(interfaceCountPtr, interfacesPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    cb = object2class(ee, clazz);
    VALID_CLASS(cb);

    jvmdi_GetClassStatus(clazz, &state);
    if (!(state & JVMDI_CLASS_STATUS_PREPARED)) {
        return JVMDI_ERROR_CLASS_NOT_PREPARED;
    }

    interfaces_count = CVMcbImplementsCount(cb);
    *interfaceCountPtr = interfaces_count;
    sz = CVMint2Long(interfaces_count * sizeof(jclass));
    ALLOC(sz, interfacesPtr);
    imps = *interfacesPtr;
    for (i = 0; i < interfaces_count; ++i) {
	imps[i] =
	    (*env)->NewGlobalRef(env,
				 CVMcbJavaInstance(CVMcbInterfacecb(cb, i)));
	if (imps[i] == NULL) {
            int j;
            /* Release all the previously allocated resources before exiting: */
            for (j = 0; j < i; j++) {
                (*env)->DeleteGlobalRef(env, imps[j]);
            }
            free(imps);
            *interfacesPtr = 0;
	    return JVMDI_ERROR_OUT_OF_MEMORY;
	}
    }
    return JVMDI_ERROR_NONE;
}

/* Return via "isInterfacePtr" a boolean indicating whether this "clazz"
 * is an interface.
 * Errors: JVMDI_ERROR_INVALID_CLASS, JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_IsInterface(jclass clazz, jboolean *isInterfacePtr) 
{
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    DEBUG_ENABLED();
    THREAD_OK(ee);

    NOT_NULL(isInterfacePtr);
    *isInterfacePtr = JVM_IsInterface(env, clazz);
    return JVMDI_ERROR_NONE;
}


/* Return via "isArrayClassPtr" a boolean indicating whether this "clazz"
 * is an array class.
 * Errors: JVMDI_ERROR_INVALID_CLASS, JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_IsArrayClass(jclass clazz, jboolean *isArrayClassPtr) 
{
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;

    DEBUG_ENABLED();
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    NOT_NULL(isArrayClassPtr);
    *isArrayClassPtr = JVM_IsArrayClass(env, clazz);
    return JVMDI_ERROR_NONE;
}

/* Return via "classloaderPtr" the class loader that created this class
 * or interface, null if this class was not created by a class loader.
 * Note: object returned via 'classloaderPtr' is a JNI global 
 * reference and must be explicitly managed.
 * Errors: JVMDI_ERROR_INVALID_CLASS, JVMDI_ERROR_NULL_POINTER,
 * JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetClassLoader(jclass clazz, jobject *classloaderPtr) 
{
    jobject loader;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    

    DEBUG_ENABLED();
    NOT_NULL(classloaderPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    if ((*env)->PushLocalFrame(env, 1) < 0) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    loader = JVM_GetClassLoader(env, clazz);
    *classloaderPtr = (jobject)((*env)->NewGlobalRef(env, loader));
    (*env)->PopLocalFrame(env, 0);
    return JVMDI_ERROR_NONE;
}


  /*
   *  Object access
   */

/* Return via "hashCodePtr" a hash code that can be used in maintaining
 * hash table of object references. This function guarantees the same 
 * hash code value for a particular object throughout its life
 * Errors: JVMDI_ERROR_INVALID_OBJECT, JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetObjectHashCode(jobject object, jint *hashCodePtr) 
{
    DEBUG_ENABLED();
    VALID_OBJECT(object);
    NOT_NULL(hashCodePtr);
    *hashCodePtr = JVM_IHashCode(CVMexecEnv2JniEnv(CVMgetEE()), object);
    return JVMDI_ERROR_NONE;
}

#ifndef FAST_MONITOR

#define SysThread2JThread(env, systhread) \
  (*env)->NewGlobalRef(env, MkRefLocal(env, SysThread2EE(systhread)->thread));

static jvmdiError 
SysThreads2JThreads(JNIEnv *env, sys_thread_t **threads, int count) 
{
    sys_thread_t **thread;
    jvmdiError rc = JVMDI_ERROR_NONE;
    int i = 0;

    if ((*env)->PushLocalFrame(env, count) < 0) {
        rc = JVMDI_ERROR_OUT_OF_MEMORY;
    } else {
        for (i = 0, thread = threads; i < count; i++, thread++) {
            *(jthread *)thread = SysThread2JThread(env, *thread);
            if (*thread == NULL) {
                rc = JVMDI_ERROR_OUT_OF_MEMORY;
                break;
            }
        }
        (*env)->PopLocalFrame(env, 0);
    }

    if (rc != JVMDI_ERROR_NONE) {
        deleteRefArray(env, (jthread *)threads, i);
    }

    return rc;
}

#endif

/* Return via "infoPtr" monitor info for the given object. 
 * All application threads should be suspended before calling this function.
 * Memory for the waiter arrays is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Note: returned objects are allocated globally and must be explictly
 * freed with DeleteGlobalRef.
 * Errors: JVMDI_ERROR_INVALID_OBJECT, JVMDI_ERROR_THREAD_NOT_SUSPENDED,
 * JVMDI_ERROR_OUT_OF_MEMORY
 */


static jvmdiError JNICALL
jvmdi_GetMonitorInfo(jobject object, JVMDI_monitor_info *infoPtr) 
{
#ifdef FAST_MONITOR

    /*
     * Note: this ought to be implementable with fast monitors. All it needs
     * is an implementation of  lookupMonitor which is not currently there.
     */
    return JVMDI_ERROR_NOT_IMPLEMENTED;
#else
    CVMExecEnv *ee = CVMgetEE();
    JNIEnv *env;
    uintptr_t monitorKey;
    monitor_t *monitor;
    jvmdiError rc;
    
    sysAssert(sizeof(jthread) == sizeof(sys_thread_t *));

    DEBUG_ENABLED();
    NOT_NULL(infoPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    monitorKey = obj_monitor(DeRef(env, object)); 
    monitor = lookupMonitor(self, monitorKey, CVM_FALSE);

    if (monitor == MID_NULL) {
        infoPtr->owner = NULL;
        infoPtr->entry_count = 0;
        infoPtr->waiter_count = 0;
        infoPtr->waiters = NULL;
    } else {
        sys_mon_t *sysMonitor = sysmon(monitor);
        sys_mon_info sysInfo;
        int i;
        jthread *thread;
        int totalWaiters;

        /*
         * Use the first call to count waiters
         */
        sysInfo.sz_monitor_waiters = 0;
        sysInfo.sz_condvar_waiters = 0;
        QUEUE_LOCK(self);
        sysMonitorGetInfo(sysMonitor, &sysInfo);
        QUEUE_UNLOCK(self);

        /*
         * Allocate buffer based on counts
         */
        totalWaiters = sysInfo.n_monitor_waiters + sysInfo.n_condvar_waiters;
        if (totalWaiters > 0) {
            ALLOC(size_to_jlong(totalWaiters * sizeof(sys_thread_t *)),
		  &infoPtr->waiters);
        } else {
            infoPtr->waiters = NULL;
        }

        /*
         * Set up waiter buffers and make the second call to get all the
         * info.
         */
        sysInfo.sz_monitor_waiters = sysInfo.n_monitor_waiters;
        sysInfo.sz_condvar_waiters = sysInfo.n_condvar_waiters;
        sysInfo.monitor_waiters = (sys_thread_t **)infoPtr->waiters;
        sysInfo.condvar_waiters = sysInfo.monitor_waiters + 
                                  sysInfo.n_monitor_waiters;

        QUEUE_LOCK(self);
        sysMonitorGetInfo(sysMonitor, &sysInfo);
        QUEUE_UNLOCK(self);

        if (sysInfo.owner != NULL) {
            if ((*env)->PushLocalFrame(env, 1) < 0) {
                jvmdi_Deallocate((jbyte *)infoPtr->waiters);
                return JVMDI_ERROR_OUT_OF_MEMORY;
            }
            infoPtr->owner = SysThread2JThread(env, sysInfo.owner);
            infoPtr->entry_count = sysInfo.entry_count;
            (*env)->PopLocalFrame(env, 0);
        } else {
            infoPtr->owner = NULL;
            infoPtr->entry_count = 0;
        }
        if ((sysInfo.sz_monitor_waiters != sysInfo.n_monitor_waiters) ||
            (sysInfo.sz_condvar_waiters != sysInfo.n_condvar_waiters)) {
            /* 
             * Something changed between calls. That means a significant
             * thread was running
             */
            jvmdi_Deallocate((jbyte *)infoPtr->waiters);
            return JVMDI_ERROR_THREAD_NOT_SUSPENDED;
        }
        infoPtr->waiter_count = totalWaiters;

        rc = SysThreads2JThreads(env, (sys_thread_t **)infoPtr->waiters,
                                 infoPtr->waiter_count);
        if (rc != JVMDI_ERROR_NONE) {
            jvmdi_Deallocate((jbyte *)infoPtr->waiters);
            return rc;
        }
    }
   
    return JVMDI_ERROR_NONE;
#endif /* FAST_MONITOR */    
}

  /*
   *  Field access
   */

/* Return via "namePtr" the field's name and via "signaturePtr" the
 * field's signature (UTF8).
 * Memory for these strings is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Errors: JVMDI_ERROR_INVALID_FIELDID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetFieldName(jclass clazz, jfieldID field, 
                   char **namePtr, char **signaturePtr)
{
    char *name;
    char *sig;
    CVMFieldBlock* fb = (CVMFieldBlock*) field;
    CVMJavaLong sz;
    
    DEBUG_ENABLED();
    NOT_NULL2(namePtr, signaturePtr);
    if (fb == NULL) {
        return JVMDI_ERROR_INVALID_FIELDID;
    }

    name = CVMtypeidFieldNameToAllocatedCString(CVMfbNameAndTypeID(fb));
    sz = CVMint2Long(strlen(name)+1);
    ALLOC(sz, namePtr);
    strcpy(*namePtr, name);
    free(name);

    sig = CVMtypeidFieldTypeToAllocatedCString(CVMfbNameAndTypeID(fb));
    sz = CVMint2Long(strlen(sig)+1);
    ALLOC(sz, signaturePtr);
    strcpy(*signaturePtr, sig);
    free(sig);

    return JVMDI_ERROR_NONE;
}

/* Return via "declaringClassPtr" the class in which this field is
 * defined.
 * Note: class returned via 'declaringClassPtr' is a JNI global 
 * reference and must be explicitly managed.
 * Errors: JVMDI_ERROR_INVALID_FIELDID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetFieldDeclaringClass(jclass clazz, jfieldID field,
 			     jclass *declaringClassPtr)
{
    CVMFieldBlock* fb = (CVMFieldBlock*) field;
    jobject dclazz;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    

    DEBUG_ENABLED();
    if (clazz == NULL || CVMID_icellIsNull(clazz)) {
        return JVMDI_ERROR_INVALID_CLASS;
    }
    NULL_CHECK(field, JVMDI_ERROR_INVALID_FIELDID);
    NULL_CHECK(declaringClassPtr, JVMDI_ERROR_NULL_POINTER);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    if ((*env)->PushLocalFrame(env, 1) < 0) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    dclazz = (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMfbClassBlock(fb)));
    *declaringClassPtr = (jobject)((*env)->NewGlobalRef(env, dclazz));
    (*env)->PopLocalFrame(env, 0);
    return JVMDI_ERROR_NONE;
}

/* Return via "modifiersPtr" the modifiers of this field.
 * See JVM Spec "access_flags".
 * Errors: JVMDI_ERROR_INVALID_FIELDID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetFieldModifiers(jclass clazz, jfieldID field,
                        jint *modifiersPtr)
{
    CVMFieldBlock* fb = (CVMFieldBlock*) field;
    
    DEBUG_ENABLED();
    NOT_NULL(modifiersPtr);
    /* %comment: k017 */
    *modifiersPtr = CVMfbAccessFlags(fb);
    return JVMDI_ERROR_NONE;
}

/* Return via "isSyntheticPtr" a boolean indicating whether the field
 * is synthetic.
 * Errors: JVMDI_ERROR_INVALID_FIELDID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_IsFieldSynthetic(jclass clazz, jfieldID field, 
		        jboolean *isSyntheticPtr)
{
    /* Synthetic attribute info is not yet available in this VM */
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}


  /*
   *  Method access
   */

/* Return via "namePtr" the method's name and via "signaturePtr" the
 * method's signature (UTF8).
 * Memory for these strings is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetMethodName(jclass clazz, jmethodID method, 
                    char **namePtr, char **signaturePtr)
{
    char *name;
    char *sig;
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMJavaLong sz;
    
    DEBUG_ENABLED();
    NOT_NULL2(namePtr, signaturePtr);
    if (mb == NULL) {
        return JVMDI_ERROR_INVALID_METHODID;
    }

    name = CVMtypeidMethodNameToAllocatedCString(CVMmbNameAndTypeID(mb));
    sz = CVMint2Long(strlen(name)+1);
    ALLOC(sz, namePtr);
    strcpy(*namePtr, name);
    free(name);

    sig = CVMtypeidMethodTypeToAllocatedCString(CVMmbNameAndTypeID(mb));
    sz = CVMint2Long(strlen(sig)+1);
    ALLOC(sz, signaturePtr);
    strcpy(*signaturePtr, sig);
    free(sig);

    return JVMDI_ERROR_NONE;
}

/* Return via "declaringClassPtr" the class in which this method is
 * defined.
 * Note: class returned via 'declaringClassPtr' is a JNI global 
 * reference and must be explicitly managed.
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetMethodDeclaringClass(jclass clazz, jmethodID method,
			      jclass *declaringClassPtr)
{
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    jobject dclazz;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    

    DEBUG_ENABLED();
    NOT_NULL(declaringClassPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    if ((*env)->PushLocalFrame(env, 1) < 0) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    dclazz = (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMmbClassBlock(mb)));
    *declaringClassPtr = (jclass)((*env)->NewGlobalRef(env, dclazz));
    (*env)->PopLocalFrame(env, 0);
    return JVMDI_ERROR_NONE;
}

/* Return via "modifiersPtr" the modifiers of this method.
 * See JVM Spec "access_flags".
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetMethodModifiers(jclass clazz, jmethodID method,
                         jint *modifiersPtr)
{
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    jint modifiers;
    
    DEBUG_ENABLED();
    NOT_NULL(modifiersPtr);

    /* %comment: k018 */
    modifiers = 0;
    if (CVMmbIs(mb, PUBLIC))
	modifiers |= java_lang_reflect_Modifier_PUBLIC;
    if (CVMmbIs(mb, PRIVATE))
	modifiers |= java_lang_reflect_Modifier_PRIVATE;
    if (CVMmbIs(mb, PROTECTED))
	modifiers |= java_lang_reflect_Modifier_PROTECTED;
    if (CVMmbIs(mb, STATIC))
	modifiers |= java_lang_reflect_Modifier_STATIC;
    if (CVMmbIs(mb, FINAL))
	modifiers |= java_lang_reflect_Modifier_FINAL;
    if (CVMmbIs(mb, SYNCHRONIZED))
	modifiers |= java_lang_reflect_Modifier_SYNCHRONIZED;
    if (CVMmbIs(mb, NATIVE))
	modifiers |= java_lang_reflect_Modifier_NATIVE;
    if (CVMmbIs(mb, ABSTRACT))
	modifiers |= java_lang_reflect_Modifier_ABSTRACT;
    if (CVMmbIsJava(mb) && CVMjmdIs(CVMmbJmd(mb), STRICT))
	modifiers |= java_lang_reflect_Modifier_STRICT;

    *modifiersPtr = modifiers;
    return JVMDI_ERROR_NONE;
}

/* Return via "maxPtr" the maximum number of words on the operand
 * stack at any point in during the execution of this method.
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetMaxStack(jclass clazz, jmethodID method, jint *maxPtr)
{
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    
    DEBUG_ENABLED();
    NOT_NULL(maxPtr);

    if (!CVMmbIsJava(mb)) {
	return JVMDI_ERROR_INVALID_METHODID;
    }

    /* The jmd for <clinit> gets freed after it executes. Can't take
       any chances. */
    if (CVMmbJmd(mb) == NULL) {
        return JVMDI_ERROR_INVALID_METHODID;
    }

    *maxPtr = CVMjmdMaxStack(CVMmbJmd(mb));
    return JVMDI_ERROR_NONE;
}

/* Return via "maxPtr" the number of local variable slots used by 
 * this method. Note two-word locals use two slots.
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetMaxLocals(jclass clazz, jmethodID method, jint *maxPtr)
{
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    
    DEBUG_ENABLED();
    NOT_NULL(maxPtr);

    if (!CVMmbIsJava(mb)) {
	return JVMDI_ERROR_INVALID_METHODID;
    }

    /* The jmd for <clinit> gets freed after it executes. Can't take
       any chances. */
    if (CVMmbJmd(mb) == NULL) {
        return JVMDI_ERROR_INVALID_METHODID;
    }

    *maxPtr = CVMmbMaxLocals(mb);
    return JVMDI_ERROR_NONE;
}

/* Return via "sizePtr" the number of local variable slots used by 
 * arguments. Note two-word arguments use two slots.
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetArgumentsSize(jclass clazz, jmethodID method, jint *sizePtr)
{
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    
    DEBUG_ENABLED();
    NOT_NULL(sizePtr);

    *sizePtr = CVMmbArgsSize(mb);
    return JVMDI_ERROR_NONE;
}

/* Return via "tablePtr" the line number table ("entryCountPtr" 
 * returns the number of entries in the table).
 * Memory for this table is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY,
 * JVMDI_ERROR_ABSENT_INFORMATION
 */
static jvmdiError JNICALL
jvmdi_GetLineNumberTable(jclass clazz, jmethodID method,
 			 jint *entryCountPtr, 
                         JVMDI_line_number_entry **tablePtr)
{
    int i;
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMLineNumberEntry* vmtbl;
    CVMUint16 length;
    JVMDI_line_number_entry *tbl;
    CVMJavaLong sz;

    DEBUG_ENABLED();
    NOT_NULL2(entryCountPtr, tablePtr);

    /* NOTE:  Added this check, which is necessary, at least in
       our VM, because this seems to get called for native methods as
       well. Should we return ABSENT_INFORMATION or INVALID_METHODID
       in this case?  */
    if (!CVMmbIsJava(mb)) {
	return JVMDI_ERROR_ABSENT_INFORMATION;
    }

    /* %comment: k025 */

    if (CVMmbJmd(mb) == NULL) {
	sz = CVMint2Long(sizeof(JVMDI_line_number_entry));
	ALLOC(sz, tablePtr);
	*entryCountPtr = (jint)1;
	tbl = *tablePtr;
	tbl[0].start_location = CVMint2Long(0);
	tbl[0].line_number = 0;
	return JVMDI_ERROR_NONE;
    }

    vmtbl = CVMmbLineNumberTable(mb);
    length = CVMmbLineNumberTableLength(mb);

    if (vmtbl == NULL) {
        return JVMDI_ERROR_ABSENT_INFORMATION;
    }
    sz = CVMint2Long(length * sizeof(JVMDI_line_number_entry));
    ALLOC(sz, tablePtr);
    *entryCountPtr = (jint)length;
    tbl = *tablePtr;
    for (i = 0; i < length; ++i) {
        tbl[i].start_location = CVMint2Long(vmtbl[i].startpc);
        tbl[i].line_number = (jint)(vmtbl[i].lineNumber);
    }
    return JVMDI_ERROR_NONE;
}

/* Return via "startLocationPtr" the first location in the method.
 * Return via "endLocationPtr" the last location in the method.
 * If conventional byte code index locations are being used then
 * this returns zero and the number of byte codes minus one
 * (respectively).
 * If location information is not available return 
 * JVMDI_ERROR_ABSENT_INFORMATION and negative one through the pointers.
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_ABSENT_INFORMATION
 *
 */
/* %comment: k019 */
static jvmdiError JNICALL
jvmdi_GetMethodLocation(jclass clazz, jmethodID method,
                        jlocation *startLocationPtr, 
                        jlocation *endLocationPtr)
{
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    
    DEBUG_ENABLED();
    NOT_NULL2(startLocationPtr, endLocationPtr);

    if (!CVMmbIsJava(mb)) {
	*startLocationPtr = CVMint2Long(-1);
	*endLocationPtr = CVMint2Long(-1);
	/*	return JVMDI_ERROR_ABSENT_INFORMATION; */
	return JVMDI_ERROR_NONE;
    }

    /* Because we free the Java method descriptor for <clinit> after
       it's run, it's possible for us to get a frame pop event on a
       method which has a NULL jmd. If that happens, we'll do the same
       thing as above. */

    if (CVMmbJmd(mb) == NULL) {
	/* %coment: k020 */
	*startLocationPtr = CVMlongConstZero();
	*endLocationPtr = CVMlongConstZero();
	/*	return JVMDI_ERROR_ABSENT_INFORMATION; */
	return JVMDI_ERROR_NONE;
    }

    *startLocationPtr = CVMlongConstZero();
    *endLocationPtr = CVMint2Long(CVMmbCodeLength(mb) - 1);
    return JVMDI_ERROR_NONE;
}

/* Return via "tablePtr" the local variable table ("entryCountPtr" 
 * returns the number of entries in the table).
 * Memory for this table and for each "name" and "signature" 
 * string is allocated by the function specified in
 * JVMDI_SetAllocationHooks().
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY,
 * JVMDI_ERROR_ABSENT_INFORMATION
 */
static jvmdiError JNICALL
jvmdi_GetLocalVariableTable(jclass clazz, jmethodID method,
                            jint *entryCountPtr, 
                            JVMDI_local_variable_entry **tablePtr)
{
    int i;
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMLocalVariableEntry* vmtbl;
    CVMUint16 length;
    CVMClassBlock* cb = CVMmbClassBlock(mb);
    CVMConstantPool* constantpool;
    JVMDI_local_variable_entry *tbl;
    CVMJavaLong sz;
    
    DEBUG_ENABLED();
    NOT_NULL2(entryCountPtr, tablePtr);

    if (!CVMmbIsJava(mb)) {
	return JVMDI_ERROR_ABSENT_INFORMATION;
    }

    /* The jmd for <clinit> gets freed after it executes. Can't take
       any chances. */
    if (CVMmbJmd(mb) == NULL) {
	return JVMDI_ERROR_ABSENT_INFORMATION;
    }

    vmtbl = CVMmbLocalVariableTable(mb);
    length = CVMmbLocalVariableTableLength(mb);
    constantpool = CVMcbConstantPool(cb);

    if (vmtbl == NULL) {
        return JVMDI_ERROR_ABSENT_INFORMATION;
    }
    sz = CVMint2Long(length * sizeof(JVMDI_local_variable_entry));
    ALLOC(sz, tablePtr);
    *entryCountPtr = (jint)length;
    tbl = *tablePtr;
    for (i = 0; i < length; ++i) {
	char* nameAndTypeTmp;
	char* buf;

        tbl[i].start_location = CVMint2Long(vmtbl[i].startpc);
        tbl[i].length = (jint)(vmtbl[i].length);

	nameAndTypeTmp =
	    CVMtypeidFieldNameToAllocatedCString(CVMtypeidCreateTypeIDFromParts(vmtbl[i].nameID, vmtbl[i].typeID));
	sz = CVMint2Long(strlen(nameAndTypeTmp)+1);
        ALLOC(sz, &buf);
        strcpy(buf, nameAndTypeTmp);
	free(nameAndTypeTmp);
	tbl[i].name = buf;

	nameAndTypeTmp =
	    CVMtypeidFieldTypeToAllocatedCString(CVMtypeidCreateTypeIDFromParts(vmtbl[i].nameID, vmtbl[i].typeID));
	sz = CVMint2Long(strlen(nameAndTypeTmp)+1);
        ALLOC(sz, &buf);
        strcpy(buf, nameAndTypeTmp);
	free(nameAndTypeTmp);
	tbl[i].signature = buf;

        tbl[i].slot = (jint)(vmtbl[i].index);
    }
    return JVMDI_ERROR_NONE;
}

#ifdef CVM_CLASSLOADING
static jvmdiError 
resolveConstantPoolIndex(CVMExecEnv* ee, JNIEnv *env, 
			 CVMClassBlock* cb, unsigned short cpIdx, 
			 jclass *classPtr) 
{
    if (CVMcpResolveEntryFromClass(ee, cb, CVMcbConstantPool(cb), cpIdx)) {
        CVMClassBlock* cbRes = CVMcpGetCb(CVMcbConstantPool(cb), cpIdx);
        *classPtr =
	    (jclass)((*env)->NewGlobalRef(env, CVMcbJavaInstance(cbRes)));
	return JVMDI_ERROR_NONE;
    } else {
        *classPtr = NULL;
	return JVMDI_ERROR_INTERNAL;
    }
}
#endif

/* Return via "tablePtr" the exception handler table ("entryCountPtr" 
 * returns the number of entries in the table).
 * Memory for this table is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Note: the classes returned in the "exception" field of
 * JVMDI_exception_handler_entry are JNI global references and 
 * must be explicitly managed.
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetExceptionHandlerTable(jclass clazz, jmethodID method,
                               jint *entryCountPtr, 
                               JVMDI_exception_handler_entry **tablePtr)
{
    int i;
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMExceptionHandler* vmtbl;
    CVMUint16 length;
    CVMClassBlock* cb = CVMmbClassBlock(mb);
    JVMDI_exception_handler_entry *tbl;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    
    CVMJavaLong sz;

    DEBUG_ENABLED();
    NOT_NULL2(entryCountPtr, tablePtr);
    THREAD_OK(ee);

    if (!CVMmbIsJava(mb)) {
	return JVMDI_ERROR_ABSENT_INFORMATION;
    }

    /* The jmd for <clinit> gets freed after it executes. Can't take
       any chances. */
    if (CVMmbJmd(mb) == NULL) {
        return JVMDI_ERROR_ABSENT_INFORMATION;
    }

    env = CVMexecEnv2JniEnv(ee);

    vmtbl = CVMmbExceptionTable(mb);
    if (vmtbl == NULL) {
        return JVMDI_ERROR_ABSENT_INFORMATION;
    }
    length = CVMmbExceptionTableLength(mb);
    sz = CVMint2Long(length * sizeof(JVMDI_exception_handler_entry));
    ALLOC(sz, tablePtr);
    *entryCountPtr = (jint)length;
    tbl = *tablePtr;
    for (i = 0; i < length; ++i) {
#ifdef CVM_CLASSLOADING
        jclass catchClass;
        jvmdiError err;
#endif

        tbl[i].start_location = CVMint2Long(vmtbl[i].startpc);
        tbl[i].end_location = CVMint2Long(vmtbl[i].endpc);
        tbl[i].handler_location = CVMint2Long(vmtbl[i].handlerpc);

#ifdef CVM_CLASSLOADING
        err = resolveConstantPoolIndex(ee, env, cb, vmtbl[i].catchtype,
				       &catchClass);
        if (err != JVMDI_ERROR_NONE) {
            return err;
	}

	tbl[i].exception = catchClass;
#else /* CVM_CLASSLOADING */
        tbl[i].exception = (jclass)
	    (*env)->NewGlobalRef(env,
	        CVMcbJavaInstance(CVMcpGetCb(CVMcbConstantPool(cb),
					     vmtbl[i].catchtype)));
	if (tbl[i].exception == NULL) {
	    /* clean up previously-allocated global refs */
	    while (--i >= 0) {
		(*env)->DeleteGlobalRef(env, tbl[i].exception);
	    }
	    jvmdi_Deallocate((jbyte *) *tablePtr);
	    *tablePtr = NULL;
	    return JVMDI_ERROR_OUT_OF_MEMORY;
	}
#endif /* CVM_CLASSLOADING */
    }
    return JVMDI_ERROR_NONE;
}

/* Return via "exceptionsPtr" an array of the checked exceptions this method
 * may throw ("exceptionCountPtr" returns the number of such exceptions).
 * Memory for this array is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Note: classes in the array are JNI global references and must
 * be explicitly managed.
 * Errors: JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetThrownExceptions(jclass clazz, jmethodID method,
                          jint *exceptionCountPtr, jclass **exceptionsPtr)
{
    int i;
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMCheckedExceptions* checkedExceptions;
    CVMUint16* vmexcs;
    CVMClassBlock* cb = CVMmbClassBlock(mb);
    jclass *excs;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    
    CVMJavaLong sz;
    CVMUint32 length;
    jvmdiError err = JVMDI_ERROR_NONE;

    DEBUG_ENABLED();
    NOT_NULL2(exceptionCountPtr, exceptionsPtr);
    THREAD_OK(ee);

    if (!CVMmbIsJava(mb)) {
	return JVMDI_ERROR_INVALID_METHODID;
    }

    env = CVMexecEnv2JniEnv(ee);

    checkedExceptions = CVMmbCheckedExceptions(mb);
    vmexcs = &checkedExceptions->exceptions[0];
    length = checkedExceptions->numExceptions;

    sz = CVMint2Long(length * sizeof(jclass));
    ALLOC(sz, exceptionsPtr);
    *exceptionCountPtr = (jint)length;
    excs = *exceptionsPtr;
    for (i = 0; i < length; ++i) {
#ifdef CVM_CLASSLOADING

        jclass exc;

	err = resolveConstantPoolIndex(ee, env, cb, vmexcs[i], &exc);
        excs[i] = exc;

#else /* CVM_CLASSLOADING */

        excs[i] = (jclass)
	    (*env)->NewGlobalRef(env,
	        CVMcbJavaInstance(CVMcpGetCb(CVMcbConstantPool(cb),
					     vmexcs[i])));
	if (excs[i] == NULL) {
	    err = JVMDI_ERROR_OUT_OF_MEMORY;
	}

#endif /* CVM_CLASSLOADING */

	if (err != JVMDI_ERROR_NONE) {
	    /* clean up previously-allocated global refs */
	    while (--i >= 0) {
		(*env)->DeleteGlobalRef(env, excs[i]);
	    }
	    jvmdi_Deallocate((jbyte *) excs);
	    *exceptionsPtr = NULL;
	    return err;
	}
    }
    return JVMDI_ERROR_NONE;
}

/* NOTE:  This is not implemented in CVM */
/* Return via "bytecodesPtr" the bytecodes implementing this method
 * ("bytecodeCountPtr" returns the number of byte codes).
 * Memory for this array is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Errors: JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_INVALID_METHODID.
 */
static jvmdiError JNICALL
jvmdi_GetBytecodes(jclass clazz, jmethodID method,
		   jint *bytecodeCountPtr, jbyte **bytecodesPtr)
{
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

/* Return via "isNativePtr" a boolean indicating whether the method
 * is native.
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_IsMethodNative(jclass clazz, jmethodID method, 
		    jboolean *isNativePtr)
{
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    
    DEBUG_ENABLED();
    NOT_NULL(isNativePtr);
    *isNativePtr = (CVMmbIs(mb, NATIVE) != 0);
    return JVMDI_ERROR_NONE;
}

/* Return via "isSyntheticPtr" a boolean indicating whether the method
 * is synthetic.
 * Errors: JVMDI_ERROR_INVALID_METHODID, JVMDI_ERROR_INVALID_CLASS,
 * JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_IsMethodSynthetic(jclass clazz, jmethodID method, 
		        jboolean *isSyntheticPtr)
{
    /* Synthetic attribute info is not yet available in this VM */
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

#if 0
/* This is unused in CVM (replaced by JVM_StartSystemThread) */
static void debugThreadWrapper(void *arg) {
    ThreadNode *node = findThread((CVMObjectICell*) arg);
    if (node == NULL) {
        return;
    }

    (node->startFunction)(node->startFunctionArg);
}
#endif

/* Start the execution of a debugger thread. The given thread must be
 * newly created and never previously run. Thread execution begins 
 * in the function "startFunc" which is passed the single argument, "arg".
 * The thread is set to be a daemon.
 * Errors: JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_INVALID_PRIORITY,
 * JVMDI_ERROR_OUT_OF_MEMORY, JVMDI_ERROR_INVALID_THREAD.
 */
static jvmdiError JNICALL
jvmdi_RunDebugThread(jthread thread, 
                     JVMDI_StartFunction proc, void *arg, int priority)
{
    ThreadNode *node;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv* env;

    DEBUG_ENABLED();
    NOT_NULL(proc);

    env = CVMexecEnv2JniEnv(ee);
    /*
     * Make sure a thread hasn't been run for this thread object 
     * before.
     */
    if ((thread == NULL) ||
	(jthreadToExecEnv(ee, thread) != NULL)) {
        return JVMDI_ERROR_INVALID_THREAD;
    }

    if ((priority < JVMDI_THREAD_MIN_PRIORITY) ||
	(priority > JVMDI_THREAD_MAX_PRIORITY)) {
        return JVMDI_ERROR_INVALID_PRIORITY;
    }

    node = insertThread(ee, (CVMObjectICell*) thread);
    if (node == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }

    /* NOTE: These are unused in CVM */
    node->startFunction = proc;
    node->startFunctionArg = arg;
    /* %comment: k021 */
    CVMID_fieldWriteInt(ee, (CVMObjectICell*) thread,
			CVMoffsetOfjava_lang_Thread_daemon,
			1);
    CVMID_fieldWriteInt(ee, (CVMObjectICell*) thread,
			CVMoffsetOfjava_lang_Thread_priority,
			priority);
    JVM_StartSystemThread(env, thread, proc, arg);
    if ((*env)->ExceptionCheck(env)) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }

    return JVMDI_ERROR_NONE;
}

  /*
   *  Debugger monitor support allows easy coordination among debugger 
   *  threads.
   */

/* Return via "monitorPtr" a newly created debugger monitor that can be 
 * used by debugger threads to coordinate. The semantics of the
 * monitor are the same as those described for Java language 
 * monitors in the Java Language Specification.
 * Errors: JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_CreateRawMonitor(char *name, JVMDI_RawMonitor *monitorPtr)
{
    DEBUG_ENABLED();
    ASSERT_NOT_NULL2_ELSE_EXIT_WITH_ERROR(name, monitorPtr, JVMDI_ERROR_NULL_POINTER);

    *monitorPtr = (JVMDI_RawMonitor *) CVMnamedSysMonitorInit(name);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(*monitorPtr, JVMDI_ERROR_OUT_OF_MEMORY);
    return JVMDI_ERROR_NONE;
}

/* Destroy a debugger monitor created with JVMDI_CreateRawMonitor.
 * Errors: JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_DestroyRawMonitor(JVMDI_RawMonitor monitor)
{
    DEBUG_ENABLED();
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMDI_ERROR_NULL_POINTER);

    CVMnamedSysMonitorDestroy((CVMNamedSysMonitor *) monitor);
    return JVMDI_ERROR_NONE;
}

/* Gain exclusive ownership of a debugger monitor.
 * Errors: JVMDI_ERROR_INVALID_MONITOR, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_RawMonitorEnter(JVMDI_RawMonitor monitor)
{
    CVMExecEnv *current_ee = CVMgetEE();

    DEBUG_ENABLED();
    THREAD_OK(current_ee);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMDI_ERROR_INVALID_MONITOR);

    CVMnamedSysMonitorEnter((CVMNamedSysMonitor *) monitor, current_ee);
    return JVMDI_ERROR_NONE;
}

/* Release exclusive ownership of a debugger monitor.
 * Errors: JVMDI_ERROR_INVALID_MONITOR, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_RawMonitorExit(JVMDI_RawMonitor monitor)
{
    CVMExecEnv *current_ee = CVMgetEE();

    DEBUG_ENABLED();
    THREAD_OK(current_ee);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMDI_ERROR_INVALID_MONITOR);

    CVMnamedSysMonitorExit((CVMNamedSysMonitor *) monitor, current_ee);
    return JVMDI_ERROR_NONE;
}

/* Wait for notification of the debugger monitor. The calling thread
 * must own the monitor. "millis" specifies the maximum time to wait, 
 * in milliseconds.  If millis is -1, then the thread waits forever.
 * Errors: JVMDI_ERROR_INVALID_MONITOR, JVMDI_ERROR_NOT_MONITOR_OWNER,
 * JVMDI_ERROR_INTERRUPT
 */
static jvmdiError JNICALL
jvmdi_RawMonitorWait(JVMDI_RawMonitor monitor, jlong millis)
{
    jvmdiError result = JVMDI_ERROR_INTERNAL;
    CVMWaitStatus error;
    CVMExecEnv *current_ee = CVMgetEE();

    DEBUG_ENABLED();
    THREAD_OK(current_ee);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMDI_ERROR_INVALID_MONITOR);

    /* %comment l001 */
    if (CVMlongLt(millis, CVMlongConstZero()))  /* if (millis < 0) */
        millis = CVMlongConstZero();            /*     millis = 0; */

    error = CVMnamedSysMonitorWait((CVMNamedSysMonitor *) monitor, current_ee, millis);
    switch (error) {
        case CVM_WAIT_OK:           result = JVMDI_ERROR_NONE; break;
        case CVM_WAIT_INTERRUPTED:  result = JVMDI_ERROR_INTERRUPT; break;
        case CVM_WAIT_NOT_OWNER:    result = JVMDI_ERROR_NOT_MONITOR_OWNER; break;
    }
    return result;
}

/* Notify a single thread waiting on the debugger monitor. The calling 
 * thread must own the monitor.
 * Errors: JVMDI_ERROR_INVALID_MONITOR, JVMDI_ERROR_NOT_MONITOR_OWNER
 */
static jvmdiError JNICALL
jvmdi_RawMonitorNotify(JVMDI_RawMonitor monitor)
{
    CVMExecEnv *current_ee = CVMgetEE();
    CVMBool successful;

    DEBUG_ENABLED();
    THREAD_OK(current_ee);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMDI_ERROR_INVALID_MONITOR);

    successful = CVMnamedSysMonitorNotify((CVMNamedSysMonitor*) monitor, current_ee);
    return successful ? JVMDI_ERROR_NONE : JVMDI_ERROR_NOT_MONITOR_OWNER;
}

/* Notify all threads waiting on the debugger monitor. The calling 
 * thread must own the monitor.
 * Errors: JVMDI_ERROR_INVALID_MONITOR, JVMDI_ERROR_NOT_MONITOR_OWNER
 */
static jvmdiError JNICALL
jvmdi_RawMonitorNotifyAll(JVMDI_RawMonitor monitor)
{
    CVMExecEnv *current_ee = CVMgetEE();
    CVMBool successful;

    DEBUG_ENABLED();
    THREAD_OK(current_ee);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMDI_ERROR_INVALID_MONITOR);

    successful = CVMnamedSysMonitorNotifyAll((CVMNamedSysMonitor*) monitor, current_ee);
    return successful ? JVMDI_ERROR_NONE : JVMDI_ERROR_NOT_MONITOR_OWNER;
}

  
  /*
   *  Misc
   */

/* Return via "classesPtr" all classes currently loaded in the VM
 * ("classCountPtr" returns the number of such classes).
 * Memory for this table is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Note: classes in the array are JNI global references and must
 * be explicitly managed.
 * Errors: JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY
 */

typedef struct {
    int numClasses;
    int i;
    jclass *classes;
    jvmdiError err;
} LoadedClassesUserData;

static void
LoadedClassesCountCallback(CVMExecEnv* ee,
			   CVMClassBlock* cb,
			   void* arg)
{
    /* NOTE: in our implementation (compared to JDK 1.2), I think that
       a class showing up in this iteration will always be "loaded",
       and that it isn't necessary to check, for example, the
       CVM_CLASS_SUPERCLASS_LOADED runtime flag. */
    if (!CVMcbCheckErrorFlag(ee, cb)) {
	((LoadedClassesUserData *) arg)->numClasses++;
    }
}

static void
LoadedClassesGetCallback(CVMExecEnv* ee,
			 CVMClassBlock* cb,
			 void* arg)
{
    LoadedClassesUserData *data = (LoadedClassesUserData *) arg;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    if (!CVMcbCheckErrorFlag(ee, cb)) {
	if (data->err == JVMDI_ERROR_NONE) {
	    data->classes[data->i] = CVMID_getGlobalRoot(ee);

	    if (data->classes[data->i] == NULL) {
		while (--data->i >= 0) {
		    (*env)->DeleteGlobalRef(env, data->classes[data->i]);
		}
		jvmdi_Deallocate((jbyte *) data->classes);
		data->classes = NULL;
		data->err = JVMDI_ERROR_OUT_OF_MEMORY;
		/* Can't abort this iteration */
	    } else {
		CVMID_icellAssign(ee, data->classes[data->i],
		    CVMcbJavaInstance(cb));
	    }
	    data->i++;
	}
    }
}

static jvmdiError JNICALL
jvmdi_GetLoadedClasses(jint *classCountPtr, jclass **classesPtr)
{
    LoadedClassesUserData *data;
    CVMExecEnv* ee = CVMgetEE();
    CVMJavaLong sz;
    jvmdiError rc;

    data = malloc(sizeof(LoadedClassesUserData));
    data->numClasses = 0;
    data->i = 0;
    data->err = JVMDI_ERROR_NONE;
    /* Give it a safe value in case we have to abort */
    *classCountPtr = 0;

    /* Seize classTableLock so nothing gets added while we're counting. */
    CVM_CLASSTABLE_LOCK(ee);

    CVMclassIterateAllClasses(ee, &LoadedClassesCountCallback, data);
    sz = CVMint2Long(data->numClasses * sizeof(jclass));
    rc = jvmdi_Allocate(sz, (jbyte**)classesPtr);
    if (rc == JVMDI_ERROR_NONE) {
	data->classes = *classesPtr;
	CVMclassIterateAllClasses(ee, &LoadedClassesGetCallback, data);
	rc = data->err;
    }
    
    if (rc != JVMDI_ERROR_NONE) {
	/* All global refs and (jclass *) memory are freed by this point */
	*classesPtr = NULL;
    } else {
	*classCountPtr = data->numClasses;
    }

    CVM_CLASSTABLE_UNLOCK(ee);
    free(data);
    return rc;
}

/* Return via "classesPtr" all classes loaded through the 
 * given initiating class loader.
 * ("classesCountPtr" returns the number of such threads).
 * Memory for this table is allocated by the function specified 
 * in JVMDI_SetAllocationHooks().
 * Note: classes in the array are JNI global references and must
 * be explicitly managed.
 * Errors: JVMDI_ERROR_NULL_POINTER, JVMDI_ERROR_OUT_OF_MEMORY
 */
static jvmdiError JNICALL
jvmdi_GetClassLoaderClasses(jobject initiatingLoader,
                            jint *classesCountPtr, jclass **classesPtr)
{
    jvmdiError rc = JVMDI_ERROR_NONE;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;    
    CVMLoaderCacheIterator iter;
    int c, n;

    DEBUG_ENABLED();
    THREAD_OK(ee);
    NOT_NULL2(classesCountPtr, classesPtr);

    env = CVMexecEnv2JniEnv(ee);

    CVM_LOADERCACHE_LOCK(ee);

    /* count the classes */
    n = 0;
    CVMloaderCacheIterate(ee, &iter);
    while (CVMloaderCacheIterateNext(ee, &iter)) {
	CVMObjectICell *loader = CVMloaderCacheIterateGetLoader(ee, &iter);
	CVMBool same;
	CVMID_icellSameObjectNullCheck(ee, initiatingLoader, loader, same);
	if (same) {
	    ++n;
	}
    }

    rc = jvmdi_Allocate(CVMint2Long(n * sizeof(jclass)),
                        (jbyte**)classesPtr);
    *classesCountPtr = n;
    c = 0;
    if (rc == JVMDI_ERROR_NONE) {
	/* fill in the classes */
	CVMloaderCacheIterate(ee, &iter);
	while (CVMloaderCacheIterateNext(ee, &iter)) {
	    CVMObjectICell *loader = CVMloaderCacheIterateGetLoader(ee, &iter);
	    CVMBool same;
	    CVMID_icellSameObjectNullCheck(ee, initiatingLoader, loader, same);
	    if (same) {
		CVMClassBlock *cb;
		CVMObjectICell *r = CVMID_getGlobalRoot(ee);
		if (r == NULL) {
		    break;
		}
		cb = CVMloaderCacheIterateGetCB(ee, &iter);
		CVMID_icellAssign(ee, r, CVMcbJavaInstance(cb));
		(*classesPtr)[c] = r;
		++c;
	    }
        }
    }

    if (c < n) {
	int i;
	for (i = 0; i < c; ++i) {
	    CVMID_freeGlobalRoot(ee, (*classesPtr)[i]);
	}
	jvmdi_Deallocate((jbyte *)*classesPtr);
	rc = JVMDI_ERROR_OUT_OF_MEMORY;
    }
    CVM_LOADERCACHE_UNLOCK(ee);

    return rc;
}

/* Get the operand stack
* Errors: JVMDI_ERROR_NULL_POINTER, JVMDI_THREAD_NOT_SUSPENDED,
* JVMDI_ERROR_INVALID_FRAMEID,
* JVMDI_ERROR_NOT_IMPLEMENTED (if not canGetOperandStack).
*/
static jvmdiError JNICALL
jvmdi_GetOperandStack(jframeID frame,  jint *operandStackSizePtr, 
                JVMDI_operand_stack_element **operandStackPtr)
{
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

/* Pop the topmost stack frame of the stack of the thread argument
* Errors: JVMDI_ERROR_INVALID_THREAD, 
* JVMDI_ERROR_THREAD_NOT_SUSPENDED, JVMDI_ERROR_NO_MORE_FRAMES
* JVMDI_ERROR_NOT_IMPLEMENTED (if not canPopFrame).
*/
static jvmdiError JNICALL
jvmdi_PopFrame(jthread thread)
{
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

/* Set the BCI.
* Errors: JVMDI_ERROR_INVALID_FRAMEID,
* JVMDI_ERROR_INVALID_LOCATION,JVMDI_THREAD_NOT_SUSPENDED,
* JVMDI_ERROR_NOT_IMPLEMENTED (if not canSetOperandStack).
*/
static jvmdiError JNICALL
jvmdi_SetFrameLocation(jframeID frame, jlocation location)
{
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

/* Set the operand stack
* Note that here, the VM can interpret the operand stack types itself
* Errors: JVMDI_ERROR_INVALID_FRAMEID,
* JVMDI_THREAD_NOT_SUSPENDED,
* JVMDI_ERROR_ILLEGAL_ARGUMENT (if operandStackSize negative or
* operand types are out of range),
* JVMDI_ERROR_NOT_IMPLEMENTED (if not canSetOperandStack).
*
*/
static jvmdiError JNICALL
jvmdi_SetOperandStack(jframeID frame, jint operandStackSize, 
                      JVMDI_operand_stack_element *operandStack)
{
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

/* Return all instances of class clazz. 
* Errors: JVMDI_ERROR_INVALID_CLASS, 
* JVMDI_ERROR_NOT_IMPLEMENTED (if not canGetHeapInfo). 
*/  
static jvmdiError JNICALL
jvmdi_AllInstances(jclass clazz, jint *instanceCountPtr, 
                   jobject **instancesPtr)
{
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

/* All references to obj 
* Errors: JVMDI_ERROR_INVALID_OBJECT, 
* JVMDI_ERROR_NOT_IMPLEMENTED (if not canGetHeapInfo).
*/
static jvmdiError JNICALL
jvmdi_References(jobject obj, JVMDI_object_reference_info *refs)
{
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

/* Return via "classDefPtr" the definition of this class.
* Memory for this array is allocated by the function specified 
* in JVMDI_SetAllocationHooks().
* Errors: JVMDI_ERROR_INVALID_CLASS,
* JVMDI_ERROR_NOT_IMPLEMENTED (if not canGetClassDefinition)
*/
static jvmdiError JNICALL
jvmdi_GetClassDefinition(jclass clazz, 
                         JVMDI_class_definition *classDefPtr)
{
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

/* Redefine the classes defined by "classDefs". "classCount" 
* specifies number of classes to redefine.
* Errors: JVMDI_ERROR_INVALID_CLASS, JVMDI_ERROR_FAILS_VERIFICATION,
* JVMDI_ERROR_INVALID_CLASS_FORMAT, JVMDI_ERROR_CIRCULAR_CLASS_DEFINITION,
* JVMDI_ERROR_SCHEMA_CHANGE_NOT_IMPLEMENTED (if not canChangeSchema)
* JVMDI_ERROR_ADD_METHOD_NOT_IMPLEMENTED (if not canAddMethod)
* JVMDI_ERROR_NOT_IMPLEMENTED (if not canRedefineClasses)
*/
static jvmdiError JNICALL
jvmdi_RedefineClasses(jint classCount, 
                      JVMDI_class_definition *classDefs)
{
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

/* Return via "versionPtr" the JVMDI version number.
 * Errors: JVMDI_ERROR_NULL_POINTER
 */
static jvmdiError JNICALL
jvmdi_GetVersionNumber(jint *versionPtr)
{
    /* 
     * We allow version queries even when debugging is not enabled 
     */
    NOT_NULL(versionPtr);
    *versionPtr = JVMDI_VERSION_1;
    return JVMDI_ERROR_NONE;
}

/* Return via "capPtr" the capabilities of this JVMDI implementation.
 * Errors: JVMDI_ERROR_NULL_POINTER,
 */
static jvmdiError JNICALL
jvmdi_GetCapabilities(JVMDI_capabilities *capPtr) 
{
    DEBUG_ENABLED();
    NOT_NULL(capPtr);

    capPtr->can_watch_field_modification = JNI_TRUE;
    capPtr->can_watch_field_access = JNI_TRUE;
    capPtr->can_get_bytecodes = JNI_FALSE;
    capPtr->can_get_synthetic_attribute = JNI_FALSE;
    capPtr->can_get_current_contended_monitor = JNI_FALSE;
#ifdef FAST_MONITOR
    capPtr->can_get_owned_monitor_info = JNI_FALSE;
    capPtr->can_get_monitor_info = JNI_FALSE;
#else
    capPtr->can_get_owned_monitor_info = JNI_TRUE;
    capPtr->can_get_monitor_info = JNI_TRUE;
#endif /* FAST_MONITOR */
    capPtr->can_get_heap_info = JNI_FALSE;
    capPtr->can_get_operand_stack = JNI_FALSE;
    capPtr->can_set_operand_stack = JNI_FALSE;
    capPtr->can_pop_frame = JNI_FALSE;
    capPtr->can_get_class_definition = JNI_FALSE;
    capPtr->can_redefine_classes = JNI_FALSE; 
    capPtr->can_add_method = JNI_FALSE;
    capPtr->can_change_schema = JNI_FALSE;

    return JVMDI_ERROR_NONE;
}

static jvmdiError JNICALL
jvmdi_GetSourceDebugExtension(jclass clazz, char **sourceDebugExtension)
{
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

static jvmdiError JNICALL
jvmdi_IsMethodObsolete(jclass clazz, jmethodID method, jboolean *isObsoletePtr)
{
    return JVMDI_ERROR_NOT_IMPLEMENTED;
}

/* sole interface instance */
static const JVMDI_Interface_1 interface = {
    jvmdi_SetEventHook,
    jvmdi_SetEventNotificationMode,
    jvmdi_GetThreadStatus,
    jvmdi_GetAllThreads,
    jvmdi_SuspendThread,
    jvmdi_ResumeThread,
    jvmdi_StopThread,
    jvmdi_InterruptThread,
    jvmdi_GetThreadInfo,
    jvmdi_GetOwnedMonitorInfo,
    jvmdi_GetCurrentContendedMonitor,
    jvmdi_RunDebugThread,
    jvmdi_GetTopThreadGroups,
    jvmdi_GetThreadGroupInfo,
    jvmdi_GetThreadGroupChildren,
    jvmdi_GetFrameCount,
    jvmdi_GetCurrentFrame,
    jvmdi_GetCallerFrame,
    jvmdi_GetFrameLocation,
    jvmdi_NotifyFramePop,
    jvmdi_GetLocalObject,
    jvmdi_GetLocalInt,
    jvmdi_GetLocalLong,
    jvmdi_GetLocalFloat,
    jvmdi_GetLocalDouble,
    jvmdi_SetLocalObject,
    jvmdi_SetLocalInt,
    jvmdi_SetLocalLong,
    jvmdi_SetLocalFloat,
    jvmdi_SetLocalDouble,

    jvmdi_CreateRawMonitor,
    jvmdi_DestroyRawMonitor,
    jvmdi_RawMonitorEnter,
    jvmdi_RawMonitorExit,
    jvmdi_RawMonitorWait,
    jvmdi_RawMonitorNotify,
    jvmdi_RawMonitorNotifyAll,

    jvmdi_SetBreakpoint,
    jvmdi_ClearBreakpoint,
    jvmdi_ClearAllBreakpoints,
    jvmdi_SetFieldAccessWatch,
    jvmdi_ClearFieldAccessWatch,
    jvmdi_SetFieldModificationWatch,
    jvmdi_ClearFieldModificationWatch,
    jvmdi_SetAllocationHooks,
    jvmdi_Allocate,
    jvmdi_Deallocate,
    jvmdi_GetClassSignature,
    jvmdi_GetClassStatus,
    jvmdi_GetSourceFileName,
    jvmdi_GetClassModifiers,
    jvmdi_GetClassMethods,
    jvmdi_GetClassFields,
    jvmdi_GetImplementedInterfaces,
    jvmdi_IsInterface,
    jvmdi_IsArrayClass,
    jvmdi_GetClassLoader,
    jvmdi_GetObjectHashCode,
    jvmdi_GetMonitorInfo,
    jvmdi_GetFieldName,
    jvmdi_GetFieldDeclaringClass,
    jvmdi_GetFieldModifiers,
    jvmdi_IsFieldSynthetic,
    jvmdi_GetMethodName,
    jvmdi_GetMethodDeclaringClass,
    jvmdi_GetMethodModifiers,
    jvmdi_GetMaxStack,
    jvmdi_GetMaxLocals,
    jvmdi_GetArgumentsSize,
    jvmdi_GetLineNumberTable,
    jvmdi_GetMethodLocation,
    jvmdi_GetLocalVariableTable,
    jvmdi_GetExceptionHandlerTable,
    jvmdi_GetThrownExceptions,
    jvmdi_GetBytecodes,
    jvmdi_IsMethodNative,
    jvmdi_IsMethodSynthetic,
    jvmdi_GetLoadedClasses,
    jvmdi_GetClassLoaderClasses,
    jvmdi_PopFrame,
    jvmdi_SetFrameLocation,
    jvmdi_GetOperandStack,
    jvmdi_SetOperandStack,
    jvmdi_AllInstances,
    jvmdi_References,
    jvmdi_GetClassDefinition,
    jvmdi_RedefineClasses,
    jvmdi_GetVersionNumber,
    jvmdi_GetCapabilities,
    jvmdi_GetSourceDebugExtension,
    jvmdi_IsMethodObsolete
};

/*
 * Retrieve the JVMDI interface pointer.  This function is called by
 * CVMjniGetEnv.
 */
JVMDI_Interface_1 *
CVMjvmdiGetInterface1(JavaVM *interfaces_vm)
{
    CVMglobals.jvmdiStatics.vm = interfaces_vm;

    return (JVMDI_Interface_1 *)&interface;
}

void
CVMjvmdiStaticsInit(struct CVMjvmdiStatics * statics)
{
    statics->jvmdiInitialized = CVM_FALSE;
    statics->vm = NULL;
    statics->eventHook = NULL;
    statics->allocHook = NULL;
    statics->deallocHook = NULL;
    statics->breakpoints = NULL;
    statics->framePops = NULL;
    statics->watchedFieldModifications = NULL;
    statics->watchedFieldAccesses = NULL;
    statics->threadList = NULL;
}

void
CVMjvmdiStaticsDestroy(struct CVMjvmdiStatics * statics)
{
    if (statics->breakpoints != NULL) {
	CVMbagDestroyBag(statics->breakpoints);
    }
    if (statics->framePops != NULL) {
	CVMbagDestroyBag(statics->framePops);
    }
    if (statics->watchedFieldModifications != NULL) {
	CVMbagDestroyBag(statics->watchedFieldModifications);
    }
    if (statics->watchedFieldAccesses != NULL) {
	CVMbagDestroyBag(statics->watchedFieldAccesses);
    }
}


#endif /* CVM_JVMDI */
