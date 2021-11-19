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

#ifndef _SYSMACROS_MD_H_
#define _SYSMACROS_MD_H_

/*
 * Because these are used directly as function ptrs, just redefine the name
 */
#define sysMalloc    malloc
#define sysFree        free
#define sysCalloc    calloc
#define sysRealloc    realloc

/* A macro for sneaking into a sys_mon_t to get the owner sys_thread_t */
#define sysMonitorOwner(mid)   ((mid)->monitor_owner)

#ifdef DEBUG
extern void DumpThreads(void);
void panic (const char *, ...);
#define sysAssert(expression) {        \
    if (!(expression)) {        \
    DumpThreads();            \
    panic("\"%s\", line %d: assertion failure\n", __FILE__, __LINE__); \
    }                    \
}
#else
#define sysAssert(expression) 
#endif

/*
 * Case insensitive compare of ASCII strings
 */
#define sysStricmp(a, b)        strcasecmp(a, b)

/*
 * File system macros
 */
#ifdef UNIX
#define sysOpen(_path, _oflag, _mode)    open(_path, _oflag, _mode)
#define sysNativePath(path)            (path) 
#endif
#ifdef WIN32
#include <io.h>
#define sysOpen(_path, _oflag, _mode)    open(_path, _oflag | O_BINARY, _mode)
char *sysNativePath(char *);
#endif
#define sysRead(_fd, _buf, _n)        read(_fd, _buf, _n)
#define sysWrite(_fd, _buf, _n)        write(_fd, _buf, _n)
#define sysClose(_fd)            close(_fd)
#define sysAccess(_path, _mode)        access(_path, _mode)
#define sysStat(_path, _buf)        stat(_path, _buf)
#define sysMkdir(_path, _mode)        mkdir(_path, _mode)
#define sysUnlink(_path)        unlink(_path)
#define sysIsAbsolute(_path)        (*(_path) == '/')
#define sysCloseDir(_dir)        closedir(_dir)
#define sysOpenDir(_path)        opendir(_path)
#define sysRmdir(_dir)                  remove(_dir)
#define sysSeek(fd, offset, whence)    lseek(fd, offset, whence)
#define sysRename(s, d)            rename(s, d)

#endif /*_SYSMACROS_MD_H_*/
