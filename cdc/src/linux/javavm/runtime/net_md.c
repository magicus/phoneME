/*
 * @(#)net_md.c	1.8 06/10/10
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

#include "javavm/include/porting/net.h"
#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/io_md.h"
#include <stdio.h>

CVMInt32
CVMnetConnect(CVMInt32 fd, struct sockaddr *him, CVMInt32 len)
{
    CVMInt32 result;
    CVMThreadID *self = CVMthreadSelf();
    self->fd = fd;
    LINUXioEnqueue(self);
    result = POSIXnetConnect(fd, him, len);
    LINUXioDequeue(self);
    return result;
}

CVMInt32
CVMnetAccept(CVMInt32 fd, struct sockaddr *him, CVMInt32 *len)
{
    CVMInt32 result;
    CVMThreadID *self = CVMthreadSelf();
    self->fd = fd;
    LINUXioEnqueue(self);
    result = POSIXnetAccept(fd, him, len);
    LINUXioDequeue(self);
    return result;
}

CVMInt32
CVMnetSendTo(CVMInt32 fd, char *buf, CVMInt32 len, CVMInt32 flags,
	     struct sockaddr *to, CVMInt32 tolen)
{
    CVMInt32 result;
    CVMThreadID *self = CVMthreadSelf();
    self->fd = fd;
    LINUXioEnqueue(self);
    result = POSIXnetSendTo(fd, buf, len, flags, to, tolen);
    LINUXioDequeue(self);
    return result;
}

CVMInt32
CVMnetRecvFrom(CVMInt32 fd, char *buf, CVMInt32 nBytes,
	       CVMInt32 flags, struct sockaddr *from,
	       CVMInt32 *fromlen)
{
    CVMInt32 result;
    CVMThreadID *self = CVMthreadSelf();
    self->fd = fd;
    LINUXioEnqueue(self);
    result = POSIXnetRecvFrom(fd, buf, nBytes, flags, from, fromlen);
    LINUXioDequeue(self);
    return result;
}

CVMInt32
CVMnetRecv(CVMInt32 fd, char *buf, CVMInt32 nBytes,
	   CVMInt32 flags)
{
    CVMInt32 result;
    CVMThreadID *self = CVMthreadSelf();
    self->fd = fd;
    LINUXioEnqueue(self);
    result = POSIXnetRecv(fd, buf, nBytes, flags);
    LINUXioDequeue(self);
    return result;
}

CVMInt32
CVMnetSend(CVMInt32 fd, char *buf, CVMInt32 nBytes,
	   CVMInt32 flags)
{
    CVMInt32 result;
    CVMThreadID *self = CVMthreadSelf();
    self->fd = fd;
    LINUXioEnqueue(self);
    result = POSIXnetSend(fd, buf, nBytes, flags);
    LINUXioDequeue(self);
    return result;
}

CVMInt32
CVMnetTimeout(CVMInt32 fd, CVMInt32 timeout)
{
    CVMInt32 result;
    CVMThreadID *self = CVMthreadSelf();
    self->fd = fd;
    LINUXioEnqueue(self);
    result = POSIXnetTimeout(fd, timeout);
    LINUXioDequeue(self);
    return result;
}

void
linuxNetInit(void)
{
}
