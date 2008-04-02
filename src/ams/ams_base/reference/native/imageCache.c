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

#include <string.h>

#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midpServices.h>
#include <midpStorage.h>
#include <midpJar.h>
#include <suitestore_task_manager.h>
#include <midp_constants_data.h>
#include <img_image.h>
#include <midpUtilKni.h>

/**
 * @file
 *
 * Implements a cache for native images.
 * <p>
 * All images are loaded from the Jar file, converted to the native platform
 * representation, and stored as files. When an ImmutableImage is created then
 * loadCachedImage() is called to check if that particular image has been
 * cached, and if yes the native representation is loaded from the file. This
 * significantly reduce the time spent instantiating an ImmutableImage.
 * <p>
 * Each cached image is stored with the following naming conventions:
 * <blockquote>
 *   <suite Id><image resource name>".tmp"
 * </blockquote>
 * All cache files are deleted when a suite is updated or removed.
 * <p>
 * This implementation contains a simple method for preventing cached images
 * to grab too much storage space: If the remaining space is below
 * IMAGE_CACHE_THRESHOLD or the last cached image couldn't be saved due
 * to lack of space, then the creating of the cache for this suite is
 * aborted and all already created files are deleted. This is not an
 * ideal approach, but should be sufficient for now.
 * <p>
 * Note: Currently, only images ending with ".png" or ".PNG" are supported.
 */

/**
 * Holds the suite ID. It is initialized during createImageCache() call
 * and used in png_cache_action() to avoid passing an additional parameter to it
 * because this function is called for each image entry in the suite's jar file.
 */
static SuiteIdType globalSuiteId;

/**
 * Holds the storage ID where the cached images will be saved. It is initialized
 * during createImageCache() call and used in png_cache_action() to avoid passing
 * an additional parameter to it.
 */
static StorageIdType globalStorageId;

/**
 * Handle to the opened jar file with the midlet suite. It is used to
 * passing an additional parameter to png_cache_action().
 */
static void *handle;

/**
 * Holds the amount of free space in the storage. It is initialized during
 * createImageCache() call and used in png_cache_action() to avoid passing
 * an additional parameter to it.
 */
static long remainingSpace;

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(PNG_EXT1)
    {'.', 'p', 'n', 'g', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(PNG_EXT1);

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(PNG_EXT2)
    {'.', 'P', 'N', 'G', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(PNG_EXT2);

/* Forward declaration */
static int storeImageToCache(SuiteIdType suiteId,
                             StorageIdType storageId,
                             const pcsl_string * resName,
                             unsigned char *bufPtr, int len);


/**
 * Tests if JAR entry is a PNG image, by name extension
 */
static jboolean png_filter(const pcsl_string * entry) {

    if (pcsl_string_ends_with(entry, &PNG_EXT1) ||
        pcsl_string_ends_with(entry, &PNG_EXT2)) {
        return KNI_TRUE;
    }

    return KNI_FALSE;
}

/**
 * Loads PNG image from JAR, decodes it and writes as native
 */
static jboolean png_cache_action(const pcsl_string * entry) {
    unsigned char *pngBufPtr = NULL;
    unsigned int pngBufLen = 0;
    unsigned char *nativeBufPtr = NULL;
    unsigned int nativeBufLen = 0;
    jboolean status = KNI_FALSE;

    do {
        pngBufLen = midpGetJarEntry(handle, entry, &pngBufPtr);

        if (img_decode_data2cache(pngBufPtr, pngBufLen,
                &nativeBufPtr, &nativeBufLen) != MIDP_ERROR_NONE) {
            break;
        }

        /* Check if we can store this file in the remaining storage space */
        if (remainingSpace - IMAGE_CACHE_THRESHOLD < (long)nativeBufLen) {
            break;
        }

        /* write native buffer to persistent store */
        status = storeImageToCache(globalSuiteId, globalStorageId, entry,
                                   nativeBufPtr, nativeBufLen);

    } while (0);

    if (status == 1) {
        remainingSpace -= nativeBufLen;
    }

    if (nativeBufPtr != NULL) {
        midpFree(nativeBufPtr);
    }
    if (pngBufPtr != NULL) {
        midpFree(pngBufPtr);
    }

    return status;
}

/**
 * Iterates over all images in a jar, and tries to load and cached them.
 *
 * @param  jarFileName   The name of the jar file
 * @param  filter        Pointer to filter function
 * @param  action        Pointer to action function
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
                     "Warning: could not open image cached; Error: %d\n",
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
 * Remove all the cached images associated with a suite.
 *
 * @param suiteId   Suite ID
 * @param storageId ID of the storage where to create the cache
*/
static void deleteImageCache(SuiteIdType suiteId, StorageIdType storageId) {
    pcsl_string root;
    pcsl_string filename;
    char*  pszError;
    void*  handle = NULL;
    jint errorCode;

    if (suiteId == UNUSED_SUITE_ID) {
        return;
    }

    errorCode =     midp_suite_get_cached_resource_filename(suiteId, storageId,
                                                        &PCSL_STRING_EMPTY,
                                                        &root);
    if (errorCode != MIDP_ERROR_NONE) {
        return;
    }

    handle = storage_open_file_iterator(&root);
    if (handle == NULL) {
        pcsl_string_free(&root);
        return;
    }

    /* Delete all files that start with suite Id and end with TMP_EXT */
    for (;;) {
        if (0 != storage_get_next_file_in_iterator(&root, handle, &filename)) {
            break;
        }
        if (pcsl_string_ends_with(&filename, &TMP_EXT)) {
            storage_delete_file(&pszError, &filename);
            if (pszError != NULL) {
                storageFreeError(pszError);
            }
        }

        pcsl_string_free(&filename);
    }

    storageCloseFileIterator(handle);
    pcsl_string_free(&root);
}

/**
 * Creates a cache of natives images by iterating over all png images in the jar
 * file, loading each one, decoding it into native, and caching it persistent
 * store.
 *
 * @param suiteId The suite ID
 * @param storageId ID of the storage where to create the cache
 */
void createImageCache(SuiteIdType suiteId, StorageIdType storageId) {

    pcsl_string jarFileName;
    int result;
    jint errorCode;

    if (suiteId == UNUSED_SUITE_ID) {
        return;
    }

    /*
     * This makes the code non-reentrant and unsafe for threads,
     *  but that is ok
     */
    globalSuiteId   = suiteId;
    globalStorageId = storageId;

    /*
     * First, blow away any existing cache. Note: when a suite is
     * removed, midp_remove_suite() removes all files associated with
     * a suite, including the cache, so we don't have to do it
     * explicitly.
     */
    deleteImageCache(suiteId, storageId);

    /* Get the amount of space available at this point */
    remainingSpace = storage_get_free_space(storageId);

    errorCode = midp_suite_get_class_path(suiteId, storageId,
                                          KNI_TRUE, &jarFileName);

    if (errorCode != MIDP_ERROR_NONE) {
        return;
    }

    result = loadAndCacheJarFileEntries(&jarFileName,
        (jboolean (*)(const pcsl_string *))&png_filter,
        (jboolean (*)(const pcsl_string *))&png_cache_action);

    /* If something went wrong then clean up anything that was created */
    if (result != 1) {
        REPORT_WARN1(LC_LOWUI,
            "Warning: image cache could not be created; Error: %d\n",
            result);
        deleteImageCache(suiteId, storageId);
    }

    pcsl_string_free(&jarFileName);

}

/**
 * Moves cached native images from ome storage to another.
 * For security reasons we allow to move cache only to the
 * internal storage.
 *
 * @param suiteId The suite ID
 * @param storageIdFrom ID of the storage where images are cached
 * @param storageIdTo ID of the storage where to move the cache
 */
void moveImageCache(SuiteIdType suiteId, StorageIdType storageIdFrom, StorageIdType storageIdTo) {
    pcsl_string root;
    pcsl_string filePath;
    char*  pszError;
    void*  handle = NULL;
    jint errorCode;
    jsize oldRootLength;
    jsize newRootLength;
    const pcsl_string* newRoot;


    if (suiteId == UNUSED_SUITE_ID) {
        return;
    }

    /*
     * IMPL_NOTE: for security reasons we allow to move cache
     * only to the internal storage.
     */
    if (storageIdTo != INTERNAL_STORAGE_ID) {
        return;
    }

    errorCode = midp_suite_get_cached_resource_filename(suiteId, storageIdFrom,
                                                        &PCSL_STRING_EMPTY,
                                                        &root);
    if (errorCode != MIDP_ERROR_NONE) {
        return;
    }

    handle = storage_open_file_iterator(&root);
    if (handle == NULL) {
        pcsl_string_free(&root);
        return;
    }

    newRoot = storage_get_root(storageIdTo);
    newRootLength = pcsl_string_length(newRoot);
    oldRootLength = pcsl_string_length(storage_get_root(storageIdFrom));

    /* Move all files that start with suite Id and end with TMP_EXT to new storage */
    for (;;) {
        pcsl_string fileName;
        pcsl_string newFilePath = PCSL_STRING_NULL;
        jsize filePathLength;
        
        if (0 != storage_get_next_file_in_iterator(&root, handle, &filePath)) {
            break;
        }
        if (pcsl_string_ends_with(&filePath, &TMP_EXT)) {
            /* construct new file name. */
            filePathLength = pcsl_string_length(&filePath);
            pcsl_string_predict_size(&fileName, filePathLength - oldRootLength);
            if (PCSL_STRING_OK != pcsl_string_substring(&filePath,
                    oldRootLength, filePathLength, &fileName)) {
                break;
            }
            pcsl_string_predict_size(&newFilePath, newRootLength + pcsl_string_length(&fileName));
            if (PCSL_STRING_OK != pcsl_string_append(&newFilePath, newRoot)) {
                pcsl_string_free(&fileName);
                break;
            }
            if (PCSL_STRING_OK != pcsl_string_append(&newFilePath,
                    (const pcsl_string*)&fileName)) {
                pcsl_string_free(&fileName);
                pcsl_string_free(&newFilePath);
                break;
            }
            /* rename file. */
            storage_rename_file(&pszError, &filePath, &newFilePath);
            pcsl_string_free(&fileName);
            pcsl_string_free(&newFilePath);
            
            if (pszError != NULL) {
                storageFreeError(pszError);
            }
        }

        pcsl_string_free(&filePath);
    }

    storageCloseFileIterator(handle);
    pcsl_string_free(&root);
}


/**
 * Store a native image to cache.
 *
 * @param suiteId    The suite id
 * @param storageId ID of the storage where to create the cache
 * @param resName    The image resource name
 * @param bufPtr     The buffer with the image data
 * @param len        The length of the buffer
 * @return           1 if successful, 0 if not
 */
static int storeImageToCache(SuiteIdType suiteId, StorageIdType storageId,
                             const pcsl_string * resName,
                             unsigned char *bufPtr, int len) {

    int            handle = -1;
    char*          errmsg = NULL;
    pcsl_string    path;
    int status     = 1;
    int errorCode;

    if (suiteId == UNUSED_SUITE_ID || pcsl_string_is_null(resName) ||
        (bufPtr == NULL)) {
        return 0;
    }

    /* Build the complete path */
    errorCode = midp_suite_get_cached_resource_filename(suiteId,
                                                        storageId,
                                                        resName,
                                                        &path);

    if (errorCode != MIDP_ERROR_NONE) {
        return 0;
    }

    /* Open the file */
    handle = storage_open(&errmsg, &path, OPEN_READ_WRITE_TRUNCATE);
    pcsl_string_free(&path);
    if (errmsg != NULL) {
        REPORT_WARN1(LC_LOWUI,"Warning: could not save cached image; %s\n",
		     errmsg);

	storageFreeError(errmsg);
	return 0;
    }

    /* Finally write the file */
    storageWrite(&errmsg, handle, (char*)bufPtr, len);
    if (errmsg != NULL) {
      status = 0;
      storageFreeError(errmsg);
    }

    storageClose(&errmsg, handle);

    return status;
}


/**
 * Loads a native image from cache, if present.
 *
 * @param suiteId    The suite id
 * @param resName    The image resource name
 * @param **bufPtr   Pointer where a buffer will be allocated and data stored
 * @return           -1 if failed, else length of buffer
 */
int loadImageFromCache(SuiteIdType suiteId, const pcsl_string * resName,
                       unsigned char **bufPtr) {

    int                len = -1;
    int                handle;
    char*              errmsg = NULL;
    int                bytesRead;
    pcsl_string        path;
    pcsl_string        resNameFix;
    MIDPError          errorCode;
    StorageIdType      storageId;
    pcsl_string_status res;

    if (suiteId == UNUSED_SUITE_ID || pcsl_string_is_null(resName)) {
        return len;
    }

    /* If resource starts with slash, remove it */
    if (pcsl_string_index_of(resName, (jint)'/') == 0) {
        jsize resNameLen = pcsl_string_length(resName);
        res = pcsl_string_substring(resName, 1, resNameLen, &resNameFix);
    } else {
        res = pcsl_string_dup(resName, &resNameFix);
    }
    if (PCSL_STRING_OK != res) {
        return len;
    }

    /*
     * IMPL_NOTE: here is assumed that the image cache is located in
     * the same storage as the midlet suite. This may not be true.
     */
    /* Build path */
    errorCode = midp_suite_get_suite_storage(suiteId, &storageId);
    if (errorCode != ALL_OK) {
        return len;
    }

    errorCode = midp_suite_get_cached_resource_filename(suiteId,
        storageId, &resNameFix, &path);
    pcsl_string_free(&resNameFix);
    if (errorCode != ALL_OK) {
        return len;
    }

    /* Open file */
    handle = storage_open(&errmsg, &path, OPEN_READ);
    pcsl_string_free(&path);
    if (errmsg != NULL) {
        REPORT_WARN1(LC_LOWUI,"Warning: could not load cached image; %s\n",
                     errmsg);

        storageFreeError(errmsg);
        return len;
    }

    do {
        /* Get size of file and allocate buffer */
        len = storageSizeOf(&errmsg, handle);
        *bufPtr = midpMalloc(len);
        if (*bufPtr == NULL) {
            len = -1;
            break;
        }

        /* Read data */
        bytesRead = storageRead(&errmsg, handle, (char*)*bufPtr, len);
        if (errmsg != NULL) {
            len = -1;
            midpFree(*bufPtr);
            storageFreeError(errmsg);
            break;
        }

        /* Sanity check */
        if (bytesRead != len) {
            len = -1;
            midpFree(*bufPtr);
            break;
        }

    } while (0);

    storageClose(&errmsg, handle);

    return len;
}
