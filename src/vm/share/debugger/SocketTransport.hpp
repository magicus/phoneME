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

/** \class SocketTransport
    Handles the communication between the VM and the debug agent over
    TCP/IP sockets

*/

#if ENABLE_JAVA_DEBUGGER

class SocketTransport : public Transport {

public:
  HANDLE_DEFINITION(SocketTransport, Transport);

  static char *name();
  static void disconnect_transport(Transport *t);

  static bool char_avail(Transport *t,int timeout);
  static int write_bytes(Transport *t, void *buf, int len);
  static int write_int(Transport *t, void *buf);
  static int write_short(Transport *t, void *buf);
  static void flush(Transport *t);
  static int peek_bytes(Transport *t, void *buf, int len);
  static int read_bytes(Transport *t, void *buf, int len, int blockflag);
  static int read_int(Transport *t, void *buf);
  static short read_short(Transport *t, void *buf);

  static void init_transport(void *t JVM_TRAPS);
  static void destroy_transport(Transport *t);
  static bool initialized(Transport *t);
  static bool connect_transport(Transport *t, ConnectionType type, int timeout);

  static Transport::transport_op_def_t socket_transport_ops;
  static ReturnOop new_transport(JVM_SINGLE_ARG_TRAPS);

  int debugger_socket() {
    return int_field(debugger_socket_offset());
  }

  void set_debugger_socket(int socket) {
    int_field_put(debugger_socket_offset(), socket);
  }

  int listener_socket() {
    return int_field(listener_socket_offset());
  }

  void set_listener_socket(int socket) {
    int_field_put(listener_socket_offset(), socket);
  }

  static ReturnOop allocate(JVM_SINGLE_ARG_TRAPS) {
    return Universe::new_mixed_oop(MixedOopDesc::Type_Transport,
                                   SocketTransportDesc::allocation_size(),
                                   SocketTransportDesc::pointer_count()
                                   JVM_NO_CHECK_AT_BOTTOM);
  }
private:
  static bool _first_time;

  static int debugger_socket_offset() {
    return (FIELD_OFFSET(SocketTransportDesc, _debugger_socket));
  }
  static int listener_socket_offset() {
    return (FIELD_OFFSET(SocketTransportDesc, _listener_socket));
  }
};
#endif
