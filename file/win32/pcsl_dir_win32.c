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
#include <wchar.h>

#include <pcsl_directory.h>
#include <java_types.h>

/* 
 * This constant is defined in "WinBase.h" when using VS7 (2003) and VS8 (2005),
 * but absent in Visual C++ 6 headers. 
 * For successful build with VC6 we need to define it manually.
 */

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

/**
 * Check if the directory exists in FS storage.
 */
int pcsl_file_is_directory(const pcsl_string * path)
{
    DWORD attrs;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(path);

    if (NULL == pszOsFilename) {
        return -1;
    }

    attrs = GetFileAttributesW(pszOsFilename);
    pcsl_string_release_utf16_data(pszOsFilename, path);

    if (INVALID_FILE_ATTRIBUTES == attrs) {
        return -1;
    }

    return ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) ? 1 : 0;
}

/**
 * Creates the directory with specified name.
 */
int pcsl_file_mkdir(const pcsl_string * dirName)
{
    int res;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(dirName);

    if (NULL == pszOsFilename) {
        return -1;
    }

    res = CreateDirectoryW(pszOsFilename, NULL) ? 0 : -1;
    pcsl_string_release_utf16_data(pszOsFilename, dirName);

    return res;
}

/**
 * The function deletes the directory named dirName from the persistent storage.
 */
int pcsl_file_rmdir(const pcsl_string * dirName)
{
    int res;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(dirName);

    if (NULL == pszOsFilename) {
        return -1;
    }

    res = RemoveDirectoryW(pszOsFilename) ? 0 : -1;
    pcsl_string_release_utf16_data(pszOsFilename, dirName);

    return res;
}

/**
 * The getFreeSize function checks the available size in storage.
 */
jlong pcsl_file_getfreesize(const pcsl_string * path)
{
    BOOL res;

    ULARGE_INTEGER available;
    ULARGE_INTEGER totalBytes;
    ULARGE_INTEGER freeBytes;

    const jchar * pszOsFilename = pcsl_string_get_utf16_data(path);
    if (NULL == pszOsFilename) {
        return -1;
    }

    res = GetDiskFreeSpaceExW(pszOsFilename, &available, &totalBytes, &freeBytes);
    pcsl_string_release_utf16_data(pszOsFilename, path);

    if (!res) {
        return -1;
    }

    return (jlong)available.QuadPart;
}

/**
 * The getTotalSize function checks the total space in storage.
 */
jlong pcsl_file_gettotalsize(const pcsl_string * path)
{
    BOOL res;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(path);

    ULARGE_INTEGER available;
    ULARGE_INTEGER totalBytes;
    ULARGE_INTEGER freeBytes;

    if (NULL == pszOsFilename) {
        return -1;
    }

    res = GetDiskFreeSpaceExW(pszOsFilename, &available, &totalBytes, &freeBytes);
    pcsl_string_release_utf16_data(pszOsFilename, path);

    if (!res) {
        return -1;
    }

    return (jlong)totalBytes.QuadPart;
}

/**
 * The function returns value of the attribute for the specified file.
 */
int pcsl_file_get_attribute(const pcsl_string * fileName, int type, int* result)
{
    DWORD attrs;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(fileName);

    if (NULL == pszOsFilename) {
        return -1;
    }

    attrs = GetFileAttributesW(pszOsFilename);
    pcsl_string_release_utf16_data(pszOsFilename, fileName);

    if (INVALID_FILE_ATTRIBUTES == attrs) {
        return -1;
    }

    switch (type) {
        case PCSL_FILE_ATTR_READ:
        case PCSL_FILE_ATTR_EXECUTE:
            *result = 1;
            break;
        case PCSL_FILE_ATTR_WRITE:
            *result = (attrs & FILE_ATTRIBUTE_READONLY) ? 0 : 1;
            break;
        case PCSL_FILE_ATTR_HIDDEN:
            *result = (attrs & FILE_ATTRIBUTE_HIDDEN) ? 1 : 0;
            break;
        default:
            return -1;
    }        
    return 0;
}

/**
 * The function sets value of the attribute for the specified file.
 */
int pcsl_file_set_attribute(const pcsl_string * fileName, int type, int value)
{
    DWORD attrs, newmode;
    int result = -1;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(fileName);

    if (NULL == pszOsFilename) {
        return -1;
    }

    newmode = attrs = GetFileAttributesW(pszOsFilename);
    while (INVALID_FILE_ATTRIBUTES != attrs) {
        switch (type) {
        case PCSL_FILE_ATTR_READ:
        case PCSL_FILE_ATTR_EXECUTE:
            break;
        case PCSL_FILE_ATTR_WRITE:
            if (value) {
                newmode &= ~FILE_ATTRIBUTE_READONLY;
            } else {
                newmode |= FILE_ATTRIBUTE_READONLY;
            }
            break;
        case PCSL_FILE_ATTR_HIDDEN:
            if (value) {
                newmode |= FILE_ATTRIBUTE_HIDDEN;
            } else {
                newmode &= ~FILE_ATTRIBUTE_HIDDEN;
            }
        }
    
        /* do not update file attributes if they are not changed */
        if (newmode == attrs) {
            result = 0;
            break;
        }

        if (0 != SetFileAttributesW(pszOsFilename, newmode)) {
            result = 0;
        }
        break;
    }

    pcsl_string_release_utf16_data(pszOsFilename, fileName);
    return result;
}

/**
 * The function returns value of the time for the specified file.
 */
int pcsl_file_get_time(const pcsl_string * fileName, int type, long* result)
{
    /*
     * Expected time is count as seconds from (00:00:00), January 1, 1970 UTC
     * FILETIME time is 100-nanosecond intervals since January 1, 1601 UTC 
     */
    /* Visual C++ 6 suports only i64 suffix (not LL) */
    static const LONGLONG FILETIME_1970_01_JAN_UTC = 116444736000000000i64;

    WIN32_FILE_ATTRIBUTE_DATA attrib;
    int state = -1;
    const jchar* pOsFN;
    
    if (PCSL_FILE_TIME_LAST_MODIFIED != type)
        return -1;

    pOsFN = pcsl_string_get_utf16_data(fileName);
    if (pOsFN != NULL) {
        LONGLONG fcurUTC = 0;
        if (GetFileAttributesExW(pOsFN, GetFileExInfoStandard, &attrib)) {
            /* FS times is in UTC */
            fcurUTC = attrib.ftLastWriteTime.dwHighDateTime;
            fcurUTC = (fcurUTC << 32) | attrib.ftLastWriteTime.dwLowDateTime;
        }

        /* FILETIME members are zero if the FS does not support this time */
        if (0 != fcurUTC) {
            fcurUTC -= FILETIME_1970_01_JAN_UTC;
            fcurUTC /= 10000000;
            state = 0;
            *result = (long)fcurUTC;
        }

        pcsl_string_release_utf16_data(pOsFN, fileName);
    }

    return state;
}
