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

#ifndef HPROF_TABLE_H
#define HPROF_TABLE_H

/* Key based generic lookup table */

struct LookupTable;

typedef void (*LookupTableIterator)
                (TableIndex, void *key_ptr, int key_len, void*, void*);

struct LookupTable * table_initialize(const char *name, int size, 
                                int incr, int buckets, int esize);
int                  table_element_count(struct LookupTable *ltable);
TableIndex           table_create_entry(struct LookupTable *ltable,
                                void *key_ptr, int key_len, void *info_ptr);
TableIndex           table_find_entry(struct LookupTable *ltable,
                                void *key_ptr, int key_len);
TableIndex           table_find_or_create_entry(struct LookupTable *ltable,
                                void *key_ptr, int key_len, 
				jboolean *pnew_entry, void *info_ptr);
void                 table_free_entry(struct LookupTable *ltable,
				TableIndex index);
void                 table_cleanup(struct LookupTable *ltable,
                                LookupTableIterator func, void *arg);
void                 table_walk_items(struct LookupTable *ltable,
                                LookupTableIterator func, void *arg);
void *               table_get_info(struct LookupTable *ltable, 
                                TableIndex index);
void                 table_get_key(struct LookupTable *ltable, 
                                TableIndex index, void **pkey_ptr, 
				int *pkey_len);
void                 table_lock_enter(struct LookupTable *ltable);
void                 table_lock_exit(struct LookupTable *ltable);

#endif

