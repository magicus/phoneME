/*
 * @(#)jcov_util.h	1.17 06/10/10
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

#ifndef _JCOV_UTIL_H
#define _JCOV_UTIL_H

#include "javavm/include/porting/ansi/stdlib.h"

#include "jvmpi.h"

#include "jcov.h"
#include "jcov_types.h"
#include "jcov_jvm.h"

#define MAX_FILTER_SIZE 256
#define jcov_free(p) if (p != NULL) free(p)

typedef struct {
    UINT8 tag;
    union {
        struct {
            char *bytes;
        } Utf8;
        struct {
            UINT16 name_index;
        } Class;
        struct {
            UINT16 class_index;
            UINT16 name_and_type_index;
        } Methodref;
        struct {
            UINT16 class_index;
            UINT16 name_and_type_index;
        } InterfaceMethodref;
        struct {
            UINT16 name_index;
            UINT16 type_index;
        } NameAndType;
#ifdef DEBUG
        struct {
            INT32 val;
        } Integer;
        struct {
            UINT32 val;
        } Float;
        struct {
            UINT32 lo;
            UINT32 hi;
        } Long;
        struct {
            UINT32 lo;
            UINT32 hi;
        } Double;
        struct {
            UINT16 cp_index;
        } String;
        struct {
            UINT16 class_index;
            UINT16 name_and_type_index;
        } Field;
#endif /* DEBUG */
    } u;
} cp_entry_t;

typedef struct {
    jobjectID id;
    jobjectID id_sav;
} jcov_class_id_t;

typedef struct {
    jcov_hooked_class_t *hooked_class;
    UINT8               *class_data;
    jint                 class_len;
    cp_entry_t         **cp;
    SSIZE_T              cp_size;
    UINT8               *code;
    SSIZE_T              code_len;
} bin_class_context_t;

typedef struct {
    char **incl;
    int  incl_size;
    char **excl;
    int  excl_size;
} jcov_filter_t;

extern int opc_lengths[];

void       *jcov_calloc(SIZE_T size);
void        jcov_dummy_free(void *);
UINT8       read1bytes(UINT8 **buf, jint *len, int *err_code);
UINT16      read2bytes(UINT8 **buf, jint *len, int *err_code);
INT32       read4bytes(UINT8 **buf, jint *len, int *err_code);
char       *readUTF8(UINT8 **buf, jint *len, int utflen, int *err_code);
cp_entry_t *read_next_cp_entry(UINT8 **buf, jint *len, int *err_code);

#ifdef DEBUG
void print_cp_entry(cp_entry_t *cp_entry);
void print_cp(cp_entry_t **cp_entry, int size);
void debug_print_filter(jcov_filter_t *filter);
void debug_print_cov_table(cov_item_t *table, int size);
void debug_print_crt_entry(crt_entry_t *e);
#endif /* DEBUG */

void jcov_free_constant_pool(cp_entry_t **cp, int cp_count);
void jcov_free_hooked_class(jcov_hooked_class_t *c);
void jcov_free_thread(jcov_thread_t *thread);

void jcov_conserve_thread(jcov_thread_t *thread);

int get_instr_size(UINT16 pc, UINT8 *code);

char *jcov_strdup(const char *str);
void decode_modifiers(char *buf, int access_flags);
char *dummy_src_name(char *class_name);

jcov_filter_t *jcov_filter_new();
void filter_add_incl(jcov_filter_t *filter, char *str);
void filter_add_excl(jcov_filter_t *filter, char *str);
void filter_finalize(jcov_filter_t *filter);
int string_suits_filter(jcov_filter_t *filter, char *str);

void qsort_strings(char **array, int array_size);
char *search_prefix(const char *key, const char **prefix_array, int array_size);

void jcov_close(FILE **f);
void jcov_remove(const char *filename);

void add_to_list(jcov_list_t **l, void *elem);
void add_to_list_end(jcov_list_t **l, void *elem);
void free_list(jcov_list_t **l, void (*free_elem_f)(void *));
int list_size(jcov_list_t *l);

void jcov_util_init(void);

#endif /* _JCOV_UTIL_H */
