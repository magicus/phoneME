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
#include <commonKNIMacros.h>
#include <nim.h>

#define numElems(x) sizeof(x)/sizeof(x[0])

#include <stdio.h>


static constraint_map flags = {
        //  ANY      EMAILADDR   NUMERIC  PHONENUMBER    URL      DECIMAL
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_FULLWIDTH_DIGITS
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_FULLWIDTH_LATIN
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_HALFWIDTH_KATAKANA
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_HANJA
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_KANJI
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_LATIN
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_LATIN_DIGITS
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_SIMPLIFIED_HANZI
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // IS_TRADITIONAL_HANZI
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // MIDP_UPPERCASE_LATIN
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }, // MIDP_LOWERCASE_LATIN
        { KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE, KNI_FALSE }  // NULL
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
    { 's', 't', 'u', 'b', 0 }
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

jint nim_process_key(jint id, jint* pInstanceData, jint key, jboolean longPress, jint isClearKey, state_data* state, pcsl_string* stringRes) {
    (void)id;
    (void)pInstanceData;
    (void)key;
    (void)longPress;
    (void)isClearKey;
    (void)state;
    (void)stringRes;
    return 0;
}

jchar nim_get_pending_char(jint id, jint* pInstanceData) {
    (void)id;
    (void)pInstanceData;
    return 0;
}

pcsl_string_status nim_get_next_match(jint id, jint* pInstanceData, pcsl_string* name) {
    (void)id;
    (void)pInstanceData;
    *name = PCSL_STRING_EMPTY;
    return PCSL_STRING_OK;
}

jboolean nim_has_more_matches(jint id, jint* pInstanceData) {
    (void)id;
    (void)pInstanceData;
    return 0;
}

pcsl_string_status nim_get_match_list(jint id, jint* pInstanceData, pcsl_string** match_list, int* nmatches) {
    (void)id;
    (void)pInstanceData;

    *nmatches = 0;
    *match_list = NULL;
    return PCSL_STRING_OK;
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
    VIRTUAL_INPUT_MODE
    /* native input modes, if any */
/*
    110,111,120
*/
};

jint* nim_get_input_mode_ids(jint* n) {
    *n = numElems(input_mode_ids);
    return input_mode_ids;
}
