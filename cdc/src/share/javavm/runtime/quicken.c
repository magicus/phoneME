/*
 * @(#)quicken.c	1.57 06/10/25
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

#ifdef CVM_CLASSLOADING

#include "javavm/include/interpreter.h"
#include "javavm/include/classes.h"
#include "javavm/include/utils.h"
#include "javavm/include/preloader.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/opcodes.h"
#include "javavm/include/globals.h"
#include "javavm/include/bcattr.h"
#include "javavm/include/indirectmem.h"

#ifdef CVM_JVMTI
#include "javavm/include/jvmtiExport.h"
#endif

#ifdef CVM_HW
#include "include/hw.h"
#endif

/*
 * GET_INDEX - Macro used for getting an unaligned unsigned short from
 * the byte codes.
 */
#undef  GET_INDEX
#define GET_INDEX(ptr) (CVMgetUint16(ptr))

/* Used to be that we defined CVM_NO_LOSSY_OPCODES in the makefile.
 * Now we do a runtime check to determine if we will quicken to
 * a lossy opcode.  This is so we can build with JVMTI code compiled
 * in but run with the non-jvmti interpreter loop and with lossy
 * romized opcodes.  JVMPI instruction tracing needs lossless opcodes
 * as well, hence this bool.
 */
#ifdef CVM_JVMPI_TRACE_INSTRUCTION
static CVMBool isLossy = CVM_FALSE;
#else
static CVMBool isLossy = CVM_TRUE;
#endif

/* 
 * Macro to make sure that no register caching is done when we
 * write a new byte code.
 */
#undef  CVM_WRITE_CODE_BYTE
#define CVM_WRITE_CODE_BYTE(ptr, offset, newvalue) \
    ((volatile CVMUint8*) (ptr))[(offset)] = (newvalue);

static CVMQuickenReturnCode
CVMquickenOpcodeHelper(CVMExecEnv* ee, CVMUint8* quickening, CVMUint8* pc, 
		       CVMConstantPool* cp, CVMClassBlock** p_cb,
		       CVMBool clobbersCpIndex);

/*
 * CVMopcodeWasQuickened - check to see if the opcode was already quickened.
 * Note: If breakpoints are enabled and we have to deal with
 * two threads trying to quicken at the same time.
 *
 * WARNING:  Must be executed within the code lock.
 *
 *   p_opcode: Pointer to the opcode we are quickening. The current opcode
 *             at *pc is also returned here. Once this is updated, the code
 *             lock is not released, so the opcode remains valid.
 *   pc:       Pointer to the instruction we are quickening
 */
static CVMBool
CVMopcodeWasQuickened(CVMExecEnv* ee, CVMUint8* p_opcode, CVMUint8* pc)
{
    /*
     * IMPORTANT NOTE: This is not allowed to block, since it is executed
     * under the global CODE_LOCK microlock.
     */

#ifdef CVM_JVMTI
    if (*pc == opc_breakpoint) {
	/* If the original opcode is an opc_breakpoint, then we need call
	 * the debugger to get the real opcode and make sure that it
	 * hasn't been quickened already.
	 */
	CVMUint8 breakpointOpcode =
	    CVMjvmtiGetBreakpointOpcode(ee, pc, CVM_FALSE);
	*p_opcode = breakpointOpcode;  /* alway return updated opcode */
    } else {
	*p_opcode = *pc;  /* alway return updated opcode */
    }
#else
    *p_opcode = *pc;  /* alway return updated opcode */
#endif

    return CVMbcAttr(*p_opcode, QUICK);
}


/*
 * CVMquickenOpcodeHelper - quickens all opcodes.
 *
 * quickening:
 *            A pointer to a three-byte location. On entry, the first byte
 *            contains the instruction to be quickened. On exit, quickening[]
 *            contains the new instruction (and/or the operands) to be written
 *            into the instruction stream.
 * pc:        A pointer to the current instruction.
 * cp:        A pointer to the constant pool.
 * p_cb:      A place to store the CVMClassBlock* in case static initializers
 *            still need to be run. The interpreter needs this. Otherwise
 *            it would need to enter the code lock and repeat what
 *            CVMopcodeWasQuickened() has already done in order to get
 *            the cb from the constant pool.
 * clobbersCpIndex: true if quickening the opcode will cause the constant
 *            pool index in the instruction to be overwritten.
 *
 * Possible return values are:
 *
 *    CVM_QUICKEN_SUCCESS_OPCODE_ONLY:
 *        quickening succeeded, only opcode changed.
 *    CVM_QUICKEN_SUCCESS_OPCODE_AND_OPERANDS
 *        quickening succeeded, opcode and operands changed.
 *    CVM_QUICKEN_NEED_TO_RUN_STATIC_INITIALIZERS
 *        need to run static initializers, *p_cb updated. The interpreter
 *        needs to run static initializers first and then call
 *        CVMquickenOpcode() again.
 *    CVM_QUICKEN_ALREADY_QUICKENED
 *        someone else already raced us to it
 *            OR 
 *        in the JVMTI case, the instruction stream is already
 *        updated in CVMquickenOpcode().
 *    CVM_QUICKEN_ERROR
 *        an error occurred, exception pending
 *
 * On exit  quickening[0] contains the new opcode.
 *          quickening[1,2] contain the new operands if they have changed.
 *
 */
static CVMQuickenReturnCode
CVMquickenOpcodeHelper(CVMExecEnv* ee, CVMUint8* quickening, CVMUint8* pc, 
		       CVMConstantPool* cp, CVMClassBlock** p_cb,
		       CVMBool clobbersCpIndex)
{
    CVMUint8        opcode;
    CVMUint16       cpIndex;
    CVMBool         isStaticOpcode = CVM_FALSE; /* default to false */
    CVMBool         isPutOpcode = CVM_FALSE;    /* default to false */
    CVMClassBlock*  cb = NULL;
    CVMMethodBlock* mb = NULL; /* only setup for some invoke opcodes */
    CVMFieldBlock*  fb = NULL; /* only setup for field opcodes */
    CVMBool wasQuickened;

    /* grab the CVM_CODE_LOCK if necessary */
    if (clobbersCpIndex) {
	/*
	 * We can't trust the cpIndex that we will fetch from the instruction 
	 * stream unless we also verify that the opcode hasn't been quickened.
	 * If the opcode was already quickened then there is nothing for 
	 * us to do.
	 *
	 * Grabbing the code lock ensures that no one is in the middle
	 * of quickening when we check to see if the opcode has been
	 * quickened yet. We want to avoid the race condition where the
	 * index has been overwritten, but the opcode has not been
	 * written yet. By calling CVMopcodeWasQuickened
	 * from within the code lock, we guarantee that if the opcode
	 * wasn't quickened then the cpIndex is good. If the opcode
	 * has been quickened, then we don't care about cpIndex.
	 */
#ifdef CVM_JVMTI
	CVM_JVMTI_LOCK(ee);
#else
	CVM_CODE_LOCK(ee);
#endif
    }

    wasQuickened = CVMopcodeWasQuickened(ee, quickening, pc);
    opcode = quickening[0];

    /*
     * If quickening clobbers the cpIndex, then we must fetch it within
     * the CVM_CODE_LOCK.
     */
    if (opcode == opc_ldc) {
	cpIndex = pc[1];
    } else {
	cpIndex = GET_INDEX(pc + 1);
    }

    /* release the CVM_CODE_LOCK if necessary */
    if (clobbersCpIndex) {
#ifdef CVM_JVMTI
	CVM_JVMTI_UNLOCK(ee);
#else
	CVM_CODE_UNLOCK(ee);
#endif
    }

    if (wasQuickened) { 
	/* opcode has already been quickened */
	return CVM_QUICKEN_ALREADY_QUICKENED;
    }

    if (!CVMcpResolveEntry(ee, cp, cpIndex)) {
	return CVM_QUICKEN_ERROR;
    }

    /*
     * Do any required error checking an handling.
     */
    switch (opcode) {
    case opc_new: {
	cb = CVMcpGetCb(cp, cpIndex);

#ifndef CVM_TRUSTED_CLASSLOADERS
	/*
	 * Make sure we are not trying to instantiate an iterface or an
	 * abstract class.
	 */
        if (!CVMclassIsOKToInstantiate(ee, cb)) {
	    return CVM_QUICKEN_ERROR;
	}
#endif
	/*
	 * If static intializers still need to be run for this class, then
	 * leave it up to the interpreter to do this for us first. 
	 */
	if (CVMcbInitializationNeeded(cb, ee)) {
	    *p_cb = cb; /* return cb to the interpreter. */
	    return CVM_QUICKEN_NEED_TO_RUN_STATIC_INITIALIZERS;
	}
	break;
    }
    case opc_putstatic:
	isStaticOpcode = CVM_TRUE;
	/* fall through */
    case opc_putfield:
	isPutOpcode = CVM_TRUE;
	goto check_field;
    case opc_getstatic:
	isStaticOpcode = CVM_TRUE;
	/* fall through */
    case opc_getfield:
    check_field: {
	    fb = CVMcpGetFb(cp, cpIndex);
	    cb = CVMfbClassBlock(fb);

#ifndef CVM_TRUSTED_CLASSLOADERS
	    /*
	     * Make sure that both the opcode and the fb agree on whether or
	     * not the field is static.
	     */
            if (!CVMfieldHasNotChangeStaticState(ee, fb, isStaticOpcode)) {
		return CVM_QUICKEN_ERROR;
	    }

	    /*
	     * Make sure we aren't trying to store into a final field.
	     * The VM spec says that you can only store into a final field
	     * when initializing it. The JDK seems to interpret this as
	     * meaning that a class has write access to its final fields,
	     * so we do the same here (see how isFinalField gets setup above).
	     */
            if (isPutOpcode) {
		if (!CVMfieldIsOKToWriteTo(ee, fb, CVMeeGetCurrentFrameCb(ee),
					   CVM_TRUE)) {
		    return CVM_QUICKEN_ERROR;
		}
	    } 
#endif /* CVM_TRUSTED_CLASSLOADERS */

	    /*
	     * If static intializers still need to be run for this class,
	     * then leave it up to the interpreter to do this for us 
	     * first.
	     */
	    if (CVMcbInitializationNeeded(cb, ee)) {
		*p_cb = cb; /* return cb to the interpreter. */
		return CVM_QUICKEN_NEED_TO_RUN_STATIC_INITIALIZERS;
	    }
	    break;
	}
    case opc_invokestatic:
	isStaticOpcode = CVM_TRUE;
	goto check_method;
    case opc_invokevirtual:
    case opc_invokespecial:
    check_method: {
	    mb = CVMcpGetMb(cp, cpIndex);
	    cb = CVMmbClassBlock(mb);
#ifndef CVM_TRUSTED_CLASSLOADERS
	    /*
	     * Make sure that both the opcode and the mb agree on whether or
	     * not the method is static.
	     */
            if (!CVMmethodHasNotChangeStaticState(ee, mb, isStaticOpcode)) {
		return CVM_QUICKEN_ERROR;
	    } 
#endif
	    /*
	     * If static intializers still need to be run for this class, then
	     * leave it up to the interpreter to do this for us first. 
	     */
	    if ((opcode == opc_invokestatic) 
		&& CVMcbInitializationNeeded(cb, ee)) {
		*p_cb = cb; /* return cb to the interpreter. */
		return CVM_QUICKEN_NEED_TO_RUN_STATIC_INITIALIZERS;
	    }
	    break;
	}
    case opc_invokeinterface:
    case opc_anewarray:
    case opc_multianewarray:
    case opc_checkcast:
    case opc_instanceof:
    case opc_ldc:
    case opc_ldc_w:
    case opc_ldc2_w:
	/* nothing to do for these opcodes */
	break;
    default:
	CVMassert(CVM_FALSE);
    }

    /*
     * Get the new instruction bytes for the quickened opcode.
     */
    {
	CVMUint8  newOpcode = opc_xxxunusedxxx;/* opcode we will quicken to. */
	CVMUint8  operand1 = 0;  /* 1st instruction operand if needed. */
	CVMUint8  operand2 = 0;  /* 2nd instruction operand if needed. */

	CVMBool   changesOperands = CVM_FALSE; /* true if quickening changes 
						* the operands */

	/*
	 * Assign newOpcode to the opcode we are going to quicken to.
	 * If the 1st or 2nd operand bytes are also modified, set operand1
	 * and operand2 to the proper values and set the changesOperands flag.
	 */
	switch (opcode) {
	case opc_new:
	    if (CVMcbCheckInitNeeded(cb, ee)) {
		newOpcode = opc_new_checkinit_quick;
	    } else {
		newOpcode = opc_new_quick;
	    }
	    break;
	case opc_anewarray:
	    newOpcode = opc_anewarray_quick;
	    break;
	case opc_multianewarray:
	    newOpcode = opc_multianewarray_quick;
	    break;
	case opc_checkcast:
	    newOpcode = opc_checkcast_quick;
	    break;
	case opc_instanceof:
	    newOpcode = opc_instanceof_quick;
	    break;
	case opc_ldc:
	    switch (CVMcpEntryType(cp, cpIndex)) {
	    case CVM_CONSTANT_StringICell:
		newOpcode = opc_aldc_ind_quick;
		break;
	    case CVM_CONSTANT_ClassBlock:
		CVMpanic("Trying to quicken ldc of type Class");
		break;
	    default:
		newOpcode = opc_ldc_quick;
	    }
	    break;
	case opc_ldc_w:
	    switch (CVMcpEntryType(cp, cpIndex)) {
	    case CVM_CONSTANT_StringICell:
		newOpcode = opc_aldc_ind_w_quick;
		break;
	    case CVM_CONSTANT_ClassBlock:
		CVMpanic("Trying to quicken ldc of type Class");
		break;
	    default:
		newOpcode = opc_ldc_w_quick;
	    }
	    break;
	case opc_ldc2_w:
	    newOpcode = opc_ldc2_w_quick;
	    break;
	case opc_invokeinterface:
	    newOpcode = opc_invokeinterface_quick;
	    break;
	case opc_invokestatic:
	    if (CVMcbCheckInitNeeded(cb, ee)) {
		newOpcode = opc_invokestatic_checkinit_quick;
	    } else {
		newOpcode = opc_invokestatic_quick;
	    }
	    break;
	case opc_invokevirtual:
	    if (CVMmbIs(mb, PRIVATE) || CVMmbIs(mb, FINAL) ||
		CVMcbIs(cb, FINAL)) {
		newOpcode = opc_invokenonvirtual_quick;
	    }
	    /* This used to be #ifndef CVM_NO_LOSSY_OPCODES but now
	     * we determine at runtime if we are going to quicken to
	     * a non-lossy opcode based on whether JVMPI instruction tracing
	     * is on (rare) or some JVMTI agent has connected with the VM.
	     * This is so we can build with JVMTI code compiled
	     * in but run with the non-jvmti interpreter loop and with lossy
	     * romized opcodes. 
	     */
#ifndef CVM_JIT
	    /* The JIT causes unconditional quickening to
	     *  invokevirtual_quick_w for inlining purposes */

	    else if (isLossy &&
#ifdef CVM_JVMTI
                !CVMjvmtiIsEnabled() &&
#endif
             CVMmbMethodTableIndex(mb) <= 0xFF) {
		if (CVMmbClassBlock(mb) == CVMsystemClass(java_lang_Object)) {
		    newOpcode = opc_invokevirtualobject_quick;
		} else switch(CVMtypeidGetReturnType(CVMmbNameAndTypeID(mb))) {
		case CVM_TYPEID_VOID:
		    newOpcode = opc_vinvokevirtual_quick;
		    break;
		case CVM_TYPEID_OBJ:
		    newOpcode = opc_ainvokevirtual_quick;
		    break;
		case CVM_TYPEID_LONG:
		case CVM_TYPEID_DOUBLE:
		    newOpcode = opc_dinvokevirtual_quick;
		    break;
		default:
		    newOpcode = opc_invokevirtual_quick;
		    break;
		}
		operand1 = CVMmbMethodTableIndex(mb);
		operand2 = CVMmbArgsSize(mb);
		changesOperands = CVM_TRUE;
	    }
#endif
	    else {
		newOpcode = opc_invokevirtual_quick_w;
	    }
	    break;
	case opc_invokespecial: {
	    CVMMethodBlock* new_mb = mb;
	    CVMMethodTypeID methodID = CVMmbNameAndTypeID(mb);
	    CVMClassBlock* currClass = CVMeeGetCurrentFrameCb(ee);
	    if (CVMisSpecialSuperCall(currClass, mb)) {
		/* Find matching declared method in a super class. */
		new_mb = CVMlookupSpecialSuperMethod(ee, currClass, methodID);
	    }
	    if (mb == new_mb) {
		if (CVMmbClassBlock(mb) == CVMsystemClass(java_lang_Object) &&
		    methodID == CVMglobals.initTid) {
		    /* we can ignore all calls to Object.<init> */
		    newOpcode = opc_invokeignored_quick;
		    CVMassert(CVMmbArgsSize(mb) == 1);
		    operand1 = 1;
		    operand2 = CVM_TRUE; /* null check needed */
		    changesOperands = CVM_TRUE;
		} else {
		    newOpcode = opc_invokenonvirtual_quick;
		}
	    } else {
		newOpcode = opc_invokesuper_quick;
		operand1 = CVMmbMethodTableIndex(new_mb) >> 8;
		operand2 = CVMmbMethodTableIndex(new_mb) & 0xFF;
		changesOperands = CVM_TRUE;
	    }
	    break;
	}

	case opc_putstatic:
	case opc_putfield:
	case opc_getstatic:
	case opc_getfield: {
	    if (isStaticOpcode) {	    
		if (CVMcbCheckInitNeeded(cb, ee)) {
		    if (CVMfbIsRef(fb)) {
			newOpcode = (isPutOpcode 
				     ? opc_aputstatic_checkinit_quick
				     : opc_agetstatic_checkinit_quick);
		    } else if (CVMfbIsDoubleWord(fb)) {
			newOpcode = (isPutOpcode
				     ? opc_putstatic2_checkinit_quick
				     : opc_getstatic2_checkinit_quick);
		    } else {
			newOpcode = (isPutOpcode
				     ? opc_putstatic_checkinit_quick
				     : opc_getstatic_checkinit_quick);
		    }
		} else {
		    if (CVMfbIsRef(fb)) {
			newOpcode = (isPutOpcode ? opc_aputstatic_quick
						 : opc_agetstatic_quick);
		    } else if (CVMfbIsDoubleWord(fb)) {
			newOpcode = (isPutOpcode ? opc_putstatic2_quick
						 : opc_getstatic2_quick);
		    } else {
			newOpcode = (isPutOpcode ? opc_putstatic_quick
						 : opc_getstatic_quick);
		    }
		}
	    } else {
		CVMBool getfieldQuickeningAllowed;
		CVMUint16 offset = CVMfbOffset(fb);
                /* Only quicken if the field is non-volatile. For volatile
                 * fields, opc_putfield_quick_w and opc_getfield_quick_w
                 * are used.
                 */
		getfieldQuickeningAllowed = !CVMfbIs(fb, VOLATILE);
		/* We determine at runtime if we are going to quicken to
		 * a lossy opcode based on whether JVMPI instruction tracing
		 * is on (rare) or some JVMTI agent has connected with the VM
		 */
		if (isLossy &&
#ifdef CVM_JVMTI
                    !CVMjvmtiIsEnabled() &&
#endif
                    (offset <= 0xFF) && getfieldQuickeningAllowed) {
		    if (CVMfbIsDoubleWord(fb)) {
			newOpcode = (isPutOpcode ? opc_putfield2_quick
				                 : opc_getfield2_quick);
		    } else if (CVMfbIsRef(fb)) { 
			newOpcode = (isPutOpcode ? opc_aputfield_quick
				                 : opc_agetfield_quick);
		    } else {
			newOpcode = (isPutOpcode ? opc_putfield_quick
				                 : opc_getfield_quick);
		    }
		    operand1 = offset;
		    operand2 = 0; /* not really needed */
		    changesOperands = CVM_TRUE;
		} else

		{
		    newOpcode = (isPutOpcode ? opc_putfield_quick_w 
				             : opc_getfield_quick_w);
		}
	    }
	}
	break;
	
	default:
	    CVMassert(CVM_FALSE);
	    break;
	}
	CVMassert(newOpcode != opc_xxxunusedxxx);

#if 0	
	CVMdebugPrintf(("\t%squickening %s to %s\n", 
			(changesOperands ? "+" : "-"),
			CVMopnames[opcode], CVMopnames[newOpcode]));
#endif
	
	/*
	 * Write out the new instruction. 
	 */
	quickening[0] = newOpcode;	/* newOpcode is known by now */ 
#ifdef CVM_JVMTI
	/*
	 * Don't let *pc change to an opc_breakpoint while we are trying
	 * to determine its current status.
	 */
	CVM_JVMTI_LOCK(ee);

	/*
	 * If we have to modify the instruction operands, then the
	 * instruction must be quickened within the code lock. Also, we
	 * must always modify the operands before the opcode so the
	 * interpreter will never fetch a quick opcode before the
	 * operands are written.
	 */
	if (changesOperands) {
	    CVM_CODE_LOCK(ee);
	    CVM_WRITE_CODE_BYTE(pc, 1, operand1);
	    CVM_WRITE_CODE_BYTE(pc, 2, operand2);
#ifdef CVM_HW
	    CVMhwFlushCache(pc + 1, pc + 3);
#endif
#ifdef CVM_MP_SAFE
	    CVMmemoryBarrier();
#endif
	}
	/* Don't overwrite existing breakpoint opcode */
	if (*pc != opc_breakpoint) {
	    CVM_WRITE_CODE_BYTE(pc, 0, newOpcode);
#ifdef CVM_HW
	    CVMhwFlushCache(pc, pc + 1);
#endif
	} else {
	    /* notify debugger of new opcode at breakpoint */
	    CVMjvmtiSetBreakpointOpcode(ee, pc, newOpcode);
	}
	if (changesOperands) {
	    CVM_CODE_UNLOCK(ee);
	}

	CVM_JVMTI_UNLOCK(ee);
	return CVM_QUICKEN_ALREADY_QUICKENED;
#else /* CVM_JVMTI */
	if (changesOperands) {
	    quickening[1] = operand1;
	    quickening[2] = operand2;
	    return CVM_QUICKEN_SUCCESS_OPCODE_AND_OPERANDS;
	} else {
	    return CVM_QUICKEN_SUCCESS_OPCODE_ONLY;
	}
#endif /* CVM_JVMTI */

    }
}

/*
 * Quicken the opcode. Possible return values are:
 *
 *    CVM_QUICKEN_ALREADY_QUICKENED
 *        the opcode has been quickened
 *    CVM_QUICKEN_NEED_TO_RUN_STATIC_INITIALIZERS
 *        need to run static initializers for cb
 *    CVM_QUICKEN_ERROR
 *        an error occurred, exception pending
 */
CVMQuickenReturnCode
CVMquickenOpcode(CVMExecEnv* ee, CVMUint8* pc, CVMConstantPool* cp,
		 CVMClassBlock** p_cb, CVMBool clobbersCpIndex)
{
    CVMUint8 quickening[3];
    CVMUint8 newOpcode;
    CVMQuickenReturnCode retCode;
    CVMD_gcSafeExec(ee, {
	/*
	 * Call quicken helper. Possible return values are:
	 *
	 *    CVM_QUICKEN_SUCCESS_OPCODE_ONLY:
	 *        quickening succeeded, only opcode changed.
	 *    CVM_QUICKEN_SUCCESS_OPCODE_AND_OPERANDS
	 *        quickening succeeded, opcode and operands changed.
	 *    CVM_QUICKEN_NEED_TO_RUN_STATIC_INITIALIZERS
	 *        need to run static initializers, p_cb updated
	 *    CVM_QUICKEN_ALREADY_QUICKENED
	 *        someone else already raced us to it
	 *            OR 
	 *        in the JVMTI case, the instruction stream is already
	 *        updated in CVMquickenOpcode().
	 *    CVM_QUICKEN_ERROR
	 *        an error occurred, exception pending
	 *
	 * Upon success, either quickening[] holds the instruction
	 * sequence to replace the current one (max. length 3),
	 * or we need to run static initializers, in which case
	 * "cb" will contain the CVMClassBlock* to initialize.
	 */
	quickening[0] = pc[0];
	quickening[1] = 0; quickening[2] = 0; /* prevent compiler warning */
	retCode = CVMquickenOpcodeHelper(ee, quickening, pc, cp, p_cb,
					 clobbersCpIndex);
	newOpcode = quickening[0];
    });
#ifdef CVM_JVMTI
    /* These two should never happen with JVMTI. CVMquickenOpcode()
     * should take care of updating the instruction stream
     */
    CVMassert(retCode != CVM_QUICKEN_SUCCESS_OPCODE_AND_OPERANDS);
    CVMassert(retCode != CVM_QUICKEN_SUCCESS_OPCODE_ONLY);
#endif
    /*
     * We should write to the instruction stream only when GC-unsafe.
     * Otherwise, a GC may occur after the instruction stream
     * has been updated with the quick opcode, and the stackmap
     * computer would skip this GC-safe point. GC points without
     * stackmaps are very bad.
     *
     * (See bug #4353843)
     */
    switch (retCode) {
#ifndef CVM_JVMTI
        case CVM_QUICKEN_SUCCESS_OPCODE_ONLY: {
	    CVM_WRITE_CODE_BYTE(pc, 0, newOpcode);
#ifdef CVM_HW
	    CVMhwFlushCache(pc, pc + 1);
#endif
	    retCode = CVM_QUICKEN_ALREADY_QUICKENED;
	    break;
	}
        case CVM_QUICKEN_SUCCESS_OPCODE_AND_OPERANDS: {
	    /* If we have to modify the instruction operands, then
	     * the instruction must be quickened within the code
	     * lock. Also, we must always modify the operands
	     * before the opcode so the interpreter will never
	     * fetch a quick opcode before the operands are
	     * written. */
	    CVM_CODE_LOCK(ee);
	    CVM_WRITE_CODE_BYTE(pc, 1, quickening[1]);
	    CVM_WRITE_CODE_BYTE(pc, 2, quickening[2]);
#ifdef CVM_HW
	    CVMhwFlushCache(pc + 1, pc + 3);
#endif
#ifdef CVM_MP_SAFE
	    CVMmemoryBarrier();
#endif
	    CVM_WRITE_CODE_BYTE(pc, 0, newOpcode);
#ifdef CVM_HW
	    CVMhwFlushCache(pc, pc + 1);
#endif
	    CVM_CODE_UNLOCK(ee);
	    CVMassert(newOpcode == quickening[0]);
	    retCode = CVM_QUICKEN_ALREADY_QUICKENED;
	    break;
	}
#endif
        case CVM_QUICKEN_ALREADY_QUICKENED:
        case CVM_QUICKEN_NEED_TO_RUN_STATIC_INITIALIZERS:
        case CVM_QUICKEN_ERROR:
	    /* These are all handled by the interpreter loop */
	    break;
        default:
	    CVMassert(CVM_FALSE); /* We should never get here */
    }
    return retCode;
}
#endif /* CVM_CLASSLOADING */
