/*
 * @(#)jvmtiExport.h	1.4 06/10/27
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

#ifndef _JAVA_JVMTIEXPORT_H_
#define _JAVA_JVMTIEXPORT_H_

#ifdef CVM_JVMTI

#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/stdlib.h"

#include "javavm/include/defs.h"
#include "javavm/export/jni.h"
#include "javavm/export/jvmti.h"
#include "javavm/include/jvmtiEnv.h"

/* This class contains the JVMTI interface for the rest of CVM. */

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

#define	  NEED_FRAME_POP_BIT (((jlong)1) << (CVM_NEED_FRAME_POP - TOTAL_MIN_EVENT_TYPE_VAL))

typedef struct {
   int         _field_access_count;
   int         _field_modification_count;

   jboolean        _can_get_source_debug_extension;
   jboolean        _can_examine_or_deopt_anywhere;
   jboolean        _can_maintain_original_method_order;
   jboolean        _can_post_interpreter_events;
   jboolean        _can_hotswap_or_post_breakpoint;
   jboolean        _can_modify_any_class;
   jboolean	       _can_walk_any_space;
   jboolean        _can_access_local_variables;
   jboolean        _can_post_exceptions;
   jboolean        _can_post_breakpoint;
   jboolean        _can_post_field_access;
   jboolean        _can_post_field_modification;
   jboolean        _can_post_method_entry;
   jboolean        _can_post_method_exit;
   jboolean        _can_pop_frame;
   jboolean        _can_force_early_return;

   jboolean        _should_post_single_step;
   jboolean        _should_post_field_access;
   jboolean        _should_post_field_modification;
   jboolean        _should_post_class_load;
   jboolean        _should_post_class_prepare;
   jboolean        _should_post_class_unload;
   jboolean        _should_post_class_file_load_hook;
   jboolean        _should_post_native_method_bind;
   jboolean        _should_post_compiled_method_load;
   jboolean        _should_post_compiled_method_unload;
   jboolean        _should_post_dynamic_code_generated;
   jboolean        _should_post_monitor_contended_enter;
   jboolean        _should_post_monitor_contended_entered;
   jboolean        _should_post_monitor_wait;
   jboolean        _should_post_monitor_waited;
   jboolean        _should_post_data_dump;
   jboolean        _should_post_garbage_collection_start;
   jboolean        _should_post_garbage_collection_finish;
   jboolean        _should_post_thread_life;
   jboolean	   _should_post_object_free;
   jboolean        _should_clean_up_heap_objects;
   jboolean        _should_post_vm_object_alloc;    
   jboolean        _has_redefined_a_class;
   jboolean        _all_dependencies_are_recorded;

} _jvmtiExports;


void jvmti_set_can_get_source_debug_extension(jboolean on);
void jvmti_set_can_examine_or_deopt_anywhere(jboolean on);
void jvmti_set_can_maintain_original_method_order(jboolean on);
void jvmti_set_can_post_interpreter_events(jboolean on);
void jvmti_set_can_hotswap_or_post_breakpoint(jboolean on);
void jvmti_set_can_modify_any_class(jboolean on);
void jvmti_set_can_walk_any_space(jboolean on);
void jvmti_set_can_access_local_variables(jboolean on);
void jvmti_set_can_post_exceptions(jboolean on);
void jvmti_set_can_post_breakpoint(jboolean on);
void jvmti_set_can_post_field_access(jboolean on);
void jvmti_set_can_post_field_modification(jboolean on);
void jvmti_set_can_post_method_entry(jboolean on);
void jvmti_set_can_post_method_exit(jboolean on);
void jvmti_set_can_pop_frame(jboolean on);
void jvmti_set_can_force_early_return(jboolean on);

void jvmti_set_should_post_single_step(jboolean on);
void jvmti_set_should_post_field_access(jboolean on);
void jvmti_set_should_post_field_modification(jboolean on);
void jvmti_set_should_post_class_load(jboolean on);
void jvmti_set_should_post_class_prepare(jboolean on);
void jvmti_set_should_post_class_unload(jboolean on);
void jvmti_set_should_post_class_file_load_hook(jboolean on);
void jvmti_set_should_post_native_method_bind(jboolean on);
void jvmti_set_should_post_compiled_method_load(jboolean on);
void jvmti_set_should_post_compiled_method_unload(jboolean on);
void jvmti_set_should_post_dynamic_code_generated(jboolean on);
void jvmti_set_should_post_monitor_contended_enter(jboolean on);
void jvmti_set_should_post_monitor_contended_entered(jboolean on);
void jvmti_set_should_post_monitor_wait(jboolean on);
void jvmti_set_should_post_monitor_waited(jboolean on);
void jvmti_set_should_post_garbage_collection_start(jboolean on);
void jvmti_set_should_post_garbage_collection_finish(jboolean on);
void jvmti_set_should_post_data_dump(jboolean on);
void jvmti_set_should_post_object_free(jboolean on);
void jvmti_set_should_post_vm_object_alloc(jboolean on);

void jvmti_set_should_post_thread_life(jboolean on);
void jvmti_set_should_clean_up_heap_objects(jboolean on);
jlong jvmti_get_thread_event_enabled(CVMExecEnv *ee);
void jvmti_set_should_post_any_thread_event(CVMExecEnv *ee, jlong enabled);


enum {
  JVMTI_VERSION_MASK   = 0x70000000,
  JVMTI_VERSION_VALUE  = 0x30000000,
  JVMDI_VERSION_VALUE  = 0x20000000
};


void set_has_redefined_a_class();

jboolean has_redefined_a_class();

jboolean all_dependencies_are_recorded();

void set_all_dependencies_are_recorded(jboolean on);


/* let JVMTI know that the JVM_OnLoad code is running */
void CVMjvmtiEnterOnloadPhase();

/* let JVMTI know that the VM isn't up yet (and JVM_OnLoad code isn't running) */
void CVMjvmtiEnterPrimordialPhase();

/* let JVMTI know that the VM isn't up yet but JNI is live */
void CVMjvmtiEnterStartPhase();

/* let JVMTI know that the VM is fully up and running now */
void CVMjvmtiEnterLivePhase();

/* let JVMTI know that the VM is dead, dead, dead.. */
void CVMjvmtiEnterDeadPhase();

jvmtiPhase CVMjvmtiGetPhase();

/* ------ can_* conditions (below) are set at OnLoad and never changed ----------*/

jboolean can_get_source_debug_extension();

/* BP, expression stack, hotswap, interp_only, local_var, monitor info */
jboolean can_examine_or_deopt_anywhere();

/* JVMDI spec requires this, does this matter for JVMTI? */
jboolean can_maintain_original_method_order();

/* any of single-step, method-entry/exit, frame-pop, and field-access/modification */
jboolean jvmti_can_post_interpreter_events();

jboolean jvmti_can_hotswap_or_post_breakpoint();

jboolean jvmti_can_modify_any_class();

jboolean jvmti_can_walk_any_space();

/* can retrieve frames, set/get local variables or hotswap */
jboolean jvmti_can_access_local_variables();

/* throw or catch */
jboolean jvmti_can_post_exceptions();

jboolean jvmti_can_post_breakpoint();
jboolean jvmti_can_post_field_access();
jboolean jvmti_can_post_field_modification();
jboolean jvmti_can_post_method_entry();
jboolean jvmti_can_post_method_exit();
jboolean jvmti_can_pop_frame();
jboolean jvmti_can_force_early_return();


/* the below maybe don't have to be (but are for now) fixed conditions here ----*/
/* any events can be enabled */
jboolean jvmti_should_post_thread_life();


/* ------ DYNAMIC conditions here ------------ */

jboolean jvmti_should_post_single_step();
jboolean jvmti_should_post_field_access();
jboolean jvmti_should_post_field_modification();
jboolean jvmti_should_post_class_load();
jboolean jvmti_should_post_class_prepare();
jboolean jvmti_should_post_class_unload();
jboolean jvmti_should_post_class_file_load_hook();
jboolean jvmti_should_post_native_method_bind();
jboolean jvmti_should_post_compiled_method_load();
jboolean jvmti_should_post_compiled_method_unload();
jboolean jvmti_should_post_dynamic_code_generated();
jboolean jvmti_should_post_monitor_contended_enter();
jboolean jvmti_should_post_monitor_contended_entered();
jboolean jvmti_should_post_monitor_wait();
jboolean jvmti_should_post_monitor_waited();
jboolean jvmti_should_post_data_dump();
jboolean jvmti_should_post_garbage_collection_start();
jboolean jvmti_should_post_garbage_collection_finish();
jboolean jvmti_should_post_object_free();
jboolean jvmti_should_post_vm_object_alloc();

/* we are holding objects on the heap - need to talk to GC - e.g. breakpoint info */
jboolean jvmti_should_clean_up_heap_objects();

/* ----------------- */

jboolean is_jvmti_version(jint version);
jboolean is_jvmdi_version(jint version);
jint get_jvmti_interface(JavaVM *jvm, void **penv, jint version);
  
/* SetNativeMethodPrefix support */
char** get_all_native_method_prefixes(int* count_ptr);

/* call after CMS has completed referencing processing */
void cms_ref_processing_epilogue();

typedef struct AttachOperation_ {
  char *args;
} AttachOperation;

/*
 * Definitions for the debugger events and operations.
 * Only for use when CVM_JVMTI is defined.
 */



/* This #define is necessary to prevent the declaration of 2 jvmti static
   variables (in jvmti.h) which is not needed in the VM: */
#ifndef NO_JVMTI_MACROS
#define NO_JVMTI_MACROS
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

#define CVM_THREAD_LOCK(ee)	  CVMsysMutexLock(ee, &CVMglobals.threadLock)
#define CVM_THREAD_UNLOCK(ee) CVMsysMutexUnlock(ee, &CVMglobals.threadLock)

/*
 * JVMTI globally defined static variables.
 * jvmtiInitialized tells whether JVMTI initialization is done once
 *                  before anything that accesses event structures.
 */
typedef struct ThreadNode_ {
    CVMObjectICell* thread;        /* Global root; always allocated */
    jobject lastDetectedException; /* JNI Global Ref; allocated in
                                      CVMjvmtiPostExceptionEvent */
    jboolean eventEnabled[JVMTI_MAX_EVENT_TYPE_VAL+1];
    jvmtiStartFunction startFunction;  /* for debug threads only */
    void *startFunctionArg;             /* for debug threads only */
    JvmtiEnv *_env;
    void *jvmtiPrivateData;    /* JVMTI thread-local data. */
    CVMClassBlock *redefine_cb;  /* current class being redefined */
    struct ThreadNode_ *next;
} ThreadNode;

typedef struct CVMjvmtiStatics {
  /* event hooks, etc */
  CVMBool jvmtiInitialized;
  JavaVM* vm;
  jvmtiEventCallbacks *eventHook;
  /*  JVMTI_AllocHook allocHook; */
  /*  JVMTI_DeallocHook deallocHook; */
  struct CVMBag* breakpoints;
  struct CVMBag* framePops;
  struct CVMBag* watchedFieldModifications;
  struct CVMBag* watchedFieldAccesses;
  volatile ThreadNode *threadList;
  JvmtiEnv *_jvmti_env;
  CVMUint32 eventEnable[JVMTI_MAX_EVENT_TYPE_VAL+1];
} JVMTI_Static;

typedef struct CVMJvmtiRecord {
    CVMGCLocker gcLocker;
    CVMBool gcWasStarted;
    CVMBool dataDumpRequested;
}CVMJvmtiRecord;

jint CVMcreateJvmti(JavaVM *interfaces_vm, void **penv);
jvmtiError CVMinitializeJVMTI();

/* Purpose: Indicate that we have started a GC cycle (independent of whether
            actual GC'ing has been blocked or not). */
void CVMjvmtiSetGCWasStarted(void);
#define CVMjvmtiSetGCWasStarted() \
    (CVMjvmtiRec()->gcWasStarted = CVM_TRUE)

/* Purpose: Indicate that we have ended a GC cycle which was started. */
void CVMjvmtiResetGCWasStarted(void);
#define CVMjvmtiResetGCWasStarted() \
    (CVMjvmtiRec()->gcWasStarted = CVM_FALSE)

/* Purpose: Check if we have started a GC cycle. */
CVMBool CVMjvmtiGCWasStarted(void);
#define CVMjvmtiGCWasStarted() \
    (CVMjvmtiRec()->gcWasStarted)

/* Purpose: Gets the global CVMJvmtiRecord. */
#define CVMjvmtiRec()   (&CVMglobals.jvmtiRecord)

#define CHECK_PHASE(x) { \
	if (CVMjvmtiGetPhase() != (x)) {     \
	    return JVMTI_ERROR_WRONG_PHASE;  \
	}				     \
    }

#define CHECK_PHASE2(x, y) {		     \
	if (CVMjvmtiGetPhase() != (x) &&     \
	    CVMjvmtiGetPhase() != (y)) {     \
	    return JVMTI_ERROR_WRONG_PHASE;  \
	}				     \
    }
#define CHECK_PHASEV(x) { \
	if (CVMjvmtiGetPhase() != (x)) {     \
	    return;  \
	}				     \
    }

#define CHECK_PHASEV2(x, y) {		     \
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
 * CVMD_gcSafeExec() block.
 */

extern void CVMjvmtiPostExceptionEvent(CVMExecEnv* ee,
				       CVMUint8* pc,
				       CVMObjectICell* object);
extern void CVMjvmtiPostExceptionCatchEvent(CVMExecEnv* ee,
					    CVMUint8* pc,
					    CVMObjectICell* object);
extern void CVMjvmtiPostSingleStepEvent(CVMExecEnv* ee,
					CVMUint8* pc);
/** OBJ is expected to be NULL for static fields */
extern void CVMjvmtiPostFieldAccessEvent(CVMExecEnv* ee,
					 CVMObjectICell* obj,
					 CVMFieldBlock* fb);
/** OBJ is expected to be NULL for static fields */
extern void CVMjvmtiPostFieldModificationEvent(CVMExecEnv* ee,
					       CVMObjectICell* obj,
					       CVMFieldBlock* fb,
					       jvalue jval);
extern void CVMjvmtiPostThreadStartEvent(CVMExecEnv* ee,
					 CVMObjectICell* thread);
extern void CVMjvmtiPostThreadEndEvent(CVMExecEnv* ee,
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
   notify_debugger_of_frame_pop) checks for a thrown exception
   internally in order to allow the calling code to be sloppy about
   ensuring that events aren't sent if an exception occurred. */
extern void CVMjvmtiPostFramePushEvent(CVMExecEnv* ee);
extern void CVMjvmtiPostFramePopEvent(CVMExecEnv* ee, CVMBool isRef,
				      CVMBool is_exception, jlong);
extern void CVMjvmtiPostClassLoadEvent(CVMExecEnv* ee,
				  CVMObjectICell* clazz);
extern void CVMjvmtiPostClassPrepareEvent(CVMExecEnv* ee,
						 CVMObjectICell* clazz);
extern void CVMjvmtiPostClassUnloadEvent(CVMExecEnv* ee,
						CVMObjectICell* clazz);
extern void CVMjvmtiPostVmStartEvent(CVMExecEnv* ee);
extern void CVMjvmtiPostVmInitEvent(CVMExecEnv* ee);
extern void CVMjvmtiPostVmExitEvent(CVMExecEnv* ee);

extern CVMUint8 CVMjvmtiGetBreakpointOpcode(CVMExecEnv* ee, CVMUint8* pc,
					    CVMBool notify);
extern CVMBool CVMjvmtiSetBreakpointOpcode(CVMExecEnv* ee, CVMUint8* pc,
					   CVMUint8 opcode);
extern void CVMjvmtiStaticsInit(struct CVMjvmtiStatics * statics);
extern void CVMjvmtiStaticsDestroy(struct CVMjvmtiStatics * statics);

extern void CVMjvmtiPostClassLoadHookEvent(jclass klass,
					   CVMClassLoaderICell *loader,
					   const char *class_name,
					   jobject protectionDomain,
					   CVMInt32 bufferLength,
					   CVMUint8 *buffer,
					   CVMInt32 *new_bufferLength,
					   CVMUint8 **new_buffer);
extern void CVMjvmtiPostCompiledMethodLoadEvent(CVMExecEnv *ee,
						CVMMethodBlock *mb);
extern void CVMjvmtiPostCompiledMethodUnloadEvent(CVMExecEnv *ee,
						  CVMMethodBlock* mb);
extern void CVMjvmtiPostDataDumpRequest(void);
extern void CVMjvmtiPostGCStartEvent(void);
extern void CVMjvmtiPostGCFinishEvent(void);
extern void CVMjvmtiPostStartUpEvents(CVMExecEnv *ee);
extern void CVMjvmtiPostNativeMethodBind(CVMExecEnv *ee, CVMMethodBlock *mb,
					 CVMUint8 *nativeCode,
					 CVMUint8 **new_nativeCode);
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
    (CVMjvmtiRec()->dataDumpRequested = CVM_TRUE)


#define CVMjvmtiEventsEnabled()	\
    (CVMglobals.jvmtiStatics.jvmtiInitialized == CVM_TRUE)

#define CVMjvmtiThreadEventsEnabled(ee)	\
  ((ee)->debugEventsEnabled && CVMglobals.jvmtiStatics._jvmti_env != NULL && \
   CVMglobals.jvmtiStatics._jvmti_env->_thread_events_enabled)

extern void CVMjvmtiEnableThreadEvents(CVMExecEnv*, ThreadNode *, jvmtiEvent, jboolean);
/*
 * This function is used by CVMjniGetEnv.
 */
extern jvmtiInterface_1* CVMjvmtiGetInterface1(JavaVM* interfaces_vm);

ThreadNode *CVMjvmtiFindThread(CVMExecEnv* ee, CVMObjectICell* thread);
ThreadNode *CVMjvmtiInsertThread(CVMExecEnv* ee, CVMObjectICell* thread);
jboolean CVMjvmtiRemoveThread(CVMObjectICell *thread);

jvmtiError CVMjvmtiAllocate(jlong size, unsigned char **mem);
jvmtiError CVMjvmtiDeallocate(unsigned char *mem);
CVMBool CVMjvmtiClassBeingRedefined(CVMExecEnv *ee, CVMClassBlock *cb);
CVMBool CVMjvmtiDebuggerConnected();
void CVMjvmtiSetDebuggerConnected(CVMBool connected);
const CVMClassBlock *
CVMjvmtiClassInstance2ClassBlock(CVMExecEnv *ee, CVMObject *obj);
void CVMjvmtiRehash(void);

#endif   /* _CVM_JVMTI */
#endif   /* _JAVA_JVMTIEXPORT_H_ */
