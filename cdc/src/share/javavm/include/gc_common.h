/*
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
 * This file includes functions common to all GCs. It is the interface
 * between a particular GC implementation and the rest of the VM.
 */

#ifndef _INCLUDED_GC_COMMON_H
#define _INCLUDED_GC_COMMON_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/stacks.h"
#include "javavm/include/gc/gc_impl.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/preloader.h"
#include "javavm/include/weakrefs.h"
#include "javavm/include/utils.h"
#include "generated/offsets/java_lang_Class.h"

/*
 * The default min and max sizes of the heap
 */
#ifdef JAVASE
#ifndef CVM_DEFAULT_MAX_HEAP_SIZE_IN_BYTES
#define CVM_DEFAULT_MAX_HEAP_SIZE_IN_BYTES (32 * 1024 * 1024)
#endif
#ifndef CVM_DEFAULT_MIN_HEAP_SIZE_IN_BYTES
#define CVM_DEFAULT_MIN_HEAP_SIZE_IN_BYTES (4 * 1024 * 1024)
#endif
#ifndef CVM_DEFAULT_START_HEAP_SIZE_IN_BYTES
#define CVM_DEFAULT_START_HEAP_SIZE_IN_BYTES (4 * 1024 * 1024)
#endif
#endif

#ifndef CVM_DEFAULT_MAX_HEAP_SIZE_IN_BYTES
#define CVM_DEFAULT_MAX_HEAP_SIZE_IN_BYTES (5 * 1024 * 1024)
#endif
#ifndef CVM_DEFAULT_MIN_HEAP_SIZE_IN_BYTES
#define CVM_DEFAULT_MIN_HEAP_SIZE_IN_BYTES (2 * 1024 * 1024)
#endif
#ifndef CVM_DEFAULT_START_HEAP_SIZE_IN_BYTES
#define CVM_DEFAULT_START_HEAP_SIZE_IN_BYTES (2 * 1024 * 1024)
#endif

typedef struct CVMGCCommonGlobalState CVMGCCommonGlobalState;
struct CVMGCCommonGlobalState {
    /*
     * GC attributes, to be parsed by the GC implementation
     */
    CVMParsedSubOptions gcOptions;
    CVMUint32 maxStackMapsMemorySize;

    /* Runtime state information to optimize GC scans: */
    CVMBool doClassCleanup;
    CVMBool stringInternedSinceLastGC;
    CVMBool classCreatedSinceLastGC;
    CVMBool loaderCreatedSinceLastGC;

    /* Cached stackMaps */
    CVMStackMaps *firstStackMaps;
    CVMStackMaps *lastStackMaps;
    CVMUint32 stackMapsTotalMemoryUsed;
};

#define CVM_GC_SHARED_OPTIONS "[maxStackMapsMemorySize=<size>][,stat=true]"
#ifdef CVM_GCIMPL_GC_OPTIONS
#define CVM_GC_OPTIONS "[-Xgc:"\
			CVM_GC_SHARED_OPTIONS\
			CVM_GCIMPL_GC_OPTIONS\
			"] "
#else
#define CVM_GC_OPTIONS "[-Xgc:"\
			CVM_GC_SHARED_OPTIONS\
			"] "
#endif

/*
 * A structure indicating the current GC mode. This is passed around
 * the root scan phase.
 */
struct CVMGCOptions {
    CVMBool isUpdatingObjectPointers;
    CVMBool discoverWeakReferences;
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
    CVMBool isProfilingPass;
#endif
};

#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI) || defined(CVM_JVMTI)

/* CVM_GC_ARENA_UNKNOWN is reserved for error conditions where the arenaID
   of an object is unknown.  A valid arenaID should never have the same value
   as CVM_GC_ARENA_UNKNOWN (i.e. should never be 0): */
#define CVM_GC_ARENA_UNKNOWN 0

/* The following enumerates the type of GC roots that are found during a GC
   scan:
   NOTE: CVMGCRefType_INVALID is reserved for error cases.  It does not
         correspond to any real reference types.
*/
enum CVMGCRefType {
    CVMGCRefType_INVALID          = 0,
    CVMGCRefType_GLOBAL_ROOT,
    CVMGCRefType_PRELOADER_STATICS,
    CVMGCRefType_CLASS_STATICS,
    CVMGCRefType_LOCAL_ROOTS,
    CVMGCRefType_UNKNOWN_STACK_FRAME,
    CVMGCRefType_JAVA_FRAME,
    CVMGCRefType_JNI_FRAME,
    CVMGCRefType_TRANSITION_FRAME,
    CVMGCRefType_OBJECT_FIELD
};
typedef enum CVMGCRefType CVMGCRefType;

/* The following structure is used to store profiling information to be passed
   as the data argument to a callback function during a GC root scan if the
   isProfilingPass flag is set to CVM_TRUE in the CVMGCOptions: */
typedef struct CVMGCProfilingInfo CVMGCProfilingInfo;
struct CVMGCProfilingInfo {
    CVMGCRefType type;
    void *data;
    union {
        struct {
            CVMClassBlock *cb;
        } clazz;
        struct {
            CVMExecEnv *ee;
            CVMInt32 frameNumber;
        } frame;
    } u;
};

/*================================================== CVMGCLocker mechanism ==*/

/* Class CVMGCLocker used for disabling GC Collection and Compaction cycles
    for debugging or profiling purposes: */

typedef struct CVMGCLocker CVMGCLocker;
struct CVMGCLocker
{
    volatile CVMUint32 lockCount;
    volatile CVMBool wasContended;
};

/* Purpose: Constuctor. */
void CVMgcLockerInit(CVMGCLocker *self);

/* Purpose: Destructor. */
#define CVMgcLockerDestroy(/* CVMGCLocker * */self) ;

/* Purpose: Indicates if the GC lock is activated. */
#define CVMgcLockerIsActive(/* CVMGCLocker * */self) \
    ((self)->lockCount > 0)

/* Purpose: Gets the lockCount of the GC lock. */
#define CVMgcLockerGetLockCount(/* CVMGCLocker * */self) \
    ((self)->lockCount)

/* Purpose: Activates the GC lock. */
/* NOTE: Calls to CVMgcLockerLock() & CVMgcLockerUnlock() can be nested. */
void CVMgcLockerLock(CVMGCLocker *self, CVMExecEnv *current_ee);

/* Purpose: Deactivates the GC lock. */
/* NOTE: Calls to CVMgcLockerLock() & CVMgcLockerUnlock() can be nested. */
void CVMgcLockerUnlock(CVMGCLocker *self, CVMExecEnv *current_ee);

#endif

/*===========================================================================*/

/* Purpose: Checks to see if the specified thread is running the GC. */
#define CVMgcIsGCThread(/* CVMExecEnv * */ ee) \
    (CVM_CSTATE(CVM_GC_SAFE)->requester == ee)

#ifdef CVM_MTASK
extern CVMBool
CVMgcParseXgcOptions(CVMExecEnv* ee, const char* xgcOpts);
#endif

/*
 * Extracts 'minBytes' and 'maxBytes' from options, then
 * initializes a heap of at least 'minBytes' bytes, and at most 'maxBytes'
 * bytes.
 */
extern CVMBool 
CVMgcInitHeap(CVMOptions *options);

/*
 * Get GC attribute named 'attrName'.
 * Return NULL if not found.
 */
extern char *
CVMgcGetGCAttributeVal(char* attrName);

/*
 * Allocate heap object of type 'cb'.
 */
extern CVMObject*
CVMgcAllocNewInstance(CVMExecEnv* ee, CVMClassBlock* cb);

/*
 * Allocate java.lang.Class instance corresponding to 'cbOfJavaLangClass'
 */
extern CVMObject*
CVMgcAllocNewClassInstance(CVMExecEnv* ee, CVMClassBlock* cbOfJavaLangClass);
 
/*
 * Allocate heap array of length 'len'. The type of the array is arrayCb.
 * The elements are basic type 'typeCode'.
 */
extern CVMArrayOfAnyType*
CVMgcAllocNewArray(CVMExecEnv* ee, CVMBasicType typeCode,
		   CVMClassBlock* arrayCb, CVMJavaInt len);

/*
 * Allocate heap array of length 'len'. The type of the array is arrayCb.
 * The total size of the array is 'instanceSize'.
 */
extern CVMArrayOfAnyType*
CVMgcAllocNewArrayWithInstanceSize(CVMExecEnv* ee, CVMJavaInt instanceSize,
				   CVMClassBlock* arrayCb, CVMJavaInt len);

/*
 * Return the number of bytes free in the heap
 */
extern CVMJavaLong
CVMgcFreeMemory(CVMExecEnv* ee);

/*
 * Return the amount of total memory in the heap, in bytes
 */
extern CVMJavaLong
CVMgcTotalMemory(CVMExecEnv* ee);

/*
 * Constants related to object reference map formats
 */
#define CVM_GCMAP_NUM_FLAGS     2    /* the no. of flags bits in the map */
#define CVM_GCMAP_NUM_WEAKREF_FIELDS  2 /* the no. of special fields in
					  java.lang.ref.Reference */
#define CVM_GCMAP_FLAGS_MASK      ((1 << CVM_GCMAP_NUM_FLAGS) - 1)

#define CVM_GCMAP_NOREFS          0x0  /* No references in this object */

#define CVM_GCMAP_SHORTGCMAP_FLAG 0x0  /* 0 - object with inline GC map */
#define CVM_GCMAP_ALLREFS_FLAG    0x1  /* 1 - array of all references */
#define CVM_GCMAP_LONGGCMAP_FLAG  0x2  /* 2 - object with long GC map */

/*
 * Walk refs of heap object 'obj' with first header word
 * 'firstHeaderWord', and perform refAction on each discovered object
 * reference field. Also scan class of object if it has not been scanned
 * yet.
 *
 * Within the body of refAction, CVMObject** refPtr refers to the
 * address of a reference typed slot (an array element, or an object
 * field).  
 *
 * CVMscanClassIfNeeded(CVMExecEnv* ee, CVMClassBlock* cb,
 *                      CVMRefCallbackFunc callback, void* data);
 */
#define CVMscanClassIfNeededConditional(ee, cb, cond, callback, data)	    \
    if (!CVMcbIsInROM(cb) && !CVMcbGcScanned(cb)) {			    \
        if (cond) {							    \
            CVMclassScan(ee, cb, callback, data);    	    		    \
	    CVMcbSetGcScanned(cb);					    \
        }								    \
    }

#define CVMscanClassIfNeeded(ee, cb, callback, data)			    \
    CVMscanClassIfNeededConditional(ee, cb, CVM_TRUE, callback, data)

#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
#define CVMscanClassWithGCOptsIfNeeded(ee_, cb_, gcOpts_, callback_, data_) \
    if (!CVMcbIsInROM(cb_) && !CVMcbGcScanned(cb_)) {                       \
        CVMGCProfilingInfo info_;                                           \
        void *callbackData_ = data_;                                        \
        if (gcOpts_->isProfilingPass) {                                     \
            info_.type = CVMGCRefType_CLASS_STATICS;                        \
            info_.data = data_;                                             \
            info_.u.clazz.cb = cb_;                                         \
            callbackData_ = &info_;                                         \
        }                                                                   \
        CVMclassScan(ee_, cb_, callback_, callbackData_);                   \
        CVMcbSetGcScanned(cb_);                                             \
    }
#else
#define CVMscanClassWithGCOptsIfNeeded(ee, cb, gcOpts, callback, data)      \
    CVMscanClassIfNeeded(ee, cb, callback, data)
#endif

/* 
 * make CVM ready to run on 64 bit platforms
 * 
 *  CVMobjectWalkRefsWithSpecialHandling()
 *
 * - firstHeaderWord_ is CVMClassBlock*
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 *
 */
#define CVMobjectWalkRefsWithSpecialHandling(ee, gcOpts, obj,		    \
					     firstHeaderWord_, refAction,   \
					     callback, data)		    \
{									    \
    CVMClassBlock* cb_ = (CVMClassBlock*)				    \
        (((CVMAddr)(firstHeaderWord_)) & CVM_OBJECT_CLASS_MASK);	    \
    CVMscanClassWithGCOptsIfNeeded(ee, cb_, gcOpts, callback, data);        \
    /*									    \
     * If we are scanning a java.lang.Class instance, scan its 		    \
     * corresponding CVMClassBlock.					    \
     */									    \
    if (cb_ == CVMsystemClass(java_lang_Class)) {                           \
	CVMClassBlock* classBlockPtr = 					    \
            *((CVMClassBlock**)(obj) + 					    \
	      CVMoffsetOfjava_lang_Class_classBlockPointer);		    \
        CVMscanClassWithGCOptsIfNeeded(ee, classBlockPtr, gcOpts,           \
                                       callback, data);                     \
    }                                                                       \
    CVMobjectWalkRefsAux(ee, gcOpts, obj, firstHeaderWord_,		    \
			 CVM_TRUE, refAction);   	    		    \
}

#define CVMobjectWalkRefs(ee, gcOpts, obj, firstHeaderWord_, refAction)	    \
    CVMobjectWalkRefsAux(ee, gcOpts, obj, firstHeaderWord_, CVM_FALSE, refAction)

/*
 * Walk refs of heap object 'obj' with first header word
 * 'firstHeaderWord', and perform refAction on each discovered object
 * reference field.
 *
 * Within the body of refAction, CVMObject** refPtr refers to the
 * address of a reference typed slot (an array element, or an object
 * field).  
 */
/* 
 * make CVM ready to run on 64 bit platforms
 * 
 * CVMobjectWalkRefsAux()
 *
 * - firstHeaderWord_ is CVMClassBlock*
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 *
 */
#define CVMobjectWalkRefsAux(ee, gcOpts, obj, firstHeaderWord_,		  \
			     handleWeakReferences, refAction)		  \
{									  \
    CVMClassBlock* cb_ = (CVMClassBlock*)				  \
        (((CVMAddr)(firstHeaderWord_)) & CVM_OBJECT_CLASS_MASK);	  \
    /* 
     * Make 'map' the same type as 'map' in 'CVMGCBitMap'.		  \
     */									  \
    CVMAddr        map = CVMcbGcMap(cb_).map;				  \
    CVMObject**    refPtr;						  \
									  \
    if (map != CVM_GCMAP_NOREFS) {					  \
        /*
	 * Make 'flags' the same type as 'map' in 'CVMGCBitMap'.	  \
         */								  \
	CVMAddr flags = map & CVM_GCMAP_FLAGS_MASK;			  \
	refPtr = (CVMObject**)&(obj)->fields[0];		  	  \
									  \
        if (flags == CVM_GCMAP_SHORTGCMAP_FLAG) {			  \
            /* object with inline GC map: the most common case */  	  \
            								  \
            /* First skip the flags */				  	  \
            map >>= CVM_GCMAP_NUM_FLAGS;			  	  \
            								  \
            if (handleWeakReferences && 				  \
		(gcOpts)->discoverWeakReferences) { 			  \
                if (CVMcbIs(cb_, REFERENCE)) {				  \
                    /* Skip referent, next in case we just discovered	  \
		       a new active weak reference object. */		  \
                    if (CVMweakrefField(obj, next) == NULL) {		  \
			CVMweakrefDiscover(ee, obj);			  \
		        map >>= CVM_GCMAP_NUM_WEAKREF_FIELDS;	  	  \
                        refPtr += CVM_GCMAP_NUM_WEAKREF_FIELDS;		  \
                    }							  \
		}							  \
	    }								  \
            /* Now get through all the references in the object */	  \
            while (map != 0) {					  	  \
                if ((map & 0x1) != 0) {				  	  \
                    refAction;					  	  \
                }							  \
		map >>= 1;						  \
		refPtr++;						  \
            }							  	  \
        } else if (flags == CVM_GCMAP_ALLREFS_FLAG) {			  \
	    /* an array of all refs. Scan each and every one. */	  \
	    CVMArrayOfRef* arrRefs = (CVMArrayOfRef*)(obj);		  \
	    CVMJavaInt     arrLen  = CVMD_arrayGetLength(arrRefs);	  \
									  \
	    /*								  \
	     * Address of the first element of the array		  \
             * (skip the length field)					  \
	     */								  \
	    /* 
	     * refPtr += 1 would work on 32-bit platforms.
	     * But refPtr +1 does not fit on 64 bit platforms because the     \
	     * length field is an int (4 byte) and a native pointer is    \
	     * 8 byte on 64 bit platforms                                 \
	     * Therefore get the start address of the elems array         \
	     * instead of doing address calculations.                     \
	     */                                                           \
	    refPtr = (CVMObject**) &arrRefs->elems[0];			  \
	    while (arrLen-- > 0) {					  \
		refAction;						  \
		refPtr++;						  \
	    }								  \
	} else {							  \
            /* object with "big" GC map */			  	  \
	    /* 								  \
	     * CVMGCBitMap is a union of the scalar 'map' and the	  \
	     * pointer 'bigmap'. So it's best to                	  \
	     * use the pointer if we need the pointer             	  \
             */								  \
            CVMBigGCBitMap* bigmap = (CVMBigGCBitMap*)			  \
				     ((CVMAddr)(CVMcbGcMap(cb_).bigmap)	  \
				      & ~CVM_GCMAP_FLAGS_MASK);		  \
            CVMUint32       mapLen = bigmap->maplen;		  	  \
            /*                                          		  \
	     * Make 'mapPtr' and 'mapEnd' pointers to the type of 'map'	  \
	     * in 'CVMBigGCBitMap'.					  \
	     */								  \
            CVMAddr*        mapPtr = bigmap->map;			  \
            CVMAddr*        mapEnd = mapPtr + mapLen;		  	  \
            CVMObject**     otherRefPtr = refPtr;			  \
            								  \
            CVMassert(flags == CVM_GCMAP_LONGGCMAP_FLAG);		  \
            /* Make sure there is something to do */			  \
            CVMassert(mapPtr < mapEnd);					  \
	    map = *mapPtr;				  	  	  \
            if (handleWeakReferences &&					  \
		(gcOpts)->discoverWeakReferences) { 			  \
                if (CVMcbIs(cb_, REFERENCE)) {				  \
                    /* Skip referent, next in case we just discovered	  \
		       a new active weak reference object. */		  \
                    if (CVMweakrefField(obj, next) == NULL) {		  \
			CVMweakrefDiscover(ee, obj);			  \
		        map >>= CVM_GCMAP_NUM_WEAKREF_FIELDS;	  	  \
                        refPtr += CVM_GCMAP_NUM_WEAKREF_FIELDS;		  \
                    }							  \
		}							  \
	    }								  \
            while (mapPtr < mapEnd) {				  	  \
		while (map != 0) {					  \
		    if ((map & 0x1) != 0) {				  \
			refAction;					  \
		    }						  	  \
		    map >>= 1;					  	  \
		    refPtr++;					  	  \
		}							  \
		mapPtr++;						  \
                /* This may be a redundant read that may fall off the	  \
		   edge of the world. To prevent this case, 		  \
		   we've left an extra	  				  \
		   safety word in the big map. */		 	  \
                map = *mapPtr;						  \
		/* Advance the object pointer to the next group of 32	  \
		 * fields, in case map becomes 0 before we iterate	  \
		 * through all the fields in group *mapPtr		  \
                 */		  					  \
		otherRefPtr += 32;					  \
		refPtr = otherRefPtr;				  	  \
            }							  	  \
	}								  \
    }									  \
}

/*
 * Initiate a GC. Acquire all GC locks, stop all threads, and then
 * call back to the particular GC to do the work. When the particular
 * GC is done, resume.
 *
 * Called with heap locked and while GC-unsafe.
 * return CVM_TRUE on success, CVM_FALSE if GC could not be run.
 */
extern CVMBool
CVMgcStopTheWorldAndGC(CVMExecEnv* ee, CVMUint32 numBytes);

/* Purpose: Synchronizes all threads on a consistent state in preparation
   for a GC or similar cycles. */
extern CVMBool
CVMgcStopTheWorldAndDoAction(CVMExecEnv *ee, void *data,
                 CVMUint32 (*preActionCallback)(CVMExecEnv *ee, void *data),
                 CVMBool (*actionCallback)(CVMExecEnv *ee, void *data),
                 void (*postActionCallback)(CVMExecEnv *ee, void *data,
                                            CVMBool actionSuccess,
                                            CVMUint32 preActionStatus),
		 void (*retryAfterActionCallback)(CVMExecEnv *ee, void *data),
		 void* retryData);

/*
 * Start a GC cycle. Will clear class marks if necessary.
 */
extern void
CVMgcStartGC(CVMExecEnv* ee);

/*
 * Clear the class GC_SCANNED marks for dynamically loaded classes.
 */
extern void
CVMgcClearClassMarks(CVMExecEnv* ee, CVMGCOptions* gcOpts);

/*
 * End a GC cycle.
 */
extern void
CVMgcEndGC(CVMExecEnv* ee);

/*
 * Like above, but called without heap locked and while GC-safe.
 */
extern void
CVMgcRunGC(CVMExecEnv* ee);

/*
 * Like above, but only trys to free 1 byte.
 */
extern void
CVMgcRunGCMin(CVMExecEnv* ee);

/*
 * The null CVMFrameGCScannerFunc that doesn't do anything
 */
extern void
CVMnullFrameScanner(CVMFrame* frame, CVMStackChunk* chunk,
		    CVMRefCallbackFunc callback, void* data);

/*
 * Process special objects with liveness info from a particular GC
 * implementation. This covers special scans like string intern table,
 * weak references and monitor structures.
 *
 * isLive - a predicate that returns true if an object is strongly referenced
 * transitiveScanner - a callback that marks an object and all its children
 */
extern void
CVMgcProcessSpecialWithLivenessInfo(CVMExecEnv* ee, CVMGCOptions* gcOpts,
				    CVMRefLivenessQueryFunc isLive,
				    void* isLiveData,
                                    CVMRefCallbackFunc transitiveScanner,
				    void* transitiveScannerData);

/* Process weak references with liveness info from a particular GC
 * implementation.
 * isLive - a predicate that returns true if an object is strongly referenced
 * transitiveScanner - a callback that marks an object and all its children
 */
void
CVMgcProcessWeakrefWithLivenessInfo(CVMExecEnv* ee,
    CVMGCOptions* gcOpts, CVMRefLivenessQueryFunc isLive, void* isLiveData,
    CVMRefCallbackFunc transitiveScanner, void* transitiveScannerData);

/* Process interned strings with liveness info from a particular GC
 * implementation.
 * isLive - a predicate that returns true if an object is strongly referenced
 * transitiveScanner - a callback that marks an object and all its children
 */
void
CVMgcProcessInternedStringsWithLivenessInfo(CVMExecEnv* ee,
    CVMGCOptions* gcOpts, CVMRefLivenessQueryFunc isLive, void* isLiveData,
    CVMRefCallbackFunc transitiveScanner, void* transitiveScannerData);

/*
 * Process special objects with liveness info from a particular GC
 * implementation. This covers special scans like string intern table,
 * and monitor structures.
 *
 * This purposefully leaves out weak references for the rare GC that
 * wants to treat weak references specially.
 *
 * isLive - a predicate that returns true if an object is strongly referenced
 * transitiveScanner - a callback that marks an object and all its children
 */
extern void
CVMgcProcessSpecialWithLivenessInfoWithoutWeakRefs(
                                    CVMExecEnv* ee, CVMGCOptions* gcOpts,
				    CVMRefLivenessQueryFunc isLive,
				    void* isLiveData,
                                    CVMRefCallbackFunc transitiveScanner,
				    void* transitiveScannerData);

/*
 * Scan and optionally update special objects. This covers special
 * scans like string intern table, weak references and monitor
 * structures.  
 */
extern void
CVMgcScanSpecial(CVMExecEnv* ee, CVMGCOptions* gcOpts,
		 CVMRefCallbackFunc callback, void* data);

/*
 * Scan the root set of collection
 */
extern void
CVMgcScanRoots(CVMExecEnv* ee, CVMGCOptions* gcOpts,
	       CVMRefCallbackFunc callback, void* data);

/* Purpose: Attempts to compute stackmaps for all the GC's root scan. */
/* Returns: CVM_TRUE if successful, else returns CVM_FALSE. */
extern CVMBool
CVMgcEnsureStackmapsForRootScans(CVMExecEnv *ee);

/*
 * Destroy the heap
 */
extern CVMBool 
CVMgcDestroyHeap();

/* Purpose: Scans objects in the specified memory range and invoke the callback
            function on each object. */
extern CVMBool
CVMgcScanObjectRange(CVMExecEnv* ee, CVMUint32* base, CVMUint32* top,
                     CVMObjectCallbackFunc callback, void* callbackData);

#ifdef CVM_JVMPI
/* Purpose: Posts the JVMPI_EVENT_ARENA_NEW events. */
/* NOTE: This function is necessary to compensate for the fact that
         this event has already transpired by the time that JVMPI is
         initialized. */
void CVMgcPostJVMPIArenaNewEvent(void);

/* Purpose: Gets the last shared arena ID that is used by shared code.  GC
            specific implementations should start their arena ID after the last
            shared arena ID returned by this function. */
extern CVMUint32 CVMgcGetLastSharedArenaID(void);

/* Purpose: Gets the arena ID for the specified object. */
extern CVMUint32 CVMgcGetArenaID(CVMObject *obj);

#endif


#define CVMgcIsInSafeAllState() \
    (CVM_CSTATE(CVM_GC_SAFE)->reached)


#if defined(CVM_DEBUG) || defined(CVM_INSPECTOR)
/* Dumps info about the configuration of the GC. */
#define CVMgcDumpSysInfo() CVMgcimplDumpSysInfo()
#endif

#endif /* _INCLUDED_GC_COMMON_H */
