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


#include <stdlib.h>
#include <string.h>
#include "javacall_chapi_result.h"
#include "inc/javautil_str.h"


/**
 * The granularity of dynamically expanded size for result buffer.
 * Note! The value should mask last bits!
 */
#define RESBUF_GRAN 0xFF


#define GET_CH_SIZE(id_size, clas_size) \
    id_size + class_name_size + 5


/**
 * Tests if the string is not identical to any of ones included in array.
 */
static javacall_bool isUniqueString(const javacall_utf16 *str, int sz,
            javacall_bool caseSens, javacall_chapi_result_str_array array) {
	JAVAUTIL_CHAPI_RESBUF* array_ = (JAVAUTIL_CHAPI_RESBUF*)array;	
    javacall_utf16 *ptr = array_->buf;
    javacall_utf16 *end;
    int z, cmp;

    if (ptr != NULL && array_->used > 0) {
        end = ptr + array_->used;
        ptr++;
        while (ptr < end) {
            z = *ptr++;
			if ( z == sz ) {
				cmp=1;
				if (caseSens == JAVACALL_TRUE) {
					cmp = javautil_wcsncmp(str, ptr, z);
				} else {
					cmp = javautil_wcsincmp(str, ptr, z);
				}
				if (!cmp) return JAVACALL_FALSE;
			}
            ptr += z;
        }
    }

    return JAVACALL_TRUE;
}


/**
 * Assure result buffer (<code>resbuf</code>) capacity to append additional 
 * portion of data by <code>ext</code> javacall_utf16 units.
 */
static javacall_result assureBufferCap(JAVAUTIL_CHAPI_RESBUF* resbuf, 
                                                                int ext)  {
	JAVAUTIL_CHAPI_RESBUF* resbuf_ = (JAVAUTIL_CHAPI_RESBUF*)resbuf;
    if (resbuf_->used + ext > resbuf_->bufsz) {
        int sz = (resbuf_->used + ext + RESBUF_GRAN) & (~RESBUF_GRAN);
		javacall_utf16* tmp = (javacall_utf16*) realloc(resbuf_->buf, sz * sizeof(javacall_utf16));
        if (!tmp) {
            return JAVACALL_OUT_OF_MEMORY;
        }
		resbuf_->buf = tmp;
        resbuf_->bufsz = sz;
    }
    return JAVACALL_OK;
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
 * @param suit suite Id
 * @param class_name handler class name
 * @param class_name_size handler class name size
 * @param flag handler installation flag
 * @param result output result structure.
 * @return operation status.
 */
javacall_result javautil_chapi_fillHandler(
        const javacall_utf16* id, int id_size,
        int suit, 
        const javacall_utf16* class_name, int class_name_size,
        int flag, /*OUT*/ javacall_chapi_result_CH result) {
    int sz;
    javacall_utf16 *buf;
	JAVAUTIL_CHAPI_RESBUF* resbuf_ = (JAVAUTIL_CHAPI_RESBUF*)result;	

    sz = GET_CH_SIZE(id_size, class_name_size);
    buf = (javacall_utf16*) malloc(sz * sizeof(javacall_utf16));
    if (buf == NULL) {
        return JAVACALL_OUT_OF_MEMORY;
    }
    resbuf_->buf = buf;
    resbuf_->bufsz = resbuf_->used = sz;

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
    JAVAUTIL_CHAPI_RESBUF* array_ = (JAVAUTIL_CHAPI_RESBUF*)array;	

    if (array_->used == 0) {
        array_->used = 1;
        n = 0;
    } else {
        n = array_->buf[0];
    }

    if (JAVACALL_OK != assureBufferCap(array, str_size + 1)) {
        return JAVACALL_OUT_OF_MEMORY;
    }

    array_->buf[array_->used++] = str_size;
    memcpy(array_->buf + array_->used, str, str_size * sizeof(javacall_utf16));
    array_->used += str_size;
    array_->buf[0] = n + 1;
    return JAVACALL_OK;
}

/**
 * Appends string to output string array if it is not already in array.
 * @param str appended string
 * @param str_size the string size
 * @casesensetive should comparision be case sensetive
 * @param array string array.
 * @return operation status.
 */
javacall_result javautil_chapi_appendUniqueString(const javacall_utf16* str, int str_size, javacall_bool casesens,
												  /*OUT*/ javacall_chapi_result_str_array array){

	if (!isUniqueString(str,str_size,casesens,array)) return JAVACALL_OK;
	return javautil_chapi_appendString(str,str_size,array);
}

/**
 * Appends the handler data to the result array.
 * @param id handler ID
 * @param id_size handler ID size
 * @param suit suite Id
 * @param class_name handler class name
 * @param class_name_size handler class name size
 * @param flag handler installation flag
 * @param array output result array.
 * @return operation status.
 */
javacall_result javautil_chapi_appendHandler(
        const javacall_utf16* id, int id_size,
        int suit,
        const javacall_utf16* class_name, int class_name_size,
        int flag, /*OUT*/ javacall_chapi_result_CH_array array) {
    int sz = GET_CH_SIZE(id_size, class_name_size);
    int n;
	JAVAUTIL_CHAPI_RESBUF* array_ = (JAVAUTIL_CHAPI_RESBUF*)array;	

    if (array_->buf == NULL) {
        array_->used = 1;
        n = 0;
    } else {
        n = array_->buf[0];
    }

    if (JAVACALL_OK != assureBufferCap(array, sz + 1)) {
        return JAVACALL_OUT_OF_MEMORY;
    }

    array_->buf[array_->used++] = sz;
    fill_ch_buf(id, id_size, suit, class_name, class_name_size, flag, 
                                                    array_->buf + array_->used);
    array_->used += sz;
    array_->buf[0] = n + 1;
    return JAVACALL_OK;
}

/**
 * Releases any memory used in result structure
 * @param result output result structure.
 * @return operation status.
 */
void javautil_chapi_clearResult(JAVAUTIL_CHAPI_RESBUF* resbuf){
	JAVAUTIL_CHAPI_RESBUF* resbuf_ = (JAVAUTIL_CHAPI_RESBUF*)resbuf;
	 if (resbuf_->buf) free(resbuf_->buf);
	 resbuf_->buf = 0;
	 resbuf_->bufsz = 0;
	 resbuf_->used = 0;
}