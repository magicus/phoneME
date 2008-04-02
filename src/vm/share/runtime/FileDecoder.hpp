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

enum {
  MUST_CLOSE_FILE     = 1,
  LAST_BLOCK          = 2,
  INCREMENTAL_INFLATE = 4,
  SYSTEM_CLASSPATH    = 8
};

class FileDecoder : public MixedOop {
public:
  HANDLE_DEFINITION_CHECK(FileDecoder, MixedOop);

protected:
  static int jar_file_name_offset() {
    return FIELD_OFFSET(FileDecoderDesc, _jar_file_name);
  }
  static int file_handle_offset() {
    return FIELD_OFFSET(FileDecoderDesc, _file_handle);
  }
  static int file_pos_offset() {
    return FIELD_OFFSET(FileDecoderDesc, _file_pos);
  }
  static int file_size_offset() {
    return FIELD_OFFSET(FileDecoderDesc, _file_size);
  }
  static int bytes_remain_offset() {
    return FIELD_OFFSET(FileDecoderDesc, _bytes_remain);
  }
  static int flags_offset() {
    return FIELD_OFFSET(FileDecoderDesc, _flags);
  }

  // Ensure that if this FileDecoder is associated with a JAR file,
  // the file handle must be stored in a valid JarFileParser object.
  // See JarFileParser.cpp for more information about JAR file caching,
  // and how it affects FileDecoder.
  void guarantee_jar_file_handle() PRODUCT_RETURN;

public:
  OsFile_Handle file_handle() {
    guarantee_jar_file_handle();
    return (OsFile_Handle) int_field(file_handle_offset());
  }
  void set_file_handle(OsFile_Handle value) {
    int_field_put(file_handle_offset(), (int) value);
  }

  ReturnOop jar_file_name() {
    return obj_field(jar_file_name_offset());
  }
  void set_jar_file_name(TypeArray* value) {
    obj_field_put(jar_file_name_offset(), value);
  }

  int file_pos() {
    return int_field(file_pos_offset());
  }
  void set_file_pos(int value) {
    int_field_put(file_pos_offset(), value);
  }

  int file_size() {
    return int_field(file_size_offset());
  }
  void set_file_size(int value) {
    int_field_put(file_size_offset(), value);
  }

  int bytes_remain() {
    return int_field(bytes_remain_offset());
  }
  void set_bytes_remain(int value) {
    int_field_put(bytes_remain_offset(), value);
  }

  int flags() {
    return int_field(flags_offset());
  }
  void set_flags(int value) {
    int_field_put(flags_offset(), value);
  }

  void add_flags(juint mask) {
    set_flags(flags() | mask);    
  }


  ReturnOop read_completely(JVM_SINGLE_ARG_TRAPS);
  int get_bytes(ArrayPointer* destination, int count JVM_TRAPS);
  static ReturnOop allocate(OsFile_Handle handle, int pos, int size,
                            int flags JVM_TRAPS);

protected:
  int get_bytes_raw(address dest_address, int count);

  // Returns the JarFileParser object associated with this FileDecoder, or
  // NULL if this FileDecoder is not associated with a JarFileParser.
  ReturnOop get_jar_parser_if_needed(JVM_SINGLE_ARG_TRAPS);

  ReturnOop read_completely0(JVM_SINGLE_ARG_TRAPS);
};
