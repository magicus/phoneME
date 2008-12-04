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
#ifndef WINCE
	#include "winsock2.h"
#else
	#include <windows.h>
	#include <winsock.h>
#endif
#include "javavm/include/porting/net.h"
#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/io_md.h"
#include <stdio.h>

static WSADATA WSAData;


CVMInt32
CVMnetStartup()
{
    int err = WSAStartup (MAKEWORD(1,1), &WSAData);
    return err;
}

void CVMnetShutdown()
{
    WSACleanup();
}


CVMInt32
CVMnetSocketShutdown(CVMInt32 fd, CVMInt32 howto)
{
    return shutdown(fd, howto);
}

CVMInt32
CVMnetSocketAvailable(CVMInt32 fd, CVMInt32 *pbytes)
{
	if (fd < 0 || ioctlsocket(fd, FIONREAD, pbytes) < 0) {
		return SOCKET_ERROR  ;
	}
    return 0;
}

CVMInt32
CVMnetConnect(CVMInt32 fd, struct sockaddr *him, CVMInt32 len)
{
    return connect(fd, him, len);
}

CVMInt32
CVMnetAccept(CVMInt32 fd, struct sockaddr *him, CVMInt32 *len)
{
    return accept(fd, him, len);
}

#ifdef WINCE
static winpaC=0;

CVMInt32
CVMnetSendTo(CVMInt32 fd, char *buf, CVMInt32 len, CVMInt32 flags,
	     struct sockaddr *to, CVMInt32 tolen)
{
    int c;

    c = sendto(fd, buf, len, flags, to, tolen);
    return c;
}

/* Note: fd should be care */
CVMInt32
CVMnetRecvFrom(CVMInt32 fd, char *buf, CVMInt32 nBytes,
	       CVMInt32 flags, struct sockaddr *from,
	       CVMInt32 *fromlen)
{
    int c = 0;
#ifdef CVM_DEBUG
    CVMThreadID *tid = CVMthreadSelf();
    
    tid->where = 3;
#endif
    c = recvfrom(fd, buf, nBytes, 0, from, fromlen);
#ifdef CVM_DEBUG
    tid->where = 0;
#endif
    return c;
}

#else
CVMInt32
CVMnetSendTo(CVMInt32 fd, char *buf, CVMInt32 len, CVMInt32 flags,
	     struct sockaddr *to, CVMInt32 tolen)
{
    return sendto(fd, buf, len, flags, to, tolen);
}

CVMInt32
CVMnetRecvFrom(CVMInt32 fd, char *buf, CVMInt32 nBytes,
	       CVMInt32 flags, struct sockaddr *from,
	       CVMInt32 *fromlen)
{
    return recvfrom(fd, buf, nBytes, flags, from, fromlen);
}
#endif

CVMInt32
CVMnetListen(CVMInt32 fd, CVMInt32 count)
{
    return listen(fd, count);
}

CVMInt32
CVMnetRecv(CVMInt32 fd, char *buf, CVMInt32 nBytes,
	   CVMInt32 flags)
{
    return recv(fd, buf, nBytes, flags);
}

CVMInt32
CVMnetSend(CVMInt32 fd, char *buf, CVMInt32 nBytes,
	   CVMInt32 flags)
{
    return send(fd, buf, nBytes, flags);
}

CVMInt32
CVMnetTimeout(CVMInt32 fd, CVMInt32 timeout)
{
    fd_set tbl;
    struct timeval t;

#ifndef WINCE
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
	{
		DWORD dwBytesReturned = 0;
		BOOL bNewBehavior = FALSE;
		DWORD status;

		// disable  new behavior using
		// IOCTL: SIO_UDP_CONNRESET
		status = WSAIoctl(fd, SIO_UDP_CONNRESET,
					&bNewBehavior, sizeof(bNewBehavior),
					NULL, 0, &dwBytesReturned,
					NULL, NULL);
	}
#endif

    t.tv_sec = timeout / 1000;
    t.tv_usec = (timeout % 1000) * 1000;
    
    FD_ZERO(&tbl);
    FD_SET(fd, &tbl);

#ifdef CVM_DEBUG
{
    int x;
    CVMthreadSelf()->where = 4;
    x = select(fd + 1, &tbl, 0, 0, &t);
    CVMthreadSelf()->where = 0;
    return x;
}
#else
    return select(fd + 1, &tbl, 0, 0, &t);
#endif
}

CVMInt32
CVMnetSocket(CVMInt32 domain, CVMInt32 type, CVMInt32 protocol)
{
    return socket(domain, type, protocol);
}


struct protoent *
CVMnetGetProtoByName(char * protoName)
{
#ifndef WINCE
    return (struct protoent *)getprotobyname(protoName);
#else
    return NULL;
#endif
}


CVMInt32
CVMnetSetSockOpt(CVMInt32 fd, CVMInt32 type, CVMInt32 dir, const void * arg,
    CVMInt32 argSize)
{
    return setsockopt(fd, type, dir, arg, argSize);
}

CVMInt32 
CVMnetGetSockOpt(CVMInt32 fd, CVMInt32 proto, CVMInt32 flag, void *in_addr,
    CVMInt32 *inSize)
{
    return getsockopt(fd, proto, flag, in_addr, inSize);
}

CVMInt32
CVMnetGetSockName(CVMInt32 fd, struct sockaddr *lclAddr, CVMInt32 *lclSize)
{
    return getsockname(fd, lclAddr, lclSize);
}

CVMInt32
CVMnetGetHostName(char *hostname, CVMInt32 maxlen)
{
    int res = gethostname(hostname, maxlen);
    return res;
}

CVMInt32
CVMnetBind(CVMInt32 fd, struct sockaddr *bindAddr, CVMInt32 size)
{
    CVMInt32 ret = bind(fd, bindAddr, size);
    return ret;
}

CVMInt32
CVMnetSocketClose(CVMInt32 fd)
{
    int ret = closesocket(fd);
    return ret;
}
