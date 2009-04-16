/*
 * @(#)jcov_events.c	1.29 06/10/10
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
#include "jcov_jvm.h"
#include "jcov_error.h"
#include "jcov_crt.h"
#include "jcov_util.h"
#include "jcov_htables.h"
#include "jcov_setup.h"
#include "jcov_file.h"
#include "jcov_java.h"

jcov_list_t *thread_list = NULL;

static long cnt_load_hooks = 0;
static long cnt_loads = 0;
static long cnt_req_loads = 0;
static long cnt_prof = 0;
static long cnt_skip = 0;

JVMPI_RawMonitor jcov_threads_lock;
JVMPI_RawMonitor jcov_cls_key_lock;
JVMPI_RawMonitor jcov_cls_id_lock;
JVMPI_RawMonitor jcov_cls_flt_lock;
JVMPI_RawMonitor jcov_methods_lock;
JVMPI_RawMonitor jcov_instr_count_lock;

static jcov_thread_t *jcov_thread_new(JNIEnv *env_id);
static jcov_hooked_class_t *jcov_hooked_class_new(void);
static jcov_class_id_t *jcov_class_id_new(jobjectID id);

#define METH_CACHE_SIZE 256 /* default size of cache for methods in a hooked class */

#define LOCK(monitor)   (jvmpi_interface->RawMonitorEnter)(jcov_##monitor##_lock)
#define UNLOCK(monitor) (jvmpi_interface->RawMonitorExit)(jcov_##monitor##_lock)

#define CALL_DEPTH 2

#define ASSRT(cond, n, ctx) if (!(cond)) { \
        if ((ctx).hooked_class != NULL && ((ctx).hooked_class)->name != NULL) \
            sprintf(info, "assertion failure #%d in class: %s", n, ((ctx).hooked_class)->name); \
        else \
            sprintf(info, "assertion failure #%d (class unknown)", n); \
        jcov_error(info); \
        goto cleanupAndFail; \
    }

#define READ_AND_CHECK(dest, bytes_total, ctx) \
    dest = read##bytes_total##bytes(&((ctx).class_data), &((ctx).class_len), &err_code); \
    if (err_code) { \
        if ((ctx).hooked_class != NULL && ((ctx).hooked_class)->name != NULL) \
            sprintf(info, "bad class format : %s", ((ctx).hooked_class)->name); \
        else \
            sprintf(info, "bad class format"); \
        jcov_error(info); \
        goto cleanupAndFail; \
    }

#define ERROR_VERBOSE(err_str) if (verbose_mode > 0) jcov_error(err_str)

#define SKIP(n, ctx) (ctx).class_data += n ; (ctx).class_len -= n ;

#define ASSRT_CP_ENTRY(cp_entry, x, n, ctx) if (!(cp_entry) || (cp_entry)->tag != x) { \
        if ((ctx).hooked_class != NULL && ((ctx).hooked_class)->name != NULL) \
            sprintf(info, "bad constant pool entry in : %s (assrt: %d)", ((ctx).hooked_class)->name, n); \
        else \
            sprintf(info, "bad constant pool entry (assrt: %d)", n); \
        jcov_error(info); \
        goto cleanupAndFail; \
    }

#define GET_CP_ENTRY(cp_entry, index, ctx) \
    if (index >= (ctx).cp_size) { \
        if ((ctx).hooked_class != NULL && ((ctx).hooked_class)->name != NULL) \
            sprintf(info, "invalid constant pool index in %s : %d", ((ctx).hooked_class)->name, index); \
        else \
            sprintf(info, "invalid constant pool index"); \
        jcov_error(info); \
        goto cleanupAndFail; \
    } \
    cp_entry = (ctx).cp[index]

static jcov_thread_t *find_thread(JNIEnv *env_id) {
    jcov_list_t *l;
    for (l = thread_list; l != NULL; l = l->next) {
        jcov_thread_t *thread = (jcov_thread_t*)l->elem;
        if (thread->id == env_id) {
            return thread;
        }
    }
    return NULL;
}

static void add_thread(jcov_thread_t *t) {
    add_to_list(&thread_list, t);
}

static jcov_thread_t *find_or_add_thread(JNIEnv *env_id) {
    jcov_thread_t *res;

    LOCK(threads);
    res = find_thread(env_id);
    if (res == NULL) {
        res = jcov_thread_new(env_id);
        add_thread(res);
    }
    UNLOCK(threads);
    return res;
}

static Bool jcov_parse_class_data(JNIEnv *env_id, bin_class_context_t *context) {
    int err_code = 0;
    INT32 x4;
    UINT16 x2;
    INT32 attr_len, skip;
    char info[MAX_PATH_LEN];
    int count, attr_count, where_line, non_abstract_met, i, j, k;
    Bool crt_met = 0, cov_met = 0, caller_filt_ok = 0, filt_ok = 0;
    cp_entry_t *cp_entry;
    jcov_method_t *meth;
    jcov_thread_t *this_thread;
    jcov_hooked_class_t *hooked_class;
    bin_class_context_t ctx = *context;

    this_thread = find_or_add_thread(env_id);
    ctx.cp = NULL;

    hooked_class = jcov_hooked_class_new();
    ctx.hooked_class = hooked_class;
    context->hooked_class = hooked_class;

    READ_AND_CHECK(x4, 4, ctx);
    if (x4 != 0xCAFEBABE) {
        ERROR_VERBOSE("wrong magic number");
        goto cleanupAndFail;
    }

    SKIP(4, ctx); /* class file version number */
    READ_AND_CHECK(ctx.cp_size, 2, ctx); /* number of items in constant pool */
    ctx.cp = (cp_entry_t**)jcov_calloc(sizeof(cp_entry_t*) * ctx.cp_size);
    for (i = 1; i < ctx.cp_size; i++) { /* read the whole cp */
        ctx.cp[i] = read_next_cp_entry(&(ctx.class_data), &(ctx.class_len), &err_code);
        if (err_code) {
            jcov_error("bad constant pool format");
            goto cleanupAndFail;
        }
        if (ctx.cp[i]->tag == JVM_CONSTANT_Long || ctx.cp[i]->tag == JVM_CONSTANT_Double) {
            i++;
        }
    }
    READ_AND_CHECK(x2, 2, ctx);
    hooked_class->access_flags = x2 & JVM_RECOGNIZED_CLASS_MODIFIERS & ~JVM_ACC_SUPER;
    if (hooked_class->access_flags & JVM_ACC_INTERFACE) {
        hooked_class->access_flags &= ~JVM_ACC_ABSTRACT;
    }
    READ_AND_CHECK(x2, 2, ctx);
    GET_CP_ENTRY(cp_entry, x2, ctx); /*this_class*/
    ASSRT_CP_ENTRY(cp_entry, JVM_CONSTANT_Class, 0, ctx);
    GET_CP_ENTRY(cp_entry, cp_entry->u.Class.name_index, ctx);
    ASSRT_CP_ENTRY(cp_entry, JVM_CONSTANT_Utf8, 1, ctx);

    caller_filt_ok = string_suits_filter(caller_filter, cp_entry->u.Utf8.bytes);
    filt_ok        = string_suits_filter(class_filter, cp_entry->u.Utf8.bytes);

    if ((caller_filter == NULL || !caller_filt_ok) && !filt_ok) {
        goto cleanupAndFail;
    }
    hooked_class->name = jcov_strdup(cp_entry->u.Utf8.bytes);
    if (!filt_ok) {
        hooked_class->data_type = JCOV_SKIP_CLASS;
    }

    SKIP(2, ctx); /* super_class */
    READ_AND_CHECK(x2, 2, ctx);
    skip = x2 * 2;
    SKIP(skip, ctx); /* skip interfaces */

    READ_AND_CHECK(count, 2, ctx); /* fields_count */
    for (i = 0; i < count; i++) {
        SKIP(6, ctx);
        /* attributes_count */
        READ_AND_CHECK(attr_count, 2, ctx);
        for (j = 0; j < attr_count; j++) {
            SKIP(2, ctx); /* attribute_name_index */
            READ_AND_CHECK(attr_len, 4, ctx);
            SKIP(attr_len, ctx);
        }
    }
    /*-- we're at methods section at last --*/
    READ_AND_CHECK(hooked_class->methods_total, 2, ctx); /* methods_count */
    hooked_class->method_cache = (jcov_method_t**)jcov_calloc(sizeof(jcov_method_t*) *
                                                              hooked_class->methods_total);
    non_abstract_met = 0;
    for (i = 0; i < hooked_class->methods_total; i++) {
        meth = (jcov_method_t*)jcov_calloc(sizeof(jcov_method_t));
        meth->covtable_size = 0;
        meth->covtable = NULL;
        meth->pc_cache = NULL;
        meth->pc_cache_size = 0;
        READ_AND_CHECK(meth->access_flags, 2, ctx);
        meth->access_flags &= JVM_RECOGNIZED_METHOD_MODIFIERS;
        non_abstract_met |= !(meth->access_flags & JVM_ACC_ABSTRACT);
        where_line = 0;
        READ_AND_CHECK(x2, 2, ctx);
        GET_CP_ENTRY(cp_entry, x2, ctx); /* method name */
        ASSRT_CP_ENTRY(cp_entry, JVM_CONSTANT_Utf8, 2, ctx);
        meth->name = jcov_strdup(cp_entry->u.Utf8.bytes);
        READ_AND_CHECK(x2, 2, ctx);
        GET_CP_ENTRY(cp_entry, x2, ctx); /* method signature */
        ASSRT_CP_ENTRY(cp_entry, JVM_CONSTANT_Utf8, 3, ctx);
        meth->signature = jcov_strdup(cp_entry->u.Utf8.bytes);
        /* read the method's attributes */
        READ_AND_CHECK(count, 2, ctx); /* attribute count */
        for (j = 0; j < count; j++) {
            READ_AND_CHECK(x2, 2, ctx);
            GET_CP_ENTRY(cp_entry, x2, ctx);
            ASSRT_CP_ENTRY(cp_entry, JVM_CONSTANT_Utf8, 4, ctx);
            READ_AND_CHECK(attr_len, 4, ctx);
            if (!filt_ok || strcmp(cp_entry->u.Utf8.bytes, CODE_ATTR_NAME)) {
                SKIP(attr_len, ctx);
                continue;
            }
            /* parse code attribute */
            SKIP(4, ctx); /* max_stack and max_locals */
            READ_AND_CHECK(skip, 4, ctx); /* code_count */
            meth->pc_cache_size = skip;
            ctx.code = ctx.class_data;
            SKIP(skip, ctx);
            READ_AND_CHECK(skip, 2, ctx);
            skip *= 8; /* handlers_count * handler_size */
            SKIP(skip, ctx);
            READ_AND_CHECK(attr_count, 2, ctx); /* attributes_count */
            for (k = 0; k < attr_count; k++) {
                READ_AND_CHECK(x2, 2, ctx);
                GET_CP_ENTRY(cp_entry, x2, ctx);
                ASSRT_CP_ENTRY(cp_entry, JVM_CONSTANT_Utf8, 5, ctx);
                READ_AND_CHECK(attr_len, 4, ctx);
                if (!strcmp(cp_entry->u.Utf8.bytes, LINE_NUMBER_TABLE_ATTR_NAME)) {
                    READ_AND_CHECK(x2, 2, ctx);
                    if (x2 < 1) { /* lines_count */
                        skip = attr_len - 2;
                        SKIP(skip, ctx);
                    } else {
                        SKIP(2, ctx); /* 1st start_pc */
                        READ_AND_CHECK(where_line, 2, ctx);
                        skip = attr_len - 6;
                        SKIP(skip, ctx);
                    }
                    continue;
                }
                if (jcov_data_type == JCOV_DATA_B &&
                    !strcmp(cp_entry->u.Utf8.bytes, COV_TABLE_ATTR_NAME)) {
                    read_cov_table(attr_len, meth, &ctx);
                    cov_met = 1;
                    ASSRT(!crt_met, 3, ctx);
                    continue;
                }
                if (jcov_data_type == JCOV_DATA_B &&
                    !strcmp(cp_entry->u.Utf8.bytes, CRT_TABLE_ATTR_NAME)) {
                    read_crt_table(attr_len, meth, &ctx);
                    crt_met = 1;
                    ASSRT(!cov_met, 4, ctx);
                    hooked_class->data_type = JCOV_DATA_C;
                    continue;
                }
                SKIP(attr_len, ctx);
            }
        }
        if (filt_ok && meth->covtable == NULL && jcov_data_type == JCOV_DATA_M) {
            cov_item_t *cov_item = cov_item_new(0, CT_METHOD, INSTR_ANY, where_line, 0);
            meth->covtable_size = 1;
            meth->covtable = (cov_item_t*)jcov_calloc(sizeof(cov_item_t) * meth->covtable_size);
            meth->covtable[0] = *cov_item;
        } else if (filt_ok && meth->covtable == NULL && jcov_data_type == JCOV_DATA_B) {
            /* covtable_size field used here to store the line num */
            meth->covtable_size = where_line;
        }
        hooked_class->method_cache[i] = meth;
    }
    if (!non_abstract_met) {  /* interface encountered - skip it */
        goto cleanupAndFail;
    }
    if (jcov_data_type == JCOV_DATA_B && !(crt_met || cov_met) && (caller_filter == NULL || !caller_filt_ok)) {
        goto cleanupAndFail;
    }

    if (filt_ok && jcov_data_type == JCOV_DATA_B && (crt_met || cov_met)) {
        for (i = 0; i < hooked_class->methods_total; i++) {
            unsigned char single_item = 0;
            cov_item_t    *cov_item;

            meth = hooked_class->method_cache[i];
            if (meth->covtable != NULL)
                continue;

            single_item = (meth->access_flags & JVM_ACC_NATIVE) ? 1 : 0;
            single_item = single_item || strcmp(meth->name, "<init>");

            where_line = meth->covtable_size; /* get remembered line number */
            meth->covtable_size = single_item ? 1 : 2;
            meth->covtable = (cov_item_t*)jcov_calloc(sizeof(cov_item_t) * meth->covtable_size);
            if (hooked_class->data_type == JCOV_DATA_C)
                where_line = where_line << CRT_ENTRY_PACK_BITS;
            cov_item = cov_item_new(0, CT_METHOD, INSTR_ANY, where_line, 0);
            meth->covtable[0] = *cov_item;
            jcov_free(cov_item);
            if (!single_item) {
                cov_item = cov_item_new(0, CT_BLOCK, INSTR_ANY, where_line, 0);
                meth->covtable[1] = *cov_item;
                jcov_free(cov_item);
                meth->pc_cache = (int*)jcov_calloc(sizeof(int) * meth->pc_cache_size);
                meth->pc_cache[0] = 2; /* 2 is the index of the last cov_item plus one */
            }
        }
    }

    /* read the class' attributes */
    READ_AND_CHECK(count, 2, ctx); /* class attrs total */
    for (i = 0; i < count; i++) {
        READ_AND_CHECK(x2, 2, ctx);
        GET_CP_ENTRY(cp_entry, x2, ctx); /* attr name index */
        ASSRT_CP_ENTRY(cp_entry, JVM_CONSTANT_Utf8, 6, ctx);
        READ_AND_CHECK(attr_len, 4, ctx); /* bytes_count */

        if (!filt_ok) {
            SKIP(attr_len, ctx);
            continue;
        }

        if (!strcmp(cp_entry->u.Utf8.bytes, ABS_SRC_PATH_ATTR_NAME)) {
            READ_AND_CHECK(x2, 2, ctx);
            GET_CP_ENTRY(cp_entry, x2, ctx);
            ASSRT_CP_ENTRY(cp_entry, JVM_CONSTANT_Utf8, 7, ctx);
            jcov_free(hooked_class->src_name);
            hooked_class->src_name = jcov_strdup(cp_entry->u.Utf8.bytes);
        } else if (!strcmp(cp_entry->u.Utf8.bytes, SRC_FILE_ATTR_NAME)) {
            READ_AND_CHECK(x2, 2, ctx);
            GET_CP_ENTRY(cp_entry, x2, ctx);
            ASSRT_CP_ENTRY(cp_entry, JVM_CONSTANT_Utf8, 8, ctx);
            if (hooked_class->src_name == NULL) {
                hooked_class->src_name = jcov_strdup(cp_entry->u.Utf8.bytes);
            }
        } else if (!strcmp(cp_entry->u.Utf8.bytes, TIMESTAMP_ATTR_NAME)) {
            union {
                struct {
                    UINT32 hi;
                    UINT32 lo;
                } parts;
                jlong timestamp;
            } tstamp;
            
            READ_AND_CHECK(tstamp.parts.hi, 4, ctx);
            READ_AND_CHECK(tstamp.parts.lo, 4, ctx);
            if (hooked_class->timestamp != NULL)
                continue;
            hooked_class->timestamp = (char*)jcov_calloc(24 * sizeof(char));
            sprintf(hooked_class->timestamp, "%lld", tstamp.timestamp);
        } else if (!strcmp(cp_entry->u.Utf8.bytes, COMPILATION_ID_ATTR_NAME)) {
            READ_AND_CHECK(x2, 2, ctx); /* compilation_id_index */
            GET_CP_ENTRY(cp_entry, x2, ctx);
            ASSRT_CP_ENTRY(cp_entry, JVM_CONSTANT_Utf8, 8, ctx);
            if (hooked_class->timestamp != NULL) {
                goto cleanupAndFail;
            }
            hooked_class->timestamp = jcov_strdup(cp_entry->u.Utf8.bytes);
        } else {
    	    SKIP(attr_len, ctx);
        }
    }
    if (hooked_class->timestamp == NULL) {
        hooked_class->timestamp = (char*)jcov_calloc(2 * sizeof(char));
        sprintf(hooked_class->timestamp, "%c", '0');
    }
    if (jcov_data_type == JCOV_DATA_B && !(crt_met || cov_met)) {
        for (i = 0; i < hooked_class->methods_total; i++) {
            hooked_class->method_cache[i]->covtable_size = 0;
            hooked_class->data_type = JCOV_SKIP_CLASS;
        }
    }

    if (hooked_class->src_name == NULL) {
        hooked_class->src_name = dummy_src_name(hooked_class->name);
    }
    /* constant pool may be freed now */
    jcov_free_constant_pool(ctx.cp, ctx.cp_size);
    put_hooked_class(this_thread->hooked_class_table, &hooked_class);
    *context = ctx;
    return TRUE;

cleanupAndFail:
    jcov_free_constant_pool(ctx.cp, ctx.cp_size);
    jcov_free_hooked_class(hooked_class);
    return FALSE;
}

void jcov_class_load_hook_event(JVMPI_Event *event) {
    bin_class_context_t ctx;
    Bool res;

    ctx.class_len = event->u.class_load_hook.class_data_len;
    ctx.class_data = event->u.class_load_hook.class_data;
    event->u.class_load_hook.new_class_data = 
        (unsigned char*)event->u.class_load_hook.malloc_f(ctx.class_len);
    memcpy(event->u.class_load_hook.new_class_data, ctx.class_data, ctx.class_len);
    event->u.class_load_hook.new_class_data_len = ctx.class_len;

    res = jcov_parse_class_data(event->env_id, &ctx);
    cnt_load_hooks++;
    if (verbose_mode > 1 && res && ctx.hooked_class->name != NULL) {
        char info[MAX_PATH_LEN];
        sprintf(info, "CLASS_LOAD_HOOK : %s", ctx.hooked_class->name);
        jcov_info(info);
    }
}
	
#undef SKIP
#undef ASSRT_CP_ENTRY

void jcov_class_load_event(JVMPI_Event *event) {
    char info[MAX_PATH_LEN];
    jcov_class_t *class;
    jcov_hooked_class_t *hooked_class;
    jcov_class_t *found_class;
    jcov_method_t *found_method;
    jcov_method_t **class_methods;
    JVMPI_Method *meth;
    jcov_thread_t *this_thread;
    int i, ind;
    INT32 mem;
    char *name, *tmp;
    int last_matched = 0;
    JNIEnv *env_id = event->env_id;

    LOCK(threads);
    this_thread = find_thread(env_id);
    UNLOCK(threads);
    if (this_thread == NULL) {
        return;
    }

    if (!(event->event_type & JVMPI_REQUESTED_EVENT)) {
        cnt_loads++;
    }
    name = jcov_strdup(event->u.class_load.class_name);
    for (; (tmp = (char*)strchr(name, '.')); *tmp = '/');

    hooked_class = lookup_hooked_class(this_thread->hooked_class_table, name);
    if (hooked_class == NULL) {
        cnt_skip++;
        if (verbose_mode > 1) {
            sprintf(info, "class will not be profiled : %s", name);
            jcov_info(info);
        }
        if (load_early_classes && jcov_java_init_done) {
            LOCK(cls_flt);
            if (!lookup_classID(class_filt_table, event->u.class_load.class_id)) {
                jcov_class_id_t *cid = jcov_class_id_new(event->u.class_load.class_id);
                put_classID(class_filt_table, &cid);
            }
            UNLOCK(cls_flt);
        }
        return;
    }
    
    class = (jcov_class_t*)jcov_calloc(sizeof(jcov_class_t));
    class->name = name;
    if (verbose_mode > 1) {
        char *req = (event->event_type & JVMPI_REQUESTED_EVENT) ? "(requested) " : "";
        sprintf(info, "%sCLASS_LOAD : %s", req, name);
        jcov_info(info);
    }
    
    class->id = event->u.class_load.class_id;
    class->num_methods = event->u.class_load.num_methods;
    class->unloaded = 0;
    mem = class->num_methods * sizeof(JVMPI_Method);
    class->methods = (JVMPI_Method*)jcov_calloc(mem);
    memcpy(class->methods, event->u.class_load.methods, mem);
    class->timestamp = jcov_strdup(hooked_class->timestamp);
    class->src_name = jcov_strdup(hooked_class->src_name);
    class->access_flags = hooked_class->access_flags;
    class->data_type = hooked_class->data_type;

    for (i = 0; i < event->u.class_load.num_methods; i++) {
        class->methods[i].method_name = jcov_strdup(class->methods[i].method_name);
        class->methods[i].method_signature = jcov_strdup(class->methods[i].method_signature);
    }

    LOCK(cls_key);
    found_class = lookup_class_by_key(class_key_table, class);
    UNLOCK(cls_key);

    cnt_prof++;
    if (found_class) {
        if (!(found_class->unloaded)) {
            if (verbose_mode > 0) {
                sprintf(info, "class is loaded twice : %s", found_class->name);
                jcov_error(info);
            }
            return;
        } 

        CHK(found_class->num_methods != class->num_methods,
            "jcov_class_load_event: method number mistmatch");
        LOCK(cls_id);
        LOCK(methods);
        remove_class_by_id(class_id_table, found_class->id);
        found_class->id = class->id;
        for (i = 0; i < class->num_methods; i++) {
            meth = &(class->methods[i]);
            ind = find_method_in_class(found_class, meth);
            CHK(ind == -1, "jcov_class_load_event: method def not found");
            found_method = lookup_method(method_table, (found_class->methods[ind]).method_id);
            CHK(!found_method, "jcov_class_load_event: method not found");
            remove_method(method_table, found_method->id);
            found_method->id = meth->method_id;
            (found_class->methods[ind]).method_id = meth->method_id;
            put_method(method_table, &found_method);
        }
        put_class_by_id(class_id_table, &found_class);
        UNLOCK(methods);
        UNLOCK(cls_id);
        found_class->unloaded = 0;
        jcov_free(class->methods);
        jcov_free(class->name);
        jcov_free(class->src_name);
        jcov_free(class);
        return;
    }
    LOCK(cls_id);
    LOCK(cls_key);
    put_class_by_id(class_id_table, &class);
    put_class_by_key(class_key_table, &class);
    UNLOCK(cls_key);
    UNLOCK(cls_id);

    class_methods = hooked_class->method_cache;
    LOCK(methods);
    for (i = 0; i < class->num_methods; i++) {
        ind = array_lookup_method(&(class->methods[i]), class_methods, 
                                  hooked_class->methods_total, last_matched);
        /* The method could be a miranda method which was not in the initial
           classfile.  If so, just ignore it: */
        if (ind != -1) {
            last_matched = ind;
            class_methods[ind]->id = class->methods[i].method_id;
            class_methods[ind]->class = class;
            if (lookup_method(method_table, class_methods[ind]->id) != NULL)
                continue;
            put_method(method_table, &(class_methods[ind]));
            /* Ownership of this method info is now transferred from the
               hooked_class to the global method hash table.  Remove it
               from the hooked_class's method cache: */
            class_methods[ind] = NULL;
        }
    }
    UNLOCK(methods);
    remove_hooked_class(this_thread->hooked_class_table, hooked_class);
    jcov_free_hooked_class(hooked_class);
}


#define SKIP_CLASS if (verbose_mode > 1) { \
        sprintf(info, "class will not be profiled : %s", class.name); \
            jcov_info(info); \
        } \
        cnt_skip++

void jcov_req_class_load_event(JVMPI_Event *event) {
    jcov_class_t *found_class, class;
    bin_class_context_t ctx = { 0, 0, 0, 0, 0, 0, 0 };
    UINT8 *class_buf = NULL;
    char *tmp;
    Bool res;
    char info[MAX_PATH_LEN];

    cnt_req_loads++;
    class.name = jcov_strdup(event->u.class_load.class_name);
    for (; (tmp = (char*)strchr(class.name, '.')); *tmp = '/');
    LOCK(cls_key);
    found_class = lookup_class_by_key_short(class_key_table, &class);
    UNLOCK(cls_key);
    if (found_class) {
        jcov_free(class.name);
        SKIP_CLASS;
        return;
    }

    find_or_add_thread(event->env_id);

    if (!get_class_binary_data(event->env_id, class.name, &class_buf, &ctx.class_len)) {
        jcov_free(class.name);
        jcov_free(class_buf);
        SKIP_CLASS;
        return;
    }

    ctx.class_data = class_buf;
    res = jcov_parse_class_data(event->env_id, &ctx);
    jcov_free(class_buf);
    if (res) {
        jcov_class_load_event(event);
    } else {
        SKIP_CLASS;
    }
    jcov_free(class.name);
}

void jcov_class_unload_event(JVMPI_Event *event) {
    jcov_class_t *found_class;
    char info[MAX_PATH_LEN];

    LOCK(cls_id);
    found_class = lookup_class_by_id(class_id_table, event->u.class_unload.class_id);
    UNLOCK(cls_id);
    if (!found_class)
        return;
    found_class->unloaded = 1;
    if (verbose_mode > 1) {
        sprintf(info, "CLASS_UNLOAD : %s", found_class->name);
        jcov_info(info);
    }
}

void jcov_object_move_event(JVMPI_Event *event) {
    jcov_class_t *found_class = NULL;
    jcov_class_id_t *found_id = NULL;
    char info[MAX_PATH_LEN];

    found_class = lookup_class_by_id(class_id_table, event->u.obj_move.obj_id);
    if (found_class) {
        /* since we are in thread suspended mode here, we can't manipulate
         * hashtables at this point because hashtable functions make
         * blocking system calls (e.g. malloc). Rehashing the moved class
         * happens later, in GC_FINISH event handler.
         */
        found_class->id_sav = event->u.obj_move.new_obj_id;
        if (verbose_mode > 1) {
            sprintf(info, "OBJECT_MOVE : class moved on heap: %s", found_class->name);
            jcov_info(info);
        }
    }
    if (!load_early_classes)
        return;
    found_id = lookup_classID(class_filt_table, event->u.obj_move.obj_id);
    if (found_id) {
        found_id->id_sav = event->u.obj_move.new_obj_id;
    }
}

void jcov_gc_start_event(void) {
    if (verbose_mode > 0) {
        jcov_info("GC_START");
    }
    LOCK(threads);
    LOCK(cls_id);
    LOCK(cls_key);
    LOCK(methods);
    if (load_early_classes) {
        LOCK(cls_flt);
    }
}

static int store_moved_class(void *class_, void *arg) {
    jcov_class_t *class = *(jcov_class_t**)class_;
    jcov_list_t **list = (jcov_list_t**)arg;
    if (class->id_sav != NULL) {
        add_to_list(list, class);
	return 1;
    }
    return 0;
}

static int store_moved_id(void *id_, void *arg) {
    jcov_class_id_t *id = *(jcov_class_id_t**)id_;
    jcov_list_t **list = (jcov_list_t**)arg;
    if (id->id_sav != NULL) {
        add_to_list(list, id);
	return 1;
    }
    return 0;
}

void jcov_gc_finish_event(void) {
    jcov_list_t *moved_classes_list = NULL;
    jcov_list_t *moved_id_list = NULL;
    jcov_list_t *l;

    /* iterate and remove */
    jcov_hash_remove(class_id_table, store_moved_class, (void*)(&moved_classes_list));
    /* we can't remove a class by its "old" id and immediately add it by its "new" id
     * in one cycle - since the set of "old" ids and the set of "new" ids may overlap.
     */
    for (l = moved_classes_list; l != NULL; l = l->next) {
        jcov_class_t *class = (jcov_class_t*)l->elem;
        class->id = class->id_sav;
        class->id_sav = NULL;
        put_class_by_id(class_id_table, &class);
    }
    free_list(&moved_classes_list, NULL);

    if (load_early_classes) {
	/* iterate and remove */
        jcov_hash_remove(class_filt_table, store_moved_id, (void*)(&moved_id_list));
        for (l = moved_id_list; l != NULL; l = l->next) {
            jcov_class_id_t *id = (jcov_class_id_t*)l->elem;
	    id->id = id->id_sav;
	    id->id_sav = NULL;
            put_classID(class_filt_table, &id);
        }
        free_list(&moved_id_list, NULL);
        UNLOCK(cls_flt);
    }

    if (verbose_mode > 0) {
        jcov_info("GC_FINISH");
    }
    UNLOCK(methods);
    UNLOCK(cls_key);
    UNLOCK(cls_id);
    UNLOCK(threads);
}

static jcov_hooked_class_t *jcov_hooked_class_new(void) {
    jcov_hooked_class_t *res;
    res = (jcov_hooked_class_t*)jcov_calloc(sizeof(jcov_hooked_class_t));
    res->data_type = jcov_data_type;
    return res;
}

static jcov_thread_t *jcov_thread_new(JNIEnv *env_id) {
    jcov_thread_t *thread = (jcov_thread_t*)jcov_calloc(sizeof(jcov_thread_t));
    thread->id = env_id;
    thread->hooked_class_table = jcov_hash_new(37, hash_hooked_class, size_hooked_class, cmp_hooked_classes);
    return thread;
}

static jcov_class_id_t *jcov_class_id_new(jobjectID id) {
    jcov_class_id_t *res;
    res = (jcov_class_id_t*)jcov_calloc(sizeof(jcov_class_id_t));
    res->id = id;
    return res;
}

void jcov_thread_start_event(JVMPI_Event *event) {
    char info[MAX_PATH_LEN];
    JNIEnv *env_id = event->u.thread_start.thread_env_id;

    if (verbose_mode > 0) {
        char *name = event->u.thread_start.thread_name;
        name = name == NULL ? "NO_NAME" : name;
        sprintf(info, "THREAD_START : %s [%p]", name, env_id);
        jcov_info(info);
    }
    CALL(SetThreadLocalStorage)(env_id, jcov_calloc(sizeof(void*)));
    find_or_add_thread(env_id);
}

void jcov_thread_end_event(JVMPI_Event *event) {
    jcov_thread_t *thread;
    char info[MAX_PATH_LEN];
    
    if (verbose_mode > 0) {
        sprintf(info, "THREAD_END : %p", event->env_id);
        jcov_info(info);
    }
    LOCK(threads);
    thread = find_thread(event->env_id);
    UNLOCK(threads);
    if (thread != NULL) {
        jcov_conserve_thread(thread);
    } else {
        sprintf(info, "jcov_thread_end_event: thread not found: %p", event->env_id);
        jcov_warn(info);
    }
}

void jcov_method_entry_event(JVMPI_Event *event) {
    jcov_method_t *m, *m1;
    char info[MAX_PATH_LEN];
    JVMPI_CallTrace call_trace;
    JVMPI_CallFrame call_frames[CALL_DEPTH];
    jmethodID method_id = event->u.method.method_id;
    void *mem = NULL;
    int i;

    if (load_early_classes) {
        mem = CALL(GetThreadLocalStorage)(event->env_id);
        if (mem != NULL && *((char*)mem) == LOADING_EARLY_CLASS) {
            return;
        }
        if (mem == NULL) {
            mem = jcov_calloc(sizeof(void*));
            CALL(SetThreadLocalStorage)(event->env_id, mem);
        }
    }

    LOCK(methods);
    m = lookup_method(method_table, method_id);
    UNLOCK(methods);

    if (m == NULL && load_early_classes && jcov_java_init_done) {
        int res;
        jobjectID class_id;
        jcov_class_id_t *id;
        char state_sav;

        /* 
         * According to JVMPI specs, the profiler must disable GC before
         * calling GetMethodClass. 
         */
        CALL(DisableGC)();
        class_id = CALL(GetMethodClass)(method_id);
        CALL(EnableGC)();
        LOCK(cls_flt);
        id = lookup_classID(class_filt_table, class_id);
        UNLOCK(cls_flt);
        if (id) {
            return;
        } else {
            id = jcov_class_id_new(class_id);
            LOCK(cls_flt);
            put_classID(class_filt_table, &id);
            UNLOCK(cls_flt);
        }

        state_sav = *((char*)mem);
        *((char*)mem) = LOADING_EARLY_CLASS;
        res = CALL(RequestEvent)(JVMPI_EVENT_CLASS_LOAD, class_id);
        *((char*)mem) = state_sav;

        if (res != JVMPI_SUCCESS && verbose_mode > 1) {
            sprintf(info, "could not request CLASS_LOAD for 0x%x\n",
                    (unsigned int)class_id);
            jcov_warn(info);
            return;
        }
        LOCK(methods);
        m = lookup_method(method_table, method_id);
        UNLOCK(methods);
    }

    if (m == NULL || m->class == NULL || m->class->data_type == JCOV_SKIP_CLASS) {
        return;
    }
    if (caller_filter != NULL) {
        mem = CALL(GetThreadLocalStorage)(event->env_id);
        if (mem == NULL) {
            mem = jcov_calloc(sizeof(void*));
            CALL(SetThreadLocalStorage)(event->env_id, mem);
        }
        *((char*)mem) = CALLER_MISMATCH;

        call_trace.frames = call_frames;
        call_trace.env_id = event->env_id;
        CALL(GetCallTrace)(&call_trace, CALL_DEPTH);
        if (method_id != call_frames[0].method_id)
            jcov_error_stop("event's method_id and GetCallTrace's method_id don't match");
        LOCK(methods);
        m1 = lookup_method(method_table, call_frames[1].method_id);
        UNLOCK(methods);
        if (m1 != NULL && string_suits_filter(caller_filter, m1->class->name)) {
            *((char*)mem) = CALLER_MATCH;
        } else {
            return;
        }
    }    
    if (verbose_mode > 2) {
        sprintf(info, "METHOD_ENTRY : %s%s", m->name, m->signature);
        jcov_info(info);
    }
    i = m->covtable_size;
    if (i == 1 || (jcov_data_type == JCOV_DATA_M && i > 0)) {
        m->covtable[0].count++;
    }
}

void jcov_jvm_shut_down_event() {
    save_jcov_data(jcov_file);
    if (verbose_mode > 0) {
        printf("********** Jcov execution stats **********\n");
        printf("***       Total memory allocated : %ld\n", memory_allocated);
        printf("*** CLASS_LOAD_HOOK_EVENTs total : %ld\n", cnt_load_hooks);
        printf("***      CLASS_LOAD_EVENTs total : %ld\n", cnt_loads);
        if (load_early_classes) {
            printf("*** (requested) CLASS_LOAD_EVENTs total : %ld\n", cnt_req_loads);
        }
        printf("***       Profiled classes total : %ld\n", cnt_prof);
        printf("***        Skipped classes total : %ld\n", cnt_skip);
        printf("******************************************\n");
    }
}


void jcov_events_init() {
    jcov_threads_lock = CALL(RawMonitorCreate)("jcov_threads_lock");
    jcov_cls_key_lock = CALL(RawMonitorCreate)("jcov_cls_key_lock");
    jcov_cls_id_lock  = CALL(RawMonitorCreate)("jcov_cls_id_lock");
    jcov_cls_flt_lock = CALL(RawMonitorCreate)("jcov_cls_flt_lock");
    jcov_methods_lock = CALL(RawMonitorCreate)("jcov_methods_lock");
    jcov_instr_count_lock = CALL(RawMonitorCreate)("jcov_instr_count_lock");
}
