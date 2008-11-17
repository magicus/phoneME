/*
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
 * This file includes the implementation of a copying semispace generation.
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/directmem.h"

/*
 * This file is generated from the GC choice given at build time.
 */
#include "generated/javavm/include/gc_config.h"

#include "javavm/include/gc_common.h"
#include "javavm/include/gc/gc_impl.h"

#include "javavm/include/gc/generational/generational.h"

typedef struct CVMGenSemispaceScanStackEntry CVMGenSemispaceScanStackEntry;
struct CVMGenSemispaceScanStackEntry {
    CVMObject *obj;
    CVMAddr offset;
};

#define CVMGC_MAX_SCAN_STACK 32

typedef struct CVMGenSemispaceGeneration {
    CVMGeneration gen;
    CVMGenSpace*  fromSpace;     /* Semispace #1 */
    CVMGenSpace*  toSpace;       /* Semispace #2 */
    CVMUint32*    copyBase;      /* The base pointer for copying */
    CVMUint32*    copyTop;       /* The top pointer for copying */
    CVMUint32*    copyThreshold; /* The top pointer for copying */

    CVMBool hasFailedPromotion;  /* Attempt to promote to oldGen failed. */

    /* Stack context for depth first scanning: */
    CVMGenSemispaceScanStackEntry  scanStack[CVMGC_MAX_SCAN_STACK];

} CVMGenSemispaceGeneration;

/*
 * Allocate a new one of these generations
 */
extern CVMGeneration* 
CVMgenSemispaceAlloc(CVMUint32* space, CVMUint32 totalNumBytes);

/*
 * Free one of these generations
 */
void
CVMgenSemispaceFree(CVMGenSemispaceGeneration* thisGen);

#if defined(CVM_DEBUG) || defined(CVM_INSPECTOR)
/* Dumps info about the configuration of the semispace generation. */
void CVMgenSemispaceDumpSysInfo(CVMGenSemispaceGeneration* thisGen);
#endif
