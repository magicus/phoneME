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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <midpStorage.h>

/**
 * Returns a platform-specific file separator represented as a string.
 *
 * @return pointer to null-terminated string containing the file separator
 */
char* getCharFileSeparator() {
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
