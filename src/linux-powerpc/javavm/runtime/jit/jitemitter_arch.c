/*
 * @(#)jitemitter_arch.c	1.14 06/10/10
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
#include "javavm/include/jit/jitintrinsic.h"
#include "portlibs/jit/risc/include/porting/jitriscemitter.h"

#ifdef CVMJIT_INTRINSICS

/**************************************************************
 * CPU C Call convention abstraction - The following are prototypes of calling
 * convention support functions required by the RISC emitter porting layer.
 **************************************************************/

#define MAX_INT_ARG_REGS        8
#define MAX_FLOAT_ARG_REGS      8

#define CVMPPCCCALLargSize(argType) \
    (((argType == CVM_TYPEID_LONG) || (argType == CVM_TYPEID_DOUBLE)) ? 2 : 1)

#define CVMPPCCCALLargIsFloatOrDouble(argType) \
    ((argType == CVM_TYPEID_FLOAT) || (argType == CVM_TYPEID_DOUBLE))

/* Purpose: Performs initialization in preparation for pinning arguments to
            registers or to overflow to the native stack. */
void
CVMCPUCCALLinitArgs(CVMJITCompilationContext *con,
                    CVMCPUCallContext *callContext,
                    CVMJITIntrinsic *irec,
                    CVMBool forTargetting,
		    CVMBool useRegArgs)
{
    CVMUint16 properties = irec->config->properties;
    if ((properties & CVMJITINTRINSIC_ADD_CCEE_ARG) == 0) {
        callContext->currentIntRegIndex = 0;
    } else {
        /* Account for the ccee as the first argument: */
        callContext->currentIntRegIndex = 1;
    }
#ifdef CVM_JIT_USE_FP_HARDWARE_FOR_ARGS
    callContext->currentFloatRegIndex = 0;
#endif
}

static int
getIntArgRegNumber(CVMCPUCallContext *callContext, int argType)
{
    int regIndex = 0;
    switch (argType) {
    case CVM_TYPEID_BYTE:
    case CVM_TYPEID_BOOLEAN:
    case CVM_TYPEID_CHAR:
    case CVM_TYPEID_SHORT:
    case CVM_TYPEID_INT:
    case CVM_TYPEID_OBJ:
#ifndef CVM_JIT_USE_FP_HARDWARE_FOR_ARGS
    case CVM_TYPEID_FLOAT:
#endif
        regIndex = callContext->currentIntRegIndex++;
        break;

    case CVM_TYPEID_LONG:
#ifndef CVM_JIT_USE_FP_HARDWARE_FOR_ARGS
    case CVM_TYPEID_DOUBLE:
#endif
       regIndex = callContext->currentIntRegIndex;
       callContext->currentIntRegIndex += 2;
       /* 64 bit values need to be aligned on even addresses.  Adjust
          accordingly if not already aligned: */
       if ((regIndex & 0x1) != 0) {
           regIndex++;
           callContext->currentIntRegIndex++;
       }
       break;
    }
    CVMassert(callContext->currentIntRegIndex <= MAX_INT_ARG_REGS);
    return (regIndex + CVMCPU_ARG1_REG);
}

#ifdef CVM_JIT_USE_FP_HARDWARE_FOR_ARGS
static int
getFloatArgRegNumber(CVMCPUCallContext *callContext, int argType)
{
    int regIndex;
    CVMassert(CVMPPCCCALLargIsFloatOrDouble(argType));
    regIndex = callContext->currentFloatRegIndex++;
    CVMassert(callContext->currentFloatRegIndex <= MAX_FLOAT_ARG_REGS);
    return (regIndex + CVMCPU_FARG1_REG);
}
#endif

/* Purpose: Gets the register targets for the specified argument. */
CVMRMregset
CVMCPUCCALLgetArgTarget(CVMJITCompilationContext *con,
                        CVMCPUCallContext *callContext,
                        int argType, int argNo, int argWordIndex,
			CVMBool useRegArgs)
{
    int regno;
#ifdef CVM_JIT_USE_FP_HARDWARE_FOR_ARGS
    if (CVMPPCCCALLargIsFloatOrDouble(argType)) {
        regno = getFloatArgRegNumber(callContext, argType);
    } else
#endif
    {
        regno = getIntArgRegNumber(callContext, argType);
    }
    return (1U << regno);
}

/* Purpose: Pins an arguments to the appropriate register or store it into the
            appropriate stack location. */
CVMRMResource *
CVMCPUCCALLpinArg(CVMJITCompilationContext *con,
                  CVMCPUCallContext *callContext, CVMRMResource *arg,
                  int argType, int argNo, int argWordIndex,
                  CVMRMregset *outgoingRegs, CVMBool useRegArgs)
{
    int regno;
#ifdef CVM_JIT_USE_FP_HARDWARE_FOR_ARGS
    if (CVMPPCCCALLargIsFloatOrDouble(argType)) {
        regno = getFloatArgRegNumber(callContext, argType);
        /* Pin the floating point reg: */
        arg = CVMRMpinResourceSpecific(CVMRM_FP_REGS(con), arg, regno);

        /* Update the outgoingRegs for minor spills: */

#error "TODO: Not supported. Need rework for hardware floating point support"
    } else
#endif
    {
        regno = getIntArgRegNumber(callContext, argType);
        arg = CVMRMpinResourceSpecific(CVMRM_INT_REGS(con), arg, regno);
        *outgoingRegs |= arg->rmask;
    }
    return arg;
}

/* Purpose: Relinquish a previously pinned arguments. */
void
CVMCPUCCALLrelinquishArg(CVMJITCompilationContext *con,
                         CVMCPUCallContext *callContext, CVMRMResource *arg,
                         int argType, int argNo, int argWordIndex,
			 CVMBool useRegArgs)
{
#ifdef CVM_JIT_USE_FP_HARDWARE_FOR_ARGS
    if (CVMPPCCCALLargIsFloatOrDouble(argType)) {
        CVMRMrelinquishResource(CVMRM_FP_REGS(con), arg);
    } else 
#endif
    {
        CVMRMrelinquishResource(CVMRM_INT_REGS(con), arg);
    }
}

#endif /* CVMJIT_INTRINSICS */
