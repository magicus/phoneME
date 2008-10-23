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

#include <kni.h>
#include "midp_file_cache.h"
#include <midpMalloc.h>
#include <midpStorage.h> /* IMPL_NOTE: use PCSL File API */
#include <midp_logging.h>
#include <midp_properties_port.h>
#include <string.h>
#include <stdio.h>

#define CHECK_ERROR(pszError) \
    if (pszError != (char*) NULL) { \
        REPORT_ERROR(LC_RMS, "File cache error"); \
        return; \
    }

#define DATA(b) (((char*)b)+sizeof(MidpFileCacheBlock))

#define UNINITIALIZED_CACHED_VALUE (-1)

/* Cache for a single file */
static MidpFileCache *mFileCache;

/* Cache limit for a single file */
static unsigned int fileCacheLimit = 0;

/**
 * Test if region 1 that starts from position x1 with size s1
 * overlaps with region 2 that starts from position x2 with
 * size s2.
 */
static int is_overlap(long x1, long s1, long x2, long s2) {
    if (x1 <= x2) {
        if (x2 < x1 + s1) {
            return 1;
        } else {
            return 0;
        }
    } else {
        if (x1 < x2 + s2) {
            return 1;
        } else {
            return 0;
        }
    }
}

/**
 * Test if region 1 that starts from position x1 with size s1
 * completely includes region 2 that starts from position x2 with
 * size s2.
 */
static int is_include(long x1, long s1, long x2, long s2) {
    return (x1 <= x2 && x1+s1 >= x2+s2);
}

/**
 * Initializes mFileCache->cachedAvailableSpace with the number
 * of available bytes on the storage defined by mFileCache->storageId.
 */
static void midp_init_cached_free_space() {
    if (mFileCache) {
        mFileCache->cachedAvailableSpace = storage_get_free_space(
            mFileCache->storageId);
    }
}

/* Upon success write, update file position, size and available space */
static void updateCachedSizes(long lengthWritten) {
    mFileCache->cachedPosition += lengthWritten;
    if (mFileCache->cachedPosition > mFileCache->cachedFileSize) {
        if (mFileCache->cachedAvailableSpace ==
                UNINITIALIZED_CACHED_VALUE) {
            midp_init_cached_free_space();
        }

        mFileCache->cachedAvailableSpace -= mFileCache->cachedPosition -
                                            mFileCache->cachedFileSize;
        mFileCache->cachedFileSize = mFileCache->cachedPosition;
    }
}

/* Directly write to storage. File position will be updated also. */
static
void uncachedWrite(char** ppszError, int handle, char *buffer, int length) {
    *ppszError = NULL;
    storagePosition(ppszError, handle, mFileCache->cachedPosition);
    if (*ppszError == NULL) {
        storageWrite(ppszError, handle, buffer, length);
        if (*ppszError == NULL) {
            updateCachedSizes(length);
        }
    }
}

/* Initialize file cache limit reading RMS_CACHE_LIMIT property,
 * using RMS_CACHE_LIMIT constant as default value.
 * File cache limit is initialized only once.
 */
static void initFileCacheLimit() {
    if (0 == fileCacheLimit) {
        int rmsCacheLimit = getInternalPropertyInt("RMS_CACHE_LIMIT");
        if (0 == rmsCacheLimit) {
            REPORT_INFO(LC_AMS, "RMS_CACHE_LIMIT property not set");
            /* set XML constant value as property value */
            rmsCacheLimit = RMS_CACHE_LIMIT;
        }
        fileCacheLimit = (unsigned)rmsCacheLimit;
    }
}

/**
 * Flush the cache and stop current file caching.
 * Seek cached position for the case the file won't be closed after
 * flushing (it's possible if a few files are openned simultaneously).
 */
static void midp_file_cache_finalize(char **ppszError, int stayOpen) {
    *ppszError = NULL;

    if (mFileCache != NULL) {
        midp_file_cache_flush(ppszError, mFileCache->handle);
        /* Do no seek for the file that is either damaged or to be closed */
        if (*ppszError == NULL && stayOpen) {
            storagePosition(ppszError, mFileCache->handle,
                mFileCache->cachedPosition);
        }
        /* If read is cached, free all read blocks here */
        midpFree(mFileCache);
        mFileCache = NULL;
    }
}

/** A helper function for midp_file_cache_flush(). */
static
void midp_file_cache_flush_using_buffer(char** ppszError, int handle,
                                        char* buf, long bufsize) {
    MidpFileCacheBlock *b; /* current cache block */
    MidpFileCacheBlock *n; /* next cache block */
    MidpFileCacheBlock *q; /* first block not (yet) copied to the buffer,
                              finally, the value to be stored
                              in mFileCache->blocks */
    char *bufPos;          /* first not yet used byte in the write buffer */
    long startPos, endPos; /* positions in the file cached in a series
                              of buffers, from...upto */
    long len;
    *ppszError = NULL;

    while ((b = mFileCache->blocks) != NULL) {

        if (  NULL != buf
           && NULL != (n = b->next)
           && n->position == b->position + b->length /* adjacent? */
           && b->length + n->length < bufsize
           ) {  /* begin merging blocks */

                /* position in the buffer */
                bufPos = buf;
                /* position in the file */
                startPos = endPos = b->position;

                b = mFileCache->blocks;
                /* now copy a series of adjacent blocks to the write buffer */
                do {
                    len = b->length;
                    memcpy(bufPos,DATA(b),len);
                    endPos += len;
                    bufPos += len;
                    b = b->next;
                } while (  NULL != b
                        && b->position == endPos
                        && endPos - startPos + b->length < bufsize
                        );

                q = b; /* q is next to the last copied block */
                /* the cache state has not been modified yet */

                /* now write from the write buffer */
                storagePosition(ppszError, handle, startPos);
                CHECK_ERROR(*ppszError);
                storageWrite(ppszError, handle, buf, endPos-startPos);
                CHECK_ERROR(*ppszError);

                /* write successful, now free the cache blocks */
                while ( q != (b = mFileCache->blocks)) {
                    mFileCache->size -= b->length + sizeof(MidpFileCacheBlock);
                    mFileCache->blocks = b->next;
                        midpFree(b);
                }
        } else {
            storagePosition(ppszError, handle, b->position);
            CHECK_ERROR(*ppszError);
            storageWrite(ppszError, handle, DATA(b), b->length);
            CHECK_ERROR(*ppszError);

            mFileCache->size -= b->length + sizeof(MidpFileCacheBlock);
            mFileCache->blocks = b->next;
            midpFree(b);
        }
    }
    storageCommitWrite(ppszError, handle);
    CHECK_ERROR(*ppszError);

    /* ASSERT (mFileCache->size == 0) */
    if (mFileCache->size != 0) {
        REPORT_ERROR(LC_RMS, "File cache out of sync");
    }
}

void midp_file_cache_flush(char** ppszError, int handle) {
    char *buf;     /* write buffer */
    long bufsize;  /* its size */
    *ppszError = NULL;

    if (mFileCache == NULL || mFileCache->handle != handle
        || mFileCache->blocks == NULL) {
        return;
    }

    /* allocate a buffer, as large as possible, but no larger than the cache */
    /* the buffer will be freed before the function returns */
    bufsize = mFileCache->size;
    do {
         buf = (char*)midpMalloc(bufsize);

         if (buf != NULL) {
            midp_file_cache_flush_using_buffer(ppszError, handle, buf, bufsize);
            midpFree(buf);
            break;
         } else if (bufsize > (signed) (4*sizeof(MidpFileCacheBlock))) {
            /* we prefer to have at least some buffer so that failure to
               allocate a big buffer does not lead to slowing down write
               operations by multiple times (e.g. 5x to 12x on non-cached
               compact flash) */
            bufsize >>= 1;
         } else {
            /* failed to allocate buffer of any size */
            midp_file_cache_flush_using_buffer(ppszError, handle, NULL, 0);
            break;
         }
    } while(1);
}

int midp_file_cache_open(char** ppszError, StorageIdType storageId,
                         const pcsl_string* filename, int ioMode) {
    int h;
    *ppszError = NULL;
    h = storage_open(ppszError, filename, ioMode);

    if (*ppszError == NULL) { /* Open successfully */
        if (mFileCache == NULL) {
            initFileCacheLimit();
            mFileCache = (MidpFileCache *)midpMalloc(sizeof(MidpFileCache));
            mFileCache->handle = h;
            mFileCache->size = 0;
            mFileCache->storageId = storageId;
            mFileCache->cachedPosition = 0;
            mFileCache->cachedAvailableSpace = UNINITIALIZED_CACHED_VALUE;
            mFileCache->cachedFileSize = storageSizeOf(ppszError, h);
            mFileCache->blocks = NULL;
        } else {
            /* More than one file is open. Available space can no longer been
             * cached. Stop caching completely. */
            midp_file_cache_finalize(ppszError, KNI_TRUE);
        }
    }

    return h;
}

void midp_file_cache_close(char** ppszError, int handle) {
    char *pszErrorTmp = NULL;
    *ppszError = NULL;

    if (mFileCache != NULL && mFileCache->handle == handle) {
        midp_file_cache_finalize(ppszError, KNI_FALSE);
        pszErrorTmp = *ppszError;
    }

    storageClose(ppszError, handle);

    if (*ppszError == NULL) {
        *ppszError = pszErrorTmp;
    } else {
        storageFreeError(pszErrorTmp);
    }
}

void midp_file_cache_seek(char** ppszError, int handle, long position) {
    *ppszError = NULL;

    if (position >= 0 && mFileCache != NULL && mFileCache->handle == handle) {
        mFileCache->cachedPosition = position;
    } else {
        storagePosition(ppszError, handle, position);
    }
}

void midp_file_cache_write(char** ppszError, int handle,
                           char* buffer, long length) {

    MidpFileCacheBlock *p, *b;
    *ppszError = NULL;

    if (length <= 0) {
        return;
    }

    if (mFileCache == NULL || mFileCache->handle != handle) {
        storageWrite(ppszError, handle, buffer, length);
        return;
    }

    /* Try to cache it */
    p = NULL; /* the block previous to b */
    b = mFileCache->blocks;

    while (b != NULL) {
        if (is_overlap(b->position, b->length,
                       mFileCache->cachedPosition, length)) {
            /* Handle simple overriding case */
            if (is_include(b->position, b->length,
                           mFileCache->cachedPosition, length)) {
                memcpy(DATA(b) + mFileCache->cachedPosition - b->position,
                        buffer, length);
                updateCachedSizes(length);
                return;
            } else {
                /* Flush everything out */
                midp_file_cache_flush(ppszError, handle);
                /* Reset previous block pointer after flush */
                p = NULL;
                /* Try to cache this write below */
                break;
            }
        } else if (mFileCache->cachedPosition+length <= b->position) {
            /* No match. Try to cache this write below */
            break;
        } else {
            /* Continue to scan the next block */
            p = b;
            b = b->next; /* that is, p->next == b */
        }
    } /* end of while (b) */
    /* the current value of b will not be used below */

    /* This is a new write block that has not been cached */

    /* Never try to cache large write that is bigger than cache limit */
    if (sizeof(MidpFileCacheBlock)+length > fileCacheLimit) {
        uncachedWrite(ppszError, handle, buffer, length);
        return;
    }

    /* If cache is full, flush it before caching new write */
    if (mFileCache->size+sizeof(MidpFileCacheBlock)+length > fileCacheLimit) {
        midp_file_cache_flush(ppszError, handle);
        /* Reset previous block pointer after flush */
        p = NULL;
    }

    /* Cache is not full, check if memory is full */
    b = (MidpFileCacheBlock *)midpMalloc(sizeof(MidpFileCacheBlock)+length);
    if (b == NULL) {
        /* Out of memory. Write directly to storage */
        uncachedWrite(ppszError, handle, buffer, length);
        return;
    }

    /* Insert a new cache block (b) between p and p->next */
    b->position = mFileCache->cachedPosition;
    b->length = length;
    memcpy(DATA(b), buffer, length);

    if (p == NULL) {
        /* inserting in the beginning of the list */
        b->next = mFileCache->blocks;
        mFileCache->blocks = b;
    } else {
        /* inserting after p, in the middle of the list */
        b->next = p->next;
        p->next = b;
    }

    mFileCache->size += sizeof(MidpFileCacheBlock)+length;
    updateCachedSizes(length);
}

long midp_file_cache_read(char** ppszError, int handle,
                          char* buffer, long length) {
    MidpFileCacheBlock *b;
    long l;

    *ppszError = NULL;

    if (length <= 0) {
        return 0;
    }

    if (mFileCache == NULL || mFileCache->handle != handle) {
        return storageRead(ppszError, handle, buffer, length);
    }

    /* See if it is in the cache */
    b = mFileCache->blocks;

    while (b != NULL) {
        if (is_overlap(b->position, b->length,
                        mFileCache->cachedPosition, length)) {
            /* Handle simple inclusive case */
            if (is_include(b->position, b->length,
                           mFileCache->cachedPosition, length)) {
                /* Read from cache */
                memcpy(buffer,
                        DATA(b) + mFileCache->cachedPosition - b->position,
                        length);
                mFileCache->cachedPosition += length;
                return length;

            } else {
                /* Flush everything out */
                midp_file_cache_flush(ppszError, handle);
                /* Read from file below */
                break;
            }
        } else if (mFileCache->cachedPosition+length <= b->position) {
            /* No match. Read from file below */
            break;
        } else {
            /* Continue to scan the next block */
            b = b->next;
        }
    } /* end of while (b) */

    /* Read from file */
    storagePosition(ppszError, handle, mFileCache->cachedPosition);
    if (*ppszError == NULL) {
        l = storageRead(ppszError, handle, buffer, length);
        if (*ppszError == NULL) {
            mFileCache->cachedPosition += length;
        }
    } else {
        l = 0;
    }

    return l;
}

jlong midp_file_cache_available_space(char** ppszError, int handle,
                                      StorageIdType storageId) {
    /* Storage may have more then 2Gb space available so use 64-bit type */
    jlong availSpace;
    *ppszError = NULL;

    if (mFileCache == NULL) {
        availSpace = storage_get_free_space(storageId);
    } else {
        if (mFileCache->handle != handle) {
            /* Flush current cache to storage before query for new space */
            midp_file_cache_flush(ppszError, mFileCache->handle);
            mFileCache->storageId = storageId;
            mFileCache->cachedAvailableSpace = UNINITIALIZED_CACHED_VALUE;
        }

        if (mFileCache->cachedAvailableSpace == UNINITIALIZED_CACHED_VALUE) {
            midp_init_cached_free_space();
        }

        availSpace = mFileCache->cachedAvailableSpace;
    }

    return availSpace;
}

long midp_file_cache_sizeof(char** ppszError, int handle) {
    *ppszError = NULL;

    if (mFileCache == NULL || mFileCache->handle != handle) {
        return storageSizeOf(ppszError, handle);
    } else {
        return mFileCache->cachedFileSize;
    }
}

void midp_file_cache_truncate(char** ppszError, int handle, long size) {
    *ppszError = NULL;

    midp_file_cache_flush(ppszError, handle);
    CHECK_ERROR(*ppszError);

    storageTruncate(ppszError, handle, size);

    if (*ppszError == NULL && mFileCache != NULL) {
        if (mFileCache->handle != handle) {
            mFileCache->cachedAvailableSpace = UNINITIALIZED_CACHED_VALUE;
        } else {
            if (mFileCache->cachedAvailableSpace !=
                    UNINITIALIZED_CACHED_VALUE) {
                mFileCache->cachedAvailableSpace +=
                    mFileCache->cachedFileSize - size;
                mFileCache->cachedFileSize = size;
            }
        }
    }
}
