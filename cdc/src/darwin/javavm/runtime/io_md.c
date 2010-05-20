/*
 * @(#)io_md.c	1.17 06/10/10
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
#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/globals.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

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
    CVMInt32 fd; 
    
    /* 4502246: Remove trailing slashes, since the kernel won't */
    if (name[strlen(name)-1] == '/') {
        char* p; 
        char* newName;
        newName = (char* )malloc(strlen(name) + 1);
	if (newName == NULL) {
	    return -1;
	}
        strcpy(newName, name);
        p = newName + strlen(newName) - 1;
        while ((p > newName) && (*p == '/')) {
            *p-- = '\0';
        }
        fd = open64(newName, openMode, filePerm);
        free(newName);
    }
    else {
        fd = open64(name, openMode, filePerm);
    }
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

static CVMMutex io_lock;

void
LINUXioEnqueue(CVMThreadID *t)
{
    CVMmutexLock(&io_lock);
    if (CVMtargetGlobals->io_queue != NULL) {
	CVMtargetGlobals->io_queue->prev_p = &t->next;
    }
    t->prev_p = &CVMtargetGlobals->io_queue;
    t->next = CVMtargetGlobals->io_queue;
    CVMtargetGlobals->io_queue = t;
    CVMmutexUnlock(&io_lock);
}

void
LINUXioDequeue(CVMThreadID *t)
{
    CVMmutexLock(&io_lock);
    *t->prev_p = t->next;

    if (t->next != NULL) {
	t->next->prev_p = t->prev_p;
    }

    t->next = NULL;
    t->prev_p = NULL;
    CVMmutexUnlock(&io_lock);
}

CVMInt32
CVMioRead(CVMInt32 fd, void *buf, CVMUint32 nBytes)
{
    CVMInt32 result;
    CVMThreadID *self = CVMthreadSelf();
    self->fd = fd;
    LINUXioEnqueue(self);
    result = POSIXioRead(fd, buf, nBytes);
    LINUXioDequeue(self);
    return result;
}

CVMInt32
CVMioClose(CVMInt32 fd)
{
    CVMInt32 result = POSIXioClose(fd);
    /* Interrupt any threads blocking on this fd */
    CVMmutexLock(&io_lock);
    {
	CVMThreadID *t = CVMtargetGlobals->io_queue;
	while (t != NULL) {
	    if (t->fd == fd) {
		pthread_kill(POSIX_COOKIE(t), SIGUSR1);
	    }
	    t = t->next;
	}
    }
    CVMmutexUnlock(&io_lock);
    return result;
}

CVMBool
linuxIoInit(void)
{
    return CVMmutexInit(&io_lock);
}
