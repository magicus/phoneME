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
 * This file includes functions that need to be implemented by a particular
 * GC implementation. Common VM GC code will call these when necessary.
 */

#ifndef _INCLUDED_GC_IMPL_H
#define _INCLUDED_GC_IMPL_H

/*
 * This file is generated from the GC choice given at build time.
 * We include it first so that we can allow the specific implementation
 * to override some defaults specified by the common implementation.
 */
#include "generated/javavm/include/gc_config.h"

#include "javavm/include/gc_common.h"

#include "javavm/include/clib.h"

/*
 * Initialize GC global state
 */
extern void
CVMgcimplInitGlobalState(CVMGCGlobalState* globalState);

/*
 * Initialize a heap of at least 'minBytes' bytes, and at most 'maxBytes'
 * bytes.  minIsUnspecified and maxIsUnspecified (if TRUE) indicates that
 * the user did not explicitly specify a min and/or max value respectively,
 * and that the default is being passed in here.
 */
extern CVMBool
CVMgcimplInitHeap(CVMGCGlobalState* globalState,
		  CVMUint32 startBytes,
		  CVMUint32 minBytes, 
		  CVMUint32 maxBytes,
		  CVMBool startIsUnspecified,
		  CVMBool minIsUnspecified,
		  CVMBool maxIsUnspecified);

/*
 * Allocate uninitialized heap object of size numBytes
 */
extern CVMObject*
CVMgcimplAllocObject(CVMExecEnv* ee, CVMUint32 numBytes);

/*
 * Allocate uninitialized heap object of size numBytes after a GC
 * has been tried.
 */
extern CVMObject*
CVMgcimplRetryAllocationAfterGC(CVMExecEnv* ee, CVMUint32 numBytes);

/*
 * This routine is called by the common GC code after all locks are
 * obtained, and threads are stopped at GC-safe points. It's the
 * routine that needs a snapshot of the world while all threads are
 * stopped (typically at least a root scan).
 *
 * The goal to free is 'numBytes' bytes.
 */
void
CVMgcimplDoGC(CVMExecEnv* ee, CVMUint32 numBytes);

#ifdef CVM_JVMPI
/* Purpose: Posts the JVMPI_EVENT_ARENA_NEW events for the GC specific
            implementation. */
/* NOTE: This function is necessary to compensate for the fact that
         this event has already transpired by the time that JVMPI is
         initialized. */
void CVMgcimplPostJVMPIArenaNewEvent(void);

/* Purpose: Posts the JVMPI_EVENT_ARENA_DELETE events for the GC specific
            implementation. */
void CVMgcimplPostJVMPIArenaDeleteEvent(void);

/* Purpose: Gets the arena ID for the specified object. */
/* Returns: The requested ArenaID if found, else returns 0. */
CVMUint32 CVMgcimplGetArenaID(CVMObject *obj);

/* Purpose: Gets the number of objects allocated in the heap. */
CVMUint32 CVMgcimplGetObjectCount(CVMExecEnv *ee);

#endif /* CVM_JVMPI */

/*
 * Return the number of bytes free in the heap. 
 */
CVMUint32
CVMgcimplFreeMemory(CVMExecEnv* ee);

/*
 * Return the amount of total memory in the heap, in bytes.
 */
CVMUint32
CVMgcimplTotalMemory(CVMExecEnv* ee);

/*
 * Destroy GC global state
 */
extern void
CVMgcimplDestroyGlobalState(CVMGCGlobalState* globalState);

/*
 * Destroy heap
 */
extern CVMBool
CVMgcimplDestroyHeap(CVMGCGlobalState* globalState);


/*
 * The time of the last full GC of the heap
 */
extern CVMInt64
CVMgcimplTimeOfLastMajorGC();

/*
 * Heap iterator support from the GC implementation
 */
extern CVMBool
CVMgcimplIterateHeap(CVMExecEnv* ee, CVMObjectCallbackFunc cback, void* data);

#if defined(CVM_DEBUG) || defined(CVM_INSPECTOR)
/* Dumps info about the configuration of the GC. */
void CVMgcimplDumpSysInfo();
#endif

/* Sample read and write barriers. Put these in the gc_config.h file
   for a given GC implementation to get verbose output on when the
   barriers are hit. */

/*#define VERBOSE_BARRIERS */

#ifdef VERBOSE_BARRIERS
#define CVMgcimplReadBarrierByte(objRef, fieldLoc_)			 \
    CVMtraceBarriers(("BARRIER: Read of byte 0x%x[%d] (0x%x) -> 0x%x\n", \
		     objRef, fieldLoc_ - (CVMJavaByte*)objRef,		 \
		     fieldLoc_, *(fieldLoc_)));

#define CVMgcimplReadBarrierShort(objRef, fieldLoc_)			  \
    CVMtraceBarriers(("BARRIER: Read of short 0x%x[%d] (0x%x) -> 0x%x\n", \
		     objRef, fieldLoc_ - (CVMJavaShort*)objRef,		  \
		     fieldLoc_, *(fieldLoc_)));

#define CVMgcimplReadBarrierInt(objRef, fieldLoc_)			\
    CVMtraceBarriers(("BARRIER: Read of int 0x%x[%d] (0x%x) -> 0x%x\n",	\
		     objRef, fieldLoc_ - (CVMJavaInt*)objRef,		\
		     fieldLoc_, *(fieldLoc_)));

#define CVMgcimplReadBarrier64(objRef, fieldLoc_)			\
    CVMtraceBarriers(("BARRIER: Read of 64-bit 0x%x[%d] (0x%x) "	\
		      "-> (0x%x,0x%x)\n",				\
		      objRef, fieldLoc_ - (CVMJavaVal32*)objRef,	\
		      fieldLoc_, fieldLoc_[0].i, fieldLoc_[1].i));

#define CVMgcimplReadBarrierRef(objRef, fieldLoc_)			\
    CVMtraceBarriers(("BARRIER: Read of Ref 0x%x[%d] (0x%x) -> 0x%x\n",	\
		      objRef, fieldLoc_ - (CVMObject**)objRef,		\
		      fieldLoc_, *(fieldLoc_)));

#define CVMgcimplWriteBarrierByte(objRef, fieldLoc_, rhs)		  \
    CVMtraceBarriers(("BARRIER: Write to byte 0x%x[%d] (0x%x) <- 0x%x\n", \
		      objRef, fieldLoc_ - (CVMJavaByte*)objRef,		  \
		      fieldLoc_, rhs));

#define CVMgcimplWriteBarrierShort(objRef, fieldLoc_, rhs)		   \
    CVMtraceBarriers(("BARRIER: Write to short 0x%x[%d] (0x%x) <- 0x%x\n", \
		      objRef, fieldLoc_ - (CVMJavaShort*)objRef,	   \
		      fieldLoc_, rhs));

#define CVMgcimplWriteBarrierInt(objRef, fieldLoc_, rhs)		 \
    CVMtraceBarriers(("BARRIER: Write to int 0x%x[%d] (0x%x) <- 0x%x\n", \
		      objRef, fieldLoc_ - (CVMJavaInt*)objRef,		 \
		      fieldLoc_, rhs));

#define CVMgcimplWriteBarrier64(objRef, fieldLoc_, location)		\
    CVMtraceBarriers(("BARRIER: Write of 64-bit 0x%x[%d] (0x%x) "	\
		      "<- (0x%x,0x%x)\n",				\
		      objRef, fieldLoc_ - (CVMJavaVal32*)objRef,	\
		      fieldLoc_, location[0].i, location[1].i));

#define CVMgcimplWriteBarrierRef(objRef, fieldLoc_, rhs)		 \
    CVMtraceBarriers(("BARRIER: Write to Ref 0x%x[%d] (0x%x) <- 0x%x\n", \
		      objRef, fieldLoc_ - (CVMObject**)objRef,		 \
		      fieldLoc_, rhs));

#define CVMgcimplReadBarrierBool(objRef, fieldLoc_)			 \
    CVMtraceBarriers(("BARRIER: Read of bool 0x%x[%d] (0x%x) -> 0x%x\n", \
		      objRef, fieldLoc_ - (CVMJavaBool*)objRef,		 \
		      fieldLoc_, *(fieldLoc_)));

#define CVMgcimplReadBarrierChar(objRef, fieldLoc_)			 \
    CVMtraceBarriers(("BARRIER: Read of char 0x%x[%d] (0x%x) -> 0x%x\n", \
		      objRef, fieldLoc_ - (CVMJavaChar*)objRef,		 \
		      fieldLoc_, *(fieldLoc_)));

#define CVMgcimplReadBarrierFloat(objRef, fieldLoc_)			\
    CVMtraceBarriers(("BARRIER: Read of float 0x%x[%d] (0x%x) -> %f\n",	\
		      objRef, fieldLoc_ - (CVMJavaFloat*)objRef,	\
		      fieldLoc_, *(fieldLoc_)));

#define CVMgcimplWriteBarrierBool(objRef, fieldLoc_, rhs)		  \
    CVMtraceBarriers(("BARRIER: Write to bool 0x%x[%d] (0x%x) <- 0x%x\n", \
		      objRef, fieldLoc_ - (CVMJavaBool*)objRef,		  \
		      fieldLoc_, rhs));

#define CVMgcimplWriteBarrierChar(objRef, fieldLoc_, rhs)		  \
    CVMtraceBarriers(("BARRIER: Write to char 0x%x[%d] (0x%x) <- 0x%x\n", \
		      objRef, fieldLoc_ - (CVMJavaChar*)objRef,		  \
		      fieldLoc_, rhs));

#define CVMgcimplWriteBarrierFloat(objRef, fieldLoc_, rhs)		  \
    CVMtraceBarriers(("BARRIER: Write to float 0x%x[%d] (0x%x) <- %f\n",  \
		      objRef, fieldLoc_ - (CVMJavaFloat*)objRef,	  \
		      fieldLoc_, rhs));

#endif /* VERBOSE_BARRIERS */

#endif /* _INCLUDED_GC_IMPL_H */


