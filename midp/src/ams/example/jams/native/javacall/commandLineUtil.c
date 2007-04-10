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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <midpStorage.h>
#include <pcsl_string.h>
#include <midp_logging.h>
#include <javacall_defs.h>
#include <javacall_dir.h>

#include "commandLineUtil.h"

static char* getCharFileSeparator() {
    /*
     * This function is called before the debug heap is initialized.
     * so we can't use midpMalloc.
     */
    static char fsep[2];

    fsep[0] = (char)storageGetFileSeparator();
    fsep[1] = 0;

    return fsep;
}

/**
 * Generates a correct MIDP home directory based on several rules. If
 * the <tt>MIDP_HOME</tt> environment variable is set, its value is used
 * unmodified. Otherwise, this function will search for the <tt>appdb</tt>
 * directory in the following order:
 * <ul>
 * <li>current directory (if the MIDP executable is in the <tt>PATH</tt>
 *     environment variable and the current directory is the right place)
 * <li>the parent directory of the midp executable
 * <li>the grandparent directory of the midp executable
 * </ul>
 * <p>
 * If <tt>cmd</tt> does not contain a directory (i.e. just the text
 * <tt>midp</tt>), the search starts from the current directory. Otherwise,
 * the search starts from the directory specified in <tt>cmd</tt> (i.e.
 * start in the directory <tt>bin</tt> if <tt>cmd</tt> is <tt>bin/midp</tt>).
 * <p>
 * <b>NOTE:</b> This is only applicable for development platforms.
 *
 * @param cmd A 'C' string containing the command used to start MIDP.
 * @return A 'C' string the found MIDP home directory, otherwise
 *         <tt>NULL</tt>, this will be a static buffer, so that it safe
 *       to call this function before midpInitialize, don't free it
 */

#define MAX_PATH_LEN JAVACALL_MAX_ROOT_PATH_LENGTH
    
char* midpFixMidpHome(char *cmd) {

    static javacall_utf16 path[MAX_PATH_LEN];
    static char midpRealHome[MAX_PATH_LEN];
    javacall_result ret;
    int len = MAX_PATH_LEN - 1;
    pcsl_string str = PCSL_STRING_NULL_INITIALIZER;
            

    ret = javacall_dir_get_root_path (path, &len);
            
    if ( ret != JAVACALL_OK ) {
        REPORT_ERROR(LC_AMS,"midpFixMidpHome() << Root path query failed.");
        return NULL;
    }
                
    if (PCSL_STRING_OK != pcsl_string_convert_from_utf16 (path, len, &str)) {
        REPORT_ERROR(LC_AMS,"midpFixMidpHome() << pcsl_string conversion operation failed.");
        return NULL;
    }

    if (pcsl_string_utf8_length (&str) >= MAX_PATH_LEN) {
        REPORT_ERROR(LC_AMS,"midpFixMidpHome() << Root path length is too large.");
        pcsl_string_free (&str);
        return NULL;
    }

    if (PCSL_STRING_OK != pcsl_string_convert_to_utf8 (&str, 
                                                       (jbyte *)midpRealHome, 
                                                       MAX_PATH_LEN-1, 
                                                       &len)) {
        REPORT_ERROR(LC_AMS,"midpFixMidpHome() << pcsl_string conversion operation failed.");
        pcsl_string_free (&str);
        return NULL;
    };
    pcsl_string_free (&str);
    
    midpRealHome[len] = 0;
    return midpRealHome;
}

/**
 * Removes the flag and value for a given option from command line argument
 * array.
 *
 * @param pszFlag flag for the option
 * @param apszArgs array of arguments
 * @param pArgc pointer to the count of arguments in the array, it
 * will updated with the length
 *
 * @return value of the option or NULL if it does not exist
 */
char* midpRemoveCommandOption(char* pszFlag, char* apszArgs[], int* pArgc) {
    int i;
    int len = *pArgc;
    char* result = NULL;

    for (i = 0; i < (len - 1); i++) {
        if (strcmp(pszFlag, apszArgs[i]) == 0) {
            result = apszArgs[i + 1];
            break;
        }
    }

    if (result == NULL) {
        return NULL;
    }

    /* Remove the flag and value of the option. */
    for (; i < (len - 2); i++) {
        apszArgs[i] = apszArgs[i + 2];
    }

    *pArgc = len - 2;
    return result;
}

/**
 * Removes a given option flag from command line argument
 *
 * @param pszFlag flag for the option
 * @param apszArgs array of arguments
 * @param pArgc pointer to the count of arguments in the array, it
 * will updated with the length
 *
 * @return value of the flag or NULL if it does not exist
 */
char* midpRemoveOptionFlag(char* pszFlag, char* apszArgs[], int* pArgc) {
    int i;
    int len = *pArgc;
    char* result = NULL;

    for (i = 0; i < len; i++) {
        if (strcmp(pszFlag, apszArgs[i]) == 0) {
            result = apszArgs[i];
            break;
        }
    }

    if (result == NULL) {
        return NULL;
    }

    /* Remove the flag */
    for (; i < (len - 1); i++) {
        apszArgs[i] = apszArgs[i + 1];
    }

    *pArgc = len - 1;
    return result;
}
