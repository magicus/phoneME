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
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/statvfs.h>

#include <pcsl_directory.h>
#include <pcsl_memory.h>

/**
 * @def PCSL_DIR_MAX_NAME_LEN
 * Maximum length of a file name
 */
#define PCSL_DIR_MAX_NAME_LEN	   256

#define DEFAULT_DIR_CREATION_MODE  (0444 | 0222 | 0111)

/**
 * Check if the directory exists in FS storage.
 */
int pcsl_file_is_directory(const pcsl_string * path)
{
    struct stat stat_buf;
    int   status  = -1;
    const jbyte * pszName = pcsl_string_get_utf8_data(path);

    if (pszName == NULL) {
      return -1;
    }
 
    status = stat((char*)pszName, &stat_buf);

    pcsl_string_release_utf8_data(pszName, path);

    if (status >= 0 && S_ISDIR(stat_buf.st_mode)) {
        /* stat completed without error and it is a directory */
        return 1;
    }

    /* either stat completed with error or it is not a directory */
    return 0;
}

/**
 * Creates the directory with specified name.
 */
int pcsl_file_mkdir(const pcsl_string * dirName)
{    
    int   res          = -1;
    const jbyte * pszOsDirName = pcsl_string_get_utf8_data(dirName);

    if (pszOsDirName == NULL) {
      return -1;
    }

    res = mkdir((char*)pszOsDirName, DEFAULT_DIR_CREATION_MODE);

    pcsl_string_release_utf8_data(pszOsDirName, dirName);
    
    return res;
}

/**
 * The function deletes the directory named dirName from the persistent storage.
 */
int pcsl_file_rmdir(const pcsl_string * dirName)
{
    int   status     = -1;
    const jbyte * pszDirName = pcsl_string_get_utf8_data(dirName);

    if (pszDirName == NULL) {
      return -1;
    }

    status = rmdir((char*)pszDirName);

    pcsl_string_release_utf8_data(pszDirName, dirName);

    return status;
}

/**
 * The getFreeSize function checks the available size in storage.
 */
jlong pcsl_file_getfreesize(const pcsl_string * path)
{
    struct statvfs sbuf;
    jlong  size   = -1; /* -1 if the file system is not accessible */
    int    status = -1;
    const jbyte * pszPath = pcsl_string_get_utf8_data(path);

    if (pszPath == NULL) {
      return -1;
    }
    
    status = statvfs((char*)pszPath, &sbuf);
    if (status == 0)
    {
        size = (jlong)(sbuf.f_bsize) * sbuf.f_bavail;
    }
    
    pcsl_string_release_utf8_data(pszPath, path);

    return size;
}

/**
 * The getTotalSize function checks the total space in storage.
 */
jlong pcsl_file_gettotalsize(const pcsl_string * path)
{
    struct statvfs sbuf;
    jlong  size   = -1; /* -1 if the file system is not accessible */
    int    status = -1;
    const jbyte * pszPath = pcsl_string_get_utf8_data(path);

    if (pszPath == NULL) {
      return -1;
    }
    
    status = statvfs((char*)pszPath, &sbuf);
    if (status == 0)
    {
        size = (jlong)(sbuf.f_bsize) * sbuf.f_blocks;
    }
    
    pcsl_string_release_utf8_data(pszPath, path);

    return size;
}

//-----------------------------------------------------------------------------

/**
 * The function returns value of the attribute for the specified file.
 */
int pcsl_file_get_attribute(const pcsl_string * fileName, int type, int* result)
{
    struct stat sbuf;
    int   status = -1;
    const jbyte * pszName = pcsl_string_get_utf8_data(fileName);

    if (pszName == NULL) {
      return -1;
    }
    
    *result = 0;
    
    status = stat((char*)pszName, &sbuf);

    pcsl_string_release_utf8_data(pszName, fileName);
        
    if (status == 0)
    {
        switch (type) {
        case PCSL_FILE_ATTR_READ:
            *result = (sbuf.st_mode & S_IRUSR) ? 1 : 0;
            break;
        case PCSL_FILE_ATTR_WRITE:
            *result = (sbuf.st_mode & S_IWUSR) ? 1 : 0;
            break;
        case PCSL_FILE_ATTR_EXECUTE:
            *result = (sbuf.st_mode & S_IXUSR) ? 1 : 0;
            break;
        case PCSL_FILE_ATTR_HIDDEN:
            *result = 0;
            break;
        default:
            return -1;
        }        
        return 0;
    }
    
    return -1;
}

/**
 * The function sets value of the attribute for the specified file.
 */
int pcsl_file_set_attribute(const pcsl_string * fileName, int type, int value)
{
    struct stat sbuf;
    int   status   = -1;    
    int   newmode  = 0;
    int   modeMask = 0;
    int   result   = -1;
    const jbyte * pszName = pcsl_string_get_utf8_data(fileName);

    if (pszName == NULL) {
      return -1;
    }

    status = stat((char*)pszName, &sbuf);    
        
    while (status == 0)
    {        
        switch (type) {
        case PCSL_FILE_ATTR_READ:            
            modeMask = S_IRUSR;
            break;
        case PCSL_FILE_ATTR_WRITE:
            modeMask = S_IWUSR;
            break;
        case PCSL_FILE_ATTR_EXECUTE:
            modeMask = S_IXUSR;
            break;
        case PCSL_FILE_ATTR_HIDDEN:
            result = 0;
            status = -1;
            break;
        default:
            status = -1;
        }

        if (status != 0) {
            break;
        }
        
        if (value) {
            newmode =  modeMask | sbuf.st_mode;
        } else {
            newmode = ~modeMask & sbuf.st_mode;
        }        
        

        /* do not update file attributes if they are not changed */        
        if (newmode == sbuf.st_mode) {
            result = 0;
            break;
        }

        status = chmod((char*)pszName, newmode);        
        if (status != -1) {
            result = 0;
        }
        break;
    }
    
    pcsl_string_release_utf8_data(pszName, fileName);
    return result;
}

/**
 * The function returns value of the time for the specified file.
 */
int pcsl_file_get_time(const pcsl_string * fileName, int type, long* result)
{
    struct stat sbuf;
    int   status = -1;
    const jbyte * pszName = pcsl_string_get_utf8_data(fileName);

    if (pszName == NULL) {
      return -1;
    }
    
    *result = 0;
    
    status = stat((char*)pszName, &sbuf);

    pcsl_string_release_utf8_data(pszName, fileName);
        
    if (status == 0)
    {
        switch (type) {
        case PCSL_FILE_TIME_LAST_MODIFIED:
            *result = sbuf.st_mtime;
            break;        
        default:
            return -1;
        }        
        return 0;
    }
    
    return -1;
}
