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
 * so we have to use workaround. In debug mode it does buffer 
 * overflow checking, and in release mode it works as vsprintf.
 */
#if ENABLE_DEBUG
extern int midp_vsnprintf(char *buffer, int bufferSize, 
        const char* format, va_list argptr);
#else
#define midp_vsnprintf(buffer, bufferSize, format, argptr) \
    vsprintf(buffer, format, argptr)
#endif

#ifdef __cplusplus
}
#endif  
    
#endif /* MIDP_LIBC_EXT_H */

