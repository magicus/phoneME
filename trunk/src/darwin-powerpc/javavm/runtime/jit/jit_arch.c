/*
 * @(#)jit_arch.c	1.13 06/10/10
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
#include <ucontext.h>
#include <dlfcn.h>

#define MAXSIGNUM 32
#define MASK(sig) ((CVMUint32)1 << sig)

static CVMBool cvmSignalInstalling = CVM_FALSE;
#if 0
/* TODO: chaining doesn't work because I'm not sure how to
 * get the dlopen call to work.
 */
static struct sigaction sact[MAXSIGNUM];
static unsigned int cvmSignals = 0; /* signals used by cvm */

/* 
 * Call pthread's __sigaction 
 */
typedef int (*sigaction_t)(int, const struct sigaction *, struct sigaction *);

static sigaction_t sysSigaction = 0;

static int
callSysSigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    if (sysSigaction == NULL) {
        void * handle = dlopen("libpthread.dylib", RTLD_LAZY);
        if (handle == NULL) {
            return -1;
        }
        sysSigaction = (sigaction_t)dlsym(handle, "__sigaction");
        dlclose(handle);
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
#endif /* TODO */

/*
 * SEGV handler
 */
static void handleSegv(int sig, siginfo_t * info,
		       struct ucontext* scp)
{
#ifdef CVM_DEBUG
    int pid = getpid();
#endif
    CVMUint8* pc = (CVMUint8 *)scp->uc_mcontext->ss.srr0;
    if (CVMJITcodeCacheInCompiledMethod(pc)) {
#if 0 /* useful for debugging */
        fprintf(stderr, "Process #%d received signal %d in jit code (0x%x)\n",
                pid, sig, *(CVMUint32*)pc);
	CVMJITcodeCacheFindCompiledMethod(pc,1);
#endif
	/* Coming from compiled code. */
	/* Branch and link to throw null pointer exception glue */
	scp->uc_mcontext->ss.lr = scp->uc_mcontext->ss.srr0 + 4;
	scp->uc_mcontext->ss.srr0 = 
	    (unsigned int)CVMCCMruntimeThrowNullPointerExceptionGlue;
	return;
    } else if (CVMJITcodeCacheInCCM(pc)) {
#if 0
        fprintf(stderr, "Process #%d received signal %d in ccm code (0x%x)\n",
                pid, sig, *(CVMUint32*)pc);
	CVMJITcodeCacheFindCompiledMethod(pc,1);
#endif
	/* Coming from CCM code. */
	/* Branch to throw null pointer exception glue */
	scp->uc_mcontext->ss.srr0 = 
	    (unsigned int)CVMCCMruntimeThrowNullPointerExceptionGlue;
	return;
    } else
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
#if 0  /* TODO: chaining doesn't work */
        struct sigaction sa = sact[sig];
	if (sa.sa_handler == SIG_IGN || sa.sa_handler == SIG_DFL) {
	    fprintf(stderr, "Process #%d received signal %d\n", pid, sig);
	    fprintf(stderr, "Process #%d being suspended\n", pid);
	    kill(pid, SIGSTOP);
	} else {
	    (*(sa.sa_handler))(sig);
        }
#endif
    }
#endif
}

/*
 * Install specific handlers for JIT exception handling
 */
CVMBool
linuxJITSyncInitArch(void)
{
    int signals[] = {SIGSEGV, SIGBUS};
    int i;
    int result = 0;
    
    cvmSignalInstalling = CVM_TRUE;

    for (i = 0; result != -1 && i < (sizeof signals / sizeof signals[0]); ++i){
        struct sigaction sa;
        sa.sa_sigaction = (void(*)(int, siginfo_t*, void *))handleSegv;
        sa.sa_flags = SA_RESTART | SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        result = sigaction(signals[i], &sa, NULL);
	if (result != 0) {
	    break;
	}
    }

    cvmSignalInstalling = CVM_FALSE;

    return (result != -1);
}
