/*
 * @(#)sysSocket.h	1.10 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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
#ifndef _JAVASOFT_WIN32_SOCKET_MD_H

#include <jni.h>
#include "sys.h"
#include "socket_md.h"

int dbgsysSocketClose(int fd);
long dbgsysSocketAvailable(int fd, long *pbytes);
int dbgsysConnect(int fd, struct sockaddr *him, int len);
int dbgsysAccept(int fd, struct sockaddr *him, int *len);
int dbgsysSendTo(int fd, char *buf, int len, int flags, struct sockaddr *to,
	      int tolen);
int dbgsysRecvFrom(int fd, char *buf, int nbytes, int flags,
                struct sockaddr *from, int *fromlen);
int dbgsysListen(int fd, long count);
int dbgsysRecv(int fd, char *buf, int nBytes, int flags);
int dbgsysSend(int fd, char *buf, int nBytes, int flags);
int dbgsysTimeout(int fd, long timeout); 
struct hostent *dbgsysGetHostByName(char *hostname);
int dbgsysSocket(int domain, int type, int protocol);
int dbgsysBind(int fd, struct sockaddr *name, int namelen);
int dbgsysSetSocketOption(int fd, jint cmd, jboolean on, jvalue value);
unsigned long dbgsysHostToNetworkLong(unsigned long hostlong);
unsigned short dbgsysHostToNetworkShort(unsigned short hostshort);
unsigned long dbgsysNetworkToHostLong(unsigned long netlong);
unsigned short dbgsysNetworkToHostShort(unsigned short netshort);
int dbgsysGetSocketName(int fd, struct sockaddr *him, int *len);

#endif


