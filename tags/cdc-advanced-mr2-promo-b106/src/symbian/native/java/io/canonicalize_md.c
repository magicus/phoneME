/*
 * @(#)canonicalize_md.c	1.3 06/10/10
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
 * Pathname canonicalization for Win32 file systems
 */
#ifndef WINCE
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <sys/stat.h>

#ifdef _WIN32_WINNT
#include "javavm/include/winntUtil.h"
#endif

#ifndef WINCE
	#include <errno.h>
#else
	#include "javavm/include/porting/ansi/errno.h"
#endif

#include <limits.h>

#define isfilesep(c) ((c) == '/' || (c) == '\\')
#define wisfilesep(c) ((c) == L'/' || (c) == L'\\')
#define islb(c)      (IsDBCSLeadByte((BYTE)(c)))

/* Note: The comments in this file use the terminology
         defined in the java.io.File class */

/* Copy bytes to dst, not going past dend; return dst + number of bytes copied,
   or NULL if dend would have been exceeded.  If first != '\0', copy that byte
   before copying bytes from src to send - 1. */

static char *
cp(char *dst, char *dend, char first, char *src, char *send)
{
    char *p = src, *q = dst;
    if (first != '\0') {
        if (q < dend) {
            *q++ = first;
        } else {
            errno = ENAMETOOLONG;
            return NULL;
        }
    }
    if (send - p > dend - q) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    while (p < send) {
        *q++ = *p++;
    }
    return q;
}

/* Wide character version of cp */

static WCHAR* 
wcp(WCHAR *dst, WCHAR *dend, WCHAR first, WCHAR *src, WCHAR *send)
{
    WCHAR *p = src, *q = dst;
    if (first != L'\0') {
        if (q < dend) {
            *q++ = first;
        } else {
            errno = ENAMETOOLONG;
            return NULL;
        }
    }
    if (send - p > dend - q) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    while (p < send)
        *q++ = *p++;
    return q;
}


/* Find first instance of '\\' at or following start.  Return the address of
   that byte or the address of the null terminator if '\\' is not found. */

static char *
nextsep(char *start)
{
    char *p = start;
    int c;
    while ((c = *p) && (c != '\\')) {
        p += ((islb(c) && p[1]) ? 2 : 1);
    }
    return p;
}

/* Wide character version of nextsep */

static WCHAR *
wnextsep(WCHAR *start)
{
    WCHAR *p = start;
    int c;
    while ((c = *p) && (c != L'\\'))
        p++;
    return p;
}

/* Tell whether the given string contains any wildcard characters */

static int
wild(char *start)
{
    char *p = start;
    int c;
    while (c = *p) {
        if ((c == '*') || (c == '?')) return 1;
        p += ((islb(c) && p[1]) ? 2 : 1);
    }
    return 0;
}

/* Wide character version of wild */

static int
wwild(WCHAR *start)
{
    WCHAR *p = start;
    int c;
    while (c = *p) {
        if ((c == L'*') || (c == L'?')) 
            return 1;
        p++;
    }
    return 0;
}

/* Tell whether the given string contains prohibited combinations of dots.
   In the canonicalized form no path element may have dots at its end.
   Allowed canonical paths: c:\xa...dksd\..ksa\.lk    c:\...a\.b\cd..x.x
   Prohibited canonical paths: c:\..\x  c:\x.\d c:\...
*/
static int 
dots(char *start)
{
    char *p = start;
    while (*p) {
        if ((p = strchr(p, '.')) == NULL) // find next occurence of '.'
            return 0; // no more dots
        p++; // next char
        while ((*p) == '.') // go to the end of dots
            p++;
        if (*p && (*p != '\\')) // path element does not end with a dot
            p++; // go to the next char
        else 
            return 1; // path element does end with a dot - prohibited
    }
    return 0; // no prohibited combinations of dots found
}

/* Wide character version of dots */
static int 
wdots(WCHAR *start)
{
    WCHAR *p = start;
    while (*p) {
        if ((p = wcschr(p, L'.')) == NULL) // find next occurence of '.'
            return 0; // no more dots
        p++; // next char
        while ((*p) == L'.') // go to the end of dots
            p++;
        if (*p && (*p != L'\\')) // path element does not end with a dot
            p++; // go to the next char
        else 
            return 1; // path element does end with a dot - prohibited
    }
    return 0; // no prohibited combinations of dots found
}

/* Convert a pathname to canonical form.  The input orig_path is assumed to
   have been converted to native form already, via JVM_NativePath().  This is
   necessary because _fullpath() rejects duplicate separator characters on
   Win95, though it accepts them on NT. */

int 
canonicalize(char *orig_path, char *result, int size)
{
    WCHAR *wPath;
    WCHAR wPathResult[1024];
    char *pathResult;
    int iResult;

    wPath = createWCHAR(orig_path);
    iResult = wcanonicalize(wPath, wPathResult, 1024);
    free(wPath);
    if (iResult) {
        return iResult;
    }
    pathResult = createMCHAR(wPathResult);
    if (strlen(pathResult) >= size) {
        free(pathResult);
        errno = ENAMETOOLONG;
        return -1;
    }
    strncpy(result, pathResult, size);
    free(pathResult);
    return 0;
}
    
/* Wide character version of canonicalize. Size is a wide-character size. */

int
wcanonicalize(WCHAR *orig_path, WCHAR *result, int size)
{
    WIN32_FIND_DATAW fd;
    HANDLE h;
    WCHAR path[1024];    /* Working copy of path */
    WCHAR *src, *dst, *dend, c;

    /* Reject paths that contain wildcards */
    if (wwild(orig_path)) {
        errno = EINVAL;
        return -1;
    }

    /* Collapse instances of "foo\.." and ensure absoluteness.  Note that
       contrary to the documentation, the _fullpath procedure does not require
       the drive to be available.  */
    if(!_wfullpath(path, orig_path, sizeof(path)/2)) {
        return -1;
    }

    if (wdots(path)) /* Check for phohibited combinations of dots */
        return -1;

    src = path;            /* Start scanning here */
    dst = result;        /* Place results here */
    dend = dst + size;        /* Don't go to or past here */

    /* Copy prefix, assuming path is absolute */
    c = src[0];
    if (((c <= L'z' && c >= L'a') || (c <= L'Z' && c >= L'A'))
       && (src[1] == L':') && (src[2] == L'\\')) {
        /* Drive specifier */
        *src = towupper(*src);    /* Canonicalize drive letter */
        if (!(dst = wcp(dst, dend, L'\0', src, src + 2))) {
            return -1;
        }

        src += 2;
    } else if ((src[0] == L'\\') && (src[1] == L'\\')) {
        /* UNC pathname */
        WCHAR *p;
        p = wnextsep(src + 2);    /* Skip past host name */
        if (!*p) {
            /* A UNC pathname must begin with "\\\\host\\share",
               so reject this path as invalid if there is no share name */
            errno = EINVAL;
            return -1;
        }
        p = wnextsep(p + 1);    /* Skip past share name */
        if (!(dst = wcp(dst, dend, L'\0', src, p)))
            return -1;
        src = p;
    } else {
        /* Invalid path */
        errno = EINVAL;
        return -1;
    }
    /* At this point we have copied either a drive specifier ("z:") or a UNC
       prefix ("\\\\host\\share") to the result buffer, and src points to the
       first byte of the remainder of the path.  We now scan through the rest
       of the path, looking up each prefix in order to find the true name of
       the last element of each prefix, thereby computing the full true name of
       the original path. */
    while (*src) {
        WCHAR *p = wnextsep(src + 1);    /* Find next separator */
        WCHAR c = *p;
        assert(*src == L'\\');        /* Invariant */
        *p = L'\0';            /* Temporarily clear separator */
        h = FindFirstFileW(path, &fd);    /* Look up prefix */
        *p = c;                /* Restore separator */
        if (h != INVALID_HANDLE_VALUE) {
            /* Lookup succeeded; append true name to result and continue */
            FindClose(h);
            if (!(dst = wcp(dst, dend, L'\\', fd.cFileName, 
                           fd.cFileName + wcslen(fd.cFileName))))
                return -1;
        src = p;
        continue;
    } else {
        /* If the lookup of a particular prefix fails because the file does
           not exist, because it is of the wrong type, or because access is
           denied, then we copy the remainder of the input path as-is.
           Other I/O errors cause an error return. */
        DWORD errval = GetLastError();
        if ((errval == ERROR_FILE_NOT_FOUND)
            || (errval == ERROR_DIRECTORY)
            || (errval == ERROR_PATH_NOT_FOUND)
            || (errval == ERROR_BAD_NETPATH)
            || (errval == ERROR_BAD_NET_NAME)
            || (errval == ERROR_ACCESS_DENIED)
            || (errval == ERROR_NETWORK_ACCESS_DENIED)) {
            if (!(dst = wcp(dst, dend, L'\0', src, src + wcslen(src))))
                return -1;
            break;
        } else {
#ifdef DEBUG_PATH
            jio_fprintf(stderr, "canonicalize: errval %d\n", errval);
#endif DEBUG_PATH
            return -1;
        }
    }
    }

    if (dst >= dend) {
    errno = ENAMETOOLONG;
    return -1;
    }
    *dst = L'\0';
    return 0;

}
