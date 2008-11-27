/*
 * @(#)segvhandler_arch.c	1.14 06/10/10
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
#include "javavm/include/jit/jitcodebuffer.h"

#include "javavm/include/porting/jit/jit.h"
#include "portlibs/jit/risc/include/porting/ccmrisc.h"

#include <signal.h>
#include <sys/ucontext.h>

#define MAXSIGNUM 32
#define MASK(sig) ((CVMUint32)1 << sig)

static CVMBool cvmSignalInstalling = CVM_FALSE;
#if 0 /* TODO - get chaining to work. */
static struct sigaction sact[MAXSIGNUM];
static unsigned int cvmSignals = 0; /* signals used by cvm */

#include <dlfcn.h>

/* 
 * Call pthread's __sigaction 
 */

typedef int (*sigaction_t)(int, const struct sigaction *, struct sigaction *);

static int
callSysSigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    static sigaction_t sysSigaction = 0;

    if (sysSigaction == NULL) {
        sysSigaction = (sigaction_t)dlsym(RTLD_NEXT, "__sigaction");
        if (sysSigaction == NULL) {
            return -1;
        }
    }
    return (*sysSigaction)(sig, act, oact); 
}

/* 
 * Override sigaction to chain signal handler. 
 */
int 
__sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    struct sigaction oldAct;
    if (cvmSignalInstalling) {
        int result = 0;
        if (act != NULL) {
	    /* CVM is installing handler for this signal. Install the new
               handler and save the old one. */
            result = callSysSigaction(sig, act, &oldAct);
            if (result != -1) {
                sact[sig] = oldAct;
                if (oact != NULL) {
                    *oact = oldAct;
                }
                cvmSignals |= MASK(sig); /* Mark the signal as used by CVM */
            }
        }
        return result;
    } else {
        if ((MASK(sig) & cvmSignals) != 0) {
	    /* CVM has installed its signal handler for this signal. Chain the
               new handler. */
	    if (oact != NULL) {
                *oact = sact[sig];
	    }
            if (act != NULL) {
                sact[sig] = *act;
            }
            return 0;
        } else {
	    /* The signal is not used by CVM. Install the new handler.*/
            return callSysSigaction(sig, act, oact);
        }
    }
}

int
sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    return __sigaction(sig, act, oact);
}

#endif

/*
 * WARNING #1:
 *
 * There are no fewer than 3 different sparc signal handler calling
 * conventions. The following may not work on all linux platforms.
 * For another alternative, look at the signal handler in the
 * solaris-sparc port.
 */

/*
 * WARNING #2:
 *
 * The following structs are to deal with a buggy sparc
 * signal handler with the Debian 3.0r1 kernel, which is 2.4.18 by default.
 * Supposedly this bug in fixed by the 2.4.19 kernel. The bug is that the
 * sigc argument is not setup, so you need to go digging for the registers
 * in the stack frame of the caller of the signal handler.
 */

struct pt_regs {
    unsigned long psr;
    unsigned long pc;
    unsigned long npc;
    unsigned long y;
    unsigned long u_regs[16]; /* globals and ins */
};

/* A Sparc stack frame */
struct sparc_stackf {
    unsigned long locals[8];
    unsigned long ins[6];
    struct sparc_stackf *fp;
    unsigned long callers_pc;
    char *structptr;
    unsigned long xargs[6];
    unsigned long xxargs[1];
};      

struct rt_signal_frame {
    struct sparc_stackf    ss;
    siginfo_t              info;
    struct pt_regs         regs;
    sigset_t               mask;
    void                  *fpu_save;
    unsigned int           insns [2];
    stack_t                stack;
#if 0
    unsigned int            extra_size; /* Should be 0 */
    __siginfo_fpu_t         fpu_state;
#endif
};
 
/*
 * SEGV handler
 */
static void handleSegv(int sig, siginfo_t* info, struct sigcontext* sigc)
{
#ifdef CVM_DEBUG
    int pid = getpid();
#endif
    struct pt_regs* regs = 
     &((struct rt_signal_frame*) ((char*) info - STACKFRAME_SZ))->regs;
    CVMUint8* pc = (CVMUint8 *)regs->pc;
    if (CVMJITcodeCacheInCompiledMethod(pc)) {
#if 0
        fprintf(stderr, "Process #%d received signal %d in jit code\n",
                pid, sig);
#endif
	/* Coming from compiled code. */
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
	{
	    CVMCPUInstruction gcTrapInstr = *(CVMCPUInstruction*)pc;
	    /* ldr offset in lower 13 bits*/
	    int offset = gcTrapInstr & 0x1fff;
	    if (offset >= 0x1000) {
		offset |= 0xffffe000; /* sign extend */
	    }
	    gcTrapInstr = gcTrapInstr & CVMCPU_GCTRAP_INSTRUCTION_MASK;
	    if (gcTrapInstr == CVMCPU_GCTRAP_INSTRUCTION) {
		if (offset >= 0) {
		    /* set link register to return to just before the trap
		     * instruction where incoming locals are reloaded. */
		    /* WARNING: REG_O7 won't work. It is equal to 18
		     * in an attempt to be the offset from the start
		     * of pt_regs, but even this is wrong since the
		     * offset should actually be 19. */
		    regs->u_regs[15] = regs->pc - offset - 8;  /* setup %o7 */
		    /* Branch to do a gc rendezvous */
		    regs->pc = (unsigned long)CVMCCMruntimeGCRendezvousGlue;
		    regs->npc = (unsigned long)CVMCCMruntimeGCRendezvousGlue;
		} else {
		    /* phi handling: branch to generated code that will
		     * spill phis, call CVMCCMruntimeGCRendezvousGlue, and
		     * then reload phis.
		     */
		    regs->pc += offset;
		    regs->npc = regs->pc;
		}
#if 0
		fprintf(stderr, "redirecting to rendezvous code 0x%x 0x%x\n",
			(int)regs->pc,
			(int)regs->u_regs[15]);
#endif
		return;
	    }
	}
#endif
#ifdef CVMJIT_TRAP_BASED_NULL_CHECKS
	/* Branch and link to throw null pointer exception glue */
	/* WARNING: REG_O7 won't work. It is equal to 18 in an attempt
	 * to be the offset from the start of pt_regs, but even this is
	 * wrong since the offset should actually be 19. */
	regs->u_regs[15] = regs->pc - 4;  /* setup %o7 */
        regs->pc =
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
        regs->npc =
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
	return;
#endif
    } else if (CVMJITcodeCacheInCCM(pc)) {
#ifdef CVMJIT_TRAP_BASED_NULL_CHECKS
#if 0
        fprintf(stderr, "Process #%d received signal %d in ccm code\n",
                pid, sig);
#endif
	/* Coming from CCM code. */
	/* Branch to throw null pointer exception glue */
        regs->pc = 
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
        regs->npc =
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
	return;
#endif
    }
#ifdef CVM_DEBUG
    {
	fprintf(stderr, "Process #%d received signal %d\n", pid, sig);
	/* A real one. Suspend thread */
	fprintf(stderr, "Process #%d being suspended\n", pid);
	
	kill(pid, SIGSTOP);
    }
#else
    /* Call chained handler */
    {
#if 0  /* TODO: get chaining to work */
        struct sigaction sa = sact[sig];
        if ((sa.sa_flags & SA_SIGINFO) != 0) {
            (*(sa.sa_sigaction))(sig, info, ucp);
        } else {
            if (sa.sa_handler == SIG_IGN || sa.sa_handler == SIG_DFL) {
                fprintf(stderr, "Process #%d received signal %d\n", pid, sig);
                fprintf(stderr, "Process #%d being suspended\n", pid);
                kill(pid, SIGSTOP);
            } else {
                (*(sa.sa_handler))(sig);
            }
        }
#endif
    }
#endif
}

/*
 * Install specific handlers for JIT exception handling
 */
CVMBool
linuxSegvHandlerInit(void)
{
    int signals[] = {SIGSEGV};
    int i;
    int result = 0;
    
    cvmSignalInstalling = CVM_TRUE;

    for (i = 0; result != -1 && i < (sizeof signals / sizeof signals[0]); ++i){
        struct sigaction sa;
        sa.sa_sigaction = (void(*)(int,siginfo_t*,void*))handleSegv;
        sa.sa_flags = SA_RESTART | SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        result = sigaction(signals[i], &sa, NULL);
    }

    cvmSignalInstalling = CVM_FALSE;

    return (result != -1);
}
