/*
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
 *
 * Implmentation of jad file parsing
 */

#include <string.h>
#include "javautil_jad_parser.h"
#include "javautil_string.h"
#include "javacall_memory.h"
#include "javacall_file.h"
#include "javacall_dir.h"

static char NAME_PROP[]      = "MIDlet-Name";
static char VERSION_PROP[]   = "MIDlet-Version";
static char VENDOR_PROP[]    = "MIDlet-Vendor";
static char JAR_URL_PROP[]   = "MIDlet-Jar-URL";
static char JAR_SIZE_PROP[]  = "MIDlet-Jar-Size";
static char DATA_SIZE_PROP[] = "MIDlet-Data-Size";

/** structure for jad properties */
typedef struct _javacall_jad_properties {
    char* midletName;
    char* midletVersion;
    char* midletVendor;
    char* midletJarUrl;
    int midletJarSize;
    int dataSize;
} javacall_jad_properties;

/* auxiliary functions definitions */
javacall_result javautil_parse_jad(char** jadBuffer,
                                   javacall_jad_properties* jadProps);
javacall_bool javautil_is_new_line(char* c);
javacall_result javautil_store_property(javacall_jad_properties* jadProps,
                                        char* propName, char* propValue);
javacall_result javautil_check_must_properties(javacall_jad_properties jadProps,
                                               javacall_parse_result* status);
javacall_result javautil_create_jar_url(char* jadUrl,
                                        javacall_jad_properties jadProps,
                                        char** jarUrl);

/**
 * Extract the jar URL from a given jad file.
 *
 * @param jadPath unicode string of path to jad file in local FS
 * @param jadPathLen length of the jad path
 * @param jadUrl URL from which the jad was downloaded
 * @param jarUrl pointer to the jarUrl extracted from the jad file
 * @param status status of jad file parsing
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_get_jar_url_from_jad(const javacall_utf16* jadPath,
                                              int jadPathLen,
                                              char* jadUrl,
                                              /* OUT */ char** jarUrl,
                                              /* OUT */ javacall_parse_result* status) {
    javacall_result res;
    int jadUrlLen = strlen(jadUrl);
    long jadSize;
    char* jadBuffer;
    char* jadBufferStart;
    javacall_jad_properties jadProps;
    javacall_int64 freeSpace, neededSpace;

    /**
     * Read the entire jad file into a buffer.
     * Parse JAD file and get required fields.
     * MIDP2.0 spec specifies the application descriptor MUST contain
     * the following attributes:
     *   MIDlet-Name
     *   MIDlet-Version
     *   MIDlet-Vendor
     *   MIDlet-Jar-URL
     *   MIDlet-Jar-Size
     */
    jadSize = javautil_read_jad_file(jadPath, jadPathLen, &jadBuffer);
    if (jadSize <= 0 || jadBuffer == NULL) {
        *status = JAVACALL_PARSE_CANT_READ_JAD_FILE;
        return jadSize;
    }
    jadBufferStart = jadBuffer;

    jadProps.dataSize = 0;
    res = javautil_parse_jad(&jadBuffer, &jadProps);
    if (res != JAVACALL_OK) {
        *status = JAVACALL_PARSE_FAILED;
        return res;
    }

    // check if all required properties are present
    res = javautil_check_must_properties(jadProps, status);
    if (res != JAVACALL_OK) {
        return res;
    }

    /*
     * Check if there's anough storage space to install the application.
     * A suite is a jad + jar + manifest + url + data size.
     * Assuming the manifest is the same size as the jad
     * The size is in bytes and UTF-8 chars can be upto 3 bytes
     */
    freeSpace = javacall_dir_get_free_space_for_java();
    neededSpace = jadProps.midletJarSize + (jadSize * 2) +
                  (jadUrlLen * 3) + jadProps.dataSize;

    if (neededSpace > freeSpace) {
        *status = JAVACALL_PARSE_NOT_ENOUGH_STORAGE_SPACE;
        return JAVACALL_FAIL;
    }

    // return Jar URL to the platform
    res = javautil_create_jar_url(jadUrl, jadProps, jarUrl);

    // free all resources
    javacall_free(jadBufferStart);
    javacall_free(jadProps.midletName);
    javacall_free(jadProps.midletVersion);
    javacall_free(jadProps.midletVendor);
    javacall_free(jadProps.midletJarUrl);

    if (res == JAVACALL_OUT_OF_MEMORY) {
        *status = JAVACALL_PARSE_FAILED;
        return JAVACALL_FAIL;
    }

    *status = JAVACALL_PARSE_OK;
    return JAVACALL_OK;
}

/**
 * Opens the jad file and fills the content of the file in DestBuf.
 * This function allocates memory for the buffer.
 *
 * @param jadPath full path to the jad file.
 * @param Destbuf pointer to the buffer that will be filled by the
 *        contents of the file.
 * @return buffer file size in bytes
 */
long javautil_read_jad_file(const javacall_utf16* jadPath,
                            int jadPathLen, char** destBuf) {
    javacall_handle jadHandle;
    javacall_result res;
    long jadSize;
    char* jadBuf;
    long bytesRead;

    *destBuf = NULL;

    // check if file name is valid
    if (jadPath == NULL) {
        return JAVACALL_BAD_FILE_NAME;
    }

    // open the jad file in read-only mode
    res = javacall_file_open(jadPath, jadPathLen,
                             JAVACALL_FILE_O_RDONLY, &jadHandle);
    if (res != JAVACALL_OK) {
        return JAVACALL_FILE_NOT_FOUND;
    }

    // get the open jad file size
    jadSize = (long)javacall_file_sizeofopenfile(jadHandle);
    if (-1L == jadSize) {
        javacall_file_close(jadHandle);
        return JAVACALL_IO_ERROR;
    }

    // allocate a buffer to hold the jad file contents
    // NEED REVISIT: SHOULD USE pcsl_mem_malloc() ?
    jadBuf = (char*)javacall_malloc(jadSize+1);
    if (jadBuf == NULL) {
        javacall_file_close(jadHandle);
        return JAVACALL_OUT_OF_MEMORY;
    }
    memset(jadBuf, 0, (jadSize+1));

    // read the entire jad file into the allocated buffer
    bytesRead = javacall_file_read(jadHandle, (unsigned char*)jadBuf, jadSize);
    if ((bytesRead <= 0) || (bytesRead != jadSize)) {
        javacall_free(jadBuf);
        javacall_file_close(jadHandle);
        return JAVACALL_IO_ERROR;
    }

    // close the file and return the required information
    javacall_file_close(jadHandle);

    *destBuf = jadBuf;
    return jadSize;
}

/**
 * Parse the given jad file and extract the required properties.
 * Every property occupies one line of the input stream. Each line
 * is terminated by a line terminator which can be a LF, or (CR LF).
 * Lines from the input stream are processed until end of file
 * is reached on the input stream.
 * <p>
 * Every line describes one property to be added to the table.
 * The key consists of all the characters from beginning of the line
 * up to, but not including the first ASCII <code>:</code>.
 * All remaining characters on the line become part of the associated
 * element. The element is also trimmed of leading and trailing
 * whitespace.
 * <p>
 * This method will try to continue after a format error and load as
 * many properties it can, but return the last error encountered.
 *
 * @param jadbuffer buffer that contains the jad file contents.
 * @param jadProperties pointer to properties structure filled with keys
 *        and values from the jad
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_parse_jad(char** jadBuffer,
                                   javacall_jad_properties* jadProps) {
    javacall_result res;
    int propsNum=0;
    int i;
    char* jadLine;
    int jadLineSize;
    int index;
    char* jadPropName;
    char* jadPropValue;

    res = javautil_get_number_of_properties(*(jadBuffer), &propsNum);
    if ((res != JAVACALL_OK) || (propsNum <= 0)) {
        return JAVACALL_OUT_OF_MEMORY;
    }

    for (i=0; i<propsNum; i++) {
        // read a line from the jad file.
        res = javautil_read_jad_line(jadBuffer, &jadLine, &jadLineSize);
        if (res == JAVACALL_END_OF_FILE) {
            break;
        } else if ((res != JAVACALL_OK) ||
                   (jadLine == NULL) ||
                   (jadLineSize == 0)) {
            return JAVACALL_FAIL;
        }

        // find the index of ':'
        res = javautil_string_index_of(jadLine, COLON, &index);
        if ((res != JAVACALL_OK) || (index <= 0)) {
            javacall_free(jadLine);
            continue;
        }

        // get sub string of jad property name
        res = javautil_string_substring(jadLine, 0, index, &jadPropName);
        if (res == JAVACALL_OUT_OF_MEMORY) {
            javacall_free(jadLine);
            return res;
        }
        if ((res != JAVACALL_OK) || (jadPropName == NULL)) {
            javacall_free(jadLine);
            continue;
        }

        res = javautil_string_trim(jadPropName);
        if (res != JAVACALL_OK) {
            javacall_free(jadLine);
            javacall_free(jadPropName);
            continue;
        }

        // skip white space between jad property name and value
        while (*(jadLine+index+1) == SPACE) {
            index++;
        }

        // get sub string of jad property value
        res = javautil_string_substring(jadLine, index+1, jadLineSize, &jadPropValue);
        if (res == JAVACALL_OUT_OF_MEMORY) {
            javacall_free(jadLine);
            javacall_free(jadPropName);
            return res;
        }
        if ((res != JAVACALL_OK) || (jadPropValue == NULL)) {
            javacall_free(jadLine);
            javacall_free(jadPropName);
            continue;
        }

        /* jad property name an value are available,
         * we can free the jad file line */
        javacall_free(jadLine);

        res = javautil_string_trim(jadPropValue);
        if (res != JAVACALL_OK) {
            javacall_free(jadPropName);
            javacall_free(jadPropValue);
            continue;
        }

        res = javautil_store_property(jadProps, jadPropName, jadPropValue);
        if (res != JAVACALL_OK) {
            javacall_free(jadPropName);
            javacall_free(jadPropValue);
            continue;
        }

        /* jad property value is stored, can free the property name */
        javacall_free(jadPropName);

    } // end of for loop

    return JAVACALL_OK;
}

/**
 * Count the number of properties in the jad file.
 * Skip commented out lines and blank lines.
 *
 * @param jadBuffer buffer that contains the jad file contents.
 * @param numOfProps variable to hold the number of properties
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_get_number_of_properties(char* jadBuffer,
                                                  /* OUT */ int* numOfProps) {
    int linesNum = 0;
    *numOfProps = -1;

    if (jadBuffer == NULL) {
        return JAVACALL_FAIL;
    }

    while (*jadBuffer) {
        /* skip commented out lines and blank lines */
        if ((*(jadBuffer) == POUND) || (javautil_is_new_line(jadBuffer))) {
            /* run to the end of line */
            while (!javautil_is_new_line(jadBuffer)) {
                jadBuffer++;
            }

            /* skip all the new line characters an the end of line */
            while (javautil_is_new_line(jadBuffer)) {
                jadBuffer++;
            }
        } else {
            linesNum++;
            while (!javautil_is_new_line(jadBuffer)) {
                jadBuffer++;
            }
            while (javautil_is_new_line(jadBuffer)) {
                jadBuffer++;
            }
        }
    }

    *numOfProps = linesNum;
    return JAVACALL_OK;
}

/**
 * Check if character is a new-line character.
 * @param c character to check
 * @return <code>JAVACALL_TRUE</code> if equal,
 *         <code>JAVACALL_FALSE</code> otherwise.
 */
javacall_bool javautil_is_new_line(char* c) {

    if (c == NULL) {
        return JAVACALL_FALSE;
    }

    if ((*(c) == CR) || (*(c) == LF) || ((*(c) == CR) && (*(c+1) == LF))) {
        return JAVACALL_TRUE;
    }

    return JAVACALL_FALSE;
}

/**
 * Read a line from the jad file.
 * This function allocates memory for the line.
 *
 * @param jadBuffer pointer to the buffer that contains the jad file contents.
 * @param jadLine pointer to jad line read
 * @param jadLineSize size of the jad line read
 *
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_read_jad_line(char** jadBuffer,
                                       /* OUT */ char** jadLine,
                                       /* OUT */ int* jadLineSize) {

    char* line;
    char* lineStart;
    char* pJadBuf = *jadBuffer;
    int charCount=0;

    *jadLine = NULL;
    *jadLineSize = -1;

    if (!(*pJadBuf)) {
        return JAVACALL_END_OF_FILE;
    }

    /* skip commented out and blank lines */
    while ((*pJadBuf == POUND) || (javautil_is_new_line(pJadBuf))) {

        while (!javautil_is_new_line(pJadBuf)) {
            pJadBuf++; // skip commented out line
        }
        while (javautil_is_new_line(pJadBuf)) {
            pJadBuf++; // skip blank lines
        }
    }

    lineStart = pJadBuf;

    while (*pJadBuf) {
        charCount++;
        pJadBuf++;

        /* reached the end of the line */
        if (javautil_is_new_line(pJadBuf)) {
            /* if not end of file, point to the next jad file line */
            if (*(pJadBuf+1)) {
                pJadBuf++;
                break;
            }
        }
    }

    *jadBuffer = pJadBuf; // jadBuffer points to the next jad line

    line = (char*)javacall_malloc(charCount + 1);
    if (line == NULL) {
        return JAVACALL_OUT_OF_MEMORY;
    }
    memset(line, 0, charCount + 1);
    memcpy(line, lineStart, charCount);

    *jadLine = line;
    *jadLineSize = charCount;

    return JAVACALL_OK;
}

/**
 * Store a jad property in its proper place in the had properties structure.
 *
 * @param jadProps the jad properties structure
 * @param propName the jad property name
 * @param propValue the jad property value
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_store_property(javacall_jad_properties* jadProps,
                                        char* propName, char* propValue) {

    int jarSize=0;
    int dataSize=0;

    if (javautil_string_equals(propName, NAME_PROP)) {
        jadProps->midletName = propValue;
        return JAVACALL_OK;
    }

    if (javautil_string_equals(propName, VERSION_PROP)) {
        jadProps->midletVersion = propValue;
        return JAVACALL_OK;
    }

    if (javautil_string_equals(propName, VENDOR_PROP)) {
        jadProps->midletVendor = propValue;
        return JAVACALL_OK;
    }

    if (javautil_string_equals(propName, JAR_URL_PROP)) {
        jadProps->midletJarUrl = propValue;
        return JAVACALL_OK;
    }

    if (javautil_string_equals(propName, JAR_SIZE_PROP)) {
        javautil_string_parse_int(propValue, &jarSize);
        javacall_free(propValue);
        jadProps->midletJarSize = jarSize;
        if (jarSize != -1) {
            return JAVACALL_OK;
        }
    }

    if (javautil_string_equals(propName, DATA_SIZE_PROP)) {
        javautil_string_parse_int(propValue, &dataSize);
        javacall_free(propValue);
        if (dataSize == -1) {
            dataSize = 0;
        }
        jadProps->dataSize = dataSize;
        if (dataSize != -1) {
            return JAVACALL_OK;
        }
    }

    return JAVACALL_FAIL;
}

/**
 * Check if all mandatory jad properties are present
 *
 * @param jadProps jad properties structure
 * @param status indicates what is the problem in case of failure
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_check_must_properties(javacall_jad_properties jadProps,
                                               javacall_parse_result* status) {
    if (jadProps.midletName == NULL) {
        *status = JAVACALL_PARSE_MISSING_MIDLET_NAME_PROP;
        return JAVACALL_FAIL;
    }

    if (jadProps.midletVersion == NULL) {
        *status = JAVACALL_PARSE_MISSING_MIDLET_VERSION_PROP;
        return JAVACALL_FAIL;
    }

    if (jadProps.midletVendor == NULL) {
        *status = JAVACALL_PARSE_MISSING_MIDLET_VENDOR_PROP;
        return JAVACALL_FAIL;
    }

    if (jadProps.midletJarUrl == NULL) {
        *status = JAVACALL_PARSE_MISSING_MIDLET_JAR_URL_PROP;
        return JAVACALL_FAIL;
    }

    if (jadProps.midletJarSize == -1) {
        *status = JAVACALL_PARSE_MISSING_MIDLET_JAR_SIZE_PROP;
        return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}

/**
 * Create a jar url from given jad URL and the MIDlet-Jar_URL property
 * in the jad file.
 *
 * @param jadUrl given jad URL
 * @param jadProps jad properties structure
 * @param jarUrl pointer to jar URL
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_create_jar_url(char* jadUrl,
                                        javacall_jad_properties jadProps,
                                        char** jarUrl) {
    javacall_result res;
    int index;
    int jarUrlLen = strlen(jadProps.midletJarUrl);
    char separator = '/';
    char* tmpUrl;

    *jarUrl = NULL;

    /* if jar URL is absolute, return the same URL */
    if ((jadProps.midletJarUrl[0] == 'h') &&
        (jadProps.midletJarUrl[1] == 't') &&
        (jadProps.midletJarUrl[2] == 't') &&
        (jadProps.midletJarUrl[3] == 'p')) {

        tmpUrl = (char*)javacall_malloc(jarUrlLen + 1);
        if (tmpUrl == NULL) {
            return JAVACALL_OUT_OF_MEMORY;
        }
        memcpy(tmpUrl, jadProps.midletJarUrl, jarUrlLen+1);
        *jarUrl = tmpUrl;
        return JAVACALL_OK;
    }

    /* jar URL is relative, get jad URL without jad file name
     * and append the jar file name */
    res = javautil_string_last_index_of(jadUrl, separator, &index);
    tmpUrl = (char*)javacall_malloc(index + 1 + jarUrlLen + 1);
    if (tmpUrl == NULL) {
        return JAVACALL_OUT_OF_MEMORY;
    }
    memcpy(tmpUrl, jadUrl, index+1);
    memcpy(tmpUrl+index+1, jadProps.midletJarUrl, jarUrlLen+1);

    *jarUrl = tmpUrl;
    return JAVACALL_OK;
}

