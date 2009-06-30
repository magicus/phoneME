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

#include <MD5.h>
#include <midpString.h>
#include <midpMalloc.h>
#include <midpStorage.h>
#include <midpDataHash.h>

/* Reset the context of MD5 message digest*/
static void resetContext(MD5_CTX *c) {
    int i;
    c->A = (unsigned long)0x67452301L;
    c->B = (unsigned long)0xefcdab89L;
    c->C = (unsigned long)0x98badcfeL;
    c->D = (unsigned long)0x10325476L;
    c->Nl= 0;
    c->Nh = 0;
    c->num = 0;
    for (i = 0; i < 16; i ++) {
        c->data[i] = 0;
    }
}

/**
 * Evaluates hash value for the file.
 * Current implementation uses MD5 digest to evaluate the value,
 * but SHA, CRC32 and other algorithms can be used as well.
 *
 * Note that implementation MUST allocate memory for the output
 * parameter hashValue using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param filename_str file name whose hash value is needed
 * @param hashValue variable that receives a pointer to the hash value
 * @param hashLen variable that receives hash value length
 * @return one of the error codes:
 * <pre>
 *       MIDP_HASH_OK, MIDP_HASH_IO_ERROR,
 *       MIDP_HASH_OUT_OF_MEM_ERROR
 * </pre>
 */
int midp_get_file_hash(const pcsl_string* filename_str,
    unsigned char **hashValue, int *hashLen) {

    int handle;
    int status;
    MD5_CTX ctx;
    char *pszError;
    unsigned char *buffer;
    long bytesRead;

    *hashLen = 0;
    *hashValue = NULL;
    pszError = NULL;
    status = MIDP_HASH_OK;

    handle = storage_open(&pszError, filename_str, OPEN_READ);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return MIDP_HASH_IO_ERROR;
    }

    do {
        resetContext(&ctx);

        buffer = midpMalloc(HASH_CHUNK_SIZE);
        if (buffer == NULL) {
            status = MIDP_HASH_OUT_OF_MEM_ERROR;
            break;
        }

        /* Iteratively read data portions from the file
         * and update digest value for each portion. */
        do {
            bytesRead = storageRead(&pszError, handle,
                (char*)buffer, HASH_CHUNK_SIZE);
            if (pszError != NULL) {
                status = MIDP_HASH_IO_ERROR;
                break;
            }
            if (bytesRead <= 0) break;
            MD5_Update(&ctx, buffer, bytesRead);
        } while (1);

        midpFree(buffer);
        if (status != MIDP_HASH_OK)
            break;

        /* Allocate and fill result hash value */
        *hashValue = midpMalloc(MD5_LBLOCK);
        if (*hashValue == NULL) {
            status = MIDP_HASH_OUT_OF_MEM_ERROR;
            break;
        }
        MD5_Final(*hashValue, &ctx);
        *hashLen = MD5_LBLOCK;
    } while(0);

    storageClose(&pszError, handle);
    storageFreeError(pszError);
    return status;
}

/**
 * Evaluates hash value for the data block.
 * Current implementation uses MD5 digest to evaluate the value,
 * but SHA, CRC32 and other algorithms can be used as well.
 *
 * Note that implementation MUST allocate memory for the output
 * parameter hashValue using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @return one of the error codes:
 * <pre>
 *       MIDP_HASH_OK,
 *       MIDP_HASH_OUT_OF_MEM_ERROR
 * </pre>
 */
int midpGetDataHash(unsigned char *data, int dataLen,
    unsigned char **hashValue, int *hashLen) {

    MD5_CTX ctx;
    *hashLen = 0;
    *hashValue = NULL;
    *hashValue = midpMalloc(MD5_LBLOCK);
    if (*hashValue == NULL)
        return MIDP_HASH_OUT_OF_MEM_ERROR;

    resetContext(&ctx);
    MD5_Update(&ctx, data, dataLen);
    MD5_Final(*hashValue, &ctx);
    *hashLen = MD5_LBLOCK;
    return MIDP_HASH_OK;
}
