/*
 * @(#)hash.cc	1.6 06/10/10
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

#include "hash.h"
#include "pool_alloc.h"
#include <string.h>
#include <stdio.h>


hashtab::hashtab()
{
	int i;

		/* get a new memory pool allocator */
	he_memory_pool = new pool_alloc( sizeof(struct hashel) );

		/* null out the bucket linked list heads */
	for (i = 0; i < (sizeof(he_hashhead) / sizeof(he_hashhead[0])); i += 1) {
		he_hashhead[i] = (struct hashel *) NULL;
	}
}


char *
hashtab::enhash( const char * name )
{
	struct hashel *p, **pp;
	unsigned int bucket;
	char *cp;

	//CG_ASSERT( name != CG_NULL, "hashtab.enhash: bad string" );

	/* hash the string into a hash_num and a length */
	unsigned int hash_num = 0;
	int len = 1;
	cp = (char *) name;
	while (*cp != '\0') {
		hash_num = (hash_num * 17) + *cp;
		len += 1;
		cp += 1;
	}
	hash_num = hash_num + (hash_num >> 16);
	bucket = hash_num & ((1 << HASHTAB_BITS) - 1);

	pp = &(he_hashhead[bucket]);
	while ( (p = *pp) != NULL ) {
		if ( p->he_num == hash_num && strcmp( p->he_charp, name) == 0) {
			return p->he_charp; // found it in the dict.
		}
		pp = & (p->he_next);
	}

	/* must add to list */
	//p = new hashel;
	//p = (struct hashel *) malloc( sizeof(struct hashel) );
	p = (struct hashel *) he_memory_pool->mem( sizeof(struct hashel) );
	p->he_num = hash_num;
	p->he_next = (struct hashel *) NULL;
	p->he_charp = (char *) he_memory_pool->mem( len );
	memcpy( p->he_charp, name, len );
	*pp = p;

	return p->he_charp;
}


void
hashtab::destruct()
{
	delete he_memory_pool;

	int i;

	for (i = 0; i < (sizeof(he_hashhead) / sizeof(he_hashhead[0])); i += 1) {
		/* re-initialize */
		he_hashhead[i] = (struct hashel *) NULL;
	}
	return;
}

