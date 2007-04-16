/*
 *  
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
#ifndef _PCSL_UTIL_FILELIST_H_
#define _PCSL_UTIL_FILELIST_H_

#include <pcsl_file.h>

/**
 * @file
 * @ingroup system
 *
 * @brief Interface to useful utilities for file list iteration for posix 
 *        compliant OSes.
 */

typedef struct _PCSLStorageDirInfo {
    int savedRootLength;
    int savedMatchLength;
    void* savedDirectory;
} PCSLStorageDirInfo;

void *pcsl_util_openfileiterator(const pcsl_string * filelist);
int pcsl_util_closefileiterator(void *handle);
int pcsl_util_stringlastindexof(const pcsl_string, jchar ch);
void pcsl_util_stringcat(const pcsl_string * str1, const pcsl_string * str2,
                         pcsl_string * result);

#endif /* _PCSL_UTIL_FILELIST_H_ */
