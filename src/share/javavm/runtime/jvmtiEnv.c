/*
 * @(#)jvmtiEnv.c	1.7 06/10/31
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
 * This file is derived from the original CVM jvmdi.c file.	 In addition
 * the 'jvmti' components of this file are derived from the J2SE
 * jvmtiEnv.cpp class.	The primary purpose of this file is to 
 * instantiate the JVMTI API to external libraries.
 */ 

#include "javavm/include/porting/ansi/stdarg.h"
#include "javavm/include/defs.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/localroots.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/signature.h"
#include "javavm/include/globals.h"
#include "javavm/include/stacks.h"
#include "javavm/include/bag.h"
#include "javavm/include/porting/time.h"
#include "javavm/include/path_md.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/named_sys_monitor.h"
#include "javavm/include/constantpool.h"
#include "javavm/include/stackmaps.h"
#include "javavm/include/opcodes.h"
#include "generated/offsets/java_lang_Thread.h"
#include "generated/jni/java_lang_reflect_Modifier.h"
#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"
#include "javavm/export/jvmti.h"
#include "javavm/include/jvmtiEnv.h"
#include "javavm/include/jvmtiDumper.h"
#include "javavm/include/jvmtiCapabilities.h"
#include "javavm/include/verify.h"

#ifdef CVM_HW
#include "include/hw.h"
#endif

/* CVM JVMTI Implementation Presuppositions:
   =========================================
   1. Stack traces will include reflection frames.
      This is reflected in the frame count and all other stack related
      inspection functions as well.
 */

/* TODO: Review JVMTI functions for all the error conditions that the spec says
   that can be checked for. */
/* TODO: Review interplay of CVMglobals.jvmtiLock and CVM_CODE_LOCK for race
   conditions.  Currently, jvmtiLock replaces CODE_LOCK in some cases but not
   in JIT cases.  Need to be consistent with this.
   - Not critical yet because JIT is not used wit breakpoints currently.
*/

/* Convenience macros */

/* TODO: #undef this before final release: */
#define DO_PARANOID_ASSERTS
#ifdef DO_PARANOID_ASSERTS
#define CVMextraAssert(x)     CVMassert(x)
#else
#define CVMextraAssert(x) 
#endif

/* differences in flags from java class version */
#define CVM_CLASS_FLAG_DIFFS  (CVM_CLASS_ACC_SUPER |            \
                               CVM_CLASS_ACC_REFERENCE |        \
                               CVM_CLASS_ACC_FINALIZABLE)

#define INITIAL_BAG_SIZE 4

#define VALID_CLASS(cb)			      \
    {					      \
	if ((cb)==NULL) {		      \
	    return JVMTI_ERROR_INVALID_CLASS; \
	}				      \
    }

#define VALID_OBJECT(o)					\
    {							\
	if ((o)==NULL) {				\
	    return JVMTI_ERROR_INVALID_OBJECT;		\
	}						\
    }

#define JVMTI_ENABLED() \
    {					      \
	if (!CVMjvmtiIsEnabled()) {	      \
	    return JVMTI_ERROR_ACCESS_DENIED; \
	}				      \
    }

#define NULL_CHECK(Ptr, Error)   \
    {				   \
	if ((Ptr) == NULL) {	   \
	    return (Error);	   \
	}			   \
    }
#define NOT_NULL(ptr)			     \
    {					     \
	if (ptr == NULL) {		     \
	    return JVMTI_ERROR_NULL_POINTER; \
	}				     \
    }
#define NOT_NULL2(ptr1,ptr2)			\
    {						\
	if (ptr1 == NULL || ptr2 == NULL) {	\
	    return JVMTI_ERROR_NULL_POINTER;	\
	}					\
    }

#define ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(ptr, errorCode)	\
    {								\
	if (NULL == (ptr)) {					\
	    return (errorCode);				\
	}							\
    }

#define ASSERT_NOT_NULL2_ELSE_EXIT_WITH_ERROR(ptr1, ptr2, errorCode) \
    {								      \
	if (NULL == (ptr1) || NULL == (ptr2)) {			      \
	    return (errorCode);				      \
	}							      \
    }

#define METHOD_ID_OK(mb)				\
    {							\
	if (mb == NULL) {				\
	    return JVMTI_ERROR_INVALID_METHODID;	\
	}						\
    }

#define THREAD_OK(ee)					\
    {							\
	if (ee == NULL) {				\
	    return JVMTI_ERROR_INVALID_THREAD;		\
	}						\
    }

#define ALLOC(env, size, where) \
    {									\
	jvmtiError allocErr =						\
	    jvmti_Allocate(env, size, (unsigned char**)where);		\
	if (allocErr != JVMTI_ERROR_NONE) {				\
	    return allocErr;						\
	}								\
    }

#define ALLOC_WITH_CLEANUP_IF_FAILED(env, size, where, cleanup)		\
    {									\
	jvmtiError allocErr =						\
	    jvmti_Allocate(env, size, (unsigned char**)where);		\
	if (allocErr != JVMTI_ERROR_NONE) {				\
	    cleanup;							\
	    return allocErr;						\
	}								\
    }

#define CHECK_JVMTI_ENV	 						\
	CVMJvmtiContext *context = CVMjvmtiEnv2Context(jvmtienv);	\
	if (!context->isValid) {					\
	    return JVMTI_ERROR_INVALID_ENVIRONMENT;			\
	}								\

#define CVM_THREAD_LOCK(ee)   CVMsysMutexLock(ee, &CVMglobals.threadLock)
#define CVM_THREAD_UNLOCK(ee) CVMsysMutexUnlock(ee, &CVMglobals.threadLock)

#ifdef CVM_JIT
#define CVM_JIT_LOCK(ee)      CVMsysMutexLock(ee, &CVMglobals.jitLock)
#define CVM_JIT_UNLOCK(ee)    CVMsysMutexUnlock(ee, &CVMglobals.jitLock)
#else
#define CVM_JIT_LOCK(ee)
#define CVM_JIT_UNLOCK(ee)
#endif

#define CHECK_CAP(cap_)							\
    if (getCapabilities(context)->cap_ != 1) {				\
	return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;			\
    }


/* StackMutationStopped
   ====================
   Before inspecting the stack of a thread, we must ensure that the stack won't
   be changing while we're in the midst of inspecting it.  Otherwise, the data
   we collect may contain some inconsistencies.  There are 4 ways to guarantee
   that stack frames won't be mutating:

   1. The thread is in a GC Safe state
      ================================
      Stack frames can only be pushed and popped in a GC unsafe state.
      JIT code execution can only occur in a GC unsafe state.  Hence, if the
      thread is in a GC safe state, we know that it will not be pushing/popping
      frames, nor entering and exiting inlined methods in the case of JIT
      compiled code.  Hence, stack mutation has stopped.

   2. The thread is suspended
      =======================
      Obviously, if the thread is suspended, it won't be mutating.

   3. The thread is the current thread
      ================================
      While we're executing in our current function, the Java interpreter and
      JIT compiled code cannot be running in this thread at the same time.
      Hence, we know that stack mutation has stopped.

   4. All threads are in a GC safe state
      ==================================
      This state is basically an overkill approach to achieve condition 1
      above.  If all threads are GC safe, then it necessarily follows that the
      target of interest is also in a GC safe state.  Therefore, one technique
      for stopping mutation on another thread is to request a GC safe all
      state from the current thread.

   Note: The term stackMutationStopped would be more accurate if it were called
   stackFrameMutationStopped.  The actual ceasing of mutation that we are
   ensuring here is that of stack "frame"s being pushed or popped.  The thread
   can still be mutating the stack in terms modifying local variables stored
   on it.  However, for all useful purposes here, we need not be concerned with
   those kinds of stack mutations.  Hence, we use the term stackMutationStopped
   just for brevity.

   Why CVMgcIsInSafeAllState()??
      You may roll all threads to safe points in the context of thread A
      and then test for safety using the ee of thread B which may hit
      an assertion since A called for safety and B is doing a test.  Or
      you may roll all threads to safety in thread A and then try it again
      in some deeper routine in JVMTI which will assert.  So we check this
      flag.

   Using the CVMjvmti...ICellDirect() macros
   =========================================
   Note: When handling direct object pointers from icells/refs, we need to use
   the CVMjvmti...ICellDirect macros (defined further down below) if the
   current thread is excuting with a code region protected by the
   STOP/RESUME_STACK_MUTATION pair.  This is because a GC safe all state
   may be used to stop stack mutation.  In a GC safe all state, the current
   thread is not allowed to become GC unsafe (which is needed by the
   indirectmem.h macros).  Hence, we'll need to use these macros instead.

       Note, however, that the stack mutation stopped state is not necessarily
   the equivalent of a "safe to access direct object" state (as captured by
   the CVMjvmtiIsSafeToAccessDirectObject() macro).  Both states are similar
   in that they can both be achieved by requesting a GC safe all VM state.
   However, the current thread is already in a stack mutation stopped state
   without doing any extra work, but it certainly is not in a "safe to access
   direct object" state.  This is because without anything extra, there is
   nothing to prevent another thread from triggering a GC.

       Since the STOP/RESUME_STACK_MUTATION macro pair requires the use of
   the ICellDirect macros (assuming we will be manipulating direct object
   pointers), we need to ensure that the current thread is also in the "safe
   to access direct object" state.  We can achieve this by first acquiring the
   heapLock before entering the STOP/RESUME region.

       Again, the current thread is not allowed to become GC unsafe because
   the region may be operating under a GC safe all condition.

   Allocating JNI local refs in the STOP/RESUME
   ============================================
   If you want to allocate a JNI local ref to store results, DON'T do the
   allocation inside the STOP/RESUME region.  This is because the allocation
   may become GC unsafe.  The solution is to allocate the local ref using
   the CVM internal API CVMjniCreateLocalRef() before entering the region.
   After exiting the region,  if the local ref was not used, then release
   it with CVMjniDeleteLocalRef().

   Calling JNI functions inside the STOP/RESUME region
   ===================================================
   In general, make sure that any functions you call within the STOP/RESUME
   region will not cause the current thread to become GC unsafe.
*/

/* Checks if mutation is stopped on the stack of the specified thread. */
#define stackMutationStopped(ee) \
    (CVMgcIsInSafeAllState() || CVMD_isgcSafe(ee) ||	\
     CVMeeThreadIsSuspended(ee) || (ee == CVMgetEE()))

/* Stops mutation on the specified thread if necessary. */
#define STOP_STACK_MUTATION_ACTION(ee, targetEE, lockAction)		\
    {									\
	/* ts and bits used for assertion testing */			\
	CVMThreadState ts = targetEE->threadState;			\
	CVMThreadState bits = CVM_THREAD_SUSPENDED |			\
	    CVM_THREAD_STACK_MUTATOR_LOCK;				\
	(void)bits; 							\
	/* If the target thread is not the self thread, then the only	\
	   way to stop its mutation is to force it so a GC safe point.	\
	   It is not safe to suspend that thread as an alternative.	\
	   								\
	   If the target thread is the self thread then there's nothing \
	   to do.							\
	*/								\
	if (CVMeeThreadIsSuspended(targetEE)) {				\
	    targetEE->threadState |= CVM_THREAD_STACK_MUTATOR_LOCK;	\
	    ts |= CVM_THREAD_STACK_MUTATOR_LOCK;			\
	}								\
	CVMassert((ts &	bits) == bits || (ts & bits) ==  0);		\
	if (targetEE != ee && !CVMeeThreadIsSuspended(targetEE)) {	\
	    CVMExecEnv *refetchedEE;					\
									\
	    lockAction;							\
	    CVMD_gcBecomeSafeAll(ee);					\
	    /* Since the target thread is not the self thread, then	\
	       there's a chance that it could have died before we can   \
	       get the stack trace.  While the jthread keeps the object \
	       alive, we don't have anything to keep it's ee and stack	\
	       alive too.						\
									\
	       Hence, after forcing it to a GC safe point, we need to   \
	       refetch and recheck the threadEE from the jthread.  The	\
	       GC safe point prevents Java code from running, and	\
	       consequently the thread from exiting.			\
	    */								\
	    refetchedEE = getThreadEETopField(ee, thread);		\
	    if (refetchedEE == NULL) {					\
		err = JVMTI_ERROR_THREAD_NOT_ALIVE;			\
		goto _cleanUpAndReturn;					\
	    }								\
	    CVMassert(refetchedEE == targetEE);				\
	}								\
    }

/* Resumes mutation on the specified thread if previously stopped. */
#define RESUME_STACK_MUTATION_ACTION(ee, targetEE, unlockAction)	\
    _cleanUpAndReturn:							\
    if (targetEE->threadState & CVM_THREAD_STACK_MUTATOR_LOCK) {	\
	targetEE->threadState &= ~CVM_THREAD_STACK_MUTATOR_LOCK;	\
    }									\
    if (targetEE != ee && !CVMeeThreadIsSuspended(targetEE)) {		\
	CVMD_gcAllowUnsafeAll(ee);					\
	unlockAction;							\
    }

#define STOP_STACK_MUTATION(ee, targetEE)	\
    STOP_STACK_MUTATION_ACTION(ee, targetEE, { CVM_THREAD_LOCK(ee); })
#define RESUME_STACK_MUTATION(ee, targetEE)	\
    RESUME_STACK_MUTATION_ACTION(ee, targetEE, { CVM_THREAD_UNLOCK(ee); })


#define STOP_STACK_MUTATION_JIT(ee, targetEE)	\
    STOP_STACK_MUTATION_ACTION(ee, targetEE, {	\
	CVM_JIT_LOCK(ee);			\
	CVM_THREAD_LOCK(ee);		        \
    })
#define RESUME_STACK_MUTATION_JIT(ee, targetEE)	 \
    RESUME_STACK_MUTATION_ACTION(ee, targetEE, { \
	CVM_THREAD_UNLOCK(ee);                   \
	CVM_JIT_UNLOCK(ee);			 \
    })

typedef struct {
    CVMSysMutex* mutex;
} jvmtiSysMutexes;

static const jvmtiSysMutexes jvmtiMutexes[] = {
#ifdef CVM_CLASSLOADING
    {&CVMglobals.nullClassLoaderLock},
#endif
#ifdef CVM_JIT
    {&CVMglobals.jitLock},
#endif
    {&CVMglobals.heapLock},
    {&CVMglobals.threadLock}
};

#define NUM_JVMTI_SYSMUTEXES \
    (sizeof(jvmtiMutexes) / sizeof(jvmtiSysMutexes))

#define ACQUIRE_SYS_MUTEXES                               \
    {                                                     \
        int i;                                            \
        for (i = 0; i < NUM_JVMTI_SYSMUTEXES; i++) {     \
            CVMsysMutexLock(ee, jvmtiMutexes[i].mutex);   \
        }                                                 \
    }

#define RELEASE_SYS_MUTEXES                               \
    {                                                     \
        int i;                                            \
        for (i = NUM_JVMTI_SYSMUTEXES - 1; i >= 0; i--) {      \
            CVMsysMutexUnlock(ee, jvmtiMutexes[i].mutex); \
        }                                                 \
    }

/* Error names */
static const char* const jvmtiErrorNames[] =
{
    "JVMTI_ERROR_NONE",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "JVMTI_ERROR_INVALID_THREAD",
    "JVMTI_ERROR_INVALID_THREAD_GROUP",
    "JVMTI_ERROR_INVALID_PRIORITY",
    "JVMTI_ERROR_THREAD_NOT_SUSPENDED",
    "JVMTI_ERROR_THREAD_SUSPENDED",
    "JVMTI_ERROR_THREAD_NOT_ALIVE",
    NULL,
    NULL,
    NULL,
    NULL,
    "JVMTI_ERROR_INVALID_OBJECT",
    "JVMTI_ERROR_INVALID_CLASS",
    "JVMTI_ERROR_CLASS_NOT_PREPARED",
    "JVMTI_ERROR_INVALID_METHODID",
    "JVMTI_ERROR_INVALID_LOCATION",
    "JVMTI_ERROR_INVALID_FIELDID",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "JVMTI_ERROR_NO_MORE_FRAMES",
    "JVMTI_ERROR_OPAQUE_FRAME",
    NULL,
    "JVMTI_ERROR_TYPE_MISMATCH",
    "JVMTI_ERROR_INVALID_SLOT",
    NULL,
    NULL,
    NULL,
    NULL,
    "JVMTI_ERROR_DUPLICATE",
    "JVMTI_ERROR_NOT_FOUND",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "JVMTI_ERROR_INVALID_MONITOR",
    "JVMTI_ERROR_NOT_MONITOR_OWNER",
    "JVMTI_ERROR_INTERRUPT",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "JVMTI_ERROR_INVALID_CLASS_FORMAT",
    "JVMTI_ERROR_CIRCULAR_CLASS_DEFINITION",
    "JVMTI_ERROR_FAILS_VERIFICATION",
    "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_ADDED",
    "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_SCHEMA_CHANGED",
    "JVMTI_ERROR_INVALID_TYPESTATE",
    "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_HIERARCHY_CHANGED",
    "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_DELETED",
    "JVMTI_ERROR_UNSUPPORTED_VERSION",
    "JVMTI_ERROR_NAMES_DONT_MATCH",
    "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_CLASS_MODIFIERS_CHANGED",
    "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_MODIFIERS_CHANGED",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "JVMTI_ERROR_UNMODIFIABLE_CLASS",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "JVMTI_ERROR_NOT_AVAILABLE",
    "JVMTI_ERROR_MUST_POSSESS_CAPABILITY",
    "JVMTI_ERROR_NULL_POINTER",
    "JVMTI_ERROR_ABSENT_INFORMATION",
    "JVMTI_ERROR_INVALID_EVENT_TYPE",
    "JVMTI_ERROR_ILLEGAL_ARGUMENT",
    "JVMTI_ERROR_NATIVE_METHOD",
    NULL,
    "JVMTI_ERROR_CLASS_LOADER_UNSUPPORTED",
    NULL,
    NULL,
    NULL,
    "JVMTI_ERROR_OUT_OF_MEMORY",
    "JVMTI_ERROR_ACCESS_DENIED",
    "JVMTI_ERROR_WRONG_PHASE",
    "JVMTI_ERROR_INTERNAL",
    NULL,
    "JVMTI_ERROR_UNATTACHED_THREAD",
    "JVMTI_ERROR_INVALID_ENVIRONMENT"
};

#if 0 /* Unused.  TODO: remove if still unused later. */
/* Event threaded */
const jboolean JvmtiEventThreaded[] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    1,
    1,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};
#endif /* TODO */

static CVMJvmtiContext *CVMjvmtiCreateContext();

/* mlam :: REVIEW DONE */
/* TODO: Change to not rely on 64bit masking if possible.  At least change
   to us HPI long functions. */
static const jlong	GLOBAL_EVENT_BITS = ~THREAD_FILTERED_EVENT_BITS;

static CVMBool jvmtiIsGCOwner;

CVMBool
CVMjvmtiIsGCOwner() {
    return jvmtiIsGCOwner;
}

void
CVMjvmtiSetGCOwner(CVMBool jvmtiGCOwner) {
    CVMassert(jvmtiIsGCOwner != jvmtiGCOwner);
    jvmtiIsGCOwner = jvmtiGCOwner;
}

/*
 *	Threads
 */

/*
   CVMjvmti...ICellDirect() macros
   ===============================
   The ICellDirect macros (defined further down below) are used to get/set
   direct object pointers from/to icells/refs.

   CVMjvmtiGetICellDirect() is analogous to CVMID_icellGetDirect().
   CVMjvmtiSetICellDirect() is analogous to CVMID_icellSetDirect().
   CVMjvmtiAssignICellDirect() is analogous to CVMID_icellAssignDirect().

   Unlike their similarly named counterparts in indirectmem.h, this version of
   these macros allow direct handling of the object pointers under any of these
   conditions:
   1. the current thread is in a GC unsafe state, or
   2. the VM is in a GC safe all state that is requested by the current
      thread, or
   3. the current thread owns the heapLock.

   If any of these conditions are true, then the current thread will be able
   to handle direct object pointers of fearing complications with the GC.

   The above condition expresses a "safe to access direct object" state (as
   captured in the CVMjvmtiIsSafeToAccessDirectObject() macro) which
   basically says that it is safe to handle direct object pointers while the
   current thread is in this state.

   Note: these macros need to be used in place of their indirectmem.h
   counterparts when the current thread is in a region protected by the
   STOP/RESUME_STACK_MUTATION pair.  This is because a GC safe all state
   may be used to stop stack mutation.  In a GC safe all state, the current
   thread is not allowed to become GC unsafe (which is needed by the
   indirectmem.h macros).  Hence, we'll need to use these macros instead.
*/

/* Purpose: Checks to see if the VM has reached a safe all state requested by
            the current thread.  ee is the ee for the current thread.

   Note: in order for the VM to reached a safe all state, the current thread,
   being the requester, must have first acquire certain necessary sys mutexes.
   These locks also prevents GC from running.  Hence, it is safe for us to do
   direct object accesses without becoming GC unsafe.  In fact, the current
   thread is forbidden from becoming GC unsafe while we are in a safe all
   state.
*/


/* mlam :: REVIEW DONE */
/* Purpose: Gets the cb pointer from the class object. */
CVMClassBlock *
CVMjvmtiGetObjectCB(CVMExecEnv *ee, jobject obj)
{
    CVMClassBlock *cb = NULL;
    if (obj != NULL) {
	/* If we requested safety then don't do the gcUnsafeExec as
	 * we'll hit an assertion.  We requested the safeall so we
	 * can access objects directly.
	 */
	if (CVMgcIsGCThread(ee)) {
	    CVMObject* directObj = CVMjvmtiGetICellDirect(ee, obj);
	    cb = CVMobjectGetClass(directObj);
	} else {
	    CVMD_gcUnsafeExec(ee, {
		CVMObject* directObj = CVMID_icellDirect(ee, obj);
		cb = CVMobjectGetClass(directObj);
	    });
	}
    }
    return cb;
}

#define THREAD_ID_OK(thread)						\
    {									\
	CVMClassBlock *__cb__ = CVMjvmtiGetObjectCB(ee, thread);	\
	if (__cb__ != CVMsystemClass(java_lang_Thread) &&		\
	    !CVMisSubclassOf(ee, __cb__, CVMsystemClass(java_lang_Thread))) { \
	    return JVMTI_ERROR_INVALID_THREAD;				\
	}								\
    }

/* mlam :: REVIEW DONE */
/* Small utility function to read the eetop field from the Thread object.
   This function doesn't do any error checking.  It's assumed that the caller
   will do all necessary checks before call this function.
*/
static CVMExecEnv *
getThreadEETopField(CVMExecEnv *ee, jthread thread)
{
    CVMExecEnv *targetEE;
    CVMJavaLong eetop;

    CVMextraAssert(CVMD_isgcSafe(ee));
    CVMextraAssert(thread != NULL);
    CVMextraAssert(!CVMID_icellIsNull(thread));

    if (CVMjvmtiIsSafeToAccessDirectObject(ee)) {
	/* We're in a safe all state requested by the current thread.
	   Hence, cannot and do not need to become unsafe in order to
	   access the field. */
	CVMObject *threadObj = CVMjvmtiGetICellDirect(ee, thread);
	CVMD_fieldReadLong(threadObj, CVMoffsetOfjava_lang_Thread_eetop, eetop);
	
    } else {
	/* Else, access the field the normal way: */
	CVMID_fieldReadLong(ee, (CVMObjectICell*) thread,
			    CVMoffsetOfjava_lang_Thread_eetop,
			    eetop);
    }
    targetEE =  (CVMExecEnv*)CVMlong2VoidPtr(eetop);
    return targetEE;
}

/* mlam :: REVIEW DONE */
static CVMBool
getThreadHasRunOnceField(CVMExecEnv *ee, jthread thread)
{
    CVMJavaInt hasRunOnce;

    CVMextraAssert(CVMD_isgcSafe(ee));
    CVMextraAssert(thread != NULL);
    CVMextraAssert(!CVMID_icellIsNull(thread));

    if (CVMjvmtiIsSafeToAccessDirectObject(ee)) {
	/* We're in a safe all state requested by the current thread.
	   Hence, cannot and do not need to become unsafe in order to
	   access the field. */
	CVMObject *threadObj = CVMjvmtiGetICellDirect(ee, thread);
	CVMD_fieldReadInt(threadObj,
	    CVMoffsetOfjava_lang_Thread_hasStartedOnce, hasRunOnce);
	
    } else {
	/* Else, access the field the normal way: */
	CVMID_fieldReadInt(ee, (CVMObjectICell*) thread,
			   CVMoffsetOfjava_lang_Thread_hasStartedOnce,
			   hasRunOnce);
    }
    return (CVMBool)hasRunOnce;
}

/* mlam :: REVIEW DONE */
/* Purpose: Gets the ee pointer from the jthread object. */
static jvmtiError
jthreadToExecEnv(CVMExecEnv *ee, jthread thread, CVMExecEnv **targetEE)
{
    CVMassert(ee == CVMgetEE());
    CVMassert(targetEE != NULL);

    CVMassert(CVMD_isgcSafe(ee));

    /* JVMTI may pass in a NULL jthread as an indicator that the target
       thread of interest is the current thread. */
    if (thread == NULL) {
	*targetEE = ee;
	return JVMTI_ERROR_NONE;
    }
    if (CVMID_icellIsNull(thread)) {
	return JVMTI_ERROR_INVALID_THREAD;
    }
    /* Make sure that object passed in is in fact a Thread */
    THREAD_ID_OK(thread);
    *targetEE = getThreadEETopField(ee, thread);
    if (*targetEE == NULL) {
	return JVMTI_ERROR_THREAD_NOT_ALIVE;
    }
    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
/*
 * clean up breakpoint - doesn't actually remove it. 
 * lock must be held
 */
static CVMBool
clearBkpt(CVMExecEnv *ee, struct bkpt *bp)
{
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    CVMassert(*(bp->pc) == opc_breakpoint);
    CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.jvmtiLock));
    /* TODO: Do we need the CODE_LOCK here? Check interplay of CODE_LOCK
       and jvmtiLock. */
    *(bp->pc) = (CVMUint8)(bp->opcode);
#ifdef CVM_HW
    CVMhwFlushCache(bp->pc, bp->pc + 1);
#endif
	
    /* 
     * De-reference the enclosing class so that it's GC 
     * is no longer prevented by this breakpoint. 
     */
    (*env)->DeleteGlobalRef(env, bp->classRef);
#ifdef CVM_DEBUG
    bp->classRef = NULL;
#endif
    return CVM_TRUE;
}

/* Purpose: Gets the ClassBlock from the specified java.lang.Class ref. 
   See also CVMjvmtiClassObject2ClassBlock() in jvmtiEnv.h.
   CVMjvmtiClassObject2ClassBlock() takes a direct class object as input while
   CVMjvmtiClassRef2ClassBlock() takes a class ref i.e.  jclass.
*/
CVMClassBlock *
CVMjvmtiClassRef2ClassBlock(CVMExecEnv *ee, jclass clazz)
{
    CVMClassBlock *cb = NULL;

    CVMassert(CVMD_isgcSafe(ee));

    if (clazz == NULL) {
	return cb;
    }

#ifdef CVM_DEBUG_ASSERTS
    /* The ref must point to an instance of java.lang.Class: */
    CVMD_gcUnsafeExec(ee, {
	CVMClassBlock *ccb;
	CVMObject* directObj = CVMID_icellDirect(ee, clazz);
	ccb = CVMobjectGetClass(directObj);
	CVMassert(ccb == CVMsystemClass(java_lang_Class));
    });
#endif

    CVMD_gcUnsafeExec(ee, {
	cb = CVMgcUnsafeClassRef2ClassBlock(ee, clazz);
    });

    return cb;
}

/*
 * Capability functions
 */ 
/* mlam :: REVIEW DONE */
static jvmtiCapabilities *getCapabilities(CVMJvmtiContext *context)
{
    return &context->currentCapabilities;
}

/* mlam :: REVIEW DONE */
static jvmtiCapabilities *getProhibitedCapabilities(CVMJvmtiContext *context)
{
    return &context->prohibitedCapabilities;
}

static jvmtiError JNICALL
jvmti_GetPotentialCapabilities(jvmtiEnv* jvmtienv,
			       jvmtiCapabilities* capabilitiesPtr)
{
    CHECK_JVMTI_ENV;

    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);
    NOT_NULL(capabilitiesPtr);
    CVMjvmtiGetPotentialCapabilities(getCapabilities(context),
			       getProhibitedCapabilities(context),
			       capabilitiesPtr);
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_AddCapabilities(jvmtiEnv* jvmtienv,
		      const jvmtiCapabilities* capabilitiesPtr)
{
    CHECK_JVMTI_ENV;

    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);

    return CVMjvmtiAddCapabilities(getCapabilities(context),
			    getProhibitedCapabilities(context),
			    capabilitiesPtr, 
			    getCapabilities(context));
}


static jvmtiError JNICALL
jvmti_RelinquishCapabilities(jvmtiEnv* jvmtienv,
			     const jvmtiCapabilities* capabilitiesPtr)
{
    CHECK_JVMTI_ENV;

    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);
    CVMjvmtiRelinquishCapabilities(getCapabilities(context), capabilitiesPtr,
			    getCapabilities(context));
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetCapabilities(jvmtiEnv* jvmtienv,
		      jvmtiCapabilities* capabilitiesPtr)
{
    CHECK_JVMTI_ENV;

    NOT_NULL(capabilitiesPtr);
    CVMjvmtiCopyCapabilities(getCapabilities(context), capabilitiesPtr);
    return JVMTI_ERROR_NONE;
}

/*
 * Memory Management functions
 */ 

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_Allocate(jvmtiEnv* jvmtienv,
	       jlong size,
	       unsigned char** memPtr)
{
    unsigned char *mem;
    CVMJavaInt intSize;
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();

    NOT_NULL(memPtr);

    *memPtr = NULL;
    /* If size is 0, then we're done. */
    if (CVMlongEqz(size)) {
	return JVMTI_ERROR_NONE;
    }
    /* If size < 0, then the argument is illegal. */
    if (CVMlongLtz(size)) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }

    intSize = CVMlong2Int(size);
    /* If size is greater than will fit in an int, then it's asking for too
       much memory: */
    if (CVMlongNe(CVMint2Long(intSize), size)) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }

    mem = (unsigned char*)calloc(intSize, 1);
    if (mem == NULL) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }
    *memPtr = mem;
    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_Deallocate(jvmtiEnv* env,
		 unsigned char* mem)
{
    JVMTI_ENABLED();

    if (mem != NULL) {
	free(mem);
    }
    return JVMTI_ERROR_NONE;
}

/* Return via "threadStatusPtr" the current status of the thread and 
 * via "suspendStatusPtr" information on suspension. If the thread
 * has been suspended, bits in the suspendStatus will be set, and the 
 * thread status indicates the pre-suspend status of the thread.
 * JVMTI_THREAD_STATUS_* flags are used to identify thread and 
 * suspend status.
 * Errors: JVMTI_ERROR_INVALID_THREAD, JVMTI_ERROR_NULL_POINTER
 */

/* mlam :: REVIEW DONE */
/* TODO: Verify that thread states agree with the JVMTI spec. */
static jvmtiError JNICALL
jvmti_GetThreadState(jvmtiEnv* jvmtienv,
		     jthread thread,
		     jint* threadStatePtr)
{
    /* %comment: k029 */
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv* targetEE;
    jvmtiError err;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);

    /* NOTE:  Added this lock. This used to be a SYSTHREAD call,
       which I believe is unsafe unless you have the thread queue
       lock, because the thread might exit after you get a handle to
       its EE. (Must file bug) */
    /* On second thought, don't grab the lock because we can hit
     * a deadlock conditon.  One thread may have the lock, issued some
     * event to the jdwp agent which is calling here via a different
     * thread.  This thread needs to return so it can release the 
     * other thread which is waiting for the event to complete
     */

    /* just look for the CVMJvmtiThreadNode and if not there, report that the
     * thread is terminated
     */

    /* NOTE: We need to acquire the threadLock here to ensure that targetEE
       doesn't disappear on us before we dereference it to get the threadState.

       The comment above says it will deadlock because the following may have
       been encountered:
           Thread A acquires threadLock to do some big job.
	   While doing said job, thread A encounters a need to notify
	       the JVMTI agent of an event.
	   Thread A calls the agent, and the agent decides it needs to
	       do some job too.  It notifies agent thread B to render the
	       needed service.  Meanwhile, thread A blocks to wait for
	       thread B.
	   Thread B starts its job and decides that it needs to get some
	       info from JVMTI.  It calls GetThreadState() which tries
	       to lock the threadLock.
	   Deadlock:
	       Thread A is blocked waiting for thread B to finish its job.
	       Thread B is blocked waiting for thread A to release the
	           threadLock.

       However, how realistic is this scenario?  Is there any situation where
       the VM will post a JVMTI event while holding a sys mutex?  The likely
       case would be during a GC cycle.  However, the JVMTI spec specific
       prohibits the agent from calling certain JVMTI functions from those
       GC call back functions.  But is the agent allowed to call said functions
       from another thread?

       This needs to be reviewed further.  Also need to retest to see if the
       above observation is true.
    */

    /*
    CVM_THREAD_LOCK(ee);
    */
    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err == JVMTI_ERROR_INVALID_THREAD) {
	goto cleanUpAndReturn;
    } else {
	int state = 0;
	if (threadStatePtr == NULL) {
	    err = JVMTI_ERROR_NULL_POINTER;
	    goto cleanUpAndReturn;
	}

	err = JVMTI_ERROR_NONE;
	if (thread == NULL && targetEE != NULL) {
	    thread = CVMcurrentThreadICell(targetEE);
	}
	if (targetEE != NULL && CVMjvmtiFindThread(ee, thread) != NULL) {
	    state = targetEE->threadState;
	} else if (targetEE == NULL &&
		   getThreadHasRunOnceField(ee, thread)) {
	    state = CVM_THREAD_TERMINATED;
	} else {
	    /* thread is new */
	    *threadStatePtr = 0;
	    goto cleanUpAndReturn;
	}
	switch(state) {
	case CVM_THREAD_TERMINATED:
	    *threadStatePtr = JVMTI_THREAD_STATE_TERMINATED;
	    break;
	case CVM_THREAD_RUNNING:
	    *threadStatePtr = JVMTI_THREAD_STATE_ALIVE |
		JVMTI_THREAD_STATE_RUNNABLE;
	    break;
        case  CVM_THREAD_IN_NATIVE:
	    *threadStatePtr = state | JVMTI_THREAD_STATE_ALIVE |
		JVMTI_THREAD_STATE_RUNNABLE;
	    break;
	default:
	    *threadStatePtr = JVMTI_THREAD_STATE_ALIVE | state;
	    break;
	}
    }
    err = JVMTI_ERROR_NONE;

 cleanUpAndReturn:
    /* TODO: Need to replace this with alternate solution: 
    CVM_THREAD_UNLOCK(ee);
    */
    return err;
}

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_GetCurrentThread(jvmtiEnv* jvmtienv,
		       jthread* threadPtr)
{
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));
    JVMTI_ENABLED();

    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(threadPtr);
    THREAD_OK(ee);
    env = CVMexecEnv2JniEnv(ee);

    /* Note: the JNI spec says that NewLocalRef() can never.  Failure there
       will result in a fatal error.  So no need to check for failure here. */
    *threadPtr = (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));

    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
/* Return via "threadsPtr" all threads in this VM
 * ("threadsCountPtr" returns the number of such threads).
 * Memory for this table is allocated by the function specified 
 * in JVMTI_SetAllocationHooks().
 * Note: threads in the array are JNI global references and must
 * be explicitly managed.
 * Errors: JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY
 */
static jvmtiError JNICALL
jvmti_GetAllThreads(jvmtiEnv* jvmtienv,
		    jint* threadsCountPtr,
		    jthread** threadsPtr) {

    jvmtiError err = JVMTI_ERROR_NONE;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMJavaLong sz;
    jint threadsCount;
    jthread *threads = NULL;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(threadsCountPtr);
    NOT_NULL(threadsPtr);
    THREAD_OK(ee);

    /* NOTE: We must not trigger a GC while holding the thread lock.
       We are safe here because allocating global roots can cause
       expansion of the global root stack, but not a GC. */
    CVM_THREAD_LOCK(ee);

    /* count the threads */
    threadsCount = 0;
    CVM_WALK_ALL_THREADS(ee, currentEE, {
        /* TODO: JVMTI Spec says that only live threads are to be returned
	   here.  And live is defined by java.lang.Thread.isAlive() returning
	   TRUE for that thread.  Need to verify that the following is
	   adequate.  If not fix this to check isAlive() (or its internal
	   field). */
        CVMassert(CVMcurrentThreadICell(currentEE) != NULL);
	if (!CVMID_icellIsNull(CVMcurrentThreadICell(currentEE))) {
	    threadsCount++;
	}
    });

    sz = CVMint2Long(threadsCount * sizeof(jthread));
    err = jvmti_Allocate(jvmtienv, sz, (unsigned char**)&threads);
    if (err == JVMTI_ERROR_NONE) {
	int i = 0;

	/* fill in the threads */
	CVM_WALK_ALL_THREADS(ee, currentEE, {
	    /* TODO: Same isAlive() check as TODO above. */
	    if (!CVMID_icellIsNull(CVMcurrentThreadICell(currentEE))) {

		/* Note: the JNI spec says that NewLocalRef() can never.
		   Failure there will result in a fatal error.  So no need
		   to check for failure here. */
		threads[i] = (*env)->NewLocalRef(env,
				        CVMcurrentThreadICell(currentEE));
		i++;
	    }
	});
	CVMassert(i <= threadsCount);

    } else {
	threads = NULL;
	threadsCount = 0;
    }

    *threadsCountPtr = threadsCount;
    *threadsPtr = threads;
    CVM_THREAD_UNLOCK(ee);
    return err;
}

/* mlam :: REVIEW DONE */
/* Suspend the specified thread.
 * Errors: JVMTI_ERROR_INVALID_THREAD, JVMTI_ERROR_THREAD_SUSPENDED,
 * JVMTI_ERROR_VM_DEAD 
 */
static jvmtiError JNICALL
jvmti_SuspendThread(jvmtiEnv* jvmtienv,
		    jthread thread)
{
    jvmtiError err = JVMTI_ERROR_NONE;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv* env;
    CVMExecEnv* targetEE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    /* NOTE that this looks unsafe. However, later code will get the
       EE out of the Java thread object again, this time under
       protection of the thread lock, before doing any real work with
       the EE. */

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	CVMtraceJVMTI(("JVMTI: suspend thread error: 0x%x\n", (int)thread));
	return err;
    }
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);
    CVMtraceJVMTI(("JVMTI: suspend thread: 0x%x\n", (int)targetEE));
    if (targetEE == ee) {
	/*
	 * Suspending self. Don't need to check for previous
	 * suspension.	Shouldn't grab any locks here. (NOTE: JDK's
	 * JVM_SuspendThread recomputes whether it's suspending itself
	 * down in JDK's threads.c)
	 */
	JVM_SuspendThread(env, (jobject)thread);
	err = JVMTI_ERROR_NONE;
    } else {
#if 0
	CVMconsolePrintf("jvmti_SuspendThread: suspending thread \"");
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
	err = jthreadToExecEnv(ee, thread, &targetEE);
	if (targetEE == NULL) {
	    /*
	     * It has finished running and is a zombie
	     */
	    err = JVMTI_ERROR_INVALID_THREAD;
	} else {
	    if (CVMeeThreadIsSuspended(targetEE)) {
		err = JVMTI_ERROR_THREAD_SUSPENDED;
	    } else {
		JVM_SuspendThread(env, (jobject)thread);
		err = JVMTI_ERROR_NONE;
	    }
	}
	CVMlocksForThreadSuspendRelease(ee);
    }
    return err;
}

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_SuspendThreadList(jvmtiEnv* jvmtienv,
			jint requestCount,
			const jthread* requestList,
			jvmtiError* results)
{
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv* targetEE;
    int i;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL2(requestList, results);

    if (requestCount < 0) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }

    for (i = 0; i < requestCount; i++) {
	if (requestList[i] == NULL) {
	    results[i] = JVMTI_ERROR_INVALID_THREAD;
	    continue;
	}
	(void)jthreadToExecEnv(ee, requestList[i], &targetEE);
	if (targetEE == NULL) {
	    results[i] = JVMTI_ERROR_INVALID_THREAD;
	    continue;
	}
	results[i] = jvmti_SuspendThread(jvmtienv, requestList[i]);
    }
    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
/* Resume the specified thread.
 * Errors: JVMTI_ERROR_INVALID_THREAD, JVMTI_ERROR_THREAD_NOT_SUSPENDED, 
 * JVMTI_ERROR_INVALID_TYPESTATE,
 * JVMTI_ERROR_VM_DEAD
 */
static jvmtiError JNICALL
jvmti_ResumeThread(jvmtiEnv* jvmtienv,
		   jthread thread)
{
    jvmtiError err = JVMTI_ERROR_NONE;
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv* targetEE;
    JNIEnv* env;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(thread);

    env = CVMexecEnv2JniEnv(ee);

    CVM_THREAD_LOCK(ee);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	CVM_THREAD_UNLOCK(ee);
	return err;
    }
    CVMtraceJVMTI(("JVMTI: resume thread: 0x%x\n", (int)targetEE));
    if (targetEE != NULL) {
	if (CVMeeThreadIsSuspended(targetEE)) {
	    CVMassert(!(targetEE->threadState &CVM_THREAD_STACK_MUTATOR_LOCK));
	    JVM_ResumeThread(env, (jobject) thread);
	} else {
	    err = JVMTI_ERROR_THREAD_NOT_SUSPENDED;
	}
    }

    CVM_THREAD_UNLOCK(ee);

    return err;
}

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_ResumeThreadList(jvmtiEnv* jvmtienv,
		       jint requestCount,
		       const jthread* requestList,
		       jvmtiError* results)
{
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv* targetEE;
    int i;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL2(requestList, results);

    if (requestCount < 0) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }

    for (i = 0; i < requestCount; i++) {
	if (requestList[i] == NULL) {
	    results[i] = JVMTI_ERROR_INVALID_THREAD;
	    continue;
	}
	(void)jthreadToExecEnv(ee, requestList[i], &targetEE);
	if (targetEE == NULL) {
	    results[i] = JVMTI_ERROR_INVALID_THREAD;
	    continue;
	}
	results[i] = jvmti_ResumeThread(jvmtienv, requestList[i]);
    }
    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_StopThread(jvmtiEnv* jvmtienv,
		 jthread thread,
		 jobject exception)
{

    /*    CHECK_CAP(can_signal_thread); */
    /* See comment below, until we study this in detail we
     * return JVMTI_ERROR_MUST_POSSESS_CAPABILITY 
     */
#if 1
    return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
#else
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv* targetEE;
    CVMClassBlock* exceptionCb;
    jvmtiError err;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NULL_CHECK(exception, JVMTI_ERROR_INVALID_OBJECT);
    NULL_CHECK(thread, JVMTI_ERROR_INVALID_THREAD)

    CVMID_objectGetClass(ee, exception, exceptionCb);
    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }
    /* TODO: Exceptions are normally not thrown asynchronously.  This may
       cause the target thread to not be able to react appropriately
       in some cases.  Need to review the async exception system. */
    CVMsignalError(targetEE, exceptionCb, "");
    return JVMTI_ERROR_NONE;
#endif
}

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_InterruptThread(jvmtiEnv* jvmtienv,
		      jthread thread)
{
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv *targetEE;
    JNIEnv *env;	  
    jvmtiError err;
    CHECK_JVMTI_ENV;
    CHECK_CAP(can_signal_thread);

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    THREAD_OK(ee);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }
    env = CVMexecEnv2JniEnv(ee);

    JVM_Interrupt(env, thread);

    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
static jfieldID 
getFieldID(JNIEnv *env, jclass clazz, char *name, char *type)
{
    jfieldID id = (*env)->GetFieldID(env, clazz, name, type);
    jthrowable exc = (*env)->ExceptionOccurred(env);
    if (exc) {
	(*env)->ExceptionDescribe(env);
	(*env)->FatalError(env, "internal error in JVMTI");
    }
    return id;
}

/* mlam :: REVIEW DONE */
/* TODO: These are useful general purpose utils.  Consider moving to
   utils.c. */
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

/* mlam :: REVIEW DONE */
/* TODO: These are useful general purpose utils.  Consider moving to
   utils.c. *//* convert Unicode to UTF */
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

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_GetThreadInfo(jvmtiEnv* jvmtienv,
		    jthread thread,
		    jvmtiThreadInfo* infoPtr)
{
    jfieldID nameID;
    jfieldID priorityID;
    jfieldID daemonID;
    jfieldID groupID;
    jfieldID loaderID;
    jcharArray nameObj;
    jobject groupObj;
    jobject loaderObj;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(infoPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    /* JVMTI speci says that thread == NULL means the current thread: */
    if (thread == NULL) {
	thread = CVMcurrentThreadICell(ee);
    }

    THREAD_ID_OK(thread);

    /* Fetch and cache needed field IDs: */
    nameID = CVMglobals.jvmti.statics.nameID;
    if (nameID == 0) {
	/* We need to fetch the fieldIDs for the various private Thread fields
	   here.  Get them directly from the Thread class because:
	   1. The target thread arg may not be a subclass of Thread and may be
	      an illegal arg.
	   2. This doesn't work for subclasses of Thread, because CVM's impl
	      doesn't allow private fields to be accessible via JNI.

	   So, just get the fieldIDs from the Thread class itself.
	*/
	jclass threadClass =
	    CVMcbJavaInstance(CVMsystemClass(java_lang_Thread));

	CVMglobals.jvmti.statics.nameID =
	    getFieldID(env, threadClass, "name", "[C");
	CVMglobals.jvmti.statics.priorityID =
	    getFieldID(env, threadClass, "priority", "I");
	CVMglobals.jvmti.statics.daemonID =
	    getFieldID(env, threadClass, "daemon", "Z");
	CVMglobals.jvmti.statics.groupID =
	    getFieldID(env, threadClass, "group", "Ljava/lang/ThreadGroup;");
	CVMglobals.jvmti.statics.loaderID =
	    getFieldID(env, threadClass, "contextClassLoader",
		       "Ljava/lang/ClassLoader;");

	nameID = CVMglobals.jvmti.statics.nameID;
    }

    nameID = CVMglobals.jvmti.statics.nameID;
    priorityID = CVMglobals.jvmti.statics.priorityID;
    daemonID = CVMglobals.jvmti.statics.daemonID;
    groupID = CVMglobals.jvmti.statics.groupID;
    loaderID = CVMglobals.jvmti.statics.loaderID;

    /* Fill in the thread info result: */
    /* NOTE that when we use the JNI we don't want our accesses
       recorded, so we bypass the instrumented JNI vector and call the
       implementation directly. */
    nameObj = (jcharArray)CVMjniGetObjectField(env, thread, nameID);
    groupObj = CVMjniGetObjectField(env, thread, groupID);
    loaderObj = CVMjniGetObjectField(env, thread, loaderID);

    /* Copy the thread name into an allocated buffer. */
    {	/* copy the thread name */
	jint uniLen = CVMjniGetArrayLength(env, nameObj);
	jchar *uniChars = CVMjniGetCharArrayElements(env, nameObj, NULL);
	jint utfLen = lengthCharsUTF(uniLen, uniChars);
	CVMJavaLong sz = CVMint2Long(utfLen+1);

	ALLOC_WITH_CLEANUP_IF_FAILED(jvmtienv, sz, &(infoPtr->name), {
	    CVMjniDeleteLocalRef(env, nameObj);
	    CVMjniDeleteLocalRef(env, groupObj);
	    CVMjniDeleteLocalRef(env, loaderObj);
	    CVMjniReleaseCharArrayElements(env, nameObj, uniChars, JNI_ABORT);
	});

	setBytesCharsUTF(uniLen, uniChars, infoPtr->name);
	CVMjniReleaseCharArrayElements(env, nameObj, uniChars, JNI_ABORT);
    }
   
    /* Note: there's no need to allocate new JNI local refs for the group and
       loader because they were already attained as JNI local refs. */
    infoPtr->priority = CVMjniGetIntField(env, thread, priorityID);
    infoPtr->is_daemon = CVMjniGetBooleanField(env, thread, daemonID);
    infoPtr->thread_group = (jthreadGroup)groupObj;
    infoPtr->context_class_loader = loaderObj;

    return JVMTI_ERROR_NONE;
}

/* NOTE: We implement fast locking, so we don't implement
   GetOwnedMonitorInfo or GetCurrentContendedMonitor. */

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_GetOwnedMonitorInfo(jvmtiEnv* jvmtienv,
			  jthread thread,
			  jint* ownedMonitorCountPtr,
			  jobject** ownedMonitorsPtr)
{
    CVMOwnedMonitor *mon;
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *threadEE;
    jobject *tmpPtr;
    jobject lockedObjICell;
    int count = 0;
    int i = 0;
    JNIEnv *env;
    jvmtiError err;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(ownedMonitorCountPtr);
    NOT_NULL(ownedMonitorsPtr);


    env = CVMexecEnv2JniEnv(ee);
    *ownedMonitorCountPtr = 0;
    CVM_THREAD_LOCK(ee);
    err = jthreadToExecEnv(ee, thread, &threadEE);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    /* Count the monitors that are owned by this thread: */
    /* We need to acquire the syncLock here to prevent the inflated
     * monitor from being deflated or scavenged while we're in the
     * middle of inspecting it. */
    CVMsysMutexLock(ee, &CVMglobals.syncLock);
    mon = threadEE->objLocksOwned;
    while (mon != NULL) {
#ifdef CVM_DEBUG
	CVMassert(mon->state != CVM_OWNEDMON_FREE);
#endif
	if (mon->object != NULL) {
	    /* If the monitor is inflated, the current thread could be waiting
	     * on it.  We should not count those as being owned by the thread.
	     */
	    if (mon->type == CVM_OWNEDMON_HEAVY) {
		/* check heavy monitor */
		if (&mon->u.heavy.mon->mon != threadEE->blockingWaitMonitor) {
		    count++;
		}
	    } else {
		count++;
	    }
	}
	mon = mon->next;
    }
    CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
    /* If there are no monitors owned by this thread, then we're done: */
    if (count == 0) {
	*ownedMonitorsPtr =  NULL;
	CVMassert(err == JVMTI_ERROR_NONE);
	goto cleanUpAndReturn;
    }

    /* Allocate and fill in the owned monitor info: */
    ALLOC_WITH_CLEANUP_IF_FAILED(jvmtienv, count * sizeof(jobject),
				 ownedMonitorsPtr, {
        CVM_THREAD_UNLOCK(ee);
    });
    tmpPtr = *ownedMonitorsPtr;

    lockedObjICell = CVMmiscICell(ee);
    CVMassert(CVMID_icellIsNull(lockedObjICell)); /* Make sure not in use. */

    CVMsysMutexLock(ee, &CVMglobals.syncLock);
    mon = threadEE->objLocksOwned;
    while (mon != NULL) {
	if (mon->object != NULL) {
	    if (mon->type != CVM_OWNEDMON_HEAVY ||
		(&mon->u.heavy.mon->mon != threadEE->blockingWaitMonitor)) {
		CVMD_gcUnsafeExec(ee, {
		    CVMID_icellSetDirect(ee, lockedObjICell, mon->object);
		});
		tmpPtr[i++] = (*env)->NewLocalRef(env, lockedObjICell);
	    }
	}
	mon = mon->next;
    }
    CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
    CVMID_icellSetNull(lockedObjICell);
    *ownedMonitorCountPtr = count;

 cleanUpAndReturn:
    CVM_THREAD_UNLOCK(ee);
    return err;
}


static jvmtiError JNICALL
jvmti_GetOwnedMonitorStackDepthInfo(jvmtiEnv* jvmtienv,
				    jthread thread,
				    jint* monitorInfoCountPtr,
				    jvmtiMonitorStackDepthInfo** monitorInfoPtr) {
    CHECK_JVMTI_ENV;
    CHECK_CAP(can_get_owned_monitor_stack_depth_info);
    /*NOTE: we do not support this api at this time */
    return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
}


static jvmtiError JNICALL
jvmti_GetCurrentContendedMonitor(jvmtiEnv* jvmtienv,
				 jthread thread,
				 jobject* monitorPtr) {
    CVMExecEnv *threadEE, *ee;
    CVMObjMonitor *mon;
    JNIEnv *env;
    jvmtiError err;
    CVMObjectICell *monObj;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(monitorPtr);

    ee = CVMgetEE();
    env = CVMexecEnv2JniEnv(ee);
    CVM_THREAD_LOCK(ee);
    err = jthreadToExecEnv(ee, thread, &threadEE);
    if (err != JVMTI_ERROR_NONE) {
	CVM_THREAD_UNLOCK(ee);
	return err;
    }
    CVM_THREAD_UNLOCK(ee);

    *monitorPtr = NULL;
    /* If the monitor is inflated, the current thread could be waiting
     * on it.  We should not count those as being owned by the thread.
     */
    CVMsysMutexLock(ee, &CVMglobals.syncLock);
    mon = threadEE->objLockCurrent;
    if (mon != NULL) {
	monObj = CVMjniCreateLocalRef(ee);
	/* there could be a monitor here somewhere */
	if (threadEE->blockingLockEntryMonitor != NULL ||
	    threadEE->blockingWaitMonitor != NULL) {
	    if (mon->obj != NULL) {
		CVMD_gcUnsafeExec(ee, {
		    CVMID_icellSetDirect(ee, monObj, mon->obj);
		});
		*monitorPtr = monObj;
	    } else {
		CVMjniDeleteLocalRef(env, monObj);
	    }
	}
    }
    CVMsysMutexUnlock(ee, &CVMglobals.syncLock);
    return JVMTI_ERROR_NONE;
}


void startFunctionWrapper(void *arg)
{
    CVMJvmtiThreadNode *node = (CVMJvmtiThreadNode *)arg;
    jvmtiStartFunction proc = node->startFunction;
    const void *procArg = node->startFunctionArg;
    CVMJvmtiContext *context = node->context;

    /* TODO :: NOTE the cast (void *)procArg here gets rid of the warning
       but is it really safe to cast it?  I think procArg is passed in
       from the agent.  Hence, it can really be constant.  I think we
       pass it back to the agent as non const later.  If the agent does
       something stupid and modifies a const arg, then this may crash
       the VM.  Is that OK?
       - Consider that the agent is native code, and with native code
         all bets are off.
    */
    proc(&context->jvmtiExternal, CVMexecEnv2JniEnv(CVMgetEE()),
	 (void *)procArg);

}

/* Start the execution of a debugger thread. The given thread must be
 * newly created and never previously run. Thread execution begins 
 * in the function "startFunc" which is passed the single argument, "arg".
 * The thread is set to be a daemon.
 * Errors: JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_INVALID_PRIORITY,
 * JVMTI_ERROR_OUT_OF_MEMORY, JVMTI_ERROR_INVALID_THREAD.
 */
static jvmtiError JNICALL
jvmti_RunAgentThread(jvmtiEnv* jvmtienv,
		     jthread thread,
		     jvmtiStartFunction proc,
		     const void* arg,
		     jint priority)
{
    CVMJvmtiThreadNode *node;
    JNIEnv* env;
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv* targetEE;

    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(proc);

    env = CVMexecEnv2JniEnv(ee);
    /*
     * Make sure a thread hasn't been run for this thread object 
     * before.
     */
    if ((thread == NULL) ||
	(jthreadToExecEnv(ee, thread, &targetEE) == JVMTI_ERROR_NONE)) {
	return JVMTI_ERROR_INVALID_THREAD;
    }

    if ((priority < JVMTI_THREAD_MIN_PRIORITY) ||
	(priority > JVMTI_THREAD_MAX_PRIORITY)) {
	return JVMTI_ERROR_INVALID_PRIORITY;
    }

    node = CVMjvmtiInsertThread(ee, (CVMObjectICell*) thread);
    if (node == NULL) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }

    node->startFunction = proc;
    node->startFunctionArg = arg;
    node->context = context;
    /* %comment: k021 */
    CVMID_fieldWriteInt(ee, (CVMObjectICell*) thread,
			CVMoffsetOfjava_lang_Thread_daemon,
			1);
    CVMID_fieldWriteInt(ee, (CVMObjectICell*) thread,
			CVMoffsetOfjava_lang_Thread_priority,
			priority);

    JVM_StartSystemThread(env, thread, startFunctionWrapper, (void *)node);
    if ((*env)->ExceptionCheck(env)) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }

    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_SetThreadLocalStorage(jvmtiEnv* jvmtienv,
			    jthread thread,
			    const void* data)
{
    CVMJvmtiThreadNode *node;
    CVMExecEnv *ee = CVMgetEE();
    jvmtiError err;
    CVMExecEnv *threadEE;

    CHECK_JVMTI_ENV;

    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);

    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));

    if (thread == NULL) {
	thread = CVMcurrentThreadICell(ee);
    }
    /* check to see if thread is alive at all */
    err = jthreadToExecEnv(ee, thread, &threadEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }
    node = CVMjvmtiFindThread(ee, thread);
    if (node == NULL) {
        node = CVMjvmtiInsertThread(ee, thread);
	if (node == NULL) {
	    return JVMTI_ERROR_THREAD_NOT_ALIVE;
	}
    }
    node->jvmtiPrivateData = (void*)data;
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetThreadLocalStorage(jvmtiEnv* jvmtienv,
			    jthread thread,
			    void** dataPtr)
{
    CVMJvmtiThreadNode *node;
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *threadEE;
    jvmtiError err;

    CHECK_JVMTI_ENV;

    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);

    CVMassert (ee != NULL);
    CVMassert(CVMD_isgcSafe(ee));

    if (thread == NULL) {
	thread = CVMcurrentThreadICell(ee);
    }
    /* check to see if thread is alive at all */
    err = jthreadToExecEnv(ee, thread, &threadEE);
    if (err != JVMTI_ERROR_NONE) {
	CVMtraceJVMTI(("JVMTI: thread2execenv error: %d\n", (int)err));
	return err;
    }
    node = CVMjvmtiFindThread(ee, thread);
    if (node == NULL) {
        node = CVMjvmtiInsertThread(ee, thread);
	if (node == NULL) {
            CVMtraceJVMTI(("JVMTI: thread insert error: \n"));
	    return JVMTI_ERROR_THREAD_NOT_ALIVE;
	}
    }
    *dataPtr =  node->jvmtiPrivateData;
    return JVMTI_ERROR_NONE;
}


/*
 * Thread Group functions
 */ 

/* Return in 'groups' an array of top-level thread groups (parentless 
 * thread groups).
 * Note: returned objects are allocated globally and must be explictly
 * freed with DeleteGlobalRef.
 * Errors: JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY
 */
static jvmtiError JNICALL
jvmti_GetTopThreadGroups(jvmtiEnv* jvmtienv,
			 jint* groupCountPtr,
			 jthreadGroup** groupsPtr) {
    jobject sysgrp;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;	  
    CVMJavaLong sz;
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL2(groupCountPtr, groupsPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    /* We only have the one top group */
    sz = CVMint2Long(sizeof(jthreadGroup));
    ALLOC(jvmtienv, sz, groupsPtr);
    *groupCountPtr = 1;

    /* Obtain the system ThreadGroup object from a static field of 
     * Thread class */
    {
	jclass threadClass =
	    CVMcbJavaInstance(CVMsystemClass(java_lang_Thread));
	jobject systemThreadGroup = 
	    CVMjniGetStaticObjectField(env, threadClass,
		CVMjniGetStaticFieldID(env, threadClass,
		    "systemThreadGroup", "Ljava/lang/ThreadGroup;"));

	CVMassert(systemThreadGroup != NULL);

	sysgrp = (*env)->NewLocalRef(env, systemThreadGroup);
    }

    if (sysgrp == NULL) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }
    **groupsPtr = (jthreadGroup) sysgrp;
   
    return JVMTI_ERROR_NONE;
}

/* Return via "infoPtr" thread group info.
 * Errors: JVMTI_ERROR_INVALID_THREAD_GROUP, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetThreadGroupInfo(jvmtiEnv* jvmtienv,
			 jthreadGroup group,
			 jvmtiThreadGroupInfo* infoPtr)
{
    jfieldID tgParentID;
    jfieldID tgNameID = 0;
    jfieldID tgMaxPriorityID;
    jfieldID tgDaemonID;
    jstring nmString;
    jobject parentObj;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;	  
    CHECK_JVMTI_ENV;
  
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(infoPtr);
    THREAD_OK(ee);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(group, JVMTI_ERROR_INVALID_THREAD_GROUP);

    env = CVMexecEnv2JniEnv(ee);

    tgNameID = CVMglobals.jvmti.statics.tgNameID;
    if (tgNameID == NULL) {
	jclass tgClass =
	    CVMcbJavaInstance(CVMsystemClass(java_lang_ThreadGroup));

	CVMglobals.jvmti.statics.tgParentID =
	    getFieldID(env, tgClass, "parent", "Ljava/lang/ThreadGroup;");
	CVMglobals.jvmti.statics.tgNameID =
	    getFieldID(env, tgClass, "name", "Ljava/lang/String;");
	CVMglobals.jvmti.statics.tgMaxPriorityID =
	    getFieldID(env, tgClass, "maxPriority", "I");
	CVMglobals.jvmti.statics.tgDaemonID =
	    getFieldID(env, tgClass, "daemon", "Z");

	tgNameID = CVMglobals.jvmti.statics.tgNameID;
    }

    tgParentID = CVMglobals.jvmti.statics.tgParentID;
    tgMaxPriorityID = CVMglobals.jvmti.statics.tgMaxPriorityID;
    tgDaemonID = CVMglobals.jvmti.statics.tgDaemonID;


    /* NOTE that when we use the JNI we don't want our accesses
       recorded, so we bypass the instrumented JNI vector and call the
       implementation directly. */
    parentObj = CVMjniGetObjectField(env, group, tgParentID);
    infoPtr->parent = parentObj == NULL? NULL : 
	(jthreadGroup)((*env)->NewLocalRef(env, parentObj));
    nmString = (jstring)(CVMjniGetObjectField(env, group, tgNameID));
    infoPtr->max_priority = CVMjniGetIntField(env, group, 
					       tgMaxPriorityID);
    infoPtr->is_daemon = CVMjniGetBooleanField(env, group, tgDaemonID);

    {	  /* copy the thread group name */
	jint nmLen = (*env)->GetStringUTFLength(env, nmString);
	const char *nmValue = (*env)->GetStringUTFChars(env, nmString, NULL);
	CVMJavaLong sz = CVMint2Long(nmLen+1);
	ALLOC(jvmtienv, sz, &(infoPtr->name));
	strcpy(infoPtr->name, nmValue);
	(*env)->ReleaseStringUTFChars(env, nmString, nmValue);
    }
   
    return JVMTI_ERROR_NONE;
}

static jvmtiError 
objectArrayToArrayOfObject(jvmtiEnv *jvmtienv, JNIEnv *env, jint cnt,
			   jobject **destPtr, jobjectArray arr)
{
    jvmtiError rc;
    CVMJavaLong sz = CVMint2Long(cnt * sizeof(jobject));

    /* allocate the destination array */
    rc = jvmti_Allocate(jvmtienv, sz, (unsigned char**)destPtr);

    /* fill-in the destination array */
    if (rc == JVMTI_ERROR_NONE) {
	int inx;
	jobject *rp = *destPtr;
	for (inx = 0; inx < cnt; inx++) {
	    jobject obj = (*env)->NewLocalRef(env, 
			      (*env)->GetObjectArrayElement(env, arr, inx));
	    if (obj == NULL) {
		rc = JVMTI_ERROR_OUT_OF_MEMORY;
		break;
	    }
	    *rp++ = obj;
	}

	/* clean up partial array after any error */
	if (rc != JVMTI_ERROR_NONE) {
	    jvmti_Deallocate(jvmtienv, (unsigned char *)*destPtr);
	    *destPtr = NULL;
	}			 
    }
    return rc;
}

/* Return via "threadCountPtr" the number of threads in this thread group.
 * Return via "threadsPtr" the threads in this thread group.
 * Return via "groupCountPtr" the number of thread groups in this thread group.
 * Return via "groupsPtr" the thread groups in this thread group.
 * Errors: JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetThreadGroupChildren(jvmtiEnv* jvmtienv,
			     jthreadGroup group,
			     jint* threadCountPtr,
			     jthread** threadsPtr,
			     jint* groupCountPtr,
			     jthreadGroup** groupsPtr)
{
    jfieldID nthreadsID;
    jfieldID threadsID;
    jfieldID ngroupsID;
    jfieldID groupsID;
    jvmtiError rc;
    jint nthreads;
    jobject threads;
    jint ngroups;
    jobject groups;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;	  
    CHECK_JVMTI_ENV;
	
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL2(threadCountPtr, threadsPtr);
    NOT_NULL2(groupCountPtr, groupsPtr);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    nthreadsID = CVMglobals.jvmti.statics.nthreadsID;
    if (nthreadsID == 0) {
	jclass tgClass =
	    CVMcbJavaInstance(CVMsystemClass(java_lang_ThreadGroup));

	CVMglobals.jvmti.statics.nthreadsID =
	    getFieldID(env, tgClass, "nthreads", "I");
	CVMglobals.jvmti.statics.threadsID =
	    getFieldID(env, tgClass, "threads", "[Ljava/lang/Thread;");
	CVMglobals.jvmti.statics.ngroupsID =
	    getFieldID(env, tgClass, "ngroups", "I");
	CVMglobals.jvmti.statics.groupsID =
	    getFieldID(env, tgClass, "groups", "[Ljava/lang/ThreadGroup;");

	nthreadsID = CVMglobals.jvmti.statics.nthreadsID;
    }

    threadsID = CVMglobals.jvmti.statics.threadsID;
    ngroupsID = CVMglobals.jvmti.statics.ngroupsID;
    groupsID = CVMglobals.jvmti.statics.groupsID;

    /* NOTE that when we use the JNI we don't want our accesses
       recorded, so we bypass the instrumented JNI vector and call the
       implementation directly. */
    nthreads = CVMjniGetIntField(env, group, nthreadsID);
    threads = CVMjniGetObjectField(env, group, threadsID);
    ngroups = CVMjniGetIntField(env, group, ngroupsID);
    groups = CVMjniGetObjectField(env, group, groupsID);

    rc = objectArrayToArrayOfObject(jvmtienv, env, nthreads, threadsPtr,
				    threads);
    if (rc == JVMTI_ERROR_NONE) {
	rc = objectArrayToArrayOfObject(jvmtienv, env, ngroups, groupsPtr,
					groups);
	/* deallocate all of threads list on error */
	if (rc != JVMTI_ERROR_NONE) {
	    jvmti_Deallocate(jvmtienv, (unsigned char *)*threadsPtr);
	} else {
	    *threadCountPtr = nthreads;
	    *groupCountPtr = ngroups;
	}
    }
    return rc;
}


/*
 * Stack Frame functions
 */

/* mlam :: REVIEW DONE */
/* Returns the number of frames on the stack of the specified thread,
   including reflection and inlined frames if present. */
static int getFrameCount(CVMExecEnv *threadEE)
{
    CVMFrameIterator iter;
    CVMFrame *frame;
    jint count = 0;

    /* We're inspecting stack frames.  Make sure that stack mutation has
       stopped.  See comment on StackMutationStopped above for more details. 
       Our caller is responsible for ensuring this condition. */
    CVMassert(stackMutationStopped(threadEE));

    /* Note: in the following, we're counting the reflection and inlined
       frames as well if present. */
    frame = CVMeeGetCurrentFrame(threadEE);
    CVMframeIterateInit(&iter, frame);
    while (CVMframeIterateNextReflection(&iter, CVM_FALSE)) {
	count++;
    }
    return count;
}


/* mlam :: REVIEW DONE */
/* Return via "countPtr" the number of frames in the thread's call stack.
 * Errors: JVMTI_ERROR_INVALID_THREAD, JVMTI_ERROR_NULL_POINTER
 */
static jvmtiError JNICALL
jvmti_GetFrameCount(jvmtiEnv* jvmtienv,
		    jthread thread,
		    jint* countPtr)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *targetEE;
    jvmtiError err;
    CHECK_JVMTI_ENV;

    /* Agent code (i.e. our caller) should always be in a GC safe state. */
    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(countPtr);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);
    if (targetEE != ee) {
	/* We could do this for jdwp but the test suite doesn't suspend
	   CVMassert(CVMeeThreadIsSuspended(targetEE)); */
    }
    STOP_STACK_MUTATION(ee, targetEE);

    *countPtr = getFrameCount(targetEE);
    err = JVMTI_ERROR_NONE; /* success. */

    RESUME_STACK_MUTATION(ee, targetEE);
    return err;
}

/* mlam :: REVIEW DONE */
static jvmtiError
getFrame(CVMExecEnv *targetEE, jint depth, CVMFrameIterator *iter)
{
    CVMFrame *frame;
    CVMBool hasFrame;

    /* We're inspecting stack frames.  Make sure that stack mutation has
       stopped.  See comment on StackMutationStopped above for more details. 
       Our caller is responsible for ensuring this condition. */
    CVMassert(stackMutationStopped(targetEE));

    /* TODO :: Should this be JVMTI_ERROR_THREAD_NOT_ALIVE instead? */
    if (targetEE == NULL) {
	/* Thread hasn't any state yet. */
	return JVMTI_ERROR_INVALID_THREAD;
    }
    if (depth < 0) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }

    CVMassert(iter != NULL);

    /* Initialize the iterator: */
    frame = CVMeeGetCurrentFrame(targetEE);
    CVMframeIterateInit(iter, frame);
    /* Move the iterator to the desired "frame": */
    hasFrame = CVMframeIterateSkipReflection(iter, depth, CVM_FALSE, CVM_FALSE);

    /* Check to see if there is a "frame" to inspect: */
    if (!hasFrame) {
	return JVMTI_ERROR_NO_MORE_FRAMES;
    }
    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
/* Gets the method and PC of the frame at the specified depth. */
static jvmtiError
getFrameLocation(CVMExecEnv *targetEE, jint depth, jmethodID* methodPtr,
		 jlocation* locationPtr)
{
    CVMFrameIterator iter;
    CVMMethodBlock* mb;
    jvmtiError err;

    /* We're inspecting stack frames.  Make sure that stack mutation has
       stopped.  See comment on StackMutationStopped above for more details. 
       Our caller is responsible for ensuring this condition. */
    CVMassert(stackMutationStopped(targetEE));

    NOT_NULL2(methodPtr, locationPtr);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    mb = CVMframeIterateGetMb(&iter);
    /* The iterator skips all special frames with mb == NULL.  Hence, we are
       guaranteed that the mb should not be NULL.  The only other way for mb to
       be NULL (apart from special frames) is for there to be no more frames
       on the stack.  If that occurs, getFrame() would have returned with
       JVMTI_ERROR_NO_MORE_FRAMES above.  So, mb cannot be NULL. */
    CVMassert(mb != NULL);

    *methodPtr = (jmethodID)mb;

    /* Note: we don't need to worry about the jmd for <clinit> being freed
       causing us any problems here because the stack that we are inspecting
       is guaranteed to have stopped mutating for this duration.  Hence, if
       the mb is a <clinit>, it's JMD won't be freed until after stack
       mutation is resumed on that thread. */
    CVMassert(CVMmbIs(mb, NATIVE) || (CVMmbJmd(mb) != NULL));

    if (CVMmbIs(mb, NATIVE)) {
	*locationPtr = CVMint2Long(-1);
    } else {
	CVMUint8 *pc = CVMframeIterateGetJavaPc(&iter);
	*locationPtr = CVMint2Long(pc - CVMmbJavaCode(mb));
    }
    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
/* Return via "methodPtr" the active method in the 
 * specified frame. Return via "locationPtr" the index of the
 * currently executing instruction within the specified frame,
 * return negative one if location not available.
 * Errors: JVMTI_ERROR_INVALID_THREAD, JVMTI_ERROR_THREAD_NOT_ALIVE,
 * JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_NO_MORE_FRAMES
 */
static jvmtiError JNICALL
jvmti_GetFrameLocation(jvmtiEnv* jvmtienv,
		       jthread thread,
		       jint depth,
		       jmethodID* methodPtr,
		       jlocation* locationPtr)
{
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv *targetEE;
    jvmtiError err = JVMTI_ERROR_NONE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL2(methodPtr, locationPtr);
    if (depth < 0) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }
    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    STOP_STACK_MUTATION(ee, targetEE);

    err =  getFrameLocation(targetEE, depth, methodPtr, locationPtr);

    RESUME_STACK_MUTATION(ee, targetEE);
    return err;
}

/* mlam :: REVIEW DONE */
static jvmtiError
getStackTrace(CVMExecEnv *targetEE, jint startDepth, jint maxFrameCount,
	      jvmtiFrameInfo* frameBuffer, jint* countPtr)
{
    CVMFrameIterator iter;
    CVMFrame *frame;

    jvmtiError err = JVMTI_ERROR_NONE;
    jint frameCount = 0;
    int i = 0;
    int frameIndex;

    /* We're inspecting stack frames.  Make sure that stack mutation has
       stopped.  See comment on StackMutationStopped above for more details. 
       Our caller is responsible for ensuring this condition. */
    CVMassert(stackMutationStopped(targetEE));

    frameCount = getFrameCount(targetEE);

    /* If startDepth < 0, then start frame is at frameCount - |startDepth|.
       A negative startDepth is used to indicate that the caller wants to get
       a stack trace from the bottom of the stack instead of from the top
       (for a stack that grows from bottom to top). */
    frameIndex = startDepth;
    if (startDepth < 0) {
	if (startDepth < -frameCount) {
	    err = JVMTI_ERROR_ILLEGAL_ARGUMENT;
	    goto cleanupAndExit;
	}
	frameIndex = frameCount + startDepth;
    } else if (startDepth > 0) {
	if (startDepth >= frameCount) {
	    err = JVMTI_ERROR_ILLEGAL_ARGUMENT;
	    goto cleanupAndExit;
	}
    }
    /* frameIndex is the frame index from which to start the trace from. */
    CVMassert(frameIndex >= 0); 

    /* If we have more buffer capacity then frame data to fill it with, then
       cap the amount of capacity we'll use: */
    if (maxFrameCount > (frameCount - frameIndex)) {
	maxFrameCount = frameCount - frameIndex;
    }

    /* Extract frame into: */
    /* Note: This implementation is based on the logic in getFrame() and
       getFrameLocation().  We choose to use our own frame iterator loop
       here instead for efficiency reasons.  Going through getFrameLocation()
       for each frame would cause us to iterate the stack from the start
       frame for each frame that we need location info from.
    */
    frame = CVMeeGetCurrentFrame(targetEE);
    CVMframeIterateInit(&iter, frame);
    CVMframeIterateSkipReflection(&iter, frameIndex, CVM_FALSE, CVM_FALSE);

    for (i = 0; i < maxFrameCount; i++) {

	CVMMethodBlock *mb;

	mb = frameBuffer[i].method = CVMframeIterateGetMb(&iter);

	/* Note: we don't need to worry about the jmd for <clinit> being
	   freed causing us any problems here because the stack that we are
	   inspecting is guaranteed to have stopped mutating for this duration.
	   Hence, if the mb is a <clinit>, it won't be freed until after stack
	   mutation is resumed on that thread. */
	CVMassert(CVMmbIs(mb, NATIVE) || (CVMmbJmd(mb) != NULL));

	frameBuffer[i].method = mb;
	if (CVMmbIs(mb, NATIVE)) {
	    frameBuffer[i].location = CVMint2Long(-1);
	} else {
	    CVMUint8 *pc = CVMframeIterateGetJavaPc(&iter);
	    frameBuffer[i].location = CVMint2Long(pc - CVMmbJavaCode(mb));
	}

	/* Move on to the next frame: */
	CVMframeIterateNextReflection(&iter, CVM_FALSE);
    }

 cleanupAndExit:
    *countPtr = i; /* This is how many frames we've filled in. */
    return err;
}

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_GetStackTrace(jvmtiEnv* jvmtienv,
		    jthread thread,
		    jint startDepth,
		    jint maxFrameCount,
		    jvmtiFrameInfo* frameBuffer,
		    jint* countPtr)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *targetEE;
    jvmtiError err = JVMTI_ERROR_NONE;
    JNIEnv *env;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL2(frameBuffer, countPtr);
    if (maxFrameCount < 0) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }
    env = CVMexecEnv2JniEnv(ee);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    STOP_STACK_MUTATION_JIT(ee, targetEE);

    err = getStackTrace(targetEE, startDepth, maxFrameCount, frameBuffer,
			countPtr);

    RESUME_STACK_MUTATION_JIT(ee, targetEE);

 cleanUpAndReturn:
    return err;
}

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_GetThreadListStackTraces(jvmtiEnv* jvmtienv,
			       jint threadCount,
			       const jthread* threadList,
			       jint maxFrameCount,
			       jvmtiStackInfo** stackInfoPtr)
{
    int index, totalFrameCount;
    CVMExecEnv *ee = CVMgetEE();
    JNIEnv *env;
    jlong totalSize;
    jvmtiStackInfo *si;
    jvmtiFrameInfo *fi;
    jvmtiError err = JVMTI_ERROR_NONE;
    CVMExecEnv **targetEEs = NULL;
    CHECK_JVMTI_ENV;


    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL2(threadList, stackInfoPtr);

    CVMextraAssert(CVMD_isgcSafe(ee));

    env = CVMexecEnv2JniEnv(ee);

    err = jvmti_Allocate(jvmtienv, threadCount * sizeof(CVMExecEnv *),
			 (unsigned char **)&targetEEs);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanupAndExit;
    }

    CVM_JIT_LOCK(ee);
    CVM_THREAD_LOCK(ee);

    /* Roll all threads to safe points.  We do this before we call
       jthreadToExecEnv() below to ensure that the ee's that we get fetch
       the thread objects won't be invalidated before we get to inspect
       them. */
    CVMD_gcBecomeSafeAll(ee);

    /* Fetch the ee's for each of the thread objects: */
    for (index = 0; index < threadCount; index++) {
	jthreadToExecEnv(ee, threadList[index], &targetEEs[index]);
    }

    /* Add up the number of frames in all the stacks, not to exceed the
       specified maxFrameCount per stack: */
    totalFrameCount = 0;
    for (index = 0; index < threadCount; index++) {
	CVMExecEnv *currentEE = targetEEs[index];
	int frameCount = 0;
	if (currentEE != NULL) {
	    frameCount = getFrameCount(currentEE);
	}
	totalFrameCount += (frameCount > maxFrameCount ?
			      maxFrameCount : frameCount);
    }

    /* Allocate the buffer space to hold the jvmtiStackInfo and
       jvmtiFrameInfo results: */
    totalSize = threadCount * sizeof(jvmtiStackInfo) +
	totalFrameCount * sizeof(jvmtiFrameInfo);
    err = jvmti_Allocate(jvmtienv, totalSize,
			 (unsigned char **)stackInfoPtr);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanupAndExitSafe;
    }

    /* Fill in the stack anf frame info: */
    /* si moves  up, fi moves down */
    si = *stackInfoPtr + threadCount;  /* point to end of stackInfo */
    fi = (jvmtiFrameInfo *)si;            /* top of frame info area */
    for (index = 0; index < threadCount; index++) {
	CVMExecEnv *currentEE = targetEEs[index];
	--si;
	if (currentEE == NULL) {
	    si->thread = NULL;
	    /* TODO: Is this always true?  Is it  possible for a
	       thread to just be starting out and have a NULL ee? */
	    si->state = JVMTI_THREAD_STATE_TERMINATED;
	    si->frame_buffer = NULL;
	    si->frame_count = 0;
	} else {
	    jint frameCount;
	    err = getStackTrace(currentEE,
				/* startDepth */ 0,
				maxFrameCount,
				fi,
				&frameCount);
	    if (err != JVMTI_ERROR_NONE) {
		goto cleanupAndExitSafe;
	    }
	    si->thread = threadList[index];
	    /* TODO: Should the thread state be more comprehesive than this? 
	       What about waitng or blocked states, etc? */
	    si->state = JVMTI_THREAD_STATE_ALIVE |
		(CVMeeThreadIsSuspended(currentEE) ?
		 JVMTI_THREAD_STATE_SUSPENDED : JVMTI_THREAD_STATE_RUNNABLE);
	    si->frame_buffer = fi;
	    si->frame_count = frameCount;
	    fi += frameCount;
	}
    }
    CVMassert(si == *stackInfoPtr);
    /* The GC safe all would guarantee that mutation has stopped on all stacks
       because frame push/pops can only occur while a thread is GC unsafe.
       Hence, fi should now be pointing exactly at the end of the buffer as we
       expected from the buffer size calculations: */
    CVMassert((CVMUint8 *)fi == (((CVMUint8 *)*stackInfoPtr) + totalSize));

 cleanupAndExitSafe:
    CVMD_gcAllowUnsafeAll(ee);
    CVM_THREAD_UNLOCK(ee);
    CVM_JIT_UNLOCK(ee);

 cleanupAndExit:
    if (targetEEs != NULL) {
	jvmti_Deallocate(jvmtienv, (unsigned char*)targetEEs);
    }
    if (err != JVMTI_ERROR_NONE) {
	jvmti_Deallocate(jvmtienv, (unsigned char *)*stackInfoPtr);
	*stackInfoPtr = NULL;
    }
    return err;
}

/* mlam :: REVIEWING */
static jvmtiError JNICALL
jvmti_GetAllStackTraces(jvmtiEnv* jvmtienv,
			jint maxFrameCount,
			jvmtiStackInfo** stackInfoPtr,
			jint* threadCountPtr) {
    int threadCount, index;
    CVMExecEnv *ee = CVMgetEE();
    JNIEnv *env;
    jthread *threadlist;
    jvmtiError err = JVMTI_ERROR_NONE;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);

    env = CVMexecEnv2JniEnv(ee);
    /* count the threads */
    threadCount = 0;
    CVM_THREAD_LOCK(ee);
    CVM_WALK_ALL_THREADS(ee, currentEE, {
	    CVMassert(CVMcurrentThreadICell(currentEE) != NULL);
	    if (!CVMID_icellIsNull(CVMcurrentThreadICell(currentEE))) {
		threadCount++;
	    }
	});
    err = jvmti_Allocate(jvmtienv, threadCount * sizeof(jthread),
			 (unsigned char **)&threadlist);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }
    for (index = 0; index < threadCount; index++) {
	threadlist[index] = (*env)->NewLocalRef(env,
				    (CVMObjectICell *)&CVMID_nullICell);
    }
    index = 0;
    CVMD_gcBecomeSafeAll(ee);
    /* mlam :: TEST begin */
	    if (!CVMID_icellIsNull(CVMcurrentThreadICell(ee))) {
		CVMjvmtiAssignICellDirect(ee, threadlist[index++],
					  CVMcurrentThreadICell(ee));
	    }
	    /* mlam :: END test */
    CVM_WALK_ALL_THREADS(ee, currentEE, {
	    if (!CVMID_icellIsNull(CVMcurrentThreadICell(currentEE))) {
		CVMjvmtiAssignICellDirect(ee, threadlist[index++],
					  CVMcurrentThreadICell(currentEE));
	    }
	});
    CVMD_gcAllowUnsafeAll(ee);
    CVM_THREAD_UNLOCK(ee);
    *threadCountPtr = 0;
    err = jvmti_GetThreadListStackTraces(jvmtienv, threadCount, threadlist,
					 maxFrameCount, stackInfoPtr);
    if (err == JVMTI_ERROR_NONE) {
	*threadCountPtr = threadCount;
    }
    jvmti_Deallocate(jvmtienv, (unsigned char *)threadlist);
    return err;
}




static jvmtiError JNICALL
jvmti_PopFrame(jvmtiEnv* jvmtienv,
	       jthread thread) {

    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *threadEE;
    CVMFrame *currentFrame, *prevPrev;
    jvmtiError err;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    CHECK_CAP(can_pop_frame);

    if (getCapabilities(context)->can_pop_frame != 1) {
	return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
    }

    if (thread == NULL) {
	return JVMTI_ERROR_INVALID_THREAD;
    }
    CVM_THREAD_LOCK(ee);
    err = jthreadToExecEnv(ee, thread, &threadEE);
    if (err != JVMTI_ERROR_NONE) {
	CVM_THREAD_UNLOCK(ee);
	return err;
    }
    if (!(threadEE->threadState & CVM_THREAD_SUSPENDED)) {
	CVM_THREAD_UNLOCK(ee);
	return JVMTI_ERROR_THREAD_NOT_SUSPENDED;
    }
    if (CVMjvmtiNeedFramePop(threadEE)) {
	CVM_THREAD_UNLOCK(ee);
	return JVMTI_ERROR_NONE;
    }
    /*
     * From J2SE jvmtiEnvBase.cpp
     * 4812902: popFrame hangs if the method is waiting at a synchronize 
     * Catch this condition and return an error to avoid hanging.
     * Now JVMTI spec allows an implementation to bail out with an opaque
     * frame error.
     */
    if ((threadEE->threadState &
	 (CVM_THREAD_WAITING | CVM_THREAD_OBJECT_WAIT))) {
	CVM_THREAD_UNLOCK(ee);
	return JVMTI_ERROR_OPAQUE_FRAME;
    }
    currentFrame = CVMeeGetCurrentFrame(threadEE);
    /*
     * See if this frame and prev are both Java frames OR prev frame is Java and
     * prev(prev) frame is Java
     */
    if ((CVMframeIsJava(currentFrame) &&
	 CVMframeIsJava(CVMframePrev(currentFrame)))) {
	CVMjvmtiNeedFramePop(threadEE) = CVM_TRUE;
	CVMjvmtiSetProcessingCheck(threadEE);
	CVM_THREAD_UNLOCK(ee);
	CVMtraceJVMTI(("JVMTI: Popframe: 0x%x\n", (int)threadEE));
	return JVMTI_ERROR_NONE;
    }
    if (CVMframeIsJava(CVMframePrev(currentFrame))) {
	prevPrev = CVMframePrev(CVMframePrev(currentFrame));
	if (CVMframeIsJava(prevPrev)) {
	    CVMjvmtiNeedFramePop(threadEE) = CVM_TRUE;
	    CVMjvmtiSetProcessingCheck(threadEE);
	    CVM_THREAD_UNLOCK(ee);
	    CVMtraceJVMTI(("JVMTI: Popframe: 0x%x\n", (int)threadEE));
	    return JVMTI_ERROR_NONE;
	}
    }
    CVM_THREAD_UNLOCK(ee);
    return JVMTI_ERROR_OPAQUE_FRAME;
}



/*
 * Force Early Return functions
 */ 
/* mlam :: REVIEWING */
static jvmtiError
forceEarlyReturn(jvmtiEnv* jvmtienv, jthread thread, jvalue val,
		   CVMUint32 opcode) {
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *threadEE;
    CVMFrame *currentFrame;
    CVMFrameIterator iter;
    jvmtiError err;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    CHECK_CAP(can_force_early_return);

    CVM_THREAD_LOCK(ee);
    err = jthreadToExecEnv(ee, thread, &threadEE);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }
    if (threadEE != ee && !CVMeeThreadIsSuspended(threadEE)) {
	err = JVMTI_ERROR_THREAD_NOT_SUSPENDED;
	goto cleanUpAndReturn;
    }

    /* NOTE:  A JVMTI_ERROR_INTERNAL
       implies that the VM has a bug that prevents it from servicing this
       request.  However, is it really the agent's responsibility to make
       sure that it doesn't do more than one request concurrently?
       Consider the case of multiple concurrent agents.  It makes more
       sense that this JVMTI function should take care of synchronizing
       requests from agents automatically.

       On the other hand, once an early return or pop frame has been initiated,
       what's stopping another thread from getting a wrong frame (i.e. one
       that is about to be popped) from the stack?  It should be the
       responsibility of the stack dump code to provide automatic
       synchronization for this.

       What is realistic is actually to serialize pop frame requests and fail
       to pop if the frame has already been popped.  This is reasonable.
       It's up to the debugger to serial itself so as to not race against
       itself.  Just treat the request as a request on the next frame if
       any.

       However, need to check for a potential deadlock situation because the
       function is supposed to be called while suspended.  If we require
       pop frame (or early return) to synchronize on some lock, then
       the following deadlock can occur:

           Consider thread A successfully requests a pop frame on thread C.
           Thread B suspends thread C before frame is popped.
	   Thread B gets thread C stack dump.
	   Thread B requests pop frame on thread C.
	       Thread B blocks waiting for C to complete previous request.
	       Deadlock:
	           Thread B waits for thread C to complete previous request.
		   Thread C waits for thread B to resume it from suspension.

       Another option may be to fail with JVMTI_ERROR_OPAQUE_FRAME instead.
       Consider this: the frame becomes opaque the moment a force return
       has been requested on it.  It's like the frame is now considered
       native.  It's the responsibility of the debugger to not race against
       itself anyway.

       What if the target thread hasn't completed the request yet?  In this
       case the JVMTI function won't know any different from a race with
       another thread.  So, how can the debugger keep from racing in this
       way?  Conceivably, the debugger would want to know when the pop frame
       or early return has completed because it will want to let the user
       know.  This kind of requests usually come from the user and not
       from a programmatic sequence of things.  In the debugger scenario,
       they would want to break at either calling PC or the return PC.
       Hence, the debugger can run till the appropriate one of those
       breakpoints is hit.  Then, it can allow the user to request another
       pop frame or early return.  This means that returning
       JVMTI_ERROR_OPAQUE_FRAME would work also in the event that the
       previous request has not been completed yet.

       Summary:
       Case 1: race between 2 debugger threads: can be reasonably resolved
       by returning JVMTI_ERROR_OPAQUE_FRAME.
       Case 2: race between debugger thread and target thread's ability to
       fulfill the request: can be reasonably resolved by returning
       JVMTI_ERROR_OPAQUE_FRAME.

    */
    if (CVMjvmtiNeedEarlyReturn(threadEE)) {
	 /* The previous request is not done yet.  We can't service another
	    early return request on this target thread until it is done.
	    Also see the comments on "PopFrame and ForceEarlyReturn" at the
	    top of this file for more details on returning this error.
	 */
	err = JVMTI_ERROR_OPAQUE_FRAME;
	goto cleanUpAndReturn;
    }
    /*
     * From J2SE jvmtiEnvBase.cpp
     * 4812902: popFrame hangs if the method is waiting at a synchronize 
     * Catch this condition and return an error to avoid hanging.
     * Now JVMTI spec allows an implementation to bail out with an opaque
     * frame error.
     */
    if ((threadEE->threadState &
	 (CVM_THREAD_WAITING | CVM_THREAD_OBJECT_WAIT))) {
	err = JVMTI_ERROR_OPAQUE_FRAME;
	goto cleanUpAndReturn;
    }

    /* Use the frame iterator to skip over special frames that should not be
       visible to JVMTI and get the top most frame from JVMTI's perspective: */
    currentFrame = CVMeeGetCurrentFrame(threadEE);
    CVMframeIterateInit(&iter, currentFrame);
    CVMframeIterateNextReflection(&iter, CVM_FALSE);

    /* Note: If the top frame is a JIT inlined frame, then the GetFrame() below
       will return the frame for the outer most method that inlined the "top"
       frame.  This outer most method will necessarily be a compiled method
       and therefore, the IsJava() test will fail and we'll return a 
       JVMTI_ERROR_OPAQUE_FRAME which is what we would expect for a JIT inlined
       frame or a plain compiled frame.
     */
    currentFrame = CVMframeIterateGetFrame(&iter);
    if (CVMframeIsJava(currentFrame)) {
	CVMBool isAssignable;
	CVMMethodBlock *mb = currentFrame->mb;
	CVMClassBlock* cb = CVMmbClassBlock(mb);
	CVMMethodTypeID nameAndTypeID;
	CVMSigIterator parameters;
	CVMClassBlock* returnType;
	char retType =
	    CVMtypeidGetReturnType(CVMmbNameAndTypeID(currentFrame->mb));

	if (retType >= CVM_TYPEID_OBJ) {
	    nameAndTypeID =  CVMmbNameAndTypeID(mb);
	    CVMtypeidGetSignatureIterator( nameAndTypeID, &parameters );
	    returnType = CVMclassLookupClassWithoutLoading(ee,
		           CVM_SIGNATURE_ITER_RETURNTYPE(parameters),
		           CVMcbClassLoader(cb));

	    CVMD_gcUnsafeExec(ee, {
		CVMObject *directObj = CVMID_icellDirect(ee, val.l);
		isAssignable =
		    CVMisAssignable(ee, CVMobjectGetClass(directObj),
				    returnType);
	    });
	    if (!isAssignable) {
		err = JVMTI_ERROR_INVALID_OBJECT;
		goto cleanUpAndReturn;
	    }
	}
	CVMjvmtiNeedEarlyReturn(threadEE) = CVM_TRUE;
	CVMjvmtiSetProcessingCheck(threadEE);
	CVMjvmtiReturnValue(threadEE) = val;
	CVMjvmtiReturnOpcode(threadEE) = opcode;
    } else {
	err = JVMTI_ERROR_OPAQUE_FRAME;
    }

 cleanUpAndReturn:
    CVM_THREAD_UNLOCK(ee);
    return err;
}

static jvmtiError JNICALL
jvmti_ForceEarlyReturnObject(jvmtiEnv* jvmtienv,
			     jthread thread,
			     jobject value) {
    jvalue val;
    val.l = value;
    return forceEarlyReturn(jvmtienv, thread, val, opc_areturn);
}


static jvmtiError JNICALL
jvmti_ForceEarlyReturnInt(jvmtiEnv* jvmtienv,
			  jthread thread,
			  jint value) {
    jvalue val;
    val.i = value;
    return forceEarlyReturn(jvmtienv, thread, val, opc_ireturn);
}


static jvmtiError JNICALL
jvmti_ForceEarlyReturnLong(jvmtiEnv* jvmtienv,
			   jthread thread,
			   jlong value) {
    jvalue val;
    val.j = value;
    return forceEarlyReturn(jvmtienv, thread, val, opc_lreturn);
}


static jvmtiError JNICALL
jvmti_ForceEarlyReturnFloat(jvmtiEnv* jvmtienv,
			    jthread thread,
			    jfloat value) {
    jvalue val;
    val.f = value;
    return forceEarlyReturn(jvmtienv, thread, val, opc_freturn);
}


static jvmtiError JNICALL
jvmti_ForceEarlyReturnDouble(jvmtiEnv* jvmtienv,
			     jthread thread,
			     jdouble value) {
    jvalue val;
    val.d = value;
    return forceEarlyReturn(jvmtienv, thread, val, opc_dreturn);
}


static jvmtiError JNICALL
jvmti_ForceEarlyReturnVoid(jvmtiEnv* jvmtienv,
			   jthread thread) {
    jvalue val;
    val.j = 0L;
    return forceEarlyReturn(jvmtienv, thread, val, opc_return);
}


static jvmtiError
suspendAllThreads(jvmtiEnv *jvmtienv, CVMExecEnv *ee, jthread **threads,
		  int *threadCount)
{

    jvmtiError err;
    jint threadState;
    CVMExecEnv *target;
    int i;

    err = jvmti_GetAllThreads(jvmtienv, threadCount, threads);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }
    for (i = 0; i < *threadCount; i++) {
	jthreadToExecEnv(ee, (*threads)[i], &target);
	if (target == ee) {
	    (*threads)[i] = NULL;
	    continue;
	}
	err = jvmti_GetThreadState(jvmtienv, (*threads)[i], &threadState);
	if (err != JVMTI_ERROR_NONE) {
	    return err;
	}
	if ((threadState & CVM_THREAD_SUSPENDED) != 0) {
	    (*threads)[i] = NULL;
	}
    }

    for (i = 0; i < *threadCount; i++) {
	if ((*threads)[i] != NULL) {
	    err = jvmti_SuspendThread(jvmtienv, (*threads)[i]);
	    if (err != JVMTI_ERROR_NONE) {
		for (--i; i >= 0; i--) {
		    if ((*threads)[i] != NULL) {
			jvmti_ResumeThread(jvmtienv, (*threads)[i]);
		    }
		}
		return err;
	    }
	}
    }
    return JVMTI_ERROR_NONE;
}

static jvmtiError
resumeAllThreads(jvmtiEnv *jvmtienv, CVMExecEnv *ee, jthread *threads,
		 int threadCount)
{
    int i;

    for (i = 0; i < threadCount; i++) {
	if (threads[i] != NULL) {
	    jvmti_ResumeThread(jvmtienv, threads[i]);
	}
    }
    return JVMTI_ERROR_NONE;
}

/*
 * Heap functions
 */

CVMBool
jvmtiCheckMarkAndAdd(jvmtiEnv *jvmtienv, CVMObject *obj) {

    CVMJvmtiTagNode *node, *prev;
    int hashCode = (int)(obj) >> 2;
    int slot;
    CVMJvmtiTagNode **romObjects = CVMglobals.jvmti.statics.romObjects;

    if (hashCode < 0) {
	hashCode = -hashCode;
    }
    slot = hashCode % HASH_SLOT_COUNT;
    node = romObjects[slot];
    prev = NULL;
    while (node != NULL) {
	if (obj ==  (CVMObject*)node->ref) {
	    break;
	}
	prev = node;
	node = node->next;
    }
    if (node != NULL) {
	return CVM_TRUE;
    }
    if (jvmti_Allocate(jvmtienv, sizeof(CVMJvmtiTagNode),
		       (unsigned char **)&node) !=
	JVMTI_ERROR_NONE) {
	return CVM_TRUE;
    }
    node->ref = (jobject)obj;
    node->next = romObjects[slot];
    romObjects[slot] = node;
    return CVM_FALSE;
}

jvmtiError
CVMjvmtiVisitStackPush(CVMObject *obj)
{
    jvmtiError err = JVMTI_ERROR_NONE;
    CVMJvmtiVisitStack *stkp = &CVMglobals.jvmti.statics.currentStack;
    unsigned char *newStack;

    if (jvmtiCheckMarkAndAdd(stkp->env, obj)) {
	return err;
    }
    if (stkp->stackPtr >= stkp->stackBase + stkp->stackSize) {
	err = jvmti_Allocate(stkp->env,
		(stkp->stackSize + VISIT_STACK_INCR) * sizeof(CVMObject*),
		&newStack);
	if (err != JVMTI_ERROR_NONE) {
	    return err;
	}
	memcpy(newStack, stkp->stackBase,
	       stkp->stackSize * sizeof(CVMObject *));
	stkp->stackPtr = (CVMObject**)newStack + stkp->stackSize;
	jvmti_Deallocate(stkp->env, (unsigned char*)stkp->stackBase);
	stkp->stackBase = (CVMObject **)newStack;
	stkp->stackSize = stkp->stackSize + VISIT_STACK_INCR;
    }
    *stkp->stackPtr = obj;
    stkp->stackPtr++;
    return err;
}

static CVMBool
jvmtiVisitStackPop(CVMObject **obj)
{
    CVMJvmtiVisitStack *currentStack = &CVMglobals.jvmti.statics.currentStack;

    if (currentStack->stackPtr <= currentStack->stackBase) {
	return CVM_FALSE;
    }
    currentStack->stackPtr--;
    *obj = *currentStack->stackPtr;
    return CVM_TRUE;
}

void
jvmtiCleanupMarked() {
    int i;
    CVMJvmtiTagNode *node, *next;
    CVMJvmtiTagNode **romObjects = CVMglobals.jvmti.statics.romObjects;

    for (i = 0; i < HASH_SLOT_COUNT; i++) {
	node = romObjects[i];
	while (node != NULL) {
	    next = node->next;
	    CVMjvmtiDeallocate((unsigned char *)node);
	    node = next;
	}
        romObjects[i] = NULL;
    }
}

static jvmtiError JNICALL
jvmti_FollowReferences(jvmtiEnv* jvmtienv,
		       jint heapFilter,
		       jclass klass,
		       jobject initialObject,
		       const jvmtiHeapCallbacks* callbacks,
		       const void* userData)
{
    jvmtiError err = JVMTI_ERROR_NONE;
    CVMObject *obj;
    CVMExecEnv *ee = CVMgetEE();
    unsigned char *newStack;
    CVMJvmtiDumpContext dumpContext;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CVMJvmtiVisitStack *currentStack = &CVMglobals.jvmti.statics.currentStack;
    jint threadCount;
    jthread *threads;

    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);

    memset((void *)currentStack, 0, sizeof(*currentStack));

    ACQUIRE_SYS_MUTEXES
    if ((err = suspendAllThreads(jvmtienv, ee, &threads,
                                 &threadCount)) != JVMTI_ERROR_NONE) {
	jvmti_Deallocate(jvmtienv, (unsigned char*)threads);
	CVMjniPopLocalFrame(env, NULL);
        RELEASE_SYS_MUTEXES
	return err;
    }
    /* jit, heap and thread lock held above */
    /* Lock out the GC. (All threads suspended at this point anyway) */
    CVMlocksForGCAcquire(ee);

    CVMjvmtiSetGCOwner(CVM_TRUE);

    err = jvmti_Allocate(jvmtienv, VISIT_STACK_INIT * sizeof(CVMObject *),
			 &newStack);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanupAndExit;
    }
    currentStack->stackPtr = currentStack->stackBase = (CVMObject **)newStack;
    currentStack->stackSize = VISIT_STACK_INIT;
    currentStack->env = jvmtienv;
    dumpContext.heapFilter = heapFilter;
    dumpContext.klass = klass;
    dumpContext.callbacks = callbacks;
    dumpContext.userData = userData;

    if (initialObject == NULL) {
	/* scan all roots */
	if (CVMgcEnsureStackmapsForRootScans(ee)) {
	    CVMsysMutexLock(ee, &CVMglobals.threadLock);
	    if (CVMjvmtiScanRoots(ee, &dumpContext) == CVM_FALSE) {
		CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
		goto cleanupAndExit;
	    }
	    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
	} else {
	    /* We should always be able to compute the stackmaps: */
	    CVMassert(CVM_FALSE);
	}
    } else {
	obj = CVMjvmtiGetICellDirect(ee, initialObject);
	CVMjvmtiVisitStackPush(obj);
    }
    while(currentStack->stackPtr > currentStack->stackBase) {
	if (!jvmtiVisitStackPop(&obj)) {
	    goto cleanupAndExit;
	}
	if (CVMjvmtiDumpObject(obj, &dumpContext) == CVM_FALSE) {
	    err = JVMTI_ERROR_OUT_OF_MEMORY;
	    goto cleanupAndExit;
	}
    }
cleanupAndExit:
     resumeAllThreads(jvmtienv, ee, threads, threadCount);

    CVMjvmtiSetGCOwner(CVM_FALSE);
    CVMlocksForGCRelease(ee);

    RELEASE_SYS_MUTEXES
    jvmtiCleanupMarked();
    if (currentStack->stackBase != NULL) {
	jvmti_Deallocate(jvmtienv, (unsigned char*)currentStack->stackBase);
    }
    return err;
}


static jvmtiError JNICALL
jvmti_IterateThroughHeap(jvmtiEnv* jvmtienv,
			 jint heapFilter,
			 jclass klass,
			 const jvmtiHeapCallbacks* callbacks,
			 const void* userData) {
    CVMExecEnv *ee = CVMgetEE();
    CVMJvmtiDumpContext dumpContext;
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    jvmtiError err = JVMTI_ERROR_NONE;
    jint threadCount;
    jthread *threads;

    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);

    /* suspend all threads */

    ACQUIRE_SYS_MUTEXES

    if ((err = suspendAllThreads(jvmtienv, ee, &threads,
                                 &threadCount)) != JVMTI_ERROR_NONE) {
	jvmti_Deallocate(jvmtienv, (unsigned char*)threads);
	CVMjniPopLocalFrame(env, NULL);
        RELEASE_SYS_MUTEXES
	return err;
    }
    /* heap jit and thread lock held above */

    /* And get the rest of the GC locks: */
    CVMlocksForGCAcquire(ee);

    CVMjvmtiSetGCOwner(CVM_TRUE);

    dumpContext.heapFilter = heapFilter;
    dumpContext.klass = klass;
    dumpContext.callbacks = callbacks;
    dumpContext.userData = userData;
    CVMgcimplIterateHeap(ee, CVMjvmtiIterateDoObject,
			 (void *)&dumpContext);

    resumeAllThreads(jvmtienv, ee, threads, threadCount);
    CVMjvmtiSetGCOwner(CVM_FALSE);
    CVMlocksForGCRelease(ee);

    RELEASE_SYS_MUTEXES
    return err;
}


static jvmtiError JNICALL
jvmti_GetTag(jvmtiEnv* jvmtienv,
	     jobject object,
	     jlong* tagPtr) {

    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);

    return CVMjvmtiTagGetTag(object, tagPtr, CVM_FALSE);
}


static jvmtiError JNICALL
jvmti_SetTag(jvmtiEnv* jvmtienv,
	     jobject object,
	     jlong tag) {

    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);

    if (object == NULL || CVMID_icellIsNull(object)) {
	return JVMTI_ERROR_INVALID_OBJECT;
    }
    return CVMjvmtiTagSetTag(object, tag, CVM_FALSE);
}

void
jvmtiTagCallback(jobject obj, jlong tag, void *data)
{
    (*(jlong *)data)++;
}

void
jvmtiTagCountCallback(jobject obj, jlong tag, void *data)
{
    (*(jlong *)data)++;
}


static jvmtiError JNICALL
jvmti_GetObjectsWithTags(jvmtiEnv* jvmtienv,
			 jint tagCount,
			 const jlong* tags,
			 jint* countPtr,
			 jobject** objectResultPtr,
			 jlong** tagResultPtr) {

    int count;
    CVMExecEnv *ee = CVMgetEE();
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(countPtr);
    NOT_NULL(tags);

    if (tagCount < 0) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }
    *countPtr = 0;

    count = CVMjvmtiGetObjectsWithTag(env, tags, tagCount, NULL, NULL);
    if (count == 0) {
	return JVMTI_ERROR_NONE;
    }
    if (objectResultPtr != NULL) {
	ALLOC(jvmtienv, count * sizeof(jobject), objectResultPtr);
    }
    if (tagResultPtr != NULL) {
	ALLOC(jvmtienv, count * sizeof(jlong), tagResultPtr);
    }
    count = CVMjvmtiGetObjectsWithTag(env, tags, tagCount, objectResultPtr,
				      tagResultPtr);
    *countPtr = count;
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_ForceGarbageCollection(jvmtiEnv* jvmtienv) {

    CVMExecEnv *ee = CVMgetEE();

    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    CVMassert (ee != NULL);
    CVMassert(!CVMgcIsGCThread(ee));
    CVMgcRunGC(ee);
    return JVMTI_ERROR_NONE;
}


/*
 * Heap (1.0) functions
 */ 

static jvmtiError JNICALL
jvmti_IterateOverObjectsReachableFromObject(jvmtiEnv* env,
    jobject object, jvmtiObjectReferenceCallback objectReferenceCallback,
    const void* userData)
{
    return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_IterateOverReachableObjects(jvmtiEnv* env,
    jvmtiHeapRootCallback heapRootCallback,
    jvmtiStackReferenceCallback stackRefCallback,
    jvmtiObjectReferenceCallback objectRefCallback,
    const void* userData)
{
    return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_IterateOverHeap(jvmtiEnv* env,
		      jvmtiHeapObjectFilter objectFilter,
		      jvmtiHeapObjectCallback heapObjectCallback,
		      const void* userData)
{
    return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_IterateOverInstancesOfClass(jvmtiEnv* env,
				  jclass klass,
				  jvmtiHeapObjectFilter objectFilter,
				  jvmtiHeapObjectCallback heapObjectCallback,
				  const void* userData)
{
    return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Local Variable functions
 */ 

/* mlam :: REVIEW DONE */
/* Return via "siPtr" the stack item at the specified slot.
 * Errors:
 * JVMTI_ERROR_NO_MORE_FRAMES, JVMTI_ERROR_INVALID_SLOT,
 * JVMTI_ERROR_TYPE_MISMATCH, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OPAQUE_FRAME. Must be called from GC-unsafe code and is
 * guaranteed not to become GC-safe internally. (Stack pointers are
 * invalid across GC-safe points.)
 */
static jvmtiError
jvmtiGCUnsafeGetSlot(CVMFrameIterator *iter, jint slot, CVMJavaVal32 **siPtr)
{
    CVMSlotVal32* vars = NULL;
    CVMMethodBlock* mb;
    CVMFrame *frame = CVMframeIterateGetFrame(iter);

    /* If the frame isn't an interpreted frame, then we can't read it: */
    if (!CVMframeIsJava(frame)) {
	return JVMTI_ERROR_OPAQUE_FRAME;
    }

    mb = frame->mb;
    vars = CVMframeLocals(frame);
    if (mb == NULL || vars == NULL) {
	return JVMTI_ERROR_NO_MORE_FRAMES;
    }
    if (CVMmbIs(mb, NATIVE)) {
	return JVMTI_ERROR_OPAQUE_FRAME;
    }
    if (slot >= CVMmbMaxLocals(mb)) {
	return JVMTI_ERROR_INVALID_SLOT;
    }
    *siPtr = &vars[slot].j;
    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
/* Return via "valuePtr" the value of the local variable at the
 * specified slot.
 * Errors: JVMTI_ERROR_INVALID_SLOT, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OPAQUE_FRAME, JVMTI_ERROR_OUT_OF_MEMORY
 */
static jvmtiError JNICALL
jvmti_GetLocalObject(jvmtiEnv *jvmtienv,
		     jthread thread,
		     jint depth,
		     jint slot,
		     jobject *valuePtr)
{
    CVMFrameIterator iter;
    CVMJavaVal32* si;
    jvmtiError err = JVMTI_ERROR_NONE;
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *targetEE;
    jobject result;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(valuePtr);
    CHECK_CAP(can_access_local_variables);

    *valuePtr = NULL;
    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    result = CVMjniCreateLocalRef(ee);
    if (result == NULL) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }

    /* Note: we need to lock the heapLock here because we'll be manipulating
       some direct object pointers below without going GC unsafe.  The heapLock
       prevents GC from running and prevents the objects pointer from changing
       on us unsuspectingly. */
    CVMsysMutexLock(ee, &CVMglobals.heapLock);
    STOP_STACK_MUTATION(ee, targetEE);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    err = jvmtiGCUnsafeGetSlot(&iter, slot, &si);
    if (err == JVMTI_ERROR_NONE && !CVMID_icellIsNull(&si->r)) {
	CVMjvmtiAssignICellDirect(ee, result, &si->r);
	*valuePtr = result;
    }

 cleanUpAndReturn:
    RESUME_STACK_MUTATION(ee, targetEE);
    CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
    /* local ref on stack of caller and gets flushed when that
     * local frame goes away
     */
    return err;
}

/* mlam :: REVIEW DONE */
/* Return via "valuePtr" the value of the local variable at the
 * specified slot.
 * Errors: JVMTI_ERROR_INVALID_SLOT, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OPAQUE_FRAME, JVMTI_ERROR_NO_MORE_FRAMES
 */
static jvmtiError JNICALL
jvmti_GetLocalInt(jvmtiEnv *jvmtienv,
		  jthread thread,
		  jint depth,
		  jint slot,
		  jint *valuePtr)
{
    CVMFrameIterator iter;
    CVMJavaVal32 *si;
    jvmtiError err;
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *targetEE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(valuePtr);
    CHECK_CAP(can_access_local_variables);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    STOP_STACK_MUTATION(ee, targetEE);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    err = jvmtiGCUnsafeGetSlot(&iter, slot, &si);
    if (err == JVMTI_ERROR_NONE) {
	*valuePtr = (jint)si->i;
    }

 cleanUpAndReturn:
    RESUME_STACK_MUTATION(ee, targetEE);
    return err;
}

/* mlam :: REVIEW DONE */
/* Return via "valuePtr" the value of the local variable at the
 * specified slot.
 * Errors: JVMTI_ERROR_INVALID_SLOT, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OPAQUE_FRAME, JVMTI_ERROR_NO_MORE_FRAMES
 */
static jvmtiError JNICALL
jvmti_GetLocalLong(jvmtiEnv *jvmtienv,
		   jthread thread,
		   jint depth,
		   jint slot,
		   jlong *valuePtr)
{
    CVMFrameIterator iter;
    CVMJavaVal32 *si;
    jvmtiError err;
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *targetEE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(valuePtr);
    CHECK_CAP(can_access_local_variables);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    STOP_STACK_MUTATION(ee, targetEE);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    err = jvmtiGCUnsafeGetSlot(&iter, slot, &si);
    if (err == JVMTI_ERROR_NONE) {
	*valuePtr = CVMjvm2Long(&si->raw);
    }

 cleanUpAndReturn:
    RESUME_STACK_MUTATION(ee, targetEE);
    return err;
}

/* mlam :: REVIEW DONE */
/* Return via "valuePtr" the value of the local variable at the
 * specified slot.
 * Errors: JVMTI_ERROR_INVALID_SLOT, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OPAQUE_FRAME, JVMTI_ERROR_NO_MORE_FRAMES
 */
static jvmtiError JNICALL
jvmti_GetLocalFloat(jvmtiEnv *jvmtienv,
		    jthread thread,
		    jint depth,
		    jint slot,
		    jfloat *valuePtr)
{
    CVMFrameIterator iter;
    CVMJavaVal32 *si;
    jvmtiError err;
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *targetEE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(valuePtr);
    CHECK_CAP(can_access_local_variables);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    STOP_STACK_MUTATION(ee, targetEE);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    err = jvmtiGCUnsafeGetSlot(&iter, slot, &si);
    if (err == JVMTI_ERROR_NONE) {
	*valuePtr = (jfloat)si->f;
    }

 cleanUpAndReturn:
    RESUME_STACK_MUTATION(ee, targetEE);
    return err;
}

/* mlam :: REVIEW DONE */
/* Return via "valuePtr" the value of the local variable at the
 * specified slot.
 * Errors: JVMTI_ERROR_INVALID_SLOT, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OPAQUE_FRAME, JVMTI_ERROR_NO_MORE_FRAMES
 */
static jvmtiError JNICALL
jvmti_GetLocalDouble(jvmtiEnv *jvmtienv,
		     jthread thread,
		     jint depth,
		     jint slot,
		     jdouble *valuePtr)
{
    CVMFrameIterator iter;
    CVMJavaVal32 *si;
    jvmtiError err;
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *targetEE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(valuePtr);
    CHECK_CAP(can_access_local_variables);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    STOP_STACK_MUTATION(ee, targetEE);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    err = jvmtiGCUnsafeGetSlot(&iter, slot, &si);
    if (err == JVMTI_ERROR_NONE) {
	*valuePtr = CVMjvm2Double(&si->raw);
    }

 cleanUpAndReturn:
    RESUME_STACK_MUTATION(ee, targetEE);
    return err;
}

/* mlam :: REVIEW DONE */
/* Set the value of the local variable at the specified slot.
 * Errors: JVMTI_ERROR_INVALID_SLOT, JVMTI_ERROR_NO_MORE_FRAMES
 * JVMTI_ERROR_OPAQUE_FRAME
 */
static jvmtiError JNICALL
jvmti_SetLocalObject(jvmtiEnv *jvmtienv,
		     jthread thread,
		     jint depth,
		     jint slot,
		     jobject value)
{
    CVMFrameIterator iter;
    CVMJavaVal32 *si;
    CVMExecEnv *ee = CVMgetEE();
    jvmtiError err;
    CVMExecEnv *targetEE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    CHECK_CAP(can_access_local_variables);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    /* Note: we need to lock the heapLock here because we'll be manipulating
       some direct object pointers below without going GC unsafe.  The heapLock
       prevents GC from running and prevents the objects pointer from changing
       on us unsuspectingly. */
    CVMsysMutexLock(ee, &CVMglobals.heapLock);
    STOP_STACK_MUTATION(ee, targetEE);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    err = jvmtiGCUnsafeGetSlot(&iter, slot, &si);
    if (err == JVMTI_ERROR_NONE) {
	if (value != NULL) {
	    CVMjvmtiAssignICellDirect(ee, &si->r, value);
	} else {
	    CVMID_icellSetNull(&si->r);
	}
    }

 cleanUpAndReturn:
    RESUME_STACK_MUTATION(ee, targetEE);
    CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
    return err;
}

/* mlam :: REVIEW DONE */
/* Set the value of the local variable at the specified slot.
 * Errors: JVMTI_ERROR_INVALID_FRAMEID, JVMTI_ERROR_INVALID_SLOT,
 * JVMTI_ERROR_TYPE_MISMATCH, JVMTI_ERROR_OPAQUE_FRAME
 */
static jvmtiError JNICALL
jvmti_SetLocalInt(jvmtiEnv *jvmtienv,
		  jthread thread,
		  jint depth,
		  jint slot,
		  jint value)
{
    CVMFrameIterator iter;
    CVMJavaVal32 *si;
    CVMExecEnv *ee = CVMgetEE();
    jvmtiError err;
    CVMExecEnv *targetEE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    CHECK_CAP(can_access_local_variables);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    STOP_STACK_MUTATION(ee, targetEE);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    err = jvmtiGCUnsafeGetSlot(&iter, slot, &si);
    if (err == JVMTI_ERROR_NONE) {
	si->i = (CVMJavaInt)value;
    }

 cleanUpAndReturn:
    RESUME_STACK_MUTATION(ee, targetEE);
    return err;
}

/* mlam :: REVIEW DONE */
/* Set the value of the local variable at the specified slot.
 * Errors: JVMTI_ERROR_INVALID_FRAMEID, JVMTI_ERROR_INVALID_SLOT,
 * JVMTI_ERROR_TYPE_MISMATCH, JVMTI_ERROR_OPAQUE_FRAME
 */
static jvmtiError JNICALL
jvmti_SetLocalLong(jvmtiEnv *jvmtienv,
		   jthread thread,
		   jint depth,
		   jint slot,
		   jlong value)
{
    CVMFrameIterator iter;
    CVMJavaVal32* si;
    jvmtiError err;
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv *targetEE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    CHECK_CAP(can_access_local_variables);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    STOP_STACK_MUTATION(ee, targetEE);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    err = jvmtiGCUnsafeGetSlot(&iter, slot, &si);
    if (err == JVMTI_ERROR_NONE) {
	CVMlong2Jvm(&si->raw, value);
    }

 cleanUpAndReturn:
    RESUME_STACK_MUTATION(ee, targetEE);
    return err;	 
}

/* mlam :: REVIEW DONE */
/* Set the value of the local variable at the specified slot.
 * Errors: JVMTI_ERROR_INVALID_FRAMEID, JVMTI_ERROR_INVALID_SLOT,
 * JVMTI_ERROR_TYPE_MISMATCH, JVMTI_ERROR_OPAQUE_FRAME
 */
static jvmtiError JNICALL
jvmti_SetLocalFloat(jvmtiEnv *jvmtienv,
		    jthread thread,
		    jint depth,
		    jint slot,
		    jfloat value)
{
    CVMFrameIterator iter;
    CVMJavaVal32 *si;
    jvmtiError err;
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *targetEE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    CHECK_CAP(can_access_local_variables);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    STOP_STACK_MUTATION(ee, targetEE);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    err = jvmtiGCUnsafeGetSlot(&iter, slot, &si);
    if (err == JVMTI_ERROR_NONE) {
	si->f = value;
    }

 cleanUpAndReturn:
    RESUME_STACK_MUTATION(ee, targetEE);
    return err;
}

/* mlam :: REVIEW DONE */
/* Set the value of the local variable at the specified slot.
 * Errors: JVMTI_ERROR_INVALID_FRAMEID, JVMTI_ERROR_INVALID_SLOT,
 * JVMTI_ERROR_TYPE_MISMATCH, JVMTI_ERROR_OPAQUE_FRAME
 */
static jvmtiError JNICALL
jvmti_SetLocalDouble(jvmtiEnv *jvmtienv,
		     jthread thread,
		     jint depth,
		     jint slot,
		     jdouble value)
{
    CVMFrameIterator iter;
    CVMJavaVal32 *si;
    jvmtiError err;
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *targetEE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    CHECK_CAP(can_access_local_variables);

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    STOP_STACK_MUTATION(ee, targetEE);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    err = jvmtiGCUnsafeGetSlot(&iter, slot, &si);
    if (err == JVMTI_ERROR_NONE) {
	CVMdouble2Jvm(&si->raw, value);
    }

 cleanUpAndReturn:
    RESUME_STACK_MUTATION(ee, targetEE);
    return err;	 
}


/*
 * Breakpoint functions
 */ 

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_SetBreakpoint(jvmtiEnv* jvmtienv,
		    jmethodID method,
		    jlocation location)
{
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMUint8* code;
    CVMInt32 bciInt;
    CVMUint8* pc;
    jvmtiError err;
    CVMExecEnv* ee = CVMgetEE();
    CVMJavaLong sz;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    THREAD_OK(ee);
    METHOD_ID_OK(mb);

    /* TODO: This may not be the right error to return for this case.
       However, OPAQUE_METHOD doesn't appear to be one of the options
       for an error.  Need to consider the options.
     */
    if (CVMmbIs(mb, NATIVE)) {
	return JVMTI_ERROR_INVALID_LOCATION;
    }

    /* TODO: This may not be the right error to return for this case.
       However, OPAQUE_METHOD doesn't appear to be one of the options
       for an error.  Need to consider the options.
     */
    /* The jmd for <clinit> gets freed after it executes. Can't take
       any chances. */
    if (CVMmbJmd(mb) == NULL) {
	return JVMTI_ERROR_INVALID_METHODID;
    }

    code = CVMmbJavaCode(mb);
    CVMassert(code != NULL);

    bciInt = CVMlong2Int(location);
    pc = code + bciInt;

    sz = CVMint2Long(CVMmbCodeLength(mb));
    if (CVMlongLtz(location) || CVMlongGe(location,sz)) {
	return JVMTI_ERROR_INVALID_LOCATION;
    }

    JVMTI_LOCK(ee);
    /* TODO: Review how bag works. */
    if (CVMbagFind(CVMglobals.jvmti.statics.breakpoints, pc) == NULL) {
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	jobject methodClassRef = CVMcbJavaInstance(CVMmbClassBlock(mb));
	jobject classRef = (*env)->NewGlobalRef(env, methodClassRef);

	if (classRef == NULL) {
	    err = JVMTI_ERROR_OUT_OF_MEMORY;
	} else {
	    struct bkpt *bp = CVMbagAdd(CVMglobals.jvmti.statics.breakpoints);
	    if (bp == NULL) {
		(*env)->DeleteGlobalRef(env, classRef);
		err = JVMTI_ERROR_OUT_OF_MEMORY;
	    } else {
		bp->pc = pc;
		/* TODO: Do we need the CODE_LOCK here? Check interplay of
		   CODE_LOCK and jvmtiLock. */
		bp->opcode = *pc;
		/* Keep a reference to the class to prevent gc */
		bp->classRef = classRef;
		*pc = opc_breakpoint;
#ifdef CVM_HW
		CVMhwFlushCache(pc, pc + 1);
#endif
		err = JVMTI_ERROR_NONE;
	    }
	}
    } else {
	err = JVMTI_ERROR_DUPLICATE;
    }
    JVMTI_UNLOCK(ee);	  

    return err;
}

/* mlam :: REVIEWING */
static jvmtiError JNICALL
jvmti_ClearBreakpoint(jvmtiEnv* jvmtienv,
		      jmethodID method,
		      jlocation location)
{
    struct bkpt *bp;
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMUint8* code;
    CVMInt32 bciInt;
    CVMUint8* pc;
    jvmtiError err;
    CVMExecEnv* ee = CVMgetEE();
    CVMJavaLong sz;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    THREAD_OK(ee);

    METHOD_ID_OK(mb);
    /* TODO: This may not be the right error to return for this case.
       However, OPAQUE_METHOD doesn't appear to be one of the options
       for an error.  Need to consider the options.
     */
    if (CVMmbIs(mb, NATIVE)) {
	return JVMTI_ERROR_INVALID_METHODID;
    }

    /* TODO: This may not be the right error to return for this case.
       However, OPAQUE_METHOD doesn't appear to be one of the options
       for an error.  Need to consider the options.
     */
    /* The jmd for <clinit> gets freed after it executes. Can't take
       any chances. (NOTE: this is probably a bug because you can
       never clear out a breakpoint set in a <clinit> method after
       it's run.) */
    if (CVMmbJmd(mb) == NULL) {
	return JVMTI_ERROR_INVALID_METHODID;
    }

    code = CVMmbJavaCode(mb);
    CVMassert(code != NULL);

    bciInt = CVMlong2Int(location);
    pc = code + bciInt;

    sz = CVMint2Long(CVMmbCodeLength(mb));
    if (CVMlongLtz(location) || CVMlongGe(location,sz)) {
	return JVMTI_ERROR_INVALID_LOCATION;
    }

    JVMTI_LOCK(ee);
    /* TODO: Review how bag works. */
    bp = CVMbagFind(CVMglobals.jvmti.statics.breakpoints, pc);
    if (bp == NULL) {
	err = JVMTI_ERROR_NOT_FOUND;
    } else {
	clearBkpt(ee, bp);
	CVMbagDelete(CVMglobals.jvmti.statics.breakpoints, bp);
	err = JVMTI_ERROR_NONE;
    }
    JVMTI_UNLOCK(ee);

    return err;
}


/*
 * Watched Field functions
 */ 

/* Set a field watch. */
static jvmtiError
setFieldWatch(jclass clazz, jfieldID field, 
	      struct CVMBag* watchedFields, CVMBool *ptrWatching)
{
    CVMFieldBlock* fb = (CVMFieldBlock*) field;
    jvmtiError err;
    CVMExecEnv* ee = CVMgetEE();

    JVMTI_ENABLED();
    THREAD_OK(ee);

    JVMTI_LOCK(ee);
    if (CVMbagFind(watchedFields, fb) == NULL) {
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	jobject classRef = (*env)->NewGlobalRef(env, clazz);

	if (classRef == NULL) {
	    err = JVMTI_ERROR_OUT_OF_MEMORY;
	} else {
	    struct fieldWatch *fw = CVMbagAdd(watchedFields);
	    if (fw == NULL) {
		(*env)->DeleteGlobalRef(env, classRef);
		err = JVMTI_ERROR_OUT_OF_MEMORY;
	    } else {
		fw->fb = fb;
		/* Keep a reference to the class to prevent gc */
		fw->classRef = classRef;
		/* reset the global flag */
		*ptrWatching = CVM_TRUE;
		err = JVMTI_ERROR_NONE;
	    }
	}
    } else {
	err = JVMTI_ERROR_DUPLICATE;
    }
    JVMTI_UNLOCK(ee);	  

    return err;
}

/* Clear a field watch. */
static jvmtiError
clearFieldWatch(jclass clazz, jfieldID field, 
		struct CVMBag* watchedFields, CVMBool *ptrWatching)
{
    CVMFieldBlock* fb = (CVMFieldBlock*) field;
    jvmtiError err;
    CVMExecEnv* ee = CVMgetEE();
    struct fieldWatch *fw;

    JVMTI_ENABLED();
    THREAD_OK(ee);

    JVMTI_LOCK(ee);
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
	err = JVMTI_ERROR_NONE;
    } else {
	err = JVMTI_ERROR_NOT_FOUND;
    }
    JVMTI_UNLOCK(ee);

    return err;
}

static jvmtiError JNICALL
jvmti_SetFieldAccessWatch(jvmtiEnv* jvmtienv,
			  jclass klass,
			  jfieldID field) {
    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(field, JVMTI_ERROR_INVALID_FIELDID);
    return setFieldWatch(klass, field, 
			 CVMglobals.jvmti.statics.watchedFieldAccesses, 
			 &CVMglobals.jvmti.isWatchingFieldAccess);
}


static jvmtiError JNICALL
jvmti_ClearFieldAccessWatch(jvmtiEnv* jvmtienv,
			    jclass klass,
			    jfieldID field) {
    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(field, JVMTI_ERROR_INVALID_FIELDID);
    return clearFieldWatch(klass, field, 
			   CVMglobals.jvmti.statics.watchedFieldAccesses, 
			   &CVMglobals.jvmti.isWatchingFieldAccess);
}


static jvmtiError JNICALL
jvmti_SetFieldModificationWatch(jvmtiEnv* jvmtienv,
				jclass klass,
				jfieldID field) {
    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(field, JVMTI_ERROR_INVALID_FIELDID);
    return setFieldWatch(klass, field, 
			 CVMglobals.jvmti.statics.watchedFieldModifications, 
			 &CVMglobals.jvmti.isWatchingFieldModification);
}


static jvmtiError JNICALL
jvmti_ClearFieldModificationWatch(jvmtiEnv* jvmtienv,
				  jclass klass,
				  jfieldID field) {
    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(field, JVMTI_ERROR_INVALID_FIELDID);
    return clearFieldWatch(klass, field, 
	       CVMglobals.jvmti.statics.watchedFieldModifications, 
	       &CVMglobals.jvmti.isWatchingFieldModification);
}


/*
 * Class functions
 */ 

typedef struct {
    int numClasses;
    int i;
    jclass *classes;
    jvmtiError err;
    jvmtiEnv *jvmtienv;
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
	if (!CVMcbIs(cb, PRIMITIVE)) {
	    ((LoadedClassesUserData *) arg)->numClasses++;
	}
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
	if (data->err == JVMTI_ERROR_NONE) {
	    if (!CVMcbIs(cb, PRIMITIVE)) {
		data->classes[data->i] =
		    (*env)->NewLocalRef(env, CVMcbJavaInstance(cb));
		data->i++;
	    }
	}
    }
}

/* Return via "classesPtr" all classes currently loaded in the VM
 * ("classCountPtr" returns the number of such classes).
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetLoadedClasses(jvmtiEnv* jvmtienv,
		       jint* classCountPtr,
		       jclass** classesPtr) {
    LoadedClassesUserData *data;
    CVMExecEnv* ee = CVMgetEE();
    CVMJavaLong sz;
    jvmtiError rc;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    THREAD_OK(ee);
    NOT_NULL2(classCountPtr, classesPtr);

    data = malloc(sizeof(LoadedClassesUserData));
    data->numClasses = 0;
    data->i = 0;
    data->err = JVMTI_ERROR_NONE;
    /* Give it a safe value in case we have to abort */
    *classCountPtr = 0;

    /* Seize classTableLock so nothing gets added while we're counting. */
    CVM_CLASSTABLE_LOCK(ee);

    CVMclassIterateAllClasses(ee, &LoadedClassesCountCallback, data);
    sz = CVMint2Long(data->numClasses * sizeof(jclass));
    rc = jvmti_Allocate(jvmtienv, sz, (unsigned char**)classesPtr);
    if (rc == JVMTI_ERROR_NONE) {
	data->classes = *classesPtr;
	CVMclassIterateAllClasses(ee, &LoadedClassesGetCallback, data);
	rc = data->err;
    }

    if (rc != JVMTI_ERROR_NONE) {
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
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetClassLoaderClasses(jvmtiEnv* jvmtienv,
			    jobject initiatingLoader,
			    jint* classCountPtr,
			    jclass** classesPtr) {
    jvmtiError rc = JVMTI_ERROR_NONE;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;	  
    CVMLoaderCacheIterator iter;
    int c, n;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    THREAD_OK(ee);
    NOT_NULL2(classCountPtr, classesPtr);

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
	    if (!CVMcbIs(CVMloaderCacheIterateGetCB(ee, &iter), PRIMITIVE)) {
		++n;
	    }
	}
    }

    rc = jvmti_Allocate(jvmtienv, CVMint2Long(n * sizeof(jclass)),
			(unsigned char**)classesPtr);
    *classCountPtr = n;
    c = 0;
    if (rc == JVMTI_ERROR_NONE) {
	/* fill in the classes */
	CVMloaderCacheIterate(ee, &iter);
	while (CVMloaderCacheIterateNext(ee, &iter)) {
	    CVMObjectICell *loader = CVMloaderCacheIterateGetLoader(ee, &iter);
	    CVMBool same;
	    CVMID_icellSameObjectNullCheck(ee, initiatingLoader, loader, same);
	    if (same) {
		CVMClassBlock *cb;
		cb = CVMloaderCacheIterateGetCB(ee, &iter);
		if (!CVMcbIs(cb, PRIMITIVE)) {
		    (*classesPtr)[c] =
			(*env)->NewLocalRef(env, CVMcbJavaInstance(cb));
		    ++c;
		}
	    }
	}
    }

    if (c < n) {
	jvmti_Deallocate(jvmtienv, (unsigned char *)*classesPtr);
	rc = JVMTI_ERROR_OUT_OF_MEMORY;
    }
    CVM_LOADERCACHE_UNLOCK(ee);

    return rc;
}


static jvmtiError JNICALL
jvmti_GetClassSignature(jvmtiEnv* jvmtienv,
			jclass klass,
			char** signaturePtr,
			char** genericPtr)
{
    CVMExecEnv *ee = CVMgetEE();
    char *name;
    int len;
    CVMJavaLong longLen;
    CVMClassBlock* cb;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);

    cb = CVMjvmtiClassRef2ClassBlock(ee, klass);
    VALID_CLASS(cb);

    if (genericPtr != NULL) {
	*genericPtr = NULL;
    }
    if (signaturePtr == NULL) {
	return JVMTI_ERROR_NONE;
    }
    if (!CVMcbIs(cb, PRIMITIVE)) {
	name = CVMtypeidClassNameToAllocatedCString(CVMcbClassName(cb));
	len = strlen(name);
	if (name[0] == '[') { /* arrays are already in signature form */
	    longLen = CVMint2Long(len+1);
	    ALLOC_WITH_CLEANUP_IF_FAILED(jvmtienv, longLen, signaturePtr, {
		    free (name);
		});
	    strcpy(*signaturePtr, name);
	} else {
	    char *sig;
	    longLen = CVMint2Long(len+3);
	    ALLOC_WITH_CLEANUP_IF_FAILED(jvmtienv, longLen, signaturePtr, {
		    free (name);
		});
	    sig = *signaturePtr;
	    sig[0] = 'L';
	    strcpy(sig+1, name);
	    sig[len+1] = ';';
	    sig[len+2] = 0;
	}		 
	free(name);
    } else {
	char *sig;
	longLen = CVMint2Long(2);
	ALLOC(jvmtienv, longLen, signaturePtr);
	sig = *signaturePtr;
	sig[0] = CVMbasicTypeSignatures[CVMcbBasicTypeCode(cb)];
	sig[1] = 0;
    }
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetClassStatus(jvmtiEnv* jvmtienv,
		     jclass klass,
		     jint* statusPtr)
{
    jint state = 0;
    CVMClassBlock* cb;
    CVMExecEnv *ee = CVMgetEE();
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL(statusPtr);

    cb = CVMjvmtiClassRef2ClassBlock(ee, klass);
    VALID_CLASS(cb);

    if (CVMcbIs(cb, PRIMITIVE)) {
	state = JVMTI_CLASS_STATUS_PRIMITIVE;
    } else if (CVMisArrayClass(cb)) {
	state = JVMTI_CLASS_STATUS_ARRAY;
    } else {
	if (CVMcbCheckRuntimeFlag(cb, VERIFIED)) {
	    state |= JVMTI_CLASS_STATUS_VERIFIED;
	}

	if (CVMcbCheckRuntimeFlag(cb, LINKED)) {
	    state |= JVMTI_CLASS_STATUS_PREPARED;
	}

	if (CVMcbInitializationDoneFlag(ee, cb)) {
	    state |= JVMTI_CLASS_STATUS_INITIALIZED;
	}

	if (CVMcbCheckErrorFlag(ee, cb)) {
	    state |= JVMTI_CLASS_STATUS_ERROR;
	}
    }
    *statusPtr = state;
    return JVMTI_ERROR_NONE;
}

/* Return via "sourceName_Ptr" the class's source path (UTF8).
 * Errors: JVMTI_ERROR_OUT_OF_MEMORY, JVMTI_ERROR_ABSENT_INFORMATION
 */

static jvmtiError JNICALL
jvmti_GetSourceFileName(jvmtiEnv* jvmtienv,
			jclass klass,
			char** sourceNamePtr)
{
    CVMExecEnv *ee = CVMgetEE();
    char *srcName;
    CVMClassBlock* cb;
    CVMJavaLong sz;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);

    cb = CVMjvmtiClassRef2ClassBlock(ee, klass);
    VALID_CLASS(cb);
    srcName = CVMcbSourceFileName(cb);
    if (srcName == NULL) {
	return JVMTI_ERROR_ABSENT_INFORMATION;
    }
    sz = CVMint2Long(strlen(srcName)+1);
    ALLOC(jvmtienv, sz, sourceNamePtr);
    strcpy(*sourceNamePtr, srcName);
    return JVMTI_ERROR_NONE;
}

/* Return via "modifiers_Ptr" the modifiers of this class.
 * See JVM Spec "accessFlags".
 * Errors: JVMTI_ERROR_CLASS_NO_PREPARED, JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetClassModifiers(jvmtiEnv* jvmtienv,
			jclass klass,
			jint* modifiersPtr)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMClassBlock *cb;
    CVMBool isArray = CVM_FALSE;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL(modifiersPtr);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);
    THREAD_OK(ee);

    cb = CVMjvmtiClassRef2ClassBlock(ee, klass);

    if (CVMisArrayClass(cb)) {
	isArray = CVM_TRUE;
	klass = JVM_GetComponentType(CVMexecEnv2JniEnv(ee), klass);
    }
    *modifiersPtr = JVM_GetClassModifiers(CVMexecEnv2JniEnv(ee), klass);
    if (isArray) {
	*modifiersPtr &= ~(JVM_ACC_INTERFACE);
	*modifiersPtr |= JVM_ACC_FINAL;
    }
    return JVMTI_ERROR_NONE;
}

/* Return via "methodsPtr" a list of the class's methods 
 * Inherited methods are not included.
 * Errors: JVMTI_ERROR_CLASS_NOT_PREPARED, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetClassMethods(jvmtiEnv* jvmtienv,
		      jclass klass,
		      jint* methodCountPtr,
		      jmethodID** methodsPtr)
{
    CVMExecEnv *ee = CVMgetEE();
    jint methodsCount;
    jmethodID *mids;
    int i;
    jint state;
    CVMClassBlock* cb; 
    CVMJavaLong sz;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL2(methodCountPtr, methodsPtr);

    cb = CVMjvmtiClassRef2ClassBlock(ee, klass);
    VALID_CLASS(cb);

    jvmti_GetClassStatus(jvmtienv, klass, &state);
    if (!CVMcbIs(cb, PRIMITIVE) && !CVMisArrayClass(cb) &&
	!(state & JVMTI_CLASS_STATUS_PREPARED)) {
	return JVMTI_ERROR_CLASS_NOT_PREPARED;
    }

    /* %comment: k015 */
    methodsCount = CVMcbMethodCount(cb);
    *methodCountPtr = methodsCount;
    sz = CVMint2Long(methodsCount*sizeof(jmethodID));
    ALLOC(jvmtienv, sz, methodsPtr);
    mids = *methodsPtr;
    for (i = 0; i < methodsCount; ++i) {
	mids[i] = CVMcbMethodSlot(cb, i);
    }
    return JVMTI_ERROR_NONE;
}

/* Return via "fieldsPtr" a list of the class's fields ("fieldCountPtr" 
 * returns the number of fields).
 * Inherited fields are not included.
 * Errors: JVMTI_ERROR_CLASS_NOT_PREPARED, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetClassFields(jvmtiEnv* jvmtienv,
		     jclass klass,
		     jint* fieldCountPtr,
		     jfieldID** fieldsPtr)
{
    CVMExecEnv *ee = CVMgetEE();
    jint fieldsCount;
    jfieldID *fids;
    int i;
    jint state;
    CVMClassBlock* cb;
    CVMJavaLong sz;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL2(fieldCountPtr, fieldsPtr);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);

    cb = CVMjvmtiClassRef2ClassBlock(ee, klass);
    VALID_CLASS(cb);
	
    jvmti_GetClassStatus(jvmtienv, klass, &state);
    if (!CVMcbIs(cb, PRIMITIVE) && !CVMisArrayClass(cb) &&
	!(state & JVMTI_CLASS_STATUS_PREPARED)) {
	return JVMTI_ERROR_CLASS_NOT_PREPARED;
    }

    fieldsCount = CVMcbFieldCount(cb);
    *fieldCountPtr = fieldsCount;
    sz = CVMint2Long(fieldsCount*sizeof(jfieldID));
    ALLOC(jvmtienv, sz, fieldsPtr);
    fids = *fieldsPtr;
    for (i = 0; i < fieldsCount; ++i) {
	fids[i] = CVMcbFieldSlot(cb, i);
    }
    return JVMTI_ERROR_NONE;
}

/* Return via "interfacesPtr" a list of the interfaces this class
 * declares ("interfaceCountPtr" returns the number of such interfaces).  
 * Errors: JVMTI_ERROR_CLASS_NOT_PREPARED, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetImplementedInterfaces(jvmtiEnv* jvmtienv,
			       jclass klass,
			       jint* interfaceCountPtr,
			       jclass** interfacesPtr)
{
    jint interfacesCount;
    int i;
    /* %comment: k016 */
    jclass *imps;
    jint state;
    CVMClassBlock* cb;   
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;	  
    CVMJavaLong sz;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL2(interfaceCountPtr, interfacesPtr);
    THREAD_OK(ee);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);

    env = CVMexecEnv2JniEnv(ee);

    cb = CVMjvmtiClassRef2ClassBlock(ee, klass);
    VALID_CLASS(cb);

    jvmti_GetClassStatus(jvmtienv, klass, &state);
    if (!(state & JVMTI_CLASS_STATUS_PREPARED)) {
	return JVMTI_ERROR_CLASS_NOT_PREPARED;
    }

    interfacesCount = CVMcbImplementsCount(cb);
    *interfaceCountPtr = interfacesCount;
    sz = CVMint2Long(interfacesCount * sizeof(jclass));
    ALLOC(jvmtienv, sz, interfacesPtr);
    imps = *interfacesPtr;
    for (i = 0; i < interfacesCount; ++i) {
	imps[i] =
	    (*env)->NewLocalRef(env,
				CVMcbJavaInstance(CVMcbInterfacecb(cb, i)));
	if (imps[i] == NULL) {
	    free(imps);
	    *interfacesPtr = 0;
	    return JVMTI_ERROR_OUT_OF_MEMORY;
	}
    }
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetClassVersionNumbers(jvmtiEnv* env,
			     jclass klass,
			     jint* minorVersionPtr,
			     jint* majorVersionPtr) {
    return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetConstantPool(jvmtiEnv* env,
		      jclass klass,
		      jint* constantPoolCountPtr,
		      jint* constantPoolByteCountPtr,
		      unsigned char** constantPoolBytesPtr) {
    return JVMTI_ERROR_ACCESS_DENIED;
}

/* Return via "isInterfacePtr" a boolean indicating whether this "klass"
 * is an interface.
 * Errors: JVMTI_ERROR_CLASS_NOT_PREPARED, JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_IsInterface(jvmtiEnv* jvmtienv,
		  jclass klass,
		  jboolean* isInterfacePtr) {
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    THREAD_OK(ee);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);

    NOT_NULL(isInterfacePtr);
    *isInterfacePtr = JVM_IsInterface(env, klass);
    return JVMTI_ERROR_NONE;
}

/* Return via "isArrayClassPtr" a boolean indicating whether this "klass"
 * is an array class.
 * Errors: 
 */

static jvmtiError JNICALL
jvmti_IsArrayClass(jvmtiEnv* jvmtienv,
		   jclass klass,
		   jboolean* isArrayClassPtr) {
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    THREAD_OK(ee);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);

    env = CVMexecEnv2JniEnv(ee);

    NOT_NULL(isArrayClassPtr);
    *isArrayClassPtr = JVM_IsArrayClass(env, klass);
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_IsModifiableClass(jvmtiEnv* jvmtienv,
			jclass klass,
			jboolean* isModifiableClassPtr)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMClassBlock* cb;
    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL(isModifiableClassPtr);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);

    cb = CVMjvmtiClassRef2ClassBlock(ee, klass);
    VALID_CLASS(cb);
    *isModifiableClassPtr = !CVMcbIsInROM(cb);
    return JVMTI_ERROR_NONE;
}

/* Return via "classloaderPtr" the class loader that created this class
 * or interface, null if this class was not created by a class loader.
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetClassLoader(jvmtiEnv* jvmtienv,
		     jclass klass,
		     jobject* classloaderPtr) {
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;	  
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL(classloaderPtr);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);

    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);
    /* JVM_GetClassLoader returns a localRef */
    *classloaderPtr = JVM_GetClassLoader(env, klass);
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetSourceDebugExtension(jvmtiEnv* env,
			      jclass klass,
			      char** sourceDebugExtensionPtr) {
    return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_RetransformClasses(jvmtiEnv* env,
			 jint classCount,
			 const jclass* classes) {
    return JVMTI_ERROR_ACCESS_DENIED;
}

#if 0
typedef struct {
    CVMClassBlock *oldCb;
    CVMClassBlock *foundCb;
    CVMBool found;
} FindClassData;

static void
findClassCallback(CVMExecEnv* ee,
		  CVMClassBlock* cb,
		  void* arg)
{
    FindClassData *data = (FindClassData*)arg;
    CVMClassBlock *oldCb;
    if (data->found) {
	return;
    }
    oldCb = data->oldCb;

    if (!CVMcbCheckErrorFlag(ee, cb)) {
	if (oldCb == cb) {
	    data->found = CVM_TRUE;
	    data->foundCb = cb;
	}
    }
}
#endif

typedef struct {
    CVMClassBlock *oldcb;
    CVMClassBlock *newcb;
} cpFixupData;

/* mlam :: REVIEW DONE */
static CVMBool
redefineClearBreakpoints(void *item, void *arg)
{
    CVMExecEnv *ee = CVMgetEE();
    JNIEnv *env = CVMexecEnv2JniEnv(ee);
    struct bkpt *bp = (struct bkpt *)item;
    jclass klass = (jclass)arg;
    if ((*env)->IsSameObject(env, klass, bp->classRef)) {
	clearBkpt(ee, bp);
	/* TODO: Review how bag works.  Will deletion unchain bp properly?
	   Will this affect the caller of redefineClearBreakpoints() who is
	   iterating the bag?
	 */
	CVMbagDelete(CVMglobals.jvmti.statics.breakpoints, bp);
    }
    return CVM_TRUE;
}

static void
fixupCallback(CVMExecEnv* ee,
	      CVMClassBlock* cb,
	      void* arg)
{
    cpFixupData *data = (cpFixupData *) arg;
    CVMClassBlock *oldcb = data->oldcb;
    CVMClassBlock *newcb = data->newcb;
    CVMMethodBlock *thisMb, *newMb;
    CVMClassBlock *thisCb;
    CVMConstantPool *cp = CVMcbConstantPool(cb);
    int i;

    if ((cp != NULL) && (CVMcpTypes(cp) != NULL)) {
        for (i = 1; i < CVMcbConstantPoolCount(cb); i++) {
            CVMConstantPoolEntryType cpType = CVMcpEntryType(cp, i);
	    if (CVMcpIsResolved(cp, i)) {
		switch(cpType) {
		case CVM_CONSTANT_MethodBlock:
		    thisMb = CVMcpGetMb(cp, i);
		    thisCb = CVMmbClassBlock(thisMb);
		    if (thisCb == oldcb) {
                        if (!CVMmbIsMiranda(thisMb)) {
                            newMb = CVMcbMethodSlot(newcb,
                                        CVMmbFullMethodIndex(thisMb));
                        } else {
                            /* Miranda method */
                            newMb = CVMcbMethodTableSlot(newcb,
                                       CVMmbMethodTableIndex(thisMb));
                        }
                        CVMcpResetMb(cp, i, newMb);
                    }
                    break;
		}
	    }
	}
    }
    /* Fixup method table as well */
    if (cb == oldcb) {
        /* no need to update the cb methodtableptr entries as we
         * replace the methodtableptr 
         */
	return;
    }

    if (CVMcbMethodTablePtr(cb) != NULL) {
	for (i = 0; i < CVMcbMethodTableCount(cb); i++) {
	    thisMb = CVMcbMethodTableSlot(cb, i);
	    if (CVMmbClassBlock(thisMb) == oldcb) {
		CVMassert(i < CVMcbMethodTableCount(newcb));
		CVMassert(CVMmbNameAndTypeID(thisMb) ==
			  CVMmbNameAndTypeID(CVMcbMethodTableSlot(newcb, i)));
		CVMcbMethodTableSlot(cb,i) = CVMcbMethodTableSlot(newcb, i);
	    }
	}
    }
}

static jvmtiError
jvmtiCheckSchemas(CVMClassBlock *oldcb, CVMClassBlock *newcb)
{
    CVMClassBlock *newSupercb, *oldSupercb, *oldIcb, *newIcb;
    CVMMethodBlock *oldmb, *newmb;
    CVMFieldBlock *oldfb, *newfb;
    int i;

    /* Check for circularity and that hierarchy is still the same */
    newSupercb = CVMcbSuperclass(newcb);
    oldSupercb = CVMcbSuperclass(oldcb);
    while (newSupercb != NULL) {
        if (newSupercb == oldcb) {
            return JVMTI_ERROR_CIRCULAR_CLASS_DEFINITION;
        }
        if (newSupercb != oldSupercb) {
            return JVMTI_ERROR_UNSUPPORTED_REDEFINITION_HIERARCHY_CHANGED;
        }
        newSupercb = CVMcbSuperclass(newSupercb);
        oldSupercb = CVMcbSuperclass(oldSupercb);
    }
    /* count direct superinterfaces */
    if (CVMcbImplementsCount(oldcb) != CVMcbImplementsCount(newcb)) {
        return JVMTI_ERROR_UNSUPPORTED_REDEFINITION_HIERARCHY_CHANGED;
    }
    for (i = 0; i < CVMcbImplementsCount(oldcb); i++) {
        oldIcb = CVMcbInterfacecb(oldcb, i);
        newIcb = CVMcbInterfacecb(newcb, i);
        if (oldIcb != newIcb) {
            return JVMTI_ERROR_UNSUPPORTED_REDEFINITION_HIERARCHY_CHANGED;
        }
    }
    /* verify number of methods/fields */
    if (CVMcbMethodCount(oldcb) < CVMcbMethodCount(newcb)) {
        return JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_ADDED;
    }
    if (CVMcbMethodCount(oldcb) > CVMcbMethodCount(newcb)) {
        return JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_DELETED;
    }
    if (CVMcbFieldCount(oldcb) != CVMcbFieldCount(newcb)) {
        return JVMTI_ERROR_UNSUPPORTED_REDEFINITION_SCHEMA_CHANGED;
    }
    /* check sig of each method/field */
    for (i = 0; i < CVMcbMethodCount(oldcb); i++) {
        CVMUint16 oldFlags, newFlags;

        oldmb = CVMcbMethodSlot(oldcb, i);
        newmb = CVMcbMethodSlot(newcb, i);
        if (CVMmbNameAndTypeID(oldmb) != CVMmbNameAndTypeID(newmb)) {
            return JVMTI_ERROR_UNSUPPORTED_REDEFINITION_SCHEMA_CHANGED;
        }
        oldFlags = CVMmbAccessFlags(oldmb);
        newFlags = CVMmbAccessFlags(newmb);
        if (oldFlags != newFlags) {
            return
                JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_MODIFIERS_CHANGED;
        }
    }
    for (i = 0; i < CVMcbFieldCount(oldcb); i++) {
        CVMUint8 oldFlags, newFlags;

        oldfb = CVMcbFieldSlot(oldcb, i);
        newfb = CVMcbFieldSlot(newcb, i);
        if (CVMfbNameAndTypeID(oldfb) != CVMfbNameAndTypeID(newfb)) {
            return JVMTI_ERROR_UNSUPPORTED_REDEFINITION_SCHEMA_CHANGED;
        }
        oldFlags = CVMfbAccessFlags(oldfb);
        newFlags = CVMfbAccessFlags(newfb);
        if (oldFlags != newFlags) {
            return
                JVMTI_ERROR_UNSUPPORTED_REDEFINITION_SCHEMA_CHANGED;
        }
    }
    return JVMTI_ERROR_NONE;
}

static jvmtiError JNICALL
jvmti_RedefineClasses(jvmtiEnv* jvmtienv,
		      jint classCount,
		      const jvmtiClassDefinition* classDefinitions)
{
    CVMExecEnv* ee = CVMgetEE();
    CVMClassBlock *oldcb, *newcb;
    char *className = NULL;
    int i, j;
    jclass klass;
    jclass *classes;
    CVMClassICell *newKlass;
    jobject loader = NULL;
    /*    FindClassData data; */
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMJvmtiThreadNode *node;
    jvmtiError err = JVMTI_ERROR_NONE;
    CVMClassICell* newClassRoot = NULL;
    jlong threadBits;
    jmethodID loaderRemoveClass;
    cpFixupData cpFixup;
    class_size_info class_size;
    jint res;
    jint threadCount;
    jthread *threads;
    CVMUint16 oldFlags, newFlags;

    CHECK_JVMTI_ENV;

    CVMextraAssert(CVMD_isgcSafe(ee));

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(classDefinitions);

    if (getCapabilities(context)->can_redefine_classes != 1) {
	return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
    }

    CVMtraceJVMTI(("JVMTI: Redefine: Start 0x%x\n", (int)ee));

    if (CVMjniPushLocalFrame(env, classCount+1) != JNI_OK) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }
    ALLOC(jvmtienv, CVMint2Long(classCount * sizeof(CVMClassBlock*)),
	  (unsigned char**)&classes);

    node = CVMjvmtiFindThread(ee, CVMcurrentThreadICell(ee));
    CVMassert(node != NULL);
    for (i = 0; i < classCount; i++) {
        char buf[200];

	klass = classDefinitions[i].klass;
	if (klass == NULL) {
	    err = JVMTI_ERROR_INVALID_CLASS;
	    goto cleanup;
	}
	oldcb = CVMjvmtiClassRef2ClassBlock(ee, klass);
	if (oldcb == NULL) {
	    err = JVMTI_ERROR_INVALID_CLASS;
	    goto cleanup;
	}
	if (classDefinitions[i].class_bytes == NULL) {
	    err = JVMTI_ERROR_NULL_POINTER;
	    goto cleanup;
	}
        /* Netbeans will sometimes issue system classes to be redefined
         * even if the system class filter is in place. We cannot redefine
         * system classes since they are read-only in romized space
         */
	if (CVMcbIsInROM(oldcb)) {
            continue;
	}
	node->oldCb = oldcb;
        className = CVMtypeidClassNameToAllocatedCString(CVMcbClassName(oldcb));
	loader = CVMcbClassLoader(oldcb);
        CVMtraceJVMTI(("JVMTI: Redefine: verify %s\n", className));
        res = CVMverifyClassFormat(className,
                                   classDefinitions[i].class_bytes,
                                   classDefinitions[i].class_byte_count,
                                   &class_size, buf, sizeof(buf), CVM_TRUE,
#ifdef CVM_DUAL_STACK
                                   CVM_FALSE,
#endif
                                   CVM_TRUE);
        if (res != JVMTI_ERROR_NONE) {
            CVMtraceJVMTI(("JVMTI: Redefine: verify error %d\n", res));
        }
	if (res == -1) { /* nomem */
	    err = JVMTI_ERROR_OUT_OF_MEMORY;
            goto cleanup;
	}
	if (res == -2) { /* format error */
	    err = JVMTI_ERROR_INVALID_CLASS_FORMAT;
            goto cleanup;
	}
	if (res == -3) { /* unsupported class version error */
            err = JVMTI_ERROR_UNSUPPORTED_VERSION;
            goto cleanup;
	}
	if (res == -4) { /* bad name */
	    err = JVMTI_ERROR_NAMES_DONT_MATCH;
            goto cleanup;
	}

	/*
	 * loader is 'NULL' since we don't want Class.java to call
	 * addClass which would create a reference to the javaInstance
	 */
        CVMtraceJVMTI(("JVMTI: Redefine: define\n"));
	newKlass =
	    CVMdefineClass(ee,
			   className,
			   loader,
			   classDefinitions[i].class_bytes,
			   classDefinitions[i].class_byte_count,
			   NULL,
			   CVM_TRUE);
	if (newKlass == NULL) {
	    /* TODO: Not sure at this point how to get appropriate error */
            CVMtraceJVMTI(("JVMTI: Redefine: define error\n"));
	    err = JVMTI_ERROR_INVALID_CLASS;
	    goto cleanup;
	}
	/* newKlass is a local ref */
	classes[i] = newKlass;
        oldFlags = CVMcbAccessFlags(oldcb);
        newFlags = CVMcbAccessFlags(CVMjvmtiClassRef2ClassBlock(ee, newKlass));
        if (((oldFlags & ~(CVM_CLASS_FLAG_DIFFS)) ^
             (newFlags & ~(CVM_CLASS_FLAG_DIFFS))) != 0) {
            err = JVMTI_ERROR_UNSUPPORTED_REDEFINITION_CLASS_MODIFIERS_CHANGED;
            goto cleanup;
        }
        free(className);
        className = NULL;
    }
    for (i = 0; i < classCount && classes[i] != NULL; i++) {
	klass = classDefinitions[i].klass;
	oldcb = CVMjvmtiClassRef2ClassBlock(ee, klass);
	newKlass = classes[i];
	newcb = CVMjvmtiClassRef2ClassBlock(ee, newKlass);
	className = CVMtypeidClassNameToAllocatedCString(CVMcbClassName(oldcb));
	/* clear all breakpoints in this classes methods */
	JVMTI_LOCK(ee);
	CVMbagEnumerateOver(CVMglobals.jvmti.statics.breakpoints,
			    redefineClearBreakpoints, (void *)klass);
	JVMTI_UNLOCK(ee);
	/* load superclasses in a non-recursive way */
	node->redefineCb = newcb;
	node->oldCb = oldcb;
	(*env)->CallVoidMethod(env, newKlass,
			   CVMglobals.java_lang_Class_loadSuperClasses);
	if (CVMexceptionOccurred(ee)) {
	    err = JVMTI_ERROR_INVALID_CLASS;
	    goto cleanup;
	}
	if (loader != NULL) {
	    /*
	     * We don't want the class loader to have a reference to the 
	     * new class so we make this call to remove it from its
             * 'classes' vector which is used to keep it from being GC'd
	     */
	    loaderRemoveClass =
		(*env)->GetMethodID(env,
		CVMcbJavaInstance(CVMsystemClass(java_lang_ClassLoader)), 
		"removeClass", "(Ljava/lang/Class;)V");

	    (*env)->CallVoidMethod(env, loader,
				   loaderRemoveClass, newKlass);
	    if (CVMexceptionOccurred(ee)) {
                (*env)->ExceptionClear(env);
	    }
	}
        if ((err = jvmtiCheckSchemas(oldcb, newcb)) != JVMTI_ERROR_NONE) {
            classes[i] = NULL; 
            free(className); 
            className = NULL; 
            node->redefineCb = NULL; 
            node->oldCb = NULL; 
            continue;
        }
	/* clear all breakpoints in this classes methods */
	JVMTI_LOCK(ee);
	CVMbagEnumerateOver(CVMglobals.jvmti.statics.breakpoints,
			    redefineClearBreakpoints, (void *)klass);
	JVMTI_UNLOCK(ee);

	newClassRoot = CVMID_getGlobalRoot(ee);
	if (newClassRoot == NULL) {
	    err = JVMTI_ERROR_OUT_OF_MEMORY;
	    goto cleanup;
	}
	CVMD_gcUnsafeExec(ee, {
	    CVMcbJavaInstance(newcb) = newClassRoot;
	    CVMID_icellAssignDirect(ee, newClassRoot, newKlass);
	    CVMassert(CVMID_icellDirect(ee, newClassRoot) ==
		      CVMID_icellDirect(ee, newKlass));
	});
	CVMtraceJVMTI(("JVMTI: Redefine: newroot 0x%x, 0x%x\n",
		       (int)newClassRoot,
		       (int)newClassRoot->ref_DONT_ACCESS_DIRECTLY));
	threadBits = CVMjvmtiGetThreadEventEnabled(ee);
	CVMjvmtiSetShouldPostAnyThreadEvent(ee,
		       threadBits & ~CLASS_PREPARE_BIT);
	if (!CVMclassLink(ee, newcb, CVM_TRUE)) {
            err = JVMTI_ERROR_INVALID_CLASS;
            if (CVMlocalExceptionOccurred(ee)) {
                CVMD_gcUnsafeExec(ee, {
                    if (CVMobjectGetClass(CVMgetLocalExceptionObj(ee)) ==
                        CVMsystemClass(java_lang_VerifyError)) {
                        err = JVMTI_ERROR_FAILS_VERIFICATION;
                    }
                });
                (*env)->ExceptionClear(env);
            }
            CVMjvmtiSetShouldPostAnyThreadEvent(ee, threadBits);
            CVMID_freeGlobalRoot(ee, newClassRoot);
#ifdef CVM_DEBUG
            if (err == JVMTI_ERROR_FAILS_VERIFICATION) {
                int fd;
                CVMUint32 oldFlags;
                char *baseName;
                char dbgName[64];
                baseName = strrchr(className,'/');
                baseName++;
                strncpy(dbgName, baseName, 58);
                strcat(dbgName, ".class");
                fd = CVMioOpen(dbgName, O_RDWR | O_CREAT | O_TRUNC, 0);
                if (fd < 0) {
                    goto cleanup;
                }
                CVMioWrite(fd, classDefinitions[i].class_bytes,
                           classDefinitions[i].class_byte_count);
                CVMioClose(fd);
                CVMconsolePrintf("JVMTI: Redefine: VERIFY ERROR in %s\n",
                                 className);
                oldFlags = CVMglobals.debugFlags;
                CVMglobals.debugFlags |= 0x200000;
                CVMcbClearRuntimeFlag(newcb, ee, VERIFIED);
                CVMclassVerify(ee, newcb, CVM_TRUE);
                CVMglobals.debugFlags = oldFlags;
                (*env)->ExceptionClear(env);
            }
#endif
            goto cleanup;
	}
	CVMjvmtiSetShouldPostAnyThreadEvent(ee, threadBits);
	CVMcbJavaInstance(newcb) = NULL;
	/* done with this global root */
	CVMID_freeGlobalRoot(ee, newClassRoot);
        free(className);
        className = NULL;
	node->redefineCb = NULL;
	node->oldCb = NULL;
    }
    for (i = 0; i < classCount && classes[i] != NULL; i++) {
        CVMClassBlockData *oldCBData;
        CVMMethodArray *oldMethods;
        CVMMethodBlock **oldMethodTablePtr;
        CVMConstantPool *oldCp;
        CVMInt32 oldCpCount;

        oldCBData = calloc(sizeof(CVMClassBlockData), 1);
        if (oldCBData == NULL) {
            err = JVMTI_ERROR_OUT_OF_MEMORY;
            goto cleanup;
        }

	klass = classDefinitions[i].klass;
	oldcb = CVMjvmtiClassRef2ClassBlock(ee, klass);
	newKlass = classes[i];
	newcb = CVMjvmtiClassRef2ClassBlock(ee, newKlass);
	className = CVMtypeidClassNameToAllocatedCString(CVMcbClassName(oldcb));
	/*
	 * Fixup all constant pools in other classes that point back to us 
	 * as well as method table entries
	 */
        /* Roll all threads to safe points. */
        CVM_NULL_CLASSLOADER_LOCK(ee);
#ifdef CVM_JIT
        CVMsysMutexLock(ee, &CVMglobals.jitLock);
#endif
        CVMsysMutexLock(ee, &CVMglobals.heapLock);
        CVM_THREAD_LOCK(ee);
        CVMD_gcBecomeSafeAll(ee);
        CVMjvmtiSetGCOwner(CVM_TRUE);
	cpFixup.oldcb = oldcb;
	cpFixup.newcb = newcb;

	CVMclassIterateDynamicallyLoadedClasses(ee, fixupCallback, &cpFixup);
	/* If we end up doing system classes then use this one */
	/* CVMclassIterateAllClasses(ee, fixupCpCallback, &cpFixup); */
	CVMcbClassLoader(newcb) = NULL;
	/* All constant pool resolved references fixed */
 #ifdef CVM_DEBUG
	{
	char *methodName;

	CVMtraceJVMTI(("JVMTI: Redefine: %s, oldcb: 0x%x, newcb 0x%x, "
		       "klass 0x%x, 0x%x\n", 
		       className, (int)oldcb, (int)newcb, (int)newKlass,
		       (int)newKlass->ref_DONT_ACCESS_DIRECTLY));
	CVMtraceJVMTI(("JVMTI: Redefine: oldCP 0x%x, newCP 0x%x\n", 
		       CVMcbConstantPool(oldcb), CVMcbConstantPool(newcb)));
	for (j = 0; j < CVMcbMethodCount(oldcb); j++) {
	    CVMMethodBlock *oldmb = CVMcbMethodSlot(oldcb, j);
	    CVMMethodBlock *newmb = CVMcbMethodSlot(newcb, j);
	    if (CVMmbIsJava(oldmb)) {
		CVMJavaMethodDescriptor* oldjmd = CVMmbJmd(oldmb);
		methodName =
		    CVMtypeidMethodNameToAllocatedCString(
			CVMmbNameAndTypeID(oldmb));
		CVMtraceJVMTI(("JVMTI:old: %s, oldMB 0x%x, mlocs %d, mcap %d\n",
			       methodName, (int)oldmb,
			       CVMjmdMaxLocals(oldjmd),
			       CVMjmdCapacity(oldjmd)));
	    }
	    if (CVMmbIsJava(newmb)) {
		CVMJavaMethodDescriptor* newjmd = CVMmbJmd(newmb);
		methodName =
		    CVMtypeidMethodNameToAllocatedCString(
		        CVMmbNameAndTypeID(newmb));
		CVMtraceJVMTI(("JVMTI:new: %s, newMB 0x%x, mlocs %d, mcap %d\n",
			       methodName, (int)newmb, CVMjmdMaxLocals(newjmd),
			       CVMjmdCapacity(newjmd)));
	    }
	}
	}
#endif
	for (j = 0; j < CVMcbMethodCount(oldcb); j++) {
	    CVMMethodBlock *oldmb = CVMcbMethodSlot(oldcb, j);
	    CVMMethodBlock *newmb = CVMcbMethodSlot(newcb, j);
	    CVMmbClassBlock(newmb) = oldcb;
	    CVMmbClassBlockInRange(newmb) = oldcb;
            CVMmbClassBlockInRange(oldmb) = newcb;
            CVMjvmtiMarkAsObsolete(oldmb);
	}
        oldMethods = CVMcbMethods(oldcb);
	CVMcbMethods(oldcb) = CVMcbMethods(newcb);
        CVMcbMethods(newcb) = oldMethods;
        oldMethodTablePtr = CVMcbMethodTablePtr(oldcb);
	CVMcbMethodTablePtr(oldcb) = CVMcbMethodTablePtr(newcb);
        CVMcbMethodTablePtr(newcb) = oldMethodTablePtr;
        oldCp = CVMcbConstantPool(oldcb);
        oldCpCount = CVMcbConstantPoolCount(oldcb);
	CVMcbConstantPool(oldcb) = CVMcbConstantPool(newcb);
	CVMcbConstantPoolCount(oldcb) = CVMcbConstantPoolCount(newcb);
	CVMcbConstantPool(newcb) = oldCp;
	CVMcbConstantPoolCount(newcb) = oldCpCount;

        newcb->oldCBData = oldCBData;
        oldCBData->currentCb = oldcb;

        /* make sure entries in new cp that point to old methods
         * are fixed up to point to the new methods.  Note that we
         * pass in oldcb as the cb to fixup AND we swap oldcb and
         * newcb in the cpFixup structure since we have already 
         * swapped method arrays above.
         */
	cpFixup.oldcb = newcb;
	cpFixup.newcb = oldcb;
        fixupCallback(ee, oldcb, &cpFixup);

        CVMjvmtiSetGCOwner(CVM_FALSE);
        CVMD_gcAllowUnsafeAll(ee);
        CVM_THREAD_UNLOCK(ee);
        CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
#ifdef CVM_JIT
        CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif
        CVM_NULL_CLASSLOADER_UNLOCK(ee);
        CVMcpResolveCbEntriesWithoutClassLoading(ee, oldcb);

#if 0
	/* NOTE: We don't know if/when old constant pool references and old mb 
	 * references will go away so we can't just free up the unused space.
	 * Need to look into doing this at some point.
	 * This worked as shown when we replaced only jmd's but now we
	 * replace mb's so we can't free up the old mb's or old jmd's
	 */

	/* Store the miranda method count where CVMclassFree() can find it.
	 * We need to do this now because accessing the last method in the
	 * method table is not allowed during class unloading because it 
	 * may refer to a superclass method that has already been freed.
	 */
	CVMcbMirandaMethodCount(newcb) = 0;
	if (!CVMcbIs(newcb, INTERFACE) && CVMcbCheckRuntimeFlag(newcb, LINKED)
	    && CVMcbMethodTableCount(newcb) != 0)
	{
	    CVMMethodBlock *mb;
	    mb = CVMcbMethodTableSlot(newcb, CVMcbMethodTableCount(newcb) - 1);
	    if ((mb != NULL) && CVMmbIsMiranda(mb) &&
		(CVMmbClassBlock(mb) == newcb)){
		CVMcbMirandaMethodCount(newcb) = CVMmbMethodIndex(mb) + 1;
	    }
	}
	CVMsysMutexLock(ee, &CVMglobals.heapLock);
	for (j = 0; j < CVMcbMethodCount(newcb); j++) {
	    CVMMethodBlock *oldmb = CVMcbMethodSlot(oldcb, j);
	    CVMMethodBlock *newmb = CVMcbMethodSlot(newcb, j);
	    CVMStackMaps *smaps;
	    if (CVMmbIsJava(oldmb)) {
		oldmb->immutX.codeX.jmd = newmb->immutX.codeX.jmd;
		if ((smaps = CVMstackmapFind(ee, oldmb)) != NULL) {
		    CVMstackmapDestroy(ee, smaps);
		}
	    }
	}
	CVMsysMutexUnlock(ee, &CVMglobals.heapLock);

	/* Free all the malloc'd memory we don't need */
	CVMclassFree(ee, newcb, CVM_TRUE);
#endif
        free(className);
        className = NULL;
    }
 cleanup:
    if (err != JVMTI_ERROR_NONE) {
        CVMtraceJVMTI(("JVMTI: Redefine: cleanup error %d\n", err));
    }
    if (className != NULL) {
        free(className);
    }
    if (node != NULL) {
	node->oldCb = NULL;
	node->redefineCb = NULL;
    }
    CVMjniPopLocalFrame(env, NULL);
    jvmti_Deallocate(jvmtienv, (unsigned char *)classes);
    return err;
}


/*
 * Object functions
 */ 

static jvmtiError JNICALL
jvmti_GetObjectSize(jvmtiEnv* jvmtienv,
		    jobject object,
		    jlong* sizePtr) {

    CVMExecEnv* ee = CVMgetEE();
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    VALID_OBJECT(object);
    NOT_NULL(sizePtr);

    CVMD_gcUnsafeExec(ee, {
	CVMObject *directObj = CVMID_icellDirect(ee, object);
	*sizePtr = (jlong)CVMobjectSize(directObj);
    });
    return JVMTI_ERROR_NONE;
}

/* Return via "hashCodePtr" a hash code that can be used in maintaining
 * hash table of object references. This function guarantees the same 
 * hash code value for a particular object throughout its life
 * Errors:
 */

static jvmtiError JNICALL
jvmti_GetObjectHashCode(jvmtiEnv* jvmtienv,
			jobject object,
			jint* hashCodePtr) {
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    VALID_OBJECT(object);
    NOT_NULL(hashCodePtr);

    *hashCodePtr = JVM_IHashCode(CVMexecEnv2JniEnv(CVMgetEE()), object);
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetObjectMonitorUsage(jvmtiEnv* jvmtienv,
			    jobject object,
			    jvmtiMonitorUsage* infoPtr) {
    return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
}


/*
 * Field functions
 */ 

/* Return via "namePtr" the field's name and via "signaturePtr" the
 * field's signature (UTF8).
 * Errors: JVMTI_ERROR_INVALID_FIELDID, 
 * JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetFieldName(jvmtiEnv* jvmtienv,
		   jclass klass,
		   jfieldID field,
		   char** namePtr,
		   char** signaturePtr,
		   char** genericPtr) {
    char *name;
    char *sig;
    CVMFieldBlock* fb = (CVMFieldBlock*) field;
    CVMJavaLong sz;

    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);

    if (klass == NULL  || CVMID_icellIsNull(klass)) {
	return JVMTI_ERROR_INVALID_CLASS;
    }
    if (fb == NULL) {
	return JVMTI_ERROR_INVALID_FIELDID;
    }

    if (genericPtr != NULL) {
	*genericPtr = NULL;
    }
    if (namePtr != NULL) {
	name = CVMtypeidFieldNameToAllocatedCString(CVMfbNameAndTypeID(fb));
	sz = CVMint2Long(strlen(name)+1);
	ALLOC(jvmtienv, sz, namePtr);
	strcpy(*namePtr, name);
	free(name);
    }
    if (signaturePtr != NULL) {
	sig = CVMtypeidFieldTypeToAllocatedCString(CVMfbNameAndTypeID(fb));
	sz = CVMint2Long(strlen(sig)+1);
	ALLOC(jvmtienv, sz, signaturePtr);
	strcpy(*signaturePtr, sig);
	free(sig);
    }

    return JVMTI_ERROR_NONE;
}

/* Return via "declaringClassPtr" the class in which this field is
 * defined.
 * Errors: JVMTI_ERROR_INVALID_FIELDID, JVMTI_ERROR_INVALID_CLASS,
 * JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetFieldDeclaringClass(jvmtiEnv* jvmtienv,
			     jclass klass,
			     jfieldID field,
			     jclass* declaringClassPtr) {
    CVMFieldBlock* fb = (CVMFieldBlock*) field;
    jobject dklass;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);

    if (klass == NULL || CVMID_icellIsNull(klass)) {
	return JVMTI_ERROR_INVALID_CLASS;
    }
    NULL_CHECK(field, JVMTI_ERROR_INVALID_FIELDID);
    NULL_CHECK(declaringClassPtr, JVMTI_ERROR_NULL_POINTER);
    THREAD_OK(ee);

    env = CVMexecEnv2JniEnv(ee);

    dklass = (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMfbClassBlock(fb)));
    *declaringClassPtr = dklass;
    return JVMTI_ERROR_NONE;
}

/* Return via "modifiersPtr" the modifiers of this field.
 * See JVM Spec "accessFlags".
 * Errors:  JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetFieldModifiers(jvmtiEnv* jvmtienv,
			jclass klass,
			jfieldID field,
			jint* modifiersPtr) {
    CVMFieldBlock* fb = (CVMFieldBlock*) field;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(klass, JVMTI_ERROR_INVALID_CLASS);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(field, JVMTI_ERROR_INVALID_FIELDID);
    NOT_NULL(modifiersPtr);

    /* %comment: k017 */
    *modifiersPtr = CVMfbAccessFlags(fb);
    /* clear synthetic bit (bit 2 - 0x4) */
    *modifiersPtr &= ~CVM_FIELD_ACC_SYNTHETIC;
    if ((*modifiersPtr & CVM_FIELD_ACC_PRIVATE) == CVM_FIELD_ACC_PRIVATE) {
	*modifiersPtr &= ~CVM_FIELD_ACC_PRIVATE;
	*modifiersPtr |= JVM_ACC_PRIVATE;
    } else if ((*modifiersPtr & CVM_FIELD_ACC_PRIVATE) ==
	       CVM_FIELD_ACC_PROTECTED) {
	*modifiersPtr &= ~CVM_FIELD_ACC_PRIVATE;
	*modifiersPtr |= JVM_ACC_PROTECTED;
    }
    return JVMTI_ERROR_NONE;
}

/* Return via "isSyntheticPtr" a boolean indicating whether the field
 * is synthetic.
 * Errors: JVMTI_ERROR_MUST_POSSESS_CAPABILITY
 */

static jvmtiError JNICALL
jvmti_IsFieldSynthetic(jvmtiEnv* env,
		       jclass klass,
		       jfieldID field,
		       jboolean* isSyntheticPtr) {
    return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
}


/*
 * Method functions
 */ 

/* Return via "namePtr" the method's name and via "signaturePtr" the
 * method's signature (UTF8).
 * Errors: JVMTI_ERROR_INVALID_METHODID
 * JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetMethodName(jvmtiEnv* jvmtienv,
		    jmethodID method,
		    char** namePtr,
		    char** signaturePtr,
		    char** genericPtr)
{
    char *name;
    char *sig;
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMJavaLong sz;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);

    METHOD_ID_OK(mb);

    if (namePtr != NULL) {
	name = CVMtypeidMethodNameToAllocatedCString(CVMmbNameAndTypeID(mb));
	sz = CVMint2Long(strlen(name)+1);
	ALLOC(jvmtienv, sz, namePtr);
	strcpy(*namePtr, name);
	free(name);
    }
    if (signaturePtr != NULL) {
	sig = CVMtypeidMethodTypeToAllocatedCString(CVMmbNameAndTypeID(mb));
	sz = CVMint2Long(strlen(sig)+1);
	ALLOC(jvmtienv, sz, signaturePtr);
	strcpy(*signaturePtr, sig);
	free(sig);
    }
    if (genericPtr != NULL) {
	*genericPtr = NULL;
    }

    return JVMTI_ERROR_NONE;
}

/* Return via "declaringClassPtr" the class in which this method is
 * defined.
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static CVMMethodBlock*
findDeclaredMethod(const CVMClassBlock* cb, CVMMethodTypeID tid,
		   CVMBool lookInTable, CVMBool lookInClass,
		   CVMBool lookInInterfaces)
{
    int i, j;
    CVMMethodBlock *mb;

    if (lookInTable) {
	for (i = 0; i < CVMcbMethodTableCount(cb); i++) {
	    mb = CVMcbMethodTableSlot(cb, i);
	    if (CVMtypeidIsSame(CVMmbNameAndTypeID(mb), tid)) {
		return mb;
	    }
	}
    }
    if (lookInClass) {
	for (i = 0; i < CVMcbMethodCount(cb); i++) {
	    mb = CVMcbMethodSlot(cb, i);
	    if (CVMtypeidIsSame(CVMmbNameAndTypeID(mb), tid)) {
		return mb;
	    }
	}
    }
    if (lookInInterfaces) {
	for (i = 0; i < CVMcbInterfaceCount(cb); i++) {
	    CVMClassBlock *icb = CVMcbInterfacecb(cb, i);
	    for (j = 0; j < CVMcbMethodCount(icb); j++) {
		mb = CVMcbMethodSlot(icb, j);
		if (CVMtypeidIsSame(CVMmbNameAndTypeID(mb), tid)) {
		    return mb;
		}
	    }
	}
    }
    return NULL;
}

static jvmtiError JNICALL
jvmti_GetMethodDeclaringClass(jvmtiEnv* jvmtienv,
			      jmethodID method,
			      jclass* declaringClassPtr) {
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMClassBlock *cb;
    CVMMethodTypeID tid;
    CVMExecEnv* ee = CVMgetEE();
    JNIEnv *env;	  
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL(declaringClassPtr);
    THREAD_OK(ee);

    METHOD_ID_OK(mb);
    tid = CVMmbNameAndTypeID(mb);
    cb = CVMmbClassBlock(mb);
    env = CVMexecEnv2JniEnv(ee);

    if (!CVMmbIs(mb, STATIC)) {
	if (CVMmbIsMiranda(mb)) {
	    mb = findDeclaredMethod(cb, tid, CVM_FALSE, CVM_FALSE, CVM_TRUE);
	} else {
	    mb = findDeclaredMethod(cb, tid, CVM_TRUE, CVM_TRUE, CVM_TRUE);
	}
	if (mb != NULL) {
	    goto found;
	}
    } else {
	const CVMClassBlock *cb0 = cb;

	while (cb0 != NULL) {
	    mb = findDeclaredMethod(cb, tid, CVM_FALSE, CVM_TRUE, CVM_FALSE);
	    if (mb != NULL) {
		if (CVMmbIs(mb, PRIVATE) && cb0 != cb) {
		    break;
		} else {
		    goto found;
		}
	    }

	    /* <clinit> from current class only, not superclasses */
	    if (CVMtypeidIsStaticInitializer(tid)) {
		break;
	    }

	    /* We're still here! Keep searching. */
	    cb0 = CVMcbSuperclass(cb0);
	}
    }
    return JVMTI_ERROR_INVALID_METHODID;

 found:
    *declaringClassPtr =
	(*env)->NewLocalRef(env, CVMcbJavaInstance(CVMmbClassBlock(mb)));
    return JVMTI_ERROR_NONE;
}

/* Return via "modifiersPtr" the modifiers of this method.
 * See JVM Spec "accessFlags".
 * Errors:  JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetMethodModifiers(jvmtiEnv* jvmtienv,
			 jmethodID method,
			 jint* modifiersPtr) {
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    jint modifiers;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL(modifiersPtr);
    METHOD_ID_OK(mb);

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
    return JVMTI_ERROR_NONE;
}

/* Return via "maxPtr" the number of local variable slots used by 
 * this method. Note two-word locals use two slots.
 * Errors: JVMTI_ERROR_INVALID_METHODID, JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetMaxLocals(jvmtiEnv* jvmtienv,
		   jmethodID method,
		   jint* maxPtr) {
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL(maxPtr);

    METHOD_ID_OK(mb);
    if (CVMmbIs(mb, NATIVE)) {
	return JVMTI_ERROR_NATIVE_METHOD;
    }
  
    /* The jmd for <clinit> gets freed after it executes. Can't take
       any chances. */
    /* 
     * Have to do it 'by hand' as we want to avoid an assert in the 
     * case of abstract methods.
     */
    if (mb->immutX.codeX.jmd == NULL) {
	*maxPtr = 0;
	return JVMTI_ERROR_NONE;
    }

    *maxPtr = CVMmbMaxLocals(mb);
    return JVMTI_ERROR_NONE;
}

/* Return via "sizePtr" the number of local variable slots used by 
 * arguments. Note two-word arguments use two slots.
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetArgumentsSize(jvmtiEnv* jvmtienv,
		       jmethodID method,
		       jint* sizePtr) {
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL(sizePtr);

    METHOD_ID_OK(mb);
    if (CVMmbIs(mb, NATIVE)) {
	return JVMTI_ERROR_NATIVE_METHOD;
    }

    *sizePtr = CVMmbArgsSize(mb);
    return JVMTI_ERROR_NONE;
}

/* Return via "tablePtr" the line number table ("entryCountPtr" 
 * returns the number of entries in the table).
 * Errors: JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY,
 * JVMTI_ERROR_ABSENT_INFORMATION
 */

static jvmtiError JNICALL
jvmti_GetLineNumberTable(jvmtiEnv* jvmtienv,
			 jmethodID method,
			 jint* entryCountPtr,
			 jvmtiLineNumberEntry** tablePtr) {
    int i;
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMLineNumberEntry* vmtbl;
    CVMUint16 length;
    jvmtiLineNumberEntry *tbl;
    CVMJavaLong sz;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL2(entryCountPtr, tablePtr);

    METHOD_ID_OK(mb);
    /* NOTE:  Added this check, which is necessary, at least in
       our VM, because this seems to get called for native methods as
       well. Should we return ABSENT_INFORMATION or INVALID_METHODID
       in this case?	*/
    if (!CVMmbIsJava(mb)) {
	return JVMTI_ERROR_ABSENT_INFORMATION;
    }

    /* %comment: k025 */

    if (CVMmbJmd(mb) == NULL) {
	sz = CVMint2Long(sizeof(jvmtiLineNumberEntry));
	ALLOC(jvmtienv, sz, tablePtr);
	*entryCountPtr = (jint)1;
	tbl = *tablePtr;
	tbl[0].start_location = CVMint2Long(0);
	tbl[0].line_number = 0;
	return JVMTI_ERROR_NONE;
    }

    vmtbl = CVMmbLineNumberTable(mb);
    length = CVMmbLineNumberTableLength(mb);

    if (vmtbl == NULL || length == 0) {
	return JVMTI_ERROR_ABSENT_INFORMATION;
    }
    sz = CVMint2Long(length * sizeof(jvmtiLineNumberEntry));
    ALLOC(jvmtienv, sz, tablePtr);
    *entryCountPtr = (jint)length;
    tbl = *tablePtr;
    for (i = 0; i < length; ++i) {
	tbl[i].start_location = CVMint2Long(vmtbl[i].startpc);
	tbl[i].line_number = (jint)(vmtbl[i].lineNumber);
    }
    return JVMTI_ERROR_NONE;
}

/* Return via "startLocationPtr" the first location in the method.
 * Return via "endLocationPtr" the last location in the method.
 * If conventional byte code index locations are being used then
 * this returns zero and the number of byte codes minus one
 * (respectively).
 * If location information is not available return negative one through the
 * pointers.
 * Errors:  JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_ABSENT_INFORMATION
 *
 */

static jvmtiError JNICALL
jvmti_GetMethodLocation(jvmtiEnv* jvmtienv,
			jmethodID method,
			jlocation* startLocationPtr,
			jlocation* endLocationPtr) {
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL2(startLocationPtr, endLocationPtr);

    METHOD_ID_OK(mb);
    if (CVMmbIs(mb, NATIVE)) {
	return JVMTI_ERROR_NATIVE_METHOD;
    }
    if (!CVMmbIsJava(mb)) {
	*startLocationPtr = CVMint2Long(-1);
	*endLocationPtr = CVMint2Long(-1);
	/*	return JVMTI_ERROR_ABSENT_INFORMATION; */
	return JVMTI_ERROR_NONE;
    }

    /* Because we free the Java method descriptor for <clinit> after
       it's run, it's possible for us to get a frame pop event on a
       method which has a NULL jmd. If that happens, we'll do the same
       thing as above. */

    if (CVMmbJmd(mb) == NULL) {
	/* %coment: k020 */
	*startLocationPtr = CVMlongConstZero();
	*endLocationPtr = CVMlongConstZero();
	/*	return JVMTI_ERROR_ABSENT_INFORMATION; */
	return JVMTI_ERROR_NONE;
    }

    *startLocationPtr = CVMlongConstZero();
    *endLocationPtr = CVMint2Long(CVMmbCodeLength(mb) - 1);
    return JVMTI_ERROR_NONE;
}

/* Return via "tablePtr" the local variable table ("entryCountPtr" 
 * returns the number of entries in the table).
 * Errors:  JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY,
 * JVMTI_ERROR_ABSENT_INFORMATION
 */

static jvmtiError JNICALL
jvmti_GetLocalVariableTable(jvmtiEnv* jvmtienv,
			    jmethodID method,
			    jint* entryCountPtr,
			    jvmtiLocalVariableEntry** tablePtr) {
    int i;
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CVMLocalVariableEntry* vmtbl;
    CVMUint16 length;
    CVMClassBlock* cb;
    CVMConstantPool* constantpool;
    jvmtiLocalVariableEntry *tbl;
    CVMJavaLong sz;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL2(entryCountPtr, tablePtr);

    METHOD_ID_OK(mb);
    if (CVMmbIs(mb, NATIVE)) {
	return JVMTI_ERROR_NATIVE_METHOD;
    }

    /* The jmd for <clinit> gets freed after it executes. Can't take
       any chances. */
    if (CVMmbJmd(mb) == NULL) {
	return JVMTI_ERROR_ABSENT_INFORMATION;
    }

    cb = CVMmbClassBlock(mb);
    vmtbl = CVMmbLocalVariableTable(mb);
    length = CVMmbLocalVariableTableLength(mb);
    if (CVMjvmtiMbIsObsolete(mb)) {
	constantpool = CVMjvmtiMbConstantPool(mb);
	if (constantpool == NULL) {
	    constantpool = CVMcbConstantPool(cb);
	}	
    } else {
	constantpool = CVMcbConstantPool(cb);
    }

    if (vmtbl == NULL || length == 0) {
	return JVMTI_ERROR_ABSENT_INFORMATION;
    }
    sz = CVMint2Long(length * sizeof(jvmtiLocalVariableEntry));
    ALLOC(jvmtienv, sz, tablePtr);
    *entryCountPtr = (jint)length;
    tbl = *tablePtr;
    for (i = 0; i < length; ++i) {
	char* nameAndTypeTmp;
	char* buf;
	tbl[i].generic_signature = NULL;
	tbl[i].start_location = CVMint2Long(vmtbl[i].startpc);
	tbl[i].length = (jint)(vmtbl[i].length);

	nameAndTypeTmp =
	    CVMtypeidFieldNameToAllocatedCString(
                CVMtypeidCreateTypeIDFromParts(vmtbl[i].nameID,
					       vmtbl[i].typeID));
	sz = CVMint2Long(strlen(nameAndTypeTmp)+1);
	ALLOC(jvmtienv, sz, &buf);
	strcpy(buf, nameAndTypeTmp);
	free(nameAndTypeTmp);
	tbl[i].name = buf;

	nameAndTypeTmp =
	    CVMtypeidFieldTypeToAllocatedCString(
		CVMtypeidCreateTypeIDFromParts(vmtbl[i].nameID,
					       vmtbl[i].typeID));
	sz = CVMint2Long(strlen(nameAndTypeTmp)+1);
	ALLOC(jvmtienv, sz, &buf);
	strcpy(buf, nameAndTypeTmp);
	free(nameAndTypeTmp);
	tbl[i].signature = buf;

	tbl[i].slot = (jint)(vmtbl[i].index);
    }
    return JVMTI_ERROR_NONE;
}

/* TODO: (k) This is not implemented in CVM */

static jvmtiError JNICALL
jvmti_GetBytecodes(jvmtiEnv* jvmtienv,
		   jmethodID method,
		   jint* bytecodeCountPtr,
		   unsigned char** bytecodesPtr)
{
    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    CHECK_CAP(can_get_bytecodes);
    return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
}


/* Return via "isNativePtr" a boolean indicating whether the method
 * is native.
 * Errors:  JVMTI_ERROR_NULL_POINTER
 */
static jvmtiError JNICALL
jvmti_IsMethodNative(jvmtiEnv* jvmtienv,
		     jmethodID method,
		     jboolean* isNativePtr) {
    CVMMethodBlock* mb = (CVMMethodBlock*) method;
    CHECK_JVMTI_ENV;

    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL(isNativePtr);

    METHOD_ID_OK(mb);
    *isNativePtr = CVMmbIs(mb, NATIVE);
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_IsMethodSynthetic(jvmtiEnv* env,
			jmethodID method,
			jboolean* isSyntheticPtr) {
    return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
}


static jvmtiError JNICALL
jvmti_IsMethodObsolete(jvmtiEnv* env,
		       jmethodID method,
		       jboolean* isObsoletePtr) {
    CVMMethodBlock *mb = (CVMMethodBlock*)method;
    METHOD_ID_OK(mb);
    *isObsoletePtr = CVMjvmtiMbIsObsolete(mb);
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_SetNativeMethodPrefix(jvmtiEnv* env,
			    const char* prefix) {

    return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_SetNativeMethodPrefixes(jvmtiEnv* env,
			      jint prefixCount,
			      char** prefixes) {

    return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Raw Monitor functions
 */ 

/* Return via "monitorPtr" a newly created debugger monitor that can be 
 * used by debugger threads to coordinate. The semantics of the
 * monitor are the same as those described for Java language 
 * monitors in the Java Language Specification.
 * Errors: JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_CreateRawMonitor(jvmtiEnv* jvmtienv,
		       const char* name,
		       jrawMonitorID* monitorPtr) {
    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);
    JVMTI_ENABLED();
    ASSERT_NOT_NULL2_ELSE_EXIT_WITH_ERROR(name, monitorPtr,
					  JVMTI_ERROR_NULL_POINTER);

    if (name == NULL || *name == '\0') {
	/* want some name to tell if monitor is destroyed or not */
	*monitorPtr = (jrawMonitorID) CVMnamedSysMonitorInit("JVMTI");	
    } else {
	*monitorPtr = (jrawMonitorID) CVMnamedSysMonitorInit(name);
    }
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(*monitorPtr,
					 JVMTI_ERROR_OUT_OF_MEMORY);
    return JVMTI_ERROR_NONE;
}

/* Destroy a debugger monitor created with jvmti_CreateRawMonitor.
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_DestroyRawMonitor(jvmtiEnv* jvmtienv,
			jrawMonitorID monitor) {
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_INVALID_MONITOR);

    if (CVMnamedSysMonitorGetName((CVMNamedSysMonitor *) monitor)[0] == '\0') {
	/* probably already destroyed */
	return JVMTI_ERROR_INVALID_MONITOR;
    }
    CVMnamedSysMonitorGetName((CVMNamedSysMonitor *) monitor)[0] = 0;
    CVMnamedSysMonitorDestroy((CVMNamedSysMonitor *) monitor);
    return JVMTI_ERROR_NONE;
}

/* Gain exclusive ownership of a debugger monitor.
 * Errors: JVMTI_ERROR_INVALID_MONITOR
 */

static jvmtiError JNICALL
jvmti_RawMonitorEnter(jvmtiEnv* jvmtienv,
		      jrawMonitorID monitor) {

    CVMExecEnv *ee = CVMgetEE();

    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    THREAD_OK(ee);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_INVALID_MONITOR);

    if (((CVMNamedSysMonitor *) monitor)->_super.type !=
	CVM_LOCKTYPE_NAMED_SYSMONITOR) {
	return JVMTI_ERROR_INVALID_MONITOR;
    }
    CVMnamedSysMonitorEnter((CVMNamedSysMonitor *) monitor, ee);
    return JVMTI_ERROR_NONE;
}

/* Release exclusive ownership of a debugger monitor.
 * Errors: JVMTI_ERROR_INVALID_MONITOR
 */

static jvmtiError JNICALL
jvmti_RawMonitorExit(jvmtiEnv* jvmtienv,
		     jrawMonitorID monitor) {

    CVMExecEnv *currentEE = CVMgetEE();

    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    THREAD_OK(currentEE);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_INVALID_MONITOR);

    if (CVMnamedSysMonitorGetType((CVMNamedSysMonitor *) monitor) !=
	CVM_LOCKTYPE_NAMED_SYSMONITOR) {
	return JVMTI_ERROR_INVALID_MONITOR;
    }
    if (CVMnamedSysMonitorGetOwner((CVMNamedSysMonitor*)monitor) !=
	currentEE) {
	return JVMTI_ERROR_NOT_MONITOR_OWNER;
    }
    CVMnamedSysMonitorExit((CVMNamedSysMonitor *) monitor, currentEE);
    return JVMTI_ERROR_NONE;
}

/* Wait for notification of the debugger monitor. The calling thread
 * must own the monitor. "millis" specifies the maximum time to wait, 
 * in milliseconds.  If millis is -1, then the thread waits forever.
 * Errors: JVMTI_ERROR_INVALID_MONITOR, JVMTI_ERROR_NOT_MONITOR_OWNER,
 * JVMTI_ERROR_INTERRUPT
 */

static jvmtiError JNICALL
jvmti_RawMonitorWait(jvmtiEnv* jvmtienv,
		     jrawMonitorID monitor,
		     jlong millis) {

    jvmtiError result = JVMTI_ERROR_INTERNAL;
    CVMWaitStatus error;
    CVMExecEnv *currentEE = CVMgetEE();

    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    THREAD_OK(currentEE);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_INVALID_MONITOR);

    if (CVMnamedSysMonitorGetType((CVMNamedSysMonitor *) monitor) !=
	CVM_LOCKTYPE_NAMED_SYSMONITOR) {
	return JVMTI_ERROR_INVALID_MONITOR;
    }
    /* %comment l001 */
    if (CVMlongLt(millis, CVMlongConstZero()))  /* if (millis < 0) */
	millis = CVMlongConstZero();		/*   millis = 0; */

    error = CVMnamedSysMonitorWait((CVMNamedSysMonitor *) monitor, currentEE,
				   millis);
    switch (error) {
    case CVM_WAIT_OK:		result = JVMTI_ERROR_NONE; break;
    case CVM_WAIT_INTERRUPTED:  result = JVMTI_ERROR_INTERRUPT; break;
    case CVM_WAIT_NOT_OWNER:	result = JVMTI_ERROR_NOT_MONITOR_OWNER; break;
    }
    return result;
}

/* Notify a single thread waiting on the debugger monitor. The calling 
 * thread must own the monitor.
 * Errors: JVMTI_ERROR_INVALID_MONITOR, JVMTI_ERROR_NOT_MONITOR_OWNER
 */

static jvmtiError JNICALL
jvmti_RawMonitorNotify(jvmtiEnv* jvmtienv,
		       jrawMonitorID monitor) {

    CVMExecEnv *currentEE = CVMgetEE();
    CVMBool successful;

    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    THREAD_OK(currentEE);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_INVALID_MONITOR);

    if (CVMnamedSysMonitorGetType((CVMNamedSysMonitor *) monitor) !=
	CVM_LOCKTYPE_NAMED_SYSMONITOR) {
	return JVMTI_ERROR_INVALID_MONITOR;
    }
    successful =
	CVMnamedSysMonitorNotify((CVMNamedSysMonitor*) monitor, currentEE);
    return successful ? JVMTI_ERROR_NONE : JVMTI_ERROR_NOT_MONITOR_OWNER;
}

/* Notify all threads waiting on the debugger monitor. The calling 
 * thread must own the monitor.
 * Errors: JVMTI_ERROR_INVALID_MONITOR, JVMTI_ERROR_NOT_MONITOR_OWNER
 */

static jvmtiError JNICALL
jvmti_RawMonitorNotifyAll(jvmtiEnv* jvmtienv,
			  jrawMonitorID monitor) {

    CVMExecEnv *currentEE = CVMgetEE();
    CVMBool successful;

    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    THREAD_OK(currentEE);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_INVALID_MONITOR);

    if (CVMnamedSysMonitorGetType((CVMNamedSysMonitor *) monitor) !=
	CVM_LOCKTYPE_NAMED_SYSMONITOR) {
	return JVMTI_ERROR_INVALID_MONITOR;
    }
    successful =
	CVMnamedSysMonitorNotifyAll((CVMNamedSysMonitor*) monitor, currentEE);
    return successful ? JVMTI_ERROR_NONE : JVMTI_ERROR_NOT_MONITOR_OWNER;
}

/*
 * JNI Function Interception functions
 */ 

static jvmtiError JNICALL
jvmti_SetJNIFunctionTable(jvmtiEnv* jvmtienv,
			  const jniNativeInterface* functionTable) {

    jniNativeInterface *currentJNIPtr;
    CVMExecEnv *ee = CVMgetEE();

    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(ee, JVMTI_ERROR_UNATTACHED_THREAD)
    NOT_NULL(functionTable);
    if (ee == NULL) {
	return JVMTI_ERROR_UNATTACHED_THREAD;
    }
 
    /* Need to do atomic copy */
    CVMassert(CVMD_isgcSafe(ee));
#ifdef CVM_JIT
    CVMsysMutexLock(ee, &CVMglobals.jitLock);
#endif
    CVM_THREAD_LOCK(ee);
    /* Roll all threads to safe points. */
    CVMD_gcBecomeSafeAll(ee);
    currentJNIPtr =
	(jniNativeInterface*)CVMjniGetInstrumentableJNINativeInterface();
    memcpy(currentJNIPtr, functionTable, sizeof(jniNativeInterface));
    CVMD_gcAllowUnsafeAll(ee);
    CVM_THREAD_UNLOCK(ee);
#ifdef CVM_JIT
    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetJNIFunctionTable(jvmtiEnv* jvmtienv,
			  jniNativeInterface** functionTable) {
    CVMExecEnv* ee = CVMgetEE();
    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    if (ee == NULL) {
	return JVMTI_ERROR_UNATTACHED_THREAD;
    }
    ALLOC(jvmtienv, sizeof(jniNativeInterface), functionTable);
    memcpy(*functionTable, CVMjniGetInstrumentableJNINativeInterface(),
	   sizeof(jniNativeInterface));
    return JVMTI_ERROR_NONE;
}


/*
 * Event Management functions
 * 
 * First pass - don't port the whole complicated thread x env matrix
 * capability
 */
/* Check Event Capabilities */
CVMBool hasEventCapability(jvmtiEvent eventType,
			     jvmtiCapabilities* capabilitiesPtr) {
    switch (eventType) {
    case JVMTI_EVENT_SINGLE_STEP:
	return capabilitiesPtr->can_generate_single_step_events != 0;
    case JVMTI_EVENT_BREAKPOINT:
	return capabilitiesPtr->can_generate_breakpoint_events != 0;
    case JVMTI_EVENT_FIELD_ACCESS:
	return capabilitiesPtr->can_generate_field_access_events != 0;
    case JVMTI_EVENT_FIELD_MODIFICATION:
	return capabilitiesPtr->can_generate_field_modification_events != 0;
    case JVMTI_EVENT_FRAME_POP:
	return capabilitiesPtr->can_generate_frame_pop_events != 0;
    case JVMTI_EVENT_METHOD_ENTRY:
	return capabilitiesPtr->can_generate_method_entry_events != 0;
    case JVMTI_EVENT_METHOD_EXIT:
	return capabilitiesPtr->can_generate_method_exit_events != 0;
    case JVMTI_EVENT_NATIVE_METHOD_BIND:
	return capabilitiesPtr->can_generate_native_method_bind_events != 0;
    case JVMTI_EVENT_EXCEPTION:
	return capabilitiesPtr->can_generate_exception_events != 0;
    case JVMTI_EVENT_EXCEPTION_CATCH:
	return capabilitiesPtr->can_generate_exception_events != 0;
    case JVMTI_EVENT_COMPILED_METHOD_LOAD:
	return capabilitiesPtr->can_generate_compiled_method_load_events != 0;
    case JVMTI_EVENT_COMPILED_METHOD_UNLOAD:
	return capabilitiesPtr->can_generate_compiled_method_load_events != 0;
    case JVMTI_EVENT_MONITOR_CONTENDED_ENTER:
	return capabilitiesPtr->can_generate_monitor_events != 0;
    case JVMTI_EVENT_MONITOR_CONTENDED_ENTERED:
	return capabilitiesPtr->can_generate_monitor_events != 0;
    case JVMTI_EVENT_MONITOR_WAIT:
	return capabilitiesPtr->can_generate_monitor_events != 0;
    case JVMTI_EVENT_MONITOR_WAITED:
	return capabilitiesPtr->can_generate_monitor_events != 0;
    case JVMTI_EVENT_VM_OBJECT_ALLOC:
	return capabilitiesPtr->can_generate_vm_object_alloc_events != 0;
    case JVMTI_EVENT_OBJECT_FREE:
	return capabilitiesPtr->can_generate_object_free_events != 0;
    case JVMTI_EVENT_GARBAGE_COLLECTION_START:
	return capabilitiesPtr->can_generate_garbage_collection_events != 0;
    case JVMTI_EVENT_GARBAGE_COLLECTION_FINISH:
	return capabilitiesPtr->can_generate_garbage_collection_events != 0;
    default:
        return JNI_TRUE;
    }
    /* if it does not have a capability it is required */
    return JNI_TRUE;
}

static CVMBool isValidEventType(jvmtiEvent eventType) {
    return ((int)eventType >= JVMTI_MIN_EVENT_TYPE_VAL) &&
	((int)eventType <= JVMTI_MAX_EVENT_TYPE_VAL);
}

jlong bitFor(jvmtiEvent eventType) {
    CVMassert(isValidEventType(eventType));
    return ((jlong)1) << CVMjvmtiEvent2EventBit(eventType);	 
}

CVMBool hasCallback(CVMJvmtiContext *context, jvmtiEvent eventType)
{
    void **callbacks = (void **)&context->eventCallbacks;
    CVMassert(eventType >= JVMTI_MIN_EVENT_TYPE_VAL && 
	      eventType <= JVMTI_MAX_EVENT_TYPE_VAL);
    return callbacks[eventType-JVMTI_MIN_EVENT_TYPE_VAL] != NULL;
}

/* mlam :: REVIEW DONE */
static CVMBool
isGlobalEvent(jvmtiEvent eventType)
{
    /* TODO: Change implementation to not rely on 64bit masking if possible. */
    jlong bitFor;
    CVMassert(isValidEventType(eventType));
    bitFor = ((jlong)1) << CVMjvmtiEvent2EventBit(eventType); 
    return((bitFor & GLOBAL_EVENT_BITS)!=0);
}

static void setUserEnabled(jvmtiEnv *jvmtienv, CVMExecEnv *ee,
			   jvmtiEvent eventType, CVMBool enabled)
{
    volatile CVMJvmtiEventEnabled *eventp;
    jlong bits;
    jlong mask;

    if (ee != NULL) {
	eventp = &CVMjvmtiUserEventEnabled(ee);
    } else {
	CVMJvmtiContext *context = CVMjvmtiEnv2Context(jvmtienv);
	eventp = &context->envEventEnable.eventUserEnabled;
    }
    bits = eventp->enabledBits;
    mask = bitFor(eventType);
    if (enabled) {
	bits |= mask;
    } else {
	bits &= ~mask;
    }
    eventp->enabledBits = bits;
}

static jlong
recomputeEnvEnabled(CVMJvmtiEnvEventEnable *eventp)
{
    jlong nowEnabled =
	eventp->eventUserEnabled.enabledBits &
	eventp->eventCallbackEnabled.enabledBits;

    eventp->eventEnabled.enabledBits |= nowEnabled;
    return(nowEnabled);
}

jlong
CVMjvmtiRecomputeThreadEnabled(CVMExecEnv *ee, CVMJvmtiEnvEventEnable *eventp)
{
    jlong wasEnabled = CVMjvmtiEventEnabled(ee).enabledBits;
    jlong anyEnabled = THREAD_FILTERED_EVENT_BITS &
	eventp->eventCallbackEnabled.enabledBits &
	(eventp->eventUserEnabled.enabledBits |
	 CVMjvmtiUserEventEnabled(ee).enabledBits);

    if (anyEnabled != wasEnabled) {
	CVMjvmtiEventEnabled(ee).enabledBits = anyEnabled;
    }
    CVMjvmtiSingleStepping(ee) = ((anyEnabled & SINGLE_STEP_BIT) != 0);
    CVMjvmtiSetProcessingCheck(ee);

    return anyEnabled;
}

static
void jvmtiRecomputeEnabled(CVMJvmtiEnvEventEnable *eventp)
{
    jlong wasAnyEnvThreadEnabled = 0;
    jlong anyEnvThreadEnabled = 0;
    jlong delta;

    CVMExecEnv *ee = CVMgetEE();

    wasAnyEnvThreadEnabled = eventp->eventEnabled.enabledBits;
    anyEnvThreadEnabled |= recomputeEnvEnabled(eventp);
    CVM_THREAD_LOCK(ee);

    /*    CVM_WALK_ALL_THREADS(ee, currentEE, { */
    {
	CVMExecEnv *currentEE = CVMglobals.threadList;
	while(currentEE != NULL) {
	    anyEnvThreadEnabled |=
		CVMjvmtiRecomputeThreadEnabled(currentEE, eventp);
	    currentEE = currentEE->nextEE;
	}
    }
    CVM_THREAD_UNLOCK(ee);

    /* set global flags */
    eventp->eventEnabled.enabledBits = anyEnvThreadEnabled;

    delta = wasAnyEnvThreadEnabled ^ anyEnvThreadEnabled;

#undef IF_SET
#define IF_SET(x)  ((anyEnvThreadEnabled & (x)) != 0)

    /* set universal state (across all envs and threads) */
    CVMjvmtiSetShouldPostFieldAccess(IF_SET(FIELD_ACCESS_BIT));
    CVMjvmtiSetShouldPostFieldModification(IF_SET(FIELD_MODIFICATION_BIT));

    CVMjvmtiSetShouldPostClassLoad(IF_SET(CLASS_LOAD_BIT));
    CVMjvmtiSetShouldPostClassFileLoadHook(IF_SET(CLASS_FILE_LOAD_HOOK_BIT));

    CVMjvmtiSetShouldPostNativeMethodBind(IF_SET(NATIVE_METHOD_BIND_BIT));
    CVMjvmtiSetShouldPostDynamicCodeGenerated(
        IF_SET(DYNAMIC_CODE_GENERATED_BIT));

    CVMjvmtiSetShouldPostDataDump(IF_SET(DATA_DUMP_BIT));
    CVMjvmtiSetShouldPostClassPrepare(IF_SET(CLASS_PREPARE_BIT));

    CVMjvmtiSetShouldPostMonitorContendedEnter(
        IF_SET(MONITOR_CONTENDED_ENTER_BIT));
    CVMjvmtiSetShouldPostMonitorContendedEntered(
        IF_SET(MONITOR_CONTENDED_ENTERED_BIT));
    CVMjvmtiSetShouldPostMonitorWait(IF_SET(MONITOR_WAIT_BIT));
    CVMjvmtiSetShouldPostMonitorWaited(IF_SET(MONITOR_WAITED_BIT));

    CVMjvmtiSetShouldPostGarbageCollectionStart(
        IF_SET(GARBAGE_COLLECTION_START_BIT));
    CVMjvmtiSetShouldPostGarbageCollectionFinish(
        IF_SET(GARBAGE_COLLECTION_FINISH_BIT));

    CVMjvmtiSetShouldPostObjectFree(IF_SET(OBJECT_FREE_BIT));

    CVMjvmtiSetShouldPostCompiledMethodLoad(IF_SET(COMPILED_METHOD_LOAD_BIT));
    CVMjvmtiSetShouldPostCompiledMethodUnload(
        IF_SET(COMPILED_METHOD_UNLOAD_BIT));

    CVMjvmtiSetShouldPostVmObjectAlloc(IF_SET(VM_OBJECT_ALLOC_BIT));
    CVMjvmtiSetShouldPostThreadLife(IF_SET(NEED_THREAD_LIFE_EVENTS));

#undef IF_SET
}

static jvmtiError JNICALL
jvmti_SetEventCallbacks(jvmtiEnv* jvmtienv,
			const jvmtiEventCallbacks* callbacks,
			jint sizeOfCallbacks) {
    size_t byteCnt = sizeof(jvmtiEventCallbacks);
    jlong enabledBits = 0;
    int ei;
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);
    THREAD_OK(CVMgetEE());

    /* clear in either case to be sure we got any gap between sizes */
    memset(&context->eventCallbacks, 0, byteCnt);
    if (callbacks != NULL) {
	if (sizeOfCallbacks < (jint)byteCnt) {
	    byteCnt = sizeOfCallbacks;
	}
	memcpy(&context->eventCallbacks, callbacks, byteCnt);
    }
    for (ei = JVMTI_MIN_EVENT_TYPE_VAL;
	 ei <= JVMTI_MAX_EVENT_TYPE_VAL; ++ei) {
	jvmtiEvent evtT = (jvmtiEvent)ei;
	if (hasCallback(context, evtT)) {
	    enabledBits |= bitFor(evtT);
	}
    }
    context->envEventEnable.eventCallbackEnabled.enabledBits = enabledBits;
    /* Need to set global event flag which is the 'and' of user and callback
       events */
    jvmtiRecomputeEnabled(&context->envEventEnable);  

    /*    context->envEventEnable.EventEnabled.enabledBits =
	context->envEventEnable.EventUserEnabled.enabledBits &
	context->envEventEnable.EventCallbackEnabled.enabledBits; */
    if (context->envEventEnable.eventEnabled.enabledBits &
	(bitFor(JVMTI_EVENT_THREAD_START) | bitFor(JVMTI_EVENT_THREAD_END))) {
	context->threadEventsEnabled = CVM_TRUE;
    }
    return JVMTI_ERROR_NONE;
}

static jvmtiError JNICALL
jvmti_SetEventNotificationMode(jvmtiEnv* jvmtienv,
			       jvmtiEventMode mode,
			       jvmtiEvent eventType,
			       jthread eventThread,
			       ...) {
    CVMExecEnv* ee = NULL;
    CVMBool enabled;
    jvmtiError err;

    CHECK_JVMTI_ENV;

    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);
    if (eventThread != NULL) {
	err = jthreadToExecEnv(CVMgetEE(), eventThread, &ee);

	if (err != JVMTI_ERROR_NONE) {
	    return err;
	}
    }

    /* eventType must be valid */
    if (!isValidEventType(eventType)) {
	return JVMTI_ERROR_INVALID_EVENT_TYPE;
    }

    /* global events cannot be controlled at thread level. */
    if (eventThread != NULL && isGlobalEvent(eventType)) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }
	   
    enabled = (mode == JVMTI_ENABLE);
    /* assure that needed capabilities are present */
    if (enabled && !hasEventCapability(eventType,
				       &context->currentCapabilities)) {
	return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
    }

    CVMtraceJVMTI(("JVMTI: SetEvtN: ee: 0x%x, evt: 0x%x, ena: %d\n", 
		   (int)ee, (int)eventType, (int)enabled));
    if (eventThread != NULL) {
	CVMJvmtiThreadNode *node;
	
	node = CVMjvmtiFindThread(CVMgetEE(), eventThread);
	if (node == NULL) {
	    return JVMTI_ERROR_INVALID_THREAD;
	}
    }

    /*	   
      if (eventType == JVMTI_EVENT_CLASS_FILE_LOAD_HOOK && enabled) {
      recordClassFileLoadHookEnabled();
      }
    */
    setUserEnabled(jvmtienv, ee, eventType, enabled);
    /* Need to set global event flag which is the 'and' of user and callback
       events */
    /*    context->envEventEnable.EventEnabled.enabledBits =
	context->envEventEnable.EventUserEnabled.enabledBits &
	context->envEventEnable.EventCallbackEnabled.enabledBits; */
    if (context->envEventEnable.eventEnabled.enabledBits &
	(bitFor(JVMTI_EVENT_THREAD_START) | bitFor(JVMTI_EVENT_THREAD_END))) {
	context->threadEventsEnabled = CVM_TRUE;
    }
    jvmtiRecomputeEnabled(&context->envEventEnable);  
    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
/* Send a JVMTI_EVENT_FRAME_POP event when the specified frame is
 * popped from the stack.  The event is sent after the pop has occurred.
 * Errors: JVMTI_ERROR_OPAQUE_FRAME, JVMTI_ERROR_NO_MORE_FRAMES,
 * JVMTI_ERROR_OUT_OF_MEMORY
 */
static jvmtiError JNICALL
jvmti_NotifyFramePop(jvmtiEnv* jvmtienv,
		     jthread thread,
		     jint depth)
{
    CVMFrameIterator iter;
    CVMFrame* frame;
    CVMFrame* framePrev;
    jvmtiError err = JVMTI_ERROR_NONE;
    CVMExecEnv* ee = CVMgetEE();
    CVMExecEnv *targetEE;

    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    if (depth < 0) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }
    CVMextraAssert(CVMD_isgcSafe(ee));

    err = jthreadToExecEnv(ee, thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }

    /* jthreadToExecEnv() should have ensured that the targetEE is not NULL.
       If it is, we would have returned with a JVMTI_ERROR_THREAD_NOT_ALIVE
       as required by the JVMTI spec. */
    CVMextraAssert(targetEE != NULL);

    STOP_STACK_MUTATION(ee, targetEE);

    err = getFrame(targetEE, depth, &iter);
    if (err != JVMTI_ERROR_NONE) {
	goto cleanUpAndReturn;
    }

    /* Note: this following code basically ensures that we're not dealing
       with compiled or inlined frames.  Currently, these aren't supported
       at the same time that the JIT is enabled.  So, if we see frames
       that are compiled, we'll fail with JVMTI_ERROR_OPAQUE_FRAME.
       Note that the same checks also takes care of native frames.
    */

    /* Get the physical frame.  If the frame is not an interpreted frame,
       then it is effectively opaque and we can't service it.
       Note: if the iterator is currently pointing to a JIT inlined method,
       then GetFrame() below will return the frame of the outer most method
       that inlined this method.
    */
    frame = CVMframeIterateGetFrame(&iter);
    if (!CVMframeIsJava(frame)) {
	err = JVMTI_ERROR_OPAQUE_FRAME;
	goto cleanUpAndReturn;
    }

    /*
     * Frame pop notification operates on the calling frame, so 
     * make sure it exists.
     */
    framePrev = CVMframePrev(frame);
    if (framePrev == NULL) {
	/* First frame (no previous) must be opaque */
	err = JVMTI_ERROR_OPAQUE_FRAME;
    } else {
	/* This is needed by the code below, but wasn't
	   present in the 1.2 sources */
	if (!CVMframeIsInterpreter(framePrev)) {
	    err = JVMTI_ERROR_OPAQUE_FRAME;
	    goto cleanUpAndReturn;
	}

	JVMTI_LOCK(ee);
	/* TODO: Review this further:
	   JVMDI checks for duplicate bag entries.  That has been removed here.
	   Make sure that it is OK.
	*/
	CVMtraceJVMTI(("JVMTI: Notify Frame Pop: ee 0x%x\n", (int)ee));
	/* check if one already exists */
	if (CVMbagFind(CVMglobals.jvmti.statics.framePops, frame) == NULL) {	   
	    /* allocate space for it */
	    struct fpop *fp = CVMbagAdd(CVMglobals.jvmti.statics.framePops);
	    if (fp == NULL) {
		err = JVMTI_ERROR_OUT_OF_MEMORY;
	    } else {
		/* Note that "frame pop sentinel" (i.e., returnpc
		   mangling) code removed in CVM. The JDK can do this
		   because it has two returnpc slots per frame. See
		   CVMjvmtiNotifyDebuggerOfFramePop for possible
		   optimization/solution. */
		fp->frame = frame;
		err = JVMTI_ERROR_NONE;
		/*
		setUserEnabled(jvmtienv, NULL, JVMTI_EVENT_FRAME_POP, CVM_TRUE);
		jvmtiRecomputeEnabled(&context->envEventEnable);  
		*/
		CVMjvmtiRecomputeThreadEnabled(ee,
		    &context->envEventEnable);
	    }
	}
	JVMTI_UNLOCK(ee);
    }

 cleanUpAndReturn:
    RESUME_STACK_MUTATION(ee, targetEE);
    return err;
}


static jvmtiError JNICALL
jvmti_GenerateEvents(jvmtiEnv* env,
		     jvmtiEvent eventType) {
    return JVMTI_ERROR_NONE;
}


/*
 * Extension Mechanism functions
 */ 

static jvmtiError JNICALL
jvmti_GetExtensionFunctions(jvmtiEnv* jvmtienv,
			    jint* extensionCountPtr,
			    jvmtiExtensionFunctionInfo** extensions) {
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);
    NOT_NULL(extensionCountPtr);
    NOT_NULL(extensions);

    *extensionCountPtr = 0;
    *extensions = NULL;
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetExtensionEvents(jvmtiEnv* jvmtienv,
			 jint* extensionCountPtr,
			 jvmtiExtensionEventInfo** extensions) {
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);
    NOT_NULL(extensionCountPtr);
    NOT_NULL(extensions);

    *extensionCountPtr = 0;
    *extensions = NULL;
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_SetExtensionEventCallback(jvmtiEnv* env,
				jint extensionEventIndex,
				jvmtiExtensionEvent callback) {
    return JVMTI_ERROR_ACCESS_DENIED;
}



/*
 * Timers functions
 */ 

static jvmtiError JNICALL
jvmti_GetCurrentThreadCpuTimerInfo(jvmtiEnv* jvmtienv,
				   jvmtiTimerInfo* infoPtr) {

    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL(infoPtr);

    infoPtr->max_value = CONST64(0xFFFFFFFFFFFFFFFF);
                                       /* will not wrap in less than 64 bits */
    infoPtr->may_skip_backward = CVM_FALSE;    /* elapsed time not wall time */
    infoPtr->may_skip_forward = CVM_FALSE;     /* elapsed time not wall time */
    infoPtr->kind = JVMTI_TIMER_TOTAL_CPU;  /* user+system time is returned */

    return JVMTI_ERROR_NONE;
}

static jvmtiError JNICALL
jvmti_GetTime(jvmtiEnv* jvmtienv, jlong* nanosPtr);

static jvmtiError JNICALL
jvmti_GetCurrentThreadCpuTime(jvmtiEnv* jvmtienv,
			      jlong* nanosPtr) {
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_START);
    NOT_NULL(nanosPtr);
    *nanosPtr = CVMtimeCurrentThreadCpuTime(CVMthreadSelf());
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetThreadCpuTimerInfo(jvmtiEnv* jvmtienv,
			    jvmtiTimerInfo* infoPtr) {
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);
    NOT_NULL(infoPtr);

    infoPtr->max_value = CONST64(0xFFFFFFFFFFFFFFFF);
                                        /* will not wrap in less than 64 bits */
    infoPtr->may_skip_backward = CVM_FALSE;     /* elapsed time not wall time */
    infoPtr->may_skip_forward = CVM_FALSE;      /* elapsed time not wall time */
    infoPtr->kind = JVMTI_TIMER_TOTAL_CPU;  /* user+system time is returned */

    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetThreadCpuTime(jvmtiEnv* jvmtienv,
		       jthread thread,
		       jlong* nanosPtr) {
    CVMExecEnv *targetEE;
    jvmtiError err;

    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_LIVE);

#if 0
    jlong timeClks = CVMgetCpuTimes();
    jlong timeNs = CVMlongMul(timeClks, CVMjvmtiGetClockTicks);
    *nanosPtr = timeNs;
    return JVMTI_ERROR_NONE;
#endif
    err = jthreadToExecEnv(CVMgetEE(), thread, &targetEE);
    if (err != JVMTI_ERROR_NONE) {
	return err;
    }
    NOT_NULL(nanosPtr);
    *nanosPtr = CVMtimeThreadCpuTime(&targetEE->threadInfo);
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetTimerInfo(jvmtiEnv* jvmtienv,
		   jvmtiTimerInfo* infoPtr) {

    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    NOT_NULL(infoPtr);

    infoPtr->max_value = CONST64(0xFFFFFFFFFFFFFFFF);
                                  /* will not wrap in less than 64 bits */
    infoPtr->may_skip_backward = CVM_TRUE;     /* wall time */
    infoPtr->may_skip_forward = CVM_TRUE;      /* wall time */
    infoPtr->kind = JVMTI_TIMER_ELAPSED;  /* elapsed time */

    return JVMTI_ERROR_NONE;
}

static jvmtiError JNICALL
jvmti_GetTime(jvmtiEnv* jvmtienv,
	      jlong* nanosPtr) {
    jlong timeNs;
    CHECK_JVMTI_ENV;
#ifdef CVM_DEBUG_ASSERTS
    {
	CVMExecEnv *ee = CVMgetEE();
	CVMassert (ee != NULL);
	CVMassert(CVMD_isgcSafe(ee));
    }
#endif

    JVMTI_ENABLED();
    NOT_NULL(nanosPtr);

    timeNs = CVMtimeNanosecs();
    *nanosPtr = timeNs;
    return JVMTI_ERROR_NONE;
}

static jvmtiError JNICALL
jvmti_GetAvailableProcessors(jvmtiEnv* jvmtienv,
			     jint* processorCountPtr) {
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    NOT_NULL(processorCountPtr);
    /* TODO: should add support to get proper count of processors when OS
     * layer supports that 
     */
    *processorCountPtr = 1;
    return JVMTI_ERROR_NONE;
}


/*
 * Class Loader Search functions
 */ 

static jvmtiError JNICALL
jvmti_AddToBootstrapClassLoaderSearch(jvmtiEnv* jvmtienv,
				      const char* segment) {
#ifdef CVM_CLASSLOADING
    char *tmp;
    JNIEnv *env;
    CVMExecEnv *ee = CVMgetEE();
    int len;

    CHECK_JVMTI_ENV;

    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);
    env = CVMexecEnv2JniEnv(ee);

    if (CVMjvmtiGetPhase() == JVMTI_PHASE_ONLOAD) {
        if (CVMglobals.bootClassPath.pathString == NULL) {
            tmp = strdup(segment);
            if (tmp == NULL) {
                return JVMTI_ERROR_OUT_OF_MEMORY;
            }
        } else {
            /* pathString len + seperator + segmetn + null */
            len = strlen(CVMglobals.bootClassPath.pathString) +
                strlen(segment) + 2;
            tmp = calloc(len, sizeof(char));
            if (tmp == NULL) {
                return JVMTI_ERROR_OUT_OF_MEMORY;
            }
            sprintf(tmp, "%s%s%s", CVMglobals.bootClassPath.pathString,
                    CVM_PATH_CLASSPATH_SEPARATOR, segment);
            free(CVMglobals.bootClassPath.pathString);
        }
	CVMglobals.bootClassPath.pathString = tmp;
    } else if (CVMjvmtiGetPhase() == JVMTI_PHASE_LIVE) {
	char *seg = (char *)segment;
	CVMclassPathInit(env, &CVMglobals.bootClassPath,
			 seg, CVM_FALSE, CVM_FALSE);
    }
    return JVMTI_ERROR_NONE;
#else
    return JVMTI_ERROR_CLASS_LOADER_UNSUPPORTED;
#endif
}


static jvmtiError JNICALL
jvmti_AddToSystemClassLoaderSearch(jvmtiEnv* env,
				   const char* segment) {
    return JVMTI_ERROR_CLASS_LOADER_UNSUPPORTED;
}


/*
 * System Properties functions
 */ 

static jvmtiError JNICALL
jvmti_GetSystemProperties(jvmtiEnv* jvmtienv,
			  jint* countPtr,
			  char*** propertyPtr) {
    char *vendor = "java.vm.vendor";
    char *version = "java.vm.version";
    char *name = "java.vm.name";
    char *info = "java.vm.info";
    char *libpath = "java.library.path";
    char *classpath = "java.class.path";
    int numProps = 6; /* hard-wired! */
    char **props;

    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);
    NOT_NULL(propertyPtr);
    *propertyPtr = NULL;

    ALLOC(jvmtienv, numProps * sizeof(char **), &props);
    ALLOC(jvmtienv, strlen(vendor)+1, &props[0]);
    ALLOC(jvmtienv, strlen(version)+1, &props[1]);
    ALLOC(jvmtienv, strlen(name)+1, &props[2]);
    ALLOC(jvmtienv, strlen(info)+1, &props[3]);
    ALLOC(jvmtienv, strlen(libpath)+1, &props[4]);
    ALLOC(jvmtienv, strlen(classpath)+1, &props[5]);
    strcpy(props[0], vendor);
    strcpy(props[1], version);
    strcpy(props[2], name);
    strcpy(props[3], info);
    strcpy(props[4], libpath);
    strcpy(props[5], classpath);
    *countPtr = numProps;
    *propertyPtr = props;
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetSystemProperty(jvmtiEnv* jvmtienv,
			const char* property,
			char** valuePtr) {

    jstring valueString;
    jstring nameString;
    jmethodID systemGetProperty;
    const char *utf;
    JNIEnv *env;
    jclass systemClass;
    CVMExecEnv *ee = CVMgetEE();
    jvmtiError err = JVMTI_ERROR_NONE;

    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE2(JVMTI_PHASE_LIVE, JVMTI_PHASE_ONLOAD);
    NOT_NULL(valuePtr);
    *valuePtr = NULL;

    env = CVMexecEnv2JniEnv(ee);
    if ((*env)->PushLocalFrame(env, 1) < 0) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }

    nameString = (*env)->NewStringUTF(env, property);
    if ((*env)->ExceptionOccurred(env)) {
	(*env)->ExceptionClear(env);
        err = JVMTI_ERROR_NOT_AVAILABLE;
	/* NULL will be returned below */
    } else {
	if (CVMjvmtiGetPhase() == JVMTI_PHASE_ONLOAD) {
	    const char *prop;
	    const CVMProperties *sprops = CVMgetProperties();
	    if (!strcmp(property, "java.library.path")) {
		prop = sprops->library_path;
	    } else if (!strcmp(property, "sun.boot.library.path")) {
		prop = sprops->dll_dir;
	    } else if (!strcmp(property, "java.home")) {
		prop = sprops->java_home;
	    } else if (!strcmp(property, "java.ext.dirs")) {
		prop = sprops->ext_dirs;
	    } else if (!strcmp(property, "sun.boot.class.path")) {
		prop = sprops->sysclasspath;
	    } else {
		(*env)->PopLocalFrame(env, 0);
		return JVMTI_ERROR_NOT_AVAILABLE;
	    }
	    /* Can not allocate at this point */
	    *(const char **)valuePtr = prop;
	} else {
	    systemClass = CVMcbJavaInstance(CVMsystemClass(java_lang_System));
	    systemGetProperty = (*env)->GetStaticMethodID(env, systemClass, 
		"getProperty", "(Ljava/lang/String;)Ljava/lang/String;");

	    valueString =
		(*env)->CallStaticObjectMethod(env, systemClass, 
					       systemGetProperty, nameString);
	    if ((*env)->ExceptionOccurred(env)) {
		(*env)->ExceptionClear(env);
		err = JVMTI_ERROR_NOT_AVAILABLE;
	    } else if (valueString != NULL) {
		utf = (*env)->GetStringUTFChars(env, valueString, NULL);
		ALLOC(jvmtienv, strlen(utf)+1, valuePtr);
		strcpy(*valuePtr, utf);
		(*env)->ReleaseStringUTFChars(env, valueString, utf);
	    } else {
		err = JVMTI_ERROR_NOT_AVAILABLE;
	    }
	}
    }

    (*env)->PopLocalFrame(env, 0);

    return err;
}


static jvmtiError JNICALL
jvmti_SetSystemProperty(jvmtiEnv* jvmtienv,
			const char* property,
			const char* value) {
    JNIEnv *env;
    CVMExecEnv *ee = CVMgetEE();
    char **prop;
    const CVMProperties *sprops;

    CHECK_JVMTI_ENV;
    CVMJVMTI_CHECK_PHASE(JVMTI_PHASE_ONLOAD);

    env = CVMexecEnv2JniEnv(ee);
    if ((*env)->PushLocalFrame(env, 1) < 0) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }

    sprops = CVMgetProperties();
    if (!strcmp(property, "java.library.path")) {
	prop = ( char **) &sprops->library_path;
    } else if (!strcmp(property, "sun.boot.library.path")) {
	prop = ( char **)&sprops->dll_dir;
    } else if (!strcmp(property, "java.home")) {
	prop = ( char **)&sprops->java_home;
    } else if (!strcmp(property, "java.ext.dirs")) {
	prop = ( char **)&sprops->ext_dirs;
    } else if (!strcmp(property, "sun.boot.class.path")) {
	prop = ( char **)&sprops->sysclasspath;
    } else { 
	(*env)->PopLocalFrame(env, 0);
	return JVMTI_ERROR_NOT_AVAILABLE;
    }
    free(*prop);
    *prop = malloc(strlen(value) + 1);
    if (*prop == NULL) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
    }
    strcpy(*prop, value);
    (*env)->PopLocalFrame(env, 0);
    return JVMTI_ERROR_NONE;
}


/*
 * General functions
 */ 

static jvmtiError JNICALL
jvmti_GetPhase(jvmtiEnv* jvmtienv,
	       jvmtiPhase* phasePtr)
{
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    NOT_NULL(phasePtr);

    *phasePtr = CVMjvmtiGetPhase();
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_DisposeEnvironment(jvmtiEnv* jvmtienv)
{
    jvmtiCapabilities *cap;
    CHECK_JVMTI_ENV;

    cap = getCapabilities(context);
    CVMjvmtiRelinquishCapabilities(cap, cap, cap);
    jvmti_SetEventCallbacks(jvmtienv, NULL, 0);
    /* TODO: The following clean up should only be done if we are disposing
       of the last agentLib JVMTI environment.  However, since we only support
       one agentLib for now, we can do this unconditionally for now. */
    CVMjvmtiDestroy(&CVMglobals.jvmti);

    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_SetEnvironmentLocalStorage(jvmtiEnv* jvmtienv,
				 const void* data)
{
    CHECK_JVMTI_ENV;

    context->envLocalStorage = data;
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetEnvironmentLocalStorage(jvmtiEnv* jvmtienv,
				 void** dataPtr)
{
    CHECK_JVMTI_ENV;

    *dataPtr = (void *)context->envLocalStorage;
    return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetVersionNumber(jvmtiEnv* env,
		       jint* versionPtr)
{
    NOT_NULL(versionPtr);
    *versionPtr = JVMTI_VERSION;
    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_GetErrorName(jvmtiEnv* jvmtienv,
		   jvmtiError error,
		   char** namePtr)
{
    const char *name;
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    NOT_NULL(namePtr);

    if ((int)error < JVMTI_ERROR_NONE || error > JVMTI_ERROR_MAX) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }
    name = jvmtiErrorNames[error];
    ALLOC(jvmtienv, strlen(name)+1, namePtr);
    strcpy(*namePtr, name);
    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_SetVerboseFlag(jvmtiEnv* env,
		     jvmtiVerboseFlag flag,
		     jboolean value)
{
    if (flag != JVMTI_VERBOSE_OTHER &&
	flag != JVMTI_VERBOSE_GC &&
	flag != JVMTI_VERBOSE_CLASS &&
	flag != JVMTI_VERBOSE_JNI) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }

    return JVMTI_ERROR_NONE;
}

/* mlam :: REVIEW DONE */
static jvmtiError JNICALL
jvmti_GetJLocationFormat(jvmtiEnv* jvmtienv,
			 jvmtiJlocationFormat* formatPtr) {
    CHECK_JVMTI_ENV;
    JVMTI_ENABLED();
    NOT_NULL(formatPtr);

    *formatPtr = JVMTI_JLOCATION_JVMBCI;
    return JVMTI_ERROR_NONE;
}



/* JVMTI API functions */
jvmtiInterface_1 CVMjvmti_Interface = {
    /*   1 :	RESERVED */
    NULL,
    /*   2 : Set Event Notification Mode */
    jvmti_SetEventNotificationMode,
    /*   3 :	RESERVED */
    NULL,
    /*   4 : Get All Threads */
    jvmti_GetAllThreads,
    /*   5 : Suspend Thread */
    jvmti_SuspendThread,
    /*   6 : Resume Thread */
    jvmti_ResumeThread,
    /*   7 : Stop Thread */
    jvmti_StopThread,
    /*   8 : Interrupt Thread */
    jvmti_InterruptThread,
    /*   9 : Get Thread Info */
    jvmti_GetThreadInfo,
    /*   10 : Get Owned Monitor Info */
    jvmti_GetOwnedMonitorInfo,
    /*   11 : Get Current Contended Monitor */
    jvmti_GetCurrentContendedMonitor,
    /*   12 : Run Agent Thread */
    jvmti_RunAgentThread,
    /*   13 : Get Top Thread Groups */
    jvmti_GetTopThreadGroups,
    /*   14 : Get Thread Group Info */
    jvmti_GetThreadGroupInfo,
    /*   15 : Get Thread Group Children */
    jvmti_GetThreadGroupChildren,
    /*   16 : Get Frame Count */
    jvmti_GetFrameCount,
    /*   17 : Get Thread State */
    jvmti_GetThreadState,
    /*   18 : Get Current Thread */
    jvmti_GetCurrentThread,
    /*   19 : Get Frame Location */
    jvmti_GetFrameLocation,
    /*   20 : Notify Frame Pop */
    jvmti_NotifyFramePop,
    /*   21 : Get Local Variable - Object */
    jvmti_GetLocalObject,
    /*   22 : Get Local Variable - Int */
    jvmti_GetLocalInt,
    /*   23 : Get Local Variable - Long */
    jvmti_GetLocalLong,
    /*   24 : Get Local Variable - Float */
    jvmti_GetLocalFloat,
    /*   25 : Get Local Variable - Double */
    jvmti_GetLocalDouble,
    /*   26 : Set Local Variable - Object */
    jvmti_SetLocalObject,
    /*   27 : Set Local Variable - Int */
    jvmti_SetLocalInt,
    /*   28 : Set Local Variable - Long */
    jvmti_SetLocalLong,
    /*   29 : Set Local Variable - Float */
    jvmti_SetLocalFloat,
    /*   30 : Set Local Variable - Double */
    jvmti_SetLocalDouble,
    /*   31 : Create Raw Monitor */
    jvmti_CreateRawMonitor,
    /*   32 : Destroy Raw Monitor */
    jvmti_DestroyRawMonitor,
    /*   33 : Raw Monitor Enter */
    jvmti_RawMonitorEnter,
    /*   34 : Raw Monitor Exit */
    jvmti_RawMonitorExit,
    /*   35 : Raw Monitor Wait */
    jvmti_RawMonitorWait,
    /*   36 : Raw Monitor Notify */
    jvmti_RawMonitorNotify,
    /*   37 : Raw Monitor Notify All */
    jvmti_RawMonitorNotifyAll,
    /*   38 : Set Breakpoint */
    jvmti_SetBreakpoint,
    /*   39 : Clear Breakpoint */
    jvmti_ClearBreakpoint,
    /*   40 :	 RESERVED */
    NULL,
    /*   41 : Set Field Access Watch */
    jvmti_SetFieldAccessWatch,
    /*   42 : Clear Field Access Watch */
    jvmti_ClearFieldAccessWatch,
    /*   43 : Set Field Modification Watch */
    jvmti_SetFieldModificationWatch,
    /*   44 : Clear Field Modification Watch */
    jvmti_ClearFieldModificationWatch,
    /*   45 : Is Modifiable Class */
    jvmti_IsModifiableClass,
    /*   46 : Allocate */
    jvmti_Allocate,
    /*   47 : Deallocate */
    jvmti_Deallocate,
    /*   48 : Get Class Signature */
    jvmti_GetClassSignature,
    /*   49 : Get Class Status */
    jvmti_GetClassStatus,
    /*   50 : Get Source File Name */
    jvmti_GetSourceFileName,
    /*   51 : Get Class Modifiers */
    jvmti_GetClassModifiers,
    /*   52 : Get Class Methods */
    jvmti_GetClassMethods,
    /*   53 : Get Class Fields */
    jvmti_GetClassFields,
    /*   54 : Get Implemented Interfaces */
    jvmti_GetImplementedInterfaces,
    /*   55 : Is Interface */
    jvmti_IsInterface,
    /*   56 : Is Array Class */
    jvmti_IsArrayClass,
    /*   57 : Get Class Loader */
    jvmti_GetClassLoader,
    /*   58 : Get Object Hash Code */
    jvmti_GetObjectHashCode,
    /*   59 : Get Object Monitor Usage */
    jvmti_GetObjectMonitorUsage,
    /*   60 : Get Field Name (and Signature) */
    jvmti_GetFieldName,
    /*   61 : Get Field Declaring Class */
    jvmti_GetFieldDeclaringClass,
    /*   62 : Get Field Modifiers */
    jvmti_GetFieldModifiers,
    /*   63 : Is Field Synthetic */
    jvmti_IsFieldSynthetic,
    /*   64 : Get Method Name (and Signature) */
    jvmti_GetMethodName,
    /*   65 : Get Method Declaring Class */
    jvmti_GetMethodDeclaringClass,
    /*   66 : Get Method Modifiers */
    jvmti_GetMethodModifiers,
    /*   67 :	 RESERVED */
    NULL,
    /*   68 : Get Max Locals */
    jvmti_GetMaxLocals,
    /*   69 : Get Arguments Size */
    jvmti_GetArgumentsSize,
    /*   70 : Get Line Number Table */
    jvmti_GetLineNumberTable,
    /*   71 : Get Method Location */
    jvmti_GetMethodLocation,
    /*   72 : Get Local Variable Table */
    jvmti_GetLocalVariableTable,
    /*   73 : Set Native Method Prefix */
    jvmti_SetNativeMethodPrefix,
    /*   74 : Set Native Method Prefixes */
    jvmti_SetNativeMethodPrefixes,
    /*   75 : Get Bytecodes */
    jvmti_GetBytecodes,
    /*   76 : Is Method Native */
    jvmti_IsMethodNative,
    /*   77 : Is Method Synthetic */
    jvmti_IsMethodSynthetic,
    /*   78 : Get Loaded Classes */
    jvmti_GetLoadedClasses,
    /*   79 : Get Classloader Classes */
    jvmti_GetClassLoaderClasses,
    /*   80 : Pop Frame */
    jvmti_PopFrame,
    /*   81 : Force Early Return - Object */
    jvmti_ForceEarlyReturnObject,
    /*   82 : Force Early Return - Int */
    jvmti_ForceEarlyReturnInt,
    /*   83 : Force Early Return - Long */
    jvmti_ForceEarlyReturnLong,
    /*   84 : Force Early Return - Float */
    jvmti_ForceEarlyReturnFloat,
    /*   85 : Force Early Return - Double */
    jvmti_ForceEarlyReturnDouble,
    /*   86 : Force Early Return - Void */
    jvmti_ForceEarlyReturnVoid,
    /*   87 : Redefine Classes */
    jvmti_RedefineClasses,
    /*   88 : Get Version Number */
    jvmti_GetVersionNumber,
    /*   89 : Get Capabilities */
    jvmti_GetCapabilities,
    /*   90 : Get Source Debug Extension */
    jvmti_GetSourceDebugExtension,
    /*   91 : Is Method Obsolete */
    jvmti_IsMethodObsolete,
    /*   92 : Suspend Thread List */
    jvmti_SuspendThreadList,
    /*   93 : Resume Thread List */
    jvmti_ResumeThreadList,
    /*   94 :	 RESERVED */
    NULL,
    /*   95 :	 RESERVED */
    NULL,
    /*   96 :	 RESERVED */
    NULL,
    /*   97 :	 RESERVED */
    NULL,
    /*   98 :	 RESERVED */
    NULL,
    /*   99 :	 RESERVED */
    NULL,
    /*   100 : Get All Stack Traces */
    jvmti_GetAllStackTraces,
    /*   101 : Get Thread List Stack Traces */
    jvmti_GetThreadListStackTraces,
    /*   102 : Get Thread Local Storage */
    jvmti_GetThreadLocalStorage,
    /*   103 : Set Thread Local Storage */
    jvmti_SetThreadLocalStorage,
    /*   104 : Get Stack Trace */
    jvmti_GetStackTrace,
    /*   105 :  RESERVED */
    NULL,
    /*   106 : Get Tag */
    jvmti_GetTag,
    /*   107 : Set Tag */
    jvmti_SetTag,
    /*   108 : Force Garbage Collection */
    jvmti_ForceGarbageCollection,
    /*   109 : Iterate Over Objects Reachable From Object */
    jvmti_IterateOverObjectsReachableFromObject,
    /*   110 : Iterate Over Reachable Objects */
    jvmti_IterateOverReachableObjects,
    /*   111 : Iterate Over Heap */
    jvmti_IterateOverHeap,
    /*   112 : Iterate Over Instances Of Class */
    jvmti_IterateOverInstancesOfClass,
    /*   113 :  RESERVED */
    NULL,
    /*   114 : Get Objects With Tags */
    jvmti_GetObjectsWithTags,
    /*   115 : Follow References */
    jvmti_FollowReferences,
    /*   116 : Iterate Through Heap */
    jvmti_IterateThroughHeap,
    /*   117 :  RESERVED */
    NULL,
    /*   118 :  RESERVED */
    NULL,
    /*   119 :  RESERVED */
    NULL,
    /*   120 : Set JNI Function Table */
    jvmti_SetJNIFunctionTable,
    /*   121 : Get JNI Function Table */
    jvmti_GetJNIFunctionTable,
    /*   122 : Set Event Callbacks */
    jvmti_SetEventCallbacks,
    /*   123 : Generate Events */
    jvmti_GenerateEvents,
    /*   124 : Get Extension Functions */
    jvmti_GetExtensionFunctions,
    /*   125 : Get Extension Events */
    jvmti_GetExtensionEvents,
    /*   126 : Set Extension Event Callback */
    jvmti_SetExtensionEventCallback,
    /*   127 : Dispose Environment */
    jvmti_DisposeEnvironment,
    /*   128 : Get Error Name */
    jvmti_GetErrorName,
    /*   129 : Get JLocation Format */
    jvmti_GetJLocationFormat,
    /*   130 : Get System Properties */
    jvmti_GetSystemProperties,
    /*   131 : Get System Property */
    jvmti_GetSystemProperty,
    /*   132 : Set System Property */
    jvmti_SetSystemProperty,
    /*   133 : Get Phase */
    jvmti_GetPhase,
    /*   134 : Get Current Thread CPU Timer Information */
    jvmti_GetCurrentThreadCpuTimerInfo,
    /*   135 : Get Current Thread CPU Time */
    jvmti_GetCurrentThreadCpuTime,
    /*   136 : Get Thread CPU Timer Information */
    jvmti_GetThreadCpuTimerInfo,
    /*   137 : Get Thread CPU Time */
    jvmti_GetThreadCpuTime,
    /*   138 : Get Timer Information */
    jvmti_GetTimerInfo,
    /*   139 : Get Time */
    jvmti_GetTime,
    /*   140 : Get Potential Capabilities */
    jvmti_GetPotentialCapabilities,
    /*   141 :  RESERVED */
    NULL,
    /*   142 : Add Capabilities */
    jvmti_AddCapabilities,
    /*   143 : Relinquish Capabilities */
    jvmti_RelinquishCapabilities,
    /*   144 : Get Available Processors */
    jvmti_GetAvailableProcessors,
    /*   145 : Get Class Version Numbers */
    jvmti_GetClassVersionNumbers,
    /*   146 : Get Constant Pool */
    jvmti_GetConstantPool,
    /*   147 : Get Environment Local Storage */
    jvmti_GetEnvironmentLocalStorage,
    /*   148 : Set Environment Local Storage */
    jvmti_SetEnvironmentLocalStorage,
    /*   149 : Add To Bootstrap Class Loader Search */
    jvmti_AddToBootstrapClassLoaderSearch,
    /*   150 : Set Verbose Flag */
    jvmti_SetVerboseFlag,
    /*   151 : Add To System Class Loader Search */
    jvmti_AddToSystemClassLoaderSearch,
    /*   152 : Retransform Classes */
    jvmti_RetransformClasses,
    /*   153 : Get Owned Monitor Stack Depth Info */
    jvmti_GetOwnedMonitorStackDepthInfo,
    /*   154 : Get Object Size */
    jvmti_GetObjectSize
};

/*
 * Retrieve the JVMTI interface pointer.  This function is called by
 * CVMjniGetEnv.
 */

jint
CVMjvmtiGetInterface(JavaVM *vm, void **penv)
{
    CVMJvmtiContext *context;
    jvmtiError err;

    err = CVMjvmtiInitialize(vm);
    if (err != JVMTI_ERROR_NONE) {
	return JNI_ENOMEM;
    }
    context = CVMjvmtiCreateContext();
    if (context == NULL) {
	return JNI_ENOMEM;
    }

    /* Register the context: 
       NOTE: Currently, we support only one agentLib attachment, and therefore
       only one context per VM launch.  This code will have to change if we
       add support for multiple concurrent agentLibs later.
     */
    CVMglobals.jvmti.statics.context = context;

    /* Set the pointer to the JVMTI interpreter loop */
    CVMglobals.CVMgcUnsafeExecuteJavaMethodProcPtr =
	&CVMgcUnsafeExecuteJavaMethodJVMTI;

    /* Return the jvmtiEnv * ptr: */
    *penv = (void *)&context->jvmtiExternal;
    CVMjvmtiGetPotentialCapabilities(getCapabilities(context),
			       getProhibitedCapabilities(context),
			       &context->currentCapabilities);

    return JNI_OK;
}

/* Constructor for the CVMJvmtiContext. */
static CVMJvmtiContext *CVMjvmtiCreateContext()
{
    CVMJvmtiContext *context;
    context = (CVMJvmtiContext *)calloc(1, sizeof(CVMJvmtiContext));
    if (context == NULL) {
	return NULL;
    }

    context->jvmtiExternal = (jvmtiInterface_1 *)&CVMjvmti_Interface;
    context->isValid = CVM_TRUE;
    return context;
}

/* Destructor for the CVMJvmtiContext. */
int CVMjvmtiDestroyContext(CVMJvmtiContext *context)
{
    CVMassert(CVMglobals.jvmti.statics.context == context);
    CVMglobals.jvmti.statics.context = NULL;
    free(context);
    return JNI_OK;
}
