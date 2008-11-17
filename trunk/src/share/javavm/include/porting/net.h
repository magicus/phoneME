/*
 * @(#)net.h	1.19 06/10/10
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

#ifndef _INCLUDED_PORTING_NET_H
#define _INCLUDED_PORTING_NET_H

#include "javavm/include/porting/defs.h"

/* This is brought over from the JDK 1.2's HPI with minimal
   conversion. The major assumption here is that the host platform
   supports Berkeley sockets. However, this is more or less true and
   was assumed in the Win32 port of the HPI.
   
*/

/*
 * Many of these interfaces are based on common standard interfaces.
 * In particular, the Single UNIX Specification available from
 * The Open Group at www.opengroup.org will be referred to where
 * appropriate.
 */

struct sockaddr;

/*
 * Close the socket associated with the file descriptor
 * Corresponds to X/Open "close"
 */
CVMInt32 CVMnetSocketClose(CVMInt32 fd);

/*
 * Shutdown either the send, receive, or both sides of
 * a socket, based on the value of the second argument.
 * Corresponds to X/Open "shutdown"
 */
CVMInt32 CVMnetSocketShutdown(CVMInt32 fd, CVMInt32 howto);

/*
 * For the given socket, gets the number of bytes available
 * for reading and stores the result into the value pointed
 * to by the second argument.  The result should satisfy the
 * requirements of java.net.SocketInputStream.available().
 */
CVMInt32 CVMnetSocketAvailable(CVMInt32 fd, CVMInt32 *pbytes);

/*
 * Corresponds to X/Open "connect" in <sys/socket.h>
 */
CVMInt32 CVMnetConnect(CVMInt32 fd, struct sockaddr *him, CVMInt32 len);

/*
 * Corresponds to X/Open "accept" in <sys/socket.h>
 */
CVMInt32 CVMnetAccept(CVMInt32 fd, struct sockaddr *him, CVMInt32 *len);

/*
 * Corresponds to X/Open "sendto" in <sys/socket.h>
 */
CVMInt32 CVMnetSendTo(CVMInt32 fd, char *buf, CVMInt32 len, CVMInt32 flags,
		      struct sockaddr *to, CVMInt32 tolen);
/*
 * Corresponds to X/Open "recvfrom" in <sys/socket.h>
 */
CVMInt32 CVMnetRecvFrom(CVMInt32 fd, char *buf, CVMInt32 nBytes,
			CVMInt32 flags, struct sockaddr *from,
			CVMInt32 *fromlen);
/*
 * Corresponds to X/Open "listen" in <sys/socket.h>
 */
CVMInt32 CVMnetListen(CVMInt32 fd, CVMInt32 count);

/*
 * Corresponds to X/Open "recv" in <sys/socket.h>
 */
CVMInt32 CVMnetRecv(CVMInt32 fd, char *buf, CVMInt32 nBytes, CVMInt32 flags);

/*
 * Corresponds to X/Open "send" in <sys/socket.h>
 */
CVMInt32 CVMnetSend(CVMInt32 fd, char *buf, CVMInt32 nBytes, CVMInt32 flags);

/*
 * Used to timeout blocking operations, as defined by
 * java.net.SocketOptions.SO_TIMEOUT.  Returns
 * immediately if an "accept", "recv", or "recvfrom"
 * operation would succeed without blocking,
 * otherwise blocks for up to "timeout" milliseconds.
 * Corresponds to X/Open "poll" with the "POLLIN"
 * flag set.
 */
CVMInt32 CVMnetTimeout(CVMInt32 fd, CVMInt32 timeout); 

/*
 * Corresponds to X/Open "socket" in <sys/socket.h>
 */
CVMInt32 CVMnetSocket(CVMInt32 domain, CVMInt32 type, CVMInt32 protocol);

/*
 * Corresponds to X/Open "getprotobyname" in <netdb.h>
 */
struct protoent * CVMnetGetProtoByName(char * protoName);

/*
 * Corresponds to X/Open "setsockopt" in <sys/socket.h>
 */
CVMInt32 CVMnetSetSockOpt(CVMInt32 fd, CVMInt32 type, CVMInt32 dir, 
			  const void * arg, CVMInt32 argSize);

/*
 * Corresponds to X/Open "getsockopt" in <sys/socket.h>
 */
CVMInt32 CVMnetGetSockOpt(CVMInt32 fd, CVMInt32 proto, CVMInt32 flag,
			  void *in_addr, CVMInt32 *inSize);

/*
 * Corresponds to X/Open "getsockname" in <sys/socket.h>
 */
CVMInt32 CVMnetGetSockName(CVMInt32 fd, struct sockaddr *lclAddr, 
			   CVMInt32 *lclSize);

/*
 * Corresponds to X/Open "gethostname" in <unistd.h>
 */
CVMInt32 CVMnetGetHostName(char *hostname, CVMInt32 maxlen);

/*
 * Corresponds to X/Open "bind" in <sys/socket.h>
 */
CVMInt32 CVMnetBind(CVMInt32 fd, struct sockaddr *bindAddr, CVMInt32 size);


#include CVM_HDR_NET_H

#endif /* _INCLUDED_PORTING_NET_H */
