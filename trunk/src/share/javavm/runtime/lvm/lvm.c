/*
 * @(#)lvm.c	1.10 06/10/10
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

#ifndef CVM_REMOTE_EXCEPTIONS_SUPPORTED
#error Remote exception support is required for Logical VM
#endif


#include "javavm/include/defs.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/preloader.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/globals.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/globalroots.h"
#include "javavm/export/jni.h"
#include "native/common/jni_util.h"


/* For printing heavy duty debugging messages, etc. */
/* #define EXTRA_DEBUG */
#undef  EXTRA_DEBUG_EXEC
#if defined(CVM_DEBUG) && defined(EXTRA_DEBUG)
#define EXTRA_DEBUG_EXEC(cmd) cmd 
#else
#define EXTRA_DEBUG_EXEC(cmd) ((void)0)
#endif

/* Debug helpers */
#undef  INVALID_VAL
#define INVALID_VAL (0x55555555) /* == 1431655765 */
#undef  INVALID_PTR
#define INVALID_PTR ((void*)INVALID_VAL)

#undef  ASSERT_VALID_VAL
#define ASSERT_VALID_VAL(n) CVMassert((n) != 0 && (n) != INVALID_VAL)
#undef  ASSERT_VALID_PTR
#define ASSERT_VALID_PTR(p) CVMassert((p) != NULL && (void*)(p) != INVALID_PTR)

/* Lock for syncing modifications on a CVMLVMContext: */
#undef  LVM_LOCK
#define LVM_LOCK(ee)	CVMmutexLock(&CVMglobals.lvm.lvmLock)
#undef  LVM_UNLOCK
#define LVM_UNLOCK(ee)	CVMmutexUnlock(&CVMglobals.lvm.lvmLock)


#if defined(CVM_DEBUG) && defined(EXTRA_DEBUG)
void
CVMLVMcontextDumpAll(CVMExecEnv *ee)
{
    CVMLVMContext *context;

    LVM_LOCK(ee);
    context = CVMglobals.lvm.contexts;
    {
	CVMUint32 numRef = (context != NULL)?(context->statics[0].raw):(-1);

	CVMconsolePrintf("*** LVM: CVMLVMcontextDumpAll:\n");

	while (context != NULL) {
	    CVMconsolePrintf("\t%2d: %I, context: 0x%x\n",
		context->id, context->lvmICell, context);

	    /* Sanity check: the number of object references stored at the 
	     * 0th slot should be the same across all the statics buffers. */
	    CVMassert(numRef == context->statics[0].raw);
	    CVMassert(context->next == NULL
		      || context->next->prevPtr == &context->next);
	    context = context->next;
	}
    }
    LVM_UNLOCK(ee);
}
#endif /* CVM_DEBUG && EXTRA_DEBUG */

/*
 * Allocate a CVMLVMContext structure, initialize it, and
 * link it in the list of CVMLVMContexts.
 */
static CVMLVMContext *
CVMLVMcontextAlloc(CVMExecEnv* ee, jobject lvmObj)
{
    CVMLVMContext *context;
    CVMLVMGlobals *lvm = &CVMglobals.lvm;
    CVMUint32 numStatics = lvm->numStatics;
    const CVMJavaVal32* staticDataMaster = lvm->staticDataMaster;

    ASSERT_VALID_VAL(numStatics);
    ASSERT_VALID_PTR(staticDataMaster);

    /* Allocate the buffer. Note that CVMLVMContext already includes 
     * one slot for a static field (so subtract 1 from numStatics). */
    context = (CVMLVMContext *)
	malloc(sizeof(CVMLVMContext) + (numStatics-1) * sizeof(CVMJavaVal32));
    if (context == NULL) {
	return NULL; /* Out of memory */
    }

    if (lvmObj == NULL) {
	/* Should be invoked during the VM startup. The main LVM is not
	 * initialized.  Cannot allocate global root yet. */
	CVMassert(lvm->mainLVM == NULL);
    } else {
	context->lvmICell = CVMID_getGlobalRoot(ee);
	if (context->lvmICell == NULL) {
	    free(context);
	    return NULL; /* Out of memory */
	}
	CVMID_icellAssign(ee, context->lvmICell, lvmObj);
    }

    context->halting = CVM_FALSE;

    /* systemClassLoader is initialized lazily in
     * CVMclassGetSystemClassLoader() */
    context->systemClassLoader = NULL;

    /* Initialize the static fields */
    memcpy(context->statics, staticDataMaster,
	   numStatics * sizeof(CVMJavaVal32));

    /* Link it into the linked list */
    LVM_LOCK(ee);

    context->id = lvm->numberOfContexts++;
    {
	CVMLVMContext **list= &lvm->contexts;

	context->prevPtr = list;
	context->next = *list;
	if (*list != NULL) {
	    /* At least one context exists; the main context must
	     * have been initialized. */
	    CVMassert(lvm->mainLVM != NULL);
	    (*list)->prevPtr = &context->next;
	}
	*list = context;
    }
    LVM_UNLOCK(ee);

    EXTRA_DEBUG_EXEC(CVMconsolePrintf("*** LVM[%d]: CVMLVMcontextAlloc: "
	"context: 0x%x\n", context->id, context));

    return context;
}

static void
CVMLVMcontextFree(CVMExecEnv *ee, CVMLVMContext *context)
{
    CVMwithAssertsOnly(CVMLVMGlobals* lvm = &CVMglobals.lvm);

    /* There should be at least one context: */
    ASSERT_VALID_PTR(lvm->contexts);

    ASSERT_VALID_PTR(context);
    ASSERT_VALID_PTR(context->prevPtr);
    ASSERT_VALID_PTR(context->lvmICell);
    ASSERT_VALID_VAL(context->id + 1 /* id can be 0 */);

    EXTRA_DEBUG_EXEC(CVMconsolePrintf("*** LVM[%d]: CVMLVMcontextFree: "
	"context: 0x%x\n", context->id, context));

    /* Unlink it from the list */
    LVM_LOCK(ee);
    {
	*context->prevPtr = context->next;
	if (context->next != NULL) {
	    context->next->prevPtr = context->prevPtr;
	}
    }
    LVM_UNLOCK(ee);

    CVMID_freeGlobalRoot(ee, context->lvmICell);

    if (context->systemClassLoader != NULL) {
	CVMID_freeGlobalRoot(ee, context->systemClassLoader);
    }

    /* Invalidates entries */
    CVMwithAssertsOnly({
	context->prevPtr = (CVMLVMContext**)INVALID_PTR;
	context->next = (CVMLVMContext*)INVALID_PTR;
	context->lvmICell = (CVMObjectICell*)INVALID_PTR;
	context->halting = CVM_FALSE;
	context->systemClassLoader = (CVMObjectICell*)INVALID_PTR;
	context->id = INVALID_VAL;
	memset(context->statics, (int)((char)INVALID_VAL),
	    lvm->numStatics * sizeof(CVMJavaVal32));
    });

    free(context);
}

/*
 * Walk through the static references referred by all the contexts.
 * Invoked from CVMgcScanRoots() if CVM_LVM is defined.
 */
void
CVMLVMwalkStatics(CVMRefCallbackFunc callback, void* callbackData)
{
    CVMLVMGlobals *lvm = &CVMglobals.lvm;
    CVMLVMContext *context;

    /* TODO: This is usually only called from GC to scan all the static refs.
       Consider adding an assert here to ensure that we don't call this at
       other times.  Otherwise, we can have a race condition on the access of
       the list of LVM contexts. */

    EXTRA_DEBUG_EXEC(CVMconsolePrintf("*** LVM: CVMLVMwalkStatics: "
	"callback: 0x%x, callabckData: 0x%x\n", callback, callbackData));

    context = lvm->contexts;
    do {
	CVMUint32 numRefStatics = context->statics[0].raw;
	CVMJavaVal32* refStatics = &context->statics[1];

	EXTRA_DEBUG_EXEC(CVMconsolePrintf("\t%2d: context: 0x%x, "
	    "refStatics: 0x%x, " "numRefStatics: %d\n",
	    context->id, context, refStatics, numRefStatics));

	CVMwalkContiguousRefs(refStatics, numRefStatics, callback,
                              callbackData);

	context = context->next;
    } while (context != NULL);
}

/*
 * Logical VM global variables initialization -- Phase 1.
 * Invoke this before invoking CVMinitExecEnv().
 */
CVMBool
CVMLVMglobalsInitPhase1(CVMLVMGlobals* lvm)
{
    /* NOTE: CVMLVMGlobals is part of CVMGlobals.  Hence, it should contain
       all 0 and NULL values to begin with.  So, instead of setting the
       respective fields to 0 and NULL below, we have an assert to ensure
       that the values are as we expect it. */

    EXTRA_DEBUG_EXEC(CVMconsolePrintf("*** LVM: CVMLVMglobalsInitPhase1\n"));

    /* Just in case, make sure that CVM_nStaticData is not 
     * equal to INVALID_VAL. */
    ASSERT_VALID_VAL(CVM_nStaticData);

    lvm->numClasses = CVM_nROMClasses;
    lvm->numStatics = CVM_nStaticData;
    /* CVM_StaticDataMaster is defined as CVMInt32 by JavaCodeCompact
     * for convenience of initialization.
     * We cast it to the right type here. */
    lvm->staticDataMaster = (CVMJavaVal32*)CVM_StaticDataMaster;

    /* mainLVM will be initialized in Phase 2.
     * Need to be set to NULL since CVMLVMinitExecEnv() need to behave
     * differently if it is invoked for mainLVM, and this field is used
     * as a flag to tell it. */
    CVMassert(lvm->mainLVM == NULL);

    CVMwithAssertsOnly(lvm->mainStatics = (CVMJavaVal32*)INVALID_PTR);

    /* contexts will be initialized right after returning 
     * from this call (in CVMLVMcontextAlloc() that is invoked 
     * subsequently from CVMinitExecEnv()). */
    CVMassert(lvm->contexts == NULL);

    /* Do this here before CVMLVMinitExecEnv() gets invoked */
    CVMmutexInit(&lvm->lvmLock);

    CVMassert(lvm->numberOfContexts == 0);

    CVMtraceLVM(("LVM: Logical VM related globals initialized:\n"
	"\tNumber of target classes: %d\n"
	"\tNumber of static fields: %d\n"
	"\tPointer to static data master: 0x%x\n",
	lvm->numClasses,
	lvm->numStatics,
	lvm->staticDataMaster));

    return CVM_TRUE;
}

/*
 * Initialize the main (primordial) Logical VM.
 * Invoke private Isolate() constructor that is provided for
 * main Logical VM initialization only.
 */
static jobject
CVMLVMcreateMainLVM(JNIEnv* env)
{
    jclass cls = CVMcbJavaInstance(CVMsystemClass(sun_misc_Isolate));
    jmethodID mid = CVMjniGetMethodID(env, cls, "<init>", "()V");

    CVMassert(mid != NULL);
    return CVMjniNewObject(env, cls, mid);
}

/*
 * Logical VM global variables initialization -- Phase 2.
 * Creates the main Logical VM and save a global reference to it 
 * into CVMglobals structure.
 */
CVMBool
CVMLVMglobalsInitPhase2(CVMExecEnv* ee, CVMLVMGlobals* lvm)
{
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    jobject mainLVM;

    EXTRA_DEBUG_EXEC(CVMconsolePrintf("*** LVM: CVMLVMglobalsInitPhase2\n"));

    /* CVMLVMglobalsInitPhase1() should have run already */
    ASSERT_VALID_VAL(lvm->numClasses);
    ASSERT_VALID_VAL(lvm->numStatics);
    ASSERT_VALID_PTR(lvm->staticDataMaster);
    /* Must have been initialized in the CVMLVMinitExecEnv() call 
     * against the main 'ee'. */
    ASSERT_VALID_PTR(lvm->mainStatics);
    /* mainLVM should be NULL as set in CVMLVMglobalsInitPhase1() */
    CVMassert(lvm->mainLVM == NULL);

    mainLVM = CVMLVMcreateMainLVM(env);
    if (mainLVM == NULL) {
	return CVM_FALSE;
    }
    if (!CVMLVMjobjectInit(env, mainLVM, NULL)) {
	return CVM_FALSE;
    }

    lvm->mainLVM = CVMID_getGlobalRoot(ee);
    if (lvm->mainLVM == NULL) {
	CVMLVMjobjectDestroy(env, mainLVM);
	return CVM_FALSE;
    }

    /* Hold a global reference to the main Logical VM
     * throughout the lifecycle of the JVM */
    CVMID_icellAssign(ee, lvm->mainLVM, mainLVM);
    CVMjniDeleteLocalRef(env, mainLVM);

    /* Print the mainLVM info here using %I; we cannot use %I in 
     * earlier stage. */
    CVMtraceLVM(("LVM: Main Logical VM registered\n"));

    return CVM_TRUE;
}

CVMBool
CVMLVMfinishUpBootstrapping(CVMExecEnv* ee)
{
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMLVMGlobals* lvm = &CVMglobals.lvm;
    jclass lvmCls = CVMcbJavaInstance(CVMsystemClass(sun_misc_Isolate));
    jmethodID mid = CVMjniGetMethodID(env, lvmCls,
				      "startMainRequestHandler", "()V");

    EXTRA_DEBUG_EXEC(CVMconsolePrintf("*** LVM: "
				      "CVMLVMstartMainRequestHandler\n"));

    CVMassert(mid != NULL);
    CVMjniCallVoidMethod(env, lvm->mainLVM, mid);

    if (CVMlocalExceptionOccurred(ee)) {
	CVMtraceLVM(("LVM: Error during starting the main request handler\n"));
	return CVM_FALSE;
    }
    return CVM_TRUE;
}

/*
 * Destroy Logical VM related global states.
 * Free the global reference to the main Logical VM.
 */
void
CVMLVMglobalsDestroy(CVMExecEnv* ee, CVMLVMGlobals* lvm)
{
    ASSERT_VALID_VAL(lvm->numClasses);
    ASSERT_VALID_VAL(lvm->numStatics);
    ASSERT_VALID_PTR(lvm->staticDataMaster);
    ASSERT_VALID_PTR(lvm->mainLVM);
    ASSERT_VALID_PTR(lvm->mainStatics);
    ASSERT_VALID_PTR(lvm->contexts);

    /* systemClassLoader has been freed in CVMunloadApplicationclasses() 
     * already. Clear the reference before calling CVMLVMjobjectDestroy(). */
    CVMLVMeeGetContext(ee)->systemClassLoader = NULL;
    /* Free up mainLVM's context here at the end of VM execution */
    CVMLVMjobjectDestroy(CVMexecEnv2JniEnv(ee), lvm->mainLVM);

    CVMtraceLVM(("LVM: Main Logical VM deregistered\n"));
    CVMID_freeGlobalRoot(ee, lvm->mainLVM);
    CVMmutexDestroy(&lvm->lvmLock);
    CVMtraceLVM(("LVM: Logical VM related globals destroyed\n"));

    CVMwithAssertsOnly({
	lvm->numClasses       = INVALID_VAL;
	lvm->numStatics       = INVALID_VAL;
	lvm->staticDataMaster = (CVMJavaVal32 *)INVALID_PTR;
	lvm->mainLVM          = (jobject)INVALID_PTR;
	lvm->mainStatics      = (CVMJavaVal32 *)INVALID_PTR;
	lvm->contexts         = (CVMLVMContext *)INVALID_PTR;
    });
}

/*
 * Initialize CVMLVMEEInitAux structure from given 'ee'.
 * It is used to pass Logical VM related info from parent thread to child.
 * CVMLVMeeInitAuxDestroy() will be invoked to free up used resources
 * after initialization of the child thread's 'ee'.
 */
CVMBool 
CVMLVMeeInitAuxInit(CVMExecEnv* ee, CVMLVMEEInitAux* aux)
{
    aux->statics = ee->lvmEE.statics;

    return CVM_TRUE;
}

/* 
 * Destroy CVMLVMEEInitAux after initialization of an 'ee'.
 * Invoked from CVMinitExecEnv() via CVMLVMinitExecEnv() after
 * initialization of the 'ee'.
 * For use in an error recovery operation, this is kept as non-static.
 */
void
CVMLVMeeInitAuxDestroy(CVMExecEnv* ee, CVMLVMEEInitAux* aux)
{
    /* currently nothing to free */
    ASSERT_VALID_PTR(aux->statics);
    CVMwithAssertsOnly(aux->statics = (CVMJavaVal32*)INVALID_PTR);
}

/*
 * Initialize Logical VM specific part of 'ee' based on info in 'aux'.
 * The 'aux' argument can be NULL if the invocation is initiated by
 * CVMinitVMGlobalState() or CVMjniAttachCurrentThread().
 * Behaves differently based on from which function this is invoked.
 */
CVMBool
CVMLVMinitExecEnv(CVMExecEnv* ee, CVMLVMEEInitAux* aux)
{
    CVMLVMGlobals* lvm = &CVMglobals.lvm;
    jobject mainLVM = lvm->mainLVM;

    /* Check if this invocation is initiated by CVMinitVMGlobalState().
     * At that point, CVMglobals.mainLVM is not initialized yet. 
     * Use this fact as a key. */
    if (mainLVM == NULL) {
	/* 
	 * Invocation originated by CVMinitVMGlobalState().
	 * Must be initializing CVMglobals.mainEE.
	 * 
	 * We need to set up the static field replication feature at this
	 * very beginning of the VM initialization, since following 
	 * initialization of system classes requires it.
	 */
	CVMLVMContext *context;
	CVMassert(aux == NULL);

	/* We cannot initialized the main (primordial) Logical VM
	 * object at this point. Give NULL to CVMLVMcontextAlloc() 
	 * and initialize it later in CVMLVMjobjectInit(). */
	context = CVMLVMcontextAlloc(ee, NULL);
	if (context == NULL) {
	    return CVM_FALSE; /* Out of memory */
	}
	ee->lvmEE.context = context; 
	ee->lvmEE.statics = context->statics;
	EXTRA_DEBUG_EXEC(CVMLVMcontextDumpAll(ee));

	CVMassert(lvm->mainStatics == (CVMJavaVal32*)INVALID_PTR);
	lvm->mainStatics = context->statics;

    } else if (aux != NULL /* && mainLVM != NULL */) {
	ASSERT_VALID_PTR(mainLVM);
	/*
	 * The condition "aux != NULL && mainLVM != NULL" indicates that 
	 * this invocation is initiated by start_func() as a result of 
	 * a new Thread creation. In that case, inherit the info of Logical 
	 * VM that the parent thread belongs to.  This info is given by 'aux'.
	 */
	ee->lvmEE.context = CVMLVMstatics2Context(aux->statics);
	ee->lvmEE.statics = aux->statics;

    } else { /* aux == NULL && mainLVM != NULL */
	CVMJavaVal32* mainStatics = lvm->mainStatics;
	ASSERT_VALID_PTR(mainLVM);
	ASSERT_VALID_PTR(mainStatics);
	/*
	 * The condition "aux == NULL && mainLVM != NULL" indicates that
	 * this invocation is initiated by CVMjniAttachCurrentThread().
	 * Attach to the mainLVM in this case.
	 * 
	 * Set ee->lvmEE.statics from the global data structure.
	 * Note that we cannot get it from the mainLVM object using
	 * CVMjniGetIntField(), since the JNI function requires 
	 * ee->lvmEE.statics being initialized.  To break this circular 
	 * dependency, we store statics of main LVM in the global data 
	 * structure together with CVMObjectICell* of the main LVM object.
	 */
	ee->lvmEE.context = CVMLVMstatics2Context(mainStatics);
	ee->lvmEE.statics = mainStatics;
    }

    CVMwithAssertsOnly({
	/* Consistency check against two copies of pointer to 'context'.
	 * Check if mainLVM == NULL, since the main LVM has different
	 * order of initialization and this test cannot be performed. */
	if (mainLVM != NULL) {
	    JNIEnv* env = CVMexecEnv2JniEnv(ee);
	    CVMLVMContext *context = CVMLVMeeGetContext(ee);
	    jobject lvmObj = context->lvmICell;
	    jclass lvmCls = CVMjniGetObjectClass(env, lvmObj);
	    CVMLVMContext *contextFromObj =
		(CVMLVMContext*)CVMjniGetIntField(env, lvmObj, 
		    CVMjniGetFieldID(env, lvmCls, "context", "I"));
	    CVMassert(context == contextFromObj);
	}
    });

    EXTRA_DEBUG_EXEC(CVMconsolePrintf("*** LVM: CVMLVMinitExecEnv: "
	"context: 0x%x\n", CVMLVMeeGetContext(ee)));

    return CVM_TRUE;
}

/*
 * Destroy Logical VM related part of 'ee'.
 */
void
CVMLVMdestroyExecEnv(CVMExecEnv* ee)
{
    /* currently nothing to free */
    ASSERT_VALID_PTR(ee->lvmEE.statics);
    CVMwithAssertsOnly(ee->lvmEE.statics = (CVMJavaVal32*)INVALID_PTR);
}

/*
 * Initializes LVM classes (replicated static fields).
 * Most of the part of this function is derived from JNI_CreateJavaVM().
 * Once we cleared all the LVM initialization procedure, we should
 * think about sharing the code with JNI_CreateJavaVM().
 */
static CVMBool
CVMLVMinitSystemClasses(JNIEnv* env, jobject initThread)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);

    /* JCC preallocates Class and String objects, so we need to run
     * their static initializers as if they had been created at runtime.
     */
    if (CVMclassInit(ee, (CVMClassBlock*)CVMsystemClass(java_lang_Class))) {
	CVMclassInit(ee, (CVMClassBlock*)CVMsystemClass(java_lang_String));
    }
    if (CVMlocalExceptionOccurred(ee)) {
	CVMtraceLVM(("LVM: Error during initializing Class and String\n"));
	return CVM_FALSE;
    }
    /*
     * Create system ThreadGroup, main ThreadGroup, and reinitialize
     * the startup thread of this LVM.
     */
    {
	jclass threadClass;
	jmethodID initMethodID;

	threadClass = CVMcbJavaInstance(CVMsystemClass(java_lang_Thread));
	initMethodID = CVMjniGetMethodID(env, threadClass,
					 "reinitLVMStartupThread", "()V");
	CVMassert(initMethodID != NULL);
	CVMjniCallVoidMethod(env, initThread, initMethodID);

	if (CVMlocalExceptionOccurred(ee)) {
	    CVMtraceLVM(("LVM: Error during initializing Thread class\n"));
	    return CVM_FALSE;
	}
    }
    {
	jclass systemClass;
	jmethodID initMethodID;

	systemClass = CVMcbJavaInstance(CVMsystemClass(java_lang_System));
	initMethodID = (*env)->GetStaticMethodID(env, systemClass,
						 "initializeSystemClass",
						 "()V");
	CVMassert(initMethodID != NULL);
	(*env)->CallStaticVoidMethod(env, systemClass, initMethodID);
	
	if (CVMlocalExceptionOccurred(ee)) {
	    CVMtraceLVM(("LVM: Error during initializing System class\n"));
	    return CVM_FALSE;
	}
    }
    return CVM_TRUE;
}

/*
 * Perform farther initialization of the newly created Logical VM.
 */
CVMBool
CVMLVMjobjectInit(JNIEnv* env, jobject lvmObj, jobject initThread)
{
    jclass lvmCls = CVMjniGetObjectClass(env, lvmObj);
    jfieldID infoField = CVMjniGetFieldID(env, lvmCls, "context","I");
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMLVMContext *context;
    jboolean isMainLVM = (initThread == NULL);

    if (isMainLVM) {
	CVMassert(ee == &CVMglobals.mainEE);
	/* ee->lvmEE.statics should have been allocated and 
	 * initialized in CVMLVMinitExecEnv: */
	ASSERT_VALID_PTR(ee->lvmEE.statics);
	context = CVMLVMeeGetContext(ee);

	/* We postponed context->lvmICell initialization for 
	 * the main LVM.  Now we can finish it up. */
	context->lvmICell = CVMID_getGlobalRoot(ee);
	if (context->lvmICell == NULL) {
	    return CVM_FALSE;
	}
	CVMID_icellAssign(ee, context->lvmICell, lvmObj);

	/* Store pointers to the context in the main LVM instance. */
	CVMjniSetIntField(env, lvmObj, infoField, (jint)context);
    } else {
	/* Allocates per-Logical VM native data structure */
	context = CVMLVMcontextAlloc(ee, lvmObj);
	if (context == NULL) { /* Out of memory */
	    return CVM_FALSE;
	}

	/* Store pointers to context in the new LVM instance.
	 * Do this before switching the context. */
	CVMjniSetIntField(env, lvmObj, infoField, (jint)context);

	/* Set the newly created per-EE data cache for the new Logical VM.
	 * This effectually switches the LVM context. */
	ee->lvmEE.context = context;
	ee->lvmEE.statics = context->statics;

	CVMwithAssertsOnly(CVMpreloaderCheckROMClassInitState(ee));

	/* Now, we initialize the system classes in the new LVM context. */
	if (!CVMLVMinitSystemClasses(env, initThread)) {
	    CVMLVMcontextFree(ee, context);
	    return CVM_FALSE;
	}
    }

    CVMtraceLVM(("LVM[%d]: Created: context: 0x%x, Starter: %I\n",
	context->id, context, initThread));

    EXTRA_DEBUG_EXEC(CVMLVMcontextDumpAll(ee));

    return CVM_TRUE;
}

void
CVMLVMcontextSwitch(JNIEnv* env, jobject targetLvmObj)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jclass lvmCls = CVMjniGetObjectClass(env, targetLvmObj);
#ifdef CVM_TRACE
    CVMLVMContext *originalContext = CVMLVMeeGetContext(ee);
#endif
    jfieldID infoField;
    CVMLVMContext *targetContext;

    infoField = CVMjniGetFieldID(env, lvmCls, "context","I");
    targetContext =
	(CVMLVMContext *)CVMjniGetIntField(env, targetLvmObj, infoField);

    CVMassert(targetContext != NULL);
    CVMtraceLVM(("LVM: Thread %I switching context from [%d] to [%d]\n",
	ee->threadICell, originalContext->id, targetContext->id));

    ee->lvmEE.context = targetContext;
    ee->lvmEE.statics = targetContext->statics;
}

/*
 * Destroy Logical VM. Free up all the malloc'ed buffers.
 */
void
CVMLVMjobjectDestroy(JNIEnv* env, jobject lvmObj)
{
    jclass lvmCls = CVMjniGetObjectClass(env, lvmObj);
    CVMLVMGlobals *lvm = &CVMglobals.lvm;
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jfieldID infoField;
    CVMLVMContext *context;
    CVMBool isMainLVM;

    infoField = CVMjniGetFieldID(env, lvmCls, "context","I");
    context = (CVMLVMContext *)CVMjniGetIntField(env, lvmObj, infoField);

    /* Invalidate the field by setting NULL to it */
    CVMjniSetIntField(env, lvmObj, infoField, (jint)0);

    ASSERT_VALID_PTR(context);

    /* The main LVM is supposed to be destroyed by a thread that 
     * belongs to the main LVM itself. */
    CVMID_icellSameObject(ee, lvmObj, lvm->mainLVM, isMainLVM);
    CVMassert(!isMainLVM || context == CVMLVMeeGetContext(ee));

    CVMtraceLVM(("LVM[%d]: Destroyed: context: 0x%x\n",
	context->id, context));

    EXTRA_DEBUG_EXEC(CVMLVMcontextDumpAll(ee));

    CVMLVMcontextFree(ee, context);
}

/*
 * Halt this LVM; set ThreadRegistry.threadCreationDisabled true 
 * and throw ThreadDeath remote exception to all the thread running 
 * in this LVM.
 */
void
CVMLVMcontextHalt(CVMExecEnv* ee)
{
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMJavaVal32* currentStatics = ee->lvmEE.statics;
    CVMLVMContext *context = CVMLVMeeGetContext(ee);
    jclass threadRegistryClass;
    jobject threadDeathObj;

    /* Check if already in process of halting */
    LVM_LOCK(ee);
    if (context->halting) {
	LVM_UNLOCK(ee);
	return;
    } else {
	context->halting = CVM_TRUE;
    }
    LVM_UNLOCK(ee);

    CVMtraceLVM(("LVM[%d]: Halting: context: 0x%x\n", context->id, context));

    threadRegistryClass =
	CVMcbJavaInstance(CVMsystemClass(sun_misc_ThreadRegistry));
    CVMjniSetStaticBooleanField(env, threadRegistryClass, 
	CVMjniGetStaticFieldID(env, threadRegistryClass, 
	    "threadCreationDisabled", "Z"), JNI_TRUE);

    /* We may better use a private subclass of ThreadDeath.
     * Sharing the same ThreadDeath object for termination of all the
     * thread should be OK for now... */

    /* TODO: Change this into an LVMDeath exception that cannot be caught. */
    threadDeathObj = JNU_NewObjectByName(env, "java/lang/ThreadDeath", "()V");

    /*
     * Walk though threads and post the remote exception against those
     * who belongs to this LVM.  We use targetEE->lvmEE.statics as a 
     * key to distinguish if a thread belongs to this LVM.  This may 
     * look imperfect, since a very new thread started in this LVM may 
     * not have lvmEE.statics initialized. However, such a new thread 
     * indeed have not executed code in Thread.startup() that checks
     * ThreadRegistry.threadCreationDisabled flag.  The new thread 
     * eventually reaches this check and discontinue its execution, 
     * since we already have set the flag above.
     */
    CVMassert(CVMD_isgcSafe(ee));
    CVMsysMutexLock(ee, &CVMglobals.threadLock);
    CVM_WALK_ALL_THREADS(ee, targetEE, {
	if (targetEE->lvmEE.statics == currentStatics) {
	    CVMtraceLVM(("\tStopping Thread: %I\n", targetEE->threadICell));
	    CVMgcSafeThrowRemoteException(ee, targetEE, threadDeathObj);
	}
    });
    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

/*
 * Return CVM_TRUE when this frame's exception handers need to be
 * ignored; if this frame is of user code and the LVM this thread
 * belongs to is in process of halting.
 */
CVMBool
CVMLVMskipExceptionHandling(CVMExecEnv* ee, CVMFrameIterator* iter)
{
    CVMMethodBlock *mb = CVMframeIterateGetMb(iter);
    CVMClassBlock* cb = CVMmbClassBlock(mb);
    CVMLVMContext *context = CVMLVMeeGetContext(ee);

    return (context->halting && !CVMcbIsLVMSystemClass(cb));
}

/*
 * Return CVM_TRUE if the current thread is of the main LVM.
 */
CVMBool
CVMLVMinMainLVM(CVMExecEnv* ee)
{
    CVMLVMGlobals* lvm = &CVMglobals.lvm;
    jobject mainLVM = lvm->mainLVM;
    jobject thisLVM = CVMLVMeeGetContext(ee)->lvmICell;
    CVMBool isInMain;

    CVMID_icellSameObject(ee, mainLVM, thisLVM, isInMain);

    return isInMain;
}

#ifdef CVM_DEBUG

void CVMLVMtraceAttachThread(CVMExecEnv* ee)
{
    CVMtraceLVM(("LVM[%d]: Thread attached: %I\n",
	CVMLVMeeGetContext(ee)->id, CVMcurrentThreadICell(ee)));
}

void CVMLVMtraceDetachThread(CVMExecEnv* ee)
{
    CVMtraceLVM(("LVM[%d]: Detaching thread: %I\n",
	CVMLVMeeGetContext(ee)->id, CVMcurrentThreadICell(ee)));
}

void CVMLVMtraceNewThread(CVMExecEnv* ee)
{
    CVMtraceLVM(("LVM[%d]: New thread: %I\n",
	CVMLVMeeGetContext(ee)->id, CVMcurrentThreadICell(ee)));
}

void CVMLVMtraceTerminateThread(CVMExecEnv* ee)
{
    CVMtraceLVM(("LVM[%d]: Terminating thread: %I\n",
	CVMLVMeeGetContext(ee)->id, CVMcurrentThreadICell(ee)));
}

#endif /* CVM_DEBUG */
