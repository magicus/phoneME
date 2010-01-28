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

#include <stdio.h>

#include <pcsl_directory.h>

/**
 * Check if the path is a directory and it exists in file system storage.
 * @param path name of file or directory
 * @return 1 if it exists and is a directory,
 *         0 otherwise,
 *         -1 in case of an error.
 */
int pcsl_file_is_directory(const pcsl_string * path) {
    return -1;
}

/**
 * The mkdir function creates directory   
 * @param dirName name of directory to be created
 *                 In case of hierarchial systems, this can be a path
 * @return 0 on success, -1 otherwise
 * 
 */
int pcsl_file_mkdir(const pcsl_string * dirName) {
    return -1;
}

/**
 * The function deletes a directory from the persistent storage.
 * @param dirName name of directory to be deleted
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_rmdir(const pcsl_string * dirName) {
    return -1;
}

/**
 * Checks the size of storage space that is available for user.
 * @param path path is the path name of any file within the file system
 * @return size of available space in storage on success,
 *         -1 otherwise.
 */
jlong pcsl_file_getfreesize(const pcsl_string * path) {
    return -1;
}

/**
 * Checks the capacity of the storage.
 * @param path path is the path name of any file within the file system
 * @return size of total space in storage on success, -1 otherwise
 */
jlong pcsl_file_gettotalsize(const pcsl_string * path) {
    return -1;
}

//-----------------------------------------------------------------------------

/**
 * Returns value of the attribute for the specified file.
 * @param fileName name of file
 * @param type type of attribute to be got.
 *             Valid values are PCSL_FILE_ATTR_READ, PCSL_FILE_ATTR_WRITE,
 *             PCSL_FILE_ATTR_EXECUTE and PCSL_FILE_ATTR_HIDDEN.
 * @param result returned attribute's value
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_get_attribute(const pcsl_string * fileName, int type, int* result) {
    return -1;
}

/**
 * Sets value of the attribute for the specified file.
 * @param fileName name of file
 * @param type type of attribute to be setted
 *             Valid values are PCSL_FILE_ATTR_READ, PCSL_FILE_ATTR_WRITE,
 *             PCSL_FILE_ATTR_EXECUTE and PCSL_FILE_ATTR_HIDDEN. 
 * @param value 1 to set the attribute, 0 to reset
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_set_attribute(const pcsl_string * fileName, int type, int value) {
    return -1;
}

/**
 * Returns the specified time for the file.
 * @param fileName name of file
 * @param type type of time to be got
 *             Valid type is PCSL_FILE_TIME_LAST_MODIFIED.
 * @param result returned time's value in seconds
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_get_time(const pcsl_string * fileName, int type, long* result) {
    return -1;
}

