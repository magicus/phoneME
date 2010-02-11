/*
 * @(#)jitopcodes.c	1.27 06/10/10
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

#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitir.h"
#include "javavm/include/jit/jitirnode.h"
#include "portlibs/jit/risc/jitopcodes.h"
#include "portlibs/jit/risc/include/porting/jitriscemitter.h"

int
CVMJITgetMassagedIROpcode(CVMJITCompilationContext *con, CVMJITIRNode* ip)
{
    int fulltag = ip->tag;
    int typetag = CVMJITgetTypeTag(ip);
    switch (CVMJITgetOpcode(ip) >> CVMJIT_SHIFT_OPCODE) {
    case CVMJIT_GET_OBJECT_VTBL:
	/* for our purposes, these are identical */
	return GET_VTBL;
    case CVMJIT_NEW_ARRAY_BOOLEAN:
    case CVMJIT_NEW_ARRAY_CHAR:
    case CVMJIT_NEW_ARRAY_FLOAT:
    case CVMJIT_NEW_ARRAY_DOUBLE:
    case CVMJIT_NEW_ARRAY_BYTE:
    case CVMJIT_NEW_ARRAY_SHORT:
    case CVMJIT_NEW_ARRAY_INT:
    case CVMJIT_NEW_ARRAY_LONG:
	/* for our purposes, these are identical */
	/* codegen will extract true type after pattern match */
	return NEW_ARRAY_BASIC;
    case CVMJIT_ASSIGN:
#ifdef CVM_JIT_REGISTER_LOCALS
	if (ip->decorationType == CVMJIT_ASSIGN_DECORATION) {
	    CVMRMsetAssignTarget(con, ip);
	}
#endif
	return ASSIGN;
    case CVMJIT_INDEX:
	/* fold all 16-bit operations into the short type */
	/* (this may apply for some of the below, too, but we haven't
	 * analyzed yet.)
	 */
	switch (typetag){
	case CVM_TYPEID_SHORT:
	case CVM_TYPEID_CHAR:
	case CVM_TYPEID_BYTE:
	case CVM_TYPEID_OBJ:    
	    return fulltag; /* don't fall through for these one */
	}
        goto doFoldingIntoInt;

    case CVMJIT_BCOND:
        if (typetag == CVM_TYPEID_OBJ ||
            typetag == CVMJIT_TYPEID_ADDRESS) {
	    fulltag = fulltag & ~CVMJIT_TYPE_MASK;
	    fulltag = fulltag | CVM_TYPEID_INT;
        }
        return fulltag;

    case CVMJIT_DEFINE:
	CVMRMsetDefineTarget(con, ip);
    case CVMJIT_USED:
    case CVMJIT_IDENTITY:
    case CVMJIT_PARAMETER:
    case CVMJIT_INVOKE:
#ifdef CVM_JIT_USE_FP_HARDWARE
	switch (typetag){
	case CVM_TYPEID_FLOAT:
	case CVM_TYPEID_DOUBLE:
	    return fulltag;
	}
#endif
	goto doFoldingIntoInt;
    case CVMJIT_LOCAL:
#ifdef CVM_JIT_REGISTER_LOCALS
	if (ip->decorationType == CVMJIT_LOCAL_DECORATION) {
	    CVMRMsetLocalTarget(con, ip);
	}
#endif
    case CVMJIT_FETCH:
    case CVMJIT_INTRINSIC:
    case CVMJIT_RETURN_VALUE:
    doFoldingIntoInt:
	/* cases where we fold all 32-bit operations into the int type */
	switch (typetag){
	case CVM_TYPEID_SHORT:
	case CVM_TYPEID_CHAR:
	case CVM_TYPEID_BYTE:
	case CVM_TYPEID_BOOLEAN:
	    /*
	     * We should never see the above because they are converted by
	     * the IR generator to INT.
	     */
	    CVMassert(CVM_FALSE);
	case CVM_TYPEID_INT:
	case CVM_TYPEID_FLOAT:
	case CVM_TYPEID_OBJ:
	case CVMJIT_TYPEID_32BITS:
        case CVMJIT_TYPEID_ADDRESS:
	    fulltag = fulltag & ~CVMJIT_TYPE_MASK;
	    return fulltag | CVM_TYPEID_INT;
	/* cases where we fold all 64-bit operations into the long type */
	case CVM_TYPEID_LONG:
	case CVM_TYPEID_DOUBLE:
	case CVMJIT_TYPEID_64BITS:
	    fulltag = fulltag & ~CVMJIT_TYPE_MASK;
	    return fulltag | CVM_TYPEID_LONG;
	case CVM_TYPEID_VOID:
	    break;
	default:
	    CVMassert(CVM_FALSE);
	}
	return fulltag;

    case CVMJIT_IARG:
        fulltag = fulltag & ~CVMJIT_TYPE_MASK;
        return fulltag | CVM_TYPEID_VOID;

    case CVMJIT_SEQUENCE_R:
    case CVMJIT_SEQUENCE_L:
#ifdef CVM_JIT_USE_FP_HARDWARE
	/* We have rules for these when FP is supported */
	if (typetag == CVM_TYPEID_DOUBLE || typetag == CVM_TYPEID_FLOAT) {
	    return fulltag;
	}
#endif
	/* Keep "untyped" SEQUENCE as is. */
        if (typetag != CVM_TYPEID_NONE) {
            goto doFoldingIntoInt;
        }
        return fulltag;

    case CVMJIT_STATIC_FIELD_REF:
	/* We fold volatile 64-bit field refs into special nodes in order to ensure
	   the atomicity of 64-bit accesses: */
	if (typetag == CVM_TYPEID_LONG || typetag == CVM_TYPEID_DOUBLE) {
	    if (CVMJITirnodeUnaryNodeIs(ip, VOLATILE_FIELD)) {
		fulltag = fulltag & ~CVMJIT_TYPE_MASK;
		return fulltag | CVMJITCG_TYPEID_64BITS_VOLATILE;
	    }	    
        }
        goto doFoldingIntoInt;

    case CVMJIT_FIELD_REF:
	/* We fold volatile 64-bit field refs into special nodes in order to ensure
	   the atomicity of 64-bit accesses: */
	if (typetag == CVM_TYPEID_LONG || typetag == CVM_TYPEID_DOUBLE) {
	    if (CVMJITirnodeBinaryNodeIs(ip, VOLATILE_FIELD)) {
		fulltag = fulltag & ~CVMJIT_TYPE_MASK;
		return fulltag | CVMJITCG_TYPEID_64BITS_VOLATILE;
	    }	    
        }
        /* We do not fold object ref fields because they need special treatment
           e.g. memory write barriers: */
	if (typetag != CVM_TYPEID_OBJ) {
            goto doFoldingIntoInt;
        }
        return fulltag;

    case CVMJIT_CONST_JAVA_NUMERIC64:
	CVMassert(typetag == CVM_TYPEID_LONG ||
		  typetag == CVM_TYPEID_DOUBLE || 
		  typetag == CVMJIT_TYPEID_64BITS);
	return ICONST_64;

    case CVMJIT_CONST_METHODTABLE_INDEX: {
        CVMJITConstant32 *constant32 = CVMJITirnodeGetConstant32(ip);
        /* Adjust the method index into an offset into the method table: */
        constant32->v32 = constant32->v32 * sizeof(CVMMethodBlock *);
        return ICONST_32;
    }
    case CVMJIT_CONST_FIELD_OFFSET: {
        CVMJITConstant32 *constant32 = CVMJITirnodeGetConstant32(ip);
        /* Adjust the field offset (which is specified in words) into a byte
           offset: */
        constant32->v32 = constant32->v32 * sizeof(CVMJavaVal32);
        return ICONST_32;
    }
    case CVMJIT_CONST_JAVA_NUMERIC32:
	CVMassert(typetag == CVM_TYPEID_INT ||
		  typetag == CVM_TYPEID_FLOAT ||
		  typetag == CVMJIT_TYPEID_32BITS);
    case CVMJIT_CONST_STRING_OBJECT:
    case CVMJIT_CONST_STATIC_FIELD_ADDRESS:
        return ICONST_32;

    default:
	return fulltag;
    }
}
