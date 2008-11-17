/*
 * @(#)net.h	1.6 06/10/10
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

#ifndef _POSIX_NET_H
#define _POSIX_NET_H

CVMInt32 POSIXnetSocketClose(CVMInt32 fd);
#ifndef CVMnetSocketClose
#define CVMnetSocketClose POSIXnetSocketClose
#endif

CVMInt32 POSIXnetSocketShutdown(CVMInt32 fd, CVMInt32 howto);
#ifndef CVMnetSocketShutdown
#define CVMnetSocketShutdown POSIXnetSocketShutdown
#endif

CVMInt32 POSIXnetSocketAvailable(CVMInt32 fd, CVMInt32 *pbytes);
#ifndef CVMnetSocketAvailable
#define CVMnetSocketAvailable POSIXnetSocketAvailable
#endif

CVMInt32 POSIXnetConnect(CVMInt32 fd, struct sockaddr *him, CVMInt32 len);
#ifndef CVMnetConnect
#define CVMnetConnect POSIXnetConnect
#endif

CVMInt32 POSIXnetAccept(CVMInt32 fd, struct sockaddr *him, CVMInt32 *len);
#ifndef CVMnetAccept
#define CVMnetAccept POSIXnetAccept
#endif

CVMInt32 POSIXnetSendTo(CVMInt32 fd, char *buf, CVMInt32 len, CVMInt32 flags,
			struct sockaddr *to, CVMInt32 tolen);
#ifndef CVMnetSendTo
#define CVMnetSendTo POSIXnetSendTo
#endif

CVMInt32 POSIXnetRecvFrom(CVMInt32 fd, char *buf, CVMInt32 nBytes,
			  CVMInt32 flags, struct sockaddr *from,
			  CVMInt32 *fromlen);
#ifndef CVMnetRecvFrom
#define CVMnetRecvFrom POSIXnetRecvFrom
#endif

CVMInt32 POSIXnetListen(CVMInt32 fd, CVMInt32 count);
#ifndef CVMnetListen
#define CVMnetListen POSIXnetListen
#endif

CVMInt32 POSIXnetRecv(CVMInt32 fd, char *buf, CVMInt32 nBytes, CVMInt32 flags);
#ifndef CVMnetRecv
#define CVMnetRecv POSIXnetRecv
#endif

CVMInt32 POSIXnetSend(CVMInt32 fd, char *buf, CVMInt32 nBytes, CVMInt32 flags);
#ifndef CVMnetSend
#define CVMnetSend POSIXnetSend
#endif

CVMInt32 POSIXnetTimeout(CVMInt32 fd, CVMInt32 timeout);
#ifndef CVMnetTimeout
#define CVMnetTimeout POSIXnetTimeout
#endif

CVMInt32 POSIXnetSocket(CVMInt32 domain, CVMInt32 type, CVMInt32 protocol);
#ifndef CVMnetSocket
#define CVMnetSocket POSIXnetSocket
#endif

struct protoent * POSIXnetGetProtoByName(char * protoName);
#ifndef CVMnetGetProtoByName
#define CVMnetGetProtoByName POSIXnetGetProtoByName
#endif

CVMInt32 POSIXnetSetSockOpt(CVMInt32 fd, CVMInt32 type, CVMInt32 dir,
			    const void * arg, CVMInt32 argSize);
#ifndef CVMnetSetSockOpt
#define CVMnetSetSockOpt POSIXnetSetSockOpt
#endif

CVMInt32 POSIXnetGetSockOpt(CVMInt32 fd, CVMInt32 proto, CVMInt32 flag,
			    void *in_addr, CVMInt32 *inSize);
#ifndef CVMnetGetSockOpt
#define CVMnetGetSockOpt POSIXnetGetSockOpt
#endif

CVMInt32 POSIXnetGetSockName(CVMInt32 fd, struct sockaddr *lclAddr,
			     CVMInt32 *lclSize);
#ifndef CVMnetGetSockName
#define CVMnetGetSockName POSIXnetGetSockName
#endif

CVMInt32 POSIXnetGetHostName(char *hostname, CVMInt32 maxlen);
#ifndef CVMnetGetHostName
#define CVMnetGetHostName POSIXnetGetHostName
#endif

CVMInt32 POSIXnetBind(CVMInt32 fd, struct sockaddr *bindAddr, CVMInt32 size);
#ifndef CVMnetBind
#define CVMnetBind POSIXnetBind
#endif

#endif /* _POSIX_NET_H */
