/*
 * @(#)jit.h	1.52 06/10/10
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

/*
 * Porting layer for JIT.
 * Things in this file are use by the shared JIT code.
 * Many are supplied by the RISC layer if you are doing a RISC-based port,
 * some are usually supplied by the POSIX layer,
 * and some depend on the porting target.
 */

#ifndef _INCLUDED_PORTING_JIT_JIT_H
#define _INCLUDED_PORTING_JIT_JIT_H

#include "javavm/include/porting/defs.h"
#ifndef _ASM
#include "javavm/include/defs.h"
#include "javavm/include/jit/jit_defs.h"
#endif


/*
 * Number of 32-bit words per register (must be 1 in current implementation)
 *
 * #define CVMCPU_MAX_REG_SIZE	1
 */


#ifdef CVM_JIT_USE_FP_HARDWARE
/*
 * Number of 32-bit words per FP register (should be 1 or 2)
 *
 * #define CVMCPU_FP_MAX_REG_SIZE 
 */
#endif

/*
 * CVMCPU_HAS_64BIT_REGISTERS: define the value if the target has
 * 64-bit registers. In that case, two optional APIs, 
 * CVMCPUemitMoveTo64BitRegister and CVMCPUemitMoveFrom64BitRegister 
 * need to be implemented for the target. The share RISC grammar rules use 
 * CVMCPUemitMoveTo64BitRegister to emit code to move the 64-bit content
 * of HI and LO registers into one 64-bit register before emitting calls 
 * to a C helper that has 64-bit integer type arguments.
 * CVMCPUemitMoveFrom64BitRegister is used to emit code to move the 
 * content of a 64-bit register back to HI and LO register pair if a C 
 * helper has 64-bit integer return type.
 */
#undef CVMCPU_HAS_64BIT_REGISTERS

/*
 * CVMCPU_HAS_CP_REG: define this value if you use a base register for
 * the jit constant pool rather than using the PC as a base register.
 * This will cause a cell to be reserved in the CVMCompiledFrame for
 * saving and restoring the base register between method calls.
 * Depends on the target.
 */
#undef CVMCPU_HAS_CP_REG

/*
 * #define CVMCPU_HAS_VOLATILE_GC_REG if you are using trap-based GC
 * checks (CVMJIT_TRAP_BASED_GC_CHECKS), and you don't want to use a
 * dedicated register for storing CVMglobals.jit.gcTrapAddr. This will
 * result in slightly slower loops because CVMglobals.jit.gcTrapAddr
 * will need to be loaded from memory each time. However, it also
 * frees up an extra register for general purpose use, so it should be
 * enabled on platforms with fewer registers.
 *
 * NOTE: This flag is ignored unless CVMJIT_TRAP_BASED_GC_CHECKS
 *       is #define'd.
 */
#undef CVMCPU_HAS_VOLATILE_GC_REG

/*
 * #define CVMJIT_SIMPLE_SYNC_METHODS if the platform supports mapping
 * some synchronized library methods to versions that synchronize
 * much faster and can be inlined.
 */
#undef CVMJIT_SIMPLE_SYNC_METHODS

/*******************************************************************
 * Intrinsics handling. The following are used for code generation
 * of specially-recognized methods for which we want to provide a 
 * custom (and probably cheaper) implementation.
 * See src/share/javavm/include/jitintrinsic.h for greater detail.
 *
 * CVMJITintrinsicsList: define this value to be the name of the target-
 * specific intrinsics config list if one is available.  If the target
 * does not define it, then the CVMJITdefaultIntrinsicsList will be used.
 *
 * They all depend on the target.
 *
 * NOTE: The default value for CVMJITintrinsicsList is defined after
 *       CVM_HDR_JIT_JIT_H is included below.
 */

/*
 * CVMJIT_INTRINSICS: define this value if the target port wishes to support
 *                    JIT intrinsics.  Otherwise, all intrinsics code will be
 *                    negated and omitted from the build where possible.
 */
#undef CVMJIT_INTRINSICS

/* 
 * CVMJIT_INTRINSICS_HAVE_PLATFORM_SPECIFIC_REG_ARGS:
 * define this value if the target port wishes to implement support for
 * CVMJITINTRINSIC_REG_ARGS.
 */
#undef CVMJIT_INTRINSICS_HAVE_PLATFORM_SPECIFIC_REG_ARGS

/*
 * CVMCPU_MAX_ARG_REGS: define this value to be the maximum number registers
 *                      that are available to use as argument registers.
 *			Supplied by the target.
 */
#undef CVMCPU_MAX_ARG_REGS

/*
 * CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS: define this value if the target
 * will allow C arguments beyond the maximum number or argument registers
 * (specified by CVMCPU_MAX_ARG_REGS) are able to accommodate.
 * Depends on the target.
 */
#undef CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS

/*******************************************************************/

/* 
 * Required specific architecture constants.
 * All depend on the target.
 */

/*
 * Estimate the number of instructions it takes to invoke compiled to 
 * compiled. Adjust the value to compensate for stalls and branch overhead.
 * This value is used by profiling code to estimate how much performance
 * would be improved if a method was inlined.
 * Used by JIT profiling.
 */
/* CVMCPU_NUM_METHOD_INVOCATION_INSTRUCTIONS */

/*
 * We estimate the amount of native code that will be generated based
 * on the bytecodes of the method (including inlining).
 */

/* The minimum size. Our computations for very short methods tend to
 * underestimate code size. This is a floor for code buffer size.
 */
/* CVMCPU_INITIAL_SIZE */

/* A ROUGH estimate of how much code will expand from byte-code to
   native form */
/* CVMCPU_CODE_EXPANSION_FACTOR */

/* Size of method entry and exit code */
/* CVMCPU_ENTRY_EXIT_SIZE */

/*
 * Some bytecodes generated far more native code than others. Several of those
 * are counted separately. The multipliers for these must be defined
 * by these macro values.
 */

/* A GC write barrier is generated for each reference store into the heap.
 * (i.e. for a putfield of a reference type, or an aastore).
 * This gives the write barrier size, not counting potential spills,
 * nor the actual store instruction.
 */
/* CVMCPU_GC_BARRIER_SIZE */

/* Some misc opcodes which generate abnormally large code segments */
/*
    CVMCPU_WORD_ARRAY_LOAD_SIZE
    CVMCPU_DWORD_ARRAY_LOAD_SIZE
    CVMCPU_WORD_ARRAY_STORE_SIZE
    CVMCPU_DWORD_ARRAY_STORE_SIZE
    CVMCPU_INVOKEVIRTUAL_SIZE
    CVMCPU_INVOKEINTERFACE_SIZE
    CVMCPU_CHECKCAST_SIZE
    CVMCPU_INSTANCEOF_SIZE
    CVMCPU_RESOLVE_SIZE
    CVMCPU_CHECKINIT_SIZE
    Include size of null check, if applicable.
*/


/* Virtual inlining overhead size including phi handling,
   class block comparison, and out-of-line call on comparison failure*/
/* CVMCPU_VIRTINLINE_SIZE */


/* If trap-based null checks are supported, the target should set: */
/* CVMJIT_TRAP_BASED_NULL_CHECKS */

/*
 * Instruction size. If instructions have variable length, then use a 
 * size of 1.
 * CVMCPU_INSTRUCTION_SIZE
 */

/*
 * Typedef for the datatype capable of holding an instruction. If the
 * instruction size is variable length, use a type capable of holding
 * the longest possible instruction.
 *
 *     typedef CVMCPUInstruction;
 */

/*
 * Structure for the target-specific portion of the CVMJITCompilationContext.
 * Supplied by the RISC layer for RISC ports.
 *
 *     struct CVMJITTargetCompilationContext;
 */

/*
 * Structure or enum for specifying the regsRequired synthesis field
 * of an IRNode. Note, "regsRequired" is somewhat of a misnomer.Tthis
 * field of the inrode can contain any value useful during the JCS
 * synthesis action.
 *
 *     typedef CVMJITRegsRequiredType;
 */

/*
 * Structure or enum for specifying the regsRequired synthesis field
 * of an IRNode. Note, "regsRequired" is somewhat of a misnomer. This
 * field of the irnode can contain any value useful during the JCS
 * synthesis action.
 *
 *     typedef CVMJITRegsRequiredType;
 */

/*
 * TODO - CVMCompiledGCCheckPCs is needed by the shared risc port,
 * but may not be needed by other ports. We should really require the
 * export of a target specific data structure that can contain anything
 * rather than a CVMCompiledGCCheckPCs struct.
 *
 * Code offsets within the method that require GC check instructions.
 * 
 * #ifdef CVMJIT_PATCH_BASED_GC_CHECKS
 *     struct CVMCompiledGCCheckPCs
 * #endif
 */

/******************************************************************
 * GC CheckPoints support.
 *
 * There are three choice for handling gc rendezvous points (checkpoints)
 * in compiled code:
 *   1. Patch-based gc checks, enabled by doing a #define of
 *      CVMJIT_PATCH_BASED_GC_CHECKS. When a gc is requested, each
 *      gc checkpoint is patched with an instruction that will make
 *      sure CVMD_gcRendezvous() is called. This technique gives zero
 *      overhead gc points, but is the hardest to implement.
 *   2. Trap-based gc checks, enabled by doing a #define of
 *      CVMJIT_TRAP_BASED_GC_CHECKS. At each gc point an instruction is
 *      generated that can conditionally produced a SEGV, but only when
 *      checkpoints are enabled. A signal handler will be responsible for
 *      making sure CVMD_gcRendezvous() is called. Not quite zero overhead
 *      like (1), but far better than (3) and easier to implement then (1).
 *   3. If neither CVMJIT_TRAP_BASED_GC_CHECKS or CVMJIT_TRAP_BASED_GC_CHECKS
 *      are enabled, then code is generated inline to check if a
 *      gc rendezvous has been requested, and call CVMD_gcRendezvous() if
 *      one has. This is the easiest of the 3 to implement, but has very
 *      high overhead, as much as 20% on some microbenchmarks.
 *
 */

/* NOTE: by default patch-based is enabled unless explicitly #undef'd */
#define CVMJIT_PATCH_BASED_GC_CHECKS

/*
 * IAI-20:
 * For inlined virtual method invocation, use class guard test to 
 * replace the original method guard test; move method guard test
 * into out-of-line block to guard those failed in class guard test.
 *
 * By default this is enabled.
 */
#define IAI_VIRTUAL_INLINE_CB_TEST

#ifndef _ASM

/******************************************************************
 * Purpose: Initialized the target specific compiler back-end.
 * Supplied by the target.
 */
extern CVMBool
CVMJITinitCompilerBackEnd(void);

/*
 * Purpose: Destroys the target specific compiler back-end.
 */
extern void
CVMJITdestroyCompilerBackEnd(void);

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
/* Purpose: back-end PMI initialization. */
extern CVMBool
CVMJITPMIinitBackEnd(void);
#endif

/******************************************************************
 * The following two code cache functions are called if 
 * the macro CVMJIT_HAVE_PLATFORM_SPECIFIC_ALLOC_FREE_CODECACHE
 * is defined. They must be supplied by the target port.
 * Otherwise, malloc is used.
 */

/*
 * Purpose: Allocate the code cache.
 */
void *
CVMJITallocCodeCache(CVMSize *size);

/*
 * Purpose: Free the code cache.
 */
void
CVMJITfreeCodeCache(void *start);

/******************************************************************
 * JIT AOT API's. Only need to be implemented if CVM_AOT is 
 * #define'd.
 */

/* Don't use fixed address for AOT codecache by default. */
#undef CVMAOT_USE_FIXED_ADDRESS

/* Purpose: Find AOT code from the persistent storage. Initialize
 *          following AOT related global variables:
 *              jgs->codeCacheAOTStart
 *              jgs->codeCacheAOTEnd
 *              jgs->codeCacheAOTCodeExist
 *          The return value is the size of the AOT code.
 *          If there is no existing AOT code, allocate a consecutive
 *          code cache for bot h AOT and JIT compilation.
 */
extern CVMInt32
CVMfindAOTCode();

/*
 * The compiled code above the codeCacheDecompileStart will be saved
 * into persistent storage if there is no previously saved AOT code,
 * and will be reloaded next time.
 */
extern void
CVMJITcodeCachePersist();

/*
 * Destroy AOT code cache. The return value indicates if 
 * we need to free the JIT code cache separately.
 */
extern CVMBool
CVMJITAOTcodeCacheDestroy();

/******************************************************************/

/*
 * Code generation entry point.
 * Supplied by the RISC layer for RISC ports
 */
extern void
CVMJITcompileGenerateCode(CVMJITCompilationContext* con);

/*
 * Flush I & D caches for given range after writing compiled code.
 * Must be defined by the target. NOTE: when flushing after patching
 * a single instruction, "begin" must be the actual address of the
 * patched instruction. No rounding (such as to the cache line
 * boundary) should be done.
 */
extern void
CVMJITflushCache(void* begin, void* end);

/* Purpose: Massage the compiled PC.  This is used in the mapping of a compiled
            PC to Java Bytecode PC.  The massaging is necessary because
            compiled PCs point to the return address from a method call as
            opposed to the caller's PC as is the convention for Java PCs.
	    Supplied by the RISC layer for RISC ports */
extern CVMUint8 *
CVMJITmassageCompiledPC(CVMUint8 *compiledPC, CVMUint8 *startPC);

/******************************************************************
 * JIT profiling API's
 * If the target is a Posix system, these can be provided by the POSIX layer.
 */

/* Return how big the profiling buffer needs to be */
CVMUint32
CVMJITprofileBufferSize();

/* enable profiling */
void
CVMJITprofileEnable();

/* disable profiling */
void
CVMJITprofileDisable();

/* return code pointer for next region profiled */
CVMUint8*
CVMJITprofileGetNextProfiledRegion(CVMUint8* regionPtr);

/* return sample count for specified region */
CVMUint32
CVMJITprofileGetSampleCount(CVMUint8* regionPtr);

/* number of samples taken per second */
/* CVMJIT_PROFILE_SAMPLES_PER_SECOND */

/******************************************************************
 * GC CheckPoints APIs.
 *
 * These APIs only need to be implemented if CVMJIT_PATCH_BASED_GC_CHECKS
 * or CVMJIT_TRAP_BASED_GC_CHECKS are #define'd. The are tasked with
 * enabling and disabling gc rendezvous points (checkpoints) in compiled
 * code.
 */

extern void
CVMJITenableRendezvousCalls(CVMExecEnv* ee);

extern void
CVMJITdisableRendezvousCalls(CVMExecEnv* ee);

extern void
CVMJITenableRendezvousCallsTrapbased(CVMExecEnv* ee);

extern void
CVMJITdisableRendezvousCallsTrapbased(CVMExecEnv* ee);

/******************************************************************
 * Runtime routines supplied by the target:
 *
 * Enter a compiled method from the interpreter loop.
 * CVMMethodBlock* return value allows
 * the compiled code to 'return' to the interpreter for execution
 * of an interpreted method.
 */
extern CVMMethodBlock*
CVMJITgoNative(CVMObject* exceptionObject, CVMExecEnv* ee,
	       CVMCompiledFrame* jfp, void* pc);

/*
 * Exit a compiled method and return to the interpreter loop.
 * Returns NULL to the caller of CVMJITgoNative.
 * Used in exception handling.
 */
extern void
CVMJITexitNative(CVMCCExecEnv* ccee);

/*
 * Fixup up uninitialized fields in compiled frames
 * In compiled code, we don't always keep all the bits in
 * the stack frame perfectly up to date. This reduces overhead,
 * but requires use of this function when correctness does matter.
 */
extern void
CVMJITfixupFrames(CVMFrame *);

/******************************************************************
 * Used and supplied by the RISC layer for RISC ports:
 * Special backend actions to be done at the beginning and end of each
 * codegen rule. Usually only enabled when debugging.
 * Not needed when not using JCS-based code generation.
 */
extern void
CVMJITdoStartOfCodegenRuleAction(CVMJITCompilationContext *con, int ruleno,
                                 const char *description,
                                 CVMJITIRNode* node);
extern void
CVMJITdoEndOfCodegenRuleAction(CVMJITCompilationContext *con);


/******************************************************************
 * Compilation routines supplied by the target:
 *
 * CVMJITcanReachAddress - Check if toPC can be reached by an
 * instruction at fromPC using the specified addressing mode. If
 * needMargin is true, then a margin of safety is added (usually the
 * allowed offset range is cut in half).
 * Also used for determining when to dump a constant pool.
 */
extern CVMBool
CVMJITcanReachAddress(CVMJITCompilationContext* con,
		      int fromPC, int toPC,
		      CVMJITAddressMode mode, CVMBool needMargin);

/*
 * CVMJITfixupAddress - change the instruction to reference the specified
 * targetLogicalAddress.
 */
extern void
CVMJITfixupAddress(CVMJITCompilationContext* con,
		   int instructionLogicalAddress,
		   int targetLogicalAddress,
		   CVMJITAddressMode instructionAddressMode);

#ifdef IAI_CACHEDCONSTANT
/*
 * CVMJITemitLoadConstantAddress - Emit instruction(s) to load the address
 * of a constant pool constant into a platform defined register.
 */
extern void
CVMJITemitLoadConstantAddress(CVMJITCompilationContext* con,
			      int targetLogicalAddress);
#endif

/*
 * Emit a 32 bit value (aka .word).
 * Supplied by the target.
 * This is used by jitconstantpool manager, which is independent
 * of the RISC layer, as well as a number of places in the RISC layer.
 * Other emitters that are used only within the RISC
 * layer are declared in a different file.
 */
extern void
CVMJITemitWord(CVMJITCompilationContext *con, CVMInt32 const32);

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS

/*
 * Patch CPU specific branch bits.
 * 
 * This is used for the patched method invocations implementation.
 * The implementation of this method will patch the actual
 * bits of the branch instruction at instr_addr, so that
 * the branch will now point to the new offset from instr_addr.
 */

extern void 
CVMCPUpatchBranchInstruction(int offset, CVMUint8* instr_addr);

#endif

/*
 * More porting layer constants are defined in ccm.h. See details there.
 */
#include "javavm/include/porting/jit/ccm.h"

#endif /* _ASM */

#ifdef CVM_HDR_JIT_JIT_H
#include CVM_HDR_JIT_JIT_H
#endif

#ifdef CVMJIT_INTRINSICS
/* The following must come after CVM_HDR_JIT_JIT_H is included so that the
 * target can override it: */
#ifndef CVMJITintrinsicsList
#define CVMJITintrinsicsList CVMJITdefaultIntrinsicsList
#endif
#endif /* CVMJIT_INTRINSICS */

#endif /* _INCLUDED_PORTING_JIT_JIT_H */
