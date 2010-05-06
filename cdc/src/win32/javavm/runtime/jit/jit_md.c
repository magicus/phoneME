/*
 * @(#)jit_md.c	1.4 05/06/21
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

#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#include <WinBase.h>

/*
 * Enable gc rendezvous points in compiled code by denying access to
 * the page that is accessed at each rendezvous point. This will
 * cause a GP Fault and handleException() will setup the gc rendezvous
 * when this happens.
 */
void
CVMJITenableRendezvousCallsTrapbased(CVMExecEnv* ee)
{

    DWORD oldprotect;

    /* 
     * Since we offset CVMglobals.jit.gcTrapAddr by 
     * CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET words, we need to readjust and
     * also map out twice this many words (positive and negative offsets)
     */
    BOOL result = VirtualProtect(CVMglobals.jit.gcTrapAddr - 
                                 CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET,
		                 sizeof(void*) * 2 * 
                                 CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET,
		 		 PAGE_NOACCESS, &oldprotect);
    CVMassert(result);
    (void)result;
}

/*
 * Disable gc rendezvous points be making the page readable again.
 */
void
CVMJITdisableRendezvousCallsTrapbased(CVMExecEnv* ee)
{
    DWORD oldprotect;

    BOOL result = VirtualProtect(CVMglobals.jit.gcTrapAddr - 
                                 CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET,
		 		 sizeof(void*) * 2 * 
				 CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET,
		 		 PAGE_READONLY, &oldprotect);
    CVMassert(result);
    (void)result;
}

#endif /* CVMJIT_TRAP_BASED_GC_CHECKS */
