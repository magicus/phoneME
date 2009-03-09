/*
 * @(#)sysSocket.h	1.12 06/10/26
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
#ifndef _JAVASOFT_WIN32_SOCKET_MD_H

#include <jni.h>
#include "sys.h"
#include "socket_md.h"

#ifdef _LP64
typedef unsigned int    U_SOCKINT32;
typedef int             SOCKINT32;
#else /* _LP64 */
typedef unsigned long   U_SOCKINT32;
typedef long            SOCKINT32;
#endif /* _LP64 */

#define DBG_POLLIN		1
#define DBG_POLLOUT		2

#define DBG_EINPROGRESS		-150
#define DBG_ETIMEOUT		-200

int dbgsysSocketClose(int fd);
SOCKINT32 dbgsysSocketAvailable(int fd, SOCKINT32 *pbytes);
int dbgsysConnect(int fd, struct sockaddr *him, int len);
int dbgsysFinishConnect(int fd, long timeout);
int dbgsysAccept(int fd, struct sockaddr *him, int *len);
int dbgsysSendTo(int fd, char *buf, int len, int flags, struct sockaddr *to,
	      int tolen);
int dbgsysRecvFrom(int fd, char *buf, int nbytes, int flags,
                struct sockaddr *from, int *fromlen);
int dbgsysListen(int fd, SOCKINT32 count);
int dbgsysRecv(int fd, char *buf, int nBytes, int flags);
int dbgsysSend(int fd, char *buf, int nBytes, int flags);
int dbgsysTimeout(int fd, SOCKINT32 timeout); 
struct hostent *dbgsysGetHostByName(char *hostname);
int dbgsysSocket(int domain, int type, int protocol);
int dbgsysBind(int fd, struct sockaddr *name, int namelen);
int dbgsysSetSocketOption(int fd, jint cmd, jboolean on, jvalue value);
U_SOCKINT32 dbgsysInetAddr(const char* cp);
U_SOCKINT32 dbgsysHostToNetworkLong(U_SOCKINT32 hostlong);
unsigned short dbgsysHostToNetworkShort(unsigned short hostshort);
U_SOCKINT32 dbgsysNetworkToHostLong(U_SOCKINT32 netlong);
unsigned short dbgsysNetworkToHostShort(unsigned short netshort);
int dbgsysGetSocketName(int fd, struct sockaddr *him, int *len);
int dbgsysConfigureBlocking(int fd, jboolean blocking);
int dbgsysPoll(int fd, jboolean rd, jboolean wr, long timeout);
int dbgsysGetLastIOError(char *buf, jint size);
long dbgsysCurrentTimeMillis();

int dbgsysInit(JavaVM *jvm);

/*
 * TLS support
 */
int dbgsysTlsAlloc();
void dbgsysTlsFree(int index);
void dbgsysTlsPut(int index, void *value);
void* dbgsysTlsGet(int index);

#endif


