/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 */

#include <sni.h>
#include <kni.h>
#include <midpUtilKni.h>
#include <string.h>
#include <midpError.h>
#include <pcsl_string.h>
#include <pcsl_memory.h>
#include <commonKNIMacros.h>
#include <nim.h>

#define numElems(x) sizeof(x)/sizeof(x[0])

#include <stdio.h>


static constraint_map flags = {
        //  ANY      EMAILADDR   NUMERIC  PHONENUMBER    URL      DECIMAL
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_FULLWIDTH_DIGITS
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_FULLWIDTH_LATIN
        { KNI_TRUE,  KNI_TRUE,  KNI_FALSE, KNI_FALSE, KNI_TRUE,  KNI_FALSE }, // IS_HALFWIDTH_KATAKANA
        { KNI_TRUE,  KNI_TRUE,  KNI_FALSE, KNI_FALSE, KNI_TRUE,  KNI_FALSE }, // IS_HANJA
        { KNI_TRUE,  KNI_TRUE,  KNI_FALSE, KNI_FALSE, KNI_TRUE,  KNI_FALSE }, // IS_KANJI
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_LATIN
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_LATIN_DIGITS
        { KNI_TRUE,  KNI_TRUE,  KNI_FALSE, KNI_FALSE, KNI_TRUE,  KNI_FALSE }, // IS_SIMPLIFIED_HANZI
        { KNI_TRUE,  KNI_TRUE,  KNI_FALSE, KNI_FALSE, KNI_TRUE,  KNI_FALSE }, // IS_TRADITIONAL_HANZI
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // MIDP_UPPERCASE_LATIN
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // MIDP_LOWERCASE_LATIN
        { KNI_TRUE,  KNI_TRUE,  KNI_FALSE, KNI_FALSE, KNI_TRUE,  KNI_FALSE }  // NULL
};

constraint_map* nim_get_constraint_map(jint id, jint* pInstanceData) {
    (void)id;
    (void)pInstanceData;
    return &flags;
}

jint nim_initialize(jint id, jint* pInstanceData) {
    (void)id;
    (void)pInstanceData; /* could malloc() some memory */
    return 0; /* 0 for ok */
}

void nim_finalize(jint id, jint* pInstanceData) {
    (void)id;
    (void)pInstanceData; /* could free() memory */
}

jboolean nim_supports_constraints(jint id, jint* pInstanceData, jint constraints) {
    (void)id;
    (void)pInstanceData;
    (void)constraints;
    return KNI_TRUE;
}

pcsl_string_status nim_get_name(jint id, jint* pInstanceData, pcsl_string* name) {
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(xname)
    { 'x', 'y', 'z', 0 }
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(xname);
    *name = xname;
    (void)id;
    (void)pInstanceData;
    return PCSL_STRING_OK;
}

pcsl_string_status nim_get_command_name(jint id, jint* pInstanceData, pcsl_string* name) {
    return nim_get_name(id, pInstanceData, name);
}

void nim_begin_input(jint id, jint* pInstanceData, const pcsl_string* inputSubset, int constraints) {
    (void)id;
    (void)pInstanceData;
    (void)inputSubset;
    (void)constraints;
}

#define Canvas_RIGHT 5
#define Canvas_UP 1
#define Canvas_DOWN 6
#define Canvas_LEFT 2

/*
    @return string status: -1 -- string error, 0 -- no string returned, 1 -- string indeed is returned
*/
jint nim_process_key(jint id, jint* pInstanceData, jint key, jboolean longPress, jint isClearKey, state_data* state, pcsl_string* stringRes) {

    struct _locals { int x; int y; } *locals; /* to demonstrate the usage */

    (void)id;
    (void)pInstanceData;

    printf("nim_process_key(0x%x='%c',long=%i,clear=%i)\n",key,key,longPress,isClearKey);
    switch((*state)[STATE_NEXT_STATE]) {
    case 0:
        {   /* state of initial invocation */
            pcsl_string_status errc = PCSL_STRING_OK;
            locals = (struct _locals*)pcsl_mem_malloc(sizeof(locals));
            locals->x = 0;
            (*state)[STATE_INTERNAL] = (jint)locals;
            if( key >= 0 ) {
                if (0 == isClearKey &&
                    key != Canvas_RIGHT &&
                    key != Canvas_UP &&
                    key != Canvas_DOWN &&
                    key != Canvas_LEFT &&
                    !longPress) {
                    /* mediator.commit("" + (char)keyCode); */
                    errc = pcsl_string_append_char(stringRes,key);
                    (*state)[STATE_FUNC_TOKEN] = MEDIATOR_COMMIT;
                }
                (*state)[STATE_NEXT_STATE] = 1;
                locals->x ++;
                printf("x=%i\n",locals->x); /* will print 1 */
                return (errc != PCSL_STRING_OK) ? -1 : 1;
            } else {
                /* check if it is 'select', that is, 'enter' */
                (*state)[STATE_NEXT_STATE] = 2;
                (*state)[STATE_INT_ARG] = key;
                (*state)[STATE_FUNC_TOKEN] = MEDIATOR_ISNEWLINEKEY;
                locals->x ++;
                printf("x=%i\n",locals->x); /* will print 1 */
                return 0;
            }
        }
    case 1: /* state after mediator.commit */
        {
            locals = (struct _locals*)(*state)[STATE_INTERNAL];

            locals->x ++;
            printf("x=%i\n",locals->x); /* will print 2 */

            pcsl_mem_free(locals);
            (*state)[STATE_INTERNAL] = 0;
            (*state)[STATE_NEXT_STATE] = 0; /* will exit */
            (*state)[STATE_FINAL_RES] = -3; /* as in Java */

            return 0; /* no string  returned */
        }
    case 2:
        {   /* state after calling mediator.isNewlineKey */
            pcsl_string_status errc = PCSL_STRING_OK;
            locals = (struct _locals*)(*state)[STATE_INTERNAL];

            locals->x ++;
            printf("x=%i check_for_enter=%i\n",locals->x,
                                (*state)[STATE_CALLBACK_RES]);

            if (0 != (*state)[STATE_CALLBACK_RES]) { /* it's "enter" key */
                errc = pcsl_string_append_char(stringRes,'\n');
                (*state)[STATE_FUNC_TOKEN] = MEDIATOR_COMMIT;
            }
            (*state)[STATE_NEXT_STATE] = 1;
            return (errc != PCSL_STRING_OK) ? -1 : 1;
        }
    default:
        return -1;
    }
}

jchar nim_get_pending_char(jint id, jint* pInstanceData) {
    (void)id;
    (void)pInstanceData;
    return 0;
}

pcsl_string_status nim_get_next_match(jint id, jint* pInstanceData, pcsl_string* name) {
    /* meaningless code */
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(xname)
    { 'x', 'y', 'z', 0 }
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(xname);

    (void)id;
    (void)pInstanceData;

    *name = xname;
    return PCSL_STRING_OK;
}

jboolean nim_has_more_matches(jint id, jint* pInstanceData) {
    (void)id;
    (void)pInstanceData;
    return 0;
}

pcsl_string_status nim_get_match_list(jint id, jint* pInstanceData, pcsl_string** match_list, int* nmatches) {
#if 0
    /* this code demonstrates how one can return a list of strings */
    pcsl_string* list = alloc_pcsl_string_list(2);

    (void)id;
    (void)pInstanceData;

    if (NULL == list) {
        return PCSL_STRING_ENOMEM;
    } else {
        PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(xname1)
        { 'x', 'y', 'z', 0 }
        PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(xname1);
        PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(xname2)
        { 'p', 'q', 'r', 0 }
        PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(xname2);
        list[0] = xname1;
        list[1] = xname2;
        *nmatches = 2;
        *match_list = list;
        return PCSL_STRING_OK;
    }
#else
    (void)id;
    (void)pInstanceData;
    *nmatches = 0;
    *match_list = NULL;
	return PCSL_STRING_OK;
#endif
}

void nim_end_input(jint id, jint* pInstanceData) {
    (void)id;
    (void)pInstanceData;
}

static jint input_mode_ids[] =
{
    KEYBOARD_INPUT_MODE,
    NUMERIC_INPUT_MODE,
    ALPHANUMERIC_INPUT_MODE,
    PREDICTIVE_TEXT_INPUT_MODE,
    SYMBOL_INPUT_MODE,
    110,111,120
};
jint* nim_get_input_mode_ids(jint* n) {
    *n = numElems(input_mode_ids);
    return input_mode_ids;
}
