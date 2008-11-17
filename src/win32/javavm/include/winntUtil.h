/*
 * @(#)winntUtil.h	1.4 06/10/10
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

#ifndef WINNT_JAVAI_H
#define WINNT_JAVAI_H

#define JAVAI_API _declspec(dllexport)
#include <tchar.h>
#include "jni.h"

#ifdef _UNICODE
JAVAI_API TCHAR *createTCHAR(const char *);
JAVAI_API void freeTCHAR(TCHAR *);
JAVAI_API char  *createMCHAR(const TCHAR *);
JAVAI_API void freeMCHAR(char *);
#else
#define createTCHAR(s) (s)
#define freeTCHAR(s) ((void) (s))
#define createMCHAR(s) (s)
#define freeMCHAR(s) ((void) (s))
#endif

JAVAI_API WCHAR *createWCHAR(const char *);
JAVAI_API TCHAR *createTCHARfromJString(JNIEnv *env, jstring jstr);
JAVAI_API int copyToMCHAR(char *s, const TCHAR *tcs, int sLength);
JAVAI_API char* createU8fromA(const char* astring);
JAVAI_API char* convertWtoU8(char* astring, const WCHAR* u16string);
JAVAI_API char* convertWtoA(char* astring, const WCHAR* u16string);

#endif /* WINNT_JAVAI_H */
