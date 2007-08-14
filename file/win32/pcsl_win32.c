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

#include <windows.h>

#include <pcsl_file.h>
#include <pcsl_memory.h>

/* 
 * This constant is defined in "WinBase.h" when using VS7 (2003) and VS8 (2005),
 * but absent in Visual C++ 6 headers. 
 * For successful build with VC6 we need to define it manually.
 */

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

/* 
 * Modern Win32 supports both \ and / as file separator 
 * but / has some problems when used at command line.
 */
static const jchar FILESEP = '/';
static const jchar PATHSEP = ';';

typedef struct _PCSLFile {
    int createFlags;
    HANDLE fileHandle;
} PCSLFile;

typedef struct _PCSLFileIterator {
    int savedRootLength;
    HANDLE iteratorHandle;
} PCSLFileIterator;

static int findFirstMatch(PCSLFileIterator* pIterator,
                          const pcsl_string * match,
                          pcsl_string * result);
static int findNextMatch(PCSLFileIterator* pIterator,
                         const pcsl_string * match,
                         pcsl_string * result);

/**
 * The initialize function initials the File System
 */
int pcsl_file_init() {
    /* Verify assumptions on the types */
    if (sizeof(jchar) != sizeof(wchar_t)) {
      return -1;
    }
    if (sizeof(jchar) != sizeof(WCHAR)) {
      return -1;
    }

    return (pcsl_string_is_active() == PCSL_TRUE) ? 0 : -1;
}

/**
 * Cleans up resources used by file system. It is only needed for the 
 * Ram File System (RMFS).
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_finalize() {
    return 0;
}

/**
 * The open function creates and returns a new file identifier for the 
 * file named by filename. Initially, the file position indicator for the 
 * file is at the beginning of the file.
 */
int pcsl_file_open(const pcsl_string * fileName, int flags, void **handle) {
    HANDLE fileHandle;

    DWORD dwDesiredAccess; 
    DWORD dwCreationDisposition;

    const jchar* pszOsFilename = pcsl_string_get_utf16_data(fileName);

    *handle = NULL;

    if (NULL == pszOsFilename) {
        return -1;
    }

    switch (flags & (PCSL_FILE_O_RDWR | PCSL_FILE_O_WRONLY | PCSL_FILE_O_RDONLY)) {
        case PCSL_FILE_O_RDONLY: dwDesiredAccess = GENERIC_READ;  break;
        case PCSL_FILE_O_WRONLY: dwDesiredAccess = GENERIC_WRITE; break;
        default: /* PCSL_FILE_O_RDWR or other flag combination */
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE; break;
    }

    switch (flags & (PCSL_FILE_O_CREAT | PCSL_FILE_O_TRUNC)) {
        case PCSL_FILE_O_CREAT | PCSL_FILE_O_TRUNC: 
            dwCreationDisposition = CREATE_ALWAYS; break;
        case PCSL_FILE_O_CREAT: 
            dwCreationDisposition = OPEN_ALWAYS; break;
        case PCSL_FILE_O_TRUNC: 
            dwCreationDisposition = TRUNCATE_EXISTING; break;
        default:
            dwCreationDisposition = OPEN_EXISTING; break;
    }

    fileHandle = CreateFileW(pszOsFilename, dwDesiredAccess, 
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, dwCreationDisposition, 
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);

    pcsl_string_release_utf16_data(pszOsFilename, fileName);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    {
        PCSLFile* pFHandle = pcsl_mem_malloc(sizeof(PCSLFile));
        if (pFHandle == NULL) {
            CloseHandle(fileHandle);
            return -1;
        }

        pFHandle->createFlags = flags;
        pFHandle->fileHandle = fileHandle;
        *handle  = pFHandle;
        return 0;
    }
}

/**
 * The close function  loses the file with descriptor identifier in FS. 
 */
int pcsl_file_close(void *handle) 
{
    if (NULL == handle)
        return -1;

    {
        PCSLFile* pFH = (PCSLFile*)handle;
        pcsl_file_commitwrite(handle); /* commit pending writes */
        if (!CloseHandle(pFH->fileHandle))
            return -1;
        pcsl_mem_free(handle);
        return 0;
    }
}

/**
 * The read function reads up to size bytes from the file with descriptor identifier , 
 * storing the results in the buffer.
 */
int pcsl_file_read(void *handle, unsigned  char *buffer, long length)
{
    if (0 == length) {
        return 0;
    }
    if (NULL == handle)
        return -1;

    {
        DWORD bytesRead;
        PCSLFile* pFH = (PCSLFile*)handle;
        if (!ReadFile(pFH->fileHandle, buffer, (DWORD)length, &bytesRead, NULL))
            return -1;
        return bytesRead;

    }
}

/**
 * The write function writes up to size bytes from buffer to the file 
 * with descriptor  identifier. 
 * The return value is the number of bytes actually written. 
 * This is normally the same as size, but might be less (for example, 
 * if the persistent storage being written to fills up).
 */
int pcsl_file_write(void *handle, unsigned char* buffer, long length)
{
    DWORD bytesWritten;
    PCSLFile* pFH = (PCSLFile*)handle;
    if (NULL == handle)
        return -1;

    if (PCSL_FILE_O_APPEND == (pFH->createFlags && PCSL_FILE_O_APPEND)) {
        if (!pcsl_file_seek(handle, 0, PCSL_FILE_SEEK_END))
            return -1;
    }

    if (!WriteFile(pFH->fileHandle, buffer, (DWORD)length, &bytesWritten, NULL))
        return -1;
    return bytesWritten;
}

/**
 * The unlink function deletes the file named filename from the persistent storage.
 */
int pcsl_file_unlink(const pcsl_string * fileName)
{
    int status = -1;
    const jchar* pszOsFilename = pcsl_string_get_utf16_data(fileName);
    if (NULL != pszOsFilename) {
        status = DeleteFileW(pszOsFilename) ? 0 : -1;
        pcsl_string_release_utf16_data(pszOsFilename, fileName);
    }

    return status;
}

/**
 * The  truncate function is used to truncate the size of an open file in storage.
 */
int pcsl_file_truncate(void *handle, long size)
{
    if (-1 == pcsl_file_seek(handle, size, PCSL_FILE_SEEK_SET))
        return -1;

    {
        PCSLFile* pFH = (PCSLFile*)handle;
        return SetEndOfFile(pFH->fileHandle) ? 0 : -1;
    }
}

/**
 * The lseek function is used to change the file position of the file with descriptor 
 * identifier 
 */
long pcsl_file_seek(void *handle, long offset, long position)
{
    DWORD method;

    if (NULL == handle)
        return -1;

    switch (position) {
        case PCSL_FILE_SEEK_CUR: method = FILE_CURRENT; break;
        case PCSL_FILE_SEEK_END: method = FILE_END;     break;
        case PCSL_FILE_SEEK_SET:
        default:
            method = FILE_BEGIN;
            break;
    }

    {
        PCSLFile* pFH = (PCSLFile*)handle;
        DWORD res = SetFilePointer(pFH->fileHandle, offset, NULL, method);
        return (INVALID_SET_FILE_POINTER != res) ? res : -1;
    }
}

/**
 * FS only need to support MIDLets to query the size of the file. 
 * Check the File size by file handle
 */
long pcsl_file_sizeofopenfile(void *handle)
{
    DWORD sizeHigh;
    DWORD sizeLo;
    if (NULL == handle)
        return -1;

    {
        PCSLFile* pFH = (PCSLFile*)handle;
        sizeLo = GetFileSize(pFH->fileHandle, &sizeHigh);
        /* NOTE Returned only 32 lowest bit */
        return (INVALID_FILE_SIZE != sizeLo) ? sizeLo : -1;
    }
}

/**
 * FS only need to support MIDLets to query the size of the file. 
 * Check the File size by file name
 */
long pcsl_file_sizeof(const pcsl_string * fileName)
{
    WIN32_FILE_ATTRIBUTE_DATA attrib;
    int result = -1;
    const jchar* pOsFN = pcsl_string_get_utf16_data(fileName);

    if (NULL != pOsFN) {
        if (GetFileAttributesExW(pOsFN, GetFileExInfoStandard, &attrib)) {
            /* NOTE Returned only 32 lowest bit */
            result = attrib.nFileSizeLow;
        }
        pcsl_string_release_utf16_data(pOsFN, fileName);
    }

    return result;
}

/**
 * Check if the file exists in FS storage.
 */
int pcsl_file_exist(const pcsl_string * fileName)
{
    DWORD attrib;
    const jchar* pszOsFilename = pcsl_string_get_utf16_data(fileName);
    if (NULL != pszOsFilename) {
        attrib = GetFileAttributesW(pszOsFilename);
        pcsl_string_release_utf16_data(pszOsFilename, fileName);

        if (INVALID_FILE_ATTRIBUTES != attrib) 
            return (attrib & FILE_ATTRIBUTE_DIRECTORY) ? 0 : 1;
    }
    return -1;
}

/* Force the data to be written into the FS storage */
int pcsl_file_commitwrite(void *handle)
{
    if (NULL == handle)
        return -1;
    {
        PCSLFile* pFH = (PCSLFile*)handle;
        return FlushFileBuffers(pFH->fileHandle) ? 0 : -1;
    }
}

/**
 * The rename function updates the filename.
 */
int pcsl_file_rename(const pcsl_string * oldName, 
                     const pcsl_string * newName)
{
    int status = -1;
    const jchar * pszOldFilename = pcsl_string_get_utf16_data(oldName);
    const jchar * pszNewFilename = pcsl_string_get_utf16_data(newName);

    if ((NULL != pszOldFilename) && (NULL != pszNewFilename)) {
        status = MoveFileW(pszOldFilename, pszNewFilename) ? 0 : -1;
    }

    if (pszNewFilename)
        pcsl_string_release_utf16_data(pszNewFilename, newName);

    if (pszOldFilename)
        pcsl_string_release_utf16_data(pszOldFilename, oldName);

    return status;
}

/**
 * The opendir function opens directory named dirname. 
 */
void* pcsl_file_openfilelist(const pcsl_string * string)
{
    int rootLength;
    PCSLFileIterator* pIterator;

    pIterator = (PCSLFileIterator*)pcsl_mem_malloc(sizeof (PCSLFileIterator));
    if (pIterator == NULL) {
        /* Error in allocation */
        return NULL;
    }

    memset(pIterator, 0, sizeof(PCSLFileIterator));
    pIterator->iteratorHandle = INVALID_HANDLE_VALUE;

    /*
     * Find the root dir of the string
     */
    rootLength = pcsl_string_last_index_of(string, FILESEP);
    if (-1 == rootLength) {
        rootLength = 0;
    } else {
        /* Include the file separator. */
        rootLength++;
    }

    pIterator->savedRootLength = rootLength;

    /*
     * FindFirstFile open and get the first file, so do not
     * do any thing more until get next is called.
     */
    return pIterator;

}

/**
 * The mkdir function closes the directory named dirname. 
 */
int pcsl_file_closefilelist(void *handle)
{
    BOOL status = FALSE;
    PCSLFileIterator* pIterator = (PCSLFileIterator *)handle;

    if (NULL == handle) {
        return 0;
    }

    if (INVALID_HANDLE_VALUE != pIterator->iteratorHandle) {
        status = FindClose(pIterator->iteratorHandle);
    }

    pcsl_mem_free(pIterator);

    return status ? 0 : -1;
}

/**
 * The getFreeSpace function checks the size of free space in storage. 
 */
long pcsl_file_getfreespace()
{
    return 0;
}

/**
 * The getUsedSpace function checks the size of used space in storage. 
 */
long pcsl_file_getusedspace(const pcsl_string * systemDir)
{
    long used = 0;
    void* pIterator;
    pcsl_string current = PCSL_STRING_NULL;

    pIterator = pcsl_file_openfilelist(systemDir);
    for (; ; ) {
        if (pcsl_file_getnextentry(pIterator, systemDir, &current) == -1) {
            break;
        }

        {
            long size = pcsl_file_sizeof(&current);
            if (size >= 0)
                used += size;
        }

        pcsl_string_free(&current);
    }

    pcsl_file_closefilelist(pIterator);
    return used;
}

/* The getNextEntry function search the next file which is  specified DIR */
int pcsl_file_getnextentry(void *handle, const pcsl_string * string, 
                           pcsl_string * result)
{

    PCSLFileIterator* pIterator = (PCSLFileIterator *)handle;

    if (NULL == pIterator) {
        return -1;
    }

    if (INVALID_HANDLE_VALUE == pIterator->iteratorHandle) {
        return findFirstMatch(pIterator, string, result);
    }

    return findNextMatch(pIterator, string, result);

}

jchar
pcsl_file_getfileseparator() {
    return FILESEP;
}

jchar
pcsl_file_getpathseparator() {
    return PATHSEP;
}

static int findFirstMatch(PCSLFileIterator* pIterator,
                          const pcsl_string * match,
                          pcsl_string * result) 
{
    WIN32_FIND_DATAW findData;
    HANDLE handle;
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(starSuffix) 
        {'*', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(starSuffix);
    pcsl_string root = PCSL_STRING_NULL;
    pcsl_string foundName = PCSL_STRING_NULL;
    pcsl_string matchStar = PCSL_STRING_NULL;
    jsize rootLen = 0;

    if (NULL == result) {
        return -1;
    }

    * result = PCSL_STRING_NULL;

    if (PCSL_STRING_OK != pcsl_string_cat(match, &starSuffix, &matchStar)) {
        return -1;
    }

    {
        const jchar * pwszMatch = pcsl_string_get_utf16_data(&matchStar);

        if (NULL == pwszMatch) {
            pcsl_string_free(&matchStar);
            return -1;
        }

        handle = FindFirstFileW(pwszMatch, &findData);

        pcsl_string_free(&matchStar);
    }

    if (INVALID_HANDLE_VALUE == handle) {
        return -1;
    }

    pIterator->iteratorHandle = handle;
    rootLen = pIterator->savedRootLength;

    if (PCSL_STRING_OK != pcsl_string_substring(match, 0, rootLen, &root)) {
        return -1;
    }

    if (pcsl_string_convert_from_utf16(findData.cFileName,
        wcslen(findData.cFileName),
        &foundName) != PCSL_STRING_OK) {
            pcsl_string_free(&root);    
            return -1;
    }

    {
        int state = PCSL_STRING_OK == pcsl_string_cat(&root, &foundName, result);
        pcsl_string_free(&foundName);
        pcsl_string_free(&root);
        return state ? 0 : -1;
    }
}

static int findNextMatch(PCSLFileIterator* pIterator,
                         const pcsl_string * match,
                         pcsl_string * result) 
{
    WIN32_FIND_DATAW findData;
    pcsl_string root = PCSL_STRING_NULL;
    pcsl_string foundName = PCSL_STRING_NULL;
    jsize rootLen = 0;

    if (NULL == result) {
        return -1;
    }

    * result = PCSL_STRING_NULL;

    if (!FindNextFileW(pIterator->iteratorHandle, &findData)) {
        return -1;
    }

    rootLen = pIterator->savedRootLength;

    if (PCSL_STRING_OK != pcsl_string_substring(match, 0, rootLen, &root)) {
        return -1;
    }

    if (pcsl_string_convert_from_utf16(findData.cFileName, 
        wcslen(findData.cFileName),
        &foundName) != PCSL_STRING_OK) {
            pcsl_string_free(&root);    
            return -1;
    }

    {
        int state = PCSL_STRING_OK == pcsl_string_cat(&root, &foundName, result);
        pcsl_string_free(&foundName);
        pcsl_string_free(&root);
        return state ? 0 : -1;
    }
}
