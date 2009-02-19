/*
 * @(#)hprof_hash.h	1.13 06/10/10
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

#ifndef _HPROF_HASH_H
#define _HPROF_HASH_H

/* Generic support for hash tables */

typedef struct hprof_bucket_t {
    struct hprof_bucket_t *next;
    void *content;
} hprof_bucket_t;

typedef struct {
#ifdef WATCH_ALLOCS
    unsigned int table_type;           /* ALLOC_HASH_... value */
#endif /* WATCH_ALLOCS */
    unsigned int n_entries;            /* total buckets in the table */
    unsigned int size;                 /* size of the top-level table */
    hprof_bucket_t **entries;
    unsigned int (*hash_f)(void *);    /* hash function */
    unsigned int (*size_f)(void *);    /* content size */
    int (*compare_f)(void *, void *);  /* comparison function */
#ifdef HASH_STATS
    unsigned int table_deletes;        /* # of entries deleted */
    unsigned int table_hits;           /* # of hash hits */
    unsigned int table_misses;         /* # of hash misses */
#endif /* HASH_STATS */
} hprof_hash_t;

void hprof_hash_init(int table_type,
		     hprof_hash_t *table,
		     int size,
		     unsigned int (*hash_f)(void *),
		     unsigned int (*size_f)(void *),
		     int (*compare_f)(void *, void *));
void * hprof_hash_iterate(hprof_hash_t *table,
			  void * (*f)(void *, void *),
			  void *arg);
void hprof_hash_remove(hprof_hash_t *table,
		       int (*f)(void *, void *),
		       void *arg);
void hprof_hash_removeall(hprof_hash_t *table);
void * hprof_hash_lookup(hprof_hash_t *table, void *new);
void * hprof_hash_put(hprof_hash_t *table, void *new);
void * hprof_hash_intern(hprof_hash_t *table, void *new);
#ifdef HASH_STATS
void hprof_print_global_hash_stats(FILE *fp);
void hprof_print_tbl_hash_stats(FILE *fp, hprof_hash_t *table);
#endif /* HASH_STATS */
#ifdef WATCH_ALLOCS
void hprof_hash_free(hprof_hash_t *table);
#endif /* WATCH_ALLOCS */

#endif /* _HPROF_HASH_H */
