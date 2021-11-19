/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
 */



static int debug = 0;

#ifdef WIN32

#include <windows.h>
#define lime_vsnprintf _vsnprintf

static WSADATA wsaData;
#ifdef USE_WINSOCK32
#define SOCKETS_WORD MAKEWORD(2,2)
#else
#define SOCKETS_WORD MAKEWORD(1,1)
#endif

#define INIT_SOCKETS() WSAStartup(SOCKETS_WORD, &wsaData)

#else

#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>

#endif

#include <assert.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "limeio.h"

typedef struct InternalSocketData {
    int socketID;
    int errorCondition;
    int peekMode;
    int delayMode;
    FILE *fd;
} InternalSocketData;

typedef struct InternalServerSocketData {
    int socketID;
    int errorCondition;
    int port;
} InternalServerSocketData;

#define MALLOC(s) malloc(s)
#define FREE(p) free(p)

static void reportErrorState(void) {
#ifdef WIN32
        {
            int errorCode = WSAGetLastError();
            if (errorCode != 0) {
                fprintf(stderr, "Windows error code %i\n", errorCode);
            }
        }
#endif
}

static void error(const char *s, ...) {
  va_list args;
  va_start(args, s);
  vfprintf(stderr, s, args);
  va_end(args);
  reportErrorState();
}

static Socket *socket_alloc(void) {
    Socket *s = (Socket *) MALLOC( sizeof(Socket) );
    InternalSocketData *d = (InternalSocketData *)
        MALLOC( sizeof(InternalSocketData) );
    if (!s || !d) {
        if (s) FREE(s);
        error("Cannot allocate memory for new socket object\n");
        return 0;
    }
    s->data = d;
    d->errorCondition = 0;
    d->peekMode = 0;
    d->delayMode = 1; /* this is the OS default */
    return s;
}

static void socket_clearError(Socket *s) {
    s->data->errorCondition = 0;
}

/* A macro to speed up checking of socket data */
#define socket_checkError(s) { if (s->data->errorCondition) socket_error(s); }

static void socket_error(Socket *s) {
    int state = s->data->errorCondition;
    if (state) {
        error("Error condition %i on socket %i\n", state, s->data->socketID);
        exit(1);
    }
}

static int socket_getError(Socket *s) {
    return s->data->errorCondition;
}

static void socket_setError(Socket *s, int state) {
    s->data->errorCondition = state;
}

static void serversocket_clearError(ServerSocket *s) {
    s->data->errorCondition = 0;
}

static void serversocket_checkError(ServerSocket *s) {
    int state = s->data->errorCondition;
    if (state) {
        error("Error condition %i on server socket %i\n",
              state, s->data->socketID);
        exit(1);
    }
}

static int serversocket_getError(ServerSocket *s) {
    return s->data->errorCondition;
}

static void serversocket_setError(ServerSocket *s, int state) {
    s->data->errorCondition = state;
}

void DeleteSocket(Socket *s) {
    assert(s != NULL);
    assert(s->data != NULL);
    FREE(s->data);
    FREE(s);
}

static void socket_setDelayMode(Socket *s, int delay) {
    if (delay != s->data->delayMode) {
        int flag = ! delay;
        assert(s != NULL);
        setsockopt(s->data->socketID, IPPROTO_TCP, TCP_NODELAY,
                   (char *) &flag, sizeof(flag));
        s->data->delayMode = delay;
    }
}

static int socket_open(Socket *s, int port, const char *host) {
    struct sockaddr_in sa;
    assert(s != NULL);
    assert (port > 0);
    socket_checkError(s);

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons( (uint16_t) port);
    sa.sin_addr.s_addr = inet_addr(host);

    if (connect(s->data->socketID, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
        error("Cannot connect socket to port %i\n", port);
        socket_setError(s, 1);
        return 1;
    }

#ifndef WIN32	
    /* Get a file descriptor for the socket */
    s->data->fd = fdopen(s->data->socketID, "w+");
#endif

    /* Turn off the Nagle algorithm */
    socket_setDelayMode(s, 0);

    return 0;
}

static int socket_close(Socket *s) {
    int rc;
    assert(s != NULL);
    socket_checkError(s);
    rc = close(s->data->socketID);
    socket_setError(s, rc);
    return rc;
}

static int socket_read(Socket *s, char *buffer, int buflen, int *bytes) {
    int bytesRead;
    int peekFlag;
    assert(s != NULL);
	if (buflen == 0) {
		/* No data to read */
		*bytes = 0;
		return 0;
	}
    assert(buffer != NULL);
    assert(buflen > 0);
    socket_checkError(s);
    peekFlag = (s->data->peekMode) ? MSG_PEEK : 0;
    bytesRead = recv(s->data->socketID, buffer, buflen, peekFlag);
    if (bytesRead < 0) {
        socket_setError(s, bytesRead);
        return bytesRead;
    } else {
        if (bytes != NULL) {
            *bytes = bytesRead;
        }
        return 0;
    }
}

static int socket_readn(Socket *s, char *buffer, int length) {
    while (length != 0) {
    int readBytes = 0;
    int rc = s->read(s, buffer, length, &readBytes);
    if (rc) return rc;
    length -= readBytes;
    buffer += readBytes;
    }
    return 0;
}

static int socket_write(Socket *s, char *buffer, int bytes) {
    int bytesWritten;
    socket_checkError(s);
    bytesWritten = send(s->data->socketID, buffer, bytes, 0);
    assert(s != NULL);
    assert(buffer != NULL);
    assert(bytes > 0);
    if (bytesWritten < 0) {
        socket_setError(s, errno);
        return bytesWritten;
    } else {
        return 0;
    }
}

static int socket_printf(Socket *s, char *format, ...) {
    char buffer[1024];
    va_list args;
    assert(s != NULL);
    assert(format != NULL);
    va_start(args, format);
    lime_vsnprintf(buffer, 1024, format, args);
    va_end(args);
    return s->write(s, buffer, strlen(buffer));
}

static int writeInt(Socket *s, u_long value) {
    assert(s != NULL);
    return s->write(s, (char *) &value, sizeof(u_long));
}

static int readInt(Socket *s, u_long *result) {
    assert(s != NULL);
    return socket_readn(s, (char *) result, sizeof(u_long));
}

/** Extra optimization here, since it is the most frequently called
 * socket function */
static int socket_writeInt32(Socket *s, int32_t value) {
    int rc;
    assert(s != NULL);
    socket_checkError(s);
    value = (int32_t) htonl( * (u_long *) &value);
    rc = send(s->data->socketID, (char *) &value, 4, 0);
    if (rc < 0) {
        socket_setError(s, errno);
        return rc;
    } else {
        return 0;
    }
}

static int socket_readInt32(Socket *s, int32_t *result) {
  u_long l = 12345678;
    assert(s != NULL);
    assert(result != NULL);
  if (readInt(s, &l)) {
    return 1;
  } else {
    * (u_long *) result = ntohl(l);
    return 0;
  }
}

static int socket_readLong64(Socket *s, long64 *result) {
    u_long l = 12345678, h = 12345678;
    assert(s != NULL);
    assert(result != NULL);

    if (readInt(s, &h)) return 1;
    
    if (readInt(s, &l)) return 1;
    
    (*result) = ((long64)ntohl(h) << 32) | ((long64)ntohl(l));

    return 0;
}


static int socket_writeLong64(Socket *s, long64 value) {
  int32_t high;
  uint32_t low;
    assert(s != NULL);
  high = (int32_t) (value >> 32);
  low = (int32_t) (value & 0xffffffffUL);
  low = htonl(low);
  socket_writeInt32(s, high);
  return writeInt(s, low);
}

#ifndef WIN32
static int socket_flush(Socket *s) {
  int rc;
  socket_checkError(s);
    assert(s != NULL);
    rc = fflush(s->data->fd);
    if (rc==EOF) {
    return 1;
    } else {
    return 0;
  }
}

static int socket_ready(Socket *s) {
  struct timeval tv;
  fd_set fds;
  assert(s != NULL);
  socket_checkError(s);
  tv.tv_sec = tv.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(s->data->socketID, &fds);
  select(s->data->socketID+1, &fds, 0, 0, &tv);
  return FD_ISSET(s->data->socketID, &fds);
}

static int socket_block(Socket *s) {
    fd_set fds;
    assert(s != NULL);
    socket_checkError(s);
    FD_ZERO(&fds);
    FD_SET(s->data->socketID, &fds);
    if (select(s->data->socketID+1, &fds, 0, 0, NULL) >= 0) {
    return 0;
    } else {
    socket_setError(s, errno);
    return errno;
    }
}
#endif

static void socket_setPeekMode(Socket *s, int mode) {
    s->data->peekMode = mode;
}

static void socket_initPointers(Socket *s) {
    s->open = socket_open;
    s->close = socket_close;
    s->read = socket_read;
    s->write = socket_write;
    s->printf = socket_printf;
#ifndef WIN32
    s->flush = socket_flush;
    s->ready = socket_ready;
    s->block = socket_block;
#endif
    s->readInt32 = socket_readInt32;
    s->readLong64 = socket_readLong64;
    s->writeInt32 = socket_writeInt32;
    s->writeLong64 = socket_writeLong64;
    s->getError = socket_getError;
    s->clearError = socket_clearError;
    s->setPeekMode = socket_setPeekMode;
    s->setDelayMode = socket_setDelayMode;
}

static int serversocket_listen(ServerSocket *s, int port) {
    struct sockaddr_in sa;
    int rc;
    assert(s != NULL);
    assert (port >= 0);
    serversocket_checkError(s);
    
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons( (uint16_t) port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    
    rc = bind(s->data->socketID, (struct sockaddr *) &sa, sizeof(sa));
    if (rc) {
        error("Cannot bind socket to port %i\n", port);
        serversocket_setError(s, rc);
        return 1;
    }
    if (debug) {
        printf("LimeIO: Bound socket %i to port %i\n",
               s->data->socketID, port);
    }

    rc = listen(s->data->socketID, 10); /* backlog of 10 */
    if (rc) {
        error("Cannot use socket %i as a server socket\n",
              s->data->socketID);
        serversocket_setError(s, 1);
        return 1;
    }

    if (debug) {
        printf("LimeIO: Using socket %i as a server\n", s->data->socketID);
    }
    
    s->data->port = port;

    return 0;
}

static int serversocket_listenAny(ServerSocket *s, int *port) {
    struct sockaddr_in sa;
    size_t len;
    int rc;

    rc = serversocket_listen(s, 0);
    if (rc) {
        return rc;
    }

    memset(&sa, 0, sizeof(sa));
    len = sizeof(sa);
    rc = getsockname(s->data->socketID, (struct sockaddr *) &sa, &len);
    if (rc) {
        error("Error calling getsockname on socket %i\n", s->data->socketID);
        serversocket_setError(s, 1);
        return rc;
    }
    s->data->port = ntohs(sa.sin_port);
    *port = s->data->port;
    if (debug) {
        printf("LimeIO: Resolved port number as %i\n", *port);
    }
    return 0;
}

static int serversocket_accept(ServerSocket *ss, Socket **sp) {
    Socket *s;
    int sid;
    
    serversocket_checkError(ss);

    if (debug) {
        printf("LimeIO: Waiting for connection on socket %i, port %i\n",
               ss->data->socketID, ss->data->port);
    }

    sid = accept(ss->data->socketID, NULL, NULL);
    if (sid <= 0) {
        error("LimeIO: Cannot accept an incoming connection on server socket"
              "%i\n", ss->data->socketID);
        serversocket_setError(ss, 1);
        return 1;
    }

    if (debug) {
        printf("LimeIO: Accepted connection on socket %i, port %i\n",
               ss->data->socketID, ss->data->port);
    }

    s = socket_alloc();
    if (s == NULL) {
        serversocket_setError(ss, 1);
        return 1;
    }
    
    s->data->socketID = sid;
    socket_initPointers(s);

    /* Turn off the Nagle algorithm */
    {
        int flag = 1;
        setsockopt(s->data->socketID, IPPROTO_TCP, TCP_NODELAY,
                   (char *) &flag, sizeof(flag));
    }

    *sp = s;
    return 0;
}

static int serversocket_close(ServerSocket *s) {
    int rc;
    assert(s != NULL);
    serversocket_checkError(s);
    rc = close(s->data->socketID);
    serversocket_setError(s, rc);
    return rc;
}

static void serversocket_initPointers(ServerSocket *s) {
    s->listen = serversocket_listen;
    s->listenAny = serversocket_listenAny;
    s->accept = serversocket_accept;
    s->close = serversocket_close;
    s->getError = serversocket_getError;
    s->clearError = serversocket_clearError;
}

Socket *NewSocket(void) {
    Socket *s = socket_alloc();
    InternalSocketData *d;
    if (s == 0) {
        return NULL;
    }
    d = s->data;

#ifdef WIN32
    if (INIT_SOCKETS()) {
        error("Cannot initialize socket system\n");
        return 0;
    }
#endif

    d->socketID = socket(AF_INET, SOCK_STREAM, 0);
    if (d->socketID < 0) {
        FREE(s);
        FREE(d);
        error("Cannot create a new socket (rc=%i)\n",
              d->socketID);
        return 0;
    }
    socket_initPointers(s);

    return s;
}

ServerSocket *NewServerSocket(void) {
    ServerSocket *s = (ServerSocket *) MALLOC( sizeof(ServerSocket) );
    InternalServerSocketData *d = (InternalServerSocketData *)
        MALLOC( sizeof(InternalServerSocketData) );

#ifdef WIN32
    if (INIT_SOCKETS()) {
        error("Cannot initialize socket system\n");
        return 0;
    }
#endif

    d->socketID = socket(AF_INET, SOCK_STREAM, 0);
    if (d->socketID < 0) {
        FREE(s);
        FREE(d);
        error("Cannot create a new server socket (rc=%i)\n",
              d->socketID);
        return 0;
    }
    d->errorCondition = 0;
    d->port = 0;
    s->data = d;

    serversocket_initPointers(s);

    return s;
}

void DeleteServerSocket(ServerSocket *s) {
    assert(s != NULL);
    assert(s->data != NULL);
    FREE(s->data);
    FREE(s);
}

