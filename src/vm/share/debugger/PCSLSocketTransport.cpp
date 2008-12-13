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
#include "incls/_PCSLSocketTransport.cpp.incl"

// The implementation from this file should be used, until the target platform
// is not supported by PCSL, or when building without PCSL
#if ENABLE_PCSL

#if ENABLE_JAVA_DEBUGGER

#include <pcsl_network.h>
#include <pcsl_socket.h>

// this definition is needed to include PCSL support for server sockets
#define ENABLE_SERVER_SOCKET
#include <pcsl_serversocket.h>

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
void SocketTransport::network_initialized_callback(int isInit, int status)
{
  if (Verbose) {
    tty->print_cr("SocketTransport::network_initialized_callback()");
  }

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
  
  if (Verbose) {
    jvm_printf("Network status: %s\n", SocketTransport::_network_is_up ? "UP" : "DOWN");
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

  if (Verbose) {
    tty->print_cr("SocketTransport::init_transport()");
  }

  if (_first_time) {

    if (!_network_is_up) {
      if (!_wait_for_network_init) {
        res = pcsl_network_init_start(SocketTransport::network_initialized_callback);
      } else {
        res = pcsl_network_init_finish();
      }

      if (res == PCSL_NET_WOULDBLOCK) {
        // wait until the network is up
        if (Verbose) {
          tty->print_cr("SocketTransport::init_transport(): "
	                "waiting for network initialization.");
        }
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
  this_transport().reset_read_ahead_buffer();
  
  return this_transport;
}

bool SocketTransport::connect_transport(Transport *t, ConnectionType ct,
                                        int timeout)
{
  UsingFastOops fastoops;
  void* connect_handle = INVALID_HANDLE;
  SocketTransport *st = (SocketTransport *)t;
  int status;
  static void *pContext = NULL;

  if (Verbose) {
    tty->print_cr("SocketTransport::connect_transport()");
  }

  if (_listen_handle == INVALID_HANDLE) {
    if (Verbose) {
      tty->print_cr("SocketTransport::connect_transport(): "
                    "invalid _listen_handle");
    }
    return false;
  }

  if (ct == SERVER) {
    /* listen = st->listener_socket(); */

    (void)timeout; // TODO: take timeout into account

    if (!_wait_for_accept) {
      status = pcsl_serversocket_accept_start(_listen_handle,
        &connect_handle, &pContext);

      if (status == PCSL_NET_WOULDBLOCK) {
        if (Verbose) {
          tty->print_cr("SocketTransport::connect_transport(): "
	                "Waiting for accept() (start)");
        }
        _wait_for_accept = true;
        return false;
      }
    } else {
      status = pcsl_serversocket_accept_finish(_listen_handle,
        &connect_handle, &pContext);
      if (status == PCSL_NET_WOULDBLOCK) {
        if (Verbose) {
          tty->print_cr("SocketTransport::connect_transport(): "
	                "Waiting for accept() (finish)");
        }
        return false;
      }
      _wait_for_accept = false;
    }

    if (status != PCSL_NET_SUCCESS) {
      if (Verbose) {
        tty->print_cr("SocketTransport::connect_transport(): accept() failed");
      }
      return false;
    }

    if (Verbose) {
      tty->print_cr("SocketTransport::connect_transport(): connection accepted");
    }

    st->reset_read_ahead_buffer();
    st->set_debugger_socket((int)connect_handle);
     
    _wait_for_accept = false;

    return true;

#if 0
      /*
       * Turn off Nagle algorithm which is slow in NT. Since we send many
       * small packets Nagle actually slows us down as we send the packet
       * header in tiny chunks before sending the data portion. Without this
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

bool SocketTransport::initialized(Transport *t)
{
  SocketTransport *st = (SocketTransport *)t;
  return ((void*)st->debugger_socket() != INVALID_HANDLE);
}

void SocketTransport::disconnect_transport(Transport *t)
{
  UsingFastOops fastoops;
  SocketTransport *st = (SocketTransport *)t;
  void* dbg_handle = (void*)st->debugger_socket();

  if (Verbose) {
    tty->print_cr("SocketTransport::disconnect_transport()");
  }

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

void SocketTransport::destroy_transport(Transport *t)
{
  UsingFastOops fastoops;
  SocketTransport *st = (SocketTransport *)t;
  void* listener_handle = (void*)st->listener_socket();

  if (Verbose) {
    tty->print_cr("SocketTransport::destroy_transport()");
  }

  st->reset_read_ahead_buffer();

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
  void* dbg_handle = (void*)st->debugger_socket();
  int bytesAvailable = 0;
  (void)timeout; // IMPL_NOTE: take timeout into account

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
  void* dbg_handle = (void*)st->debugger_socket();
  int bytes_sent = 0;
  int status;
  static void* pContext;

  if (Verbose) {
    tty->print_cr("SocketTransport::write_bytes()");
  }

  if (dbg_handle == INVALID_HANDLE) {
    return 0;
  }

  if (!_wait_for_write) {
    status = pcsl_socket_write_start(dbg_handle, (char*)buf,
                                     len, &bytes_sent, &pContext);
  } else {  /* Reinvocation after unblocking the thread */
    status = pcsl_socket_write_finish(dbg_handle, (char*)buf,
                                      len, &bytes_sent, pContext);
  }

  if (status == PCSL_NET_WOULDBLOCK) {
    if (Verbose) {
      tty->print_cr("SocketTransport::write_bytes(): waiting for write");
    }
    _wait_for_write = true;
    return 0;
  }

  _wait_for_write = false;

  if (status != PCSL_NET_SUCCESS) {
#ifdef AZZERT
    tty->print_cr("SocketTransport: pcsl_socket_write_*() failed");
#endif
    return 0;
  }

  return bytes_sent;
}

int SocketTransport::peek_bytes(Transport *t, void *buf, int len)
{
  UsingFastOops fastoops;

  if (Verbose) {
    tty->print_cr("SocketTransport::peek_bytes()");
  }

  return read_bytes_impl(t, buf, len, false, true);
}

int SocketTransport::read_bytes(Transport *t, void *buf, int len, bool blockflag)
{
  UsingFastOops fastoops;

  if (Verbose) {
    tty->print_cr("SocketTransport::read_bytes()");
  }

  return read_bytes_impl(t, buf, len, blockflag, false);
}

int SocketTransport::read_bytes_impl(Transport *t, void *buf, int len,
                                     bool blockflag, bool peekOnly)
{
  UsingFastOops fastoops;
  int nread;
  unsigned int nleft = len;
  unsigned char *ptr = (unsigned char *) buf;
  unsigned int total = 0;
  SocketTransport *st = (SocketTransport *)t;
  void* dbg_handle = (void*)st->debugger_socket();
  int status;
  static void *pContext;

  if (Verbose) {
    tty->print_cr("SocketTransport::read_bytes_impl()");
  }

  if (dbg_handle == INVALID_HANDLE) {
    return 0;
  }
  if (nleft == 0) {
    // trying to read 0 bytes, just return
    return 0;
  }

  int bytes_cached = st->bytes_cached_for_read();
  TypeArray read_buffer = st->read_ahead_buffer();

  if (bytes_cached >= len) {
    jvm_memcpy((unsigned char *)buf, read_buffer.base_address(), len);
    if (!peekOnly) {
      st->set_bytes_cached_for_read(bytes_cached - len);
    }

    return len;
  } else {
    if (bytes_cached > 0) {
      jvm_memcpy((unsigned char *)buf, read_buffer.base_address(), bytes_cached);
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
    if (!_wait_for_read) {
      status = pcsl_socket_read_start(dbg_handle, ptr, nleft,
                                      &nread, &pContext);
    } else {  /* Reinvocation after unblocking the thread */
      status = pcsl_socket_read_finish(dbg_handle,  ptr, nleft,
                                       &nread, pContext);
    }

    if (status == PCSL_NET_WOULDBLOCK) {
      if (Verbose) {
        tty->print_cr("SocketTransport::read_bytes_impl(): waiting for read");
      }
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
      bool success = st->add_to_read_ahead_buffer(ptr, nread);
      if (!success) {
        break;
      }
    }

    total += nread;
    nleft -= nread;
    ptr   += nread;
  } while (nleft);

  if (Verbose) {
    jvm_printf("SocketTransport: bytes read: %d\n", (int)total);
  }
  
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

bool SocketTransport::add_to_read_ahead_buffer(unsigned char* buf_to_add,
                                               int len)
{
  UsingFastOops fastoops;
  int cache_size = read_ahead_buffer_size();
  int bytes_cached = bytes_cached_for_read();
  TypeArray read_buffer = read_ahead_buffer();
  
  if (Verbose) {
    tty->print_cr("add_to_read_ahead_buffer()");
  }
  
  if (len <= 0 || buf_to_add == NULL) {
    return false;
  }

  if (cache_size < bytes_cached + len) {
    SETUP_ERROR_CHECKER_ARG;
    int new_cache_size = cache_size + len;
    TypeArray tmp_buf = Universe::new_byte_array(new_cache_size JVM_NO_CHECK);

    if (CURRENT_HAS_PENDING_EXCEPTION) {
      Thread::clear_current_pending_exception();
      return false;
    }

    if (bytes_cached > 0) {
      TypeArray::array_copy(&read_buffer, 0, &tmp_buf, 0, bytes_cached);
    }

    // update the object if it was changed
    set_read_ahead_buffer(&tmp_buf);
    read_buffer = read_ahead_buffer();

    if (Verbose) {
      jvm_printf("new cache size: %d\n", new_cache_size);
    }
  }

  jvm_memcpy(read_buffer.base_address() + bytes_cached, buf_to_add, len);
  set_bytes_cached_for_read(bytes_cached + len);
  
  return true;
}

#if USE_BSD_SOCKET
extern "C" int JVM_GetDebuggerSocketFd() {
  Transport::Raw t = Universe::transport_head();
  SocketTransport::Raw st = t().obj();
  return ((int)st().debugger_socket());
}
#endif

/**
 * Informs the VM that a socket's status was changed.
 *
 * Note: this function is not needed for Javacall builds.
 *
 * @param handle Platform specific handle
 * @param signalType Enumerated signal type
 */
extern "C" void
NotifySocketStatusChanged(long handle, int waitingFor)
{
  // do nothing
  (void)handle;
  (void)waitingFor;
}

#endif // ENABLE_JAVA_DEBUGGER

#endif // ENABLE_PCSL
