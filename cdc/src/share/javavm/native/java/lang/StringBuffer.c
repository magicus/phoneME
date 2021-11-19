/*
 * @(#)StringBuffer.c	1.11 04/12/05
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
 * In SE6, the implementations of these methods have all moved out of
 * StringBuffer and into its superclass AbstractStringBuilder.
 */
#ifndef JAVASE

#include "javavm/export/jvm.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/common_exceptions.h"

#include "generated/offsets/java_lang_StringBuffer.h"

#define JAVA_LANG_INTEGER_MAX_VALUE (0x7fffffff)

#undef FIELD_READ_COUNT
#define FIELD_READ_COUNT(obj, val) \
    CVMD_fieldReadInt(obj,         \
        CVMoffsetOfjava_lang_StringBuffer_count, val)

#undef FIELD_READ_VALUE
#define FIELD_READ_VALUE(obj, val) \
    CVMD_fieldReadRef(obj,         \
        CVMoffsetOfjava_lang_StringBuffer_value, val)

#undef FIELD_READ_SHARED
#define FIELD_READ_SHARED(obj, val) \
    CVMD_fieldReadInt(obj,          \
        CVMoffsetOfjava_lang_StringBuffer_shared, val)

#undef FIELD_WRITE_VALUE
#define FIELD_WRITE_VALUE(obj, val)   \
    CVMD_fieldWriteRef(obj,                 \
        CVMoffsetOfjava_lang_StringBuffer_value, val);

#undef FIELD_WRITE_COUNT
#define FIELD_WRITE_COUNT(obj, val) \
    CVMD_fieldWriteInt(obj,         \
        CVMoffsetOfjava_lang_StringBuffer_count, val)

#undef FIELD_WRITE_SHARED
#define FIELD_WRITE_SHARED(obj, val)   \
    CVMD_fieldWriteInt(obj,                 \
        CVMoffsetOfjava_lang_StringBuffer_shared, val);

/*
 * helper routine for "expandCapacity(int minCapacity)" 
 */
static void
CVMexpandCapacityHelper(CVMExecEnv* ee, CVMObjectICell *thisICell,
                        CVMJavaInt minimumCapacity)
{
    CVMObject* thisObject;
    
    CVMJavaInt count;
    CVMJavaBoolean shared;
    CVMObject* valueObject;

    CVMJavaInt valueLength;
    CVMJavaInt newCapacity;
    CVMArrayOfChar* valueDirect;
    CVMArrayOfChar* newValue;

    thisObject = CVMID_icellDirect(ee, thisICell);
    FIELD_READ_COUNT(thisObject, count);
    FIELD_READ_SHARED(thisObject, shared);
    FIELD_READ_VALUE(thisObject, valueObject);
    valueDirect = (CVMArrayOfChar*)valueObject;
    valueLength = CVMD_arrayGetLength(valueDirect);
    
    newCapacity = (valueLength + 1) * 2;
    if (newCapacity < 0) {
        newCapacity = JAVA_LANG_INTEGER_MAX_VALUE;
    } else if (minimumCapacity > newCapacity) {
        newCapacity = minimumCapacity;
    }

    /* Do not allow stale pointers */
    thisObject = NULL;
    
    /* Allocate an array with the new capacity */ 
    /* NOTE: CVMgcAllocNewArray may block and initiate GC. */
    newValue = (CVMArrayOfChar *)CVMgcAllocNewArray(
        ee, CVM_T_CHAR, 
        (CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_CHAR], 
        newCapacity);
    
    if (newValue == NULL) {
        CVMthrowOutOfMemoryError(ee, NULL);
        return;
    }
    if ((count > valueLength) || (count < 0)) {
        CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
        return; 
    }

    /* GC might have initiated so get thisObject and value once again */
    thisObject = CVMID_icellDirect(ee, thisICell);
    FIELD_READ_VALUE(thisObject, valueObject);
    valueDirect = (CVMArrayOfChar*)valueObject;
    CVMD_arrayCopyChar(valueDirect, 0, newValue, 0, count);

    FIELD_WRITE_VALUE(thisObject,(CVMObject*)newValue);
    FIELD_WRITE_SHARED(thisObject, CVM_FALSE); 
}

/*
 * helper routine for "append(...)" functions
 */
static CVMObjectICell*  
CVMappendHelper(CVMExecEnv* ee, CVMObjectICell *thisICell, 
                CVMArrayOfCharICell* strICell, CVMJavaInt offset, 
                CVMJavaInt len) 
{
    CVMObjectICell *result = thisICell;
    CVMJavaInt newcount;
    CVMJavaInt valueLength;
    CVMJavaInt strLength;

    CVMJavaInt count;

    /* The direct pointers in this function */
    CVMObject* thisObject;
    CVMObject* valueObject;
    CVMArrayOfChar* valueDirect;
    CVMArrayOfChar* strDirect;

    /* 
     * NOTE: This is a synchronized method. The interpreter 
     * loop does not take care of synchronization for CNI 
     * method. So we should do it here.
     */
    if (!CVMfastTryLock(ee, CVMID_icellDirect(ee, thisICell))) {
        /* May become GC-safe */
	/* Prevent staleness of direct pointers */
	strDirect   = NULL;
	valueDirect = NULL;
	valueObject = NULL;
	thisObject  = NULL;
        if (!CVMobjectLock(ee, thisICell)) {
            CVMthrowOutOfMemoryError(ee, NULL);
            return NULL;
        }
    }

    thisObject = CVMID_icellDirect(ee, thisICell);
 
    FIELD_READ_COUNT(thisObject, count);
    FIELD_READ_VALUE(thisObject, valueObject);
    valueDirect = (CVMArrayOfChar* )valueObject;
    valueLength = CVMD_arrayGetLength(valueDirect);
    newcount = count + len;

    if (newcount > valueLength) {
        /* May become GC-safe */
	/* Prevent staleness of direct pointers */
	strDirect   = NULL;
	valueDirect = NULL;
	valueObject = NULL;
	thisObject  = NULL;
        CVMexpandCapacityHelper(ee, thisICell, newcount);
	/* CVMexpandCapacityHelper might have changed thisObject & value */
	thisObject = CVMID_icellDirect(ee, thisICell);
        if (CVMexceptionOccurred(ee)) {
	    result = NULL;
	    goto unlock;
        }
	FIELD_READ_VALUE(thisObject, valueObject);
	valueDirect = (CVMArrayOfChar* )valueObject;
	valueLength = CVMD_arrayGetLength(valueDirect);
    }
    
    strDirect = CVMID_icellDirect(ee, strICell);
    strLength = CVMD_arrayGetLength(strDirect);
    
    if (((count + len) > valueLength) || ((offset + len) > strLength) ||
        (offset < 0) || (len < 0) || (count < 0) )
    {
	/* invalidate */
	strDirect   = NULL; valueDirect = NULL;
	valueObject = NULL; thisObject  = NULL;
        CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
	/* recache */
	thisObject = CVMID_icellDirect(ee, thisICell);
	result = NULL;
	goto unlock;
    }
  
    CVMD_arrayCopyChar(strDirect, offset, valueDirect, count, len);

    FIELD_WRITE_COUNT(thisObject, newcount);

unlock:
    /* Unlock the object before return */
    if (!CVMfastTryUnlock(ee, thisObject)) {
	/* Prevent staleness of direct pointers */
	/* We are about to throw an exception and return,
	   so we are not going to use these again. Make sure */
	strDirect   = NULL;
	valueDirect = NULL;
	valueObject = NULL;
	thisObject  = NULL;
        if (!CVMobjectUnlock(ee, thisICell)) {
	    /* How can this happen? */
            CVMclearLocalException(ee);
            CVMthrowIllegalMonitorStateException(ee,
                                                 "current thread not owner");
            return NULL;
        }
    }
    return result;
}

/*
 * Class:	java/lang/StringBuffer
 * Method:	append
 * Signature:	([C)Ljava/lang/StringBuffer;
 */
CNIEXPORT CNIResultCode
CNIjava_lang_StringBuffer_append___3C(CVMExecEnv* ee, CVMStackVal32 *arguments,
                           CVMMethodBlock **p_mb)
{
    CVMObjectICell* thisICell;
    CVMObject* strObject;
    CVMArrayOfCharICell* strICell;
    CVMArrayOfChar* strDirect; 

    CVMJavaInt strLength;
    CVMObjectICell* result;
    
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    thisICell = &arguments[0].j.r;
    strICell  = (CVMArrayOfCharICell*)&arguments[1].j.r;
    
    if (CVMID_icellIsNull(strICell)) {
        CVMthrowNullPointerException(ee, NULL);
        return CNI_EXCEPTION;
    }
    strObject = CVMID_icellDirect(ee, (CVMObjectICell*)strICell);
    strDirect = (CVMArrayOfChar* )strObject;
    strLength = CVMD_arrayGetLength(strDirect);
    result = CVMappendHelper(ee, thisICell, strICell, 0, strLength);

    if (result == NULL) {
        return CNI_EXCEPTION;
    }

    /* 'this' is replaced by return value */ 
    CVMID_icellAssignDirect(ee, thisICell, result);

    return CNI_SINGLE;
}

/*
 * Class:	java/lang/StringBuffer
 * Method:	append
 * Signature:	([CII)Ljava/lang/StringBuffer;
 */
CNIEXPORT CNIResultCode
CNIjava_lang_StringBuffer_append___3CII(
    CVMExecEnv* ee, CVMStackVal32* arguments, CVMMethodBlock** p_mb)
{
    CVMObjectICell* thisICell;
    CVMArrayOfCharICell* strICell;
    CVMJavaInt offset = arguments[2].j.i;
    CVMJavaInt len = arguments[3].j.i;
    CVMObjectICell* result;
    
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    thisICell = &arguments[0].j.r;
    strICell  = (CVMArrayOfCharICell*)&arguments[1].j.r;
    
    if (CVMID_icellIsNull(strICell)) {
        CVMthrowNullPointerException(ee, NULL);
        return CNI_EXCEPTION;
    }
  
    result = CVMappendHelper(ee, thisICell, strICell, offset, len);
   
    if (result == NULL) {
        return CNI_EXCEPTION;
    }

    /* 'this' is replaced by return value */ 
    CVMID_icellAssignDirect(ee, thisICell, result);

    return CNI_SINGLE;
}

/*
 * Class:	java/lang/StringBuffer
 * Method:	expandCapacity
 * Signature:	(I)V
 */
CNIEXPORT CNIResultCode
CNIjava_lang_StringBuffer_expandCapacity(
    CVMExecEnv* ee, CVMStackVal32* arguments, CVMMethodBlock** p_mb)
{
    CVMObjectICell* thisICell = &arguments[0].j.r;
    CVMJavaInt minimumCapacity = arguments[1].j.i;
  
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    CVMexpandCapacityHelper(ee, thisICell, minimumCapacity);
    
    if (CVMexceptionOccurred(ee)) {
        return CNI_EXCEPTION;
    }
    
    return CNI_VOID;
}

#endif /* !JAVASE */
