/*
 * @(#)jitcodebuffer.h	1.47 06/10/10
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

#ifndef _INCLUDED_JITCODEBUFFER_H
#define _INCLUDED_JITCODEBUFFER_H

#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitutils.h"
#include "javavm/include/jit_common.h"

#ifdef CVM_USE_MEM_MGR
#include "javavm/include/mem_mgr.h"
#endif

/*
 * Code buffer management.
 */

#define CVMJITcbufGetLogicalPC(con)   ((con)->curLogicalPC)
#define CVMJITcbufGetPhysicalPC(con)  ((con)->curPhysicalPC)
#define CVMJITcbufGetLogicalInstructionPC(con) ((con)->logicalInstructionPC)
#define CVMJITcbufLogicalToPhysical(con, logical)	\
    (&(con)->codeEntry[logical])

/*
 * Rewind where we emit code be the specified amount. Used for
 * gc patching related code.
 */
#define CVMJITcbufRewind(con, amount)		\
    (con)->curLogicalPC -= amount;		\
    (con)->curPhysicalPC -= amount;

/*
 * each allocated and free buffer contains a 4-byte size value at both
 * the start and the end of the buffer. Free buffers have the low
 * bit set to indicate that they are free. Uncommitted buffers (allocated
 * but not compiled into yet) have the 2nd lowest bit set. Buffers used
 * for glue code have both of the lower bits set. Buffers containing
 * compiled methods don't have any of the lower bits set.
 */

#define CVMJIT_CBUFSIZE_FREE_BITS        0x1  /* free buffer */
#define CVMJIT_CBUFSIZE_UNCOMMITTED_BITS 0x2  /* compiling into */
#define CVMJIT_CBUFSIZE_CCMCOPY_BITS     0x3  /* contains copied CCM code */
#define CVMJIT_CBUFSIZE_MASK             0x3

#define CVMJITcbufSizeWord(cbuf)        (*(CVMUint32*)cbuf)
#define CVMJITcbufPrevBufSizeWord(cbuf) (*((CVMUint32*)cbuf-1))

#define CVMJITcbufSetBufSizeWord(cbuf, size, sizeword)			\
    *(CVMUint32*)(cbuf) = sizeword;					\
    *(CVMUint32*)((cbuf) + (size) - sizeof(CVMUint32)) = sizeword;
#define CVMJITcbufSetCommittedBufSize(cbuf, size)	       		\
    CVMJITcbufSetBufSizeWord(cbuf, size, size);
#define CVMJITcbufSetUncommitedBufSize(cbuf, size)			\
    CVMJITcbufSetBufSizeWord(cbuf, size, 				\
			     (size) | CVMJIT_CBUFSIZE_UNCOMMITTED_BITS);
#define CVMJITcbufSetFreeBufSize(cbuf, size)				\
    CVMJITcbufSetBufSizeWord(cbuf, size, (size) | CVMJIT_CBUFSIZE_FREE_BITS);
#define CVMJITcbufSetCCMCopyBufSize(cbuf, size)				\
    CVMJITcbufSetBufSizeWord(cbuf, size,				\
			     (size) | CVMJIT_CBUFSIZE_CCMCOPY_BITS);

#define CVMJITcbufSize(cbuf)        \
    (CVMJITcbufSizeWord(cbuf) & ~CVMJIT_CBUFSIZE_MASK)
#define CVMJITcbufPrevBufSize(cbuf) \
    (CVMJITcbufPrevBufSizeWord(cbuf) & ~CVMJIT_CBUFSIZE_MASK)

#define CVMJITcbufNextBuf(cbuf) (cbuf + CVMJITcbufSize(cbuf))
#define CVMJITcbufPrevBuf(cbuf) (cbuf - CVMJITcbufPrevBufSize(cbuf))

#define CVMJITcbufIsCommitted(cbuf)	\
	((CVMJITcbufSizeWord(cbuf) & CVMJIT_CBUFSIZE_MASK) == 0)
#define CVMJITcbufIsFree(cbuf)					\
	((CVMJITcbufSizeWord(cbuf) & CVMJIT_CBUFSIZE_MASK) ==	\
	 CVMJIT_CBUFSIZE_FREE_BITS)
#define CVMJITcbufIsCCMCopy(cbuf)				\
	((CVMJITcbufSizeWord(cbuf) & CVMJIT_CBUFSIZE_MASK) ==	\
	 CVMJIT_CBUFSIZE_CCMCOPY_BITS)

/*
 * CVMJITcbufEmit - various macros used to emit instructions. The macros
 * support emitting 1, 2, and 4 bytes at time. Odd sized instructions will
 * need to be supported in the platform specific emitter by using a 
 * combination of the macros defined here.
 *
 * CVMJITcbufEmit is always defined to emits the number of bytes 
 * equivalent to CVMCPU_INSTRUCTION_SIZE.
 */
#define CVMJITcbufEmit_n(con, instr, size, castType)			\
{									\
    if (((con)->curPhysicalPC + size) > (con)->codeBufEnd) {            \
        /* Fail to compile. We will retry with a larger buffer */	\
        CVMJITerror((con), CODEBUFFER_TOO_SMALL,			\
                    "Estimated code buffer too small");			\
    }									\
    *(castType*)(con)->curPhysicalPC = instr;				\
    (con)->curPhysicalPC += size;					\
    (con)->curLogicalPC  += size;					\
}

/* use to emit an instruction when the size is always the same*/
#define CVMJITcbufEmit(con, instr) \
    CVMJITcbufEmit_n(con, instr, CVMCPU_INSTRUCTION_SIZE, CVMCPUInstruction)

/* these are for processors with variable length instructions */
#define CVMJITcbufEmit1(con, instr) CVMJITcbufEmit_n(con, instr, 1, CVMUint8)
#define CVMJITcbufEmit2(con, instr) CVMJITcbufEmit_n(con, instr, 2, CVMUint16)
#define CVMJITcbufEmit4(con, instr) CVMJITcbufEmit_n(con, instr, 4, CVMUint32)

/*
 * CVMJITcbufEmitPC - emit an instruction at the specified pc.
 */
#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
#define CVMJITcbufEmitPC_n(con, pc, instr, size, castType)             \
{                                                                      \
    if ((CVMJITcbufLogicalToPhysical(con, pc) + size) > (con)->codeBufEnd) { \
       CVMJITerror((con), CODEBUFFER_TOO_SMALL,                        \
                  "Estimated code buffer too small");                  \
   }                                                                   \
   *(castType*)CVMJITcbufLogicalToPhysical(con, pc) = instr;           \
}

#define CVMJITcbufEmitPC(con, pc, instr)				\
    CVMJITcbufEmitPC_n(con, pc, instr,					\
		       CVMCPU_INSTRUCTION_SIZE, CVMCPUInstruction)
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */

/*
 * Returns the logical PC of the fixup pc, so you can re-emit code there
 */
extern void
CVMJITcbufPushFixup(CVMJITCompilationContext* con, CVMInt32 startPC);

extern void
CVMJITcbufPop(CVMJITCompilationContext* con);

extern void
CVMJITcbufInitialize(CVMJITCompilationContext* con);

/*
 * Allocate buffer and set pointers
 */
extern void
CVMJITcbufAllocate(CVMJITCompilationContext* con, CVMSize extraSpace);

extern void
CVMJITcbufCommit(CVMJITCompilationContext* con);

extern void
CVMJITcbufFree(CVMUint8* codeBuffer, CVMBool committed);

void
CVMJITcbufSetCmd(CVMUint8 *cbuf, CVMCompiledMethodDescriptor *);

CVMCompiledMethodDescriptor *
CVMJITcbufCmd(CVMUint8 *cbuf);

/**************************
 * Code Cache Management
 **************************/

extern CVMBool
CVMJITcodeCacheInit(CVMExecEnv* ee, CVMJITGlobalState *jgs);

extern CVMBool
CVMJITcodeCacheInitOptions(CVMJITGlobalState *jgs);

#ifdef CVM_AOT
CVMBool
CVMJITinitializeAOTCode();
#endif

extern void
CVMJITcodeCacheDestroy(CVMJITGlobalState *jgs);

extern void
CVMJITcodeCacheAgeEntryCountsAndDecompile(CVMExecEnv* ee,
					  CVMUint32 bytesRequested);

#ifdef CVM_JIT_PROFILE
extern void
CVMJITcodeCacheDumpProfileData();
#endif

/*
 * Returns CVM_TRUE iff 'pc' is in compiled code 
 */
extern CVMBool
CVMJITcodeCacheInCompiledMethod(CVMUint8* pc);

/*
 * Returns CVM_TRUE iff 'pc' is in CCM
 */
extern CVMBool
CVMJITcodeCacheInCCM(CVMUint8* pc);

#ifdef CVM_USE_MEM_MGR
/* Register the shared codecache. */
extern void
CVMmemRegisterCodeCache();

/*
 * Notify writes to registered shared code cache.
 */
extern void
CVMmemCodeCacheWriteNotify(int pid, void *addr, void *pc, CVMMemHandle *h);
#endif

#if defined(CVM_DEBUG) || defined(CVM_USE_MEM_MGR) || defined(CVM_TRACE_JIT)
extern CVMMethodBlock*
CVMJITcodeCacheFindCompiledMethod(CVMUint8* pc, CVMBool doPrint);
#endif

#ifdef CVM_DEBUG
extern CVMBool
CVMJITcodeCacheVerify(CVMBool doDump);
#endif

#endif /* _INCLUDED_JITCODEBUFFER_H */
