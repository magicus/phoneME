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
#include <string.h>
#include <stdio.h>

#include <midp_logging.h>
#include <midpMalloc.h>
#include <suitestore_common.h>
#include <midp_ams_status.h>

#include <findMidlet.h>
#include <midpUtilKni.h>

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(KEY_PREFIX)
    {'M', 'I', 'D', 'l', 'e', 't', '-', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(KEY_PREFIX);

int
find_midlet_class(SuiteIdType id, int midletNumber, pcsl_string* res) {
    MidpProperties properties;
    pcsl_string keySuffix = PCSL_STRING_NULL;
    pcsl_string key = PCSL_STRING_NULL;
    const pcsl_string* property = &PCSL_STRING_NULL;
    pcsl_string temp = PCSL_STRING_NULL;
    pcsl_string_status stat;
    int begin;
    int result = 0;
    *res = PCSL_STRING_NULL;

    properties = midp_get_suite_properties(id);
    if (OUT_OF_MEM_PROPERTY_STATUS(properties)) {
        return OUT_OF_MEM_LEN;
    }

    do {
        if (CORRUPTED_PROPERTY_STATUS(properties)) {
            midp_free_properties(&properties);
            REPORT_ERROR(LC_AMS, "Error : Suite is corrupted");
            fprintf(stderr, "Error : Suite is corrupted\n");
            result = NULL_LEN;
            break;
        }

        if (READ_ERROR_PROPERTY_STATUS(properties)) {
            midp_free_properties(&properties);
            REPORT_ERROR(LC_AMS, "Corrupt properties");
            fprintf(stderr, "Corrupt properties\n");
            result = NULL_LEN;
            break;
        }

        stat = pcsl_string_convert_from_jint(midletNumber, &keySuffix);
        if (PCSL_STRING_OK != stat) {
            if(PCSL_STRING_ENOMEM == stat) {
                result = OUT_OF_MEM_LEN;
            } else {
                result = NULL_LEN;
            }
            break;
        }

        stat = pcsl_string_cat(&KEY_PREFIX, &keySuffix, &key);
        pcsl_string_free(&keySuffix);
        if (PCSL_STRING_OK != stat) {
            result = OUT_OF_MEM_LEN;
            break;
        }

        property = midp_find_property(&properties, &key);
        if (pcsl_string_length(property) <= 0) {
            /* property not found */
            result = NULL_LEN;
            break;
        }

        /* The class is the last item in the set. */
        begin = pcsl_string_last_index_of(property, (jchar)',');
        if (begin < 0 || begin >= pcsl_string_length(property)) {
            result = NULL_LEN;
            break;
        }

        begin++;
        stat = pcsl_string_substring(property, begin, pcsl_string_length(property), &temp);
        if (PCSL_STRING_OK != stat) {
            result = OUT_OF_MEM_LEN;
            break;
        }
        if (pcsl_string_length(&temp) <= 0) {
            pcsl_string_free(&temp);
            result = NULL_LEN;
            break;
        }
        stat = pcsl_string_trim(&temp, res);
        pcsl_string_free(&temp);
        if (PCSL_STRING_OK != stat) {
            result = OUT_OF_MEM_LEN;
            break;
        }
    } while (0);

    midp_free_properties(&properties);
    return result;
}

