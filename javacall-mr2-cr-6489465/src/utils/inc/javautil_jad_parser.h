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

#ifdef __cplusplus
}
#endif

#endif
