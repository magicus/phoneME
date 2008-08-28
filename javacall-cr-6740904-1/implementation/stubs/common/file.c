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

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_file.h"

/**
 * PLEASE NOTE:
 * API descriptions in the header file SUPERCEDE the descriptions
 * herein.
 */

/**
 * Initializes the File System
 * @return <tt>JAVACALL_OK</tt> on success, <tt>JAVACALL_FAIL</tt> or negative value on error
 */
javacall_result javacall_file_init(void) {
    return JAVACALL_OK;
}
/**
 * Cleans up resources used by file system
 * @return <tt>JAVACALL_OK</tt> on success, <tt>JAVACALL_FAIL</tt> or negative value on error
 */ javacall_result javacall_file_finalize(void) {
    return JAVACALL_OK;
}

/**
 * Open a file
 * @param unicodeFileName fully-qualified name (including path) of file to be 
 *        opened, in UNICODE (utf16)
 * @param fileNameLen length of the file name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @param flags open control flags
 *        Applications must specify exactly one of the first three
 *        values (file access modes) below in the value of "flags"
 *        JAVACALL_FILE_O_RDONLY, 
 *        JAVACALL_FILE_O_WRONLY, 
 *        JAVACALL_FILE_O_RDWR
 *
 *        And any combination (bitwise-inclusive-OR) of the following:
 *        JAVACALL_FILE_O_CREAT, 
 *        JAVACALL_FILE_O_TRUNC, 
 *        JAVACALL_FILE_O_APPEND,
 *
 * @param handle address of pointer to file identifier
 *        on successful completion, file identifier is returned in this 
 *        argument. This identifier is platform specific and is opaque
 *        to the caller.  
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value on error
 * 
 */
javacall_result javacall_file_open(const javacall_utf16 * unicodeFileName, int fileNameLen, int flags, /*OUT*/ javacall_handle * handle) {
    return JAVACALL_FAIL;
}

/**
 * Closes the file with the specified handle
 * @param handle handle of file to be closed
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_close(javacall_handle handle) {
    return JAVACALL_FAIL;
}


/**
 * Reads a specified number of bytes from a file.
 * @param handle handle of file 
 * @param buf buffer to which data is read
 * @param size number of bytes to be read. Actual number of bytes
 *              read may be less, if an end-of-file is encountered
 * @return the number of bytes actually read
 */
long javacall_file_read(javacall_handle handle, unsigned char *buf, long size) {
    return 0;
}

/**
 * Writes bytes to file.
 * @param handle handle of file 
 * @param buf buffer to be written
 * @param size number of bytes to write
 * @return the number of bytes actually written. This is normally the same 
 *         as size, but might be less (for example, if the persistent storage being 
 *         written to fills up).
 */
long javacall_file_write(javacall_handle handle, const unsigned char *buf, long size) {
    return 0;
}

/**
 * Deletes a file from the persistent storage.
 * @param unicodeFileName fully-qualified name (including path) of file to be 
 *         deleted, in UNICODE (utf16)
 * @param fileNameLen length of the file name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value on error
 */
javacall_result javacall_file_delete(const javacall_utf16 * unicodeFileName, int fileNameLen) {
    return JAVACALL_FAIL;
}

/**
 * The  truncate function is used to truncate the size of an open file in 
 * the filesystem storage.
 * For CDC-HI - based implementations, it is required that this function also
 * be capable of enlarging a file (standard unix truncate() functionality).
 * For CLDC-HI - based implementations, this function need not have the 
 * capability to enlarge a file.
 * @param handle identifier of file to be truncated
 *         This is the identifier returned by javacall_file_open()
 * @param size size to truncate to
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value on error
 */
javacall_result javacall_file_truncate(javacall_handle handle, javacall_int64 size) {
    return JAVACALL_FAIL;
}

/**
 * Sets the file pointer associated with a file identifier 
 * @param handle identifier of file
 *               This is the identifier returned by javacall_file_open()
 * @param offset number of bytes to offset file position by
 * @param flag controls from where offset is applied, from 
 *                 the beginning, current position or the end
 *                 Can be one of JAVACALL_FILE_SEEK_CUR, JAVACALL_FILE_SEEK_SET 
 *                 or JAVACALL_FILE_SEEK_END
 * @return on success the actual resulting offset from beginning of file
 *         is returned, otherwise -1 is returned
 */
javacall_int64 javacall_file_seek(javacall_handle handle, javacall_int64 offset, javacall_file_seek_flags flag) {
    return -1;
}


/**
 * Get file size 
 * @param handle identifier of file
 *               This is the identifier returned by pcsl_file_open()
 * @return size of file in bytes if successful, -1 otherwise
 */
javacall_int64 javacall_file_sizeofopenfile(javacall_handle handle) {
    return -1;
}

/**
 * Get file size
 * @param fileName fully-qualified name (including path) of file
 *         in UNICODE (utf16)
 * @param fileNameLen length of the file name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return size of file in bytes if successful, -1 otherwise 
 */
javacall_int64 javacall_file_sizeof(const javacall_utf16 * fileName, int fileNameLen) {
    return -1;
}

/**
 * Check if a regular file exists in file system storage.
 *
 * @param fileName fully-qualified name (including path) of file
 *         in UNICODE (utf16)
 * @param fileNameLen length of the file name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return <tt>JAVACALL_OK </tt> if it exists and is a regular file, 
 *         <tt>JAVACALL_FAIL</tt> otherwise (e.g., if the file is a directory, or does not exist)
 */
javacall_result javacall_file_exist(const javacall_utf16 * fileName, int fileNameLen) {
    return JAVACALL_FAIL;
}


/** Force the data to be written into the file system storage
 * @param handle identifier of file
 *               This is the identifier returned by javacall_file_open()
 * @return JAVACALL_OK  on success, <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_flush(javacall_handle handle) {
    return JAVACALL_FAIL;
}

/**
 * Renames the filename. 
 * If the underlying operating system API can "rename" from one arbitrary 
 * path to another, this behavior is preferable. If not, and if the filename 
 * parameters have different paths, a value of JAVACALL_FAIL, should be
 * returned.
 * @param unicodeOldFilename current fully-qualified name (including path) of 
 *         file in UNICODE (utf16)
 * @param oldNameLen current name length or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string
 * @param unicodeNewFilename new fully-qualified name (including path) of 
 *         file in UNICODE (utf16)
 * @param newNameLen length of new name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return <tt>JAVACALL_OK</tt>  on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_rename(const javacall_utf16 * unicodeOldFilename, int oldNameLen, const javacall_utf16 * unicodeNewFilename, int newNameLen) {
    return JAVACALL_FAIL;
}


#ifdef __cplusplus
}
#endif
