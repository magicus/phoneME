/*
 * @(#)jitstackman.c	1.27 03/11/14
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
 * Stack manager implementation.
 */
#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/utils.h"
#include "javavm/include/bcutils.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitir.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitcomments.h"
/* SVMC_JIT d022609 (ML) 2004-02-23. rename/move */
#include "javavm/include/jit/jitarchemitter.h"
#include "javavm/include/jit/jitregman.h"
#include "javavm/include/jit/jitstackman.h"

#include "javavm/include/clib.h"

/* x86 can store a constant */
#define CVMCPU_HAS_CONSTANT_STORE

/*
 * Compile-time management of the run-time Java expression and
 * parameter stack.
 * There will be target-specific things mixed in here, and they will be
 * obvious and easy to separate out.
 *
 */

/* SVMC_JIT */
/* define architecture dependent size of a stack slot */
#ifdef CVM_64
#define SIZE_OF_STACK_SLOT 8
#else
#define SIZE_OF_STACK_SLOT 4
#endif

void
CVMSMinit(CVMJITCompilationContext* con)
{
    con->SMdepth = 0;
    con->SMmaxDepth = 0;
#ifndef CVMCPU_HAS_POSTINCREMENT_STORE
    con->SMjspOffset = 0;
#endif
    CVMJITsetInit(con, &(con->SMstackRefSet));
}

void
CVMSMstartBlock(CVMJITCompilationContext* con)
{
    CVMassert(con->SMdepth == 0);
    CVMJITsetClear(con, &(con->SMstackRefSet));
}

#ifdef CVM_DEBUG
/*
 * Check for stack sanity
 */
void
CVMSMassertStackTop(CVMJITCompilationContext* con, CVMJITIRNode* expr){
    /* Make sure it used to be an invocation */
    CVMassert((CVMJITgetOpcode(expr) == (CVMJIT_INVOKE<<CVMJIT_SHIFT_OPCODE))
        || (CVMJITgetOpcode(expr)== (CVMJIT_INTRINSIC<<CVMJIT_SHIFT_OPCODE)));
    /* Make sure that the SMdepth we recorded after pushing it is the
     * same one we have now.
     */
    CVMassert(expr->type_node.binOp.data == con->SMdepth);
}
#endif

/*
 * push word(s) to the TOS from the given src register(s).
 * Adjusts TOS
 */
void
CVMSMpushSingle(CVMJITCompilationContext* con,
    CVMJITRMContext* rc,
    CVMRMResource* r)
{
    /* if this is a reference, note the fact */
    if (CVMRMisRef(r)){
	CVMJITsetAdd(con, &(con->SMstackRefSet), con->SMdepth);
    } else {
	CVMJITsetRemove(con, &(con->SMstackRefSet), con->SMdepth);
    }

    con->SMdepth += 1;
    if (con->SMdepth > con->SMmaxDepth)
	con->SMmaxDepth = con->SMdepth;

#ifdef CVMCPU_HAS_CONSTANT_STORE
/* SVMC_JIT d022609 (ML) 2004-02-23. 
   CISC architectures do have these. */
    if ( CVMRMisConstant( r ) )   
    {
#ifdef CVMCPU_HAS_POSTINCREMENT_STORE
    CVMCPUemitMemoryReferenceConst(con, CVMRMgetStoreOpcode(rc, r),
         r->constant, CVMCPU_JSP_REG,
        CVMCPUmemspecEncodePostIncrementImmediateToken(con, SIZE_OF_STACK_SLOT));
#else /* CVMCPU_HAS_POSTINCREMENT_STORE */
    CVMCPUemitMemoryReferenceConst(con, CVMRMgetStoreOpcode(rc, r),
        r->constant, CVMCPU_JSP_REG,
        CVMCPUmemspecEncodeImmediateToken(con, con->SMjspOffset*SIZE_OF_STACK_SLOT));
    con->SMjspOffset += 1;
#endif /* CVMCPU_HAS_POSTINCREMENT_STORE */
    }
    else
    {
#endif /* CVMCPU_HAS_CONSTANT_STORE */

    if (con->SMdepth > CVMRM_MAX_STACK_LOCATION) {
        CVMJITlimitExceeded(con, "exceeding max stack locations");
    }

      CVMRMpinResource(rc, r, CVMRM_GET_ANY_SET(rc), CVMRM_EMPTY_SET);
#ifdef CVMCPU_HAS_POSTINCREMENT_STORE
    CVMCPUemitMemoryReference(con, CVMRMgetStoreOpcode(rc, r),
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG,
        CVMCPUmemspecEncodePostIncrementImmediateToken(con, SIZE_OF_STACK_SLOT));
#else
    CVMCPUemitMemoryReference(con, CVMRMgetStoreOpcode(rc, r),
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG,
        CVMCPUmemspecEncodeImmediateToken(con, con->SMjspOffset*SIZE_OF_STACK_SLOT));
    con->SMjspOffset += 1;
#endif
#ifdef CVMCPU_HAS_CONSTANT_STORE
    }
#endif
}

void
CVMSMpushDouble(
    CVMJITCompilationContext* con,
    CVMJITRMContext* rc,
    CVMRMResource* r)
{
    CVMassert(!CVMRMisRef(r));
    CVMJITsetRemove(con, &(con->SMstackRefSet), con->SMdepth);
    CVMJITsetRemove(con, &(con->SMstackRefSet), con->SMdepth+1);

    con->SMdepth += 2;
    if (con->SMdepth > con->SMmaxDepth)
	con->SMmaxDepth = con->SMdepth;
    
    if (con->SMdepth > CVMRM_MAX_STACK_LOCATION) {
        CVMJITlimitExceeded(con, "exceeding max stack locations");
    }
    CVMRMpinResource(rc, r, CVMRM_GET_ANY_SET(rc), CVMRM_EMPTY_SET);
#ifdef CVM_64
    /* get opcode directly from regman context (due to trouble with double
       words and CVMRMgetStoreOpcode (size mismatch yields wrong opcode))
    */
#ifdef CVMCPU_HAS_POSTINCREMENT_STORE
    CVMCPUemitMemoryReference(con, rc->storeOpcode[ 1 ],
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG,
        CVMCPUmemspecEncodePostIncrementImmediateToken(con, 2 * SIZE_OF_STACK_SLOT ));
#else /* CVMCPU_HAS_POSTINCREMENT_STORE */
    CVMCPUemitMemoryReference(con, rc->storeOpcode[ 1 ],
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG,
        CVMCPUmemspecEncodeImmediateToken(con, con->SMjspOffset * SIZE_OF_STACK_SLOT ));
    con->SMjspOffset += 2;
#endif /* CVMCPU_HAS_POSTINCREMENT_STORE */
#else /* CVM_64 */
#ifdef CVMCPU_HAS_POSTINCREMENT_STORE
    CVMCPUemitMemoryReference(con, CVMRMgetStoreOpcode(rc, r),
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG,
        CVMCPUmemspecEncodePostIncrementImmediateToken(con, 2 * SIZE_OF_STACK_SLOT ));
#else /* CVMCPU_HAS_POSTINCREMENT_STORE */
    CVMCPUemitMemoryReference(con, CVMRMgetStoreOpcode(rc, r),
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG,
        CVMCPUmemspecEncodeImmediateToken(con, con->SMjspOffset * SIZE_OF_STACK_SLOT ));
    con->SMjspOffset += 2;
#endif /* CVMCPU_HAS_POSTINCREMENT_STORE */
#endif /* CVM_64 */

}

void
CVMSMpopParameters(CVMJITCompilationContext* con, CVMUint16 numberOfArgs)
{
    con->SMdepth -= numberOfArgs;
    CVMassert(con->SMdepth >= 0);
}

/*
 * Make JSP point just past the last argument. This is only needed when
 * postincrement stores are not supported. In this case we defer making
 * any stack adjustments until we are ready to make the method call.
 */
#ifndef CVMCPU_HAS_POSTINCREMENT_STORE
extern void
CVMSMadjustJSP(CVMJITCompilationContext* con)
{
    if (con->SMjspOffset != 0) {
	CVMJITaddCodegenComment((con, "adjust JSP before method call"));
	CVMCPUemitBinaryALUConstant(con, CVMCPU_ADD_OPCODE,
				    CVMCPU_JSP_REG, CVMCPU_JSP_REG,
				    con->SMjspOffset*SIZE_OF_STACK_SLOT, CVMJIT_NOSETCC);
	con->SMjspOffset = 0;
    }
}
#endif

/*
 * Record the effect on the stack of the method call. Returns a
 * CVMRMResource which is stack bound.
 */
CVMRMResource*
CVMSMinvocation(CVMJITCompilationContext* con, CVMJITIRNode* invokeNode)
{
    int typetag = CVMJITgetTypeTag(invokeNode);
    int resultWords = 1;
    CVMJITRMContext* regCon;
    switch (typetag){
    case CVM_TYPEID_VOID:
	return NULL;
    case CVM_TYPEID_OBJ:
	CVMJITsetAdd(con, &(con->SMstackRefSet), con->SMdepth);
	con->SMdepth += 1;
	break;
    case CVM_TYPEID_DOUBLE:
    case CVM_TYPEID_LONG:
	resultWords = 2;
	CVMJITsetRemove(con, &(con->SMstackRefSet), con->SMdepth);
	con->SMdepth += 1;
	/* FALLTHRU */
    default:
	/* includes int, float */
	CVMJITsetRemove(con, &(con->SMstackRefSet), con->SMdepth);
	con->SMdepth += 1;
	break;
    }
    
#ifdef CVM_JIT_USE_FP_HARDWARE
    switch (typetag){
    case CVM_TYPEID_FLOAT:
    case CVM_TYPEID_DOUBLE:
	regCon = CVMRM_FP_REGS(con);
	break;
    default:
	regCon = CVMRM_INT_REGS(con);
	break;
    }
#else /* CVM_JIT_USE_FP_HARDWARE */
    regCon = CVMRM_INT_REGS(con);
#endif /* CVM_JIT_USE_FP_HARDWARE */

#ifdef CVM_DEBUG
    CVMJITirnodeGetBinaryOp(invokeNode)->data = con->SMdepth;
#endif
    if (con->SMdepth > con->SMmaxDepth)
	con->SMmaxDepth = con->SMdepth;

    if (con->SMdepth > CVMRM_MAX_STACK_LOCATION) {
        CVMJITlimitExceeded(con, "exceeding max stack locations");
    }
    return CVMRMbindStackTempToResource(regCon, invokeNode, resultWords);
}

static void
physicalStackAdjust(CVMJITCompilationContext* con, int n)
{
    CVMCPUemitBinaryALUConstant(con, CVMCPU_SUB_OPCODE, CVMCPU_JSP_REG,
				CVMCPU_JSP_REG, n, CVMJIT_NOSETCC);
}

/*
 * pop "size" words from the TOS into the given dest register(s).
 * If r == NULL then just adjust JSP without the memory fetch.
 * Adjusts TOS.
 * NOTE: The resource r must be of type JavaStackTopValue.
 */
void
CVMSMpopSingle( CVMJITCompilationContext* con, CVMRMResource* r)
{
    int opcode;
    con->SMdepth -= 1;
    CVMassert(con->SMdepth >= 0);
    if (r == NULL){
	physicalStackAdjust(con, SIZE_OF_STACK_SLOT);
	return;
    }
#ifdef CVM_DEBUG_ASSERTS
    {
	CVMBool isRef;
	CVMJITsetContains(&con->SMstackRefSet, con->SMdepth, isRef);
	CVMassert(r == NULL || CVMRMisRef(r) == isRef);
    }
#endif
    opcode = CVMRMgetLoadOpcode(r->rmContext, r);
    /* Register already pinned for us, just issue the load */
    CVMassert(CVMRMisJavaStackTopValue(r));
    CVMCPUemitMemoryReference(con, opcode,
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG,
        CVMCPUmemspecEncodePreDecrementImmediateToken(con, SIZE_OF_STACK_SLOT));
}

void
CVMSMpopDouble( CVMJITCompilationContext* con, CVMRMResource* r)
{
    int opcode;
    con->SMdepth -= 2;
    CVMassert(con->SMdepth >= 0);
    if (r == NULL){
	physicalStackAdjust(con, 2 * SIZE_OF_STACK_SLOT);
	return;
    }
    /* Registers already pinned for us, just issue the loads */
    opcode = CVMRMgetLoadOpcode(r->rmContext, r);
    CVMassert(CVMRMisJavaStackTopValue(r));
    CVMCPUemitMemoryReference(con, opcode,
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG,
        CVMCPUmemspecEncodePreDecrementImmediateToken(con, 2 * SIZE_OF_STACK_SLOT));
}

/* Purpose: Fetches a word from the java stack into a register without
            adjusting the stack pointer. */

/*
 * Get "size" words from the stack locations corresponding to the specified
 * resources into the given dest register(s).
 * CVMSMgetSingle for all single-word data types, including ref's
 * CVMSMgetDouble for both double-word types.
 * NOTE: The resource r must be of type StackParam.
 */
void
CVMSMgetSingle(CVMJITCompilationContext *con, CVMRMResource *r)
{
    int offset;
    CVMassert(con->SMdepth >= r->stackLoc);
    CVMassert(CVMRMisStackParam(r));
    offset = ((r->stackLoc - con->SMdepth) * 4) - SIZE_OF_STACK_SLOT;
    /* Register already pinned for us, just issue the load: */
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_LDR32_OPCODE,
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG, offset);
}

void
CVMSMgetDouble(CVMJITCompilationContext *con, CVMRMResource *r)
{
    int offset;
    CVMassert(con->SMdepth >= r->stackLoc);
    CVMassert(CVMRMisStackParam(r));
    offset = ((r->stackLoc - con->SMdepth) * 4) - ( 2 * SIZE_OF_STACK_SLOT );
    /* Registers already pinned for us, just issue the loads: */
    CVMCPUemitMemoryReferenceImmediate(con, CVMCPU_LDR64_OPCODE,
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG, offset);
}

#undef SIZE_OF_STACK_SLOT
