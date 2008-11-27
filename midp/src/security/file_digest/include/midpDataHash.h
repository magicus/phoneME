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

#ifndef _MIDP_DATAHASH_H_
#define _MIDP_DATAHASH_H_

/**
 * @file
 * API for hash value calculations. Declares methods to obtain
 * hash value by different data objects.
 */

#include <midpString.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Successful hash evaluation */
#define MIDP_HASH_OK  0
/* Out of memory during hash evaluation */
#define MIDP_HASH_OUT_OF_MEM_ERROR  -2
/* Data read error */
#define MIDP_HASH_IO_ERROR  -3

/* Data portion size (in bytes) for hash evaluation */
#define HASH_CHUNK_SIZE  10240

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
    unsigned char **hashValue, int *hashLen);

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
    unsigned char **hashValue, int *hashLen);

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_DATACACHE_H_ */
