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


/**
 * @file jsr211_chapi_result.c
 * @ingroup CHAPI
 * @brief javacall registry access implementation
 */


//#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <javacall_defs.h>
#include "jsr211_result.h"

#include <jsrop_memory.h> 
#include <jsrop_suitestore.h> 

/**
 * The granularity of dynamically expanded size for result buffer.
 * Note! The value should mask last bits!
 */
#define RESBUF_GRAN 0xFF


static int GET_CH_SIZE(id_size, suit_size, clas_size) {
	return id_size + 1 + suit_size + 1 + clas_size + 1 + 1;
}

/**
 * Internal structure.
 * Handle to result buffer for serialized data storage.
 */
typedef struct _JSR211_CHAPI_INTERNAL_RESBUF_{
    jchar* buf;
    int bufsz;
    int used;
} JSR211_CHAPI_INTERNAL_RESBUF;


JSR211_RESULT_BUFFER jsr211_create_result_buffer(){
    JSR211_CHAPI_INTERNAL_RESBUF* res = (JSR211_CHAPI_INTERNAL_RESBUF*)
		MALLOC(sizeof(*res));
	if (!res) return 0;
	res->buf = 0;
	res->bufsz = 0;
	res->used = 0;
	return (JSR211_RESULT_BUFFER)res;
}

void jsr211_release_result_buffer(JSR211_RESULT_BUFFER resbuf){
    if (!resbuf) return;
    if (((JSR211_CHAPI_INTERNAL_RESBUF*)resbuf)->buf) {
	FREE(((JSR211_CHAPI_INTERNAL_RESBUF*)resbuf)->buf);
	((JSR211_CHAPI_INTERNAL_RESBUF*)resbuf)->buf = 0;
    }
    ((JSR211_CHAPI_INTERNAL_RESBUF*)resbuf)->bufsz = 0;
	((JSR211_CHAPI_INTERNAL_RESBUF*)resbuf)->used = 0;
}

const jchar* jsr211_get_result_data(JSR211_RESULT_BUFFER resbuf){
    if (!resbuf) return 0;
    return (const jchar*)((JSR211_CHAPI_INTERNAL_RESBUF*)resbuf)->buf;
}


int jsr211_get_result_data_length(JSR211_RESULT_BUFFER resbuf){
    if (!resbuf) return 0;
    return ((JSR211_CHAPI_INTERNAL_RESBUF*)resbuf)->used;
}



/**
 * Assure result buffer (<code>resbuf</code>) capacity to append additional 
 * portion of data by <code>ext</code> javacall_utf16 units.
 */
static jsr211_result assureBufferCap(JSR211_CHAPI_INTERNAL_RESBUF* resbuf, 
                                                                int ext)  {
	JSR211_CHAPI_INTERNAL_RESBUF* resbuf_ = (JSR211_CHAPI_INTERNAL_RESBUF*)resbuf;
    if (resbuf_->used + ext > resbuf_->bufsz) {
        int sz = (resbuf_->used + ext + RESBUF_GRAN) & (~RESBUF_GRAN);
		javacall_utf16* tmp = (javacall_utf16*) REALLOC(resbuf_->buf, sz * sizeof(javacall_utf16));
        if (!tmp) {
            return JSR211_FAILED;
        }
		resbuf_->buf = tmp;
        resbuf_->bufsz = sz;
    }
    return JSR211_OK;
}



/**
 * Serializes handler data into buffer.
 * Variable <code>buf</code> after macros comletion points at the end of 
 * filled area.
 */
static void fill_ch_buf(const javacall_utf16* id, int id_size, 
						const jchar* suit, int suit_size, 
                        const jchar* clas, int clas_size, 
                        int flag, jchar* buf) {
    // put id
    memcpy(buf, id, id_size * sizeof(jchar));
    buf += id_size;
    *buf++ = '\n';

	// put suite_id
	memcpy(buf,suit,suit_size * sizeof(jchar));
	buf += suit_size;
	*buf++ = '\n';

    // put class_name
    memcpy(buf, clas, clas_size * sizeof(jchar));
    buf += clas_size;
    *buf++ = '\n';

    
    // put flag
    *buf = (jchar)flag;
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
jsr211_result jsr211_fillHandler(
        const jchar* id, int id_size,
		const jchar* suit, int suit_size, 
        const jchar* class_name, int class_name_size,
        int flag, /*OUT*/ JSR211_RESULT_CH result) {
    int sz;
    jchar *buf;
	JSR211_CHAPI_INTERNAL_RESBUF* resbuf_ = (JSR211_CHAPI_INTERNAL_RESBUF*)result;	

    sz = GET_CH_SIZE(id_size, suit_size, class_name_size);

    buf = (jchar*) MALLOC(sz * sizeof(jchar));
    if (buf == NULL) {
        return JSR211_FAILED;
    }
    resbuf_->buf = buf;
    resbuf_->bufsz = resbuf_->used = sz;

    fill_ch_buf(id, id_size,suit,suit_size,class_name, class_name_size, flag, buf);
    return JSR211_OK;
}


/**
 * Appends string to output string array.
 * @param str appended string
 * @param str_size the string size
 * @param array string array.
 * @return operation status.
 */
jsr211_result jsr211_appendString(
        const jchar* str, int str_size,
        /*OUT*/ JSR211_RESULT_CHARRAY array) {
    int n;
    JSR211_CHAPI_INTERNAL_RESBUF* array_ = (JSR211_CHAPI_INTERNAL_RESBUF*)array;	

    if (array_->used == 0) {
        array_->used = 1;
        n = 0;
    } else {
        n = array_->buf[0];
    }

    if (JSR211_OK != assureBufferCap(array, str_size + 1)) {
        return JSR211_FAILED;
    }

    array_->buf[array_->used++] = str_size;
    memcpy(array_->buf + array_->used, str, str_size * sizeof(jchar));
    array_->used += str_size;
    array_->buf[0] = n + 1;
    return JSR211_OK;
}

/**
 * Tests if the string is not identical to any of ones included in array.
 * @param str appended string
 * @param str_size the string size
 * @param casesens should comparison be case sensitive
 * @param array string array.
 * @return JSR211_TRUE if string does not present in result yet JSR211_FALSE if does
 */
jsr211_boolean jsr211_isUniqueString(const jchar *str, int sz,
            int casesens, JSR211_RESULT_STRARRAY array) {
	JSR211_CHAPI_INTERNAL_RESBUF* array_ = (JSR211_CHAPI_INTERNAL_RESBUF*)array;	
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
				if (casesens == JAVACALL_TRUE) {
					cmp = wcsncmp(str, ptr, z);
				} else {
					cmp = wcsnicmp(str, ptr, z);
				}
				if (!cmp) return JAVACALL_FALSE;
			}
            ptr += z;
        }
    }

    return JAVACALL_TRUE;
}

/**
 * Tests if the handler id is not identical to any of ones included in content handler array.
 * @param id tested content handler id 
 * @param id_sz the id size in jchars
 * @param array array of content handlers.
 * @return JSR211_TRUE if id does not present in result yet JSR211_FALSE if does
 */
jsr211_boolean jsr211_isUniqueHandler(const jchar *id, int id_sz,
									  JSR211_RESULT_CHARRAY array) {
	JSR211_CHAPI_INTERNAL_RESBUF* array_ = (JSR211_CHAPI_INTERNAL_RESBUF*)array;	
    javacall_utf16 *ptr = array_->buf;
    javacall_utf16 *end;
    int z;

    if (ptr != NULL && array_->used > 0) {
        end = ptr + array_->used;
        ptr++; /* skip number of data items */
        while (ptr < end) {
            z = *ptr++; /* item length */
			if (z>id_sz && (ptr[id_sz]=='\n') && !wcsncmp(id, ptr, id_sz)){
				return JAVACALL_FALSE;
			}
            ptr += z;
        }
    }
	
    return JAVACALL_TRUE;
}


/**
 * Appends string to output string array if it is not already in array.
 * @param str appended string
 * @param str_size the string size
 * @param casesensetive should comparison be case sensitive
 * @param array string array.
 * @return operation status.
 */
jsr211_result jsr211_appendUniqueString(const jchar* str, int str_size, int casesens,
												  /*OUT*/ JSR211_RESULT_CHARRAY array){
	if (JSR211_FALSE == jsr211_isUniqueString(str,str_size,casesens,array)) return JSR211_OK;
	return jsr211_appendString(str,str_size,array);
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
jsr211_result jsr211_appendHandler(
        const jchar* id, int id_size,
        const jchar* suit, int suit_size,
        const jchar* class_name, int class_name_size,
        int flag, /*OUT*/ JSR211_RESULT_CHARRAY array) {

    int sz = GET_CH_SIZE(id_size, suit_size, class_name_size);
    int n;
	JSR211_CHAPI_INTERNAL_RESBUF* array_ = (JSR211_CHAPI_INTERNAL_RESBUF*)array;	

    if (array_->buf == NULL) {
		array_->used = 1;
        n = 0;
    } else {
        n = array_->buf[0];
    }

    if (JSR211_OK != assureBufferCap(array, sz + 1)) {
        return JSR211_FAILED;
    }

    array_->buf[array_->used++] = sz;
    fill_ch_buf(id, id_size, suit, suit_size, class_name, class_name_size, flag, array_->buf + array_->used);
    array_->used += sz;
    array_->buf[0] = n + 1;
    return JSR211_OK;
}


#define MAX_SUITE_ID_SIZE 12

