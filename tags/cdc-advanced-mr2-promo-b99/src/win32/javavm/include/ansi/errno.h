/*
 * @(#)errno.h	1.10 06/10/10
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

#ifndef WINCE_ERRNO_H
#define WINCE_ERRNO_H
//error not running on Windows CE!
#if 0
/* definition of errno is in stdlib.h in winCE 2.0 */
#if (_WIN32_WCE < 211)
 #include <stdlib.h>
#else
 /* but in 2.11 it isn't */
#include "wceUtil.h"
JAVAI_API int *_errno();
 #define errno (*_errno())
#endif /* 211 */

#endif

#include "javavm/include/wceUtil.h"

/* _declspec(dllexport) int *_errno(); */
int errno;

/*
#define errno   GetLastError()
*/


/* Error Codes */
#define EPERM           1
#define ENOENT          2
#define ESRCH           3
#define EINTR           4
#define EIO             5
#define ENXIO           6
#define E2BIG           7
#define ENOEXEC         8
#define EBADF           9
#define ECHILD          10
#define EAGAIN          11
#define ENOMEM          12
#define EACCES          13
#define EFAULT          14
#define EBUSY           16
#define EEXIST          17
#define EXDEV           18
#define ENODEV          19
#define ENOTDIR         20
#define EISDIR          21
#define EINVAL          22
#define ENFILE          23
#define EMFILE          24
#define ENOTTY          25
#define EFBIG           27
#define ENOSPC          28
#define ESPIPE          29
#define EROFS           30
#define EMLINK          31
#define EPIPE           32
#define EDOM            33
#define ERANGE          34
#define EDEADLK         36

/*
#ifndef ENAMETOOLONG
#define ENAMETOOLONG    38
#endif

#define ENOLCK          39
#define ENOSYS          40

#ifndef ENOTEMPTY
#define ENOTEMPTY       41
#endif
*/
#include <winsock.h>   /* for Error Codes */

#define EILSEQ          42

/* #define ENAMETOOLONG 78  path name is too long                */


#endif /* WINCE_ERRNO_H */
