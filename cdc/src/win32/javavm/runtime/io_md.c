/*
 * @(#)io_md.c	1.23 06/10/10
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
 */

#include "javavm/include/globals.h"
#include "javavm/include/porting/io.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/wceUtil.h"
#include "javavm/include/io_sockets.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define isfilesep(c) ((c) == '/' || (c) == '\\')
#define islb(c)      (IsDBCSLeadByte((BYTE)(c)))

static int MAX_INPUT_EVENTS = 2000;

#ifdef WINCE
/* sometimes, HANDLE which is the return value of CreateFile become minus */
#define NUM_FDTABLE_ENTRIES 255
static HANDLE fdTable[NUM_FDTABLE_ENTRIES];
static CRITICAL_SECTION fdTableLock;
#if 0
#define STD_FDS_BIAS 3
#else
#define STD_FDS_BIAS 0
#endif
static HANDLE
WIN_GET_HANDLE(HANDLE fdd)
{
    return (fdTable[(int)fdd - STD_FDS_BIAS]);
}
#else
#define WIN_GET_HANDLE(fdd)  (fdd)
#endif

void WIN32ioInit()
{
#ifdef WINCE
    int i;

    InitializeCriticalSection(&fdTableLock);

    for (i = 0; i < NUM_FDTABLE_ENTRIES; i++) {
	fdTable[i] = INVALID_HANDLE_VALUE; 
    }

    /* initialize stdio redirection */ 
    initializeStandardIO();
#endif
}

static int
fileMode(HANDLE fd, int *mode)
{
    int ret;
    BY_HANDLE_FILE_INFORMATION info;
    ret = GetFileInformationByHandle(fd, &info);
    if ( ret ) {
        *mode = info.dwFileAttributes;
        return 0;
    } else {
        *mode = 0;
        return -1;
    }
}

CVMInt32
CVMioGetLastErrorString(char *buf, CVMInt32 len)
{
    DWORD err = GetLastError();
    if (err == 0) {
	return 0;
    } else {
	TCHAR* lpMsgBuf = 0;
	int n;
	int ret = FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				err,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
 
	if ((lpMsgBuf != NULL) && (ret != 0)) {
	    if (sizeof lpMsgBuf[0] == 1) {
		n = _tcslen(lpMsgBuf);
		if (n >= len) n = len - 1;
		strncpy(buf, (char*)lpMsgBuf, n);
		buf[n] = '\0';
	    } else {
		wcstombs(buf, lpMsgBuf, len);
	    }
	    LocalFree( lpMsgBuf );
	}
	return err;
    }
}

char *
CVMioReturnLastErrorString(int errNo)
{
    LPVOID lpMsgBuf;
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errNo,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
        );
    return lpMsgBuf;
}

char *
CVMioNativePath(char *path)
{
    char *src = path, *dst = path;
    char *colon = NULL;		/* If a drive specifier is found, this will
				   point to the colon following the drive
				   letter */
    /* Check for leading separators */
    while (isfilesep(*src)) src++;
#ifndef WINCE
    if (isalpha(*src) && !islb(*src) && src[1] == ':') {
	/* Remove leading separators if followed by drive specifier.
	   This is necessary to support file URLs containing drive
	   specifiers (e.g., "file://c:/path").  As a side effect,
	   "/c:/path" can be used as an alternative to "c:/path". */
	*dst++ = *src++;
	colon = dst;
	*dst++ = ':'; src++;
    } else
#endif
    {
	src = path;
	if (isfilesep(src[0]) && isfilesep(src[1])) {
	    /* UNC pathname: Retain first separator; leave src pointed at
	       second separator so that further separators will be collapsed
	       into the second separator.  The result will be a pathname
	       beginning with "\\\\" followed (most likely) by a host name. */
	    src = dst = path + 1;
	    path[0] = '\\';	/* Force first separator to '\\' */
	}
    }

    /* Remove redundant separators from remainder of path, forcing all
       separators to be '\\' rather than '/' */
    while (*src != '\0') {
	if (isfilesep(*src)) {
	    *dst++ = '\\'; src++;
	    while (isfilesep(*src)) src++;
	    if (*src == '\0') {	/* Check for trailing separator */
#ifndef WINCE
		if (colon == dst - 2) break;                      /* "z:\\" */
#endif
		if (dst == path + 1) break;                       /* "\\" */

		if (dst == path + 2 && isfilesep(path[0])) {
		    /* "\\\\" is not collapsed to "\\" because "\\\\" marks the
		       beginning of a UNC pathname.  Even though it is not, by
		       itself, a valid UNC pathname, we leave it as is in order
		       to be consistent with sysCanonicalPath() (below) as well
		       as the win32 APIs, which treat this case as an invalid
		       UNC pathname rather than as an alias for the root
		       directory of the current drive. */
		    break;
		}

		dst--;		/* Path does not denote a root directory, so
				   remove trailing separator */
		break;
	    }
	} else {
	    if (islb(*src)) {	/* Copy a double-byte character */
		*dst++ = *src++;
		if (*src) {
		    *dst++ = *src++;
		}
	    } else {		/* Copy a single-byte character */
		*dst++ = *src++;
	    }
	}
    }

    *dst = '\0';
    return path;
}

#ifdef WINCE
#include "javavm/include/porting/path.h"
#include "javavm/include/porting/ansi/errno.h"

int
CVMcanonicalize(const char* path, char* out, int len)
{
    if (strlen(path) >= len) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(out, path);
    return 0;
}
#endif

CVMInt32
CVMioFileType(const char *path)
{
    DWORD attr;

#ifdef WINCE
    {
	WCHAR *wc = createWCHAR(path);
	attr = GetFileAttributes(wc); 
	free(wc); 
    }
#else
    attr = GetFileAttributesA(path);
#endif

    if (attr == 0xFFFFFFFF) {
	return -1;
    }
    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
	return CVM_IO_FILETYPE_DIRECTORY;
    }
    if ((attr == FILE_ATTRIBUTE_NORMAL) || (attr & FILE_ATTRIBUTE_ARCHIVE)
	|| attr & FILE_ATTRIBUTE_READONLY) {
	return CVM_IO_FILETYPE_REGULAR;
    }
    if (attr == 0) {
	return CVM_IO_FILETYPE_REGULAR;  /* maybe samba */
    }
    return CVM_IO_FILETYPE_OTHER;
}

CVMInt32
CVMioOpen(const char *name, CVMInt32 openMode,
	  CVMInt32 filePerm)
{
    DWORD mode = openMode & 0x3;
    DWORD cFlag;
    HANDLE fd;
	int statres;

    switch (mode) {
    case _O_RDONLY: mode = GENERIC_READ; break;
    case _O_WRONLY: mode = GENERIC_WRITE; break;
    case _O_RDWR:   mode = GENERIC_READ | GENERIC_WRITE; break;
    }

    cFlag = OPEN_EXISTING;
    if ((openMode & (_O_CREAT | _O_TRUNC)) ==  (_O_CREAT | _O_TRUNC))
	cFlag = CREATE_ALWAYS;
    else if (openMode & _O_CREAT) cFlag = OPEN_ALWAYS;
    else if (openMode & _O_TRUNC) cFlag = TRUNCATE_EXISTING;
    
    if ((openMode & (_O_CREAT |_O_EXCL)) == (_O_CREAT |_O_EXCL))
	cFlag = CREATE_NEW;

#ifdef WINCE
    {
	WCHAR *wc;
	int fdIndex = -1;
	int i;

	EnterCriticalSection(&fdTableLock);

        /* for (i = 0; i < NUM_FDTABLE_ENTRIES; i++) {  */
	for (i = 3; i < NUM_FDTABLE_ENTRIES; i++) { 
	    if (fdTable[i] == INVALID_HANDLE_VALUE) {
		fdIndex = i;
		break;
	    }
	} 

	if (fdIndex == -1) {
	    LeaveCriticalSection(&fdTableLock);
	    return -1;
	}

	wc = createWCHAR(name);
	fdTable[fdIndex] = CreateFile(wc, mode, 
				      FILE_SHARE_READ | FILE_SHARE_WRITE,
				      0, cFlag, FILE_ATTRIBUTE_NORMAL, 0);
        printf("fdTable[fdIndex] = %d", fdTable[fdIndex]); //djm: Added: Testing
	if (fdTable[fdIndex] == INVALID_HANDLE_VALUE) {
	    LeaveCriticalSection(&fdTableLock);
	    free(wc);
	    return -1;
	}
	fd = (HANDLE)(fdIndex + STD_FDS_BIAS);
	LeaveCriticalSection(&fdTableLock);
	free(wc);
    }
#else
    fd = CreateFileA(name, mode, FILE_SHARE_READ | FILE_SHARE_WRITE,
		     0, cFlag, FILE_ATTRIBUTE_NORMAL, 0);
    if (fd == INVALID_HANDLE_VALUE)
	return -1;

#endif
    statres = fileMode(WIN_GET_HANDLE(fd), &mode);
    if (statres == -1) {
	CloseHandle(WIN_GET_HANDLE(fd));
	return -1;
    }
    if (mode & FILE_ATTRIBUTE_DIRECTORY) {
	CloseHandle(WIN_GET_HANDLE(fd));
	return (CVMInt32)-1;
    }
    if (openMode & _O_APPEND) {
	if (SetFilePointer(WIN_GET_HANDLE(fd), 0, 0, FILE_END) == 0xFFFFFFFF) {
	    CloseHandle(WIN_GET_HANDLE(fd));
	    return (CVMInt32)-1;
	}
    }
    return (CVMInt32)fd;
}

CVMInt32
CVMioClose(CVMInt32 fd)
{
    CVMInt32 ret;
    if (fd < 0)
	return -1;
    
    if (0 <= fd && fd <=2)
	return 0;
    ret = (CloseHandle(WIN_GET_HANDLE((HANDLE)fd)) == 0) ? -1 : 0;

#ifdef WINCE
    fdTable[fd - STD_FDS_BIAS] = INVALID_HANDLE_VALUE; 
#endif
    return ret;
}


CVMInt64
CVMioSeek(CVMInt32 fd, CVMInt64 offset, CVMInt32 whence)
{
    CVMInt64 r;
    LONG lo = (LONG)(offset & 0xffffffff);
    LONG hi = (LONG)((offset >> 32) & 0xffffffff);
    DWORD ret = SetFilePointer(WIN_GET_HANDLE((HANDLE)fd), lo, &hi, whence);
    if (ret == 0xFFFFFFFF && GetLastError() != NO_ERROR) {
        r = -1;
    }
    else {
        r = ret | ((CVMInt64)hi << 32);
    }

    return r;
}

CVMInt32
CVMioSetLength(CVMInt32 fd, CVMInt64 length)
{
    CVMInt64 ret = CVMioSeek(fd, length, FILE_BEGIN);
    if (ret == -1)
	return -1;
    if (SetEndOfFile(WIN_GET_HANDLE((HANDLE)fd)) == 0)
        return -1;
    else
        return 0;
}

CVMInt32
CVMioSync(CVMInt32 fd)
{
    return FlushFileBuffers(WIN_GET_HANDLE((HANDLE)fd));
}

#ifndef WINCE
CVMInt32
nonSeekAvailable(HANDLE han, long *pbytes) {
    /* This is used for available on non-seekable devices
     * (like both named and anonymous pipes, such as pipes
     *  connected to an exec'd process).
     * Standard Input is a special case.
     *
     */

    if (han == (HANDLE)-1) {
	return FALSE;
    }

    if (! PeekNamedPipe(han, NULL, 0, NULL, pbytes, NULL)) {
	/* PeekNamedPipe fails when at EOF.  In that case we
	 * simply make *pbytes = 0 which is consistent with the
	 * behavior we get on Solaris when an fd is at EOF.
	 * The only alternative is to raise and Exception,
	 * which isn't really warranted.
	 */
	if (GetLastError() != ERROR_BROKEN_PIPE) {
	    return FALSE;
	}
	*pbytes = 0;
    }
    return TRUE;
}

CVMInt32
stdinAvailable(int fd, long *pbytes) {
    HANDLE han;
    DWORD numEventsRead = 0;	/* Number of events read from buffer */
    DWORD numEvents = 0;	/* Number of events in buffer */
    DWORD i = 0;		/* Loop index */
    DWORD curLength = 0;	/* Position marker */
    DWORD actualLength = 0;	/* Number of bytes readable */
    BOOL error = FALSE;         /* Error holder */
    INPUT_RECORD *lpBuffer;     /* Pointer to records of input events */

    if ((han = GetStdHandle(STD_INPUT_HANDLE)) == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    /* Construct an array of input records in the console buffer */
    error = GetNumberOfConsoleInputEvents(han, &numEvents);
    if (error == 0) {
	return nonSeekAvailable(han, pbytes);
    }

    /* lpBuffer must fit into 64K or else PeekConsoleInput fails */
    if (numEvents > MAX_INPUT_EVENTS) {
        numEvents = MAX_INPUT_EVENTS;
    }

    lpBuffer = malloc(numEvents * sizeof(INPUT_RECORD));
    if (lpBuffer == NULL) {
     	return FALSE;
    }

    error = PeekConsoleInput(han, lpBuffer, numEvents, &numEventsRead);
    if (error == 0) {
	free(lpBuffer);
	return FALSE;
    }

    /* Examine input records for the number of bytes available */
    for(i=0; i<numEvents; i++) {
	if (lpBuffer[i].EventType == KEY_EVENT) {
            KEY_EVENT_RECORD *keyRecord =
		(KEY_EVENT_RECORD*)&(lpBuffer[i].Event);
	    if (keyRecord->bKeyDown == TRUE) {
                CHAR* keyPressed = (CHAR*) &(keyRecord->uChar);
	       	curLength++;
	       	if (*keyPressed == '\r')
                    actualLength = curLength;
            }
        }
    }
    if(lpBuffer != NULL)
       	free(lpBuffer);
    *pbytes = (long) actualLength;
    return TRUE;
}
#endif

CVMInt32
CVMioAvailable(CVMInt32 fd, CVMInt64 *bytes)
{
    int mode;
    int stat;
    DWORD cur, end;
    long stdbytes;

#ifndef WINCE
    if (fd == 0) {
	if (stdinAvailable(fd, &stdbytes)) {
	    *bytes = (CVMInt64)stdbytes;
	    return 1;
	} else {
	    return 0;
	}
    }
#endif

    stat = fileMode(WIN_GET_HANDLE((HANDLE)fd), &mode);
    if (stat == 0) {
	if (!(mode & FILE_ATTRIBUTE_DIRECTORY)) {
	    cur = (DWORD)CVMioSeek(fd, 0, FILE_CURRENT);
	    end = (DWORD)CVMioSeek(fd, 0, FILE_END);
	    if (CVMioSeek(fd, cur, FILE_BEGIN) != -1) {
		*bytes = CVMlongSub(end, cur);
		return 1;
	    }
	}
    }
    return 0;
}

CVMInt32
CVMioRead(CVMInt32 fd, void *buf, CVMUint32 nBytes)
{
    DWORD bytes;
    BOOL b;
    HANDLE h = WIN_GET_HANDLE((HANDLE)fd);

#ifdef WINCE
    if (fd == 0 && h == INVALID_HANDLE_VALUE) { /* stdin, no PocketConsole */ 
	bytes = readStandardIO(fd, buf, nBytes);
        b = (bytes != 0);
    } else
#else
    switch (fd) {
        case 0: h = GetStdHandle(STD_INPUT_HANDLE); break;
        case 1: h = GetStdHandle(STD_OUTPUT_HANDLE); break;
        case 2: h = GetStdHandle(STD_ERROR_HANDLE); break;
    }
#endif
    { 
	b = ReadFile(h, buf, nBytes, &bytes, NULL);
    }

    if (b) {
	return bytes;
    } else {
	/* Behaviour similar to POSIX read( ) and ensures that 
	 * a parent process reading from a child's output and 
	 * error streams doesn't encounter an IOException when 
	 * the child exits. 
	 */
	if (GetLastError () == ERROR_BROKEN_PIPE) {
	    return 0;
        } else {
	    return -1;
	}
    }
}

#ifdef WINCE
#undef DEBUG
#define DEBUG
#include <dbgapi.h>
#endif

CVMInt32
CVMioWrite(CVMInt32 fd, const void *buf, CVMUint32 nBytes)
{
    DWORD bytes;
    HANDLE h = WIN_GET_HANDLE((HANDLE)fd);
    int b;
    
    /* check if redirection sockets are used */
    if (CVMglobals.target.stdoutPort >= 0 || CVMglobals.target.stderrPort >= 0) {
        /* check if output should be redirected */ 
        if ((fd >= 1) && (fd <= 2)) {
            SIOWrite(fd, (char *)buf, nBytes);
        }
    }

#ifndef WINCE
    switch (fd) {
        case 0: h = GetStdHandle(STD_INPUT_HANDLE); break;
        case 1: h = GetStdHandle(STD_OUTPUT_HANDLE); break;
        case 2: h = GetStdHandle(STD_ERROR_HANDLE); break;
    }
    b = WriteFile(h, buf, nBytes, &bytes, NULL);
#else /* WINCE */
    if (fd >= 1 && fd <= 2) {
	FILE *fp = NULL;
#ifdef CVM_DEBUG
	NKDbgPrintfW(TEXT("%.*hs"), nBytes, buf);
#endif
	switch (fd) {
	    case 1:
		fp = stdout;
		break;
	    case 2:
		fp = stderr;
		break;
	}
	fwrite(buf, sizeof (char), nBytes, fp);
	
	/* Silently ignore errors */
	writeStandardIO(fd, buf, nBytes);
	bytes = nBytes;
	b = 1;
    } else {
	b = WriteFile(h, buf, nBytes, &bytes, NULL);
    }

#endif /* WINCE */
    if (b) {
        return (bytes);
    } else {
        return -1;
    }
}


CVMInt32
CVMioFileSizeFD(CVMInt32 fd, CVMInt64 *size)
{
    DWORD lo, hi;
    lo = GetFileSize(WIN_GET_HANDLE((HANDLE)fd), &hi);
    if (lo == 0xFFFFFFFF)
        return -1;
    else {
        *size = lo | ((CVMInt64)hi << 32);
        return 0;
    }
}

#if defined(WINCE) && defined(CVM_DEBUG)
int fprintf(FILE *fp, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (fp == stdout || fp == stderr) {
	char buf[512];
	int fd;
	if (fp == stdout) {
	    fd = 1;
	} else {
	    fd = 2;
	}
	vsprintf(buf, fmt, args);
	va_end(args);
	return CVMioWrite(fd, buf, strlen(buf));
    } else {
	int n = vfprintf(fp, fmt, args);
	va_end(args);
	return n;
    }
}
#endif
