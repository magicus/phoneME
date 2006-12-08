/*
 * @(#)socket_md.c	1.2 06/10/10
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


#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "socket_md.h"
#include "sysSocket.h"

static int waitForInput(int fd)
{
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 250000;
    return select(fd + 1, &fds, NULL, NULL, &tv);
}

int
dbgsysListen(int fd, long count) {
    return listen(fd, count);
}

int
dbgsysConnect(int fd, struct sockaddr *name, int namelen) {
    return connect(fd, name, namelen);
}

/*
 * Work around problem where we may block in accept after socket
 * has been closed.
 */
int
dbgsysAccept(int fd, struct sockaddr *name, int *namelen)
{
    int result;
    fcntl(fd, F_SETFL, O_NONBLOCK);
    while (1) {
	result = accept(fd, name, (socklen_t*)namelen);
	if (result == -1 && errno == EAGAIN) {
	    (void) waitForInput(fd);
	} else {
	    break;
	}
    }
    fcntl(fd, F_SETFL, 0);
    return result;
}

int
dbgsysRecvFrom(int fd, char *buf, int nBytes,
                  int flags, struct sockaddr *from, int *fromlen) {
    return recvfrom(fd, buf, nBytes, flags, from, (socklen_t*)fromlen);
}

int
dbgsysSendTo(int fd, char *buf, int len,
                int flags, struct sockaddr *to, int tolen) {
    return sendto(fd, buf, len, flags, to, tolen);
}

int
dbgsysRecv(int fd, char *buf, int nBytes, int flags) {
    return recv(fd, buf, nBytes, flags);
}

int
dbgsysSend(int fd, char *buf, int nBytes, int flags) {
    return send(fd, buf, nBytes, flags);
}

struct hostent *
dbgsysGetHostByName(char *hostname) {
    return gethostbyname(hostname);
}

unsigned short
dbgsysHostToNetworkShort(unsigned short hostshort) {
    return htons(hostshort);
}

int
dbgsysSocket(int domain, int type, int protocol) {
    return socket(domain, type, protocol);
}

int dbgsysSocketClose(int fd)
{
    shutdown(fd, 2);	/* force recv to return */
    return close(fd);
}

int
dbgsysBind(int fd, struct sockaddr *name, int namelen) {
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
    return getsockname(fd, name, (socklen_t*)namelen);
}

unsigned long
dbgsysNetworkToHostLong(unsigned long netlong) {
    return ntohl(netlong);
}


int
dbgsysSetSocketOption(int fd, jint cmd, jboolean on, jvalue value) 
{
    if (cmd == TCP_NODELAY) {
        struct protoent *proto = getprotobyname("TCP");
        int tcp_level = (proto == 0 ? IPPROTO_TCP: proto->p_proto);
        long onl = (long)on;

        if (setsockopt(fd, tcp_level, TCP_NODELAY,
                       (char *)&onl, sizeof(long)) < 0) {
                return SYS_ERR;
        }
    } else if (cmd == SO_LINGER) {
        struct linger arg;
        arg.l_onoff = on;

        if(on) {
            arg.l_linger = (unsigned short)value.i;
            if(setsockopt(fd, SOL_SOCKET, SO_LINGER,
                          (char*)&arg, sizeof(arg)) < 0) {
                return SYS_ERR;
            }
        } else {
            if (setsockopt(fd, SOL_SOCKET, SO_LINGER,
                           (char*)&arg, sizeof(arg)) < 0) {
                return SYS_ERR;
            }
        }
    } else if (cmd == SO_SNDBUF) {
        jint buflen = value.i;
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
                       (char *)&buflen, sizeof(buflen)) < 0) {
            return SYS_ERR;
        }
    } else if (cmd == SO_REUSEADDR) {
        int oni = (int)on;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                       (char *)&oni, sizeof(oni)) < 0) {
            return SYS_ERR;

        }
    } else {
        return SYS_ERR;
    }
    return SYS_OK;
}
