/*
 * @(#)jitregman.h	1.7 06/10/23
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

#ifndef _INCLUDED_JITREGMAN_H
#define _INCLUDED_JITREGMAN_H

#include "javavm/include/jit/jitcisc.h"
#include "javavm/include/jit/jitirnode.h"

/*
 * The CVM Register Manager interface.
 */

#ifdef CVM_DEBUG_ASSERTS
enum {
    CVMJIT_EXPRESSION_ATTRIBUTE_TARGET_AVOID,
    CVMJIT_EXPRESSION_ATTRIBUTE_CALL_CONTEXT,
};
#endif

typedef struct CVMJITCompileExpression_attribute {
#ifdef CVM_DEBUG_ASSERTS
    CVMUint8 type;
#endif
    union {
        struct {
            CVMRMregset target;
            CVMRMregset avoid;
#ifdef  CVM_JIT_USE_FP_HARDWARE
            CVMRMregset float_target;
            CVMRMregset float_avoid;
#endif
        } rs;
        void *callContext;
    } u;
}CVMJITCompileExpression_attribute;

/*
 * The register bank contexts
 */
#undef	CVMRM_INT_REGS
#undef	CVMRM_FP_REGS
#define CVMRM_INT_REGS(_con) (&((_con)->RMcontext[0]))
#define CVMRM_FP_REGS(_con)  (&((_con)->RMcontext[1]))


/************************************************************************
 * Register Sets
 *
 * These are the register sets we need to setup:
 *   CVMRM_BUSY_SET:   all preassigned registers
 *   CVMRM_EMPTY_SET:  the empty set of registers
 *   CVMRM_ANY_SET:    all registers we are allowed to allocate
 *   CVMRM_SAFE_SET:   all non-volatile registers we are allowed to allocate
 *   CVMRM_UNSAFE_SET: all volatile registers we are allowed to allocate
 ************************************************************************/

/*
 * Some registers may or may not be allocated for a specific platform
 * based on whether or not there are spare registers for things like the
 * ee, CVMglobals, and chunkend pointers.
 */
#ifdef CVMCPU_CHUNKEND_REG
#define CVMRM_CHUNKEND_BUSY_BIT (1U<<CVMCPU_CHUNKEND_REG)
#else
#define CVMRM_CHUNKEND_BUSY_BIT 0
#endif
#ifdef CVMCPU_CVMGLOBALS_REG
#define CVMRM_CVMGLOBALS_BUSY_BIT (1U<<CVMCPU_CVMGLOBALS_REG)
#else
#define CVMRM_CVMGLOBALS_BUSY_BIT 0
#endif
#ifdef CVMCPU_EE_REG
#define CVMRM_EE_BUSY_BIT (1U<<CVMCPU_EE_REG)
#else
#define CVMRM_EE_BUSY_BIT 0
#endif
#ifdef CVMCPU_HAS_CP_REG
#define CVMRM_CP_BUSY_BIT (1U<<CVMCPU_CP_REG)
#else
#define CVMRM_CP_BUSY_BIT 0
#endif
#if defined(CVMJIT_TRAP_BASED_GC_CHECKS) && \
         !defined(CVMCPU_HAS_VOLATILE_GC_REG)
#define CVMRM_GC_BUSY_BIT (1U<<CVMCPU_GC_REG)
#else
#define CVMRM_GC_BUSY_BIT 0
#endif                                            

/*
 * Set CVMRM_BUSY_SET to the set of register we know are busy, plus
 * those that the processor has also specified are busy.
 */
#define CVMRM_BUSY_SET (						\
    CVMCPU_BUSY_SET |							\
    1U<<CVMCPU_SP_REG | 1U<<CVMCPU_JSP_REG | 1U<<CVMCPU_JFP_REG |	\
    CVMRM_CHUNKEND_BUSY_BIT |						\
    CVMRM_CVMGLOBALS_BUSY_BIT |						\
    CVMRM_EE_BUSY_BIT |							\
    CVMRM_CP_BUSY_BIT |							\
    CVMRM_GC_BUSY_BIT)

/* The empty set of registers. */
#define CVMRM_EMPTY_SET 0

/* All registers that regman is allowed to allocate */
#define CVMRM_ANY_SET    (CVMCPU_ALL_SET & ~CVMRM_BUSY_SET)

#define CVMRM_GET_ANY_SET(_rc) ((_rc)->anySet)

/*
 * All registers that regman is allowed to allocate that *will not* be
 * clobbered by C calls.
 */
#define CVMRM_SAFE_SET   (CVMCPU_NON_VOLATILE_SET & CVMRM_ANY_SET)

/*
 * All registers that regman is allowed to allocate that *will* be
 * clobbered by C calls.
 */
#define	CVMRM_UNSAFE_SET  (CVMCPU_VOLATILE_SET & CVMRM_ANY_SET)

/*
 * And the same for the FP registers if you have them.
 */
#ifdef  CVM_JIT_USE_FP_HARDWARE
#define CVMRM_FP_BUSY_SET   CVMCPU_FP_BUSY_SET
#define CVMRM_FP_ANY_SET    (CVMCPU_FP_ALL_SET & ~CVMCPU_FP_BUSY_SET)
#define CVMRM_FP_SAFE_SET   (CVMCPU_FP_NON_VOLATILE_SET & CVMRM_FP_ANY_SET)
#define	CVMRM_FP_UNSAFE_SET  (CVMCPU_FP_VOLATILE_SET & CVMRM_FP_ANY_SET)
#endif

/* Used to assert that no registers are pinned */
#ifdef CVMPCPU_USE_FP_HARDWARE
#define CVMRMhasNoPinnedRegisters(con) \
    ((((CVMRM_INT_REGS(con))->pinnedRegisters & \
    ~(CVMRM_INT_REGS(con))->phiPinnedRegisters)  \
    == (CVMRM_INT_REGS(con))->busySet) && \
    (((CVMRM_FP_REGS(con))->pinnedRegisters & \
    ~(CVMRM_FP_REGS(con))->phiPinnedRegisters) \
    == (CVMRM_FP_REGS(con))->busySet))
#else
#define CVMRMhasNoPinnedRegisters(con) \
    (((CVMRM_INT_REGS(con))->pinnedRegisters & \
    ~(CVMRM_INT_REGS(con))->phiPinnedRegisters) \
    == (CVMRM_INT_REGS(con))->busySet)
#endif

/* this is implementation */
struct CVMRMResource {
    /*
     * The data for the "super-class"
     */
    CVMJITIdentityDecoration  dec;

    CVMUint16   flags;
    CVMInt8     regno;
    CVMInt32	rmask;
    CVMInt16    spillLoc;
    CVMInt16    stackLoc; /* If isStackParam. */
    CVMUint16   localNo;  /* If local */

    CVMInt8     size;     /* size in words of data */
    CVMInt8     nregs;    /* number of registers occupied */

    /* SVMC_JIT HS 20031218: 64-bit constants on 64-bit platforms possible */
    CVMAddr     constant; /* If constant */

    CVMJITIRNode*     expr;
    struct CVMRMResource* prev;
    struct CVMRMResource* next;

#ifdef CVM_DEBUG
    CVMUint16   originalTag; /* for debug purposes only */
#endif
#ifdef CVM_TRACE_JIT
    const char* name; /* Name, if applicable */
#endif
    /* SVMC_JIT rr 2004-06-24 Substituted the key identifying the register
     * manager context by a pointer to the context, which we need when poping a
     * value from the stack (see CVMSMpopSingle)
     */
/*  CVMUint8 key; */
    CVMJITRMContext* rmContext;
};

/* limits */
#define CVMRM_MAX_SPILL_LOCATION        0x7fff
#define CVMRM_MAX_STACK_LOCATION        0x7fff
#define CVMRM_INVALID_LOCAL_NUMBER      ((CVMUint16)-1)

/* flags */
#define CVMRMpinned             (1<<0)
#define CVMRMdirty              (1<<1)
#define CVMRMoccupied           (1<<2)
#define CVMRMjavaStackTopValue  (1<<3)
#define CVMRMstackParam         (1<<4)
#define CVMRMphi                (1<<5)
#define CVMRMtrash              (1<<6)
#define CVMRMref                (1<<7)
#define CVMRMclone              (1<<8)

/* The types of resources */
#define CVMRMLocalVar   (1<<9)  /* Local variable */
#define CVMRMConstant32 (1<<10) /* Immediate or PC-relative 32-bit constant */

#define CVMRMConstantAddr (1<<11) /* Immediate or PC-relative 32/64-bit constant */
#define CVMRMaddr         (1<<12)

#define CVMRMisAddr(rp) \
    ((((rp)->flags) & CVMRMaddr) != 0)

#define CVMRMisConstant(rp)  \
    (((((rp)->flags) & CVMRMConstant32) != 0) || \
     ((((rp)->flags) & CVMRMConstantAddr) != 0))
#define CVMRMisConstant32(rp) \
    ((((rp)->flags) & CVMRMConstant32) != 0)
#define CVMRMisConstantAddr(rp) \
    ((((rp)->flags) & CVMRMConstantAddr) != 0)

#define CVMRMisLocal(rp) \
    ((((rp)->flags) & CVMRMLocalVar) != 0)

#define CVMRMisRef(rp) \
    ((((rp)->flags) & CVMRMref) != 0)

#define CVMRMgetRefCount(con, rp)  \
    CVMJITidentityGetDecorationRefCount(con , &((rp)->dec))  

#define CVMRMincRefCount(con, rp)  \
    CVMJITidentityIncrementDecorationRefCount(con , &((rp)->dec))

#define CVMRMdecRefCount(con, rp)  \
    CVMJITidentityDecrementDecorationRefCount(con , &((rp)->dec))

#define CVMRMsetRefCount(con, rp, cnt)  \
    CVMJITidentitySetDecorationRefCount(con , &((rp)->dec), cnt)

#define CVMRMisJavaStackTopValue(rp) \
    ((((rp)->flags) & CVMRMjavaStackTopValue) != 0)

#define CVMRMsetJavaStackTopValue(rp) \
    (((rp)->flags) |= CVMRMjavaStackTopValue)

#define CVMRMclearJavaStackTopValue(rp) \
    (((rp)->flags) &= ~CVMRMjavaStackTopValue)

#define CVMRMisStackParam(rp) \
    ((((rp)->flags) & CVMRMstackParam) != 0)

#define CVMRMsetStackParam(rp) \
    (((rp)->flags) |= CVMRMstackParam)

#define CVMRMclearStackParam(rp) \
    (((rp)->flags) &= ~CVMRMstackParam)

#define CVMRMgetSize(rp) \
    ((rp)->size)

#define CVMRMgetConstant(rp) \
    ((rp)->constant)

extern CVMRMResource *
CVMRMgetResource(
    CVMJITRMContext*, CVMRMregset target,
    CVMRMregset avoid, int size);

extern CVMRMResource*
CVMRMgetResourceStrict(
    CVMJITRMContext* con,
    CVMRMregset	target,
    CVMRMregset	avoid,
    int		size);

extern CVMRMResource *
CVMRMgetResourceSpecific(
    CVMJITRMContext*, int target, int size);

/*
 * Call this first to initialize state.
 */
extern void
CVMRMinit(CVMJITCompilationContext*);

/*
 * Clone a given resource to another resource in the other register bank.
 * The target is not pinned. In fact, it probably isn't even loaded into
 * a register. You just get a resource with a spill.
 */
extern CVMRMResource *
CVMRMcloneResource(
    CVMJITRMContext* srcCx, CVMRMResource* src, CVMJITRMContext* destCx,
    CVMRMregset target, CVMRMregset avoid);

extern CVMRMResource *
CVMRMbindStackTempToResource(
    CVMJITRMContext*, CVMJITIRNode* expr, int size );

/* Purpose: Convert a value at the top of the Java stack into a param that is
            stored on the stack but may not be at the top of the top of the
            stack anymore because other params may be pushed on top of it. */
extern void
CVMRMconvertJavaStackTopValue2StackParam(
    CVMJITCompilationContext*, CVMRMResource *rp);

/* Return lazily-pinned constant */
extern CVMRMResource *
CVMRMbindResourceForConstant32(CVMJITRMContext*, CVMInt32 constant);

/* Return pinned constant */
extern CVMRMResource *
CVMRMgetResourceForConstant32(
    CVMJITRMContext*, CVMRMregset target,
    CVMRMregset avoid, CVMInt32 constant);

extern CVMRMResource *
CVMRMbindResourceForConstantAddr(CVMJITRMContext*, CVMAddr constant);
extern CVMRMResource *
CVMRMgetResourceForConstantAddr(
    CVMJITRMContext*, CVMRMregset target,
    CVMRMregset avoid, CVMAddr constant);

/* Return lazily-pinned local */
extern CVMRMResource *
CVMRMbindResourceForLocal(
    CVMJITRMContext*, int size, CVMBool isRef, CVMInt32 localNumber);

/* Return pinned local */
extern CVMRMResource *
CVMRMgetResourceForLocal(
    CVMJITRMContext*, CVMRMregset target, CVMRMregset avoid, int size,
    CVMBool isRef, CVMBool strict, CVMInt32 localNumber);

extern void
CVMRMstoreJavaLocal(
    CVMJITRMContext*, CVMRMResource*, int size,
    CVMBool isRef, CVMInt32 localNumber );

/*
 * Like CVMRMstoreJavaLocal, but
 * does not do as much work.
 */
extern void
CVMRMstoreReturnValue(CVMJITRMContext*, CVMRMResource*);

/*
 * get the opcode appropriate for storing this resource to memory
 */
extern CVMInt32
CVMRMgetStoreOpcode(CVMJITRMContext*, CVMRMResource*);

/*
 * get the opcode appropriate for loading this resource from memory
 */
extern CVMInt32
CVMRMgetLoadOpcode(CVMJITRMContext*, CVMRMResource*);

extern void
CVMRMpinResource(
    CVMJITRMContext*, CVMRMResource*,
    CVMRMregset target, CVMRMregset avoid);

extern CVMRMResource *
CVMRMpinResourceSpecific(
    CVMJITRMContext*, CVMRMResource*,
    int target);

/* Pin resource, respecting the target and avoid sets strictly */
extern void
CVMRMpinResourceStrict(
    CVMJITRMContext* con,
    CVMRMResource* rp,
    CVMRMregset target,
    CVMRMregset avoid);

/*
 * Pin the resource, but only if it is believed that pinning eagerly
 * will be desirabable. Pinning eagerly is desirable if it appears
 * that the register will not get spilled before used, and will not
 * require the spilling of another register.
 */
extern void
CVMRMpinResourceEagerlyIfDesireable(
    CVMJITRMContext* rc, CVMRMResource* rp,
    CVMRMregset target, CVMRMregset avoid);

extern void
CVMRMunpinResource(CVMJITRMContext*, CVMRMResource* );

extern void
CVMRMoccupyAndUnpinResource(CVMJITRMContext*, CVMRMResource*,
	CVMJITIRNode*);

/*
 * Find does the lookup. YOU do the pinning yourself
 * at the appropriate time.
 * This is a semantic change.
 */
extern CVMRMResource*
CVMRMfindResource( CVMJITRMContext*, CVMJITIRNode* );

extern void
CVMRMrelinquishResource( CVMJITRMContext*, CVMRMResource*);

#define CVMRMgetRegisterNumber(rp) \
    (CVMassert((rp)->flags & CVMRMpinned), ((CVMX86Register) (rp)->regno))

#define CVMRMgetRegisterNumberUnpinned(rp) \
    ((rp)->regno)

extern void
CVMRMbeginBlock(CVMJITCompilationContext*, CVMJITIRBlock* b);

extern void
CVMRMendBlock(CVMJITCompilationContext*);

/*
 * In the current implementation, this is a no-op, since
 * java locals are always written back immediately by
 * CVMRMstoreJavaLocal. However, see the Spill routines below.
 */
extern void
CVMRMsynchronizeJavaLocals(CVMJITCompilationContext*);
/*
 * And this should always return 0, unless the above
 * assumptions are changed.
 */
extern int
CVMRMdirtyJavaLocalsCount(CVMJITCompilationContext *con);

/* 
 * SVMC_JIT d022609 (ML) 2004-01-07. ensure that a resource is up to
 * date in its home location.  for locals nothing happens.
 * temporaries might get pinned and spilled such that they will have a
 * spill location and will not be dirty. 
 */
extern void
CVMRMflushResource(CVMJITRMContext* con, CVMRMResource* res);

int
CVMRMspillPhisCount(CVMJITCompilationContext *con, CVMRMregset safe);

CVMBool
CVMRMspillPhis(CVMJITCompilationContext *con, CVMRMregset safe);

void
CVMRMreloadPhis(CVMJITCompilationContext *con, CVMRMregset safe);

/*
 * A minor spill is the register spill needed to call a light-weight
 * helper function, such as integer division or logical shift. These
 * cannot cause a GC or throw an exception, so only the registers which
 * are not saved by the C function-call protocol need to be spilled.
 * This is obviously a target-dependent set.
 *
 * The set of outgoing parameter WORDS is given to know not to
 * molest any expression we may have already pinned to those registers.
 *
 * FIXME(rt) when we can have outgoing FP registers, we need to rethink!
 */
extern int findSpillLoc(CVMJITCompilationContext* con,
			int                       size,
			CVMBool                   isRef);

/*
 * A major spill is the register spill needed to call a heavy-weight
 * helper function, such as allocation or invocation. These
 * CAN cause a GC or throw an exception, so all registers need to be
 * spilled to our save area in the Java stack frame.
 *
 * We do not distinguish yet between spilling to call the invoker
 * (which probably really needs all registers spilled) and calling
 * a heavy-weight helper. The latter really only need the REFERENCES
 * spilled, as other quantities will be saved by the C function call
 * protocol, and we don't care if they're lost due to an exception.
 * This is a distinction TBD.
 *
 * The set of outgoing parameter WORDS is given to know not to
 * molest any expression we may have already pinned to those registers.
 */
extern void
CVMRMmajorSpill(CVMJITCompilationContext*, CVMRMregset outgoing,
		CVMRMregset safe);

/*
 *
 * For now, we assume that all outgoing registers are in the integer set.
 * FIXME(rt)
 */
extern void
CVMRMminorSpill(CVMJITCompilationContext* con, CVMRMregset outgoing);



/*
 * Return true for resource that are USED values.
 */
#define CVMRMisBoundUse(rp) ((rp)->flags & CVMRMphi)

/*
 * The DEFINE node representing the computation of an out-flowing phi
 * value needs to be stored in some safe location (designated register or
 * stack location), where it can be found in the receiving block.
 * Returns CVM_FALSE if it cannot handle the store, in which case the
 * compilation should fail.
 */
extern CVMBool
CVMRMstoreDefinedValue(
    CVMJITCompilationContext*, CVMJITIRNode* expr, CVMRMResource*, int size);

/*
 * Either load all Phis into registers or release the registers they
 * are occupying, depending on the value of isLoad.
 */ 
void
CVMRMloadOrReleasePhis(CVMJITCompilationContext* con,
		       CVMJITIRNode* defineNode, CVMBool isLoad);

/*
 * Decide which register each incoming local will be in.
 */
void
CVMRMallocateIncomingLocalsRegisters(CVMJITCompilationContext *con,
				     CVMJITIRBlock *targetBlock);

/*
 * Decide which register each incoming phi will be in.
 */
void
CVMRMallocateIncomingPhisRegisters(CVMJITCompilationContext *con,
				   CVMJITIRBlock *targetBlock);

/*
 * Allocate registers for phis.
 */
void
CVMRMallocatePhiRegisters(CVMJITCompilationContext *con,
			  CVMJITIRBlock *targetBlock);

/*
 * Decorate DEFINE operand with register target info.
 */
void
CVMRMsetDefineTarget(CVMJITCompilationContext *con, CVMJITIRNode *defineNode);

/*
 * Decorate ASSIGN operand with register target info.
 */
#ifdef CVM_JIT_REGISTER_LOCALS
void
CVMRMsetAssignTarget(CVMJITCompilationContext* con, CVMJITIRNode* assignNode);
void
CVMRMsetLocalTarget(CVMJITCompilationContext* con, CVMJITIRNode* localNode);
#endif

#ifdef CVM_JIT_REGISTER_LOCALS
void
CVMRMpinAllIncomingLocals(CVMJITCompilationContext* con, CVMJITIRBlock* b,
                          CVMBool blockEntry);
void
CVMRMunpinAllIncomingLocals(CVMJITCompilationContext* con, CVMJITIRBlock* b);
#else
#define CVMRMpinAllIncomingLocals(con,b,x)
#define CVMRMunpinAllIncomingLocals(con,b)
#endif

/*
 * Return register context for phi based on tag.
 */
CVMJITRMContext *
CVMRMgetPhiContext(CVMJITCompilationContext *con, int tag);

/* SVMC_JIT rr 2003-10-13 */
void CVMJITpreInitializeResource( CVMJITRMContext* con, CVMRMResource **rp, int size);

/*******************************************************
 * Following are register sandbox related
 * *****************************************************/

typedef struct
{
    int num;
    CVMRMResource* res[CVMCPU_MAX_INTERESTING_REG -
                       CVMCPU_MIN_INTERESTING_REG];
}CVMRMregSandboxResources;


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
                            int numOfRegs);


/* Relinquish the resources associated with the sandbox registers. */
void
CVMRMrelinquishRegSandboxResources(CVMJITRMContext* con,
                                   CVMRMregSandboxResources *sandboxRes);

/* Remove the register sandbox restriction applied to the block. Once
 * the sandbox is removed, the normal register usage rules resume.
 */
void
CVMRMremoveRegSandboxRestriction(CVMJITRMContext* con, CVMJITIRBlock* b);

#endif /* _INCLUDED_JITREGMAN_H */
