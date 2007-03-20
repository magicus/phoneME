/*
 * @(#)wceUtil.c	1.13 06/10/10
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
 * glue code for WinCE
 */

#include "javavm/include/defs.h"
#include "javavm/include/porting/system.h"
#include "javavm/include/porting/ansi/time.h"
#include "javavm/include/porting/ansi/string.h"

#include <winbase.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <winreg.h>

#include "javavm/include/wceUtil.h"

JAVAI_API
WCHAR *createWCHAR(const char *string);

#if _WIN32_WCE < 300
int stricmp(const char* s1, const char* s2) {
    while ((*s1 != '\0') && (tolower(*s1) == tolower(*s2))) {
	s1++;
	s2++;
    }
    return (int)(*s1 - *s2);
}
#endif

JAVAI_API BOOL GetUserNameA(LPTSTR lpBuff, LPDWORD lpL) {
    lpBuff[0] = 0;
    *lpL = 1;
    return (1);
}


/* these corrupt non-ascii strings */
#define ASCII2UNICODE(astr, ustr, max) \
    MultiByteToWideChar(CP_ACP, 0, (astr), -1, (ustr), (max))

#define UNICODE2ASCII(ustr, astr, max) \
    WideCharToMultiByte(CP_ACP, 0, (ustr), -1, (astr), (max), NULL, NULL)


JAVAI_API
char *getenv(const char* name) {
    #define WINREG_MAX_NUM             10 /* temporary restriction */
    #define WINREG_HKEY_NAME_FOR_PJAVA TEXT("Software\\Sun\\PersonalJava\\")
    #define WINREG_MAX_NAME_LEN        64
    #define WINREG_MAX_VAL_LEN         1024
    #define PJAVA_SUBKEY               TEXT("default")

    static HKEY hKey = NULL;
    static int numVal = 0;
    static char* nameTable[WINREG_MAX_NUM];
    static char* valTable[WINREG_MAX_NUM];

    if (hKey == NULL) {
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			 WINREG_HKEY_NAME_FOR_PJAVA /*+*/ PJAVA_SUBKEY,
			 0, 0, &hKey) 
	    != ERROR_SUCCESS) {
	    ASSERT(hKey == NULL);
	    return NULL;
	}
    } else {
	int i;
	for (i = 0; i < numVal; i++) {
	    if (strcmp(name, nameTable[i]) == 0) {
		return valTable[i];
	    }
	}
    }
    if (numVal >= WINREG_MAX_NUM) {
	return NULL;
    } else {
	WCHAR wName[WINREG_MAX_NAME_LEN];
	WCHAR wVal[WINREG_MAX_VAL_LEN];
	char  aVal[WINREG_MAX_VAL_LEN];
	DWORD dwSize = sizeof(wVal); /* size in bytes */
	DWORD dwType;

	ASCII2UNICODE(name, wName, WINREG_MAX_NAME_LEN);
	if (RegQueryValueEx(hKey, wName, NULL, &dwType, (BYTE*)wVal, &dwSize)
	    != ERROR_SUCCESS 
	    || dwType != REG_SZ) { /* null-terminated UNICODE string */
	    return NULL;
	}
	UNICODE2ASCII(wVal, aVal, WINREG_MAX_VAL_LEN);
	if ((nameTable[numVal] = strdup(name)) == NULL) { /* out of memory*/
	    return NULL;
	}
	if ((valTable[numVal] = strdup(aVal)) == NULL) { /* out of memory*/
	    free(nameTable[numVal]);
	    return NULL;
	}
    }
    return valTable[numVal++];
}


JAVAI_API time_t time(time_t *tloc) {
    CVMsystemPanic("time: not supported");
    return 0;
}

JAVAI_API char *ctime(const time_t *clock) {
    CVMsystemPanic("ctime: not supported");
    return NULL;
}

JAVAI_API time_t mktime(struct tm *timeptr) {
{
	if (0 >= (int) (timeptr->tm_mon -= 2)) {	/* 1..12 -> 11,12,1..10 */
		timeptr->tm_mon += 12;	/* Puts Feb last since it has leap day */
		timeptr->tm_year -= 1;
	}
	return (((
	    (unsigned long)(timeptr->tm_year/4 - 
			timeptr->tm_year/100 + 
			timeptr-> tm_year/400 + 
			367* timeptr->tm_mon/12 + 
			timeptr->tm_mday) +
	      timeptr->tm_year*365 - 719499
	    )*24 + timeptr->tm_hour /* now have hours */
	   )*60 + timeptr->tm_min /* now have minutes */
	  )*60 + timeptr->tm_sec; /* finally seconds */
	}
}

#if 0
/* CreateThread() does not initialize C runtime library */
JAVAI_API
unsigned long _beginthreadex(void* security, unsigned stack_size, 
			     unsigned(__stdcall* start_address)(void*),
			     void* arglist, unsigned initflag,
			     unsigned* thrdaddr) {

    return (unsigned long)CreateThread(security, stack_size,
				       (LPTHREAD_START_ROUTINE)start_address,
				       arglist, initflag, thrdaddr);
}

JAVAI_API void _endthreadex(unsigned retval) {
    ExitThread(retval);
}
#endif

JAVAI_API char* strerror(int err) {
    static char* emsg[] = {
	"No such file or directory",
	"Empty File",
	"Bad Value",
	"Bad file number",
	"Not enough memory",
	"Permission denied",
	"Not a directory",
	"Unknown error",
    };
    static int eid[] = {
	ENOENT,
	EMFILE,
	EINVAL,
	EBADF,
	ENOMEM,
	EACCES,
	ENOTDIR,
	0,
    };
    int i;

    for (i = 0; eid[i] != err && eid[i] != 0; i++);

    return emsg[i];
}

/*
 * missing functions 
 */

int
_wrename(const wchar_t* oldPath, const wchar_t* newPath) 
{
    if (MoveFile(oldPath, newPath) == 0) {
	return -1;
    }
    return 0;
}

/*
int mkdir(const char* path) {

    return (CreateDirectory(path, NULL))?(0):(-1);
}

int _rmdir(const char* path) {

    return (RemoveDirectory(path))?(0):(-1);
}
*/

static char *setCwd=NULL; /* stored in ascii format */

JAVAI_API BOOL
SetCurrentDirectory(char *str)
{
    int len = strlen(str);

    
    if (len > MAX_PATH) {
	return 0;
    }

    if (setCwd) { /* already set, free old string */
	free(setCwd);
    }
    setCwd = (char *) malloc(len+1);
    if (!setCwd) { /* out of memory */
	return 0;
    }
    strcpy(setCwd, str);
    return 1;
}

JAVAI_API DWORD GetCurrentDirectory(DWORD nBufferLength, char *lpBuffer) {
    const char* cwd;
    char buf[MAX_PATH];

    if (setCwd) {
	strncpy(lpBuffer, setCwd, nBufferLength-1);
	lpBuffer[nBufferLength - 1] = '\0';
	return strlen(lpBuffer);
    }
    if ((cwd = getenv("PWD")) == NULL || cwd[0] == '\0') {
	/*Current Working Directory will be set to ROOT*/
	buf[0] = '\\';
	buf[1] = '\0';
	cwd = (buf[0] == '\0')?("\\."):(buf);
    }
    ASSERT(nBufferLength >= 1);
    strncpy(lpBuffer, cwd, nBufferLength - 1);
    /* strncpy'ed result may not be null-terminated */
    lpBuffer[nBufferLength - 1] = '\0';

    return strlen(lpBuffer);
}


_CRTIMP void * __cdecl calloc(size_t num, size_t size) {
    size_t len = num * size;
    void* p = malloc(len);
    if (p != NULL) {
	memset(p, 0, len);
    }
    return p;
}

#include "javavm/include/ansi/unistd.h"

int _waccess(const wchar_t* path, int amode)
{
    DWORD attr;
    WCHAR *wc; 

    attr = GetFileAttributes(path);

    if (attr == 0xFFFFFFFF) {
	/* file not found -- fails F_OK, R_OK, W_OK, X_OK */
	errno = ENOENT;
	return -1;
    }
    if ((amode & W_OK) && (attr & FILE_ATTRIBUTE_READONLY)) {
	/* no write permission */
	return -1;
    }
    return 0;
}

wchar_t *
_wfullpath(wchar_t *absPath, const wchar_t *relPath, size_t maxLength)
{
    if (wcslen(relPath) >= maxLength) {
	return NULL;
    }
    wcscpy(absPath, relPath);
    return absPath;
}

#if _WIN32_WCE < 300
JAVAI_API char *strcatA( char *string1, const char *string2 )
{
	int i = 0;
	int j = 0;
	while (string1[i] != '\0') i++;
	while (string2[j] != '\0') {
		string1[i++] = string2[j++];
	}
	string1[i] = '\0';
	return string1;
}

JAVAI_API char *strchrA( const char *string, int c )
{
	int i = 0;
	while (string[i] != '\0' && string[i] != c) i++;
	if (string[i] == '\0')
		return NULL;
	else
		return &string[i];
}

JAVAI_API int strncmpA( const char *string1, const char *string2, size_t count ) 
{
	int i = 0;

	if (string1 == NULL) return -1;
	if (string2 == NULL) return 1;

	while ((string1[i] != '\0') && (string2[i] != '\0') && (i < count) ) {
		if (string1[i] != string2[i]) {
			return  (string1[i] - string2[i]);
		}
		i++;
	}
	if (i == count)
		return 0;
	return  (string1[i] - string2[i]);
}

JAVAI_API int strcmpA( const char *string1, const char *string2 ) 
{
	int i = 0;

	if (string1 == NULL) return -1;
	if (string2 == NULL) return 1;

	while ((string1[i] != '\0') && (string2[i] != '\0')) {
		if (string1[i] != string2[i]) {
			return  (string1[i] - string2[i]);
		}
		i++;
	}
	return  (string1[i] - string2[i]);
}

JAVAI_API int strlenA(char *str)
{
	int i = 0;
	while (str[i] != '\0' ) i++;
	return i;
}


JAVAI_API char *strncpyA( char *string1, const char *string2, size_t count ) 
{
	memset(string1, 0, count);
	memcpy(string1, string2, count);
	return string1;
}

JAVAI_API char *strcpyA( char *string1, const char *string2 ) 
{
	int len = strlen(string2) + 1;
	memcpy(string1, string2, len);
	return string1;
}

JAVAI_API char *strdupA( const char *string ) 
{
	int len = strlen(string) + 1;
	char *s = (char *)malloc(len);
	if (s == NULL)
		return NULL;
	memcpy(s, string, len);
	return s;
}

JAVAI_API char* strrchrA(const char* s, int c) {
    const char* r = NULL;

    while (*s != '\0') {
	if (*s == c) 
	    r = s;
	s++;
    }
    return (char*)r;
}

JAVAI_API char *strstrA( const char *string1, const char *string2 )
{
	int i = 0,j = 0, k;
	int c = string2[0];

	if (string2 == NULL || string2[0] == '\0')
		return string1;

	while (string1[i] != '\0') {
		if (string1[i++] == c) { 
			k = i; j = 1;
			while (string1[k] == string2[j] && string2[j] != '\0') {
				k++; j++;
			}
			if (string2[j] =='\0')
				return &string1[i-1];
		}
	}
	return NULL;
}


JAVAI_API long strtolA( const char *nptr, char **endptr, int base )
{
    char *s = nptr;
    long n = 0;
    while ((*s != '\0') && ('0' <= *s) && (*s <= '9'))  {
        n = n * base;
		n += (*s - '0');
		s++;
    }
	*endptr = s;
    return n;
}
#endif

void
abort(void)
{
    TerminateProcess(GetCurrentProcess(), -1);
}
