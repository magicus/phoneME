/*
 * @(#)jitstackman.c	1.32 06/10/10
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
 * Stack manager implementation.
 * Has many ARM-isms built in.
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
#include "portlibs/jit/risc/include/porting/jitriscemitter.h"
#include "portlibs/jit/risc/include/export/jitregman.h"
#include "portlibs/jit/risc/jitstackman.h"

#include "javavm/include/clib.h"

/*
 * Compile-time management of the run-time Java expression and
 * parameter stack.
 * There will be target-specific things mixed in here, and they will be
 * obvious and easy to separate out.
 *
 */

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

static void
CVMSMpush(
    CVMJITCompilationContext* con,
    CVMJITRMContext* rc,
    CVMRMResource* r,
    int size)
{
    con->SMdepth += size;
    if (con->SMdepth > con->SMmaxDepth) {
	con->SMmaxDepth = con->SMdepth;
    }

    if (con->SMdepth > CVMRM_MAX_STACK_LOCATION) {
        CVMJITlimitExceeded(con, "exceeding max stack locations");
    }

    CVMRMpinResource(rc, r, CVMRM_GET_ANY_SET(rc), CVMRM_EMPTY_SET);

    /* store resource to the stack */
#ifdef CVMCPU_HAS_POSTINCREMENT_STORE
    CVMCPUemitMemoryReference(con, CVMRMgetStoreOpcode(rc, r),
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG,
        CVMCPUmemspecEncodePostIncrementImmediateToken(con, 4 * size));
#else
    CVMCPUemitMemoryReference(con, CVMRMgetStoreOpcode(rc, r),
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG,
        CVMCPUmemspecEncodeImmediateToken(con, con->SMjspOffset * 4));
    con->SMjspOffset += size;
#endif

}

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

    CVMSMpush(con, rc, r, 1);
}

void
CVMSMpushDouble(
    CVMJITCompilationContext* con,
    CVMJITRMContext* rc,
    CVMRMResource* r)
{
    /* no double-word references */
    CVMassert(!CVMRMisRef(r));
    CVMJITsetRemove(con, &(con->SMstackRefSet), con->SMdepth);
    CVMJITsetRemove(con, &(con->SMstackRefSet), con->SMdepth+1);

    CVMSMpush(con, rc, r, 2);
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
				    con->SMjspOffset*4, CVMJIT_NOSETCC);
	con->SMjspOffset = 0;
    }
}
#endif

/*
 * Record the effect on the stack of the method call. Returns a
 * CVMRMResource which is stack bound.
 */
CVMRMResource*
CVMSMinvocation(CVMJITCompilationContext* con, CVMJITRMContext* rc,
		CVMJITIRNode* invokeNode)
{
    int typetag = CVMJITgetTypeTag(invokeNode);
    int resultWords = 1;
    switch (typetag){
    case CVM_TYPEID_VOID:
	return NULL;
    case CVM_TYPEID_OBJ:
	CVMassert(rc == CVMRM_INT_REGS(con));
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
#ifdef CVM_DEBUG
    CVMJITirnodeGetBinaryOp(invokeNode)->data = con->SMdepth;
#endif
    if (con->SMdepth > con->SMmaxDepth)
	con->SMmaxDepth = con->SMdepth;
    if (con->SMdepth > CVMRM_MAX_STACK_LOCATION) {
        CVMJITlimitExceeded(con, "exceeding max stack locations");
    }
    return CVMRMbindStackTempToResource(rc, invokeNode, resultWords);
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

static void
CVMSMpop(CVMJITCompilationContext* con, 
	 CVMJITRMContext* rc, CVMRMResource* r,
	 int size)
{
    con->SMdepth -= size;
    CVMassert(con->SMdepth >= 0);
    if (r == NULL){
	physicalStackAdjust(con, 4 * size);
	return;
    }
#ifdef CVM_DEBUG_ASSERTS
    {
	CVMBool isRef;
	CVMJITsetContains(&con->SMstackRefSet, con->SMdepth, isRef);
	CVMassert(r == NULL || CVMRMisRef(r) == isRef);
    }
#endif
    /* Register already pinned for us, just issue the load */
    CVMassert(CVMRMisJavaStackTopValue(r));
    CVMCPUemitMemoryReference(con, CVMRMgetLoadOpcode(rc, r),
        CVMRMgetRegisterNumber(r), CVMCPU_JSP_REG,
        CVMCPUmemspecEncodePreDecrementImmediateToken(con, 4 * size));
    /* NOTE: 4 is the size of a single word. */
}

void
CVMSMpopSingle(CVMJITCompilationContext* con, 
	       CVMJITRMContext* rc, CVMRMResource* r)
{
    CVMSMpop(con, rc, r, 1);
}

void
CVMSMpopDouble(CVMJITCompilationContext* con, 
	       CVMJITRMContext* rc, CVMRMResource* r)
{
    CVMSMpop(con, rc, r, 2);
}
