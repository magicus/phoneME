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

# include "incls/_precompiled.incl"
# include "incls/_SocketTransport.cpp.incl"

#if ENABLE_JAVA_DEBUGGER

#if defined(LINUX) || defined (CYGWIN)
#define USE_UNISTD_SOCKETS 1
#else
#if defined(WIN32) || defined(UNDER_CE)
#define USE_WINSOCK_SOCKETS 1
#else
#if defined(__SYMBIAN32__)
#define USE_SYMBIAN_SOCKETS 1
#endif
#endif
#endif

// ======================================================================

#ifdef USE_WINSOCK_SOCKETS
#include <winsock.h>
typedef int socklen_t;
#endif

// ======================================================================

#ifdef USE_UNISTD_SOCKETS
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#define closesocket close
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
#endif

// ======================================================================

#ifdef USE_SYMBIAN_SOCKETS

extern "C" {

/*
 * NOTE: these declarations must exactly match WinSock. Do NOT use
 * the socket header files included in the Symbian OS SDK.
 *
 * Socket functionality needed by the Java debugger is implemented
 * on WINS only. ARM4 is not supported.
 *
 * The WINS implementation is based on WinSock. All socket calls made by
 * the Java debugger are routed to WinSock via functions such as bsd_socket()
 * and bsd_send(). See build/share/symbian/ewinsock/ewinsock.c
 * for more information.
 */

#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif

#define FD_SETSIZE 64
#define AF_INET         2
#define SOCK_STREAM     1
#ifndef MSG_PEEK
#define MSG_PEEK        2
#endif

struct in_addr {
  union {
    struct { unsigned char s_b1,s_b2,s_b3,s_b4; } S_un_b;
    struct { unsigned short s_w1,s_w2; } S_un_w;
    unsigned long S_addr;
  } S_un;
#define s_addr  S_un.S_addr
};

struct sockaddr_in {
  short   sin_family;
  u_short sin_port;
  struct  in_addr sin_addr;
  char    sin_zero[8];
};

#define INADDR_ANY       ((unsigned long)0x00000000)
#define IPPROTO_TCP      6
#define TCP_NODELAY      0x0001
#define SOL_SOCKET       0xffff
#define SO_REUSEADDR     0x0004

typedef struct bsd_fd_set {
  u_int  fd_count;
  int    fd_array[FD_SETSIZE];
} bsd_fd_set;

struct bsd_timeval {
  long tv_sec;
  long tv_usec;
};

#ifdef FD_SET
#undef FD_SET
#endif

#ifdef FD_ZERO
#undef FD_ZERO
#endif

#ifdef FD_ISSET
#undef FD_ISSET
#endif

#define FD_SET(fd, set) do { \
    if (((fd_set *)(set))->fd_count < FD_SETSIZE) \
        ((fd_set *)(set))->fd_array[((fd_set *)(set))->fd_count++]=(fd);\
} while(0)
#define FD_ZERO(set) (((fd_set  *)(set))->fd_count=0)
#define FD_ISSET(fd, set) bsd_fd_is_set(fd, (fd_set *)(set))

#define socket        bsd_socket
#define bind          bsd_bind
#define send          bsd_send
#define recv          bsd_recv
#define listen        bsd_listen
#define accept        bsd_accept
#define select        bsd_select
#define closesocket   bsd_close_socket
#define htons         bsd_htons
#define htonl         bsd_htonl
#define ntohs         bsd_ntohs
#define ntohl         bsd_ntohl
#define shutdown      bsd_shutdown
#define setsockopt    bsd_setsockopt

#define fd_set        bsd_fd_set
#define timeval       bsd_timeval
#define socklen_t     int

int            bsd_socket(int af, int type, int protocol);
int            bsd_close_socket(int s);
int            bsd_bind(int s, const struct sockaddr*  name, int namelen);
int            bsd_listen(int s, int backlog);
int            bsd_select(int nfds, fd_set * readfds,  fd_set * writefds,
                          fd_set * exceptfds,
                          const struct bsd_timeval * timeout);
int            bsd_fd_is_set(int fd, fd_set * fds);
int            bsd_accept(int s, struct sockaddr* addr, int* addrlen);
int            bsd_recv(int sock_fd, char* buf, int len, int flags);
int            bsd_send(int sock_fd, char* buf, int len, int flags);
unsigned short bsd_htons(unsigned short hostshort);
unsigned long  bsd_ntohl(unsigned long netlong);
unsigned short bsd_ntohs(unsigned short netshort);
unsigned long  bsd_htonl(unsigned long hostlong);

int            bsd_setsockopt(int s, int level, int optname,
                              const char * optval, int optlen);
int            bsd_shutdown(int s, int how);
int            bsd_errno();

} // extern "C"

#endif // __SYMBIAN32__

// ======================================================================

#if UNDER_ADS

// IMPL_NOTE: This should be done only for ARMulator builds.

extern "C" {

#define FD_SETSIZE 64
#define AF_INET         2
#define SOCK_STREAM     1

struct in_addr {
  union {
    struct { unsigned char s_b1,s_b2,s_b3,s_b4; } S_un_b;
    struct { unsigned short s_w1,s_w2; } S_un_w;
    unsigned long S_addr;
  } S_un;
#define s_addr  S_un.S_addr
};

struct sockaddr_in {
  short   sin_family;
  jushort sin_port;
  struct  in_addr sin_addr;
  char    sin_zero[8];
};

#define INADDR_ANY       ((unsigned long)0x00000000)
#define IPPROTO_TCP      6
#define TCP_NODELAY      0x0001
#define SOL_SOCKET       0xffff
#define SO_REUSEADDR     0x0004
#ifndef MSG_PEEK
#define MSG_PEEK         2
#endif

typedef struct bsd_fd_set {
  juint  fd_count;
  int    fd_array[FD_SETSIZE];
} bsd_fd_set;

struct bsd_timeval {
  int tv_sec;
  int tv_usec;
};

#define FD_SET(fd, set) do { \
    if (((fd_set *)(set))->fd_count < FD_SETSIZE) \
        ((fd_set *)(set))->fd_array[((fd_set *)(set))->fd_count++]=(fd);\
} while(0)
#define FD_ZERO(set) (((fd_set  *)(set))->fd_count=0)
#define FD_ISSET(fd, set) bsd_fd_is_set(fd, (fd_set *)(set))

#define socket        bsd_socket
#define bind          bsd_bind
#define send          bsd_send
#define recv          bsd_recv
#define listen        bsd_listen
#define accept        bsd_accept
#define select        bsd_select
#define closesocket   bsd_close_socket
#define htons         bsd_htons
#define htonl         bsd_htonl
#define ntohs         bsd_ntohs
#define ntohl         bsd_ntohl
#define shutdown      bsd_shutdown
#define setsockopt    bsd_setsockopt

#define fd_set        bsd_fd_set
#define timeval       bsd_timeval
#define socklen_t     int

int            bsd_socket(int /*af*/, int /*type*/, int /*protocol*/)
               {return 0; /* IMPL_NOTE DUMMY */}
int            bsd_close_socket(int /*s*/)
               {return 0; /* IMPL_NOTE DUMMY */}
int            bsd_bind(int /*s*/, const struct sockaddr* /*name*/,
                        int /*namelen*/)
               {return 0; /* IMPL_NOTE DUMMY */}
int            bsd_listen(int /*s*/, int /*backlog*/)
               {return 0; /* IMPL_NOTE DUMMY */}
int            bsd_select(int /*nfds*/, fd_set* /*readfds*/,
                          fd_set * /*writefds*/, fd_set * /*exceptfds*/,
                          const struct bsd_timeval* /*timeout*/)
               {return 0; /* IMPL_NOTE DUMMY */}
int            bsd_fd_is_set(int /*fd*/, fd_set * /*fds*/)
               {return 0; /* IMPL_NOTE DUMMY */}
int            bsd_accept(int /*s*/, struct sockaddr* /*addr*/,
                          int* /*addrlen*/)
               {return 0; /* IMPL_NOTE DUMMY */}
int            bsd_recv(int /*sock_fd*/, char* /*buf*/, int /*len*/,
                        int /*flags*/)
               {return 0; /* IMPL_NOTE DUMMY */}
int            bsd_send(int /*sock_fd*/, char* /*buf*/, int /*len*/,
                        int /*flags*/)
               {return 0; /* IMPL_NOTE DUMMY */}
unsigned short bsd_htons(unsigned short /*hostshort*/)
               {return 0; /* IMPL_NOTE DUMMY */}
unsigned long  bsd_ntohl(unsigned long /*netlong*/)
               {return 0; /* IMPL_NOTE DUMMY */}
unsigned short bsd_ntohs(unsigned short /*netshort*/)
               {return 0; /* IMPL_NOTE DUMMY */}
unsigned long  bsd_htonl(unsigned long /*hostlong*/)
               {return 0; /* IMPL_NOTE DUMMY */}

int            bsd_setsockopt(int /*s*/, int /*level*/, int /*optname*/,
                              const char * /*optval*/, int /*optlen*/)
               {return 0; /* IMPL_NOTE DUMMY */}
int            bsd_shutdown(int /*s*/, int /*how*/)
               {return 0; /* IMPL_NOTE DUMMY */}
int            bsd_errno()
               {return 0; /* IMPL_NOTE DUMMY */}

} // extern "C"

#endif // UNDER_ADS

bool SocketTransport::_first_time = true;
int _listen_socket = 0;

Transport::transport_op_def_t SocketTransport::socket_transport_ops = {
  name,
  char_avail,
  write_bytes,
  write_int,
  write_short,
  flush,
  peek_bytes,
  read_bytes,
  read_int,
  read_short,
  destroy_transport,
  initialized,
  connect_transport,
  disconnect_transport
};

// ======================================================================

char *SocketTransport::name() {
  return (char*)"socket";
}

// Initialize the socket transport by setting up a listener socket

void SocketTransport::init_transport(void *t JVM_TRAPS)
{
  (void)t; // Why? Do we need it in our arguments?
  JVM_IGNORE_TRAPS;
  UsingFastOops fast_oops;
  struct sockaddr_in local_addr;
  int optval;
  short debugger_port;
  int socket_err;

  if (_first_time) {
    _first_time = false;
    debugger_port = Arguments::_debugger_port;
    if (debugger_port == 0) {
      debugger_port = DefaultDebuggerPort;
    }
#ifdef USE_WINSOCK_SOCKETS
    WSADATA wsaData;
    if (::WSAStartup(0x0101, &wsaData) != 0) {
      jvm_fprintf(stdout, "Could not init winsock");
      return;
    }
#endif
    _listen_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (_listen_socket < 0) {
      jvm_fprintf(stdout, "Could not open listen socket");
      return;
    }
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(debugger_port);

    optval = 1;
    ::setsockopt(_listen_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&optval,
                 sizeof(optval));
    socket_err = ::bind(_listen_socket,
                        (struct sockaddr *)((void*)&local_addr),
                        sizeof(local_addr));
    if (socket_err < 0) {
      ::closesocket(_listen_socket);
      _listen_socket = -1;
      jvm_fprintf(stdout, "Could not bind to listen socket");
      return;
    }
    socket_err = ::listen(_listen_socket, 1);
    if (socket_err < 0) {
      ::closesocket(_listen_socket);
      _listen_socket = -1;
      jvm_fprintf(stdout, "Could not listen on socket");
      return;
    }
  }
}

ReturnOop SocketTransport::new_transport(JVM_SINGLE_ARG_TRAPS)
{
  SocketTransport::Raw this_transport =
    SocketTransport::allocate(JVM_SINGLE_ARG_CHECK_0);
  this_transport().set_listener_socket(_listen_socket);
  this_transport().set_debugger_socket(-1);
  this_transport().set_ops(&socket_transport_ops);
  return this_transport;

}

bool SocketTransport::connect_transport(Transport *t, ConnectionType ct, int timeout)
{
  UsingFastOops fastoops;
  struct sockaddr_in rem_addr;
  int rem_addr_len = sizeof(rem_addr);
  fd_set readFDs, writeFDs, exceptFDs;
  int numFDs;
  int width;
  struct timeval tv;
  int optval;
  int connect_socket = 0;
  SocketTransport *st = (SocketTransport *)t;

  if (_listen_socket == -1) {
    return false;
  }
  if (ct == SERVER) {
    /*listen =*/ st->listener_socket();
    if (timeout != -1) {
      tv.tv_sec = timeout;
      tv.tv_usec = 0;
    }
    FD_ZERO(&readFDs);
    FD_ZERO(&writeFDs);
    FD_ZERO(&exceptFDs);
    FD_SET((unsigned int)_listen_socket, &readFDs);
    width = _listen_socket;
    if (timeout == -1) {
      numFDs = ::select(width+1, &readFDs, &writeFDs, &exceptFDs, NULL);
    } else {
      numFDs = ::select(width+1, &readFDs, &writeFDs, &exceptFDs, &tv);
    }
    if (numFDs <= 0) {
      if (Verbose) {
        tty->print_cr("Select failed");
      }
      return(false);
    }
    if (FD_ISSET(_listen_socket, &readFDs)) {
      connect_socket = ::accept(_listen_socket,
                                (struct sockaddr *)((void*)&rem_addr),
                                (socklen_t *)&rem_addr_len);
      if (connect_socket < 0) {
        if (Verbose) {
          tty->print_cr("Accept failed");
        }
        return (false);
      }
      /* Turn off Nagle algorithm which is slow in NT.  Since we send many
       * small packets Nagle actually slows us down as we send the packet
       * header in tiny chunks before sending the data portion.  Without this
       * option, it could take 200 ms. per packet roundtrip from KVM on NT to
       * Forte running on some other machine.
       */
      optval = 1;
      ::setsockopt(connect_socket, IPPROTO_TCP, TCP_NODELAY,
                   (char *)&optval, sizeof(optval));
      st->set_debugger_socket(connect_socket);
      return (true);

    } else
      return (false);
  } else {
    return (false);
  }
}

bool SocketTransport::initialized(Transport *t){
  SocketTransport *st = (SocketTransport *)t;
  return (st->debugger_socket() != -1);
}

void SocketTransport::disconnect_transport(Transport *t)
{
  UsingFastOops fastoops;
  fd_set readFDs, writeFDs, exceptFDs;
  int width;
  struct timeval tv;
  SocketTransport *st = (SocketTransport *)t;
  int dbg_socket = st->debugger_socket();

  if (dbg_socket != -1) {
    FD_ZERO(&readFDs);
    FD_ZERO(&writeFDs);
    FD_ZERO(&exceptFDs);
    FD_SET((unsigned int)dbg_socket, &exceptFDs);
    width = dbg_socket;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    ::select(width+1, &readFDs, &writeFDs, &exceptFDs, &tv);
    ::shutdown(dbg_socket, 2);
    ::closesocket(dbg_socket);
    st->set_debugger_socket(-1);
  }
}

void SocketTransport::destroy_transport(Transport *t) {
  UsingFastOops fastoops;
  SocketTransport *st = (SocketTransport *)t;
  int listener = st->listener_socket();

  if (listener != -1) {
    // last socket in the system, shutdown the listener socket
    ::shutdown(listener, 2);
    ::closesocket(listener);
    st->set_listener_socket(-1);
  }
  _first_time = true;
}

// Select on the dbgSocket and wait for a character to arrive.
// Timeout is in millisecs.

bool SocketTransport::char_avail(Transport *t, int timeout)
{
  UsingFastOops fastoops;
  fd_set readFDs, writeFDs, exceptFDs;
  int numFDs = 0;
  int width;
  struct timeval tv, *tvp;
  SocketTransport *st = (SocketTransport *)t;
  int dbg_socket = st->debugger_socket();

  if (dbg_socket == -1)
    return false;
  FD_ZERO(&readFDs);
  FD_ZERO(&writeFDs);
  FD_ZERO(&exceptFDs);
  FD_SET((unsigned int)dbg_socket, &readFDs);
  width = dbg_socket;
  if (timeout == -1) {
    tv.tv_sec = 0;
    tv.tv_usec = 20000;
    tvp = &tv;
    //    tvp = NULL;
  } else {
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    tvp = &tv;
  }
  numFDs = select(width+1, &readFDs, &writeFDs,
                  &exceptFDs, tvp);
  if (numFDs <= 0) {
    return(false);
  } else {
    return (true);
  }
}

int SocketTransport::write_bytes(Transport *t, void *buf, int len)
{
  UsingFastOops fastoops;
  SocketTransport *st = (SocketTransport *)t;
  int dbg_socket = st->debugger_socket();
  if (dbg_socket == -1)
    return 0;
  return (send(dbg_socket, (char *)buf, len, 0));
}

int SocketTransport::peek_bytes(Transport *t, void *buf, int len)
{
  UsingFastOops fastoops;
  int nread;
  SocketTransport *st = (SocketTransport *)t;
  int dbg_socket = st->debugger_socket();

  if (dbg_socket == -1)
    return 0;
  nread = recv(dbg_socket, (char *)buf, len, MSG_PEEK);
  if (nread < 0) {
#ifndef UNDER_ADS
#ifdef USE_WINSOCK_SOCKETS
    int error = WSAGetLastError();
#elif defined (__SYMBIAN32__) || UNDER_ADS
    int error = bsd_errno();
#else
    int error = errno;
#endif
#endif
    return 0;
  }

  if (nread == 0) {
        return 0;
  }
  // nread > 0
  return nread;
}

int SocketTransport::read_bytes(Transport *t, void *buf, int len, bool blockflag)
{
  UsingFastOops fastoops;
  int nread;
  unsigned int nleft = len;
  char *ptr = (char *) buf;
  unsigned int total = 0;
  SocketTransport *st = (SocketTransport *)t;
  int dbg_socket = st->debugger_socket();

  if (dbg_socket == -1)
    return 0;
  if (nleft == 0) {
    // trying to read 0 bytes, just return
    return 0;
  }
  do {
    nread = recv(dbg_socket, ptr, nleft, 0);
    if (nread < 0) {
#ifndef UNDER_ADS
#ifdef USE_WINSOCK_SOCKETS
      int error = WSAGetLastError();
#elif defined (__SYMBIAN32__) || UNDER_ADS
      int error = bsd_errno();
#else
      int error = errno;
#endif
#endif
      return 0;
    }

    if (nread == 0) {
      if (total > 0)
        return total;
      else {
        if (blockflag) {
          // Bad news. Got 0 bytes but didn't block, socket is closed on the
          // other end.
#ifdef AZZERT
          tty->print_cr("SocketTransport: Socket closed by peer");
#endif
          JavaDebugger::close_java_debugger(t);
        }
        return 0;
      }
    }
    // nread > 0
    total += nread;
    nleft -= nread;
    ptr   += nread;
  } while (nleft);

  return total;
}

void SocketTransport::flush(Transport *t)
{
  char buf[16];

  while (read_bytes(t, buf, 16, 0) > 0)
    ;
}

int
SocketTransport::read_int(Transport *t, void *buf)
{
  return (read_bytes(t, buf, sizeof(int), true));
}

short
SocketTransport::read_short(Transport *t, void *buf)
{
  return(read_bytes(t, buf, sizeof(short), true));
}

int SocketTransport::write_int(Transport *t, void *buf)
{
  return (write_bytes(t, buf, sizeof(int)));
}

int SocketTransport::write_short(Transport *t, void *buf)
{
  return (write_bytes(t, buf, sizeof(short)));
}

//IMPL_NOTE: this is now broken

extern "C" int JVM_GetDebuggerSocketFd() {
  Transport::Raw t = Universe::transport_head();
  SocketTransport::Raw st = t().obj();
  return (st().debugger_socket());
}

#if defined(__SYMBIAN32__) || UNDER_ADS

#undef socket
#undef bind
#undef send
#undef recv
#undef listen
#undef accept
#undef select
#undef closesocket
#undef htons
#undef htonl
#undef ntohs
#undef ntohl
#undef shutdown
#undef setsockopt

#endif

#endif
