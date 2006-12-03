/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _JAR_H_
#define _JAR_H_
/**
 * @defgroup ams_installer Installer Interface
 * @ingroup ams
 */

/**
 * @file
 * ingroup ams_installer
 *
 * @brieve This is a standalone JAR extraction utility, 
 * that allows any file or memory management to be plugged in.
 */

/**
 * @name FileObj: the file object
 * Although C has no built-in OOP, an OOP object may be
 * defined in C as a structure containing object state
 * and pointers to member functions.
 * @{
 */
/**
 * The type of file-size member function for the FileObj data type.
 *
 * @param state file handle, stored in the <var>state</var> field of FileObj
 * @return the size of the file, less than zero is an error.
 * @see _FileObj
 */
typedef long (*FileSizeFunction)(void* state);

/**
 * The type of read-file member function for the FileObj data type.
 *
 * Tries to read the requested number of characters, returns the number read or
 * less than zero if an error.
 *
 * @param state file handle, stored in the <var>state</var> field of FileObj
 * @param buffer location to store characters
 * @param numberOfChars requested number of characters
 * @return the number of characters read or less than zero if an error
 * @see _FileObj
 */
typedef long (*FileReadFunction)(void* state, unsigned char* buffer,
                                long numberOfChars);

/**
 * The type of file-seek member function for the FileObj data type.
 *
 * Sets the file position. The new position, measured in bytes, is obtained
 * by adding offset bytes to the position specified by whence.
 * If whence is set to SEEK_SET, SEEK_CUR, or SEEK_END, the offset
 * is relative to the start of the file, the current position,
 * or end-of-file, respectively.
 *
 * @param state file handle, stored in the <var>state</var> field of FileObj
 * @param offset new position relative to whence
 * @param whence SEEK_SET, SEEK_CUR, or SEEK_END denoting
 *              start of file, current position, or end of file, respectively
 * @return zero on success, -1 on error
 * @see fseek
 * @see _FileObj
 */
typedef int (*FileSeekFunction)(void* state, long offset, int whence);

/**
 * The read-single-character function for the FileObj data type.
 *
 * Reads the next character from file and returns it as an unsigned
 * char cast to an int, or less than zero on end of file or error
 *
 * @param state file handle, stored in the <var>state</var> field of FileObj
 * @return a unsigned char in an int or less than zero if an error.
 * @see _FileObj
 */
typedef int (*FileReadCharFunction)(void* state);

/** The structure that represents a file object. */
typedef struct _FileObj {
    void* state;           /**< file handle, it gets passed to the member
                            * functions <var>size</var>, <var>read</var>,
                            * <var>seek</var>, <var>readChar</var> */
    FileSizeFunction size; /**< get file size. @see FileSizeFunction */
    FileReadFunction read; /**< read from file. @see FileReadFunction */
    FileSeekFunction seek; /**< reposition file. @see FileSeekFunction */
    FileReadCharFunction readChar; /**< read single character from file.
                                    *   @see FileReadCharFunction */
} FileObj;

/** @} */

/**
 * @name HeapManObj: heap manager object
 * Although C has no built-in OOP, an OOP object may be
 * defined in C as a structure containing object state
 * and pointers to member functions.
 * @{
 */
/**
 * The allocate-memory function for HeapManObj.
 * Allocates an area of memory of requested size.
 * To support heaps that compact memory, a memory handle
 * is returned rather than a memory address.
 *
 * @param state the <var>state</var> field of HeapManObj
 * @param n     memory size in bytes
 * @returns a memory handle, call addrFromHandle to use
 * @see _HeapManObj
 */
typedef void* (*AllocFunction)(void* state, int n);

/**
 * The free-memory function for HeapManObj
 *
 * @param state the <var>state</var> field of HeapManObj
 * @param handle a memory handle
 * @see _HeapManObj
 */
typedef void (*FreeFunction)(void* state, void* handle);

/**
 * Given a handle of a memory area, obtains the address
 * usable for normal memory operations like copying.
 *
 * This function is to support heaps that compact memory.
 * (Memory areas may be moved, but handles stored in data
 * structures need not be modified.)
 *
 * @param state value from the <var>state</var> field of HeapManObj
 * @param handle memory handle
 * @see _HeapManObj
*/
typedef void* (*AddrFromHandleFunction)(void* state, void* handle);

/** The structure that implements a heap manager object. */
typedef struct _HeapManObj {
    void* state;         /**< heap handle.
                           * This field may be used to distinguish
                           * different heap manager instances. */
    AllocFunction alloc; /**< allocate memory. @see AllocFunction */
    FreeFunction free;   /**< free memory. @see FreeFunction */
    AddrFromHandleFunction addrFromHandle;
                         /**< get memory address from memory handle.
                          * @see AddrFromHandleFunction */
} HeapManObj;
/** @} */

/** State needed to find entries in a Jar. */
typedef struct _JarInfo {
    int status; /**< error code, 0 for success */
    unsigned long locOffset; /**< Offset of first local entry */
    unsigned long cenOffset; /**< Offset of central directory */
} JarInfo;

/** State needed to get the name or uncompressed data of an entry. */
typedef struct _JarEntryInfo {
    int status; /**< error code, 0 for success */
    unsigned long decompLen; /**< decompressed length */
    unsigned long compLen; /**< compressed length */
    unsigned long method; /**< how it is stored */
    unsigned long expectedCRC; /**< expected CRC, mismatch means jar corrupt */
    int encrypted; /**< non-zero if encrypted */
    unsigned long offset; /**< offset of entry */
    unsigned long nameLen; /**< entry name length */
    unsigned long nameOffset; /**< offset of entry name */
    unsigned long nextCenEntryOffset; /**< offset for getNextJarEntryInfo */
} JarEntryInfo;

/********************************************************************/
/* Functions */
/********************************************************************/

/**
 * Fills a Jar info struct so that entries can be read using getJarEntryInfo.
 * Does not perform memory allocation.
 *
 * @param fileObj file object
 * 
 * @return Jar info status of 0 if success, non-zero otherwise.
 */
JarInfo getJarInfo(FileObj* fileObj);

/**
 * Reads an information for an named entry in a JAR. Does not perform
 * memory allocation.
 *
 * @param fileObj file object
 * @param jarInfo info returned by getJarInfo
 * @param name name of entry to find, can be a heap handle or memory address
 * @param nameLen length of the name
 * @param compBuffer buffer for reading in entry name for comparison,
 *                   must be nameLen long
 *
 * @return entry info with a status of zero for success or a non-zero
 * error code.
 */
JarEntryInfo findJarEntryInfo(FileObj* pFileObj, JarInfo* jarInfo,
    const unsigned char *name, unsigned int nameLen,
    unsigned char* compBuffer);

/**
 * Reads an information for the first entry in a JAR. Does not perform
 * memory allocation.
 *
 * @param fileObj file object
 * @param jarInfo structure returned from getJarInfo
 *
 * @return entry info with a status of zero for success or a non-zero
 * error code.
 */
JarEntryInfo getFirstJarEntryInfo(FileObj* pFileObj, JarInfo* pJarInfo);

/**
 * Reads an information for the entry after the given entry in a JAR.
 * Does not perform memory allocation.
 *
 * @param fileObj file object
 * @param jarInfo JAR info object
 * @param current current JAR entry info
 *
 * @return entry info with a status of zero for success or a non-zero
 * error code including JAR_ENTRY_NOT_FOUND at the end of the entries
 */
JarEntryInfo getNextJarEntryInfo(FileObj* fileObj, JarInfo* jarInfo,
                                 JarEntryInfo* current);
/**
 * Get the name of a JAR entry. No allocations happen in the function.
 * Does not perform memory allocation.
 *
 * parameters:  
 * @param fileObj file object
 * @param entryInfo entry info structure returned by getJarEntryInfo
 * @param nameBuffer buffer for the name must be at least entry.nameLen long
 *
 * @return 0 for success else an error status
 */
int getJarEntryName(FileObj* pFileObj, JarEntryInfo* pEntry,
    unsigned char *nameBuffer);

/**
 * Inflates a JAR entry.
 *
 * @param fileObj file object
 * @param heapManObj heap object, only used for temp tables
 * @param entryInfo entry info structure returned by getJarEntryInfo
 * @param decompBuffer buffer for uncompressed data must be at
 *                     least entry.decompLen long, can be heap handle or
 *                     memory address
 * @param bufferIsAHandle non-zero if decompBuffer is mem handle that must be
 *        given to heapManObj.addrFromHandle before using
 *
 * @return 0 for success else an error status
 */
int inflateJarEntry(FileObj* fileObj, HeapManObj* heapManObj,
    JarEntryInfo* entry, unsigned char *decompBuffer, int bufferIsAHandle);

/**
 * Inflates the data in a file.
 * <p>
 * NOTE:
 *    The caller of this method must insure that this function can safely
 *    up to INFLATER_EXTRA_BYTES beyond compData + compLen without causing
 *    any problems.
 *    The inflater algorithm occasionally reads one more byte than it needs
 *    to.  But it double checks that it doesn't actually care what's in that
 *    extra byte.</p>
 *
 * @param fileObj File object for reading the compressed data with the
 *                current file position set to the beginning of the data
 * @param heapManObj Heap object for temp data
 * @param method Compression method
 * @param compLen Length of the compressed data
 * @param decompBuffer where to store the uncompressed data
 * @param decompLen Expected length of the uncompressed data
 * @param bufferIsAHandle non-zero if decompBuffer is mem handle that must be
 *        given to heapManObj.addrFromHandle before using
 *
 * @return TRUE if the data was encoded in a supported <method> and the
 *                  size of the decoded data is exactly the same as <decompLen>
 *               FALSE if an error occurs
 */
int inflateData(FileObj* fileObj, HeapManObj* heapManObj, int compLen,
                unsigned char* decompBuffer, int decompLen,
                int bufferIsAHandle);

/**
 * @name Inflate errors.
 * @{
 */
#define INFLATE_LEVEL_ERROR (-1)
#define OUT_OF_MEMORY_ERROR                (INFLATE_LEVEL_ERROR - 1)
#define INFLATE_INVALID_BTYPE              (INFLATE_LEVEL_ERROR - 2)
#define INFLATE_INPUT_BIT_ERROR            (INFLATE_LEVEL_ERROR - 3)
#define INFLATE_OUTPUT_BIT_ERROR           (INFLATE_LEVEL_ERROR - 4)
#define INFLATE_BAD_LENGTH_FIELD           (INFLATE_LEVEL_ERROR - 5)
#define INFLATE_INPUT_OVERFLOW             (INFLATE_LEVEL_ERROR - 6)
#define INFLATE_OUTPUT_OVERFLOW            (INFLATE_LEVEL_ERROR - 7)
#define INFLATE_EARLY_END_OF_INPUT         (INFLATE_LEVEL_ERROR - 8)
#define INFLATE_HUFFMAN_ENTRY_ERROR        (INFLATE_LEVEL_ERROR - 9)
#define INFLATE_INVALID_LITERAL_OR_LENGTH  (INFLATE_LEVEL_ERROR - 10)
#define INFLATE_BAD_DISTANCE_CODE          (INFLATE_LEVEL_ERROR - 11)
#define INFLATE_COPY_UNDERFLOW             (INFLATE_LEVEL_ERROR - 12)
#define INFLATE_CODE_TABLE_LENGTH_ERROR    (INFLATE_LEVEL_ERROR - 13)
#define INFLATE_EARLY_END_OF_CCTABLE_INPUT (INFLATE_LEVEL_ERROR - 14)
#define INFLATE_BAD_REPEAT_CODE            (INFLATE_LEVEL_ERROR - 15)
#define INFLATE_BAD_CODELENGTH_CODE        (INFLATE_LEVEL_ERROR - 16)
#define INFLATE_CODE_TABLE_EMPTY           (INFLATE_LEVEL_ERROR - 17)
/** @} */

/**
 * @name JAR errors.
 * @{
 */
#define JAR_LEVEL_ERROR 100

#define JAR_CORRUPT                  (JAR_LEVEL_ERROR + 1)
#define JAR_ENTRY_NOT_FOUND          (JAR_LEVEL_ERROR + 2)
#define JAR_ENCRYPTION_NOT_SUPPORTED (JAR_LEVEL_ERROR + 3)
#define JAR_EARLY_END_OF_INPUT       (JAR_LEVEL_ERROR + 4)
#define JAR_ENTRY_SIZE_MISMATCH      (JAR_LEVEL_ERROR + 5)
#define JAR_UNKNOWN_COMP_METHOD      (JAR_LEVEL_ERROR + 6)
#define JAR_ENTRY_CORRUPT            (JAR_LEVEL_ERROR + 7)
/** @} */

#endif /* _JAR_H_ */
