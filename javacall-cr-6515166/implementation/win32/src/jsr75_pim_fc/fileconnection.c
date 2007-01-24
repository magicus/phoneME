/*
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

/**
 * @file
 *
 * win32 implemenation for  FileConnection API
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 *
 *             JSR075's FileConnection API
 *            =============================
 *
 *
 * The following API definitions are required by JSR075.
 * These APIs are not required by standard JTWI implementations.
 * These APIs are file related only. Additional APIs required by JSR075
 * which are directory related can be found in javacall_dir.h
 *
 * These extensions include:
 * - javacall_fileconnection_init()
 * - javacall_fileconnection_finalize()
 * - javacall_fileconnection_is_hidden()
 * - javacall_fileconnection_is_readable()
 * - javacall_fileconnection_is_writable()
 * - javacall_fileconnection_set_hidden()
 * - javacall_fileconnection_set_readable()
 * - javacall_fileconnection_set_writable()
 * - javacall_fileconnection_get_last_modified()
 * - javacall_fileconnection_get_illegal_filename_chars()
 * - javacall_fileconnection_is_directory()
 * - javacall_fileconnection_create_dir()
 * - javacall_fileconnection_delete_dir()
 * - javacall_fileconnection_dir_exists()
 * - javacall_fileconnection_rename_dir()
 * - javacall_fileconnection_get_free_size()
 * - javacall_fileconnection_get_total_size()
 * - javacall_fileconnection_get_mounted_roots()
 * - javacall_fileconnection_get_photos_dir()
 * - javacall_fileconnection_get_videos_dir()
 * - javacall_fileconnection_get_graphics_dir()
 * - javacall_fileconnection_get_tones_dir()
 * - javacall_fileconnection_get_music_dir()
 * - javacall_fileconnection_get_recordings_dir()
 * - javacall_fileconnection_get_private_dir()
 * - javacall_fileconnection_get_localized_mounted_roots()
 * - javacall_fileconnection_get_localized_photos_dir()
 * - javacall_fileconnection_get_localized_videos_dir()
 * - javacall_fileconnection_get_localized_graphics_dir()
 * - javacall_fileconnection_get_localized_tones_dir()
 * - javacall_fileconnection_get_localized_music_dir()
 * - javacall_fileconnection_get_localized_recordings_dir()
 * - javacall_fileconnection_get_localized_private_dir()
 * - javacall_fileconnection_get_path_for_root()
 * - javacall_fileconnection_dir_content_size()
 * - javanotify_fileconnection_root_changed()
 */

#include <windows.h>
#include <direct.h>
#include <wchar.h>
//#include <fcntl.h>
//#include <string.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include "javacall_time.h"
#include "javacall_logging.h"
#include "javacall_dir.h"
#include "javacall_file.h"
#include "javacall_fileconnection.h"


/* 
 * This constant is defined in "WinBase.h" when using MS Visual C++ 7, but absent
 * in Visual C++ 6 headers. For successful build with VC6 we need to define it manually.
 */
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif


static void mount_timer_callback(javacall_handle);
static unsigned long oldRoots;
static javacall_handle mount_timer;

static const javacall_utf16 photos_dir[]     = L"C:/fc/photos/";
static const javacall_utf16 videos_dir[]     = L"C:/fc/videos/";
static const javacall_utf16 graphics_dir[]   = L"C:/fc/graphics/";
static const javacall_utf16 tones_dir[]      = L"C:/fc/tones/";
static const javacall_utf16 music_dir[]      = L"C:/fc/music/";
static const javacall_utf16 recordings_dir[] = L"C:/fc/recordings/";
static const javacall_utf16 private_dir[]    = L"C:/fc/private/";

static const javacall_utf16 localized_root_prefix[]    = L"Drive ";
static const javacall_utf16 localized_photos_dir[]     = L"My Photos";
static const javacall_utf16 localized_videos_dir[]     = L"My Videos";
static const javacall_utf16 localized_graphics_dir[]   = L"My Graphics";
static const javacall_utf16 localized_tones_dir[]      = L"My Tones";
static const javacall_utf16 localized_music_dir[]      = L"My Music";
static const javacall_utf16 localized_recordings_dir[] = L"My Recordings";
static const javacall_utf16 localized_private_dir[]    = L"Private";

/**
 * Makes all the required initializations for JSR 75 FileConnection
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured or feature is not supported
 */
javacall_result javacall_fileconnection_init(void) {

    javacall_result res;

    oldRoots = _getdrives();
    res = javacall_time_initialize_timer(1000,
                                         JAVACALL_TRUE,
                                         mount_timer_callback,
                                         &mount_timer);
    return res;

}

/**
 * Cleans up resources used by fileconnection
 * @return JAVACALL_OK on success, JAVACALL_FAIL otherwise
 */
javacall_result javacall_fileconnection_finalize(void) {
    return javacall_time_finalize_timer(mount_timer);
}

/**
 * Returns the HIDDEN attribute for the specified file or directory
 * If hidden files are not supported, the function should 
 * return JAVACALL_FALSE
 *
 * @param fileName      name in UNICODE of file
 * @param fileNameLen   length of file name
 * @param result        returned value: JAVACALL_TRUE if file is hidden
 *                      JAVACALL_FALSE file is not hidden or 
 *                      feature is not supported
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */
javacall_result javacall_fileconnection_is_hidden(const javacall_utf16* fileName,
                                                  int fileNameLen,
                                                  javacall_bool* /* OUT */ result) {
    wchar_t wOsFilename[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name
    int attrs;

    if( fileNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
        javacall_print("Error: javacall_fileconnection_is_hidden(), file name is too long\n");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, fileName, fileNameLen*sizeof(wchar_t));
    wOsFilename[fileNameLen] = 0;

    attrs = GetFileAttributesW(wOsFilename);
    if (INVALID_FILE_ATTRIBUTES == attrs) {
        javacall_print("Error: javacall_fileconnection_is_hidden(), file not found\n");
        return JAVACALL_FAIL;
    }

    *result = ((attrs & FILE_ATTRIBUTE_HIDDEN) == 0) ? JAVACALL_FALSE : JAVACALL_TRUE;
    return JAVACALL_OK;
}

/**
 * Returns the READABLE attribute for the specified file or directory
 *
 * @param pathName      name in UNICODE of file or directory
 * @param pathNameLen   length of path name
 * @param result        returned value: JAVACALL_TRUE if file/dir is readable
 *                      JAVACALL_FALSE file/dir is not readable
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */ 
javacall_result javacall_fileconnection_is_readable(const javacall_utf16* pathName,
                                                    int pathNameLen,
                                                    javacall_bool* /* OUT */ result) {

    wchar_t wOsFilename[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name

    if( pathNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
        javacall_print("Error: javacall_fileconnection_is_readable(), file name is too long\n");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, pathName, pathNameLen*sizeof(wchar_t));
    wOsFilename[pathNameLen] = 0;

    if(_waccess(wOsFilename, 0) == -1) {
        javacall_print("Error: javacall_fileconnection_is_readable(), file is not accessible\n");
        return JAVACALL_FAIL;
    }

    *result = (_waccess(wOsFilename, 4) == 0) ? JAVACALL_TRUE : JAVACALL_FALSE;
    return JAVACALL_OK;
}

/**
 * Returns the WRITABLE attribute for the specified file or directory
 *
 * @param pathName      name in UNICODE of file or directory
 * @param pathNameLen   length of path name
 * @param result        returned value: JAVACALL_TRUE if file/dir is writable
 *                      JAVACALL_FALSE file/dir is not writable
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */ 
javacall_result javacall_fileconnection_is_writable(const javacall_utf16* pathName,
                                                    int pathNameLen,
                                                    javacall_bool* /* OUT */ result) {

    wchar_t wOsFilename[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name

    if( pathNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
        javacall_print("Error: javacall_fileconnection_is_writable(), file name is too long\n");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, pathName, pathNameLen*sizeof(wchar_t));
    wOsFilename[pathNameLen] = 0;

    if(_waccess(wOsFilename, 0) == -1) {
        javacall_print("Error: javacall_fileconnection_is_writable(), file is not accessible\n");
        return JAVACALL_FAIL;
    }

    *result = (_waccess(wOsFilename, 2) == 0) ? JAVACALL_TRUE : JAVACALL_FALSE;
    return JAVACALL_OK;
}

/**
 * Sets the HIDDEN attribute for the specified file or directory
 *
 * @param fileName      name in UNICODE of file
 * @param fileNameLen   length of file name
 * @param value         JAVACALL_TRUE to set file as hidden
 *                      JAVACALL_FALSE to set file as not hidden
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */
javacall_result javacall_fileconnection_set_hidden(const javacall_utf16* fileName,
                                                   int fileNameLen,
                                                   javacall_bool value) {

    int attrs;
    wchar_t wOsFilename[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name

    if( fileNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
        javacall_print("Error: javacall_fileconnection_set_hidden(), file name is too long\n");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, fileName, fileNameLen*sizeof(wchar_t));
    wOsFilename[fileNameLen] = 0;

    attrs = GetFileAttributesW(wOsFilename);

    if(-1 == attrs) {
        javacall_print("Error: javacall_fileconnection_set_hidden(), cannot get file attributes\n");
        return JAVACALL_FAIL;
    }

    if(JAVACALL_TRUE == value) {
        attrs = attrs | FILE_ATTRIBUTE_HIDDEN;
    } else {
        attrs = attrs & ~FILE_ATTRIBUTE_HIDDEN;
    }

    if(!SetFileAttributesW(wOsFilename, attrs)) {
        javacall_print("Error: javacall_fileconnection_set_hidden(), cannot set file attributes\n");
        return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}

/**
 * Sets the READABLE attribute for the specified file or directory
 *
 * @param pathName      name in UNICODE of file or directory
 * @param pathNameLen   length of path name
 * @param value         JAVACALL_TRUE to set file as readable
 *                      JAVACALL_FALSE to set file as not readable
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */ 
javacall_result javacall_fileconnection_set_readable(const javacall_utf16* pathName,
                                                     int pathNameLen,
                                                     javacall_bool value) {

    wchar_t wOsFilename[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name

    if( pathNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
        javacall_print("Error: javacall_fileconnection_set_readable(), file name is too long\n");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, pathName, pathNameLen*sizeof(wchar_t));
    wOsFilename[pathNameLen] = 0;

    if(_waccess(wOsFilename, 0) == -1) {
        javacall_print("Error: javacall_fileconnection_set_readable(), file is not accessible\n");
        return JAVACALL_FAIL;
    }

    return JAVACALL_OK; // files are always readable, the call is ignored
}

/**
 * Sets the WRITABLE attribute for the specified file or directory
 *
 * @param pathName      name in UNICODE of file or directory
 * @param pathNameLen   length of path name
 * @param value         JAVACALL_TRUE to set file as writable
 *                      JAVACALL_FALSE to set file as not writable
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */ 
javacall_result javacall_fileconnection_set_writable(const javacall_utf16* pathName,
                                                     int pathNameLen,
                                                     javacall_bool value) {

    wchar_t wOsFilename[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name

    if( pathNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
        javacall_print("Error: javacall_fileconnection_set_writable(), file name is too long\n");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, pathName, pathNameLen*sizeof(wchar_t));
    wOsFilename[pathNameLen] = 0;

    if(_wchmod(wOsFilename, (JAVACALL_TRUE == value) ? _S_IWRITE : _S_IREAD) == -1) {
        javacall_print("Error: javacall_fileconnection_set_writable(), file is not accessible\n");
        return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}

/**
 * Returns the time when the file was last modified.
 *
 * @param fileName      name in UNICODE of file
 * @param fileNameLen   length of file name
 * @param result        A javacall_int64 value representing the time the file was 
 *                      last modified, measured in seconds since the epoch (00:00:00 GMT, 
 *                      January 1, 1970)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_fileconnection_get_last_modified(const javacall_utf16* fileName,
                                                          int fileNameLen,
                                                          javacall_int64* /* OUT */ result) {
    struct _stat buf;
    wchar_t wOsFilename[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name

    if( fileNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
        javacall_print("Error: javacall_fileconnection_get_last_modified(), file name is too long\n");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, fileName, fileNameLen*sizeof(wchar_t));
    wOsFilename[fileNameLen] = 0;

    if (_wstat(wOsFilename, &buf) == -1) {
        javacall_print("Error: javacall_fileconnection_get_last_modified(), file is not accessible\n");
        return JAVACALL_FAIL;
    }

    *result = buf.st_mtime;
    return JAVACALL_OK;
}

/**
 * Returns the list of illegal characters in file names. The list must not
 * include '/', but must include native file separator, if it is different
 * from '/' character
 * @param illegalChars returned value: pointer to UNICODE string, allocated
 *                     by the VM, to be filled with the characters that are
 *                     not allowed inside file names.
 * @param illegalCharsLenMaxLen available size, in javacall_utf16 symbols,
 *                              of the buffer provided
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_fileconnection_get_illegal_filename_chars(javacall_utf16* /* OUT */ illegalChars,
                                                                   int illegalCharsMaxLen) {
    int i;
    char str[] = "<>:\"\\|";

    if(illegalCharsMaxLen < 7) {
        javacall_print("Error: javacall_fileconnection_get_illegal_filename_chars(), insufficient buffer size\n");
        return JAVACALL_FAIL;
    }

    for(i = 0; i <= 6; i++) { // all chars, including trailing zero
        illegalChars[i] = (unsigned short) str[i];
    }

    return JAVACALL_OK;
}


/**
 * Checks if the path exists in the file system storage and if 
 * it is a directory.
 * @param pathName name of file or directory in unicode format
 * @param pathNameLen length of pathName
 * @param result returned value: JAVACALL_TRUE if path is a directory, 
 *                               JAVACALL_FALSE otherwise
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */
javacall_result javacall_fileconnection_is_directory(const javacall_utf16* pathName,
                                                     int pathNameLen,
                                                     javacall_bool* /* OUT */ result) {
    int attrs;
    wchar_t wOsFilename[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name

    if( pathNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
        javacall_print("Error: javacall_fileconnection_is_directory(), file name is too long\n");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, pathName, pathNameLen*sizeof(wchar_t));
    wOsFilename[pathNameLen] = 0;

    attrs = GetFileAttributesW(wOsFilename);
    if(-1 == attrs) {
        javacall_print("Error: javacall_fileconnection_is_directory(), cannot get file attributes\n");
        return JAVACALL_FAIL;
    }

    *result = ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) ? JAVACALL_TRUE : JAVACALL_FALSE;
    return JAVACALL_OK;
}

/**
 * Create a directory   
 * @param dirName path name in UNICODE of directory
 * @param dirNameLen length of directory name
 * @return <tt>JAVACALL_OK</tt> success
 *         <tt>JAVACALL_FAIL</tt> fail
 */
javacall_result javacall_fileconnection_create_dir(const javacall_utf16* dirName,
                                                   int dirNameLen) {

    wchar_t wOsFilename[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name

    if( dirNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
        javacall_print("Error: javacall_fileconnection_create_dir(), file name is too long\n");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, dirName, dirNameLen*sizeof(wchar_t));
    wOsFilename[dirNameLen] = 0;

    if(0 != _wmkdir(wOsFilename)) {
        javacall_print("Error: javacall_fileconnection_create_dir(), cannot create directory\n");
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Deletes an empty directory from the persistent storage.
 *   If directory not empty this function must fail.
 * @param dirName path name in UNICODE of directory
 * @param dirNameLen length of directory name
 * @return <tt>JAVACALL_OK</tt> success
 *         <tt>JAVACALL_FAIL</tt> fail
 */
javacall_result javacall_fileconnection_delete_dir(const javacall_utf16* dirName,
                                                   int dirNameLen) {

    wchar_t wOsFilename[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name

    if( dirNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
        javacall_print("Error: javacall_fileconnection_delete_dir(), file name is too long\n");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, dirName, dirNameLen*sizeof(wchar_t));
    wOsFilename[dirNameLen] = 0;

    if(0 != _wrmdir(wOsFilename)) {
        javacall_print("Error: javacall_fileconnection_delete_dir(), cannot delete directory\n");
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Check if the directory exists in file system storage.
 * @param pathName name of directory in unicode format
 * @param pathNameLen length of directory name
 * @return <tt>JAVACALL_OK </tt> if it exists and it is a regular directory, 
 *         <tt>JAVACALL_FAIL</tt> if directory not exists or error
 */
javacall_result javacall_fileconnection_dir_exists(const javacall_utf16* pathName,
                                                   int pathNameLen) {

    javacall_bool res;
    if(JAVACALL_OK != javacall_fileconnection_is_directory(pathName, pathNameLen, &res)) {
        javacall_print("Error: javacall_fileconnection_dir_exists(), cannot access directory\n");
        return JAVACALL_FAIL;
    }

    if(JAVACALL_FALSE == res) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Renames the specified directory
 * @param oldDirName current name of file
 * @param oldDirNameLen current name length
 * @param newDirName new name of file
 * @param newDirNameLen length of new name
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_fileconnection_rename_dir(const javacall_utf16* oldDirName,
                                                   int oldDirNameLen,
                                                   const javacall_utf16* newDirName,
                                                   int newDirNameLen) {

    if(JAVACALL_OK != javacall_file_rename(oldDirName, oldDirNameLen, newDirName, newDirNameLen)) {
        javacall_print("Error: javacall_fileconnection_rename_dir(), cannot rename directory\n");
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Determines the free memory in bytes that is available on the 
 *      file system the file or directory resides on
 * @param pathName path name in UNICODE of any file within the file system
 * @param pathNameLen length of path
 * @param result returned value: on success, size of available storage space (bytes)
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_fileconnection_get_free_size(const javacall_utf16* pathName,
                                                      int pathNameLen,
                                                      javacall_int64* /* OUT */ result) {

    struct _diskfree_t df;

    if(0 != _getdiskfree(pathName[0] - (pathName[0] > 'Z' ? 'a' : 'A') + 1, &df)) {
        javacall_print("Error: javacall_fileconnection_get_free_size(), cannot get free space\n");
        return JAVACALL_FAIL;
    }

    *result = (javacall_int64)(df.avail_clusters) * df.sectors_per_cluster * df.bytes_per_sector;
    return JAVACALL_OK;
}

/**
 * Determines the total size in bytes of the file system the file 
 * or directory resides on
 * @param pathName file name in UNICODE of any file within the file system
 * @param pathNameLen length of path name
 * @param result returned value: on success, total size of storage space (bytes)
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_fileconnection_get_total_size(const javacall_utf16* pathName,
                                                       int pathNameLen,
                                                       javacall_int64* /* OUT */ result) {

    struct _diskfree_t df;

    if(0 != _getdiskfree(pathName[0] - (pathName[0] > 'Z' ? 'a' : 'A') + 1, &df)) {
        javacall_print("Error: javacall_fileconnection_get_total_size(), cannot get total space\n");
        return JAVACALL_FAIL;
    }

    *result = (javacall_int64)(df.total_clusters) * df.sectors_per_cluster * df.bytes_per_sector;
    return JAVACALL_OK;
}

/** 
 * Returns the mounted root file systems (UNICODE format). Each root must end
 * with '/' character
 * @param roots buffer to store the UNICODE string containing 
 *              currently mounted roots separated by '\n' character
 * @param rootsLen available buffer size (maximum number of javacall_utf16
 *                 symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_fileconnection_get_mounted_roots(javacall_utf16* /* OUT */ roots,
                                                          int rootsLen) {

    unsigned long driveMask;
    unsigned short ch = 'A';
    int index = 0;

    for(driveMask = _getdrives(); driveMask; driveMask >>= 1) {
        if(driveMask & 1) {
            if(index > 0 && index > rootsLen - 5 || rootsLen < 4) {
                javacall_print("Error: javacall_fileconnection_get_mounted_roots(), buffer is too small\n");
                return JAVACALL_FAIL;
            }
            if(index > 0) {
                roots[index++] = '\n';
            }
            roots[index++] = ch;
            roots[index++] = ':';
            /* roots must be in URL format, so '/' separator is used */
            roots[index++] = '/';
        }
        ch++;
    }
    roots[index] = 0;

    return JAVACALL_OK;
}

/** 
 * Returns the path to photo and other images storage, using '/' as
 * file separator. The path must end with this separator as well
 * @param dir buffer to store the UNICODE string containing path to
 *            directory with photos
 * @param dirLen available buffer size (maximum number of javacall_utf16
 *               symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_photos_dir(javacall_utf16* /* OUT */ dir,
                                       int dirLen) {
    if (dirLen < sizeof(photos_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_photos_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(dir, photos_dir, sizeof(photos_dir));

    return JAVACALL_OK;
}

/** 
 * Returns the path to video clips storage, using '/' as file separator.
 * The path must end with this separator as well
 * @param dir buffer to store the UNICODE string containing path to
 *            directory with video clips
 * @param dirLen available buffer size (maximum number of javacall_utf16
 *               symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_videos_dir(javacall_utf16* /* OUT */ dir,
                                       int dirLen) {
    if (dirLen < sizeof(videos_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_videos_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(dir, videos_dir, sizeof(videos_dir));

    return JAVACALL_OK;
}

/** 
 * Returns the path to clip art graphics storage, using '/' as file separator.
 * The path must end with this separator as well
 * @param dir buffer to store the UNICODE string containing path to
 *            directory with graphics
 * @param dirLen available buffer size (maximum number of javacall_utf16
 *               symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_graphics_dir(javacall_utf16* /* OUT */ dir,
                                         int dirLen) {
    if (dirLen < sizeof(graphics_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_graphics_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(dir, graphics_dir, sizeof(graphics_dir));

    return JAVACALL_OK;
}

/** 
 * Returns the path to ring tones and other related audio files storage,
 * using '/' as file separator. The path must end with this separator as well
 * @param dir buffer to store the UNICODE string containing path to
 *            directory with ring tones
 * @param dirLen available buffer size (maximum number of javacall_utf16
 *               symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_tones_dir(javacall_utf16* /* OUT */ dir,
                                      int dirLen) {
    if (dirLen < sizeof(tones_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_tones_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(dir, tones_dir, sizeof(tones_dir));

    return JAVACALL_OK;
}

/** 
 * Returns the path to music files storage, using '/' as file separator.
 * The path must end with this separator as well
 * @param dir buffer to store the UNICODE string containing path to
 *            directory with music
 * @param dirLen available buffer size (maximum number of javacall_utf16
 *               symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_music_dir(javacall_utf16* /* OUT */ dir,
                                      int dirLen) {
    if (dirLen < sizeof(music_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_music_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(dir, music_dir, sizeof(music_dir));

    return JAVACALL_OK;
}

/** 
 * Returns the path to voice recordings storage, using '/' as file separator.
 * The path must end with this separator as well
 * @param dir buffer to store the UNICODE string containing path to
 *            directory with voice recordings
 * @param dirLen available buffer size (maximum number of javacall_utf16
 *               symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_recordings_dir(javacall_utf16* /* OUT */ dir,
                                           int dirLen) {
    if (dirLen < sizeof(recordings_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_recordings_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(dir, recordings_dir, sizeof(recordings_dir));

    return JAVACALL_OK;
}

/** 
 * Returns the path to directory, that is used to store private directories
 * for all applications (accessed via "fileconn.dir.private" system property).
 * The returned path must use '/' as file separator and have this separator at
 * the end
 * @param dir buffer to store the UNICODE string containing path to
 *            location of private directories for all applications
 * @param dirLen available buffer size (maximum number of javacall_utf16
 *               symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_private_dir(javacall_utf16* /* OUT */ dir,
                                        int dirLen) {
    if (dirLen < sizeof(private_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_private_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(dir, private_dir, sizeof(private_dir));

    return JAVACALL_OK;
}

/** 
 * Returns the localized names for mounted root file systems (UNICODE format)
 * @param roots buffer to store the UNICODE string containing localized names
 *              of currently mounted roots separated by ';' character
 * @param rootsLen available buffer size (maximum number of javacall_utf16
 *                 symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_fileconnection_get_localized_mounted_roots(javacall_utf16* /* OUT */ roots,
                                                                    int rootsLen) {
    unsigned long driveMask;
    unsigned short ch = 'A';
    int index = 0;

    for(driveMask = _getdrives(); driveMask; driveMask >>= 1) {
        if(driveMask & 1) {
            if(index > 0 &&
               index > rootsLen - 5 - (int)(sizeof(localized_root_prefix) / sizeof(javacall_utf16) - 1) ||
               rootsLen < 4 + (sizeof(localized_root_prefix) / sizeof(javacall_utf16) - 1)) {
                javacall_print("Error: javacall_fileconnection_get_localized_mounted_roots(), buffer is too small\n");
                return JAVACALL_FAIL;
            }
            if(index > 0) {
                roots[index++] = ';';
            }
            memcpy(roots + index, localized_root_prefix, sizeof(localized_root_prefix));
            index += sizeof(localized_root_prefix) / sizeof(javacall_utf16) - 1;
            roots[index++] = ch;
            roots[index++] = ':';
            roots[index++] = javacall_get_file_separator();
        }
        ch++;
    }
    roots[index] = 0;

    return JAVACALL_OK;
}

/**
 * Returns localized name for directory with photos and other images, corresponding to
 * the path returned by <code>System.getProperty("fileconn.dir.photos")</code>.
 * @param name buffer to store the UNICODE string containing localized name
 * @param nameLen available buffer size (maximum number of javacall_utf16
 *                symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_localized_photos_dir(javacall_utf16* /* OUT */ name,
                                                 int nameLen) {
    if (nameLen < sizeof(localized_photos_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_localized_photos_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(name, localized_photos_dir, sizeof(localized_photos_dir));

    return JAVACALL_OK;
}

/**
 * Returns localized name for video clips location, corresponding to
 * the path returned by <code>System.getProperty("fileconn.dir.videos")</code>.
 * @param name buffer to store the UNICODE string containing localized name
 * @param nameLen available buffer size (maximum number of javacall_utf16
 *                symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_localized_videos_dir(javacall_utf16* /* OUT */ name,
                                                 int nameLen) {
    if (nameLen < sizeof(localized_videos_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_localized_videos_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(name, localized_videos_dir, sizeof(localized_videos_dir));

    return JAVACALL_OK;
}

/**
 * Returns localized name for directory containing clip art graphics, corresponding
 * to the path returned by <code>System.getProperty("fileconn.dir.graphics")</code>.
 * @param name buffer to store the UNICODE string containing localized name
 * @param nameLen available buffer size (maximum number of javacall_utf16
 *                symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_localized_graphics_dir(javacall_utf16* /* OUT */ name,
                                                   int nameLen) {
    if (nameLen < sizeof(localized_graphics_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_localized_graphics_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(name, localized_graphics_dir, sizeof(localized_graphics_dir));

    return JAVACALL_OK;
}

/**
 * Returns localized name for directory with ring tones and other related audio files,
 * corresponding to the path returned by
 * <code>System.getProperty("fileconn.dir.tones")</code>.
 * @param name buffer to store the UNICODE string containing localized name
 * @param nameLen available buffer size (maximum number of javacall_utf16
 *                symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_localized_tones_dir(javacall_utf16* /* OUT */ name,
                                                int nameLen) {
    if (nameLen < sizeof(localized_tones_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_localized_tones_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(name, localized_tones_dir, sizeof(localized_tones_dir));

    return JAVACALL_OK;
}

/**
 * Returns localized name for music files storage, corresponding to
 * the path returned by <code>System.getProperty("fileconn.dir.music")</code>.
 * @param name buffer to store the UNICODE string containing localized name
 * @param nameLen available buffer size (maximum number of javacall_utf16
 *                symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_localized_music_dir(javacall_utf16* /* OUT */ name,
                                                int nameLen) {
    if (nameLen < sizeof(localized_music_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_localized_music_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(name, localized_music_dir, sizeof(localized_music_dir));

    return JAVACALL_OK;
}

/**
 * Returns localized name for voice recordings storage, corresponding to
 * the path returned by <code>System.getProperty("fileconn.dir.recordings")</code>.
 * @param name buffer to store the UNICODE string containing localized name
 * @param nameLen available buffer size (maximum number of javacall_utf16
 *                symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_localized_recordings_dir(javacall_utf16* /* OUT */ name,
                                                     int nameLen) {
    if (nameLen < sizeof(localized_recordings_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_localized_recordings_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(name, localized_recordings_dir, sizeof(localized_recordings_dir));

    return JAVACALL_OK;
}

/**
 * Returns localized private directory name corresponding to the path returned by
 * <code>System.getProperty("fileconn.dir.private")</code>.
 * @param name buffer to store the UNICODE string containing localized name
 * @param nameLen available buffer size (maximum number of javacall_utf16
 *                symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_fileconnection_get_localized_private_dir(javacall_utf16* /* OUT */ name,
                                                  int nameLen) {
    if (nameLen < sizeof(localized_private_dir) / sizeof(javacall_utf16)) {
        javacall_print("Error: javacall_fileconnection_get_localized_private_dir(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    memcpy(name, localized_private_dir, sizeof(localized_private_dir));

    return JAVACALL_OK;
}


/**
 * Returns OS-specific path for the specified file system root.
 * @param rootName Root name in UNICODE
 * @param rootNameLen length of rootName
 * @param pathName buffer to store the UNICODE string containing 
 *                 the system-dependent path to access the specified 
 *                 root
 * @param pathNameLen available buffer size (maximum number of javacall_utf16
 *                    symbols to be stored) 
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */ 
javacall_result javacall_fileconnection_get_path_for_root(const javacall_utf16* rootName,
                                                          int rootNameLen,
                                                          javacall_utf16* /* OUT */ pathName,
                                                          int pathNameLen) {

    if(pathNameLen < rootNameLen + 1) {
        javacall_print("Error: javacall_fileconnection_get_path_for_root(), buffer is too small\n");
        return JAVACALL_FAIL;
    }

    pathName[rootNameLen] = 0;
    pathName[--rootNameLen] = javacall_get_file_separator();
    while(--rootNameLen >= 0) {
        pathName[rootNameLen] = rootName[rootNameLen];
    }
    return JAVACALL_OK;
}


#define MAX_DIRECTORY_NESTING_LEVEL 50

/**
 * Get size in bytes of all files and possibly subdirectories contained 
 * in the specified dir.
 *
 * @param pathName          path name in UNICODE of directory
 * @param pathNameLen       length of path name
 * @param includeSubdirs    if JAVACALL_TRUE include subdirectories size too,
 *                          if JAVACALL_FALSE do not include subdirectories
 * @param result            returned value: size in bytes of all files contained in 
 *                          the specified directory and possibly its subdirectories
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */ 
javacall_result javacall_fileconnection_dir_content_size(const javacall_utf16* pathName,
                                                         int pathNameLen,
                                                         javacall_bool includeSubdirs,
                                                         javacall_int64* /* OUT */ result) {
    wchar_t subSearch[JAVACALL_MAX_FILE_NAME_LENGTH + 3];
    WIN32_FIND_DATAW dir_data;
    javacall_int64 contentSize = 0;
    HANDLE listHandle[MAX_DIRECTORY_NESTING_LEVEL];
    int pathLen[MAX_DIRECTORY_NESTING_LEVEL];
    int nestLevel = 0;
    int nextExists = 1;

    memcpy(subSearch, pathName, pathNameLen * sizeof(javacall_utf16));
    subSearch[pathNameLen++] = javacall_get_file_separator();
    subSearch[pathNameLen++] = '*';
    subSearch[pathNameLen] = 0;

    listHandle[0] = FindFirstFileW(subSearch, &dir_data);
    pathLen[0] = pathNameLen - 1;
    if (INVALID_HANDLE_VALUE == listHandle[0]) {
        javacall_print("Error: javacall_fileconnection_dir_content_size(), cannot open directory\n");
        return JAVACALL_FAIL;
    }

    for ( ; ; ) {
        while (nextExists) {
            if ((dir_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                // found subdirectory
                if (JAVACALL_TRUE == includeSubdirs) {
                    // must count subdirectory sizes
                    int dirNameLen = wcslen(dir_data.cFileName);
                    if (wcscmp(dir_data.cFileName, L".") && wcscmp(dir_data.cFileName, L"..")) {
                        // the subdirectory is not "." or ".."
                        if (nestLevel >= MAX_DIRECTORY_NESTING_LEVEL - 1) {
                            // nesting level overflow
                            while (nestLevel >= 0) {
                                FindClose(listHandle[nestLevel--]);
                            }
                            javacall_print("Error: javacall_fileconnection_dir_content_size(), directory nesting overflow\n");
                            return JAVACALL_FAIL;
                        }
                        subSearch[pathLen[nestLevel]] = 0;
                        wcscat(subSearch, dir_data.cFileName);
                        pathLen[nestLevel + 1] = pathLen[nestLevel] + dirNameLen;
                        subSearch[pathLen[++nestLevel]++] = javacall_get_file_separator();
                        subSearch[pathLen[nestLevel]] = '*';
                        subSearch[pathLen[nestLevel] + 1] = 0;
                        listHandle[nestLevel] = FindFirstFileW(subSearch, &dir_data);
                        if (INVALID_HANDLE_VALUE == listHandle[nestLevel]) {
                            while (--nestLevel >= 0) {
                                FindClose(listHandle[nestLevel]);
                            }
                            javacall_print("Error: javacall_fileconnection_dir_content_size(), cannot open subdirectory\n");
                            return JAVACALL_FAIL;
                        }
                        nextExists = 1;
                        continue;
                    }
                }
            } else {
                contentSize += ((javacall_int64)(dir_data.nFileSizeHigh) << 32) + dir_data.nFileSizeLow;
            }
            nextExists = FindNextFileW(listHandle[nestLevel], &dir_data);
        }
        FindClose(listHandle[nestLevel]);
        if (nestLevel > 0) {
            nextExists = FindNextFileW(listHandle[--nestLevel], &dir_data);
        } else {
            break;
        }
    }

    *result = contentSize;
    return JAVACALL_OK;
}


/**
 * Callback function for mount_timer event. Checks if any drives were
 * mounted or unmounted, calls javanotify_file_root_changed() if it occured.
 */
static void mount_timer_callback(javacall_handle handle) {
    unsigned long newRoots = _getdrives();
    if(newRoots != oldRoots) {
        oldRoots = newRoots;
        javanotify_fileconnection_root_changed();
    }
}

#ifdef __cplusplus
}
#endif
