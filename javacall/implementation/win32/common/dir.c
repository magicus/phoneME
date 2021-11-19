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
 * Implemenation for dir javacall functions
 */

#include "javacall_dir.h"
#include "javacall_file.h"
#include "javacall_logging.h"
#include "javacall_memory.h"
#include "javacall_events.h"

#include <wchar.h>
#include <windows.h>

typedef struct JAVACALL_FIND_DATA {
        WIN32_FIND_DATAW find_data;
        BOOL first_time;
        HANDLE hFind; /*win32 searching handle*/
} JAVACALL_FIND_DATA;


#define DEFAULT_MAX_JAVA_SPACE (8*1024*1024) 
#define EMPTY_DIRECTORY_HANDLE NULL
#define APPDB_DIR L"appdb"
#define CONFIG_DIR L"lib"

/**
 * Returns file separator character used by the underlying file system
 * The file separator, typically, is a single character that separates
 * directories in a path name, for example <dir1>/<dir2>/file.c
 * (usually this function will return '\\';)
 * @return 16-bit Unicode encoded file separator
 */
javacall_utf16 javacall_get_file_separator(void) 
{
    return (javacall_utf16)L'\\';
}


/**
 * Returns handle to a file list. This handle can be used in
 * subsequent calls to javacall_dir_get_next() to iterate through
 * the file list and get the names of files that match the given string.
 * 
 * @param path the name of a directory, but it can be a
 *             partial file name
 * @param pathLen length of directory name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return pointer to an opaque filelist structure, that can be used in
 *         javacall_dir_get_next() and javacall_dir_close().
 *         NULL returned on error, for example if root directory of the
 *         input path cannot be found.
 */
javacall_handle javacall_dir_open(javacall_const_utf16_string path, int pathLen)
{
    HANDLE              hFind;
    JAVACALL_FIND_DATA* pFindData;
    wchar_t wOsPath[JAVACALL_MAX_FILE_NAME_LENGTH+1];

    if (pathLen == JAVACALL_UNKNOWN_LENGTH) {
        pathLen = wcslen(path);
    }
    if( pathLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
        return NULL;
    }

    memcpy(wOsPath, path, pathLen * sizeof(wchar_t));

    if (wOsPath[pathLen - 2] != '/' || wOsPath[pathLen - 2] != '\\' 
        || wOsPath[pathLen - 1] != '*') {
            if (wOsPath[pathLen - 1] == L'/' || wOsPath[pathLen - 1] == L'\\') {
                wOsPath[pathLen++] = '*';
            } else {
                wOsPath[pathLen++] = javacall_get_file_separator();
                wOsPath[pathLen++] = '*';
            }
    }
    wOsPath[pathLen] = '\0';
    
    pFindData = malloc(sizeof(JAVACALL_FIND_DATA));
    if (pFindData == NULL)
        return NULL;

    hFind = FindFirstFileW(wOsPath, &(pFindData->find_data));
    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD nErrNo = GetLastError();
        if (nErrNo == ERROR_NO_MORE_FILES) {
            hFind = EMPTY_DIRECTORY_HANDLE;
        } else {
            free(pFindData);
            return NULL;
        }
    }

    pFindData->first_time = TRUE;
    pFindData->hFind = hFind;
    return pFindData;
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

    if (pFindData != NULL) {
        if (pFindData->hFind != EMPTY_DIRECTORY_HANDLE) {
            FindClose(pFindData->hFind);
        }
        free(pFindData);
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

    if (outFileNameLength != NULL ) { 
        /* Default value */
        *outFileNameLength = 0; 
    }

    if ((pFindData == NULL) || (pFindData->hFind == EMPTY_DIRECTORY_HANDLE)) {
        return NULL;
    }

    if (!pFindData->first_time) {
        if (FindNextFileW(pFindData->hFind, &(pFindData->find_data)) == 0) {
            return NULL;
        }
    }
    pFindData->first_time = FALSE;

    if (outFileNameLength != NULL) { 
        *outFileNameLength = wcslen(pFindData->find_data.cFileName); 
    }
    return (pFindData->find_data.cFileName);
}

/**
 * Checks the size of free space in storage.
 * @return size of free space
 */
javacall_int64 javacall_dir_get_free_space_for_java(void)
{
    wchar_t rootPath[JAVACALL_MAX_FILE_NAME_LENGTH+1]; /* max file name */
    int rootPathLen = JAVACALL_MAX_FILE_NAME_LENGTH+1;
    ULARGE_INTEGER freeBytesForMe, totalBytes, totalFreeBytes;

    memset(rootPath, 0, (JAVACALL_MAX_FILE_NAME_LENGTH+1) * sizeof(wchar_t));
    javacall_dir_get_root_path(rootPath, &rootPathLen);
    rootPath[rootPathLen] = 0;
    if (GetDiskFreeSpaceExW(rootPath, &freeBytesForMe, &totalBytes, &totalFreeBytes)) {
        javacall_int64 ret = (javacall_int64)freeBytesForMe.QuadPart;
        if (ret > DEFAULT_MAX_JAVA_SPACE) {
            return DEFAULT_MAX_JAVA_SPACE;
        }
        return  ret;
    } else { 
        return 0;
    }
}


/**
 * Returns the path of java's home directory, in which appdb and lib reside
 *
 * @param rootPath returned value: pointer to unicode buffer, allocated
 *        by the VM, to be filled with the root path.
 * @param rootPathLen IN  : lenght of max rootPath buffer
 *                    OUT : lenght of set rootPath
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */
#ifdef UNDER_CE
javacall_result helper_dir_get_home_path(javacall_utf16* /* OUT */ rootPath, 
                                                    int* /* IN | OUT */ rootPathLen)
{
    int i;
    static BOOL bCreated = FALSE;
    
    DWORD res = GetModuleFileNameW(NULL, rootPath, *rootPathLen);
    if (0 == res)
        return JAVACALL_FAIL;

    for (i= wcslen(rootPath) -1 ; i>=0; i--) {
        if (rootPath[i] != '\\') {
            rootPath[i] = L'\0';
        } else {
            break;
        }
    }
    rootPath[i] = L'\0'; /* null-terminated */
    wcscat(rootPath, L"\\Java"); /* java-home dir is at jvm_exe_path/Java. */
    *rootPathLen = wcslen(rootPath);

    if (!bCreated) { 
        /* at first time, we create jvm-exe-path/Java dir whether it is existed. */
        CreateDirectoryW(rootPath, NULL);
        bCreated = TRUE;
    }
    return JAVACALL_OK;
}
#else /* !UNDER_CE */
javacall_result helper_dir_get_home_path(javacall_utf16* /* OUT */ rootPath, 
                                           int* /* IN | OUT */ rootPathLen)
{
    wchar_t dirBuffer[JAVACALL_MAX_ROOT_PATH_LENGTH + 1];
    wchar_t currDir[JAVACALL_MAX_ROOT_PATH_LENGTH + 1];
    wchar_t* midpHome;

    if (rootPath == NULL || rootPathLen == NULL) {
        return JAVACALL_FAIL;
    }

    /*
     * If MIDP_HOME is set, just use it. Does not check if MIDP_HOME is
     * pointing to a directory contain "appdb".
     */
    midpHome = _wgetenv(L"MIDP_HOME");
    if (midpHome != NULL) {
        int len = (int) wcslen(midpHome);
        if (len >= *rootPathLen) {
            * rootPathLen = 0;
            return JAVACALL_FAIL;
        }

        wcscpy(rootPath, midpHome);
        * rootPathLen = len;
        return JAVACALL_OK;
    }

    /*
     * Look for "appdb" until it is found in the following places:
     * - current directory;
     * - the parent directory of the midp executable;
     * - the grandparent directory of the midp executable.
     */
    if ( _wgetcwd( currDir, sizeof(currDir)/sizeof(wchar_t) ) == NULL) {
        * rootPathLen = 0;
        return JAVACALL_FAIL;
    } else {
        wchar_t* lastsep;
        WIN32_FILE_ATTRIBUTE_DATA lpFileInformation;
        int i, j = 1;
        wchar_t chSep = javacall_get_file_separator();
        wchar_t filesep[2] = {chSep, (wchar_t)0};

        dirBuffer[sizeof(dirBuffer)/sizeof(wchar_t) - 1] = (wchar_t)0;
        wcsncpy(dirBuffer, currDir, sizeof(dirBuffer)/sizeof(wchar_t) - 1);

        while (j < 2) {
            /* Look for the last slash in the pathname. */
            lastsep = wcsrchr(dirBuffer, *filesep);
            if (lastsep != NULL) {
                *(lastsep + 1) = L'\0';
            } else {
                /* no file separator */
                wcscpy(dirBuffer, L".");
                wcscat(dirBuffer, filesep);
            }

            wcscat(dirBuffer, APPDB_DIR);
            i = 0;

            /* try to search for "appdb" 3 times only (see above) */
            while (i < 3) {
                /* found it and it is a directory */
                if ((GetFileAttributesExW(dirBuffer, GetFileExInfoStandard, &lpFileInformation) != 0) &&
                    (lpFileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                    break;

                /* strip off "appdb" to add 1 more level of ".." */
                *(wcsrchr(dirBuffer, *filesep)) = L'\0';
                wcscat(dirBuffer, filesep);
                wcscat(dirBuffer, L"..");
                wcscat(dirBuffer, filesep);
                wcscat(dirBuffer, APPDB_DIR);

                i++;
            } /* end while (i < 3) */

            if (i < 3) {
                break;
            }

            j++;
        } /* end while (j < 2) */

        if (j == 2) {
            * rootPathLen = 0;
            return JAVACALL_FAIL;
        }

        /* strip off "appdb" from the path */
        *(wcsrchr(dirBuffer, *filesep)) = L'\0';

        wcscpy(rootPath, dirBuffer);
        * rootPathLen = wcslen(rootPath);

        return JAVACALL_OK;
    }
}
#endif /* UNDER_CE */


/**
 * Returns the path of java's application directory.
 *
 * @param rootPath returned value: pointer to unicode buffer, allocated
 *        by the VM, to be filled with the root path.
 * @param rootPathLen IN  : lenght of max rootPath buffer
 *                    OUT : lenght of set rootPath
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */
javacall_result javacall_dir_get_root_path(javacall_utf16* /* OUT */ rootPath, 
                                           int* /* IN | OUT */ rootPathLen)
{
    wchar_t chSep = javacall_get_file_separator();
    wchar_t filesep[2] = {chSep, (wchar_t)0};
    javacall_result res;
    int len;

    if (rootPathLen == NULL) {
        return JAVACALL_FAIL;
    }
    
    len = *rootPathLen;
    res = helper_dir_get_home_path(rootPath, &len);

    if (res != JAVACALL_OK) {
        return res;
    }

    if (len + (int)wcslen(APPDB_DIR) >= *rootPathLen) {
        return JAVACALL_FAIL;
    }

    wcscat(rootPath, filesep);
    wcscat(rootPath, APPDB_DIR);
    *rootPathLen = wcslen(rootPath);

    return JAVACALL_OK;
}


/**
 * Returns the path of java's configuration directory.
 *
 * @param configPath returned value: pointer to unicode buffer, allocated
 *        by the VM, to be filled with the root path.
 * @param configPathLen IN  : lenght of max configPath buffer
 *                    OUT : lenght of set configPath
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */
javacall_result javacall_dir_get_config_path(javacall_utf16* /* OUT */ configPath, 
                                             int* /* IN | OUT */ configPathLen)
{
    wchar_t chSep = javacall_get_file_separator();
    wchar_t filesep[2] = {chSep, (wchar_t)0};
    javacall_result res;
    int len;

    if (configPathLen == NULL) {
        return JAVACALL_FAIL;
    }
    
    len = *configPathLen;
    res = helper_dir_get_home_path(configPath, &len);

    if (res != JAVACALL_OK) {
        return res;
    }

    if (len + (int)wcslen(CONFIG_DIR) >= *configPathLen) {
        return JAVACALL_FAIL;
    }

    wcscat(configPath, filesep);
    wcscat(configPath, CONFIG_DIR);
    *configPathLen = wcslen(configPath);

    return JAVACALL_OK;
}

