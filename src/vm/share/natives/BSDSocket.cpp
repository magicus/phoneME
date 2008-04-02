/*
 *   
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
 */

#include "incls/_precompiled.incl"
#include "incls/_BSDSocket.cpp.incl"

// Please see PCSLSocket.cpp for the reason of "|| 1" here.
#if !ENABLE_PCSL || 1
/*
 * *** WARNING WARNING WARNING ***
 *
 * This file is an EXAMPLE IMPLEMENTATION of the socket protocol for
 * platforms that supports the BSD socket API. It is NOT supposed to
 * be portable to platforms with other socket APIs. If you are porting
 * to a new platform, please make a copy of this file and modify to
 * suit your platform.
 */

#ifdef __cplusplus
extern "C" {
#endif

#if USE_BSD_SOCKET

#if defined(LINUX) || defined (CYGWIN)
#define USE_UNISTD_SOCKETS 1
#else
#if defined(WIN32) || defined(UNDER_CE)
#define USE_WINSOCK_SOCKETS 1
#else
#error "Unknown sockets type"
#endif
#endif

#if USE_UNISTD_SOCKETS
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#if USE_LIBC_GLUE
// IMPL_NOTE: this is bad idea
#define GET_LAST_ERROR()        0
#else
#define GET_LAST_ERROR()        errno
#endif
#define init_sockets()
#define closesocket(x)          jvm_close(x)
#endif // LINUX

#if USE_WINSOCK_SOCKETS
#undef FIELD_OFFSET
#define WIN32_LEAN_AND_MEAN
#include <winsock.h>

#define GET_LAST_ERROR()        WSAGetLastError()
#define EWOULDBLOCK             WSAEWOULDBLOCK
#ifdef  EINPROGRESS
#undef  EINPROGRESS
#endif
#define EINPROGRESS             WSAEWOULDBLOCK

static void init_sockets() {
  WSADATA WSAData;
  static int sock_initialized = 0;

  // Initialize WinSock.
  if (!sock_initialized) {
    WSAStartup(MAKEWORD(1,1), &WSAData);
    sock_initialized = 1;
  }
}
#endif // defined(WIN32) || defined(UNDER_CE)

#if ENABLE_REMOTE_TRACER

int open_socket(char* hostname, int port, int mode, jboolean nonblock) {
  int sock;
  struct sockaddr_in destination_sin;
  struct hostent* phostent = NULL;

  init_sockets();

  // Create a TCP/IP socket
  if ((sock = jvm_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return -1;
  }

  // Retrieve the host information corresponding to the host name.
  if ((phostent = (struct hostent*)jvm_gethostbyname(hostname)) == NULL) {
    jvm_shutdown(sock, 2);
    // IMPL_NOTE: Linux: still need to close the file descriptor!
    return -1;
  }

  // Assign the socket IP address.
  destination_sin.sin_family = AF_INET;
  destination_sin.sin_port = jvm_htons(port);
  jvm_memcpy((char *)&(destination_sin.sin_addr),
              phostent->h_addr,
              phostent->h_length);

  // Establish a connection to the server.
  if (jvm_connect(sock, (struct sockaddr*)&destination_sin,
                  sizeof(destination_sin)) < 0) {
    jvm_shutdown(sock, 2);
    return -1;
  }

  if (nonblock == KNI_TRUE) {
    u_long flag = 1;

#ifdef USE_UNISTD_SOCKETS
    jvm_fcntl(sock, F_SETFL, O_NONBLOCK);
#else
    if (ioctlsocket(sock, FIONBIO, &flag) < 0) {
      jvm_shutdown(sock, 2);
      return -1;
    }
#endif

  }
  return sock;
}

int socket_send(int sock_fd, char *buf, int size, int offset) {
    return jvm_send(sock_fd, buf + offset, size, 0);
}

#endif // ENABLE_REMOTE_TRACER

#define CHECK_READ      0x01
#define CHECK_WRITE     0x02
#define CHECK_EXCEPTION 0x04

typedef struct {
  int fd;
  int check_flags;     /* Should we check for read/write/exception? */
} SocketOpenParameter;

static jboolean set_blocking_flags(int *fd, jboolean is_blocking) {
#if USE_UNISTD_SOCKETS
  int flags = jvm_fcntl(*fd, F_GETFL);
  if (flags >= 0) {
    if (is_blocking == KNI_TRUE) {
      flags &= ~O_NONBLOCK;
    } else {
      flags |= O_NONBLOCK;
    }
    jvm_fcntl(*fd, F_SETFL, flags);
    if (flags != -1) {
      return true;
    }
  }
  jvm_shutdown(*fd, 2);
  closesocket(*fd);
  *fd = -1;
  return false;
#else
  if (!is_blocking) {
    unsigned long flags = 1; // 1 = enable non-blocking mode
    if (ioctlsocket(*fd, FIONBIO, &flags) < 0) {
      jvm_shutdown(*fd, 2);
      closesocket(*fd);
      *fd = -1;
      return false;
    }
  }
  return true;
#endif
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_open0() {
  init_sockets();

  SocketOpenParameter *p = (SocketOpenParameter*) SNI_GetReentryData(NULL);
  if (p == NULL) {
    p = (SocketOpenParameter*) SNI_AllocateReentryData(sizeof(*p));
    p->fd = -1;

    struct hostent *phostent;
    KNI_StartHandles(1);
    KNI_DeclareHandle(hostname_object);
    KNI_GetParameterAsObject(1, hostname_object);

    // hostname is always NUL terminated. See socket/Protocol.java for detail.
    char *hostname = (char*) SNI_GetRawArrayPointer(hostname_object);
    phostent = (struct hostent*)jvm_gethostbyname(hostname);
    KNI_EndHandles();

    if (phostent == NULL) {
      KNI_ReturnInt(-1);
    }
    struct sockaddr_in destination_sin;
    destination_sin.sin_family = AF_INET;
    int port = KNI_GetParameterAsInt(2);
    destination_sin.sin_port = jvm_htons(port);
    jvm_memcpy((char *) &destination_sin.sin_addr, phostent->h_addr,
                phostent->h_length);

    p->fd = jvm_socket(AF_INET, SOCK_STREAM, 0);
    if (p->fd < 0) {
       KNI_ReturnInt(-1);
    }
    if (!set_blocking_flags(&p->fd, /*is_blocking*/ KNI_FALSE)) {
      KNI_ReturnInt(-1);
    }

    if (jvm_connect(p->fd, (struct sockaddr *) &destination_sin,
                    sizeof(destination_sin)) < 0) {
      int err_code = GET_LAST_ERROR();
      if (err_code == EINPROGRESS) {
        // When the socket is ready for connect, it becomes *writable*
        // (according to BSD socket spec of select())
        p->check_flags = CHECK_WRITE | CHECK_EXCEPTION;
        SNI_BlockThread();
      } else {
        jvm_shutdown(p->fd, 2);
        closesocket(p->fd);
        p->fd = -1;
      }
    }
    KNI_ReturnInt(p->fd);
  } else {
    // When we come to here, a CheckEvent() call has reported one of the
    // following:
    // [1] connect() has succeeded. In this case we return the socket fd.
    // [2] connect() has failed. In this case CheckEvent has already closed
    //     the socket and set p->fd to -1. So we'd be returning -1.
    KNI_ReturnInt(p->fd);
  }
}

struct BlockingSocket {
  int fd;                   /* The socket that returned EWOULDBLOCK */
  int check_flags;          /* Should we check for read/write/exception? */
};

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_readBuf() {
  int result;
  int fd = KNI_GetParameterAsInt(1);
  int offset = KNI_GetParameterAsInt(3);
  int length = KNI_GetParameterAsInt(4);

  KNI_StartHandles(1);
  KNI_DeclareHandle(buffer_object);
  KNI_GetParameterAsObject(2, buffer_object);
  char *buffer = (char *) SNI_GetRawArrayPointer(buffer_object) + offset;

  result = jvm_recv(fd, buffer, length, 0); // We rely on open0() for setting the socket to non-blocking
  KNI_EndHandles();

  if (result == 0) {
    // If remote side has shut down the connection gracefully, and all
    // data has been received, recv() will complete immediately with
    // zero bytes received.
    //
    // This is true for Win32/CE and Linux
    result = -1;
  }
  else if (result < 0) {
    int err_code = GET_LAST_ERROR();
    if (err_code == EWOULDBLOCK) {
      if (SNI_GetReentryData(NULL) == NULL) {
        BlockingSocket *socket =
            (BlockingSocket *)SNI_AllocateReentryData(sizeof(*socket));
        socket->fd = fd;
        socket->check_flags = CHECK_READ;
      }
      SNI_BlockThread();
    }
  }

  KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_readByte() {
  jint result = -1;
  unsigned char byte;

  int fd = KNI_GetParameterAsInt(1);
  int n = jvm_recv(fd, (char*)&byte, 1, 0);

  if (n == 1) {
    result = byte; // do not sign-extend
    GUARANTEE(0 <= result && result <= 255, "no sign extension");
  }
  else if (n == 0) {
    // If remote side has shut down the connection gracefully, and all
    // data has been received, recv() will complete immediately with
    // zero bytes received.
    //
    // This is true for Win32/CE and Linux
    result = -1;
  }
  else {
    int err_code = GET_LAST_ERROR();
    if (err_code == EWOULDBLOCK) {
      if (SNI_GetReentryData(NULL) == NULL) {
        BlockingSocket *socket =
            (BlockingSocket *)SNI_AllocateReentryData(sizeof(*socket));
        socket->fd = fd;
        socket->check_flags = CHECK_READ;
      }
      SNI_BlockThread();
    } else {
      result = -1;
    }
  }

  KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_writeBuf() {
  int result;
  int fd = KNI_GetParameterAsInt(1);
  int offset = KNI_GetParameterAsInt(3);
  int length = KNI_GetParameterAsInt(4);

  KNI_StartHandles(1);
  KNI_DeclareHandle(buffer_object);
  KNI_GetParameterAsObject(2, buffer_object);
  char *buffer = (char *) SNI_GetRawArrayPointer(buffer_object) + offset;

  result = jvm_send(fd, buffer, length, 0); // We rely on open0() for setting the socket to non-blocking
  KNI_EndHandles();

  if (result < 0) {
    int err_code = GET_LAST_ERROR();
    if (err_code == EWOULDBLOCK) {
      if (SNI_GetReentryData(NULL) == NULL) {
        BlockingSocket *socket =
            (BlockingSocket *) SNI_AllocateReentryData(sizeof(*socket));
        socket->fd = fd;
        socket->check_flags = CHECK_WRITE;
      }
      SNI_BlockThread();
    }
  }

  KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_writeByte() {
  int fd = KNI_GetParameterAsInt(1);
  char byte = (char) KNI_GetParameterAsInt(2);

  // We rely on open0() for setting the socket to non-blocking
  int result = jvm_send(fd, &byte, 1, 0);

  if (result < 0) {
    int err_code = GET_LAST_ERROR();
    if (err_code == EWOULDBLOCK) {
      if (SNI_GetReentryData(NULL) == NULL) {
        BlockingSocket *socket =
            (BlockingSocket *)SNI_AllocateReentryData(sizeof(*socket));
        socket->fd = fd;
        socket->check_flags = CHECK_WRITE;
      }
      SNI_BlockThread();
    }
  }

  KNI_ReturnInt(result);
}

void JVMSPI_CheckEvents(JVMSPI_BlockedThreadInfo * blocked_threads,
                        int blocked_threads_count, jlong timeout_milli_seconds)
{
  fd_set read_fds;
  fd_set write_fds;
  fd_set except_fds;
  int i, num_fds, num_ready;

  bool debugger_active = JVM_IsDebuggerActive();
#if ENABLE_JAVA_DEBUGGER
  int dbg_socket_fd = -1;
  if (debugger_active) {
    dbg_socket_fd = JVM_GetDebuggerSocketFd();
  }
#endif

  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);
  FD_ZERO(&except_fds);

  // [1] Gather the FDs that we want to check event for
  num_fds = 1;
  for (i=0; i<blocked_threads_count; i++) {
    BlockingSocket *socket = (BlockingSocket *)blocked_threads[i].reentry_data;
    if (socket->check_flags & CHECK_READ) {
      FD_SET(socket->fd, &read_fds);
    }
    if (socket->check_flags & CHECK_WRITE) {
      FD_SET(socket->fd, &write_fds);
    }
    if (socket->check_flags & CHECK_EXCEPTION) {
      FD_SET(socket->fd, &except_fds);
    }
    if (num_fds < socket->fd) {
      num_fds = socket->fd;
    }
  }

#if ENABLE_JAVA_DEBUGGER
  if (debugger_active) {
    if (dbg_socket_fd != -1) {
      FD_SET(dbg_socket_fd, &read_fds);
      if (num_fds < dbg_socket_fd) {
        num_fds = dbg_socket_fd;
      }
    }
  }
#endif

  // [2] Call select() on the FDs, without appropriate timeout value
  if (timeout_milli_seconds < 0) {
    // Sleep forever until an event happens
    GUARANTEE(blocked_threads_count > 0,
              "can't sleep forever with no event sources!");
    num_ready = jvm_select(num_fds+1, &read_fds, &write_fds, &except_fds,
                           NULL);
  } else if (!debugger_active && blocked_threads_count == 0) {
    // It's better to call Os::sleep than select() with no fds -- WinSock
    // returns immediately with an error status if you don't pass any
    // fds.
    Os::sleep(timeout_milli_seconds);
    num_ready = 0;
  } else {
    jlong sec, usec;
    struct timeval timeout;

    sec  = timeout_milli_seconds / 1000;
    usec = (timeout_milli_seconds % 1000) * 1000;

    if (msw(sec) <= 0) {
      timeout.tv_sec = lsw(sec);
    } else {
      // mmmm, are you sure you want to sleep so long??
      timeout.tv_sec = 0x7fffffff;
    }
    timeout.tv_usec = lsw(usec);

    num_ready = jvm_select(num_fds+1, &read_fds, &write_fds, &except_fds,
                           &timeout);
  }

  // [3] If a thread's FD is ready, change the thread's status to ready
  if (num_ready > 0) {
    for (i = 0; i < blocked_threads_count; i++) {
      BlockingSocket *socket =
          (BlockingSocket *)blocked_threads[i].reentry_data;
      jboolean is_ready = KNI_FALSE;
      if (socket->check_flags & CHECK_READ) {
        if (FD_ISSET(socket->fd, &read_fds)) {
          is_ready = KNI_TRUE;

        }
      }
      if (socket->check_flags & CHECK_WRITE) {
        if (FD_ISSET(socket->fd, &write_fds)) {
          is_ready = KNI_TRUE;
        }
      }
      if (socket->check_flags & CHECK_EXCEPTION) {
        if (FD_ISSET(socket->fd, &except_fds)) {
          // This happens only if a connect() call failed. Let's close
          // the socket and make sure open0() returns -1:
          //
          // Note to QA: this block needs more testing!
          jvm_shutdown(socket->fd, 2);
          closesocket(socket->fd);
          socket->fd = -1;
          is_ready = KNI_TRUE;
        }
      }

      if (is_ready) {
        SNI_UnblockThread(blocked_threads[i].thread_id);
      }
    }
#if ENABLE_JAVA_DEBUGGER
    if (debugger_active) {
      if (FD_ISSET(dbg_socket_fd, &read_fds)) {
        JVM_ProcessDebuggerCmds();
      }
    }
#endif
  }
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_available0() {
  //jint sock = KNI_GetParameterAsInt(1);
  return (jint) 0; // IMPL_NOTE: use select() .....
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_cldc_io_j2me_socket_Protocol_close0() {
  jint sock = KNI_GetParameterAsInt(1);

  // NOTE: this would block the VM. A real implementation should
  // make this a async native method.
  jvm_shutdown(sock, 2);
  closesocket(sock);
}

#endif /* USE_BSD_SOCKET */

#ifdef __cplusplus
}
#endif
#endif /* !ENABLE_PCSL */
