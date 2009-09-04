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

#ifndef _FONTCACHE_H_
#define _FONTCACHE_H_

/**
 * @file
 * @ingroup ams_base
 * API for font cache. Declares methods for creating cache
 * and dealing with cached fonts.
 */

#include <midpString.h>
#include <suitestore_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Loads a FONT from cache, if present.
 *
 * @param suiteID   Suite id
 * @param resName   Name of the FONT resource
 * @param bufPtr    Pointer to buffer pointer. Caller will need
 *                  to free this on return.
 *
 * @return handle to a jar or NULL if an error with pError containing a status
 */
/* Not necessary at this moment
int loadFontFromCache(SuiteIdType suiteID, const pcsl_string * resName,
                       unsigned char **bufPtr);
*/

/**
 * Creates a cache of font by iterating over all fonts in the jar
 * file, loading each one, and caching it persistent store.
 *
 * @param suiteId The suite ID
 * @param storageId ID of the storage where to create the cache
 * @param pOutDataSize [out] points to a place where the size of the
 *                           written data is saved; can be NULL
 */
void createFontCache(SuiteIdType suiteID, StorageIdType storageId,
                      jint* pOutDataSize);

/**
 * Moves cached font from one storage to another.
 *
 * @param suiteId The suite ID
 * @param storageIdFrom ID of the storage where font are cached
 * @param storageIdTo ID of the storage where to move the cache
 */
void moveFontCache(SuiteIdType suiteID, StorageIdType storageIdFrom,
                    StorageIdType storageIdTo);

#ifdef __cplusplus
}
#endif

#endif /* _FONTCACHE_H_ */
