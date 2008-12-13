/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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


/* rmfsOpen control flags  */
#define O_ACCMODE          0003
#define O_RDONLY             00
#define O_WRONLY             01
#define O_RDWR               02
#define O_CREAT            0100 /* not fcntl */
#define O_EXCL             0200 /* not fcntl */
#define O_NOCTTY           0400 /* not fcntl */
#define O_TRUNC           01000 /* not fcntl */
#define O_APPEND          02000
#define O_NONBLOCK        04000
#define O_NDELAY        O_NONBLOCK
#define O_SYNC           010000
#define FASYNC           020000 /* fcntl, for BSD compatibility */
#define O_DIRECT         040000 /* direct disk access hint */
#define O_LARGEFILE     0100000
#define O_DIRECTORY     0200000 /* must be a directory */
#define O_NOFOLLOW      0400000 /* don't follow links */
#define O_ATOMICLOOKUP  01000000 /* do atomic file lookup */


/* I/O Modes */
/**
 * Open the file for reading only.
 */
#define OPEN_READ (1)

/**
 * Open the file for writing only; create the file if it does not
 * exist.
 */
#define OPEN_WRITE (2)

/**
 * Open the file for reading and writing; create the file if it does
 * not exist.
 */
#define OPEN_READ_WRITE (OPEN_READ|OPEN_WRITE)

/**
 * Open the file for reading and writing, creating it if it does not
 * exist and truncating it to 0 bytes if it does exist.
 */
#define OPEN_READ_WRITE_TRUNCATE (-OPEN_READ_WRITE)

/* Below definitions is for lseek whence parameter */
#define SEEK_SET  0 /* The offset is set to offset bytes. */

#define	SEEK_CUR  1 /* The offset is set to its current location plus offset bytes. */

#define SEEK_END  2 /* The offset is set to the size of the file plus offset bytes. */

#define MAX_OPEN_FILE_NUM     (10)

typedef struct rmfsFileDescriptor
{
    int  	        handle;
    int                 size;
    int                 offset;
    unsigned short      dataBlockIndex;
    unsigned short      nameID;
    unsigned char       fileType;
    unsigned char       flags;
    unsigned char       createMode;

} _rmfsFileDes, *_rmfsFileDesPtr;

/* Below structure is definied for supporting ROMized FS */
typedef struct { 
    int index;                    // index in fs_image_table
    const unsigned char * data;   // start of data
    int length;                   // length of data
    int pos;                      // current position
} OsFile;

extern _rmfsFileDes   fileTab[MAX_OPEN_FILE_NUM];

/* Initialize the Open File Table */
void rmfsInitOpenFileTab();

/* Refresh the RMFS storage */
int rmfsRefresh (int start, int size);
/**
 * The open function creates and returns a new RMFS file identifier for the 
 * file named by filename. Initially, the file position indicator for the 
 * file is at the beginning of the file. The argument  creationMode  is used 
 * only when a file is created
 */
int rmfsOpen (const char *filename, int flags, int creationMode);


/**
 * The function close closes the file with descriptor identifier in RMFS. 
 */
int rmfsClose (int identifier);

/**
 * The read function reads up to size bytes from the file with descriptor identifier , 
 * storing the results in the buffer.
 */
int rmfsRead (int identifier , void *buffer, int size);

/**
 * The write function writes up to size bytes from buffer to the file with descriptor 
 * identifier. 
 * The return value is the number of bytes actually written. This is normally the same 
 * as size, but might be less (for example, if the persistent storage being written to
 * fills up).
 */
int  rmfsWrite (int identifier, const void *buffer, int size);

/**
 * The lseek function is used to change the file position of the file with descriptor 
 * identifier 
 */
int  rmfsLseek (int identifier, int offset,int whence);

/**
 * The  truncate function is used to truncate the size of an open file in storage.
 */
int rmfsTruncate(int identifier , int size);

/**
 * RMFS only need to support MIDLets to quiry the size of the file.
 */
int rmfsFileSize (int identifier);

/**
 * Check if the file exists in RMFS storage.
 */
void rmfsFileExist (const char* filename, int *status);

/**
 * Check if the dir exists in RMFS storage.
 */
void rmfsDirExist (char* filename, int *status);

/**
 * The unlink function deletes the file named filename from the persistent storage.
 */
int rmfsUnlink (const char *filename);

/**
 * The rename function updates the filename.
 */
int rmfsRename (const char *oldFilename, const char* newFilename);

/**
 * The opendir function opens and returns a directory stream for reading the directory 
 * whose file name is dirname. The stream has type DIR *.
 */
DIR rmfsOpendir (const char *dirname);

/**
 * This function closes the directory stream dirname 
 */
int rmfsClosedir (char * dirname );


/* Check if the file reaches the end */
int  rmfsFileEof(int handle);

/* Write the Romized file into RMFS */
int initFileSystem ();

/* Check if there is one file, whose name starts with one special stream */
char* rmfsFileStartWith(const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* _RMFSAPI_H_ */
