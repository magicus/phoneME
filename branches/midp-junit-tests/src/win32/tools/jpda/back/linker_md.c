/*
 * @(#)linker_md.c	1.13 06/10/10
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
 * Maintains a list of currently loaded DLLs (Dynamic Link Libraries)
 * and their associated handles. Library names are case-insensitive.
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef WINCE
#include <stdlib.h>
#endif

#include "sys.h"

#include "path_md.h"
#ifdef _WIN32_WINNT
#include "javavm/include/winntUtil.h"
#endif

/*
 * From system_md.c v1.54
 */
int
dbgsysGetLastErrorString(char *buf, int len)
{
    long errval;

    if ((errval = GetLastError()) != 0) {
	/* DOS error */
        TCHAR *tbuf = createTCHAR(buf);
	int n = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
			      NULL, errval,
			      0, tbuf, len, NULL);
        freeTCHAR(tbuf);        
	if (n > 3) {
	    /* Drop final '.', CR, LF */
	    if (buf[n - 1] == '\n') n--;
	    if (buf[n - 1] == '\r') n--;
	    if (buf[n - 1] == '.') n--;
	    buf[n] = '\0';
	}
	return n;
    }

#ifndef WINCE
    if (errno != 0) {
	/* C runtime error that has no corresponding DOS error code */
	const char *s = strerror(errno);
	int n = strlen(s);
	if (n >= len) n = len - 1;
	strncpy(buf, s, n);
	buf[n] = '\0';
	return n;
    }
#endif

    return 0;
}

/* 
 * Rest Adapted from JDK1.2 linker_md.c	1.43 98/09/28
 */
 
/*
 * create a string for the JNI native function name by adding the
 * appropriate decorations.
 *
 * On Win32, "__stdcall" functions are exported differently, depending
 * on the compiler. In MSVC 4.0, they are decorated with a "_" in the 
 * beginning, and @nnn in the end, where nnn is the number of bytes in 
 * the arguments (in decimal). Borland C++ exports undecorated names.
 *
 * dbgsysBuildFunName handles different encodings depending on the value 
 * of encodingIndex. It returns 0 when handed an out-of-range
 * encodingIndex.
 */
int
dbgsysBuildFunName(char *name, int nameMax, int args_size, int encodingIndex)
{
  if (encodingIndex == 0) {
    /* For Microsoft MSVC 4.0 */
    char suffix[6];    /* This is enough since Java never has more than 
			   256 words of arguments. */
    int nameLen;
    int i;

    sprintf(suffix, "@%d", args_size * 4);
    
    nameLen = strlen(name);
    if (nameLen >= nameMax - 7)
        return 1;
    for(i = nameLen; i > 0; i--)
        name[i] = name[i-1];
    name[0] = '_';
    
    sprintf(name + nameLen + 1, "%s", suffix);
    return 1;
  } else if (encodingIndex == 1)
    /* For Borland, etc. */
    return 1;
  else
    return 0;
}

/*
 * Build a machine dependent library name out of a path and file name.
 */
void
dbgsysBuildLibName(char *holder, int holderlen, char *pname, char *fname)
{
    const int pnamelen = pname ? strlen(pname) : 0;
    const char c = (pnamelen > 0) ? pname[pnamelen-1] : 0;
    char *suffix;

#ifdef DEBUG   
    suffix = "_g";
#else
    suffix = "";
#endif 

    /* Quietly truncates on buffer overflow. Should be an error. */
    if (pnamelen + strlen(fname) + 10 > (unsigned int)holderlen) {
        *holder = '\0';
        return;
    }

    if (pnamelen == 0) {
        sprintf(holder, "lib%s%s.dll", fname, suffix);
    } else if (c == ':' || c == '\\') {
        sprintf(holder, "lib%s%s%s.dll", pname, fname, suffix);
    } else {
        sprintf(holder, "%s\\lib%s%s.dll", pname, fname, suffix);
    }
}

void *
dbgsysLoadLibrary(const char * name, char *err_buf, int err_buflen)
{
#ifdef UNICODE
    void *result;
    size_t len0 = strlen(name);
    size_t len = mbstowcs(NULL, name, len0);
    size_t len1;
    wchar_t *wname = (wchar_t *)calloc(sizeof wname[0], len + 1);
    if (wname == NULL) {
	strncpy(err_buf, "Out of memory", err_buflen-2);
	err_buf[err_buflen-1] = '\0';
	return NULL;
    }
    len1 = mbstowcs(wname, name, len0 + 1);
    wname[len] = L'\0';
    result = LoadLibrary(wname);
    free(wname);
#else
    void *result = LoadLibrary(name);
#endif
    if (result == NULL) {
	/* Error message is pretty lame, try to make a better guess. */
	long errcode = GetLastError();
	if (errcode == ERROR_MOD_NOT_FOUND) {
	    strncpy(err_buf, "Can't find dependent libraries", err_buflen-2);
	    err_buf[err_buflen-1] = '\0';
	} else {
	    dbgsysGetLastErrorString(err_buf, err_buflen);
	}
    }
    return result;
}

void dbgsysUnloadLibrary(void *handle)
{
    FreeLibrary(handle);
}

void * dbgsysFindLibraryEntry(void *handle, const char *name)
{
#ifdef WINCE
    void *result;
    size_t len0 = strlen(name);
    size_t len = mbstowcs(NULL, name, len0);
    size_t len1;
    wchar_t *wname = (wchar_t *)calloc(sizeof wname[0], len + 1);
    if (wname == NULL) {
	return NULL;
    }
    len1 = mbstowcs(wname, name, len0 + 1);
    wname[len] = L'\0';
    result = GetProcAddress(handle, wname);
    free(wname);
    return result;
#else
    return GetProcAddress(handle, name);
#endif
}
