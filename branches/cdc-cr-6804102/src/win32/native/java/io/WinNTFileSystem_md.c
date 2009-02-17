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
 */

#include <windows.h>

#include "java_io_WinNTFileSystem.h"

JNIEXPORT jint JNICALL
Java_java_io_WinNTFileSystem_listRoots0(JNIEnv *env, jobject ignored)
{
    return GetLogicalDrives();
}

#include <direct.h>
#include <stdlib.h>

JNIEXPORT jobject JNICALL
Java_java_io_WinNTFileSystem_getDriveDirectory(JNIEnv *env, jobject this, 
                                               jint drive) 
{
    jchar buf[_MAX_PATH];
    jchar *p = _wgetdcwd(drive, buf, _MAX_PATH);

    if (p == NULL) return NULL;
    if (iswalpha(*p) && (p[1] == L':')) p += 2;
    return (*env)->NewString(env, p, wcslen(p));
}
