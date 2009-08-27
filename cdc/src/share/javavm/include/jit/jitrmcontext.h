/*
 * @(#)jitrmcontext.h	1.17 06/10/10
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

#ifndef _INCLUDED_JIT_RMCONTEXT_H
#define _INCLUDED_JIT_RMCONTEXT_H

#include "javavm/include/defs.h"
#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/jit/jitset.h"
#include "javavm/include/porting/jit/jit.h"

/*
 * Common Register information.
 * There is only one of these, which is shared by all
 * register managers.
 * It is found in the compilation context.
 * It contains the spill information.
 */
struct CVMJITRMCommonContext{
    CVMInt32		      maxSpillNumber; /* can be negative (-1) */
    CVMJITSet		      spillBusySet;
    CVMJITSet		      spillRefSet;
    CVMUint8*		      spillBusyRefCount;
    CVMInt32		      spillBusyRefSize;
    CVMUint16		      maxPhiSize;
    CVMRMResource*	      freeResourceList;
};

/* subroutine pointer type(s) */
typedef void
(*CVMJITRMConstantLoaderAddr)(CVMJITCompilationContext*, int, CVMAddr);
typedef void
(*CVMJITRMConstantLoader32)(CVMJITCompilationContext*, int, CVMInt32);

/*
 * Register management state.
 * There is one of these for each bank of registers to be managed.
 */
struct CVMJITRMContext {
    CVMRMResource*	      resourceList;
    CVMRMResource*	      freeResourceList;
#ifdef CVMCPU_HAS_ZERO_REG
    CVMRMResource*            preallocatedResourceForConstantZero;
#endif
    CVMRMregset		      occupiedRegisters;
    CVMRMregset		      pinnedRegisters;
    CVMRMregset		      phiPinnedRegisters;
    CVMRMregset		      phiRegSet;
    CVMRMregset		      sandboxRegSet;
    CVMRMregset               busySet;
    CVMRMregset               safeSet;
    CVMRMregset               unsafeSet;
    CVMRMregset               anySet;

    /* Used during phi iteration */
    CVMRMregset		      phiFreeRegSet;

    CVMRMResource**	      reg2res;
    CVMRMResource**	      local2res;
#ifdef CVM_JIT_REGISTER_LOCALS
    CVMRMResource**	      pinnedOutgoingLocals;
#endif
    CVMJITCompilationContext* compilationContext; 
    CVMUint32		      loadOpcode[2];
    CVMUint32		      storeOpcode[2];
    CVMUint32		      movOpcode;
    CVMJITRMConstantLoader32    constantLoader32;
    CVMJITRMConstantLoaderAddr    constantLoaderAddr;
#ifdef CVM_DEBUG_ASSERTS
    int			      key;
#endif
    CVMUint16		      numberLocalWords;
    CVMUint16		      minInteresting;
    CVMUint16		      maxInteresting;
    CVMUint8		      singleRegAlignment;
    CVMUint8		      doubleRegAlignment;
    CVMUint8		      maxRegSize; /* in words! */
};


#endif /* _INCLUDED_JIT_RMCONTEXT_H */
