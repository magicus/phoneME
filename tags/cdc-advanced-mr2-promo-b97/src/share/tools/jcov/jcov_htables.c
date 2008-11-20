/*
 * @(#)jcov_htables.c	1.17 06/10/10
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

#include "javavm/include/porting/ansi/stdlib.h"
#include "javavm/include/porting/ansi/string.h"

#include "jvmpi.h"

#include "jcov.h"
#include "jcov_types.h"
#include "jcov_hash.h"
#include "jcov_htables.h"
#include "jcov_setup.h"
#include "jcov_util.h"

/* ====================================*/
/* === functions related to classes ===*/
/* ====================================*/

static UINTPTR_T hash_string(char *str) {
    int i;
    UINTPTR_T hash = 0;
    for (i = 0; i < strlen(str); i++) {
        hash = 37*hash + str[i];
    }
    return hash;
}

UINTPTR_T hash_class_id(void *c) {
    return (UINTPTR_T)(*(jcov_class_t**)c)->id;
}

UINTPTR_T hash_hooked_class(void *c) {
    jcov_hooked_class_t *class = *(jcov_hooked_class_t**)c;
    return hash_string(class->name);
}

UINTPTR_T hash_class_key(void *c) {
    UINTPTR_T hash = 0;
    jcov_class_t *class = *(jcov_class_t**)c;
    hash += hash_string(class->name);
    hash += hash_string(class->src_name);
    hash += hash_string(class->timestamp);
    return hash;
}

UINTPTR_T hash_class_key_short(void *c) {
    jcov_class_t *class = *(jcov_class_t**)c;
    return hash_string(class->name);
}

SIZE_T size_class(void *m) {
    return sizeof(jcov_class_t*);
}

SIZE_T size_hooked_class(void *c) {
    return sizeof(jcov_hooked_class_t*);
}

int cmp_hooked_classes(void *c1, void *c2) {
    jcov_hooked_class_t *class1 = *(jcov_hooked_class_t**)c1;
    jcov_hooked_class_t *class2 = *(jcov_hooked_class_t**)c2;
    return strcmp(class1->name, class2->name);
}

int cmp_hooked_classes_not(void *c1, void *c2) {
    return !cmp_hooked_classes(c1, c2);
}

int cmp_class_key(void *c1, void *c2) {
    int cmp_res;
    jcov_class_t *class1 = *(jcov_class_t**)c1;
    jcov_class_t *class2 = *(jcov_class_t**)c2;

    cmp_res = strcmp(class1->name, class2->name);
    if (cmp_res) {
        return cmp_res;
    }
    cmp_res = strcmp(class1->src_name, class2->src_name);
    if (cmp_res) {
        return cmp_res;
    }
    return strcmp(class1->timestamp, class2->timestamp);
}

int cmp_class_key_short(void *c1, void *c2) {
    jcov_class_t *class1 = *(jcov_class_t**)c1;
    jcov_class_t *class2 = *(jcov_class_t**)c2;

    return strcmp(class1->name, class2->name);
}

int cmp_class_id(void *c1, void *c2) {
    jcov_class_t *class1 = *(jcov_class_t**)c1;
    jcov_class_t *class2 = *(jcov_class_t**)c2;

    return (int)((UINTPTR_T)class1->id - (UINTPTR_T)class2->id);
}

static int cmp_class_id_not(void *c1, void *c2) {
    return !cmp_class_id(c1, c2);
}

void hash_table_iterate(jcov_hash_t *table, void *(*f)(void *, void *)) {
    jcov_hash_iterate(table, f, NULL);
}

jcov_class_t *lookup_class_by_id(jcov_hash_t *table, jobjectID class_id) {
    jcov_class_t **ptr;
    jcov_class_t class_tmp;
    jcov_class_t *class_tmp_addr = &class_tmp;
    class_tmp.id = class_id;
    ptr = (jcov_class_t**)jcov_hash_lookup(table, &class_tmp_addr);
    return (ptr ? *ptr : NULL);
}

jcov_class_t *lookup_class_by_key(jcov_hash_t *table, jcov_class_t *class) {
    jcov_class_t **ptr;
    ptr = (jcov_class_t**)jcov_hash_lookup(table, &class);
    return (ptr ? *ptr : NULL);
}

jcov_class_t *lookup_class_by_key_short(jcov_hash_t *table, jcov_class_t *class) {
    jcov_class_t **ptr;
    int (*cmp_sav)(void *, void *) = table->compare_f;
    UINTPTR_T (*hash_sav)(void *)  = table->hash_f;
    table->compare_f = cmp_class_key_short;
    table->hash_f    = hash_class_key_short;
    ptr = (jcov_class_t**)jcov_hash_lookup(table, &class);
    table->compare_f = cmp_sav;
    table->hash_f    = hash_sav;
    return (ptr ? *ptr : NULL);
}

jcov_hooked_class_t *lookup_hooked_class(jcov_hash_t *table, char *name) {
    jcov_hooked_class_t **ptr;
    jcov_hooked_class_t class_tmp;
    jcov_hooked_class_t *class_tmp_addr = &class_tmp;
    class_tmp.name = name;
    ptr = (jcov_hooked_class_t**)jcov_hash_lookup(table, &class_tmp_addr);
    return (ptr ? *ptr : NULL);
}

jcov_class_t *put_class_by_id(jcov_hash_t *table, jcov_class_t **c) {
    jcov_class_t **ptr;
    ptr = (jcov_class_t**)jcov_hash_put(table, c);
    return *ptr;
}

jcov_class_t *put_class_by_key(jcov_hash_t *table, jcov_class_t **c) {
    jcov_class_t **ptr;
    ptr = (jcov_class_t**)jcov_hash_put(table, c);
    return *ptr;
}

jcov_hooked_class_t *put_hooked_class(jcov_hash_t *table, jcov_hooked_class_t **c) {
    jcov_hooked_class_t **ptr;
    ptr = (jcov_hooked_class_t**)jcov_hash_put(table, c);
    return *ptr;
}

void remove_hooked_class(jcov_hash_t *table, jcov_hooked_class_t *c) {
    jcov_hash_remove(table, cmp_hooked_classes_not, &c);
}

void remove_class_by_id(jcov_hash_t *table, jobjectID class_id) {
    jcov_class_t class_tmp;
    jcov_class_t *class_tmp_addr = &class_tmp;
    class_tmp.id = class_id;
    jcov_hash_remove(table, cmp_class_id_not, &class_tmp_addr);
}

int find_method_in_class(jcov_class_t *c, JVMPI_Method *m) {
    int i;
    for (i = 0; i < c->num_methods; i++) {
        if (!strcmp((c->methods[i]).method_name, m->method_name) &&
            !strcmp((c->methods[i]).method_signature, m->method_signature)) {
            return i;
        }
    }
    return -1;
}

/* ======================================*/
/* === functions related to jobjectID ===*/
/* ======================================*/

UINTPTR_T hash_classID(void *m) {
    return (UINTPTR_T)(*(jcov_class_id_t**)m)->id;
}

SIZE_T size_classID(void *m) {
    return sizeof(jcov_class_id_t*);
}

int cmp_classID(void *m1, void *m2) {
    return (int)((UINTPTR_T)(*(jcov_class_id_t**)m1)->id -
                 (UINTPTR_T)(*(jcov_class_id_t**)m2)->id);
}

int cmp_classID_not(void *m1, void *m2) {
    return !cmp_classID(m1, m2);
}

jcov_class_id_t *lookup_classID(jcov_hash_t *table, jobjectID class_id) {
    jcov_class_id_t **ptr;
    jcov_class_id_t id_tmp;
    jcov_class_id_t *id_tmp_addr = &id_tmp;
    id_tmp.id = class_id;
    ptr = (jcov_class_id_t**)jcov_hash_lookup(table, &id_tmp_addr);
    return (ptr ? *ptr : NULL);
}

jcov_class_id_t *put_classID(jcov_hash_t *table, jcov_class_id_t **m) {
    jcov_class_id_t **ptr;
    ptr = (jcov_class_id_t**)jcov_hash_put(table, m);
    return *ptr;
}

void remove_classID(jcov_hash_t *table, jobjectID class_id) {
    jcov_class_id_t id_tmp;
    jcov_class_id_t *id_tmp_addr = &id_tmp;
    id_tmp.id = class_id;
    jcov_hash_remove(table, cmp_classID_not, &id_tmp_addr);
}

/* ====================================*/
/* === functions related to methods ===*/
/* ====================================*/

UINTPTR_T hash_method(void *m) {
    return (UINTPTR_T)(*(jcov_method_t**)m)->id;
}

SIZE_T size_method(void *m) {
    return sizeof(jcov_method_t*);
}

int cmp_method(void *m1, void *m2) {
    return (int)((UINTPTR_T)(*(jcov_method_t**)m1)->id -
                 (UINTPTR_T)(*(jcov_method_t**)m2)->id);
}

int cmp_method_not(void *m1, void *m2) {
    return !(int)((UINTPTR_T)(*(jcov_method_t**)m1)->id -
                  (UINTPTR_T)(*(jcov_method_t**)m2)->id);
}

jcov_method_t *lookup_method(jcov_hash_t *table, jmethodID method_id) {
    jcov_method_t **ptr;
    jcov_method_t method_tmp;
    jcov_method_t *method_tmp_addr = &method_tmp;
    method_tmp.id = method_id;
    ptr = (jcov_method_t**)jcov_hash_lookup(table, &method_tmp_addr);
    return (ptr ? *ptr : NULL);
}

jcov_method_t *put_method(jcov_hash_t *table, jcov_method_t **m) {
    jcov_method_t **ptr;
    ptr = (jcov_method_t**)jcov_hash_put(table, m);
    return *ptr;
}

void remove_method(jcov_hash_t *table, jmethodID method_id) {
    jcov_method_t method_tmp;
    jcov_method_t *method_tmp_addr = &method_tmp;
    method_tmp.id = method_id;
    jcov_hash_remove(table, cmp_method_not, &method_tmp_addr);
}

int array_lookup_method(JVMPI_Method *key,
                        jcov_method_t **arr,
                        int arr_len,
                        int start_search_index) {
    int i;
    jcov_method_t *meth;

    for (i = start_search_index; i < arr_len; i++) {
        meth = arr[i];
        if (meth == NULL) continue;
        if (!(strcmp(key->method_name, meth->name)) &&
            !(strcmp(key->method_signature, meth->signature))) {
            return i;
        }
    }
    for (i = 0; i < start_search_index; i++) {
        meth = arr[i];
        if (meth == NULL) continue;
        if (!(strcmp(key->method_name, meth->name)) &&
            !(strcmp(key->method_signature, meth->signature))) {
            return i;
        }
    }
    return -1;
}

/* ====================================*/
/* === functions related to threads ===*/
/* ====================================*/

UINTPTR_T hash_thread(void *t) {
    return (UINTPTR_T)(*(jcov_thread_t**)t)->id;
}

SIZE_T size_thread(void *m) {
    return sizeof(jcov_thread_t*);
}

int cmp_thread(void *t1, void *t2) {
    return (int)((UINTPTR_T)(*(jcov_thread_t**)t1)->id -
                 (UINTPTR_T)(*(jcov_thread_t**)t2)->id);
}

int cmp_thread_not(void *t1, void *t2) {
    return !(int)((UINTPTR_T)(*(jcov_thread_t**)t1)->id -
                  (UINTPTR_T)(*(jcov_thread_t**)t2)->id);
}

jcov_thread_t *lookup_thread(jcov_hash_t *table, JNIEnv *thread_id) {
    jcov_thread_t **ptr;
    jcov_thread_t thread_tmp;
    jcov_thread_t *thread_tmp_addr = &thread_tmp;
    thread_tmp.id = thread_id;
    ptr = (jcov_thread_t**)jcov_hash_lookup(table, &thread_tmp_addr);
    return (ptr ? *ptr : NULL);
}

jcov_thread_t *put_thread(jcov_hash_t *table, jcov_thread_t **thread) {
    jcov_thread_t **ptr;
    ptr = (jcov_thread_t**)jcov_hash_put(table, thread);
    return *ptr;
}

void remove_thread(jcov_hash_t *table, JNIEnv *thread_id) {
    jcov_thread_t thread_tmp;
    jcov_thread_t *thread_tmp_addr = &thread_tmp;
    thread_tmp.id = thread_id;
    jcov_hash_remove(table, cmp_thread_not, &thread_tmp_addr);
}

/* ======================*/
/* === initialization ===*/
/* ======================*/

void jcov_htables_init(void) {
}
