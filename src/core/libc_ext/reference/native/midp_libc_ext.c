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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <midp_libc_ext.h>
#include <midp_logging.h>

/**
 * Not all compilers provide snprintf function, so we have 
 * to use workaround. In debug mode it does buffer overflow
 * checking, and in release mode it works as sprintf.
 */
int midp_snprintf(char* buffer, int bufferSize, const char* format, ...) {
    va_list argptr;
    int rv;
    
    /*
     * To prevent warning about unused variable when
     * not checking for overflow
     */
    (void)bufferSize;

    va_start(argptr, format);
    rv = midp_vsnprintf(buffer, bufferSize, format, argptr);
    va_end(argptr);

    return rv;
}

/**
 * Same as for snprintf. Not all compilers provide vsnprintf function, 
 * so we have to use workaround. Platform's vsnprintf function can be used
 * instead of midp_vsnprintf when it is supported.
 * 
 */
#define VSPRINTF_BUFFER_SIZE 1024
int midp_vsnprintf(char *buffer, int bufferSize, 
        const char* format, va_list argptr) {

	int rv, num;
    char vsprintf_buffer[VSPRINTF_BUFFER_SIZE];

    /* Fill num as minimum of bufferSize and VSPRINTF_BUFFER_SIZE */
	if (bufferSize > VSPRINTF_BUFFER_SIZE) {
        num = VSPRINTF_BUFFER_SIZE;
		REPORT_WARN1(LC_CORE,
            "Internal buffer is smaller than %d, data truncated", bufferSize);
	} else 
        num = bufferSize;
 
   /* IMPL_NOTE: using vsprintf function is not safely because it can
    * overflow output buffer. The javacall implementation will support
	* size limit format output and this function will be used here.
    * Current implementation detects buffer overflow by checking the last
	* byte and reports about it. In this case the VSPRINTF_BUFFER_SIZE
    * value need be increased.
    */
    rv = vsprintf(vsprintf_buffer, format, argptr);
    if (rv > VSPRINTF_BUFFER_SIZE - 1) {
		REPORT_CRIT2(LC_CORE, "Internal buffer %p overflow detected at %p !!!",
            vsprintf_buffer, vsprintf_buffer + VSPRINTF_BUFFER_SIZE);
    }
    if (num  > rv + 1) {
		num = rv + 1;
	} else 
        vsprintf_buffer[num - 1] = 0;

    memcpy(buffer, vsprintf_buffer, num);

    return rv;
}

/**
 * Not all compilers provide the POSIX function strcasesmp, so we need to
 * use workaround for it. The function compares to strings ignoring the
 * case of characters. How uppercase and lowercase characters are related
 * is determined by the currently selected locale.
 *
 * @param s1 the first string for comparision
 * @param s2 the second string for comparison
 * @return an integer less than, equal to, or greater than zero if s1 is
 *   found, respectively, to be less than, to  match, or be greater than s2.
 */
int midp_strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int d = tolower(*s1) - tolower(*s2);
        if (d) return d;
        ++s1;
        ++s2;
    }
    return tolower(*s1) - tolower(*s2);
}

/**
 * Same as for midp_strcasecmp, except it only compares the first n
 * characters of s1.
 */
int midp_strncasecmp(const char *s1, const char *s2, size_t n) {
    if (!n) return 0;
    while (*s1 && *s2) {
        int d = tolower(*s1) - tolower(*s2);
        if (d) return d;
        else if (!--n) return 0;
        ++s1;
        ++s2;
    }
    return tolower(*s1) - tolower(*s2);
}
