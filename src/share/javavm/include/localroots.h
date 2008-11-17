/*
 * @(#)localroots.h	1.25 06/10/10
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

#ifndef _INCLUDED_LOCALROOTS_H
#define _INCLUDED_LOCALROOTS_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/stacks.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/gc_common.h"

/*
 * An CVMLocalRootsFrame is pushed each time CVMID_localrootBegin is called
 * and popped by CVMID_localrootEnd.
 *
 * There is no out-of-order deletion in this type of frame; therefore it is not
 * an CVMFreelistFrame
 */
struct CVMLocalRootsFrame {
    CVMFrame        cvmFrame;
    CVMStackVal32   vals[1];    /* space for local roots */
};

#define CVMID_localrootBegin(ee_)					\
{									\
    CVMExecEnv*         currentLRee_ = ee_;		       		\
    CVMStack*           lrStack_ = &((ee_)->localRootsStack);		\
    CVMFrame*           currentLRFrame_ = lrStack_->currentFrame;	\
    CVMassert(CVMD_isgcSafe(ee_));					\
    CVMpushGCRootFrame(ee_, lrStack_, currentLRFrame_,			\
		       CVMbaseFrameCapacity, CVM_FRAMETYPE_LOCALROOT);

#define CVMID_localrootEnd()						\
    CVMpopGCRootFrame(currentLRee_, lrStack_, currentLRFrame_);         \
}

#define CVMID_localrootBeginGcUnsafe(ee_)				     \
{									     \
    CVMExecEnv*         currentLRee_ = ee_;				     \
    CVMStack*           lrStack_ = &((ee_)->localRootsStack);		     \
    CVMFrame*           currentLRFrame_ = lrStack_->currentFrame;	     \
    CVMassert(CVMD_isgcUnsafe(ee_));					     \
    CVMpushGCRootFrameUnsafe(ee_, lrStack_, currentLRFrame_,		     \
		             CVMbaseFrameCapacity, CVM_FRAMETYPE_LOCALROOT);

#define CVMID_localrootEndGcUnsafe()					\
    CVMpopGCRootFrameUnsafe(currentLRee_, lrStack_, currentLRFrame_);	\
}


/*
 * The following function is private to the local roots implementation
 */
extern CVMObjectICell*
CVMIDprivate_allocateLocalRoot(CVMExecEnv* ee,
			       CVMStack* lrStack, CVMFrame* lrFrame);
extern CVMObjectICell*
CVMIDprivate_allocateLocalRootUnsafe(CVMExecEnv* ee,
				     CVMStack* lrStack, CVMFrame* lrFrame);

#define CVMID_localrootDeclare(T, v)			\
    T * v = (T *)CVMIDprivate_allocateLocalRoot(	\
	currentLRee_, lrStack_, currentLRFrame_)

#define CVMID_localrootDeclareGcUnsafe(T, v)		\
    T * v = (T *)CVMIDprivate_allocateLocalRootUnsafe(	\
        currentLRee_, lrStack_, currentLRFrame_)

/*
 * Scan local roots for GC, calling 'callback' on each
 */
extern CVMFrameGCScannerFunc CVMlocalrootFrameScanner;

#endif /* _INCLUDED_LOCALROOTS_H */

