/*
 * @(#)ccee.h	1.26 06/10/10
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

#ifndef _INCLUDED_CCEE_H
#define _INCLUDED_CCEE_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"

/*===========================================================================*/

struct CVMCCExecEnv
{
    CVMExecEnv    *eeX;
    CVMStackVal32 *stackChunkEndX;
    CVMUint8*     ccmGCRendezvousGlue;
    void**        gcTrapAddr; /* same as CVMglobals.jit.gcTrapAddr */
    CVMUint32     ccmStorage[16]; /* storage for frameless asm helpers */
};

/* Purpose: Gets the CVMExecEnv for the specified CVMCCExecEnv. */
#define CVMcceeGetEE(/* CVMCCExecEnv * */ ccee_) \
    ((ccee_)->eeX)

#ifdef CVM_DEBUG
/* We better not be holding a microlock */
#define CVMCCM_FIXUP_CHECKS(ee)   CVMassert(ee->microLock == 0);
#else
#define CVMCCM_FIXUP_CHECKS(ee)
#endif

/* Purpose: Do lazy fixup frames for CCM runtime functions. */
#define CVMCCMruntimeLazyFixups(ee)					\
{									\
    CVMFrame *frame = CVMeeGetCurrentFrame(ee);				\
    CVMCCM_FIXUP_CHECKS(ee)						\
    if (!CVMframeMaskBitsAreCorrect(frame)) {				\
	CVMJITfixupFrames(frame);					\
    }									\
}

/* Purpose: Handles exceptions caused by compiled code by returning
            directly to the interpreter so it can handle the exception. */
extern void
CVMCCMhandleException(CVMCCExecEnv *ccee);

/*===========================================================================*/

#endif /* _INCLUDED_CCEE_H */
