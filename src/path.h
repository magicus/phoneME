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

#ifndef    _PATH_H_
#define    _PATH_H_

#include <sys/stat.h>
#include <path_md.h>

typedef struct {
    struct {
        unsigned long locpos;
    unsigned long cenpos;
    } jar;
    char type;
    char name[1];
} zip_t; 

/*
 * Class path element, which is either a directory or zip file.
 */
typedef struct {
    enum { CPE_DIR, CPE_ZIP } type;
    union {
    zip_t *zip;
    char *dir;
    } u;
} cpe_t;

cpe_t **sysGetClassPath(void);
void pushDirectoryOntoClassPath(char* directory);
void pushJarFileOntoClassPath(zip_t *zip);
void popClassPath(void);

bool_t 
findJARDirectories(zip_t *entry, struct stat *statbuf);

#endif    /* !_PATH_H_ */
