/*
 * @(#)jitstats.c	1.13 06/10/10
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

#include "javavm/include/globals.h"
#include "javavm/include/jit/jitstats.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitcomments.h"

#ifdef CVM_JIT_COLLECT_STATS

/* NOTE: #define CVM_JIT_EXCLUDE_TOP_N_CONST to exclude the top N most popular
   constants from the compilation characteristics.  This feature is used as
   a what-if analysis tool to see what happens if we made the top N most
   popular constants sticky in a global constant pool to be shared by all
   compiled methods.
*/
#undef CVM_JIT_EXCLUDE_TOP_N_CONST
#ifdef CVM_JIT_EXCLUDE_TOP_N_CONST
#define NUMBER_OF_CONST_TO_EXCLUDE              100
#define NUMBER_OF_CONST_PER_METHOD_THRESHOLD    6
#endif

#undef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

/* Purpose: See if help on the stats option is needed. */
#define CVMJITstatsAborted() \
    (0 != (CVMglobals.jit.statsOptions & (1 << CVMJIT_STATS_ABORTED)))

/* Purpose: See if help on the stats option is needed. */
#define CVMJITstatsHelp() \
    (0 != (CVMglobals.jit.statsOptions & (1 << CVMJIT_STATS_HELP)))

/* Purpose: See if we should not collect any JIT stats. */
#define CVMJITstatsCollectNone() \
    (0 != (CVMglobals.jit.statsOptions & \
           ((1 << CVMJIT_STATS_COLLECT_NONE) | \
            (1 << CVMJIT_STATS_ABORTED))))

/* Purpose: See if we should collect minimal stats. */
#define CVMJITstatsCollectMinimal() \
    (0 != (CVMglobals.jit.statsOptions & \
           ((1 << CVMJIT_STATS_COLLECT_MINIMAL) | \
            (1 << CVMJIT_STATS_COLLECT_MORE) | \
            (1 << CVMJIT_STATS_COLLECT_CONSTANTS) | \
            (1 << CVMJIT_STATS_COLLECT_VERBOSE) | \
            (1 << CVMJIT_STATS_COLLECT_MAXIMAL))))

/* Purpose: See if we should collect more stats. */
#define CVMJITstatsCollectMore() \
    (0 != (CVMglobals.jit.statsOptions & \
           ((1 << CVMJIT_STATS_COLLECT_MORE) | \
            (1 << CVMJIT_STATS_COLLECT_CONSTANTS) | \
            (1 << CVMJIT_STATS_COLLECT_VERBOSE) | \
            (1 << CVMJIT_STATS_COLLECT_MAXIMAL))))

/* Purpose: See if we should collect stats on constants emissions. */
#define CVMJITstatsCollectConstants() \
    (0 != (CVMglobals.jit.statsOptions & \
           ((1 << CVMJIT_STATS_COLLECT_CONSTANTS) | \
            (1 << CVMJIT_STATS_COLLECT_MAXIMAL))))

/* Purpose: See if we should dump per compilation stats. */
#define CVMJITstatsCollectVerbose() \
    (0 != (CVMglobals.jit.statsOptions & \
           ((1 << CVMJIT_STATS_COLLECT_VERBOSE) | \
            (1 << CVMJIT_STATS_COLLECT_MAXIMAL))))

/* Purpose: See if we should collect maximal stats. */
#define CVMJITstatsCollectMaximal() \
    (0 != (CVMglobals.jit.statsOptions & (1 << CVMJIT_STATS_COLLECT_MAXIMAL)))

/* Purpose: For displaying timer ticks ('.') while processing statistics. */
#define INITIALIZE_TICK \
    CVMInt64 _tickTimeValue = CVMtimeMillis(); \
    CVMUint32 _tickCounter = 0;

#define doTick() { \
    if (_tickCounter++ > 100) { \
        CVMInt64 newTick = CVMtimeMillis(); \
        if (CVMlong2Int(CVMlongSub(newTick, _tickTimeValue)) > 1000) { \
            CVMconsolePrintf("."); \
            _tickTimeValue = newTick; \
        } \
    } \
}

static void
CVMJITstatsDeleteConstant(CVMJITStatsConstant *constant);
static void
CVMJITstatsUpdateGlobalStats(CVMJITCompilationContext *con);

/* Purpose: Indicate that we've stop collecting stats from this point forth. */
static void
CVMJITstatsAbort()
{
    CVMglobals.jit.statsOptions = (1 << CVMJIT_STATS_ABORTED);
    CVMconsolePrintf("Aborting JIT stats collection due to insufficient"
                     " memory\n");
}

/* Purpose: Construct stats records in the compilation context. */
CVMBool
CVMJITstatsInitCompilationContext(CVMJITCompilationContext *con)
{
    if (!CVMJITstatsCollectNone()) {
        con->stats = calloc(1, sizeof(CVMJITStats));
        if (con->stats == NULL) {
            CVMJITstatsAbort();
            return CVM_FALSE;
        }
    }
    return CVM_TRUE;
}

/* Purpose: Destroy stats records allocated in the compilation context. */
void
CVMJITstatsDestroyCompilationContext(CVMJITCompilationContext *con)
{
    if (con->stats != NULL) {
        /* We should have moved the constant stats over to the global records
           by now if any: */
        CVMassert(con->stats->constants == NULL);
        free(con->stats);
    }
}

/* Purpose: Prints help information on the JIT stats options. */
static void
CVMJITstatsPrintHelp()
{
#undef P
#define P(x) CVMconsolePrintf(x)
    P("\nJIT stats options:\n");
    P("Usage: cvm -Xjit:stats={help|none|minimal|more|verbose|"
                               "constants|maximal}\n");
    P("    help       - print this help info.\n");
    P("    none       - do not collect any statistics.\n");
    P("    minimal    - collect the minimal set of statistics\n"
      "                 including total, min, max statistics.\n");
    P("    more       - collect more statistics in addition to the minimal\n"
      "                 set including average, standard deviation, and\n"
      "                 projected range values.\n");
    P("    verbose    - collect the more set statistics and dumps a verbose\n"
      "                 stats record for each method compiled.  This output\n"
      "                 is appropriate for getting data that can be imported\n"
      "                 into a spreadsheet.\n");
    P("    constants  - collect constants emission statistics in addition\n"
      "                 to the more set.\n");
    P("    maximal    - collects all statistics and dumps a verbose stats\n"
      "                 record for each method compiled.  This output is\n"
      "                 appropriate for getting data that can be imported\n"
      "                 into a spreadsheet.\n");
    P("\n");
    P("    The default option is \"none\".\n");
    P("\n");
    P("    Compilation speed is estimated based on a millisecond timer\n"
      "    measured over individual method compilations.  Hence, the\n"
      "    results may be subjected to significant gross rounding\n"
      "    errors.\n");
#undef P
}

/*
 * Add 'increment' to the 'count' of instances of an event of type 'tag'.
 * 'increment' can be negative as well as positive.
 */
void 
CVMJITstatsRecordAdd(CVMJITCompilationContext* con, CVMJITStatsTag tag,
                     CVMInt32 increment)
{
    if (!CVMJITstatsCollectNone()) {
        CVMassert(tag < CVMJIT_STATS_NUM_TAGS);
        con->stats->stats[tag] += increment;
    }
}

/*
 * Set count
 */
void 
CVMJITstatsRecordSetCount(CVMJITCompilationContext* con, CVMJITStatsTag tag,
			  CVMInt32 count)
{
    if (!CVMJITstatsCollectNone()) {
        CVMassert(tag < CVMJIT_STATS_NUM_TAGS);
        con->stats->stats[tag] = count;
    }
}

/*
 * Get count
 */
CVMInt32
CVMJITstatsRecordGetCount(CVMJITCompilationContext* con, CVMJITStatsTag tag)
{
    if (!CVMJITstatsCollectNone()) {
        CVMassert(tag < CVMJIT_STATS_NUM_TAGS);
        return con->stats->stats[tag];
    }
    return 0;
}

/* Purpose: Constructs a CVMJITStats record. */
static CVMJITStats *
CVMJITstatsNew(CVMJITCompilationContext *con)
{
    CVMJITStats *self;
    CVMJITStats *stats = con->stats;

    CVMassert(CVMJITstatsCollectMore());

    /* Allocate the memory: */
    self = malloc(sizeof(CVMJITStats));
    if (self == NULL) {
        /* Oops ... out of memory: */
        CVMJITstatsAbort();
        /* Release the constants stats array in the compilation context's
           stats record because we're supposed to take over respoinsibility
           for it here anyway: */
        if (con->stats->constants != NULL) {
            free(con->stats->constants);
            con->stats->constants = NULL;
        }
        return NULL;
    }

    /* Initialize the record: */
    self->prev = NULL;

    /* Copy the stats including the constant records: */
    memcpy(self, stats, sizeof(CVMJITStats));

    /* The global record will keep track and take care of freeing of it now. 
       The compilation context need not worry about it: */
    con->stats->constants = NULL;

    return self;
}

/* Purpose: Destructs a CVMJITStats record. */
static void
CVMJITstatsDelete(CVMJITStats *self)
{
    CVMJITStatsConstant *constant;

    if (self == NULL) {
        return; /* Nothing to delete. */
    }

    /* Free the constant records: */
    constant = self->constants;
    while(constant) {
        CVMJITStatsConstant *next = constant->next;
        CVMJITstatsDeleteConstant(constant);
        constant = next;
    }
    free(self);
}

/* Purpose: Initializes the globals stats. */
CVMBool
CVMJITstatsInitGlobalStats(CVMJITGlobalStats **gstats)
{
    /* Convert the statsToCollect option into a bitfield option:
       Note: The statsOptions macros will do bitfield testing. */
    CVMglobals.jit.statsOptions = (1 << CVMglobals.jit.statsToCollect);
    *gstats = NULL;

    if (CVMJITstatsHelp()) {
        CVMJITstatsPrintHelp();
        return CVM_FALSE; /* Tell VM to abort. */

    } else if (CVMJITstatsCollectMinimal()) {
        CVMJITGlobalStats *gs;
        CVMUint32 i;

        /* Allocate the GlobalStats record: */
        gs = calloc(1, sizeof(CVMJITGlobalStats));
        if (gs == NULL) {
            return CVM_FALSE; /* Initialization failed. */
        }

        /* Initialize the min stats table: */
        for (i = 0; i < CVMJIT_STATS_NUM_TAGS; i++) {
            gs->minStats[i] = CVMJIT_STATS_MAX_VALUE;
        }

        *gstats = gs;
    }
    return CVM_TRUE;
}

/* Purpose: Initializes the globals stats. */
void
CVMJITstatsDestroyGlobalStats(CVMJITGlobalStats **gstats)
{
    CVMJITGlobalStats *gs;
    CVMJITStats *record;

    gs = *gstats;
    if (gs == NULL) {
        return; /* Nothing to destruct. */
    }

    /* Destroy the "more" sampled stats records: */
    record = gs->perCompilationStats;
    while (record != NULL) {
        CVMJITStats *prev = record->prev;
        CVMJITstatsDelete(record);
        record = prev;
    }

    /* Destroy the global stats record: */
    free(gs);
    *gstats = NULL;
}

/* Purpose: Fills in the ratio fields based on other fields.
   NOTE: It is assumed that all the needed input fields have already been
         initialized before hand. */
static void
CVMJITstatsSetRatios(CVMInt32 *stats)
{
    CVMassert(!CVMJITstatsCollectNone());

    CVMassert(stats[CVMJIT_STATS_BC_SIZE] != 0);
    stats[CVMJIT_STATS_CODESIZE_TO_BC_RATIO] =
        stats[CVMJIT_STATS_CODESIZE_EMITTED] * 100 /
        stats[CVMJIT_STATS_BC_SIZE];

    stats[CVMJIT_STATS_NODE_TO_BYTECODE_RATIO] =
        stats[CVMJIT_STATS_TOTAL_NUMBER_OF_NODES] * 100 /
        (stats[CVMJIT_STATS_BC_SIZE] + stats[CVMJIT_STATS_BC_SIZE_INLINED]);

    stats[CVMJIT_STATS_STACKMAP_TO_BYTECODE_RATIO] =
        stats[CVMJIT_STATS_STACKMAPS_CAPTURED] * 100 /
        (stats[CVMJIT_STATS_BC_SIZE] + stats[CVMJIT_STATS_BC_SIZE_INLINED]);

    stats[CVMJIT_STATS_COMPILATION_TIME_PER_BC_W_INLINE] =
        stats[CVMJIT_STATS_COMPILATION_TIME] * 100 /
        (stats[CVMJIT_STATS_BC_SIZE] + stats[CVMJIT_STATS_BC_SIZE_INLINED]);

    stats[CVMJIT_STATS_COMPILATION_TIME_PER_BC] =
        stats[CVMJIT_STATS_COMPILATION_TIME] * 100 /
        stats[CVMJIT_STATS_BC_SIZE];

    CVMassert(stats[CVMJIT_STATS_CODEBUFFER_SIZE_ALLOCATED] != 0);
    stats[CVMJIT_STATS_WASTED_TO_ALLOCATED_RATIO] =
        stats[CVMJIT_STATS_WASTED_CODEBUFFER_SIZE] * 100 /
        stats[CVMJIT_STATS_CODEBUFFER_SIZE_ALLOCATED];

    CVMassert(stats[CVMJIT_STATS_COMPUTED_COMP_STACK_MAX] != 0);
    stats[CVMJIT_STATS_ACTUAL_MATCH_STACK_WASTE] =
        100 - (stats[CVMJIT_STATS_ACTUAL_MATCH_STACK_MAX] * 100 /
        stats[CVMJIT_STATS_COMPUTED_COMP_STACK_MAX]);

    stats[CVMJIT_STATS_ACTUAL_SYNTHESIS_STACK_WASTE] =
        100 - (stats[CVMJIT_STATS_ACTUAL_SYNTHESIS_STACK_MAX] * 100 /
        stats[CVMJIT_STATS_COMPUTED_COMP_STACK_MAX]);

    stats[CVMJIT_STATS_ACTUAL_ACTION_STACK_WASTE] =
        100 - (stats[CVMJIT_STATS_ACTUAL_ACTION_STACK_MAX] * 100 /
        stats[CVMJIT_STATS_COMPUTED_COMP_STACK_MAX]);

    stats[CVMJIT_STATS_ACTUAL_RESOURCE_STACK_WASTE] =
        100 - (stats[CVMJIT_STATS_ACTUAL_RESOURCE_STACK_MAX] * 100 /
        stats[CVMJIT_STATS_COMPUTED_COMP_STACK_MAX]);
}

/* Purpose: Updates the per compilation cycle statistics. */
void
CVMJITstatsUpdateStats(CVMJITCompilationContext *con)
{
    if (CVMJITstatsCollectMinimal()) {
	CVMUint32 codeLen = CVMjmdCodeLength(con->jmd);
	CVMUint32 additionalCodeLen = 
	    con->codeLength - CVMjmdCodeLength(con->jmd);
	CVMUint32 genCodeLen        = con->curPhysicalPC - con->codeBufAddr;
	CVMUint32 genWastedLen      = con->codeBufEnd - con->curPhysicalPC;
	CVMUint32 genAllocatedLen   = con->codeBufEnd - con->codeBufAddr;
	
        /* Remember and compute a few key numbers: */
        CVMJITstatsRecordSetCount(con, CVMJIT_STATS_BC_SIZE, codeLen);
        CVMJITstatsRecordSetCount(con, CVMJIT_STATS_BC_SIZE_INLINED,
                                  additionalCodeLen);
        CVMJITstatsRecordSetCount(con, CVMJIT_STATS_CODESIZE_EMITTED,
                                  genCodeLen);
        CVMJITstatsRecordSetCount(con, CVMJIT_STATS_CODEBUFFER_SIZE_ALLOCATED,
                                  genAllocatedLen);
        CVMJITstatsRecordSetCount(con, CVMJIT_STATS_WASTED_CODEBUFFER_SIZE,
                                  genWastedLen);
        CVMJITstatsRecordSetCount(con, CVMJIT_STATS_MAX_IR_TREE_DEPTH,
                                  con->saveRootCnt);

        /* Fill in the ratio fields based on the data initialized above and
           those that have been cummulated so far: */
        CVMJITstatsSetRatios(con->stats->stats);

        /* Go update the global stats as well: */
        CVMJITstatsUpdateGlobalStats(con);
    }
}

/* Purpose: Add stats about a constant being emitted to a specific list. */
static CVMJITStatsConstant *
CVMJITstatsAddConstantInternal(CVMJITStatsConstant **list, CVMInt32 value,
                               const char *category, CVMUint32 refCount)
{
    CVMJITStatsConstant *constant = *list;

    CVMassert(CVMJITstatsCollectConstants());

    /* First see if we already have a record for this: */
    while(constant) {
        if ((constant->value == value) &&
            (strcmp(constant->category, category) == 0)) {
            /* If so, just inc its refCount: */
            constant->refCount += refCount;
            return NULL; /* No new constant. */
        }
        constant = constant->next;
    }

    /* Else, create and initialize a new constant record: */
    constant = malloc(sizeof(CVMJITStatsConstant));
    if (constant == NULL) {
        CVMJITstatsAbort();
        return NULL;
    }
    constant->value = value;
    constant->category = category;
    constant->refCount = refCount;

    /* Add the constant record to the list: */
    constant->next = *list;
    *list =  constant;

    return constant; /* New constant created. */
}

/* Purpose: Add stats about a constant being emitted. */
void
CVMJITstatsAddConstant(CVMJITCompilationContext *con, CVMInt32 value,
                       const char *category, CVMUint32 refCount)
{
    if (CVMJITstatsCollectConstants()) {
        CVMJITStatsConstant *constant;
        constant = CVMJITstatsAddConstantInternal(&con->stats->constants,
                                                  value, category, refCount);
#ifdef CVM_TRACE_JIT
        /* Only set the comment string if a new constant was added: */
        if (constant != NULL) {
            CVMUint32 length = 0;
            CVMCodegenComment *comment = con->comments;
            char *commentStr;
            while(comment != NULL) {
                length += strlen(comment->commentStr) + 1;
                comment = comment->next;
            }
            length += 1;
            commentStr =  malloc(length * sizeof(char));
            if (commentStr == NULL) {
                CVMJITstatsAbort();
                constant->comment = NULL;
                return;
            }
            CVMassert(commentStr != NULL);
            commentStr[0] = '\0';
            comment = con->comments;
            while(comment != NULL) {
                strcat(commentStr, comment->commentStr);
                strcat(commentStr, " ");
                comment = comment->next;
            }
            constant->comment = commentStr;
        }
#endif
    }
}

/* Purpose: Destructs a constant. */
static void
CVMJITstatsDeleteConstant(CVMJITStatsConstant *constant)
{
#ifdef CVM_TRACE_JIT
    free(constant->comment);
#endif
    free(constant);
}

/* Purpose: Updates the global stats. */
static void
CVMJITstatsUpdateGlobalStats(CVMJITCompilationContext *con)
{
    CVMJITGlobalStats *gstats = CVMglobals.jit.globalStats;
    CVMInt32 *stats;
    CVMUint32 i;

    if (CVMJITstatsAborted()) {
        return;
    }

    /* Make sure we're at least collecting minimal stats: */
    CVMassert(CVMJITstatsCollectMinimal());

    stats = con->stats->stats;
    gstats->numberOfCompilations++;

    /* Update the other global stats: */
    for (i = 0; i < CVMJIT_STATS_NUM_TAGS; i++) {
        CVMInt32 value = stats[i];

        /* Update the total stats: */
        gstats->totalStats[i] += value;
        /* Update the min stats: */
        if (value < gstats->minStats[i]) {
            gstats->minStats[i] = value;
        }
        /* Update the max stats: */
        if (value > gstats->maxStats[i]) {
            gstats->maxStats[i] = value;
        }
    }

    /* Update the ratio fields: 
       NOTE: The min and max fields should not be computed this way.
    */
    CVMJITstatsSetRatios(gstats->totalStats);

    /* Do more stuff if we're collecting "more" stats: */
    if (CVMJITstatsCollectMore()) {
        /* Construct the stats record: */
        CVMJITStats *record = CVMJITstatsNew(con);
        if (record == NULL) {
            return; /* Abort because we're out of stats memory. */
        }

        /* Add it to the list: */
        record->prev = gstats->perCompilationStats;
        gstats->perCompilationStats = record;
    }
}

/* Purpose: Computes the standard deviation stats. */
static void
CVMJITstatsComputeStandardDeviation(CVMJITStats *records,
                                    CVMUint32 numberOfRecords,
                                    double *averageValues,
                                    double *stdDevValues)
{
    CVMJITStats *record;
    double total = 0;
    double mean;
    double variance;
    CVMUint32 i;
    extern double CVMfdlibmSqrt(double);

    /* Process the other stats: */
    for (i = 0; i < CVMJIT_STATS_NUM_TAGS; i++) {
        /* Compute the total and the mean: */
        total = 0.0;
        for (record = records; record != NULL; record = record->prev) {
            total += record->stats[i];
        }
        mean = total / numberOfRecords;

        /* Compute the standard dev: */
        total = 0.0;
        for (record = records; record != NULL; record = record->prev) {
            double diff = record->stats[i] - mean;
            total += diff * diff; /* i.e. diff squared. */
        }
        variance = total / (numberOfRecords - 1);
        averageValues[i] = mean;
        stdDevValues[i] = CVMfdlibmSqrt(variance);
    }
}

/* Purpose: Compares the refCount of 2 stats constant records. */
static int
CVMJITstatsConstantRefCountComparator(const void *v1, const void *v2)
{
    CVMJITStatsConstant *c1 = *(CVMJITStatsConstant **)v1;
    CVMJITStatsConstant *c2 = *(CVMJITStatsConstant **)v2;
    return (c2->refCount - c1->refCount);
}

/* Purpose: Tally up the stats on constant emissions. */
static CVMJITStatsConstant **
CVMJITstatsTallyConstantEmmisions(CVMJITStats *records,
                                  CVMUint32 *numberOfConstantsEmitted,
                                  CVMUint32 *numberOfUniqueConstants)
{
    CVMJITStatsConstant *totalList = NULL;
    CVMJITStatsConstant **sortedTotalList = NULL;
    CVMJITStats *record = records;
    CVMUint32 emitCount = 0;
    CVMUint32 uniqueCount = 0;
    CVMUint32 i;
    CVMJITStatsConstant *constant;
    INITIALIZE_TICK

    CVMassert(CVMJITstatsCollectConstants());

    /* Coalate into a single list: */
    while(record) {
        constant = record->constants;
        while(constant) {
            CVMJITStatsConstant *crecord;
            crecord =
                CVMJITstatsAddConstantInternal(&totalList,
                    constant->value, constant->category, constant->refCount);
            /* Only set the comment string if a new constant was added: */
            if (crecord != NULL) {
                uniqueCount++;
#ifdef CVM_TRACE_JIT
                crecord->comment = strdup(constant->comment);
                if (crecord->comment == NULL) {
                    goto handleAbort;
                }
#endif
            }
            if (CVMJITstatsAborted()) {
                goto handleAbort;
            }
            doTick(); /* Print a tick. */
            constant = constant->next;
        }
        record = record->prev;
    }

    /* Get some stats: */
    sortedTotalList = malloc(sizeof(CVMJITStatsConstant *) * uniqueCount);
    if (sortedTotalList == NULL) {
        goto handleAbort;
    }
    constant = totalList;
    for(i = 0; i < uniqueCount; i++) {
        sortedTotalList[i] = constant;
        emitCount += constant->refCount;
        constant = constant->next;
        doTick(); /* Print a tick. */
    }
    qsort(sortedTotalList, uniqueCount, sizeof(CVMJITStatsConstant *),
          CVMJITstatsConstantRefCountComparator);

    *numberOfConstantsEmitted = emitCount;
    *numberOfUniqueConstants = uniqueCount;
    return sortedTotalList;

handleAbort:
    CVMJITstatsAbort();

    /* Free the sortedTotalList: */
    if (sortedTotalList) {
        free(sortedTotalList);
    }

    /* Free the constant records: */
    constant = totalList;
    while(constant) {
        CVMJITStatsConstant *next = constant->next;
        CVMJITstatsDeleteConstant(constant);
        constant = next;
    }

    *numberOfConstantsEmitted = 0;
    *numberOfUniqueConstants = 0;
    return NULL;
}

/* Purpose: Destructs the sorted constants list. */
static void
CVMJITstatsDeleteSortedConstantsList(CVMJITStatsConstant **sortedConstantsList,
                                     CVMUint32 size)
{
    CVMUint32 i;
    for (i = 0; i < size; i++) {
        CVMJITstatsDeleteConstant(sortedConstantsList[i]);
    }
    free(sortedConstantsList);
}

#ifdef CVM_JIT_EXCLUDE_TOP_N_CONST
/* Purpose: Excludes the top N constants from the methods. */
static CVMUint32
CVMJITstatsExcludeTopConstantsFromStats(CVMJITStats *records,
        CVMJITStatsConstant **sortedConstantList, CVMUint32 numberToExclude)
{
    INITIALIZE_TICK
    CVMUint32 i;
    CVMUint32 methodsBelowThreshold = 0;

    /* Remove top N constants from all method stats: */
    for (i = 0; i < numberToExclude; i++) {
        CVMJITStatsConstant *current = sortedConstantList[i];
        CVMJITStats *record = records;
        while (record != NULL) {
            CVMJITStatsConstant **prev = &record->constants;
            CVMJITStatsConstant *constant = record->constants;
            while (constant != NULL) {
                CVMJITStatsConstant *next = constant->next;
                if ((constant->value == current->value) &&
                    (strcmp(constant->category, current->category) == 0)) {
                    /* Undo its effect on the refCount: */
                    record->stats[CVMJIT_STATS_NUMBER_OF_CONSTANTS_EMITTED]
                        -= constant->refCount;
                    *prev = next; /* Unlink from the list. */
                    CVMJITstatsDeleteConstant(constant); /* Release it. */
                } else {
                    prev = &constant->next;
                }
                constant = next;
                doTick(); /* Print a tick. */
            }
            record = record->prev;
        }
    }

    /* Adjust/Recompute the cummulative stats: */
    {
        CVMUint32 tag = CVMJIT_STATS_NUMBER_OF_CONSTANTS_EMITTED;
        CVMJITGlobalStats *gstats = CVMglobals.jit.globalStats;
        CVMJITStats *record = records;

        gstats->totalStats[tag] = 0;
        gstats->minStats[tag] = CVMJIT_STATS_MAX_VALUE;;
        gstats->maxStats[tag] = 0;

        while (record != NULL) {
            CVMInt32 value = record->stats[tag];

            /* Update the total stats: */
            gstats->totalStats[tag] += value;
            /* Update the min stats: */
            if (value < gstats->minStats[tag]) {
                gstats->minStats[tag] = value;
            }
            /* Update the max stats: */
            if (value > gstats->maxStats[tag]) {
                gstats->maxStats[tag] = value;
            }
            /* Count the number of methods below the desired threshold: */
            if (record->stats[CVMJIT_STATS_NUMBER_OF_CONSTANTS_EMITTED] <=
                NUMBER_OF_CONST_PER_METHOD_THRESHOLD) {
                methodsBelowThreshold++;
            }
            record = record->prev;
            doTick(); /* Print a tick. */
        }
    }

    return methodsBelowThreshold;
}
#endif /* CVM_JIT_EXCLUDE_TOP_N_CONST */

/* The following are some formating information for the stats dumps: */
typedef struct CVMJITStatsSection CVMJITStatsSection;
struct CVMJITStatsSection
{
    CVMJITStatsTag startOfSection;
    const char *name;
    CVMBool printLegend;
    CVMBool hideSection;
};

static const CVMJITStatsSection statsSections[] =
{
    /* Start of section, Section name, printLegend? */
    { CVMJIT_STATS_FIRST_WORKING_MEM_STATS,
      "Working memory statistics", CVM_TRUE, CVM_FALSE },
    { CVMJIT_STATS_MAX_IR_TREE_DEPTH, NULL, CVM_FALSE, CVM_FALSE },
    { CVMJIT_STATS_FIRST_NODE_STATS, "IR Node Statistics",
      CVM_TRUE, CVM_FALSE },
    { CVMJIT_STATS_FIRST_CCMRUNTIME_STATS, "CCM Runtime Statistics",
      CVM_TRUE, CVM_TRUE },
    { CVMJIT_STATS_FIRST_MISC_STATS, "Misc Statistics",
      CVM_TRUE, CVM_FALSE },
};

typedef struct CVMJITStatsFormat CVMJITStatsFormat;
typedef enum CVMJITStatsFormatType
{
    CVMJIT_STATS_DEFAULT,
    CVMJIT_STATS_MULTIPLIER,
    CVMJIT_STATS_PERCENT
} CVMJITStatsFormatType;

struct CVMJITStatsFormat
{
    CVMJITStatsFormatType formatType;
    CVMJITStatsTag tag;
};

static const CVMJITStatsFormat statsFormats[] =
{
    { CVMJIT_STATS_PERCENT, CVMJIT_STATS_ACTUAL_MATCH_STACK_WASTE, },
    { CVMJIT_STATS_PERCENT, CVMJIT_STATS_ACTUAL_SYNTHESIS_STACK_WASTE, },
    { CVMJIT_STATS_PERCENT, CVMJIT_STATS_ACTUAL_ACTION_STACK_WASTE, },
    { CVMJIT_STATS_PERCENT, CVMJIT_STATS_ACTUAL_RESOURCE_STACK_WASTE, },
    { CVMJIT_STATS_MULTIPLIER, CVMJIT_STATS_CODESIZE_TO_BC_RATIO, },
    { CVMJIT_STATS_PERCENT, CVMJIT_STATS_WASTED_TO_ALLOCATED_RATIO, },
    { CVMJIT_STATS_MULTIPLIER, CVMJIT_STATS_NODE_TO_BYTECODE_RATIO, },
    { CVMJIT_STATS_MULTIPLIER, CVMJIT_STATS_STACKMAP_TO_BYTECODE_RATIO, },
};

static const char* const tagNames[] = {

    /* Working mem stats: */
    "JIT_ALLOC_IRGEN_NODE (bytes)     ", /* IR node allocation */
    "JIT_ALLOC_IRGEN_OTHER (bytes)    ", /* Other front-end allocation */
    "JIT_ALLOC_OPTIMIZER (bytes)      ", /* Optimizer allocations */
    "JIT_ALLOC_CGEN_REGMAN (bytes)    ", /* Codegen working mem for regs */
    "JIT_ALLOC_CGEN_ALURHS (bytes)    ", /* Codegen working mem for alurhs' */
    "JIT_ALLOC_CGEN_MEMSPEC (bytes)   ", /* Codegen working mem for memspecs*/
    "JIT_ALLOC_CGEN_FIXUP (bytes)     ", /* Codegen working mem for fixups */
    "JIT_ALLOC_CGEN_OTHER (bytes)     ", /* Other codegen working memory */
    "JIT_ALLOC_COLLECTIONS (bytes)    ", /* Allocs for sets, arrays, etc. */
    "JIT_ALLOC_DEBUGGING (bytes)      ", /* Allocs for debugging purposes */
#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
    "JIT_ALLOC_CODE_SCHEDULING (bytes)",
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */   
    "Total working memory (bytes)     ",

    "Max IR tree depth                ",
    "Estimated Comp Stack max         ",
    "Actual Match Stack max           ",
    "Actual Synthesis Stack max       ",
    "Actual Action Stack max          ",
    "Actual Resource Stack max        ",
    "Actual Match Stack waste %       ",
    "Actual Synthesis Stack waste %   ",
    "Actual Action Stack waste %      ",
    "Actual Resource Stack waste %    ",

    /* IR Node Stats: */
    "Total Number of IR Nodes         ",
    "Number of NullCheck Nodes        ",
    "Number of NullChecks eliminated  ",
    "Number of ArrayBoundsCheck Nodes ",
    "Number of ArrayBoundsCheck Elim. ",
    "Number of Invoke Nodes           ",
    "Number of Resolve Nodes          ",
    "Number of CheckInit Nodes        ",
    "Number of Root Nodes             ",
    "Number of TEMP Nodes             ",
    "Number of Const Nodes            ",
    "Number of Local Nodes            ",
    "Number of NULL Nodes             ",

    /* CCM Runtime Stats: */
    "Calls to CVMCCMruntimeIDiv       ",
    "Calls to CVMCCMruntimeIRem       ",
    "Calls to CVMCCMruntimeLMul       ",
    "Calls to CVMCCMruntimeLDiv       ",
    "Calls to CVMCCMruntimeLRem       ",
    "Calls to CVMCCMruntimeLNeg       ",
    "Calls to CVMCCMruntimeLShl       ",
    "Calls to CVMCCMruntimeLShr       ",
    "Calls to CVMCCMruntimeLUshr      ",
    "Calls to CVMCCMruntimeLAnd       ",
    "Calls to CVMCCMruntimeLOr        ",
    "Calls to CVMCCMruntimeLXor       ",
    "Calls to CVMCCMruntimeFAdd       ",
    "Calls to CVMCCMruntimeFSub       ",
    "Calls to CVMCCMruntimeFMul       ",
    "Calls to CVMCCMruntimeFDiv       ",
    "Calls to CVMCCMruntimeFRem       ",
    "Calls to CVMCCMruntimeFNeg       ",
    "Calls to CVMCCMruntimeDAdd       ",
    "Calls to CVMCCMruntimeDSub       ",
    "Calls to CVMCCMruntimeDMul       ",
    "Calls to CVMCCMruntimeDDiv       ",
    "Calls to CVMCCMruntimeDRem       ",
    "Calls to CVMCCMruntimeDNeg       ",
    "Calls to CVMCCMruntimeI2L        ",
    "Calls to CVMCCMruntimeI2F        ",
    "Calls to CVMCCMruntimeI2D        ",
    "Calls to CVMCCMruntimeL2I        ",
    "Calls to CVMCCMruntimeL2F        ",
    "Calls to CVMCCMruntimeL2D        ",
    "Calls to CVMCCMruntimeF2I        ",
    "Calls to CVMCCMruntimeF2L        ",
    "Calls to CVMCCMruntimeF2D        ",
    "Calls to CVMCCMruntimeD2I        ",
    "Calls to CVMCCMruntimeD2L        ",
    "Calls to CVMCCMruntimeD2F        ",
    "Calls to CVMCCMruntimeLCmp       ",
    "Calls to CVMCCMruntimeFCmp       ",
    "Calls to CVMCCMruntimeDCmpl      ",
    "Calls to CVMCCMruntimeDCmpg      ",
    "Calls to CVMCCMruntimeThrowClass ",
    "Calls to CVMCCMruntimeThrowObject",
    "Calls to CVMCCMruntimeCheckCast  ",
    "Calls to ...CheckArrayAssignable ",
    "Calls to CVMCCMruntimeInstanceOf ",
    "Calls to ...LookupInterfaceMB    ",
    "Calls to CVMCCMruntimeNew        ",
    "Calls to CVMCCMruntimeNewArray   ",
    "Calls to CVMCCMruntimeANewArray  ",
    "Calls to ...MultiANewArray       ",
    "Calls to ...RunClassInitializer  ",
    "Calls to ...ResolveClassBlock    ",
    "Calls to ...ResolveArrayClassBlock",
    "Calls to ....GetfieldFieldOffset ",
    "Calls to ....PutfieldFieldOffset ",
    "Calls to ....MethodBlock         ",
    "Calls to ....SpecialMethodBlock  ",
    "Calls to ....MethodTableOffset   ",
    "Calls ....NewClassBlockAndClinit ",
    "....GetstaticFieldBlockAndClinit ",
    "....PutstaticFieldBlockAndClinit ",
    "....StaticMethodBlockAndClinit   ",
    "Calls to ...Putstatic64Volatile  ",
    "Calls to ...Getstatic64Volatile  ",
    "Calls to ...Putfield64Volatile   ",
    "Calls to ...Getfield64Volatile   ",
    "Calls to ...MonitorEnter         ",
    "Calls to CVMCCMruntimeMonitorExit",
    "Calls to ...GCRendezvous         ",
#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
    "Calls to ...SimpleSyncUnlock     ",
#endif

    /* Misc Stats: */
    "Byte-code size                   ",
    "Byte-codes inlined               ",
    "Generated code size              ",
    "Generated to byte-code size ratio",
    "Allocated codebuffer bytes       ",
    "Wasted codebuffer bytes          ",
    "Wasted as % of codebuffer alloc  ",
    "Number of emitted branches       ",
    "Number of emitted memory reads   ",
    "Number of emitted memory writes  ",
    "Number of stackmaps captured     ",
    "Number of constants emitted      ",
    "IR Node to Bytecode ratio        ",
    "Stackmap to Bytecode ratio       ",

    /* Compilation Time Stats: */
    "Compilation time (ms)            ",
    "ms/bytecode bytes (w/ inlined)   ",
    "ms/bytecode bytes (w/o inlined)  ",
};

/*
 * Dump the collected statistics
 */
static const char* statsTagName(CVMJITStatsTag tag)
{
    /* Make sure the tag names table is of the right size */
    CVMassert(ARRAY_SIZE(tagNames) == CVMJIT_STATS_NUM_TAGS);
    CVMassert(tag < CVMJIT_STATS_NUM_TAGS);
    return tagNames[tag];
}

/* Purpose: Dumps the stats for the compilation of a single method. */
void 
CVMJITstatsDump(CVMJITCompilationContext *con)
{
    if (!CVMJITstatsCollectNone()) {
        const CVMJITStatsSection *section;
        CVMUint32 sectionIndex;
        CVMUint32 i;
        CVMBool hideSection = CVM_FALSE;

        CVMconsolePrintf("COMPILATION ENDED FOR %C.%M\n",
                         CVMmbClassBlock(con->mb), con->mb);

        sectionIndex = 0;
        section = &statsSections[0];
        for (i = 0; i < CVMJIT_STATS_NUM_TAGS; i++) {
            /* Apply section info if necessary: */
            if ((section != NULL) && (i == section->startOfSection)) {
                hideSection = section->hideSection;
                if (sectionIndex < ARRAY_SIZE(statsSections)) {
                    sectionIndex++;
                    section = &statsSections[sectionIndex];
                } else {
                    section = NULL;
                }
            }

            if (!hideSection) {
                CVMconsolePrintf("\t%s  %d\n", statsTagName(i),
                                 con->stats->stats[i]);
            }
        }
    }
}

/* Purpose: Prints a section separator. */
static void
CVMJITstatsDumpSectionSeparator(const CVMJITStatsSection *section)
{
    CVMassert(!CVMJITstatsCollectNone());

    CVMconsolePrintf("\n");
    if (section->name != NULL) {
        CVMconsolePrintf("  %s:\n", section->name);
    }
    if (section->printLegend) {
        CVMconsolePrintf("    %s : %10s %6s %8s", 
            "Category                         ",
            "Total", "Min", "Max");
        if (CVMJITstatsCollectMore()) {
            CVMconsolePrintf(" %8s %8s %-13s %-13s",
                "Average", "StdDev", "  Range 68%", "  Range 95%");
        }
        CVMconsolePrintf("\n");
    }
}

static void
CVMJITstatsDumpGlobalStatsRowDefault(CVMJITGlobalStats *gstats, CVMUint32 i,
                                     double *stdDevValues,
                                     double *averageValues)
{
    CVMconsolePrintf("    %s : %10d %6d %8d", statsTagName(i),
                     gstats->totalStats[i],
                     gstats->minStats[i],
                     gstats->maxStats[i]);
    /* Print "more" stats if necessary: */
    if (CVMJITstatsCollectMore()) {
        double average = averageValues[i];
        double stdDev = stdDevValues[i];
        CVMconsolePrintf(" %8.1f %8.1f %6d,%-6d %6d,%-6d",
            (float)average, (float)stdDev,
            (CVMInt32)(average - stdDev), (CVMInt32)(average + stdDev),
            (CVMInt32)(average - (2 * stdDev)),
            (CVMInt32)(average + (2 * stdDev)));
    }
    CVMconsolePrintf("\n");
}

static void
CVMJITstatsDumpGlobalStatsRowMultiplier(CVMJITGlobalStats *gstats, CVMUint32 i,
                                        double *stdDevValues,
                                        double *averageValues)
{
    float total, min, max;
#undef SET_VALUE
#ifdef HUGE
#define MARKER_VALUE	HUGE
#else
#define MARKER_VALUE	1.e16
#endif
#define SET_VALUE(x, y) \
    x = ((y == CVMJIT_STATS_MAX_VALUE) ? MARKER_VALUE : ((float)y / 100.0))

    SET_VALUE(total, gstats->totalStats[i]);
    SET_VALUE(min, gstats->minStats[i]);
    SET_VALUE(max, gstats->maxStats[i]);
#undef SET_VALUE
#undef MARKER_VALUE

    CVMconsolePrintf("    %s : %9.1fx %5.1fx %7.1fx", statsTagName(i),
                     total, min, max);
    /* Print "more" stats if necessary: */
    if (CVMJITstatsCollectMore()) {
        double average = averageValues[i] / 100.0;
        double stdDev = stdDevValues[i] / 100.0;
        CVMconsolePrintf(" %7.1fx %7.1fx %6d,%-6d %6d,%-6d",
            (float)average, (float)stdDev,
            (CVMInt32)(average - stdDev), (CVMInt32)(average + stdDev),
            (CVMInt32)(average - (2 * stdDev)),
            (CVMInt32)(average + (2 * stdDev)));
    }
    CVMconsolePrintf("\n");
}

static void
CVMJITstatsDumpGlobalStatsRowPercent(CVMJITGlobalStats *gstats, CVMUint32 i,
                                     double *stdDevValues,
                                     double *averageValues)
{
    CVMconsolePrintf("    %s : %9d%% %5d%% %7d%%", statsTagName(i),
                     gstats->totalStats[i],
                     gstats->minStats[i],
                     gstats->maxStats[i]);
    /* Print "more" stats if necessary: */
    if (CVMJITstatsCollectMore()) {
        double average = averageValues[i];
        double stdDev = stdDevValues[i];
        CVMconsolePrintf(" %7.1f%% %7.1f%% %6d,%-6d %6d,%-6d",
            (float)average, (float)stdDev,
            (CVMInt32)(average - stdDev), (CVMInt32)(average + stdDev),
            (CVMInt32)(average - (2 * stdDev)),
            (CVMInt32)(average + (2 * stdDev)));
    }
    CVMconsolePrintf("\n");
}

/* Purpose: Dumps the global stats. */
void
CVMJITstatsDumpGlobalStats()
{
    CVMJITGlobalStats *gstats = CVMglobals.jit.globalStats;
    double stdDevValues[CVMJIT_STATS_NUM_TAGS];
    double averageValues[CVMJIT_STATS_NUM_TAGS];
    CVMUint32 i;
    CVMUint32 sectionIndex;
    const CVMJITStatsSection *section;
    CVMUint32 formatIndex;
    const CVMJITStatsFormat *formatRec;
    CVMJITStatsConstant **sortedConstantsList = NULL;
    CVMUint32 numberOfConstantsEmitted = 0;
    CVMUint32 numberOfUniqueConstantsEmitted = 0;
#ifdef CVM_JIT_EXCLUDE_TOP_N_CONST
    CVMUint32 methodsBelowConstThreshold = 0;
#endif
    CVMBool hideSection = CVM_FALSE;

    if (CVMJITstatsAborted()) {
        goto handleAbort;
    }

    /* If we have no stats, there is nothing to dump: */
    if (CVMJITstatsCollectNone()) {
        return;
    }

    CVMconsolePrintf("\nDone collecting statistics!\n");
    CVMconsolePrintf("Processing statistics ...");

    /* Compute the standard deviation values if necessary: */
    if (CVMJITstatsCollectMore()) {
        if (CVMJITstatsCollectConstants()) {
            sortedConstantsList =
                CVMJITstatsTallyConstantEmmisions(gstats->perCompilationStats,
                    &numberOfConstantsEmitted,
                    &numberOfUniqueConstantsEmitted);
            if (CVMJITstatsAborted()) {
                goto handleAbort;
            }
#ifdef CVM_JIT_EXCLUDE_TOP_N_CONST
            methodsBelowConstThreshold =
                CVMJITstatsExcludeTopConstantsFromStats(
                    gstats->perCompilationStats,
                    sortedConstantsList, NUMBER_OF_CONST_TO_EXCLUDE);
#endif
        }
        CVMJITstatsComputeStandardDeviation(gstats->perCompilationStats,
            gstats->numberOfCompilations, averageValues, stdDevValues);
    }

    CVMconsolePrintf("\nDone processing statistics!\n");

    /* Print the title: */
    CVMconsolePrintf("\nCUMMULATIVE JIT STATISTICS\n");

    /* Print the number of compilations: */
    CVMconsolePrintf("  Number of Compilations = %d\n",
                     gstats->numberOfCompilations);

    /* Print the other stats: */
    sectionIndex = 0;
    section = &statsSections[0];
    formatIndex = 0;
    formatRec = &statsFormats[0];
    for (i = 0; i < CVMJIT_STATS_NUM_TAGS; i++) {
        CVMJITStatsFormatType format = CVMJIT_STATS_DEFAULT;

        /* Print the section header if necessary: */
        if ((section != NULL) && (i == section->startOfSection)) {
            hideSection = section->hideSection;
            if (!hideSection) {
                CVMJITstatsDumpSectionSeparator(section);
            }
            if (sectionIndex < ARRAY_SIZE(statsSections)) {
                sectionIndex++;
                section = &statsSections[sectionIndex];
            } else {
                section = NULL;
            }
        }

        /* Change the formatting strings if necessary: */
        if ((formatRec != NULL) && (i == formatRec->tag)) {
            format = formatRec->formatType;
            if (formatIndex < ARRAY_SIZE(statsFormats)) {
                formatIndex++;
                formatRec = &statsFormats[formatIndex];
            } else {
                formatRec = NULL;
            }
        }

        if (!hideSection) {
            /* Print a row of stats: */
            switch(format) {
                case CVMJIT_STATS_MULTIPLIER:
                    CVMJITstatsDumpGlobalStatsRowMultiplier(gstats, i,
                        stdDevValues, averageValues);
                    break;
                case CVMJIT_STATS_PERCENT:
                    CVMJITstatsDumpGlobalStatsRowPercent(gstats, i,
                        stdDevValues, averageValues);
                    break;
                default:
                    CVMJITstatsDumpGlobalStatsRowDefault(gstats, i,
                        stdDevValues, averageValues);
            }
        }
    }

    /* Print the compilation speed estimates if necessary: */
    {
        float speedWInline;
        float speedWOInline;
        float fudge;
        speedWInline =
            ((float)(gstats->totalStats[CVMJIT_STATS_BC_SIZE] +
                     gstats->totalStats[CVMJIT_STATS_BC_SIZE_INLINED]) *
             1000.0 /
             (float)gstats->totalStats[CVMJIT_STATS_COMPILATION_TIME]);
        speedWOInline =
            ((float)gstats->totalStats[CVMJIT_STATS_BC_SIZE] * 1000.0 /
             (float)gstats->totalStats[CVMJIT_STATS_COMPILATION_TIME]);

        /* Apply fudge adjustments to account for the slow down due to stats
           collection: */
        fudge = 1.127;
#ifdef CVM_TRACE_JIT
        /* Apply fudge adjustments to account for the slow down due to tracing
           activity: */
        fudge *= 2.586;
#endif
        /* Apply fudge adjustments to account for the slow down due to
           collecting constant emission stats: */
        if (CVMJITstatsCollectConstants()) {
            fudge *= 1.029;
        }

        /* Print the speed data and estimates: */
        CVMconsolePrintf("    Bytecode bytes/sec (w/ inlined)   : %10.2f\n",
                         speedWInline);
        CVMconsolePrintf("    Bytecode bytes/sec (w/o inlined)  : %10.2f\n",
                         speedWOInline);
        CVMconsolePrintf("    Estimated fudge factor            : %10.3f\n",
                         fudge);
        CVMconsolePrintf("    Estimated speed bytes/sec (w/ I)  : %10.2f\n",
                         (speedWInline * fudge));
        CVMconsolePrintf("    Estimated speed bytes/sec (w/o I) : %10.2f\n",
                         (speedWOInline * fudge));
        CVMconsolePrintf("\n");
    }

    /* Print the constant emission stats if necessary: */
    if (CVMJITstatsCollectConstants()) {
        CVMUint32 totalRefCount;

        CVMconsolePrintf("  Constants Emission Statistics:\n");
#ifdef CVM_JIT_EXCLUDE_TOP_N_CONST
        CVMconsolePrintf("    Number of methods with <= %d consts = %d\n",
                         NUMBER_OF_CONST_PER_METHOD_THRESHOLD,
                         methodsBelowConstThreshold);
#endif
        CVMconsolePrintf("    Number of Constants emitted        = %d\n",
                         numberOfConstantsEmitted);
        CVMconsolePrintf("    Number of Unique Constants emitted = %d\n",
                         numberOfUniqueConstantsEmitted);
        CVMconsolePrintf("    Number of Redundancies             = %d\n",
                         numberOfConstantsEmitted -
                         numberOfUniqueConstantsEmitted);

        CVMconsolePrintf("\n");
        CVMconsolePrintf("    rank   value      : "
                         "refCnt    total   %%    category  comment\n");
        totalRefCount = 0;
        for(i = 0; i < numberOfUniqueConstantsEmitted; i++) {
            const char *comment = "<symbol info unavailable>";
#ifdef CVM_TRACE_JIT
            comment = sortedConstantsList[i]->comment;
#endif
            totalRefCount += sortedConstantsList[i]->refCount;
            CVMconsolePrintf("    [%4d] 0x%-8x : %6d %8d %5.2f%% %-8s %s\n",
                             i+1, sortedConstantsList[i]->value,
                             sortedConstantsList[i]->refCount,
                             totalRefCount,
                             ((float)totalRefCount * 100.0 /
                              numberOfConstantsEmitted),
                             sortedConstantsList[i]->category,
                             comment);
        }
        CVMconsolePrintf("\n");
    }

    if (CVMJITstatsCollectVerbose()) {
        CVMUint32 methodIdx;
        CVMJITStats *record = gstats->perCompilationStats;
        CVMconsolePrintf("  Per method compilation stats "
                         "(%d methods compiled):\n",
                         gstats->numberOfCompilations);
        methodIdx = 0;
        for (; record != NULL; record = record->prev) {
            CVMUint32 i;
            CVMconsolePrintf("%d", methodIdx + 1);
            for (i = 0; i < CVMJIT_STATS_NUM_TAGS; i++) {
                CVMconsolePrintf("\t%d", record->stats[i]);
            }
            CVMconsolePrintf("\n");
            methodIdx++;
        }
        CVMconsolePrintf("  End of per method compilation stats\n");
        CVMconsolePrintf("\n");
    }

    goto cleanUp;

handleAbort:
    CVMconsolePrintf("\nStatistics collection was aborted due to insufficient"
                     "memory.\n");

cleanUp:
    if (sortedConstantsList != NULL) {
        CVMJITstatsDeleteSortedConstantsList(sortedConstantsList,
                                             numberOfUniqueConstantsEmitted);
    }
}

#undef ARRAY_SIZE

#endif /* CVM_JIT_COLLECT_STATS */
