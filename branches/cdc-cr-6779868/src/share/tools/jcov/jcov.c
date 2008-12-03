/*
 * @(#)jcov.c	1.21 06/10/10
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

#include "jni.h"
#include "jvmpi.h"
#include "jcov.h"
#include "jcov_htables.h"
#include "jcov_error.h"
#include "jcov_setup.h"
#include "jcov_events.h"
#include "jcov_types.h"
#include "jcov_java.h"

JVMPI_Interface *jvmpi_interface;
static JavaVM *jvm;

jcov_hash_t *class_key_table;
jcov_hash_t *class_id_table;
jcov_hash_t *method_table;
jcov_hash_t *class_filt_table;
long memory_allocated;

#ifndef JVMPI_VERSION_1_1
#define JVMPI_VERSION_1_1 ((jint)0x10000002)
#endif

static char* get_event_name(int event_no) {
    char* s = "UNKNOWN_EVENT";
    switch (event_no) {
#define def_event_name(e,v) case e: v=#e ; break
        def_event_name(JVMPI_EVENT_METHOD_ENTRY, s);
        def_event_name(JVMPI_EVENT_METHOD_ENTRY2, s);
        def_event_name(JVMPI_EVENT_METHOD_EXIT, s);
        def_event_name(JVMPI_EVENT_OBJECT_ALLOC, s);
        def_event_name(JVMPI_EVENT_OBJECT_FREE, s);
        def_event_name(JVMPI_EVENT_OBJECT_MOVE, s);
        def_event_name(JVMPI_EVENT_COMPILED_METHOD_LOAD, s);
        def_event_name(JVMPI_EVENT_COMPILED_METHOD_UNLOAD, s);
        def_event_name(JVMPI_EVENT_THREAD_START, s);
        def_event_name(JVMPI_EVENT_THREAD_END, s);
        def_event_name(JVMPI_EVENT_INSTRUCTION_START, s);
        def_event_name(JVMPI_EVENT_CLASS_LOAD_HOOK, s);
        def_event_name(JVMPI_EVENT_HEAP_DUMP, s);
        def_event_name(JVMPI_EVENT_JNI_GLOBALREF_ALLOC, s);
        def_event_name(JVMPI_EVENT_JNI_GLOBALREF_FREE, s);
        def_event_name(JVMPI_EVENT_JNI_WEAK_GLOBALREF_ALLOC, s);
        def_event_name(JVMPI_EVENT_JNI_WEAK_GLOBALREF_FREE, s);
        def_event_name(JVMPI_EVENT_CLASS_LOAD, s);
        def_event_name(JVMPI_EVENT_CLASS_UNLOAD, s);
        def_event_name(JVMPI_EVENT_DATA_DUMP_REQUEST, s);
        def_event_name(JVMPI_EVENT_DATA_RESET_REQUEST, s);
        def_event_name(JVMPI_EVENT_JVM_INIT_DONE, s);
        def_event_name(JVMPI_EVENT_JVM_SHUT_DOWN, s);
        def_event_name(JVMPI_EVENT_ARENA_NEW, s);
        def_event_name(JVMPI_EVENT_ARENA_DELETE, s);
        def_event_name(JVMPI_EVENT_OBJECT_DUMP, s);
        def_event_name(JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER, s);
        def_event_name(JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED, s);
        def_event_name(JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT, s);
        def_event_name(JVMPI_EVENT_MONITOR_CONTENDED_ENTER, s);
        def_event_name(JVMPI_EVENT_MONITOR_CONTENDED_ENTERED, s);
        def_event_name(JVMPI_EVENT_MONITOR_CONTENDED_EXIT, s);
        def_event_name(JVMPI_EVENT_MONITOR_WAIT, s);
        def_event_name(JVMPI_EVENT_MONITOR_WAITED, s);
        def_event_name(JVMPI_EVENT_MONITOR_DUMP, s);
        def_event_name(JVMPI_EVENT_GC_START, s);
        def_event_name(JVMPI_EVENT_GC_FINISH, s);
#undef def_event_name
    }
    return s;
}

static void init_jni_bridge(JNIEnv *env_id) {
    Bool res;
    CALL(DisableEvent)(JVMPI_EVENT_METHOD_ENTRY, NULL);
    CALL(DisableEvent)(JVMPI_EVENT_CLASS_LOAD, NULL);
    res = jcov_java_init(env_id);
    CALL(EnableEvent)(JVMPI_EVENT_METHOD_ENTRY, NULL);
    CALL(EnableEvent)(JVMPI_EVENT_CLASS_LOAD, NULL);
    if (!res) {
        jcov_warn("Unable to workaround the early classes JVMPI bug");
        load_early_classes = FALSE;
    } else {
        if (verbose_mode > 0) {
            jcov_info("Jcov-JNI bridge initialized successfully");
        }
    }
    return;
}

static void jcov_notify_event(JVMPI_Event *event) {
    char info[256];
    cov_item_t *cov_table, *ct_entry;
    int hash;
    int diff;
    UINT16 instr_pc;
    jcov_method_t *meth;

    switch (event->event_type) {
    case JVMPI_EVENT_METHOD_ENTRY: 
        /* find the method in the hash table, inc exec counter */
        jcov_method_entry_event(event);
	return;
    case JVMPI_EVENT_CLASS_LOAD:
        /* if the class being loaded has previously been unloaded
         * then use data structures allocated on the first loading
         * of this class (having updated the class' and its methods' IDs;
         * otherwise - create new ones;
         * class' compare key : (source name, class name, timestamp)
         */
        jcov_class_load_event(event);
        return;
    case JVMPI_EVENT_CLASS_LOAD | JVMPI_REQUESTED_EVENT:
        /* This event is requested only if there was no normal JVMPI_EVENT_CLASS_LOAD
         * generated for a class. Load the class' classfile via JNI, parse it,
         * as it's done in JVMPI_EVENT_CLASS_LOAD_HOOK handler, then do what
         * normal JVMPI_EVENT_CLASS_LOAD handler does.
         */
        jcov_req_class_load_event(event);
        return;
    case JVMPI_EVENT_CLASS_LOAD_HOOK:
        /* parse the class' bytecode, create jcov_method_t structures
         * for each method and put them into temporary array for later
         * use by JVMPI_EVENT_CLASS_LOAD event handler; also extract
         * compare key for this class (source name, class name, timestamp)
         */
        jcov_class_load_hook_event(event);
        return;
    case JVMPI_EVENT_CLASS_UNLOAD:
        /* lookup the class in the class table and mark it as unloaded */
        jcov_class_unload_event(event);
        return;
    case JVMPI_EVENT_OBJECT_MOVE:
        /* update ID of moved class and rehash it */
        jcov_object_move_event(event);
        return;
    case JVMPI_EVENT_INSTRUCTION_START:
        /* Increment counter of corresponding jcov_item_t.
         * For the sake of efficiency, this code is not
         * a separate function
         */
        LOCK_INSTR();
        meth = lookup_method(method_table, event->u.instruction.method_id);
        if (!meth || meth->covtable_size < 2) {
            UNLOCK_INSTR();
            return;
        }
        
        /* If the caller filter is present - check if current method's
         * caller belongs to a class satisfying the filter by checking
         * current thread-spesific global flag. The flag is modified inside
         * method entry event handler. Or if we use JNI to load "early" class
         * files via calls to Java methods, check if we are not currently
         * loading the class file in this thread.
         */
        if (caller_filter != NULL || load_early_classes) {
            void* mem = CALL(GetThreadLocalStorage)(event->env_id);
            if (mem == NULL || *((char*)mem) != CALLER_MATCH) {
                UNLOCK_INSTR();
                return;
            }
        }

        instr_pc = (UINT16)(event->u.instruction.offset);
        cov_table = meth->covtable;
        hash = meth->pc_cache[instr_pc];
        if (!hash) {
            UNLOCK_INSTR();
            return;
        }
        ct_entry = cov_table + hash - 1;
        switch (ct_entry->instr_type) {
        case INSTR_ANY:
            while (ct_entry >= cov_table && ct_entry->pc == instr_pc) {
                int type = ct_entry->type;
                if (type == CT_BLOCK) {
                    ct_entry->count++;
                    ct_entry--;
                } else if (type == CT_BRANCH_TRUE || type == CT_BRANCH_FALSE) {
                    (ct_entry - 1)->count++;
                    ct_entry -= 2;
                } else {
                    ct_entry->count++;
                    ct_entry--;
                }
            }
            UNLOCK_INSTR();
            return;
        case INSTR_IF:
            while (ct_entry >= cov_table && ct_entry->pc == instr_pc) {
                if (ct_entry->type == CT_BLOCK) {
                    ct_entry->count++;
                    ct_entry--;
                } else {
                    event->u.instruction.u.if_info.is_true ? 
                        (ct_entry - 1)->count++ : ct_entry->count++;
                    ct_entry -= 2;
                }
            }
            UNLOCK_INSTR();
            return;
        case INSTR_LOOKUPSW:
#define BASE event->u.instruction.u.lookupswitch_info
            (ct_entry - BASE.pairs_total + BASE.chosen_pair_index)->count++;
#undef BASE
            UNLOCK_INSTR();
            return;
        case INSTR_TABLESW:
#define BASE event->u.instruction.u.tableswitch_info
            diff = BASE.key - BASE.hi;
            if (diff > 0 || BASE.key - BASE.low < 0) {
                ct_entry->count++;
            } else {
                (ct_entry + diff - 1)->where_line ? 
                    (ct_entry + diff - 1)->count++ : ct_entry->count++;
            }
#undef BASE
            UNLOCK_INSTR();
            return;
        default:
            UNLOCK_INSTR();
            jcov_internal_error("jcov_notify_event: unknown instruction type");
        }
        return;
    case JVMPI_EVENT_GC_START:
        /* lock access to hash tables */
        jcov_gc_start_event();
        return;
    case JVMPI_EVENT_GC_FINISH:
        /* unlock access to hash tables */
        jcov_gc_finish_event();
        return;
    case JVMPI_EVENT_THREAD_START:
        /* allocate new jcov_thread_t object, initialize it and put
         * it into the 'thread_table' hash table
         */
        jcov_thread_start_event(event);
        return;
    case JVMPI_EVENT_THREAD_END:
        /* remove corresponding jcov_thread_t object from thread_table
         * and free it
         */
        jcov_thread_end_event(event);
        return;
    case JVMPI_EVENT_DATA_DUMP_REQUEST:
    case JVMPI_EVENT_JVM_SHUT_DOWN:
        /* save jcov data */
        jcov_jvm_shut_down_event();
        return;
    case JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER:
    case JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED:
    case JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT:
        sprintf(info,
                "%s %s [0x%X], thread 0x%X",
                get_event_name(event->event_type),
                event->u.raw_monitor.name,
                (unsigned int)event->u.raw_monitor.id,
                (unsigned int)event->env_id);
        jcov_info(info);
        return;
    case JVMPI_EVENT_MONITOR_CONTENDED_ENTER:
    case JVMPI_EVENT_MONITOR_CONTENDED_ENTERED:
    case JVMPI_EVENT_MONITOR_CONTENDED_EXIT:
        sprintf(info, "%s [0x%X], thread: 0x%X",
                get_event_name(event->event_type),
                (unsigned int)event->u.monitor.object,
                (unsigned int)event->env_id);
        jcov_info(info);
        return;
    case JVMPI_EVENT_MONITOR_WAIT:
    case JVMPI_EVENT_MONITOR_WAITED:
        sprintf(info, "%s [0x%X], thread: 0x%X",
                get_event_name(event->event_type),
                (unsigned int)event->u.monitor_wait.object,
                (unsigned int)event->env_id);
        jcov_info(info);
        return;
    default:
        return;
    }
}

/* jcov initialization */
JNIEXPORT jint JNICALL JVM_OnLoad(JavaVM *vm, char *options, void *reserved) {
    char err_message[256];
    int res;
    jint err_code;
    JNIEnv *env;

    jvm = vm;
    res = (*jvm)->GetEnv(jvm, (void**)(void*)&jvmpi_interface,
                         JVMPI_VERSION_1_1);
    if (res < 0) {
      res = (*jvm)->GetEnv(jvm, (void**)(void*)&jvmpi_interface,
                             JVMPI_VERSION_1);
        if (res < 0) {
            return JNI_ERR;
        }
    }
    jvmpi_interface->NotifyEvent = jcov_notify_event;
    
#define ENABLE_JVMPI_EVENT(event_type) \
    err_code = jvmpi_interface->EnableEvent(event_type, NULL); \
    if (err_code != JVMPI_SUCCESS) { \
        if (event_type != JVMPI_EVENT_INSTRUCTION_START) { \
            char *s = "cannot enable JVMPI event : %s"; \
            sprintf(err_message, s, get_event_name(event_type)); \
        } else { \
            char *s = "cannot enable %s event. Try using %s JVM option"; \
            sprintf(err_message, s, get_event_name(event_type), JVM_ENABLE_INSTR_START); \
        } \
        jcov_error(err_message); \
        return JNI_ERR; \
    }

    jcov_init(options);

    if (verbose_mode > 0)
        jcov_info("Initializing Jcov...\n");

    ENABLE_JVMPI_EVENT(JVMPI_EVENT_CLASS_LOAD);
    ENABLE_JVMPI_EVENT(JVMPI_EVENT_CLASS_LOAD_HOOK);
    ENABLE_JVMPI_EVENT(JVMPI_EVENT_CLASS_UNLOAD);
    ENABLE_JVMPI_EVENT(JVMPI_EVENT_OBJECT_MOVE);
    ENABLE_JVMPI_EVENT(JVMPI_EVENT_GC_START);
    ENABLE_JVMPI_EVENT(JVMPI_EVENT_GC_FINISH);
    ENABLE_JVMPI_EVENT(JVMPI_EVENT_THREAD_START);
    ENABLE_JVMPI_EVENT(JVMPI_EVENT_THREAD_END);
    ENABLE_JVMPI_EVENT(JVMPI_EVENT_METHOD_ENTRY);
    ENABLE_JVMPI_EVENT(JVMPI_EVENT_DATA_DUMP_REQUEST);
    ENABLE_JVMPI_EVENT(JVMPI_EVENT_JVM_SHUT_DOWN);

    if (verbose_mode > 3) {
        ENABLE_JVMPI_EVENT(JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER);
        ENABLE_JVMPI_EVENT(JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED);
        ENABLE_JVMPI_EVENT(JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT);
        ENABLE_JVMPI_EVENT(JVMPI_EVENT_MONITOR_CONTENDED_ENTER);
        ENABLE_JVMPI_EVENT(JVMPI_EVENT_MONITOR_CONTENDED_ENTERED);
        ENABLE_JVMPI_EVENT(JVMPI_EVENT_MONITOR_CONTENDED_EXIT);
        ENABLE_JVMPI_EVENT(JVMPI_EVENT_MONITOR_WAIT);
        ENABLE_JVMPI_EVENT(JVMPI_EVENT_MONITOR_WAITED);
    }

    if (jcov_data_type == JCOV_DATA_B) {
        ENABLE_JVMPI_EVENT(JVMPI_EVENT_INSTRUCTION_START);
    }

    if (verbose_mode > 0) {
        sprintf(err_message, "jcov data type is set to \'%c\'", jcov_data_type);
        jcov_info(err_message);
    }

    if (verbose_mode > 0)
        jcov_info("Jcov initialized successfully\n");

    if (load_early_classes) {
      if ((*vm)->GetEnv(vm, (void**)(void*)&env, JNI_VERSION_1_2)) {
            jcov_warn("Unable to workaround the early classes JVMPI bug");
            load_early_classes = FALSE;
        } else {
            init_jni_bridge(env);
        }
    }
    return JNI_OK;
#undef ENABLE_JVMPI_EVENT
}
