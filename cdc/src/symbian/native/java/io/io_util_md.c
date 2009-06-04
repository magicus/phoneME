/*
 * @(#)io_util_md.c	1.3 06/10/10
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

#include "jni.h"
#include "jni_util.h"
#include "jvm.h"
#include "io_util.h"
#include <stdio.h>

//FIXME #include "java_io_RandomAccessFile.h"

extern jboolean onNT = JNI_FALSE;

void
initializeWindowsVersion() {
    OSVERSIONINFO ver;
    ver.dwOSVersionInfoSize = sizeof(ver);
    GetVersionEx(&ver);
    if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        onNT = JNI_TRUE;
    } else {
        onNT = JNI_FALSE;
    }
}

// FIXME I don't think we need this 
//void
//fileOpen(JNIEnv *env, jobject this, jstring path, jfieldID fid, int flags)
//{

//    if (onNT) {
//        WCHAR *pathbuf = NULL;
//        jint fd;
//        DWORD handleFlags = 0;
//        DWORD access = 0;
//        DWORD mode = 0;
//        DWORD sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
//        DWORD disposition = OPEN_EXISTING;
//          DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
//        HANDLE h = NULL;
//        int pathlen = 0;
//
//        WITH_UNICODE_STRING(env, path, ps) {
//            pathlen = wcslen(ps);
//            if (pathlen != 0) {
//                if (pathlen > MAX_PATH - 1) {
//                    /* copy \\?\ to the front of path */
//                    pathbuf = (WCHAR*)malloc((pathlen + 10) * 2);
//                    if (pathbuf != 0) {                    
//                        pathbuf[0] = L'\0';
//                        wcscpy(pathbuf, L"\\\\?\\\0");
//                        wcscat(pathbuf, ps);
//                    }
//                } else {
//                    pathbuf = (WCHAR*)malloc((pathlen + 6) * 2);
//                    if (pathbuf != 0) {
//                        pathbuf[0] = L'\0';
//                        wcscpy(pathbuf, ps);
//                    }
//                }
//            }
//        } END_UNICODE_STRING(env, ps);
//
//        if (pathlen == 0) {
//            throwFileNotFoundException(env, path);
//            return;
//        }

//        if (pathbuf == 0) {
//            JNU_ThrowOutOfMemoryError(env, 0);
//            return;
//        }

        /* Note: O_TRUNC overrides O_CREAT */
//        if (flags & O_TRUNC) {
//            disposition = CREATE_ALWAYS;
//        } else if (flags & O_CREAT) {
//            disposition = OPEN_ALWAYS;
//        }
//        if (flags & O_SYNC) {
//            flagsAndAttributes = FILE_FLAG_WRITE_THROUGH;
//        }
//        if (flags & O_DSYNC) {
//            flagsAndAttributes = FILE_FLAG_WRITE_THROUGH;
//        }
//        if (flags & O_RDONLY != 0) {
//            access = GENERIC_READ;
//        }
//        if (flags & O_WRONLY) {
//            access = GENERIC_WRITE;
//        }
//       if (flags & O_RDWR) {
//           access = GENERIC_READ | GENERIC_WRITE;
//        }
//        if (flags == 0) {
//            access = GENERIC_READ;
//        }

//        h = CreateFileW(
//            pathbuf,   /* Wide char path name */
//            access,    /* Combine read and/or write permission */
//            sharing,   /* File sharing flags */
//            NULL,      /* Security attributes */
//            disposition,         /* creation disposition */
//            flagsAndAttributes,  /* flags and attributes */
//            NULL);

 //       free(pathbuf);

//        if (h == INVALID_HANDLE_VALUE) {
//            throwFileNotFoundException(env, path);
//            return;
//        }

//        if (flags & O_APPEND) {
//            handleFlags = _O_APPEND;
//        }

//        fd = _open_osfhandle(h, handleFlags);
//
//        if (fd >= 0) {
//            SET_FD(fd, fid);
//        } else {
//            JNU_ThrowIOExceptionWithLastError(env,
//                                              "More than 2048 open files");
//            return;
//        }
            
//    } else {
//        jint fd;
//        WITH_PLATFORM_STRING(env, path, _ps) {
//            fd = JVM_Open(_ps, flags, 0666);
//            if (fd >= 0) {
//                SET_FD(fd, fid);
//            }
//        } END_PLATFORM_STRING(env, _ps);

//        if (fd < 0) {
//            throwFileNotFoundException(env, path);
//        }    
//    }
//}

