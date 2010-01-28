/*
 * @(#)stackmaps.c	1.111 06/10/25
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
#include "javavm/include/stackmaps.h"
#include "javavm/include/indirectmem.h"

#ifdef CVM_JIT
#include "javavm/include/jit/jit.h"
#endif

#include "javavm/include/clib.h"
#include "javavm/include/porting/ansi/setjmp.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/system.h"

#ifdef CVM_JVMTI
#include "javavm/include/jvmtiExport.h"
#endif

#ifdef CVM_HW
#include "include/hw.h"
#endif

#ifdef CVM_DEBUG
static int CVMstackmapVerboseDebug = 0;
#endif

/*
  The Layout of the Stackmap Buffer
  =================================
  The stackmaps for a method is made up of a list of entries: one for each
  GC point in the method.  Each entry has a bytecode PC as a unique key.

  Most stack frames tend to be relatively small.  Hence, the typical stackmap
  entry would fit within 32-bits: a 16-bit PC, and a 15-bit map.  The remaining
  bit is used to indicate whether the map actually has more than 15 locals
  and/or stack words thereby requiring more than 15 bits in the map i.e. does
  it have an extended or super extended entry that holds the real information.
  
  How to find the real entry?
  ==========================
  The stackmap entry can be found by using the PC as the unique key.
  Given the PC, CVMstackmapGetEntryForPC() will acquire a basic stack map
  entry.  A basic stackmap entry is one that fits within 32-bits.

  If bit 0 of entry->state[0] is set, then entry->state[0] actually contains
  an offset to a larger (more than 32bits) stackmap entry either in the
  extended region or the super extended region of stackmaps buffer.

  The offset value is computed as entry->state[0] >> 1, and is in units of
  CVMUint16 i.e. each offset represents a 16 bit increment.

  If the offset is less than CVM_STACKMAP_RESERVED_OFFSET, then the actual
  stackmap entry will reside in the extended region.  It's address can be
  computed using the offset to index into the extended region which resides
  at the end of the basic region.

  If the offset is equal to CVM_STACKMAP_RESERVED_OFFSET, then the actual
  stackmap entry will reside in the super extended region.  The start of the
  extended region will contain a pointer to the CVMStackMapSuperExtendedMaps
  struct (abbreviated as seMaps).  The seMaps will contain a sorted list
  of pointers to every stackmap entry in the super extended region.  To
  find the address of the actual stackmap entry that reside in the super
  extended region, we do a binary search on the entries in the seMaps
  keying off the PC.

  The memory layout
  =================

  Stackmaps buffer:
                         |----------------------------|
      CVMStackMaps       | header info                |
                         |----------------------------|
      basic region       | entry[0]                   |
                         | entry[1]                   |
                         | entry[2]                   |
                         | ...                        |
                         | entry[N]                   |
                         |----------------------------|
      extended region    | &seMaps                    |     
                         | extendedEntries[0]         |
                         | extendedEntries[1]         |
                         | ...                        |
                         | extendedEntries[nE]        |
                         |----------------------------|
      seMaps             | numOfSuperExtendedEntries  |
                         | &superExtendedEntries[0]   |
                         | &superExtendedEntries[0]   |
                         | ...                        |
                         | &superExtendedEntries[nSE] |
                         |----------------------------|
      super extended     | superExtendedEntries[0]    |
      region             | superExtendedEntries[1]    |
                         | ...                        |
                         | superExtendedEntries[nSE]  |
                         |----------------------------|

  where
      N is the total number of GC points.  For each GC point
          there is one basic entry.
      nE is the total number of entries in the extended region
          which can be reached using an offset stored in the
	  basic entry.
      nSE is the total number of entries in the super extended
          region.  Entries in this region must be looked up via
	  the seMaps table.

  Note: A given GC point can have only one of the following:
        1. a basic entry
        2. a basic entry + an extended entry
        3. a basic entry + a super extended entry

	It cannot have both an extended entry and a super extended entry.
	Hence, nE + nSE <= N.

  Note: the sizes of the entries in the extended region and super extended
        region are of variable size.  They aren't guaranteed to be uniform.

  Stackmap Context pointers
  =========================
  stackMapAreaBegin points pass the end of the basic region to the start of
  the extended region.

  stackMapAreaEnd points to the end of the entire stackmaps buffer.  If a
  super extended region is present, it will point just pass the end of the
  super extended region.  Otherwise, it will point pass the end of the
  extended region or the basic region depending on whether the extended
  region is present or not.

  stackMapAreaPtr is used to determine the next location in the stackmaps
  buffer to emit data into.  If the extended region is present, initially,
  it is pointed just pass the seMaps pointer in preparation for emitting
  the first entry in the extended region.  If the extended region is not
  present, it will set equal to stackMapAreaBegin and will never be
  incremented.

  Optional Regions
  ================
  The extended region, seMaps, and super extended region are all optional.
  They are included only when needed.  In most cases, these regions will
  not be present in the stackmaps.

  What does this mean to the GC?
  =============================
  The GC is not aware of the internal layout of the stackmap entries.  The
  GC calls CVMstackmapGetEntryForPC(), and the appropriate stackmap entry
  will be returned to it.  The returned entry may be a basic entry, an
  extended entry, or a super extended entry.  They all look the same to the
  GC.  The GC expects the entry to be of variable size.  The boundary of
  the map is not attained from the entry but rather computed from the runtime
  topOfStack pointer of the stack frame.

 */

/* CVM_STACKMAP_ENTRY_MIN_NUMBER_OF_CHUNKS is the number of CVMUint16 chunks
   that is available to store map bits in a basic stackmap entry.  In the
   current implementation, this is always 1 and is determined by the
   definition of the CVMStackMapEntry struct in classes.h.
*/
#define CVM_STACKMAP_ENTRY_MIN_NUMBER_OF_CHUNKS    1

/* CVM_STACKMAP_ENTRY_NUMBER_OF_BASIC_BITS is the number of map bits available
   in a basic stackmap entry.  For a CVM_STACKMAP_ENTRY_MIN_NUMBER_OF_CHUNKS
   value of 1, the number of bits will be 15 (i.e. 16 - 1 bit reserved to
   indicate the presence/absence of an extended entry.
*/
#define CVM_STACKMAP_ENTRY_NUMBER_OF_BASIC_BITS \
    ((CVM_STACKMAP_ENTRY_MIN_NUMBER_OF_CHUNKS * sizeof(CVMUint16) * 8) - 1)

/* CVM_STACKMAP_RESERVED_OFFSET is the reserved offset to indicate that
   the stackmap entry is located in the super extended region of the
   stackmaps buffer.  See comment above for more details. */
#define CVM_STACKMAP_RESERVED_OFFSET   (0x7fff)

/* CVMStackMapSuperExtendedMaps is used to store a table of pointers to
   CVMStackMapEntry structs that reside in the super extended region of
   the stackmaps buffer.  See comment above on the layout of the stackmaps
   buffer for more details. */
typedef struct CVMStackMapSuperExtendedMaps CVMStackMapSuperExtendedMaps;
struct CVMStackMapSuperExtendedMaps
{
    CVMUint32 numberOfEntries;
    CVMStackMapEntry *entries[1];
};


#define WRITE_INT16( pc, val ) { \
	(pc)[0] = (val) >> 8;    \
	(pc)[1] = val & 0xff;    \
    }
#define WRITE_INT32( pc, val ) {   \
	(pc)[0] = (val) >> 24;     \
	(pc)[1] = (val >>16)&0xff; \
	(pc)[2] = (val >>8)&0xff;  \
	(pc)[3] = val & 0xff;      \
    }

#define CVMalign4( n ) (( (n)+3)&~3)
/*
 * This file contains routines to generate stackmaps for GC points.
 *
 * ASSUMPTION: The code for the method we are analyzing has already
 * been verified.
 */
typedef CVMUint32 CVMCellTypeState;

/*
 * Each cell (a local variable or a stack slot) can hold one of the
 * following values. 
 *
 * (The program counter is used to track a JSR block,
 *  and holds the *target* of a JSR, and not the return value).
 */
#define CVM_MAP_BIT(n)    ((CVMCellTypeState)1 << (n))
#define CVMctsBottom      ((CVMCellTypeState)0)
#define CVMctsUninit      CVM_MAP_BIT(31) /* The initial state of all locals 
					     (except the args) */
#define CVMctsRef         CVM_MAP_BIT(30) /* A reference type */
#define CVMctsVal         CVM_MAP_BIT(29) /* A value type */
#define CVMctsPC          CVM_MAP_BIT(28) /* A JSR PC type. The rest of the
					     bits of the state contain the
					     target PC of the JSR */
#define CVMctsAll         (CVMctsUninit | CVMctsRef | CVMctsVal | CVMctsPC)

#define CVMctsIsUninit(cts)  (((cts) & CVM_MAP_BIT(31)) != 0)
#define CVMctsIsRef(cts)     (((cts) & CVM_MAP_BIT(30)) != 0)
#define CVMctsIsVal(cts)     (((cts) & CVM_MAP_BIT(29)) != 0)
#define CVMctsIsPC(cts)      (((cts) & CVM_MAP_BIT(28)) != 0)

#define CVMctsGetPC(cts)     ((cts) & ~CVMctsAll)
/*
 * In a 'CVMCellTypeState' the (relative) PC is only 28 bits wide anyway.
 * So there's no need to worry about losing bits and we can safely cast
 * our (on 64 bit platforms 64 bit wide) expression to 'CVMCellTypeState'. 
 */
#define CVMctsMakePC(pc)     ((CVMCellTypeState)(CVMctsPC | (pc)))
#define CVMctsGetFlags(cts)  ((cts) & CVMctsAll)

typedef struct CVMBasicBlock    CVMBasicBlock;
typedef struct CVMJsrTableEntry CVMJsrTableEntry;

/*
 * The representation of a basic block. 
 */
struct CVMBasicBlock {
    CVMUint8*              startPC;      /* The absolute (ptr) start PC */
    CVMCellTypeState*      varState;     /* The vars state on entry */
    CVMCellTypeState*      stackState;   /* The stack state on entry */
    CVMBool                changed;      /* Has the entry state changed? */
    CVMInt32               topOfStack;   /* The topOfStack for interpreting */
    CVMBool		   isLive;	 /* Is this block reachable? */
    CVMBool		   endsWithJsr;  /* The bb ends with a JSR opcode? */
    struct CVMBasicBlock*  liveNext;	 /* Next on liveness analysis list */
#ifdef CVM_JIT
    CVMBool                endsWithRet;  /* The bb ends with a ret opcode? */
    CVMInt32               endTopOfStack;/* The topOfStack at the end of bb. */
#endif
    struct CVMBasicBlock** successors;   /* Successor BB's (0-terminated) */
};

/*
 * The representation of a JSR table. Keeps track of all JSR targets,
 * and for each, its callers. That way, we know who the possible callers
 * of a jsr block are when we encounter a 'ret' instruction during
 * abstract interpretation
 */
struct CVMJsrTableEntry {
    CVMUint32        jsrPC;      /* The start of the jsr block (relative) */
    CVMUint32        jsrNoCalls; /* No of callers for this jsr target */
    CVMBasicBlock**  jsrCallers; /* Caller BB's of this jsr */
};

/*
 * When code is rewritten due to conflicts, it can change sizes.
 * The remapping of relative addresses is tracked using this.
 * It must be kept sorted by address.
 */
typedef struct CVMstackmapPCMapEntry{
    int addr;
    int adjustment;
}CVMstackmapPCMapEntry;

typedef struct CVMstackmapPCMap {
    int nAdjustments;
    int mapSize;
    struct CVMstackmapPCMapEntry map[1];
} CVMstackmapPCMap;

/*
 * The context for the current stackmap generation
 */
typedef struct {
    CVMExecEnv*        ee;

    /* Error handler context to longjmp to */
    jmp_buf errorHandlerContext;

    /* Method information */
    CVMMethodBlock*    mb;
    CVMJavaMethodDescriptor* jmd;
    CVMClassBlock*     cb;
    CVMConstantPool*   cp;
    CVMUint8*          code;
    CVMUint16          codeLen;
    CVMUint32          stateSize; /* Total 'state' size */
    CVMUint16          nVars;     /* No of locals */
    CVMUint16          maxStack;  /* Maximum number of stack slots */

    /* Created data structures */

    /* The first three are the heads of all allocated memory.
       We free them when done, or on error. */
    CVMBasicBlock*     basicBlocks;
    void*              mapsArea;
    CVMStackMaps*      stackMaps;

    CVMUint32          nBasicBlocks;
    CVMUint32          nGCPoints;
    CVMUint32*         gcPointsBitmap;
    CVMBasicBlock**    successorsArea;
    CVMBasicBlock*     liveStack; /* liveness analysis stack */

    /* Mapping from exception handler to basic block */
    CVMUint32          nExceptionHandlers;
    CVMBasicBlock**    exceptionsArea; 

    /* Jsr table */
    CVMJsrTableEntry*  jsrTable;
    CVMUint32          jsrTableSize;

    /* The state of the interpretation of the current basic block.
       We need this because we don't want to destroy the entry state of
       a basic block. */
    CVMCellTypeState*  varState;
    CVMCellTypeState*  stackState;

    /*
     * The memory area that holds the stackmaps. 
     */
    CVMUint16*         stackMapAreaEnd;
    CVMUint16*         stackMapAreaBegin; /* The allocation pointer */
    CVMUint16*         stackMapAreaPtr; /* The allocation pointer */
    CVMUint32          stackMapLongFormatNumBytes; /* # of long format bytes */
     
    /* Tracking info for the super extended region: */
    CVMUint32          numberOfSuperExtendedEntries;
    CVMStackMapSuperExtendedMaps *seMaps;
    CVMUint32          currentNumberOfSEEntries;

    /*
     * Ref-uninit conflict uses of local variables
     */
    CVMCellTypeState*  refVarsToInitialize;
    CVMUint32          numRefVarsToInitialize;

    /*
     * For variable splitting, ref-pc and ref-val conflicts
     */
    CVMUint32**		localRefsWithAddrAtTOS;
    CVMUint32		nlocalRefsWithAddrAtTOS;
    CVMUint32		maxlocalRefsWithAddrAtTOS;

    int			nRefValConflicts;
    int            	nRefPCConflicts;
    int			varMapSize;
    CVMUint32*		newVarMap; /* for re-mapping conflicting variables */
    int                 newVars;   /* The number of new variables */
    
    /*
     * Whether this method may need to be re-written to eliminate conflicts.
     */
    CVMBool            mayNeedRewriting;

    /*
     * A buffer of the right size to print variable and stack state
     */
    char*              printBuffer;
    
    /*
     * A flag indicating whether we do consider "conditional GC-points",
     * encountered during quickening of an opcode. If there is a GC at that
     * point, we have to re-generate stackmaps to include entries for these
     * conditional GC points, since two threads racing to quicken an
     * instruction may cause a conditional GC point to be visible to GC.
     */
    CVMBool           doConditionalGcPoints;

    /* Error code */
    CVMMapError   error;
} CVMStackmapContext;


/* Basic block macros: */
#ifdef CVM_JIT
#define CVMbbEndsWithRet(bb)            ((bb)->endsWithRet)
#define CVMbbStartTopOfStackCount(bb)   ((bb)->topOfStack)
#define CVMbbEndTopOfStackCount(bb)     ((bb)->endTopOfStack)
#endif

/*
 * Forward declarations
 */
static void
CVMstackmapRewriteMethodForConflicts(CVMStackmapContext* con);

static CVMBool
isJsrTarget(CVMStackmapContext* con, CVMUint8* pc);

static void
remapConflictVarno(CVMStackmapContext* con, int varNo);

static int
mapVarno(CVMStackmapContext * con, int varNo, CVMUint8 * pc );

static CVMBool
CVMstackmapAnalyze( CVMStackmapContext *con );

#ifdef CVM_JVMTI
static CVMUint32
getOpcodeLength(CVMStackmapContext* con, CVMUint8* pc)
{
    CVMUint32 opLen;
    /* Make sure we get the underlying instruction length, and not that
       of opc_breakpoint */
    if (*pc == opc_breakpoint) {
	/* Find the length of the original opcode, so we can
	   skip over it by the appropriate amount */
	CVMOpcode instr = CVMjvmtiGetBreakpointOpcode(con->ee, pc, CVM_FALSE);
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
#define getOpcodeLength(con, pc) (CVMopcodeGetLength(pc))
#endif

static void
CVMstackmapInitializeContext(CVMExecEnv* ee, CVMStackmapContext* con,
			     CVMMethodBlock* mb, CVMBool doConditionalGcPoints)
{
    CVMJavaMethodDescriptor* jmd;
    memset((void*)con, 0, sizeof(CVMStackmapContext));
    jmd            = CVMmbJmd(mb);
    con->ee        = ee;
    con->mb        = mb;
    con->cb        = CVMmbClassBlock(mb);
#ifdef CVM_JVMTI
    if (CVMjvmtiMbIsObsolete(mb)) {
	con->cp        = CVMjvmtiMbConstantPool(mb);
	if (con->cp == NULL) {
	    con->cp = CVMcbConstantPool(con->cb);
	}	
    } else
#endif
    {
	/* Matching else from above */
	con->cp        = CVMcbConstantPool(con->cb);
    }
    con->jmd       = jmd;
    con->code      = CVMjmdCode(jmd);
    con->codeLen   = CVMjmdCodeLength(jmd);
    con->nVars     = CVMjmdMaxLocals(jmd);
    /*
     * This expression is obviously a rather small pointer
     * difference. So just cast it to the type of 'maxStack'.
     */
    con->maxStack  = (CVMUint16)(CVMjmdMaxStack(jmd));
    con->stateSize = con->nVars + con->maxStack;
    con->nExceptionHandlers = CVMmbExceptionTableLength(con->mb);
    con->doConditionalGcPoints = doConditionalGcPoints;
}

static void
CVMstackmapDestroyContext(CVMStackmapContext* con)
{
    free(con->mapsArea);
    free(con->basicBlocks);
    if (con->stackMaps != NULL) {
	/* con->stackMaps was not assigned to the method */
	free(con->stackMaps);
    }
    if ( con->localRefsWithAddrAtTOS != NULL )
	free( con->localRefsWithAddrAtTOS );
    if ( con->newVarMap != NULL )
	free( con->newVarMap );
}

#ifdef CVM_TRACE
static const char*
errorString(CVMMapError errorCode)
{
    switch(errorCode) {
    case CVM_MAP_SUCCESS: 
	return "Success";
    case CVM_MAP_OUT_OF_MEMORY: 
	return "Out of memory";
    case CVM_MAP_EXCEEDED_LIMITS: 
	return "Exceeded limits";
    case CVM_MAP_CANNOT_MAP: 
	return "Cannot compute maps";
    default:
	return "Unknown error code";
    }
}
#endif

static void
throwError(CVMStackmapContext* con, CVMMapError error)
{
    con->error = error;
    longjmp(con->errorHandlerContext, 1);
}

#if CVM_JIT
/* Purpose: Filter out some uncompilable methods based on the stackmap data
            flow info. */
void
CVMstackmapFilterNonCompilableMethod(CVMExecEnv *ee, CVMStackmapContext *con)
{
    CVMUint32 i;
    for (i = 0; i < con->nBasicBlocks; i++) {
        CVMBasicBlock *bb = con->basicBlocks + i;

        /* We cannot compile method which have:
           1. jsr opcodes with a non empty stack at that point which is the
              same as the start of a jsr target block with non empty stack.
           2. ret opcodes with a non-empty stack at that point.

           These kinds of scenarios are rare because javac does not generate
           code like this.  So, mark these methods as never compile and let
           them run interpretered instead.
        */
        if ((isJsrTarget(con, bb->startPC) &&
             (CVMbbStartTopOfStackCount(bb) > 1)) ||
            (CVMbbEndsWithRet(bb) && (CVMbbEndTopOfStackCount(bb) != 0))) {
            CVMJITneverCompileMethod(ee, con->mb);
            return;
        }
    }
}
#endif /* CVM_JIT */

static void 
CVMstackmapMarkGCPoint(CVMStackmapContext* con,
		       CVMUint32* gcPointsBitmap, CVMUint16 pc)
{
    CVMUint32 idx = pc / 32;
    CVMUint32 bit = (1 << (pc % 32));
    /* Mark and count if it has not been seen before */
    if ((gcPointsBitmap[idx] & bit) == 0) {
	gcPointsBitmap[idx] |= bit;
	con->nGCPoints++;
    } 
}

static CVMBool 
CVMstackmapIsGCPoint(CVMUint32* gcPointsBitmap, CVMUint16 pc)
{
    CVMUint32 idx = pc / 32;
    CVMUint32 bit = (1 << (pc % 32));
    return ((gcPointsBitmap[idx] & bit) != 0);
}

/*
 * For maintaining the liveness analysis stack.
 * LIFO stack, to which we only add blocks that aren't already
 * marked as live. Mark them as live as we push on the stack
 * to prevent re-adding later.
 */
static void
CVMstackmapLivenessPush(CVMStackmapContext* con, CVMBasicBlock* bb){
    if (bb->isLive){
	return; /* already on list or analysed */
    }
    bb->isLive = CVM_TRUE;
    bb->liveNext = con->liveStack;
    con->liveStack = bb;
}

static CVMBasicBlock*
CVMstackmapLivenessPop(CVMStackmapContext* con){
    CVMBasicBlock* nextBlock = con->liveStack;
    if (nextBlock == NULL){
	return NULL;
    }
    con->liveStack = nextBlock->liveNext;
    nextBlock->liveNext = NULL; 
    return nextBlock;
}

static CVMBool
CVMstackmapLivenessProcessNext(CVMStackmapContext* con){
    CVMBasicBlock* bb = CVMstackmapLivenessPop(con);
    CVMBasicBlock**successor;
    CVMBasicBlock* successorBlock;

    if (bb == NULL){
	return CVM_FALSE;
    }
    CVMassert(bb->isLive);
    successor = bb->successors;
    /* If there is no list of successors, then there can be no JSR either */
    if (successor != NULL){
	while ((successorBlock = *successor++) != NULL){
	    CVMstackmapLivenessPush(con, successorBlock);
	}
	if (bb->endsWithJsr){
	    /* 
	     * the block immediately following this one
	     * is live, too, as the jsr/ret combination
	     * will end up there. The jsr is a "fall through"
	     * instruction in this sense, unlike goto.
	     */
	    CVMstackmapLivenessPush(con, bb+1);
	}
    }
    return CVM_TRUE;
}

static void
CVMstackmapLivenessAnalyze(CVMStackmapContext* con){
    CVMBasicBlock* bb;
    CVMUint32      nBasicBlocks;

    /*
     * First, do the flow to find all the live blocks
     * and thus detect the dead ones.
     */
    while (CVMstackmapLivenessProcessNext(con))
	;

    /*
     * Now find any that are dead and which end
     * with a JSR. The following block is on a list
     * of jsr targets and must be removed.
     */
    for (bb = con->basicBlocks, nBasicBlocks = con->nBasicBlocks;
	nBasicBlocks>0;
	bb++, nBasicBlocks--){
	    CVMBasicBlock * nextBb;
	    CVMJsrTableEntry* jsrTable;
	    CVMInt32        jsrTableSize;
	    CVMInt32	    ncallers;
	    if (bb->isLive){
		/* this block is fine */
		continue;
	    }
	    if (!(bb->endsWithJsr)){
		/* don't care - this is not the situation we're looking for */
		continue;
	    }
	    nextBb = bb+1; /* which will really appear in the tables. */
	    /*
	     * This may not be the most efficient search,
	     * but it is certainly correct.
	     * Since this never happens, it doesn't matter much.
	     */
	    for( jsrTable = con->jsrTable, jsrTableSize = con->jsrTableSize;
		jsrTableSize> 0;
		jsrTableSize--, jsrTable++){
		    ncallers = jsrTable->jsrNoCalls;
		    while (ncallers-- > 0){
			if (jsrTable->jsrCallers[ncallers] == nextBb){
			    /* expunge this reference */
			    jsrTable->jsrCallers[ncallers] = NULL;
			}
		    }
	    }
    }
}

/*
 * Find the CVMJsrTableEntry for a given targetPC. Return 0 if the
 * target does not occur in the JSR table. Linear search is good
 * enough, since this is so uncommon.  
 */
static CVMJsrTableEntry*
CVMstackmapGetJsrTargetEntry(CVMStackmapContext* con, CVMUint32 targetPC)
{
    int i;
    for (i = 0; i < con->jsrTableSize; i++) {
	if (con->jsrTable[i].jsrPC == targetPC) {
	    return &con->jsrTable[i];
	}
    }
    return 0;
}

/*
 * Add a new jsr call to the list of jsr's encountered. If this is the first
 * time this jsr target was encountered, make a new entry for it.
 */
/* 
 * CVMstackmapAddJsrCall()
 * the argument returnPC is cast to type CVMBasicBlock*
 * therefore the type has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
static void
CVMstackmapAddJsrCall(CVMStackmapContext* con,
		      CVMUint16 targetPC, CVMAddr returnPC,
		      CVMBasicBlock*** jsrCallersArea, CVMUint32 nJsrs)
{
    CVMJsrTableEntry* targetEntry =
	CVMstackmapGetJsrTargetEntry(con, targetPC);
    /*
     * If we haven't seen this target yet, add a new
     * CVMJsrTableEntry to the list of known jsr's.
     */
    if (targetEntry == 0) {
	targetEntry = &con->jsrTable[con->jsrTableSize++];
	targetEntry->jsrPC = targetPC;
	targetEntry->jsrNoCalls = 0;
	targetEntry->jsrCallers = *jsrCallersArea;
	/*
	 * Allocate the worst-case number of callers +
	 * one word for zero-termination.
	 *
	 * This is uncommon enough and the number
	 * of jsr's is few enough, so I am not
	 * worried about the space waste here.
	 */
	*jsrCallersArea += nJsrs;
    }
    /*
     * Store the return from this jsr as the caller
     * address. This address will be the successor
     * of a matching ret instruction.
     *
     * Note: We will be mapping the returnPC to a basic block with
     * returnPC as the header.
     */
    targetEntry->jsrCallers[targetEntry->jsrNoCalls++] =
	(CVMBasicBlock*)returnPC;
}

/*
 * Make two passes over the code in order to build basic blocks. 
 *
 * The first pass:
 *
 * 1) Find out where basic block headers are.
 * 2) Count the number of basic blocks
 * 3) Count the total number of "successors" to each basic block
 *
 * At the end of this pass, we are ready to create the basic blocks
 * themselves, as well as the data area for all successors.
 *
 * The second pass:
 *
 * 1) Make the basic block structures
 * 2) Set successors of basic blocks
 * 3) Do fast mappings of exception edges and return tables to basic blocks.
 *
 * %comment rt010
 */
static void
CVMstackmapFindBasicBlocks(CVMStackmapContext* con)
{
    /*
     * Two maps. pcToBBMap indicates at first whether a bytecode is a
     * basic block header or not and later on holds a mapping of basic
     * block header PC's to basic block structures. gcPointsBitmap
     * tells us which PC's hold GC points (including backwards
     * branches, etc.)
     */
    void*           mapsArea;
    CVMBasicBlock** pcToBBMap;
    CVMUint32*      gcPointsBitmap;

#define CVM_MAP_MARK_BB_HDR(pc) \
    pcToBBMap[(CVMUint32)(pc)] = (CVMBasicBlock*)1
#define CVM_MAP_IS_BB_HDR(pc) \
    (pcToBBMap[(CVMUint32)(pc)] != 0)
#define CVM_MAP_SET_BB_FOR_PC(pc, bb) \
    pcToBBMap[(CVMUint32)(pc)] = (bb)
#define CVM_MAP_GET_BB_FOR_PC(pc) \
    (pcToBBMap[(CVMUint32)(pc)])

    CVMUint32  nSuccessors = 0;
    CVMUint32  nBasicBlocks = 0;
    CVMUint32  nInvocations = 0;
    CVMUint32  nJsrs = 0;  /* No of 'jsr' calls */
    CVMUint32  nExceptionHandlers = con->nExceptionHandlers;

    CVMBasicBlock**   jsrCallersArea;
    CVMUint8*  pc;
    CVMUint8*  codeBegin;
    CVMUint8*  codeEnd;

    void*      bbAndSuccessorsArea;
    CVMBasicBlock*  basicBlocks;
    CVMBasicBlock*  currentBasicBlock;
    CVMBasicBlock** successorsArea;
    CVMBasicBlock** successorsAreaEnd;
    CVMBasicBlock** exceptionsArea;
    CVMCellTypeState*  stateArea;

    CVMUint32  memSize; /* The size of allocated extra memory */

    /* 
     * mapsArea needs space to hold native pointers because
     * it is cast to CVMBasicBlock**
     * therefore use sizeof(CVMAddr) which is 4 byte on 32 bit
     * platforms and 8 byte on 64 bit platforms
     */
    mapsArea  = calloc(con->codeLen + (con->codeLen + 31) / 32,
		       sizeof(CVMAddr));
    if (mapsArea == NULL) {
	throwError(con, CVM_MAP_OUT_OF_MEMORY);
    }
    pcToBBMap = (CVMBasicBlock**)mapsArea;
    gcPointsBitmap = (CVMUint32*)(pcToBBMap + con->codeLen);

    con->mapsArea = mapsArea; /* Will free when done */
    con->gcPointsBitmap = gcPointsBitmap; /* A summary of GC points seen */
    /*
     * Start counting and marking 
     */
    codeBegin = con->code;
    codeEnd   = &con->code[con->codeLen];
    pc = codeBegin;
    CVM_MAP_MARK_BB_HDR(0); /* The first instruction is a basic block */
    nBasicBlocks++;

    /* The first instruction is also a GC point */
    CVMstackmapMarkGCPoint(con, gcPointsBitmap, 0);

    while(pc < codeEnd) {
	CVMOpcode instr = (CVMOpcode)*pc;
	CVMUint32 instrLen = getOpcodeLength(con, pc);
	if (CVMbcAttr(instr, BRANCH)) {
	    /*
	     * Set headers, and count successors for all branches.
	     * We either have a goto or jsr, a tableswitch, a lookupswitch,
	     * or one of the if's.
	     */
	    switch(instr) {
	        case opc_goto:
	        case opc_jsr: {
		    /* An unconditional goto, 2-byte offset */
		    CVMInt16 offset = CVMgetInt16(pc+1);
                    /* 
                     * The one and only use for 'codeOffset' is to
                     * store pointer differences. Even if we know that
                     * they will always fit into 16 bits I can see no
                     * sense in using a 16 bit variable since the code
                     * will never be slower (but often faster) if we
                     * use a variable matching the machine size
                     * instead.  */
		    CVMAddr codeOffset = pc + offset - codeBegin;
		    if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			CVM_MAP_MARK_BB_HDR(codeOffset);
			nBasicBlocks++;
		    }
		    CVMstackmapMarkGCPoint(con,
					   gcPointsBitmap,
                                           (CVMUint16)(pc - codeBegin));
		    nSuccessors++;
		    /* And mark the following instruction */
		    codeOffset = pc + instrLen - codeBegin;
		    if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			CVM_MAP_MARK_BB_HDR(codeOffset);
			nBasicBlocks++;
		    }
		    if (instr == opc_jsr) {
			nJsrs++;
		    }
		    break;
		}
	        case opc_goto_w:
	        case opc_jsr_w: {
		    /* An unconditional goto, 4-byte offset */
		    CVMInt32 offset = CVMgetInt32(pc+1);
                    /* 
                     * The one and only use for 'codeOffset' is to
                     * store pointer differences. Even if we know that
                     * they will always fit into 32 bits I can see no
                     * sense in using a 32 bit variable since the code
                     * will never be slower (but often faster) if we
                     * use a variable matching the machine size
                     * instead.  */
		    CVMAddr codeOffset = pc + offset - codeBegin;
		    /* We don't know how to deal with code size > 64k */
		    CVMassert(codeOffset <= 65535);
		    if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			CVM_MAP_MARK_BB_HDR(codeOffset);
			nBasicBlocks++;
		    }
		    nSuccessors++;
		    /* And mark the following instruction */
		    codeOffset = pc + instrLen - codeBegin;
		    if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			CVM_MAP_MARK_BB_HDR(codeOffset);
			nBasicBlocks++;
		    }
		    CVMstackmapMarkGCPoint(con,
					   gcPointsBitmap,
                                           (CVMUint16)(pc - codeBegin));
		    if (instr == opc_jsr_w) {
			nJsrs++;
		    }
		    break;
	        }
	        case opc_lookupswitch: {
		    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(pc+1);
		    CVMInt32  skip = CVMgetAlignedInt32(lpc); /* default */
		    CVMInt32  npairs = CVMgetAlignedInt32(&lpc[1]);
		    /* First mark the default */
                    /* 
                     * The one and only use for 'codeOffset' is to
                     * store pointer differences. Even if we know that
                     * they will always fit into 16 bits I can see no
                     * sense in using a 16 bit variable since the code
                     * will never be slower (but often faster) if we
                     * use a variable matching the machine size
                     * instead.  */
		    CVMAddr codeOffset = pc + skip - codeBegin;
		    if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			CVM_MAP_MARK_BB_HDR(codeOffset);
			nBasicBlocks++;
		    }

		    CVMstackmapMarkGCPoint(con,
					   gcPointsBitmap,
                                           (CVMUint16)(pc - codeBegin));
		    nSuccessors += npairs + 1;
		    /* And all the possible case arms */
		    lpc += 3; /* Go to the first offset */
		    while (--npairs >= 0) {
			skip = CVMgetAlignedInt32(lpc);
			codeOffset = pc + skip - codeBegin;
			if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			    CVM_MAP_MARK_BB_HDR(codeOffset);
			    nBasicBlocks++;
			}
			lpc += 2; /* next offset */
		    }

		    /* Mark the following instruction as block header */
		    codeOffset = pc + instrLen - codeBegin;
		    if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			CVM_MAP_MARK_BB_HDR(codeOffset);
			nBasicBlocks++;
		    }
		    break;
	        }
	        case opc_tableswitch: {
		    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(pc+1);
		    CVMInt32  skip = CVMgetAlignedInt32(lpc); /* default */
		    CVMInt32  low  = CVMgetAlignedInt32(&lpc[1]);
		    CVMInt32  high = CVMgetAlignedInt32(&lpc[2]);
		    CVMInt32  noff = high - low + 1;
		    /* First mark the default */
                    /* 
                     * The one and only use for 'codeOffset' is to
                     * store pointer differences. Even if we know that
                     * they will always fit into 16 bits I can see no
                     * sense in using a 16 bit variable since the code
                     * will never be slower (but often faster) if we
                     * use a variable matching the machine size
                     * instead.  */
		    CVMAddr codeOffset = pc + skip - codeBegin;
		    if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			CVM_MAP_MARK_BB_HDR(codeOffset);
			nBasicBlocks++;
		    }
		    CVMstackmapMarkGCPoint(con,
					   gcPointsBitmap,
                                           (CVMUint16)(pc - codeBegin));
		    nSuccessors += noff + 1;
		    lpc += 3; /* Skip default, low, high */
		    while (--noff >= 0) {
			skip = CVMgetAlignedInt32(lpc);
			codeOffset = pc + skip - codeBegin;
			if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			    CVM_MAP_MARK_BB_HDR(codeOffset);
			    nBasicBlocks++;
			}
			lpc++;
		    }

		    /* Mark the following instruction as block header */
		    codeOffset = pc + instrLen - codeBegin;
		    if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			CVM_MAP_MARK_BB_HDR(codeOffset);
			nBasicBlocks++;
		    }
		    break;
	        }
	        default: {
		    CVMInt16 skip;
                    /* 
                     * The one and only use for 'codeOffset' is to
                     * store pointer differences. Even if we know that
                     * they will always fit into 16 bits I can see no
                     * sense in using a 16 bit variable since the code
                     * will never be slower (but often faster) if we
                     * use a variable matching the machine size
                     * instead.  */
		    CVMAddr codeOffset;
		    /* This had better be one of the 'if' guys */
		    CVMassert(((instr >= opc_ifeq) &&
			       (instr <= opc_if_acmpne)) ||
			      (instr == opc_ifnull) ||
			      (instr == opc_ifnonnull));
		    CVMassert(instrLen == 3);
		    skip = CVMgetInt16(pc+1);
		    /* Mark the target of the 'if' */
		    codeOffset = pc + skip - codeBegin;
		    if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			CVM_MAP_MARK_BB_HDR(codeOffset);
			nBasicBlocks++;
		    }
		    CVMstackmapMarkGCPoint(con,
					   gcPointsBitmap,
					   (CVMUint16)(pc - codeBegin));
		    /* And mark the following instruction */
		    codeOffset = pc + 3 - codeBegin;
		    if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
			CVM_MAP_MARK_BB_HDR(codeOffset);
			nBasicBlocks++;
		    }
		    nSuccessors += 2; /* Target + fall-through */
		}
	    }
	} else if (CVMbcAttr(instr, NOCONTROLFLOW) ||
		   ((instr == opc_wide) && (pc[1] == opc_ret))) {
	    if (pc + instrLen < codeEnd) { /* Don't unless there are more
					      instructions */
                /* 
                 * The one and only use for 'codeOffset' is to store a pointer
                 * difference. Even if we know that they will always fit into
                 * 16 bits I can see no sense in using a 16 bit variable since
                 * the code will never be slower (but often faster) if we use
                 * a variable matching the machine size instead.
                 */
		CVMAddr codeOffset;
		/*
		 * Mark the next guy as a new basic block header
		 */
		codeOffset = pc + instrLen - codeBegin;
		if (!CVM_MAP_IS_BB_HDR(codeOffset)) {
		    CVM_MAP_MARK_BB_HDR(codeOffset);
		    nBasicBlocks++;
		}
	    }
	}

        {
            /*
             * We counted the potential backwards branches. Now count the
             * other GC points.
             */
            CVMBool needStackMap = CVMbcAttr(instr, GCPOINT) ||
                (con->doConditionalGcPoints && CVMbcAttr(instr, COND_GCPOINT));
#ifdef CVM_JVMPI_TRACE_INSTRUCTION
            /* JVMPI needs this in order to support instruction tracing: */
            needStackMap = CVM_TRUE;
#endif
#ifdef CVM_JVMTI
            /* If JVMTI is enable, mark all instructions as GC points
               because a thread can be suspended anywhere.  */
            needStackMap |= CVMjvmtiIsEnabled();
#endif
            if (needStackMap) {
                CVMstackmapMarkGCPoint(con, gcPointsBitmap,
                                       (CVMUint16)(pc - codeBegin));
            }
        }

	/*
	 * Also count the number of invocations in this method.
	 */
	if (CVMbcAttr(instr, INVOCATION)) {
	    nInvocations++;
	}
	pc += instrLen;
    }
    /*
     * And finally, make sure we mark exception handlers as basic blocks.
     * Instructions that may throw exceptions will propagate their states
     * to all possible exception handlers.
     */
    if (nExceptionHandlers > 0) {
	CVMExceptionHandler* excTab = CVMmbExceptionTable(con->mb);
	CVMInt32 nEntries = nExceptionHandlers;
	while(--nEntries >= 0) {
	    if (!CVM_MAP_IS_BB_HDR(excTab->handlerpc)) {
		CVM_MAP_MARK_BB_HDR(excTab->handlerpc);
		nBasicBlocks++;
	    }
	    /* The start PC of an exception handler is a GC point */
	    CVMstackmapMarkGCPoint(con, gcPointsBitmap, excTab->handlerpc);
	    excTab++;
	}
    }
    /*
     * Do this just to make sure. There may be two consecutive basic blocks
     * with no branches from one to the other. In that case, we want to be
     * acting as if we have a jump from the end of one to the start of the
     * other. To get a right number for nSuccessors, we would have to make
     * another pass over the code, which is really not that essential, so
     * just guess over the real number a little bit.
     */
    nSuccessors += nBasicBlocks;
    /*
     * And this conservative one for the NULL-termination
     */
    nSuccessors += nBasicBlocks;
    /*
     * At this point, we have nBasicBlocks b.b.'s, nSuccessors
     * successors, nExceptionHandlers exception edges, and nGCPoints
     * GC points. Each basic block also holds con->stateSize
     * CVMCellTypeState's of local variable and stack state.
     *
     * Also, a conservative estimate of all the JSR table memory
     * needed is nJsr's CVMJsrTableEntry's, nJsr ^ 2
     * CVMBasicBlock*'s to hold callers of each.
     * The number of jsr's is typically quite small so the above conservative
     * estimate is really not that bad.
     *
     * Allocate memory to hold all that.  */
    memSize = 
	/* Basic blocks */
	nBasicBlocks * sizeof(CVMBasicBlock) +
	/* Basic block successors */
	nSuccessors * sizeof(CVMBasicBlock*) +
	/* Exception mappings */
	nExceptionHandlers * sizeof(CVMBasicBlock*) +
	/* Jsr tables */
	nJsrs * sizeof(CVMJsrTableEntry) +
	nJsrs * nJsrs * sizeof(CVMBasicBlock*) +
	/* Basic block stack and var states with
	   an additional one for the context state */
	(nBasicBlocks + 1) * con->stateSize * sizeof(CVMCellTypeState) +
	/* space for recording conflict uses of variables */
	con->nVars * sizeof(CVMCellTypeState) +
	/* printBuffer size for the state */
	(con->stateSize + 2) * sizeof(char);

    bbAndSuccessorsArea = calloc(1, memSize);

    if (bbAndSuccessorsArea == 0) {
	throwError(con, CVM_MAP_OUT_OF_MEMORY);
    }
    basicBlocks = (CVMBasicBlock*)bbAndSuccessorsArea;

    con->basicBlocks  = basicBlocks;
    con->nBasicBlocks = nBasicBlocks;
    successorsArea = (CVMBasicBlock**)(basicBlocks + nBasicBlocks);
    con->successorsArea = successorsArea;
    exceptionsArea = successorsArea + nSuccessors;
    con->exceptionsArea = exceptionsArea;

    /*
     * Get the JSR table ready
     */
    con->jsrTable  = (CVMJsrTableEntry*)(exceptionsArea + nExceptionHandlers);
    jsrCallersArea = (CVMBasicBlock**)(con->jsrTable + nJsrs);
    con->jsrTableSize = 0;

    /*
     * This is the area for the basic block variable and stack states
     */
    stateArea = (CVMCellTypeState*)(jsrCallersArea + nJsrs * nJsrs);

    /*
     * There is need for code re-writing only if the code contains 
     * a jsr instruction.
     */
    if (nJsrs > 0) {
	con->mayNeedRewriting = CVM_TRUE;
    }

    /*
     * Now make a second pass over the code. This time make up the actual
     * basic blocks, and set successors. Make sure the basic blocks are
     * ordered by PC.
     *
     * Set successors by PC initially. We will then make a separate pass
     * over successors, and map each PC to basic block.
     */
    pc = codeBegin;
    currentBasicBlock = 0; /* Just to make sure */

    while (pc < codeEnd) {
	CVMOpcode instr = (CVMOpcode)*pc;
	CVMUint32 instrLen = getOpcodeLength(con, pc);
	/*
	 * Assign basic block structures to all basic blocks.
	 */
	if (CVM_MAP_IS_BB_HDR(pc - codeBegin)) {
	    currentBasicBlock = basicBlocks++;
	    CVM_MAP_SET_BB_FOR_PC(pc - codeBegin, currentBasicBlock);
	    currentBasicBlock->startPC = pc;
	    currentBasicBlock->varState = stateArea;
	    currentBasicBlock->stackState = stateArea + con->nVars;
	    stateArea += con->stateSize;
	    /* This is the part that needs to be re-done on each
	       full abstract interpretation */
	    currentBasicBlock->topOfStack = -1; /* un-interpreted */
	    currentBasicBlock->successors = NULL;
	    currentBasicBlock->changed = CVM_FALSE;
	}
	if (CVMbcAttr(instr, BRANCH)) {
	    /* Assert that we are not overriding a previously set
	       successors pointer */
	    CVMassert(currentBasicBlock->successors == NULL);
	    currentBasicBlock->successors = successorsArea;
	    /*
	     * Set successors for each of the possible branches.
	     * This is possible for all but jsr. Jsr's can only be 'succeeded'
	     * at abstract interpretation time, since only then can we know
	     * which jsr target a given 'ret <n>' corresponds to.
	     *
	     * We either have a goto or jsr, a tableswitch, a lookupswitch,
	     * or one of the if's.
	     */
	    switch(instr) {
	        case opc_goto:
	        case opc_jsr: {
		    /* An unconditional goto, 2-byte offset */
		    CVMInt16 offset = CVMgetInt16(pc+1);
                    /* 
                     * The one and only use for 'codeOffset' is to
                     * store a pointer difference. Even if we know
                     * that they will always fit into 16 bits I can
                     * see no sense in using a 16 bit variable since
                     * the code will never be slower (but often
                     * faster) if we use a variable matching the
                     * machine size instead.  */
		    CVMAddr codeOffset = pc + offset - codeBegin;
		    *successorsArea++ = (CVMBasicBlock*)codeOffset;
		    if (instr == opc_jsr) {
			CVMstackmapAddJsrCall(con, (CVMUint16)(codeOffset),
					      pc + instrLen - codeBegin,
					      &jsrCallersArea, nJsrs);
			currentBasicBlock->endsWithJsr = CVM_TRUE;
		    }
		    break;
		}
	        case opc_goto_w:
	        case opc_jsr_w: {
		    /* An unconditional goto, 4-byte offset */
		    CVMInt32 offset = CVMgetInt32(pc+1);
                    /* 
                     * The one and only use for 'codeOffset' is to
                     * store a pointer difference. Even if we know
                     * that they will always fit into 32 bits I can
                     * see no sense in using a 32 bit variable since
                     * the code will never be slower (but often
                     * faster) if we use a variable matching the
                     * machine size instead.  */
                    CVMAddr codeOffset = pc + offset - codeBegin;
		    *successorsArea++ = (CVMBasicBlock*)codeOffset;
		    if (instr == opc_jsr_w) {
			CVMstackmapAddJsrCall(con, (CVMUint16)codeOffset,
					      pc + instrLen - codeBegin,
					      &jsrCallersArea, nJsrs);
			currentBasicBlock->endsWithJsr = CVM_TRUE;
		    }
		    break;
	        }
	        case opc_lookupswitch: {
		    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(pc+1);
		    CVMInt32  skip = CVMgetAlignedInt32(lpc); /* default */
		    CVMInt32  npairs = CVMgetAlignedInt32(&lpc[1]);
		    /* First mark the default */
                    /* 
                     * The one and only use for 'codeOffset' is to
                     * store pointer differences. Even if we know that
                     * they will always fit into 16 bits I can see no
                     * sense in using a 16 bit variable since the code
                     * will never be slower (but often faster) if we
                     * use a variable matching the machine size
                     * instead.  */
		    CVMAddr codeOffset = pc + skip - codeBegin;
		    *successorsArea++ = (CVMBasicBlock*)codeOffset;
		    /* And all the possible case arms */
		    lpc += 3; /* Go to the first offset */
		    while (--npairs >= 0) {
			skip = CVMgetAlignedInt32(lpc);
			codeOffset = pc + skip - codeBegin;
			*successorsArea++ = (CVMBasicBlock*)codeOffset;
			lpc += 2; /* next offset */
		    }
		    break;
	        }
	        case opc_tableswitch: {
		    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(pc+1);
		    CVMInt32  skip = CVMgetAlignedInt32(lpc); /* default */
		    CVMInt32  low  = CVMgetAlignedInt32(&lpc[1]);
		    CVMInt32  high = CVMgetAlignedInt32(&lpc[2]);
		    CVMInt32  noff = high - low + 1;
		    /* First mark the default */
                    /* 
                     * The one and only use for 'codeOffset' is to
                     * store pointer differences. Even if we know that
                     * they will always fit into 16 bits I can see no
                     * sense in using a 16 bit variable since the code
                     * will never be slower (but often faster) if we
                     * use a variable matching the machine size
                     * instead.  */
		    CVMAddr codeOffset = pc + skip - codeBegin;
		    *successorsArea++ = (CVMBasicBlock*)codeOffset;
		    lpc += 3; /* Skip default, low, high */
		    while (--noff >= 0) {
			skip = CVMgetAlignedInt32(lpc);
			codeOffset = pc + skip - codeBegin;
			*successorsArea++ = (CVMBasicBlock*)codeOffset;
			lpc++;
		    }
		    break;
	        }
	        default: {
                    /* 
                     * The one and only use for 'codeOffset' is to
                     * store pointer differences. Even if we know that
                     * they will always fit into 16 bits I can see no
                     * sense in using a 16 bit variable since the code
                     * will never be slower (but often faster) if we
                     * use a variable matching the machine size
                     * instead.  */
		    CVMAddr codeOffset;
		    /* This had better be one of the 'if' guys */
		    CVMassert(((instr >= opc_ifeq) &&
			       (instr <= opc_if_acmpne)) ||
			      (instr == opc_ifnull) ||
			      (instr == opc_ifnonnull));
		    CVMassert(instrLen == 3);
		    /* Mark the target of the 'if' */
		    codeOffset = pc + CVMgetInt16(pc+1) - codeBegin;
		    *successorsArea++ = (CVMBasicBlock*)codeOffset;
		    /* And mark the following instruction */
		    codeOffset = pc + 3 - codeBegin;
		    *successorsArea++ = (CVMBasicBlock*)codeOffset;
		}
	    }
	    /* We'll convert these shortly */
	    *successorsArea++ = (CVMBasicBlock*)~0; 
	} else if ((pc + instrLen < codeEnd) &&
		   CVM_MAP_IS_BB_HDR(pc + instrLen - codeBegin)) {
	    /*
	     * If we are the last instruction of a basic block, and
	     * our instruction is not a branch, and it is not a
	     * NOCONTROLFLOW one, then we'd better express the
	     * 'fall-through' in the successors array.
	     */
	    if (!CVMbcAttr(instr, NOCONTROLFLOW) &&
		!((instr == opc_wide) && (pc[1] == opc_ret))) {
		CVMassert(pc + instrLen < codeEnd); /* can't fall off !! */
		currentBasicBlock->successors = successorsArea;
		*successorsArea++ =
		    (CVMBasicBlock*)(pc + instrLen - codeBegin);
		/* We'll convert these shortly */
		*successorsArea++ = (CVMBasicBlock*)~0; 
	    }
	}
	pc += instrLen;
    }
    /*
     * The last state goes to the context
     */
    con->varState = stateArea;
    con->stackState = stateArea + con->nVars;

    /*
     * Then come the conflict uses of variables
     */
    con->refVarsToInitialize = stateArea + con->stateSize;
    /*
     * The printing buffer comes right afterwards
     */
    con->printBuffer = (char*)(con->refVarsToInitialize + con->nVars);

    successorsAreaEnd = successorsArea;
    successorsArea    = con->successorsArea;
    /* 
     * Make pass over the successors array, and map 16-bit pc's
     * to basic block pointers.
     */
    while(successorsArea < successorsAreaEnd) {
	CVMAddr codeOffset = (CVMAddr)(*successorsArea);
	if (codeOffset == ~0) {
	    *successorsArea++ = 0;
	} else {
	    CVMBasicBlock* bb = CVM_MAP_GET_BB_FOR_PC(codeOffset);
	    CVMassert((CVMAddr)bb > 1);
	    *successorsArea++ = bb;
	}
    }
    /*
     * Do a fast mapping for exception handlers.
     */
    if (nExceptionHandlers > 0) {
	CVMExceptionHandler* excTab = CVMmbExceptionTable(con->mb);
	CVMInt32 nEntries = nExceptionHandlers;
	while(--nEntries >= 0) {
	    CVMBasicBlock* bb = CVM_MAP_GET_BB_FOR_PC(excTab->handlerpc);
	    CVMassert((CVMAddr)bb > 1);
	    *exceptionsArea++ = bb;
	    excTab++;
	    /* all exception entries will be considered as live */
	    CVMstackmapLivenessPush(con, bb);
	}
    }

    /* and of course the entry point is live */
    CVMstackmapLivenessPush(con, con->basicBlocks);

    /*
     * And for the jsr return table as well.
     */
    if (nJsrs > 0) {
	int i;
	for (i = 0; i < con->jsrTableSize; i++) {
	    CVMBasicBlock** callers  = con->jsrTable[i].jsrCallers;
	    CVMInt32        ncallers = con->jsrTable[i].jsrNoCalls;
	    while(--ncallers >= 0) {
		/* relPC must be able to hold a native pointer
		 * because *callers is of type CVMBasicBlock*
		 * therefore the type has to be CVMAddr which is 4 byte on
		 * 32 bit platforms and 8 byte on 64 bit platforms
		 */
		CVMAddr relPC = (CVMAddr)*callers;
		CVMBasicBlock* bb = CVM_MAP_GET_BB_FOR_PC(relPC);
		CVMassert((CVMAddr)bb > 1);
		*callers++ = bb;
	    }
	}
    }

    /*
     * A duplicate entry for the method entry map. We might use it in case
     * we do code re-writing for ref-uninit conflicts.
     */
    if (con->mayNeedRewriting) {
	con->nGCPoints++;
    }

#ifdef CVM_DEBUG
    /*
     * Print stats on what we saw in this method 
     */
    if ( CVMstackmapVerboseDebug ){
	CVMtraceStackmaps(("\tCode size          = %d\n", con->codeLen));
	CVMtraceStackmaps(("\tNo of locals       = %d\n", con->nVars));
	CVMtraceStackmaps(("\tMax stack          = %d\n", con->maxStack));
	CVMtraceStackmaps(("\tNo of basic blocks = %d\n", nBasicBlocks));
	CVMtraceStackmaps(("\tNo of invocations  = %d\n", nInvocations));
	CVMtraceStackmaps(("\tNo of GC points    = %d\n", con->nGCPoints));
	CVMtraceStackmaps(("\tNo of JSR calls    = %d\n", nJsrs));
	CVMtraceStackmaps(("\tNo of JSRtab elems = %d\n", con->jsrTableSize));
	CVMtraceStackmaps(("\tNo of exc handlers = %d\n", nExceptionHandlers));
	CVMtraceStackmaps(("\tSize of mapsArea   = %d\n",
			 (con->codeLen + (con->codeLen + 31) / 32) *
			 sizeof(CVMUint32)));
	CVMtraceStackmaps(("\tSize of other mem  = %d\n", memSize));
    }
#endif

}

/*
 * Convert a cell type-state to a printable character.
 */
#ifdef CVM_DEBUG
static char
CVMstackmapCtsToChar(CVMCellTypeState cts)
{
    CVMCellTypeState flags = CVMctsGetFlags(cts);
    switch(flags) {
        case CVMctsUninit: return 'u';
        case CVMctsRef:    return 'r';
        case CVMctsVal:    return 'v';
        case CVMctsPC:     return 'p';
        case CVMctsBottom: return '@';
        case CVMctsRef | CVMctsVal: 
	    CVMtraceStackmaps(("CFL\n")); return '!';
        case CVMctsRef | CVMctsUninit: 
	    CVMtraceStackmaps(("CFL\n")); return '$';
        case CVMctsRef | CVMctsPC: 
	    CVMtraceStackmaps(("CFL\n")); return '%';
        case CVMctsVal | CVMctsUninit: 
	    CVMtraceStackmaps(("CFL\n")); return '^';
        case CVMctsVal | CVMctsPC: 
	    CVMtraceStackmaps(("CFL\n")); return '&';
        case CVMctsPC | CVMctsUninit: 
	    CVMtraceStackmaps(("CFL\n")); return '*';
        default:           
	    CVMtraceStackmaps(("CFL\n0x%x\n", flags)); return '#';
    }
}
#endif

#ifdef CVM_DEBUG
static void
CVMstackmapPrintState(CVMStackmapContext* con,
		      CVMInt32            topOfStack,
		      CVMCellTypeState*   varState,
		      CVMCellTypeState*   stackState)
{
    CVMInt32 v;
    char* state = con->printBuffer;

    if (!CVMstackmapVerboseDebug ) return;

    for (v = 0; v < con->nVars; v++) {
	state[v] = CVMstackmapCtsToChar(varState[v]);
    }
    state[v] = '\0';
    CVMtraceStackmaps(("Variables %s\n", state));

    for (v = 0; v < topOfStack; v++) {
	state[v] = CVMstackmapCtsToChar(stackState[v]);
    } 
    state[v] = '\0';
    CVMtraceStackmaps(("Stack %s\n", state));
    CVMtraceStackmaps(("Top of stack %d\n", topOfStack));
}

/*
 * Print the entry state of a basic block in human-readable form
 */
static void
CVMstackmapPrintBBState(CVMStackmapContext* con, CVMBasicBlock* bb)
{
    CVMstackmapPrintState(con, bb->topOfStack, bb->varState, bb->stackState);
}
#endif

/*
 * Is the field reference of the non-quick field access bytecode at 'pc'
 * a reference or a value.
from quicken.c
....

 */
static void
CVMstackmapFieldKind(CVMStackmapContext* con, CVMUint8* pc,
		     CVMBool* isRef, CVMBool* isDoubleword)
{
    CVMUint32      cpIdx = CVMgetUint16(pc+1);
    CVMFieldTypeID fbTid;
#ifdef CVM_DEBUG_ASSERTS
    {
        CVMOpcode instr = (CVMOpcode)*pc;
#ifdef CVM_JVMTI
        if (instr == opc_breakpoint) {
	    /* Must get the original byte-code: */
	    instr = CVMjvmtiGetBreakpointOpcode(con->ee, pc, CVM_FALSE);
	}
#endif
        CVMassert(instr == opc_getfield || instr == opc_getstatic ||
                  instr == opc_putfield || instr == opc_putstatic ||
                  instr == opc_putfield_quick_w ||
                  instr == opc_getfield_quick_w);
    }
#endif
    /*
     * Get the typeID of the field at cpIdx.
     */
#ifdef CVM_CLASSLOADING
    if (CVMcpIsResolved(con->cp, cpIdx)) {
    resolved:
#endif
	fbTid = CVMfbNameAndTypeID(CVMcpGetFb(con->cp, cpIdx));
#ifdef CVM_CLASSLOADING
    } else {
	CVMcpLock(con->ee);
	if (CVMcpIsResolved(con->cp, cpIdx)) {
	    CVMcpUnlock(con->ee);
	goto resolved;
	} else {
	    CVMUint16 typeIDIdx = CVMcpGetMemberRefTypeIDIdx(con->cp, cpIdx);
	    fbTid = CVMcpGetFieldTypeID(con->cp, typeIDIdx);
	    CVMcpUnlock(con->ee);
#endif /* CVM_CLASSLOADING */
	}
    }

    *isRef = CVMtypeidFieldIsRef(fbTid);
    *isDoubleword = CVMtypeidFieldIsDoubleword(fbTid);
}

/*
 * Handle method invocation given a methodblock
 */
static CVMInt32
CVMstackmapHandleMethod(CVMStackmapContext* con,
			CVMCellTypeState* stack, 
			CVMInt32 topOfStack,
			CVMBool isStatic,
			CVMMethodBlock* mb)
{
    topOfStack -= CVMmbArgsSize(mb);
    if (isStatic != CVMmbIs(mb, STATIC)) {
	/*
	 * There has been an illegal method call that has confused 
	 * the stackmap generator. See bug #4381392. The method's
	 * argsize is off by one because the invoker and invokee do
	 * no agree on whether or not the method is static, and thus
	 * whether or not there is a "this" argument. If the invoker
	 * thinks the method should be static, then we need to "undo"
	 * the popping of the "this" argument. If the invoker thinks
	 * the method is not static, then we need to pop the "this"
	 * argument because it was not accounted for in CVMmbArgsSize(mb).
	 */
	topOfStack += (isStatic ? 1 : -1);
    }
    switch(CVMtypeidGetReturnType(CVMmbNameAndTypeID(mb))) {
	case CVM_TYPEID_VOID:
	    break;
	case CVM_TYPEID_OBJ:
	    stack[topOfStack++] = CVMctsRef;
	    break;
	case CVM_TYPEID_LONG:
	case CVM_TYPEID_DOUBLE:
	    stack[topOfStack++] = CVMctsVal;
	    stack[topOfStack++] = CVMctsVal;
	    break;
	default:
	    stack[topOfStack++] = CVMctsVal;
    }	
    return topOfStack;
}

/*
 * Handle method invocation given a pc pointing to an invocation
 */
static CVMInt32
CVMstackmapHandleMethodRefFromCode(CVMStackmapContext* con,
				   CVMCellTypeState* stack, 
				   CVMInt32 topOfStack,
				   CVMUint8* pc,
				   CVMBool isStatic)
{
    CVMUint32 cpIdx;

#ifdef CVM_JVMTI
    CVMassert(CVMbcAttr(*pc, INVOCATION) ||
        (((CVMOpcode)*pc == opc_breakpoint) &&
         CVMbcAttr(CVMjvmtiGetBreakpointOpcode(con->ee, pc, CVM_FALSE),
                   INVOCATION)));
#else
    CVMassert(CVMbcAttr(*pc, INVOCATION));
#endif

    cpIdx = CVMgetUint16(pc+1);
    /*
     * The entry may not be resolved already, in which case we have
     * to deal with an CVMMethodTypeID instead of an CVMMethodBlock.
     */
#ifdef CVM_CLASSLOADING
    if (CVMcpIsResolved(con->cp, cpIdx)) {
    resolved:
#endif
	return CVMstackmapHandleMethod(con, stack, topOfStack, isStatic,
				       CVMcpGetMb(con->cp, cpIdx));
#ifdef CVM_CLASSLOADING
    } else {
	CVMcpLock(con->ee);
	if (CVMcpIsResolved(con->cp, cpIdx)) {
	    CVMcpUnlock(con->ee);
	    goto resolved;
	} else {
	    CVMUint16 typeIDIdx = CVMcpGetMemberRefTypeIDIdx(con->cp, cpIdx);
	    CVMMethodTypeID methodTypeID = 
		CVMcpGetMethodTypeID(con->cp, typeIDIdx);
	    CVMcpUnlock(con->ee);
	    
	    /* adjust the stack by the number of arguments.*/
	    topOfStack -= (isStatic ? 0 : 1); /* handle "this" argument */
	    topOfStack -= CVMtypeidGetArgsSize(methodTypeID);
	    
	    /* set the return type */
	    switch(CVMtypeidGetReturnType(methodTypeID)) {
	    case CVM_TYPEID_VOID:
		break;
	    case CVM_TYPEID_OBJ:
		stack[topOfStack++] = CVMctsRef;
		break;
	    case CVM_TYPEID_LONG:
	    case CVM_TYPEID_DOUBLE:
		stack[topOfStack++] = CVMctsVal;
		stack[topOfStack++] = CVMctsVal;
		break;
	    default:
		stack[topOfStack++] = CVMctsVal;
	    }
	    return topOfStack;
	}
    }
#endif  /* CVM_CLASSLOADING */
}

static CVMInt32
CVMstackmapLocalStore(CVMStackmapContext* con,
		      CVMUint32 varNo, CVMInt32 topOfStack,
		      CVMCellTypeState* stack, CVMCellTypeState* vars)
{
    CVMassert(topOfStack >= 1);
    CVMassert(stack[topOfStack - 1] == CVMctsVal);
    vars[varNo] = stack[topOfStack - 1];
    topOfStack--;
    return topOfStack;
}

static CVMInt32
CVMstackmapLocalAStore(CVMStackmapContext* con,
		       CVMUint32 varNo, CVMInt32 topOfStack,
		       CVMCellTypeState* stack, CVMCellTypeState* vars)
{
    CVMassert(topOfStack >= 1);
    CVMassert((stack[topOfStack - 1] == CVMctsRef) ||
	      (CVMctsGetFlags(stack[topOfStack - 1]) == CVMctsPC));
    vars[varNo] = stack[topOfStack - 1];
    topOfStack--;
    return topOfStack;
}

static CVMInt32
CVMstackmapLocalDStore(CVMStackmapContext* con,
		       CVMUint32 varNo, CVMInt32 topOfStack,
		       CVMCellTypeState* stack, CVMCellTypeState* vars)
{
    CVMassert(topOfStack >= 2);
    CVMassert(stack[topOfStack - 1] == CVMctsVal);
    CVMassert(stack[topOfStack - 2] == CVMctsVal);
    vars[(varNo)]     = stack[topOfStack - 2];
    vars[(varNo) + 1] = stack[topOfStack - 1];
    topOfStack -= 2;
    return topOfStack;
}

static CVMInt32
CVMstackmapLocalLoad(CVMStackmapContext* con,
		     CVMUint32 varNo, CVMInt32 topOfStack,
		     CVMCellTypeState* stack, CVMCellTypeState* vars)
{
    CVMassert(topOfStack < con->maxStack);
    /*
     * We don't care if we encounter a conflict here. We are only
     * worried about using a conflict as a ref, which is handled in
     * CVMstackmapLocalALoad().
     */
    stack[topOfStack++] = CVMctsVal;
    return topOfStack;
}

static CVMInt32
CVMstackmapLocalALoad(CVMStackmapContext* con, CVMUint8* pc,
		      CVMUint32 varNo, CVMInt32 topOfStack,
		      CVMCellTypeState* stack, CVMCellTypeState* vars)
{
    CVMCellTypeState theVar = vars[varNo];
    CVMassert(topOfStack < con->maxStack);

    if (theVar != CVMctsRef) {
	/*
	 * Conflicts had better not occur unless we have pre-determined
	 * that they are possible.
	 */
	CVMassert(con->mayNeedRewriting);
	if (CVMctsIsUninit(theVar)) {
	    CVMtraceStackmaps(("ALOAD(%d) at relPC=%d: "
			       "Ref-uninit conflict (%C.%M)\n",
			       varNo, pc - con->code, con->cb, con->mb));
	    /* 
	     * Record the use of this conflict, along with its type. We'll
	     * check this use during map generation.
	     */
	    if (con->refVarsToInitialize[varNo] == 0) {
		/*
		 * We have not made a record of this variable yet
		 */
		con->refVarsToInitialize[varNo] = theVar;
		con->numRefVarsToInitialize++;
	    }
	} else if (CVMctsIsPC(theVar)) {
	    CVMtraceStackmaps(("ALOAD(%d) at relPC=%d: "
			       "Ref-pc conflict (%C.%M)\n",
			       varNo, pc - con->code, con->cb, con->mb));
	    con->nRefPCConflicts++;
	    remapConflictVarno(con, varNo);
	} else {
	    CVMassert(CVMctsIsVal(theVar));
	    CVMtraceStackmaps(("ALOAD(%d) at relPC=%d: "
			       "Ref-val conflict (%C.%M)\n",
			       varNo, pc - con->code, con->cb, con->mb));
	    con->nRefValConflicts++;
	    remapConflictVarno(con, varNo);
	}
    }
    stack[topOfStack++] = CVMctsRef;
    return topOfStack;
}

static CVMInt32
CVMstackmapLocalDLoad(CVMStackmapContext* con,
		      CVMUint32 varNo, CVMInt32 topOfStack,
		      CVMCellTypeState* stack, CVMCellTypeState* vars)
{
    CVMassert(topOfStack - 1 < con->maxStack);
    /*
     * We don't care if we encounter a conflict here. We are only
     * worried about using a conflict as a ref, which is handled in
     * CVMstackmapLocalALoad().
     */
    stack[topOfStack++] = CVMctsVal;
    stack[topOfStack++] = CVMctsVal;
    return topOfStack;
}

/*
 * Interpret one opcode. Return updated topOfStack. Don't do anything
 * for opcodes that don't affect state (variables, stack).  */
static CVMInt32
CVMstackmapInterpretOne(CVMStackmapContext* con,
			CVMUint8* pc, CVMCellTypeState* vars,
			CVMCellTypeState* stack, CVMInt32 topOfStack)
{
    CVMOpcode instr = (CVMOpcode)*pc;
    CVMassert(topOfStack >= 0);
#ifdef CVM_JVMTI
    /* Just in case we have to re-do the instruction due to a breakpoint */
 interpretInstr:
#endif
    switch(instr) {
        case opc_nop:
        case opc_goto:
        case opc_goto_w:
        case opc_iinc:
        case opc_return:
        case opc_ret: /* hairy ret handling outside of here */
	    /* nothing */
	    break;
        case opc_iload:
        case opc_fload:
	    topOfStack =
		CVMstackmapLocalLoad(con, pc[1], topOfStack, stack, vars);
	    break;
        case opc_lload:
        case opc_dload:
	    topOfStack =
		CVMstackmapLocalDLoad(con, pc[1], topOfStack, stack, vars);
	    break;
        case opc_aload:          
	    topOfStack =
		CVMstackmapLocalALoad(con, pc, pc[1], topOfStack, stack, vars);
	    break;
        case opc_iload_0:
        case opc_fload_0:
	    topOfStack =
		CVMstackmapLocalLoad(con, 0, topOfStack, stack, vars);
	    break;
        case opc_iload_1:
        case opc_fload_1:
	    topOfStack =
		CVMstackmapLocalLoad(con, 1, topOfStack, stack, vars);
	    break;
        case opc_iload_2:
        case opc_fload_2:
	    topOfStack =
		CVMstackmapLocalLoad(con, 2, topOfStack, stack, vars);
	    break;
        case opc_iload_3:
        case opc_fload_3:
	    topOfStack =
		CVMstackmapLocalLoad(con, 3, topOfStack, stack, vars);
	    break;
        case opc_lload_0:
        case opc_dload_0:
	    topOfStack =
		CVMstackmapLocalDLoad(con, 0, topOfStack, stack, vars);
	    break;
        case opc_lload_1:
        case opc_dload_1:
	    topOfStack =
		CVMstackmapLocalDLoad(con, 1, topOfStack, stack, vars);
	    break;
        case opc_lload_2:
        case opc_dload_2:
	    topOfStack =
		CVMstackmapLocalDLoad(con, 2, topOfStack, stack, vars);
	    break;
        case opc_lload_3:
        case opc_dload_3:
	    topOfStack =
		CVMstackmapLocalDLoad(con, 3, topOfStack, stack, vars);
	    break;
        case opc_aload_0:
	    topOfStack =
		CVMstackmapLocalALoad(con, pc, 0, topOfStack, stack, vars);
	    break;
        case opc_aload_1:
	    topOfStack =
		CVMstackmapLocalALoad(con, pc, 1, topOfStack, stack, vars);
	    break;
        case opc_aload_2:
	    topOfStack =
		CVMstackmapLocalALoad(con, pc, 2, topOfStack, stack, vars);
	    break;
        case opc_aload_3:
	    topOfStack =
		CVMstackmapLocalALoad(con, pc, 3, topOfStack, stack, vars);
	    break;
        case opc_aconst_null:
#ifdef CVM_CLASSLOADING
        case opc_new:
#endif /* CVM_CLASSLOADING */
        case opc_aldc_quick:
        case opc_aldc_w_quick:
        case opc_aldc_ind_quick:
        case opc_aldc_ind_w_quick:
        case opc_agetstatic_quick:
        case opc_agetstatic_checkinit_quick:
        case opc_new_quick:
        case opc_new_checkinit_quick:
	    /* ... -> Ref,... */
	    CVMassert(topOfStack < con->maxStack);
	    stack[topOfStack++] = CVMctsRef;
	    break;
        case opc_iconst_m1:
        case opc_iconst_0:
        case opc_iconst_1:
        case opc_iconst_2:
        case opc_iconst_3:
        case opc_iconst_4:
        case opc_iconst_5:
        case opc_fconst_0:
        case opc_fconst_1:
        case opc_fconst_2:
        case opc_bipush:
        case opc_sipush:
        case opc_getstatic_quick:
        case opc_getstatic_checkinit_quick:
        case opc_ldc_quick:
        case opc_ldc_w_quick:
	    /* ... -> Val,... */
	    CVMassert(topOfStack < con->maxStack);
	    stack[topOfStack++] = CVMctsVal;
	    break;
        case opc_lconst_0:
        case opc_lconst_1:
        case opc_dconst_0:
        case opc_dconst_1:
#ifdef CVM_CLASSLOADING
        case opc_ldc2_w:
#endif /* CVM_CLASSLOADING */
        case opc_ldc2_w_quick:
        case opc_getstatic2_quick:
        case opc_getstatic2_checkinit_quick:
	    /* ... -> Val,Val,... */
	    CVMassert(topOfStack - 1 < (CVMInt32)con->maxStack);
	    stack[topOfStack++] = CVMctsVal;
	    stack[topOfStack++] = CVMctsVal;
	    break;
        case opc_getfield2_quick:            
	    /* Ref,... -> Val,Val,... */
	    CVMassert(topOfStack >= 1);
	    CVMassert(topOfStack < con->maxStack);
	    CVMassert(stack[topOfStack - 1] == CVMctsRef);
	    stack[topOfStack - 1] = CVMctsVal;
	    stack[topOfStack]     = CVMctsVal;
	    topOfStack++;
	    break;
        case opc_putfield2_quick:            
	    /* Val,Val,Ref,... -> ... */
	    CVMassert(topOfStack >= 3);
	    CVMassert(stack[topOfStack - 3] == CVMctsRef);
	    CVMassert(stack[topOfStack - 2] == CVMctsVal);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    topOfStack -= 3;
	    break;
        case opc_putfield_quick:            
	    /* Val,Ref,... -> ... */
	    CVMassert(topOfStack >= 2);
	    CVMassert(stack[topOfStack - 2] == CVMctsRef);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    topOfStack -= 2;
	    break;
        case opc_iaload:
        case opc_faload:
        case opc_baload:    
        case opc_caload:
        case opc_saload:
	    /* Val,Ref,... -> Val,... */
	    CVMassert(topOfStack >= 2);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    CVMassert(stack[topOfStack - 2] == CVMctsRef);
	    stack[topOfStack - 2] = CVMctsVal;
	    topOfStack--;
	    break;
        case opc_laload:
        case opc_daload:            
	    /* Val,Ref,... -> Val,Val,... */
	    CVMassert(topOfStack >= 2);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    CVMassert(stack[topOfStack - 2] == CVMctsRef);
	    stack[topOfStack - 2] = CVMctsVal;
	    break;
        case opc_aaload:            
	    /* Val,Ref,... -> Ref,... */
	    CVMassert(topOfStack >= 2);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    CVMassert(stack[topOfStack - 2] == CVMctsRef);
	    topOfStack--;
	    break;

        case opc_istore:
        case opc_fstore:
	    topOfStack =
		CVMstackmapLocalStore(con, pc[1], topOfStack, stack, vars);
	    break;
        case opc_lstore:
        case opc_dstore:
	    topOfStack =
		CVMstackmapLocalDStore(con, pc[1], topOfStack, stack, vars);
	    break;
        case opc_astore:          
	    topOfStack =
		CVMstackmapLocalAStore(con, pc[1], topOfStack, stack, vars);
	    break;
        case opc_istore_0:
        case opc_fstore_0:
	    topOfStack =
		CVMstackmapLocalStore(con, 0, topOfStack, stack, vars);
	    break;
        case opc_istore_1:
        case opc_fstore_1:
	    topOfStack =
		CVMstackmapLocalStore(con, 1, topOfStack, stack, vars);
	    break;
        case opc_istore_2:
        case opc_fstore_2:
	    topOfStack =
		CVMstackmapLocalStore(con, 2, topOfStack, stack, vars);
	    break;
        case opc_istore_3:
        case opc_fstore_3:
	    topOfStack =
		CVMstackmapLocalStore(con, 3, topOfStack, stack, vars);
	    break;
        case opc_lstore_0:
        case opc_dstore_0:
	    topOfStack =
		CVMstackmapLocalDStore(con, 0, topOfStack, stack, vars);
	    break;
        case opc_lstore_1:
        case opc_dstore_1:
	    topOfStack =
		CVMstackmapLocalDStore(con, 1, topOfStack, stack, vars);
	    break;
        case opc_lstore_2:
        case opc_dstore_2:
	    topOfStack =
		CVMstackmapLocalDStore(con, 2, topOfStack, stack, vars);
	    break;
        case opc_lstore_3:
        case opc_dstore_3:
	    topOfStack =
		CVMstackmapLocalDStore(con, 3, topOfStack, stack, vars);
	    break;
        case opc_astore_0:
	    topOfStack =
		CVMstackmapLocalAStore(con, 0, topOfStack, stack, vars);
	    break;
        case opc_astore_1:
	    topOfStack =
		CVMstackmapLocalAStore(con, 1, topOfStack, stack, vars);
	    break;
        case opc_astore_2:
	    topOfStack =
		CVMstackmapLocalAStore(con, 2, topOfStack, stack, vars);
	    break;
        case opc_astore_3:
	    topOfStack =
		CVMstackmapLocalAStore(con, 3, topOfStack, stack, vars);
	    break;

        case opc_iastore:
        case opc_fastore:
        case opc_bastore:
        case opc_castore:
        case opc_sastore:
	    CVMassert(topOfStack >= 3);
	    CVMassert(stack[topOfStack - 3] == CVMctsRef);
	    CVMassert(stack[topOfStack - 2] == CVMctsVal);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    topOfStack -= 3;
	    break;
        case opc_lastore:
        case opc_dastore:
	    CVMassert(topOfStack >= 4);
	    CVMassert(stack[topOfStack - 4] == CVMctsRef);
	    CVMassert(stack[topOfStack - 3] == CVMctsVal);
	    CVMassert(stack[topOfStack - 2] == CVMctsVal);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    topOfStack -= 4;
	    break;
        case opc_aastore:
	    CVMassert(topOfStack >= 3);
	    CVMassert(stack[topOfStack - 3] == CVMctsRef);
	    CVMassert(stack[topOfStack - 2] == CVMctsVal);
	    CVMassert(stack[topOfStack - 1] == CVMctsRef);
	    topOfStack -= 3;
	    break;
        case opc_pop:
	    CVMassert(topOfStack >= 1);
	    topOfStack -= 1;
	    break;
        case opc_pop2:
	    CVMassert(topOfStack >= 2);
	    topOfStack -= 2;
	    break;
        case opc_dup:
	    CVMassert(topOfStack < con->maxStack);
	    CVMassert(topOfStack >= 1);
	    stack[topOfStack] = stack[topOfStack - 1];
	    topOfStack++;
	    break;
        case opc_dup2:
	    CVMassert(topOfStack + 1 < con->maxStack);
	    CVMassert(topOfStack >= 2);
	    stack[topOfStack]     = stack[topOfStack - 2];
	    stack[topOfStack + 1] = stack[topOfStack - 1];
	    topOfStack += 2;
	    break;
        case opc_dup_x1:
	    CVMassert(topOfStack < con->maxStack);
	    CVMassert(topOfStack >= 2);
	    stack[topOfStack]     = stack[topOfStack - 1];
	    stack[topOfStack - 1] = stack[topOfStack - 2];
	    stack[topOfStack - 2] = stack[topOfStack];
	    topOfStack++;
	    break;
        case opc_dup_x2:
	    CVMassert(topOfStack < con->maxStack);
	    CVMassert(topOfStack >= 3);
	    stack[topOfStack]     = stack[topOfStack - 1];
	    stack[topOfStack - 1] = stack[topOfStack - 2];
	    stack[topOfStack - 2] = stack[topOfStack - 3];
	    stack[topOfStack - 3] = stack[topOfStack];
	    topOfStack++;
	    break;
        case opc_dup2_x1:
	    CVMassert(topOfStack + 1 < con->maxStack);
	    CVMassert(topOfStack >= 3);
	    stack[topOfStack + 1] = stack[topOfStack - 1];
	    stack[topOfStack]     = stack[topOfStack - 2];
	    stack[topOfStack - 1] = stack[topOfStack - 3];
	    stack[topOfStack - 2] = stack[topOfStack + 1];
	    stack[topOfStack - 3] = stack[topOfStack];
	    topOfStack += 2;
	    break;
        case opc_dup2_x2:
	    CVMassert(topOfStack + 1 < con->maxStack);
	    CVMassert(topOfStack >= 4);
	    stack[topOfStack + 1] = stack[topOfStack - 1];
	    stack[topOfStack]     = stack[topOfStack - 2];
	    stack[topOfStack - 1] = stack[topOfStack - 3];
	    stack[topOfStack - 2] = stack[topOfStack - 4];
	    stack[topOfStack - 3] = stack[topOfStack + 1];
	    stack[topOfStack - 4] = stack[topOfStack];
	    topOfStack += 2;
	    break;
        case opc_swap:
	    {
		CVMCellTypeState temp;
		CVMassert(topOfStack >= 2);
		temp = stack[topOfStack - 1];
		stack[topOfStack - 1] = stack[topOfStack - 2];
		stack[topOfStack - 2] = temp;
		break;
	    }
        case opc_iadd:
        case opc_fadd:
        case opc_isub:
        case opc_fsub:
        case opc_imul:
        case opc_fmul:
        case opc_idiv:
        case opc_fdiv:
        case opc_irem:
        case opc_frem:
        case opc_ishl:
        case opc_ishr:
        case opc_iushr:
        case opc_iand:
        case opc_ior:
        case opc_ixor:
        case opc_l2f:
        case opc_l2i:
        case opc_d2f:
        case opc_d2i:
        case opc_fcmpl:
        case opc_fcmpg:
	    /* Val,Val,... -> Val,... */
	    CVMassert(topOfStack >= 2);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    CVMassert(stack[topOfStack - 2] == CVMctsVal);
	    topOfStack--;
	    break;
        case opc_ladd:
        case opc_dadd:
        case opc_lsub:
        case opc_dsub:
        case opc_lmul:
        case opc_dmul:
        case opc_ldiv:
        case opc_ddiv:
        case opc_lrem:
        case opc_drem:
        case opc_land:
        case opc_lor:
        case opc_lxor:
	    /* Val,Val,Val,Val,... -> Val,Val,... */
	    CVMassert(topOfStack >= 4);
	    CVMassert(stack[topOfStack - 4] == CVMctsVal);
	    CVMassert(stack[topOfStack - 3] == CVMctsVal);
	    CVMassert(stack[topOfStack - 2] == CVMctsVal);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    topOfStack -= 2;
	    break;
        case opc_ineg:
        case opc_fneg:
        case opc_i2f:
        case opc_f2i:
        case opc_i2c:
        case opc_i2s:
        case opc_i2b:
	    /* Val,... -> Val,... */
	    CVMassert(topOfStack >= 1);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    break;
        case opc_lneg:
        case opc_dneg:
        case opc_l2d:
        case opc_d2l:
	    /* Val,Val,... -> Val,Val,... */
	    CVMassert(topOfStack >= 2);
	    CVMassert(stack[topOfStack - 2] == CVMctsVal);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    break;
        case opc_lshl:
        case opc_lshr:
        case opc_lushr:
	    /* Val,Val,Val,... -> Val,Val,... */
	    CVMassert(topOfStack >= 3);
	    CVMassert(stack[topOfStack - 3] == CVMctsVal);
	    CVMassert(stack[topOfStack - 2] == CVMctsVal);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    topOfStack--;
	    break;
        case opc_i2l:
        case opc_i2d:
        case opc_f2l:
        case opc_f2d:
	    /* Val,... -> Val,Val,... */
	    CVMassert(topOfStack < con->maxStack);
	    CVMassert(topOfStack >= 1);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    stack[topOfStack++] = CVMctsVal;
	    break;
        case opc_lcmp:
        case opc_dcmpl:
        case opc_dcmpg:
	    /* Val,Val,Val,Val,... -> Val,... */
	    CVMassert(topOfStack >= 4);
	    CVMassert(stack[topOfStack - 4] == CVMctsVal);
	    CVMassert(stack[topOfStack - 3] == CVMctsVal);
	    CVMassert(stack[topOfStack - 2] == CVMctsVal);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    topOfStack -= 3;
	    break;
        case opc_ifeq:
        case opc_ifne:
        case opc_iflt:
        case opc_ifge:
        case opc_ifgt:
        case opc_ifle:
        case opc_tableswitch:
        case opc_lookupswitch:
        case opc_ireturn:
        case opc_freturn:
        case opc_putstatic_quick:
        case opc_putstatic_checkinit_quick:
	    /* Val,... -> ... */
	    CVMassert(topOfStack >= 1);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    topOfStack--;
	    break;
        case opc_if_icmpeq:
        case opc_if_icmpne:
        case opc_if_icmplt:
        case opc_if_icmpge:
        case opc_if_icmpgt:
        case opc_if_icmple:
        case opc_lreturn:
        case opc_dreturn:
        case opc_putstatic2_quick:
        case opc_putstatic2_checkinit_quick:
	    /* Val,Val,... -> ... */
	    CVMassert(topOfStack >= 2);
	    CVMassert(stack[topOfStack - 2] == CVMctsVal);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    topOfStack -= 2;
	    break;
        case opc_if_acmpeq:
        case opc_if_acmpne:
        case opc_aputfield_quick:
	    /* Ref,Ref,... -> ... */
	    CVMassert(topOfStack >= 2);
	    CVMassert(stack[topOfStack - 2] == CVMctsRef);
	    CVMassert(stack[topOfStack - 1] == CVMctsRef);
	    topOfStack -= 2;
	    break;
        case opc_jsr:
	    CVMassert(topOfStack < con->maxStack);
	    /*
	     * Remember the target. This value will eventually reach a 'ret'
	     * instruction, be looked up in the jsrTable, and cause propagation
	     * of state to all instructions that can follow JSR's to that
	     * target.
	     */
	    stack[topOfStack++] =
		CVMctsMakePC(pc + CVMgetInt16(pc+1) - con->code);
	    break;
        case opc_jsr_w:
	    CVMassert(topOfStack < con->maxStack);
	    stack[topOfStack++] =
		CVMctsMakePC(pc + CVMgetInt32(pc+1) - con->code);
	    break;
#ifdef CVM_CLASSLOADING
        case opc_anewarray:
#endif /* CVM_CLASSLOADING */	    
        case opc_newarray:
        case opc_anewarray_quick:
	    /* Val,... -> Ref,... */
	    CVMassert(topOfStack >= 1);
	    CVMassert(stack[topOfStack - 1] == CVMctsVal);
	    stack[topOfStack - 1] = CVMctsRef;
	    break;
#ifdef CVM_CLASSLOADING
        case opc_checkcast:
#endif /* CVM_CLASSLOADING */
        case opc_checkcast_quick:
        case opc_agetfield_quick:
	    /* Ref,... -> Ref,... */
	    CVMassert(topOfStack >= 1);
	    CVMassert(stack[topOfStack - 1] == CVMctsRef);
	    break;
#ifdef CVM_CLASSLOADING
        case opc_instanceof:
#endif /* CVM_CLASSLOADING */
        case opc_arraylength:
        case opc_instanceof_quick:
        case opc_getfield_quick:
	    /* Ref,... -> Val,... */
	    CVMassert(topOfStack >= 1);
	    CVMassert(stack[topOfStack - 1] == CVMctsRef);
	    stack[topOfStack - 1] = CVMctsVal;
	    break;
        case opc_athrow:
        case opc_areturn:
        case opc_monitorenter:  
        case opc_monitorexit:
        case opc_ifnull:   
        case opc_ifnonnull:
        case opc_nonnull_quick:
        case opc_aputstatic_quick:
        case opc_aputstatic_checkinit_quick:
	    /* Ref,... -> ... */
	    CVMassert(topOfStack >= 1);
	    CVMassert(stack[topOfStack - 1] == CVMctsRef);
	    topOfStack--;
	    break;
#ifdef CVM_CLASSLOADING
        case opc_multianewarray:
#endif /* CVM_CLASSLOADING */
        case opc_multianewarray_quick:
	    CVMassert(topOfStack >= pc[3]);
#ifdef CVM_DEBUG
	    {
		int i;
		for (i = 1; i <= pc[3]; i++) {
		    CVMassert(stack[topOfStack - i] == CVMctsVal);
		}
	    }
#endif
	    topOfStack -= pc[3] - 1; /* Discard all but one of the dim.s */
	    stack[topOfStack - 1] = CVMctsRef;
	    break;
        /*
	 * Handle the wide version
	 */
    case opc_wide: 
	{
	    CVMOpcode instrWide;
	    pc += 1;
	    instrWide = (CVMOpcode)*pc;
	    CVMassert(topOfStack >= 0);
	    switch(instrWide) {
	        case opc_iload:
	        case opc_fload:
		    topOfStack = CVMstackmapLocalLoad(con,
						      CVMgetUint16(pc+1),
						      topOfStack,
						      stack, vars);
		    break;
                case opc_aload:
		    topOfStack = CVMstackmapLocalALoad(con, pc,
						       CVMgetUint16(pc+1),
						       topOfStack,
						       stack, vars);
		    break;
                case opc_lload:
                case opc_dload:
		    topOfStack = CVMstackmapLocalDLoad(con,
						       CVMgetUint16(pc+1),
						       topOfStack,
						       stack, vars);
		    break;
	        case opc_istore:
                case opc_fstore:
		    topOfStack = CVMstackmapLocalStore(con,
						       CVMgetUint16(pc+1),
						       topOfStack,
						       stack, vars);
		    break;
	        case opc_lstore:
                case opc_dstore:
		    topOfStack = CVMstackmapLocalDStore(con,
							CVMgetUint16(pc+1),
							topOfStack,
							stack, vars);
		    break;
                case opc_astore:          
		    topOfStack = CVMstackmapLocalAStore(con,
							CVMgetUint16(pc+1),
							topOfStack,
							stack, vars);
		    break;
                case opc_ret: /* hairy ret handling outside of here */
                case opc_iinc:
		    break;
	    default:
		CVMassert(CVM_FALSE); /* Unknown wide instruction */
		return topOfStack;
	    }
	    break; /* Done with opc_wide */
	}
#ifdef CVM_CLASSLOADING
        case opc_getfield:
#endif /* CVM_CLASSLOADING */
        case opc_getfield_quick_w: {
	    CVMBool isRef, isDoubleword;
	    CVMassert(topOfStack >= 1);
	    CVMassert(stack[topOfStack - 1] == CVMctsRef);
	    CVMstackmapFieldKind(con, pc, &isRef, &isDoubleword);
	    if (!isRef) {
		stack[topOfStack - 1] = CVMctsVal; /* replace */
		if (isDoubleword) {
		    CVMassert(topOfStack < con->maxStack);
		    stack[topOfStack++] = CVMctsVal;
		}
	    }
	    break;
	}
#ifdef CVM_CLASSLOADING
       case opc_getstatic: {
	    CVMBool isRef, isDoubleword;
	    CVMassert(topOfStack < con->maxStack);
	    CVMstackmapFieldKind(con, pc, &isRef, &isDoubleword);
	    if (isRef) {
		stack[topOfStack++] = CVMctsRef;
	    } else {
		stack[topOfStack++] = CVMctsVal;
		if (isDoubleword) {
		    CVMassert(topOfStack < con->maxStack);
		    stack[topOfStack++] = CVMctsVal;
		}
	    }
	    break;
	}
#endif /* CVM_CLASSLOADING */
#ifdef CVM_CLASSLOADING
        case opc_putfield:
#endif /* CVM_CLASSLOADING */
        case opc_putfield_quick_w: {
	    CVMBool isRef, isDoubleword;
	    CVMstackmapFieldKind(con, pc, &isRef, &isDoubleword);
	    if (isDoubleword) {
		CVMassert(topOfStack >= 3);
		CVMassert(stack[topOfStack - 3] == CVMctsRef);
		CVMassert(stack[topOfStack - 2] == CVMctsVal);
		CVMassert(stack[topOfStack - 1] == CVMctsVal);
		topOfStack -= 3;
	    } else {
		CVMassert(topOfStack >= 2);
		CVMassert(stack[topOfStack - 2] == CVMctsRef);
#ifdef CVM_DEBUG_ASSERTS
		if (isRef) {
		    CVMassert(stack[topOfStack - 1] == CVMctsRef);
		} else {
		    CVMassert(stack[topOfStack - 1] == CVMctsVal);
		}
#endif		    
		topOfStack -= 2;
	    }
	    break;
	}
#ifdef CVM_CLASSLOADING
        case opc_putstatic: {
	    CVMBool isRef, isDoubleword;
	    CVMstackmapFieldKind(con, pc, &isRef, &isDoubleword);
	    if (isDoubleword) {
		CVMassert(topOfStack >= 2);
		CVMassert(stack[topOfStack - 2] == CVMctsVal);
		CVMassert(stack[topOfStack - 1] == CVMctsVal);
		topOfStack -= 2;
	    } else {
		CVMassert(topOfStack >= 1);
#ifdef CVM_DEBUG_ASSERTS
		if (isRef) {
		    CVMassert(stack[topOfStack - 1] == CVMctsRef);
		} else {
		    CVMassert(stack[topOfStack - 1] == CVMctsVal);
		}
#endif		    
		topOfStack -= 1;
	    }
	    break;
	}
#endif /* CVM_CLASSLOADING */
#ifdef CVM_CLASSLOADING
       case opc_ldc:
        case opc_ldc_w: {
	    CVMUint16 cpIdx;
	    if (instr == opc_ldc) {
		cpIdx = pc[1];
	    } else {
		cpIdx = CVMgetUint16(pc+1);
	    }
	    switch (CVMcpEntryType(con->cp, cpIdx)) {
	    case CVM_CONSTANT_StringICell:
	    case CVM_CONSTANT_StringObj:
	    case CVM_CONSTANT_ClassTypeID:
	    case CVM_CONSTANT_ClassBlock:
		/* ... -> Ref,... */
		CVMassert(topOfStack < con->maxStack);
		stack[topOfStack++] = CVMctsRef;
		break;
	    case CVM_CONSTANT_Integer:
	    case CVM_CONSTANT_Float:
		/* ... -> Val,... */
		CVMassert(topOfStack < con->maxStack);
		stack[topOfStack++] = CVMctsVal;
		break;
	    case CVM_CONSTANT_Long:
	    case CVM_CONSTANT_Double:
		/* ... -> Val,Val,... */
		CVMassert(topOfStack - 1 < con->maxStack);
		stack[topOfStack++] = CVMctsVal;
		stack[topOfStack++] = CVMctsVal;
		break;
	    default:
		CVMassert(CVM_FALSE);
	    }
	    break;
	}
#endif /* CVM_CLASSLOADING */	    
#ifdef CVM_CLASSLOADING
        case opc_invokevirtual:
        case opc_invokespecial:
        case opc_invokeinterface:
#endif
        case opc_invokevirtual_quick_w:
        case opc_invokenonvirtual_quick:
        case opc_invokeinterface_quick:
	    topOfStack = CVMstackmapHandleMethodRefFromCode(con, stack,
							    topOfStack, pc,
							    CVM_FALSE);
	    break;
#ifdef CVM_CLASSLOADING
        case opc_invokestatic:
#endif /* CVM_CLASSLOADING */	    
        case opc_invokestatic_quick:
        case opc_invokestatic_checkinit_quick:
	    topOfStack = CVMstackmapHandleMethodRefFromCode(con, stack,
							    topOfStack, pc,
							    CVM_TRUE);
	    break;
        case opc_invokesuper_quick: {
	    CVMUint32 mtIdx = CVMgetUint16(pc+1);
	    CVMClassBlock* sup = CVMcbSuperclass(con->cb);
	    CVMMethodBlock* mb = CVMcbMethodTableSlot(sup, mtIdx);
	    topOfStack = CVMstackmapHandleMethod(con, stack, topOfStack,
						 CVM_FALSE, mb);
	    break;
	}
        case opc_invokevirtualobject_quick: {
	    /*
	     * Get the methodblock from java.lang.Object
	     */
	    CVMUint32 mtIdx = pc[1];
	    const CVMClassBlock* objCB = CVMsystemClass(java_lang_Object);
	    CVMMethodBlock* mb = CVMcbMethodTableSlot(objCB, mtIdx);
	    topOfStack = CVMstackmapHandleMethod(con, stack, topOfStack,
						 CVM_FALSE, mb);
	    break;
	}
        case opc_invokeignored_quick:
	    CVMassert(topOfStack >= pc[1]);
	    topOfStack -= pc[1]; /* Discard args and ignore invocation */
	    break;
        case opc_invokevirtual_quick: /* Non-ref one-word return */
	    CVMassert(topOfStack >= pc[2]);
	    topOfStack -= pc[2]; /* Discard args */
	    stack[topOfStack++] = CVMctsVal;
	    break;
        case opc_ainvokevirtual_quick: /* Ref return */
	    CVMassert(topOfStack >= pc[2]);
	    topOfStack -= pc[2]; /* Discard args */
	    stack[topOfStack++] = CVMctsRef;
	    break;
        case opc_dinvokevirtual_quick: /* Double-word return */
	    CVMassert(topOfStack >= pc[2]);
	    topOfStack -= pc[2]; /* Discard args */
	    stack[topOfStack++] = CVMctsVal;
	    stack[topOfStack++] = CVMctsVal;
	    break;
        case opc_vinvokevirtual_quick: /* Void return */
	    CVMassert(topOfStack >= pc[2]);
	    topOfStack -= pc[2]; /* Discard args */
	    break;

#ifdef CVM_HW
#include "include/hw/stackmaps.i"
	    break;
#endif

        case opc_breakpoint:
#ifdef CVM_JVMTI
	    /* Must get the original byte-code and re-do */
	    instr = CVMjvmtiGetBreakpointOpcode(con->ee, pc, CVM_FALSE);
	    goto interpretInstr;
#endif
        default:
	    CVMassert(CVM_FALSE);
	    break;
    }
    return topOfStack;
}

/*
 * Merge two states
 */
static CVMCellTypeState
CVMstackmapMerge(CVMCellTypeState cts1, CVMCellTypeState cts2)
{
    CVMCellTypeState result;
    if (CVMctsIsPC(cts1) && CVMctsIsPC(cts2)) {
	if (CVMctsGetPC(cts1) != CVMctsGetPC(cts2)) {
	    /* %comment rt011 */
	    result = cts2;
	} else {
	    result = cts2;
	}
    } else {
	result = cts1 | cts2;
    }
    return result;
}

static void
CVMstackmapMergeState(CVMStackmapContext* con, CVMBasicBlock* bb,
		      CVMCellTypeState* varState, CVMCellTypeState* stackState,
		      CVMInt32 topOfStack)
{
    CVMBool change = CVM_FALSE;
    CVMUint32 i;
    if ((bb->topOfStack != -1) && (bb->topOfStack != topOfStack)){
	throwError(con, CVM_MAP_CANNOT_MAP);
    }

#ifdef CVM_DEBUG
    if ( CVMstackmapVerboseDebug ){
	CVMtraceStackmaps(("Basic block=%x, merging state: Input\n", bb));
	CVMstackmapPrintState(con, topOfStack, varState, stackState);

	CVMtraceStackmaps(("Basic block=%x, merging state: Before\n", bb));
	CVMstackmapPrintState(con, bb->topOfStack, bb->varState, bb->stackState);
    }
#endif

    if (bb->topOfStack == -1) {
	/*
	 * Never interpreted before. Mark as changed
	 */
	bb->topOfStack = topOfStack;
	change = CVM_TRUE;
    }
    for (i = 0; i < con->nVars; i++) {
	CVMCellTypeState oldState = bb->varState[i];
	CVMCellTypeState newState = CVMstackmapMerge(varState[i], oldState);
	bb->varState[i] = newState;
	if (newState != oldState) {
	    change = CVM_TRUE;
	}
    }
    if (topOfStack > 0) {
	for (i = 0; i < topOfStack; i++) {
	    CVMCellTypeState oldState = bb->stackState[i];
	    CVMCellTypeState newState =
		CVMstackmapMerge(stackState[i], oldState);
	    bb->stackState[i] = newState;
	    if (newState != oldState) {
		change = CVM_TRUE;
	    }
	}
    }
    bb->changed |= change;
#ifdef CVM_DEBUG
    if ( CVMstackmapVerboseDebug ){
	CVMtraceStackmaps(("Basic block=%x, merging state (changed=%s): After\n",
		     bb, bb->changed ? "TRUE" : "FALSE"));
	CVMstackmapPrintState(con, bb->topOfStack, bb->varState, bb->stackState);
    }
#endif
}

/*
 * See the exceptions that the instruction at pc can raise, and merge state
 * into those handlers.
 *
 * The stack is in a special state for an exception handler -- it only holds
 * one Ref at the top, the exception being thrown.
 */
static void
CVMstackmapMergeStateIntoExcHandlers(CVMStackmapContext* con,
				     CVMUint8* pc,
				     CVMCellTypeState* varState)
{
    CVMExceptionHandler* excTab = CVMmbExceptionTable(con->mb);
    CVMUint16            relPC  = (CVMUint16)(pc - con->code);
    CVMBasicBlock*       handlerBB;
    CVMCellTypeState     excHandlerStackState[1] = {CVMctsRef};
    CVMInt32             i;

    CVMassert(CVMbcAttr(*pc, THROWSEXCEPTION));
    CVMassert(con->nExceptionHandlers > 0);

    for (i = 0; i < con->nExceptionHandlers; i++) {
	if (excTab->startpc <= relPC && relPC < excTab->endpc) {
	    handlerBB = con->exceptionsArea[i];
#ifdef CVM_DEBUG
	    if ( CVMstackmapVerboseDebug ){
		CVMtraceStackmaps(("relPC=%d fits [%d, %d), propagating to %x (pc=%d)\n",
			     relPC, excTab->startpc, excTab->endpc, handlerBB,
			    excTab->handlerpc));
	    }
#endif
	    CVMassert(handlerBB->startPC - con->code == excTab->handlerpc);
	    /*
	     * An exception handler's stack state is special. There is an
	     * exception object at the top of the stack, and the height
	     * of the stack is 1.
	     */
	    CVMstackmapMergeState(con, handlerBB,
				  varState, excHandlerStackState, 1);
	}
	excTab++;
    }
}

/*
 * Do abstract interpretation on a basic block, and merge its exit state
 * to its successors, marking each changed in the process.
 */
static void
CVMstackmapDoDataflowOnBasicBlock(CVMStackmapContext* con, CVMBasicBlock* bb)
{
    CVMCellTypeState* varState   = con->varState;
    CVMCellTypeState* stackState = con->stackState;
    CVMInt32          topOfStack = bb->topOfStack;
    CVMUint8*         pc = bb->startPC;
    CVMUint8*         endPC;
    CVMUint8*         lastPC = NULL;
    CVMBasicBlock**   successors;
    
    /*
     * Copy the entry state of the basic block to the 'work state'. We don't
     * want to destroy the entry state of the current block while interpreting.
     */
    memcpy(varState, bb->varState, con->nVars * sizeof(CVMCellTypeState));
    if (topOfStack > 0) {
	memcpy(stackState, bb->stackState,
		   topOfStack * sizeof(CVMCellTypeState));
    }
#ifdef CVM_DEBUG
    if ( CVMstackmapVerboseDebug ){
	CVMtraceStackmaps(("Interpreting basic block 0x%x, pc=%d. Entry state:\n",
		     bb, pc - con->code));
	CVMstackmapPrintState(con, topOfStack, varState, stackState);
    }
#endif
    if (bb + 1 == con->basicBlocks + con->nBasicBlocks) {
	endPC = &con->code[con->codeLen];
    } else {
	endPC = bb[1].startPC;
    }
    if (con->nExceptionHandlers == 0) {
	/*
	 * No exception checks here
	 */
	while (pc < endPC) {
	    topOfStack = CVMstackmapInterpretOne(con, pc, varState, stackState,
						 topOfStack);
	    lastPC = pc;
	    pc += getOpcodeLength(con, pc);
	}
    } else {
	while (pc < endPC) {
	    /*
	     * Before we 'execute' the instruction, propagate its state
	     * to exception handlers.
	     */
	    if (CVMbcAttr(*pc, THROWSEXCEPTION)) {
		/*
		 * The stack state for all exception handlers is fixed.
		 * So send over only the variable state.
		 */
		CVMstackmapMergeStateIntoExcHandlers(con, pc, varState);
	    }
	    topOfStack = CVMstackmapInterpretOne(con, pc, varState, stackState,
						 topOfStack);
	    lastPC = pc;
	    pc += getOpcodeLength(con, pc);
	}
    }
#ifdef CVM_DEBUG
    if ( CVMstackmapVerboseDebug ){
	CVMtraceStackmaps(("Interpreting basic block 0x%x, pc=%d, endpc=%d, "
		     "last instr %s[%d,%d]."
		     "Exit state:\n",
		     bb, bb->startPC - con->code, pc - con->code,
		     CVMopnames[*lastPC], lastPC[1], lastPC[2]));
	CVMstackmapPrintState(con, topOfStack, varState, stackState);
    }
#endif
    /*
     * Now varState and stackState hold the exit state of the current basic
     * block. Propagate this state into all successors of this basic block.
     */
    successors = bb->successors;
    if (successors != 0) {
	do {
#ifdef CVM_DEBUG
	    if ( CVMstackmapVerboseDebug ){
		CVMtraceStackmaps(("Propagating state to successor 0x%x, pc=%d\n",
			     *successors, (*successors)->startPC - con->code));
	    }
#endif
	    CVMstackmapMergeState(con, *successors,
				  varState, stackState, topOfStack);
	    successors++;
	} while (*successors != 0);
    }

#ifdef CVM_JIT
    bb->endTopOfStack = topOfStack;
#endif

    /*
     * The only successors we could not determine statically were those
     * of jsr blocks. If the last instruction of this basic block was ret
     * propagate the return value to all jsr-returns that follow this ret.
     */
    if ((lastPC[0] == opc_ret) ||
	((lastPC[0] == opc_wide) && (lastPC[1] == opc_ret))) {
	CVMJsrTableEntry* jsrEntry;
	CVMInt32          nCallers;
	CVMUint32         targetPC;
	CVMCellTypeState  ctsTargetPC;

#ifdef CVM_JIT
        bb->endsWithRet = CVM_TRUE;
#endif
	if (lastPC[0] == opc_ret) {
	    ctsTargetPC = varState[lastPC[1]];
	} else {
	    CVMassert((lastPC[0] == opc_wide) && (lastPC[1] == opc_ret));
	    ctsTargetPC = varState[CVMgetUint16(lastPC+2)];
	}

	CVMassert(CVMctsIsPC(ctsTargetPC));
	targetPC = CVMctsGetPC(ctsTargetPC);
	jsrEntry = CVMstackmapGetJsrTargetEntry(con, targetPC);
	CVMassert(jsrEntry != 0);
	nCallers = jsrEntry->jsrNoCalls;
	/*
	 * Send the state of this jsr block back to all potential callers
	 */
	while (--nCallers >= 0) {
	    CVMBasicBlock* caller = jsrEntry->jsrCallers[nCallers];
	    /* if this is an expunged entry, skip it */
	    if (caller == NULL){
		continue;
	    }
#ifdef CVM_DEBUG
	    if ( CVMstackmapVerboseDebug ){
		CVMtraceStackmaps(("Propagating state to jsr caller 0x%x, pc=%d\n",
			     caller, caller->startPC - con->code));
	    }
#endif
	    CVMstackmapMergeState(con, caller,
				  varState, stackState, topOfStack);
	}
    }
#ifdef CVM_DEBUG
    if ( CVMstackmapVerboseDebug ){
	CVMtraceStackmaps(("Done interpreting basic block %x\n", bb));
    }
#endif
}

/*
 * Initialize the method entry state for the first basic block.
 *
 */
static void
CVMstackmapSetupMethodEntryState(CVMStackmapContext* con,
				 CVMBasicBlock* firstBB)
{
    CVMMethodTypeID  tid = CVMmbNameAndTypeID(con->mb);
    CVMterseSigIterator  terseSig;
    int	       typeSyllable;

    CVMCellTypeState* vars = firstBB->varState;
    CVMUint32 varNo = 0;

    CVMtypeidGetTerseSignatureIterator( tid, &terseSig );

    if (!CVMmbIs(con->mb, STATIC)) {
	vars[varNo++] = CVMctsRef; /* this */
    }	
    while((typeSyllable=CVM_TERSE_ITER_NEXT( terseSig) ) != CVM_TYPEID_ENDFUNC ){
	switch( typeSyllable ){
	case CVM_TYPEID_OBJ:
	    vars[varNo++] = CVMctsRef; break;
	case CVM_TYPEID_LONG: 
	case CVM_TYPEID_DOUBLE:
	    vars[varNo++] = CVMctsVal; vars[varNo++] = CVMctsVal; break;
	default:
	    vars[varNo++] = CVMctsVal;
	}
    }
    /*
     * The rest of the locals are uninitialized
     */
    while(varNo < con->nVars) {
	vars[varNo++] = CVMctsUninit;
    }
    CVMassert(varNo == con->nVars);
}

/*
 * Do data-flow analysis on basic blocks until none are changed.
 */
static void
CVMstackmapDoDataflow(CVMStackmapContext* con)
{
    CVMBool change = CVM_TRUE;
    CVMBasicBlock* firstBB;
    int i;
    /*
     * First get our entry state from the method signature. 
     */
    firstBB = &con->basicBlocks[0];
    CVMstackmapSetupMethodEntryState(con, firstBB);
    firstBB->topOfStack = 0;     /* Empty entry stack */
    firstBB->changed = CVM_TRUE; /* Start here */
    
#ifdef CVM_DEBUG
    if ( CVMstackmapVerboseDebug ){
	CVMtraceStackmaps(("firstBB=%x\n", firstBB));
	CVMstackmapPrintBBState(con, &con->basicBlocks[0]);
    }
#endif
    /*
     * Now interpret basic blocks until none changed
     */
    while (change) {
	change = CVM_FALSE;
	for (i = 0; i < con->nBasicBlocks; i++) {
	    CVMBasicBlock* bb = con->basicBlocks + i;
	    if (bb->changed) {
#ifdef CVM_DEBUG
		if ( CVMstackmapVerboseDebug ){
		    CVMtraceStackmaps(("bb=%x changed -- interpreting\n", bb));
		}
#endif
		change = CVM_TRUE;
		bb->changed = CVM_FALSE;
		CVMstackmapDoDataflowOnBasicBlock(con, bb);
	    }
	}
    }
}

/*
 * A callback function to be called on each GC point with info
 * on the state of the method at the GC point.
 */
typedef void CVMGCPointCallbackFunc(CVMStackmapContext* con, CVMUint16 relPC,
				    CVMCellTypeState* varState,
				    CVMCellTypeState* stackState,
				    CVMInt32 topOfStack,
				    void* data);

/*
 * Go to GC points from the type-states at the head of each basic
 * block, interpret, and call a callback function for each GC point
 * with the current state. This is used for counting the memory to allocate,
 * emitting the maps, and other suitable things (maybe emitting stats).
 */
static void
CVMstackmapDoForeachGCPointInBB(CVMStackmapContext* con,
				CVMBasicBlock* bb,
				CVMGCPointCallbackFunc* callback,
				void* data)
{
    CVMCellTypeState* varState   = con->varState;
    CVMCellTypeState* stackState = con->stackState;
    CVMInt32          topOfStack = bb->topOfStack;
    CVMUint8*         pc = bb->startPC;
    CVMUint8*         endPC;
    CVMUint32*        gcPointsBitmap = con->gcPointsBitmap;
    
    /*
     * Copy the entry state of the basic block to the 'work state'. We don't
     * want to destroy the entry state of the current block while interpreting.
     */
    memcpy(varState, bb->varState, con->nVars * sizeof(CVMCellTypeState));
    if (topOfStack > 0) {
	memcpy(stackState, bb->stackState,
		   topOfStack * sizeof(CVMCellTypeState));
    }

    if (bb + 1 == con->basicBlocks + con->nBasicBlocks) {
	endPC = &con->code[con->codeLen];
    } else {
	endPC = bb[1].startPC;
    }

    if (con->mayNeedRewriting && (pc == con->code)) {
	/*
	 * Do pc == 0 an extra time when re-writing ref-uninit conflicts
	 */
	callback(con, 0, varState, stackState, topOfStack, data);
    }
    while (pc < endPC) {
	CVMUint16 relPC = (CVMUint16)(pc - con->code);
	/*
	 * We already have a bitmap here indicating where our GC points are.
	 * This is a summary of all unconditional GC points like invocations
	 * and returns, as well as backwards branches.
	 */
	if (CVMstackmapIsGCPoint(gcPointsBitmap, relPC)) {
	    /* If this is a return opcode then we want to ignore all locals. */
	    if (CVMbcAttr(*pc, RETURN)) {
		memset(varState, CVMctsBottom,
		       con->nVars * sizeof(CVMCellTypeState));
	    }
	    /* 
	     * For each GC point, do what needs to be done.
	     */
	    callback(con, relPC, varState, stackState, topOfStack, data);
	}
	topOfStack = CVMstackmapInterpretOne(con, pc, 
					     varState, stackState, topOfStack);
	pc += getOpcodeLength(con, pc);
    }
}

/*
 * This is the thing to do on each GC point while counting number
 * of bytes to allocate.
 */
static void
CVMstackmapCountAllocBytes(CVMStackmapContext* con, CVMUint16 relPC,
			   CVMCellTypeState* varState,
			   CVMCellTypeState* stackState,
			   CVMInt32 topOfStack,
			   void* data)
{
    CVMUint32* nBytes = (CVMUint32*)data;
    /*
     * Number of 16-bit chunks to store the state at this point.
     */
    CVMUint32  nStateBits = topOfStack + con->nVars;
    /*
     * This function is called when the maximum state size required is more
     * than 15 bits. However, there may still be GC points in such a method
     * with less than 15 state bits.
     */
    if (nStateBits > 15) {
	/* An additional bit for the 'long' flag */
	CVMUint32  nStateChunks = (nStateBits + 1 + 15) / 16;
	CVMUint32  additionalSize = (nStateChunks + 1) * sizeof(CVMUint16);

	/* If the resultant offset cannot be encoded in the state field
	   of CVMStackMapEntry, then we have a super extended entry.
	   
	   The current nBytes gives us an indication of the offset for this
	   entry.  However, the offset is in units of CVMUint16 instead of
	   bytes.  Hence, we need to multiply CVM_STACKMAP_RESERVED_OFFSET
	   by sizeof(CVMUint16) in order for the comparison to be valid.

	   We need to check nBytes before (not after) we add the size of the
	   current entry because the offset of the current entry is determined
	   by the cummulative sizes of all the previous entries (i.e. previous
	   nBytes).
	*/
	if (*nBytes >= (CVM_STACKMAP_RESERVED_OFFSET * sizeof(CVMUint16))) {
	    con->numberOfSuperExtendedEntries++;
	}

	/* The additional chunk is for the PC for this point */
	*nBytes += additionalSize;
    }
}

/*
 * We've already counted the number of GC points. Allocate space
 * for all the stackmaps for this method.
 */
static void
CVMstackmapAllocateMaps(CVMStackmapContext* con)
{
    int i;
    CVMUint32 nBytes;
    CVMUint32 initialSize;

    /*
     * There are two cases here. 
     *
     * If the maxstack of this method indicates we can fit each GC
     * point in one short stackmap entry even in the worst case, we
     * don't have to go figure out the amount of extra storage. So we
     * allocate enough memory to hold the short entries for all GC
     * points. This is the fast, common case.
     * 
     * If maxstack+nLocals indicates that there may be GC points which
     * will need the long version, then we have to make a pass over
     * the code to count how many. This is slow, but does not happen
     * too often.  
     */

    /*
     * We are going to be allocating a variable-size CVMStackMaps structure. 
     * Start out with all short entries, con->nGCPoints long.
     */
    initialSize = sizeof(CVMStackMaps) +
	(con->nGCPoints - 1) * sizeof(CVMStackMapEntry);
    nBytes = initialSize;

    /* The CVMStackMaps and CVMStackMapEntry should have already been packed
       to word-aligned boundaries.  Hence, nBytes should automatically be
       word aligned.  Assert this because we rely on it to ensure that the
       CVMStackMapSuperExtendedMaps pointer at the start of the extended area
       will be located on a word-aligned boundary: */
    CVMassert(nBytes == CVMalignWordUp(nBytes));

    con->numberOfSuperExtendedEntries = 0;
    if (con->stateSize > CVM_STACKMAP_ENTRY_NUMBER_OF_BASIC_BITS) {
	CVMUint32 extendedSize = 0;

	/* Reserve space for the pointer to the super extended maps at the
	   start of the extended area: */
	extendedSize += sizeof(CVMStackMapSuperExtendedMaps *);

	/* potentially long version. Run through and count extra memory */
	for (i = 0; i < con->nBasicBlocks; i++) {
	    CVMBasicBlock* bb = con->basicBlocks + i;
	    if (bb->topOfStack != -1) {
		CVMstackmapDoForeachGCPointInBB(con, bb,
						CVMstackmapCountAllocBytes,
						&extendedSize);
	    }
	}

	/* Add the memory budget for the extended and super extended (if
	   present) entries: */
	nBytes += extendedSize;
	
	/* Add the memory budget for the CVMStackMapSuperExtendedMaps struct if
	   needed: */
	if (con->numberOfSuperExtendedEntries > 0) {
	    nBytes += sizeof(CVMStackMapSuperExtendedMaps) +
		      (con->numberOfSuperExtendedEntries - 1) *
		          sizeof(CVMStackMapEntry *);

	    /* Also add some padding for possible alignment bytes needed to
	       ensure that the CVMStackMapSuperExtendedMaps struct is word
	       aligned: */
	    nBytes += sizeof(CVMAddr);
	}
    }
    /*
     * Note that we have not uncounted unreachable GC points here. It
     * is not worth the trouble of making an extra pass to detect a
     * very rare situation just to save a few bytes.  */
    /*
     * We know we need nBytes bytes of storage for the stackmaps.
     * Allocate.
     */
    con->stackMaps = (CVMStackMaps*)calloc(1, nBytes);
    if (con->stackMaps == NULL) {
	throwError(con, CVM_MAP_OUT_OF_MEMORY);
    }

    con->stackMaps->size = nBytes;
    con->stackMaps->mb = con->mb;

    /*
     * Make the variable size part be just past the constant size section.
     */
    con->stackMapAreaBegin = (CVMUint16*)
	&con->stackMaps->smEntries[con->nGCPoints];

    /* Ensure that the extended area starts on a word-aligned boundary as
       expected.  This is because the CVMStackMapSuperExtendedMaps pointer
       which is stored at the start of this area needs to be on a word-aligned
       boundary: */
    CVMassert(con->stackMapAreaBegin ==
	      (CVMUint16*)CVMalignWordUp(con->stackMapAreaBegin));

    if (con->stateSize > CVM_STACKMAP_ENTRY_NUMBER_OF_BASIC_BITS) {
	/* Reserve space for the pointer to the super extended maps: */
	con->stackMapAreaPtr =
	    (CVMUint16 *)((CVMUint8 *)con->stackMapAreaBegin +
			  sizeof(CVMStackMapSuperExtendedMaps *));
	
	/* Initialize the seMaps pointer to be NULL first.  This will get set
	   to a non-NULL address later if an seMaps struct is actually needed:
	*/
	con->seMaps = NULL;
	CVMassert(*(CVMStackMapSuperExtendedMaps **)con->stackMapAreaBegin ==
		  NULL);
	con->currentNumberOfSEEntries = 0;
    } else {
	con->stackMapAreaPtr = con->stackMapAreaBegin;
    }

    /*
     * And this is the end, for assertions
     */
    con->stackMapAreaEnd = (CVMUint16*)
	((CVMUint8*)con->stackMaps + nBytes);
    /*
     * How many bytes did we allocate for long format stackmap entries?
     */
    con->stackMapLongFormatNumBytes = (CVMUint32)
	((con->stackMapAreaEnd - con->stackMapAreaBegin) * sizeof(CVMUint16));
}

/*
 * Convert a state into a bitmap. Return the number of chunks used.
 */
static CVMUint32
CVMstackmapStateToBitmap(CVMStackmapContext* con,
			 CVMStackMapEntry* smArea, CVMUint16 pc,
			 CVMCellTypeState* varState,
			 CVMCellTypeState* stackState,
			 CVMInt32 topOfStack)
{
    /*
     * Number of 16-bit chunks to store the state at this point.
     */
    CVMUint32  chunkNo = 0;
    CVMUint16  bit     = 1;
    CVMUint32  bitNo   = 0;
    CVMUint32  i;

    /*
     * We are doing the entry for 'pc'
     */
    smArea->pc = pc;

    /* The lowest bit of state[0] is reserved */
    bit <<= 1;
    bitNo += 1;

    /*
     * Do the vars first
     */
    for (i = 0; i < con->nVars; i++) {
	CVMCellTypeState theVar = varState[i];
	/*
	 * We either have a conflict, or we have a real ref.
	 */
	if (CVMctsIsRef(theVar)) {
	    if (theVar == CVMctsRef) {
		smArea->state[chunkNo] |= bit;
	    } else {
		/*
		 * We have a conflict. If it is a ref-uninit conflict, we know
		 * how to handle it.
		 */
		if (con->refVarsToInitialize[i] != 0) {
		    /*
		     * Mark variables with ref-uninit conflicts as refs in the
		     * stackmap. We will have prepended code to the method
		     * to initialize these variables.
		     */
		    smArea->state[chunkNo] |= bit;
		}
	    }
	}
	bit <<= 1;
	bitNo++;
	if (bitNo == 16) {
	    bit = 1;
	    bitNo = 0;
	    chunkNo++;
	}
    }
    /*
     * And then the stack
     */
    for (i = 0; i < topOfStack; i++) {
	if (stackState[i] == CVMctsRef) {
	    smArea->state[chunkNo] |= bit;
	}
	bit <<= 1;
	bitNo++;
	if (bitNo == 16) {
	    bit = 1;
	    bitNo = 0;
	    chunkNo++;
	}
    }
    /*
     * Did we put anything in the next chunk?
     */
    if (bitNo == 0) {
	chunkNo--;
    }
    CVMassert(chunkNo + 1 == (con->nVars + topOfStack + 1 + 15) / 16);
    return chunkNo + 1;
}

/*
 * This is the thing to do on each GC point while emitting stackmaps
 */
static void
CVMstackmapEmitMapForPC(CVMStackmapContext* con, CVMUint16 relPC,
			CVMCellTypeState* varState,
			CVMCellTypeState* stackState,
			CVMInt32 topOfStack,
			void* data)
{
    CVMStackMapEntry* thisEntry =
	&con->stackMaps->smEntries[con->stackMaps->noGcPoints];
    /*
     * Number of bits needed for the state at this point.
     */
    CVMUint32  nStateBits = topOfStack + con->nVars;
    if (nStateBits > CVM_STACKMAP_ENTRY_NUMBER_OF_BASIC_BITS) {
	CVMUint32 nChunks;
	CVMStackMapEntry* longArea;
	CVMUint32 longOffset;

	/*
	 * Get some from the extra storage area
	 */
	longArea = (CVMStackMapEntry*)con->stackMapAreaPtr;

	/* If the longOffset looks like it is larger than will fit within
	   entry->state[0], then the real entry need to reside in the super
	   extended region instead of the extended region: */
	longOffset = (con->stackMapAreaPtr - con->stackMapAreaBegin);
	if (longOffset >= CVM_STACKMAP_RESERVED_OFFSET) {
	    CVMStackMapSuperExtendedMaps *seMaps;
	    seMaps = *(CVMStackMapSuperExtendedMaps **)con->stackMapAreaBegin;

	    /* Initialize the superExtendedMaps table if needed: */
	    if (seMaps == NULL) {

		/* Make sure that we're aligned on a word boundary for the
		   pointers in the super extended maps: */
		longArea = (CVMStackMapEntry*)CVMalignWordUp(longArea);

		con->seMaps = (CVMStackMapSuperExtendedMaps *)longArea;
		con->seMaps->numberOfEntries = con->numberOfSuperExtendedEntries;
		CVMassert(con->currentNumberOfSEEntries == 0);

		/* Point the extended stackmap entry buffer after the seMaps: */
		longArea = (CVMStackMapEntry*)
		    ((CVMUint8 *)longArea +
		     sizeof(CVMStackMapSuperExtendedMaps) +
		     (con->numberOfSuperExtendedEntries - 1) *
		         sizeof(CVMStackMapEntry*));
		con->stackMapAreaPtr = (CVMUint16 *)longArea;

		/* Make sure that the CVMStackMapSuperExtendedMaps pointer is
		   located on a word-aligned boundary: */
		CVMassert(con->stackMapAreaBegin ==
			  (CVMUint16 *)CVMalignWordUp(con->stackMapAreaBegin));

		/* Initialize the seMaps pointer in the extended region: */
		*(CVMStackMapSuperExtendedMaps **)con->stackMapAreaBegin =
		    con->seMaps;
	    }

	    con->seMaps->entries[con->currentNumberOfSEEntries++] = longArea;
	    longOffset = CVM_STACKMAP_RESERVED_OFFSET;
	}

	nChunks = CVMstackmapStateToBitmap(con, longArea, relPC,
					   varState, stackState, topOfStack);

	CVMassert(nChunks > CVM_STACKMAP_ENTRY_MIN_NUMBER_OF_CHUNKS);
	
	/*
	 * Build the indirect entry here.
	 * Flag the lowest bit, and insert the offset
	 */
	thisEntry->pc = relPC;
	thisEntry->state[0] = (CVMUint16)((longOffset << 1) + 1);
	con->stackMapAreaPtr += nChunks + 1;
	CVMassert(con->stackMapAreaPtr <= con->stackMapAreaEnd);
    } else {
	/* The short state version. Put the state in-line. */
	CVMUint32 nChunks;
	nChunks = CVMstackmapStateToBitmap(con, thisEntry, relPC,
					   varState, stackState, topOfStack);
	CVMassert(nChunks <= CVM_STACKMAP_ENTRY_MIN_NUMBER_OF_CHUNKS);
    }
    /* Emitted another one */
    con->stackMaps->noGcPoints++;
#ifdef CVM_DEBUG
    if ( CVMstackmapVerboseDebug ){
	CVMtraceStackmaps(("Emitted: pc=%d, state=%d\n", relPC, nStateBits));
	CVMstackmapPrintState(con, topOfStack, varState, stackState);
    }
#endif
}

#ifdef CVM_DEBUG_ASSERTS
static void
CVMstackmapsVerifyStackmaps(CVMStackmapContext* con)
{
    CVMUint32 i;
    CVMUint32 numberOfBasicEntries = 0;
    CVMUint32 numberOfExtendedEntries = 0;
    CVMUint32 numberOfSuperExtendedEntries = 0;

    /* The numberOfBasicEntries counted here is a count of the basic entries
       which do not have an extended or super extended entry.  This is different
       than the use of the term of "basic entry" in other parts of this file
       where it refers to entries which reside in the basic region: */

    for (i = 0; i < con->stackMaps->noGcPoints; i++) {
	CVMStackMapEntry *entry = &con->stackMaps->smEntries[i];
	if ((entry->state[0] & 1) == 0) {
	    numberOfBasicEntries++;
	} else {
	    CVMUint16 offset = entry->state[0] >> 1;
	    if (offset == CVM_STACKMAP_RESERVED_OFFSET) {
		numberOfSuperExtendedEntries++;
	    } else {
		numberOfExtendedEntries++;
	    }
	}
    }

#ifdef CVM_DEBUG
    if (CVMstackmapVerboseDebug) {
	CVMconsolePrintf("Stackmap region verification\n");
	CVMconsolePrintf("  total expected GC points: %d\n", con->nGCPoints);
	CVMconsolePrintf("  total actual GC points: %d\n",
			 con->stackMaps->noGcPoints);
	CVMconsolePrintf("  From basic region:\n");
	CVMconsolePrintf("     numberOfBasicEntries: %d\n",
			 numberOfBasicEntries);
	CVMconsolePrintf("     numberOfExtendedEntries: %d\n",
			 numberOfExtendedEntries);
	CVMconsolePrintf("     numberOfSuperExtendedEntries: %d\n",
			 numberOfSuperExtendedEntries);
	CVMconsolePrintf("     total entries: %d\n",
			 numberOfBasicEntries + numberOfExtendedEntries +
			 numberOfSuperExtendedEntries);
	CVMconsolePrintf("     numberOfSuperExtendedEntries created: %d\n",
			 con->currentNumberOfSEEntries);
    }
#endif

    /* The number of each type of entries must add up to the number of GC
       points: */
    CVMassert(con->stackMaps->noGcPoints ==
	      numberOfBasicEntries + numberOfExtendedEntries +
	      numberOfSuperExtendedEntries);


    /* The number of super extended entries found must equal the number that we
       created earlier: */
    CVMassert(numberOfSuperExtendedEntries == con->currentNumberOfSEEntries);

    if (numberOfSuperExtendedEntries > 0) {
	CVMStackMapSuperExtendedMaps **seMapsPtr;
	CVMStackMapSuperExtendedMaps *seMaps;
	CVMUint16 lastPC;
    
	seMapsPtr = (CVMStackMapSuperExtendedMaps **)
	    &con->stackMaps->smEntries[con->stackMaps->noGcPoints];

	/* The pointer must be located on a word-aligned boundary: */
	CVMassert(seMapsPtr ==
		  (CVMStackMapSuperExtendedMaps **)CVMalignWordUp(seMapsPtr));

	seMaps = *seMapsPtr;

	/* The number of entries in the seMaps must match the number expected
	   as infered from the basic entries: */
	CVMassert(seMaps->numberOfEntries == numberOfSuperExtendedEntries);

#ifdef CVM_DEBUG
	if (CVMstackmapVerboseDebug) {
	    for (i = 0; i < seMaps->numberOfEntries; i++) {
		if (seMaps->entries[i] == NULL) {
		    CVMconsolePrintf("seMaps->entries[%d] is NULL\n", i);
		}
	    }
	}
#endif
	/* Ensure that every entry in the seMaps is not NULL.  Also ensure that
	   the entries are sorted in increasing order: 

	   Note: it is OK to initialize lastPC to 0 because we should never
	   ever have an entry in the seMaps for PC 0.  This is because we will
	   at least be able to store that entry in the extended region if not
	   the basic region.
	*/
	lastPC = 0;
	for (i = 0; i < seMaps->numberOfEntries; i++) {
	    CVMassert(seMaps->entries[i] != NULL);
	    CVMassert(seMaps->entries[i]->pc > lastPC);
	    lastPC = seMaps->entries[i]->pc;
	}
    }
}
#endif /* CVM_DEBUG_ASSERTS */

/*
 * We are done with the flow analysis. Now allocate and emit the
 * stackmaps for everybody.
 */
static void
CVMstackmapDoStackmaps(CVMStackmapContext* con)
{
    int i;
    /*
     * First allocate the space for the stackmaps
     */
    CVMstackmapAllocateMaps(con);

    /*
     * Now go through each basic block, and generate the stackmaps.
     */
    for (i = 0; i < con->nBasicBlocks; i++) {
	CVMBasicBlock* bb = con->basicBlocks + i;
	/*
	 * I came across some unreachable code generated by javac. The
	 * basic block in question was an exception handler for a
	 * range of code with no instructions that could throw
	 * exceptions! Therefore, check whether the basic block was
	 * ever reached before trying to generate stackmaps for it.  
	 */
	if (bb->topOfStack != -1) {
	    CVMstackmapDoForeachGCPointInBB(con, bb,
					    CVMstackmapEmitMapForPC, 0);
	}
    }

    /*
     * We'd better have counted at most con->nGCPoints. We might have
     * emitted less, since there may be unreachable code (very
     * unlikely, but it could happen).  
     */
    CVMassert(con->stackMaps->noGcPoints <= con->nGCPoints);
    /*
     * Now in case there was unreachable code, we would not have computed
     * the start of the long format stackmap entries correctly. This is the
     * time to move them up in memory to immediately follow the actually
     * emitted stackmap entries.
     */
    if (con->stackMaps->noGcPoints < con->nGCPoints) {

	CVMassert(&con->stackMaps->smEntries[con->nGCPoints] ==
		  (CVMStackMapEntry *)con->stackMapAreaBegin);

	/* Move from previous start to right after the stackmaps */
	memmove(&con->stackMaps->smEntries[con->stackMaps->noGcPoints],
		    &con->stackMaps->smEntries[con->nGCPoints],
		    con->stackMapLongFormatNumBytes);

	/* Since the data in the extended and super extended (if present)
	   regions have been moved above, we will need to adjust the pointer
	   to the seMaps and the pointers in the seMaps accordingly (that is
	   is the seMaps is present.

	   The following code computes the adjustment (i.e. the displacement)
	   that needs to be made to the pointers, and does all the necessary
	   adjustments:
	*/
	if (con->numberOfSuperExtendedEntries > 0) {
	    CVMStackMapEntry *entries = con->stackMaps->smEntries;
	    CVMStackMapSuperExtendedMaps **seMapsPtr;
	    CVMStackMapSuperExtendedMaps *seMaps;
	    CVMUint32 displacement;
	    
	    /* Compute the number of bytes of displacement that the data has
	       been moved in memory: */
	    displacement = (CVMUint8 *)&entries[con->nGCPoints] -
		           (CVMUint8 *)&entries[con->stackMaps->noGcPoints];

	    /* Get the address of the seMaps which should be located at the end
	       of the basic region now that we've moved it down: */
	    seMapsPtr = (CVMStackMapSuperExtendedMaps **)
		        &entries[con->stackMaps->noGcPoints];
	    CVMassert(seMapsPtr ==
	        (CVMStackMapSuperExtendedMaps **)CVMalignWordUp(seMapsPtr));

	    /* Fetch the address of the seMaps: */
	    seMaps = *seMapsPtr;

	    /* Adjust the address by the displacement: */
	    seMaps = (CVMStackMapSuperExtendedMaps *)
		((CVMUint8 *)seMaps - displacement);
	    *seMapsPtr = seMaps;

	    /* Next adjust all the entry pointers in seMaps: */
	    for (i = 0; i < seMaps->numberOfEntries; i++) {
		CVMStackMapEntry *entry = seMaps->entries[i];
		entry = (CVMStackMapEntry *)
		    ((CVMUint8 *)entry - displacement);
		seMaps->entries[i] = entry;
	    }
	}
    }

#ifdef CVM_DEBUG_ASSERTS
    /* Do the verification at the very end after everything is done including
       the part above where the seMaps pointers need to be readjusted: */
    CVMstackmapsVerifyStackmaps(con);
#endif
}

/* Adds the newly created stackmaps to the global list. */
static void
CVMstackmapAddToGlobalList(CVMExecEnv *ee, CVMStackMaps *maps)
{
    CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.heapLock));

    /* Insert into the global stackMaps list: */
    if (CVMglobals.gcCommon.lastStackMaps == NULL) {
        CVMglobals.gcCommon.lastStackMaps = maps;
    }
    maps->next = CVMglobals.gcCommon.firstStackMaps;
    maps->prev = NULL;
    if (CVMglobals.gcCommon.firstStackMaps != NULL) {
        CVMglobals.gcCommon.firstStackMaps->prev = maps;
    }
    CVMglobals.gcCommon.firstStackMaps = maps;
    CVMglobals.gcCommon.stackMapsTotalMemoryUsed += maps->size;
}

void
CVMstackmapPromoteToFrontOfGlobalList(CVMExecEnv *ee, CVMStackMaps *maps)
{
    CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.heapLock));

    if (CVMglobals.gcCommon.firstStackMaps == maps) {
        return; /* Already at the front of the list. */
    }

    /* Remove from the list: */
    if (maps->next != NULL) {
        maps->next->prev = maps->prev;
    } else {
        CVMglobals.gcCommon.lastStackMaps = maps->prev;
    }
    if (maps->prev != NULL) {
        maps->prev->next = maps->next;
    } else {
        CVMglobals.gcCommon.firstStackMaps = maps->next;
    }

    /* Insert at the head of the list: */
    maps->next = CVMglobals.gcCommon.firstStackMaps;
    maps->prev = NULL;
    CVMassert(CVMglobals.gcCommon.firstStackMaps != NULL);
    CVMglobals.gcCommon.firstStackMaps->prev = maps;
    CVMglobals.gcCommon.firstStackMaps = maps;
}

/* Destroys the stackmaps.
 */
void
CVMstackmapDestroy(CVMExecEnv *ee, CVMStackMaps *maps)
{
    /* 
     * The only way that the ee can be NULL is when calling
     * from CVMclassFreeJavaMethods() (called by CVMpreloaderDestroy())
     * during -XfullShutdown, and we are safe without locking in
     * that case.
     */
    if (ee != NULL) {
        CVMassert(CVMD_isgcSafe(ee));
        CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.heapLock));
    }

    /* Remove from the list: */
    if (maps->next != NULL) {
        maps->next->prev = maps->prev;
    } else {
        CVMglobals.gcCommon.lastStackMaps = maps->prev;
    }
    if (maps->prev != NULL) {
        maps->prev->next = maps->next;
    } else {
        CVMglobals.gcCommon.firstStackMaps = maps->next;
    }
    CVMglobals.gcCommon.stackMapsTotalMemoryUsed -= maps->size;

    /* Release the maps: */
    free(maps);
}

static void
doStatistics(CVMStackmapContext *c)
{
    /* compute stackmap statistics, savings for various encodings
	?? check for duplicates?
    */
}

/*
 * Compute stackmaps 
 */
CVMStackMaps*
CVMstackmapCompute(CVMExecEnv* ee, CVMMethodBlock* mb,
		   CVMBool doConditionalGcPoints)
{
    CVMStackmapContext c;
    CVMBool 	       needRewritePass;
    CVMStackMaps *maps;

    /*
     * Assume method is properly disambiguated.
     * so just go for it.
     */
    CVMstackmapInitializeContext(ee, &c, mb, doConditionalGcPoints);
    CVMtraceStackmaps(("Stackmap generation for %C.%M\n", c.cb, mb ));
    if (setjmp(c.errorHandlerContext) == 0) {
	needRewritePass = CVMstackmapAnalyze( &c );
	if (needRewritePass) {
	    /* This should have been handled before */
	    throwError(&c, CVM_MAP_EXCEEDED_LIMITS);
	}
	
	CVMstackmapDoStackmaps(&c);
#if 1
	doStatistics(&c);
#endif
    } else {
	CVMassert(c.error != CVM_MAP_SUCCESS);
	CVMtraceStackmaps(("Stackmap analysis for %C.%M failed: %s\n",
			   c.cb, mb, errorString(c.error)));
	CVMstackmapDestroyContext(&c);
	return NULL;
    }
    
    /*
     * Set the stackmap in the method
     */
    maps = c.stackMaps;
    CVMstackmapAddToGlobalList(ee, maps);
    c.stackMaps = 0;
    CVMstackmapDestroyContext(&c);

    return maps;
}

/*
 * Find the stackmaps for the mb. Return NULL if there is no stackmaps.
 */
CVMStackMaps*
CVMstackmapFind(CVMExecEnv* ee, CVMMethodBlock* mb)
{
    CVMStackMaps *maps = CVMglobals.gcCommon.firstStackMaps;

    /* 
     * The only way that the ee can be NULL is when calling
     * from CVMclassFreeJavaMethods() (called by CVMpreloaderDestroy())
     * during -XfullShutdown, and we are safe without locking in
     * that case.
     */
    if (ee != NULL) {
        CVMassert(CVMD_isgcSafe(ee));
        CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.heapLock));
    }

    while (maps != NULL) {
	if (maps->mb == mb) {
            return maps;
        }
        maps = maps->next;
    }
    return NULL;
}


/*
 * Called by CVMstackmapDisambiguate to see if there are
 * any JSRs in this method. Also tracks switches.
 */
static CVMBool
CVMstackmapJSRScan( CVMMethodBlock *mb ){
    CVMJavaMethodDescriptor *jmd = CVMmbJmd(mb);
    CVMUint8*	pc;
    CVMUint8*	codeEnd;
    CVMBool	hasJSR = CVM_FALSE;
    CVMBool	hasSwitch = CVM_FALSE;

    pc = CVMjmdCode(jmd);
    codeEnd   = &pc[CVMjmdCodeLength(jmd)];
    while ( pc < codeEnd ){
	switch (*pc){
	case opc_jsr:
        case opc_jsr_w:
	    hasJSR = CVM_TRUE;
	    /*
	     * keep looking, to detect switch instructions
	     */
	    break;
	case opc_lookupswitch:
	case opc_tableswitch:
	    hasSwitch = CVM_TRUE;
	    break;
	}
#ifdef CVM_JVMTI
	/* This should not happen before a class is first used */
	CVMassert(*pc != opc_breakpoint);
#endif
	pc += CVMopcodeGetLength(pc);
    }
    if ( hasSwitch )
	CVMjmdFlags(jmd) |= CVM_JMD_NEEDS_ALIGNMENT;
    if ( hasJSR )
	CVMjmdFlags(jmd) |= CVM_JMD_MAY_NEED_REWRITE;
    return hasJSR;
}

/*
 * Rewrite if necesary, without generating stackmaps.
 * Returns a CVMMapError value.
 * Caller can still do something sensible with failure at this point?
 * ... so we don't declare a fatal error here.
 */

CVMMapError
CVMstackmapDisambiguate(CVMExecEnv* ee, CVMMethodBlock* mb, CVMBool didJSRscan)
{
    CVMStackmapContext c;
    volatile int n = 0;
    CVMBool needRewritePass = didJSRscan || CVMstackmapJSRScan( mb );

    /*
     * Its ok to rewrite twice. Anything more than that is probably 
     * a bug of ours.
     */
    while ( needRewritePass ){
	CVMtraceStackmaps(("Stackmap disambiguation(%d) for %C.%M\n",
			   n, CVMmbClassBlock(mb), mb ));

	CVMstackmapInitializeContext(ee, &c, mb, CVM_FALSE);
	if (setjmp(c.errorHandlerContext) == 0) {
	    needRewritePass = CVMstackmapAnalyze( &c );
	    /*
	     * Now see if we have recorded any local variables with a conflict
	     * If so, rewrite the method and iterate.
	     *
	     * Otherwise, we know the converged type-states at the head of each
	     * basic block, so we are done.
	     */
	    if (needRewritePass) {
		CVMtraceStackmaps(("Rewriting method %C.%M to fix conflicts\n",
				   c.cb, mb));
		CVMstackmapRewriteMethodForConflicts(&c);
	    }

#ifdef CVM_JIT
	    CVMstackmapFilterNonCompilableMethod(ee, &c);
#endif
	} else {
	    CVMassert(c.error != CVM_MAP_SUCCESS);
	    CVMstackmapDestroyContext(&c);
	    return c.error;
	}
	CVMstackmapDestroyContext(&c);
	/*
	 * A couple of iterations is ok. More is probably a bug.
	 */
	if ( n++ > 3 ) {
	    CVMdebugPrintf(("Stackmap disambiguation for %C.%M "
			    "taking too many iterations\n",
			    c.cb, mb));
	    return CVM_MAP_EXCEEDED_LIMITS;
	}
    }
    return CVM_MAP_SUCCESS;
}

/*
 * Return CVM_TRUE if there are conflicts, else CVM_FALSE
 */
static CVMBool
CVMstackmapAnalyze( CVMStackmapContext *con )
{

    /*
     * Build data structures for data flow
     */
    CVMstackmapFindBasicBlocks(con);
    /*
     * Discover which blocks are dead. At the moment,
     * this is only used for preventing ret's from propagating
     * information where they shouldn't.
     */
    CVMstackmapLivenessAnalyze(con);
    /*
     * Do the dataflow analysis and type all slots at basic block
     * entry
     */
    CVMstackmapDoDataflow(con);
    return (con->numRefVarsToInitialize + con->nRefValConflicts + con->nRefPCConflicts > 0 );
}

/* Gets the actual stackmap entry in the super extended region. */
static CVMStackMapEntry*
CVMstackmapGetSuperExtendedEntry(CVMStackMapEntry *basicEntry,
				 CVMStackMapSuperExtendedMaps *seMaps)
{
    CVMStackMapEntry **entries = seMaps->entries;
    int lowest = 0;
    int highbound = seMaps->numberOfEntries;
    int i;
    CVMUint16 candidatePC;
    CVMUint16 pc = basicEntry->pc;

    /* Do a binary search of the list of stack entries to look for a
       matching PC: */
    while (lowest < highbound) {
	CVMStackMapEntry *entry;
	i = lowest + (highbound-lowest)/2;
	entry = entries[i];
	candidatePC = entry->pc;
	if (candidatePC == pc){
	    /* We've found a matching PC.  Now, get the stackmap entry info: */
	    return entry;
	}
	if (candidatePC < pc) {
	    lowest = i+1;
	}else{
	    highbound = i;
	}
    }
    /* We should always be able to find a super extended entry if we get to
       this function.  This is because we only came here when a basic stackmap
       entry was found for the PC in the first place.  Hence, there should be
       no exceptions where a stackmap entry cannot be found. */
    CVMassert(CVM_FALSE);

    return NULL;
}

/* Gets the actual stackmap entry in the extended region. */
static CVMStackMapEntry*
CVMstackmapGetRealEntry(CVMStackMapEntry* entry,
			CVMStackMapEntry* entryEnd)
{
    CVMassert(entryEnd == (CVMStackMapEntry *)CVMalignWordUp(entryEnd));

    /* If the low bit of the entry is set, then we have a large stackmap entry
       encoding.  The current entry actually represents an offset into the
       extra stackmap data area, and that location is where the real stackmap
       entry info resides: */
    if ((entry->state[0] & 1) != 0) {
	CVMUint16 offset = entry->state[0] >> 1;
	/* If the offset is CVM_STACKMAP_RESERVED_OFFSET, then the actual entry
	   resides in the super extended region instead of here in the extended
	   region.  So, call CVMstackmapGetSuperExtendedEntry() to fetch it:
	*/
	if (offset == CVM_STACKMAP_RESERVED_OFFSET) {
	    CVMStackMapSuperExtendedMaps *seMaps;

	    seMaps = *(CVMStackMapSuperExtendedMaps **)entryEnd;
	    CVMassert(seMaps != NULL);
	    CVMassert(seMaps ==
		      (CVMStackMapSuperExtendedMaps *)CVMalignWordUp(seMaps));
	    return CVMstackmapGetSuperExtendedEntry(entry, seMaps);
	} else {
#ifdef CVM_DEBUG_ASSERTS
	    CVMStackMapEntry *realEntry;
	    realEntry = (CVMStackMapEntry*)((CVMUint16*)entryEnd + offset);
	    CVMassert( realEntry->pc == entry->pc );
#endif
	}
	/* Return the entry in the extended region: */
	return (CVMStackMapEntry*)((CVMUint16*)entryEnd + offset);
    } else {
	/* Return the basic entry: */
	return entry;
    }
}

/*
 * Get the stackmap data for a given PC. This is a 16-bit PC value,
 * followed by a bunch of 16-bit chunks.
 */
CVMStackMapEntry*
CVMstackmapGetEntryForPC(CVMStackMaps* smaps, CVMUint16 pc)
{
    /* This is a binary search. */
    CVMStackMapEntry* entries  = &smaps->smEntries[0];
    int lowest = 0;
    int highbound = smaps->noGcPoints;
    int i;
    CVMStackMapEntry* entry;
    CVMUint16 candidatePC;

    /* Do a binary search of the list of stack entries to look for a
       matching PC: */
    while ( lowest < highbound ){
	i = lowest + (highbound-lowest)/2;
	entry = entries+i;
	candidatePC = entry->pc;
	if (candidatePC == pc){
	    /* We've found a matching PC.  Now, get the stackmap entry info: */
	    CVMStackMapEntry *entryEnd = entries + smaps->noGcPoints;
	    return CVMstackmapGetRealEntry(entry, entryEnd);
	}
	if ( candidatePC < pc ){
	    lowest = i+1;
	}else{
	    highbound = i;
	}
    }
    /* Not found! */
    return NULL;
}

/*
 * Is the instruction at 'pc' the target of a JSR?
 */
static CVMBool isJsrTarget(CVMStackmapContext* con, CVMUint8* pc)
{
    /* 
     * The member'jsrPC' of 'CVMJsrTableEntry' contains relative
     * PCs (see declaration of struct 'CVMJsrTableEntry' above).
     * Therefore 32 bits are sufficient regardless of the platform.
     */
    CVMJsrTableEntry* e = CVMstackmapGetJsrTargetEntry(con, (CVMUint32)(pc - con->code));
    if (e != NULL) {
	CVMassert(e->jsrPC == pc - con->code);
	return CVM_TRUE;
    } else {
	return CVM_FALSE;
    }
}

/*
 * %comment rt012
 */
static CVMBool thisPChasAddrAtTOS( CVMStackmapContext *con, CVMUint8* pc ){
    if (isJsrTarget(con, pc)) {
	CVMassert(*pc == opc_astore ||
		  *pc == opc_astore_0 ||
		  *pc == opc_astore_1 ||
		  *pc == opc_astore_2 ||
		  *pc == opc_astore_3 || 
		  ((*pc == opc_wide) && (pc[1] == opc_astore)));
	return CVM_TRUE;
    } else {
	return CVM_FALSE;
    }
}

/*
 * Build look-up table that can quickly answer the question
 * "Does the top-of-stack at a given PC contain a JSR return address?"
 *
static void findPCAtTOS( CVMStackmapContext *con ){
    CVMassert( CVM_FALSE );
}
*/

/*
 * When there are conflicts between Ref's and numeric values or PC (return)
 * values, we split variables. These functions manage that information.
 * When a conflict is first discoverd, remapConflictVarno is called to
 * update the variable mapping to indicate how the given variable is split.
 * When we are rewriting the code accordingly, mapVarno gives us the new
 * number to use.
 */
static void
remapConflictVarno(CVMStackmapContext* con, int varNo){
    int newVar;

    CVMassert( varNo < con->nVars );
    if ( con->varMapSize == 0 ){
	int i;
	con->varMapSize = con->nVars + con->newVars + 2;
	con->newVarMap = (CVMUint32*) malloc( con->varMapSize * sizeof (CVMUint32*));
	if (con->newVarMap == NULL) {
	    throwError(con, CVM_MAP_OUT_OF_MEMORY);
	}
	
	for ( i = 0; i < con->nVars + con->newVars; i++ )
	    con->newVarMap[i] = i;
    }
    if ( con->newVarMap[varNo] != varNo )
	return; /* already remapped. don't do it again */
    if ( con->varMapSize <= con->nVars + con->newVars ){
	int newSize = ((con->varMapSize*5)/4) + 2;
	int i;
	CVMUint32* newMap = (CVMUint32*) malloc(newSize * sizeof (CVMUint32*));
	if (newMap == NULL) {
	    throwError(con, CVM_MAP_OUT_OF_MEMORY);
	}
	
	for ( i = 0; i < con->varMapSize; i++ )
	    newMap[i] = con->newVarMap[i];
	for ( ; i < newSize; i++ )
	    newMap[i] = i;
	free( con->newVarMap );
	con->varMapSize = newSize;
	con->newVarMap = newMap;
    }
    /* The next new variable */
    newVar = con->nVars + con->newVars;
    con->newVarMap[varNo] = newVar;
    /* make sure new maps to self */
    con->newVarMap[newVar] =  newVar;
    con->newVars++; /* Add a new variable */
    if (newVar >= 65535 ){
	CVMtraceStackmaps(("...too many local variables %d\n", newVar ));
	throwError(con, CVM_MAP_EXCEEDED_LIMITS);
    }
}

static int
mapVarno(CVMStackmapContext * con, int varNo, CVMUint8 * pc ){
    int newVarNo;
    CVMassert( varNo < con->varMapSize );
    newVarNo = con->newVarMap[varNo];
    if ( newVarNo == varNo ){
	/* Nothing to do */
	return varNo;
    }
    /*
     * Use when general enough
     * if (con->localRefsWithAddrAtTOS != NULL && 
     * thisPChasAddrAtTOS(con, pc)) {
     */
    if (thisPChasAddrAtTOS(con, pc)) {
	/* Do not re-map if this PC has a return address at tos */
	/* This sort of astore is not a reference astore */
	return varNo;
    } 
    return newVarNo;
}

/* 
 * Management of pc-pc mapping when code expands.
 */

/* Purpose: Computes the size of the needed PCMap. */
static CVMUint32 computePCMapSize(int nExpansionPoints)
{
    CVMUint32 size;
    size = sizeof(CVMstackmapPCMap) +
           ((nExpansionPoints-1) * sizeof(struct CVMstackmapPCMapEntry));
    return size;
}

/* Purpose: Initializes the PCMap. */
static void initializePCMap(CVMstackmapPCMap *pcMap, int nExpansionPoints)
{
    pcMap->mapSize = nExpansionPoints;
    pcMap->nAdjustments = 0;
}

/* Purpose: Allocate and initialize the needed PCMap. */
static CVMstackmapPCMap *
allocatePCMap( int nExpansionPoints ){
    CVMUint32 allocSize;
    CVMstackmapPCMap * v;
    allocSize = computePCMapSize(nExpansionPoints);
    v = (CVMstackmapPCMap*)malloc( allocSize );
    if ( v == NULL ){
	return NULL;
    }
    initializePCMap(v, nExpansionPoints);
    return v;
}

static void
recordExpansion( CVMstackmapPCMap * pcMap, int relativePC, int adjustment ){
    int nextIndex = pcMap->nAdjustments;
    CVMassert( nextIndex < pcMap->mapSize );
    /*
     * Someday we may wish to do an insertion here into an ordered array.
     * For now, we just assert that we are adding at the end.
     */
    if ( nextIndex > 0 ){
	CVMassert( relativePC > pcMap->map[nextIndex-1].addr );
    }
    pcMap->map[nextIndex].addr = relativePC;
    pcMap->map[nextIndex].adjustment = adjustment;
    pcMap->nAdjustments += 1;
}

/* Purpose: Given a pc in the old routine, compute the corresponding in the
            new, adjusted one. */
static CVMInt32
mapPC(CVMstackmapPCMap *pcMap, CVMInt32 relativePC){
    int endIndex = pcMap->nAdjustments;
    int mapIndex = 0;
    unsigned int adjustment = 0;
    while ( (mapIndex < endIndex) && (relativePC > pcMap->map[mapIndex].addr ) ){
	adjustment += pcMap->map[mapIndex].adjustment; 
	mapIndex += 1;
    }
    return relativePC+adjustment;
}

/*
 * Given a pc in the new routine, compute the corresponding
 * in the old, un-adjusted one.
 */
static CVMInt32
reverseMapPC(const CVMstackmapPCMap *pcMap, CVMInt32 relativePC) {
    int endIndex = pcMap->nAdjustments;
    int mapIndex = 0;
    while ( (mapIndex < endIndex) && (relativePC > pcMap->map[mapIndex].addr ) ){
	relativePC -= pcMap->map[mapIndex].adjustment; 
        if (relativePC <= pcMap->map[mapIndex].addr) {
            /* If we get here, then the specified offset must correspond to
               synthesized opcodes like the padding "opc_nop"s.  Hence, return
               the address of the opcode which was inflated to create this
               opcode: */
            return pcMap->map[mapIndex].addr;
        }
	mapIndex += 1;
    }
    return relativePC;
}

#ifdef CVM_JVMPI_TRACE_INSTRUCTION

/* Purpose: Gets the PCMap for the specified jmd. */
static CVMstackmapPCMap *CVMstackmapGetJmdPCMap(CVMJavaMethodDescriptor *jmd)
{
    CVMstackmapPCMap *pcMap;
    CVMUint32 pcMapOffset;
    CVMassert(CVMjmdFlags(jmd) & CVM_JMD_DID_REWRITE);
    pcMapOffset = CVMjmdTotalSize(jmd) + sizeof(struct CVMJavaMethodExtension);
    pcMapOffset = CVMalign4(pcMapOffset);
    pcMap = (CVMstackmapPCMap *) ((char *)jmd + pcMapOffset);
    return pcMap;
}

/* Purpose: Maps a Java method's PC offset back to its original PC offset
            prior to stackmap disambiguation. */
/* Returns: A negative value if the specified PC offset is for code that
            is added to the original and hence does not have an original
            PC offset equivalent. */
/* NOTE: This function is based on reverseMapPC() with the addition of the
         check to see if the bytecodes at the specified offset does not exist
         in the original.  If so, return a negative value. */
CVMInt32 CVMstackmapReverseMapPC(CVMJavaMethodDescriptor *jmd, int newOffset)
{
    CVMstackmapPCMap *pcMap = CVMstackmapGetJmdPCMap(jmd);

    int endIndex = pcMap->nAdjustments;
    int mapIndex = 0;
    CVMInt32 origOffset = newOffset;

    while ((mapIndex < endIndex) && (origOffset > pcMap->map[mapIndex].addr)) {
        origOffset -= pcMap->map[mapIndex].adjustment; 
        /* The should only be an original opcode at the expansion point i.e. at
           address pcMap->map[mapIndex].addr.  The next original opcode must be
           greater than pcMap->map[mapIndex].addr. */
        if (origOffset <= pcMap->map[mapIndex].addr) {
            /* If we get here, then the specified offset must correspond to
               synthesized opcodes like the padding "opc_nop"s.  Hence, return
               a negative number to indicate that there is no original
               equivalent to the specified offset. */
            return -1;
        }
        mapIndex += 1;
    }
    return origOffset;
}

/* Purpose: Merge the PCMap data. */
static void CVMstackmapMergePCMaps(CVMstackmapPCMap *newPCMap,
                                   const CVMstackmapPCMap *oldPCMap,
                                   const CVMstackmapPCMap *currentPCMap)
{
    int newIndex = 0;
    int oldIndex = 0;
    int currentIndex = 0;

    CVMassert(currentPCMap != NULL);
    CVMassert(oldPCMap != NULL);
    CVMassert(newPCMap != NULL);

    /* NOTE: The PCMaps entries are structured as pairs of the following
       values:
       1. The original PC offset of the bytecode which underwent re-writing.
          This value is stored in the addr field.
       2. The increase in size due to the re-writing of the bytecode plus any
          padding with 'opc_nop's that is added for alignment.  This value is
          stored in the adjustment field.

       oldPCMap maps PCs of the previous form of the method to the PCs of the
       absolute original form of the method before it went through any
       re-writing.  oldPCMap is attained from the end of the old JMD record.

       currentPCMap maps the PCs in the current re-written method to the PCs of
       the previous form of the method.  currentPCMap is computed in this
       function.

       newPCMap maps the PCs in the current re-written method to the PCs of the
       absolute original form of the method.  newPCMap is computed by merging
       the data in oldPCMap and currentPCMap in a meaningful way.  If this
       method goes through another pass of re-writing again, then newPCMap will
       become the oldPCMap in the next pass.

       Hence, to merge the currentPCMap with the oldPCMap, each entry in the
       currentPCMap is reverse mapped into the original PC range using the
       oldPCMap.  If the resultant PC offset is already in the oldPCMap, then
       adjustment values are added together.  Else, if the resultant PC offset
       is not in the oldPCMap yet, then a new entry is inserted.
    */

    /* Copy the oldPCmap to the newPCMap: */
    while (oldIndex < oldPCMap->nAdjustments) {
        CVMassert(newIndex < newPCMap->mapSize);
        newPCMap->map[newIndex++] = oldPCMap->map[oldIndex++];
    }
    newPCMap->nAdjustments = newIndex;

    /* Merge in the currentPCMap info: */
    for (; currentIndex < currentPCMap->nAdjustments; currentIndex++) {

        int addr = currentPCMap->map[currentIndex].addr;
        int adjustment = currentPCMap->map[currentIndex].adjustment;
        int i;

        /* Map the addr into the original PC space: */
        addr = reverseMapPC(oldPCMap, addr);

        /* Find the place to merge/insert the entry: */
        for (i = 0; i < newPCMap->nAdjustments; i++) {
            int entryAddr = newPCMap->map[i].addr;

            /* If the addresses are the same, merge the adjustments: */
            if (addr == entryAddr) {
                newPCMap->map[i].adjustment += adjustment;
                break;

            /* Else, find the range where this addr fits into: */
            } else {
                int nextAddr = 0x7FFFFFFF;
                /* The nextAddr is either the beginning of the next range,
                   or some large ridiculous number (set above by default)
                   to represent the offset at the bytecode at end of the
                   method.  It's OK to use this large number because it is
                   only use to denote the end range and not actually used
                   for mapping bytecode offsets. */

                if ((i + 1) < newPCMap->nAdjustments) {
                    nextAddr = newPCMap->map[i + 1].addr;
                }

                /* Insert the entry if it is in the range: */
                if ((addr > entryAddr) && (addr < nextAddr)) {

                    /* Make sure we have adequate space to insert the entry: */
                    CVMassert(newPCMap->nAdjustments < newPCMap->mapSize);

                    /* If we're not inserting the entry at the end of the list,
                       then we have to shift some elements down: */
                    if (i + 1 < newPCMap->nAdjustments) {
                        memmove(&newPCMap->map[i+2], &newPCMap->map[i+1],
                                newPCMap->nAdjustments *
                                sizeof(CVMstackmapPCMapEntry));
                    }

                    /* Write in the new entry: */
                    newPCMap->map[i+1].addr = addr;
                    newPCMap->map[i+1].adjustment = adjustment;
                    newPCMap->nAdjustments++;
                }
            }
        }
    }
}

#endif

/*
 * Estimate (conservatively) how much to expand the code in order to 
 * resolve conflicts. Return both the number of bytes of expansion
 * and the number of expansion points. The former is used for allocation
 * of the code array. The latter is used for allocation of the pc-mapping
 * array.
 */
static CVMUint32
CVMstackmapComputeExpansionSize( CVMStackmapContext * con, CVMUint32 *nPoints ){
    int 	nExpansionPoints = 0;
    int 	nExpansionBytes = 0;
    CVMUint8*   pc;
    CVMUint8*   endPC;
    CVMOpcode 	instr;
    int		thisInstrLength;
    int		i;
    /*
     * First count up all the initializations required.
     */
    if ( con->numRefVarsToInitialize != 0 ){
	/*
	 * This many nulls to begin with.
	 */
	nExpansionBytes += CVMopcodeLengths[opc_aconst_null] *
	    con->numRefVarsToInitialize;
	/*
	 * Now get the total size of initialization instructions needed.
	 */
	for (i = 0; i < con->nVars; i++) {
	    if (con->refVarsToInitialize[i] != 0) {
		if (i <= 3) {
		    nExpansionBytes += CVMopcodeLengths[opc_astore_0];
		} else if (i <= 255) {
		    nExpansionBytes += CVMopcodeLengths[opc_astore];
		} else {
		    CVMassert(i <= 65535);
		    /*
		     * Need opc_wide here.
		     */
		    nExpansionBytes += CVMopcodeLengths[opc_astore] + 2;
		}
	    }
	}
	/*
	 * Round up to the next word. This operation is rare
	 * enough that a few bytes won't make a difference.
	 */
	nExpansionBytes = CVMalign4(nExpansionBytes);
	nExpansionPoints += 1; /* one big expansion at the head */
    }
    /*
     * Now dredge through the code, finding all the aload and astore
     * instructions that need to be changed, and calculating how much
     * their sizes must change.
     * In this loop, we ignore any "wide" instructions completely,
     * since we will never have to widen those. However, when
     * doing the remapping, remember to find those as well!
     */
    if ( con->nRefValConflicts + con->nRefPCConflicts > 0 ){
	for ( pc= con->code, endPC=pc+con->codeLen-1;
	    pc <= endPC; 
	    pc += thisInstrLength )
	{
	    unsigned int varNo;
	    unsigned int mappedVarNo;
	    unsigned int newInstrLength;

	    thisInstrLength = getOpcodeLength(con, pc);
	    switch ( instr = (CVMOpcode)*pc ){
	    case opc_aload_0:
	    case opc_aload_1:
	    case opc_aload_2:
	    case opc_aload_3:
		varNo = instr - opc_aload_0;
		goto aLoadStore;
	    case opc_astore_0:
	    case opc_astore_1:
	    case opc_astore_2:
	    case opc_astore_3:
		varNo = instr - opc_astore_0;
		goto aLoadStore;
	    case opc_aload:
	    case opc_astore:
		varNo = pc[1];
	    aLoadStore:
		if ( (mappedVarNo = mapVarno( con, varNo, pc ) ) != varNo ){
		    if ( mappedVarNo <= 3 )
			newInstrLength = CVMopcodeLengths[opc_astore_0];
		    else if ( mappedVarNo <= 255 )
			newInstrLength = CVMopcodeLengths[opc_astore];
		    else 
			newInstrLength = CVMopcodeLengths[opc_astore]+2;
		    if ( newInstrLength > thisInstrLength ){
			nExpansionPoints += 1;
			nExpansionBytes  += CVMalign4( newInstrLength - thisInstrLength );
		    }
		}
	    default:
		continue;
	    }
	}
    }

    *nPoints = nExpansionPoints;
    return nExpansionBytes;
}

/*
 * Write initialization code.
 * record it in the pc mapping structure
 */
static CVMUint8 *
initializeUninitRefs(
    CVMStackmapContext *con,
    CVMUint8 * newCode,
    CVMstackmapPCMap *map)
{
    CVMUint8*	prependedCode = newCode;
    CVMUint32	numPaddingNops;
    int nVars = CVMjmdMaxLocals(con->jmd);
    int i;
    for (i = 0; i < nVars; i++) {
	if (con->refVarsToInitialize[i] != 0) {
	    /*
	     * First push a null.
	     */
	    *prependedCode++ = opc_aconst_null;
	    /*
	     * And now store it into the appropriate local.
	     */
	    if (i <= 3) {
		switch(i) {
		case 0: *prependedCode++ = opc_astore_0; break;
		case 1: *prependedCode++ = opc_astore_1; break;
		case 2: *prependedCode++ = opc_astore_2; break;
		case 3: *prependedCode++ = opc_astore_3; break;
		default: CVMassert(CVM_FALSE);
		}
	    } else if (i <= 255) {
		*prependedCode++ = opc_astore;
		*prependedCode++ = i;
	    } else {
		CVMassert(i <= 65535);
		*prependedCode++ = opc_wide;
		*prependedCode++ = opc_astore;
		WRITE_INT16( prependedCode, i );
		prependedCode += 2;
	    }
	}
    }
    /*
     * Pad the rest with nop's
     */
    numPaddingNops = (CVMUint32)((CVMUint8*)CVMalignWordUp( prependedCode ) - prependedCode);
    for (i = 0; i < numPaddingNops; i++) {
	*prependedCode++ = opc_nop;
    }
    /*
     * record what we just did, return address of end of work.
     * -1 is an address before the beginning.
     */
    recordExpansion( map, -1, (int)(prependedCode-newCode) );
    return prependedCode;
}

/*
 * Write an aload or an astore instruction.
 * If lenghtening is indicated, always write a sequence of instructions that
 * will be 4 greater than the natural length. The best (?) way to do this
 * is to write the wide form (4 bytes), followed by a number of opc_nop equal to
 * the length of the old instruction.
 * Return number of bytes written.
 */
static int
CVMstackmapPutVarRef(
    CVMUint8 *	newCode,
    CVMBool	isLoad,
    unsigned	varNo,
    unsigned	oldInstrLength
){

    int     newInstrLength;
    CVMBool mustLengthen;
    /*
     * Figure out how long an instruction we need in the minimum case.
     */
    if ( varNo <= 3 )
	newInstrLength = CVMopcodeLengths[opc_astore_0];
    else if ( varNo <= 255 )
	newInstrLength = CVMopcodeLengths[opc_astore];
    else 
	newInstrLength = CVMopcodeLengths[opc_astore]+2;

    /*
     * Figure out if we can fit it into the old space.
     * If possible, use the same instruction format as before.
     */
    if (newInstrLength <= oldInstrLength ){
	newInstrLength = oldInstrLength;
	mustLengthen = CVM_FALSE;
    } else {
	newInstrLength = CVMopcodeLengths[opc_astore]+2; 
	mustLengthen = CVM_TRUE;
    }
    switch ( newInstrLength ){
    case 1:
	newCode[0] = ( isLoad?opc_aload_0:opc_astore_0 ) + varNo;   
	return 1;
    case 2:
	newCode[0] = isLoad ? opc_aload : opc_astore;
	newCode[1] = varNo;
	return 2;
    case 4:
	newCode[0] = opc_wide;
	newCode[1] = isLoad ? opc_aload : opc_astore;
	WRITE_INT16( &newCode[2], varNo );
	break;
    default:
	CVMassert(CVM_FALSE);
    }
    if ( mustLengthen ){
	int i;
	for ( i = 0; i < oldInstrLength; i++ ){
	    newCode[i+4] = opc_nop;
	}
	newInstrLength += oldInstrLength;
    }
    return newInstrLength;
}

/*
 * There are ref-val and ref-pc conflicts.
 * Copy code over, fixing the conflicts, and recording what we do.
 * branches and other code ref's will get twiddled later based on
 * our changes.
 */
static CVMUint8 *
CVMstackmapRemapConflictVars(
    CVMStackmapContext* con,
    CVMUint8 *		newCode,
    CVMstackmapPCMap *	pcMap
){
    CVMUint8 *	oldPC = con->code;
    CVMUint8 *	startPC = oldPC;
    CVMUint8 *	endPC = oldPC+con->codeLen;
    CVMUint8 *	lastCopy = oldPC;
    CVMUint32	varNo;
    CVMUint32	mappedVarNo;
    CVMUint32	thisInstrLength;
    CVMUint32	newInstrLength;
    CVMUint8    instr;
    CVMBool     isLoad;

    for ( ; oldPC < endPC; oldPC += thisInstrLength ){
	thisInstrLength = getOpcodeLength(con, oldPC);
	switch ( instr = *oldPC ){
	case opc_wide:
	    if ( oldPC[1] == opc_aload ){
		isLoad = CVM_TRUE;
		varNo = CVMgetUint16(oldPC+2);
		goto aLoadStore;
	    }else if ( oldPC[1] == opc_astore ){
		isLoad = CVM_FALSE;
		varNo = CVMgetUint16(oldPC+2);
		goto aLoadStore;
	    }
	    continue;
	case opc_aload_0:
	case opc_aload_1:
	case opc_aload_2:
	case opc_aload_3:
	    varNo = instr - opc_aload_0;
	    isLoad = CVM_TRUE;
	    goto aLoadStore;
	case opc_astore_0:
	case opc_astore_1:
	case opc_astore_2:
	case opc_astore_3:
	    varNo = instr - opc_astore_0;
	    isLoad = CVM_FALSE;
	    goto aLoadStore;
	case opc_aload:
	    varNo = oldPC[1];
	    isLoad = CVM_TRUE;
	    goto aLoadStore;
	case opc_astore:
	    isLoad = CVM_FALSE;
	    varNo = oldPC[1];
	    /* FALL THROUGH */
	aLoadStore:
	    if ( (mappedVarNo = mapVarno( con, varNo, oldPC ) ) != varNo ){
		break;
	    }
	    continue;
	default:
	    continue;
	}
	/*
	 * copy over all the bytes we examined without remapping.
	 */
	if ( lastCopy < oldPC ){
	    int nBytes = (int)(oldPC - lastCopy);
	    memmove(newCode, lastCopy, nBytes);
	    lastCopy = oldPC;
	    newCode += nBytes;
	}
        /* Write out the new re-written opcode to the newCode buffer: */
	newInstrLength = CVMstackmapPutVarRef( newCode, isLoad, mappedVarNo,
			thisInstrLength );
	newCode += newInstrLength;
	lastCopy += thisInstrLength; /* don't re-copy this instruction. */
	if ( newInstrLength != thisInstrLength ){
	    CVMtraceStackmaps(("... changed length from %d to %d at %d\n",
		thisInstrLength, newInstrLength, oldPC-startPC ));
	    recordExpansion( pcMap, (int)(oldPC-startPC), (int)(newInstrLength-thisInstrLength) );
	}
	/* oldPC += thisInstrLength; is part of the for-loop so don't here */
    }
    /*
     * copy over trailing bytes we examined without remapping.
     */
    if ( lastCopy < oldPC ){
	int nBytes = (int)(oldPC - lastCopy);
	memmove(newCode, lastCopy, nBytes);
	lastCopy = oldPC;
	newCode += nBytes;
    }
    return newCode;
}

/*
 * The JavaMethodDescriptor contains code and other data structures.
 * All of these have references into the code which are now incorrect
 * because of the rewriting we have done.
 * The pcMap tells us where code has expanded, and thus how to
 * repair the broken references.
 */
static void
CVMstackmapRelocateCodeReferences(
    CVMStackmapContext*		con,
    CVMJavaMethodDescriptor *	jmd,
    CVMstackmapPCMap *		pcMap
){
    CVMUint8*	pc = CVMjmdCode(jmd);
    CVMUint8*	startPC = pc;
    CVMUint8*	lastPc = pc+CVMjmdCodeLength(jmd);
    CVMUint8	instr;
    int		instrLength;
    int		oldInstrOffset;
    int		newInstrOffset;
    int 	oldTargetOffset;
    int		newTargetOffset;
    /*
     * First fix the code itself.
     */
#define mapBranchOffset( _oldOffset, _oldPC, _newPC, _map )\
	(mapPC( (_map), (_oldOffset)+(_oldPC) ) - (_newPC) )

    for ( ; pc < lastPc; pc += instrLength ){
	instrLength = getOpcodeLength(con, pc);
	instr = *pc;
	if (CVMbcAttr(instr, BRANCH)) {
	    newInstrOffset = (int)(pc-startPC);
	    oldInstrOffset = reverseMapPC( pcMap, newInstrOffset );
	    switch(instr) {
	        case opc_goto_w:
	        case opc_jsr_w: {
		    /* An unconditional goto, 4-byte offset */
		    CVMInt32 offset = CVMgetInt32(pc+1);
		    newTargetOffset = mapBranchOffset( offset, oldInstrOffset, newInstrOffset, pcMap );
		    WRITE_INT32( pc+1, newTargetOffset );
		    break;
	        }
	        case opc_lookupswitch: {
		    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(pc+1);
		    CVMInt32  skip = CVMgetAlignedInt32(lpc); /* default */
		    CVMInt32  npairs = CVMgetAlignedInt32(&lpc[1]);
		    /* adjust the the default */
		    newTargetOffset = mapBranchOffset( skip, oldInstrOffset, newInstrOffset, pcMap );
		    CVMputAlignedInt32( lpc, newTargetOffset );
		    /* And all the possible case arms */
		    lpc += 3; /* Go to the first offset */
		    while (--npairs >= 0) {
			skip = CVMgetAlignedInt32(lpc);
			newTargetOffset = mapBranchOffset( skip, oldInstrOffset, newInstrOffset, pcMap );
			CVMputAlignedInt32( lpc, newTargetOffset );
			lpc += 2; /* next offset */
		    }
		    break;
	        }
	        case opc_tableswitch: {
		    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(pc+1);
		    CVMInt32  skip = CVMgetAlignedInt32(lpc); /* default */
		    CVMInt32  low  = CVMgetAlignedInt32(&lpc[1]);
		    CVMInt32  high = CVMgetAlignedInt32(&lpc[2]);
		    CVMInt32  noff = high - low + 1;
		    /* First do the default */
		    newTargetOffset = mapBranchOffset( skip, oldInstrOffset, newInstrOffset, pcMap );
		    CVMputAlignedInt32( lpc, newTargetOffset );

		    lpc += 3; /* Skip default, low, high */
		    while (--noff >= 0) {
			skip = CVMgetAlignedInt32(lpc);
			newTargetOffset = mapBranchOffset( skip, oldInstrOffset, newInstrOffset, pcMap );
			CVMputAlignedInt32( lpc, newTargetOffset );
			lpc++;
		    }
		    break;
	        }
	        default: {
		    /* This had better be one of the 'if' guys */
		    /* or the usual goto or jsr instructions   */
		    CVMassert(instrLength == 3);
		    /* Mark the target of the 'if' */
		    oldTargetOffset = CVMgetInt16(pc+1);
		    newTargetOffset = mapBranchOffset( oldTargetOffset, oldInstrOffset, newInstrOffset, pcMap );
		    if ( newTargetOffset > 0x7fff || 
			 newTargetOffset < -0x8000 ){
			CVMtraceStackmaps(("...trying to make big branch %d\n",
					   newTargetOffset));
			throwError(con, CVM_MAP_EXCEEDED_LIMITS);
		    }
		    WRITE_INT16( (pc+1), newTargetOffset );
		}
	    }
	}
    }
    /*
     * Now that the code is better, repair all the other
     * structures as well.
     */
    if (con->nExceptionHandlers > 0) {
	CVMExceptionHandler* excTab = CVMjmdExceptionTable(jmd);
	CVMExceptionHandler* excTabEnd = excTab + con->nExceptionHandlers;
	/*
	 * Update the exception tables
	 */
	while (excTab < excTabEnd) {
	    excTab->startpc   = mapPC( pcMap, excTab->startpc );
	    excTab->endpc     = mapPC( pcMap, excTab->endpc );
	    excTab->handlerpc = mapPC( pcMap, excTab->handlerpc );
	    excTab++;
	}
    }
#ifdef CVM_DEBUG_CLASSINFO
    /*
     * Update the class debugging info
     */
    {
	CVMLineNumberEntry* lineNoTab = CVMjmdLineNumberTable(jmd);
	CVMLineNumberEntry* lineNoTabEnd =
	    lineNoTab + CVMjmdLineNumberTableLength(jmd);
	/*
	 * Update the line number tables
	 */
	while (lineNoTab < lineNoTabEnd) {
	    lineNoTab->startpc = mapPC( pcMap, lineNoTab->startpc );
	    lineNoTab++;
	}
    }
    {
	CVMLocalVariableEntry* localvarTab = CVMjmdLocalVariableTable(jmd);
	CVMLocalVariableEntry* localvarTabEnd =
	    localvarTab + CVMjmdLocalVariableTableLength(jmd);
	/*
	 * Update the local variables table
	 */
	while (localvarTab < localvarTabEnd) {
	    localvarTab->startpc = mapPC( pcMap, localvarTab->startpc );
	    localvarTab++;
	}
    }
#endif /* CVM_DEBUG_CLASSINFO */
    
}

/*
 * We have recorded some ref-uninit conflicts for this method. Be sure
 * to rewrite this method to provide initialization for these variables.
 */
/*
 * Rewrite code in order to resolve conflicts.
 * There are three sorts of problems:
 * ref-unit conflicts
 *	These are resolved by beginning the method with initialization
 *	code of the form
 *		aconst_null
 *		astore n
 * ref-val conflicts
 *	These are resolved by allocating another local variable and
 *	re-mapping all ref-uses of the conflicted variable to the new one.
 *	This can be determined statically, by looking at the opcode type.
 * ref-pc conflicts
 *	Similar to ref-val conflicts, but we have to take another pass over
 *	the code to determine which astore (and I suppose aload) instructions
 *	are of the pc type and which are of the ref type, since they all use
 * 	the a-type instructions.
 *
 * Pre-pending initialization code obviously makes the code bigger.
 * Re-targeting aload and astore instructions can, too, if the new local
 * variable name will no longer fit into the the old instruction. For instance
 * to re-map aload_0 to use local 15 changes a 1-byte instructions into 
 * 2 bytes, at least.
 *
 * Our strategy is always to change the code length in increments of 4 bytes,
 * to maintain alignment of switch instructions. We could apply more cleverness
 * here and actually re-write those instructions with different padding, but
 * it doesn't seem worthwhile, given the infrequence of this occurence.
 * Thus we will often use longer instructions than absolutely necessary,
 * and pad with no-ops as required.
 *
 * The aload/store re-mapping will also require us to look for and
 * modify branch instructions. The pre-pending remapping does not, at
 * all those instructions are self-relative. In any case, auxiliary
 * tables containing PC offsets must be modified.
 */
static void
CVMstackmapRewriteMethodForConflicts(CVMStackmapContext* con)
{
    CVMUint32 expandBy = 0;
    CVMUint32 nExpansionPoints = 0;
    CVMJavaMethodDescriptor* newJmd;
    CVMJavaMethodDescriptor* oldJmd = con->jmd;
    CVMUint32 newJMDAllocationSize;
    CVMUint8* newCode;
    /* currentPCMap holds the adjustments this function makes. */
    CVMstackmapPCMap *currentPCMap;
    struct CVMJavaMethodExtension * extension;

#ifdef CVM_JVMPI_TRACE_INSTRUCTION
    /* If a jmd's method has been rewritten then the jmd's memory
       includes a PCMap.
       oldPCMap points to oldJmd's PCMap, or NULL if it doesn't have one.
       newPCMap points to newJmd's PCMap.
       When we're done, oldPCMap and currentPCMap will be merged to
       newPCMap and currentPCMap will be freed, unless oldPCMap is
       NULL in which case we just point currentPCMap to newPCMap and
       fill it in directly as we go along. */
    CVMstackmapPCMap *oldPCMap = NULL;
    CVMstackmapPCMap *newPCMap;

    if (CVMjmdFlags(oldJmd) & CVM_JMD_DID_REWRITE) {
        oldPCMap = CVMstackmapGetJmdPCMap(oldJmd);
    }
#endif

    expandBy = CVMstackmapComputeExpansionSize( con, &nExpansionPoints );

    CVMtraceStackmaps(("*** Method (%C.%M) expanding by %d @ %d points in stackmap rewriting\n",
			     con->cb, con->mb, expandBy, nExpansionPoints));
    if ( con->codeLen + expandBy > 65535 ){
	CVMconsolePrintf("*** WARNING: Method (%C.%M) got too big in stackmap rewriting\n",
				 con->cb, con->mb);
    }
    /*
     * OK, now we know the sizes involved. First make a copy of the
     * JavaMethodDescriptor with all its trailing data structures, and leave
     * room for the new code.
     */
    /* 
     * Make sure that we have enough room for the correct alignment of
     * 'extension' (see towards the end of this function). Fortunately 
     * 'extension' is computed using 'newJMDAllocationSize' so fixing
     * 'newJMDAllocationSize' here makes things work there as well.
     */
    newJMDAllocationSize = CVMalignAddrUp(expandBy + CVMjmdTotalSize(oldJmd)) + sizeof(struct CVMJavaMethodExtension);

#ifdef CVM_JVMPI_TRACE_INSTRUCTION
    /* Add adjustment to the newJMDAllocationSize to store the cummulative
       PCMap information: */
    {
        CVMUint32 size;
        CVMUint32 newPCMapSize;
        CVMUint32 newExpansionPoints = nExpansionPoints;
        if (oldPCMap != NULL) {
            /* NOTE: The worst case for the size of the newPCMap is equal to
              the sum of the PCMaps. */
            newExpansionPoints += oldPCMap->nAdjustments;
        }

        newPCMapSize = computePCMapSize(newExpansionPoints);
        size = CVMalign4(newJMDAllocationSize) + newPCMapSize;
        newJmd = calloc(1, size);
        if (newJmd == NULL) {
            throwError(con, CVM_MAP_OUT_OF_MEMORY);
        }
        newPCMap = (CVMstackmapPCMap *)((char *)newJmd + size - newPCMapSize);
        initializePCMap(newPCMap, newExpansionPoints);

        if (oldPCMap == NULL) {
            /* If we have no oldPCMap info to merge in, then let the new PCMap
               be the current PCMap (saves us from having to copy it over
               later): */
            currentPCMap = newPCMap;
        } else {
            currentPCMap = allocatePCMap(nExpansionPoints);
            if (currentPCMap == NULL) {
                free(newJmd);
                throwError(con, CVM_MAP_OUT_OF_MEMORY);
            }
        }
    }
#else
    newJmd = (CVMJavaMethodDescriptor *)calloc(1, newJMDAllocationSize );
    if (newJmd == NULL) {
        throwError(con, CVM_MAP_OUT_OF_MEMORY);
    }
    /* Allocate and initialize the current PCMap: */
    currentPCMap = allocatePCMap(nExpansionPoints);
    if (currentPCMap == NULL) {
        free(newJmd);
        throwError(con, CVM_MAP_OUT_OF_MEMORY);
    }
#endif

    /*
     * First copy the CVMJavaMethodDescriptor struct.
     */
    memmove(newJmd, oldJmd, sizeof(CVMJavaMethodDescriptor));

    newCode = (CVMUint8*)newJmd + sizeof(CVMJavaMethodDescriptor);
    /*
     * Add initialization for variables that need it.
     */
    if ( con->numRefVarsToInitialize != 0 ){
        newCode = initializeUninitRefs(con, newCode, currentPCMap);
    }
    if ( con->nRefValConflicts + con->nRefPCConflicts > 0 ){
	CVMUint8* trailingMoveSrc;
	int trailingMoveLength;
	CVMUint8* trailingMoveDest;
	/*
	 * Remap to avoid conflict.
	 */
        newCode = CVMstackmapRemapConflictVars(con, newCode, currentPCMap);
	/*
	 * Then copy over the rest of the trailing structures
	 * All the code is done at this point.
	 */
	trailingMoveSrc = CVMjmdCode(oldJmd)+CVMjmdCodeLength(oldJmd);
	trailingMoveDest = newCode;
	trailingMoveLength = CVMjmdTotalSize(oldJmd)
		- sizeof(CVMJavaMethodDescriptor) - CVMjmdCodeLength(oldJmd);

	memmove(trailingMoveDest,
		trailingMoveSrc,
		trailingMoveLength );
    } else {
	/*
	 * Then copy the rest of the code and the trailing structures,
	 * leaving room for the initialization code.
	 */
	memmove((CVMUint8*)newJmd + sizeof(CVMJavaMethodDescriptor) + expandBy,
		CVMjmdCode(oldJmd),
		CVMjmdTotalSize(oldJmd) - sizeof(CVMJavaMethodDescriptor));
	newCode += CVMjmdCodeLength(oldJmd);
    }
    /*
     * DEBUG
     * Dump out the maps so we can see what happened. Sort of.
     *
    CVMconsolePrintf("*** REWRITE INFO FOR METHOD (%C.%M)\n", con->cb, con->mb);
    if ( con->varMapSize != 0 ){
	int i;
	CVMconsolePrintf("    Variables split:\n");
	for ( i = 0; i < con->nVars; i++ ){
	    if ( con->newVarMap[i] != i ){
		CVMconsolePrintf("	%d => %d\n", i, con->newVarMap[i] );
	    }
	}
    }
    {
        struct CVMstackmapPCMapEntry *m = currentPCMap->map;
	int i;
	CVMconsolePrintf("    Code expansion points:\n");
	for ( i = 0; i < currentPCMap->nAdjustments; i++ ){
	    CVMconsolePrintf("	pc %d added %d\n", m[i].addr, m[i].adjustment );
	}
    }
    if ( newCode != (CVMUint8*)newJmd + sizeof(CVMJavaMethodDescriptor) + CVMjmdCodeLength(newJmd)+ expandBy){
	int v= 
    newCode - ((CVMUint8*)newJmd + sizeof(CVMJavaMethodDescriptor) + CVMjmdCodeLength(newJmd)+ expandBy);
	CVMconsolePrintf("!!!mismatch length: new is %d (longer) than expected\n", v);
    }
    * END DEBUG */
    /*
     * Make sure we've filled in no more than necessary.
     */
    CVMassert( newCode ==
	      (CVMUint8*)newJmd + sizeof(CVMJavaMethodDescriptor) + CVMjmdCodeLength(newJmd)+ expandBy);
    /*
     * Update the codeLength and maxLocals fields to reflect the rewritten method.
     * Note in the new structure that it is a replacement.
     * Replace the stackmap indirect cell with our own. This would only
     * matter if the old one is being deallocated, but since we already
     * made space for it, we'll use it and forgo fixing this bug later.
     */
    CVMjmdCodeLength(newJmd) += expandBy;
    CVMjmdMaxLocals(newJmd) = con->nVars + con->newVars;
    CVMassert(con->nVars == CVMjmdMaxLocals(oldJmd));
    CVMjmdCapacity(newJmd) += con->newVars;
    CVMjmdFlags(newJmd) |= CVM_JMD_DID_REWRITE;

    extension = (struct CVMJavaMethodExtension*) ( (char*)newJmd
			+ newJMDAllocationSize
			- sizeof(struct CVMJavaMethodExtension) );
    CVMassert((CVMAddr)CVMalignAddrUp(extension) == (CVMAddr)extension);

    /*
     * Update everything PC-dependent in the new Jmd. This includes
     * exception tables, line number tables, local variable tables,
     * and stackmaps.
     */
    CVMstackmapRelocateCodeReferences(con, newJmd, currentPCMap);

#ifdef CVM_JVMPI_TRACE_INSTRUCTION
    /* If we have an old PCMap to merge with, ... */
    if (oldPCMap != NULL) {
        /* Merge the oldPCMap and currentPcMap into the newPCMap: */
        CVMstackmapMergePCMaps(newPCMap, oldPCMap, currentPCMap);
    }
#endif
    /*
     * Replace the CVMJavaMethodDescriptor with the new one.
     * If the old one alread was a replacement, free it.
     */
    CVMmbJmd(con->mb) = newJmd;

    /* Make sure that the expression we are going to use to find
       the CVMJavaMethodExtension is going to work. */
    CVMassert(CVMjmdEndPtr(newJmd) == (CVMUint8*)extension);

    if ( CVMjmdFlags(oldJmd) & CVM_JMD_DID_REWRITE ){
	extension->originalJmd = 
	    ((struct CVMJavaMethodExtension*)CVMjmdEndPtr(oldJmd))->originalJmd;
	free( oldJmd );
    } else {
	extension->originalJmd = oldJmd;
    }

    /* 
     * extension->originalJmd has to start at an aligned address
     */
    CVMassert((CVMAddr)CVMalignAddrUp(&extension->originalJmd) == 
	      (CVMAddr)&extension->originalJmd);

#ifdef CVM_JVMPI_TRACE_INSTRUCTION
    if (oldPCMap != NULL)
#endif
        free(currentPCMap);
}

/*
 * Initialization and destruction routines
 */
void
CVMstackmapComputerInit()
{
}

void
CVMstackmapComputerDestroy()
{
}

