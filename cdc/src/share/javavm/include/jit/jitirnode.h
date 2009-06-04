/*
 * @(#)jitirnode.h	1.125 06/10/10
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

#ifndef _INCLUDED_JITIRNODE_H
#define _INCLUDED_JITIRNODE_H

#include "javavm/include/typeid.h"
#include "javavm/include/objects.h"
#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitir.h"

/*
 * Here are the kinds of IR nodes, in hierarchical display:
 * 
 * IRNode
 *     Constant
 *         CLASS_CONSTANT
 *         METHOD_CONSTANT
 *         NULL_CONSTANT   (Implemented as a NULL String Constant)
 *         STRING_CONSTANT
 *         ICONST_32
 *         ICONST_64
 *     UnaryOp
 *         GOTO   (goto)
 *         JSR    (jsr)
 *         ARRAY_LENGTH   (array length)
 *         GET_VTBL       (object methodtable)
 *         RET            (jsr return)
 *         FETCH          (access array entry)
 *         Effect_RETURN_VALUE
 *         Effect_PUSH
 *         Effect_POP
 *     BinaryOp
 *         Assignment (store into memory, for locals or for heap)
 *             IASSIGN  (integer store)
 *             FASSIGN  (f.p. store)
 *             RASSIGN  (object reference store)
 *         Index (array indexing)
 *             IINDEX   (integer array)
 *             FINDEX   (float array)
 *             RINDEX   (object reference array)
 *         Array Index Out Of Bounds trap
 *             BOUNDS_CHECK
 *         Arithmetic Operations
 *             IADD
 *             FADD
 *         Comparison Operations
 *             ULT    (array bounds check, <)
 *         Method index
 *             METHOD_INDEX (vtable indexing)
 *         Parameter
 *             PARAMETER_NODE (method parameter)
 *             IARG (intrinsic method argument in register)
 *     Locals
 *         ILOCAL
 *         FLOCAL
 *         RLOCAL
 *     ControlOp
 *         BCOND  (conditional branch)
 *     PopValue
 *         POP_VALUE
 *     Invoke
 *         INVOKE
 *         INTRINSIC (invoke intrinsic method)
 *     NullParameter
 *         NULL_PARAMETER
 *         NULL_IARG  (intrinsic method argument terminator)
 *     Return (method return)
 *         RRETURN
 *         IRETURN
 *         FRETURN
 *         LRETURN
 *         DRETURN
 * 	   RETURN
 *     Throw
 *         THROW
 */

/*
 * For now, the ID's below conform to the prototype versions,
 * and as such jump around a lot.  This may change in the future.  
 */
typedef enum CVMJITIROpcodeTag {
    CVMJITOP_INVALID = 0,
    
    /****************************************************************
     * Constants - opcodes starting with CVMJIT_CONST are all opcodes
     * for CVMJITConstant32 or CVMJITConstant64 nodes.
     ****************************************************************/

    /* The follow are all genreted by ldc opcodes */
    CVMJIT_CONST_JAVA_NUMERIC32,  /* java Integer and Float  */
    CVMJIT_CONST_JAVA_NUMERIC64,  /* java Long and Double */
    CVMJIT_CONST_STRING_ICELL,  /* StringICell */
    CVMJIT_CONST_STRING_OBJECT, /* StringObject */

    CVMJIT_CONST_METHODTABLE_INDEX, /* used by GET_VTBL and GET_OBJECT_VTBL */

#ifdef CVM_LVM
    CVMJIT_CONST_STATIC_FB,  /* Resolved static field reference */
#endif
    CVMJIT_CONST_MC,  /* method context */
    CVMJIT_CONST_MB,  /* Resolved method reference */
    CVMJIT_CONST_CB,  /* Resolved class reference */

    CVMJIT_CONST_FIELD_OFFSET,         /* field offset for non-static access */
    CVMJIT_CONST_STATIC_FIELD_ADDRESS, /* field address for static access. */

    /* The following contain a cpIndex and are operands of RESOLVE_REFERENCE */
    CVMJIT_CONST_NEW_CB_UNRESOLVED,       /* unresolved class for new. */
    CVMJIT_CONST_CB_UNRESOLVED,           /* unresolved class for others. */
    CVMJIT_CONST_ARRAY_CB_UNRESOLVED,     /* unresolved class for anewarray. */
    CVMJIT_CONST_GETFIELD_FB_UNRESOLVED,  /* unresolved field for getfield. */
    CVMJIT_CONST_PUTFIELD_FB_UNRESOLVED,  /* unresolved field for putfield. */
    CVMJIT_CONST_GETSTATIC_FB_UNRESOLVED, /* unresolved static field for
                                             getstatic. */
    CVMJIT_CONST_PUTSTATIC_FB_UNRESOLVED, /* unresolved static field for
                                             putstatic. */
    CVMJIT_CONST_STATIC_MB_UNRESOLVED,    /* unresolved method for
                                             invokestatic. */
    CVMJIT_CONST_SPECIAL_MB_UNRESOLVED,   /* unresolved method for
                                             invokespecial. */
    CVMJIT_CONST_INTERFACE_MB_UNRESOLVED, /* unresolved method for
                                             invokeinterface. */
    CVMJIT_CONST_METHOD_TABLE_INDEX_UNRESOLVED, /* unresolved method index for
                                                   invokevirtual. */

    /* CVMJITNull */
    CVMJIT_NULL_PARAMETER,
    CVMJIT_NULL_IARG,
    CVMJIT_JSR_RETURN_ADDRESS,
    CVMJIT_EXCEPTION_OBJECT,
    CVMJIT_RETURN,		/* vreturn */	

    /* CVMJITLocal, reference to local variables */
    CVMJIT_LOCAL,		/* {i|f|a|l|d|l}load */


    /* CVMJITUnaryOp */
    CVMJIT_ARRAY_LENGTH,
    CVMJIT_GET_VTBL,		/* invokevirtual */
    CVMJIT_GET_OBJECT_VTBL,	/* invokevirtualobject */
    CVMJIT_GET_ITBL,		/* invokeinterface */
    CVMJIT_FETCH,
    CVMJIT_NEW_OBJECT,
    CVMJIT_NULL_CHECK,

    /* NOTE: It is important that the following NEW_ARRAY opcodes are sorted
       in the same order as the sequence of corresponding types in the
       CVMBasicType enum list (see basictypes.h). */
    CVMJIT_NEW_ARRAY_BOOLEAN,
    CVMJIT_NEW_ARRAY_CHAR,
    CVMJIT_NEW_ARRAY_FLOAT,
    CVMJIT_NEW_ARRAY_DOUBLE,
    CVMJIT_NEW_ARRAY_BYTE,
    CVMJIT_NEW_ARRAY_SHORT,
    CVMJIT_NEW_ARRAY_INT,
    CVMJIT_NEW_ARRAY_LONG,

    /* CVMJITUnaryOp, arithmethic and conversion ops */
    CVMJIT_CONVERT_I2B,       /* convert i2b. */
    CVMJIT_CONVERT_I2C,       /* convert i2c. */
    CVMJIT_CONVERT_I2S,       /* convert i2s. */
    CVMJIT_CONVERT_INTEGER,   /* convert i2{f|l|d} */
    CVMJIT_CONVERT_LONG,      /* convert long2{i|f|d} */
    CVMJIT_CONVERT_FLOAT,     /* convert float2{i|l|d} */
    CVMJIT_CONVERT_DOUBLE,    /* convert double2{i|f|l} */
    CVMJIT_NEG,		      /* {i|f|l|d}neg */
    CVMJIT_NOT,               /* (x == 0)?1:0. */
    CVMJIT_INT2BIT,           /* (x != 0)?1:0. */

    /* CVMJITUnaryOp */
    CVMJIT_RET,		      /* opc_ret */
    CVMJIT_RETURN_VALUE,      /* {a|i|f|l|d}return */

    /* An Identity operator, used to signal a value-producing CSE */
    CVMJIT_IDENTITY,

    CVMJIT_RESOLVE_CLASS,    /* unused */
    CVMJIT_RESOLVE_REFERENCE,
    CVMJIT_CLASS_CHECKINIT,

    CVMJIT_STATIC_FIELD_REF,
    CVMJIT_MONITOR_ENTER,
    CVMJIT_MONITOR_EXIT,
    CVMJIT_THROW,

    /* CVMJITBinaryOp */
    CVMJIT_ASSIGN,	/* {i|f|a|l|d|c|s|b}store */	
    CVMJIT_INDEX,	/* {i|f|a|l|d|c|s|b}astore|aload */
    CVMJIT_BOUNDS_CHECK,
    CVMJIT_INVOKE,
    CVMJIT_INTRINSIC,
    CVMJIT_CHECKCAST,
    CVMJIT_INSTANCEOF,
    CVMJIT_FETCH_MB_FROM_VTABLE,
/* IAI - 20 */
    CVMJIT_FETCH_VCB,	
    CVMJIT_FETCH_MB_FROM_VTABLE_OUTOFLINE,
    CVMJIT_MB_TEST_OUTOFLINE,
/* IAI - 20 */
    CVMJIT_FETCH_MB_FROM_INTERFACETABLE,
    CVMJIT_PARAMETER,
    CVMJIT_IARG,
    CVMJIT_NEW_ARRAY_REF,
    CVMJIT_MULTIANEW_ARRAY,
    CVMJIT_FIELD_REF,
    CVMJIT_SEQUENCE_R, /* SEQUENCE with <expr> on the right */
    CVMJIT_SEQUENCE_L, /* SEQUENCE with <expr> on the left */

    /* CVMJITBinaryOp, arithmetic ops */ 
    CVMJIT_ADD,	/* {i|f|l|d}ADD */
    CVMJIT_SUB,	/* {i|f|l|d}SUB */
    CVMJIT_MUL,	/* {i|f|l|d}MUL */
    CVMJIT_AND,	/* {i|l}AND */
    CVMJIT_OR,	/* {i|l}OR */
    CVMJIT_XOR,	/* {i|l}XOR */
    CVMJIT_DIV,	/* {i|f|l|d}DIV */
    CVMJIT_REM,	/* {i|f|l|d}REM */
    CVMJIT_SHL,	/* {i|l}SHL */
    CVMJIT_SHR,	/* {i|l}SHR */
    CVMJIT_USHR,/* {i|l}USHR */

    /* CVMJITBinaryOp, handle exceptional cases that javac normally does not have*/
    CVMJIT_LCMP,
    CVMJIT_DCMPL,
    CVMJIT_DCMPG,
    CVMJIT_FCMPL,
    CVMJIT_FCMPG,

    /* CVMJITBranchOp */
    CVMJIT_GOTO,
    CVMJIT_JSR,

    /* CVMJITConditionalBranch */
    CVMJIT_BCOND,

    /* CVMJITLookupSwitch */ 
    CVMJIT_LOOKUPSWITCH,

    /* CVMJITTableSwitch */ 
    CVMJIT_TABLESWITCH,

    CVMJIT_DEFINE,
    CVMJIT_USED,
    CVMJIT_LOAD_PHIS,
    CVMJIT_RELEASE_PHIS,

    /* An "instruction" to emit a Java -> native PC map */
    CVMJIT_MAP_PC,

    /*
     * Information on inlining
     */
    CVMJIT_BEGIN_INLINING,
    CVMJIT_END_INLINING,
    CVMJIT_OUTOFLINE_INVOKE,

    /* NOTE: CVMJIT_TOTAL_IR_OPCODE_TAGS must always be at end of this enum
       list.  It is used to ensure that the number of types of IROpcodeTags
       don't exceed the storage capacity alloted for it in the encoding of
       node tags. */
    CVMJIT_TOTAL_IR_OPCODE_TAGS

} CVMJITIROpcodeTag;

/*
 * IR subclass node tag
 */

typedef enum CVMJITIRNodeTag {
    /* these are sorted based on number of children */
    CVMJIT_CONSTANT_NODE,
    CVMJIT_NULL_NODE,
    CVMJIT_LOCAL_NODE,
    CVMJIT_PHI_LIST_NODE,     /* LOAD_PHIS, RELEASE_PHIS */
    CVMJIT_MAP_PC_NODE,
    CVMJIT_BRANCH_NODE,
    CVMJIT_PHI0_NODE,
    /* unary nodes start here */
    CVMJIT_UNARY_NODE,
    CVMJIT_PHI1_NODE,
    CVMJIT_LOOKUPSWITCH_NODE,
    CVMJIT_TABLESWITCH_NODE,
    /* binary nodes start here */
    CVMJIT_BINARY_NODE,
    CVMJIT_CONDBRANCH_NODE,

    /* NOTE: CVMJIT_TOTAL_IR_NODE_TAGS must always be at end of this enum list.
       It is used to ensure that the number of types of IROpcodeTags don't
       exceed the storage capacity alloted for it in the encoding of node tags.
    */
    CVMJIT_TOTAL_IR_NODE_TAGS

} CVMJITIRNodeTag;

typedef enum CVMJITCondition {
    CVMJIT_LT,	/* {i|f|d|l}lt */
    CVMJIT_LE,	/* {i|f|d|l}le */
    CVMJIT_EQ,	/* {i|f|d|l|a}eq */
    CVMJIT_GE,	/* {i|f|d|l}ge */
    CVMJIT_GT,	/* {i|f|d|l}gt */
    CVMJIT_NE	/* {i|f|d|l|a}ne */
} CVMJITCondition;

/* 
 * Some binaryop flags. 
 */
#define CVMJITBINOP_ALLOCATION                    0x01 /* is allocation */
#define CVMJITCMPOP_UNORDERED_LT 		  0x02
#define CVMJITBINOP_VOLATILE_FIELD                0x04
#define CVMJITBINOP_ARITHMETIC                    0x08 /* No side effects */

/*
 * Association with a read operation or a write operation or both
 * Required to identify what we intend to do with INDEX nodes.
 */
#define CVMJITBINOP_READ                          0x10 /* Used for reading */
#define CVMJITBINOP_WRITE                         0x20 /* Used for writing */
/* This is a virtual invoke converted to a non-virtual invoke */
#define CVMJITBINOP_VIRTUAL_INVOKE		  0x40

/*
 * in CVMJITUnaryOp node, save flag for checkinit of class needs
 * initialization and unresolvable class
 *
 * CVMJITUNOP_CLASSINIT flag is used by CVMJIT_UNRESOLVE_REFERENCE node
 *		        to determine if clinit is needed.
 * Both flags are used by CVMJIT_CLASSINIT node to determine
 *	      clinit for LVM or non-LVM.
 */
#define CVMJITUNOP_ALLOCATION                     0x01 /* is allocation */
#define CVMJITUNOP_VOLATILE_FIELD                 0x04
#define CVMJITUNOP_CLASSINIT			  0x08 
#define CVMJITUNOP_CLASSINIT_LVM		  0x10 
#define CVMJITUNOP_ARITHMETIC                     0x08 /* No side effects */

/* Node used by backend to generate javaPC -> compiledPC table */
typedef struct {
    CVMUint16 javaPcToMap;
} CVMJITMapPcNode;

/* WARNING: these must all be 32 bit types. That also means no
 * 8 or 16 bit types (because of endianess concerns). */
typedef union {
    CVMUint32         v32;          /* used by CVMJITirnodeNewConstant32 */
    CVMJavaVal32      j;            /* CVMJIT_CONST_JAVA_NUMERIC32 */
    CVMUint32         mtIndex;      /* CVMJIT_CONST_METHODTABLE_INDEX */
    CVMUint32         fieldOffset;  /* CVMJIT_CONST_FIELD_OFFSET */
    CVMUint32         cpIndex;      /* CVMJIT_XXX_UNRESOLVED */
} CVMJITConstant32;


typedef union {
    CVMAddr           vAddr;
    CVMStringICell*   stringICell;  /* CVMJIT_CONST_STRING_ICELL */
    CVMStringObject*  stringObject; /* CVMJIT_CONST_STRING_OBJECT */
#ifdef CVM_LVM
    CVMFieldBlock*    fb;           /* CVMJIT_CONST_STATIC_FB */
#endif
    CVMJITMethodContext* mc;	    /* CVMJIT_CONST_MC */
    CVMMethodBlock*   mb;           /* CVMJIT_CONST_MB */
    CVMClassBlock*    cb;           /* CVMJIT_CONST_CB  */
    CVMJavaVal32*     staticAddress;/* CVMJIT_CONST_STATIC_FIELD_ADDRESS */
} CVMJITConstantAddr;

/* CVMJIT_CONST_JAVA_NUMERIC64 */
typedef struct {
    struct {
	CVMAddr v[2];
    } j;
} CVMJITConstant64;

typedef struct {
    CVMUint16      localNo;
} CVMJITLocal;

/* NOTE: operand must be the first field in CVMJITUnaryOp to coincide with
         firstOperand in CVMJITGenericSubNode. */
typedef struct {
    CVMJITIRNode*  operand;
    CVMUint8       flags;  /* CVMJITUNOP_CLASSINIT_LVM|CVMJITUNOP_CLASSINIT|
			      CVMJITUNOP_ALLOCATION|CVMJITUNOP_ARITHMETIC */
} CVMJITUnaryOp;

#ifdef CVM_DEBUG_ASSERTS
typedef enum {
    CVMJIT_IDENTITY_DECORATION_RESOURCE = 0x1,
    CVMJIT_IDENTITY_DECORATION_SCALEDINDEX
} CVMJITIdentityDecorationType;

#define CVMJITidentityDecorationIs(con, expr, flag)    		\
    ((CVMJITidentityGetDecoration(con, expr)->decorationTag & 	\
     CVMJIT_IDENTITY_DECORATION_ ## flag) != 0)
#endif

typedef struct {
#ifdef CVM_DEBUG_ASSERTS
    CVMJITIdentityDecorationType  decorationTag;
#endif
    CVMInt32   refCount;
} CVMJITIdentityDecoration;

/*
 * First part MUST BE UnaryOp.
 */
typedef struct {
    CVMJITUnaryOp  uOp;
    CVMJITIdentityDecoration* identDec;
    CVMInt32 backendData;
} CVMJITIdentOp;

#define CVMJITidentDecoration(p)  (p)->type_node.identOp.identDec

#ifdef CVM_DEBUG_ASSERTS
#define CVMJITidentityInitDecoration(con, dec, decType)		\
    {								\
	(dec)->decorationTag = (decType);			\
	(dec)->refCount = 1;					\
    }
#else
#define CVMJITidentityInitDecoration(con, dec, decType)		\
    {								\
	(dec)->refCount = 1;					\
    }
#endif

#define CVMJITidentitySetDecorationRefCount(con, dec, refc)	\
    {								\
	(dec)->refCount = (refc);				\
    }

#define CVMJITidentityIncrementDecorationRefCount(con, dec)	\
    {								\
	(dec)->refCount++;             				\
    }

#define CVMJITidentityDecrementDecorationRefCount(con, dec)	\
    {								\
	(dec)->refCount--;             				\
    }

#define CVMJITidentityGetDecorationRefCount(con, dec)		\
	((dec)->refCount)             				\

/*
 * Decorate the current IDENTITY node with an IdentityDecoration
 */
extern void
CVMJITidentitySetDecoration(
    CVMJITCompilationContext* con,
    CVMJITIdentityDecoration* dec,
    CVMJITIRNode* expr);

/*
 * Get decoration of the current IDENTITY node
 */
extern CVMJITIdentityDecoration*
CVMJITidentityGetDecoration(
    CVMJITCompilationContext* con,
    CVMJITIRNode* expr);

/* NOTE: lhs must be the first field in CVMJITBinaryOp to coincide with
         firstOperand in CVMJITGenericSubNode. */
typedef struct {
    CVMJITIRNode*  lhs;
    CVMJITIRNode*  rhs;
    CVMUint16	   data;  /* e.g. true argsize of invoke node */
    CVMUint8       data2; /* more node specific data. */
    CVMUint8       flags; /* CVMJITCMPOP_UNORDERED_LT|
			     CVMJITBINOP_ALLOCATION */
} CVMJITBinaryOp;

typedef struct {
    CVMJITIRBlock*   branchLabel;
} CVMJITBranchOp;

/* NOTE: lhs and rhs must be the first two field in CVMJITConditionalBranch to
         coincide with the first two fields of in CVMJITBinaryOp. */
typedef struct {
    CVMJITIRNode*   lhs;
    CVMJITIRNode*   rhs;
    CVMJITCondition condition;
    CVMJITIRBlock*  target;
    CVMUint8        flags; /* CVMJITCMPOP_UNORDERED_LT */
} CVMJITConditionalBranch;

typedef enum {
    CVMJITIR_NULL_POINTER,
    CVMJITIR_ARRAY_INDEX_OOB,
    CVMJITIR_DIVIDE_BY_ZERO,
    CVMJITIR_NUM_TRAP_IDS
} CVMJITIRTrapID;

typedef struct { /* Nothing */
    CVMUint16      data;
} CVMJITNull;

/*
 * Switch List 
 */
typedef struct {
    CVMInt32       matchValue;
    CVMJITIRBlock* dest;
} CVMJITSwitchList;

/*
 * lookswitch
 */
/* NOTE: key must be the first field in CVMJITLookupSwitch to coincide with
         firstOperand in CVMJITGenericSubNode. */
typedef struct {
    CVMJITIRNode*    key;
    CVMJITIRBlock*   defaultTarget;
    CVMUint32        nPairs;	/* number of elements in array following */
    CVMJITSwitchList lookupList[1];
} CVMJITLookupSwitch;

/*
 * tableswitch
 */
/* NOTE: key must be the first field in CVMJITTableSwitch to coincide with
         firstOperand in CVMJITGenericSubNode. */
typedef struct {
    CVMJITIRNode*  key;
    CVMJITIRBlock* defaultTarget;
    CVMInt32 low;
    CVMInt32 high;
    CVMUint32 nElements; /* may be redundant as high-low+1 should be same.*/
    CVMJITIRBlock* tableList[1]; 
} CVMJITTableSwitch;

typedef struct {
    CVMInt16	   spillLocation;   /* location this value spills to */
    CVMUint8       registerSpillOk; /* true if ok to spill to a register */
    CVMRMResource* resource;	    /* RM resource into which it is stored */
} CVMJITUsedOp;

/* NOTE: operand must be the first field in CVMJITDefineOp to coincide with
         firstOperand in CVMJITGenericSubNode. */
typedef struct {
    CVMJITIRNode*  operand;       /* stack item this define node represents */
    CVMJITIRNode*  usedNode;      /* USED node for this define node */
    CVMRMResource* resource;	  /* RM resource into which it is stored */
} CVMJITDefineOp;

typedef struct {
    CVMJITIRBlock* targetBlock;
    CVMJITIRNode** defineNodeList; /* list of define nodes. */
} CVMJITPhisListOp;

typedef struct {
    CVMJITIRNode* firstOperand;
    CVMJITMethodContext* mc;	  /* method context. */
} CVMJITMethodContextOp;


/*
 * A generic sub node type that we can cast and fetch the lhs node.
 */
typedef struct {
    CVMJITIRNode* firstOperand;
} CVMJITGenericSubNode;

typedef union {
    CVMJITGenericSubNode     genericSubNode;
    CVMJITConstant32         constant32;
    CVMJITConstantAddr       constantAddr;
    CVMJITConstant64         constant64;
    CVMJITNull		     nullOp;
    CVMJITLocal              local;
    CVMJITUnaryOp            unOp;
    CVMJITIdentOp	     identOp;
    CVMJITBinaryOp           binOp;
    CVMJITBranchOp  	     branchOp;
    CVMJITConditionalBranch  condBranchOp;
    CVMJITLookupSwitch       lswitchOp;
    CVMJITTableSwitch        tswitchOp;
    CVMJITUsedOp	     usedOp;
    CVMJITDefineOp	     defineOp;
    CVMJITPhisListOp	     phisListOp;
    CVMJITMapPcNode    	     mapPcNode;
} CVMJITIRSubclassNode;

typedef enum {
    CVMJIT_NO_DECORATION,
    CVMJIT_REGHINT_DECORATION,
#ifdef CVM_JIT_REGISTER_LOCALS
    CVMJIT_ASSIGN_DECORATION,
    CVMJIT_LOCAL_DECORATION,
#endif
    CVMJIT_PARAMETER_DECORATION
#ifdef CVM_DEBUG
} CVMJITIRNodeDecorationType;
#else
} CVMJITIRNodeDecorationTypeValues;
typedef CVMUint8 CVMJITIRNodeDecorationType;
#endif

/*
 * Tag Encoding
 *
 * IR Node tags contain up to three pieces of information:
 * 1) the IR Node subclass of this node: CVMJITIRNodeTag
 * 2) the IR Node opcode of this node: CVMJITIROpcodeTag
 * 3) the type associated with this node: CVMJITIRTypeTag
 *
 * Encoding of the 16-bit tag field is as follows
 *
 * a) 8 high-order bits: opcode
 * b) 4 next bits: subclass
 * c) 4 next bits: type -- use the encoding from typeid.h rather
 *      than from basictypes.h -- it fits into 4 bits better.
 *
 * In addition to providing enough information, this
 * also gives us a means of type-safe extraction of subclass data.
 */

struct CVMJITIRNode {
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)
    CVMUint32  nodeID;  /* for IRdump purpose */
    CVMBool    dumpTag; /* Each IR node is dumped only once. */
#endif
    CVMUint16  tag;	/* encodes node type, opcode, and typeid */ 
    CVMUint16  refCount;
    CVMUint16  curRootCnt; /* number of nodes built in current node */
			   /* also used by code gen for state tag */
    CVMUint16  flags;      /* See CVMJITIRNodeFlags enum list below. */

    CVMJITRegsRequiredType regsRequired:8; /* codegen synthesized attribute */

    /*
     * Nodes can be decorated with some extra information so the backend can
     * do a better job of register allocation. For example, we like to pass
     * phi values in registers, but often the phi value is evaluated into
     * the wrong register before the backend sees its DEFINE node. The
     * solution is to have the creation of the DEFINE node store the stackIdx
     * in the value node so the backend knows which register to put it in.
     */

    CVMJITIRNodeDecorationType decorationType:8;

    union {
        /* regHint for expr, reg to be used as phi or outgoing local */
	CVMInt16  regHint;
	CVMUint16 argNo;       /* argument # for outgoing parameter values */
#ifdef CVM_JIT_REGISTER_LOCALS
        /* index into block's assignNodes[]. Used to get localNo that 
	   expression will be assigned to for better register targeting. */
	CVMUint16 assignIdx;
        /* index into block's successorBlocks[]. Used for outgoing locals
	 * so they are loaded into proper registers. */
	CVMUint16 successorBlocksIdx;
#endif
    } decorationData ;
    /* This field MUST BE LAST */
    CVMJITIRSubclassNode type_node;
};

/* The values in this enum list is used to set bits in the flags field of the
   CVMJITIRNode: */
enum CVMJITIRNodeFlags {
    CVMJITIRNODE_NULL_FLAGS                 = 0,
    CVMJITIRNODE_HAS_BEEN_EVALUATED         = (1 << 0), /* bit 0 */
    CVMJITIRNODE_IS_INITIAL_LOCAL           = (1 << 1), /* bit 1 */
    CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT  = (1 << 2), /* bit 2 */
    CVMJITIRNODE_THROWS_EXCEPTIONS          = (1 << 3), /* bit 3 */
    CVMJITIRNODE_PARENT_THROWS_EXCEPTIONS   = (1 << 4), /* bit 4 */
    CVMJITIRNODE_HAS_BEEN_EMITTED	    = (1 << 5)  /* bit 5 */
};

/* NOTE: A node is considered to have side effects if it has an undefined
   side effect, or if it throws exceptions.
   Hence, the side effect mask is the combination of these 2 flags.
*/
#define CVMJITIRNODE_SIDE_EFFECT_MASK \
    (CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT | CVMJITIRNODE_THROWS_EXCEPTIONS)

struct CVMJITIRRoot {
    CVMJITIRRoot* next;		/* see CVMJITIRListItem */
    CVMJITIRRoot* prev;		/* see CVMJITIRListItem */
    CVMJITIRNode* expr;
};

/*
 * CVMJITIRNode interface APIs
 */

extern CVMJITIRNode*
CVMJITirnodeNewMapPcNode(CVMJITCompilationContext* con,
			 CVMUint16 tag,
			 CVMUint16 pc, CVMJITIRBlock* curbk);

extern CVMJITIRRoot*
CVMJITirnodeNewRoot(CVMJITCompilationContext* con, 
		    CVMJITIRBlock* curbk, CVMJITIRNode* expr);

extern CVMJITIRNode*
CVMJITirnodeNewConstant32(CVMJITCompilationContext* con, 
			  CVMUint16 tag, CVMUint32 v32); 

extern CVMJITIRNode*
CVMJITirnodeNewConstantAddr(CVMJITCompilationContext* con, 
			  CVMUint16 tag, CVMAddr vAddr); 

extern CVMJITIRNode*
CVMJITirnodeNewConstant64(CVMJITCompilationContext* con, 
			  CVMUint16 tag, CVMJavaVal64* v64); 

extern CVMJITIRNode*
CVMJITirnodeNewResolveNode(CVMJITCompilationContext* con,
    CVMUint16 cpIndex, CVMJITIRBlock* curbk, 
    CVMUint8 checkInitFlag, CVMUint8 opcodeTag);

extern CVMJITIRNode*
CVMJITirnodeNewNull(CVMJITCompilationContext* con, CVMUint16 tag);

extern CVMJITIRNode*
CVMJITirnodeNewLocal(CVMJITCompilationContext* con, CVMUint16 tag, 
    CVMUint16 localNo); 

extern CVMJITIRNode*
CVMJITirnodeNewUnaryOp(CVMJITCompilationContext* con, CVMUint16 tag, 
    CVMJITIRNode* operand);

extern CVMJITIRNode*
CVMJITirnodeNewBinaryOp(CVMJITCompilationContext* con, CVMUint16 tag, 
    CVMJITIRNode* lhs, CVMJITIRNode* rhs);

extern CVMJITIRNode*
CVMJITirnodeNewBranchOp(CVMJITCompilationContext* con, CVMUint16 tag, 
    CVMJITIRBlock* target); 

extern CVMJITIRNode*
CVMJITirnodeNewCondBranchOp(CVMJITCompilationContext* con,
			    CVMJITIRNode* lhs, CVMJITIRNode* rhs,
			    CVMUint16 typeTag, CVMJITCondition condition,
			    CVMJITIRBlock* target, int flags);

extern CVMJITIRNode*
CVMJITirnodeNewBoundsCheckOp(CVMJITCompilationContext *con,
			     CVMJITIRNode *indexNode,
			     CVMJITIRNode *lengthNode);

extern CVMJITIRNode*
CVMJITirnodeNewLookupSwitchOp(CVMJITCompilationContext* con, 
			      CVMJITIRBlock* defaultTarget, 
			      CVMJITIRNode* key, CVMUint32 nPairs);

extern CVMJITIRNode*
CVMJITirnodeNewTableSwitchOp(CVMJITCompilationContext* con, 
			     CVMInt32 low, CVMInt32 high, CVMJITIRNode* key, 
			     CVMJITIRBlock* defaultTarget);

CVMJITIRNode*
CVMJITirnodeNewUsedOp(CVMJITCompilationContext* con, CVMUint16 tag,
		      CVMInt16 spillLocation,
		      CVMBool registerSpillOk);

CVMJITIRNode*
CVMJITirnodeNewDefineOp(CVMJITCompilationContext* con, CVMUint16 tag,
			CVMJITIRNode* operand, CVMJITIRNode* usedNode);

CVMJITIRNode*
CVMJITirnodeNewPhisListOp(CVMJITCompilationContext* con, CVMUint16 tag,
			  CVMJITIRBlock* targetBlock,
			  CVMJITIRNode** defineNodeList);

extern CVMJITIRNode*
CVMJITirnodeNewArray(CVMJITCompilationContext* con, CVMBasicType atype, 
    CVMJITIRNode* count);

extern CVMJITIRNode*
CVMJITirnodeNewMultiArray(CVMJITCompilationContext* con, 
    CVMUint16 cpEntry, CVMUint16 dimension);

#ifdef CVM_DEBUG_ASSERTS
extern void 
CVMJITirnodeAssertIsGenericSubNode(CVMJITIRNode* node);
#else
#define CVMJITirnodeAssertIsGenericSubNode(node) ((void)0)
#endif

/*
 * CVMJITIRNode inforamtion access macros 
 */

#define CVMJIT_IRNODE_SIZE    \
    (sizeof(CVMJITIRNode) - sizeof(CVMJITIRSubclassNode))

#define CVMJITirnodeGetTypeNode(node)	(&((node)->type_node))

/* root node operation API */
#define CVMJITirnodeGetRootNext(node)    ((node)->next)

#define CVMJITirnodeGetRootPrev(node)    ((node)->prev)

#define CVMJITirnodeGetRootOperand(node) ((node)->expr)

/* IR node operation API */ 
#define CVMJITirnodeGetConstant32(node)   \
    (CVMassert(CVMJITirnodeIsConstant32Node(node)), \
     &((node)->type_node.constant32))

#define CVMJITirnodeGetConstantAddr(node)   \
    (CVMassert(CVMJITirnodeIsConstantAddrNode(node)), \
     &((node)->type_node.constantAddr))

#define CVMJITirnodeGetConstant64(node)   \
    (CVMassert(CVMJITirnodeIsConstant64Node(node)), \
     &((node)->type_node.constant64))

#define CVMJITirnodeGetNull(node) 	\
    (CVMassert(CVMJITirnodeIsNullNode(node)), \
     &((node)->type_node.nullOp))

#define CVMJITirnodeGetLocal(node) 	\
    (CVMassert(CVMJITirnodeIsLocalNode(node)), \
     &((node)->type_node.local))

#define CVMJITirnodeGetIdentOp(node)   \
    (CVMassert(CVMJITirnodeIsIdentity(node)), \
     &((node)->type_node.identOp))

#define CVMJITirnodeGetUnaryOp(node)   \
    (CVMassert(CVMJITirnodeIsUnaryNode(node)), \
     &((node)->type_node.unOp))

#define CVMJITirnodeGetBinaryOp(node) 	\
    (CVMassert(CVMJITirnodeIsBinaryNode(node)), \
     &((node)->type_node.binOp))

#define CVMJITirnodeGetBranchOp(node) 	\
    (CVMassert(CVMJITirnodeIsBranchNode(node)), \
     &((node)->type_node.branchOp))

#define CVMJITirnodeGetCondBranchOp(node) 	\
    (CVMassert(CVMJITirnodeIsCondBranchNode(node)),\
     &((node)->type_node.condBranchOp))

#define CVMJITirnodeGetLookupSwitchOp(node) \
    (CVMassert(CVMJITirnodeIsLookupSwitchNode(node)), \
     &((node)->type_node.lswitchOp))

#define CVMJITirnodeGetTableSwitchOp(node) 	\
    (CVMassert(CVMJITirnodeIsTableSwitchNode(node)), \
     &((node)->type_node.tswitchOp))

#define CVMJITirnodeGetUsedOp(node) 	\
    (CVMassert(CVMJITirnodeIsUsedNode(node)), \
     &((node)->type_node.usedOp))

#define CVMJITirnodeGetDefineOp(node) 	\
    (CVMassert(CVMJITirnodeIsDefineNode(node)), \
     &((node)->type_node.defineOp))

#define CVMJITirnodeGetPhisListOp(node) 	\
    (CVMassert(CVMJITnodeTagIs(node, PHI_LIST)), \
     &((node)->type_node.phisListOp))

#define CVMJITirnodeGetRightSubtree(node)		\
    (CVMassert(CVMJITirnodeIsBinaryNode(node) ||	\
	       CVMJITirnodeIsCondBranchNode(node)),	\
     ((CVMJITBinaryOp*)(&(node)->type_node))->rhs)

/*
 * Get the first operand of IR node.
 * Cast type_node of IR node to a generic type type so that we could
 * fetch the lhs node. The lhs node of the qualified sub nodes is
 * always the first field in the sub node.
 * A new node type called CVMJITGenericSubNode containing firstOperand 
 * (a pointer to CVMJITIRNode) which represents the lhs IR node is added 
 * into CVMJITIRSubclassNode.
 * getLeftSubtree can now simply cast a node to
 * CVMJITGenericSubNode and access the firstSubNode field.
 * Assert on the passed node which has legal node type.
 */
#define CVMJITirnodeGetLeftSubtree(node) \
    (CVMJITirnodeAssertIsGenericSubNode(node), \
     ((CVMJITGenericSubNode*)(&(node)->type_node))->firstOperand)

/* Purpose: Sets the lhs operand with the new operand. */
extern void
CVMJITirnodeSetLeftSubtree(CVMJITCompilationContext *con,
                           CVMJITIRNode *node, CVMJITIRNode *operand);

/* Purpose: Sets the rhs operand with the new operand. */
extern void
CVMJITirnodeSetRightSubtree(CVMJITCompilationContext *con,
                            CVMJITIRNode *node, CVMJITIRNode *operand);

/* Purpose: Delete the specified binary node and adjust all the refCount of
            its operands accordingly. */
extern void
CVMJITirnodeDeleteBinaryOp(CVMJITCompilationContext *con, CVMJITIRNode *node);

/* Purpose: Delete the specified unary node and adjust all the refCount of
            its operands accordingly. */
extern void
CVMJITirnodeDeleteUnaryOp(CVMJITCompilationContext *con, CVMJITIRNode *node);

/*
 * CVMJITIRNode access the encoded type tag macro APIs
 */

#define CVMJIT_SHIFT_OPCODE 8
#define CVMJIT_SHIFT_NODETAG 4

#define CVMJIT_OPCODE_MASK  (0xff << CVMJIT_SHIFT_OPCODE)
#define CVMJIT_NODETAG_MASK (0xf << CVMJIT_SHIFT_NODETAG)
#define CVMJIT_TYPE_MASK (0xf)

#define CVMJITgetNodeTag(node) ((node)->tag & CVMJIT_NODETAG_MASK)
#define CVMJITgetTypeTag(node) ((node)->tag & CVMJIT_TYPE_MASK)
#define CVMJITgetOpcode(node)  ((node)->tag & CVMJIT_OPCODE_MASK)

#define CVMJIT_TYPE_ENCODE(opcode_tag, node_tag, type_tag) \
    (((int)(opcode_tag) << CVMJIT_SHIFT_OPCODE) | \
      ((int)(node_tag) << CVMJIT_SHIFT_NODETAG) | \
      (type_tag)) 

#define CVMJIT_ENCODE_ILOCAL \
    CVMJIT_ENCODE_LOCAL(CVM_TYPEID_INT)

#define CVMJIT_ENCODE_ALOCAL \
    CVMJIT_ENCODE_LOCAL(CVM_TYPEID_OBJ)

#define CVMJIT_ENCODE_GOTO \
    CVMJIT_TYPE_ENCODE(CVMJIT_GOTO, CVMJIT_BRANCH_NODE, 0)

#define CVMJIT_ENCODE_NULL_CHECK \
    CVMJIT_TYPE_ENCODE(CVMJIT_NULL_CHECK, CVMJIT_UNARY_NODE, CVM_TYPEID_OBJ)

#define CVMJITnodeTagIs(node, nodetype)				\
    (CVMJITgetNodeTag((node)) == 				\
     (CVMJIT_##nodetype##_NODE << CVMJIT_SHIFT_NODETAG))
#define CVMJITopcodeTagIs(node, opcodetype)		\
    (CVMJITgetOpcode(node) ==				\
     (CVMJIT_##opcodetype << CVMJIT_SHIFT_OPCODE))

/*
 * Information on inlining.
 */
#define CVMJIT_ENCODE_BEGIN_INLINING		\
    CVMJIT_TYPE_ENCODE(CVMJIT_BEGIN_INLINING, 	\
		       CVMJIT_UNARY_NODE, 	\
		       CVM_TYPEID_NONE)		\

#define CVMJIT_ENCODE_END_INLINING_LEAF		\
    CVMJIT_TYPE_ENCODE(CVMJIT_END_INLINING, 	\
		       CVMJIT_NULL_NODE, 	\
		       CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_OUTOFLINE_INVOKE			\
    CVMJIT_TYPE_ENCODE(CVMJIT_OUTOFLINE_INVOKE, 	\
		       CVMJIT_UNARY_NODE, 		\
		       CVM_TYPEID_NONE)


#define CVMJIT_ENCODE_SEQUENCE_R(type) \
    CVMJIT_TYPE_ENCODE(CVMJIT_SEQUENCE_R, CVMJIT_BINARY_NODE, (type))
#define CVMJIT_ENCODE_SEQUENCE_L(type) \
    CVMJIT_TYPE_ENCODE(CVMJIT_SEQUENCE_L, CVMJIT_BINARY_NODE, (type))

#define CVMJIT_ENCODE_IBINARY(opcodeTag) \
    CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_BINARY_NODE, CVM_TYPEID_INT)
#define CVMJIT_ENCODE_LBINARY(opcodeTag) \
    CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_BINARY_NODE, CVM_TYPEID_LONG)

#define CVMJIT_ENCODE_OBINARY(opcodeTag) \
    CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_BINARY_NODE, CVM_TYPEID_OBJ)

#define CVMJIT_ENCODE_IUNARY(opcodeTag) \
    CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_UNARY_NODE, CVM_TYPEID_INT)

#define CVMJIT_ENCODE_BRANCH(opcodeTag) \
    CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_BRANCH_NODE, CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_FIELD_REF(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_FIELD_REF, CVMJIT_BINARY_NODE, typeTag)

#define CVMJIT_ENCODE_STATIC_FIELD_REF(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_STATIC_FIELD_REF, CVMJIT_UNARY_NODE, typeTag)

#define CVMJIT_ENCODE_LOCAL(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_LOCAL, CVMJIT_LOCAL_NODE, typeTag)

#define CVMJIT_ENCODE_INDEX \
    CVMJIT_TYPE_ENCODE(CVMJIT_INDEX, CVMJIT_BINARY_NODE, CVM_TYPEID_INT)

#define CVMJIT_ENCODE_ASSIGN(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_ASSIGN, CVMJIT_BINARY_NODE, typeTag)

#define CVMJIT_ENCODE_PARAMETER(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_PARAMETER, CVMJIT_BINARY_NODE, typeTag)

#define CVMJIT_ENCODE_IARG(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_IARG, CVMJIT_BINARY_NODE, typeTag)

#define CVMJIT_ENCODE_FETCH(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_FETCH, CVMJIT_UNARY_NODE, typeTag)

#define CVMJIT_ENCODE_USED(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_USED, CVMJIT_PHI0_NODE, typeTag)

#define CVMJIT_ENCODE_DEFINE(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_DEFINE, CVMJIT_PHI1_NODE, typeTag)

#define CVMJIT_ENCODE_LOAD_PHIS \
    CVMJIT_TYPE_ENCODE(CVMJIT_LOAD_PHIS, \
	CVMJIT_PHI_LIST_NODE, CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_RELEASE_PHIS \
    CVMJIT_TYPE_ENCODE(CVMJIT_RELEASE_PHIS, \
	CVMJIT_PHI_LIST_NODE, CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_INVOKE(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_INVOKE, CVMJIT_BINARY_NODE, typeTag)

#define CVMJIT_ENCODE_INTRINSIC(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_INTRINSIC, CVMJIT_BINARY_NODE, typeTag)

#define CVMJIT_ENCODE_NEW_ARRAY_BASIC(typeTag) \
    CVMJIT_TYPE_ENCODE( \
        ((typeTag) - CVM_T_BOOLEAN + CVMJIT_NEW_ARRAY_BOOLEAN), \
        CVMJIT_UNARY_NODE, CVM_TYPEID_OBJ)

#define CVMJIT_ENCODE_RETURN \
    CVMJIT_TYPE_ENCODE(CVMJIT_RETURN, CVMJIT_NULL_NODE, CVM_TYPEID_VOID)

#define CVMJIT_ENCODE_RETURN_VALUE(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_RETURN_VALUE, CVMJIT_UNARY_NODE, typeTag)

#define CVMJIT_ENCODE_IRETURN  CVMJIT_ENCODE_RETURN_VALUE(CVM_TYPEID_INT)
#define CVMJIT_ENCODE_FRETURN  CVMJIT_ENCODE_RETURN_VALUE(CVM_TYPEID_FLOAT)
#define CVMJIT_ENCODE_LRETURN  CVMJIT_ENCODE_RETURN_VALUE(CVM_TYPEID_LONG)
#define CVMJIT_ENCODE_DRETURN  CVMJIT_ENCODE_RETURN_VALUE(CVM_TYPEID_DOUBLE)

#define CVMJIT_ENCODE_ORETURN  CVMJIT_ENCODE_RETURN_VALUE(CVM_TYPEID_OBJ)
#define CVMJIT_ENCODE_ARETURN  \
    CVMJIT_ENCODE_RETURN_VALUE(CVMJIT_TYPEID_ADDRESS)
# define CVMJIT_ENCODE_JSR_RETURN_ADDRESS				\
    CVMJIT_TYPE_ENCODE(CVMJIT_JSR_RETURN_ADDRESS, CVMJIT_NULL_NODE,	\
                       CVMJIT_TYPEID_ADDRESS)

#define CVMJIT_ENCODE_RET \
    CVMJIT_TYPE_ENCODE(CVMJIT_RET, CVMJIT_UNARY_NODE, CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_EXCEPTION_OBJECT					\
    CVMJIT_TYPE_ENCODE(CVMJIT_EXCEPTION_OBJECT, CVMJIT_NULL_NODE,	\
		       CVM_TYPEID_OBJ)

#define CVMJIT_ENCODE_IDENTITY(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_IDENTITY, CVMJIT_UNARY_NODE, typeTag)

#define CVMJIT_ENCODE_BCOND(typeTag) \
    CVMJIT_TYPE_ENCODE(CVMJIT_BCOND, CVMJIT_CONDBRANCH_NODE, typeTag)

#define CVMJIT_ENCODE_BOUNDS_CHECK				\
    CVMJIT_TYPE_ENCODE(CVMJIT_BOUNDS_CHECK, CVMJIT_BINARY_NODE,	\
		       CVM_TYPEID_INT)

#define CVMJIT_ENCODE_NEW_OBJECT \
    CVMJIT_TYPE_ENCODE(CVMJIT_NEW_OBJECT, CVMJIT_UNARY_NODE, CVM_TYPEID_OBJ)

#define CVMJIT_ENCODE_NEW_ARRAY_REF \
    CVMJIT_ENCODE_OBINARY(CVMJIT_NEW_ARRAY_REF)

#define CVMJIT_ENCODE_MULTIANEW_ARRAY \
    CVMJIT_ENCODE_OBINARY(CVMJIT_MULTIANEW_ARRAY)

#define CVMJIT_ENCODE_ARRAY_LENGTH \
    CVMJIT_TYPE_ENCODE(CVMJIT_ARRAY_LENGTH, CVMJIT_UNARY_NODE, CVM_TYPEID_INT)

#define CVMJIT_ENCODE_CLASS_CHECKINIT					\
    CVMJIT_TYPE_ENCODE(CVMJIT_CLASS_CHECKINIT, CVMJIT_BINARY_NODE,      \
		       CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_NULL_PARAMETER				\
    CVMJIT_TYPE_ENCODE(CVMJIT_NULL_PARAMETER, CVMJIT_NULL_NODE,	\
		       CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_NULL_IARG \
    CVMJIT_TYPE_ENCODE(CVMJIT_NULL_IARG, CVMJIT_NULL_NODE, CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_MAP_PC \
    CVMJIT_TYPE_ENCODE(CVMJIT_MAP_PC, CVMJIT_MAP_PC_NODE, CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_TABLESWITCH					\
    CVMJIT_TYPE_ENCODE(CVMJIT_TABLESWITCH, CVMJIT_TABLESWITCH_NODE,	\
		       CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_LOOKUPSWITCH					\
    CVMJIT_TYPE_ENCODE(CVMJIT_LOOKUPSWITCH, CVMJIT_LOOKUPSWITCH_NODE,	\
		       CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_ATHROW \
    CVMJIT_TYPE_ENCODE(CVMJIT_THROW, CVMJIT_UNARY_NODE, CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_MONITOR_ENTER				\
    CVMJIT_TYPE_ENCODE(CVMJIT_MONITOR_ENTER, CVMJIT_UNARY_NODE,	\
		       CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_MONITOR_EXIT				\
    CVMJIT_TYPE_ENCODE(CVMJIT_MONITOR_EXIT, CVMJIT_UNARY_NODE,	\
		       CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_GET_VTBL \
    CVMJIT_TYPE_ENCODE(CVMJIT_GET_VTBL, CVMJIT_UNARY_NODE, CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_FETCH_MB_FROM_VTABLE			\
    CVMJIT_TYPE_ENCODE(CVMJIT_FETCH_MB_FROM_VTABLE,		\
		       CVMJIT_BINARY_NODE, CVMJIT_TYPEID_ADDRESS)
/* IAI - 20 */
#define CVMJIT_ENCODE_FETCH_VCB					\
    CVMJIT_TYPE_ENCODE(CVMJIT_FETCH_VCB,			\
		       CVMJIT_UNARY_NODE, CVMJIT_TYPEID_ADDRESS)
#define CVMJIT_ENCODE_FETCH_MB_FROM_VTABLE_OUTOFLINE			\
    CVMJIT_TYPE_ENCODE(CVMJIT_FETCH_MB_FROM_VTABLE_OUTOFLINE,		\
		       CVMJIT_BINARY_NODE, CVMJIT_TYPEID_ADDRESS)
#define CVMJIT_ENCODE_MB_TEST_OUTOFLINE					\
    CVMJIT_TYPE_ENCODE(CVMJIT_MB_TEST_OUTOFLINE, CVMJIT_BINARY_NODE,	\
    			CVMJIT_TYPEID_ADDRESS)
/* IAI - 20 */

#define CVMJIT_ENCODE_FETCH_MB_FROM_INTERFACETABLE		\
    CVMJIT_TYPE_ENCODE(CVMJIT_FETCH_MB_FROM_INTERFACETABLE,	\
		       CVMJIT_BINARY_NODE, CVMJIT_TYPEID_ADDRESS)

#define CVMJIT_ENCODE_GET_ITBL \
    CVMJIT_TYPE_ENCODE(CVMJIT_GET_ITBL, CVMJIT_UNARY_NODE, CVM_TYPEID_NONE)

#define CVMJIT_ENCODE_CHECKCAST \
    CVMJIT_TYPE_ENCODE(CVMJIT_CHECKCAST, CVMJIT_BINARY_NODE, CVM_TYPEID_OBJ)

#define CVMJIT_ENCODE_INSTANCEOF \
    CVMJIT_TYPE_ENCODE(CVMJIT_INSTANCEOF, CVMJIT_BINARY_NODE, CVM_TYPEID_INT)

#define CVMJIT_ENCODE_RESOLVE_REFERENCE					\
    CVMJIT_TYPE_ENCODE(CVMJIT_RESOLVE_REFERENCE, CVMJIT_UNARY_NODE,	\
		       CVM_TYPEID_NONE)

/*
 * Check IR node tag macros
 */

#define CVMJITirnodeIsConstant32Node(node)	\
    (CVMJITnodeTagIs(node, CONSTANT) && 	\
     !CVMJITirnodeIsConstantAddrNode(node) && \
     !CVMJITirnodeIsConstant64Node(node))

#define CVMJITirnodeIsConstantAddrNode(node)	      \
    (CVMJITnodeTagIs(node, CONSTANT) && 	      \
        (CVMJITirnodeIsReferenceType(node) ||         \
        CVMJITirnodeIsAddressType(node)))

#define CVMJITirnodeIsConstant64Node(node) \
    (CVMJITopcodeTagIs(node, CONST_JAVA_NUMERIC64))
#define CVMJITirnodeIsNullNode(node) \
    (CVMJITnodeTagIs(node, NULL))
#define CVMJITirnodeIsLocalNode(node) \
    (CVMJITnodeTagIs(node, LOCAL))
#define CVMJITirnodeIsUnaryNode(node) \
    (CVMJITnodeTagIs(node, UNARY))
#define CVMJITirnodeIsBinaryNode(node) \
    (CVMJITnodeTagIs(node, BINARY))
#define CVMJITirnodeIsBranchNode(node) \
    (CVMJITnodeTagIs(node, BRANCH))
#define CVMJITirnodeIsCondBranchNode(node) \
    (CVMJITnodeTagIs(node, CONDBRANCH))
#define CVMJITirnodeIsBoundsCheckNode(node) \
    (CVMJITnodeTagIs(node, BINARY) && CVMJITopcodeTagIs(node, BOUNDS_CHECK))

#define CVMJITirnodeIsLookupSwitchNode(node) \
    (CVMJITnodeTagIs(node, LOOKUPSWITCH))
#define CVMJITirnodeIsTableSwitchNode(node) \
    (CVMJITnodeTagIs(node, TABLESWITCH))
#define CVMJITirnodeIsDefineNode(node) \
    (CVMJITopcodeTagIs(node, DEFINE))
#define CVMJITirnodeIsUsedNode(node) \
    (CVMJITopcodeTagIs(node, USED))
#define CVMJITirnodeIsLoadPhisNode(node) \
    (CVMJITopcodeTagIs(node, LOAD_PHIS))
#define CVMJITirnodeIsReleasePhisNode(node) \
    (CVMJITopcodeTagIs(node, RELEASE_PHIS))
#define CVMJITirnodeIsMapPcNode(node) \
    (CVMJITnodeTagIs(node, MAP_PC))
#define CVMJITirnodeIsRetNode(node) \
    (CVMJITopcodeTagIs(node, RET))
#define CVMJITirnodeIsJsrReturnAddressNode(node)\
    (CVMJITopcodeTagIs(node, JSR_RETURN_ADDRESS))

/*
 * Check IR opcode tag macros
 */
#define CVMJITirnodeIsBeginInlining(node) \
    (CVMJITopcodeTagIs(node, BEGIN_INLINING))

#define CVMJITirnodeIsEndInlining(node) \
    (CVMJITopcodeTagIs(node, END_INLINING))

# define CVMJITirnodeNeedsEvaluation(node) \
    (!CVMJITirnodeIsConstant32Node(node) && \
     !CVMJITirnodeIsConstantAddrNode(node) && \
     !CVMJITirnodeIsConstant64Node(node) && \
     !CVMJITirnodeIsUsedNode(node))

#define CVMJITirnodeIsAssign(node) \
    (CVMJITopcodeTagIs(node, ASSIGN))

#define CVMJITirnodeIsFetch(node) \
    (CVMJITopcodeTagIs(node, FETCH))

#define CVMJITirnodeIsIndex(node) \
    (CVMJITopcodeTagIs(node, INDEX))

#define CVMJITirnodeIsParameter(node) \
    (CVMJITopcodeTagIs(node, PARAMETER))

#define CVMJITirnodeIsConstMC(node) \
    (CVMJITopcodeTagIs(node, CONST_MC))

#define CVMJITirnodeIsConstMB(node) \
    (CVMJITopcodeTagIs(node, CONST_MB))

#define CVMJITirnodeIsIdentity(node) \
    (CVMJITopcodeTagIs(node, IDENTITY))

#define CVMJITirnodeValueOf(node) \
    (CVMJITirnodeIsIdentity(node) ? CVMJITirnodeGetLeftSubtree(node) : node)

/* CVMJITIRNodeFlags macros: */
#define CVMJITirnodeHasBeenEvaluated(node) \
    (((node)->flags & CVMJITIRNODE_HAS_BEEN_EVALUATED) != 0)
#define CVMJITirnodeSetHasBeenEvaluated(node) \
    ((node)->flags |= CVMJITIRNODE_HAS_BEEN_EVALUATED)

#define CVMJITirnodeHasBeenEmitted(node) \
    (((node)->flags & CVMJITIRNODE_HAS_BEEN_EMITTED) != 0)
#define CVMJITirnodeSetHasBeenEmitted(node) \
    ((node)->flags |= CVMJITIRNODE_HAS_BEEN_EMITTED)

#define CVMJITirnodeIsInitialLocal(node) \
    (((node)->flags & CVMJITIRNODE_IS_INITIAL_LOCAL) != 0)
#define CVMJITirnodeSetIsInitialLocal(node) \
    ((node)->flags |= CVMJITIRNODE_IS_INITIAL_LOCAL)

#define CVMJITirnodeHasUndefinedSideEffect(node) \
    (((node)->flags & CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT) != 0)
#define CVMJITirnodeSetHasUndefinedSideEffect(node) \
    ((node)->flags |= CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT)

#define CVMJITirnodeThrowsExceptions(node) \
    (((node)->flags & CVMJITIRNODE_THROWS_EXCEPTIONS) != 0)
#define CVMJITirnodeSetThrowsExceptions(node) \
    ((node)->flags |= CVMJITIRNODE_THROWS_EXCEPTIONS)

#define CVMJITirnodeHasSideEffects(node) \
    (((node)->flags & CVMJITIRNODE_SIDE_EFFECT_MASK) != 0)

#define CVMJITirnodeSetParentThrowsExceptions(node) \
    ((node)->flags |= CVMJITIRNODE_PARENT_THROWS_EXCEPTIONS)
#define CVMJITirnodeResolveParentThrowsExceptions(parent, child) {      \
    /* If child says CVMJITIRNODE_PARENT_THROWS_EXCEPTIONS: */          \
    if (((child)->flags & CVMJITIRNODE_PARENT_THROWS_EXCEPTIONS) != 0) { \
        /* Indicate that the parent throws an exception: */             \
        CVMJITirnodeSetThrowsExceptions(parent);                        \
        /* Clear the flag now that the parent has been flagged          \
           accordingly: */                                              \
        (child)->flags &= ~CVMJITIRNODE_PARENT_THROWS_EXCEPTIONS;       \
    }                                                                   \
}

/* IR top node access macros */
#define CVMJITirnodeSetTag(node, tag_)       ((node)->tag = tag_)
#define CVMJITirnodeSetRefCount(node, cnt)   ((node)->refCount = cnt)
#define CVMJITirnodeSetcurRootCnt(node, cnt) ((node)->curRootCnt = cnt)
#define CVMJITirnodeGetTag(node)	     ((node)->tag)

#define CVMJITirnodeGetRefCountPrivate(node) ((node)->refCount)
#define CVMJITirnodeGetRefCount(node) \
    (CVMJITirnodeGetRefCountPrivate(node))

#define CVMJITirnodeGetcurRootCnt(node)	     ((node)->curRootCnt)
#define CVMJITirnodeAddcurRootCnt(node)      ((node)->curRootCnt++)

#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)
#define CVMJITirnodeSetDumpTag(node, tag)    ((node)->dumpTag = tag)
#define CVMJITirnodeSetID(con, node) {\
    (con)->nodeID++;			\
    (node)->nodeID = (con)->nodeID;	\
}
#define CVMJITirnodeGetID(node)	     	     ((node)->nodeID)
#define CVMJITirnodeGetDumpTag(node)	     ((node)->dumpTag)
#define CVMJITirnodeIsDumpNeeded(node)	     ((node)->dumpTag)
#else
#define CVMJITirnodeGetID(node)	     	     (0)
#endif

/* Purpose: Sets the unary node's flag to the specified value. */
#define CVMJITirnodeSetUnaryNodeFlag(node, value) \
    (CVMJITirnodeGetUnaryOp(node)->flags |= value)

/* Purpose: Gets the unary node's flag. */
#define CVMJITirnodeGetUnaryNodeFlag(node) \
    (CVMJITirnodeGetUnaryOp(node)->flags)

#define CVMJITirnodeUnaryNodeIs(node, flagbit) \
    ((CVMJITirnodeGetUnaryNodeFlag(node) & (CVMJITUNOP_##flagbit)) != 0)

/* Purpose: Sets the binary node's flag to the specified value. */
#define CVMJITirnodeSetBinaryNodeFlag(node, value) \
    (CVMJITirnodeGetBinaryOp(node)->flags |= value)

/* Purpose: Gets the binary node's flag. */
#define CVMJITirnodeGetBinaryNodeFlag(node) \
    (CVMJITirnodeGetBinaryOp(node)->flags)

#define CVMJITirnodeBinaryNodeIs(node, flagbit) \
    ((CVMJITirnodeGetBinaryNodeFlag(node) & (CVMJITBINOP_##flagbit)) != 0)

/* 
 * Check IR type tag macros
 */

#define CVMJITirnodeIsReferenceType(node) \
    (CVMJITgetTypeTag(node) == CVM_TYPEID_OBJ)

#define CVMJITirnodeIsAddressType(node) \
    (CVMJITgetTypeTag(node) == CVMJIT_TYPEID_ADDRESS)

#define CVMJITirnodeIsSingleWordType(node) \
    (CVMJITirIsSingleWordTypeTag(CVMJITgetTypeTag(node)))
#define CVMJITirnodeIsDoubleWordType(node) \
    (CVMJITirIsDoubleWordTypeTag(CVMJITgetTypeTag(node)))

/*
 * When compiling in a method which itself is non-static
 * (i.e. is a virtual method or an <init>) and which contains no instances
 * of astore_0 or its synonyms, then all null checks on local variable #0
 * can be eliminated. The null check flag is set globally in con by the
 * pre-pass. No null check on the result of a new-object, new-array, or
 * any other object-allocation operations.
 */
#define CVMJITirnodeIsNewOperation(node)				\
    ((CVMJITirnodeIsUnaryNode(node) && 					\
      CVMJITirnodeUnaryNodeIs(node, ALLOCATION)) ||			\
     (CVMJITirnodeIsBinaryNode(node) &&					\
      CVMJITirnodeBinaryNodeIs(node, ALLOCATION)))

/* 
 * Misc node construction macro defines 
 */

#define CVMJITirnodeAddRefCount(node) \
    ((node)->refCount++)

#define CVMJITirnodeDeleteRefCount(node) \
    (CVMassert(node->refCount != 0), (node)->refCount--)

#define CVMJIT_ENCODE_CONSTANTADDR(opcodeTag)			\
    CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_CONSTANT_NODE, CVMJIT_TYPEID_ADDRESS)
#define CVMJIT_ENCODE_CONSTANT32(opcodeTag)			\
    CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_CONSTANT_NODE, CVM_TYPEID_NONE)
#define CVMJIT_ENCODE_CONST_JAVA_NUMERIC32(typeid)			  \
    CVMJIT_TYPE_ENCODE(CVMJIT_CONST_JAVA_NUMERIC32, CVMJIT_CONSTANT_NODE, \
		       typeid)
#define CVMJIT_ENCODE_CONST_JAVA_NUMERIC64(typeid)			  \
    CVMJIT_TYPE_ENCODE(CVMJIT_CONST_JAVA_NUMERIC64, CVMJIT_CONSTANT_NODE, \
		       typeid)
#define CVMJIT_ENCODE_CONST_STRING_ICELL				\
    CVMJIT_TYPE_ENCODE(CVMJIT_CONST_STRING_ICELL, CVMJIT_CONSTANT_NODE, \
		       CVM_TYPEID_OBJ)
#define CVMJIT_ENCODE_CONST_STRING_OBJECT				 \
    CVMJIT_TYPE_ENCODE(CVMJIT_CONST_STRING_OBJECT, CVMJIT_CONSTANT_NODE, \
		       CVM_TYPEID_OBJ)
#define CVMJIT_ENCODE_CONST_METHODTABLE_INDEX			\
    CVMJIT_ENCODE_CONSTANT32(CVMJIT_CONST_METHODTABLE_INDEX)

#ifdef CVM_LVM
#  define CVMJIT_ENCODE_CONST_STATIC_FB				\
    CVMJIT_ENCODE_CONSTANTADDR(CVMJIT_CONST_STATIC_FB)
#endif
#define CVMJIT_ENCODE_CONST_MC					\
    CVMJIT_ENCODE_CONSTANTADDR(CVMJIT_CONST_MC)
#define CVMJIT_ENCODE_CONST_MB					\
    CVMJIT_ENCODE_CONSTANTADDR(CVMJIT_CONST_MB)
#define CVMJIT_ENCODE_CONST_CB					\
    CVMJIT_ENCODE_CONSTANTADDR(CVMJIT_CONST_CB)

#define CVMJIT_ENCODE_CONST_FIELD_OFFSET		       	\
    CVMJIT_ENCODE_CONSTANT32(CVMJIT_CONST_FIELD_OFFSET)
# define CVMJIT_ENCODE_CONST_STATIC_FIELD_ADDRESS		\
    CVMJIT_ENCODE_CONSTANTADDR(CVMJIT_CONST_STATIC_FIELD_ADDRESS)
#define CVMJIT_ENCODE_CONST_CPINDEX(opcodeTag)			\
    CVMJIT_TYPE_ENCODE(opcodeTag, CVMJIT_CONSTANT_NODE, CVM_TYPEID_NONE)

#define CVMJITirnodeNewConstantJavaNumeric64(con, v64, typeid)		   \
     CVMJITirnodeNewConstant64(con,					   \
			       CVMJIT_ENCODE_CONST_JAVA_NUMERIC64(typeid), \
			       v64)

#define CVMJITirnodeNewConstantJavaNumeric32(con, i, typeid)		   \
     CVMJITirnodeNewConstant32(con,					   \
			       CVMJIT_ENCODE_CONST_JAVA_NUMERIC32(typeid), \
			       (CVMUint32)i)

#define CVMJITirnodeNewConstantStringICell(con, str)			\
     CVMJITirnodeNewConstantAddr(con, CVMJIT_ENCODE_CONST_STRING_ICELL, \
			       (CVMAddr)str)

#define CVMJITirnodeNewConstantStringObject(con, str)			 \
     CVMJITirnodeNewConstantAddr(con, CVMJIT_ENCODE_CONST_STRING_OBJECT, \
			       (CVMAddr)str)

#define CVMJITirnodeNewConstantMethodTableIndex(con, mtIndex)		   \
     CVMJITirnodeNewConstant32(con, CVMJIT_ENCODE_CONST_METHODTABLE_INDEX, \
			       (CVMUint32)mtIndex)

# ifdef CVM_LVM
#  define CVMJITirnodeNewConstantStaticFB(con, fb)			\
     CVMJITirnodeNewConstantAddr(con, CVMJIT_ENCODE_CONST_STATIC_FB,	\
			       (CVMAddr)fb)
# endif

#define CVMJITirnodeNewConstantMC(con, mc)			\
     CVMJITirnodeNewConstantAddr(con, CVMJIT_ENCODE_CONST_MC,	\
			        (CVMAddr)mc)

#define CVMJITirnodeNewConstantMB(con, mb)			\
     CVMJITirnodeNewConstantAddr(con, CVMJIT_ENCODE_CONST_MB,	\
			       (CVMAddr)mb)

#define CVMJITirnodeNewConstantCB(con, cb)			\
     CVMJITirnodeNewConstantAddr(con, CVMJIT_ENCODE_CONST_CB,	\
			       (CVMAddr)cb)

#define CVMJITirnodeNewConstantFieldOffset(con, fieldOndex)		\
    CVMJITirnodeNewConstant32(con, CVMJIT_ENCODE_CONST_FIELD_OFFSET,	\
			      (CVMUint32)fieldOndex)

#define CVMJITirnodeNewConstantStaticFieldAddress(con, address)		\
    CVMJITirnodeNewConstantAddr(con,                                    \
        CVMJIT_ENCODE_CONST_STATIC_FIELD_ADDRESS,                       \
	(CVMAddr)address)

#define CVMJITirnodeNewConstantCpIndex(con, cpIndex, opcodeTag)		   \
    CVMJITirnodeNewConstant32(con, CVMJIT_ENCODE_CONST_CPINDEX(opcodeTag), \
			      (CVMUint32)cpIndex)

/*
 * CVMJIT_IARG, CVMJIT_NULL_IARG, and CVMJIT_INTRINSIC macros: 
 */
#define CVMJIT_IARG_WORD_INDEX_MASK             0xff
#define CVMJIT_IARG_ARG_NUMBER_MASK             0xff
#define CVMJIT_IARG_WORD_INDEX_SHIFT            0
#define CVMJIT_IARG_ARG_NUMBER_SHIFT            8

#define CVMJIT_IARG_WORD_INDEX(iargNode) \
    ((CVMJITirnodeGetBinaryOp(iargNode)->data >> \
      CVMJIT_IARG_WORD_INDEX_SHIFT) & CVMJIT_IARG_WORD_INDEX_MASK)
#define CVMJIT_IARG_ARG_NUMBER(iargNode) \
    ((CVMJITirnodeGetBinaryOp(iargNode)->data >> \
      CVMJIT_IARG_ARG_NUMBER_SHIFT) & CVMJIT_IARG_ARG_NUMBER_MASK)
#define CVMJIT_IARG_INTRINSIC_ID(iargNode) \
    (CVMJITirnodeGetBinaryOp(iargNode)->data2)

#define CVMJIT_NULL_IARG_INTRINSIC_ID(nullIArgNode) \
    (CVMJITirnodeGetNull(nullIArgNode)->data)

#define CVMJIT_INTRINSIC_ID(intrinsicNode) \
    (CVMJITirnodeGetBinaryOp(intrinsicNode)->data)
#define CVMJIT_INTRINSIC_NUMBER_OF_ARGS(intrinsicNode) \
    (CVMJITirnodeGetBinaryOp(intrinsicNode)->data2)

/* 
 * CVMJITIRNode operand stack operations
 */

#define CVMJITirnodeStackPush(con, node) \
    CVMJITstackPush(con, (void *)node, ((con)->operandStack))

#define CVMJITirnodeStackPop(con) \
    (CVMJITIRNode*)CVMJITstackPop(con, ((con)->operandStack))

#define CVMJITirnodeStackDiscardTop(con) \
    CVMJITstackDiscardTop(con, ((con)->operandStack))

#define CVMJITirnodeStackGetTop(con) \
    (CVMJITIRNode*)CVMJITstackGetTop(con, ((con)->operandStack))

#endif /* _INCLUDED_JITIRNODE_H */
