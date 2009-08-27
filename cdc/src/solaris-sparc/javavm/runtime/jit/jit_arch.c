/*
 * @(#)jit_arch.c	1.15 06/10/10
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
     * NOTE: "end" is exclusive of the range to flush. The byte at "end"
     * is not flushed, but the byte before it is.
     */
#undef  DCACHE_LINE_SIZE
#define DCACHE_LINE_SIZE 64
    /* round down to start of cache line */
    begin = (void*) ((unsigned long)begin & ~(DCACHE_LINE_SIZE-1));
    /* round up to start of next cache line */
    end = (void*) (((unsigned long)end + (DCACHE_LINE_SIZE-1))
		   & ~(DCACHE_LINE_SIZE-1));
#undef DCACHE_LINE_SIZE

    CVMJITflushCacheDoit(begin, end);
}

#include "portlibs/jit/risc/include/porting/ccmrisc.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include <signal.h>
#include <sys/ucontext.h>

/*
 * SEGV handler
 * TODO: Need to implement chained SEGV handler.
 */
static void handleSegv(int sig, siginfo_t* info, void* ucpPtr)
{
#ifdef CVM_DEBUG
    int pid = getpid();
#endif
    struct ucontext* ucp = (struct ucontext *)ucpPtr;
    CVMUint8* pc = (CVMUint8 *)ucp->uc_mcontext.gregs[REG_PC];
    if (CVMJITcodeCacheInCompiledMethod(pc)) {
        /* fprintf(stderr, "Process #%d received signal %d in jit code\n",
                pid, sig); */
	/* Coming from compiled code. */
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
	{
	    CVMCPUInstruction gcTrapInstr = *(CVMCPUInstruction*)pc;
	    /* ldr offset in lower 13 bits*/
	    int offset = gcTrapInstr & 0x1fff;
	    if (offset >= 0x1000) {
		offset |= 0xffffe000; /* sign extend */
	    }
	    gcTrapInstr = gcTrapInstr & ~0x1fff;
	    if (gcTrapInstr == (CVMglobals.jit.gcTrapInstr & ~0x1fff)) {
		if (offset >= 0) {
		    /* set link register to return to just before the trap
		     * instruction where incoming locals are reloaded. */
		    ucp->uc_mcontext.gregs[REG_O7] =
			(unsigned long)pc - offset - 8;
		    /* Branch to do a gc rendezvous */
		    ucp->uc_mcontext.gregs[REG_PC] =
			(unsigned long)CVMCCMruntimeGCRendezvousGlue;
		    ucp->uc_mcontext.gregs[REG_nPC] =
			(unsigned long)CVMCCMruntimeGCRendezvousGlue;
		} else {
		    /* phi handling: branch to generated code that will
		     * spill phis, call CVMCCMruntimeGCRendezvousGlue, and
		     * then reload phis.
		     */
		    ucp->uc_mcontext.gregs[REG_PC] += offset;
		    ucp->uc_mcontext.gregs[REG_nPC] =
			ucp->uc_mcontext.gregs[REG_PC] ;
		}
#if 0
		fprintf(stderr, "redirecting to rendezvous code 0x%x 0x%x\n",
			(int)ucp->uc_mcontext.gregs[REG_PC],
			(int)ucp->uc_mcontext.gregs[REG_O7]);
#endif
		return;
	    }
	}
#endif
#ifdef CVMJIT_TRAP_BASED_NULL_CHECKS
	/* Coming from CCM code. */
	/* Branch to throw null pointer exception glue */
        ucp->uc_mcontext.gregs[REG_O7] = (unsigned long)pc - 4;
        ucp->uc_mcontext.gregs[REG_PC] =
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
        ucp->uc_mcontext.gregs[REG_nPC] =
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
        return;
#endif
    } else if (CVMJITcodeCacheInCCM(pc)) {
#ifdef CVMJIT_TRAP_BASED_NULL_CHECKS
        /* fprintf(stderr, "Process #%d received signal %d in ccm code\n",
                pid, sig); */
        /* Coming from CCM code. */
	/* Branch to throw null pointer exception glue */
        ucp->uc_mcontext.gregs[REG_PC] = 
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
        ucp->uc_mcontext.gregs[REG_nPC] =
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
	return;
    }
#endif
#ifdef CVM_DEBUG
    {
	fprintf(stderr, "Process #%d received signal %d\n", pid, sig);
	/* A real one. Suspend thread */
	fprintf(stderr, "Process #%d being suspended\n", pid);
	
	kill(pid, SIGSTOP);
    }
#endif
}

/*
 * Install specific handlers for JIT exception handling
 */
CVMBool
solarisJITSyncInitArch(void)
{
    int signals[] = {SIGSEGV};
    int i;
    int result = 0;

    for (i = 0; result != -1 && i < (sizeof signals / sizeof signals[0]); ++i){
        struct sigaction sa;
        sa.sa_sigaction = handleSegv;
        sa.sa_flags = SA_RESTART | SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        result = sigaction(signals[i], &sa, NULL);
    }
    return (result != -1);
}
