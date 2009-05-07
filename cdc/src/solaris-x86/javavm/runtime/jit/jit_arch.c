/*
 * @(#)jit_arch.c	1.6 06/10/24
 *
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
    /* On the x86, this is a no-op -- the I-cache is guaranteed to be consistent
     * after the next jump, and the VM never modifies instructions directly ahead
     * of the instruction fetch path. */
}

#include "javavm/include/jit/ccmcisc.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include <signal.h>
#include <sys/ucontext.h>

#if defined(CVM_JIT) && defined(CVMJIT_TRAP_BASED_NULL_CHECKS)
/* SVMC_JIT. signal handling for null pointers. */

#include "javavm/include/jit/jitcodebuffer.h"
#include <signal.h>
#include <sys/ucontext.h>

/* REG_PC and REG_SP are already defined for solaris 10 but not solaris 8. */
#ifndef REG_PC
#define REG_PC  PC
#endif
#ifndef REG_SP
#define REG_SP  USP
#endif 

/*
 * SEGV handler
 * SVMC_JIT. Need to implement chained SEGV handler.
 */
static void handleSegv(int sig, siginfo_t* info, void* ucpPtr)
{
    struct ucontext* ucp = (struct ucontext *)ucpPtr;
    struct sigaction sa;
    CVMUint8* pc = (CVMUint8 *)ucp->uc_mcontext.gregs[REG_PC];
    CVMUint8** sp = (CVMUint8**)ucp->uc_mcontext.gregs[REG_SP];
    if (CVMJITcodeCacheInCompiledMethod(pc)) {
	/* Coming from compiled code. */
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
	{
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
		    ucp->uc_mcontext.gregs[REG_SP] = sp;
		    /* Branch to do a gc rendezvous */
		    ucp->uc_mcontext.gregs[REG_PC] =
			(unsigned long)CVMCCMruntimeGCRendezvousGlue;
		} else {
		    /* phi handling: branch to generated code that will
		     * spill phis, call CVMCCMruntimeGCRendezvousGlue, and
		     * then reload phis.
		     */
		    ucp->uc_mcontext.gregs[REG_PC] += offset;
		}
		return;
	    }
	}
#endif
#ifdef CVMJIT_TRAP_BASED_NULL_CHECKS
	/* Branch and link to throw null pointer exception glue */
        /* CVMJITmassageCompiledPC assumes a indirect call (instruction size == 2)
	 * at pc, and will subtract 2 bytes */
	*(--sp) = pc + 2;
	ucp->uc_mcontext.gregs[REG_SP] = (unsigned long)sp;
	ucp->uc_mcontext.gregs[REG_PC] =
	    (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
#ifdef CVM_DEBUG_ASSERTS
	/* In CVMCCMruntimeThrowNullPointerExceptionGlue we check if %eax == (%esp) */
	ucp->uc_mcontext.gregs[REG_R0] = (unsigned long)pc + 2;
#endif	
	return;
#endif
    } else if (CVMJITcodeCacheInCCM(pc)) {
	/* Coming from CCM code. */
	/* Branch to throw null pointer exception glue */
        *(--sp) = (CVMUint8*) 0xcafebabe; /* glue code expects a return address on top of stack (rr) */
	ucp->uc_mcontext.gregs[REG_SP] = (unsigned long)sp;
	ucp->uc_mcontext.gregs[REG_PC] =
	    (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
	return;
    } else
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
}
#endif /* defined(CVM_JIT) && defined(CVMJIT_TRAP_BASED_NULL_CHECKS) */

CVMBool
solarisJITSyncInitArch(void)
{
#if defined(CVM_JIT) && defined(CVMJIT_TRAP_BASED_NULL_CHECKS)
    /* SVMC_JIT. signal handling for null pointers. */
    int signals[] = {SIGSEGV};
    int i;
    int result = 0;

    for (i = 0; result != -1 && i < (sizeof signals / sizeof signals[0]); ++i) {
        struct sigaction sa;
        sa.sa_sigaction = handleSegv;
        sa.sa_flags = SA_RESTART | SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
	result = sigaction(signals[i], &sa, NULL);
    }
    return (result != -1);
#else
    /* Do nothing */
    return CVM_TRUE;
#endif /* defined(CVM_JIT) && defined(CVMJIT_TRAP_BASED_NULL_CHECKS) */
}
