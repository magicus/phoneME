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
 *
 */
/*
 * glue code for Windows NT
 */

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>

#include "javavm/include/winntUtil.h"

#ifdef _UNICODE

JAVAI_API TCHAR *createTCHAR(const char *s) {
    return createWCHAR(s);
}

JAVAI_API void freeTCHAR(TCHAR *p) {
    free(p);
}

/* converting UCS-2 to UTF-8 */
JAVAI_API
char *createMCHAR(const TCHAR *tcs)
{
    return convertWtoU8(NULL, tcs);
}

JAVAI_API void freeMCHAR(char *p) {
    free(p);
}

#endif

/* converting UTF-8 to UCS-2*/
JAVAI_API
WCHAR *createWCHAR(const char *string)
{
    int size;
    int err;
    LPWSTR ws_temp;
    /* calculating the required buffer size */
    size = MultiByteToWideChar(CP_UTF8, 0,
            string, -1,
            NULL, 0);
    if (size == 0) {
        return NULL;
    }
    ws_temp = malloc(size * sizeof(WCHAR));
    /* converting from local to wide string */
    size = MultiByteToWideChar(CP_UTF8, 0,
            string, -1,
            ws_temp, size);
    if (size == 0) {
        free(ws_temp);
        return NULL;
    }
    return ws_temp;
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
    /* converting from wide string to UTF-8 */
    nChars = WideCharToMultiByte(
            CP_UTF8, 0,
            tcs, -1,
            s, len, NULL, NULL);
    if (nChars == 0) {
        return len+1;
    }

    if (nChars < 0) {
        nChars = 0;
    }
#else
    strcpy(s, tcs);
#endif

    return 0;
}

JAVAI_API
TCHAR *createTCHARfromJString(JNIEnv *env, jstring jstr)
{
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

/* converting from wide string to UTF-8.
 * mallocates memory if first argument is NULL. Writes to the passed
 * address otherwise.
 */
JAVAI_API
char* convertWtoU8(char* astring, const WCHAR* u16string)
{
    int size;
    /* calculating the required buffer size */
    size = WideCharToMultiByte(
            CP_UTF8, 0,
            u16string, -1,
            NULL, 0, NULL, NULL);
    if (size == 0) {
        return NULL;
    }
    if (astring == NULL) {
        astring = malloc(size*sizeof(char));
    }
    /* converting from wide string to UTF-8 */
    size = WideCharToMultiByte(
            CP_UTF8, 0,
            u16string, -1,
            astring, size, NULL, NULL);
    if (size == 0) {
        free(astring);
        return NULL;
    }
    return astring;
}

/* converting from wide string to locale encoding.
 * mallocates memory if first argument is NULL. Writes to the passed
 * address otherwise.
 */
JAVAI_API
char* convertWtoA(char* astring, const WCHAR* u16string)
{
    int size;
    /* calculating the required buffer size */
    size = WideCharToMultiByte(
            CP_ACP, MB_PRECOMPOSED,
            u16string, -1,
            NULL, 0, NULL, NULL);
    if (size == 0) {
        return NULL;
    }
    if (astring == NULL) {
        astring = malloc(size*sizeof(char));
    }
    /* converting from wide string to UTF-8 */
    size = WideCharToMultiByte(
            CP_ACP, MB_PRECOMPOSED,
            u16string, -1,
            astring, size, NULL, NULL);
    if (size == 0) {
        free(astring);
        return NULL;
    }
    return astring;
}

/* converting ASCII string to UTF8 string through UTF-16.
 * mallocates memory for u8string
 */
JAVAI_API
char* createU8fromA(const char* astring)
{
    int size;
    LPWSTR ws_temp;
    char* u8string;
    /* calculating the required buffer size */
    size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
            astring, -1,
            NULL, 0);
    ws_temp = malloc(size * sizeof(WCHAR));
    /* converting from local to wide string */
    size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
            astring, -1,
            ws_temp, size);
    if (size == 0) {
        free(ws_temp);
        return NULL;
    }
    u8string = createMCHAR(ws_temp);
    free(ws_temp);
    return u8string;
}
