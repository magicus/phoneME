/*
 * @(#)jitcontext.h	1.195 06/10/10
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

#ifndef _INCLUDED_JIT_IMPL_H
#define _INCLUDED_JIT_IMPL_H

#include "javavm/include/defs.h"
#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitset.h"
#include "javavm/include/jit/jitirblock.h"
#include "javavm/include/jit/jitirlist.h"
#include "javavm/include/jit/jitutils.h"
#include "javavm/include/jit/jitinlineinfo.h"
#include "javavm/include/jit/jitrmcontext.h"
#include "javavm/include/jit/jitcodesched.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/porting/ansi/setjmp.h"

typedef CVMJITIRBlock** CVMJITBlockMap;

/*
 * Inlineability isn't just a binary any more: now we want isInlinable
 * to pass back more of the information it gathers while inspecting the
 * candidate.
 */

typedef enum {
    InlineStoresLocals = 0x1,		/* stores to locals */
    InlineStoresParameters = 0x2,	/* stores to parameters */
    InlineCallsOut = 0x4,		/* performs method call */
    InlineHasBranches = 0x8,		/* contains branches */
    InlineNotInlinable = 0x100		/* forget it */
} CVMJITInlinePreScanInfo;

/*
 * The compilation of the current method.
 * We can have multiple of these due to method inlining.
 */
struct CVMJITMethodContext {
    CVMJITMethodContext*     caller;
    CVMMethodBlock*          mb;      /* The method being compiled */
    CVMJavaMethodDescriptor* jmd;     /* The Java part of the method */
    CVMClassBlock*           cb;      /* Class of method */ 
    CVMConstantPool* 	     cp;      /* Constant pool */
    CVMJITIRNode**           locals;  /* Outgoing args from previous ctxt */
    CVMJITIRNode**           physLocals; /* Records the last written value. */
    CVMUint32		     localsSize; /* size of locals[] */
    CVMUint32*               localsState; /* The local state */
    CVMJITIRBlock*	     translateBlock;
    CVMInt32		     firstLocal; /* re-map locals of inlined method */
    CVMInt32		     nOwnLocals; /* # of locals needed to inline */
    CVMUint8*                code;    /* Start of method code */
    CVMUint8*                codeEnd; /* End of method code */
    CVMUint8*                startPC; /* Translation range start */
    CVMUint8*                endPC;   /* Translation range end */
    CVMUint16                invokePC; /* Invoke site in caller */
    CVMUint16		     syncObject; /* local containing sync obj */
    CVMUint32                startStackIndex; /* stack index at the start of
                                                 the method not including its
                                                 args. */
    CVMBool                  hasExceptionHandlers;
    CVMBool		     isVirtualInline;
    CVMJITIRBlock*           currBlock;
    CVMJITIRBlock*           retBlock;

    /* An index information of all blocks created in a method */
    CVMJITIRList	     bkList;

    CVMJITInlinePreScanInfo  preScanInfo;

    CVMUint32                currLocalWords;	/* current number in use    */

    /* This capacity keeps track of inlining depth */
    CVMUint32                currCapacity;

    CVMInt32		     compilationDepth;

    /* A mapping from PC to CVMJITIRBlock* */
    CVMJITBlockMap           pcToBlock;
    CVMJITSet		     labelsSet; /* bit-set for labels */
    CVMJITSet                gcPointsSet; /* bit-set for GC points */

    /* bit-set for marking bytecode pc's that need to be mapped to their
     * compiled pc equivalents.
     */
    CVMJITSet		     mapPcSet;

    CVMJITSet		     notSeq;

    /* 
     * If this boolean is true, it is safe to remove all null checks
     * of locals[0] because we know that them method is not static
     * and no one has stored a new reference into locals[0].
     */
    CVMBool                  removeNullChecksOfLocal_0;

    /*
     * Used to abort the translation of a block when a conditional
     * branch is converted into a goto.
     */
    CVMBool		     abortTranslation;
};

/*
 * A compilation context, passed around between various phases of the
 * compiler 
 */
struct CVMJITCompilationContext {
    CVMExecEnv*              ee;
    CVMJITReturnValue        resultCode;

    CVMMethodBlock*          mb;
    CVMJavaMethodDescriptor* jmd;
    CVMUint32                maxLocalWords;	/* max allowed this compile */
    CVMUint32                numberLocalWords;	/* highest number needed yet*/

    CVMJITSet		     curRefLocals;

    /* This is the maximum reached */
    CVMUint32                capacity;
    /* And this is the maximum allowed */
    CVMUint32                maxCapacity;

    /* This code length keeps track of inlining */
    CVMUint32                codeLength;
    
    /*
     * A stack of method contexts, used for inlining.
     *
     * NOTE: maxInliningDepth is used to indicate the highest level of inlining
     * that was actually done so far in the current compilation attempt. 
     * maxAllowedInliningDepth is used to indicate the cap on how high the
     * maxInliningDepth can rise.  
     *
     * NOTE: maxAllowedInliningDepth is a value that can be reduced after each
     * compilation attempt if the compilation failed because of a shortage of
     * memory.  This allows the compilation to be retried with less inlining
     * (which takes up memory) being attempted.
     *
     * NOTE: there is one exceptional case where maxInliningDepth is allowed
     * to exceed maxAllowedInliningDepth by one.  Normally,
     * maxInliningDepth must be less or equal to maxAllowedInliningDepth.
     */
    CVMUint32                numInliningInfoEntries;
    CVMInt32                 maxInliningDepth;
    CVMInt32                 maxAllowedInliningDepth;
    CVMJITMethodContext*     mc; /* top of stack */
    CVMJITMethodContext*     rootMc; /* bottom of stack */
    
    /*
     * Another inlining tracking stack, this time for the back-end to use.
     * We need to keep track of the topmost inlining info entry here.
     */
    CVMInt32                     inliningDepth;
    CVMJITInliningInfoStackEntry *inliningStack;

    /* 
     * And here's the inlining information node to be emitted for methods
     * that have performed inlining
     */
    CVMUint32                    inliningInfoIdx;
    CVMCompiledInliningInfo*     inliningInfo;
    
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)
    CVMUint32		     nodeID;

    CVMJITIRNode *           errNode; /* Indigestable node. */
    const char *             errorMessage; /* To be set by CVMJITerror(). */
#endif

#ifdef CVM_TRACE_JIT
    const char *             symbolName;  /* Name of symbol to be added. */
    CVMCodegenComment *      comments;    /* List of codegen comments. */
    CVMCodegenComment *      lastComment; /* Last node (for appending). */
#endif

#ifdef CVM_DEBUG_ASSERTS
    /* The following is used in the assertion check to ensure that operands on
       the operand stack has been fully evaluated before a root node is
       inserted: */
    CVMBool                  operandStackIsBeingEvaluated;

    /* The following is used to assert that we do not overflow any of the
       allocated blocks using CVMJITmemNew(): */
    void *                   memoryFenceBlocks;
#endif

    /* 
     * The following mechanism is for eliminating redundant null checks
     */
#define JIT_MAX_CHECK_NODES 80
    CVMJITIRNode* nullCheckNodes[JIT_MAX_CHECK_NODES];
    CVMUint32 nextNullCheckID;
    CVMUint32 maxNullCheckID;

    /*
     * ... and for eliminating redundant array bounds checks 
     */
    CVMJITIRNode* boundsCheckArrays[JIT_MAX_CHECK_NODES];
    CVMJITIRNode* boundsCheckIndices[JIT_MAX_CHECK_NODES];
    CVMJITIRNode* indexExprs[JIT_MAX_CHECK_NODES];
    CVMJITIRNode* arrayFetchExprs[JIT_MAX_CHECK_NODES];
    CVMUint32 nextBoundsCheckID;
    CVMUint32 maxBoundsCheckID;

    /*
     * ... and for eliminating redundant GETFIELD's
     */
    CVMJITIRNode* getfield[JIT_MAX_CHECK_NODES];
    CVMJITIRNode* getfieldObjrefs[JIT_MAX_CHECK_NODES];
    CVMUint16     getfieldOffsets[JIT_MAX_CHECK_NODES];
    CVMUint32     nextGetfieldID;
    CVMUint32     maxGetfieldID;

    /*
     * ... and for eliminating redundant ARRAYLENGTH's
     */
#define JIT_MAX_ARRAY_LENGTH_CSE_NODES 40
    CVMJITIRNode* arrayLength[JIT_MAX_ARRAY_LENGTH_CSE_NODES];
    CVMJITIRNode* arrayLengthObjrefs[JIT_MAX_ARRAY_LENGTH_CSE_NODES];
    CVMUint32     nextArrayLengthID;
    CVMUint32     maxArrayLengthID;

    /* A mapping from block ID (PC) to CVMJITIRBlock* */
    CVMJITBlockMap           idToBlock;

    /* A counter for block ID assignment */
    CVMUint32                blockIDCounter;
    
    CVMUint16		     mapPcNodeCount;

    CVMUint16		     saveRootCnt; /* maximum count per root node */

    /* List of all Java blocks created in a method */
    CVMJITIRList	     bkList;
    /* List of all artificial blocks created in a method */
    CVMJITIRList	     artBkList;
    /* List of all JSR return target blocks in a method */
    CVMJITIRList             jsrRetTargetList;

    CVMJITStack*	     operandStack; /* IR operand stack */

    /* queue for keeping blocks during IR generation */
    CVMJITIRBlockQueue	     transQ;      

    /* queue for pending local ref refinement */
    CVMJITIRBlockQueue	     refQ;      

    /*
     * Whether the locals information needs to be refined.
     */
    CVMBool                  refineLocalsInformation;

    /*
     * A null-terminated DFS walk of the CFG.
     */
    CVMJITIRBlock**          dfsWalk;
    
    /*
     * Loops, one for each back edge discovered in the flow graph
     */
    CVMJITLoop*              loops;
    CVMUint32                numLoops;
    
    /* Nested loops, in order from smallest to largest */
    CVMJITNestedLoop*        nestedLoops;
    CVMUint32                numNestedLoops;

    /* Indicates if the compiled method has ret opcodes: */
    CVMBool                  hasRet;

    /* true if we are currently emitting conditionally executed code.
     * Regman state is not allowed to change when true. */
    CVMBool                  inConditionalCode;

    /*
     * stacks for use of code generator.
     * The first two are mutually exclusive, so can use the same space.
    struct CVMJITCompileExpression_rule_computation_state*  rcs;
    struct CVMJITCompileExpression_match_computation_state* mcp;
     */
    /*
     * This stack, the codegen semantic stack, is used at the
     * same time as one of the other two, so must be separately allocated
     * However, it can point to a place in the same allocation.
     */
    struct CVMJITStackElement* cgsp;
    struct CVMJITStackElement* cgstackInit;
    struct CVMJITStackElement* cgstackLimit;

    void * 		      compilationStateStack;
    void *                    goal_top; /* Flushed goal_top. */

    /*
     * Block for which code is currently being generated.
     * Used by codegen to get phiSize.
     */
    CVMJITIRBlock*	      currentCompilationBlock;

    /*
     * Maximum size (in words) of all the phi nodes entering any block.
     */
    CVMUint16		      maxPhiSize;
    /*
     * Maximum number of temps needed by code generation in this method.
     */
    CVMInt32		      maxTempWords;
    /*
     * Register management state.
     */
    CVMJITRMCommonContext     RMcommonContext;
    CVMJITRMContext	      RMcontext[2];

    /*
     * Stack management state.
     */
    CVMInt32		      SMdepth;
    CVMInt32		      SMmaxDepth;
#ifndef CVMCPU_HAS_POSTINCREMENT_STORE
    /*
     * JSP offset (in words) for next argument push, which is also the amount
     * we need to eventually adjust JSP by before exiting the compiled method.
     */
    CVMInt32                  SMjspOffset;
#endif
    CVMJITSet		      SMstackRefSet;

    /*
     * Refmap of the locals, as passed to codegen by way of LOCALREFS nodes
     */
    CVMJITSet		      localRefSet;

    /*
     * list of stackmaps we collect during compilation.
     */
    CVMInt32		      stackmapListSize;
    CVMJITStackmapItem*	      stackmapList;
    CVMJITStackmapItem*	      stackmapListTail;
    CVMCompiledStackMaps*     stackmaps;
    
    /*
     * list of GC check PC's we collect during compilation.
     */
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
    CVMUint32		      gcCheckPcsSize;
    CVMUint32                 gcCheckPcsIndex;
    CVMCompiledGCCheckPCs*    gcCheckPcs;
#endif
    
    /*
     * Code emission related structures
     */
    CVMInt32                  curLogicalPC;
    CVMUint8*		      curPhysicalPC;
    CVMInt32                  logicalPCstack[4]; /* for CVMJITcbufPushFixup */
    CVMInt32                  curDepth;          /* depth of logicalPCstack */

#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
    CVMUint32                 logicalInstructionPC;
    CVMJITCSContext           csContext;
#endif /*IAI_CODE_SCHEDULER_SCORE_BOARD*/
#ifdef CVM_DEBUG_ASSERTS
    /* These are for bounds checking OOL and fixup code emission: */
    CVMUint8*                 oolPhysicalStartPCstack[4];
    CVMUint8*                 oolPhysicalEndPCstack[4];
    CVMInt32                  oolCurDepth;
#endif

    /*
     * Method prologue related 
     */

    /* The offset of the interpreted->compiled entry point from the
       beginning of the compiled method */
    CVMInt32                  intToCompOffset;

    /*
     * Some counts for estimating buffer size
     */
    
    CVMUint32                 numMainLineInstructionBytes;
    /* write barriers */
    CVMUint32                 numBarrierInstructionBytes; 
    /* checkcast/instanceof */
    CVMUint32                 numCheckCastInstructionBytes; 
    /* virtual inlining */
    CVMUint32                 numVirtinlineBytes; 
    /* large opcodes: */
    CVMUint32                 numLargeOpcodeInstructionBytes;

    /*
     * Physical buffer stuff 
     */
    CVMUint8*                 codeBufAddr;   /* The allocated code buffer */
    CVMUint8*                 codeBufEnd;    /* End of code buffer */
    CVMUint8*                 codeEntry;     /* start of generated code */
    /* adjustment to expansion factor */
    CVMUint32		      extraCodeExpansion;
    /* estimate for stackmap memory */
    CVMUint32		      extraStackmapSpace;

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
    /* Total number direct methods calls made by this method. Setup by
     * the front end */
    CVMUint32		      numCallees;

    /*
     * The callee table. 
     *  
     * The structure of this table is as follows:  
     *  
     * The value stored at callees[0] is the number of 
     * unique methods that are called by the method being compiled.
     * This number allows us to simplify the traversal of this table 
     * on method decompilation.  What follows is the mb* of each method
     * that this method calls directly.
     */
    CVMMethodBlock**	      callees;
#endif

    /* Error handler context to longjmp to */
    jmp_buf errorHandlerContext;

    CVMUint16                  constantPoolSize;
    CVMJITConstantEntry*       constantPool;
    CVMUint32                  numEntriesToEmit; /* Pending unemitted */
    CVMInt32                   earliestConstantRefPC; /* "Urgency" of dump */

#ifdef CVM_JIT_USE_FP_HARDWARE
    CVMInt32                   earliestFPConstantRefPC; /* "Urgency" of dump */
#endif

    CVMCompiledPcMapTable *pcMapTable;

#ifdef CVM_JIT_COLLECT_STATS
    CVMJITStats               *stats;
#endif

    /*
     * Memory allocator
     */
    CVMJITMemChunk*           memChunks;
    CVMUint32		      workingMemory;

#ifdef CVM_JIT_REGISTER_LOCALS
    /*
     * Array of ASSIGN nodes in the current block. 
     * This array is copied to the block's assignNodes array after the
     * block is parsed. This way the global assignNodes[] can be estimated
     * on the high end, whereas each block has an accurate
     * size. If we exceed CVMJIT_MAX_ASSIGN_NODES within a block, then
     * we stop accumulating ASSIGN nodes, so this array is not
     * guaranteed to be complete. In this case we don't allow the block
     * to have any additional incoming locals because we don't want to 
     * allow any locals that are assigned to to flow in from a prevous block,
     * because there may end up being a stackmap conflict.
     */
    CVMJITIRNode*  assignNodes[CVMJIT_MAX_ASSIGN_NODES];
    CVMUint16      assignNodesCount;
#endif

    /*
     * Target specific stuff.
     */
    CVMJITTargetCompilationContext target;
};

#endif /* _INCLUDED_JIT_IMPL_H */
