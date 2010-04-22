/*
 * @(#)segvhandler_arch.c	1.10 06/10/10
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
#if defined(CVM_JIT) || defined(CVM_USE_MEM_MGR)

#include "javavm/include/defs.h"
#include "javavm/include/globals.h"
#ifdef CVM_JIT
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitcodebuffer.h"
#endif
#include "javavm/include/porting/memory.h"
#ifdef CVM_USE_MEM_MGR
#include "javavm/include/mem_mgr.h"
#endif
#include "portlibs/jit/risc/include/porting/ccmrisc.h"
#include <signal.h>
#include <dlfcn.h>
#include <stddef.h>
#include <unistd.h>

/* Hard code the correct version of ucontext, since sys/ucontext.h
   doesn't always get it right.
*/
/* avoid conflicting ucontext definitions */
#define ucontext asm_ucontext
struct ucontext {
	unsigned long	  uc_flags;
	struct ucontext  *uc_link;
	stack_t		  uc_stack;
	struct sigcontext uc_mcontext;
	sigset_t	  uc_sigmask;
};

#define MAXSIGNUM 32
#define MASK(sig) ((CVMUint32)1 << sig)

static struct sigaction sact[MAXSIGNUM];
static unsigned int cvmSignals = 0; /* signals used by cvm */
static CVMBool cvmSignalInstalling = CVM_FALSE;

/* 
 * Call pthread's __sigaction 
 */
typedef int (*sigaction_t)(int, const struct sigaction *, struct sigaction *);

static char* sysSigactionName[] = {"sigaction", "__sigaction"};
static sigaction_t sysSigaction[2] = {0, 0};

int
callSysSigaction(int sig, const struct sigaction *act,
                 struct sigaction *oact, int idx)
{
    if (sysSigaction[idx] == NULL) {
#ifdef LINUX_DLSYM_BUG
        void * handle = dlopen("libpthread.so.0", RTLD_LAZY);
        if (handle != NULL) {
	    sysSigaction[idx] = (sigaction_t)dlsym(handle, sysSigactionName[idx]);
	    dlclose(handle);
        }
#else
        sysSigaction[idx] = (sigaction_t)dlsym(RTLD_NEXT, sysSigactionName[idx]);
#endif
        if (sysSigaction[idx] == NULL) {
	    CVMdebugPrintf(("WARNING: lookup of __sigaction failed."));
            return -1;
        }
    }
    return (*sysSigaction[idx])(sig, act, oact);
}

/* 
 * Override sigaction to chain signal handler. 
 */
int 
CVMsigaction(int sig, const struct sigaction *act, 
             struct sigaction *oact, int idx)
{
    struct sigaction oldAct;
    if (cvmSignalInstalling) {
        int result = 0;
        if (act != NULL) {
	    /* CVM is installing handler for this signal. Install the new
               handler and save the old one. */
            result = callSysSigaction(sig, act, &oldAct, idx);
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
            return callSysSigaction(sig, act, oact, idx);
        }
    }
}

int
sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    return CVMsigaction(sig, act, oact, 0);
}

int 
__sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    return CVMsigaction(sig, act, oact, 1);
}

#ifdef CVM_USE_MEM_MGR

static CVMUint32 getRegContent(int regNo, struct ucontext* ucp)
{
    switch (regNo) {
    case 0: return (CVMUint32)ucp->uc_mcontext.arm_r0;
    case 1: return (CVMUint32)ucp->uc_mcontext.arm_r1;
    case 2: return (CVMUint32)ucp->uc_mcontext.arm_r2;
    case 3: return (CVMUint32)ucp->uc_mcontext.arm_r3;
    case 4: return (CVMUint32)ucp->uc_mcontext.arm_r4;
    case 5: return (CVMUint32)ucp->uc_mcontext.arm_r5;
    case 6: return (CVMUint32)ucp->uc_mcontext.arm_r6;
    case 7: return (CVMUint32)ucp->uc_mcontext.arm_r7;
    case 8: return (CVMUint32)ucp->uc_mcontext.arm_r8;
    case 9: return (CVMUint32)ucp->uc_mcontext.arm_r9;
    case 10: return (CVMUint32)ucp->uc_mcontext.arm_r10;
    case 11: return (CVMUint32)ucp->uc_mcontext.arm_fp;
    case 12: return (CVMUint32)ucp->uc_mcontext.arm_ip;
    case 13: return (CVMUint32)ucp->uc_mcontext.arm_sp;
    case 14: return (CVMUint32)ucp->uc_mcontext.arm_lr;
    case 15: return (CVMUint32)ucp->uc_mcontext.arm_pc;
    default: CVMassert(CVM_FALSE); return 0;
    }
}

#define STR_OPCODE  0x04000000
#define STRB_OPCODE 0x04400000
#define STRH_OPCODE 0x004000b0

static void decodeAtPCAndWrite(struct ucontext* ucp)
{
    CVMUint32 pagesize = sysconf(_SC_PAGESIZE);
    CVMUint32* pc = (CVMUint32*)(ucp->uc_mcontext.arm_pc);
    CVMUint32* faultAddr = (CVMUint32 *)ucp->uc_mcontext.fault_address;
    CVMUint32* startAddr = (CVMUint32*)((CVMUint32)faultAddr &
                                                   ~(pagesize-1));
    CVMUint32 pc_instr = *pc;
    
    if ((pc_instr & STRB_OPCODE) == STRB_OPCODE ||
        (pc_instr & STRH_OPCODE) == STRH_OPCODE ||
        (pc_instr & STR_OPCODE) == STR_OPCODE) {
        int srcRegNo = (pc_instr & 0xF000) >> 12;
        CVMUint32 srcReg = getRegContent(srcRegNo, ucp);
        
        /* Unprotect the page that contains xthe faulting address */
        CVMmprotect(startAddr, (void*)((CVMUint32)startAddr + pagesize),
                    CVM_FALSE);

        /* Emulate the write. */
        if ((pc_instr & STRB_OPCODE) == STRB_OPCODE) {
            *(CVMUint8*)faultAddr= (CVMUint8)srcReg;
        } else if ((pc_instr & STRH_OPCODE) == STRH_OPCODE) {
            *(CVMUint16*)faultAddr= (CVMUint16)srcReg;
        } else {
            *faultAddr = srcReg;
        }

        /* Re-protect the page */
        /* Always re-protect a page after a write, because it hard to
         * tell if part of a CVM_MEM_MON_FIRST_WRITE page also belongs
         * a CVM_MEM_MON_ALL_WRITES. We use the dirty page map to control
         * the notify instead.
         */
        CVMmprotect(startAddr, 
                    (void*)((CVMUint32)startAddr + pagesize),
                    CVM_TRUE);  
    } else {
        CVMassert(CVM_FALSE);
    }
}
#endif /* CVM_USE_MEM_MGR */

/*
 * SEGV handler
 */
#ifndef LINUX_ARM_SIGINFO_BUG
static void handleSegv(int sig, siginfo_t* info, void* ucpPtr)
#else /* LINUX_ARM_SIGINFO_BUG */
static void handleSegv(int sig, int a2, int a3, int a4,
		       siginfo_t* info, struct ucontext* ucp)
#endif /* LINUX_ARM_SIGINFO_BUG */
{
#ifndef LINUX_ARM_SIGINFO_BUG
    struct ucontext* ucp = (struct ucontext *)ucpPtr;
#endif /* LINUX_ARM_SIGINFO_BUG */
    int pid = getpid();
    CVMUint32* pc = (CVMUint32 *)ucp->uc_mcontext.arm_pc;

#ifdef CVM_USE_MEM_MGR
    CVMUint32* faultAddr = (CVMUint32 *)ucp->uc_mcontext.fault_address;
    CVMBool isValidAddr;

    isValidAddr = CVMMemWriteNotify(pid, faultAddr, pc);

    /* If it's gcTrapInstr or NullPointerException, the faultAddr usually
     * is an invalid address.
     */
    if (isValidAddr) {

        /* Emulate the write */
        decodeAtPCAndWrite(ucp);

        /* skip the write instruction */
        ucp->uc_mcontext.arm_pc += 4;
        return;
    }
#endif

#ifdef CVM_JIT
    if (CVMJITcodeCacheInCompiledMethod((CVMUint8*)pc)) {
	/* Coming from compiled code. */
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#define ARM_LOADSTORE_ADDOFFSET	0x00800000	/* U bit */
	{
	    CVMCPUInstruction gcTrapInstr = *(CVMCPUInstruction*)pc;
	    /* ldr offset in lower 12 bits */
	    int offset = gcTrapInstr & 0xfff;
	    if ((gcTrapInstr & ARM_LOADSTORE_ADDOFFSET) == 0) {
		offset = -offset; /* make negative */
	    }
	    gcTrapInstr = gcTrapInstr & CVMCPU_GCTRAP_INSTRUCTION_MASK;
	    if (gcTrapInstr == CVMCPU_GCTRAP_INSTRUCTION) {
		if (offset >= 0) {
		    /* set link register to return to just before the trap
		     * instruction where incoming locals are reloaded. */
		    ucp->uc_mcontext.arm_lr = ucp->uc_mcontext.arm_pc - offset;
		    /* Branch to do a gc rendezvous */
		    ucp->uc_mcontext.arm_pc =
			(unsigned long)CVMCCMruntimeGCRendezvousGlue;
		} else {
		    /* phi handling: branch to generated code that will
		     * spill phis, call CVMCCMruntimeGCRendezvousGlue, and
		     * then reload phis.
		     */
		    ucp->uc_mcontext.arm_pc += offset;
		}
#if 0
		fprintf(stderr, "redirecting to rendezvous code 0x%x 0x%x\n",
			(int)ucp->uc_mcontext.arm_pc,
			(int)ucp->uc_mcontext.arm_lr);
#endif
		return;
	    }
	}
#endif
#ifdef CVMJIT_TRAP_BASED_NULL_CHECKS
	/* Branch and link to throw null pointer exception glue */
	ucp->uc_mcontext.arm_lr = ucp->uc_mcontext.arm_pc + 4;
	ucp->uc_mcontext.arm_pc =
	    (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
	return;
#endif
    } else if (CVMJITcodeCacheInCCM((CVMUint8*)pc)) {
#ifdef CVMJIT_TRAP_BASED_NULL_CHECKS
	/* Coming from CCM code. */
	/* Branch to throw null pointer exception glue */
	ucp->uc_mcontext.arm_pc =
	    (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
	return;
#endif /* CVMJIT_TRAP_BASED_NULL_CHECKS */
    }
#endif /* CVM_JIT */
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
    
    /* The AAPCS check has nothing to do with signals, this is merely
       the only existing arm-specific code where we can check that the
       setting of -DAAPCS at build matches the calling and alignment
       convention.  Even so, the check will be done only if this code
       is actually compiled, e.g., CVM_JIT=true.  If AAPCS is in use,
       doubles are 8-byte aligned, otherwise they are 4-byte
       aligned. */

    if (offsetof(struct { int i; double d; }, d) == 8) {
#if !defined(AAPCS)
	CVMpanic("AAPCS calling convention used;"
		 " compilation must use -DAAPCS.\n");
#endif
    } else {
#if defined(AAPCS)
	CVMpanic("AAPCS calling convention not used;"
		 " compilation must not use -DAAPCS.\n");
#endif
    }

    cvmSignalInstalling = CVM_TRUE;

    for (i = 0; result != -1 && i < (sizeof signals / sizeof signals[0]); ++i){
        struct sigaction sa;
#ifndef LINUX_ARM_SIGINFO_BUG
        sa.sa_sigaction = handleSegv;
#else /* LINUX_ARM_SIGINFO_BUG */
        sa.sa_sigaction = (void(*)(int,siginfo_t*,void*))handleSegv;
#endif /* LINUX_ARM_SIGINFO_BUG */
        sa.sa_flags = SA_RESTART | SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
#ifndef LINUX_ARM_SIGINFO_BUG
        result = sigaction(signals[i], &sa, NULL);
#else /* LINUX_ARM_SIGINFO_BUG */
        result = __sigaction(signals[i], &sa, NULL);
#endif /* LINUX_ARM_SIGINFO_BUG */
    }

    cvmSignalInstalling = CVM_FALSE;

    return (result != -1);
}

#endif /* defined(CVM_JIT) || defined(CVM_USE_MEM_MGR) */
