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

#ifndef _JAVACALL_UTIL_H_
#define _JAVACALL_UTIL_H_

#include <kni.h>
#include <midp_global_status.h>
#include <pcsl_string.h>
#include <javautil_unicode.h>

/**
 * Frees memory occupied by the given Javacall string.
 */
#define JCUTIL_FREE_JC_STRING(str) if (str != NULL) { \
                                javacall_free(str); \
                            }

/**
 * Converts the given MIDP error code to the Javacall error code.
 *
 * @param midpErr MIDP error code
 *
 * @return Javacall error code corresponding to the given MIDP one
 */
javacall_result midp_error2javacall(MIDPError midpErr);

/**
 * Converts the given Javacall string to pcsl string.
 *
 * The caller is responsible for freeing the converted string
 * when it is not needed.
 *
 * @param pSrcStr [in]  a Javacall string to convert
 * @param pDstStr [out] on exit will hold the converted string
 *
 * @return an error code, ALL_OK if no errors
 */
MIDPError
midp_javacall_str2pcsl_str(javacall_const_utf16_string pSrcStr,
                           pcsl_string* pDstStr);

/**
 * Converts the given pcsl string to Javacall string.
 *
 * The caller is responsible for freeing the converted string
 * when it is not needed.
 *
 * @param pSrcStr [in]  a pcsl string to convert
 * @param pDstStr [out] on exit will hold the converted string
 *
 * @return an error code, ALL_OK if no errors
 */
MIDPError
midp_pcsl_str2javacall_str(const pcsl_string* pSrcStr,
                           javacall_utf16_string* pDstStr);

/**
 * Converts the given array of pcsl strings into an array of javacall strings.
 *
 * Note: the caller is responsible for freeing memory allocated for
 *       javacall strings in this function.
 *       Even if the function has returned an error, *pOutArraySize will
 *       hold the number of entries actually allocated in the output array.
 *
 * @param pPcslStrArray [in]  array of pcsl strings to convert
 * @param srcArraySize  [in]  number of strings in the source array
 * @param ppOutArray    [out] on exit will hold a pointer to
 *                            the array of javacall strings
 * @param pOutArraySize [out] on exit will hold the size of
 *                            the allocated string array
 *
 * @return ALL_OK if succeeded, an error code otherwise
 */
MIDPError
pcsl_string_array2javacall(const pcsl_string* pPcslStrArray,
                           jint srcArraySize,
                           javacall_utf16_string** ppOutArray,
                           javacall_int32* pOutArraySize);

#endif /* _JAVACALL_UTIL_H_ */
