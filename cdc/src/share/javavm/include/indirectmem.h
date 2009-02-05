/*
 * @(#)indirectmem.h	1.28 06/10/10
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
 * This file is the definition of the indirect memory interface. All
 * Java object and array accesses from GC-safe code must go through
 * this interface in order to remain GC-safe.  
 */

#ifndef _INCLUDED_INDIRECTMEM_H
#define _INCLUDED_INDIRECTMEM_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/directmem.h"

/*
 * DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER
 *
 * CVMID_icellGetDirectWithAssertion and CVMID_icellSetDirectWithAssertion is
 * to be used to access object pointers directly, but allows the caller to
 * specify the assertion that will be checked.  These macros are ONLY to be
 * used to build other macros like CVMID_icellDirect and CVMID_icellSetDirect,
 * and ONLY by those who know what they are doing.  Be very careful!
 */
#define CVMID_icellGetDirectWithAssertion(assertionCondition_, icellPtr_) \
    (*(CVMassert(assertionCondition_),                                    \
       &(icellPtr_)->ref_DONT_ACCESS_DIRECTLY))

#define CVMID_icellSetDirectWithAssertion(assertionCondition_,   \
                                          icellPtr_, directObj_) \
    (CVMassert(assertionCondition_),                             \
     (icellPtr_)->ref_DONT_ACCESS_DIRECTLY = (directObj_))

/*
 * DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER
 *
 * Don't use CVMID_icellDirect and CVMID_icellSetDirect unless you are
 * in a GC-unsafe region and you know what you are doing.  
 */
#define CVMID_icellDirect(ee_, icellPtr_) \
    CVMID_icellGetDirectWithAssertion(CVMD_isgcUnsafe(ee_), icellPtr_)

#define CVMID_icellSetDirect(ee_, icellPtr_, directObj_) \
    CVMID_icellSetDirectWithAssertion(CVMD_isgcUnsafe(ee_), \
                                      icellPtr_, directObj_)

#define CVMID_icellAssignDirect(ee_, dstICellPtr_, srcICellPtr_)\
    CVMID_icellSetDirect((ee_), (dstICellPtr_),			\
	CVMID_icellDirect((ee_), (srcICellPtr_)))

/*
 * REGNAD REGNAD REGNAD REGNAD REGNAD REGNAD REGNAD REGNAD REGNAD
 */

/*
 * The ICell that contains a null reference
 */
extern const CVMObjectICell CVMID_nullICell;

#define CVMID_NonNullICellPtrFor(icellPtr) \
    ((icellPtr == NULL) ? (CVMObjectICell*)&CVMID_nullICell : icellPtr)

/*
 * GC-safe operations on CVMObjectICell*'s
 */

#define CVMID_allocNewInstance(ee_, cb_, dstICellPtr_)			\
    CVMD_gcUnsafeExec(ee_, {						\
	CVMObject* newInstance_ = CVMgcAllocNewInstance(ee_, cb_);	\
	CVMID_icellSetDirect(ee, dstICellPtr_, newInstance_);		\
    })

#define CVMID_allocNewArray(ee_, typeCode_, arrayCb_, 			\
			     arrayLen_, dstICellPtr_)			\
    CVMD_gcUnsafeExec(ee_, {						\
	CVMArrayOfAnyType* newArray_ = 					\
            CVMgcAllocNewArray(ee_, typeCode_, arrayCb_, arrayLen_);	\
	CVMID_icellSetDirect(ee_, dstICellPtr_, (CVMObject*)newArray_);	\
    })

#define CVMID_objectGetClass(ee_, objICellPtr_, dstCb_)			\
    CVMD_gcUnsafeExec(ee_, {						\
	CVMObject* directObj_ = CVMID_icellDirect(ee_, objICellPtr_);	\
        dstCb_ = CVMobjectGetClass(directObj_);				\
    })

/*
 * Assign object pointed to from srcICellPtr_ to the ICell referent of
 * dstICellPtr_ 
 */
#define CVMID_icellAssign(ee_, dstICellPtr_, srcICellPtr_)		\
    CVMD_gcUnsafeExec(ee_, {						\
	CVMID_icellAssignDirect((ee_), (dstICellPtr_), (srcICellPtr_));	\
    })

#define CVMID_icellSetNull(icellPtr_)					\
     (icellPtr_)->ref_DONT_ACCESS_DIRECTLY = 0;

#define CVMID_icellIsNull(icellPtr_)				\
     ((icellPtr_)->ref_DONT_ACCESS_DIRECTLY == 0)

#define CVMID_icellSameObject(ee_, icellPtr1_, icellPtr2_, res_)	\
    CVMD_gcUnsafeExec(ee_, {						\
	CVMObject* directObj1_ = CVMID_icellDirect(ee_, icellPtr1_);	\
	CVMObject* directObj2_ = CVMID_icellDirect(ee_, icellPtr2_);	\
        res_ = (directObj1_ == directObj2_);				\
    })

#define CVMID_icellSameObjectNullCheck(ee_, icellPtr1_, icellPtr2_, res_)   \
     if (icellPtr1_ == NULL || icellPtr2_ == NULL) {			    \
	 res_ = (icellPtr1_ == icellPtr2_);				    \
     } else {							   	    \
	 CVMID_icellSameObject(ee_, icellPtr1_, icellPtr2_, res_);	    \
     }								   

/*
 * GC-safe object read and write operations
 */
#define CVMID_fieldRead32(ee_, o_, off_, res_)			\
    CVMD_gcUnsafeExec(ee_, {					\
        CVMObject* directObj = CVMID_icellDirect((ee_), (o_));	\
	CVMD_fieldRead32(directObj, off_, res_);		\
    })

#define CVMID_fieldWrite32(ee_, o_, off_, item_)		\
    CVMD_gcUnsafeExec(ee_, {					\
        CVMObject* directObj = CVMID_icellDirect((ee_), (o_));	\
	CVMD_fieldWrite32(directObj, off_, item_);		\
    })

#define CVMID_fieldRead64(ee_, o_, off_, location_)		\
    CVMD_gcUnsafeExec(ee_, {					\
        CVMObject* directObj = CVMID_icellDirect((ee_), (o_));	\
	CVMD_fieldRead64(directObj, off_, location_);		\
    })

#define CVMID_fieldWrite64(ee_, o_, off_, location_)		\
    CVMD_gcUnsafeExec(ee_, {					\
        CVMObject* directObj = CVMID_icellDirect((ee_), (o_));	\
	CVMD_fieldWrite64(directObj, off_, location_);		\
    })

#define CVMID_fieldReadRef(ee_, o_, off_, resICellPtr_)			\
    CVMD_gcUnsafeExec(ee_, {						\
        CVMObject* directObj = CVMID_icellDirect((ee_), (o_));		\
        CVMObject* item_;						\
	CVMD_fieldReadRef(directObj, off_, item_);			\
        CVMID_icellSetDirect((ee_), (resICellPtr_), item_);		\
    })

#define CVMID_fieldWriteRef(ee_, o_, off_, srcICellPtr_)		\
    CVMD_gcUnsafeExec(ee_, {						\
        CVMObject* directObj = CVMID_icellDirect((ee_), (o_));		\
        CVMObject* item_     = CVMID_icellDirect((ee_), (srcICellPtr_));\
	CVMD_fieldWriteRef(directObj, off_, item_);			\
    })

#define CVMID_fieldReadInt(ee_, o_, off_, item_)		\
    CVMIDprivate_fieldRW(Read, Int, (ee_), (o_), (off_), (item_))

#define CVMID_fieldWriteInt(ee_, o_, off_, item_)		\
    CVMIDprivate_fieldRW(Write, Int, (ee_), (o_), (off_), (item_))

#define CVMID_fieldReadFloat(ee_, o_, off_, item_)	\
    CVMIDprivate_fieldRW(Read, Float, (ee_), (o_), (off_), (item_))

#define CVMID_fieldWriteFloat(ee_, o_, off_, item_)	\
    CVMIDprivate_fieldRW(Write, Float, (ee_), (o_), (off_), (item_))

#define CVMID_fieldReadLong(ee_, o_, off_, val64_)	\
    CVMIDprivate_fieldRW(Read, Long, (ee_), (o_), (off_), (val64_))

#define CVMID_fieldWriteLong(ee_, o_, off_, val64_)	\
    CVMIDprivate_fieldRW(Write, Long, (ee_), (o_), (off_), (val64_))

#define CVMID_fieldReadDouble(ee_, o_, off_, val64_)	\
    CVMIDprivate_fieldRW(Read, Double, (ee_), (o_), (off_), (val64_))

#define CVMID_fieldWriteDouble(ee_, o_, off_, val64_)	\
    CVMIDprivate_fieldRW(Write, Double, (ee_), (o_), (off_), (val64_))

#define CVMID_fieldReadAddr(ee_, o_, off_, item_)	\
    CVMIDprivate_fieldRW(Read, Addr, (ee_), (o_), (off_), (item_))

#define CVMID_fieldWriteAddr(ee_, o_, off_, item_)	\
    CVMIDprivate_fieldRW(Write, Addr, (ee_), (o_), (off_), (item_))

#define CVMIDprivate_fieldRW(rw, t, ee_, o_, off_, item_)	\
    CVMD_gcUnsafeExec(ee_, {					\
        CVMObject* directObj = CVMID_icellDirect((ee_), (o_));	\
	CVMD_field##rw##t(directObj, off_, item_);		\
    })


/*
 * GC-safe array read and write operations
 */

/*
 * Get the length of an array
 */
#define CVMID_arrayGetLength(ee_, arr_, len_)			\
    CVMD_gcUnsafeExec(ee_, {					\
        len_ = CVMD_arrayGetLength(CVMID_icellDirect((ee_), (arr_)));	\
    })

/*
 * Reference reads and writes from arrays
 */
#define CVMID_arrayReadRef(ee_, arr_, off_, resICell_)          	\
    CVMD_gcUnsafeExec(ee_, {						\
        CVMObject* readRef_;						\
	CVMD_arrayReadRef(CVMID_icellDirect((ee_), (arr_)),		\
			  off_, readRef_);				\
        CVMID_icellSetDirect((ee_), resICell_, readRef_);		\
    })

#define CVMID_arrayWriteRef(ee_, arr_, off_, srcICell_)          	\
    CVMD_gcUnsafeExec(ee_, {						\
        CVMObject* srcRef_ = CVMID_icellDirect((ee_), srcICell_);	\
	CVMD_arrayWriteRef(CVMID_icellDirect((ee_), (arr_)),		\
			   off_, srcRef_);				\
    })

/*
 * Non-reference typed array reads and writes
 */
#define CVMIDprivate_arrayRW(rw_, t_, ee_, arr_, off_, item_)		\
    CVMD_gcUnsafeExec(ee_, {						\
	CVMD_array##rw_##t_(CVMID_icellDirect((ee_), (arr_)),		\
			    off_, item_);				\
    })

#define CVMID_arrayReadInt(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Read, Int, ee_, arr_, off_, item_)

#define CVMID_arrayWriteInt(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Write, Int, ee_, arr_, off_, item_)

#define CVMID_arrayReadFloat(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Read, Float, ee_, arr_, off_, item_)

#define CVMID_arrayWriteFloat(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Write, Float, ee_, arr_, off_, item_)

#define CVMID_arrayReadBoolean(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Read, Boolean, ee_, arr_, off_, item_)

#define CVMID_arrayWriteBoolean(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Write, Boolean, ee_, arr_, off_, item_)

#define CVMID_arrayReadByte(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Read, Byte, ee_, arr_, off_, item_)

#define CVMID_arrayWriteByte(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Write, Byte, ee_, arr_, off_, item_)

#define CVMID_arrayReadShort(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Read, Short, ee_, arr_, off_, item_)

#define CVMID_arrayWriteShort(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Write, Short, ee_, arr_, off_, item_)

#define CVMID_arrayReadChar(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Read, Char, ee_, arr_, off_, item_)

#define CVMID_arrayWriteChar(ee_, arr_, off_, item_) \
    CVMIDprivate_arrayRW(Write, Char, ee_, arr_, off_, item_)


/*
 * Reading longs and doubles from arrays
 */
#define CVMID_arrayReadLong(ee_, arr_, off_, val64_)		\
    CVMIDprivate_arrayRW(Read, Long, (ee_), (arr_), (off_), (val64_))

#define CVMID_arrayWriteLong(ee_, arr_, off_, val64_)		\
    CVMIDprivate_arrayRW(Write, Long, (ee_), (arr_), (off_), (val64_))

#define CVMID_arrayReadDouble(ee_, arr_, off_, val64_)		\
    CVMIDprivate_arrayRW(Read, Double, (ee_), (arr_), (off_), (val64_))

#define CVMID_arrayWriteDouble(ee_, arr_, off_, val64_)		\
    CVMIDprivate_arrayRW(Write, Double, (ee_), (arr_), (off_), (val64_))

#endif /* _INCLUDED_INDIRECTMEM_H */
