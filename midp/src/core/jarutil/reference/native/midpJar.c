/*
 *   
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

#include <stdio.h>
#include <string.h>

#include <jar.h>
#include <midpMalloc.h>
#include <midpStorage.h>
#include <midpJar.h>
#include <pcsl_string.h>

typedef struct _MidpJarInfo {
    FileObj fileObj;
    HeapManObj heapManObj;
    int status;
    JarInfo jarInfo;
} MidpJarInfo;

static long
sizeOfFile(void* state) {
    long size;
    char* pszError;

    size = storageSizeOf(&pszError,(int)state);
    storageFreeError(pszError);
    return size;
}

static long
readChars(void* state, unsigned char* buffer, long n) {
    long size;
    char* pszError;
 
    size = storageRead(&pszError, (int)state, (char*)buffer, n);
    storageFreeError(pszError);
    return size;
}

static int
seekChars(void* state, long offset, int whence) {
    long absPos;
    char* pszError;

    switch (whence) {
    case SEEK_SET:
        absPos = offset;
        storagePosition(&pszError, (int)state, absPos);
        break;

    case SEEK_END:
        absPos = sizeOfFile(state) + offset;
        storagePosition(&pszError, (int)state, absPos);
        break;

    case SEEK_CUR:
        absPos = storageRelativePosition(&pszError, (int)state, offset);
        break;

    default:
        return -1;
    }

    storageFreeError(pszError);
    return absPos;
}

static int
readChar(void* state) {
    long size;
    unsigned char temp;

    size = readChars(state, &temp, 1);
    if (size < 1) {
        return size;
    }

    return temp;
}

static void*
allocFunction(void* state, int n) {
    (void)state; /* No-op */
    /* alloc function just returns the address. */
    return midpMalloc(n);
}

static void
freeFunction(void* state, void* handle) {
    (void)state; /* No-op */
    /* alloc function just returns the address. */
    midpFree(handle);
}

static void*
addrFromHandleFunction(void* state, void* handle) {
    (void)state; /* No-op */
    /* This is not a heap that compacts memory, so just return the address. */
    return handle;
}

void*
midpOpenJar(int* pError, const pcsl_string * name) {
    MidpJarInfo* pJarInfo;
    char* pszError;

    *pError = 0;

    pJarInfo = (MidpJarInfo*)midpMalloc(sizeof (MidpJarInfo));
    if (pJarInfo == NULL) {
        *pError = MIDP_JAR_OUT_OF_MEM_ERROR;
        return NULL;
    }

    memset(pJarInfo, 0, sizeof (MidpJarInfo));

    pJarInfo->fileObj.state = (void*)storage_open(&pszError, name, OPEN_READ);
    if (pszError != NULL) {
        midpFree(pJarInfo);
        *pError = MIDP_JAR_IO_ERROR;
        return NULL;
    }

    pJarInfo->fileObj.size = sizeOfFile;
    pJarInfo->fileObj.read = readChars;
    pJarInfo->fileObj.seek = seekChars;
    pJarInfo->fileObj.readChar = readChar;

    pJarInfo->heapManObj.alloc = allocFunction;
    pJarInfo->heapManObj.free = freeFunction;
    pJarInfo->heapManObj.addrFromHandle = addrFromHandleFunction;

    pJarInfo->jarInfo = getJarInfo(&pJarInfo->fileObj);
    if (pJarInfo->jarInfo.status != 0) {
        storageClose(&pszError, (int)(pJarInfo->fileObj.state));
        storageFreeError(pszError);
        
        midpFree(pJarInfo);
        *pError = MIDP_JAR_CORRUPT_ERROR;
        return NULL;
    }

    return pJarInfo;
}

void
midpCloseJar(void* handle) {
    MidpJarInfo* pJarInfo = (MidpJarInfo*)handle;
    char* pszError;

    if (handle == NULL) {
        return;
    }

    storageClose(&pszError, (int)(pJarInfo->fileObj.state));
    storageFreeError(pszError);

    midpFree(pJarInfo);
}

/* If the jar size is less than zero it is an error code. */
long
midpGetJarSize(void* handle) {
    MidpJarInfo* pJarInfo = (MidpJarInfo*)handle;
    long size;

    size = sizeOfFile(pJarInfo->fileObj.state);
    if (size < 0) {
        return MIDP_JAR_IO_ERROR;
    }

    return size;
}

/* Returns the entry size or less than zero if error.
 *  If the entry does not exist, size is 0 and *ppEntry is NULL
 */
long
midpGetJarEntry(void* handle, const pcsl_string * name,
                unsigned char** ppEntry) {
    MidpJarInfo* pJarInfo = (MidpJarInfo*)handle;
    JarEntryInfo entryInfo;
    char*  entryData = NULL;
    int status;
    unsigned char* pName;
    int nameLen;
    unsigned char* pCompBuffer;

    *ppEntry = NULL;

    /* Jar entry names are UTF-8 */
    pName = (unsigned char *)pcsl_string_get_utf8_data(name);
    if (pName == NULL) {
        return MIDP_JAR_OUT_OF_MEM_ERROR;
    }

    nameLen = pcsl_string_utf8_length(name);
    pCompBuffer = midpMalloc(nameLen);
    if (pCompBuffer == NULL) {
        pcsl_string_release_utf8_data((jbyte*)pName, name);
        return MIDP_JAR_OUT_OF_MEM_ERROR;
    }
    
    entryInfo = findJarEntryInfo(&pJarInfo->fileObj, &pJarInfo->jarInfo,
                pName, nameLen, pCompBuffer);
    pcsl_string_release_utf8_data((jbyte*)pName, name);
    midpFree(pCompBuffer);
    if (entryInfo.status == JAR_ENTRY_NOT_FOUND) {
        return 0;
    }
    
    if (entryInfo.status != 0) {
        return MIDP_JAR_CORRUPT_ERROR;
    }

    entryData = midpMalloc((size_t)entryInfo.decompLen);
    if (entryData == NULL) {
        return MIDP_JAR_OUT_OF_MEM_ERROR;
    }

    status = inflateJarEntry(&pJarInfo->fileObj, &pJarInfo->heapManObj,
                             &entryInfo, (unsigned char*)entryData, 0);
    if (status != 0) {
        midpFree(entryData);
        return MIDP_JAR_CORRUPT_ERROR;
    }

    *ppEntry = (unsigned char*)entryData;
    return entryInfo.decompLen;
}

int
midpJarEntryExists(void* handle, const pcsl_string * name) {
    MidpJarInfo* pJarInfo = (MidpJarInfo*)handle;
    JarEntryInfo entryInfo;
    unsigned char* pName;
    int nameLen;
    unsigned char* pCompBuffer;

    /* Jar entry names are UTF-8 */
    pName = (unsigned char *)pcsl_string_get_utf8_data(name);
    if (NULL == pName) {
        return MIDP_JAR_OUT_OF_MEM_ERROR;
    }

    nameLen = pcsl_string_utf8_length(name);
    pCompBuffer = midpMalloc(nameLen);
    if (NULL == pCompBuffer) {
        pcsl_string_release_utf8_data((jbyte*)pName, name);
        return MIDP_JAR_OUT_OF_MEM_ERROR;
    }
    
    entryInfo = findJarEntryInfo(&pJarInfo->fileObj, &pJarInfo->jarInfo,
                pName, nameLen, pCompBuffer);
    pcsl_string_release_utf8_data((jbyte*)pName, name);
    midpFree(pCompBuffer);
    if (JAR_ENTRY_NOT_FOUND == entryInfo.status) {
        return 0;
    }

    if (entryInfo.status != 0) {
        return MIDP_JAR_CORRUPT_ERROR;
    }

    return 1;
}

int 
midpIterateJarEntries(void *handle, filterFuncT *filter, actionFuncT *action) {

    MidpJarInfo *pJarInfo = (MidpJarInfo*)handle;
    JarEntryInfo entryInfo;
    pcsl_string entryName;
    unsigned char *nameBuf;
    int status = 1;
    pcsl_string_status res;

    entryInfo = getFirstJarEntryInfo(&pJarInfo->fileObj, &pJarInfo->jarInfo);

    while (entryInfo.status == 0) {
        
        nameBuf =  (unsigned char*) midpMalloc(entryInfo.nameLen);
        if (nameBuf == NULL) {
            status = MIDP_JAR_OUT_OF_MEM_ERROR;
            break;
        }
        
        entryInfo.status = getJarEntryName(&pJarInfo->fileObj, &entryInfo, nameBuf);
        if (entryInfo.status != 0) {
            status = MIDP_JAR_CORRUPT_ERROR;
            midpFree(nameBuf);
            break;
        }

        res = pcsl_string_convert_from_utf8((jbyte*)nameBuf, entryInfo.nameLen,
                                            &entryName);
        midpFree(nameBuf);
        if (PCSL_STRING_OK != res) {
            status = MIDP_JAR_OUT_OF_MEM_ERROR;
            break;
        }
       
        if ((*filter)(&entryName)) {
            /* name match: call action */   
            if (!(*action)(&entryName)) {
                /*
                 * IMPL_NOTE: continue iteration over remaining entries even
                 * action for some entry fails.
                 */
            }
        }
        
        pcsl_string_free(&entryName);
        
        entryInfo = getNextJarEntryInfo(&pJarInfo->fileObj, &pJarInfo->jarInfo, 
                                        &entryInfo);
        
    }
       
    return status;
}
