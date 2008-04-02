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
# include "incls/_DebuggerStream.cpp.incl"

/*=========================================================================
 * Functions
 *=======================================================================*/

#if ENABLE_JAVA_DEBUGGER

void
PacketInputStream::init()
{
}

void
PacketStream::read_bytes(void *dest, int size)
{

  if (_error) {
    return;
  }
  if (dest) {
    if (size > (_input_data.length() - _output_index)) {
      set_error(JDWP_Error_ILLEGAL_ARGUMENT);
      return;
    }
    jvm_memcpy(dest, current_output_addr(), size);
    _output_index += size;
  }
}

void
PacketStream::write_bytes(char* source, int size)
{
  Transport::transport_op_def_t *ops = _transport_ops;
  ops->write_bytes(&_transport, source, size);
}

void PacketInputStream::flush(Transport::transport_op_def_t *ops) {
  int nread;
  char c;
  do {
    nread = ops->read_bytes(&_transport, &c, 1, false);
  } while (nread > 0);
}

void PacketInputStream::flush_and_send_oom(Transport::transport_op_def_t *ops) {
  flush(ops);
  PacketOutputStream out(&_transport);
  out.init_reply(id());
  out.send_error(JDWP_Error_OUT_OF_MEMORY);
}

bool
PacketInputStream::receive_packet()
{
  juint len, id;
  int bytes_avail;
  unsigned char uc;
  SETUP_ERROR_CHECKER_ARG;

  Transport::transport_op_def_t *ops = _transport_ops;

  bytes_avail = ops->peek_bytes(&_transport, &len, sizeof(len));
  if (bytes_avail < sizeof(len)) {
    // Try later, not enough bytes to get the packet length
    return false;
  }
  len = JAVA_TO_HOST_INT(len);
  if (len < JDWP_HEADER_SIZE) {
    return false;
  }
  if (len > JDWP_HEADER_SIZE) {
    SAVE_CURRENT_EXCEPTION;
    _input_data = PacketStream::get_buffer(len - JDWP_HEADER_SIZE JVM_NO_CHECK);
    RESTORE_CURRENT_EXCEPTION;
    if (_input_data.is_null()) {
      // not enough memory for this packet buffer
      flush_and_send_oom(ops);
      return false;
    }
  }
  ops->read_int(&_transport, &len);
  len = JAVA_TO_HOST_INT(len);
  if (len < JDWP_HEADER_SIZE) {
    flush(ops);
    return false;
  }
  set_data_len(len - JDWP_HEADER_SIZE);
  ops->read_int(&_transport, &id);
  id = JAVA_TO_HOST_INT(id);
  set_id(id);

  ops->read_bytes(&_transport, &uc, sizeof(uc), true);
  set_flags(uc);
  if (flags() & FLAGS_REPLY) {
    short error_code;
    ops->read_short(&_transport, &error_code);
    error_code = JAVA_TO_HOST_SHORT(error_code);
    set_error(error_code);
  } else {
    ops->read_bytes(&_transport, &uc, sizeof(uc), true);
    set_cmd_set(uc);
    ops->read_bytes(&_transport, &uc, sizeof(uc), true);
    set_cmd(uc);
  }
  if (len > JDWP_HEADER_SIZE) {
    // read in the whole packet
    ops->read_bytes(&_transport, _input_data.base_address(),
                    len - JDWP_HEADER_SIZE, true);
  }
 _output_index = 0;
  return true;
}

jboolean 
PacketInputStream::read_boolean()
{
  unsigned char flag;
  read_bytes(&flag, sizeof(flag));
  if (error()) {
    return 0;
  } else {
    return flag ? (jboolean)true : (jboolean)false;
  }
}

jbyte 
PacketInputStream::read_byte( )
{
  unsigned char val = 0;
  read_bytes(&val, sizeof(val));
  return val;
}

jchar
PacketInputStream::read_char( )
{
  jchar val = 0;
  read_bytes(&val, sizeof(val));
  return JAVA_TO_HOST_CHAR(val);
}

jshort 
PacketInputStream::read_short( )
{
  jshort val = 0;
  read_bytes(&val, sizeof(val));
  return JAVA_TO_HOST_SHORT(val);
}

jint 
PacketInputStream::read_int( )
{
  int val = 0;
  read_bytes(&val, sizeof(val));
  return JAVA_TO_HOST_INT(val);
}

jlong
PacketInputStream::read_long( )
{
  jlong val = 0;
  read_bytes(&val, sizeof(val));
  return (jlong)(JAVA_TO_HOST_LONG(val));
}

ReturnOop
PacketInputStream::read_raw_string(int& length JVM_TRAPS) {

  UsingFastOops fast_oops;

  length = read_int();

  // LiteralStream barfs if it's got a null
  TypeArray::Fast ta = Universe::new_byte_array(length  JVM_NO_CHECK);
  if (ta.is_null()) {
    return NULL;
  }
  read_bytes(ta().base_address(), length);
  return ta;
}

ReturnOop 
PacketInputStream::read_string( )
{
  SAVE_CURRENT_EXCEPTION;

  UsingFastOops fast_oops;

  SETUP_ERROR_CHECKER_ARG;
  int length;
  TypeArray::Fast ta = read_raw_string(length JVM_NO_CHECK);
  if (ta.is_null()) {
    RESTORE_CURRENT_EXCEPTION;
    return NULL;
  }
  LiteralStream ls((char *)ta().base_address(), 0, length);
  String s = Universe::new_string(&ls JVM_NO_CHECK);
  RESTORE_CURRENT_EXCEPTION;
  return s;
}

ReturnOop
PacketInputStream::read_object()
{
  return JavaDebugger::get_object_by_id(read_int());
}

ReturnOop
PacketInputStream::read_thread_ref()
{
  ThreadObj::Raw t = JavaDebugger::get_object_by_id(read_int());
  if (t.is_null()) {
    return (t);
  } else {
    return (t().thread());
  }
}

void 
PacketOutputStream::init_reply(jint id)
{

  _error = JDWP_Error_NONE;
  _pkt._id = id;
  _error = REPLY_NO_ERROR;
  _pkt._x._cmdset._flags = FLAGS_REPLY;
}

void
PacketOutputStream::write_string(String *s)
{
  UsingFastOops fast_oops;
  TypeArray::Fast t;
  Buffer::Fast b;
  int unilen;
  int offset;

  SETUP_ERROR_CHECKER_ARG;

  SAVE_CURRENT_EXCEPTION;
  if (!s->is_string()) {
    // Debugger sent us a bad request? Just write a blank string.
    // This happens when running javasoft.sqe.tests.lang.excp004.excp00406.excp00406_wrapper
    // in S-KDWP test suite
    t = Universe::new_char_array(0 JVM_NO_CHECK);
    unilen = 0;
    offset = 0;
  } else {
    t = s->value();
    unilen = s->length();
    offset = s->offset();
  }

  if (t.is_null()) {
    // out of memory??  Just write a zero length string
    RESTORE_CURRENT_EXCEPTION;
    write_int(0);
    return;
  }
  int i, index;
  jchar value;
  GUARANTEE((unilen + offset) <= t().length(), "String out of bounds");
  b = Universe::new_byte_array((unilen * 3) JVM_NO_CHECK);
  if (b.is_null()) {
    // out of memory??  Just write a zero length string
    write_int(0);
    RESTORE_CURRENT_EXCEPTION;
    return;
  }
  ByteStream bs(&b, 0, b().length());
  for (index = 0, i = 0; i < unilen; i++) {
    value = t().char_at(i + offset);
    index = bs.utf8_write(index, value);
  }
  write_int(index);
  write_bytes(&b, index);
  RESTORE_CURRENT_EXCEPTION;
}

void
PacketOutputStream::write_raw_string(const char * string)
{
  int length = jvm_strlen((char *)(string));
  write_int(length);
  write_bytes((char*)string, length);
}

void
PacketOutputStream::write_thread(Thread * thread)
{ 

  if (thread->is_null()) { 
    write_int(0);
  } else {
    UsingFastOops fast_oops;
    ThreadObj::Fast t = thread->thread_obj();
    write_object(&t);
  }
}

int
PacketOutputStream::get_class_name_length(JavaClass *jc) {  
  Symbol::Raw klass_name = jc->name();
  int length = 0;

  if (!jc->is_instance_class()) {
    JavaClass::Raw klass = jc->obj(); 
    for(;;) {
      if (klass().is_obj_array_class()) {
        length++;  // for the '['
        ObjArrayClass::Raw oac = klass.obj();
        klass = oac().element_class();
      } else if (klass().is_type_array_class()) {
        length += 2;  // for the '[' and the base element type
        break;
      } else {
        klass_name = klass().name();
        length += klass_name().length() + 2;  // name length + 'L' + ';'
        break;
      }
    }
  } else {
    length = klass_name().length() + 2;
  }
  return length;
}

void
PacketOutputStream::write_class_name(JavaClass *jc) 
{
  UsingFastOops fast;
  Symbol::Fast klass_name = jc->name();
  JavaClass::Fast klass;
  // First calculate string length
  int length = get_class_name_length(jc);
  write_int(length);

  if (!jc->is_instance_class()) {
    klass = jc->obj();
    for(;;) {
      if (klass().is_obj_array_class()) {
        write_byte('[');
        ObjArrayClass::Raw oac = klass.obj();
        klass = oac().element_class();
      } else if (klass().is_type_array_class()) {
        write_byte('[');
        TypeArrayClass::Raw tac = klass.obj();
        switch (tac().type()) {
        case T_BOOLEAN:   write_byte('Z'); break;
        case T_CHAR:      write_byte('C'); break;
        case T_FLOAT:     write_byte('F'); break;
        case T_DOUBLE:    write_byte('D'); break;
        case T_BYTE:      write_byte('B'); break;
        case T_SHORT:     write_byte('S'); break;
        case T_INT:       write_byte('I'); break;
        case T_LONG:      write_byte('J'); break;
        default: SHOULD_NOT_REACH_HERE();
        }
        return;
      } else {
        GUARANTEE(klass.is_instance_class(), "must be an instance class");
        klass_name = klass().name();
        write_byte('L');
        break;
      }
    }
  } else {
      write_byte('L');
  }
  write_bytes(&klass_name, klass_name().length());
  write_byte(';');
}

void PacketOutputStream::write_class_prepare_info(JavaClass *clazz) {
  write_byte(JavaDebugger::get_jdwp_tagtype(clazz));
  write_class(clazz);
  write_class_name(clazz);
  write_int(JavaDebugger::get_jdwp_class_status(clazz));
}

void
PacketOutputStream::check_buffer_size(int size) {

  SETUP_ERROR_CHECKER_ARG;
  if (size > free_bytes()) {
    SAVE_CURRENT_EXCEPTION;
    TypeArray new_buf = Universe::new_byte_array(_output_data.length() + size +
                                                 _buffer_inc JVM_NO_CHECK);
    RESTORE_CURRENT_EXCEPTION;
    if (new_buf.is_null()) {
      set_error(JDWP_Error_OUT_OF_MEMORY);
	  return;
    }
    if (_buffer_inc < MAX_BUF_INC) {
      _buffer_inc <<= 1;
    }
    TypeArray::array_copy(&_output_data, 0, &new_buf, 0, _output_data.length());
    if (_is_cached_buffer) {
      _is_cached_buffer = false;
      // we started out with one of the cached buffers but needed more room
      // create a new buffer for the cache
      UsingFastOops fastoops;
      ObjArray::Fast plist = Universe::packet_buffer_list();
      GUARANTEE(!plist.is_null(), "Null buffer cache list");
      for (int i = 0; i < PacketStream::NUM_PACKET_BUFS; i++) {
        TypeArray::Raw data = plist().obj_at(i);
        if (data.is_null()) {
          plist().obj_at_put(i, &_output_data);
          break;
        }
      }
    }
    _output_data = new_buf.obj();
  }
}

void
PacketOutputStream::write_bytes(Symbol *source, int size) {
  check_buffer_size(size);
  if (error() != 0) {
    return;
  }
  jvm_memcpy(current_input_addr(), (unsigned char *)source->base_address(),
             size);
  _input_index += size;
}

void
PacketOutputStream::write_bytes(Buffer *source, int size) {
  check_buffer_size(size);
  if (error() != 0) {
    return;
  }
  jvm_memcpy(current_input_addr(), (unsigned char *)source->base_address(),
             size);
  _input_index += size;
}

void
PacketOutputStream::write_bytes(char* source, int size) {

  check_buffer_size(size);
  if (error() != 0) {
    return;
  }
  jvm_memcpy(current_input_addr(), (unsigned char *)source, size);
  _input_index += size;
}

void
PacketOutputStream::write_boolean(jboolean val)
{
  unsigned char byte = (val != 0) ? 1 : 0;
  write_bytes((char*)&byte, sizeof(byte));
}

void
PacketOutputStream::write_byte(jbyte val)
{
  write_bytes((char*)&val, sizeof(val));
}

void
PacketOutputStream::write_char(jchar val)
{
  HOST_TO_JAVA_CHAR(val);
  write_bytes((char*)&val, sizeof(val));
}

void
PacketOutputStream::write_short(jushort val)
{
  HOST_TO_JAVA_SHORT(val);
  write_bytes((char*)&val, sizeof(val));
}

void
PacketOutputStream::write_int(juint val)
{
  HOST_TO_JAVA_INT(val);
  write_bytes((char*)&val, sizeof(val));
}

void
PacketOutputStream::write_long(julong val)
{
  HOST_TO_JAVA_LONG(val);
  write_bytes((char*)&val, sizeof(val));
}

void
PacketOutputStream::write_object(Oop *object)
{ 
  write_int(JavaDebugger::get_object_id_by_ref(object));
}

void
PacketOutputStream::send_packet()
{
  struct debugger_packet *buffer = (struct debugger_packet *)_output_data.data();

  set_data_len(_input_index);
  if (_error) {
    _pkt._pkt_len = JDWP_HEADER_SIZE;
  } else {
    _pkt._pkt_len = data_len();
  }
#ifdef AZZERT
  if (TraceDebugger) {
    //    tty->print_cr("Debugger: pkt: size %d", _pkt._pkt_len);
  }
#endif
  // transfer packet header to buffer
  buffer->_pkt_len = _pkt._pkt_len;
  HOST_TO_JAVA_INT(buffer->_pkt_len);
  buffer->_id = _pkt._id;
  HOST_TO_JAVA_INT(buffer->_id);
  buffer->_x._cmdset._flags = _pkt._x._cmdset._flags;
  if (_pkt._x._cmdset._flags & FLAGS_REPLY) {
    //MSB
    buffer->_x._error._pkt_error_0 = (_error & 0xff00) >> BitsPerByte; 
    //LSB
    buffer->_x._error._pkt_error_1 = _error & 0xff;
  } else {
    buffer->_x._cmdset._cmd_set = _pkt._x._cmdset._cmd_set;
    buffer->_x._cmdset._cmd = _pkt._x._cmdset._cmd;
  }
  PacketStream::write_bytes((char *)_output_data.data(), _pkt._pkt_len);
}

int PacketStream::current_id = 0;

int
PacketStream::unique_id()
{
  return current_id++;
}

#endif /* ENABLE_JAVA_DEBUGGER */
