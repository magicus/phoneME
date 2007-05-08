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
 * @file javacall_invoke.h
 * @ingroup CHAPI
 * @brief Content handlers executor interface for JSR-211 CHAPI
 */


/**
 * @defgroup CHAPI JSR-211 Content Handler API (CHAPI)
 *
 *  The following API definitions are required by JSR-211.
 *  These APIs are not required by standard JTWI implementations.
 *
 * @{
 */

#ifndef __JAVACALL_CHAPI_RESULT_H
#define __JAVACALL_CHAPI_RESULT_H

#include <javacall_defs.h>

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/


/**
 * Internal structure.
 * Handle to result buffer for serialized data storage.
 */
typedef struct _JAVAUTIL_CHAPI_RESBUF_{
    javacall_utf16* buf;
    int bufsz;
    int used;
} JAVAUTIL_CHAPI_RESBUF;

#define JAVAUTIL_CHAPI_RESBUF_INITIALIZER  { NULL, 0, 0 }

/**
 * Result buffer for Content Handler, used as OUTPUT parameter of 
 * javacall functions. 
 * Use the @link javautil_chapi_fillHandler() javautil_chapi_fillHandler function to fill this structure.
 */
typedef JAVAUTIL_CHAPI_RESBUF*   javacall_chapi_result_CH;

/**
 * Result buffer for Content Handlers array, used as OUTPUT parameter of 
 * javacall functions.
 * Use the @link javautil_chapi_appendHandler() javautil_chapi_appendHandler function to fill this structure.
 */
typedef JAVAUTIL_CHAPI_RESBUF*   javacall_chapi_result_CH_array;

/**
 * Result buffer for string array, used as OUTPUT parameter of javacall 
 * functions. 
 * Use the @link javautil_chapi_appendString() javautil_chapi_appendString function to fill this structure.
 */
typedef JAVAUTIL_CHAPI_RESBUF*   javacall_chapi_result_str_array;


/**
 * Releases any memory used in result structure
 * @param result output result structure.
 * @return operation status.
 */
void javautil_chapi_clearResult(JAVAUTIL_CHAPI_RESBUF* buffer);

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
        int flag, /*OUT*/ javacall_chapi_result_CH result);


/**
 * Appends string to output string array.
 * @param str appended string
 * @param str_size the string size
 * @param array string array.
 * @return operation status.
 */
javacall_result javautil_chapi_appendString(
        const javacall_utf16* str, int str_size,
        /*OUT*/ javacall_chapi_result_str_array array);

/**
 * Appends string to output string array if it is not already in array.
 * @param str appended string
 * @param str_size the string size
 * @casesens should comparision be case sensetive
 * @param array string array.
 * @return operation status.
 */
javacall_result javautil_chapi_appendUniqueString(const javacall_utf16* str, int str_size, javacall_bool casesens,
												  /*OUT*/ javacall_chapi_result_str_array array);

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
        int flag, /*OUT*/ javacall_chapi_result_CH_array array);


#ifdef __cplusplus
}
#endif/*__cplusplus*/

#endif //__JAVACALL_NATIVE_HANDLERS_EXEC_H
