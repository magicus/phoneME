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

#ifndef MIDP_LIBC_EXT_H
#define MIDP_LIBC_EXT_H


/**
 * @defgroup core_libc External Interface to ANSI C Extensions
 * @ingroup core
 */

/**
 * @file
 * @ingroup core_libc
 *
 * @brief Uniform interface to libc extensions to ANSI C
 */

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Not all compilers provide snprintf function, so we have 
 * to use workaround. In debug mode it does buffer overflow
 * checking, and in release mode it works as sprintf.
 */
extern int midp_snprintf(char* buffer, int bufferSize, 
        const char* format, ...);

/**
 * Same as for snprintf. Not all compilers provide vsnprintf function, 
 * so we have to use workaround. Platform's vsnprintf function can be used
 * instead of midp_vsnprintf when it is supported.
 * 
 */
extern int midp_vsnprintf(char *buffer, int bufferSize, 
        const char* format, va_list argptr);

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
extern int midp_strcasecmp(const char *s1, const char *s2);

/**
 * Same as for midp_strcasecmp, except it only compares the first n
 * characters of s1.
 */
extern int midp_strncasecmp(const char *s1, const char *s2, size_t n);

#ifdef __cplusplus
}
#endif  
    
#endif /* MIDP_LIBC_EXT_H */

