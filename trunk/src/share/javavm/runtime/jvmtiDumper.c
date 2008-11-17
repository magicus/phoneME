/*
 * @(#)jvmtiDumper.c	1.0 07/01/16
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
 * This file is derived from the original CVM jvmpi.c file.	 In addition
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
#include "javavm/include/stackmaps.h"
#include "javavm/include/bag.h"
#include "javavm/include/porting/time.h"
#include "javavm/include/path_md.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/named_sys_monitor.h"
#include "javavm/include/opcodes.h"
#include "generated/offsets/java_lang_String.h"
#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"
#include "javavm/export/jvmti.h"
#include "javavm/include/jvmti_jni.h"
#include "javavm/include/jvmtiEnv.h"
#include "javavm/include/jvmtiDumper.h"
#include "javavm/include/jvmtiCapabilities.h"


/*
 * Object type constants.
 */
#define JVMTI_NORMAL_OBJECT	         ((jint)0)
#define JVMTI_CLASS		         ((jint)2)
#define JVMTI_BOOLEAN	                 ((jint)4)
#define JVMTI_CHAR                       ((jint)5)
#define JVMTI_FLOAT                      ((jint)6)
#define JVMTI_DOUBLE                     ((jint)7)
#define JVMTI_BYTE                       ((jint)8)
#define JVMTI_SHORT                      ((jint)9)
#define JVMTI_INT                        ((jint)10)
#define JVMTI_LONG                       ((jint)11)    

#define NUM_ENTRIES 20

static void
cbProcessed(CVMInt32 *curBlk, CVMInt32 *pMax,
           CVMClassBlock ***processed, const CVMClassBlock *cb)
{
    CVMClassBlock **tmpP;
    if (*curBlk == *pMax) {
        if (CVMjvmtiAllocate(sizeof(CVMClassBlock*)*(*pMax + NUM_ENTRIES),
                             (unsigned char **)&tmpP) == JVMTI_ERROR_NONE) {
            memcpy(tmpP, *processed, *pMax * sizeof(CVMClassBlock*));
            CVMjvmtiDeallocate((unsigned char*)processed);
            *processed = tmpP;
            *pMax += NUM_ENTRIES;
        } else {
            /* out of memory, we'll just process this cb again which may
             * cause the agent some grief but that's the best we can
             * do at this point.  This should never happen in real life.
             */
            return;
        }
    }
    (*processed)[(*curBlk)++] = (CVMClassBlock *)cb;
}

static CVMBool
checkProcessed(CVMInt32 *curBlk, CVMClassBlock ***processed,
           CVMClassBlock *iCb)
{
    int j;

    for (j = 0; j < *curBlk; j++) {
        if (iCb == (*processed)[j]) {
            return CVM_TRUE;
        }
    }
    return CVM_FALSE;
}
/* Search through all implemented interfaces (and super interfaces)
 * as well as super classes to determine the starting index of 
 * fields *in this class*.  Note that when dumping an instance we dump
 * all the super classes fields first so we don't include super classes
 * in this calculation.  When dumping a Class, which includes
 * the statics of that class, we don't dump all super class statics
 * so we must count super class fields in this calculation
 * For all the gory details see this reference in the JVMTI spec
 * http://java.sun.com/javase/6/docs/platform/jvmti/jvmti.html#jvmtiHeapReferenceInfoField
 */

static CVMInt32
jvmtiFindFieldStartingIndex_X(const CVMClassBlock* fieldCb, int index,
			    CVMBool isStatic,
			    CVMClassBlock ***processed, 
			    CVMInt32 *curBlk, CVMInt32 *pMax)
{
    CVMClassBlock* iCb;
    int i;

    /* C stack redzone check */
    if (!CVMCstackCheckSize(CVMgetEE(),
            CVM_REDZONE_CVMcpFindFieldInSuperInterfaces,
            "CVMcpFindFieldInSuperInterfaces", CVM_FALSE)) {
        return 0;
    }
    /* count direct interfaces */
    for (i = 0; i < CVMcbImplementsCount(fieldCb); i++) {
        iCb = CVMcbInterfacecb(fieldCb, i);
        if (checkProcessed(curBlk, processed, iCb)) {
            /* already processed this interface, so skip it */
            continue;
        }
	/* count superinterfaces */
	index = jvmtiFindFieldStartingIndex_X(iCb, index, isStatic, processed,
					    curBlk, pMax);
	index += CVMcbFieldCount(iCb);
        cbProcessed(curBlk, pMax, processed, iCb);
    }
    if (!CVMcbIs(fieldCb, INTERFACE) && CVMcbSuperclass(fieldCb) != NULL) {
        /* count direct superclasses */
	index = jvmtiFindFieldStartingIndex_X(CVMcbSuperclass(fieldCb), index,
					    isStatic, processed,
					    curBlk, pMax);
	if (isStatic) {
            /* Dumping a Class so we need to count fields to get the
             * proper starting index.
             */
	    index += CVMcbFieldCount(CVMcbSuperclass(fieldCb));
	}
    }
    return index;
}

static CVMInt32
jvmtiFindFieldStartingIndex(const CVMClassBlock* fieldCb, int index,
			    CVMBool isStatic)
{
    CVMInt32 _pMax, _curBlk;
    CVMClassBlock **_processed;
    CVMInt32 retVal;

    if (CVMjvmtiAllocate(sizeof(CVMClassBlock*) * NUM_ENTRIES,
			 (unsigned char **)&_processed) != JVMTI_ERROR_NONE) {
	return 0;
    }
    _pMax = NUM_ENTRIES;
    _curBlk = 0;
    retVal = jvmtiFindFieldStartingIndex_X(fieldCb, index, isStatic,
					   &_processed, &_curBlk, &_pMax);
    CVMjvmtiDeallocate((unsigned char*)_processed);
    return retVal;
}

#define HASH_WITH_CB_BITS(cb, hash) \
    ((hash) |								\
     ((((CVMAddr)(cb) & ~0xf)<<(CVM_HASH_BITS - 4)) ^ ((hash)<<CVM_HASH_BITS)))

/* An 'inline' version of CVMobjectSetHashBitsIfNeeded without any
 * GC safety checks/transitions/locking. Threads are suspended and all GC
 * locks are held.
 */

static CVMInt32
jvmtiObjectGetHash(CVMExecEnv *ee, CVMObject* obj)
{
    CVMAddr bits;
    CVMInt32 hashValue;

    CVMassert(CVMjvmtiIsGCOwner());

    do {
	hashValue = CVMrandomNext();
	hashValue &= CVM_OBJECT_HASH_MASK;
    } while (hashValue == CVM_OBJECT_NO_HASH);

    bits = obj->hdr.various32;

    if (CVMhdrBitsSync(bits) == CVM_LOCKSTATE_MONITOR) {
        CVMObjMonitor *mon = CVMobjMonitor(obj);
        {
            CVMUint32 h = CVMhdrBitsHashCode(mon->bits);
            if (h == CVM_OBJECT_NO_HASH) {
                CVMhdrSetHashCode(mon->bits, hashValue);
            } else {
                hashValue = h;
            }
        }
    } else {

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
        CVMAddr obits;

        obits = obj->hdr.various32;
        if (CVMhdrBitsSync(obits) == CVM_LOCKSTATE_LOCKED) {
            CVMOwnedMonitor *o = (CVMOwnedMonitor *)CVMhdrBitsPtr(obits);
            CVMUint32 h = CVMhdrBitsHashCode(o->u.fast.bits);

#ifdef CVM_DEBUG
            CVMassert(o->magic == CVM_OWNEDMON_MAGIC);
#endif
            CVMassert(o->type == CVM_OWNEDMON_FAST);

            if (h == CVM_OBJECT_NO_HASH) {
		CVMassert(!CVMobjectIsInROM(obj));
		CVMhdrSetHashCode(o->u.fast.bits, hashValue);
            } else {
                hashValue = h;
            }
        } else 
            /* neither LOCKSTATE_MONITOR nor _LOCKED, must be _UNLOCKED if
             * using FASTLOCK 
             */
#endif
        {
            CVMUint32 h = CVMhdrBitsHashCode(obj->hdr.various32);
            if (h == CVM_OBJECT_NO_HASH) {
		CVMassert(!CVMobjectIsInROM(obj));
		CVMhdrSetHashCode(obj->hdr.various32, hashValue);
            } else {
                hashValue = h;
            }
        }
    }

    return HASH_WITH_CB_BITS(CVMobjectGetClass(obj), hashValue);
}

static jint hashRef(jobject ref, CVMBool isGCOwner) 
{
    CVMExecEnv *ee = CVMgetEE();
    jint hashCode;

    if (isGCOwner) {
        CVMObject *obj = CVMjvmtiGetICellDirect(ee, ref);
        hashCode = CVMobjectGetHashNoSet(ee, obj);
        if (hashCode == CVM_OBJECT_NO_HASH) {
            hashCode = jvmtiObjectGetHash(ee, obj);
        }
    } else {
        hashCode = JVM_IHashCode(CVMexecEnv2JniEnv(CVMgetEE()), ref);
    }
    CVMassert(hashCode != CVM_OBJECT_NO_HASH);
    if (hashCode < 0) {
	hashCode = -hashCode;
    }
    return hashCode % HASH_SLOT_COUNT;
}


int
CVMjvmtiGetObjectsWithTag(JNIEnv *env, const jlong *tags, jint tagCount,
			  jobject **objPtr, jlong **tagPtr)
{
    int i, j;
    CVMJvmtiTagNode *node;
    int count = 0;
    CVMExecEnv *ee = CVMgetEE();

    JVMTI_LOCK(ee);
    for (i = 0; i < HASH_SLOT_COUNT; i++) {
	node = CVMglobals.jvmti.statics.objectsByRef[i];
	while (node != NULL) {
	    for (j = 0; j < tagCount; j++) {
		if (node->tag == tags[j]) {
		    if (objPtr != NULL) {
			(*objPtr)[count] = (*env)->NewLocalRef(env, node->ref);
		    }
		    if (tagPtr != NULL) {
			(*tagPtr)[count] = node->tag;
		    }
		    count++;
		}
	    }
	    node = node->next;
	}
    }
    JVMTI_UNLOCK(ee);
    return count;
}

void CVMjvmtiTagRehash() {

    CVMJvmtiTagNode *node, *next, *prev;
    int i;
    CVMExecEnv *ee = CVMgetEE();

    JVMTI_LOCK(ee);
    for (i = 0; i < HASH_SLOT_COUNT; i++) {
	node = CVMglobals.jvmti.statics.objectsByRef[i];
	prev = NULL;
	while (node != NULL) {
	    next = node->next;
	    if (node->ref == NULL || CVMID_icellIsNull(node->ref)) {
		if (prev == NULL) {
		    CVMglobals.jvmti.statics.objectsByRef[i] = node->next;
		} else {
		    prev->next = node->next;
		}
		CVMjvmtiDeallocate((unsigned char *)node);
	    } else {
		prev = node;
	    }
	    node = next;
	}
    }
    JVMTI_UNLOCK(ee);
}

jvmtiError
CVMjvmtiTagGetTag(jobject object, jlong* tagPtr,
                  CVMBool isGCOwner) {

    CVMJvmtiTagNode *node;
    CVMJvmtiTagNode *prev;
    JNIEnv *env;
    CVMExecEnv *ee = CVMgetEE();
    jint slot;

    *tagPtr = CVMlongConstZero();
    if (object == NULL || CVMID_icellIsNull(object)) {
	return JVMTI_ERROR_NONE;
    }

    JVMTI_LOCK(ee);

    CVMassert(CVMjvmtiIsGCOwner() || !isGCOwner);
    slot = hashRef(object, isGCOwner);

    env = CVMexecEnv2JniEnv(ee);
    node = CVMglobals.jvmti.statics.objectsByRef[slot];
    prev = NULL;

    while (node != NULL) {
        if (isGCOwner) {
            if (CVMjvmtiGetICellDirect(ee, object) ==
                CVMjvmtiGetICellDirect(ee, node->ref)) {
                break;
            }
        } else {
            if ((*env)->IsSameObject(env, object, node->ref)) {
                break;
            }
        }
	prev = node;
	node = node->next;
    }
    if (node == NULL) {
	*tagPtr = 0L;
    } else {
	*tagPtr = node->tag;
    }
    JVMTI_UNLOCK(ee);
    return JVMTI_ERROR_NONE;
}

jvmtiError
CVMjvmtiTagSetTag(jobject object, jlong tag,
                  CVMBool isGCOwner)
{
    JNIEnv *env;
    jint slot;
    CVMJvmtiTagNode *node, *prev;
    CVMExecEnv *ee = CVMgetEE();

    /*
     * Add to reference hashtable 
     */
    if (object == NULL || CVMID_icellIsNull(object)) {
	return JVMTI_ERROR_INVALID_OBJECT;
    }

    CVMassert(CVMjvmtiIsGCOwner() || !isGCOwner);
    JVMTI_LOCK(ee);
    slot = hashRef(object, isGCOwner);
    node = CVMglobals.jvmti.statics.objectsByRef[slot];
    prev = NULL;

    env = CVMexecEnv2JniEnv(ee);
    while (node != NULL) {
        if (isGCOwner) {
            if (!CVMID_icellIsNull(node->ref) &&
                (CVMjvmtiGetICellDirect(ee, object) ==
                 CVMjvmtiGetICellDirect(ee, node->ref))) {
                break;
            }
        } else {
            if (!CVMID_icellIsNull(node->ref) &&
                (*env)->IsSameObject(env, object, node->ref)) {
                break;
            }
        }
	prev = node;
	node = node->next;
    }
    if (node == NULL) {
	if (CVMjvmtiAllocate(sizeof(CVMJvmtiTagNode),
			     (unsigned char **)&node) !=
	    JVMTI_ERROR_NONE) {
	    JVMTI_UNLOCK(ee);
	    return JVMTI_ERROR_OUT_OF_MEMORY;
	}
	node->ref = NULL;
    }
    if (tag == 0L && node->ref != NULL) {
	/* clearing tag, deallocate it */
	if (prev == NULL) {
	    CVMglobals.jvmti.statics.objectsByRef[slot] = node->next;
	} else {
	    prev->next = node->next;
	}
	(*env)->DeleteWeakGlobalRef(env, node->ref);
	CVMjvmtiDeallocate((unsigned char *)node);
    } else {
        jobject objRef;
        if (isGCOwner) {
            CVMObject *obj = CVMjvmtiGetICellDirect(ee, object);
            objRef = CVMID_getWeakGlobalRoot(ee);
            if (objRef != NULL) {
                CVMjvmtiSetICellDirect(ee, objRef, obj);
            }
        } else {
            objRef = (*env)->NewWeakGlobalRef(env, object);
        }
        if (node->ref != NULL) {
            (*env)->DeleteWeakGlobalRef(env, node->ref);
        } else {
            node->next = CVMglobals.jvmti.statics.objectsByRef[slot];
            CVMglobals.jvmti.statics.objectsByRef[slot] = node;
        }
	node->ref = objRef;
	node->tag = tag;
    }
    JVMTI_UNLOCK(ee);
    return JVMTI_ERROR_NONE;
}

void
CVMjvmtiPostCallbackUpdateTag(CVMObject *obj, jlong tag)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMObjectICell objRef;

    CVMassert(CVMD_isgcSafe(ee));
    CVMassert(CVMjvmtiIsGCOwner());

    /* When we callback into the agent we pass a pointer to the tag.
     * The agent may change the actual value of the tag or create a
     * a new tag if one doesn't exist.
     */
    JVMTI_LOCK(ee);
    CVMjvmtiSetICellDirect(ee, &objRef, obj);
    CVMjvmtiTagSetTag(&objRef, tag, CVM_TRUE);
    JVMTI_UNLOCK(ee);
}

static CVMBool
jvmtiCheckForVisit(CVMObject *obj)
{
    CVMjvmtiVisitStackPush(obj);
    return CVM_TRUE;
}
static CVMBool
jvmtiIsFilteredByHeapFilter(jlong objTag, jlong objKlassTag,
			   jint heapFilter)
{
    /* apply the heap filter */
    if (objTag != 0) {
	/* filter out tagged objects */
	if (heapFilter & JVMTI_HEAP_FILTER_TAGGED) {
	    return CVM_TRUE;
	}
    } else {
	/* filter out untagged objects */
	if (heapFilter & JVMTI_HEAP_FILTER_UNTAGGED) {
	    return CVM_TRUE;
	}
    }
    if (objKlassTag != 0) {
	/* filter out objects with tagged classes */
	if (heapFilter & JVMTI_HEAP_FILTER_CLASS_TAGGED) {
	    return CVM_TRUE;
	}
    } else {
	/* filter out objects with untagged classes. */
	if (heapFilter & JVMTI_HEAP_FILTER_CLASS_UNTAGGED) {
	    return CVM_TRUE;
	}
    }
    return CVM_FALSE;
}

static CVMBool
jvmtiIsFilteredByKlassFilter(CVMClassBlock * objCb, jclass klass)
{
    if (klass != NULL) {
	if (objCb != CVMjvmtiClassRef2ClassBlock(CVMgetEE(), klass)) {
	    return CVM_TRUE;
	}
    }
    return CVM_FALSE;
}

/* invoke the object reference callback to report a reference */
static CVMBool
jvmtiObjectRefCallback(jvmtiHeapReferenceKind refKind,
		    CVMObject *referrer,
		    const CVMClassBlock *referrerCb,
		    CVMObject *obj, 
		    CVMJvmtiDumpContext *dc,
		    jint index,
		    CVMBool isRoot) 
{
    jint len;
    int res;
    jlong referrerKlassTag = 0, referrerTag = 0;
    jlong objTag, objKlassTag;
    jsize objSize;
    jvmtiHeapReferenceCallback cb = dc->callbacks->heap_reference_callback;
    /* field index is only valid field in reference_info */
    jvmtiHeapReferenceInfo refInfo, *refInfoPtr;
    CVMClassBlock *objCb;

    refInfoPtr = &refInfo;

    /* check that callback is provider */
    if (cb == NULL) {
	return jvmtiCheckForVisit(obj);
    }
    if (!isRoot) {
	CVMjvmtiTagGetTag((jobject)&referrer, &referrerTag, CVM_TRUE);
	CVMjvmtiTagGetTag(CVMcbJavaInstance(referrerCb),
                          &referrerKlassTag, CVM_TRUE);
    }
    objCb = CVMobjectGetClass(obj);
    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(objCb, dc->klass)) {
	return jvmtiCheckForVisit(obj);
    }
    
    CVMjvmtiTagGetTag((jobject)&obj, &objTag, CVM_TRUE);
    CVMjvmtiTagGetTag(CVMcbJavaInstance(objCb), &objKlassTag, CVM_TRUE);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(objTag, objKlassTag, dc->heapFilter)) {
	return jvmtiCheckForVisit(obj);
    }
    objSize = CVMobjectSizeGivenClass(obj, objCb);
    switch(refKind) {
    case JVMTI_HEAP_REFERENCE_FIELD:
    case JVMTI_HEAP_REFERENCE_STATIC_FIELD:
	/* field index is only valid field in reference_info */
	refInfo.field.index = index;
	break;
    case JVMTI_HEAP_REFERENCE_ARRAY_ELEMENT:
	refInfo.array.index = index;
	break;
    case JVMTI_HEAP_REFERENCE_CONSTANT_POOL:
	refInfo.constant_pool.index = index;
	break;
    default:
	refInfoPtr = NULL;
	break;
    }
    /* for arrays we need the length, otherwise -1 */
    if (CVMisArrayClass(objCb)) {
	len = CVMD_arrayGetLength((CVMArrayOfAnyType *) obj);
    } else {
	len = -1;
    }

    /* invoke the callback */
    res = (*cb)(refKind, 
		refInfoPtr,
		objKlassTag,
		referrerKlassTag,
		objSize,
		&objTag,
		isRoot ? NULL : &referrerTag,
		len,
		(void*)dc->userData);

    CVMjvmtiPostCallbackUpdateTag(obj, objTag);
    if (!isRoot) {
	CVMjvmtiPostCallbackUpdateTag(referrer, referrerTag);
    }
    if (res & JVMTI_VISIT_ABORT) {
	return CVM_FALSE;
    }
    if (res & JVMTI_VISIT_OBJECTS) {
	jvmtiCheckForVisit(obj);
    }
    return CVM_TRUE;
}

/* invoke the stack reference callback to report a stack reference */
static CVMBool
jvmtiStackRefCallback(jvmtiHeapReferenceKind refKind,
		   CVMObject *obj, 
		   CVMJvmtiDumpContext *dc,
		   jint index)
{
    jint len;
    int res;
    jlong objTag, objKlassTag, threadTag;
    jsize objSize;
    jvmtiHeapReferenceCallback cb = dc->callbacks->heap_reference_callback;
    /* field index is only valid field in reference_info */
    jvmtiHeapReferenceInfo refInfo;
    CVMClassBlock *objCb;
    CVMExecEnv *ee = dc->ee; 

    /* check that callback is provider */
    if (cb == NULL) {
	return jvmtiCheckForVisit(obj);
    }
    objCb = CVMobjectGetClass(obj);
    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(objCb, dc->klass)) {
	return jvmtiCheckForVisit(obj);
    }
    
    CVMjvmtiTagGetTag((jobject)&obj, &objTag, CVM_TRUE);
    CVMjvmtiTagGetTag(CVMcbJavaInstance(objCb), &objKlassTag, CVM_TRUE);
    CVMjvmtiTagGetTag(CVMcurrentThreadICell(ee), &threadTag, CVM_TRUE);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(objTag, objKlassTag, dc->heapFilter)) {
	return jvmtiCheckForVisit(obj);
    }
    objSize = CVMobjectSizeGivenClass(obj, objCb);
    switch(refKind) {
    case JVMTI_HEAP_REFERENCE_STACK_LOCAL:
	refInfo.stack_local.thread_tag = threadTag;
	refInfo.stack_local.thread_id = ee->threadID;
	refInfo.stack_local.depth = dc->frameCount;
	refInfo.stack_local.method = CVMframeGetMb(dc->frame);
	refInfo.stack_local.location = (jlong)((int)CVMframePc(dc->frame));
	refInfo.stack_local.slot = index;
	break;
    case JVMTI_HEAP_REFERENCE_JNI_LOCAL:
	refInfo.stack_local.thread_tag = threadTag;
	refInfo.stack_local.thread_id = ee->threadID;
	refInfo.stack_local.depth = dc->frameCount;
	refInfo.stack_local.method = CVMframeGetMb(dc->frame);
	break;
    default:
	break;
    }

    len = -1;
    /* for arrays we need the length, otherwise -1 */
    if (CVMisArrayClass(objCb)) {
	len = CVMD_arrayGetLength((CVMArrayOfAnyType *) obj);
    }

    /* invoke the callback */
    res = (*cb)(refKind, 
		&refInfo,
		objKlassTag,
		0,
		objSize,
		&objTag,
		NULL,
		len,
		(void*)dc->userData);

    CVMjvmtiPostCallbackUpdateTag(obj, objTag);
    if (res & JVMTI_VISIT_ABORT) {
	return CVM_FALSE;
    }
    if (res & JVMTI_VISIT_OBJECTS) {
	jvmtiCheckForVisit(obj);
    }
    return CVM_TRUE;
}

static jvmtiPrimitiveType CVMtype2jvmtiPrimitiveType[]= {0,
				      0,
				      0,
				      JVMTI_PRIMITIVE_TYPE_INT,
				      JVMTI_PRIMITIVE_TYPE_SHORT,
				      JVMTI_PRIMITIVE_TYPE_CHAR,
				      JVMTI_PRIMITIVE_TYPE_LONG,
				      JVMTI_PRIMITIVE_TYPE_BYTE,
				      JVMTI_PRIMITIVE_TYPE_FLOAT,
				      JVMTI_PRIMITIVE_TYPE_DOUBLE,
				      JVMTI_PRIMITIVE_TYPE_BOOLEAN
};

/* invoke the primitive reference callback to report a primitive reference */
static CVMBool
jvmtiPrimitiveRefCallback(jvmtiHeapReferenceKind refKind,
		       CVMObject *obj, 
		       CVMJvmtiDumpContext *dc,
		       jint index,
		       int CVMtype,
		       jvalue v)
{
    int res;
    jlong objTag, objKlassTag;
    jvmtiPrimitiveFieldCallback cb = dc->callbacks->primitive_field_callback;
    /* field index is only valid field in reference_info */
    jvmtiHeapReferenceInfo refInfo;
    CVMClassBlock *objCb;

    objCb = CVMobjectGetClass(obj);
    if (objCb == CVMsystemClass(java_lang_Class)) {
	objCb = CVMsystemClass(java_lang_Class);
    }

    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(objCb, dc->klass)) {
	return jvmtiCheckForVisit(obj);
    }
    
    CVMjvmtiTagGetTag((jobject)&obj, &objTag, CVM_TRUE);
    CVMjvmtiTagGetTag(CVMcbJavaInstance(objCb), &objKlassTag, CVM_TRUE);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(objTag, objKlassTag, dc->heapFilter)) {
	return jvmtiCheckForVisit(obj);
    }

    refInfo.field.index = index;

    /* invoke the callback */
    res = (*cb)(refKind, 
		&refInfo,
		objKlassTag,
		&objTag,
		v,
		CVMtype2jvmtiPrimitiveType[CVMtype],
		(void*)dc->userData);

    CVMjvmtiPostCallbackUpdateTag(obj, objTag);
    if (res & JVMTI_VISIT_ABORT) {
	return CVM_FALSE;
    }
    return CVM_TRUE;
}

/* invoke the array primitive callback to report a primitive array */
static CVMBool
jvmtiArrayPrimCallback(CVMObject *obj, 
		    CVMClassBlock *objCb,
		    void *elements,
		    CVMJvmtiDumpContext *dc,
		    jint count,
		    int jvmtiType)
{
    int res;
    jlong objTag, objKlassTag;
    jvmtiArrayPrimitiveValueCallback callback;
    /* field index is only valid field in reference_info */
    jlong objSize;

    callback = dc->callbacks->array_primitive_value_callback;

    if (objCb == CVMsystemClass(java_lang_Class)) {
	objCb = CVMsystemClass(java_lang_Class);
    }

    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(objCb, dc->klass)) {
	return jvmtiCheckForVisit(obj);
    }
    
    CVMjvmtiTagGetTag((jobject)&obj, &objTag, CVM_TRUE);
    CVMjvmtiTagGetTag(CVMcbJavaInstance(objCb), &objKlassTag, CVM_TRUE);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(objTag, objKlassTag, dc->heapFilter)) {
	return jvmtiCheckForVisit(obj);
    }
    objSize = CVMobjectSizeGivenClass(obj, objCb);

    /* invoke the callback */
    res = (*callback)(objKlassTag,
		      objSize,
		      &objTag,
		      count,
		      jvmtiType,
		      elements,
		      (void*)dc->userData);

    CVMjvmtiPostCallbackUpdateTag(obj, objTag);
    if (res & JVMTI_VISIT_ABORT) {
	return CVM_FALSE;
    }
    return CVM_TRUE;
}


static CVMBool
jvmtiStringPrimCallback(CVMObject *obj, 
			CVMJvmtiDumpContext *dc)
{
    int res;
    jlong objTag, objKlassTag;
    jvmtiStringPrimitiveValueCallback callback;
    /* field index is only valid field in reference_info */
    jlong objSize;
    CVMInt32 len, offset;
    CVMObject *value;
    CVMClassBlock *objCb;
    const jchar *charValues;

    callback = dc->callbacks->string_primitive_value_callback;

    objCb = CVMobjectGetClass(obj);
    CVMassert(objCb == CVMsystemClass(java_lang_String));

    CVMD_fieldReadRef(obj, CVMoffsetOfjava_lang_String_value, value);
    CVMD_fieldReadInt(obj, CVMoffsetOfjava_lang_String_count, len);
    CVMD_fieldReadInt(obj, CVMoffsetOfjava_lang_String_offset, offset);
    if (value == NULL) {
	return CVM_TRUE;
    }

    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(objCb, dc->klass)) {
	return jvmtiCheckForVisit(obj);
    }
    
    CVMjvmtiTagGetTag((jobject)&obj, &objTag, CVM_TRUE);
    CVMjvmtiTagGetTag(CVMcbJavaInstance(objCb), &objKlassTag, CVM_TRUE);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(objTag, objKlassTag, dc->heapFilter)) {
	return jvmtiCheckForVisit(obj);
    }
    objSize = CVMobjectSizeGivenClass(obj, objCb);

    /* invoke the callback */
    charValues =
	(const jchar *)CVMDprivate_arrayElemLoc(((CVMArrayOfChar *)value),0);
    res = (*callback)(objKlassTag,
		      objSize,
		      &objTag,
		      charValues,
		      len,
		      (void*)dc->userData);

    CVMjvmtiPostCallbackUpdateTag(obj, objTag);
    if (res & JVMTI_VISIT_ABORT) {
	return CVM_FALSE;
    }
    return CVM_TRUE;
}


static CVMBool
jvmtiDumpInstanceFields(CVMObject *obj, const CVMClassBlock *cb,
			CVMClassBlock *referrerCb, CVMJvmtiDumpContext *dc,
			CVMInt32 *fieldIndex, CVMBool doRefs)
{
    CVMInt32 i;

    /* NOTE: We're interating through the fields in reverse order.  This is
       intentional because JVMTI list fields in reverse order than that
       which is declared in the ClassFile.  This is especially evident in
       the definition of the JVMTI_GC_INSTANCE_DUMP record where the fields
       of the self class is to preceed that of its super class and so on so
       forth.
    */
    for (i = 0; i < CVMcbFieldCount(cb); i++, (*fieldIndex)++) {
	CVMFieldBlock *fb = CVMcbFieldSlot(cb, i);
	CVMClassTypeID fieldType;
	if (CVMfbIs(fb, STATIC)) {
	    continue;
	}
	fieldType = CVMtypeidGetType(CVMfbNameAndTypeID(fb));
	if (doRefs && CVMtypeidFieldIsRef(fieldType)) {
	    CVMObject *value;
	    CVMD_fieldReadRef(obj, CVMfbOffset(fb), value);
	    if (value != NULL) {
		if (jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_FIELD, obj,
				    referrerCb, value, dc, *fieldIndex,
					   CVM_FALSE) == CVM_FALSE) {
		    return CVM_FALSE;
		}
	    }

	} else if (dc->callbacks->primitive_field_callback != NULL) {
	    /* primitive value */
	    jvalue v;

#define DUMP_VALUE(type_, result_) {			   \
		CVMD_fieldRead##type_(obj, CVMfbOffset(fb), result_);	\
}

	    switch (fieldType) {
	    case CVM_TYPEID_LONG:    DUMP_VALUE(Long,   v.j); break;
	    case CVM_TYPEID_DOUBLE:  DUMP_VALUE(Double, v.d); break;
	    case CVM_TYPEID_INT:     DUMP_VALUE(Int,     v.i); break;
	    case CVM_TYPEID_FLOAT:   DUMP_VALUE(Float,   v.f); break;
	    case CVM_TYPEID_SHORT:   DUMP_VALUE(Int,   v.s); break;
	    case CVM_TYPEID_CHAR:    DUMP_VALUE(Int,    v.c); break;
	    case CVM_TYPEID_BYTE:    DUMP_VALUE(Int,    v.b); break;
	    case CVM_TYPEID_BOOLEAN: DUMP_VALUE(Int, v.z); break;
	    default : DUMP_VALUE(Int, v.i); break; /* fix compiler warning */
	    }
#undef DUMP_VALUE
	    if (jvmtiPrimitiveRefCallback(JVMTI_HEAP_REFERENCE_FIELD,
					  obj, 
					  dc,
					  *fieldIndex,
					  fieldType,
					  v) == CVM_FALSE) {
		return CVM_FALSE;
	    }
	}
    }
    if (CVMobjectGetClass(obj) == CVMsystemClass(java_lang_String) &&
	dc->callbacks->string_primitive_value_callback != NULL) {
	return jvmtiStringPrimCallback(obj, dc);
    }
    return CVM_TRUE;
}

static CVMBool
jvmtiDumpSuperInstanceFields(CVMObject *obj,
			     CVMClassBlock *cb,
			     CVMClassBlock *referrerCb,
			     CVMJvmtiDumpContext *dc,
			     CVMInt32 *fieldIndex,
			     CVMBool doRefs) 
{
    CVMClassBlock *superCb = CVMcbSuperclass(cb);
    CVMBool result;
    if (superCb != NULL) {
	result = jvmtiDumpSuperInstanceFields(obj, superCb,
					      referrerCb, dc,
					      fieldIndex, doRefs);
	if (result == CVM_FALSE) {
	    return result;
	}
    }
    return jvmtiDumpInstanceFields(obj, cb, referrerCb, dc,
				   fieldIndex, doRefs);
}

/* Purpose: Dump an object instance for Level 1 or 2 dumps. */
/* Returns: CVM_TRUE if successful. */
static CVMBool
jvmtiDumpInstance(CVMObject *obj, CVMJvmtiDumpContext *dc)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMClassBlock *cb = CVMobjectGetClass(obj);
    CVMInt32 fieldIndex;

    ee = ee; /* for non-debug compile */
    if (jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_CLASS, obj, cb,
		(void *)CVMjvmtiGetICellDirect(ee, CVMcbJavaInstance(cb)),
			       dc, -1, CVM_FALSE) == CVM_FALSE) {
	return CVM_FALSE;
    }

    /* Dump the field values: */
    fieldIndex = jvmtiFindFieldStartingIndex(cb, 0, CVM_FALSE);
    return jvmtiDumpSuperInstanceFields(obj, cb, cb, dc,
					&fieldIndex, CVM_TRUE);
}

 
static  CVMBool
jvmtiDumpStaticFields(CVMObject *clazz, const CVMClassBlock *cb,
		      CVMJvmtiDumpContext *dc, CVMBool doRefs)
{
    CVMExecEnv *ee = CVMgetEE();
    jint fieldIndex;
    CVMBool result;

    /* dump static field values */
    fieldIndex = jvmtiFindFieldStartingIndex(cb, 0, CVM_TRUE);
    if ((CVMcbFieldCount(cb) > 0) && (CVMcbFields(cb) != NULL)) {
        CVMInt32 i;

        for (i = 0; i < CVMcbFieldCount(cb); i++, fieldIndex++) {
            const CVMFieldBlock *fb = CVMcbFieldSlot(cb, i);
            CVMClassTypeID fieldType;

            if (!CVMfbIs(fb, STATIC)) {
                continue;
            }
            fieldType = CVMtypeidGetType(CVMfbNameAndTypeID(fb));
            if (doRefs && CVMtypeidFieldIsRef(fieldType)) {
                CVMObject *value =
		    (CVMObject*)CVMjvmtiGetICellDirect(ee, &CVMfbStaticField(ee, fb).r);
		if (value != NULL) {
		    result = jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_STATIC_FIELD,
					clazz, cb, value, dc, fieldIndex,
					CVM_FALSE);
		    if (result == CVM_FALSE) {
			return result;
		    }
		}
            } else if (dc->callbacks->primitive_field_callback != NULL) {
		/* primitive value */
		jvalue v;

#define DUMP_VALUE(type_, result_) {			\
    result_ = (type_)*(&CVMfbStaticField(ee, fb).raw);	\
}

#undef DUMP_VALUE2
#define DUMP_VALUE2(type_, result_) {			     \
    result_ = CVMjvm2##type_(&CVMfbStaticField(ee, fb).raw); \
}
                switch (fieldType) {
		case CVM_TYPEID_LONG:    DUMP_VALUE2(Long,   v.j); break;
		case CVM_TYPEID_DOUBLE:  DUMP_VALUE2(Double, v.d); break;
		case CVM_TYPEID_INT:     DUMP_VALUE(jint,     v.i); break;
		case CVM_TYPEID_FLOAT:   DUMP_VALUE(jfloat,   v.f); break;
		case CVM_TYPEID_SHORT:   DUMP_VALUE(jshort,   v.s); break;
		case CVM_TYPEID_CHAR:    DUMP_VALUE(jchar,    v.c); break;
		case CVM_TYPEID_BYTE:    DUMP_VALUE(jbyte,    v.b); break;
		case CVM_TYPEID_BOOLEAN: DUMP_VALUE(jboolean, v.z); break;
                default: DUMP_VALUE(jboolean, v.i); break;
                }
#undef DUMP_VALUE
#undef DUMP_VALUE2

		result =
		    jvmtiPrimitiveRefCallback(JVMTI_HEAP_REFERENCE_STATIC_FIELD,
					      clazz, 
					      dc,
					      fieldIndex,
					      fieldType,
					      v);
		if (result == CVM_FALSE) {
		    return result;
		}
            }
        }
    }
    return CVM_TRUE;
}

/* Purpose: Dump an instance of java.lang.Class. */
/* Returns: CVM_TRUE if successful. */

static CVMBool
jvmtiDumpClass(CVMObject *clazz, CVMJvmtiDumpContext *dc)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMClassBlock *cb = CVMjvmtiClassObject2ClassBlock(ee, clazz);
    CVMObject *signer;
    CVMBool result;

    /* If the class has not been initialized yet, then we cannot dump it: */
    if (!CVMcbInitializationDoneFlag(ee, cb))
        return CVM_TRUE;

    /* %comment l029 */
    signer = NULL;

    {
        CVMClassBlock *superCB = CVMcbSuperclass(cb);
        CVMObject *superObj;
	if (superCB != NULL && superCB != CVMsystemClass(java_lang_Object)) {
	    superObj = CVMjvmtiGetICellDirect(ee, CVMcbJavaInstance(superCB));
	    result = jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_SUPERCLASS,
					    clazz, cb, superObj, dc,
					    -1, CVM_FALSE);
	    if (result == CVM_FALSE) {
		return CVM_FALSE;
	    }
	}
			    
    }
    {
        CVMObjectICell *classloader = CVMcbClassLoader(cb);
	if (classloader != NULL && !CVMID_icellIsNull(classloader)) {
	    result = jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_CLASS_LOADER,
				    clazz, cb,
				    CVMjvmtiGetICellDirect(ee, classloader),
				    dc, -1, CVM_FALSE);
	    if (result == CVM_FALSE) {
		return CVM_FALSE;
	    }
	}
    }
    {
        CVMObjectICell *protectionDomain = CVMcbProtectionDomain(cb);
	if (protectionDomain != NULL && !CVMID_icellIsNull(protectionDomain)) {
	    result = jvmtiObjectRefCallback(
				JVMTI_HEAP_REFERENCE_PROTECTION_DOMAIN,
				clazz, cb,
			        CVMjvmtiGetICellDirect(ee, protectionDomain),
				dc, -1, CVM_FALSE);
	    if (result == CVM_FALSE) {
		return CVM_FALSE;
	    }
	}
    }
    if (signer != NULL) {
	result = jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_SIGNERS,
					clazz, cb, signer,
					dc, -1, CVM_FALSE);
	if (result == CVM_FALSE) {
	    return CVM_FALSE;
	}
    }

    /* Dump constant pool info: */
    /* NOTE: CVMcpTypes() is always NULL for ROMized classes.  So we can't get
       the constant pool type for its entries: */
    if (!CVMisArrayClass(cb) && !CVMcbIsInROM(cb)) {
        CVMConstantPool *cp = CVMcbConstantPool(cb);
        CVMUint16 i;

        /* Dump the relevent ConstantPoolEntries: */
        for (i = 1; i < CVMcbConstantPoolCount(cb); i++) {
            if (CVMcpIsResolved(cp, i)) {
                CVMConstantPoolEntryType entryType = CVMcpEntryType(cp, i);
                switch (entryType) {
		case CVM_CONSTANT_String: {
		    CVMStringICell *str = CVMcpGetStringICell(cp, i);
		   result = jvmtiObjectRefCallback(
			        JVMTI_HEAP_REFERENCE_CONSTANT_POOL,
				clazz, cb,
				(void *)CVMjvmtiGetICellDirect(ee, str),
					dc, i, CVM_FALSE);
		   if (result == CVM_FALSE) {
		       return CVM_FALSE;
		   }
		   break;
		}
		case CVM_CONSTANT_Class: {
		    CVMClassBlock *classblock = CVMcpGetCb(cp, i);
		    CVMClassICell *clazzClazz = CVMcbJavaInstance(classblock);
		    result = jvmtiObjectRefCallback(
			        JVMTI_HEAP_REFERENCE_CONSTANT_POOL,
				clazz, cb,
				(void *)CVMjvmtiGetICellDirect(ee, clazzClazz),
					dc, i, CVM_FALSE);
		    if (result == CVM_FALSE) {
			return CVM_FALSE;
		    }
		    break;
		}
		case CVM_CONSTANT_Long:
		case CVM_CONSTANT_Double:
		    i++;
                }
            }
        }
    }
    /* Dump the interfaces implemented by this class: */
    {
        CVMUint16 i;
        CVMUint16 count = CVMcbImplementsCount(cb);
        for (i = 0; i < count; i++) {
            CVMClassBlock *interfaceCb = CVMcbInterfacecb(cb, i);
            if (interfaceCb) {
                CVMClassICell *iclazz = CVMcbJavaInstance(interfaceCb);
                CVMObject *interfaceObj =
                    (CVMObject *)CVMjvmtiGetICellDirect(ee, iclazz);
		result = jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_INTERFACE,
						clazz, cb, interfaceObj,
						dc, i, CVM_FALSE);
		if (result == CVM_FALSE) {
		    return CVM_FALSE;
		}
            }
        }
    }
    return jvmtiDumpStaticFields(clazz, cb, dc, CVM_TRUE);
}

static CVMBool
jvmtiDumpArrayPrimitives(CVMObject *arrObj, CVMClassBlock *cb, jint size,
			 CVMJvmtiDumpContext *dc)
{
    
    void *elements;
    CVMBool result;

#undef DUMP_ELEMENTS
#define DUMP_ELEMENTS(type_, ptr_type_, jvmtitype_) {		\
    jint i;								\
    int allocSize = (size == 0 ? 1 : size);				\
    if (CVMjvmtiAllocate(allocSize * sizeof(ptr_type_),			\
			 (unsigned char**)&elements) != JVMTI_ERROR_NONE) { \
	return CVM_FALSE;						\
    }									\
    for (i = 0; i < size; i++) {				\
	CVMD_arrayRead##type_(((CVMArrayOf##type_ *)arrObj),	\
			      i, ((ptr_type_*)elements)[i]);	\
    }								\
    result = jvmtiArrayPrimCallback(arrObj, cb, elements, dc,		\
			   size, JVMTI_PRIMITIVE_TYPE_##jvmtitype_);	\
    if (result == CVM_FALSE) {						\
	CVMjvmtiDeallocate((unsigned char *)elements);			\
	return CVM_FALSE;						\
    }									\
  }
    switch (CVMarrayBaseType(cb)) {
    case CVM_T_INT:	DUMP_ELEMENTS(Int, jint, INT);             break;
    case CVM_T_LONG:    DUMP_ELEMENTS(Long, jlong, LONG);          break;
    case CVM_T_DOUBLE:  DUMP_ELEMENTS(Double, jdouble, DOUBLE);    break;
    case CVM_T_FLOAT:   DUMP_ELEMENTS(Float, jfloat, FLOAT);       break;
    case CVM_T_SHORT:   DUMP_ELEMENTS(Short, jshort, SHORT);       break;
    case CVM_T_CHAR:    DUMP_ELEMENTS(Char, jchar, CHAR);          break;
    case CVM_T_BYTE:    DUMP_ELEMENTS(Byte, jbyte, BYTE);          break;
    case CVM_T_BOOLEAN: DUMP_ELEMENTS(Boolean, jboolean, BOOLEAN); break;
	
    case CVM_T_CLASS:
    case CVM_T_VOID:
    case CVM_T_ERR:
    default:
        elements = NULL; /* avoid compiler warning */
	CVMassert(CVM_FALSE);
	break;
    }
    CVMjvmtiDeallocate((unsigned char *)elements);
#undef DUMP_ELEMENTS
    return CVM_TRUE;
}

/* Purpose: Dump an array. */
/* Returns: CVM_TRUE if successful. */

static CVMBool
jvmtiDumpArray(CVMObject *arrObj, CVMJvmtiDumpContext *dc)
{
    CVMClassBlock *cb = CVMobjectGetClass(arrObj);
    jint size = CVMD_arrayGetLength((CVMArrayOfAnyType *) arrObj);
    CVMBool result;

    result = jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_CLASS, arrObj, cb,
		   (void *)CVMjvmtiGetICellDirect(CVMgetEE(),
		   CVMcbJavaInstance(cb)), dc, -1, CVM_FALSE);
    if (result == CVM_FALSE) {
	return result;
    }

    if (!CVMisArrayOfAnyBasicType(cb)) {
	jint i;
        for (i = 0; i < size; i++) {
            CVMObject *element;
            CVMD_arrayReadRef((CVMArrayOfRef *)arrObj, i, element);
	    if (element != NULL) {
		result =
		    jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_ARRAY_ELEMENT,
					   arrObj, cb, element, dc,
					   i, CVM_FALSE);
		if (result == CVM_FALSE) {
		    return result;
		}
	    }
	}
    } else {
	jvmtiDumpArrayPrimitives(arrObj, cb, size, dc);
    }
    return CVM_TRUE;
}

/* Purpose: Dumps the object info */
/* Returns: CVM_TRUE if successful. */

CVMBool
CVMjvmtiDumpObject(CVMObject *obj, CVMJvmtiDumpContext *dc)
{
    CVMBool result = CVM_TRUE;
    CVMClassBlock *cb = CVMobjectGetClass(obj);

    if (CVMisArrayClass(cb)) {
	result = jvmtiDumpArray(obj, dc);
    } else {
	if (cb != (CVMClassBlock*)CVMsystemClass(java_lang_Class)) {
	    result = jvmtiDumpInstance(obj, dc);
	} else {
	    result = jvmtiDumpClass(obj, dc);
	}
    }
    return result;
}

CVMBool
CVMjvmtiIterateDoObject(CVMObject *obj, CVMClassBlock *objCb,
				CVMUint32 objSize, void *data)
{
    CVMJvmtiDumpContext *dc = (CVMJvmtiDumpContext*)data;
    jlong objKlassTag, objTag;
    jint len = -1;
    int res;
    CVMBool isArray;
    CVMInt32 fieldIndex;

    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(objCb, dc->klass)) {
	return CVM_TRUE;
    }
    
    CVMjvmtiTagGetTag((jobject)&obj, &objTag, CVM_TRUE);
    CVMjvmtiTagGetTag(CVMcbJavaInstance(objCb), &objKlassTag, CVM_TRUE);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(objTag, objKlassTag, dc->heapFilter)) {
	return CVM_TRUE;
    }
    if ((isArray = CVMisArrayClass(objCb))) {
	len = CVMD_arrayGetLength((CVMArrayOfAnyType *) obj);
    }
    if (dc->callbacks->heap_iteration_callback != NULL) {
	jvmtiHeapIterationCallback cb = dc->callbacks->heap_iteration_callback;
	res = (*cb)(objKlassTag,
		    objSize,
		    &objTag,
		    len,
		    data);
	if (res & JVMTI_VISIT_ABORT) {
	    return CVM_FALSE;
	}
    }
 
    if (dc->callbacks->primitive_field_callback != NULL &&
	!isArray) {
	if (objCb == CVMsystemClass(java_lang_Class)) {
	    if (jvmtiDumpStaticFields(obj, objCb, dc,
				      CVM_FALSE) == CVM_FALSE) {
		return CVM_FALSE;
	    }
	} else {
 	    /* Dump the field values: */
	    fieldIndex = jvmtiFindFieldStartingIndex(objCb, 0, CVM_FALSE);
	    if (jvmtiDumpSuperInstanceFields(obj, objCb, objCb,
					     dc, &fieldIndex,
					     CVM_FALSE) == CVM_FALSE) {
		return CVM_FALSE;
	    }
	}
    }
    if (isArray && dc->callbacks->array_primitive_value_callback != NULL &&
	CVMisArrayOfAnyBasicType(objCb)) {
	if (jvmtiDumpArrayPrimitives(obj, objCb, objSize, dc) == CVM_FALSE) {
	    return CVM_FALSE;
	}
    }
    return CVM_TRUE;
}

static void
jvmtiDumpGlobalRoot(CVMObject **ref, void *data)
{
    CVMJvmtiDumpContext *dc = (CVMJvmtiDumpContext*)data;
    CVMObjectICell *icell = (CVMObjectICell *) ref;
    CVMObject *obj;

    if ((icell == NULL) || CVMID_icellIsNull(icell)) {
        return;
    }
    obj = *ref;
    jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_JNI_GLOBAL, NULL, NULL,
			   obj, dc, -1, CVM_TRUE);
}

static void
jvmtiDumpLocalRoot(CVMObject **ref, void *data)
{
    CVMJvmtiDumpContext *dc = (CVMJvmtiDumpContext*)data;
    CVMObjectICell *icell = (CVMObjectICell *) ref;
    CVMObject *obj;

    if ((icell == NULL) || CVMID_icellIsNull(icell)) {
        return;
    }
    obj = *ref;
    jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_OTHER, NULL, NULL,
			   obj, dc, -1, CVM_TRUE);
 }

static void
jvmtiDumpGlobalClass(CVMExecEnv *ee, CVMClassBlock *cb, void *data)
{
    CVMJvmtiDumpContext *dc = (CVMJvmtiDumpContext*)data;
    if (CVMcbClassLoader(cb) == NULL) {
	if (CVMcbJavaInstance(cb) != NULL) {
	    CVMObject *obj =
		CVMjvmtiGetICellDirect(ee, CVMcbJavaInstance(cb));
	    jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_SYSTEM_CLASS,
				   NULL, NULL, obj, dc, -1, CVM_TRUE);
	}
    }
}

static void
jvmtiDumpJNILocal(CVMObject **ref, void *data)
{
    CVMJvmtiDumpContext *dc = (CVMJvmtiDumpContext*)data;
    CVMObjectICell *icell = (CVMObjectICell *) ref;
    CVMObject *obj;

    if ((icell == NULL) || CVMID_icellIsNull(icell)) {
        return;
    }
    obj = *ref;
    jvmtiStackRefCallback(JVMTI_HEAP_REFERENCE_JNI_LOCAL,
			  obj, dc, -1);
}

static CVMBool
jvmtiScanJavaFrame(CVMExecEnv *frameEE, CVMFrame *frame,
		   CVMStackChunk *chunk, CVMJvmtiDumpContext *dc)
{

    CVMMethodBlock* mb;
    CVMJavaMethodDescriptor* jmd;

    CVMStackVal32*   scanPtr;
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

    stackmaps = CVMstackmapFind(frameEE, mb);

    /* A previous pass ensures that the stackmaps are indeed generated. */
    CVMassert(stackmaps != NULL);

    smEntry = CVMgetStackmapEntry(frameEE, frame, jmd, stackmaps,
                               &missingStackmapOK);
    CVMassert((smEntry != NULL) || missingStackmapOK);
    if (smEntry == NULL) {
	return CVM_TRUE;
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
		if (jvmtiStackRefCallback(JVMTI_HEAP_REFERENCE_STACK_LOCAL,
					  *slot, dc, i) == CVM_FALSE) {
		    return CVM_FALSE;
		}
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
    return CVM_TRUE;
}

CVMBool
CVMjvmtiScanRoots(CVMExecEnv *ee, CVMJvmtiDumpContext *dc)
{
    CVMObjectICell *icell;
    CVMObject *obj;

    if (CVMgcEnsureStackmapsForRootScans(ee)) {
	CVMGCOptions gcOpts = {
	    /* isUpdatingObjectPointers */ CVM_FALSE,
	    /* discoverWeakReferences   */ CVM_FALSE,
	    /* isProfilingPass          */ CVM_TRUE
	};

	/* Scan JNI_GLOBAL */
	CVMstackScanRoots(ee, &CVMglobals.globalRoots, jvmtiDumpGlobalRoot,
			  (void*)dc, &gcOpts);

	/* preloaded classes */
	/* 
	 * Dump java_lang_Class first so other runtime classes can find 
	 * its tag
	 */
	jvmtiDumpGlobalClass(ee, CVMsystemClass(java_lang_Class), (void*)dc);
	CVMclassIterateAllClasses(ee, jvmtiDumpGlobalClass, (void*)dc);
	/* threads */
        CVM_WALK_ALL_THREADS(ee, currentEE, {
		icell = CVMcurrentThreadICell(currentEE);
		if (!CVMID_icellIsNull(icell)) {
		    CVMObject *obj =
			CVMjvmtiGetICellDirect(CVMgetEE(), icell);
		    if (jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_THREAD,
					       NULL, NULL, obj, dc,
					       -1, CVM_TRUE) == CVM_FALSE) {
			return CVM_FALSE;
		    }
		}
		/* Scan JNI_LOCAL roots */
		CVMstackScanRoots(ee, &currentEE->localRootsStack,
				  jvmtiDumpLocalRoot,
				  (void*)dc, &gcOpts);
	    });
    }
    /* other */
    {
	CVMObjMonitor *mon = CVMglobals.objLocksBound;
	
	while (mon != NULL) {
	    CVMassert(mon->state != CVM_OBJMON_FREE);
	    CVMassert(mon->obj != NULL);
	    obj = mon->obj;
	    if (obj != NULL) {
		if (jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_MONITOR, NULL,
					   NULL, obj, dc,
					   -1, CVM_TRUE) == CVM_FALSE) {
		    return CVM_FALSE;
		}
	    }
	    mon = mon->next;
	}
    }
    /* Thread stacks */
    /*    CVM_WALK_ALL_THREADS(ee, currentEE, { */
    {
      CVMExecEnv *currentEE = CVMglobals.threadList;
      CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.threadLock));
      while (currentEE != NULL) {
	icell = CVMcurrentThreadICell(currentEE);
	if (!CVMID_icellIsNull(icell)) {
	    int frameCount = 0;
	    CVMstackWalkAllFrames(&currentEE->interpreterStack, {
		    dc->frameCount = frameCount;
		    dc->frame = frame;
		    dc->ee = currentEE;
		    if (CVMframeIsJNI(frame)) {
			CVMStackVal32* tos_ = (frame)->topOfStack;
			/*
			 * Traverse chunks from the last chunk of the frame
			 * to the first
			 */
			while (!CVMaddressInStackChunk(frame, chunk)) {
			    chunk = chunk->prev;
			    tos_ = chunk->end_data;
			}
			CVMwalkRefsInGCRootFrameSegment(tos_,
				((CVMFreelistFrame*)frame)->vals,
				jvmtiDumpJNILocal, dc, CVM_TRUE);
		    } else if (CVMframeIsJava(frame)) {
			if (!jvmtiScanJavaFrame(ee, frame, chunk, dc)) {
			    return CVM_FALSE;
			}
		    }
		});
	}
	currentEE = currentEE->nextEE;
      }
    }
    return CVM_TRUE;
}

