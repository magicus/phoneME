/*
 * @(#)jit_arch.c	1.6 06/10/10
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

#include "javavm/include/porting/jit/jit.h"
#include "portlibs/jit/risc/include/porting/ccmrisc.h"

#include <signal.h>
#include <sigLib.h>

/*
 * WARNING #1:
 *
 * There are no fewer than 3 different sparc signal handler calling
 * conventions. The following may not work on all vxworks platforms.
 * For other alternatives, look at the signal handlers in the
 * solaris-sparc and linux-sparc ports.
 */
 
/*
 * SEGV handler
 *
 * TODO: Need to implement chained SEGV handler. Don't forget to also
 * enable CVMJIT_TRAP_BASED_NULL_CHECKS
 */
static void handleSegv(int sig, siginfo_t* info,
		       struct sigcontext *pSigContext)
{
#if 0  /* TODO: get segv handler working */
    struct ucontext* ucp = (struct ucontext *)ucpPtr;
    CVMUint8* pc = (CVMUint8 *)ucp->uc_mcontext.gregs[REG_PC];
    if (CVMJITcodeCacheInCompiledMethod(pc)) {
        ucp->uc_mcontext.gregs[REG_O7] = (unsigned long)pc - 4;
        ucp->uc_mcontext.gregs[REG_PC] =
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
        ucp->uc_mcontext.gregs[REG_nPC] =
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
        return;
    } else if (CVMJITcodeCacheInCCM(pc)) {
        /* Coming from CCM code. */
	/* Branch to throw null pointer exception glue */
        ucp->uc_mcontext.gregs[REG_PC] = 
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
        ucp->uc_mcontext.gregs[REG_nPC] =
            (unsigned long)CVMCCMruntimeThrowNullPointerExceptionGlue;
        return;
    } else
#ifdef CVM_DEBUG
    {
	fprintf(stderr, "Thread received signal %d\n", sig);
    }
#endif
#endif
}

/*
 * Install specific handlers for JIT exception handling
 */
CVMBool
vxworksJITSyncInitArch(void)
{
    int signals[] = {SIGSEGV};
    int i;
    int result = 0;

#if 1    /* TODO: get segv handler working */
    return CVM_TRUE; 
#else
    for (i = 0; result != -1 && i < (sizeof signals / sizeof signals[0]); ++i){
        struct sigaction sa;
        sa.sa_sigaction = (void(*)(int,siginfo_t*,void*))handleSegv;
        sa.sa_flags = SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        result = sigaction(signals[i], &sa, NULL);
    }

    return (result != -1);
#endif
}
