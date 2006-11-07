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

#include <string.h>
#include <midpMalloc.h>
#include <suitestore_util.h>

#if VERIFY_ONCE

/** Resource name to save the suite hash on classes preverification. */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(VERIFY_HASH_RESOURCENAME)
    {'V', 'e', 'r', 'i', 'f', 'y', '_', 'H', 'a', 's', 'h', '\0'};
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(VERIFY_HASH_RESOURCENAME);

/**
 * Read from secure persistent storage hash value of the
 * suite whose classes were preverified during installation.
 * The caller should have make sure the suite ID is valid.
 *
 * Note that memory for the hash value MUST be allocated by callee
 * with midpMalloc, the caller is responsible for freeing it.
 *
 * @param suiteID ID of a suite
 * @param ppVerifyHash address of the pointer to receive jbytes array
 *   containing the hash value
 * @param pVerifyHashLen address to receive the length of hash value array
 *
 * @return 0 if successful,
 *      OUT_OF_MEM_LEN if out of memory,
 *      IO_ERROR_LEN if an IO error occurred
 */
int readVerifyHash(const pcsl_string* suiteID,
    jbyte** ppVerifyHash, int* pVerifyHashLen) {
    MIDP_ERROR error = midp_suite_read_secure_resource(
        suiteID, &VERIFY_HASH_RESOURCENAME, ppVerifyHash, pVerifyHashLen);
    if (error == MIDP_ERROR_NONE)
        return ALL_OK;
    else if (error == MIDP_ERROR_OUT_MEM)
        return OUT_OF_MEM_LEN;
    else return IO_ERROR_LEN;
}

/**
 * Write to secure persistent storage hash value of the
 * suite whose classes were preverified during installation.
 * The caller should have make sure the suite ID is valid.
 *
 * @param suiteID ID of a suite
 * @param pVerifyHash pointer to jbytes array with hash value
 * @param verifyHashLen length of the hash value array
 *
 * @return 0 if successful,
 *      OUT_OF_MEM_LEN if out of memory,
 *      IO_ERROR_LEN if an IO error occurred
 */
int writeVerifyHash(const pcsl_string* suiteID,
    jbyte* pVerifyHash, int verifyHashLen) {

    MIDP_ERROR error = midp_suite_write_secure_resource(
        suiteID, &VERIFY_HASH_RESOURCENAME, pVerifyHash, verifyHashLen);
    if (error == MIDP_ERROR_NONE)
        return ALL_OK;
    else if (error == MIDP_ERROR_OUT_MEM)
        return OUT_OF_MEM_LEN;
    else return IO_ERROR_LEN;
}

#endif /* VERIFY_ONCE */
