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

/**
   \class PacketStream
   Handles the processing of packets for the debugger code.
 */

#if ENABLE_JAVA_DEBUGGER
#define COMMAND_ARGS PacketInputStream *in, PacketOutputStream *out
        
class PacketStream : public StackObj{

public:

  PacketStream(Transport *transport) {

    _transport = transport;
    _transport_ops = transport->ops();
    _error = 0;
    _pkt._x._cmdset._flags = 0;
    _is_cached_buffer = false;
  }

  unsigned char flags() {
    return _pkt._x._cmdset._flags;
  }
  unsigned char cmd_set() {
    return _pkt._x._cmdset._cmd_set;
  }
  unsigned char cmd() {
    return _pkt._x._cmdset._cmd;
  }
  short error(){
    return _error;
  }
  int id() {
    return _pkt._id;
  }

  int data_len() {
    return _data_len;
  }
  void set_data_len(int len) {
    _data_len = len;
  }
  void set_flags(unsigned char flags) {
    _pkt._x._cmdset._flags = flags;
  }
  void set_cmd_set(unsigned char cmd_set) {
    _pkt._x._cmdset._cmd_set = cmd_set;
  }
  void set_cmd(unsigned char cmd) {
    _pkt._x._cmdset._cmd = cmd;
  }
  void set_id(int id) {
    _pkt._id = id;
  }
  void set_error(short error) {
  _error = error;
  }

  Transport::transport_op_def_t *transport_ops() {
    return _transport_ops;
  }

  Transport * transport() {
    return &_transport;
  }

  static int unique_id();
  void read_bytes(void *, int);
  void write_bytes(char *, int);

  void restore_buffer(TypeArray *buf) {
    ObjArray::Raw plist = Universe::packet_buffer_list();
    GUARANTEE(!plist.is_null(), "Have cached buffer but no buffer list");
    if (!plist.is_null()) {
      for (int i = 0; i < PacketStream::NUM_PACKET_BUFS; i++) {
        TypeArray::Raw data = plist().obj_at(i);
        if (data.is_null()) {
          plist().obj_at_put(i, buf);
          break;
        }
      }
    }
  }

  ReturnOop get_buffer(int size JVM_TRAPS) {
    TypeArray::Raw buf;
#if ENABLE_MEMORY_PROFILER
    if (size + JDWP_HEADER_SIZE + 16 > 3000 && size + JDWP_HEADER_SIZE + 16 < 4000) {
      return Universe::packet_buffer_list()->obj_at(PacketStream::NUM_PACKET_BUFS);
    }
#endif
    if ((size + JDWP_HEADER_SIZE + 16) < PacketStream::INITIAL_SEGMENT_SIZE) {
      ObjArray::Raw plist = Universe::packet_buffer_list();
      if (!plist.is_null()) {
        for (int i = 0; i < PacketStream::NUM_PACKET_BUFS; i++) {
          buf = plist().obj_at(i);
          if (!buf.is_null()) {
            plist().obj_at_clear(i);
            _is_cached_buffer = true;
            break;
          }
        }
      }
    }
    if (buf.is_null()) {
      //add in header size and some safety zone
      SAVE_CURRENT_EXCEPTION;
      buf =
        Universe::new_byte_array((size + JDWP_HEADER_SIZE + 16) JVM_NO_CHECK);
      if (buf.is_null()) {
        // out of memory ?
        set_error(JDWP_Error_OUT_OF_MEMORY);
      }
      RESTORE_CURRENT_EXCEPTION;
    }
    return buf;
  }

  enum {
    NUM_PACKET_BUFS = 4,
    JDWP_HEADER_SIZE = 11,
    INITIAL_SEGMENT_SIZE = 128,
    MAX_BUF_INC = 2048
  };


  static int current_id;

  jubyte* current_output_addr() {
    return _input_data.base_address() + (sizeof(jubyte) * _output_index);
  }

  TypeArray _input_data;

  Transport _transport;
  Transport::transport_op_def_t * _transport_ops;
  int _data_len;
  short _error;
  struct debugger_packet {
    int _pkt_len;
    int _id;
    union {
      // This is so offsets on ARM are byte aligned, not word aligned
      struct {
        unsigned char _flags;
        unsigned char _cmd_set;
        unsigned char _cmd;
      } _cmdset;
      struct {
        unsigned char _flags;
        unsigned char _pkt_error_0;
        unsigned char _pkt_error_1;
      } _error;
    } _x;
  }_pkt;

  int _output_index;
  bool _is_cached_buffer;
};

/** \class PacketInputStream
    Handles the reading of data from the debug agent.
 */

class PacketInputStream :public PacketStream {

public:

  PacketInputStream(Transport *transport) :  PacketStream(transport){
    _output_index = 0;
  }

  ~PacketInputStream() {
    if (!_input_data.is_null() && _is_cached_buffer) {
      PacketStream::restore_buffer(&_input_data);
    }
  }
  void init();
  void command();
  jboolean read_boolean();
  jbyte read_byte();
  jchar read_char();
  jshort read_short();
  jint read_int();
  jfloat read_float() {
    int rv = read_int();
    return *(float*)&rv;
  }
  jdouble read_double() {
    // IMPL_NOTE: consider whether it is right 
    return double_from_bits(read_long());
  }
  jlong read_long();
  ReturnOop read_raw_string(int& JVM_TRAPS);
  ReturnOop read_string();
  ReturnOop read_object();
  jint read_object_id() {
    return read_int();
  }
  jlong read_method_id() {
    return read_long();
  }
  int read_thread_id() {
    return read_int();
  }
  int read_class_id() {
    return read_int();
  }
  ReturnOop read_thread_ref();
  ReturnOop read_class_ref() {
    return read_object();
  }
  bool receive_packet();
  void flush(Transport::transport_op_def_t *ops);
  void flush_and_send_oom(Transport::transport_op_def_t *ops);

};

/** \class PacketoutputStream
    Handles the writing of data to the debug agent.
 */

class PacketOutputStream : public PacketStream {

  TypeArray _output_data;
  int _input_index;
  int _buffer_inc;
public:
  PacketOutputStream(Transport *transport, jint data_len = 0,
                     unsigned char cmd_set = 0,
                     unsigned char cmd = 0) : PacketStream(transport){
    SETUP_ERROR_CHECKER_ARG;
    // check free list
    _output_data = PacketStream::get_buffer(data_len JVM_NO_CHECK);
    _input_index = JDWP_HEADER_SIZE;
    _buffer_inc = INITIAL_SEGMENT_SIZE;

    _pkt._x._cmdset._cmd_set = cmd_set;
    _pkt._x._cmdset._cmd = cmd;
    _pkt._id = unique_id();
  }

  ~PacketOutputStream() {
    if (!_output_data.is_null() && _is_cached_buffer) {
      PacketStream::restore_buffer(&_output_data);
    }
  }

  void init_reply(jint id);
  virtual void write_bytes(char *, int);
  virtual void write_bytes(Buffer *, int);
  virtual void write_bytes(Symbol *, int);
  void write_boolean(jboolean val);
  virtual void write_byte(jbyte val);
  void write_char(jchar val);
  void write_short(jushort val);
  virtual void write_int(juint val);
  virtual void write_long(julong val);
  void write_class(Oop* clazz) {
    write_object(clazz);
  }
  void write_object(Oop * object);
  void write_string(String *);
  void write_raw_string(const char *string);
  void write_float(jfloat val) {
    jint v = *(jint*)&val;
    write_int(v);
  }
  void write_double(jdouble val) {
    jlong l = double_bits(val);
    write_long(l);
  }
  static int get_class_name_length(JavaClass *jc);
  void write_class_prepare_info(JavaClass *clazz);
  void write_class_name(JavaClass *jc);
  void write_thread(Thread * thread);

  ReturnOop data_buffer() {
    return _output_data.obj();
  }
  int free_bytes() {
    return _output_data.length() - _input_index;
  }
  
  void send_packet();
  void send_error(int error) {
    set_error(error);
    send_packet();
  }

  jubyte* current_input_addr() {
    return _output_data.base_address() + (sizeof(jubyte) * _input_index);
  }
  void check_buffer_size(int size);
};

#define HOST_TO_JAVA_CHAR(x)  Bytes::put_Java_u2((address)&x, x)
#define HOST_TO_JAVA_SHORT(x) Bytes::put_Java_u2((address)&x, x)
#define HOST_TO_JAVA_INT(x)   Bytes::put_Java_u4((address)&x, x)
#define HOST_TO_JAVA_LONG(x)  Bytes::put_Java_u8((address)&x, x)

#define JAVA_TO_HOST_CHAR(x)  Bytes::get_Java_u2((address)&x)
#define JAVA_TO_HOST_SHORT(x) Bytes::get_Java_u2((address)&x)
#define JAVA_TO_HOST_INT(x)   Bytes::get_Java_u4((address)&x)
#define JAVA_TO_HOST_LONG(x)  Bytes::get_Java_u8((address)&x)

#define FLAGS_REPLY ((unsigned char)0x80)
#define FLAGS_NONE  ((unsigned char)0x0)
#define REPLY_NO_ERROR ((short)0x0)

#endif
