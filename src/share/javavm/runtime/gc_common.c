/*
 * @(#)gc_common.c	1.163 06/10/10
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
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/stackwalk.h"
#include "javavm/include/utils.h"
#include "javavm/include/string.h"
#include "javavm/include/sync.h"

#include "javavm/include/localroots.h"
#include "javavm/include/globalroots.h"

#include "javavm/include/preloader.h"

#include "javavm/include/gc_common.h"
#include "javavm/include/gc/gc_impl.h"

#include "javavm/include/gc_stat.h"

#include "javavm/include/globals.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/stackmaps.h"

#ifdef CVM_INSPECTOR
#include "javavm/include/inspector.h"
#endif
#ifdef CVM_JIT
#include "javavm/include/jit/jitcodebuffer.h"
#endif
#ifdef CVM_JVMTI
#include "javavm/include/jvmtiDumper.h"
#include "javavm/include/jvmtiExport.h"
#endif
#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

/* #define CVM_TEST_FAILED_ALLOCATION_GC to simulate an allocation failure
   every CVM_NUM_ALLOCATIONS_PER_GC allocations and trigger GCs.  This is
   only for test purposes. */
#undef CVM_TEST_FAILED_ALLOCATION_GC
#ifdef CVM_TEST_FAILED_ALLOCATION_GC
static int failedAllocGCCount = 0;
#define CVM_NUM_ALLOCATIONS_PER_GC  	10
#endif

#ifdef CVM_JVMPI
/* The following is a list of arena IDs for shared (i.e. not GC implementation
   specific GC heaps: 
   NOTE: CVM_GC_ARENA_UNKNOWN is defined in gc_common.h. */
enum {
    RESERVED_FOR__CVM_GC_ARENA_UNKNOWN        = 0,
    CVM_GC_ARENA_PRELOADED, /* Arena for preloaded objects. */

    /* CVM_GC_TOTAL_SHARED_ARENA_IDS must always be the last enum in the
       list: */
    CVM_GC_TOTAL_SHARED_ARENA_IDS
};
#endif

/*
 * Parse the heap size from the -Xms command line option.
 * 
 */
static CVMBool
parseOption(const char* sizeOption, CVMUint32* sizeRet, CVMUint32 defaultSize)
{
    if (sizeOption != NULL) {
	CVMInt32 tmpVal;
	
	tmpVal = CVMoptionToInt32(sizeOption);
	if (tmpVal < 0) {
	    return CVM_FALSE;
	}
	*sizeRet = (CVMUint32) tmpVal;
    } else {
	*sizeRet = defaultSize;
    }
    return CVM_TRUE;
}

static void
handleGcStats()
{
    const char* statAttr;
    
    /* Are we doing GC statistics? */
    statAttr = CVMgetParsedSubOption(&CVMglobals.gcCommon.gcOptions, "stat");

    if (statAttr != NULL) {
	CVMgcstatDoGCMeasurement(CVM_TRUE);
    }
}

static void
handleSmLimits()
{
    const char* maxSmMemAttr;

    /* Max stackmaps memory usage */
    maxSmMemAttr = CVMgetParsedSubOption(&CVMglobals.gcCommon.gcOptions,
                                         "maxStackMapsMemorySize");
    if (maxSmMemAttr == NULL) {
        CVMglobals.gcCommon.maxStackMapsMemorySize = 0xFFFFFFFF;
    } else {
        CVMglobals.gcCommon.maxStackMapsMemorySize = 
            (CVMUint32)CVMoptionToInt32(maxSmMemAttr);
    }
}

#ifdef CVM_MTASK
CVMBool
CVMgcParseXgcOptions(CVMExecEnv* ee, const char* xgcOpts)
{
    if (!CVMinitParsedSubOptions(&CVMglobals.gcCommon.gcOptions, xgcOpts)) {
	CVMconsolePrintf("Error parsing GC attributes\n");
	return CVM_FALSE;
    }
    /* Allow overrides of the following */
    handleGcStats();
    handleSmLimits();
    /* Ignore any settings of heap size or young generation size */
    return CVM_TRUE;
}
#endif
/*
 * Initialize a heap of at least 'minBytes' bytes, and at most 'maxBytes'
 * bytes.
 */
CVMBool 
CVMgcInitHeap(CVMOptions *options)
{
    CVMUint32 startHeapSize, minHeapSize, maxHeapSize;

    if (!parseOption(options->startHeapSizeStr, &startHeapSize,
		     CVM_DEFAULT_START_HEAP_SIZE_IN_BYTES)) {
	CVMconsolePrintf("Cannot start VM "
			 "(error parsing heap size command "
			 "line option -Xms)\n");
	return CVM_FALSE;
    }

    if (!parseOption(options->minHeapSizeStr, &minHeapSize,
		     CVM_DEFAULT_MIN_HEAP_SIZE_IN_BYTES)) {
	CVMconsolePrintf("Cannot start VM "
			 "(error parsing heap size command "
			 "line option -Xmn)\n");
	return CVM_FALSE;
    }

    if (!parseOption(options->maxHeapSizeStr, &maxHeapSize,
		     CVM_DEFAULT_MAX_HEAP_SIZE_IN_BYTES)) {
	CVMconsolePrintf("Cannot start VM "
			 "(error parsing heap size command "
			 "line option -Xmx)\n");
	return CVM_FALSE;
    }

    if (!CVMinitParsedSubOptions(&CVMglobals.gcCommon.gcOptions,
				 options->gcAttributesStr))
    {
	CVMconsolePrintf("Cannot start VM "
			 "(error parsing GC attributes "
			 "command-line option)\n");
	return CVM_FALSE;
    }

    /*
     * This is the place to parse shared attributes, if any. Currently
     * there aren't any that I can think of, so the particular GC 
     * implementation is the only client of GC attributes now.
     *
     * CVMgcParseSharedGCAttributes();
     */

    /*    CVMconsolePrintf("CVMgcInitHeap: heap size = %d, ", heapSize); */

    handleGcStats();
    handleSmLimits();
    
#ifdef CVM_JVMPI
    if (CVMjvmpiEventArenaNewIsEnabled()) {
        CVMjvmpiPostArenaNewEvent(CVM_GC_ARENA_PRELOADED, "Preloaded");
    }
#endif

    /* Let's round these up to nice double-word multiples, before
       passing them on */
    /*
     * CVMalignDoubleWordUp returns a CVMAddr.
     * Just cast it to CVMUint32
     */
    startHeapSize = (CVMUint32)CVMalignDoubleWordUp(startHeapSize);
    minHeapSize = (CVMUint32)CVMalignDoubleWordUp(minHeapSize);
    maxHeapSize = (CVMUint32)CVMalignDoubleWordUp(maxHeapSize);
    if (minHeapSize > maxHeapSize) {
	CVMdebugPrintf(("WARNING: Minimum heap size %d is larger than "
			"maximum size %d, setting both to %d\n",
			minHeapSize, maxHeapSize, minHeapSize));
	maxHeapSize = minHeapSize;
    }
    if (minHeapSize > startHeapSize) {
	CVMdebugPrintf(("WARNING: Minimum heap size %d is larger than "
			"start size %d, setting both to %d\n",
			minHeapSize, startHeapSize, minHeapSize));
	startHeapSize = minHeapSize;
    }
    if (startHeapSize > maxHeapSize) {
	CVMdebugPrintf(("WARNING: Start heap size %d is larger than "
			"maximum size %d, setting both to %d\n",
			startHeapSize, maxHeapSize, startHeapSize));
	maxHeapSize = startHeapSize;
    }

   /* stash away the max heap size */
    CVMglobals.maxHeapSize = maxHeapSize;

    return CVMgcimplInitHeap(&CVMglobals.gc, startHeapSize,
			     minHeapSize, maxHeapSize,
			     (options->startHeapSizeStr == NULL),
			     (options->minHeapSizeStr == NULL),
			     (options->maxHeapSizeStr == NULL));
}

/*
 * Get GC attribute named 'attrName'.
 * If 'attrName' is of the form 'attrName=val', return "val". 
 * If 'attrName' is a flag instead, just return attrName.
 * Return NULL if not found.
 */
char *
CVMgcGetGCAttributeVal(char* attrName)
{
    return (char *)CVMgetParsedSubOption(&CVMglobals.gcCommon.gcOptions,
                                         attrName);
}

#undef CVM_FASTALLOC_STATS
#ifdef CVM_FASTALLOC_STATS
CVMUint32 fastLockCount = 0;
CVMUint32 slowLockCount = 0;
CVMUint32 verySlowLockCount = 0;
#endif

/*
 * The fast, GC-unsafe locking guarding allocations.
 * If atomic swap operations are supported, this lock is a simple
 * integer.
 * If not, the non-blocking 'tryLock' is used on the 'heapLock'.
 */

static CVMBool
CVMgcPrivateLockHeapUnsafe(CVMExecEnv* ee)
{
#ifdef CVM_ADV_ATOMIC_SWAP
    return CVMatomicSwap(&CVMglobals.fastHeapLock, 1) == 0;
#else
    return CVMsysMutexTryLock(ee, &CVMglobals.heapLock);
#endif
}

static void
CVMgcPrivateUnlockHeap(CVMExecEnv* ee)
{
#ifdef CVM_ADV_ATOMIC_SWAP
    CVMatomicSwap(&CVMglobals.fastHeapLock, 0);
#else
    CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
#endif
}

/*
 * WARNING: GC-unsafe allocator. Use CVMID_allocNewInstance() in GC-safe
 * code.
 */
static CVMObject*
doNewInstance(CVMExecEnv* ee, CVMClassBlock* cb,
	      CVMObject* (*allocateInstance)(CVMExecEnv* ee, 
					     CVMUint32 numBytes));

/*
 * Allocate heap array of length 'len'. The array type is 'arrayCb'.
 */
static CVMArrayOfAnyType*
doNewArray(CVMExecEnv* ee, CVMJavaInt arrayObjectSize,
	   CVMClassBlock* arrayCb, CVMJavaInt arrayLen,
	   CVMObject* (*allocateInstance)(CVMExecEnv* ee, 
					  CVMUint32 numBytes));

/*
 * Handle finalizable allocations. Return a direct reference to this
 * object, since it might have changed due to GC.
 */
static CVMObject*
handleFinalizableAllocation(CVMExecEnv* ee, CVMObject* newInstance)
{
    CVMObjectICell* theCell = CVMfinalizerRegisterICell(ee);
    /*
     * We should call java.lang.ref.Finalizer.register().
     * We are going to go GC-safe here, so we'd better stash
     * the new object in a safe place in the interim. The caller
     * has not stored it in a GC-scannable place yet.
     *
     * NOTE: The following assertion is necessary to prevent a
     * case of a finalizable allocation in Finalizer.register().
     * If there is such a case, then we are going to re-enter this
     * function, and we will want to overwrite theCell which is
     * big trouble.
     */
    CVMassert(CVMID_icellIsNull(theCell));
    CVMID_icellSetDirect(ee, theCell, newInstance); /* safe! */
    CVMD_gcSafeExec(ee, {
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	jobject clazz =
	    CVMcbJavaInstance(CVMsystemClass(java_lang_ref_Finalizer));
	CVMMethodBlock* mb =
	    CVMglobals.java_lang_ref_Finalizer_register;
	/*
	 * We need to push a local frame here so we can make
	 * JNI calls at all points that instances may be allocated.
	 */
	if ((*env)->PushLocalFrame(env, 4) == JNI_OK) {
	    CVMdisableRemoteExceptions(ee);
	    (*env)->CallStaticVoidMethod(env, clazz, mb, theCell);
	    CVMenableRemoteExceptions(ee);
	    (*env)->PopLocalFrame(env, NULL);
	} else {
	    newInstance = NULL;
	}
	/* PushLocalFrame() or CallStaticVoidMethod() could have caused
	 * an exception, and we don't want to propagate it.
	 */
	CVMclearLocalException(ee);
    });
    /*
     * Now re-load, since the object's address may have changed.
     */
    if (newInstance != NULL) {
	newInstance = CVMID_icellDirect(ee, theCell);
    }
    CVMID_icellSetNull(theCell); /* done! */
    /*
     * check that a new object starts at an aligned address
     */
    CVMassert((CVMAddr)CVMalignAddrUp(newInstance) == (CVMAddr)newInstance);
    return newInstance;
}

/*
 * GC for numBytes, and retry allocation if necessary
 */
static CVMBool
stopTheWorldAndGCWithRetry(CVMExecEnv* ee, CVMUint32 numBytes,
			   void (*retryAfterActionCallback)(CVMExecEnv *ee, 
							    void *data),
			   void* retryData);

typedef struct ObjAllocWrap {
    CVMClassBlock* cb;
} ObjAllocWrap;

/*
 * Retrying allocation for newInstance()
 */
static void
newInstanceRetry(CVMExecEnv* ee, void* data)
{
    ObjAllocWrap* aw = (ObjAllocWrap*)data;
    CVMObjectICell* theCell = CVMallocationRetryICell(ee);
    CVMObject* newObject;
    CVMassert(CVMID_icellIsNull(theCell));
    newObject = doNewInstance(ee, aw->cb, CVMgcimplRetryAllocationAfterGC);
    CVMID_icellSetDirectWithAssertion(CVM_TRUE, theCell, newObject);
}

/*
 * Re-trying a java.lang.Class instance allocation
 */
static void
newClassInstanceRetry(CVMExecEnv* ee, void* data)
{
    ObjAllocWrap* aw = (ObjAllocWrap*)data;
    CVMObjectICell* theCell = CVMallocationRetryICell(ee);
    CVMObject* newObject;
    CVMassert(CVMID_icellIsNull(theCell));
    /*
     * Create a new java.lang.Class, and fill in the back pointer
     */
    newObject = doNewInstance(ee, CVMsystemClass(java_lang_Class), 
			      CVMgcimplRetryAllocationAfterGC);
    /* 
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only guarantees
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * which set/get this member are caught.
     */
    if (newObject != NULL) {
	CVMD_fieldWriteAddr(newObject, 
			    CVMoffsetOfjava_lang_Class_classBlockPointer,
			    (CVMAddr)aw->cb);
    }
    CVMID_icellSetDirectWithAssertion(CVM_TRUE, theCell, newObject);
}

/*
 * WARNING: GC-unsafe allocator. Use CVMID_allocNewInstance() in GC-safe
 * code.
 */
CVMObject*
CVMgcAllocNewInstance(CVMExecEnv* ee, CVMClassBlock* cb)
{
    CVMObject* newInstance;

    CVMassert(CVMcbCheckRuntimeFlag(cb, LINKED));
    CVMassert(cb != CVMsystemClass(java_lang_Class));

#ifdef CVM_TEST_FAILED_ALLOCATION_GC
    /* Pretend to fail allocation to trigger a GC if necessary: */
    failedAllocGCCount = ++failedAllocGCCount % CVM_NUM_ALLOCATIONS_PER_GC;
    if (failedAllocGCCount == 0) {
        newInstance = NULL;
        goto allocFailed;
    }
#endif

    if (CVMgcPrivateLockHeapUnsafe(ee)) {
#ifdef CVM_FASTALLOC_STATS
	slowLockCount++;
#endif
	newInstance = doNewInstance(ee, cb, CVMgcimplAllocObject);
	CVMgcPrivateUnlockHeap(ee);
    } else {
#ifdef CVM_FASTALLOC_STATS
	verySlowLockCount++;
#endif
	newInstance = NULL;
    }

#ifdef CVM_TEST_FAILED_ALLOCATION_GC
allocFailed:
#endif

    if (newInstance == NULL) {
	CVMObjectICell* theCell = CVMallocationRetryICell(ee);
	ObjAllocWrap aw;
	aw.cb = cb;
	stopTheWorldAndGCWithRetry(ee, CVMcbInstanceSize(cb), 
				   newInstanceRetry, &aw);
	newInstance = CVMID_icellDirect(ee, theCell);
	CVMID_icellSetNull(theCell);
    }
    
    if (CVMcbIs(cb, FINALIZABLE) && (newInstance != NULL)) {
	newInstance = handleFinalizableAllocation(ee, newInstance);
	CVMtraceWeakrefs(("WR: Registered a finalizable %C\n", cb));
    }

    /*
     * check that a new object starts at an aligned address
     */
    CVMassert((CVMAddr)CVMalignAddrUp(newInstance) == (CVMAddr)newInstance);
    return newInstance;
}
 
/*
 * WARNING: GC-unsafe allocator. Use CVMID_allocNewInstance() in GC-safe
 * code.
 */
CVMObject*
CVMgcAllocNewClassInstance(CVMExecEnv* ee, CVMClassBlock* cbOfJavaLangClass)
{
    CVMObject* newInstance;

#ifdef CVM_TEST_FAILED_ALLOCATION_GC
    /* Pretend to fail allocation to trigger a GC if necessary: */
    failedAllocGCCount = ++failedAllocGCCount % CVM_NUM_ALLOCATIONS_PER_GC;
    if (failedAllocGCCount == 0) {
        newInstance = NULL;
        goto allocFailed;
    }
#endif

    if (CVMgcPrivateLockHeapUnsafe(ee)) {
#ifdef CVM_FASTALLOC_STATS
	slowLockCount++;
#endif
	newInstance = doNewInstance(ee, CVMsystemClass(java_lang_Class), 
				    CVMgcimplAllocObject);
	CVMgcPrivateUnlockHeap(ee);
    } else {
#ifdef CVM_FASTALLOC_STATS
	verySlowLockCount++;
#endif
	newInstance = NULL;
    }

#ifdef CVM_TEST_FAILED_ALLOCATION_GC
allocFailed:
#endif

    /*
     * If GC-unsafe allocation worked, then no worries. Just fill in 
     */
    /* Fill in the back-pointer */
    if (newInstance != NULL) {
	/* 
	 * Access member variable of type 'int'
	 * and cast it to a native pointer. The java type 'int' only garanties 
	 * 32 bit, but because one slot is used as storage space and
	 * a slot is 64 bit on 64 bit platforms, it is possible 
	 * to store a native pointer without modification of
	 * java source code. This assumes that all places in the C-code
	 * are catched which set/get this member.
	 */
	CVMD_fieldWriteAddr(newInstance, 
			    CVMoffsetOfjava_lang_Class_classBlockPointer,
			    (CVMAddr)cbOfJavaLangClass);
    } else {
	CVMObjectICell* theCell = CVMallocationRetryICell(ee);
	ObjAllocWrap aw;
	aw.cb = cbOfJavaLangClass;
	stopTheWorldAndGCWithRetry(ee, 
            CVMcbInstanceSize(CVMsystemClass(java_lang_Class)), 
            newClassInstanceRetry, &aw);
	newInstance = CVMID_icellDirect(ee, theCell);
	CVMID_icellSetNull(theCell);
    }
    
    CVMassert(newInstance == NULL ||
	      CVMobjectGetClass(newInstance)==CVMsystemClass(java_lang_Class));
    return newInstance;
}
 
typedef struct ArrAllocWrap {
    CVMJavaInt arrayObjectSize;
    CVMClassBlock* arrayCb;
    CVMJavaInt arrayLen;
} ArrAllocWrap;

static void
newArrayRetry(CVMExecEnv* ee, void* data)
{
    ArrAllocWrap* aw = (ArrAllocWrap*)data;
    CVMObjectICell* theCell = CVMallocationRetryICell(ee);
    CVMArrayOfAnyType* newArray;
    CVMassert(CVMID_icellIsNull(theCell));
    newArray = doNewArray(ee, aw->arrayObjectSize, aw->arrayCb, aw->arrayLen,
			  CVMgcimplRetryAllocationAfterGC);
    CVMID_icellSetDirectWithAssertion(CVM_TRUE, theCell, (CVMObject*)newArray);
}

/*
 * Allocate heap array of length 'len'. The array type is 'arrayCb'.
 */
CVMArrayOfAnyType*
CVMgcAllocNewArray(CVMExecEnv* ee, CVMBasicType typeCode,
		   CVMClassBlock* arrayCb, CVMJavaInt arrayLen)
{
    CVMArrayOfAnyType* newArray;
    CVMJavaInt         arrayObjectSize;
    
    CVMassert(CVMbasicTypeSizes[typeCode] != 0);
    /*
     * The size of the heap object to accommodate this array
     */
    /* 
     * Keep the correct alignment for the next object/array.
     */
    arrayObjectSize = CVMalignAddrUp(CVMoffsetof(CVMArrayOfAnyType, elems) +
	CVMbasicTypeSizes[typeCode] * arrayLen);

    newArray = CVMgcAllocNewArrayWithInstanceSize(ee, arrayObjectSize,
						  arrayCb, arrayLen);
    return newArray;
}


/*
 * Allocate heap array of length 'len'. The array type is 'arrayCb'.
 */
CVMArrayOfAnyType*
CVMgcAllocNewArrayWithInstanceSize(CVMExecEnv* ee, CVMJavaInt arrayObjectSize,
				   CVMClassBlock* arrayCb, CVMJavaInt arrayLen)
{
    CVMArrayOfAnyType* newArray;
    CVMassert(CVMcbCheckRuntimeFlag(arrayCb, LINKED));

#ifdef CVM_TEST_FAILED_ALLOCATION_GC
    /* Pretend to fail allocation to trigger a GC if necessary: */
    failedAllocGCCount = ++failedAllocGCCount % CVM_NUM_ALLOCATIONS_PER_GC;
    if (failedAllocGCCount == 0) {
        newArray = NULL;
        goto allocFailed;
    }
#endif

    if (CVMgcPrivateLockHeapUnsafe(ee)) {
#ifdef CVM_FASTALLOC_STATS
	slowLockCount++;
#endif
	newArray = doNewArray(ee, arrayObjectSize, arrayCb, arrayLen,
			      CVMgcimplAllocObject);
	CVMgcPrivateUnlockHeap(ee);
    } else {
#ifdef CVM_FASTALLOC_STATS
	verySlowLockCount++;
#endif
	newArray = NULL;
    }

#ifdef CVM_TEST_FAILED_ALLOCATION_GC
allocFailed:
#endif

    if (newArray == NULL) {
	CVMObjectICell* theCell = CVMallocationRetryICell(ee);
	ArrAllocWrap aw;
	aw.arrayObjectSize = arrayObjectSize;
	aw.arrayCb = arrayCb;
	aw.arrayLen = arrayLen;
	stopTheWorldAndGCWithRetry(ee, arrayObjectSize, 
				   newArrayRetry, &aw);
	newArray = (CVMArrayOfAnyType*)CVMID_icellDirect(ee, theCell);
	CVMID_icellSetNull(theCell);
    }
    
    return newArray;
}

/*
 * WARNING: GC-unsafe allocator. Use CVMID_allocNewInstance() in GC-safe
 * code.
 */
static CVMObject*
doNewInstance(CVMExecEnv* ee, CVMClassBlock* cb,
	      CVMObject* (*allocateInstance)(CVMExecEnv* ee, 
					     CVMUint32 numBytes))
{
    CVMObject* newInstance;

    newInstance = (*allocateInstance)(ee, CVMcbInstanceSize(cb));

    /*
     * Caller responsible for handling failed allocation
     */
    if (newInstance == NULL) {
	return NULL;
    }
    /* Zero out object */

    /* %comment f003 */
    memset((void*)newInstance, 0, CVMcbInstanceSize(cb));

    /* Set header of new object */
    newInstance->hdr.clas = cb; /* sync type 0 */
    CVMobjMonitorSet(newInstance, 0, CVM_LOCKSTATE_UNLOCKED);
    /*
     * Make sure that our notion of what the default header word is
     * matches that created by the above CVMobjMonitorSet
     */
    CVMassert(CVMobjectVariousWord(newInstance) ==
	      CVM_OBJECT_DEFAULT_VARIOUS_WORD);

#ifdef CVM_JVMPI
    /* Notify the profiler of an object allocation: */
    if (CVMjvmpiEventObjectAllocIsEnabled()) {
        /* NOTE: When we get here, we may or may not be GC unsafe.  If we're
                 not GC unsafe, then we must be in a GC safe all condition
                 which precludes GC from running.

                 JVMPI will become GC safe when calling the profiling agent,
                 but GC will be disabled for that duration.  So, the new
                 object will be safe from GC.
        */
        CVMjvmpiPostObjectAllocEvent(ee, newInstance);
    }
#endif /* CVM_JVMPI */

    CVMtraceGcAlloc(("GC_COMMON: Allocated <%C> (size=%d) => 0x%x\n",
		     cb, CVMcbInstanceSize(cb), newInstance));

    /*
     * check that a new object starts at an aligned address
     */
    CVMassert((CVMAddr)CVMalignAddrUp(newInstance) == (CVMAddr)newInstance);

    return newInstance;
}

/*
 * Allocate heap array of length 'len'. The array type is 'arrayCb'.
 */
static CVMArrayOfAnyType*
doNewArray(CVMExecEnv* ee, CVMJavaInt arrayObjectSize,
	   CVMClassBlock* arrayCb, CVMJavaInt arrayLen,
	   CVMObject* (*allocateInstance)(CVMExecEnv* ee, 
					  CVMUint32 numBytes))
{
    CVMArrayOfAnyType* newArray;

    CVMassert(arrayCb != 0);

    if (arrayLen >= (1 << 28)) {
	/* Don't bother. arrayObjectSize would overflow for this if
	   the array type was a long or a double. The chances of such
	   an allocation being satisfied are slim to none anyway. */
	return NULL;
    }
    newArray = (CVMArrayOfAnyType*)(*allocateInstance)(ee, arrayObjectSize);

    /*
     * Caller responsible for handling failed allocation
     */
    if (newArray == NULL) {
	return NULL;
    }
    /* Zero out object */
    memset((void*)newArray, 0, arrayObjectSize);

    /* Set header of new object */
    newArray->hdr.clas = arrayCb; /* sync type 0 */
    CVMobjMonitorSet(newArray, 0, CVM_LOCKSTATE_UNLOCKED);
    /*
     * Make sure that our notion of what the default header word is
     * matches that created by the above CVMobjMonitorSet
     */
    CVMassert(CVMobjectVariousWord(newArray) ==
	      CVM_OBJECT_DEFAULT_VARIOUS_WORD);
    newArray->length   = arrayLen;

#ifdef CVM_JVMPI
    /* Notify the profiler of an object allocation: */
    if (CVMjvmpiEventObjectAllocIsEnabled()) {
        /* NOTE: JVMPI will become GC safe when calling the profiling agent,
                 but GC will be disabled for that duration: */
        CVMjvmpiPostObjectAllocEvent(ee, (CVMObject *) newArray);
    }
#endif /* CVM_JVMPI */

    CVMtraceGcAlloc(("GC_COMMON: Allocated <%C> (size=%d, len=%d) => 0x%x\n",
		     arrayCb, arrayObjectSize, arrayLen, newArray));

    /*
     * check that a new object starts at an aligned address
     */
    CVMassert((CVMAddr)CVMalignAddrUp(newArray) == (CVMAddr)newArray);

    return newArray;
}

#if 0
void
CVMnullFrameScanner(CVMFrame* frame, CVMStackChunk* chunk,
		    CVMRefCallbackFunc callback, void* data)
{
    CVMassert(!CVMframeIsTransition(frame) &&
	      !CVMframeIsJava(frame) &&
	      !CVMframeIsJNI(frame));
    return;
}
#endif

/*
 * The following is private to the root scanner, and is used to cart
 * arguments to CVMclassScanCallback
 */
struct CVMClassScanOptions {
    CVMGCOptions *gcOpts;
    CVMRefCallbackFunc callback;
    void* data;
};

typedef struct CVMClassScanOptions CVMClassScanOptions;

/*
 * This callback is used for keeping system classes live.
 */
static void
CVMclassScanSystemClassCallback(CVMExecEnv* ee, CVMClassBlock* cb, void* opts)
{
    CVMClassScanOptions* options = (CVMClassScanOptions*)opts;
    if (CVMcbClassLoader(cb) == NULL) {
	/* This makes sure a class is not scanned in a cycle */
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
        {
            CVMGCOptions *gcOpts = options->gcOpts;
            if (gcOpts->isProfilingPass) {
                CVMGCProfilingInfo *info = (CVMGCProfilingInfo *)options->data;
                info->u.clazz.cb = cb;
            }
        }
#endif
	CVMscanClassIfNeeded(ee, cb, options->callback, options->data);
    }
}

/*
 * Clear the scanned flag of a classblock
 */
static void
CVMclassClearScannedFlagCallback(CVMExecEnv* ee, CVMClassBlock* cb, void* opts)
{
    CVMtraceGcScan(("GC: Clearing marked bit of class %C (0x%x)\n",
		    cb, cb));
    CVMcbClearGcScanned(cb);
}

/*
 * Process special objects with liveness info from a particular GC
 * implementation. This covers special scans like string intern table,
 * weak references and monitor structures.
 */
void
CVMgcProcessSpecialWithLivenessInfo(CVMExecEnv* ee, CVMGCOptions* gcOpts,
				    CVMRefLivenessQueryFunc isLive,
				    void* isLiveData,
                                    CVMRefCallbackFunc transitiveScanner,
				    void* transitiveScannerData)
{
    CVMgcProcessWeakrefWithLivenessInfo(ee, gcOpts,
        isLive, isLiveData, transitiveScanner, transitiveScannerData);
    CVMgcProcessInternedStringsWithLivenessInfo(ee, gcOpts,
        isLive, isLiveData, transitiveScanner, transitiveScannerData);
    CVMgcProcessSpecialWithLivenessInfoWithoutWeakRefs(ee, gcOpts,
        isLive, isLiveData, transitiveScanner, transitiveScannerData);
}

/* Process weak references with liveness info from a particular GC
 * implementation.
 * isLive - a predicate that returns true if an object is strongly referenced
 * transitiveScanner - a callback that marks an object and all its children
 */
void
CVMgcProcessWeakrefWithLivenessInfo(CVMExecEnv* ee,
    CVMGCOptions* gcOpts, CVMRefLivenessQueryFunc isLive, void* isLiveData,
    CVMRefCallbackFunc transitiveScanner, void* transitiveScannerData)
{
    /*
     * Handle weak reference structures, clearing out the right
     * referents. This can change the liveness of things, so it should
     * come first.
     */
    CVMweakrefProcessNonStrong(ee, isLive, isLiveData,
			       transitiveScanner, transitiveScannerData,
			       gcOpts);

    CVMweakrefFinalizeProcessing(ee, isLive, isLiveData,
			       transitiveScanner, transitiveScannerData,
			       gcOpts);
}

/* Process interned strings with liveness info from a particular GC
 * implementation.
 * isLive - a predicate that returns true if an object is strongly referenced
 * transitiveScanner - a callback that marks an object and all its children
 */
void
CVMgcProcessInternedStringsWithLivenessInfo(CVMExecEnv* ee,
    CVMGCOptions* gcOpts, CVMRefLivenessQueryFunc isLive, void* isLiveData,
    CVMRefCallbackFunc transitiveScanner, void* transitiveScannerData)
{
    CVMinternProcessNonStrong(isLive, isLiveData,
			      transitiveScanner, transitiveScannerData);
    CVMinternFinalizeProcessing(isLive, isLiveData,
				transitiveScanner, transitiveScannerData);
}

/*
 * Process special objects with liveness info from a particular GC
 * implementation. This covers special scans like string intern table,
 * weak references and monitor structures.
 */
void
CVMgcProcessSpecialWithLivenessInfoWithoutWeakRefs(
                                    CVMExecEnv* ee, CVMGCOptions* gcOpts,
				    CVMRefLivenessQueryFunc isLive,
				    void* isLiveData,
                                    CVMRefCallbackFunc transitiveScanner,
				    void* transitiveScannerData)
{
    /*
     * Handle monitor structures
     */
    CVMdeleteUnreferencedMonitors(ee, isLive, isLiveData,
				  transitiveScanner, transitiveScannerData);
    
    /*
     * Purge unused entries for cb iff !CVMcbGcScanned(cb). NOTE: by
     * default, all ROM classes are assumed to be scanned, even though
     * CVMcbGcScanned(cbInRom) == FALSE.
     */
#ifdef CVM_CLASSLOADING
    if (CVMglobals.gcCommon.doClassCleanup) {
        CVMclassDoClassUnloadingPass1(ee, isLive, isLiveData,
            transitiveScanner, transitiveScannerData, gcOpts);
    }
#endif
}

/* Purpose: Attempts to compute stackmaps for all the GC's root scan. */
/* Returns: CVM_TRUE if successful, else returns CVM_FALSE. */
CVMBool
CVMgcEnsureStackmapsForRootScans(CVMExecEnv *ee)
{
    CVM_WALK_ALL_THREADS(ee, currentEE, {
	CVMStack* iStack = &currentEE->interpreterStack;
	CVMstackWalkAllFrames(iStack, {
	    if (CVMframeIsJava(frame)) {
		if (!CVMjavaFrameEnsureStackmaps(ee, currentEE, frame)) {
		    return CVM_FALSE;
		}
	    }
	});
    });
    return CVM_TRUE;
}

/* Purpose: Brings the stackmap memory usage back in line. */
void CVMgcTrimStackmapsMemUsage(CVMExecEnv *ee)
{
    CVMStackMaps *maps = CVMglobals.gcCommon.lastStackMaps;

#undef DUMP_STACKMAPS_MEMORY_USAGE
#ifdef DUMP_STACKMAPS_MEMORY_USAGE
    CVMconsolePrintf("STACKMAPS: Total Memory Used BEFORE GC = %d\n",
                     CVMglobals.stackMapsTotalMemoryUsed);
#endif
    while (CVMglobals.gcCommon.stackMapsTotalMemoryUsed >
                   CVMglobals.gcCommon.maxStackMapsMemorySize) {
        CVMassert(maps != NULL);
        CVMstackmapDestroy(ee, maps);
        maps = CVMglobals.gcCommon.lastStackMaps;
    }
#ifdef DUMP_STACKMAPS_MEMORY_USAGE
    CVMconsolePrintf("STACKMAPS: Total Memory Used AFTER GC = %d\n",
                     CVMglobals.stackMapsTotalMemoryUsed);
#endif
}

/*
 * Start a GC cycle.
 */
void
CVMgcStartGC(CVMExecEnv* ee)
{
}

void
CVMgcClearClassMarks(CVMExecEnv* ee, CVMGCOptions* gcOpts)
{
    /*
     * Clear the scanned bits of all classes. Each class will be scanned
     * by the object scan as necessary. This includes the class
     * global roots.
     *
     * Of course if we are not doing a full GC, we don't have to do this.
     * All classes will be scanned anyway, so the GC_SCANNED bit is
     * irrelevant.
     */
    CVMclassIterateDynamicallyLoadedClasses(
        ee, CVMclassClearScannedFlagCallback, NULL);
}

/*
 * End a GC cycle.
 */
void
CVMgcEndGC(CVMExecEnv* ee)
{
#ifdef CVM_CLASSLOADING
    if (CVMglobals.gcCommon.doClassCleanup) {
        CVMclassDoClassUnloadingPass2(ee);
    }
#endif
#ifdef CVM_JIT
    /* 
     * Do aging of compiled methods so some can be decompiled if necessary.
     */
    CVMJITcodeCacheAgeEntryCountsAndDecompile(ee, 0);
#endif
}

/*
 * Scan and optionally update special objects. This covers special
 * scans like string intern table, weak references and monitor
 * structures.
 */
void
CVMgcScanSpecial(CVMExecEnv* ee, CVMGCOptions* gcOpts,
		 CVMRefCallbackFunc refUpdate, void* refUpdateData)
{
    CVMtraceGcCollect(("GC_COMMON: "
		       "Scanning special objects (full)\n"));
    /*
     * Handle interned strings
     */
    CVMscanInternedStrings(refUpdate, refUpdateData);

    /*
     * Handle monitor structures
     */
    CVMscanMonitors(ee, refUpdate, refUpdateData);

    /*
     * Handle weak reference structures
     */
    CVMweakrefUpdate(ee, refUpdate, refUpdateData, gcOpts);

    /*
     * Update all live class loader roots
     */
    CVMstackScanRoots(ee, &CVMglobals.classLoaderGlobalRoots,
		      refUpdate, refUpdateData, gcOpts);
}

/*
 * Scan the root set of collection
 */
void
CVMgcScanRoots(CVMExecEnv *ee, CVMGCOptions* gcOpts,
	       CVMRefCallbackFunc callback, void* data)
{
    /*
     * Assert that we are holding the right locks before scanning
     * the data structures protected by those locks.
     */
    CVMassert(CVMreentrantMutexIAmOwner(
        ee, CVMsysMutexGetReentrantMutex(&CVMglobals.threadLock)));

    /*
     * Scan global roots
     */
    {
        void *callbackData = data;
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)
        CVMGCProfilingInfo info;
        if (gcOpts->isProfilingPass) {
            info.type = CVMGCRefType_GLOBAL_ROOT;
            info.data = data;
            callbackData = &info;
        }
#endif
        CVMstackScanRoots(ee, &CVMglobals.globalRoots, callback, callbackData,
			  gcOpts);
    }

    /*
     * Do the preloader statics
     */
    {
        void *callbackData = data;
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)
        CVMGCProfilingInfo info;

        if (gcOpts->isProfilingPass) {
            info.type = CVMGCRefType_PRELOADER_STATICS;
            info.data = data;
            callbackData = &info;
        }
#endif

#ifdef CVM_LVM /* %begin lvm */
	/* Each LVM has its own copy of static fields.
	 * Walk throgh each of them. */
	CVMLVMwalkStatics(callback, callbackData);
#else /* %end lvm */
	{
	    CVMAddr  numRefStatics = CVMpreloaderNumberOfRefStatics();
	    CVMAddr* refStatics    = CVMpreloaderGetRefStatics();
	    CVMwalkContiguousRefs(refStatics, numRefStatics, callback,
				  callbackData);
	}
#endif /* %begin,end lvm */

#ifdef CVM_JAVASE_CLASS_HAS_REF_FIELD
        /*
         * Scan ROMized objects.
         * The java.lang.Class in SE 1.5 and later version has non-static
         * reference fields.
         */
        CVMpreloaderScanPreloadedClassObjects(ee, gcOpts,
                                              callback, callbackData);
#endif
    }

    {
	CVMClassScanOptions opt;
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)
        CVMGCProfilingInfo info;
#endif
	/*
	 * Now make sure to keep all system classes live. These cannot
	 * be unloaded.
	 */
        opt.gcOpts = gcOpts;
	opt.callback = callback;
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)
        if (gcOpts->isProfilingPass) {
            info.type = CVMGCRefType_CLASS_STATICS;
            info.data = data;
            opt.data = &info;
        } else {
            opt.data = data;
        }
#else
        opt.data = data;
#endif
	CVMclassIterateDynamicallyLoadedClasses(
            ee, CVMclassScanSystemClassCallback, &opt);
    }

    /*
     * Scan the local and interpreter roots of each thread. The previous
     * class table iterations already cleared the mark bits of classes
     * if they are to be scanned based on reachability.
     */
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)
    {
        void *localRootData = data;
        CVMGCProfilingInfo localRootInfo;
        CVMGCProfilingInfo interpreterStackInfo;
	CVMInterpreterStackData interpreterStackData;
	
	interpreterStackData.callbackData = data;
        if (gcOpts->isProfilingPass) {
            localRootInfo.type = CVMGCRefType_LOCAL_ROOTS;
            localRootInfo.data = data;

            interpreterStackInfo.data = data;

            localRootData = &localRootInfo;
            interpreterStackData.callbackData = &interpreterStackInfo;
        }

        CVM_WALK_ALL_THREADS(ee, currentEE, {
            CVMInt32 frameNumber = 0;
            localRootInfo.u.frame.ee = currentEE;
            CVMstackScanRoots(ee, &currentEE->localRootsStack,
                              callback, localRootData, gcOpts);
	    interpreterStackData.targetEE = currentEE;
	    interpreterStackData.prevFrame = NULL;
            CVMstackWalkAllFrames(&currentEE->interpreterStack, {
                if (gcOpts->isProfilingPass) {
                    CVMGCProfilingInfo *info = &interpreterStackInfo;
                    if (CVMframeIsJava(frame)) {
                        info->type = CVMGCRefType_JAVA_FRAME;
                    } else if (CVMframeIsJNI(frame)) {
                        info->type = CVMGCRefType_JNI_FRAME;
                    } else if (CVMframeIsTransition(frame)) {
                        info->type = CVMGCRefType_TRANSITION_FRAME;
                    } else {
                        info->type = CVMGCRefType_UNKNOWN_STACK_FRAME;
                        CVMassert(CVM_FALSE);
                    }
                    info->u.frame.ee = currentEE;
                    info->u.frame.frameNumber = frameNumber++;
                    info->data = data;
                }
                (*CVMframeScanners[frame->type])(ee, frame, chunk, callback,
			        &interpreterStackData, gcOpts);
		interpreterStackData.prevFrame = frame;
            });
        });
    }
#else
    {
	CVMInterpreterStackData interpreterStackData;
	interpreterStackData.callbackData = data;
	CVM_WALK_ALL_THREADS(ee, currentEE, {
	    CVMstackScanRoots(ee, &currentEE->localRootsStack,
			      callback, data, gcOpts);
	    interpreterStackData.targetEE = currentEE;
	    interpreterStackData.prevFrame = NULL;
	    CVMstackScanRoots0(ee, &currentEE->interpreterStack,
			       callback, &interpreterStackData,
			       gcOpts, {}, {
				   interpreterStackData.prevFrame = frame;
			       });
	});
    }
#endif
}

/* Purpose: Acquire all GC locks, stop all threads, and then call back to do
            action specific work. When the action is done, the world is
            resumed.
   NOTE: If we can't execute the action successfully, return CVM_FALSE.
         Otherwise, return the status of the action.
   NOTE: The heapLock must have already been acquired before calling this
         function.  preActionCallback and postActionCallback may be NULL
         but actionCallback must point to a valid function.
*/

static CVMBool
stopTheWorldAndDoAction(CVMExecEnv *ee, void *data,
                 CVMUint32 (*preActionCallback)(CVMExecEnv *ee, void *data),
                 CVMBool (*actionCallback)(CVMExecEnv *ee, void *data),
                 void (*postActionCallback)(CVMExecEnv *ee, void *data,
                                            CVMBool actionSuccess,
                                            CVMUint32 preActionStatus),
		 void (*retryAfterActionCallback)(CVMExecEnv *ee, void *data),
		 void* retryData);

CVMBool
CVMgcStopTheWorldAndDoAction(CVMExecEnv *ee, void *data,
                 CVMUint32 (*preActionCallback)(CVMExecEnv *ee, void *data),
                 CVMBool (*actionCallback)(CVMExecEnv *ee, void *data),
                 void (*postActionCallback)(CVMExecEnv *ee, void *data,
                                            CVMBool actionSuccess,
                                            CVMUint32 preActionStatus),
		 void (*retryAfterActionCallback)(CVMExecEnv *ee, void *data),
                 void* retryData)
{
    return stopTheWorldAndDoAction(ee, data, preActionCallback, actionCallback,
	       postActionCallback, retryAfterActionCallback, retryData);
}

/* Purpose: Acquire all GC locks, stop all threads, and then call back to do
            action specific work. When the action is done, the world is
            resumed.
   NOTE: If we can't execute the action successfully, return CVM_FALSE.
         Otherwise, return the status of the action.
   NOTE: The heapLock must have already been acquired before calling this
         function.  preActionCallback and postActionCallback may be NULL
         but actionCallback must point to a valid function.
*/
static CVMBool
stopTheWorldAndDoAction(CVMExecEnv *ee, void *data,
                 CVMUint32 (*preActionCallback)(CVMExecEnv *ee, void *data),
                 CVMBool (*actionCallback)(CVMExecEnv *ee, void *data),
                 void (*postActionCallback)(CVMExecEnv *ee, void *data,
                                            CVMBool actionSuccess,
                                            CVMUint32 preActionStatus),
                 void (*retryAfterActionCallback)(CVMExecEnv *ee, void *data),
                 void* retryData)
{
    CVMBool success = CVM_FALSE;
    CVMUint32 preActionStatus = 0;

#ifdef CVM_JIT
    /* The jitLock is needed for BecomeSafeAll() and must be acquired before
       the heapLock: */
    CVMassert(CVMsysMutexOwner(&CVMglobals.jitLock) == ee);
#endif
    CVMassert(CVMsysMutexOwner(&CVMglobals.heapLock) == ee);
    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(actionCallback != NULL);

    /* Get the rest of the GC locks.  The heapLock must have already been
       acquired before calling this function. */
    CVMlocksForGCAcquire(ee);

    /* Roll all threads to GC-safe points. */
    CVMD_gcBecomeSafeAll(ee);

    /* Do callback for the preAction: */
    if (preActionCallback != NULL) {
        preActionStatus = preActionCallback(ee, data);
        if (preActionStatus != 0) {
            goto preActionFailed;
        }
    }

    if (CVMgcEnsureStackmapsForRootScans(ee)) {
        /* Do callback for the Action: */
        success = actionCallback(ee, data);
    }

preActionFailed:
    /* Even if the GC fails, we will want to allow the retry because we may
       get here not due to a shortage of memory and a need to GC, but simply
       due to contention during a fast allocation attempt.  The allocation
       may still succeeded:
    */
    if (retryAfterActionCallback != NULL) {
	retryAfterActionCallback(ee, retryData);
    }
    /* Do callback for the postAction: */
    if (postActionCallback != NULL) {
        postActionCallback(ee, data, success, preActionStatus);
    }

    /* Bring the stackmap memory usage back in line: */
    CVMgcTrimStackmapsMemUsage(ee);

    /* Allow threads to become GC-unsafe again. */
    CVMD_gcAllowUnsafeAll(ee);

    /* Relinquish all GC locks except for the heapLock. */
    CVMlocksForGCRelease(ee);

    return success;
}

/* Purpose: Do pre-GC actions. */
/* NOTE: Returns 0 if successful, else returns error code. */
static CVMUint32
CVMgcStopTheWorldAndGCSafePreAction(CVMExecEnv *ee, void *data)
{
#ifdef CVM_JVMPI
    if (CVMgcLockerIsActive(&CVMjvmpiRec()->gcLocker)) {
        return 1;
    }
#endif
#ifdef CVM_INSPECTOR
    while (CVMgcLockerIsActive(&CVMglobals.inspectorGCLocker)) {
        /* Wait until GC is re-enabled i.e. the GCLocker is deactivated: */
        CVMinspectorGCLockerWait(&CVMglobals.inspectorGCLocker, ee);
    }
#endif

    /* By default, we need to scan for class-unloading activity.  The
       specific GC implementation can choose to override this if
       appropriate: */
    CVMglobals.gcCommon.doClassCleanup = CVM_TRUE;

    CVMgcStartGC(ee);
    return 0;
}

/* Purpose: Calls the implementation specific GC to do GC work. */
static CVMBool CVMgcStopTheWorldAndGCSafeAction(CVMExecEnv *ee, void *data)
{
    /* 
     * The following cast might be problematic in a 64-bit port.
     * Assume that there will be no single allocation > 4GB and do only
     * an intermediate cast to CVMAddr to make a 64-bit compiler happy.
     * This is harmless on a 32-bit platform.
     */
    CVMUint32 numBytes = (CVMUint32)(CVMAddr) data;

    /* Starting point of calculating GC pause time */
    CVMgcstatStartGCMeasurement();

    CVMgcimplDoGC(ee, numBytes);

    /* End point of calculating GC pause time */
    CVMgcstatEndGCMeasurement();    

    return CVM_TRUE;
}

/* Purpose: Do post-GC actions. */
static void
CVMgcStopTheWorldAndGCSafePostAction(CVMExecEnv *ee, void *data,
                                     CVMBool actionSuccess,
                                     CVMUint32 preActionStatus)
{
#ifdef CVM_JVMPI
    if (CVMjvmpiEventGCFinishIsEnabled() && CVMjvmpiGCWasStarted()) {
        CVMUint32 objCount = CVMgcimplGetObjectCount(ee);
        CVMUint32 freeMem = CVMgcimplFreeMemory(ee);
        CVMUint32 totalMem = CVMgcimplTotalMemory(ee);
        CVMjvmpiPostGCFinishEvent(objCount, totalMem - freeMem, totalMem);
    }
    CVMjvmpiResetGCWasStarted();
#endif
#ifdef CVM_JVMTI
    if (CVMjvmtiShouldPostGarbageCollectionFinish()) {
        CVMjvmtiPostGCFinishEvent();
    }
    if (CVMjvmtiIsEnabled()) {
        CVMjvmtiTagRehash();
    }
#endif

    /* After GC is done and before we allow all threads to become unsafe
       again, i.e. here, take this opportunity to scavenge and deflate
       monitors: */
    CVMsyncGCSafeAllMonitorScavenge(ee);

    /* Now that we're done with a GC cycle, reset this runtime flags for
       the next GC cycle.  But only do this if we actually did run the
       GC cycle.  If the GC cycle was not run, don't reset the flags yet. */
    if (actionSuccess && (preActionStatus == 0)) {
        CVMglobals.gcCommon.stringInternedSinceLastGC = CVM_FALSE;
        CVMglobals.gcCommon.classCreatedSinceLastGC = CVM_FALSE;
        CVMglobals.gcCommon.loaderCreatedSinceLastGC = CVM_FALSE;
    }
}

/*
 * Initiate a GC. Acquire all GC locks, stop all threads, and then
 * call back to the particular GC to do the work. When the particular
 * GC is done, resume.
 *
 * If we can't execute the GC action successfully, return CVM_FALSE.
 * Otherwise, return CVM_TRUE.
 */
static CVMBool
stopTheWorldAndGCSafe(CVMExecEnv* ee, CVMUint32 numBytes,
		      void (*retryAfterActionCallback)(CVMExecEnv *ee, 
						       void *data),
		      void* data)
{
    CVMObjectICell* referenceLockCell =
	&CVMglobals.java_lang_ref_Reference_lock->r;
    CVMBool weakrefsInitialized;
    CVMBool gcOccurred;
    CVMassert(CVMD_isgcSafe(ee));

    /* We should not own the jitlock or heaplock already, or we risk
       a deadlock with the referenceLockCell */
#ifdef CVM_JIT
    CVMassert(CVMsysMutexOwner(&CVMglobals.jitLock) != ee);
#endif
    CVMassert(CVMsysMutexOwner(&CVMglobals.heapLock) != ee);

    weakrefsInitialized = !CVMID_icellIsNull(referenceLockCell);
    /*
     * First lock the reference lock.
     * %comment: f004
     */
    if (weakrefsInitialized) {
	CVMBool success;
	CVMD_gcUnsafeExec(ee, {
            /* %comment l002 */
            success = 
		CVMobjectTryLock(ee, CVMID_icellDirect(ee, referenceLockCell));
	    if (!success) {
                success = CVMobjectLock(ee, referenceLockCell);
	    }
	});
	if (!success) {
	    return CVM_FALSE;
	} else {
	    CVMglobals.referenceWorkTODO = CVM_FALSE;
	}
    }

    /* We must grab the jitLock and heapLock after the referenceLockCell */
#ifdef CVM_JIT
    CVMsysMutexLock(ee, &CVMglobals.jitLock);
#endif
    CVMsysMutexLock(ee, &CVMglobals.heapLock);

#ifdef CVM_JVMPI
    /* NOTE: The GCStart event must be sent before we actually force every
       thread to become GC safe.  This is necessary because the GCStart event
       is meant to give the JVMPI agent the opportunity to acquire locks and
       allocate memory buffer.  Hence, it is possible for us to have sent the
       GCStart event and discover after the fact that GC was disabled.  If
       the GCStart event was posted, then we should set a flag indicating
       so that we will have the opportunity to send the corresponding GCFinish
       event later on.  If GC gets disabled after we sent this event, it will
       appear as if we have a GC cycle that does nothing.
    */
    if (!CVMgcLockerIsActive(&CVMjvmpiRec()->gcLocker)) {
        if (CVMjvmpiEventGCStartIsEnabled()) {
            CVMjvmpiPostGCStartEvent();
        }
        CVMjvmpiSetGCWasStarted();
    }
#endif
#ifdef CVM_JVMTI
    /* Ditto above JVMPI comment, except for JVMTI  */
    if (CVMjvmtiShouldPostGarbageCollectionStart()) {
	CVMjvmtiPostGCStartEvent();
    }
#endif

    /* Go stop the world and do GC: */
    /* 
     * The numBytes cast might be problematic in a 64-bit port.
     * Assume that there will be no single allocation > 4GB and do only
     * an intermediate cast to CVMAddr to make a 64-bit compiler happy.
     * This is harmless on a 32-bit platform.
     */
    gcOccurred = stopTheWorldAndDoAction(ee, (void *)(CVMAddr)numBytes,
                        CVMgcStopTheWorldAndGCSafePreAction,
                        CVMgcStopTheWorldAndGCSafeAction,
                        CVMgcStopTheWorldAndGCSafePostAction,
	                retryAfterActionCallback, data);

    /* 
     * Do post-GC actions
     */
    CVMgcEndGC(ee);

    CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
#ifdef CVM_JIT
    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif

    /*
     * Now notify the reference lock waiters, and unlock.
     */
    if (weakrefsInitialized) {
	CVMD_gcUnsafeExec(ee, {
	    /*
	     * Wake the reference handler thread so it enqueues its weak
	     * references in the right ReferenceQueue's.
	     * 
	     * We're still holding the reference lock here, so another thread
	     * could not have changed CVMglobals.referenceWorkTODO from
	     * under us.
	     */
	    if (CVMglobals.referenceWorkTODO) {
		CVMobjectNotifyAll(ee, referenceLockCell);
		CVMglobals.referenceWorkTODO = CVM_FALSE;
	    }
            /* %comment l003 */
            if (!CVMobjectTryUnlock(
                    ee, CVMID_icellDirect(ee, referenceLockCell))) {
                if (!CVMobjectUnlock(ee, referenceLockCell)) {
		    CVMassert(CVM_FALSE); /* should never happen */
		}
            }
	});
    }

    return gcOccurred;
}

static CVMBool
stopTheWorldAndGCWithRetry(CVMExecEnv* ee, CVMUint32 numBytes,
			   void (*retryAfterActionCallback)(CVMExecEnv *ee, 
							    void *data),
			   void* retryData)
{
    CVMBool retVal;
    CVMD_gcSafeExec(ee, {
	retVal = stopTheWorldAndGCSafe(ee, numBytes,
				       retryAfterActionCallback,
				       retryData);
    });
    return retVal;
}


CVMBool
CVMgcStopTheWorldAndGC(CVMExecEnv* ee, CVMUint32 numBytes)
{
    return stopTheWorldAndGCWithRetry(ee, numBytes, NULL, NULL);
}

/* Purpose: Do a synchronous GC cycle. */
void
CVMgcRunGC(CVMExecEnv* ee)
{
    /* Do a full-scale GC */
    (void)stopTheWorldAndGCSafe(ee, ~0, NULL, NULL); 
}

/* Purpose: Do a synchronous GC cycle. */
void
CVMgcRunGCMin(CVMExecEnv* ee)
{
    /* Do a small-scale GC - young-gen only typically */
    (void)stopTheWorldAndGCSafe(ee, 1, NULL, NULL); 
}

/*
 * Return the number of bytes free in the heap. 
 */
CVMJavaLong
CVMgcFreeMemory(CVMExecEnv* ee)
{
    CVMUint32 freeMem = CVMgcimplFreeMemory(ee);
    /* %comment f005 */
    return CVMint2Long(freeMem);
}

/*
 * Return the amount of total memory in the heap, in bytes.
 */
CVMJavaLong
CVMgcTotalMemory(CVMExecEnv* ee)
{
    CVMUint32 totalMem = CVMgcimplTotalMemory(ee);
    /* %comment f005 */
    return CVMint2Long(totalMem);
}

/*
 * Destroy heap
 */
CVMBool 
CVMgcDestroyHeap()
{
    CVMBool result;
    CVMdestroyParsedSubOptions(&CVMglobals.gcCommon.gcOptions);
    result = CVMgcimplDestroyHeap(&CVMglobals.gc);
#ifdef CVM_JVMPI
    if (CVMjvmpiEventArenaDeleteIsEnabled()) {
        CVMjvmpiPostArenaDeleteEvent(CVM_GC_ARENA_PRELOADED);
    }
#endif
    return result;
}

/*================================================== CVMGCLocker mechanism ==*/

#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI)

/* Purpose: Constuctor. */
void CVMgcLockerInit(CVMGCLocker *self)
{
    self->lockCount = 0;
    self->wasContended = CVM_FALSE;
}

/* Purpose: Activates the GC lock. */
/* NOTE: Calls to CVMgcLockerLock() & CVMgcLockerUnlock() can be nested.
         Can be called while GC safe or unsafe. */
void CVMgcLockerLock(CVMGCLocker *self, CVMExecEnv *current_ee)
{
    if (CVMD_isgcSafe(current_ee)) {
        CVMsysMutexLock(current_ee, &CVMglobals.gcLockerLock);
    }
    CVMsysMicroLock(current_ee, CVM_GC_LOCKER_MICROLOCK);
    self->lockCount++;
    CVMsysMicroUnlock(current_ee, CVM_GC_LOCKER_MICROLOCK);
    if (CVMD_isgcSafe(current_ee)) {
        CVMsysMutexUnlock(current_ee, &CVMglobals.gcLockerLock);
    }
}

/* Purpose: Deactivates the GC lock. */
/* NOTE: Calls to CVMgcLockerLock() & CVMgcLockerUnlock() can be nested.
         Can be called while GC safe or unsafe. */
void CVMgcLockerUnlock(CVMGCLocker *self, CVMExecEnv *current_ee)
{
    if (CVMD_isgcSafe(current_ee)) {
        CVMsysMutexLock(current_ee, &CVMglobals.gcLockerLock);
    }
    CVMsysMicroLock(current_ee, CVM_GC_LOCKER_MICROLOCK);
    if (self->lockCount > 0) {
        self->lockCount--;
    }
    CVMsysMicroUnlock(current_ee, CVM_GC_LOCKER_MICROLOCK);
    if (CVMD_isgcSafe(current_ee)) {
        CVMsysMutexUnlock(current_ee, &CVMglobals.gcLockerLock);
    }
}

#endif /* defined(CVM_INSPECTOR) || defined(CVM_JVMPI) || defined(CVM_JVMTI) */

/*===========================================================================*/

/*
 * Scan objects in contiguous range, and do per-object callback in support
 * of heap dump.
 */
/* Returns: CVM_TRUE if exiting due to completion of scan i.e. every object
            in the specified region has been scanned.
            CVM_FALSE if exiting due to an abortion i.e. the callback function
            has returned a CVM_FALSE status indicating a need to abort. */
CVMBool
CVMgcScanObjectRange(CVMExecEnv* ee, CVMUint32* base, CVMUint32* top,
		     CVMObjectCallbackFunc callback, void* callbackData)
{
    CVMUint32* curr = base;

    while (curr < top) {
	CVMObject* currObj = (CVMObject*)curr;
	CVMClassBlock* currCb = CVMobjectGetClass(currObj);
	CVMUint32  objSize    = CVMobjectSizeGivenClass(currObj, currCb);
	CVMBool completeScanDone;

        completeScanDone = (*callback)(currObj, currCb, objSize, callbackData);
        if (!completeScanDone) {
            return CVM_FALSE; /* Abort i.e. complete scan NOT done. */
        }
	/* iterate */
	curr += objSize / 4;
    }
    CVMassert(curr == top); /* This had better be exact */
    return CVM_TRUE; /* Complete scan DONE. */
}

#ifdef CVM_JVMPI

/* Purpose: Posts the JVMPI_EVENT_ARENA_NEW events. */
/* NOTE: This function is necessary to compensate for the fact that
         this event has already transpired by the time that JVMPI is
         initialized. */
void CVMgcPostJVMPIArenaNewEvent(void)
{
    if (CVMjvmpiEventArenaNewIsEnabled()) {
        CVMjvmpiPostArenaNewEvent(CVM_GC_ARENA_PRELOADED, "Preloaded");
        CVMgcimplPostJVMPIArenaNewEvent();
    }
}

/* Purpose: Gets the last shared arena ID that is used by shared code.  GC
            specific implementations should start their arena ID after the last
            shared arena ID returned by this function. */
CVMUint32 CVMgcGetLastSharedArenaID(void)
{
    /* NOTE: See the enum list at the top of this file for a list of shared
       (i.e. not GC implementation specific) arena IDs. */
    return (CVM_GC_TOTAL_SHARED_ARENA_IDS - 1);
}

/* Purpose: Gets the arena ID for the specified object. */
CVMUint32 CVMgcGetArenaID(CVMObject *obj)
{
    /* First check to see if the object is located in the GC specific heap
       implementation: */
    CVMUint32 arenaID = CVMgcimplGetArenaID(obj);
    if (arenaID == CVM_GC_ARENA_UNKNOWN) {
        /* If the object isn't in the GC specific heap implementation, check
           to see if it is a preloaded object: */
        if (CVMpreloaderIsPreloadedObject(obj)) {
            arenaID = CVM_GC_ARENA_PRELOADED;
        }
    }
    return arenaID;
}

#endif /* CVM_JVMPI */

