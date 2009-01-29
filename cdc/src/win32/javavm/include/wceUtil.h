/*
 * @(#)wceUtil.h	1.9 06/10/10
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
/*
 * @(#)wceUtil.h	1.5 03/10/01
 */

#ifndef WCE_JAVAI_H
#define WCE_JAVAI_H

#ifdef WINCE

#include <windows.h>
#include <string.h>
#include <stddef.h>

#if 0

#ifdef __cplusplus
#define __LANGPREFIX extern "C"
#else
#define __LANGPREFIX
#endif /* __cplusplus */

#ifdef INSIDE_JAVAI
#define JAVAI_API __LANGPREFIX _declspec(dllexport)
#else
#define JAVAI_API __LANGPREFIX _declspec(dllimport)
#endif /* INSIDE_JAVAI */

#endif

#define JAVAI_API _declspec(dllexport)

JAVAI_API wchar_t *createWCHAR(const char *);
JAVAI_API char  *createMCHAR(const wchar_t *);
JAVAI_API char  *strerror(int err);
JAVAI_API char  *getenv(const char* name);

/* Remove "." and ".." from the path */
BOOL
WINCEpathRemoveDots(wchar_t *dst0, const wchar_t *src, size_t maxLength);

#endif /* WINCE */

#endif /* WCE_JAVAI_H */
