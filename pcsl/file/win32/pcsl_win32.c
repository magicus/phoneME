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

#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

#include <wchar.h>
#include <string.h>

#include <pcsl_file.h>
#include <pcsl_memory.h>

/**
 * directory flag
 */
#define S_ISDIR(mode)	( ((mode) & S_IFMT) == S_IFDIR )

/**
 * file flag
 */
#define S_ISREG(mode)	( ((mode) & S_IFMT) == S_IFREG )

static jchar FILESEP = '\\';
static jchar PATHSEP = ';';

typedef struct _PCSLFileIterator {
    int savedRootLength;
    HANDLE iteratorHandle;
} PCSLFileIterator;

static int findFirstMatch(PCSLFileIterator* pIterator,
                          const pcsl_string * match,
                          pcsl_string * result);
static int findNextMatch(PCSLFileIterator* pIterator,
                         const pcsl_string * match,
                         pcsl_string * result);

/**
 * The initialize function initials the File System
 */
int pcsl_file_init() {
    /* Verify assumptions on the types */
    if (sizeof(jchar) != sizeof(wchar_t)) {
      return -1;
    }
    if (sizeof(jchar) != sizeof(WCHAR)) {
      return -1;
    }

    return pcsl_string_is_active() == PCSL_TRUE ? 0 : -1;
}

/**
 * Cleans up resources used by file system. It is only needed for the 
 * Ram File System (RMFS).
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_finalize() {
    return 0;
}

/**
 * The open function creates and returns a new file identifier for the 
 * file named by filename. Initially, the file position indicator for the 
 * file is at the beginning of the file.
 */
int pcsl_file_open(const pcsl_string * fileName, int flags, void **handle) {

    int fd;
    int oFlag = O_BINARY;
    int creationMode = 0;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(fileName);

    if(pszOsFilename == NULL) {
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
        creationMode = _S_IREAD | _S_IWRITE;
    } 

    if ((flags & PCSL_FILE_O_TRUNC) == PCSL_FILE_O_TRUNC) {
        oFlag |= O_TRUNC;
    } 

    if ((flags & PCSL_FILE_O_APPEND) == PCSL_FILE_O_APPEND) {
        oFlag |= O_APPEND;
    }

    /*
     * Unlike Unix systems, Win32 will convert CR/LF pairs to LF when
     * reading and in reverse when writing, unless the file is opened
     * in binary mode.
     */
    fd = _wopen(pszOsFilename, oFlag | O_BINARY, creationMode);

    pcsl_string_release_utf16_data(pszOsFilename, fileName);

    if(fd < 0) {
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
    pcsl_file_commitwrite(handle); /* commit pending writes */
    return _close((int)handle);
}

/**
 * The read function reads up to size bytes from the file with descriptor identifier , 
 * storing the results in the buffer.
 */
int pcsl_file_read(void *handle, unsigned  char *buffer, long length)
{

    if(0 == length) {
	return 0;
    }

    return _read((int)handle, buffer, (unsigned int)length);

}

/**
 * The write function writes up to size bytes from buffer to the file with descriptor 
 * identifier. 
 * The return value is the number of bytes actually written. This is normally the same 
 * as size, but might be less (for example, if the persistent storage being written to
 * fills up).
 */
int pcsl_file_write(void *handle, unsigned char* buffer, long length)
{
    return _write((int)handle, buffer, length);
}

/**
 * The unlink function deletes the file named filename from the persistent storage.
 */
int pcsl_file_unlink(const pcsl_string * fileName)
{
    int status;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(fileName);

    if (pszOsFilename == NULL) {
    	return -1;
    }

    if (_wunlink(pszOsFilename) < 0) {
    	status = -1;
    } else {
    	status = 0;
    }

    pcsl_string_release_utf16_data(pszOsFilename, fileName);

    return status;
}


/**
 * The  truncate function is used to truncate the size of an open file in storage.
 */
int pcsl_file_truncate(void *handle, long size)
{
	return _chsize((int)handle, size);
}

/**
 * The lseek function is used to change the file position of the file with descriptor 
 * identifier 
 */
long pcsl_file_seek(void *handle, long offset, long position)
{
	return _lseek((int)handle, offset, position);
}

/**
 * FS only need to support MIDLets to quiry the size of the file. 
 * Check the File size by file handle
 */
long pcsl_file_sizeofopenfile(void *handle)
{
    struct _stat stat_buf;

    if (_fstat((int)handle, &stat_buf) < 0) {
        return -1;
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
    struct _stat stat_buf;

    openStatus = pcsl_file_open(fileName, O_RDONLY, (void **)(&handle));

    if(openStatus < 0) {
	return -1;
    }
	
     if (_fstat(handle, &stat_buf) < 0) {
         return -1;
     }

     close(handle);
     return stat_buf.st_size;;
}

/**
 * Check if the file exists in FS storage.
 */
int pcsl_file_exist(const pcsl_string * fileName)
{
    struct _stat stat_buf;
    int status;
    const jchar * pszOsFilename = pcsl_string_get_utf16_data(fileName);

    if(pszOsFilename == NULL) {
 	return -1;
    }

    status = _wstat(pszOsFilename, &stat_buf);

    pcsl_string_release_utf16_data(pszOsFilename, fileName);

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
    // Win32 caches writes.
    return _commit((int)handle);
}


/**
 * The rename function updates the filename.
 */
int pcsl_file_rename(const pcsl_string * oldName, 
		     const pcsl_string * newName)
{
    int res;
    int status;
    const jchar * pszOldFilename = pcsl_string_get_utf16_data(oldName);

    if(pszOldFilename == NULL) {
	return -1;
    } else {
      const jchar * pszNewFilename = pcsl_string_get_utf16_data(newName);

      if(pszNewFilename == NULL) {
        pcsl_string_release_utf16_data(pszOldFilename, oldName);
	return -1;
      }

      res = _wrename(pszOldFilename, pszNewFilename);
      if(res < 0) {
	status = -1;
      } else {
	status = 0;
      }

      return status;
    }
}

/**
 * The opendir function opens directory named dirname. 
 */
void* pcsl_file_openfilelist(const pcsl_string * string)
{
    int rootLength;
    PCSLFileIterator* pIterator;

    pIterator = (PCSLFileIterator*)pcsl_mem_malloc(sizeof (PCSLFileIterator));
    if (pIterator == NULL) {
	/* Error in allocation */
	return NULL;
    }

    memset(pIterator, 0, sizeof(PCSLFileIterator));
    pIterator->iteratorHandle = INVALID_HANDLE_VALUE;

    /*
     * find the root dir of the string
     */
    rootLength = pcsl_string_last_index_of(string, FILESEP);
    if (-1 == rootLength) {
	rootLength = 0;
    } else {
	/* Include the file separator. */
	rootLength++;
    }

    pIterator->savedRootLength = rootLength;

    /*
     * FindFirstFile open and get the first file, so do not
     * do any thing more until get next is called.
     */
    return pIterator;

}

/**
 * The mkdir function closes the directory named dirname. 
 */
int pcsl_file_closefilelist(void *handle)
{
    int status;
    PCSLFileIterator* pIterator = (PCSLFileIterator *)handle;

    if (handle == NULL) {
	return 0;
    }

    if (pIterator->iteratorHandle != INVALID_HANDLE_VALUE) {
	status = FindClose(pIterator->iteratorHandle);
    }

    pcsl_mem_free(pIterator);

    if (status == 0) {
        return -1;
    } else {
        return 0;
    }

}

/**
 * The getFreeSpace function checks the size of free space in storage. 
 */
long pcsl_file_getfreespace()
{
	return 0;
}

/**
 * The getUsedSpace function checks the size of used space in storage. 
 */
long pcsl_file_getusedspace(const pcsl_string * systemDir)
{
    long used = 0;
    void* pIterator;
    pcsl_string current = PCSL_STRING_NULL;
    struct _stat stat_buf;

    pIterator = pcsl_file_openfilelist(systemDir);
    for (; ; ) {
	if (pcsl_file_getnextentry(pIterator, systemDir, &current) == -1) {
            break;
	}

	{
          const jchar * pwszFilename = pcsl_string_get_utf16_data(&current);

	  if (NULL == pwszFilename) {
            break;
	  }

	  /* Don't count the subdirectories "." and ".." */
	  if (_wstat(pwszFilename, &stat_buf) != -1 &&
	      !S_ISDIR(stat_buf.st_mode)) {
            used += stat_buf.st_size;
	  }

          pcsl_string_release_utf16_data(pwszFilename, &current);
	}

	pcsl_string_free(&current);
    }

    pcsl_file_closefilelist(pIterator);
    return used;
}

/* The getNextEntry function search the next file which is  specified DIR */
int pcsl_file_getnextentry(void *handle, const pcsl_string * string, 
                           pcsl_string * result)
{

    PCSLFileIterator* pIterator = (PCSLFileIterator *)handle;

    if (pIterator == NULL) {
	return -1;
    }

    if (pIterator->iteratorHandle == INVALID_HANDLE_VALUE) {
	return findFirstMatch(pIterator, string, result);
    }

    return findNextMatch(pIterator, string, result);

}

jchar
pcsl_file_getfileseparator() {
    return FILESEP;
}

jchar
pcsl_file_getpathseparator() {
    return PATHSEP;
}

static int findFirstMatch(PCSLFileIterator* pIterator,
                          const pcsl_string * match,
                          pcsl_string * result) {
    WIN32_FIND_DATAW findData;
    HANDLE handle;
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(starSuffix)
      {'*', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(starSuffix);
    pcsl_string root = PCSL_STRING_NULL;
    pcsl_string foundName = PCSL_STRING_NULL;
    pcsl_string matchStar = PCSL_STRING_NULL;
    jsize rootLen = 0;

    if (result == NULL) {
      return -1;
    }

    * result = PCSL_STRING_NULL;

    if (pcsl_string_cat(match, &starSuffix, &matchStar) != PCSL_STRING_OK) {
      return -1;
    }

    {
      const jchar * pwszMatch = pcsl_string_get_utf16_data(&matchStar);

      if (NULL == pwszMatch) {
        pcsl_string_free(&matchStar);
	return -1;
      }

      handle = FindFirstFileW(pwszMatch, &findData);

      pcsl_string_free(&matchStar);
    }

    if (INVALID_HANDLE_VALUE == handle) {
	return -1;
    }

    pIterator->iteratorHandle = handle;
    rootLen = pIterator->savedRootLength;

    if (pcsl_string_substring(match, 0, rootLen, &root) != PCSL_STRING_OK) {
      return -1;
    }

    if (pcsl_string_convert_from_utf16(findData.cFileName,
				       wcslen(findData.cFileName),
				       &foundName) != PCSL_STRING_OK) {
      pcsl_string_free(&root);    
      return -1;
    }

    if (pcsl_string_cat(&root, &foundName, result) != PCSL_STRING_OK) {
      pcsl_string_free(&foundName);    
      pcsl_string_free(&root);    
      return -1;
    }

    pcsl_string_free(&foundName);    
    pcsl_string_free(&root);    

    return 0;
}

static int findNextMatch(PCSLFileIterator* pIterator,
                         const pcsl_string * match,
                         pcsl_string * result) {
    WIN32_FIND_DATAW findData;
    pcsl_string root = PCSL_STRING_NULL;
    pcsl_string foundName = PCSL_STRING_NULL;
    jsize rootLen = 0;

    if (result == NULL) {
      return -1;
    }

    * result = PCSL_STRING_NULL;

    if (!FindNextFileW(pIterator->iteratorHandle, &findData)) {
	return -1;
    }

    rootLen = pIterator->savedRootLength;

    if (pcsl_string_substring(match, 0, rootLen, &root) != PCSL_STRING_OK) {
      return -1;
    }

    if (pcsl_string_convert_from_utf16(findData.cFileName,
				       wcslen(findData.cFileName),
				       &foundName) != PCSL_STRING_OK) {
      pcsl_string_free(&root);    
      return -1;
    }

    if (pcsl_string_cat(&root, &foundName, result) != PCSL_STRING_OK) {
      pcsl_string_free(&foundName);    
      pcsl_string_free(&root);    
      return -1;
    }

    pcsl_string_free(&foundName);    
    pcsl_string_free(&root);

    return 0;
}
