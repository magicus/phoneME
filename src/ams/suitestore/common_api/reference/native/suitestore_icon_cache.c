/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 * @ingroup AMS
 *
 * This is reference implementation of the Suite Storage Listeners API.
 * It allows to register/unregister callbacks that will be notified
 * when the certain operation on a midlet suite is performed.
 */

#include <string.h>
#include <pcsl_memory.h>
#include <midpInit.h>
#include <suitestore_intern.h>
#include <suitestore_icon_cache.h>

/**
 * Maximal number of free entries allowed in the file containing the cache until
 * it is compacted (i.e. rewritten without free entries).
 */
#define MAX_FREE_ENTRIES 10

/**
 * A number of additional free entries that will be allocated in
 * g_pIconCache array to avoid memory reallocations when a new
 * icon is added into the cache.
 */
#define RESERVED_CACHE_ENTRIES_NUM 10

/**
 * An array of IconCache structures representing
 * the icon cache in the memory.
 */
static IconCache* g_pIconCache = NULL;

/** A flag indicating if the icon cache is loaded from file into memory. */
static int g_iconsLoaded = 0;

/** Number of currently occupied entries in the g_pIconCache array. */
static int g_numberOfIcons = 0;

/** Number of entries currently allocated in the g_pIconCache array. */
static int g_numberOfEntries = 0;

#define ADJUST_POS_IN_BUF(pos, bufferLen, n) \
    pos += n; \
    bufferLen -= n;

/**
 * Search for a structure containing the cached suite's icon(s)
 * by the suite's ID.
 *
 * @param suiteId unique ID of the midlet suite
 *
 * @return pointer to the IconCache structure containing
 * the cached suite's icon(s) or NULL if the it was not found
 */
static IconCache*
get_icon_cache_for_suite(SuiteIdType suiteId) {
    IconCache* pData;
    int i;

    if (!g_iconsLoaded) {
        MIDPError status = midp_load_suites_icons();
        if (status != ALL_OK) {
            return NULL;
        }
    }

    pData = g_pIconCache;

    /* search the given suite Id in the array of cache entries */
    for (i = 0; i < g_numberOfIcons; i++) {
        if (g_pIconCache[i].suiteId == suiteId) {
            return &g_pIconCache[i];
        }
    }

    return NULL;
}

/**
 * Initializes the icons cache.
 *
 * @return status code: ALL_OK if no errors,
 *         OUT_OF_MEMORY if malloc failed
 *         IO_ERROR if an IO_ERROR
 */
MIDPError midp_load_suites_icons() {
    int i;
    long bufferLen, pos, alignedOffset;
    char* buffer = NULL;
    char* pszError = NULL;
    unsigned char* pIconBytes;
    pcsl_string_status rc;
    pcsl_string iconsCacheFile;
    IconCache *pIconsData, *pData;
    IconCacheHeader* pCacheFileHeader;
    IconCacheEntry* pNextCacheEntry;
    int numOfIcons = 0, numOfEntries = 0;
    MIDPError status;

    if (g_iconsLoaded) {
        return ALL_OK;
    }

    if (midpInit(LIST_LEVEL) != 0) {
        return OUT_OF_MEMORY;
    }

    /* get a full path to the _suites.dat */
    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &ICON_CACHE_FILENAME, &iconsCacheFile);
    if (rc != PCSL_STRING_OK) {
        return OUT_OF_MEMORY;
    }

    /* read the file */
    status = read_file(&pszError, &iconsCacheFile, &buffer, &bufferLen);
    pcsl_string_free(&iconsCacheFile);
    storageFreeError(pszError);

    if (status == NOT_FOUND || (status == ALL_OK && bufferLen == 0)) {
        /* _icons.dat is absent or empty, it's a normal situation */
        g_pIconCache  = NULL;
        g_iconsLoaded = 1;
        return ALL_OK;
    }

    if (status == ALL_OK && bufferLen < (long)(sizeof(unsigned long)<<1)) {
        status = IO_ERROR; /* _icons.dat is corrupted */
    }
    if (status != ALL_OK) {
        pcsl_mem_free(buffer);
        return status;
    }

    /* parse its contents */
    pos = 0;

    /* checking the file header */
    pCacheFileHeader = (IconCacheHeader*)buffer;
    if (pCacheFileHeader->magic   != ICON_CACHE_MAGIC ||
        pCacheFileHeader->version != ICON_CACHE_VERSION ||
        pCacheFileHeader->numberOfFreeEntries >
            pCacheFileHeader->numberOfEntries) {
        pcsl_mem_free(buffer);
        return IO_ERROR;
    }

    numOfIcons   = pCacheFileHeader->numberOfEntries -
                       pCacheFileHeader->numberOfFreeEntries;
    numOfEntries = numOfIcons + RESERVED_CACHE_ENTRIES_NUM;

    ADJUST_POS_IN_BUF(pos, bufferLen, sizeof(IconCacheHeader));

    pIconsData = (IconCache*) pcsl_mem_malloc(sizeof(IconCache) * numOfEntries);
    if (pIconsData == NULL) {
        pcsl_mem_free(buffer);
        return OUT_OF_MEMORY;
    }

    /* iterating through the cache entries */
    for (i = 0; i < pCacheFileHeader->numberOfEntries; i++) {
        if ((long)sizeof(IconCacheEntry) > bufferLen) {
            status = IO_ERROR;
            break;
        }

        pNextCacheEntry = (IconCacheEntry*)&buffer[pos];

        if (pNextCacheEntry->isFree) {
            alignedOffset = sizeof(IconCacheEntry) +
                pNextCacheEntry->nameLength + pNextCacheEntry->imageDataLength;
            alignedOffset = SUITESTORE_ALIGN_4(alignedOffset);
            ADJUST_POS_IN_BUF(pos, bufferLen, alignedOffset);
            continue;
        }

        pData = &pIconsData[i];
        pData->suiteId = pNextCacheEntry->suiteId;
        pData->numberOfCachedImages = 1;

        pData->pInfo[0].isFree = 0;
        pData->pInfo[0].entryOffsetInFile = pos;
        pData->pInfo[0].pImageData = (unsigned char*)pcsl_mem_malloc(
            pNextCacheEntry->imageDataLength);
        if (pData->pInfo[0].pImageData == NULL) {
            status = OUT_OF_MEMORY;
            break;
        }

        pIconBytes = (unsigned char*)pNextCacheEntry + sizeof(IconCacheEntry) +
            pNextCacheEntry->nameLength;
        if (sizeof(IconCacheEntry) + pNextCacheEntry->nameLength +
                pNextCacheEntry->imageDataLength > (unsigned long)bufferLen) {
            status = IO_ERROR;
            break;
        }

        pData->pInfo[0].imageDataLength = pNextCacheEntry->imageDataLength;

        memcpy(pData->pInfo[0].pImageData, pIconBytes,
               pNextCacheEntry->imageDataLength);

        rc = pcsl_string_convert_from_utf16(
            (jchar*)((char*)pNextCacheEntry + sizeof(IconCacheEntry)),
            pNextCacheEntry->nameLength >> 1,
            &pData->pInfo[0].imageName);
        if (rc != PCSL_STRING_OK) {
            status = OUT_OF_MEMORY;
            break;
        }

        alignedOffset = sizeof(IconCacheEntry) +
            pNextCacheEntry->nameLength + pNextCacheEntry->imageDataLength;
        alignedOffset = SUITESTORE_ALIGN_4(alignedOffset);
        ADJUST_POS_IN_BUF(pos, bufferLen, alignedOffset);
    } /* end for (numOfEntries) */

    if (status == ALL_OK) {
        g_numberOfIcons = numOfIcons;
        g_numberOfEntries = numOfEntries;
        g_pIconCache    = pIconsData;
        g_iconsLoaded   = 1;

        if (pCacheFileHeader->numberOfFreeEntries > MAX_FREE_ENTRIES) {
            (void)midp_compact_icons();
        }
    } else {
        /* IMPL_NOTE: free the memory allocated for the image itself */
        pcsl_mem_free(pIconsData);
    }

    pcsl_mem_free(buffer);

    return status;
}

/**
 * Writes the cache contents into the icon cache file.
 *
 * @param pIconCache currently unused; may be useful to optimize
 *                   the writing of the file
 *
 * @return status code: ALL_OK if no errors,
 *         OUT_OF_MEMORY if malloc failed
 *         IO_ERROR if an IO_ERROR
 */
static MIDPError store_suites_icons(const IconCache* pIconCache) {
    MIDPError status = ALL_OK;
    int i;
    long bufferLen, pos, alignedOffset;
    char* buffer = NULL;
    char *pszError = NULL;
    pcsl_string_status rc;
    pcsl_string iconsCacheFile;
    IconCache *pData;
    IconCacheHeader* pCacheFileHeader;
    IconCacheEntry* pNextCacheEntry;

    if (pIconCache == NULL) {
        return BAD_PARAMS;
    }

    /* IMPL_NOTE: currently the whole file is rewritten */

    /* get a full path to the _icons.dat */
    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &ICON_CACHE_FILENAME, &iconsCacheFile);
    if (rc != PCSL_STRING_OK) {
        return OUT_OF_MEMORY;
    }

    if (!g_numberOfIcons) {
        /* there are no icons to cache, truncate the file */
        status = write_file(&pszError, &iconsCacheFile, buffer, 0);
        storageFreeError(pszError);
        pcsl_string_free(&iconsCacheFile);
        return status;
    }

    /* allocate a buffer to store icons data */
    bufferLen = g_numberOfIcons * sizeof(IconCacheEntry) +
        sizeof(IconCacheHeader);

    for (i = 0; i < g_numberOfIcons; i++) {
        long strLen, dataLen = 0;
        pData = &g_pIconCache[i];
        if (pData->pInfo[0].isFree) {
            continue;
        }

        dataLen += pData->pInfo[0].imageDataLength;

        strLen = pcsl_string_utf16_length(&pData->pInfo[0].imageName);
        strLen <<= 1;
        if (strLen > 0) {
            dataLen += strLen;
        }

        bufferLen += SUITESTORE_ALIGN_4(dataLen);
    }

    buffer = pcsl_mem_malloc(bufferLen);
    if (buffer == NULL) {
        pcsl_string_free(&iconsCacheFile);
        return OUT_OF_MEMORY;
    }

    /* assemble the information about all icons into the allocated buffer */
    pos = 0;

    pCacheFileHeader = (IconCacheHeader*)buffer;
    pCacheFileHeader->magic   = ICON_CACHE_MAGIC;
    pCacheFileHeader->version = ICON_CACHE_VERSION;
    pCacheFileHeader->numberOfEntries = g_numberOfIcons;
    pCacheFileHeader->numberOfFreeEntries = 0;

    ADJUST_POS_IN_BUF(pos, bufferLen, sizeof(IconCacheHeader));

    for (i = 0; i < g_numberOfIcons; i++) {
        pData = &g_pIconCache[i];

        if (pData->pInfo[0].isFree) {
            if (pCacheFileHeader->numberOfEntries > 0) {
                pCacheFileHeader->numberOfEntries--;
            }
            continue;
        }

        pNextCacheEntry = (IconCacheEntry*)&buffer[pos];
        pNextCacheEntry->isFree = 0;
        pNextCacheEntry->suiteId = pData->suiteId;
        pNextCacheEntry->imageDataLength = pData->pInfo[0].imageDataLength;

        rc = pcsl_string_convert_to_utf16(&pData->pInfo[0].imageName,
            (jchar*)((char*)pNextCacheEntry + sizeof(IconCacheEntry)),
                bufferLen / sizeof(jchar),
                    &(pNextCacheEntry->nameLength));
        if (rc != PCSL_STRING_OK) {
            status = OUT_OF_MEMORY;
            break;
        }

        /* convert UTF16 length to size in bytes */
        pNextCacheEntry->nameLength <<= 1;
        if (pNextCacheEntry->nameLength <= 0) {
            status = IO_ERROR;
            break;
        }

        memcpy((char*)pNextCacheEntry + sizeof(IconCacheEntry) +
            pNextCacheEntry->nameLength, pData->pInfo[0].pImageData,
                pNextCacheEntry->imageDataLength);

        alignedOffset = sizeof(IconCacheEntry) +
            pNextCacheEntry->nameLength + pNextCacheEntry->imageDataLength;
        alignedOffset = SUITESTORE_ALIGN_4(alignedOffset);
        ADJUST_POS_IN_BUF(pos, bufferLen, alignedOffset);
    }

    if (status == ALL_OK) {
        /* write the buffer into the file */
        status = write_file(&pszError, &iconsCacheFile, buffer, pos);
        storageFreeError(pszError);
    }

    /* cleanup */
    pcsl_mem_free(buffer);
    pcsl_string_free(&iconsCacheFile);

    return status;
}

#undef ADJUST_POS_IN_BUF

/**
 * Frees the memory allocated for icons cache.
 */
void midp_free_suites_icons() {
    int i, n;

    if (g_iconsLoaded && g_numberOfEntries > 0 && g_pIconCache != NULL) {
        /* for each suite whose icons are in cache */
        for (i = 0; i < g_numberOfIcons; i++) {
            /* for each cached icon of this suite (currently 1) */
            for (n = 0; n < g_pIconCache[i].numberOfCachedImages; n++) {
                CachedImageInfo* pEntry = &g_pIconCache[i].pInfo[n];

                if (pEntry->isFree) {
                    continue;
                }

                if (pEntry->pImageData != NULL) {
                    pcsl_mem_free(pEntry->pImageData);
                }

                pcsl_string_free(&pEntry->imageName);
            }
        }

        pcsl_mem_free(g_pIconCache);
    }

    g_pIconCache      = NULL;
    g_iconsLoaded     = 0;
    g_numberOfIcons   = 0;
    g_numberOfEntries = 0;
}

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
                    unsigned char** ppImageData, int* pImageDataLen) {
    IconCache* pIconCache;
    int i;

    if (pIconName == NULL || ppImageData == NULL || pImageDataLen == NULL ||
            suiteId == UNUSED_SUITE_ID) {
        return BAD_PARAMS;
    }

    pIconCache = get_icon_cache_for_suite(suiteId);
    if (pIconCache == NULL) {
        return NOT_FOUND;
    }

    /* iterate through the icons cache */
    for (i = 0; i < pIconCache->numberOfCachedImages; i++) {
        if (pcsl_string_equals(pIconName, &(pIconCache->pInfo[i].imageName))) {
            *pImageDataLen = pIconCache->pInfo[i].imageDataLength;
            *ppImageData = pIconCache->pInfo[i].pImageData;
            return ALL_OK;
        }
    }

    /* icon not found */
    return NOT_FOUND;
}

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
                    unsigned char* pImageData, int imageDataLen) {
    MIDPError status = ALL_OK;
    pcsl_string_status res;

    if (pIconName == NULL || pImageData == NULL || imageDataLen == 0 ||
            suiteId == UNUSED_SUITE_ID) {
        return BAD_PARAMS;
    }

    do {
        IconCache* pIconCache = get_icon_cache_for_suite(suiteId);

        if (pIconCache == NULL) {
            /* try to find a free entry */
            int n;
            for (n = 0; n < g_numberOfIcons; n++) {
                if (g_pIconCache[n].pInfo[0].isFree) {
                    pIconCache = &g_pIconCache[n];
                    break;
                }
            }

            if (pIconCache == NULL) {
                /* there are no free entries in the cache */
                if (g_numberOfEntries <= g_numberOfIcons + 1) {
                    /* the cache is too small - add more entries */
                    int numOfEntries = g_numberOfIcons + RESERVED_CACHE_ENTRIES_NUM;
                    IconCache *pIconsData = (IconCache*) pcsl_mem_malloc(
                        sizeof(IconCache) * numOfEntries);
                    if (pIconsData == NULL) {
                        status = OUT_OF_MEMORY;
                        break;
                    }

                    if (g_numberOfEntries > 0) {
                        memcpy((char*)pIconsData, (char*)g_pIconCache,
                            g_numberOfEntries * sizeof(IconCache));
                        pcsl_mem_free(g_pIconCache);
                    }

                    g_pIconCache = pIconsData;
                }

                pIconCache = &g_pIconCache[g_numberOfIcons];
                g_numberOfIcons++;
            }
        } else {
            /* cache entry for this suite already exists, free it first */
            pcsl_string_free(&pIconCache->pInfo[0].imageName);
            pcsl_mem_free(pIconCache->pInfo[0].pImageData);
        }

        /* until the entry is filled, consider it as free */
        pIconCache->pInfo[0].isFree = 1;

        pIconCache->suiteId = suiteId;
        pIconCache->numberOfCachedImages = 1;

        res = pcsl_string_dup(pIconName, &pIconCache->pInfo[0].imageName);
        if (res != PCSL_STRING_OK) {
            status = OUT_OF_MEMORY;
            break;
        }
        pIconCache->pInfo[0].isFree = 0;
        pIconCache->pInfo[0].entryOffsetInFile = (unsigned long)-1;
        pIconCache->pInfo[0].imageDataLength = imageDataLen;
        pIconCache->pInfo[0].pImageData = pImageData;

        status = store_suites_icons(pIconCache);
    } while (0);

    return status;
}

/**
 * Removes all icons belonging to the given suite from the cache.
 *
 * @param suiteId ID of the suite whose icons should be removed
 *
 * @return status code (ALL_OK if successful)
 */
MIDPError midp_remove_suite_icons(SuiteIdType suiteId) {
    IconCache* pIconCache = get_icon_cache_for_suite(suiteId);

    if (pIconCache != NULL) {
        pcsl_string_free(&pIconCache->pInfo[0].imageName);
        pcsl_mem_free(pIconCache->pInfo[0].pImageData);
        pIconCache->pInfo[0].pImageData = NULL;
        pIconCache->pInfo[0].isFree = 1;
    }

    return store_suites_icons(pIconCache);
}

/**
 * Compacts the storage with the cached icons.
 *
 * IMPL_NOTE: currently does nothing because the whole file
 *            containing the cached icons is overwritten, so
 *            there are no free entries in it.
 *
 * @return status code (ALL_OK if successful)
 */
MIDPError midp_compact_icons() {
    return ALL_OK;
}
