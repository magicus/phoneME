/*
 * @(#)executejava_split2.c	1.77 06/10/25
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
 * CVM_USELABELS - If using GCC, then use labels for the opcode dispatching
 * rather than a switch statement. This improves performance because it
 * gives us the oportunity to have the instructions that calculate the
 * next opcode to jump to be intermixed with the rest of the instructions
 * that implement the opcode (see UPDATE_PC_AND_TOS_AND_CONTINUE macro).
 */
#ifdef __GNUC__
#define CVM_USELABELS
#endif

/*
 * CVM_PREFETCH_OPCODE - Some compilers do better if you prefetch the next 
 * opcode before going back to the top of the while loop, rather then having
 * the top of the while loop handle it. This provides a better opportunity
 * for instruction scheduling. Some compilers just do this prefetch
 * automatically. Some actually end up with worse performance if you
 * force the prefetch. Solaris gcc seems to do better, but cc does worse.
 */
#ifdef __GNUC__
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
#define TRACESTATUS(frame) {						    \
    CVMtraceStatus(("stack=0x%x frame=0x%x locals=0x%x tos=0x%x pc=0x%x\n", \
		    &ee->interpreterStack, frame, locals, topOfStack, pc)); \
 }

#ifdef CVM_TRACE
#define TRACEIF(o, a)				\
    if (*pc == o) {				\
        TRACE(a);				\
    }
#else
#define TRACEIF(o, a)
#endif

/*
 * CVM_JAVA_ERROR - Macro for throwing a java exception from
 * the interpreter loop. It makes sure that the PC is flushed
 * and the topOfStack is reset before CVMsignalError is called.
 */
#define CVM_JAVA_ERROR(exception_, error_)			\
{								\
    DECACHE_PC(frame);	/* required for CVMsignalError */	\
    CVM_RESET_JAVA_TOS(frame->topOfStack, frame);		\
    CVMthrow##exception_(ee, error_);				\
    goto handle_exception;					\
}

/* 
 * INIT_CLASS_IF_NEEDED - Makes sure that static initializers are
 * run for a class.  Used in opc_*_checkinit_quick opcodes.
 */
#undef INIT_CLASS_IF_NEEDED
#define INIT_CLASS_IF_NEEDED(ee, cb)				\
    if (CVMcbInitializationNeeded(cb, ee)) {			\
        DECACHE_PC(frame);					\
        DECACHE_TOS(frame);					\
        *retCb = cb;						\
        /* pc == NULL means init_class must be performed */	\
        return NULL;						\
    }								\
    JVMPI_TRACE_INSTRUCTION();

/*
 * Check that the returned PC and TOS from the second level are in line
 * with the current frame 
 */
#ifdef CVM_DEBUG_ASSERTS
static CVMBool
frameSanity(CVMFrame* frame)
{
    return CVMframePrev(frame) != frame;
}
#endif

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
#define JVMTI_SINGLE_STEPPING(opcodeVar)				\
{									\
    if (ee->jvmtiSingleStepping && CVMjvmtiThreadEventsEnabled(ee)) {	\
	DECACHE_PC(frame);						\
	DECACHE_TOS(frame);						\
	CVMD_gcSafeExec(ee, {						\
	    CVMjvmtiPostSingleStepEvent(ee, pc);			\
	});								\
	/* Refetch opcode. See above. */				\
	opcodeVar = *pc;						\
	OPCODE_UPDATE_NEXT(opcodeVar);					\
    }									\
}

#define JVMTI_WATCHING_FIELD_ACCESS(location)				\
    if (CVMglobals.jvmtiWatchingFieldAccess) {				\
	DECACHE_PC(frame);						\
	DECACHE_TOS(frame);						\
	CVMD_gcSafeExec(ee, {						\
		CVMjvmtiPostFieldAccessEvent(ee, location, fb);		\
	});								\
    }

#define JVMTI_WATCHING_FIELD_MODIFICATION(location, isDoubleWord, isRef)      \
   if (CVMglobals.jvmtiWatchingFieldModification) {			      \
       jvalue val;							      \
       DECACHE_PC(frame);						      \
       DECACHE_TOS(frame);						      \
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
	       CVMjvmtiPostFieldModificationEvent(ee, location, fb, val);     \
       });								      \
   }

#else
#define JVMTI_SINGLE_STEPPING(opc)
#define JVMTI_WATCHING_FIELD_ACCESS(location)
#define JVMTI_WATCHING_FIELD_MODIFICATION(location, isDoubleWord, isRef)

#endif

#ifdef CVM_JVMPI
#define JVMPI_CHECK_FOR_DATA_DUMP_REQUEST(ee) \
    if (CVMjvmpiDataDumpWasRequested()) {     \
        DECACHE_PC(frame);                    \
        DECACHE_TOS(frame);                   \
        CVMD_gcSafeExec(ee, {                 \
            CVMjvmpiPostDataDumpRequest();    \
        });                                   \
    }

#else
#define JVMPI_CHECK_FOR_DATA_DUMP_REQUEST(ee)
#endif

/* NOTE: About JVMPI bytecode tracing:

    JVMPI bytecode tracing is only supported if CVM is built with
    CVM_JVMPI_TRACE_INSTRUCTION=true.  Otherwise, the JVMPI trace macros will
    be #defined out.

    JVMPI Trace Macros:
    ==================
    Some JVMPI bytecode trace events require additional information.  These
    special cases are handled by posting the event using special macros as
    follows:
        1. JVMPI_TRACE_IF() for condition "if" bytecodes.
        2. JVMPI_TRACE_TABLESWITCH() fot the tableswitch bytecode.
        3. JVMPI_TRACE_LOOKUPSWITCH() for the lookupswitch bytecode.

    For everything else, the generic JVMPI_TRACE_INSTRUCTION macro is used to
    post the JVMPI bytecode trace event.

    Embedding calls to the trace macros:
    ===================================
    In most cases, a call to JVMPI_TRACE_INSTRUCTION() is embedded in the CASE()
    macro.  This ensures that the JVMPI event is posted before the bytecode is
    executed.  This gets around complications with dealing with exceptions that
    may be thrown before the bytecode executes completely.

    However, allowance is made for some special cases for which posting the
    generic trace event from within the CASE() macro is inadequate.  For these
    cases, the CASE_NT() macro (NT stands for "No Trace") is introduced.  The
    CASE_NT() macro behaves exactly like the CASE() macro with the exception of
    not calling JVMPI_TRACE_INSTRUCTION().  The special cases where the
    CASE_NT() macro is used are as follows:

        1. The bytecodes which require additional information in their JVMPI
           trace events as outlined above.  The opcode's handler will be
           responsible for calling the appropriate macro (JVMPI_TRACE_IF(),
           JVMPI_TRACE_TABLESWITCH(), or JVMPI_TRACE_LOOKUPSWITCH()) to post
           the event with the appropriate information.

        2. Bytecodes which are interpreted by the same handler.  For
           example,

                CASE_NT(opc_1)
                CASE_NT(opc_2)
                CASE_NT(opc_3)
                CASE(opc_4)
                    ... // Execute bytecode.

            Note opc1, opc2, and opc3 did not need to call
            JVMPI_TRACE_INSTRUCTION() because opc4 will call it on their behalf.

        3. Bytecode handlers which fall through to another bytecode handler.
           For example,

                CASE_NT(opc5)
                    ... // Execute part of bytecode functionality.
                    // Fall through to opc6 below.

                CASE(opc6)
                    ... // Execute some more functionality.

            Note opc5 did not need to call JVMPI_TRACE_INSTRUCTION() because
            opc6 will call it on its behalf.  This is only used where opc5
            will always fall through to opc6.  If opc5 could end up branching
            elsewhere (perhaps due to an exception being thrown), then other
            special treatment should be applied.

        4. Bytecodes which will be quickened.  These don't need to call
           JVMPI_TRACE_INSTRUCTION() because their quicken formed will be
           executed after the bytecode is quickened.  The quickened bytecode
           will call JVMPI_TRACE_INSTRUCTION().

        5. Bytecodes which need to check if a static initializer has run.
           The call to JVMPI_TRACE_INSTRUCTION() should happen after the check
           to see if the static initializer has already been called.  This is
           because if static initialization is needed, the first attempt to
           execute the bytecode will fail, and the bytecode will be executed
           a second time after the static initializer has been called.  We only
           want to post the JVMPI trace event once.  Hence the call to
           JVMPI_TRACE_INSTRUCTION() is done in the INIT_CLASS_IF_NEEDED()
           macro only after the static initializer has been executed.

*/
#ifdef CVM_JVMPI_TRACE_INSTRUCTION
/* The following macros are for posting the JVMPI_EVENT_INSTRUCTION_START
 * event for JVMPI support of tracing bytecode execution: */

#define JVMPI_TRACE_INSTRUCTION() {                     \
    if (CVMjvmpiEventInstructionStartIsEnabled()) {     \
        DECACHE_PC(frame);                              \
        DECACHE_TOS(frame);                             \
        CVMjvmpiPostInstructionStartEvent(ee, pc);      \
    }                                                   \
}

#define JVMPI_TRACE_IF(isTrue) {                                \
    if (CVMjvmpiEventInstructionStartIsEnabled()) {             \
        DECACHE_PC(frame);                                      \
        DECACHE_TOS(frame);                                     \
        CVMjvmpiPostInstructionStartEvent4If(ee, pc, isTrue);   \
    }                                                           \
}

#define JVMPI_TRACE_TABLESWITCH(key, low, hi) {                      \
    if (CVMjvmpiEventInstructionStartIsEnabled()) {                  \
        DECACHE_PC(frame);                                           \
        DECACHE_TOS(frame);                                          \
        CVMjvmpiPostInstructionStartEvent4Tableswitch(ee, pc, key,   \
                                                      low, hi);      \
    }                                                                \
}

#define JVMPI_TRACE_LOOKUPSWITCH(chosenPairIndex, pairsTotal) { \
    if (CVMjvmpiEventInstructionStartIsEnabled()) {             \
        DECACHE_PC(frame);                                      \
        DECACHE_TOS(frame);                                     \
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
    __asm__ __volatile__ (".LL_" #name ":");
#else
#define ASMLABEL(name)
#endif

/*
 * Macro for setting up an opcode case or goto label.
 */
#undef PRIVATE_CASE
#ifdef CVM_USELABELS
#define PRIVATE_CASE(opcode_, jvmpiAction_)     \
    opcode_:                                    \
    ASMLABEL(opcode_);                          \
    jvmpiAction_;
#define DEFAULT					\
    opc_DEFAULT:				\
    ASMLABEL(opc_DEFAULT);
#else
#define PRIVATE_CASE(opcode_, jvmpiAction_)     \
    case opcode_:                               \
    ASMLABEL(opcode_);                          \
    jvmpiAction_;
#define DEFAULT					\
    default:					\
    ASMLABEL(opc_DEFAULT);
#endif

#undef CASE
#define CASE(opcname)                           \
    PRIVATE_CASE(opcname, {                     \
        JVMPI_TRACE_INSTRUCTION();              \
    })

/* CASE_NT (no trace), in contrast with the CASE() macro, does not post a
 * JVMPI_EVENT_INSTRUCTION_START event.  This is used where special handling
 * is needed to post the JVMPI event, or when the case falls through to
 * another case immediately below it which will post the event. */
#undef CASE_NT
#define CASE_NT(opcname)                         \
    PRIVATE_CASE(opcname, {})

/*
 * CONTINUE - Macro for executing the next opcode.
 */
#undef CONTINUE
#ifdef CVM_USELABELS
#define CONTINUE_NEXT(opc)			\
	UPDATE_INSTRUCTION_COUNT(opc);		\
        JVMTI_SINGLE_STEPPING(opc);		\
	goto *nextLabel;

#define CONTINUE {					\
        CVMUint32 opcode = *pc;				\
        const void* nextLabel = opclabels[opcode];	\
        CONTINUE_NEXT(opcode);				\
    }
#else
#ifdef CVM_PREFETCH_OPCODE
#define CONTINUE_NEXT(opc)			\
	UPDATE_INSTRUCTION_COUNT(opc);		\
        JVMTI_SINGLE_STEPPING(opc);		\
	continue;

#define CONTINUE {				\
        opcode = *pc; 				\
        CONTINUE_NEXT(opcode);			\
    }
#else
#define CONTINUE_NEXT(opc)			\
	UPDATE_INSTRUCTION_COUNT(opc);		\
        JVMTI_SINGLE_STEPPING(opc);		\
	continue;

#define CONTINUE {				\
        CONTINUE_NEXT(opcode);			\
    }
#endif
#endif

/*
 * REEXECUTE_CURRENT_BYTECODE - Macro for reexecuting the opcode.
 */
#undef REEXECUTE_BYTECODE
#ifdef CVM_USELABELS
#define REEXECUTE_BYTECODE(opc) {		\
	goto *opclabels[(opc)];			\
    }
#else
#ifdef CVM_PREFETCH_OPCODE
#define REEXECUTE_BYTECODE(opc) {		\
        opcode = (opc); 			\
	goto skip_prefetch;			\
    }
#else
#define REEXECUTE_BYTECODE(opc) {		\
        opcode = (opc);				\
	goto skip_prefetch;			\
    }
#endif
#endif

#ifdef CVM_USELABELS
#define OPCODE_INTRO_DECL					\
    CVMUint32 opcode;						\
    const void* nextLabel;
#define OPCODE_UPDATE_NEXT(opc)					\
    nextLabel = opclabels[(opc)]
#else
#define OPCODE_INTRO_DECL
#define OPCODE_UPDATE_NEXT(opc)
#endif

/*
 * This is for prefetching the next opcode
 */
#ifdef CVM_PREFETCH_OPCODE
#define OPCODE_UPDATE_NEW(opc)     opcode = (opc)
#else
#define OPCODE_UPDATE_NEW(opc)
#endif

/*
 * UPDATE_PC_AND_TOS - Macro for updating the pc and topOfStack.
 */
#undef UPDATE_PC_AND_TOS
#define UPDATE_PC_AND_TOS(opsize, stack) \
    {pc += opsize; topOfStack += stack;}

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
 * UPDATE_PC_AND_TOS_AND_CONTINUE - Macro for updating the pc and topOfStack,
 * and executing the next opcode. It's somewhat similar to the combination
 * of UPDATE_PC_AND_TOS and CONTINUE, but with some minor optimizations.
 */
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_WORK
#define UPDATE_PC_AND_TOS_AND_CONTINUE_WORK(opsize, stack, doPendingCheck) {\
        OPCODE_INTRO_DECL					\
        /* 							\
         * Offer a GC-safe point. The current frame is always up\
         * to date, so don't worry about de-caching frame.  	\
         */							\
	if ((doPendingCheck) && CHECK_PENDING_REQUESTS(ee)) {	\
            goto handle_pending_request;			\
        }							\
        JVMPI_CHECK_FOR_DATA_DUMP_REQUEST(ee);                  \
        OPCODE_UPDATE_NEW(pc[opsize]);			        \
        OPCODE_UPDATE_NEXT(opcode);				\
        UPDATE_PC_AND_TOS(opsize, stack);			\
        CONTINUE_NEXT(opcode);					\
    }

#undef UPDATE_PC_AND_TOS_AND_CONTINUE
#define UPDATE_PC_AND_TOS_AND_CONTINUE(opsize, stack) \
    UPDATE_PC_AND_TOS_AND_CONTINUE_WORK((opsize), (stack), CVM_FALSE)

/*
 * For those opcodes that need to have a GC point
 */
#undef UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_GC_CHECK
#define UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_GC_CHECK(skip, stack) \
    UPDATE_PC_AND_TOS_AND_CONTINUE_WORK((skip), (stack), CVM_TRUE)

#undef UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK
#define UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(skip, stack) \
    UPDATE_PC_AND_TOS_AND_CONTINUE_WORK((skip), (stack), ((skip) <= 0))

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

#define DECACHE_TOS(frame)    {					\
    (frame)->topOfStack = topOfStack;				\
}
#define CACHE_TOS(frame)    {					\
    topOfStack = (frame)->topOfStack;				\
}
#define CACHE_PREV_TOS(frame) topOfStack = CVMframePrev(frame)->topOfStack;

#undef DECACHE_PC
#undef CACHE_PC
#define DECACHE_PC(frame)	{				\
    CVMframePc((frame)) = pc;					\
}

#define CACHE_PC(frame)	{					\
    pc = CVMframePc((frame));					\
}

/*
 * CHECK_NULL - Macro for throwing a NullPointerException if the object
 * passed is a null ref.
 */
#undef CHECK_NULL
#define CHECK_NULL(obj_) 				\
    if ((obj_) == NULL) {   				\
        goto throwNullPointerException;			\
    }

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
 *
 * Returns the current PC.
 */
CVMUint8*
CVMgcUnsafeExecuteJavaMethodQuick(CVMExecEnv* ee, CVMUint8* pc,
				  CVMStackVal32* topOfStack,
				  CVMSlotVal32* locals,
				  CVMConstantPool* cp,
				  CVMClassBlock** retCb)
{
#ifdef CVM_TRACE
    char              trBuf[30];   /* needed by GET_LONGCSTRING */
#endif
#ifndef CVM_USELABELS
    CVMUint32         opcode;
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
#endif /* CVM_USELABELS */

    CVMMethodBlock* mb;
    CVMFrame* frame;
    
    frame = ee->interpreterStack.currentFrame;

    CVMassert(CVMD_isgcUnsafe(ee));

#ifdef CVM_PREFETCH_OPCODE
#ifndef CVM_USELABELS
    opcode = *pc;  /* prefetch first opcode */
#endif
#endif

#ifdef CVM_JVMTI
    if (*pc == opc_breakpoint) {
	CVMUint32 opcode0;
	
	CVMD_gcSafeExec(ee, {
	    opcode0 = CVMjvmtiGetBreakpointOpcode(ee, pc, CVM_FALSE);
	});
        REEXECUTE_BYTECODE(opcode0);
    }
#endif

#ifndef CVM_USELABELS
    while (1)
    {
#endif
#ifndef CVM_PREFETCH_OPCODE
	opcode = *pc;
#endif
	/* Using this label avoids double breakpoints when quickening and
	 * when returing from transition frames.
	 */
#ifndef CVM_USELABELS
skip_prefetch:
#endif

	/*
	 * Concentrate all byte-code definition macros here, so we can
	 * freely re-order their locations in the handler list 
	 */
#undef  OPC_CONST_n
#define OPC_CONST_n(opcname, const_type, value)				\
	CASE(opcname) {							\
            OPCODE_INTRO_DECL						\
									\
	    TRACE(("\t%s\n", CVMopnames[*pc])); 			\
            OPCODE_UPDATE_NEW(pc[1]);					\
            topOfStack += 1;						\
            OPCODE_UPDATE_NEXT(opcode);   				\
            pc += 1;							\
	    STACK_INFO(-1).const_type = value; 				\
            CONTINUE_NEXT(opcode);					\
        }

#undef  OPC_CONST2_n
#define OPC_CONST2_n(opcname, value, key)	     			\
        CASE(opc_##opcname)						\
        {                                                               \
           CVM##key##2Jvm(&STACK_INFO(0).raw,				\
	       CVM##key##Const##value());	   			\
	   TRACE(("\t%s\n", CVMopnames[*pc])); 				\
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, 2);                        \
        }

/* Load 1 word values from a local variable */
#define OPC_LOAD1_n(type, num)						\
	CASE(opc_##type##load_##num) {					\
            OPCODE_INTRO_DECL						\
            CVMSlotVal32 l;						\
									\
            OPCODE_UPDATE_NEW(pc[1]);					\
            pc += 1;							\
            l = locals[num];						\
            OPCODE_UPDATE_NEXT(opcode);   				\
            topOfStack += 1;						\
	    TRACEIF(opc_##type##load_##num,				\
		    ("\t%s locals[%d](0x%x) =>\n",			\
		     CVMopnames[pc[-1]], num, STACK_OBJECT(0)));       	\
	    STACK_SLOT(-1) = l;						\
            CONTINUE_NEXT(opcode);					\
       }

/* Store a 1 word local variable */
#undef  OPC_STORE1_n
#define OPC_STORE1_n(type, num)						\
	CASE(opc_##type##store_##num) {					\
            OPCODE_INTRO_DECL						\
            CVMSlotVal32 l;						\
									\
            OPCODE_UPDATE_NEW(pc[1]);					\
            pc += 1;							\
            topOfStack -= 1;						\
            l = STACK_SLOT(0);						\
            OPCODE_UPDATE_NEXT(opcode);   				\
	    TRACEIF(opc_##type##load_##num, 				\
		    ("\t%s locals[%d](0x%x) =>\n",			\
		     CVMopnames[pc[0]], num, STACK_OBJECT(0)));		\
	    locals[num] = l;						\
            CONTINUE_NEXT(opcode);					\
	}

/* Load 2 word values from a local variable */
#define OPC_LOAD2_n(type, num)						\
	CASE(opc_##type##load_##num)					\
	    CVMmemCopy64(&STACK_INFO(0).raw, &locals[num].j.raw);	\
	    TRACEIF(opc_dload_##num, ("\t%s locals[%d](%f) =>\n",	\
				      CVMopnames[*pc], num,		\
				      STACK_DOUBLE(0)));		\
	    TRACEIF(opc_lload_##num, ("\t%s locals[%d](%s) =>\n",	\
				      CVMopnames[*pc], num,		\
				      GET_LONGCSTRING(STACK_LONG(0)))); \
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 2);

/* Store a 2 word local variable */
#undef  OPC_STORE2_n
#define OPC_STORE2_n(type, num)						\
	CASE(opc_##type##store_##num)					\
	    CVMmemCopy64(&locals[num].j.raw, &STACK_INFO(-2).raw);	\
	    TRACEIF(opc_dstore_##num, ("\t%s %f => locals[%d]\n",	\
		    CVMopnames[*pc], STACK_DOUBLE(-2), num));		\
	    TRACEIF(opc_lstore_##num, ("\t%s %s => locals[%d]\n",	\
		    CVMopnames[*pc], 					\
		    GET_LONGCSTRING(STACK_LONG(-2)), num));		\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -2);

/* Perform various binary integer operations */
#undef  OPC_INT1_BINARY 
#define OPC_INT1_BINARY(opcname, opname, test)				\
	CASE(opc_i##opcname) {						\
            OPCODE_INTRO_DECL						\
	    CVMJavaInt lhs, rhs, result;				\
									\
            OPCODE_UPDATE_NEW(pc[1]);					\
	    rhs = STACK_INT(-1);					\
	    lhs = STACK_INT(-2);					\
            OPCODE_UPDATE_NEXT(opcode);   				\
	    if (test && (rhs == 0)) {					\
                goto throwArithmethicException;				\
	    }								\
	    pc += 1;							\
            result = CVMint##opname(lhs, rhs);				\
	    topOfStack -= 1;						\
	    								\
            STACK_INT(-1) = result;					\
	    								\
	    TRACE(("\t%s => %d\n", CVMopnames[*pc], STACK_INT(-1)));	\
									\
            CONTINUE_NEXT(opcode);					\
	}
	

#undef  OPC_INT2_BINARY 
#define OPC_INT2_BINARY(opcname, opname, test)				\
	CASE(opc_l##opcname)						\
	{								\
	    CVMJavaLong l1, l2, r1;					\
	    l2 = CVMjvm2Long(&STACK_INFO(-2).raw);			\
	    if (test && CVMlongEqz(l2)) {				\
                goto throwArithmethicException;				\
	    }								\
	    l1 = CVMjvm2Long(&STACK_INFO(-4).raw);			\
	    r1 = CVMlong##opname(l1, l2);				\
	    CVMlong2Jvm(&STACK_INFO(-4).raw, r1);			\
	    TRACE(("\t%s => %s\n", CVMopnames[*pc],			\
		   GET_LONGCSTRING(STACK_LONG(-4))));		       	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -2);			\
	}

/* Perform various binary floating number operations */
#undef  OPC_FLOAT1_BINARY 
#define OPC_FLOAT1_BINARY(opcname, opname)				   \
        CASE(opc_f##opcname)						   \
            STACK_FLOAT(-2) =						   \
		CVMfloat##opname(STACK_FLOAT(-2), STACK_FLOAT(-1));	   \
            TRACE(("\t%s => %f\n", CVMopnames[*pc], STACK_FLOAT (-2)));    \
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

#undef  OPC_FLOAT2_BINARY 
#define OPC_FLOAT2_BINARY(opcname, opname)				   \
	CASE(opc_d##opcname) {						   \
            CVMJavaDouble l1, l2, r;					   \
            l1 = CVMjvm2Double(&STACK_INFO(-4).raw);			   \
            l2 = CVMjvm2Double(&STACK_INFO(-2).raw);			   \
            r = CVMdouble##opname(l1, l2);				   \
            CVMdouble2Jvm(&STACK_INFO(-4).raw, r);			   \
            TRACE(("\t%s => %f\n", CVMopnames[*pc], STACK_DOUBLE(-4)));    \
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -2);			   \
        }								   \

 	/* Shift operations				   
	 * Shift left int and long: ishl, lshl           
	 * Logical shift right int and long w/zero extension: iushr, lushr
	 * Arithmetic shift right int and long w/sign extension: ishr, lshr
	 */
#undef  OPC_SHIFT1_BINARY
#define OPC_SHIFT1_BINARY(opcname, opname)				\
        CASE(opc_i##opcname)						\
	   STACK_INT(-2) = CVMint##opname(STACK_INT(-2), STACK_INT(-1));\
           TRACE(("\t%s => %d\n", CVMopnames[*pc], STACK_INT(-2)));	\
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);			\
      
#undef  OPC_SHIFT2_BINARY
#define OPC_SHIFT2_BINARY(opcname, opname)				\
        CASE(opc_l##opcname)						\
	{								\
	   CVMJavaLong v, r;						\
	   v = CVMjvm2Long(&STACK_INFO(-3).raw);			\
	   r = CVMlong##opname(v, STACK_INT(-1));			\
	   CVMlong2Jvm(&STACK_INFO(-3).raw, r);				\
           TRACE(("\t%s => %s\n", CVMopnames[*pc],			\
		    GET_LONGCSTRING(STACK_LONG(-3)))); 			\
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);			\
	}
 
	/* comparison operators */
#define COMPARISON_OP_WITHZERO(name, comparison)			     \
	CASE_NT(opc_if##name) {						     \
            int skip = (STACK_INT(-1) comparison 0)			     \
                        ? CVMgetInt16(pc + 1) : 3;			     \
            JVMPI_TRACE_IF((STACK_INT(-1) comparison 0));        	     \
            TRACE(("\t%s goto +%d (%staken)\n",				     \
		   CVMopnames[*pc],					     \
		   CVMgetInt16(pc + 1),				             \
		   (STACK_INT(-1) comparison 0) ? "" : "not "));	     \
            UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_GC_CHECK(skip, -1);   	     \
        }


#define COMPARISON_OP_INTS(name, comparison)				     \
	CASE_NT(opc_if_icmp##name) {					     \
            int skip = (STACK_INT(-2) comparison STACK_INT(-1))		     \
	                ? CVMgetInt16(pc + 1) : 3;			     \
            JVMPI_TRACE_IF((STACK_INT(-2) comparison STACK_INT(-1)));        \
            TRACE(("\t%s goto +%d (%staken)\n",				     \
		   CVMopnames[*pc],					     \
		   CVMgetInt16(pc + 1),				    	     \
		   (STACK_INT(-2) comparison STACK_INT(-1)) ? "" : "not ")); \
            UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_GC_CHECK(skip, -2);   	     \
        }								     \

#define COMPARISON_OP_REFS(name, comparison)				    \
	CASE_NT(opc_if_acmp##name) {					    \
	    int skip = (STACK_OBJECT(-2) comparison STACK_OBJECT(-1))	    \
			 ? CVMgetInt16(pc + 1) : 3;			    \
            JVMPI_TRACE_IF((STACK_OBJECT(-2) comparison STACK_OBJECT(-1))); \
            TRACE(("\t%s goto +%d (%staken)\n",				    \
		   CVMopnames[*pc],					    \
		   CVMgetInt16(pc + 1),					    \
                   (STACK_OBJECT(-2) comparison STACK_OBJECT(-1))	    \
		   ? "" : "not "));					    \
            UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_GC_CHECK(skip, -2);	    \
        }

#define NULL_COMPARISON_OP(name, not)					\
	CASE_NT(opc_if##name) {						\
            int skip = (not(STACK_OBJECT(-1) == 0))			\
			? CVMgetInt16(pc + 1) : 3;			\
            JVMPI_TRACE_IF((not(STACK_OBJECT(-1) == 0)));		\
            TRACE(("\t%s goto +%d (%staken)\n",				\
		   CVMopnames[*pc],					\
		   CVMgetInt16(pc + 1),					\
		   (not(STACK_OBJECT(-1) == 0))				\
		   ? "" : "not "));					\
            UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_GC_CHECK(skip, -1);	\
        }

	/* Array access byte-codes */

	/* Every array access byte-code starts out like this */
#define ARRAY_INTRO(T, arrayOff)				      	      \
        CVMArrayOf##T* arrObj = (CVMArrayOf##T*)STACK_OBJECT(arrayOff);       \
	CVMJavaInt     index  = STACK_INT(arrayOff + 1);		      \
        CHECK_NULL(arrObj);						      \
        /* This clever idea is from ExactVM. No need to test index >= 0.      \
	   Instead, do an unsigned comparison. Negative indices will appear   \
	   as positive numbers >= 2^31, which is just out of the range	      \
	   of legal Java array indices */				      \
	if ((CVMUint32)index >= CVMD_arrayGetLength(arrObj)) {		      \
            goto throwArrayIndexOutOfBoundsException;			      \
	}

	/* 32-bit loads. These handle conversion from < 32-bit types */
#define ARRAY_LOADTO32(T, format, stackRes)				      \
        {								      \
	    ARRAY_INTRO(T, -2);						      \
	    CVMD_arrayRead##T(arrObj, index, stackRes(-2));		      \
	    TRACE(("\t%s %O[%d](" format ") ==>\n", CVMopnames[*pc],	      \
		   arrObj, index, stackRes(-1))); 			      \
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);			      \
        }

	/* 64-bit loads */
#define ARRAY_LOADTO64(T,T2)						\
        {								\
	    CVMJava##T temp64;						\
            ARRAY_INTRO(T, -2);						\
	    CVMD_arrayRead##T(arrObj, index, temp64);			\
	    CVM##T2##2Jvm(&STACK_INFO(-2).raw, temp64);			\
	    TRACE(("\t%s %O[%d](0x%X, 0x%X) ==>\n", CVMopnames[*pc],	\
		   arrObj, index, STACK_INT(-2), STACK_INT(-1)));      	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);			\
        }
	/* 32-bit stores. These handle conversion to < 32-bit types */
#define ARRAY_STOREFROM32(T, format, stackSrc)				      \
        {								      \
	    ARRAY_INTRO(T, -3);						      \
	    CVMD_arrayWrite##T(arrObj, index, stackSrc(-1));		      \
	    TRACE(("\t%s " format " ==> %O[%d] ==>\n", CVMopnames[*pc],    \
		   stackSrc(-1), arrObj, index)); 			      \
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -3);			      \
        }

	/* 64-bit stores */
#define ARRAY_STOREFROM64(T)						    \
        {								    \
	    CVMJava##T temp64;						    \
            ARRAY_INTRO(T, -4);						    \
	    temp64 = CVMjvm2##T(&STACK_INFO(-2).raw);			    \
	    CVMD_arrayWrite##T(arrObj, index, temp64);			    \
	    TRACE(("\t%s (0x%X, 0x%X) ==> %O[%d]\n", CVMopnames[*pc],    \
		   STACK_INT(-2), STACK_INT(-1), arrObj, index));      	    \
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -4);			    \
        }

#ifdef CVM_USELABELS
	goto *opclabels[*pc];
#else
	switch (opcode)
	{
#endif

	/* Quick object field access byte-codes. The non-wide version are
	 * all lossy and are not needed when in losslessmode.
	 */

#ifndef CVM_NO_LOSSY_OPCODES
        CASE(opc_getfield_quick)
	{
            OPCODE_INTRO_DECL
	    CVMObject* directObj;
	    CVMUint32  slotIndex; 

	    directObj = STACK_OBJECT(-1);
	    OPCODE_UPDATE_NEW(pc[3]);
	    OPCODE_UPDATE_NEXT(opcode);
	    TRACE(("\t%s %O[%d](0x%X) ==>\n", CVMopnames[*pc],
		   directObj, pc[1], STACK_INT(-1)));
	    slotIndex = pc[1];
	    CHECK_NULL(directObj);
	    pc += 3;
	    CVMD_fieldRead32(directObj, slotIndex, STACK_INFO(-1));

	    CONTINUE_NEXT(opcode);
        }

        CASE(opc_putfield_quick)
	{
            OPCODE_INTRO_DECL
	    CVMUint32 slotIndex;
	    CVMObject* directObj;
	    
	    directObj = STACK_OBJECT(-2);
	    OPCODE_UPDATE_NEW(pc[3]);
	    OPCODE_UPDATE_NEXT(opcode);
	    TRACE(("\t%s (0x%X) ==> %O[%d]\n", CVMopnames[*pc],
		   STACK_INT(-1), directObj, pc[1]));
	    slotIndex = pc[1];
	    CHECK_NULL(directObj);
	    pc += 3;
	    CVMD_fieldWrite32(directObj, slotIndex, STACK_INFO(-1));
	    topOfStack -= 2;

	    CONTINUE_NEXT(opcode);
        }

	CASE(opc_agetfield_quick)
	{
            OPCODE_INTRO_DECL
	    CVMObject* directObj;
	    CVMUint32  slotIndex; 
	    CVMObject* fieldObj;

	    directObj = STACK_OBJECT(-1);
	    OPCODE_UPDATE_NEW(pc[3]);
	    OPCODE_UPDATE_NEXT(opcode);
	    TRACE(("\t%s %O[%d](0x%x) ==>\n", CVMopnames[*pc],
		   directObj, pc[1], STACK_OBJECT(-1)));
	    slotIndex = pc[1];
	    CHECK_NULL(directObj);
	    pc += 3;
	    CVMD_fieldReadRef(directObj, slotIndex, fieldObj);
	    CVMID_icellSetDirect(ee, &STACK_ICELL(-1), fieldObj); 

	    CONTINUE_NEXT(opcode);
        }

	CASE(opc_aputfield_quick)
	{
            OPCODE_INTRO_DECL
	    CVMObject* directObj;
	    CVMUint32  slotIndex; 

	    directObj = STACK_OBJECT(-2);
	    OPCODE_UPDATE_NEW(pc[3]);
	    OPCODE_UPDATE_NEXT(opcode);
	    TRACE(("\t%s (0x%x) ==> %O[%d]\n", CVMopnames[*pc],
		   STACK_OBJECT(-1), directObj, pc[1]));
	    slotIndex= pc[1];
	    CHECK_NULL(directObj);
	    pc += 3;
	    CVMD_fieldWriteRef(directObj, slotIndex, STACK_OBJECT(-1));
	    topOfStack -= 2;

	    CONTINUE_NEXT(opcode);
        }

#endif /* CVM_NO_LOSSY_OPCODES */

	CASE(opc_bipush) {
	    /* Push a 1-byte signed integer value onto the stack. */
	    OPCODE_INTRO_DECL
	    CVMInt32 c;

            c = (CVMInt8)pc[1];
	    OPCODE_UPDATE_NEW(pc[2]);
	    OPCODE_UPDATE_NEXT(opcode);
            pc += 2;
            topOfStack += 1;
	    TRACE(("\tbipush %d\n", (int)c));
	    STACK_INT(-1) = c;

	    CONTINUE_NEXT(opcode);
	}
	
	    /* Push a 2-byte signed integer constant onto the stack. */
	CASE(opc_sipush) {
	    OPCODE_INTRO_DECL
	    CVMUint8 lo;
            CVMInt16 c;
	    
	    OPCODE_UPDATE_NEW(pc[3]);
	    OPCODE_UPDATE_NEXT(opcode);
	    lo = pc[2];
	    topOfStack += 1;
	    c = pc[1];
	    pc += 3;
	    c = (c << 8) | lo;
	    
	    STACK_INT(-1) = c;
	    TRACE(("\tsipush %d\n", (int)c));
	    
	    CONTINUE_NEXT(opcode);
        }
	
	    /* load from local variable */

	CASE_NT(opc_aload)
	CASE_NT(opc_iload)
	CASE(opc_fload) {
	    OPCODE_INTRO_DECL
	    CVMUint32  localNo; 
            CVMSlotVal32 l;
		
	    localNo = pc[1];
	    OPCODE_UPDATE_NEW(pc[2]);
	    l = locals[localNo];
	    OPCODE_UPDATE_NEXT(opcode);
	    pc += 2;
	    topOfStack += 1;
	    
	    STACK_SLOT(-1) = l;

	    TRACEIF(opc_aload, ("\taload locals[%d](0x%x) =>\n",
				pc[1], STACK_OBJECT(-1)));
	    TRACEIF(opc_iload, ("\tiload locals[%d](%d) =>\n", 
				pc[1], STACK_INT(-1)));
	    TRACEIF(opc_fload, ("\tfload locals[%d](%f) =>\n", 
				pc[1], STACK_FLOAT(-1)));

	    CONTINUE_NEXT(opcode);
	}
	
	OPC_LOAD1_n(a,0);
	OPC_LOAD1_n(i,0);
	OPC_LOAD1_n(f,0);

	OPC_LOAD1_n(a,1);
	OPC_LOAD1_n(i,1);
	OPC_LOAD1_n(f,1);

	OPC_LOAD1_n(a,2);
	OPC_LOAD1_n(i,2);
	OPC_LOAD1_n(f,2);

	OPC_LOAD1_n(a,3);
	OPC_LOAD1_n(i,3);
	OPC_LOAD1_n(f,3);

	    /* store to a local variable */

	CASE_NT(opc_astore)
	CASE_NT(opc_istore)
	CASE(opc_fstore) {
	    OPCODE_INTRO_DECL
	    CVMUint32  localNo; 
            CVMSlotVal32 l;
	
	    OPCODE_UPDATE_NEW(pc[2]);
	    pc += 2;
	    topOfStack -= 1;
	    
	    l = STACK_SLOT(0);
	    localNo = pc[-1];
	    OPCODE_UPDATE_NEXT(opcode);
	    locals[localNo] = l;
	    TRACEIF(opc_astore, ("\tastore 0x%x => locals[%d]\n",
				 STACK_OBJECT(-1), pc[-1]));
	    TRACEIF(opc_istore,
		    ("\tistore %d ==> locals[%d]\n", STACK_INT(-1), pc[-1]));
	    TRACEIF(opc_fstore,
		    ("\tfstore %f ==> locals[%d]\n", STACK_FLOAT(-1), pc[-1]));

	    CONTINUE_NEXT(opcode);
	}
	
	OPC_STORE1_n(i, 0);
	OPC_STORE1_n(a, 0);
	OPC_STORE1_n(f, 0);

	OPC_STORE1_n(i, 1);
	OPC_STORE1_n(a, 1);
	OPC_STORE1_n(f, 1);

	OPC_STORE1_n(i, 2);
	OPC_STORE1_n(a, 2);
	OPC_STORE1_n(f, 2);

	OPC_STORE1_n(i, 3);
	OPC_STORE1_n(a, 3);
	OPC_STORE1_n(f, 3);
	
	    /* Push miscellaneous constants onto the stack. */

	CASE(opc_aconst_null)
	    CVMID_icellSetNull(&STACK_ICELL(0));
	    TRACE(("\t%s\n", CVMopnames[*pc]));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);

	OPC_CONST_n(opc_iconst_m1,   i,       -1);
	OPC_CONST_n(opc_iconst_0,    i,        0);
	OPC_CONST_n(opc_iconst_1,    i,        1);
	OPC_CONST_n(opc_iconst_2,    i,        2);
	OPC_CONST_n(opc_iconst_3,    i,        3);
	OPC_CONST_n(opc_iconst_4,    i,        4);
	OPC_CONST_n(opc_iconst_5,    i,        5);
	OPC_CONST_n(opc_fconst_0,    f,      0.0);
	OPC_CONST_n(opc_fconst_1,    f,      1.0);
	OPC_CONST_n(opc_fconst_2,    f,      2.0);

	   /* Load constant from constant pool: */

	CASE(opc_aldc_ind_quick) { /* Indirect String (loaded classes) */
	    CVMObjectICell* strICell = CVMcpGetStringICell(cp, pc[1]);
	    CVMID_icellAssignDirect(ee, &STACK_ICELL(0), strICell);
            TRACE(("\t%s #%d => 0x%x\n", CVMopnames[*pc],
		   pc[1], STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 1);
	}

        CASE_NT(opc_aldc_quick) /* aldc loads direct String refs (ROM only) */
        CASE(opc_ldc_quick)
	    STACK_INFO(0) = CVMcpGetVal32(cp, pc[1]);
            TRACE(("\t%s #%d => 0x%x\n", CVMopnames[*pc],
		   pc[1], STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 1);

	/* stack pop, dup, and insert opcodes */

	CASE(opc_nonnull_quick)  /* pop stack, and error if it is null */
	    TRACE(("\tnonnull_quick\n"));
	    CHECK_NULL(STACK_OBJECT(-1));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

	   
	CASE(opc_pop)		 /* Discard the top item on the stack */
	    TRACE(("\tpop\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);

	   
	CASE(opc_pop2)		 /* Discard the top 2 items on the stack */
	    TRACE(("\tpop2\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -2);

	    
	CASE(opc_dup) {		/* Duplicate the top item on the stack */
	    OPCODE_INTRO_DECL
	    CVMJavaVal32 rhs;
	    
	    OPCODE_UPDATE_NEW(pc[1]);
	    rhs = STACK_INFO(-1);
	    OPCODE_UPDATE_NEXT(opcode);
	    topOfStack += 1;
	    pc += 1;
	    STACK_INFO(-1) = rhs;
	    TRACE(("\tdup\n"));

	    CONTINUE_NEXT(opcode);
	}
	
	CASE(opc_dup2)		/* Duplicate the top 2 items on the stack */
	    STACK_INFO(0) = STACK_INFO(-2);
	    STACK_INFO(1) = STACK_INFO(-1);
	    TRACE(("\tdup2\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 2);

	CASE(opc_dup_x1) {	/* insert top word two down */
	    OPCODE_INTRO_DECL
	    
	    OPCODE_UPDATE_NEW(pc[1]);
	    OPCODE_UPDATE_NEXT(opcode);
	    pc += 1;
	    topOfStack += 1;
	    STACK_INFO(-1) = STACK_INFO(-2);
	    STACK_INFO(-2) = STACK_INFO(-3);
	    STACK_INFO(-3) = STACK_INFO(-1);
	    TRACE(("\tdup_x1\n"));

	    CONTINUE_NEXT(opcode);
	}

	CASE(opc_dup_x2)	/* insert top word three down  */
	    STACK_INFO(0) = STACK_INFO(-1);
	    STACK_INFO(-1) = STACK_INFO(-2);
	    STACK_INFO(-2) = STACK_INFO(-3);
	    STACK_INFO(-3) = STACK_INFO(0);
	    TRACE(("\tdup_x2\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);

	CASE(opc_swap) {        /* swap top two elements on the stack */
	    CVMJavaVal32 j = STACK_INFO(-1);
	    STACK_INFO(-1) = STACK_INFO(-2);
	    STACK_INFO(-2) = j;
	    TRACE(("\tswap\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}

	OPC_INT1_BINARY(add, Add, 0);
	OPC_INT1_BINARY(sub, Sub, 0);
	OPC_INT1_BINARY(mul, Mul, 0);
	OPC_INT1_BINARY(and, And, 0);
	OPC_INT1_BINARY(or,  Or,  0);
	OPC_INT1_BINARY(xor, Xor, 0);

	OPC_FLOAT1_BINARY(add, Add);
	OPC_FLOAT1_BINARY(sub, Sub);
	OPC_FLOAT1_BINARY(mul, Mul);
	
        OPC_SHIFT1_BINARY(shl, Shl);
	OPC_SHIFT1_BINARY(shr, Shr);
	OPC_SHIFT1_BINARY(ushr, Ushr);

       /* Increment local variable by constant */ 
        CASE(opc_iinc) 
	{
	    CVMUint32  localNo; 
	    CVMInt32   incr;
	    OPCODE_INTRO_DECL
	
	    OPCODE_UPDATE_NEW(pc[3]);
	    localNo = pc[1];
	    incr    = (CVMInt8)(pc[2]);
	    pc += 3;
	    OPCODE_UPDATE_NEXT(opcode);
	    locals[localNo].j.i += incr;
	    TRACE(("\tiinc locals[%d]+%d => %d\n", pc[1], 
		   (CVMInt8)(pc[2]), locals[pc[1]].j.i)); 

	    CONTINUE_NEXT(opcode);
	}

	/* Check if an object is an instance of given type. Throw an
	 * exception if not. */
        CASE(opc_checkcast_quick)
	{
	    CVMObject* directObj;
	    CVMClassBlock* cb;
	    
	    directObj = STACK_OBJECT(-1);
	    cb = CVMcpGetCb(cp, GET_INDEX(pc+1));   /* cb of cast type */ 
	    TRACE(("\t%s %C\n", CVMopnames[*pc], cb));
	    if (!CVMgcUnsafeIsInstanceOf(ee, directObj, cb)) {
		goto throwClassCastException;
	    } 
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 0);
	}

	/* Check if an object is an instance of given type and store
	 * result on the stack. */
        CASE(opc_instanceof_quick)
	{
	    CVMObject* directObj;
	    CVMClassBlock* cb;
	    
	    directObj = STACK_OBJECT(-1);
	    cb = CVMcpGetCb(cp, GET_INDEX(pc+1));  /* cb of casttype */ 
	    STACK_INT(-1) = (directObj != NULL) &&
		CVMgcUnsafeIsInstanceOf(ee, directObj, cb); 
	    /* CVMgcUnsafeIsInstanceOf() may have thrown StackOverflowError */
	    if (CVMlocalExceptionOccurred(ee)) {
		goto handle_exception;
	    }
	    TRACE(("\t%s %C => %d\n", CVMopnames[*pc], cb, STACK_INT(-1)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 0);
 	}

       /* negate the value on the top of the stack */

        CASE(opc_ineg)
	   STACK_INT(-1) = CVMintNeg(STACK_INT(-1));
           TRACE(("\t%s => %d\n", CVMopnames[*pc], STACK_INT(-1)));
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

        CASE(opc_fneg)
	   STACK_FLOAT(-1) = CVMfloatNeg(STACK_FLOAT(-1));
           TRACE(("\t%s => %f\n", CVMopnames[*pc], STACK_FLOAT(-1)));
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

	/* Conversion operations */

        CASE(opc_i2f)	/* convert top of stack int to float */
	   STACK_FLOAT(-1) = CVMint2Float(STACK_INT(-1));
           TRACE(("\ti2f => %f\n", STACK_FLOAT(-1)));
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

        CASE(opc_f2i)  /* Convert top of stack float to int */
            STACK_INT(-1) = CVMfloat2Int(STACK_FLOAT(-1)); 
            TRACE(("\tf2i => %d\n", STACK_INT(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

        CASE(opc_i2b)
	    STACK_INT(-1) = CVMint2Byte(STACK_INT(-1));
            TRACE(("\ti2b => %d\n", STACK_INT(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

        CASE(opc_i2c)
	    STACK_INT(-1) = CVMint2Char(STACK_INT(-1));
            TRACE(("\ti2c => %d\n", STACK_INT(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

        CASE(opc_i2s)
	    STACK_INT(-1) = CVMint2Short(STACK_INT(-1));
            TRACE(("\ti2s => %d\n", STACK_INT(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);

	COMPARISON_OP_WITHZERO(lt, <);
	COMPARISON_OP_WITHZERO(gt, >);
	COMPARISON_OP_WITHZERO(le, <=);
	COMPARISON_OP_WITHZERO(ge, >=);

	COMPARISON_OP_INTS(lt, <);
	COMPARISON_OP_INTS(gt, >);
	COMPARISON_OP_INTS(le, <=);
	COMPARISON_OP_INTS(ge, >=);

	COMPARISON_OP_WITHZERO(eq, ==);
	COMPARISON_OP_WITHZERO(ne, !=);

	COMPARISON_OP_INTS(eq, ==);
	COMPARISON_OP_INTS(ne, !=);

	COMPARISON_OP_REFS(eq, ==);  /* include ref comparison */
        COMPARISON_OP_REFS(ne, !=);  /* include ref comparison */

	NULL_COMPARISON_OP(null, !!);
	NULL_COMPARISON_OP(nonnull, !);

	CASE(opc_return) {
	    /* 
	     * Offer a GC-safe point. The current frame is always up
	     * to date, so don't worry about de-caching frame.  
	     */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	    CACHE_PREV_TOS(frame);
	    TRACE(("\treturn\n"));
	    goto handleReturn;
        }
	
	CASE_NT(opc_ireturn)
	CASE_NT(opc_freturn)
	CASE(opc_areturn)
        {
	    CVMJavaVal32 result;
	    
	    /* 
	     * Offer a GC-safe point. The current frame is always up
	     * to date, so don't worry about de-caching frame.  
	     */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	    result = STACK_INFO(-1);
	    CACHE_PREV_TOS(frame);
	    STACK_INFO(0) = result;
	    topOfStack++;
	    TRACEIF(opc_ireturn, ("\tireturn %d\n", STACK_INT(-1)));
	    TRACEIF(opc_areturn, ("\tareturn 0x%x\n", STACK_OBJECT(-1)));
	    TRACEIF(opc_freturn, ("\tfreturn %f\n", STACK_FLOAT(-1)));
	    goto handleReturn;
	}

	CASE_NT(opc_agetstatic_quick)
	CASE(opc_getstatic_quick) {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    JVMTI_WATCHING_FIELD_ACCESS(NULL)
	    STACK_INFO(0) = CVMfbStaticField(ee, fb);
	    TRACE(("\t%s #%d %C.%F[%d] (0x%X) ==>\n",
		   CVMopnames[*pc], GET_INDEX(pc+1), CVMfbClassBlock(fb), fb, 
		   CVMfbOffset(fb), STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
	}

	CASE_NT(opc_aputstatic_quick)
	CASE(opc_putstatic_quick) {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    JVMTI_WATCHING_FIELD_MODIFICATION(NULL, CVM_FALSE, CVMfbIsRef(fb));
	    CVMfbStaticField(ee, fb) = STACK_INFO(-1);
	    TRACE(("\t%s #%d (0x%X) ==> %C.%F[%d]\n",
		   CVMopnames[*pc], GET_INDEX(pc+1), STACK_INT(-1), 
		   CVMfbClassBlock(fb), fb, CVMfbOffset(fb)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, -1);
	}


	/* Quick static field access for romized classes with initializers. */

	CASE_NT(opc_agetstatic_checkinit_quick)
	CASE_NT(opc_getstatic_checkinit_quick) {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    CVMClassBlock* cb = CVMfbClassBlock(fb);
	    INIT_CLASS_IF_NEEDED(ee, cb);
	    JVMTI_WATCHING_FIELD_ACCESS(NULL)
	    STACK_INFO(0) = CVMfbStaticField(ee, fb);
	    TRACE(("\t%s #%d %C.%F[%d] (0x%X) ==>\n",
		   CVMopnames[*pc], GET_INDEX(pc+1), cb, fb, 
		   CVMfbOffset(fb), STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
	}

	CASE_NT(opc_aputstatic_checkinit_quick)
	CASE_NT(opc_putstatic_checkinit_quick) {
	    CVMFieldBlock* fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    CVMClassBlock* cb = CVMfbClassBlock(fb);
	    INIT_CLASS_IF_NEEDED(ee, cb);
	    JVMTI_WATCHING_FIELD_MODIFICATION(NULL, CVM_FALSE, CVMfbIsRef(fb));
	    CVMfbStaticField(ee, fb) = STACK_INFO(-1);
	    TRACE(("\t%s #%d (0x%X) ==> %C.%F[%d]\n",
		   CVMopnames[*pc], GET_INDEX(pc+1), STACK_INT(-1), 
		   cb, fb, CVMfbOffset(fb)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, -1);
	}

        CASE(opc_iaload)
	    ARRAY_LOADTO32(Int,   "%d",   STACK_INT);
        CASE(opc_faload)
	    ARRAY_LOADTO32(Float, "%f",   STACK_FLOAT);
        CASE(opc_aaload)
	    ARRAY_LOADTO32(Ref,   "0x%x", STACK_OBJECT);
        CASE(opc_baload)
	    ARRAY_LOADTO32(Byte,  "%d",   STACK_INT);
        CASE(opc_caload)
	    ARRAY_LOADTO32(Char,  "%d",   STACK_INT);
        CASE(opc_saload)
	    ARRAY_LOADTO32(Short, "%d",   STACK_INT);

        CASE(opc_iastore)
	    ARRAY_STOREFROM32(Int,   "%d",   STACK_INT);
        CASE(opc_fastore)
	    ARRAY_STOREFROM32(Float, "%f",   STACK_FLOAT);
	/*
	 * This one looks different because of the assignability check
	 */
        CASE(opc_aastore) {
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
			    goto throwArrayStoreException;
			} else {
			    /* When C StackOverflowError is thrown */
			    goto handle_exception;
			}
		    }
		}
	    }
	    CVMD_arrayWriteRef(arrObj, index, rhsObject);
	    TRACE(("\t%s 0x%x ==> %O[%d] ==>\n",
		   CVMopnames[*pc], rhsObject, arrObj, index));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -3);
	}
        CASE(opc_bastore)
	    ARRAY_STOREFROM32(Byte,  "%d",   STACK_INT);
        CASE(opc_castore)
	    ARRAY_STOREFROM32(Char,  "%d",   STACK_INT);
        CASE(opc_sastore)
	    ARRAY_STOREFROM32(Short, "%d",   STACK_INT);

        CASE(opc_arraylength)
	{
	    CVMArrayOfAnyType* arrObj = (CVMArrayOfAnyType*)STACK_OBJECT(-1);
	    CHECK_NULL(arrObj);
	    STACK_INT(-1) = CVMD_arrayGetLength(arrObj);
	    TRACE(("\t%s %O.length(%d) ==>\n", CVMopnames[*pc],
		   arrObj, STACK_INT(-1)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}

	/* Throw an exception. */

	CASE(opc_athrow) {
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CHECK_NULL(directObj);
	    TRACE(("\tathrow => %O\n", directObj));
	    DECACHE_PC(frame);  /* required for handle_exception */
	    CVMgcUnsafeThrowLocalException(ee, directObj);
	    goto handle_exception;
	}

	/* monitorenter and monitorexit for locking/unlocking an object */

	/* goto and jsr. They are exactly the same except jsr pushes
	 * the address of the next instruction first.
	 */

        CASE(opc_jsr) {
	    CVMInt16 offset = CVMgetInt16(pc + 1);
	    /* push return address on the stack */
	    STACK_ADDR(0) = pc + 3;
	    TRACE(("\t%s %#x (offset=%d)\n",CVMopnames[*pc],
		   pc + offset, offset));
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_GC_CHECK(offset, 1);
	}

        CASE(opc_goto)
	{
	    CVMInt16 offset = CVMgetInt16(pc + 1);
	    TRACE(("\t%s %#x (offset=%d)\n",CVMopnames[*pc],
		   pc + offset, offset));
	    /* %comment h002 */
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(offset, 0);
	}

	/* Allocate memory for a new java object. */

	CASE_NT(opc_new_checkinit_quick) {
	    CVMClassBlock* cb = CVMcpGetCb(cp, GET_INDEX(pc + 1));
	    INIT_CLASS_IF_NEEDED(ee, cb);
	    /* Fall through */
	}

	CASE(opc_new_quick) {
	    CVMObject* directObj;
	    CVMClassBlock* cb = CVMcpGetCb(cp, GET_INDEX(pc + 1));

	    /*
	     * CVMgcAllocNewInstance may block and initiate GC, so we must
	     * de-cache our state for GC first.
	     */
	    DECACHE_PC(frame);
	    DECACHE_TOS(frame);
	    directObj = CVMgcAllocNewInstance(ee, cb);		
	    if (directObj == NULL) {
		goto throwOutOfMemoryError;
	    }
	    CVMID_icellSetDirect(ee, &STACK_ICELL(0), directObj);

	    TRACE(("\tnew_quick %C => 0x%x\n", cb, directObj));

	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
	}

	CASE(opc_invokenonvirtual_quick) {
	    CVMObject* directObj;
	    CVMUint32 mbIndex = GET_INDEX(pc + 1);
	    
	    DECACHE_TOS(frame); /* must include arguments for proper gc */
	    mb = CVMcpGetMb(cp, mbIndex);
	    topOfStack -= CVMmbArgsSize(mb);
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);

	    goto callJavaMethod;
	}

	/* invoke a virtual method of java/lang/Object */
	/* 
	 * NOTE: we can get rid of opc_invokevirtualobject_quick now because
	 * arrays have their own methodTablePtr (they just use Object's
	 * methodTablePtr), and CVMobjMethodTableSlot() has been optimized
	 * to no longer check if it's dealing with an array type, so we
	 * could just as well use opc_invokevirtual_quick.
	 */

	CASE(opc_invokevirtualobject_quick) {
	    CVMObject* directObj;
	    
	    DECACHE_TOS(frame); /* must include arguments for proper gc */
	    topOfStack -= pc[2];
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    mb = CVMobjMethodTableSlot(directObj, pc[1]);

	    goto callJavaMethod;
	}

	/* invoke a virtual method */

	CASE_NT(opc_invokevirtual_quick)
	CASE_NT(opc_ainvokevirtual_quick)
	CASE_NT(opc_dinvokevirtual_quick)
	CASE(opc_vinvokevirtual_quick) {
	    CVMObject* directObj;
	    CVMClassBlock* ocb;
	    CVMInt32 argsAdjust;
	    CVMUint32 methodIndex;
	    CVMMethodBlock** methodTablePtr;

	    argsAdjust = -pc[2]; /* Quick! */
	    methodIndex = pc[1]; /* Quick! */

	    DECACHE_TOS(frame); /* must include arguments for proper gc */

	    directObj = STACK_OBJECT(argsAdjust);
	    DECACHE_PC(frame);

	    CHECK_NULL(directObj);

	    ocb = CVMobjectGetClass(directObj);
	    methodTablePtr = CVMcbMethodTablePtr(ocb);
	    mb = methodTablePtr[methodIndex];
	    topOfStack += argsAdjust;

	    goto callJavaMethod;
	}

	/* invoke a virtual method */

	CASE(opc_invokevirtual_quick_w) {
	    CVMObject* directObj;
	    CVMClassBlock* ocb;
	    CVMInt32 argsAdjust;
	    CVMUint32 methodIndex;
	    CVMMethodBlock** methodTablePtr;
	    
	    mb = CVMcpGetMb(cp, GET_INDEX(pc + 1));
	    DECACHE_TOS(frame); /* must include arguments for proper gc */

	    argsAdjust = -CVMmbArgsSize(mb); /* Quick! */
	    methodIndex = CVMmbMethodTableIndex(mb); /* Quick! */

	    directObj = STACK_OBJECT(argsAdjust);
	    DECACHE_PC(frame);

	    CHECK_NULL(directObj);

	    ocb = CVMobjectGetClass(directObj);
	    methodTablePtr = CVMcbMethodTablePtr(ocb);
	    mb = methodTablePtr[methodIndex];
	    topOfStack += argsAdjust;

	    goto callJavaMethod;
	}

	/* invoke an interface method */

        CASE(opc_invokeinterface_quick) {
	    CVMObject*      directObj;
	    CVMMethodBlock* imb;  /* interface mb for method we are calling */
	    CVMClassBlock*  icb;  /* interface cb for method we are calling */
	    CVMClassBlock*  ocb;  /* cb of the object */
	    CVMUint32 interfaceCount;
	    CVMInterfaces* objectIntfs;
	    CVMInterfaceTable* itablePtr;
	    CVMUint16 methodTableIndex;/* index into ocb's methodTable */
	    int guess = pc[4];

	    imb = CVMcpGetMb(cp, GET_INDEX(pc + 1));

	    DECACHE_TOS(frame); /* must include arguments for proper gc */

	    /* don't use pc[3] because transtion mb's don't set it up */
	    topOfStack -= CVMmbArgsSize(imb);

	    icb = CVMmbClassBlock(imb);

	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    ocb = CVMobjectGetClass(directObj);
	    objectIntfs = CVMcbInterfaces(ocb);
	    interfaceCount = CVMcbInterfaceCountGivenInterfaces(objectIntfs);
	    itablePtr = CVMcbInterfaceItable(objectIntfs);

	    /*
	     * Search the objects interface table, looking for the entry
	     * whose interface cb matches the cb of the inteface we
	     * are invoking.
	     */
	    if (guess < 0 || guess >= interfaceCount || 
		CVMcbInterfacecbGivenItable(itablePtr, guess) != icb) {
		CVMBool containerNotInROM =
		    !CVMisROMPureCode(CVMmbClassBlock(frame->mb));
		
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
		    CVMassert(!CVMframeIsTransition(frame));
		    if (containerNotInROM)
			pc[4] = guess;
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
                    objectIntfs, guess, CVMmbMethodSlotIndex(imb));
            } else if (icb == CVMsystemClass(java_lang_Object)) {
                /*
                 * Per VM spec 5.4.3.4, the resolved interface method may be
                 * a method of java/lang/Object.
                 */ 
                 methodTableIndex = CVMmbMethodTableIndex(imb);
            } else {
                goto throwIncompatibleClassChangeError;
            }

            /*
	     * Fetch the proper mb and it's cb.
	     */
	    mb = CVMcbMethodTableSlot(ocb, methodTableIndex);
	    goto callJavaMethod;
	}

	handleReturn:
	ASMLABEL(label_handleReturn);
	{
#undef RETURN_OPCODE
#if (defined(CVM_JVMTI) || defined(CVM_JVMPI))
#define RETURN_OPCODE return_opcode
	    CVMUint32 return_opcode;
	    /* We might gc, so flush state. */
	    DECACHE_PC(frame);
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
		    DECACHE_PC(frame);
		    if (!CVMsyncReturnHelper(ee, frame, &STACK_ICELL(-1),
					     RETURN_OPCODE == opc_areturn)) {
			goto handle_exception;
		    }
		}
	    }

	    TRACESTATUS(frame);
	    TRACE_METHOD_RETURN(frame);
	    CVMpopFrame(&ee->interpreterStack, frame);
	    CVMassert(frameSanity(frame));

#ifdef CVM_JIT
	    if (CVMframeIsCompiled(frame)) {
		goto return_to_compiled;
	    }
#endif

	    /* 
	     * Restore our state. We need to recache all of the local
	     * variables that got trounced when we made the method call.
	     * topOfStack was restored before handlereturn was entered.
	     */
	    CACHE_PC(frame);   /* restore our pc from before the call */

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

	    /* Increment callers pc based on the callers invoke opcode. */
	    pc += (*pc == opc_invokeinterface_quick ? 5 : 3);

	    /* We don't restore mb for performance reasons. The only
	     * quick opcode that needs it is opc_invokesuper_quick and
	     * we'll get better performance if it handles the
	     * frame->mb overhead instead of doing it on every return.
	     */
	    /* mb = frame->mb;*/

	    TRACESTATUS(frame);
	    
	    CONTINUE;
	} /* handleReturn: */
	
        callJavaMethod:
	ASMLABEL(label_callJavaMethod);
	
#ifdef CVM_JVMPI
        JVMPI_CHECK_FOR_DATA_DUMP_REQUEST(ee);
#endif
	if (CVMmbInvokerIdx(mb) >= CVM_INVOKE_CNI_METHOD) {
	    /*
	     * It's a hard method. Go back to parent to do the call
	     */
	    return pc;
	}

	{
	    /*
	     * It's a Java method. Push the frame and setup our state 
	     * for the new method.
	     */

	    CVMClassBlock*  cb = CVMmbClassBlock(mb);
	    CVMJavaMethodDescriptor* jmd;
	    CVMObjectICell* receiverObjICell = &STACK_ICELL(0);
	    CVMFrame* prev;

	    /* Decache the interpreter state before pushing the
	     * frame.  We leave frame->topOfStack pointing after
	     * the args, so they will get scanned it gc starts up
	     * during the pushFrame.
	     */
	    DECACHE_PC(frame);

#ifdef CVM_JIT
new_mb_from_compiled:
#endif

	    jmd = CVMmbJmd(mb);
	    prev = frame;

	    /* update the new locals value so our pushframe logic
	     * is easier. */
	    locals = &topOfStack->s;

	    TRACE(("\t%s %C.%M\n", CVMopnames[*pc], cb, mb));

	    TRACESTATUS(frame);

#if defined(CVM_JIT)
	    {
		if (CVMmbIsCompiled(mb)) {
		    CVMCompiledResultCode resCode;
invoke_compiled:
		    resCode = CVMinvokeCompiledHelper(ee, frame, &mb);
result_code:
		    switch (resCode) {
		    case CVM_COMPILED_RETURN:
			frame = ee->interpreterStack.currentFrame;
			CACHE_TOS(frame);
			locals = CVMframeLocals(frame);
			cp = CVMframeCp(frame);
			CACHE_PC(frame);
			pc += (*pc == opc_invokeinterface_quick ? 5 : 3);
			CONTINUE;
		    case CVM_COMPILED_NEW_MB:
			frame = ee->interpreterStack.currentFrame;
			CACHE_TOS(frame);
			topOfStack -= CVMmbArgsSize(mb);
			cb = CVMmbClassBlock(mb);
			if (CVMmbIs(mb, STATIC)) {
			    receiverObjICell = CVMcbJavaInstance(cb);
			} else {
			    receiverObjICell = &STACK_ICELL(0);
			}
			CVMassert(CVMframeIsCompiled(frame));
			goto new_mb_from_compiled;
		    case CVM_COMPILED_NEW_TRANSITION:
			/* return to level1 loop */
			*retCb = NULL;
			return NULL;
		    case CVM_COMPILED_EXCEPTION:
			frame = ee->interpreterStack.currentFrame;
			goto handle_exception;
		    default:
			CVMassert(CVM_FALSE);
		    }

		    /* This is kind of a funny location for this goto label,
		     * but it's needed because resCode is only defined in
		     * this block, and we don't want to give it a wider scope.
		     */
return_to_compiled:
		    DECACHE_TOS(frame);
		    resCode = CVMreturnToCompiledHelper(ee, frame, &mb, NULL);
		    goto result_code;
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
	    }
#endif
	    /*
	     * pushFrame() might have to become GC-safe if it needs to 
	     * expand the stack. frame->topOfStack for the current frame 
	     * includes the arguments, and the stackmap does as well, so
	     * that the GC-safe action is really safe.
	     */
	    CVMpushFrame(ee, &ee->interpreterStack, frame, topOfStack,
			 CVMjmdCapacity(jmd), CVMjmdMaxLocals(jmd),
			 CVM_FRAMETYPE_JAVA, mb, CVM_TRUE,
	    /* action if pushFrame fails */
	    {
		CVMassert(frameSanity(ee->interpreterStack.currentFrame));
		/* just return from quick loop */
		goto handle_exception;
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

	    CVMassert(frameSanity(frame));

	   /*
	    * Set up new PC and constant pool
	    */
	    pc = CVMjmdCode(jmd);
	    cp = CVMcbConstantPool(cb);

	    /*
	     * GC-safety stage 2: Update caller frame's topOfStack to
	     * exclude the arguments. Once we update our new frame
	     * state, the callee frame's stackmap will cover the arguments
	     * as incoming locals.  
	     */
	    prev->topOfStack = topOfStack;

	    /* set topOfStack to start of new operand stack */
	    CVM_RESET_JAVA_TOS(topOfStack, frame);

	    /* We have to cache frame->locals if this method ever
	     * calls another method (which we don't know), and in most
	     * cases it's cheaper just to cache it here rather then cache
	     * it every time we make a method call from this method. The
	     * same is true of frame->mb and frame->constantpool.
	     */

	    /*
	     * Set up frame
	     */
	    CVMframeLocals(frame) = locals;

	    CVMframeCp(frame) = cp;

	    /* This is needed to get CVMframe2string() to work properly */
	    CVMdebugExec(DECACHE_PC(frame));
	    TRACE_METHOD_CALL(frame, CVM_FALSE);
	    TRACESTATUS(frame);

	    CVMID_icellAssignDirect(ee,
				    &CVMframeReceiverObj(frame, Java),
				    receiverObjICell);
	    
	    /* %comment c002 */
	    if (CVMmbIs(mb, SYNCHRONIZED)) {
		/* The method is sync, so lock the object. */
		/* %comment l002 */
		if (!CVMfastTryLock(
			ee, CVMID_icellDirect(ee, receiverObjICell))) {
		    goto handleLockFailureInCall;
		}
	    returnFromLockFailureHandling: ;
	    }

#ifdef CVM_JVMPI
	    if (CVMjvmpiEventMethodEntryIsEnabled()) {
		/* Need to decache because JVMPI will become GC safe
		 * when calling the profiler agent: */
		DECACHE_PC(frame);
		DECACHE_TOS(frame);
		CVMjvmpiPostMethodEntryEvent(ee,
					     &CVMframeReceiverObj(frame, Java)
		);
	    }
#endif /* CVM_JVMPI */

#ifdef CVM_JVMTI
	    /* %comment kbr001 */
	    /* Decache all curently uncached interpreter state */
	    DECACHE_PC(frame);
	    DECACHE_TOS(frame);
	    CVMD_gcSafeExec(ee, {
		    CVMjvmtiPostFramePushEvent(ee);
		});
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

	    /*
	     * Offer a GC-safe point. The current frame is always up
	     * to date, so don't worry about de-caching frame.
	     */
	    if (CVMD_gcSafeCheckRequest(ee)) {
		goto doGcThing;
	    }

	    CONTINUE;
	}

        CASE(opc_fcmpl)
	{
	    STACK_INT(-2) = CVMfloatCompare(STACK_FLOAT(-2), 
					    STACK_FLOAT(-1), 
					    -1);
            TRACE(("\t%s => %d\n", CVMopnames[*pc], STACK_INT(-2)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	}

        CASE(opc_fcmpg)
	{
	    STACK_INT(-2) = CVMfloatCompare(STACK_FLOAT(-2), 
					    STACK_FLOAT(-1), 
					    1);
            TRACE(("\t%s => %d\n", CVMopnames[*pc], STACK_INT(-2)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	}

	/* ignore invocation. This is a result of inlining a method
	 * that does nothing.
	 */
#define CVM_INLINING
#ifdef CVM_INLINING
        CASE(opc_invokeignored_quick) {
	    CVMObject* directObj;
	    TRACE(("\tinvokeignored_quick\n"));
	    topOfStack -= pc[1];
	    directObj = STACK_OBJECT(0);
	    if (pc[2]) {
		CHECK_NULL(directObj);
	    }
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 0);
	}
#endif

        CASE_NT(opc_jsr_w) {
	    /* push return address on the stack */
	    STACK_ADDR(0) = pc + 5;
	    topOfStack++;
	    /* FALL THROUGH */
	}

        CASE(opc_goto_w)
	{
	    CVMInt32 offset = CVMgetInt32(pc + 1);
	    TRACE(("\t%s %#x (offset=%d)\n", CVMopnames[*pc],
		   pc + offset, offset));
	    /* %comment h002 */
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_BACKWARDS_CHECK(offset, 0);
	}

	CASE(opc_dup2_x1)	/* insert top double three down */
	    STACK_INFO(1) = STACK_INFO(-1);
	    STACK_INFO(0) = STACK_INFO(-2);
	    STACK_INFO(-1) = STACK_INFO(-3);
	    STACK_INFO(-2) = STACK_INFO(1);
	    STACK_INFO(-3) = STACK_INFO(0);
	    TRACE(("\tdup2_x1\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 2);

	CASE(opc_dup2_x2)	/* insert top double four down */
	    STACK_INFO(1) = STACK_INFO(-1);
	    STACK_INFO(0) = STACK_INFO(-2);
	    STACK_INFO(-1) = STACK_INFO(-3);
	    STACK_INFO(-2) = STACK_INFO(-4);
	    STACK_INFO(-3) = STACK_INFO(1);
	    STACK_INFO(-4) = STACK_INFO(0);
	    TRACE(("\tdup2_x2\n"));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 2);

	OPC_INT2_BINARY(add, Add, 0);
	OPC_INT2_BINARY(sub, Sub, 0);
	OPC_INT2_BINARY(and, And, 0);
	OPC_INT2_BINARY(or,  Or,  0);
	OPC_INT2_BINARY(xor, Xor, 0);
	OPC_INT2_BINARY(div, Div, 1);
	OPC_INT2_BINARY(rem, Rem, 1);

	OPC_INT1_BINARY(div, Div, 1);
	OPC_INT1_BINARY(rem, Rem, 1);

	OPC_FLOAT1_BINARY(div, Div);
	OPC_FLOAT1_BINARY(rem, Rem);

	OPC_FLOAT2_BINARY(add, Add);
	OPC_FLOAT2_BINARY(sub, Sub);
	OPC_FLOAT2_BINARY(mul, Mul);
	OPC_FLOAT2_BINARY(div, Div);
	OPC_FLOAT2_BINARY(rem, Rem);
	
        OPC_SHIFT2_BINARY(shl, Shl);
	OPC_SHIFT2_BINARY(shr, Shr);
	OPC_SHIFT2_BINARY(ushr, Ushr);

	CASE(opc_nop)
	    TRACE(("\t%s\n", CVMopnames[*pc]));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);


	OPC_CONST2_n(dconst_0, Zero, double);
	OPC_CONST2_n(dconst_1, One,  double);
	OPC_CONST2_n(lconst_0, Zero, long);
	OPC_CONST2_n(lconst_1, One,  long);

        CASE(opc_ldc2_w_quick)
 	{
	    CVMJavaVal64 r;
	    CVMcpGetVal64(cp, GET_INDEX(pc+1), r);
	    CVMmemCopy64(&STACK_INFO(0).raw, r.v); 
            TRACE(("\topc_ldc2_w_quick #%d => long=%s double=%g\n", 
		   GET_INDEX(pc + 1),
                   GET_LONGCSTRING(STACK_LONG(0)), STACK_DOUBLE(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 2);
	}


	CASE_NT(opc_lload)
	CASE(opc_dload)
	    CVMmemCopy64(&STACK_INFO(0).raw, &locals[pc[1]].j.raw);
	    TRACEIF(opc_dload, ("\tdload locals[%d](%f) =>\n",
				pc[1], STACK_DOUBLE(0)));
	    TRACEIF(opc_lload, ("\tlload locals[%d](%s) =>\n",
				pc[1], GET_LONGCSTRING(STACK_LONG(0))));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 2);

	OPC_LOAD2_n(l,0);
	OPC_LOAD2_n(d,0);
	    
	OPC_LOAD2_n(l,1);
	OPC_LOAD2_n(d,1);
	    
	OPC_LOAD2_n(l,2);
	OPC_LOAD2_n(d,2);
	    
	OPC_LOAD2_n(l,3);
	OPC_LOAD2_n(d,3);

	CASE_NT(opc_lstore)
	CASE(opc_dstore)
	    CVMmemCopy64(&locals[pc[1]].j.raw, &STACK_INFO(-2).raw);
	    TRACEIF(opc_dstore, ("\tdstore %f => locals[%d]\n",
				 STACK_DOUBLE(-2), pc[1]));
	    TRACEIF(opc_lstore, ("\tlstore %s => locals[%d]\n",
				 GET_LONGCSTRING(STACK_LONG(-2)), pc[1]));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, -2);

	OPC_STORE2_n(d, 0);
	OPC_STORE2_n(l, 0);
	
	OPC_STORE2_n(d, 1);
	OPC_STORE2_n(l, 1);

	OPC_STORE2_n(d, 2);
	OPC_STORE2_n(l, 2);

	OPC_STORE2_n(d, 3);
	OPC_STORE2_n(l, 3);

        CASE(opc_lneg)
	{
 	   CVMJavaLong r;
	   r = CVMjvm2Long(&STACK_INFO(-2).raw);
	   r = CVMlongNeg(r);
	   CVMlong2Jvm(&STACK_INFO(-2).raw, r);
           TRACE(("\t%s => %s\n", CVMopnames[*pc],
		  GET_LONGCSTRING(STACK_LONG(-2))));
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}

        CASE(opc_dneg)
	{
 	   CVMJavaDouble r;
	   r = CVMjvm2Double(&STACK_INFO(-2).raw);
	   r = CVMdoubleNeg(r);
	   CVMdouble2Jvm(&STACK_INFO(-2).raw, r);
           TRACE(("\t%s => %f\n", CVMopnames[*pc], STACK_DOUBLE(-2)));
           UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}

        CASE(opc_i2l)	/* convert top of stack int to long */
	{
	    CVMJavaLong r;
	    r = CVMint2Long(STACK_INT(-1));
	    CVMlong2Jvm(&STACK_INFO(-1).raw, r);
            TRACE(("\ti2l => %s\n", GET_LONGCSTRING(STACK_LONG(-1))));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);
	}

        CASE(opc_l2i)	/* convert top of stack long to int */
	{
   	    CVMJavaLong r; 
	    r = CVMjvm2Long(&STACK_INFO(-2).raw);
	    STACK_INT(-2) = CVMlong2Int(r); 
            TRACE(("\tl2i => %d\n", STACK_INT(-2)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	}
	
        CASE(opc_l2f)   /* convert top of stack long to float */
	{
   	    CVMJavaLong r; 
	    r = CVMjvm2Long(&STACK_INFO(-2).raw);
	    STACK_FLOAT(-2) = CVMlong2Float(r); 
            TRACE(("\tl2f => %f\n", STACK_FLOAT(-2)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	}

        CASE(opc_f2l)  /* convert top of stack float to long */
	{
	    CVMJavaLong r;
 	    r = CVMfloat2Long(STACK_FLOAT(-1));
	    CVMlong2Jvm(&STACK_INFO(-1).raw, r);
            TRACE(("\tf2l => %s\n", GET_LONGCSTRING(STACK_LONG(-1))));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);
	}

        CASE(opc_f2d)  /* convert top of stack float to double */
	{
	    CVMJavaDouble r;
	    r = CVMfloat2Double(STACK_FLOAT(-1));
	    CVMdouble2Jvm(&STACK_INFO(-1).raw, r);
            TRACE(("\tf2d => %f\n", STACK_DOUBLE(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);
        }

        CASE(opc_d2f) /* convert top of stack double to float */
   	{
	    CVMJavaDouble r;
	    r = CVMjvm2Double(&STACK_INFO(-2).raw);
	    STACK_FLOAT(-2) = CVMdouble2Float(r);
            TRACE(("\td2f => %f\n", STACK_FLOAT(-2)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	}

        CASE(opc_dcmpl)
	{
 	    CVMJavaDouble value1, value2; 
            value1 = CVMjvm2Double(&STACK_INFO(-4).raw);
            value2 = CVMjvm2Double(&STACK_INFO(-2).raw);
	    STACK_INT(-4) = CVMdoubleCompare(value1, value2, -1);
            TRACE(("\t%s => %d\n", CVMopnames[*pc], STACK_INT(-4)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -3);
	}

        CASE(opc_dcmpg)
	{
 	    CVMJavaDouble value1, value2; 
            value1 = CVMjvm2Double(&STACK_INFO(-4).raw);
            value2 = CVMjvm2Double(&STACK_INFO(-2).raw);
	    STACK_INT(-4) = CVMdoubleCompare(value1, value2, 1);
            TRACE(("\t%s => %d\n", CVMopnames[*pc], STACK_INT(-4)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -3);
	}

	CASE(opc_getstatic2_quick) {
	    CVMUint32 cpIndex = GET_INDEX(pc + 1);
	    CVMFieldBlock* fb = CVMcpGetFb(cp, cpIndex);
	    JVMTI_WATCHING_FIELD_ACCESS(NULL)
	    CVMmemCopy64(&STACK_INFO(0).raw, &CVMfbStaticField(ee, fb).raw);
	    TRACE(("\t%s #%d %C.%F[%d] ((0x%X,0x%X) ==>\n",
		   CVMopnames[*pc], GET_INDEX(pc+1), CVMfbClassBlock(fb), fb,
		   CVMfbOffset(fb), STACK_INT(0), STACK_INT(1)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 2);
	}

	CASE(opc_putstatic2_quick) {
	    CVMUint32 cpIndex = GET_INDEX(pc + 1);
	    CVMFieldBlock* fb = CVMcpGetFb(cp, cpIndex);
	    JVMTI_WATCHING_FIELD_MODIFICATION(NULL, CVM_TRUE, CVM_FALSE);
	    CVMmemCopy64(&CVMfbStaticField(ee, fb).raw, &STACK_INFO(-2).raw);
	    TRACE(("\t%s #%d (0x%X,0x%X) ==> %C.%F[%d]\n",
		   CVMopnames[*pc], GET_INDEX(pc+1), 
		   STACK_INT(-2), STACK_INT(-1),
		   CVMfbClassBlock(fb), fb, CVMfbOffset(fb)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, -2);
	}

	CASE_NT(opc_getstatic2_checkinit_quick) {
	    CVMUint32 cpIndex = GET_INDEX(pc + 1);
	    CVMFieldBlock* fb = CVMcpGetFb(cp, cpIndex);
	    CVMClassBlock* cb = CVMfbClassBlock(fb);
	    INIT_CLASS_IF_NEEDED(ee, cb);
	    JVMTI_WATCHING_FIELD_ACCESS(NULL)
	    CVMmemCopy64(&STACK_INFO(0).raw, &CVMfbStaticField(ee, fb).raw);
	    TRACE(("\t%s #%d %C.%F[%d] ((0x%X,0x%X) ==>\n",
		   CVMopnames[*pc], GET_INDEX(pc+1), cb, fb,
		   CVMfbOffset(fb), STACK_INT(0), STACK_INT(1)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 2);
	}

	CASE_NT(opc_putstatic2_checkinit_quick) {
	    CVMUint32 cpIndex = GET_INDEX(pc + 1);
	    CVMFieldBlock* fb = CVMcpGetFb(cp, cpIndex);
	    CVMClassBlock* cb = CVMfbClassBlock(fb);
	    INIT_CLASS_IF_NEEDED(ee, cb);
	    JVMTI_WATCHING_FIELD_MODIFICATION(NULL, CVM_TRUE, CVM_FALSE);
	    CVMmemCopy64(&CVMfbStaticField(ee, fb).raw, &STACK_INFO(-2).raw);
	    TRACE(("\t%s #%d (0x%X,0x%X) ==> %C.%F[%d]\n",
		   CVMopnames[*pc], GET_INDEX(pc+1), 
		   STACK_INT(-2), STACK_INT(-1),
		   cb, fb, CVMfbOffset(fb)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, -2);
	}

#ifndef CVM_NO_LOSSY_OPCODES
        CASE(opc_getfield2_quick)
	{
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CHECK_NULL(directObj);
	    CVMD_fieldRead64(directObj, pc[1], &STACK_INFO(-1));
	    TRACE(("\t%s %O[%d](0x%X, 0x%X) ==>\n", CVMopnames[*pc],
		   directObj, pc[1], STACK_INT(-1), STACK_INT(0)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
        }

        CASE(opc_putfield2_quick)
	{
	    CVMObject* directObj = STACK_OBJECT(-3);
	    CHECK_NULL(directObj);
	    CVMD_fieldWrite64(directObj, pc[1], &STACK_INFO(-2));
	    TRACE(("\t%s (0x%X, 0x%X) ==> %O[%d]\n", CVMopnames[*pc],
		   STACK_INT(-1), STACK_INT(0), directObj, pc[1]));
            UPDATE_PC_AND_TOS_AND_CONTINUE(3, -3);
        }
#endif /* CVM_NO_LOSSY_OPCODES */

        CASE(opc_laload)
	    ARRAY_LOADTO64(Long,long);
        CASE(opc_daload)
	    ARRAY_LOADTO64(Double,double);

        CASE(opc_lastore)
	    ARRAY_STOREFROM64(Long);
        CASE(opc_dastore)
	    ARRAY_STOREFROM64(Double);

	/* return from a jsr or jsr_w */

        CASE(opc_ret) {
	    /* %comment h002 */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	    TRACE(("\tret %d (%#x)\n", pc[1], locals[pc[1]].a));
	    pc = locals[pc[1]].a;
	    UPDATE_PC_AND_TOS_AND_CONTINUE(0, 0);
	}

	/* debugger breakpoint */
        CASE(opc_breakpoint) {
#ifdef CVM_JVMTI
	    CVMUint32 opcode0;
	    
	    TRACE(("\tbreakpoint\n"));
	    DECACHE_PC(frame);
	    DECACHE_TOS(frame);
	    CVMD_gcSafeExec(ee, {
		opcode0 = CVMjvmtiGetBreakpointOpcode(ee, pc, CVM_TRUE);
	    });
	    REEXECUTE_BYTECODE(opcode0);
#else
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);    /* treat as opc_nop */
#endif
	}

        CASE(opc_newarray) {
	    CVMJavaInt         arrLen;
	    CVMBasicType       typeCode;
	    CVMClassBlock*     arrCb;
	    CVMArrayOfAnyType* directArr;

	    arrLen  = STACK_INT(-1);
	    if (arrLen < 0) {
		goto throwNegativeArraySizeException;
	    }
	    typeCode = pc[1];
	    arrCb    = (CVMClassBlock*)CVMbasicTypeArrayClassblocks[typeCode];
	    CVMassert(arrCb != 0);

	    /*
	     * CVMgcAllocNewArray may block and initiate GC, so we must
	     * de-cache our state for GC first.
	     */
	    DECACHE_PC(frame);
	    DECACHE_TOS(frame);

	    directArr = (CVMArrayOfAnyType*)
		CVMgcAllocNewArray(ee, typeCode, arrCb, arrLen);
	    if (directArr == NULL) {
		goto throwOutOfMemoryError;
	    }
	    CVMID_icellSetDirect(ee, &STACK_ICELL(-1), (CVMObject*)directArr);
	    /*
	     * Make sure we created the right beast
	     */
	    CVMassert(CVMisArrayClassOfBasicType(CVMobjectGetClass(directArr),
						 CVMbasicTypeID[typeCode]));

	    TRACE(("\tnewarray %C.%d => 0x%x\n", arrCb, arrLen, directArr));

	    UPDATE_PC_AND_TOS_AND_CONTINUE(2, 0);
	}

        CASE(opc_anewarray_quick) {
	    CVMJavaInt     arrLen;
	    CVMClassBlock* arrCb;
	    CVMClassBlock* cb;
	    CVMArrayOfRef* directArr;

	    arrLen  = STACK_INT(-1);
	    if (arrLen < 0) {
		goto throwNegativeArraySizeException;
	    }
	    /*
	     * This doesn't have to run static initializers, so we
	     * don't have to do INIT_CLASS_IF_NEEDED()
	     */
	    cb      = CVMcpGetCb(cp, GET_INDEX(pc + 1));

	    /*
	     * CVMclassGetArrayOf may block. CVMgcAllocNewArrayOf may block
	     * and initiate GC. So we must de-cache our state for GC
	     * first.  
	     */
	    DECACHE_PC(frame);
	    DECACHE_TOS(frame);

	    CVMD_gcSafeExec(ee, {
		arrCb = CVMclassGetArrayOf(ee, cb);
	    });
	    if (arrCb == NULL) {
		goto handle_exception;
	    }

	    directArr = (CVMArrayOfRef*)
		CVMgcAllocNewArray(ee, CVM_T_CLASS, arrCb, arrLen);
	    if (directArr == NULL) {
		goto throwOutOfMemoryError;
	    }
	    CVMID_icellSetDirect(ee, &STACK_ICELL(-1), (CVMObject*)directArr);

	    TRACE(("\tanewarray_quick %C => 0x%x\n", arrCb, directArr));

	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 0);
	}

        CASE(opc_monitorenter) {
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CHECK_NULL(directObj);
	    TRACE(("\topc_monitorenter(%O)\n", directObj));
	    /* %comment l002 */
	    if (CVMfastTryLock(ee, directObj)) {
		UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	    }
	    /* non-blocking attempt failed... */
	    goto monitorEnterLong;
	}

        CASE(opc_monitorexit) {
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CHECK_NULL(directObj);
            TRACE(("\topc_monitorexit(%O)\n", directObj));
            /* %comment l003 */
	    if (CVMfastTryUnlock(ee, directObj)) {
		UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	    }
	    /* non-blocking attempt failed... */
	    goto monitorExitLong;
	}

	/* Goto pc at specified offset in switch table. */

	CASE_NT(opc_tableswitch) {
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
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_GC_CHECK(skip, -1);
	}

        /* Goto pc whose table entry matches specified key */

	CASE_NT(opc_lookupswitch) {
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
	    UPDATE_PC_AND_TOS_AND_CONTINUE_WITH_GC_CHECK(skip, -1);
	}

	/*
	 * Lossy opcodes in non-lossy mode
	 */
#ifdef CVM_NO_LOSSY_OPCODES
        CASE_NT(opc_getfield_quick)
        CASE_NT(opc_putfield_quick)
        CASE_NT(opc_getfield2_quick)
        CASE_NT(opc_putfield2_quick)
	CASE_NT(opc_agetfield_quick)
	CASE_NT(opc_aputfield_quick)
#endif

	/*
	 * Infrequently executed
	 */
        CASE_NT(opc_getfield_quick_w)
        CASE_NT(opc_putfield_quick_w)
	CASE_NT(opc_d2l)
	CASE_NT(opc_d2i)
	CASE_NT(opc_l2d)
	CASE_NT(opc_i2d)
	CASE_NT(opc_aldc_ind_w_quick)
	CASE_NT(opc_aldc_w_quick)
	CASE_NT(opc_ldc_w_quick)
	CASE_NT(opc_multianewarray_quick)

	/*
	 * "Quickenables"
	 */
        CASE_NT(opc_invokevirtual)
        CASE_NT(opc_invokespecial)
	CASE_NT(opc_putfield)
	CASE_NT(opc_getfield)
        CASE_NT(opc_invokestatic)
	CASE_NT(opc_getstatic)
        CASE_NT(opc_putstatic)
        CASE_NT(opc_new)
	CASE_NT(opc_invokeinterface)
        CASE_NT(opc_anewarray)
        CASE_NT(opc_multianewarray)
        CASE_NT(opc_checkcast)
	CASE_NT(opc_instanceof)
        CASE_NT(opc_ldc)
        CASE_NT(opc_ldc_w)
	CASE_NT(opc_ldc2_w)

	/*
	 * Hard returns
	 */
	CASE_NT(opc_lreturn)
	CASE_NT(opc_dreturn)

#if 0
	CASE_NT(opc_areturn)
	CASE_NT(opc_ireturn)
	CASE_NT(opc_freturn)
	CASE_NT(opc_return)
#endif

	/*
	 * Uncommon invocations
	 */

#if DONT_DO_OBJECT_INVOCATIONS_HERE
	CASE_NT(opc_invokevirtual_quick)
	CASE_NT(opc_ainvokevirtual_quick)
	CASE_NT(opc_dinvokevirtual_quick)
	CASE_NT(opc_vinvokevirtual_quick)
	CASE_NT(opc_invokevirtualobject_quick)
	CASE_NT(opc_invokenonvirtual_quick)
        CASE_NT(opc_invokeinterface_quick)
#endif

	CASE_NT(opc_invokestatic_quick)
	CASE_NT(opc_invokestatic_checkinit_quick)
	CASE_NT(opc_invokesuper_quick)

	    /* Transitions */
	CASE_NT(opc_exittransition) 

	    /* Register heavy ops */
	CASE_NT(opc_lmul)
	CASE_NT(opc_lcmp)

	/*
	 * Very uncommon ops
	 */
	CASE_NT(opc_wide)

	ASMLABEL(label_return_to_caller);
	{
	    DECACHE_TOS(frame);
	    return pc;
	}

#ifndef CVM_INLINING
	CASE_NT(opc_invokeignored_quick)
#endif
        CASE(opc_xxxunusedxxx)
	DEFAULT
	    CVMdebugPrintf(("\t*** Unimplemented opcode: %d = %s\n",
		   *pc, CVMopnames[*pc]));
	return NULL; /* Crash in caller */

#ifndef CVM_USELABELS
	} /* switch(opc) */
#endif
	/* Common exceptions */
    throwNullPointerException:
	ASMLABEL(label_throwNullPointerException);

        CVM_JAVA_ERROR(NullPointerException, NULL);

    throwArrayIndexOutOfBoundsException:
	ASMLABEL(label_throwArrayIndexOutOfBoundsException);
    
	CVM_JAVA_ERROR(ArrayIndexOutOfBoundsException, NULL);

    throwClassCastException:
	ASMLABEL(label_throwClassCastException);
	/* CVMgcUnsafeIsInstanceOf() may have thrown StackOverflowError */
	if (!CVMlocalExceptionOccurred(ee)) {
	    CVM_JAVA_ERROR(ClassCastException, NULL);
	} else {
	    goto handle_exception;
	}
	

    throwArithmethicException:
	ASMLABEL(label_throwArithmeticException);
	
	CVM_JAVA_ERROR(ArithmeticException, "/ by zero");

    throwOutOfMemoryError:
	ASMLABEL(label_throwOutOfMemoryError);
	
	CVM_JAVA_ERROR(OutOfMemoryError, NULL);

    throwNegativeArraySizeException:
	ASMLABEL(label_throwNegativeArraySizeException);
	
	CVM_JAVA_ERROR(NegativeArraySizeException, NULL);

    throwIllegalMonitorStateException:
	ASMLABEL(label_throwIllegalMonitorStateException);
   
        CVM_JAVA_ERROR(IllegalMonitorStateException,
		       "current thread not owner");

    throwIncompatibleClassChangeError:
	ASMLABEL(label_throwIncompatibleClassChangeError);
   
        CVM_JAVA_ERROR(IncompatibleClassChangeError, NULL);

    throwArrayStoreException:
        ASMLABEL(label_throwArrayStoreException);
	
	CVM_JAVA_ERROR(ArrayStoreException, NULL);

    handle_pending_request: {
	ASMLABEL(label_handle_pending_request);
#ifdef CVM_REMOTE_EXCEPTIONS_SUPPORTED
	if (CVMD_gcSafeCheckRequest(ee))
#endif
	{
    doGcThing:
	    CVMD_gcSafeCheckPoint(ee, {
		DECACHE_PC(frame);    /* only needed if we gc */
		DECACHE_TOS(frame);   /* only needed if we gc */
	    }, {});
	    /*
	     * Re-execute
	     */
	    REEXECUTE_BYTECODE(*pc);
	}
#ifdef CVM_REMOTE_EXCEPTIONS_SUPPORTED
	else {
	    CVMassert(CVMremoteExceptionOccurred(ee));
	    /* Flush our state. */
	    DECACHE_PC(frame);
	    DECACHE_TOS(frame);
	    goto handle_exception;
	}
#endif
    } /* handle_pending_request: */
	
    monitorEnterLong:
	ASMLABEL(label_monitorEnterLong);
	DECACHE_PC(frame);
	DECACHE_TOS(frame);
	if (!CVMobjectLock(ee, &STACK_ICELL(-1))) {
	    goto throwOutOfMemoryError;
	}
	UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	
    monitorExitLong:
	ASMLABEL(label_monitorExitLong);
	DECACHE_PC(frame);
	DECACHE_TOS(frame);
	if (!CVMobjectUnlock(ee, &STACK_ICELL(-1))) {
	    goto throwIllegalMonitorStateException;
	}
	UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	
    handleLockFailureInCall:
	ASMLABEL(label_handleLockFailureInCall);
	{
	    DECACHE_PC(frame);
	    DECACHE_TOS(frame);
	    if (!CVMobjectLock(ee, &CVMframeReceiverObj(frame, Java))) {
		/* pop the java frame */
		CVMpopFrame(&ee->interpreterStack, frame); 
		CVMassert(frameSanity(frame));
		goto throwOutOfMemoryError;
	    }
	    goto returnFromLockFailureHandling;
	}

    handle_exception:
	ASMLABEL(label_handle_exception);
	{
	    /* Don't care about top of stack on exception cases */
	    return pc;
	}

#ifndef CVM_USELABELS
    } /* while (1) interpreter loop */
#endif
}
