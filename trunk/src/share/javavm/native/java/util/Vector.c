/*
 * @(#)Vector.c	1.14 06/10/10
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
#include "javavm/include/interpreter.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/common_exceptions.h"

#if 0
#include "generated/offsets/java_util_Vector.h"
#include "generated/offsets/java_util_AbstractList.h"
#endif

#if 0
#undef FIELD_READ_ELEMENTDATA
#define FIELD_READ_ELEMENTDATA(obj, val)         \
    CVMD_fieldReadRef(                           \
        obj,                                     \
        CVMoffsetOfjava_util_Vector_elementData, \
        val)

#undef FIELD_READ_ELEMENTCOUNT
#define FIELD_READ_ELEMENTCOUNT(obj, val)         \
    CVMD_fieldReadInt(                            \
        obj,                                      \
        CVMoffsetOfjava_util_Vector_elementCount, \
        val)

#undef FIELD_WRITE_ELEMENTDATA
#define FIELD_WRITE_ELEMENTDATA(obj, val)        \
    CVMD_fieldWriteRef(                          \
        obj,                                     \
        CVMoffsetOfjava_util_Vector_elementData, \
        val);

#undef FIELD_WRITE_ELEMENTCOUNT
#define FIELD_WRITE_ELEMENTCOUNT(obj, val)        \
    CVMD_fieldWriteInt(                           \
        obj,                                      \
        CVMoffsetOfjava_util_Vector_elementCount, \
        val);
#endif

#if 0
/* 
 * Helper routine that implements the unsynchronized 
 * semantics of ensureCapacity.
 * 
 * Note: This routine may block and initiate GC.
 */
static void
CVMensureCapacityHelper(CVMExecEnv* ee, CVMObjectICell *thisICell,
                        CVMJavaInt minCapacity)
{
    CVMObject *thisObject = CVMID_icellDirect(ee, thisICell);
    CVMObject *elementDataObject;
    CVMJavaInt oldCapacity;

    FIELD_READ_ELEMENTDATA(thisObject, elementDataObject);

    /* NULL check. */
    if (elementDataObject == NULL) {
        CVMthrowNullPointerException(ee, NULL);
        return;
    }
    
    oldCapacity = CVMD_arrayGetLength((CVMArrayOfRef *)elementDataObject);

    if (minCapacity > oldCapacity) {
        CVMArrayOfRef *newData;
        CVMJavaInt newCapacity;
        CVMJavaInt capacityIncrement;
        CVMJavaInt elementCount;

        /* Calculate the new capacity */
        CVMD_fieldReadInt(thisObject,
                          CVMoffsetOfjava_util_Vector_capacityIncrement,
                          capacityIncrement);
        newCapacity = (capacityIncrement > 0) ?
            (oldCapacity + capacityIncrement) : (oldCapacity * 2);
        if (newCapacity < minCapacity) {
            newCapacity = minCapacity;
        }

        /* Allocate an array with the new capacity */ 
        /* NOTE: CVMgcAllocNewArray may block and initiate GC. */
        newData = (CVMArrayOfRef *)CVMgcAllocNewArray(
            ee, CVM_T_CLASS, 
            (CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_CLASS], 
            newCapacity);

        if (newData == NULL) {
            CVMthrowOutOfMemoryError(ee, NULL);
            return;
        }

        /* 
         * Copy the contents from the old array to 
         * the new array, then set elementData to be
         * the new one. 
         */
        thisObject = CVMID_icellDirect(ee, thisICell);
        FIELD_READ_ELEMENTDATA(thisObject, elementDataObject);
        /* 
         * Get the number of existing elements. Can't trust
         * the number though.
         */
        FIELD_READ_ELEMENTCOUNT(thisObject, elementCount);
        if (elementCount > oldCapacity) {
            CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
            return; 
        }
        CVMD_arrayCopyRef((CVMArrayOfRef *)elementDataObject, 0, 
                          newData, 0, elementCount);
        FIELD_WRITE_ELEMENTDATA(thisObject, (CVMObject *)newData);
    }
}
#endif

#if 0
/*
 * Class:       java/util/Vector
 * Method:      ensureCapacityHelper
 * Signature:   (I)V
 *
 * This implements the unsynchronized semantics of ensureCapacity.
 * Synchronized methods in Vector class can internally call this
 * method for ensuring capacity without incurring the cost of an
 * extra synchronization.
 */
CNIEXPORT CNIResultCode
CNIjava_util_Vector_ensureCapacityHelper(CVMExecEnv* ee,
                                         CVMStackVal32 *arguments,
                                         CVMMethodBlock **p_mb)
{
    CVMObjectICell *thisICell = &arguments[0].j.r;
    CVMJavaInt minCapacity = arguments[1].j.i;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    /* Call helper routine, which may throw exception */
    CVMensureCapacityHelper(ee, thisICell, minCapacity);

    if (CVMexceptionOccurred(ee)) {
        return CNI_EXCEPTION;
    }

    return CNI_VOID;
}
#endif

#if 0
/*
 * Class:       java/util/Vector
 * Method:      elementAt
 * Signature:   (I)Ljava/lang/Object;
 */
CNIEXPORT CNIResultCode
CNIjava_util_Vector_elementAt(CVMExecEnv* ee, CVMStackVal32 *arguments,
                              CVMMethodBlock **p_mb)
{
    CVMObjectICell *thisICell = &arguments[0].j.r;
    CVMJavaInt index = arguments[1].j.i;

    CVMObject *thisObject;
  
    CVMJavaInt elementCount;
    CVMObject *elementDataObject;
    CVMObject *theElementObject;
    CVMArrayOfRef * elementData;

    CVMJavaInt elementDataLen;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    /* 
     * NOTE: This is a synchronized method. The interpreter 
     * loop does not take care of synchronization for CNI 
     * method. So we need to do it here.
     */
    if (!CVMfastTryLock(ee, CVMID_icellDirect(ee, thisICell))) {
        /* May become GC-safe */
        if (!CVMobjectLock(ee, thisICell)) {
            CVMthrowOutOfMemoryError(ee, NULL);
            return CNI_EXCEPTION;
        }
    }

    thisObject = CVMID_icellDirect(ee, thisICell);
    FIELD_READ_ELEMENTDATA(thisObject, elementDataObject);

    /* NULL check. */
    if (elementDataObject == NULL) {
        /* Unlock the object before return */
        if (!CVMfastTryUnlock(ee, thisObject)) {
            if (!CVMobjectUnlock(ee, thisICell)) {
                CVMthrowIllegalMonitorStateException(ee,
                    "current thread not owner");
                return CNI_EXCEPTION;
            }
        }

        CVMthrowNullPointerException(ee, NULL);
        return CNI_EXCEPTION; 
    }

    FIELD_READ_ELEMENTCOUNT(thisObject, elementCount);
    elementData = (CVMArrayOfRef *)elementDataObject;
  
    /* 
     * Check array bounds. We can't trust elementCount since elementData
     * and elementCount are protected fields. They can be reset in a
     * subclass.
     */
    elementDataLen = CVMD_arrayGetLength(elementData); 
    if (index >= elementCount || index >= elementDataLen) {
        /* Unlock the object before return */
        if (!CVMfastTryUnlock(ee, thisObject)) {
            if (!CVMobjectUnlock(ee, thisICell)) {
                CVMthrowIllegalMonitorStateException(ee,
                    "current thread not owner");
                return CNI_EXCEPTION;
            }
        }

        if (elementCount != elementDataLen) {
            elementCount = elementDataLen; 
        }
        CVMthrowArrayIndexOutOfBoundsException(ee, "%d >= %d",
                                               index, elementCount);
        return CNI_EXCEPTION;
    }else if (index < 0) {
        /* Unlock the object before return */
        if (!CVMfastTryUnlock(ee, thisObject)) {
            if (!CVMobjectUnlock(ee, thisICell)) {
                CVMthrowIllegalMonitorStateException(ee,
                    "current thread not owner");
                return CNI_EXCEPTION;
            }
        }

        CVMthrowArrayIndexOutOfBoundsException(ee, "%d < 0", index);
        return CNI_EXCEPTION;
    } 

    CVMD_arrayReadRef(elementData, index, theElementObject); 
    CVMID_icellSetDirect(ee, &arguments[0].j.r, theElementObject);

    /* Unlock the object before return */
    if (!CVMfastTryUnlock(ee, thisObject)) {
        if (!CVMobjectUnlock(ee, thisICell)) {
            CVMthrowIllegalMonitorStateException(ee,
                "current thread not owner");
            return CNI_EXCEPTION;
        }
    }

    return CNI_SINGLE; 
}
#endif

#if 0
/*
 * Class:       java/util/Vector
 * Method:      addElement
 * Signature:   (Ljava/lang/Object;)V
 */
CNIEXPORT CNIResultCode
CNIjava_util_Vector_addElement(CVMExecEnv* ee, CVMStackVal32 *arguments,
                               CVMMethodBlock **p_mb)
{
    CVMObjectICell *thisICell = &arguments[0].j.r;
    CVMObjectICell *objICell = &arguments[1].j.r;
    CVMObject *thisObject;
    CVMObject *obj;
    CVMObject *elementDataObject;
    CVMJavaInt modCount;
    CVMJavaInt elementCount;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    /* Note: This is a synchronized method */
    if (!CVMfastTryLock(ee, CVMID_icellDirect(ee, thisICell))) {
         /* CVMobjectLock may become GC-safe */
         if (!CVMobjectLock(ee, thisICell)) {
             CVMthrowOutOfMemoryError(ee, NULL);
             return CNI_EXCEPTION;
         }
    }

    thisObject = CVMID_icellDirect(ee, thisICell);

    /* Increment modCount */
    CVMD_fieldReadInt(thisObject,
                      CVMoffsetOfjava_util_AbstractList_modCount,
                      modCount);
    modCount ++;
    CVMD_fieldWriteInt(thisObject,
                       CVMoffsetOfjava_util_AbstractList_modCount,
                       modCount);

    /* Get elementCount */
    FIELD_READ_ELEMENTCOUNT(thisObject, elementCount);
    elementCount ++;

    /* 
     * Ensure capacity. May become GC-safe. 
     * May throw exception.
     */
    CVMensureCapacityHelper(ee, thisICell, elementCount);

    /* Check exception */
    if (CVMexceptionOccurred(ee)) {
        /* Unlock the object before return */
        if (!CVMfastTryUnlock(ee, CVMID_icellDirect(ee, thisICell))) {
            if (!CVMobjectUnlock(ee, thisICell)) {
                CVMthrowIllegalMonitorStateException(ee,
                    "current thread not owner");
            }
        }
        return CNI_EXCEPTION;
    }
    
    /* Add the new element */
    thisObject = CVMID_icellDirect(ee, thisICell);
    obj = CVMID_icellDirect(ee, objICell);

    /* 
     * We don't need to do NULL check here since CVMensureCapacityHelper
     * already did it for us.
     */
    FIELD_READ_ELEMENTDATA(thisObject, elementDataObject);
    CVMD_arrayWriteRef((CVMArrayOfRef *)elementDataObject,
                       elementCount - 1,
                       obj);

    /* Don't forget write back elementCount and elementData*/
    FIELD_WRITE_ELEMENTCOUNT(thisObject, elementCount);
    FIELD_WRITE_ELEMENTDATA(thisObject, elementDataObject);

    /* Unlock the object before return */
    if (!CVMfastTryUnlock(ee, thisObject)) {
        if (!CVMobjectUnlock(ee, thisICell)) {
            CVMthrowIllegalMonitorStateException(ee,
                "current thread not owner");
            return CNI_EXCEPTION;
        }
    }
 
    return CNI_VOID;
}

#endif
