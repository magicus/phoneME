/*
 * @(#)jitutils.h	1.71 06/10/10
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

#ifndef _INCLUDED_JITUTILS_H
#define _INCLUDED_JITUTILS_H

#include "javavm/include/assert.h"
#include "javavm/include/utils.h"
#include "javavm/include/jit/jit_defs.h"

/*************************************
 * Error handling
 *************************************/

#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)
#define CVMJITsetErrorMessage(con, message) ((con)->errorMessage = message)
#else
#define CVMJITsetErrorMessage(con, message)
#endif

/* CVMJITerror() - Works just like an exception throw */
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)
#define CVMJITerror(con, err, message) {  \
    const char *msg = message;		  \
    if (msg == NULL) {			  \
	msg = "<unspecified>";		  \
    }					  \
    CVMJITsetErrorMessage(con, msg);	  \
    CVMJIThandleError(con, CVMJIT_##err); \
}
#else
#define CVMJITerror(con, err, message)	  \
    CVMJIThandleError(con, CVMJIT_##err)
#endif

extern void
CVMJIThandleError(CVMJITCompilationContext* con, int error);

extern void
CVMJITlimitExceeded(CVMJITCompilationContext *con, const char *errorStr);

/*************************************
 * CVMJITStack implementation
 *************************************/

/* A stack implementation to keep track of generic items 
 * such as a stack of pointers to a CVMJITIRBLOCK or CVMJITIRNode.
 */

struct CVMJITStack {
    /* Explicit stack for keeping track of items */
    void** 	    todo;
    CVMUint32       todoIdx;
    CVMUint32       todoIdxMax;
};

/*
 * Initialize stack. numItems has the maximum depth
 * of the stack.
 */
extern void
CVMJITstackInit(CVMJITCompilationContext* con, CVMJITStack* stack,
		CVMUint32 numItems);

#define CVMJITstackPush(con, item, stack)               \
{                                                       \
    CVMassert((stack)->todoIdx < (stack)->todoIdxMax);  \
    (stack)->todo[(stack)->todoIdx++] = item;           \
}

#define CVMJITstackPop(con, stack) \
    (CVMassert((stack)->todoIdx >= 1), --((stack)->todoIdx), \
     (stack)->todo[(stack)->todoIdx])

#define CVMJITstackDiscardTop(con, stack) \
    (CVMassert((stack)->todoIdx >= 1), --((stack)->todoIdx))

#define CVMJITstackGetTop(con, stack) \
    (CVMassert((stack)->todoIdx >= 1), (stack)->todo[((stack)->todoIdx)-1])

#define CVMJITstackGetElementAtIdx(con, stack, idx) \
    (CVMassert((stack)->todoIdx > idx), (stack)->todo[idx])

#define CVMJITstackIsEmpty(con, stack) ((stack)->todoIdx == 0)

#define CVMJITstackCnt(con, stack)     ((stack)->todoIdx)

#define CVMJITstackResetCnt(con, stack) ((stack)->todoIdx = 0)

#define CVMJITstackSetCount(con, stack, newTopIndex) \
    (CVMassert(newTopIndex <= (stack)->todoIdxMax), \
     ((stack)->todoIdx = newTopIndex))

/*************************************
 * CVMJITGrowableArray
 *************************************/

/*
 * A growable array of generic items
 */
typedef struct CVMJITGrowableArray {
    void*       items;
    CVMUint32   itemSize; /* How big is one item */
    CVMUint32   capacity; /* Space for how many? */
    CVMUint32   numItems; /* Number of items contained */
} CVMJITGrowableArray;

extern void
CVMJITgarrInit(CVMJITCompilationContext* con, CVMJITGrowableArray* garr,
	       CVMUint32 itemSize);

#define CVMJITgarrGetElems(con, garr)       ((garr)->items)
#define CVMJITgarrGetNumElems(con, garr)    ((garr)->numItems)
#define CVMJITgarrIsEmpty(con, garr)	    ((garr)->numItems == 0)
#define CVMJITgarrPopElem(con, garr) 	    ((garr)->numItems--) 

/*
 * Do not call this directly. Private to implementation
 */
extern void
CVMJITgarrExpandPrivate(CVMJITCompilationContext* con,
			CVMJITGrowableArray* garr);


#define CVMJITgarrNewElem(con, garr, retVal)				\
{									\
    /* Do we need to expand the capacity of this set of items */	\
    if ((garr)->numItems == (garr)->capacity) {				\
	CVMJITgarrExpandPrivate(con, (garr));				\
    }									\
    									\
    /* Return the next space for the item, and count this item */	\
    (retVal) = (CVMUint8*)((garr)->items) + 				\
                   ((garr)->numItems++) * (garr)->itemSize;		\
}

/*************************************
 * JIT tracing utilities
 *************************************/

#ifdef CVM_TRACE_JIT

#define CVMtraceJIT(flags, x) \
    (CVMcheckDebugJITFlags(flags) != 0 ? CVMconsolePrintf x : (void)0)
#define CVMtraceJITExec(flags, x) \
    if (CVMcheckDebugJITFlags(flags) != 0) x
#else
#define CVMtraceJIT(flags, x)
#define CVMtraceJITExec(flags, x)
#endif /* CVM_TRACE_JIT */

#define CVMtraceJITBCTOIR(x)    CVMtraceJIT(CVM_DEBUGFLAG(TRACE_JITBCTOIR), x)
#define CVMtraceJITBCTOIRExec(x) \
    CVMtraceJITExec(CVM_DEBUGFLAG(TRACE_JITBCTOIR), x)
#define CVMtraceJITCodegen(x)   CVMtraceJIT(CVM_DEBUGFLAG(TRACE_JITCODEGEN), x)
#define CVMtraceJITCodegenExec(x) \
    CVMtraceJITExec(CVM_DEBUGFLAG(TRACE_JITCODEGEN), x)
#define CVMtraceJITStats(x)     CVMtraceJIT(CVM_DEBUGFLAG(TRACE_JITSTATS), x)
#define CVMtraceJITStatsExec(x) \
    CVMtraceJITExec(CVM_DEBUGFLAG(TRACE_JITSTATS), x)
#define CVMtraceJITStatus(x)     CVMtraceJIT(CVM_DEBUGFLAG(TRACE_JITSTATUS), x)
#define CVMtraceJITStatusExec(x) \
    CVMtraceJITExec(CVM_DEBUGFLAG(TRACE_JITSTATUS), x)
#define CVMtraceJITError(x)     CVMtraceJIT(CVM_DEBUGFLAG(TRACE_JITERROR), x)
#define CVMtraceJITErrorExec(x) \
    CVMtraceJITExec(CVM_DEBUGFLAG(TRACE_JITERROR), x)

#define CVMtraceJITIROPT(x)     CVMtraceJIT(CVM_DEBUGFLAG(TRACE_JITIROPT), x)
#define CVMtraceJITIROPTExec(x) \
    CVMtraceJITExec(CVM_DEBUGFLAG(TRACE_JITIROPT), x)
#define CVMtraceJITInlining(x) \
    CVMtraceJIT(CVM_DEBUGFLAG(TRACE_JITINLINING), x)
#define CVMtraceJITInliningExec(x) \
    CVMtraceJITExec(CVM_DEBUGFLAG(TRACE_JITINLINING), x)
#define CVMtraceJITOSR(x) \
    CVMtraceJIT(CVM_DEBUGFLAG(TRACE_JITOSR), x)
#define CVMtraceJITOSRExec(x) \
    CVMtraceJITExec(CVM_DEBUGFLAG(TRACE_JITOSR), x)
#define CVMtraceJITRegLocals(x) \
    CVMtraceJIT(CVM_DEBUGFLAG(TRACE_JITREGLOCALS), x)
#define CVMtraceJITRegLocalsExec(x) \
    CVMtraceJITExec(CVM_DEBUGFLAG(TRACE_JITREGLOCALS), x)
#define CVMtraceJITPatchedInvokes(x) \
    CVMtraceJIT(CVM_DEBUGFLAG(TRACE_JITPATCHEDINVOKES), x)
#define CVMtraceJITPatchedInvokesExec(x) \
    CVMtraceJITExec(CVM_DEBUGFLAG(TRACE_JITPATCHEDINVOKES), x)

#define CVMtraceJITAny(x) \
    CVMtraceJIT((CVM_DEBUGFLAG(TRACE_JITBCTOIR) | \
                 CVM_DEBUGFLAG(TRACE_JITCODEGEN) | \
                 CVM_DEBUGFLAG(TRACE_JITSTATS)), x)
#define CVMtraceJITAnyExec(x) \
    CVMtraceJITExec((CVM_DEBUGFLAG(TRACE_JITBCTOIR) | \
                     CVM_DEBUGFLAG(TRACE_JITCODEGEN) | \
                     CVM_DEBUGFLAG(TRACE_JITSTATS)), x)

#ifdef CVM_TRACE_JIT

extern CVMInt32
CVMcheckDebugJITFlags(CVMInt32 flags);

extern CVMInt32
CVMsetDebugJITFlags(CVMInt32 flags);

extern CVMInt32
CVMclearDebugJITFlags(CVMInt32 flags);

extern CVMInt32
CVMrestoreDebugJITFlags(CVMInt32 flags, CVMInt32 oldvalue);

#endif /* CVM_TRACE_JIT */

#endif /* _INCLUDED_JITUTILS_H */

