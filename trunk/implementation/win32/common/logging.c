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
* win32 implementation for logging javacall functions
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "local_defs.h"
#include "javacall_logging.h"

char print_buffer[PRINT_BUFFER_SIZE];

/**
* Prints out a string to a system specific output stream
*
* @param s a NULL terminated character buffer to be printed
*/
void javacall_print(const char *s) {
    if(s != NULL) {
        printf("%s", s);
    } else {
        printf("NULL\n");
    }
}

void javacall_print_chars(const char* s, int length) {   
    fwrite(s, length, 1, stdout);
}


/*
 * Returns the jlong-specifier prefix used with type characters in
 * printf functions or wprintf functions to specify interpretation
 * of jlong, the 64-bit signed integer type, 
 * e.g. for win32 is "%I64d", for linux is "%lld"
 */
const char * javacall_jlong_format_specifier() {
  return "%I64d";
}

/*
 * Returns the julong-specifier prefix used with type characters in
 * printf functions or wprintf functions to specify interpretation
 * of julong, the 64-bit unsigned integer type,
 * e.g. for win32 is "%I64u", for linux is "%llu"
 */
const char * javacall_julong_format_specifier() {
  return "%I64u";
}

#ifdef __cplusplus
}
#endif

