/*
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


int javautil_str_wcslen(const javacall_utf16* str);
int javautil_str_wcscmp(const javacall_utf16* str1, const javacall_utf16* str2);
int javautil_str_wcsncmp(const javacall_utf16* str1, const javacall_utf16* str2, int size);
int javautil_str_wcsicmp(const javacall_utf16* str1, const javacall_utf16* str2);
int javautil_str_wcsincmp(const javacall_utf16* str1, const javacall_utf16* str2, int size);

const javacall_utf16* javautil_str_wcsstr(const javacall_utf16* str, const javacall_utf16* pattern);
const javacall_utf16* javautil_str_wcsstri(const javacall_utf16* str, const javacall_utf16* pattern);
const javacall_utf16* javautil_str_wcsrstr(const javacall_utf16* str, const javacall_utf16* pattern);
const javacall_utf16* javautil_str_wcsrstri(const javacall_utf16* str, const javacall_utf16* pattern);

const javacall_utf16* javautil_str_wcschr(const javacall_utf16* str, javacall_utf16 wch);
const javacall_utf16* javautil_str_wcschri(const javacall_utf16* str, javacall_utf16 wch);
const javacall_utf16* javautil_str_wcsrchr(const javacall_utf16* str, javacall_utf16 wch);
const javacall_utf16* javautil_str_wcsrchri(const javacall_utf16* str, javacall_utf16 wch);

javacall_utf16* javautil_str_wcstolow(javacall_utf16* str);

int javautil_str_strlen(const char* str1);
int javautil_str_strcmp(const char* str1, const char* str2);
int javautil_str_strncmp(const char* str1, const char* str2, int size);
int javautil_str_stricmp(const char* str1, const char* str2);
int javautil_str_strincmp(const char* str1, const char*, int size);
char* javautil_str_strtolow(char* str);

#ifdef __cplusplus
}
#endif/*__cplusplus*/

#endif //__JAVAUTIL_STR_H___
