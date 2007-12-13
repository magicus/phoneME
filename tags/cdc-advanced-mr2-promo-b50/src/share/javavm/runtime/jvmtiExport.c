/*
 * @(#)jvmtiExport.c	1.7 06/10/31
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

/*
 * Breakpoints, single-stepping and debugger event notification.
 * Debugger event notification/handling includes breakpoint, single-step
 * and exceptions.
 */

/*
 * This file is derived from the original CVM jvmdi.c file.  In addition
 * the 'jvmti' components of this file are derived from the J2SE
 * jvmtiExport.cpp class.  The primary purpose of this file is to
 * export VM events to external libraries.
 */

#ifdef CVM_JVMTI

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
#include "generated/javavm/include/opcodes.h"
#include "generated/offsets/java_lang_Thread.h"
#include "generated/jni/java_lang_reflect_Modifier.h"
#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"

#include "javavm/include/jvmtiExport.h"
#include "javavm/include/jvmtiDumper.h"
#include "javavm/include/jvmtiCapabilities.h"

#ifdef CVM_JIT
#include "javavm/include/jit/jitcodebuffer.h"
#endif

jvmtiPhase jvmtiCurrentPhase;
jlong clks_per_sec;
CVMBool debuggerConnected = CVM_FALSE;

CVMUint32 jvmtiUniqueId = 0x10000;

/* %comment: k001 */


/* (This is unused in CVM -- only used in unimplemented GetBytecodes) */
/* Defined in inline.c thru an include of "opcodes.length". This is dangerous
   as is. There should be a header file with this extern shared by both .c files */
/* extern const short opcode_length[256]; */


/* Convenience macros */
#define CVM_THREAD_LOCK(ee)	  CVMsysMutexLock(ee, &CVMglobals.threadLock)
#define CVM_THREAD_UNLOCK(ee) CVMsysMutexUnlock(ee, &CVMglobals.threadLock)

/* Purpose: Checks to see if it is safe to access object pointers directly. */
#define CVMjvmtiIsSafeToAccessDirectObject(ee_)       \
    (CVMD_isgcUnsafe(ee_) || CVMgcIsGCThread(ee_) ||  \
     CVMsysMutexIAmOwner(ee_, &CVMglobals.heapLock))

/* NOTE: "Frame pop sentinels" removed in CVM version of JVMTI
   because we don't have two return PC slots and mangling the only
   available one is incorrect if an exception is thrown. See
   CVMjvmtiNotifyDebuggerOfFramePop and jvmti_NotifyFramePop below. */

/* #define FRAME_POP_SENTINEL ((unsigned char *)1) */

#define GLOBAL_ENV CVMglobals.jvmtiStatics.jvmtiEnv

#define INITIAL_BAG_SIZE 4

/* additional local frames to push to cover usage by client debugger */
#define LOCAL_FRAME_SLOP 10

#define JVMTI_EVENT_GLOBAL_MASK 0xf0000000
#define JVMTI_EVENT_THREAD_MASK 0x7fffffff

static JvmtiMethodNode *nodeByMB[HASH_SLOT_COUNT];

CVMUint32
CVMjvmtiUniqueID()
{
    return jvmtiUniqueId++;
}


static jint methodHash(CVMUint32 mid) {
    return ((mid) % HASH_SLOT_COUNT);
}

static JvmtiMethodNode *getNode(CVMMethodBlock *mb)
{
    JvmtiMethodNode *node;
    jint slot;

    slot = methodHash((CVMUint32)mb);
    node = nodeByMB[slot];
    while (node != NULL) {
	if (node->mb == mb) {
		break;
	    }
	node = node->next;
    }
    return node;
}

/* FIXME: This function is called from the interpreter loop for each method
   invocation even when JVMTI is not being used.  This is very heavy overhead
   and need to be fixed.  One option is to use some sort of bit marker on the
   mb itself to indicate that it is obsolete so as to not have to do this
   look up.  In the least, the bit can indicate if a look is necessary. */
CVMBool
CVMjvmtiMbIsObsoleteX(CVMMethodBlock *mb)
{
    JvmtiMethodNode *node;
    node = getNode(mb);
    if (node == NULL) {
	return CVM_FALSE;
    }
    return node->isObsolete;
}

CVMConstantPool *
CVMjvmtiMbConstantPool(CVMMethodBlock *mb) {
    JvmtiMethodNode *node;
    jint slot;
    slot = methodHash((CVMUint32)mb);
    node = getNode(mb);
    if (node == NULL) {
	return NULL;
    }
    CVMassert(node->isObsolete);
    return node->cp;
}

void
CVMjvmtiMarkAsObsolete(CVMMethodBlock *oldmb, CVMConstantPool *cp)
{
    JvmtiMethodNode *node;
    CVMExecEnv *ee = CVMgetEE();
    jint slot;

    JVMTI_LOCK(ee);
    node = getNode(oldmb);
    if (node == NULL) {
	slot = methodHash((CVMUint32)oldmb);
	if (CVMjvmtiAllocate(sizeof(JvmtiMethodNode),
			     (unsigned char **)&node) != JVMTI_ERROR_NONE) {
	    JVMTI_UNLOCK(ee);
	    return;
	}
	node->next = nodeByMB[slot];
	nodeByMB[slot] = node;
    }
    node->cp = cp;
    node->mb = oldmb;
    node->isObsolete = CVM_TRUE;
    JVMTI_UNLOCK(ee);
}

/* Purpose: Gets the ClassBlock from the specified java.lang.Class instance. */
CVMClassBlock *
CVMjvmtiClassInstance2ClassBlock(CVMExecEnv *ee, CVMObject *obj)
{
    CVMJavaInt cbAddr;
    CVMClassBlock *cb;

    /* The following condition will have to be true to guarantee that the
       CVMObject *obj that is passed in remains valid for the duration of this
       function, and that we can access the
       CVMoffsetOfjava_lang_Class_classBlockPointer directly below: */
    CVMassert(CVMjvmtiIsSafeToAccessDirectObject(ee));

    CVMD_fieldReadInt(obj,
        CVMoffsetOfjava_lang_Class_classBlockPointer, cbAddr);
    cb = (CVMClassBlock*) cbAddr;
    CVMassert(cb != NULL);

    return cb;
}

/*
 * This macro is used in notify_debugger_* to determine whether a JVMTI 
 * event should be generated.
 */
#define MUST_NOTIFY(ee, eventType)		\
    (CVMjvmtiEnvEventEnabled(ee, eventType))

#define MUST_NOTIFY_THREAD(ee, eventType)				\
    (MUST_NOTIFY(ee, eventType) && CVMjvmtiThreadEventEnabled(ee, eventType))

/* forward defs */
#ifdef JDK12
static void
handleExit(void);
#endif


/*
 * Initialize JVMTI - if and only if it hasn't been initialized.
 * Must be called before anything that accesses event structures.
 */
jvmtiError
CVMinitializeJVMTI() 
{
    CVMExecEnv *ee = CVMgetEE();
    CVMBool haveFailure = CVM_FALSE;

    /* %comment: k003 */
    if (CVMjvmtiEnabled()) {
	return JVMTI_ERROR_NONE;
    }

#ifdef JDK12
    CVMatExit(handleExit);
#endif

    CVMglobals.jvmtiStatics.breakpoints = 
	CVMbagCreateBag(sizeof(struct bkpt), INITIAL_BAG_SIZE);
    CVMglobals.jvmtiStatics.framePops = CVMbagCreateBag(
							sizeof(struct fpop), INITIAL_BAG_SIZE);
    CVMglobals.jvmtiStatics.watchedFieldModifications = 
	CVMbagCreateBag(sizeof(struct fieldWatch), INITIAL_BAG_SIZE);
    CVMglobals.jvmtiStatics.watchedFieldAccesses = 
	CVMbagCreateBag(sizeof(struct fieldWatch), INITIAL_BAG_SIZE);
    if (CVMglobals.jvmtiStatics.breakpoints == NULL || 
	CVMglobals.jvmtiStatics.framePops == NULL || 
	CVMglobals.jvmtiStatics.watchedFieldModifications == NULL || 
	CVMglobals.jvmtiStatics.watchedFieldAccesses == NULL) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }

    /* NOTE: We must not trigger a GC while holding the thread lock.
       We are safe here because insertThread() can allocate global roots
       which can cause expansion of the global root stack, but not cause
       a GC. */
    CVM_THREAD_LOCK(ee);

    /* Log all thread that were created prior to JVMTI's initialization: */
    /* NOTE: We are only logging the pre-existing threads into the JVMTI
       threads list.  We don't send currently send JVMTI_EVENT_THREAD_START
       for these threads that started before JVMTI was initialized. */
    CVM_WALK_ALL_THREADS(ee, currentEE, {
	    jthread thread = CVMcurrentThreadICell(currentEE);
	    if (!haveFailure && !CVMID_icellIsNull(thread)) {
		ThreadNode *node = CVMjvmtiFindThread(ee, thread);
		if (node == NULL) {
		    node = CVMjvmtiInsertThread(ee, thread);
		    if (node == NULL) {
			haveFailure = CVM_TRUE;
		    }
		}
	    }
	});

    CVM_THREAD_UNLOCK(ee);

    /* Abort if we detected a failure while trying to log threads: */
    if (haveFailure) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }
    CVMjvmtiInitializeCapabilities();
    /*    clks_per_sec = CVMgetClockTicks(); */
    return JVMTI_ERROR_NONE;
}
/*
CVMInt64 CVMjvmtiGetClockTicks() {
    return clks_per_sec;
}
*/

CVMClassBlock* CVMjvmtiGetCurrentRedefinedClass(CVMExecEnv *ee) {
    ThreadNode *node = CVMjvmtiFindThread(ee, CVMcurrentThreadICell(ee));
    CVMassert(node != NULL);
    return node->oldCb;
}

/*
 * These functions maintain the linked list of currently running threads. 
 */
ThreadNode *CVMjvmtiFindThread(CVMExecEnv* ee, CVMObjectICell* thread) {
    ThreadNode *node;
    CVMBool thrEq;

    JVMTI_LOCK(ee);

    /* cast away volatility */
    node = (ThreadNode *)CVMglobals.jvmtiStatics.threadList;  
    while (node != NULL) {
	CVMID_icellSameObject(ee, node->thread, thread, thrEq);
	if (thrEq) {
	    break;
	}
	node = node->next;
    }
    JVMTI_UNLOCK(ee);

    return node;
}

ThreadNode *CVMjvmtiInsertThread(CVMExecEnv* ee, CVMObjectICell* thread) {
    ThreadNode *node;

    /* NOTE: you could move the locking and unlocking inside the if
       clause in such a way as to avoid the problem with seizing both
       the debugger and global root lock at the same time, but it
       wouldn't solve the problem of removeThread, which is harder,
       nor the (potential) problem of the several routines which
       perform indirect memory accesses while holding the debugger
       lock; if there was ever a lock associated
       with those accesses there would be a problem. */
    JVMTI_LOCK(ee);

    node = (ThreadNode *)malloc(sizeof(*node));
    if (node != NULL) {
	memset(node, 0, sizeof(*node));
	node->thread = CVMID_getGlobalRoot(ee);
	if (node->thread == NULL) {
	    goto fail0;
	}
	node->lastDetectedException = CVMID_getGlobalRoot(ee);
	if (node->lastDetectedException == NULL) {
	    goto fail1;
	}

	CVMID_icellAssign(ee, node->thread, thread);

	/* cast away volatility */
	node->next = (ThreadNode *)CVMglobals.jvmtiStatics.threadList; 
	CVMglobals.jvmtiStatics.threadList = node;
    } 

 unlock:
    JVMTI_UNLOCK(ee);

    return node;

 fail1:
    CVMID_freeGlobalRoot(ee, node->thread);
 fail0:
    free(node);
    node = NULL;
    goto unlock;
}

jboolean 
CVMjvmtiRemoveThread(CVMObjectICell* thread) {
    ThreadNode *previous = NULL;
    ThreadNode *node; 
    CVMExecEnv* ee = CVMgetEE();
    CVMBool thrEq;
    jboolean rc = JNI_FALSE;

    JVMTI_LOCK(ee);

    /* cast away volatility */
    node = (ThreadNode *)CVMglobals.jvmtiStatics.threadList;  
    while (node != NULL) {
	CVMID_icellSameObject(ee, node->thread, thread, thrEq);
	if (thrEq) {
	    if (previous == NULL) {
		CVMglobals.jvmtiStatics.threadList = node->next;
	    } else {
		previous->next = node->next;
	    }
	    CVMID_freeGlobalRoot(ee, node->thread);
	    CVMID_freeGlobalRoot(ee, node->lastDetectedException);

	    free((void *)node);
	    rc = JNI_TRUE;
	    break;
	}
	previous = node;
	node = node->next;
    }
    JVMTI_UNLOCK(ee);

    return rc;
}

static void
checkAndSendStartEvent(CVMExecEnv *ee) {
    ThreadNode *node =
	CVMjvmtiFindThread(ee, CVMcurrentThreadICell(ee));
    if (node == NULL || node->startEventSent == CVM_FALSE) {
	/* Send thread start event */
	CVMjvmtiPostThreadStartEvent(ee, CVMcurrentThreadICell(ee));
    }
}

static void
unlock_monitors(CVMExecEnv* threadEE) {
    CVMOwnedMonitor *mon, *monNext;
    CVMFrame *currentFrame;
    CVMjvmtiLockInfo *li;
    CVMObjectICell objIcell;
    CVMObjectICell *objp = &objIcell;
    CVMExecEnv *ee =CVMgetEE();

    mon = threadEE->objLocksOwned;
    currentFrame = CVMeeGetCurrentFrame(threadEE);
    CVMassert(CVMframeIsJava(currentFrame));
    li = CVMgetJavaFrame(currentFrame)->jvmtiLockInfo;
    while (li != NULL) {
	mon = li->lock;
	monNext = mon->next;
	CVMassert(mon->state != CVM_OWNEDMON_FREE);
	if (mon->object != NULL) {
	    CVMD_gcUnsafeExec(ee, {
		    if(!CVMobjectTryUnlock(threadEE, mon->object)) {
			CVMID_icellSetDirect(ee, objp, mon->object);
			CVMobjectUnlock(threadEE, objp);
		    }
		});
	}
	/* unlocking lock removes lockinfo record */
	li = CVMgetJavaFrame(currentFrame)->jvmtiLockInfo;
    }
}

void
reportException(CVMExecEnv* ee, CVMUint8 *pc,
                CVMObjectICell* object, CVMFrame* frame) {
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMMethodBlock* exceptionMb = frame->mb;
    CVMClassBlock* exceptionCb;
    CVMJavaLong catchLocation = 0L;
    CVMMethodBlock *catchMb = NULL;
    JvmtiEnv *jvmti_env;
    jvmtiEventException callback;
    CVMFrameIterator iter;

    if (exceptionMb == NULL) {
	return;
    }

    /* NOTE: MUST BE A JAVA METHOD */
    CVMassert(!CVMmbIs(exceptionMb, NATIVE));

    CVMID_objectGetClass(ee, object, exceptionCb);
    jvmti_env = GLOBAL_ENV;
    callback = jvmti_env->eventCallbacks.Exception;
    if (callback != NULL) {
	/* walk up the stack to see if this exception is caught anywhere. */

	CVMframeIterateInit(&iter, frame);
	while (CVMframeIterateNextReflection(&iter, CVM_TRUE)) {
	    /* %comment: k004 */
	    /* skip transition frames, which can't catch exceptions */
	    if (CVMframeIterateCanHaveJavaCatchClause(&iter)) {
		catchMb = CVMframeIterateGetMb(&iter);
		/* %comment: k005 */
		if (catchMb != NULL) {
		    CVMUint8* pc = CVMframeIterateGetJavaPc(&iter);
		    CVMUint8* cpc =
			CVMgcSafeFindPCForException(ee, &iter,
						    exceptionCb,
						    pc);
		    if (cpc != NULL) {
			/* NOTE: MUST BE A JAVA METHOD */
			CVMassert(!CVMmbIs(catchMb, NATIVE));
			catchLocation = CVMint2Long(cpc - CVMmbJavaCode(catchMb));
			break;
		    } else {
			catchMb = NULL;
		    }
		}
	    }
	}
	(*callback)(&jvmti_env->jvmtiExternal, env,
		    (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
		    exceptionMb,
		    CVMint2Long(pc - CVMmbJavaCode(exceptionMb)),
		    (*env)->NewLocalRef(env, object),
		    catchMb,
		    catchLocation);
    }

}

/* FIXME: This function is being called if JVMTI is being enabled globally even
   if this event is not enabled.  This is vey heavy overhead and should be
   fixed. */
/* %comment: k006 */
void
CVMjvmtiPostExceptionEvent(CVMExecEnv* ee, CVMUint8 *pc,
			   CVMObjectICell* object)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    ThreadNode *threadNode;
    CVMBool exceptionsEqual = CVM_FALSE;

    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    /* This check is probably unnecessary */
    if (CVMcurrentThreadICell(ee) == NULL) {
	return;
    }
    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_EXCEPTION)) {
	return;
    }
    checkAndSendStartEvent(ee);
    threadNode = CVMjvmtiFindThread(ee, CVMcurrentThreadICell(ee));
#if 0
    if (threadNode == NULL) {
	/* ignore any exceptions before thread start */
	return;
    }
#endif
    CVMID_icellSameObject(ee, threadNode->lastDetectedException,
			  object,
			  exceptionsEqual);

    if (!exceptionsEqual) {

	CVMObjectICell *exceptionRef;
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);

	CVMassert(CVMID_icellIsNull(CVMmiscICell(ee)));
	if (CVMlocalExceptionOccurred(ee)) {
	    CVMID_icellAssign(ee, CVMmiscICell(ee),
			      CVMlocalExceptionICell(ee));
	    CVMclearLocalException(ee);
	} else {
	    CVMID_icellAssign(ee, CVMmiscICell(ee),
			      CVMremoteExceptionICell(ee));
	    CVMclearRemoteException(ee);
	}
	if ((*env)->PushLocalFrame(env, 5+LOCAL_FRAME_SLOP) < 0) {
	    goto cleanup;
	}

        /* OK, we now have a frame so create a local ref to hold
	   the exception */
	exceptionRef = (*env)->NewLocalRef(env, CVMmiscICell(ee));
	CVMID_icellSetNull(CVMmiscICell(ee));

	/*
	 * Save the pending exception so it does not get
	 * overwritten if CVMgcSafeFindPCForException() throws an
	 * exception. Also, in classloading builds,
	 * CVMgcSafeFindPCForException may cause loading of the
	 * exception class, which code expects no exception to
	 * have occurred upon entry.
	 */

	reportException(ee, pc, exceptionRef, frame);

	/*
	 * This needs to be a global ref; otherwise, the detected
	 * exception could be cleared and collected in a native method
	 * causing later comparisons to be invalid.
	 */
	CVMID_icellAssign(ee, threadNode->lastDetectedException, object);

	CVMID_icellAssign(ee, CVMmiscICell(ee), exceptionRef);
	(*env)->PopLocalFrame(env, 0);

    cleanup:
	CVMclearLocalException(ee);
	CVMgcSafeThrowLocalException(ee, CVMmiscICell(ee));
	CVMID_icellSetNull(CVMmiscICell(ee));
    }
}

/* FIXME: This function is being called from the interpreter loop if JVMTI is
   enabed globally even if this event is not needed.  This has to heavy
   overhead and needs to be fixed. */
/*
 * This function is called by the interpreter whenever:
 * 1) The interpreter catches an exception, or
 * 2) The interpreter detects that a native method has returned
 *    without an exception set (i.e. the native method *may* have
 *    cleared an exception; cannot tell for sure)
 *
 * This function performs 2 tasks. It removes any exception recorded
 * as the last detected exception by notify_debugger_of_exception. It
 * also reports an event to the JVMTI client. (2) above implies that 
 * we need to make sure that an exception was actually cleared before
 * reporting the event. This 
 * can be done by checking whether there is currently a last detected
 * detected exception value saved for this thread.
 */
void
CVMjvmtiPostExceptionCatchEvent(CVMExecEnv* ee, CVMUint8 *pc,
				CVMObjectICell* object)
{
    ThreadNode *threadNode;
    JvmtiEnv *jvmti_env;
    jvmtiEventExceptionCatch callback;
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMFrame* frame;
    CVMMethodBlock* mb;
    
    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    /* This check is probably unnecessary */
    if (CVMcurrentThreadICell(ee) == NULL) {
	return;
    }

    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_EXCEPTION_CATCH)) {
	return;
    }
    checkAndSendStartEvent(ee);
    threadNode = CVMjvmtiFindThread(ee, CVMcurrentThreadICell(ee));
#if 0
    if (threadNode == NULL) {
	/* ignore any exceptions before thread start */
	return;
    }
#endif
    frame = CVMeeGetCurrentFrame(ee);
    mb = frame->mb;

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

	jvmti_env = GLOBAL_ENV;
	callback = jvmti_env->eventCallbacks.ExceptionCatch;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			mb,
			CVMint2Long(pc - CVMmbJavaCode(mb)),
			(*env)->NewLocalRef(env, object));
	}

	(*env)->PopLocalFrame(env, 0);
    }
    CVMID_icellSetNull(threadNode->lastDetectedException);
}

void
CVMjvmtiPostSingleStepEvent(CVMExecEnv* ee, CVMUint8 *pc)
{
    /*
     * The interpreter notifies us only for threads that have stepping enabled,
     * so we don't do any checks of the global or per-thread event
     * enable flags here.
     */
    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);

    if (CVMjvmtiEnabled()) {
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	CVMMethodBlock* mb = frame->mb;
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventSingleStep callback;

	if (CVMframeIsTransition(frame) || mb == NULL) {
	    return;
	}

	/* NOTE: MUST BE A JAVA METHOD */
	CVMassert(!CVMmbIs(mb, NATIVE));
	CVMtraceJVMTI(("JVMTI: Post Step: ee 0x%x\n", (int)ee));
	if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	jvmti_env = GLOBAL_ENV;
	callback = jvmti_env->eventCallbacks.SingleStep;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			mb,
			CVMint2Long(pc - CVMmbJavaCode(mb)));
	}

	(*env)->PopLocalFrame(env, 0);
    }
    if (CVMjvmtiNeedFramePop(ee)) {
	unlock_monitors(ee);
    }
}

void
notify_debugger_of_breakpoint(CVMExecEnv* ee, CVMUint8 *pc)
{

    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_BREAKPOINT)) {
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	CVMMethodBlock* mb = frame->mb;
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventBreakpoint callback;

	if (mb == NULL) {
	    return;
	}
	/* NOTE: MUST BE A JAVA METHOD */
	CVMassert(!CVMmbIs(mb, NATIVE));
	if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	jvmti_env = GLOBAL_ENV;
	callback = jvmti_env->eventCallbacks.Breakpoint;
	if (callback != NULL) {
	    CVMtraceJVMTI(("JVMTI: Post breakpoint: ee 0x%x\n",
			   (int)ee));
	    checkAndSendStartEvent(ee);
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			mb,
			CVMint2Long(pc - CVMmbJavaCode(mb)));
	}
  
	(*env)->PopLocalFrame(env, 0);
    }
    if (CVMjvmtiNeedFramePop(ee)) {
	unlock_monitors(ee);
    }
}

/* Initializes some JVMTI flags for the thread and posts a THREAD_START 
 * event if appropriate. */
void
CVMjvmtiPostThreadStartEvent(CVMExecEnv* ee, CVMObjectICell* thread)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    JvmtiEnv *jvmti_env = GLOBAL_ENV;
    jvmtiEventThreadStart callback;

    /*
     * Look for existing thread info for this thread. If there is 
     * a ThreadNode already, it just means that it's a debugger thread
     * started by jvmti_RunDebugThread; if not, we create the ThreadNode
     * here. 
     */
    ThreadNode *node = CVMjvmtiFindThread(ee, thread);
    if (node == NULL) {
	node = CVMjvmtiInsertThread(ee, thread);
	if (node == NULL) {
	    (*env)->FatalError(env, "internal allocation error in JVMTI");
	}
    }

    CVMjvmtiRecomputeThreadEnabled(ee, &jvmti_env->envEventEnable);
    CVMJVMTI_CHECK_PHASEV2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    if (MUST_NOTIFY(ee, JVMTI_EVENT_THREAD_START)) {
	if ((*env)->PushLocalFrame(env, 1+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	callback = jvmti_env->eventCallbacks.ThreadStart;
	if (callback != NULL) {
	    CVMtraceJVMTI(("JVMTI: Post thread start: ee 0x%x\n",
			   (int)ee));
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)));
	    node->startEventSent = CVM_TRUE;
	}
	
	(*env)->PopLocalFrame(env, 0);
    }
}

void
CVMjvmtiPostThreadEndEvent(CVMExecEnv* ee, CVMObjectICell* thread)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMJVMTI_CHECK_PHASEV2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    if (MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_THREAD_END)) {
	JvmtiEnv *jvmti_env;
	jvmtiEventThreadEnd callback;

	if ((*env)->PushLocalFrame(env, 1+LOCAL_FRAME_SLOP) < 0) {
	    goto forgetThread;
	}
	jvmti_env = GLOBAL_ENV;
	callback = jvmti_env->eventCallbacks.ThreadEnd;
	if (callback != NULL) {
	    CVMtraceJVMTI(("JVMTI: Post thread end: ee 0x%x\n", (int)ee));
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)));
	}
	(*env)->PopLocalFrame(env, 0);
    }

 forgetThread:
    CVMjvmtiRemoveThread(thread);
}

void
CVMjvmtiPostFieldAccessEvent(CVMExecEnv* ee, CVMObjectICell* obj,
			     CVMFieldBlock* fb)
{
    struct fieldWatch *fwfb;

    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    JVMTI_LOCK(ee);
    fwfb = CVMbagFind(CVMglobals.jvmtiStatics.watchedFieldAccesses, fb);
    JVMTI_UNLOCK(ee);          

    if (fwfb != NULL &&
	MUST_NOTIFY(ee, JVMTI_EVENT_FIELD_ACCESS)) {
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	CVMMethodBlock* mb;
	jlocation location;
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventFieldAccess callback;

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
	jvmti_env = GLOBAL_ENV;
	callback = jvmti_env->eventCallbacks.FieldAccess;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			mb, location,
			(*env)->NewLocalRef(env,
					    CVMcbJavaInstance(CVMfbClassBlock(fb))),
			(obj == NULL ? NULL :
			 (*env)->NewLocalRef(env, obj)), fb);
	}
	(*env)->PopLocalFrame(env, 0);
    }
    if (CVMjvmtiNeedFramePop(ee)) {
	unlock_monitors(ee);
    }
}

void
CVMjvmtiPostFieldModificationEvent(CVMExecEnv* ee, CVMObjectICell* obj,
				   CVMFieldBlock* fb, jvalue jval)
{
    struct fieldWatch *fwfb;

    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    JVMTI_LOCK(ee);
    fwfb = CVMbagFind(CVMglobals.jvmtiStatics.watchedFieldModifications, fb);
    JVMTI_UNLOCK(ee);          

    if (fwfb != NULL &&
	MUST_NOTIFY(ee, JVMTI_EVENT_FIELD_MODIFICATION)) {
	CVMFrame* frame = CVMeeGetCurrentFrame(ee);
	CVMMethodBlock* mb;
	jlocation location;
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	CVMFieldTypeID tid = CVMfbNameAndTypeID(fb);
	char sig_type;
	JvmtiEnv *jvmti_env;
	jvmtiEventFieldModification callback;

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
	    location = CVMlongConstZero();
	}

	if ((*env)->PushLocalFrame(env, 5+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	if ((sig_type == CVM_SIGNATURE_CLASS) ||
	    (sig_type == CVM_SIGNATURE_ARRAY)) {
	    jval.l = (*env)->NewLocalRef(env, jval.l);
	}

	jvmti_env = GLOBAL_ENV;
	callback = jvmti_env->eventCallbacks.FieldModification;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			mb, location,
			(*env)->NewLocalRef(env, CVMcbJavaInstance(CVMfbClassBlock(fb))),
			(obj == NULL? NULL : 
			 (*env)->NewLocalRef(env, obj)),
			fb, sig_type, jval);
	}
	(*env)->PopLocalFrame(env, 0);
    }
    if (CVMjvmtiNeedFramePop(ee)) {
	unlock_monitors(ee);
    }
}

void
CVMjvmtiPostClassLoadEvent(CVMExecEnv* ee, CVMObjectICell* clazz)
{
    CVMJVMTI_CHECK_PHASEV2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    if (MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_CLASS_LOAD)) {
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventClassLoad callback;

	if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	jvmti_env = GLOBAL_ENV;
	callback = jvmti_env->eventCallbacks.ClassLoad;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			(*env)->NewLocalRef(env, clazz));
	}
	(*env)->PopLocalFrame(env, 0);
    }
}


void
CVMjvmtiPostClassPrepareEvent(CVMExecEnv* ee, CVMObjectICell* clazz)
{
    CVMJVMTI_CHECK_PHASEV2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    if (MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_CLASS_PREPARE)) {
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventClassPrepare callback;

	if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	jvmti_env = GLOBAL_ENV;
	callback = jvmti_env->eventCallbacks.ClassPrepare;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)), 
			(*env)->NewLocalRef(env, clazz));
	}
	(*env)->PopLocalFrame(env, 0);
    }
}

void
reportFrameEvent(CVMExecEnv* ee, jint kind, CVMFrame *targetFrame,
		 CVMBool is_exception, jvalue *value)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMObjectICell* thread = CVMcurrentThreadICell(ee);
    CVMMethodBlock* mb = targetFrame->mb;
    JvmtiEnv *jvmti_env;

    if (mb == NULL) {
	return;
    }

    jvmti_env = GLOBAL_ENV;
    switch(kind) {
    case JVMTI_EVENT_FRAME_POP:
	{
	    jvmtiEventFramePop callback;
	    callback = jvmti_env->eventCallbacks.FramePop;
	    if (callback != NULL) {
		(*callback)(&jvmti_env->jvmtiExternal, env,
			    (*env)->NewLocalRef(env, thread),
			    mb, is_exception);
	    }
	}
	break;
    case JVMTI_EVENT_METHOD_ENTRY:
	{
	    jvmtiEventMethodEntry callback;
	    callback = jvmti_env->eventCallbacks.MethodEntry;
	    if (callback != NULL) {
		(*callback)(&jvmti_env->jvmtiExternal, env,
			    (*env)->NewLocalRef(env, thread),
			    mb);
	    }
	}
	break;
    case JVMTI_EVENT_METHOD_EXIT:
	{
	    /* NOTE: need to pass return value */
	    jvmtiEventMethodExit callback;
	    callback = jvmti_env->eventCallbacks.MethodExit;
	    if (callback != NULL) {
		(*callback)(&jvmti_env->jvmtiExternal, env,
			    (*env)->NewLocalRef(env, thread),
			    mb, is_exception, *value);
	    }
	}
	break;
    }

}

void
CVMjvmtiPostFramePopEvent(CVMExecEnv* ee, CVMBool isRef,
			  CVMBool is_exception, jvalue *retValue)
{
    CVMFrame* frame = CVMeeGetCurrentFrame(ee);
    CVMFrame* framePrev;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    /* %comment: k008 */

    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (frame == NULL) {
	return;
    }

    if (!MUST_NOTIFY(ee, JVMTI_EVENT_FRAME_POP) &&
	!MUST_NOTIFY(ee, JVMTI_EVENT_METHOD_EXIT)) {
	return;
    }

    checkAndSendStartEvent(ee);
    framePrev = CVMframePrev(frame);

    if (framePrev == NULL) {
	return;
    }

    /* NOTE: This is needed by the code below, but wasn't
       present in the 1.2 sources */
    if (!CVMframeIsInterpreter(framePrev)) {
	return;
    }

    if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	return;
    }

    if (isRef) {
	/* return value was stashed in ee->miscICell */
	retValue->l = (*env)->NewLocalRef(env, ee->miscICell);
    }

    /* NOTE: the JDK has two slots for the return PC; one is the
       "lastpc" and one is the "returnpc". The JDK's version of the
       JVMTI inserts a "frame pop sentinel" as the returnpc in
       jvmti_NotifyFramePop. The munging of the return PC is
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
    if (MUST_NOTIFY(ee, JVMTI_EVENT_FRAME_POP)) {
	struct fpop *fp = NULL;
	CVMBool gotFramePop = CVM_FALSE;

	JVMTI_LOCK(ee);
	/* It seems that this gets called before JVMTI is initialized */
	if (CVMglobals.jvmtiStatics.framePops != NULL) {
	    fp = CVMbagFind(CVMglobals.jvmtiStatics.framePops, frame);
	}

	if (fp != NULL) {
	    /* Found a frame pop */
	    /* fp now points to randomness */
	    CVMbagDelete(CVMglobals.jvmtiStatics.framePops, fp); 
	    gotFramePop = CVM_TRUE;
	}
	JVMTI_UNLOCK(ee);

	if (gotFramePop) {
	    if (!CVMexceptionOccurred(ee) &&
		MUST_NOTIFY(ee, JVMTI_EVENT_FRAME_POP)) {
		reportFrameEvent(ee, JVMTI_EVENT_FRAME_POP, frame,
				 is_exception, retValue);
	    }
	}
    }
    if (!CVMexceptionOccurred(ee) && 
	MUST_NOTIFY(ee, JVMTI_EVENT_METHOD_EXIT)) {
	reportFrameEvent(ee, JVMTI_EVENT_METHOD_EXIT, frame, is_exception,
			 retValue);
    }
    (*env)->PopLocalFrame(env, 0);
}


/* FIXME: This is being called unconditionally from the interpreter loop even
   when JVMTI is not in use.  This is very heavy overhead and need to be fixed.
   JVMTI should provide some bit flags that can be checked to avoid calling
   this function in unnecessary conditons. */
void
CVMjvmtiPostFramePushEvent(CVMExecEnv* ee)
{
    CVMFrame* frame = CVMeeGetCurrentFrame(ee);
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    jvalue val;
    val.j = CVMlongConstZero();
    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if ((*env)->PushLocalFrame(env, 2+LOCAL_FRAME_SLOP) < 0) {
	return;
    }
    if (MUST_NOTIFY(ee, JVMTI_EVENT_METHOD_ENTRY)) {
	checkAndSendStartEvent(ee);
	reportFrameEvent(ee, JVMTI_EVENT_METHOD_ENTRY, frame, CVM_FALSE, &val);
    }
    (*env)->PopLocalFrame(env, 0);
}

/*
 * This function is called near the end of the VM initialization process.
 * It triggers the sending of an event to the JVMTI client. At this point
 * The JVMTI client is free to use all of JNI and JVMTI.
 */
void
CVMjvmtiPostVmStartEvent(CVMExecEnv* ee)
{    
    JvmtiEnv *jvmti_env;
    jvmtiEventVMStart callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    CVMJVMTI_CHECK_PHASEV2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    jvmti_env = GLOBAL_ENV;
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.VMStart;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, env);
	}
    }
}

/*
 * This function is called at the end of the VM initialization process.
 * It triggers the sending of an event to the JVMTI client. At this point
 * The JVMTI client is free to use all of JNI and JVMTI.
 */
void
CVMjvmtiPostVmInitEvent(CVMExecEnv* ee)
{    
    JvmtiEnv *jvmti_env;
    jvmtiEventVMInit callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if ((*env)->PushLocalFrame(env, 1+LOCAL_FRAME_SLOP) < 0) {
	return;
    }
    jvmti_env = GLOBAL_ENV;
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.VMInit;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)));
	}
    }
    (*env)->PopLocalFrame(env, 0);
}

void
CVMjvmtiPostVmExitEvent(CVMExecEnv* ee)
{    
    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (CVMjvmtiEnabled()) {
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	JvmtiEnv *jvmti_env;
	jvmtiEventVMDeath callback;

	if ((*env)->PushLocalFrame(env, 0+LOCAL_FRAME_SLOP) < 0) {
	    return;
	}
	jvmti_env = GLOBAL_ENV;
	callback = jvmti_env->eventCallbacks.VMDeath;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, env);
	}
	(*env)->PopLocalFrame(env, 0);
    }
}

#ifdef JDK12

void
handleExit(void)
{
    CVMjvmtiPostVmExitEvent(CVMgetEE());
}

#endif

/**
 * Return the underlying opcode at the specified address.
 * Notify of the breakpoint, if it is still there and 'notify' is true.
 * For use outside jvmti.
 */
CVMUint8
CVMjvmtiGetBreakpointOpcode(CVMExecEnv* ee, CVMUint8 *pc, CVMBool notify)
{
    struct bkpt *bp;
    int rv;

    JVMTI_LOCK(ee);
    bp = CVMbagFind(CVMglobals.jvmtiStatics.breakpoints, pc);
    if (bp == NULL) {
	rv = *pc;  
    } else {
	rv = bp->opcode;
    }
    JVMTI_UNLOCK(ee);
    
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
CVMjvmtiSetBreakpointOpcode(CVMExecEnv* ee, CVMUint8 *pc, CVMUint8 opcode)
{
    struct bkpt *bp;

    CVMassert(JVMTI_IS_LOCKED(ee));
    bp = CVMbagFind(CVMglobals.jvmtiStatics.breakpoints, pc);
    CVMassert(bp != NULL);
    bp->opcode = opcode;
    return CVM_TRUE;
}

/*  Profiling support */

/* Purpose: Posts a JVMTI_CLASS_LOAD_HOOK event. */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostClassLoadHookEvent(jclass klass, CVMClassLoaderICell *loader,
				    const char *class_name,
				    jobject protection_domain,
				    CVMInt32 bufferLength, CVMUint8 *buffer,
				    CVMInt32 *new_bufferLength,
				    CVMUint8 **new_buffer)
{
    CVMExecEnv *ee = CVMgetEE();
    jvmtiEventClassFileLoadHook callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    JvmtiEnv *jvmti_env;

    if (CVMjvmtiGetPhase() != JVMTI_PHASE_PRIMORDIAL &&
	CVMjvmtiGetPhase() != JVMTI_PHASE_ONLOAD &&
	CVMjvmtiGetPhase() != JVMTI_PHASE_LIVE) {
	return;
    }
    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(!ee->hasPostedExitEvents);


    if ((*env)->PushLocalFrame(env, 1+LOCAL_FRAME_SLOP) < 0) {
	return;
    }
    /* Fill event info and notify the profiler: */
    jvmti_env = GLOBAL_ENV;
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.ClassFileLoadHook;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			klass, loader,
			class_name, protection_domain,
			bufferLength, buffer,
			new_bufferLength, new_buffer);
	}
    }
    (*env)->PopLocalFrame(env, 0);

}

/* Purpose: Posts a JVMTI_EVENT_COMPILED_METHOD_LOAD event. */
/* IAI-07: Notify JVMTI of compilations and decompilations. */
void CVMjvmtiPostCompiledMethodLoadEvent(CVMExecEnv *ee, CVMMethodBlock *mb)
{
#ifndef CVM_JIT
    CVMassert(CVM_FALSE);
#else
    jvmtiEventCompiledMethodLoad callback;
    CVMCompiledMethodDescriptor *cmd = CVMmbCmd(mb);
    JvmtiEnv *jvmti_env = GLOBAL_ENV;

#ifdef CVM_DEBUG_CLASSINFO
    int i;
    CVMCompiledPcMap * maps = CVMcmdCompiledPcMaps(cmd);
    int map_entries;
    jvmtiAddrLocationMap *addrMap = NULL;
    jvmtiError err;
    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);

    /* cmd: javapc<->compiled pc */
    map_entries = CVMcmdNumPcMaps(cmd);
    if (map_entries <= 0) {
        goto fillfields;
    }

    if (jvmti_env == NULL) {
	goto failed;
    }
    err = CVMjvmtiAllocate(map_entries * sizeof(jvmtiAddrLocationMap),
			   (unsigned char **)&addrMap);
    if (err != JVMTI_ERROR_NONE) {
        goto failed;
    }

    for (i = 0; i < map_entries; i++) {
        addrMap[i].start_address = CVMcmdCodeBufAddr(cmd) + maps[i].compiledPc;
	addrMap[i].location = maps[i].javaPc;
    }

#else
    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
#endif /* CVM_DEBUG_CLASSINFO */

 fillfields:
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.CompiledMethodLoad;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, mb,
			CVMcmdCodeBufSize(cmd), CVMcmdCodeBufAddr(cmd),
			map_entries, addrMap, NULL);
	}
    }

 doCleanUpAndExit:
    if (addrMap != NULL) {
	CVMjvmtiDeallocate((unsigned char *)addrMap);
    }
    return;

 failed:
    goto doCleanUpAndExit;
#endif /* CVM_JIT */
}

/* Purpose: Posts a JVMTI_EVENT_COMPILED_METHOD_UNLOAD event. */
/* IAI-07: Notify JVMTI of compilations and decompilations. */
void CVMjvmtiPostCompiledMethodUnloadEvent(CVMExecEnv *ee, CVMMethodBlock* mb)
{
#ifndef CVM_JIT
    CVMassert(CVM_FALSE);
#else
    jvmtiEventCompiledMethodUnload callback;
    JvmtiEnv *jvmti_env = GLOBAL_ENV;
    CVMCompiledMethodDescriptor *cmd = CVMmbCmd(mb);
    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);

    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.CompiledMethodUnload;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, mb,
			CVMcmdCodeBufAddr(cmd));
	}
    }
#endif /* CVM_JIT */
}

/* Purpose: Posts the JVMTI_EVENT_DATA_DUMP_REQUEST event. */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostDataDumpRequest(void)
{
    jvmtiEventDataDumpRequest callback;
    JvmtiEnv *jvmti_env = GLOBAL_ENV;
    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);

    CVMjvmtiResetDataDumpRequested();
    if (CVMjvmtiShouldPostDataDump()) {
	if (jvmti_env != NULL) {
	    callback = jvmti_env->eventCallbacks.DataDumpRequest;
	    if (callback != NULL) {
		(*callback)(&jvmti_env->jvmtiExternal);
	    }
	}
    }
}

/*
 * Purpose: Posts the JVMTI_EVENT_GARBAGE_COLLECTION_START event just before
 * GC starts.
 * NOTE: Called while GC safe before GC thread requests all threads to be
 * safe.
 */
void CVMjvmtiPostGCStartEvent(void)
{
    jvmtiEventGarbageCollectionStart callback;
    JvmtiEnv *jvmti_env = GLOBAL_ENV;
    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);

    CVMassert(!CVMgcIsGCThread(CVMgetEE()));
    CVMassert(CVMD_isgcSafe(CVMgetEE()));
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.GarbageCollectionStart;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal);
	}
    }
}

/*
 * Purpose: Posts the JVMTI_EVENT_GARBAGE_COLLECTION_FINISH event just after
 * GC finishes.
 * NOTE: Called while GC safe before GC thread allows all threads to become
 * unsafe again.
 */
void CVMjvmtiPostGCFinishEvent(void)
{
    jvmtiEventGarbageCollectionFinish callback;
    JvmtiEnv *jvmti_env = GLOBAL_ENV;

    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    CVMassert(CVMgcIsGCThread(CVMgetEE()));
    CVMassert(CVMD_isgcSafe(CVMgetEE()));
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.GarbageCollectionFinish;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal);
	}
    }
}

#ifdef CVM_CLASSLIB_JCOV
/*
 * Purpose: Posts a JVMTI_EVENT_CLASS_LOAD_HOOK for a preloaded class.
 * NOTE: This function is used to simulate loading of classes that were
 *       preloaded.  If exceptions are encountered while doing this, the
 *       exceptions will be cleared and discarded.
 */
static void
CVMjvmtiPostStartupClassLoadHookEvents(CVMExecEnv *ee,
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

    if (CVMjvmtiShouldPostClassFileLoadHook()) {
        CVMUint8 *buffer = (CVMUint8 *)classfile;
        CVMUint32 bufferSize = classfileSize;
	CVMUint8 *new_buffer = NULL;
	CVMInt32 new_bufferLength = 0;

        CVMjvmtiPostClassLoadHookEvent(NULL, classname, bufferSize, buffer,
                                       &new_bufferLength, &new_buffer);

        /* If the new_buffer != NULL, then the profiler agent must have
           replaced it with an instrumented version.  We should clean this up
           properly even though we can't use it.
        */
        if (new_buffer != NULL) {
	    JvmtiEnv *jvmti_env = GLOBAL_ENV;
            CVMjvmtiDeallocate(buffer);
        }
    }

    /* Clean-up resources and return: */
    CVMclassReleaseSystemClassFile(ee, classfile);
    free(classname);
}
#endif

/* Purpose: Posts a JVMTI_EVENT_CLASS_LOAD for a missed class. */
static void
CVMjvmtiPostStartupClassEvents(CVMExecEnv *ee, CVMClassBlock *cb, void *data)
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
    if (CVMjvmtiShouldPostClassFileLoadHook()) {
        CVMjvmtiPostStartupClassLoadHookEvents(ee, cb,
                                               &CVMglobals.bootClassPath);
        /* If an exception occurred during event notification, then abort.
           We cannot go on with more event notifications: */
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }
#endif
    /* NOTE: need to see if CVMisArrayOfAnyBasicType() can be changed
     * to CVMisArrayClass()
     */
    if (CVMjvmtiShouldPostClassLoad() && !CVMisArrayOfAnyBasicType(cb)) {
        CVMjvmtiPostClassLoadEvent(ee, CVMcbJavaInstance(cb));
    }
}

/* Purpose: Posts a JVMTI_EVENT_VM_OBJECT_ALLOC event. */
/* NOTE: Called while GC safe. */
static void CVMjvmtiPostVMObjectAllocEvent(CVMExecEnv *ee,
					   CVMObject *obj)
                                          
{
    const CVMClassBlock *cb;
    CVMUint32 size;
    jvmtiEventVMObjectAlloc callback;
    JvmtiEnv *jvmti_env = GLOBAL_ENV;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    jobject thisObj;
    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);

    CVMassert(CVMD_isgcSafe(ee));

    if (obj == NULL) {
        return;
    }
    cb = CVMobjectGetClass(obj);
    size = CVMobjectSizeGivenClass(obj, cb);

    if (jvmti_env != NULL) {
	if ((*env)->PushLocalFrame(env, 3) < 0) {
	    return;
	}
	CVMID_localrootBegin(ee); {
	    CVMID_localrootDeclare(CVMObjectICell, tempObj);
	    CVMD_gcUnsafeExec(ee, {
		    CVMID_icellSetDirect(ee, tempObj, obj);
		});
	    thisObj = (*env)->NewLocalRef(env, tempObj);
	} CVMID_localrootEnd();
	callback = jvmti_env->eventCallbacks.VMObjectAlloc;
	if (callback != NULL) {
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			thisObj,
			(*env)->NewLocalRef(env, CVMcbJavaInstance(cb)),
			size);
	}
	(*env)->PopLocalFrame(env, 0);
    }
}

/* Purpose: Posts a JVMTI_EVENT_VM_OBJECT_ALLOC for a missed object. */
static CVMBool
CVMjvmtiPostStartupObjectEvents(CVMObject *obj, CVMClassBlock *cb,
                                CVMUint32 size, void *data)
{
    if (CVMobjectGetClass(obj) != CVMsystemClass(java_lang_Class)) {
        if (CVMjvmtiShouldPostVmObjectAlloc()) {
            CVMExecEnv *ee = (CVMExecEnv *)data;
            CVMjvmtiPostVMObjectAllocEvent(ee, obj);
            /* If an exception occurred during event notification, then abort.
               We cannot go on with more event notifications: */
            if (CVMexceptionOccurred(ee)) {
                return CVM_FALSE; /* Abort scan. */
            }
        }
    }
    return CVM_TRUE; /* OK to continue scan. */
}

/*
 * Purpose: Posts startup events required by JVMTI.  This includes defining
 * events which transpired prior to the completion of VM
 * initialization.
 * NOTE: Called while GC safe.
 */
void CVMjvmtiPostStartUpEvents(CVMExecEnv *ee)
{
    CVMObjectICell *threadICell = CVMcurrentThreadICell(ee);

    /* NOTE: It is not trivial to reconstruct the order in which all the
       defining events prior to this point has occurred.  Also, these events
       don't exist for preloaded objects.  The following is a gross
       approximation which will hopefully lessen the need missed events.
       This is a temporary measure until a need for a more comprehensive
       solution is found, or until a way to compute the order is found. */

    CVMassert(CVMD_isgcSafe(ee));

    /* 1. Post the current Thread Start Event first because all events will
       make reference to this thread: */
    if (CVMjvmtiShouldPostThreadLife()) {
	CVMjvmtiPostThreadStartEvent(ee, threadICell);
	if (CVMexceptionOccurred(ee)) {
	    return;
	}
    }

    /* 2. Post Class Load Events: */
    if (CVMjvmtiShouldPostClassLoad()
#ifdef CVM_CLASSLIB_JCOV
        || CVMjvmtiShouldPostClassFileLoadHook()
#endif
        ) {
        /* Posts a JVMTI_EVENT_CLASS_LOAD event for each ROMized class: */
        CVMclassIterateAllClasses(ee, CVMjvmtiPostStartupClassEvents, 0);
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }

    /* 3. Post Object Allocation Events: */
    if (CVMjvmtiShouldPostVmObjectAlloc()) {
#ifdef CVM_JIT
	CVMsysMutexLock(ee, &CVMglobals.jitLock);
#endif
        CVMsysMutexLock(ee, &CVMglobals.heapLock);
	CVMpreloaderIteratePreloadedObjects(ee,
					    CVMjvmtiPostStartupObjectEvents, (void *)ee);
	if (!CVMexceptionOccurred(ee)) {
	    CVMgcimplIterateHeap(ee, CVMjvmtiPostStartupObjectEvents,
				 (void *)ee);
	}
        CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
#ifdef CVM_JIT
	CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }

    /* 4. Post other Thread Start Events: */
    if (CVMjvmtiShouldPostThreadLife()) {
	/* try to avoid holding a lock across an event */
	int count = 0;
	int i;
	CVMObjectICell **threads;
#ifdef CVM_JIT
	CVMsysMutexLock(ee, &CVMglobals.jitLock);
#endif
        CVMsysMutexLock(ee, &CVMglobals.heapLock);
        CVMsysMutexLock(ee, &CVMglobals.threadLock);
        CVM_WALK_ALL_THREADS(ee, threadEE, {
		if (ee != threadEE && !CVMexceptionOccurred(ee)) {
		    count++;
		}
	    });
	if (CVMjvmtiAllocate(count * sizeof(jthread),
			     (unsigned char **)&threads) != JVMTI_ERROR_NONE) {
	    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
	    CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
#ifdef CVM_JIT
	    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif
	    return;
	}
	i = 0;
        CVM_WALK_ALL_THREADS(ee, threadEE, {
		if (ee != threadEE && !CVMexceptionOccurred(ee)) {
		    threads[i++] = CVMcurrentThreadICell(threadEE);
		}
	    });
        CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
        CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
#ifdef CVM_JIT
	CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif
	for (i = 0; i < count; i++) {
	    CVMjvmtiPostThreadStartEvent(ee, threads[i]);
	}
        if (CVMexceptionOccurred(ee)) {
            return;
        }
    }
}

void
CVMjvmtiPostNativeMethodBind(CVMExecEnv* ee, CVMMethodBlock *mb,
			     CVMUint8 *nativeCode, CVMUint8 **new_nativeCode)
{    
    JvmtiEnv *jvmti_env;
    jvmtiEventNativeMethodBind callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    if (CVMjvmtiGetPhase() != JVMTI_PHASE_PRIMORDIAL &&
	CVMjvmtiGetPhase() != JVMTI_PHASE_ONLOAD &&
	CVMjvmtiGetPhase() != JVMTI_PHASE_LIVE) {
	return;
    }
    jvmti_env = GLOBAL_ENV;
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.NativeMethodBind;
	if (callback != NULL) {
	    if ((*env)->PushLocalFrame(env, 1) < 0) {
		return;
	    }
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			mb, nativeCode,
			(void **)new_nativeCode);
	    (*env)->PopLocalFrame(env, 0);
	}
    }
}

/* Purpose: Posts a JVMTI_EVENT_MONITOR_CONTENDED_ENTER */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostMonitorContendedEnterEvent(CVMExecEnv *ee,
                                            CVMProfiledMonitor *pm)
{
    JvmtiEnv *jvmti_env;
    jvmtiEventMonitorContendedEnter callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    jobject monitor_object;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));
    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);

    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_MONITOR_CONTENDED_ENTER)) {
	return;
    }
    jvmti_env = GLOBAL_ENV;
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.MonitorContendedEnter;
	if (callback != NULL) {
	    CVMObjMonitor *mon = CVMcastProfiledMonitor2ObjMonitor(pm);
	    if ((*env)->PushLocalFrame(env, 2) < 0) {
		return;
	    }
	    CVMID_localrootBegin(ee); {
		CVMID_localrootDeclare(CVMObjectICell, tempObj);
		CVMD_gcUnsafeExec(ee, {
			CVMID_icellSetDirect(ee, tempObj, mon->obj);
		    });
		monitor_object = (*env)->NewLocalRef(env, tempObj);
	    } CVMID_localrootEnd();

	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			monitor_object);
	    (*env)->PopLocalFrame(env, 0);
	}
    }
}

/* Purpose: Posts a  JVMTI_EVENT_MONITOR_CONTENDED_ENTERED event. */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostMonitorContendedEnteredEvent(CVMExecEnv *ee,
                                              CVMProfiledMonitor *pm)
{
    JvmtiEnv *jvmti_env;
    jvmtiEventMonitorContendedEntered callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    jobject monitor_object;

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED)) {
	return;
    }
    jvmti_env = GLOBAL_ENV;
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.MonitorContendedEntered;
	if (callback != NULL) {
	    CVMObjMonitor *mon = CVMcastProfiledMonitor2ObjMonitor(pm);
	    if ((*env)->PushLocalFrame(env, 2) < 0) {
		return;
	    }
	    CVMID_localrootBegin(ee); {
		CVMID_localrootDeclare(CVMObjectICell, tempObj);
		CVMD_gcUnsafeExec(ee, {
			CVMID_icellSetDirect(ee, tempObj, mon->obj);
		    });
		monitor_object = (*env)->NewLocalRef(env, tempObj);
	    } CVMID_localrootEnd();
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			monitor_object);
	    (*env)->PopLocalFrame(env, 0);
	}
    }
}

/* Purpose: Posts a JVMTI_EVENT_MONITOR_WAIT_EVENT */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostMonitorWaitEvent(CVMExecEnv *ee,
				  jobject obj, jlong millis)
{
    JvmtiEnv *jvmti_env;
    jvmtiEventMonitorWait callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_MONITOR_WAIT)) {
	return;
    }
    jvmti_env = GLOBAL_ENV;
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.MonitorWait;
	if (callback != NULL) {
	    if ((*env)->PushLocalFrame(env, 2) < 0) {
		return;
	    }
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			(*env)->NewLocalRef(env, obj), millis);
	    (*env)->PopLocalFrame(env, 0);
	}
    }
}

/* Purpose: Posts a JVMTI_EVENT_MONITOR_WAITED_EVENT */
/* NOTE: Called while GC safe. */
void CVMjvmtiPostMonitorWaitedEvent(CVMExecEnv *ee,
				  jobject obj, CVMBool timedout)
{
    JvmtiEnv *jvmti_env;
    jvmtiEventMonitorWaited callback;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    CVMassert(!CVMgcIsGCThread(ee));
    CVMassert(CVMD_isgcSafe(ee));

    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    if (!MUST_NOTIFY_THREAD(ee, JVMTI_EVENT_MONITOR_WAITED)) {
	return;
    }
    jvmti_env = GLOBAL_ENV;
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.MonitorWaited;
	if (callback != NULL) {
	    if ((*env)->PushLocalFrame(env, 2) < 0) {
		return;
	    }
	    (*callback)(&jvmti_env->jvmtiExternal, env,
			(*env)->NewLocalRef(env, CVMcurrentThreadICell(ee)),
			(*env)->NewLocalRef(env, obj), timedout);
	    (*env)->PopLocalFrame(env, 0);
	}
    }
}

void CVMjvmtiPostObjectFreeEvent(CVMObject *obj)
{
    JvmtiEnv *jvmti_env;
    jvmtiEventObjectFree callback;
    CVMExecEnv *ee = CVMgetEE();
    int hash;
    jlong tag;

    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(CVMgcIsGCThread(ee));

    CVMJVMTI_CHECK_PHASEV(JVMTI_PHASE_LIVE);
    jvmti_env = GLOBAL_ENV;
    if (jvmti_env != NULL) {
	callback = jvmti_env->eventCallbacks.ObjectFree;
	if (callback != NULL) {
	    /*	    if ((*env)->PushLocalFrame(env, 2) < 0) {
		return;
	    }
	    CVMID_localrootBegin(ee); {
		CVMID_localrootDeclare(CVMObjectICell, tempObj);
		CVMD_gcUnsafeExec(ee, {
			CVMID_icellSetDirect(ee, tempObj, obj);
		    });
		object = (*env)->NewLocalRef(env, tempObj);
	    } CVMID_localrootEnd();
	    */
	    hash = CVMobjectGetHashNoSet(ee, obj);
	    if (hash == CVM_OBJECT_NO_HASH) {
		return;
	    }
	    CVMjvmtiTagGetTagGC(obj, &tag);
	    if (tag == 0) {
		return;
	    }
	    (*callback)(&jvmti_env->jvmtiExternal, tag);
	    /*	    (*env)->PopLocalFrame(env, 0); */
	}
    }
}

_jvmtiExports jvmtiExports;

void CVMjvmtiSetCanGetSourceDebugExtension(jboolean on) {
    jvmtiExports.canGetSourceDebugExtension = (on != 0);
}
void CVMjvmtiSetCanExamineOrDeoptAnywhere(jboolean on) {
    jvmtiExports.canExamineOrDeoptAnywhere = (on != 0);
}
void CVMjvmtiSetCanMaintainOriginalMethodOrder(jboolean on) {
    jvmtiExports.canMaintainOriginalMethodOrder = (on != 0);
}
void CVMjvmtiSetCanPostInterpreterEvents(jboolean on) {
    jvmtiExports.canPostInterpreterEvents = (on != 0);
}
void CVMjvmtiSetCanHotswapOrPostBreakpoint(jboolean on) {
    jvmtiExports.canHotswapOrPostBreakpoint = (on != 0);
}
void CVMjvmtiSetCanModifyAnyClass(jboolean on) {
    jvmtiExports.canModifyAnyClass = (on != 0);
}
void CVMjvmtiSetCanWalkAnySpace(jboolean on) {
    jvmtiExports.canWalkAnySpace = (on != 0);
}
void CVMjvmtiSetCanAccessLocalVariables(jboolean on) {
    jvmtiExports.canAccessLocalVariables = (on != 0);
}
void CVMjvmtiSetCanPostExceptions(jboolean on) {
    jvmtiExports.canPostExceptions = (on != 0);
}
void CVMjvmtiSetCanPostBreakpoint(jboolean on) {
    jvmtiExports.canPostBreakpoint = (on != 0);
}
void CVMjvmtiSetCanPostFieldAccess(jboolean on) {
    jvmtiExports.canPostFieldAccess = (on != 0);
}
void CVMjvmtiSetCanPostFieldModification(jboolean on) {
    jvmtiExports.canPostFieldModification = (on != 0);
}
void CVMjvmtiSetCanPostMethodEntry(jboolean on) {
    jvmtiExports.canPostMethodEntry = (on != 0);
}
void CVMjvmtiSetCanPostMethodExit(jboolean on) {
    jvmtiExports.canPostMethodExit = (on != 0);
}
void CVMjvmtiSetCanPopFrame(jboolean on) {
    jvmtiExports.canPopFrame = (on != 0);
}
void CVMjvmtiSetCanForceEarlyReturn(jboolean on) {
    jvmtiExports.canForceEarlyReturn = (on != 0);
}


void CVMjvmtiSetShouldPostSingleStep(jboolean on) {
    jvmtiExports.shouldPostSingleStep = on;
}
void CVMjvmtiSetShouldPostFieldAccess(jboolean on) {
    jvmtiExports.shouldPostFieldAccess = on;
}
void CVMjvmtiSetShouldPostFieldModification(jboolean on) {
    jvmtiExports.shouldPostFieldModification = on;
}
void CVMjvmtiSetShouldPostClassLoad(jboolean on) {
    jvmtiExports.shouldPostClassLoad = on;
}
void CVMjvmtiSetShouldPostClassPrepare(jboolean on) {
    jvmtiExports.shouldPostClassPrepare = on;
}
void CVMjvmtiSetShouldPostClassFileLoadHook(jboolean on) {
    jvmtiExports.shouldPostClassFileLoadHook = on;
}   
void CVMjvmtiSetShouldPostNativeMethodBind(jboolean on) {
    jvmtiExports.shouldPostNativeMethodBind = on;
}
void CVMjvmtiSetShouldPostCompiledMethodLoad(jboolean on) {
    jvmtiExports.shouldPostCompiledMethodLoad = on;
}
void CVMjvmtiSetShouldPostCompiledMethodUnload(jboolean on) {
    jvmtiExports.shouldPostCompiledMethodUnload = on;
}
void CVMjvmtiSetShouldPostDynamicCodeGenerated(jboolean on) {
    jvmtiExports.shouldPostDynamicCodeGenerated = on;
}   
void CVMjvmtiSetShouldPostMonitorContendedEnter(jboolean on) {
    jvmtiExports.shouldPostMonitorContendedEnter = on;
}
void CVMjvmtiSetShouldPostMonitorContendedEntered(jboolean on) {
    jvmtiExports.shouldPostMonitorContendedEntered = on;
}
void CVMjvmtiSetShouldPostMonitorWait(jboolean on) {
    jvmtiExports.shouldPostMonitorWait = on;
}
void CVMjvmtiSetShouldPostMonitorWaited(jboolean on) {
    jvmtiExports.shouldPostMonitorWaited = on;
}
void CVMjvmtiSetShouldPostGarbageCollectionStart(jboolean on) {
    jvmtiExports.shouldPostGarbageCollectionStart = on;
}
void CVMjvmtiSetShouldPostGarbageCollectionFinish(jboolean on) {
    jvmtiExports.shouldPostGarbageCollectionFinish = on;
}
void CVMjvmtiSetShouldPostDataDump(jboolean on) {
    jvmtiExports.shouldPostDataDump = on;
}   
void CVMjvmtiSetShouldPostObjectFree(jboolean on) {
    jvmtiExports.shouldPostObjectFree = on;
}
void CVMjvmtiSetShouldPostVmObjectAlloc(jboolean on) {
    jvmtiExports.shouldPostVmObjectAlloc = on;
}    

void CVMjvmtiSetShouldPostThreadLife(jboolean on) {
    jvmtiExports.shouldPostThreadLife = on;
}
void CVMjvmtiSetShouldCleanUpHeapObjects(jboolean on) {
    jvmtiExports.shouldCleanUpHeapObjects = on;
}

jlong
CVMjvmtiGetThreadEventEnabled(CVMExecEnv *ee) {
    return CVMjvmtiEventEnabled(ee).enabledBits;
}

void
CVMjvmtiSetShouldPostAnyThreadEvent(CVMExecEnv *ee, jlong enabled) {
    CVMjvmtiEventEnabled(ee).enabledBits = enabled;
}

/*
 * the below maybe don't have to be (but are for now) fixed conditions here 
 * any events can be enabled
 */
jboolean CVMjvmtiShouldPostThreadLife() {
    return jvmtiExports.shouldPostThreadLife;
}

/* ------ DYNAMIC conditions here ------------ */

jboolean CVMjvmtiShouldPostSingleStep() {
    return jvmtiExports.shouldPostSingleStep;
}
jboolean CVMjvmtiShouldPostFieldAccess() {
    return jvmtiExports.shouldPostFieldAccess;
}
jboolean CVMjvmtiShouldPostFieldModification() {
    return jvmtiExports.shouldPostFieldModification;
}
jboolean CVMjvmtiShouldPostClassLoad() {
    return jvmtiExports.shouldPostClassLoad;
}
jboolean CVMjvmtiShouldPostClassPrepare() {
    return jvmtiExports.shouldPostClassPrepare;
}
jboolean CVMjvmtiShouldPostClassFileLoadHook() {
    return jvmtiExports.shouldPostClassFileLoadHook;
}
jboolean CVMjvmtiShouldPostNativeMethodBind() {
    return jvmtiExports.shouldPostNativeMethodBind;
}
jboolean CVMjvmtiShouldPostCompiledMethodLoad() {
    return jvmtiExports.shouldPostCompiledMethodLoad;
}
jboolean CVMjvmtiShouldPostCompiledMethodUnload() {
    return jvmtiExports.shouldPostCompiledMethodUnload;
}
jboolean CVMjvmtiShouldPostDynamicCodeGenerated() {
    return jvmtiExports.shouldPostDynamicCodeGenerated;
}
jboolean CVMjvmtiShouldPostMonitorContendedEnter() {
    return jvmtiExports.shouldPostMonitorContendedEnter;
}
jboolean CVMjvmtiShouldPostMonitorContendedEntered() {
    return jvmtiExports.shouldPostMonitorContendedEntered;
}
jboolean CVMjvmtiShouldPostMonitorWait() {
    return jvmtiExports.shouldPostMonitorWait;
}
jboolean CVMjvmtiShouldPostMonitorWaited() {
    return jvmtiExports.shouldPostMonitorWaited;
}
jboolean CVMjvmtiShouldPostDataDump() {
    return jvmtiExports.shouldPostDataDump;
}
jboolean CVMjvmtiShouldPostGarbageCollectionStart() {
    return jvmtiExports.shouldPostGarbageCollectionStart;
}
jboolean CVMjvmtiShouldPostGarbageCollectionFinish() {
    return jvmtiExports.shouldPostGarbageCollectionFinish;
}
jboolean CVMjvmtiShouldPostVmObjectAlloc() {
    return jvmtiExports.shouldPostVmObjectAlloc;
}

/* attach support */
jint loadAgentLibrary(AttachOperation* op, char* out);

/* ----------------- */

jboolean isJvmtiVersion(jint version) {
    return (version & JVMTIVERSIONMASK) == JVMTIVERSIONVALUE;
}
jint getJvmtiInterface(JavaVM *jvm, void **penv, jint version);


void
CVMjvmtiEnterPrimordialPhase() {
  
    jvmtiCurrentPhase = JVMTI_PHASE_PRIMORDIAL;
}

void
CVMjvmtiEnterStartPhase() {
    jvmtiCurrentPhase = JVMTI_PHASE_START;
}

void
CVMjvmtiEnterOnloadPhase() {
    jvmtiCurrentPhase = JVMTI_PHASE_ONLOAD;
}

void
CVMjvmtiEnterLivePhase() {
    jvmtiCurrentPhase = JVMTI_PHASE_LIVE;
}

void
CVMjvmtiEnterDeadPhase() {
    jvmtiCurrentPhase = JVMTI_PHASE_DEAD;
}

jvmtiPhase
CVMjvmtiGetPhase() {
    return jvmtiCurrentPhase;
}

jvmtiError
CVMjvmtiAllocate(jlong size, unsigned char **mem) {
    JvmtiEnv *jvmtiEnv = GLOBAL_ENV;
    CVMassert((jvmtiEnv != NULL));
    CVMassert((jvmtiEnv->jvmtiExternal != NULL));
    jvmtiEnv->jvmtiExternal->Allocate(&jvmtiEnv->jvmtiExternal, size, mem);
    if (size > 0) {
	CVMassert(*mem != NULL);
    }
    return JVMTI_ERROR_NONE;
}

jvmtiError
CVMjvmtiDeallocate(unsigned char *mem) {
    JvmtiEnv *jvmtiEnv = GLOBAL_ENV;
    CVMassert((jvmtiEnv != NULL));
    CVMassert((jvmtiEnv->jvmtiExternal != NULL));
    jvmtiEnv->jvmtiExternal->Deallocate(&jvmtiEnv->jvmtiExternal, mem);
    return JVMTI_ERROR_NONE;
}

CVMBool
CVMjvmtiClassBeingRedefined(CVMExecEnv *ee, CVMClassBlock *cb) {

    ThreadNode *node;
    if (CVMjvmtiEnabled()) {
	node = CVMjvmtiFindThread(ee, CVMcurrentThreadICell(ee));
	return node == NULL ? CVM_FALSE : (node->redefineCb == cb);
    }
    return CVM_FALSE;
}

static JvmtiLockInfo *
jvmtiGetLockInfoX(JvmtiEnv *jvmtiEnv, CVMExecEnv *ee) {
    int i;
    JvmtiLockInfo *li;

    if (ee->jvmtiEE.jvmtiLockInfoFreelist == NULL) {
	for (i = 0; i < 25; i++) {
	    /* allocate some lock info elements */
	    if (CVMjvmtiAllocate(sizeof(JvmtiLockInfo),
				 (unsigned char **)&li) != JVMTI_ERROR_NONE) {
		return NULL;
	    }
	    li->next = ee->jvmtiEE.jvmtiLockInfoFreelist;
	    ee->jvmtiEE.jvmtiLockInfoFreelist = li;
	}
    }
    li = ee->jvmtiEE.jvmtiLockInfoFreelist;
    CVMassert(li != NULL);
    ee->jvmtiEE.jvmtiLockInfoFreelist = li->next;
    li->next = NULL;
    return li;
}

static JvmtiLockInfo *
jvmtiGetLockInfo(CVMExecEnv *ee) {
    JvmtiLockInfo *li;
    JvmtiEnv *jvmti_env = GLOBAL_ENV;
    if (!CVMD_isgcSafe(ee)) {
	CVMD_gcSafeExec(ee, {
		li = jvmtiGetLockInfoX(jvmti_env, ee);
	    });
    } else {
	li = jvmtiGetLockInfoX(jvmti_env, ee);
    }
    return li;
}

static void
jvmtiFreeLockInfoX(JvmtiEnv *jvmtiEnv, CVMExecEnv *ee, JvmtiLockInfo *li) {

    li->next = ee->jvmtiEE.jvmtiLockInfoFreelist;
    li->lock = NULL;
    ee->jvmtiEE.jvmtiLockInfoFreelist = li;
}

static void
jvmtiFreeLockInfo(CVMExecEnv *ee, JvmtiLockInfo *li) {
    JvmtiEnv *jvmti_env = GLOBAL_ENV;
    if (!CVMD_isgcSafe(ee)) {
	CVMD_gcSafeExec(ee, {
		jvmtiFreeLockInfoX(jvmti_env, ee, li);
	    });
    } else {
	jvmtiFreeLockInfoX(jvmti_env, ee, li);
    }
}

/*
 * Keep track of locks per frame so that we can unlock them if a PopFrame
 * is requested
 */

void
CVMjvmtiAddLock(CVMExecEnv *ee, CVMOwnedMonitor *o) {
    if (CVMframeIsJava(CVMeeGetCurrentFrame(ee))) {
	CVMJavaFrame *jframe = CVMgetJavaFrame(CVMeeGetCurrentFrame(ee));
	JvmtiLockInfo *li;
	li = jvmtiGetLockInfo(ee);
	if (li != NULL) {
	    li->next = jframe->jvmtiLockInfo;
	    jframe->jvmtiLockInfo = li;
	    li->lock = o;
	}
    }
}

void
CVMjvmtiAddMon(CVMExecEnv *ee, CVMObjMonitor *mon) {
    if (CVMframeIsJava(CVMeeGetCurrentFrame(ee))) {
	CVMJavaFrame *jframe = CVMgetJavaFrame(CVMeeGetCurrentFrame(ee));
	JvmtiLockInfo *li = jvmtiGetLockInfo(ee);
	if (li != NULL) {
            CVMOwnedMonitor *o = ee->objLocksOwned;
            if (o->u.heavy.mon != mon) {
                do {
                    o = o->next;
                } while ((o != NULL) && (o->u.heavy.mon != mon));
                CVMassert (o != NULL);
            }
	    li->next = jframe->jvmtiLockInfo;
	    jframe->jvmtiLockInfo = li;
	    li->lock = o;
	}
    }
}

void
CVMjvmtiRemoveLock(CVMExecEnv *ee, CVMOwnedMonitor *o) {
    if (CVMframeIsJava(CVMeeGetCurrentFrame(ee))) {
	CVMJavaFrame *jframe = CVMgetJavaFrame(CVMeeGetCurrentFrame(ee));
	JvmtiLockInfo *li = jframe->jvmtiLockInfo;
	if (li != NULL) {
	    CVMassert(o == li->lock);
	    jframe->jvmtiLockInfo = li->next;
	    jvmtiFreeLockInfo(ee, li);
	}
    }
}

void
CVMjvmtiRemoveMon(CVMExecEnv *ee, CVMObjMonitor *mon) {
    if (CVMframeIsJava(CVMeeGetCurrentFrame(ee))) {
	CVMJavaFrame *jframe = CVMgetJavaFrame(CVMeeGetCurrentFrame(ee));
	JvmtiLockInfo *li = jframe->jvmtiLockInfo;
	if (li != NULL) {
	    CVMOwnedMonitor *o = ee->objLocksOwned;

	    if (o->u.heavy.mon != mon) {
		do {
		    o = o->next;
		} while ((o != NULL) && (o->u.heavy.mon != mon));
		CVMassert (o != NULL);
	    }
	    CVMassert(o == li->lock);
	    jframe->jvmtiLockInfo = li->next;
	    jvmtiFreeLockInfo(ee, li);
	}
    }
}

#endif /* CVM_JVMTI */
