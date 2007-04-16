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

#if ENABLE_DEBUG
/**
 * Same as for midp_snprintf. Not all compilers provide vsnprintf 
 * function, so we have to use workaround. In debug mode it does 
 * buffer overflow checking, and in release mode it works as vsprintf.
 */
int midp_vsnprintf(char *buffer, int bufferSize, 
        const char* format, va_list argptr) {

    int rv;
    
    buffer[bufferSize-1] = '\0';
    rv = vsprintf(buffer, format, argptr);

    if (buffer[bufferSize-1] != '\0') {
        buffer[bufferSize-1] = '\0';
        REPORT_CRIT2(LC_CORE, "Buffer %p overflow detected at %p !!!",
                buffer, buffer + bufferSize);
    }

    return rv;
}
#endif /* ENABLE_DEBUG */

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
