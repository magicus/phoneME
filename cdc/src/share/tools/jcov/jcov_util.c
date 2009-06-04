/*
 * @(#)jcov_util.c	1.20 06/10/10
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
#include "jcov_jvm.h"
#include "jcov_error.h"
#include "jcov_htables.h"
#include "jcov_hash.h"
#include "jcov_util.h"
#include "jcov_setup.h"

const char *DUMMY_SRC_PREF;
const char *DUMMY_SRC_SUFF;

int opc_lengths[] = {
	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	1,	1,	1,	1,	2,	3,	2,	3,
	3,	2,	2,	2,	2,	2,	1,	1,	1,	1,
	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	1,	1,	2,	2,	2,	2,	2,	1,
	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	3,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
	1,	1,	1,	3,	3,	3,	3,	3,	3,	3,
	3,	3,	3,	3,	3,	3,	3,	3,	3,	2,
	99,	99,	1,	1,	1,	1,	1,	1,	3,	3,
	3,	3,	3,	3,	3,	5,	1,	3,	2,	3,
	1,	1,	3,	3,	1,	1,	1,	4,	3,	3,
	5,	5,	1
};

void *jcov_calloc(SIZE_T size) {
    void *p = (void *)calloc(1, size);
    if (p == NULL) {
        jcov_error("***out of memory***");
        CALL(ProfilerExit)((jint)1);
    }
    memory_allocated += size;
    return p;
}

void jcov_dummy_free(void *p) {
    printf("free: %p\n", p);
}

UINT8 read1bytes(UINT8 **buf, jint *len, int *err_code) {
    *len = *len - 1;
    if ((*err_code = (*len < 0))) return 0;
    (*buf)++;
    return (*buf-1)[0];
}

UINT16 read2bytes(UINT8 **buf, jint *len, int *err_code) {
    UINT16 res;
    UINT8 *buf1 = *buf;
    *len = *len - 2;
    if ((*err_code = (*len < 0)))
      return 0;
    res = (UINT16)((buf1[0] << 8) | buf1[1]);
    *buf += 2;
    return res;
}

INT32 read4bytes(UINT8 **buf, jint *len, int *err_code) {
    INT32 res;
    UINT8 *buf1 = *buf;
    *len = *len - 4;
    if ((*err_code = (*len < 0))) return 0;
    res = (buf1[0] << 24) | (buf1[1] << 16) | (buf1[2] << 8) | buf1[3];
    *buf += 4;
    return res;
}

#define CHK_UTF(cond) if (cond) { \
    if (verbose_mode > 0) jcov_error("bad UTF string"); \
    *err_code = 1; \
    return NULL; \
}

char *readUTF8(UINT8 **buf, jint *len, int utflen, int *err_code) {
    UINT8 *str = (UINT8*)jcov_calloc(utflen + 1);
    int count = 0;
    int stringlen = 0;

    while (count < utflen) {
        UINT8 char2, char3;
        UINT8 c = read1bytes(buf, len, err_code);
        if (*err_code) return NULL;
        switch (c >> 4) {
        default:
            /* 0xxx xxxx */
            count++;
            str[stringlen++] = c;
            break;
        case 0xC: case 0xD:
            /* 110x xxxx   10xx xxxx */
            count += 2;
            CHK_UTF(count > utflen);
            char2 = read1bytes(buf, len, err_code);
            CHK_UTF((char2 & 0xC0) != 0x80);
            str[stringlen++] = '?';
            break;
        case 0xE:
            /* 1110 xxxx  10xx xxxx  10xx xxxx */
            count += 3;
            CHK_UTF(count > utflen);
            char2 = read1bytes(buf, len, err_code);
            char3 = read1bytes(buf, len, err_code);
            CHK_UTF(((char2 & 0xC0) != 0x80) || ((char3 & 0xC0) != 0x80));
            str[stringlen++] = '?';
            break;
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xF:
            /* 10xx xxxx,  1111 xxxx */
            CHK_UTF(1);
        }
        if (*err_code) return NULL;
    }
    str[stringlen] = '\0';
    return (char*)str;
}

#ifdef DEBUG

#define def_name(e,v) case e: v=#e ; break
static char *get_cp_entry_type_name(int tag) {
    char* s = "UNKNOWN_TYPE";
    switch (tag) {
        def_name(JVM_CONSTANT_Utf8, s);
        def_name(JVM_CONSTANT_Class, s);
        def_name(JVM_CONSTANT_Methodref, s);
        def_name(JVM_CONSTANT_InterfaceMethodref, s);
        def_name(JVM_CONSTANT_NameAndType, s);
        def_name(JVM_CONSTANT_Integer, s);
        def_name(JVM_CONSTANT_Float, s);
        def_name(JVM_CONSTANT_Long, s);
        def_name(JVM_CONSTANT_Double, s);
        def_name(JVM_CONSTANT_String, s);
        def_name(JVM_CONSTANT_Fieldref, s);
    }
    return s;
}

void print_cp_entry(cp_entry_t *cp_entry) {
    if (cp_entry == NULL) {
        printf("NULL\n");
        return;
    }
    printf("%s ", get_cp_entry_type_name(cp_entry->tag));
    switch (cp_entry->tag) {
    case JVM_CONSTANT_Utf8:
        printf("\"%s\"\n", cp_entry->u.Utf8.bytes);
        break;
    case JVM_CONSTANT_Class:
        printf("#%d\n", cp_entry->u.Class.name_index);
        break;
    case JVM_CONSTANT_Methodref:
        printf("class: #%d, name_and_type: #%d\n",
               cp_entry->u.Methodref.class_index,
               cp_entry->u.Methodref.name_and_type_index);
        break;
    case JVM_CONSTANT_InterfaceMethodref:
        printf("class: #%d, name_and_type: #%d\n",
               cp_entry->u.InterfaceMethodref.class_index,
               cp_entry->u.InterfaceMethodref.name_and_type_index);
        break;
    case JVM_CONSTANT_NameAndType:
        printf("name: #%d, type: #%d\n",
               cp_entry->u.NameAndType.name_index,
               cp_entry->u.NameAndType.type_index);
        break;
    case JVM_CONSTANT_Integer:
    case JVM_CONSTANT_Float:
            /* we're not interested in the rest of tags */
            printf("0x%x\n", (unsigned)cp_entry->u.Integer.val);
            break;
    case JVM_CONSTANT_Long:
    case JVM_CONSTANT_Double:
        printf("hi: 0x%x, lo: 0x%x\n",
               (unsigned)cp_entry->u.Long.hi, (unsigned)cp_entry->u.Long.lo);
        break;
    case JVM_CONSTANT_String:
        printf("#%d\n", cp_entry->u.String.cp_index);
        break;
    case JVM_CONSTANT_Fieldref:
        printf("class: #%d, name_and_type: #%d\n",
               cp_entry->u.Field.class_index,
               cp_entry->u.Field.name_and_type_index);
        break;
    }
}

void print_cp(cp_entry_t **cp, int size) {
    int i;
    for (i = 0; i < size; i++) {
        printf("#%d ", i+1);
        print_cp_entry(cp[i]);
    }
}

void debug_print_filter(jcov_filter_t *filter) {
    int i;

    printf("FILTER\n");
    if (filter == NULL)
        return;
    printf("INCL:\n");
    for (i = 0; i < filter->incl_size; i++)
        printf("\t%s\n", filter->incl[i]);
    printf("EXCL:\n");
    for (i = 0; i < filter->excl_size; i++)
        printf("\t%s\n", filter->excl[i]);
}

void debug_print_cov_table(cov_item_t *table, int size) {
    int i;
    char *types[] = {
        "METHOD",
        "FICT_METHOD",
        "BLOCK",
        "FICT_RET",
        "CASE",
        "SWITCH_WO_DEF",
        "BRANCH_TRUE",
        "BRANCH_FALSE"
    };

    for (i = 0; i < size; i++) {
        cov_item_t it = table[i];
        printf("type: %s\tpc: %d, line: %ld pos: %ld\n",
               types[it.type - 1], it.pc, it.where_line, it.where_pos);
    }
}

void debug_print_crt_entry(crt_entry_t *e) {
    char tmp[128];
    char *s;
    int i;
    int CRT_TYPE_ARR_LEN = 9;
    int CRT_TYPE_ARR[] = {
        CRT_STATEMENT,
        CRT_BLOCK,
        CRT_ASSIGNMENT,
        CRT_FLOW_CONTROLLER,
        CRT_FLOW_TARGET,
        CRT_INVOKE,
        CRT_CREATE,
        CRT_BRANCH_TRUE,
        CRT_BRANCH_FALSE
    };
    char *CRT_TYPE_STR_ARR[] = {
        "STMT",
        "BLOCK",
        "ASSGN",
        "F_CTRL",
        "F_TARG",
        "INVOKE",
        "CREATE",
        "B_TRUE",
        "B_FALSE"
    };

    if (e == NULL) {
        printf("NULL");
        return;
    }
    for (i = 0, s = tmp; i < CRT_TYPE_ARR_LEN; i++) {
        if (e->flags & CRT_TYPE_ARR[i] & 0xFFFF) {
            sprintf(s, "%s ", CRT_TYPE_STR_ARR[i]);
            s += strlen(CRT_TYPE_STR_ARR[i]) + 1;
        }
    }
    printf("ps_s=%d pc_e=%d rng_s=%ld rng_e=%ld [%s]",
           e->pc_start,  e->pc_end, e->rng_start, e->rng_end, tmp);
}

#endif /* DEBUG */

void jcov_free_cp_entry(cp_entry_t *cp_entry);
cp_entry_t *read_next_cp_entry(UINT8 **buf, jint *len, int *err_code) {
    int utflen;
    cp_entry_t *cp_buf = (cp_entry_t *)jcov_calloc(sizeof(cp_entry_t));
    cp_buf->tag = read1bytes(buf, len, err_code);
    if (!*err_code) {
        switch (cp_buf->tag) {
        case JVM_CONSTANT_Utf8:
            utflen = read2bytes(buf, len, err_code);
            if (*err_code) break;
            cp_buf->u.Utf8.bytes = readUTF8(buf, len, utflen, err_code);
            break;
        case JVM_CONSTANT_Class:
            cp_buf->u.Class.name_index = read2bytes(buf, len, err_code);
            break;
        case JVM_CONSTANT_Methodref:
            cp_buf->u.Methodref.class_index = read2bytes(buf, len, err_code);
            if (*err_code) break;
            cp_buf->u.Methodref.name_and_type_index = read2bytes(buf, len, err_code);
            break;
        case JVM_CONSTANT_InterfaceMethodref:
            cp_buf->u.InterfaceMethodref.class_index = read2bytes(buf, len, err_code);
            if (*err_code) break;
            cp_buf->u.InterfaceMethodref.name_and_type_index = read2bytes(buf, len, err_code);
            break;
        case JVM_CONSTANT_NameAndType:
            cp_buf->u.NameAndType.name_index = read2bytes(buf, len, err_code);
            if (*err_code) break;
            cp_buf->u.NameAndType.type_index = read2bytes(buf, len, err_code);
            break;
        case JVM_CONSTANT_Integer:
        case JVM_CONSTANT_Float:
            /* we're not interested in the rest of tags */
#ifndef DEBUG
            read4bytes(buf, len, err_code);
#else
            cp_buf->u.Integer.val = read4bytes(buf, len, err_code);
#endif
            break;
        case JVM_CONSTANT_Long:
        case JVM_CONSTANT_Double:
#ifndef DEBUG
            read4bytes(buf, len, err_code);
            if (*err_code) break;
            read4bytes(buf, len, err_code);
#else
            cp_buf->u.Long.hi = read4bytes(buf, len, err_code);
            if (*err_code) break;
            cp_buf->u.Long.lo = read4bytes(buf, len, err_code);
#endif
            break;
        case JVM_CONSTANT_String:
#ifndef DEBUG
            read2bytes(buf, len, err_code);
#else
            cp_buf->u.String.cp_index = read2bytes(buf, len, err_code);
#endif
            break;
        case JVM_CONSTANT_Fieldref:
#ifndef DEBUG
            read4bytes(buf, len, err_code);
#else
            cp_buf->u.Field.class_index = read2bytes(buf, len, err_code);
            if (*err_code) break;
            cp_buf->u.Field.name_and_type_index = read2bytes(buf, len, err_code);
#endif
            break;
        default:
            jcov_error("unrecognized constant pool entry encountered");
            *err_code = 1;
        }
    }
    if (*err_code) {
        jcov_free_cp_entry(cp_buf);
        cp_buf = NULL;
    }
    return cp_buf;
}

void jcov_free_cp_entry(cp_entry_t *cp_entry) {
    if (cp_entry != NULL) {
	if (cp_entry->tag == JVM_CONSTANT_Utf8) {
	    jcov_free(cp_entry->u.Utf8.bytes);
	}
        jcov_free(cp_entry);
    }
}

void jcov_free_constant_pool(cp_entry_t **cp, int cp_count) {
    int i;
    if (cp == NULL) return;
    for (i = 1; i < cp_count; i++) {
        jcov_free_cp_entry(cp[i]);
    }
    jcov_free(cp);
}

void jcov_free_method(jcov_method_t *meth) {
    if (meth == NULL) return;
    jcov_free(meth->name);
    jcov_free(meth->signature);
    jcov_free(meth->covtable);
    jcov_free(meth->pc_cache);
    jcov_free(meth);
}

void jcov_free_hooked_class(jcov_hooked_class_t *c) {
    int i;
    if (c == NULL)
        return;
    jcov_free(c->src_name);
    jcov_free(c->name);
    if (c->method_cache != NULL) {
        for (i = 0; i < c->methods_total; i++) {
            jcov_free_method(c->method_cache[i]);
        }
        jcov_free(c->method_cache);
    }
    jcov_free(c);
}


static void *f(void *c, void *arg) {
    jcov_free_hooked_class((jcov_hooked_class_t*)c);
    return NULL;
}

void jcov_conserve_thread(jcov_thread_t *thread) {
    void *dummy = NULL;
    jcov_hash_iterate(thread->hooked_class_table, f, dummy);
}

char *jcov_strdup(const char *str) {
    char *res = jcov_calloc(strlen(str)+1);
    strcpy(res, str);
    return res;
}

int get_instr_size(UINT16 pc, UINT8 *code) {
    UINT8 opcode = code[pc] & 0xFF;
    UINT16 cur_pc, instr_size;
    switch (opcode) {
    case opc_wide:
        return ((code[pc + 1] & 0xFF) == opc_iinc ? 6 : 4);
    case opc_lookupswitch:
        cur_pc = (pc + 4) & (~3);
        instr_size = cur_pc - pc /* padding bytes */ + 8 /* default offset and npairs */;
        cur_pc += 4;
        return INT32_AT(code, cur_pc) * 8 + instr_size;
    case opc_tableswitch:
        cur_pc = (pc + 4) & (~3);
        instr_size = cur_pc - pc /* padding bytes */ + 12 /* default offset, low and high */;
        cur_pc += 4;
        return (INT32_AT(code, cur_pc + 4) - INT32_AT(code, cur_pc) + 1) * 4 + instr_size;
    case opc_xxxunusedxxx:
        return 1;
    default:
        return opc_lengths[opcode];
    }
}

static int bit_flags[12];
static int lengths[12];
static char *string_flags[12];

void decode_modifiers(char *buf, int access_flags) {
    char *ptr = buf;
    int i = 0;
    for (i = 0; i < 12; i++) {
	if (access_flags & bit_flags[i]) {
	    memcpy(ptr, string_flags[i], lengths[i]);
	    ptr += lengths[i];
	    *ptr = ' ';
	    ptr++;
	}
    }
    if (ptr == buf) {
	ptr++;
    }
    *(ptr-1) = '\0';
}

char *dummy_src_name(char *class_name) {
    char *res;
    int len = strlen(class_name) + strlen(DUMMY_SRC_PREF) + strlen(DUMMY_SRC_SUFF) + 1;
    res = (char*)jcov_calloc(len);
    sprintf(res, "%s%s%s", DUMMY_SRC_PREF, class_name, DUMMY_SRC_SUFF);
    res[len - 1] = '\0';
    return res;
}

static int my_strcmp(const void *s1, const void *s2) {
	return strcmp(*(char**)s1, *(char**)s2);
}

static int starts_with(const void *str1, const void *str2) {
    return strncmp(*(char**)str1, *(char**)str2, strlen(*(char**)str2));
}

jcov_filter_t *jcov_filter_new() {
    jcov_filter_t *res = (jcov_filter_t*)jcov_calloc(sizeof(jcov_filter_t));
    res->incl = (char**)jcov_calloc(sizeof(char*) * MAX_FILTER_SIZE);
    res->excl = (char**)jcov_calloc(sizeof(char*) * MAX_FILTER_SIZE);
    return res;
}

void filter_add_incl(jcov_filter_t *filter, char *str) {
    if (filter->incl_size >= MAX_FILTER_SIZE)
        jcov_error_stop("too many filters");
    filter->incl[filter->incl_size++] = jcov_strdup(str);
}

void filter_add_excl(jcov_filter_t *filter, char *str) {
    if (filter->excl_size >= MAX_FILTER_SIZE)
        jcov_error_stop("too many filters");
    filter->excl[filter->excl_size++] = jcov_strdup(str);
}

void filter_finalize(jcov_filter_t *filter) {
    if (filter == NULL)
        return;
    qsort(filter->incl, filter->incl_size, sizeof(char*), my_strcmp);
    qsort(filter->excl, filter->excl_size, sizeof(char*), my_strcmp);
}

int string_suits_filter(jcov_filter_t *filter, char *str) {
    if (filter == NULL)
        return 1;
    if (bsearch(&str, filter->excl, filter->excl_size, sizeof(char*), starts_with) != NULL)
        return 0;
    if (filter->incl_size == 0)
        return 1;
    if (bsearch(&str, filter->incl, filter->incl_size, sizeof(char*), starts_with) != NULL)
        return 1;
    return 0;
}

void jcov_close(FILE **f) {
    if (*f == NULL)
        return;
    if (fclose(*f) != 0) {
        jcov_error("cannot close file");
    }
    *f = NULL;
}

void jcov_remove(const char *filename) {
    char buf[MAX_PATH_LEN];

    if (remove(filename) == -1) {
        sprintf(buf,"cannot remove file : %s\n", filename);
        jcov_error(buf);
    }
}

void free_list(jcov_list_t **l, void (*free_elem_f)(void *)) {
    jcov_list_t *list;
    if (l == NULL || *l == NULL)
        return;
    list = *l;
    while (list != NULL) {
        jcov_list_t *next = list->next;
        void *tmp = list->elem;
        list->elem = NULL;
        if (free_elem_f != NULL) {
            free_elem_f(tmp);
        }
        jcov_free(list);
        list = next;
    }
    *l = NULL;
}

void add_to_list(jcov_list_t **l, void *elem) {
    jcov_list_t *new_entry = (jcov_list_t*)jcov_calloc(sizeof(jcov_list_t));
    new_entry->elem = elem;
    new_entry->next = *l;
    *l = new_entry;
}

void add_to_list_end(jcov_list_t **l, void *elem) {
    jcov_list_t *cur = *l;
    jcov_list_t *new_entry = (jcov_list_t*)jcov_calloc(sizeof(jcov_list_t));
    new_entry->elem = elem;
    new_entry->next = NULL;
    if (cur == NULL) {
        *l = new_entry;
        return;
    }
    for (; cur->next != NULL; cur = cur->next);
    cur->next = new_entry;
}

int list_size(jcov_list_t *l) {
    int res = 0;
    for (; l != NULL; l = l->next, res++);
    return res;
}

void jcov_util_init(void) {
    int i;
    bit_flags[0] = JVM_ACC_PUBLIC;
    bit_flags[1] = JVM_ACC_PRIVATE;
    bit_flags[2] = JVM_ACC_PROTECTED;
    bit_flags[3] = JVM_ACC_STATIC;
    bit_flags[4] = JVM_ACC_FINAL;
    bit_flags[5] = JVM_ACC_SYNCHRONIZED;
    bit_flags[6] = JVM_ACC_VOLATILE;
    bit_flags[7] = JVM_ACC_TRANSIENT;
    bit_flags[8] = JVM_ACC_NATIVE;
    bit_flags[9] = JVM_ACC_INTERFACE;
    bit_flags[10] = JVM_ACC_ABSTRACT;
    bit_flags[11] = JVM_ACC_STRICT;
	
    string_flags[0] = "public";
    string_flags[1] = "private";
    string_flags[2] = "protected";
    string_flags[3] = "static";
    string_flags[4] = "final";
    string_flags[5] = "synchronized";
    string_flags[6] = "volatile";
    string_flags[7] = "transient";
    string_flags[8] = "native";
    string_flags[9] = "interface";
    string_flags[10] = "abstract";
    string_flags[11] = "strictfp";

    for (i = 0; i < 12; i++){
	lengths[i] = strlen(string_flags[i]);
    }

    DUMMY_SRC_PREF = "<UNKNOWN_SOURCE/";
    DUMMY_SRC_SUFF = ".java>";
}
