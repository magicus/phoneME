/*
 * @(#)jitconstantpool.c	1.51 06/10/10
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
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitfixup.h"
#include "javavm/include/jit/jitconstantpool.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/jit/jitcomments.h"
#include "javavm/include/jit/jitstats.h"

#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/porting/doubleword.h"

/*
 * This implementation of run-time constant management
 * is for PC-relative pools intermixed with the program text,
 * as the CPU uses. Other implementations will be required for other
 * strategies.
 */

/*
 * a comparison function type used to lookup a value
 * CVM_TRUE  => equal
 * CVM_FALSE => not equal
 */
typedef CVMBool (*CVMCompareFunction)(CVMJITConstantEntry* ep, void* vp);

/*
 * a function for filling in information in an entry being created
 */
typedef void (*CVMFillinFunction)(CVMJITConstantEntry* ep, void* vp);

/*
 * Look for the existing constant.
 * This can be called repeatedly, because a constant can be
 * in the constant pool multiple times with different addresses.
 * This is required by reachability constraints.
 */
static CVMJITConstantEntry *
findConstantEntry(
    int constSize,
    CVMCompareFunction cfp,
    void* vp,
    CVMJITConstantEntry *head)
{
    CVMJITConstantEntry *thisEntry = head;
    while (thisEntry != NULL){
	/* We ignore cached constants since they aren't real constants */
	if ((!thisEntry->isCachedConstant) &&
	    (thisEntry->constSize == constSize) && (*cfp)(thisEntry, vp))
	{
	    return thisEntry;
	}
	thisEntry = thisEntry->next;
    }
    return NULL;
}

static CVMJITConstantEntry *
addConstantEntry(
    CVMJITCompilationContext *con,
    CVMFillinFunction ffp,
    void* vp,
    CVMUint8 isCachedConstant)
{
    CVMJITConstantEntry *thisEntry;

    thisEntry = (CVMJITConstantEntry*)
	CVMJITmemNew(con, JIT_ALLOC_CGEN_OTHER, sizeof(CVMJITConstantEntry));

    thisEntry->hasBeenEmitted = CVM_FALSE;
    thisEntry->references = NULL;
    thisEntry->isCachedConstant = isCachedConstant;
    (*ffp)(thisEntry, vp); /* fill in type and value */
#ifdef CVM_TRACE_JIT
    thisEntry->printName = CVMJITgetSymbolName(con);
#endif
    thisEntry->next = con->constantPool;
    con->constantPool = thisEntry;
    con->constantPoolSize += (thisEntry->constSize == shortConst) ? 4 : 8;
    return thisEntry;
}

/*
 * Periodic check point to see if the constant pool needs to be dumped
 * any time soon
 */
CVMBool
CVMJITcpoolNeedDump(CVMJITCompilationContext* con)
{
#ifndef CVMCPU_HAS_CP_REG
    /* Make sure we haven't used more than half our load/store offset range. */
    if (con->numEntriesToEmit != 0) {
	CVMInt32 logicalAddress = CVMJITcbufGetLogicalPC(con);

#ifdef CVM_JIT_USE_FP_HARDWARE
	if (con->earliestFPConstantRefPC != MAX_LOGICAL_PC &&
	    !CVMJITcanReachAddress(con, 
	        logicalAddress, con->earliestFPConstantRefPC,
	        CVMJIT_FPMEMSPEC_ADDRESS_MODE, CVM_TRUE)) {
	    /* We are drifting dangerously towards not being able to reach
	       the constants. */
	    return CVM_TRUE;
	}
#endif

	if (con->earliestConstantRefPC != MAX_LOGICAL_PC && 
	    !CVMJITcanReachAddress(con,
	        logicalAddress, con->earliestConstantRefPC,
	        CVMJIT_MEMSPEC_ADDRESS_MODE, CVM_TRUE)) {
	    /* We are drifting dangerously towards not being able to reach
	       the constants. */
	    return CVM_TRUE;
	}
    }
#endif
    return CVM_FALSE;
}


static CVMInt32
CVMJITgetRuntimeConstantReference(
    CVMJITCompilationContext* con,
    CVMInt32 logicalAddress,
    int constSize,
    CVMCompareFunction cfp,
    CVMFillinFunction  ffp,
    void* vp,
    CVMUint8 isCachedConstant, CVMBool isFloat)
{
    CVMJITConstantEntry *thisEntry;

    if (isCachedConstant) {
	/* don't bother looking for cached constants. They are always unique */
	thisEntry = NULL;
    } else {
	thisEntry = GET_CONSTANT_POOL_LIST_HEAD(con);
#ifdef CVMCPU_HAS_CP_REG
	thisEntry = findConstantEntry(constSize, cfp, vp, thisEntry);
#else
        /* See if this constant has been emitted and is still in range */
        while (thisEntry != NULL){
	    thisEntry = findConstantEntry(constSize, cfp, vp, thisEntry);
	    if (thisEntry == NULL)
		break;
	    if (!(thisEntry->hasBeenEmitted))
		break;
	    /* We found an already emitted entry. Return its address if it can
	       be reached from 'logicalAddress'. */

#ifdef CVM_JIT_USE_FP_HARDWARE
 	    if (isFloat && CVMJITcanReachAddress(con,
				   logicalAddress, thisEntry->address,
				   CVMJIT_FPMEMSPEC_ADDRESS_MODE, CVM_FALSE)) {
		return thisEntry->address;
	    }
#endif

	    if (!isFloat && CVMJITcanReachAddress(con,
				      logicalAddress, thisEntry->address,
				      CVMJIT_MEMSPEC_ADDRESS_MODE, CVM_FALSE)){
		return thisEntry->address;
	    }
	    /* If we are here, we found an already emitted constant that is
	       out of reach of the current 'logicalAddress'. Therefore we
	       keep looking */
	    thisEntry = thisEntry->next;
	}
#endif
    }
    if (thisEntry == NULL){
	/* No constant has been found, reachable or not */
	thisEntry = addConstantEntry(con, ffp, vp, isCachedConstant);
	/* Remember how many we have to write out at the next dump */
	con->numEntriesToEmit++;
    }
    CVMassert(!thisEntry->hasBeenEmitted);
    CVMJITfixupAddElement(con, &(thisEntry->references), logicalAddress);
    CVMJITresetSymbolName(con);

#ifdef CVM_JIT_USE_FP_HARDWARE
    if (isFloat) {
	if (logicalAddress < con->earliestFPConstantRefPC) {
	    /* See the farthest away constant reference */
	    con->earliestFPConstantRefPC = logicalAddress;
	}
    } else
#endif
    {
	if (logicalAddress < con->earliestConstantRefPC) {
	    /* See the farthest away constant reference */
	    con->earliestConstantRefPC = logicalAddress;
	}
    }
    return 0;
}

/*
 * Helper functions for CVMJITgetRuntimeConstantReference32
 */
static CVMBool
CVMCompareEntry32(CVMJITConstantEntry* thisEntry, void* vp)
{
    CVMassert(thisEntry->constSize == shortConst);
    return (thisEntry->v.single == *(CVMInt32*)vp);
}

static void
CVMFillinEntry32(CVMJITConstantEntry* thisEntry, void* vp)
{
    thisEntry->constSize = shortConst;
    if (!thisEntry->isCachedConstant) {
	thisEntry->v.single = *(CVMInt32*)vp;
    }
}

/*
 * Lookup and insert a 32-bit constant
 */
CVMInt32
CVMJITgetRuntimeConstantReference32(
    CVMJITCompilationContext* con,
    CVMInt32 logicalAddress,
    CVMInt32 v)
{
    return CVMJITgetRuntimeConstantReference( con, logicalAddress, shortConst,
        &CVMCompareEntry32, &CVMFillinEntry32, &v, CVM_FALSE, CVM_FALSE);
}

#ifdef CVM_JIT_USE_FP_HARDWARE
CVMInt32
CVMJITgetRuntimeFPConstantReference32(
    CVMJITCompilationContext* con,
    CVMInt32 logicalAddress,
    CVMInt32 v)
{
    return CVMJITgetRuntimeConstantReference( con, logicalAddress, shortConst,
        &CVMCompareEntry32, &CVMFillinEntry32, &v, CVM_FALSE, CVM_TRUE);
}
#endif

#ifdef IAI_CACHEDCONSTANT
/*
 * Insert a 32-bit cached constant
 */
void
CVMJITgetRuntimeCachedConstant32(
    CVMJITCompilationContext* con)
{
    CVMInt32 logicalAddress = CVMJITcbufGetLogicalPC(con);
    /* Reserve a slot for the cached constant */
    CVMJITgetRuntimeConstantReference(
	con, logicalAddress, shortConst,
        &CVMCompareEntry32, &CVMFillinEntry32, NULL, CVM_TRUE, CVM_FALSE);
    /*
     * Emit code to load the address of the constant into a platform
     * specific register. We don't know the true address of the cached
     * constant yet, so just use the address where code to load the
     * cache constant address will be emitted at. Later on this will
     * get patched when the constant is dumped.
     */
    CVMJITemitLoadConstantAddress(con, logicalAddress);

}
#endif /* IAI_CACHEDCONSTANT */


/*
 * Helper functions for CVMJITgetRuntimeConstantReference64
 */
static CVMBool
CVMCompareEntry64(CVMJITConstantEntry* thisEntry, void* vp)
{
    CVMJavaLong l1 = CVMjvm2Long(thisEntry->v.longVal.v);
    CVMJavaLong l2 = CVMjvm2Long(((CVMJavaVal64*)vp)->v);
    CVMassert(thisEntry->constSize == longConst);
    return (CVMlongCompare(l1, l2) == 0);
}

static void
CVMFillinEntry64(CVMJITConstantEntry* thisEntry, void* vp)
{
    thisEntry->constSize = longConst;
    CVMmemCopy64(thisEntry->v.longVal.v,  ((CVMJavaVal64*)vp)->v);
}


/*
 * Lookup and insert a 64-bit constant
 */
CVMInt32
CVMJITgetRuntimeConstantReference64(
    CVMJITCompilationContext* con,
    CVMInt32 logicalAddress,
    CVMJavaVal64* vp)
{
    return CVMJITgetRuntimeConstantReference( con, logicalAddress, longConst,
        &CVMCompareEntry64, &CVMFillinEntry64, vp, CVM_FALSE, CVM_FALSE);
}

#ifdef CVM_JIT_USE_FP_HARDWARE
CVMInt32
CVMJITgetRuntimeFPConstantReference64(
    CVMJITCompilationContext* con,
    CVMInt32 logicalAddress,
    CVMJavaVal64* vp)
{
    return CVMJITgetRuntimeConstantReference( con, logicalAddress, longConst,
        &CVMCompareEntry64, &CVMFillinEntry64, vp, CVM_FALSE, CVM_TRUE);
}
#endif

void
CVMJITdumpRuntimeConstantPool(CVMJITCompilationContext* con, CVMBool forceDump)
{
    CVMJITConstantEntry *thisEntry;
    CVMJITFixupElement *thisRef;
    CVMInt32 emitted = 0; /* Count those emitted */
    CVMUint32 size;


#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
    CVMJITCSConstantPoolDumpInfo* dumpInfoItem;
#endif

    if (con->numEntriesToEmit == 0) {
	return;
    }

    /* Check whether constants still reachable */
    if (!forceDump && !CVMJITcpoolNeedDump(con)) {
	return;
    }

#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
    CVMJITcsSetEmitInPlace(con);
    dumpInfoItem = CVMJITcsAllocateConstantPoolDumpInfoEntry(con);
    dumpInfoItem->startPC = CVMJITcbufGetLogicalPC(con);
#endif /*IAI_CODE_SCHEDULER_SCORE_BOARD*/

    /* align constant pool to 4 byte boundary if necessary */
#if (CVMCPU_INSTRUCTION_SIZE != 4)
    {
	int i = (CVMUint32)CVMJITcbufGetPhysicalPC(con) & 0x3;
	if (i != 0) {
	    for (; i < 4; i++) {
		CVMtraceJITCodegenExec({
		    CVMconsolePrintf("0x%8.8x\t%d:",
				     CVMJITcbufGetPhysicalPC(con),
				     CVMJITcbufGetLogicalPC(con));
		    CVMconsolePrintf("	.byte	%d\n", i);
		});
		CVMJITcbufEmit1(con, i);
	    }
	}
    }
#endif

    /*
     * Dump the 1 word constants first, followed by the 2 word constants.
     * This way we can align the two work constants if necessary.
     */
    size = shortConst;
 dumpForNewSize:
    thisEntry = GET_CONSTANT_POOL_LIST_HEAD(con);
    while (thisEntry != NULL) {
	if (!thisEntry->hasBeenEmitted && thisEntry->constSize == size) {
	    CVMInt32 thisLogicalAddress = CVMJITcbufGetLogicalPC(con);
            CVMJITaddCodegenComment((con, thisEntry->printName));
            CVMJITstatsRecordInc(con,
                CVMJIT_STATS_NUMBER_OF_CONSTANTS_EMITTED);
	    if (thisEntry->constSize == shortConst) {
		CVMJITemitWord(con, thisEntry->v.single);
	    } else {
		/*
		 * We left it up to the user to order the words
		 * in the right order. We just put them out.
		 */
		CVMJITemitWord(con, thisEntry->v.longVal.v[0]);
		CVMJITemitWord(con, thisEntry->v.longVal.v[1]);
	    }
	    thisRef = thisEntry->references;
	    while (thisRef != NULL) {
	       if (thisEntry->isCachedConstant) {
#ifdef IAI_CACHEDCONSTANT
		   CVMtraceJITCodegen((
			":::::Fixed instruction at %d to reference %d\n",
			thisRef->logicalAddress, thisLogicalAddress));
		   CVMJITcbufPushFixup(con, thisRef->logicalAddress);
                   CVMJITcsSetEmitInPlace(con);
		   CVMJITemitLoadConstantAddress(con, thisLogicalAddress);
                   CVMJITcsClearEmitInPlace(con);
		   CVMJITcbufPop(con);
#endif /* IAI_CACHEDCONSTANT */
	       } else {

		   /* patch constant reference to proper address */
		   CVMJITfixupAddress(con,
			thisRef->logicalAddress, thisLogicalAddress,
			CVMJIT_MEMSPEC_ADDRESS_MODE);
	       }
	       thisRef = thisRef->next;
	    }
	    thisEntry->hasBeenEmitted = CVM_TRUE;
	    thisEntry->address = thisLogicalAddress;
	    emitted++;
	}
	thisEntry = thisEntry->next;
    }
    if (size == shortConst) {
	/* see if we have some 64-bit constants to emit */
	if (con->numEntriesToEmit != emitted) {
	    /* align 64-bit constants */
	    if (((CVMUint32)CVMJITcbufGetPhysicalPC(con) & 0x4) != 0) {
		CVMJITemitWord(con, 0);
	    }
	    /* emit the 64-bit constants */
	    size = longConst;
	    goto dumpForNewSize;
	}
    }
    CVMassert(con->numEntriesToEmit == emitted);
    con->numEntriesToEmit = 0;
    /* 64 bit align for dumping 64 bit constants */
    con->constantPoolSize = 4;
    con->earliestConstantRefPC = MAX_LOGICAL_PC;
#ifdef CVM_JIT_USE_FP_HARDWARE
    con->earliestFPConstantRefPC = MAX_LOGICAL_PC;
#endif

#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
    CVMJITcsClearEmitInPlace(con);
    dumpInfoItem->endPC = CVMJITcbufGetLogicalPC(con);
    CVMJITcsBeginBlock(con);
#endif /*IAI_CODE_SCHEDULER_SCORE_BOARD*/
}
