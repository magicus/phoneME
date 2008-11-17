/*
 * @(#)hash.h	1.6 06/10/10
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

#ifndef _HASH_H_
#define _HASH_H_

// A very hash table for strings. Can be used in a symbol table to cut
// down on internal string duplication, but more frequently when using
// strings as lookup keys: first "hash" the sting to yield a 32-bit (char *),
// then use that value as a key.
//

#define HASHTAB_BITS	10
#define HASHTAB_BUCKETS	(1 << HASHTAB_BITS)

class pool_alloc;

class hashtab {
	struct hashel {
		char * he_charp;
		struct hashel * he_next;
		unsigned int he_num;
	};
		/* actual heads of linked lists */
	struct hashel * he_hashhead[ HASHTAB_BUCKETS ];

		/* a memory pool to cut down on malloc calls */
	pool_alloc * he_memory_pool;

public:
		/* constructor to initialize the hashtable */
	hashtab();

		/* enter one string into the hashtable */
	char * enhash( const char * name );

		/* destructor to free the hashtable and re-initialize */
	void destruct();

}; // end hashtab

#endif // ! _HASH_H_
