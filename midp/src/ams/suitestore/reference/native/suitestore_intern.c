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
 * @ingroup AMS
 *
 * This is reference implementation of the internal MIDlet suite storage
 * functions.
 * <p>
 * <h3>Common String List Files</h3>
 * <p>
 * A suite has more state than what is in the JAD or JAR, to hold this state
 * there a multiple files that contain lists of string values, with a string
 * key. The format of the string list file is a common Unicode-string file
 * format, for any file that is a series of Unicode strings. The format of
 * the file is binary and is written without byte conversion. These decisions
 * save code. Byte conversion can be omitted because the file is never copied
 * between devices.
 * <p>
 * The structure of the file is one native integer (int) for the number of
 * strings, followed by the data. The data consists of, for each Unicode
 * string, a Java programming language int for the number characters,
 * followed by the Java programming language characters themselves.
 * <p>
 * <h3>Suite Storage Files</h3>
 * <p>
 * When a suite is stored the files or record in a file listed below are
 * created.
 * <p>
 * <table border=2>
 * <tr>
 *   <th>File Type</th>
 *   <th>Description</th>
 * </tr>
 * <tr>
 *   <td>JAR</td>
 *   <td>The JAR file downloaded for the suite. This file is created by the
 *       installer and renamed when it is stored.</td>
 * </tr>
 * <tr>
 *   <td>Application Properties</td>
 *   <td>JAD properties file followed by the manifest properties of the MIDlet
 *       suite. The file is a Unicode string file, with each property written
 *       as two strings: the property key followed by the property value.</td>
 * </tr>
 * <tr>
 *   <td>Suite Settings</td>
 *   <td>API permissions and push interrupt setting of the MIDlet suite,
 *       written as an array of bytes</td>
 * </tr>
 * <tr>
 *   <td>Install Information</td>
 *   <td>The JAD file URL, JAR file URL, Software Certificate Authority (CA),
 *       Content Provider, Permission Domain, and a trusted boolean</td>
 * </tr>
 * <tr>
 *   <td>Suite ID</td>
 *   <td>List of MIDlet-suite identifiers. There is only one copy of this
 *       file, which is shared by all MIDlet suites. It is a common Unicode
 *       string file.</td>
 * </tr>
 * <tr>
 *   <td>Install Notification URLs</td>
 *   <td>One record is added to a list of URLs to which to post install
 *       notifications when the next OTA installation is performed or a MIDlet
 *       suite is run. There is only one copy of this file, which is shared by
 *       all MIDlet suites. It is a common Unicode string file.</td>
 * </tr>
 *   <td>Delete Notification URLs</td>
 *   <td>One record is added to a list of URLs to which to post delete
 *       notifications when the next OTA installation is performed. There is
 *       only one copy of this file, which is shared by all MIDlet suites. It
 *       is a common Unicode string file.</td>
 * </tr>
 * <tr>
 *   <td>Verification Hash<td>
 *   <td>If the preverification option is built, a file with the hash of
 *       the JAR is written to its own file.</td>
 * </tr>
 * <tr>
 *   <td>Cached Images<td>
 *   <td>If the image cache option is is built, each image in the JAR, will
 *       be extracted, decode into a platform binary image, and stored in the
 *       image cache. See imageCache.h.</td>
 * </tr>
 * </table>
 */
#include <string.h>

#include <kni.h>

#include <midpMalloc.h>
#include <midpServices.h>
#include <midpStorage.h>
#include <midpInit.h>
#include <midpRMS.h>
#include <suitestore_export.h>
#include <suitestore_locks.h>
#include <suitestore_verify_hash.h>
#include <suitestore_intern.h>
#include <suitestore_util.h>
#include <suitestore_otanotifier_db.h>
#include <push_server_export.h>
#include <imageCache.h>
#include <midpUtilKni.h>

#define NUM_SUITE_FILES 4

static int suiteInList(const pcsl_string* string);
static int checkForCorruptedSuite(const pcsl_string* suiteID);

static int store_jar(char** ppszError, const pcsl_string* suiteID,
                                       const pcsl_string* jarName);

static MIDPError storeInstallProperties(const pcsl_string* suiteID,
        MidpProperties jadProps, MidpProperties jarProps);

#ifdef ENABLE_JSR_211
int jsr211_removeSuiteHandlers(MidpString suiteID);
#endif  /* ENABLE_JSR_211 */

#ifdef ENABLE_JSR_75
#include <fcCleanup.h>
#endif  /* ENABLE_JSR_75 */


/**
 * Filename to suites settings.
 * ".ss"
 */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(SETTINGS_FILENAME)
    {'.', 's', 's', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(SETTINGS_FILENAME);


/**
 * Filename to save the suite installation information.
 * ".ii"
 */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(INSTALL_INFO_FILENAME)
    {'.', 'i', 'i', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(INSTALL_INFO_FILENAME);

#if ENABLE_MONET
/** Filename to save the application image file of suite. */

PCSL_DEFINE_ASCII_STRING_LITERAL_START(APP_IMAGE_FILENAME)
#ifdef PRODUCT
    {'.', 'b', 'u', 'n', '\0'}
#elif defined(AZZERT)
    {'_', 'g', '.', 'b', 'u', 'n', '\0'}
#else
    {'_', 'r', '.', 'b', 'u', 'n', '\0'}
#endif
PCSL_DEFINE_ASCII_STRING_LITERAL_END(APP_IMAGE_FILENAME);
#endif

/**
 * Filename of the list of installed suites.
 * "_suites.dat"
 */
PCSL_DEFINE_ASCII_STRING_LITERAL_START(SUITE_LIST_FILENAME)
    {'_', 's', 'u', 'i', 't', 'e', 's', '.', 'd', 'a', 't', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(SUITE_LIST_FILENAME);

/** MIDlet property for the install notify URL. */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(INSTALL_NOTIFY_PROP)
    {'M', 'I', 'D', 'l', 'e', 't', '-', 'I', 'n', 's',
    't', 'a', 'l', 'l', '-', 'N', 'o', 't', 'i', 'f', 'y', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(INSTALL_NOTIFY_PROP);

/** MIDlet property for the delete notify URL. */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(DELETE_NOTIFY_PROP)
    {'M', 'I', 'D', 'l', 'e', 't', '-', 'D', 'e', 'l',
     'e', 't', 'e', '-', 'N', 'o', 't', 'i', 'f', 'y', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(DELETE_NOTIFY_PROP);

/** MIDlet property for the suite name. */
PCSL_DEFINE_ASCII_STRING_LITERAL_START(SUITE_NAME_PROP)
    {'M', 'I', 'D', 'l', 'e', 't', '-', 'N', 'a', 'm', 'e', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(SUITE_NAME_PROP);

/** MIDlet property for the suite vendor. */
PCSL_DEFINE_ASCII_STRING_LITERAL_START(SUITE_VENDOR_PROP)
    {'M', 'I', 'D', 'l', 'e', 't', '-', 'V', 'e', 'n', 'd', 'o', 'r', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(SUITE_VENDOR_PROP);

/** MIDlet property for the suite version. */
PCSL_DEFINE_ASCII_STRING_LITERAL_START(SUITE_VERSION_PROP)
    {'M', 'I', 'D', 'l', 'e', 't', '-', 'V', 'e', 'r', 's', 'i', 'o', 'n', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(SUITE_VERSION_PROP);

/** MIDlet property for the suite description. */
PCSL_DEFINE_ASCII_STRING_LITERAL_START(SUITE_DESC_PROP)
    {'M', 'I', 'D', 'l', 'e', 't', '-', 
     'D', 'e', 's', 'r', 'i', 'p', 't', 'i', 'o', 'n', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(SUITE_DESC_PROP);

/** ID of rommized MIDlets. */
PCSL_DEFINE_ASCII_STRING_LITERAL_START(INTERNAL_SUITE_ID)
    {'i', 'n', 't', 'e', 'r', 'n', 'a', 'l', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(INTERNAL_SUITE_ID);

/**
 * Filename to save the suite application properties.
 * ".ap"
 */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(PROPS_FILENAME)
    {'.', 'a', 'p', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(PROPS_FILENAME);

/** Cache of the last suite the exists function found. */
static pcsl_string lastSuiteExistsID = PCSL_STRING_NULL_INITIALIZER;

/**
 * Tells if a suite exists.
 *
 * @param suiteID ID of a suite
 *
 * @return > 0 if a suite exists,
 *           0 if not,
 *           OUT_OF_MEM_LEN if out of memory or IO error.
 *           IO_ERROR_LEN if out of memory or IO error.
 *           SUITE_CORRUPTED_ERROR is suite is found in the list, but it's
 *           corrupted
 */
int
suite_exists(const pcsl_string* suiteID) {
    int status;
    if (pcsl_string_equals(suiteID, &lastSuiteExistsID)) {
        return 1;
    }

    pcsl_string_free(&lastSuiteExistsID);
    lastSuiteExistsID = PCSL_STRING_NULL;

    /* The internal romized suite will not be found in appdb. */
    if (pcsl_string_equals(suiteID, &INTERNAL_SUITE_ID)) {
        if (PCSL_STRING_OK != pcsl_string_dup(suiteID, &lastSuiteExistsID)) {
            lastSuiteExistsID = PCSL_STRING_NULL;
        }
        return 1;
    }

    status = suiteInList(suiteID);

    if (status > 0) {
        pcsl_string_dup(suiteID, &lastSuiteExistsID);
    }

    return status;
}

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
 *        the offset, must not be the memory as str
 * @param offset where to start putting the characters
 *
 * @return number of characters put in pBuffer
 */
int
unicodeToEscapedAscii(const jchar* str_data, const int str_len, jchar* pBuffer, int offset) {
    static jchar NUMS[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    int i;
    int j;

    for (i = 0, j = offset; i < str_len; i++) {
        jchar c = str_data[i];

        if ((c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9')) {
            pBuffer[j++] = c;
        } else if (c >= 'A' && c <= 'Z') {
            /* Some file systems do not treat capital letters differently. */
            pBuffer[j++] = '#';
            pBuffer[j++] = c;
        } else {
            pBuffer[j++] = '%';
            pBuffer[j++] = NUMS[(c >> 12) & 0x000f];
            pBuffer[j++] = NUMS[(c >>  8) & 0x000f];
            pBuffer[j++] = NUMS[(c >>  4) & 0x000f];
            pBuffer[j++] = NUMS[c & 0x000f];
        }
    }

    return j - offset;
}

/**
 * Convert a Unicode string into a form that can be safely stored on
 * an ANSI-compatible file system and append it to the string specified
 * as the first parameter. All characters that are not
 * [A-Za-z0-9] are converted into %uuuu, where uuuu is the hex
 * representation of the character's unicode value. Note even
 * though "_" is allowed it is converted because we use it for
 * for internal purposes. Potential file separators are converted
 * so the storage layer does not have deal with sub-directory hierarchies.
 *
 * @param dst the string to which the converted text is appendsd
 * @param suffix text to be converted into escaped-ascii
 * @return error code
 */
pcsl_string_status
pcsl_string_append_escaped_ascii(pcsl_string* dst, const pcsl_string* suffix) {
    pcsl_string_status rc = PCSL_STRING_ENOMEM;
    jchar* id_data = NULL;
    int len = -1;
    if(pcsl_string_length(suffix) <= 0) { /* nothing to do */
        return PCSL_STRING_OK;
    }
    GET_PCSL_STRING_DATA_AND_LENGTH(suffix)
    if (NULL != suffix_data) {
        int id_len = ESCAPED_BUFFER_SIZE(suffix_len);
        id_data = (jchar*)midpMalloc(id_len * sizeof (jchar));
        if (NULL != id_data) {
            len = unicodeToEscapedAscii(suffix_data, suffix_len, id_data, 0);
        }
    }
    RELEASE_PCSL_STRING_DATA_AND_LENGTH
    if (NULL != id_data) {
        rc = pcsl_string_append_buf(dst, id_data, len);
        midpFree(id_data);
    }
    return rc;
}

/**
 * Gets the storage root for a MIDlet suite by ID.
 * Free the data of the string returned with midpFree.
 *
 * @param id suite ID
 * @param sRoot receives storage root (gets set to NULL in the case of an error)
 *
 * @return status: 0 if success,
 * OUT_OF_MEM_LEN if out-of-memory
 */
static int
get_suite_storage_root(const pcsl_string* id, pcsl_string* sRoot) {
    const pcsl_string* root = storage_get_root();
    *sRoot = PCSL_STRING_EMPTY;
    pcsl_string_predict_size(sRoot, pcsl_string_length(root)+pcsl_string_length(id));
    if( PCSL_STRING_OK == pcsl_string_append(sRoot, root)
     && PCSL_STRING_OK == pcsl_string_append(sRoot, id)) {
        return 0;
    }
    pcsl_string_free(sRoot);
    *sRoot = PCSL_STRING_NULL;
    return OUT_OF_MEM_LEN;
}

/**
 * Resets any persistent resources allocated by MIDlet suite storage functions.
 */
void
resetMidletSuiteStorage() {
    pcsl_string_free(&lastSuiteExistsID);
    lastSuiteExistsID = PCSL_STRING_NULL;
    removeAllStorageLock();
}

/**
 * Builds a full file name using the storage root and MIDlet suite by ID.
 *
 * @param id suite ID
 * @param filename filename without a root path
 * @param sRoot receives full name of the file
 *
 * @return the status,
 * NULL_STR_LEN mean the suite does not exist,
 * OUT_OF_MEM_LEN if out of memory,
 * IO_ERROR if an IO_ERROR
 * 0 if ok
 */
static int
get_suite_filename(const pcsl_string* id, const pcsl_string* filename, pcsl_string* sRoot) {
  int sRoot_len;
  const pcsl_string* root = storage_get_root();

  *sRoot = PCSL_STRING_EMPTY;

  sRoot_len = pcsl_string_length(root)
            + pcsl_string_length(id)
            + pcsl_string_length(filename);
  pcsl_string_predict_size(sRoot, sRoot_len);

  if (PCSL_STRING_OK == pcsl_string_append(sRoot, root)
   && PCSL_STRING_OK == pcsl_string_append(sRoot, id)
   && PCSL_STRING_OK == pcsl_string_append(sRoot, filename)) {
        return 0;
  } else {
        pcsl_string_free(sRoot);
        *sRoot = PCSL_STRING_NULL;
        return OUT_OF_MEM_LEN;
  }
}

/**
 * Builds a full file name using the storage root and MIDlet suite by ID.
 * get_suite_filename is used to build a filename after validation checks.
 *
 * @param id suite ID
 * @param filename filename without a root path
 * @param res receives full name of the file
 *
 * @return the status,
 * NULL_LEN mean the suite does not exist,
 * OUT_OF_MEM_LEN if out of memory,
 * IO_ERROR_LEN if an IO_ERROR
 * 0 if ok
 */
static int
build_suite_filename(const pcsl_string* id, const pcsl_string* filename, pcsl_string* res) {
    int status;

    *res = PCSL_STRING_NULL;

    status = suite_exists(id);
    if (status == 0) {
        return NULL_LEN;
    }

    /* Ignore if suite is corrupted */
    if ((status < 0)&&(status != SUITE_CORRUPTED_ERROR)) {
        return status;
    }

    get_suite_filename(id, filename, res);

    return 0;
}

/**
 * Frees an InstallInfo struct. Does nothing if passed NULL.
 *
 * @param installInfo installation information returned from readInstallInfo.
 */
void
midpFreeInstallInfo(MidpInstallInfo installInfo) {
    pcsl_string_free(&installInfo.jadUrl_s);
    pcsl_string_free(&installInfo.jarUrl_s);
    pcsl_string_free(&installInfo.domain_s);

    free_pcsl_string_list(installInfo.authPath_as, installInfo.authPathLen);
    installInfo.authPath_as = NULL;
    installInfo.authPathLen = 0;
}

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
MidpInstallInfo
read_install_info(char** ppszError, const pcsl_string* suiteID) {
    MidpInstallInfo installInfo;
    pcsl_string filename;
    char* pszTemp;
    int handle;
    int no_errors = 0;
    int i;
    int rc;

    *ppszError = NULL;
    memset((unsigned char*)&installInfo, 0, sizeof (MidpInstallInfo));

    rc = build_suite_filename(suiteID, &INSTALL_INFO_FILENAME, &filename);

    if (rc < 0) {
        installInfo.status = rc;
        return installInfo;
    }

    handle = storage_open(ppszError, &filename, OPEN_READ);
    pcsl_string_free(&filename);

    if (*ppszError != NULL) {
        installInfo.status = IO_ERROR_LEN;
        return installInfo;
    }

    do {
        storage_read_utf16_string(ppszError, handle, &installInfo.jadUrl_s);
        storage_read_utf16_string(ppszError, handle, &installInfo.jarUrl_s);
        storage_read_utf16_string(ppszError, handle, &installInfo.domain_s);

        storageRead(ppszError, handle, (char*)(&installInfo.trusted),
            sizeof (installInfo.trusted));
        if (*ppszError != NULL) {
            break;
        }

        installInfo.authPathLen = 0;
        installInfo.authPath_as = NULL;
        storageRead(ppszError, handle, (char*)(&installInfo.authPathLen),
            sizeof (installInfo.authPathLen));
        if (*ppszError != NULL) {
            break;
        }

        installInfo.authPath_as = alloc_pcsl_string_list(installInfo.authPathLen);
        if (NULL == installInfo.authPath_as) {
            installInfo.authPathLen = 0;
            break;
        }

        for (i = 0; i < installInfo.authPathLen; i++) {
            storage_read_utf16_string(ppszError, handle, &installInfo.authPath_as[i]);
        }

        if (i != installInfo.authPathLen) {
            installInfo.authPathLen = i;
            break;
        }

        no_errors = 1;
    } while (0);

    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);

    if (no_errors) {
        return installInfo;
    }

    midpFreeInstallInfo(installInfo);

    memset((unsigned char*)&installInfo, 0, sizeof (MidpInstallInfo));

    if (*ppszError != NULL) {
        installInfo.status = IO_ERROR_LEN;
    } else {
        installInfo.status = OUT_OF_MEM_LEN;
    }

    return installInfo;
}

/**
 * Write the install information of a suite to persistent storage.
 * The caller should have make sure the suite ID is valid.
 * <pre>
 * The fields are
 *   jadUrl
 *   jarUrl
 *   domain
 *   trusted
 *   authPathLen
 *   authPath
 *
 * Unicode strings are written as an int and jshort array.
 * </pre>
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteID ID of a suite
 * @param an allocated InstallInfo struct
 *
 * @return false if there is not enough memory
 */
int
write_install_info(char** ppszError, const pcsl_string* suiteID,
                 MidpInstallInfo installInfo) {
    pcsl_string filename;
    char* pszTemp;
    int handle;
    int i;
    int rc;
    *ppszError = NULL;

    rc = build_suite_filename(suiteID, &INSTALL_INFO_FILENAME, &filename);
    if (0 != rc) {
        return 0;
    }

    handle = storage_open(ppszError, &filename, OPEN_READ_WRITE_TRUNCATE);
    pcsl_string_free(&filename);
    if (*ppszError != NULL) {
        return 1;
    }

    do {
        storage_write_utf16_string(ppszError, handle, &installInfo.jadUrl_s);
        storage_write_utf16_string(ppszError, handle, &installInfo.jarUrl_s);
        storage_write_utf16_string(ppszError, handle, &installInfo.domain_s);

        storageWrite(ppszError, handle, (char*)(&installInfo.trusted),
            sizeof (installInfo.trusted));
        if (*ppszError != NULL) {
            break;
        }

        storageWrite(ppszError, handle, (char*)(&installInfo.authPathLen),
            sizeof (installInfo.authPathLen));
        if (*ppszError != NULL) {
            break;
        }

        for (i = 0; i < installInfo.authPathLen; i++) {
            storage_write_utf16_string(ppszError, handle, &installInfo.authPath_as[i]);
        }
    } while (0);

    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);
    return 1;
}

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
int
getStringList(char** ppszError, const pcsl_string* filename, pcsl_string** paList) {
    char* pszTemp;
    int numberOfStrings = 0;
    pcsl_string* pStrings = NULL;
    int i = 0;
    int handle;
    int no_errors = 0;
    int out_of_mem = 0;

    *ppszError = NULL;
    *paList = NULL;

    handle = storage_open(ppszError, filename, OPEN_READ);
    if (*ppszError != NULL) {
        return 0;
    }

    storageRead(ppszError, handle, (char*)&numberOfStrings,
        sizeof (numberOfStrings));
    do {
        if (*ppszError != NULL || numberOfStrings == 0) {
            break;
        }

        pStrings = alloc_pcsl_string_list(numberOfStrings);
        if (pStrings == NULL) {
            out_of_mem = 1;
            break;
        }

        for (i = 0; i < numberOfStrings; i++) {
            pStrings[i] = PCSL_STRING_NULL;
        }

        for (i = 0; i < numberOfStrings; i++) {
            storage_read_utf16_string(ppszError, handle, &pStrings[i]);
            if (*ppszError != NULL) {
                break;
            }

        }

        if (i != numberOfStrings) {
            break;
        }

        no_errors = 1;
    } while (0);

    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);

    if (no_errors) {
        *paList = pStrings;
        return numberOfStrings;
    }

    free_pcsl_string_list(pStrings, i);

    if (out_of_mem) {
        return OUT_OF_MEM_LEN;
    }

    return 0;
}

/**
 * Frees a list of suite IDs.
 *
 * @param pStrings point to an array of suites
 * @param numberOfSuites number of elements in pSuites
 */
void
midpFreeSuiteIDs(pcsl_string* pSuites, int numberOfSuites) {
    free_pcsl_string_list(pSuites, numberOfSuites);
}

/**
 * Get the list installed of MIDlet suite IDs.
 *
 * @param suites empty array of strings to fill with suite IDs
 *
 * @returns number of suites (can be 0),
 *          OUT_OF_MEM_LEN if for out of memory,
 *          IO_ERROR_LEN if an IO error
 */
int
midpGetSuiteIDs(pcsl_string** ppSuites) {
    pcsl_string filename;
    pcsl_string_status rc = PCSL_STRING_ENOMEM;
    int numberOfSuites;
    char* pszError;

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        return OUT_OF_MEM_LEN;
    }

    *ppSuites = NULL;

    rc = pcsl_string_cat(storage_get_root(), &SUITE_LIST_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return OUT_OF_MEM_LEN;
    }

    numberOfSuites = getStringList(&pszError, &filename, ppSuites);
    pcsl_string_free(&filename);
    if (pszError != NULL) {
        storageFreeError(pszError);
        /* IMPL_NOTE: here we use filename after it's freed. It's bad. */
        if (storage_file_exists(&filename)) {
            return IO_ERROR_LEN;
        }

        /* Its OK for the file not to exist. */
        return 0;
    }

    return numberOfSuites;
}

/**
 * Retrieves the number of strings in a file.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param filename name of the file
 *
 * @return number of installed suites
 */
int
getNumberOfStrings(char** ppszError, const pcsl_string* filename) {
    char* pszTemp;
    int numberOfStrings;
    int handle;

    handle = storage_open(ppszError, filename, OPEN_READ);
    if (*ppszError != NULL) {
        return 0;
    }

    if (storageRead(ppszError, handle, (char*)&numberOfStrings,
            sizeof (numberOfStrings)) != sizeof (numberOfStrings)) {
        numberOfStrings = 0;
    }

    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);
    return numberOfStrings;
}

/**
 * Tells if a given string is in a list of strings in a file.
 *
 * @param filename name of a file
 * @param string string to look for
 *
 * @return > 0 if a string is in the list,
 *           0 if not,
 *         < OUT_OF_MEM_LEN if out of memory or IO error.
 *         < IO_ERROR_LEN if out of memory or IO error.
 *         SUITE_CORRUPTED_ERROR is suite is found in the list, but it's
 *         corrupted
 */
static int
suiteInList(const pcsl_string* string) {
    pcsl_string filename;
    pcsl_string_status rc = PCSL_STRING_ENOMEM;
    char* pszError;
    int numberOfStrings;
    pcsl_string* pStrings;
    int i;
    int status = 1;

    rc = pcsl_string_cat(storage_get_root(), &SUITE_LIST_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return OUT_OF_MEM_LEN;
    }

    numberOfStrings = getStringList(&pszError, &filename, &pStrings);
    if (pszError != NULL) {
        storageFreeError(pszError);

        if (storage_file_exists(&filename)) {
            pcsl_string_free(&filename);
            return IO_ERROR_LEN;
        }

        pcsl_string_free(&filename);
        /* It OK for the file not to exist. */
        return 0;
    }

    pcsl_string_free(&filename);

    if (numberOfStrings == OUT_OF_MEM_LEN) {
        return OUT_OF_MEM_LEN;
    }

    for (i = 0; i < numberOfStrings; i++) {
        if (pcsl_string_equals(string, &pStrings[i])) {
            free_pcsl_string_list(pStrings, numberOfStrings);
            /*
             * Make sure that suite is not corrupted. Return
             * SUITE_CORRUPTED_ERROR if the suite is corrupted. Remove
             * the suite before returning the status
             */
            status = checkForCorruptedSuite(string);
            if ((status == SUITE_CORRUPTED_ERROR)||
                (status == OUT_OF_MEM_LEN)) {
                return status;
            }
            return 1;
        }
    }

    free_pcsl_string_list(pStrings, numberOfStrings);
    return 0;
}

/**
 * Check if the suite is corrupted
 * @param suiteID ID of a suite
 *
 * @return SUITE_CORRUPTED_ERROR is suite is corrupted
 *         OUT_OF_MEM_LEN if out of memory or IO error.
 *         1 otherwise
 */
static int
checkForCorruptedSuite(const pcsl_string* suiteID) {
    pcsl_string filename[NUM_SUITE_FILES];
    int arc[NUM_SUITE_FILES];
    int i;
    int handle;
    char* pszError;
    char* pszTemp;
    int err = 1; /* Default to no error */

    arc[0] = get_suite_filename(suiteID, &INSTALL_INFO_FILENAME, &filename[0]);
    arc[1] = get_suite_filename(suiteID, &SETTINGS_FILENAME, &filename[1]);

    arc[2] = midp_suite_get_class_path(suiteID, KNI_FALSE, &filename[2]);
    if ( arc[2] == MIDP_ERROR_OUT_MEM) {
      err = OUT_OF_MEM_LEN;
    }

    if ((arc[3]=midp_example_get_property_file(suiteID, KNI_FALSE, &filename[3])) ==
        MIDP_ERROR_OUT_MEM) {
      err = OUT_OF_MEM_LEN;
    }

    for (i = 0; i < NUM_SUITE_FILES; i++) {

        if (0 != arc[i]) {
            err = OUT_OF_MEM_LEN;
                break;
        }
        handle = storage_open(&pszError, &filename[i], OPEN_READ);
        if (pszError != NULL) {
            /* File does not exist; suite must be corrupted */
            err = SUITE_CORRUPTED_ERROR;
            storageFreeError(pszError);
            break;
        }

        storageClose(&pszTemp, handle);
        storageFreeError(pszTemp);
    }

    pcsl_string_free(&filename[0]);
    pcsl_string_free(&filename[1]);
    pcsl_string_free(&filename[2]);
    pcsl_string_free(&filename[3]);

    return err;
}

/**
 * Save the given string list and new string to disk.
 * Strings that are null or empty will not be saved.
 * The file has the number of strings at the front, each string
 * is a length and the jchars.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param filename file to save to
 * @param pStrings array as one returned by getStringList()
 * @param numberOfStrings number of elements in pStrings
 * @param newString new string to add; if null or empty, it will not be added
 */
static void
saveStringList(char** ppszError, const pcsl_string* filename, pcsl_string* pStrings,
               int numberOfStrings, const pcsl_string* newString) {
    char* pszTemp;
    int numberOfStringsLeft;
    int i;
    int handle;

    *ppszError = NULL;

    handle = storage_open(ppszError, filename, OPEN_READ_WRITE_TRUNCATE);
    if (*ppszError != NULL) {
        return;
    }

    numberOfStringsLeft = numberOfStrings;
    for (i = 0; i < numberOfStrings; i++) {
        if (pcsl_string_length(&pStrings[i]) <= 0) {
            numberOfStringsLeft--;
        }
    }

    if (pcsl_string_length(newString) != 0) {
        numberOfStringsLeft++;
    }

    storageWrite(ppszError, handle, (char*)&numberOfStringsLeft,
        sizeof (numberOfStringsLeft));
    do {
        if (*ppszError != NULL || numberOfStringsLeft == 0) {
            break;
        }

        for (i = 0; i < numberOfStrings; i++) {
            if (pcsl_string_length(&pStrings[i]) > 0) {
                storage_write_utf16_string(ppszError, handle, &pStrings[i]);
                if (*ppszError != NULL) {
                    break;
                }
            }
        }

        if (i != numberOfStrings) {
            break;
        }

        if (pcsl_string_length(newString) > 0) {
            storage_write_utf16_string(ppszError, handle,newString);
            if (*ppszError != NULL) {
                break;
            }

        }
    } while (0);

    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);
}

/**
 * Adds a string to the list of strings in a file.
 *
 * @param newString string to add
 *
 * @return 0 for success,
 *         OUT_OF_MEM_LEN if out of memory,
 *         IO_ERROR_LEN if an IO error
 */
int
addToStringList(const pcsl_string* filename, const pcsl_string* newString) {
    char* pszError;
    int numberOfStrings = 0;
    pcsl_string* pStrings = NULL;

    numberOfStrings = getStringList(&pszError, filename, &pStrings);
    if (pszError != NULL) {
        storageFreeError(pszError);

        if (storage_file_exists(filename)) {
            return IO_ERROR_LEN;
        }
    }

    if (numberOfStrings == OUT_OF_MEM_LEN) {
        return OUT_OF_MEM_LEN;
    }

    saveStringList(&pszError, filename, pStrings, numberOfStrings,
                   newString);
    free_pcsl_string_list(pStrings, numberOfStrings);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return IO_ERROR_LEN;
    }

    return 0;
}

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
int
addToSuiteList(const pcsl_string* suiteID) {
    pcsl_string filename;
    int status;

    status = suiteInList(suiteID);
    if (status == 0) {
        pcsl_string_status rc = PCSL_STRING_ENOMEM;
        rc = pcsl_string_cat(storage_get_root(),
                             &SUITE_LIST_FILENAME,
                             &filename);
        if (PCSL_STRING_OK != rc) {
            return OUT_OF_MEM_LEN;
        }
        /* only add if suite does not exist */
        status = addToStringList(&filename, suiteID);
        pcsl_string_free(&filename);
    }

    return status;
}

/**
 * Removes a string from a the list of strings in a file.
 *
 * @param filename name of the file
 * @param oldString string to remove
 *
 * @return true if the string was in the list
 */
jboolean
removeFromStringList(const pcsl_string* filename, const pcsl_string* oldString) {
    char* pszError;
    int numberOfStrings;
    pcsl_string* pStrings;
    int i;

    numberOfStrings = getStringList(&pszError, filename, &pStrings);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return KNI_FALSE;
    }

    if (pStrings == NULL) {
        return KNI_FALSE;
    }

    for (i = 0; i < numberOfStrings; i++) {
        /* we remove only one occurence... it works for us so far */
        if (pcsl_string_equals(oldString, &pStrings[i])) {
            /* Tell saveStringList do not save this string. */
            pcsl_string_free(&pStrings[i]);
            pStrings[i] = PCSL_STRING_EMPTY;

            /* Tell saveStringList do not add an ID. */
            saveStringList(&pszError, filename, pStrings, numberOfStrings,
                           &PCSL_STRING_EMPTY);
            free_pcsl_string_list(pStrings, numberOfStrings);
            return KNI_TRUE;
        }
    }

    free_pcsl_string_list(pStrings, numberOfStrings);
    return KNI_FALSE;
}

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
int
removeFromSuiteList(const pcsl_string* suiteID) {
    pcsl_string filename;
    int existed = 0;
    pcsl_string_status rc = PCSL_STRING_ENOMEM;

    rc = pcsl_string_cat(storage_get_root(), &SUITE_LIST_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return -1;
    }

    existed = removeFromStringList(&filename, suiteID);
    pcsl_string_free(&filename);

    /* Reset the static variable for MVM-safety */
    pcsl_string_free(&lastSuiteExistsID);

    return existed;
}

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
 * @param suiteID  ID of the suite
 * @param pEnabledState
 *
 * @return 0 (false) if out of memory or
 *         non-zero (true) if not out of memory
 *           (*ppszError will be NULL for success)
 */
int readEnabledState(char** ppszError, const pcsl_string* suiteID,
                     jboolean* pEnabled) {
    pcsl_string filename;
    int handle;
    char* pszTemp;
    int bytesRead;
    int rc;

    *ppszError = NULL;
    
    rc = build_suite_filename(suiteID, &SETTINGS_FILENAME, &filename);
    if (0 != rc) {
        return 0;
    }

    handle = storage_open(ppszError, &filename, OPEN_READ);
    pcsl_string_free(&filename);
    if (*ppszError != NULL) {
        return 1;
    }

    bytesRead = storageRead(ppszError, handle, (char*)pEnabled,
        sizeof (jboolean));

    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);
    return 1;
}

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
 * @param suiteID  ID of the suite
 * @param pEnabled pointer to an enabled setting
 * @param pPushInterrupt pointer to a push interruptSetting
 * @param pPushOptions user options for push interrupts
 * @param ppPermissions pointer a pointer to accept a permissions array
 * @param pNumberOfPermissions pointer to an int
 *
 * @return 0 (false) if out of memory or
 *         non-zero (true) if not out of memory
 *           (*ppszError will be NULL for success)
 */
int readSettings(char** ppszError, const pcsl_string* suiteID, jboolean* pEnabled,
                 jbyte* pPushInterrupt, jint* pPushOptions,
                 jbyte** ppPermissions, int* pNumberOfPermissions) {
    pcsl_string filename;
    int handle;
    int bytesRead;
    char* pszTemp;
    int result = 1;
    int rc;

    *ppszError = NULL;
    *ppPermissions = NULL;
    *pNumberOfPermissions = 0;

    rc = build_suite_filename(suiteID, &SETTINGS_FILENAME, &filename);
    if (0 != rc) {
        return 0;
    }

    handle = storage_open(ppszError, &filename, OPEN_READ);
    pcsl_string_free(&filename);
    if (*ppszError != NULL) {
        return 1;
    }

    bytesRead = storageRead(ppszError, handle, (char*)pEnabled,
        sizeof (jboolean));
    do {
        if (*ppszError != NULL) {
            break;
        }

        bytesRead = storageRead(ppszError, handle, (char*)pPushInterrupt,
                                sizeof (jbyte));
        if (*ppszError != NULL) {
            break;
        }

        bytesRead = storageRead(ppszError, handle, (char*)pNumberOfPermissions,
                                sizeof (int));

        if (bytesRead != sizeof (int) || *pNumberOfPermissions == 0) {
            break;
        }

        *ppPermissions = (jbyte*)midpMalloc(*pNumberOfPermissions *
                                           sizeof (jbyte));
        if (*ppPermissions == NULL) {
            result = 0;
            break;
        }

        bytesRead = storageRead(ppszError, handle, (char*)(*ppPermissions),
                                *pNumberOfPermissions * sizeof (jbyte));

        if (bytesRead != *pNumberOfPermissions) {
            *pNumberOfPermissions = 0;
            midpFree(*ppPermissions);
            break;
        }

        /* Old versions of the file may not have options. */
        *pPushOptions = 0;
        bytesRead = storageRead(ppszError, handle, (char*)pPushOptions,
                                sizeof (jint));
        if (*ppszError != NULL) {
            storageFreeError(*ppszError);
            *ppszError = NULL;
            break;
        }
    } while (0);

    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);
    return result;
}

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
 * @return 0 (false) if out of memory or
 *         non-zero true if not out of memory
               (*ppszError will be NULL for success)
 */
int writeSettings(char** ppszError, const pcsl_string* suiteID, jboolean enabled,
                  jbyte pushInterrupt, jint pushOptions,
                  jbyte* pPermissions, int numberOfPermissions) {
    pcsl_string filename;
    int handle;
    char* pszTemp;
    int rc;

    *ppszError = NULL;

    rc = build_suite_filename(suiteID, &SETTINGS_FILENAME, &filename);
    if (0 != rc) {
        return 0;
    }

    handle = storage_open(ppszError, &filename, OPEN_READ_WRITE_TRUNCATE);
    pcsl_string_free(&filename);
    if (*ppszError != NULL) {
        return 1;
    }

    storageWrite(ppszError, handle, (char*)&enabled, sizeof (jboolean));
    do {
        if (*ppszError != NULL) {
            break;
        }

        storageWrite(ppszError, handle, (char*)&pushInterrupt, sizeof (jbyte));
        if (*ppszError != NULL) {
            break;
        }

        storageWrite(ppszError, handle, (char*)&numberOfPermissions,
                                sizeof (int));
        if (*ppszError != NULL) {
            break;
        }

        storageWrite(ppszError, handle, (char*)pPermissions,
                                numberOfPermissions * sizeof (jbyte));

        storageWrite(ppszError, handle, (char*)&pushOptions, sizeof (jint));
        if (*ppszError != NULL) {
            break;
        }
    } while (0);

    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);
    return 1;
}

/**
 * Gets the amount of storage on the device that this suite is using.
 * This includes the JAD, JAR, management data, and RMS.
 *
 * @param suiteID  ID of the suite
 *
 * @return number of bytes of storage the suite is using or less than
 * 0 if out of memory
 */
long
midp_get_suite_storage_size(const pcsl_string* suiteID) {
    long used = 0;
    long rms = 0;
    pcsl_string filename[NUM_SUITE_FILES];
    int i;
    int handle;
    char* pszError;
    char* pszTemp;

    for (i = 0; i < NUM_SUITE_FILES; i++) {
        filename[i] = PCSL_STRING_NULL;
    }
    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        return OUT_OF_MEM_LEN;
    }

    build_suite_filename(suiteID, &INSTALL_INFO_FILENAME, &filename[0]);
    build_suite_filename(suiteID, &SETTINGS_FILENAME, &filename[1]);
    midp_suite_get_class_path(suiteID, KNI_TRUE, &filename[2]);
    midp_example_get_property_file(suiteID, KNI_TRUE, &filename[3]);
    for (i = 0; i < NUM_SUITE_FILES; i++) {
        if (pcsl_string_is_null(&filename[i])) {
            continue;
        }

        handle = storage_open(&pszError, &filename[i], OPEN_READ);
        pcsl_string_free(&filename[i]);
        if (pszError != NULL) {
            storageFreeError(pszError);
            continue;
        }

        used += storageSizeOf(&pszError, handle);
        storageFreeError(pszError);

        storageClose(&pszTemp, handle);
        storageFreeError(pszTemp);
    }

    rms = rmsdb_get_rms_storage_size(suiteID);
    if (rms == OUT_OF_MEM_LEN) {
        return OUT_OF_MEM_LEN;
    }

    return used + rms;
}
/**
 * Stores the JAR of a MIDlet suite to persistent storage.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteID  ID of the suite
 * @param jarName filename of the temporary Jar
 *
 * @returns 0 if out of memory (no need to call storageFreeError)
 *          1 if successful or i/o error
 */
static int store_jar(char** ppszError, const pcsl_string* suiteID,
                    const pcsl_string* jarName) {
    pcsl_string filename = PCSL_STRING_NULL;
    jint error = midp_suite_get_class_path(suiteID, KNI_TRUE, &filename);
    if (error != MIDP_ERROR_NONE) {
        return 0;
    }

    storage_rename_file(ppszError, jarName, &filename);

    pcsl_string_free(&filename);
    return 1;
}

/*
 * Change the enabled state of a suite.
 *
 * @param suiteID ID of the suite
 * @param enabled true if the suite is be enabled
 *
 *
 */
static int changeEnabledState(const pcsl_string* suiteID, jboolean enabled) {
    char* pszError;
    int status;
    jbyte* pPermissions;
    int numberOfPermissions;
    jbyte pushInterrupt;
    jint pushOptions;
    jboolean temp;
    lockStorageList* node;

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        return 0;
    }
    status = suite_exists(suiteID);
     
    if ((status <= 0)&&(status != SUITE_CORRUPTED_ERROR)) {
        return MIDP_ERROR_AMS_SUITE_NOT_FOUND;
    }

    node = find_storage_lock(suiteID);
    if (node != NULL) {
        if (node->update == KNI_TRUE) {
            /* Suite is being updated currently. */
            return SUITE_LOCKED;
        }
    }

    if (!readSettings(&pszError, suiteID, &temp, &pushInterrupt,
           &pushOptions, &pPermissions, &numberOfPermissions)) {
        return MIDP_ERROR_OUT_MEM;
    }

    if (pszError != NULL) {
        return IO_ERROR_LEN;
    }

    if (!writeSettings(&pszError, suiteID, enabled, pushInterrupt, pushOptions,
                       pPermissions, numberOfPermissions)) {
        midpFree(pPermissions);
        return MIDP_ERROR_OUT_MEM;
    }

    midpFree(pPermissions);

    if (pszError != NULL) {
        return IO_ERROR_LEN;
    }

    return MIDP_ERROR_NONE;
}

/**
 * Disables a suite given its suite ID.
 * <p>
 * The method does not stop the suite if is in use. However any future
 * attepts to run a MIDlet from this suite while disabled should fail.
 *
 * @param suiteID  ID of the suite
 *
 * @return 0 if the suite does not exist SUITE_LOCKED if the 
 * suite is locked
 */
int
midpDisableSuite(const pcsl_string* suiteID) {
    return changeEnabledState(suiteID, KNI_FALSE);
}

/**
 * Enables a suite given its suite ID.
 * <p>
 * The method does update an suites that are currently loaded for
 * settings or of application management purposes.
 *
 * @param suiteID  ID of the suite
 *
 * @return 0 if the suite does not exist SUITE_LOCKED if the 
 * suite is locked
 */
int
midpEnableSuite(const pcsl_string* suiteID) {
    return changeEnabledState(suiteID, KNI_TRUE);
}

/**
 * Removes a software package given its suite ID
 * <p>
 * If the component is in use it must continue to be available
 * to the other components that are using it.  The resources it
 * consumes must not be released until it is not in use.
 *
 * @param suiteID  ID of the suite
 *
 * @return 0 if the suite does not exist
 * SUITE_LOCKED if the suite is locked
 * 1 on success
 */
int
midp_remove_suite(const pcsl_string* suiteID) {
    pcsl_string filename;
    char* pszError;
    pcsl_string suiteRoot;
    int status;
    void* fileIteratorHandle;
    MidpProperties properties;
    pcsl_string* pproperty;

    lockStorageList *node = NULL;
    node = find_storage_lock(suiteID);
    if (node != NULL) {
        if (node->update != KNI_TRUE) {
            return SUITE_LOCKED;
        }
    }

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        return 0;
    }

    status = suite_exists(suiteID);

    if ((status <= 0)&&(status != SUITE_CORRUPTED_ERROR)) {
        return 0;
    }

    /*
     * Remove the files
     * Call the native RMS method to remove the RMS data.
     * This function call is needed for portability
     */
     {
        int rc;
        rc = rmsdb_remove_record_stores_for_suite(suiteID);
        if (rc == KNI_FALSE) {
            return SUITE_LOCKED;
        }
    }
    //if (midpRemoveRecordStoresForSuite(suiteID) == KNI_FALSE) {
    //    return SUITE_LOCKED;
    //}

    pushdeletesuite(suiteID);

    /*
     * If there is a delete notify property, add the value to the delete
     * notify URL list.
     */
    properties = midp_get_suite_properties(suiteID);
    if (properties.numberOfProperties > 0) {
        pproperty = midp_find_property(properties, &DELETE_NOTIFY_PROP);
        if (pcsl_string_length(pproperty) > 0) {
            midpAddDeleteNotification(suiteID, pproperty);
        }

        pproperty = midp_find_property(properties, &INSTALL_NOTIFY_PROP);
        if (pcsl_string_length(pproperty) > 0) {
            /*
             * Remove any pending install notifications since they are only
             * retried when the suite is run.
             */
        midpRemoveInstallNotification(suiteID);
        }

        midpFreeProperties(properties);
    }


    if( 0 != get_suite_storage_root(suiteID, &suiteRoot))
    {
        return 0;
    }

    fileIteratorHandle = storage_open_file_iterator(&suiteRoot);
    if (!fileIteratorHandle) {
        return 0;
    }

    for (;;) {
        int rc = storage_get_next_file_in_iterator(&suiteRoot, fileIteratorHandle,&filename);
        if (0 != rc) {
            break;
        }
        storage_delete_file(&pszError, &filename);
        pcsl_string_free(&filename);
        if (pszError != NULL) {
            storageFreeError(pszError);
            break;
        }
    }

    pcsl_string_free(&suiteRoot);
    storageCloseFileIterator(fileIteratorHandle);
    removeFromSuiteList(suiteID);
    remove_storage_lock(suiteID);

#ifdef ENABLE_JSR_211
    GET_MIDP_STRING_FOR_PCSL_STRING_PTR(suiteID)
    jsr211_removeSuiteHandlers(midp_suiteID);
    RELEASE_MIDP_STRING_FOR_PCSL_STRING
#endif  /* ENABLE_JSR_211 */

#ifdef ENABLE_JSR_75
    jsr75_suite_remove_cleanup(suiteID);
#endif  /* ENABLE_JSR_75 */
    return 1;
}

/**
 * Return the number of properties.
 *
 * @param properties property list
 *
 * @return number of properties or OUT_MEM_STR_LEN if there was not enough
 * memory to build the list
 */
int
midpGetNumberOfProperties(MidpProperties properties) {
    return properties.numberOfProperties;
}

/**
 * Find and return the property the matches the given key.
 * The returned value need not be freed because it resides
 * in an internal data structure.
 *
 * @param properties property list
 * @param key key of property to find
 *
 * @return a pointer to the property value,
 *        or to PCSL_STRING_NULL if not found.
 */
pcsl_string*
midp_find_property(MidpProperties properties, const pcsl_string* key) {
    int i;

    /* Properties are stored as key, value pairs. */
    for (i = 0; i < properties.numberOfProperties; i++) {
        if (pcsl_string_equals(&properties.pStringArr[i * 2], key)) {
            return &properties.pStringArr[(i * 2) + 1];
        }
    }

    return (pcsl_string*)&PCSL_STRING_NULL;
}

/**
 * Free a list of properties.
 *
 * @param properties property list
 */
void
midpFreeProperties(MidpProperties properties) {
    /* Properties are stored as key, value pairs. */
    if (properties.numberOfProperties <= 0) {
        return;
    }
    free_pcsl_string_list(properties.pStringArr, properties.numberOfProperties * 2);
    properties.pStringArr = NULL;
}

/**
 * Gets the properties of a MIDlet suite to persistent storage.
 * <pre>
 * The format of the properties file will be:
 * <number of strings as int (2 strings per property)>
 *    {repeated for each property}
 *    <length of a property key as int>
 *    <property key as jchars>
 *    <length of property value as int>
 *    <property value as jchars>
 * </pre>
 * @param suiteID  ID of the suite
 *
 * @return properties in a pair pattern of key and value,
 * use the status macros to check the result. A SUITE_CORRUPTED_ERROR
 * is returned as a status of MidpProperties when suite is corrupted
 */
MidpProperties
midp_get_suite_properties(const pcsl_string* suiteID) {
    pcsl_string filename;
    MidpProperties result = { 0,ALL_OK,NULL };
    int len;
    char* pszError;

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        result.numberOfProperties = 0;
        result.status =  OUT_OF_MEMORY;
        return result;
    }


    if (checkForCorruptedSuite(suiteID) == SUITE_CORRUPTED_ERROR) {
        result.numberOfProperties = 0;
        result.status = SUITE_CORRUPTED_ERROR;
        return result;
    }

    if (midp_example_get_property_file(suiteID, KNI_TRUE, &filename) !=
        MIDP_ERROR_NONE) {
        result.numberOfProperties = 0;
        result.status = NOT_FOUND;
        return result;
    }

    len = getStringList(&pszError, &filename, &result.pStringArr);
    pcsl_string_free(&filename);
    if (pszError != NULL) {
        result.numberOfProperties = 0;
        result.status = IO_ERROR;
        storageFreeError(pszError);
        return result;
    }

    if (len < 0) {
        /* error */
        result.numberOfProperties = 0;
        result.status = GENERAL_ERROR;
    } else {
        /* each property is 2 strings (key and value) */
        result.numberOfProperties = len / 2;
    }

    return result;
}

/**
 * Gets the install information of a MIDlet suite.
 *
 * @param suiteID  ID of the suite
 *
 * @return Installation information, use status macros to check the result
 * A SUITE_CORRUPTED_ERROR is returned as a status in InstallInfo
 * when suite is corrupted
 */
MidpInstallInfo
midp_get_suite_install_info(const pcsl_string* suiteID) {
    MidpInstallInfo installInfo;
    char* pszError;

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        installInfo.status = OUT_OF_MEM_LEN;
        return installInfo;
    }

    if (checkForCorruptedSuite(suiteID) == SUITE_CORRUPTED_ERROR) {
        installInfo.status = SUITE_CORRUPTED_ERROR;
        return installInfo;
    }

    installInfo = read_install_info(&pszError, suiteID);
    if (pszError != NULL) {
        storageFreeError(pszError);
    }

#if VERIFY_ONCE
    /* Read verify hash info */
    installInfo.status = readVerifyHash(suiteID,
        &installInfo.pVerifyHash, &installInfo.verifyHashLen);
#endif /* VERIFY_ONCE */

    return installInfo;
}



/**
 * Returns a unique identifier of MIDlet suite.
 * Constructed from the combination
 * of the values of the <code>MIDlet-Name</code> and
 * <code>MIDlet-Vendor</code> attributes.
 *
 * @param vendor name of the vendor that created the application, as
 *          given in a JAD file
 * @param name name of the suite, as given in a JAD file
 * @param id [out] receives the platform-specific storage name of the
 *          application given by vendorName and appName
 *
 * @return 0 if success, else an error code (OUT_OF_MEM_LEN)
 */
MIDPError
midp_create_suite_id(const pcsl_string* vendor, const pcsl_string* name, pcsl_string* id) {

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    *id = PCSL_STRING_NULL;
    if (midpInit(LIST_LEVEL) != 0) {
        return OUT_OF_MEM_LEN;
    }

    { /* performance hint: predict buffer capacity*/
        int id_len = ESCAPED_BUFFER_SIZE(pcsl_string_length(vendor)
                                         + pcsl_string_length(name)) + 2;
        pcsl_string_predict_size(id, id_len);
    }

    if (PCSL_STRING_OK ==
            pcsl_string_append_escaped_ascii(id, vendor)
     && PCSL_STRING_OK ==
            pcsl_string_append_char(id, '_')
     && PCSL_STRING_OK ==
            pcsl_string_append_escaped_ascii(id, name)
     && PCSL_STRING_OK ==
            pcsl_string_append_char(id, '_')) {
        return 0;
    }
    pcsl_string_free(id);
    return OUT_OF_MEM_LEN;
}

/**
 * If the suite exists, this function returns a unique identifier of
 * MIDlet suite. Note that suite may be corrupted even if it exists.
 *
 * @param vendor name of the vendor that created the application, as
 *          given in a JAD file
 * @param name name of the suite, as given in a JAD file
 * @id    [out] receives the platform-specific suite ID of the application
 *          given by vendorName and appName, or string with
 *          a null data if suite does not exist, or
 *          out of memory error occured, or suite is corrupted.
 *
 * @return  >0 if suite found
 *          NULL_LEN if suite does not exist,
 *          OUT_OF_MEM_LEN if out of memory or
 *          IO_ERROR if suite is corrupted
 */
MIDPError
midp_get_suite_id(const pcsl_string* vendor, const pcsl_string* name, pcsl_string* id) {
    pcsl_string suiteID;
    int status;
    *id = PCSL_STRING_NULL;
    {
            MIDPError rc;
            rc = midp_create_suite_id(vendor, name, &suiteID);
            if (rc == OUT_OF_MEM_LEN) {
                return OUT_OF_MEM_LEN;
            }
        }

    status = suite_exists(&suiteID);

    if ((status > 0)||(status == SUITE_CORRUPTED_ERROR)) {
        /* status > 0 implies that suite is successfully found */
        *id = suiteID;
        return status;
    }

    pcsl_string_free(&suiteID);
    return NULL_LEN;
}

/**
 * Stores or updates a suite.
 *
 * @param suiteID unique ID of the suite
 * @param jadUrl the absolute URL where the JAD came from, can be null
 * @param jadProps properties the JAD
 * @param jarUrl the absolute URL where the JAR came from
 * @param jarFilename the downloaded JAR
 * @param manifestProps properties of the manifest
 * @param domain security domain of the suite
 * @param trusted true if suite is trusted, false if not
 * @param pAuthPath if suite signed, the Certificate Authorization path,
 *        begining with the most trusted entity, that authorized it.
 *        This is a pointer to an array, so pAuthPath MUST NOT be NULL.
 *        However, *pAuthPath may be NULL (if the suite is not signed).
 * @param authPathLen length of the authorization path, 0 if suite not signed
 * @param pPermissions permissions for the suite, on permission per byte
 * @param permissionsLen length of the permissions array
 * @param pushInterruptSetting push interrupt setting for the suite
 * @param pushOptions user options for push interrupts
 * @param pVerifyHash hash value of the suite with preverified classes
 * @param verifyHashLen length of the hash value of the suite
 *
 * @return status 0 for success else an error code
 */
MIDPError midp_store_suite(const pcsl_string* suiteID, const pcsl_string* jadUrl,
        MidpProperties jadProps, const pcsl_string* jarUrl,
        const pcsl_string* jarFilename,
        MidpProperties manifestProps, const pcsl_string* domain, jboolean trusted,
        pcsl_string** pAuthPath, int authPathLen,
        unsigned char*pPermissions, int permissionsLen,
        unsigned char pushInterruptSetting, long pushOptions,
        unsigned char*pVerifyHash, int verifyHashLen) {

    MidpInstallInfo installInfo;
    int status;
    char* pszError;

    lockStorageList *node;

    node = find_storage_lock(suiteID);
    if (node != NULL) {
        if (node->update != KNI_TRUE) {
            return SUITE_LOCKED;
        }
    }

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    status = midpInit(LIST_LEVEL);
    if (status != 0) {
        return OUT_OF_MEM_LEN;
    }

    installInfo.jadUrl_s = *jadUrl;
    installInfo.jarUrl_s = *jarUrl;
    installInfo.domain_s = *domain;
    installInfo.trusted = trusted;
    installInfo.authPath_as = *pAuthPath;
    installInfo.authPathLen = authPathLen;

    do {
        status = addToSuiteList(suiteID);
        if ((status < 0)&&(status != SUITE_CORRUPTED_ERROR)) {
            break;
        }

        if (0 == store_jar(&pszError, suiteID, jarFilename)) {
            status = OUT_OF_MEM_LEN;
            break;
        }
        if (pszError != NULL) {
            storageFreeError(pszError);
            status = IO_ERROR_LEN;
            break;
        }

        if (0 == write_install_info(&pszError, suiteID, installInfo)) {
            status = OUT_OF_MEM_LEN;
            break;
        }

        if (pszError != NULL) {
            storageFreeError(pszError);
            status = IO_ERROR_LEN;
            break;
        }

        status = storeInstallProperties(suiteID, jadProps, manifestProps);
        if (status != 0) {
            break;
        }

        {
            int rc;

            /* Suites start off as enabled. */
            rc = writeSettings(&pszError, suiteID, KNI_TRUE, pushInterruptSetting,
                pushOptions, (jbyte*)pPermissions, permissionsLen);

            if (!rc) {
                status = OUT_OF_MEM_LEN;
                break;
            }
        }

        if (pszError != NULL) {
            storageFreeError(pszError);
            status = IO_ERROR_LEN;
            break;
        }

#if VERIFY_ONCE
        if (pVerifyHash != NULL) {
            status = writeVerifyHash(
                suiteID, (jbyte*)pVerifyHash, verifyHashLen);
            if (status != 0) {
               break;
            }
        }
#else
        /* Prevent compiler warnings about unused names */
        (void)pVerifyHash;
        (void)verifyHashLen;
#endif /* VERIFY_ONCE */

#if ENABLE_IMAGE_CACHE
        createImageCache(suiteID);
#endif
    } while (0);

    if (status != 0) {
        midp_remove_suite(suiteID);
    }

    return status;
}

/**
 * Stores the properties of a MIDlet suite to persistent storage.
 * Take 2 property lists for convenience. The JAD properties are written
 * first then the JAR properties.
 * <pre>
 * The format of the properties file will be:
 * <number of strings as int (2 strings per property)>
 *    {repeated for each property}
 *    <length of a property key as int>
 *    <property key as jchars>
 *    <length of property value as int>
 *    <property value as jchars> * </pre>
 * @param suiteID  ID of the suite
 * @param jadProps an array of strings, in a pair pattern of key and
 *  value
 * @param jarProps an array of strings, in a pair pattern of key and
 *  value
 *
 * @return 0 for success or a MIDPError error code
 */
static MIDPError
storeInstallProperties(const pcsl_string* suiteID, MidpProperties jadProps,
                       MidpProperties jarProps) {
    pcsl_string filename;
    char* pszError = NULL;
    int handle;
    int numberOfStrings;
    int i;
    int status = 0;
    MIDP_ERROR rc;

    rc = midp_example_get_property_file(suiteID, KNI_TRUE, &filename);
    if (MIDP_ERROR_NONE != rc) {
        return OUT_OF_MEM_LEN;
    }

    handle = storage_open(&pszError, &filename, OPEN_READ_WRITE_TRUNCATE);
    pcsl_string_free(&filename);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return IO_ERROR_LEN;
    }

    do {
        numberOfStrings = (jadProps.numberOfProperties +
                           jarProps.numberOfProperties) * 2;
        storageWrite(&pszError, handle, (char*)&numberOfStrings,
                     sizeof (numberOfStrings));
        if (pszError != NULL) {
            break;
        }

        /* The JAD properties override the JAR properties. Write them first. */
        for (i = 0; i < jadProps.numberOfProperties * 2; i++) {
            storage_write_utf16_string(&pszError, handle, &jadProps.pStringArr[i]);
            if (pszError != NULL) {
                break;
            }
        }

        if (pszError != NULL) {
            break;
        }

        for (i = 0; i < jarProps.numberOfProperties * 2; i++) {
            storage_write_utf16_string(&pszError, handle, &jarProps.pStringArr[i]);
            if (pszError != NULL) {
                break;
            }
        }
    } while(0);

    if (pszError != NULL) {
        status = IO_ERROR_LEN;
        storageFreeError(pszError);
    }

    storageClose(&pszError, handle);
    storageFreeError(pszError);
    return status;
}

/**
 * Gets location of the properties file
 * for the suite with the specified suiteId.
 *
 * Note that in/out parameter filename MUST be allocated by callee with
 * midpMalloc, the caller is responsible for freeing it.
 *
 * @param suiteId    - The application suite ID string
 * @param checkSuiteExists - true if suite should be checked for existence or not
 * @param filename - The in/out parameter that contains returned filename
 * @return  error code that should be one of the following:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM,
 *       MIDP_ERROR_AMS_SUITE_NOT_FOUND
 * </pre>
 */
MIDP_ERROR midp_example_get_property_file(const pcsl_string* suiteId,
                                      jboolean checkSuiteExists,
                                      pcsl_string *filename) {
  return midp_example_get_suite_resource_file(suiteId, &PROPS_FILENAME,
            checkSuiteExists, filename);
}

/**
 * Gets location of the file associated with the named resource
 * of the suite with the specified suiteId.
 *
 * Note that in/out parameter filename MUST be allocated by callee with 
 * midpMalloc, the caller is responsible for freeing it.
 *
 * @param suiteId    - The application suite ID string
 * @param resourceName - The name of suite resource whose location is requested
 * @param checkSuiteExists - true if suite should be checked for existence or not
 * @param filename - The in/out parameter that contains returned filename
 * @return  error code that should be one of the following:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM,
 *       MIDP_ERROR_AMS_SUITE_NOT_FOUND,MIDP_ERROR_AMS_SUITE_CORRUPTED,
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
MIDP_ERROR midp_example_get_suite_resource_file(const pcsl_string* suiteID,
                      const pcsl_string*  resourceName,
				      jboolean checkSuiteExists,
				      pcsl_string *filename) {
  pcsl_string returnFilename = PCSL_STRING_NULL;
  int rc;
  *filename = PCSL_STRING_NULL;

  if (checkSuiteExists) {
    rc = build_suite_filename(suiteID, resourceName, &returnFilename);
  } else {
    rc = get_suite_filename(suiteID, resourceName, &returnFilename);
  }

  switch(rc) {
  case 0:
    break;
  default: /* IMPL_NOTE: add more errcode analysis branches */
  case OUT_OF_MEM_LEN:
    return MIDP_ERROR_OUT_MEM;
  case NULL_LEN:
    return MIDP_ERROR_AMS_SUITE_NOT_FOUND;
  }

  *filename = returnFilename;

  return MIDP_ERROR_NONE;
}
