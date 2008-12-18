/*
 * @(#)executejava_aligned.c	1.404 06/10/25
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
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/opcodes.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/classes.h"
#include "javavm/include/stacks.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/globals.h"
#include "javavm/include/preloader.h"
#include "javavm/include/utils.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/clib.h"

#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/float.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/jni.h"

#ifdef CVM_JVMTI
#include "javavm/include/jvmti_impl.h"
#include "generated/offsets/java_lang_Thread.h"
#endif

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

#ifdef CVM_JIT
#include "javavm/include/jit_common.h"
#endif

/*
 * The following macros are used to define how opcode dispatching is
 * done. The are all described immediately below and defined a bit later
 * based on the compiler be used and the target processor.
 *
 * CVM_USELABELS - use gcc style first class label.
 * CVM_PREFETCH_OPCODE - force prefetching of next opcode.
 * CVM_ALIGNMENT_BASED_DISPATCHING - use alignment-based dispatching.
 * CVM_OPCODE_ALIGN_SIZE - opcode size for alignment-based dispatching.
 * CVM_ALIGN_OPCODE - macro used to actually do the opcode alignment. 
 * CVM_CHECK_OPCODE_SIZES - debugging support for alignment-based dispatching.
 * CVM_ADD_4K_PAD - Add a 4K pad for better caching behaviour.
 * CVM_ALIGN_4K - Macro used to assist in doing the 4K pad.
 */

/*
 * CVM_USELABELS - If defined, then gcc-style first class labels are
 * used for the opcode dispatching rather than a switch statement.
 *
 * Using this feataure improves performance because it allows the compiler
 * to schedule the opcode dispatching code in with the opcode's
 * implementation.
 * 
 * NOTE: will be defined automatically if CVM_ALIGNMENT_BASED_DISPATCHING
 * is defined.
 */

/*
 * CVM_PREFETCH_OPCODE - if defined, then forces the next opcode to be
 * fetched within the implementation of each opcode.
 *
 * Some compilers do better if you prefetch the next opcode before
 * going back to the top of the while loop, rather then having the top
 * of the while loop handle it. This provides a better opportunity for
 * instruction scheduling. Some compilers just do this prefetch
 * automatically. Some actually end up with worse performance if you
 * force the prefetch. Solaris gcc seems to do better, but cc does
 * worse.
 *
 * NOTE: will be defined automatically if CVM_USELABELS is defined.
 */

/*
 * CVM_ALIGNMENT_BASED_DISPATCHING - Use dispatching technique that assumes
 * all opcodes are aligned to a certain sized boundary.
 *
 * Some compilers support the ability to align opcodes on a certain
 * sized boundary. If you do this and also make the implementation of
 * all opcodes no bigger than the alignment size, then you can do what
 * is known as alignment-based dispatching. The address you need to
 * jump to for an opocode's implementation is always the address of
 * the first opcode (opc_nop) + the opcode left shifted a certain
 * number of bits. On some platforms this saves you an instruction and
 * in all cases is saves you having to do a memory fetch from the goto
 * table.
 *
 * The following must hold true for alignment-based dispatching to work:
 * 1. The alignment size (CVM_OPCODE_ALIGN_SIZE) must be a power of 2.
 * 2. All opcode implementations must be no bigger than CVM_OPCODE_ALIGN_SIZE.
 * 3. The opcodes must be implemented in order of their opcode number.
 *
 * (2) has already been taken care of for you on sparc and sarm when the
 * CVM_OPCODE_ALIGN_SIZE is 64 bytes. Other platform may require more work
 * in the interpreter loop to get the size down to 64 bytes. No attempt
 * was made to keep opcodes small enough to fit in 64 bytes on x86 because
 * there didn't seem to be a performance gain. Note that all types of
 * debugging must be turned off to keep the opcode size no bigger than 64.
 *
 * (3) has also already been taken care of for all platforms.
 */

/*
 * CVM_OPCODE_ALIGN_SIZE - the size in bytes to align each opcode to if
 * CVM_ALIGNMENT_BASED_DISPATCHING is defined.
 */

/*
 * CVM_ALIGN_OPCODE - macro used to actually do the opcode alignment.
 */

/*
 * CVM_CHECK_OPCODE_SIZES - if defined, then debugging code is added
 * that will print out the size of each opcode implementations and
 * warn you if any are too big.
 *
 * WARNING: Only enable this for debugging purposes. Otherwise it may
 * slow down the interpreter loop very slightly.
 *
 * WARNING: turning on this option could possibly affect how the compiler
 * allocates registers for executeJava(), causing some opcodes to have
 * a different size than they would if this option was off. To date this
 * does not seem to have been a problem on sparc, arm, and x86.
 */

/*
 * CVM_ADD_4K_PAD - if defined, then a 4k pad block is addd after the
 * implementation of the opcodes in order to improve caching behaviour
 * for the interpreter loop.
 *
 * The main loop is a bit less than 16k (~250 opcodes times 64 bytes
 * per opcode). It was noticed that 45% of the opcodes executed are in
 * the first 4k of the loop. By putting a 4k pad of after the loop,
 * this prevent any of the overflow (spill over) code located after
 * the opcodes from executing in the same cache lines as the first 4K
 * of the loop. This gives about a 4% performance boost on sparc.
 *
 * The following assumptions are made. Otherwise this code may not help
 * performance and will waste space.
 * 1. A 16K instructon cache is assumed.
 * 2. The cache address that any address maps to is simply its
 *    address & 0x3FFF.
 * 3. The amount of code in excuteJava before the first opcode is less
 *    than about 300 bytes
 *
 * NOTE: CVM_ADD_4K_PAD will be disabled in CVM_ALIGNMENT_BASED_DISPATCHING
 * is not defined.
 */

/*
 * CVM_ALIGN_4K - macro used to do assist in adding the 4K padd if
 * CVM_ADD_4K_PAD is defined.
 */


/*
 * Always prefetch opcodes unless we are using the sun cc compiler
 * for sparc, in which case doing the prefetch seems to cause slower
 * code to be generated.
 */
#if defined(__GNUC__) || !defined(__sparc__)
#define CVM_PREFETCH_OPCODE
#endif

/*
 * Only do label dispatching when using gcc.
 */
#ifdef __GNUC__
#define CVM_USELABELS
#endif

/* 
 * Only do alignment-based dispatching when ussing gcc and 
 * compiling for sparc or sarm.
 */
#if defined(__GNUC__) && \
   (defined(__sparc__) || defined(arm) || defined(__arm__) || defined(__arm))
/*#define CVM_ALIGNMENT_BASED_DISPATCHING*/
#endif

/*
 * Setup various alignment-based dispatching info for various processors.
 * Note that these will only be used if CVM_ALIGNMENT_BASED_DISPATCHING
 * is defined.
 */

#if defined(__sparc__) || defined(arm) || defined(__arm__) || defined(__arm)
#define CVM_OPCODE_ALIGN_SIZE 64
#else
#define CVM_OPCODE_ALIGN_SIZE 128
#endif

#if defined(__sparc__)
#define CVM_ADD_4K_PAD
#endif

#if defined(arm) || defined(__arm__) || defined(__arm)
#define CVM_ALIGN_OPCODE __asm__ __volatile__ ( ".align 6" ::: "memory") 
#define CVM_ALIGN_4K     __asm__ __volatile__ ( ".align 12" ::: "memory");
#elif defined(__sparc__)
#define CVM_ALIGN_OPCODE __asm__ __volatile__ ( ".align 64" ::: "memory") 
#define CVM_ALIGN_4K     __asm__ __volatile__ ( ".align 4096" ::: "memory");
#else
#define CVM_ALIGN_OPCODE __asm__ __volatile__ ( ".align 128" ::: "memory") 
#define CVM_ALIGN_4K     __asm__ __volatile__ ( ".align 8192" ::: "memory");
#endif

/*
 * If CVM_ALIGNMENT_BASED_DISPATCHING is defined and you want some
 * debugging output to tell you how big each opcode implementation is,
 * then define CVM_CHECK_OPCODE_SIZES.
 */
#if 0 /* only turn on for debugging */
#define CVM_CHECK_OPCODE_SIZES
#endif

/*
 * None of the following can be true if using alignment-based dispatching.
 */
#if defined(CVM_JVMPI_TRACE_INSTRUCTION) || \
    defined(CVM_JVMTI) || defined(CVM_DEBUG) || defined (CVM_DEBUG_ASSERTS) || \
    defined(CVM_INSTRUCTION_COUNTING) || !defined(CVM_OPTIMIZED)
#undef CVM_ALIGNMENT_BASED_DISPATCHING
#endif

/*
 * Alignment-based dispatching is required for CVM_CHECK_OPCODE_SIZES.
 */
#ifndef CVM_ALIGNMENT_BASED_DISPATCHING
#undef CVM_CHECK_OPCODE_SIZES
#endif

/*
 * If CVM_ALIGNMENT_BASED_DISPATCHING is not defined, the make sure that
 * CVM_ALIGN_OPCODE does nothing.
 */
#ifndef CVM_ALIGNMENT_BASED_DISPATCHING
#undef  CVM_ALIGN_OPCODE
#define CVM_ALIGN_OPCODE
#endif

/*
 * CVM_ALIGNMENT_BASED_DISPATCHING requires CVM_USELABELS 
 */
#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
#define CVM_USELABELS
#endif

/*
 * CVM_USELABELS requires CVM_PREFETCH_OPCODE 
 */
#ifdef CVM_USELABELS
#define CVM_PREFETCH_OPCODE
#endif

/*
 * CVM_GENERATE_ASM_LABELS - Define this if you want an asm label generated
 * at the start of each opcode and goto label. This makes it much easier
 * to read the generated assembler. However, this seems to slightly change
 * the generated code in some very minor ways on some processors. x86 does
 * not seem to be affected, but sparc does. This option only works with gcc.
 */
#if 0 /* off by default */
#define CVM_GENERATE_ASM_LABELS
#endif

/*
 * Currently, because of the funny way sarm lays out references to
 * global data, we need to turn on CVM_GENERATE_ASM_LABELS in order to
 * get the references to fall in just the right place. Otherwise it will
 * end up making an opcode too big.
 */
#if defined(CVM_ALIGNMENT_BASED_DISPATCHING) && \
    (defined(arm) || defined(__arm__) || defined(__arm))
#define CVM_GENERATE_ASM_LABELS
#endif

/*
 * GET_INDEX - Macro used for getting an unaligned unsigned short from
 * the byte codes.
 */
#undef  GET_INDEX
#define GET_INDEX(ptr) (CVMgetUint16(ptr))

/* %comment c001 */
#define CVMisROMPureCode(cb) (CVMcbIsInROM(cb))

/*
 * Tracing macros
 */
#undef TRACE
#undef TRACEIF
#undef TRACESTATUS

#define TRACE(a) CVMtraceOpcode(a)

#define TRACESTATUS()							    \
    CVMtraceStatus(("stack=0x%x frame=0x%x locals=0x%x tos=0x%x pc=0x%x\n", \
		    stack, frame, locals, topOfStack, pc));

#ifdef CVM_TRACE
#define TRACEIF(o, a)				\
    if (pc[0] == o) {				\
        TRACE(a);				\
    }
#else
#define TRACEIF(o, a)
#endif

/* 
 * INIT_CLASS_IF_NEEDED - Makes sure that static initializers are
 * run for a class.  Used in opc_*_checkinit_quick opcodes.
 */
#undef INIT_CLASS_IF_NEEDED
#define INIT_CLASS_IF_NEEDED(ee, cb)		\
    if (CVMcbInitializationNeeded(cb, ee)) {	\
        initCb = cb;				\
	goto init_class;			\
    }						\
    JVMPI_TRACE_INSTRUCTION();

#ifdef CVM_INSTRUCTION_COUNTING

#include "generated/javavm/include/opcodeSimplification.h"

/*
 * Instruction counting. CVMbigCount is incremented every 1M
 * insns. CVMsmallCount keeps track of the small change.  
 */
static CVMUint32  CVMsmallCount = 0;
static CVMUint32  CVMbigCount = 0;

/*
 * Maximum length of sequences tracked
 */
#define CVM_SEQ_DEPTH 3

/*
 * An information node. Incidence of byte-codes at this depth, and pointers
 * to information nodes at the next depth.
 */
struct CVMRunInfo {
    int incidence[256];
    struct CVMRunInfo* followers[256];
};
typedef struct CVMRunInfo CVMRunInfo;

/*
 * The sliding window of byte-codes most recently executed.
 */
static CVMOpcode CVMopcodeWindow[CVM_SEQ_DEPTH];
static int CVMopcodeWindowIdx;

/*
 * The root of all sequence information nodes.
 */
static CVMRunInfo* CVMseqInfo;

/*
 * Record window of last executed opcodes, starting from index idx.
 * Lazily create information nodes.
 */ 
static void
CVMrecordWindow(int idx)
{
    int i;
    CVMRunInfo* record   = CVMseqInfo;
    CVMOpcode currOpcode = CVMopcodeWindow[idx];

    /*
     * Traverse information node tree, creating nodes as needed.
     */
    for (i = 1; i < CVM_SEQ_DEPTH; i++) {
	CVMUint32   theIdx;
	CVMRunInfo* nextRecord;

	record->incidence[currOpcode]++;
	nextRecord = record->followers[currOpcode];
	if (nextRecord == 0) {
	    nextRecord = calloc(1, sizeof(CVMRunInfo));
	    record->followers[currOpcode] = nextRecord;
	}
	record = nextRecord;
	theIdx = (idx + i) % CVM_SEQ_DEPTH;
	currOpcode = CVMopcodeWindow[theIdx];
    }
    /*
     * We are now at the right level. Record this opcode.
     */
    record->incidence[currOpcode]++;
}

static void
CVMupdateStats(CVMOpcode opcode)
{
    CVMopcodeWindow[CVMopcodeWindowIdx] = CVMopcodeSimplification[opcode];
    CVMopcodeWindowIdx++;
    CVMopcodeWindowIdx %= CVM_SEQ_DEPTH;
    CVMrecordWindow(CVMopcodeWindowIdx);
}

void 
CVMinitStats()
{
    CVMopcodeWindowIdx = 0;
    CVMseqInfo = calloc(1, sizeof(CVMRunInfo));
}

static void
CVMdumpStats2()
{
    CVMOpcode opcode1, opcode2;
    CVMconsolePrintf("Instruction count=%dM[,%d]\n",
		     CVMbigCount, CVMsmallCount);
    printf("\n\n\n\nSequences:\n");
    for (opcode1 = 0; opcode1 < 255; opcode1++) {
	for (opcode2 = 0; opcode2 < 255; opcode2++) {
	    CVMRunInfo* info = CVMseqInfo->followers[opcode1];
	    if (info != 0) {
		if (info->incidence[opcode2] != 0) {
		    printf("%d Seq: <%s>,<%s>\n",
			       info->incidence[opcode2],
			       CVMopnames[opcode1],
			       CVMopnames[opcode2]);
		}
	    }
	}
    }
}

static void
CVMdumpStats3()
{
    CVMOpcode opcode1, opcode2, opcode3;
    CVMconsolePrintf("Instruction count=%dM[,%d]\n",
		     CVMbigCount, CVMsmallCount);
    printf("\n\n\n\nSequences:\n");
    for (opcode1 = 0; opcode1 < 255; opcode1++) {
	for (opcode2 = 0; opcode2 < 255; opcode2++) {
	    for (opcode3 = 0; opcode3 < 255; opcode3++) {
		CVMRunInfo* info = CVMseqInfo->followers[opcode1];
		if (info != 0) {
		    info = info->followers[opcode2];
		    if (info != 0) {
			if (info->incidence[opcode3] != 0) {
			    printf("%d Seq: <%s>,<%s>,<%s>\n",
				       info->incidence[opcode3],
				       CVMopnames[opcode1],
				       CVMopnames[opcode2],
				       CVMopnames[opcode3]);
			}
		    }
		}
	    }
	}
    }
}

static void
CVMdumpStats4()
{
    CVMOpcode opcode1, opcode2, opcode3, opcode4;
    CVMconsolePrintf("Instruction count=%dM[,%d]\n",
		     CVMbigCount, CVMsmallCount);
    printf("\n\n\n\nSequences:\n");
    for (opcode1 = 0; opcode1 < 255; opcode1++) {
	for (opcode2 = 0; opcode2 < 255; opcode2++) {
	    for (opcode3 = 0; opcode3 < 255; opcode3++) {
		for (opcode4 = 0; opcode4 < 255; opcode4++) {
		    CVMRunInfo* info = CVMseqInfo->followers[opcode1];
		    if (info != 0) {
			info = info->followers[opcode2];
			if (info != 0) {
			    info = info->followers[opcode3];
			    if (info != 0) {
				if (info->incidence[opcode4] != 0) {
				    printf("%d Seq: <%s>,<%s>,<%s>,<%s>\n",
					       info->incidence[opcode4],
					       CVMopnames[opcode1],
					       CVMopnames[opcode2],
					       CVMopnames[opcode3],
					       CVMopnames[opcode4]);
				}
			    }
			}
		    }
		}
	    }
	}
    }
}

void
CVMdumpStats()
{
    CVMdumpStats2();
    CVMdumpStats3();
/*      CVMdumpStats4(); */
}

#define UPDATE_INSTRUCTION_COUNT(opcode)		\
{							\
    CVMsmallCount++;					\
    if (CVMsmallCount % 1000000 == 0) {			\
	CVMbigCount++;					\
        CVMsmallCount = 0;				\
    }							\
    CVMupdateStats(opcode);				\
}
#else
#define UPDATE_INSTRUCTION_COUNT(opcode)
#endif


#ifdef CVM_JVMTI
/* NOTE: This macro must be called AFTER the PC has been
   incremented. CVMjvmtiNotifyDebuggerOfSingleStep may cause a
   breakpoint opcode to get inserted at the current PC to allow the
   debugger to coalesce single-step events. Therefore, we need to
   refetch the opcode after calling out to JVMTI. */
#define JVMTI_SINGLE_STEPPING()					   \
{									\
    if (ee->jvmtiSingleStepping && CVMjvmtiThreadEventsEnabled(ee)) {	\
	DECACHE_PC();						   	\
	DECACHE_TOS();						   	\
	CVMD_gcSafeExec(ee, {					   	\
	    CVMjvmtiPostSingleStepEvent(ee, pc);			\
	});								\
	/* Refetch opcode. See above. */				\
	FETCH_NEXT_OPCODE(0);      					\
    }									\
}

#define JVMTI_WATCHING_FIELD_ACCESS(location)				\
    if (CVMglobals.jvmtiWatchingFieldAccess) {				\
	DECACHE_PC();							\
	DECACHE_TOS();							\
	CVMD_gcSafeExec(ee, {						\
		CVMjvmtiPostFieldAccessEvent(ee, location, fb);		\
	});								\
    }

#define JVMTI_WATCHING_FIELD_MODIFICATION(location, isDoubleWord, isRef)      \
   if (CVMglobals.jvmtiWatchingFieldModification) {			      \
       jvalue val;							      \
       DECACHE_PC();							      \
       DECACHE_TOS();							      \
       if (isDoubleWord) {						      \
	   CVMassert(CVMfbIsDoubleWord(fb));				      \
	   if (CVMtypeidGetType(CVMfbNameAndTypeID(fb)) == CVM_TYPEID_LONG) { \
	       val.j = CVMjvm2Long(&STACK_INFO(-2).raw);		      \
	   } else /* CVM_TYPEID_DOUBLE */ {				      \
	       val.d = CVMjvm2Double(&STACK_INFO(-2).raw);		      \
	   }								      \
       } else if (isRef) {						      \
	   val.l = &STACK_ICELL(-1);					      \
       } else {								      \
	   val.i = STACK_INT(-1);					      \
       }								      \
       CVMD_gcSafeExec(ee, {						      \
	       CVMjvmtiPostFieldModificationEvent(ee, location, fb, val); \
       });								      \
   }

#else
#define JVMTI_SINGLE_STEPPING()
#define JVMTI_WATCHING_FIELD_ACCESS(location)
#define JVMTI_WATCHING_FIELD_MODIFICATION(location, isDoubleWord, isRef)

#endif

#ifdef CVM_JVMPI
#define JVMPI_CHECK_FOR_DATA_DUMP_REQUEST(ee) \
    if (CVMjvmpiDataDumpWasRequested()) {     \
        DECACHE_PC();                         \
        DECACHE_TOS();                        \
        CVMD_gcSafeExec(ee, {                 \
            CVMjvmpiPostDataDumpRequest();    \
        });                                   \
    }

#else
#define JVMPI_CHECK_FOR_DATA_DUMP_REQUEST(ee)
#endif

#ifdef CVM_JVMPI_TRACE_INSTRUCTION
/* The following macros are for posting the JVMPI_EVENT_INSTRUCTION_START
 * event for JVMPI support of tracing bytecode execution: */

#define JVMPI_TRACE_INSTRUCTION() {                     \
    if (CVMjvmpiEventInstructionStartIsEnabled()) {     \
        DECACHE_PC();                                   \
        DECACHE_TOS();                                  \
        CVMjvmpiPostInstructionStartEvent(ee, pc);      \
    }                                                   \
}

#define JVMPI_TRACE_IF(isTrue) {                                \
    if (CVMjvmpiEventInstructionStartIsEnabled()) {             \
        DECACHE_PC();                                           \
        DECACHE_TOS();                                          \
        CVMjvmpiPostInstructionStartEvent4If(ee, pc, isTrue);   \
    }                                                           \
}

#define JVMPI_TRACE_TABLESWITCH(key, low, hi) {                      \
    if (CVMjvmpiEventInstructionStartIsEnabled()) {                  \
        DECACHE_PC();                                                \
        DECACHE_TOS();                                               \
        CVMjvmpiPostInstructionStartEvent4Tableswitch(ee, pc, key,   \
                                                      low, hi);      \
    }                                                                \
}

#define JVMPI_TRACE_LOOKUPSWITCH(chosenPairIndex, pairsTotal) { \
    if (CVMjvmpiEventInstructionStartIsEnabled()) {             \
        DECACHE_PC();                                           \
        DECACHE_TOS();                                          \
        CVMjvmpiPostInstructionStartEvent4Lookupswitch(ee, pc,  \
                                chosenPairIndex, pairsTotal);   \
    }                                                           \
}

#else /* !CVM_JVMPI_TRACE_INSTRUCTION */
#define JVMPI_TRACE_INSTRUCTION()
#define JVMPI_TRACE_IF(isTrue)
#define JVMPI_TRACE_TABLESWITCH(key, low, hi)
#define JVMPI_TRACE_LOOKUPSWITCH(chosenPairIndex, pairsTotal)
#endif /* CVM_JVMPI_TRACE_INSTRUCTION */

/*
 * GCC macro for adding an assembler label so the .s file is easier to read.
 */
#undef ASMLABEL
#if defined(__GNUC__) && defined(CVM_GENERATE_ASM_LABELS)
#define ASMLABEL(name) \
    __asm__ __volatile__ ("LL_" #name ":");
#else
#define ASMLABEL(name)
#endif

/*
 * FETCH_NEXT_OPCODE() - macro for fetching to the next opcode or the next
 * opcode goto label if we are using gcc first class labels.
 */
#undef FETCH_NEXT_OPCODE
#if defined(CVM_ALIGNMENT_BASED_DISPATCHING)
#define FETCH_NEXT_OPCODE(opsize)		\
    nextOpcodeImpl = (void*)(firstOpcodeImplPtr + pc[opsize]);
#elif defined(CVM_PREFETCH_OPCODE)
#define FETCH_NEXT_OPCODE(opsize) \
    opcode = pc[opsize];
#else
#define FETCH_NEXT_OPCODE(opsize) 
#endif

/*
 * DISPATCH_OPCODE - macro for dispatching to the next opcode implementation.
 */
#undef DISPATCH_OPCODE
#if defined(CVM_ALIGNMENT_BASED_DISPATCHING)
#define DISPATCH_OPCODE() goto *nextOpcodeImpl;
#elif defined(CVM_USELABELS)
#define DISPATCH_OPCODE() goto *opclabels[opcode];
#else
#define DISPATCH_OPCODE() continue;
#endif

/*
 * REEXECUTE_BYTECODE - Macro for reexecuting the opcode.
 */
#undef REEXECUTE_BYTECODE
#ifdef CVM_JVMTI
#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
/* #define REEXECUTE_BYTECODE() goto *nextOpcodeImpl; */
#error "Not currently supported"
#elif defined(CVM_USELABELS)
#define REEXECUTE_BYTECODE() goto *opclabels[opcode];
#elif defined(CVM_PREFETCH_OPCODE)
#define REEXECUTE_BYTECODE() goto opcode_switch;
#else
#define REEXECUTE_BYTECODE() goto opcode_switch;
#endif
#endif /* CVM_JVMTI */

/*
 * CASE - Macro for setting up an opcode case or goto label.
 * DEFAULT - Macro for setting up the default case or goto label.
 */
#undef CASE
#undef DEFAULT
#undef CASE_KEYWORD
#undef DEFAULT_KEYWORD
#ifdef CVM_USELABELS
#define CASE_KEYWORD
#define DEFAULT_KEYWORD opc_DEFAULT
#else
#define CASE_KEYWORD    case
#define DEFAULT_KEYWORD default
#endif
#define CASE(opcode_, opsize)			\
    CASE_KEYWORD opcode_:		       	\
    CVM_ALIGN_OPCODE;				\
    ASMLABEL(opcode_);				\
    if (opsize != 0) {				\
        JVMPI_TRACE_INSTRUCTION();		\
        FETCH_NEXT_OPCODE(opsize);		\
    }
#define DEFAULT					\
    DEFAULT_KEYWORD:				\
    ASMLABEL(opc_DEFAULT);

/*
 * CONTINUE - Macro for executing the next opcode.
 */
#undef CONTINUE
#define CONTINUE {				\
        FETCH_NEXT_OPCODE(0);			\
	UPDATE_INSTRUCTION_COUNT(*pc);		\
        JVMTI_SINGLE_STEPPING();		\
        DISPATCH_OPCODE();   			\
    }

/*
 * UPDATE_PC_AND_TOS - Macro for updating the pc and topOfStack.
 */
#undef UPDATE_PC_AND_TOS
#define UPDATE_PC_AND_TOS(opsize, stack) {	\
        pc += opsize;				\
        topOfStack += stack;			\
	UPDATE_INSTRUCTION_COUNT(*pc);		\
        JVMTI_SINGLE_STEPPING();		\
    }

/*
 * UPDATE_PC_AND_TOS_AND_CONTINUE - Macro for updating the pc and topOfStack,
 * and executing the next opcode. It's somewhat similar to the combination
 * of UPDATE_PC_AND_TOS and CONTINUE, but with some minor optimizations.
 */
#undef UPDATE_PC_AND_TOS_AND_CONTINUE
#define UPDATE_PC_AND_TOS_AND_CONTINUE(opsize, stack) {		\
	UPDATE_PC_AND_TOS(opsize, stack);			\
        DISPATCH_OPCODE();					\
    }

/*
 * CHECK_PENDING_REQUESTS - Macro for checking pending requests which need
 * to be checked periodically in the interpreter loop.
 */
#undef CHECK_PENDING_REQUESTS
#ifdef CVM_REMOTE_EXCEPTIONS_SUPPORTED
/* %comment h001 */
#define CHECK_PENDING_REQUESTS(ee) \
	(CVMD_gcSafeCheckRequest(ee) || CVMremoteExceptionOccurred(ee))
#else
#define CHECK_PENDING_REQUESTS(ee) \
	(CVMD_gcSafeCheckRequest(ee))
#endif

/*
 * For those opcodes that need to have a GC point on a backwards branch
 */
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK
#define UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(skip, stack) { \
    FETCH_NEXT_OPCODE(skip);						   \
    if ((skip) <= 0 && CHECK_PENDING_REQUESTS(ee)) {			   \
	goto handle_pending_request;					   \
    }									   \
    JVMPI_CHECK_FOR_DATA_DUMP_REQUEST(ee);                                 \
    UPDATE_PC_AND_TOS_AND_CONTINUE((skip), stack);			   \
}

/*
 * UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK2 - Same as
 * above but the dispatching code does a goto dispatch_opcode.
 * This saves us a few bytes in the opcode implementation when
 * doing alignment based dispatching, which helps some opcodes
 * fit into the required size.
 */
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK2
#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
#define UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK2(skip, stack) { \
    FETCH_NEXT_OPCODE(skip)						    \
    if ((skip) <= 0 && CHECK_PENDING_REQUESTS(ee)) {			    \
	goto handle_pending_request;					    \
    }									    \
    JVMPI_CHECK_FOR_DATA_DUMP_REQUEST(ee);                                 \
    UPDATE_PC_AND_TOS(skip, stack);					    \
    goto dispatch_opcode;      	       				    	    \
}
#else
#define UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK2(skip, stack) \
    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(skip, stack)
#endif

/*
 * A bunch of macros for optionally handling dispatching out of line.
 * Rather than inlining the dispatching code with the opcode, you can
 * optionally choose to goto a label that will handle dispatching
 * for the opcode's specific opcode size and stack adjustment. This
 * allows us to get a number of opcodes below the 64 bytes limit when
 * using alignment based dispatching. Otherwise these macros just default
 * to inlining the dispatching code with the opcode. Note that only
 * opcodes that need to be shrunk a little to fit in 64 bytes should
 * use these macros.
 */
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_1_0
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_1_m1
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_1_m2
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_1_m4
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_2_0
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_3_0
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_3_1
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_3_2
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_3_m2
#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
#define UPDATE_PC_AND_TOS_AND_CONTINUE_1_0   \
    goto update_pc_and_tos_and_continue_1_0;
#define UPDATE_PC_AND_TOS_AND_CONTINUE_1_m1   \
    goto update_pc_and_tos_and_continue_1_m1;
#define UPDATE_PC_AND_TOS_AND_CONTINUE_1_m2   \
    goto update_pc_and_tos_and_continue_1_m2;
#define UPDATE_PC_AND_TOS_AND_CONTINUE_1_m3   \
    goto update_pc_and_tos_and_continue_1_m3;
#define UPDATE_PC_AND_TOS_AND_CONTINUE_1_m4   \
    goto update_pc_and_tos_and_continue_1_m4;
#define UPDATE_PC_AND_TOS_AND_CONTINUE_2_0   \
    goto update_pc_and_tos_and_continue_2_0;
#define UPDATE_PC_AND_TOS_AND_CONTINUE_3_0   \
    goto update_pc_and_tos_and_continue_3_0;
#define UPDATE_PC_AND_TOS_AND_CONTINUE_3_1   \
    goto update_pc_and_tos_and_continue_3_1;
#define UPDATE_PC_AND_TOS_AND_CONTINUE_3_2   \
    goto update_pc_and_tos_and_continue_3_2;
#define UPDATE_PC_AND_TOS_AND_CONTINUE_3_m2   \
    goto update_pc_and_tos_and_continue_3_m2;
#else
#define UPDATE_PC_AND_TOS_AND_CONTINUE_1_0	\
    FETCH_NEXT_OPCODE(1);			\
    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
#define UPDATE_PC_AND_TOS_AND_CONTINUE_1_m1	\
    FETCH_NEXT_OPCODE(1);			\
    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
#define UPDATE_PC_AND_TOS_AND_CONTINUE_1_m2	\
    FETCH_NEXT_OPCODE(1);			\
    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -2);
#define UPDATE_PC_AND_TOS_AND_CONTINUE_1_m3	\
    FETCH_NEXT_OPCODE(1);			\
    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -3);
#define UPDATE_PC_AND_TOS_AND_CONTINUE_1_m4	\
    FETCH_NEXT_OPCODE(1);			\
    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -4);
#define UPDATE_PC_AND_TOS_AND_CONTINUE_2_0	\
    FETCH_NEXT_OPCODE(2);			\
    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 0);
#define UPDATE_PC_AND_TOS_AND_CONTINUE_3_0	\
    FETCH_NEXT_OPCODE(3);			\
    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 0);
#define UPDATE_PC_AND_TOS_AND_CONTINUE_3_1	\
    FETCH_NEXT_OPCODE(3);			\
    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
#define UPDATE_PC_AND_TOS_AND_CONTINUE_3_2	\
    FETCH_NEXT_OPCODE(3);			\
    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 2);
#define UPDATE_PC_AND_TOS_AND_CONTINUE_3_m2	\
    FETCH_NEXT_OPCODE(3);			\
    UPDATE_PC_AND_TOS_AND_CONTINUE(3, -2);
#endif


/*
 * Macros for accessing the stack.
 */
#undef STACK_INFO
#undef STACK_INT
#undef STACK_FLOAT
#undef STACK_ICELL
#undef STACK_ADDR
#undef STACK_OBJECT
#undef STACK_DOUBLE
#undef STACK_LONG
#define STACK_SLOT(offset)    (topOfStack[offset].s)
#define STACK_INFO(offset)    (STACK_SLOT(offset).j)
#define STACK_ADDR(offset)    (STACK_SLOT(offset).a)
#define STACK_INT(offset)     (STACK_INFO(offset).i)
#define STACK_FLOAT(offset)   (STACK_INFO(offset).f)
#define STACK_ICELL(offset)   (STACK_INFO(offset).r)
#define STACK_OBJECT(offset)  (CVMID_icellDirect(ee, &STACK_ICELL(offset)))

#define STACK_DOUBLE(offset) \
        CVMjvm2Double(&STACK_INFO(offset).raw)
#define STACK_LONG(offset)   \
        CVMjvm2Long(&STACK_INFO(offset).raw)

#ifdef CVM_TRACE
#undef GET_LONGCSTRING
#define GET_LONGCSTRING(number) \
        (CVMlong2String(number, trBuf, trBuf+sizeof(trBuf)), trBuf)
#endif

/*
 * Macros for caching and flushing the interpreter state. Some local
 * variables need to be flushed out to the frame before we do certain
 * things (like pushing frames or becomming gc safe) and some need to 
 * be recached later (like after popping a frame). We could use one
 * macro to cache or decache everything, but this would be less then
 * optimal because we don't always need to cache or decache everything
 * because some things we know are already cached or decached.
 */
#undef DECACHE_TOS
#undef CACHE_TOS
#undef CACHE_PREV_TOS
#define DECACHE_TOS()    frame->topOfStack = topOfStack;
#define CACHE_TOS()   	 topOfStack = frame->topOfStack;
#define CACHE_PREV_TOS() topOfStack = CVMframePrev(frame)->topOfStack;

#undef DECACHE_PC
#undef CACHE_PC
#define DECACHE_PC()	CVMframePc(frame) = pc;
#define CACHE_PC()      pc = CVMframePc(frame);

#undef CACHE_FRAME
#define CACHE_FRAME()   frame = stack->currentFrame;
 
/*
 * CHECK_NULL - Macro for throwing a NullPointerException if the object
 * passed is a null ref.
 */
#undef CHECK_NULL
#define CHECK_NULL(obj_)			\
    if ((obj_) == 0) {				\
        goto null_pointer_exception;		\
    }

/*
 * Function to verify that all opcodes are less than CVM_OPCODE_ALIGN_SIZE
 * in bytes. It can also be modified to print out the size of each
 * opcode implementation. We don't do this inline or static, because
 * this can disrupt gcc register allocation.
 */
#ifdef CVM_CHECK_OPCODE_SIZES
extern void
CVMcheckOpcodeSizes(const void* firstOpcodeImplPtr,
		    const void* const *opclabels)
{
    int i;
    const void* opcodePtr = firstOpcodeImplPtr;
    for (i = 0; i < 255; i++) {
	int opcodeSize = (int)opclabels[i+1] - (int)opcodePtr;
	if (opcodeSize > CVM_OPCODE_ALIGN_SIZE) {
#if 1
	    opcodePtr += CVM_OPCODE_ALIGN_SIZE;
	    printf("interpreter loop opcode too big!\n");
#else
	    CVMpanic("interpreter loop opcode too big!");
#endif
	}
	/*printf("0x%5x: %d %-20s\n", opcodeSize, i, CVMopnames[i]);*/
	printf("0x%5x: %d\n", opcodeSize, i);
	opcodePtr += CVM_OPCODE_ALIGN_SIZE;
    }
}
#endif /* CVM_CHECK_OPCODE_SIZES */

/*
 * The following 4 structures are static java bytecodes
 * that contain the transition code for various types of method
 * invocations. They are used to handle the invocation of
 * the mb passed into executeJavaMethod.
 */
static const CVMUint8 CVMinvokeStaticTransitionCode[] = {
    opc_invokestatic_quick, 0, 1, opc_exittransition
};
static const CVMUint8 CVMinvokeVirtualTransitionCode[] = {
    opc_invokevirtual_quick_w, 0, 1, opc_exittransition
};
static const CVMUint8 CVMinvokeNonVirtualTransitionCode[] = {
    opc_invokenonvirtual_quick, 0, 1, opc_exittransition
};
static const CVMUint8 CVMinvokeInterfaceTransitionCode[] = {
    opc_invokeinterface_quick, 0, 1, 0, 0, opc_exittransition
};

/*
 * CVMgcUnsafeExecuteJavaMethod
 *
 * The real deal. This is where byte codes actually get interpreted.
 * Basically it's a big while loop that iterates until we return from
 * the method passed in.
 *
 * As long as java code is being executed, the interpreter is iterative,
 * not recursive. However, if native code is invoked, then it may re-enter
 * executeJavaMethod. In this case there will already be frames on
 * the stack.
 *
 * Upon entry:
 *   -ee->interpreterStack.currentFrame must be a transition
 *    frame that holds the method arguments.
 *   -currentFrame->topOfStack must point after the arguments.
 */
void
CVMgcUnsafeExecuteJavaMethod(CVMExecEnv* ee, CVMMethodBlock* mb,
			     CVMBool isStatic, CVMBool isVirtual)
{
    CVMFrame*         frame;         /* actually it's a CVMJavaFrame */
    CVMFrame*         initialframe;  /* the initial transition frame
					that holds the arguments */

    CVMStack*         stack = &ee->interpreterStack;
    CVMStackVal32*    topOfStack; /* access with STACK_ and _TOS macros */    
    CVMSlotVal32*     locals = 0; /* area in the frame for local variables */
    CVMUint8*         pc;
#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
    const void*       nextOpcodeImpl;
#else
    CVMUint32         opcode;
#endif
    CVMConstantPool*  cp;
    CVMTransitionConstantPool   transitioncp;
    CVMClassBlock*    initCb;
    CVMClassBlock*    newCb;
    CVMObjectICell*   receiverObjICell; /* the object we are dispatching on or
					   the Class object for static calls */
    CVMArrayOfAnyType* directArr;   /* for opc_newarray */

#ifdef CVM_CLASSLOADING
    CVMBool clobbersCpIndex; /* true if quickening clobbers the cp index */
#endif
#ifdef CVM_TRACE
    char              trBuf[30];   /* needed by GET_LONGCSTRING */
#endif

#ifdef CVM_USELABELS
#include "generated/javavm/include/opcodeLabels.h"
    const static void* const opclabels_data[256] = { 
	&&opc_0, &&opc_1, &&opc_2, &&opc_3, &&opc_4, &&opc_5,
	&&opc_6, &&opc_7, &&opc_8, &&opc_9, &&opc_10, &&opc_11,
	&&opc_12, &&opc_13, &&opc_14, &&opc_15, &&opc_16, &&opc_17,
	&&opc_18, &&opc_19, &&opc_20, &&opc_21, &&opc_22, &&opc_23,
	&&opc_24, &&opc_25, &&opc_26, &&opc_27, &&opc_28, &&opc_29,
	&&opc_30, &&opc_31, &&opc_32, &&opc_33, &&opc_34, &&opc_35,
	&&opc_36, &&opc_37, &&opc_38, &&opc_39, &&opc_40, &&opc_41,
	&&opc_42, &&opc_43, &&opc_44, &&opc_45, &&opc_46, &&opc_47,
	&&opc_48, &&opc_49, &&opc_50, &&opc_51, &&opc_52, &&opc_53,
	&&opc_54, &&opc_55, &&opc_56, &&opc_57, &&opc_58, &&opc_59,
	&&opc_60, &&opc_61, &&opc_62, &&opc_63, &&opc_64, &&opc_65,
	&&opc_66, &&opc_67, &&opc_68, &&opc_69, &&opc_70, &&opc_71,
	&&opc_72, &&opc_73, &&opc_74, &&opc_75, &&opc_76, &&opc_77,
	&&opc_78, &&opc_79, &&opc_80, &&opc_81, &&opc_82, &&opc_83,
	&&opc_84, &&opc_85, &&opc_86, &&opc_87, &&opc_88, &&opc_89,
	&&opc_90, &&opc_91, &&opc_92, &&opc_93, &&opc_94, &&opc_95,
	&&opc_96, &&opc_97, &&opc_98, &&opc_99, &&opc_100, &&opc_101,
	&&opc_102, &&opc_103, &&opc_104, &&opc_105, &&opc_106, &&opc_107,
	&&opc_108, &&opc_109, &&opc_110, &&opc_111, &&opc_112, &&opc_113,
	&&opc_114, &&opc_115, &&opc_116, &&opc_117, &&opc_118, &&opc_119,
	&&opc_120, &&opc_121, &&opc_122, &&opc_123, &&opc_124, &&opc_125,
	&&opc_126, &&opc_127, &&opc_128, &&opc_129, &&opc_130, &&opc_131,
	&&opc_132, &&opc_133, &&opc_134, &&opc_135, &&opc_136, &&opc_137,
	&&opc_138, &&opc_139, &&opc_140, &&opc_141, &&opc_142, &&opc_143,
	&&opc_144, &&opc_145, &&opc_146, &&opc_147, &&opc_148, &&opc_149,
	&&opc_150, &&opc_151, &&opc_152, &&opc_153, &&opc_154, &&opc_155,
	&&opc_156, &&opc_157, &&opc_158, &&opc_159, &&opc_160, &&opc_161,
	&&opc_162, &&opc_163, &&opc_164, &&opc_165, &&opc_166, &&opc_167,
	&&opc_168, &&opc_169, &&opc_170, &&opc_171, &&opc_172, &&opc_173,
	&&opc_174, &&opc_175, &&opc_176, &&opc_177, &&opc_178, &&opc_179,
	&&opc_180, &&opc_181, &&opc_182, &&opc_183, &&opc_184, &&opc_185,
	&&opc_186, &&opc_187, &&opc_188, &&opc_189, &&opc_190, &&opc_191,
	&&opc_192, &&opc_193, &&opc_194, &&opc_195, &&opc_196, &&opc_197,
	&&opc_198, &&opc_199, &&opc_200, &&opc_201, &&opc_202, &&opc_203,
	&&opc_204, &&opc_205, &&opc_206, &&opc_207, &&opc_208, &&opc_209,
	&&opc_210, &&opc_211, &&opc_212, &&opc_213, &&opc_214, &&opc_215,
	&&opc_216, &&opc_217, &&opc_218, &&opc_219, &&opc_220, &&opc_221,
	&&opc_222, &&opc_223, &&opc_224, &&opc_225, &&opc_226, &&opc_227,
	&&opc_228, &&opc_229, &&opc_230, &&opc_231, &&opc_232, &&opc_233,
	&&opc_234, &&opc_235, &&opc_236, &&opc_237, &&opc_238, &&opc_239,
	&&opc_240, &&opc_241, &&opc_242, &&opc_243, &&opc_244, &&opc_245,
	&&opc_246, &&opc_247, &&opc_248, &&opc_249, &&opc_250, &&opc_251,
	&&opc_252, &&opc_253, &&opc_254, &&opc_255
    };
    const void* const *opclabels = &opclabels_data[0];

#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
    typedef struct {
	CVMUint8 bytecodes[CVM_OPCODE_ALIGN_SIZE];
    } CVMopcodeImplementation;

    /* Don't included the "const" below or bad code will be generated. */
    const CVMopcodeImplementation* /* const */ firstOpcodeImplPtr =
	&&opc_first_opcode;
#endif /* CVM_ALIGNMENT_BASED_DISPATCHING */
#endif /* CVM_USELABELS */
  
    goto executejava_setup;

dispatch_opcode:
    ASMLABEL(label_dispatch_opcode);

    FETCH_NEXT_OPCODE(0);

#ifdef CVM_USELABELS
    DISPATCH_OPCODE();
#endif
 
#ifndef CVM_USELABELS
    while (1)
#endif
    {
#ifndef CVM_PREFETCH_OPCODE
	opcode = *pc;
#endif
#ifndef CVM_USELABELS
    opcode_switch:
	switch (opcode)
#endif
	{
	CASE(opc_nop, 1)
#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
	opc_first_opcode:
#endif
	    TRACE(("\t%s\n", CVMopnames[pc[0]]));
	    FETCH_NEXT_OPCODE(1);
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

	    /* Push miscellaneous constants onto the stack. */

	CASE(opc_aconst_null, 1)
	    CVMID_icellSetNull(&STACK_ICELL(0));
	    TRACE(("\t%s\n", CVMopnames[pc[0]]));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);

#undef  OPC_CONST_n
#define OPC_CONST_n(opcode, const_type, value) 				\
	CASE(opcode, 1) 						\
	    STACK_INFO(0).const_type = value; 				\
	    TRACE(("\t%s\n", CVMopnames[pc[0]])); 			\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);

	    /* TODO - outline to make pc-relative table smaller on sarm */
#undef  OPC_CONST2_n
#define OPC_CONST2_n(opcname, value, key)				  \
        CASE(opc_##opcname, 1)						  \
            CVM##key##2Jvm(&STACK_INFO(0).raw, CVM##key##Const##value()); \
	    TRACE(("\t%s\n", CVMopnames[pc[0]]));			  \
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 2);

	    OPC_CONST_n(opc_iconst_m1,   i,       -1);
	    OPC_CONST_n(opc_iconst_0,    i,        0);
	    OPC_CONST_n(opc_iconst_1,    i,        1);
	    OPC_CONST_n(opc_iconst_2,    i,        2);
	    OPC_CONST_n(opc_iconst_3,    i,        3);
	    OPC_CONST_n(opc_iconst_4,    i,        4);
	    OPC_CONST_n(opc_iconst_5,    i,        5);

	    OPC_CONST2_n(lconst_0, Zero, long);
	    OPC_CONST2_n(lconst_1, One,  long);

	    OPC_CONST_n(opc_fconst_0,    f,      0.0);
	    OPC_CONST_n(opc_fconst_1,    f,      1.0);
	    OPC_CONST_n(opc_fconst_2,    f,      2.0);

	    OPC_CONST2_n(dconst_0, Zero, double);
	    OPC_CONST2_n(dconst_1, One,  double);

	    /* Push a 1-byte signed integer value onto the stack. */
	CASE(opc_bipush, 2)
	    STACK_INT(0) = (CVMInt8)(pc[1]);
	    TRACE(("\tbipush %d\n", STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 1);

	    /* Push a 2-byte signed integer constant onto the stack. */
	CASE(opc_sipush, 3)
	    STACK_INT(0) = CVMgetInt16(pc + 1);
	    TRACE(("\tsipush %d\n", STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);

	/* A few of the non-quick opcodes */

#ifdef  CVM_CLASSLOADING
        CASE(opc_ldc, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;
        CASE(opc_ldc_w, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;
	CASE(opc_ldc2_w, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;
#else
        CASE(opc_ldc, 0)
	    goto unimplemented_opcode;
        CASE(opc_ldc_w, 0)
	    goto unimplemented_opcode;
	CASE(opc_ldc2_w, 0)
	    goto unimplemented_opcode;
#endif

	/* load from local variable */

	CASE(opc_iload, 2) {
	    CVMUint32    localNo = pc[1]; 
            CVMSlotVal32 l = locals[localNo];
	    topOfStack += 1;
	    STACK_SLOT(-1) = l;
	    TRACE(("\tiload locals[%d](%d) =>\n", localNo, STACK_INT(-1)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 0);
	}

	/* outline to make room for pc-relative table on sarm */
        CASE(opc_lload, 2)
	    CVMmemCopy64(&STACK_INFO(0).raw, &locals[pc[1]].j.raw);
	    TRACE(("\tlload locals[%d](%s) =>\n",
		   pc[1], GET_LONGCSTRING(STACK_LONG(0))));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 2);

	CASE(opc_fload, 2) {
	    CVMUint32    localNo = pc[1]; 
            CVMSlotVal32 l = locals[localNo];
	    topOfStack += 1;
	    STACK_SLOT(-1) = l;
	    TRACE(("\tfload locals[%d](%f) =>\n", localNo, STACK_FLOAT(-1)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 0);
	}

        CASE(opc_dload, 2)
	    CVMmemCopy64(&STACK_INFO(0).raw, &locals[pc[1]].j.raw);
	    TRACE(("\tdload locals[%d](%f) =>\n", pc[1], STACK_DOUBLE(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 2);

	CASE(opc_aload, 2) {
	    CVMUint32    localNo = pc[1]; 
            CVMSlotVal32 l = locals[localNo];
	    topOfStack += 1;
	    STACK_SLOT(-1) = l;
	    TRACE(("\taload locals[%d](0x%x) =>\n",
		   localNo, STACK_OBJECT(-1)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 0);
	}

#undef  OPC_ILOAD_n
#define OPC_ILOAD_n(num)					\
	CASE(opc_iload_##num, 1) {				\
            CVMSlotVal32 l;					\
            pc += 1;						\
            l = locals[num];					\
            topOfStack += 1;					\
	    STACK_SLOT(-1) = l;					\
	    TRACE(("\t%s locals[%d](%d) =>\n",			\
		   CVMopnames[pc[0]], num, STACK_INT(-1)));	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(0, 0);		\
	}

	    OPC_ILOAD_n(0);
	    OPC_ILOAD_n(1);
	    OPC_ILOAD_n(2);
	    OPC_ILOAD_n(3);

#undef  OPC_LLOAD_n
#define OPC_LLOAD_n(num)						\
	CASE(opc_lload_##num, 1)					\
	    CVMmemCopy64(&STACK_INFO(0).raw, &locals[num].j.raw);	\
	    TRACE(("\t%s locals[%d](%s) =>\n",				\
		   CVMopnames[pc[0]], num,				\
		   GET_LONGCSTRING(STACK_LONG(0))));			\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 2);

	    OPC_LLOAD_n(0);
	    OPC_LLOAD_n(1);
	    OPC_LLOAD_n(2);
	    OPC_LLOAD_n(3);

#undef  OPC_FLOAD_n
#define OPC_FLOAD_n(num)					\
	CASE(opc_fload_##num, 1)      				\
	    STACK_SLOT(0) = locals[num];			\
	    TRACE(("\t%s locals[%d](%d) =>\n",			\
		   CVMopnames[pc[0]], num, STACK_FLOAT(0)));	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);

	    OPC_FLOAD_n(0);
	    OPC_FLOAD_n(1);
	    OPC_FLOAD_n(2);
	    OPC_FLOAD_n(3);

#undef  OPC_DLOAD_n
#define OPC_DLOAD_n(num)						\
	CASE(opc_dload_##num, 1)       					\
	    CVMmemCopy64(&STACK_INFO(0).raw, &locals[num].j.raw);	\
	    TRACE(("\t%s locals[%d](%f) =>\n",				\
		     CVMopnames[pc[0]], num, STACK_DOUBLE(0)));	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 2);

	    OPC_DLOAD_n(0);
	    OPC_DLOAD_n(1);
	    OPC_DLOAD_n(2);
	    OPC_DLOAD_n(3);

#undef  OPC_ALOAD_n
#define OPC_ALOAD_n(num)						\
	CASE(opc_aload_##num, 1)       					\
	    STACK_SLOT(0) = locals[num];				\
	    TRACE(("\t%s locals[%d](0x%x) =>\n",		       	\
		   CVMopnames[pc[0]], num, STACK_OBJECT(0)));	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);

	    OPC_ALOAD_n(0);
	    OPC_ALOAD_n(1);
	    OPC_ALOAD_n(2);
	    OPC_ALOAD_n(3);

	/* Array load opcodes */

	/* Every array load and store opcodes starts out like this */
#define ARRAY_INTRO(T, arrayOff)				      	      \
        CVMArrayOf##T* arrObj = (CVMArrayOf##T*)STACK_OBJECT(arrayOff);       \
	CVMJavaInt     index  = STACK_INT(arrayOff + 1);		      \
        CHECK_NULL(arrObj);						      \
        /* This clever idea is from ExactVM. No need to test index >= 0.      \
	   Instead, do an unsigned comparison. Negative indices will appear   \
	   as positive numbers >= 2^31, which is just out of the range	      \
	   of legal Java array indices */				      \
	if ((CVMUint32)index >= CVMD_arrayGetLength(arrObj)) {		      \
	    goto array_index_out_of_bounds_exception;    		      \
	}

	/* 32-bit loads. These handle conversion from < 32-bit types */
#define ARRAY_LOADTO32(T, format, stackRes)				      \
        {								      \
	    ARRAY_INTRO(T, -2);						      \
	    CVMD_arrayRead##T(arrObj, index, stackRes(-2));		      \
	    TRACE(("\t%s %O[%d](" format ") ==>\n", CVMopnames[pc[0]],	      \
		   arrObj, index, stackRes(-1))); 			      \
            UPDATE_PC_AND_TOS_AND_CONTINUE_1_m1;			      \
        }

	/* 64-bit loads */
#define ARRAY_LOADTO64(T,T2)						\
        {								\
	    CVMJava##T temp64;						\
            ARRAY_INTRO(T, -2);						\
	    CVMD_arrayRead##T(arrObj, index, temp64);			\
	    CVM##T2##2Jvm(&STACK_INFO(-2).raw, temp64);			\
	    TRACE(("\t%s %O[%d](0x%X, 0x%X) ==>\n", CVMopnames[pc[0]],	\
		   arrObj, index, STACK_INT(-2), STACK_INT(-1)));      	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE_1_0;				\
        }

        CASE(opc_iaload, 1)
	    ARRAY_LOADTO32(Int,   "%d",   STACK_INT);
        CASE(opc_laload, 1)
	    ARRAY_LOADTO64(Long,long);
        CASE(opc_faload, 1)
	    ARRAY_LOADTO32(Float, "%f",   STACK_FLOAT);
        CASE(opc_daload, 1)
	    goto opc_daload_overflow;
        CASE(opc_aaload, 1)
	    ARRAY_LOADTO32(Ref,   "0x%x", STACK_OBJECT);
        CASE(opc_baload, 1)
	    ARRAY_LOADTO32(Byte,  "%d",   STACK_INT);
        CASE(opc_caload, 1)
	    ARRAY_LOADTO32(Char,  "%d",   STACK_INT);
        CASE(opc_saload, 1)
	    ARRAY_LOADTO32(Short, "%d",   STACK_INT);

	/* store to a local variable */

	CASE(opc_istore, 2)
	    locals[pc[1]] = STACK_SLOT(-1);
	    TRACE(("\tistore %d ==> locals[%d]\n", STACK_INT(-1), pc[1]));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, -1);

	CASE(opc_lstore, 2)
	    CVMmemCopy64(&locals[pc[1]].j.raw, &STACK_INFO(-2).raw);
	    TRACE(("\tlstore %s => locals[%d]\n",
		   GET_LONGCSTRING(STACK_LONG(-2)), pc[1]));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, -2);

	CASE(opc_fstore, 2)
	    locals[pc[1]] = STACK_SLOT(-1);
	    TRACE(("\tfstore %f ==> locals[%d]\n", STACK_FLOAT(-1), pc[1]));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, -1);

	CASE(opc_dstore, 2)
	    CVMmemCopy64(&locals[pc[1]].j.raw, &STACK_INFO(-2).raw);
	    TRACE(("\tdstore %f => locals[%d]\n", STACK_DOUBLE(-2), pc[1]));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, -2);

	CASE(opc_astore, 2)
	    locals[pc[1]] = STACK_SLOT(-1);
	    TRACE(("\tastore 0x%x => locals[%d]\n", STACK_OBJECT(-1), pc[1]));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, -1);

#undef  OPC_ISTORE_n
#define OPC_ISTORE_n(num)					\
	CASE(opc_istore_##num, 1)				\
	    locals[num] = STACK_SLOT(-1);			\
	    TRACE(("\t%s %d => locals[%d]\n",			\
		   CVMopnames[pc[0]], STACK_INT(-1), num));	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

	    OPC_ISTORE_n(0);
	    OPC_ISTORE_n(1);
	    OPC_ISTORE_n(2);
	    OPC_ISTORE_n(3);

#undef  OPC_LSTORE_n
#define OPC_LSTORE_n(num)						\
	CASE(opc_lstore_##num, 1)					\
	    CVMmemCopy64(&locals[num].j.raw, &STACK_INFO(-2).raw);	\
	    TRACE(("\t%s %s => locals[%d]\n",				\
		   CVMopnames[pc[0]],					\
                   GET_LONGCSTRING(STACK_LONG(-2)), num));		\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -2);

	    OPC_LSTORE_n(0);
	    OPC_LSTORE_n(1);
	    OPC_LSTORE_n(2);
	    OPC_LSTORE_n(3);

#undef  OPC_FSTORE_n
#define OPC_FSTORE_n(num)					\
	CASE(opc_fstore_##num, 1)      				\
	    locals[num] = STACK_SLOT(-1);			\
	    TRACE(("\t%s %f => locals[%d]\n",			\
		   CVMopnames[pc[0]], STACK_FLOAT(-1), num));	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

	    OPC_FSTORE_n(0);
	    OPC_FSTORE_n(1);
	    OPC_FSTORE_n(2);
	    OPC_FSTORE_n(3);

#undef  OPC_DSTORE_n
#define OPC_DSTORE_n(num)						\
	CASE(opc_dstore_##num, 1)      					\
	    CVMmemCopy64(&locals[num].j.raw, &STACK_INFO(-2).raw);	\
	    TRACE(("\t%s %f => locals[%d]\n",				\
		   CVMopnames[pc[0]], STACK_DOUBLE(-2), num));		\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -2);

	    OPC_DSTORE_n(0);
	    OPC_DSTORE_n(1);
	    OPC_DSTORE_n(2);
	    OPC_DSTORE_n(3);

#undef  OPC_ASTORE_n
#define OPC_ASTORE_n(num)					\
	CASE(opc_astore_##num, 1)      				\
	    locals[num] = STACK_SLOT(-1);			\
	    TRACE(("\t%s 0x%x => locals[%d]\n",			\
		   CVMopnames[pc[0]], STACK_OBJECT(-1), num));	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

	    OPC_ASTORE_n(0);
	    OPC_ASTORE_n(1);
	    OPC_ASTORE_n(2);
	    OPC_ASTORE_n(3);

	/* Array store opcodes */

	/* 32-bit stores. These handle conversion to < 32-bit types */
#define ARRAY_STOREFROM32(T, format, stackSrc)				      \
        {								      \
	    ARRAY_INTRO(T, -3);						      \
	    CVMD_arrayWrite##T(arrObj, index, stackSrc(-1));		      \
	    TRACE(("\t%s " format " ==> %O[%d] ==>\n", CVMopnames[pc[0]],     \
		   stackSrc(-1), arrObj, index)); 			      \
            UPDATE_PC_AND_TOS_AND_CONTINUE_1_m3;			      \
        }

	/* 64-bit stores */
#define ARRAY_STOREFROM64(T)						    \
        {								    \
	    CVMJava##T temp64;						    \
            ARRAY_INTRO(T, -4);						    \
	    temp64 = CVMjvm2##T(&STACK_INFO(-2).raw);			    \
	    CVMD_arrayWrite##T(arrObj, index, temp64);			    \
	    TRACE(("\t%s (0x%X, 0x%X) ==> %O[%d]\n", CVMopnames[pc[0]],     \
		   STACK_INT(-2), STACK_INT(-1), arrObj, index));      	    \
	    UPDATE_PC_AND_TOS_AND_CONTINUE_1_m4;			    \
        }

        CASE(opc_iastore, 1)
	    ARRAY_STOREFROM32(Int,   "%d",   STACK_INT);
        CASE(opc_lastore, 1)
	    ARRAY_STOREFROM64(Long);
        CASE(opc_fastore, 1)
	    ARRAY_STOREFROM32(Float, "%f",   STACK_FLOAT);
        CASE(opc_dastore, 1)
	    goto opc_dastore_overflow;

	/*
	 * This one looks different because of the assignability check
	 */
        CASE(opc_aastore, 1)
	    goto opc_aastore_overflow;

        CASE(opc_bastore, 1)
	    ARRAY_STOREFROM32(Byte,  "%d",   STACK_INT);
        CASE(opc_castore, 1)
	    ARRAY_STOREFROM32(Char,  "%d",   STACK_INT);
        CASE(opc_sastore, 1)
	    ARRAY_STOREFROM32(Short, "%d",   STACK_INT);

	/* stack pop, dup, and insert opcodes */
	   
	CASE(opc_pop, 1)	 /* Discard the top item on the stack */
	    TRACE(("\tpop\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

	   
	CASE(opc_pop2, 1)	 /* Discard the top 2 items on the stack */
	    TRACE(("\tpop2\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -2);
	    
	CASE(opc_dup, 1) {	/* Duplicate the top item on the stack */
	    CVMJavaVal32 rhs = STACK_INFO(-1);
	    topOfStack += 1;
	    pc += 1;
	    STACK_INFO(-1) = rhs;
	    TRACE(("\tdup\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(0, 0);
	}

	CASE(opc_dup_x1, 1)	/* insert top word two down */
	    STACK_INFO(0) = STACK_INFO(-1);
	    STACK_INFO(-1) = STACK_INFO(-2);
	    STACK_INFO(-2) = STACK_INFO(0);
	    TRACE(("\tdup_x1\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);

	CASE(opc_dup_x2, 1)	/* insert top word three down  */
	    STACK_INFO(0) = STACK_INFO(-1);
	    STACK_INFO(-1) = STACK_INFO(-2);
	    STACK_INFO(-2) = STACK_INFO(-3);
	    STACK_INFO(-3) = STACK_INFO(0);
	    TRACE(("\tdup_x2\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);

	CASE(opc_dup2, 1)      	/* Duplicate the top 2 items on the stack */
	    STACK_INFO(0) = STACK_INFO(-2);
	    STACK_INFO(1) = STACK_INFO(-1);
	    TRACE(("\tdup2\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 2);

	CASE(opc_dup2_x1, 1)	/* insert top double three down */
	    STACK_INFO(1) = STACK_INFO(-1);
	    STACK_INFO(0) = STACK_INFO(-2);
	    STACK_INFO(-1) = STACK_INFO(-3);
	    STACK_INFO(-2) = STACK_INFO(1);
	    STACK_INFO(-3) = STACK_INFO(0);
	    TRACE(("\tdup2_x1\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 2);

	CASE(opc_dup2_x2, 1)	/* insert top double four down */
	    STACK_INFO(1) = STACK_INFO(-1);
	    STACK_INFO(0) = STACK_INFO(-2);
	    STACK_INFO(-1) = STACK_INFO(-3);
	    STACK_INFO(-2) = STACK_INFO(-4);
	    STACK_INFO(-3) = STACK_INFO(1);
	    STACK_INFO(-4) = STACK_INFO(0);
	    TRACE(("\tdup2_x2\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 2);

	CASE(opc_swap, 1) {     /* swap top two elements on the stack */
	    CVMJavaVal32 j = STACK_INFO(-1);
	    STACK_INFO(-1) = STACK_INFO(-2);
	    STACK_INFO(-2) = j;
	    TRACE(("\tswap\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}

	/* binary numeric opcodes */

#undef  OPC_INT_BINARY 
#define OPC_INT_BINARY(opname, test)					   \
	    if (test && (STACK_INT(-1) == 0)) {				   \
                goto arithmetic_exception_divide_by_zero;	   	   \
	    }								   \
            STACK_INT(-2) =						   \
		CVMint##opname(STACK_INT(-2), STACK_INT(-1));		   \
	    TRACE(("\t%s => %d\n", CVMopnames[pc[0]], STACK_INT(-2)));	   \
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

#undef  OPC_LONG_BINARY
#define OPC_LONG_BINARY(opname, test)					   \
	{								   \
	    CVMJavaLong l1, l2, r1;					   \
	    l2 = CVMjvm2Long(&STACK_INFO(-2).raw);			   \
	    if (test && CVMlongEqz(l2)) {				   \
                goto arithmetic_exception_divide_by_zero;		   \
	    }								   \
	    l1 = CVMjvm2Long(&STACK_INFO(-4).raw);			   \
	    r1 = CVMlong##opname(l1, l2);				   \
	    CVMlong2Jvm(&STACK_INFO(-4).raw, r1);			   \
	    TRACE(("\t%s => %s\n", CVMopnames[pc[0]],			   \
		   GET_LONGCSTRING(STACK_LONG(-4))));			   \
	    UPDATE_PC_AND_TOS_AND_CONTINUE_1_m2;		   	   \
	}

#undef  OPC_FLOAT_BINARY 
#define OPC_FLOAT_BINARY(opname, test)					   \
            STACK_FLOAT(-2) =						   \
		CVMfloat##opname(STACK_FLOAT(-2), STACK_FLOAT(-1));	   \
            TRACE(("\t%s => %f\n", CVMopnames[pc[0]], STACK_FLOAT (-2))); \
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

#undef  OPC_DOUBLE_BINARY 
#define OPC_DOUBLE_BINARY(opname, test)					   \
	{								   \
            CVMJavaDouble l1, l2, r;					   \
            l1 = CVMjvm2Double(&STACK_INFO(-4).raw);			   \
            l2 = CVMjvm2Double(&STACK_INFO(-2).raw);			   \
            r = CVMdouble##opname(l1, l2);				   \
            CVMdouble2Jvm(&STACK_INFO(-4).raw, r);			   \
            TRACE(("\t%s => %f\n", CVMopnames[pc[0]], STACK_DOUBLE(-4)));  \
	    UPDATE_PC_AND_TOS_AND_CONTINUE_1_m2;			   \
        }

	CASE(opc_iadd, 1)
	    OPC_INT_BINARY(Add, 0);
	CASE(opc_ladd, 1)
	    OPC_LONG_BINARY(Add, 0);
	CASE(opc_fadd, 1)
	    OPC_FLOAT_BINARY(Add, 0);
	CASE(opc_dadd, 1)
	    OPC_DOUBLE_BINARY(Add, 0);

	CASE(opc_isub, 1)
	    OPC_INT_BINARY(Sub, 0);
	CASE(opc_lsub, 1)
	    OPC_LONG_BINARY(Sub, 0);
	CASE(opc_fsub, 1)
	    OPC_FLOAT_BINARY(Sub, 0);
	CASE(opc_dsub, 1)
	    OPC_DOUBLE_BINARY(Sub, 0);
	
	CASE(opc_imul, 1)
	    OPC_INT_BINARY(Mul, 0);
	CASE(opc_lmul, 1)
	    OPC_LONG_BINARY(Mul, 0);
	CASE(opc_fmul, 1)
	    OPC_FLOAT_BINARY(Mul, 0);
	CASE(opc_dmul, 1)
	    OPC_DOUBLE_BINARY(Mul, 0);
	
	CASE(opc_idiv, 1)
	    OPC_INT_BINARY(Div, 1);
	CASE(opc_ldiv, 1)
	    goto opc_ldiv_overflow;
	CASE(opc_fdiv, 1)
	    OPC_FLOAT_BINARY(Div, 1);
	CASE(opc_ddiv, 1)
	    goto opc_ddiv_overflow;

	CASE(opc_irem, 1)
	    OPC_INT_BINARY(Rem, 1);
	CASE(opc_lrem, 1)
	    goto opc_lrem_overflow;
	CASE(opc_frem, 1)
	    goto opc_frem_overflow;
	CASE(opc_drem, 1)
	    goto opc_drem_overflow;

       /* negate the value on the top of the stack */

        CASE(opc_ineg, 1)
	   STACK_INT(-1) = CVMintNeg(STACK_INT(-1));
           TRACE(("\t%s => %d\n", CVMopnames[pc[0]], STACK_INT(-1)));
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

        CASE(opc_lneg, 1)
	{
 	   CVMJavaLong r;
	   r = CVMjvm2Long(&STACK_INFO(-2).raw);
	   r = CVMlongNeg(r);
	   CVMlong2Jvm(&STACK_INFO(-2).raw, r);
           TRACE(("\t%s => %s\n", CVMopnames[pc[0]],
		  GET_LONGCSTRING(STACK_LONG(-2))));
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}

        CASE(opc_fneg, 1)
	   STACK_FLOAT(-1) = CVMfloatNeg(STACK_FLOAT(-1));
           TRACE(("\t%s => %f\n", CVMopnames[pc[0]], STACK_FLOAT(-1)));
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

        CASE(opc_dneg, 1)
	{
 	   CVMJavaDouble r;
	   r = CVMjvm2Double(&STACK_INFO(-2).raw);
	   r = CVMdoubleNeg(r);
	   CVMdouble2Jvm(&STACK_INFO(-2).raw, r);
           TRACE(("\t%s => %f\n", CVMopnames[pc[0]], STACK_DOUBLE(-2)));
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}

 	/* Shift operations				   
	 * Shift left int and long: ishl, lshl           
	 * Logical shift right int and long w/zero extension: iushr, lushr
	 * Arithmetic shift right int and long w/sign extension: ishr, lshr
	 */

#undef  OPC_SHIFT_BINARY
#define OPC_SHIFT_BINARY(opcname, opname)				\
        CASE(opc_i##opcname, 1)						\
	   STACK_INT(-2) = CVMint##opname(STACK_INT(-2), STACK_INT(-1));\
           TRACE(("\t%s => %d\n", CVMopnames[pc[0]], STACK_INT(-2)));	\
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);			\
        CASE(opc_l##opcname, 1)						\
	{								\
	   CVMJavaLong v, r;						\
	   v = CVMjvm2Long(&STACK_INFO(-3).raw);			\
	   r = CVMlong##opname(v, STACK_INT(-1));			\
	   CVMlong2Jvm(&STACK_INFO(-3).raw, r);				\
           TRACE(("\t%s => %s\n", CVMopnames[pc[0]],			\
		    GET_LONGCSTRING(STACK_LONG(-3)))); 			\
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);			\
	}
      
        OPC_SHIFT_BINARY(shl, Shl);
	OPC_SHIFT_BINARY(shr, Shr);
	OPC_SHIFT_BINARY(ushr, Ushr);

	/* binary logic opcodes */

#undef  OPC_LOGICAL_BINARY 
#define OPC_LOGICAL_BINARY(opcname, opname)				\
	CASE(opc_i##opcname, 1)						\
            STACK_INT(-2) =						\
		CVMint##opname(STACK_INT(-2), STACK_INT(-1));		\
	    TRACE(("\t%s => %d\n", CVMopnames[pc[0]], STACK_INT(-2)));	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);			\
	CASE(opc_l##opcname, 1)						\
	{								\
	    CVMJavaLong l1, l2, r1;					\
	    l2 = CVMjvm2Long(&STACK_INFO(-2).raw);			\
	    l1 = CVMjvm2Long(&STACK_INFO(-4).raw);			\
	    r1 = CVMlong##opname(l1, l2);				\
	    CVMlong2Jvm(&STACK_INFO(-4).raw, r1);			\
	    TRACE(("\t%s => %s\n", CVMopnames[pc[0]],			\
		   GET_LONGCSTRING(STACK_LONG(-4))));			\
	    UPDATE_PC_AND_TOS_AND_CONTINUE_1_m2;			\
	}

	OPC_LOGICAL_BINARY(and, And);
	OPC_LOGICAL_BINARY(or,  Or);
	OPC_LOGICAL_BINARY(xor, Xor);

       /* Increment local variable by constant */ 

        CASE(opc_iinc, 3) 
	{
	    CVMUint32  localNo = pc[1]; 
	    CVMInt32   incr = (CVMInt8)pc[2];
	    pc += 3;
	    locals[localNo].j.i += incr;
	    TRACE(("\tiinc locals[%d]+%d => %d\n", localNo, 
		   (CVMInt8)(incr), locals[localNo].j.i)); 
	    UPDATE_PC_AND_TOS_AND_CONTINUE(0, 0);
	}

	/* Conversion operations */

        CASE(opc_i2l, 1)   /* convert top of stack int to long */
	{
	    CVMJavaLong r;
	    r = CVMint2Long(STACK_INT(-1));
	    CVMlong2Jvm(&STACK_INFO(-1).raw, r);
            TRACE(("\ti2l => %s\n", GET_LONGCSTRING(STACK_LONG(-1))));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);
	}

        CASE(opc_i2f, 1)   /* convert top of stack int to float */
	   STACK_FLOAT(-1) = CVMint2Float(STACK_INT(-1));
           TRACE(("\ti2f => %f\n", STACK_FLOAT(-1)));
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

        CASE(opc_i2d, 1)   /* convert top of stack int to double */
	{
	    CVMJavaDouble r;
	    r = CVMint2Double(STACK_INT(-1));
	    CVMdouble2Jvm(&STACK_INFO(-1).raw, r);
            TRACE(("\ti2d => %f\n", STACK_DOUBLE(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);
	}

        CASE(opc_l2i, 1)   /* convert top of stack long to int */
	{
   	    CVMJavaLong r; 
	    r = CVMjvm2Long(&STACK_INFO(-2).raw);
	    STACK_INT(-2) = CVMlong2Int(r); 
            TRACE(("\tl2i => %d\n", STACK_INT(-2)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	}
	
        CASE(opc_l2f, 1)   /* convert top of stack long to float */
	{
   	    CVMJavaLong r; 
	    r = CVMjvm2Long(&STACK_INFO(-2).raw);
	    STACK_FLOAT(-2) = CVMlong2Float(r); 
            TRACE(("\tl2f => %f\n", STACK_FLOAT(-2)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	}

        CASE(opc_l2d, 1)   /* convert top of stack long to double */
	{
   	    CVMJavaLong r1; 
	    CVMJavaDouble r2;
	    r1 = CVMjvm2Long(&STACK_INFO(-2).raw);
	    r2 = CVMlong2Double(r1); 
	    CVMdouble2Jvm(&STACK_INFO(-2).raw, r2);
            TRACE(("\tl2d => %f\n", STACK_DOUBLE(-2)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}

        CASE(opc_f2i, 1)  /* Convert top of stack float to int */
            STACK_INT(-1) = CVMfloat2Int(STACK_FLOAT(-1)); 
            TRACE(("\tf2i => %d\n", STACK_INT(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

        CASE(opc_f2l, 1)  /* convert top of stack float to long */
	    goto opc_f2l_overflow;

        CASE(opc_f2d, 1)  /* convert top of stack float to double */
	{
	    CVMJavaDouble r;
	    r = CVMfloat2Double(STACK_FLOAT(-1));
	    CVMdouble2Jvm(&STACK_INFO(-1).raw, r);
            TRACE(("\tf2d => %f\n", STACK_DOUBLE(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);
        }

        CASE(opc_d2i, 1)  /* convert top of stack double to int */
   	{
	    CVMJavaDouble r1;
	    CVMJavaInt r2;
	    r1 = CVMjvm2Double(&STACK_INFO(-2).raw);
	    r2 = CVMdouble2Int(r1);
	    STACK_INT(-2) = r2;
            TRACE(("\td2i => %d\n", STACK_INT(-2)));
            UPDATE_PC_AND_TOS_AND_CONTINUE_1_m1;
	}


        CASE(opc_d2l, 1)  /* convert top of stack double to long */
	    goto opc_d2l_overflow;

        CASE(opc_d2f, 1)  /* convert top of stack double to float */
   	{
	    CVMJavaDouble r;
	    r = CVMjvm2Double(&STACK_INFO(-2).raw);
	    STACK_FLOAT(-2) = CVMdouble2Float(r);
            TRACE(("\td2f => %f\n", STACK_FLOAT(-2)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	}

        CASE(opc_i2b, 1)
	    STACK_INT(-1) = CVMint2Byte(STACK_INT(-1));
            TRACE(("\ti2b => %d\n", STACK_INT(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

        CASE(opc_i2c, 1)
	    STACK_INT(-1) = CVMint2Char(STACK_INT(-1));
            TRACE(("\ti2c => %d\n", STACK_INT(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

        CASE(opc_i2s, 1)
	    STACK_INT(-1) = CVMint2Short(STACK_INT(-1));
            TRACE(("\ti2s => %d\n", STACK_INT(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

	/* comparison operators */

        CASE(opc_lcmp, 1)
	    goto opc_lcmp_overflow;

        CASE(opc_fcmpl, 1)
	{
	    STACK_INT(-2) = 
		CVMfloatCompare(STACK_FLOAT(-2), STACK_FLOAT(-1), -1);
            TRACE(("\t%s => %d\n", CVMopnames[pc[0]], STACK_INT(-2)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE_1_m1;
	}

        CASE(opc_fcmpg, 1)
	{
	    STACK_INT(-2) =
		CVMfloatCompare(STACK_FLOAT(-2), STACK_FLOAT(-1), 1);
            TRACE(("\t%s => %d\n", CVMopnames[pc[0]], STACK_INT(-2)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE_1_m1;
	}

        CASE(opc_dcmpl, 1)
	    goto opc_dcmpl_overflow;
        CASE(opc_dcmpg, 1)
	    goto opc_dcmpg_overflow;

	/* conditional branches */

#define COMPARISON_IF(name, comparison)					\
	CASE(opc_if##name, 0) {						\
            TRACE(("\t%s goto +%d (%staken)\n",				\
		   CVMopnames[pc[0]],					\
		   CVMgetInt16(pc + 1),					\
		   (STACK_INT(-1) comparison 0) ? "" : "not "));	\
            JVMPI_TRACE_IF((STACK_INT(-1) comparison 0));               \
            if ((STACK_INT(-1) comparison 0)) {				\
	        goto opc_if_overflow;					\
	    }								\
	    FETCH_NEXT_OPCODE(3);					\
            UPDATE_PC_AND_TOS_AND_CONTINUE(3, -1);			\
        }

#define COMPARISON_IF_ICMP(name, comparison)				     \
	CASE(opc_if_icmp##name, 0) {					     \
            TRACE(("\t%s goto +%d (%staken)\n",				     \
		   CVMopnames[pc[0]],					     \
		   CVMgetInt16(pc + 1),					     \
		   (STACK_INT(-2) comparison STACK_INT(-1)) ? "" : "not ")); \
            JVMPI_TRACE_IF((STACK_INT(-2) comparison STACK_INT(-1)));        \
            if (STACK_INT(-2) comparison STACK_INT(-1)) {		     \
	        goto opc_if_icmp_overflow;				     \
	    }								     \
	    FETCH_NEXT_OPCODE(3);					     \
            UPDATE_PC_AND_TOS_AND_CONTINUE(3, -2);	     	             \
        }

#define COMPARISON_IF_ACMP(name, comparison)				    \
	CASE(opc_if_acmp##name, 0) {					    \
            TRACE(("\t%s goto +%d (%staken)\n",				    \
		   CVMopnames[pc[0]],					    \
		   CVMgetInt16(pc + 1),					    \
                   (STACK_OBJECT(-2) comparison STACK_OBJECT(-1))	    \
		   ? "" : "not "));					    \
            JVMPI_TRACE_IF((STACK_OBJECT(-2) comparison STACK_OBJECT(-1))); \
            if (STACK_INT(-2) comparison STACK_INT(-1)) {		    \
	        goto opc_if_acmp_overflow;				    \
	    }								    \
	    FETCH_NEXT_OPCODE(3);					    \
            UPDATE_PC_AND_TOS_AND_CONTINUE(3, -2);			    \
        }

	COMPARISON_IF(eq, ==);
        COMPARISON_IF(ne, !=);
	COMPARISON_IF(lt, <);
	COMPARISON_IF(ge, >=);
	COMPARISON_IF(gt, >);
	COMPARISON_IF(le, <=);

	COMPARISON_IF_ICMP(eq, ==);
        COMPARISON_IF_ICMP(ne, !=);
	COMPARISON_IF_ICMP(lt, <);
	COMPARISON_IF_ICMP(ge, >=);
	COMPARISON_IF_ICMP(gt, >);
	COMPARISON_IF_ICMP(le, <=);

	COMPARISON_IF_ACMP(eq, ==);
        COMPARISON_IF_ACMP(ne, !=);

	/* goto and jsr. They are exactly the same except jsr pushes
	 * the address of the next instruction first.
	 */

        CASE(opc_goto, 3)
	    /* This opcode is too big for alignment-based dispatching
	     * if CVM_JVMPI is defined.
	     */
#if defined(CVM_ALIGNMENT_BASED_DISPATCHING) && defined(CVM_JVMPI)
	    goto opc_goto_overflow;
#else
	{
	    CVMInt16 skip = CVMgetInt16(pc + 1);
	    TRACE(("\t%s %#x (skip=%d)\n",CVMopnames[pc[0]],
		   pc + skip, skip));
	    /* %comment h002 */
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK2(skip, 0);
	}
#endif

        CASE(opc_jsr, 3) 
	    goto opc_jsr_overflow;

	/* return from a jsr or jsr_w */

        CASE(opc_ret, 1) {
	    /* %comment h002 */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	    TRACE(("\tret %d (%#x)\n", pc[1], locals[pc[1]].a));
	    pc = locals[pc[1]].a;
	    CONTINUE;
	}

	/* Goto pc at specified offset in switch table. */

	CASE(opc_tableswitch, 0)
	    goto opc_tableswitch_overflow;

        /* Goto pc whose table entry matches specified key */

	CASE(opc_lookupswitch, 0)
	    goto opc_lookupswitch_overflow;

        /* Return from a method */

	CASE(opc_ireturn, 1)
        {
	    CVMJavaVal32 result;
	    /* 
	     * Offer a GC-safe point. 
	     */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	return1:
	    result = STACK_INFO(-1);
	    CACHE_PREV_TOS();
	    STACK_INFO(0) = result;
	    topOfStack++;
	    TRACEIF(opc_ireturn, ("\tireturn %d\n", STACK_INT(-1)));
	    TRACEIF(opc_areturn, ("\tareturn 0x%x\n", STACK_OBJECT(-1)));
	    TRACEIF(opc_freturn, ("\tfreturn %f\n", STACK_FLOAT(-1)));
	    goto handle_return;
        }

	CASE(opc_lreturn, 1)
        {
	    CVMJavaVal64 result;
	    /* 
	     * Offer a GC-safe point. 
	     */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	return2:
	    CVMmemCopy64(result.v, &STACK_INFO(-2).raw);
	    CACHE_PREV_TOS();
	    CVMmemCopy64(&STACK_INFO(0).raw, result.v);
	    topOfStack += 2;
	    TRACEIF(opc_dreturn, ("\tfreturn1 %f\n", STACK_DOUBLE(-2)));
	    TRACEIF(opc_lreturn, ("\tlreturn %s\n",
				  GET_LONGCSTRING(STACK_LONG(-2))));
	    goto handle_return;
        }

	CASE(opc_freturn, 1)
	    /* 
	     * Offer a GC-safe point. 
	     */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	    goto return1;

	CASE(opc_dreturn, 1)
	    /* 
	     * Offer a GC-safe point. 
	     */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	    goto return2;

	CASE(opc_areturn, 1)
	    /* 
	     * Offer a GC-safe point. 
	     */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	    goto return1;

	CASE(opc_return, 1) {
	    /* 
	     * Offer a GC-safe point. 
	     */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	    CACHE_PREV_TOS();
	    TRACE(("\treturn\n"));
	    goto handle_return;
	}

	/* A few of the non-quick opcodes */

#ifdef CVM_CLASSLOADING
	CASE(opc_getstatic, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;

        CASE(opc_putstatic, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;

	CASE(opc_getfield, 0)
	    clobbersCpIndex = CVM_TRUE;
	    goto quicken_opcode;

	CASE(opc_putfield, 0)
	    clobbersCpIndex = CVM_TRUE;
	    goto quicken_opcode;

        CASE(opc_invokevirtual, 0)
	    clobbersCpIndex = CVM_TRUE;
	    goto quicken_opcode;

        CASE(opc_invokespecial, 0)
	    clobbersCpIndex = CVM_TRUE;
	    goto quicken_opcode;

        CASE(opc_invokestatic, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;

	CASE(opc_invokeinterface, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;
 
       CASE(opc_xxxunusedxxx, 0)
	    goto unimplemented_opcode;
 
	CASE(opc_new, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;
#else
	CASE(opc_getstatic, 0)
	    goto unimplemented_opcode;

        CASE(opc_putstatic, 0)
	    goto unimplemented_opcode;

	CASE(opc_getfield, 0)
	    goto unimplemented_opcode;

	CASE(opc_putfield, 0)
	    goto unimplemented_opcode;

        CASE(opc_invokevirtual, 0)
	    goto unimplemented_opcode;

        CASE(opc_invokespecial, 0)
	    goto unimplemented_opcode;

        CASE(opc_invokestatic, 0)
	    goto unimplemented_opcode;

        CASE(opc_invokeinterface, 0)
	    goto unimplemented_opcode;

        CASE(opc_xxxunusedxxx, 0)
	    goto unimplemented_opcode;

	CASE(opc_new, 0)
	    goto unimplemented_opcode;
#endif

	/* Allocate an array of a basic type */

        CASE(opc_newarray, 1) {
	    CVMJavaInt         arrLen;
	    CVMBasicType       typeCode;
	    CVMClassBlock*     arrCb;

	    arrLen  = STACK_INT(-1);
	    if (arrLen < 0) {
		goto negative_array_size_exception;
	    }
	    typeCode = pc[1];
	    arrCb = (CVMClassBlock*)CVMbasicTypeArrayClassblocks[typeCode];
	    CVMassert(arrCb != 0);

	    /*
	     * CVMgcAllocNewArray may block and initiate GC, so we must
	     * de-cache our state for GC first.
	     */
	    DECACHE_PC();
	    DECACHE_TOS();
	    directArr = (CVMArrayOfAnyType*)
		CVMgcAllocNewArray(ee, typeCode, arrCb, arrLen);

	    TRACE(("\tnewarray %C.%d => 0x%x\n", arrCb, arrLen, directArr));

	    /*
	     * Make sure we created the right beast
	     */
	    CVMassert(directArr == NULL || 
		      CVMisArrayClassOfBasicType(CVMobjectGetClass(directArr),
						 CVMbasicTypeID[typeCode]));
	    CVMID_icellSetDirect(ee, &STACK_ICELL(-1), (CVMObject*)directArr);

	    goto opc_newarray_overflow;
	}

	/* A few of the non-quick opcodes */

#ifdef CVM_CLASSLOADING
	CASE(opc_anewarray, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;
#else 
	CASE(opc_anewarray, 0)
	    goto unimplemented_opcode;
#endif
	/*
	 * We have enough space after opc_anewarray for the opc_newarray
	 * overflow, so stick the overflow code here.
	 */
	opc_newarray_overflow:
	    if (directArr == NULL) {
		goto out_of_memory_error_for_newarray;
	    }
	    UPDATE_PC_AND_TOS_AND_CONTINUE_2_0;

	/* Get the length of an array */

        CASE(opc_arraylength, 1)
	{
	    CVMArrayOfAnyType* arrObj = (CVMArrayOfAnyType*)STACK_OBJECT(-1);
	    CHECK_NULL(arrObj);
	    STACK_INT(-1) = CVMD_arrayGetLength(arrObj);
	    TRACE(("\t%s %O.length(%d) ==>\n", CVMopnames[pc[0]],
		   arrObj, STACK_INT(-1)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}

	/* Throw an exception. */

	CASE(opc_athrow, 1) {
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CHECK_NULL(directObj);
	    TRACE(("\tathrow => %O\n", directObj));
	    DECACHE_PC();  /* required for handle_exception */
	    CVMgcUnsafeThrowLocalException(ee, directObj);
	    goto handle_exception;
	}

	/* A few of the non-quick opcodes */

#ifdef CVM_CLASSLOADING
	CASE(opc_checkcast, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;
	CASE(opc_instanceof, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;
#else 
	CASE(opc_checkcast, 0)
	    goto unimplemented_opcode;
	CASE(opc_instanceof, 0)
	    goto unimplemented_opcode;
#endif

	/* monitorenter and monitorexit for locking/unlocking an object */

       CASE(opc_monitorenter, 1) {
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CHECK_NULL(directObj);
	    TRACE(("\topc_monitorenter(%O)\n", directObj));
	    /* %comment l002 */
	    if (CVMfastTryLock(ee, directObj)) {
		UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	    }
	    /* non-blocking attempt failed... */
	    goto opc_monitorenter_overflow; /* try it the hard way */
	}

        CASE(opc_monitorexit, 1) {
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CHECK_NULL(directObj);
	    TRACE(("\topc_monitorexit(%O)\n", directObj));
	    /* %comment l003 */
	    if (CVMfastTryUnlock(ee, directObj)){
		UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	    }
	    /* non-blocking attempt failed... */
	    goto opc_monitorexit_overflow; /* try it the hard way */
	}


	/* Wide opcodes */

        CASE(opc_wide, 0)
	    goto opc_wide_overflow;

	/* A few of the non-quick opcodes */

#ifdef CVM_CLASSLOADING
	CASE(opc_multianewarray, 0)
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;
#else 
	CASE(opc_multianewarray, 0)
	    goto unimplemented_opcode;
#endif

	/* ifnull and ifnonnull */

#define NULL_COMPARISON_OP(name, not)				\
	CASE(opc_if##name, 0) {					\
            TRACE(("\t%s goto +%d (%staken)\n",			\
		   CVMopnames[pc[0]],				\
		   CVMgetInt16(pc + 1),				\
		   (not(STACK_OBJECT(-1) == 0))			\
		   ? "" : "not "));				\
            JVMPI_TRACE_IF((not(STACK_OBJECT(-1) == 0)));	\
            if (not(STACK_OBJECT(-1) == 0)) {			\
	        goto opc_if_overflow;				\
	    }							\
	    FETCH_NEXT_OPCODE(3);				\
            UPDATE_PC_AND_TOS_AND_CONTINUE(3, -1);		\
        }

	NULL_COMPARISON_OP(null, !!);
	NULL_COMPARISON_OP(nonnull, !);

	/* goto_w and jsr_w. They are exactly the same except jsr_w pushes
	 * the address of the next instruction first.
	 */

        CASE(opc_goto_w, 5)
	    goto opc_goto_w_overflow;

        CASE(opc_jsr_w, 5)
	    goto opc_jsr_w_overflow;

	/* debugger breakpoint */

        CASE(opc_breakpoint, 1) {
	    TRACE(("\tbreakpoint\n"));
#ifdef CVM_JVMTI
	    DECACHE_TOS();
	    DECACHE_PC();
	    CVMD_gcSafeExec(ee, {
		opcode = CVMjvmtiGetBreakpointOpcode(ee, pc, CVM_TRUE);
	    });
            REEXECUTE_BYTECODE();
#else
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);    /* treat as opc_nop */
#endif
	}

	/* Load constant from constant pool: */

	CASE(opc_aldc_ind_quick, 2) { /* Indirect String (loaded classes) */
	    CVMObjectICell* strICell = CVMcpGetStringICell(cp, pc[1]);
	    CVMID_icellAssignDirect(ee, &STACK_ICELL(0), strICell);
            TRACE(("\t%s #%d => 0x%x\n", CVMopnames[pc[0]],
		   pc[1], STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 1);
	}

	CASE(opc_aldc_ind_w_quick, 3) { /* Indirect String (loaded classes) */
	    CVMObjectICell* strICell =
		CVMcpGetStringICell(cp, GET_INDEX(pc + 1));
	    CVMID_icellAssignDirect(ee, &STACK_ICELL(0), strICell);
            TRACE(("\t%s #%d => 0x%x\n", CVMopnames[pc[0]],
		   pc[1], STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
	}

#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
        CASE(opc_aldc_quick, 2) /* loads direct String refs (ROM only) */
	    STACK_INFO(0) = CVMcpGetVal32(cp, pc[1]);
            TRACE(("\t%s #%d => 0x%x\n", CVMopnames[pc[0]],
		   pc[1], STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 1);
#else
        CASE(opc_aldc_quick, 0) /* loads direct String refs (ROM only) */
	    /* Fall through... */
#endif

        CASE(opc_ldc_quick, 2)
	    STACK_INFO(0) = CVMcpGetVal32(cp, pc[1]);
            TRACE(("\t%s #%d => 0x%x\n", CVMopnames[pc[0]],
		   pc[1], STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 1);

#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
        CASE(opc_aldc_w_quick, 3) /* load a direct String refs (ROM only) */
            STACK_INFO(0) = CVMcpGetVal32(cp, GET_INDEX(pc + 1));
            TRACE(("\t%s #%d => 0x%x\n", CVMopnames[pc[0]],
		   GET_INDEX(pc + 1), STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
#else
        CASE(opc_aldc_w_quick, 0) /* load a direct String refs (ROM only) */
	    /* Fall through... */
#endif

        CASE(opc_ldc_w_quick, 3)
            STACK_INFO(0) = CVMcpGetVal32(cp, GET_INDEX(pc + 1));
            TRACE(("\t%s #%d => 0x%x\n", CVMopnames[pc[0]],
		   GET_INDEX(pc + 1), STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);

        CASE(opc_ldc2_w_quick, 3)
 	{
	    CVMJavaVal64 r;
	    CVMcpGetVal64(cp, GET_INDEX(pc + 1), r);
	    CVMmemCopy64(&STACK_INFO(0).raw, r.v); 
            TRACE(("\topc_ldc2_w_quick #%d => long=%s double=%g\n", 
		   GET_INDEX(pc + 1),
                   GET_LONGCSTRING(STACK_LONG(0)), STACK_DOUBLE(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 2);
	}

	/* invoke a static method */

	CASE(opc_invokestatic_quick, 0) {
	    CVMClassBlock* cb;
	    mb = CVMcpGetMb(cp, GET_INDEX(pc + 1));
	    cb = CVMmbClassBlock(mb);
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    receiverObjICell = CVMcbJavaInstance(cb);
	    goto callmethod_static;
	}
 
	/* invoke a romized static method that has a static initializer */

	CASE(opc_invokestatic_checkinit_quick, 0)
	    goto opc_invokestatic_checkinit_quick_overflow;

	/* invoke a virtual method */

	CASE(opc_invokevirtual_quick, 0)
#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
	    goto invokevirtual_quick;
#endif
	CASE(opc_ainvokevirtual_quick, 0)
#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
	    goto invokevirtual_quick;
#endif
	CASE(opc_dinvokevirtual_quick, 0)
#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
	    goto invokevirtual_quick;
#endif
	CASE(opc_vinvokevirtual_quick, 0)
        {
	    CVMObject* directObj;
#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
	invokevirtual_quick:
#endif
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    topOfStack -= pc[2];
	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    mb = CVMcbMethodTableSlot(CVMobjectGetClass(directObj), pc[1]);
	    goto callmethod;
	}

	CASE(opc_invokevirtual_quick_w, 0)
	    goto opc_invokevirtual_quick_w_overflow;
  
	/* invoke a virtual method of java/lang/Object */
	/* 
	 * NOTE: we can get rid of opc_invokevirtualobject_quick now because
	 * arrays have their own methodTablePtr (they just use Object's
	 * methodTablePtr), and CVMobjMethodTableSlot() has been optimized
	 * to no longer check if it's dealing with an array type, so we
	 * could just as well use opc_invokevirtual_quick.
	 */

	CASE(opc_invokevirtualobject_quick, 0) {
	    CVMObject* directObj;
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    topOfStack -= pc[2];
	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    mb = CVMobjMethodTableSlot(directObj, pc[1]);
	    goto callmethod;
	}

	/* invoke a nonvirtual method */

	CASE(opc_invokenonvirtual_quick, 0) {
	    CVMObject* directObj;
	    mb = CVMcpGetMb(cp, GET_INDEX(pc + 1));
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    topOfStack -= CVMmbArgsSize(mb);
	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    goto callmethod;
	}

	/* invoke a superclass method */

	CASE(opc_invokesuper_quick, 0)
	    goto opc_invokesuper_quick_overflow;

	/* ignore invocation. This is a result of inlining a method
	 * that does nothing.
	 */
#define CVM_INLINING
#ifdef CVM_INLINING
        CASE(opc_invokeignored_quick, 3) {
	    CVMObject* directObj;
	    TRACE(("\tinvokeignored_quick\n"));
	    topOfStack -= pc[1];
	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    if (pc[2]) {
		CHECK_NULL(directObj);
	    }
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 0);
	}
#else
	CASE(opc_invokeignored_quick, 0)
	    goto unimplemented_opcode;
#endif

	/* invoke an interface method */

        CASE(opc_invokeinterface_quick, 0)
	    goto opc_invokeinterface_quick_overflow;

	/* Check if an object is an instance of given type. Throw an
	 * exception if not. */
        CASE(opc_checkcast_quick, 3)
	{
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CVMClassBlock* castCb = 
		CVMcpGetCb(cp, GET_INDEX(pc+1));   /* cb of cast type */ 
	    TRACE(("\t%s %C\n", CVMopnames[pc[0]], castCb));
	    if (!CVMgcUnsafeIsInstanceOf(ee, directObj, castCb)) {
		goto class_cast_exception;
	    }
	    UPDATE_PC_AND_TOS_AND_CONTINUE_3_0;
	}

	/* Check if an object is an instance of given type and store
	 * result on the stack. */
        CASE(opc_instanceof_quick, 3)
	{
	    CVMObject* directObj = STACK_OBJECT(-1);
	    STACK_INT(-1) = (directObj != NULL) &&
		CVMgcUnsafeIsInstanceOf(ee, directObj,
					CVMcpGetCb(cp, GET_INDEX(pc+1))); 
	    /* CVMgcUnsafeIsInstanceOf() may have thrown StackOverflowError */
	    if (CVMlocalExceptionOccurred(ee)) {
		DECACHE_PC();
		goto handle_exception;
	    }
	    TRACE(("\t%s %C => %d\n",
		   CVMopnames[pc[0]], CVMcpGetCb(cp, GET_INDEX(pc+1)),
		   STACK_INT(-1)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE_3_0;
 	}

	/* pop stack, and error if it is null */

	CASE(opc_nonnull_quick, 1)
	    TRACE(("\tnonnull_quick\n"));
	    CHECK_NULL(STACK_OBJECT(-1));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

	/* Exit a transition frame. Like opc_return, but with
	 * special semantics.
	 */

	CASE(opc_exittransition, 0) {
	    TRACE(("\texittransition\n"));
	    if (frame == initialframe) {
		/* We get here when we've reached the transition frame that
		 * was used to enter CVMgcUnsafeExecuteJavaMethod. All we
		 * to do is exit. The caller is reponsible for cleaning
		 * up the transiton frame.
		 */
		goto finish;
	    } else {
		goto opc_exittransition_overflow;
	    }
	}

#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
	CASE(opc_agetstatic_quick, 3) {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    JVMTI_WATCHING_FIELD_ACCESS(NULL)
	    STACK_INFO(0) = CVMfbStaticField(ee, fb);
	    TRACE(("\t%s #%d %C.%F[%d] (0x%X) ==>\n",
		   CVMopnames[pc[0]], GET_INDEX(pc+1),
		   CVMfbClassBlock(fb), fb, 
		   CVMfbOffset(fb), STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
	}
#else
	CASE(opc_agetstatic_quick, 0)
#endif
	CASE(opc_getstatic_quick, 3) {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    JVMTI_WATCHING_FIELD_ACCESS(NULL)
	    STACK_INFO(0) = CVMfbStaticField(ee, fb);
	    TRACE(("\t%s #%d %C.%F[%d] (0x%X) ==>\n",
		   CVMopnames[pc[0]], GET_INDEX(pc+1),
		   CVMfbClassBlock(fb), fb, 
		   CVMfbOffset(fb), STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
	}

	CASE(opc_getstatic2_quick, 3) {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    JVMTI_WATCHING_FIELD_ACCESS(NULL)
	    CVMmemCopy64(&STACK_INFO(0).raw, &CVMfbStaticField(ee, fb).raw);
	    TRACE(("\t%s #%d %C.%F[%d] ((0x%X,0x%X) ==>\n",
		   CVMopnames[pc[0]], GET_INDEX(pc+1), CVMfbClassBlock(fb), fb,
		   CVMfbOffset(fb), STACK_INT(0), STACK_INT(1)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 2);
	}

#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
	CASE(opc_aputstatic_quick, 3) {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    JVMTI_WATCHING_FIELD_MODIFICATION(NULL, CVM_FALSE, CVMfbIsRef(fb));
	    CVMfbStaticField(ee, fb) = STACK_INFO(-1);
	    TRACE(("\t%s #%d (0x%X) ==> %C.%F[%d]\n",
		   CVMopnames[pc[0]], GET_INDEX(pc+1), STACK_INT(-1), 
		   CVMfbClassBlock(fb), fb, CVMfbOffset(fb)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, -1);
	}
#else
	CASE(opc_aputstatic_quick, 0)
#endif
	CASE(opc_putstatic_quick, 3) {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    JVMTI_WATCHING_FIELD_MODIFICATION(NULL, CVM_FALSE, CVMfbIsRef(fb));
	    CVMfbStaticField(ee, fb) = STACK_INFO(-1);
	    TRACE(("\t%s #%d (0x%X) ==> %C.%F[%d]\n",
		   CVMopnames[pc[0]], GET_INDEX(pc+1), STACK_INT(-1), 
		   CVMfbClassBlock(fb), fb, CVMfbOffset(fb)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, -1);
	}

	CASE(opc_putstatic2_quick, 3) {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    JVMTI_WATCHING_FIELD_MODIFICATION(NULL, CVM_TRUE, CVM_FALSE);
	    CVMmemCopy64(&CVMfbStaticField(ee, fb).raw, &STACK_INFO(-2).raw);
	    TRACE(("\t%s #%d (0x%X,0x%X) ==> %C.%F[%d]\n",
		   CVMopnames[pc[0]], GET_INDEX(pc+1), 
		   STACK_INT(-2), STACK_INT(-1),
		   CVMfbClassBlock(fb), fb, CVMfbOffset(fb)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, -2);
	}

	/* 
	 * Quick static field access for romized classes with initializers.
	 */

	CASE(opc_agetstatic_checkinit_quick, 3)
	    goto opc_getstatic_checkinit_quick_overflow;

	CASE(opc_getstatic_checkinit_quick, 3)
	    goto opc_getstatic_checkinit_quick_overflow;

	CASE(opc_getstatic2_checkinit_quick, 3)
	    goto opc_getstatic2_checkinit_quick_overflow;

	CASE(opc_aputstatic_checkinit_quick, 3)
	    goto opc_putstatic_checkinit_quick_overflow;

	CASE(opc_putstatic_checkinit_quick, 3) 
	    goto opc_putstatic_checkinit_quick_overflow;

	CASE(opc_putstatic2_checkinit_quick, 3)
	    goto opc_putstatic2_checkinit_quick_overflow;

	/* Quick object field access byte-codes. The non-wide version are
	 * all lossy and are not needed when in losslessmode.
	 */

#ifdef CVM_NO_LOSSY_OPCODES
        CASE(opc_getfield_quick, 0)
	    goto unimplemented_opcode;
        CASE(opc_putfield_quick, 0)
	    goto unimplemented_opcode;
        CASE(opc_getfield2_quick, 0)
	    goto unimplemented_opcode;
        CASE(opc_putfield2_quick, 0)
	    goto unimplemented_opcode;
	CASE(opc_agetfield_quick, 0)
	    goto unimplemented_opcode;
	CASE(opc_aputfield_quick, 0)
	    goto unimplemented_opcode;
#else

        CASE(opc_getfield_quick, 3)
	{
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CVMUint32  slotIndex = pc[1]; 
	    CHECK_NULL(directObj);
	    pc += 3;
	    CVMD_fieldRead32(directObj, slotIndex, STACK_INFO(-1));
	    TRACE(("\t%s %O[%d](0x%X) ==>\n", CVMopnames[pc[0]],
		   directObj, slotIndex, STACK_INT(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(0, 0);
        }

        CASE(opc_putfield_quick, 3)
	{
	    CVMObject* directObj = STACK_OBJECT(-2);
	    CVMUint32  slotIndex = pc[1]; 
	    CHECK_NULL(directObj);
	    CVMD_fieldWrite32(directObj, slotIndex, STACK_INFO(-1));
	    TRACE(("\t%s (0x%X) ==> %O[%d]\n", CVMopnames[pc[0]],
		   STACK_INT(-1), directObj, slotIndex));
            UPDATE_PC_AND_TOS_AND_CONTINUE(3, -2);
        }

        CASE(opc_getfield2_quick, 3)
	{
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CHECK_NULL(directObj);
	    CVMD_fieldRead64(directObj, pc[1], &STACK_INFO(-1));
	    TRACE(("\t%s %O[%d](0x%X, 0x%X) ==>\n", CVMopnames[pc[0]],
		   directObj, pc[1], STACK_INT(-1), STACK_INT(0)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
        }

        CASE(opc_putfield2_quick, 3)
	{
	    CVMObject* directObj = STACK_OBJECT(-3);
	    CHECK_NULL(directObj);
	    CVMD_fieldWrite64(directObj, pc[1], &STACK_INFO(-2));
	    TRACE(("\t%s (0x%X, 0x%X) ==> %O[%d]\n", CVMopnames[pc[0]],
		   STACK_INT(-1), STACK_INT(0), directObj, pc[1]));
            UPDATE_PC_AND_TOS_AND_CONTINUE(3, -3);
        }

	CASE(opc_agetfield_quick, 3)
	{
	    CVMObject* fieldObj;
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CVMUint32  slotIndex = pc[1]; 
	    CHECK_NULL(directObj);
	    pc += 3;
	    CVMD_fieldReadRef(directObj, slotIndex, fieldObj);
	    CVMID_icellSetDirect(ee, &STACK_ICELL(-1), fieldObj); 
	    TRACE(("\t%s %O[%d](0x%x) ==>\n", CVMopnames[pc[0]],
		   directObj, slotIndex, fieldObj));
            UPDATE_PC_AND_TOS_AND_CONTINUE(0, 0);
        }

	CASE(opc_aputfield_quick, 3)
	    goto opc_aputfield_quick_overflow;
#endif /* CVM_NO_LOSSY_OPCODES */

        CASE(opc_getfield_quick_w, 3)
	    goto opc_getfield_quick_w_overflow;

        CASE(opc_putfield_quick_w, 3)
	    goto opc_putfield_quick_w_overflow;

	/* Allocate memory for a new java object. */

	CASE(opc_new_checkinit_quick,1) {
	    newCb = CVMcpGetCb(cp, GET_INDEX(pc + 1));
	    goto opc_new_checkinit_quick_overflow;
	}

	CASE(opc_new_quick, 3) {
	    newCb = CVMcpGetCb(cp, GET_INDEX(pc + 1));
	    goto opc_new_quick_overflow;
	}

        CASE(opc_anewarray_quick, 3)
	    goto opc_anewarray_quick_overflow;

        CASE(opc_multianewarray_quick, 4)
	    goto opc_multianewarray_quick_overflow;

	DEFAULT
	unimplemented_opcode:
	    CVMdebugPrintf(("\t*** Unimplemented opcode: %d = %s\n",
		   pc[0], CVMopnames[pc[0]]));
	    goto finish;

	} /* switch(opcode) */

#ifdef CVM_ADD_4K_PAD
	/*
	 * WARNING: This code assumes 16K cache and also assumes that
	 * the cache address any address maps to is simply its address
	 * & 0x3FFF. Otherwise this code may not help performance and 
	 * will waste space. It also assumes the the amount of code
	 * in this function before the first opcode is less than about
	 * 300 bytes.
	 *
	 * It turns out that the first 64 opcodes (4K of code) are the
	 * ones being execute about 45% of the time. For this reason, we
	 * want as little other code as possible overlapping them in the
	 * L1 cache memory layout. Our main loop is just under 16K, about 250
	 * opcodes of size 64 each. The code just before that is only about
	 * 100 bytes. The first .align 4096 will move the pc forward a couple
	 * hundred bytes to the 16k boundary. The 2nd align 4096 skips 
	 * another 4K, meaning that no other code in this function will
	 * overlap the first 64 opcodes, which gives us better performance
	 */
    post_loop_alignment:
	CVM_ALIGN_4K;
	receiverObjICell = NULL; /* get us off the 4K boundary */
	CVM_ALIGN_4K;
#endif

    callmethod_static:
	ASMLABEL(label_callmethod_static);
	topOfStack -= CVMmbArgsSize(mb);

    callmethod:
	ASMLABEL(label_callmethod);
	{
	    /* callmethod handles the method invoking for all invoke
	     * opcodes. Upon entry, the following locals must be setup:
	     *    mb:            the method to invoke
	     *    pc:            points to the invoke opcode
	     *    topOfStack:    points before the method arguments
	     *    receiverObjICell: the object we are dispatching on or
	     *			 the Class object for static calls
	     */
	    CVMFrame* prev;
	    int invokerIdx;

            TRACE(("\t%s %C.%M\n",
		   CVMopnames[pc[0]], CVMmbClassBlock(mb), mb));

	    TRACESTATUS();

	    /* Decache the interpreter state before pushing the frame.
	     * We leave frame->topOfStack pointing after the args,
	     * so they will get scanned it gc starts up during the pushFrame.
	     */
	    DECACHE_PC();

#ifdef CVM_JIT
    new_mb0:
#endif
	    prev = frame;

    new_mb:
	    ASMLABEL(label_new_mb);
#ifdef CVM_JVMPI
            JVMPI_CHECK_FOR_DATA_DUMP_REQUEST(ee);
#endif
	    invokerIdx = CVMmbInvokerIdx(mb);
	    if (invokerIdx < CVM_INVOKE_CNI_METHOD) {
		/*
		 * It's a Java method. Push the frame and setup our state 
		 * for the new method.
		 */
		CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
#ifdef CVM_JIT
		if (CVMmbIsCompiled(mb)) {
		    goto invoke_compiled;
		} else {
		    CVMInt32 cost = CVMmbInvokeCost(mb);
		    cost -= CVMglobals.jit.interpreterTransitionCost;
		    if (cost <= 0) {
			CVMD_gcSafeExec(ee, {
			    CVMmbInvokeCostSet(mb, 0);
			    CVMJITcompileMethod(ee, mb);
			});
			if (CVMmbIsCompiled(mb)) {
			    goto invoke_compiled;
			}
		    } else {
			CVMassert(cost <= CVMJIT_MAX_INVOKE_COST);
			CVMmbInvokeCostSet(mb, cost);
		    }
		}
#endif

		/* update the new locals value so our pushframe logic
		 * is easier. */
		locals = &topOfStack->s;

		/*
		 * pushFrame() might have to become GC-safe if it needs to 
		 * expand the stack. frame->topOfStack for the current frame 
		 * includes the arguments, and the stackmap does as well, so
		 * that the GC-safe action is really safe.
		 */
		CVMpushFrame(ee, stack, frame, topOfStack,
			     CVMjmdCapacity(jmd), CVMjmdMaxLocals(jmd),
			     CVM_FRAMETYPE_JAVA, mb, CVM_TRUE,
		/* action if pushFrame fails */
		{
		    CACHE_FRAME();  /* pushframe set it to NULL */
		    locals = CVMframeLocals(frame);
		    goto handle_exception; /* exception already thrown */
		},
		/* action to take if stack expansion occurred */
		{
		    TRACE(("pushing JavaFrame caused stack expansion\n"));
		    /* Since the stack expanded, we need to set locals
		     * to the new frame minus the number of locals.
		     * Also, we need to copy the arguments, which
		     * topOfStack points to, to the new locals area.
		     */
		    locals = (CVMSlotVal32*)frame - CVMjmdMaxLocals(jmd);
		    memcpy((void*)locals, (void*)topOfStack,
			       CVMmbArgsSize(mb) * sizeof(CVMSlotVal32));
		});

		/*
		 * GC-safety stage 2: Update caller frame's topOfStack to
		 * exclude the arguments. Once we update our new frame
		 * state, the callee frame's stackmap will cover the arguments
		 * as incoming locals.  
		 */
		prev->topOfStack = topOfStack;

		/* set topOfStack to start of new operand stack */
		CVM_RESET_JAVA_TOS(topOfStack, frame);

		/* We have to cache frame->locals if this method ever calls
		 * calls another method (which we don't know), and in most
		 * cases it's cheaper just to cache it here rather then cache
		 * it every time we make a method call from this method. The
		 * same is true of frame->mb and frame->constantpool.
		 */
		CVMframeLocals(frame) = locals;
		pc = CVMjmdCode(jmd);
		cp = CVMcbConstantPool(CVMmbClassBlock(mb));
		CVMframeCp(frame) = cp;
		
		/* This is needed to get CVMframe2string() to work properly */
		CVMdebugExec(DECACHE_PC());

		TRACE_METHOD_CALL(frame, CVM_FALSE);
		TRACESTATUS();

		CVMID_icellAssignDirect(ee, 
					&CVMframeReceiverObj(frame, Java),
					receiverObjICell);
		/* old location no longer scanned, so update to frame */
		receiverObjICell = &CVMframeReceiverObj(frame, Java);
		
		/* %comment c002 */
		if (CVMmbIs(mb, SYNCHRONIZED)) {
                    /* The method is sync, so lock the object. */
                    /* %comment l002 */
                    if (!CVMfastTryLock(
			    ee, CVMID_icellDirect(ee, receiverObjICell))) {
                        DECACHE_PC();
                        DECACHE_TOS();
                        if (!CVMobjectLock(ee, receiverObjICell)) {
			    CVMpopFrame(stack, frame); /* pop the java frame */
			    CVMthrowOutOfMemoryError(ee, NULL);
			    goto handle_exception;
			}
                    }
		}

#ifdef CVM_JVMPI
                if (CVMjvmpiEventMethodEntryIsEnabled()) {
                    /* Need to decache because JVMPI will become GC safe
                     * when calling the profiler agent: */
                    DECACHE_PC();
                    DECACHE_TOS();
                    CVMjvmpiPostMethodEntryEvent(ee, receiverObjICell);
                }
#endif /* CVM_JVMPI */

#ifdef CVM_JVMTI
		/* %comment kbr001 */
		/* Decache all curently uncached interpreter state */
		if (CVMjvmtiThreadEventsEnabled(ee)) {
		    DECACHE_PC();
		    DECACHE_TOS();
		    CVMD_gcSafeExec(ee, {
			CVMjvmtiPostFramePushEvent(ee);
		    });
		}
#endif

		/*
		 * GC-safety stage 3: Now that we have "committed" the new 
		 * frame state, we can request a GC-safe point. At this point,
		 * the topOfStack of the caller frame has been decremented,
		 * and it is the callee that will be responsible for the 
		 * arguments
		 *
		 * Decache the new interpreter state if there's a gc request.
		 */
		CVMD_gcSafeCheckPoint(ee, {
		    DECACHE_PC();
		    DECACHE_TOS();
		}, {});
	    } else if (invokerIdx < CVM_INVOKE_JNI_METHOD) {

		/*
		 * It's an CNI method. See c07/13/99 in interpreter.h for
		 * details on how CNI methods work.
		 */
		CNIResultCode ret;
#ifdef CVM_TRACE
		CVMMethodBlock *mb0 = mb;
#endif
		CNINativeMethod *f = (CNINativeMethod *)CVMmbNativeCode(mb);

		TRACE_FRAMELESS_METHOD_CALL(frame, mb0, CVM_FALSE);

		ret = (*f)(ee, topOfStack, &mb);

		TRACE_FRAMELESS_METHOD_RETURN(mb0, frame);

		/* 
		 * Look at the result code to see what the CNI method did.
		 */
		if ((int)ret >= 0) {
		    /* This is a normal return from an CNI method. The only
		     * positive result codes are CNI_VOID, CNI_SINGLE, and
		     * CNI_DOUBLE, all of which represent the actual size
		     * in words of the result. Bump up the pc to point
		     * beyond the invoke opcode and increment the TOS by
		     * the size of the method result.
		     */
		    topOfStack += (int)ret;
		    pc += (*pc == opc_invokeinterface_quick ? 5 : 3);
		    CONTINUE;
		} else if (ret == CNI_NEW_TRANSITION_FRAME) {
		    /* The CNI method pushed a transition frame. This is
		     * used by CVM.invoke(). The transition frame holds
		     * the arguments for the method to be invoked. We
		     * need to cache the new frame and then act like we just
		     * entered CVMgcUnsafeExecuteJavaMethod (almost).
		     */
		    DECACHE_TOS(); /* pop invoker's arguments. Must be done
				    * before CACHE_FRAME() */
		    CACHE_FRAME(); /* get the transition frame. */

		    /* Set the flag that says opc_exittransition should
		     * increment the pc.
		     */
		    CVMframeIncrementPcFlag(frame) = CVM_TRUE;

		    isStatic = CVMmbIs(mb, STATIC);
		    isVirtual = CVM_FALSE;

		    /* no gcsafe checkpoint is needed since new_transition
		     * will eventually lead to one.
		     */
		    goto new_transition;
#if 0 /* currently no one uses this feature.*/
		} else if (ret == CNI_NEW_JAVA_FRAME) {
		    /* No one uses this yet. It's used if you want to do 
		     * push a JavaFrame and have exectuion resume at the
		     * pc you stored in the JavaFrame. 
		     */
		    CACHE_FRAME();
		    CACHE_PC();
		    CACHE_TOS();
		    locals = CVMframeLocals(frame);
		    cp = CVMframeCp(frame);

		    /* needed to get CVMframe2string() to work properly */
		    CVMdebugExec(DECACHE_PC());
		    TRACE_METHOD_CALL(frame, CVM_TRUE);

		    CVMD_gcSafeCheckPoint(ee, {
			DECACHE_PC();    /* only needed if we gc */
			DECACHE_TOS();   /* only needed if we gc */
		    }, {});
#endif
		} else if (ret == CNI_NEW_MB) {
		    /* This is used by CVM.executeClinit. It's eliminates
		     * C recursion by simply returning the mb of the <clinit>
		     * method that needs to be called. topOfStack currently
		     * points before the arguments, which is what we want.
		     * However, we need to update frame->topOfStack to point
		     * after the arguments, since the number of arguments
		     * may have changed.
		     */
		    DECACHE_TOS();
		    frame->topOfStack += CVMmbArgsSize(mb);

		    /* no gcsafe checkpoint is needed since new_mb
		     * will eventually lead to one.
		     */
		    goto new_mb;
		} else if (ret == CNI_EXCEPTION) {
		    /* This is for when an CNI method throws an exception. */
		    goto handle_exception;
		} else {
		    CVMassert(CVM_FALSE);
		}

	    } else if (invokerIdx < CVM_INVOKE_ABSTRACT_METHOD) {
		/*
		 * It's a JNI method.
		 */

		/*
		 * We've already done a DECACHE_TOS and DECACHE_PC.
		 */
		if (!CVMinvokeJNIHelper(ee, mb)) {
		    goto handle_exception;
		}
		CACHE_TOS();

		/* Bump up the pc to point beyond the invoke opcode in
		 * the transition frame.
		 */
		pc += (*pc == opc_invokeinterface_quick ? 5 : 3);

		CONTINUE;
	    } else if (invokerIdx == CVM_INVOKE_ABSTRACT_METHOD) {
		/*
		 * It's an abstract method.
		 */
		CVMthrowAbstractMethodError(ee, "%C.%M", 
					    CVMmbClassBlock(mb), mb);
		goto handle_exception;
#ifdef CVM_CLASSLOADING
	    } else if (invokerIdx == CVM_INVOKE_NONPUBLIC_MIRANDA_METHOD) {
		/*
		 * It's a miranda method created to deal with a non-public
		 * method with the same name as an interface method.
		 */
		CVMthrowIllegalAccessError(
                    ee, "access non-public method %C.%M through an interface",
		    CVMmbClassBlock(mb), CVMmbMirandaInterfaceMb(mb));
		goto handle_exception;
	    } else if (invokerIdx == 
		       CVM_INVOKE_MISSINGINTERFACE_MIRANDA_METHOD) {
		/*
		 * It's a miranda method created to deal with a missing
		 * interface method.
		 */
		CVMthrowAbstractMethodError(ee, "%C.%M",
					    CVMmbClassBlock(mb), mb);
		goto handle_exception;
	    } else if (invokerIdx == CVM_INVOKE_LAZY_JNI_METHOD) {
		/*
		 * It's a native method of a dynamically loaded class.
		 * We still need to lookup the native code.
		 */
		CVMBool result;
		CVMD_gcSafeExec(ee, {
		    result = CVMlookupNativeMethodCode(ee, mb);
		});
		if (!result) {
		    goto handle_exception;
		} else {
		    /*
		     * CVMlookupNativeMethod() stored the pointer to the
		     * native method and also changed the invoker index, so
		     * just branch to new_mb and the correct native
		     * invoker will be used.
		     */
		    goto new_mb;
		}
#endif
	    } else {
		CVMdebugPrintf(("Unkown method invoker: %d\n", invokerIdx));
		CVMassert(CVM_FALSE);
	    }
	};  /* callmethod: */
	CONTINUE;
	
    handle_return: 
	ASMLABEL(label_handle_return);
	{
#undef RETURN_OPCODE
#if (defined(CVM_JVMTI) || defined(CVM_JVMPI))
#define RETURN_OPCODE return_opcode
	    CVMUint32 return_opcode;
	    /* We might gc, so flush state. */
	    DECACHE_PC();
	    CVM_RESET_JAVA_TOS(frame->topOfStack, frame);
	    return_opcode = CVMregisterReturnEvent(ee, pc, &STACK_ICELL(-1));
#else
#define RETURN_OPCODE pc[0]
#endif

	    /* %comment c003 */
	    /* Unlock the receiverObj if this was a sync method call. */
	    if (CVMmbIs(frame->mb, SYNCHRONIZED)) {
                /* %comment l003 */
                CVMObjectICell *objICell = &CVMframeReceiverObj(frame, Java);
                if (!CVMfastTryUnlock(ee, CVMID_icellDirect(ee, objICell))) {
		    ASMLABEL(label_handleUnlockFailureInReturn);
		    /* We might gc, so flush state. */
		    CVM_RESET_JAVA_TOS(frame->topOfStack, frame);
		    DECACHE_PC();
		    if (!CVMsyncReturnHelper(ee, frame, &STACK_ICELL(-1),
					     RETURN_OPCODE == opc_areturn))
		    {
                        CACHE_FRAME();
			goto handle_exception;
		    }
		}
	    }

	    TRACESTATUS();
	    TRACE_METHOD_RETURN(frame);
	    CVMpopFrame(stack, frame);

#ifdef CVM_JIT
	    if (CVMframeIsCompiled(frame)) {
		goto return_to_compiled;
	    }
#endif

	    /* 
	     * Restore our state. We need to recache all of the local
	     * variables that got trounced when we made the method call.
	     * topOfStack was restored before handle_return was entered.
	     */
	    CACHE_PC();   /* restore our pc from before the call */

	    /* The following two lines can cause a read from
               uninitialized memory in the case of popping the frame
               just above a transition frame, which doesn't have
               locals or a constant pool. This is harmless, since
               these values won't be used in this case, but in debug
               mode we'll leave in the check to avoid problems when
               using dbx's memory checking. */
#ifdef CVM_DEBUG
	    if (CVMframeIsJava(frame)) {
#endif
		locals = CVMframeLocals(frame);
		cp = CVMframeCp(frame);
#ifdef CVM_DEBUG
	    }
#endif

	    /* Increment caller's pc based on the callers invoke opcode. */
	    pc += (*pc == opc_invokeinterface_quick ? 5 : 3);

	    /* We don't restore mb for performance reasons. The only
	     * quick opcode that needs it is opc_invokesuper_quick and
	     * we'll get better performance if it handles the
	     * frame->mb overhead instead of doing it on every return.
	     */
	    /* mb = frame->mb;*/

	    TRACESTATUS();
	    
	    CONTINUE;
	} /* handle_return: */

#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
    update_pc_and_tos_and_continue_1_0:
	FETCH_NEXT_OPCODE(1);
	UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

    update_pc_and_tos_and_continue_1_m1: {
	FETCH_NEXT_OPCODE(1);
	UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
    }

    update_pc_and_tos_and_continue_1_m2:
	FETCH_NEXT_OPCODE(1);
	UPDATE_PC_AND_TOS_AND_CONTINUE(1, -2);

    update_pc_and_tos_and_continue_1_m3:
	FETCH_NEXT_OPCODE(1);
	UPDATE_PC_AND_TOS_AND_CONTINUE(1, -3);

    update_pc_and_tos_and_continue_1_m4:
	FETCH_NEXT_OPCODE(1);
	UPDATE_PC_AND_TOS_AND_CONTINUE(1, -4);

    update_pc_and_tos_and_continue_2_0:
	FETCH_NEXT_OPCODE(2);
	UPDATE_PC_AND_TOS_AND_CONTINUE(2, 0);

    update_pc_and_tos_and_continue_3_0:
	FETCH_NEXT_OPCODE(3);
	UPDATE_PC_AND_TOS_AND_CONTINUE(3, 0);

    update_pc_and_tos_and_continue_3_1:
	FETCH_NEXT_OPCODE(3);
	UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);

    update_pc_and_tos_and_continue_3_2:
	FETCH_NEXT_OPCODE(3);
	UPDATE_PC_AND_TOS_AND_CONTINUE(3, 2);

    update_pc_and_tos_and_continue_3_m2:
	FETCH_NEXT_OPCODE(3);
	UPDATE_PC_AND_TOS_AND_CONTINUE(3, -2);
#endif

    handle_exception: {
	    ASMLABEL(label_handle_exception);

	    /* When we get here we are guaranteed that all callers have
	     * made sure that the PC has been flushed. However, we must
	     * still reset frame->topOfStack to keep gc happy because
	     * we may not be at a gc point with a stackmap.
	     */
#ifdef CVM_JIT
	    if (CVMframeIsCompiled(frame)) {
		CVM_RESET_COMPILED_TOS(frame->topOfStack, frame);
	    } else
#endif
	    {
		CVM_RESET_INTERPRETER_TOS(frame->topOfStack, frame,
					  CVMframeIsJava(frame));
	    }

    handle_exception_tos_already_reset:	  
	    ASMLABEL(label_handle_exception_tos_already_reset);
	    CVMassert(CVMexceptionOccurred(ee));


	    /* Get the frame that handles the exception. frame->pc will
	     * also get updated.
	     */
	    frame = CVMgcUnsafeHandleException(ee, frame, initialframe);
	    
	    /* If we didn't find a handler for the exception, then just
	     * return to the CVMgcUnsafeExecuteJavaMethod caller and 
	     * let it handle the exception. Do not attempt to bump up
	     * the pc in the transition code and execute the 
	     * exittransition opcode. This will cause gc grief
	     * because it will think it needs to scan the result
	     * if the method returns a ref.
	     */
	    if (frame == initialframe) {
#ifdef CVM_JVMTI
	    if (CVMjvmtiEventsEnabled()) {
		CVMD_gcSafeExec(ee, {
		    CVMjvmtiPostExceptionEvent(ee, pc,
			(CVMlocalExceptionOccurred(ee) ?
			     CVMlocalExceptionICell(ee) :
			     CVMremoteExceptionICell(ee)));
		});
	    }
#endif
		goto finish;
	    }

	    /* The exception was caught. Cache our new state. */
#ifdef CVM_JIT
	    if (CVMframeIsCompiled(frame)) {
#ifdef CVM_JVMTI
		pc = CVMpcmapCompiledPcToJavaPc(frame->mb,
						CVMcompiledFramePC(frame));
		CVMassert(pc != NULL); /* there better be a mapping */
#endif
		/* Set the frame's topOfStack to just past the spill area. */
		CVM_RESET_COMPILED_TOS(topOfStack, frame);
		topOfStack += CVMcmdSpillSize(CVMmbCmd(frame->mb));
	    } else 
#endif
	    {
		CACHE_PC();
		CVM_RESET_JAVA_TOS(topOfStack, frame);
		locals = CVMframeLocals(frame);
		cp = CVMframeCp(frame);
	    }

	    /* Push the exception back on stack per vm spec */
	    CVMID_icellSetDirect(ee, &STACK_ICELL(0), 
				 CVMgetCurrentExceptionObj(ee));
	    CVMsetCurrentExceptionObj(ee, NULL);
	    topOfStack++;

#ifdef CVM_JVMTI
	    if (CVMjvmtiEventsEnabled()) {
                DECACHE_PC();
                DECACHE_TOS();
		CVMD_gcSafeExec(ee, {
		    CVMjvmtiPostExceptionCatchEvent(ee, pc,
						    &STACK_ICELL(-1));
		});
	    }
#endif

#ifdef CVM_JIT
	    if (CVMframeIsCompiled(frame)) {
		goto return_to_compiled_with_exception;
	    }
#endif
	    CONTINUE;   /* continue interpreter loop */
	}  /* handle_exception: */

    init_class:
	/* 
	 * init_class will run static initializers for the specified class.
	 * It does this in a way that won't involve C recursion.
	 */
	ASMLABEL(label_init_class);
	{
            int result;
	    TRACE(("\t%s ==> running static initializers...\n",
		   CVMopnames[pc[0]]));
	    DECACHE_PC();
	    DECACHE_TOS();
	    CVMD_gcSafeExec(ee, {
		result = CVMclassInitNoCRecursion(ee, initCb, &mb);
	    });
	    
	    /* If the result is 0, then the intializers have already been run.
	     * If the result is 1, then mb was setup to run the initializers.
	     * If the result is -1, then an exception was thrown.
	     */
	    if (result == 0) {
		CONTINUE;
	    } else  if (result == 1) {
		CACHE_FRAME();
		isStatic = CVM_FALSE;
		isVirtual = CVM_FALSE;
		goto new_transition;
	    } else {
		goto handle_exception;
	    }
        }

	/*
	 * This is where opcodes that overflow a preset maximum size go.
	 * This allows us to us the gcc opcode alignment feature, and not
	 * have any opcode implementations exceed a certain size.
	 */
    opc_monitorenter_overflow:
	ASMLABEL(opc_monitorenter_overflow);
	/* non-blocking attempt failed... */	    
	DECACHE_PC();
        DECACHE_TOS();
        if (!CVMobjectLock(ee, &STACK_ICELL(-1))) {
	    goto out_of_memory_error;
	}
        UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

    opc_monitorexit_overflow:
	ASMLABEL(opc_monitorexit_overflow);
	/* non-blocking attempt failed... */	    
	DECACHE_PC();
	DECACHE_TOS();
	if (!CVMobjectUnlock(ee, &STACK_ICELL(-1))) {
	    goto illegal_monitor_state_exception_thread_not_owner;
	}
	UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

    opc_daload_overflow:
	ASMLABEL(opc_daload_overflow);
	ARRAY_LOADTO64(Double,double);

    opc_dastore_overflow:
	ASMLABEL(opc_dastore_overflow);
	ARRAY_STOREFROM64(Double);

    opc_getstatic_checkinit_quick_overflow:
	ASMLABEL(opc_getstatic_checkinit_quick_overflow);
	{
	    CVMFieldBlock* fb;
	    CVMClassBlock* cb;
	    fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    cb = CVMfbClassBlock(fb);
	    INIT_CLASS_IF_NEEDED(ee, cb);
	    JVMTI_WATCHING_FIELD_ACCESS(NULL)
	    STACK_INFO(0) = CVMfbStaticField(ee, fb);
	    TRACE(("\t%s #%d %C.%F[%d] (0x%X) ==>\n",
		   CVMopnames[pc[0]], GET_INDEX(pc+1), cb, fb, 
		   CVMfbOffset(fb), STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
	}

   opc_getstatic2_checkinit_quick_overflow:
	ASMLABEL(opc_getstatic2_checkinit_quick_overflow);
       {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    CVMClassBlock* cb = CVMfbClassBlock(fb);
	    INIT_CLASS_IF_NEEDED(ee, cb);
	    JVMTI_WATCHING_FIELD_ACCESS(NULL)
	    CVMmemCopy64(&STACK_INFO(0).raw, &CVMfbStaticField(ee, fb).raw);
	    TRACE(("\t%s #%d %C.%F[%d] ((0x%X,0x%X) ==>\n",
		   CVMopnames[pc[0]], GET_INDEX(pc+1), cb, fb,
		   CVMfbOffset(fb), STACK_INT(0), STACK_INT(1)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 2);
	}

    opc_putstatic_checkinit_quick_overflow:
	ASMLABEL(opc_putstatic_checkinit_quick_overflow);
        {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    CVMClassBlock* cb = CVMfbClassBlock(fb);
	    INIT_CLASS_IF_NEEDED(ee, cb);
	    JVMTI_WATCHING_FIELD_MODIFICATION(NULL, CVM_FALSE, CVMfbIsRef(fb));
	    CVMfbStaticField(ee, fb) = STACK_INFO(-1);
	    TRACE(("\t%s #%d (0x%X) ==> %C.%F[%d]\n",
		   CVMopnames[pc[0]], GET_INDEX(pc+1), STACK_INT(-1), 
		   cb, fb, CVMfbOffset(fb)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, -1);
	}
    
    opc_putstatic2_checkinit_quick_overflow:
	ASMLABEL(opc_putstatic2_checkinit_quick_overflow);
	{
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    CVMClassBlock* cb = CVMfbClassBlock(fb);
	    INIT_CLASS_IF_NEEDED(ee, cb);
	    JVMTI_WATCHING_FIELD_MODIFICATION(NULL, CVM_TRUE, CVM_FALSE);
	    CVMmemCopy64(&CVMfbStaticField(ee, fb).raw, &STACK_INFO(-2).raw);
	    TRACE(("\t%s #%d (0x%X,0x%X) ==> %C.%F[%d]\n",
		   CVMopnames[pc[0]], GET_INDEX(pc+1), 
		   STACK_INT(-2), STACK_INT(-1),
		   cb, fb, CVMfbOffset(fb)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, -2);
	}

    opc_lcmp_overflow:
	ASMLABEL(opc_lcmp_overflow);
	{
 	    CVMJavaLong value1, value2; 
            value1 = CVMjvm2Long(&STACK_INFO(-4).raw);
            value2 = CVMjvm2Long(&STACK_INFO(-2).raw);
	    STACK_INT(-4) = CVMlongCompare(value1, value2);
            TRACE(("\t%s => %d\n", CVMopnames[pc[0]], 
				   STACK_INT(-4)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -3);
	}


    opc_f2l_overflow: /* NOTE: fits on sarm */
	ASMLABEL(opc_f2l_overflow);
	{
	    CVMJavaLong r;
 	    r = CVMfloat2Long(STACK_FLOAT(-1));
	    CVMlong2Jvm(&STACK_INFO(-1).raw, r);
            TRACE(("\tf2l => %s\n", GET_LONGCSTRING(STACK_LONG(-1))));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);
	}

    opc_d2l_overflow: /* NOTE: fits on sarm */
	ASMLABEL(opc_d2l_overflow);
	{
	    CVMJavaDouble r1;
	    CVMJavaLong r2;
	    r1 = CVMjvm2Double(&STACK_INFO(-2).raw);
	    r2 = CVMdouble2Long(r1);
	    CVMlong2Jvm(&STACK_INFO(-2).raw, r2);
            TRACE(("\td2l => %s\n", GET_LONGCSTRING(STACK_LONG(-2))));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}
	
#ifndef CVM_NO_LOSSY_OPCODES
    opc_aputfield_quick_overflow:
	ASMLABEL(opc_aputfield_quick_overflow);
	{
	    CVMObject* directObj = STACK_OBJECT(-2);
	    CHECK_NULL(directObj);
	    CVMD_fieldWriteRef(directObj, pc[1], STACK_OBJECT(-1));
	    TRACE(("\t%s (0x%x) ==> %O[%d]\n", CVMopnames[pc[0]],
		   STACK_OBJECT(-1), directObj, pc[1]));
            UPDATE_PC_AND_TOS_AND_CONTINUE_3_m2;
        }
#endif

    opc_getfield_quick_w_overflow:
	ASMLABEL(opc_getfield_quick_w_overflow);
	{
	    CVMFieldBlock* fb;
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CHECK_NULL(directObj);
	    fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    JVMTI_WATCHING_FIELD_ACCESS(&STACK_ICELL(-1))
	    if (CVMfbIsDoubleWord(fb)) {
		CVMD_fieldRead64(directObj, CVMfbOffset(fb), &STACK_INFO(-1));
		TRACE(("\t%s %O[%d](0x%X, 0x%X) ==>\n", CVMopnames[pc[0]],
		       directObj, GET_INDEX(pc+1), 
		       STACK_INT(-1), STACK_INT(0)));
		UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
	    } else {
		if (CVMfbIsRef(fb)) {
		    CVMD_fieldReadRef(directObj, CVMfbOffset(fb),
				     STACK_OBJECT(-1));
		} else {
		    CVMD_fieldRead32(directObj, CVMfbOffset(fb),
				     STACK_INFO(-1));
		}
		TRACE(("\t%s %O[%d](0x%X) ==>\n", CVMopnames[pc[0]],
		       directObj, GET_INDEX(pc+1), STACK_INT(-1)));
		UPDATE_PC_AND_TOS_AND_CONTINUE(3, 0);
	    }
        }

    opc_putfield_quick_w_overflow:
	ASMLABEL(opc_putfield_quick_w_overflow);
	{
	    CVMObject* directObj;
	    CVMFieldBlock* fb;
	    fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    if (CVMfbIsDoubleWord(fb)) {
		directObj = STACK_OBJECT(-3);
		CHECK_NULL(directObj);
		JVMTI_WATCHING_FIELD_MODIFICATION(&STACK_ICELL(-3),
						  CVM_TRUE, CVM_FALSE);
		CVMD_fieldWrite64(directObj, CVMfbOffset(fb), &STACK_INFO(-2));
		TRACE(("\t%s (0x%X, 0x%X) ==> %O[%d]\n", CVMopnames[pc[0]],
		       STACK_INT(-1), STACK_INT(0),
		       directObj, GET_INDEX(pc+1)));
		UPDATE_PC_AND_TOS_AND_CONTINUE(3, -3);
	    } else {
		directObj = STACK_OBJECT(-2);	
		CHECK_NULL(directObj);
		JVMTI_WATCHING_FIELD_MODIFICATION(&STACK_ICELL(-2),
						  CVM_FALSE, CVMfbIsRef(fb));
		if (CVMfbIsRef(fb)) {
		    CVMD_fieldWriteRef(directObj, CVMfbOffset(fb),
				       STACK_OBJECT(-1));
		} else {
		    CVMD_fieldWrite32(directObj, CVMfbOffset(fb),
				      STACK_INFO(-1));
		}
		TRACE(("\t%s (0x%X) ==> %O[%d]\n", CVMopnames[pc[0]],
		       STACK_INT(-1), directObj, GET_INDEX(pc+1)));
		UPDATE_PC_AND_TOS_AND_CONTINUE(3, -2);
	    }
        }

    opc_aastore_overflow:
	ASMLABEL(opc_aastore_overflow);
	{
	    CVMObject* rhsObject = STACK_OBJECT(-1);
	    ARRAY_INTRO(Ref, -3);
	    if (rhsObject != 0) {
		/* Check assignability of rhsObject into arrObj */
		CVMClassBlock* elemType =
		    CVMarrayElementCb(CVMobjectGetClass(arrObj));
		CVMClassBlock* rhsType = CVMobjectGetClass(rhsObject);
		if (rhsType != elemType) { /* Try a quick check first */
		    if (!CVMisAssignable(ee, rhsType, elemType)) {
			if (!CVMlocalExceptionOccurred(ee)) {
			    goto array_store_exception;
			} else {
			    /* When C StackOverflowError is thrown */
			    goto handle_exception;
			}
		    }
		}
	    }
	    CVMD_arrayWriteRef(arrObj, index, rhsObject);
	    TRACE(("\t%s 0x%x ==> %O[%d] ==>\n",
		   CVMopnames[pc[0]], rhsObject, arrObj, index));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -3);
	}

    opc_anewarray_quick_overflow:
	ASMLABEL(opc_anewarray_quick_overflow);
	{
	    CVMJavaInt     arrLen;
	    CVMClassBlock* arrCb;
	    CVMClassBlock* elemCb;
	    CVMArrayOfRef* directArr;

	    arrLen  = STACK_INT(-1);
	    if (arrLen < 0) {
		goto negative_array_size_exception;
	    }
	    /*
	     * This doesn't have to run static initializers, so we
	     * don't have to do INIT_CLASS_IF_NEEDED()
	     */
	    elemCb = CVMcpGetCb(cp, GET_INDEX(pc + 1));

	    /*
	     * CVMclassGetArrayOf may block. CVMgcAllocNewArrayOf may block
	     * and initiate GC. So we must de-cache our state for GC
	     * first.  
	     */
	    DECACHE_PC();
	    DECACHE_TOS();

	    CVMD_gcSafeExec(ee, {
		arrCb = CVMclassGetArrayOf(ee, elemCb);
	    });
	    if (arrCb == NULL) {
		goto handle_exception;
	    }

	    directArr = (CVMArrayOfRef*)
		CVMgcAllocNewArray(ee, CVM_T_CLASS, arrCb, arrLen);
	    if (directArr == NULL) {
		CVMthrowOutOfMemoryError(ee, "%C", arrCb);
		goto handle_exception;
	    }
	    CVMID_icellSetDirect(ee, &STACK_ICELL(-1), (CVMObject*)directArr);

	    TRACE(("\tanewarray_quick %C => 0x%x\n", arrCb, directArr));

	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 0);
	}

    opc_multianewarray_quick_overflow:
	ASMLABEL(opc_multianewarray_quick_overflow);
	{
	    CVMClassBlock* arrCb;
	    CVMInt32       nDimensions;
	    CVMInt32       dimCount; /* in case we encounter a 0 */
	    CVMInt32       effectiveNDimensions;
	    CVMObjectICell* resultCell;

	    arrCb         = CVMcpGetCb(cp, GET_INDEX(pc + 1));
	    nDimensions   = pc[3];
	    effectiveNDimensions = nDimensions;

	    DECACHE_TOS(); /* it doesn't matter when we DECACHE_TOS really */

	    /*
	     * Point to beginning of dimension elements instead
	     * of the end, for convenience.
	     */
	    topOfStack   -= nDimensions;

	    /*
	     * Now go through the dimensions. Check for negative dimensions.
	     * Also check for whether there is a 0 in the dimensions array
	     * which would prevent the allocation from continuing.
	     */
	    for (dimCount = 0; dimCount < nDimensions; dimCount++) {
		CVMInt32 dim = STACK_INT(dimCount);
		if (dim <= 0) {
		    /* Remember the first 0 in the dimensions array */
		    if ((dim == 0) && (effectiveNDimensions == nDimensions)) {
			effectiveNDimensions = dimCount + 1;
			/* ... and keep checking for -1's despite a 0 */
		    } else if (dim < 0) {
			goto negative_array_size_exception;
		    }
		}
	    }

	    /*
	     * CVMmultiArrayAlloc() will do all the work
	     *
	     * Be paranoid about GC in this case (too many intermediate
	     * results).
	     */
	    DECACHE_PC();
	    resultCell = CVMmiscICell(ee);
	    CVMassert(CVMID_icellIsNull(resultCell));
	    CVMD_gcSafeExec(ee, {
		/* 
		 * do not cast the pointer type of topOfStack
		 * because it is required for correct stack access
		 * via topOfStack[index] in CVMmultiArrayAlloc
		 */
		CVMmultiArrayAlloc(ee, effectiveNDimensions,
				   topOfStack,
				   arrCb, resultCell);
	    });

	    if (CVMID_icellIsNull(resultCell)) {
		/*
		 * CVMmultiArrayAlloc() returned a null. This only happens
		 * if an allocation failed, so throw an OutOfMemoryError
		 */
		goto out_of_memory_error;
	    }
	    CVMID_icellAssignDirect(ee, &STACK_ICELL(0), resultCell);
	    /*
	     * Set this to null when we're done. The misc icell is
	     * needed elsewhere, and the nullness is used to prevent
	     * errors.
	     */
	    CVMID_icellSetNull(resultCell);

	    TRACE(("\tmultianewarray_quick %C "
		   "(dimensions = %d, effective = %d) => 0x%x\n",
		   arrCb, nDimensions, effectiveNDimensions, STACK_OBJECT(0)));

	    UPDATE_PC_AND_TOS_AND_CONTINUE(4, 1);
	}

    opc_invokestatic_checkinit_quick_overflow:
	ASMLABEL(opc_invokestatic_checkinit_quick_overflow);
        {
	    CVMClassBlock* cb;
	    mb = CVMcpGetMb(cp, GET_INDEX(pc + 1));
	    cb = CVMmbClassBlock(mb);
	    INIT_CLASS_IF_NEEDED(ee, cb);
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    receiverObjICell = CVMcbJavaInstance(cb);
	    goto callmethod_static;
	}

    opc_invokesuper_quick_overflow:
	ASMLABEL(opc_invokesuper_quick_overflow);
        {
	    CVMObject* directObj;
	    CVMClassBlock* cb = CVMmbClassBlock(frame->mb);
	    mb = CVMcbMethodTableSlot(CVMcbSuperclass(cb), GET_INDEX(pc + 1));
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    topOfStack -= CVMmbArgsSize(mb);
	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    goto callmethod;
	}

    opc_invokevirtual_quick_w_overflow:
	ASMLABEL(opc_invokevirtual_quick_w_overflow);
        {
	    CVMObject* directObj;
	    mb = CVMcpGetMb(cp, GET_INDEX(pc + 1));
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    topOfStack -= CVMmbArgsSize(mb);
	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    /* JDK uses the equivalent of CVMobjMethodTableSlot, because
	     * in lossless mode you quicken into opc_invokevirtual_quick_w
	     * instead of opc_invokevirtualobject_quick. When we don't
	     * care about lossless mode, we use the faster
	     * CVMcbMethodTableSlot version.
	     */
#ifdef CVM_NO_LOSSY_OPCODES
	    mb = CVMobjMethodTableSlot(directObj, CVMmbMethodTableIndex(mb));
#else
	    mb = CVMcbMethodTableSlot(CVMobjectGetClass(directObj),
				      CVMmbMethodTableIndex(mb));
#endif /* CVM_NO_LOSSY_OPCODES */
	    goto callmethod;
	}

    opc_invokeinterface_quick_overflow:
	ASMLABEL(opc_invokeinterface_quick_overflow);
	{
	    CVMObject*      directObj;
	    CVMMethodBlock* imb;  /* interface mb for method we are calling */
	    CVMClassBlock*  icb;  /* interface cb for method we are calling */
	    CVMClassBlock*  ocb;  /* cb of the object */
	    CVMUint32       interfaceCount;
	    CVMInterfaces*  interfaces;
	    CVMInterfaceTable* itablePtr;
	    CVMUint16       methodTableIndex;/* index into ocb's methodTable */
	    int guess = pc[4];

	    DECACHE_TOS(); /* must include arguments for proper gc */

	    imb = CVMcpGetMb(cp, GET_INDEX(pc + 1));
	    icb = CVMmbClassBlock(imb);

	    /* don't use pc[3] because transtion mb's don't set it up */
	    topOfStack -= CVMmbArgsSize(imb);

	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    ocb = CVMobjectGetClass(directObj);

	    interfaces = CVMcbInterfaces(ocb);
	    interfaceCount = CVMcbInterfaceCountGivenInterfaces(interfaces);
	    itablePtr = CVMcbInterfaceItable(interfaces);

	    /*
	     * Search the objects interface table, looking for the entry
	     * whose interface cb matches the cb of the inteface we
	     * are invoking.
	     */
	    if (guess < 0 || guess >= interfaceCount || 
		CVMcbInterfacecbGivenItable(itablePtr, guess) != icb) {
		
		guess = interfaceCount - 1;
		
		while ((guess >= 0) && 
		       (CVMcbInterfacecbGivenItable(itablePtr,guess) != icb)) {
		    guess--;
		}

		if (guess >= 0) {
		    CVMassert(CVMcbInterfacecbGivenItable(itablePtr, guess) 
			      == icb);
		    /* Update the guess if it was wrong, the code is not
		     * for a romized class, and the code is not transition
		     * code, which is also in ROM.
		     */
		    if (CVMcbInterfacecb(ocb, guess) == icb) {
		        if (!CVMisROMPureCode(CVMmbClassBlock(frame->mb)) && 
			    !CVMframeIsTransition(frame)) {
			    pc[4] = guess;
			}
		    }
		}
	    }

            if (guess >= 0) {
	        /*
	         * We know CVMcbInterfacecb(ocb, guess) is for the interface that 
	         * we are invoking. Use it to convert the index of the method in 
	         * the interface's methods array to the index of the method 
	         * in the class' methodTable array.
	         */
	        methodTableIndex = CVMcbInterfaceMethodTableIndexGivenInterfaces(
                    interfaces, guess, CVMmbMethodSlotIndex(imb));

	        /*
	         * Fetch the proper mb and it's cb.
	         */
	        mb = CVMcbMethodTableSlot(ocb, methodTableIndex);
            } else if (icb ==  CVMsystemClass(java_lang_Object)) {
                /*
                 * Per VM spec 5.4.3.4, the resolved interface method may be
                 * a method of java/lang/Object.
                 */ 
                methodTableIndex = CVMmbMethodTableIndex(imb);
                mb = CVMcbMethodTableSlot(ocb, methodTableIndex);
            } else {
                goto incompatible_class_change_error;
            }

            goto callmethod;
	}

    opc_exittransition_overflow:
	ASMLABEL(opc_exittransition_overflow);
        {
	    /* We get here when we've reached a transition frame
	     * that was setup by the init_class: label to invoke
	     * Class.runStaticInitializers, or by an CNI method that
	     * returned CNI_NEW_TRANSITION_FRAME. Method.invoke[BCI...]
	     * is currently the only method that causes this type of
	     * transition frame to be setup. See c07/13/99 to see why
	     * CNI methods may want to setup a transtion frame and
	     * return CNI_NEW_TRANSITION_FRAME.
	     */
	    char 		retType;
	    CVMBool         incrementPC;
	    
	    TRACE_METHOD_RETURN(frame);
	    
	    /* There should never be an outstanding local
	     * exception because handle_exception skips transition
	     * frames (except for the initialframe). (Note
	     * when callmethod is done it goes to
	     * check_for_exception after invoking JNI and CVM
	     * methods. Java methods, of course, can only raise
	     * exceptions using athrow.)
	     */
	    CVMassert(!CVMlocalExceptionOccurred(ee));
	    
	    /* See whether there was a return value from the
	     * invoked method. If there was, copy it onto the
	     * caller's frame. This makes it possible to use
	     * transition frames to implement the CVM methods
	     * called by Method.invoke(); the return value from
	     * the invoked method will be copied onto the "real"
	     * invoker's stack, which in this case is the
	     * Java-based Method.invoke() implementation.
	     */
	    retType = 
		CVMtypeidGetReturnType(CVMmbNameAndTypeID(frame->mb));
	    
	    /* There are only two cases we need to check: void and
	     * long/double. Every other return type is 32 bit. */
	    if (retType == CVM_TYPEID_VOID) {
		CACHE_PREV_TOS();
	    } else if ((retType == CVM_TYPEID_LONG) ||
		       (retType == CVM_TYPEID_DOUBLE)) {
		/* NOTE where topOfStack points and where each
		 * frame->topOfStack points. topOfStack is cached and
		 * points just above the return argument. 
		 * frame->topOfStack in the transition frame points just
		 * BELOW the return argument.
		 * CVMframePrev(frame)->topOfStack points just BELOW the
		 * arguments to whatever (CNI) method caused this 
		 * transition frame to be pushed.
		 */
		CVMJavaVal64 result;
		CVMmemCopy64(result.v, &STACK_INFO(-2).raw);
		CACHE_PREV_TOS();
		CVMmemCopy64(&STACK_INFO(0).raw, result.v);
		topOfStack += 2;
	    } else {
		CVMJavaVal32 result = STACK_INFO(-1);
		CACHE_PREV_TOS();
		STACK_INFO(0) = result;
		topOfStack++;
	    }
	    
	    /* Check if we will need to increment the pc after popping
	     * the transition frame. We need to do this before
	     * the frame is popped and we lose the transition frame.
	     */
	    incrementPC = CVMframeIncrementPcFlag(frame);
	    
	    CVMpopFrame(stack, frame);   /* pop the transition frame */
#ifdef CVM_JIT
	    if (CVMframeIsCompiled(frame)) {
		CVMCompiledResultCode resCode;
		
		mb = frame->mb;
return_to_compiled:
		DECACHE_TOS();
		resCode = CVMreturnToCompiledHelper(ee, frame, &mb, NULL);
		goto compiled_code_result_code;
		
return_to_compiled_with_exception:
		/* return to the compiled method handling the exception */
		topOfStack--;
		DECACHE_TOS();
		resCode = 
		    CVMreturnToCompiledHelper(ee, frame, &mb,
					      STACK_OBJECT(0));

compiled_code_result_code:
		switch (resCode) {
		case CVM_COMPILED_RETURN:
		    CACHE_FRAME();
		    CACHE_TOS();
		    locals = CVMframeLocals(frame);
		    cp = CVMframeCp(frame);
		    CACHE_PC();
		    pc += (*pc == opc_invokeinterface_quick ? 5 : 3);
		    CONTINUE;
		case CVM_COMPILED_NEW_MB:
		    CACHE_FRAME();
		    CACHE_TOS();
		    topOfStack -= CVMmbArgsSize(mb);
		    if (CVMmbIs(mb, STATIC)) {
			receiverObjICell =
			    CVMcbJavaInstance(CVMmbClassBlock(mb));
		    } else {
			receiverObjICell = &STACK_ICELL(0);
		    }
		    goto new_mb0;
		case CVM_COMPILED_NEW_TRANSITION:
		    CACHE_FRAME();
		    CVMframeIncrementPcFlag(frame) = CVM_TRUE;
		    isStatic = CVMmbIs(mb, STATIC);
		    isVirtual = CVM_FALSE;
		    goto new_transition;
		case CVM_COMPILED_EXCEPTION:
		    CACHE_FRAME();
		    goto handle_exception;
		}
	    invoke_compiled:
		resCode = CVMinvokeCompiledHelper(ee, frame, &mb);
		goto compiled_code_result_code;
	    }
#endif
	    CACHE_PC();                  /* fetch pc from new frame */
	    locals = CVMframeLocals(frame);
	    cp = CVMframeCp(frame);
	    if (incrementPC) {
		pc += (*pc == opc_invokeinterface_quick ? 5 : 3);
		CONTINUE;
	    } else {
		FETCH_NEXT_OPCODE(0);
#ifdef CVM_JVMTI
		/* Prevent double breakpoints. If the opcode we are
		 * returning to is an opc_breakpoint, then we've
		 * already notified the debugger of the breakpoint
		 * once. We don't want to do this again.
		 */
		if (opcode == opc_breakpoint) {
		    DECACHE_PC();
		    DECACHE_TOS();
		    CVMD_gcSafeExec(ee, {
			opcode = CVMjvmtiGetBreakpointOpcode(ee, pc,
							     CVM_FALSE);
		    });
#ifndef CVM_PREFETCH_OPCODE
                } else {
                    opcode = *pc;
#endif
		}
                REEXECUTE_BYTECODE();
#else
		DISPATCH_OPCODE();
#endif
	    }
	}

    opc_wide_overflow:
	ASMLABEL(opc_wide_overflow);
	{
            CVMUint16 reg = GET_INDEX(pc + 2);
	    JVMPI_TRACE_INSTRUCTION();

            switch(pc[1]) {
                case opc_aload:
                case opc_iload:
                case opc_fload:
		    FETCH_NEXT_OPCODE(4);
                    STACK_SLOT(0) = locals[reg]; 
                    TRACEIF(opc_iload, ("\tiload_w locals[%d](%d) =>\n", 
					reg, STACK_INT(0)));
                    TRACEIF(opc_aload, ("\taload_w locals[%d](%0x%x) =>\n", 
					reg, STACK_OBJECT(0)));
                    TRACEIF(opc_fload, ("\tfload_w locals[%d](%f) =>\n",
                            	        reg, STACK_FLOAT(0)));
            	    UPDATE_PC_AND_TOS_AND_CONTINUE(4, 1);
                case opc_lload:
                case opc_dload:
		    FETCH_NEXT_OPCODE(4);
	  	    CVMmemCopy64(&STACK_INFO(0).raw, &locals[reg].j.raw);
                    TRACEIF(opc_lload, ("\tlload_w locals[%d](%s) =>\n",
                            		reg, GET_LONGCSTRING(STACK_LONG(0))));
                    TRACEIF(opc_dload, ("\tdload_w locals[%d](%f) =>\n",
                            		reg, STACK_DOUBLE(0)));
            	    UPDATE_PC_AND_TOS_AND_CONTINUE(4, 2);
                case opc_istore:
                case opc_astore:
                case opc_fstore:
		    FETCH_NEXT_OPCODE(4);
		    locals[reg] = STACK_SLOT(-1);
                    TRACEIF(opc_istore,
			    ("\tistore_w %d ==> locals[%d]\n",
			     STACK_INT(-1), reg));
                    TRACEIF(opc_astore, ("\tastore_w 0x%x => locals[%d]\n",
                                 STACK_OBJECT(-1), reg));
                    TRACEIF(opc_fstore,
			    ("\tfstore_w %f ==> locals[%d]\n",
			     STACK_FLOAT(-1), reg));
            	    UPDATE_PC_AND_TOS_AND_CONTINUE(4, -1);
                case opc_lstore:
                case opc_dstore:
		    FETCH_NEXT_OPCODE(4)
		    CVMmemCopy64(&locals[reg].j.raw, &STACK_INFO(-2).raw);
                    TRACEIF(opc_lstore, ("\tlstore_w %s => locals[%d]\n",
                                 GET_LONGCSTRING(STACK_LONG(-2)), reg));
                    TRACEIF(opc_dstore, ("\tdstore_w %f => locals[%d]\n",
                                 STACK_DOUBLE(-2), reg));
            	    UPDATE_PC_AND_TOS_AND_CONTINUE(4, -2);
                case opc_iinc: {
		    CVMInt16 offset = CVMgetInt16(pc+4); 
		    FETCH_NEXT_OPCODE(6);
		    locals[reg].j.i += offset; 
            	    TRACE(("\tiinc_w locals[%d]+%d => %d\n", reg, 
                          offset, locals[reg].j.i));
            	    UPDATE_PC_AND_TOS_AND_CONTINUE(6, 0);
	        }
                case opc_ret:
		    /* %comment h002 */
		    if (CHECK_PENDING_REQUESTS(ee)) {
			goto handle_pending_request;
		    }
            	    TRACE(("\tret_w %d (%#x)\n", reg, locals[reg].a));
                    pc = locals[reg].a;
            	    CONTINUE;
                default:
                    CVMassert(CVM_FALSE);
            }
        }

    opc_tableswitch_overflow:
	ASMLABEL(opc_tableswitch_overflow);
	{
	    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(pc+1);
	    CVMInt32  key  = STACK_INT(-1);
	    CVMInt32  low  = CVMgetAlignedInt32(&lpc[1]);
	    CVMInt32  high = CVMgetAlignedInt32(&lpc[2]);
	    CVMInt32  skip;
	    TRACE(("\ttableswitch %d [%d-%d]\n", key, low, high));
	    key -= low;
            JVMPI_TRACE_TABLESWITCH(key + low, low, high);
	    skip = ((CVMUint32) key > high - low)
		        ? CVMgetAlignedInt32(&lpc[0])
		        : CVMgetAlignedInt32(&lpc[key + 3]);
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(skip, -1);
	}

    opc_lookupswitch_overflow:
	ASMLABEL(opc_lookupswitch_overflow);
	{
	    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(pc+1);
	    CVMInt32  key  = STACK_INT(-1);
	    CVMInt32  skip = CVMgetAlignedInt32(lpc); /* default amount */
	    CVMInt32  npairs = CVMgetAlignedInt32(&lpc[1]);
#ifdef CVM_JVMPI_TRACE_INSTRUCTION
            CVMInt32  pairsCount = npairs;
#endif
	    TRACE(("\tlookupswitch %d\n", key));
	    while (--npairs >= 0) {
		lpc += 2;
		if (key == CVMgetAlignedInt32(lpc)) {
		    skip = CVMgetAlignedInt32(&lpc[1]);
		    break;
		}
	    }
            JVMPI_TRACE_LOOKUPSWITCH(pairsCount - npairs - 1, pairsCount);
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(skip, -1);
	}

        {
	    int direction;
    opc_dcmpl_overflow:
	    ASMLABEL(opc_dcmpl_overflow);
	    direction = -1;
	    goto opc_dcmp_overflow;
    opc_dcmpg_overflow:
	    ASMLABEL(opc_dcmpg_overflow);
	    direction = 1;
    opc_dcmp_overflow:
	    ASMLABEL(opc_dcmp_overflow);
	    {
		CVMJavaDouble value1, value2; 
		value1 = CVMjvm2Double(&STACK_INFO(-4).raw);
		value2 = CVMjvm2Double(&STACK_INFO(-2).raw);
		STACK_INT(-4) = CVMdoubleCompare(value1, value2, direction);
		TRACE(("\t%s => %d\n", CVMopnames[pc[0]], STACK_INT(-4)));
		UPDATE_PC_AND_TOS_AND_CONTINUE_1_m3;
	    }
        }

    /* NOTE - These might fit in 64 bytes on some processors, but not sparc */
    opc_ldiv_overflow:
	ASMLABEL(opc_ldiv_overflow);
	OPC_LONG_BINARY(Div, 1)
    opc_ddiv_overflow:
	ASMLABEL(opc_ddiv_overflow);
	OPC_DOUBLE_BINARY(Div, 1)
    opc_lrem_overflow:
	ASMLABEL(opc_lrem_overflow);
	OPC_LONG_BINARY(Rem, 1)
    opc_frem_overflow:
	ASMLABEL(opc_frem_overflow);
	OPC_FLOAT_BINARY(Rem, 1);
    opc_drem_overflow:
	ASMLABEL(opc_drem_overflow);
	OPC_DOUBLE_BINARY(Rem, 1)

     /*
      * opc_if overflow code. Handles the dispatching when the branch
      * is taken, which has the overhead of a gc checkpoint.
      */
    opc_if_overflow:
	ASMLABEL(opc_if_overflow);
	{
	    int skip = CVMgetInt16(pc + 1);
            UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(skip, -1);
	}

    opc_if_icmp_overflow:
    opc_if_acmp_overflow:
	ASMLABEL(opc_if_cmp_overflow);
	{
	    int skip = CVMgetInt16(pc + 1);
            UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(skip, -2);
	}

    opc_new_checkinit_quick_overflow:
	ASMLABEL(opc_new_checkinit_quick_overflow);
	INIT_CLASS_IF_NEEDED(ee, newCb);
        /* fall through */

    opc_new_quick_overflow:
	ASMLABEL(opc_new_quick_overflow);
	{
	    CVMObject* directObj;

	    /*
	     * CVMgcAllocNewInstance may block and initiate GC, so we must
	     * de-cache our state for GC first.
	     */
	    DECACHE_TOS();
	    DECACHE_PC();

	    directObj = CVMgcAllocNewInstance(ee, newCb);		
	    if (directObj == 0) {
		goto out_of_memory_error_for_new_quick;
	    }
	    CVMID_icellSetDirect(ee, &STACK_ICELL(0), directObj);

	    TRACE(("\tnew_quick %C => 0x%x\n", newCb, directObj));

	    UPDATE_PC_AND_TOS_AND_CONTINUE_3_1;
	}

#if defined(CVM_ALIGNMENT_BASED_DISPATCHING) && defined(CVM_JVMPI)
    opc_goto_overflow:
	{
	    CVMInt16 skip = CVMgetInt16(pc + 1);
	    TRACE(("\t%s %#x (skip=%d)\n",CVMopnames[pc[0]],
		   pc + skip, skip));
	    /* %comment h002 */
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(skip, 0);
	}
#endif

    opc_jsr_overflow:
	ASMLABEL(opc_jsr_overflow);
        {
	    CVMInt16 skip = CVMgetInt16(pc + 1);
	    STACK_ADDR(0) = pc + 3; /* push return address on the stack */
	    TRACE(("\t%s %#x (skip=%d)\n",CVMopnames[pc[0]],
		   pc + skip, skip));
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(skip, 1);
	}

    opc_goto_w_overflow:
	ASMLABEL(opc_goto_w_overflow);
	{
	    CVMInt32 skip = CVMgetInt32(pc + 1);
	    TRACE(("\t%s %#x (skip=%d)\n", CVMopnames[pc[0]],
		   pc + skip, skip));
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(skip, 0);
	}

    opc_jsr_w_overflow:
	ASMLABEL(opc_jsr_w_overflow);
        {
	    CVMInt32 skip = CVMgetInt32(pc + 1);
	    STACK_ADDR(0) = pc + 5;
	    TRACE(("\t%s %#x (skip=%d)\n", CVMopnames[pc[0]],
		   pc + skip, skip));
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(skip, 1);
	}

    handle_pending_request: {
	ASMLABEL(label_handle_pending_request);
#ifdef CVM_REMOTE_EXCEPTIONS_SUPPORTED
	if (CVMD_gcSafeCheckRequest(ee))
#endif
	{
	    CVMD_gcSafeCheckPoint(ee, {
		DECACHE_PC();
		DECACHE_TOS();
	    }, {});
	    CONTINUE;
	}
#ifdef CVM_REMOTE_EXCEPTIONS_SUPPORTED
	else {
	    CVMassert(CVMremoteExceptionOccurred(ee));
	    /* We will be gcsafe soon, so flush our state. */
	    DECACHE_PC();
	    goto handle_exception;
	}
#endif
    } /* handle_pending_request: */

#ifdef CVM_CLASSLOADING
	/* 
	 * Opcode quickener
	 *  -Set clobbersCpIndex true if the quickened opcode clobbers the
	 *   constant pool index in the instruction.
	 */
    quicken_opcode:
	ASMLABEL(label_quicken_opcode);
	{	    
	    CVMQuickenReturnCode retCode;
	    CVMClassBlock *cb;
	    DECACHE_TOS();
	    DECACHE_PC();
	    TRACE(("\t%s ==> quickening...\n", CVMopnames[pc[0]]));
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
	    retCode = CVMquickenOpcode(ee, pc, cp, &cb, clobbersCpIndex);
	    switch (retCode) {
	        case CVM_QUICKEN_ALREADY_QUICKENED:
		    break;
	        case CVM_QUICKEN_NEED_TO_RUN_STATIC_INITIALIZERS:
		    initCb = cb;
		    goto init_class;
	        case CVM_QUICKEN_ERROR: {
		    CVMassert(CVMexceptionOccurred(ee));
		    goto handle_exception;
		}
	        default:
		    CVMassert(CVM_FALSE); /* We should never get here */
	    }
	    FETCH_NEXT_OPCODE(0);
#ifdef CVM_JVMTI
	    /* Prevent double breakpoints. If the opcode we just
	     * quickened is an opc_breakpoint, then we've
	     * already notified the debugger of the breakpoint
	     * once. We don't want to do this again.
	     */
	    if (opcode == opc_breakpoint) {
		CVMD_gcSafeExec(ee, {
		    opcode = CVMjvmtiGetBreakpointOpcode(ee, pc, CVM_TRUE);
		});
#ifndef CVM_PREFETCH_OPCODE
            } else {
                opcode = *pc;
#endif
	    }
            REEXECUTE_BYTECODE();
#else
	    DISPATCH_OPCODE();
#endif
	} /* quicken_opcode */
#endif /* CVM_CLASSLOADING */
    } /* while (1) interpreter loop */

/*
 * CVM_JAVA_ERROR - Macro for throwing a java exception from
 * the interpreter loop. It makes sure that the PC is flushed
 * and the topOfStack is reset before CVMsignalError is called.
 */
#define CVM_JAVA_ERROR(exception_, error_)			\
{								\
    DECACHE_PC();	/* required for CVMsignalError */	\
    CVM_RESET_INTERPRETER_TOS(frame->topOfStack, frame,		\
			      CVMframeIsJava(frame));		\
    CVMthrow##exception_(ee, error_);				\
    goto handle_exception_tos_already_reset;			\
}

    null_pointer_exception:
	ASMLABEL(label_null_pointer_exception);
	CVM_JAVA_ERROR(NullPointerException, NULL);

    out_of_memory_error:
	ASMLABEL(label_out_of_memory_error);
	CVM_JAVA_ERROR(OutOfMemoryError, NULL);

    array_index_out_of_bounds_exception:
	ASMLABEL(label_array_index_out_of_bounds_exception);
	CVM_JAVA_ERROR(ArrayIndexOutOfBoundsException, NULL);

    array_store_exception:
	ASMLABEL(label_array_store_exception);
	CVM_JAVA_ERROR(ArrayStoreException, NULL);

    arithmetic_exception_divide_by_zero:
	ASMLABEL(label_arithmetic_exception_divide_by_zero);
	CVM_JAVA_ERROR(ArithmeticException, "/ by zero");

    negative_array_size_exception:
	ASMLABEL(label_negative_array_size_exception);
	CVM_JAVA_ERROR(NegativeArraySizeException, NULL);

    illegal_monitor_state_exception_thread_not_owner:
	ASMLABEL(label_illegal_monitor_state_exception_thread_not_owner);
	CVM_JAVA_ERROR(IllegalMonitorStateException, 
		       "current thread not owner");

    out_of_memory_error_for_newarray:
    ASMLABEL(label_out_of_memory_error_for_newarray);
    {
        CVMBasicType typeCode = pc[1];
	CVMClassBlock* arrCb =
	    (CVMClassBlock*)CVMbasicTypeArrayClassblocks[typeCode];
	CVMthrowOutOfMemoryError(ee, "%C", arrCb);
	goto handle_exception_tos_already_reset;
    }

    out_of_memory_error_for_new_quick:
    ASMLABEL(label_out_of_memory_error_for_new_quick);
    {
	CVMClassBlock* newCb = CVMcpGetCb(cp, GET_INDEX(pc + 1));
	CVMthrowOutOfMemoryError(ee, "%C", newCb);
	goto handle_exception_tos_already_reset;
    }

    class_cast_exception:
    ASMLABEL(label_class_cast_exception);
    {
	CVMObject* directObj = STACK_OBJECT(-1);
	CVMClassBlock* cb = CVMobjectGetClass(directObj);
	DECACHE_PC();	/* required for CVMsignalError() */
	CVM_RESET_JAVA_TOS(frame->topOfStack, frame);
	/* CVMgcUnsafeIsInstanceOf() may have thrown StackOverflowError */
	if (!CVMlocalExceptionOccurred(ee)) {
	    CVMthrowClassCastException(ee, "%C", cb);
	}
	goto handle_exception_tos_already_reset;
    }

    incompatible_class_change_error:
    ASMLABEL(label_incompatible_class_change_error);
    {
	CVMMethodBlock* imb;  /* interface mb for method we are calling */
	CVMClassBlock*  icb;  /* interface cb for method we are calling */
	CVMClassBlock*  ocb;  /* cb of the object */
	imb = CVMcpGetMb(cp, GET_INDEX(pc + 1));
	icb = CVMmbClassBlock(imb);
	ocb = CVMobjectGetClass(STACK_OBJECT(0));
	DECACHE_PC();  /* required for CVMsignalError() */
	CVMthrowIncompatibleClassChangeError(
	    ee,
	    "class %C does not implement interface %C",
	    ocb, icb);
	goto handle_exception;
    }

    /*
     * This is really the entry code for executeJava(), but we need to move
     * it after the opcodes implementations because on sarm the following
     * code results in a large table being embedded in the middle of the
     * opcodes, which messes up the alignment-based dispatching.
     */
executejava_setup:
    ASMLABEL(label_executejava_setup);

    /* C stack redzone check */
    if (!CVMCstackCheckSize(ee, CVM_REDZONE_ILOOP, 
        "CVMgcUnsafeExecuteJavaMethod", CVM_TRUE)) {
	return;
    }

    CVMassert(CVMD_isgcUnsafe(ee));

#ifdef CVM_CHECK_OPCODE_SIZES
    {
	static CVMBool opcodeSizeCheckNeeded = CVM_TRUE;
	if (opcodeSizeCheckNeeded) {
	    opcodeSizeCheckNeeded = CVM_FALSE;
	    CVMcheckOpcodeSizes(firstOpcodeImplPtr, opclabels);
	}
    }
#endif /* CVM_ALIGNMENT_BASED_DISPATCHING */

    /* The initialframe is always a transition frame. Its topOfStack
     * always points just after the arguments. 
     */
    CACHE_FRAME();
    initialframe = frame;

    /* We reach this point each time a new transtion frame has been
     * pushed. This happens just before CVMgcUnsafeExecuteJavaMethod
     * is first invoked, by the init_class: label, and if
     * CVM_NEW_TRANSITION_FRAME is returned from an CVM native method.
     */
new_transition:
    ASMLABEL(label_new_transition);
    CVMassert(CVMframeIsTransition(frame));
    CACHE_TOS();   /* topOfStack points after the arguments. */

    /* Get the correct transtion code pointer. */
    if (isStatic) {
	/* static method */
	pc = (CVMUint8*)CVMinvokeStaticTransitionCode;
    } else {
	if (CVMcbIs(CVMmbClassBlock(mb), INTERFACE)) {
	    /* interface method */
	    pc = (CVMUint8*)CVMinvokeInterfaceTransitionCode;
	} else if (isVirtual && !CVMmbIs(mb, PRIVATE) &&
		   !(CVMtypeidIsConstructor(CVMmbNameAndTypeID(mb)))) {
	    /* virtual method */
	    pc = (CVMUint8*)CVMinvokeVirtualTransitionCode;
	} else {
	    /* nonvirtual method */ 
	    pc = (CVMUint8*)CVMinvokeNonVirtualTransitionCode;
	}
    }

    /* The constant pool for the transition code has just one entry
     * which points to the resolved mb of the method we want to call.
     */
    CVMinitTransitionConstantPool(&transitioncp, mb);
    cp = (CVMConstantPool*)&transitioncp;

		
    /* This is needed to get CVMframe2string() to work properly */
    CVMdebugExec(DECACHE_PC());
    TRACE_METHOD_CALL(frame, CVM_FALSE);
    TRACESTATUS();

#ifdef CVM_ALIGNMENT_BASED_DISPATCHING
    /* This is an effort to keep the opclabels live */
    if (ee == (CVMExecEnv*)1) {
	goto *opclabels[0];
    }
#endif

    goto dispatch_opcode;

 finish:
    ASMLABEL(label_finish);

#ifdef CVM_ADD_4K_PAD
    /* This is an effort to keep the post_loop_alignment label live */
    if (ee == (CVMExecEnv*)1) {
	goto post_loop_alignment;
    }
#endif

    TRACE(("Exiting interpreter\n"));
    DECACHE_TOS();
    DECACHE_PC();

    return;
}
