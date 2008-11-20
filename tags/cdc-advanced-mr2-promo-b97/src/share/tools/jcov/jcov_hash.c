/*
 * @(#)jcov_hash.c	1.16 06/10/10
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

/* based on src/share/tools/hprof/hprof_hash.c */

#include "javavm/include/porting/ansi/stdlib.h"
#include "javavm/include/porting/ansi/string.h"
#include "jcov.h"
#include "jcov_types.h"
#include "jcov_hash.h"
#include "jcov_util.h"

/* create a hashtable */
jcov_hash_t *jcov_hash_new(SIZE_T    size,
                           UINTPTR_T (*hash_f)(void *),
                           SIZE_T    (*size_f)(void *),
                           int       (*compare_f)(void *, void *)) {
    jcov_hash_t *table = (jcov_hash_t*)jcov_calloc(sizeof(jcov_hash_t));
    table->n_entries = 0;
    table->size = size;
    table->entries = jcov_calloc(size * sizeof(jcov_bucket_t *));
    table->hash_f = hash_f;
    table->size_f = size_f;
    table->compare_f = compare_f;
    return table;
}

/* iterate through the hash table */
void *jcov_hash_iterate(jcov_hash_t *table,
                        void * (*f)(void *, void *), 
                        void *arg) {
    int i;
    for (i = 0; i < table->size; i++) {
        jcov_bucket_t *bucket = table->entries[i];
        while (bucket) {
            f(bucket->content, arg);
            bucket = bucket->next;
        }
    }
    return arg;
}

/* remove an entry from the hash table */
void jcov_hash_remove(jcov_hash_t *table, 
                      int (*f)(void *, void *), 
                      void *arg) {
    int i;
    for (i = 0; i < table->size; i++) {
        jcov_bucket_t **p = &(table->entries[i]);
        jcov_bucket_t *bucket;
        while ((bucket = *p)) {
            if (f(bucket->content, arg)) {
                *p = bucket->next;
                jcov_free(bucket->content);
                jcov_free(bucket);
                table->n_entries--;
                continue;
            }
            p = &(bucket->next);
        }
    }    
}

/*
 * lookup a hashtable, if present, a ptr to the contents is
 * returned, else a NULL.
 */
void *jcov_hash_lookup(jcov_hash_t *table, void *new) {
    int index;
    jcov_bucket_t **p;
    index = table->hash_f(new) % table->size;
    p = &(table->entries[index]);
    
    while (*p) {
        if (table->compare_f(new, (*p)->content) == 0) {
            return (*p)->content;
        }
        p = &((*p)->next);
    }
    return NULL;
}

/* put an entry into a hashtable
   CAUTION - this should be used only if 
   jcov_hash_lookup returns NULL */
void *jcov_hash_put(jcov_hash_t *table, void *new) {
    int index = table->hash_f(new) % table->size;
    int bytes = table->size_f(new);
    jcov_bucket_t *new_bucket = (jcov_bucket_t*)jcov_calloc(sizeof(jcov_bucket_t));
    void *ptr = jcov_calloc(bytes);

    memcpy(ptr, new, bytes);
    new_bucket->next = table->entries[index];
    new_bucket->content = ptr;
    table->entries[index] = new_bucket;
    table->n_entries++;
    return ptr;
}

void **jcov_hash_to_array(jcov_hash_t *table) {
    void **res = (void**)jcov_calloc(table->n_entries * sizeof(void*));
    int i, cnt = 0;
    for (i = 0; i < table->size; i++) {
        jcov_bucket_t *bucket = table->entries[i];
        while (bucket) {
            res[cnt++] = bucket->content;
            bucket = bucket->next;
        }
    }
    return res;
}
