/*
 * @(#)jvmtiEnv.c	1.7 06/10/31
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
#include "javavm/include/bag.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/named_sys_monitor.h"
#include "generated/javavm/include/opcodes.h"
#include "generated/offsets/java_lang_Thread.h"
#include "generated/jni/java_lang_reflect_Modifier.h"
#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"
#include "javavm/export/jvmti.h"
#include "javavm/include/jvmti_jni.h"
#include "javavm/include/jvmtiEnv.h"
#include "javavm/include/jvmtiCapabilities.h"

#undef DEBUGGER_LOCK
#undef DEBUGGER_UNLOCK
#undef DEBUGGER_IS_LOCKED
#define DEBUGGER_LOCK(ee)	   CVM_DEBUGGER_LOCK(ee)
#define DEBUGGER_UNLOCK(ee)	   CVM_DEBUGGER_UNLOCK(ee)
#define DEBUGGER_IS_LOCKED(ee) CVM_DEBUGGER_IS_LOCKED(ee)
/* Convenience macros */
#define CVM_THREAD_LOCK(ee)	  CVMsysMutexLock(ee, &CVMglobals.threadLock)
#define CVM_THREAD_UNLOCK(ee) CVMsysMutexUnlock(ee, &CVMglobals.threadLock)

#define INITIAL_BAG_SIZE 4

#define VALID_CLASS(cb) { if ((cb)==NULL)		\
	  return JVMTI_ERROR_INVALID_CLASS; }

#define VALID_OBJECT(o) { if ((o)==NULL)		\
	  return JVMTI_ERROR_INVALID_OBJECT; }

#define DEBUG_ENABLED() { if (!CVMglobals.jvmtiDebuggingEnabled) return JVMTI_ERROR_ACCESS_DENIED; }

#define NULL_CHECK(_ptr, _error) { if ((_ptr) == NULL) return (_error); }
#define NOT_NULL(ptr)	{ if (ptr == NULL) return JVMTI_ERROR_NULL_POINTER; }
#define NOT_NULL2(ptr1,ptr2)   { if (ptr1 == NULL || ptr2 == NULL)	\
	  return JVMTI_ERROR_NULL_POINTER; }

#define ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(ptr, error_code)	\
  { if (NULL == (ptr)) return (error_code); }

#define ASSERT_NOT_NULL2_ELSE_EXIT_WITH_ERROR(ptr1, ptr2, error_code)	\
  { if (NULL == (ptr1) || NULL == (ptr2)) return (error_code); }

#define THREAD_OK(ee)  { if (ee == NULL)		\
	  return JVMTI_ERROR_UNATTACHED_THREAD; }

#define ALLOC(env, size, where) { jvmtiError allocErr =			 \
	  jvmti_Allocate(env, size, (unsigned char**)where);				 \
	if (allocErr != JVMTI_ERROR_NONE) return allocErr; }

#define ALLOC_WITH_CLEANUP_IF_FAILED(env, size, where, cleanup) {	 \
	jvmtiError allocErr = jvmti_Allocate(env, size, (unsigned char**)where); \
	if (allocErr != JVMTI_ERROR_NONE) {							\
	  cleanup;													\
	  return allocErr;											\
	}															\
  }


/*
  static JvmtiEnv *_retransformable_environments;
  static JvmtiEnv *_non_retransformable_environments;

  static bool			   _globally_initialized;
  static jvmtiPhase		   _phase;
*/

/* Error names */
const char* _jvmti_error_names[] = {
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

/* Event threaded */
const jboolean _jvmti_event_threaded[] = {
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


/* bits for standard events */

#define SINGLE_STEP_BIT (((jlong)1) << (JVMTI_EVENT_SINGLE_STEP - TOTAL_MIN_EVENT_TYPE_VAL))
#define	 FRAME_POP_BIT	(((jlong)1) << (JVMTI_EVENT_FRAME_POP - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  BREAKPOINT_BIT (((jlong)1) << (JVMTI_EVENT_BREAKPOINT - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  FIELD_ACCESS_BIT (((jlong)1) << (JVMTI_EVENT_FIELD_ACCESS - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  FIELD_MODIFICATION_BIT (((jlong)1) << (JVMTI_EVENT_FIELD_MODIFICATION - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  METHOD_ENTRY_BIT (((jlong)1) << (JVMTI_EVENT_METHOD_ENTRY - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  METHOD_EXIT_BIT (((jlong)1) << (JVMTI_EVENT_METHOD_EXIT - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  CLASS_FILE_LOAD_HOOK_BIT (((jlong)1) << (JVMTI_EVENT_CLASS_FILE_LOAD_HOOK - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  NATIVE_METHOD_BIND_BIT (((jlong)1) << (JVMTI_EVENT_NATIVE_METHOD_BIND - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  VM_START_BIT (((jlong)1) << (JVMTI_EVENT_VM_START - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  VM_INIT_BIT (((jlong)1) << (JVMTI_EVENT_VM_INIT - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  VM_DEATH_BIT (((jlong)1) << (JVMTI_EVENT_VM_DEATH - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  CLASS_LOAD_BIT (((jlong)1) << (JVMTI_EVENT_CLASS_LOAD - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  CLASS_PREPARE_BIT (((jlong)1) << (JVMTI_EVENT_CLASS_PREPARE - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  THREAD_START_BIT (((jlong)1) << (JVMTI_EVENT_THREAD_START - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  THREAD_END_BIT (((jlong)1) << (JVMTI_EVENT_THREAD_END - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  EXCEPTION_THROW_BIT (((jlong)1) << (JVMTI_EVENT_EXCEPTION - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  EXCEPTION_CATCH_BIT (((jlong)1) << (JVMTI_EVENT_EXCEPTION_CATCH - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  MONITOR_CONTENDED_ENTER_BIT (((jlong)1) << (JVMTI_EVENT_MONITOR_CONTENDED_ENTER - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  MONITOR_CONTENDED_ENTERED_BIT (((jlong)1) << (JVMTI_EVENT_MONITOR_CONTENDED_ENTERED - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  MONITOR_WAIT_BIT (((jlong)1) << (JVMTI_EVENT_MONITOR_WAIT - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  MONITOR_WAITED_BIT (((jlong)1) << (JVMTI_EVENT_MONITOR_WAITED - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  DYNAMIC_CODE_GENERATED_BIT (((jlong)1) << (JVMTI_EVENT_DYNAMIC_CODE_GENERATED - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  DATA_DUMP_BIT (((jlong)1) << (JVMTI_EVENT_DATA_DUMP_REQUEST - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  COMPILED_METHOD_LOAD_BIT (((jlong)1) << (JVMTI_EVENT_COMPILED_METHOD_LOAD - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  COMPILED_METHOD_UNLOAD_BIT (((jlong)1) << (JVMTI_EVENT_COMPILED_METHOD_UNLOAD - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  GARBAGE_COLLECTION_START_BIT (((jlong)1) << (JVMTI_EVENT_GARBAGE_COLLECTION_START - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  GARBAGE_COLLECTION_FINISH_BIT (((jlong)1) << (JVMTI_EVENT_GARBAGE_COLLECTION_FINISH - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  OBJECT_FREE_BIT (((jlong)1) << (JVMTI_EVENT_OBJECT_FREE - TOTAL_MIN_EVENT_TYPE_VAL))
#define	  VM_OBJECT_ALLOC_BIT (((jlong)1) << (JVMTI_EVENT_VM_OBJECT_ALLOC - TOTAL_MIN_EVENT_TYPE_VAL))

/* bits for extension events */
#define	  CLASS_UNLOAD_BIT (((jlong)1) << (EXT_EVENT_CLASS_UNLOAD - TOTAL_MIN_EVENT_TYPE_VAL))

#define	 MONITOR_BITS (MONITOR_CONTENDED_ENTER_BIT | MONITOR_CONTENDED_ENTERED_BIT | MONITOR_WAIT_BIT | MONITOR_WAITED_BIT)
#define	 EXCEPTION_BITS (EXCEPTION_THROW_BIT | EXCEPTION_CATCH_BIT)
#define	 INTERP_EVENT_BITS	(SINGLE_STEP_BIT | METHOD_ENTRY_BIT | METHOD_EXIT_BIT | FRAME_POP_BIT | FIELD_ACCESS_BIT | FIELD_MODIFICATION_BIT)
#define	 THREAD_FILTERED_EVENT_BITS (INTERP_EVENT_BITS | EXCEPTION_BITS | MONITOR_BITS | BREAKPOINT_BIT | CLASS_LOAD_BIT | CLASS_PREPARE_BIT | THREAD_END_BIT)
#define NEED_THREAD_LIFE_EVENTS	 (THREAD_FILTERED_EVENT_BITS | THREAD_START_BIT)
#define	 EARLY_EVENT_BITS (CLASS_FILE_LOAD_HOOK_BIT | VM_START_BIT | VM_INIT_BIT | VM_DEATH_BIT | NATIVE_METHOD_BIND_BIT | THREAD_START_BIT | THREAD_END_BIT |	DYNAMIC_CODE_GENERATED_BIT;)
static const jlong	GLOBAL_EVENT_BITS = ~THREAD_FILTERED_EVENT_BITS;



/*
 *	Threads
 */

CVMExecEnv*
jthreadToExecEnv(CVMExecEnv* ee, jthread thread)
{
  CVMJavaLong eetop;

  if ((thread == NULL) || CVMID_icellIsNull(thread)) {
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

/*
 * clean up breakpoint - doesn't actually remove it. 
 * lock must be held
 */
CVMBool
clear_bkpt(JNIEnv *env, struct bkpt *bp)
{
  CVMassert(*(bp->pc) == opc_breakpoint);
  *(bp->pc) = (CVMUint8)(bp->opcode);
	
  /* 
   * De-reference the enclosing class so that it's GC 
   * is no longer prevented by this breakpoint. 
   */
  (*env)->DeleteGlobalRef(env, bp->classRef);
  return CVM_TRUE;
}

CVMClassBlock *object2class(CVMExecEnv *ee, jclass clazz)
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

int jvmti_external_offset() {
  return CVMoffsetof(JvmtiEnv, _jvmti_external); 
};

#define CHECK_JVMTI_ENV	 {					   \
	JvmtiEnv* jvmti_env =					   \
	  (JvmtiEnv*)((int)jvmtienv - CVMoffsetof(JvmtiEnv, _jvmti_external));	 \
	if (!jvmti_env->_is_valid) {			   \
	  return JVMTI_ERROR_INVALID_ENVIRONMENT;  \
	}										   \
  }

/*
 * Memory Management functions
 */ 

static jvmtiError JNICALL
jvmti_Allocate(jvmtiEnv* jvmtienv,
			   jlong size,
			   unsigned char** mem_ptr) {

  unsigned char *mem;
  CVMJavaInt intSize;
  CHECK_JVMTI_ENV
	/*	JVMTI_AllocHook alloc_hook = CVMglobals.jvmtiStatics.allocHook; */
	

  DEBUG_ENABLED();
  NOT_NULL(mem_ptr);
  /*  if (alloc_hook != NULL) {
	return alloc_hook(size, memPtr);
  }
  */
  /* %comment: k014 */
  intSize = CVMlong2Int(size);
  mem = (unsigned char*)malloc(intSize);
  if (mem == NULL) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
  }
  *mem_ptr = mem;
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_Deallocate(jvmtiEnv* env,
				 unsigned char* mem) {

  /*	JVMTI_DeallocHook deallocHook = CVMglobals.jvmtiStatics.deallocHook; */


  DEBUG_ENABLED();
  NOT_NULL(mem);
  /*
  if (deallocHook != NULL) {
  return deallocHook(mem);
  }
  */
  free(mem);
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

static jvmtiError JNICALL
jvmti_GetThreadState(jvmtiEnv* jvmtienv,
					 jthread thread,
					 jint* thread_state_ptr) {

  /* %comment: k029 */
  CVMExecEnv* ee = CVMgetEE();
  CVMExecEnv* targetEE;
  CHECK_JVMTI_ENV

  /* NOTE:  Added this lock. This used to be a SYSTHREAD call,
	 which I believe is unsafe unless you have the thread queue
	 lock, because the thread might exit after you get a handle to
	 its EE. (Must file bug) */
  CVM_THREAD_LOCK(ee);
  /* %comment: k035 */
  targetEE = jthreadToExecEnv(ee, thread);
  if (targetEE == NULL) {
	*thread_state_ptr = JVMTI_ERROR_INVALID_THREAD;
  } else {
	*thread_state_ptr = JVMTI_THREAD_STATE_ALIVE |
	  ((targetEE->threadState & CVM_THREAD_SUSPENDED) ?
	   JVMTI_THREAD_STATE_SUSPENDED : 0);
  }
  CVM_THREAD_UNLOCK(ee);
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetCurrentThread(jvmtiEnv* env,
					   jthread* thread_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}

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

  jvmtiError rc = JVMTI_ERROR_NONE;
  CVMExecEnv* ee = CVMgetEE();
  CVMJavaLong sz;
  CVMInt32 i;
  JNIEnv *env = CVMexecEnv2JniEnv(ee);
  CHECK_JVMTI_ENV

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
  rc = jvmti_Allocate(jvmtienv, sz, (unsigned char**)threadsPtr);
  if (rc == JVMTI_ERROR_NONE) {

	jthread *tempPtr = *threadsPtr;
	i = 0;
	/* fill in the threads */
	CVM_WALK_ALL_THREADS(ee, currentEE, {
		if (!CVMID_icellIsNull(CVMcurrentThreadICell(currentEE))) {
		  tempPtr[i] = (*env)->NewLocalRef(env, CVMcurrentThreadICell(currentEE));
		  i++;
		}
	  });
  }

  CVM_THREAD_UNLOCK(ee);
  return rc;
}

/* Suspend the specified thread.
 * Errors: JVMTI_ERROR_INVALID_THREAD, JVMTI_ERROR_THREAD_SUSPENDED,
 * JVMTI_ERROR_VM_DEAD 
 */

static jvmtiError JNICALL
jvmti_SuspendThread(jvmtiEnv* jvmtienv,
					jthread thread) {
  jvmtiError rc = JVMTI_ERROR_NONE;
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv* env;
  /* NOTE that this looks unsafe. However, later code will get the
	 EE out of the Java thread object again, this time under
	 protection of the thread lock, before doing any real work with
	 the EE. */
  CVMExecEnv* target = jthreadToExecEnv(ee, thread);

  CHECK_JVMTI_ENV


  DEBUG_ENABLED();
  THREAD_OK(ee);

  env = CVMexecEnv2JniEnv(ee);
  if (target == ee) {
	/*
	 * Suspending self. Don't need to check for previous
	 * suspension.	Shouldn't grab any locks here. (NOTE: JDK's
	 * JVM_SuspendThread recomputes whether it's suspending itself
	 * down in JDK's threads.c)
	 */
	JVM_SuspendThread(env, (jobject)thread);
	rc = JVMTI_ERROR_NONE;
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
	target = jthreadToExecEnv(ee, thread);
	if (target == NULL) {
	  /*
	   * It has finished running and is a zombie
	   */
	  rc = JVMTI_ERROR_INVALID_THREAD;
	} else {
	  if (target->threadState & CVM_THREAD_SUSPENDED) {
		rc = JVMTI_ERROR_THREAD_SUSPENDED;
	  } else {
		JVM_SuspendThread(env, (jobject)thread);
		rc = JVMTI_ERROR_NONE;
	  }
	}
	CVMlocksForThreadSuspendRelease(ee);
  }
  return rc;
}


static jvmtiError JNICALL
jvmti_SuspendThreadList(jvmtiEnv* jvmtienv,
						jint request_count,
						const jthread* request_list,
						jvmtiError* results) {
  CVMExecEnv* ee = CVMgetEE();
  CVMExecEnv* target;
  int i;

  CHECK_JVMTI_ENV
  for (i = 0; i < request_count; i++) {
	target = jthreadToExecEnv(ee, request_list[i]);
	if (target == NULL) {
	  results[i] = JVMTI_ERROR_INVALID_THREAD;
	  continue;
	}
	results[i] = jvmti_SuspendThread(jvmtienv, request_list[i]);
  }
  return JVMTI_ERROR_NONE;
}

/* Resume the specified thread.
 * Errors: JVMTI_ERROR_INVALID_THREAD, JVMTI_ERROR_THREAD_NOT_SUSPENDED, 
 * JVMTI_ERROR_INVALID_TYPESTATE,
 * JVMTI_ERROR_VM_DEAD
 */

static jvmtiError JNICALL
jvmti_ResumeThread(jvmtiEnv* jvmtienv,
				   jthread thread) {
  jvmtiError rc = JVMTI_ERROR_THREAD_NOT_SUSPENDED;
  CVMExecEnv* ee = CVMgetEE();
  CVMExecEnv* target;
  JNIEnv* env;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
	
  env = CVMexecEnv2JniEnv(ee);

  CVM_THREAD_LOCK(ee);

  target = jthreadToExecEnv(ee, thread);
  if (target != NULL) {
	if (target->threadState & CVM_THREAD_SUSPENDED) {
	  JVM_ResumeThread(env, (jobject) thread);
	  rc = JVMTI_ERROR_NONE;
	}
  }

  CVM_THREAD_UNLOCK(ee);

  return rc;
}


static jvmtiError JNICALL
jvmti_ResumeThreadList(jvmtiEnv* jvmtienv,
					   jint request_count,
					   const jthread* request_list,
					   jvmtiError* results) {
  CVMExecEnv* ee = CVMgetEE();
  CVMExecEnv* target;
  int i;
  CHECK_JVMTI_ENV

  for (i = 0; i < request_count; i++) {
	target = jthreadToExecEnv(ee, request_list[i]);
	if (target == NULL) {
	  results[i] = JVMTI_ERROR_INVALID_THREAD;
	  continue;
	}
	results[i] = jvmti_ResumeThread(jvmtienv, request_list[i]);
  }

  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_StopThread(jvmtiEnv* env,
				 jthread thread,
				 jobject exception) {
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_InterruptThread(jvmtiEnv* jvmtienv,
					  jthread thread) {
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env;	  
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(ee);

  env = CVMexecEnv2JniEnv(ee);

  JVM_Interrupt(env, thread);

  return JVMTI_ERROR_NONE;
 }

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


static jvmtiError JNICALL
jvmti_GetThreadInfo(jvmtiEnv* jvmtienv,
					jthread thread,
					jvmtiThreadInfo* info_ptr) {
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
	
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(info_ptr);
  THREAD_OK(ee);

  env = CVMexecEnv2JniEnv(ee);

  if (thread == NULL) {
	thread = (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
  }
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
  info_ptr->priority = CVMjniGetIntField(env, thread, priorityID);
  info_ptr->is_daemon = CVMjniGetBooleanField(env, thread, daemonID);
  groupObj = CVMjniGetObjectField(env, thread, groupID);
  info_ptr->thread_group = groupObj == NULL ? NULL :
	(jthreadGroup)((*env)->NewLocalRef(env, groupObj));
  loaderObj = CVMjniGetObjectField(env, thread, loaderID);
  info_ptr->context_class_loader = loaderObj == NULL ? NULL :
	(*env)->NewLocalRef(env, loaderObj);
  
  {	  /* copy the thread name */
	jint uniLen = (*env)->GetArrayLength(env, nmObj);
	jchar *uniChars = (*env)->GetCharArrayElements(env, nmObj, NULL);
	jint utfLen = lengthCharsUTF(uniLen, uniChars);
	CVMJavaLong sz = CVMint2Long(utfLen+1);
	ALLOC(jvmtienv, sz, &(info_ptr->name));
	setBytesCharsUTF(uniLen, uniChars, info_ptr->name);
	(*env)->ReleaseCharArrayElements(env, nmObj, uniChars, JNI_ABORT);
  }
   
  return JVMTI_ERROR_NONE;
}

/* NOTE: We implement fast locking, so we don't implement
   GetOwnedMonitorInfo or GetCurrentContendedMonitor. */

static jvmtiError JNICALL
jvmti_GetOwnedMonitorInfo(jvmtiEnv* env,
						  jthread thread,
						  jint* owned_monitor_count_ptr,
						  jobject** owned_monitors_ptr) {
  *owned_monitor_count_ptr = 0;
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetOwnedMonitorStackDepthInfo(jvmtiEnv* env,
									jthread thread,
									jint* monitor_info_count_ptr,
									jvmtiMonitorStackDepthInfo** monitor_info_ptr) {
  *monitor_info_count_ptr = 0;
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetCurrentContendedMonitor(jvmtiEnv* env,
								 jthread thread,
								 jobject* monitor_ptr) {
  *monitor_ptr = NULL;
  return JVMTI_ERROR_NONE;
}


void start_function_wrapper(void *arg) {
  ThreadNode *node = (ThreadNode *)arg;
  jvmtiStartFunction proc = node->startFunction;
  void *proc_arg = node->startFunctionArg;
  JvmtiEnv *env = node->_env;

  proc(&env->_jvmti_external, CVMexecEnv2JniEnv(CVMgetEE()), proc_arg);

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
					 void* arg,
					 jint priority) {
  ThreadNode *node;
  JNIEnv* env;
  CVMExecEnv* ee = CVMgetEE();
  JvmtiEnv* jvmti_env =
	(JvmtiEnv*)((int)jvmtienv - CVMoffsetof(JvmtiEnv, _jvmti_external));
  if (!jvmti_env->_is_valid) {
	return JVMTI_ERROR_INVALID_ENVIRONMENT;
  }

  DEBUG_ENABLED();
  NOT_NULL(proc);

  env = CVMexecEnv2JniEnv(ee);
  /*
   * Make sure a thread hasn't been run for this thread object 
   * before.
   */
  if ((thread == NULL) ||
	  (jthreadToExecEnv(ee, thread) != NULL)) {
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
  node->_env = jvmti_env;
  /* %comment: k021 */
  CVMID_fieldWriteInt(ee, (CVMObjectICell*) thread,
					  CVMoffsetOfjava_lang_Thread_daemon,
					  1);
  CVMID_fieldWriteInt(ee, (CVMObjectICell*) thread,
					  CVMoffsetOfjava_lang_Thread_priority,
					  priority);

  JVM_StartSystemThread(env, thread, start_function_wrapper, (void *)node);
  if ((*env)->ExceptionCheck(env)) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
  }

  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_SetThreadLocalStorage(jvmtiEnv* jvmtienv,
							jthread thread,
							const void* data) {
  ThreadNode *node;
  CVMExecEnv *ee = CVMgetEE();
  CHECK_JVMTI_ENV

#ifdef CVM_DEBUG_ASSERTS
  CVMassert (ee != NULL);
  CVMassert(CVMD_isgcSafe(ee));
#endif
  node = CVMjvmtiFindThread(ee, thread);
  if (node == NULL) {
	return JVMTI_ERROR_THREAD_NOT_ALIVE;
  }
  node->jvmtiPrivateData = (void*)data;
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetThreadLocalStorage(jvmtiEnv* jvmtienv,
							jthread thread,
							void** data_ptr) {
  ThreadNode *node;
  CVMExecEnv *ee = CVMgetEE();
  CHECK_JVMTI_ENV

#ifdef CVM_DEBUG_ASSERTS
	CVMassert (ee != NULL);
	CVMassert(CVMD_isgcSafe(ee));
#endif
  node = CVMjvmtiFindThread(ee, thread);
  if (node == NULL) {
	return JVMTI_ERROR_THREAD_NOT_ALIVE;
  }
  *data_ptr =  node->jvmtiPrivateData;
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
						 jint* group_count_ptr,
						 jthreadGroup** groups_ptr) {
  jobject sysgrp;
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env;	  
  CVMJavaLong sz;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL2(group_count_ptr, groups_ptr);
  THREAD_OK(ee);

  env = CVMexecEnv2JniEnv(ee);

  /* We only have the one top group */
  sz = CVMint2Long(sizeof(jthreadGroup));
  ALLOC(jvmtienv, sz, groups_ptr);
  *group_count_ptr = 1;

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
  **groups_ptr = (jthreadGroup) sysgrp;
   
  return JVMTI_ERROR_NONE;
}

/* Return via "info_ptr" thread group info.
 * Errors: JVMTI_ERROR_INVALID_THREAD_GROUP, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetThreadGroupInfo(jvmtiEnv* jvmtienv,
						 jthreadGroup group,
						 jvmtiThreadGroupInfo* info_ptr) {
  static jfieldID parentID;
  static jfieldID nameID = 0;
  static jfieldID maxPriorityID;
  static jfieldID daemonID;
  jstring nmString;
  jobject parentObj;
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env;	  
  
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(info_ptr);
  THREAD_OK(ee);

  env = CVMexecEnv2JniEnv(ee);
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
  info_ptr->parent = parentObj == NULL? NULL : 
	(jthreadGroup)((*env)->NewLocalRef(env, parentObj));
  nmString = (jstring)(CVMjniGetObjectField(env, group, nameID));
  info_ptr->max_priority = CVMjniGetIntField(env, group, 
											maxPriorityID);
  info_ptr->is_daemon = CVMjniGetBooleanField(env, group, daemonID);

  {	  /* copy the thread group name */
	jint nmLen = (*env)->GetStringUTFLength(env, nmString);
	const char *nmValue = (*env)->GetStringUTFChars(env, nmString, NULL);
	CVMJavaLong sz = CVMint2Long(nmLen+1);
	ALLOC(jvmtienv, sz, &(info_ptr->name));
	strcpy(info_ptr->name, nmValue);
	(*env)->ReleaseStringUTFChars(env, nmString, nmValue);
  }
   
  return JVMTI_ERROR_NONE;
}

static jvmtiError 
objectArrayToArrayOfObject(jvmtiEnv *jvmtienv, JNIEnv *env, jint cnt, jobject **destPtr,
						   jobjectArray arr) {
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

/* Return via "thread_count_ptr" the number of threads in this thread group.
 * Return via "threads_ptr" the threads in this thread group.
 * Return via "group_count_ptr" the number of thread groups in this thread group.
 * Return via "groups_ptr" the thread groups in this thread group.
 * Errors: JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetThreadGroupChildren(jvmtiEnv* jvmtienv,
							 jthreadGroup group,
							 jint* thread_count_ptr,
							 jthread** threads_ptr,
							 jint* group_count_ptr,
							 jthreadGroup** groups_ptr) {
  static jfieldID nthreadsID = 0;
  static jfieldID threadsID;
  static jfieldID ngroupsID;
  static jfieldID groupsID;
  jvmtiError rc;
  jint nthreads;
  jobject threads;
  jint ngroups;
  jobject groups;
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env;	  
	
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL2(thread_count_ptr, threads_ptr);
  NOT_NULL2(group_count_ptr, groups_ptr);
  THREAD_OK(ee);

  env = CVMexecEnv2JniEnv(ee);
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

  rc = objectArrayToArrayOfObject(jvmtienv, env, nthreads, threads_ptr, threads);
  if (rc == JVMTI_ERROR_NONE) {
	rc = objectArrayToArrayOfObject(jvmtienv, env, ngroups, groups_ptr, groups);
	/* deallocate all of threads list on error */
	if (rc != JVMTI_ERROR_NONE) {
	  jvmti_Deallocate(jvmtienv, (unsigned char *)*threads_ptr);
	} else {
	  *thread_count_ptr = nthreads;
	  *group_count_ptr = ngroups;
	}
  }
  return rc;
}


/*
 * Stack Frame functions
 */ 

static jvmtiError JNICALL
jvmti_GetStackTrace(jvmtiEnv* env,
					jthread thread,
					jint start_depth,
					jint max_frame_count,
					jvmtiFrameInfo* frame_buffer,
					jint* count_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetAllStackTraces(jvmtiEnv* env,
						jint max_frame_count,
						jvmtiStackInfo** stack_info_ptr,
						jint* thread_count_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetThreadListStackTraces(jvmtiEnv* env,
							   jint thread_count,
							   const jthread* thread_list,
							   jint max_frame_count,
							   jvmtiStackInfo** stack_info_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}

/* Return via "count_ptr" the number of frames in the thread's call stack.
 * Errors: JVMTI_ERROR_INVALID_THREAD, JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetFrameCount(jvmtiEnv* jvmtienv,
					jthread thread,
					jint* count_ptr) {
  /* NOTE that we do not seize the thread lock, because this
	 function requires that the target thead be suspended, which
	 means it can't exit out from under us once we've gotten a
	 pointer to its EE. Once we can query a thread's status, we can
	 put in an assert. */
  CVMExecEnv* ee = CVMgetEE();
  CVMExecEnv* thread_ee;
  CVMFrame* frame;
  jint count = 0;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();

  thread_ee = jthreadToExecEnv(ee, thread);

  if (thread_ee == 0) {
	return JVMTI_ERROR_INVALID_THREAD;
  }

  NOT_NULL(count_ptr);

  frame = CVMeeGetCurrentFrame(thread_ee);
  frame = CVMgetCallerFrameSpecial(frame, 0, CVM_FALSE);
  /* Workaround for bug 4196771, derived from util.c from older CVM jdwp src */
#if 0
  while (frame != NULL) {
	/* skip pseudo frames */
	if (frame->mb != 0) {
	  count++;
	}
	frame = CVMframePrev(frame);
  }
#else
  if (frame == NULL) {
	*count_ptr = 0;
	return JVMTI_ERROR_NONE;
  }
  count = 1;	 /* for current frame */
  while (JNI_TRUE) {
	frame = CVMgetCallerFrameSpecial(frame, 1, CVM_FALSE);
	if (frame == NULL) {
	  break;
	}
	++count;
  }
#endif
  *count_ptr = count;
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_PopFrame(jvmtiEnv* env,
			   jthread thread) {
  return JVMTI_ERROR_ACCESS_DENIED;
}

static jvmtiError get_frame(CVMExecEnv *ee, jint depth, CVMFrame **frame) {

  CVMFrame *frm;
  if (ee == NULL) {
	/* Thread hasn't any state yet. */
	return JVMTI_ERROR_INVALID_THREAD;
  }
  frm = CVMeeGetCurrentFrame(ee);

  frm = CVMgetCallerFrameSpecial(frm, depth, CVM_FALSE);
  *frame = frm;
  if (frm == NULL) {
	return JVMTI_ERROR_NO_MORE_FRAMES;
  }
  return JVMTI_ERROR_NONE;
}

/* Return via "method_ptr" the active method in the 
 * specified frame. Return via "location_ptr" the index of the
 * currently executing instruction within the specified frame,
 * return negative one if location not available.
 * Errors: JVMTI_ERROR_OPAQUE_FRAME, JVMTI_ERROR_NULL_POINTER
 * JVMTI_ERROR_NO_MORE_FRAMES
 */

static jvmtiError JNICALL
jvmti_GetFrameLocation(jvmtiEnv* jvmtienv,
					   jthread thread,
					   jint depth,
					   jmethodID* method_ptr,
					   jlocation* location_ptr) {
  CVMFrame* frame;
  CVMMethodBlock* mb;
  CVMExecEnv* ee;
  jvmtiError err;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL2(method_ptr, location_ptr);

  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }
  mb = frame->mb;
  if (mb == NULL) {
	return JVMTI_ERROR_OPAQUE_FRAME;
  }
  *method_ptr = (jmethodID)mb;

  /* The jmd for <clinit> is freed after it is run. Can't take
	 any chances. */
  if (CVMmbIs(mb, NATIVE) ||
	  (CVMmbJmd(mb) == NULL)) {
	*location_ptr = CVMint2Long(-1);
  } else {
	*location_ptr = CVMint2Long(CVMframePc(frame) - CVMmbJavaCode(mb));
  }
  return JVMTI_ERROR_NONE;
}

/* Send a JVMTI_EVENT_FRAME_POP event when the specified frame is
 * popped from the stack.  The event is sent after the pop has occurred.
 * Errors: JVMTI_ERROR_OPAQUE_FRAME, JVMTI_ERROR_NO_MORE_FRAMES,
 * JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_NotifyFramePop(jvmtiEnv* jvmtienv,
					 jthread thread,
					 jint depth) {
  CVMFrame* frame;
  CVMFrame* framePrev;
  CVMMethodBlock* mb;
  jvmtiError err = JVMTI_ERROR_NONE;
  CVMExecEnv* ee;

  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }
  mb = frame->mb;
  if (mb == NULL) {
	return JVMTI_ERROR_NO_MORE_FRAMES;
  }
  if (CVMmbIs(mb, NATIVE)) {
	return JVMTI_ERROR_OPAQUE_FRAME;
  }
  if (!CVMframeIsInterpreter(frame)) {
	return JVMTI_ERROR_OPAQUE_FRAME;
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
	  return JVMTI_ERROR_OPAQUE_FRAME;
	}

	DEBUGGER_LOCK(ee);
	/* check if one already exists */
	if (CVMbagFind(CVMglobals.jvmtiStatics.framePops, frame) == NULL) {	   
	  /* allocate space for it */
	  struct fpop *fp = CVMbagAdd(CVMglobals.jvmtiStatics.framePops);
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
	  }
	}
	DEBUGGER_UNLOCK(ee);		  
  }
  return err;
}


/*
 * Force Early Return functions
 */ 

static jvmtiError JNICALL
jvmti_ForceEarlyReturnObject(jvmtiEnv* env,
							 jthread thread,
							 jobject value) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_ForceEarlyReturnInt(jvmtiEnv* env,
						  jthread thread,
						  jint value) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_ForceEarlyReturnLong(jvmtiEnv* env,
						   jthread thread,
						   jlong value) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_ForceEarlyReturnFloat(jvmtiEnv* env,
							jthread thread,
							jfloat value) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_ForceEarlyReturnDouble(jvmtiEnv* env,
							 jthread thread,
							 jdouble value) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_ForceEarlyReturnVoid(jvmtiEnv* env,
						   jthread thread) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Heap functions
 */ 

static jvmtiError JNICALL
jvmti_FollowReferences(jvmtiEnv* env,
					   jint heap_filter,
					   jclass klass,
					   jobject initial_object,
					   const jvmtiHeapCallbacks* callbacks,
					   const void* user_data) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_IterateThroughHeap(jvmtiEnv* env,
						 jint heap_filter,
						 jclass klass,
						 const jvmtiHeapCallbacks* callbacks,
						 const void* user_data) {
  return JVMTI_ERROR_ACCESS_DENIED;
}

static TagNode *objectsByRef[HASH_SLOT_COUNT];

static jint hashRef(jobject ref) 
{
	jint hashCode = JVM_IHashCode(CVMexecEnv2JniEnv(CVMgetEE()), ref);
	if (hashCode < 0) {
	  hashCode = -hashCode;
	}
	return hashCode % HASH_SLOT_COUNT;
}

static jvmtiError JNICALL
jvmti_GetTag(jvmtiEnv* jvmtienv,
			 jobject object,
			 jlong* tag_ptr) {
  TagNode *node;
  TagNode *prev;
  JNIEnv *env;
  jint slot = hashRef(object);
  CHECK_JVMTI_ENV


  env = CVMexecEnv2JniEnv(CVMgetEE());
  node = objectsByRef[slot];
  prev = NULL;

  *tag_ptr = 0L;
  while (node != NULL) {
	if ((*env)->IsSameObject(env, object, node->ref)) {
	  break;
	}
	prev = node;
	node = node->next;
  }
  if (node == NULL) {
	return JVMTI_ERROR_INVALID_OBJECT;
  }
  *tag_ptr = node->tag;
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_SetTag(jvmtiEnv* jvmtienv,
			 jobject object,
			 jlong tag) {

  JNIEnv *env;
  jint slot = hashRef(object);
  TagNode *node, *prev;
  CHECK_JVMTI_ENV

  /*
   * Add to reference hashtable 
   */
  node = objectsByRef[slot];
  prev = NULL;

  env = CVMexecEnv2JniEnv(CVMgetEE());
  while (node != NULL) {
	if ((*env)->IsSameObject(env, object, node->ref)) {
	  break;
	}
	prev = node;
	node = node->next;
  }
  if (node == NULL) {
	if (tag == 0L) {
	  /* clearing tag on non-existent object */
	  return JVMTI_ERROR_INVALID_OBJECT;
	}
	ALLOC(jvmtienv, sizeof(TagNode), (void *)&node);
  }
  if (tag == 0L) {
	/* clearing tag, deallocate it */
	if (prev == NULL) {
	  objectsByRef[slot] = node->next;
	} else {
	  prev->next = node->next;
	}
	jvmti_Deallocate(jvmtienv, (unsigned char *)node);
  } else {
	node->next = objectsByRef[slot];
	node->ref = object;
	node->tag = tag;
	objectsByRef[slot] = node;
  }
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetObjectsWithTags(jvmtiEnv* env,
						 jint tag_count,
						 const jlong* tags,
						 jint* count_ptr,
						 jobject** object_result_ptr,
						 jlong** tag_result_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_ForceGarbageCollection(jvmtiEnv* env) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Heap (1.0) functions
 */ 

static jvmtiError JNICALL
jvmti_IterateOverObjectsReachableFromObject(jvmtiEnv* env,
											jobject object,
											jvmtiObjectReferenceCallback object_reference_callback,
											const void* user_data) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_IterateOverReachableObjects(jvmtiEnv* env,
								  jvmtiHeapRootCallback heap_root_callback,
								  jvmtiStackReferenceCallback stack_ref_callback,
								  jvmtiObjectReferenceCallback object_ref_callback,
								  const void* user_data) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_IterateOverHeap(jvmtiEnv* env,
					  jvmtiHeapObjectFilter object_filter,
					  jvmtiHeapObjectCallback heap_object_callback,
					  const void* user_data) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_IterateOverInstancesOfClass(jvmtiEnv* env,
								  jclass klass,
								  jvmtiHeapObjectFilter object_filter,
								  jvmtiHeapObjectCallback heap_object_callback,
								  const void* user_data) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Local Variable functions
 */ 

/* Return via "siPtr" the stack item at the specified slot.	 Errors:
 * JVMTI_ERROR_NO_MORE_FRAMES, JVMTI_ERROR_INVALID_SLOT,
 * JVMTI_ERROR_TYPE_MISMATCH, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OPAQUE_FRAME. Must be called from GC-unsafe code and is
 * guaranteed not to become GC-safe internally. (Stack pointers are
 * invalid across GC-safe points.)
 */
static jvmtiError
jvmtiGCUnsafeGetSlot(jframeID jframe, jint slot, CVMJavaVal32 **siPtr)
{
	CVMFrame* frame = (CVMFrame*) jframe;
	CVMSlotVal32* vars = NULL;
	CVMMethodBlock* mb;

	if (!CVMframeIsInterpreter(frame)) {
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

/* Return via "value_ptr" the value of the local variable at the
 * specified slot.
 * Errors: JVMTI_ERROR_INVALID_SLOT, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OPAQUE_FRAME, JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetLocalObject(jvmtiEnv* jvmtienv,
					 jthread thread,
					 jint depth,
					 jint slot,
					 jobject* value_ptr) {
  CVMJavaVal32* si;
  CVMFrame* frame;
  jvmtiError err = JVMTI_ERROR_NONE;
  CVMExecEnv* ee;
  JNIEnv *env;	  
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(value_ptr);
  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }
  env = CVMexecEnv2JniEnv(ee);

  CVMID_localrootBegin(ee); {
	CVMID_localrootDeclare(CVMObjectICell, obj);

	CVMD_gcUnsafeExec(ee, {
		err = jvmtiGCUnsafeGetSlot((jframeID)frame, slot, &si);
		if (err == JVMTI_ERROR_NONE) {
		  CVMID_icellAssignDirect(ee, obj, &si->r);
		}
	  });
	if (err == JVMTI_ERROR_NONE) {
	  /* NOTE: at this point, "obj" could contain a null direct
		 object. We must not allow these to "escape" to the rest
		 of the system. */
	  /* %comment: k012 */
	  if (!CVMID_icellIsNull(obj)) {
		*value_ptr = (*env)->NewLocalRef(env, obj);
		if (*value_ptr == NULL) {
		  err = JVMTI_ERROR_OUT_OF_MEMORY;
		}
	  } else {
		*value_ptr = NULL;
	  }
	}
  } CVMID_localrootEnd();

  return err;
}

/* Return via "value_ptr" the value of the local variable at the
 * specified slot.
 * Errors: JVMTI_ERROR_INVALID_SLOT, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OPAQUE_FRAME, JVMTI_ERROR_NO_MORE_FRAMES
 */

static jvmtiError JNICALL
jvmti_GetLocalInt(jvmtiEnv* jvmtienv,
				  jthread thread,
				  jint depth,
				  jint slot,
				  jint* value_ptr) {
  CVMJavaVal32* si;
  jvmtiError err;
  CVMFrame* frame;
  CVMExecEnv* ee = CVMgetEE();
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(value_ptr);
  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }
  CVMD_gcUnsafeExec(ee, {
	  err = jvmtiGCUnsafeGetSlot((jframeID)frame, slot, &si);
	});

  if (err == JVMTI_ERROR_NONE) {
	*value_ptr = (jint)si->i;
  }
  return err;
}


static jvmtiError JNICALL
jvmti_GetLocalLong(jvmtiEnv* jvmtienv,
				   jthread thread,
				   jint depth,
				   jint slot,
				   jlong* value_ptr) {
  CVMJavaVal32* si;
  CVMFrame* frame;
  jvmtiError err;
  CVMExecEnv* ee;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(value_ptr);
  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }

  CVMD_gcUnsafeExec(ee, {
	  err = jvmtiGCUnsafeGetSlot((jframeID)frame, slot, &si);
	});

  if (err == JVMTI_ERROR_NONE) {
	*value_ptr = CVMjvm2Long(&si->raw);
  }
  return err;
}


static jvmtiError JNICALL
jvmti_GetLocalFloat(jvmtiEnv* jvmtienv,
					jthread thread,
					jint depth,
					jint slot,
					jfloat* value_ptr) {
  CVMJavaVal32* si;
  CVMFrame* frame;
  jvmtiError err;
  CVMExecEnv* ee;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(value_ptr);
  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }

  CVMD_gcUnsafeExec(ee, {
	  err = jvmtiGCUnsafeGetSlot((jframeID)frame, slot, &si);
	});

  if (err == JVMTI_ERROR_NONE) {
	*value_ptr = (jfloat)si->f;
  }
  return err;
}


static jvmtiError JNICALL
jvmti_GetLocalDouble(jvmtiEnv* jvmtienv,
					 jthread thread,
					 jint depth,
					 jint slot,
					 jdouble* value_ptr) {
  CVMJavaVal32* si;
  jvmtiError err;
  CVMFrame* frame;
  CVMExecEnv* ee;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(value_ptr);

  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }
  CVMD_gcUnsafeExec(ee, {
	  err = jvmtiGCUnsafeGetSlot((jframeID)frame, slot, &si);
	});

  if (err == JVMTI_ERROR_NONE) {
	*value_ptr = CVMjvm2Double(&si->raw);
  }
  return err;
}

/* Set the value of the local variable at the specified slot.
 * Errors: JVMTI_ERROR_INVALID_SLOT, JVMTI_ERROR_NO_MORE_FRAMES
 * JVMTI_ERROR_OPAQUE_FRAME
 */

static jvmtiError JNICALL
jvmti_SetLocalObject(jvmtiEnv* jvmtienv,
					 jthread thread,
					 jint depth,
					 jint slot,
					 jobject value) {
  CVMJavaVal32* si;
  CVMExecEnv* ee;
  CVMFrame* frame;
  jvmtiError err;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }

  CVMD_gcUnsafeExec(ee, {
	  err = jvmtiGCUnsafeGetSlot((jframeID)frame, slot, &si);
	
	  if (err == JVMTI_ERROR_NONE) {
		if (value != NULL) {
		  CVMID_icellAssignDirect(ee, &si->r, value);
		} else {
		  CVMID_icellSetNull(&si->r);
		}
	  }
	});
  return err;
}


static jvmtiError JNICALL
jvmti_SetLocalInt(jvmtiEnv* jvmtienv,
				  jthread thread,
				  jint depth,
				  jint slot,
				  jint value) {
  CVMJavaVal32* si;
  CVMExecEnv* ee;
  jvmtiError err;
  CVMFrame* frame;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }

  CVMD_gcUnsafeExec(ee, {
	  err = jvmtiGCUnsafeGetSlot((jframeID)frame, slot, &si);
	  if (err == JVMTI_ERROR_NONE) {
		si->i = (CVMJavaInt)value;
	  }
	});

  return err;
}


static jvmtiError JNICALL
jvmti_SetLocalLong(jvmtiEnv* jvmtienv,
				   jthread thread,
				   jint depth,
				   jint slot,
				   jlong value) {
  CVMJavaVal32* si;
  jvmtiError err;
  CVMExecEnv* ee;
  CVMFrame *frame;

  DEBUG_ENABLED();

  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }
  CVMD_gcUnsafeExec(ee, {
	  err = jvmtiGCUnsafeGetSlot((jframeID)frame, slot, &si);
	  if (err == JVMTI_ERROR_NONE) {
		CVMlong2Jvm(&si->raw, value);
	  }
	});

  return err;	 
}


static jvmtiError JNICALL
jvmti_SetLocalFloat(jvmtiEnv* jvmtienv,
					jthread thread,
					jint depth,
					jint slot,
					jfloat value) {
  CVMJavaVal32* si;
  jvmtiError err;
  CVMExecEnv* ee;
  CVMFrame *frame;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();

  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }
  CVMD_gcUnsafeExec(ee, {
	  err = jvmtiGCUnsafeGetSlot((jframeID)frame, slot, &si);
	  if (err == JVMTI_ERROR_NONE) {
		si->f = value;
	  }
	});
  return err;
}


static jvmtiError JNICALL
jvmti_SetLocalDouble(jvmtiEnv* jvmtienv,
					 jthread thread,
					 jint depth,
					 jint slot,
					 jdouble value) {
  CVMJavaVal32* si;
  jvmtiError err;
  CVMExecEnv* ee;
  CVMFrame *frame;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();

  ee = jthreadToExecEnv(CVMgetEE(), thread);
  err = get_frame(ee, depth, &frame);
  if (err != JVMTI_ERROR_NONE) {
	return err;
  }
  CVMD_gcUnsafeExec(ee, {
	  err = jvmtiGCUnsafeGetSlot((jframeID)frame, slot, &si);
	if (err == JVMTI_ERROR_NONE) {
	  CVMdouble2Jvm(&si->raw, value);
	}
	});
  return err;	 
}


/*
 * Breakpoint functions
 */ 

static jvmtiError JNICALL
jvmti_SetBreakpoint(jvmtiEnv* jvmtienv,
					jmethodID method,
					jlocation location) {
  CVMMethodBlock* mb = (CVMMethodBlock*) method;
  CVMUint8* code;
  CVMInt32 bciInt;
  CVMUint8* pc;
  jvmtiError err;
  jclass klass;
  CVMExecEnv* ee = CVMgetEE();
  CVMJavaLong sz;

  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(ee);

  if (CVMmbIs(mb, NATIVE)) {
	return JVMTI_ERROR_INVALID_METHODID;
  }

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

  DEBUGGER_LOCK(ee);
  if (CVMbagFind(CVMglobals.jvmtiStatics.breakpoints, pc) == NULL) {
	JNIEnv *env = CVMexecEnv2JniEnv(CVMgetEE());
	jobject classRef;
	klass = (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMmbClassBlock(mb)));
	classRef = (*env)->NewGlobalRef(env, klass);

	if (classRef == NULL) {
	  err = JVMTI_ERROR_OUT_OF_MEMORY;
	} else {
	  struct bkpt *bp = CVMbagAdd(CVMglobals.jvmtiStatics.breakpoints);
	  if (bp == NULL) {
		(*env)->DeleteGlobalRef(env, classRef);
		err = JVMTI_ERROR_OUT_OF_MEMORY;
	  } else {
		bp->pc = pc;
		bp->opcode = *pc;
		/* Keep a reference to the class to prevent gc */
		bp->classRef = classRef;
		*pc = opc_breakpoint;
		err = JVMTI_ERROR_NONE;
	  }
	}
  } else {
	err = JVMTI_ERROR_DUPLICATE;
  }
  DEBUGGER_UNLOCK(ee);	  

  return err;
}


static jvmtiError JNICALL
jvmti_ClearBreakpoint(jvmtiEnv* jvmtienv,
					  jmethodID method,
					  jlocation location) {
  struct bkpt *bp;
  CVMMethodBlock* mb = (CVMMethodBlock*) method;
  CVMUint8* code;
  CVMInt32 bciInt;
  CVMUint8* pc;
  jvmtiError err;
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env;	  
  CVMJavaLong sz;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(ee);

  if (CVMmbIs(mb, NATIVE)) {
	return JVMTI_ERROR_INVALID_METHODID;
  }

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

  env = CVMexecEnv2JniEnv(ee);

  sz = CVMint2Long(CVMmbCodeLength(mb));
  if (CVMlongLtz(location) || CVMlongGe(location,sz)) {
	return JVMTI_ERROR_INVALID_LOCATION;
  }

  DEBUGGER_LOCK(ee);
  bp = CVMbagFind(CVMglobals.jvmtiStatics.breakpoints, pc);
  if (bp == NULL) {
	err = JVMTI_ERROR_NOT_FOUND;
  } else {
	clear_bkpt(env, bp);
	CVMbagDelete(CVMglobals.jvmtiStatics.breakpoints, bp);
	err = JVMTI_ERROR_NONE;
  }
  DEBUGGER_UNLOCK(ee);

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

  DEBUG_ENABLED();
  THREAD_OK(ee);

  DEBUGGER_LOCK(ee);
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
  DEBUGGER_UNLOCK(ee);	  

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

  DEBUG_ENABLED();
  THREAD_OK(ee);

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
	err = JVMTI_ERROR_NONE;
  } else {
	err = JVMTI_ERROR_NOT_FOUND;
  }
  DEBUGGER_UNLOCK(ee);

  return err;
}

static jvmtiError JNICALL
jvmti_SetFieldAccessWatch(jvmtiEnv* jvmtienv,
						  jclass klass,
						  jfieldID field) {
  CHECK_JVMTI_ENV
  return setFieldWatch(klass, field, 
					   CVMglobals.jvmtiStatics.watchedFieldAccesses, 
					   &CVMglobals.jvmtiWatchingFieldAccess);
}


static jvmtiError JNICALL
jvmti_ClearFieldAccessWatch(jvmtiEnv* jvmtienv,
							jclass klass,
							jfieldID field) {
  CHECK_JVMTI_ENV
  return clearFieldWatch(klass, field, 
						 CVMglobals.jvmtiStatics.watchedFieldAccesses, 
						 &CVMglobals.jvmtiWatchingFieldAccess);
}


static jvmtiError JNICALL
jvmti_SetFieldModificationWatch(jvmtiEnv* jvmtienv,
								jclass klass,
								jfieldID field) {
  CHECK_JVMTI_ENV
  return setFieldWatch(klass, field, 
					   CVMglobals.jvmtiStatics.watchedFieldModifications, 
					   &CVMglobals.jvmtiWatchingFieldModification);
}


static jvmtiError JNICALL
jvmti_ClearFieldModificationWatch(jvmtiEnv* jvmtienv,
								  jclass klass,
								  jfieldID field) {
  CHECK_JVMTI_ENV
  return clearFieldWatch(klass, field, 
						 CVMglobals.jvmtiStatics.watchedFieldModifications, 
						 &CVMglobals.jvmtiWatchingFieldModification);
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
	if (data->err == JVMTI_ERROR_NONE) {
	  data->classes[data->i] = (*env)->NewLocalRef(env, CVMcbJavaInstance(cb));
	}
	data->i++;
  }
}

/* Return via "classes_ptr" all classes currently loaded in the VM
 * ("class_count_ptr" returns the number of such classes).
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetLoadedClasses(jvmtiEnv* jvmtienv,
					   jint* class_count_ptr,
					   jclass** classes_ptr) {
  LoadedClassesUserData *data;
  CVMExecEnv* ee = CVMgetEE();
  CVMJavaLong sz;
  jvmtiError rc;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(ee);
  NOT_NULL2(class_count_ptr, classes_ptr);

  data = malloc(sizeof(LoadedClassesUserData));
  data->numClasses = 0;
  data->i = 0;
  data->err = JVMTI_ERROR_NONE;
  /* Give it a safe value in case we have to abort */
  *class_count_ptr = 0;

  /* Seize classTableLock so nothing gets added while we're counting. */
  CVM_CLASSTABLE_LOCK(ee);

  CVMclassIterateAllClasses(ee, &LoadedClassesCountCallback, data);
  sz = CVMint2Long(data->numClasses * sizeof(jclass));
  rc = jvmti_Allocate(jvmtienv, sz, (unsigned char**)classes_ptr);
  if (rc == JVMTI_ERROR_NONE) {
	data->classes = *classes_ptr;
	CVMclassIterateAllClasses(ee, &LoadedClassesGetCallback, data);
	rc = data->err;
  }

  if (rc != JVMTI_ERROR_NONE) {
	*classes_ptr = NULL;
  } else {
	*class_count_ptr = data->numClasses;
  }

  CVM_CLASSTABLE_UNLOCK(ee);
  free(data);
  return rc;
}

/* Return via "classes_ptr" all classes loaded through the 
 * given initiating class loader.
 * ("classes_count_ptr" returns the number of such threads).
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetClassLoaderClasses(jvmtiEnv* jvmtienv,
							jobject initiating_loader,
							jint* class_count_ptr,
							jclass** classes_ptr) {
  jvmtiError rc = JVMTI_ERROR_NONE;
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env;	  
  CVMLoaderCacheIterator iter;
  int c, n;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(ee);
  NOT_NULL2(class_count_ptr, classes_ptr);

  env = CVMexecEnv2JniEnv(ee);

  CVM_LOADERCACHE_LOCK(ee);

  /* count the classes */
  n = 0;
  CVMloaderCacheIterate(ee, &iter);
  while (CVMloaderCacheIterateNext(ee, &iter)) {
	CVMObjectICell *loader = CVMloaderCacheIterateGetLoader(ee, &iter);
	CVMBool same;
	CVMID_icellSameObjectNullCheck(ee, initiating_loader, loader, same);
	if (same) {
	  ++n;
	}
  }

  rc = jvmti_Allocate(jvmtienv, CVMint2Long(n * sizeof(jclass)),
					  (unsigned char**)classes_ptr);
  *class_count_ptr = n;
  c = 0;
  if (rc == JVMTI_ERROR_NONE) {
	/* fill in the classes */
	CVMloaderCacheIterate(ee, &iter);
	while (CVMloaderCacheIterateNext(ee, &iter)) {
	  CVMObjectICell *loader = CVMloaderCacheIterateGetLoader(ee, &iter);
	  CVMBool same;
	  CVMID_icellSameObjectNullCheck(ee, initiating_loader, loader, same);
	  if (same) {
		CVMClassBlock *cb;
		cb = CVMloaderCacheIterateGetCB(ee, &iter);
		(*classes_ptr)[c] = (*env)->NewLocalRef(env, CVMcbJavaInstance(cb));
		++c;
	  }
	}
  }

  if (c < n) {
	jvmti_Deallocate(jvmtienv, (unsigned char *)*classes_ptr);
	rc = JVMTI_ERROR_OUT_OF_MEMORY;
  }
  CVM_LOADERCACHE_UNLOCK(ee);

  return rc;
}


static jvmtiError JNICALL
jvmti_GetClassSignature(jvmtiEnv* jvmtienv,
						jclass klass,
						char** signature_ptr,
						char** generic_ptr) {
  char *name;
  int len;
  CVMJavaLong longLen;
  CVMClassBlock* cb;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();

  cb = object2class(CVMgetEE(), klass);
  VALID_CLASS(cb);

  if (generic_ptr != NULL) {
	*generic_ptr = NULL;
  }
  if (!CVMcbIs(cb, PRIMITIVE)) {
	name = CVMtypeidClassNameToAllocatedCString(CVMcbClassName(cb));
	len = strlen(name);
	if (name[0] == '[') { /* arrays are already in signature form */
	  longLen = CVMint2Long(len+1);
	  ALLOC_WITH_CLEANUP_IF_FAILED(jvmtienv, longLen, signature_ptr, {
				free (name);
			});
		strcpy(*signature_ptr, name);
	} else {
	  char *sig;
	  longLen = CVMint2Long(len+3);
	  ALLOC_WITH_CLEANUP_IF_FAILED(jvmtienv, longLen, signature_ptr, {
		  free (name);
		});
	  sig = *signature_ptr;
	  sig[0] = 'L';
	  strcpy(sig+1, name);
	  sig[len+1] = ';';
	  sig[len+2] = 0;
	}		 
	free(name);
  } else {
	char *sig;
	longLen = CVMint2Long(2);
	ALLOC(jvmtienv, longLen, signature_ptr);
	sig = *signature_ptr;
	sig[0] = CVMbasicTypeSignatures[CVMcbBasicTypeCode(cb)];
	sig[1] = 0;
  }
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetClassStatus(jvmtiEnv* jvmtienv,
					 jclass klass,
					 jint* status_ptr) {
  jint state = 0;
  CVMClassBlock* cb;
  CVMExecEnv *ee = CVMgetEE();
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(status_ptr);

  cb = object2class(ee, klass);
  VALID_CLASS(cb);

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

  *status_ptr = state;
  return JVMTI_ERROR_NONE;
}

/* Return via "source_name_Ptr" the class's source path (UTF8).
 * Errors: JVMTI_ERROR_OUT_OF_MEMORY, JVMTI_ERROR_ABSENT_INFORMATION
 */

static jvmtiError JNICALL
jvmti_GetSourceFileName(jvmtiEnv* jvmtienv,
						jclass klass,
						char** source_name_ptr) {
  char *srcName;
  CVMClassBlock* cb;
  CVMJavaLong sz;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();

  cb = object2class(CVMgetEE(), klass);
  VALID_CLASS(cb);
  srcName = CVMcbSourceFileName(cb);
  if (srcName == NULL) {
	return JVMTI_ERROR_ABSENT_INFORMATION;
  }
  sz = CVMint2Long(strlen(srcName)+1);
  ALLOC(jvmtienv, sz, source_name_ptr);
  strcpy(*source_name_ptr, srcName);
  return JVMTI_ERROR_NONE;
}

/* Return via "modifiers_Ptr" the modifiers of this class.
 * See JVM Spec "access_flags".
 * Errors: JVMTI_ERROR_CLASS_NO_PREPARED, JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetClassModifiers(jvmtiEnv* jvmtienv,
						jclass klass,
						jint* modifiers_ptr) {
  CVMExecEnv *ee = CVMgetEE();
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(modifiers_ptr);
  THREAD_OK(ee);

  *modifiers_ptr = JVM_GetClassModifiers(CVMexecEnv2JniEnv(ee), klass);
  return JVMTI_ERROR_NONE;
}

/* Return via "methods_ptr" a list of the class's methods 
 * Inherited methods are not included.
 * Errors: JVMTI_ERROR_CLASS_NOT_PREPARED, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetClassMethods(jvmtiEnv* jvmtienv,
					  jclass klass,
					  jint* method_count_ptr,
					  jmethodID** methods_ptr) {
  jint methods_count;
  jmethodID *mids;
  int i;
  jint state;
  CVMClassBlock* cb; 
  CVMJavaLong sz;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL2(method_count_ptr, methods_ptr);

  cb = object2class(CVMgetEE(), klass);
  VALID_CLASS(cb);

  jvmti_GetClassStatus(jvmtienv, klass, &state);
  if (!(state & JVMTI_CLASS_STATUS_PREPARED)) {
	return JVMTI_ERROR_CLASS_NOT_PREPARED;
  }

  /* %comment: k015 */
  methods_count = CVMcbMethodCount(cb);
  *method_count_ptr = methods_count;
  sz = CVMint2Long(methods_count*sizeof(jmethodID));
  ALLOC(jvmtienv, sz, methods_ptr);
  mids = *methods_ptr;
  for (i = 0; i < methods_count; ++i) {
	mids[i] = CVMcbMethodSlot(cb, i);
  }
  return JVMTI_ERROR_NONE;
}

/* Return via "fields_ptr" a list of the class's fields ("field_count_ptr" 
 * returns the number of fields).
 * Inherited fields are not included.
 * Errors: JVMTI_ERROR_CLASS_NOT_PREPARED, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetClassFields(jvmtiEnv* jvmtienv,
					 jclass klass,
					 jint* field_count_ptr,
					 jfieldID** fields_ptr) {
  jint fields_count;
  jfieldID *fids;
  int i;
  jint state;
  CVMClassBlock* cb;
  CVMJavaLong sz;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL2(field_count_ptr, fields_ptr);

  cb = object2class(CVMgetEE(), klass);
  VALID_CLASS(cb);
	
  jvmti_GetClassStatus(jvmtienv, klass, &state);
  if (!(state & JVMTI_CLASS_STATUS_PREPARED)) {
	return JVMTI_ERROR_CLASS_NOT_PREPARED;
  }

  fields_count = CVMcbFieldCount(cb);
  *field_count_ptr = fields_count;
  sz = CVMint2Long(fields_count*sizeof(jfieldID));
  ALLOC(jvmtienv, sz, fields_ptr);
  fids = *fields_ptr;
  for (i = 0; i < fields_count; ++i) {
	fids[i] = CVMcbFieldSlot(cb, i);
  }
  return JVMTI_ERROR_NONE;
}

/* Return via "interfaces_ptr" a list of the interfaces this class
 * declares ("interface_count_ptr" returns the number of such interfaces).  
 * Errors: JVMTI_ERROR_CLASS_NOT_PREPARED, JVMTI_ERROR_NULL_POINTER,
 * JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetImplementedInterfaces(jvmtiEnv* jvmtienv,
							   jclass klass,
							   jint* interface_count_ptr,
							   jclass** interfaces_ptr) {
  jint interfaces_count;
  int i;
  /* %comment: k016 */
  jclass *imps;
  jint state;
  CVMClassBlock* cb;   
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env;	  
  CVMJavaLong sz;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL2(interface_count_ptr, interfaces_ptr);
  THREAD_OK(ee);

  env = CVMexecEnv2JniEnv(ee);

  cb = object2class(ee, klass);
  VALID_CLASS(cb);

  jvmti_GetClassStatus(jvmtienv, klass, &state);
  if (!(state & JVMTI_CLASS_STATUS_PREPARED)) {
	return JVMTI_ERROR_CLASS_NOT_PREPARED;
  }

  interfaces_count = CVMcbImplementsCount(cb);
  *interface_count_ptr = interfaces_count;
  sz = CVMint2Long(interfaces_count * sizeof(jclass));
  ALLOC(jvmtienv, sz, interfaces_ptr);
  imps = *interfaces_ptr;
  for (i = 0; i < interfaces_count; ++i) {
	imps[i] =
	  (*env)->NewLocalRef(env,
						  CVMcbJavaInstance(CVMcbInterfacecb(cb, i)));
	if (imps[i] == NULL) {
	  free(imps);
	  *interfaces_ptr = 0;
	  return JVMTI_ERROR_OUT_OF_MEMORY;
	}
  }
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetClassVersionNumbers(jvmtiEnv* env,
							 jclass klass,
							 jint* minor_version_ptr,
							 jint* major_version_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetConstantPool(jvmtiEnv* env,
					  jclass klass,
					  jint* constant_pool_count_ptr,
					  jint* constant_pool_byte_count_ptr,
					  unsigned char** constant_pool_bytes_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}

/* Return via "is_interface_ptr" a boolean indicating whether this "klass"
 * is an interface.
 * Errors: JVMTI_ERROR_CLASS_NOT_PREPARED, JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_IsInterface(jvmtiEnv* jvmtienv,
				  jclass klass,
				  jboolean* is_interface_ptr) {
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env = CVMexecEnv2JniEnv(ee);
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(ee);

  NOT_NULL(is_interface_ptr);
  *is_interface_ptr = JVM_IsInterface(env, klass);
  return JVMTI_ERROR_NONE;
}

/* Return via "is_array_class_ptr" a boolean indicating whether this "klass"
 * is an array class.
 * Errors: 
 */

static jvmtiError JNICALL
jvmti_IsArrayClass(jvmtiEnv* jvmtienv,
				   jclass klass,
				   jboolean* is_array_class_ptr) {
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(ee);

  env = CVMexecEnv2JniEnv(ee);

  NOT_NULL(is_array_class_ptr);
  *is_array_class_ptr = JVM_IsArrayClass(env, klass);
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_IsModifiableClass(jvmtiEnv* env,
						jclass klass,
						jboolean* is_modifiable_class_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}

/* Return via "classloader_ptr" the class loader that created this class
 * or interface, null if this class was not created by a class loader.
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetClassLoader(jvmtiEnv* jvmtienv,
					 jclass klass,
					 jobject* classloader_ptr) {
  jobject loader;
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env;	  
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(classloader_ptr);
  THREAD_OK(ee);

  env = CVMexecEnv2JniEnv(ee);

  loader = JVM_GetClassLoader(env, klass);
  *classloader_ptr = (jobject)((*env)->NewLocalRef(env, loader));
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetSourceDebugExtension(jvmtiEnv* env,
							  jclass klass,
							  char** source_debug_extension_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_RetransformClasses(jvmtiEnv* env,
						 jint class_count,
						 const jclass* classes) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_RedefineClasses(jvmtiEnv* env,
					  jint class_count,
					  const jvmtiClassDefinition* class_definitions) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Object functions
 */ 

static jvmtiError JNICALL
jvmti_GetObjectSize(jvmtiEnv* env,
					jobject object,
					jlong* size_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}

/* Return via "hash_code_ptr" a hash code that can be used in maintaining
 * hash table of object references. This function guarantees the same 
 * hash code value for a particular object throughout its life
 * Errors:
 */

static jvmtiError JNICALL
jvmti_GetObjectHashCode(jvmtiEnv* jvmtienv,
						jobject object,
						jint* hash_code_ptr) {
  CHECK_JVMTI_ENV
  DEBUG_ENABLED();
  VALID_OBJECT(object);
  NOT_NULL(hash_code_ptr);
  *hash_code_ptr = JVM_IHashCode(CVMexecEnv2JniEnv(CVMgetEE()), object);
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetObjectMonitorUsage(jvmtiEnv* env,
							jobject object,
							jvmtiMonitorUsage* info_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Field functions
 */ 

/* Return via "name_ptr" the field's name and via "signature_ptr" the
 * field's signature (UTF8).
 * Errors: JVMTI_ERROR_INVALID_FIELDID, 
 * JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetFieldName(jvmtiEnv* jvmtienv,
				   jclass klass,
				   jfieldID field,
				   char** name_ptr,
				   char** signature_ptr,
				   char** generic_ptr) {
  char *name;
  char *sig;
  CVMFieldBlock* fb = (CVMFieldBlock*) field;
  CVMJavaLong sz;
  CHECK_JVMTI_ENV
	
  DEBUG_ENABLED();
  NOT_NULL2(name_ptr, signature_ptr);
  if (fb == NULL) {
	return JVMTI_ERROR_INVALID_FIELDID;
  }

  if (generic_ptr != NULL) {
	*generic_ptr = NULL;
  }
  name = CVMtypeidFieldNameToAllocatedCString(CVMfbNameAndTypeID(fb));
  sz = CVMint2Long(strlen(name)+1);
  ALLOC(jvmtienv, sz, name_ptr);
  strcpy(*name_ptr, name);
  free(name);

  sig = CVMtypeidFieldTypeToAllocatedCString(CVMfbNameAndTypeID(fb));
  sz = CVMint2Long(strlen(sig)+1);
  ALLOC(jvmtienv, sz, signature_ptr);
  strcpy(*signature_ptr, sig);
  free(sig);

  return JVMTI_ERROR_NONE;
}

/* Return via "declaring_class_ptr" the class in which this field is
 * defined.
 * Errors: JVMTI_ERROR_INVALID_FIELDID, JVMTI_ERROR_INVALID_CLASS,
 * JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetFieldDeclaringClass(jvmtiEnv* jvmtienv,
							 jclass klass,
							 jfieldID field,
							 jclass* declaring_class_ptr) {
  CVMFieldBlock* fb = (CVMFieldBlock*) field;
  jobject dklass;
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  if (klass == NULL || CVMID_icellIsNull(klass)) {
	return JVMTI_ERROR_INVALID_CLASS;
  }
  NULL_CHECK(field, JVMTI_ERROR_INVALID_FIELDID);
  NULL_CHECK(declaring_class_ptr, JVMTI_ERROR_NULL_POINTER);
  THREAD_OK(ee);

  env = CVMexecEnv2JniEnv(ee);

  dklass = (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMfbClassBlock(fb)));
  *declaring_class_ptr = (jobject)((*env)->NewLocalRef(env, dklass));
  return JVMTI_ERROR_NONE;
}

/* Return via "modifiers_ptr" the modifiers of this field.
 * See JVM Spec "access_flags".
 * Errors:  JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetFieldModifiers(jvmtiEnv* jvmtienv,
						jclass klass,
						jfieldID field,
						jint* modifiers_ptr) {
  CVMFieldBlock* fb = (CVMFieldBlock*) field;
  CHECK_JVMTI_ENV
	
  DEBUG_ENABLED();
  NOT_NULL(modifiers_ptr);
  /* %comment: k017 */
  *modifiers_ptr = CVMfbAccessFlags(fb);
  return JVMTI_ERROR_NONE;
}

/* Return via "is_synthetic_ptr" a boolean indicating whether the field
 * is synthetic.
 * Errors: JVMTI_ERROR_MUST_POSSESS_CAPABILITY
 */

static jvmtiError JNICALL
jvmti_IsFieldSynthetic(jvmtiEnv* env,
					   jclass klass,
					   jfieldID field,
					   jboolean* is_synthetic_ptr) {
  return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
}


/*
 * Method functions
 */ 

/* Return via "name_ptr" the method's name and via "signature_ptr" the
 * method's signature (UTF8).
 * Errors: JVMTI_ERROR_INVALID_METHODID
 * JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_GetMethodName(jvmtiEnv* jvmtienv,
					jmethodID method,
					char** name_ptr,
					char** signature_ptr,
					char** generic_ptr) {
  char *name;
  char *sig;
  CVMMethodBlock* mb = (CVMMethodBlock*) method;
  CVMJavaLong sz;
  CHECK_JVMTI_ENV
	
  DEBUG_ENABLED();
  NOT_NULL2(name_ptr, signature_ptr);
  if (mb == NULL) {
	return JVMTI_ERROR_INVALID_METHODID;
  }

  name = CVMtypeidMethodNameToAllocatedCString(CVMmbNameAndTypeID(mb));
  sz = CVMint2Long(strlen(name)+1);
  ALLOC(jvmtienv, sz, name_ptr);
  strcpy(*name_ptr, name);
  free(name);

  sig = CVMtypeidMethodTypeToAllocatedCString(CVMmbNameAndTypeID(mb));
  sz = CVMint2Long(strlen(sig)+1);
  ALLOC(jvmtienv, sz, signature_ptr);
  strcpy(*signature_ptr, sig);
  free(sig);

  return JVMTI_ERROR_NONE;
}

/* Return via "declaring_class_ptr" the class in which this method is
 * defined.
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetMethodDeclaringClass(jvmtiEnv* jvmtienv,
							  jmethodID method,
							  jclass* declaring_class_ptr) {
  CVMMethodBlock* mb = (CVMMethodBlock*) method;
  jobject dklass;
  CVMExecEnv* ee = CVMgetEE();
  JNIEnv *env;	  
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(declaring_class_ptr);
  THREAD_OK(ee);

  env = CVMexecEnv2JniEnv(ee);

  dklass = (*env)->NewLocalRef(env, CVMcbJavaInstance(CVMmbClassBlock(mb)));
  *declaring_class_ptr = dklass;
  return JVMTI_ERROR_NONE;
}

/* Return via "modifiers_ptr" the modifiers of this method.
 * See JVM Spec "access_flags".
 * Errors:  JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetMethodModifiers(jvmtiEnv* jvmtienv,
						 jmethodID method,
						 jint* modifiers_ptr) {
  CVMMethodBlock* mb = (CVMMethodBlock*) method;
  jint modifiers;
  CHECK_JVMTI_ENV
	
  DEBUG_ENABLED();
  NOT_NULL(modifiers_ptr);

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

  *modifiers_ptr = modifiers;
  return JVMTI_ERROR_NONE;
}

/* Return via "max_ptr" the number of local variable slots used by 
 * this method. Note two-word locals use two slots.
 * Errors: JVMTI_ERROR_INVALID_METHODID, JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetMaxLocals(jvmtiEnv* jvmtienv,
				   jmethodID method,
				   jint* max_ptr) {
  CVMMethodBlock* mb = (CVMMethodBlock*) method;
  CHECK_JVMTI_ENV
	
  DEBUG_ENABLED();
  NOT_NULL(max_ptr);

  if (!CVMmbIsJava(mb)) {
	return JVMTI_ERROR_INVALID_METHODID;
  }
  
  /* The jmd for <clinit> gets freed after it executes. Can't take
	 any chances. */
  if (CVMmbJmd(mb) == NULL) {
	return JVMTI_ERROR_INVALID_METHODID;
  }

  *max_ptr = CVMmbMaxLocals(mb);
  return JVMTI_ERROR_NONE;
}

/* Return via "size_ptr" the number of local variable slots used by 
 * arguments. Note two-word arguments use two slots.
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_GetArgumentsSize(jvmtiEnv* jvmtienv,
					   jmethodID method,
					   jint* size_ptr) {
  CVMMethodBlock* mb = (CVMMethodBlock*) method;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(size_ptr);

  *size_ptr = CVMmbArgsSize(mb);
  return JVMTI_ERROR_NONE;
}

/* Return via "table_ptr" the line number table ("entry_count_ptr" 
 * returns the number of entries in the table).
 * Errors: JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY,
 * JVMTI_ERROR_ABSENT_INFORMATION
 */

static jvmtiError JNICALL
jvmti_GetLineNumberTable(jvmtiEnv* jvmtienv,
						 jmethodID method,
						 jint* entry_count_ptr,
						 jvmtiLineNumberEntry** table_ptr) {
  int i;
  CVMMethodBlock* mb = (CVMMethodBlock*) method;
  CVMLineNumberEntry* vmtbl;
  CVMUint16 length;
  jvmtiLineNumberEntry *tbl;
  CVMJavaLong sz;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL2(entry_count_ptr, table_ptr);

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
	ALLOC(jvmtienv, sz, table_ptr);
	*entry_count_ptr = (jint)1;
	tbl = *table_ptr;
	tbl[0].start_location = CVMint2Long(0);
	tbl[0].line_number = 0;
	return JVMTI_ERROR_NONE;
  }

  vmtbl = CVMmbLineNumberTable(mb);
  length = CVMmbLineNumberTableLength(mb);

  if (vmtbl == NULL) {
	return JVMTI_ERROR_ABSENT_INFORMATION;
  }
  sz = CVMint2Long(length * sizeof(jvmtiLineNumberEntry));
  ALLOC(jvmtienv, sz, table_ptr);
  *entry_count_ptr = (jint)length;
  tbl = *table_ptr;
  for (i = 0; i < length; ++i) {
	tbl[i].start_location = CVMint2Long(vmtbl[i].startpc);
	tbl[i].line_number = (jint)(vmtbl[i].lineNumber);
  }
  return JVMTI_ERROR_NONE;
}

/* Return via "start_location_ptr" the first location in the method.
 * Return via "end_location_ptr" the last location in the method.
 * If conventional byte code index locations are being used then
 * this returns zero and the number of byte codes minus one
 * (respectively).
 * If location information is not available return negative one through the pointers.
 * Errors:  JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_ABSENT_INFORMATION
 *
 */

static jvmtiError JNICALL
jvmti_GetMethodLocation(jvmtiEnv* jvmtienv,
						jmethodID method,
						jlocation* start_location_ptr,
						jlocation* end_location_ptr) {
  CVMMethodBlock* mb = (CVMMethodBlock*) method;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL2(start_location_ptr, end_location_ptr);

  if (!CVMmbIsJava(mb)) {
	*start_location_ptr = CVMint2Long(-1);
	*end_location_ptr = CVMint2Long(-1);
	/*	return JVMTI_ERROR_ABSENT_INFORMATION; */
	return JVMTI_ERROR_NONE;
  }

  /* Because we free the Java method descriptor for <clinit> after
	 it's run, it's possible for us to get a frame pop event on a
	 method which has a NULL jmd. If that happens, we'll do the same
	 thing as above. */

  if (CVMmbJmd(mb) == NULL) {
	/* %coment: k020 */
	*start_location_ptr = CVMlongConstZero();
	*end_location_ptr = CVMlongConstZero();
	/*	return JVMTI_ERROR_ABSENT_INFORMATION; */
	return JVMTI_ERROR_NONE;
  }

  *start_location_ptr = CVMlongConstZero();
  *end_location_ptr = CVMint2Long(CVMmbCodeLength(mb) - 1);
  return JVMTI_ERROR_NONE;
}

/* Return via "table_ptr" the local variable table ("entry_count_ptr" 
 * returns the number of entries in the table).
 * Errors:  JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY,
 * JVMTI_ERROR_ABSENT_INFORMATION
 */

static jvmtiError JNICALL
jvmti_GetLocalVariableTable(jvmtiEnv* jvmtienv,
							jmethodID method,
							jint* entry_count_ptr,
							jvmtiLocalVariableEntry** table_ptr) {
  int i;
  CVMMethodBlock* mb = (CVMMethodBlock*) method;
  CVMLocalVariableEntry* vmtbl;
  CVMUint16 length;
  CVMClassBlock* cb = CVMmbClassBlock(mb);
  CVMConstantPool* constantpool;
  jvmtiLocalVariableEntry *tbl;
  CVMJavaLong sz;
  CHECK_JVMTI_ENV
	
  DEBUG_ENABLED();
  NOT_NULL2(entry_count_ptr, table_ptr);

  if (!CVMmbIsJava(mb)) {
	return JVMTI_ERROR_ABSENT_INFORMATION;
  }

  /* The jmd for <clinit> gets freed after it executes. Can't take
	 any chances. */
  if (CVMmbJmd(mb) == NULL) {
	return JVMTI_ERROR_ABSENT_INFORMATION;
  }

  vmtbl = CVMmbLocalVariableTable(mb);
  length = CVMmbLocalVariableTableLength(mb);
  constantpool = CVMcbConstantPool(cb);

  if (vmtbl == NULL) {
	return JVMTI_ERROR_ABSENT_INFORMATION;
  }
  sz = CVMint2Long(length * sizeof(jvmtiLocalVariableEntry));
  ALLOC(jvmtienv, sz, table_ptr);
  *entry_count_ptr = (jint)length;
  tbl = *table_ptr;
  for (i = 0; i < length; ++i) {
	char* nameAndTypeTmp;
	char* buf;
	tbl[i].generic_signature = NULL;
	tbl[i].start_location = CVMint2Long(vmtbl[i].startpc);
	tbl[i].length = (jint)(vmtbl[i].length);

	nameAndTypeTmp =
	  CVMtypeidFieldNameToAllocatedCString(CVMtypeidCreateTypeIDFromParts(vmtbl[i].nameID, vmtbl[i].typeID));
	sz = CVMint2Long(strlen(nameAndTypeTmp)+1);
	ALLOC(jvmtienv, sz, &buf);
	strcpy(buf, nameAndTypeTmp);
	free(nameAndTypeTmp);
	tbl[i].name = buf;

	nameAndTypeTmp =
	  CVMtypeidFieldTypeToAllocatedCString(CVMtypeidCreateTypeIDFromParts(vmtbl[i].nameID, vmtbl[i].typeID));
	sz = CVMint2Long(strlen(nameAndTypeTmp)+1);
	ALLOC(jvmtienv, sz, &buf);
	strcpy(buf, nameAndTypeTmp);
	free(nameAndTypeTmp);
	tbl[i].signature = buf;

	tbl[i].slot = (jint)(vmtbl[i].index);
  }
  return JVMTI_ERROR_NONE;
}

/* NOTE: (k) This is not implemented in CVM */

static jvmtiError JNICALL
jvmti_GetBytecodes(jvmtiEnv* env,
				   jmethodID method,
				   jint* bytecode_count_ptr,
				   unsigned char** bytecodes_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


/* Return via "is_native_ptr" a boolean indicating whether the method
 * is native.
 * Errors:  JVMTI_ERROR_NULL_POINTER
 */
static jvmtiError JNICALL
jvmti_IsMethodNative(jvmtiEnv* jvmtienv,
					 jmethodID method,
					 jboolean* is_native_ptr) {
  CVMMethodBlock* mb = (CVMMethodBlock*) method;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  NOT_NULL(is_native_ptr);
  *is_native_ptr = (CVMmbIs(mb, NATIVE) != 0);
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_IsMethodSynthetic(jvmtiEnv* env,
						jmethodID method,
						jboolean* is_synthetic_ptr) {
  return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
}


static jvmtiError JNICALL
jvmti_IsMethodObsolete(jvmtiEnv* env,
					   jmethodID method,
					   jboolean* is_obsolete_ptr) {
  *is_obsolete_ptr = CVM_FALSE;
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_SetNativeMethodPrefix(jvmtiEnv* env,
							const char* prefix) {

  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_SetNativeMethodPrefixes(jvmtiEnv* env,
							  jint prefix_count,
							  char** prefixes) {

  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Raw Monitor functions
 */ 

/* Return via "monitor_ptr" a newly created debugger monitor that can be 
 * used by debugger threads to coordinate. The semantics of the
 * monitor are the same as those described for Java language 
 * monitors in the Java Language Specification.
 * Errors: JVMTI_ERROR_NULL_POINTER, JVMTI_ERROR_OUT_OF_MEMORY
 */

static jvmtiError JNICALL
jvmti_CreateRawMonitor(jvmtiEnv* jvmtienv,
					   const char* name,
					   jrawMonitorID* monitor_ptr) {
  CHECK_JVMTI_ENV
  DEBUG_ENABLED();
  ASSERT_NOT_NULL2_ELSE_EXIT_WITH_ERROR(name, monitor_ptr, JVMTI_ERROR_NULL_POINTER);

  *monitor_ptr = (jrawMonitorID) CVMnamedSysMonitorInit(name);
  ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(*monitor_ptr, JVMTI_ERROR_OUT_OF_MEMORY);
  return JVMTI_ERROR_NONE;
}

/* Destroy a debugger monitor created with jvmti_CreateRawMonitor.
 * Errors: JVMTI_ERROR_NULL_POINTER
 */

static jvmtiError JNICALL
jvmti_DestroyRawMonitor(jvmtiEnv* jvmtienv,
						jrawMonitorID monitor) {
  CHECK_JVMTI_ENV
  DEBUG_ENABLED();
  ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_NULL_POINTER);

  CVMnamedSysMonitorDestroy((CVMNamedSysMonitor *) monitor);
  return JVMTI_ERROR_NONE;
}

/* Gain exclusive ownership of a debugger monitor.
 * Errors: JVMTI_ERROR_INVALID_MONITOR
 */

static jvmtiError JNICALL
jvmti_RawMonitorEnter(jvmtiEnv* jvmtienv,
					  jrawMonitorID monitor) {

  CVMExecEnv *current_ee = CVMgetEE();
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(current_ee);
  ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_INVALID_MONITOR);

  CVMnamedSysMonitorEnter((CVMNamedSysMonitor *) monitor, current_ee);
  return JVMTI_ERROR_NONE;
}

/* Release exclusive ownership of a debugger monitor.
 * Errors: JVMTI_ERROR_INVALID_MONITOR
 */

static jvmtiError JNICALL
jvmti_RawMonitorExit(jvmtiEnv* jvmtienv,
					 jrawMonitorID monitor) {

  CVMExecEnv *current_ee = CVMgetEE();
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(current_ee);
  ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_INVALID_MONITOR);

  CVMnamedSysMonitorExit((CVMNamedSysMonitor *) monitor, current_ee);
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
  CVMExecEnv *current_ee = CVMgetEE();
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(current_ee);
  ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_INVALID_MONITOR);

  /* %comment l001 */
  if (CVMlongLt(millis, CVMlongConstZero()))  /* if (millis < 0) */
	millis = CVMlongConstZero();			/*	   millis = 0; */

  error = CVMnamedSysMonitorWait((CVMNamedSysMonitor *) monitor, current_ee, millis);
  switch (error) {
  case CVM_WAIT_OK:			  result = JVMTI_ERROR_NONE; break;
  case CVM_WAIT_INTERRUPTED:  result = JVMTI_ERROR_INTERRUPT; break;
  case CVM_WAIT_NOT_OWNER:	  result = JVMTI_ERROR_NOT_MONITOR_OWNER; break;
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

  CVMExecEnv *current_ee = CVMgetEE();
  CVMBool successful;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(current_ee);
  ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_INVALID_MONITOR);

  successful = CVMnamedSysMonitorNotify((CVMNamedSysMonitor*) monitor, current_ee);
  return successful ? JVMTI_ERROR_NONE : JVMTI_ERROR_NOT_MONITOR_OWNER;
}

/* Notify all threads waiting on the debugger monitor. The calling 
 * thread must own the monitor.
 * Errors: JVMTI_ERROR_INVALID_MONITOR, JVMTI_ERROR_NOT_MONITOR_OWNER
 */

static jvmtiError JNICALL
jvmti_RawMonitorNotifyAll(jvmtiEnv* jvmtienv,
						  jrawMonitorID monitor) {

  CVMExecEnv *current_ee = CVMgetEE();
  CVMBool successful;
  CHECK_JVMTI_ENV

  DEBUG_ENABLED();
  THREAD_OK(current_ee);
  ASSERT_NOT_NULL_ELSE_EXIT_WITH_ERROR(monitor, JVMTI_ERROR_INVALID_MONITOR);

  successful = CVMnamedSysMonitorNotifyAll((CVMNamedSysMonitor*) monitor, current_ee);
  return successful ? JVMTI_ERROR_NONE : JVMTI_ERROR_NOT_MONITOR_OWNER;
}


/*
 * JNI Function Interception functions
 */ 

static jvmtiError JNICALL
jvmti_SetJNIFunctionTable(jvmtiEnv* jvmtienv,
						  const jniNativeInterface* function_table) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetJNIFunctionTable(jvmtiEnv* jvmtienv,
						  jniNativeInterface** function_table) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Event Management functions
 * 
 * First pass - don't port the whole complicated thread x env matrix
 * capability
 */
/* Check Event Capabilities */
CVMBool has_event_capability(jvmtiEvent event_type,
							 jvmtiCapabilities* capabilities_ptr) {
  switch (event_type) {
	case JVMTI_EVENT_SINGLE_STEP:
	  return capabilities_ptr->can_generate_single_step_events != 0;
	case JVMTI_EVENT_BREAKPOINT:
	  return capabilities_ptr->can_generate_breakpoint_events != 0;
	case JVMTI_EVENT_FIELD_ACCESS:
	  return capabilities_ptr->can_generate_field_access_events != 0;
	case JVMTI_EVENT_FIELD_MODIFICATION:
	  return capabilities_ptr->can_generate_field_modification_events != 0;
	case JVMTI_EVENT_FRAME_POP:
	  return capabilities_ptr->can_generate_frame_pop_events != 0;
	case JVMTI_EVENT_METHOD_ENTRY:
	  return capabilities_ptr->can_generate_method_entry_events != 0;
	case JVMTI_EVENT_METHOD_EXIT:
	  return capabilities_ptr->can_generate_method_exit_events != 0;
	case JVMTI_EVENT_NATIVE_METHOD_BIND:
	  return capabilities_ptr->can_generate_native_method_bind_events != 0;
	case JVMTI_EVENT_EXCEPTION:
	  return capabilities_ptr->can_generate_exception_events != 0;
	case JVMTI_EVENT_EXCEPTION_CATCH:
	  return capabilities_ptr->can_generate_exception_events != 0;
	case JVMTI_EVENT_COMPILED_METHOD_LOAD:
	  return capabilities_ptr->can_generate_compiled_method_load_events != 0;
	case JVMTI_EVENT_COMPILED_METHOD_UNLOAD:
	  return capabilities_ptr->can_generate_compiled_method_load_events != 0;
	case JVMTI_EVENT_MONITOR_CONTENDED_ENTER:
	  return capabilities_ptr->can_generate_monitor_events != 0;
	case JVMTI_EVENT_MONITOR_CONTENDED_ENTERED:
	  return capabilities_ptr->can_generate_monitor_events != 0;
	case JVMTI_EVENT_MONITOR_WAIT:
	  return capabilities_ptr->can_generate_monitor_events != 0;
	case JVMTI_EVENT_MONITOR_WAITED:
	  return capabilities_ptr->can_generate_monitor_events != 0;
	case JVMTI_EVENT_VM_OBJECT_ALLOC:
	  return capabilities_ptr->can_generate_vm_object_alloc_events != 0;
	case JVMTI_EVENT_OBJECT_FREE:
	  return capabilities_ptr->can_generate_object_free_events != 0;
	case JVMTI_EVENT_GARBAGE_COLLECTION_START:
	  return capabilities_ptr->can_generate_garbage_collection_events != 0;
	case JVMTI_EVENT_GARBAGE_COLLECTION_FINISH:
	  return capabilities_ptr->can_generate_garbage_collection_events != 0;

  default:
	return JNI_TRUE;
  }
  /* if it does not have a capability it is required */
  return JNI_TRUE;
}

static CVMBool is_valid_event_type(jvmtiEvent event_type) {
  return ((int)event_type >= TOTAL_MIN_EVENT_TYPE_VAL) &&
	((int)event_type <= TOTAL_MAX_EVENT_TYPE_VAL);
}

jlong bit_for(jvmtiEvent event_type) {
  CVMassert(is_valid_event_type(event_type));
  return ((jlong)1) << (event_type - TOTAL_MIN_EVENT_TYPE_VAL);	 
}

CVMBool has_callback(JvmtiEnv* jvmti_env, jvmtiEvent event_type) {
	CVMassert(event_type >= JVMTI_MIN_EVENT_TYPE_VAL && 
	   event_type <= JVMTI_MAX_EVENT_TYPE_VAL);
	return ((void**)&jvmti_env->_event_callbacks)[event_type-JVMTI_MIN_EVENT_TYPE_VAL] != NULL;
  }

static jvmtiError JNICALL
jvmti_SetEventCallbacks(jvmtiEnv* jvmtienv,
						const jvmtiEventCallbacks* callbacks,
						jint size_of_callbacks) {
  size_t byte_cnt = sizeof(jvmtiEventCallbacks);
  jlong enabled_bits = 0;
  int ei;
  JvmtiEnv* jvmti_env =
	(JvmtiEnv*)((int)jvmtienv - CVMoffsetof(JvmtiEnv, _jvmti_external));
  if (!jvmti_env->_is_valid) {
	return JVMTI_ERROR_INVALID_ENVIRONMENT;
  }
  DEBUG_ENABLED();
  THREAD_OK(CVMgetEE());

  /* clear in either case to be sure we got any gap between sizes */
  memset(&jvmti_env->_event_callbacks, 0, byte_cnt);
  if (callbacks != NULL) {
	if (size_of_callbacks < (jint)byte_cnt) {
	  byte_cnt = size_of_callbacks;
	}
	memcpy(&jvmti_env->_event_callbacks, callbacks, byte_cnt);
  }
  for (ei = JVMTI_MIN_EVENT_TYPE_VAL;
	   ei <= JVMTI_MAX_EVENT_TYPE_VAL; ++ei) {
	jvmtiEvent evt_t = (jvmtiEvent)ei;
	if (has_callback(jvmti_env, evt_t)) {
	  enabled_bits |= bit_for(evt_t);
	}
  }
  jvmti_env->_env_event_enable._event_callback_enabled._enabled_bits = enabled_bits;
  /* Need to set global event flag which is 'or' of user and callback events */
  jvmti_env->_env_event_enable._event_enabled._enabled_bits =
	jvmti_env->_env_event_enable._event_user_enabled._enabled_bits |
	jvmti_env->_env_event_enable._event_callback_enabled._enabled_bits;
  if (jvmti_env->_env_event_enable._event_enabled._enabled_bits &
	  (bit_for(JVMTI_EVENT_THREAD_START) | bit_for(JVMTI_EVENT_THREAD_END))) {
	jvmti_env->_thread_events_enabled = CVM_TRUE;
  }
  return JVMTI_ERROR_NONE;
}

CVMBool
is_global_event(jvmtiEvent event_type) {
  jlong bit_for;
  CVMassert(is_valid_event_type(event_type));
  bit_for = ((jlong)1) << (event_type - TOTAL_MIN_EVENT_TYPE_VAL); 
  return((bit_for & GLOBAL_EVENT_BITS)!=0);
}

void set_user_enabled(jvmtiEnv *jvmtienv, CVMExecEnv *ee,
					  jvmtiEvent event_type, CVMBool enabled) {
  JvmtiEventEnabled *eventp;
  jlong bits;
  jlong mask;
  JvmtiEnv* jvmti_env =
	(JvmtiEnv*)((int)jvmtienv - CVMoffsetof(JvmtiEnv, _jvmti_external));

  if (ee != NULL) {
	eventp = &ee->_jvmtiThreadEventEnabled;
  } else {
	eventp = &jvmti_env->_env_event_enable._event_user_enabled;
  }
  bits = eventp->_enabled_bits;
  mask = bit_for(event_type);
  if (enabled) {
	bits |= mask;
  } else {
	bits &= ~mask;
  }
  eventp->_enabled_bits = bits;
}

static jvmtiError JNICALL
jvmti_SetEventNotificationMode(jvmtiEnv* jvmtienv,
							   jvmtiEventMode mode,
							   jvmtiEvent event_type,
							   jthread event_thread,
							   ...) {
  CVMExecEnv* ee = NULL;
  CVMBool enabled;
  JvmtiEnv* jvmti_env =
	(JvmtiEnv*)((int)jvmtienv - CVMoffsetof(JvmtiEnv, _jvmti_external));
  if (!jvmti_env->_is_valid) {
	return JVMTI_ERROR_INVALID_ENVIRONMENT;
  }
  if (event_thread != NULL) {
	ee = jthreadToExecEnv(CVMgetEE(), event_thread);

	if (ee == NULL) {
	  return JVMTI_ERROR_THREAD_NOT_ALIVE;
	}
  }

  /* event_type must be valid */
  if (!is_valid_event_type(event_type)) {
	return JVMTI_ERROR_INVALID_EVENT_TYPE;
  }

  /* global events cannot be controlled at thread level. */
  if (event_thread != NULL && is_global_event(event_type)) {
	return JVMTI_ERROR_ILLEGAL_ARGUMENT;
  }
	   
  enabled = (mode == JVMTI_ENABLE);
  /* assure that needed capabilities are present */
  if (enabled && !has_event_capability(event_type,
									   &jvmti_env->_current_capabilities)) {
	return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
  }

  if (event_type == JVMTI_EVENT_SINGLE_STEP) {
	if (event_thread == NULL) {
	  return JVMTI_ERROR_INVALID_THREAD;
	} else {
	  DEBUGGER_LOCK(ee);
	  ee->jvmtiSingleStepping = enabled;
	  DEBUGGER_UNLOCK(ee);
	}
  } else if (event_thread != NULL) {
	ThreadNode *node;
	
	DEBUGGER_LOCK(ee);
	
	node = CVMjvmtiFindThread(ee, event_thread);
	if (node == NULL) {
	  return JVMTI_ERROR_INVALID_THREAD;
	} else {
	  CVMjvmtiEnableThreadEvents(ee, node, event_type, enabled);
	}
	DEBUGGER_UNLOCK(ee);
  }

  /*	   
  if (event_type == JVMTI_EVENT_CLASS_FILE_LOAD_HOOK && enabled) {
	record_class_file_load_hook_enabled();
  }
  */
  set_user_enabled(jvmtienv, ee, event_type, enabled);
  /* Need to set global event flag which is 'or' of user and callback events */
  jvmti_env->_env_event_enable._event_enabled._enabled_bits =
	jvmti_env->_env_event_enable._event_user_enabled._enabled_bits |
	jvmti_env->_env_event_enable._event_callback_enabled._enabled_bits;
  if (jvmti_env->_env_event_enable._event_enabled._enabled_bits &
	  (bit_for(JVMTI_EVENT_THREAD_START) | bit_for(JVMTI_EVENT_THREAD_END))) {
	jvmti_env->_thread_events_enabled = CVM_TRUE;
  }
  
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GenerateEvents(jvmtiEnv* env,
					 jvmtiEvent event_type) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Extension Mechanism functions
 */ 

static jvmtiError JNICALL
jvmti_GetExtensionFunctions(jvmtiEnv* env,
							jint* extension_count_ptr,
							jvmtiExtensionFunctionInfo** extensions) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetExtensionEvents(jvmtiEnv* env,
						 jint* extension_count_ptr,
						 jvmtiExtensionEventInfo** extensions) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_SetExtensionEventCallback(jvmtiEnv* env,
								jint extension_event_index,
								jvmtiExtensionEvent callback) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Capability functions
 */ 
jvmtiCapabilities *get_capabilities(JvmtiEnv *jvmti_env) {
  return &jvmti_env->_current_capabilities;
}

jvmtiCapabilities *get_prohibited_capabilities(JvmtiEnv *jvmti_env) {
  return &jvmti_env->_prohibited_capabilities;
}

static jvmtiError JNICALL
jvmti_GetPotentialCapabilities(jvmtiEnv* jvmtienv,
							   jvmtiCapabilities* capabilities_ptr) {
  JvmtiEnv* jvmti_env =
	(JvmtiEnv*)((int)jvmtienv - CVMoffsetof(JvmtiEnv, _jvmti_external));
  if (!jvmti_env->_is_valid) {
	return JVMTI_ERROR_INVALID_ENVIRONMENT;
  }
  get_potential_capabilities(get_capabilities(jvmti_env),
							 get_prohibited_capabilities(jvmti_env),
							 capabilities_ptr);
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_AddCapabilities(jvmtiEnv* jvmtienv,
					  const jvmtiCapabilities* capabilities_ptr) {
  JvmtiEnv* jvmti_env =
	(JvmtiEnv*)((int)jvmtienv - CVMoffsetof(JvmtiEnv, _jvmti_external));
  if (!jvmti_env->_is_valid) {
	return JVMTI_ERROR_INVALID_ENVIRONMENT;
  }

  return add_capabilities(get_capabilities(jvmti_env),
						  get_prohibited_capabilities(jvmti_env),
						  capabilities_ptr, 
						  get_capabilities(jvmti_env));
}


static jvmtiError JNICALL
jvmti_RelinquishCapabilities(jvmtiEnv* jvmtienv,
							 const jvmtiCapabilities* capabilities_ptr) {
  JvmtiEnv* jvmti_env =
	(JvmtiEnv*)((int)jvmtienv - CVMoffsetof(JvmtiEnv, _jvmti_external));
  if (!jvmti_env->_is_valid) {
	return JVMTI_ERROR_INVALID_ENVIRONMENT;
  }
  relinquish_capabilities(get_capabilities(jvmti_env), capabilities_ptr,
						  get_capabilities(jvmti_env));
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetCapabilities(jvmtiEnv* jvmtienv,
					  jvmtiCapabilities* capabilities_ptr) {
  JvmtiEnv* jvmti_env =
	(JvmtiEnv*)((int)jvmtienv - CVMoffsetof(JvmtiEnv, _jvmti_external));
  if (!jvmti_env->_is_valid) {
	return JVMTI_ERROR_INVALID_ENVIRONMENT;
  }
  copy_capabilities(get_capabilities(jvmti_env), capabilities_ptr);
  return JVMTI_ERROR_NONE;
}


/*
 * Timers functions
 */ 

static jvmtiError JNICALL
jvmti_GetCurrentThreadCpuTimerInfo(jvmtiEnv* env,
								   jvmtiTimerInfo* info_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetCurrentThreadCpuTime(jvmtiEnv* env,
							  jlong* nanos_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetThreadCpuTimerInfo(jvmtiEnv* env,
							jvmtiTimerInfo* info_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetThreadCpuTime(jvmtiEnv* env,
					   jthread thread,
					   jlong* nanos_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetTimerInfo(jvmtiEnv* env,
				   jvmtiTimerInfo* info_ptr) {

  return JVMTI_ERROR_ACCESS_DENIED;
}

static jvmtiError JNICALL
jvmti_GetTime(jvmtiEnv* env,
			  jlong* nanos_ptr) {

  return JVMTI_ERROR_ACCESS_DENIED;
}

static jvmtiError JNICALL
jvmti_GetAvailableProcessors(jvmtiEnv* env,
							 jint* processor_count_ptr) {

  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * Class Loader Search functions
 */ 

static jvmtiError JNICALL
jvmti_AddToBootstrapClassLoaderSearch(jvmtiEnv* env,
									  const char* segment) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_AddToSystemClassLoaderSearch(jvmtiEnv* env,
								   const char* segment) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * System Properties functions
 */ 

static jvmtiError JNICALL
jvmti_GetSystemProperties(jvmtiEnv* env,
						  jint* count_ptr,
						  char*** property_ptr) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetSystemProperty(jvmtiEnv* jvmtienv,
						const char* property,
						char** value_ptr) {

  jstring valueString;
  jstring nameString;
  jmethodID systemGetProperty;
  const char *utf;
  JNIEnv *env;
  jclass systemClass;
  CVMExecEnv *ee = CVMgetEE();

  CHECK_JVMTI_ENV

  env = CVMexecEnv2JniEnv(ee);
  if ((*env)->PushLocalFrame(env, 1) < 0) {
	return JVMTI_ERROR_OUT_OF_MEMORY;
  }

  nameString = (*env)->NewStringUTF(env, property);
  if ((*env)->ExceptionOccurred(env)) {
	(*env)->ExceptionClear(env);
	/* NULL will be returned below */
  } else {
	systemClass = CVMcbJavaInstance(CVMsystemClass(java_lang_System));
	systemGetProperty = (*env)->GetStaticMethodID(env, systemClass, 
					"getProperty", "(Ljava/lang/String;)Ljava/lang/String;");

	valueString = (*env)->CallStaticObjectMethod(env, systemClass, 
											 systemGetProperty, nameString);
	if ((*env)->ExceptionOccurred(env)) {
	  (*env)->ExceptionClear(env);
	} else if (valueString != NULL) {
	  utf = (*env)->GetStringUTFChars(env, valueString, NULL);
	  ALLOC(jvmtienv, strlen(utf)+1, *value_ptr);
	  strcpy(*value_ptr, utf);
	  (*env)->ReleaseStringUTFChars(env, valueString, utf);
	}
  }

  (*env)->PopLocalFrame(env, 0);

  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_SetSystemProperty(jvmtiEnv* env,
						const char* property,
						const char* value) {
  return JVMTI_ERROR_ACCESS_DENIED;
}


/*
 * General functions
 */ 

static jvmtiError JNICALL
jvmti_GetPhase(jvmtiEnv* env,
			   jvmtiPhase* phase_ptr) {

  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_DisposeEnvironment(jvmtiEnv* jvmtienv) {

  JvmtiEnv* jvmti_env =
	(JvmtiEnv*)((int)jvmtienv - CVMoffsetof(JvmtiEnv, _jvmti_external));
  if (!jvmti_env->_is_valid) {
	return JVMTI_ERROR_INVALID_ENVIRONMENT;
  }

  jvmtiCapabilities *cap = get_capabilities(jvmti_env);
  relinquish_capabilities(cap, cap, cap);
  CVMdestroyJvmti(jvmti_env);
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_SetEnvironmentLocalStorage(jvmtiEnv* env,
								 const void* data) {

  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetEnvironmentLocalStorage(jvmtiEnv* env,
								 void** data_ptr) {

  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetVersionNumber(jvmtiEnv* env,
					   jint* version_ptr) {
  *version_ptr = JVMTI_VERSION;
  return JVMTI_ERROR_NONE;
}


static jvmtiError JNICALL
jvmti_GetErrorName(jvmtiEnv* env,
				   jvmtiError error,
				   char** name_ptr) {

  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_SetVerboseFlag(jvmtiEnv* env,
					 jvmtiVerboseFlag flag,
					 jboolean value) {

  return JVMTI_ERROR_ACCESS_DENIED;
}


static jvmtiError JNICALL
jvmti_GetJLocationFormat(jvmtiEnv* env,
						 jvmtiJlocationFormat* format_ptr) {

  return JVMTI_ERROR_ACCESS_DENIED;
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
CVMcreateJvmti(JavaVM *vm, void **penv)
{
  JvmtiEnv *_env;
  initializeJVMTI();
  _env = (JvmtiEnv *)malloc(sizeof(JvmtiEnv));
  if (_env == NULL) {
	  return JNI_ENOMEM;
  }
  (void)memset(_env, 0, sizeof(JvmtiEnv));
  CVMglobals.jvmtiStatics.vm = vm;
  _env->_jvmti_external = (jvmtiInterface_1 *)&CVMjvmti_Interface;
  _env->_is_valid = JNI_TRUE;
  CVMglobals.jvmtiStatics._jvmti_env = _env;
  *penv = (void *)&_env->_jvmti_external;
  CVMglobals.jvmtiDebuggingEnabled = CVM_TRUE;
  CVMjvmtiInstrumentJNINativeInterface();
  return JNI_OK;
}

jint
CVMdestroyJvmti(JvmtiEnv *jvmti_env)
{
  CVMjvmtiStaticsDestroy(&CVMglobals.jvmtiStatics);
  free(jvmti_env);
  return JNI_OK;
}

void
CVMjvmtiStaticsInit(struct CVMjvmtiStatics * statics)
{
	statics->jvmtiInitialized = CVM_FALSE;
	statics->vm = NULL;
	statics->breakpoints = NULL;
	statics->framePops = NULL;
	statics->watchedFieldModifications = NULL;
	statics->watchedFieldAccesses = NULL;
	statics->threadList = NULL;
	statics->_jvmti_env = NULL;
}

void
CVMjvmtiStaticsDestroy(struct CVMjvmtiStatics * statics)
{
  CVMglobals.jvmtiDebuggingEnabled = CVM_FALSE;
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

