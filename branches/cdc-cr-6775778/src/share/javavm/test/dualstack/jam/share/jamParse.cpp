/*
 * @(#)jamParse.cpp	1.7 06/10/10
 * 
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

/*=========================================================================
 * Include files
 *=======================================================================*/

#include "jam.hpp"

/*=========================================================================
 * Functions
 *=======================================================================*/

/*
 * JamGetProp
 *
 *      Parse a text buffer of property name-value pairs and return
 *      the value of the property identified by <propName>. <buffer>
 *      is a NUL-terminated string containing text in the following format:
 *
 *              "Prop-Name1: prop value 1\nProp-Name2: prop value 2..."
 *
 * Return-value:
 *
 *      A pointer into the buffer of the first non-space character of
 *      the property value. <*length> returns the length of the property
 *      value, excluding any trailing white-space characters.
 *
 *      If the given property is not found in <buffer>, NULL is returned.
 */
char*
JamGetProp(char* buffer, char* name, int* length) {
    char* p;
    char* retval;
    int len;

    for (p=buffer;*p;) {
        if (strncmp(p, name, strlen(name)) != 0) {
            while (*p) {
                if (*p == '\n') {
                    ++p;
                    break;
                }
                ++ p;
            }
            continue;
        }
        p += strlen(name);
        while (*p && *p != '\n' && IS_SPACE(*p)) {
            p++;
        }
        if (*p == ':') {
            p++;
        }
        while (*p && *p != '\n' && IS_SPACE(*p)) {
            p++;
        }
        retval = p;
        while (*p && *p != '\n') {
            p++;
        }
        while (p > retval && IS_SPACE(*p)) {
            p--;
        }
        len = (p - retval + 1);

        if (length == NULL) { /* DUP the string */
            char * newStr;
            if ((newStr = (char*)jam_malloc(len + 1)) == NULL) {
                return NULL;
            }
            strnzcpy(newStr, retval, len);
            newStr[len] = 0;
            for (p=newStr; *p; p++) {
                if (*p == '\r') {
                    *p = '\0';
                }
            }
            return newStr;
        } else {
            *length = len;
            return retval;
        }
    }
    return NULL;
}

