/*
 * @(#)jitcompile.c	1.148 06/10/13
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
 * Copyright 2005 Intel Corporation. All rights reserved.  
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/directmem.h"
#include "javavm/include/utils.h"
#include "javavm/include/bcutils.h"
#include "javavm/include/ccee.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitir.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitdebug.h"
#include "javavm/include/jit/jitutils.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitirblock.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include "javavm/include/jit/jitstackmap.h"
#include "javavm/include/jit/jitstats.h"
#include "javavm/include/jit/jitasmconstants.h"

#include "javavm/include/porting/jit/ccm.h"
#include "javavm/include/porting/jit/jit.h"

#include "javavm/include/clib.h"
#include "javavm/include/porting/ansi/setjmp.h"
#if defined(CVM_JIT_COLLECT_STATS) || \
    defined(CVM_JIT_ESTIMATE_COMPILATION_SPEED)
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/time.h"
#endif

#ifdef CVM_DEBUG_ASSERTS
#include "generated/offsets/java_lang_String.h"
#endif


#define CVMJIT_MIN_MAX_LOCAL_WORDS	300
#define CVMJIT_MIN_MAX_CAPACITY		500

/*
 * Mark the method as bad.
 */
static CVMBool
badMethod(CVMExecEnv *ee, CVMMethodBlock* mb)
{
    CVMBool success = CVM_TRUE;

    /* Mark method as bad */

    CVM_COMPILEFLAGS_LOCK(ee);
    if (CVMmbIsCompiled(mb)) {
	success = CVM_FALSE;
    } else {
	CVMmbCompileFlags(mb) |= CVMJIT_NOT_COMPILABLE;
    }
    CVM_COMPILEFLAGS_UNLOCK(ee);

    return success;
}

static CVMBool
isBadMethod(CVMExecEnv *ee, CVMMethodBlock *mb)
{
    CVMBool bad = CVM_FALSE;

    if (!CVMmbIsCompiled(mb)) {
        /* If already not compilable: */
	if ((CVMmbCompileFlags(mb) & CVMJIT_NOT_COMPILABLE) != 0) {
	    bad = CVM_TRUE;

        /* Reject the method if it is a <clinit>(): */
        } else if (CVMtypeidIsStaticInitializer(CVMmbNameAndTypeID(mb))) {
            /* Mark method as not compilable: */
            CVM_COMPILEFLAGS_LOCK(ee);
            CVMmbCompileFlags(mb) |= CVMJIT_NOT_COMPILABLE;
            CVM_COMPILEFLAGS_UNLOCK(ee);
            bad = CVM_TRUE;

#ifdef CVM_JIT_DEBUG
        } else if (!CVMJITdebugMethodIsToBeCompiled(ee, mb)) {
	    /* TODO: add option to allow if caller frame is compiled
	       (transitive closure) */
            /* Mark method as not compilable: */
            CVM_COMPILEFLAGS_LOCK(ee);
            CVMmbCompileFlags(mb) |= CVMJIT_NOT_COMPILABLE;
            CVM_COMPILEFLAGS_UNLOCK(ee);
            bad = CVM_TRUE;
#endif /* CVM_JIT_DEBUG */
        }
    }

    return bad;
}

#ifdef CVM_DEBUG_ASSERTS
/* Purpose: Assert some assumptions made in the implementation of the JIT. */
void CVMJITassertMiscJITAssumptions(void)
{
    /* The following are sanity checks for the JIT system in general that
       does not have anything to do with the current context of compilation.
       These assertions can be called from anywhere with the same result.
       NOTE: Each of these assertions corresponds to a constant or offset
       value defined in jitasmconstants.h.
    */

    /* Verifying CVMClassBlock offsets: */
    CVMassert(OFFSET_CVMClassBlock_classNameX ==
	      offsetof(CVMClassBlock, classNameX));
    CVMassert(OFFSET_CVMClassBlock_superClassCb ==
	      offsetof(CVMClassBlock, superclassX.superclassCb));

    CVMassert(OFFSET_CVMClassBlock_interfacesX ==
	      offsetof(CVMClassBlock, interfacesX));
    CVMassert(OFFSET_CVMClassBlock_arrayInfoX ==
	      offsetof(CVMClassBlock, cpX.arrayInfoX));
    CVMassert(OFFSET_CVMClassBlock_accessFlagsX ==
	      offsetof(CVMClassBlock, accessFlagsX));
    CVMassert(OFFSET_CVMClassBlock_instanceSizeX ==
	      offsetof(CVMClassBlock, instanceSizeX));
    CVMassert(OFFSET_CVMClassBlock_javaInstanceX ==
	      offsetof(CVMClassBlock, javaInstanceX));
    CVMassert(OFFSET_CVMClassBlock_methodTablePtrX ==
	      offsetof(CVMClassBlock, methodTablePtrX));
    
    /* Verifying CVMArrayInfo offsets and constants: */
    CVMassert(OFFSET_CVMArrayInfo_elementCb ==
	      offsetof(CVMArrayInfo, elementCb));

    /* Offsets and constants for CVMTypeID */
    CVMassert(CONSTANT_CVMtypeidArrayShift == CONSTANT_CVMtypeidArrayShift);
    CVMassert(CONSTANT_CVMtypeidArrayMask == CONSTANT_CVMtypeidArrayMask);
    CVMassert(CONSTANT_CVM_TYPEID_INT_ARRAY ==
	      CVMtypeidEncodeBasicPrimitiveArrayType(CVM_TYPEID_INT));
    CVMassert(CONSTANT_CVM_TYPEID_SHORT_ARRAY ==
	      CVMtypeidEncodeBasicPrimitiveArrayType(CVM_TYPEID_SHORT));
    CVMassert(CONSTANT_CVM_TYPEID_CHAR_ARRAY ==
	      CVMtypeidEncodeBasicPrimitiveArrayType(CVM_TYPEID_CHAR));
    CVMassert(CONSTANT_CVM_TYPEID_LONG_ARRAY ==
	      CVMtypeidEncodeBasicPrimitiveArrayType(CVM_TYPEID_LONG));
    CVMassert(CONSTANT_CVM_TYPEID_BYTE_ARRAY ==
	      CVMtypeidEncodeBasicPrimitiveArrayType(CVM_TYPEID_BYTE));
    CVMassert(CONSTANT_CVM_TYPEID_FLOAT_ARRAY ==
	      CVMtypeidEncodeBasicPrimitiveArrayType(CVM_TYPEID_FLOAT));
    CVMassert(CONSTANT_CVM_TYPEID_DOUBLE_ARRAY ==
	      CVMtypeidEncodeBasicPrimitiveArrayType(CVM_TYPEID_DOUBLE));
    CVMassert(CONSTANT_CVM_TYPEID_BOOLEAN_ARRAY ==
	      CVMtypeidEncodeBasicPrimitiveArrayType(CVM_TYPEID_BOOLEAN));

    /* Verifying CVMMethodBlock offsets and constants: */
#ifdef CVM_METHODBLOCK_HAS_CB
    CVMassert(OFFSET_CVMMethodBlock_cbX ==
	      offsetof(CVMMethodBlock, immutX.cbX));
#endif
    CVMassert(OFFSET_CVMMethodBlock_jitInvokerX ==
	      offsetof(CVMMethodBlock, jitInvokerX));
    CVMassert(OFFSET_CVMMethodBlock_methodTableIndexX ==
	      offsetof(CVMMethodBlock, immutX.methodTableIndexX));
    CVMassert(OFFSET_CVMMethodBlock_argsSizeX ==
	      offsetof(CVMMethodBlock, immutX.argsSizeX));
    CVMassert(OFFSET_CVMMethodBlock_methodIndexX ==
	      offsetof(CVMMethodBlock, immutX.methodIndexX));
    CVMassert(OFFSET_CVMMethodBlock_invokerAndAccessFlagsX ==
	      offsetof(CVMMethodBlock, immutX.invokerAndAccessFlagsX));
    CVMassert(OFFSET_CVMMethodBlock_codeX ==
	      offsetof(CVMMethodBlock, immutX.codeX));
    CVMassert(CONSTANT_CVMMethodBlock_size == sizeof(CVMMethodBlock));

    /* Verifying CVMMethodRange offsets and constants: */
    CVMassert(OFFSET_CVMMethodRange_mb ==
	      offsetof(CVMMethodRange, mb));

    /* Verifying CVMJavaMethodDescriptor offsets and constants: */
    CVMassert(OFFSET_CVMJmd_maxLocalsX ==
	      offsetof(CVMJavaMethodDescriptor, maxLocalsX));

    /* Verifying CVMCompiledFrame offsets and constants: */
    CVMassert(OFFSET_CVMCompiledFrame_PC == offsetof(CVMCompiledFrame, pcX));
    CVMassert(OFFSET_CVMCompiledFrame_receiverObjX ==
	      offsetof(CVMCompiledFrame, receiverObjX));
#ifdef CVMCPU_HAS_CP_REG
    CVMassert(OFFSET_CVMCompiledFrame_cpBaseRegX ==
	      offsetof(CVMCompiledFrame, cpBaseRegX));
#endif
    CVMassert(OFFSET_CVMCompiledFrame_opstackX ==
	      offsetof(CVMCompiledFrame, opstackX));

    /* Verifying CVMFrame offsets and constants: */
    CVMassert(OFFSET_CVMFrame_prevX == offsetof(CVMFrame, prevX));
    CVMassert(OFFSET_CVMFrame_type == offsetof(CVMFrame, type));
    CVMassert(OFFSET_CVMFrame_flags == offsetof(CVMFrame, flags));
    CVMassert(OFFSET_CVMFrame_topOfStack == offsetof(CVMFrame, topOfStack));
    CVMassert(OFFSET_CVMFrame_mb == offsetof(CVMFrame, mb));

    /* Verifying CVMExecEnv offsets and constants: */
    CVMassert(OFFSET_CVMExecEnv_tcstate_GCSAFE ==
	      offsetof(CVMExecEnv, tcstate[CVM_GC_SAFE]));
    CVMassert(OFFSET_CVMExecEnv_interpreterStack ==
	      offsetof(CVMExecEnv, interpreterStack));
    CVMassert(OFFSET_CVMExecEnv_miscICell ==
	      offsetof(CVMExecEnv, miscICell));
    CVMassert(OFFSET_CVMExecEnv_objLocksOwned ==
	      offsetof(CVMExecEnv, objLocksOwned));
    CVMassert(OFFSET_CVMExecEnv_objLocksFreeOwned ==
	      offsetof(CVMExecEnv, objLocksFreeOwned));
    CVMassert(OFFSET_CVMExecEnv_invokeMb ==
	      offsetof(CVMExecEnv, invokeMb));

    /* Verifying CVMInterfaceTable offsets and constants: */
    CVMassert((1 << CONSTANT_LOG2_CVMInterfaceTable_SIZE) ==
	      sizeof(CVMInterfaceTable));
    CVMassert((1 << CONSTANT_LOG2_CVMInterfaceTable_methodTableIndex_SIZE) ==
	      sizeof(*CVMcbInterfaceMethodTableIndices(
                        CVMsystemClass(java_lang_Class), 0)));

    /* Verifying CVMInterfaces offsets and constants: */
    CVMassert(OFFSET_CVMInterfaces_interfaceCountX == 
	      offsetof(CVMInterfaces, interfaceCountX));
    CVMassert(OFFSET_CVMInterfaces_itable ==
	      offsetof(CVMInterfaces, itable));
    CVMassert(OFFSET_CVMInterfaces_itable0_intfInfoX ==
	      offsetof(CVMInterfaces, itable[0].intfInfoX));

    /* Verifying CVMStack offsets and constants: */
    CVMassert(OFFSET_CVMStack_currentFrame ==
	      offsetof(CVMStack, currentFrame));
    CVMassert(OFFSET_CVMStack_stackChunkEnd ==
	      offsetof(CVMStack, stackChunkEnd));

    /* Verifying CVMCCExecEnv offsets and constants: */
    CVMassert(OFFSET_CVMCCExecEnv_ee ==
	      offsetof(CVMCCExecEnv, eeX));
    CVMassert(OFFSET_CVMCCExecEnv_stackChunkEnd ==
	      offsetof(CVMCCExecEnv, stackChunkEndX));
#ifndef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
    CVMassert(OFFSET_CVMCCExecEnv_ccmGCRendezvousGlue ==
	      offsetof(CVMCCExecEnv, ccmGCRendezvousGlue));
#endif
#if defined(CVMJIT_TRAP_BASED_GC_CHECKS) && defined(CVMCPU_HAS_VOLATILE_GC_REG)
    CVMassert(OFFSET_CVMCCExecEnv_gcTrapAddr ==
	      offsetof(CVMCCExecEnv, gcTrapAddr));
#endif
    CVMassert(OFFSET_CVMCCExecEnv_ccmStorage ==
	      offsetof(CVMCCExecEnv, ccmStorage));
    CVMassert(CONSTANT_CVMCCExecEnv_size ==
	      (size_t)(((CVMCCExecEnv *)0) + 1));

    /* Verifying CVMGlobalState offsets and constants: */
    CVMassert(OFFSET_CVMGlobalState_allocPtrPtr ==
	      offsetof(CVMGlobalState, allocPtrPtr));
    CVMassert(OFFSET_CVMGlobalState_allocTopPtr ==
	      offsetof(CVMGlobalState, allocTopPtr));
#ifdef CVM_ADV_ATOMIC_SWAP
    CVMassert(OFFSET_CVMGlobalState_fastHeapLock ==
	      offsetof(CVMGlobalState, fastHeapLock));
#endif
#ifdef CVM_TRACE_ENABLED
    CVMassert(OFFSET_CVMGlobalState_debugFlags ==
	      offsetof(CVMGlobalState, debugFlags));
#endif
#ifdef CVM_TRACE_JIT
    CVMassert(OFFSET_CVMGlobalState_debugJITFlags ==
	      offsetof(CVMGlobalState, debugJITFlags));
#endif
    CVMassert(OFFSET_CVMGlobalState_cstate_GCSAFE ==
	      offsetof(CVMGlobalState, cstate));

    /* Verifying CVMCState offsets and constants: */
    CVMassert(OFFSET_CVMCState_request == offsetof(CVMCState, request));

    /* Verifying CVMObjectHeader offsets and constants: */
    CVMassert(OFFSET_CVMObjectHeader_clas ==
	      offsetof(CVMObjectHeader, clas));
    CVMassert(OFFSET_CVMObjectHeader_various32 ==
	      offsetof(CVMObjectHeader, various32));

    /* Verifying CVMOwnedMonitor offsets and constants: */
    CVMassert(CONSTANT_CVM_OBJECT_NO_HASH == CVM_OBJECT_NO_HASH);
    CVMassert(CONSTANT_CVM_SYNC_BITS == CVM_SYNC_BITS);
    CVMassert(CONSTANT_CVM_HASH_BITS == CVM_HASH_BITS);
    CVMassert(CONSTANT_CVM_HASH_MASK == CVM_HASH_MASK);
    CVMassert(OFFSET_CVMObjMonitor_bits == offsetof(CVMObjMonitor, bits));

    CVMassert(OFFSET_CVMOwnedMonitor_owner ==
	      offsetof(CVMOwnedMonitor, owner));
    CVMassert(OFFSET_CVMOwnedMonitor_type ==
	      offsetof(CVMOwnedMonitor, type));
    CVMassert(OFFSET_CVMOwnedMonitor_object ==
	      offsetof(CVMOwnedMonitor, object));
    CVMassert(OFFSET_CVMOwnedMonitor_u_fast_bits ==
	      offsetof(CVMOwnedMonitor, u.fast.bits));
    CVMassert(OFFSET_CVMOwnedMonitor_next ==
	      offsetof(CVMOwnedMonitor, next));
#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
    CVMassert(OFFSET_CVMOwnedMonitor_count ==
	      offsetof(CVMOwnedMonitor, count));
#endif
#ifdef CVM_DEBUG
    CVMassert(OFFSET_CVMOwnedMonitor_magic ==
	      offsetof(CVMOwnedMonitor, magic));
    CVMassert(OFFSET_CVMOwnedMonitor_state ==
	      offsetof(CVMOwnedMonitor, state));
    CVMassert(CONSTANT_CVM_OWNEDMON_FREE == CVM_OWNEDMON_FREE);
    CVMassert(CONSTANT_CVM_OWNEDMON_OWNED == CVM_OWNEDMON_OWNED);
#endif

    CVMassert(CONSTANT_CVM_LOCKSTATE_UNLOCKED == CVM_LOCKSTATE_UNLOCKED);
    CVMassert(CONSTANT_CVM_LOCKSTATE_LOCKED == CVM_LOCKSTATE_LOCKED);
    CVMassert(CONSTANT_CVM_INVALID_REENTRY_COUNT == CVM_INVALID_REENTRY_COUNT);
    
    CVMassert(CONSTANT_CLASS_ACC_FINALIZABLE == CVM_CLASS_ACC_FINALIZABLE);
    CVMassert(CONSTANT_METHOD_ACC_STATIC == CVM_METHOD_ACC_STATIC);
    CVMassert(CONSTANT_METHOD_ACC_SYNCHRONIZED == CVM_METHOD_ACC_SYNCHRONIZED);
    CVMassert(CONSTANT_METHOD_ACC_NATIVE == CVM_METHOD_ACC_NATIVE);
    CVMassert(CONSTANT_METHOD_ACC_ABSTRACT == CVM_METHOD_ACC_ABSTRACT);

    CVMassert(CONSTANT_INVOKE_CNI_METHOD == CVM_INVOKE_CNI_METHOD);
    CVMassert(CONSTANT_INVOKE_JNI_METHOD == CVM_INVOKE_JNI_METHOD);

    CVMassert(CONSTANT_CNI_NEW_TRANSITION_FRAME == CNI_NEW_TRANSITION_FRAME);
    CVMassert(CONSTANT_CNI_NEW_MB == CNI_NEW_MB);

    CVMassert(CONSTANT_CMD_SIZE ==
	      (size_t)(((CVMCompiledMethodDescriptor *)0) + 1));

    CVMassert(CONSTANT_TRACE_METHOD == sun_misc_CVM_DEBUGFLAG_TRACE_METHOD);
    
    CVMassert(CONSTANT_CVM_FRAME_MASK_SPECIAL == CVM_FRAME_MASK_SPECIAL);
    CVMassert(CONSTANT_CVM_FRAME_MASK_SLOW == CVM_FRAME_MASK_SLOW);
    CVMassert(CONSTANT_CVM_FRAME_MASK_ALL == CVM_FRAME_MASK_ALL);

#ifdef CVM_DEBUG_ASSERTS
    CVMassert(CONSTANT_CVM_FRAMETYPE_NONE == CVM_FRAMETYPE_NONE);
#endif
    CVMassert(CONSTANT_CVM_FRAMETYPE_COMPILED == CVM_FRAMETYPE_COMPILED);

    /* Offsets and constants for the java.lang.String class: */
    CVMassert(OFFSET_java_lang_String_value ==
              CVMoffsetOfjava_lang_String_value * sizeof(CVMUint32));
    CVMassert(OFFSET_java_lang_String_offset ==
              CVMoffsetOfjava_lang_String_offset * sizeof(CVMUint32));
    CVMassert(OFFSET_java_lang_String_count ==
              CVMoffsetOfjava_lang_String_count * sizeof(CVMUint32));

    /* Offset and constants for Array classes: */
    CVMassert(OFFSET_ARRAY_LENGTH == offsetof(CVMArrayOfAnyType, length));
    CVMassert(OFFSET_ARRAY_ELEMENTS == offsetof(CVMArrayOfAnyType, elems));

    /* Offset and constants for GC: */
#if (CVM_GCCHOICE == CVM_GC_GENERATIONAL) && !defined(CVM_SEGMENTED_HEAP)
    CVMassert(CONSTANT_CARD_DIRTY_BYTE == CARD_DIRTY_BYTE);
    CVMassert(CONSTANT_CVM_GENGC_CARD_SHIFT == CVM_GENGC_CARD_SHIFT);
    CVMassert(CONSTANT_NUM_BYTES_PER_CARD == NUM_BYTES_PER_CARD);
#endif

    /* We only have 8 bits for the encoding of the CVMJITIROpcodeTag: */
    CVMassert(CVMJIT_TOTAL_IR_OPCODE_TAGS <= 256);
    
    /* We only have 4 bits for the encoding of the CVMJITIRNodeTag: */
    CVMassert(CVMJIT_TOTAL_IR_NODE_TAGS <= 16);

    /* The following nodes should be polymorphic with respect to
       CVMJITBinaryOp:
                CVMJITConditionalBranch
    */
    CVMassert(offsetof(CVMJITConditionalBranch, rhs) ==
	      offsetof(CVMJITBinaryOp, rhs));

    /* The following nodes should be polymorphic with respect to
       CVMJITGenericSubNode:
                CVMJITDefineOp
                CVMJITTableSwitch
                CVMJITLookupSwitch
                CVMJITConditionalBranch
                CVMJITBinaryOp
                CVMJITUnaryOp
    */
    CVMassert(offsetof(CVMJITDefineOp, operand) ==
	      offsetof(CVMJITGenericSubNode, firstOperand));
    CVMassert(offsetof(CVMJITTableSwitch, key) ==
	      offsetof(CVMJITGenericSubNode, firstOperand));
    CVMassert(offsetof(CVMJITLookupSwitch, key) ==
	      offsetof(CVMJITGenericSubNode, firstOperand));
    CVMassert(offsetof(CVMJITConditionalBranch, lhs) ==
	      offsetof(CVMJITGenericSubNode, firstOperand));
    CVMassert(offsetof(CVMJITBinaryOp, lhs) ==
	      offsetof(CVMJITGenericSubNode, firstOperand));
    CVMassert(offsetof(CVMJITUnaryOp, operand) ==
	      offsetof(CVMJITGenericSubNode, firstOperand));
}
#endif

static void
backOff(CVMMethodBlock* mb, CVMUint32 newInvokeCost)
{
    /* Try again later. */
    CVMUint32 oldInvokeCost = CVMmbInvokeCost(mb);
    if (newInvokeCost > CVMJIT_MAX_INVOKE_COST) {
	newInvokeCost = CVMJIT_MAX_INVOKE_COST;
    }
    if (newInvokeCost > oldInvokeCost) {
	CVMmbInvokeCostSet(mb, newInvokeCost);
    }
}

/*
 * Entry point to compiler
 */

CVMJITReturnValue
CVMJITcompileMethod(CVMExecEnv *ee, CVMMethodBlock* mb)
{
    CVMJITCompilationContext con;
    CVMJITReturnValue retVal;
    volatile CVMBool needDestroyContext = CVM_FALSE;
    volatile int extraCodeExpansion = 0;
    volatile int extraStackmapSpace = 0;
    volatile int maxAllowedInliningDepth = CVMglobals.jit.maxAllowedInliningDepth;

    /* Cache the ee's noOSRSkip & noOSRStackAdjust in case we recurse into the
       interpreter while compiling: */
    CVMInt32 noOSRSkip = ee->noOSRSkip;
    CVMInt8  noOSRStackAdjust = ee->noOSRStackAdjust;

#ifdef CVM_JIT_COLLECT_STATS
    CVMInt64 startTime;
    CVMInt64 endTime;
#endif

#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
    static CVMInt32 measuredCycle;
    static CVMInt64 measuredStartTime;
#endif

    CVMtraceJITAny(("JS: ATTEMPTING TO COMPILE %C.%M\n",
		    CVMmbClassBlock(mb), mb));

    CVMassert(CVMD_isgcSafe(ee));

    /*
     * Check if this thread has been prohibited from doing compilations
     * in order to avoid a deadlock with the jitLock.
     */
    if (ee->noCompilations) {
	return CVMJIT_CANNOT_COMPILE_NOW;
    }

    CVMJITstatsExec({ startTime = CVMtimeMillis(); });

    /* One compilation at a time. Lock others out. */
    CVMsysMutexLock(ee, &CVMglobals.jitLock);

    /*
     * Recursive entry into compiler should not happen!
     */
    CVMassert(!CVMglobals.jit.compiling);
    CVMglobals.jit.compiling = CVM_TRUE;

#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
    measuredCycle = CVMglobals.jit.doCSpeedMeasurement ?
        CVMJIT_NUMBER_OF_COMPILATION_PASS_MEASUREMENT_REPETITIONS : 1;
    measuredStartTime = CVMtimeMillis();
#endif

    CVMglobals.jit.compilationAttempts++;

#ifdef CVM_JIT_DEBUG
    CVMJITdebugInitCompilation(ee, mb);
#endif

#ifdef CVM_AOT
    /* AOT requires ROMization. If we are doing AOT compilation,
     * we better make sure the method is a ROMized method.
     */
    if (CVMglobals.jit.isPrecompiling) {
        if (!CVMcbIsInROM(CVMmbClassBlock(mb))) {
	    CVMconsolePrintf("CANNOT AOT NON-ROMIZED %C.%M\n",
                              CVMmbClassBlock(mb), mb);
            CVMglobals.jit.aotCompileFailed = CVM_TRUE;
            retVal = CVMJIT_SUCCESS;
            goto done;
        }
    }
#endif

#ifdef CVM_JVMTI
    if (CVMjvmtiIsInDebugMode() || CVMjvmtiMbIsObsolete(mb)) {
	retVal = CVMJIT_CANNOT_COMPILE;
	CVMJITsetErrorMessage(&con, "Debugger connected or obsolete method");
	goto done;
    }
#endif
    /* Check if mb is not compilable */
    if (isBadMethod(ee, mb)) {
	retVal = CVMJIT_CANNOT_COMPILE;
	CVMJITsetErrorMessage(&con, "Method already marked as not compilable");
	goto done;
    }

    /* Make sure the method isn't already compiled. */
    if (CVMmbIsCompiled(mb)) {
        CVMtraceJITStatus(("JS: ALREADY COMPILED %C.%M\n",
			   CVMmbClassBlock(mb), mb));
	retVal = CVMJIT_SUCCESS;
	goto done;
    }

    CVMtraceJITAny(("JS: COMPILING %C.%M\n", CVMmbClassBlock(mb), mb));

 retry:

    if (!CVMJITinitializeContext(&con, mb, ee, maxAllowedInliningDepth)) {
	CVMJITsetErrorMessage(&con,
			      "Could not initialize compilation context");
	retVal = CVMJIT_OUT_OF_MEMORY;
	goto done;
    }
    needDestroyContext = CVM_TRUE;

    if (setjmp(con.errorHandlerContext) == 0) {
	CVMJITinitializeCompilation(&con);
	
	CVMJITcompileBytecodeToIR(&con);

	/* Sanity check capacity */
	CVMassert(con.capacity >= CVMjmdCapacity(con.jmd));

	/* Sanity check code length */
	CVMassert(con.codeLength >= CVMjmdCodeLength(con.jmd));

	CVMJITcompileOptimizeIR(&con);
	con.extraCodeExpansion = extraCodeExpansion;
	con.extraStackmapSpace = extraStackmapSpace;
	CVMJITcompileGenerateCode(&con);
	CVMJITwriteStackmaps(&con);

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
	if (con.callees != NULL && (CVMUint32)con.callees[0] != 0) {
	    /* Make room for the callee table. It will be after the stackmaps,
	       which are located after the generated code. */
	    CVMUint32 numCallees = (CVMUint32)con.callees[0];
	    /* On platforms with variably sized instructions, such as x86,
	       need to pad the codebuffer before the start of callee table. */
	    CVMMethodBlock** callees = (CVMMethodBlock**)
		(((CVMAddr)CVMJITcbufGetPhysicalPC(&con) +
		  sizeof(CVMAddr) - 1) &
		 ~(sizeof(CVMAddr) - 1));

            CVMassert(CVMglobals.jit.pmiEnabled);

	    CVMJITcbufGetPhysicalPC(&con) =
		(CVMUint8*)&callees[numCallees + 1];
	    if (CVMJITcbufGetPhysicalPC(&con) >= con.codeBufEnd) {
		/* Fail to compile. We will retry with a larger buffer */
		CVMJITerror(&con, CODEBUFFER_TOO_SMALL,
			    "Estimated code buffer too small");
	    }
	    memcpy(callees, con.callees,
		   (numCallees + 1) * sizeof(CVMMethodBlock*));
	    con.callees = callees;
	} else {
	    con.callees = NULL;
	}
#endif

#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
        if (--measuredCycle != 0) {
            CVMJITerror(&con, RETRY, "Retry to measure compilation time");
        }
#endif

        /* By now, we know exactly the number of the maximum Temp words
         * (maxTempWords) of the compiled method. Before we go
         * any further, make sure it doesn't exceed 255 (0xFF) limit.
         */
        if (con.maxTempWords > 0xFF) {
            CVMJITerror(&con, CANNOT_COMPILE,
                        "Too many temp words (> 255)");
        }

	/* No more errors allowed after this point */

	{
	    CVMUint8 *startPC = con.codeEntry;
	    CVMCompiledMethodDescriptor *cmd =
		((CVMCompiledMethodDescriptor *)startPC) - 1;

	    CVMassert(con.mapPcNodeCount == 0 ||
		con.pcMapTable->numEntries == con.mapPcNodeCount);

	    CVMJITcbufSetCmd(con.codeBufAddr, cmd);
#ifdef CVM_DEBUG_ASSERTS
	    cmd->magic = CVMJIT_CMD_MAGIC;
#endif
	    CVMcmdSpillSize(cmd) = con.maxTempWords;
	    CVMcmdCapacity(cmd) = con.capacity + CVMcmdSpillSize(cmd);
	    CVMcmdMaxLocals(cmd) = con.numberLocalWords;

	    CVMassert(CVMcmdStartPC(cmd) == con.codeEntry);

	    {
		CVMUint8 *interpStartPC = startPC + con.intToCompOffset;
		cmd->startPCFromInterpretedOffsetX =
		            interpStartPC - (CVMUint8 *)cmd;
		CVMassert(CVMcmdStartPCFromInterpreted(cmd) ==
		    interpStartPC);
	    }

	    /* We allocate memory in the code cache for other data, not
	       just the compiled code.  These regions are:
	       PC map table, inlining info, gc patches, stackmaps,
	       and callee table. */
	       
	    /* Fill in the offsets for reaching our memory regions
	       from the cmd */

	    cmd->codeBufOffsetX =
		(CVMUint16 *)con.codeBufAddr - (CVMUint16 *)cmd;
	    CVMassert(CVMcmdCodeBufAddr(cmd) == con.codeBufAddr);

	    cmd->pcMapTableOffsetX =
		(CVMUint16 *)con.pcMapTable - (CVMUint16 *)cmd;
	    CVMassert(CVMcmdCompiledPcMapTable(cmd) == con.pcMapTable);

	    if (con.stackmaps != NULL) {
		cmd->stackMapsOffsetX =
		    (CVMUint16 *)con.stackmaps - (CVMUint16 *)cmd;
	    } else {
		cmd->stackMapsOffsetX = 0;
	    }
	    CVMassert(CVMcmdStackMaps(cmd) == con.stackmaps);

	    if (con.inliningInfo != NULL) {
		cmd->inliningInfoOffsetX =
		    (CVMUint16 *)con.inliningInfo - (CVMUint16 *)cmd;
	    } else {
		cmd->inliningInfoOffsetX = 0;
	    }
	    CVMassert(CVMcmdInliningInfo(cmd) == con.inliningInfo);

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
	    if (con.callees != NULL) {
                CVMassert(CVMglobals.jit.pmiEnabled);
	    	cmd->calleesOffsetX =
		    (CVMUint32*)con.callees - (CVMUint32 *)cmd;
	    } else {
	    	cmd->calleesOffsetX = 0;
	    }
		    
	    CVMassert(CVMcmdCallees(cmd) == con.callees);
#endif

#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
	    {
	        if (con.gcCheckPcs != NULL) {
		    cmd->gcCheckPCsOffsetX =
		        (CVMUint16 *)con.gcCheckPcs - (CVMUint16 *)cmd;
	        } else {
		    cmd->gcCheckPCsOffsetX = 0;
	        }
	        CVMassert(CVMcmdGCCheckPCs(cmd) == con.gcCheckPcs);
            }
#endif

#ifdef CVMCPU_HAS_CP_REG
	    {
		CVMUint8 *cp =
		    CVMJITcbufLogicalToPhysical(&con, con.target.cpLogicalPC);
		cmd->cpBaseRegOffsetX =
		    (CVMUint16 *)cp - (CVMUint16 *)cmd;
		CVMassert(CVMcmdCPBaseReg(cmd) == cp);
	    }
#endif

	    CVMcmdMb(cmd) = mb;

	    /* make sure we survive at least one gc */
	    CVMcmdEntryCount(cmd) = 0x2;

	    /*
	     * Make sure the cost is properly counted down.
	     * A simplifying assumption in a couple of other spots.
	     * Note how this is reset in the case of decompilation.
	     */
	    CVMmbInvokeCostSet(mb, 0);

	    /*
	     * We must add the entry to the code cache and set startPC
	     * atomically. The main thing we need to avoid is having
	     * aging happening after the method is added to the code cache,
	     * but before startPC is set. Since aging code will grab
	     * the jitLock first, we are safe, since we own jitLock
	     * for the duration of compilation
	     */
	    CVMassert(CVMsysMutexIAmOwner(con.ee, &CVMglobals.jitLock));
	    {
		/*
		 * The following will also make CVMmbIsCompiled() start
		 * to return TRUE. We must do this last.
		 */
		CVMassert (!CVMmbIsCompiled(mb));
		CVMmbStartPC(mb) = startPC;
		CVMmbJitInvoker(mb) = startPC;
		/*
		 * We need to commit the code buffer while in the lock to
		 * protect from races with decompilation.
		 */
		CVMJITcbufCommit(&con);

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
		/* If any compiled method calls this method, patch the call
		 * to be a direct call to this compiled method.
		 */
		CVMJITPMIpatchCallsToMethod(mb, CVMJITPMI_PATCHSTATE_COMPILED);
#endif
	    }

	    /* IAI-07: Notify JVMPI of compilation. */
#ifdef CVM_JVMPI
	    if (CVMjvmpiEventCompiledMethodLoadIsEnabled()) {
		CVMjvmpiPostCompiledMethodLoadEvent(ee, mb);
	    }
#endif
	    /* IAI-07: Notify JVMTI of compilation. */
#ifdef CVM_JVMTI
	    if (CVMjvmtiShouldPostCompiledMethodLoad()) {
		CVMjvmtiPostCompiledMethodLoadEvent(ee, mb);
	    }
#endif
	    CVMtraceJITStatus(("JS: COMPILED: size=%d startPC=0x%x %C.%M\n",
			       CVMcmdCodeBufSize(cmd), startPC,
			       CVMmbClassBlock(mb), mb));

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
	    CVMtraceJITPatchedInvokesExec({
	        CVMJITPMIdumpMethodCalleeInfo(con.mb, CVM_FALSE);
	    });
#endif

            CVMJITstatsExec({
                endTime = CVMtimeMillis();
                CVMJITstatsRecordSetCount(&con, CVMJIT_STATS_COMPILATION_TIME,
                    CVMlong2Int(CVMlongSub(endTime, startTime)));
            });

            /* Update the per compilation cycle and global statistics: */
#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
            if (CVMglobals.jit.doCSpeedMeasurement) {
                CVMglobals.jit.numberOfByteCodeBytesCompiled
                    += con.codeLength;
                CVMglobals.jit.numberOfByteCodeBytesCompiledWithoutInlinedMethods
                    += CVMjmdCodeLength(con.jmd);
                CVMglobals.jit.numberOfBytesOfGeneratedCode
                    += CVMcmdCodeBufSize(cmd);
            }
#endif
            CVMJITstatsUpdateStats(&con);
	    CVMtraceJITStatsExec({CVMJITstatsDump(&con);});
	}
    } else {
	if (con.codeBufAddr != NULL) {
	    CVMassert(CVMsysMutexIAmOwner(con.ee, &CVMglobals.jitLock));
	    CVMJITcbufFree(con.codeBufAddr, CVM_FALSE);
	}

	if (con.resultCode == CVMJIT_CANNOT_COMPILE) {
	    CVMglobals.jit.failedCompilationAttempts++;

	    /* Mark method as bad */
	    badMethod(ee, mb);
	}
    } 

    retVal = con.resultCode;

 done:

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
    /* We've probably already added some patchable method invokes to
       the database, so we need to remove them if compilation failed. */
    if (retVal != CVMJIT_SUCCESS && needDestroyContext && con.callees != NULL){
	/*
	 * For each method directly called by this method, have that method
	 * (the callee method) remove this method (the caller method)
	 * from its entries in the patch table.
	 */
	CVMUint32 numCallees = (CVMUint32)con.callees[0];
	CVMUint32 i;
        CVMassert(CVMglobals.jit.pmiEnabled);
	for (i = 1; i <= numCallees; i++) {
	    CVMMethodBlock* calleeMb = con.callees[i];
	    /* Remove records. */ 
	    CVMJITPMIremovePatchRecords(con.mb, calleeMb,
					con.codeBufAddr, con.codeBufEnd);
	}
    }
#endif

    switch (retVal) {
        case CVMJIT_SUCCESS:
	    break;
        case CVMJIT_CANNOT_COMPILE: {
	    /* Don't try again for as long as possible */
	    CVMmbInvokeCostSet(mb, CVMJIT_MAX_INVOKE_COST);
	    CVMtraceJITError(("JE: CANNOT COMPILE %C.%M\n",
			      CVMmbClassBlock(mb), mb));
	    CVMtraceJITError(("    Error(%d) Message: \"%s\"\n", 
			      retVal, con.errorMessage));
	    break;
	}
        case CVMJIT_CODEBUFFER_TOO_SMALL: {
	    /*
	     * If we ran out of code buffer space while compiling, then try
	     * again with a larger code buffer. 
	     */

	    CVMtraceJITError(("JE: Code buffer too small - retrying\n"));
	    CVMtraceJITError(("    Error(%d) Message: \"%s\"\n", 
			      retVal, con.errorMessage));
	    if (con.extraStackmapSpace > extraStackmapSpace) {
		extraStackmapSpace = con.extraStackmapSpace;
	    }
	    extraCodeExpansion += 2;
	    CVMJITdestroyContext(&con);
	    needDestroyContext = CVM_FALSE;
	    goto retry;
	    
	}
        case CVMJIT_INLINING_TOO_DEEP: {
	    /*
	     * If we failed to compile because inlining generated too
	     * much code, then retry with a smaller max inlining depth.
	     */
	    CVMassert(con.maxAllowedInliningDepth > 0);
	    /*
	     * We sometimes allow inlining trivial methods beyond
	     * maxAllowedInliningDepth, which results in con.maxInliningDepth
	     * being greater than maAllowedInliningDepth. In this case
	     * we just decrement maxAllowedInliningDepth. Otherwise we set
	     * maxAllowedInliningDepth to one less than the depth we were
	     * at when the failure happened.
	     */
	    if (maxAllowedInliningDepth < con.maxInliningDepth) {
		maxAllowedInliningDepth--;
	    } else {
		maxAllowedInliningDepth = con.maxInliningDepth - 1;
	    }
	    CVMtraceJITError(("JE: Inlining too deep (%d) - "
			      "retrying with %d\n",
			      con.maxAllowedInliningDepth,
			      maxAllowedInliningDepth));
	    CVMtraceJITError(("    Error(%d) Message: \"%s\"\n", 
			      retVal, con.errorMessage));
	    CVMJITdestroyContext(&con);
	    needDestroyContext = CVM_FALSE;
	    goto retry;
	}

#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
        case CVMJIT_RETRY: {
            /* A retry has been requested for whatever reason.  Go retry: */
            CVMJITdestroyContext(&con);
            needDestroyContext = CVM_FALSE;
            goto retry;
        }
#endif

	/* The following cases are for backing off from compilation */
        case CVMJIT_OUT_OF_MEMORY:
	    CVMtraceJITError(("JE: OUT OF MEMORY %C.%M\n",
			      CVMmbClassBlock(mb), mb));
	    CVMtraceJITError(("    Error(%d) Message: \"%s\"\n", 
			      retVal, con.errorMessage));
	    goto doBackOff;

        case CVMJIT_CANNOT_COMPILE_NOW:
	    CVMtraceJITError(("JE: CANNOT COMPILE NOW %C.%M\n",
			      CVMmbClassBlock(mb), mb));
	    CVMtraceJITError(("    Error(%d) Message: \"%s\"\n", 
			      retVal, con.errorMessage));
doBackOff:
	    /* We're out of memory, or we have a non-compilable
	       method. It's less likely for us to emerge from this state
	       so back off more agressively in this case. */
	    backOff(mb, CVMJIT_DEFAULT_CLIMIT * 2);
	    break;
        case CVMJIT_RETRY_LATER:
	    /* The method is not compilable at this time, but we would
             * like to retry the compilation some time later. So set
             * the invokeCost to be half of the compileThreshold.
             */
            CVMmbInvokeCostSet(mb, CVMglobals.jit.compileThreshold / 2);
	    CVMtraceJITError(("JE: RETRY LATER %s %C.%M\n",
			      con.errorMessage, CVMmbClassBlock(mb), mb));
	    CVMtraceJITError(("    Error(%d) Message: \"%s\"\n", 
			      retVal, con.errorMessage));
            break;

        case CVMJIT_DEFINE_USED_NODE_MISMATCH: {
	    /* If we failed to compile because there are define and used nodes
             * that have different spill locations, then we may be able to
             * succeed compiling with less inlining depth which reduces the
             * need for phi nodes.
             * However, we don't want to keep repeating this retry for each
             * inlining depth.  For now, we will implement the heuristic that
             * we'll at most retry with inline depth 1 if appropriate.  If
             * that fails, we'll go to inline depth 0.
	     * If we still fail with the same error when the inline depth is 0
             * then, we have to fail compilation.
	     */
	    maxAllowedInliningDepth = con.maxInliningDepth - 1;
	    if (maxAllowedInliningDepth > 1) {
                maxAllowedInliningDepth = 1;
            }
	    if (maxAllowedInliningDepth >= 0) {
                CVMtraceJITError(("JE: Defined Used node mismatch - "
		    "retrying with inline depth %d\n", maxAllowedInliningDepth));
		CVMtraceJITError(("    Error(%d) Message: \"%s\"\n", 
				  retVal, con.errorMessage));
	        CVMJITdestroyContext(&con);
	        needDestroyContext = CVM_FALSE;
	        goto retry;
	    }
            /* Else, compilation failed. Possibly this method will
	       be compilable in the future, so we just want to
	       backoff for a while. */
	    backOff(mb, CVMJIT_DEFAULT_CLIMIT * 2);
	    CVMtraceJITError(("JE: Defined Used node mismatch - retry later:"
                " %s %C.%M\n", con.errorMessage, CVMmbClassBlock(mb), mb));
	    CVMtraceJITError(("    Error(%d) Message: \"%s\"\n", 
			      retVal, con.errorMessage));
            break;
	}

        default:
	    CVMassert(CVM_FALSE);
    }

    /* If we are going to fail compilation, then print the trace message
       for it: */
    if (retVal != CVMJIT_SUCCESS) {
	CVMtraceJITStatus(("JS: COMPILATION FAILED: %C.%M\n", 
			   CVMmbClassBlock(mb), mb));
	CVMtraceJITStatus(("    Error(%d) Message: \"%s\"\n", 
			   retVal, con.errorMessage));
    }

    if (needDestroyContext) {
	CVMJITdestroyContext(&con);
    }

#ifdef CVM_JIT_ESTIMATE_COMPILATION_SPEED
    if (CVMglobals.jit.doCSpeedMeasurement) {
        if (retVal == CVMJIT_SUCCESS) {
            CVMInt64 measuredEndTime = CVMtimeMillis();
            CVMglobals.jit.numberOfMethodsCompiled++;
            CVMglobals.jit.totalCompilationTime +=
                CVMlong2Int(CVMlongSub(measuredEndTime, measuredStartTime));
        } else {
            CVMglobals.jit.numberOfMethodsNotCompiled++;
        }
    }
#endif

#ifdef CVM_JIT_DEBUG
    CVMJITdebugEndCompilation(ee, mb);
#endif
    CVMglobals.jit.compiling = CVM_FALSE;

    /* Restore the ee's noOSRSkip & noOSRStackAdjust now that we're done
       compiling: */
    ee->noOSRSkip = noOSRSkip;
    ee->noOSRStackAdjust = noOSRStackAdjust;

    /* Compilation or error handling done. Release lock */
    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
    
    return retVal;
}

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
/*
 * Removes any patch records for decompiledMb as a callerMb. Also
 * updates the patch state of instructions that call decompiledMb to
 * reflect the decompiled state.
 */
static void
CVMJITPMIhandleDecompilation(CVMMethodBlock* decompiledMb) 
{
    CVMMethodBlock** callees = CVMcmdCallees(CVMmbCmd(decompiledMb));
    CVMUint8* callerStartPC = CVMcmdStartPC(CVMmbCmd(decompiledMb));
    CVMUint8* callerEndPC = 
	callerStartPC + CVMcmdCodeSize(CVMmbCmd(decompiledMb));
    if (callees != NULL) {
	/*
	 * For each method directly called by this method, have that method
	 * (the callee method) remove this method (the caller method)
	 * from its entries in the patch table.
	 */
	CVMUint32 numCallees = (CVMUint32)callees[0];
	CVMUint32 i;
	for (i = 1; i <= numCallees; i++) {
	    CVMMethodBlock* calleeMb = callees[i];
	    /* Remove records. */ 
	    CVMJITPMIremovePatchRecords(decompiledMb, calleeMb,
					callerStartPC, callerEndPC);
	}
    }
    
    /* If the method being decompiled is called directly, patch the
       direct calls to instead defer to the interpreter. */
    CVMJITPMIpatchCallsToMethod(decompiledMb,
				CVMJITPMI_PATCHSTATE_DECOMPILED);
}
#endif

extern void
CVMJITdecompileMethod(CVMExecEnv* ee, CVMMethodBlock* mb)
{
    CVMCompiledMethodDescriptor* cmd = CVMmbCmd(mb);

    CVMtraceJITStatus(("JS: DECOMPILED: cbuf=0x%x size=%d mb=0x%x %C.%M\n",
		       CVMcmdCodeBufAddr(cmd), CVMcmdCodeBufSize(cmd), mb,
		       CVMmbClassBlock(mb), mb));

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
    /* If the ee is NULL then the vm is shutting down */
    if (ee != NULL) {
        CVMJITPMIhandleDecompilation(mb);
    }
#endif

    /* The ee is NULL during VM shutdown, in which case jitLock is gone. */
    if (ee != NULL) {
	CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.jitLock));
    }

    /* Let's let the interpreter invoke this method next time */
    CVMmbJitInvoker(mb) = (void*)CVMCCMletInterpreterDoInvoke;

    /*
     * If the method is ever decompiled, we don't want it to 
     * necessarily compile again the first time it is executed.
     * However, we should also give it some credit for having been
     * called enough times to be compiled once. Setting the invokeCost
     * to half of the threshold seems like a fair compromise.
     */
    CVMmbInvokeCostSet(mb, CVMglobals.jit.compileThreshold >> 1);
    
    CVMmbStartPC(mb) = NULL;
    CVMJITcbufFree(CVMcmdCodeBufAddr(cmd), CVM_TRUE);

    /* IAI-07: Notify JVMPI of decompilation. */
#ifdef CVM_JVMPI
    if (ee != NULL && CVMjvmpiEventCompiledMethodUnloadIsEnabled()) {
        CVMjvmpiPostCompiledMethodUnloadEvent(ee, mb);
    }
#endif
    /* IAI-07: Notify JVMTI of decompilation. */
#ifdef CVM_JVMTI
    if (ee != NULL && CVMjvmtiShouldPostCompiledMethodUnload()) {
        CVMjvmtiPostCompiledMethodUnloadEvent(ee, mb);
    }
#endif
}

/*
 * Mark the method so it will never be compiled.
 */
CVMBool
CVMJITneverCompileMethod(CVMExecEnv *ee, CVMMethodBlock* mb)
{
    return badMethod(ee, mb);
}

/*
 * CVM_FALSE means could not initialize due to out of memory
 * CVM_TRUE means success.
 *
 * No CVMJITmemNew() is allowed here, as the jmpbuf for the longjmp
 * has not been setup yet.
 */
CVMBool
CVMJITinitializeContext(CVMJITCompilationContext* con, CVMMethodBlock* mb,
			CVMExecEnv* ee, CVMInt32 maxAllowedInliningDepth)
{
    memset((void*)con, 0, sizeof(CVMJITCompilationContext));
    con->ee = ee;

    /* Initialize stats counters: */
    if (!CVMJITstatsInitCompilationContext(con)) {
        return CVM_FALSE;
    }

    /*
     * Special case for when we generate code into the start of the code cache.
     */
    if (mb == NULL) {
	return CVM_TRUE;
    }

    con->mb = mb;
    con->jmd = CVMmbJmd(mb);

    con->maxLocalWords = 3 * CVMjmdMaxLocals(con->jmd) / 2 + 3;
    if (con->maxLocalWords < CVMJIT_MIN_MAX_LOCAL_WORDS) {
	con->maxLocalWords = CVMJIT_MIN_MAX_LOCAL_WORDS;
    }

    /* At most double frame usage */
    con->maxCapacity = CVMjmdCapacity(con->jmd) * 2;
    if (con->maxCapacity < CVMJIT_MIN_MAX_CAPACITY) {
	con->maxCapacity = CVMJIT_MIN_MAX_CAPACITY;
    }

    con->maxAllowedInliningDepth = maxAllowedInliningDepth;

    con->codeLength = CVMjmdCodeLength(con->jmd);
    con->resultCode = CVMJIT_SUCCESS;
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)
    con->nodeID = 0;
    con->errNode = NULL;
    con->errorMessage = NULL;
#endif
#if defined(CVM_TRACE_JIT)
    con->comments = NULL;
    con->lastComment = NULL;
#endif /* CVM_TRACE_JIT */

#ifdef CVM_JIT_DEBUG
    CVMJITdebugInitCompilationContext(con, ee);
#endif

    /* Empty context stack */
    con->maxInliningDepth = -1;
    
    return CVM_TRUE;
}

/*
 * Init data structures required for compilation
 */
void
CVMJITinitializeCompilation(CVMJITCompilationContext* con)
{
    /*
     * Initialize the memory allocator.
     */
    CVMJITmemInitialize(con);

    /*
     * Special case for when we generate code into the start of the code cache.
     */
    if (con->mb == NULL) {
	return;
    }

    /* create blockList */
    CVMJITirlistInit(con, &con->bkList);

    /* operandStack */ 
    /* We really need con->maxCapacity - sizeof(locals) - sizeof(Frame).
       But it can't hurt to overshoot here. The limits are checked during
       inlining anyway. */
    con->operandStack = CVMJITmemNew(con, JIT_ALLOC_COLLECTIONS,
				     sizeof(CVMJITStack));
    CVMJITstackInit(con, con->operandStack, con->maxCapacity);

    /* Initialize code buffer */
    CVMJITcbufInitialize(con);

    /* The localrefs used during analysis */
    CVMJITsetInit(con, &con->curRefLocals);
}

void
CVMJITdestroyContext(CVMJITCompilationContext* con)
{
    CVMJITmemFlush(con); /* Free up allocated memory */

    /*
     * Special case for when we generate code into the start of the code cache.
     */
    if (con->mb == NULL) {
	return;
    }

    CVMJITstatsDestroyCompilationContext(con);
}

/*
 * Nothing yet. 
 */
void 
CVMJITcompileOptimizeIR(CVMJITCompilationContext* con)
{
    return;
}

extern void
CVMJIThandleError(CVMJITCompilationContext* con, int error)
{
    (con)->resultCode = error;
    longjmp((con)->errorHandlerContext, 1);
}

extern void
CVMJITlimitExceeded(CVMJITCompilationContext *con, const char *errorStr)
{
    /* If we have been doing some inlining (i.e. con->maxInliningDepth > 0)
       and the cap on inlining depth is still not 0 (i.e.
       con->maxAllowedInliningDepth > 0), then throw an error to allow
       retrying with a lower inlining depth cap. */
    if ((con->maxInliningDepth > 0) && (con->maxAllowedInliningDepth > 0)) {
	/* Try less inlining. */
	CVMJITerror(con, INLINING_TOO_DEEP, "Inlining is too deep");
    } else {
	CVMJITerror(con, CANNOT_COMPILE, errorStr);
    }
}
