/*
 * @(#)io_md.c	1.15 06/10/10
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
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/ansi/errno.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ioLib.h>
#include <taskLib.h>

static fd_set append_set;

CVMInt32
CVMioOpen(const char *name, CVMInt32 openMode, CVMInt32 filePerm)
{
    CVMInt32 fd;

    taskLock();

    {
	struct stat buf;
	STATUS status;

	status = stat(name, &buf);
	if (status == OK) {
	    if ((openMode & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL)) {
		errno = EEXIST;
		taskUnlock();
		return -1;
	    }
	    /* Work around NFS bug, where it will delete an empty directory */
	    if (S_ISDIR(buf.st_mode) &&
		((openMode & O_WRONLY) == O_WRONLY ||
		(openMode & O_RDWR) == O_RDWR))
	    {
		errno = EISDIR;
		taskUnlock();
		return -1;
	    }
	}
    }

    fd = POSIXioOpen(name, openMode, filePerm);

    taskUnlock();

    if (fd != -1) {
	assert(fd >= 0 && fd < FD_SETSIZE);
	if ((openMode & O_APPEND) == O_APPEND) {
	    FD_SET(fd, &append_set);
	} else {
	    FD_CLR(fd, &append_set);
	}
    }


    return fd;
}

CVMInt32
CVMioWrite(CVMInt32 fd, const void *buf, CVMUint32 nBytes)
{
    CVMInt32 result;

    if (fd < 0 || fd >= FD_SETSIZE) {
#ifdef CVM_DEBUG
	fprintf(stderr, "fd %d passed to CVMioWrite!\n", fd);
#endif
	errno = EBADF;
	return -1;
    }

    taskLock();

    if (FD_ISSET(fd, &append_set)) {
	CVMInt64 pos = CVMioSeek(fd, 0LL, SEEK_END);
	assert(pos != -1LL);
    }
    result = POSIXioWrite(fd, buf, nBytes);

    taskUnlock();

    return result;
}

char *
CVMioNativePath(char *path)
{
    /*
     * Support for File.toURL() which insists on prepending
     * a slash to pathnames.  This should have been removed in
     * a higher layer.  Since it wasn't, we have to remove it here.
     * See bugid 4288740
     */
    if (path[0] == '/') {
	char *p = strchr(path + 1, ':');
	if (p != NULL && p[1] == '/') {
	    memmove(path, path + 1, strlen(path));
	}
    }
    return path;
}

CVMInt64
CVMioSeek(CVMInt32 fd, CVMInt64 offset, CVMInt32 whence)
{
    if (whence == SEEK_END) {
	/* Avoid VxWorks FIONREAD bug */
	struct stat buf;
	if (fstat(fd, &buf) == OK) {
	    return lseek(fd, buf.st_size + offset, SEEK_SET);
	} else {
	    return -1;
	}
    } else {
	return lseek(fd, offset, whence);
    }
}

CVMInt32
CVMioAvailable(CVMInt32 fd, CVMInt64 *bytes)
{
    CVMInt64 cur, end;
    struct stat buf;
    STATUS status;

    /* Use lseek() if it's a regular file, otherwise use FIONREAD */
    status = fstat(fd, &buf);
    if (status == OK) {
	int mode = buf.st_mode;
	if (!S_ISCHR(mode) && !S_ISFIFO(mode) && !S_ISSOCK(mode)) {
	    CVMInt32 cur32, end32;
	    if ((cur32 = lseek(fd, 0L, SEEK_CUR)) == -1) {
		return 0;
	    } else if ((end32 = lseek(fd, 0L, SEEK_END)) == -1) {
		return 0;
	    } else if (lseek(fd, cur32, SEEK_SET) == -1) {
		return 0;
	    }
	    /* %comment: mam050 */
	    cur = CVMint2Long(cur32);
	    end = CVMint2Long(end32);
	    *bytes = CVMlongSub(end, cur);
	    return 1;
	}
    }
    {
	int n;
	if (ioctl(fd, FIONREAD, (int)&n) >= 0) {
	    /* %comment: mam050 */
	    *bytes = CVMint2Long(n);
	    return 1;
	}
    }
    return 0;
}

void
ioInitStaticState(void)
{
    FD_ZERO(&append_set);
}

void
ioDestroyStaticState(void)
{
}
