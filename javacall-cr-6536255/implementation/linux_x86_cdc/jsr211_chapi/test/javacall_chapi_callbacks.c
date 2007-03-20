/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * @file javacall_chapi_callbacks.c
 * @ingroup CHAPI
 * @brief javacall registry access implementation
 */

/**************************************************************
*                                                             *
*	NOTE!                                                 *
*	THIS FILE SHOULD NOT BE INCLUDED TO REGULAR BUILD     *
*	FOR STANDALONE JAVACALL IMPLEMENTATION TESTING ONLY!  *
*                                                             *
***************************************************************/


#include "javacall_chapi_callbacks.h"
#include <string.h>
#include <stdlib.h>

typedef int SuiteIdType;

#define _SUITE_ID_LEN 8
/**
 * The granularity of dynamically expanded size for result buffer.
 * Note! The value should mask last bits!
 */
#define RESBUF_GRAN 0xFF


#define GET_CH_SIZE(id_size, clas_size) \
    id_size + class_name_size + 5


/**
 * String length calculation.
 * In win32 implementation used standart strlen() procedure for wide chars.
 * Parameter 'str' of 'javacall_utf16_string' type (terminated with zero).
 */
#define CHAPI_STRLEN(str)   wcslen(str)


/**
 * String strict comparison
 * Parameters:
 * const javacall_utf16* str1
 * const javacall_utf16* str2
 * int sz
 * Result: boolean true if strings are fully identical.
 */
#define CHAPI_ISEQUAL(str1, str2, sz) (0 == memcmp((str1), (str2), (sz) * sizeof(javacall_utf16)))

/**
 * String case insensitive comparison
 * Parameters:
 * const javacall_utf16* str1
 * const javacall_utf16* str2
 * int sz
 * Result: boolean true if strings are lexically identical.
 */
#define CHAPI_ISEQUAL_I(str1, str2, sz) (0 == _wcsnicmp((str1), (str2), (sz)))


/**
 * Assure result buffer (<code>resbuf</code>) capacity to append additional 
 * portion of data by <code>ext</code> javacall_utf16 units.
 */
static javacall_result assureBufferCap(_JAVAUTIL_CHAPI_RESBUF_* resbuf, 
                                                                int ext)  {
    if (resbuf->used + ext > resbuf->bufsz) {
        int sz = (resbuf->used + ext + RESBUF_GRAN) & (~RESBUF_GRAN);
		resbuf->buf = (javacall_utf16*) realloc(resbuf->buf, 
											sz * sizeof(javacall_utf16));
        if (resbuf->buf == NULL) {
            return JAVACALL_OUT_OF_MEMORY;
        }
        resbuf->bufsz = sz;
    }
    return JAVACALL_OK;
}



static SuiteIdType get_midp_suite(const javacall_utf16* jc_suite, int sz) {
    int ret = 0;
    
    if (sz == _SUITE_ID_LEN) {
        int hex;
        const javacall_utf16 *p = jc_suite;

        while (sz-- > 0) {
            if (ret != 0 || *p != '0') {
                hex = *p > '9'? *p - 'A' + 0xA: *p - '0';
                if (hex < 0 || hex > 0xF) {
                    ret = 0;
                    break; // Invalid suite ID
                } 

                ret <<= 4;
                ret |= hex;
            }
            p++;
        }
        
    }

    return ret;
}

/**
 * Serializes handler data into buffer.
 * Variable <code>buf</code> after macros comletion points at the end of 
 * filled area.
 */
static void fill_ch_buf(const javacall_utf16* id, int id_size, int suit, 
                        const javacall_utf16* clas, int clas_size, 
                        int flag, javacall_utf16* buf) {
    // put ID
    memcpy(buf, id, id_size * sizeof(javacall_utf16));
    buf += id_size;
    *buf++ = '\n';

    // put class_name
    memcpy(buf, clas, clas_size * sizeof(javacall_utf16));
    buf += clas_size;
    *buf++ = '\n';
    
    // put suiteId and flag
    *buf++ = (javacall_utf16)(suit >> 16);
    *buf++ = (javacall_utf16)suit;
    *buf = flag;
}


/**
 * Fills output result structure with handler data.
 * @param id handler ID
 * @param id_size handler ID size
 * @param suite_id suite Id
 * @param suite_id_size suite ID size
 * @param class_name handler class name
 * @param class_name_size handler class name size
 * @param flag handler installation flag
 * @param result output result structure.
 * @return operation status.
 */
javacall_result javautil_chapi_fillHandler(
        const javacall_utf16* id, int id_size,
        const javacall_utf16* suite_id, int suite_id_size,
        const javacall_utf16* class_name, int class_name_size,
        int flag, /*OUT*/ javacall_chapi_result_CH result) {
    int sz;
    SuiteIdType suit = get_midp_suite(suite_id, suite_id_size);
    javacall_utf16 *buf;

    sz = GET_CH_SIZE(id_size, class_name_size);
    buf = (javacall_utf16*) malloc(sz * sizeof(javacall_utf16));
    if (buf == NULL) {
        return JAVACALL_OUT_OF_MEMORY;
    }
    result->buf = buf;
    result->bufsz = result->used = sz;

    fill_ch_buf(id, id_size, suit, class_name, class_name_size, flag, buf);
    return JAVACALL_OK;
}


/**
 * Appends string to output string array.
 * @param str appended string
 * @param str_size the string size
 * @param array string array.
 * @return operation status.
 */
javacall_result javautil_chapi_appendString(
        const javacall_utf16* str, int str_size,
        /*OUT*/ javacall_chapi_result_str_array array) {
    int n;

    if (array->used == 0) {
        array->used = 1;
        n = 0;
    } else {
        n = array->buf[0];
    }

    if (JAVACALL_OK != assureBufferCap(array, str_size + 1)) {
        return JAVACALL_OUT_OF_MEMORY;
    }

    array->buf[array->used++] = str_size;
    memcpy(array->buf + array->used, str, str_size * sizeof(javacall_utf16));
    array->used += str_size;
    array->buf[0] = n + 1;
    return JAVACALL_OK;
}


/**
 * Appends the handler data to the result array.
 * @param id handler ID
 * @param id_size handler ID size
 * @param suite_id suite Id
 * @param suite_id_size suite ID size
 * @param class_name handler class name
 * @param class_name_size handler class name size
 * @param flag handler installation flag
 * @param array output result array.
 * @return operation status.
 */
javacall_result javautil_chapi_appendHandler(
        const javacall_utf16* id, int id_size,
        const javacall_utf16* suite_id, int suite_id_size,
        const javacall_utf16* class_name, int class_name_size,
        int flag, /*OUT*/ javacall_chapi_result_CH_array array) {
    int sz = GET_CH_SIZE(id_size, class_name_size);
    SuiteIdType suit = get_midp_suite(suite_id, suite_id_size);
    int n;

    if (array->buf == NULL) {
        array->used = 1;
        n = 0;
    } else {
        n = array->buf[0];
    }

    if (JAVACALL_OK != assureBufferCap(array, sz + 1)) {
        return JAVACALL_OUT_OF_MEMORY;
    }

    array->buf[array->used++] = sz;
    fill_ch_buf(id, id_size, suit, class_name, class_name_size, flag, 
                                                    array->buf + array->used);
    array->used += sz;
    array->buf[0] = n + 1;
    return JAVACALL_OK;
}


/**
 * Tests if the string is not identical to any of ones included in array.
 */
javacall_bool isUniqueString(const javacall_utf16 *str, int sz,
            javacall_bool caseSens, javacall_chapi_result_str_array array) {
    javacall_utf16 *ptr = array->buf;
    javacall_utf16 *end;
    int z;

    if (ptr != NULL && array->used > 0) {
        end = ptr + array->used;
        ptr++;
        while (ptr < end) {
            z = *ptr++;
            if (z == sz && 
                ((caseSens == JAVACALL_TRUE && CHAPI_ISEQUAL(str, ptr, z)) ||
                (caseSens == JAVACALL_FALSE && CHAPI_ISEQUAL_I(str, ptr, z)))) {
                return JAVACALL_FALSE;
            }
            ptr += z;
        }
    }

    return JAVACALL_TRUE;
}

