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

#include <pcsl_file.h>
#include <pcsl_memory.h>
#include <pcsl_util_filelist.h>

/**
 * Maximum length of a file name
 */
#define PCSL_FILE_MAX_NAME_LEN	   256

#define O_RDONLY            PCSL_FILE_O_RDONLY
#define O_WRONLY            PCSL_FILE_O_WRONLY
#define O_CREATE            PCSL_FILE_O_CREAT
#define O_APPEND            PCSL_FILE_O_APPEND
#define O_TRUNC             PCSL_FILE_O_TRUNC
#define RDWR_CREATE         PCSL_FILE_O_RDWR | PCSL_FILE_O_CREAT
#define RDWR_CREATE_TRUNC   PCSL_FILE_O_RDWR | PCSL_FILE_O_CREAT | PCSL_FILE_O_TRUNC
#define RDWR_TRUNC          PCSL_FILE_O_RDWR | PCSL_FILE_O_TRUNC

/**
 * The initialize function initials the File System
 */
int pcsl_file_init() {
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
 * file is at the beginning of the file. The argument  creationMode  is used 
 * only when a file is created
 */
int pcsl_file_open(const pcsl_string * fileName, int flags, void **handle) {
    
    int fd;
    char* mode;
    const jbyte * pszOsFilename = pcsl_string_get_utf8_data(fileName);

    if (pszOsFilename == NULL) {
      return -1;
    }

    if (flags == O_RDONLY) {
        mode = "rb";
    } else if (flags == O_WRONLY) {
        mode = "wb";
    } else if (flags == RDWR_CREATE || flags == RDWR_CREATE_TRUNC || flags == RDWR_TRUNC) {
        mode = "w+b";
    } else {
        mode = "r+b";
    }

    fd = (int)fopen((char*)pszOsFilename, mode);

    pcsl_string_release_utf8_data(pszOsFilename, fileName);

    if (fd == 0) {
        *handle = NULL;
        return -1;
    } else {
        *handle  = (void *)fd;
        return 0;
    }
}


/**
 * The close function  loses the file with descriptor identifier in FS. 
 */
int pcsl_file_close(void *handle) 
{
    return fclose(handle);
}

/**
 * The read function reads up to size bytes from the file with descriptor identifier , 
 * storing the results in the buffer.
 */
int pcsl_file_read(void *handle, unsigned char *buf, long size)
{
    return fread(buf, 1, size, handle);
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
    return fwrite(buffer, 1, length, handle);
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

    status = remove((char*)pszOsFilename);

    pcsl_string_release_utf8_data(pszOsFilename, fileName);

    return status;
}


/**
 * The  truncate function is used to truncate the size of an open file in storage.
 */
int pcsl_file_truncate(void *handle, long size)
{
      return -1;
}

/**
 * The lseek function is used to change the file position of the file with descriptor 
 * identifier 
 */
long pcsl_file_seek(void *handle, long offset, long position)
{    
    return fseek(handle, offset, position);
}

/**
 * FS only need to support MIDLets to quiry the size of the file. 
 * Check the File size by file handle
 */
long pcsl_file_sizeofopenfile(void *handle)
{
    long size;
    
    fseek(handle, 0, SEEK_END);
    size = ftell(handle);
    fseek(handle, 0, SEEK_SET);// reset file
    return size;
}

/**
 * FS only need to support MIDLets to quiry the size of the file. 
 * Check the File size by file name
 */
long pcsl_file_sizeof(const pcsl_string * fileName)
{
     void *fd;
     long size;
     
     pcsl_file_open(fileName, PCSL_FILE_O_RDONLY, &fd);
     size = pcsl_file_sizeofopenfile(fd);
     pcsl_file_close(fd);
     return size;
}

/**
 * Check if the file exists in FS storage.
 */
int pcsl_file_exist(const pcsl_string * fileName)
{
    FILE* fd;
    int status = 0;
    const jbyte * pszOsFilename = pcsl_string_get_utf8_data(fileName);

    if (pszOsFilename == NULL) {
      return 0;
    }

    {
      const jsize len = strlen(pszOsFilename);
      // in special case of directories we always return false
      if (pszOsFilename && len > 0 && 
	  pszOsFilename[len - 1] == '/') {
	return 0;
      }
    }

    if ((fd = fopen((char*)pszOsFilename,"r")) !=0 ) {
       fclose(fd);
       status = 1;
    }     

    pcsl_string_release_utf8_data(pszOsFilename, fileName);

    return status;

}

/* Force the data to be written into the FS storage */
int pcsl_file_commitwrite(void *handle)
{
    return fflush(handle);
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
    }

    {
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
 * The opendir function opens directory named dirname. 
 */
void* pcsl_file_openfilelist(const pcsl_string * string)
{
    return (void *)pcsl_util_openfileiterator(string);
}

/**
 * The mkdir function closes the directory named dirname. 
 */
int pcsl_file_closefilelist(void *handle)
{
    return pcsl_util_closefileiterator(handle);
}

/**
 * The getFreeSpace function checks the size of free space in storage. 
 */
long pcsl_file_getfreespace()
{
    return 10000000;
}

/**
 * The getUsedSpace function checks the size of used space in storage. 
 */
long pcsl_file_getusedspace(const pcsl_string * systemDir)
{
    return 0;
}

/* The getNextEntry function search the next file which is  specified DIR */
int pcsl_file_getnextentry(void *handle, const pcsl_string * string, 
                           pcsl_string * result)
{

    *result = PCSL_STRING_NULL;
    return -1;
}

// The FILESEP and PATHSEP are returned dynamically by the
// emulator, depending on which OS you're running the emulator.
#define PROFILER_FILE_SEP_REG        ((unsigned int*)0x0d100028)
#define PROFILER_PATH_SEP_REG        ((unsigned int*)0x0d10002c)

jchar
pcsl_file_getfileseparator() {
    return (jchar)(*PROFILER_FILE_SEP_REG);
}

jchar
pcsl_file_getpathseparator() {
    return (jchar)(*PROFILER_PATH_SEP_REG);
}

void* pcsl_file_opendir(const pcsl_string * dirName) {
    printf(" unimplemented pcsl_file_opendir\n");
    return 0;
}

int pcsl_file_closedir(void *handle) {
    return 0;
}
