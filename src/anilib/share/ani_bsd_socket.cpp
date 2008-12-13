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
#include "anilib_impl.h"

/*
 * *** WARNING WARNING WARNING ***
 *
 * This file is an EXAMPLE IMPLEMENTATION of the socket protocol for
 * platforms that supports the BSD socket API. It is NOT supposed to
 * be portable to platforms with other socket APIs. If you are porting
 * to a new platform, please make a copy of this file and modify to
 * suit your platform.
 */

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

#define GET_LAST_ERROR()        errno
#define init_sockets()
#define closesocket(x)          close(x)
#endif

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
#endif

#define CHECK_READ      0x01
#define CHECK_WRITE     0x02
#define CHECK_EXCEPTION 0x04

typedef struct _SocketOpenParameter {
  int fd;
  int check_flags;     /* Should we check for read/write/exception? */
  struct sockaddr_in destination_sin;
} SocketOpenParameter;

static jboolean set_blocking_flags(int *fd, jboolean is_blocking) {
#if USE_UNISTD_SOCKETS
  int flags = fcntl(*fd, F_GETFL);
  if (flags >= 0) {
    if (is_blocking == KNI_TRUE) {
      flags &= ~O_NONBLOCK;
    } else {
      flags |= O_NONBLOCK;
    }
    fcntl(*fd, F_SETFL, flags);
    if (flags != -1) {
      return KNI_TRUE;
    }
  }
  shutdown(*fd, 2);
  closesocket(*fd);
  *fd = -1;
  return KNI_FALSE;
#else
  if (!is_blocking) {
    unsigned long flags = 1; // 1 = enable non-blocking mode
    if (ioctlsocket(*fd, FIONBIO, &flags) < 0) {
      shutdown(*fd, 2);
      closesocket(*fd);
      *fd = -1;
      return KNI_FALSE;
    }
  }
  return KNI_TRUE;
#endif
}

static jboolean
asynchronous_connect_socket(void* parameter, jboolean is_non_blocking) {
  SocketOpenParameter *p = (SocketOpenParameter *)(parameter);
  if (connect(p->fd, (struct sockaddr *) &p->destination_sin,
              sizeof(p->destination_sin)) != 0) {
    shutdown(p->fd, 2);
    closesocket(p->fd);
    p->fd = -1;
  }
  return KNI_TRUE;
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_open0() {
  SocketOpenParameter *p;
  struct hostent *phostent;
  char *hostname;
  int port;
  int result;

  init_sockets();

  if (!ANI_Start()) {
    ANI_Wait();
    KNI_ReturnInt(-1);
  }

  p = (SocketOpenParameter *)(ANI_GetParameterBlock(NULL));
  if (p == NULL) {
    p = (SocketOpenParameter *)
        (ANI_AllocateParameterBlock(sizeof(SocketOpenParameter)));
    p->fd = -1;

    KNI_StartHandles(1);
    KNI_DeclareHandle(hostname_object);
    KNI_GetParameterAsObject(1, hostname_object);

    // hostname is always NUL terminated. See socket/Protocol.java for detail.
    hostname = (char *)(SNI_GetRawArrayPointer(hostname_object));
    // 'gethostbyname()' is NON-REENTRANT and its result is in global memory!
    // => its call must not be moved to 'asynchronous_connect_socket()'!
    phostent = gethostbyname(hostname);
    KNI_EndHandles();

    if (phostent != NULL) {
      p->destination_sin.sin_family = AF_INET;
      port = KNI_GetParameterAsInt(2);
      p->destination_sin.sin_port = htons((short)port);
      memcpy((char *) &p->destination_sin.sin_addr,
             phostent->h_addr, phostent->h_length);
      p->fd = socket(AF_INET, SOCK_STREAM, 0);
      if (p->fd >= 0 && set_blocking_flags(&p->fd, /*is_blocking*/ KNI_TRUE) &&
          !ANI_UseFunction(asynchronous_connect_socket,
                           /*try_non_blocking*/ KNI_FALSE)) {
        ANI_BlockThread();
      }
    }
  }

  result = p->fd;
  ANI_End();
  KNI_ReturnInt(result);
}

typedef struct {
  int fd;
  char *buffer;
  int buffer_size;
} SocketBufferParameter;

static SocketBufferParameter *
new_buffer_parameter(int fd, size_t buffer_size) {
  SocketBufferParameter *p;
  size_t n = sizeof(SocketBufferParameter) + buffer_size;
  if (n > ANI_MAX_ALLOCATION_SIZE) {
    n = ANI_MAX_ALLOCATION_SIZE;
    buffer_size = n - sizeof(SocketBufferParameter);
  }
  p = (SocketBufferParameter *) ANI_AllocateParameterBlock(n);
  p->fd = fd;
  p->buffer = (char *) (p + 1);
  p->buffer_size = buffer_size;
  return p;
}

static jboolean
asynchronous_socket_read(void *parameter, jboolean is_non_blocking) {
  int n;
  SocketBufferParameter *p = (SocketBufferParameter *) parameter;

  //JVMSPI_PrintRaw("[");
  n = recv(p->fd, p->buffer, p->buffer_size, 0);
  //JVMSPI_PrintRaw("]");
  if (n == 0) {
    // If remote side has shut down the connection gracefully, and all
    // data has been received, recv() will complete immediately with
    // zero bytes received.
    //
    // This is true for Win32/CE and Linux
    n = -1;
  }

  if (n < 0) {
    if (GET_LAST_ERROR() == EWOULDBLOCK && is_non_blocking) {
      return KNI_FALSE;
    }
  }
  p->buffer_size = n;

  return KNI_TRUE;
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_readBuf() {
  int result = -1;
  SocketBufferParameter *p;

  if (!ANI_Start()) {
    ANI_Wait();
    KNI_ReturnInt(-1);
  }
  KNI_StartHandles(1);
  KNI_DeclareHandle(buffer_object);
  KNI_GetParameterAsObject(2, buffer_object);

  p = (SocketBufferParameter *) ANI_GetParameterBlock(NULL);
  if (p == NULL) {
    int buffer_size = KNI_GetParameterAsInt(4);
    p = new_buffer_parameter(/*fd*/ KNI_GetParameterAsInt(1), buffer_size);

    if (!ANI_UseFunction(asynchronous_socket_read,
                         /*try_non_blocking*/ KNI_FALSE)) {
      ANI_BlockThread();
      goto EXIT;
    }
  }

  if ((result = p->buffer_size) > 0) {
    jint offset = KNI_GetParameterAsInt(3);
    KNI_SetRawArrayRegion(buffer_object, offset, p->buffer_size,
                          (jbyte *) p->buffer);
  }

EXIT:
  KNI_EndHandles();
  ANI_End();
  KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_readByte() {
  int result = -1;
  SocketBufferParameter *p;

  if (!ANI_Start()) {
    ANI_Wait();
    KNI_ReturnInt(-1);
  }

  p = (SocketBufferParameter *) ANI_GetParameterBlock(NULL);
  if (p == NULL) {
    p = new_buffer_parameter(/*fd*/ KNI_GetParameterAsInt(1),
                             /*buffer_size*/ 1);

    if (!ANI_UseFunction(asynchronous_socket_read,
                         /*try_non_blocking*/ KNI_FALSE)) {
      ANI_BlockThread();
      goto EXIT;
    }
  }

  if (p->buffer_size == 1) {
    result = (unsigned char)(p->buffer[0]); // do not sign-extend
  }

EXIT:
  ANI_End();
  KNI_ReturnInt(result);
}

int try_count;
int done_count;
static jboolean
asynchronous_socket_write(void *parameter, jboolean is_non_blocking) {
  SocketBufferParameter *p = (SocketBufferParameter *) parameter;
  int n = send(p->fd, p->buffer, p->buffer_size, 0);
  done_count ++;
  if (n < 0) {
    if (GET_LAST_ERROR() == EWOULDBLOCK && is_non_blocking) {
      return KNI_FALSE;
    }
  }
  p->buffer_size = n;

  return KNI_TRUE;
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_writeBuf() {
  int result = -1;
  SocketBufferParameter *p;

  if (!ANI_Start()) {
    ANI_Wait();
    KNI_ReturnInt(-1);
  }

  p = (SocketBufferParameter *) ANI_GetParameterBlock(NULL);
  if (p == NULL) {
    p = new_buffer_parameter(/*fd*/ KNI_GetParameterAsInt(1),
                             /*buffer_size*/ KNI_GetParameterAsInt(4));
    KNI_StartHandles(1);
    KNI_DeclareHandle(buffer_object);
    KNI_GetParameterAsObject(2, buffer_object);
    KNI_GetRawArrayRegion(buffer_object, /*offset*/ KNI_GetParameterAsInt(3),
                          p->buffer_size, (jbyte *) p->buffer);
    KNI_EndHandles();

    if (!ANI_UseFunction(asynchronous_socket_write,
                         /*try_non_blocking*/ KNI_FALSE)) {
      try_count ++;
      ANI_BlockThread();
      goto EXIT;
    }
  }

  result = p->buffer_size;

EXIT:
  ANI_End();
  KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_writeByte() {
  int result = -1;
  SocketBufferParameter *p;

  if (!ANI_Start()) {
    ANI_Wait();
    KNI_ReturnInt(-1);
  }

  p = (SocketBufferParameter *) ANI_GetParameterBlock(NULL);
  if (p == NULL) {
    p = new_buffer_parameter(/*fd*/ KNI_GetParameterAsInt(1),
                             /*buffer_size*/ 1);
    p->buffer[0] = KNI_GetParameterAsInt(2) & 0x000000ff;

    if (!ANI_UseFunction(asynchronous_socket_write,
                         /*try_non_blocking*/ KNI_FALSE)) {
      try_count ++;
      ANI_BlockThread();
      goto EXIT;
    }
  }

  result = p->buffer_size;

EXIT:
  ANI_End();
  KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_available0() {
  jint sock = KNI_GetParameterAsInt(1);

  return (jint) 0; // IMPL_NOTE: use select() .....
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_cldc_io_j2me_socket_Protocol_close0() {
  jint sock = KNI_GetParameterAsInt(1);

  // NOTE: this would block the VM. A real implementation should
  // make this a async native method.
  shutdown(sock, 2);
  closesocket(sock);
}

void
JVMSPI_CheckEvents(JVMSPI_BlockedThreadInfo * blocked_threads,
                   int blocked_threads_count, jlong timeout_milli_seconds)
{
  //
  // Nothing to do on behalf of bsd sockets!
  //
  // Copy this function to your favorite place and
  // add your event checking code for any other stuff than bsd sockets.

  if (JVM_IsDebuggerActive()) {
    JVM_ProcessDebuggerCmds();
  }

#if NOT_CURRENTLY_USED
  // Enable this portion if you want to perform polling
  // with increasing timeouts

#define MIN_POLL_MILLI_SECONDS     1
#define MAX_POLL_MILLI_SECONDS  5000

  static int poll_interval_milli_seconds = MIN_POLL_MILLI_SECONDS;

  if (timeout_milli_seconds < 0) {
    // There seems to be no active thread right now
    // Poll in increasing intervals
    poll_interval_milli_seconds *= 2;
    if (poll_interval_milli_seconds > MAX_POLL_MILLI_SECONDS) {
      poll_interval_milli_seconds = MAX_POLL_MILLI_SECONDS;
    }
    timeout_milli_seconds = poll_interval_milli_seconds;
  } else {
    // We have encountered an active thread again
    // Reset the polling wait to minimum
    poll_interval_milli_seconds = MIN_POLL_MILLI_SECONDS;
  }
#endif

  ANI_WaitForThreadUnblocking(blocked_threads, blocked_threads_count,
                              timeout_milli_seconds);

  // Implement alternating polling by enabling the code above
  // and then adding your own checking code
  // for additional event sources here
}
