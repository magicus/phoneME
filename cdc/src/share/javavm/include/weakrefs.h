/*
 * @(#)weakrefs.h	1.18 06/10/10
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

/*
 * This file includes functions that are pertinent to weak reference
 * handling.
 */

#ifndef _INCLUDED_GC_WEAKREFS_H
#define _INCLUDED_GC_WEAKREFS_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/stacks.h"
#include "javavm/include/gc/gc_impl.h"
#include "generated/offsets/java_lang_ref_Reference.h"

/*
 * Initializing the weakref module
 */
extern void
CVMweakrefInit();

/*
 * Discover a new weak reference instance
 */
extern void
CVMweakrefDiscover(CVMExecEnv* ee, CVMObject* weakRef);

/*
 * Process weak references that don't have strong references to them.
 */
extern void
CVMweakrefProcessNonStrong(CVMExecEnv* ee,
			   CVMRefLivenessQueryFunc isLive,
			   void* isLiveData,
			   CVMRefCallbackFunc transitiveScanner,
			   void* transitiveScannerData,
			   CVMGCOptions* gcOpts);

/*
 * If CVMweakrefProcessNonStrong() fails, call
 * CVMweakrefRollbackHandling() to undo all that it's done this far 
 */
extern void
CVMweakrefRollbackHandling(CVMExecEnv* ee,
			   CVMGCOptions* gcOpts,
			   CVMRefCallbackFunc rootRollbackFunction,
			   void* rootRollbackData);

/* 
 * The second part of weakrefs handling, to be called on a successful
 * CVMweakrefProcessNonStrong(). 
 */
void
CVMweakrefFinalizeProcessing(CVMExecEnv* ee,
			     CVMRefLivenessQueryFunc isLive, void* isLiveData,
			     CVMRefCallbackFunc transitiveScanner,
			     void* transitiveScannerData,
			     CVMGCOptions* gcOpts);

/*
 * Update weak references 
 */
extern void
CVMweakrefUpdate(CVMExecEnv* ee,
		 CVMRefCallbackFunc refUpdate, void* updateData,
		 CVMGCOptions* gcOpts);

/* Clean-up due to GC aborting before FinalizeProcessing is called. */
extern void
CVMweakrefCleanUpForGCAbort(CVMExecEnv* ee);

/*
 * This is used to get a named field of a java.lang.ref.Referemce subclass
 */
#define CVMweakrefFieldAddr(weakRef, fieldName) \
    ((CVMObject**)(weakRef) + CVMoffsetOfjava_lang_ref_Reference_##fieldName)

#define CVMweakrefField(weakRef, fieldName) \
    *CVMweakrefFieldAddr(weakRef, fieldName)

#define CVMweakrefFieldSet(weakRef, fieldName, rhs)			\
    CVMD_fieldWriteRef(weakRef, 					\
		       CVMoffsetOfjava_lang_ref_Reference_##fieldName,	\
		       rhs)

#endif /* _INCLUDED_GC_WEAKREFS_H */
