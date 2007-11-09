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

/**
 * @file
 */

/**
 * Functions and data structures that make up the Raw Memory File System 
 * interface. The RMFS  external interfaces hides any directory hierarchies by
 * making the file system look like a flat file system.
 *
 * <p>More specifically, for filenames, the storage implementation must
 * support 255 character long, 8-bit character, file names. Legal characters
 * are A-Z, a-z, 0-9, _, %, and #.  The filenames will differ in more than
 * just case, so case-insensitive systems like Win32 are not a problem.
 *
 */

#ifndef _RMFSAPI_H_
#define _RMFSAPI_H_


#ifdef __cplusplus
extern "C" {
#endif


typedef char* DIR;

/**
 * Maximum length of a file name, not including the terminator.
 */
#define MAX_FILENAME_LENGTH 255 /* does not include the zero terminator */

#define MAX_OPEN_FILE_NUM     (10)

typedef struct rmfsFileDescriptor
{
    jint  	         handle;
    jint                 size;
    jint                 offset;
    uchar                dataBlockIndex;
    uchar                nameID;
    jboolean             fileType;
    jboolean             flags;
    jboolean             createMode;

} _rmfsFileDes, *_rmfsFileDesPtr;

extern _rmfsFileDes   fileTab[MAX_OPEN_FILE_NUM];

/**
 * FUNCTION:      rmfsInitialize()
 * TYPE:          public operation
 * OVERVIEW:      Initialize the RMFS raw memory storage
 *                 NOTE: This must only be called once
 * INTERFACE:
 *   parameters:  size   Size of Raw Memory Storage to use
 *                start  start address of the Raw memory storage
 *   returns:     0 on succes; != 0 on failure
 *                
 */
/*jint rmfsInitialize(jint start, jint size); */

jint rmfsInitialize(jint start, jint size);


/* Initialize the Open File Table */
void rmfsInitOpenFileTab();

/* Refresh the RMFS storage */
jint rmfsRefresh (jint start, jint size);
/**
 * The open function creates and returns a new RMFS file identifier for the 
 * file named by filename. Initially, the file position indicator for the 
 * file is at the beginning of the file. The argument  creationMode  is used 
 * only when a file is created
 */
jint rmfsOpen (const char *filename, jint flags, jint creationMode);


/**
 * The function close closes the file with descriptor identifier in RMFS. 
 */
jint rmfsClose (jint identifier);

/**
 * The read function reads up to size bytes from the file with descriptor identifier , 
 * storing the results in the buffer.
 */
jint rmfsRead (jint identifier , void *buffer, jint size);

/**
 * The write function writes up to size bytes from buffer to the file with descriptor 
 * identifier. 
 * The return value is the number of bytes actually written. This is normally the same 
 * as size, but might be less (for example, if the persistent storage being written to
 * fills up).
 */
jint  rmfsWrite (jint identifier, void *buffer, jint size);

/**
 * The lseek function is used to change the file position of the file with descriptor 
 * identifier 
 */
jint  rmfsLseek (jint identifier, jint offset,jint whence);

/**
 * The  truncate function is used to truncate the size of an open file in storage.
 */
jint rmfsTruncate(jint identifier , jint size);

/**
 * RMFS only need to support MIDLets to quiry the size of the file.
 */
jint rmfsFileSize (jint identifier);

/**
 * Check if the file exists in RMFS storage.
 */
void rmfsFileExist (const char* filename, jint *status);

/**
 * Check if the dir exists in RMFS storage.
 */
void rmfsDirExist (char* filename, jint *status);

/**
 * The unlink function deletes the file named filename from the persistent storage.
 */
jint rmfsUnlink (const char *filename);

/**
 * The rename function updates the filename.
 */
jint rmfsRename (const char *oldFilename, const char* newFilename);

/**
 * The opendir function opens and returns a directory stream for reading the directory 
 * whose file name is dirname. The stream has type DIR *.
 */
DIR* rmfsOpendir (const char *dirname);

/**
 * This function closes the directory stream dirname 
 */
jint rmfsClosedir (char * dirname );


/* Check if the file reaches the end */
jint  rmfsFileEof(jint handle);

/* Write the Romized file jinto RMFS */
jint initFileSystem ();

/* Check if there is one file, whose name starts with one special stream */
char* rmfsFileStartWith(const char* filename);

/* Get the free space size in RMFS */
jint rmfsGetFreeSpace();

/* Get the used space size in RMFS */
jint rmfsGetUsedSpace();


/**
 * FUNCTION:      rmfsMkdir()
 * TYPE:          public operation
 * OVERVIEW:      create a Dir in the rmfs File System
 * INTERFACE:
 *   None   
 *
 *   returns:    >=0 Success, -1 failure
 *                
 */
jint rmfsMkdir();

/**
 * FUNCTION:      rmfsRmdir()
 * TYPE:          public operation
 * OVERVIEW:      delete a Dir in the rmfs File System
 * INTERFACE:
 *   None   
 *
 *   returns:    >=0 Success, -1 failure
 *                
 */
jint rmfsRmdir();
#ifdef __cplusplus
}
#endif

#endif /* _RMFSAPI_H_ */
