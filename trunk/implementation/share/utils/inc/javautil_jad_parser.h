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
#ifndef __JAVAUTIL_JAD_PARSER_H_
#define __JAVAUTIL_JAD_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"

 /**
 * @enum javacall_lifecycle_parse_result
 */
typedef enum {
    JAVACALL_PARSE_OK = 0,
    JAVACALL_PARSE_FAILED = -1,
    JAVACALL_PARSE_MISSING_MIDLET_NAME_PROP = -2,
    JAVACALL_PARSE_MISSING_MIDLET_VERSION_PROP = -3,
    JAVACALL_PARSE_MISSING_MIDLET_VENDOR_PROP = -4,
    JAVACALL_PARSE_MISSING_MIDLET_JAR_URL_PROP = -5,
    JAVACALL_PARSE_MISSING_MIDLET_JAR_SIZE_PROP = -6,
    JAVACALL_PARSE_NOT_ENOUGH_STORAGE_SPACE = -7,
    JAVACALL_PARSE_CANT_READ_JAD_FILE = -8
} javacall_parse_result;

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
                                              /* OUT */ javacall_parse_result* status);

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
                            int jadPathLen, char** destBuf);

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
                                                  /* OUT */ int* numOfProps);

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
                                       /* OUT */ int* jadLineSize);

#ifdef __cplusplus
}
#endif

#endif
