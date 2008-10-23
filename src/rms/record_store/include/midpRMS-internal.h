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
 * Checks whether a record-store file of the given name for the given
 * MIDlet suite exists.
 *
 * @param filenameBase MIDlet suite's identifier
 * @param name name of the record store
 * @param extension platform-specific extension for the record-store file
 *
 * @return 1 if the file exists, 0 if it does not
 */
int rmsdb_record_store_exists(pcsl_string* filenameBase,
                              const pcsl_string* name,
                              int extension);

/**
 * Removes the record-store file of the given name for the given MIDlet
 * suite, if it exists.
 *
 * @param ppszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (this function sets <tt>ppszError</tt>'s value).
 * @param filenameBase MIDlet suite's identifier
 * @param name name of the record store
 * @param extension platform-specific extension for the record-store file
 *
 * @return 1 if successful
 *         0 if an IOException occurred
 *        -1 if file is locked by another isolate
 *        -2 if out of memory error occurs
 *
 */
int rmsdb_record_store_delete(char** ppszError,
                              pcsl_string* filenameBase,
                              const pcsl_string* name,
                              int extension);

/**
 * Gets the number of record stores owned by the given MIDlet suite.
 *
 * @param filenameBase MIDlet suite's identifier
 *
 * @return number of record stores or <tt>OUT_OF_MEM_LEN</tt> if the
 * MIDP implementation has run out of memory
 */
int rmsdb_get_number_of_record_stores(pcsl_string *filenameBase);


/**
 * Gets the names of the record stores owned by the given MIDlet
 * suite.
 *
 * @param filenameBase MIDlet suite's identifier
 * @param ppStoreNames pointer to an array that will hold record-store
 *        names. This function sets ppStoreNames' value
 *
 * @return the number of record store names or <tt>OUT_OF_MEM_LEN</tt>
 * if the MIDP implementation has run out of memory
 */
int rmsdb_get_record_store_list(pcsl_string* filenameBase,
                                pcsl_string* *const ppNames);

/**
 * Gets the approximate number of bytes of RMS space available to the
 * given suite.
 *
 * <p><b>Usage Warning:</b> This may be a slow operation if the
 * platform must look at the size of every file in the MIDP memory
 * space.</p>
 *
 * @param storageId storage id
 *
 * @return the approximate space available to grow the record store,
 *         in bytes
 */
long rmsdb_get_new_record_store_space_available(int storageId);

/**
 * Opens a native record-store file.
 *
 * @param pszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (this function sets <tt>ppszError</tt>'s value).
 * @param filenameBase  MIDlet suite's identifier
 * @param name name of the record-store file
 * @param extension platform-specific extension for the record-store file
 *
 * @return storage handle on success
 *  return -1 if there is an error getting the filename
 *  return -2 if the file is locked
 *
 */
int rmsdb_record_store_open(char** ppszError,
                            pcsl_string* filenameBase,
                            const pcsl_string * name_str,
                            int extension);

/**
 * Frees the given error message; does nothing if null is passed.
 * The error message is expected to have been returned from a
 * record-store function.  This function is for systems that
 * dynamically allocate error strings.
 *
 * @param pszError an I/O error message
 */
void recordStoreFreeError(char* pszError);

/**
 * Returns the approximate number of bytes of RMS space available for the
 * given record-store file of the given MIDlet suite.
 *
 * <p><b>Usage Warning:</b> This may be a slow operation if the
 * platform looks at the size of every file stored in the MIDP memory
 * space.
 *
 * @param handle handle to a record-store file
 * @param storageId id of the midlets storage
 *
 * @return approximate space available to grow the record-store file,
 * in bytes
 */
long rmsdb_get_record_store_space_available(int handle, int storageId);

/**
 * Changes the read/write position in the given open record-store file
 * to the given position.
 *
 * @param pszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (this function sets <tt>ppszError</tt>'s value).
 * @param handle handle to the open record-store file
 * @param absolutePosition position, in bytes from the beginning of
 *        the record-store file, to move the current-position pointer
 *        to
 */
void recordStoreSetPosition(char** pszError, int handle, int absolutePosition);

/**
 * Writes to the given open record-store file. This function is
 * atomic: it will write all or none of the given bytes. If it writes
 * none, it will pass back an error in the given pointer to a
 * string.
 *
 * @param pszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (this function sets <tt>ppszError</tt>'s value).
 * @param handle handle to the open record-store file
 * @param bytes array of characters to write into storage
 * @param length the number of bytes to write (the length of buffer)
 */
void recordStoreWrite(char** pszError, int handle, char* bytes, long length);

/**
 * Commits pending writes to the record-store file. If the commit fails,
 * it will pass back an error in the given pointer to a string.
 *
 * @param pszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (this function sets <tt>ppszError</tt>'s value).
 * @param handle handle to the open record-store file
 */
void recordStoreCommitWrite(char** pszError, int handle);

/**
 * Reads from the given open record-store file, and places the results
 * in the given buffer. This function might read less than the length
 * of the buffer; it returns the number of bytes it actually
 * read.
 *
 * @param pszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (this function sets <tt>ppszError</tt>'s value).
 * @param handle handle to the open record-store file
 * @param bytes array to hold the information read (This function
 *        sets bytes' value)
 * @param length of the given bytes array
 *
 * @return the number of bytes read, or -1 if there were no bytes to
 * be read in the record-store file
 */
long recordStoreRead(char** pszError, int handle, char* bytes, long length);

/**
 * Closes a record-store file opened by rmsdb_record_store_open.
 *
 * @param pszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (this function sets <tt>ppszError</tt>'s value).
 * @param handle handle to the open record-store file.
 */
void recordStoreClose(char** pszError, int handle);

/**
 * Truncates the size of the given open record-store file to the given
 * number of bytes.
 *
 * @param pszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (this function sets <tt>ppszError</tt>'s value).
 * @param handle handle to the open record-store file
 * @param size new size of the file, in bytes
 */
void recordStoreTruncate(char** pszError, int handle, long size);


/**
 * Returns the size, in bytes, of the given open record-store
 * file.
 *
 * @param pszError pointer to a string that will hold an error message
 *        if there is a problem, or null if the function is
 *        successful (this function sets <tt>ppszError</tt>'s value).
 * @param handle handle to the open record-store file
 *
 * @return the size, in bytes, of the given open record-store file, or
 * 0 if there was an error (for example, if the given handle was not
 * valid)
 */
long recordStoreSizeOf(char** ppszError, int handle);

/**
 * Returns true if the suite has created at least one record store.
 * Used by an OTA installer.
 *
 * @param filenameBase ID of the suite
 *
 * @return true if the suite has at least one record store
 */
int rmsdb_suite_has_rms_data(pcsl_string* filenameBase);

/* @} */
