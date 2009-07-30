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
 * @file
 *
 * This header file is interface to the internal MIDlet suite storage
 * functions.
 */

#ifndef _SUITESTORE_VERIFY_HASH_H_
#define _SUITESTORE_VERIFY_HASH_H_

#include <kni.h>
#include <midpString.h>
#include <suitestore_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#if VERIFY_ONCE

/**
 * Read from secure persistent storage hash value of the
 * suite whose classes were preverified during installation.
 * The caller should have make sure the suite ID is valid.
 *
 * Note that memory for the hash value MUST be allocated by callee
 * with pcsl_mem_malloc(), the caller is responsible for freeing it.
 *
 * @param suiteId ID of a suite
 * @param ppVerifyHash address of the pointer to receive jbytes array
 *   containing the hash value
 * @param pVerifyHashLen address to receive the length of hash value array
 *
 * @return ALL_OK if successful,
 *         OUT_OF_MEMORY if out of memory,
 *         IO_ERROR if an IO error occurred
 */
MIDPError readVerifyHash(SuiteIdType suiteId, jbyte** ppVerifyHash,
    int* pVerifyHashLen);

/**
 * Write to secure persistent storage hash value of the
 * suite whose classes were preverified during installation.
 * The caller should have make sure the suite ID is valid.
 *
 * @param suiteId ID of a suite
 * @param pVerifyHash pointer to jbytes array with hash value
 * @param verifyHashLen length of the hash value array
 *
 * @return ALL_OK if successful,
 *         OUT_OF_MEMORY if out of memory,
 *         IO_ERROR if an IO error occurred
 */
MIDPError writeVerifyHash(SuiteIdType suiteId, jbyte* pVerifyHash,
    int verifyHashLen);

#endif /* VERIFY_ONCE */

#ifdef __cplusplus
}
#endif

#endif /* _SUITESTORE_VERIFY_HASH_H_ */
