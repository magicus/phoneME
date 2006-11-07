/*
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

/**
 * @file
 *
 * This header file is interface to the internal MIDlet suite storage
 * functions.
 */

#ifndef _SUITESTORE_INTERN_H_
#define _SUITESTORE_INTERN_H_

#include <kni.h>
#include <midpString.h>
#include <suitestore_export.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const pcsl_string SUITE_LIST_FILENAME;

#if ENABLE_MONET
extern const pcsl_string APP_IMAGE_FILENAME;
#endif

/**
 * This macro us used to determine size of the result buffer for passing it
 * to unicodeToEscapedAscii function. Namely it multiplies given length by 5.
 */
#define ESCAPED_BUFFER_SIZE(length) ((length) * 5)

/**
 * Convert a Unicode string into a form that can be safely stored on
 * an ANSI-compatible file system. All characters that are not
 * [A-Za-z0-9] are converted into %uuuu, where uuuu is the hex
 * representation of the character's unicode value. Note even
 * though "_" is allowed it is converted because we use it for
 * for internal purposes. Potential file separators are converted
 * so the storage layer does not have deal with sub-directory hierarchies.
 *
 * @param str_data buffer with a string that may contain any unicode character
 * @param str_len length of the string pointed to by str_data
 * @param pBuffer a buffer that is at least 5 times the size of str after
 *        the offset, must not overlap with str; it is recommended to use
 *        ESCAPED_BUFFER_SIZE macro to calculate sufficient size for the buffer
 * @param offset where to start putting the characters
 *
 * @return number of characters put in pBuffer
 */
int unicodeToEscapedAscii(const jchar* str_data, const int str_len, jchar* pBuffer, int offset);


/**
 * Convert a Unicode string into a form that can be safely stored on
 * an ANSI-compatible file system and append it to the string specified
 * as the first parameter. All characters that are not
 * [A-Za-z0-9] are converted into %uuuu, where uuuu is the hex
 * representation of the character's unicode value. Note even
 * though "_" is allowed it is converted because we use it
 * for internal purposes. Potential file separators are converted
 * so the storage layer does not have deal with sub-directory hierarchies.
 *
 * @param dst the string to which the converted text is appended
 * @param suffix text to be converted into escaped-ascii
 * @return error code
 */
pcsl_string_status
pcsl_string_append_escaped_ascii(pcsl_string* dst, const pcsl_string* suffix);

/**
 * Resets any persistent resources allocated by MIDlet suite storage functions.
 */
void resetMidletSuiteStorage();

/**
 * Gets the enable state of a MIDlet suite from persistent storage.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteID ID of the suite
 * @param pEnabled pointer to an enabled setting
 *
 * @return false if out of memory
 */
int readEnabledState(char** ppszError, const pcsl_string* suiteID, jboolean* pEnabled);

/**
 * Gets the settings of a MIDlet suite from persistent storage.
 * <pre>
 * The format of the properties file will be:
 *
 *   push interrupt setting as an jbyte
 *   length of a permissions as an int
 *   array of permissions jbytes
 *   push options as jint
 * </pre>
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteID ID of the suite
 * @param pEnabled pointer to an enabled setting
 * @param pPushInterrupt pointer to a push interruptSetting
 * @param pPushOptions user options for push interrupts
 * @param ppPermissions pointer a pointer to accept a permissions array
 * @param pNumberOfPermissions pointer to an int
 *
 * @return false if out of memory
 */
int readSettings(char** ppszError, const pcsl_string* suiteID, jboolean* pEnabled,
                 jbyte* pPushInterrupt, jint* pPushOptions,
                 jbyte** ppPermissions, int* pNumberOfPermissions);

/**
 * Writes the settings of a MIDlet suite to persistent storage.
 * <pre>
 * The format of the properties file will be:
 *
 *   push interrupt setting as an jbyte
 *   length of a permissions as an int
 *   array of permissions jbytes
 *   push options as jint
 * </pre>
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteID  ID of the suite
 * @param enabled enabled setting
 * @param pushInterrupt pointer to a push interruptSetting
 * @param pushOptions user options for push interrupts
 * @param pPermissions pointer a pointer to accept a permissions array
 * @param numberOfPermissions length of pPermissions
 *
 * @return false if out of memory
 */
int writeSettings(char** ppszError,  const pcsl_string* suiteID, jboolean enabled,
                  jbyte pushInterrupt, jint pushOptions, jbyte* pPermissions,
                  int numberOfPermissions);


/**
 * Read a the install information of a suite from persistent storage.
 * The caller should have make sure the suite ID is valid.
 * The caller should call midpFreeInstallInfo when done with the information.
 * <pre>
 * The fields are
 *   jadUrl
 *   jarUrl
 *   ca
 *   domain
 *   trusted
 *
 * Unicode strings are written as an int length and a jchar array.
 * </pre>
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteID ID of a suite
 *
 * @return an InstallInfo struct, use the status macros to check for
 * error conditions in the struct
 */
MidpInstallInfo read_install_info(char** ppszError, const pcsl_string* suiteID);

/**
 * Write the install information of a suite to persistent storage.
 * The caller should have make sure the suite ID is valid.
 * <pre>
 * The fields are
 *   jadUrl
 *   jarUrl
 *   ca
 *   domain
 *   trusted
 *
 * Unicode strings are written as an int and jshort array.
 * </pre>
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteID ID of a suite
 * @param an allocated InstallInfo struct
 *
 * @return false if there is not enough memory
 */
int write_install_info(char** ppszError, const pcsl_string* suiteID,
    MidpInstallInfo installInfo);

/**
 * Adds an ID to the list of installed suites.
 *
 * @param suiteID ID of a suite
 *
 * @return 0 if successful,
 *        > 0 if already in the list
 *        OUT_OF_MEM_LEN if out of memory,
 *        IO_ERROR_LEN if an IO error occurred
 */
int addToSuiteList(const pcsl_string* suiteID);

/**
 * Retrieves the number of strings in a file.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param filename name of the file
 *
 * @return number of installed suites
 */
int getNumberOfStrings(char** ppszError, const pcsl_string* filename);

/**
 * Tells if a suite exists.
 *
 * @param suiteID ID of a suite
 *
 * @return > 0 if a suite exists,
 *           0 if not,
 *           OUT_OF_MEM_LEN if out of memory or IO error.
 *           IO_ERROR_LEN if out of memory or IO error.
 */
int suite_exists(const pcsl_string* suiteID);

/**
 * Native method boolean removeFromSuiteList(String) for class
 * com.sun.midp.midletsuite.MIDletSuiteStorage.
 * <p>
 * Removes the suite from the list of installed suites.
 *
 * @param suiteID ID of a suite
 *
 * @return 1 if the suite was in the list, 0 if not
 * -1 if out of memory
 */
int removeFromSuiteList(const pcsl_string* suiteID);

/**
 * Retrieves the list of strings in a file.
 * The file has the number of strings at the front, each string
 * is a length and the jchars.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param filename name of the file of strings
 * @param paList pointer to an array of pcsl_strings, free with
 *               free_pcsl_string_list
 *
 * @return number of strings if successful (can be 0),
 *         OUT_OF_MEM_LEN if midpMalloc failed
 */
int getStringList(char** ppszError,
                  const pcsl_string* filename,
                  pcsl_string** paList);

/**
 * Adds a string to the list of strings in a file.
 *
 * @param newString string to add
 *
 * @return 0 for success,
 *         OUT_OF_MEM_LEN if out of memory,
 *         IO_ERROR_LEN if an IO error
 */
int addToStringList(const pcsl_string* filename, const pcsl_string* newString);

/**
 * Removes a string from a the list of strings in a file.
 *
 * @param filename name of the file
 * @param oldString string to remove
 *
 * @return true if the suite was in the list
 */
jboolean removeFromStringList(const pcsl_string* filename, const pcsl_string* oldString0);

/**
 * Gets location of the properties file
 * for the suite with the specified suiteId.
 *
 *
 * @param suiteId    - The application suite ID string
 * @param checkSuiteExists - true if suite should be checked for existence or
 *                            not
 * @param filename - The in/out parameter that contains returned filename
 * @return  error code that should be one of the following:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM,
 *       MIDP_ERROR_AMS_SUITE_NOT_FOUND
 * </pre>
 */
MIDP_ERROR midp_example_get_property_file(const pcsl_string* suiteId,
                                      jboolean checkSuiteExists,
                                      pcsl_string *filename);

/**
 * Gets location of the specified resource file
 * for the suite with the specified suiteId.
 *
 *
 * @param suiteId    - The application suite ID string
 * @param resourceName - The name of suite resource whose location is requested  
 * @param checkSuiteExists - true if suite should be checked for existence or not
 * @param filename - The in/out parameter that contains returned filename
 * @return  error code that should be one of the following:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_OF_MEM,
 *       MIDP_ERROR_AMS_SUITE_NOT_FOUND,MIDP_ERROR_AMS_SUITE_CORRUPTED,
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
MIDP_ERROR midp_example_get_suite_resource_file(const pcsl_string* suiteID,
                      const pcsl_string*  resourceName,
				      jboolean checkSuiteExists,
				      pcsl_string* filename);


#ifdef __cplusplus
}
#endif

#endif /* _SUITESTORE_INTERN_H_ */
