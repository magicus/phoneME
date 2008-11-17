/*
 * @(#)jit_md.c	1.7 06/10/10
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

#include "javavm/include/globals.h"
#include "javavm/include/assert.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/porting/jit/jit.h"

#include <sys/mman.h>


#ifdef CVMJIT_HAVE_PLATFORM_SPECIFIC_ALLOC_FREE_CODECACHE

/* The following is needed to supported an executable code cache
   with some Solaris releases.
*/
void *
CVMJITallocCodeCache(CVMSize *size)
{
    void* s = mmap(0, *size, 
          PROT_EXEC | PROT_READ | PROT_WRITE, 
          MAP_PRIVATE | MAP_ANON, -1, 0);
    if (s == MAP_FAILED) {
        return NULL;
    }
    return s;
}

void
CVMJITfreeCodeCache(void *start)
{
    munmap(start, CVMglobals.jit.codeCacheSize);
}

#endif /* CVMJIT_HAVE_PLATFORM_SPECIFIC_ALLOC_FREE_CODECACHE */


#ifdef CVMJIT_TRAP_BASED_GC_CHECKS

/*
 * Enable gc rendezvous points in compiled code by denying access to
 * the page that is accessed at each rendezvous point. This will
 * cause a SEGV and handleSegv() will setup the gc rendezvous
 * when this happens.
 */
void
CVMJITenableRendezvousCallsTrapbased(CVMExecEnv* ee)
{
    /* Since we offset CVMglobals.jit.gcTrapAddr by 
       CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET words, we need to readjust and
       also map out twice this many words (positive and negative offsets)
    */
    int result =
	mprotect((void*)(CVMglobals.jit.gcTrapAddr -
			 CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET),
		 sizeof(void*) * 2 * CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET,
		 PROT_NONE);
    CVMassert(result == 0);
    (void)result;
}

/*
 * Disable gc rendezvous points be making the page readable again.
 */
void
CVMJITdisableRendezvousCallsTrapbased(CVMExecEnv* ee)
{
    int result = 
	mprotect((void*)(CVMglobals.jit.gcTrapAddr -
			 CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET),
		 sizeof(void*) * 2 * CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET,
		 PROT_READ);
    CVMassert(result == 0);
    (void)result;
}

#endif /* CVMJIT_TRAP_BASED_GC_CHECKS */
