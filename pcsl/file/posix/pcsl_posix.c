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
#include <dirent.h>

#include <pcsl_file.h>
#include <pcsl_memory.h>
#include <pcsl_util_filelist.h>

/**
 * Maximum length of a file name
 */
#define PCSL_FILE_MAX_NAME_LEN	   256

/*  creation mode = 0444 | 0222 */
#define DEFAULT_FILE_CREATION_MODE (0444 | 0222)

static const jchar FILESEP = '/';
static const jchar PATHSEP = ':';

void* pcsl_file_opendir(const pcsl_string * dirName);
int pcsl_file_closedir(void *handle);

/**
 * The initialize function initials the File System
 */
int pcsl_file_init() {
    return pcsl_string_is_active() == PCSL_TRUE ? 0 : -1;
}

/**
 * Cleans up resources used by file system.
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_finalize() {
    return 0;
}


/**
 * The open function creates and returns a new file identifier for the 
 * file named by filename. Initially, the file position indicator for the 
 * file is at the beginning of the file.
 * 
 */
int pcsl_file_open(const pcsl_string * fileName, int flags, void **handle)
{
    int   fd;
    int oFlag = O_RDONLY;
    int   creationMode = 0;
    const jbyte * pszOsFilename = pcsl_string_get_utf8_data(fileName);

    if (pszOsFilename == NULL) {
      return -1;
    }

    /* compute open control flag */
    if ((flags & PCSL_FILE_O_WRONLY) == PCSL_FILE_O_WRONLY) {
        oFlag |= O_WRONLY;
    } 

    if ((flags & PCSL_FILE_O_RDWR) == PCSL_FILE_O_RDWR) {
        oFlag |= O_RDWR;
    } 

    if ((flags & PCSL_FILE_O_CREAT) == PCSL_FILE_O_CREAT) {
        oFlag |= O_CREAT;
        creationMode = DEFAULT_FILE_CREATION_MODE;
    } 

    if ((flags & PCSL_FILE_O_TRUNC) == PCSL_FILE_O_TRUNC) {
        oFlag |= O_TRUNC;
    } 

    if ((flags & PCSL_FILE_O_APPEND) == PCSL_FILE_O_APPEND) {
        oFlag |= O_APPEND;
    }

    fd = open((char*)pszOsFilename, oFlag, creationMode);

    pcsl_string_release_utf8_data(pszOsFilename, fileName);

    if (fd < 0) {
        *handle = NULL;
        return -1;
    }

    *handle = (void *)fd;
    return 0;
}

/**
 * The close function  loses the file with descriptor identifier in FS. 
 */
int pcsl_file_close(void *handle) 
{
	return close((int)handle);
}

/**
 * The read function reads up to size bytes from the file with descriptor identifier , 
 * storing the results in the buffer.
 */
int pcsl_file_read(void *handle, unsigned  char *buf, long size)
{
	return read((int)handle, buf, size);
}

/**
 * The write function writes up to size bytes from buffer to the file with descriptor 
 * identifier. 
 * The return value is the number of bytes actually written. This is normally the same 
 * as size, but might be less (for example, if the persistent storage being written to
 * fills up).
 */
int pcsl_file_write(void *handle, unsigned char* buffer, long size)
{
	return write((int)handle, buffer, size);
}

/**
 * The unlink function deletes the file named fileName from the persistent storage.
 */
int pcsl_file_unlink(const pcsl_string * fileName)
{
    int status;
    const jbyte * pszOsFilename = pcsl_string_get_utf8_data(fileName);

    if (pszOsFilename == NULL) {
      return -1;
    }

    status = unlink((char*)pszOsFilename);

    pcsl_string_release_utf8_data(pszOsFilename, fileName);
    
    return status;
}

/**
 * The  truncate function is used to truncate the size of an open file in storage.
 */
int pcsl_file_truncate(void *handle, long size)
{
	return ftruncate((int)handle, (off_t)size);
}

/**
 * The lseek function is used to change the file position of the file with descriptor 
 * identifier 
 */
long pcsl_file_seek(void *handle, long offset, long position)
{
	return lseek((int)handle, offset, position);
}

/**
 * FS only need to support MIDLets to quiry the size of the file. 
 * Check the File size by file handle
 */
long pcsl_file_sizeofopenfile(void *handle)
{
    struct stat stat_buf;
    int status;

    if ((status = fstat((int)handle, &stat_buf)) < 0) {
        return status;
    }

 
     return stat_buf.st_size;
}

/**
 * FS only need to support MIDLets to quiry the size of the file. 
 * Check the File size by file name
 */
long pcsl_file_sizeof(const pcsl_string * fileName)
{
    int handle, openStatus;
    struct stat stat_buf;

    openStatus = pcsl_file_open(fileName, O_RDONLY, (void **)(&handle));

    if(openStatus < 0) {
	return -1;
    }
	
     if (fstat(handle, &stat_buf) < 0) {
         return -1;
     }

     close(handle);
     return stat_buf.st_size;
}

/**
 * Check if the file exists in FS storage.
 */
int pcsl_file_exist(const pcsl_string * fileName)
{
    struct stat stat_buf;
    int status;
    const jbyte * pszOsFilename = pcsl_string_get_utf8_data(fileName);

    if (pszOsFilename == NULL) {
      return 0;
    }
 
    status = stat((char*)pszOsFilename, &stat_buf);

    pcsl_string_release_utf8_data(pszOsFilename, fileName);

    if (status >= 0 && S_ISREG(stat_buf.st_mode)) {
        /* stat completed without error and it is a file */
        return 1;
    }

    /* either stat completed with error or it is not a file */
    return 0;
}

/* Force the data to be written into the FS storage */
int pcsl_file_commitwrite(void *handle)
{
    return 0;
}

/**
 * The rename function updates the filename.
 */
int pcsl_file_rename(const pcsl_string * oldName, const pcsl_string * newName) {
    int status;
    const jbyte * pszOldFilename = pcsl_string_get_utf8_data(oldName);

    if (pszOldFilename == NULL) {
      return -1;
    } else {
      const jbyte * pszNewFilename = pcsl_string_get_utf8_data(newName);

      if (pszNewFilename == NULL) {
	pcsl_string_release_utf8_data(pszOldFilename, oldName);
	return -1;
      } 

      if (rename((char*)pszOldFilename, (char*)pszNewFilename) < 0) {
        status = -1;
      } else {
        status = 0;
      }
	
      pcsl_string_release_utf8_data(pszOldFilename, oldName);
      pcsl_string_release_utf8_data(pszNewFilename, newName);

      return status;
    }
}

/**
 * The openfilelist function opens the directory. 
 */
void* pcsl_file_openfilelist(const pcsl_string * string)
{

    return pcsl_util_openfileiterator(string);
}

/**
 * The closefilelist function closes the directory. 
 */
int pcsl_file_closefilelist(void *handle)
{
    return pcsl_util_closefileiterator(handle);
}

/**
 * The getFreeSpace function checks the size of free space in storage. 
 * Note: the function to be removed.
 */
long pcsl_file_getfreespace()
{
    return 0;
}

/**
 * The getUsedSpace function checks the size of used space by
 * all of the files that are contained in the directory.
 */
long pcsl_file_getusedspace(const pcsl_string * dirName)
{
    DIR *dir;
    long size;
    struct dirent *de;
    struct stat stat_buf;
    char filename[PCSL_FILE_MAX_NAME_LEN + 1];
    int rootLen;
    int exists;

    dir = (DIR *)pcsl_file_opendir(dirName);
    if (dir == NULL) {
        return 0;
    }

    size = 0;

    {
      if (pcsl_string_convert_to_utf8(dirName, (jbyte*)filename, sizeof(filename), NULL)
	  != PCSL_STRING_OK) {
	pcsl_file_closedir(dir);
	return 0;
      }

      rootLen = strlen(filename);
      for (de = readdir(dir); de != NULL;
	   de = readdir(dir), filename[rootLen] = 0) {
        if (strcmp(de->d_name, ".") == 0 ||
            strcmp(de->d_name, "..") == 0) {
	  continue;
        }

        strncat(filename, de->d_name, sizeof (filename) - 1 - rootLen);

        exists = stat(filename, &stat_buf);
        if (exists != -1 && !S_ISDIR(stat_buf.st_mode)) {
            size += stat_buf.st_size;
        }
      }

      pcsl_file_closedir(dir);
      return size;
    }
}

/** 
    The getNextEntry function search the next file which is specified DIR
 */
int pcsl_file_getnextentry(void *handle, const pcsl_string * string, 
                           pcsl_string * result)
{
    pcsl_string rootpath = PCSL_STRING_NULL;
    pcsl_string returnVal = PCSL_STRING_NULL;
    pcsl_string matchName = PCSL_STRING_NULL;
    jsize matchLen;
    jsize len;
    struct dirent *de;
    char* pszFilename = NULL;
    DIR *dir;
    jsize savedRootLength = 0;
    jsize savedMatchLength = 0;

    PCSLStorageDirInfo* pPCSLStorageDirInfo = (PCSLStorageDirInfo *)handle;
    if (pPCSLStorageDirInfo == NULL) {
        return -1;
    }

    savedRootLength = pPCSLStorageDirInfo->savedRootLength;
    savedMatchLength = pPCSLStorageDirInfo->savedMatchLength;

    if (pcsl_string_substring(string, savedRootLength, 
			      savedRootLength + savedMatchLength,
			      &matchName) != PCSL_STRING_OK) {
      return -1;
    }

    {
      const jbyte * pszMatch = NULL;
      
      dir = pPCSLStorageDirInfo->savedDirectory;

      if (dir == NULL) {
        return -1;
      }

      pszMatch = pcsl_string_get_utf8_data(&matchName);
      
      if (pszMatch == NULL) {
	return -1;
      }

      matchLen = strlen((char*)pszMatch);

      /* find the first match file not "." or ".." */

      for (de = readdir(dir); de != NULL; de = readdir(dir)) {

        pszFilename = de->d_name;
        if (strcmp(pszFilename, ".") == 0 ||
            strcmp(pszFilename, "..") == 0) {
            continue;
        }

        if (strncmp(pszFilename, (char*)pszMatch, matchLen) == 0) {
            break;
        }
      }

      pcsl_string_release_utf8_data(pszMatch, &matchName);

      pcsl_string_free(&matchName);

      if (NULL == de) {
        /* End of Dir without a match. */
        return -1;
      }
      len = strlen(pszFilename);

      if (len >= 0) {
	if (pcsl_string_convert_from_utf8((jbyte*)pszFilename, len, &returnVal) != PCSL_STRING_OK) {
	  return -1;
	}

	if (pcsl_string_substring(string, 0, savedRootLength, &rootpath) != PCSL_STRING_OK) {
	  pcsl_string_free(&returnVal);
	  return -1;
	}

	if (pcsl_string_cat(&rootpath, &returnVal, result) != PCSL_STRING_OK) {
	  pcsl_string_free(&returnVal);
	  pcsl_string_free(&rootpath);
	  return -1;
	}

	pcsl_string_free(&returnVal);
	pcsl_string_free(&rootpath);
      }
    }

    return 0;

}

jchar pcsl_file_getfileseparator() {
    return FILESEP;
}

jchar pcsl_file_getpathseparator() {
    return PATHSEP;
}

/**
 * The opendir function opens directory named dirname.
 */
void* pcsl_file_opendir(const pcsl_string * dirName)
{
    DIR* dir;
    const jbyte * pszOsFilename = pcsl_string_get_utf8_data(dirName);

    if (pszOsFilename == NULL) {
      return NULL;
    }

    dir = opendir((char*)pszOsFilename);

    pcsl_string_release_utf8_data(pszOsFilename, dirName);

    return dir;

}

int pcsl_file_closedir(void *handle)
{
    return closedir((DIR *)handle);
}
