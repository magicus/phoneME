/*
 * @(#)dirent_md.c	1.24 06/10/10
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

/*
 * Posix-compatible directory access routines
 */

#include <windows.h>
#ifndef WINCE
#include <direct.h>                    /* For _getdrive() */
#endif
#include <errno.h>
#include <assert.h>

#include "dirent_md.h"
#include "javavm/include/winntUtil.h"

/* Caller must have already run dirname through JVM_NativePath, which removes
   duplicate slashes and converts all instances of '/' into '\\'. */

DIR *
opendir(const char *dirname)
{
    DIR *dirp = (DIR *)malloc(sizeof(DIR));
    DWORD fattr;
    char alt_dirname[4] = { 0, 0, 0, 0 };
    TCHAR *tpath;
    if (dirp == 0) {
	errno = ENOMEM;
	return 0;
    }

#ifndef WINCE
    /*
     * Win32 accepts "\" in its POSIX stat(), but refuses to treat it
     * as a directory in FindFirstFile().  We detect this case here and
     * prepend the current drive name.
     */
    if (dirname[1] == '\0' && dirname[0] == '\\') {
	alt_dirname[0] = _getdrive() + 'A' - 1;
	alt_dirname[1] = ':';
	alt_dirname[2] = '\\';
	alt_dirname[3] = '\0';
	dirname = alt_dirname;
    }
#endif

    dirp->path = (char *)malloc(strlen(dirname) + 5);
    if (dirp->path == 0) {
	free(dirp);
	errno = ENOMEM;
	return 0;
    }
    strcpy(dirp->path, dirname);

    tpath = createTCHAR(dirp->path);
    fattr = GetFileAttributes(tpath);
    freeTCHAR(tpath);
    if (fattr == ((DWORD)-1)) {
	free(dirp->path);
	free(dirp);
	errno = ENOENT;
	return 0;
    } else if (fattr & FILE_ATTRIBUTE_DIRECTORY == 0) {
	free(dirp->path);
	free(dirp);
	errno = ENOTDIR;
	return 0;
    }

    /* Append "*.*", or possibly "\\*.*", to path */
    if (dirp->path[1] == ':'
	&& (dirp->path[2] == '\0'
	    || (dirp->path[2] == '\\' && dirp->path[3] == '\0'))) {
	/* No '\\' needed for cases like "Z:" or "Z:\" */
	strcat(dirp->path, "*.*");
    } else {
	strcat(dirp->path, "\\*.*");
    }

    tpath = createTCHAR(dirp->path);
    dirp->handle = FindFirstFile(tpath, &dirp->find_data);
    freeTCHAR(tpath);
    if (dirp->handle == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
	    free(dirp->path);
	    free(dirp);
	    errno = EACCES;
	    return 0;
	}
    }
    return dirp;
}

struct dirent *
readdir(DIR *dirp)
{
    char *cpath;
    if (dirp->handle == INVALID_HANDLE_VALUE) {
	return 0;
    }

    cpath = createMCHAR(dirp->find_data.cFileName);
    strcpy(dirp->dirent.d_name, cpath);
    freeMCHAR(cpath);

    if (!FindNextFile(dirp->handle, &dirp->find_data)) {
	if (GetLastError() == ERROR_INVALID_HANDLE) {
	    errno = EBADF;
	    return 0;
	}
	FindClose(dirp->handle);
	dirp->handle = INVALID_HANDLE_VALUE;
    }

    return &dirp->dirent;
}

int
closedir(DIR *dirp)
{
    if (dirp->handle != INVALID_HANDLE_VALUE) {
	if (!FindClose(dirp->handle)) {
	    errno = EBADF;
	    return -1;
	}
	dirp->handle = INVALID_HANDLE_VALUE;
    }
    free(dirp->path);
    free(dirp);
    return 0;
}

void
rewinddir(DIR *dirp)
{
    TCHAR *tpath;
    if (dirp->handle != INVALID_HANDLE_VALUE) {
	FindClose(dirp->handle);
    }
    tpath = createTCHAR(dirp->path);
    dirp->handle = FindFirstFile(tpath, &dirp->find_data);
    freeTCHAR(tpath);
}
