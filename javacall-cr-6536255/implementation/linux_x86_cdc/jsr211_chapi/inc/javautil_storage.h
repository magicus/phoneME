/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * @file javacall_invoke.h
 * @ingroup CHAPI
 * @brief Content handlers executor interface for JSR-211 CHAPI
 */


/**
 * @defgroup CHAPI JSR-211 Content Handler API (CHAPI)
 *
 *  The following API definitions are required by JSR-211.
 *  These APIs are not required by standard JTWI implementations.
 *
 * @{
 */

#ifndef __JAVAUTIL_STORAGE_H___
#define __JAVAUTIL_STORAGE_H___

#include <javacall_defs.h>

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

// opens on read, storage must exist
#define JUS_O_RDONLY 0

// opens on read/wrie, storage must exist (unless JUS_O_CREATE is set)
#define JUS_O_RDWR 1

// if storage not exists it is created (used with JUS_O_RDWR)
#define JUS_O_CREATE 2


typedef void* javautil_storage;

#define JAVAUTIL_INVALID_STORAGE_HANDLE ((void*)-1L)


#define JUS_SEEK_CUR    1
#define JUS_SEEK_END    2
#define JUS_SEEK_SET    0

#define MAX_STORAGE_NAME 256

javacall_result javautil_storage_open(const char* name, int flag, /* OUT */ javautil_storage* storage);
javacall_result javautil_storage_gettmpname(char* name, int* len);
javacall_result javautil_storage_close(javautil_storage storage);
javacall_result javautil_storage_remove(const char* name);
javacall_result javautil_storage_rename(const char *oldname, const char *newname);
javacall_result javautil_storage_getsize(javautil_storage storage,/* OUT */ long* size);
javacall_result javautil_storage_setpos(javautil_storage storage, long pos, int flag);
javacall_result javautil_storage_getpos(javautil_storage storage, /* OUT */long* pos);
int javautil_storage_read(javautil_storage storage, char* buffer, unsigned int size);
int javautil_storage_write(javautil_storage storage, char* buffer, unsigned int size);

/*
int _chsize(int a, long b);
int _close(int a);
int _open(char* name, int flag);
long _filelength(int a);
long _lseek(int a, long b , int c);
int _read(int a , void *b, unsigned int c);
long tell(int a);
int _write(int a, const void *b, unsigned int c);
*/

#ifdef __cplusplus
}
#endif/*__cplusplus*/

#endif //__JAVAUTIL_STORAGE_H___
