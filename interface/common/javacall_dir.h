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

#ifndef __JAVACALL_DIR_H
#define __JAVACALL_DIR_H

/**
 * @file javacall_dir.h
 * @ingroup MandatoryDirectory
 * @brief Javacall interfaces for directory
 */

#include "javacall_defs.h" 

#ifdef __cplusplus
extern "C" {
#endif
    

/**
 * @defgroup MandatoryDirectory Directory API
 * @ingroup JTWI
 *
 * The following functions specify how iterate through all files in a 
 * given directory.
 * The following functionality is provided:
 * 
 * - Open a directory file list: by calling function javacall_dir_open
 * - Get next file name in directory file list: calling javacall_dir_get_next
 * - Close directory file list: By calling javacall_dir_close
 * - Getting total free storage space available to Java
 * - Getting total storage space used by Java files
 *
 * @{
 */
        
/**
 * Returns a handle to a file list. This handle can be used in
 * subsequent calls to <code>javacall_dir_get_next()</code> to iterate 
 * through the file list and get the names of files that match 
 * the given string. 
 * This function must support repeated calls from the Java 
 * layer, which means the handle returned would not be affected
 * by subsequent calls to <code>javacall_dir_open()</code> and 
 * the file list/iterator will not be affected. 
 *  
 * 
 * @param path the name of a directory, but it can be a
 *             partial file name
 * @param pathLen length of directory name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return pointer to an opaque filelist structure, that can be used in
 *         javacall_dir_get_next() and javacall_dir_close
 *         NULL returned on error, for example if root directory of the
 *         input 'string' cannot be found.
 */
javacall_handle javacall_dir_open(const javacall_utf16* path,
                                  int pathLen);
    
/**
 * closes the specified file list. This handle will no longer be
 * associated with this file list.
 * @param handle pointer to opaque filelist struct returned by
 *               javacall_dir_open 
 */
void javacall_dir_close(javacall_handle handle);
    
/**
 * Returns the next filename or sub directory in the directory
 * path (in UNICODE format). The order is defined by the 
 * underlying file system. Current and parent directory links 
 * ("." and "..") must not be returned. This function must 
 * behave correctly (e.g. not skip any existing files) even if 
 * some files are deleted from the directory, or 
 * <code>javacall_dir_open()</code> is called between 
 * subsequent calls to <code>javacall_dir_get_next()</code>. 
 * 
 * On success, the resulting file will be copied to user supplied buffer.
 * The filename returned will omit the file's path
 * 
 * This function must not distinguish files and directories, 
 * regardless of native file system behavior. 
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
                                        int* /*OUT*/ outFilenameLength);
    
/**
 * Checks the size of free space in storage. (for Java )
 * @return size of free space for java
 */
javacall_int64 javacall_dir_get_free_space_for_java(void);
    
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
                                           int* /* IN | OUT */ rootPathLen);

/**
 * Returns the path of java's configuration directory.
 * 
 * @param configPath returned value: pointer to unicode buffer, allocated 
 *        by the VM, to be filled with the root path.
 * @param configPathLen IN  : lenght of max rootPath buffer
 *                    OUT : lenght of set rootPath
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */
javacall_result javacall_dir_get_config_path(javacall_utf16* /* OUT */ configPath,
                                                    int* /* IN | OUT */ configPathLen);


/**
 *  Returns file separator character used by the underlying file system
 * (usually this function will return '\\';)
 * @return 16-bit Unicode encoded file separator
 */
javacall_utf16 javacall_get_file_separator(void);


/**
 * Check if the given path is located on secure storage
 * The function should return JAVACALL_TRUE only in the given path
 * is located on non-removable storage, and cannot be accessed by the 
 * user or overwritten by unsecure applications.
 * @return <tt>JAVACALL_TRUE</tt> if the given path is guaranteed to be on 
 *         secure storage
 *         <tt>JAVACALL_FALSE</tt> otherwise
 */
javacall_bool /* OPTIONAL*/  javacall_dir_is_secure_storage(javacall_utf16* classPath, 
                                                            int pathLen);

/** @} */
    
#ifdef __cplusplus
}
#endif

#endif 

