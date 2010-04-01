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
#include "javavm/include/porting/ansi/setjmp.h"

/*
 * This file is generated from the GC choice given at build time.
 */
#include "generated/javavm/include/gc_config.h"

#include "javavm/include/gc_common.h"
#include "javavm/include/gc/gc_impl.h"

#include "javavm/include/gc/generational/generational.h"

typedef struct CVMMCPreservedItem {
    CVMObject* movedRef;
    CVMAddr    originalWord;
} CVMMCPreservedItem;

struct CVMGenMarkCompactGeneration {
    CVMGeneration gen;

    /* To-do list (linked list of objects) for the entire heap: */
    CVMObject*    todoList;
    
    /* Preserved header words for the entire heap: */
    CVMMCPreservedItem*   preservedItems;
    CVMUint32             preservedIdx;
    CVMUint32             preservedSize;

    volatile int          gcPhase;
    CVMObject *           lastProcessedRef;
    jmp_buf               errorContext;
};

typedef struct CVMGenMarkCompactGeneration CVMGenMarkCompactGeneration;

/*
 * Allocate a new one of these generations
 */
extern CVMGeneration* 
CVMgenMarkCompactAlloc(CVMUint32* space, CVMUint32 totalNumBytes);

/*
 * Free one of these generations
 */
void
CVMgenMarkCompactFree(CVMGenMarkCompactGeneration* thisGen);

/* Rebuild the old-to-young pointer tables. */
void
CVMgenMarkCompactRebuildBarrierTable(CVMGenMarkCompactGeneration* thisGen,
				     CVMExecEnv* ee,
				     CVMGCOptions* gcOpts,
				     CVMUint32* startRange,
				     CVMUint32* endRange);

#if defined(CVM_DEBUG) || defined(CVM_INSPECTOR)
/* Dumps info about the configuration of the markcompact generation. */
void CVMgenMarkCompactDumpSysInfo(CVMGenMarkCompactGeneration* thisGen);
#endif
