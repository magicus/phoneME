/*
 * @(#)jcov_hash.h	1.14 06/10/10
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

#ifndef _JCOV_HASH_H
#define _JCOV_HASH_H

/* Generic support for hash tables */
/* based on src/share/tools/hprof/hprof_hash.h */
#include "jcov_types.h"

typedef struct jcov_bucket_t {
    struct jcov_bucket_t *next;
    void *content;
} jcov_bucket_t;

typedef struct {
    SIZE_T        n_entries;          /* total buckets in the table  */
    SIZE_T        size;               /* size of the top-level table */
    jcov_bucket_t **entries;
    UINTPTR_T     (*hash_f)(void *);  /* hash function       */
    SIZE_T        (*size_f)(void *);  /* content size        */
    int (*compare_f)(void *, void *); /* comparison function */
} jcov_hash_t;

jcov_hash_t *jcov_hash_new(SIZE_T    size,
                           UINTPTR_T (*hash_f)(void *),
                           SIZE_T    (*size_f)(void *),
                           int       (*compare_f)(void *, void *));
void *jcov_hash_iterate(jcov_hash_t *table,
                        void * (*f)(void *, void *),
                        void *arg);
void jcov_hash_remove(jcov_hash_t *table,
                      int (*f)(void *, void *),
                      void *arg);
void  *jcov_hash_lookup(jcov_hash_t *table, void *new);
void  *jcov_hash_put(jcov_hash_t *table, void *new);
void **jcov_hash_to_array(jcov_hash_t *table);

#endif /* _JCOV_HASH_H */
