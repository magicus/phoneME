/*
 * @(#)posix_io_md.c	1.23 06/10/10
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

/* NOTE: the implementations here definitely do not support the
   concept of "interruptible I/O". They are also not thread-safe. Any
   necessary locks should be implemented up in Java. */

/* %comment d001 */
CVMInt32
POSIXioGetLastErrorString(char *buf, CVMInt32 len)
{
   if (errno == 0) {
	return 0;
    } else {
	const char *s = strerror(errno);
	int n = strlen(s);
	if (n >= len) n = len - 1;
	strncpy(buf, s, n);
	buf[n] = '\0';
	return n;
    }
}

/*
 * Return the last error string, without any buffers.
 */
char *
POSIXioReturnLastErrorString()
{
    return strerror(errno);
}

#ifndef POSIX_HAVE_CUSTOM_NATIVE_PATH
char *
POSIXioNativePath(char *path)
{
    return path;
}
#endif

CVMInt32
POSIXioFileType(const char *path)
{
    int ret;
    struct stat buf;

    if ((ret = stat(path, &buf)) == 0) {
      mode_t mode = buf.st_mode & S_IFMT;
      if (mode == S_IFREG) return CVM_IO_FILETYPE_REGULAR;
      if (mode == S_IFDIR) return CVM_IO_FILETYPE_DIRECTORY;
      return CVM_IO_FILETYPE_OTHER;
    }
    return ret;
}


#ifndef POSIX_HAVE_64_BIT_IO
static int
fileMode(int fd, int *mode)
{
    int ret;
    struct stat buf;

    ret = fstat(fd, &buf);
    (*mode) = buf.st_mode;
    return ret;
}

CVMInt32
POSIXioOpen(const char *name, CVMInt32 openMode,
	  CVMInt32 filePerm)
{
    CVMInt32 fd = open(name, openMode, filePerm);
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
#endif

CVMInt32
POSIXioClose(CVMInt32 fd)
{
    return close(fd);
}

#if !defined(POSIX_HAVE_64_BIT_IO) && !defined(POSIX_HAVE_CUSTOM_LSEEK)
CVMInt64
POSIXioSeek(CVMInt32 fd, CVMInt64 offset, CVMInt32 whence)
{
    return lseek(fd, offset, whence);
}
#endif

#ifndef POSIX_HAVE_64_BIT_IO
CVMInt32
POSIXioSetLength(CVMInt32 fd, CVMInt64 length)
{
    /* %comment d002 */
    CVMInt32 length32;
    CVMInt64 length64;
    length32 = CVMlong2Int(length);
    length64 = CVMint2Long(length32);

    if (CVMlongNe(length64, length)) {
	/* 32-bit overflow */
	return -1;
    }
#ifdef POSIX_HAVE_FTRUNCATE
    return ftruncate(fd, length32);
#elif defined(POSIX_HAVE_FIOTRUNC)
    /* %comment d004 */
    return ioctl(fd, FIOTRUNC, length32);
#else
    return -1;
#endif
}
#endif

CVMInt32
POSIXioSync(CVMInt32 fd)
{
#ifdef POSIX_HAVE_FSYNC
    return fsync(fd);
#elif defined(POSIX_HAVE_FIOFLUSH)
    /* %comment d004 */
    return ioctl(fd, FIOFLUSH, 0);
#else
    return -1;
#endif
}

#if !defined(POSIX_HAVE_64_BIT_IO) && !defined(POSIX_HAVE_CUSTOM_IO_AVAILABLE)
CVMInt32
POSIXioAvailable(CVMInt32 fd, CVMInt64 *bytes)
{
    CVMInt64 cur, end;
    int mode;

    if (fileMode(fd, &mode) >= 0) {
        if (S_ISCHR(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)) {
	    int n;
            if (ioctl(fd, FIONREAD, &n) >= 0) {
		/* %comment d003 */
		*bytes = CVMint2Long(n);
                return 1;
            }
        }
    }
    {
	CVMInt32 cur32, end32;
	if ((cur32 = lseek(fd, 0L, SEEK_CUR)) == -1) {
	    return 0;
	} else if ((end32 = lseek(fd, 0L, SEEK_END)) == -1) {
	    return 0;
	} else if (lseek(fd, cur32, SEEK_SET) == -1) {
	    return 0;
	}
	/* %comment d003 */
	cur = CVMint2Long(cur32);
	end = CVMint2Long(end32);
    }
    *bytes = CVMlongSub(end, cur);
    return 1;
}
#endif

CVMInt32
POSIXioRead(CVMInt32 fd, void *buf, CVMUint32 nBytes)
{
    return read(fd, buf, nBytes);
}

CVMInt32
POSIXioWrite(CVMInt32 fd, const void *buf, CVMUint32 nBytes)
{
    return write(fd, buf, nBytes);    
}

#ifndef POSIX_HAVE_64_BIT_IO
CVMInt32
POSIXioFileSizeFD(CVMInt32 fd, CVMInt64 *size)
{
    int ret;
    struct stat buf;

    ret = fstat(fd, &buf);
    /* %comment d003 */
    *size = CVMint2Long(buf.st_size);
    return ret;
}
#endif
