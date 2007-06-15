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

#ifndef __JAVACALL_FILE_H
#define __JAVACALL_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file javacall_file.h
 * @ingroup MandatoryFile
 * @brief Javacall interfaces for file
 */
    
#include "javacall_defs.h" 
 
/**
 * @defgroup MandatoryFile File API
 * @ingroup JTWI
 *
 * File System APIs define the standatd POSIX file system functionality. 
 * This functionality includes reading and writing from files and 
 * accessing file's properties:
 * - Accesing file contents:
 *    - file open
 *    - file close
 *    - file read
 *    - file write
 *    - file seek
 * - Accessing file's properties
 *    - rename file
 *    - delete file
 *    - truncate file
 *    - sizeof file
 *    - file exist
 * 
 * 
 * <b>Unicode support.</b> Note that file names defined in Java are encoded in
 * UTF-16, and long file names are supported. Most devices requires conversion
 * of UTF-16 filenames to some 8-bit encoding (e.g. UTF-8), or wide char encoded
 * representation with limited number of characters.
 * These gaps need to be implemented by the platform. Note that utilities 
 * for this conversion can be supplied by Sun.
 *
 * @{
 */

/**
 * File system Open control flags, to be passed to javacall_file_open().
 */
    
/** Open the file for reading only. */
#define JAVACALL_FILE_O_RDONLY  0x00

/** Open the file for writing only. */
#define JAVACALL_FILE_O_WRONLY  0x01

/** Open the file for reading and writing. */
#define JAVACALL_FILE_O_RDWR    0x02

/** Create the file if it does not exist. */
#define JAVACALL_FILE_O_CREAT   0x40

/**
 * If the file exists and is a regular file, and the file
 * is successfully opened with JAVACALL_FILE_O_RDWR or JAVACALL_FILE_O_WRONLY,
 * its length is truncated to 0.
 */
#define JAVACALL_FILE_O_TRUNC   0x200

/**
 * If set, the file offset is set to the end of the file
 * immediately after opening. Writing will append to the 
 * existing contents of a file.
 */
#define JAVACALL_FILE_O_APPEND  0x400

/**
 * @enum javacall_file_seek_flags
 * @brief Seek flags
 */
typedef enum {
    /** Seek from the start of file position */
    JAVACALL_FILE_SEEK_SET =0,
    /** Seek from the current of file position */
    JAVACALL_FILE_SEEK_CUR =1,
    /** Seek from the end of file position */
    JAVACALL_FILE_SEEK_END =2
} javacall_file_seek_flags;

/**
 * Initializes the file system.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_init(void);
    
/**
 * Cleans up resources used by file system.
 *
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_finalize(void);
    
/**
 * Opens a file.
 *
 * @param fileName path name of the file to be opened.
 * @param flags open control flags.
 *        Applications must specify exactly one of the first three
 *        values (file access modes) below in the value of "flags":
 *        JAVACALL_FILE_O_RDONLY, 
 *        JAVACALL_FILE_O_WRONLY, 
 *        JAVACALL_FILE_O_RDWR
 *
 *        and any combination (bitwise-inclusive-OR) of the following:
 *        JAVACALL_FILE_O_CREAT, 
 *        JAVACALL_FILE_O_TRUNC, 
 *        JAVACALL_FILE_O_APPEND,
 *
 * @param handle pointer to store the file identifier.
 *        On successful completion, file identifier is returned in this 
 *        argument. This identifier is platform specific and is opaque
 *        to the caller.  
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 * 
 */
javacall_result javacall_file_open(javacall_const_utf16_string fileName,
                                   int flags,
                                   javacall_handle* /* OUT */ handle);

/**
 * Closes the file with the specified handle.
 *
 * @param handle identifier of the file to be closed.
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_close(javacall_handle handle);

/**
 * Reads the specified number of bytes from a file.
 *
 * @param handle identifier of the file.
 * @param buf buffer to store the data being read.
 * @param size number of bytes to be read. Actual number of bytes
 *             read may be less, if an end-of-file is encountered.
 * @return the number of bytes actually read.
 */
long javacall_file_read(javacall_handle handle, 
                        unsigned char *buf,
                        long size);

/**
 * Writes bytes to a file.
 *
 * @param handle identifier of the file.
 * @param buf buffer to be written.
 * @param size number of bytes to write.
 * @return the number of bytes actually written. This is normally the same 
 *         as size, but might be less (for example, if the persistent 
 *         storage being written to fills up).
 */
long javacall_file_write(javacall_handle handle,
                         const unsigned char* buf,
                         long size);

/**
 * Deletes a file from persistent storage.
 *
 * @param fileName name of file to be deleted
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_delete(javacall_const_utf16_string fileName);

/**
 * Truncates an open file in file system storage.
 *
 * @param handle identifier of the file to be truncated, as returned
 *               by javacall_file_open().
 * @param size size to truncate to.
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_truncate(javacall_handle handle,
                                       javacall_int64 size);

/**
 * Sets the file pointer associated with the file identifier.
 *
 * @param handle identifier of the file, as returned by javacall_file_open().
 * @param offset number of bytes to offset file position by.
 * @param flag controls where offset is applied from. Possible values:
 *                 JAVACALL_FILE_SEEK_SET - from the beginning,
 *                 JAVACALL_FILE_SEEK_CUR - from the current position,
 *                 JAVACALL_FILE_SEEK_END - from the end of the file.
 * @return on success, the actual resulting offset from the beginning of the file,
 *         -1 otherwise.
 */
javacall_int64 javacall_file_seek(javacall_handle handle,
                        javacall_int64 offset,
                        javacall_file_seek_flags flag); 

/**
 * Returns the size of an open file.
 *
 * @param handle identifier of the file, as returned by javacall_file_open().
 * @return size of file in bytes if successful, -1 otherwise.
 */
javacall_int64 javacall_file_sizeofopenfile(javacall_handle handle);

/**
 * Returns the file size.
 *
 * @param fileName name of the file.
 * @return size of file in bytes if successful, -1 otherwise 
 */
javacall_int64 javacall_file_sizeof(javacall_const_utf16_string fileName);

/**
 * Checks if the file exists in file system storage.
 *
 * @param fileName name of the file.
 * @return <tt>JAVACALL_OK </tt> if it exists and is a regular file, 
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_exist(javacall_const_utf16_string fileName);

/** 
 * Forces the data to be written into the file system storage.
 *
 * @param handle identifier of the file, as returned by javacall_file_open().
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_flush(javacall_handle handle);

/**
 * Renames the file.
 *
 * @param oldFileName current name of file.
 * @param newFileName new name of file.
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_rename(javacall_const_utf16_string oldFileName,
                                     javacall_const_utf16_string newFileName);


/** @} */


#ifdef __cplusplus
}
#endif

#endif
