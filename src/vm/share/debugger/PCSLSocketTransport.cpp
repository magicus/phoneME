/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
#include "incls/_PCSLSocketTransport.cpp.incl"

// The implementation from this file should be used, until the target platform
// is not supported by PCSL, or when building without PCSL
#if ENABLE_PCSL && !defined(__SYMBIAN32__)

#if ENABLE_JAVA_DEBUGGER

#include <pcsl_network.h>
#include <pcsl_socket.h>
#include <pcsl_serversocket.h>

#if 0
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

// in milliseconds
#define JVM_NET_INIT_ATTEMPT_TIME ((unsigned long)500)
#define JVM_NET_INIT_TIMEOUT      ((unsigned long)5000)

bool SocketTransport::_first_time = true;
int _listen_socket = 0;
bool _wait_for_network_init = false;
bool _wait_for_read = false;
bool _wait_for_write = false;

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

/**
 * This function is called when the network initialization
 * or finalization is completed.
 *
 * @param isInit 0 if the network finalization has been finished,
 *               not 0 - if the initialization
 * @param status one of PCSL_NET_* completion codes
 */
static void pcsl_network_initialized(int isInit, int status) {
    (void)isInit;
    (void)status;
}

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
  int res;

  if (_first_time) {
    _first_time = false;
    debugger_port = Arguments::_debugger_port;
    if (debugger_port == 0) {
      debugger_port = DefaultDebuggerPort;
    }

    //unsigned long timeout = 0;
    if (!_wait_for_network_init) {
      res = pcsl_network_init_start(pcsl_network_initialized);
    } else {
      res = pcsl_network_init_finish(NULL);
    }

    if (res == PCSL_NET_WOULDBLOCK) {
      // wait until the network is up
      _wait_for_network_init = true;
      _first_time = true;
      return;
    }

    _wait_for_network_init = false;

    if (res != PCSL_NET_SUCCESS) {
      jvm_fprintf(stdout, "Could not init the network");
      return;
    }

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
//  struct sockaddr_in rem_addr;
//  int rem_addr_len = sizeof(rem_addr);
//  fd_set readFDs, writeFDs, exceptFDs;
//  int numFDs;
//  int width;
//  struct timeval tv;
  int optval;
  int connect_socket = 0;
  SocketTransport *st = (SocketTransport *)t;
  int status;
  static void *pContext = NULL;

  if (_listen_socket == -1) {
    return false;
  }

  if (ct == SERVER) {
    /*listen =*/ st->listener_socket();
//    if (timeout != -1) {
//      tv.tv_sec = timeout;
//      tv.tv_usec = 0;
//    }
//    FD_ZERO(&readFDs);
//    FD_ZERO(&writeFDs);
//    FD_ZERO(&exceptFDs);
//    FD_SET((unsigned int)_listen_socket, &readFDs);
//    width = _listen_socket;

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

    //if (FD_ISSET(_listen_socket, &readFDs)) {
      //connect_socket = ::accept(_listen_socket,
      //                          (struct sockaddr *)((void*)&rem_addr),
      //                          (socklen_t *)&rem_addr_len);
      status = pcsl_serversocket_accept_start((void*)_listen_socket,
        (void**)&connect_socket, &pContext);
      if (status == PCSL_NET_WOULDBLOCK) {
        return(false);
      }

      status = pcsl_serversocket_accept_finish((void*)_listen_socket,
        (void**)&connect_socket, pContext);
      if (status == PCSL_NET_WOULDBLOCK) {
        return(false);
      }

      if (status != PCSL_NET_SUCCESS) {
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

    //} else {
      //return (false);
    //}
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
#if 1
  SocketTransport *st = (SocketTransport *)t;
  int dbg_socket = st->debugger_socket();
  int bytesAvailable = 0;

  if (dbg_socket == -1) {
    return false;
  }

  int status = pcsl_socket_available((void*)dbg_socket, &bytesAvailable);
  if (status == PCSL_NET_SUCCESS && bytesAvailable >= 1) {
    return true;
  } else {
    return false;
  }

#else
  UsingFastOops fastoops;
  fd_set readFDs, writeFDs, exceptFDs;
  int numFDs = 0;
  int width;
  struct timeval tv, *tvp;
  SocketTransport *st = (SocketTransport *)t;
  int dbg_socket = st->debugger_socket();

  if (dbg_socket == -1) {
    return false;
  }
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
#endif
}

int SocketTransport::write_bytes(Transport *t, void *buf, int len)
{
  UsingFastOops fastoops;
  SocketTransport *st = (SocketTransport *)t;
  int dbg_socket = st->debugger_socket();
  int bytes_sent = 0;
  int status;
  static void* pContext;

  if (dbg_socket == -1) {
    return 0;
  }

  //bytes_sent = (int)send(dbg_socket, (char *)buf, len, 0);

  if (!_wait_for_write) {
    status = pcsl_socket_write_start((void*)dbg_socket, buf, len,
                                     &bytes_sent, &pContext);
  } else {  /* Reinvocation after unblocking the thread */
    status = pcsl_socket_write_finish((void*)dbg_socket, buf, len,
                                      &bytes_sent, pContext);
  }

  if (status == PCSL_NET_WOULDBLOCK) {
      _wait_for_write = true;
      return 0;
  }

  _wait_for_write = false;

  return bytes_sent;
}

int SocketTransport::peek_bytes(Transport *t, void *buf, int len)
{
  UsingFastOops fastoops;
  int nread;
  SocketTransport *st = (SocketTransport *)t;
  int dbg_socket = st->debugger_socket();

  if (dbg_socket == -1 || len <= 0) {
    return 0;
  }

  int bytes_cached = st->bytes_cached();

  if (bytes_cached >= len) {
    jvm_memcpy((unsigned char *)buf, st->get_read_cache(), len);
    nread = len;
  } else {
    st->cache_bytes(len);

    if (bytes_cached > 0) {
      jvm_memcpy((unsigned char *)buf, st->get_read_cache(), bytes_cached);
    }

    nread = bytes_cached;
  }

  return nread;

  //nread = recv(dbg_socket, (char *)buf, len, MSG_PEEK);
  //if (nread <= 0) {
  //  return 0;
  //}

  // nread > 0
  //return nread;
}

void SocketTransport::cache_bytes(int len) {

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
  int ststus;
  static void *pContext;

  if (dbg_socket == -1) {
    return 0;
  }
  if (nleft == 0) {
    // trying to read 0 bytes, just return
    return 0;
  }
  
  do {
//    nread = recv(dbg_socket, ptr, nleft, 0);
    nread = 0;

    if (!_wait_for_read) {
      status = pcsl_socket_read_start((void*)dbg_socket, ptr, nleft,
                                      &nread, &pContext);
    } else {  /* Reinvocation after unblocking the thread */
      status = pcsl_socket_read_finish((void*)dbg_socket,  ptr, nleft,
                                       &nread, pContext);
    }

    if (status == PCSL_NET_WOULDBLOCK) {
        _wait_for_read = true;
        return 0;
    }

    _wait_for_read = false;

    if (status != PCSL_NET_SUCCESS) {
#ifdef AZZERT
          tty->print_cr("SocketTransport: pcsl_socket_read_*() failed");
#endif
      return 0;
    }

    if (nread == 0) {
      if (total > 0) {
        return total;
      } else {
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

#undef JVM_NET_INIT_ATTEMPT_TIME
#undef JVM_NET_INIT_TIMEOUT

#endif // ENABLE_JAVA_DEBUGGER

#endif // ENABLE_PCSL && !defined(__SYMBIAN32__)
