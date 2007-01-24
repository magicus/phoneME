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

#ifndef __JAVACALL_JSR75_FC_H_
#define __JAVACALL_JSR75_FC_H_

/**
 * @file javacall_fileconnection.h
 * @ingroup JSR75
 * @brief Javacall interfaces for JSR-75 FileConnection
 */

#include "javacall_defs.h"


#define JAVACALL_MAX_LOCALIZED_ROOTS_LIST_LENGTH  512
#define JAVACALL_MAX_LOCALIZED_DIR_NAME_LENGTH    512

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup JSR75 JSR75 File Connection API
 *
 * The following API definitions are required by JSR-75.
 * These APIs are not required by standard JTWI implementations.
 *
 * @{
 */

/** 
 * @defgroup jsrMandatoryFileConnection Mandatory FC API
 * @ingroup JSR75
 *
 * @{
 */ 

/*
 * 
 *             JSR075's FileConnection API
 *            =============================
 * 
 * 
 * The following API definitions are required by JSR075.
 * These APIs are not required by standard JTWI implementations.
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

/**
 * Makes all the required initializations for JSR 75 FileConnection 
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured or feature is not supported
 */
javacall_result javacall_fileconnection_init(void);

/**
 * Cleans up resources used by fileconnection 
 * @return JAVACALL_OK on success, JAVACALL_FAIL otherwise
 */
javacall_result javacall_fileconnection_finalize(void);

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
javacall_result  
javacall_fileconnection_is_hidden(const javacall_utf16* fileName,
                                  int fileNameLen,
                                  javacall_bool* /* OUT */ result);

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
javacall_result 
javacall_fileconnection_is_readable(const javacall_utf16* pathName,
                                    int pathNameLen,
                                    javacall_bool* /* OUT */ result);

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
javacall_result 
javacall_fileconnection_is_writable(const javacall_utf16* pathName,
                                    int pathNameLen,
                                    javacall_bool* /* OUT */ result);

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
javacall_result 
javacall_fileconnection_set_hidden(const javacall_utf16* fileName,
                                   int fileNameLen,
                                   javacall_bool value);

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
javacall_result  
javacall_fileconnection_set_readable(const javacall_utf16* pathName,
                                     int pathNameLen,
                                     javacall_bool value);

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
javacall_result 
javacall_fileconnection_set_writable(const javacall_utf16* pathName,
                                     int pathNameLen,
                                     javacall_bool value);    

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
javacall_result 
javacall_fileconnection_get_last_modified(const javacall_utf16* fileName, 
                                          int fileNameLen, 
                                          javacall_int64* /* OUT */ result);


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
javacall_result
javacall_fileconnection_get_illegal_filename_chars(javacall_utf16* /* OUT */ illegalChars,
                                                   int illegalCharsLenMaxLen);




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
javacall_result 
javacall_fileconnection_is_directory(const javacall_utf16* pathName, 
                                     int pathNameLen,
                                     javacall_bool* /* OUT */ result);

/**
 * Create a directory   
 * @param dirName path name in UNICODE of directory
 * @param dirNameLen length of directory name
 * @return <tt>JAVACALL_OK</tt> success
 *         <tt>JAVACALL_FAIL</tt> fail
 */
javacall_result 
javacall_fileconnection_create_dir(const javacall_utf16* dirName,
                                   int dirNameLen);

/**
 * Deletes an empty directory from the persistent storage.
 *   If directory not empty this function must fail.
 * @param dirName path name in UNICODE of directory
 * @param dirNameLen length of directory name
 * @return <tt>JAVACALL_OK</tt> success
 *         <tt>JAVACALL_FAIL</tt> fail
 */
javacall_result 
javacall_fileconnection_delete_dir(const javacall_utf16* dirName,
                                   int dirNameLen);

/**
 * Check if the directory exists in file system storage.
 * @param pathName name of directory in unicode format
 * @param pathNameLen length of directory name
 * @return <tt>JAVACALL_OK </tt> if it exists and it is a regular directory, 
 *         <tt>JAVACALL_FAIL</tt> if directory not exists or error
 */
javacall_result 
javacall_fileconnection_dir_exists(const javacall_utf16* pathName, 
                                   int pathNameLen);


/**
 * Renames the specified directory
 * @param oldDirName current name of file
 * @param oldDirNameLen current name length
 * @param newDirName new name of file
 * @param newDirNameLen length of new name
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result 
javacall_fileconnection_rename_dir(const javacall_utf16* oldDirName,
                                   int oldDirNameLen, 
                                   const javacall_utf16* newDirName, 
                                   int newDirNameLen);    

/**
 * Determines the free memory in bytes that is available on the 
 *      file system the file or directory resides on
 * @param pathName path name in UNICODE of any file within the file system
 * @param pathNameLen length of path
 * @param result returned value: on success, size of available storage space (bytes)
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result 
javacall_fileconnection_get_free_size(const javacall_utf16* pathName, 
                                      int pathNameLen,
                                      javacall_int64* /* OUT */ result);

/**
 * Determines the total size in bytes of the file system the file 
 * or directory resides on
 * @param pathName file name in UNICODE of any file within the file system
 * @param pathNameLen length of path name
 * @param result returned value: on success, total size of storage space (bytes)
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result  
javacall_fileconnection_get_total_size(const javacall_utf16* pathName,
                                       int pathNameLen,
                                       javacall_int64* /* OUT */ result);

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
javacall_result
javacall_fileconnection_get_mounted_roots(javacall_utf16* /* OUT */ roots,
                                          int rootsLen);

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
                                       int dirLen);

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
                                       int dirLen);

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
                                         int dirLen);

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
                                      int dirLen);

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
                                      int dirLen);

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
                                           int dirLen);

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
                                        int dirLen);

/** 
 * Returns the localized names for mounted root file systems (UNICODE format)
 * @param roots buffer to store the UNICODE string containing localized names
 *              of currently mounted roots separated by ';' character
 * @param rootsLen available buffer size (maximum number of javacall_utf16
 *                 symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_fileconnection_get_localized_mounted_roots(javacall_utf16* /* OUT */ names,
                                                    int namesLen);

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
                                                 int nameLen);

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
                                                 int nameLen);

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
                                                   int nameLen);

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
                                                int nameLen);

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
                                                int nameLen);

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
                                                     int nameLen);

/**
 * Returns localized private directory name corresponding to the path returned by
 * <code>System.getProperty("fileconn.dir.private")</code>.
 * @param name buffer to store the UNICODE string containing localized name
 * @param nameLen available buffer size (maximum number of javacall_utf16
 *                symbols to be stored)
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result  
javacall_fileconnection_get_localized_private_dir(javacall_utf16* /* OUT */ name,
                                                  int nameLen);

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
javacall_result 
javacall_fileconnection_get_path_for_root(const javacall_utf16* rootName,
                                          int rootNameLen,
                                          javacall_utf16* /* OUT */ pathName,
                                          int pathNameLen);

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
javacall_result 
javacall_fileconnection_dir_content_size(const javacall_utf16* pathName,
                                         int pathNameLen,
                                         javacall_bool includeSubdirs,
                                         javacall_int64* /* OUT */ result);

/** @} */

/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - -  - - - - - - -  
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurence of the
  undelying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/

/** 
 * @defgroup NotificationFileConnection Notification API for JSR-75 FC
 * @ingroup JSR75
 * @{
 */ 

/**
 * A callback function to be called by the platfrom in order to notify
 * about changes in the available file system roots (new root was added/
 * a root on removed).
 */
void javanotify_fileconnection_root_changed(void);



/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif 


