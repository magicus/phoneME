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
 * This header defines an API for working with the Icons Cache.
 */

#ifndef _SUITESTORE_ICON_CACHE_H_
#define _SUITESTORE_ICON_CACHE_H_

#include <suitestore_common.h>

#define MAX_CACHE_ENTRIES_PER_SUITE 1

#define ICON_CACHE_MAGIC   0x41434349
#define ICON_CACHE_VERSION 0x00010000

/** A header of the file containing the cached icons. */
typedef struct _iconCacheHeader {
    /** Magic number. */
    unsigned long magic;
    /** Version of the file. */
    unsigned long version;
    /** Number of cached images (including the removed) in the file. */
    int numberOfEntries;
    /** Number of free entries in the file. */
    int numberOfFreeEntries;
    /*
     * The following variable-length data are located in the file
     * immediately after this structure:
     *
     * numberOfEntries x IconCacheEntry
     */
} IconCacheHeader;

/** A structure representing a cached icon in the file. */
typedef struct _iconCacheEntry {
    /** True if this entry is free, false otherwise. */
    int isFree;
    /** ID of the suite which the icon belongs to. */
    SuiteIdType suiteId;
    /** Length of the icon's binary data. */
    int imageDataLength;
    /** Length of the icon's name, in bytes. */
    jint nameLength;
    /*
     * The following variable-length data are located in the file
     * immediately after this structure:
     *
     * UTF16 string with the icon's name.
     * jchar* pImageName;
     * Icon's binary data.
     * unsigned char* pImageData;
     */
} IconCacheEntry;

/** A structure describing a cached image loaded into memory. */
typedef struct _cachedImageInfo {
    /** True if this entry is free, false otherwise. */
    int isFree;
    /** The name of the cached icon. */
    pcsl_string imageName;
    /** -1 if unknown */
    unsigned long entryOffsetInFile;
    /** Length of the icon's binary data. */
    int imageDataLength;
    /** Pointer to the image bytes */
    unsigned char* pImageData;
    /** Pointer to the next entry in the linked list. */
    /* struct _cachedImageInfo* nextEntry; */
} CachedImageInfo;

/** A structure described all images cached for the particular suite. */
typedef struct _iconCache {
    /** Unique ID identifying the suite. */
    SuiteIdType suiteId;
    /** Number of used entries in pInfo array. */
    int numberOfCachedImages;
    /** Array of structures describing the cached images. */
    CachedImageInfo pInfo[MAX_CACHE_ENTRIES_PER_SUITE];
} IconCache;

/**
 * Initializes the icons cache.
 *
 * @return status code (ALL_OK if successful)
 */
MIDPError midp_load_suites_icons();

/**
 * Frees the memory allocated for icons cache.
 */
void midp_free_suites_icons();

/**
 * Retrieves image bytes of the icon with the given name belonging
 * to the given suite.
 *
 * @param suiteId ID of the suite which the icon belongs to
 * @param pIconName the icon's name
 * @param ppImageData   [out] pointer to a place where the pointer to the
 *                            area inside the cache where the icon's
 *                            bytes are located will be saved
 * @param pImageDataLen [out] pointer to a place where the length of the
 *                            retrieved data will be saved
 *
 * @return status code (ALL_OK if successful)
 */
MIDPError
midp_get_suite_icon(SuiteIdType suiteId, const pcsl_string* pIconName,
                    unsigned char** ppImageData, int* pImageDataLen);

/**
 * Adds a new icon with the given name belonging to the given suite
 * to the cache.
 * If an icon with the same name belonging to the same suite exists,
 * it will be overwritten.
 *
 * @param suiteId ID of the suite which the icon belongs to
 * @param pIconName the icon's name
 *        Note: this function (not the caller) is responsible for allocating
 *              the memory needed to store a copy of the icon's name.
 * @param pImageData pointer to the array containing the icon's bytes
 *        Note: after calling this function the control over the memory
 *              occupied by pImageData is given to midp_add_suite_icon().
 * @param imageDataLen size of data given in pImageData
 *
 * @return status code (ALL_OK if successful)
 */
MIDPError
midp_add_suite_icon(SuiteIdType suiteId, const pcsl_string* pIconName,
                    unsigned char* pImageData, int imageDataLen);

/**
 * Removes all icons belonging to the given suite from the cache.
 *
 * @param suiteId ID of the suite whose icons should be removed
 *
 * @return status code (ALL_OK if successful)
 */
MIDPError midp_remove_suite_icons(SuiteIdType suiteId);

/**
 * Compacts the storage with the cached icons.
 *
 * @return status code (ALL_OK if successful)
 */
MIDPError midp_compact_icons();

#ifdef __cplusplus
}
#endif

#endif /* _SUITESTORE_ICON_CACHE_H_ */
