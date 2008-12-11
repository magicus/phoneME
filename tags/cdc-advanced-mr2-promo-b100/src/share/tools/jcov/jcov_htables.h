/*
 * @(#)jcov_htables.h	1.12 06/10/10
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

#ifndef _JCOV_HTABLES_H
#define _JCOV_HTABLES_H

#include "jvmpi.h"
#include "jni.h"

#include "jcov.h"
#include "jcov_types.h"
#include "jcov_util.h"

void jcov_htables_init(void);
void hash_table_iterate(jcov_hash_t *table, void *(*f)(void *, void *));

UINTPTR_T     hash_class_key(void *c);
SIZE_T        size_class(void *m);
int           cmp_class_key(void *c1, void *c2);
jcov_class_t *lookup_class_by_key(jcov_hash_t *table, jcov_class_t *class);
jcov_class_t *lookup_class_by_key_short(jcov_hash_t *table, jcov_class_t *class);
jcov_class_t *put_class_by_key(jcov_hash_t *table, jcov_class_t **c);

UINTPTR_T            hash_hooked_class(void *c);
SIZE_T               size_hooked_class(void *c);
int                  cmp_hooked_classes(void *c1, void *c2);
jcov_hooked_class_t *lookup_hooked_class(jcov_hash_t *table, char *name);
jcov_hooked_class_t *put_hooked_class(jcov_hash_t *table, jcov_hooked_class_t **c);
void                 remove_hooked_class(jcov_hash_t *table, jcov_hooked_class_t *c);

UINTPTR_T     hash_class_id(void *c);
int           cmp_class_id(void *c1, void *c2);
jcov_class_t *lookup_class_by_id(jcov_hash_t *table, jobjectID class_id);
jcov_class_t *put_class_by_id(jcov_hash_t *table, jcov_class_t **c);
void          remove_class_by_id(jcov_hash_t *table, jobjectID class_id);
int           find_method_in_class(jcov_class_t *c, JVMPI_Method *m);

UINTPTR_T        hash_classID(void *m);
SIZE_T           size_classID(void *m);
int              cmp_classID(void *m1, void *m2);
jcov_class_id_t *lookup_classID(jcov_hash_t *table, jobjectID class_id);
jcov_class_id_t *put_classID(jcov_hash_t *table, jcov_class_id_t **m);
void             remove_classID(jcov_hash_t *table, jobjectID class_id);


UINTPTR_T      hash_method(void *m);
SIZE_T         size_method(void *m);
int            cmp_method(void *m1, void *m2);
jcov_method_t *lookup_method(jcov_hash_t *table, jmethodID method_id);
jcov_method_t *put_method(jcov_hash_t *table, jcov_method_t **m);
void remove_method(jcov_hash_t *table, jmethodID method_id);
int array_lookup_method(JVMPI_Method *key,
			jcov_method_t **arr,
			int arr_len,
			int start_index);

jcov_thread_t *lookup_thread(jcov_hash_t *table, JNIEnv *thread_id);
jcov_thread_t *put_thread(jcov_hash_t *table, jcov_thread_t **thread);
void remove_thread(jcov_hash_t *table, JNIEnv *thread_id);

#endif /* _JCOV_HTABLES_H */
