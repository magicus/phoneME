/**
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

#ifndef _PCSL_FILE_H_
#define _PCSL_FILE_H_

#include <pcsl_string.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @defgroup file File I/O Interface
 */

/**
 * @defgroup file_high_interface High Level Interface
 * @ingroup file
 */

/**
 * @defgroup file_low_interface Low Level Interface
 * @ingroup file
 */

/**
 * @file
 * @ingroup file
 */

/**
 * @addtogroup file_high_interface
 * @brief Interface for handling file system operations. \n
 * ##include <pcsl_file.h>
 *
 * @{
 *
 * This file defines the api for common file operations, like
 * opening, closing, deleting, renaming etc. In order to accomodate 
 * different implementations, the data structures returned by 
 * some of the functions are opaque, i.e. the calling routine need have no
 * knowledge of internal structures. The calling routines just pass
 * these data structures around. An example of this is the handle returned,
 * when opening a file. The handle is a void*, that the calling routine can then
 * use to read from, write to or close the file. So each platform, can design data 
 * structures that suit its needs.  
 * Similarly, there is no explicit notion of a "directory". There is a notion of
 * a "file list", that you can iterate through and get the file names one by 
 * one. Again the "file list" handle is a void* pointer. 
 * Hierarchial file systems that have a directory structure, can easily hide
 * the directory data structures in the implementation. 
 * All file names are 16-bit Unicode encoded strings, that need not be NULL
 * terminated. So wherever a fill name has to passed in, it's length will also
 * need to be passed in.
 *
 */

/** Out of memory error code. */
#define PCSL_OUT_OF_MEM_STATUS -2

/**
 *  File system Open control flags, to be passed to pcsl_file_open() 
 */

/**
 * Open the file for reading only.
 */

#define PCSL_FILE_O_RDONLY             0x00

/**
 * Open the file for writing only.
 */

#define PCSL_FILE_O_WRONLY             0x01

/**
 * Open the file for reading and writing.
 */

#define PCSL_FILE_O_RDWR               0x02

/**
 * Create the file  if  it  does  not  exist.
 * If the file exists, this flag has no effect except.
 */

#define PCSL_FILE_O_CREAT              0x40

/**
 * If the file exists and is a regular file, and the file
 * is successfully opened PCSL_O_RDWR or PCSL_O_WRONLY, its length
 * is  truncated  to  0.
 */

#define PCSL_FILE_O_TRUNC              0x200 

/**
 * If set, the file offset is set to the end of the file
 * prior to each write. So writing will append to the existing
 * contents of a file.
 */

#define PCSL_FILE_O_APPEND             0x400

/**  
 * Below definitions are for pcsl_file_seek's position parameter 
 */

/**
 *  The offset is set to the specified number of offset bytes. 
 */
#define PCSL_FILE_SEEK_SET  0

/**
 * The offset is set to its current location plus offset bytes. 
 */
#define	PCSL_FILE_SEEK_CUR  1

/**
 *  The offset is set to the size of the file plus offset bytes. 
 */
#define PCSL_FILE_SEEK_END  2


/**
 * Initializes the File System. 
 * For RMFS, this function 
 * specifies one contiguous memory block to be the File System storage. 
 * If PCSL_RAMFS_USE_STATIC is defined at compile time, then a static array of size
 * DEFAULT_RAMFS_SIZE is used as memory. If not defined, then memory is dynamically
 * allocated using pcsl_mem_malloc().
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_init();

/**
 * Cleans up resources used by file system.
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_finalize();

/**
 * The open function establishes a connection between a file and a
 * file identifier. It creates and returns a file identifier,
 * that is used by other I/O functions to refer to that file. 
 * Initially, the file position indicator for the file is at the
 * beginning of the file. 
 * @param fileName name of file to be opened
 *                 In case of hierarchial systems, this can be a path
 * @param flags open control flags
 *              Applications must specify exactly one of the first three
 *              values (file access modes) below in the value of "flags"
 *                  PCSL_O_RDONLY, PCSL_O_WRONLY, PCSL_O_RDWR
 *
 *              Any combination (bitwise-inclusive-OR) of the following may be used:
 *                  PCSL_O_CREAT, PCSL_O_TRUNC, PCSL_O_APPEND,
 *
 * @param handle address of pointer to file identifier
 *               on successful completion or NULL on failure,
 *               file identifier is returned in this argument.
 *               This identifier is platform specific and is opaque
 *               to the caller.
 * @return 0 on success, -1 otherwise
 * 
 */
int pcsl_file_open(const pcsl_string * fileName, int flags, void **handle);

/**
 * Closes the file with the specified identifier i.e.this identifier
 * is no longer associated with a file. Any pending writes, will be
 * commited.
 * @param handle identifier of file to be closed
 *               This is the identifier returned by pcsl_file_open()
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_close(void *handle);

/**
 * Reads up to a specified number of bytes from a file, 
 * storing the results in a buffer.
 * @param handle identifier of file to be read
 *               This is the identifier returned by pcsl_file_open()
 * @param buf buffer in which result is stored
 *            This has to be allocated in the calling routine
 * @param size number of bytes to be read. Actual number of bytes
 *              read may be less, if an end-of-file is encountered
 * @return the number of bytes actually read,
 *          returns -1 in case of an error,
 *          return 0, if end-of-file was reached by previous read
 */
int pcsl_file_read(void *handle, unsigned char *buf, long size);

/**
 * Writes up to a specified number of bytes from a buffer, 
 * to a specified file.
 * @param handle identifier of file to be written to
 *               This is the identifier returned by pcsl_file_open()
 * @param buf buffer to be written
 * @param size number of bytes to write
 * @return the number of bytes actually written. This is normally the same 
 *         as size, but might be less (for example, if the persistent storage being 
 *         written to fills up).
 */
int pcsl_file_write(void *handle, unsigned char* buf, long size);

/**
 * The unlink function deletes a file from the persistent storage.
 * @param fileName name of file to be deleted
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_unlink(const pcsl_string * fileName);

/**
 * The  truncate function is used to truncate the size of an open file in storage.
 * @param handle identifier of file to be truncated
 *               This is the identifier returned by pcsl_file_open()
 * @param size size to truncate to
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_truncate(void *handle, long size);

/**
 * Sets the file pointer associated with a file identifier 
 * @param handle identifier of file
 *               This is the identifier returned by pcsl_file_open()
 * @param offset number of bytes to offset file position by
 * @param position controls from where offset is applied, from 
 *                 the beginning, current position or the end
 *                 Can be one of
 *                 PCSL_SEEK_CUR, PCSL_SEEK_SET or PCSL_SEEK_END
 * @return on success the actual resulting offset from beginning of file
 *         is returned, otherwise -1 is returned
 */
long pcsl_file_seek(void *handle, long offset, long position);

/**
 * Get file size
 * @param handle identifier of file
 *               This is the identifier returned by pcsl_file_open()
 * @return size of file in bytes if successful, -1 otherwise
 */
long pcsl_file_sizeofopenfile(void *handle);

/**
 * Get file size
 * @param fileName name of file
 * @return size of file in bytes if successful, -1 otherwise
 */
long pcsl_file_sizeof(const pcsl_string * fileName);

/**
 * Check if the file exists in file system storage.
 * @param fileName name of file
 * @return 1 if it exists and is a regular file,
 *         0 otherwise (eg: 0 returned if it is a directory),
 *         -1 in case of an error.
 */
int pcsl_file_exist(const pcsl_string * fileName);

/**
 * Force the data to be written into the file system storage
 * @param handle identifier of file
 *               This is the identifier returned by pcsl_file_open()
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_commitwrite(void *handle);

/**
 * Renames file or directory.
 * @param oldName current name
 * @param oldNameLen current name length
 * @param newName new name
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_rename(const pcsl_string * oldName, const pcsl_string * newName);

/**
 * Returns a handle to a file list. This handle can be used in
 * subsequent calls to pcsl_file_getnextentry() to iterate through
 * the file list and get the names of files that match the given string.
 * @param string Typically the name of a directory, but it can be a
 *             partial file name
 * @return pointer to an opaque filelist structure, that can be used in
 *         pcsl_file_getnextentry() and pcsl_file_closefilelist.
 *         NULL is returned on any error, for example if root directory of the
 *         input 'string' cannot be found.
 */
void* pcsl_file_openfilelist(const pcsl_string * string);

/**
 * Closes the specified file list. This handle will no longer be
 * associated with this file list.
 * @param handle pointer to opaque filelist struct returned by
 *               pcsl_file_openfilelist 
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_closefilelist(void *handle);

/**
 * Checks the size of free space in storage.
 * Note: remove this method since it incorrect: it has no parameters
 *       (but it must has a parameter that specify storage),
 *       also this method has no implementation for any target now,
 *       and, at last, similar method was added to 'pcsl_directory'.
 * @return size of free space
 */
long pcsl_file_getfreespace();

/**
 * Checks the size of used space by all of the files
 * that are contained in the directory.
 * Note: the function does not consider files in subdirectories.
 * @param dirName name of directory
 * @return size of used spacereturns -1 in case of an error,
 */
long pcsl_file_getusedspace(const pcsl_string * dirName);

/**
 * Returns the filename in storage that begins with a given string.
 * The order is defined by the underlying file system.
 * This function needs to be repeatedly called for all next
 * occurances of the file that begins with a given string. 
 * The caller is responsible for freeing the returned string when done.
 *
 * @param handle pointer to filelist struct returned by pcsl_file_openfilelist
 * @param string file name to be matched
 * @param result if matching directory is found, name returned in result
 * @return 0 on success, -1 otherwise
 */
int pcsl_file_getnextentry(void *handle, const pcsl_string * string, 
			   pcsl_string * result);

/**
 *  Returns file separator character used by the underlying file system
 * The file separator, typically, is a single character that separates
 * directories in a path name, for example <dir1>/<dir2>/file.c
 * @return 16-bit Unicode encoded file separator
 */
jchar pcsl_file_getfileseparator();

/**
 * Returns path separator character used by the underlying file system.
 * @return 16-bit Unicode encoded path separator
 */
jchar pcsl_file_getpathseparator();


/** @} */

/**
 *
 * @addtogroup file_low_interface
 * @brief Low Level Interface using standard Posix functions \n
 * In Posix compliant systems, these functions are usually defined in \n
 * ##include <fcntl.h>
 *
 * @{ 
 *
 * If the supplied <b>posix</b> file module is being used, then the target platform
 * needs to supply the following Posix file manipulation functions: \n
 *
 * Opens a specified file and establishes a connection between the open
 * file and a file descriptor or handle, that is returned.
 *
 * @param filename name of file to be opened
 * @param flags integer open control flags. This is bitwise-inclusive-OR of \n
 *              flags from the following list. \n
 *              Any one of these 3 flags:\n 
 *              #PCSL_FILE_O_RDONLY, #PCSL_FILE_O_WRONLY, #PCSL_FILE_O_RDWR, \n 
 *              Any combination of these three:\n
 *              #PCSL_FILE_O_CREAT, #PCSL_FILE_O_APPEND, #PCSL_FILE_O_TRUNC \n 
 * @param  mode creation mode. Needs to be specified only if a new file
 *              is being created (see #PCSL_FILE_O_CREAT), in which case, it
 *              has a value of (0444 | 0222), i.e. all have read-write
 *              permission. This is the only value that needs to be supported. 
 * @return an integer handle to the open file. -1 returned on failure.
 *
 * <b>int open(char *filename, int flags, int mode);</b>
 *
 * Closes the file connected to a file descriptor i.e. the specified file
 * handle will no longer be associated with a file and this handle can be
 * returned by subsequent open() calls.
 *
 * @param handle file handle previously returned by open()
 * @return 0 on success, -1 on failure
 *
 * <b>int close(int handle);</b>
 *
 * Reads up to a specified number of bytes from a file, 
 * storing the results in a buffer.
 * @param handle handle of file to be read
 *               This is the handle returned by open()
 * @param buf buffer in which result is stored
 *            This has to be allocated in the calling routine
 * @param size number of bytes to be read. Actual number of bytes
 *              read may be less, if an end-of-file is encountered
 * @return the number of bytes actually read. -1 on failure
 *
 * <b>int read(int handle, unsigned  char *buf, unsigned int size); </b>
 *
 * Writes up to a specified number of bytes from a buffer, 
 * to a specified file.
 * @param handle handle of file to be written to
 *               This is the identifier returned by open()
 * @param buf buffer to be written
 * @param size number of bytes to write
 * @return the number of bytes actually written. This is normally the same 
 *         as size, but might be less (for example, if the persistent storage being 
 *         written to fills up). -1 on failure
 *
 * <b>int write(int handle, unsigned char* buf, long size);</b>
 *
 * The unlink function deletes a file from the persistent storage.
 * @param fileName name of file to be deleted
 * @return 0 on success, -1 otherwise
 *
 * <b>int unlink(char *fileName); </b>
 *
 * The ftruncate function is used to truncate the size of an open file in storage.
 * @param handle handle of file to be truncated
 *		 This is the handle returned by open()
 * @param size size to truncate to
 * @return 0 on success, -1 otherwise
 *
 * <b>int ftruncate(int handle, int size);</b>
 *
 * Sets the file pointer associated with a file identifier
 * @param handle handle of file
 *		 This is the handle returned by open()
 * @param offset number of bytes to offset file position by
 * @param position controls from where offset is applied, from
 *		   the beginning, current position or the end
 *		   Can be one of
 *		   #PCSL_FILE_SEEK_CUR, #PCSL_FILE_SEEK_SET or #PCSL_FILE_SEEK_END
 * @return on success the actual resulting offset from beginning of file
 *	   is returned, otherwise -1 is returned
 *
 * <b>long lseek(int handle, long offset, long position);</b>
 *
 * Obtains information about an open file, like its size, etc.
 *
 * @param handle handle of file.
 *		 This is the handle returned by open()
 * @param stat_buf pointer to a stat struct, defined in
 *                 <sys/stat.h> on Posix compliant systems.
 *                 Gathered information is put into this structure.
 *                 stat_buf->st_size is the size of file in bytes.
 * @return 0 on success, -1 otherwise
 *
 * <b>int fstat(int handle, struct stat *stat_buf)</b>
 *
 * Obtains information about an open file, like its size, etc.
 * The difference between stat and fstat, is that with stat,
 * read, write or execute permission of the file is not required.
 *
 * @param handle handle of file.
 *		 This is the handle returned by open()
 * @param stat_buf pointer to a stat struct, defined in
 *                 <sys/stat.h> on Posix compliant systems.
 *                 Gathered information is put into this structure.
 *                 stat_buf->st_size is the size of file in bytes.
 * @return 0 on success, -1 otherwise. A return value of 0,
 *         also indicates that the file exists.
 *
 * <b>int stat(int handle, struct stat *stat_buf)</b>
 *
 * Renames the filename.
 * @param oldName current name of file
 * @param newName new name of file
 * @return 0 on success, -1 otherwise
 *
 * <b>int rename(char *oldName, char *newName);</b>
 *
 * Opens a directory stream corresponding to the named directory 
 *
 * @param dirName name of directory to be opened
 * @return Upon successful completion, opendir() returns
 *         a  pointer  to  an  object  of  type  DIR, defined in <dirent.h>
 *         Otherwise, a null pointer is returned.
 *
 * <b>DIR *opendir(const char *dirName);</b>
 *
 * Closes a directory stream.
 *
 * @param dirp pointer to a DIR object, returned by opendir()
 * @return 0 on success, -1 otherwise
 *
 * <b>int closedir(DIR *dirp);</b>
 *
 * The readdir() function returns  a  pointer  to  a  structure
 * representing  the directory entry at the current position in
 * the directory stream specified by  the  argument  dirp,  and
 * positions the directory stream at the next entry. It returns
 * a null pointer  upon  reaching  the  end  of  the  directory
 * stream.  The  structure  dirent  defined  by  the <dirent.h>
 * header describes a directory entry.
 * If entries for . (dot) or .. (dot-dot) exist, one entry will
 * be  returned for dot and one entry will be returned for dot-
 * dot; otherwise they will not be returned.
 * The pointer returned by readdir() points to data  which  may
 * be  overwritten  by  another  call  to readdir() on the same
 * directory stream. This data is not  overwritten  by  another
 * call to readdir() on a different directory stream.
 *
 * @param dirp pointer to directory stream
 * @return pointer to a directory entry structure at the current position,
 *         NULL when end of directory is reached.
 *
 * <b>struct dirent *readdir(DIR *dirp);</b>
 *
 * <b>NOTE</b> Besides the above functions, the posix file module
 * also uses pcsl_mem_malloc() and pcsl_mem_free() and so the
 * low level functions required by these two functions, will need to
 * be ported, when porting the posix module. \n 
 * The ram file module, is only dependent on pcsl_mem_malloc() and
 * pcsl_mem_free() and so these are the only 2 functions that you need
 * port.
 */ 
/** @} */
 
#ifdef __cplusplus
}
#endif

#endif /* _PCSLFILE_H_ */
