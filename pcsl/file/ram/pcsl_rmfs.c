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
#include <memory.h>

#include <rmfsAlloc.h>
#include <rmfsApi.h>
#include <pcsl_file.h>
#include <pcsl_memory.h>
#include <pcsl_util_filelist.h>

#define PCSL_FILE_MAX_NAME_LEN 256

extern unsigned char tempBuf[300];      /* This buffer is used to avoid jvmMalloc and jvmFree */
static jchar FILESEP = '/';
static jchar PATHSEP = ':';

/*
 * Default size of RAM FS; in bytes
 */
static int DEFAULT_RAMFS_SIZE  = 4096*1024;

#ifdef PCSL_RAMFS_USE_STATIC
/* Cannot allocate dynamic memory on the phone. Use static array. */
static char RamfsMemory[DEFAULT_RAMFS_SIZE];	 /* Where RAMFS starts */
#else  /* use malloc or similar function provided */
static char* RamfsMemory = NULL;			 /* Where RAMFS starts */
#endif

void* pcsl_file_opendir(const pcsl_string * dirName);
int pcsl_file_closedir(void *handle);

/** The initialize function initials the File System, it is only needed for RMFS, for RMFS, this
 *  function specifies one memory block to be File System storage.
 */
int pcsl_file_init()
{
  if (pcsl_string_is_active() == PCSL_FALSE) {
    return -1;
  }

#ifndef PCSL_RAMFS_USE_STATIC
    if (RamfsMemory == NULL) {
        /* allocate the chunk of memory to C heap */
        RamfsMemory = (char*)pcsl_mem_malloc(DEFAULT_RAMFS_SIZE);
        if (RamfsMemory == NULL) {
    	    return -1;
        }
    }
#endif /* ! PCSL_RAMSF_USE_STATIC */

    return rmfsInitialize((jint)RamfsMemory, DEFAULT_RAMFS_SIZE);
}

/**
 * Cleans up resources used by file system. It is only needed for the 
 * Ram File System (RMFS).
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_finalize() {
#ifndef PCSL_RAMFS_USE_STATIC
    if (RamfsMemory != NULL) {
        pcsl_mem_free(RamfsMemory);
    }
#endif /* ! PCSL_RAMSF_USE_STATIC */
    return 0;
}


/**
 * The open function creates and returns a new file identifier for the 
 * file named by filename. Initially, the file position indicator for the 
 * file is at the beginning of the file.
 * 
 */
int pcsl_file_open(const pcsl_string * fileName, int flags, void **handle) {

    int fd;
    int mode = 0;
    const jbyte * pszOsFilename = pcsl_string_get_utf8_data(fileName);

    if (pszOsFilename == NULL) {
	return -1;
    }

    if ((flags & PCSL_FILE_O_CREAT) == PCSL_FILE_O_CREAT) {
        mode = 0664;
    }

    fd = rmfsOpen(pszOsFilename, flags, mode);

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
    if((int)handle >= 0) {
	return rmfsClose((int)handle);
    }

    return -1;
}

/**
 * The read function reads up to size bytes from the file with descriptor identifier , 
 * storing the results in the buffer.
 */
int pcsl_file_read(void *handle, unsigned  char *buf, long size)
{
    if((int)handle >= 0) {
	return rmfsRead((int)handle, buf, size);
    }

    return -1;
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
    if((int)handle >= 0) {
	return rmfsWrite((int)handle, buffer, length);
    }

    return -1;
}

/**
 * The unlink function deletes the file named filename from the persistent storage.
 */
int pcsl_file_unlink(const pcsl_string * fileName)
{
    int status;
    const jbyte * pszOsFilename = pcsl_string_get_utf8_data(fileName);

    if (pszOsFilename == NULL) {
	return -1;
    }

    status = rmfsUnlink(pszOsFilename);

    pcsl_string_release_utf8_data(pszOsFilename, fileName);

    return status;

}


/**
 * The  truncate function is used to truncate the size of an open file in storage.
 */
int pcsl_file_truncate(void *handle, long size)
{
    if((int)handle >= 0) {
	return rmfsTruncate((int)handle, size);
    }

    return -1;
}

/**
 * The lseek function is used to change the file position of the file with descriptor 
 * identifier 
 */
long pcsl_file_seek(void *handle, long offset, long position)
{
    if((int)handle >= 0) {
	return rmfsLseek((int)handle, offset, position);
    }

    return -1;
}

/**
 * FS only need to support MIDLets to quiry the size of the file. 
 * Check the File size by file handle
 */
long pcsl_file_sizeofopenfile(void *handle)
{

    if((int)handle >= 0) {
        return (long)rmfsFileSize((int)handle);
    }

    return -1;
}

/**
 * FS only need to support MIDLets to quiry the size of the file. 
 * Check the File size by file name
 */
long pcsl_file_sizeof(const pcsl_string * fileName)
{
	int fd, openStatus;

	openStatus = (int)pcsl_file_open(fileName, 
					 PCSL_FILE_O_RDONLY, 
					 (void **)(&fd));

	if(openStatus == 0 ) {
	  return (long)rmfsFileSize(fd);
	} 

	return -1;
}

/**
 * Check if the file exists in FS storage.
 */
int pcsl_file_exist(const pcsl_string * fileName)
{
    int status;
    const jbyte * pszOsFilename = pcsl_string_get_utf8_data(fileName);

    if (pszOsFilename == NULL) {
	return -1;
    }

    rmfsFileExist(pszOsFilename, &status);

    pcsl_string_release_utf8_data(pszOsFilename, fileName);

    return status;
}

/* Force the data to be written into the FS storage */
int pcsl_file_commitwrite(void *handle)
{
	return 0;
}


/**
 * The rename function updates the filename.
 */
int pcsl_file_rename(const pcsl_string * oldName, 
		     const pcsl_string * newName)
{
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

      status = rmfsRename(pszOldFilename, pszNewFilename);

      pcsl_string_release_utf8_data(pszOldFilename, oldName);
      pcsl_string_release_utf8_data(pszNewFilename, newName);

      return status;
    }
}

/**
 * The openfilelist function opens filelist 'string' 
 */
void* pcsl_file_openfilelist(const pcsl_string * string)
{
    return pcsl_file_opendir(string);
}

/**
 * The closefilelist function closes filelist pointed to by handle. 
 */
int pcsl_file_closefilelist(void *handle)
{
 	return pcsl_file_closedir(handle);
}

/**
 * The getFreeSpace function checks the size of free space in storage. 
 */
long pcsl_file_getfreespace()
{
    return rmfsGetFreeSpace();
}

/**
 * The getUsedSpace function checks the size of used space in storage. 
 */
long pcsl_file_getusedspace(const pcsl_string * sysdir)
{
    return rmfsGetUsedSpace();
}

/* The getNextEntry function search the next file which is  specified DIR */
int pcsl_file_getnextentry(void *handle, const pcsl_string * string, 
                           pcsl_string * result)
{
    char *pszFilename; /* name of result */
    int len;
    const jbyte * pszMatch = pcsl_string_get_utf8_data(string);

    if (NULL == pszMatch) {
	return -1;
    }

    if (result == NULL) {
      pcsl_string_release_utf8_data(pszMatch, string);
      return -1;
    }

    pszFilename = rmfsFileStartWith(pszMatch);

    pcsl_string_release_utf8_data(pszMatch, string);

    if (NULL == pszFilename) {
      * result = PCSL_STRING_NULL;
      /* End of Dir without a match. */
      return -1;
    }

    len = strlen(pszFilename);

    if (pcsl_string_convert_from_utf8(pszFilename, len, result) != 
	PCSL_STRING_OK) {
      * result = PCSL_STRING_NULL;
      return -1;
    }

    return 0;
}

jchar
pcsl_file_getfileseparator() {
    return FILESEP;
}

jchar
pcsl_file_getpathseparator() {
    return PATHSEP;
}

/*
 * The opendir function opens directory named dirname. 
 */
void* pcsl_file_opendir(const pcsl_string * dirName)
{
    DIR* dir;
    const jbyte * pszOsFilename = pcsl_string_get_utf8_data(dirName);

    dir = rmfsOpendir(pszOsFilename);

    pcsl_string_release_utf8_data(pszOsFilename, dirName);

    return dir;
}

/**
 * The mkdir function closes the directory named dirname. 
 */
int pcsl_file_closedir(void *dirname)
{
	return rmfsClosedir((char *)dirname);
}
