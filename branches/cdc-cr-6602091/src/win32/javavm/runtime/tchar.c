/*
 * @(#)tchar.c	1.4 06/10/10
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
 *
 */
/*
 * glue code for Windows NT
 */

#include <windows.h>

#include "javavm/include/winntUtil.h"

#ifdef _UNICODE

JAVAI_API TCHAR *createTCHAR(const char *s) {
    int len;
    TCHAR *tcs;
    int nChars;

    len = strlen(s);
    tcs = (TCHAR *) malloc(sizeof(TCHAR) * (len + 1));
    nChars = mbstowcs(tcs, s, len);
    if (nChars < 0) {
        nChars = 0;
    }
    tcs[nChars] = 0;
    return tcs;
}

JAVAI_API void freeTCHAR(TCHAR *p) {
    free(p);
}

JAVAI_API
char *createMCHAR(const TCHAR *tcs)
{
    int len = _tcslen(tcs);
    char *s;
    int nChars;

    if (len < 0)
        len = 0;

    s = (char *) malloc(len + 1);
    nChars = wcstombs(s, tcs, len);

    if (nChars < 0) {
        nChars = 0;
    }
    s[nChars] = 0;

    return s;
}

JAVAI_API void freeMCHAR(char *p) {
    free(p);
}

#endif

JAVAI_API
WCHAR *createWCHAR(const char *string)
{
    int len;
    WCHAR *wc;
    int nChars;

    len = strlen(string);
    wc = (WCHAR *)malloc(sizeof(WCHAR) * (len + 1));
    nChars = mbstowcs(wc, string, len);
    if (nChars < 0)
        nChars = 0;

    wc[nChars] = 0;
    return wc;
}

JAVAI_API
int copyToMCHAR(char *s, const TCHAR *tcs, int sLength)
{
    int len = _tcslen(tcs);
    int nChars;

    if (len < 0) {
        len = 0;
    }

    if (sLength < len + 1) {
        return len + 1;
    }

#ifdef _UNICODE
    nChars = wcstombs(s, tcs, len);

    if (nChars < 0) {
        nChars = 0;
    }
    s[nChars] = 0;
#else
    strcpy(s, tcs);
#endif

    return 0;
}

JAVAI_API
TCHAR *createTCHARfromJString(JNIEnv *env, jstring jstr) {
    int i;
    TCHAR *result;
    jint len = (*env)->GetStringLength(env, jstr);
    const jchar *str = (*env)->GetStringCritical(env, jstr, 0);
    result = (TCHAR *) malloc(sizeof(TCHAR) * (len + 1));

    if (result == NULL) {
        (*env)->ReleaseStringCritical(env, jstr, str);
	return NULL;
    }

    for (i = 0; i < len; i++) {
        result[i] = str[i];
    }
    result[len] = (TCHAR) 0;
    (*env)->ReleaseStringCritical(env, jstr, str);
    return result;
}


