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

#include <stdio.h>
#include <stdarg.h>

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
#endif
