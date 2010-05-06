/*
 * @(#)jcov_crt.c	1.8 06/10/10
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

#include "jcov.h"
#include "jcov_util.h"
#include "jcov_types.h"
#include "jcov_error.h"
#include "jcov_crt.h"
#include "jcov_jvm.h"

#define ASSRT(cond, n, ctx) if (!(cond)) { \
        if ((ctx).hooked_class != NULL && ((ctx).hooked_class)->name != NULL) \
            sprintf(info, "assertion failure #%d in class: %s", n, ((ctx).hooked_class)->name); \
        else \
            sprintf(info, "assertion failure #%d (class unknown)", n); \
        jcov_error(info); \
        exit(1); \
    }

#define READ_AND_CHECK(dest, bytes_total, ctx) \
    dest = read##bytes_total##bytes(&((ctx).class_data), &((ctx).class_len), &err_code); \
    if (err_code) { \
        if ((ctx).hooked_class != NULL && ((ctx).hooked_class)->name != NULL) \
            sprintf(info, "bad class format : %s", ((ctx).hooked_class)->name); \
        else \
            sprintf(info, "bad class format"); \
        jcov_error(info); \
        exit(1); \
    }

cov_item_t *cov_item_new(UINT16 pc, UINT8 type, UINT8 instr_type, INT32 line, INT32 pos) {
    cov_item_t *cov_item = (cov_item_t*)jcov_calloc(sizeof(cov_item_t));

    cov_item->pc         = pc;
    cov_item->type       = type;
    cov_item->instr_type = instr_type;
    cov_item->where_line = line;
    cov_item->where_pos  = pos;
    cov_item->count      = 0;

    return cov_item;
}

static crt_entry_t *crt_entry_new(UINT16 pc_start, UINT16 pc_end,
                                  INT32 rng_start, INT32 rng_end, UINT16 flags) {
    crt_entry_t *crt_entry = (crt_entry_t*)jcov_calloc(sizeof(crt_entry_t));
    crt_entry->pc_start  = pc_start;
    crt_entry->pc_end    = pc_end;
    crt_entry->rng_start = rng_start;
    crt_entry->rng_end   = rng_end;
    crt_entry->flags     = flags;

    return crt_entry;
}

/* Note: CRT_ANY is used to represent the logical OR'ed value of all possible
         CRT flags to be passed to the flags argument in get_crt_entry()
         below. */
#define CRT_ANY       ((UINT16)-1)

static crt_entry_t *get_crt_entry(INT32 pc, jcov_list_t *map[], UINT16 flags) {
    jcov_list_t *l = map[pc];
    for (; l != NULL; l = l->next) {
        crt_entry_t *e = (crt_entry_t*)l->elem;
        if (e->flags & flags)
            return e;
    }
    return NULL;
}

static cov_item_t *get_cov_item(INT32 pc, jcov_list_t *map[], UINT8 type) {
    jcov_list_t *l = map[pc];
    for (; l != NULL; l = l->next) {
        cov_item_t *ci = (cov_item_t*)l->elem;
        if (ci->type == type)
            return ci;
    }
    return NULL;
}

static void gen_cov_for_if(UINT16              pc,
                           crt_entry_t         *ce,
                           jcov_list_t         *cov_map[],
                           jcov_list_t         *crt_map[],
                           bin_class_context_t *context) {

    cov_item_t *cov_item;
    UINT8 opc;
    UINT16 targ_pc;
    UINT8  *code = context->code;
    UINT8  type = ce->flags & CRT_BRANCH_TRUE ? CT_BRANCH_FALSE : CT_BRANCH_TRUE;
    UINT8  itype;
    Bool is_if;

    opc = code[pc] & 0xFF;
    is_if = !(opc < opc_ifeq                            ||
              (opc > opc_if_acmpne && opc < opc_ifnull) ||
              opc > opc_ifnonnull);

    itype = is_if ? INSTR_IF : INSTR_ANY;
    cov_item = cov_item_new(pc, type, itype, ce->rng_start, ce->rng_end);
    add_to_list(&(cov_map[pc]), cov_item);
    type = type == CT_BRANCH_TRUE ? CT_BRANCH_FALSE : CT_BRANCH_TRUE;
    cov_item = cov_item_new(pc, type, itype, ce->rng_start, ce->rng_end);
    add_to_list(&(cov_map[pc]), cov_item);

    if (!is_if)
        return;

    targ_pc = pc + INT16_AT(code, pc + 1);
    if (get_cov_item(targ_pc, cov_map, CT_BLOCK) == NULL) {
        ce = get_crt_entry(targ_pc, crt_map, CRT_STATEMENT);
        if (ce != NULL) {
            cov_item = cov_item_new(targ_pc, CT_BLOCK, INSTR_ANY, ce->rng_start, ce->rng_end);
            add_to_list(&(cov_map[targ_pc]), cov_item);
        }
    }
    targ_pc = pc + get_instr_size(pc, code);
    if (get_cov_item(targ_pc, cov_map, CT_BLOCK) == NULL) {
        ce = get_crt_entry(targ_pc, crt_map, CRT_STATEMENT);
        if (ce != NULL) {
            cov_item = cov_item_new(targ_pc, CT_BLOCK, INSTR_ANY, ce->rng_start, ce->rng_end);
            add_to_list(&(cov_map[targ_pc]), cov_item);
        }
    }
}

static void gen_cov_for_switch(UINT8               *code,
                               crt_entry_t         *crt_entry,
                               jcov_list_t         *cov_list[],
                               jcov_list_t         *map[],
                               bin_class_context_t *context) {
    INT32 n, off, def_off;
    UINT8 type;
    UINT16 cur_pc, i;
    char info[MAX_PATH_LEN];
    crt_entry_t *ce = NULL;
    cov_item_t *def_item;
    UINT16 pc = crt_entry->pc_start; /* get remembered pc */

    if (cov_list[pc] != NULL) {
        char *class = (context->hooked_class == NULL || context->hooked_class->name == NULL) ?
            "UNKNOWN" : context->hooked_class->name;
        sprintf(info, "invalid crt at switch instruction pc. Class: %s", class);
        jcov_error(info);
    }

    switch (code[pc]) {
    case opc_lookupswitch:
        cur_pc = (pc + 4) & (~3);
        off = INT32_AT(code, cur_pc) + pc;
        def_off = off;
        ce = get_crt_entry(off, map, CRT_FLOW_TARGET);
        if (ce == NULL)
            ce = get_crt_entry(off, map, CRT_ANY);
        if (ce == NULL)
            ce = crt_entry;
        ASSRT(ce != NULL, 20, *context);
        type = (ce->flags & CRT_FLOW_TARGET) ? CT_CASE : CT_SWITCH_WO_DEF;
        def_item = cov_item_new(pc, type, INSTR_LOOKUPSW, ce->rng_start, ce->rng_end);
        if (type == CT_SWITCH_WO_DEF && get_cov_item(off, cov_list, CT_BLOCK) == NULL) {
            /* workaround a compiler bug */
            cov_item_t *ci = cov_item_new((UINT16)off, CT_BLOCK, INSTR_ANY, ce->rng_start, ce->rng_end);
            add_to_list(&(cov_list[off]), ci);
        }
        cur_pc += 4;
        n = INT32_AT(code, cur_pc);
        cur_pc += 4;
        for (i = 0; i < n; i++) {
            cur_pc += 4;
            off = INT32_AT(code, cur_pc) + pc;
            ce = get_crt_entry(off, map, CRT_FLOW_TARGET);
            if (ce == NULL)
                ce = get_crt_entry(off, map, CRT_ANY);
            ASSRT(ce != NULL || off == def_off, 21, *context);
            if (off != def_off) {
                cov_item_t *ci = cov_item_new(pc, CT_CASE, INSTR_ANY, ce->rng_start, ce->rng_end);
                add_to_list_end(&(cov_list[pc]), ci);
            }
            cur_pc += 4;
        }
        add_to_list_end(&(cov_list[pc]), def_item);
        break;
    case opc_tableswitch:
        cur_pc = (pc + 4) & (~3);
        off = INT32_AT(code, cur_pc) + pc;
        def_off = off;
        ce = get_crt_entry(off, map, CRT_FLOW_TARGET);
        if (ce == NULL)
            ce = get_crt_entry(off, map, CRT_ANY);
        if (ce == NULL)
            ce = crt_entry;
        ASSRT(ce != NULL, 22, *context);
        type = (ce->flags & CRT_FLOW_TARGET) ? CT_CASE : CT_SWITCH_WO_DEF;
        def_item = cov_item_new(pc, type, INSTR_TABLESW, ce->rng_start, ce->rng_end);
        if (type == CT_SWITCH_WO_DEF && get_cov_item(off, cov_list, CT_BLOCK) == NULL) {
            /* workaround a compiler bug */
            cov_item_t *ci = cov_item_new((UINT16)off, CT_BLOCK, INSTR_ANY, ce->rng_start, ce->rng_end);
            add_to_list(&(cov_list[off]), ci);
        }
        cur_pc += 4;
        n = INT32_AT(code, cur_pc + 4) - INT32_AT(code, cur_pc) + 1;
        cur_pc += 8;
        for (i = 0; i < n; i++) {
            cov_item_t *ci;
            off = INT32_AT(code, cur_pc) + pc;
            ce = get_crt_entry(off, map, CRT_FLOW_TARGET);
            if (ce == NULL)
                ce = get_crt_entry(off, map, CRT_ANY);
            ASSRT(ce != NULL || off == def_off, 23, *context);
            type = (off != def_off);
            ci = cov_item_new(pc, CT_CASE, INSTR_ANY, type ? ce->rng_start : 0, type ? ce->rng_end : 0);
            add_to_list_end(&(cov_list[pc]), ci);
            cur_pc += 4;
        }
        add_to_list_end(&(cov_list[pc]), def_item);
        break;
    default:
        ASSRT(0, 24, *context);
    }
}

static void gen_cov_table(jcov_list_t **cov_list, jcov_method_t *meth) {
    SSIZE_T n = 0;
    SSIZE_T code_len = meth->pc_cache_size;
    int i;

    for (i = 0; i < code_len; n += list_size(cov_list[i]), i++);

    meth->covtable_size = n;
    if (n == 0)
        return;
    meth->pc_cache = (int*)jcov_calloc(code_len * sizeof(int));
    meth->covtable = (cov_item_t*)jcov_calloc(n * sizeof(cov_item_t));

    for (i = 0, n = 0; i < code_len; i++) {
        jcov_list_t *l = cov_list[i];
        for (; l != NULL; l = l->next, n++) {
            cov_item_t *ci = (cov_item_t*)l->elem;
            meth->covtable[n] = *ci;
            if (l->next == NULL)
                meth->pc_cache[i] = n + 1;
        }
    }
}

static void gen_cov_for_catch(UINT16      catch_pc,
                       jcov_list_t **cov_map,
                       jcov_list_t **crt_map,
                       UINT8       *code,
                       SSIZE_T     code_len) {
    crt_entry_t *res = NULL;
    int catch_instr_len = 0;
    Bool work_around = 0;
    cov_item_t *cov_item;
    
    if (catch_pc >= code_len)
        return; /* konst: error? */
    res = get_crt_entry(catch_pc, crt_map, CRT_STATEMENT);
    if (res == NULL)
        res = get_crt_entry(catch_pc, crt_map, CRT_BLOCK);
    
    if (res == NULL) {
        /* konst: currently, CRT entry's start PC for a catch handler is 1 instruction greater */
        /* than actual catch handler's offset in bytecode - a compiler bug                     */
        catch_instr_len = opc_lengths[code[catch_pc] & 0xFF];
        if (catch_instr_len <= 0 || catch_instr_len > 5)
            return; /* konst: error? */
        catch_pc += catch_instr_len;
        if (catch_pc >= code_len)
            return; /* konst: error? */
        res = get_crt_entry(catch_pc, crt_map, CRT_STATEMENT);
        if (res == NULL)
            res = get_crt_entry(catch_pc, crt_map, CRT_BLOCK);
        if (res == NULL)
            return; /* konst: error? */
        work_around = 1;
    }
    cov_item = get_cov_item(catch_pc, cov_map, CT_BLOCK);
    if (cov_item != NULL && work_around) {
        cov_item->pc = catch_pc - catch_instr_len;
        return;
    }
    if (cov_item == NULL) {
        UINT16 pc = work_around ? catch_pc - catch_instr_len : catch_pc;
        cov_item = cov_item_new(pc, CT_BLOCK, INSTR_ANY, res->rng_start, res->rng_end);
        add_to_list(&(cov_map[pc]), cov_item);
    }
}

static void free_mem(void *mem) {
    jcov_free(mem);
}

void read_crt_table(int                 attr_len,
                    jcov_method_t       *meth,
                    bin_class_context_t *context) {

    UINT16 i;
    char info[MAX_PATH_LEN];
    Bool new_block;

    int entry_count = 0;
    int err_code = 0;
    SSIZE_T code_len = meth->pc_cache_size;
    UINT8 opcode = 0;
    Bool block_was = 0;
    UINT8 *code = context->code;
    UINT8 *excp_table = code + code_len;
    INT16 excp_table_size = INT16_AT(excp_table, 0);

    cov_item_t  *cov_item = NULL;
    cov_item_t  *min_cov_item = NULL;
    crt_entry_t *crt_entry = NULL;
    crt_entry_t **switch_buf   = (crt_entry_t**)jcov_calloc(code_len * sizeof(crt_entry_t*));
    jcov_list_t **cov_list     = (jcov_list_t**)jcov_calloc(code_len * sizeof(jcov_list_t*));
    jcov_list_t **pc_start2crt = (jcov_list_t**)jcov_calloc(code_len * sizeof(jcov_list_t*));
    jcov_list_t **pc_end2crt   = (jcov_list_t**)jcov_calloc(code_len * sizeof(jcov_list_t*));

    READ_AND_CHECK(entry_count, 2, *context); /* number of entries in the char range table */
    ASSRT(entry_count == 0 || (attr_len - 2) % entry_count == 0, 10, *context);

    /* read CharacterRangeTable entries */
    for (i = 0; i < entry_count; i++) {
        INT32 rng_start, rng_end;
        UINT16 pc_start, pc_end, flags;

        /* read crt entry data */
        READ_AND_CHECK(pc_start, 2, *context);
        READ_AND_CHECK(pc_end,   2, *context);
        READ_AND_CHECK(rng_start, 4, *context);
        READ_AND_CHECK(rng_end,   4, *context);
        READ_AND_CHECK(flags, 2, *context);

        /*ASSRT(pc_start < code_len, 11, *context);*/
        /*ASSRT(pc_end < code_len, 12, *context);*//* commented out due to compiler bug */

        /* select entries of interest and add the to the <pc>-><crt entry> maps */
        if (flags & CRT_STATEMENT   || flags & CRT_FLOW_TARGET ||
            flags & CRT_BRANCH_TRUE || flags & CRT_BRANCH_FALSE) {
            if (pc_start < code_len) { /* workaround a compiler bug */
                crt_entry = crt_entry_new(pc_start, pc_end, rng_start, rng_end, flags);
                add_to_list(&(pc_start2crt[pc_start]), crt_entry);
            }
            if (pc_end < code_len) { /* workaround a compiler bug */
                crt_entry = crt_entry_new(pc_start, pc_end, rng_start, rng_end, flags);
                add_to_list(&(pc_end2crt[pc_end]), crt_entry);
            }
        }

        /* lookupswitch and tableswitch instructions are always preceded
         * by an instruction to which a FLOW_CONTROLLER points in its end pc
         */
        if (flags & CRT_FLOW_CONTROLLER && pc_end + 1 < code_len) {
            opcode = code[pc_end + 1];
            if (opcode == opc_lookupswitch || opcode == opc_tableswitch) {
                /* temporarily store pc in pc_start field */
                switch_buf[pc_end + 1] = crt_entry_new((UINT16)(pc_end + 1), 0, rng_start, rng_end, 0);
            }
        }

        /* try to find source code range for the method's coverage table entry
         * (which will be generated later) indicating the method's coverage
         * (CT_METHOD entry type)
         */
        if (flags & CRT_BLOCK || flags & CRT_STATEMENT) {
            cov_item_t *mci = min_cov_item;
            unsigned char replace = 0;
            if (min_cov_item == NULL) {
                min_cov_item = cov_item_new(pc_start, CT_METHOD, INSTR_ANY, rng_start, rng_end);
                continue;
            }
            if (flags & CRT_BLOCK) { /* this range is a block */
                if (!block_was || /* there were no blocks yet */
                    (rng_start <= mci->where_line && rng_end >= mci->where_pos) || /* new range includes old */
                    rng_start > mci->where_pos) { /* new range is below the old one */

                    block_was = 1;
                    replace = 1;
                }
            } else {
                if (!block_was && pc_start < min_cov_item->pc)
                    replace = 1;
            }
            if (replace) {
                mci->pc = pc_start;
                mci->where_line = rng_start;
                mci->where_pos = rng_end;
            }
        }
    }

    /* find all linear blocks and branches and
     * generate coverage table entries for them
     */
    new_block = 1;
    for (i = 0; i < code_len; i++) {
        jcov_list_t *ls, *le;
        crt_entry_t *ce;
        if (switch_buf[i] != NULL) { /* table- or lookup-switch instruction found at pc=i  */
            gen_cov_for_switch(code, switch_buf[i], cov_list, pc_start2crt, context);
            continue;
        }
        ls = pc_start2crt[i];
        le = pc_end2crt[i];
        if (ls == NULL && le == NULL)
            continue;
        ce = get_crt_entry(i, pc_start2crt, CRT_BRANCH_TRUE | CRT_BRANCH_FALSE);
        if (ce != NULL) { /* jump instruction found at pc=i */
            gen_cov_for_if(i, ce, cov_list, pc_start2crt, context);
            continue;
        }
        ce = get_crt_entry(i, pc_start2crt, CRT_FLOW_TARGET);
        if (ce != NULL) { /* any CRT_FLOW_TARGET begins a block */
            new_block = 0;
            if (get_cov_item(i, cov_list, CT_BLOCK) == NULL) {
                cov_item = cov_item_new(i, CT_BLOCK, INSTR_ANY, ce->rng_start, ce->rng_end);
                add_to_list(&(cov_list[i]), cov_item);
            }
            continue;
        }
        ce = get_crt_entry(i, pc_start2crt, CRT_STATEMENT);
        if (new_block && ce != NULL) { /* a CRT_STATEMENT can begin a block */
            new_block = 0;
            if (get_cov_item(i, cov_list, CT_BLOCK) == NULL) {
                cov_item = cov_item_new(i, CT_BLOCK, INSTR_ANY, ce->rng_start, ce->rng_end);
                add_to_list(&(cov_list[i]), cov_item);
            }
            continue;
        }
        ce = get_crt_entry(i, pc_end2crt, CRT_FLOW_TARGET);
        if (ce != NULL) {
             /* there is always a block after the last instruction of a CRT_FLOW_TARGET
              * (except when the CRT_FLOW_TARGET ends a method)
              */
            new_block = 1;
        }
    }

    if (min_cov_item != NULL) {
        /* add coverage item of type CT_METHOD */
        min_cov_item->pc = 0;
        add_to_list(&(cov_list[0]), min_cov_item);
    }

    /* add blocks for exception handlers */
    excp_table += 2; /* skip 2 bytes of exception_table_length */
    for (i = 0; i < excp_table_size; i++) {
        UINT16 catch_pc, start_pc, end_pc, cur_pc;
        crt_entry_t *ce;
        unsigned char skip = 0;

        start_pc = (UINT16)INT16_AT(excp_table, 0);
        excp_table += 2; /* skip start_pc */
        end_pc = (UINT16)INT16_AT(excp_table, 0);
        excp_table += 2; /* skip end_pc */
        catch_pc = (UINT16)INT16_AT(excp_table, 0); /* handler_pc */

        /* exception tables are also generated for 'synchronized' statements - exception
         * handlers from such tables do not start a linear block, so they should be skipped
         */
        for (cur_pc = 0; cur_pc < code_len; cur_pc += get_instr_size(cur_pc, code)) {
            if (cur_pc >= end_pc)
                break;
            if ((code[cur_pc] & 0xFF) == opc_monitorexit && cur_pc == end_pc - 1) {
                /* end_pc of the guarded bytecode range is immediately preceeded by the
                 * 'monitorexit' instruction - this means that this exception handler
                 * was generated for a 'synchronized' statement. Skip it.
                 */
                skip = 1;
                break;
            }
        }
        if (skip) {
            excp_table += 4; /* skip handler_pc catch_type */
            continue;
        }

        gen_cov_for_catch(catch_pc, cov_list, pc_start2crt, code, code_len);

        ce = get_crt_entry(start_pc, pc_start2crt, CRT_STATEMENT); /* entry for a 'try' */
        if (ce != NULL && ce->pc_end + 1 < code_len) {
            /* try add a block starting at the statement immediately following the closing
             * '}' of the last 'catch' clause of the 'try' statement which is guarded by
             * this (at catch_pc) exception handler.
             */
            UINT16 end_pc = ce->pc_end + 1;
            if (get_cov_item(end_pc, cov_list, CT_BLOCK) == NULL) {
                /* the block is not added yet */
                ce = get_crt_entry(end_pc, pc_start2crt, CRT_STATEMENT);
                if (ce != NULL) {
                    cov_item = cov_item_new(end_pc, CT_BLOCK, INSTR_ANY, ce->rng_start, ce->rng_end);
                    add_to_list(&(cov_list[end_pc]), cov_item);
                }
            }
        }
        excp_table += 4; /* skip handler_pc catch_type */
    }

    gen_cov_table(cov_list, meth);

    for (i = 0; i < code_len; i++) {
        if (switch_buf[i] != NULL) {
            jcov_free(switch_buf[i]);
            switch_buf[i] = NULL;
        }
    }
    jcov_free(switch_buf);

    for (i = 0; i < code_len; free_list(&(cov_list[i]), free_mem), i++);
    jcov_free(cov_list);

    for (i = 0; i < code_len; free_list(&(pc_start2crt[i]), free_mem), i++);
    jcov_free(pc_start2crt);

    for (i = 0; i < code_len; free_list(&(pc_end2crt[i]), free_mem), i++);
    jcov_free(pc_end2crt);
}

void read_cov_table(int                 attr_len,
                    jcov_method_t       *meth,
                    bin_class_context_t *context) {
    int ct_entry_size = 0;
    int entry_count = 0;
    int err_code = 0;
    UINT8 opcode = 0;
    Bool err_told = 0;
    UINT8 *code = context->code;
    cov_item_t *cov_item = NULL;

    int m, pos;
    char info[MAX_PATH_LEN];

    jcov_hooked_class_t *hooked_class = context->hooked_class;

    READ_AND_CHECK(entry_count, 2, *context); /* number of entries in the coverage table */
    if (entry_count <= 0) {
        char *class_name = (hooked_class != NULL && hooked_class->name != NULL) ?
            hooked_class->name : "<unknown>";
        char *meth_name = (meth->name != NULL) ? meth->name : "<unknown>";
        sprintf(info, "Invalid coverage table size (%d). Class: %s, method: %s",
                entry_count, class_name, meth_name);
        jcov_error_stop(info);
    }
    ASSRT((attr_len - 2) % entry_count == 0, 1, *context);
    ct_entry_size = (attr_len - 2) / entry_count;
    ASSRT(ct_entry_size == 8 /* JDK1.1 */ || ct_entry_size == 12 /* >= JDK1.2 */, 2, *context);
    meth->covtable_size = entry_count;
    meth->covtable = (cov_item_t*)jcov_calloc(entry_count * sizeof(cov_item_t));
    meth->pc_cache = (int*)jcov_calloc(meth->pc_cache_size * sizeof(int));
    for (m = 0; m < entry_count; m++) {
        cov_item = &(meth->covtable[m]);
        READ_AND_CHECK(cov_item->pc, 2, *context);
        if (cov_item->pc >= meth->pc_cache_size) {
            if (!err_told) {
                if (hooked_class != NULL && hooked_class->name != NULL && meth->name != NULL)
                    sprintf(info,
                            "invalid CoverageTable attribute in class %s method %s",
                            hooked_class->name,
                            meth->name);
                else
                    sprintf(info, "invalid CoverageTable attribute met");
                jcov_error(info);
                err_told = 1;
            }
            continue;
        }
        opcode = code[cov_item->pc];
        cov_item->instr_type = INSTR_ANY;
        if ((opcode >= opc_ifeq && opcode <= opc_if_acmpne) ||
            opcode == opc_ifnull || opcode == opc_ifnonnull) {
            cov_item->instr_type = INSTR_IF;
        } else if (opcode == opc_tableswitch) {
            cov_item->instr_type = INSTR_TABLESW;
        } else if (opcode == opc_lookupswitch) {
            cov_item->instr_type = INSTR_LOOKUPSW;
        }
        READ_AND_CHECK(cov_item->type, 2, *context);
        if (ct_entry_size == 8) {
            READ_AND_CHECK(pos, 4, *context);
            cov_item->where_line = pos >> JDK11_CT_ENTRY_PACK_BITS;
            cov_item->where_pos = pos & JDK11_CT_ENTRY_POS_MASK;
        } else { /* ct_entry_size == 12 */
            READ_AND_CHECK(cov_item->where_line, 4, *context);
            READ_AND_CHECK(cov_item->where_pos,  4, *context);
        }
        cov_item->count = 0;
        meth->pc_cache[cov_item->pc] = m+1;
    }
}
