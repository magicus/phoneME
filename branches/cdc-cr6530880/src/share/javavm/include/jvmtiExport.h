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

#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/stdlib.h"

#include "javavm/include/defs.h"
#include "javavm/export/jni.h"
#include "javavm/export/jvmti.h"
#include "javavm/include/jvmtiEnv.h"

/* This class contains the JVMTI interface for the rest of CVM. */

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
   jboolean	       _should_post_object_free;
   jboolean        _should_clean_up_heap_objects;
   jboolean        _should_post_vm_object_alloc;    
   jboolean        _has_redefined_a_class;
   jboolean        _all_dependencies_are_recorded;

} _jvmtiExports;


void set_can_get_source_debug_extension(jboolean on);
void set_can_examine_or_deopt_anywhere(jboolean on);
void set_can_maintain_original_method_order(jboolean on);
void set_can_post_interpreter_events(jboolean on);
void set_can_hotswap_or_post_breakpoint(jboolean on);
void set_can_modify_any_class(jboolean on);
void set_can_walk_any_space(jboolean on);
void set_can_access_local_variables(jboolean on);
void set_can_post_exceptions(jboolean on);
void set_can_post_breakpoint(jboolean on);
void set_can_post_field_access(jboolean on);
void set_can_post_field_modification(jboolean on);
void set_can_post_method_entry(jboolean on);
void set_can_post_method_exit(jboolean on);
void set_can_pop_frame(jboolean on);
void set_can_force_early_return(jboolean on);

void set_should_post_single_step(jboolean on);
void set_should_post_field_access(jboolean on);
void set_should_post_field_modification(jboolean on);
void set_should_post_class_load(jboolean on);
void set_should_post_class_prepare(jboolean on);
void set_should_post_class_unload(jboolean on);
void set_should_post_class_file_load_hook(jboolean on);
void set_should_post_native_method_bind(jboolean on);
void set_should_post_compiled_method_load(jboolean on);
void set_should_post_compiled_method_unload(jboolean on);
void set_should_post_dynamic_code_generated(jboolean on);
void set_should_post_monitor_contended_enter(jboolean on);
void set_should_post_monitor_contended_entered(jboolean on);
void set_should_post_monitor_wait(jboolean on);
void set_should_post_monitor_waited(jboolean on);
void set_should_post_garbage_collection_start(jboolean on);
void set_should_post_garbage_collection_finish(jboolean on);
void set_should_post_data_dump(jboolean on);
void set_should_post_object_free(jboolean on);
void set_should_post_vm_object_alloc(jboolean on);

void set_should_post_thread_life(jboolean on);
void set_should_clean_up_heap_objects(jboolean on);

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
void enter_onload_phase();

/* let JVMTI know that the VM isn't up yet (and JVM_OnLoad code isn't running) */
void enter_primordial_phase();

/* let JVMTI know that the VM isn't up yet but JNI is live */
void enter_start_phase();

/* let JVMTI know that the VM is fully up and running now */
void enter_live_phase();

/* ------ can_* conditions (below) are set at OnLoad and never changed ----------*/

jboolean can_get_source_debug_extension();

/* BP, expression stack, hotswap, interp_only, local_var, monitor info */
jboolean can_examine_or_deopt_anywhere();

/* JVMDI spec requires this, does this matter for JVMTI? */
jboolean can_maintain_original_method_order();

/* any of single-step, method-entry/exit, frame-pop, and field-access/modification */
jboolean can_post_interpreter_events();

jboolean can_hotswap_or_post_breakpoint();

jboolean can_modify_any_class();

jboolean can_walk_any_space();

/* can retrieve frames, set/get local variables or hotswap */
jboolean can_access_local_variables();

/* throw or catch */
jboolean can_post_exceptions();

jboolean can_post_breakpoint();
jboolean can_post_field_access();
jboolean can_post_field_modification();
jboolean can_post_method_entry();
jboolean can_post_method_exit();
jboolean can_pop_frame();
jboolean can_force_early_return();


/* the below maybe don't have to be (but are for now) fixed conditions here ----*/
/* any events can be enabled */
jboolean should_post_thread_life();


/* ------ DYNAMIC conditions here ------------ */

jboolean should_post_single_step();
jboolean should_post_field_access();
jboolean should_post_field_modification();
jboolean should_post_class_load();
jboolean should_post_class_prepare();
jboolean should_post_class_unload();
jboolean should_post_class_file_load_hook();
jboolean should_post_native_method_bind();
jboolean should_post_compiled_method_load();
jboolean should_post_compiled_method_unload();
jboolean should_post_dynamic_code_generated();
jboolean should_post_monitor_contended_enter();
jboolean should_post_monitor_contended_entered();
jboolean should_post_monitor_wait();
jboolean should_post_monitor_waited();
jboolean should_post_data_dump();
jboolean should_post_garbage_collection_start();
jboolean should_post_garbage_collection_finish();
jboolean should_post_object_free();
jboolean should_post_vm_object_alloc();

/* we are holding objects on the heap - need to talk to GC - e.g. breakpoint info */
jboolean should_clean_up_heap_objects();

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

#define CVM_DEBUGGER_LOCK(ee)   \
	CVMsysMutexLock(ee, &CVMglobals.debuggerLock)
#define CVM_DEBUGGER_UNLOCK(ee) \
	CVMsysMutexUnlock(ee, &CVMglobals.debuggerLock)
#define CVM_DEBUGGER_IS_LOCKED(ee)					\
         CVMreentrantMutexIAmOwner(ee,					\
	   CVMsysMutexGetReentrantMutex(&CVMglobals.debuggerLock))

/*
 * JVMTI globally defined static variables.
 * jvmtiInitialized tells whether JVMTI initialization is done once
 *                  before anything that accesses event structures.
 */
typedef struct ThreadNode_ {
    CVMObjectICell* thread;        /* Global root; always allocated */
    jobject lastDetectedException; /* JNI Global Ref; allocated in
                                      CVMjvmtiNotifyDebuggerOfException */
    jboolean eventEnabled[JVMTI_MAX_EVENT_TYPE_VAL+1];
    jvmtiStartFunction startFunction;  /* for debug threads only */
    void *startFunctionArg;             /* for debug threads only */
    JvmtiEnv *_env;
    void *jvmtiPrivateData;    /* JVMTI thread-local data. */
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

/*
 * This section for use by the interpreter.
 *
 * NOTE: All of these routines are implicitly gc-safe. When the
 * interpreter calls them, the call site must be wrapped in an
 * CVMD_gcSafeExec() block.
 */

extern void CVMjvmtiNotifyDebuggerOfException(CVMExecEnv* ee,
					      CVMUint8* pc,
						  CVMObjectICell* object);
extern void CVMjvmtiNotifyDebuggerOfExceptionCatch(CVMExecEnv* ee,
						   CVMUint8* pc,
						   CVMObjectICell* object);
extern void CVMjvmtiNotifyDebuggerOfSingleStep(CVMExecEnv* ee,
					       CVMUint8* pc);
/** OBJ is expected to be NULL for static fields */
extern void CVMjvmtiNotifyDebuggerOfFieldAccess(CVMExecEnv* ee,
						CVMObjectICell* obj,
						CVMFieldBlock* fb);
/** OBJ is expected to be NULL for static fields */
extern void CVMjvmtiNotifyDebuggerOfFieldModification(CVMExecEnv* ee,
						      CVMObjectICell* obj,
						      CVMFieldBlock* fb,
						      jvalue jval);
extern void CVMjvmtiNotifyDebuggerOfThreadStart(CVMExecEnv* ee,
						CVMObjectICell* thread);
extern void CVMjvmtiNotifyDebuggerOfThreadEnd(CVMExecEnv* ee,
					      CVMObjectICell* thread);
/* The next two are a bit of a misnomer. If requested, the JVMTI
   reports three types of events to the caller: method entry (for all
   methods), method exit (for all methods), and frame pop (for a
   single frame identified in a previous call to notifyFramePop). The
   "frame push" method below may cause a method entry event to be sent
   to the debugging agent. The "frame pop" method below may cause a
   method exit event, frame pop event, both, or neither to be sent to
   the debugging agent. Also note that
   CVMjvmtiNotifyDebuggerOfFramePop (originally
   notify_debugger_of_frame_pop) checks for a thrown exception
   internally in order to allow the calling code to be sloppy about
   ensuring that events aren't sent if an exception occurred. */
extern void CVMjvmtiNotifyDebuggerOfFramePush(CVMExecEnv* ee);
extern void CVMjvmtiNotifyDebuggerOfFramePop(CVMExecEnv* ee, CVMBool is_exception, jlong);
extern void CVMjvmtiNotifyDebuggerOfClassLoad(CVMExecEnv* ee,
					      CVMObjectICell* clazz);
extern void CVMjvmtiNotifyDebuggerOfClassPrepare(CVMExecEnv* ee,
						 CVMObjectICell* clazz);
extern void CVMjvmtiNotifyDebuggerOfClassUnload(CVMExecEnv* ee,
						CVMObjectICell* clazz);
extern void CVMjvmtiNotifyDebuggerOfVmInit(CVMExecEnv* ee);
extern void CVMjvmtiNotifyDebuggerOfVmExit(CVMExecEnv* ee);

extern CVMUint8 CVMjvmtiGetBreakpointOpcode(CVMExecEnv* ee, CVMUint8* pc,
					    CVMBool notify);
extern CVMBool CVMjvmtiSetBreakpointOpcode(CVMExecEnv* ee, CVMUint8* pc,
					   CVMUint8 opcode);
extern void CVMjvmtiStaticsInit(struct CVMjvmtiStatics * statics);
extern void CVMjvmtiStaticsDestroy(struct CVMjvmtiStatics * statics);

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

#endif   /* _JAVA_JVMTIEXPORT_H_ */
