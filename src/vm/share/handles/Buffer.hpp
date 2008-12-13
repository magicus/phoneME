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

typedef TypeArray Buffer;

#if NOT_CURRENTLY_USED

class Buffer : public TypeArray {
 public:
  HANDLE_DEFINITION_WITH_DISPOSING_FAST(Buffer, TypeArray);

  // Accessing data in buffer
  jubyte read_u1(int& index) {
    jint offset = offset_from_byte_index(index);
    index += sizeof(jubyte);
    return (jubyte) ubyte_field(offset);
  }

  // Read jushort from stream
  jushort read_native_u2(int& index) {
    jint offset = offset_from_byte_index(index);
    index += sizeof(jushort);
    return Bytes::get_native_u2((address)field_base(offset));
  }

  // Read jushort from stream
  jushort read_Java_u2(int& index) {
    jint offset = offset_from_byte_index(index);
    index += sizeof(jushort);
    return Bytes::get_Java_u2((address)field_base(offset));
  }

  // Read juint from stream
  juint read_native_u4(int& index) {
    jint offset = offset_from_byte_index(index);
    index += sizeof(juint);
    return Bytes::get_native_u4((address)field_base(offset));
  }

  // Read juint from stream
  juint read_Java_u4(int& index) {
    jint offset = offset_from_byte_index(index);
    index += sizeof(juint);
    return Bytes::get_Java_u4((address)field_base(offset));
  }

  jubyte* base_address() { return (jubyte*) field_base(base_offset()); }

  // Allocates a new byte array using the specified alloc_proc.
  // If alloc_proc is NULL, the array is allocated on the Java heap.
  static ReturnOop allocate(unsigned int length, 
                            JvmAllocProc alloc_proc JVM_TRAPS);
};

#endif
