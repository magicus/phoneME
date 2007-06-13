/*
 * @(#)fs_util_md.h	1.1 07/01/04
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

#ifndef _WIN32_FS_UTIL_H_
#define _WIN32_FS_UTIL_H_

#ifdef WINCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

int _access(const char *path, int mode);
int rename(const char *oldpath, const char *newpath);
int _chdrive(int drive);
int _getdrive();
char* _getdcwd(int drive, char *buf, int maxlen);
wchar_t* _wgetdcwd(int drive, wchar_t *buf, int maxlen);

#endif
#endif
