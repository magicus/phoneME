/*
 * @(#)jit_md.c	1.2 04/02/24
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
#include <sys/mman.h>

/*
 * Enable gc rendezvous points in compiled code by denying access to
 * the page that is accessed at each rendezvous point. This will
 * cause a SEGV and handleSegv() will setup the gc rendezvous
 * when this happens.
 */
void
CVMJITenableRendezvousCallsTrapbased(CVMExecEnv* ee)
{
}

/*
 * Disable gc rendezvous points be making the page readable again.
 */
void
CVMJITdisableRendezvousCallsTrapbased(CVMExecEnv* ee)
{
}

#endif /* CVMJIT_TRAP_BASED_GC_CHECKS */
