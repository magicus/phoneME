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

#include <javacall_util.h>
#include <javacall_memory.h>
#include <pcsl_memory.h>

/**
 * Converts the given MIDP error code to the Javacall error code.
 *
 * @param midpErr MIDP error code
 *
 * @return Javacall error code corresponding to the given MIDP one
 */
javacall_result midp_error2javacall(MIDPError midpErr) {
    javacall_result jcRes;

    /*
     * We can't assume that the MIDP and Javacall constants are identical,
     * so this switch is used.
     */
    switch (midpErr) {
        case ALL_OK: {
            jcRes = JAVACALL_OK;
            break;
        }
        case NOT_FOUND: {
            jcRes = JAVACALL_FILE_NOT_FOUND;
            break;
        }
        case OUT_OF_MEMORY: {
            jcRes = JAVACALL_OUT_OF_MEMORY;
            break;
        }
        case IO_ERROR: {
            jcRes = JAVACALL_IO_ERROR;
            break;
        }
        case SUITE_CORRUPTED_ERROR: {
            jcRes = JAVACALL_FAIL;
            break;
        }
        default: {
            jcRes = JAVACALL_FAIL;
        }
    }

    return jcRes;
}

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
                           pcsl_string* pDstStr) {
    jsize srcStrLen;
    pcsl_string_status pcslRes;

    if (pDstStr == NULL) {
        return BAD_PARAMS;
    }

    if (pSrcStr == NULL) {
        *pDstStr = PCSL_STRING_NULL;
        return ALL_OK;
    }

    if (javautil_unicode_utf16_ulength(pSrcStr, &srcStrLen) != JAVACALL_OK) {
        return GENERAL_ERROR;
    }

    pcslRes =
        pcsl_string_convert_from_utf16((jchar*)pSrcStr, srcStrLen, pDstStr);
    if (pcslRes != PCSL_STRING_OK) {
        return OUT_OF_MEMORY;
    }

    return ALL_OK;
}

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
                           javacall_utf16_string* pDstStr) {
    jsize len = pcsl_string_utf16_length(pSrcStr);
    const jchar* pBuf = pcsl_string_get_utf16_data(pSrcStr);

    if (len > 0) {
        len = (len + 1) << 1; /* length in bytes, + 1 for terminating \0\0 */
        *pDstStr = (javacall_utf16_string)javacall_malloc(len);

        if (*pDstStr == NULL) {
            pcsl_string_release_utf16_data(pBuf, pSrcStr);
            return OUT_OF_MEMORY;
        }

        memcpy((unsigned char*)*pDstStr, (const unsigned char*)pBuf, len);
    } else {
        *pDstStr = NULL;
    }

    pcsl_string_release_utf16_data(pBuf, pSrcStr);

    return ALL_OK;
}

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
                           javacall_int32* pOutArraySize) {
    int i;
    jsize strSize, convertedLength;
    pcsl_string_status pcslRes;
    javacall_utf16_string* pResArray;
    javacall_int32 outSize = 0;
    MIDPError status = ALL_OK;

    do {
        /* allocate memory to hold the output array */
        pResArray = (javacall_utf16_string*)javacall_malloc(
            srcArraySize * sizeof(javacall_utf16_string));

        if (pResArray == NULL) {
            outSize = 0;
            status = OUT_OF_MEMORY;
            break;
        }

        /* convert each string in the source array */
        for (i = 0; i < srcArraySize; i++) {
            strSize = pcsl_string_utf16_length(&pPcslStrArray[i]);

            pResArray[i] =
                (javacall_utf16_string)javacall_malloc((strSize + 1) << 1);

            if (pResArray[i] == NULL) {
                /* save the current array size for proper freeing */
                outSize = i;
                status = OUT_OF_MEMORY;
                break;
            }

            pcslRes = pcsl_string_convert_to_utf16(&pPcslStrArray[i],
                (jchar*)pResArray[i], strSize, &convertedLength);

            if (pcslRes != PCSL_STRING_OK) {
                status = GENERAL_ERROR;
                break;
            }
        }
    } while (0);

    *pOutArraySize = outSize;
    *ppOutArray = pResArray;

    return status;
}
