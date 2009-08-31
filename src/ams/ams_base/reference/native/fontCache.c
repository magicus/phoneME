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

#include <string.h>

#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midpServices.h>
#include <midpStorage.h>
#include <midpJar.h>
#include <suitestore_task_manager.h>
#include <midp_constants_data.h>
#include <midpUtilKni.h>
#include <fileCache.h>

/**
 * @file
 *
 * Implements a cache for fonts.
 * <p>
 * All fonts are loaded from the Jar file and stored as files. 
 * <p>
 * Each cached font is stored with the following naming conventions:
 * <blockquote>
 *   <suite Id><font resource name>".tmp"
 * </blockquote>
 * All cache files are deleted when a suite is updated or removed.
 * <p>
 * Note: Currently, true type fonts (ttf) are supported.
 */

/**
 * Holds the suite ID. It is initialized during createFontCache() call
 * and used in font_cache_action() to avoid passing an additional parameter to it
 * because this function is called for each font entry in the suite's jar file.
 */
static SuiteIdType globalSuiteId;

/**
 * Holds the storage ID where the cached fonts will be saved. It is initialized
 * during createFontCache() call and used in font_cache_action() to avoid
 * passing an additional parameter to it.
 */
static StorageIdType globalStorageId;

/**
 * Handle to the opened jar file with the midlet suite. It is used to
 * passing an additional parameter to image_cache_action().
 */
static void *handle;

/**
 * Holds the amount of free space in the storage. It is initialized during
 * createFontCache() call and used in font_cache_action() to avoid passing
 * an additional parameter to it.
 */
static jlong remainingSpace;

/**
 * Holds the amount of cached data placed into the storage. It is initialized
 * during createFontCache() call and used in fonr_cache_action() to avoid
 * passing an additional parameter to it.
 */
static long cachedDataSize;

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(TTF_EXT1)
    {'.', 't', 't', 'f', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(TTF_EXT1);

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(TTF_EXT2)
    {'.', 'T', 'T', 'F', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(TTF_EXT2);


/**
 * Tests if JAR entry is a TTF font, by name extension
 */
static jboolean font_filter(const pcsl_string * entry) {

    if (pcsl_string_ends_with(entry, &TTF_EXT1)
        || pcsl_string_ends_with(entry, &TTF_EXT2)
        ) {
        return KNI_TRUE;
    }

    return KNI_FALSE;
}

/**
 * Loads TTF fonts from JAR, writes to storage
 */
static jboolean font_cache_action(const pcsl_string * entry) {
    unsigned char *fontBufPtr = NULL;
    int fontBufLen = 0;
    jboolean status = KNI_FALSE;

    do {
        fontBufLen = midpGetJarEntry(handle, entry, &fontBufPtr);
        if (fontBufLen < 0) {
            break;
        }

        /* write native buffer to persistent store */
        /* status = KNI_TRUE on success */
        status = storeFileToCache(globalSuiteId, globalStorageId, entry,
                                   fontBufPtr, fontBufLen);

    } while (0);

    if (status != KNI_FALSE) {
        remainingSpace -= fontBufLen;
        cachedDataSize += fontBufLen;
    }

    if (fontBufPtr != NULL) {
        midpFree(fontBufPtr);
    }

    return status;
}

/**
 * Iterates over all files in a jar based on filter function, and tries to load 
 * and cached them.
 *
 * @param  jarFileName   The name of the jar file
 * @param  filter        Pointer to filter function
 * @param  action        Pointer to action function (returns KNI_FALSE to stop iteration)
 * @return               1 if all was successful, <= 0 if some error
 */
static int loadAndCacheJarFileEntries(const pcsl_string * jarFileName,
                                      jboolean (*filter)(const pcsl_string *),
                                      jboolean (*action)(const pcsl_string *)) {

    int error = 0;
    int status = 0;

    handle = midpOpenJar(&error, jarFileName);
    if (error) {
        REPORT_WARN1(LC_LOWUI,
                     "Warning: could not open font cached; Error: %d\n",
                     error);
        handle = NULL;
        return status;
    }
    status = midpIterateJarEntries(handle, filter, action);

    midpCloseJar(handle);
    handle = NULL;

    return status;
}

/**
 * Creates a cache of fonts by iterating over all TTF files
 * in the jar file, loading each one and caching it
 * persistent store.
 *
 * @param suiteId The suite ID
 * @param storageId ID of the storage where to create the cache
 * @param pOutDataSize [out] points to a place where the size of the
 *                           written data is saved; can be NULL
 */
void createFontCache(SuiteIdType suiteId, StorageIdType storageId,
                      jint* pOutDataSize) {
    pcsl_string jarFileName;
    int result;
    jint errorCode;

    if (suiteId == UNUSED_SUITE_ID) {
        return;
    }

    /*
     * This makes the code non-reentrant and unsafe for threads,
     * but that is ok
     */
    globalSuiteId   = suiteId;
    globalStorageId = storageId;

    cachedDataSize = 0;

#if ENABLE_IMAGE_CACHE == 0
    /*
     * First, blow away any existing cache. Note: when a suite is
     * removed, midp_remove_suite() removes all files associated with
     * a suite, including the cache, so we don't have to do it
     * explicitly.
     */
    deleteFileCache(suiteId, storageId);
#endif

    /* Get the amount of space available at this point */
    //remainingSpace = storage_get_free_space(storageId);

    errorCode = midp_suite_get_class_path(suiteId, storageId,
                                          KNI_TRUE, &jarFileName);

    if (errorCode != MIDP_ERROR_NONE) {
        return;
    }

    result = loadAndCacheJarFileEntries(&jarFileName,
        (jboolean (*)(const pcsl_string *))&font_filter,
        (jboolean (*)(const pcsl_string *))&font_cache_action);

    /* If something went wrong then clean up anything that was created */
    if (result != 1) {
        REPORT_WARN1(LC_LOWUI,
            "Warning: font cache could not be created; Error: %d\n",
            result);
        deleteFileCache(suiteId, storageId);
        if (pOutDataSize != NULL) {
            *pOutDataSize = 0;
        }
    } else {
        if (pOutDataSize != NULL) {
            *pOutDataSize = (jint)cachedDataSize;
        }
    }

    pcsl_string_free(&jarFileName);
}

/**
 * Moves cached fonts from ome storage to another.
 * For security reasons we allow to move cache only to the
 * internal storage.
 *
 * @param suiteId The suite ID
 * @param storageIdFrom ID of the storage where fonts are cached
 * @param storageIdTo ID of the storage where to move the cache
 */
void moveFontCache(SuiteIdType suiteId, StorageIdType storageIdFrom,
                    StorageIdType storageIdTo) {
    moveFileCache(suiteId, storageIdFrom, storageIdTo);
}

