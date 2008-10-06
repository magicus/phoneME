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

bool SocketTransport::_first_time = true;
bool SocketTransport::_network_is_up = false;
bool SocketTransport::_wait_for_network_init = false;

void* SocketTransport::_listen_handle = INVALID_HANDLE;

bool SocketTransport::_wait_for_accept = false;
bool SocketTransport::_wait_for_read = false;
bool SocketTransport::_wait_for_write = false;

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
  if (isInit) {
    if (status == PCSL_NET_SUCCESS) {
      SocketTransport::_wait_for_network_init = false;
      SocketTransport::_network_is_up = true;
    }
  } else {
    if (status == PCSL_NET_SUCCESS) {
      SocketTransport::_wait_for_network_init = false;
      SocketTransport::_network_is_up = false;
      SocketTransport::_first_time = true;
    }
  }
}

char *SocketTransport::name() {
  return (char*)"socket";
}

// Initialize the socket transport by setting up a listener socket

void SocketTransport::init_transport(void *t JVM_TRAPS)
{
  (void)t; // Why? Do we need it in our arguments?
  JVM_IGNORE_TRAPS;
  UsingFastOops fastoops;
  short debugger_port;
  int res;

  if (_first_time) {

    if (!_network_is_up) {
      //unsigned long timeout = 0;
      if (!_wait_for_network_init) {
        res = pcsl_network_init_start(pcsl_network_initialized);
      } else {
        res = pcsl_network_init_finish(NULL);
      }

      if (res == PCSL_NET_WOULDBLOCK) {
        // wait until the network is up
        _wait_for_network_init = true;
        return;
      }

      _wait_for_network_init = false;

      if (res != PCSL_NET_SUCCESS) {
        jvm_fprintf(stdout, "Could not init the network");
        _first_time = false; // don't try to init network anymore
        return;
      }

      _network_is_up = true;
    }

    debugger_port = Arguments::_debugger_port;
    if (debugger_port == 0) {
      debugger_port = DefaultDebuggerPort;
    }

    res = pcsl_serversocket_open(debugger_port, &_listen_handle);

    if (res != PCSL_NET_SUCCESS) {
      jvm_fprintf(stdout, "Could not open listen socket");
      return;
    }

    _first_time = false;
  }
}

ReturnOop SocketTransport::new_transport(JVM_SINGLE_ARG_TRAPS)
{
  SocketTransport::Raw this_transport =
  SocketTransport::allocate(JVM_SINGLE_ARG_CHECK_0);
  this_transport().set_listener_socket((int)_listen_handle);
  this_transport().set_debugger_socket((int)INVALID_HANDLE);
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
  void* connect_handle = INVALID_HANDLE;
  SocketTransport *st = (SocketTransport *)t;
  int status;
  static void *pContext = NULL;

  if (_listen_handle == INVALID_HANDLE) {
    return false;
  }

  st->init_read_cache();

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

#if 0
    if (timeout == -1) {
      numFDs = ::select(width+1, &readFDs, &writeFDs, &exceptFDs, NULL);
    } else {
      numFDs = ::select(width+1, &readFDs, &writeFDs, &exceptFDs, &tv);
    }

    if (numFDs <= 0) {
      if (Verbose) {
        tty->print_cr("Select failed");
      }
      return false;
    }
#endif

    //if (FD_ISSET(_listen_socket, &readFDs)) {
      //connect_socket = ::accept(_listen_socket,
      //                          (struct sockaddr *)((void*)&rem_addr),
      //                          (socklen_t *)&rem_addr_len);
    if (!_wait_for_accept) {
      status = pcsl_serversocket_accept_start(_listen_handle,
        &connect_handle, &pContext);
      if (status == PCSL_NET_WOULDBLOCK) {
        _wait_for_accept = true;
        return false;
      }
    } else {
      status = pcsl_serversocket_accept_finish(_listen_handle,
        &connect_handle, pContext);
      if (status == PCSL_NET_WOULDBLOCK) {
        return false;
      }
      _wait_for_accept = false;
    }

    if (status != PCSL_NET_SUCCESS) {
      if (Verbose) {
        tty->print_cr("Accept failed");
      }
      return false;
    }

    return true;

#if 0
      /* Turn off Nagle algorithm which is slow in NT.  Since we send many
       * small packets Nagle actually slows us down as we send the packet
       * header in tiny chunks before sending the data portion.  Without this
       * option, it could take 200 ms. per packet roundtrip from KVM on NT to
       * Forte running on some other machine.
       */
      optval = 1;
      ::setsockopt(na_get_fd(connect_handle), IPPROTO_TCP, TCP_NODELAY,
                   (char *)&optval, sizeof(optval));
      st->set_debugger_socket((int)connect_handle);
      return true;
#endif

  } else {
    return false;
  }
}

bool SocketTransport::initialized(Transport *t){
  SocketTransport *st = (SocketTransport *)t;
  return ((void*)st->debugger_socket() != INVALID_HANDLE);
}

void SocketTransport::disconnect_transport(Transport *t)
{
  UsingFastOops fastoops;
  SocketTransport *st = (SocketTransport *)t;
  void* dbg_handle = st->debugger_socket();

  if (dbg_handle != INVALID_HANDLE) {
    /* IMPL_NOTE: wait here for 2 seconds to let other side to finish */

    pcsl_socket_shutdown_output(dbg_handle);

   /* 
    * Note that this function NEVER returns PCSL_NET_WOULDBLOCK. Therefore, the
    * finish() function should never be called and does nothing.
    */
    pcsl_socket_close_start(dbg_handle, NULL);

    st->set_debugger_socket((int)INVALID_HANDLE);
  }
}

void SocketTransport::destroy_transport(Transport *t) {
  UsingFastOops fastoops;
  SocketTransport *st = (SocketTransport *)t;
  void* listener_handle = st->listener_socket();

  st->finalize_read_cache();

  if (listener_handle != INVALID_HANDLE) {
    // last socket in the system, shutdown the listener socket
    pcsl_socket_shutdown_output(listener_handle);

   /* 
    * Note that this function NEVER returns PCSL_NET_WOULDBLOCK. Therefore, the
    * finish() function should never be called and does nothing.
    */
    pcsl_socket_close_start(listener_handle, NULL);

    st->set_listener_socket((int)INVALID_HANDLE);
  }

  /* The listener socket may be reopen again later */
  _first_time = true;
}

// Select on the dbgSocket and wait for a character to arrive.
// Timeout is in millisecs.

bool SocketTransport::char_avail(Transport *t, int timeout)
{
  UsingFastOops fastoops;
  SocketTransport *st = (SocketTransport *)t;
  void* dbg_handle = st->debugger_socket();
  int bytesAvailable = 0;

  if (dbg_handle == INVALID_HANDLE) {
    return false;
  }

  int status = pcsl_socket_available(dbg_handle, &bytesAvailable);
  if (status == PCSL_NET_SUCCESS && bytesAvailable >= 1) {
    return true;
  } else {
    return false;
  }
}

int SocketTransport::write_bytes(Transport *t, void *buf, int len)
{
  UsingFastOops fastoops;
  SocketTransport *st = (SocketTransport *)t;
  void* dbg_handle = st->debugger_socket();
  int bytes_sent = 0;
  int status;
  static void* pContext;

  if (dbg_handle == INVALID_HANDLE) {
    return 0;
  }

  //bytes_sent = (int)send(dbg_socket, (char *)buf, len, 0);

  if (!_wait_for_write) {
    status = pcsl_socket_write_start(dbg_handle, buf, len,
                                     &bytes_sent, &pContext);
  } else {  /* Reinvocation after unblocking the thread */
    status = pcsl_socket_write_finish(dbg_handle, buf, len,
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
  void* dbg_handle = st->debugger_socket();

  if (dbg_handle == INVALID_HANDLE || len <= 0) {
    return 0;
  }

  return read_bytes_impl(t, buf, len, false, true);

  //nread = recv(dbg_socket, (char *)buf, len, MSG_PEEK);
  //if (nread <= 0) {
  //  return 0;
  //}

  // nread > 0
  //return nread;
}

int SocketTransport::read_bytes(Transport *t, void *buf, int len, bool blockflag)
{
  return read_bytes_impl(t, buf, len, blockflag, false);
}

int SocketTransport::read_bytes_impl(Transport *t, void *buf, int len,
                                     bool blockflag, bool peekOnly)
{
  UsingFastOops fastoops;
  int nread;
  unsigned int nleft = len;
  char *ptr = (char *) buf;
  unsigned int total = 0;
  SocketTransport *st = (SocketTransport *)t;
  void* dbg_handle = st->debugger_socket();
  int ststus;
  static void *pContext;

  if (dbg_socket == INVALID_HANDLE) {
    return 0;
  }
  if (nleft == 0) {
    // trying to read 0 bytes, just return
    return 0;
  }

  int bytes_cached = st->bytes_cached_for_read();

  if (bytes_cached >= len) {
    jvm_memcpy((unsigned char *)buf, st->get_read_cache(), len);
    if (!peekOnly) {
      st->set_bytes_cached_for_read(bytes_cached - len);
    }

    return len;
  } else {
    if (bytes_cached > 0) {
      jvm_memcpy((unsigned char *)buf, st->get_read_cache(), bytes_cached);
      len -= bytes_cached;
      ptr += bytes_cached;
    }

    nread = bytes_cached;
    total = bytes_cached;
    nleft = len; 

    if (!peekOnly) {
      st->set_bytes_cached_for_read(0);
    }
  }

  do {
//    nread = recv(dbg_socket, ptr, nleft, 0);
//    nread = 0;

    if (!_wait_for_read) {
      status = pcsl_socket_read_start(dbg_handle, ptr, nleft,
                                      &nread, &pContext);
    } else {  /* Reinvocation after unblocking the thread */
      status = pcsl_socket_read_finish(dbg_handle,  ptr, nleft,
                                       &nread, pContext);
    }

    if (status == PCSL_NET_WOULDBLOCK) {
        _wait_for_read = true;
        return total;
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
    if (peekOnly) {
      // add the read data to the cache
      bool success = st->add_to_read_cache(ptr, nread);
      if (!success) {
        break;
      }
    }

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

bool SocketTransport::add_to_read_cache(unsigned char* p_buf, int len) {
  if (len <= 0 || p_buf == NULL) {
    return;
  }

  if (_m_read_cache_size < _m_bytes_cached_for_read + len) {
    unsigned char* p_tmp_buf;
    int new_cache_size = _m_read_cache_size + len;

    p_tmp_buf = (unsigned char*)jvm_malloc(new_cache_size);
    if (p_tmp_buf == NULL) {
      return false;
    }

    _m_p_read_cache = p_tmp_buf;
    _m_read_cache_size = new_cache_size;
  }

  jvm_memcpy(&_m_read_cache[_m_bytes_cached_for_read], p_buf, len);
  _m_bytes_cached_for_read += len;
  
  return true;
}

void SocketTransport::finalize_read_cache() {
  if (_m_p_read_cache != NULL) {
    jvm_free(_m_p_read_cache);
    _m_p_read_cache = NULL;
    _m_read_cache_size = 0;
    _m_bytes_cached_for_read = 0;
  }
}

//IMPL_NOTE: this is now broken

extern "C" int JVM_GetDebuggerSocketFd() {
  Transport::Raw t = Universe::transport_head();
  SocketTransport::Raw st = t().obj();
  return (na_get_fd(st().debugger_socket()));
}

#endif // ENABLE_JAVA_DEBUGGER

#endif // ENABLE_PCSL && !defined(__SYMBIAN32__)
