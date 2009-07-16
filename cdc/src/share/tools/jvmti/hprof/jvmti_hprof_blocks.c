/*
 * jvmti hprof
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

/* Allocations from large blocks, no individual free's */

#include "jvmti_hprof.h"

/*
 * This file contains some allocation code that allows you
 *   to have space allocated via larger blocks of space.
 * The only free allowed is of all the blocks and all the elements.
 * Elements can be of different alignments and fixed or variable sized.
 * The space allocated never moves.
 *
 */

/* Get the real size allocated based on alignment and bytes needed */
static int
real_size(int alignment, int nbytes)
{
    if ( alignment > 1 ) {
	int wasted;

	wasted = alignment - ( nbytes % alignment );
	if ( wasted != alignment ) {
	    nbytes += wasted;
	}
    }
    return nbytes;
}

/* Add a new current_block to the Blocks* chain, adjust size if nbytes big. */
static void
add_block(Blocks *blocks, int nbytes)
{
    int header_size;
    int block_size;
    BlockHeader *block_header;

    HPROF_ASSERT(blocks!=NULL);
    HPROF_ASSERT(nbytes>0);

    header_size          = real_size(blocks->alignment, sizeof(BlockHeader));
    block_size           = blocks->elem_size*blocks->population;
    if ( nbytes > block_size ) {
	block_size = real_size(blocks->alignment, nbytes);
    }
    block_header         = (BlockHeader*)HPROF_MALLOC(block_size+header_size);
    block_header->next   = NULL;
    block_header->bytes_left = block_size;
    block_header->next_pos   = header_size;
   
    /* Link in new block */
    if ( blocks->current_block != NULL ) {
        blocks->current_block->next = block_header;
    }
    blocks->current_block = block_header;
    if ( blocks->first_block == NULL ) {
        blocks->first_block = block_header;
    }
}

/* Initialize a new Blocks */
Blocks *
blocks_init(int alignment, int elem_size, int population)
{
    Blocks *blocks;

    HPROF_ASSERT(alignment>0);
    HPROF_ASSERT(elem_size>0);
    HPROF_ASSERT(population>0);

    blocks                = (Blocks*)HPROF_MALLOC(sizeof(Blocks));
    blocks->alignment     = alignment;
    blocks->elem_size     = elem_size;
    blocks->population    = population;
    blocks->first_block   = NULL;
    blocks->current_block = NULL;
    return blocks;
}

/* Allocate bytes from a Blocks area. */
void *
blocks_alloc(Blocks *blocks, int nbytes)
{
    BlockHeader *block;
    int   pos;
    void *ptr;

    HPROF_ASSERT(blocks!=NULL);
    HPROF_ASSERT(nbytes>=0);
    if ( nbytes == 0 ) {
	return NULL;
    }
    
    block = blocks->current_block;
    nbytes = real_size(blocks->alignment, nbytes);
    if ( block == NULL || block->bytes_left < nbytes ) {
        add_block(blocks, nbytes);
        block = blocks->current_block;
    }
    pos = block->next_pos;
    ptr = (void*)(((char*)block)+pos);
    block->next_pos   += nbytes;
    block->bytes_left -= nbytes;
    return ptr;
}

/* Terminate the Blocks */
void
blocks_term(Blocks *blocks)
{
    BlockHeader *block;

    HPROF_ASSERT(blocks!=NULL);
    
    block = blocks->first_block;
    while ( block != NULL ) {
	BlockHeader *next_block;

	next_block = block->next;
	HPROF_FREE(block);
	block = next_block;
    }
    HPROF_FREE(blocks);
}

