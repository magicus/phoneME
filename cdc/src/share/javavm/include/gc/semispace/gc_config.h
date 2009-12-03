/*
 * @(#)gc_config.h	1.14 06/10/10
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
 * This file includes the configuration of the semi-space copying
 * collector 
 */

#ifndef _INCLUDED_SEMISPACE_GC_CONFIG_H
#define _INCLUDED_SEMISPACE_GC_CONFIG_H

#include "javavm/include/gc/gc_impl.h"
#include "javavm/include/gc/semispace/semispace.h"

#ifdef CVM_TEST_GC
typedef enum {
    R_BARRIER_BYTE=0,
    R_BARRIER_BOOLEAN,
    R_BARRIER_SHORT,
    R_BARRIER_CHAR,
    R_BARRIER_INT,
    R_BARRIER_FLOAT,
    R_BARRIER_REF,
    R_BARRIER_64,
    W_BARRIER_BYTE,
    W_BARRIER_BOOLEAN,
    W_BARRIER_SHORT,
    W_BARRIER_CHAR,
    W_BARRIER_INT,
    W_BARRIER_FLOAT,
    W_BARRIER_REF,
    W_BARRIER_64
} CVMBarrierType;

#define CVMgcimplReadBarrierByte(directObj, slotAddr) \
        CVMgetEE()->barrier = R_BARRIER_BYTE;
#define CVMgcimplReadBarrierBoolean(directObj, slotAddr) \
        CVMgetEE()->barrier = R_BARRIER_BOOLEAN;
#define CVMgcimplReadBarrierShort(directObj, slotAddr) \
        CVMgetEE()->barrier = R_BARRIER_SHORT;
#define CVMgcimplReadBarrierChar(directObj, slotAddr) \
        CVMgetEE()->barrier = R_BARRIER_CHAR;
#define CVMgcimplReadBarrierInt(directObj, slotAddr) \
        CVMgetEE()->barrier = R_BARRIER_INT;
#define CVMgcimplReadBarrierFloat(directObj, slotAddr) \
        CVMgetEE()->barrier = R_BARRIER_FLOAT;
#define CVMgcimplReadBarrierRef(directObj, slotAddr) \
        CVMgetEE()->barrier = R_BARRIER_REF;
#define CVMgcimplReadBarrier64(directObj, slotAddr) \
        CVMgetEE()->barrier = R_BARRIER_64;

#define CVMgcimplWriteBarrierByte(directObj, slotAddr, rhsValue) \
        CVMgetEE()->barrier = W_BARRIER_BYTE;
#define CVMgcimplWriteBarrierBoolean(directObj, slotAddr, rhsValue) \
        CVMgetEE()->barrier = W_BARRIER_BOOLEAN;
#define CVMgcimplWriteBarrierShort(directObj, slotAddr, rhsValue) \
        CVMgetEE()->barrier = W_BARRIER_SHORT;
#define CVMgcimplWriteBarrierChar(directObj, slotAddr, rhsValue) \
        CVMgetEE()->barrier = W_BARRIER_CHAR;
#define CVMgcimplWriteBarrierInt(directObj, slotAddr, rhsValue) \
        CVMgetEE()->barrier = W_BARRIER_INT;
#define CVMgcimplWriteBarrierFloat(directObj, slotAddr, rhsValue) \
        CVMgetEE()->barrier = W_BARRIER_FLOAT;
#define CVMgcimplWriteBarrierRef(directObj, slotAddr, rhsValue) \
        CVMgetEE()->barrier = W_BARRIER_REF;
#define CVMgcimplWriteBarrier64(directObj, slotAddr, location_) \
        CVMgetEE()->barrier = W_BARRIER_64;
#endif

/* 
 * Global state specific to GC 
 */
struct CVMGCGlobalState {
    CVMUint32  heapSize;
    CVMSsHeap* heap;
};

/*
 * an example override of an array copy routine.
 */
#if OVERRIDE_BASIC_ARRAY_COPY
#define CVMgcimplArrayCopyInt(srcArr, srcIdx, dstArr, dstIdx, len) \
    memmove((void*)((dstArr)->elems + dstIdx), \
	    (const void *)((srcArr)->elems + srcIdx), \
	    (len) * sizeof(CVMJavaInt))
#endif

#define CVM_GC_SEMISPACE 111
#define CVM_GCCHOICE CVM_GC_SEMISPACE

#endif /* _INCLUDED_SEMISPACE_GC_CONFIG_H */
