/*
 * @(#)socket_md.c	1.16 06/10/10
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

#include "socket_md.h"
#include <sockLib.h>
#include <resolvLib.h>
#include "sysSocket.h"

int
dbgsysInit(JavaVM *jvm)
{
    return 0;
}

int
dbgsysListen(int fd, long count)
{
    if (fd < 0 || fd >= FD_SETSIZE) {
	return EBADF;
    }
    return listen(fd, count);
}

int
dbgsysConnect(int fd, struct sockaddr *name, int namelen)
{
    if (fd < 0 || fd >= FD_SETSIZE) {
	return EBADF;
    }
    return  connect(fd, name, namelen);
}

int
dbgsysAccept(int fd, struct sockaddr *name, int *namelen)
{
    if (fd < 0 || fd >= FD_SETSIZE) {
	return EBADF;
    }
    return accept(fd, name, namelen);
}

int
dbgsysRecvFrom(int fd, char *buf, int nBytes,
                  int flags, struct sockaddr *from, int *fromlen)
{
    if (fd < 0 || fd >= FD_SETSIZE) {
	return EBADF;
    }
    return recvfrom(fd, buf, nBytes, flags, from, fromlen);
}

int
dbgsysSendTo(int fd, char *buf, int len,
                int flags, struct sockaddr *to, int tolen)
{
    if (fd < 0 || fd >= FD_SETSIZE) {
	return EBADF;
    }
    return sendto(fd, buf, len, flags, to, tolen);
}

int
dbgsysRecv(int fd, char *buf, int nBytes, int flags)
{
    if (fd < 0 || fd >= FD_SETSIZE) {
	return EBADF;
    }
    return recv(fd, buf, nBytes, flags);
}

int
dbgsysSend(int fd, char *buf, int nBytes, int flags)
{
    if (fd < 0 || fd >= FD_SETSIZE) {
	return EBADF;
    }
    return send(fd, buf, nBytes, flags);
}

struct hostent *
dbgsysGetHostByName(char *hostname) {
    /* Caution: not thread safe (but neither is gethostbyname on Solaris...) */
    static struct hostent h;
    static int ipAddr = 0;

    ipAddr = hostGetByName(hostname);

    /* fake a hostent for this IP address. The caller only uses h_addr_list */
    h.h_name = hostname;
    h.h_aliases = NULL;
    h.h_addrtype = AF_INET;
    h.h_length = sizeof(int);
    h.h_addr_list = (char *)&ipAddr;

    return &h;
}

unsigned short
dbgsysHostToNetworkShort(unsigned short hostshort) {
    return htons(hostshort);
}

int
dbgsysSocket(int domain, int type, int protocol)
{
    return socket(domain, type, protocol);
}

int dbgsysSocketClose(int fd)
{
    if (fd < 0 || fd >= FD_SETSIZE) {
	return EBADF;
    }
    return close(fd);
}

int
dbgsysBind(int fd, struct sockaddr *name, int namelen)
{
    if (fd < 0 || fd >= FD_SETSIZE) {
	return EBADF;
    }
    return bind(fd, name, namelen);
}

unsigned long
dbgsysHostToNetworkLong(unsigned long hostlong) {
    return htonl(hostlong);
}

unsigned short
dbgsysNetworkToHostShort(unsigned short netshort) {
    return ntohs(netshort);
}

int
dbgsysGetSocketName(int fd, struct sockaddr *name, int *namelen) {
    return getsockname(fd, name, namelen);
}

unsigned long
dbgsysNetworkToHostLong(unsigned long netlong) {
    return ntohl(netlong);
}


int
dbgsysSetSocketOption(int fd, jint cmd, jboolean on, jvalue value) 
{
    if (fd < 0 || fd >= FD_SETSIZE) {
	return EBADF;
    }
    if (cmd == TCP_NODELAY) {
	/*
        struct protoent *proto = getprotobyname("TCP");
        int tcp_level = (proto == 0 ? IPPROTO_TCP: proto->p_proto);
	*/
	
	int tcp_level = IPPROTO_TCP;
        long onl = (long)on;

        if (setsockopt(fd, tcp_level, TCP_NODELAY,
                       (char *)&onl, sizeof(long)) < 0) {
                return -1;
        }
    } else if (cmd == SO_LINGER) {
        struct linger arg;
        arg.l_onoff = on;

        if(on) {
            arg.l_linger = (unsigned short)value.i;
            if(setsockopt(fd, SOL_SOCKET, SO_LINGER,
                          (char*)&arg, sizeof(arg)) < 0) {
                return -1;
            }
        } else {
            if (setsockopt(fd, SOL_SOCKET, SO_LINGER,
                           (char*)&arg, sizeof(arg)) < 0) {
                return -1;
            }
        }
    } else if (cmd == SO_SNDBUF) {
        jint buflen = value.i;
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
                       (char *)&buflen, sizeof(buflen)) < 0) {
            return -1;
        }
    } else if (cmd == SO_REUSEADDR) {
        int oni = (int)on;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                       (char *)&oni, sizeof(oni)) < 0) {
            return -1;

        }
    } else {
        return -1;
    }
    return 0;
}
