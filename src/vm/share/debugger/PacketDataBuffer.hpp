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

/** \class PacketDataBuffer
    Holds data for reading/writing from/to the debug agent.  Has functions
    to return the total length of the linked list of buffers as well as
    functions to set/get the next linked buffer.

*/
#if ENABLE_JAVA_DEBUGGER
class PacketDataBuffer : public MixedOop {

public:
  HANDLE_DEFINITION(PacketDataBuffer, MixedOop);

  void set_next(Oop * p) {obj_field_put(next_offset(), p);}
  ReturnOop next(){ return obj_field(next_offset());}

  void set_data(Oop * p) {obj_field_put(data_offset(), p);}
  ReturnOop data(){ return obj_field(data_offset());}

  jint data_length() { return int_field(data_length_offset());}
  void set_data_length(jint len) { int_field_put(data_length_offset(), len);}

  int total_data_length()
  {
    int len = data_length();
    PacketDataBuffer this_buf = next();
    while (this_buf.not_null()) {
      len += this_buf.data_length();
      this_buf = this_buf.next();
    }
    return len;
  }
  static int data_length_offset() {
    return FIELD_OFFSET(PacketDataBufferDesc, _data_length);
  }
  static int next_offset() {
    return FIELD_OFFSET(PacketDataBufferDesc, _next);
  }
  static int data_offset() {
    return FIELD_OFFSET(PacketDataBufferDesc, _data);
  }

  static ReturnOop allocate(JVM_SINGLE_ARG_TRAPS) {
    return Universe::new_mixed_oop(MixedOopDesc::Type_PacketDataBuffer,
                                   PacketDataBufferDesc::allocation_size(),
                                   PacketDataBufferDesc::pointer_count()
                                   JVM_NO_CHECK_AT_BOTTOM);
  }
};
#endif
