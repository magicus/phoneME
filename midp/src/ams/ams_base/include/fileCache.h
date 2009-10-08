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

#ifndef _FILECACHE_H_
#define _FILECACHE_H_

/**
 * @file
 * @ingroup ams_base
 * API for native image cache and font cache support. Declares methods for creating
 * and dealing with cache.
 */

#include <midpString.h>
#include <suitestore_common.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Remove all the cached files associated with a suite.
 *
 * @param suiteId   Suite ID
 * @param storageId ID of the storage where to create the cache
*/
void deleteFileCache(SuiteIdType suiteId, StorageIdType storageId);

/**
 * Moves cached files from ome storage to another.
 * For security reasons we allow to move cache only to the
 * internal storage.
 *
 * @param suiteId The suite ID
 * @param storageIdFrom ID of the storage where files are cached
 * @param storageIdTo ID of the storage where to move the cache
 */
void moveFileCache(SuiteIdType suiteId, StorageIdType storageIdFrom,
                    StorageIdType storageIdTo);

/**
 * Store a file to cache.
 *
 * @param suiteId    The suite id
 * @param storageId ID of the storage where to create the cache
 * @param resName    The file resource name
 * @param bufPtr     The buffer with the data
 * @param len        The length of the buffer
 * @return           1 if successful, 0 if not
 */
int storeFileToCache(SuiteIdType suiteId, StorageIdType storageId,
                             const pcsl_string * resName,
                             unsigned char *bufPtr, int len);

/**
 * Loads file from cache, if present.
 *
 * @param suiteId    The suite id
 * @param resName    The file resource name
 * @param **bufPtr   Pointer where a buffer will be allocated and data stored
 * @return           -1 if failed, else length of buffer
 */
int loadFileFromCache(SuiteIdType suiteId, const pcsl_string * resName,
                       unsigned char **bufPtr);


#ifdef __cplusplus
}
#endif

#endif /* _FILECACHE_H_ */
