/*
 * @(#)bcutils.c	1.23 06/10/25
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
#include "javavm/include/classes.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/stacks.h"
#include "javavm/include/bcattr.h"
#include "javavm/include/utils.h"
#include "javavm/include/typeid.h"
#include "javavm/include/opcodes.h"
#include "javavm/include/globals.h"

#include "javavm/include/porting/int.h"

#include "javavm/include/bcutils.h"

#ifdef CVM_JVMTI
#ifdef CVM_HW
#include "include/hw.h"
#endif
#endif

#ifdef CVM_DEBUG
/*
 * A few debugging routines to help out with byte-codes
 */

/*
 * CVMbcList(mb): Print out the byte-codes of a given method, with
 * associated PC.
 */

static void 
printInst(CVMMethodBlock* mb, CVMUint8* pc, CVMUint32 level, CVMBool doRecurse);

static void
printExceptionTable(CVMJavaMethodDescriptor* jmd, CVMUint32 level);

static void
disassembleRangeWithIndentation(CVMMethodBlock* mb,		      
				CVMUint8* codeStart, 
				CVMUint8* codeEnd,
				CVMUint32 level,
				CVMBool   doRecurse);

static void
indent(CVMUint32 level);

#ifdef CVM_JVMTI
static CVMUint32
getOpcodeLength(CVMExecEnv* ee, CVMUint8* pc)
{
    CVMUint32 opLen;
    /* Make sure we get the underlying instruction length, and not that
       of opc_breakpoint */
    if (*pc == opc_breakpoint) {
	/* Find the length of the original opcode, so we can
	   skip over it by the appropriate amount */
	CVMOpcode instr = CVMjvmtiGetBreakpointOpcode(ee, pc, CVM_FALSE);
	*pc = instr;
	opLen = CVMopcodeGetLength(pc);
	*pc = opc_breakpoint;
#ifdef CVM_HW
	CVMhwFlushCache(pc, pc + 1);
#endif
    } else {
	opLen = CVMopcodeGetLength(pc);
    }
    return opLen;
}
#else
#define getOpcodeLength(ee, pc) (CVMopcodeGetLength(pc))
#endif

/*
 * The most general disassembler.
 * Keeps track of call level 'level' and indents accordingly.
 * Controls recursion based on 'doRecurse'.
 */
void
CVMbcListWithIndentation(CVMMethodBlock* mb, CVMUint32 level, CVMBool doRecurse)
{
    CVMJavaMethodDescriptor *jmd;
    CVMUint8* codeStart;
    CVMUint8* codeEnd;
    if (!CVMmbIsJava(mb)) {
	/* Not a Java method. Give up */
	CVMconsolePrintf("CVMbcList(%M): not a Java method\n", mb);
	return;
    }
    indent(level);
    CVMconsolePrintf("Method %C.%M\n", CVMmbClassBlock(mb), mb);
    jmd = CVMmbJmd(mb);
    indent(level);
    CVMconsolePrintf("Max_stack=%d, max_locals=%d\n",
		     CVMjmdMaxStack(jmd), CVMjmdMaxLocals(jmd));
    codeStart = CVMjmdCode(jmd);
    codeEnd   = &codeStart[CVMjmdCodeLength(jmd)];
    /*
     * Print out the whole range
     */
    disassembleRangeWithIndentation(mb, codeStart, codeEnd, level, doRecurse);

    /* 
     * Also print out exception table
     */
    if (CVMjmdExceptionTableLength(jmd) > 0) {
	printExceptionTable(jmd, level);
    }
}

void
CVMbcList(CVMMethodBlock* mb)
{
    CVMbcListWithIndentation(mb, 0, CVM_FALSE);
}

static void
indent(CVMUint32 level)
{
    while (level-- > 0) {
	CVMconsolePrintf("    ");
    }
}

static void
disassembleRangeWithIndentation(CVMMethodBlock* mb,		      
				CVMUint8* codeStart, 
				CVMUint8* codeEnd,
				CVMUint32 level,
				CVMBool doRecurse)
{
    CVMUint8* mbcodeStart = CVMmbJavaCode(mb);
    CVMUint8* pc;

    /* C stack redzone check */
    if (!CVMCstackCheckSize(CVMgetEE(), CVM_REDZONE_disassembleRangeWithIndentation,
        "disassembleRangeWithIndentation", CVM_FALSE)) {
        return;
    }

    for (pc = codeStart; pc < codeEnd;
	 pc += getOpcodeLength(CVMgetEE(), pc)) {
	indent(level);
	CVMconsolePrintf("<%d>  (0x%x):    ", pc - mbcodeStart, pc);
	printInst(mb, pc, level, doRecurse);
    }
}

void
CVMbcDisassembleRange(CVMMethodBlock* mb,		      
		      CVMUint8* codeStart, 
		      CVMUint8* codeEnd)
{
    disassembleRangeWithIndentation(mb, codeStart, codeEnd, 0, CVM_FALSE);
}

static void
printExceptionTable(CVMJavaMethodDescriptor* jmd, CVMUint32 level)
{
    CVMExceptionHandler *eh = CVMjmdExceptionTable(jmd);
    CVMExceptionHandler *ehEnd = eh + CVMjmdExceptionTableLength(jmd);
	
    indent(level);
    CVMconsolePrintf("Exception table:\n");
    /* Check each exception handler to see if the pc is in its range */
    for (; eh < ehEnd; eh++) { 
	indent(level);
	CVMconsolePrintf("    start=%d\tend=%d\thandler=%d\n",
			 eh->startpc, eh->endpc, eh->handlerpc);
    }
}


static const char * const basicTypeNames[] = {
    0,
    0,
    "Object",
    0,
    "boolean",
    "char",
    "float",
    "double",
    "byte",
    "short",
    "int",
    "long",
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    "void"
};

static void 
printInst(CVMMethodBlock* mb, 
	  CVMUint8* pc, 
	  CVMUint32 level, 
	  CVMBool doRecurse)
{
    CVMClassBlock*   cb = CVMmbClassBlock(mb);
    CVMConstantPool* cp = CVMcbConstantPool(cb);
    CVMUint8*        codeStart = CVMmbJavaCode(mb);
    CVMOpcode        opcode = (CVMOpcode)*pc;
    
    CVMconsolePrintf("%s ", CVMopnames[opcode]);
    switch(opcode) {
    case opc_aload: case opc_astore:
    case opc_fload: case opc_fstore:
    case opc_iload: case opc_istore:
    case opc_lload: case opc_lstore:
    case opc_dload: case opc_dstore:
    case opc_ret: {
	CVMconsolePrintf("%d", pc[1]);
	break;
    }
    case opc_iinc: {
	CVMconsolePrintf("%d %d", pc[1], pc[2]);
	break;
    }
    case opc_newarray: {
	CVMBasicType t = (CVMBasicType)pc[1];
	CVMconsolePrintf("%s", basicTypeNames[t]);
	break;
    }
    case opc_anewarray: case opc_anewarray_quick: {
	CVMUint32 idx = CVMgetInt16(pc+1);
	CVMconsolePrintf("class #%d", idx);
	break;
    }
    case opc_sipush: {
	CVMInt32 number = CVMgetInt16(pc+1);
	CVMconsolePrintf("%d", number);
	break;
    }
    case opc_bipush: {
	CVMInt32 number = pc[1];
	CVMconsolePrintf("%d", number);
	break;
    }
    case opc_ldc_quick: case opc_aldc_quick:
    case opc_aldc_ind_quick: case opc_ldc: {
	CVMUint32 index = pc[1];
	CVMconsolePrintf("#%d", index);
	break;
    }

    case opc_invokevirtual_quick_w:
    case opc_invokenonvirtual_quick:
    case opc_invokestatic_quick:
    case opc_invokestatic_checkinit_quick: 
    case opc_invokeinterface_quick:
    case opc_invokestatic: {
	CVMUint32 index = CVMgetInt16(pc+1);
	CVMconsolePrintf("#%d", index);
	if (CVMcpIsResolved(cp, index)) {
	    CVMMethodBlock* targetMb = CVMcpGetMb(cp, index);
	    CVMconsolePrintf(" <%C.%M>", CVMmbClassBlock(targetMb), targetMb);
	    if (doRecurse && CVMmbIsJava(targetMb) &&
		(CVMmbCodeLength(targetMb) < 20)) {
		CVMconsolePrintf("\n");
		if (level > 10) {
		    CVMconsolePrintf("NO RECURSION -- GETTING TOO DEEP\n");
		} else {
		    CVMbcListWithIndentation(targetMb, level+1, CVM_TRUE);
		}
	    }
	}
	break;
    }

    case opc_invokesuper_quick: {
	CVMUint32 index = CVMgetInt16(pc+1);
	CVMconsolePrintf("#%d", index);
	if (CVMcpIsResolved(cp, index)) {
	    CVMMethodBlock* targetMb = CVMcpGetMb(cp, index);
	    CVMClassBlock* supercb = CVMcbSuperclass(CVMmbClassBlock(targetMb));
	    /* Get superclass method */
	    targetMb = CVMcbMethodTableSlot(supercb, index);
	    CVMconsolePrintf(" <%C.%M>", CVMmbClassBlock(targetMb), targetMb);
	    if (doRecurse && CVMmbIsJava(targetMb) &&
		(CVMmbCodeLength(targetMb) < 20)) {
		CVMconsolePrintf("\n");
		if (level > 10) {
		    CVMconsolePrintf("NO RECURSION -- GETTING TOO DEEP\n");
		} else {
		    CVMbcListWithIndentation(targetMb, level+1, CVM_TRUE);
		}
	    }
	}
	break;
    }

    /*
     * I don't know the targets precisely here, and I don't want to waste
     * any time finding out. 
     */
    case opc_invokespecial:
    case opc_invokevirtual:
    case opc_invokeinterface: {
	CVMUint32 index = CVMgetInt16(pc+1);
	CVMconsolePrintf("#%d", index);
	if (CVMcpIsResolved(cp, index)) {
	    CVMMethodBlock* targetMb = CVMcpGetMb(cp, index);
	    CVMconsolePrintf(" <%C.%M>", CVMmbClassBlock(targetMb), targetMb);
	}
	break;
    }

    case opc_ldc_w: 
    case opc_ldc2_w:
    case opc_ldc_w_quick:
    case opc_aldc_w_quick:
    case opc_aldc_ind_w_quick:
    case opc_ldc2_w_quick: {
	CVMUint32 index = CVMgetInt16(pc+1);
	CVMconsolePrintf("#%d", index);
	break;
    }

    case opc_instanceof: case opc_checkcast:
    case opc_new:
    case opc_new_quick:
    case opc_new_checkinit_quick: 
    case opc_checkcast_quick:
    case opc_instanceof_quick: {
	CVMUint32 index = CVMgetInt16(pc+1);
	CVMconsolePrintf("#%d", index);
	if (CVMcpIsResolved(cp, index)) {
	    CVMClassBlock* targetCb = CVMcpGetCb(cp, index);
	    CVMconsolePrintf(" <%C>", targetCb);
	}
	break;
    }

    case opc_multianewarray_quick: {
	CVMUint32 index = CVMgetInt16(pc+1);
	CVMconsolePrintf("#%d ndim=%d", index, pc[3]);
	break;
    }

    case opc_getstatic:
    case opc_putstatic:
    case opc_getstatic_quick:
    case opc_putstatic_quick:
    case opc_agetstatic_quick:
    case opc_aputstatic_quick: 
    case opc_getstatic2_quick:
    case opc_putstatic2_quick:
    case opc_agetstatic_checkinit_quick:
    case opc_aputstatic_checkinit_quick:
    case opc_getstatic_checkinit_quick: 
    case opc_getstatic2_checkinit_quick: 
    case opc_putstatic_checkinit_quick: 
    case opc_putstatic2_checkinit_quick:  
    case opc_getfield_quick_w:
    case opc_putfield_quick_w:
    case opc_putfield:
    case opc_getfield: {
	CVMUint32 index = CVMgetInt16(pc+1);
	CVMconsolePrintf("#%d", index);
	if (CVMcpIsResolved(cp, index)) {
	    CVMFieldBlock* targetFb = CVMcpGetFb(cp, index);
	    CVMconsolePrintf(" <%C.%F>", CVMfbClassBlock(targetFb), targetFb);
	}
	break;
    }

    case opc_jsr: case opc_goto:
    case opc_ifeq: case opc_ifge: case opc_ifgt:
    case opc_ifle: case opc_iflt: case opc_ifne:
    case opc_if_icmpeq: case opc_if_icmpne: case opc_if_icmpge:
    case opc_if_icmpgt: case opc_if_icmple: case opc_if_icmplt:
    case opc_if_acmpeq: case opc_if_acmpne:
    case opc_ifnull: case opc_ifnonnull: {
	CVMUint32 target = (CVMUint32)(pc - codeStart + CVMgetInt16(pc+1));
	CVMconsolePrintf("<%d>", target);
	break;
    }
    case opc_getfield_quick:
    case opc_putfield_quick:
    case opc_getfield2_quick:
    case opc_putfield2_quick:
    case opc_agetfield_quick:
    case opc_aputfield_quick: {
	CVMUint32 index = pc[1];
	CVMconsolePrintf("#%d", index);
	break;
    }
    case opc_invokevirtualobject_quick:
    case opc_invokevirtual_quick:
    case opc_ainvokevirtual_quick:
    case opc_dinvokevirtual_quick:
    case opc_vinvokevirtual_quick: {
	CVMconsolePrintf("#%d args=%d", pc[1], pc[2]);
	break;
    }
    default: break; /* print nothing */
    }
    CVMconsolePrintf("\n");
}

#endif /* CVM_DEBUG */
