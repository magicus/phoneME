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

/**
 * @file javacall_invoke.h
 * @ingroup CHAPI
 * @brief Content handlers executor interface for JSR-211 CHAPI
 */


/**
 * @defgroup CHAPI JSR-211 Content Handler API (CHAPI)
 *
 *  The following API definitions are required by JSR-211.
 *  These APIs are not required by standard JTWI implementations.
 *
 * @{
 */

#ifndef __JAVAUTIL_STR_H___
#define __JAVAUTIL_STR_H___

#include <javacall_defs.h>

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/


int javautil_wcslen(javacall_const_utf16_string str);
int javautil_wcscmp(javacall_const_utf16_string str1, javacall_const_utf16_string str2);
int javautil_wcsncmp(javacall_const_utf16_string str1, javacall_const_utf16_string str2, int size);
int javautil_wcsicmp(javacall_const_utf16_string str1, javacall_const_utf16_string str2);
int javautil_wcsincmp(javacall_const_utf16_string str1, javacall_const_utf16_string str2, int size);

int javautil_strlen(const char* str1);
int javautil_strcmp(const char* str1, const char* str2);
int javautil_strncmp(const char* str1, const char* str2, int size);
int javautil_stricmp(const char* str1, const char* str2);
int javautil_strincmp(const char* str1, const char*, int size);


#ifdef __cplusplus
}
#endif/*__cplusplus*/

#endif //__JAVAUTIL_STR_H___
