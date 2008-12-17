/*
 * @(#)jitstats.h	1.11 06/10/10
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

#ifndef _INCLUDED_JITSTATS_H
#define _INCLUDED_JITSTATS_H

#ifdef CVM_JIT_COLLECT_STATS
#include "javavm/include/porting/time.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/jit/jitmemory.h"

/*
 * JIT statistics
 *
 * These count the number of events of certain kinds and report on the
 * count of each.
 * 
 * The statistics may be obtained at all levels of the JIT from the front-end
 * all the way back to the instruction emitter.
 *
 * Use CVMJITstatsRecord(con, tag) to record an incidence of an event.
 * 
 * If the JIT_STATS trace flag is enabled, we get a report of all events
 */
enum CVMJITStatsTag {
    /* We reserve the first range of enum values for the JIT alloc tags: */
    CVMJIT_STATS_FIRST_WORKING_MEM_STATS = JIT_ALLOC_IRGEN_NODE,
    CVMJIT_STATS_TOTAL_WORKING_MEMORY = JIT_ALLOC_NUM_ALLOCATION_KINDS,

    CVMJIT_STATS_MAX_IR_TREE_DEPTH,
    CVMJIT_STATS_COMPUTED_COMP_STACK_MAX,
    CVMJIT_STATS_ACTUAL_MATCH_STACK_MAX,
    CVMJIT_STATS_ACTUAL_SYNTHESIS_STACK_MAX,
    CVMJIT_STATS_ACTUAL_ACTION_STACK_MAX,
    CVMJIT_STATS_ACTUAL_RESOURCE_STACK_MAX,
    CVMJIT_STATS_ACTUAL_MATCH_STACK_WASTE,
    CVMJIT_STATS_ACTUAL_SYNTHESIS_STACK_WASTE,
    CVMJIT_STATS_ACTUAL_ACTION_STACK_WASTE,
    CVMJIT_STATS_ACTUAL_RESOURCE_STACK_WASTE,

    /* Start of IR Node Stats tags: */
    CVMJIT_STATS_FIRST_NODE_STATS,
    CVMJIT_STATS_TOTAL_NUMBER_OF_NODES = CVMJIT_STATS_FIRST_NODE_STATS,
    CVMJIT_STATS_NUMBER_OF_NULL_CHECK_NODES,   /* Number of gen null checks */
    CVMJIT_STATS_NULL_CHECKS_ELIMINATED,       /* Number of eliminated nulls */
    CVMJIT_STATS_NUMBER_OF_BOUNDS_CHECK_NODES, /* Number of gen bound checks */
    CVMJIT_STATS_BOUNDS_CHECKS_ELIMINATED,     /* Number of gen bound checks */
    CVMJIT_STATS_NUMBER_OF_INVOKE_NODES,
    CVMJIT_STATS_NUMBER_OF_RESOLVE_NODES,
    CVMJIT_STATS_NUMBER_OF_CHECKINIT_NODES,
    CVMJIT_STATS_NUMBER_OF_ROOT_NODES,
    CVMJIT_STATS_NUMBER_OF_TEMP_NODES,
    CVMJIT_STATS_NUMBER_OF_CONST_NODES,
    CVMJIT_STATS_NUMBER_OF_LOCAL_NODES,
    CVMJIT_STATS_NUMBER_OF_NULL_NODES,

    CVMJIT_STATS_FIRST_CCMRUNTIME_STATS,
    CVMJIT_STATS_CVMCCMruntimeIDiv = CVMJIT_STATS_FIRST_CCMRUNTIME_STATS,
    CVMJIT_STATS_CVMCCMruntimeIRem,
    CVMJIT_STATS_CVMCCMruntimeLMul,
    CVMJIT_STATS_CVMCCMruntimeLDiv,
    CVMJIT_STATS_CVMCCMruntimeLRem,
    CVMJIT_STATS_CVMCCMruntimeLNeg,
    CVMJIT_STATS_CVMCCMruntimeLShl,
    CVMJIT_STATS_CVMCCMruntimeLShr,
    CVMJIT_STATS_CVMCCMruntimeLUshr,
    CVMJIT_STATS_CVMCCMruntimeLAnd,
    CVMJIT_STATS_CVMCCMruntimeLOr,
    CVMJIT_STATS_CVMCCMruntimeLXor,
    CVMJIT_STATS_CVMCCMruntimeFAdd,
    CVMJIT_STATS_CVMCCMruntimeFSub,
    CVMJIT_STATS_CVMCCMruntimeFMul,
    CVMJIT_STATS_CVMCCMruntimeFDiv,
    CVMJIT_STATS_CVMCCMruntimeFRem,
    CVMJIT_STATS_CVMCCMruntimeFNeg,
    CVMJIT_STATS_CVMCCMruntimeDAdd,
    CVMJIT_STATS_CVMCCMruntimeDSub,
    CVMJIT_STATS_CVMCCMruntimeDMul,
    CVMJIT_STATS_CVMCCMruntimeDDiv,
    CVMJIT_STATS_CVMCCMruntimeDRem,
    CVMJIT_STATS_CVMCCMruntimeDNeg,
    CVMJIT_STATS_CVMCCMruntimeI2L,
    CVMJIT_STATS_CVMCCMruntimeI2F,
    CVMJIT_STATS_CVMCCMruntimeI2D,
    CVMJIT_STATS_CVMCCMruntimeL2I,
    CVMJIT_STATS_CVMCCMruntimeL2F,
    CVMJIT_STATS_CVMCCMruntimeL2D,
    CVMJIT_STATS_CVMCCMruntimeF2I,
    CVMJIT_STATS_CVMCCMruntimeF2L,
    CVMJIT_STATS_CVMCCMruntimeF2D,
    CVMJIT_STATS_CVMCCMruntimeD2I,
    CVMJIT_STATS_CVMCCMruntimeD2L,
    CVMJIT_STATS_CVMCCMruntimeD2F,
    CVMJIT_STATS_CVMCCMruntimeLCmp,
    CVMJIT_STATS_CVMCCMruntimeFCmp,
    CVMJIT_STATS_CVMCCMruntimeDCmpl,
    CVMJIT_STATS_CVMCCMruntimeDCmpg,
    CVMJIT_STATS_CVMCCMruntimeThrowClass,
    CVMJIT_STATS_CVMCCMruntimeThrowObject,
    CVMJIT_STATS_CVMCCMruntimeCheckCast,
    CVMJIT_STATS_CVMCCMruntimeCheckArrayAssignable,
    CVMJIT_STATS_CVMCCMruntimeInstanceOf,
    CVMJIT_STATS_CVMCCMruntimeLookupInterfaceMB,
    CVMJIT_STATS_CVMCCMruntimeNew,
    CVMJIT_STATS_CVMCCMruntimeNewArray,
    CVMJIT_STATS_CVMCCMruntimeANewArray,
    CVMJIT_STATS_CVMCCMruntimeMultiANewArray,
    CVMJIT_STATS_CVMCCMruntimeRunClassInitializer,
    CVMJIT_STATS_CVMCCMruntimeResolveClassBlock,
    CVMJIT_STATS_CVMCCMruntimeResolveArrayClassBlock,
    CVMJIT_STATS_CVMCCMruntimeResolveGetfieldFieldOffset,
    CVMJIT_STATS_CVMCCMruntimeResolvePutfieldFieldOffset,
    CVMJIT_STATS_CVMCCMruntimeResolveMethodBlock,
    CVMJIT_STATS_CVMCCMruntimeResolveSpecialMethodBlock,
    CVMJIT_STATS_CVMCCMruntimeResolveMethodTableOffset,
    CVMJIT_STATS_CVMCCMruntimeResolveNewClassBlockAndClinit,
    CVMJIT_STATS_CVMCCMruntimeResolveGetstaticFieldBlockAndClinit,
    CVMJIT_STATS_CVMCCMruntimeResolvePutstaticFieldBlockAndClinit,
    CVMJIT_STATS_CVMCCMruntimeResolveStaticMethodBlockAndClinit,
    CVMJIT_STATS_CVMCCMruntimePutstatic64Volatile,
    CVMJIT_STATS_CVMCCMruntimeGetstatic64Volatile,
    CVMJIT_STATS_CVMCCMruntimePutfield64Volatile,
    CVMJIT_STATS_CVMCCMruntimeGetfield64Volatile,
    CVMJIT_STATS_CVMCCMruntimeMonitorEnter,
    CVMJIT_STATS_CVMCCMruntimeMonitorExit,
    CVMJIT_STATS_CVMCCMruntimeGCRendezvous,
#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
    CVMJIT_STATS_CVMCCMruntimeSimpleSyncUnlock,
#endif
    /* Start of Misc Stats tags: */
    CVMJIT_STATS_FIRST_MISC_STATS,
    CVMJIT_STATS_BC_SIZE = CVMJIT_STATS_FIRST_MISC_STATS, /* Byte-code size */
    CVMJIT_STATS_BC_SIZE_INLINED,              /* # of inlined byte-codes */
    CVMJIT_STATS_CODESIZE_EMITTED,             /* # of bytes of code emitted */
    CVMJIT_STATS_CODESIZE_TO_BC_RATIO,         /* Codesize / byte-codes */ 
    CVMJIT_STATS_CODEBUFFER_SIZE_ALLOCATED,    /* # of bytes of alloced cbuf */
    CVMJIT_STATS_WASTED_CODEBUFFER_SIZE,       /* # of bytes of wasted cbuf */
    CVMJIT_STATS_WASTED_TO_ALLOCATED_RATIO,    /* % of wasted to allocated */ 
    CVMJIT_STATS_BRANCHES,                     /* # of emitted branches */
    CVMJIT_STATS_MEMREAD,                      /* # of emitted mem reads */
    CVMJIT_STATS_MEMWRITE,                     /* # of emitted mem writes */
    CVMJIT_STATS_STACKMAPS_CAPTURED,           /* # of stackmaps captured */
    CVMJIT_STATS_NUMBER_OF_CONSTANTS_EMITTED,  /* # of constants emitted */
    CVMJIT_STATS_NODE_TO_BYTECODE_RATIO,       /* # of Node / bytecode size */
    CVMJIT_STATS_STACKMAP_TO_BYTECODE_RATIO,   /* # of stackmaps / bc size */

    CVMJIT_STATS_FIRST_SPEED_STATS,
    CVMJIT_STATS_COMPILATION_TIME = CVMJIT_STATS_FIRST_SPEED_STATS, /* in ms */
    CVMJIT_STATS_COMPILATION_TIME_PER_BC_W_INLINE,
    CVMJIT_STATS_COMPILATION_TIME_PER_BC,

    /* NOTE: CVMJIT_STATS_NUM_TAGS must be last in this enum list. */
    CVMJIT_STATS_NUM_TAGS                      /* Number of stats tags */
};

typedef enum CVMJITStatsTag CVMJITStatsTag;

typedef struct CVMJITStatsConstant CVMJITStatsConstant;
struct CVMJITStatsConstant
{
    CVMJITStatsConstant *next;
    const char *category;
    CVMInt32 value;
    CVMUint32 refCount;
#ifdef CVM_TRACE_JIT
    char *comment;
#endif
};

struct CVMJITStats
{
    CVMJITStats *prev;
    CVMInt32 stats[CVMJIT_STATS_NUM_TAGS];
    CVMJITStatsConstant *constants;
};

/* NOTE: CVMJITGlobalStats encapsulates the global stats that will be
         collected.
   The typedef is already defined in jit.h:
       typedef struct CVMJITGlobalStats CVMJITGlobalStats;
*/
struct CVMJITGlobalStats
{
    CVMUint32  numberOfCompilations;

    /* Total, Min, and Max JIT statistics: */
    CVMInt32 totalStats[CVMJIT_STATS_NUM_TAGS];
    CVMInt32 minStats[CVMJIT_STATS_NUM_TAGS];
    CVMInt32 maxStats[CVMJIT_STATS_NUM_TAGS];

    /* List of per compilation stats records: */
    CVMJITStats *perCompilationStats;
};

extern const char* CVMJITstatsNames[]; /* The names of these categories */

#define CVMJIT_STATS_MAX_VALUE          (0x7fffffff)

/* Purpose: execute x only if CVM_JIT_COLLECT_STATS is defined. */
#define CVMJITstatsExec(x) x

/* Purpose: Construct stats records in the compilation context. */
extern CVMBool
CVMJITstatsInitCompilationContext(CVMJITCompilationContext *con);

/* Purpose: Destroy stats records allocated in the compilation context. */
extern void
CVMJITstatsDestroyCompilationContext(CVMJITCompilationContext *con);

/*
 * Add 'num' to the 'count' of instances of an event of type 'tag'.
 * 'num' can be negative as well as positive.
 */
extern void 
CVMJITstatsRecordAdd(CVMJITCompilationContext* con, CVMJITStatsTag tag,
                     CVMInt32 num);

/*
 * Increment the 'count' of instances of an event of type 'tag'.
 */
extern void
CVMJITstatsRecordInc(CVMJITCompilationContext* con, CVMJITStatsTag tag);
#define CVMJITstatsRecordInc(con, tag) CVMJITstatsRecordAdd((con), (tag), 1)

/*
 * Set 'count' instances of an event of type 'tag'.
 */
extern void
CVMJITstatsRecordSetCount(CVMJITCompilationContext* con, CVMJITStatsTag tag,
			  CVMInt32 count);

/*
 * Get the count of instances of an event of type 'tag'.
 */
extern CVMInt32
CVMJITstatsRecordGetCount(CVMJITCompilationContext* con, CVMJITStatsTag tag);

/*
 * Captures a new max value if necessary.
 */
extern void
CVMJITstatsRecordUpdateMax(CVMJITCompilationContext* con, CVMJITStatsTag tag,
                           CVMInt32 value);
#define CVMJITstatsRecordUpdateMax(con, tag, value) {        \
    if ((value) > CVMJITstatsRecordGetCount((con), (tag))) { \
        CVMJITstatsRecordSetCount((con), (tag), (value));    \
    }                                                        \
}

/*
 * Captures a new min value if necessary.
 */
extern void
CVMJITstatsRecordUpdateMin(CVMJITCompilationContext* con, CVMJITStatsTag tag,
                           CVMInt32 value);
#define CVMJITstatsRecordUpdateMin(con, tag, value) {        \
    if ((value) < CVMJITstatsRecordGetCount((con), (tag))) { \
        CVMJITstatsRecordSetCount((con), (tag), (value));    \
    }                                                        \
}

/* Purpose: Updates the stats in the compilation context. */
extern void
CVMJITstatsUpdateStats(CVMJITCompilationContext *con);

/*
 * Add stats about a constant being emitted.
 */
extern void
CVMJITstatsAddConstant(CVMJITCompilationContext *con, CVMInt32 value,
                       const char *category, CVMUint32 refCount);

/*
 * Dump current stats
 */
extern void 
CVMJITstatsDump(CVMJITCompilationContext* con);

/*
 * Initializes the globals stats.
 */
extern CVMBool
CVMJITstatsInitGlobalStats(CVMJITGlobalStats **jgs);

/*
 * Destroy the globals stats.
 */
extern void
CVMJITstatsDestroyGlobalStats(CVMJITGlobalStats **jgs);

/*
 * Dump global stats
 */
extern void 
CVMJITstatsDumpGlobalStats();

#else

/* No implementation */
#define CVMJITstatsExec(x)
#define CVMJITstatsInitCompilationContext(con) (CVM_TRUE)
#define CVMJITstatsDestroyCompilationContext(con)
#define CVMJITstatsRecordAdd(con, tag, num)
#define CVMJITstatsRecordInc(con, tag)
#define CVMJITstatsRecordSetCount(con, tag, count)
#define CVMJITstatsRecordGetCount(con, tag)  (-1)
#define CVMJITstatsRecordUpdateMax(con, tag, value)
#define CVMJITstatsRecordUpdateMin(con, tag, value)
#define CVMJITstatsUpdateStats(con)
#define CVMJITstatsAddConstant(con, value, category, refCount)
#define CVMJITstatsDump(con)
#define CVMJITstatsInitGlobalStats(jgs)      (CVM_TRUE)
#define CVMJITstatsDestroyGlobalStats(jgs)
#define CVMJITstatsDumpGlobalStats()

#endif /* CVM_JIT_COLLECT_STATS */

#endif /* _INCLUDED_JITSTATS_H */
