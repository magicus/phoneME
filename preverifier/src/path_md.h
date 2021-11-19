/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _PATH_MD_H_
#define _PATH_MD_H_

#define    DIR_SEPARATOR        '/'

#ifdef UNIX
#define    LOCAL_DIR_SEPARATOR        '/'
#define PATH_SEPARATOR          ':'

#include <dirent.h>
#endif
#ifdef WIN32

#define    LOCAL_DIR_SEPARATOR        '\\'
#define PATH_SEPARATOR          ';'

#include <direct.h>
struct dirent {
    char d_name[1024];
};

typedef struct {
    struct dirent dirent;
    char *path;
    HANDLE handle;
    WIN32_FIND_DATA find_data;
} DIR;

DIR *opendir(const char *dirname);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#endif

#endif /* !_PATH_MD_H_ */
