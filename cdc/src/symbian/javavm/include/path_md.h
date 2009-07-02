/*
 * @(#)path_md.h	1.4 06/10/10
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

#ifndef _SYMBIAN_PATH_MD_H
#define _SYMBIAN_PATH_MD_H

#include <limits.h>

#ifndef PATH_MAX
#ifdef __cplusplus
#include <e32def.h>
#include <e32std.h>
#define PATH_MAX KMaxFullName
#else
#include <unistd.h>
#define PATH_MAX MAXPATHLEN
#endif
#endif

#define CVM_PATH_MAXLEN                 PATH_MAX
#define CVM_PATH_CLASSFILEEXT           "class"
#define CVM_PATH_LOCAL_DIR_SEPARATOR    '\\'
#define CVM_PATH_CLASSPATH_SEPARATOR    ";"
#define CVM_PATH_CURDIR                 "."

/* Defined in canonicalize_md.c */
extern int
canonicalize(const char *original, char *resolved, int len);
#define CVMcanonicalize canonicalize

IMPORT_C char *
realpath(const char *, char *);

#endif /* _SYMBIAN_PATH_MD_H */
