/*
 * @(#)posix_net_md.c	1.24 06/10/10
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
#include "javavm/include/porting/io.h"
#include "javavm/include/porting/time.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/doubleword.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <sys/socket.h>
#include <netdb.h>

#include "javavm/include/porting/net.h"
#include "javavm/include/porting/io.h"

/* NOTE that this will NOT work with winsock, because integer file
   descriptors and sockets aren't in the same "namespace". I hope it
   works on VxWorks. */

CVMInt32
POSIXnetSocketClose(CVMInt32 fd)
{
    return CVMioClose(fd);
}

CVMInt32
POSIXnetSocketShutdown(CVMInt32 fd, CVMInt32 howto)
{
    return shutdown(fd, howto);
}

CVMInt32
POSIXnetSocketAvailable(CVMInt32 fd, CVMInt32 *pbytes)
{
    long ret = 1;

    if (fd < 0 || ioctl(fd, FIONREAD, pbytes) < 0) {
	ret = 0;
    } 
    return ret;
}

CVMInt32
POSIXnetConnect(CVMInt32 fd, struct sockaddr *him, CVMInt32 len)
{
    return connect(fd, him, len);
}

CVMInt32
POSIXnetAccept(CVMInt32 fd, struct sockaddr *him, CVMInt32 *len)
{
    assert(sizeof(socklen_t) == sizeof(CVMInt32));
    return accept(fd, him, (socklen_t *)len);
}

CVMInt32
POSIXnetSendTo(CVMInt32 fd, char *buf, CVMInt32 len, CVMInt32 flags,
	     struct sockaddr *to, CVMInt32 tolen)
{
    return sendto(fd, buf, len, flags, to, tolen);
}

CVMInt32
POSIXnetRecvFrom(CVMInt32 fd, char *buf, CVMInt32 nBytes,
	       CVMInt32 flags, struct sockaddr *from,
	       CVMInt32 *fromlen)
{
    assert(sizeof(socklen_t) == sizeof(CVMInt32));
    return recvfrom(fd, buf, nBytes, flags, from, (socklen_t *)fromlen);
}

CVMInt32
POSIXnetListen(CVMInt32 fd, CVMInt32 count)
{
    return listen(fd, count);
}

CVMInt32
POSIXnetRecv(CVMInt32 fd, char *buf, CVMInt32 nBytes, CVMInt32 flags)
{
    return recv(fd, buf, nBytes, flags);
}

CVMInt32
POSIXnetSend(CVMInt32 fd, char *buf, CVMInt32 nBytes, CVMInt32 flags)
{
    return send(fd, buf, nBytes, flags);
}

#ifndef POSIX_USE_SELECT
#include <poll.h>
#endif

CVMInt32
POSIXnetTimeout(CVMInt32 fd, CVMInt32 timeout)
{
#ifndef POSIX_USE_SELECT
    struct pollfd pfd;

    pfd.fd = fd;
    pfd.events = POLLIN;

    return poll(&pfd, 1, timeout);
#else
    fd_set tbl;
    struct timeval t;

    if ((fd < 0) || (fd >= FD_SETSIZE)) {
        return CVM_IO_ERR;
    }

    t.tv_sec = timeout / 1000;
    t.tv_usec = (timeout % 1000) * 1000;
    
    FD_ZERO(&tbl);
    FD_SET(fd, &tbl);

    return select(fd + 1, &tbl, 0, 0, &t);
#endif
}

CVMInt32
POSIXnetSocket(CVMInt32 domain, CVMInt32 type, CVMInt32 protocol)
{
    return socket(domain, type, protocol);
}

/*
 * Note: Currently unsure if vxWorks supports getprotobyname(),
 * not even sure if it is POSIX or not. For now we allow Solaris
 * platform purport to HAVE it.
 *
 * If vxWorks has platform specific support for this functionality
 * it should be implemented in src/vxworks/javavm/runtime/net_md.c
*/

#ifdef POSIX_HAVE_GETPROTOBYNAME
struct protoent *
POSIXnetGetProtoByName(char * protoName)
{
    return (struct protoent *)getprotobyname(protoName);
}
#endif

CVMInt32
POSIXnetSetSockOpt(CVMInt32 fd, CVMInt32 type, CVMInt32 dir, const void * arg,
    CVMInt32 argSize)
{
    return setsockopt(fd, type, dir, arg, argSize);
}

CVMInt32 
POSIXnetGetSockOpt(CVMInt32 fd, CVMInt32 proto, CVMInt32 flag, void *in_addr,
    CVMInt32 *inSize)
{
    assert(sizeof(socklen_t) == sizeof(CVMInt32));
    return getsockopt(fd, proto, flag, in_addr, (socklen_t *)inSize);
}

CVMInt32
POSIXnetGetSockName(CVMInt32 fd, struct sockaddr *lclAddr, CVMInt32 *lclSize)
{
    assert(sizeof(socklen_t) == sizeof(CVMInt32));
    return getsockname(fd, lclAddr, (socklen_t *)lclSize);
}

CVMInt32
POSIXnetGetHostName(char *hostname, CVMInt32 maxlen)
{
    return gethostname(hostname, maxlen);
}

CVMInt32
POSIXnetBind(CVMInt32 fd, struct sockaddr *bindAddr, CVMInt32 size)
{
    return bind(fd, bindAddr, size);
}
