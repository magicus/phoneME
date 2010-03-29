/*
 * @(#)jitregman.c	1.9 06/10/23
 * 
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 */

/*
 * Register manager implementation.
 */
#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/utils.h"
#include "javavm/include/bcutils.h"
#include "javavm/include/ccee.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitir.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitirblock.h"
#include "javavm/include/jit/jitcomments.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/jit/jitirdump.h"
#include "javavm/include/jit/jitstackmap.h"
#include "javavm/include/jit/jitarchemitter.h"
#include "javavm/include/jit/jitregman.h"
#include "javavm/include/jit/jitopcodes.h"
#include "javavm/include/jit/jitstackman.h"
#include "javavm/include/jit/jitgrammar.h"

#include "javavm/include/clib.h"

#ifdef CVM_TRACE_ENABLED
#include "javavm/include/globals.h"
#endif

#ifdef CVM_DEBUG_ASSERTS
/* FIXME: undef for now */
#undef CVMJIT_STRICT_REFLOCAL_ASSERTS
#endif

#define RM_INITIAL_REF_COUNT_SIZE	64 /* more than ever needed */
#define RM_STICKY_REF_COUNT		255

#ifdef CVM_JIT_USE_FP_HARDWARE
#define RM_MAX_INTERESTING_REG	(con)->maxInteresting
#define RM_MIN_INTERESTING_REG	(con)->minInteresting
#define RM_SAFE_SET 		(con)->safeSet
#define RM_UNSAFE_SET 		(con)->unsafeSet
#define RM_ANY_SET 		(con)->anySet
#define RM_SINGLE_REG_ALIGN	(con)->singleRegAlignment
#define RM_DOUBLE_REG_ALIGN	(con)->doubleRegAlignment
#define RM_MOV_OPCODE(_con)	(_con)->movOpcode
#define RM_MAX_REG_SIZE(_con)	(_con)->maxRegSize

#else
    /* not using FP hardware */
#define RM_MAX_INTERESTING_REG	CVMCPU_MAX_INTERESTING_REG
#define RM_MIN_INTERESTING_REG	CVMCPU_MIN_INTERESTING_REG
#define RM_SAFE_SET 		CVMRM_SAFE_SET
#define RM_UNSAFE_SET 		CVMRM_UNSAFE_SET
#define RM_ANY_SET 		CVMRM_ANY_SET
#define RM_SINGLE_REG_ALIGN	CVMCPU_SINGLE_REG_ALIGNMENT
#define RM_DOUBLE_REG_ALIGN	CVMCPU_DOUBLE_REG_ALIGNMENT
#define RM_MAX_REG_SIZE(_con)	CVMCPU_MAX_REG_SIZE
#define RM_MOV_OPCODE(_con)	CVMCPU_MOV_OPCODE
#endif

#if defined(CVM_64)
#define RM_STORE_OPCODE(_con,_res)	((_con)->storeOpcode[ (CVMRMisRef((_res)) || CVMRMisAddr((_res))) ? ((_res)->size) : ((_res)->size)-1])
#define RM_LOAD_OPCODE(_con,_res)	((_con)->loadOpcode[  (CVMRMisRef((_res)) || CVMRMisAddr((_res))) ? ((_res)->size) : ((_res)->size)-1])
#else
#define RM_STORE_OPCODE(_con,_res)	((_con)->storeOpcode[((_res)->size)-1])
#define RM_LOAD_OPCODE(_con,_res)	((_con)->loadOpcode[((_res)->size)-1])
#endif   /* CVM_64 */

#if ((CVMCPU_MAX_REG_SIZE != 1) || \
	(defined(CVM_JIT_USE_FP_HARDWARE) && (CVMCPU_FP_MAX_REG_SIZE != 1)))
/* there are some double-word registers */
#define RM_NREGISTERS(_con,_size)	((_size) <= RM_MAX_REG_SIZE(_con) ? 1 : 2)
#else
/* all the registers are single-word */
#define RM_NREGISTERS(_con,_size) 	(_size)
#endif


#ifdef  CVM_DEBUG_ASSERTS
#define RM_INT_KEY	1
#define RM_FP_KEY	2
#define CVMRMassertContextMatch(cx,rp) CVMassert((cx) == (rp)->rmContext)
#else
#define CVMRMassertContextMatch(cx,rp)
#endif

typedef enum {
    CVMRM_REGPREF_TARGET,
    CVMRM_REGPREF_SCRATCH,
    CVMRM_REGPREF_PERSISTENT
} CVMRMRegisterPreference;

#define DEFAULT_CONSTANT_PREF	CVMRM_REGPREF_PERSISTENT
#define DEFAULT_LOCAL_PREF	CVMRM_REGPREF_PERSISTENT
#ifndef CVMCPU_NO_REGPREF_PERSISTENT
#define DEFAULT_RESOURCE_PREF	CVMRM_REGPREF_PERSISTENT
#else
#define DEFAULT_RESOURCE_PREF  CVMRM_REGPREF_TARGET
#endif

static void
CVMRMpinResource0(CVMJITRMContext*, CVMRMResource*,
    CVMRMregset target, CVMRMregset avoid,
    CVMRMRegisterPreference pref, CVMBool strict);

#ifdef CVM_JIT_REGISTER_LOCALS
static void
CVMRMbindAllIncomingLocalNodes(CVMJITCompilationContext* con,
			       CVMJITIRBlock* b);
#endif

static void
CVMRMbindAllUsedNodes(CVMJITCompilationContext* con, CVMJITIRBlock* b);

static void
CVMRMbindUseToResource(CVMJITCompilationContext* con, CVMJITIRNode* expr);

/* Purpose: Gets an available reg from the specified register set. */
static int
findMaxRegInSet(
    CVMJITRMContext* con, 
    CVMRMregset target,
    int minInteresting,
    int maxInteresting,
    int alignment)
{
    int i;

#ifdef CVM_JIT_ROUND_ROBIN_REGISTER_SELECTION
    {
       /* SVMC_JIT c5041613 (dk) 2004-01-26.  round robin-based
	* register selection.  The original naive strategy lead to
	* massive register reuse of the largest available registers
	* causing tremendous spilling overhead.
	*
	* Improved register selection variant is based on a round
	* robin strategy.  The idea is to select that register which
	* had not been selected for the longest time among all
	* registers between which to choose.  This is context and flow
	* insensitive, but should yield a better register utilization
	* and prevent bottlenecks in register usage.  The selection is
	* done in two stages: first we try to find an unoccupied
	* register - subject to the round robin strategy. If none can
	* be found, then we just select one and have to generate spill
	* code later on. */
       /* SVMC_JIT d022609 (ML) 2004-05-18. improved code. */
       CVMRMResource* rp;
       int mru = con->lastRegNo;
       CVMRMregset o = con->occupiedRegisters; 
       CVMassert((minInteresting <= mru) && (mru <= maxInteresting));
       if (alignment == 2) {
	  minInteresting = (1 + minInteresting) & ~1 /* round up to even */;  
	  maxInteresting = maxInteresting & ~1 /* round down to even */;
	  mru = mru & ~1 /* round down to even */;
       }
       for (i = mru - alignment; i >= minInteresting; i -= alignment) {
	  if (target & (1U<<i)) {
	     rp = con->reg2res[i];
	     if (rp == NULL) {
		if (!(o & (1U<<i)))
		   return i;
	     }
	     else {
		CVMassert(!(rp->flags & CVMRMpinned));
		if (!((o & (1U<<i)) && (rp->flags & CVMRMdirty)))
		   return i;
	     }
	  }
       }
       for (i = maxInteresting; i >= mru; i -= alignment) {
	  if (target & (1U<<i)) {
	     rp = con->reg2res[i];
	     if (rp == NULL) {
		if (!(o & (1U<<i)))
		   return i;
	     }
	     else {
		CVMassert(!(rp->flags & CVMRMpinned));
		if (!((o & (1U<<i)) 
		      && (rp->flags & CVMRMdirty)))
		   return i;
	     }
	  }
       }
       /* We did not find an unoccupied register. So now we try to
	  select one we have to generate spill code for. */
       for (i = mru - alignment; i >= minInteresting; i -= alignment) {
	  if (target & (1U<<i))
	     return i;
       }
       for (i = maxInteresting; i >= mru; i -= alignment) {
	  if (target & (1U<<i))
	     return i;
       }
    }
#else /* CVM_JIT_ROUND_ROBIN_REGISTER_SELECTION */
    /* The register with the highest ID value in the specified set
       will be selected first: */
    if (alignment == 2 )
	maxInteresting &= ~1; /* round down to even */
    for (i=maxInteresting; i>=minInteresting; i-= alignment ) {
	if ((target & (1U<<i)) != 0)
	    return i;
    }
#endif /* CVM_JIT_ROUND_ROBIN_REGISTER_SELECTION */
    return -1;
}

/* Purpose: Gets an available reg from the specified register set. */
static int
findMinRegInSet(
    CVMRMregset target,
    int minInteresting,
    int maxInteresting,
    int alignment,
    int nregs)
{
    int i;

    /*
     * Make search easier if a 32-bit pair is needed by masking off the upper
     * register of each pair if the lower register is not available. 
     * This way we know if any bit is set, then the following bit is also set.
     */
    if (nregs > 1) {
	target = target & (target >> 1);
    }

    /* The register with the lowest ID value in the specified set
       will be selected first: */
    CVMassert(alignment > 0);
    if (alignment != 1) {
	int left = (minInteresting % alignment);
	if (left > 0) {
	    /* round up to alignment */
	    minInteresting += alignment - left;
	}
    }
    CVMassert(minInteresting % alignment == 0);
    for (i=minInteresting; i<=maxInteresting; i+= alignment ) {
	if ((target & (1U<<i)) != 0)
	    return i;
    }
    return -1;
}

/*
 * Spill Location Management. Finding and relinquishing spill locations
 * using CVMJITSet structures
 */

static void
resizeSpillRefCount(CVMJITCompilationContext* con, int needed)
{
    int newsize = needed;
    CVMUint8* newallocation;
    /* round up to multiple of 64 */
    newsize = (newsize+63) & ~63;
    CVMassert(newsize > con->RMcommonContext.spillBusyRefSize);

    newallocation = CVMJITmemNew(con, JIT_ALLOC_CGEN_REGMAN,
				 newsize*sizeof(CVMUint8));
    memcpy(newallocation, con->RMcommonContext.spillBusyRefCount,
	   con->RMcommonContext.spillBusyRefSize*sizeof(CVMUint8));
    con->RMcommonContext.spillBusyRefCount = newallocation;
    con->RMcommonContext.spillBusyRefSize = newsize;
}


static void
resetSpillMap(CVMJITCompilationContext* con, CVMJITIRBlock* b)
{
	CVMJITRMCommonContext* common = &(con->RMcommonContext);
	/* reset the spill set */
	CVMJITsetClear(con, &(common->spillBusySet));
	memset(common->spillBusyRefCount, 0,
	       common->spillBusyRefSize * sizeof(CVMUint8));
	/* mark all phiWords as busy */
	if (con->maxPhiSize > 0){
	    int i;
	    int maxPhi = con->maxPhiSize;
	    CVMUint8* refElement;
	    CVMJITsetPopulate(con, &(common->spillBusySet),
			      maxPhi);
	    if (maxPhi > common->spillBusyRefSize){
		resizeSpillRefCount(con, maxPhi);
	    }
	    refElement = common->spillBusyRefCount;
	    for( i = 0; i < maxPhi; i++){
		*(refElement++) = RM_STICKY_REF_COUNT;
	    }
		
	}

	/* initialize the spill ref set */
	CVMJITsetClear(con, &(con->RMcommonContext.spillRefSet));
	con->RMcommonContext.maxPhiSize     = con->maxPhiSize;
	con->RMcommonContext.maxSpillNumber = con->maxPhiSize-1;
}

int findSpillLoc(
    CVMJITCompilationContext* con,
    int                       size,
    CVMBool                   isRef)
{
    int spilloff, topoff; 
    CVMBool isBusy;
    CVMJITSet*	busySet = &con->RMcommonContext.spillBusySet;
    CVMUint8*   refCount=  con->RMcommonContext.spillBusyRefCount;
    CVMJITSet*	refSet  = &con->RMcommonContext.spillRefSet;
    CVMassert((size == 1) || (size == 2));
    CVMassert((size == 1) || !isRef);
    for ( spilloff = con->maxPhiSize; ;spilloff += 1){
	CVMJITsetContains(busySet, spilloff, isBusy);
	if (!isBusy){
	    /* this one is available. If double word, look for next, too. */
	    if (size > 1){
		CVMJITsetContains(busySet, (spilloff+1), isBusy);
		if (!isBusy){
		    /* a pair of winners */
		    CVMJITsetAdd(con, busySet, spilloff);
		    CVMJITsetAdd(con, busySet, spilloff+1);
		    CVMJITsetRemove(con, refSet, spilloff);
		    CVMJITsetRemove(con, refSet, spilloff+1);
		    break;
		}
		/* else keep trying */
	    } else {
		/* a winner */
		CVMJITsetAdd(con, busySet, spilloff);
		if (isRef){
		    CVMJITsetAdd(con, refSet, spilloff);
		} else {
		    CVMJITsetRemove(con, refSet, spilloff);
		}
		break;
	    }
	}
    }
    /* location spilloff is available. see if it is a new hight water mark */
    topoff = spilloff+size-1;
    if (topoff > con->RMcommonContext.maxSpillNumber){
	con->RMcommonContext.maxSpillNumber = topoff;
    }
    if (topoff >= con->RMcommonContext.spillBusyRefSize){
	resizeSpillRefCount(con, topoff+1);
	refCount=  con->RMcommonContext.spillBusyRefCount;
    }
    refCount[spilloff] = 1;
    if (size > 1){
	refCount[spilloff+1] = 1;
    }
    
    if (spilloff > CVMRM_MAX_SPILL_LOCATION) {
        CVMJITlimitExceeded(con, "exceeding max spill locations");
    }

    return spilloff;
}

static void
relinquishSpillLoc(CVMJITCompilationContext*con, int loc, int size)
{
    CVMJITSet*	busySet = &con->RMcommonContext.spillBusySet;
    CVMUint8*   refCount=  con->RMcommonContext.spillBusyRefCount;
    CVMassert(loc>=0);
    CVMassert((size == 1) || (size == 2));
#ifdef CVM_DEBUG_ASSERTS
    {
	CVMBool isBusy;
	CVMJITsetContains(busySet, loc, isBusy);
	CVMassert(isBusy);
	CVMassert((loc+size) <= con->RMcommonContext.spillBusyRefSize);
	CVMassert(refCount[loc] > 0);
	if (size > 1){
	    CVMJITsetContains(busySet, (loc+1), isBusy);
	    CVMassert(isBusy);
	    CVMassert(refCount[loc+1] == refCount[loc]);
	}
    }
#endif
    if (refCount[loc] == RM_STICKY_REF_COUNT)
	return; /* cannot deallocate */
    if ((refCount[loc] -= 1) == 0){
	CVMJITsetRemove(con, busySet, loc);
	if (size > 1){
	    CVMJITsetRemove(con, busySet, (loc+1));
	}
    }
    if (size > 1){
	refCount[loc+1] -= 1;
    }
}

static void
incrementSpillCount(CVMJITCompilationContext*con, int loc, int size)
{
    CVMUint8*   refCount= con->RMcommonContext.spillBusyRefCount;
    CVMassert((loc+size) <= con->RMcommonContext.spillBusyRefSize);
    if (refCount[loc] == RM_STICKY_REF_COUNT)
	return; /* maxed out */
    refCount[loc] += 1;
    if (size > 1)
	refCount[loc+1] += 1;
}

static void
flushResource(CVMJITRMContext* con, CVMRMResource* rp)
{
    CVMJITCompilationContext* cc = con->compilationContext;
    if (CVMRMisLocal(rp)) {
        CVMassert(rp->localNo != CVMRM_INVALID_LOCAL_NUMBER);
	/* spill directly to local */
#if 0
	/* Currently locals are write through, so there is no need to
	 * spill them. However, because of the way the backend works
	 * when we need to force evalauation node, it can lead to
	 * a resource for a local being marked as dirty. We can
	 * ignore these
	 */
	CVMCPUemitFrameReference(cc, RM_STORE_OPCODE(con,rp), rp->regno,
	    CVMCPU_FRAME_LOCAL, rp->localNo, CVMRMisConstant(rp));
#endif
#if 0
	/* Sanity check to make sure the local is in sync. We'll load
	 * the local into JFP since we know the local can't be there.
	 * If it's 64bit, then we'll also use JSP. We also need to save
	 * and restore these registers.
	 */
	if (rp->size == 2) {
	    /* Save JSP and load 2nd word into it */
	    CVMCPUemitCCEEReferenceImmediate(cc, CVMCPU_STR32_OPCODE,
		CVMCPU_JSP_REG,
		offsetof(CVMCCExecEnv, ccmStorage) + sizeof(CVMUint32));
	    CVMCPUemitFrameReference(cc, CVMCPU_LDR32_OPCODE,
				     CVMCPU_JSP_REG, CVMCPU_FRAME_LOCAL,
				     rp->localNo+1, CVMRMisConstant(rp));
	}
	/* Save JFP and load 1st word into it */
	CVMCPUemitCCEEReferenceImmediate(cc, CVMCPU_STR32_OPCODE,
	    CVMCPU_JFP_REG, offsetof(CVMCCExecEnv, ccmStorage));
	/* this will trash JFP, so do it after the above frame ref */
	CVMCPUemitFrameReference(cc, CVMCPU_LDR32_OPCODE,
				 CVMCPU_JFP_REG, CVMCPU_FRAME_LOCAL,
				 rp->localNo, CVMRMisConstant(rp));
	CVMCPUemitCompareRegister(cc, CVMCPU_CMP_OPCODE,
	    CVMCPU_COND_NE, CVMCPU_JFP_REG, rp->regno);
	if (rp->size == 2) {
	    CVMconsolePrintf("64bit!\n");
	    CVMCPUemitBranch(cc, CVMJITcbufGetLogicalPC(con) + 
			     2*CVMCPU_INSTRUCTION_SIZE,
			     CVMCPU_COND_NE);
	    CVMCPUemitCompareRegister(cc, CVMCPU_CMP_OPCODE,
		CVMCPU_COND_NE, CVMCPU_JSP_REG, rp->regno+1);
	}
	CVMJITaddCodegenComment((cc, "call CVMsystemPanic"));
	CVMJITsetSymbolName((cc, "CVMsystemPanic"));
	CVMCPUemitAbsoluteCallConditional(cc, CVMsystemPanic, 
	    CVMJIT_CPDUMPOK, CVMJIT_CPBRANCHOK, CVMCPU_COND_NE);
	/* restore JFP and JSP */
	CVMCPUemitCCEEReferenceImmediate(cc, CVMCPU_LDR32_OPCODE,
	    CVMCPU_JFP_REG, offsetof(CVMCCExecEnv, ccmStorage));
	if (rp->size == 2) {
	    CVMCPUemitCCEEReferenceImmediate(cc, CVMCPU_LDR32_OPCODE,
		CVMCPU_JSP_REG,
		offsetof(CVMCCExecEnv, ccmStorage) + sizeof(CVMUint32));
	}
#endif
    } else if (!CVMRMisConstant(rp)) {
	/* spill to temp location */
	CVMJITaddCodegenComment((cc, CVMRMisRef(rp)?"REF spill":"spill"));
	if (rp->spillLoc < 0){
	    rp->spillLoc = findSpillLoc(cc, rp->size, CVMRMisRef(rp));
	}
	CVMCPUemitFrameReference(cc, RM_STORE_OPCODE(con, rp), rp->regno,
	    CVMCPU_FRAME_TEMP, rp->spillLoc, CVMRMisConstant(rp));
    }
}

/* 
 * SVMC_JIT d022609 (ML) 2004-01-07. ensure that a resource is up to
 * date in its home location.  for locals nothing happens.
 * temporaries might get pinned and spilled such that they will have a
 * spill location and will not be dirty. 
 */
void
CVMRMflushResource(CVMJITRMContext* con, CVMRMResource* res)
{
   if (CVMRMisLocal(res)) {
      /* according to remarks in `flushResource' locals are always in
       * sync, nothing to do here. */
   } else {
      CVMassert(!CVMRMisConstant(res));
      CVMassert(!CVMRMisJavaStackTopValue(res));
      CVMassert(!CVMRMisStackParam(res));
      CVMRMpinResource(con, res, CVMRM_ANY_SET, CVMRM_EMPTY_SET);
      if (res->flags & CVMRMdirty) {
	 flushResource(con, res);
	 res->flags ^= CVMRMdirty;
      }
      CVMassert(!(res->spillLoc < 0));
   }
}

void
spillResource(CVMJITRMContext* con, CVMRMResource* rp, CVMBool evict)
{
    if (rp->flags & CVMRMdirty) {
	flushResource(con, rp);
	rp->flags ^= CVMRMdirty;
    }
    if (evict) {
        /* Disassociate the register from the resource: */
        con->occupiedRegisters &= ~rp->rmask; /* Release the register. */
	con->reg2res[rp->regno] = NULL;
	if (rp->nregs > 1){
	    CVMassert(rp->nregs == 2);
	    con->reg2res[rp->regno + 1] = NULL;
	}
	rp->regno = -1;
    }
}

/* Purpose: Finds an available register that fits the specified profile as
            defined by the various parameters. */
static int 
findAvailableRegister0(
    CVMJITRMContext* con,
    CVMRMregset	target,
    CVMRMregset	avoid,
    CVMRMRegisterPreference pref,
    CVMBool     targetSetIsStrict,
    CVMBool     okToSpill,
    int		nregs)
{
    int		   regNo = -1;
    CVMRMregset	   availList[6];
    CVMRMregset	   prefMask;
    CVMRMregset    unoccupiedRegisters;
    CVMRMregset    unpinnedRegisters;
    int            numTargetGroups = 1;
    struct {
	int numTargetSets;
        CVMRMregset targetList[2];
    } targetGroup[2];
    int		   numAvailSets = 0;
    int		   g;

    switch (pref) {
    case CVMRM_REGPREF_TARGET:
        /* CVMRM_REGPREF_TARGET: Prefers that the reg be allocated from the
           specified target set.  No additional restriction necessary. */
	prefMask = RM_ANY_SET;
	break;
    case CVMRM_REGPREF_SCRATCH:
        /* CVMRM_REGPREF_SCRATCH: The reg to be allocated is intended for use
           as a scratch register, and its content is not expected to be
           retained for a long time.  Hence, we would prefer to allocate it
           from the unsafe set (could be trashed by C calls) so as not to
           compete with those with pref CVMRM_REGPREF_PERSISTENT. */
	prefMask = RM_UNSAFE_SET;
	break;
    case CVMRM_REGPREF_PERSISTENT:
        /* CVMRM_REGPREF_PERSISTENT: The reg to be allocated will probably be
           used to hold content that will be retained for a long time.  Hence,
           we would prefer to allocate it from the safe set (won't be trashed
           by C calls) so as not to minimize the need for spilling and
           reloading. */
	prefMask = RM_SAFE_SET;
	break;
    default:
	CVMassert(CVM_FALSE);
	prefMask = 0; /* get rid of compiler warning */
	break;
    };

    /* There can be up to 4 possible target register lists organized as 2
       groups.  The first group is for any reg allocation.  The second group
       is only applicable if the register need not be strictly allocated from
       the specified target set.  The lists are:
        1. Group 0, List 0: The target set specified by the caller limited to
                            the preferred set.
        2. Group 0, List 1: The full target set specified by the caller.
        3. Group 1, List 0: Any register limited to the preferred set.
        4. Group 1, List 1: Any register.
    */
    targetGroup[0].numTargetSets = 1;
    targetGroup[0].targetList[0] = target & prefMask; /* Group 0, List 0. */
    if (pref != CVMRM_REGPREF_TARGET) {
	targetGroup[0].targetList[1] = target; /* Group 0, List 1. */
	targetGroup[0].numTargetSets = 2;
    } else {
	CVMassert(target == targetGroup[0].targetList[0]);
    }
    if (!targetSetIsStrict) {
        numTargetGroups++;
	targetGroup[1].numTargetSets = 1;
	targetGroup[1].targetList[0] = RM_ANY_SET & prefMask; /* G1,L0. */
	if (pref != CVMRM_REGPREF_TARGET) {
	    targetGroup[1].targetList[1] = RM_ANY_SET; /* Group 1,List 1.*/
	    targetGroup[1].numTargetSets = 2;
	} else {
	    CVMassert(RM_ANY_SET == targetGroup[1].targetList[0]);
	}
    }

    /* The target lists about will be match against the following availability
       sets.  The availability sets are:
        Set 1: Unoccupied regs not on the avoid set, and on 
               the sandboxRegSet.
        Set 2: All unoccupied regs on the sandboxRegSet.
        Set 3: Unpinned regs not on the avoid set, and on 
               the sandboxRegSet.
        Set 4: All unpinned regs on the sandboxRegSet.
    */
    unoccupiedRegisters = ~con->occupiedRegisters & con->sandboxRegSet;
    unpinnedRegisters = ~con->pinnedRegisters & con->sandboxRegSet;

    /* include unoccupied, unavoided, sandbox registers */
    availList[numAvailSets++] = ~avoid & unoccupiedRegisters; /* Set 1.*/
#ifndef PREFER_SPILL_OVER_AVOID
    /* include unoccupied, avoided, sandbox registers */
    {
	if (unoccupiedRegisters != availList[numAvailSets - 1]) {
	    availList[numAvailSets++] = unoccupiedRegisters; /* Set 2.*/
	}
    }
#endif
    if (okToSpill) {
	CVMRMregset set;
#if 0
	/* include clean, occupied, unavoided, sandbox registers */
	set = ~avoid & ~con->dirtyRegisters & con->sandboxRegSet;
	if (set != availList[numAvailSets - 1]) {
	    availList[numAvailSets++] = set;
	}
#endif
	/* include dirty, occupied, unavoided, sandbox registers */
	set = ~avoid & unpinnedRegisters;
	if (set != availList[numAvailSets - 1]) {
	    availList[numAvailSets++] = set; /* Set 3. */
	}
    }
#ifdef PREFER_SPILL_OVER_AVOID
    /* include unoccupied, avoided, sandbox registers */
    availList[numAvailSets++] = unoccupiedRegisters; /* Set 2. */
#endif
    if (okToSpill) {
	CVMRMregset set;
#if 0
	/* include clean, occupied, avoided, sandbox registers */
	set = ~con->dirtyRegisters & con->sandboxRegSet;
	if (set != availList[numAvailSets - 1]) {
	    availList[numAvailSets++] = set;
	}
#endif
	/* include dirty, occupied, avoided, sandbox registers */
	set = unpinnedRegisters;
	if (set != availList[numAvailSets - 1]) {
	    availList[numAvailSets++] = set; /* Set 4. */
	}
    }

    /* Now that the target and availability sets have all been set up, go find
       a match: */
    for (g = 0; g < numTargetGroups; ++g) {
	int a;
	CVMRMregset *targetList = targetGroup[g].targetList;
	size_t numTargetSets = targetGroup[g].numTargetSets;
	for (a = 0; a < numAvailSets; ++a) {
	    int t;
	    CVMRMregset avail = availList[a];
	    for (t = 0; t < numTargetSets; ++t) {
		CVMRMregset target = targetList[t];

                /* Attempt to allocate from the intersection of the current
                   target and availability sets: */
		if (nregs == 1) {
		    target &= avail;
		} else {
		    target &= avail & (avail>>1);
		}

		if (target != CVMRM_EMPTY_SET) {
		    int regNo;
		    CVMassert(avail != CVMRM_EMPTY_SET);
		    regNo = findMaxRegInSet(con, target,
				RM_MIN_INTERESTING_REG, RM_MAX_INTERESTING_REG,
				(nregs>1) ? RM_DOUBLE_REG_ALIGN :
					   RM_SINGLE_REG_ALIGN);
		    if (regNo != -1) {
                        if (okToSpill) {
			    int i;
			    /* special fussing for nregs == 2 */
			    for (i=0; i < nregs; i++ ){
				if (con->occupiedRegisters & (1U<<(regNo+i))){
				    CVMRMResource *rp = con->reg2res[regNo+i];
				    /* If sandbox is in effect, the
                                     * register must come from the
                                     * sandboxRegSet. */
                                    CVMassert((con->sandboxRegSet &
                                              (1U<<(regNo+i))) != 0);
				    CVMassert(!(rp->flags & CVMRMpinned));
				    spillResource(con, rp, CVM_TRUE);
				}
			    }
			}
			return regNo;
		    }
		}
	    }
	}
    }

    CVMassert(regNo == -1);
    return regNo;
}

static int
findAvailableRegister(
    CVMJITRMContext* con,
    CVMRMregset target,
    CVMRMregset avoid,
    CVMRMRegisterPreference pref,
    CVMBool     strict,
    int		nregs)
{
    int reg = findAvailableRegister0(con, target, avoid,
	pref, strict, CVM_TRUE, nregs);
    CVMassert(!strict || reg == -1 || (target & (1U << reg)));
    
#ifdef CVM_JIT_ROUND_ROBIN_REGISTER_SELECTION
    /* SVMC_JIT c5041613 (dk) 2004-01-26. */
    con->lastRegNo = reg;
#endif

    return reg;
}

static void
pinToRegister(
    CVMJITRMContext* con,
    CVMRMResource* rp,
    int regNo)
{
    if (rp->regno != -1 && rp->regno != regNo) {
	/* handle shuffle */
	CVMassert(con->reg2res[rp->regno] == rp);
	/* dirty is OK, but pinned is not */
	CVMassert((rp->flags & CVMRMpinned) == 0);
	con->reg2res[rp->regno] = NULL;
	if (rp->nregs > 1) {
	    CVMassert(con->reg2res[rp->regno + 1] == rp);
	    con->reg2res[rp->regno + 1] = NULL;
	}
    }
    rp->flags |= CVMRMpinned;
    rp->regno = regNo;
    rp->rmask = ((1U << rp->nregs) - 1) << regNo;

    /* pin this resource to this register */
    con->pinnedRegisters |= rp->rmask;
    con->occupiedRegisters |= rp->rmask;

    {
	CVMRMResource* prev = con->reg2res[regNo];
	if (prev != NULL && prev != rp) {
	    CVMassert((prev->flags & CVMRMpinned) == 0);
	    CVMassert((prev->flags & CVMRMdirty) == 0);
	    prev->regno = -1;
	}
    }
    con->reg2res[rp->regno] = rp;
    if (rp->nregs > 1 ){
	CVMassert(rp->nregs == 2);
	con->reg2res[rp->regno + 1] = rp;
    }

    rp->rmContext = con;
}

/*
 * Re-compute a constant into a register based on its value
 */
static void
reloadConstant32(
    CVMJITCompilationContext* con,
    CVMJITRMContext* rx,
    CVMRMResource* rp)
{
    CVMassert(CVMRMisConstant32(rp));
#ifdef CVM_TRACE_JIT
    if (rp->name != NULL) {
        CVMJITaddCodegenComment((con, rp->name));
    } else {
        CVMJITaddCodegenComment((con, "const %d", rp->constant));
    }
    CVMJITsetSymbolName((con, rp->name));
#endif
    (rx->constantLoader32)(con, rp->regno, rp->constant);
}

static void
reloadConstantAddr(
    CVMJITCompilationContext* con,
    CVMJITRMContext* rx,
    CVMRMResource* rp)
{
    CVMassert(CVMRMisConstantAddr(rp));
#ifdef CVM_TRACE_JIT
    if (rp->name != NULL) {
        CVMJITaddCodegenComment((con, rp->name));
    } else {
        CVMJITaddCodegenComment((con, "const %d", rp->constant));
    }
    CVMJITsetSymbolName((con, rp->name));
#endif
    (rx->constantLoaderAddr)(con, rp->regno, rp->constant);
}

/*
 * Re-compute a local into a register
 */
static void
reloadLocal(CVMJITCompilationContext* con, int opcode, CVMRMResource* rp)
{
    CVMassert(CVMRMisLocal(rp));
    CVMCPUemitFrameReference(con, opcode, rp->regno,
                             CVMCPU_FRAME_LOCAL, rp->localNo,
			     CVMRMisConstant(rp));
}

/*
 * reloadRegister() knows how to re-compute a resource into a register.
 * Special-cased are constants, local variables, stack locations,
 * and frame references
 */
static void
reloadRegister(
    CVMJITCompilationContext* con,
    CVMJITRMContext* rx,
    CVMRMResource* rp){
    /*
     * a spilled resource has just been re-pinned
     * and needs to be reloaded.
     */
    if (CVMRMisConstant32(rp)) {
	reloadConstant32(con, rx, rp);
    } else if (CVMRMisConstantAddr(rp)) {
        reloadConstantAddr(con, rx, rp);
    } else if (CVMRMisLocal(rp)) {
	reloadLocal(con, RM_LOAD_OPCODE(rx, rp), rp);
    } else if (CVMRMisJavaStackTopValue(rp)) {
	/*
	 * this is a deferred pop operation.
	 * Make sure that this is the stack top.
	 */
	CVMSMassertStackTop(con, rp->expr);
	if (rp->size == 1){
	    CVMSMpopSingle(con, rp);
	} else {
	    CVMSMpopDouble(con, rp);
	}
        CVMRMclearJavaStackTopValue(rp); /* not on top of the stack any more.*/
        rp->flags |= CVMRMdirty;

    } else if (CVMRMisStackParam(rp)) {
        /* This means we're accessing a value at a specific location on the
           stack: */
        if (rp->size == 1){
            CVMSMgetSingle(con, rp);
        } else {
            CVMSMgetDouble(con, rp);
        }

    } else {
	CVMassert(rp->spillLoc >= 0);
        CVMCPUemitFrameReference(con, RM_LOAD_OPCODE(rx, rp), rp->regno,
            CVMCPU_FRAME_TEMP, rp->spillLoc, CVMRMisConstant(rp));
    }
}

static void
shuffleRegisters( 
    CVMJITRMContext* con,
    CVMRMResource* rp,
    CVMRMregset	target,
    CVMRMregset	avoid,
    CVMRMRegisterPreference pref,
    CVMBool strict)
{
    /* 
     * The resource is currently in a register we want to avoid.
     * We need to find an acceptable register, then move it.
     */
    int regNo;
    int oldRegNo = rp->regno;
    CVMRMregset oldrmask = rp->rmask;
    CVMJITCompilationContext* cc = con->compilationContext;

    CVMRMassertContextMatch(con, rp);
    /*
     * We already know that the resource is not in the target set, so the
     * only way findAvailableRegister() will cause the resource to spill is
     * if the resource is a 64-bit one and its 2nd register overlaps the 
     * target set (we know the first one is not in it). We choose not to pin
     * the resource and risk the wasted spill in this case. It turns out that
     * this is extremely rare and keeping it pinned would result in 
     * findAvailableRegister() failing.
     */
    regNo = findAvailableRegister(con, target, avoid, pref, strict, rp->nregs);
    CVMassert(regNo != -1);

    /*
     * If you expect excess spilling because the resource is not kept pinned,
     * then enable the following code. I could only get it to trigger the
     * printf once when running the entire tck.
     */
#if 0
    if ((oldrmask & con->occupiedRegisters) != oldrmask) {
	CVMconsolePrintf("****May have spilled\n");
    }
#endif

    /*
     * Issue the move instruction(s)
     */
    if (rp->nregs == 1) {
      CVMCPUemitMoveRegister(cc, RM_MOV_OPCODE(con), regNo, oldRegNo,
			     CVMJIT_NOSETCC);    
    } else if (regNo == oldRegNo + 1) {
      CVMCPUemitMoveRegister(cc, RM_MOV_OPCODE(con), regNo+1, oldRegNo+1,
			     CVMJIT_NOSETCC);
      CVMCPUemitMoveRegister(cc, RM_MOV_OPCODE(con), regNo, oldRegNo,
			     CVMJIT_NOSETCC);
    } else {
      CVMCPUemitMoveRegister(cc, RM_MOV_OPCODE(con), regNo, oldRegNo,
			     CVMJIT_NOSETCC);
      CVMCPUemitMoveRegister(cc, RM_MOV_OPCODE(con), regNo+1, oldRegNo+1,
			     CVMJIT_NOSETCC);
    }

    /* The resource no longer occupies the old registers it was in */
    con->occupiedRegisters &= ~oldrmask;

    /*
     * This is necessary in case there is overlap between the new and old
     * registers, which of course can only happen with 64-bit resources.
     */
    if (rp->nregs == 2) {
	/* undo damage done by masking with ~oldrmask */
	con->occupiedRegisters |= ((1U << regNo) | (1U << (regNo + 1)));
    }

    pinToRegister(con, rp, regNo);
}

static CVMRMResource*
newResource(CVMJITRMContext* con)
{
    CVMJITCompilationContext* cc = con->compilationContext;
    CVMRMResource *rp = cc->RMcommonContext.freeResourceList;

    if (rp != NULL) {
	cc->RMcommonContext.freeResourceList = rp->next;
	memset(rp, 0, sizeof *rp);
    } else {
	rp = CVMJITmemNew(cc, JIT_ALLOC_CGEN_REGMAN, sizeof(CVMRMResource));
    }
    return rp;
}

static CVMRMResource*
CVMRMallocateAndPutOnList(CVMJITRMContext* con) 
{
    CVMRMResource* rp = newResource(con);
    CVMJITidentityInitDecoration(con, &rp->dec,
				 CVMJIT_IDENTITY_DECORATION_RESOURCE);
    rp->next = con->resourceList;
    rp->prev = NULL;
    if (con->resourceList != NULL){
	con->resourceList->prev = rp;
    }
    con->resourceList = rp;
    rp->rmContext = con;
    return rp;
}

/*
 * Return register preference based on target and avoid sets.
 * If there is an unsafe register in the target set that is not in
 * the avoid set, then use SCRATCH preference. Otherwise use PERSISTENT.
 */
static CVMRMRegisterPreference
getRegisterPreference(CVMJITRMContext* con,
		      CVMRMregset target, CVMRMregset avoid)
{
#ifdef IAI_ROUNDROBIN_REGISTER_ALLOCATION
    return  CVMRM_REGPREF_TARGET;
#endif

#if 1
    return DEFAULT_RESOURCE_PREF;
#else
    /*
     * NOTE: the following code is disabled since it hasn't been determined
     * to be of any value and may actually hurt performance. This is probably
     * because the avoid set is not accurate enough (due to TEMP nodes and
     * IDENTITY nodes).
     */
    if ((~avoid & target & RM_UNSAFE_SET) != CVMRM_EMPTY_SET) {
	return CVMRM_REGPREF_SCRATCH;
    } else {
	return CVMRM_REGPREF_PERSISTENT;
    }
#endif
}

CVMRMResource*
CVMRMgetResource0(
    CVMJITRMContext* con,
    CVMRMregset	target,
    CVMRMregset	avoid,
    CVMRMRegisterPreference pref,
    CVMBool     strict,
    int		size)
{
    int		   regNo;
    CVMRMResource* rp;
    int		   nregs = RM_NREGISTERS(con, size);

    CVMassert(!con->compilationContext->inConditionalCode);

    regNo = findAvailableRegister(con, target, avoid, pref, strict, nregs);
    /*
     * we have a register (or asserted while trying)
     * so make up a new resource and return it.
     */
    CVMJITpreInitializeResource(con, &rp, size);
    pinToRegister(con, rp, regNo);
    return rp;
}

/*
 * For now, ask for any register.  
 * Eventually, we should probably pass in
 * the expression so we can make a better decision based
 * on reference counts or other usage information.
 */
CVMRMResource*
CVMRMgetResource(
    CVMJITRMContext* con,
    CVMRMregset	target,
    CVMRMregset	avoid,
    int		size)
{
    return CVMRMgetResource0(con, target, avoid,
			     DEFAULT_RESOURCE_PREF, CVM_FALSE, size); 
    /* The orginal default preference always was REGPREF_PERSISTENT. This means 
       that a persistent register is preferredly reclaimed. In the
       case of x86, there is only one integer non-volatile register (ESI), and
       no floating-point non-volatile register at all. Thus this preference does
       not make sense here; it should be possible to change DEFAULT_RESOURCE_PREF.
       To this end, there is a new define CVMCPU_NO_REGPREF_PERSISTENT which has to
       be defined for processors where preferring PERSISTENT regs does not make
       sense. */
}

CVMRMResource*
CVMRMgetPersistentResource(
    CVMJITRMContext* con,
    CVMRMregset	target,
    CVMRMregset	avoid,
    int		size)
{
    return CVMRMgetResource0(con, target, avoid, CVM_FALSE, CVM_FALSE, size);
}

CVMRMResource*
CVMRMgetResourceStrict(
    CVMJITRMContext* con,
    CVMRMregset	target,
    CVMRMregset	avoid,
    int		size)
{
    CVMRMResource* r = CVMRMgetResource0(con, target, avoid, 
				 CVMRM_REGPREF_TARGET, CVM_TRUE, size);
    CVMassert(((1U<<r->regno) & target) != 0);
    return r;
}

CVMRMResource*
CVMRMgetResourceSpecific(
    CVMJITRMContext* con,
    int		target,
    int		size)
{
    CVMRMregset t = 1U << target;
    CVMRMResource* r = CVMRMgetResource0(con, t, ~t,
					 CVMRM_REGPREF_TARGET, CVM_TRUE, size);
    CVMassert(r->regno == target);
    return r;
}

/*
 * Clone a given resource to a target register, and emit move of
 * source contents to target. The returned clone has a reference count
 * of 1.
 */
static CVMRMResource*
CVMRMcloneResourceSpecific(
    CVMJITRMContext* con,
    CVMRMResource *rp,
    int		target)
{
    CVMRMResource *rnew;
    CVMJITCompilationContext* cc = con->compilationContext;

    CVMassert(rp->flags & CVMRMpinned); /* Only clone pinned resources */
    CVMassert(rp->regno != target);     /* Be sensible */

    rnew = CVMRMgetResourceSpecific(con, target, rp->size);

    CVMassert(rnew->regno == target);
    CVMassert(rnew->flags & CVMRMpinned);
    CVMRMassertContextMatch(con, rp);

    /* clone */
    rnew->flags       = rp->flags & (CVMRMpinned|CVMRMref|CVMRMaddr);
    rnew->flags       |= CVMRMclone;
    rnew->expr        = rp->expr;
#ifdef CVM_DEBUG
    rnew->originalTag = rp->originalTag;
#endif
#ifdef CVM_TRACE_JIT
    rnew->name        = rp->name;
#endif
    CVMRMdecRefCount(con, rp);

    /* Emit move from original to clone */
    CVMCPUemitMoveRegister(cc, RM_MOV_OPCODE(con), rnew->regno, rp->regno,
			   CVMJIT_NOSETCC);
    if (rp->nregs > 1) {
	/* Handle 64-bit values too */
        CVMCPUemitMoveRegister(cc, RM_MOV_OPCODE(con),
			       rnew->regno+1, rp->regno+1, CVMJIT_NOSETCC);
    }
    
    return rnew;
}

/* DEBUG */
char * regByKey[] = { "X", "%r", "%f" };


/*
 * Clone a given resource to another resource in the other register bank.
 * The target pinned and loaded.
 * The reference count on the spill location is incremented.
 */
extern CVMRMResource *
CVMRMcloneResource(
    CVMJITRMContext* srcCx, CVMRMResource* src,
    CVMJITRMContext* destCx,
    CVMRMregset target, CVMRMregset avoid)
{
    CVMRMResource* rnew;
    int spilloff;

    CVMRMassertContextMatch(srcCx, src);

    if (CVMRMisConstant32(src)) {
	CVMassert(src->size == 1);
	rnew = CVMRMgetResourceForConstant32(destCx, target, avoid, 
					     src->constant);
	return rnew;
    }
    else if (CVMRMisConstantAddr(src)) {
	CVMassert(src->size == 1);
	rnew = CVMRMgetResourceForConstantAddr(destCx, target, avoid, 
					     src->constant);
	return rnew;
    }
    /*
     * Make sure the spill area is consistent with the src register
     */
    if ((src->regno >= 0) && (src->flags & CVMRMdirty)){
	flushResource(srcCx, src);
	src->flags ^= CVMRMdirty;
    }
    /*
     * now clone it.
     */
    rnew	      = CVMRMgetResource( destCx, target, avoid, src->size);
    rnew->flags       = src->flags | CVMRMclone | CVMRMpinned;
    rnew->nregs	      = RM_NREGISTERS(destCx, rnew->size); /* recompute nregs! */
    rnew->spillLoc    = src->spillLoc;
    rnew->localNo     = src->localNo;
    rnew->expr        = src->expr;
    rnew->constant    = src->constant;
    /* SVMC_JIT PJ 2004-06-28
                  correct syntax error in opt build
    */
#ifdef CVM_DEBUG
    rnew->originalTag = src->originalTag;
#endif
#ifdef CVM_TRACE_JIT
    rnew->name        = src->name;
#endif
    /*
     * Incremement ref count on spill location, if any
     */
    spilloff = src->spillLoc;
    if (spilloff >= 0){
	incrementSpillCount(destCx->compilationContext, spilloff, src->size);
    }
    /*
     * LOAD THE DATA & DISASSOCIATE FROM LOCAL, IF ANY.
     */
    reloadRegister( destCx->compilationContext, destCx, rnew);
    if (CVMRMisLocal(rnew)){
	/* if it has to do a writeback, do it somewhere else */
	rnew->flags = (rnew->flags & ~CVMRMLocalVar) | CVMRMdirty;
        rnew->localNo     = CVMRM_INVALID_LOCAL_NUMBER;
    }
    /* DEBUG
    CVMconsolePrintf("CLONING resource 0x%x (%s%d) to resource 0x%x (%s%d)\n",
	src, regByKey[src->key], src->regno,
	rnew, regByKey[rnew->key], rnew->regno);
     */
    return rnew;
}

/*
 * Allocate a resource to a temporary value which is on the Java stack.
 * Defer pinning until needed.
 */
CVMRMResource*
CVMRMbindStackTempToResource(
    CVMJITRMContext* con,
    CVMJITIRNode* expr,
    int size)
{
    CVMRMResource* rp;
    rp = CVMRMallocateAndPutOnList(con);
    /* could probably tell this by looking at node tag */
    rp->flags = CVMRMjavaStackTopValue;
    rp->size = size;
    rp->nregs = RM_NREGISTERS(con, size);
    rp->expr = expr;
    rp->spillLoc = -1;
    rp->localNo = CVMRM_INVALID_LOCAL_NUMBER;
    rp->constant = -1;
    rp->regno = -1;
    CVMRMsetRefCount(con, rp, CVMJITirnodeGetRefCount(expr));
    if (CVMJITirnodeIsReferenceType(expr)) {
	rp->flags |= CVMRMref;
    } else if (CVMJITirnodeIsAddressType(expr)) {
	rp->flags |= CVMRMaddr;
    }
    return rp;
}

/* Purpose: Convert a value at the top of the Java stack into a param that is
            stored on the stack but may not be at the top of the top of the
            stack anymore because other params may be pushed on top of it. */
void
CVMRMconvertJavaStackTopValue2StackParam(CVMJITCompilationContext *con,
                                         CVMRMResource *rp)
{
    CVMassert(CVMRMisJavaStackTopValue(rp));
    CVMSMassertStackTop(con, rp->expr);
    CVMRMclearJavaStackTopValue(rp);
    CVMRMsetStackParam(rp);
    rp->stackLoc = CVMSMgetStackDepth(con);
}

static void
deleteResource( CVMJITRMContext* con, CVMRMResource* rp ){
  if (con->resourceList == rp )
    con->resourceList = rp->next;
  if (rp->prev != NULL)
    rp->prev->next = rp->next;
  if (rp->next != NULL)
    rp->next->prev = rp->prev;
  rp->flags = CVMRMtrash;
  if (rp->regno != -1){
    con->occupiedRegisters &= ~rp->rmask;
    con->reg2res[rp->regno] = NULL;
    if (rp->nregs == 2 ){
      con->reg2res[rp->regno + 1] = NULL;
    }
#ifdef CVM_DEBUG
	rp->regno = 127; /* make it look trashy */
#endif
  }
    rp->next = con->freeResourceList;
    con->freeResourceList = rp;
}

/*
 * Make sure that a given resource is in a register, in accordance
 * with 'target' and 'avoid'
 */
static void
ensureInRegister(CVMJITRMContext* con,
		 CVMRMregset    target,
		 CVMRMregset	avoid,
		 CVMRMRegisterPreference pref,
		 CVMBool	strict,
		 CVMRMResource* rp)
{
    CVMRMincRefCount(con, rp);
    CVMRMpinResource0(con, rp, target, avoid, pref, strict);
}

/*
 * TBD: Constants should get their own list. If constants
 * get their own table, associateResourceWithConstant32()
 * also needs to be updated.
 */
static CVMRMResource*
findResourceConstant32(CVMJITRMContext* con,
		       CVMInt32 constant)
{
    CVMRMResource* rp;
    
    for (rp = con->resourceList; rp != NULL; rp = rp->next) {
	if (CVMRMisConstant32(rp)) {
	    if (rp->constant == constant) {
		return rp;
	    }
	}
    }
    return NULL;
}

static CVMRMResource*
findResourceConstantAddr(CVMJITRMContext* con,
		       CVMAddr constant)
{
    CVMRMResource* rp;
    
    for (rp = con->resourceList; rp != NULL; rp = rp->next) {
	if (CVMRMisConstantAddr(rp)) {
	    if (rp->constant == constant) {
		return rp;
	    }
	}
    }
    return NULL;
}

/*
 * This is a no-op currently. If and when constants get their
 * own table, this has to do something.
 */
static void
associateResourceWithConstant32(CVMJITRMContext* con,
				CVMRMResource* rp,
				CVMInt32 constant)
{
    CVMassert(findResourceConstant32(con, constant) != NULL);
}

static void
associateResourceWithConstantAddr(CVMJITRMContext* con,
				CVMRMResource* rp,
				CVMAddr constant)
{
    CVMassert(findResourceConstantAddr(con, constant) != NULL);
}

/*
 * Handle special 32-bit constants:
 *    Common immediates such as #0
 *    PC-relative constants:  PC+off
 */
CVMRMResource*
CVMRMbindResourceForConstant32(
    CVMJITRMContext* con,
    CVMInt32 constant)
{
    CVMRMResource* rp;

    CVMassert(!con->compilationContext->inConditionalCode);

#ifdef CVMCPU_HAS_ZERO_REG
    if (constant == 0 && con->preallocatedResourceForConstantZero != NULL) {
        rp = con->preallocatedResourceForConstantZero;
    } else 
#endif
    {
        rp = findResourceConstant32(con, constant); 
    }  
    if (rp != NULL) {
	CVMassert(CVMRMisConstant32(rp));
	CVMRMincRefCount(con, rp);
    } else {
	rp = CVMRMallocateAndPutOnList(con);
	rp->size = 1;
	rp->nregs= 1;
	rp->spillLoc = -1;
        rp->localNo = CVMRM_INVALID_LOCAL_NUMBER;
	rp->expr = NULL;
	rp->regno = -1;
	rp->flags |= CVMRMConstant32;
	rp->constant = constant;
#ifdef CVM_TRACE_JIT
        rp->name = CVMJITgetSymbolName(con->compilationContext);
        CVMJITresetSymbolName(con->compilationContext);
#endif
	/* Add this to our constant-resource mapping */
	associateResourceWithConstant32(con, rp, constant);
    }
    return rp;
}

CVMRMResource*
CVMRMbindResourceForConstantAddr(
    CVMJITRMContext* con,
    CVMAddr constant)
{
    CVMRMResource* rp;

    CVMassert(!con->compilationContext->inConditionalCode);

#ifdef CVMCPU_HAS_ZERO_REG
    if (constant == 0 && con->preallocatedResourceForConstantZero != NULL) {
        rp = con->preallocatedResourceForConstantZero;
    } else 
#endif
    {
        rp = findResourceConstantAddr(con, constant); 
    }  
    if (rp != NULL) {
	CVMassert(CVMRMisConstantAddr(rp));
	CVMRMincRefCount(con, rp);
    } else {
	rp = CVMRMallocateAndPutOnList(con);
	rp->size = 1;
	rp->nregs= 1;
	rp->spillLoc = -1;
        rp->localNo = CVMRM_INVALID_LOCAL_NUMBER;
	rp->expr = NULL;
	rp->regno = -1;
	rp->flags |= CVMRMConstantAddr;
	rp->constant = constant;
#ifdef CVM_TRACE_JIT
        rp->name = CVMJITgetSymbolName(con->compilationContext);
        CVMJITresetSymbolName(con->compilationContext);
#endif
	/* Add this to our constant-resource mapping */
	associateResourceWithConstantAddr(con, rp, constant);
    }
    return rp;
}

/*
 * Handle special 32-bit constants:
 *    Common immediates such as #0
 *    PC-relative constants:  PC+off
 */
CVMRMResource*
CVMRMgetResourceForConstant32(
    CVMJITRMContext* con,
    CVMRMregset	target,
    CVMRMregset	avoid,
    CVMInt32	constant)
{
    CVMRMResource* rp;

    rp = CVMRMbindResourceForConstant32(con, constant);
    CVMassert(CVMRMisConstant32(rp));
    /* Make sure it is in a register */
    ensureInRegister(con, target, avoid, DEFAULT_CONSTANT_PREF, CVM_FALSE, rp);
    return rp;
}

CVMRMResource*
CVMRMgetResourceForConstantAddr(
    CVMJITRMContext*  con,
    CVMRMregset	target,
    CVMRMregset	avoid,
    CVMAddr	constant)
{
    CVMRMResource* rp;

    rp = CVMRMbindResourceForConstantAddr(con, constant);
    CVMassert(CVMRMisConstantAddr(rp));
    /* Make sure it is in a register */
    ensureInRegister(con, target, avoid, DEFAULT_CONSTANT_PREF, CVM_FALSE, rp);
    return rp;
}

/*
 * Create a resource for a local, but don't load the local into the registers.
 */
CVMRMResource*
CVMRMbindResourceForLocal( 
    CVMJITRMContext* rc,
    int		size,
    CVMBool     isRef,
    CVMInt32	localNumber)
{
    CVMRMResource* rp = rc->local2res[localNumber];
#ifdef CVM_DEBUG_ASSERTS
    CVMJITCompilationContext* con = rc->compilationContext;
#endif
    CVMassert(!con->inConditionalCode);

    if (rp != NULL) {
	/* local already has a resource */
	CVMassert(size == rp->size);
	CVMRMincRefCount(rc, rp);
    } else {
	/* allocate a new resource for this local */
	rp = CVMRMallocateAndPutOnList(rc);
	rp->size = size;
	rp->nregs = RM_NREGISTERS(rc, size);
	rp->spillLoc = -1;
	rp->constant = -1;
	rp->expr = NULL;
	rp->regno = -1;
	rp->flags = CVMRMLocalVar;
	rp->localNo = localNumber;
	CVMassert(rc->local2res[localNumber] == NULL);
	rc->local2res[localNumber] = rp;
	if (size == 2) {
	    rc->local2res[localNumber + 1] = rp;
	}
	if (isRef) {
	    rp->flags |= CVMRMref;
	}
    }

#ifdef CVMJIT_STRICT_REFLOCAL_ASSERTS
    CVMassert(isRef == CVMRMisRef(rp)); /* Make sure we agree */
    {
	CVMBool localIsRef;
	CVMJITsetContains(&con->localRefSet, localNumber, localIsRef);
	CVMassert(localIsRef == isRef);
    }
#endif
    return rp;
}

CVMRMResource*
CVMRMgetResourceForLocal(
    CVMJITRMContext* rc,
    CVMRMregset	target,
    CVMRMregset	avoid,
    int		size,
    CVMBool     isRef,
    CVMBool     strict,
    CVMInt32	localNumber)
{
    /* get resource for local */
    CVMRMResource* rp =
	CVMRMbindResourceForLocal(rc, size, isRef, localNumber);
    /* load local into register */
    ensureInRegister(rc, target, avoid,
		     getRegisterPreference(rc, target, avoid),
		     strict, rp);
    return rp;
}


static void
flushLocal(CVMJITRMContext* con, CVMInt32 localNumber)
{
    CVMRMResource *rp = con->local2res[localNumber];
    CVMJITCompilationContext* cc = con->compilationContext;

    if (rp != NULL) {
	/* Make sure we got a resource representing a local */
	CVMassert(CVMRMisLocal(rp));
	/* not good any more. */
	/* if it was lingering just for that sake, get rid of it */
	if (CVMRMgetRefCount(con, rp) == 0) {
	    if (rp->size == 2) {
		CVMassert(rp->localNo == localNumber ||
			rp->localNo + 1 == localNumber);
		localNumber = rp->localNo;
		CVMassert(con->local2res[localNumber + 1] == rp);
		con->local2res[localNumber + 1] = NULL;
	    }
	    con->local2res[localNumber] = NULL;
	    deleteResource(con, rp);
	} else {
	    if (rp->regno == -1) {
		/*
		 * This is a hideous special case.
		 * we have a value in a resource shadowing the local,
		 * BUT it's been spilled, presumably to its java local,
		 * AND we need this value in the future (refCount >0).
		 * This means that we must RE-LOAD the value into a register
		 * so that it is preserved. If necessary, it can be spilled
		 * to the spill area, but not back to the register.
		 * We are pretty desparate here, so are not going to be
		 * picky about register selection. We'll just re-spill if
		 * we need to.
		 */
		int regNo = findAvailableRegister(con, RM_ANY_SET,
		    CVMRM_EMPTY_SET, CVM_FALSE, CVM_FALSE, rp->nregs);
		pinToRegister(con, rp, regNo);
		reloadRegister(cc, con, rp);
		CVMRMunpinResource(con, rp);
		/*
		 * It is now safe to do the store.
		 */
	    }
	    CVMassert(con->local2res[localNumber] == rp);
	    if (rp->size == 2) {
		CVMassert(rp->localNo == localNumber ||
			rp->localNo + 1 == localNumber);
		localNumber = rp->localNo;
		CVMassert(con->local2res[localNumber + 1] == rp);
		con->local2res[localNumber + 1] = NULL;
	    }
	    con->local2res[localNumber] = NULL;
	    CVMassert(rp->localNo == localNumber);
	    rp->flags &= ~CVMRMLocalVar;
	    /* disassociate from the local */
            rp->localNo = CVMRM_INVALID_LOCAL_NUMBER;
	    /* and in any case, it is now out-of-sync with
	     * its (new) spill location.
	     */
	    rp->flags |= CVMRMdirty;
	}
    }
}

/*
 * Like CVMRMstoreJavaLocal, but
 * does not do as much work.
 */
extern void
CVMRMstoreReturnValue(CVMJITRMContext* con, CVMRMResource* valueToStore)
{
  int is_imm;
  CVMJITCompilationContext* cc = con->compilationContext;
  CVMassert(!(valueToStore->flags & CVMRMtrash));
  CVMRMassertContextMatch(con, valueToStore);

  is_imm = CVMRMisConstant(valueToStore);
  CVMCPUemitFrameReference(cc, RM_STORE_OPCODE(con, valueToStore),
			   (is_imm)?valueToStore->constant:CVMRMgetRegisterNumber(valueToStore), 
			   CVMCPU_FRAME_LOCAL, 0, CVMRMisConstant(valueToStore));
}

/*
 * get the opcode appropriate for storing this resource to memory
 */
extern CVMInt32
CVMRMgetStoreOpcode(CVMJITRMContext* con, CVMRMResource* valueToStore){
    CVMassert(!(valueToStore->flags & CVMRMtrash));
    CVMRMassertContextMatch(con, valueToStore);
    return RM_STORE_OPCODE(con, valueToStore);
}

/*
 * get the opcode appropriate for loading this resource from memory
 */
extern CVMInt32
CVMRMgetLoadOpcode(CVMJITRMContext* con, CVMRMResource* valueToStore){
    CVMassert(!(valueToStore->flags & CVMRMtrash));
    CVMRMassertContextMatch(con, valueToStore);
    return RM_LOAD_OPCODE(con, valueToStore);
}

void
CVMRMstoreJavaLocal(
    CVMJITRMContext* con,
    CVMRMResource* valueToStore,
    int		size,
    CVMBool     isRef,
    CVMInt32	localNumber)
{
    CVMJITCompilationContext* cc = con->compilationContext;
    int	        nregs;
    int         is_imm=0;
    CVMassert(!con->compilationContext->inConditionalCode);
    CVMassert( !(valueToStore->flags & CVMRMtrash) );
    CVMRMassertContextMatch(con, valueToStore);
    nregs = RM_NREGISTERS(con, size);
	
#ifdef CVM_JIT_USE_FP_HARDWARE
    {
	/* If there are two register banks, make sure to 
	 * invalidate entries in the other one!!
	 */
	int contextIndex;
	CVMJITRMContext* otherContext;
	contextIndex = con - &(cc->RMcontext[0]);
	/*
	 * C pointer arithmetic gives the index of the element: 0 or 1.
	 * now find the other one:
	 * map 0=>1 or 1=>0
	 */
	contextIndex ^= 1;
	otherContext = &(cc->RMcontext[contextIndex]);
	flushLocal(otherContext, localNumber);
	if (nregs == 2) {
	    flushLocal(otherContext, localNumber + 1);
	}
    }
#endif
    flushLocal(con, localNumber);
    if (nregs == 2) {
	flushLocal(con, localNumber + 1);
    }

    if (!(valueToStore->flags & CVMRMpinned))
	is_imm = CVMRMisConstant(valueToStore);
    else
	CVMassert(valueToStore->regno >= 0);

    CVMCPUemitFrameReference(cc, RM_STORE_OPCODE(con, valueToStore),
			     (is_imm)?valueToStore->constant:CVMRMgetRegisterNumber(valueToStore), 
			     CVMCPU_FRAME_LOCAL, localNumber, is_imm);

    valueToStore->flags &= ~CVMRMdirty; /* did store, so no longer dirty */
    if (!CVMRMisLocal(valueToStore)) {
	/* now associate it with this local */
	valueToStore->localNo = localNumber;
	valueToStore->flags |= CVMRMLocalVar;
	CVMassert(con->local2res[localNumber] == NULL);
	con->local2res[localNumber] = valueToStore;
	if (size == 2) {
	    con->local2res[localNumber + 1] = valueToStore;
	}
#ifdef CVMJIT_STRICT_REFLOCAL_ASSERTS
	if (isRef != CVMRMisRef(valueToStore)) {
	    if (CVMRMisConstant(valueToStore)) {
		CVMassert(valueToStore->constant == 0);
	    } else {
		CVMassert(CVM_FALSE);
	    }
	}
#endif
    }

    if (isRef) {
	CVMJITlocalrefsSetRef(cc, &cc->localRefSet, localNumber);
    } else {
	CVMJITlocalrefsSetValue(cc, &cc->localRefSet, localNumber, size == 2);
    }

#if 0
    /* DEBUGGING ONLY

    CVMconsolePrintf("DIGESTED LOCALREFS NODE: ");

    CVMJITsetDumpElements(con, &refsNode->refLocals);
    CVMconsolePrintf("\n");

    */
#endif
}

static void
CVMRMpinResource0(
    CVMJITRMContext* con,
    CVMRMResource* rp,
    CVMRMregset target,
    CVMRMregset avoid,
    CVMRMRegisterPreference pref,
    CVMBool strict)
{
    CVMCodegenComment *comment;
    CVMJITCompilationContext* cc = con->compilationContext;
    int regNo;
    CVMassert(!con->compilationContext->inConditionalCode);
    CVMassert( !(rp->flags & CVMRMtrash) );
    if (rp->flags & CVMRMpinned) {
	return; /* already pinned */
    } else if (rp->regno != -1) {
	/*
	 * already in a register. 
	 * If we're lucky, it's in the target set and we
	 * can just pin it in.
	 * Otherwise, we have to move it. Grumble.
	 */
	CVMRMregset rmask = 1U << rp->regno;
	/*
	 * When strict is set, we choose not to shuffle if
	 * the register is in the target set, even if it is
	 * also in the avoid set.
	 *
	 * Note the use of rmask with target and rp->rmask
	 * with avoid.  This is because double word resources
	 * interpret the target set as a "start set".  So
	 * Rn,Rn+1 is allowed if Rn is in the target set, even
	 * if Rn+1 is not.
	 */
#ifdef SHUFFLE_FOR_AVOID
	if (strict ? !(rmask & target) : (rp->rmask & avoid))
#else
	if (strict && !(rmask & target))
#endif
	{
	    /* Currently in a register we want to avoid.
	     * Needed to shuffle it into an acceptable one.
	     */
            CVMJITpopCodegenComment(cc, comment);
	    shuffleRegisters(con, rp, target, avoid, pref, strict);
            CVMJITpushCodegenComment(cc, comment);
	} else {
	    /* resource is not in a register we need to avoid */
	    con->pinnedRegisters |= rp->rmask;
	    rp->flags |= CVMRMpinned;
	}
	return;
    }
    CVMJITpopCodegenComment(cc, comment);
    regNo = findAvailableRegister(con, target, avoid,
	pref, strict, rp->nregs);
    CVMassert(regNo != -1);
    pinToRegister(con, rp, regNo);
#ifdef CVM_TRACE_JIT
    rp->name = NULL;
    CVMJITresetSymbolName(con->compilationContext);
#endif
    reloadRegister(cc, con, rp);
    CVMJITpushCodegenComment(cc, comment);
}

/*
 * For now, ask for any register.
 * Eventually, we should probably pass in
 * the expression so we can make a better decision based
 * on reference counts or other usage information.
 */
void
CVMRMpinResource(
    CVMJITRMContext* con,
    CVMRMResource* rp,
    CVMRMregset target,
    CVMRMregset avoid)
{
    CVMRMassertContextMatch(con, rp);
    CVMRMpinResource0(con, rp, target, avoid,
		      DEFAULT_RESOURCE_PREF, CVM_FALSE);
}

/*
 * Be sure to respect the target and avoid sets strictly.
 */
void
CVMRMpinResourceStrict(
    CVMJITRMContext* con,
    CVMRMResource* rp,
    CVMRMregset target,
    CVMRMregset avoid)
{
    CVMRMassertContextMatch(con, rp);
    CVMRMpinResource0(con, rp, target, avoid, CVMRM_REGPREF_TARGET, CVM_TRUE);
}

CVMRMResource *
CVMRMpinResourceSpecific(
    CVMJITRMContext* con,
    CVMRMResource* rp,
    int target)
{
    CVMRMregset t = 1U << target;

    /* If the resource is already pinned, then we may have to clone the
       resource if it is not pinned to the register we want: */
    CVMRMassertContextMatch(con, rp);
    if (rp->flags & CVMRMpinned) {
        if (rp->regno != target) {
            rp = CVMRMcloneResourceSpecific(con, rp, target);
            CVMassert(rp->regno == target);
        }
        return rp;
    }

    /* Go ahead and pin the resource: */
    CVMRMpinResourceStrict(con, rp, t, ~t);
    CVMassert(rp->regno == target);
    return rp;
}

/*
 * Pin the resource, but only if it is believed that pinning eagerly
 * will be desirabable. Pinning eagerly is desirable if it appears
 * that the register will not get spilled before used, and will not
 * require the spilling of another register.
 */
void
CVMRMpinResourceEagerlyIfDesireable(
    CVMJITRMContext* con, CVMRMResource* rp,
    CVMRMregset target, CVMRMregset avoid)
{
    int avail;

    /* if the resource is already in a register, don't try to pin it.
     * This may result in moving it into a different register, which
     * we'd rather do lazily. */
    if (rp->regno != -1) {
	return;
    }

    /*
     * We require that the target register not be in the avoid set. Otherwise
     * it will get trashed before used. This happens, for example, when
     * the lhs of an expression is a local and the rhs contains a method
     * call which would trash the local if loaded eagerly.
     */
    avail = ~avoid;

    /* Don't pin to an occupied register, since this will cause a spill.
     * We don't want to be eager about pinning if it requires a spill
     * of another register. */
    avail &= ~con->occupiedRegisters;

    /* Don't attempt to pin to a pinned register or we'll get an assert */
    avail &= ~con->pinnedRegisters;

    /* Don't pin to a register outside the sandboxRegSet*/
    avail &= con->sandboxRegSet;

    /* limit the target set to the available registers */
    if (rp->nregs == 1) {
	target &= avail;
    } else {
	target &= avail & (avail>>1) & ~0x80000000;
    }

    /* Pin if there appears to be a desirable and available target register.*/
    if (target != CVMRM_EMPTY_SET) {
	/* double check to make sure a register is available. */
	int regNo = findMaxRegInSet(con, target,
				    RM_MIN_INTERESTING_REG,
				    RM_MAX_INTERESTING_REG,
				    (rp->nregs>1) ? RM_DOUBLE_REG_ALIGN :
				                    RM_SINGLE_REG_ALIGN);
	if (regNo != -1) {
	    /*CVMconsolePrintf("EAGER PIN (%d)\n",regNo);*/
	    CVMRMpinResourceSpecific(con, rp, regNo);
	}
    }
}

void
CVMRMunpinResource(CVMJITRMContext* con, CVMRMResource* rp)
{
    CVMassert( !(rp->flags & CVMRMtrash) );
    CVMassert(!con->compilationContext->inConditionalCode);
    CVMRMassertContextMatch(con, rp);

#ifdef CVMCPU_HAS_ZERO_REG
    if (rp == con->preallocatedResourceForConstantZero) {
        return;
    } else 
#endif
    if (rp->flags & CVMRMpinned){
	rp->flags &= ~CVMRMpinned;
	con->pinnedRegisters &= ~rp->rmask;
    }
}


void
CVMRMoccupyAndUnpinResource(
    CVMJITRMContext* con,
    CVMRMResource* rp,
    CVMJITIRNode* expr)
{
    CVMassert( !(rp->flags & CVMRMtrash) );
    CVMRMassertContextMatch(con, rp);
    rp->flags |= CVMRMoccupied;
    /* 
     * If the expression occupying the register is the same one we just
     * did a loadJavaLocal from, then don't set the dirty flag,
     * because it isn't dirty: i.e. is not out of sync with respect to
     * backing store (the local). In all other cases, this flag must be set.
     */
    if (!((expr != NULL) && 
	  CVMJITirnodeIsLocalNode(expr) && CVMRMisLocal(rp) &&
	  (rp->localNo == CVMJITirnodeGetLocal(expr)->localNo))) {
	rp->flags |= CVMRMdirty;
    }
    
    if (expr != NULL) {
	CVMBool isRef = CVMJITirnodeIsReferenceType(expr);

	/*
	 * Associate 'rp' with this expression if it's an identity node,
	 * and set reference count appropriately.
	 */
	CVMJITidentitySetDecoration(con->compilationContext,
				    (CVMJITIdentityDecoration*)rp, expr);
	
	/* DEBUG
	    if (CVMRMgetRefCount(con, rp) > 1 ){
		CVMconsolePrintf("Expr node with ref count %d\n", 
		CVMRMgetRefCount(con, rp));
                CVMJITirdumpIRNode(NULL, expr, 0, "   ");
		CVMconsolePrintf("\n   > \n\n");
	    }
	*/

	/* if there is another alias for this register, then it
	 * had better have the same reference state.
	 * Try to ignore constants, such as 0, for purposes of this check.
	 */
	if (CVMRMisRef(rp)){
 	    CVMassert(isRef || CVMRMisConstantAddr(rp));
	}
	if (isRef){
	    rp->flags |= CVMRMref;
	} else if (CVMJITirnodeIsAddressType(expr)) {
	    rp->flags |= CVMRMaddr;
	}

#ifdef CVM_DEBUG
	if (!CVMJITirnodeIsIdentity(expr) && 
	    CVMJITirnodeGetRefCount(expr) > 1) {
	    if (!CVMJITirnodeIsConstant32Node(expr) &&
                !CVMJITirnodeIsConstantAddrNode(expr) &&
		!CVMJITirnodeIsConstant64Node(expr)){
		CVMconsolePrintf("Expr node with ref count >1\n" );
		CVMtraceJITBCTOIRExec({
		    CVMJITirdumpIRNodeAlways(con->compilationContext, expr, 0, "   ");
		    CVMconsolePrintf("\n   > \n\n");
		});
	    }
	}
#endif
	    
    } else {
	rp->flags |= CVMRMdirty;
    }
    CVMRMunpinResource(con, rp);
}

CVMRMResource*
CVMRMfindResource(
    CVMJITRMContext* con,
    CVMJITIRNode* expr)
{
    CVMJITCompilationContext* cc = con->compilationContext;
    CVMRMResource* rp = NULL;

    CVMassert(CVMJITirnodeIsIdentity(expr));
    rp = (CVMRMResource*)CVMJITidentityGetDecoration(cc, expr);
    
    CVMassert((rp == NULL) ||
	      CVMJITidentityDecorationIs(cc, expr, RESOURCE));
    
    /* Don't want to pin it -- just find it! */
    if ( (rp == NULL) || (rp->flags & CVMRMtrash)){
	return NULL;
    }
    return rp;
}

void
CVMRMrelinquishResource( CVMJITRMContext* con, CVMRMResource* rp )
{
  CVMRMunpinResource(con, rp);
  /* We should never decrement below 0. */
  CVMassert(!con->compilationContext->inConditionalCode);
  CVMassert(CVMRMgetRefCount(con, rp) > 0); 
  CVMRMdecRefCount(con, rp);
  if (CVMRMgetRefCount(con, rp) <= 0) {
    if ((rp->flags & (CVMRMLocalVar|CVMRMConstant32|CVMRMConstantAddr)) == 0) {
      /* no local, not a constant, no references, no reason to retain */
      if ((!CVMRMisBoundUse(rp)) && rp->spillLoc >= 0) {
	/* CVMRMphi use spillLoc, but aren't managed the same. */
	/* CVMRMclone uses spillLoc, but let original get rid of
	   spill loc */
	relinquishSpillLoc(con->compilationContext,
			   rp->spillLoc, rp->size);
      }
      deleteResource(con, rp);
    }
  }
}

void
CVMRMsynchronizeJavaLocals(CVMJITCompilationContext* con){
    /* does nothing since we always write back immediately */
    CVMassert(!con->inConditionalCode);
}

int
CVMRMdirtyJavaLocalsCount(CVMJITCompilationContext *con)
{
    /* This needs to change if we allow dirty locals */
    CVMassert(!con->inConditionalCode);
    return 0;
}

#ifdef CVMCPU_HAS_ZERO_REG
static void 
preallocateResourceForConstantZero(CVMJITRMContext* con)
{
    CVMRMResource* rp = newResource(con);

    CVMJITidentityInitDecoration(con, &rp->dec,
                                 CVMJIT_IDENTITY_DECORATION_RESOURCE);
    rp->rmContext = con;
    rp->next = NULL;
    rp->prev = NULL;
    rp->size = 1;
    rp->nregs = 1;
    rp->constant = 0;
    rp->regno = CVMCPU_ZERO_REG;
    rp->rmask = 0;
    rp->spillLoc = -1;
    rp->localNo = CVMRM_INVALID_LOCAL_NUMBER;
    rp->flags = CVMRMpinned | CVMRMConstant32 | CVMRMConstantAddr;
    con->preallocatedResourceForConstantZero = rp;

}
#endif

static void
RMContextInit(
    CVMJITCompilationContext* cc,
    CVMJITRMContext* con,
    CVMUint16   minInteresting,
    CVMUint16   maxInteresting,
    CVMRMregset phiRegSet,
    CVMRMregset busySet,
    CVMRMregset safeSet,
    CVMRMregset unsafeSet,
    CVMRMregset anySet,
    CVMUint32   loadopcode1,
    CVMUint32   loadopcode2,
    CVMUint32   storeopcode1,
    CVMUint32   storeopcode2,
    CVMUint32   movopcode,
    int		singleRegAlignment,
    int		doubleRegAlignment,
    int		maxRegSize,
    CVMJITRMConstantLoader32   ldc32,
    CVMJITRMConstantLoaderAddr ldcAddr
  ) {
    con->compilationContext = cc;
    con->numberLocalWords = cc->numberLocalWords;
    con->resourceList = NULL;
    con->minInteresting = minInteresting;
    con->maxInteresting = maxInteresting;
    con->loadOpcode[0] = loadopcode1;
    con->loadOpcode[1] = loadopcode2;
    con->storeOpcode[0]= storeopcode1;
    con->storeOpcode[1]= storeopcode2;
    con->movOpcode     = movopcode;
    con->reg2res = CVMJITmemNew(cc, JIT_ALLOC_CGEN_REGMAN, 
	sizeof(CVMRMResource *) * (maxInteresting + 1));
    con->local2res = CVMJITmemNew(cc, JIT_ALLOC_CGEN_REGMAN, 
	sizeof(CVMRMResource *) * con->numberLocalWords);
#ifdef CVM_JIT_REGISTER_LOCALS
    con->pinnedOutgoingLocals = CVMJITmemNew(cc, JIT_ALLOC_CGEN_REGMAN,
        sizeof(CVMRMResource *) * con->numberLocalWords);
#endif

    /* NOTE: The following need to be adjusted when we want to reserve regs
       for shared values across blocks due to global CSE.
       We should allocate global CSE regs from phiRegSet.
       RMbusySet need to have the corresponding bits for the global CSE regs
       set.
    */
    con->phiRegSet = phiRegSet;
    con->busySet = busySet;
    con->safeSet = safeSet;
    con->unsafeSet = unsafeSet;
    con->anySet = anySet;
    con->singleRegAlignment = singleRegAlignment;
    con->doubleRegAlignment = doubleRegAlignment;
    con->maxRegSize     = maxRegSize;
    con->constantLoader32   = ldc32;
    con->constantLoaderAddr = ldcAddr;
#ifdef CVM_JIT_ROUND_ROBIN_REGISTER_SELECTION
    /* SVMC_JIT d022609 (ML) 2004-05-18. */
    con->lastRegNo = maxInteresting; 
#endif
}

/* SVMC_JIT PJ 2004-07-01
              purify emitter's "constant loader" interface
*/
/* don't ferget to complete this upon porting to 64 bit, see RISC version */
#define constantFPAddrLoader CVMCPUemitLoadConstantFP

void
CVMRMinit(CVMJITCompilationContext* con)
{
    CVMJITRMCommonContext* common = &(con->RMcommonContext);
    con->maxTempWords = con->maxPhiSize;
    /* initialize the common context information ourselves */
    CVMJITsetInit(con, &(common->spillBusySet));
    CVMJITsetInit(con, &(common->spillRefSet));
    common->spillBusyRefSize = RM_INITIAL_REF_COUNT_SIZE;
    common->spillBusyRefCount = CVMJITmemNew(con, JIT_ALLOC_CGEN_REGMAN,
				 RM_INITIAL_REF_COUNT_SIZE*sizeof(CVMUint8));
    /* go initialize the individual regman context(s) */
    RMContextInit(con, &con->RMcontext[0], CVMCPU_MIN_INTERESTING_REG,
	   CVMCPU_MAX_INTERESTING_REG,
	   CVMCPU_PHI_REG_SET,
	   CVMRM_BUSY_SET, CVMRM_SAFE_SET, CVMRM_UNSAFE_SET, CVMRM_ANY_SET,
	   CVMCPU_LDR32_OPCODE, CVMCPU_LDR64_OPCODE,
	   CVMCPU_STR32_OPCODE, CVMCPU_STR64_OPCODE,
	   CVMCPU_MOV_OPCODE,
	   CVMCPU_SINGLE_REG_ALIGNMENT, CVMCPU_DOUBLE_REG_ALIGNMENT,
	   CVMCPU_MAX_REG_SIZE,
	   CVMCPUemitLoadConstant, 
#ifdef CVM_JIT_POSTPASS_OPTIMZE_CODE
	   CVMCPUemitLoadRelocAddrConstant
#else
	   CVMCPUemitLoadAddrConstant
#endif
       );
#ifdef CVMCPU_HAS_ZERO_REG
    preallocateResourceForConstantZero(&con->RMcontext[0]);
#endif

#ifdef CVM_JIT_USE_FP_HARDWARE
    RMContextInit(con, &con->RMcontext[1], CVMCPU_FP_MIN_INTERESTING_REG,
	   CVMCPU_FP_MAX_INTERESTING_REG,
	   CVMCPU_FP_PHI_REG_SET,
	   CVMRM_FP_BUSY_SET, CVMRM_FP_SAFE_SET, CVMRM_FP_UNSAFE_SET,
	   CVMRM_FP_ANY_SET,
	   CVMCPU_FLDR32_OPCODE, CVMCPU_FLDR64_OPCODE,
	   CVMCPU_FSTR32_OPCODE, CVMCPU_FSTR64_OPCODE,
	   CVMCPU_FMOV_OPCODE,
	   CVMCPU_FP_SINGLE_REG_ALIGNMENT, CVMCPU_FP_DOUBLE_REG_ALIGNMENT,
	   CVMCPU_FP_MAX_REG_SIZE,
	   ( CVMJITRMConstantLoader32 ) CVMCPUemitLoadConstantFP,
           constantFPAddrLoader
       );
#endif
}


#ifdef CVM_JIT_GENERATE_GC_RENDEZVOUS
      /* SVMC_JIT d022609 (ML) 2004-06-30. TODO: our code does not
       * yet reflect Sun's new strategy for gc patching. see remark at
       * `jitgrammarrules.jcs:checkGC'.
       */
extern void
CVMJITcheckGC( CVMJITCompilationContext * con, CVMJITIRBlock* b );
#endif /* CVM_JIT_GENERATE_GC_RENDEZVOUS */


static void
RMbeginBlock(CVMJITRMContext* con, CVMJITIRBlock* b){
    con->resourceList = NULL;
    /* NOTE: RMbusySet need to need to be adjusted here for block
       reserved regs. */
    con->pinnedRegisters = con->busySet;
    con->occupiedRegisters = con->busySet;

    con->phiPinnedRegisters = 0;

    /* By default there is no restrictions on register allocation. */
    con->sandboxRegSet = CVMCPU_ALL_SET;
    memset(con->reg2res, 0,
	sizeof(CVMRMResource *) * (RM_MAX_INTERESTING_REG + 1));
    memset(con->local2res, 0,
	sizeof(CVMRMResource *) * con->numberLocalWords);
}

static void
RMapplyRegSandboxRestriction(CVMJITRMContext* con, CVMJITIRBlock* b)
{
    CVMassert(con->sandboxRegSet == CVMCPU_ALL_SET);
    if (b->sandboxRegSet != 0) {
        con->sandboxRegSet = b->sandboxRegSet;
    }
}

void
CVMRMbeginBlock(CVMJITCompilationContext* con, CVMJITIRBlock* b){

    resetSpillMap(con, b);
    RMbeginBlock(CVMRM_INT_REGS(con), b);
#ifdef CVM_JIT_USE_FP_HARDWARE
    RMbeginBlock(CVMRM_FP_REGS(con), b);
#endif

    CVMJITlocalrefsCopy(con, &con->localRefSet, &b->localRefs);

    CVMtraceJITCodegenExec({
	CVMconsolePrintf("\t\t\t@ Initial Temp REF set is " );
	CVMJITsetDumpElements(con, &(con->RMcommonContext.spillRefSet));
	CVMconsolePrintf("\n" );
    });

    /* bind phi values to the appropriate registers */
    CVMRMbindAllUsedNodes(con, b);

    if (CVMJITirblockIsBackwardBranchTarget(b) ||
        CVMJITirblockIsJSRReturnTarget(b)){
#ifdef CVM_JIT_GENERATE_GC_RENDEZVOUS
      /* SVMC_JIT rr 08Sept2003:
       * CVM_JIT_GENERATE_GC_RENDEZVOUS controls whether the runtime
       * compiler is to generate GC rendezvous points into compiled
       * methods. If green threads are used, there is no need for GC
       * rendezvous, because only the gc requesting thread is running.
       */
      /* SVMC_JIT d022609 (ML) 2004-06-30. TODO: our code does not
       * yet reflect Sun's new strategy for gc patching. see remark at
       * `jitgrammarrules.jcs:checkGC'.
       */
      /*
       * CVMJITcheckGC implements Explicit inlining of GC Checks, Trap Based GC
       * Checks and Patch Based GC Checks.
       */
       CVMJITcheckGC(con, b);
#endif /* CVM_JIT_GENERATE_GC_RENDEZVOUS */

    } else if (CVMJITirblockIsExcHandler(b)){
	/*
	 * Must always write a stackmap at the top of each
	 * and every exception handler.
	 */
        /* 
	   SVMC_JIT d022609 (ML) c5041614 (SW) 2004-05-05. There
	   exists Java code that has exception handler blocks that are
	   unreachable. See e.g.
	   `javasoft.sqe.tests.lang.stmt149.stmt14901.stmt14901'.  The
	   JIT compiler does not generate machine code, but previously
	   did generate a stack map for such blocks. We avoid this
	   now. Otherwise we would end up with multiple stack maps
	   associated to the same machine code instruction site.  This
	   in turn would trigger an assertion that had been introduced
	   into `CVMJITcaptureStackmap' recently.
         */
	b->logicalAddress = CVMJITcbufGetLogicalPC(con);
         if ((CVMJITIRBLOCK_NO_JAVA_MAPPING & b->flags) == 0) {
            CVMJITcaptureStackmap(con, 0);
         }
    } else {
	/* This is where all branches to this block will branch to */
	b->logicalAddress = CVMJITcbufGetLogicalPC(con);
	if (b == (CVMJITIRBlock*)CVMJITirlistGetHead(&(con->bkList))) {
	    /* Start of method. Need to preload locals explicitly. */
	    CVMtraceJITCodegen((
		"\tL%d:\t%d:\t@ entry point for first block\n",
                CVMJITirblockGetBlockID(b), CVMJITcbufGetLogicalPC(con)));
	    CVMRMpinAllIncomingLocals(con, b, CVM_TRUE);
	    CVMRMunpinAllIncomingLocals(con, b);
	} else {
	    CVMtraceJITCodegen(("\tL%d:\t%d:\t@ entry point for branches\n",
                CVMJITirblockGetBlockID(b), CVMJITcbufGetLogicalPC(con)));
#ifdef CVM_JIT_REGISTER_LOCALS
	    /* Locals already loaded. Bind them. */
	    CVMRMbindAllIncomingLocalNodes(con, b);
#endif
	}
    }


    RMapplyRegSandboxRestriction(CVMRM_INT_REGS(con), b);

#if 0
    /* DEBUGGING ONLY */

    CVMconsolePrintf("DIGESTED LOCALREFS NODE: ");

    CVMJITsetDumpElements(con, &refsNode->refLocals);
    CVMconsolePrintf("\n");
#endif
}

static void
recycleResources(CVMJITCompilationContext* con, CVMRMResource *list)
{
    CVMRMResource *rp, *next;
    for (rp = list; rp != NULL; rp = next) {
	next = rp->next;
	rp->next = con->RMcommonContext.freeResourceList;
	con->RMcommonContext.freeResourceList = rp;
    }
}

static void
RMendBlock(CVMJITRMContext* con){
#if 0
    /* DEBUGGING */
    for (rp = con->resourceList; rp != NULL; rp = nextp){
	nextp = rp->next;
	if (CVMRMgetRefCount(con, rp) != 0){
	    CVMconsolePrintf("Lingering resource at block end with ref count %d\n",
		CVMRMgetRefCount(con, rp));
	    if (CVMRMisConstant(rp)) {
		CVMconsolePrintf("	R%d, constant %d, spill %d\n",
				 rp->regno, rp->constant, rp->spillLoc);
	    } else {
		CVMconsolePrintf("	R%d, local %d, spill %d\n",
				 rp->regno, rp->localNo, rp->spillLoc);
	    }
	    
	    if (rp->expr != NULL){
                CVMJITirdumpIRNode(con->compilationContext, rp->expr,
				    0, "    ");
		CVMconsolePrintf("\n- - - - -\n");
	    }
	}
    }
#endif
    recycleResources(con->compilationContext, con->freeResourceList);
    recycleResources(con->compilationContext, con->resourceList);
    con->freeResourceList = NULL;
    con->resourceList = NULL;
    con->pinnedRegisters = con->busySet;
    con->occupiedRegisters = con->busySet;
}

void
CVMRMendBlock(CVMJITCompilationContext* con){
    CVMRMsynchronizeJavaLocals(con); /* should do nothing */
    RMendBlock(CVMRM_INT_REGS(con));
#ifdef CVM_JIT_USE_FP_HARDWARE
    RMendBlock(CVMRM_FP_REGS(con));
#endif
    if (con->RMcommonContext.maxSpillNumber+1 > con->maxTempWords){
	con->maxTempWords = con->RMcommonContext.maxSpillNumber+1;
    }
}

/*
 * Get register sandbox for the target block 'b'. The number of
 * registers in the sandbox is specified by 'numOfRegs'. The
 * register numbers are stored in the target block. The sandbox
 * will take effect when enter the target block, and only the
 * registers in the sandbox are allowed to be allocated before
 * sandbox is removed. When the sandbox is in effect, the registers
 * occupied by any 'USED' values can also be accessed. However
 * cautions need to be taken to prevent 'USED' registers being
 * trashed.
 */
CVMRMregSandboxResources*
CVMRMgetRegSandboxResources(CVMJITRMContext* con,
                            CVMJITIRBlock* b,
                            CVMRMregset target,
                            CVMRMregset avoid,
                            int numOfRegs)
{
    int i = 0;
    CVMRMregSandboxResources* sandboxRes;

    CVMassert(numOfRegs <
        CVMCPU_MAX_INTERESTING_REG - CVMCPU_MIN_INTERESTING_REG);

    if (numOfRegs <= 0) {
        return NULL;
    }

    sandboxRes = CVMJITmemNew(con->compilationContext,
                              JIT_ALLOC_CGEN_REGMAN,
                              sizeof(CVMRMregSandboxResources));
    for (i = 0; i < numOfRegs; i++) {
        CVMRMResource* rp = CVMRMgetResource(con,
                                             target, avoid, 1);
        CVMtraceJITCodegen((":::::Reserve reg(%d) for block ID: %d\n",
            CVMRMgetRegisterNumber(rp), CVMJITirblockGetBlockID(b)));
        b->sandboxRegSet |= rp->rmask;
        sandboxRes->num++;
        sandboxRes->res[i] = rp;
    }
    CVMassert(sandboxRes->num == numOfRegs);
    return sandboxRes;
}

/* Relinquish the resources associated with the sandbox registers. */
void
CVMRMrelinquishRegSandboxResources(CVMJITRMContext* con,
                                   CVMRMregSandboxResources *sandboxRes)
{
    if (sandboxRes != NULL) {
        int i = 0;
        for (i = 0; i < sandboxRes->num; i++) {
            CVMRMrelinquishResource(con, sandboxRes->res[i]);
        }
    }
}

/* Remove the register usage restriction applied to the block. Once
 * the sandbox is removed, the normal register usage rules resume. */
void
CVMRMremoveRegSandboxRestriction(CVMJITRMContext* con, CVMJITIRBlock* b)
{
    if (b->sandboxRegSet != 0) {
        CVMtraceJITCodegen((":::::Revoke register usage restriction\n"));
        con->sandboxRegSet = CVMCPU_ALL_SET;
        b->sandboxRegSet = 0;
    }
    CVMassert(con->sandboxRegSet == CVMCPU_ALL_SET);
}

/*
 * A minor spill is the register spill needed to call a light-weight
 * helper function, such as integer division or logical shift. These
 * cannot cause a GC or throw an exception, so only the registers which
 * are not saved by the C function-call protocol need to be spilled.
 * This is obviously a target-dependent set.
 *
 * The set of outgoing parameter WORDS is given to know not to
 * molest any expression we may have already pinned to those registers.
 */
/*
 * Note: 
 * The 'outgoing' set indicates the registers that are going to be used as
 * arguments for an upcoming function call.  Hence, we may not need to spill
 * them.
 * The 'ignore' set indicates the registers that we don't have to spill.
 * The 'safe' set indicates the registers which don't have to be evicted
 * (i.e. disassociated from its resource), but they may still need to be
 * spilled.  To force eviction of every register, use CVMRM_EMPTY_SET for the
 * the safe set.
 */
static void
spillRegsInRange(CVMJITRMContext* con,
    CVMRMregset outgoing, CVMRMregset ignore, CVMRMregset safe)
{
    int i;
    CVMRMregset or;
    CVMassert(!con->compilationContext->inConditionalCode);

    /* Can't ignore registers that are not preserved */
    ignore &= safe; /* Exclude the 'safe' set from the 'ignore' set. */

    /* The set of registers to spill are all the occupied registers excluding
       the the BUSY regs and the ignore list i.e.
                occupied registers - BUSY regs - ignore set. 
       The BUSY regs are the JSP, JFP, PC, and SP.  These are preserved by
       other mechanisms (not the the spill mechanism).  The spill mechanism
       should not mess with these registers.
    */
    or = (con->occupiedRegisters & ~con->busySet) & ~ignore;
    if (or == 0) {
        return; /* Nothing to spill. */
    }
    for (i=RM_MIN_INTERESTING_REG; i<=RM_MAX_INTERESTING_REG; i++){
	if ( or & (1U<<i)){
	    CVMRMResource* rp = con->reg2res[i];
            /* If it's an outgoing register, we may be able to save a spill: */
	    if (outgoing & (1U<<i)){
		/*
		 * This is complicated.
		 * If the refCount on this is 1, then we dont want to spill
		 * to memory, because this is the only use.
		 * But we always want the other semantics of spillResource,
		 * and don't want to bother reproducing them here.
		 * Thus mark the resource as dirty if the refCount is <= 1.
		 */
		if (CVMRMgetRefCount(con, rp) <= 1){
                    /* Mark as NOT dirty which means no need to spill: */
		    rp->flags &= ~CVMRMdirty;
		}
	    }
	    spillResource(con, rp, ((1U << i) & ~safe));
	    /* recompute, since multiple regs may have been spilled */
	    or = (con->occupiedRegisters & ~con->busySet) & ~ignore;
	    if (or == 0) {
		return;
	    }
	}
    }
}

/*
 *
 * TODO: For now, we assume that all outgoing registers are in the integer
 * set. if any of the outgoing registers are in the FP register set,
 * they should not be included in "outgoing" and they will always be
 * spilled. We should provide another API to support mixed int and fp
 * arguments.
 */
extern void
CVMRMminorSpill(CVMJITCompilationContext* con, CVMRMregset outgoing)
{

    spillRegsInRange(CVMRM_INT_REGS(con), outgoing,
		     CVMRM_SAFE_SET, CVMRM_SAFE_SET);
#ifdef CVM_JIT_USE_FP_HARDWARE
#if (CVMRM_FP_UNSAFE_SET != CVMRM_EMPTY_SET)
    /* if fp registers are all callee save, we need not do this */
    spillRegsInRange(CVMRM_FP_REGS(con), CVMRM_EMPTY_SET,
		     CVMRM_FP_SAFE_SET, CVMRM_FP_SAFE_SET);
#endif
#endif
}

/*
 * A major spill is the register spill needed to call a heavy-weight
 * helper function, such as allocation or invocation. These
 * CAN cause a GC or throw an exception, so all registers need to be
 * spilled to our save area in the Java stack frame.
 *
 * The "safe" argument specifies the set of registers that we don't
 * require to be spilled. However, if refs are in any of these registers
 * then they are still spilled. Normally "safe" is set to CVMRM_EMPTY_SET
 * for things like method invocations. However, when calling C functions
 * it can be set to CVMRM_SAFE_SET, because these will be
 * restored for us by the C function.
 *
 * The set of outgoing parameter WORDS is given to know not to
 * molest any expression we may have already pinned to those registers.
 *
 * See TODO for CVMRMminorSpill() regarding fp arguments.
 */
extern void
CVMRMmajorSpill(CVMJITCompilationContext* con, CVMRMregset outgoing,
		CVMRMregset safe)
{
    CVMRMregset nonRefs;
    CVMRMregset Refs = CVMRM_EMPTY_SET;
    CVMJITRMContext* rcp = CVMRM_INT_REGS(con);

    /* Compute Refs set */
    {
	CVMRMResource *rp;
	for (rp = rcp->resourceList; rp != NULL; rp = rp->next) {
	    if (rp->regno != -1 && CVMRMisRef(rp)) {
		Refs |= rp->rmask;
	    }
	}
    }

    CVMassert((Refs & rcp->occupiedRegisters) == Refs);

    /* Everything else is not a ref */
    nonRefs = rcp->occupiedRegisters & ~Refs;

    /* remove refs from safe set if GC can move objects. */
    safe &= ~Refs;

    spillRegsInRange(rcp, outgoing, nonRefs, safe);

#ifdef CVM_JIT_USE_FP_HARDWARE
#if (CVMRM_FP_UNSAFE_SET != CVMRM_EMPTY_SET)
    /* if fp registers are all callee save, we need not do this */
    spillRegsInRange(CVMRM_FP_REGS(con), CVMRM_EMPTY_SET,
		     CVMRM_FP_SAFE_SET, CVMRM_FP_SAFE_SET);
#endif
#endif
}

int
CVMRMspillPhisCount(CVMJITCompilationContext *con, CVMRMregset safe)
{
    CVMJITRMContext* rc = CVMRM_INT_REGS(con);
    int spilled = 0;

    if (con->currentCompilationBlock->phiCount > 0) {
	CVMRMResource *rp;
	for (rp = rc->resourceList; rp != NULL; rp = rp->next) {
	    if (rp->regno != -1) {
		if (CVMRMisRef(rp) || (rp->rmask & safe) != rp->rmask) {
		    ++spilled;
		}
	    }
	}
    }
    return spilled;
}

CVMBool
CVMRMspillPhis(CVMJITCompilationContext *con, CVMRMregset safe)
{
    CVMJITRMContext* rc = CVMRM_INT_REGS(con);
    CVMBool spilled = CVM_FALSE;

    {
	CVMRMResource *rp;
	for (rp = rc->resourceList; rp != NULL; rp = rp->next) {
	    if (rp->regno != -1) {
		if (CVMRMisRef(rp) || (rp->rmask & safe) != rp->rmask) {
		    CVMCPUemitFrameReference(con, RM_STORE_OPCODE(rc, rp),
					     rp->regno,
					     CVMCPU_FRAME_TEMP, rp->localNo, CVM_FALSE);
		    spilled = CVM_TRUE;
		}
		if (CVMRMisRef(rp)) {
		    CVMJITsetAdd(con, &con->RMcommonContext.spillRefSet,
			rp->localNo);
		}
	    }
	}
    }
    return spilled;
}

void
CVMRMreloadPhis(CVMJITCompilationContext *con, CVMRMregset safe)
{
    CVMJITRMContext* rc = CVMRM_INT_REGS(con);

    {
	CVMRMResource *rp;
	for (rp = rc->resourceList; rp != NULL; rp = rp->next) {
	    if (rp->regno != -1) {
		if (CVMRMisRef(rp) || (rp->rmask & safe) != rp->rmask) {
		    CVMCPUemitFrameReference(con, RM_LOAD_OPCODE(rc, rp),
					     rp->regno,
					     CVMCPU_FRAME_TEMP, rp->localNo, CVM_FALSE);
		}
		if (CVMRMisRef(rp)) {
		    CVMJITsetRemove(con, &con->RMcommonContext.spillRefSet,
			rp->localNo);
		}
	    }
	}
    }
}

/* 
 * Bind all of the USED nodes to resources. They will either be bound
 * to a register (if passed in one) or a spill location.
 */
static void
CVMRMbindAllUsedNodes(CVMJITCompilationContext* con, CVMJITIRBlock* b)
{
    int count;
    for (count = b->phiCount - 1; count >= 0; --count) {
	CVMJITIRNode* usedNode= b->phiArray[count];
	CVMRMbindUseToResource(con, usedNode);
    }
}

static int
CVMRMgetPhiReg(CVMJITCompilationContext *con,
	       CVMJITRMContext *rc,
	       int size)
{
    CVMUint32 mask;
    int nregs = RM_NREGISTERS(rc, size);
    int align = (nregs>1) ? rc->doubleRegAlignment : rc->singleRegAlignment;
    CVMRMregset avail = rc->phiFreeRegSet;
    int regNo;

    regNo = findMinRegInSet(avail,
			    rc->minInteresting, rc->maxInteresting,
			    align, nregs);

    if (regNo != -1) {
	mask = ((1U << nregs) - 1) << regNo;
	rc->phiFreeRegSet &= ~mask;
    }

    return regNo;
}
 
#ifdef CVM_JIT_REGISTER_LOCALS
static int
CVMRMgetLocalReg(CVMJITCompilationContext *con,
		 CVMJITRMContext *rc,
		 int size)
{
    int regNo = CVMRMgetPhiReg(con, rc, size);
#ifdef CVM_JIT_USE_FP_HARDWARE
    /*
     * Also use up registers in the "other" register set. This helps
     * to order locals properly when there is a mix of float and int
     * locals, thus reducing register movement. This isn't perfect
     * in all cases, but is better than not trying.
     */
    if (rc == CVMRM_FP_REGS(con)) {
	rc = CVMRM_INT_REGS(con);
    } else {
	rc = CVMRM_FP_REGS(con);
    }
    CVMRMgetPhiReg(con, rc, size);
#endif
    return regNo;
}
#endif

CVMJITRMContext *
CVMRMgetContextForTag(CVMJITCompilationContext *con, int tag)
{
#ifdef CVM_JIT_USE_FP_HARDWARE
    if (tag == CVM_TYPEID_FLOAT || tag == CVM_TYPEID_DOUBLE) {
	return CVMRM_FP_REGS(con);
    }
#endif
    return CVMRM_INT_REGS(con);
}

void
CVMRMallocatePhiRegisters(CVMJITCompilationContext *con, CVMJITIRBlock *b)
{
    int count;

    CVMassert(b->phiCount > 0);

    CVMRM_INT_REGS(con)->phiFreeRegSet = CVMRM_INT_REGS(con)->phiRegSet;
    CVMRM_FP_REGS(con)->phiFreeRegSet = CVMRM_FP_REGS(con)->phiRegSet;

    /* Assign register values from the bottom of the stack in
       in a repeating pattern.
     */

    for (count = 0; count < b->phiCount; ++count) {
	CVMJITIRNode *usedNode = CVMJITirnodeValueOf(b->phiArray[count]);
	int size = (CVMJITirnodeIsDoubleWordType(usedNode) ? 2 : 1);
	int tag = CVMJITgetTypeTag(usedNode);
	CVMJITRMContext *rc = CVMRMgetContextForTag(con, tag);
	int targetReg = -1;

	if (CVMJITirnodeGetUsedOp(usedNode)->registerSpillOk) {
	    targetReg = CVMRMgetPhiReg(con, rc, size);
	}
       	usedNode->decorationData.regHint = targetReg;
	usedNode->decorationType = CVMJIT_REGHINT_DECORATION;
    }
}

#ifdef CVM_JIT_REGISTER_LOCALS
/* Decide which register each incoming local will be in. */
void
CVMRMallocateIncomingLocalsRegisters(CVMJITCompilationContext *con,
				     CVMJITIRBlock *b)
{
    int count;

    CVMassert(b->incomingLocalsCount > 0);

    CVMRM_INT_REGS(con)->phiFreeRegSet = CVMRM_INT_REGS(con)->phiRegSet;
    CVMRM_FP_REGS(con)->phiFreeRegSet = CVMRM_FP_REGS(con)->phiRegSet;

    /* 
     * Assign registers for each incoming local
     */
    for (count = 0; count < b->incomingLocalsCount; ++count) {
	if (b->incomingLocals[count] != NULL) {
	    CVMJITIRNode *localNode =
		CVMJITirnodeValueOf(b->incomingLocals[count]);
	    int size = (CVMJITirnodeIsDoubleWordType(localNode) ? 2 : 1);
	    int tag = CVMJITgetTypeTag(localNode);
	    CVMJITRMContext *rc = CVMRMgetContextForTag(con, tag);
	    int targetReg = targetReg = CVMRMgetLocalReg(con, rc, size);
	    localNode->decorationData.regHint = targetReg;
	    localNode->decorationType = CVMJIT_REGHINT_DECORATION;
	} else {
	    /*
	     * If there is no node associated with the incoming local slot,
	     * than it is still necessary to call CVMRMgetLocalReg, so each
	     * local gets the proper register for its slot, ensuring that
	     * as we flow from block to block, we don't end up with
	     * unnecessary register movement.
	     */
	    CVMRMgetLocalReg(con, CVMRM_INT_REGS(con), 1);
	}
    }
}
#endif

static void
CVMRMbindUseToResource(CVMJITCompilationContext* con,
		       CVMJITIRNode* usedNode0)
{
    CVMJITIRNode* usedNode = CVMJITirnodeValueOf(usedNode0);
    CVMJITUsedOp* usedOp = CVMJITirnodeGetUsedOp(usedNode);
    CVMInt16 spillLocation = usedOp->spillLocation;
    CVMBool  isRef = CVMJITirnodeIsReferenceType(usedNode);
    int size = (CVMJITirnodeIsDoubleWordType(usedNode) ? 2 : 1);
    int tag = CVMJITgetTypeTag(usedNode);
    CVMJITRMContext* rc = CVMRMgetContextForTag(con, tag);
    CVMRMResource* rp;
    int targetReg = -1;

    if (usedOp->registerSpillOk) {
	targetReg = usedNode->decorationData.regHint;
    }

    /*
     * This value may have been been passed in a register or in the spill
     * area. Check which and do the right thing.
     */
    if (targetReg != -1){
	rp = CVMRMgetResourceSpecific(rc,
				      targetReg,
				      size);
	rp->flags |= CVMRMdirty; /* reg is not in sync with spill location */
	rp->localNo = spillLocation; /* remember spill location */
	CVMRMunpinResource(rc, rp);
    } else {
	rp = CVMRMallocateAndPutOnList(rc);
	rp->size = size;
	rp->nregs = RM_NREGISTERS(rc,size);
        rp->localNo = CVMRM_INVALID_LOCAL_NUMBER;
	rp->constant = -1;
	rp->regno = -1;
	rp->flags |= CVMRMphi;
	rp->spillLoc = spillLocation;
	if (isRef) {
	    CVMJITsetAdd(con, &(con->RMcommonContext.spillRefSet),
			 spillLocation);
	}
	/* else set starts empty anyway, so we don't need to remove */
    }
    rp->expr = usedNode;
    CVMRMsetRefCount(rc, rp, CVMJITirnodeGetRefCount(usedNode));
#ifdef CVM_DEBUG
    rp->originalTag = usedNode->tag;
#endif
    if (isRef) {
	rp->flags |= CVMRMref;
    }
    if (CVMJITirnodeIsAddressType(usedNode)) {
	rp->flags |= CVMRMaddr;
    }

    usedOp->resource = rp;
    return;
}

#ifdef CVM_JIT_REGISTER_LOCALS
/*
 * Decorate LOCAL or ASSIGN operand (exprNode) with register target info.
 * Uses targeting information from the target block that was setup by
 * CVMRMallocateIncomingLocalsRegisters().
 */
static void
setLocalExprTarget(CVMJITCompilationContext *con,
		   CVMJITIRBlock* curbk, int successorBlockIdx,
		   CVMJITIRNode* localNode, CVMJITIRNode* exprNode)
{
    CVMUint16 localNo =
	CVMJITirnodeGetLocal(CVMJITirnodeValueOf(localNode))->localNo;
    CVMJITIRNode* targetLocalNode;
    CVMInt32      targetLocalIdx;
    int i;

    if (successorBlockIdx == curbk->successorBlocksCount &&
	successorBlockIdx > 0) {
	/*
	 * We didn't find the right successor block. This can only happen
	 * if the ASSIGN or LOCAL is after a conditional branch to a block
	 * that we also fallthrough to, in which case just use the last
	 * successor block, which likely is also the fallthrough block.
	 */
	successorBlockIdx--;
    }

    exprNode = CVMJITirnodeValueOf(exprNode);
    if (exprNode->decorationType != CVMJIT_NO_DECORATION) {
	return; /* don't overwrite previous decoration */
    }

    /* look in each successor block */
    for (i = successorBlockIdx; i < curbk->successorBlocksCount; i++) {
	CVMJITIRBlock* targetbk = curbk->successorBlocks[i];
	/*
	 * Find the local in the target block. If found, then use its regHint
	 * as the regHint for the assign expression. regHint would have been
	 * setup in the block prepass by CVMRMallocateIncomingLocalsRegisters()
	 */
	targetLocalIdx = CVMJITirblockFindIncomingLocal(targetbk, localNo);
	if (targetLocalIdx == -1) {
	    continue; /* local does not flow into this block */
	}

	targetLocalNode = targetbk->incomingLocals[targetLocalIdx];
	targetLocalNode = CVMJITirnodeValueOf(targetLocalNode);
	if (targetLocalNode->decorationType != CVMJIT_REGHINT_DECORATION) {
	    continue; /* local does not flow in a reg into this block */
	}

	/* decorate the expression with info from target block*/
	exprNode->decorationData.regHint =
	    targetLocalNode->decorationData.regHint;
	exprNode->decorationType = CVMJIT_REGHINT_DECORATION;
	
	break;
    }
}

/*
 * Decorate ASSIGN operand with register target info appropriate for the
 * local being assigned to. Basically we want to try to the the operand
 * to evaulate into the register that the local needs to be in when
 * flowing to the next block.
 */
void
CVMRMsetAssignTarget(CVMJITCompilationContext* con, CVMJITIRNode* assignNode)
{
    CVMJITIRBlock* curbk = con->currentCompilationBlock;
    CVMInt16 assignIdx = assignNode->decorationData.assignIdx;
    CVMJITIRNode* localNode = CVMJITirnodeGetLeftSubtree(assignNode);
    CVMJITIRNode* exprNode = CVMJITirnodeGetRightSubtree(assignNode);
    int i;
    CVMassert(assignNode->decorationType == CVMJIT_ASSIGN_DECORATION);
    CVMassert(assignIdx < curbk->assignNodesCount);
    
    /* find the next block we might branch to */
    for (i = 0; i < curbk->successorBlocksCount; i++) {
	if (curbk->successorBlocksAssignOrder[i] > assignIdx) {
	    break;
	}
    }
    setLocalExprTarget(con, curbk, i, localNode, exprNode);
}

/*
 * Decorate LOCAL node with register target info.
 */
void
CVMRMsetLocalTarget(CVMJITCompilationContext* con, CVMJITIRNode* localNode)
{
    CVMJITIRBlock* curbk = con->currentCompilationBlock;
    /* find out next block we'll may branch to after loading the local */
    int successorBlocksIdx = localNode->decorationData.successorBlocksIdx;
    CVMassert(localNode->decorationType == CVMJIT_LOCAL_DECORATION);
    /* clear the current decoration so we can add a REGHINT one */
    localNode->decorationType = CVMJIT_NO_DECORATION;
    /* set REGHINT decoration on localNode */
    setLocalExprTarget(con, curbk, successorBlocksIdx, localNode, localNode);
}
#endif

/*
 * Decorate DEFINE operand with register target info.
 */
void
CVMRMsetDefineTarget(CVMJITCompilationContext *con, CVMJITIRNode *defineNode)
{
    CVMJITDefineOp* defineOp = CVMJITirnodeGetDefineOp(defineNode);
    CVMJITIRNode* usedNode = CVMJITirnodeValueOf(defineOp->usedNode);
    CVMJITUsedOp* usedOp = CVMJITirnodeGetUsedOp(usedNode);
#if 1
    CVMJITIRNode *operand = CVMJITirnodeValueOf(defineOp->operand);
#else
    /* old behavior */
    CVMJITIRNode *operand = defineOp->operand;
#endif

    if (usedOp->registerSpillOk) {
	int targetReg = usedNode->decorationData.regHint;
	if (targetReg != -1) {
	    if (operand->decorationType == CVMJIT_NO_DECORATION) {
		operand->decorationType = CVMJIT_REGHINT_DECORATION;
                operand->decorationData.regHint = targetReg;
	    }
	}
    }
}

/* SVMC_JIT d022609 (ML) 2004-05-17. extracted this workaround from
   `CVMRMstoreDefinedValue'. see comment there. */
static void freeDefineSpillLocation(CVMJITCompilationContext* con, 
				    CVMInt16 spillLocation)
{
      CVMJITRMContext* reg_con;
      CVMRMResource* liveUse;
      CVMtraceJITCodegen(("\t\t\t@ DEFINE of a USED with a different spillLocation.\n"));
      reg_con = CVMRM_INT_REGS(con);
      for(liveUse = reg_con->resourceList; liveUse != NULL; liveUse = liveUse->next) {
	 if (liveUse->spillLoc == spillLocation) {
	    CVMtraceJITCodegen(("\t\t\t@ Found live USE. Loading it into register before storing DEFINE\n"));
	    CVMRMpinResource(liveUse->rmContext, liveUse, liveUse->rmContext->anySet, CVMRM_EMPTY_SET);
	    liveUse->spillLoc = -1;
	    liveUse->flags &= ~CVMRMphi;
	    liveUse->flags |= CVMRMdirty;
	    if (liveUse->flags & CVMRMref) {
	       CVMJITsetRemove(con, &(con->RMcommonContext.spillRefSet),
			       spillLocation);
	    }
	    CVMRMunpinResource(liveUse->rmContext, liveUse);
	 }
      }
      reg_con = CVMRM_FP_REGS(con);
      for(liveUse = reg_con->resourceList; liveUse != NULL; liveUse = liveUse->next) {
	 if (liveUse->spillLoc == spillLocation) {
	    CVMtraceJITCodegen(("Found live USE. Loading it into register before storing DEFINE"));
	    CVMRMpinResource(liveUse->rmContext, liveUse, liveUse->rmContext->anySet, CVMRM_EMPTY_SET);
	    liveUse->spillLoc = -1;
	    liveUse->flags &= ~CVMRMphi;
	    liveUse->flags |= CVMRMdirty;
	    CVMRMunpinResource(liveUse->rmContext, liveUse);
	 }
      }
}

/*
 * The DEFINE node representing the computation of an out-flowing phi
 * value needs to be stored in some safe location (designated register or
 * stack location), where it can be found in the receiving block.
 *
 */
extern CVMBool
CVMRMstoreDefinedValue(
    CVMJITCompilationContext*	con,
    CVMJITIRNode*		defineNode,
    CVMRMResource*		src,
    int 			size)
{
    CVMJITDefineOp* defineOp = CVMJITirnodeGetDefineOp(defineNode);
    CVMJITIRNode* usedNode = CVMJITirnodeValueOf(defineOp->usedNode);
    CVMJITUsedOp* usedOp = CVMJITirnodeGetUsedOp(usedNode);
    CVMInt16 spillLocation = usedOp->spillLocation;
    CVMBool  useRegisterForPhi = CVM_FALSE;
    int      targetReg = -1;
    int tag = CVMJITgetTypeTag(defineNode);
    CVMJITRMContext* rc = CVMRMgetContextForTag(con, tag);

    if (usedOp->registerSpillOk) {
	targetReg = usedNode->decorationData.regHint;
	useRegisterForPhi = (targetReg != -1);
    }

    defineOp->resource = src;

#if 0
    /* SVMC_JIT rr 2003-10-13:
     * We can handle this special case now. See below for details.
     */

    /*
     * If we're defining a use of a different depth, then fail. We can't
     * handle this easily and don't think it ever happens in normal code.
     */
    if (!useRegisterForPhi && CVMRMisBoundUse(src)
	&& (src->spillLoc != spillLocation))
    {
#ifdef CVM_DEBUG
	CVMconsolePrintf("Cannot handle a DEFINE of a USED with"
			 " a different spillLocation.\n");
#endif
	CVMJITsetErrorMessage(con, "Cannot handle a DEFINE of a USED with"
			           " a different spillLocation.\n");
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)
	con->errNode = defineNode;
#endif
	return CVM_FALSE;
    }
#endif /* disable code */

    /*
     * Spill into a register if possible. Otherwise use a spill location.
     */
    CVMtraceJITCodegen(("\t\t\t@ Processing DEFINE(%d) ", spillLocation));
    if (useRegisterForPhi) {
	/* 
	 * The phi value has already been evaluated, but we will
	 * delay loading it into the phi register until we process the
	 * LOAD_PHIS node.  This will prevent it from getting spilled when
	 * subsequent expressions are evaluated. We also defer
	 * relinquishing the resource until RELEASE_PHIS.
	 */
	CVMtraceJITCodegenExec({
	    CVMconsolePrintf("register(%d): ",
			     targetReg);
	    if (src->regno == -1) {
		CVMconsolePrintf("not in register\n");
	    } else if (src->regno != targetReg) {
		CVMconsolePrintf("in wrong register\n");
	    } else {
		CVMconsolePrintf("in correct register\n");
	    }
	});
    } else {
	/*
	 * If we're defining a use of the same depth, then there's nothing
	 * to do. Otherwise spill it.
	 */
	CVMtraceJITCodegen(("spillLocation(%d): ", spillLocation));
	if (!CVMRMisBoundUse(src) || (src->spillLoc != spillLocation)) {
	   if (CVMRMisBoundUse(src) && (src->spillLoc != spillLocation)) {
	      /* SVMC_JIT rr 2003-10-13:
	       *  
	       * Here we have a DEFINE (d) of a USED (u1) with a different
	       * spill location due to a different depth of the DEFINE and
	       * USED in the expression stack (see CVMJITirblockPhiMerge).
	       *
	       * The problem here is, that there might be a live USED (u2) at
	       * d's spillLocation. The solution: check every resource, if
	       * it's spillLoc is equal to d's spillLocation. If it is, pin it
	       * to a register and set it's spillLoc to -1. The register
	       * manager will find an other spillLoc if it has to be spilled
	       * again and d can be stored to the stack.
	       */
	      freeDefineSpillLocation(con, spillLocation);
	    }
	    CVMtraceJITCodegen(("needs spilling\n"));
	    CVMRMpinResource(rc, src, rc->anySet, CVMRM_EMPTY_SET);
	    CVMassert(spillLocation >= 0);
            /* ensure that opcode selection doesn't go wrong */
            CVMassert( src->size == size );
            CVMCPUemitFrameReference(con, RM_STORE_OPCODE(rc, src),
				     CVMRMgetRegisterNumber(src),
                                     CVMCPU_FRAME_TEMP, spillLocation, CVM_FALSE);
	} else {
	    CVMtraceJITCodegen(("already spilled\n"));
	}
	CVMRMrelinquishResource(rc, src);

	/*
	 * But always make sure we've accounted for pointers.
	 * This may be redundant in the no-store case.
	 */
	if (CVMJITirnodeIsReferenceType(defineNode)) {
	    CVMJITsetAdd(con, &(con->RMcommonContext.spillRefSet), spillLocation);
	} else {
	    CVMJITsetRemove(con, &(con->RMcommonContext.spillRefSet), spillLocation);
	    if (size==2){
		CVMJITsetRemove(con, &(con->RMcommonContext.spillRefSet), spillLocation+1);
	    }
	}
    }
    return CVM_TRUE;
}


void
CVMRMloadOrReleasePhis(CVMJITCompilationContext* con,
		       CVMJITIRNode* node, CVMBool isLoad)
{
    CVMJITIRBlock *targetBlock = 
	CVMJITirnodeGetPhisListOp(node)->targetBlock;
    CVMJITIRNode** defineNodeList =
	CVMJITirnodeGetPhisListOp(node)->defineNodeList;
    int phiCount = targetBlock->phiCount;
    int i;

    /*
     * Load or release each phi that is suppose to be passed in a register.
     *
     * Loading is done after all expressions related to computing
     * the conditional branch have already been evaluated, so we don't keep
     * the registers pinned while evaluating the expressions.
     *
     * Releasing is done after the code for the branch has been generated,
     * so it is safe to unpin the phi registers at this point.
     */

    for (i = 0; i < phiCount; ++i) {
	CVMJITIRNode* defineNode = defineNodeList[i];
	CVMJITDefineOp* defineOp = &defineNode->type_node.defineOp;
	CVMJITIRNode* usedNode = CVMJITirnodeValueOf(defineOp->usedNode);
	CVMJITUsedOp* usedOp = CVMJITirnodeGetUsedOp(usedNode);
	CVMBool  useRegisterForPhi = CVM_FALSE;
	int phiRegister = -1;
	int tag = CVMJITgetTypeTag(defineNode);
	CVMJITRMContext* rc = CVMRMgetContextForTag(con, tag);

	CVMassert(defineOp->usedNode == targetBlock->phiArray[i]);

	if (usedOp->registerSpillOk) {
	    phiRegister = usedNode->decorationData.regHint;
	    useRegisterForPhi = (phiRegister != -1);
	}

	if (useRegisterForPhi) {
	    CVMRMResource* rp;
	    if (isLoad) {
		CVMtraceJITCodegen(("\t\t\t@ Loading DEFINE(%d) "
				    "register(%d): ",
				    usedOp->spillLocation, phiRegister));
	    }
	    rp = defineOp->resource;
	    CVMassert(rp != NULL);
	    CVMRMassertContextMatch(rc, rp);
	    CVMtraceJITCodegenExec({
		if (isLoad) {
		    if (rp->regno == -1) {
			CVMconsolePrintf("not in register\n");
		    } else if (rp->regno != phiRegister) {
			CVMconsolePrintf("in wrong register\n");
		    } else {
			CVMconsolePrintf("in correct register\n");
		    }
		}
	    });
	    if (isLoad) {
                rp = CVMRMpinResourceSpecific(rc, rp, phiRegister);
                rc->phiPinnedRegisters |= rp->rmask;
                /* The resource may have changed due to cloning.  Make sure to
                   save it away: */
                defineOp->resource = rp;
	    } else {
                rc->phiPinnedRegisters &= rp->rmask;
                CVMRMrelinquishResource(rc, rp);
	    }
	}
    }
}

#ifdef CVM_JIT_REGISTER_LOCALS

/*
 * Bind the specified local to the register that we know it is already
 * in. Note that the localNode0 argument is only used to get information
 * about the local. We don't actually bind the node to a resource, but
 * just populate the local2res[] with the resource we create for
 * the local.
 *
 * Also, we can't rely on the regHint stored in the localNode by
 * CVMRMallocateIncomingLocalsRegisters, because the same localNode
 * can appear in more than one blocks' incomingLocals[], and have a
 * different order in each, although this is rare.
 */
static void
CVMRMbindIncomingLocalNodeToResource(CVMJITCompilationContext* con,
				     CVMJITIRNode* localNode0)
{
    CVMJITIRNode* localNode = CVMJITirnodeValueOf(localNode0);
    CVMJITLocal* localOp = CVMJITirnodeGetLocal(localNode);
    CVMInt16 localNumber = localOp->localNo;
    CVMBool  isRef = CVMJITirnodeIsReferenceType(localNode);
    int size = (CVMJITirnodeIsDoubleWordType(localNode) ? 2 : 1);
    int tag = CVMJITgetTypeTag(localNode);
    CVMJITRMContext* rc = CVMRMgetContextForTag(con, tag);
    int targetReg = CVMRMgetLocalReg(con, rc, size);
    CVMRMResource* rp;
    int regNo;

    if (targetReg == -1) {
	return; /* no more registers for incoming locals */
    }

    CVMtraceJITCodegen(("\t\t\t@ Binding Incoming Local(%d) to reg(%d)\n",
			localNumber, targetReg));

    /* create a resource for this local and pin it */
    rp = CVMRMbindResourceForLocal(rc, size, isRef, localNumber);
    regNo = findAvailableRegister(rc, 1 << targetReg, ~(1 <<targetReg),
				  CVMRM_REGPREF_TARGET, CVM_TRUE, rp->nregs);
    CVMassert(regNo == targetReg);
    pinToRegister(rc, rp, regNo);

    /* There are no direct references to this resource, so release it. */
    CVMRMrelinquishResource(rc, rp);

#ifdef CVM_DEBUG
    rp->originalTag = localNode->tag;
#endif

    return;
}

/*
 * On block entry, create a resource for each local we know is already
 * in a register and store in the local2res[] array.
 *
 * Like with CVMRMbindLocalToResource, we can't rely on the regHint
 * stored by CVMRMallocateIncomingLocalsRegisters.
 */
static void
CVMRMbindAllIncomingLocalNodes(CVMJITCompilationContext* con, CVMJITIRBlock* b)
{
    int count;
    CVMRM_INT_REGS(con)->phiFreeRegSet = CVMRM_INT_REGS(con)->phiRegSet;
    CVMRM_FP_REGS(con)->phiFreeRegSet = CVMRM_FP_REGS(con)->phiRegSet;
    if (!CVMglobals.jit.registerLocals) {
	return;
    }
    for (count = 0; count < b->incomingLocalsCount; count++) {
	CVMJITIRNode* localNode = b->incomingLocals[count];
	if (localNode != NULL) {
	    CVMRMbindIncomingLocalNodeToResource(con, localNode);
	} else {
	    /*
	     * If there is no node associated with the incoming local slot,
	     * than it is still necessary to call CVMRMgetLocalReg, so each
	     * local gets the proper register for its slot, ensuring that
	     * as we flow from block to block, we don't end up with
	     * unnecessary register movement.
	     */
	    CVMRMgetLocalReg(con, CVMRM_INT_REGS(con), 1);
	}
    }
}

/*
 * Before branching to a block b, make sure the specified local is pinned
 * to the proper register so it can properly flow into the block. Note that
 * the localNode0 argument is only used to get information
 * about the local. We don't actually bind the node to a resource, but
 * just populate the local2res[] with the resource we create for
 * the local.
 */
static void
pinIncomingLocal(CVMJITCompilationContext* con, CVMJITIRBlock* b,
		 CVMJITIRNode* localNode0, CVMBool pin, CVMBool blockEntry)
{
    CVMJITIRNode* localNode = CVMJITirnodeValueOf(localNode0);
    CVMJITLocal* localOp = CVMJITirnodeGetLocal(localNode);
    CVMInt16 localNumber = localOp->localNo;
    CVMBool  isRef = CVMJITirnodeIsReferenceType(localNode);
    int size = (CVMJITirnodeIsDoubleWordType(localNode) ? 2 : 1);
    int tag = CVMJITgetTypeTag(localNode);
    CVMJITRMContext* rc = CVMRMgetContextForTag(con, tag);
    int targetReg = CVMRMgetLocalReg(con, rc, size);
    CVMRMResource* rp = rc->local2res[localNumber];

    if (targetReg == -1) {
	return; /* no more registers left for incoming locals */
    }

    if (!pin) {
	/* unpin the resource for the outgoing/incoming local */
	CVMRMunpinResource(rc, rc->pinnedOutgoingLocals[localNumber]);
	rc->pinnedOutgoingLocals[localNumber] = NULL;
	return;
    } else {
	/* pin the resource for the outgoing/incoming local */
	CVMtraceJITCodegenExec({
	    if (blockEntry) {
		CVMconsolePrintf(
		    "\t\t\t@ Preloading incoming local(%d) reg(%d)\n",
		    localNumber, targetReg);
	    } else {
		CVMconsolePrintf("\t\t\t@ Outgoing local(%d) reg(%d): ",
				 localNumber, targetReg);
		if (rp == NULL || rp->regno == -1) {
		    CVMconsolePrintf("not in register\n");
		} else if (rp->regno != targetReg) {
		    CVMconsolePrintf("in wrong register\n");
		} else {
		    CVMconsolePrintf("in correct register\n");
		}
	    }
	});
    
	if (rp == NULL) {
	    /* Local is not in a resource, so we need to load it. Force it
	     * to be loaded into targetReg. */
	    rp = CVMRMgetResourceForLocal(rc, 1 << targetReg, ~(1 <<targetReg),
					  size, isRef, CVM_TRUE, localNumber);
	    CVMRMdecRefCount(rc, rp);
	} else {
	    /* local is in a resource, make sure it is pinned properly */
	    rp = CVMRMpinResourceSpecific(rc, rp, targetReg);
	    if (rp != rc->local2res[localNumber]) {
		/* Resource was cloned. Undo damage. */
		CVMRMincRefCount(rc, rc->local2res[localNumber]);
		CVMRMdecRefCount(rc, rp);
	    }
	}

        CVMJITcsSetOutGoingRegisters(con, targetReg);
	/* remember the resource so we can unpin later */
	rc->pinnedOutgoingLocals[localNumber] = rp;
	CVMassert(rp->regno == targetReg);
    }
}

static void
pinAllIncomingLocals(CVMJITCompilationContext* con, CVMJITIRBlock* b,
		     CVMBool pin, CVMBool blockEntry)
{
    int count;
    CVMRM_INT_REGS(con)->phiFreeRegSet = CVMRM_INT_REGS(con)->phiRegSet;
    CVMRM_FP_REGS(con)->phiFreeRegSet = CVMRM_FP_REGS(con)->phiRegSet;
    if (!CVMglobals.jit.registerLocals) {
	return;
    }
    for (count = 0; count < b->incomingLocalsCount; count++) {
	CVMJITIRNode* localNode = b->incomingLocals[count];
	if (localNode != NULL) {
	    pinIncomingLocal(con, b, localNode, pin, blockEntry);
	} else {
	    /*
	     * If there is no node associated with the incoming local slot,
	     * then it is still necessary to call CVMRMgetLocalReg, so each
	     * local gets the proper register for its slot, ensuring that
	     * as we flow from block to block, we don't end up with
	     * unnecessary register movement.
	     */
	    CVMRMgetLocalReg(con, CVMRM_INT_REGS(con), 1);
	}
    }
}

/* Pin all locals flowing to block b. Usually called before emitting a
 * branch to block b.
 *
 * If blockEntry is TRUE, then this indicates that pinIncomingLocals is
 * being called to emit code to setup locals on entry to the block
 * rather than before branching to the block. This is to handle cases
 * where the block may be entered without first setting up incoming
 * locals, such as on fallthrough.
*/
void
CVMRMpinAllIncomingLocals(CVMJITCompilationContext* con, CVMJITIRBlock* b,
			  CVMBool blockEntry)
{
    pinAllIncomingLocals(con, b, CVM_TRUE, blockEntry);
}

/* unpin all locals pinned by CVMRMpinAllIncomingLocals */
void
CVMRMunpinAllIncomingLocals(CVMJITCompilationContext* con, CVMJITIRBlock* b)
{
    pinAllIncomingLocals(con, b, CVM_FALSE, CVM_FALSE /* unused */);
}

#endif /* CVM_JIT_REGISTER_LOCALS */

/* SVMC_JIT rr 2003-10-13 */
void CVMJITpreInitializeResource(CVMJITRMContext* con, CVMRMResource **rp, int size)
{
  *rp = CVMRMallocateAndPutOnList(con);
  (*rp)->size = size;
  (*rp)->nregs = RM_NREGISTERS(con, size);
  (*rp)->spillLoc = -1;
  (*rp)->localNo = CVMRM_INVALID_LOCAL_NUMBER;
  (*rp)->constant = -1;
  (*rp)->expr = NULL;
  (*rp)->regno = -1;
  (*rp)->rmContext = con;
}
