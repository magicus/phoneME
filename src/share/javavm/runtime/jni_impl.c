/*
 * @(#)jni_impl.c	1.380 06/10/31
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

#define _JNI_IMPLEMENTATION_

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/stacks.h"
#include "javavm/include/localroots.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/typeid.h"
#include "javavm/include/jni_impl.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/preloader.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/utils.h"
#include "javavm/include/timestamp.h"
#include "javavm/include/stackwalk.h"
#include "javavm/include/packages.h"
#ifndef CDC_10
#include "javavm/include/javaAssertions.h"
#endif

#include "generated/offsets/java_lang_Class.h"
#include "generated/offsets/java_lang_String.h"
#include "generated/offsets/java_lang_Thread.h"
#include "generated/jni/java_lang_Thread.h"
#include "generated/cni/sun_misc_CVM.h"

#include "javavm/include/porting/time.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/system.h"
#include "javavm/include/porting/globals.h"
#include "javavm/include/porting/ansi/stdarg.h"

#include "javavm/export/jni.h"

#ifdef CVM_XRUN
#include "javavm/include/xrun.h"
#endif
#ifdef CVM_AGENTLIB
#include "javavm/include/agentlib.h"
#endif

#ifdef CVM_CLASSLOADING
#include "javavm/include/porting/java_props.h"
#include "native/java/util/zip/zip_util.h"
#include "javavm/include/porting/io.h"
#include "javavm/include/porting/path.h"
#endif

#ifdef CVM_REFLECT
#include "javavm/include/reflect.h"
#endif

#ifdef CVM_JVMTI
#include "javavm/export/jvmti.h"
#endif

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

#ifdef CVM_JIT
#include "javavm/include/ccm_runtime.h"
#include "javavm/include/porting/jit/ccm.h"
#include "javavm/include/jit_common.h"
#include "javavm/include/jit/jitstats.h"
#include "javavm/include/jit/jitcodebuffer.h"
#endif

#ifdef CVM_DEBUG_ASSERTS
#include "javavm/include/constantpool.h"
#endif

#ifdef CVM_EMBEDDED_HOOK
#include "javavm/include/hook.h"
#endif

#ifdef CVM_HW
#include "include/hw.h"
#endif

#undef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))

/*
 * If gc latency is of concern, you may want to reduce this buffer size.
 * It will affect the maximum amount of time spent gc-unsafe while copying
 * characters from the heap to a local buffer.
 */
#undef  MAX_CHARS_PER_ACCESS
#define MAX_CHARS_PER_ACCESS	100

/* %comment c006 */

#define CVMjniNonNullICellPtrFor(jobj) \
    CVMID_NonNullICellPtrFor(jobj)

#ifdef CVM_JVMTI_IOVEC
static CVMIOVector CVMioFuncs;
#endif

static CVMUint32
CVMjniAvailableCapacity(CVMStack* stack, CVMJNIFrame* frame)
{
    if (!CVMaddressInStackChunk(frame, stack->currentStackChunk)) {
        CVMStackChunk* chunk = stack->currentStackChunk;
        CVMUint32 available = 0;
       
        while (!CVMaddressInStackChunk(frame, chunk)) {
            available += (CVMUint32)(chunk->end_data - chunk->data);
            chunk = chunk->prev;
        }
        CVMassert(CVMaddressInStackChunk(frame, chunk));
        available += (((CVMUint32)chunk->end_data - 
                     (CVMUint32)frame) / sizeof(CVMStackVal32)) -
                    CVMJNIFrameCapacity - frame->inUse;
        return available;
    } else {
	/* 
	 * casting to the result type jint should be done after 
	 * subtracting the addresses frame->topOfStack and frame
	 * because jint can not hold a native pointer on 64 bit platforms
	 */
	return (jint)((((CVMAddr)((stack->currentStackChunk)->end_data) - 
                    (CVMAddr)frame) / sizeof(CVMStackVal32)) - 
                   CVMJNIFrameCapacity - frame->inUse);
    }
}


/* private types */

typedef char
JNI_PushArguments_t(JNIEnv *env, CVMterseSigIterator  *terse_signature,
                    CVMFrame *current_frame, const void *args);

/* Forward declarations */

static JNI_PushArguments_t CVMjniPushArgumentsVararg;
static JNI_PushArguments_t CVMjniPushArgumentsArray;

static void CVMjniInvoke(JNIEnv *env, jobject obj, jmethodID methodID,
    JNI_PushArguments_t pushArguments, const void *args,
    CVMUint16 info, jvalue *retValue);

/* Definitions */


void JNICALL
CVMjniFatalError(JNIEnv *env, const char *msg)
{
    CVMconsolePrintf("JNI FATAL ERROR: %s\n", msg);
#if defined(CVM_DEBUG) && defined(CVM_DEBUG_DUMPSTACK)
    CVMdumpStack(&CVMjniEnv2ExecEnv(env)->interpreterStack,
		 CVM_FALSE, CVM_FALSE, 100);
#endif 
    CVMexit(1);
}

#undef CVMjniGcUnsafeRef2Class
#define CVMjniGcUnsafeRef2Class(ee, cl) CVMgcUnsafeClassRef2ClassBlock(ee, cl)

#undef CVMjniGcSafeRef2Class
#define CVMjniGcSafeRef2Class(ee, cl) CVMgcSafeClassRef2ClassBlock(ee, cl)



/* 
 * CreateLocalRef is used to create a local ref in the current frame of
 * the specified thread.  It should check the free list first.
 */
CVMObjectICell*
CVMjniCreateLocalRef0(CVMExecEnv *ee, CVMExecEnv *targetEE)
{
    CVMStack*      jniLocalsStack = &targetEE->interpreterStack;
    CVMFrame*	   currentFrame   = jniLocalsStack->currentFrame;
    CVMStackVal32* result         = NULL;

    CVMassert(CVMframeIsJNI(currentFrame));

    CVMframeAlloc(ee, jniLocalsStack, CVMgetJNIFrame(currentFrame), result);
    if (result != NULL) {
	return &result->ref;
    } else {
	/* The JNI spec says that if an attempt to allocate a localRef
	 * fails, it's a fatal error since the caller really should have
	 * called EnsureLocalCapacity() first. 
	 */
	CVMjniFatalError(CVMexecEnv2JniEnv(ee), 
			 "Out of memory when expanding local ref "
			 "table beyond capacity");
	return NULL;
    }
}

/* 
 * CreateLocalRef is used to create a local ref in the current frame
 * of the current thread.
 */
CVMObjectICell*
CVMjniCreateLocalRef(CVMExecEnv *ee)
{
    return CVMjniCreateLocalRef0(ee, ee);
}

/* Version information */

jint JNICALL
CVMjniGetVersion(JNIEnv *env)
{
    return JNI_VERSION_1_4;
}

/* 
 * NewLocalRef is used to create a local ref in the specified frame. It
 * should check the free list first.
 */
jobject JNICALL
CVMjniNewLocalRef(JNIEnv *env, jobject obj)
{
    CVMExecEnv*     currentEE      = CVMjniEnv2ExecEnv(env);
    CVMObjectICell* resultCell     = NULL;

    CVMassert(CVMframeIsJNI(CVMeeGetCurrentFrame(currentEE)));

    if ((obj == NULL) || CVMID_icellIsNull(obj)) {
	/*
	 * If ref is NULL, or ref points to a cell containing a NULL
	 * (a cleared weak global ref), return NULL.
	 */
	return NULL;
    } else {
	resultCell = CVMjniCreateLocalRef(currentEE);
	if (resultCell == NULL) {
	    return NULL;
	}
	CVMID_icellAssign(currentEE, resultCell, obj);
        /* If obj is a weak reference GC could have happened before
         * unsafe assign action happened so check for null
         */
        if (CVMID_icellIsNull(resultCell)) {
            CVMjniDeleteLocalRef(env, resultCell);
            resultCell = NULL;
        }
        return resultCell;
    }
}

/*
 * Is a given ref in any of the chunks of 'frame'?
 */
static CVMBool
CVMjniRefInFrame(CVMStackChunk* chunk, CVMFrame* frame, CVMStackVal32* ref)
{
    CVMJNIFrame*   jniFrame = CVMgetJNIFrame(frame);
    CVMStackVal32* tos      = frame->topOfStack;
    /*
     * Traverse chunks from the last chunk of the frame to the first
     */
    while (!CVMaddressInStackChunk(frame, chunk)) {
	if ((ref < tos) && (ref >= chunk->data)) {
	    return CVM_TRUE;
	}
	chunk = chunk->prev;
	tos = chunk->end_data;
    }
    /* Now we are looking at the first chunk of the frame.
       Do a range check. */
    if ((ref < tos) && (ref >= jniFrame->vals)) {
	return CVM_TRUE;
    } else {
	return CVM_FALSE;
    }
}

/* 
 * DeleteLocalRef is used delete a JNI local ref in the specified frame.
 */
void JNICALL
CVMjniDeleteLocalRef(JNIEnv *env, jobject obj)
{
    CVMExecEnv*    currentEE      = CVMjniEnv2ExecEnv(env);
    CVMStack*      jniLocalsStack = &currentEE->interpreterStack;
    CVMStackVal32* refToFree	  = (CVMStackVal32*)obj;
    CVMFrame*      frame;
    CVMFrame*      calleeFrame;
    CVMStackChunk* chunk;
    CVMStackWalkContext swc;

    frame = jniLocalsStack->currentFrame;
    CVMassert(frame != NULL);
    CVMassert(CVMframeIsJNI(frame));
    if (refToFree == NULL) {
	return; /* Nothing to do */
    }
    /* 
     * Walk the stack through all local roots and JNI frames. These
     * may be PushLocalFrame frames, or the frame of a JNI
     * method. If the local frame belongs to any one of these frames
     * (each of which may span multiple stack chunks), return it to
     * the right free list.  
     */
    CVMstackwalkInit(jniLocalsStack, &swc);
    frame = swc.frame;
    chunk = swc.chunk;
    do {
	if (CVMjniRefInFrame(chunk, frame, refToFree)) {
	    CVMframeFree(CVMgetJNIFrame(frame), refToFree);
	    return;
	}
	calleeFrame = frame;
	CVMstackwalkPrev(&swc);
	frame = swc.frame;
	chunk = swc.chunk;
	CVMassert(frame != NULL);
    } while (CVMframeIsJNI(frame));
    /*
     * Well, we couldn't find where this ref belongs. Maybe it is an argument
     * to this JNI function.
     *
     * At this point:
     *    frame-        the caller of the JNI method
     *    calleeFrame-  the frame of the JNI method
     */
    if (CVMframeIsInterpreter(frame)) {
	/* We are now at the caller of the JNI method. Its top of stack is set
	   to just above the arguments to the JNI method. See if refToFree
	   was one of these arguments. */
	CVMStackVal32* topOfArgs = frame->topOfStack;
	if ((refToFree <  topOfArgs) &&
	    (refToFree >= topOfArgs - CVMmbArgsSize(calleeFrame->mb))) {
	    CVMID_icellSetNull(obj);
	    return;
	}
    }
    /*
     * If we got here, this cannot be a valid ref. Just ignore -- we can't
     * do anything useful with this.
     */
    CVMdebugPrintf(("Attempt to delete invalid local ref -- ignoring!\n"));
}

/*
 * Ensure local refs capacity in frame (in number of words).
 */
jint JNICALL
CVMjniEnsureLocalCapacity(JNIEnv *env, jint capacity)
{
    CVMExecEnv*    ee             = CVMjniEnv2ExecEnv(env);
    CVMStack*      jniLocalsStack = &ee->interpreterStack;
    CVMFrame*	   currentFrame   = jniLocalsStack->currentFrame;
    CVMJNIFrame*   jniFrame;
    CVMUint32      available;

    CVMassert(CVMframeIsJNI(currentFrame));
    jniFrame = CVMgetJNIFrame(currentFrame);
    available = CVMjniAvailableCapacity(jniLocalsStack, jniFrame);

    /* The 'capacity <= 0' is necessary if capacity is negative */
    if (capacity <= 0 || capacity <= available) {
	return JNI_OK; /* We have enough to satisfy the capacity request */
    }

    /* Now expand the stack */
    capacity -= available; 
    CVMD_gcUnsafeExec(ee, {
	CVMexpandStack(ee, jniLocalsStack, capacity, CVM_TRUE, CVM_TRUE);
    });
    
    if (CVMlocalExceptionOccurred(ee)) {
	/* The JNI spec says to throw an OutOfMemoryError. However,
	 * CVMexpandStack may have thrown a StackOverflowError
	 * error instead.
	 */
	CVMclearLocalException(ee);
	CVMthrowOutOfMemoryError(ee, NULL);
	return JNI_ENOMEM;
    } else {
	return JNI_OK;
    }
}

/*
 * Push local frame that can accommodate 'capacity' local refs
 */
jint JNICALL
CVMjniPushLocalFrame(JNIEnv *env, jint capacity)
{
    CVMExecEnv*    ee             = CVMjniEnv2ExecEnv(env);
    CVMStack*      jniLocalsStack = &ee->interpreterStack;
    CVMFrame*      currentFrame   = jniLocalsStack->currentFrame;

    /* Make sure we have the room on the stack for the specified bytes. */
    if (!CVMensureCapacity(ee, jniLocalsStack, 
			   capacity + CVMJNIFrameCapacity)) {
	/* The JNI spec says to throw an OutOfMemoryError. However,
	 * CVMensureCapacity may have thrown a StackOverflowError
	 * error instead.
	 */
	CVMclearLocalException(ee);
	CVMthrowOutOfMemoryError(ee, NULL);
	return JNI_ENOMEM;
    }

    /* We know this will succeed, since we've ensured larger capacity above */
    CVMpushGCRootFrame(ee, jniLocalsStack, currentFrame,
		       CVMJNIFrameCapacity, CVM_FRAMETYPE_JNI);
    CVMjniFrameInit(CVMgetJNIFrame(currentFrame), CVM_TRUE);
    /* Ready to go! */
    return JNI_OK;
}

/*
 * Pop local frame, and return a ref to the old frame
 */
jobject JNICALL
CVMjniPopLocalFrame(JNIEnv *env, jobject resultArg)
{
    CVMExecEnv*    currentEE      = CVMjniEnv2ExecEnv(env);
    CVMStack*      jniLocalsStack = &currentEE->interpreterStack;
    CVMFrame*      currentFrame   = jniLocalsStack->currentFrame;
    CVMJNIFrame*   jniFrame;
    CVMStackVal32* returnVal;
    CVMObjectICell* result;

    CVMassert(CVMframeIsFreelist(currentFrame));
    CVMassert(currentFrame->mb == NULL);

    /* If the resultArg is NULL, we are not responsible for handing
       the object reference to the previous stack frame. This is
       important for making the JVMTI work. See the Java 1.2 JNI spec
       additions. */
    if (resultArg == NULL) {
	CVMpopGCRootFrame(currentEE, jniLocalsStack, currentFrame);
	/* It is not required that the previous frame be JNI, because
           we're not going to try to get a "local ref" for the result
           argument. */
	return NULL;
    } else {
	result = CVMjniNonNullICellPtrFor(resultArg);
	CVMID_localrootBegin(currentEE); {
	    CVMID_localrootDeclare(CVMObjectICell, temp);

	    /*
	     * Hold the result in a temporary, so it doesn't get lost
	     * when the locals frame is popped
	     */
	    CVMID_icellAssign(currentEE, temp, result);

	    CVMpopGCRootFrame(currentEE, jniLocalsStack, currentFrame);

	    if (!CVMframeIsJNI(currentFrame)) {
		CVMjniFatalError(env, "can't return reference to "
				 "non-JNI frame");
	    }

	    CVMassert(CVMframeIsJNI(currentFrame));
	    jniFrame = CVMgetJNIFrame(currentFrame);

	    /*
	     * Now get a new slot for the result
	     */
	    CVMframeAlloc(currentEE, jniLocalsStack, jniFrame, returnVal);
	    CVMID_icellAssign(currentEE, &returnVal->ref, temp);
	} CVMID_localrootEnd();
	/*
	 * Don't return ICells containing NULL. 
	 *
	 * %comment: rt011
	 */
	if (CVMID_icellIsNull(&returnVal->ref)) {
	    return NULL;
	} else {
	    return &returnVal->ref;
	}
    }
}

/*
 * Make global ref, and make it point to the same object 'value' points to
 */
jobject JNICALL
CVMjniNewGlobalRef(JNIEnv *env, jobject ref)
{
#if 0
    /* This might break JNI code, but it is good for debugging
       cvm code */
#ifdef CVM_DEBUG_ASSERTS
    CVMassert(!CVMlocalExceptionOccurred(CVMjniEnv2ExecEnv(env)));
#endif
#endif
    if ((ref == NULL) || CVMID_icellIsNull(ref)) {
	/*
	 * If ref is NULL, or ref points to a cell containing a NULL
	 * (a cleared weak global ref), return NULL.  
	 */
	return NULL;
    } else {
	CVMExecEnv* currentEE = CVMjniEnv2ExecEnv(env);
	CVMObjectICell* cell = CVMID_getGlobalRoot(currentEE);
	if (cell != NULL) {
	    CVMID_icellAssign(currentEE, cell, ref);
#ifdef CVM_JVMPI
            if (CVMjvmpiEventJNIGlobalrefAllocIsEnabled()) {
                CVMjvmpiPostJNIGlobalrefAllocEvent(currentEE, cell);
            }
#endif /* CVM_JVMPI */
	} else {
	    CVMthrowOutOfMemoryError(currentEE, NULL);
	}
        /* If ref is a weak reference, GC could have happened before
         * unsafe assign action happened so check for null
         */
        if (CVMID_icellIsNull(cell)) {
            CVMjniDeleteGlobalRef(env, cell);
            cell = NULL;
        }
	return cell;
    }
}

/*
 * Delete global ref
 */
void JNICALL
CVMjniDeleteGlobalRef(JNIEnv *env, jobject ref)
{
    /* Apparently Sheng's book, which contains the revised JNI spec,
       says that deleting a NULL global ref is a no-op */
    /* %comment: rt012 */
    if (ref != NULL) {
	CVMExecEnv* currentEE = CVMjniEnv2ExecEnv(env);
#ifdef CVM_JVMPI
        if (CVMjvmpiEventJNIGlobalrefFreeIsEnabled()) {
            CVMjvmpiPostJNIGlobalrefFreeEvent(currentEE, ref);
        }
#endif /* CVM_JVMPI */
	CVMID_freeGlobalRoot(currentEE, ref);
    }
}

/* Purpose: Allocates a weak global ref. */
static jweak JNICALL
CVMjniNewWeakGlobalRef(JNIEnv* env, jobject ref)
{
    if ((ref == NULL) || CVMID_icellIsNull(ref)) {
	/*
	 * If ref is NULL, or ref points to a cell containing a NULL
	 * (a cleared weak global ref), return NULL.
	 */
	return NULL;
    } else {
	CVMExecEnv* currentEE = CVMjniEnv2ExecEnv(env);
	CVMObjectICell* cell = CVMID_getWeakGlobalRoot(currentEE);
	if (cell != NULL) {
	    CVMID_icellAssign(currentEE, cell, ref);
#ifdef CVM_JVMPI
            if (CVMjvmpiEventJNIWeakGlobalrefAllocIsEnabled()) {
                CVMjvmpiPostJNIWeakGlobalrefAllocEvent(currentEE, cell);
            }
#endif /* CVM_JVMPI */
	} else {
	    CVMthrowOutOfMemoryError(currentEE, NULL);
	}
	return cell;
    }
}

/* Purpose: Releases the specified weak global ref. */
static void JNICALL
CVMjniDeleteWeakGlobalRef(JNIEnv* env, jweak ref)
{
    /* Apparently Sheng's book, which contains the revised JNI spec,
       says that deleting a NULL global ref is a no-op */
    /* %comment: rt012 */
    if (ref != NULL) {
	CVMExecEnv* currentEE = CVMjniEnv2ExecEnv(env);
#ifdef CVM_JVMPI
        if (CVMjvmpiEventJNIWeakGlobalrefFreeIsEnabled()) {
            CVMjvmpiPostJNIWeakGlobalrefFreeEvent(currentEE, ref);
        }
#endif /* CVM_JVMPI */
	CVMID_freeWeakGlobalRoot(currentEE, ref);
    }
}

/*
 * CVMjniDefineClass - define a class. Used by Classloader.defineClass.
 */
static jclass JNICALL
CVMjniDefineClass(JNIEnv *env, const char *name, jobject loaderRef,
		  const jbyte *buf, jsize bufLen)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassICell* classRoot;

    /* define the class */
    classRoot =
	CVMdefineClass(ee, name, loaderRef, (CVMUint8*)buf, bufLen, NULL,
		       CVM_FALSE);
    if (classRoot == NULL) {
	return NULL;
    }

    /* load superclasses in a non-recursive way */
    (*env)->CallVoidMethod(env, classRoot,
			   CVMglobals.java_lang_Class_loadSuperClasses);
    if (CVMexceptionOccurred(ee)) {
	CVMjniDeleteLocalRef(env, classRoot);
	return NULL;
    }

    return classRoot;
}

jclass JNICALL
CVMjniFindClass(JNIEnv *env, const char *name)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);

    CVMClassLoaderICell* loader;

    CVMClassBlock* callerCb = CVMgetCallerClass(ee, 0);

    /*
     * According to JNI spec, 'name' must be a fully-qualified class name,
     * which is a package name, delimited by "/", followed by the class
     * name. First check name and throw NoClassDefFoundError if it's not
     * legal, instead of throwing the exception after the class is actually
     * created.
     */
    if (strchr(name, '.') != NULL) {
        CVMthrowNoClassDefFoundError(ee, "Bad class name %s", name);
        return NULL;
    }

#ifdef CVM_CLASSLOADING
    CVMassert(!CVMlocalExceptionOccurred(ee));

    if (callerCb != NULL) {
        /* Special handling to make sure JNI_OnLoad and JNI_OnUnload are
	 * executed in the correct class context.
	 */
	if (callerCb == CVMsystemClass(java_lang_ClassLoader_NativeLibrary)) {
	    jobject clazz;
	    CVMMethodBlock* mb = 
		CVMglobals.java_lang_ClassLoader_NativeLibrary_getFromClass;
	    clazz = (*env)->CallStaticObjectMethod(env,
						   CVMcbJavaInstance(callerCb),
						   mb);
	    if (CVMexceptionOccurred(ee)) {
		return NULL;
	    }
	    callerCb = CVMgcSafeClassRef2ClassBlock(ee, clazz);
	}
	loader = CVMcbClassLoader(callerCb);
    } else {
	loader = CVMclassGetSystemClassLoader(ee);
	if (loader == NULL) {
	    return NULL;   /* exception already thrown */
	}
    }
#else
    loader = NULL;
#endif /* CVM_CLASSLOADING */

    CVMassert(!CVMlocalExceptionOccurred(ee));
    {
	CVMObjectICell* pd = callerCb ? CVMcbProtectionDomain(callerCb) : NULL;
	CVMClassBlock* cb = 
	    CVMclassLookupByNameFromClassLoader(ee, name, CVM_TRUE, 
						loader, pd, CVM_TRUE);
	if (cb != NULL) {
	    return CVMjniNewLocalRef(env, CVMcbJavaInstance(cb));
	}
	return NULL;
    }
}

/*
 * Native Bootstrap
 */

jint JNICALL
CVMjniRegisterNatives(JNIEnv *env, jclass clazz,
		      const JNINativeMethod *methods, jint nMethods)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock *cb = CVMgcSafeClassRef2ClassBlock(ee, clazz);
    CVMBool inROM = CVMcbIsInROM(cb);
    int i;

    for (i = 0; i < nMethods; ++i) {
        struct CVMMethodBlock *mb;
        char *name = methods[i].name;
        char *sig = methods[i].signature;

	mb = CVMclassGetDeclaredMethodBlock(ee, cb, name, sig);
        if (mb == NULL || !CVMmbIs(mb, NATIVE)) {
            CVMthrowNoSuchMethodError(ee, "%s", name);
            return JNI_EINVAL;
	}
    }

    /* %comment: rt013 */
    for (i = 0; i < nMethods; ++i) {
        struct CVMMethodBlock *mb;
        const char *name = methods[i].name;
        const char *sig = methods[i].signature;
        void *nativeProc = methods[i].fnPtr;
	int invoker;

	mb = CVMclassGetDeclaredMethodBlock(ee, cb, name, sig);
	CVMassert(mb != NULL && CVMmbIs(mb, NATIVE));
#ifdef JDK12
	if (verbose_jni) {
	    jio_fprintf(stderr, "[Registering JNI native method %s.%s]\n",
			cbName(mb->fb.clazz), mb->fb.name);
	}
#endif
	invoker = CVMmbIs(mb, SYNCHRONIZED) ? CVM_INVOKE_JNI_SYNC_METHOD
					    : CVM_INVOKE_JNI_METHOD;
	if (inROM) {
	    CVMassert(CVMmbInvokerIdx(mb) == invoker);
	    CVMassert(CVMmbNativeCode(mb) == nativeProc);
#ifdef CVM_JIT
	    CVMassert(CVMmbJitInvoker(mb) == (void*)CVMCCMinvokeJNIMethod);
#endif
	} else {
#ifdef CVM_JVMTI
	    if (CVMjvmtiShouldPostNativeMethodBind()) {
		CVMUint8* new_nativeCode = NULL;
		CVMjvmtiPostNativeMethodBind(ee, mb, (CVMUint8*)nativeProc,
					     &new_nativeCode);
		if (new_nativeCode != NULL) {
		    nativeProc = (void *)new_nativeCode;
		}
	    }
#endif
	    CVMmbSetInvokerIdx(mb, invoker);
	    CVMmbNativeCode(mb) = (CVMUint8 *)nativeProc;
#ifdef CVM_JIT
	    CVMmbJitInvoker(mb) = (void*)CVMCCMinvokeJNIMethod;
#endif
	}
    }

    return JNI_OK;
}

jint JNICALL
CVMjniUnregisterNatives(JNIEnv *env, jclass clazz)
{
#ifdef CVM_CLASSLOADING
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock *cb = CVMgcSafeClassRef2ClassBlock(ee, clazz);
    CVMBool inROM = CVMcbIsInROM(cb);
    int i;

    if (inROM) {
	return JNI_OK;
    }

    /* %comment: rt013 */
    for (i = 0; i < CVMcbMethodCount(cb); ++i) {
        CVMMethodBlock* mb = CVMcbMethodSlot(cb, i);

	if (CVMmbIs(mb, NATIVE)) {
	    CVMmbSetInvokerIdx(mb, CVM_INVOKE_LAZY_JNI_METHOD);
	    CVMmbNativeCode(mb) = NULL;
#ifdef CVM_JIT
	    CVMmbJitInvoker(mb) = (void*)CVMCCMletInterpreterDoInvoke;
#endif
	}
    }
    return JNI_OK;

#else
    return JNI_ERR;
#endif
}


static jmethodID
CVMjniGetXMethodID(JNIEnv *env,
    jclass clazz, const char *name, const char *sig, jboolean isStatic)
{
    CVMExecEnv *    ee;
    CVMClassBlock*  cb;
    CVMMethodTypeID tid;
    CVMMethodBlock* mb;

    ee  = CVMjniEnv2ExecEnv(env);
    cb  = CVMjniGcSafeRef2Class(ee, clazz);

    if (!CVMclassInit(ee, cb)) {
	return NULL;
    }

    /*
     * We need to use the "new" function, because the "lookup" function
     * is only valid when we know that the type already exists.
     */
    tid = CVMtypeidNewMethodIDFromNameAndSig(ee, name, sig);

    if (tid != CVM_TYPEID_ERROR) {
	/* never search superclasses for constructors */
	mb  = CVMclassGetMethodBlock(cb, tid, isStatic);
	CVMtypeidDisposeMethodID(ee, tid);
    } else {
	mb = NULL;
	CVMclearLocalException(ee);
    }

    if (mb == NULL) {
        CVMthrowNoSuchMethodError(ee, "%s", name);
	return NULL;
    }
    return mb;
}

jmethodID JNICALL
CVMjniGetStaticMethodID(JNIEnv *env,
    jclass clazz, const char *name, const char *sig)
{
    return CVMjniGetXMethodID(env, clazz, name, sig, JNI_TRUE);
}

jmethodID JNICALL
CVMjniGetMethodID(JNIEnv *env,
    jclass clazz, const char *name, const char *sig)
{
    return CVMjniGetXMethodID(env, clazz, name, sig, JNI_FALSE);
}

static jfieldID
CVMjniGetXFieldID(JNIEnv *env,
    jclass clazz, const char *name, const char *sig, jboolean isStatic)
{
    CVMExecEnv *    ee;
    CVMClassBlock*  cb;
    CVMFieldTypeID       tid;
    CVMFieldBlock*  fb;

    ee  = CVMjniEnv2ExecEnv(env);
    cb  = CVMjniGcSafeRef2Class(ee, clazz);

    if (!CVMclassInit(ee, cb)) {
	return NULL;
    }

    /*
     * We need to use the "new" function, because the "lookup" function
     * is only valid when we know that the type already exists.
     */
    tid = CVMtypeidNewFieldIDFromNameAndSig(ee, name, sig);

    if (tid != CVM_TYPEID_ERROR) {
	fb  = CVMclassGetFieldBlock(cb, tid, isStatic);
	CVMtypeidDisposeFieldID(ee, tid);
    } else {
	fb = NULL;
	CVMclearLocalException(ee);
    }

    if (fb == NULL || (CVMfbIs(fb, STATIC) != isStatic)) {
        CVMthrowNoSuchFieldError(ee, "%s", name);
	return NULL ;
    }
    return fb;
}

jfieldID JNICALL
CVMjniGetStaticFieldID(JNIEnv *env,
    jclass clazz, const char *name, const char *sig)
{
    return CVMjniGetXFieldID(env, clazz, name, sig, JNI_TRUE);
}

jfieldID JNICALL
CVMjniGetFieldID(JNIEnv *env,
    jclass clazz, const char *name, const char *sig)
{
    return CVMjniGetXFieldID(env, clazz, name, sig, JNI_FALSE);
}

/*
 * Make sure we are accessing a real field
 */
#ifdef CVM_DEBUG
static void 
CVMjniSanityCheckFieldAccess(JNIEnv* env, jobject jobj, jfieldID jfb)
{
    CVMClassBlock* objCb;
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);

    /* Simple checks */
    CVMassert(jobj != NULL);
    CVMassert(jfb != NULL);
    CVMassert(!CVMfbIs(jfb, STATIC));
    /* Also make sure that the object class is a subclass of the
       field class */
    CVMID_objectGetClass(ee, jobj, objCb);
    CVMassert(CVMisSubclassOf(ee, objCb, CVMfbClassBlock(jfb)));
}
#else
#define CVMjniSanityCheckFieldAccess(env, jobj, jfb)
#endif

#define CVM_DEFINE_JNI_FIELD_GETTER_AND_SETTER(jType_, elemType_,	\
					       wideType_)		\
jType_ JNICALL								\
CVMjniGet##elemType_##Field(JNIEnv* env, jobject obj, jfieldID fid)	\
{									\
    CVMExecEnv* ee  = CVMjniEnv2ExecEnv(env);				\
    jType_      retVal;							\
									\
    CVMjniSanityCheckFieldAccess(env, obj, fid);			\
    CVMID_fieldRead##wideType_(ee, obj, CVMfbOffset(fid), retVal);     	\
    return retVal;							\
}									\
									\
void JNICALL								\
CVMjniSet##elemType_##Field(JNIEnv* env, jobject obj, 			\
			    jfieldID fid, jType_ rhs)			\
{									\
    CVMExecEnv* ee  = CVMjniEnv2ExecEnv(env);				\
									\
    CVMjniSanityCheckFieldAccess(env, obj, fid);			\
    CVMID_fieldWrite##wideType_(ee, obj, CVMfbOffset(fid), rhs);       	\
}

CVM_DEFINE_JNI_FIELD_GETTER_AND_SETTER(jboolean, Boolean, Int)
CVM_DEFINE_JNI_FIELD_GETTER_AND_SETTER(jchar,    Char,    Int)
CVM_DEFINE_JNI_FIELD_GETTER_AND_SETTER(jshort,   Short,   Int)
CVM_DEFINE_JNI_FIELD_GETTER_AND_SETTER(jfloat,   Float,   Float)
CVM_DEFINE_JNI_FIELD_GETTER_AND_SETTER(jbyte,    Byte,    Int)
CVM_DEFINE_JNI_FIELD_GETTER_AND_SETTER(jint,     Int,     Int)

/*
 * The reference version is slightly different, so don't use above
 * macro.
 */
jobject JNICALL
CVMjniGetObjectField(JNIEnv* env, jobject obj, jfieldID fid)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMObjectICell* resultCell = CVMjniCreateLocalRef(ee);
    if (resultCell == NULL) {
	return NULL;
    }

    CVMjniSanityCheckFieldAccess(env, obj, fid);
    CVMID_fieldReadRef(ee, obj, CVMfbOffset(fid), resultCell);
    /* Don't return ICells containing NULL */
    if (CVMID_icellIsNull(resultCell)) {
	CVMjniDeleteLocalRef(env, resultCell);
	resultCell = NULL;
    }
    return resultCell;
}

void JNICALL
CVMjniSetObjectField(JNIEnv* env, jobject obj, jfieldID fid, jobject rhs)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMjniSanityCheckFieldAccess(env, obj, fid);
    CVMID_fieldWriteRef(ee, obj, CVMfbOffset(fid), 
			CVMjniNonNullICellPtrFor(rhs));
}

#define CVM_DEFINE_JNI_64BIT_FIELD_GETTER_AND_SETTER(jType_, elemType_)	\
									\
jType_ JNICALL								\
CVMjniGet##elemType_##Field(JNIEnv* env, jobject obj, jfieldID fid)	\
{									\
    CVMExecEnv* ee  = CVMjniEnv2ExecEnv(env);				\
    jType_ v64;								\
    CVMFieldBlock* fb = (CVMFieldBlock*)fid;				\
									\
    CVMjniSanityCheckFieldAccess(env, obj, fid);			\
    if (CVMfbIs(fb, VOLATILE)) {					\
	CVM_ACCESS_VOLATILE_LOCK(ee);					\
    }									\
    CVMID_fieldRead##elemType_(ee, obj, CVMfbOffset(fid), v64);		\
    if (CVMfbIs(fb, VOLATILE)) {					\
	CVM_ACCESS_VOLATILE_UNLOCK(ee);					\
    }									\
    return v64;								\
}									\
									\
void JNICALL								\
CVMjniSet##elemType_##Field(JNIEnv* env, jobject obj, 			\
			    jfieldID fid, jType_ rhs)			\
{									\
    CVMExecEnv* ee  = CVMjniEnv2ExecEnv(env);				\
    CVMFieldBlock* fb = (CVMFieldBlock*)fid;				\
    CVMjniSanityCheckFieldAccess(env, obj, fid);			\
    if (CVMfbIs(fb, VOLATILE)) {					\
	CVM_ACCESS_VOLATILE_LOCK(ee);					\
    }									\
    CVMID_fieldWrite##elemType_(ee, obj, CVMfbOffset(fid), rhs);       	\
    if (CVMfbIs(fb, VOLATILE)) {					\
	CVM_ACCESS_VOLATILE_UNLOCK(ee);					\
    }									\
}

CVM_DEFINE_JNI_64BIT_FIELD_GETTER_AND_SETTER(jlong,   Long)
CVM_DEFINE_JNI_64BIT_FIELD_GETTER_AND_SETTER(jdouble, Double)

/* Static fields */

#ifdef CVM_DEBUG_ASSERTS
static CVMBool 
CVMisInnerClassOf(CVMExecEnv* ee, CVMClassBlock* icb, CVMClassBlock* fcb)
{
    CVMClassBlock* outerClassBlock;
    CVMUint16 outerIdx = 0;
    int i = 0;

    if (CVMcbInnerClassesInfoCount(icb) == 0) {
        return CVM_FALSE;
    } else { 
        CVMConstantPool* cp = CVMcbConstantPool(icb);
        for (i=0; i<CVMcbInnerClassesInfoCount(icb); i++) {
            outerIdx = CVMcbInnerClassInfo(icb, i)->outerClassIndex;
            /* This cannot happen -- the outer class entry must
	       be resolved */
            CVMassert(CVMcpCheckResolvedAndGetTID(ee, icb, cp, 
						  outerIdx, NULL));
            outerClassBlock = CVMcpGetCb(cp, outerIdx);
            if (outerClassBlock == fcb) {
                return CVM_TRUE;
            }
        }
    }
    return CVM_FALSE;
}
#endif /* end of CVM_DEBUG_ASSERTS */

/*
 * Make sure we are accessing a real field
 */
#ifdef CVM_DEBUG_ASSERTS
static void
CVMjniSanityCheckStaticFieldAccess(JNIEnv* env, jclass clazz, jfieldID jfb)
{
    CVMClassBlock* cb;
    CVMExecEnv* ee  = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* fcb = CVMfbClassBlock(jfb);
    
    /* Simple checks */
    CVMassert(clazz != NULL);
    CVMassert(jfb != NULL);
    CVMassert(CVMfbIs(jfb, STATIC));

    /* Also make sure that clazz is the same as or a subclass of the
       field class or inner class of the field class */
     cb = CVMjniGcSafeRef2Class(ee, clazz);
     CVMassert(CVMisSubclassOf(ee, cb, fcb) || CVMisInnerClassOf(ee, cb, fcb));
   
}
#else
#define CVMjniSanityCheckStaticFieldAccess(env, jobj, jfb)
#endif

#define CVM_DEFINE_JNI_STATIC_GETTER_AND_SETTER(jType_, elemType_,	\
					       unionField_)		\
jType_ JNICALL								\
CVMjniGetStatic##elemType_##Field(JNIEnv* env, 				\
                                  jclass clazz, jfieldID fid)		\
{									\
    CVMExecEnv* ee  = CVMjniEnv2ExecEnv(env);				\
    jType_ retVal;							\
									\
    CVMjniSanityCheckStaticFieldAccess(env, clazz, fid);		\
    CVMD_gcUnsafeExec(ee, {						\
        retVal = CVMfbStaticField(ee, fid).unionField_;			\
    });									\
    return retVal;							\
}									\
									\
void JNICALL								\
CVMjniSetStatic##elemType_##Field(JNIEnv* env, jclass clazz,		\
			          jfieldID fid, jType_ rhs)		\
{									\
    CVMExecEnv* ee  = CVMjniEnv2ExecEnv(env);				\
									\
    CVMjniSanityCheckStaticFieldAccess(env, clazz, fid);		\
    CVMD_gcUnsafeExec(ee, {						\
        CVMfbStaticField(ee, fid).unionField_ = rhs;			\
    });									\
}

CVM_DEFINE_JNI_STATIC_GETTER_AND_SETTER(jboolean, Boolean, i)
CVM_DEFINE_JNI_STATIC_GETTER_AND_SETTER(jchar,    Char,    i)
CVM_DEFINE_JNI_STATIC_GETTER_AND_SETTER(jshort,   Short,   i)
CVM_DEFINE_JNI_STATIC_GETTER_AND_SETTER(jfloat,   Float,   f)
CVM_DEFINE_JNI_STATIC_GETTER_AND_SETTER(jbyte,    Byte,    i)
CVM_DEFINE_JNI_STATIC_GETTER_AND_SETTER(jint,     Int,     i)

/*
 * The reference versions are slightly different, so don't use above
 * macro.
 */
jobject JNICALL
CVMjniGetStaticObjectField(JNIEnv* env, jclass clazz, jfieldID fid)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMObjectICell* resultCell;


    CVMjniSanityCheckStaticFieldAccess(env, clazz, fid);

    /* Don't return ICells containing NULL */
    if (CVMID_icellIsNull(&CVMfbStaticField(ee, fid).r)) {
	return NULL;
    }

    resultCell = CVMjniCreateLocalRef(ee);
    if (resultCell == NULL) {
	return NULL;
    }

    CVMD_gcUnsafeExec(ee, {
        CVMID_icellAssignDirect(ee,
				resultCell,
				&CVMfbStaticField(ee, fid).r);
    });
    return resultCell;
}

void JNICALL
CVMjniSetStaticObjectField(JNIEnv* env, jclass clazz, jfieldID fid,
			   jobject rhs)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMjniSanityCheckStaticFieldAccess(env, clazz, fid);
    CVMD_gcUnsafeExec(ee, {
        CVMID_icellAssignDirect(ee,
				&CVMfbStaticField(ee, fid).r,
				CVMjniNonNullICellPtrFor(rhs));
    });
}

#define CVM_DEFINE_JNI_64BIT_STATIC_GETTER_AND_SETTER(jType_, 		\
                                                     elemType_,		\
                                                     elemType2_)	\
jType_ JNICALL								\
CVMjniGetStatic##elemType_##Field(JNIEnv* env, 				\
                                  jclass clazz, jfieldID fid)		\
{									\
    CVMExecEnv* ee  = CVMjniEnv2ExecEnv(env);				\
    jType_ v64;								\
									\
    CVMjniSanityCheckStaticFieldAccess(env, clazz, fid);		\
    CVMD_gcUnsafeExec(ee, {						\
        CVMFieldBlock* fb = (CVMFieldBlock*)fid;			\
	if (CVMfbIs(fb, VOLATILE)) {				        \
	    CVM_ACCESS_VOLATILE_LOCK(ee);				\
	}								\
	v64 = CVMjvm2##elemType_(&CVMfbStaticField(ee, fid).raw);	\
	if (CVMfbIs(fb, VOLATILE)) {					\
	    CVM_ACCESS_VOLATILE_UNLOCK(ee);				\
	}								\
    });									\
    return v64;								\
}									\
									\
void JNICALL								\
CVMjniSetStatic##elemType_##Field(JNIEnv* env, jclass clazz,		\
			          jfieldID fid, jType_ rhs)		\
{									\
    CVMExecEnv* ee  = CVMjniEnv2ExecEnv(env);				\
    CVMjniSanityCheckStaticFieldAccess(env, clazz, fid);		\
    CVMD_gcUnsafeExec(ee, {						\
	CVMFieldBlock* fb = (CVMFieldBlock*)fid;			\
	if (CVMfbIs(fb, VOLATILE)) {				        \
	    CVM_ACCESS_VOLATILE_LOCK(ee);				\
	}								\
	CVM##elemType2_##2Jvm(&CVMfbStaticField(ee, fid).raw, rhs);	\
	if (CVMfbIs(fb, VOLATILE)) {					\
	    CVM_ACCESS_VOLATILE_UNLOCK(ee);				\
	}								\
    });									\
}

CVM_DEFINE_JNI_64BIT_STATIC_GETTER_AND_SETTER(jlong,   Long,   long)
CVM_DEFINE_JNI_64BIT_STATIC_GETTER_AND_SETTER(jdouble, Double, double)

jobject JNICALL
CVMjniAllocObject(JNIEnv *env, jclass clazz)
{
    CVMExecEnv*     ee = CVMjniEnv2ExecEnv(env);
    CVMObjectICell* resultCell;
    CVMClassBlock*  cb = CVMjniGcSafeRef2Class(ee, clazz);

    resultCell = CVMjniCreateLocalRef(ee);
    if (resultCell == NULL) {
	return NULL;
    }

    if (CVMcbIs(cb,INTERFACE) || CVMcbIs(cb,ABSTRACT)) {
	CVMthrowInstantiationException(ee, "%C", cb);
    } else if (!CVMcbCheckRuntimeFlag(cb, LINKED) &&
	       !CVMclassLink(ee,cb, CVM_FALSE)) {
	/* exception already thrown */
    } else {
	CVMD_gcUnsafeExec(ee, {
	    CVMObject* obj = NULL;
	    obj = CVMgcAllocNewInstance(ee, cb);
	    if (obj == NULL) {
		CVMthrowOutOfMemoryError(ee, NULL);
	    } else {
		CVMID_icellSetDirect(ee, resultCell, obj);
	    }
	});
    }
    
    /* Free the localref if we didn't allocate an object. */
    if (CVMID_icellIsNull(resultCell)) {
	CVMjniDeleteLocalRef(env, resultCell);
	resultCell = NULL;
    }
    return resultCell;
}

static jobject
CVMjniConstruct(JNIEnv *env, jclass clazz, jmethodID methodID,
	        JNI_PushArguments_t pushArguments, const void *args)
{
#ifndef JAVASE
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
#endif
    jobject result;
    CVMMethodBlock *mb = methodID;

    result = CVMjniAllocObject(env, clazz);

    if (result == NULL) {
        return NULL;
    }

    if (!CVMtypeidIsConstructor(CVMmbNameAndTypeID(mb))) {
        CVMjniFatalError(env, "a non-constructor passed to NewObject");
    }
#ifndef JAVASE
    if (CVMmbClassBlock(mb) != CVMjniGcSafeRef2Class(ee, clazz)) {
	CVMjniFatalError(env, "wrong method ID passed to NewObject");
    }
#endif

#ifdef JDK12
    CVMjniInvoke(env, result, methodID, pushArguments, args,
		 CVM_INVOKE_VIRTUAL | CVM_TYPEID_VOID, NULL);
#else
    CVMjniInvoke(env, result, methodID, pushArguments, args,
		 CVM_INVOKE_NONVIRTUAL | CVM_TYPEID_VOID, NULL);
#endif

    if (CVMjniExceptionCheck(env)) {
	CVMjniDeleteLocalRef(env, result);
	return NULL;
    }
    return result;
}

typedef struct {
    va_list args;
} CVMVaList;

jobject JNICALL
CVMjniNewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
    jobject result;
    CVMVaList v;
    va_start(v.args, methodID);
    result = CVMjniConstruct(env, clazz, methodID, CVMjniPushArgumentsVararg,
			     &v);
    va_end(v.args);
    return result;
}

jobject JNICALL
CVMjniNewObjectA(JNIEnv *env, jclass clazz, jmethodID methodID,
		 const jvalue *args)
{
    return CVMjniConstruct(env, clazz, methodID, CVMjniPushArgumentsArray,
			   args);
}

jobject JNICALL
CVMjniNewObjectV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
    CVMVaList v;
    jobject result;
    va_copy(v.args, args);
    result = CVMjniConstruct(env, clazz, methodID, CVMjniPushArgumentsVararg,
			    &v);
    va_end(v.args);
    return result;
}


jclass JNICALL
CVMjniGetObjectClass(JNIEnv *env, jobject obj)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock *cb;
    CVMID_objectGetClass(ee, obj, cb);
    return CVMjniNewLocalRef(env, CVMcbJavaInstance(cb));
}

jboolean JNICALL
CVMjniIsSameObject(JNIEnv *env, jobject ref1, jobject ref2)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    jboolean res;
    jobject nonnullRef1 = CVMjniNonNullICellPtrFor(ref1);
    jobject nonnullRef2 = CVMjniNonNullICellPtrFor(ref2);
    CVMID_icellSameObject(ee, nonnullRef1, nonnullRef2, res);
    return res;
}

jboolean JNICALL
CVMjniIsInstanceOf(JNIEnv* env, jobject obj, jclass clazz)
{
    if (obj == NULL) {
	return JNI_TRUE;
    } else {
	CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
	CVMBool     retVal;
	CVMD_gcUnsafeExec(ee, {
	    CVMClassBlock* cb = CVMjniGcUnsafeRef2Class(ee, clazz);
	    CVMObject* directObj = CVMID_icellDirect(ee, obj);
	    retVal = CVMgcUnsafeIsInstanceOf(ee, directObj, cb);
	});
	return retVal;
    }
}

/*
 * Array related operations
 *
 * Define CVMjni{Get,Set}<PrimitiveType>ArrayRegion() functions 
 * (16 total)
 */
#define CVM_DEFINE_JNI_ARRAY_REGION_ACCESSORS(arrType, jelemType, nativeType) \
void JNICALL								   \
CVMjniGet##jelemType##ArrayRegion(JNIEnv *env, arrType array, jsize start, \
				  jsize len, nativeType *buf)		   \
{									   \
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);				   \
    CVMD_gcUnsafeExec(ee, {						   \
	CVMArrayOf##jelemType* directArray =				   \
	    (CVMArrayOf##jelemType*)CVMID_icellDirect(ee, array);	   \
	CVMUint32 arrLen = CVMD_arrayGetLength(directArray);		   \
									   \
	/*								   \
	 * Make sure the java-side array and the passed in buffer    	   \
	 * have the same size elements.					   \
	 */								   \
        /* NOTE:							   \
	 * In case of 64 bit the following assert can not be used          \
	 * because for jlong the arrayElemSize is 16 and sizeof jlong is   \
	 * 8 byte.                                                         \
	 */								   \
	CVMassert(CVMarrayElemSize(CVMobjectGetClass(directArray)) == 	   \
		  sizeof(nativeType));					   \
	/* Check for bounds and copy */					   \
	if ((start >= 0) && (len >= 0) && 				   \
	    ((CVMUint32)start + (CVMUint32)len <= arrLen)) {		   \
            CVMD_arrayReadBody##jelemType(buf, directArray, start, len);   \
	} else {							   \
	    CVMthrowArrayIndexOutOfBoundsException(ee, NULL);		   \
	}								   \
    });									   \
}									   \
void JNICALL								   \
CVMjniSet##jelemType##ArrayRegion(JNIEnv *env, arrType array, jsize start, \
				 jsize len, const nativeType *buf)	   \
{									   \
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);				   \
    CVMD_gcUnsafeExec(ee, {						   \
	CVMArrayOf##jelemType* directArray =				   \
	    (CVMArrayOf##jelemType*)CVMID_icellDirect(ee, array);	   \
	CVMUint32 arrLen = CVMD_arrayGetLength(directArray);		   \
									   \
	/*								   \
	 * Make sure the java-side array and the passed in buffer    	   \
	 * have the same size elements.					   \
	 */								   \
        /* NOTE:                                                           \
	 * In case of 64 bit the following assert can not be used          \
	 * because for jlong the arrayElemSize is 16 and sizeof jlong is   \
	 * 8 byte.                                                         \
	 */                                                                \
	CVMassert(CVMarrayElemSize(CVMobjectGetClass(directArray)) == 	   \
		  sizeof(nativeType));					   \
	/* Check for bounds and copy */					   \
	if ((start >= 0) && (len >= 0) && 				   \
	    ((CVMUint32)start + (CVMUint32)len <= arrLen)) {		   \
            CVMD_arrayWriteBody##jelemType(buf, directArray, start, len);  \
	} else {							   \
	    CVMthrowArrayIndexOutOfBoundsException(ee, NULL);		   \
	}								   \
    });									   \
}									   \

CVM_DEFINE_JNI_ARRAY_REGION_ACCESSORS(jbooleanArray, Boolean, jboolean)
CVM_DEFINE_JNI_ARRAY_REGION_ACCESSORS(jbyteArray,    Byte,    jbyte)
CVM_DEFINE_JNI_ARRAY_REGION_ACCESSORS(jcharArray,    Char,    jchar)
CVM_DEFINE_JNI_ARRAY_REGION_ACCESSORS(jshortArray,   Short,   jshort)
CVM_DEFINE_JNI_ARRAY_REGION_ACCESSORS(jintArray,     Int,     jint)
CVM_DEFINE_JNI_ARRAY_REGION_ACCESSORS(jfloatArray,   Float,   jfloat)
CVM_DEFINE_JNI_ARRAY_REGION_ACCESSORS(jlongArray,    Long,    jlong)
CVM_DEFINE_JNI_ARRAY_REGION_ACCESSORS(jdoubleArray,  Double,  jdouble)

#define CVM_DEFINE_JNI_ARRAY_ELEMENTS_ACCESSORS(arrType, jelemType, nativeType) \
nativeType* JNICALL							   \
CVMjniGet##jelemType##ArrayElements(JNIEnv *env, arrType array,		   \
                                    jboolean *isCopy)			   \
{									   \
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);				   \
    CVMJavaInt  arrLen;							   \
    nativeType* buf;							   \
    CVMArrayOf##jelemType##ICell* arrayCell = 				   \
        (CVMArrayOf##jelemType##ICell*)array;				   \
									   \
    CVMID_arrayGetLength(ee, arrayCell, arrLen);			   \
    buf = (nativeType*)malloc(arrLen * sizeof(nativeType));		   \
    if (buf == NULL) {							   \
	CVMthrowOutOfMemoryError(ee, NULL);    				   \
        return NULL;							   \
    }									   \
    if (isCopy != NULL) {						   \
        *isCopy = JNI_TRUE;						   \
    }								   	   \
    CVMD_gcUnsafeExec(ee, {						   \
	CVMArrayOf##jelemType* directArray =				   \
	    (CVMArrayOf##jelemType*)CVMID_icellDirect(ee, array);	   \
									   \
	/*								   \
	 * Make sure the java-side array and the passed in buffer    	   \
	 * have the same size elements.					   \
	 */								   \
        /* NOTE:							   \
	 * In case of 64 bit the following assert can not be used          \
	 * because for jlong the arrayElemSize is 16 and sizeof jlong is   \
	 * 8 byte.                                                         \
	 */                                                                \
	CVMassert(CVMarrayElemSize(CVMobjectGetClass(directArray)) ==      \
		  sizeof(nativeType));					   \
        CVMD_arrayReadBody##jelemType(buf, directArray, 0, arrLen);	   \
    });									   \
    return buf;								   \
}									   \
									   \
void JNICALL							   	   \
CVMjniRelease##jelemType##ArrayElements(JNIEnv *env, arrType array,	   \
                                        nativeType* buf, jint mode)   	   \
{									   \
    CVMExecEnv* ee;							   \
    if (mode == JNI_ABORT) {						   \
        free(buf);						   	   \
	return;								   \
    }									   \
    ee = CVMjniEnv2ExecEnv(env);				   	   \
    CVMD_gcUnsafeExec(ee, {						   \
	CVMArrayOf##jelemType* directArray =				   \
	    (CVMArrayOf##jelemType*)CVMID_icellDirect(ee, array);	   \
	CVMUint32 arrLen = CVMD_arrayGetLength(directArray);		   \
									   \
	/*								   \
	 * Make sure the java-side array and the passed in buffer    	   \
	 * have the same size elements.					   \
	 */								   \
        /* NOTE:                                                           \
	 * in case of 64 bit the following assert can not be used          \
	 * because for jlong the arrayElemSize is 16 and sizeof jlong is   \
	 * 8 byte.                                                         \
	 */                                                                \
	CVMassert(CVMarrayElemSize(CVMobjectGetClass(directArray)) == \
		  sizeof(nativeType));					   \
        CVMD_arrayWriteBody##jelemType(buf, directArray, 0, arrLen);	   \
    });									   \
    if (mode != JNI_COMMIT) {						   \
        CVMassert(mode == 0);						   \
        free(buf);						   	   \
    }									   \
}									   \

CVM_DEFINE_JNI_ARRAY_ELEMENTS_ACCESSORS(jbooleanArray, Boolean, jboolean)
CVM_DEFINE_JNI_ARRAY_ELEMENTS_ACCESSORS(jbyteArray,    Byte,    jbyte)
CVM_DEFINE_JNI_ARRAY_ELEMENTS_ACCESSORS(jcharArray,    Char,    jchar)
CVM_DEFINE_JNI_ARRAY_ELEMENTS_ACCESSORS(jshortArray,   Short,   jshort)
CVM_DEFINE_JNI_ARRAY_ELEMENTS_ACCESSORS(jintArray,     Int,     jint)
CVM_DEFINE_JNI_ARRAY_ELEMENTS_ACCESSORS(jfloatArray,   Float,   jfloat)
CVM_DEFINE_JNI_ARRAY_ELEMENTS_ACCESSORS(jlongArray,    Long,    jlong)
CVM_DEFINE_JNI_ARRAY_ELEMENTS_ACCESSORS(jdoubleArray,  Double,  jdouble)

/*
 * The conditionally copying versions of
 * CVMjni{Get,Release}PrimitiveArrayCritical().
 */

static CVMBool
mustCopy(CVMBasicType base)
{
#ifdef CVMGC_HAS_NONREF_BARRIERS
    return CVM_TRUE;   /* always copy if there are gc barriers */
#else
#ifndef CAN_DO_UNALIGNED_DOUBLE_ACCESS
    if (base == CVM_T_DOUBLE) {
	return CVM_TRUE; /* copy if we can't do unaligned double access */
    }
#endif
#ifndef CAN_DO_UNALIGNED_INT64_ACCESS
    if (base == CVM_T_LONG) {
	return CVM_TRUE; /* copy if we can't do unaligned long long access */
    }
#endif
    return CVM_FALSE;
#endif
}

void* JNICALL
CVMjniGetPrimitiveArrayCritical(JNIEnv *env, jarray array, jboolean *isCopy)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMJavaInt  arrLen;
    void*       buf = NULL;  /* avoid compiler warning */
    CVMClassBlock*       arrayCb;
    CVMBasicType         base;
    CVMArrayOfAnyType*  directArray;

    /*
     * Become gcUnsafe and stay that way until outermost 
     * CVMjniReleasePrimitiveArrayCritical call is made.
     */
    CVMD_gcEnterCriticalRegion(ee);

    directArray = (CVMArrayOfAnyType*)CVMID_icellDirect(ee, array);
    arrayCb = CVMobjectGetClass((CVMObject*)directArray);
    arrLen = CVMD_arrayGetLength(directArray);
    base = CVMarrayBaseType(arrayCb);

    /*
     * This had better be an array of primitives
     */
    CVMassert(base != CVM_T_CLASS);
    CVMassert(CVMarrayDepth(arrayCb) == 1);

    /*
     * If there is no requirement that we copy the array, then just return
     * the pointer to it.
     */
    if (!mustCopy(base)) {
	buf = (void*)&directArray->elems;
	if (isCopy != NULL) {
	    *isCopy = JNI_FALSE;
	}
	return buf;
    }

    /*
     * The rest of this function is only needed if there is a possible case
     * that will require copying the array.
     */  
#if defined(CVMGC_HAS_NONREF_BARRIERS) ||	\
    !defined(CAN_DO_UNALIGNED_DOUBLE_ACCESS) ||	\
    !defined(CAN_DO_UNALIGNED_INT64_ACCESS)

    buf = malloc(arrLen * CVMarrayElemSize(arrayCb));
    if (buf == NULL) {
	CVMD_gcExitCriticalRegion(ee);
	CVMthrowOutOfMemoryError(ee, NULL);
        return NULL;
    }
    if (isCopy != NULL) {
	*isCopy = JNI_TRUE;
    }

    /* %comment rt015 */
    switch(base) {
#ifdef CVMGC_HAS_NONREF_BARRIERS
        case CVM_T_BOOLEAN: {
	    CVMArrayOfBoolean* dirArr = (CVMArrayOfBoolean*)directArray;
	    CVMD_arrayReadBodyBoolean(buf, dirArr, 0, arrLen);
	    break;
	}
        case CVM_T_INT: {
	    CVMArrayOfInt* dirArr = (CVMArrayOfInt*)directArray;
	    CVMD_arrayReadBodyInt(buf, dirArr, 0, arrLen);
	    break;
	}
        case CVM_T_FLOAT: {
	    CVMArrayOfFloat* dirArr = (CVMArrayOfFloat*)directArray;
	    CVMD_arrayReadBodyFloat(buf, dirArr, 0, arrLen);
	    break;
	}
        case CVM_T_BYTE: {
	    CVMArrayOfByte* dirArr = (CVMArrayOfByte*)directArray;
	    CVMD_arrayReadBodyByte(buf, dirArr, 0, arrLen);
	    break;
	}
        case CVM_T_CHAR: {
	    CVMArrayOfChar* dirArr = (CVMArrayOfChar*)directArray;
	    CVMD_arrayReadBodyChar(buf, dirArr, 0, arrLen);
	    break;
	}
        case CVM_T_SHORT: {
	    CVMArrayOfShort* dirArr = (CVMArrayOfShort*)directArray;
	    CVMD_arrayReadBodyShort(buf, dirArr, 0, arrLen);
	    break;
	}
#endif
#if defined(CVMGC_HAS_NONREF_BARRIERS) || \
    !defined(CAN_DO_UNALIGNED_INT64_ACCESS)
        case CVM_T_LONG: {
	    CVMArrayOfLong* dirArr = (CVMArrayOfLong*)directArray;
	    CVMD_arrayReadBodyLong(buf, dirArr, 0, arrLen);
	    break;
	}
#endif
#if defined(CVMGC_HAS_NONREF_BARRIERS) || \
    !defined(CAN_DO_UNALIGNED_DOUBLE_ACCESS)
        case CVM_T_DOUBLE: {
	    CVMArrayOfDouble* dirArr = (CVMArrayOfDouble*)directArray;
	    CVMD_arrayReadBodyDouble(buf, dirArr, 0, arrLen);
	    break;
	}
#endif
        default: CVMassert(CVM_FALSE); /* can't happen */
    }
#endif
    CVMassert(buf != NULL);
    return buf;
}

void JNICALL
CVMjniReleasePrimitiveArrayCritical(JNIEnv *env, jarray array,
				    void* buf, jint mode)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMJavaInt  arrLen;
    CVMClassBlock*       arrayCb;
    CVMBasicType         base;
    CVMArrayOfAnyType*  directArray;

    directArray = (CVMArrayOfAnyType*)CVMID_icellDirect(ee, array);
    arrayCb = CVMobjectGetClass((CVMObject*)directArray);
    arrLen = CVMD_arrayGetLength(directArray);
    base = CVMarrayBaseType(arrayCb);

    /*
     * This had better be an array of primitives
     */
    CVMassert(base != CVM_T_CLASS);
    CVMassert(CVMarrayDepth(arrayCb) == 1);

    if (!mustCopy(base)) {
	if (mode != JNI_COMMIT) {
	    /* become gcSafe again if this is the outmost call */
	    CVMD_gcExitCriticalRegion(ee);
	}
	return;
    }

    /*
     * The following is only needed if there is a possible case
     * that will require copying the array.
     */  
#if defined(CVMGC_HAS_NONREF_BARRIERS) ||	\
    !defined(CAN_DO_UNALIGNED_DOUBLE_ACCESS) ||	\
    !defined(CAN_DO_UNALIGNED_INT64_ACCESS)

    if (mode == JNI_ABORT) {
	free(buf);
	/* become gcSafe again if this is the outmost call */
	CVMD_gcExitCriticalRegion(ee);
	return;
    }

    /* %comment rt015 */
    switch(base) {
#ifdef CVMGC_HAS_NONREF_BARRIERS
        case CVM_T_BOOLEAN: {
	    CVMArrayOfBoolean* dirArr = (CVMArrayOfBoolean*)directArray;
	    CVMD_arrayWriteBodyBoolean(buf, dirArr, 0, arrLen);
	    break;
	}
        case CVM_T_INT: {
	    CVMArrayOfInt* dirArr = (CVMArrayOfInt*)directArray;
	    CVMD_arrayWriteBodyInt(buf, dirArr, 0, arrLen);
	    break;
	}
        case CVM_T_FLOAT: {
	    CVMArrayOfFloat* dirArr = (CVMArrayOfFloat*)directArray;
	    CVMD_arrayWriteBodyFloat(buf, dirArr, 0, arrLen);
	    break;
	}
        case CVM_T_BYTE: {
	    CVMArrayOfByte* dirArr = (CVMArrayOfByte*)directArray;
	    CVMD_arrayWriteBodyByte(buf, dirArr, 0, arrLen);
	    break;
	}
        case CVM_T_CHAR: {
	    CVMArrayOfChar* dirArr = (CVMArrayOfChar*)directArray;
	    CVMD_arrayWriteBodyChar(buf, dirArr, 0, arrLen);
	    break;
	}
        case CVM_T_SHORT: {
	    CVMArrayOfShort* dirArr = (CVMArrayOfShort*)directArray;
	    CVMD_arrayWriteBodyShort(buf, dirArr, 0, arrLen);
	    break;
	}
#endif
#if defined(CVMGC_HAS_NONREF_BARRIERS) || \
    !defined(CAN_DO_UNALIGNED_INT64_ACCESS)
        case CVM_T_LONG: {
	    CVMArrayOfLong* dirArr = (CVMArrayOfLong*)directArray;
	    CVMD_arrayWriteBodyLong(buf, dirArr, 0, arrLen);
	    break;
	}
#endif
#if defined(CVMGC_HAS_NONREF_BARRIERS) || \
    !defined(CAN_DO_UNALIGNED_DOUBLE_ACCESS)
        case CVM_T_DOUBLE: {
	    CVMArrayOfDouble* dirArr = (CVMArrayOfDouble*)directArray;
	    CVMD_arrayWriteBodyDouble(buf, dirArr, 0, arrLen);
	    break;
	}
#endif
        default: CVMassert(CVM_FALSE); /* can't happen */
    }

    if (mode != JNI_COMMIT) {
        CVMassert(mode == 0);
	free(buf);
	/* become gcSafe again if this is the outmost call */
	CVMD_gcExitCriticalRegion(ee);
    }
#endif
}

#define CVM_DEFINE_JNI_ARRAY_ALLOCATOR(jType_, typeName_, typeCode_)	\
jType_ JNICALL								\
CVMjniNew##typeName_##Array(JNIEnv* env, jsize length)			\
{									\
    CVMExecEnv*     ee         = CVMjniEnv2ExecEnv(env);		\
    CVMObjectICell* resultCell = CVMjniCreateLocalRef(ee);		\
    CVMClassBlock*  arrayCb;						\
									\
    if (resultCell == NULL) {						\
	return NULL;							\
    }									\
									\
    arrayCb = (CVMClassBlock*)CVMbasicTypeArrayClassblocks[typeCode_];	\
    CVMID_allocNewArray(ee, typeCode_, arrayCb, length, resultCell);	\
    if (CVMID_icellIsNull(resultCell)) {				\
        CVMjniDeleteLocalRef(env, resultCell);				\
	CVMthrowOutOfMemoryError(ee, NULL);    				\
	return NULL;							\
    } else {								\
	return resultCell;						\
    }									\
}									\

CVM_DEFINE_JNI_ARRAY_ALLOCATOR(jbooleanArray, Boolean, CVM_T_BOOLEAN)
CVM_DEFINE_JNI_ARRAY_ALLOCATOR(jbyteArray,    Byte,    CVM_T_BYTE)
CVM_DEFINE_JNI_ARRAY_ALLOCATOR(jcharArray,    Char,    CVM_T_CHAR)
CVM_DEFINE_JNI_ARRAY_ALLOCATOR(jshortArray,   Short,   CVM_T_SHORT)
CVM_DEFINE_JNI_ARRAY_ALLOCATOR(jintArray,     Int,     CVM_T_INT)
CVM_DEFINE_JNI_ARRAY_ALLOCATOR(jfloatArray,   Float,   CVM_T_FLOAT)
CVM_DEFINE_JNI_ARRAY_ALLOCATOR(jlongArray,    Long,    CVM_T_LONG)
CVM_DEFINE_JNI_ARRAY_ALLOCATOR(jdoubleArray,  Double,  CVM_T_DOUBLE)

jobject JNICALL
CVMjniGetObjectArrayElement(JNIEnv* env, jarray arrArg, jsize index)
{
    CVMExecEnv*     ee         = CVMjniEnv2ExecEnv(env);
    CVMObjectICell* resultCell;
    CVMArrayOfAnyTypeICell* array   = (CVMArrayOfAnyTypeICell*)arrArg;
    jsize arrayLen;

    CVMassert(array != NULL);
    CVMassert(!CVMID_icellIsNull(array));

    CVMID_arrayGetLength(ee, array, arrayLen);
    if ((CVMUint32)index >= (CVMUint32)arrayLen) {
	CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
	return NULL;
    }

    resultCell = CVMjniCreateLocalRef(ee);
    if (resultCell == NULL) {
	return NULL;
    }

    CVMID_arrayReadRef(ee, array, index, resultCell);
    if (CVMID_icellIsNull(resultCell)) {
	CVMjniDeleteLocalRef(env, resultCell);
	return NULL;
    } else {
	return resultCell;
    }
}

/*
 * Just like 'aastore'
 */
void JNICALL
CVMjniSetObjectArrayElement(JNIEnv* env, jarray arrArg,
			    jsize index, jobject value)
{
    CVMExecEnv*     ee         = CVMjniEnv2ExecEnv(env);
    CVMArrayOfAnyTypeICell* array   = (CVMArrayOfAnyTypeICell*)arrArg;
    jsize arrayLen;

    CVMassert(array != NULL);
    CVMassert(!CVMID_icellIsNull(array));

    CVMID_arrayGetLength(ee, array, arrayLen);
    if ((CVMUint32)index >= (CVMUint32)arrayLen) {
	CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
	return;
    }

    if ((value != NULL) && !CVMID_icellIsNull(value)) {
	CVMBool assignmentOk;
	CVMD_gcUnsafeExec(ee, {
	    /* Check assignability of 'value' into 'array' */
	    CVMClassBlock* arrayType = 
		CVMobjectGetClass(CVMID_icellDirect(ee, array));
	    CVMClassBlock* elemType =
		CVMarrayElementCb(arrayType);
	    CVMClassBlock* rhsType =
		CVMobjectGetClass(CVMID_icellDirect(ee, value));

	    if (rhsType != elemType) { /* Try a quick check first */
		assignmentOk = CVMisAssignable(ee, rhsType, elemType);
	    } else {
		assignmentOk = CVM_TRUE;
	    }
	});
	if (!assignmentOk) {
	    if (!CVMlocalExceptionOccurred(ee)) {
		CVMthrowArrayStoreException(ee, NULL);
	    }
	    return;
	}
    }
    CVMID_arrayWriteRef(ee, array, index, CVMjniNonNullICellPtrFor(value));
}

/*
 * Just like anewarray, except for the initialization part.
 */
jarray JNICALL
CVMjniNewObjectArray(JNIEnv* env, jsize length,
		     jclass elementClass, jobject initialElement)
{
    CVMExecEnv*     ee         = CVMjniEnv2ExecEnv(env);
    CVMObjectICell* resultCell;
    CVMClassBlock*  arrayCb;
    CVMClassBlock*  elementClassCb = CVMjniGcSafeRef2Class(ee, elementClass);
    
    if (!CVMcbCheckRuntimeFlag(elementClassCb, LINKED) &&
	!CVMclassLink(ee,elementClassCb, CVM_FALSE)) {
	return NULL; /* exception already thrown */
    }

    arrayCb = CVMclassGetArrayOf(ee, elementClassCb);
    if (arrayCb == NULL) {
	return NULL; /* exception already thrown */
    }

    resultCell = CVMjniCreateLocalRef(ee);
    if (resultCell == NULL) {
	return NULL;
    }

    CVMID_allocNewArray(ee, CVM_T_CLASS, arrayCb, length, resultCell);
    if (CVMID_icellIsNull(resultCell)) {
	CVMjniDeleteLocalRef(env, resultCell);
        CVMthrowOutOfMemoryError(ee, NULL);
	return NULL;
    }
    if ((initialElement == NULL) ||
	CVMID_icellIsNull(initialElement)) {
	/* No need to initialize to NULL. The allocation already did that */
	return resultCell;
    } else {
	/* Check for assignability before initialization */
	CVMBool assignmentOk = CVM_TRUE;
	CVMD_gcUnsafeExec(ee, {
	    CVMObject* directInitObj = CVMID_icellDirect(ee, initialElement);
	    CVMClassBlock*   rhsType = CVMobjectGetClass(directInitObj);
	    
	    if (rhsType != elementClassCb) { /* Try a quick check first */
		assignmentOk = CVMisAssignable(ee, rhsType, elementClassCb);
	    } else {
		assignmentOk = CVM_TRUE;
	    }
	    if (assignmentOk) {
		CVMArrayOfRef* refArray =
		    (CVMArrayOfRef*)CVMID_icellDirect(ee, resultCell);
		int i;

		/* %comment: rt016 */
		for (i = 0; i < length; i++) {
		    CVMD_arrayWriteRef(refArray, i, directInitObj);
		}
	    } else {
		CVMjniDeleteLocalRef(env, resultCell);
		resultCell = NULL;
		if (!CVMlocalExceptionOccurred(ee)) {
		    CVMthrowArrayStoreException(ee, NULL);
		}
	    }
	});
	return resultCell;
    }
}

jsize JNICALL
CVMjniGetArrayLength(JNIEnv* env, jarray arrArg)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMArrayOfAnyTypeICell* array   = (CVMArrayOfAnyTypeICell*)arrArg;
    jsize arrayLen;

    CVMID_arrayGetLength(ee, array, arrayLen);
    return arrayLen;
}

/*
 * Class operations
 */
jclass JNICALL
CVMjniGetSuperclass(JNIEnv* env, jclass clazz)
{
    CVMExecEnv*    ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = CVMjniGcSafeRef2Class(ee, clazz);
    CVMClassBlock* superCb;

    if (CVMcbIs(cb, INTERFACE)) {
	return NULL;
    }
    superCb = CVMcbSuperclass(cb);
    if (superCb == NULL) {
	return NULL;
    }
    return CVMjniNewLocalRef(env, CVMcbJavaInstance(superCb));
}


jboolean JNICALL
CVMjniIsAssignableFrom(JNIEnv* env, jclass clazz1, jclass clazz2)
{
    CVMExecEnv*    ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* srcCb;
    CVMClassBlock* dstCb;

    CVMD_gcUnsafeExec(ee, {
	srcCb = CVMjniGcUnsafeRef2Class(ee, clazz1);
	dstCb = CVMjniGcUnsafeRef2Class(ee, clazz2);
    });
    return CVMisAssignable(ee, srcCb, dstCb);
}

/*
 * Reflection operations
 */

/*
 * NOTE: The following routines are conditionally compiled:
 * CVMjniToReflectedMethod
 * CVMjniToReflectedField
 * CVMjniFromReflectedMethod
 * CVMjniFromReflectedField
 */

#ifdef CVM_REFLECT

jobject JNICALL
CVMjniToReflectedMethod(JNIEnv *env, jclass clazz,
			jmethodID methodID, jboolean isStatic)
{
    CVMExecEnv*     ee = CVMjniEnv2ExecEnv(env);
    CVMMethodBlock* mb = (CVMMethodBlock *)methodID;
    CVMObjectICell* resultCell; /* The resulting java.lang.reflect.Method */

    resultCell = CVMjniCreateLocalRef(ee);
    if (resultCell != NULL) {
	CVMreflectMethodBlockToNewJavaMethod(ee, mb, resultCell);
    }
    return resultCell; /* Exception already thrown if resultCell == 0 */
}

jobject JNICALL
CVMjniToReflectedField(JNIEnv *env, jclass clazz, jfieldID fieldID,
		       jboolean isStatic)
{
    CVMExecEnv*     ee = CVMjniEnv2ExecEnv(env);
    CVMFieldBlock*  fb = (CVMFieldBlock *)fieldID;
    CVMObjectICell* resultCell; /* The resulting java.lang.reflect.Field */

    resultCell = CVMjniCreateLocalRef(ee);
    if (resultCell != NULL) {
	CVMreflectNewJavaLangReflectField(ee, fb, resultCell);
    }
    return resultCell; /* Exception already thrown if resultCell == 0 */
}

jmethodID JNICALL
CVMjniFromReflectedMethod(JNIEnv* env, jobject method)
{
    CVMExecEnv*     ee = CVMjniEnv2ExecEnv(env);
    return CVMreflectGCSafeGetMethodBlock(ee, method, NULL);
}

jfieldID JNICALL
CVMjniFromReflectedField(JNIEnv* env, jobject field)
{
    CVMExecEnv*     ee = CVMjniEnv2ExecEnv(env);
    return CVMreflectGCSafeGetFieldBlock(ee, field);
}

#else
# define CVMjniToReflectedMethod NULL
# define CVMjniToReflectedField NULL
# define CVMjniFromReflectedMethod NULL
# define CVMjniFromReflectedField NULL
#endif /* CVM_REFLECT */

/*
 * String operations
 */
#ifdef CVM_DEBUG
static void
CVMjniSanityCheckStringAccess(CVMExecEnv* ee, jstring string)
{
    CVMClassBlock* cb;
    CVMassert(string != NULL);
    CVMID_objectGetClass(ee, string, cb);
    CVMassert(cb == CVMsystemClass(java_lang_String));
}
#else
#define CVMjniSanityCheckStringAccess(ee, string)
#endif

jsize JNICALL
CVMjniGetStringLength(JNIEnv* env, jstring string)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    jsize strLen;

    CVMjniSanityCheckStringAccess(ee, string);
    CVMID_fieldReadInt(ee, string, 
		       CVMoffsetOfjava_lang_String_count,
		       strLen);
    return strLen;
}

jstring JNICALL
CVMjniNewString(JNIEnv *env, const jchar* unicodeChars, jsize len)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMStringICell* stringICell = CVMjniCreateLocalRef(ee);
    if (stringICell == NULL) {
	return NULL;
    }

    CVMnewString(ee, stringICell, unicodeChars, len);
    if (CVMID_icellIsNull(stringICell)) {
	CVMjniDeleteLocalRef(env, stringICell);
	CVMthrowOutOfMemoryError(ee, NULL);
	return NULL;
    } else {
	return stringICell;
    }
}

jsize JNICALL
CVMjniGetStringUTFLength(JNIEnv *env, jstring string)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jsize length;

    CVMjniSanityCheckStringAccess(ee, string);

    CVMID_localrootBegin(ee); {
	CVMID_localrootDeclare(CVMArrayOfCharICell, valueICellPtr);
	CVMObjectICell *valueAsObject = (CVMObjectICell *)valueICellPtr;
	CVMJavaChar c;
	CVMInt32 i, offset, len, readLen;

	CVMID_fieldReadRef(ee, string, 
			   CVMoffsetOfjava_lang_String_value,
			   valueAsObject);
	CVMID_fieldReadInt(ee, string, 
			   CVMoffsetOfjava_lang_String_offset,
			   offset);
	CVMID_fieldReadInt(ee, string, 
			   CVMoffsetOfjava_lang_String_count,
			   len);
	/*
	 * The repeated entries to CVMD_gcUnsafeExec are
	 * somewhat slow, but give GC a chance to
	 * run for very long strings.
	 */
	length = 0;
	while ( len > 0 ){
	    readLen = MIN( len, MAX_CHARS_PER_ACCESS );
	    CVMD_gcUnsafeExec(ee,{
		CVMArrayOfChar* theChars;
		theChars = CVMID_icellDirect(ee, valueICellPtr);
		for ( i = 0; i < readLen; i++ ){
		    CVMD_arrayReadChar( theChars, offset, c );
		    /*
		     * Because CVMunicodeChar2UtfLength is such a
		     * trivial function (a macro actually), I don't
		     * feel too bad about invoking it within this
		     * unsafe section. The alternative is to copy into an
		     * array and then iterate over it with the length
		     * function.
		     */
		    length += CVMunicodeChar2UtfLength(c);
		    offset++;
		}

	    });
	    len -= readLen;
	}
    } CVMID_localrootEnd();
    return length;
}

void JNICALL
CVMjniGetStringRegion(JNIEnv *env,
		      jstring string, jsize start, jsize len, jchar *buf)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);

    CVMjniSanityCheckStringAccess(ee, string);

    CVMID_localrootBegin(ee); {
	CVMID_localrootDeclare(CVMArrayOfCharICell, valueICellPtr);
	CVMObjectICell *valueAsObject = (CVMObjectICell *)valueICellPtr;
	CVMInt32 offset, str_len;

	CVMID_fieldReadRef(ee, string, 
			   CVMoffsetOfjava_lang_String_value,
			   valueAsObject);
	CVMID_fieldReadInt(ee, string, 
			   CVMoffsetOfjava_lang_String_offset,
			   offset);
	CVMID_fieldReadInt(ee, string, 
			   CVMoffsetOfjava_lang_String_count,
			   str_len);

	if ((start >= 0) && (len >= 0) &&
	    ((CVMUint32)start + (CVMUint32)len <= (CVMUint32)str_len)) {
	    CVMD_gcUnsafeExec(ee, {
		CVMArrayOfChar* charArray =
		    (CVMArrayOfChar*)CVMID_icellDirect(ee, valueAsObject);
		CVMD_arrayReadBodyChar(buf, charArray, offset + start, len);
	    });
	} else {
	    CVMthrowStringIndexOutOfBoundsException(ee, NULL);
	}
    } CVMID_localrootEnd();
}

void JNICALL
CVMjniGetStringUTFRegion(JNIEnv *env,
			 jstring string, jsize start, jsize len, char *buf)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);

    CVMjniSanityCheckStringAccess(ee, string);

    CVMID_localrootBegin(ee); {
	CVMID_localrootDeclare(CVMArrayOfCharICell, valueICellPtr);
	CVMObjectICell *valueAsObject = (CVMObjectICell *)valueICellPtr;
	CVMJavaChar charBuffer[MAX_CHARS_PER_ACCESS];
	CVMInt32 offset, str_len, bufferLen;

	CVMID_fieldReadRef(ee, string, 
			   CVMoffsetOfjava_lang_String_value,
			   valueAsObject);
	CVMID_fieldReadInt(ee, string, 
			   CVMoffsetOfjava_lang_String_offset,
			   offset);
	CVMID_fieldReadInt(ee, string, 
			   CVMoffsetOfjava_lang_String_count,
			   str_len);

	if ((start >= 0) && (len >= 0) &&
	    ((CVMUint32)start + (CVMUint32)len <= (CVMUint32)str_len)) {
	    /*
	     * This is somewhat slow, but gives GC a chance to
	     * run for very long strings.
	     */
	    offset += start;
	    while ( len > 0 ){
		bufferLen = MIN( len, MAX_CHARS_PER_ACCESS );
		CVMD_gcUnsafeExec(ee,{
		    CVMArrayOfChar* theChars;

		    theChars = CVMID_icellDirect(ee, valueICellPtr);
		    CVMD_arrayReadBodyChar( charBuffer, theChars, offset, bufferLen );

		});
		buf = CVMutfCopyFromCharArray(charBuffer, buf, bufferLen);
		len -= bufferLen;
		offset += bufferLen;
	    }
	    *buf = '\0';
	} else {
	    CVMthrowStringIndexOutOfBoundsException(ee, NULL);
	}
    } CVMID_localrootEnd();
}

const char* JNICALL
CVMjniGetStringUTFChars(JNIEnv *env, jstring string, jboolean *isCopy)
{
    char *utf;
    jsize strLen = CVMjniGetStringLength(env, string);
    jsize utfLen = CVMjniGetStringUTFLength(env, string);

    utf = (char *)malloc(utfLen + 1);
    if (utf == NULL) {
        CVMthrowOutOfMemoryError(CVMjniEnv2ExecEnv(env), NULL);
	return NULL;
    }
    CVMjniGetStringUTFRegion(env, string, 0, strLen, utf);
    if (isCopy) {
        *isCopy = JNI_TRUE;
    }
    return (const char*)utf;
}

void JNICALL
CVMjniReleaseStringUTFChars(JNIEnv *env, jstring str, const char *chars)
{
    free((void *)chars);
}

const jchar* JNICALL
CVMjniGetStringChars(JNIEnv *env, jstring string, jboolean *isCopy)
{
    jchar *chars;
    jsize strLen = CVMjniGetStringLength(env, string);
    size_t allocLen = strLen;

    /* Fix for 5071855. Never do malloc(0) */
    if (allocLen == 0) {
	allocLen = 1;
    }

    chars = (jchar *)malloc(sizeof(jchar) * allocLen);
    if (chars == NULL) {
        CVMthrowOutOfMemoryError(CVMjniEnv2ExecEnv(env), NULL);
	return NULL;
    }
    CVMjniGetStringRegion(env, string, 0, strLen, chars);
    if (isCopy) {
        *isCopy = JNI_TRUE;
    }
    return (const jchar*)chars;
}

void JNICALL
CVMjniReleaseStringChars(JNIEnv *env, jstring str, const jchar *chars)
{
    free((void *)chars);
}

/*
 * Constructs a new java.lang.String object from an array of UTF-8 characters. 
 *
 * We could avoid the temporary buffer if we copied directly into the
 * char[], but that would require going through the indirect layer.
 */
jstring JNICALL
CVMjniNewStringUTF(JNIEnv* env, const char* utf8Bytes)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMStringICell* stringICell = CVMjniCreateLocalRef(ee);
    if (stringICell == NULL) {
	return NULL;
    }

    CVMnewStringUTF(ee, stringICell, utf8Bytes);
    if (CVMID_icellIsNull(stringICell)) {
	CVMjniDeleteLocalRef(env, stringICell);
	CVMthrowOutOfMemoryError(ee, NULL);
	return NULL;
    } else {
	return stringICell;
    }
}

jint JNICALL
CVMjniThrow(JNIEnv *env, jthrowable obj)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMBool     isThrowable;

    CVMD_gcUnsafeExec(ee, {
	isThrowable = CVMgcUnsafeIsInstanceOf(
             ee, CVMID_icellDirect(ee, obj), 
	     CVMsystemClass(java_lang_Throwable));
    });
    if (!isThrowable) {
	CVMjniFatalError(env, 
	    "throw an object that is not an instance of java/lang/Throwable");
    }

    CVMgcSafeThrowLocalException(CVMjniEnv2ExecEnv(env), obj);
    return JNI_OK;
}

jint JNICALL
CVMjniThrowNew(JNIEnv *env, jclass clazz, const char *message)
{
    jint result;
    jstring msg = NULL;
    jobject obj = NULL;
    jmethodID methodID;

    if (CVMjniEnsureLocalCapacity(env, 2) != JNI_OK) {
        return JNI_ENOMEM;
    }

    methodID = CVMjniGetMethodID(env, clazz,
				 "<init>", "(Ljava/lang/String;)V");

    if (methodID == NULL) {
	/* Exception already thrown */
	result = JNI_ERR;
	goto done;
    }

    if (message != NULL) {
	msg = CVMjniNewStringUTF(env, message);
	if (msg == NULL) {
	    result = JNI_ERR;
	    goto done;
	}
    } else {
	msg = NULL;
    }

    obj = CVMjniNewObject(env, clazz, methodID, msg);
    if (obj == NULL) {
        result = JNI_ERR;
	goto done;
    }

    result = CVMjniThrow(env, obj);

done:
    if (msg != NULL) {
	CVMjniDeleteLocalRef(env, msg);
    }
    if (obj != NULL) {
	CVMjniDeleteLocalRef(env, obj);
    }
    return result;
}

jthrowable JNICALL
CVMjniExceptionOccurred(JNIEnv *env)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMThrowableICell* exceptionICell = NULL;

    if (CVMlocalExceptionOccurred(ee)) {
	exceptionICell = CVMlocalExceptionICell(ee);
    } else if (CVMremoteExceptionOccurred(ee)) {
	exceptionICell = CVMremoteExceptionICell(ee);
    }

    if (exceptionICell != NULL) {
        /* turn into local ref */
        return CVMjniNewLocalRef(env, exceptionICell);
    } else {
	return NULL;
    }

}

jboolean JNICALL
CVMjniExceptionCheck(JNIEnv *env)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);

    if (CVMexceptionOccurred(ee)) {
	return JNI_TRUE;
    } else {
	return JNI_FALSE;
    }
}

void JNICALL
CVMjniExceptionClear(JNIEnv *env)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    /* %comment: rt017 */
    CVMclearLocalException(ee);
    CVMclearRemoteException(ee);
}

/* NOTE:  If we decide later to get rid of the prototypes just
   added to jni_impl.h, we will need to forward declare
   CVMjniCallVoidMethod here because CVMjniExceptionDescribe uses it
   before it's defined in this file. */

void JNICALL
CVMjniExceptionDescribe(JNIEnv *env)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jobject throwable = NULL;
    jclass clazz;
    jmethodID method;

    if ((!CVMexceptionOccurred(ee))) {
	return;
    }

    if (CVMjniEnsureLocalCapacity(env, 3) != JNI_OK) {
        return;
    }

    if (CVMlocalExceptionOccurred(ee)) {
	throwable =
	    (*env)->NewLocalRef(env, CVMlocalExceptionICell(ee));
    } else if (CVMremoteExceptionOccurred(ee)) {
	throwable =
	    (*env)->NewLocalRef(env, CVMremoteExceptionICell(ee));
    }
    if (throwable == NULL) {
	return;
    }
    
    CVMclearLocalException(ee);
    /* We might want to preserve the remote exception if we
       are recognizing the local exception, but for now we clear
       both for compatibility with JDK */
    CVMclearRemoteException(ee);

    clazz = CVMjniGetObjectClass(env, throwable);
    CVMassert(clazz != NULL);
    method = CVMjniGetMethodID(env, clazz, "printStackTrace", "()V");
    CVMjniDeleteLocalRef(env, clazz);
    CVMassert(method != NULL);
    CVMjniCallVoidMethod(env, throwable, method);

    /* If printStackTrace threw an exception, then print a message. */
    if (CVMlocalExceptionOccurred(ee)) {
	/* NOTE: this has happened in the past because of static
           initializers failing to get run, causing the system class
           not to be initialized and System.out being NULL, causing a
           NullPointerException while printing the stack trace. */
	CVMconsolePrintf("CVMjniExceptionDescribe failed: "
			 "couldn't print stack trace.\n");
	CVMconsolePrintf("Using brute force method to print stack trace.\n");
	CVMID_icellAssign(ee, CVMlocalExceptionICell(ee), throwable);
	CVMdumpException(ee); 
    }

    CVMjniDeleteLocalRef(env, throwable);
}

/*
 * Methods calls
 */

#undef SAFE_COUNT
#define SAFE_COUNT 32	/* number of arguments to push between GC checkpoint */ 

static char
CVMjniPushArgumentsVararg(JNIEnv *env,
			  CVMterseSigIterator *terse_signature,
			  CVMFrame *current_frame, const void *a)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMStackVal32 *topOfStack = current_frame->topOfStack;
    int safeCount = SAFE_COUNT;
    CVMVaList *v = (CVMVaList *)a;
    va_list *args = &v->args;
    int typeSyllable;

    while ((typeSyllable=CVM_TERSE_ITER_NEXT(*terse_signature))!=CVM_TYPEID_ENDFUNC) {
	/* Perform GC checkpoint every so often */
	if (safeCount-- == 0) {
	    CVMD_gcSafeCheckPoint(ee, {
		current_frame->topOfStack = topOfStack;
	    }, {});
	    safeCount = SAFE_COUNT;
	}

        switch (typeSyllable) {
	case CVM_TYPEID_BOOLEAN:
	case CVM_TYPEID_SHORT:
	case CVM_TYPEID_BYTE:
	case CVM_TYPEID_CHAR:
	case CVM_TYPEID_INT:
            (topOfStack++)->j.i = va_arg(*args, jint);
            continue;
	case CVM_TYPEID_FLOAT:
            (topOfStack++)->j.f = va_arg(*args, jdouble);
            continue;
        case CVM_TYPEID_OBJ: {
	    jobject obj = va_arg(*args, jobject);
	    CVMID_icellAssignDirect(ee, &topOfStack->j.r,
		CVMjniNonNullICellPtrFor(obj));
	    topOfStack++;
	    continue;
	}
	case CVM_TYPEID_LONG: {
	    jlong l = va_arg(*args, jlong);
	    CVMlong2Jvm(&topOfStack->j.raw, l);
            topOfStack += 2;
            continue;
	}
	case CVM_TYPEID_DOUBLE: {
	    jdouble d = va_arg(*args, jdouble);
	    CVMdouble2Jvm(&topOfStack->j.raw, d);
            topOfStack += 2;
            continue;
	}
	default:
	    /* This should never happen */
	    CVMassert(CVM_FALSE);
	    return 0;
        }
    }
    /* fell out of while loop. Return return type */
    current_frame->topOfStack = topOfStack;
    return CVM_TERSE_ITER_RETURNTYPE(*terse_signature);
}

static char
CVMjniPushArgumentsArray(JNIEnv *env,
			 CVMterseSigIterator *terse_signature,
			 CVMFrame *current_frame, const void *a)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMStackVal32 *topOfStack = current_frame->topOfStack;
    int safeCount = SAFE_COUNT;
    jvalue *args = (jvalue *)a;
    int typeSyllable;
    
    while ((typeSyllable=CVM_TERSE_ITER_NEXT(*terse_signature))!=CVM_TYPEID_ENDFUNC) {
	/* Perform GC checkpoint every so often */
	if (safeCount-- == 0) {
	    CVMD_gcSafeCheckPoint(ee, {
		current_frame->topOfStack = topOfStack;
	    }, {});
	    safeCount = SAFE_COUNT;
	}

        switch (typeSyllable) {
	case CVM_TYPEID_BOOLEAN:
            (topOfStack++)->j.i = (*args++).z;
            continue;
	case CVM_TYPEID_SHORT:
            (topOfStack++)->j.i = (*args++).s;
            continue;
	case CVM_TYPEID_BYTE:
            (topOfStack++)->j.i = (*args++).b;
            continue;
	case CVM_TYPEID_CHAR:
            (topOfStack++)->j.i = (*args++).c;
            continue;
	case CVM_TYPEID_INT:
            (topOfStack++)->j.i = (*args++).i;
            continue;
	case CVM_TYPEID_FLOAT:
	    (topOfStack++)->j.f = (*args++).f;
            continue;
	case CVM_TYPEID_OBJ: {
	    jobject obj = (*args++).l;
	    CVMID_icellAssignDirect(ee, &topOfStack->j.r,
		CVMjniNonNullICellPtrFor(obj));
	    topOfStack++;
	    continue;
	}
	case CVM_TYPEID_LONG: {
	    jlong l = (*args++).j;
	    CVMlong2Jvm(&topOfStack->j.raw, l);
            topOfStack += 2;
            continue;
	}
	case CVM_TYPEID_DOUBLE: {
	    jdouble d = (*args++).d;
	    CVMdouble2Jvm(&topOfStack->j.raw, d);
            topOfStack += 2;
            continue;
	}
	default:
            CVMassert(CVM_FALSE);
	    return 0;
        }
    }
    /* fell out of loop. return return type */
    current_frame->topOfStack = topOfStack;
    return CVM_TERSE_ITER_RETURNTYPE(*terse_signature);
}

static void
CVMjniInvoke(JNIEnv *env, jobject obj, jmethodID methodID,
    JNI_PushArguments_t pushArguments, const void *args,
    CVMUint16 info, jvalue *retValue)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock *cb;
    CVMFrame*     frame;
    CVMInt32 invokeType = (info & CVM_INVOKE_TYPE_MASK);
    char retType = (info & CVM_INVOKE_RETTYPE_MASK);
    CVMBool isStatic = (invokeType == CVM_INVOKE_STATIC);
    CVMBool isVirtual = (invokeType == CVM_INVOKE_VIRTUAL);
    
    CVMassert(CVMframeIsFreelist(CVMeeGetCurrentFrame(ee)));

    /* %comment hideya003 */
    if (CVMlocalExceptionOccurred(ee)) {
#ifdef CVM_DEBUG
	CVMconsolePrintf("Pending exception entering CVMjniInvoke!\n");
	CVMjniExceptionDescribe(env);
#endif
#ifdef CVM_DEBUG_ASSERTS
        CVMassertHook(__FILE__, __LINE__,
                      "Pending exception entering CVMjniInvoke!\n");
#endif
	return;
    }

    if (retType == CVM_TYPEID_OBJ) {
	/* Caller should have ensured capacity */
	retValue->l = CVMjniCreateLocalRef(ee);
	/* We should never reach here if the ref creation fails */
	CVMassert(retValue->l != NULL);
    }

    /* Create the transition frame.*/
    CVMD_gcUnsafeExec(ee, {
	if (isStatic) {
	    cb = CVMjniGcUnsafeRef2Class(ee, obj);
	} else {
	    cb = CVMobjectGetClass(CVMID_icellDirect(ee, obj));
	}
	frame = (CVMFrame*)CVMpushTransitionFrame(ee, methodID);
    });
    if (frame == NULL) {
	if (retType == CVM_TYPEID_OBJ) {
	    CVMjniDeleteLocalRef(env, retValue->l);
	    retValue->l = NULL;
	}
	CVMassert(CVMexceptionOccurred(ee));
	return;  /* exception already thrown */
    }

    CVMassert(!CVMlocalExceptionOccurred(ee));

    /* Push arguments and invoke the method. */
    CVMD_gcUnsafeExec(ee, {
	CVMBool resultIsNullObjectRef = CVM_FALSE;
	/* Push the "this" argument if the method is not static. */
	if (!isStatic) {
	    CVMID_icellAssignDirect(ee, &frame->topOfStack->j.r, obj);
	    frame->topOfStack++;
	}

	/* Push the all the other arguments. */
	{
	    CVMterseSigIterator terseSig;
	    char       	methodRetType;
	    CVMMethodBlock *mb = methodID;

	    CVMtypeidGetTerseSignatureIterator(CVMmbNameAndTypeID(mb), 
					       &terseSig);

	    methodRetType = (*pushArguments)(env, &terseSig, frame, args);
	    if (methodRetType != retType) {
		CVMjniFatalError(env, "Native code expects wrong return type"
				 "from Java callback");
	    }
	}

	CVMassert(!CVMlocalExceptionOccurred(ee));

	/* Call the java method. */
	(*CVMglobals.CVMgcUnsafeExecuteJavaMethodProcPtr)(ee,
							  methodID,
							  isStatic,
							  isVirtual);

	/* 
	 * Copy the result. frame->topOfStack will point after the result.
	 */
	if (!CVMlocalExceptionOccurred(ee)) {
	    switch (retType) {
	    case CVM_TYPEID_VOID:
		break;
	    case CVM_TYPEID_BOOLEAN:
		retValue->z = frame->topOfStack[-1].j.i;
		break;
	    case CVM_TYPEID_BYTE:
		retValue->b = frame->topOfStack[-1].j.i;
		break;
	    case CVM_TYPEID_CHAR:
		retValue->c = frame->topOfStack[-1].j.i;
		break;
	    case CVM_TYPEID_SHORT:
		retValue->s = frame->topOfStack[-1].j.i;
		break;
	    case CVM_TYPEID_INT:
		retValue->i = frame->topOfStack[-1].j.i;
		break;
	    case CVM_TYPEID_LONG: {
		retValue->j = CVMjvm2Long(&frame->topOfStack[-2].j.raw);
		break;
	    }
	    case CVM_TYPEID_FLOAT:
		retValue->f = (jfloat)frame->topOfStack[-1].j.f;
		break;
	    case CVM_TYPEID_DOUBLE: {
		retValue->d = CVMjvm2Double(&frame->topOfStack[-2].j.raw);
		break;
	    }
	    case CVM_TYPEID_OBJ:
		CVMID_icellAssignDirect(ee, retValue->l,
					&frame->topOfStack[-1].j.r);
		break;
	    }
	}

	/* Regardless of whether an exception occurred, we must not
	   allow an ICell containing NULL to escape to the caller. */
	if ((retType == CVM_TYPEID_OBJ) &&
	    CVMID_icellIsNull(retValue->l)) {
	    resultIsNullObjectRef = CVM_TRUE;
	}

	/* Pop the transition frame. */
	CVMpopFrame(&ee->interpreterStack,
		    ee->interpreterStack.currentFrame);
	/* Do not delete the local ref until the transition frame is
           popped! */
	if (resultIsNullObjectRef) {
	    CVMjniDeleteLocalRef(env, retValue->l);
	    retValue->l = NULL;
	}
    });
}

#undef CONCAT
#undef EVALCONCAT
#undef CONCAT3
#define CONCAT(x,y)		x##y
#define EVALCONCAT(x,y)		C(x,y)
#define CONCAT3(a,b,c)		a##b##c
#undef N_NONVIRTUAL
#undef N_VIRTUAL
#undef N_STATIC
#define N_NONVIRTUAL	CVMjniCallNonvirtual
#define N_VIRTUAL	CVMjniCall
#define N_STATIC	CVMjniCallStatic
#undef SIG_NONVIRTUAL
#undef SIG_VIRTUAL
#undef SIG_STATIC
#define SIG_NONVIRTUAL	jobject obj, jclass clazz
#define SIG_VIRTUAL	jobject obj
#define SIG_STATIC	jclass clazz
#undef OBJ_NONVIRTUAL
#undef OBJ_VIRTUAL
#undef OBJ_STATIC
#define OBJ_NONVIRTUAL	obj
#define OBJ_VIRTUAL	obj
#define OBJ_STATIC	clazz
#undef IF_Yes
#undef IF_No
#undef IF1_Yes
#undef IF1_No
#undef IF0_Yes
#undef IF0_No
#undef IF
#define IF_Yes(x,y)	x
#define IF_No(x,y)	y
#define IF1_Yes(x)	x
#define IF1_No(x)
#define IF0_No(x)	x
#define IF0_Yes(x)
#define IF(YN,x,y)	CONCAT(IF_,YN)(x,y)
#define IFY(YN,x)	CONCAT(IF1_,YN)(x)
#define IFN(YN,x)	CONCAT(IF0_,YN)(x)
#undef CALL_NAME_DOTDOTDOT
#undef CALL_NAME_VA_LIST
#undef CALL_NAME_JVALUE_ARRAY
#define CALL_NAME_DOTDOTDOT	Method
#define CALL_NAME_VA_LIST	MethodV
#define CALL_NAME_JVALUE_ARRAY	MethodA
#undef ARGS_DOTDOTDOT
#undef ARGS_VA_LIST
#undef ARGS_JVALUE_ARRAY
#define ARGS_DOTDOTDOT		&v
#define ARGS_VA_LIST		&v
#define ARGS_JVALUE_ARRAY	args
#undef PUSHARGSFUNC_DOTDOTDOT
#undef PUSHARGSFUNC_VA_LIST
#undef PUSHARGSFUNC_JVALUE_ARRAY
#define PUSHARGSFUNC_DOTDOTDOT		CVMjniPushArgumentsVararg
#define PUSHARGSFUNC_VA_LIST		CVMjniPushArgumentsVararg
#define PUSHARGSFUNC_JVALUE_ARRAY	CVMjniPushArgumentsArray
#undef ARG_DECL_DOTDOTDOT
#undef ARG_DECL_VA_LIST
#undef ARG_DECL_JVALUE_ARRAY
#define ARG_DECL_DOTDOTDOT	...
#define ARG_DECL_VA_LIST	va_list args
#define ARG_DECL_JVALUE_ARRAY	const jvalue *args
#undef IS_DOTDOTDOT_DOTDOTDOT
#undef IS_DOTDOTDOT_VA_LIST
#undef IS_DOTDOTDOT_JVALUE_ARRAY
#define IS_DOTDOTDOT_DOTDOTDOT		Yes
#define IS_DOTDOTDOT_VA_LIST		No
#define IS_DOTDOTDOT_JVALUE_ARRAY	No
#undef IS_JVALUE_ARRAY_DOTDOTDOT
#undef IS_JVALUE_ARRAY_VA_LIST
#undef IS_JVALUE_ARRAY_JVALUE_ARRAY
#define IS_JVALUE_ARRAY_DOTDOTDOT	No
#define IS_JVALUE_ARRAY_VA_LIST		No
#define IS_JVALUE_ARRAY_JVALUE_ARRAY	Yes
#undef IS_VA_LIST_DOTDOTDOT
#undef IS_VA_LIST_VA_LIST
#undef IS_VA_LIST_JVALUE_ARRAY
#define IS_VA_LIST_DOTDOTDOT	No
#define IS_VA_LIST_VA_LIST	Yes
#define IS_VA_LIST_JVALUE_ARRAY	No

#define NAME(a,b,c)	CONCAT3(a,b,c)

#undef DEFINE_CALLMETHODXY
#define DEFINE_CALLMETHODXY(MethodType,					    \
			    ResultType, Result, unionType,		    \
			    ReturnCode,					    \
			    isValue, argType)				    \
									    \
ResultType JNICALL							    \
NAME(N_##MethodType,Result,CALL_NAME_##argType)(JNIEnv *env,	    	    \
			 SIG_##MethodType,				    \
			 jmethodID methodID, ARG_DECL_##argType)	    \
{									    \
    IFY(isValue,jvalue result;)						    \
    IFN(IS_JVALUE_ARRAY_##argType,CVMVaList v;)				    \
									    \
    IFY(IS_DOTDOTDOT_##argType,va_start(v.args, methodID));		    \
    IFY(IS_VA_LIST_##argType,va_copy(v.args, args));			    \
									    \
    CVMjniInvoke(env, OBJ_##MethodType, methodID,			    \
		 PUSHARGSFUNC_##argType,				    \
		 ARGS_##argType,					    \
		 ReturnCode | CVM_INVOKE_##MethodType,			    \
		 IF(isValue,&result,0));				    \
    IFN(IS_JVALUE_ARRAY_##argType,va_end(v.args));			    \
    IF(isValue,return result.unionType,return);				    \
}									    \

#undef DEFINE_CALLMETHODX
#define DEFINE_CALLMETHODX(MT, RT, R, UT, RC, VD)			    \
DEFINE_CALLMETHODXY(MT,RT,R,UT,RC,VD,DOTDOTDOT)	/* ... */		    \
DEFINE_CALLMETHODXY(MT,RT,R,UT,RC,VD,VA_LIST) 	/* va_list args */	    \
DEFINE_CALLMETHODXY(MT,RT,R,UT,RC,VD,JVALUE_ARRAY)	/* jvalue *args */

#undef DEFINE_CALLMETHOD
#define DEFINE_CALLMETHOD(RT, R, UT, RC, VD)				    \
DEFINE_CALLMETHODX(NONVIRTUAL,RT,R,UT,RC,VD)	/* Nonvirtual */	    \
DEFINE_CALLMETHODX(VIRTUAL,RT,R,UT,RC,VD)	/* Virtual */		    \
DEFINE_CALLMETHODX(STATIC,RT,R,UT,RC,VD)	/* Static */

DEFINE_CALLMETHOD(jobject,Object,l,CVM_TYPEID_OBJ,Yes)		/* Object */
DEFINE_CALLMETHOD(jboolean,Boolean,z,CVM_TYPEID_BOOLEAN,Yes)	/* Boolean */
DEFINE_CALLMETHOD(jbyte,Byte,b,CVM_TYPEID_BYTE,Yes)		/* Byte */
DEFINE_CALLMETHOD(jchar,Char,c,CVM_TYPEID_CHAR,Yes)		/* Char */
DEFINE_CALLMETHOD(jshort,Short,s,CVM_TYPEID_SHORT,Yes)		/* Short */
DEFINE_CALLMETHOD(jint,Int,i,CVM_TYPEID_INT,Yes)			/* Int */
DEFINE_CALLMETHOD(jlong,Long,j,CVM_TYPEID_LONG,Yes)		/* Long */
DEFINE_CALLMETHOD(jfloat,Float,f,CVM_TYPEID_FLOAT,Yes)		/* Float */
DEFINE_CALLMETHOD(jdouble,Double,d,CVM_TYPEID_DOUBLE,Yes)	/* Double */
DEFINE_CALLMETHOD(void,Void,X,CVM_TYPEID_VOID,No)		/* Void */

jint JNICALL
CVMjniMonitorEnter(JNIEnv *env, jobject obj)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMBool ok = CVMgcSafeObjectLock(ee, obj);
    if (!ok) {
        CVMthrowOutOfMemoryError(ee, "Couldn't allocate memory for monitor");
    }
    return ok ? JNI_OK : JNI_ERR;
}

jint JNICALL
CVMjniMonitorExit(JNIEnv *env, jobject obj)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMBool ok = CVMgcSafeObjectUnlock(ee, obj);
    if (!ok) {
        CVMthrowIllegalMonitorStateException(ee, "current thread not owner");
    }
    return ok ? JNI_OK : JNI_ERR;
}

jint JNICALL
CVMjniGetJavaVM(JNIEnv *env, JavaVM **p_jvm)
{
    *p_jvm = &CVMglobals.javaVM.vector;
    return JNI_OK;
}


/* NOTE: These lookups are done with the NULL (bootstrap) ClassLoader to 
 *  circumvent any security checks that would be done by jni_FindClass.
 */
static jboolean
CVMjniLookupDirectBufferClasses(JNIEnv* env)
{
    CVMJNIJavaVM *vm = &CVMglobals.javaVM;
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock *cb;

    /* Note: For the class lookups below, use the NULL classloader, and NULL
       protection domain. 

       Note that the JNI spec does not say that any exceptions will be thrown
       for NewDirectByteBuffer(), GetDirectBufferAddress(), and
       GetDirectBufferCapacity(), except for NewDirectByteBuffer() which can
       throw an OutOfMemoryError.  Hence, we also suppress
       CVMclassLookupByNameFromClassLoader from throwing exceptions.

       Note: In the following, the class lookup may fail.  When that happens,
       we may have already allocated some of the global refs.  Normally, we
       would clean up any allocations due to failed initialization.  But in
       this case, the global refs will be cleaned up automatically anyway when
       the VM shuts down.  The only cost is that we can waste up to 2 global
       refs.  However, the amount of memory wasted for this (2 words) is less
       than the amount of code it will take to clean it up.  So, we'll live
       with the potential waste of 2 global refs in the event of an
       initialization failure.
    */
    cb = CVMclassLookupByNameFromClassLoader(ee, "java/nio/Buffer",
					     CVM_TRUE, NULL, NULL, CVM_FALSE);
    if (cb == NULL) {
	return CVM_FALSE;
    }
    vm->bufferClass = (jclass) CVMjniNewGlobalRef(env, CVMcbJavaInstance(cb));

    cb = CVMclassLookupByNameFromClassLoader(ee, "sun/nio/ch/DirectBuffer",
					     CVM_TRUE, NULL, NULL, CVM_FALSE);
    if (cb == NULL) {
        CVMjniExceptionClear(env);
        /* Try again with the JSR-239 implementation class */
        cb = CVMclassLookupByNameFromClassLoader(ee, "java/nio/DirectBuffer",
                                                 CVM_TRUE, NULL, NULL, CVM_FALSE);
    }
    if (cb == NULL) {
	return CVM_FALSE;
    }
    vm->directBufferClass =
	(jclass) CVMjniNewGlobalRef(env, CVMcbJavaInstance(cb));

    cb = CVMclassLookupByNameFromClassLoader(ee, "java/nio/DirectByteBuffer",
					     CVM_TRUE, NULL, NULL, CVM_FALSE);
    if (cb == NULL) {
	return CVM_FALSE;
    }
    vm->directByteBufferClass =
	(jclass) CVMjniNewGlobalRef(env, CVMcbJavaInstance(cb));

    return CVM_TRUE;
}


static jboolean 
CVMjniInitializeDirectBufferSupport(JNIEnv* env)
{
    CVMJNIJavaVM *vm = &CVMglobals.javaVM;
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);

    CVMassert(CVMD_isgcSafe(ee));

    /* If we've already attempted initialization and failed, there's no need
       to retry it: */
    if (vm->directBufferSupportInitializeFailed) {
	return CVM_FALSE;
    }

    /* Note that lookupDirectBufferClasses() does lookups on the NULL
       classloader.  This means that it can lock the nullClassLoaderLock.
       Hence, we cannot use a sysMutex of higher rank than the
       nullClassLoaderLock to synchronize this initialization.  Hence,
       the nullClassLoaderLock is used.
    */
    CVMsysMutexLock(ee, &CVMglobals.nullClassLoaderLock);

    /* If another thread beat us to initializing this, then there's nothing
       to do: */
    if (vm->directBufferSupportInitialized) {
	goto done;
    }

    /* Check for presence of needed classes: */
    if (!CVMjniLookupDirectBufferClasses(env)) {
	vm->directBufferSupportInitializeFailed = CVM_TRUE;
	goto done;
    }

    /*  Get needed field and method IDs */
    vm->directByteBufferLongConstructor =
	CVMjniGetMethodID(env, vm->directByteBufferClass, "<init>", "(JI)V");
    if (vm->directByteBufferLongConstructor == NULL) {
        CVMjniExceptionClear(env);
        /* JSR-239 code path */
        vm->directByteBufferIntConstructor =
            CVMjniGetMethodID(env, vm->directByteBufferClass, "<init>", "(I[BI)V");
    }
    vm->directBufferAddressLongField =
	CVMjniGetFieldID(env, vm->bufferClass, "address", "J");
    if (vm->directBufferAddressLongField == NULL) {
        CVMjniExceptionClear(env);
        /* JSR-239 code path */
        vm->directBufferAddressIntField =
            CVMjniGetFieldID(env, vm->bufferClass, "arrayOffset", "I");
    }
    vm->bufferCapacityField =
	CVMjniGetFieldID(env, vm->bufferClass, "capacity", "I");

    if (((vm->directByteBufferLongConstructor == NULL) &&
         (vm->directByteBufferIntConstructor  == NULL))  ||
	((vm->directBufferAddressLongField  == NULL) &&
         (vm->directBufferAddressIntField   == NULL))    ||
	(vm->bufferCapacityField == NULL)) {
	vm->directBufferSupportInitializeFailed = CVM_TRUE;
	goto done;
    }

    vm->directBufferSupportInitialized = CVM_TRUE;

done:
    CVMsysMutexUnlock(ee, &CVMglobals.nullClassLoaderLock);
    return !vm->directBufferSupportInitializeFailed;
}

jobject JNICALL
CVMjniNewDirectByteBuffer(JNIEnv *env, void* address, jlong capacity)
{
    CVMJNIJavaVM *vm = &CVMglobals.javaVM;
    jlong addr;
    jint  cap;
    jobject buffer = NULL;

    /* Initialize the direct buffer support if not inited yet: */
    if (!vm->directBufferSupportInitialized) {
	if (!CVMjniInitializeDirectBufferSupport(env)) {
	    return NULL;
	}
    }

    addr = CVMvoidPtr2Long(address);

    /*  
     * NOTE that package-private DirectByteBuffer constructor currently 
     *  takes int capacity.
     */
    cap  = CVMlong2Int(capacity);
    if (vm->directByteBufferLongConstructor != NULL) {
        buffer = CVMjniNewObject(env, vm->directByteBufferClass,
                                 vm->directByteBufferLongConstructor, addr, cap);
    } else if (vm->directByteBufferIntConstructor != NULL) {
        /* JSR-239 variant */
        buffer = CVMjniNewObject(env, vm->directByteBufferClass,
                                 vm->directByteBufferIntConstructor, cap, NULL, CVMlong2Int(addr));
    }
    return buffer;
}


void* JNICALL
CVMjniGetDirectBufferAddress(JNIEnv *env, jobject buf)
{
    CVMJNIJavaVM *vm = &CVMglobals.javaVM;
    jlong lresult = 0;

    /* Initialize the direct buffer support if not inited yet: */
    if (!vm->directBufferSupportInitialized) {
	if (!CVMjniInitializeDirectBufferSupport(env)) {
	    return NULL;
	}
    }

    if ((buf != NULL) &&
	(!CVMjniIsInstanceOf(env, buf, vm->directBufferClass))) {
	return NULL;
    }

    if (vm->directBufferAddressLongField != NULL) {
        lresult = CVMjniGetLongField(env, buf, vm->directBufferAddressLongField);
    } else if (vm->directBufferAddressIntField != NULL) {
        lresult = CVMint2Long(CVMjniGetIntField(env, buf, vm->directBufferAddressIntField));
    }
    return CVMlong2VoidPtr(lresult);
}

jlong JNICALL
CVMjniGetDirectBufferCapacity(JNIEnv *env, jobject buf)
{
    CVMJNIJavaVM *vm = &CVMglobals.javaVM;
    jint capacity;
    jlong failedResult = CVMint2Long(-1);

    /* Initialize the direct buffer support if not inited yet: */
    if (!vm->directBufferSupportInitialized) {
	if (!CVMjniInitializeDirectBufferSupport(env)) {
	    /* NOTE: The JavaSE library implementation is expecting a failure
	       to initialize condition to return 0 here instead of -1.  Hence,
	       we return the same value to be consistent. 

	       The ideal solution is probably to return -1 if the failure is
	       due to needed classes not beig available, and a 0 if due to
	       a low memory condition.  The low memory condition can allow
	       a retry to init later.
	    */
	    return CVMlongConstZero();
	}
    }

    if (buf == NULL) {
	return failedResult;
    }

    if (!CVMjniIsInstanceOf(env, buf, vm->directBufferClass)) {
	return failedResult;
    }

    /*  NOTE that capacity is currently an int in the implementation */
    capacity = CVMjniGetIntField(env, buf, vm->bufferCapacityField);
    return CVMint2Long(capacity);
}


/* NOTE: This can not be made const any more, because the JVMTI
   will instrument it if necessary, and the "checked" version of the
   JNI may do so also if we add that later. */

static
#ifndef CVM_JVMTI
const
#endif
struct JNINativeInterface CVMmainJNIfuncs =
{
    NULL,
    NULL,
    NULL,

    NULL,

    CVMjniGetVersion,

    CVMjniDefineClass,
    CVMjniFindClass,

    CVMjniFromReflectedMethod,
    CVMjniFromReflectedField,

    CVMjniToReflectedMethod,

    CVMjniGetSuperclass,
    CVMjniIsAssignableFrom,

    CVMjniToReflectedField,

    CVMjniThrow,
    CVMjniThrowNew,
    CVMjniExceptionOccurred,
    CVMjniExceptionDescribe,
    CVMjniExceptionClear,
    CVMjniFatalError,

    CVMjniPushLocalFrame,
    CVMjniPopLocalFrame,

    CVMjniNewGlobalRef,
    CVMjniDeleteGlobalRef,
    CVMjniDeleteLocalRef,
    CVMjniIsSameObject,

    CVMjniNewLocalRef,
    CVMjniEnsureLocalCapacity,

    CVMjniAllocObject,
    CVMjniNewObject,
    CVMjniNewObjectV,
    CVMjniNewObjectA,

    CVMjniGetObjectClass,
    CVMjniIsInstanceOf,

    CVMjniGetMethodID,

    CVMjniCallObjectMethod,
    CVMjniCallObjectMethodV,
    CVMjniCallObjectMethodA,
    CVMjniCallBooleanMethod,
    CVMjniCallBooleanMethodV,
    CVMjniCallBooleanMethodA,
    CVMjniCallByteMethod,
    CVMjniCallByteMethodV,
    CVMjniCallByteMethodA,
    CVMjniCallCharMethod,
    CVMjniCallCharMethodV,
    CVMjniCallCharMethodA,
    CVMjniCallShortMethod,
    CVMjniCallShortMethodV,
    CVMjniCallShortMethodA,
    CVMjniCallIntMethod,
    CVMjniCallIntMethodV,
    CVMjniCallIntMethodA,
    CVMjniCallLongMethod,
    CVMjniCallLongMethodV,
    CVMjniCallLongMethodA,
    CVMjniCallFloatMethod,
    CVMjniCallFloatMethodV,
    CVMjniCallFloatMethodA,
    CVMjniCallDoubleMethod,
    CVMjniCallDoubleMethodV,
    CVMjniCallDoubleMethodA,
    CVMjniCallVoidMethod,
    CVMjniCallVoidMethodV,
    CVMjniCallVoidMethodA,

    CVMjniCallNonvirtualObjectMethod,
    CVMjniCallNonvirtualObjectMethodV,
    CVMjniCallNonvirtualObjectMethodA,
    CVMjniCallNonvirtualBooleanMethod,
    CVMjniCallNonvirtualBooleanMethodV,
    CVMjniCallNonvirtualBooleanMethodA,
    CVMjniCallNonvirtualByteMethod,
    CVMjniCallNonvirtualByteMethodV,
    CVMjniCallNonvirtualByteMethodA,
    CVMjniCallNonvirtualCharMethod,
    CVMjniCallNonvirtualCharMethodV,
    CVMjniCallNonvirtualCharMethodA,
    CVMjniCallNonvirtualShortMethod,
    CVMjniCallNonvirtualShortMethodV,
    CVMjniCallNonvirtualShortMethodA,
    CVMjniCallNonvirtualIntMethod,
    CVMjniCallNonvirtualIntMethodV,
    CVMjniCallNonvirtualIntMethodA,
    CVMjniCallNonvirtualLongMethod,
    CVMjniCallNonvirtualLongMethodV,
    CVMjniCallNonvirtualLongMethodA,
    CVMjniCallNonvirtualFloatMethod,
    CVMjniCallNonvirtualFloatMethodV,
    CVMjniCallNonvirtualFloatMethodA,
    CVMjniCallNonvirtualDoubleMethod,
    CVMjniCallNonvirtualDoubleMethodV,
    CVMjniCallNonvirtualDoubleMethodA,
    CVMjniCallNonvirtualVoidMethod,
    CVMjniCallNonvirtualVoidMethodV,
    CVMjniCallNonvirtualVoidMethodA,

    CVMjniGetFieldID,

    CVMjniGetObjectField,
    CVMjniGetBooleanField,
    CVMjniGetByteField,
    CVMjniGetCharField,
    CVMjniGetShortField,
    CVMjniGetIntField,
    CVMjniGetLongField,
    CVMjniGetFloatField,
    CVMjniGetDoubleField,

    CVMjniSetObjectField,
    CVMjniSetBooleanField,
    CVMjniSetByteField,
    CVMjniSetCharField,
    CVMjniSetShortField,
    CVMjniSetIntField,
    CVMjniSetLongField,
    CVMjniSetFloatField,
    CVMjniSetDoubleField,

    CVMjniGetStaticMethodID,

    CVMjniCallStaticObjectMethod,
    CVMjniCallStaticObjectMethodV,
    CVMjniCallStaticObjectMethodA,
    CVMjniCallStaticBooleanMethod,
    CVMjniCallStaticBooleanMethodV,
    CVMjniCallStaticBooleanMethodA,
    CVMjniCallStaticByteMethod,
    CVMjniCallStaticByteMethodV,
    CVMjniCallStaticByteMethodA,
    CVMjniCallStaticCharMethod,
    CVMjniCallStaticCharMethodV,
    CVMjniCallStaticCharMethodA,
    CVMjniCallStaticShortMethod,
    CVMjniCallStaticShortMethodV,
    CVMjniCallStaticShortMethodA,
    CVMjniCallStaticIntMethod,
    CVMjniCallStaticIntMethodV,
    CVMjniCallStaticIntMethodA,
    CVMjniCallStaticLongMethod,
    CVMjniCallStaticLongMethodV,
    CVMjniCallStaticLongMethodA,
    CVMjniCallStaticFloatMethod,
    CVMjniCallStaticFloatMethodV,
    CVMjniCallStaticFloatMethodA,
    CVMjniCallStaticDoubleMethod,
    CVMjniCallStaticDoubleMethodV,
    CVMjniCallStaticDoubleMethodA,
    CVMjniCallStaticVoidMethod,
    CVMjniCallStaticVoidMethodV,
    CVMjniCallStaticVoidMethodA,

    CVMjniGetStaticFieldID,

    CVMjniGetStaticObjectField,
    CVMjniGetStaticBooleanField,
    CVMjniGetStaticByteField,
    CVMjniGetStaticCharField,
    CVMjniGetStaticShortField,
    CVMjniGetStaticIntField,
    CVMjniGetStaticLongField,
    CVMjniGetStaticFloatField,
    CVMjniGetStaticDoubleField,

    CVMjniSetStaticObjectField,
    CVMjniSetStaticBooleanField,
    CVMjniSetStaticByteField,
    CVMjniSetStaticCharField,
    CVMjniSetStaticShortField,
    CVMjniSetStaticIntField,
    CVMjniSetStaticLongField,
    CVMjniSetStaticFloatField,
    CVMjniSetStaticDoubleField,

    CVMjniNewString,
    CVMjniGetStringLength,
    CVMjniGetStringChars,
    CVMjniReleaseStringChars,

    CVMjniNewStringUTF,
    CVMjniGetStringUTFLength,
    CVMjniGetStringUTFChars,
    CVMjniReleaseStringUTFChars,

    CVMjniGetArrayLength,
 
    CVMjniNewObjectArray,
    CVMjniGetObjectArrayElement,
    CVMjniSetObjectArrayElement,

    CVMjniNewBooleanArray,
    CVMjniNewByteArray,
    CVMjniNewCharArray,
    CVMjniNewShortArray,
    CVMjniNewIntArray,
    CVMjniNewLongArray,
    CVMjniNewFloatArray,
    CVMjniNewDoubleArray,

    CVMjniGetBooleanArrayElements,
    CVMjniGetByteArrayElements,
    CVMjniGetCharArrayElements,
    CVMjniGetShortArrayElements,
    CVMjniGetIntArrayElements,
    CVMjniGetLongArrayElements,
    CVMjniGetFloatArrayElements,
    CVMjniGetDoubleArrayElements,

    CVMjniReleaseBooleanArrayElements,
    CVMjniReleaseByteArrayElements,
    CVMjniReleaseCharArrayElements,
    CVMjniReleaseShortArrayElements,
    CVMjniReleaseIntArrayElements,
    CVMjniReleaseLongArrayElements,
    CVMjniReleaseFloatArrayElements,
    CVMjniReleaseDoubleArrayElements,

    CVMjniGetBooleanArrayRegion,
    CVMjniGetByteArrayRegion,
    CVMjniGetCharArrayRegion,
    CVMjniGetShortArrayRegion,
    CVMjniGetIntArrayRegion,
    CVMjniGetLongArrayRegion,
    CVMjniGetFloatArrayRegion,
    CVMjniGetDoubleArrayRegion,

    CVMjniSetBooleanArrayRegion,
    CVMjniSetByteArrayRegion,
    CVMjniSetCharArrayRegion,
    CVMjniSetShortArrayRegion,
    CVMjniSetIntArrayRegion,
    CVMjniSetLongArrayRegion,
    CVMjniSetFloatArrayRegion,
    CVMjniSetDoubleArrayRegion,

    CVMjniRegisterNatives,
    CVMjniUnregisterNatives,

    CVMjniMonitorEnter,
    CVMjniMonitorExit,

    CVMjniGetJavaVM,

    CVMjniGetStringRegion,
    CVMjniGetStringUTFRegion,

    CVMjniGetPrimitiveArrayCritical,
    CVMjniReleasePrimitiveArrayCritical,

    CVMjniGetStringChars,     /* CVMjniGetStringCritical */
    CVMjniReleaseStringChars, /* CVMjniReleaseStringCritical */

    CVMjniNewWeakGlobalRef,
    CVMjniDeleteWeakGlobalRef,

    CVMjniExceptionCheck,

    /* JNI_VERSION_1_4 additions: */
    CVMjniNewDirectByteBuffer,
    CVMjniGetDirectBufferAddress,
    CVMjniGetDirectBufferCapacity
};


JNIEXPORT jint JNICALL
JNI_GetDefaultJavaVMInitArgs(void *args_)
{
    return JNI_ERR;
}

static int numJVMs;

/* 
 * The classes to be initialized.
 */
const CVMClassBlock * classInitList[] = {
    CVMsystemClass(java_lang_Class),
    CVMsystemClass(java_lang_Float),
    CVMsystemClass(java_lang_String),
    CVMsystemClass(sun_misc_ThreadRegistry),
    CVMsystemClass(java_lang_Shutdown),
    CVMsystemClass(java_lang_ClassLoader),
    CVMsystemClass(java_lang_ClassLoader_NativeLibrary),
    CVMsystemClass(java_lang_Thread),
};

static const int classInitListLen = sizeof classInitList / sizeof classInitList[0];

static CVMBool initializeClassList(CVMExecEnv* ee,
				   char* errorStrBuf, 
				   CVMInt32 sizeofErrorStrBuf,
				   const CVMClassBlock** classList,
				   const int numClasses)
{
    int i;
    
    for (i = 0; i < numClasses; i++) {
	CVMClassBlock* cb = (CVMClassBlock*)classList[i];
	CVMBool status;
	/*
	CVMconsolePrintf("Preparing to initialize %C\n",
			 classList[i]);
	*/
	status = CVMclassInit(ee, cb);
	if (!status) {
	    CVMformatString(errorStrBuf, sizeofErrorStrBuf,
			    "class initialization failed for %C", cb);
	    return CVM_FALSE;
	}
    }
    return CVM_TRUE;
}

static char*
initializeSystemClass(JNIEnv* env)
{
    {
	/* Call System.initializeSystemClass() via the JNI. This sets
	   up the global properties table which
	   CVM.parseCommandLineOptions(), below, will modify with
	   user-defined property definitions. This was at one point
	   called from a method called CVM.initialize() via
	   reflection, but we have to be able to configure without 
	   reflection. */
	jclass systemClass;
	jmethodID initMethodID;

	systemClass = CVMcbJavaInstance(CVMsystemClass(java_lang_System));
	initMethodID = (*env)->GetStaticMethodID(env, systemClass,
						 "initializeSystemClass",
						 "()V");
	if (initMethodID == NULL) {
	    return "can't find method "
		"\"void java.lang.System.initializeSystemClass()\"";
	}
	(*env)->CallStaticVoidMethod(env, systemClass, initMethodID);
	
	if ((*env)->ExceptionCheck(env)) {
	    (*env)->ExceptionDescribe(env);
	    return "error during System.initializeSystemClass())\n";
	}
    }
    return NULL;
}

static char*
initializeThreadObjects(JNIEnv* env)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    
    /*
     * Create system ThreadGroup, main ThreadGroup, and main Thread object.
     * These are all done in Thread.initMainThread().
     */
    jclass threadClass =
	CVMcbJavaInstance(CVMsystemClass(java_lang_Thread));
    jlong eetop = CVMvoidPtr2Long(ee);

    CVMjniCallStaticVoidMethod(
	    env, threadClass,
	    CVMglobals.java_lang_Thread_initMainThread,
	    java_lang_Thread_NORM_PRIORITY, eetop);

    /* 
     * An exception may occur during the main thread initialization phase. 
     */ 
    if (CVMexceptionOccurred(ee)) {
	return "initialization of main thread failed";
    }
    return NULL; /* success */
}

#ifdef CVM_CLASSLOADING
#define CVM_CLASSLOADING_OPTIONS \
    "[-Xbootclasspath[/a | /p]:<path>] [-Xverify[:{all | remote | none}] "
#else
#define CVM_CLASSLOADING_OPTIONS
#endif

#ifdef CVM_SPLIT_VERIFY
#define CVM_SPLIT_VERIFY_OPTIONS "[-XsplitVerify={true|false}] "
#else
#define CVM_SPLIT_VERIFY_OPTIONS
#endif

#ifdef CVM_DEBUG
#define CVM_DEBUG_OPTIONS "[-Xtrace:<val>] "
#else
#define CVM_DEBUG_OPTIONS
#endif

#ifdef CVM_JVMTI
#define CVM_JVMTI_OPTIONS "[-agentlib | -agentpath] "
    /* XRUN_OPTIONS is setup below */
#else
#define CVM_JVMTI_OPTIONS
#endif

#ifdef CVM_JVMPI
#define CVM_JVMPI_OPTIONS "[-Xrunhprof] [-Xrunjcov] "
/* XRUN_OPTIONS is setup below */
#else
#define CVM_JVMPI_OPTIONS
#endif

#ifdef CVM_XRUN
#define CVM_XRUN_OPTIONS "[-Xrun<lib>[:<options>]] "
#else
#define CVM_XRUN_OPTIONS
#endif

#ifndef CVM_JIT
#define CVM_JIT_OPTIONS
#endif

#ifdef CVM_MTASK
#define CVM_MTASK_OPTIONS "[-Xserver[:<portNumber>]] "
#else
#define CVM_MTASK_OPTIONS
#endif

#ifndef CDC_10
#define CVM_JAVA_ASSERTION_OPTIONS \
        "-ea[:<packagename>...|:<classname>] " \
        "-enableassertions[:<packagename>...|:<classname>] " \
        "-da[:<packagename>...|:<classname>] " \
        "-disableassertions[:<packagename>...|:<classname>] " \
        "-esa -enablesystemassertions -dsa -disablesystemassertions "
#else
#define CVM_JAVA_ASSERTION_OPTIONS
#endif

#define CVM_NATIVE_OPTIONS \
        CVM_MTASK_OPTIONS \
	CVM_JAVA_ASSERTION_OPTIONS \
	CVM_CLASSLOADING_OPTIONS \
	CVM_SPLIT_VERIFY_OPTIONS \
	CVM_JVMTI_OPTIONS \
	CVM_JVMPI_OPTIONS \
	CVM_XRUN_OPTIONS \
	"[-XfullShutdown] " \
	"[-XhangOnStartup] " \
	"[-XunlimitedGCRoots] " \
	"[-XtimeStamping] " \
	CVM_GC_OPTIONS \
	"[-Xms<size>] " \
	"[-Xmn<size>] " \
	"[-Xmx<size>] " \
	"[-Xss<size>] " \
	"[-Xopt:[<option>[,<option>]...]] " \
	CVM_JIT_OPTIONS \
	CVM_DEBUG_OPTIONS

/* Allocate an array of Strings and pass it as argument to
   CVM.parseCommandLineOptions(), which will add any user-defined
   properties to the global properties table and optionally find
   the main class name and arguments.
   
   %comment: rt024
   
   NOTE we skip any options that were recognized in the first phase
   of initialization
*/
char*
CVMparseCommandLineOptions(JNIEnv* env, JavaVMInitArgs* initArgs, 
			   CVMInt32 numUnrecognizedOptions)
{
    jarray jInitArgs;
    CVMInt32 i, index;
    jclass cvmClass;
    jmethodID initMethodID;
    jint result;
    jstring nativeOptions = NULL;
    
    jInitArgs = (*env)->NewObjectArray(
            env, numUnrecognizedOptions,
	    CVMcbJavaInstance(CVMsystemClass(java_lang_String)),
	    NULL);
    if (jInitArgs == NULL) {
	return "out of memory while allocating jInitArgs";
    }
    index = 0;
    for (i = 0; i < initArgs->nOptions; i++) {
	/* Unrecognized options have optionString != NULL. */
	if (initArgs->options[i].optionString != NULL) {
	    jobject jstr =
		(*env)->NewStringUTF(env,
				     initArgs->options[i].optionString);
	    if (jstr == NULL) {
		return "out of memory while allocating jInitArgs";
	    }
	    (*env)->SetObjectArrayElement(env, jInitArgs, index, jstr);
	    ++index;
	    (*env)->DeleteLocalRef(env, jstr);
	}
    }
    CVMassert(index == numUnrecognizedOptions);
    
    nativeOptions = (*env)->NewStringUTF(env, CVM_NATIVE_OPTIONS);
    if (nativeOptions == NULL) {
	return "out of memory while allocating nativeOptions";
    }
    
    cvmClass = CVMcbJavaInstance(CVMsystemClass(sun_misc_CVM));
    initMethodID = (*env)->GetStaticMethodID(
            env, cvmClass,
	    "parseCommandLineOptions",
	    "([Ljava/lang/String;Ljava/lang/String;Z)I");
    CVMassert(initMethodID != NULL);
    result = (*env)->CallStaticIntMethod(env, cvmClass, initMethodID,
					     jInitArgs, nativeOptions,
					     initArgs->ignoreUnrecognized);
    if ((*env)->ExceptionCheck(env)) {
	/*
	 * If an exception occurs, the result from the JNI call
	 * cannot be trusted. Make it an error.  
	 */
	(*env)->ExceptionDescribe(env);
	return "exception during sun.misc.CVM.parseCommandLineOptions()";
    }
    if (result == sun_misc_CVM_ARG_PARSE_ERR ||
	result == sun_misc_CVM_ARG_PARSE_USAGE) {
	    CVMoptPrintUsage();
#ifdef CVM_JIT
	    CVMjitPrintUsage();
#endif
    }
    switch(result) {
    case sun_misc_CVM_ARG_PARSE_ERR:
	return ""; /* Usage already printed out */
    case sun_misc_CVM_ARG_PARSE_USAGE:
    case sun_misc_CVM_ARG_PARSE_EXITVM:
#ifdef CVM_CLASSLOADING
	/* to make sure we can exit */
	if (!CVMclassClassPathInit(env)) { 
	    return "Failed to initialize app classpath";
	}
#endif /* CVM_CLASSLOADING */
	break;
    default:
	CVMassert(result == sun_misc_CVM_ARG_PARSE_OK);
    }
    
    (*env)->DeleteLocalRef(env, nativeOptions);
    (*env)->DeleteLocalRef(env, jInitArgs);
    return NULL;
}

#ifdef CVM_MTASK
void
CVMmtaskServerCommSocket(JNIEnv* env, CVMInt32 commSocket)
{
    CVMglobals.commFd = commSocket;
}

static void
mtaskJvmtiInit(JNIEnv* env)
{
#ifdef CVM_JVMTI
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    if (CVMjvmtiIsEnabled()) {
	CVMjvmtiPostVmInitEvent(ee);    

	/*
	 * The debugger event hook might have thrown an exception
	 */
	if ((*env)->ExceptionCheck(env)) {
	    (*env)->ExceptionDescribe(env);
	    fprintf(stderr, "Error during JVMTI_EVENT_VM_INIT handling");
	    exit(1);
	} else {
	    CVMjvmtiDebugEventsEnabled(ee) = CVM_TRUE;
	}
	
	if (CVMjvmtiShouldPostThreadLife()) {
	    CVMjvmtiPostThreadStartEvent(ee, CVMcurrentThreadICell(ee));
	}
	/*
	 * The debugger event hook might have thrown an exception
	 */
	if ((*env)->ExceptionCheck(env)) {
	    (*env)->ExceptionDescribe(env);
	    fprintf(stderr, "Error during JVMTI_EVENT_THREAD_START handling");
	    exit(1);
	}
    }
#endif
}

static void
mtaskJvmpiInit(JNIEnv* env)
{
#ifdef CVM_JVMPI
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);

    CVMjvmpiPostStartUpEvents(ee);

    if ((*env)->ExceptionCheck(env)) {
	(*env)->ExceptionDescribe(env);
	fprintf(stderr, "Error during JVMPI VM startup events handling");
	exit(1);
    }

#ifdef CVM_JVMPI_PROFILER
    /*
     * If we are including JVMPI support for a specific profiler, then
     * do the profiler specific initialization.
     */
    CVMJVMPIprofilerInit(ee);
    if ((*env)->ExceptionCheck(env)) {
	(*env)->ExceptionDescribe(env);
	fprintf(stderr, "Error during JVMPI profiler init");
	exit(1);
    }
#endif
#endif
}

void
CVMmtaskReinitializeChildVM(JNIEnv* env, CVMInt32 clientId)
{
    CVMglobals.clientId = clientId;

    if (clientId != 0) {
#ifdef CVM_JIT
        /*
         * Set CVMglobals.jit.isPrecompiling to false to allow
         * patch enabled code being generated in child process.
         */
        CVMglobals.jit.isPrecompiling = CVM_FALSE;
#endif

        mtaskJvmtiInit(env);

        mtaskJvmpiInit(env);
    }
}

void
CVMmtaskServerPort(JNIEnv* env, CVMInt32 serverPort)
{
    CVMglobals.serverPort = serverPort;
}



#ifdef CVM_TIMESTAMPING
jboolean
CVMmtaskTimeStampReinitialize(JNIEnv* env) 
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);

    if (!CVMtimeStampFinishup(ee)) {
	return JNI_FALSE;
    }
    CVMtimeStampWallClkInit();
    if (!CVMtimeStampStart(ee)) {
	return JNI_FALSE;
    }

    return JNI_TRUE;
}

jboolean
CVMmtaskTimeStampRecord(JNIEnv* env, const char* loc, int pos) 
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);

    return CVMtimeStampRecord(ee, loc, pos);
}    
#endif

#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)
extern void
CVMmtaskHandleSuspendChecker()
{
    if (CVMglobals.suspendCheckerInitialized) {
	CVMconsolePrintf("Suspend checker shutting down\n");
	CVMsuspendCheckerDestroy();	    
	CVMglobals.suspendCheckerInitialized = CVM_FALSE;
    }
}
#endif
#endif

/*
 * Returns error string on failure, NULL on success
 */
static char*
initializeSystemClasses(JNIEnv* env,
			char* errorStrBuf,
			CVMInt32 sizeofErrorStrBuf,
			JavaVMInitArgs* initArgs, 
			CVMInt32 numUnrecognizedOptions)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    char* errorStr;

#ifdef CVM_CLASSLOADING
    /* init bootclasspath before doing System.initializeSystemClass()*/
    if (!CVMclassBootClassPathInit(env)) {
	return "Failed to initialize bootclasspath";
    }
#endif

    /* First ensure that we have no clinit for sun_misc_CVM before proceeding
       with class initialization of system classes: */
    if (CVMclassGetMethodBlock(CVMsystemClass(sun_misc_CVM),
			       CVMglobals.clinitTid, CVM_TRUE) != NULL) {
	CVMconsolePrintf("WARNING: sun.misc.CVM has a static initializer!\n");
    }

    /* 
     * Set initialization flag for sun_misc_CVM to prevent executing
     * its clinit. This is a work around for bug 4645152 in JDK 1.4
     * javac, which incorrectly inserts <clinit> in class files when
     * -g is specified. When 4645152 is fixed in javac, we can replace
     * the work around with an assert:
     * CVMassert(CVMclassGetMethodBlock(CVMsystemClass(sun_misc_CVM),
     *     CVMglobals.clinitTid, CVM_TRUE) == NULL);
     */
    CVMcbSetROMClassInitializationFlag(ee, CVMsystemClass(sun_misc_CVM));
    
    /* Prepare for thread creation */
    if ((errorStr = initializeThreadObjects(env)) != NULL) {
	return errorStr;
    }

    /* Initilize some core classes first. */
    if (!initializeClassList(ee, errorStrBuf, sizeofErrorStrBuf,
			     classInitList, classInitListLen)) {
	errorStr = errorStrBuf;
	return errorStr;
    }
    
#ifdef CVM_LVM /* %begin lvm */
    /* Now we can finish up CVMglobals.lvm initialization.
     * This creates the main (primordial) LogicalVM object. */
    if (!CVMLVMglobalsInitPhase2(ee, &CVMglobals.lvm)) {
	errorStr = "out of memory during LVM initialization";
        return errorStr;
    }
#endif /* %end lvm */

    /* Initialize java.lang.System */
    if ((errorStr = initializeSystemClass(env)) != NULL) {
	return errorStr;
    }
    
    /* Parse the command line options that are passed in 
       (mostly done in sun.misc.CVM) */
    errorStr = CVMparseCommandLineOptions(env, initArgs, 
					  numUnrecognizedOptions);
    if (errorStr != NULL) {
	return errorStr;
    }
    
    /* Now that we've parsed command line options, we can set up the
       app class path as well. */
#ifdef CVM_CLASSLOADING
    if (!CVMclassClassPathInit(env)) { 
	return "Failed to initialize app classpath";
    }
#endif /* CVM_CLASSLOADING */

#ifdef CVM_MTASK
    if (CVMglobals.isServer) {
	/* Any server-only functions go in here */
    }
#endif
    
    return NULL;
}

/* Purpose: Prints a message about the erroneous use of the specified memory
            sizing option. */
static void printMemorySizeSpecificationError(const char *option)
{
    CVMconsolePrintf("Illegal %s option: no size specified\n"
		     "\tUsage: %s<size> e.g. %s1m\n"
		     "\tNote: Do not insert spaces between %s and the size.\n",
		     option, option, option, option);
}

JNIEXPORT jint JNICALL
JNI_CreateJavaVM(JavaVM **p_jvm, void **p_env, void *args)
{
    CVMExecEnv *ee = NULL;
    JNIEnv *env;
    int argCtr;
    const CVMProperties *sprops;
    CVMOptions options;
    JavaVMInitArgs* initArgs = (JavaVMInitArgs*) args;
    CVMInt32 numUnrecognizedOptions = 0;

    char *errorStr = NULL;
    char errorStrBuf[256];
    int  errorNo = JNI_OK;
#ifdef CVM_AGENTLIB
    CVMInt32 numAgentlibArguments = 0;
#endif
#ifdef CVM_XRUN
    CVMInt32 numXrunArguments = 0;
    char **xrunArguments = NULL;
#endif
#ifdef CVM_CLASSLOADING
    char *xbootclasspath = NULL;
#endif
    CVMBool userHomePropSpecified = CVM_FALSE; /* -Duser.home specified */
    CVMBool userNamePropSpecified = CVM_FALSE; /* -Duser.name specified */
    CVMpathInfo pathInfo;
    const char *sunlibrarypathStr = NULL;

    /*
     * If the current system time is set to be before January 1, 1970,
     * a NullPointerException would be thrown when running the static
     * initializer of java.lang.System during the main thread
     * initialization phase later. So we just check the time here.
     */
    if (CVMtimeMillis() <= 0) {
        errorStr = "current system time before January 1, 1970 ";
        errorNo = JNI_ERR;
        goto done; 
    }

    if (initArgs == NULL) {
	errorStr = "arguments passed via Invocation API were NULL";
	errorNo = JNI_EINVAL;
	goto done;
    }

    memset(&options, 0, sizeof options);
    memset(&pathInfo, 0, sizeof pathInfo);

#ifndef CDC_10
    /* assertions are off by default */
    options.javaAssertionsUserDefault = CVM_FALSE;
    options.javaAssertionsSysDefault = CVM_FALSE;
#endif

    /* %comment: rt021 */
    if (numJVMs == 0) {
	if (!CVMinitStaticState(&pathInfo)) {
	    errorStr = "CVMinitStaticState failed";
	    errorNo = JNI_ENOMEM;
	    goto done;
	}
    }

    /*
     * Get the default platform dependent value for bootclasspath,
     * etc.
     */ 
    sprops = CVMgetProperties();

#ifdef CVM_CLASSLOADING
#ifndef NO_JDK_COMPATABILITY
    /*
     * The default value of java.class.path
     * If NO_JDK_COMPATABILITY is not set, the default value of
     * our class path is ., unless one is explicitly indicated.
     *
     */
    options.appclasspathStr = ".";
#endif

    /*
     * The default verification mode is CVM_VERIFY_REMOTE
     */
    options.classVerificationLevel = CVM_VERIFY_REMOTE;

#ifdef CVM_SPLIT_VERIFY
    options.splitVerify = CVM_TRUE;
#endif
#endif

    for (argCtr = 0; argCtr < initArgs->nOptions; argCtr++) {

	const char *str = initArgs->options[argCtr].optionString;
	if (!strncmp(str, "-Xms", 4)) {
	    options.startHeapSizeStr = str + 4;
	    if (options.startHeapSizeStr[0] == '\0') {
		printMemorySizeSpecificationError("-Xms");
		errorNo = JNI_EINVAL;
		goto done;
	    }
	} else if (!strncmp(str, "-Xmn", 4)) {
	    options.minHeapSizeStr = str + 4;
	    if (options.minHeapSizeStr[0] == '\0') {
		printMemorySizeSpecificationError("-Xmn");
		errorNo = JNI_EINVAL;
		goto done;
	    }
	} else if (!strncmp(str, "-Xmx", 4)) {
	    options.maxHeapSizeStr = str + 4;
	    if (options.maxHeapSizeStr[0] == '\0') {
		printMemorySizeSpecificationError("-Xmx");
		errorNo = JNI_EINVAL;
		goto done;
	    }
	} else if (!strncmp(str, "-Xss", 4)) {
	    options.nativeStackSizeStr = str + 4;
	    if (options.nativeStackSizeStr[0] == '\0') {
		printMemorySizeSpecificationError("-Xss");
		errorNo = JNI_EINVAL;
		goto done;
	    }
#ifdef CVM_MTASK
	} else if (!strncmp(str, "-Xserver", 8)) {
	    options.isServer = CVM_TRUE;
#endif
	} else if (!strncmp(str, "-Xopt:", 6)) {
	    if (options.optAttributesStr != NULL) {
		CVMconsolePrintf("Previous -Xopt:%s ignored.\n",
				 options.optAttributesStr);
	    }
	    options.optAttributesStr = str + 6;
	} else if (!strncmp(str, "-Xgc:", 5)) {
	    if (options.gcAttributesStr != NULL) {
		CVMconsolePrintf("Previous -Xgc:%s ignored.\n",
				 options.gcAttributesStr);
	    }
	    options.gcAttributesStr = str + 5;
#ifdef CVM_JIT
	} else if (!strncmp(str, "-Xjit:", 6)) {
	    if (options.jitAttributesStr != NULL) {
		CVMconsolePrintf("Previous -Xjit:%s ignored.\n",
				 options.jitAttributesStr);
	    }
	    options.jitAttributesStr = str + 6;
#endif
	}
#ifdef CVM_CLASSLOADING
	else if (!strcmp(str, "-Xverify")) {
	    options.classVerificationLevel = CVM_VERIFY_ALL;
	} else if (!strncmp(str, "-Xverify:", 9)) {
	    char* kind = (char*)str + 9;
	    int verification = CVMclassVerificationSpecToEncoding(kind);
	    if (verification != CVM_VERIFY_UNRECOGNIZED) {
		options.classVerificationLevel = verification;
	    } else {
		CVMconsolePrintf("Illegal -Xverify option: \"%s\"\n",
				 kind);
		errorNo = JNI_EINVAL;
		goto done;
	    }	    
#ifdef CVM_SPLIT_VERIFY
	} else if (!strncmp(str, "-XsplitVerify=", 14)) {
	    char* value = (char*)str + 14;
            if (!strcmp(value, "true")) {
                options.splitVerify = CVM_TRUE;
            } else if (!strcmp(value, "false")) {
                options.splitVerify = CVM_FALSE;
	    } else {
		CVMconsolePrintf("Illegal -XsplitVerify option: \"%s\"\n",
				 value);
		errorNo = JNI_EINVAL;
		goto done;
	    }	    
#endif
	}
        else if (!strncmp(str, "-Xbootclasspath=", 16) ||
                 !strncmp(str, "-Xbootclasspath:", 16)) {
            options.bootclasspathStr = str + 16;
            /* user has set bootclasspath, it overrides any
             * -Xbootclasspath/a or /p
             */
            if (pathInfo.preBootclasspath != NULL) {
                free(pathInfo.preBootclasspath);
                pathInfo.preBootclasspath = NULL;
            }
            if (pathInfo.postBootclasspath != NULL) {
                free(pathInfo.postBootclasspath);
                pathInfo.postBootclasspath = NULL;
            }
        }
        else if (!strncmp(str, "-Xbootclasspath/a=", 18) ||
                 !strncmp(str, "-Xbootclasspath/a:", 18)) {
            const char* p = str + 18;
            char* tmp = pathInfo.postBootclasspath;
            pathInfo.postBootclasspath = (char *)
                malloc(strlen(p) +
                       (tmp == NULL ? 0 : strlen(tmp)) +
                       strlen(CVM_PATH_CLASSPATH_SEPARATOR) + 1);
            if (pathInfo.postBootclasspath == NULL) {
                errorStr = "out of memory while parsing -Xbootclasspath";
		errorNo = JNI_ENOMEM;
		goto done;
            }
            if (tmp != NULL) {
                sprintf(pathInfo.postBootclasspath, "%s%s%s", 
                        tmp, CVM_PATH_CLASSPATH_SEPARATOR, p);
                free(tmp);
            } else {
                pathInfo.postBootclasspath = strdup(p);
            }
        }
        else if (!strncmp(str, "-Xbootclasspath/p=", 18) ||
                 !strncmp(str, "-Xbootclasspath/p:", 18)) {
            const char* p = str + 18;
            char* tmp = pathInfo.preBootclasspath;
            pathInfo.preBootclasspath = (char *)
                malloc(strlen(p) +
                       (tmp == NULL ?
                        0 : strlen(tmp)) +
                       strlen(CVM_PATH_CLASSPATH_SEPARATOR) + 1);
            if (pathInfo.preBootclasspath == NULL) {
                errorStr = "out of memory while parsing -Xbootclasspath";
                errorNo = JNI_ENOMEM;
                goto done;
            }
            if (tmp != NULL) {
                sprintf(pathInfo.preBootclasspath, "%s%s%s", 
                        p, CVM_PATH_CLASSPATH_SEPARATOR, tmp);
                free(tmp);
            } else {
                pathInfo.preBootclasspath = strdup(p);
            }
        }
#ifdef NO_JDK_COMPATABILITY
        else if (!strncmp(str, "-Dsun.boot.class.path=", 22)) { 
            if (xbootclasspath != NULL) {
                free(xbootclasspath);
            }
            options.bootclasspathStr = str + 22;
        }
#endif
        else if (!strncmp(str, "-Djava.class.path=", 18)) {
            options.appclasspathStr = str + 18;
	    /* This class path value will be extracted and put into
	       the system property java.class.path by
	       JVM_InitProperties(). So we don't pass it into
	       CVM.parseCommandLineOptions(). */
        }
#endif                   
        else if (!strncmp(str, "-Duser.home=", 12)) {
           /* CR 6246485 We merely want to record that this has
              been set, but not consume it, so that it can be passed
              to CVM.java */
            userHomePropSpecified = CVM_TRUE;
            ++numUnrecognizedOptions;
            continue;
         }
         else if (!strncmp(str, "-Duser.name=", 12)) {
           /* CR 6246485 We merely want to record that this has
              been set, but not consume it, so that it can be passed
              to CVM.java */
            userNamePropSpecified = CVM_TRUE;
            ++numUnrecognizedOptions;
            continue;
         }
        else if (!strncmp(str, "-Xcp=", 5)) {
            options.appclasspathStr = str + 5;

            /* pass this to CVM.java still, so we can free up the memory 
               later on */
            ++numUnrecognizedOptions;
            continue;
        }

        else if (!strncmp(str, "-Xjar=", 6)) {
          options.appclasspathStr = str + 6;
          /* Note: The code below is so that the -jar value can be passed
             up to CVM.parseCommandOptions(). Please do not remove. */                
          ++numUnrecognizedOptions;
	  continue;
        } 

#ifndef CDC_10
	/* java assertion handling */
	else if (!strncmp(str, "-ea:", 4)) {
	    CVMBool success = CVMJavaAssertions_addOption(
		str + 4, CVM_TRUE,
		&options.javaAssertionsClasses,
		&options.javaAssertionsPackages);
	    if (!success) goto addOption_failed;
	} else if (!strncmp(str, "-enableassertions:", 18)) {
	    CVMBool success = CVMJavaAssertions_addOption(
                str + 18, CVM_TRUE,
		&options.javaAssertionsClasses,
		&options.javaAssertionsPackages);
	    if (!success) goto addOption_failed;
	} else if (!strncmp(str, "-da:", 4)) {
	    CVMBool success = CVMJavaAssertions_addOption(
                str + 4, CVM_FALSE,
		&options.javaAssertionsClasses,
		&options.javaAssertionsPackages);
	    if (!success) goto addOption_failed;
	} else if (!strncmp(str, "-disableassertions:", 18)) {
	    CVMBool success;
	    success = CVMJavaAssertions_addOption(
                str + 19, CVM_FALSE,
		&options.javaAssertionsClasses,
		&options.javaAssertionsPackages);
            if (!success) {
	addOption_failed:
                errorStr = "out of memory while parsing assertion option";
		errorNo = JNI_ENOMEM;
		goto done;
            }
	} else if (!strcmp(str, "-ea") |
		   !strcmp(str, "-enableassertions")) {
	    options.javaAssertionsUserDefault = CVM_TRUE;
	} else if (!strcmp(str, "-da") |
		   !strcmp(str, "-disableassertions")) {
	    options.javaAssertionsUserDefault = CVM_FALSE;
	} else if (!strcmp(str, "-esa") |
		   !strcmp(str, "-enablesystemassertions")) {
	    options.javaAssertionsSysDefault = CVM_TRUE;
	} else if (!strcmp(str, "-dsa") |
		   !strcmp(str, "-disablesystemassertions")) {
	    options.javaAssertionsSysDefault = CVM_FALSE;
	}
#endif

	/* NOTE that -Xdebug must currently be handled before
           CVMpreloaderGenerateAllStackmaps() is called */
	else if (!strcmp(str, "-Xdebug")) {
#ifdef CVM_JVMTI
	    options.debugging = CVM_TRUE;
#else
	    errorStr =
		"-Xdebug specified, but debugging support not compiled in";
	    errorNo = JNI_EINVAL;
	    goto done;
#endif
#ifdef CVM_TRACE_ENABLED
	} else if (!strncmp(str, "-Xtrace:", 8)) {
	    options.traceFlagsStr = str + 8;
#endif
	} else if (!strcmp(str, "vfprintf")) {
	    options.vfprintfHook = initArgs->options[argCtr].extraInfo;
	} else if (!strcmp(str, "exit")) {
	    options.exitHook = initArgs->options[argCtr].extraInfo;
	} else if (!strcmp(str, "abort")) {
	    options.abortHook = initArgs->options[argCtr].extraInfo;
	} else if (!strcmp(str, "_safeExit")) {
	    options.safeExitHook = initArgs->options[argCtr].extraInfo;
	} else if (!strcmp(str, "-XtimeStamping")) {
#ifdef CVM_TIMESTAMPING
	    options.timeStampEnabled = CVM_TRUE;
#else
	    errorStr =
		"-XtimeStamping specified, but timestamping support not compiled in";
	    errorNo = JNI_EINVAL;
	    goto done;
#endif
	} else if (!strcmp(str, "-XfullShutdown")) {
#ifdef CVM_HAVE_PROCESS_MODEL
	    options.fullShutdownFlag = CVM_TRUE;
#endif
	} else if (!strcmp(str, "-XhangOnStartup")) {
	    options.hangOnStartup = CVM_TRUE;
	} else if (!strcmp(str, "-XunlimitedGCRoots")) {
            options.unlimitedGCRoots = CVM_TRUE;
#ifdef CVM_AGENTLIB
	} else if (!strncmp(str, "-agentlib:", 10) ||
               !strncmp(str, "-agentpath:", 11)) {
	    ++numAgentlibArguments;
	    if (numAgentlibArguments > 1) {
		/* currently only support one agent connection
		 * At some point in the future we will support multiple
		 * agent connections.
		 */
		CVMconsolePrintf("WARNING: Only one agent connection supported!\n");
	    }

	    /* Don't set the optionString to NULL, so we can take care
           of this on the next pass */
	    continue;
#endif
#ifdef CVM_XRUN
	} else if (!strncmp(str, "-Xrun", 5)) {
	    ++numXrunArguments;
	    /* Don't set the optionString to NULL, so we can take care
               of this on the next pass */
	    continue;
#endif
	} else if (!strncmp(str, "-Dsun.boot.library.path=", 24)) {
            sunlibrarypathStr = str + 24;
            ++numUnrecognizedOptions;
            continue;
	} else {
	    /* Unrecognized option, pass to Java */
	    ++numUnrecognizedOptions;
	    continue;
	}

	/* Recognized options fall through to here */
	initArgs->options[argCtr].optionString = NULL;
    }

#ifdef CVM_XRUN
    /* Now snag -Xrun arguments so we can run them after initializing
       the VM */
    if (numXrunArguments > 0) {
	int i = 0;
	xrunArguments = malloc(numXrunArguments * sizeof(char *));
	if (xrunArguments == NULL) {
	    errorStr = "out of memory parsing -Xrun arguments";
	    errorNo = JNI_ENOMEM;
	    goto done;
	}
	for (argCtr = 0; argCtr < initArgs->nOptions; argCtr++) {
	    const char *str = initArgs->options[argCtr].optionString;
	    if ((str != NULL) && (!strncmp(str, "-Xrun", 5))) {
		xrunArguments[i] = (char *) str;
		initArgs->options[argCtr].optionString = NULL;		
		++i;
	    }
	}
	CVMassert(i == numXrunArguments);
    }
#endif

    if (sunlibrarypathStr != NULL) {
        if (pathInfo.dllPath != NULL) {
            free(pathInfo.dllPath);
        }
        pathInfo.dllPath = strdup(sunlibrarypathStr);
    }

    {
        if (!CVMinitPathValues((void *)CVMgetProperties(), &pathInfo,
                               (char **)&options.bootclasspathStr)) {
            errorStr = "CVMinitPathValues failed";
            errorNo = JNI_ERR;
            goto done;
        }
    }

    if (!CVMinitVMGlobalState(&CVMglobals, &options)) {
	/* <tbd> - CVMinitVMGlobalState() should return the proper
	 * JNI error code and have always already printed the error message.
	 * Currently it does not, so we need to print out a message here.
	 * See RFE #4680415
	 */
        errorStr = "CVMinitVMGlobalState failed";
	errorNo = JNI_ERR;
	goto done;
    }

    ee = &CVMglobals.mainEE;
    CVMglobals.userHomePropSpecified = userHomePropSpecified;
    CVMglobals.userNamePropSpecified = userNamePropSpecified;

    *p_jvm = &CVMglobals.javaVM.vector;
    env = CVMexecEnv2JniEnv(ee);
    *p_env = (void *)env;

#ifdef CVM_HW
    CVMhwInit();
#endif

#ifdef CVM_JVMTI
    CVMjvmtiEnterOnloadPhase();
    CVMtimeThreadCpuClockInit(&ee->threadInfo);
#endif

#ifdef CVM_JVMTI_IOVEC
    CVMinitIOVector(&CVMioFuncs);
#endif
    /* Run agents */
    /* Agents run before VM is fully initialized */
#ifdef CVM_AGENTLIB
    /* Grab -agentlib arguments so we can run them */
    /*
     * NOTE: at this point we should go through the Xrun args
     * to see if any could be converted to agentlib type
     */
    if (numAgentlibArguments > 0) {
#ifdef CVM_DYNAMIC_LINKING
      CVMAgentlibArg_t agentlibArgument;
      /* Initialize unload list */
      if (!CVMAgentInitTable(&CVMglobals.agentTable,
                             numAgentlibArguments)) {
	    errorStr = "CVMAgentInitTable() failed";
	    errorNo = JNI_ENOMEM;
	    goto done;
      }
      for (argCtr = 0; argCtr < initArgs->nOptions; argCtr++) {
	    const char *str = initArgs->options[argCtr].optionString;
	    if ((str != NULL)) {
          agentlibArgument.is_absolute = CVM_FALSE;
          if (!strncmp(str, "-agentlib:", 10) ||
              !strncmp(str, "-agentpath:", 11)) {
            agentlibArgument.str = (char *) str;
            initArgs->options[argCtr].optionString = NULL;
            if (!strncmp(str, "-agentpath:", 11)) {
              agentlibArgument.is_absolute = CVM_TRUE;
            }
            if (!CVMAgentHandleArgument(&CVMglobals.agentTable, env,
                                        &agentlibArgument)) {
              CVMconsolePrintf("Cannot start VM "
                "(error handling -agentlib or -agentpath argument %s)\n",
                               agentlibArgument.str);
              errorNo = JNI_ERR;
              goto done;
            } else {
		/* loaded one agent, that's all we support right now */
		break;
	    }
          }
        }
      }
#ifdef CVM_JVMTI
      CVMjvmtiEnterPrimordialPhase();
#endif
#else
      errorStr = "-agentlib, -agentpath not supported - dynamic linking not built into VM";
      errorNo = JNI_EINVAL;
      goto done;
#endif
    }
#endif

#ifdef CVM_INSTRUCTION_COUNTING
    /*
     * Initialize byte-code statistics gathering
     */
    CVMinitStats();
#endif /* CVM_INSTRUCTION_COUNTING */

    /* record time-stamp at the VM beginning */
#ifdef CVM_TIMESTAMPING
    if (!CVMtimeStampStart(ee)) {
        errorStr = "CVMtimeStampStart failed";
	errorNo = JNI_ERR;
	goto done;
    }
#endif

    if (!CVMpreloaderDisambiguateAllMethods(ee)) {
	errorStr = "method disambiguation failed";
	errorNo = JNI_ERR;
	goto done;
    }

    /* Initialize system classes, classpaths, and command line options */
    if ((errorStr = initializeSystemClasses(env, 
					    errorStrBuf,
					    sizeof(errorStrBuf),
					    initArgs, 
					    numUnrecognizedOptions)) != NULL) {
	errorNo = JNI_ERR;
	goto done;
    }

#ifdef CVM_XRUN
    /* Now that the VM is initialized, may need to handle "helper"
       libraries for JVMTI/PI */
    if (xrunArguments != NULL) {
#ifdef CVM_DYNAMIC_LINKING
        /* Initialize Xrun list */
        if (!CVMXrunInitTable(&CVMglobals.onUnloadTable, numXrunArguments)) {
	    errorStr = "CVMXrunInitTable() failed";
	    errorNo = JNI_ENOMEM;
	    goto done;
	}
 	for (argCtr = 0; argCtr < numXrunArguments; argCtr++) {
	    if (!CVMXrunHandleArgument(&CVMglobals.onUnloadTable, env,
				       xrunArguments[argCtr])) {
		CVMconsolePrintf("Cannot start VM "
				 "(error handling -Xrun argument %s)\n",
				 xrunArguments[argCtr]);
		if ((*env)->ExceptionCheck(env)) {
		    (*env)->ExceptionDescribe(env);
		}
		errorNo = JNI_ERR;
		goto done;
	    }
	}
#else
	errorStr = "-Xrun not supported - dynamic linking not built into VM";
	errorNo = JNI_EINVAL;
	goto done;
#endif
    }
#endif /* CVM_XRUN */

    if (CVMglobals.hangOnStartup != 0) {
	CVMconsolePrintf("Hanging on startup\n");
	while (CVMglobals.hangOnStartup != 0);
    }

#if defined(CVM_AOT) && !defined(CVM_MTASK)
#ifdef CVM_JVMTI
    if (!CVMjvmtiIsInDebugMode())
#endif
    {
        if (!CVMjitCompileAOTCode(ee)) {
	    errorStr = "error during AOT compilation";
            errorNo = JNI_ERR;
            goto done;
        }
    }
#endif
    
#ifdef CVM_LVM /* %begin lvm */
    /* Finish-up the main LVM bootstrapping after the VM gets 
     * fully initialized */
    if (!CVMLVMfinishUpBootstrapping(ee)) {
	errorStr = "error during LVM initialization";
	errorNo = JNI_ERR;
	goto done;
    }
#endif /* %end lvm */

#ifdef CVM_JVMTI
    CVMjvmtiEnterPrimordialPhase();
    if (CVMjvmtiIsEnabled()) {
	/* This thread was not fully initialized */
	jthread thread = CVMcurrentThreadICell(ee);
	if (CVMjvmtiFindThread(ee, thread) == NULL) {
	    CVMjvmtiInsertThread(ee, thread);
	}
    }
    CVMjvmtiDebugEventsEnabled(ee) = CVM_TRUE;
    CVMjvmtiEnterStartPhase();
    CVMjvmtiPostVmStartEvent(ee);    
    CVMjvmtiEnterLivePhase();
    CVMjvmtiPostVmInitEvent(ee);

    /*
     * The debugger event hook might have thrown an exception
     */
    if ((*env)->ExceptionCheck(env)) {
	(*env)->ExceptionDescribe(env);
	errorStr = "error during JVMTI_EVENT_VM_INIT handling";
	errorNo = JNI_ERR;
	goto done;
    }
    if (CVMjvmtiIsEnabled()) {
	CVMjvmtiPostStartUpEvents(ee);
    }
    /*
     * The debugger event hook might have thrown an exception
     */
    if ((*env)->ExceptionCheck(env)) {
	CVMjvmtiDebugEventsEnabled(ee) = CVM_FALSE;
	(*env)->ExceptionDescribe(env);
	errorStr = "error during JVMTI_EVENT_THREAD_START handling";
	errorNo = JNI_ERR;
	goto done;
    }
#endif

#ifdef CVM_JVMPI
    CVMjvmpiPostStartUpEvents(ee);
    if ((*env)->ExceptionCheck(env)) {
        errorStr = "error during JVMPI VM startup events handling";
        errorNo = JNI_ERR;
        goto done;
    }

    /*
     * If we are including JVMPI support for a specific profiler, then
     * do the profiler specific initialization.
     */
#ifdef CVM_JVMPI_PROFILER
    CVMJVMPIprofilerInit(ee);
    if ((*env)->ExceptionCheck(env)) {
        errorStr = "error during JVMPI profiler init";
        errorNo = JNI_ERR;
        goto done;
    }
#endif

#endif /* CVM_JVMPI */


    /* In the future JVMPI will need to get a VM init event here as
       well */

#ifdef CVM_JVMTI

#endif

#ifdef CVM_EMBEDDED_HOOK
    CVMhookVMStart(ee);
#endif

    /* In the future JVMPI will need to get a thread start event here
       as well */

    ++numJVMs;

     /* record time-stamp at VM creation*/
#ifdef CVM_TIMESTAMPING
    CVMtimeStampRecord(ee, "VM Created", -1);
    if (CVMlocalExceptionOccurred(ee)) {
        errorStr = "CVMtimeStampRecord failed";
	errorNo = JNI_ERR;
	goto done;
    }
#endif

#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
    /* The estimation compiles the preloaded classes.  Hence, we should only
       do this after disambiguating the preloaded classes.
       Only do this estimation once.  Only doing it for the first VM instance
       being created should do the trick.  This is a nice point in time
       when the application isn't running yet and therefore won't interfere
       with the estimation:
    */
    if (numJVMs == 1) {
        CVMjitEstimateCompilationSpeed(ee);
    }
#endif

 done:
#ifdef CVM_CLASSLOADING
    if (xbootclasspath != NULL) {
	free(xbootclasspath);
    }
#endif
#ifdef CVM_XRUN
    if (xrunArguments != NULL) {
	free(xrunArguments);
    }
#endif

    CVMdestroyPathInfo(&pathInfo);

    if (errorStr != NULL) {
        CVMconsolePrintf("Cannot start VM (%s)\n", errorStr);
	if (ee != NULL && CVMlocalExceptionOccurred(ee)) {
	    CVMconsolePrintf("Exception during VM startup:\n", errorStr);
	    CVMdumpException(ee);
	}
    }
    return errorNo;
}

JNIEXPORT jint JNICALL
JNI_GetCreatedJavaVMs(JavaVM **vm_buf, jsize bufLen, jsize *numVMs)
{
#ifdef JDK12
    if (VM_created) {
	if (numVMs)
	    *numVMs = 1;
	if (bufLen > 0)
	    *vm_buf = (JavaVM *)(&main_vm);
    } else
	*numVMs = 0;
    return JNI_OK;
#else
    if (numJVMs > 0) {
	if (numVMs != NULL) {
	    *numVMs = 1;
	}
	if (bufLen > 0) {
	    *vm_buf = &CVMglobals.javaVM.vector;
	}
    } else {
	if (numVMs != NULL) {
	    *numVMs = 0;
	}
    }
    return JNI_OK;
#endif
}

static void
CVMunlinkNativeLibraries(CVMExecEnv *ee)
{
        CVMFieldTypeID systemNativeLibrariesTID =
            CVMtypeidLookupFieldIDFromNameAndSig(ee, "systemNativeLibraries",
                                                 "Ljava/util/Vector;");
        CVMFieldTypeID xrunNativeLibrariesTID =
            CVMtypeidLookupFieldIDFromNameAndSig(ee, "xrunNativeLibraries",
                                                 "Ljava/util/Vector;");
        const CVMClassBlock *cb = CVMsystemClass(java_lang_ClassLoader);
        CVMFieldBlock *systemNativeLibrariesFB = 
            CVMclassGetStaticFieldBlock(cb, systemNativeLibrariesTID);
        CVMFieldBlock *xrunNativeLibrariesFB = 
            CVMclassGetStaticFieldBlock(cb, xrunNativeLibrariesTID);
        CVMfbStaticField(ee, systemNativeLibrariesFB).raw = 0;
        CVMfbStaticField(ee, xrunNativeLibrariesFB).raw = 0;
}

static jint JNICALL
CVMjniDestroyJavaVM(JavaVM *vm)
{
    JNIEnv *env;
    void *envV;
    CVMExecEnv *ee;
    jint res;

    /* %comment: rt026 */
    res = (*vm)->AttachCurrentThread(vm, &envV, NULL);
    if (res < 0) {
	return res;
    }
    env = (JNIEnv *)envV;
    ee = CVMjniEnv2ExecEnv(env);

#ifdef CVM_EMBEDDED_HOOK
    CVMhookVMShutdown(ee);
#endif

    /* %comment d001 */
    /* Wait for all the user threads exit and call the shutdown hooks 
     * by invoking java.lang.Shutdown.shutdown() */
    CVMjniCallStaticVoidMethod(env,
	CVMcbJavaInstance(CVMsystemClass(java_lang_Shutdown)),
	CVMglobals.java_lang_Shutdown_waitAllUserThreadsExitAndShutdown);

    if (CVMlocalExceptionOccurred(ee)) {
	/*
	 * Ignore and clear the exception
	 */
	CVMconsolePrintf("Exception occurred during Shutdown.shutdown()\n");
	CVMjniExceptionDescribe(env);
	CVMclearLocalException(ee);
    }

    CVMprepareToExit();

    CVMpostThreadExitEvents(ee);
#ifdef CVM_JVMTI
    if (CVMjvmtiIsEnabled()) {
	CVMjvmtiPostVmExitEvent(ee);    
	CVMjvmtiDebugEventsEnabled(ee) = CVM_FALSE;
    }
#endif

    if (CVMglobals.fullShutdown) {
	ee->threadExiting = CVM_TRUE;
	/*
	 * Disbale any remote exception during the shutdown process 
	 */
	CVMdisableRemoteExceptions(ee);

	/*
	 * Free nativeLibrary object refs and call JVM_onUnload
	 * function for each if any.
	 * We do this before calling ThreadRegistry.waitAllSystemThreadsExit()
	 * because some system threads (like those created by the debugger 
	 * agent) won't exit until JVM_OnUnload is called.
	 */
#ifdef CVM_XRUN
	CVMXrunProcessTable(&CVMglobals.onUnloadTable, env, vm);
#endif
#ifdef CVM_AGENTLIB
	CVMAgentProcessTableUnload(&CVMglobals.agentTable, env, vm);
#endif

	/*
	 * Call sun.misc.ThreadRegistry.waitAllSystemThreadsExit()
	 * to shutdown system threads
	 */
	CVMjniCallStaticVoidMethod(env,
	    CVMcbJavaInstance(CVMsystemClass(sun_misc_ThreadRegistry)),
	    CVMglobals.sun_misc_ThreadRegistry_waitAllSystemThreadsExit);

	if (CVMlocalExceptionOccurred(ee)) {
	    /*
	     * Ignore and clear the exception
	     */
	    CVMconsolePrintf("Exception occurred during "
			     "ThreadRegistry.waitAllSystemThreadsExit()\n");
	    CVMjniExceptionDescribe(env);
	    CVMclearLocalException(ee);
	}
	CVMwaitForAllThreads(ee);
	/* Nullify the references to the native libraries.  This will allow GC
	 * to finalize and collect the NativeLibrary objects, thereby unloading
	 * them.  We can only safely do this after all threads have exited.
	 */
	CVMunlinkNativeLibraries(ee);

    }


#ifdef CVM_INSTRUCTION_COUNTING
    /*
     * We are at the end of the VM run. Dump byte-code stats.
     */
    CVMdumpStats();
#endif

#ifdef CVM_JIT
#ifdef CVM_JIT_PROFILE
     CVMJITcodeCacheDumpProfileData();
#endif
     CVMjitReportCompilationSpeed();
     CVMJITstatsDumpGlobalStats();
#ifdef CVM_GLOBAL_MICROLOCK_CONTENTION_STATS
     {
	 extern CVMUint32 slowMlockimplCount;
	 extern CVMUint32 fastMlockimplCount;
	 CVMconsolePrintf("SLOW m-LOCK COUNT = %d\n",
			  slowMlockimplCount);
	 CVMconsolePrintf("FAST m-LOCK COUNT = %d\n",
			  fastMlockimplCount);
     }     
#endif
#ifdef CVM_FASTALLOC_STATS
     {
	 extern CVMUint32 fastLockCount;
	 extern CVMUint32 slowLockCount;
	 extern CVMUint32 verySlowLockCount;
	 CVMconsolePrintf("FASTCOUNT=%d, SLOWCOUNT=%d, VERYSLOWCOUNT=%d\n", 
			  fastLockCount,
			  slowLockCount,
			  verySlowLockCount);
     }
#endif
#endif

#ifdef CVM_USE_MEM_MGR
    CVMmemManagerDumpStats();
#endif

    /* record time-stamp at VM termination */
#ifdef CVM_TIMESTAMPING
    if (!CVMtimeStampFinishup(ee)) {
        CVMconsolePrintf("CVMtimeStampFinishup failed\n");
    }
#endif

#ifdef CVMJIT_COUNT_VIRTUAL_INLINE_STATS
    /* See jitgrammarrules.jcs for details */
    {
        extern void CVMJITprintVirtualInlineHitMiss();
        CVMJITprintVirtualInlineHitMiss();
    }
#endif

    /*
     * If we have a process model, then we cannot destroy the VM data
     * structures, otherwise the daemon threads may crash.
     */
    /* %comment d002 */

    if (CVMglobals.fullShutdown) {
	CVMunloadApplicationclasses(ee);

#ifdef CVM_CLASSLOADING
	CVMclassBootClassPathDestroy(ee);
	CVMclassClassPathDestroy(ee);
	CVMpackagesDestroy();
#endif
	CVMdestroyVMGlobalState(ee, &CVMglobals);
 
	/* 
	 * We should probably call DetachCurrentThread, but
         * CVMdestroyVMGlobalState has already destroyed the EE,
         * so free it here.
	 */

	if (ee != &CVMglobals.mainEE) {
	    free(ee);
#ifdef CVM_DEBUG
	    ee = NULL;
#endif
	}


	/* %comment: rt028 */
	if (numJVMs == 1) {
	    CVMdestroyStaticState();
	}

    }

#ifdef CVM_JIT
    CVMCCMstatsDumpStats();
#endif

    --numJVMs;

    return JNI_OK;
}

static jint JNICALL
attachCurrentThread(JavaVM *vm, void **penv, void *_args, CVMBool isDaemon)
{
    JavaVMAttachArgs *args = (JavaVMAttachArgs *)_args;
    /* %comment: rt029 */
    CVMExecEnv *ee = CVMgetEE();
    JNIEnv *env;

    if (ee != NULL) {
	/* already attached */
        env = CVMexecEnv2JniEnv(ee);
        *(JNIEnv **)penv = env;
	/* %comment: rt030 */
	return JNI_OK;
    } else {
	ee = (CVMExecEnv *)calloc(1, sizeof *ee);
	if (ee == NULL) {
	    return JNI_ERR;
	}
	if (!CVMinitExecEnv(ee, ee, NULL)) {
	    free(ee);
	    return JNI_ERR;
	}
	/* attach thread */
	if (!CVMattachExecEnv(ee, CVM_TRUE)) {
	    CVMdestroyExecEnv(ee);
	    free(ee);
	    return JNI_ERR;
	}
    
	CVMaddThread(ee, !isDaemon);

        env = CVMexecEnv2JniEnv(ee);
        *(JNIEnv **)penv = env;

	{
	    /* allocate thread object */
	    jclass threadClass =
		CVMcbJavaInstance(CVMsystemClass(java_lang_Thread));
	    jobject threadGroup = NULL;
	    jobject threadName = NULL;
	    jobject thread = NULL;
	    jlong eetop;

	    eetop = CVMvoidPtr2Long(ee);
	    if (args && args->version >= JNI_VERSION_1_2) {
		if (args->group != NULL) {
		    threadGroup = args->group;
		}
		if (args->name != NULL) {
		    threadName = CVMjniNewStringUTF(env, args->name);
		    if (threadName == NULL) {
			goto handleException;
		    }
		}
	    }

	    /* If the user doesn't supply a thread name, then create one now.
	     * It's not safe to let Thread.initAttachedThread() do this,
	     * since it relies on calling on java code that will result
	     * in a NullPointerException when the current thread is not
	     * yet set.
	     */
	    if (threadName == NULL) {
		char buf[20];
		jint threadNum = CVMjniCallStaticIntMethod(env, threadClass,
		    CVMglobals.java_lang_Thread_nextThreadNum);
		if (CVMexceptionOccurred(ee)) {
		    goto handleException;
		}
		sprintf(buf, "Thread-%d", threadNum);
		threadName = CVMjniNewStringUTF(env, buf);
		if (threadName == NULL) {
		    goto handleException;
		}
	    }

	    /* Initialize Thread object of this attached thread */
	    thread = CVMjniCallStaticObjectMethod(env, threadClass,
		CVMglobals.java_lang_Thread_initAttachedThread,
		threadGroup, threadName,
		java_lang_Thread_NORM_PRIORITY, eetop, isDaemon);

	handleException:
	    if (CVMexceptionOccurred(ee)) {
		CVMdumpException(ee);
		CVMjniExceptionClear(env);
		CVMdetachExecEnv(ee);
		CVMdestroyExecEnv(ee);
		CVMremoveThread(ee, ee->userThread);
		free(ee);
                return JNI_ERR;
            }

	    CVMassert(thread != NULL);

	    CVMID_icellAssign(ee, CVMcurrentThreadICell(ee), thread);
	    CVMjniDeleteLocalRef(env, thread);
	}

#ifdef CVM_JVMTI
	if (CVMjvmtiIsEnabled()) {
	    CVMjvmtiDebugEventsEnabled(ee) = CVM_TRUE;
	}
#endif
	CVMpostThreadStartEvents(ee);

#if defined(CVM_DEBUG) && defined(CVM_LVM) /* %begin lvm */
	CVMLVMtraceAttachThread(ee);
#endif /* %end lvm */

	return JNI_OK;
    }
}

static jint JNICALL
CVMjniAttachCurrentThread(JavaVM *vm, void **penv, void *_args)
{
    return attachCurrentThread(vm, penv, _args, CVM_FALSE);
}

static jint JNICALL
CVMjniDetachCurrentThread(JavaVM *vm)
{
    CVMExecEnv *ee = CVMgetEE();
    if (ee == NULL) {
	return JNI_EDETACHED;
    } else {
	/* %comment: rt031 */
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	jclass threadClass =
	    CVMcbJavaInstance(CVMsystemClass(java_lang_Thread));
	jobject thread = CVMcurrentThreadICell(ee);
	jobject throwable = CVMjniExceptionOccurred(env);
	CVMFrameIterator iter;
	CVMFrame *frame;

	/* Can't detach a thread with an active Java frame on it */
	frame = CVMeeGetCurrentFrame(ee);
	CVMframeIterateInit(&iter, frame);
	while (CVMframeIterateNextReflection(&iter, CVM_FALSE)) {
	    frame = CVMframeIterateGetFrame(&iter);
	    if (CVMframeIsJava(frame) 
#ifdef CVM_JIT
		|| CVMframeIsCompiled(frame)
#endif
	    ) {
		return JNI_ERR;
	    }
	}

	CVMjniExceptionClear(env);

	CVMpostThreadExitEvents(ee);
#ifdef CVM_JVMTI
	if (CVMjvmtiIsEnabled()) {
	    CVMjvmtiDebugEventsEnabled(ee) = CVM_FALSE;
	}
#endif

	CVMjniCallNonvirtualVoidMethod(
            env, thread, threadClass,
	    CVMglobals.java_lang_Thread_exit, throwable);

#if defined(CVM_DEBUG) && defined(CVM_LVM) /* %begin lvm */
	CVMLVMtraceDetachThread(ee);
#endif /* %end lvm */

	CVMjniDeleteLocalRef(env, throwable);

	CVMdetachExecEnv(ee);
	CVMdestroyExecEnv(ee);
	CVMremoveThread(ee, ee->userThread);
	if (ee != &CVMglobals.mainEE) {
	    free(ee);
	}
	return JNI_OK;
    }
}


static jint JNICALL
CVMjniGetEnv(JavaVM *vm, void **penv, jint version)
{
    CVMExecEnv *ee = CVMgetEE();
    if (ee != NULL) {
	if (version == JNI_VERSION_1_1 || version == JNI_VERSION_1_2 ||
	    version == JNI_VERSION_1_4) {
	    *penv = (void *)CVMexecEnv2JniEnv(ee);
	    return JNI_OK;
#ifdef CVM_JVMPI
        } else if (version == JVMPI_VERSION_1 ||
                   version == JVMPI_VERSION_1_1) {
            *penv = (void *)CVMjvmpiGetInterface1();
            return JNI_OK;
#endif /* CVM_JVMPI */

#ifdef CVM_JVMTI
        } else if (version == JVMTI_VERSION_1) {
            return CVMjvmtiGetInterface(vm, penv);
#endif /* CVM_JVMTI */
#ifdef CVM_JVMTI_IOVEC
        } else if (version == JVMIOVEC_VERSION_1) {
            *penv = (void *)&CVMioFuncs;
            return JNI_OK;
#endif
        } else {
	    *penv = NULL;
	    return JNI_EVERSION;
        }
    } else {
	*penv = NULL;
        return JNI_EDETACHED;
    }
}

static jint JNICALL
CVMjniAttachCurrentThreadAsDaemon(JavaVM *vm, void **penv, void *_args)
{
    return attachCurrentThread(vm, penv, _args, CVM_TRUE);
}

static const struct JNIInvokeInterface CVMmainJVMfuncs = {
    NULL,
    NULL,
    NULL,

    CVMjniDestroyJavaVM,
    CVMjniAttachCurrentThread,
    CVMjniDetachCurrentThread,
    CVMjniGetEnv,
    /* JNI_VERSION_1_4 additions: */
    CVMjniAttachCurrentThreadAsDaemon,
};

void
CVMinitJNIEnv(CVMJNIEnv *p_env)
{
    p_env->vector = &CVMmainJNIfuncs;
}

void
CVMdestroyJNIEnv(CVMJNIEnv *p_env)
{
}

#ifdef CVM_JVMTI

struct JNINativeInterface *
CVMjniGetInstrumentableJNINativeInterface()
{
    return &CVMmainJNIfuncs;
}

#endif /* CVM_JVMTI */

void
CVMinitJNIJavaVM(CVMJNIJavaVM *p_javaVM)
{
    p_javaVM->vector = &CVMmainJVMfuncs;
}

void
CVMdestroyJNIJavaVM(CVMJNIJavaVM *p_javaVM)
{
}
