/*
 * @(#)io_md.c	1.12 06/10/10
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

#include "javavm/include/porting/io.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/doubleword.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef POSIX_HAVE_64_BIT_IO

static int
fileMode(int fd, int *mode)
{
    int ret;
    struct stat64 buf64;

    ret = fstat64(fd, &buf64);
    (*mode) = buf64.st_mode;
    return ret;
}

CVMInt32
CVMioOpen(const char *name, CVMInt32 openMode,
	  CVMInt32 filePerm)
{
    CVMInt32 fd = open64(name, openMode, filePerm);
    if (fd >= 0) {
	int mode;
	int statres = fileMode(fd, &mode);
	if (statres == -1) {
	    close(fd);
	    return -1;
	}
	if (S_ISDIR(mode)) {
	    close(fd);
	    return -1;
	}
    }
    return fd;
}

CVMInt64
CVMioSeek(CVMInt32 fd, CVMInt64 offset, CVMInt32 whence)
{
    return lseek64(fd, offset, whence);
}

CVMInt32
CVMioSetLength(CVMInt32 fd, CVMInt64 length)
{
    /* %comment w007 */
    return ftruncate64(fd, length);
}

CVMInt32
CVMioAvailable(CVMInt32 fd, CVMInt64 *bytes)
{
    CVMInt64 cur, end;
    int mode;

    if (fileMode(fd, &mode) >= 0) {
        if (S_ISCHR(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)) {
	    int n;
            if (ioctl(fd, FIONREAD, &n) >= 0) {
		/* %comment w008 */
		*bytes = CVMint2Long(n);
                return 1;
            }
        }
    }
    if ((cur = lseek64(fd, 0L, SEEK_CUR)) == -1) {
	return 0;
    } else if ((end = lseek64(fd, 0L, SEEK_END)) == -1) {
	return 0;
    } else if (lseek64(fd, cur, SEEK_SET) == -1) {
	return 0;
    }
    *bytes = CVMlongSub(end, cur);
    return 1;
}

CVMInt32
CVMioFileSizeFD(CVMInt32 fd, CVMInt64 *size)
{
    int ret;
    struct stat64 buf64;

    ret = fstat64(fd, &buf64);
    *size = buf64.st_size;
    return ret;
}

#endif /* POSIX_HAVE_64_BIT_IO */
