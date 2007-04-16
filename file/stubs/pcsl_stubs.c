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

static jchar FILESEP = '/';
static jchar PATHSEP = ':';

/**
 * The initialize function initials the File System
 */
int pcsl_file_init() {
    return -1;
}

/**
 * Cleans up resources used by file system. It is only needed for the 
 * Ram File System (RMFS).
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_finalize() {
    return -1;
}

/**
 * The open function creates and returns a new file identifier for the 
 * file named by filename. Initially, the file position indicator for the 
 * file is at the beginning of the file. The argument  creationMode  is used 
 * only when a file is created
 */
int pcsl_file_open(const pcsl_string * fileName, int flags, void **handle) {
	
    return -1;
}


/**
 * The close function  loses the file with descriptor identifier in FS. 
 */
int pcsl_file_close(void *handle) 
{
	return -1;
}

/**
 * The read function reads up to size bytes from the file with descriptor identifier , 
 * storing the results in the buffer.
 */
int pcsl_file_read(void *handle, unsigned  char *buf, long size)
{
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
    return -1;
}

/**
 * The unlink function deletes the file named filename from the persistent storage.
 */
int pcsl_file_unlink(const pcsl_string * fileName)
{
    return -1;
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
	return -1;
}

/**
 * FS only need to support MIDLets to quiry the size of the file. 
 * Check the File size by file handle
 */
long pcsl_file_sizeofopenfile(void *handle)
{
 
     return -1;
}

/**
 * FS only need to support MIDLets to quiry the size of the file. 
 * Check the File size by file name
 */
long pcsl_file_sizeof(const pcsl_string * fileName)
{
     return -1;
}

/**
 * Check if the file exists in FS storage.
 */
int pcsl_file_exist(const pcsl_string * fileName)
{
    return -1;
}

/* Force the data to be written into the FS storage */
int pcsl_file_commitwrite(void *handle)
{
	return -1;
}


/**
 * The rename function updates the filename.
 */
int pcsl_file_rename(const pcsl_string * oldName, 
		     const pcsl_string * newName)
{
    return -1;
}

/**
 * The opendir function opens directory named dirname. 
 */
void* pcsl_file_openfilelist(const pcsl_string * string)
{

    return NULL;
}

/**
 * The mkdir function closes the directory named dirname. 
 */
int pcsl_file_closefilelist(void *handle)
{
	return -1;
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
    return 0;
	
}

/* The getNextEntry function search the next file which is  specified DIR */
int pcsl_file_getnextentry(void *handle, 
			   const pcsl_string * string, 
			   pcsl_string * result)
{

    *result = PCSL_STRING_NULL;

    return -1;

}

jchar
pcsl_file_getfileseparator() {
    return FILESEP;
}

jchar
pcsl_file_getpathseparator() {
    return PATHSEP;
}

