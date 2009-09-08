/*
 * @(#)interpreter.c	1.409 06/10/25
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

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/sync.h"
#include "javavm/include/globals.h"
#include "javavm/include/stacks.h"
#include "javavm/include/stackwalk.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/jni_impl.h"
#include "javavm/include/localroots.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/signature.h"
#include "javavm/include/preloader.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/stackmaps.h"
#include "javavm/include/bcattr.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/opcodes.h"

#include "generated/offsets/java_lang_Throwable.h"
#ifdef CVM_CLASSLOADING
#include "generated/offsets/java_lang_ClassLoader.h"
#endif

#include "javavm/include/clib.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/system.h"

#ifdef CVM_CLASSLOADING
#include "javavm/include/porting/linker.h"
#endif

#ifdef CVM_REFLECT
#include "javavm/include/reflect.h"
#endif

#ifdef CVM_JVMTI
#include "javavm/include/jvmtiExport.h"
#endif

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

#ifdef CVM_JIT
#include "javavm/include/jit/jitpcmap.h"
#include "javavm/include/porting/jit/ccm.h"
#include "javavm/include/ccee.h"
#endif

#ifdef CVM_HW
#include "include/hw.h"
#endif

#ifdef CVM_TRACE
#define MAX_TRACE_INDENT_DEPTH  15

static void printIndent(int traceDepth)
{
    /* We're assuming the number of decimal digits in the trace depth is less
       than 3.  If it reaches 3 or above (which highly unlikely), we'll just
       allow the indentation to be little off by the extra digits: */
    const char *digitPad = (traceDepth < 10) ? " " : "";
    int depth, i;

    CVMconsolePrintf("%d%s", traceDepth, digitPad);
    depth = (traceDepth < MAX_TRACE_INDENT_DEPTH) ?
	     traceDepth : MAX_TRACE_INDENT_DEPTH;
    for (i = 0; i < depth; i++) {
	CVMconsolePrintf("  ");
    }

}

#define CVMtraceMethod(ee, x) \
    CVMtraceExec(CVM_DEBUGFLAG(TRACE_METHOD), {		       \
	CVMconsolePrintf("<%d>", ee->threadID);		       \
	printIndent(ee->traceDepth);			       \
	CVMconsolePrintf x;				       \
    })

#endif /* CVM_TRACE */


static CVMFrame *
CVMframeIterateGetBaseFrame(CVMFrameIterator *iter);

/* Capacities for the thread's local roots stack. */
#define CVM_MIN_LOCALROOTS_STACKCHUNK_SIZE 20
#define CVM_MAX_LOCALROOTS_STACK_SIZE      \
    3 * CVM_MIN_LOCALROOTS_STACKCHUNK_SIZE
#define CVM_INITIAL_LOCALROOTS_STACK_SIZE   \
    CVM_MIN_LOCALROOTS_STACKCHUNK_SIZE

static CVMBool
CVMarrayIsAssignable(CVMExecEnv* ee, 
		     CVMClassBlock* srcArrayCb,
		     CVMClassBlock* dstArrayCb);


#if defined(CVM_DEBUG) || defined(CVM_DEBUG_DUMPSTACK) || defined(CVM_DEBUG_STACKTRACES)
static char*
CVMaddstr(const char *s, char* buf, char* limit, char term)
{
    char c;
    while ((c = *s) && c != term && buf < limit) {
	*buf++ = c;
	s++;
    }
    return buf;
}
#endif /* CVM_DEBUG */


#if (defined(CVM_DEBUG) || defined(CVM_DEBUG_DUMPSTACK) || \
     defined(CVM_DEBUG_STACKTRACES)) && defined(CVM_DEBUG_CLASSINFO)
static char*
CVMadddec(CVMInt32 n, char* buf, char* limit)
{ 
    char dec[11];
    int i = sizeof(dec) - 1;
    dec[i] = 0;

    if (n < 0) {
        n = -n;
        if (buf < limit)
            *buf++ = '-';
    }

    while (n != 0) {
        i--;
        dec[i] = (char)((n % 10) + '0');
        n = n / 10;
    }
    return CVMaddstr(&dec[i], buf, limit, 0);
}

#endif /* CVM_DEBUG */

#if defined(CVM_DEBUG_CLASSINFO) || defined(CVM_JVMPI)
CVMInt32
CVMpc2lineno(CVMMethodBlock *mb, CVMUint16 pc_offset)
{
    CVMUint32 length;

    CVMassert(!CVMmbIs(mb, NATIVE));

    length = CVMmbLineNumberTableLength(mb);
    if (length > 0) {
	CVMLineNumberEntry *ln = CVMmbLineNumberTable(mb);
	CVMUint16 l = 0;
	CVMUint16 u = length;
	if (pc_offset < ln[l].startpc) { 
	    return -1;
	}
	if (pc_offset >= ln[u - 1].startpc) {
	    return ln[u - 1].lineNumber;
	}
	while (l < u) {
	    int m = (l + u) >> 1;
	    if (pc_offset < ln[m].startpc) {
	        u = m;
	    } else if (pc_offset >= ln[m + 1].startpc) {
	        l = m;
	    } else {
	        /* ln[m].startpc <= pc_offset < ln[m + 1].startpc */ 
	        return ln[m].lineNumber;
	    }
	}
	CVMassert(CVM_FALSE);
    }
    return -1;
}
#endif /* defined(CVM_DEBUG_CLASSINFO) || defined(CVM_JVMPI) */

/*
 * Convert either a pc or a frame to a string in the format:
 *    <classname>.<methodname>(Native Method)
 *        or
 *    <classname>.<methodname>(<sourcefilename>:<linenumber>)
 *
 * If a pc is specified, it is always a javapc and never a compiled pc,
 * even if isCompiled is true.
 */
#if defined(CVM_TRACE) || defined(CVM_DEBUG) || defined(CVM_DEBUG_DUMPSTACK) || defined(CVM_DEBUG_STACKTRACES)

void
CVMframe2string(CVMFrame* frame, char *buf, char* limit) {
    CVMUint8* pc;
    CVMBool   isCompiled;

    if (CVMframeIsTransition(frame) | CVMframeIsJava(frame)) {
	pc = CVMframePc(frame);
    } else {
	pc = NULL;
    }

#ifdef CVM_JIT
    isCompiled = CVMframeIsCompiled(frame);
    if (isCompiled) {
	pc = CVMpcmapCompiledPcToJavaPc(frame->mb, CVMcompiledFramePC(frame));
    }
#else
    isCompiled = CVM_FALSE;
#endif

    CVMpc2string(pc, frame->mb, CVMframeIsTransition(frame), isCompiled, 
		 buf, limit);
}

void
CVMframeIterate2string(CVMFrameIterator *iter, char *buf, char *limit) {
    CVMUint8* pc;
    CVMMethodBlock *mb;
    CVMBool   isCompiled;
    CVMBool   isTransition = CVM_FALSE;

    pc = CVMframeIterateGetJavaPc(iter);
    mb = CVMframeIterateGetMb(iter);

#ifdef CVM_JIT
    isCompiled = iter->jitFrame;
#else
    isCompiled = CVM_FALSE;
#endif
    if (!isCompiled) {
	isTransition = CVMframeIsTransition(iter->frame);
    }

    CVMpc2string(pc, mb, isTransition, isCompiled, buf, limit);
}

void
CVMlineno2string(CVMInt32 lineno, CVMMethodBlock* mb, 
	         CVMBool isTransition, CVMBool isCompiled,
                 char *buf, char* limit)
{
    CVMClassBlock*  cb;

    CVMassert(buf < limit);
    CVMassert(mb != NULL);

    cb = CVMmbClassBlock(mb);

    --limit;	/* Save room for terminating '\0' */
    buf += CVMformatString(buf, limit - buf, "%C.%M", cb, mb);
    if (CVMmbIs(mb, NATIVE)) {
	buf = CVMaddstr("(Native Method)", buf, limit, 0);
    } else if (isTransition) {
	buf = CVMaddstr("(Transition Method)", buf, limit, 0);
    } else {
#ifdef CVM_DEBUG_CLASSINFO
	const char* fn = CVMcbSourceFileName(cb);
#endif

#ifdef CVM_JIT
	if (isCompiled) {
	    buf = CVMaddstr("(Compiled Method)", buf, limit, 0);
	}
#endif /* CVM_JIT */
	
#ifndef CVM_DEBUG_CLASSINFO
	buf = CVMaddstr("(Unknown Source)", buf, limit, 0);
#else
	if (fn == NULL) {
	    buf = CVMaddstr("(Unknown Source)", buf, limit, 0);
	} else {
	    /* Just want the short name */
	    fn = strrchr(fn, '/');
	    if (fn != NULL) {
		/* skip past last separator */
		fn += 1;
	    } else {
		fn = CVMcbSourceFileName(cb);
	    }
		
	    buf = CVMaddstr("(", buf, limit, 0);
	    buf = CVMaddstr(fn, buf, limit, 0);

	    if (lineno >= 0) {
		buf = CVMaddstr(":", buf, limit, 0);
		buf = CVMadddec(lineno, buf, limit);
	    }
	    buf = CVMaddstr(")", buf, limit, 0);
	}
#endif /* CVM_DEBUG_CLASSINFO */
    }
    *buf = 0;
    return;
}

void
CVMpc2string(CVMUint8* pc, CVMMethodBlock* mb,
	     CVMBool isTransition, CVMBool isCompiled,
	     char *buf, char* limit)
{
    CVMInt32    lineno;

    CVMassert(buf < limit);
    CVMassert(mb != NULL);
		    
    /*
     * Get the pc offset from the start of the code so we can
     * use it to lookup linenumber info. 
     */
    /* 
     * This expression is obviously a rather small pointer
     * difference. So just cast it to the type of 'pc_offset'.
     */
#ifdef CVM_DEBUG_CLASSINFO
    if (CVMmbIs(mb, NATIVE) || isTransition) {
        lineno = -1;
    } else {
        CVMUint16    pc_offset;
        pc_offset = pc - CVMmbJavaCode(mb);		
        lineno = CVMpc2lineno(mb, pc_offset);
    }
#else
    lineno = -1;
#endif

    CVMlineno2string(lineno, mb, isTransition, isCompiled,
                     buf, limit);
}
#endif /* CVM_DEBUG */

/*
 * Initialize a CVMExecEnv.
 * The threadInfo argument can be NULL if invoked from 
 * CVMinitVMGlobalState() or CVMjniAttachCurrentThread().
 * The argument is typically used to pass on info from a parent thread 
 * to a child.
 */
CVMBool 
CVMinitExecEnv(CVMExecEnv* ee, CVMExecEnv* targetEE,
	       CVMThreadStartInfo* threadInfo)
{
    CVMBool result;

    /*
     * Unless we are prepared to zero out every field by hand, then
     * we have to do this...
     */
    memset(targetEE, 0, sizeof *targetEE);

    CVMCstackBufferInit(targetEE);	/* Unset EE-buffer flag */

    /* threadInfo == NULL if invoked from CVMinitVMGlobalState() or 
     * CVMjniAttachCurrentThread(). Set CVM_TRUE to ee->userThread
     * in those cases. */
    targetEE->userThread =
	(threadInfo == NULL)?(CVM_TRUE):(!threadInfo->isDaemon);

    /* Need to initialize consistent states before anything else
     * or some debugging asserts may get upset.
     */
    {
	int i;

	for (i = 0; i < CVM_NUM_CONSISTENT_STATES; ++i) {
	    CVMtcsInit(CVM_TCSTATE(targetEE, i));
	}
    }

#ifdef CVM_DEBUG
    /* Also make sure this EE doesn't think we hold any locks. */
    targetEE->sysLocks = NULL;
#endif

    /* The first frame has to be a JNI frame so we can allocate
     * jni local refs from it.
     */

    result = CVMinitStack(ee, &targetEE->interpreterStack, 
			  CVMglobals.config.javaStackMinSize,
#ifdef CVM_JVMTI
                          /* Leave some space to post events */
			  CVMglobals.config.javaStackMaxSize -
                          CVMglobals.config.javaStackChunkSize, 
#else
 			  CVMglobals.config.javaStackMaxSize, 
#endif
			  CVMglobals.config.javaStackChunkSize,
			  CVMJNIFrameCapacity,
			  CVM_FRAMETYPE_JNI);
    if (!result) {
	goto failed; /* exception NOT thrown */
    }

    CVMjniFrameInit(CVMgetJNIFrame(targetEE->interpreterStack.currentFrame),
                    CVM_FALSE);

    result = CVMinitStack(ee, &targetEE->localRootsStack, 
			  CVM_INITIAL_LOCALROOTS_STACK_SIZE,
			  CVM_MAX_LOCALROOTS_STACK_SIZE, 
			  CVM_MIN_LOCALROOTS_STACKCHUNK_SIZE,
			  CVMbaseFrameCapacity,
			  CVM_FRAMETYPE_LOCALROOT);
    if (!result) {
	goto failed; /* exception NOT thrown */
    }

    if (!CVMeeSyncInit(targetEE)) {
	goto failed; /* exception NOT thrown */
    }

    CVMinitJNIEnv(&targetEE->jniEnv);

    /* Allocate JNI local refs for targetEE
     */
    CVMcurrentThreadICell(targetEE)	= CVMjniCreateLocalRef0(ee, targetEE);
    CVMmiscICell(targetEE)              = CVMjniCreateLocalRef0(ee, targetEE);
    CVMsyncICell(targetEE)              = CVMjniCreateLocalRef0(ee, targetEE);
    CVMfinalizerRegisterICell(targetEE) = CVMjniCreateLocalRef0(ee, targetEE);
    CVMallocationRetryICell(targetEE)   = CVMjniCreateLocalRef0(ee, targetEE);
    CVMlocalExceptionICell(targetEE)    = CVMjniCreateLocalRef0(ee, targetEE);
#ifdef CVM_REMOTE_EXCEPTIONS_SUPPORTED
    CVMremoteExceptionICell(targetEE)   = CVMjniCreateLocalRef0(ee, targetEE);
#endif
    CVMcurrentExceptionICell(targetEE)  = CVMjniCreateLocalRef0(ee, targetEE);
    /*
     * Clear flags and objects (which should already be null)
     */
    CVMclearLocalException(targetEE);
    CVMclearRemoteException(targetEE);

#ifdef CVM_LVM /* %begin lvm */
    {
	/* Initialize Logical VM related part of 'ee'. */
	CVMLVMEEInitAux* initInfo = 
	    (threadInfo != NULL)?(&threadInfo->lvmEEInitAux):(NULL);
	if (!CVMLVMinitExecEnv(targetEE, initInfo)) {
	    goto failed; /* exception NOT thrown */
	    return CVM_FALSE;
	}
    }
#endif /* %end lvm */

#ifdef CVM_TRACE_ENABLED

#if defined(CVM_PROFILE_METHOD)
    targetEE->cindx = -1;
#endif

    /* Inherit global flags */
    targetEE->debugFlags = CVMglobals.debugFlags;

#endif

    targetEE->threadState = CVM_THREAD_RUNNING;
    return CVM_TRUE;

 failed:
    /* cleanup anything that was allocated. */
    CVMdestroyExecEnv(targetEE);
    return CVM_FALSE;
}

CVMBool 
CVMattachExecEnv(CVMExecEnv* ee, CVMBool orphan)
{
    /* attach thread */
    if (!CVMthreadAttach(CVMexecEnv2threadID(ee), orphan)) {
	return CVM_FALSE;
    }

    /*
     * Link it in.
     */

    CVMsysMutexLock(ee, &CVMglobals.threadLock);
    {
	CVMExecEnv **firstPtr = &CVMglobals.threadList;
	ee->prevEEPtr = firstPtr;
	ee->nextEE = *firstPtr;
	if (*firstPtr != 0) {
	    (*firstPtr)->prevEEPtr = &ee->nextEE;
	}
	*firstPtr= ee;
	ee->threadID = CVMglobals.threadIDCount;
	++CVMglobals.threadIDCount;
    }
    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
    return CVM_TRUE;
}

void 
CVMdestroyExecEnv(CVMExecEnv* ee)
{
#ifdef CVM_DEBUG
    CVMassert(ee->sysLocks == NULL);
#endif

    {
	int i;

	for (i = 0; i < CVM_NUM_CONSISTENT_STATES; ++i) {
	    CVMtcsDestroy(CVM_TCSTATE(ee, i));
	}
    }
    {
#ifdef CVM_DEBUG
	CVMlocalExceptionICell(ee) = NULL;
	CVMremoteExceptionICell(ee) = NULL;
	CVMcurrentExceptionICell(ee) = NULL;
	CVMmiscICell(ee) = NULL;
        CVMsyncICell(ee) = NULL;
	CVMfinalizerRegisterICell(ee) = NULL;
	CVMallocationRetryICell(ee) = NULL;
	CVMcurrentThreadICell(ee) = NULL;
#endif
    }

#ifdef CVM_LVM /* %begin lvm */
    /* Deptroy Logical VM related part of 'ee' */
    CVMLVMdestroyExecEnv(ee);
#endif /* %end lvm */

    CVMdestroyStack(&ee->interpreterStack);
    CVMdestroyStack(&ee->localRootsStack);

    CVMdestroyJNIEnv(&ee->jniEnv);
#ifdef CVM_JVMTI
    CVMjvmtiDestroyLockInfo(ee);
#endif
    /* free the EE buffer used by C stack IO operations */
    CVMCstackFreeBuffer(ee);

    /*
     * The thread is no longer allowed to access VM data structures
     * after this point.
     */

#ifdef CVM_DEBUG
    CVMassert(ee->sysLocks == NULL);
#endif
}

void 
CVMdetachExecEnv(CVMExecEnv* ee)
{
    /* we must call CVMeeSyncDestroy() before CVMthreadDetach */
    CVMeeSyncDestroy(ee);

    /*
     * Unlink it.  After this point, GC will no longer
     * scan this thread.
     */
    CVMsysMutexLock(ee, &CVMglobals.threadLock);
    *ee->prevEEPtr = ee->nextEE;
    if (ee->nextEE != 0) {
	ee->nextEE->prevEEPtr = ee->prevEEPtr;
    }
    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);

    /* detach thread */
    CVMthreadDetach(CVMexecEnv2threadID(ee));
}

CVMExecEnv *
CVMgetEE()
{
    CVMThreadID *self = CVMthreadSelf();
    if (self == NULL) {
	return NULL;
    } else {
	return CVMthreadID2ExecEnv(self);
    }
}

/*
 * Scan reference-typed arguments by looking at signature.
 */
static void
CVMinterpreterIterateRefArguments(CVMMethodBlock* mb, CVMStackVal32* list,
				  CVMRefCallbackFunc callback, void* data)
{
    CVMMethodTypeID  tid = CVMmbNameAndTypeID(mb);
    CVMterseSigIterator terseSig;
    char       arg;

    /* 
     * The terse signature is all we need (and even less than that)
     */
    CVMtypeidGetTerseSignatureIterator( tid, &terseSig );
    if (!CVMmbIs(mb, STATIC)) {
	CVMObject** refPtr = (CVMObject**)list;
	CVMassert(*refPtr != 0);
	callback(refPtr, data);
	list++;
    }	
    while((arg = CVM_TERSE_ITER_NEXT(terseSig)) != CVM_TYPEID_ENDFUNC) {
	if (arg == CVM_TYPEID_OBJ) {
	    CVMObject** refPtr = (CVMObject**)list;
	    if (*refPtr != 0) {
		callback(refPtr, data);
	    }
	}
	switch (arg) {
	case CVM_TYPEID_LONG:
	case CVM_TYPEID_DOUBLE:
	    list += 2;
	    break;
	default:
	    list += 1;
	}
    }
}

static void
CVMtransitionFrameScanner(CVMExecEnv* ee,
			  CVMFrame* frame, CVMStackChunk* chunk,
			  CVMRefCallbackFunc callback, void* scannerData,
			  CVMGCOptions* opts)
{
    CVMInterpreterStackData *interpreterStackData =
	(CVMInterpreterStackData *)scannerData;
    void *data = interpreterStackData->callbackData;
    CVMassert(CVMframeIsTransition(frame));

    CVMtraceGcScan(("Scanning frame for %C.%M ",
		    CVMmbClassBlock(frame->mb), frame->mb));

    /*
     * If the transition frame was marked as one that returns a
     * ref and we are at the opc_exitinterpreter instruction,
     * then the result is our responsiblity and not that of the
     * java method that pushed the result. There are three possible
     * states the frame can be in:
     */
    if 	(CVMframePc(frame) != NULL &&
	  *CVMframePc(frame) == opc_exittransition) {
	/* 
	 * (1) we've caught the small window between the callee
	 * having completed the return, but the caller hasn't
	 * fetched the result and popped the transition frame yet.
	 * Scan the result if it's a reference.
	 * 
	 * This is covering the checkpoint at opc_?return (handle_return:).
	 */
	if (CVMtypeidGetReturnType(CVMmbNameAndTypeID(frame->mb)) ==
	    CVM_TYPEID_OBJ)
	{
	    CVMObject** refPtr = (CVMObject**)&frame->topOfStack[-1];
	    CVMtraceGcScan(("(transition, ref result)\n"));
	    if (*refPtr != 0) {
		(*callback)(refPtr, data);
	    }
	} else {
	    CVMtraceGcScan(("(transition, no ref result)\n"));
	}
    } else if (frame->topOfStack == 
	       (CVMStackVal32*)CVMframeOpstack(frame, Transition)) {
	/* 
	 * (2) We've caught the big window between the callee having taken
	 * responsibility for the arguments, and the callee having completed
	 * the return, so there is nothing to scan. This only happens when 
	 * the transition frame was used to invoke a java method from jni.
	 * When a transtion frame is used to invoke a jni method from jni,
	 * the arguments are always scannable.
	 */
	CVMtraceGcScan(("(transition, no scan needed)\n"));
	return;
    } else {
	/* 
	 * (3) If the invoke opcode in the transition frame had to
	 * expand the stack, you would hit this one. You could also
	 * hit it if after having pushed a transition frame and added
	 * a large number of arguments, and then. Lastly, there is a
	 * small gcsafe section in CVMjniInvoke() that occurs after
	 * the transition frame is pushed and before we become
	 * gcUnsafe to push the arguments which will also cause you
	 * to end up here. Scan all the arguments.
	 */
	CVMStackVal32* argsStart = 
	    (CVMStackVal32*)CVMframeOpstack(frame, Transition);
	CVMtraceGcScan(("(transition, scanning arguments)\n"));
	CVMinterpreterIterateRefArguments(frame->mb, argsStart,
					  callback, data);
    }
}

/*
 * Find the innermost exception handler for a PC, for stackmap purposes only.
 */
CVMUint8*
CVMfindInnermostHandlerFor(CVMJavaMethodDescriptor* jmd, CVMUint8* pc)
{
    CVMExceptionHandler *eh = CVMjmdExceptionTable(jmd);
    CVMExceptionHandler *ehEnd = eh + CVMjmdExceptionTableLength(jmd);
    CVMUint8* codeBegin = CVMjmdCode(jmd);
    /*
     * This expression is obviously a rather small pointer
     * difference. So just cast it to the type of 'relPC'.
     */
    CVMUint16 relPC = (CVMUint16)(pc - codeBegin);
	
    /* Check each exception handler to see if the pc is in its range */
    for (; eh < ehEnd; eh++) { 
	if (eh->startpc <= relPC && relPC < eh->endpc) { 
	    return codeBegin + eh->handlerpc;
	}
    }
    return 0;
}

/*
 * Get the stackmap entry for a frame. If (*missingStackMapOK), a NULL entry
 * is normal.
 */
/* NOTE: The frameEE is the CVMExecEnv of the thread which owns the frame for
         which we want to get a stackmap.  It is not necessarily the CVMExecEnv
         of the self (i.e. currently executing) thread.
*/
CVMStackMapEntry*
CVMgetStackmapEntry(CVMExecEnv *frameEE, CVMFrame *frame,
                 CVMJavaMethodDescriptor *jmd, CVMStackMaps *stackmaps,
                 CVMBool *missingStackmapOK)
{
    CVMStackMapEntry* smEntry;
    /*
     * The stackmaps have been created. Now let's check for the entry.
     */
    /*
     * This expression is obviously a rather small pointer difference.
     * So just cast it to the type of the second formal parameter.
     */
    smEntry = CVMstackmapGetEntryForPC(stackmaps,
				       (CVMUint16)(CVMframePc(frame) - CVMjmdCode(jmd)));
    if (smEntry == NULL) {
	CVMUint8* handlerPC;

        if (CVMexceptionIsBeingThrownInFrame(frameEE, frame)) {
            /* Is throwing an exception: */
	    handlerPC = CVMfindInnermostHandlerFor(jmd, CVMframePc(frame));
	    if (handlerPC == NULL) {
		/* Not handled here -- OK to be missing stackmap then, since
		   we are going to be abandoning this frame anyway */
		*missingStackmapOK = CVM_TRUE;
		return NULL;
	    } else {
		/* The handler is here. Now we must make sure that we
		   find a stackmap for 'handlerPC'. */
		*missingStackmapOK = CVM_FALSE;
	    }
            /* 
             * This expression is obviously a rather small pointer difference.
             * So just cast it to the type of the second formal parameter.
             */
	    smEntry = CVMstackmapGetEntryForPC(stackmaps, 
					       (CVMUint16)(handlerPC - CVMjmdCode(jmd)));
        } else {
            /* Not throwing an exception; therefore no entry */
            *missingStackmapOK = CVM_FALSE;
            return NULL;
	}
    }
    return smEntry;
}

/*
 * Ensure space for and perform stackmap allocations for a given frame.
 * This is called at the onset of GC _before_ CVMgcimplDoGC() is called.
 * If this returns an error, we don't even call CVMgcimplDoGC(), so it
 * does not have to deal with the error. This can be thought of as a
 * pre-verification pass to ensure that the GC stackwalk will never have
 * to encounter a stackmap creation failure.
 *
 * CVM_TRUE  if stackmaps successfully created
 * CVM_FALSE on error
 */
/* NOTE: The frameEE is the CVMExecEnv of the thread which owns the frame for
         which we want to get a stackmap.  It is not necessarily the CVMExecEnv
         of the self (i.e. currently executing) thread.
*/
CVMBool
CVMjavaFrameEnsureStackmaps(CVMExecEnv *ee, CVMExecEnv *frameEE,
                            CVMFrame *frame)
{
    CVMMethodBlock* mb;
    CVMJavaMethodDescriptor* jmd;
    CVMStackMaps*    stackmaps;
    CVMUint32        pcOffset;

    CVMassert(CVMframeIsJava(frame));

    mb = frame->mb;
    jmd = CVMmbJmd(mb);

    /*
     * This expression is obviously a rather small pointer
     * difference. So just cast it to the type of 'pcOffset'.
     */
    pcOffset = (CVMUint32)(CVMframePc(frame) - CVMjmdCode(jmd));

    CVMtraceGcScan(("Ensuring stackmaps generation in frame for %C.%M "
                    "(maxLocals=%d, locals=0x%x, stack=0x%x, pc=%d[0x%x])\n",
		    CVMmbClassBlock(frame->mb), frame->mb,
		    CVMmbMaxLocals(frame->mb),
		    CVMframeLocals(frame),
		    CVMframeOpstack(frame, Java),
		    pcOffset,
		    CVMjmdCode(jmd)));

    stackmaps = CVMstackmapFind(ee, mb);
    if (stackmaps == NULL) {
	/* Compute stackmaps, do not consider conditional GC points */
	stackmaps = CVMstackmapCompute(ee, mb, CVM_FALSE);
    } else {
        CVMstackmapPromoteToFrontOfGlobalList(ee, stackmaps);
    }

    if (stackmaps == NULL) {
	return CVM_FALSE; /* Could not create stackmaps */
    } else {
	CVMBool missingStackmapOK;
	CVMStackMapEntry* smEntry;
	CVMOpcode opcode;

        smEntry = CVMgetStackmapEntry(frameEE, frame, jmd, stackmaps,
                                   &missingStackmapOK);
	if (smEntry != NULL) {
	    return CVM_TRUE; /* Found an entry; all is fine */
	}
	if (missingStackmapOK) {
	    /* It is OK to not have an entry only if the frame
	       was throwing an exception and it is not handled in this
	       frame. */
	    return CVM_TRUE;
	}
	opcode = (CVMOpcode)*CVMframePc(frame);
	if (!CVMbcAttr(opcode, COND_GCPOINT)) {
	    /* This can't happen! */
	    CVMassert(CVM_FALSE);
	    return CVM_FALSE;
	} else {
	    /* Now, if there is no entry, and we are at a
	       conditional GC point, re-create stackmaps, and
	       re-store them */
            CVMstackmapDestroy(ee, stackmaps);
	    
	    stackmaps = CVMstackmapCompute(ee, frame->mb, CVM_TRUE);
	    if (stackmaps == NULL) {
		/* The new stackmaps could not be created: fail */
		return CVM_FALSE;
	    }
            smEntry = CVMgetStackmapEntry(frameEE, frame, jmd, 
				       stackmaps, &missingStackmapOK);
	    CVMassert(smEntry != NULL);
	    return CVM_TRUE;
	}
    }
}

/*
 * Scan a JavaFrame found on the interpreter stack.
 */
static void
CVMjavaFrameScanner(CVMExecEnv* ee,
		    CVMFrame* frame, CVMStackChunk* chunk,
		    CVMRefCallbackFunc callback, void* scannerData,
		    CVMGCOptions* gcOpts)
{
    CVMInterpreterStackData *interpreterStackData =
	(CVMInterpreterStackData *)scannerData;
    void *data = interpreterStackData->callbackData;
    CVMMethodBlock* mb;
    CVMJavaMethodDescriptor* jmd;
    CVMExecEnv *frameEE = interpreterStackData->targetEE;

    CVMStackVal32*   scanPtr;
    CVMObject**      receiverObjPtr;
    CVMStackMaps*    stackmaps;
    CVMStackMapEntry* smEntry;
    CVMUint16*       smChunk;
    CVMUint16        bitmap;
    CVMUint32        bitNo, i;
    CVMUint32        noSlotsToScan;
    CVMUint32        pcOffset;
    CVMBool          missingStackmapOK;

    CVMassert(CVMframeIsJava(frame));

    mb = frame->mb;
    jmd = CVMmbJmd(mb);

    CVMassert(CVMframePc(frame) >= CVMjmdCode(jmd));

    /*
     * This expression is obviously a rather small pointer
     * difference. So just cast it to the type of 'pcOffset'.
     */
    pcOffset = (CVMUint32)(CVMframePc(frame) - CVMjmdCode(jmd));

    CVMassert(pcOffset < CVMjmdCodeLength(jmd));

    CVMtraceGcScan(("Scanning frame for %C.%M (maxLocals=%d, locals=0x%x, stack=0x%x, pc=%d[0x%x])\n",
		    CVMmbClassBlock(frame->mb), frame->mb,
		    CVMmbMaxLocals(frame->mb),
		    CVMframeLocals(frame),
		    CVMframeOpstack(frame, Java),
		    pcOffset,
		    CVMjmdCode(jmd)));

    stackmaps = CVMstackmapFind(ee, mb);

    /* A previous pass ensures that the stackmaps are indeed generated. */
    CVMassert(stackmaps != NULL);

    smEntry = CVMgetStackmapEntry(frameEE, frame, jmd, stackmaps,
                               &missingStackmapOK);
    CVMassert((smEntry != NULL) || missingStackmapOK);
    if (smEntry == NULL) {
	goto done;
    }

    /*
     * Get ready to read the stackmap data
     */
    smChunk = &smEntry->state[0];
    bitmap = *smChunk;

    bitmap >>= 1; /* Skip flag */
    bitNo  = 1;

    /*
     * Scan locals
     */
    scanPtr = (CVMStackVal32*)CVMframeLocals(frame);
    noSlotsToScan = CVMjmdMaxLocals(jmd);

    for (i = 0; i < noSlotsToScan; i++) {
	if ((bitmap & 1) != 0) {
	    CVMObject** slot = (CVMObject**)scanPtr;
	    if (*slot != 0) {
		callback(slot, data);
	    }
	}
	scanPtr++;
	bitmap >>= 1;
	bitNo++;
	if (bitNo == 16) {
	    bitNo = 0;
	    smChunk++;
	    bitmap = *smChunk;
	}
    }

    scanPtr = (CVMStackVal32*)CVMframeOpstack(frame, Java);
    /*
     * This expression is obviously a rather small pointer
     * difference. So just cast it to the type of 'noSlotsToScan'.
     */
    noSlotsToScan = (CVMUint32)(frame->topOfStack - scanPtr);

    /*
     * The stackmaps for the variables and the operand stack are
     * consecutive. Keep bitmap, bitNo, smChunk as is.
     */
    for (i = 0; i < noSlotsToScan; i++) {
	if ((bitmap & 1) != 0) {
	    CVMObject** slot = (CVMObject**)scanPtr;
	    if (*slot != 0) {
		callback(slot, data);
	    }
	}
	scanPtr++;
	bitmap >>= 1;
	bitNo++;
	if (bitNo == 16) {
	    bitNo = 0;
	    smChunk++;
	    bitmap = *smChunk;
	}
    }

    /*
     * The class of the method executing. Do this only if not all classes
     * are roots.
     */
    {
	CVMClassBlock* cb = CVMmbClassBlock(frame->mb);
	CVMscanClassIfNeeded(ee, cb, callback, data);
    }
    
done:

    /*
     * And last but not least, javaframe->receiverObj
     */
    receiverObjPtr = (CVMObject**)&CVMframeReceiverObj(frame, Java);
    if (*receiverObjPtr != 0) {
	callback(receiverObjPtr, data);
    }
}

/*
 * Scan a free-list frame.
 */
static void
CVMfreelistFrameScanner(CVMExecEnv* ee,
			CVMFrame* frame, CVMStackChunk* chunk,
		        CVMRefCallbackFunc callback, void* scannerData,
			CVMGCOptions* opts)
{
    CVMInterpreterStackData *interpreterStackData =
	(CVMInterpreterStackData *)scannerData;
    void *data = interpreterStackData->callbackData;
    CVMassert(CVMframeIsFreelist(frame));
    /*
     * Walk the refs in this free-listing root frame.
     */
    /* See comment in CVMexpandStack that says this assert is true */
    CVMassert(CVMaddressInStackChunk(frame->topOfStack, chunk));
    CVMwalkRefsInGCRootFrame(frame,
			     ((CVMFreelistFrame*)frame)->vals,
			     chunk, callback, data, CVM_TRUE);
}

CVMFrameGCScannerFunc * const CVMframeScanners[CVM_NUM_FRAMETYPES] =
{
#ifdef CVM_DEBUG_ASSERTS
    NULL,			/* CVM_FRAMETYPE_NONE */
#endif
    CVMjavaFrameScanner,	/* CVM_FRAMETYPE_JAVA */
    CVMtransitionFrameScanner,	/* CVM_FRAMETYPE_TRANSITION */
    CVMfreelistFrameScanner,	/* CVM_FRAMETYPE_FREELIST */
    CVMlocalrootFrameScanner,	/* CVM_FRAMETYPE_LOCALROOT */
    CVMglobalrootFrameScanner,	/* CVM_FRAMETYPE_GLOBALROOT */
    NULL,			/* CVM_FRAMETYPE_CLASSTABLE */
#ifdef CVM_JIT
    CVMcompiledFrameScanner,	/* CVM_FRAMETYPE_COMPILED */
#endif
};

/*
 * Return the pc of the exception handler for the specified exception.
 */
/* NOTE: this is needed by JVMTI, so was made nonstatic */
CVMUint8*
CVMgcSafeFindPCForException(CVMExecEnv* ee, CVMFrameIterator* iter, 
			    CVMClassBlock* exceptionClass, CVMUint8* pc)
{
    CVMMethodBlock* mb = CVMframeIterateGetMb(iter);

    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(mb != NULL);

#ifdef CVM_LVM /* %begin lvm */
    /* LVM prohibits user code to execute any exception handler in 
     * process of its termination. The following code is a checks for this.
     * Monitors loked in this frame are unlocked in a way that doesn't
     * depend on exception handlers. */
    if (CVMLVMskipExceptionHandling(ee, iter)) {
	return NULL;
    }
#endif /* %end lvm */

    {
	/* Search current class for catch frame, based on pc value,
	   matching specified type. */
	CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
	CVMExceptionHandler *eh = CVMjmdExceptionTable(jmd);
	CVMExceptionHandler *ehEnd = eh + CVMjmdExceptionTableLength(jmd);
        /*
         * This expression is obviously a rather small pointer
         * difference. So just cast it to the type of 'pcTarget'.
         */
	CVMUint16 pcTarget = 
	    (CVMUint16)(pc - CVMjmdCode(jmd)); /* offset from start of code */
	CVMClassBlock*  cb = CVMmbClassBlock(mb);
        CVMClassLoaderICell* loader = CVMcbClassLoader(cb);
#ifdef CVM_DUAL_STACK
        CVMClassBlock* loaderCB;
#endif
        CVMBool ignoreHandler = CVM_FALSE;
	CVMConstantPool* cp;
#ifdef CVM_JVMTI
	if (CVMjvmtiMbIsObsolete(mb)) {
	    cp = CVMjvmtiMbConstantPool(mb);
	    if (cp == NULL) {
		cp = CVMcbConstantPool(cb);
	    }	
	} else
#endif
	{
	    cp = CVMcbConstantPool(cb);
	}	

        /* If the code in the current frame is not system code,
         * and if the 'ignoreInterruptedException' flag in the 'ee'
         * is set, just ignore the InterruptedException handler.
         * The finally handler is still honored. 
         */
        if (ee->ignoreInterruptedException && loader != NULL) {
#ifdef CVM_DUAL_STACK
            CVMID_objectGetClass(ee, loader, loaderCB);
            if (CVMcbClassName(loaderCB) != CVMglobals.midpImplClassLoaderTid)
#endif
            {
                if (exceptionClass == CVMsystemClass(
                                    java_lang_InterruptedException)) {
                    ignoreHandler = CVM_TRUE;
                }
            }
        }

	/* Check each exception handler to see if the pc is in its range */
	for (; eh < ehEnd; eh++) { 
	    if (eh->startpc <= pcTarget && pcTarget < eh->endpc) { 
		/* The pc is in the handlers range. Now we need to make
		 * sure it handles the specified exception.
		 */
		CVMUint16 catchtype = eh->catchtype;
		CVMClassBlock* catchClass;

		/* Special note put in for finally.  Always succeed. */
		if (catchtype == 0) {
		    goto found;
                }

                if (ignoreHandler) {
                    continue;
                }

		/* See if the class of the exception that the handler catches
		 * is the object's class or one of its superclasses.
		 */
		
#ifdef CVM_CLASSLOADING
		/* Make sure the class of the catchtype is resolved */
		if (!CVMcpResolveEntryFromClass(ee, CVMmbClassBlock(mb),
						cp, catchtype)) {
			return NULL;
		}
#endif
		catchClass = CVMcpGetCb(cp, catchtype);

		/* check the objects class and each superclass for a match */
		{
		    CVMClassBlock* cb;
		    for (cb = exceptionClass; cb != 0;
			 cb = CVMcbSuperclass(cb)) {
			if (cb == catchClass)
			    goto found;
		    }
		}
	    }
	}
	/* not found in this execution environment */
	return NULL;
    found:
	return CVMjmdCode(jmd) + eh->handlerpc;
    }
}

/*
 * Handle the current outstanding exception in the ee. Return the frame
 * that handles the exception after updating the frame's pc.
 *
 * Arguments:
 *    ee:           ee of the thread that needs an exception handled.
 *    frame:        frame to start searching for a handler.
 *    initialframe: last frame in which a handler should be checked for.
 */
CVMFrame*
CVMgcUnsafeHandleException(CVMExecEnv* ee, CVMFrame* frame,
			   CVMFrame* initialframe)
{
    CVMStack*        stack = &ee->interpreterStack;
    CVMClassBlock*   exceptionClass;
    CVMFrameIterator iter;
    CVMBool	     keepGoing;
    CVMFrameFlags    topFlag;

    CVMassert(CVMD_isgcUnsafe(ee));

    /* Search the java stack for the frame that handles the
     * exception. We only search the frames that this invocation
     * of CVMunsafeExecuteJavaMethod pushed. Otherwise we just put the
     * exception back in the ee return to the caller.
     */

    CVMframeIterateInitSpecial(&iter, stack, frame, initialframe);

    keepGoing = CVMframeIteratePopSpecial(&iter, CVM_FALSE);

    frame = CVMframeIterateGetBaseFrame(&iter);

 new_exception_thrown:

    /* Indicate that we're handling an exception because the top most frame may
       not be at a point where stackmaps can be generated: */
    ee->isHandlingAnException = CVM_EXCEPTION_TOP;
    topFlag = CVM_FRAMEFLAG_EXCEPTION;

    /* retrieve the exception object */
    if (CVMlocalExceptionOccurred(ee)) {
	CVMsetCurrentExceptionObj(ee, CVMgetLocalExceptionObj(ee));
	CVMclearLocalException(ee);
    } else {
	CVMsetCurrentExceptionObj(ee, CVMgetRemoteExceptionObj(ee));
	CVMclearRemoteException(ee);
    }

    /* get the cb for the exception object */
    exceptionClass = CVMobjectGetClass(CVMgetCurrentExceptionObj(ee));
	    
#define SET_EXCEPTION_STATE						\
{									\
    CVMUint8 oldIsHandlingAnException = ee->isHandlingAnException;	\
    CVMFrameFlags f0 = frame->flags;					\
    frame->flags = f0 | topFlag;					\

#define UNSET_EXCEPTION_STATE						\
    ee->isHandlingAnException = oldIsHandlingAnException;		\
    frame->flags = f0;							\
}

    while (keepGoing) {
	CVMUint8* pc;
	CVMBool unlockSuccess = CVM_TRUE;
	CVMBool isInlined = CVMframeIterateIsInlined(&iter);
	CVMMethodBlock *mb;

	pc = CVMframeIterateGetJavaPc(&iter);

	/* 
	 * If pc is NULL, then skip this frame. pc can only be NULL if
	 * we had to map from a compiled pc. This is an indication
	 * that the method does not have a handler for the pc.
	 *
	 * Also, skip transition frames since they can't catch exceptions.
	 */
#if 0
	/* Detect bug 4955461 in optimized builds */
	CVMgcStopTheWorldAndGC(ee, ~0);
#endif
	if (pc != NULL && CVMframeIterateHandlesExceptions(&iter)) {
	    /* Need to set exception state before becoming GC safe */
	    SET_EXCEPTION_STATE

	    CVMD_gcSafeExec(ee, {
		/*
		 * CVMgcSafeFindPCForException might call java to do
		 * class loading so we need to save away 
		 * CVMcurrentExceptionICell() so it won't get trashed.
		 */
		CVMID_localrootBegin(ee); {
		    CVMID_localrootDeclare(CVMObjectICell, exceptionICell);
		    CVMID_icellAssign(ee, exceptionICell, 
				      CVMcurrentExceptionICell(ee));
		    CVMID_icellSetNull(CVMcurrentExceptionICell(ee));
		    pc = CVMgcSafeFindPCForException(ee, &iter, 
						     exceptionClass, pc);
		    CVMID_icellAssign(ee, CVMcurrentExceptionICell(ee),
				      exceptionICell);
		} CVMID_localrootEnd();
	    });

	    /*
	       Restore previous value.  This prevents nested
	       exception handling from trashing ee->isHandlingAnException.
	    */
	    UNSET_EXCEPTION_STATE

	    /* If a pc was returned, then that's the pc of the code
	     * that will handle the exception.
	     */
	    if (pc != NULL) {
		CVMframeIterateSetJavaPc(&iter, pc);
		CVMtraceExceptions(("<%d> Exception caught:     %!P\n",
				    ee->threadID, &iter));
		break;
	    }
	}

	/* Looks like this frame doesn't handle the exception.
	 * Pop it and setup state for looking in the next frame.
	 * We also need to unlock the receiverObj if the method
	 * was synchronized. 
	 */

	CVMtraceExceptions(("<%d> Exception not caught: %!P\n",
			    ee->threadID, &iter));

#ifdef CVM_TRACE
	if (!isInlined) {
	    CVMFrame *f = CVMframeIterateGetFrame(&iter);
	    TRACE_METHOD_RETURN(f);
	}
#endif

	mb = CVMframeIterateGetMb(&iter);

	if (CVMmbIs(mb, SYNCHRONIZED)) {
            CVMObjectICell* objICell = CVMframeIterateSyncObject(&iter);
	    if (objICell != NULL) {
		/* %comment l003 */
		unlockSuccess = 
		    CVMobjectTryUnlock(ee, CVMID_icellDirect(ee, objICell));
		if (!unlockSuccess) {
		    /* Need to set exception state before becoming GC safe */
		    SET_EXCEPTION_STATE

		    /* NOTE: CVMobjectUnlock may become safe. */
		    unlockSuccess = CVMobjectUnlock(ee, objICell);

		    /* Restore previous value.  */
		    UNSET_EXCEPTION_STATE
		}
	    }
	}
#ifdef CVM_JVMTI
	/** The JVMTI spec is unclear about whether the frame
	    pop event is sent just before or just after the
	    frame is popped. The JDK implementation sends the
	    event just before the frame is popped. */
	/** Bugid 4191820 seems to imply that the callback must
	    be done before the frame is popped, and that no
	    local variable slots should have been overwritten
	    by the return value. */
	if (CVMjvmtiIsEnabled()) {
	    jvalue val;
	    val.j = CVMlongConstZero();
	    CVMD_gcSafeExec(ee, {
		CVMjvmtiPostFramePopEvent(ee, CVM_FALSE,
					  CVM_TRUE, &val);
	    });
	}
#endif
#ifdef CVM_JVMPI
        if (CVMjvmpiEventMethodExitIsEnabled()) {
            /* NOTE: JVMPI will become GC safe when calling the profiling
             *       agent: */
            CVMjvmpiPostMethodExitEvent(ee);
        }
#endif /* CVM_JVMPI */

	keepGoing = CVMframeIteratePopSpecial(&iter, CVM_FALSE);

        /* As soon as we have popped the frame, then it's OK to expect that
           a stackmap can be generated for the current frame, but only
	   if we didn't just pop an inlined frame.
	*/
	if (!isInlined) {
	    ee->isHandlingAnException = CVM_EXCEPTION_UNWINDING;
	    topFlag = 0;
	    /* new frame */
	    {
		CVMFrame *f = CVMframeIterateGetBaseFrame(&iter);
		CVMassert(f != frame);
		frame = f;
	    }
	} else {
	    CVMassert(frame == CVMframeIterateGetBaseFrame(&iter));
	    CVMassert(keepGoing);
	}

	if (!unlockSuccess) {
	    /* CVMgcSafeFindPCForException() may have thrown an exception */
	    CVMclearLocalException(ee); 
	    CVMthrowIllegalMonitorStateException(
	        ee, "current thread not owner");
	}

	/*
	 * CVMgcSafeFindPCForException() may have thrown an exception,
	 * so we need forget about the original exception and start 
	 * working on the new one.
	 */
	if (CVMlocalExceptionOccurred(ee)) {
	    goto new_exception_thrown;
	}
    }
	   
    frame = CVMframeIterateGetFrame(&iter);

    /* If we didn't find a handler for the exception, then we need
     * to rethrow the exception. Whoever called CVMunsafeExecuteJavaMethod
     * will handle it.
     */
    if (!keepGoing) {
	CVMassert(CVMframeIsTransition(initialframe) && initialframe == frame);
	CVMgcUnsafeThrowLocalException(ee, CVMgetCurrentExceptionObj(ee));
	CVMsetCurrentExceptionObj(ee, NULL);
	/* signal exception not handled */
	frame = initialframe;
    }

    ee->isHandlingAnException = CVM_EXCEPTION_NONE;

    return frame;
}

/*
 * CVMsignalError - signal an exception. The exception message can
 * be a format string followed by arguments just like printf.
 */

static void
CVMgcSafeSignalError(CVMExecEnv* ee, CVMClassBlock* exceptionCb,
		     const char* emessage);

void
CVMsignalError(CVMExecEnv* ee, CVMClassBlock* exceptionCb,
	       const char *format, ...)
{
    va_list argList;
    va_start(argList, format);
    CVMsignalErrorVaList(ee, exceptionCb, format, argList);
    va_end(argList);
}

void
CVMsignalErrorVaList(CVMExecEnv* ee, CVMClassBlock* exceptionCb, 
		     const char* format, va_list ap)
{
#undef CVM_ERR_STRING_BUF_SIZE
#define CVM_ERR_STRING_BUF_SIZE 256
    char *emessageBuf = NULL; 
    char *emessage = NULL; 

#ifdef CVM_JIT
    /* 4975838: Fixup frames. CVMCCMintrinsic_java_lang_System_arraycopy()
     * needs this, and possibly others that don't fixup frames before
     * throwing an exception.
     */
    CVMCCMruntimeLazyFixups(ee);
#endif

    ee->isThrowingAnException = CVM_TRUE;

    /* C stack check */
    if (!CVMCstackCheckSize(ee, CVM_REDZONE_CVMsignalErrorVaList,
                            "CVMsignalErrorVaList", CVM_TRUE)) {
        return;
    }

    if (format != NULL) {
        emessageBuf = (char *)malloc(CVM_ERR_STRING_BUF_SIZE); 
        if (emessageBuf == NULL) 
	    emessage = "<exception message unavailable due to low memory>";
        else { 
	    emessage = emessageBuf;
	    CVMformatStringVaList(emessageBuf, CVM_ERR_STRING_BUF_SIZE, 
	        format, ap);
	}
    }

    if (CVMD_isgcUnsafe(ee)) {
	CVMD_gcSafeExec(ee, {
	    CVMgcSafeSignalError(ee, exceptionCb, emessage);
	});
    } else {
	CVMgcSafeSignalError(ee, exceptionCb, emessage);
    }
    if (emessageBuf != NULL)
        free(emessageBuf);
}

static void
throwPreallocatedOutOfMemoryError(CVMExecEnv *ee, CVMClassBlock* exceptionCb)
{
    CVMdebugPrintf(("Could not allocate exception object \"%C\"; "
		    "throwing a pre-allocated "
		    "OutOfMemoryError object instead\n",
		    exceptionCb));
    CVMgcSafeThrowLocalException(ee, CVMglobals.preallocatedOutOfMemoryError);
}

static void
CVMgcSafeSignalError(CVMExecEnv* ee, CVMClassBlock* exceptionCb,
		     const char* emessage)
{
    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(!CVMlocalExceptionOccurred(ee));
#if CVM_DEBUG
    /* We better not be holding a microlock */
    CVMassert(ee->microLock == 0);
#endif

    CVMtraceExceptions(("[<%d> Signaling Error: %C, \"%s\"]\n", 
			ee->threadID, exceptionCb,
			emessage ? emessage : ""));

#if defined(CVM_TRACE) && defined(CVM_DEBUG_DUMPSTACK)
    if (CVMcheckDebugFlags(CVM_DEBUGFLAG(TRACE_EXCEPTIONS))) {
	CVMdumpStack(&ee->interpreterStack, CVM_FALSE, CVM_FALSE, 100);
    }
#endif

    /*
     * Instantiate and throw the requested exception object.
     */
    {
	/* 
	 * We borrow the CVMcurrentExceptionICell to avoid having to 
	 * allocate a localref here, which has infinite recursion
	 * issues if there is a StackOverflowError.
	 */
	CVMThrowableICell* exceptionICell = CVMcurrentExceptionICell(ee);

	/*
	 * Instantiate the exception object. If we can't then 
	 * throw a preallocated object.
	 */
	CVMID_allocNewInstance(ee, exceptionCb, exceptionICell);
	if (CVMID_icellIsNull(exceptionICell)) {
            throwPreallocatedOutOfMemoryError(ee, exceptionCb);
	    return;
	}

	/* 
	 * If a message string was passed in, then allocate a String object
	 * for it and assign it to the Throwable.msg field. We don't care if
	 * CVMnewStringUTF fails. The msg field just ends up being null
	 * in that case.
	 */
	if (emessage) {
	/* 
	 * We borrow the CVMlocalExceptionICell to avoid having to 
	 * allocate a localref here, which has infinite recursion
	 * issues if there is a StackOverflowError.
	 */
	    CVMStringICell* stringICell = CVMlocalExceptionICell(ee);
	    CVMassert(CVMID_icellIsNull(stringICell));
	    CVMnewStringUTF(ee, stringICell, emessage);
	    CVMID_fieldWriteRef(ee, exceptionICell, 
				CVMoffsetOfjava_lang_Throwable_detailMessage,
				stringICell);
            if (CVMID_icellIsNull(stringICell)) {
                throwPreallocatedOutOfMemoryError(ee, exceptionCb);
                return;
            } else {
                /* Set stringICell to null to avoid potential problems. */
                CVMID_icellSetNull(stringICell);
            }
	}

	/* Fill in the backtrace into the exception object. */
	CVMfillInStackTrace(ee, exceptionICell);

	/* Throw the exception object. */
   	if (!CVMlocalExceptionOccurred(ee)) {
	   CVMgcSafeThrowLocalException(ee, exceptionICell);
	}

	/* Set exceptionICell to null to avoid potential problems. */
	CVMID_icellSetNull(exceptionICell);
    }
}

/*
 * CVMpushTransitionFrame - pushes a transition frame. Called when we
 * are about to make a method call from native code. Transition frames store
 * the method arguments and result and also have the benefit of
 * not requiring the intepreter loop to check for (frame == initialframe)
 * each time a frame is popped. Instead, the opc_exittransition opcode
 * in the transition frame causes the interpreter loop to exit. They also
 * simplify the pushing of the CVMJavaFrame or CVMJNIFrame by leaving it
 * up to the interpreter to handle it in the normal way.
 */
CVMTransitionFrame* CVMpushTransitionFrame(CVMExecEnv* ee, CVMMethodBlock* mb)
{
    CVMFrame* frame = CVMeeGetCurrentFrame(ee);

    CVMassert(CVMD_isgcUnsafe(ee));

    /* Setting the frame's mb to the mb of the method we want to invoke.
     * We really don't need an mb for the transition frame,
     * but we do need to know what the arguments are so they can be
     * scanned. The arguments of the method we are going to call serve
     * us well, so we use them by saving away the mb in the frame.
     * Also, JVMPI wants it.
     */

    CVMpushFrame(ee, &ee->interpreterStack, frame, frame->topOfStack,
		 /* transition frame size in words plus argument size */
		 CVM_TRANSITIONFRAME_SIZE / sizeof(CVMStackVal32)
		 + (CVMmbArgsSize(mb) > 2 ? CVMmbArgsSize(mb) : 2),
		 0,         /* frame offset (no local variables) */
		 CVM_FRAMETYPE_TRANSITION, mb,
		 CVM_TRUE, /* commit */
		 /* action if pushFrame fails */
		 {
		     CVMdebugPrintf(("CVMsetupTransitionFrame - "
				     "CVMpushFrame failed.\n"));
		     return NULL;  /* exception already thrown */
		 },
		 /* action to take if stack expansion occurred */
		 {});

    /* Initialize the transition frame. */
    CVM_RESET_TRANSITION_TOS(frame->topOfStack, frame);
    CVMassert(ee->interpreterStack.currentFrame == frame);

    /* be default, don't let opc_exittransition increment the pc. */
    CVMframeIncrementPcFlag(frame) = CVM_FALSE;

    /*
     * We must do this while still gcUnsafe or CVMtransitionFrameScanner()
     * will crash if a gc starts up before CVMframePc() gets initialized
     * to its proper value.
     */
    CVMframePc(frame) = NULL;


    return (CVMTransitionFrame*)frame;    
}


/*
 * Initializes the frame iterator.
 *
 * "firstFrame" is the first frame.  Iteration proceeds to and includes
 * "lastFrame", so specifying lastFrame == firstFrame will scan only the
 * first frame (but including its JIT inlined frames).  Currently,
 * "stack" is only used to support "popFrame" below.
 *
 * Note: The specified firstFrame will be set up as the first frame to be
 *       examined when CVMframeIterateNext() is called.  The initial
 *       current frame is set to NULL to indicate that we haven't iterated
 *       through any frames yet.
 */
void
CVMframeIterateInitSpecial(CVMFrameIterator *iter, CVMStack *stack,
			   CVMFrame* firstFrame, CVMFrame *lastFrame)
{
    iter->stack = stack;
    iter->endFrame = lastFrame;
    iter->frame = NULL;
    iter->next = firstFrame;
#ifdef CVM_JIT
    iter->jitFrame = CVM_FALSE;
#endif
}

/* Initializes the frame iterator with the first frame to iterate from. */
void
CVMframeIterateInit(CVMFrameIterator *iter, CVMFrame* firstFrame)
{
    CVMframeIterateInitSpecial(iter, NULL, firstFrame, NULL);
}

CVMFrameFlags
CVMframeIterateGetFlags(CVMFrameIterator *iter)
{
#ifdef CVM_JIT
    if (iter->jitFrame) {
	return CVMJITframeIterateGetFlags(&iter->jit);
    } else
#endif
    {
	return (CVMFrameFlags)iter->frame->flags;
    }
}

CVMBool
CVMframeIterateIsInlined(CVMFrameIterator *iter)
{
#ifdef CVM_JIT
    if (iter->jitFrame) {
	 return CVMJITframeIterateIsInlined(&iter->jit);
    }
#endif
    return CVM_FALSE;
}

CVMBool
CVMframeIterateHandlesExceptions(CVMFrameIterator *iter)
{
#ifdef CVM_JIT
    if (iter->jitFrame) {
	 return CVMJITframeIterateHandlesExceptions(&iter->jit);
    }
#endif
    return !CVMframeIsTransition(iter->frame);
}

#ifdef CVM_JVMTI
CVMBool
CVMframeIterateCanHaveJavaCatchClause(CVMFrameIterator *iter)
{
#ifdef CVM_JIT
    if (iter->jitFrame) {
	 return CVMJITframeIterateHandlesExceptions(&iter->jit);
    }
#endif
    return CVMframeIsJava(iter->frame);
}
#endif

void
CVMframeIterateSetFlags(CVMFrameIterator *iter, CVMFrameFlags flags)
{
#ifdef CVM_JIT
    if (iter->jitFrame) {
	CVMJITframeIterateSetFlags(&iter->jit, flags);
    } else
#endif
    {
	iter->frame->flags = flags;
    }
}

/*
 * "skip" is how many extra frames to skip.  Use skip==0 to see
 * every frame.  To skip reflection frames, set skipReflection
 * true.  "popFrame" is used by exception handling to pop frames
 * as it iterates.
 * Note: transition frames and JNI local frames (i.e. mb == 0) will always be
 *       skipped.  All other frames will not be skipped except for reflection
 *       frames depending on the value of the skipReflection.
 */
CVMBool
CVMframeIterateSkipReflection(CVMFrameIterator *iter,
    int skip, CVMBool skipReflection, CVMBool popFrame)
{
    CVMFrame *frame = iter->next;
    CVMFrame *lastFrame = iter->endFrame;
    CVMBool stop = CVM_FALSE;

#ifdef CVM_JIT
    if (iter->jitFrame) {
	if (!CVMJITframeIterateSkip(&iter->jit, skipReflection, popFrame)) {
	    iter->jitFrame = CVM_FALSE;
	    goto again;
	}
    } else
#endif
    /* Not first time through? */
    if (frame == iter->frame) {
	goto again;
    }

    while (frame != NULL) {

	if (CVMframeIsTransition(frame) || frame->mb == NULL) {
	    goto again;
	}

#ifdef CVM_JIT
	if (!iter->jitFrame && CVMframeIsCompiled(frame)) {
	    CVMJITframeIterate(frame, &iter->jit);
	    if (CVMJITframeIterateSkip(&iter->jit, skipReflection, popFrame)) {
		iter->jitFrame = CVM_TRUE;
	    } else {
		goto again;
	    }
	}
	if (iter->jitFrame) {
	    if (skip > 0) {
		do {
		    if (!CVMJITframeIterateSkip(&iter->jit, skipReflection,
			popFrame))
		    {
			iter->jitFrame = CVM_FALSE;
			goto jit_done;
		    }
		} while (--skip > 0);
	    }
	    CVMassert(skip == 0);
	    break;
	}
#endif
	/* When skipReflection is set, all articifial frames (which is
	   currently only used by the reflection mechanism) will be skipped. */
	if (skipReflection && (frame->flags & CVM_FRAMEFLAG_ARTIFICIAL) != 0) {
	    goto again;
	}

#ifdef CVM_JIT
jit_done:
#endif
	if (skip > 0) {
	    --skip;
	} else {
	    break;
	}
again:
	if (frame == lastFrame) {
	    stop = CVM_TRUE;
	    break;
	}

	if (popFrame) {
	    CVMpopFrame(iter->stack, frame);
	    CVMassert(iter->stack->currentFrame == frame);
	} else {
	    frame = CVMframePrev(frame);
	}
    }

#ifdef CVM_JIT
    if (popFrame && iter->jitFrame) {
	CVMCompiledMethodDescriptor *cmd = CVMmbCmd(frame->mb);
	CVMStackVal32* base = (CVMStackVal32 *)frame -
	    CVMcmdMaxLocals(cmd);
	/* Set the topOfStack to its max capacity point.  This guarantees
	   that any frames pushed on top of it will not trash the
	   the contents of this frame: */
	frame->topOfStack = base + CVMcmdCapacity(cmd);
	CVMassert(CVMframeIsCompiled(frame));
#ifdef CVM_JVMTI
	/* NOTE: The default implementation below will result in junk
	   appearing in the stack frame.  Need to set it to the proper
	   frame value if we ever support JVMPI or JVMTI debug mode
	   with JITed code.

	   Hence, we better not be in debugging mode.
	*/
	CVMassert(!CVMjvmtiIsInDebugMode());
#endif
    }
#endif
    iter->frame = frame;
    iter->next = frame;
    return frame != NULL && !stop;
}

CVMMethodBlock *
CVMframeIterateGetMb(CVMFrameIterator *iter)
{
#ifdef CVM_JIT
    if (iter->jitFrame) {
	return CVMJITframeIterateGetMb(&iter->jit);
    }
#endif
    return iter->frame->mb;
}

/*
  Note that there is no physical representation of the logical frame for each
  JIT inlined method.  Hence, you cannot ask the iterator to return a
  CVMFrame * for an inlined method.  CVMframeIteratorGetFrame() returns the
  current physical frame i.e. the outer most caller of the inlined method, not
  the current logical frame for the inlined method.
*/
CVMFrame *
CVMframeIterateGetFrame(CVMFrameIterator *iter)
{
#ifdef CVM_JIT
    if (iter->jitFrame) {
	return CVMJITframeIterateGetFrame(&iter->jit);
    }
#endif
    return iter->frame;
}

static CVMFrame *
CVMframeIterateGetBaseFrame(CVMFrameIterator *iter)
{
    return iter->frame;
}

CVMUint8 *
CVMframeIterateGetJavaPc(CVMFrameIterator *iter)
{
    /*
     * Get the java pc of the frame. This is a bit tricky for compiled
     * frames since we need to map from the compiled pc to the java pc.
     */
#ifdef CVM_JIT
    if (iter->jitFrame) {
	return CVMJITframeIterateGetJavaPc(&iter->jit);
    }
#endif
    return CVMframePc(iter->frame);
}

void
CVMframeIterateSetJavaPc(CVMFrameIterator *iter, CVMUint8 *pc)
{
#ifdef CVM_JIT
    if (iter->jitFrame) {
	CVMJITframeIterateSetJavaPc(&iter->jit, pc);
	return;
    }
#endif
    CVMframePc(iter->frame) = pc;
}

CVMStackVal32 *
CVMframeIterateGetLocals(CVMFrameIterator *iter)
{
#ifdef CVM_JIT
    if (iter->jitFrame) {
	return CVMJITframeIterateGetLocals(&iter->jit);
    }
#endif
    {
	CVMFrame *frame = iter->frame;
	int numberOfLocals = CVMjmdMaxLocals(CVMmbJmd(frame->mb));
	return (CVMStackVal32 *)frame - numberOfLocals;
    }
}

CVMObjectICell *
CVMframeIterateSyncObject(CVMFrameIterator *iter)
{
#ifdef CVM_JIT
    if (iter->jitFrame) {
	return CVMJITframeIterateSyncObject(&iter->jit);
    }
#endif
    if (CVMframeIsJava(iter->frame)) {
	return &CVMframeReceiverObj(iter->frame, Java);
    } else {
	return NULL;
    }
}

CVMUint32
CVMframeIterateCount(CVMFrameIterator *iter)
{
    CVMUint32 count = 0;
    while (CVMframeIterateNext(iter)) {
	++count;
    }
    return count;
}

void
CVMframeSetContextArtificial(CVMExecEnv *ee)
{
    CVMFrameIterator iter;
    CVMframeIterateInit(&iter, CVMeeGetCurrentFrame(ee));
    CVMframeIterateSkipReflection(&iter, 0, CVM_FALSE, CVM_FALSE);
    CVMframeIterateSetFlags(&iter, (CVMFrameFlags)
	(CVMframeIterateGetFlags(&iter) | CVM_FRAMEFLAG_ARTIFICIAL));
}

CVMMethodBlock *
CVMgetCallerMb(CVMFrame* frame, int skip)
{
    CVMFrameIterator iter;
    CVMframeIterateInit(&iter, frame);
    if (CVMframeIterateSkip(&iter, skip)) {
	return CVMframeIterateGetMb(&iter);
    } else {
	return NULL;
    }
}

/* Documented in interpreter.h */
CVMClassBlock*
CVMgetCallerClass(CVMExecEnv* ee, int skip)
{
    CVMMethodBlock *mb = CVMgetCallerMb(CVMeeGetCurrentFrame(ee), skip);
    if (mb != NULL) {
	return CVMmbClassBlock(mb);
    } else {
	return NULL;
    }
}

/* Documented in interpreter.h */
CVMFrame*
CVMgetCallerFrameSpecial(CVMFrame* frame, int n, CVMBool skipReflection)
{
    CVMFrameIterator iter;
    CVMframeIterateInit(&iter, frame);
    if (CVMframeIterateSkipReflection(&iter, n, skipReflection, CVM_FALSE)) {
	return CVMframeIterateGetFrame(&iter);
    }
    return NULL;
}

/*
 * Check whether srcCb is assignable to dstCb
 *
 * May cause an exception to be thrown, but it will always be a 
 * preallocated StackOverflowError, so state does not need to be flushed.
 */
CVMBool
CVMisAssignable(CVMExecEnv* ee, CVMClassBlock* srcCb, 
		CVMClassBlock* dstCb)
{
    if (!CVMisArrayClass(srcCb)) {
	/* srcCb is an object class. Check subclass relationship */
        return CVMisSubclassOf(ee, srcCb, dstCb);
    } else {
	if (CVMisArrayClass(dstCb)) {
	    /*
	     * Both srcCb and dstCb are arrays. Check array assignability.
	     */
	    return CVMarrayIsAssignable(ee, srcCb, dstCb);
	} else {
	    /*
	     * If srcCb is an array class and dstCb is not,
	     * then only the following assignments are legal.
	     */
	    return (dstCb == CVMsystemClass(java_lang_Object) || 
		    dstCb == CVMsystemClass(java_lang_Cloneable) || 
		    dstCb == CVMsystemClass(java_io_Serializable));
	}
    }
}

/*
 * CVMgcUnsafeIsInstanceOf - Check if obj is an instance of cb. obj can be an
 * array object.
 *
 * May cause an exception to be thrown, but it will always be a 
 * preallocated StackOverflowError, so state does not need to be flushed.
 */
CVMBool
CVMgcUnsafeIsInstanceOf(CVMExecEnv* ee, CVMObject* obj, 
			CVMClassBlock* cb)
{
    CVMassert(CVMD_isgcUnsafe(ee));
    /* null can be cast to anything EXCEPT in the interpreter
       (opc_instanceof)! */
    if (obj == 0) {            
        return CVM_TRUE;
    } 
    return CVMisAssignable(ee, CVMobjectGetClass(obj), cb);

}


/*
 * CVMisSubclassOf - Check if subclasscb is a subclass of cb. subclasscb
 * must be a non-array class.
 *
 * May cause an exception to be thrown, but it will always be a 
 * preallocated StackOverflowError, so state does not need to be flushed.
 *
 * -if cb is a class type, subclasscb must be the same class as cb,
 *  or a subclass of cb.
 * -if cb is an interface type, subclasscb must implement interface cb.
 * 
 */
CVMBool
CVMisSubclassOf(CVMExecEnv* ee, CVMClassBlock* subclasscb, 
		CVMClassBlock* cb)
{
    if (subclasscb == cb) {
	return CVM_TRUE;
    }

    if (CVMcbIs(cb, INTERFACE)) {
	/*
	 * CVMimplementsInterface checks the subclasscb's CVMInterfaceTable,
	 * which is a sumary of all the interfaces subclasscb implements.
	 * Therefore, there is no need to iterate over superclasses.
	 */
	return CVMimplementsInterface(ee, subclasscb, cb);
    } else {
	/* cb is a class type */ 
	subclasscb = CVMcbSuperclass(subclasscb);
	while (subclasscb != NULL) {
            if (subclasscb == cb) {
                return CVM_TRUE;
	    }
            subclasscb = CVMcbSuperclass(subclasscb);
        }
	return CVM_FALSE;
    }
}


/*
 * CVMextendsClass - Check if subclasscb is a subclass of cb without
 * considering implemented interfaces.  subclasscb must be a non-array class.
 */
CVMBool
CVMextendsClass(CVMExecEnv* ee, CVMClassBlock* subclasscb, CVMClassBlock* cb)
{
    if (subclasscb == cb) {
        return CVM_TRUE;
    }

    /* cb is a class type */ 
    subclasscb = CVMcbSuperclass(subclasscb);
    while (subclasscb != NULL) {
        if (subclasscb == cb) {
            return CVM_TRUE;
        }
        subclasscb = CVMcbSuperclass(subclasscb);
    }
    return CVM_FALSE;
}

/* 
 * Return TRUE if srcArrayCb is assignable to dstArrayCb
 */
static CVMBool
CVMarrayIsAssignable(CVMExecEnv* ee,
		     CVMClassBlock* srcArrayCb,
		     CVMClassBlock* dstArrayCb)
{
    int              srcArrayDepth     = CVMarrayDepth(srcArrayCb);
    CVMBasicType     srcArrayBaseType  = CVMarrayBaseType(srcArrayCb);
    CVMClassBlock*   srcArrayBaseClass = CVMarrayBaseCb(srcArrayCb);

    int              dstArrayDepth     = CVMarrayDepth(dstArrayCb);
    CVMBasicType     dstArrayBaseType  = CVMarrayBaseType(dstArrayCb);
    CVMClassBlock*   dstArrayBaseClass = CVMarrayBaseCb(dstArrayCb);

    if (srcArrayDepth > dstArrayDepth) {
	/*
	 * The source array is 'deeper' than the destination array.
	 * The only way they could be assignable is if the destination
	 * array is an array of Object, Cloneable or Serializable.
	 */
        return (dstArrayBaseClass == CVMsystemClass(java_lang_Object) ||
                dstArrayBaseClass == CVMsystemClass(java_lang_Cloneable) ||
                dstArrayBaseClass == CVMsystemClass(java_io_Serializable));
    } else if (srcArrayDepth == dstArrayDepth) {
	/*
	 * Equal depths.
	 * 
	 * If the base types of both arrays are not of the same category
	 * they are definitely not assignable
	 */
	if (dstArrayBaseType != srcArrayBaseType) {
	    return CVM_FALSE;
	}

	/*
	 * Now the classes are assignable iff:
	 * 
	 * both base types are the same non-object basic type
	 * 
	 *   or
	 *
	 * both base types are object types and the source type
	 * is a subclass of the destination type.
	 */
	return ((srcArrayBaseType != CVM_T_CLASS) ||
		(CVMisSubclassOf(ee, srcArrayBaseClass, dstArrayBaseClass)));
    } else {
	/*
	 * The destination array is 'deeper' than the source array,
	 * so they are not assignable.
	 */
        return CVM_FALSE;
    }
}

/* 
 * Return TRUE if cb implements the interface interfacecb.
 *
 * May cause an exception to be thrown, but it will always be a 
 * preallocated StackOverflowError, so state does not need to be flushed.
 */
CVMBool
CVMimplementsInterface(CVMExecEnv* ee, CVMClassBlock* cb, 
		       CVMClassBlock* interfacecb)
{
    int i;

    if (CVMcbCheckRuntimeFlag(cb, LINKED)) {
	/*
	 * If the class has already been linked, then the CVMInterfaceTable
	 * contains all interfaces that the class implements. There is no
	 * need to look at super classes or super interfaces.
	 */
	for ( i = 0; i < CVMcbInterfaceCount(cb); i++) {
	    if (CVMcbInterfacecb(cb, i) == interfacecb) {
		return CVM_TRUE;
	    }
	}
	return CVM_FALSE;
    } else {
	/*
	 * If the class has not been linked, then only the declared interfaces
	 * are in the CVMInterfaceTable. In order to check all of the
	 * class' implemented interfaces, we need to call CVMisSubclassOf()
	 * for each declared interface of the class and for each declared
	 * interface of each superclass.
	 */
	/* C stack checks */
        if (!CVMCstackCheckSize(ee, CVM_REDZONE_CVMimplementsInterface, 
	    "CVMimplementsInterface", CVM_TRUE)) {
	    return CVM_FALSE;
	}

	do {
            CVMClassBlock * subclasscb;
            for (i = 0; i < CVMcbImplementsCount(cb); i++) {
                subclasscb = CVMcbInterfacecb(cb, i);
                if (subclasscb == interfacecb) {
                    return CVM_TRUE;
                }
                else if (CVMimplementsInterface(ee, subclasscb, interfacecb)){
                    return CVM_TRUE;
                }
                else if (CVMlocalExceptionOccurred(ee)) {
                    return CVM_FALSE;
                }
            }
            cb = CVMcbSuperclass(cb);
	} while (cb != NULL);
	return CVM_FALSE;
    }
}

/*
 * Multi-dimensional array creation
 */
/* 
 * CVMmultiArrayAlloc() is called from java opcode multianewarray
 * and passes the top of stack as parameter dimensions.
 * Because the width of the array dimensions is obtained via
 * dimensions[i], dimensions has to be of the same type as 
 * the stack elements for proper access.
 */
void
CVMmultiArrayAlloc(CVMExecEnv*     ee,
		   CVMInt32        nDimensions,
		   CVMStackVal32*  dimensions,
		   CVMClassBlock*  arrayCb,
		   CVMObjectICell* resultCell)
{
    /* The algorithm for allocating the tree of arrays is not as straight-
       forward as on first glance.  Here's how it is actually done by the
       code below:

       Let's say that we're going to allocate the following array tree:
       
                                        A[]
                                         |
                   ,---------------------+---------------------.
		   |                     |                     |
                  B1[]                  B2[]                  B3[]
                   |                     |                     |
            ,------+------.       ,------+------.       ,------+------.
            |      |      |       |      |      |       |      |      |
          C11[]  C12[]  C13[]   C21[]  C22[]  C23[]   C31[]  C32[]  C33[]


       While filling in the elements of A with B1, B2, and B3, we also need
       to keep track of B1, B2, and B3 so that we can allocate the element
       arrays at the next level.  This is done by chaining B1, B2, and B3
       into a link list.  The head of the list is currNodeSet.

       Once we are done allocating the B level, we iterate and allocate the
       C level.  At this point, we move the currNodeSet list over to another
       list, prevNodeSet.

           For each node in prevNodeSet, we allocate it's elements at the C
       level.  As we allocate the C elements, we insert them into the new
       currNodeSet list in order to allocate their elements for the
       subsequent level.

           However, if the C level is the last dimension and there are no
       more elements to be allocated beneath it, then we don't need to
       chain it into the currNodeSet list.
     */

    CVMID_localrootBegin(ee); {
	/*
	 * Temporary array to hold the root of the tree
	 */
	CVMID_localrootDeclare(CVMArrayOfRefICell, rootArray);

	/*
	 * The level of the tree we are constructing right now.
	 */
	CVMInt32 currDepth;

	/*
	 * The siblings at level currDepth - 1, each one an array of length
	 * 'prevNodeWidth', linked by their 0th elements.
	 */
	CVMID_localrootDeclare(CVMArrayOfRefICell, prevNodeSet);
	CVMInt32 prevNodeWidth;

	/*
	 * The node we are currently filling in at level currDepth - 1
	 */
	CVMID_localrootDeclare(CVMArrayOfRefICell, prevOneNode);

	/*
	 * The siblings at level currDepth, each one an array of length
	 * 'currNodeWidth', being created and linked together.  
	 */
	CVMID_localrootDeclare(CVMArrayOfRefICell, currNodeSet);
	CVMInt32 currNodeWidth;

	/*
	 * The node we are currently creating at level currDepth.
	 */
	CVMID_localrootDeclare(CVMArrayOfRefICell, currOneNode);

	/*
	 * The type of the arrays we are constructing at level currDepth.
	 */
	CVMClassBlock* currCb;

	/*
	 * rootArray - the root of the tree. rootArray[0] will hold
	 * the final created array (and keep the whole thing from
	 * falling apart in case GC occurs).
	 */
	CVMID_allocNewArray(ee, 
			    CVM_T_CLASS,
			    (CVMClassBlock*)
			    CVMbasicTypeArrayClassblocks[CVM_T_CLASS],
			    1, 
			    (CVMObjectICell*)rootArray);

	if (CVMID_icellIsNull(rootArray)) {
	    /*
	     * returning 0
	     */
	    CVMID_icellSetNull(resultCell);
	    goto endOfJob;
	}

	CVMID_icellAssign(ee, prevNodeSet, rootArray);
	prevNodeWidth = 1;
	currCb = arrayCb;

	/*
	 * We now have rootArray acting as the previous level to level 0.
	 * It has only one slot, and the allocator has already set it to 0,
	 * so it will appear as a linked list with one node alone.
	 *
	 * Now go through all the dimensions and do the work
	 */
	for (currDepth = 0; currDepth < nDimensions; currDepth++) {
	    CVMBool lastIteration;

	    /*
	     * Each node created at this level will be currNodeWidth wide
	     */
	    currNodeWidth = dimensions[currDepth].j.i;
	    CVMID_icellSetNull(currNodeSet);

	    /*
	     * If we are at the last iteration, remember that. In this
	     * case, we will not be linking nodes created at level
	     * currDepth.
	     */
	    lastIteration = (currDepth == nDimensions - 1);

	    /*
	     * Go through each of the previous level's nodes, allocate
	     * and link the current nodes if needed, and fill them in
	     * the slots of the previous level nodes.  
	     */
	    do {
		CVMUint32 idx;
		/*
		 * Keep the node and advance to the next link before
		 * we bash the first element.
		 */
		CVMID_icellAssign(ee, prevOneNode, prevNodeSet);
		CVMID_arrayReadRef(ee, prevNodeSet, 0,
				   (CVMObjectICell*)prevNodeSet);
		for (idx = 0; idx < prevNodeWidth; idx++) {
		    /*
		     * Allocate and link
		     */
		    CVMID_allocNewArray(ee,
					CVMarrayElemTypeCode(currCb),
					currCb,
					currNodeWidth,
					(CVMObjectICell*)currOneNode);
		    if (CVMID_icellIsNull(currOneNode)) {
			/*
			 * returning 0
			 */
			CVMID_icellSetNull(resultCell);
			goto endOfJob;
		    }
		    /*
		     * Link the children only if we are going to 
		     * iterate again.
		     */
		    if (!lastIteration) {
			CVMID_arrayWriteRef(ee, currOneNode, 0,
					    (CVMObjectICell*)currNodeSet);
			CVMID_icellAssign(ee, currNodeSet, currOneNode);
		    }
		    /*
		     * And fill a parent slot
		     */
		    CVMID_arrayWriteRef(ee, prevOneNode, idx,
					(CVMObjectICell*)currOneNode);
		}
	    } while (!CVMID_icellIsNull(prevNodeSet));

	    if (!lastIteration) {
		/*
		 * We've filled level currDepth. On to the next.
		 */
		CVMID_icellAssign(ee, prevNodeSet, currNodeSet);
		prevNodeWidth = currNodeWidth;
		currCb = CVMarrayElementCb(currCb);
		/*
		 * Make sure that we aggressively create all sub array
		 * classes whenever an array class is created.
		 */
		CVMassert(currCb != 0);
	    }
	}
	/*
	 * Now rootArray[0] has what we want. Send it back to the caller.
	 */
	CVMID_arrayReadRef(ee, rootArray, 0, resultCell);

    } 
    /* 
     * We might get here if an error occurred in the above. We've set
     * resultCell to contain 0, so the caller will handle it.  
     */
 endOfJob: 
    CVMID_localrootEnd();
}

/*
 * Functions for enabling and disabling remote exceptions. It's only safe
 * to call these functions if ee is for the current thread.
 */

#ifdef CVM_REMOTE_EXCEPTIONS_SUPPORTED

void
CVMdisableRemoteExceptions(CVMExecEnv* ee)
{
    CVMassert(ee == CVMgetEE());
    ee->remoteExceptionsDisabledCount++;
}

void
CVMenableRemoteExceptions(CVMExecEnv* ee)
{
    CVMassert(ee == CVMgetEE());
    CVMassert(ee->remoteExceptionsDisabledCount > 0);
    ee->remoteExceptionsDisabledCount--;
    return;
}

CVMBool
CVMremoteExceptionsDisabled(CVMExecEnv* ee) {
    CVMassert(ee == CVMgetEE());
    return (ee->remoteExceptionsDisabledCount != 0);
}

#endif

/*
 * Manage interrupts.  Should only be called for
 * the current thread.
 */
CVMBool
CVMmaskInterrupts(CVMExecEnv* ee)
{
    CVMBool wasMasked = ee->interruptsMasked;

    CVMassert(ee == CVMgetEE());

    CVMsysMutexLock(ee, &CVMglobals.threadLock);

    ee->interruptsMasked = CVM_TRUE;
    if (!wasMasked) {
	/* first time */
	ee->maskedInterrupt =
	    CVMthreadIsInterrupted(CVMexecEnv2threadID(ee), CVM_TRUE);
    }

    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);

    return wasMasked;
}

void
CVMunmaskInterrupts(CVMExecEnv *ee)
{
    CVMassert(ee->interruptsMasked);
    CVMassert(ee == CVMgetEE());

    CVMsysMutexLock(ee, &CVMglobals.threadLock);

    ee->interruptsMasked = CVM_FALSE;
    if (ee->maskedInterrupt) {
	ee->maskedInterrupt = CVM_FALSE;
	CVMthreadInterruptWait(CVMexecEnv2threadID(ee));
    }

    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

/* Documented in interpreter.h */
CVMBool
CVMverifyClassAccess(CVMExecEnv* ee, 
		     CVMClassBlock* currentClass, CVMClassBlock* newClass, 
		     CVMBool resolverAccess)
{
    if (currentClass == NULL ||
	currentClass == newClass ||
	CVMcbIs(newClass, PUBLIC) ||
	CVMisSameClassPackage(ee, currentClass, newClass)) {
        return CVM_TRUE;
    }

#ifdef CVM_CLASSLOADING
    /* 
     * One last chance to pass verification. See similar code in 
     * CVMverifyMemberAccess3 for details.
     */
    if (resolverAccess) {	 	/* (a) */
	CVMBool needsVerify;   		/* (b) */
	CVMBool isSameClassLoader;      /* (c) */
	CVMBool isSameProtectionDomain; /* (d) */

	needsVerify = 
	    CVMloaderNeedsVerify(ee, CVMcbClassLoader(currentClass),
                                 CVM_FALSE);
	isSameClassLoader =
	    CVMcbClassLoader(currentClass) == CVMcbClassLoader(newClass);
	CVMID_icellSameObjectNullCheck(ee,
				       CVMcbProtectionDomain(currentClass), 
				       CVMcbProtectionDomain(newClass), 
				       isSameProtectionDomain);

	/* Make sure (b), (c), and (d) hold true. */
	if (!needsVerify
	    && isSameProtectionDomain
	    && isSameClassLoader)
	{
	    return CVM_TRUE;
	}
    }
#endif
   return CVM_FALSE;
}

static CVMBool
IsAncestor(CVMClassBlock *currentClass, CVMClassBlock *memberClass)
{
    CVMClassBlock* cb;
    for (cb = CVMcbSuperclass(currentClass); cb != NULL;
         cb = CVMcbSuperclass(cb)) {
        if (cb == memberClass) {
            return CVM_TRUE;
        }
    }
    return CVM_FALSE;
} 

/* Documented in interpreter.h */
CVMBool
CVMverifyMemberAccess2(CVMExecEnv* ee,
		       CVMClassBlock* currentClass,
		       CVMClassBlock* memberClass, 
		       CVMUint32 access, CVMBool resolverAccess,
		       CVMBool protectedRestriction)
{
    return CVMverifyMemberAccess3(ee, currentClass, currentClass, memberClass,
                               access, resolverAccess, protectedRestriction);
}

CVMBool
CVMverifyMemberAccess3(CVMExecEnv* ee,
                       CVMClassBlock* currentClass,
                       CVMClassBlock* resolvedClass,
                       CVMClassBlock* memberClass,
                       CVMUint32 access, CVMBool resolverAccess,
                       CVMBool protectedRestriction)
{
    /* Since we're going to be doing some private stuff here,
       let's make sure some invariants are met */
    CVMassert(CVM_METHOD_ACC_PUBLIC == CVM_FIELD_ACC_PUBLIC);
    CVMassert(CVM_METHOD_ACC_PROTECTED == CVM_FIELD_ACC_PROTECTED);
    CVMassert(CVM_METHOD_ACC_PRIVATE == CVM_FIELD_ACC_PRIVATE);

#undef IsPublic
#undef IsProtected
#undef IsPrivate
#define IsPublic(x)	CVMmemberPPPAccessIs((x), FIELD, PUBLIC)
#define IsProtected(x)	CVMmemberPPPAccessIs((x), FIELD, PROTECTED)
#define IsPrivate(x)	CVMmemberPPPAccessIs((x), FIELD, PRIVATE)

    if (currentClass == NULL ||
        currentClass == memberClass ||
	IsPublic(access)) {
	return CVM_TRUE;
    }

    /* 
     * Access is ok when the following all met:
     *  a. The member is protected.
     *  b. The accessing class is a subclass of the memberClass.
     *  c. The accessing class is either a subclass of the resolvedClass,
     *    a superclass of the resolvedClass or the resolvedClass itself.
     */
    if (IsProtected(access) && !protectedRestriction &&
        IsAncestor(currentClass, memberClass)) {
        if (currentClass == resolvedClass ||
            IsAncestor(currentClass, resolvedClass) ||
            IsAncestor(resolvedClass, currentClass)) {
            return CVM_TRUE;
        }
    }

    /* 
     * Access is ok if both classes are in the same package and
     * the member is not private.
     */
    if (!IsPrivate(access)) {
	if (CVMisSameClassPackage(ee, currentClass, memberClass)) {
	    return CVM_TRUE;
	}
    }

#ifdef CVM_CLASSLOADING
    /* 
     * Bug #4186924. One last chance to pass verification. Needed for
     * buggy 1.1.x compiler. All of the following must be met:
     *	a. The access is performed by byte code, not through reflection.
     *	b. The class performing the access is loaded by a trusted class
     *	   loader (null loader, extension loader, and system loader).
     *	c. The class performing the access and the class being accessed
     *	   are loaded in the same class loader.
     *	d. The class performing the access and the class being accessed
     *	   have the same protection domain.
     */
    if (resolverAccess) {	 	/* (a) */
	CVMBool needsVerify;   		/* (b) */
	CVMBool isSameClassLoader;      /* (c) */
	CVMBool isSameProtectionDomain; /* (d) */

	needsVerify = 
	    CVMloaderNeedsVerify(ee, CVMcbClassLoader(currentClass),
                                 CVM_FALSE);
	isSameClassLoader =
	    (CVMcbClassLoader(currentClass) == CVMcbClassLoader(memberClass));
	CVMID_icellSameObjectNullCheck(ee,
				       CVMcbProtectionDomain(currentClass), 
				       CVMcbProtectionDomain(memberClass), 
				       isSameProtectionDomain);

	/* Make sure (b), (c), and (d) hold true. */
	if (!needsVerify
	    && isSameProtectionDomain
	    && isSameClassLoader)
	{
	    return CVM_TRUE;
	}
    }
#endif

    return CVM_FALSE;
}

/* Documented in interpreter.h */
CVMBool
CVMverifyMemberAccess(CVMExecEnv* ee,
		      CVMClassBlock* currentClass,
		      CVMClassBlock* memberClass, 
		      int access, CVMBool classloaderOnly)
{
    return CVMverifyMemberAccess2(ee, currentClass, memberClass, access,
				  classloaderOnly, CVM_FALSE);
}

/*
 * Global locking functionality.
 */

void
CVMlocksForGCAcquire(CVMExecEnv* ee)
{
    /* NOTE that we do NOT seize the heap lock. The GC thread has
       already seized this. The same is true of the jitLock. */
    CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.heapLock));
#ifdef CVM_JIT
    CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.jitLock));
#endif

    CVMsysMutexLock(ee, &CVMglobals.threadLock);
    CVMsysMutexLock(ee, &CVMglobals.classTableLock);
#ifdef CVM_JVMTI
    CVMsysMutexLock(ee, &CVMglobals.jvmtiLock),
#endif
    CVMsysMutexLock(ee, &CVMglobals.globalRootsLock);
    CVMsysMutexLock(ee, &CVMglobals.weakGlobalRootsLock);
    CVMsysMutexLock(ee, &CVMglobals.typeidLock);
    CVMsysMutexLock(ee, &CVMglobals.syncLock);
    CVMsysMutexLock(ee, &CVMglobals.internLock);
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)
    CVMsysMutexLock(ee, &CVMglobals.gcLockerLock);
#endif
}

void
CVMlocksForGCRelease(CVMExecEnv* ee)
{
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)
    CVMsysMutexUnlock(ee, &CVMglobals.gcLockerLock);
#endif
    CVMsysMutexUnlock(ee, &CVMglobals.internLock);
    CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock);
    CVMsysMutexUnlock(ee, &CVMglobals.weakGlobalRootsLock);
    CVMsysMutexUnlock(ee, &CVMglobals.globalRootsLock);
#ifdef CVM_JVMTI
    CVMsysMutexUnlock(ee, &CVMglobals.jvmtiLock),
#endif
    CVMsysMutexUnlock(ee, &CVMglobals.classTableLock);
    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)

/* The suspension checker mechanism is used by the thread suspension code to
   verify that the suspended thread isn't holding a native lock like the
   malloc lock.

   The way it works is as follows:

   The suspension checker works as a finite state machine.  These are its
   states:
   
      CVM_SUSPEND_CHECKER_START,
      CVM_SUSPEND_CHECKER_IDLE,
      CVM_SUSPEND_CHECKER_FAILED,
      CVM_SUSPEND_CHECKER_CHECK,
      CVM_SUSPEND_CHECKER_EXIT,
      CVM_SUSPEND_CHECKER_EXITED

   At VM initialization time, the checker is in state START.  The main
   thread starts the checker thread in this states.  The checker thread
   initializes itself and then enters state IDLE.  This means that it
   is now ready to service check requests.

   If the checker thread fails to initialize, it will transit to the
   FAILED state instead and exit.

   At runtime, during suspension attempts, a VM thread attempting
   suspension will first suspend a target thread and then calls
   CVMsuspendCheckerIsOK() to see if any native locks are held while
   the target thread is suspended.

   CVMsuspendCheckerIsOK() works by transitioning the checker thread
   to state CHECK, and notifying it.  CVMsuspendCheckerIsOK() then
   waits for either an acknowledgement from the checker thread that
   the check was successful and that it has returned to the IDLE
   state, or that it has failed and timed out.  If a timeout occurs,
   CVMsuspendCheckerIsOK() returns false, else it returns true.  A
   timeout will only occur if the target thread is was suspended
   while holding a native lock (like the malloc or stdio locks) that
   the checker is trying to access.  If other native locks need to
   be added to the checker in the future, they can be added to the
   CHECK state code of the checker thread.

   If CVMsuspendCheckerIsOK() returns false, its caller is expected to
   resume the target thread and sleep for some random amount of time.
   This is to hopefully allow the target thread to execute beyond the
   point where the native locks get released.  Subsequently, the
   caller may attempt to suspend the target thread and call the
   checker again.  Once the target thread releases the native locks,
   the checker thread should be able to acquire those locks and
   release them again immediately.  After that, the checker thread
   will return to state IDLE.

   If a subsequent call to CVMsuspendCheckerIsOK() finds that the
   checker thread is still in the CHECK state, this means that the
   previous target thread still holds the native locks (effectively).
   CVMsuspendCheckerIsOK() will treat this as a failure and return
   false.

   At VM shutdown, we need to clean up the resources used by the
   checker thread as well.  However, we need to ensure that the
   checker thread is no longer running before we destroy the resources.
   The VM thread does this by transiting the checker thread to state
   EXIT.  This will cause the checker thread to clean-up and exit.
   Before it exits, it transits to state EXITED and notifies the
   VM thread with an acknowledgement.  This then allows the VM
   thread to release the resources cleanly knowing that the checker
   thread won't be accessing them anymore.
*/

#define CVM_SUSPEND_CHECKER_STACK_SIZE     20*1024
#define CVM_SUSPEND_CHECKER_STACK_PRIORITY 5 /* Normal priority */
#define CVM_SUSPEND_CHECKER_TIMEOUT        1000 /* 1 second */

enum {
    CVM_SUSPEND_CHECKER_START = 0,
    CVM_SUSPEND_CHECKER_IDLE,
    CVM_SUSPEND_CHECKER_FAILED,
    CVM_SUSPEND_CHECKER_CHECK,
    CVM_SUSPEND_CHECKER_EXIT,
    CVM_SUSPEND_CHECKER_EXITED
};

static void suspendCheckerThreadFunc(void *arg)
{
    CVMBool done = CVM_FALSE;

    CVMGlobalState *gs = &CVMglobals;
    volatile CVMUint32 *state = &gs->suspendCheckerState;
    CVMMutex *lock = &gs->suspendCheckerLock;
    CVMCondVar *checkerCV = &gs->suspendCheckerCV;
    CVMCondVar *checkerAckCV = &gs->suspendCheckerAckCV;

    /* Wait for parent if necessary */
    CVMmutexLock(lock);

    /* Attach thread */
    if (!CVMthreadAttach(&gs->suspendCheckerThreadInfo, CVM_FALSE)) {
        /* Goto the FAILED state: */
        *state = CVM_SUSPEND_CHECKER_FAILED;
    }

    /* Start serving the suspension checker: */
    while (!done) {
        switch (*state) {
        case CVM_SUSPEND_CHECKER_START:
            /* Tell client that we're ready to take requests: */
            *state = CVM_SUSPEND_CHECKER_IDLE;
            CVMcondvarNotify(checkerAckCV);
            /* Fall through to state IDLE: */

        case CVM_SUSPEND_CHECKER_IDLE:
            CVMcondvarWait(checkerCV, lock, CVMlongConstZero());
            break;

	case CVM_SUSPEND_CHECKER_FAILED:
            CVMcondvarNotify(checkerAckCV);
	    done = CVM_TRUE;
	    break;

        case CVM_SUSPEND_CHECKER_CHECK:
            /* Release the lock first because we may block: */
            CVMmutexUnlock(lock);
            /* Check to see if we access stdout and stderr while a client
               thread has been suspended: */
            fprintf(stdout, "%s", ""); /* print nothing. */
            fflush(stdout);
            fprintf(stderr, "%s", ""); /* print nothing. */
            fflush(stderr);
            /* Check to see if we can malloc() while a client thread has been
               suspended: */
            {
                void *buffer = malloc(4);
                free(buffer);
	    }
            CVMmutexLock(lock);
            /* If we get here, we're ready to go again:
               NOTE: Though we're supposed to always get here one way or
                     another, our timeliness of getting to this point in
                     the code (where we return to the IDLE state) will be
                     used by CVMsuspendCheckerIsOK() to determine if the
                     suspension was successful or not.
            */
            *state = CVM_SUSPEND_CHECKER_IDLE;
            CVMcondvarNotify(checkerAckCV);
            break;

        case CVM_SUSPEND_CHECKER_EXIT:
            done = CVM_TRUE;
            /* Notify client that we've effectively exited: */
            *state = CVM_SUSPEND_CHECKER_EXITED;
            CVMcondvarNotify(checkerAckCV);
            break;
        }
    }
    CVMmutexUnlock(lock);
    CVMthreadDetach(&gs->suspendCheckerThreadInfo);
}

/* Purpose: Initializes the suspension checker mechanism. */
CVMBool CVMsuspendCheckerInit()
{
    CVMGlobalState *gs = &CVMglobals;
    volatile CVMUint32 *state = &gs->suspendCheckerState;
    CVMMutex *lock = &gs->suspendCheckerLock;
    CVMCondVar *checkerCV = &gs->suspendCheckerCV;
    CVMCondVar *checkerAckCV = &gs->suspendCheckerAckCV;

    CVMBool success;

    *state = CVM_SUSPEND_CHECKER_START;
    if (!CVMmutexInit(lock)) {
        goto failed0;
    }
    if (!CVMcondvarInit(checkerCV, lock)) {
        goto failed1;
    }
    if (!CVMcondvarInit(checkerAckCV, lock)) {
        goto failed2;
    }

    CVMmutexLock(lock);
    success = CVMthreadCreate(&gs->suspendCheckerThreadInfo,
                              CVM_SUSPEND_CHECKER_STACK_SIZE,
			      CVM_SUSPEND_CHECKER_STACK_PRIORITY,
                              suspendCheckerThreadFunc, NULL);
    if (!success) {
        CVMmutexUnlock(lock);
        goto failed3;
    }

    /* Wait for the checker thread to start before continuing: */
    while (*state == CVM_SUSPEND_CHECKER_START) {
        CVMcondvarWait(checkerAckCV, lock, CVMlongConstZero());
    }
    CVMmutexUnlock(lock);

    /* If the checker thread fails to initialize then fail: */
    if (*state == CVM_SUSPEND_CHECKER_FAILED) {
        goto failed3;
    }

    CVMassert(*state == CVM_SUSPEND_CHECKER_IDLE);

    return CVM_TRUE;

failed3:
    CVMcondvarDestroy(checkerAckCV);
failed2:
    CVMcondvarDestroy(checkerCV);
failed1:
    CVMmutexDestroy(lock);
failed0:
    return CVM_FALSE;
}

/* Purpose: Destroys the suspension checker and cleans up its resources. */
void CVMsuspendCheckerDestroy()
{
    CVMGlobalState *gs = &CVMglobals;
    volatile CVMUint32 *state = &gs->suspendCheckerState;
    CVMMutex *lock = &gs->suspendCheckerLock;
    CVMCondVar *checkerCV = &gs->suspendCheckerCV;
    CVMCondVar *checkerAckCV = &gs->suspendCheckerAckCV;

    /* Wait for the checker thread to exit before releasing the resources: */
    CVMmutexLock(lock);
    CVMassert(*state == CVM_SUSPEND_CHECKER_IDLE);
    *state = CVM_SUSPEND_CHECKER_EXIT; /* Request Exit. */
    CVMcondvarNotify(checkerCV);
    if (*state != CVM_SUSPEND_CHECKER_EXITED) {
        CVMcondvarWait(checkerAckCV, lock, CVMlongConstZero());
    }
    CVMmutexUnlock(lock);

    /* Release the resources: */
    CVMcondvarDestroy(checkerAckCV);
    CVMcondvarDestroy(checkerCV);
    CVMmutexDestroy(lock);
}

/* Purpose: Asks the suspension checker if the target thread was suspended
            cleanly or not. */
CVMBool CVMsuspendCheckerIsOK(CVMExecEnv *ee, CVMExecEnv *targetEE)
{
    CVMGlobalState *gs = &CVMglobals;
    volatile CVMUint32 *state = &gs->suspendCheckerState;
    CVMMutex *lock = &gs->suspendCheckerLock;
    CVMCondVar *checkerCV = &gs->suspendCheckerCV;
    CVMCondVar *checkerAckCV = &gs->suspendCheckerAckCV;

    CVMBool timedOut = CVM_FALSE;
    CVMBool success;

    /* NOTE: We're not allowed to do anything in this function that can
       deadlock with the suspended thread.  This includes the malloc and
       printf families of APIs. */

    /* Ask the checker to check if the suspension was successful: */
    CVMmutexLock(lock);
    if (*state == CVM_SUSPEND_CHECKER_CHECK) {
        /* This means that we've managed to get around to a CHECK state
           before the checker thread had a chance to run again.  This
           means that we had previously detected a contention between the
           client thread and the checker thread, and the cuurent call to
           CVMsuspendCheckerIsOK() is for a retry.

        .  Since, the checker thread hasn't had a chance to run yet, it
           will still be stuck on native locks that are the suspension
           target thread may not have released yet.  So, to be safe, we
           should just declare this check to be a failure and have the
           client retry the suspension again.
	*/
        CVMmutexUnlock(lock);
        return CVM_FALSE; 
    }

    CVMassert(*state == CVM_SUSPEND_CHECKER_IDLE);
    *state = CVM_SUSPEND_CHECKER_CHECK; /* Request Check. */
    CVMcondvarNotify(checkerCV);

    /* Wait as long as we're not timed out and as long as we haven't
       successfully completed the CHECK state yet: */
    while (!timedOut && (*state != CVM_SUSPEND_CHECKER_IDLE)) {
        timedOut = CVMcondvarWait(checkerAckCV, lock,
				  CVMint2Long(CVM_SUSPEND_CHECKER_TIMEOUT));
    }

    /* If we have returned to the IDLE state, then the check was successful.
       Otherwise, the checker would still be stuck in the CHECK state, and
       our client will have to resume the suspended thread in order for the
       checker to get un-stuck: */
    success = (*state == CVM_SUSPEND_CHECKER_IDLE);
    CVMmutexUnlock(lock);

    return success;
}

void
CVMlocksForThreadSuspendAcquire(CVMExecEnv* ee)
{
    CVMglobalSysMutexesAcquire(ee);
}

void
CVMlocksForThreadSuspendRelease(CVMExecEnv* ee)
{
    CVMglobalSysMutexesRelease(ee);
}

void
CVMthreadSuspendConsistentRequest(CVMExecEnv* ee)
{
    /*
       %comment d007
     */
    /* Roll all threads forward to consistent points. */
    int i;

    for (i = 0; i < CVM_NUM_CONSISTENT_STATES; ++i) {
	CVMsysMutexLock(ee, &CVM_CSTATE(i)->mutex);
	CVMcsReachConsistentState(ee, i);
    }
}

void
CVMthreadSuspendConsistentRelease(CVMExecEnv* ee)
{
    /* Allow threads to become inconsistent again. */
    /* %comment d007 */
    {
	int i;

	for (i = CVM_NUM_CONSISTENT_STATES - 1; i >= 0; --i) {
	    CVMcsResumeConsistentState(ee, i);
	    CVMsysMutexUnlock(ee, &CVM_CSTATE(i)->mutex);
	}
    }
}

#endif /* defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION) */

/*
 * CVMlookupNativeMethodCode - look up the native code for a dynamically
 * loaded JNI method.
 */

#ifdef CVM_CLASSLOADING

static void*
CVMfindBuiltinEntry(const char* name)
{
    CVMassert(CVMglobals.cvmDynHandle != NULL);
    return CVMdynlinkSym(CVMglobals.cvmDynHandle, name);
}

static CVMUint8*
CVMclassLoaderFindNative(CVMExecEnv* ee, CVMClassLoaderICell* loader, 
			 char* nativeMethodName)
{
    CVMUint8* nativeCode = NULL;
    CVMBool findBuiltin = CVM_FALSE;
    CVMassert(!CVMlocalExceptionOccurred(ee));

    if (loader == NULL) {
        findBuiltin = CVM_TRUE;
    }
#ifdef CVM_DUAL_STACK
    else {
        CVMClassBlock* loaderCB;
        CVMClassTypeID loaderID;

	CVMD_gcUnsafeExec(ee, {
	    loaderCB = 
	        CVMobjectGetClass(CVMID_icellDirect(ee, loader));
	});
        loaderID = CVMcbClassName(loaderCB);
        if (loaderID == CVMglobals.midpImplClassLoaderTid) {
            findBuiltin = CVM_TRUE;
        }
    }
#endif

    if (findBuiltin) {
	/* This is a NULL class loader so the entry must be built in */
	nativeCode = (CVMUint8*)CVMfindBuiltinEntry(nativeMethodName);
	if (nativeCode != NULL) {
	    return nativeCode;
	}
    }
    {
	/* Ask the ClassLoader to load the native method. */
	JNIEnv*   env = CVMexecEnv2JniEnv(ee);
	CVMUint8* nativeCode = NULL;
	jstring   jname;

	if ((*env)->PushLocalFrame(env, 10) < 0) {
	    return NULL; /* exception already thrown */
	}
	CVMassert(!CVMlocalExceptionOccurred(ee));

        jname = (*env)->NewStringUTF(env, nativeMethodName);
	if (jname == NULL) {
	    nativeCode = NULL; /* exception already thrown */
	} else {
	    /* Call java/lang/ClassLoader.findNative() */
	    jlong lresult = (*env)->CallStaticLongMethod(
                env,  
		CVMcbJavaInstance(CVMsystemClass(java_lang_ClassLoader)), 
		CVMglobals.java_lang_ClassLoader_findNative,
		loader, jname);
	    if (CVMlocalExceptionOccurred(ee)) {
		nativeCode = NULL; /* exception already thrown */
	    } else {
		nativeCode = (CVMUint8*)CVMlong2VoidPtr(lresult);
	    }
	}
    
	(*env)->PopLocalFrame(env, 0);
#ifdef CVM_JVMTI
	/* Need to look in loaded agentslibs */
	if (nativeCode == NULL) {
	    int i;
	    CVMAgentItem *itemPtr;
	    void *libHandle;
	    CVMAgentTable *table = &CVMglobals.agentTable;
	    if (table == NULL || table->elemCnt == 0) {
		return nativeCode;
	    }
	    itemPtr = table->table;
	    for (i = 0; i < table->elemCnt; i++) {
		if ((libHandle = itemPtr[i].libHandle) != NULL) {
		    nativeCode =
			(CVMUint8 *)CVMdynlinkSym(libHandle, nativeMethodName);
		    if (nativeCode != NULL) {
			return nativeCode;
		    }
		}
	    }
	}
#endif
	return nativeCode;
    }
}

CVMBool
CVMlookupNativeMethodCode(CVMExecEnv* ee, CVMMethodBlock* mb)
{
    char*     nativeMethodName;
    CVMUint8* nativeCode;
    CVMClassBlock* cb = CVMmbClassBlock(mb);
    int      mangleType;

    /* Try to lookup the JNI function using the short mangled name first. */
    mangleType = CVM_MangleMethodName_JNI_SHORT;

 tryLookup:
    nativeMethodName = 
	CVMmangleMethodName(ee, mb, (CVMMangleType)mangleType);
    if (nativeMethodName == NULL) {
	return CVM_FALSE; /* exception already thrown */
    }
    nativeCode = CVMclassLoaderFindNative(ee, CVMcbClassLoader(cb),
					  nativeMethodName);
    free(nativeMethodName);
    if (nativeCode != NULL) {
	goto success;
    }
    if (CVMlocalExceptionOccurred(ee)) {
	return CVM_FALSE;
    }
   
    /* Try long mangled name lookup if we haven't yet. */
    if (mangleType == CVM_MangleMethodName_JNI_SHORT) {
	mangleType = CVM_MangleMethodName_JNI_LONG;
	goto tryLookup;
    }
    if (mangleType == CVM_MangleMethodName_JNI_LONG) {
	mangleType = CVM_MangleMethodName_CNI_SHORT;
	goto tryLookup;
    }

    CVMthrowUnsatisfiedLinkError(ee, "%C.%M", cb, mb);
    return CVM_FALSE;

 success:
    /*
     * Set the method's native code pointer and new invoker index. This
     * may already have been done by another thread racing with us,
     * but doing it a 2nd time is harmless so no locking is required.
     */
#ifdef CVM_JVMTI
    if (CVMjvmtiShouldPostNativeMethodBind()) {
	CVMUint8* new_nativeCode = NULL;
	CVMjvmtiPostNativeMethodBind(ee, mb, nativeCode, &new_nativeCode);
	if (new_nativeCode != NULL) {
	    nativeCode = new_nativeCode;
	}
    }
#endif
    CVMmbNativeCode(mb) = nativeCode;
    if (mangleType == CVM_MangleMethodName_CNI_SHORT) {
	CVMmbSetInvokerIdx(mb, CVM_INVOKE_CNI_METHOD);
#if CVM_JIT
	CVMmbJitInvoker(mb) = (void*)CVMCCMinvokeCNIMethod;
#endif
    } else {
	if (CVMmbIs(mb, SYNCHRONIZED)) {
	    CVMmbSetInvokerIdx(mb, CVM_INVOKE_JNI_SYNC_METHOD);
	} else {
	    CVMmbSetInvokerIdx(mb, CVM_INVOKE_JNI_METHOD);
	}
#if CVM_JIT
	CVMmbJitInvoker(mb) = (void*)CVMCCMinvokeJNIMethod;
#endif
    }
    return CVM_TRUE;
}
#endif /* CVM_CLASSLOADING */

/*
 * CVMisTrustedClassLoader - returns TRUE if the classloader is trusted.
 * This basically means that it is either the NULL classloader, the
 * system class loader (aka application class loader), or a parent
 * of the system class loader.
 */

#ifdef CVM_CLASSLOADING

CVMBool
CVMisTrustedClassLoader(CVMExecEnv* ee, CVMClassLoaderICell* loader) {
#ifdef CVM_TRUSTED_CLASSLOADERS	
    return CVM_TRUE;
#else
    /* 
     * The NULL class loader is always trusted.
     */
    if (loader == NULL) {
	return CVM_TRUE;
    }

    /* 
     * If the systemClassLoader is not set and "loader" is not the NULL
     * class loader, then it can't be trusted.
     */
    if (CVMsystemClassLoader(ee) == NULL) {
	return CVM_FALSE;
    }

    /*
     * See if "loader" is the systemClassLoader or one of its parents.
     */
    {
	CVMBool result = CVM_FALSE;
	CVMD_gcUnsafeExec(ee, {
	    CVMObject* loaderObj = CVMID_icellDirect(ee, loader);
	    CVMObject* systemClassLoaderObj =
		CVMID_icellDirect(ee, CVMsystemClassLoader(ee));

	    while (systemClassLoaderObj != NULL) {
		if (systemClassLoaderObj == loaderObj) {
		    result = CVM_TRUE;
		    break;
		}
		CVMD_fieldReadRef(systemClassLoaderObj, 
				  CVMoffsetOfjava_lang_ClassLoader_parent,
				  systemClassLoaderObj);
	    }
	});
	
	return result;
    }
#endif /* CVM_TRUSTED_CLASSLOADERS */
}

#endif /* CVM_CLASSLOADING */

/*
 * Shutdown support
 */

/* Some of this borrowed from JDK */

static int
CVMshutdown()
{
    return 0;
}

int
CVMprepareToExit(void)
{
    struct exit_proc *pExit;

    pExit = CVMglobals.exit_procs;
    while (pExit != NULL) {
        struct exit_proc *pNext = pExit->next;
        (*pExit->proc)();
        free(pExit);
        pExit = pNext;
    }
    /* reset the globally static defined exit_procs */
    CVMglobals.exit_procs = NULL;
    return CVMshutdown();
}

void
CVMexit(int status)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMpostThreadExitEvents(ee);
#ifdef CVM_JVMTI
    if (CVMjvmtiIsEnabled()) {
	CVMjvmtiPostVmExitEvent(ee);
	CVMjvmtiDebugEventsEnabled(ee) = CVM_FALSE;
    }
#endif

    CVMprepareToExit();

    if (CVMglobals.exitHook != NULL) {
	void (*fn)(int) = (void (*)(int))CVMglobals.exitHook;
	(*fn)(status);
	/* exit() shouldn't return, but if it does, fall through */
    }
    CVMhalt(status);
}

CVMBool
CVMsafeExit(CVMExecEnv *ee, CVMInt32 status)
{
    /*
     * If "safeExit" has been enabled, then treat a "safe" System.exit()
     * by the main thread the same as returning from the main method.
     */
    {
	/*
	 * Safe exit is only supported for the main thread.
	 */
	if (ee == &CVMglobals.mainEE) {
	    /*
	     * %comment d007
	     */

	    if (CVMglobals.safeExitHook != NULL) {
		void (*fn)(int) = (void (*)(int))CVMglobals.safeExitHook;
		JavaVM *jvm = &CVMglobals.javaVM.vector;
	
		(*jvm)->DestroyJavaVM(jvm);

		(*fn)(status);
		/* safeExit() shouldn't return */
		CVMassert(CVM_FALSE);
		return CVM_TRUE;
	    }
	}
    }
    return CVM_FALSE;
}

int
CVMatExit(void (*func)(void))
{
    struct exit_proc *pExit =
	(struct exit_proc *)malloc(sizeof(struct exit_proc));
    if (pExit == NULL) {
        return 1;
    }
    pExit->proc = func;
    pExit->next = CVMglobals.exit_procs;
    CVMglobals.exit_procs = pExit;
    return 0;
}

void
CVMabort(void)
{
    if (CVMglobals.abort_entered)
        return;
    CVMglobals.abort_entered = CVM_TRUE;

    if (CVMglobals.abortHook != NULL) {
	void (*fn)() = (void (*)())CVMglobals.abortHook;
        (*fn)();
    } else {
        abort();
    }
}

void
CVMaddThread(CVMExecEnv *ee, CVMBool userThread)
{
    CVMsysMutexLock(ee, &CVMglobals.threadLock);
    if (userThread) {
	++CVMglobals.userThreadCount;
    }
    ++CVMglobals.threadCount;
    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

void
CVMremoveThread(CVMExecEnv *ee, CVMBool userThread)
{
    CVMsysMutexLock(ee, &CVMglobals.threadLock);
    CVMassert(CVMglobals.threadCount > 0);
    if (userThread) {
	CVMassert(CVMglobals.userThreadCount > 0);
	--CVMglobals.userThreadCount;
    }
    --CVMglobals.threadCount;
    CVMcondvarNotify(&CVMglobals.threadCountCV);
    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

/*
 * Wait for some threads to exit.
 */
static void
CVMwaitForThreads(CVMExecEnv *ee, CVMBool userThreads, CVMUint32 maxThreads)
{
    CVMUint32 *threadCountPtr;

    if (userThreads) {
	threadCountPtr = &CVMglobals.userThreadCount;
    } else {
	threadCountPtr = &CVMglobals.threadCount;
    }

    CVMsysMutexLock(ee, &CVMglobals.threadLock);
    while (*threadCountPtr > maxThreads) {
	CVMBool interrupted;
#ifdef CVM_DEBUG
	if (userThreads) {
	    CVMconsolePrintf("Waiting for %d user thread(s) to exit...\n",
		CVMglobals.userThreadCount - maxThreads);
	} else {
	    CVMconsolePrintf("Waiting for %d thread(s) to exit...\n",
		CVMglobals.threadCount - maxThreads);
	}
#endif
	interrupted = CVMsysMutexWait(ee, &CVMglobals.threadLock,
	    &CVMglobals.threadCountCV, CVMlongConstZero());
#ifdef CVM_DEBUG
	if (interrupted) {
	    CVMconsolePrintf("interrupted!\n");
	}
#else
	(void) interrupted; /* suppress compiler warning */
#endif
    }
    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

/*
 * Wait for all of the user threads (except this one, the current thread)
 * to exit.
 */
void
CVMwaitForUserThreads(CVMExecEnv *ee)
{
    CVMwaitForThreads(ee, CVM_TRUE, 1);
}

/*
 * Wait for all threads (except this one) to exit.  This would normally
 * be daemon threads only, but it is probably possible for a daemon
 * thread to create a new user thread.
 */
void
CVMwaitForAllThreads(CVMExecEnv *ee)
{
    CVMwaitForThreads(ee, CVM_FALSE, 1);
}

#ifdef CVM_DEBUG_ASSERTS
/* Normal case is to force a panic by de-referencing address 0x1.
 * #undef FORCE_SYSTEM_PANIC if you want to just print the assertions
 * and continue.  Useful if debugging the VM.
 */

int __forceSystemPanic__ = 1;

int
CVMassertHook(const char *filename, int lineno, const char *expr) {
    CVMconsolePrintf("Assertion failed at line %d in %s: %s\n",
                      lineno, filename, expr);
#if 0 && defined(CVMJIT_SIMPLE_SYNC_METHODS) && defined(CVM_DEBUG)
    CVMconsolePrintf("%C.%M\n",
                     CVMmbClassBlock(CVMglobals.jit.currentSimpleSyncMB),
                     CVMglobals.jit.currentSimpleSyncMB);
    CVMconsolePrintf("%C.%M\n",
                     CVMmbClassBlock(CVMglobals.jit.currentMB),
                     CVMglobals.jit.currentMB);
#endif

#ifdef CVM_DEBUG_DUMPSTACK
    {
        CVMExecEnv* ee = CVMgetEE();
#ifdef CVM_JIT
        /* 4975844: fixup frames first so CVMdumpStack doesn't also assert!
           However, CVMCCMruntimeLazyFixups asserts that ee->microlock != 0,
           so clear it just in case.
        */
#ifdef CVM_DEBUG
        if (ee->microLock != 0) {
            CVMconsolePrintf("CVMassertHook: ee->microLock != 0\n");
            ee->microLock = 0;
        }
#endif
        CVMCCMruntimeLazyFixups(ee);
#endif
        CVMdumpStack(&ee->interpreterStack,0,0,0);
    }
#endif /* CVM_DEBUG_DUMPSTACK */

    /* %comment l028 */
    /* Force a segmentation fault to cause a crash: */
    if (__forceSystemPanic__ != 0) {
	int *ptr = (int *)0x1;
	CVMconsolePrintf("%d\n", *ptr);
	return 0;
    }
    return 1;
}
#endif /* CVM_DEBUG_ASSERTS */

/*
 * CVMisSpecialSuperCall - decides if the specified mb should be turned
 * into an invokesuper_quick instead of a invokenonvirtual_quick?
 *
 * The four conditions that have to be satisfied:
 *    The ACC_SUPER flag is set in the current class
 *    The method isn't private
 *    The method isn't an <init> method
 *    The method's class is a superclass (and not equal) to the current class.
 */
CVMBool
CVMisSpecialSuperCall(CVMClassBlock* currClass, CVMMethodBlock* mb) {
    if (CVMcbIs(currClass, SUPER)
          && (!CVMmbIs(mb, PRIVATE))
          && (!CVMtypeidIsConstructor(CVMmbNameAndTypeID(mb)))
          && (currClass != CVMsystemClass(java_lang_Object))) {
        CVMClassBlock* methodClass = CVMmbClassBlock(mb);
        CVMClassBlock* cb = CVMcbSuperclass(currClass);
        while (cb != NULL) {
            if (cb == methodClass) {
                return CVM_TRUE;
            }
            cb = CVMcbSuperclass(cb);
        }
    }
    return CVM_FALSE;
}

/* 
 * CVMlookupSpecialSuperMethod - Find matching declared method in a
 * super class of currClass.
 */

CVMMethodBlock*
CVMlookupSpecialSuperMethod(CVMExecEnv* ee,
			    CVMClassBlock* currClass,
			    CVMMethodTypeID methodID) {
    CVMClassBlock* supercb = CVMcbSuperclass(currClass);
    CVMMethodBlock* mb = NULL;
    while (supercb != NULL) {
	mb = CVMclassGetDeclaredMethodBlockFromTID(supercb, methodID);
	if (mb != NULL &&
	    !CVMmbIs(mb, STATIC) &&
	    !CVMmbIs(mb, PRIVATE)) {
	    break;
	}
	mb = NULL;
	supercb = CVMcbSuperclass(supercb);
    }
    CVMassert(mb != NULL);
    CVMassert(CVMmbIs(mb, PUBLIC) ||
	      CVMmbIs(mb, PROTECTED) ||
	      CVMisSameClassPackage(ee, CVMmbClassBlock(mb), currClass));
    return mb;
}

#ifndef CVM_TRUSTED_CLASSLOADERS
/* Purpose: Checks to see if OK to instantiate of the specified class.  Will
            throw an InstantiationError if not OK. */
/* Returns: CVM_TRUE if it will throw an InstantiationError exception, else
            returns CVM_FALSE. */
CVMBool
CVMclassIsOKToInstantiate(CVMExecEnv *ee, CVMClassBlock *cb)
{
    /*
     * Make sure we are not trying to instantiate an interface or an
     * abstract class.
     */
    if (CVMcbIs(cb, INTERFACE) || CVMcbIs(cb, ABSTRACT)) {
        CVMthrowInstantiationError(ee, "%C", cb);
        return CVM_FALSE;
    }
    return CVM_TRUE;
}

/* Purpose: Checks to see if the type of the field has changed from static to
            non-static or vice-versa.  Will throw an
            IncompatibleClassChangeError if a change is detected. */
/* Returns: CVM_TRUE if it will throw an InstantiationError exception, else
            returns CVM_FALSE. */
CVMBool
CVMfieldHasNotChangeStaticState(CVMExecEnv *ee, CVMFieldBlock *fb,
                                CVMBool expectToBeStatic)
{
    CVMBool isStaticField = CVMfbIs(fb, STATIC);

    /*
     * Make sure that both the opcode and the fb agree on whether or
     * not the field is static.
     */
    if (isStaticField != expectToBeStatic) {
        CVMthrowIncompatibleClassChangeError(ee,
            "%C: field %F %s to be static", CVMfbClassBlock(fb), fb,
            expectToBeStatic ? "used" : "did not use");
        return CVM_FALSE;
    }
    return CVM_TRUE;
}

/* Purpose: Checks to see if OK to write to the specified field.  Will throw
            an IllegalAccessError if not OK unless surpressed. */
/* Returns: CVM_TRUE if it will throw an IllegalAccessError exception, else
            returns CVM_FALSE.  If okToThrow is CVM_FALSE, only the check will
            be done.  The throwing of the exception will be surpressed. */
CVMBool
CVMfieldIsOKToWriteTo(CVMExecEnv *ee, CVMFieldBlock *fb, 
		      CVMClassBlock* currentCb, CVMBool okToThrow)
{
    CVMBool isFinalField = CVM_FALSE;
    CVMClassBlock *cb = CVMfbClassBlock(fb);

    /* 
     * We only consider the field to be final if the
     * field's class is not the same as the current class.  
     *
     * This covers static fields as well as instance fields.
     */
    if (CVMfbIs(fb, FINAL) && cb != currentCb) {
        isFinalField = CVM_TRUE;
    } else {
        isFinalField = CVM_FALSE;
    }

    /*
     * Make sure we aren't trying to store into a final field.
     * The VM spec says that you can only store into a final field
     * when initializing it. The JDK seems to interpret this as meaning
     * that a class has write access to its final fields, so we do
     * the same here (see how isFinalField gets setup above).
     */
    if (isFinalField) {
        if (okToThrow) {
            CVMthrowIllegalAccessError(ee, "%C: field %F is final", cb, fb);
        }
        return CVM_FALSE;
    }
    return CVM_TRUE;
}

/* Purpose: Checks to see if the type of the method has changed from static to
            non-static or vice-versa.  Will throw an
            IncompatibleClassChangeError if a change is detected. */
/* Returns: CVM_TRUE if it will throw an IncompatibleClassChangeError
            exception, else returns CVM_FALSE. */
CVMBool
CVMmethodHasNotChangeStaticState(CVMExecEnv *ee, CVMMethodBlock *mb,
                                 CVMBool expectToBeStatic)
{
    /*
     * Make sure that both the opcode and the mb agree on whether or
     * not the method is static.
     */
    if (CVMmbIs(mb, STATIC) != expectToBeStatic) {
        CVMthrowIncompatibleClassChangeError(ee,
            "%C: method %M %s to be static", CVMmbClassBlock(mb), mb,
            expectToBeStatic ? "used" : "did not use");
        return CVM_FALSE;
    } 
    return CVM_TRUE;
}

#endif /* CVM_TRUSTED_CLASSLOADERS */

/*
 * Execute the following code to release all references to
 * to the main application ClassLoader, which should allow
 * all application classes to unload. This is used for debugging
 * purposes only.
 */


void
CVMunloadApplicationclasses(CVMExecEnv* ee)
{
#if defined(CVM_CLASSLOADING)
    /* Free the global root that points to the "system" ClassLoader. */
    if (CVMsystemClassLoader(ee) != NULL) {
	CVMID_freeGlobalRoot(ee, CVMsystemClassLoader(ee));
    }

   {
	/* Set ClassLoader.scl to null */
	CVMFieldTypeID sclTID = CVMtypeidLookupFieldIDFromNameAndSig(
            ee, "scl", "Ljava/lang/ClassLoader;");
	const CVMClassBlock* cb =
	    CVMsystemClass(java_lang_ClassLoader);
	CVMFieldBlock* sclFB = 
	    CVMclassGetStaticFieldBlock(cb, sclTID);
	CVMassert(sclFB != NULL);
	/* 
	 * Use member 'raw' to address all bytes of the field 
	 */
	CVMD_gcUnsafeExec(ee, {
	    CVMfbStaticField(ee, sclFB).raw = 0;
	});
    }
    
    {
	/* Set Launcher.launcher to null */
	CVMFieldTypeID launcherTID =
	    CVMtypeidLookupFieldIDFromNameAndSig(
                ee, "launcher", "Lsun/misc/Launcher;");
	const CVMClassBlock* cb =
	    CVMsystemClass(sun_misc_Launcher);
	CVMFieldBlock* launcherFB = 
	    CVMclassGetStaticFieldBlock(cb, launcherTID);
	CVMassert(launcherFB != NULL);
	/* Use member raw to address all bytes of the field */
	CVMD_gcUnsafeExec(ee, {
	    CVMfbStaticField(ee, launcherFB).raw = 0;
	});
    }
    
    /*
     * Set the Thread.contextClassLoader field of each ee's
     * Thread to null, so Thread instances don't keep any
     * ClassLoaders live.
     * 
     * Also, make sure there are no outstanding exceptions thrown. The
     * exception object contains an array of classes in its
     * backtrace, which will prevent classes from unloading.
     */
    {
	/* 
	 * Therefore initialize nullRef.raw with 0 to address all its fields.
	 */
	CVMJavaVal32 nullRef;
	CVMFieldTypeID cclTID =
	    CVMtypeidLookupFieldIDFromNameAndSig(
	        ee, "contextClassLoader", "Ljava/lang/ClassLoader;");
	const CVMClassBlock* cb =
	    CVMsystemClass(java_lang_Thread);
	CVMFieldBlock* cclFb = 
	    CVMclassGetNonstaticFieldBlock(cb, cclTID);

	nullRef.raw = 0;

	CVMassert(cclFb != NULL);
	
	CVMsysMutexLock(ee, &CVMglobals.threadLock);
	CVM_WALK_ALL_THREADS(ee, currEE, {
	    /* clear Thread.contextClassLoader */
	    CVMID_fieldWrite32(ee, CVMcurrentThreadICell(currEE),
			       CVMfbOffset(cclFb), nullRef);
	    /* clear exceptions */
	    CVMclearLocalException(currEE);
	    CVMclearRemoteException(currEE);
	});
	CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
    }
#endif
    
    /* Run the gc to unload the application classes. */
    CVMgcRunGC(ee);

    /*
     * Now there are new finalizable objects enqueued. Get rid of them
     * synchronously.
     */
    {
	jclass      finalizerClass;
	jclass      referenceClass;
	jmethodID   runFinalizationID;
	jmethodID   handleAllEnqueuedID;
	JNIEnv*     env = CVMexecEnv2JniEnv(ee);

	finalizerClass = CVMcbJavaInstance(CVMsystemClass(java_lang_ref_Finalizer));
	referenceClass = CVMcbJavaInstance(CVMsystemClass(java_lang_ref_Reference));
	runFinalizationID = (*env)->GetStaticMethodID(
            env, finalizerClass, "runAllFinalizers", "()V");
	handleAllEnqueuedID = (*env)->GetStaticMethodID(
            env, referenceClass, "handleAllEnqueued", "()V");
	/* Call Reference.handleAllEnqueued() */
	(*env)->CallStaticVoidMethod(env, referenceClass,
				     handleAllEnqueuedID);
	if ((*env)->ExceptionCheck(env)) {
	    (*env)->ExceptionClear(env);
	}
	/* Call Finalizer.runFinalization() */
	(*env)->CallStaticVoidMethod(env, finalizerClass,
				     runFinalizationID);
	if ((*env)->ExceptionCheck(env)) {
	    (*env)->ExceptionClear(env);
	}

        /*
         * New thread was created to run finalizers.
         * Wait for it to exit.
         */
        CVMwaitForAllThreads(ee);
    }
}

/*
 * Trace a method call or return.
 */

#ifdef CVM_TRACE

const char * const invokerNames[] = {
	"Java",
	"Java (synchronized)",
	"CNI",
	"JNI",
	"JNI (synchronized)",
	"Abstract",
	"Miranda (nonpublic)",
	"Miranda (missing interface)",
#ifdef CVM_CLASSLOADING
	"JNI (lazy)"
#endif
};

static const char *
traceMBType(CVMMethodBlock *mb)
{
#ifdef CVM_JIT
    if (CVMmbIsCompiled(mb)) {
	return "Java (compiled)";
    } else
#endif
    {
	return invokerNames[CVMmbInvokerIdx(mb)];
    }
}

#if defined(CVM_PROFILE_METHOD) || defined(CVM_PROFILE_CALL) 

static void
CVMprofileMethodCallReturn(CVMExecEnv *ee,
			   CVMMethodBlock *mb, CVMMethodBlock *mb0,
			   int call_return);
#endif

void CVMtraceMethodReturn(CVMExecEnv *ee, CVMFrame* frame)
{
#if defined(CVM_PROFILE_METHOD)
    {
	if (!CVMframeIsTransition(frame)) {
	    CVMMethodBlock *mb = frame->mb;
	    CVMMethodBlock *mb0;
	    frame = CVMframePrev(frame);
	    while (frame != NULL && (CVMframeIsTransition(frame) ||
		frame->mb == NULL))
	    {
		frame = CVMframePrev(frame);
	    }
	    mb0 = frame != NULL ? frame->mb : NULL;
	    CVMprofileMethodCallReturn(ee, mb, mb0, 1);
	}
    }
#elif defined(CVM_PROFILE_CALL)
#else
    CVMtraceMethod(ee, ("Rtrn from: %s method %P\n",
			(CVMmbIs(frame->mb, NATIVE) ? "JNI " : "Java"),
			frame));
    ee->traceDepth--;
    frame = CVMframePrev(frame);

    if (frame->mb == NULL) {
	CVMtraceMethod(ee,  ("to: (JNI Local Frame)\n"));
    } else {
	CVMtraceMethod(ee, ("to: %s method %P\n",
			    (CVMframeIsTransition(frame) 
			     ? "Tran"
			     : traceMBType(frame->mb)),
			    frame));
    }
#endif
}

void CVMtraceFramelessMethodReturn(CVMExecEnv *ee, CVMMethodBlock *mb,
    CVMFrame* frame)
{
#if defined(CVM_PROFILE_METHOD)
    CVMprofileMethodCallReturn(ee, mb, frame->mb, 1);
#elif defined(CVM_PROFILE_CALL) 
#else
    CVMtraceMethod(ee, ("Rtrn from: %s method %C.%M\n",
			traceMBType(mb), CVMmbClassBlock(mb), mb));
    ee->traceDepth--;

    if (frame->mb == NULL) {
	CVMtraceMethod(ee, ("to: (JNI Local Frame)\n"));
    } else {
	CVMtraceMethod(ee, ("to: %s method %P\n",
			    (CVMframeIsTransition(frame) 
			     ? "Tran"
			     : traceMBType(frame->mb)), 
			    frame));
    }
#endif
}

void CVMtraceMethodCall(CVMExecEnv *ee, CVMFrame* frame, CVMBool isJump)
{
#if defined(CVM_PROFILE_METHOD) || defined(CVM_PROFILE_CALL) 
    {
	if (!CVMframeIsTransition(frame)) {
	    CVMMethodBlock *mb = frame->mb;
	    CVMMethodBlock *mb0;
	    frame = CVMframePrev(frame);
	    while (frame != NULL && (CVMframeIsTransition(frame) ||
		frame->mb == NULL))
	    {
		frame = CVMframePrev(frame);
	    }
	    mb0 = frame != NULL ? frame->mb : NULL;
	    CVMprofileMethodCallReturn(ee, mb0, mb, 0);
	}
    }
#else
    CVMFrame* savedFrame = frame;

    frame = CVMframePrev(frame);
    if (frame->mb == NULL) {
	CVMtraceMethod(ee, ("Call from: (JNI Local Frame)\n"));
    } else {
	CVMtraceMethod(ee, ("%s from: %s method %P\n",
			    (isJump ? "Jump" : "Call"),
			    (CVMframeIsTransition(frame)
			     ? "Tran"
			     : (CVMmbIs(frame->mb, NATIVE) ? "JNI " : "Java")),
			    frame));
    }

    frame = savedFrame;
    ee->traceDepth++;
    CVMtraceMethod(ee, ("to: %s method %P\n",
			(CVMframeIsTransition(frame)
			 ? "Tran"
			 : (CVMmbIs(frame->mb, NATIVE) ? "JNI " : "Java")), 
			frame));
#endif
}

void CVMtraceFramelessMethodCall(CVMExecEnv *ee,
    CVMFrame* frame, CVMMethodBlock *mb, CVMBool isJump)
{
#if defined(CVM_PROFILE_CALL) || defined(CVM_PROFILE_METHOD)
    CVMprofileMethodCallReturn(ee, frame->mb, mb, 0);
#else
    if (frame->mb == NULL) {
	CVMtraceMethod(ee, ("Call from: (JNI Local Frame)\n"));
    } else {
	CVMtraceMethod(ee, ("%s from: %s method %P\n",
			    (isJump ? "Jump" : "Call"),
			    (CVMframeIsTransition(frame)
			     ? "Tran"
			     : (CVMmbIs(frame->mb, NATIVE) ? "JNI " : "Java")),
			    frame));
    }

    ee->traceDepth++;
    CVMtraceMethod(ee, ("to: %s method %C.%M\n",
			traceMBType(mb), CVMmbClassBlock(mb), mb));
#endif
}
#endif

/*
 * Tracing macros
 */
#undef TRACE
#undef TRACESTATUS

#define TRACE(a) CVMtraceOpcode(a)
#define TRACESTATUS()							    \
    CVMtraceStatus(("stack=0x%x frame=0x%x locals=0x%x tos=0x%x pc=0x%x\n", \
		   stack, frame, locals, topOfStack, pc));

/*
 * Common code when the trylock fails.
 */
CVMBool
CVMsyncReturnHelper(CVMExecEnv *ee, CVMFrame *frame, CVMObjectICell *retICell,
		    CVMBool areturn)
{
    CVMBool success;
    CVMObjectICell *objICell;
    
    /* Need to decache because CVMobjectUnlock may become
     * safe. topOfStack currently points to the previous
     * frame's topOfStack, so we can't use DECACHE_TOS().
     * It turns out that just reseting the frame's
     * topOfStack works for the current frame since it 
     * can't possibly contain anything we need to scan.
     */
    CVMassert(!CVMframeIsTransition(frame));
#ifdef CVM_JIT
    if (CVMframeIsCompiled(frame)) {
	frame->topOfStack = CVMframeOpstack(frame, Compiled);
	objICell = &CVMframeReceiverObj(frame, Compiled);
    } else
#endif
    {
	CVM_RESET_JAVA_TOS(frame->topOfStack, frame);
	objICell = &CVMframeReceiverObj(frame, Java);
    }
    if (areturn) {
	/* We could become gcsafe during the unlock, and gc
	 * will not be scanning the result on the previous
	 * frame's stack. We need to grab the result, store
	 * it in a safe place, and the put it back on the
	 * stack after the unlock completes.
	 */
	CVMassert(CVMID_icellIsNull(CVMsyncICell(ee)));
	CVMID_icellSetDirect(ee, CVMsyncICell(ee),
	    CVMID_icellDirect(ee, retICell));
	success = CVMobjectUnlock(ee, objICell);
	CVMID_icellSetDirect(ee, retICell,
	    CVMID_icellDirect(ee, CVMsyncICell(ee)));
	CVMID_icellSetNull(CVMsyncICell(ee));
    } else {
	success = CVMobjectUnlock(ee, objICell);
    }
    if (!success) {
	CVMpopFrame(&ee->interpreterStack, frame);
	CVMthrowIllegalMonitorStateException(
	    ee, "current thread not owner");
	CVMassert(CVMframePrev(frame) != frame);
	return CVM_FALSE;
    } else {
	return CVM_TRUE;
    }
}

/*
 * Register return events with jvmti and jvmpi. Return the current opcode,
 * which will be the same as the pc[0] passed in unless it is a breakpoint.
 */
#if (defined(CVM_JVMTI) || defined(CVM_JVMPI))

CVMUint32
CVMregisterReturnEvent(CVMExecEnv *ee, CVMUint8 *pc, CVMUint32 return_opcode,
		       jvalue *retValue)
{
#ifdef CVM_JVMPI
    CVMBool jvmpiEventMethodExitIsEnabled = CVMjvmpiEventMethodExitIsEnabled();
#endif

    /** The JVMTI spec is unclear about whether the frame pop
	event is sent just before or just after the frame is
	popped. The JDK implementation sends the event just
	before the frame is popped. (NOTE: putting in
	decaching code here causes an assertion failure.) */
    /** Bugid 4191820 seems to imply that the callback must
	be done before the frame is popped, and that no
	local variable slots should have been overwritten
	by the return value. */
#ifdef CVM_JVMTI
    if (return_opcode == opc_breakpoint) {
	CVMD_gcSafeExec(ee, {
	    return_opcode = CVMjvmtiGetBreakpointOpcode(ee, pc, CVM_FALSE);
	});
    }
#endif
    /* 
     * If we are returning a reference result, then we must save away
     * the result in a place where gc will find it and restore later.
     */
    if (return_opcode == opc_areturn) {
	CVMassert(CVMID_icellIsNull(CVMmiscICell(ee)));
	CVMID_icellSetDirect(ee, CVMmiscICell(ee),
			     CVMID_icellDirect(ee, (jobject)&retValue->l));
    }
    	
#ifdef CVM_JVMTI
#ifdef CVM_JVMPI
    if (CVMjvmtiIsEnabled())
#endif
    {
	CVMD_gcSafeExec(ee, {
	    CVMjvmtiPostFramePopEvent(ee,
				      (return_opcode == opc_areturn),
				      CVM_FALSE, retValue);
	});
    }
#endif
#ifdef CVM_JVMPI
    if (jvmpiEventMethodExitIsEnabled) {
	/* NOTE: JVMPI will become GC safe when calling the
	 * profiling agent: */
	CVMjvmpiPostMethodExitEvent(ee);
    }
#endif

    /* Restore the return result if it is a refrence type. */
    if (return_opcode == opc_areturn) {
	CVMID_icellSetDirect(ee, (jobject)&retValue->l,
                             CVMID_icellDirect(ee, CVMmiscICell(ee)));
	CVMID_icellSetNull(CVMmiscICell(ee));
    }
    return return_opcode;
}

CVMUint32
CVMregisterReturnEventPC(CVMExecEnv *ee, CVMUint8* pc,
		       jvalue *retValue)
{
    CVMUint32 return_opcode = pc[0];
    return CVMregisterReturnEvent(ee, pc, return_opcode, retValue);
}

#endif /* (defined(CVM_JVMTI) || defined(CVM_JVMPI)) */

CVMBool
CVMinvokeJNIHelper(CVMExecEnv *ee, CVMMethodBlock *mb)
{
    /*
     * It's a JNI method.
     */
    union {
	CVMJNIReturnValue jni;
	CVMJavaVal32     i;
	CVMObjectICell * o;
    } returnValue;

    CVMStack *stack = &ee->interpreterStack;
    CVMFrame *frame = stack->currentFrame;
    CVMStackVal32 *topOfStack = frame->topOfStack;
    CVMObjectICell *receiverObjICell;
    
    /* Result from CVMjniInvokeNative. It indicates the type
     * of result the java method returned.
     */
    CVMInt8 returnCode;

    topOfStack -= CVMmbArgsSize(mb);

    /*
     * pushFrame() might have to become GC-safe if it needs to 
     * expand the stack. frame->topOfStack for the current frame
     * includes the arguments, and the stackmap does as well, so
     * that the GC-safe action is really safe.
     */
    CVMpushFrame(ee, stack, frame, topOfStack,
		 (CVMoffsetof(CVMJNIFrame, vals) / 
		  sizeof(CVMStackVal32)) + CVM_JNI_MIN_CAPACITY,
		 CVMmbArgsSize(mb), /* skip arguments */
		 CVM_FRAMETYPE_JNI, mb,
		 CVM_TRUE, /* commit */
    /* action if pushFrame fails */
    {
	return CVM_FALSE;
    },
    /* action to take if stack expansion occurred */
    {
	TRACE(("pushing JNIFrame caused stack expansion\n"));
	/* No need to copy arguments for JNI invocations.
	 * The caller's frame will continue to hold the arguments
	 * for us.
	 */ 
    });

    /* Update the new frame's state */
    CVM_RESET_JNI_TOS(frame->topOfStack, frame);
    CVMjniFrameInit(CVMgetJNIFrame(frame), CVM_TRUE);

    TRACE_METHOD_CALL(frame, CVM_FALSE);
    {
	CVMSlotVal32 *locals = NULL;
	CVMUint8     *pc = NULL;
	TRACESTATUS(); (void)locals; (void)pc;
    }

    if (CVMmbIs(mb, STATIC)) {
	CVMClassBlock* cb = CVMmbClassBlock(mb);
	receiverObjICell = CVMcbJavaInstance(cb);
    } else {
	receiverObjICell = &topOfStack[0].ref;
    }

    /* Our stack state at this point is as follows:
     *    topOfStack        <- points before the arguments
     *    prev->topOfStack  <- points after the arguments
     *    frame->topOfStack <- points to space for jni local refs
     *
     * We want prev->topOfStack to point after the arguments so
     * they will be scanned. topOfStack will continue to point
     * before the arguments so we can store our result there.
     */

    /* If the method is sync, then lock the object. */
    if (CVMmbIs(mb, SYNCHRONIZED)) {
	/* %comment l002 */
	if (!CVMobjectTryLock(
		ee, CVMID_icellDirect(ee, receiverObjICell))) {
	    if (!CVMobjectLock(ee, receiverObjICell)) {
		CVMpopFrame(stack, frame); /* pop the jni frame */
		CVMthrowOutOfMemoryError(ee, NULL);
		return CVM_FALSE;
	    }
	}
    }

#ifdef CVM_JVMPI
    if (CVMjvmpiEventMethodEntryIsEnabled()) {
	/* NOTE: JVMPI will become GC safe when calling the
	 * profiling agent: */
	CVMjvmpiPostMethodEntryEvent(ee, receiverObjICell);
    }
#endif /* CVM_JVMPI */

#ifdef CVM_JVMTI
    /* %comment k001 */
    if (CVMjvmtiIsEnabled()) {
	CVMD_gcSafeExec(ee, {
	    CVMjvmtiPostFramePushEvent(ee);
	});
    }
#endif

    ee->threadState = CVM_THREAD_IN_NATIVE;
    /* Call the JNI method. topOfStack still points just below
     * the arguments in the caller's frame, so you can still
     * use it to access the arguments.
     */
    CVMD_gcSafeExec(ee, {
	CVMClassBlock* cb = CVMmbClassBlock(mb);
	CVMterseSig nativeSig;
	CVMtypeidGetTerseSignature(CVMmbNameAndTypeID(mb),
				   &nativeSig );
	returnCode = CVMjniInvokeNative((void*)CVMexecEnv2JniEnv(ee),
		CVMmbNativeCode(mb),
		&topOfStack->j.raw,
		nativeSig.datap,
		CVMmbArgsSize(mb),
		CVMmbIs(mb, STATIC) ? CVMcbJavaInstance(cb) : 0,
		&returnValue.jni);
    });
    
    CVMassert(ee->criticalCount == 0);
    TRACE_METHOD_RETURN(frame);

    ee->threadState &= ~CVM_THREAD_IN_NATIVE;

#ifdef CVM_JVMTI

    /** The JVMTI spec is unclear about whether the frame
	pop event is sent just before or just after the
	frame is popped. The JDK implementation sends the
	event just before the frame is popped. Also,
	Bugid 4191820 seems to imply that the callback must
	be done before the frame is popped, and that no
	local variable slots should have been overwritten
	by the return value. */
    if (CVMjvmtiIsEnabled()) {
	jvalue retValue;
	retValue.j = CVMlongConstZero();
	switch (returnCode) {
	case 0:
	    CVMD_gcSafeExec(ee, {
		CVMjvmtiPostFramePopEvent(ee, CVM_FALSE, CVM_FALSE,
					  &retValue);
	    });
	    break;
	case 1:
	    retValue.i = returnValue.i.i;
	    CVMD_gcSafeExec(ee, {
		CVMjvmtiPostFramePopEvent(ee, CVM_FALSE, CVM_FALSE,
					  &retValue);
	    });
	    break;
	case 2:
	    CVMmemCopy64(&retValue.j, returnValue.jni.v64);
	    CVMD_gcSafeExec(ee, {
		CVMjvmtiPostFramePopEvent(ee, CVM_FALSE, CVM_FALSE,
					  &retValue);
	    });
	    break;
	case -1:
	    CVMassert(CVMID_icellIsNull(CVMmiscICell(ee)));
	    {
		CVMObjectICell *o = returnValue.o;
		if (o != NULL) {
		    CVMID_icellAssignDirect(ee, CVMmiscICell(ee), o);
		}
		CVMD_gcSafeExec(ee, {
		    CVMjvmtiPostFramePopEvent(ee, CVM_TRUE,
					      CVM_FALSE, &retValue);
		});
		CVMID_icellSetNull(CVMmiscICell(ee));
	    }
	    break;
	}
    }
#endif
#ifdef CVM_JVMPI
    if (CVMjvmpiEventMethodExitIsEnabled()) {
	/* NOTE: JVMPI will become GC safe when calling the
	 *       profiling agent: */
	CVMjvmpiPostMethodExitEvent(ee);
    }
#endif /* CVM_JVMPI */
    /*
     * Unlock the sync object if necessary, before the return
     * value tramples over receiveObjICell.
     */
    if (CVMmbIs(mb, SYNCHRONIZED)) {
	/* %comment l003 */
	CVMBool success;
	success = CVMobjectTryUnlock(
	    ee, CVMID_icellDirect(ee, receiverObjICell));
	if (!success) {
	    /* NOTE: CVMobjectUnlock may become GC safe. */
	    success = CVMobjectUnlock(ee, receiverObjICell);
	    if (!success) {
		/* 
		 * We need to clear any exception that JNI may have
		 * returned and throw a new one.
		 */
		CVMclearLocalException(ee);
		CVMpopFrame(stack, frame); /* pop the jni frame */
		CVMthrowIllegalMonitorStateException(
		    ee, "current thread not owner");
		return CVM_FALSE;
	    }
	}
    }

    CVMpopFrame(stack, frame);   /* pop the jni frame */
       
    /*
       If an exception has occurred, do not worry about storing
       the result or adjusting topOfStack.  Exception handling
       will reset topOfStack anyway.
    */
    /* If an exception occured, then we don't want to bump up
     * the pc, so check first.
     */
    if (CVMexceptionOccurred(ee)) {
	return CVM_FALSE;
    }

    /* Grab the result and store it. topOfStack still points 
     * just below the arguments in the caller's frame, so you 
     * can still use the STACK_ macros with an offset of 0
     * to save the result.
     *
     * CVMjniInvokeNative stored the result in the local
     * variable "returnValue".
     */
    {
	switch (returnCode) {
	case 0:
	    break;
	case -1:
	    {
		CVMObjectICell *o = returnValue.o;
		if (o != 0) {
		    CVMID_icellAssignDirect(ee, &topOfStack[0].ref, o);
		} else {
		    CVMID_icellSetNull(&topOfStack[0].ref);
		}
		topOfStack += 1;
		break;
	    }
	case 1:
	    topOfStack[0].j = returnValue.i;
	    topOfStack += 1;
	    break;
	case 2:
	    CVMmemCopy64(&topOfStack[0].j.raw, returnValue.jni.v64);
	    topOfStack += 2;
	    break;
	default:
	    /* This should never happen. */
	    CVMassert(CVM_FALSE);
	}
    }	    

    /* topOfStack points just after the result at this point,
     * which is exactly what we want.
     */
    frame->topOfStack = topOfStack;

    return CVM_TRUE;
}


#ifdef CVM_TRACE_ENABLED

#if defined(CVM_PROFILE_METHOD) || defined(CVM_PROFILE_CALL)

#define CVM_METHOD_TRACE_SIZE 1000

#include "javavm/include/porting/time.h"

#ifdef CVM_PROFILE_CALL

struct trace_item {
    CVMMethodBlock *mb;
    CVMMethodBlock *mb0;
    int count;
} ccm_trace[CVM_METHOD_TRACE_SIZE], *ccm_trace_p[CVM_METHOD_TRACE_SIZE];

#endif /* CVM_PROFILE_CALL */

#ifdef CVM_PROFILE_METHOD

/* These should go in EE for multithreaded applications */
struct trace_item {
    CVMMethodBlock *mb;
    int count[2];
    CVMInt64 trace_msec;
} ccm_trace[CVM_METHOD_TRACE_SIZE], *ccm_trace_p[CVM_METHOD_TRACE_SIZE];

#endif /* CVM_PROFILE_METHOD */

int ccm_trace_count;
int ccm_trace_total;
int traceEra = 0;

#ifdef CVM_PROFILE_METHOD

static int
find_record(CVMMethodBlock *mb)
{
    int i;
    /* find record */
    for (i = 0; i < ccm_trace_count; ++i) {
	if (ccm_trace_p[i]->mb == mb) {
	    int count;
	    int j = i;
	    count = ccm_trace_p[i]->count[0] + ccm_trace_p[i]->count[1] + 1;
	    while (j > 0 && count > ccm_trace_p[j - 1]->count[0] +
		ccm_trace_p[j - 1]->count[1])
	    {
		--j;
	    }
	    if (i > 0 && j < i) {
		struct trace_item *tmp = ccm_trace_p[j];
		ccm_trace_p[j] = ccm_trace_p[i];
		ccm_trace_p[i] = tmp;
		return j;
	    }
	    return i;
	}
    }
    if (i == ccm_trace_count) {
	if (ccm_trace_count < CVM_METHOD_TRACE_SIZE) {
	    ccm_trace[ccm_trace_count].mb = mb;
	    ccm_trace_p[ccm_trace_count] = &ccm_trace[ccm_trace_count];
	    i = ccm_trace_count;
	    ++ccm_trace_count;
	}
    } else {
	i = -1;
    }
    return i;
}

#endif /* CVM_PROFILE_METHOD */

static void
CVMprofileMethodCallReturn(CVMExecEnv *ee,
    CVMMethodBlock *mb, CVMMethodBlock *mb0, int call_return)
{
#ifdef CVM_PROFILE_CALL
    int i;

    if (call_return == 1) {
	return;
    }
    if (mb0 < mb) {
	CVMMethodBlock *m = mb0;
	mb0 = mb;
	mb = m;
    }

    CVMsysMicroLock(ee, CVM_METHOD_TRACE_MICROLOCK);

    for (i = 0; i < ccm_trace_count; ++i) {
	if (ccm_trace_p[i]->mb == mb && ccm_trace_p[i]->mb0 == mb0) {
	    int count;
	    int j = i;
	    ++ccm_trace_p[i]->count;
	    count = ccm_trace_p[i]->count;
	    while (j > 0 && count > ccm_trace_p[j - 1]->count) {
		--j;
	    }
	    if (i > 0 && j < i) {
		struct trace_item *tmp = ccm_trace_p[j];
		ccm_trace_p[j] = ccm_trace_p[i];
		ccm_trace_p[i] = tmp;
	    }
	    goto done;
	}
    }
    if (ccm_trace_count < CVM_METHOD_TRACE_SIZE) {
	ccm_trace[ccm_trace_count].mb = mb;
	ccm_trace[ccm_trace_count].mb0 = mb0;
	ccm_trace[ccm_trace_count].count = 1;
	ccm_trace_p[ccm_trace_count] = &ccm_trace[ccm_trace_count];
	++ccm_trace_count;
    }
done:
    ++ccm_trace_total;

    CVMsysMicroUnlock(ee, CVM_METHOD_TRACE_MICROLOCK);

#endif /* CVM_PROFILE_CALL */

#ifdef CVM_PROFILE_METHOD

    CVMsysMicroLock(ee, CVM_METHOD_TRACE_MICROLOCK);

    if (ee->traceEra != traceEra) {
	ee->traceEra = traceEra;
	ee->cindx = -1;
    }

    {
	int cindx = ee->cindx;
	if (cindx != -1) {
	    if (ee->cmb == mb) {
		ccm_trace_p[cindx]->trace_msec =
		    CVMlongAdd(ccm_trace_p[cindx]->trace_msec,
			       CVMlongSub(CVMtimeMillis(), ee->t0));
		if (call_return == 0) {
		    ++ccm_trace_p[cindx]->count[1];
		}
	    } else {
		/* Should never happen */
		CVMconsolePrintf("Method discontinuity %s %C.%M : "
		    "%s %C.%M <--> %s %C.%M\n",
			traceMBType(ee->cmb),
			CVMmbClassBlock(ee->cmb), ee->cmb,
			traceMBType(mb),
			CVMmbClassBlock(mb), mb,
			traceMBType(mb0),
			CVMmbClassBlock(mb0), mb0);
	    }
	}
	ee->cindx = find_record(mb0);
	if (ee->cindx != -1) {
	    ee->cmb = mb0;
	    if (call_return == 0) {
		++ccm_trace_p[ee->cindx]->count[0];
	    }
	    ee->t0 = CVMtimeMillis();
	}
    }
    ++ccm_trace_total;
    CVMsysMicroUnlock(ee, CVM_METHOD_TRACE_MICROLOCK);

#endif /* CVM_PROFILE_METHOD */
}

static void
CVMtraceMethodDump()
{
    int i;

    CVMconsolePrintf("CCM trace dump:\n");
#ifdef CVM_PROFILE_CALL
    for (i = 0; i < ccm_trace_count; ++i) {
	CVMMethodBlock *mb;
	CVMMethodBlock *mb0;
	int count = ccm_trace_p[i]->count;
	mb = ccm_trace_p[i]->mb;
	mb0 = ccm_trace_p[i]->mb0;
	CVMconsolePrintf("[%d] CCM %s %C.%M <--> %s %C.%M\n",
	    count,
	    traceMBType(mb),
	    CVMmbClassBlock(mb), mb,
	    traceMBType(mb0),
	    CVMmbClassBlock(mb0), mb0);
    }
#endif /* CVM_PROFILE_CALL */
#ifdef CVM_PROFILE_METHOD
    CVMconsolePrintf("self ms, self ms / in count,"
	" in count, out count, method\n");
    for (i = 0; i < ccm_trace_count; ++i) {
	CVMMethodBlock *mb;
	int *count = ccm_trace_p[i]->count;
	mb = ccm_trace_p[i]->mb;
	if (mb == NULL) {
	    CVMconsolePrintf("%d %g %d %d Native Code\n",
		CVMlong2Int(ccm_trace_p[i]->trace_msec),
		CVMlong2Double(ccm_trace_p[i]->trace_msec) / count[0],
		count[0], count[1]);
	} else {
	    CVMconsolePrintf("%d %g %d %d %s %C.%M\n",
		CVMlong2Int(ccm_trace_p[i]->trace_msec),
		CVMlong2Double(ccm_trace_p[i]->trace_msec) / count[0],
		count[0], count[1],
		traceMBType(mb),
		CVMmbClassBlock(mb), mb);
	}
    }
#endif /* CVM_PROFILE_METHOD */
}

#endif /* defined(CVM_PROFILE_METHOD) || defined(CVM_PROFILE_CALL) */

void
CVMtraceReset(CVMUint32 oldValue, CVMUint32 newValue)
{
#if defined(CVM_PROFILE_METHOD)
    CVMExecEnv *ee = CVMgetEE();
    if ((oldValue ^ newValue) & CVM_DEBUGFLAG(TRACE_METHOD)) {
	CVMsysMicroLock(ee, CVM_METHOD_TRACE_MICROLOCK);
	++traceEra;
	CVMsysMicroUnlock(ee, CVM_METHOD_TRACE_MICROLOCK);
    }
#endif
}

void
CVMtraceInit()
{
#if defined(CVM_PROFILE_METHOD) || defined(CVM_PROFILE_CALL)
    CVMatExit(CVMtraceMethodDump);
#endif
}

#endif /* CVM_TRACE_ENABLED */

void
CVMpostThreadStartEvents(CVMExecEnv *ee)
{
#ifdef CVM_JVMPI
    if (CVMjvmpiEventThreadStartIsEnabled())
        CVMjvmpiPostThreadStartEvent(ee, CVMcurrentThreadICell(ee));
#endif /* CVM_JVMPI */

#ifdef CVM_JVMTI
    /* This will work for getting thread start/stop
       notification for Java threads, but when we support JVMTI's
       "RunDebugThread" or JVMPI's "CreateSystemThread" then it's
       likely this function won't be called from CVMthreadCreate. The
       specs aren't precise about whether these events are generated
       for "native" threads created using these routines. If they are
       expected to be (and the only way to find out is to look through
       the JDK source) then we'll have to be very careful when
       implementing RunDebugThread/CreateSystemThread to ensure the
       events get generated. */

    if (CVMjvmtiIsEnabled()) {
	CVMjvmtiPostThreadStartEvent(ee, CVMcurrentThreadICell(ee));
    }
#endif

#if defined(CVM_DEBUG) && defined(CVM_LVM) /* %begin lvm */
    CVMLVMtraceNewThread(ee);
#endif /* %end lvm */

}

void
CVMpostThreadExitEvents(CVMExecEnv *ee)
{
    if (!ee->hasPostedExitEvents) {
#ifdef CVM_JVMTI
	if (CVMjvmtiIsEnabled()) {
	    CVMjvmtiPostThreadEndEvent(ee, CVMcurrentThreadICell(ee));
	}
#endif

#ifdef CVM_JVMPI
	if (CVMjvmpiEventThreadEndIsEnabled()) {
	    CVMjvmpiPostThreadEndEvent(ee);
	}
#endif /* CVM_JVMPI */

#if defined(CVM_DEBUG) && defined(CVM_LVM) /* %begin lvm */
	CVMLVMtraceTerminateThread(ee);
#endif /* %end lvm */
	ee->hasPostedExitEvents = CVM_TRUE;
    }
}

#ifdef CVM_DEBUG_ASSERTS
CVMInterpreterFrame *
CVMDEBUGgetInterpreterFrame(CVMFrame *frame)
{
    return CVMgetInterpreterFrame0(frame);
}
#endif
