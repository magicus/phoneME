/*
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
 * Javacall interfaces for file
 */

#include "javacall_dir.h"
#include "javacall_file.h"
#include "javacall_logging.h"
#include "javacall_memory.h"

#include <wchar.h>
#include <windows.h>


typedef struct JAVACALL_FIND_DATA {
        WIN32_FIND_DATAW find_data;
        BOOL first_time;
        HANDLE handle; /*win32 searching handle*/
} JAVACALL_FIND_DATA;

/**
 * Returns handle to a file list. This handle can be used in
 * subsequent calls to javacall_dir_get_next() to iterate through
 * the file list and get the names of files that match the given string.
 * 
 * @param path the name of a directory, but it can be a
 *             partial file name
 * @param pathLen length of directory name
 * @return pointer to an opaque filelist structure, that can be used in
 *         javacall_dir_get_next() and javacall_dir_close().
 *         NULL returned on error, for example if root directory of the
 *         input path cannot be found.
 */
javacall_handle javacall_dir_open(javacall_const_utf16_string path, int pathLen)
{
    javacall_handle handle;
    JAVACALL_FIND_DATA* pFindData;
    WIN32_FIND_DATAW javacall_dir_data;
    int nErrNo;
    wchar_t* dirPath;

	if (pathLen == -1) {
		pathLen = wcslen(path);
	}

    if (path[pathLen - 2] != '/' || path[pathLen - 1] != '*') {
        int dirPathLen = pathLen + 2 + (path[pathLen - 1] == '/' ? 0 : 1);
        dirPath = LocalAlloc(LPTR, dirPathLen * sizeof(wchar_t));
        wcscpy(dirPath, path);
        dirPath[dirPathLen - 3] = '/';
        dirPath[dirPathLen - 2] = '*';
        dirPath[dirPathLen - 1] = 0;
    }
    else {
        dirPath = (wchar_t *)path;
    }
    
    handle = FindFirstFileW(dirPath, &javacall_dir_data);
    nErrNo = GetLastError();

    if (handle == INVALID_HANDLE_VALUE && nErrNo != ERROR_NO_MORE_FILES)
    {
        return NULL;
    }

    pFindData = LocalAlloc(LPTR, sizeof(JAVACALL_FIND_DATA));
    memset(pFindData,0,sizeof(JAVACALL_FIND_DATA));
    pFindData->find_data = javacall_dir_data;
    pFindData->first_time = TRUE;
    if ((handle == INVALID_HANDLE_VALUE) && (nErrNo == ERROR_NO_MORE_FILES)) {
        pFindData->handle = 0; /* Empty folder */
    }
    else {
        pFindData->handle = handle;
    }
    if (dirPath != path) {
        LocalFree(dirPath);
    }
    return (javacall_handle)pFindData;
}

/**
 * Closes the specified file list. The handle will no longer be
 * associated with the file list.
 *
 * @param handle pointer to opaque filelist structure returned by
 *               javacall_dir_open().
 */
void javacall_dir_close(javacall_handle handle)
{
    JAVACALL_FIND_DATA* pFindData = (JAVACALL_FIND_DATA*)handle;

    if ((pFindData != NULL) && (pFindData->handle == 0))
    {
        free(pFindData);
        return;
    }

    if ((pFindData != NULL) &&
        (pFindData->handle != INVALID_HANDLE_VALUE)) {
        FindClose(pFindData->handle);
        LocalFree(pFindData);
    }
}

/**
 * Returns the next filename in directory path.
 * The order is defined by the underlying file system. Current and
 * parent directory links ("." and "..") must not be returned.
 * This function must behave correctly (e.g. not skip any existing files)
 * even if some files are deleted from the directory between subsequent
 * calls to <tt>javacall_dir_get_next()</tt>.
 * The filename returned will omit the file's path.
 * 
 * @param handle pointer to filelist struct returned by javacall_dir_open().
 * @param outFilenameLength will be filled with number of chars written 
 * @return pointer to the next file name on success, NULL otherwise.
 * The returned value is a pointer to platform specific memory block,
 * so platform MUST BE responsible for allocating and freeing it.
 */
javacall_utf16* javacall_dir_get_next(javacall_handle handle,
                                      int* /*OUT*/ outFileNameLength)
{
    JAVACALL_FIND_DATA* pFindData = (JAVACALL_FIND_DATA*)handle;

    if((pFindData == NULL) || (pFindData->handle == INVALID_HANDLE_VALUE) ){
        if (outFileNameLength != NULL ) { 
			*outFileNameLength = 0; 
		}
        return NULL;
    }
    if (pFindData->handle == 0)
        return NULL;

    if (!pFindData->first_time) {
        if (FindNextFileW(pFindData->handle, &(pFindData->find_data)) == 0) {
            if (outFileNameLength != NULL) { 
				*outFileNameLength = 0; 
			}
            return NULL;
        }
    }
    pFindData->first_time = FALSE;

    if (outFileNameLength != NULL) { 
		*outFileNameLength = wcslen((wchar_t *)(pFindData->find_data.cFileName)); 
	}
    return  (pFindData->find_data.cFileName);
}

/**
 * Checks the size of free space in storage.
 * @return size of free space
 */
javacall_int64 javacall_dir_get_free_space_for_java(void){
    wchar_t rootPath[JAVACALL_MAX_FILE_NAME_LENGTH+1]; /* max file name */
    int rootPathLen = JAVACALL_MAX_FILE_NAME_LENGTH+1;
    ULARGE_INTEGER freeBytesForMe, totalBytes, totalFreeBytes;

    memset(rootPath, 0, (JAVACALL_MAX_FILE_NAME_LENGTH+1) * sizeof(wchar_t));
    javacall_dir_get_root_path(rootPath, &rootPathLen);
    rootPath[rootPathLen] = 0;
    if (GetDiskFreeSpaceExW(rootPath, &freeBytesForMe, &totalBytes, &totalFreeBytes)) {
       javacall_int64 ret = (javacall_int64)freeBytesForMe.QuadPart;
       return  ret;
    } else { 
       return 0;
    }
}

/**
 * Returns the root path of java's home directory.
 *
 * @param rootPath returned value: pointer to unicode buffer, allocated
 *        by the VM, to be filled with the root path.
 * @param rootPathLen IN  : lenght of max rootPath buffer
 *                    OUT : lenght of set rootPath
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */
javacall_result javacall_dir_get_root_path(javacall_utf16* /* OUT */ rootPath, int* /* IN | OUT */ rootPathLen) {
    int i;
    static BOOL bCreated = FALSE;
    DWORD res = GetModuleFileNameW(NULL, rootPath, *rootPathLen);
    if (0 == res)
        return JAVACALL_FAIL;

    for (i= wcslen(rootPath) -1 ; i>=0; i--) {
        if (rootPath[i] != '\\') {
            rootPath[i] = 0;
        } else {
            break;
        }
    }
    rootPath[i] = 0; /* null-terminated */
    wcscat(rootPath, L"\\Java"); /* java-home dir is at jvm_exe_path/Java. */
    *rootPathLen = wcslen(rootPath);

    if (!bCreated) { /* at first time, we create jvm-exe-path/Java dir whether it is existed. */
        CreateDirectoryW(rootPath, NULL);
        bCreated = TRUE;
    }
    return JAVACALL_OK;
}