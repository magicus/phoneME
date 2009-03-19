/*
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 */
#include "javavm/include/defs.h"
#include "javavm/include/objects.h"

#include "javavm/include/assert.h"
#include "javavm/include/globals.h"
#include "javavm/include/porting/jit/jit.h"

/* Defined in jit_glue.S */
extern void CVMJITflushCacheDoit(void* begin, void* end);

/* Purpose: Flush I & D caches after writing compiled code. */
void
CVMJITflushCache(void* begin, void* end)
{
    /*
     * On the x86, this is a no-op -- the I-cache is guaranteed to be consistent
     * after the next jump, and the VM never modifies instructions directly 
     * ahead of the instruction fetch path.
     */
}


/* Implementation of Trap Based NullPointerException handling */
#include "javavm/include/jit/ccmcisc.h"
#include "javavm/include/jit/jitcodebuffer.h"

extern CVMMethodBlock *
X86JITgoNative(CVMObject *exceptionObject, CVMExecEnv *ee,
             CVMCompiledFrame *jfp, CVMUint8 *pc);

#ifdef CVM_JIT
/* Exception handler for Null Pointers. */

#include <excpt.h>
static DWORD handleException(struct _EXCEPTION_POINTERS *ep, DWORD code, DWORD *Addr, DWORD *Pc)
{
    CVMUint8 *pc = (CVMUint8 *)ep->ContextRecord->Eip;
    CVMUint8 **sp = (CVMUint8 **)ep->ContextRecord->Esp;

    if (code == EXCEPTION_ACCESS_VIOLATION &&
       CVMJITcodeCacheInCompiledMethod(pc))
    {
        /* Coming from compiled code. */
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
       {
         /*
          * Check whether the Trap instruction matches with that stored in
          * CVMglobals.jit.gcTrapInstr.
          */
         CVMCPUInstruction gcTrapInstr = *(CVMCPUInstruction*)pc;
	 CVMCPUInstruction masked =
	   gcTrapInstr & CVMCPU_GCTRAP_INSTRUCTION_MASK;
	 if (masked == CVMCPU_GCTRAP_INSTRUCTION) {
	    int offset;
	    if ((pc[1] & 0xc0) == 0x40) {
		offset = *(signed char *)(pc + 2);
	    } else {
		CVMassert((pc[1] & 0xc0) == 0x80);
		offset = *(int *)(pc + 2);
	    }
	    if (offset >= 0) {
		/* set link register to return to just before the trap
		 * instruction where incoming locals are reloaded. */
		*(--sp) = pc - offset;
		ep->ContextRecord->Esp = sp;
		ep->ContextRecord->Eip =
		    (unsigned long)CVMCCMruntimeGCRendezvousGlue;
	    } else {
		/* phi handling: branch to generated code that will
		 * spill phis, call CVMCCMruntimeGCRendezvousGlue, and
		 * then reload phis.
		 */
		ep->ContextRecord->Eip += offset;
	    }
            return EXCEPTION_CONTINUE_EXECUTION;
        }
       }
#endif

#ifdef CVMJIT_TRAP_BASED_NULL_CHECKS
       {
	/*
	 * Coming from compiled code.
         * Branch and link to throw null pointer exception glue.
         * CVMJITmassageCompiledPC assumes a indirect call 
         * (instruction size == 2) at pc and will subtract 2 bytes
         */
        *(--sp) = pc + 2;
        ep->ContextRecord->Esp = (DWORD)sp;
        ep->ContextRecord->Eip =
           (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
#ifdef CVM_DEBUG_ASSERTS
	/* 
         * In CVMCCMruntimeThrowNullPointerExceptionGlue we check if 
         * %eax == (%esp)
         */
        ep->ContextRecord->Eax = (DWORD)pc + 2;
#endif
        return EXCEPTION_CONTINUE_EXECUTION;
       }
#endif

    } else if (code == EXCEPTION_ACCESS_VIOLATION &&
            CVMJITcodeCacheInCCM(pc))
    {
#ifdef CVMJIT_TRAP_BASED_NULL_CHECKS
        /* 
         * Coming from CCM code.
         * Branch to throw null pointer exception glue
         */
        *(--sp) = (CVMUint8*) 0xcafebabe; /* glue code expects a return address on top of stack (rr) */
        ep->ContextRecord->Esp = (DWORD)sp;
        ep->ContextRecord->Eip =
           (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
        return EXCEPTION_CONTINUE_EXECUTION;
#endif
    }

    {
        *Pc = ep->ContextRecord->Eip;
        *Addr = (DWORD)ep->ExceptionRecord->ExceptionAddress;
        return EXCEPTION_EXECUTE_HANDLER;
    }
}
#endif /* defined(CVM_JIT) */

#if defined(CVM_JIT) && (defined(CVMJIT_TRAP_BASED_NULL_CHECKS) || \
			defined(CVMJIT_TRAP_BASED_GC_CHECKS))

CVMMethodBlock *
CVMJITgoNative(CVMObject *exceptionObject, CVMExecEnv *ee,
    CVMCompiledFrame *jfp, CVMUint8 *pc)
{
    DWORD epc, eaddr;
    __try {
        return X86JITgoNative(exceptionObject, ee, jfp, pc);
    } __except (handleException(_exception_info(), _exception_code(), &eaddr, &epc)) {
        /* NOTE: We could also implement CVMexitNative with exceptions */
        return 0;
    }
}
#endif

