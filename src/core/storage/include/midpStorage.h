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
 * @brief Interface for managing persistent storage
 *
 * <p>The native-storage module hides any directory hierarchies by
 * making the file system look like a flat file system.</p>
 *
 * <p>Internal Java platform classes prefix all file names with a storage-root
 * string, and do not use directory separators. RMS adds a suite-unique string
 * to the storage-root string, also without using file separators.</p>
 *
 * <p>For filenames, the storage implementation must
 * support 255 character long, 8-bit character, file names. Legal characters
 * are A-Z, a-z, 0-9, _, %, and #.  The filenames will differ in more than
 * just case, so case-insensitive systems like Win32 are not a problem.</p>
 *
 * <p>The I/O modes for opening files are defined in the classes
 * <nobr><tt>javax.microedition.io.Connector.java</tt></nobr> and
 * <nobr><tt>com.sun.midp.io.j2me.storage.RandomAccessStream</tt></nobr>.</p>
 */

#ifndef _MIDPSTORAGE_H_
#define _MIDPSTORAGE_H_

#include <midpString.h>
#include <java_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maximum length of a file name, not including the terminator.
 */
#define MAX_FILENAME_LENGTH 255 /* does not include the zero terminator */

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

/* Common file stat tests */
#ifndef S_ISREG

/**
 * file is regular flag
 */
#define S_ISREG(mode)   ( ((mode) & S_IFMT) == S_IFREG )

/**
 * special character flag
 */
#define S_ISCHR(mode)   ( ((mode) & S_IFMT) == S_IFCHR )

/**
 * fifo stream flag
 */
#define S_ISFIFO(mode)  ( ((mode) & S_IFMT) == S_ISFIFO )

/**
 * directory flag
 */
#define S_ISDIR(mode)   ( ((mode) & S_IFMT) == S_IFDIR )

/**
 * blocking flag
 */
#define S_ISBLK(mode)   ( ((mode) & S_IFMT) == S_IFBLK )
#endif /* _S_ISREG */

/** Type definition for the storage identifier */
typedef jint StorageIdType;

/**
 * Initializes the storage subsystem.
 *
 * @param midp_config pathname in the file-system where MIDP is installed
 * @param midp_home file system path to where MIDP should store its
 * data like suites and such
 *
 * @return 0 for success, or non-zero if the MIDP implementation is
 * out of memory
 */
int storageInitialize(char *midp_config, char * midp_home);

/**
 * Takes any actions necessary to safely terminate the storage
 * subsystem.
 */
void storageFinalize();

/**
 * Sets the amount of total storage space allocated to MIDlet suites to the
 * given number of bytes. The given value may be different from the system
 * default.  This value is used to enforce a maximum size limit for the
 * amount of space that can be used by installed MIDlet suites and
 * their record stores.
 *
 * <p>Do not call this function from <tt>storageInitialize</tt> if you
 * get this function's parameter value from the configuration
 * system. The configuration system cannot be used until
 * <tt>storageInitialize</tt> returns.
 *
 * @param space total amount of file space allocated to MIDP, in bytes
 */
void storageSetTotalSpace(long space);

/**
 * Returns the file separator as a string.
 *
 * <p><b>Note:</b> This function is only called by the <tt>main</tt> function
 * to manage files outside of the simulated storage area.
 *
 * @return the system-specific file separator
 */
jchar storageGetFileSeparator();

/**
 * Returns the class-path separator as a string.
 *
 * <p><b>Note:</b> This function is only called by the <tt>main</tt> function
 * to build a class path.
 *
 * @return the system-specific class-path separator
 */
jchar storageGetPathSeparator();

/**
 * Returns root string that all files should begin with, including
 * a trailing file separator if needed. By including the any trailing file
 * separators the Java API does not need to know about file separators
 * or subdirectories.
 *
 * Since the lifetime of the returned object is from storageInitialize
 * until storageFinalize, you do not have to free the returned object.
 *
 * @param storageId ID of the storage the root of which must be returned
 *
 * @return prefix used for all file names in the given storage.
 *         It may be empty, but not PCSL_STRING_NULL.
 */
const pcsl_string* storage_get_root(StorageIdType storageId);

/**
 * Returns the root string that configuration files start with, including a
 * trailing file separator when necessary. If the string includes a trailing
 * file separator, callers do not need access to file separators or
 * subdirectories.
 *
 * Since the lifetime of the returned object is from initializeConfigRoot
 * until storageFinalize, you do not have to free the returned object.
 *
 * @param storageId ID of the storage the config root of which must be returned
 *
 * @return prefix used for all configuration file names. It may be empty,
 *         but not PCSL_STRING_NULL
 */
const pcsl_string* storage_get_config_root(StorageIdType storageId);

/**
 * Frees an error string, or does nothing if null is passed in. The
 * string is expected to have been returned from a storage function.
 * This function allows for systems that provide error messages that
 * are dynamically allocated.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 */
void storageFreeError(char* pszError);

/**
 * Opens a native-storage file with the given name in the given mode.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 * @param filename the name of the file to open
 * @param ioMode the mode in which to open the file (The I/O mode
 *        constants are defined in this file.)
 *
 * @return a platform-specific handle to the open file
 */
int storage_open(char** ppszError, const pcsl_string* filename, int ioMode);

/**
 * Closes a native-storage file opened by storage_open.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 * @param handle handle to the open file
 */
void storageClose(char** ppszError, int handle);

/**
 * Reads from the given open native-storage file, and places the
 * results in the given buffer.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 * @param handle handle to the open file
 * @param buffer array to hold the information read (This function
 *        sets buffer's value.)
 * @param length of the given bytes array
 *
 * @return the number of bytes read (which could be fewer than the bytes
 * available in the given buffer), or -1 if there were no bytes to be read in
 * the native-storage file
 */
long storageRead(char** ppszError, int handle, char* buffer, long length);

/**
 * Writes to the given open native-storage file.  This function is
 * atomic: it will write all or none of the given bytes. If it writes
 * none, it will pass back an error in the given string pointer.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 * @param handle handle to the open native-storage file
 * @param buffer array of characters to write into storage
 * @param length the number of bytes to write (the length of buffer)
 */
void storageWrite(char** ppszError, int handle, char* buffer, long length);

/**
 * Commits or flushes pending writes to the file system.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 * @param handle handle to the open native-storage file
 */
void storageCommitWrite(char** ppszError, int handle);

/**
 * Changes the read/write position in the given open native-storage
 * file to the given position.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value)
 * @param handle handle to the open native-storage file
 * @param absolutePosition position to move the current-position pointer to
 *        in bytes, counting from the beginning of the native-storage file
 */
void storagePosition(char** ppszError, int handle, long absolutePosition);

/**
 * Changes the read/write position in the given open native-storage
 * file by the given number of bytes.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 * @param handle handle to the open native-storage file
 * @param offset number of bytes to move the current-position pointer,
 *        counting from the pointer's current position in the
 *        native-storage file
 *
 * @return the new absolute read/write position
 */
long storageRelativePosition(char** ppszError, int handle, long offset);

/**
 * Returns the size of the given open native-storage file.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 * @param handle handle to the open native-storage file
 *
 * @return the size, in bytes, of the given open native-storage file,
 * or 0 if there was an error (for example, if the given handle was
 * not valid)
 */
long storageSizeOf(char** ppszError, int handle);

/*
 * Return the size of file with the given name in storage.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 *
 * @return size of file in bytes if successful, -1 otherwise
 */
long storage_size_of_file_by_name(char** ppszError,
                                  const pcsl_string* pFileName);

/**
 * Truncates the size of the given open native-storage file to the
 * given number of bytes.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 * @param handle handle to the open native-storage file
 * @param size new size of the file, in bytes
 */
void storageTruncate(char** ppszError, int handle, long size);

/**
 * Returns the amount of free space of native-file storage, in
 * bytes.
 *
 * @param storageId ID of the storage to check for free space
 *
 * @return the number of bytes of available free space
 */
jlong storage_get_free_space(StorageIdType storageId);

/**
 * Checks whether a native-storage file of the given name exists.
 *
 * @param filename name of the file that might exist
 *
 * @return a non-zero integer if the file exists; 0 otherwise
 */
int storage_file_exists(const pcsl_string* filename);

/**
 * Renames a native-storage file from the given current name to the given new
 * name.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 * @param oldFilename the current name of the file
 * @param newFilename the name to change the file to
 */
void storage_rename_file(char** ppszError, const pcsl_string* oldFilename,
                         const pcsl_string* newFilename);

/**
 * Deletes the given native-storage file. This function does not
 * block.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (This function sets <tt>ppszError</tt>'s value.)
 * @param filename the name of the file to delete
 */
void storage_delete_file(char** ppszError,  const pcsl_string* filename);

/*
 * Returns the handle that represents the savedRootlength, savedDirectory
 * etc. This handle needs to be passed to storage_get_next_file_in_iterator()
 * in order to get the filename that matches with a given string. In
 * order to clean-up the memory storageCloseFileIterator() must be
 * called to close the handle properly.
 *
 *
 * Returns NULL if memory allocation fails for MidpStorageDirInfo structure
 * or root directory of the input 'string' can not be found
 *
 */
void* storage_open_file_iterator(const pcsl_string* string_str);

/*
 * Return the filename in storage that begins with a given string.
 * The order is defined by the underlying file system.
 * This function needs to be repeatedly called for all next
 * occurrents of the file that begins with a given string.
 *
 * Returns NULL_MIDP_STRING if no filename matches.
 *
 */
int storage_get_next_file_in_iterator(const pcsl_string* string_str, void* handle, pcsl_string* result_str);

/*
 * Close the handle i.e. pointer to MidpStorageDirInfo properly
 * and deallocate the memory
 *
 */
void storageCloseFileIterator(void* handle);

/**
 * Read pcsl_string from storage.
 * First read a jint with length, then read the text
 * of that length.
 *
 * @param ppszError  in the case of error, receives address of a string
 *                   describing the problem; receives NULL in the case of success
 * @param handle     handle of the file to read from
 * @param str        string to receive the text
 */
void
storage_read_utf16_string(char** ppszError, int handle, pcsl_string* str);

/**
 * Write pcsl_string to storage.
 * First write a jint with length, then the text.
 *
 * @param ppszError  in the case of error, receives address of a string
 *                   describing the problem
 * @param handle     handle of the file to write to
 * @param str        string to be written
 */
void
storage_write_utf16_string(char** ppszError, int handle, const pcsl_string* str);

#ifdef __cplusplus
}
#endif

#endif /* _MIDPSTORAGE_H_ */
