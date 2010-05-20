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
#include <direct.h>
#include <wchar.h>

#include <pcsl_directory.h>
#include <java_types.h>


/* 
 * This constant is defined in "WinBase.h" when using MS Visual C++ 7, but absent
 * in Visual C++ 6 headers. For successful build with VC6 we need to define it manually.
 */
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

/**
 * Check if the directory exists in FS storage.
 */
int pcsl_file_is_directory(const pcsl_string * path)
{
    int attrs;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(path);

    if(NULL == pszOsFilename) {
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

    res = _wmkdir(pszOsFilename);
    pcsl_string_release_utf16_data(pszOsFilename, dirName);

    return (0 == res) ? 0 : -1;
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

    res = _wrmdir(pszOsFilename);
    pcsl_string_release_utf16_data(pszOsFilename, dirName);

    return (0 == res) ? 0 : -1;
}

/**
 * The getFreeSize function checks the available size in storage.
 */
jlong pcsl_file_getfreesize(const pcsl_string * path)
{
    struct _diskfree_t df;
    struct _stat buf;
    int res;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(path);

    if (NULL == pszOsFilename) {
        return -1;
    }

    res = _wstat(pszOsFilename, &buf);
    pcsl_string_release_utf16_data(pszOsFilename, path);
    if (0 != res) {
        return -1;
    }

    if (0 != _getdiskfree(buf.st_dev + 1, &df)) {
        return -1;
    }

    return (jlong)(df.avail_clusters) * df.sectors_per_cluster * df.bytes_per_sector;
}

/**
 * The getTotalSize function checks the total space in storage.
 */
jlong pcsl_file_gettotalsize(const pcsl_string * path)
{
    struct _diskfree_t df;
    struct _stat buf;
    int res;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(path);

    if (NULL == pszOsFilename) {
        return -1;
    }

    res = _wstat(pszOsFilename, &buf);
    pcsl_string_release_utf16_data(pszOsFilename, path);
    if (0 != res) {
        return -1;
    }

    if (0 != _getdiskfree(buf.st_dev + 1, &df)) {
        return -1;
    }

    return (jlong)(df.total_clusters) * df.sectors_per_cluster * df.bytes_per_sector;
}

/**
 * The function returns value of the attribute for the specified file.
 */
int pcsl_file_get_attribute(const pcsl_string * fileName, int type, int* result)
{
    int attrs;
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
    int attrs, newmode;
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
    struct _stat buf;
    int res;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(fileName);

    if (NULL == pszOsFilename) {
        return -1;
    }

    res = _wstat(pszOsFilename, &buf);
    pcsl_string_release_utf16_data(pszOsFilename, fileName);
    if (-1 == res) {
        return -1;
    }

    *result = (long)buf.st_mtime;
    return 0;
}

