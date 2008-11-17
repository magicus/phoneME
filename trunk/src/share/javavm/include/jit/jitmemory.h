/*
 * @(#)jitmemory.h	1.6 06/10/10
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
 * Functions for managing memory allocated during compilation.
 */

#ifndef _INCLUDED_JITMEMORY_H
#define _INCLUDED_JITMEMORY_H

#include "javavm/include/assert.h"
#include "javavm/include/jit/jit_defs.h"

enum CVMJITAllocationTag {
    JIT_ALLOC_IRGEN_NODE = 0,/* IR node allocation */
    JIT_ALLOC_IRGEN_OTHER,   /* Other front-end allocation */
    JIT_ALLOC_OPTIMIZER,     /* Optimizer allocations */
    JIT_ALLOC_CGEN_REGMAN,   /* Code generator working memory for registers */
    JIT_ALLOC_CGEN_ALURHS,   /* Code generator working memory for aluRhs */
    JIT_ALLOC_CGEN_MEMSPEC,  /* Code generator working memory for memSpec */
    JIT_ALLOC_CGEN_FIXUP,    /* Code generator working memory for fixups */
    JIT_ALLOC_CGEN_OTHER,    /* Other code generator working memory */
    JIT_ALLOC_COLLECTIONS,   /* Allocation for sets, growable arrays, etc. */
    JIT_ALLOC_DEBUGGING,     /* Allocations for debugging purposes */
#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
    JIT_ALLOC_CODE_SCHEDULING, /* Allocations for code scheduling purposes */
#endif /* IAI_CODE_SCHEDULER_SCORE_BOARD */
    JIT_ALLOC_NUM_ALLOCATION_KINDS
};
typedef enum CVMJITAllocationTag CVMJITAllocationTag;

/*
 * Memory allocation. A few well-known sizes are expected: we expect
 * allocations for IR nodes, for blocks and for sets. This allocator
 * should eventually be tuned for those sizes.
 *
 * This a tagged allocator. See the enum CVMJITAllocationKind above for
 * various kinds.
 */
extern void
CVMJITmemInitialize(CVMJITCompilationContext* con);

/* Purpose: Allocates scratch memory for the JIT's use.  The allocated memory
            is filled with 0s. */
extern void*
CVMJITmemNewInternal(CVMJITCompilationContext *con, CVMJITAllocationTag tag,
    CVMUint32 size DECLARE_CALLER_INFO(callerFilename, callerLineNumber));

#define CVMJITmemNew(con, tag, size) \
    CVMJITmemNewInternal((con), (tag), (size) PASS_CALLER_INFO())

/*
 * Flush all memory allocated with CVMJITmemNew()
 */
extern void
CVMJITmemFlush(CVMJITCompilationContext* con);

/*
 * Separate "permanent" code buffer allocation from temporary allocations
 */
extern void*
CVMJITmemNewLongLivedMemoryInternal(CVMJITCompilationContext *con,
    CVMUint32 size DECLARE_CALLER_INFO(callerFilename, callerLineNumber));

#define CVMJITmemNewLongLivedMemory(con, size) \
    CVMJITmemNewLongLivedMemoryInternal((con), (size) PASS_CALLER_INFO())

extern void*
CVMJITmemExpandLongLivedMemoryInternal(CVMJITCompilationContext *con, 
    void *ptr, CVMUint32 size
    DECLARE_CALLER_INFO(callerFilename, callerLineNumber));

#define CVMJITmemExpandLongLivedMemory(con, ptr, size) \
    CVMJITmemExpandLongLivedMemoryInternal((con), (ptr), (size) \
        PASS_CALLER_INFO())

/*
 * Release memory acquired by CVMJITmemNewLongLivedMemory()
 */
extern void
CVMJITmemFreeLongLivedMemoryInternal(void *chunk
    DECLARE_CALLER_INFO(callerFilename, callerLineNumber));

#define CVMJITmemFreeLongLivedMemory(chunk) \
    CVMJITmemFreeLongLivedMemoryInternal((chunk) PASS_CALLER_INFO())

#endif /* _INCLUDED_JITMEMORY_H */
