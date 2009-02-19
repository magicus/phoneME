/*
 * @(#)executejava_split1.c	1.75 06/10/25
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
#define TRACESTATUS()							    \
    CVMtraceStatus(("stack=0x%x frame=0x%x locals=0x%x tos=0x%x pc=0x%x\n", \
		    stack, frame, locals, topOfStack, pc));

#ifdef CVM_TRACE
#define TRACEIF(o, a)				\
    if (opcode == o) {				\
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
    DECACHE_PC();	/* required for CVMsignalError */	\
    CVM_RESET_INTERPRETER_TOS(frame->topOfStack, frame,	        \
			      CVMframeIsJava(frame));		\
    CVMthrow##exception_(ee, error_);				\
    goto handle_exception;					\
}

/* 
 * INIT_CLASS_IF_NEEDED - Makes sure that static initializers are
 * run for a class.  Used in opc_*_checkinit_quick opcodes.
 */
#undef INIT_CLASS_IF_NEEDED
#define INIT_CLASS_IF_NEEDED(ee, cb)		\
    if (CVMcbInitializationNeeded(cb, ee)) {	\
	goto init_class;			\
    }						\
    JVMPI_TRACE_INSTRUCTION();

/*
 * Check that the returned PC and TOS from the second level are in line
 * with the current frame 
 */
#ifdef CVM_DEBUG_ASSERTS
static CVMBool
inMethod(CVMFrame* frame, CVMUint8* pc)
{
    CVMassert(CVMframeIsInterpreter(frame));
    if (CVMframeIsJava(frame)) {
	CVMJavaMethodDescriptor* jmd = CVMmbJmd(frame->mb);
	CVMUint8* code = CVMjmdCode(jmd);
	CVMUint8* codeEnd = CVMjmdCode(jmd) + CVMjmdCodeLength(jmd);
	return ((pc >= code) && (pc < codeEnd));
    } else {
	return CVM_TRUE;
    }
}

static CVMBool
inFrame(CVMFrame* frame, CVMStackVal32* tos)
{
    CVMassert(CVMframeIsInterpreter(frame));
    if (CVMframeIsJava(frame)) {
	CVMJavaMethodDescriptor* jmd = CVMmbJmd(frame->mb);
	CVMStackVal32* base = CVMframeOpstack(frame, Java);
	CVMStackVal32* top  = base + CVMjmdMaxStack(jmd);
	return ((tos >= base) && (tos <= top));
    } else {
	return CVM_TRUE;
    }
}

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
#define JVMTI_SINGLE_STEPPING()					   \
{									\
    if (ee->jvmtiSingleStepping && CVMjvmtiThreadEventsEnabled(ee)) {	\
	DECACHE_PC();						   	\
	DECACHE_TOS();						   	\
	CVMD_gcSafeExec(ee, {					   	\
	    CVMjvmtiPostSingleStepEvent(ee, pc);		   	\
	});								\
	/* Refetch opcode. See above. */				\
	opcode = *pc;						   	\
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
	       CVMjvmtiPostFieldModificationEvent(ee, location, fb, val);     \
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
#define CASE(opcode)                            \
    PRIVATE_CASE(opcode, {                      \
        JVMPI_TRACE_INSTRUCTION();              \
    })

/* CASE_NT (no trace), in contrast with the CASE() macro, does not post a
 * JVMPI_EVENT_INSTRUCTION_START event.  This is used where special handling
 * is needed to post the JVMPI event, or when the case falls through to
 * another case immediately below it which will post the event. */
#undef CASE_NT
#define CASE_NT(opcode)                         \
    PRIVATE_CASE(opcode, {})

/*
 * CONTINUE - Macro for executing the next opcode.
 */
#undef CONTINUE
#ifdef CVM_USELABELS
#define CONTINUE {				\
        opcode = *pc;				\
	UPDATE_INSTRUCTION_COUNT(opcode);	\
        JVMTI_SINGLE_STEPPING();		\
	goto *opclabels[opcode];		\
    }
#else
#ifdef CVM_PREFETCH_OPCODE
#define CONTINUE {				\
        opcode = *pc; 				\
	UPDATE_INSTRUCTION_COUNT(opcode);	\
	JVMTI_SINGLE_STEPPING();		\
	continue;				\
    }
#else
#define CONTINUE {				\
        UPDATE_INSTRUCTION_COUNT(opcode);	\
        JVMTI_SINGLE_STEPPING();		\
        continue;				\
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
	goto opcode_switch;			\
    }
#else
#define REEXECUTE_BYTECODE(opc) {		\
	opcode = (opc);				\
	goto opcode_switch;			\
    }
#endif
#endif

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
 * UPDATE_PC_AND_TOS - Macro for updating the pc and topOfStack.
 */
#undef UPDATE_PC_AND_TOS
#define UPDATE_PC_AND_TOS(opsize, stack) \
    {pc += opsize; topOfStack += stack;}

/*
 * UPDATE_PC_AND_TOS_AND_CONTINUE - Macro for updating the pc and topOfStack,
 * and executing the next opcode. It's somewhat similar to the combination
 * of UPDATE_PC_AND_TOS and CONTINUE, but with some minor optimizations.
 */
#undef UPDATE_PC_AND_TOS_AND_CONTINUE
#ifdef CVM_USELABELS
#define UPDATE_PC_AND_TOS_AND_CONTINUE(opsize, stack) {		\
	opcode = pc[opsize]; pc += opsize; topOfStack += stack;	\
	UPDATE_INSTRUCTION_COUNT(opcode); 			\
        JVMTI_SINGLE_STEPPING();				\
        goto *opclabels[opcode];				\
    }
#else
#ifdef CVM_PREFETCH_OPCODE
#define UPDATE_PC_AND_TOS_AND_CONTINUE(opsize, stack) {		\
	opcode = pc[opsize]; pc += opsize; topOfStack += stack;	\
	UPDATE_INSTRUCTION_COUNT(opcode);			\
	JVMTI_SINGLE_STEPPING();				\
	continue;						\
    }
#else
#define UPDATE_PC_AND_TOS_AND_CONTINUE(opsize, stack) {	\
	pc += opsize; topOfStack += stack;		\
	UPDATE_INSTRUCTION_COUNT(opcode);		\
	JVMTI_SINGLE_STEPPING();			\
	continue;					\
    }
#endif
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
#undef DECACHE_FRAME
#define CACHE_FRAME()     frame = stack->currentFrame;
#define DECACHE_FRAME()   stack->currentFrame = frame;
 
/*
 * CHECK_NULL - Macro for throwing a NullPointerException if the object
 * passed is a null ref.
 */
#undef CHECK_NULL
#define CHECK_NULL(obj_) 				\
    if ((obj_) == 0) {   				\
        goto throwNullPointerException;		        \
    }

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
    CVMUint32         opcode;
    CVMConstantPool*  cp;
    CVMTransitionConstantPool   transitioncp;
    CVMClassBlock*    cb;
    CVMObjectICell*   receiverObjICell; /* the object we are dispatching on or
					   the Class object for static calls */
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
#endif /* CVM_USELABELS */

#undef  OPC_INT2_BINARY 
#define OPC_INT2_BINARY(opcname, opname)				\
	CASE(opc_l##opcname)						\
	{								\
	    CVMJavaLong l1, l2, r1;					\
	    l2 = CVMjvm2Long(&STACK_INFO(-2).raw);			\
	    l1 = CVMjvm2Long(&STACK_INFO(-4).raw);			\
	    r1 = CVMlong##opname(l1, l2);				\
	    CVMlong2Jvm(&STACK_INFO(-4).raw, r1);			\
	    TRACE(("\t%s => %s\n", CVMopnames[*pc],			\
		   GET_LONGCSTRING(STACK_LONG(-4))));		       	\
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -2);			\
	}

    /* C stack redzone check */
    if (!CVMCstackCheckSize(ee, CVM_REDZONE_ILOOP, 
        "CVMgcUnsafeExecuteJavaMethod", CVM_TRUE)) {
	return;
    }

    CVMassert(CVMD_isgcUnsafe(ee));

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
  
#ifdef CVM_PREFETCH_OPCODE
    opcode = *pc;  /* prefetch first opcode */
#endif

    TRACESTATUS();

#ifndef CVM_USELABELS
    while (1)
#endif
    {
#ifndef CVM_PREFETCH_OPCODE
	opcode = *pc;
#endif
	/* Using this label avoids double breakpoints when quickening and
	 * when returing from transition frames.
	 */
    opcode_switch:
    ASMLABEL(label_opcode_switch);
#ifdef CVM_USELABELS
	goto *opclabels[opcode];
#else
	switch (opcode)
#endif
	{
	OPC_INT2_BINARY(mul, Mul); /* lmul */
        CASE(opc_lcmp)
	{
 	    CVMJavaLong value1, value2; 
            value1 = CVMjvm2Long(&STACK_INFO(-4).raw);
            value2 = CVMjvm2Long(&STACK_INFO(-2).raw);
	    STACK_INT(-4) = CVMlongCompare(value1, value2);
            TRACE(("\t%s => %d\n", CVMopnames[opcode], 
				   STACK_INT(-4)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, -3);
	}

        CASE(opc_d2l) /* convert top of stack double to long */
	{
	    CVMJavaDouble r1;
	    CVMJavaLong r2;
	    r1 = CVMjvm2Double(&STACK_INFO(-2).raw);
	    r2 = CVMdouble2Long(r1);
	    CVMlong2Jvm(&STACK_INFO(-2).raw, r2);
            TRACE(("\td2l => %s\n", GET_LONGCSTRING(STACK_LONG(-2))));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}

        CASE(opc_d2i) /* convert top of stack double to int */
   	{
	    CVMJavaDouble r1;
	    CVMJavaInt r2;
	    r1 = CVMjvm2Double(&STACK_INFO(-2).raw);
	    r2 = CVMdouble2Int(r1);
	    STACK_INT(-2) = r2;
            TRACE(("\td2i => %d\n", STACK_INT(-2)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, -1);
	}

        CASE(opc_i2d)	/* convert top of stack int to double */
	{
	    CVMJavaDouble r;
	    r = CVMint2Double(STACK_INT(-1));
	    CVMdouble2Jvm(&STACK_INFO(-1).raw, r);
            TRACE(("\ti2d => %f\n", STACK_DOUBLE(-1)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 1);
	}

        CASE(opc_l2d)	/* convert top of stack long to double */
	{
   	    CVMJavaLong r1; 
	    CVMJavaDouble r2;
	    r1 = CVMjvm2Long(&STACK_INFO(-2).raw);
	    r2 = CVMlong2Double(r1); 
	    CVMdouble2Jvm(&STACK_INFO(-2).raw, r2);
            TRACE(("\tl2d => %f\n", STACK_DOUBLE(-2)));
            UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);
	}

        CASE(opc_multianewarray_quick) {
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
			goto throwNegativeArraySizeException;
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
		goto throwOutOfMemoryError;
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

        CASE(opc_getfield_quick_w)
	{
	    CVMFieldBlock* fb;
	    CVMObject* directObj = STACK_OBJECT(-1);
	    CHECK_NULL(directObj);
	    fb = CVMcpGetFb(cp, GET_INDEX(pc+1));
	    JVMTI_WATCHING_FIELD_ACCESS(&STACK_ICELL(-1))
	    if (CVMfbIsDoubleWord(fb)) {
		CVMD_fieldRead64(directObj, CVMfbOffset(fb), &STACK_INFO(-1));
		TRACE(("\t%s %O[%d](0x%X, 0x%X) ==>\n", CVMopnames[*pc],
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
		TRACE(("\t%s %O[%d](0x%X) ==>\n", CVMopnames[*pc],
		       directObj, GET_INDEX(pc+1), STACK_INT(-1)));
		UPDATE_PC_AND_TOS_AND_CONTINUE(3, 0);
	    }
        }

        CASE(opc_putfield_quick_w)
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
		TRACE(("\t%s (0x%X, 0x%X) ==> %O[%d]\n", CVMopnames[*pc],
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
		TRACE(("\t%s (0x%X) ==> %O[%d]\n", CVMopnames[*pc],
		       STACK_INT(-1), directObj, GET_INDEX(pc+1)));
		UPDATE_PC_AND_TOS_AND_CONTINUE(3, -2);
	    }
        }

	CASE(opc_aldc_ind_w_quick) { /* Indirect String (loaded classes) */
	    CVMObjectICell* strICell =
		CVMcpGetStringICell(cp, GET_INDEX(pc + 1));
	    CVMID_icellAssignDirect(ee, &STACK_ICELL(0), strICell);
            TRACE(("\t%s #%d => 0x%x\n", CVMopnames[*pc],
		   pc[1], STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);
	}

        CASE_NT(opc_aldc_w_quick) /* aldc_w loads direct String refs (ROM only) */
        CASE(opc_ldc_w_quick)
            STACK_INFO(0) = CVMcpGetVal32(cp, GET_INDEX(pc + 1));
            TRACE(("\t%s #%d => 0x%x\n", CVMopnames[*pc],
		   GET_INDEX(pc + 1), STACK_INT(0)));
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 1);

        CASE(opc_wide) {
            CVMUint16 reg = GET_INDEX(pc + 2);

	    opcode = pc[1];
            switch(opcode) {
                case opc_aload:
                case opc_iload:
                case opc_fload:
                    STACK_SLOT(0) = locals[reg]; 
                    TRACEIF(opc_iload, ("\tiload_w locals[%d](%d) =>\n", 
					reg, STACK_INT(0)));
                    TRACEIF(opc_aload, ("\taload_w locals[%d](0x%x) =>\n", 
					reg, STACK_OBJECT(0)));
                    TRACEIF(opc_fload, ("\tfload_w locals[%d](%f) =>\n",
                            	        reg, STACK_FLOAT(0)));
            	    UPDATE_PC_AND_TOS_AND_CONTINUE(4, 1);
                case opc_lload:
                case opc_dload:
	  	    CVMmemCopy64(&STACK_INFO(0).raw, &locals[reg].j.raw);
                    TRACEIF(opc_lload, ("\tlload_w locals[%d](%s) =>\n",
                            		reg, GET_LONGCSTRING(STACK_LONG(0))));
                    TRACEIF(opc_dload, ("\tdload_w locals[%d](%f) =>\n",
                            		reg, STACK_DOUBLE(0)));
            	    UPDATE_PC_AND_TOS_AND_CONTINUE(4, 2);
                case opc_istore:
                case opc_astore:
                case opc_fstore:
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
		    CVMmemCopy64(&locals[reg].j.raw, &STACK_INFO(-2).raw);
                    TRACEIF(opc_lstore, ("\tlstore_w %s => locals[%d]\n",
                                 GET_LONGCSTRING(STACK_LONG(-2)), reg));
                    TRACEIF(opc_dstore, ("\tdstore_w %f => locals[%d]\n",
                                 STACK_DOUBLE(-2), reg));
            	    UPDATE_PC_AND_TOS_AND_CONTINUE(4, -2);
                case opc_iinc: {
		    CVMInt16 offset = CVMgetInt16(pc+4); 
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
            	    UPDATE_PC_AND_TOS_AND_CONTINUE(0, 0);
                default:
                    CVMassert(CVM_FALSE);
            }
        }

	/* invoke a static method */

	CASE(opc_invokestatic_quick) {
	    mb = CVMcpGetMb(cp, GET_INDEX(pc + 1));
	    cb = CVMmbClassBlock(mb);
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    topOfStack -= CVMmbArgsSize(mb);
	    receiverObjICell = CVMcbJavaInstance(cb);
	    goto callmethod;
	}
 
	/* invoke a romized static method that has a static initializer */

	CASE_NT(opc_invokestatic_checkinit_quick) {
	    mb = CVMcpGetMb(cp, GET_INDEX(pc + 1));
	    cb = CVMmbClassBlock(mb);
	    INIT_CLASS_IF_NEEDED(ee, cb);
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    topOfStack -= CVMmbArgsSize(mb);
	    receiverObjICell = CVMcbJavaInstance(cb);
	    goto callmethod;
	}
  
	/* invoke a virtual method */

	CASE_NT(opc_invokevirtual_quick)
	CASE_NT(opc_ainvokevirtual_quick)
	CASE_NT(opc_dinvokevirtual_quick)
	CASE(opc_vinvokevirtual_quick) {
	    CVMObject* directObj;
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    topOfStack -= pc[2];
	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    mb = CVMcbMethodTableSlot(CVMobjectGetClass(directObj), pc[1]);
	    cb = CVMmbClassBlock(mb);
	    goto callmethod;
	}

	CASE(opc_invokevirtual_quick_w) {
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
	    cb = CVMmbClassBlock(mb);
	    goto callmethod;
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
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    topOfStack -= pc[2];
	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    mb = CVMobjMethodTableSlot(directObj, pc[1]);
	    cb = CVMmbClassBlock(mb);
	    goto callmethod;
	}

	/* invoke a nonvirtual method */

	CASE(opc_invokenonvirtual_quick) {
	    CVMObject* directObj;
	    mb = CVMcpGetMb(cp, GET_INDEX(pc + 1));
	    cb = CVMmbClassBlock(mb);
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    topOfStack -= CVMmbArgsSize(mb);
	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    goto callmethod;
	}

	/* invoke a superclass method */

	CASE(opc_invokesuper_quick) {
	    CVMObject* directObj;
	    cb = CVMmbClassBlock(frame->mb);
	    mb = CVMcbMethodTableSlot(CVMcbSuperclass(cb), GET_INDEX(pc + 1));
	    DECACHE_TOS(); /* must include arguments for proper gc */
	    topOfStack -= CVMmbArgsSize(mb);
	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    CHECK_NULL(directObj);
	    cb = CVMmbClassBlock(mb);
	    goto callmethod;
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
	    receiverObjICell = &STACK_ICELL(0);
	    directObj = STACK_OBJECT(0);
	    if (pc[2]) {
		CHECK_NULL(directObj);
	    }
	    UPDATE_PC_AND_TOS_AND_CONTINUE(3, 0);
	}
#endif

	/* inovke an interface method */

        CASE(opc_invokeinterface_quick) {
	    CVMObject*      directObj;
	    CVMMethodBlock* imb;  /* interface mb for method we are calling */
	    CVMClassBlock*  icb;  /* interface cb for method we are calling */
	    CVMClassBlock*  ocb;  /* cb of the object */
	    CVMUint16 methodTableIndex;/* index into ocb's methodTable */
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

	    /*
	     * Search the objects interface table, looking for the entry
	     * whose interface cb matches the cb of the inteface we
	     * are invoking.
	     */
	    if (guess < 0 || guess >= CVMcbInterfaceCount(ocb)
		          || CVMcbInterfacecb(ocb, guess) != icb) {
		for (guess = CVMcbInterfaceCount(ocb) - 1; guess >= 0; guess--) {
		    /* Update the guess if it was wrong, the code is not
		     * for a romized class, and the code is not transition
		     * code, which is also in ROM.
		     */
		    if (CVMcbInterfacecb(ocb, guess) == icb) {
		        if (!CVMisROMPureCode(CVMmbClassBlock(frame->mb)) && 
		            !CVMframeIsTransition(frame))
		            pc[4] = guess;
		        break;
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
	        methodTableIndex = CVMcbInterfaceMethodTableIndex(
                    ocb, guess, CVMmbMethodSlotIndex(imb));
            } else if (icb == CVMsystemClass(java_lang_Object)) {
                /*
                 * Per VM spec 5.4.3.4, the resolved interface method may be
                 * a method of java/lang/Object.
                 */
                 methodTableIndex = CVMmbMethodTableIndex(imb);
            } else {
                DECACHE_PC();  /* required for CVMsignalError() */
                CVMthrowIncompatibleClassChangeError(
                    ee, "class %C does not implement interface %C",
                    ocb, icb);
                goto handle_exception;
            }

            /*
	     * Fetch the proper mb and it's cb.
	     */
            mb = CVMcbMethodTableSlot(ocb, methodTableIndex);
            cb = CVMmbClassBlock(mb);
	    goto callmethod;
	}

        /* Return from a method */

	CASE_NT(opc_areturn)
	CASE_NT(opc_ireturn)
	CASE(opc_freturn)
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
	    CACHE_PREV_TOS();
	    STACK_INFO(0) = result;
	    topOfStack++;
	    TRACEIF(opc_ireturn, ("\tireturn %d\n", STACK_INT(-1)));
	    TRACEIF(opc_areturn, ("\tareturn 0x%x\n", STACK_OBJECT(-1)));
	    TRACEIF(opc_freturn, ("\tfreturn %f\n", STACK_FLOAT(-1)));
	    goto handle_return;
        }

	CASE(opc_return) {
	    /* 
	     * Offer a GC-safe point. The current frame is always up
	     * to date, so don't worry about de-caching frame.  
	     */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	    CACHE_PREV_TOS();
	    TRACE(("\treturn\n"));
	    goto handle_return;
	}

	CASE_NT(opc_lreturn)
	CASE(opc_dreturn)
        {
	    CVMJavaVal64 result;
	    /* 
	     * Offer a GC-safe point. The current frame is always up
	     * to date, so don't worry about de-caching frame.  
	     */
	    if (CHECK_PENDING_REQUESTS(ee)) {
		goto handle_pending_request;
	    }
	    CVMmemCopy64(result.v, &STACK_INFO(-2).raw);
	    CACHE_PREV_TOS();
	    CVMmemCopy64(&STACK_INFO(0).raw, result.v);
	    topOfStack += 2;
	    TRACEIF(opc_dreturn, ("\tfreturn1 %f\n", STACK_DOUBLE(-2)));
	    TRACEIF(opc_lreturn, ("\tlreturn %s\n",
				  GET_LONGCSTRING(STACK_LONG(-2))));
	    goto handle_return;
        }

	CASE(opc_exittransition) {
	    TRACE(("\texittransition\n"));
	    if (frame == initialframe) {
		/* We get here when we've reached the transition frame that
		 * was used to enter CVMgcUnsafeExecuteJavaMethod. All we
		 * to do is exit. The caller is reponsible for cleaning
		 * up the transiton frame.
		 */
		goto finish;
	    } else {
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
		CVMassert(frameSanity(frame));
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
			cb = CVMmbClassBlock(mb);
			if (CVMmbIs(mb, STATIC)) {
			    receiverObjICell = CVMcbJavaInstance(cb);
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
		    opcode = *pc;
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
		    }
#endif
		    goto opcode_switch;
		}
	    }
	}

	/* debugger breakpoint */
        CASE(opc_breakpoint) {
#ifdef CVM_JVMTI
	    CVMUint32 opcode0;
	    
	    TRACE(("\tbreakpoint\n"));
	    DECACHE_PC();
	    DECACHE_TOS();
	    CVMD_gcSafeExec(ee, {
		opcode0 = CVMjvmtiGetBreakpointOpcode(ee, pc, CVM_TRUE);
	    });
	    REEXECUTE_BYTECODE(opcode0);
#else
	    UPDATE_PC_AND_TOS_AND_CONTINUE(1, 0);    /* treat as opc_nop */
#endif
	}

	/* All of the non-quick opcodes. */

#ifdef CVM_CLASSLOADING
	/* -Set clobbersCpIndex true if the quickened opcode clobbers the
	 *  constant pool index in the instruction.
	 */
        CASE_NT(opc_invokevirtual)
        CASE_NT(opc_invokespecial)
	CASE_NT(opc_putfield)
	CASE_NT(opc_getfield)
	    goto quicken_opcode_clobber;
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
	    goto quicken_opcode_noclobber;
	{	    
	    CVMQuickenReturnCode retCode;
	    CVMBool clobbersCpIndex; /* true if quickening clobbers 
					the cp index */
    quicken_opcode_noclobber:
	    clobbersCpIndex = CVM_FALSE;
	    goto quicken_opcode;
    quicken_opcode_clobber:
	    clobbersCpIndex = CVM_TRUE;
    quicken_opcode:
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
		    goto init_class;
	        case CVM_QUICKEN_ERROR: {
		    CVMassert(CVMexceptionOccurred(ee));
		    goto handle_exception;
		}
	        default:
		    CVMassert(CVM_FALSE); /* We should never get here */
	    }
	    opcode = pc[0];
#ifdef CVM_JVMTI
	    /* Prevent double breakpoints. If the opcode we just
	     * quickened is an opc_breakpoint, then we've
	     * already notified the debugger of the breakpoint
	     * once. We don't want to do this again.
	     */
	    if (opcode == opc_breakpoint) {
		CVMD_gcSafeExec(ee, {
		    opcode = CVMjvmtiGetBreakpointOpcode(ee, pc,
							 CVM_FALSE);
		});
	    }
#endif
	    goto opcode_switch;
	}
#endif /* CVM_CLASSLOADING */

	CASE_NT(opc_new_checkinit_quick)
	CASE_NT(opc_agetstatic_quick)
	CASE_NT(opc_getstatic_quick)
	CASE_NT(opc_getstatic2_quick)
	CASE_NT(opc_aputstatic_quick)
	CASE_NT(opc_putstatic_quick)
	CASE_NT(opc_putstatic2_quick)
	CASE_NT(opc_putstatic2_checkinit_quick)
	CASE_NT(opc_getstatic2_checkinit_quick)
	CASE_NT(opc_aputstatic_checkinit_quick)
	CASE_NT(opc_agetstatic_checkinit_quick)
	CASE_NT(opc_putstatic_checkinit_quick)
	CASE_NT(opc_getstatic_checkinit_quick)
	CASE_NT(opc_nonnull_quick)
	CASE_NT(opc_instanceof_quick)
	CASE_NT(opc_checkcast_quick)
	CASE_NT(opc_anewarray_quick)
	CASE_NT(opc_new_quick)
	CASE_NT(opc_ldc2_w_quick)
	CASE_NT(opc_ldc_quick)
	CASE_NT(opc_aldc_ind_quick)
	CASE_NT(opc_aldc_quick)
	CASE_NT(opc_jsr_w)
	CASE_NT(opc_goto_w)
	CASE_NT(opc_ifnonnull)
	CASE_NT(opc_ifnull)
	CASE_NT(opc_monitorexit)
	CASE_NT(opc_monitorenter)
	CASE_NT(opc_athrow)
	CASE_NT(opc_arraylength)
	CASE_NT(opc_newarray)
	CASE_NT(opc_lookupswitch)
	CASE_NT(opc_tableswitch)
	CASE_NT(opc_ret)
	CASE_NT(opc_jsr)
	CASE_NT(opc_goto)
	CASE_NT(opc_if_acmpne)
	CASE_NT(opc_if_acmpeq)
	CASE_NT(opc_if_icmple)
	CASE_NT(opc_if_icmpgt)
	CASE_NT(opc_if_icmpge)
	CASE_NT(opc_if_icmplt)
	CASE_NT(opc_if_icmpne)
	CASE_NT(opc_if_icmpeq)
	CASE_NT(opc_ifle)
	CASE_NT(opc_ifgt)
	CASE_NT(opc_ifge)
	CASE_NT(opc_iflt)
	CASE_NT(opc_ifne)
	CASE_NT(opc_ifeq)
	CASE_NT(opc_dcmpg)
	CASE_NT(opc_dcmpl)
	CASE_NT(opc_fcmpg)
	CASE_NT(opc_fcmpl)
	CASE_NT(opc_i2s)
	CASE_NT(opc_i2c)
	CASE_NT(opc_i2b)
	CASE_NT(opc_d2f)
	CASE_NT(opc_f2d)
	CASE_NT(opc_f2l)
	CASE_NT(opc_f2i)
	CASE_NT(opc_l2f)
	CASE_NT(opc_l2i)
	CASE_NT(opc_i2f)
	CASE_NT(opc_i2l)
	CASE_NT(opc_iinc)
	CASE_NT(opc_lxor)
	CASE_NT(opc_ixor)
	CASE_NT(opc_lor)
	CASE_NT(opc_ior)
	CASE_NT(opc_land)
	CASE_NT(opc_iand)
	CASE_NT(opc_lushr)
	CASE_NT(opc_iushr)
	CASE_NT(opc_lshr)
	CASE_NT(opc_ishr)
	CASE_NT(opc_lshl)
	CASE_NT(opc_ishl)
	CASE_NT(opc_dneg)
	CASE_NT(opc_fneg)
	CASE_NT(opc_lneg)
	CASE_NT(opc_ineg)
	CASE_NT(opc_drem)
	CASE_NT(opc_frem)
	CASE_NT(opc_lrem)
	CASE_NT(opc_irem)
	CASE_NT(opc_ddiv)
	CASE_NT(opc_fdiv)
	CASE_NT(opc_ldiv)
	CASE_NT(opc_idiv)
	CASE_NT(opc_dmul)
	CASE_NT(opc_fmul)
	CASE_NT(opc_imul)
	CASE_NT(opc_dsub)
	CASE_NT(opc_fsub)
	CASE_NT(opc_lsub)
	CASE_NT(opc_isub)
	CASE_NT(opc_dadd)
	CASE_NT(opc_fadd)
	CASE_NT(opc_ladd)
	CASE_NT(opc_iadd)
	CASE_NT(opc_swap)
	CASE_NT(opc_dup2_x2)
	CASE_NT(opc_dup2_x1)
	CASE_NT(opc_dup2)
	CASE_NT(opc_dup_x2)
	CASE_NT(opc_dup_x1)
	CASE_NT(opc_dup)
	CASE_NT(opc_pop2)
	CASE_NT(opc_pop)
	CASE_NT(opc_sastore)
	CASE_NT(opc_castore)
	CASE_NT(opc_bastore)
	CASE_NT(opc_aastore)
	CASE_NT(opc_dastore)
	CASE_NT(opc_fastore)
	CASE_NT(opc_lastore)
	CASE_NT(opc_iastore)
	CASE_NT(opc_astore_3)
	CASE_NT(opc_astore_2)
	CASE_NT(opc_astore_1)
	CASE_NT(opc_astore_0)
	CASE_NT(opc_dstore_3)
	CASE_NT(opc_dstore_2)
	CASE_NT(opc_dstore_1)
	CASE_NT(opc_dstore_0)
	CASE_NT(opc_fstore_3)
	CASE_NT(opc_fstore_2)
	CASE_NT(opc_fstore_1)
	CASE_NT(opc_fstore_0)
	CASE_NT(opc_lstore_3)
	CASE_NT(opc_lstore_2)
	CASE_NT(opc_lstore_1)
	CASE_NT(opc_lstore_0)
	CASE_NT(opc_istore_3)
	CASE_NT(opc_istore_2)
	CASE_NT(opc_istore_1)
	CASE_NT(opc_istore_0)
	CASE_NT(opc_astore)
	CASE_NT(opc_dstore)
	CASE_NT(opc_fstore)
	CASE_NT(opc_lstore)
	CASE_NT(opc_istore)
	CASE_NT(opc_saload)
	CASE_NT(opc_caload)
	CASE_NT(opc_baload)
	CASE_NT(opc_aaload)
	CASE_NT(opc_daload)
	CASE_NT(opc_faload)
	CASE_NT(opc_laload)
	CASE_NT(opc_iaload)
	CASE_NT(opc_aload_3)
	CASE_NT(opc_aload_2)
	CASE_NT(opc_aload_1)
	CASE_NT(opc_aload_0)
	CASE_NT(opc_dload_3)
	CASE_NT(opc_dload_2)
	CASE_NT(opc_dload_1)
	CASE_NT(opc_dload_0)
	CASE_NT(opc_fload_3)
	CASE_NT(opc_fload_2)
	CASE_NT(opc_fload_1)
	CASE_NT(opc_fload_0)
	CASE_NT(opc_lload_3)
	CASE_NT(opc_lload_2)
	CASE_NT(opc_lload_1)
	CASE_NT(opc_lload_0)
	CASE_NT(opc_iload_3)
	CASE_NT(opc_iload_2)
	CASE_NT(opc_iload_1)
	CASE_NT(opc_iload_0)
	CASE_NT(opc_aload)
	CASE_NT(opc_dload)
	CASE_NT(opc_fload)
	CASE_NT(opc_lload)
	CASE_NT(opc_iload)
	CASE_NT(opc_sipush)
	CASE_NT(opc_bipush)
	CASE_NT(opc_dconst_1)
	CASE_NT(opc_dconst_0)
	CASE_NT(opc_fconst_2)
	CASE_NT(opc_fconst_1)
	CASE_NT(opc_fconst_0)
	CASE_NT(opc_lconst_1)
	CASE_NT(opc_lconst_0)
	CASE_NT(opc_iconst_5)
	CASE_NT(opc_iconst_4)
	CASE_NT(opc_iconst_3)
	CASE_NT(opc_iconst_2)
	CASE_NT(opc_iconst_1)
	CASE_NT(opc_iconst_0)
	CASE_NT(opc_iconst_m1)
	CASE_NT(opc_aconst_null)

#ifndef CVM_NO_LOSSY_OPCODES
        CASE_NT(opc_getfield_quick)
        CASE_NT(opc_putfield_quick)
        CASE_NT(opc_getfield2_quick)
        CASE_NT(opc_putfield2_quick)
	CASE_NT(opc_agetfield_quick)
	CASE_NT(opc_aputfield_quick)
#endif

	CASE_NT(opc_nop) {
	    CVMClassBlock *retCb;
	    pc = CVMgcUnsafeExecuteJavaMethodQuick(ee, pc,
						   topOfStack, locals, cp,
						   &retCb);
	    /* Refresh frame here. Quick() can do limited method
	       calls, and as such can invalidate frame */
	    CACHE_FRAME();
	    
	    if (pc != NULL) {
		/* No class initialization required */
		if (!CVMexceptionOccurred(ee)) {
		    /* Make sure we are executing within the current frame */
		    CVMassert(inMethod(frame, pc)); 
		    CACHE_TOS(); 
		    /* And make sure we have a sane topOfStack here */
		    CVMassert(inFrame(frame, topOfStack));
		    /* recache state of frame and dispatch opcode */
		    locals = CVMframeLocals(frame);
		    cp = CVMframeCp(frame);
		    opcode = *pc;
#ifdef CVM_JVMTI
		    /* Prevent double breakpoints. If the opcode we are
		     * returning to is an opc_breakpoint, then we've
		     * already notified the debugger of the breakpoint
		     * once. We don't want to do this again.
		     */
		    if (opcode == opc_breakpoint) {
			DECACHE_PC();
#if 0
			DECACHE_TOS(); /* already decached */
#endif
			CVMD_gcSafeExec(ee, {
			    opcode = CVMjvmtiGetBreakpointOpcode(ee, pc,
								 CVM_FALSE);
			});
		    }
#endif /* CVM_JVMTI */
		    goto opcode_switch;
		} else {
		    /* the TOS is about to be reset anyway, 
		       so don't bother */
		    goto handle_exception;
		}
	    } else {
		cb = retCb;
		if (cb != NULL) {
		    /*
		     * This is the class to initialize.
		     * Need to recache state of frame first
		     */
		    /*
		     * Remember the recorded PC and TOS
		     */
		    CACHE_PC(); 
		    CACHE_TOS(); 
		    opcode = *pc;
		    CVMassert(inMethod(frame, pc));
		    locals = CVMframeLocals(frame);
		    cp = CVMframeCp(frame);
		    goto init_class;
		} else {
		    /*
		     * A transition frame has been pushed.
		     */
		    CVMframeIncrementPcFlag(frame) = CVM_TRUE;
		    mb = frame->mb;
		    isStatic = CVMmbIs(mb, STATIC);
		    isVirtual = CVM_FALSE;
		    goto new_transition;
		}
	    }
	}
	

#ifdef CVM_NO_LOSSY_OPCODES
        CASE_NT(opc_getfield_quick)
        CASE_NT(opc_putfield_quick)
        CASE_NT(opc_getfield2_quick)
        CASE_NT(opc_putfield2_quick)
	CASE_NT(opc_agetfield_quick)
	CASE_NT(opc_aputfield_quick)
#endif

#ifndef CVM_CLASSLOADING
        CASE_NT(opc_ldc)
        CASE_NT(opc_ldc_w)
        CASE_NT(opc_ldc2_w)
        CASE_NT(opc_getfield)
        CASE_NT(opc_putfield)
        CASE_NT(opc_getstatic)
        CASE_NT(opc_putstatic)
        CASE_NT(opc_new)
        CASE_NT(opc_anewarray)
        CASE_NT(opc_multianewarray)
        CASE_NT(opc_checkcast)
        CASE_NT(opc_instanceof)
        CASE_NT(opc_invokevirtual)
        CASE_NT(opc_invokespecial)
        CASE_NT(opc_invokestatic)
        CASE_NT(opc_invokeinterface)
#endif

#ifndef CVM_INLINING
	CASE_NT(opc_invokeignored_quick)
#endif
        CASE(opc_xxxunusedxxx)
	DEFAULT
	    CVMdebugPrintf(("\t*** Unimplemented opcode: %d = %s\n",
		   opcode, CVMopnames[opcode]));
	    goto finish;

	} /* switch(opc) */

    callmethod:
	ASMLABEL(label_callmethod);
	{
	    /* callmethod handles the method invoking for all invoke
	     * opcodes. Upon entry, the following locals must be setup:
	     *    mb:            the method to invoke
	     *    cb:            CVMClassBlock* of mb
	     *    pc:            points to the invoke opcode
	     *    topOfStack:    points before the method arguments
	     *    receiverObjICell: the object we are dispatching on or
	     *			 the Class object for static calls
	     */
	    CVMFrame* prev;
	    int invokerIdx;

            TRACE(("\t%s %C.%M\n", CVMopnames[opcode], cb, mb));

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
		 * NOTE: We might want to just let the 2nd level loop
		 *       handle all Java calls.
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
		    /* pushframe set it to NULL */
		    CACHE_FRAME();
		    CVMassert(frameSanity(frame));
		    locals = NULL;
		    /* exception already thrown */
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

		/*
		 * Set up frame
		 */
		CVMframeLocals(frame) = locals;

	       /*
		* Set up new PC and constant pool
		*/
		pc = CVMjmdCode(jmd);
		cp = CVMcbConstantPool(cb);
		CVMframeCp(frame) = cp;
		
		/* This is needed to get CVMframe2string() to work properly */
		CVMdebugExec(DECACHE_PC());
		TRACE_METHOD_CALL(frame, CVM_FALSE);
		TRACESTATUS();

		CVMID_icellAssignDirect(ee,
					&CVMframeReceiverObj(frame, Java),
					receiverObjICell);
		
		/* %comment c002 */
		if (CVMmbIs(mb, SYNCHRONIZED)) {
		    /* The method is sync, so lock the object. */
		    /* %comment l002 */
		    /* old location no longer scanned, so update to frame */
		    receiverObjICell = &CVMframeReceiverObj(frame, Java);
		    if (!CVMobjectTryLock(
			    ee, CVMID_icellDirect(ee, receiverObjICell))) {
			DECACHE_PC();
			DECACHE_TOS();
			if (!CVMobjectLock(ee, receiverObjICell)) {
			    CVMpopFrame(stack, frame); /* pop the java frame */
			    CVMassert(frameSanity(frame));
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
		    /*
		     * Make sure that the CNI method did not leave any frames
		     * on the stack
		     */
		    CVMassert(frame == stack->currentFrame);

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
		    /*
		     * Make sure that the CNI method did not leave any frames
		     * on the stack
		     */
		    CVMassert(frame == stack->currentFrame);

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
		    cb = CVMmbClassBlock(mb);

		    /* no gcsafe checkpoint is needed since new_mb
		     * will eventually lead to one.
		     */
		    goto new_mb;
		} else if (ret == CNI_EXCEPTION) {
		    /*
		     * Make sure that the CNI method did not leave any frames
		     * on the stack
		     */
		    CVMassert(frame == stack->currentFrame);

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

		CVMassert(frameSanity(frame));
		CONTINUE;
	    } else if (invokerIdx == CVM_INVOKE_ABSTRACT_METHOD) {
		/*
		 * It's an abstract method.
		 */
		CVMthrowAbstractMethodError(ee, "%C.%M", cb, mb);
		goto handle_exception;
#ifdef CVM_CLASSLOADING
	    } else if (invokerIdx == CVM_INVOKE_NONPUBLIC_MIRANDA_METHOD) {
		/*
		 * It's a miranda method created to deal with a non-public
		 * method with the same name as an interface method.
		 */
		CVMthrowIllegalAccessError(
                    ee, "access non-public method %C.%M through an interface",
		    cb, CVMmbMirandaInterfaceMb(mb));
		goto handle_exception;
	    } else if (invokerIdx == 
		       CVM_INVOKE_MISSINGINTERFACE_MIRANDA_METHOD) {
		/*
		 * It's a miranda method created to deal with a missing
		 * interface method.
		 */
		CVMthrowAbstractMethodError(ee, "%C.%M", cb, mb);
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
		    /* We might gc, so flush state. */
		    ASMLABEL(label_handleUnlockFailureInReturn);
		    DECACHE_PC();
		    CVM_RESET_JAVA_TOS(frame->topOfStack, frame);
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
	    CVMassert(frameSanity(frame));

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
	    REEXECUTE_BYTECODE(*pc);
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

    throwNullPointerException: {
        CVM_JAVA_ERROR(NullPointerException, NULL);
    }
	
    throwOutOfMemoryError: {
	CVM_JAVA_ERROR(OutOfMemoryError, NULL);
    }
	
    throwNegativeArraySizeException: {
	CVM_JAVA_ERROR(NegativeArraySizeException, NULL);
    }
	
    handle_exception: {
	    ASMLABEL(label_handle_exception);
	    CVMassert(CVMexceptionOccurred(ee));

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
	  

	    /* Get the frame that handles the exception. frame->pc will
	     * also get updated.
	     */
	    frame = CVMgcUnsafeHandleException(ee, frame, initialframe);
	    /* Make sure the handler frame is "visible" from outside */
	    CVMassert(frame == stack->currentFrame);
	    
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
	ASMLABEL(label_init_class);
	{
	    /* init_class will run static initializers for the specified class.
	     * It does this in a way that won't involve C recursion.
	     */
            int result;
	    TRACE(("\t%s ==> running static initializers...\n",
		   CVMopnames[opcode]));
	    DECACHE_PC();
	    DECACHE_TOS();
	    CVMD_gcSafeExec(ee, {
		result = CVMclassInitNoCRecursion(ee, cb, &mb);
	    });
	    
	    /* If the result is 0, then the intializers have already been run.
	     * If the result is 1, then mb was setup to run the initializers.
	     * If the result is -1, then an exception was thrown.
	     */
	    if (result == 0) {
		CONTINUE;
	    } else if (result == 1) {
		CACHE_FRAME();
		isStatic = CVM_FALSE;
		isVirtual = CVM_FALSE;
		goto new_transition;
	    } else {
		goto handle_exception;
	    }
        }
    } /* while (1) interpreter loop */

 finish:
    ASMLABEL(label_finish);
    TRACE(("Exiting interpreter\n"));
    DECACHE_TOS();
    DECACHE_PC();

    return;
}
