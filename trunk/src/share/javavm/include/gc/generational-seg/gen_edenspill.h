/*
 * @(#)gen_edenspill.h	1.12 06/10/10
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

#include "javavm/include/gc/generational-seg/generational.h"

struct CVMGenEdenSpillGeneration {
    CVMGeneration gen;

    /* To-do list (link list of objects) */
    CVMObject*    todoList;

    /* Preserved header words */
    CVMGenPreservedItem*   preservedItems;
    CVMUint32        preservedIdx;
    CVMUint32        preservedSize;
    
    /* Heap space */
    CVMGenSpace*     theSpace;      /* The space allocated for this gen. */

    CVMBool          promotions;    /* indicate if promotions occurred */
};

typedef struct CVMGenEdenSpillGeneration CVMGenEdenSpillGeneration;

CVMGeneration*
CVMgenEdenSpillAlloc(CVMUint32* space, CVMUint32 totalNumBytes);

/*
 * Free one of these generations
 */
void
CVMgenEdenSpillFree(CVMGenEdenSpillGeneration* thisGen);

