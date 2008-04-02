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

/** \class Transport
    Base class used to create debugger transports

*/

#if ENABLE_JAVA_DEBUGGER

class Transport : public Oop {

public:

  HANDLE_DEFINITION(Transport, Oop);

  enum ConnectionType {
    SERVER = 0,
    CLIENT = 1
  };

  typedef struct transport_op_def {
    char *(*name)();
    bool (*char_avail)(Transport *t, int timeout);
    int (*write_bytes)(Transport *t, void *buf, int len);
    int (*write_int)(Transport *t, void *buf);
    int (*write_short)(Transport *t, void *buf);
    void (*flush)(Transport *t);
    int (*peek_bytes)(Transport *t, void *buf, int len);
    int (*read_bytes)(Transport *t, void *buf, int len, bool blockflag);
    int (*read_int)(Transport *t, void *len);
    short (*read_short)(Transport *t, void *buf);
    void (*destroy_transport)(Transport *t);
    bool (*initialized)(Transport *t);
    bool (*connect_transport)(Transport *t, ConnectionType ct, int timeout);
    void (*disconnect_transport)(Transport *t);
  }transport_op_def_t;


  static void init_all_transports(JVM_SINGLE_ARG_TRAPS);
  static ReturnOop new_transport(const char *name JVM_TRAPS);

  struct transport_type {
    const char *name;
    void (*_init_transport)(void *t JVM_TRAPS);   // init routine for transport
    ReturnOop (*_new_transport)(JVM_SINGLE_ARG_TRAPS);    // new transport
  };
  static struct transport_type transport_types[1];

  transport_op_def_t * ops() {
    return (transport_op_def_t *)int_field(ops_offset());
  }
  void set_ops(transport_op_def_t *ops) {
    int_field_put(ops_offset(), (int)ops);
  }

  ReturnOop next() { return obj_field(next_offset()); }
  void set_next(Oop *p) { obj_field_put(next_offset(), p); }

  int task_id() { return int_field(task_id_offset()); }
  void set_task_id(int value) { int_field_put(task_id_offset(), value); }

  int flags() { return int_field(flags_offset()); }
  void set_flags(int value) { int_field_put(flags_offset(), value); }

  static ReturnOop allocate(JVM_SINGLE_ARG_TRAPS) {
    Transport::Raw t = Universe::new_mixed_oop(MixedOopDesc::Type_VMEvent,
                           TransportDesc::allocation_size(),
                           TransportDesc::pointer_count()
                           JVM_NO_CHECK_AT_BOTTOM);
    return t;
  }

  enum {
    // flags
    SEND_CLASS_PREPARES= 1,
    CLASS_PREPS_SENT   = 2
  };

  void set_connection_confirmed(int connection_confirmed) {
    int_field_put(connection_confirmed_offset(), connection_confirmed);
  }
  int connection_confirmed() {
    return int_field(connection_confirmed_offset());
  }
private:
  static int connection_confirmed_offset() {
    return (FIELD_OFFSET(TransportDesc, _connection_confirmed));
  }

private:
  static int ops_offset() { return FIELD_OFFSET(TransportDesc, _ops); }
  static int next_offset() { return FIELD_OFFSET(TransportDesc, _next); }
  static int task_id_offset() { return FIELD_OFFSET(TransportDesc, _task_id); }
  static int flags_offset() { return FIELD_OFFSET(TransportDesc, _flags); }
};

#endif
