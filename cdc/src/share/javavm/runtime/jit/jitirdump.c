/*
 * @(#)jitirdump.c	1.102 06/10/10
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
#ifdef CVM_TRACE_JIT

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/utils.h"
#include "javavm/include/bcutils.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitir.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitirlist.h"
#include "javavm/include/jit/jitirblock.h"
#include "javavm/include/porting/doubleword.h"

#include "javavm/include/clib.h"

static const char *const opcodeTagMap[] = {
    "CVMJITOP_INVALID",

    "CONST_JAVA_NUMERIC32", 	/* CVMJITConstant32 */
    "CONST_JAVA_NUMERIC64", 	/* CVMJITConstant64 */
    "CONST_STRING_ICELL",
    "CONST_STRING_OBJECT",
    "CONST_METHODTABLE_INDEX",
#ifdef CVM_LVM
    "CONST_FB",
#endif
    "CONST_MC",
    "CONST_MB",
    "CONST_CB",
    "CONST_FIELD_OFFSET",
    "CONST_STATIC_FIELD_ADDRESS",

    "CONST_NEW_CB_UNRESOLVED",
    "CONST_CB_UNRESOLVED",
    "CONST_ARRAY_CB_UNRESOLVED",
    "CONST_GETFIELD_FB_UNRESOLVED",
    "CONST_PUTFIELD_FB_UNRESOLVED",
    "CONST_GETSTATIC_FB_UNRESOLVED",
    "CONST_PUTSTATIC_FB_UNRESOLVED",
    "CONST_STATIC_MB_UNRESOLVED",
    "CONST_SPECIAL_MB_UNRESOLVED",
    "CONST_INTERFACE_MB_UNRESOLVED",
    "CONST_METHOD_TABLE_INDEX_UNRESOLVED",

    "NULL_PARAMETER",	/* CVMJITNull */
    "NULL_IARG",
    "JSR_RETURN_ADDRESS",
    "EXCEPTION_OBJECT",
    "RETURN",

    "LOCAL",		/* CVMJITLocal */

    "ARRAY_LENGTH", 	/* CVMJITUnaryOp */
    "GET_VTBL",		/* invokevirtual */
    "GET_OBJECT_VTBL",     /* invokevirtualobject */
    "GET_ITBL",
    "FETCH",
    "NEW_OBJECT",

    "NULL_CHECK",

    "NEW_ARRAY_BOOLEAN",
    "NEW_ARRAY_CHAR",
    "NEW_ARRAY_FLOAT",
    "NEW_ARRAY_DOUBLE",
    "NEW_ARRAY_BYTE",
    "NEW_ARRAY_SHORT",
    "NEW_ARRAY_INT",
    "NEW_ARRAY_LONG",
    "CONVERT_I2B",
    "CONVERT_I2C",
    "CONVERT_I2S",
    "CONVERT_INTEGER",
    "CONVERT_LONG",
    "CONVERT_FLOAT",
    "CONVERT_DOUBLE",
    "NEG",
    "NOT",
    "INT2BIT",
    "RET",
    "RETURN_VALUE",
    "IDENTITY",
    "RESOLVE_CLASS",
    "RESOLVE_REFERENCE",
    "CLASS_CHECKINIT",
    "STATIC_FIELD_REF",
    "MONITOR_ENTER",
    "MONITOR_EXIT",
    "THROW",

    "ASSIGN",	/* CVMJITBinaryOp */
    "INDEX",
    "BOUNDS_CHECK",
    "INVOKE",
    "INTRINSIC",
    "CHECKCAST",
    "INSTANCEOF",
    "FETCH_MB_FROM_VTABLE",
/* IAI - 20 */
    "FETCH_VCB",
    "FETCH_MB_FROM_VTABLE_OUTOFLINE",
    "MB_TEST_OUTOFLINE",
/* IAI - 20 */   
    "FETCH_MB_FROM_INTERFACETABLE",
    "PARAMETER",
    "IARG",
    "NEW_ARRAY_REF",
    "MULTIANEW_ARRAY",
    "FIELD_REF",
    "SEQUENCE_R",
    "SEQUENCE_L",
    "ADD",
    "SUB",
    "MUL",
    "AND",
    "OR",
    "XOR",
    "DIV",
    "REM",
    "SHL",
    "SHR",
    "USHR",
    "LCMP",
    "DCMPL",
    "DCMPG",
    "FCMPL",
    "FCMPG",

    "GOTO",		/* CVMJITBranchOp */
    "JSR",

    "BCOND",		/* CVMJITConditionalBranch */
    "LOOKUPSWITCH",	/* CVMJITLookupSwitch */
    "TABLESWITCH",	/* CVMJITTableSwitch */
    "DEFINE",
    "USED",
    "LOAD_PHIS",
    "RELEASE_PHIS",
    "MAP_PC",
    
    "BEGIN_INLINING",
    "END_INLINING",
    "OUTOFLINE_INVOKE"
};

static const char *const typeTagMap[] = {
	"(NONE)",
	"(NODE)",
    	"(v)",
	"(int)",
	"(short)",
	"(char)",
	"(long)",
	"(byte)",
	"(float)",
	"(double)",
	"(boolean)",
	"(reference)",
	"(32-bit field)",
	"(64-bit field)",
	"(address field)",
};

static const char *const subnodeTagMap[] = {
    "CONSTANT_NODE",
    "NULL_NODE",
    "LOCAL_NODE",
    "PHI_LIST",
    "MAP_PC_NODE",
    "BRANCH_NODE",
    "PHI0_NODE",
    "UNARY_NODE",
    "PHI1_NODE",
    "LOOKUPSWITCH_NODE",
    "TABLESWITCH_NODE",
    "BINARY_NODE",
    "CONDBRANCH_NODE",
};

static const char *const conditions[] = {
    "LT",
    "LE",
    "EQ",
    "GE",
    "GT",
    "NE",
};

static void 
CVMJITirdumpConstantJavaNumeric32(CVMJavaVal32 j, CVMUint8 typeTag)
{
   switch (typeTag) {
   case CVM_TYPEID_INT:
	CVMconsolePrintf(" (%d)", j.i);
	break;
   case CVM_TYPEID_FLOAT:
	CVMconsolePrintf(" (%f)", j.f);
	break;
   case CVMJIT_TYPEID_32BITS:
	CVMconsolePrintf(" (JavaInt:%d  JavaFloat:%f)\t", j.i, j.f);
	break;
   default:
	CVMassert(CVM_FALSE);
   }
}

static void 
CVMJITirdumpConstant32(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
    CVMJITConstant32* const32 = CVMJITirnodeGetConstant32(node);
    switch (CVMJITgetOpcode(node) >> CVMJIT_SHIFT_OPCODE) {
    case CVMJIT_CONST_JAVA_NUMERIC32:
	CVMJITirdumpConstantJavaNumeric32(const32->j, CVMJITgetTypeTag(node));
	break; 
    case CVMJIT_CONST_METHODTABLE_INDEX:
	CVMconsolePrintf(" (mtIndex %d)", const32->mtIndex); 
	break;
    case CVMJIT_CONST_FIELD_OFFSET:
	CVMconsolePrintf(" (%d)", const32->fieldOffset);
	break;

    case CVMJIT_CONST_NEW_CB_UNRESOLVED:
    case CVMJIT_CONST_CB_UNRESOLVED:
    case CVMJIT_CONST_ARRAY_CB_UNRESOLVED:
    case CVMJIT_CONST_GETFIELD_FB_UNRESOLVED:
    case CVMJIT_CONST_PUTFIELD_FB_UNRESOLVED:
    case CVMJIT_CONST_GETSTATIC_FB_UNRESOLVED:
    case CVMJIT_CONST_PUTSTATIC_FB_UNRESOLVED:
    case CVMJIT_CONST_STATIC_MB_UNRESOLVED:
    case CVMJIT_CONST_SPECIAL_MB_UNRESOLVED:
    case CVMJIT_CONST_INTERFACE_MB_UNRESOLVED:
    case CVMJIT_CONST_METHOD_TABLE_INDEX_UNRESOLVED:
        CVMconsolePrintf(" (cpIndex %d)", const32->cpIndex);
        break;
    default:
	CVMconsolePrintf("undefined Constant32");
	CVMassert(CVM_FALSE);
	break;
    }
    
}

static void 
CVMJITirdumpConstantAddr(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
    CVMJITConstantAddr* constAddr = CVMJITirnodeGetConstantAddr(node);
    switch (CVMJITgetOpcode(node) >> CVMJIT_SHIFT_OPCODE) {
    case CVMJIT_CONST_STRING_ICELL:
        CVMconsolePrintf(" (stringICell 0x%x)\n",
			 constAddr->stringICell);
 	break;
    case CVMJIT_CONST_STRING_OBJECT:
        CVMconsolePrintf(" (stringObject 0x%x)\n",
			 constAddr->stringObject);
 	break;
#ifdef CVM_LVM
    case CVMJIT_CONST_STATIC_FB:
        CVMconsolePrintf(" (%F)", constAddr->fb);
        break;
#endif
    case CVMJIT_CONST_MC:
	CVMconsolePrintf(" (%C.%M)", 
			 CVMmbClassBlock(constAddr->mc->mb),
			 constAddr->mc->mb); 
	break;
    case CVMJIT_CONST_MB:
	CVMconsolePrintf(" (%C.%M)", 
			 CVMmbClassBlock(constAddr->mb),
			 constAddr->mb); 
	break;
    case CVMJIT_CONST_CB:
	CVMconsolePrintf(" (%C)", constAddr->cb); 
	break;
    case CVMJIT_CONST_STATIC_FIELD_ADDRESS:
        CVMconsolePrintf(" (0x%x)", constAddr->staticAddress);
	break;

    default:
	CVMconsolePrintf("undefined ConstantAddr");
	CVMassert(CVM_FALSE);
	break;
    }
    
}

static void
CVMJITirdumpConstant64(CVMJITCompilationContext* con, CVMJITIRNode* node)
{
    /*
      char trBuf[30];
    */
    if (CVMJITgetTypeTag(node) == CVM_TYPEID_LONG 
	|| CVMJITgetTypeTag(node) == CVMJIT_TYPEID_64BITS)
    {
	CVMJavaVal64 value;
	CVMJavaLong highLong;
	CVMUint32 high, low;
	CVMmemCopy64(value.v, CVMJITirnodeGetConstant64(node)->j.v);
	highLong = CVMlongShr(value.l, 32);
	high = CVMlong2Int(highLong);
	low = CVMlong2Int(value.l);
	CVMconsolePrintf(" (long = 0x%08x0x%08x)\n", high, low);
	/*
	  CVMconsolePrintf("	    (long = %s)\n",
	  (CVMlong2String(CVMJITirnodeGetConstant64(node)->j.l,
	  trBuf, trBuf+sizeof(trBuf)), trBuf));
	*/
    }
    if (CVMJITgetTypeTag(node) == CVM_TYPEID_DOUBLE
	|| CVMJITgetTypeTag(node) == CVMJIT_TYPEID_64BITS)
    {
	CVMJavaVal64 value;
	CVMmemCopy64(value.v, CVMJITirnodeGetConstant64(node)->j.v);
	CVMconsolePrintf(" (double = %g)\n", value.d);
	/*
	  CVMconsolePrintf("	    (double = %s)\n",
	  (CVMlong2String(CVMJITirnodeGetConstant64(node)->j.d,
	  trBuf, trBuf+sizeof(trBuf)), trBuf));
	*/
    }
}

static const char *const trapClass[] = {
    "Null pointer exception",	/* CVMJITIR_NULL_POINTER */
    "Array index out of bounds", /* CVMJITIR_ARRAY_INDEX_OOB */
    "Divide by zero", /* CVMJITIR_DIVIDE_BY_ZERO */
};

/* Purpose: Prints the specified number of indentations. */
void printIndent(const char *format, const char *prefix, int indentCount)
{
    int i;
    CVMconsolePrintf(format, prefix);
    for (i = 0; i < indentCount; i++) {
        CVMconsolePrintf("%s", "   ");
    }
}

static void
CVMJITirdumpIRNodeInternal(CVMJITCompilationContext* con, CVMJITIRNode* node,
                           int indentCount, const char *prefix,
                           CVMBool alwaysDump);

void 
CVMJITirdumpIRNodeAlways(CVMJITCompilationContext* con, CVMJITIRNode* node,
                         int indentCount, const char *prefix)
{
    CVMJITirdumpIRNodeInternal(con, node, indentCount, prefix, CVM_TRUE);
}

void 
CVMJITirdumpIRNode(CVMJITCompilationContext* con, CVMJITIRNode* node,
                   int indentCount, const char *prefix)
{
    CVMJITirdumpIRNodeInternal(con, node, indentCount, prefix, CVM_FALSE);
}

static void
CVMJITirdumpIRNodeInternal(CVMJITCompilationContext* con, CVMJITIRNode* node,
                           int indentCount, const char *prefix,
                           CVMBool alwaysDump)
{
    CVMJITIRBlock* bk = NULL;
    CVMJITIRNodeTag nodeTag;

    /* C stack redzone check */
    if (!CVMCstackCheckSize(con->ee, CVM_REDZONE_CVMJITirdumpIRNode,
        "CVMJITirdumpIRNode", CVM_FALSE)) {
        return;
    }

    /* The following assertions are done here because the arrays being asserted
       on are defined in this file.  The assertions need the size info of the
       arrays and this size info is only available in this file. */
#undef ARRAY_ELEMENTS
#define ARRAY_ELEMENTS(x) (sizeof(x)/sizeof(x[0]))
    /* Make sure we have as many opcodeTagMap strings as there are
       CVMJITIROpcodeTag: */
    CVMassert(ARRAY_ELEMENTS(opcodeTagMap) == CVMJIT_TOTAL_IR_OPCODE_TAGS);
    /* Make sure we have as many subnodeTagMap strings as there are
       CVMJITIRNodeTag: */
    CVMassert(ARRAY_ELEMENTS(subnodeTagMap) == CVMJIT_TOTAL_IR_NODE_TAGS);
#undef ARRAY_ELEMENTS

    /* Print the prefix and indentation first: */
    printIndent("%s", prefix, indentCount);

    /* Print the header: */
    CVMconsolePrintf("<(ID: %d) %s %s ", 
  	CVMJITirnodeGetID(node),
        opcodeTagMap[CVMJITgetOpcode(node) >> CVMJIT_SHIFT_OPCODE],
        typeTagMap[CVMJITgetTypeTag(node)]);

    nodeTag = CVMJITgetNodeTag(node) >> CVMJIT_SHIFT_NODETAG;
    if (nodeTag == CVMJIT_UNARY_NODE) {
	if (CVMJITirnodeUnaryNodeIs(node, VOLATILE_FIELD)){
	    CVMconsolePrintf("volatile ");
	}
    } else if (nodeTag == CVMJIT_BINARY_NODE) {
	if (CVMJITirnodeBinaryNodeIs(node, VOLATILE_FIELD)){
	    CVMconsolePrintf("volatile ");
	}
    }

#ifdef CVM_JIT_REGISTER_LOCALS
    switch (node->decorationType) {
        case CVMJIT_ASSIGN_DECORATION: {
	    CVMconsolePrintf(" (assignIdx: %d)",
			     node->decorationData.assignIdx);
	    break;
	}
        default: break;
    }
#endif
    /* Only dump the refCount if it is not obvious.  We're interested in 0 and
       negative refCounts as well because these could be (though not
       necessarily) signs of a bug if they occur: */
    if (CVMJITirnodeGetRefCount(node) != 1) {
        CVMconsolePrintf(" (ref count: %d)", CVMJITirnodeGetRefCount(node));
    }
    /* Dump identical nodes once. */
    if (!CVMJITirnodeIsDumpNeeded(node) && !CVMJITirnodeIsLocalNode(node) &&
        !alwaysDump) {
        return;
    }

    if (CVMJITirnodeIsConstant32Node(node)) {
	CVMJITirdumpConstant32(con, node);
    } else if (CVMJITirnodeIsConstantAddrNode(node)) {
	CVMJITirdumpConstantAddr(con, node);
    } else if (CVMJITirnodeIsConstant64Node(node)) {
	CVMJITirdumpConstant64(con, node);
    } else if (CVMJITirnodeIsLocalNode(node)) {
	CVMconsolePrintf("  %d>", CVMJITirnodeGetLocal(node)->localNo);
    } else if (CVMJITirnodeIsEndInlining(node)) {
	CVMconsolePrintf("  count:%d>", CVMJITirnodeGetNull(node)->data);
    } else if (CVMJITirnodeIsNullNode(node)) {
    } else if (CVMJITirnodeIsMapPcNode(node)) {
	CVMconsolePrintf("  %d>",
			 node->type_node.mapPcNode.javaPcToMap); 
    } else if (CVMJITirnodeIsBeginInlining(node)) {
	CVMJITUnaryOp* iNode = CVMJITirnodeGetUnaryOp(node);
	CVMconsolePrintf("\n");
        CVMJITirdumpIRNodeInternal(con, iNode->operand, indentCount+1, prefix,
                                   alwaysDump);
	CVMconsolePrintf("\n");
	CVMassert(CVMJITgetOpcode(iNode->operand) ==
		  CVMJIT_CONST_MC << CVMJIT_SHIFT_OPCODE);
    } else if (CVMJITirnodeIsBinaryNode(node)) {
	CVMJITBinaryOp *typeNode = CVMJITirnodeGetBinaryOp(node);
	CVMconsolePrintf("\n");
        CVMJITirdumpIRNodeInternal(con, typeNode->lhs, indentCount+1, prefix,
                                   alwaysDump);
	CVMconsolePrintf("\n");
        CVMJITirdumpIRNodeInternal(con, typeNode->rhs, indentCount+1, prefix,
                                   alwaysDump);
    } else if (CVMJITirnodeIsUnaryNode(node)) {
	CVMconsolePrintf("\n");
        CVMJITirdumpIRNodeInternal(con, CVMJITirnodeGetUnaryOp(node)->operand,
            indentCount+1, prefix, alwaysDump);
    } else if (CVMJITirnodeIsBranchNode(node)) {
	bk = (CVMJITirnodeGetBranchOp(node)->branchLabel);
        printIndent("\n%s", prefix, indentCount);
        CVMconsolePrintf("target: (BlockID: %d BlockPC: %d)", 
	    CVMJITirblockGetBlockID(bk), CVMJITirblockGetBlockPC(bk));

    } else if (CVMJITirnodeIsCondBranchNode(node)) {
	CVMJITConditionalBranch *typeNode = CVMJITirnodeGetCondBranchOp(node);
        bk = typeNode->target;
        printIndent("\n%s", prefix, indentCount);
        CVMconsolePrintf("target: (BlockID: %d BlockPC: %d)", 
	    CVMJITirblockGetBlockID(bk), CVMJITirblockGetBlockPC(bk)); 
        printIndent("\n%s", prefix, indentCount);
	CVMconsolePrintf("condition: %s", conditions[typeNode->condition]);
        printIndent("\n%s", prefix, indentCount);
	CVMconsolePrintf("flags: %d", typeNode->flags);
	CVMconsolePrintf("\n");
        CVMJITirdumpIRNodeInternal(con, typeNode->lhs, indentCount+1,
            prefix, alwaysDump); 
	CVMconsolePrintf("\n");
        CVMJITirdumpIRNodeInternal(con, typeNode->rhs, indentCount+1,
            prefix, alwaysDump); 
	CVMconsolePrintf("\n");
    } else if (CVMJITirnodeIsDefineNode(node)) {
	CVMJITDefineOp* typeNode = CVMJITirnodeGetDefineOp(node);
	CVMJITIRNode* defineOperandNode = typeNode->operand; 
	CVMJITIRNode* usedNode = CVMJITirnodeValueOf(typeNode->usedNode);
	CVMJITUsedOp* usedOp = CVMJITirnodeGetUsedOp(usedNode);
	CVMassert(defineOperandNode != NULL);
        printIndent("\n%s", prefix, indentCount);
	CVMconsolePrintf("spillLocation: %d", usedOp->spillLocation);
        printIndent("\n%s", prefix, indentCount);
	CVMconsolePrintf("registerSpillOk: %d", usedOp->registerSpillOk);
        printIndent("\n%s", prefix, indentCount);
	CVMconsolePrintf("operand: ");
        CVMJITirdumpIRNodeInternal(con, defineOperandNode, indentCount+1,
            prefix, alwaysDump);
        CVMconsolePrintf("\n");
    } else if (CVMJITirnodeIsUsedNode(node)) {
	CVMJITUsedOp* typeNode = CVMJITirnodeGetUsedOp(node);
        printIndent("\n%s", prefix, indentCount);
	CVMconsolePrintf("spillLocation: %d", typeNode->spillLocation);
        printIndent("\n%s", prefix, indentCount);
	CVMconsolePrintf("registerSpillOk: %d\n", typeNode->registerSpillOk);
    } else if (CVMJITnodeTagIs(node, PHI_LIST)) {
    } else if (CVMJITirnodeIsTableSwitchNode(node)) {
	int cnt;
        CVMJITTableSwitch* typeNode = CVMJITirnodeGetTableSwitchOp(node);
        bk = typeNode->defaultTarget;
        printIndent("\n%s", prefix, indentCount);
        CVMconsolePrintf("Key:\n");
        CVMJITirdumpIRNodeInternal(con, typeNode->key, indentCount+1, prefix,
            alwaysDump);
        printIndent("\n%s", prefix, indentCount);
	CVMassert(bk != NULL);
        CVMconsolePrintf("default target: (BlockID: %d BlockPC: %d)",
            CVMJITirblockGetBlockID(bk), CVMJITirblockGetBlockPC(bk));
        printIndent("\n%s", prefix, indentCount);
        CVMconsolePrintf("\tlow: %d", typeNode->low);
        printIndent("\n%s", prefix, indentCount);
        CVMconsolePrintf("\thigh: %d", typeNode->high);
        printIndent("\n%s", prefix, indentCount);
        CVMconsolePrintf("\tnElements: %d", typeNode->nElements);
        printIndent("\n%s", prefix, indentCount);
        CVMconsolePrintf("\ttableList blocks:");
	for (cnt = typeNode->low; cnt <= typeNode->high; cnt++) {
	    bk = typeNode->tableList[cnt-typeNode->low];
            printIndent("\n%s", prefix, indentCount+1);
            CVMconsolePrintf("jump offset %d: (BlockID: %d BlockPC: %d)",
                cnt, CVMJITirblockGetBlockID(bk), 
		CVMJITirblockGetBlockPC(bk));
	}
    } else if (CVMJITirnodeIsLookupSwitchNode(node)) {
        int cnt = 0;
        CVMJITLookupSwitch* typeNode = CVMJITirnodeGetLookupSwitchOp(node);
	CVMJITSwitchList* sl = typeNode->lookupList;
        bk = typeNode->defaultTarget;
        printIndent("\n%s", prefix, indentCount);
        CVMconsolePrintf("Key:\n");
        CVMJITirdumpIRNodeInternal(con, typeNode->key, indentCount+1, prefix,
            alwaysDump);
        printIndent("\n%s", prefix, indentCount);
        CVMassert(bk != NULL);
        CVMconsolePrintf("default target: (BlockID: %d BlockPC: %d)",
            CVMJITirblockGetBlockID(bk), CVMJITirblockGetBlockPC(bk));
        printIndent("\n%s", prefix, indentCount);
        CVMconsolePrintf("\tnElements: %d", typeNode->nPairs);
        printIndent("\n%s", prefix, indentCount);
        CVMconsolePrintf("\tlookupList blocks:");

        for (;cnt < typeNode->nPairs; cnt++) {
            int match = sl[cnt].matchValue;
            bk = sl[cnt].dest; 
            printIndent("\n%s", prefix, indentCount+1);
            CVMconsolePrintf("match %d: (BlockID: %d BlockPC: %d)",
            match, CVMJITirblockGetBlockID(bk), CVMJITirblockGetBlockPC(bk));
        }

    } else {
	CVMassert(CVM_FALSE); /* unknown or unexpected node type */
	return;
    }
    /* Set the dumpTag to CVM_FALSE */ 
    if (!alwaysDump) {
        CVMJITirnodeSetDumpTag(node, CVM_FALSE);
    }
}

void
CVMJITirdumpIRRootList(CVMJITCompilationContext* con, CVMJITIRBlock* bk)
{
    CVMJITIRRoot* rootNode =
	(CVMJITIRRoot*)CVMJITirlistGetHead(CVMJITirblockGetRootList(bk));
    CVMJITIRNode* expr = NULL;

    while (rootNode != NULL) {
        expr = CVMJITirnodeGetRootOperand(rootNode);
        CVMJITirdumpIRNode(con, expr, 0, "   ");
        CVMconsolePrintf("\n\n");
        rootNode = CVMJITirnodeGetRootNext(rootNode);
    }/* end of while loop */
}

void
CVMJITirdumpIRBlock(CVMJITCompilationContext* con, CVMJITIRBlock* bk)
{
    CVMJITIRList* blockRootList = CVMJITirblockGetRootList(bk);
    CVMconsolePrintf("\n************************"
		     "*************************\n");
    CVMconsolePrintf("*\n");
    CVMconsolePrintf("* (block ID:%d block PC:%d )\n", 
		     CVMJITirblockGetBlockID(bk), CVMJITirblockGetBlockPC(bk));
    CVMconsolePrintf("* IR root list contains %d root nodes: in block \n",
		     CVMJITirlistGetCnt(blockRootList));
    CVMconsolePrintf("* flags: 0x%x\n", bk->flags);
#ifdef CVM_JIT_REGISTER_LOCALS
    CVMconsolePrintf("* noMoreIncomingLocals: 0x%x\n",
		     bk->noMoreIncomingLocals);
    CVMconsolePrintf("* noMoreIncomingRefLocals: 0x%x\n",
		     bk->noMoreIncomingRefLocals);
    CVMconsolePrintf("* orderedIncomingLocals: 0x%x\n",
		     bk->orderedIncomingLocals);
#endif
    if (!CVMJITirblockIsArtificial(bk)) {
	CVMJITIRRange *r = bk->firstRange;
	while (r != NULL) {
	    CVMJITMethodContext* mc = r->mc;
	    CVMJavaMethodDescriptor* jmd = mc->jmd;
	    CVMUint8* code = CVMjmdCode(jmd);
	    char buf[256];
	    
	    /* Print the source file name and line number info: */
	    CVMpc2string(code + r->startPC, mc->mb, CVM_FALSE, CVM_FALSE,
			 buf, buf + sizeof(buf));
	    CVMconsolePrintf("* Source: %s\n", buf);
	    CVMconsolePrintf("* Compilation depth: %d locals : ",
			     mc->compilationDepth);
	    if (mc->firstLocal < mc->currLocalWords) {
		CVMconsolePrintf("%d .. %d\n",
				 mc->firstLocal, mc->currLocalWords - 1);
	    } else {
		CVMconsolePrintf("(none)\n");
	    }
	    
	    /* Disassemble the byte codes for the block: */
	    CVMconsolePrintf("* Byte-codes for block:\n");
#ifdef CVM_DEBUG
	    CVMbcDisassembleRange(mc->mb, code + r->startPC, 
				  code + r->endPC);
#endif
	    r = r->next;
	}
    } else {
	CVMconsolePrintf("* SPECIAL BLOCK FOR OUT OF LINE INVOKES\n");
    }
    
    CVMconsolePrintf("LOCALS MERGE INFO AT HEAD OF BLOCK: ");
    CVMJITlocalrefsDumpRefs(con, &bk->localRefs);
    CVMconsolePrintf("\n");
    
    CVMconsolePrintf("*************************************************\n");
    CVMconsolePrintf("\nNumber and size of USED nodes in phi array: %d %d\n\n",
		     bk->phiCount, bk->phiSize);
    
#ifdef CVM_JIT_REGISTER_LOCALS
    if (bk->originalIncomingLocalsCount > 0) {
	int i;
	CVMconsolePrintf("Original Incoming Locals(%d): \n",
			 bk->originalIncomingLocalsCount);
	for (i = 0; i < bk->originalIncomingLocalsCount; i++) {
	    if (bk->originalIncomingLocals[i] == NULL) {
		CVMconsolePrintf("\t<null>\n");
	    } else {
		CVMconsolePrintf("\tlocal(%d)\tID(%d)\n ",
		    CVMJITirnodeGetLocal(CVMJITirnodeValueOf(
		        bk->originalIncomingLocals[i]))->localNo,
		    CVMJITirnodeGetID(bk->originalIncomingLocals[i]));
	    }
	}
	CVMconsolePrintf("\n");
    }
    if (bk->incomingLocalsCount > 0) {
	int i;
	CVMconsolePrintf("Incoming Locals(%d): \n", bk->incomingLocalsCount);
	for (i = 0; i < bk->incomingLocalsCount; i++) {
	    if (bk->incomingLocals[i] == NULL) {
		CVMconsolePrintf("\t<null>\n");
	    } else {
		CVMconsolePrintf("\tlocal(%d)\tID(%d)\n ",
		    CVMJITirnodeGetLocal(CVMJITirnodeValueOf(
		        bk->incomingLocals[i]))->localNo,
		    CVMJITirnodeGetID(bk->incomingLocals[i]));
	    }
	}
	CVMconsolePrintf("\n");
    }
    if (bk->assignNodesCount > 0) {
	int i;
	CVMconsolePrintf("Assigned Locals(%d): \n", bk->assignNodesCount);
	for (i = 0; i < bk->assignNodesCount; i++) {
	    CVMJITIRNode* assignNode = bk->assignNodes[i];
	    CVMJITIRNode* localNode = CVMJITirnodeGetLeftSubtree(assignNode);
	    CVMUint16 localNo = CVMJITirnodeGetLocal(localNode)->localNo;
	    CVMconsolePrintf("\tASSIGN(%d): local(%d)\tID(%d)\trhsID(%d)\n ",
	        CVMJITirnodeGetID(bk->assignNodes[i]),
		localNo,
	        CVMJITirnodeGetID(localNode),
	        CVMJITirnodeGetID(CVMJITirnodeGetRightSubtree(assignNode)));
	}
	CVMconsolePrintf("\n");
    }
    if (bk->successorBlocksCount > 0) {
	int i;
	CVMconsolePrintf("Successor Blocks(%d): \n", bk->successorBlocksCount);
	for (i = 0; i < bk->successorBlocksCount; i++) {
	    CVMJITIRBlock* successorBk = bk->successorBlocks[i];
	    int j;
	    CVMconsolePrintf("\tID(%d) LocalOrder(%d) AssignOrder(%d)"
			     " RefsOK(%d) Locals(",
                             CVMJITirblockGetBlockID(successorBk),
			     bk->successorBlocksLocalOrder[i],
			     bk->successorBlocksAssignOrder[i],
			     bk->okToMergeSuccessorBlockRefs[i]);
	    for (j = 0; j < successorBk->incomingLocalsCount; j++) {
		if (successorBk->incomingLocals[j] == NULL) {
		    CVMconsolePrintf("<null>");
		} else {
		    CVMconsolePrintf("%d",
		         CVMJITirnodeGetLocal(CVMJITirnodeValueOf(
			     successorBk->incomingLocals[j]))->localNo);
		}
		if (j != successorBk->incomingLocalsCount - 1) {
		    CVMconsolePrintf(" ");
		}
	    }

	    CVMconsolePrintf(")\n");
	}
	CVMconsolePrintf("\n");
    }
#endif /* CVM_JIT_REGISTER_LOCALS */

    CVMJITirdumpIRRootList(con, bk);
}

void
CVMJITirdumpIRBlockList(CVMJITCompilationContext* con, CVMJITIRList* bkList)
{
    CVMJITIRBlock* rootbk = (CVMJITIRBlock*)CVMJITirlistGetHead(bkList);
    CVMJITIRBlock* bk =  NULL;

    CVMconsolePrintf("\n#################################################\n");
    CVMconsolePrintf("\nThe maximum node counts per root node are %d\n", 
		     con->saveRootCnt);
    CVMconsolePrintf("IR block list contains %d blocks in method\n", 
	CVMJITirlistGetCnt(bkList)); 
    CVMconsolePrintf("#################################################\n");

    for (bk = rootbk; bk != NULL; bk = CVMJITirblockGetNext(bk)) {
	CVMJITirdumpIRBlock(con, bk);
    }
    CVMconsolePrintf("\n");
}

void
CVMJITirdumpIRListPrint(CVMJITCompilationContext* con, CVMJITIRList* rlst)
{
    CVMJITirdumpIRBlockList(con, rlst); 
}

#endif
