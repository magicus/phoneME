/*
 * @(#)fs_util_md.c	1.1 07/01/14
 *
 * Copyright  2007-2007 Davy Preuveneers. All Rights Reserved.
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
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>


#ifdef WINCE

/*
 * _access() is used in the Java_java_io_Win32FileSysten_checkAccess()
 * JNI method of Win32FileSystem_md.c, but is never executed if WINCE is
 * defined. The Java_java_io_WinNTFileSysten_checkAccess() JNI method 
 * is called instead, which uses _waccess().
 *
 * This method is therefore only implemented to satisfy the JNI bindings.
 */
int _access(const char *path, int mode) {
    FILE *f;

    if (path == NULL) {
        return -1;
    }

    f = fopen(path, "r");
    if (f == NULL) {
        return -1;
    }
    fclose(f);

    return 0;
}

/*
 * rename() is used in the Java_java_io_Win32FileSysten_rename0()
 * JNI method of Win32FileSystem_md.c, but never executed if WINCE is
 * defined. The Java_java_io_WinNTFileSysten_rename0() JNI method 
 * is called instead, which uses _wrename().
 *
 * This method is therefore only implemented to satisfy the JNI bindings.
 */
int rename(const char *oldpath, const char *newpath) {
    wchar_t woldpath[_MAX_PATH];
    wchar_t wnewpath[_MAX_PATH];
    int res;

    if (oldpath == NULL || newpath == NULL) {
        return -1;
    }

    mbstowcs(woldpath, oldpath, _MAX_PATH);
    mbstowcs(wnewpath, newpath, _MAX_PATH);

    if (_access(newpath, 0) == 0) {
        res = DeleteFileW(wnewpath);
        if (!res) {
            return -1;
        }
    }

    res = MoveFileW(woldpath, wnewpath);
    if (!res) {
        return -1;
    }

    return 0;
}

/*
 * A helper method for the _getdcwd() and _wgetdcwd() methods below to change
 * the current drive. This method will likely return an error as drive letters
 * are never used in a pathname on a PocketPC 2003 device.
 */
int _chdrive(int drive) {
    wchar_t d[3];
    int res;

    if (drive < 1 || drive > 26)
        return -1;

    d[0] = '@' + drive;
    d[1] = ':';
    d[2] = 0;

    res = SetCurrentDirectory(d);
    if (!res) {
        return -1;
    }

    return 0;
}

/*
 * A helper method for the _getdcwd() and _wgetdcwd() methods below to return
 * the current drive. This method will likely return an error as drive letters
 * are never used in a pathname on a PocketPC 2003 device.
 */
int _getdrive() {
    wchar_t CurrDir[_MAX_PATH];
    int res;

    res = GetCurrentDirectory(_MAX_PATH, CurrDir);
    if ((res == 0) || (res > _MAX_PATH)) {
        return 0;
    }

    if (CurrDir[1] == ':') {
        return toupper(CurrDir[0]) - '@';
    }

    // On failure return 0
    return 0;
}

/*
 * _getdcwd() is called in the Java_java_io_Win32FileSysten_getDriveDirectory()
 * JNI method of Win32FileSystem_md.c, which in turn is called in the java
 * getDriveDirectory(char drive) method of the java.io.Win32FileSystem class.
 * The latter is called in the resolve() method of the same java class, but
 * perhaps never executed as drive letters are never used in a pathname on
 * PocketPC 2003.
 *
 * This method is never executed if WINCE is defined. In that case the 
 * Java_java_io_WinNTFileSysten_getDriveDirectory() JNI method is called 
 * instead, which uses _wgetdcwd() defined below.
 *
 * This method is therefore only implemented to satisfy the JNI bindings.
 */
char* _getdcwd(int drive, char *buf, int maxlen) {
    wchar_t CurrDir[_MAX_PATH];
    int res;

    // Get the current drive
    int currdrive = _getdrive();

    // Invalid drive?
    if (currdrive == 0 || drive < 1 || drive > 26)
        return 0;

    // Do we need to change the current drive?
    if (currdrive != drive) {
        int err = _chdrive(drive);
        if (err == -1) {
            return 0;
        }
    }

    // Get the full current directory
    res = GetCurrentDirectory(_MAX_PATH, CurrDir);

    // Change the current drive to the old one
    if (currdrive != drive) {
        int err = _chdrive(currdrive);
        if (err == -1) {
            return 0;
        }
    }

    // Check the returned length of the current directory
    if (res == 0 || maxlen == 0 || res > _MAX_PATH || res > maxlen) {
        return 0;
    }

    // If buf is NULL, allocate maxlen bytes of memory
    if (!buf) {
        buf = (char *)malloc(maxlen + 1);
        if (!buf) {
            return 0;
        }
    }

    // Convert the string to char format
    wcstombs(buf, CurrDir, _MAX_PATH);

    // Return the pathname
    return buf;
}

/*
 * _wgetdcwd() is called in the Java_java_io_WinNTFileSysten_getDriveDirectory()
 * JNI method of WinNTFileSystem_md.c, which in turn is called in the java
 * getDriveDirectory(char drive) method of the java.io.Win32FileSystem class.
 *
 * The native getDriveDirectory() is declared as protected in Win32FileSystem and
 * WinNTFileSystem, so no other methods make use of this one.
 *
 * Moreover, the resolve() method which indirectly calls this method first tests
 * if the prefix length is 2 and if the first character is a slash. Therefore,
 * the getDriveDirectory() method is never called and neither is this method.
 *
 * This method is therefore only implemented to satisfy the JNI bindings.
 *
 * See also the _getdcwd() method.
 */
wchar_t* _wgetdcwd(int drive, wchar_t *buf, int maxlen) {
    wchar_t CurrDir[_MAX_PATH];
    int res;

    int currdrive = _getdrive();

    if (currdrive == 0 || drive < 1 || drive > 26)
        return 0;

    if (currdrive != drive) {
        int err = _chdrive(drive);
        if (err == -1) {
            return 0;
        }
    }

    res = GetCurrentDirectory(_MAX_PATH, CurrDir);

    if (currdrive != drive) {
        int err = _chdrive(currdrive);
        if (err == -1) {
            return 0;
        }
    }

    if (res == 0 || maxlen == 0 || res > _MAX_PATH || res > maxlen) {
        return 0;
    }

    if (!buf) {
        buf = (wchar_t *)malloc(2 * (maxlen + 1));
        if (!buf) {
            return 0;
        }
    }
    wcscpy(buf, CurrDir);

    return buf;
}

#endif
