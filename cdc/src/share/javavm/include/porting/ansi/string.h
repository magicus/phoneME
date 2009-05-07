/*
 * @(#)string.h	1.12 06/10/10
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
 *
 */

#ifndef _INCLUDED_PORTING_ANSI_STRING_H
#define _INCLUDED_PORTING_ANSI_STRING_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/ansi/stddef.h"

/*
 * Porting layer for C library sorts of things
 */

/*
 * C library <string.h> routines
 *
 * Functions:
 *
 * void*    memset(void* s, int c, size_t n);
 * void*    memcpy(void* d, const void* s, size_t n);
 * int      memcmp(const void *s1, const void *s2, size_t n);
 * void*    memmove(void* d, const void* s, size_t n);
 * int	    strcmp(const char* s1, const char* s2);
 * int	    strncmp(const char* s1, const char* s2, size_t n);
 * size_t   strlen(const char* s1);
 * char*    strcpy(char* dst, const char* src);
 * char*    strncpy(char* dst, const char* src, size_t n);
 * char*    strdup(const char* str);
 * char*    strcat(char* dst, const char* src);
 * char*    strncat(char* dst, const char* src, size_t n);
 * char*    strstr(const char* s1, const char* s2);
 * char*    strchr(const char* s, int c);
 * char*    strrchr(const char* s, int c);
 *
 */

#ifdef CVM_HDR_ANSI_STRING_H
#include CVM_HDR_ANSI_STRING_H
#else
#include <string.h>
#endif

#endif /* _INCLUDED_PORTING_ANSI_STRING_H */
