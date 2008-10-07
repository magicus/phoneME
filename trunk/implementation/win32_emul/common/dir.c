/*
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

/**
 * @file
 *
 * win32 implemenation for dir javacall functions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <wchar.h>
#include <sys/stat.h>

#include "file.h"
#include "javacall_dir.h"
#include "javacall_file.h"
#include "javacall_logging.h"
#include "javacall_properties.h"

static unsigned short FILESEP = '\\';
static unsigned short PATHSEP = ';';

#define DEFAULT_MAX_JAVA_SPACE (20*1024*1024)

WIN32_FIND_DATAW javacall_dir_data;
BOOL javacall_dir_first_time;

/**
 * returns a handle to a file list. This handle can be used in
 * subsequent calls to javacall_dir_get_next() to iterate through
 * the file list and get the names of files that match the given string.
 *
 * @param path the name of a directory, but it can be a
 *             partial file name
 * @param pathLen length of directory name
 * @return pointer to an opaque filelist structure, that can be used in
 *         javacall_dir_get_next() and javacall_dir_close
 *         NULL returned on error, for example if root directory of the
 *         input 'string' cannot be found.
 */
javacall_handle javacall_dir_open(const javacall_utf16* path,
                                  int pathLen) {

  javacall_handle handle;
    wchar_t wOsPath[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name

    if( pathLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
 	 javautil_debug_print (JAVACALL_LOG_ERROR, "dir", "javacall_dir_open: Path length too long (%ws,%d)\n",path,pathLen);
        return NULL;
    }

    memset(wOsPath, 0, JAVACALL_MAX_FILE_NAME_LENGTH*sizeof(wchar_t));
    memcpy(wOsPath, path, pathLen*sizeof(wchar_t));

    wOsPath[pathLen++] = javacall_get_file_separator();
    wOsPath[pathLen++] = '*';
    wOsPath[pathLen++] = 0;

  // DEBUG  printf("dir open: (%ws) \n", wOsPath);

  handle = FindFirstFileW(wOsPath, &javacall_dir_data);

  if (handle == INVALID_HANDLE_VALUE) {
    return NULL;
  }

  javacall_dir_first_time = TRUE;

  return handle;
}

void javacall_dir_close(javacall_handle handle){

  if ((handle != NULL) &&
      (handle != INVALID_HANDLE_VALUE)) {
    FindClose(handle);
  }

}

/**
 * Returns the next filename in directory path (UNICODE format).
 * The order is defined by the underlying file system. Current and
 * parent directory links ("." and "..") must not be returned.
 * This function must behave correctly (e.g. not skip any existing files)
 * even if some files are deleted from the directory between subsequent
 * calls to <code>javacall_dir_get_next()</code>.
 * 
 * On success, the resulting file will be copied to user supplied buffer.
 * The filename returned will omit the file's path
 * 
 * @param handle pointer to filelist struct returned by javacall_dir_open
 * @param outFilenameLength will be filled with number of chars written 
 * 
 * 
 * @return pointer to UNICODE string for next file on success, 0 otherwise
 * returned param is a pointer to platform specific memory block
 * platform MUST BE responsible for allocating and freeing it.
 */
javacall_utf16* javacall_dir_get_next(javacall_handle handle,
                                        int* /*OUT*/ outFilenameLength) {
    if (handle == NULL) {
        if (outFilenameLength) {
            *outFilenameLength = 0;
        }
        return NULL;
    }
    if (!javacall_dir_first_time) {
        if (FindNextFileW(handle, &javacall_dir_data) == 0) {
            if (outFilenameLength) {
                *outFilenameLength = 0;
            }
            return NULL;
        }
    }
    javacall_dir_first_time = FALSE;

    while (wcscmp(javacall_dir_data.cFileName, L".") == 0
        || wcscmp(javacall_dir_data.cFileName, L"..") == 0) {
        if (FindNextFileW(handle, &javacall_dir_data) == 0) {
            if (outFilenameLength) {
                *outFilenameLength = 0;
            }
            return NULL;
        }
    }

    if (outFilenameLength) {
        *outFilenameLength = wcslen((wchar_t *)javacall_dir_data.cFileName);
    }
    return (javacall_dir_data.cFileName);
}


/**
 * Internal Helper function -
 *
 * Checks the size of used space in storage.
 * @param unicodeSystemDirName name of root directory
 * @param systemDirNameLen length of root directory name
 * @return size of used space
 * */

/**
 * directory flag
 */
#define S_ISDIR(mode) ( ((mode) & S_IFMT) == S_IFDIR )



long internal_get_used_helper(const javacall_utf16* unicodeSystemDirName,
                              int systemDirNameLen) {

          long used = 0;
          javacall_handle pIterator;
          javacall_utf16 *current;
          int currentLen;
          wchar_t wOsFilename[JAVACALL_MAX_FILE_NAME_LENGTH]; // max file name

          struct _stat stat_buf;

          pIterator = javacall_dir_open(unicodeSystemDirName, systemDirNameLen);
          for (; ; ) {
              if ((current = javacall_dir_get_next(pIterator, &currentLen)) == NULL) {
                      break;
              }

              if( currentLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
	 	      javautil_debug_print (JAVACALL_LOG_ERROR, "dir", "internal_get_used_helper: File name length too big");
                    return JAVACALL_FAIL;
              }

              memcpy(wOsFilename, current, currentLen*sizeof(wchar_t));
              wOsFilename[currentLen] = 0;


              /* Don't count the subdirectories "." and ".." */
              if (_wstat(wOsFilename, &stat_buf) != -1 &&
                  !S_ISDIR(stat_buf.st_mode)) {
                      used += stat_buf.st_size;
              }


          }

          javacall_dir_close(pIterator);
          return used;
      }


/**
 * Checks the size of free space in storage.
 * @return size of free space
 */
javacall_int64 javacall_dir_get_free_space_for_java(void) {

    long result;
    int rootPathLen = JAVACALL_MAX_FILE_NAME_LENGTH;
    javacall_utf16 rootPath[JAVACALL_MAX_FILE_NAME_LENGTH];
    javacall_int64 i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes, max_storage_size;
    char root[JAVACALL_MAX_FILE_NAME_LENGTH];
    javacall_result  res;
    int i;

    res = javacall_dir_get_root_path(rootPath, &rootPathLen);

    for (i=0; i<rootPathLen; i++) {
        root[i] = (char)rootPath[i];
    }
    root[i] = '\0';

    result = GetDiskFreeSpaceEx(root,
                                (PULARGE_INTEGER)&i64FreeBytesToCaller,
                                (PULARGE_INTEGER)&i64TotalBytes,
                                (PULARGE_INTEGER)&i64FreeBytes);

    if (result) {
        char *storage_size;
        javacall_get_property("system.jam_space", 
                            JAVACALL_APPLICATION_PROPERTY,
                            &storage_size );

        max_storage_size = (storage_size==NULL)?DEFAULT_MAX_JAVA_SPACE:atoi(storage_size);

        if (i64FreeBytesToCaller > max_storage_size) {
            return max_storage_size;
        }
        return i64FreeBytesToCaller;
    }

  return 0;
}

/*
 * Make sure:
 * "system.default_storage" points to the device appdb
 * "system.default_storage" points to the shared appdb (old 'lib')
 * "com.sun.midp.publickeystore.WebPublicKeyStore" points to the keystore
 */
javacall_result helper_normalize_properties() {
    unsigned short appdbStr[]={'\\','a','p','p','d','b','\\',0};
    unsigned short oldKS[]={'_','m','a','i','n','.','k','s',0};
    unsigned short newKS[]={'_','m','a','i','n','.','m','k','s',0};

    static BOOL firstTime = TRUE;
    javacall_utf16 buffer_w[JAVACALL_MAX_FILE_NAME_LENGTH];
    int buffer_len = 0;

    char* storage_root;
    int storage_root_ok = 0;

    char* default_storage; 
    int default_storage_ok = 0;
    
    char* sdk_version;

    if (!firstTime) {
        return JAVACALL_OK;
    }

    javacall_get_property("system.storage_root", 
                                  JAVACALL_APPLICATION_PROPERTY, 
                                  &storage_root); /* device name on wtk < 2.5.2, full path otherwise*/

    javacall_get_property("system.default_storage",
                                      JAVACALL_APPLICATION_PROPERTY,
                                      &default_storage); /* appdb in WTK-multiuser-environment (2.5.2 and up) */

    if (default_storage == NULL ) { /* Backward compatibility (WTK Version < 2.5.2)
                                       Storage root is relative */
            javacall_utf16 storage_root_w[JAVACALL_MAX_FILE_NAME_LENGTH];
            javacall_utf16 key_place_w[JAVACALL_MAX_FILE_NAME_LENGTH];

            if ( _wgetcwd( storage_root_w, JAVACALL_MAX_FILE_NAME_LENGTH ) == NULL) {
                javautil_debug_print (JAVACALL_LOG_ERROR, "dir", "javacall_dir_get_root_path:  _wgetcwd failed");
                return JAVACALL_FAIL;
            } else {
                wcscat(storage_root_w, appdbStr);
                wcscpy(key_place_w,storage_root_w);

                javacall_set_property("system.default_storage", 
                           unicode_to_char(storage_root_w), 
                           JAVACALL_TRUE,
                           JAVACALL_APPLICATION_PROPERTY);

                wcscat(key_place_w,oldKS);
                javacall_set_property("com.sun.midp.publickeystore.WebPublicKeyStore", 
                           unicode_to_char(key_place_w), 
                           JAVACALL_TRUE,
                           JAVACALL_INTERNAL_PROPERTY);

                 wcscat(storage_root_w, char_to_unicode(storage_root));

                /* Set the system.storage_root to full path */
                javacall_set_property("system.storage_root", 
                                      unicode_to_char(storage_root_w), 
                                      JAVACALL_TRUE,
                                      JAVACALL_APPLICATION_PROPERTY);

                firstTime = FALSE;
                return JAVACALL_OK;
            }
    }
    
    javacall_get_property("sdk_version",
                                      JAVACALL_APPLICATION_PROPERTY,
                                      &sdk_version);

    if (sdk_version == NULL) {
        /* WTK version is >= 2.5.2, storage_root is absolute path */
        wcscpy(buffer_w, char_to_unicode(default_storage));
        wcscat(buffer_w, newKS);
    } else {
        /* Java ME SDK version is >= 3.0 */
        unsigned short separator[] = {'\\', 0};
        wcscpy(buffer_w, char_to_unicode(storage_root));
        wcscat(buffer_w, separator);
        wcscat(buffer_w, oldKS);
    }
    
    javacall_set_property("com.sun.midp.publickeystore.WebPublicKeyStore", 
               unicode_to_char(buffer_w), 
               JAVACALL_TRUE,
               JAVACALL_INTERNAL_PROPERTY);

    firstTime = FALSE;
    return JAVACALL_OK;
}


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
                                           int* /*IN | OUT*/ rootPathLen) {

    char* storage_root;
    javacall_result res;

    res = helper_normalize_properties();
    if (res != JAVACALL_OK)
        return res;

    javacall_get_property("system.storage_root", 
                                  JAVACALL_APPLICATION_PROPERTY, 
                                  &storage_root); /* device name on wtk < 2.5.2, full path otherwise*/

    wcscpy(rootPath, char_to_unicode(storage_root));
    *rootPathLen = wcslen (rootPath);
    return JAVACALL_OK;
}

javacall_result javacall_dir_get_config_path(javacall_utf16* /* OUT */ configPath,
                                           int* /*IN | OUT*/ configPathLen) {
    char* default_storage; 
    javacall_result res;

    res = helper_normalize_properties();
    if (res != JAVACALL_OK)
        return res;

    javacall_get_property("system.default_storage",
                                      JAVACALL_APPLICATION_PROPERTY,
                                      &default_storage); /* appdb in WTK-multiuser-environment (2.5.2 and up) */

    /* If the function was called before, 
       system.storage_root is fixed, and can be used as the result
     */
    wcscpy(configPath, char_to_unicode(default_storage));
    *configPathLen = wcslen (configPath);
    return JAVACALL_OK;
}


/**
 *  Returns file separator character used by the underlying file system
 * (usually this function will return '\\';)
 * @return 16-bit Unicode encoded file separator
 */
javacall_utf16 javacall_get_file_separator(void) {

    return (javacall_utf16)'\\';
}


/**
 * Check if the given path is located on secure storage
 * The function should return JAVACALL_TRUE only in the given path
 * is located on non-removable storage, and cannot be accessed by the
 * user or overwritten by unsecure applications.
 * @return <tt>JAVACALL_TRUE</tt> if the given path is guaranteed to be on
 *         secure storage
 *         <tt>JAVACALL_FALSE</tt> otherwise
 */
javacall_bool javacall_dir_is_secure_storage(javacall_utf16* classPath,
                                             int pathLen) {
   return JAVACALL_FALSE;
}


#ifdef __cplusplus
}
#endif

