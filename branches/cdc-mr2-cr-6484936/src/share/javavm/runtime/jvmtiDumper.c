/*
 * @(#)jvmtiDumper.c	1.0 07/01/16
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.	 
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
#include "generated/javavm/include/opcodes.h"
#include "generated/offsets/java_lang_String.h"
#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"
#include "javavm/export/jvmti.h"
#include "javavm/include/jvmti_jni.h"
#include "javavm/include/jvmtiEnv.h"
#include "javavm/include/jvmtiDumper.h"
#include "javavm/include/jvmtiCapabilities.h"


/* We assume that we can access anything since we lock down the 
 * whole world before dumping the heap
 */

#define CVMjvmtiGetICellDirect(ee_, icellPtr_) \
   CVMID_icellGetDirectWithAssertion(CVM_TRUE, icellPtr_)

#define CVMjvmtiSetICellDirect(ee_, icellPtr_, directObj_) \
   CVMID_icellSetDirectWithAssertion(CVM_TRUE, icellPtr_, directObj_)
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

/* Search fieldCb's direct superinterfaces for the field recursively */
static CVMInt32
jvmtiFindFieldStartingIndex(const CVMClassBlock* fieldCb)
{
    CVMClassBlock* targetCb;
    int i;
    int index = 0;

    /* C stack redzone check */
    if (!CVMCstackCheckSize(CVMgetEE(),
            CVM_REDZONE_CVMcpFindFieldInSuperInterfaces,
            "CVMcpFindFieldInSuperInterfaces", CVM_FALSE)) {
        return 0;
    }
    /* count direct superclasses */
    for (targetCb = CVMcbSuperclass(fieldCb);
	 targetCb != NULL;
	 targetCb = CVMcbSuperclass(targetCb)) {
	index += CVMcbFieldCount(targetCb);
    }

    /* count direct superinterfaces */
    for (i = 0; i < CVMcbImplementsCount(fieldCb); i++) {
        targetCb = CVMcbInterfacecb(fieldCb, i);
	/* count superinterface's superiface */
	index += jvmtiFindFieldStartingIndex(targetCb);
	index += CVMcbFieldCount(targetCb);
    }
    return index;
}


static TagNode *objectsByRef[HASH_SLOT_COUNT];

static TagNode **jvmtiTagGetTopNode(CVMObject *obj)
{

    CVMExecEnv *ee = CVMgetEE();
    int hashCode;
    TagNode **result;

    CVMassert(CVMD_isgcSafe(ee));

    hashCode = CVMobjectGetHashNoSet(ee, obj);

    JVMTI_LOCK(ee);
    if (hashCode == CVM_OBJECT_NO_HASH) {
	if (CVMobjectIsInROM(obj)) {
	    hashCode = (int)obj >> 2;
	} else {
	    JVMTI_UNLOCK(ee);
	    return NULL;
	}
    }
    if (hashCode < 0) {
	hashCode = -hashCode;
    }
    result = &objectsByRef[hashCode % HASH_SLOT_COUNT];
    JVMTI_UNLOCK(ee);
    return result;
}

TagNode * CVMjvmtiTagGetNode(CVMObject *obj)
{
    TagNode *node, *prev;
    TagNode **slot;
    CVMExecEnv *ee = CVMgetEE();

    CVMassert(CVMD_isgcSafe(ee));

    JVMTI_LOCK(ee);
    slot = jvmtiTagGetTopNode(obj);
    if (slot == NULL) {
	JVMTI_UNLOCK(ee);
	return NULL;
    }
    node = *slot;
    prev = NULL;
    while (node != NULL) {
	if (obj == CVMID_icellGetDirectWithAssertion(CVM_TRUE, node->ref)) {
	    break;
	}
	prev = node;
	node = node->next;
    }
    JVMTI_UNLOCK(ee);
    return node;
}

jvmtiError CVMjvmtiTagGetTagGC(CVMObject *obj, jlong *tag_ptr)
{

    TagNode *node;
    CVMExecEnv *ee = CVMgetEE();
    CVMassert(CVMD_isgcSafe(ee));

    *tag_ptr = 0L;
    JVMTI_LOCK(ee);
    node = CVMjvmtiTagGetNode(obj);
    if (node == NULL) {
	*tag_ptr = 0L;
    } else {
	*tag_ptr = node->tag;
    }
    JVMTI_UNLOCK(ee);
    return JVMTI_ERROR_NONE;
}

static jint hashRef(jobject ref) 
{
    jint hashCode = JVM_IHashCode(CVMexecEnv2JniEnv(CVMgetEE()), ref);
    if (hashCode < 0) {
	hashCode = -hashCode;
    }
    return hashCode % HASH_SLOT_COUNT;
}


int
CVMjvmtiGetObjectsWithTag(JNIEnv *env, const jlong *tags, jint tag_count,
			  jobject *obj_ptr, jlong *tag_ptr)
{
    int i, j;
    TagNode *node;
    int count = 0;
    CVMExecEnv *ee = CVMgetEE();

    JVMTI_LOCK(ee);
    for (i = 0; i < HASH_SLOT_COUNT; i++) {
	node = objectsByRef[i];
	while (node != NULL) {
	    for (j = 0; j < tag_count; j++) {
		if (node->tag == tags[i]) {
		    if (obj_ptr != NULL && tag_ptr != NULL) {
			obj_ptr[count] = (*env)->NewLocalRef(env, node->ref);
			tag_ptr[count] = node->tag;
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

    TagNode *node, *next, *prev;
    int i;
    CVMExecEnv *ee = CVMgetEE();

    JVMTI_LOCK(ee);
    for (i = 0; i < HASH_SLOT_COUNT; i++) {
	node = objectsByRef[i];
	prev = NULL;
	while (node != NULL) {
	    next = node->next;
	    if (node->ref == NULL || CVMID_icellIsNull(node->ref)) {
		if (prev == NULL) {
		    objectsByRef[i] = node->next;
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
CVMjvmtiTagGetTag(jvmtiEnv* jvmtienv,
	       jobject object,
	       jlong* tag_ptr) {

    TagNode *node;
    TagNode *prev;
    JNIEnv *env;
    CVMExecEnv *ee = CVMgetEE();
    jint slot;

    JVMTI_LOCK(ee);

    slot = hashRef(object);

    env = CVMexecEnv2JniEnv(ee);
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
	*tag_ptr = 0L;
    } else {
	*tag_ptr = node->tag;
    }
    JVMTI_UNLOCK(ee);
    return JVMTI_ERROR_NONE;
}

jvmtiError
CVMjvmtiTagSetTag(jvmtiEnv *jvmtienv, jobject object, jlong tag)
{
    JNIEnv *env;
    jint slot;
    TagNode *node, *prev;
    CVMExecEnv *ee = CVMgetEE();

    /*
     * Add to reference hashtable 
     */
    JVMTI_LOCK(ee);
    slot = hashRef(object);
    node = objectsByRef[slot];
    prev = NULL;

    env = CVMexecEnv2JniEnv(ee);
    while (node != NULL) {
	if (!CVMID_icellIsNull(node->ref) &&
	    (*env)->IsSameObject(env, object, node->ref)) {
	    break;
	}
	prev = node;
	node = node->next;
    }
    if (node == NULL) {
	if (tag == 0L) {
	    /* clearing tag on non-existent object */
	    JVMTI_UNLOCK(ee);
	    return JVMTI_ERROR_INVALID_OBJECT;
	}
	CVMjvmtiAllocate(sizeof(TagNode), (unsigned char **)&node);
	node->ref = NULL;
    }
    if (tag == 0L) {
	/* clearing tag, deallocate it */
	if (prev == NULL) {
	    objectsByRef[slot] = node->next;
	} else {
	    prev->next = node->next;
	}
	(*env)->DeleteWeakGlobalRef(env, node->ref);
	CVMjvmtiDeallocate((unsigned char *)node);
    } else {
	jobject objRef = (*env)->NewWeakGlobalRef(env, object);
	if (node->ref != NULL) {
	    (*env)->DeleteWeakGlobalRef(env, node->ref);
	} else {
	    node->next = objectsByRef[slot];
	    objectsByRef[slot] = node;
	}
	node->ref = objRef;
	node->tag = tag;
    }
    JVMTI_UNLOCK(ee);
    return JVMTI_ERROR_NONE;
}

void
CVMjvmtiPostCallbackUpdateTag(CVMObject *obj, TagNode *node, jlong tag)
{
    TagNode **topNode;
    CVMObjectICell *icell;
    CVMExecEnv *ee = CVMgetEE();
    JNIEnv *env = CVMexecEnv2JniEnv(ee);

    /* When we callback into the agent we pass a pointer to the tag.
     * The agent may change the actual value of the tag and/or create a
     * a new tag if one 
     */
    JVMTI_LOCK(ee);
    if (node == NULL) {
	if (tag != 0L) {
	    /* callback changed tag from 0 to X */
	    topNode = jvmtiTagGetTopNode(obj);
	    if (topNode == NULL) {
		/* nothing we can do, just return */
		JVMTI_UNLOCK(ee);
		return ;
	    }
	    CVMjvmtiAllocate(sizeof(TagNode), (unsigned char **)&node);
	    node->tag = tag;
	    icell = CVMjniCreateLocalRef(ee);
	    CVMD_gcUnsafeExec(ee, {
		    CVMID_icellSetDirect(ee, icell, obj);
		});
	    node->ref = (*env)->NewWeakGlobalRef(env, icell);
	    node->next = *topNode;
	    *topNode = node;
	}
    } else {
	/* object was tagged, may have been cleared or tag changed */
	if (tag == 0) {
	    TagNode *tmp;
	    (*env)->DeleteWeakGlobalRef(env, node->ref);
	    topNode = jvmtiTagGetTopNode(obj);
	    if (topNode == NULL) {
		/* most unlikely, but nevertheless, return */
		JVMTI_UNLOCK(ee);
		return;
	    }
	    if (*topNode == node) {
		*topNode = node->next;
	    } else {
		tmp = *topNode;
		while (tmp != NULL) {
		    if (tmp->next == node) {
			tmp->next = node->next; /* nuke the node */
			break;
		    }
		    tmp = tmp->next;
		}
	    }
	} else {
	    node->tag = tag;
	}
    }
    JVMTI_UNLOCK(ee);
}

static CVMBool
jvmtiCheckForVisit(CVMObject *obj)
{
    jvmtiVisitStackPush(obj);
    return CVM_TRUE;
}
static CVMBool
jvmtiIsFilteredByHeapFilter(jlong obj_tag, jlong obj_klass_tag,
			   jint heap_filter)
{
    /* apply the heap filter */
    if (obj_tag != 0) {
	/* filter out tagged objects */
	if (heap_filter & JVMTI_HEAP_FILTER_TAGGED) {
	    return CVM_TRUE;
	}
    } else {
	/* filter out untagged objects */
	if (heap_filter & JVMTI_HEAP_FILTER_UNTAGGED) {
	    return CVM_TRUE;
	}
    }
    if (obj_klass_tag != 0) {
	/* filter out objects with tagged classes */
	if (heap_filter & JVMTI_HEAP_FILTER_CLASS_TAGGED) {
	    return CVM_TRUE;
	}
    } else {
	/* filter out objects with untagged classes. */
	if (heap_filter & JVMTI_HEAP_FILTER_CLASS_UNTAGGED) {
	    return CVM_TRUE;
	}
    }
    return CVM_FALSE;
}

static CVMBool
jvmtiIsFilteredByKlassFilter(CVMClassBlock * obj_cb, jclass klass)
{
    if (klass != NULL) {
	if (obj_cb != CVMjvmtiObject2Class(CVMgetEE(), klass)) {
	    return CVM_TRUE;
	}
    }
    return CVM_FALSE;
}

/* invoke the object reference callback to report a reference */
static CVMBool
jvmtiObjectRefCallback(jvmtiHeapReferenceKind ref_kind,
		    CVMObject *referrer,
		    const CVMClassBlock *referrer_cb,
		    CVMObject *obj, 
		    jvmtiDumpContext *dc,
		    jint index,
		    CVMBool isRoot) 
{
    jint len;
    int res;
    jlong referrer_klass_tag = 0, referrer_tag = 0;
    jlong obj_tag, obj_klass_tag;
    jsize obj_size;
    jvmtiHeapReferenceCallback cb = dc->callbacks->heap_reference_callback;
    /* field index is only valid field in reference_info */
    jvmtiHeapReferenceInfo ref_info, *ref_info_ptr;
    CVMClassBlock *obj_cb;
    TagNode *obj_node, *referrer_node = NULL;

    ref_info_ptr = &ref_info;

    /* check that callback is provider */
    if (cb == NULL) {
	return jvmtiCheckForVisit(obj);
    }
    if (!isRoot) {
	referrer_node = CVMjvmtiTagGetNode(referrer);
	referrer_tag = (referrer_node == NULL ? 0L : referrer_node->tag);
	CVMjvmtiTagGetTagGC(CVMjvmtiGetICellDirect(CVMgetEE(),
		         CVMcbJavaInstance(referrer_cb)),
			 &referrer_klass_tag);
    }
    obj_cb = CVMobjectGetClass(obj);
    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(obj_cb, dc->klass)) {
	return jvmtiCheckForVisit(obj);
    }
    
    obj_node = CVMjvmtiTagGetNode(obj);
    obj_tag = obj_node == NULL ? 0L : obj_node->tag;
    CVMjvmtiTagGetTagGC(CVMjvmtiGetICellDirect(CVMgetEE(),
		        CVMcbJavaInstance(obj_cb)),
			&obj_klass_tag);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(obj_tag, obj_klass_tag, dc->heap_filter)) {
	return jvmtiCheckForVisit(obj);
    }
    obj_size = CVMobjectSizeGivenClass(obj, obj_cb);
    switch(ref_kind) {
    case JVMTI_HEAP_REFERENCE_FIELD:
    case JVMTI_HEAP_REFERENCE_STATIC_FIELD:
	/* field index is only valid field in reference_info */
	ref_info.field.index = index;
	break;
    case JVMTI_HEAP_REFERENCE_ARRAY_ELEMENT:
	ref_info.array.index = index;
	break;
    case JVMTI_HEAP_REFERENCE_CONSTANT_POOL:
	ref_info.constant_pool.index = index;
	break;
    default:
	ref_info_ptr = NULL;
	break;
    }
    /* for arrays we need the length, otherwise -1 */
    if (CVMisArrayClass(obj_cb)) {
	len = CVMD_arrayGetLength((CVMArrayOfAnyType *) obj);
    } else {
	len = -1;
    }

    /* invoke the callback */
    res = (*cb)(ref_kind, 
		ref_info_ptr,
		obj_klass_tag,
		referrer_klass_tag,
		obj_size,
		&obj_tag,
		isRoot ? NULL : &referrer_tag,
		len,
		(void*)dc->user_data);

    CVMjvmtiPostCallbackUpdateTag(obj, obj_node, obj_tag);
    if (!isRoot) {
	CVMjvmtiPostCallbackUpdateTag(referrer, referrer_node, referrer_tag);
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
jvmtiStackRefCallback(jvmtiHeapReferenceKind ref_kind,
		   CVMObject *obj, 
		   jvmtiDumpContext *dc,
		   jint index)
{
    jint len;
    int res;
    jlong obj_tag, obj_klass_tag, thread_tag;
    jsize obj_size;
    jvmtiHeapReferenceCallback cb = dc->callbacks->heap_reference_callback;
    /* field index is only valid field in reference_info */
    jvmtiHeapReferenceInfo ref_info;
    CVMClassBlock *obj_cb;
    CVMExecEnv *ee = dc->ee; 
    TagNode *obj_node;

    /* check that callback is provider */
    if (cb == NULL) {
	return jvmtiCheckForVisit(obj);
    }
    obj_cb = CVMobjectGetClass(obj);
    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(obj_cb, dc->klass)) {
	return jvmtiCheckForVisit(obj);
    }
    
    obj_node = CVMjvmtiTagGetNode(obj);
    obj_tag = obj_node == NULL ? 0L : obj_node->tag;
    CVMjvmtiTagGetTagGC(CVMjvmtiGetICellDirect(ee, CVMcbJavaInstance(obj_cb)),
			&obj_klass_tag);
    CVMjvmtiTagGetTagGC(CVMjvmtiGetICellDirect(ee, CVMcurrentThreadICell(ee)),
			&thread_tag);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(obj_tag, obj_klass_tag, dc->heap_filter)) {
	return jvmtiCheckForVisit(obj);
    }
    obj_size = CVMobjectSizeGivenClass(obj, obj_cb);
    switch(ref_kind) {
    case JVMTI_HEAP_REFERENCE_STACK_LOCAL:
	ref_info.stack_local.thread_tag = thread_tag;
	ref_info.stack_local.thread_id = ee->threadID;
	ref_info.stack_local.depth = dc->frame_count;
	ref_info.stack_local.method = CVMframeGetMb(dc->frame);
	ref_info.stack_local.location = (jlong)((int)CVMframePc(dc->frame));
	ref_info.stack_local.slot = index;
	break;
    case JVMTI_HEAP_REFERENCE_JNI_LOCAL:
	ref_info.stack_local.thread_tag = thread_tag;
	ref_info.stack_local.thread_id = ee->threadID;
	ref_info.stack_local.depth = dc->frame_count;
	ref_info.stack_local.method = CVMframeGetMb(dc->frame);
	break;
    default:
	break;
    }

    len = -1;
    /* for arrays we need the length, otherwise -1 */
    if (CVMisArrayClass(obj_cb)) {
	len = CVMD_arrayGetLength((CVMArrayOfAnyType *) obj);
    }

    /* invoke the callback */
    res = (*cb)(ref_kind, 
		&ref_info,
		obj_klass_tag,
		0,
		obj_size,
		&obj_tag,
		NULL,
		len,
		(void*)dc->user_data);

    CVMjvmtiPostCallbackUpdateTag(obj, obj_node, obj_tag);
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
jvmtiPrimitiveRefCallback(jvmtiHeapReferenceKind ref_kind,
		       CVMObject *obj, 
		       jvmtiDumpContext *dc,
		       jint index,
		       int CVMtype,
		       jvalue v)
{
    int res;
    jlong obj_tag, obj_klass_tag;
    jvmtiPrimitiveFieldCallback cb = dc->callbacks->primitive_field_callback;
    /* field index is only valid field in reference_info */
    jvmtiHeapReferenceInfo ref_info;
    CVMClassBlock *obj_cb;
    TagNode *obj_node;

    obj_cb = CVMobjectGetClass(obj);
    if (obj_cb == CVMsystemClass(java_lang_Class)) {
	obj_cb = CVMsystemClass(java_lang_Class);
    }

    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(obj_cb, dc->klass)) {
	return jvmtiCheckForVisit(obj);
    }
    
    obj_node = CVMjvmtiTagGetNode(obj);
    obj_tag = obj_node == NULL ? 0L : obj_node->tag;
    CVMjvmtiTagGetTagGC(CVMjvmtiGetICellDirect(CVMgetEE(),
					 CVMcbJavaInstance(obj_cb)),
		  &obj_klass_tag);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(obj_tag, obj_klass_tag, dc->heap_filter)) {
	return jvmtiCheckForVisit(obj);
    }

    ref_info.field.index = index;

    /* invoke the callback */
    res = (*cb)(ref_kind, 
		&ref_info,
		obj_klass_tag,
		&obj_tag,
		v,
		CVMtype2jvmtiPrimitiveType[CVMtype],
		(void*)dc->user_data);

    CVMjvmtiPostCallbackUpdateTag(obj, obj_node, obj_tag);
    if (res & JVMTI_VISIT_ABORT) {
	return CVM_FALSE;
    }
    return CVM_TRUE;
}

/* invoke the array primitive callback to report a primitive array */
static CVMBool
jvmtiArrayPrimCallback(CVMObject *obj, 
		    CVMClassBlock *obj_cb,
		    void *elements,
		    jvmtiDumpContext *dc,
		    jint count,
		    int jvmtiType)
{
    int res;
    jlong obj_tag, obj_klass_tag;
    jvmtiArrayPrimitiveValueCallback cb = dc->callbacks->array_primitive_value_callback;
    /* field index is only valid field in reference_info */
    TagNode *obj_node;
    jlong obj_size;

    if (obj_cb == CVMsystemClass(java_lang_Class)) {
	obj_cb = CVMsystemClass(java_lang_Class);
    }

    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(obj_cb, dc->klass)) {
	return jvmtiCheckForVisit(obj);
    }
    
    obj_node = CVMjvmtiTagGetNode(obj);
    obj_tag = obj_node == NULL ? 0L : obj_node->tag;
    CVMjvmtiTagGetTagGC(CVMjvmtiGetICellDirect(CVMgetEE(),
					 CVMcbJavaInstance(obj_cb)),
		  &obj_klass_tag);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(obj_tag, obj_klass_tag, dc->heap_filter)) {
	return jvmtiCheckForVisit(obj);
    }
    obj_size = CVMobjectSizeGivenClass(obj, obj_cb);

    /* invoke the callback */
    res = (*cb)(obj_klass_tag,
		obj_size,
		&obj_tag,
		count,
		jvmtiType,
		elements,
		(void*)dc->user_data);

    CVMjvmtiPostCallbackUpdateTag(obj, obj_node, obj_tag);
    if (res & JVMTI_VISIT_ABORT) {
	return CVM_FALSE;
    }
    return CVM_TRUE;
}


static CVMBool
jvmtiStringPrimCallback(CVMObject *obj, 
			jvmtiDumpContext *dc)
{
    int res;
    jlong obj_tag, obj_klass_tag;
    jvmtiStringPrimitiveValueCallback cb = dc->callbacks->string_primitive_value_callback;
    /* field index is only valid field in reference_info */
    TagNode *obj_node;
    jlong obj_size;
    CVMInt32 len, offset;
    CVMObject *value;
    CVMClassBlock *obj_cb;

    obj_cb = CVMobjectGetClass(obj);
    CVMassert(obj_cb == CVMsystemClass(java_lang_String));

    CVMD_fieldReadRef(obj, CVMoffsetOfjava_lang_String_value, value);
    CVMD_fieldReadInt(obj, CVMoffsetOfjava_lang_String_count, len);
    CVMD_fieldReadInt(obj, CVMoffsetOfjava_lang_String_offset, offset);
    if (value == NULL) {
	return CVM_TRUE;
    }

    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(obj_cb, dc->klass)) {
	return jvmtiCheckForVisit(obj);
    }
    
    obj_node = CVMjvmtiTagGetNode(obj);
    obj_tag = obj_node == NULL ? 0L : obj_node->tag;
    CVMjvmtiTagGetTagGC(CVMjvmtiGetICellDirect(CVMgetEE(),
					 CVMcbJavaInstance(obj_cb)),
		  &obj_klass_tag);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(obj_tag, obj_klass_tag, dc->heap_filter)) {
	return jvmtiCheckForVisit(obj);
    }
    obj_size = CVMobjectSizeGivenClass(obj, obj_cb);

    /* invoke the callback */
    res = (*cb)(obj_klass_tag,
		obj_size,
		&obj_tag,
		(CVMJavaChar*)CVMDprivate_arrayElemLoc(((CVMArrayOfChar *)value),0),
		len,
		(void*)dc->user_data);

    CVMjvmtiPostCallbackUpdateTag(obj, obj_node, obj_tag);
    if (res & JVMTI_VISIT_ABORT) {
	return CVM_FALSE;
    }
    return CVM_TRUE;
}

static CVMInt32 fieldIndex;

static CVMBool
jvmtiDumpInstanceFields(CVMObject *obj, const CVMClassBlock *cb,
		CVMClassBlock *referrerCb, jvmtiDumpContext *dc,
		CVMBool do_refs)
{
    CVMInt32 i;

    /* NOTE: We're interating through the fields in reverse order.  This is
       intentional because JVMTI list fields in reverse order than that
       which is declared in the ClassFile.  This is especially evident in
       the definition of the JVMTI_GC_INSTANCE_DUMP record where the fields
       of the self class is to preceed that of its super class and so on so
       forth.
    */
    for (i = 0; i < CVMcbFieldCount(cb); i++, fieldIndex++) {
	CVMFieldBlock *fb = CVMcbFieldSlot(cb, i);
	CVMClassTypeID fieldType;
	if (CVMfbIs(fb, STATIC)) {
	    continue;
	}
	fieldType = CVMtypeidGetType(CVMfbNameAndTypeID(fb));
	if (do_refs && CVMtypeidFieldIsRef(fieldType)) {
	    CVMObject *value;
	    CVMD_fieldReadRef(obj, CVMfbOffset(fb), value);
	    if (value != NULL) {
		if (jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_FIELD, obj,
				    referrerCb, value, dc, fieldIndex,
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
	    }
#undef DUMP_VALUE
	    if (jvmtiPrimitiveRefCallback(JVMTI_HEAP_REFERENCE_FIELD,
					  obj, 
					  dc,
					  fieldIndex,
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
jvmtiDumpSuperInterfacesFields(CVMObject *obj,
			       CVMClassBlock *cb,
			       CVMClassBlock *referrerCb,
			       jvmtiDumpContext *dc, CVMBool do_refs)
{
    CVMClassBlock *targetCb;
    int i;
    for (i = 0; i < CVMcbImplementsCount(cb); i++) {
        targetCb = CVMcbInterfacecb(cb, i);
	if (jvmtiDumpSuperInterfacesFields(obj, targetCb, referrerCb, dc,
					   do_refs) == CVM_FALSE) {
	    return CVM_FALSE;
	}
	if (jvmtiDumpInstanceFields(obj, targetCb, referrerCb,
				    dc, do_refs) == CVM_FALSE) {
	    return CVM_FALSE;
	}
    }
    return CVM_TRUE;
}

static CVMBool
jvmtiDumpSuperInterfaces(CVMObject *obj,
			 CVMClassBlock *cb,
			 CVMClassBlock *referrerCb,
			 jvmtiDumpContext *dc, CVMBool do_refs)
{
    CVMClassBlock *targetCb;

    targetCb = CVMcbSuperclass(cb);

    if (targetCb != NULL) {
	if (jvmtiDumpSuperInterfaces(obj, targetCb, referrerCb,
				     dc, do_refs) == CVM_FALSE) {
	    return CVM_FALSE;
	}
    }
    if (jvmtiDumpSuperInterfacesFields(obj, cb, referrerCb, dc,
				       do_refs) == CVM_FALSE) {
	return CVM_FALSE;
    }
    return CVM_TRUE;
}

static CVMBool
jvmtiDumpSuperInstanceFields(CVMObject *obj,
			     CVMClassBlock *cb,
			     CVMClassBlock *referrerCb,
			     jvmtiDumpContext *dc,
			     CVMBool do_refs) 
{
    CVMClassBlock *targetCb = CVMcbSuperclass(cb);
    CVMBool result;
    if (targetCb != NULL) {
	result = jvmtiDumpSuperInstanceFields(obj, targetCb,
					      referrerCb, dc, do_refs);
	if (result == CVM_FALSE) {
	    return result;
	}
    }
    return jvmtiDumpInstanceFields(obj, cb, referrerCb, dc, do_refs);
}

/* Purpose: Dump an object instance for Level 1 or 2 dumps. */
/* Returns: CVM_TRUE if successful. */
static CVMBool
jvmtiDumpInstance(CVMObject *obj, jvmtiDumpContext *dc)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMClassBlock *cb = CVMobjectGetClass(obj);

    ee = ee; /* for non-debug compile */
    if (jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_CLASS, obj, cb,
		(void *)CVMjvmtiGetICellDirect(ee, CVMcbJavaInstance(cb)),
			       dc, -1, CVM_FALSE) == CVM_FALSE) {
	return CVM_FALSE;
    }


    fieldIndex = 0;

    /* Dump the field values: */
    if (jvmtiDumpSuperInterfaces(obj, cb, cb, dc, CVM_TRUE) == CVM_FALSE) {
	return CVM_FALSE;
    }
    return jvmtiDumpSuperInstanceFields(obj, cb, cb, dc, CVM_TRUE);
}

 
static  CVMBool
jvmtiDumpStaticFields(CVMObject *clazz, const CVMClassBlock *cb,
		      jvmtiDumpContext *dc, CVMBool do_refs)
{
    CVMExecEnv *ee = CVMgetEE();
    jint index;
    CVMBool result;

    /* dump static field values */
    index = jvmtiFindFieldStartingIndex(cb);
    if ((CVMcbFieldCount(cb) > 0) && (CVMcbFields(cb) != NULL)) {
        CVMInt32 i;

        for (i = 0; i < CVMcbFieldCount(cb); i++, index++) {
            const CVMFieldBlock *fb = CVMcbFieldSlot(cb, i);
            CVMClassTypeID fieldType;

            if (!CVMfbIs(fb, STATIC)) {
                continue;
            }
            fieldType = CVMtypeidGetType(CVMfbNameAndTypeID(fb));
            if (do_refs && CVMtypeidFieldIsRef(fieldType)) {
                CVMObject *value =
		    (CVMObject*)CVMjvmtiGetICellDirect(ee, &CVMfbStaticField(ee, fb).r);
		if (value != NULL) {
		    result = jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_STATIC_FIELD,
					clazz, cb, value, dc, index,
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
#define DUMP_VALUE2(type_, result_) {				   \
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
                }
#undef DUMP_VALUE
#undef DUMP_VALUE2

		result = jvmtiPrimitiveRefCallback(JVMTI_HEAP_REFERENCE_STATIC_FIELD,
				       clazz, 
				       dc,
				       index,
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
jvmtiDumpClass(CVMObject *clazz, jvmtiDumpContext *dc)
{
    CVMExecEnv *ee = CVMgetEE();
    const CVMClassBlock *cb = CVMjvmtiClassInstance2ClassBlock(ee, clazz);
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
	if (superCB != NULL && !CVMsystemClass(java_lang_Object)) {
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
	if (classloader != NULL) {
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
	if (protectionDomain != NULL) {
	    result = jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_PROTECTION_DOMAIN,
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
		   result = jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_CONSTANT_POOL,
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
		    result = jvmtiObjectRefCallback(JVMTI_HEAP_REFERENCE_CONSTANT_POOL,
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
			 jvmtiDumpContext *dc)
{
    
    void *elements;
    CVMBool result;

#undef DUMP_ELEMENTS
#define DUMP_ELEMENTS(type_, ptr_type_, jvmtitype_) {		\
    jint i;								\
    CVMjvmtiAllocate(size * sizeof(ptr_type_), (unsigned char**)&elements); \
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
    case CVM_T_INT:	    DUMP_ELEMENTS(Int, jint, INT);             break;
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
jvmtiDumpArray(CVMObject *arrObj, jvmtiDumpContext *dc)
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
CVMjvmtiDumpObject(CVMObject *obj, jvmtiDumpContext *dc)
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
CVMjvmtiIterateDoObject(CVMObject *obj, CVMClassBlock *obj_cb,
				CVMUint32 obj_size, void *data)
{
    jvmtiDumpContext *dc = (jvmtiDumpContext*)data;
    TagNode *obj_node;
    jlong obj_klass_tag, obj_tag;
    jint len = -1;
    int res;
    CVMBool is_array;

    /* apply class filter */
    if (jvmtiIsFilteredByKlassFilter(obj_cb, dc->klass)) {
	return CVM_TRUE;
    }
    
    obj_node = CVMjvmtiTagGetNode(obj);
    obj_tag = obj_node == NULL ? 0L : obj_node->tag;
    CVMjvmtiTagGetTagGC(CVMjvmtiGetICellDirect(CVMgetEE(),
		       CVMcbJavaInstance(obj_cb)), &obj_klass_tag);
    /* apply tag filter */
    if (jvmtiIsFilteredByHeapFilter(obj_tag, obj_klass_tag, dc->heap_filter)) {
	return CVM_TRUE;
    }
    if ((is_array = CVMisArrayClass(obj_cb))) {
	len = CVMD_arrayGetLength((CVMArrayOfAnyType *) obj);
    }
    if (dc->callbacks->heap_iteration_callback != NULL) {
	jvmtiHeapIterationCallback cb = dc->callbacks->heap_iteration_callback;
	res = (*cb)(obj_klass_tag,
		    obj_size,
		    &obj_tag,
		    len,
		    data);
	if (res & JVMTI_VISIT_ABORT) {
	    return CVM_FALSE;
	}
    }
 
    if (dc->callbacks->primitive_field_callback != NULL &&
	!is_array) {
	if (obj_cb == CVMsystemClass(java_lang_Class)) {
	    if (jvmtiDumpStaticFields(obj, obj_cb, dc, CVM_FALSE) == CVM_FALSE) {
		return CVM_FALSE;
	    }
	} else {
	    fieldIndex = 0;
 	    /* Dump the field values: */
	    if (jvmtiDumpSuperInterfaces(obj, obj_cb, obj_cb, dc,
					 CVM_FALSE) == CVM_FALSE) {
		return CVM_FALSE;
	    }
	    if (jvmtiDumpSuperInstanceFields(obj, obj_cb, obj_cb, dc,
					     CVM_FALSE) == CVM_FALSE) {
		return CVM_FALSE;
	    }
	}
    }
    if (is_array && dc->callbacks->array_primitive_value_callback != NULL &&
	CVMisArrayOfAnyBasicType(obj_cb)) {
	if (jvmtiDumpArrayPrimitives(obj, obj_cb, obj_size, dc) == CVM_FALSE) {
	    return CVM_FALSE;
	}
    }
    return CVM_TRUE;
}

static void
jvmtiDumpGlobalRoot(CVMObject **ref, void *data)
{
    jvmtiDumpContext *dc = (jvmtiDumpContext*)data;
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
    jvmtiDumpContext *dc = (jvmtiDumpContext*)data;
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
    jvmtiDumpContext *dc = (jvmtiDumpContext*)data;
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
    jvmtiDumpContext *dc = (jvmtiDumpContext*)data;
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
			    CVMStackChunk *chunk, jvmtiDumpContext *dc)
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

    smEntry = getStackmapEntry(frameEE, frame, jmd, stackmaps,
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
CVMjvmtiScanRoots(CVMExecEnv *ee, jvmtiDumpContext *dc)
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
		    dc->frame_count = frameCount;
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
			if (jvmtiScanJavaFrame(ee, frame, chunk, dc) == CVM_FALSE) {
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

