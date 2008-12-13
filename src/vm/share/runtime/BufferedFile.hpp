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

/** \class BufferedFile.hpp
    Supply a buffered stream to file readers.
    Used primarily to load classfiles.  Holds info related the the file
    being read.
 */

class BufferedFile : public MixedOop {

protected:
  static int file_pointer_offset() {
    return FIELD_OFFSET(BufferedFileDesc, _file_pointer);
  }
  static int at_eof_offset() {
    return FIELD_OFFSET(BufferedFileDesc, _at_eof);
  }
  static int data_buffer_offset() {
    return FIELD_OFFSET(BufferedFileDesc, _data_buffer);
  }
  static int buffer_size_offset() {
    return FIELD_OFFSET(BufferedFileDesc, _buffer_size);
  }
  static int file_size_offset() {
    return FIELD_OFFSET(BufferedFileDesc, _file_size);
  }
  static int index_offset() {
    return FIELD_OFFSET(BufferedFileDesc, _index);
  }
  static int count_offset() {
    return FIELD_OFFSET(BufferedFileDesc, _count);
  }
  static int file_pos_offset() {
    return FIELD_OFFSET(BufferedFileDesc, _file_pos);
  }
  static int on_native_heap_offset() {
    return FIELD_OFFSET(BufferedFileDesc, _on_native_heap);
  }
  void refill_buffer();

public:
  HANDLE_DEFINITION(BufferedFile, MixedOop);

  enum {
    BUFFERSIZE = 256
  };

  jint get_byte();
  size_t get_bytes(address, jint, bool is_buffered = true);
  int seek(jint offset, int origin);
  bool open(const JvmPathChar *name, const char *mode);
  int close();
  size_t length();
  void init();
  void init(Buffer *, int, int, int);
  void new_buffer(int size);

  static int base_offset()   {
    return BufferedFileDesc::header_size();
  }

  jboolean on_native_heap() {
    // At this time BufferedFile is never allocated on native heap.
    // To save footprint we just ignore _on_native_heap field.
    return false; // bool_field(on_native_heap_offset());
  }
  void set_on_native_heap(jboolean val) {
    bool_field_put(on_native_heap_offset(), val);
  }
  OsFile_Handle file_pointer() {
    return (OsFile_Handle)int_field(file_pointer_offset());
  }
#ifdef AZZERT
  void junk_file_pointer() {
    int_field_put(file_pointer_offset(), -1);       
  }
#endif

  void set_file_pointer(OsFile_Handle val);

  jboolean at_eof() {
    return bool_field(at_eof_offset());
  }
  void set_at_eof(jboolean val) {
     bool_field_put(at_eof_offset(), val);
  }
  ReturnOop data_buffer() {
    return obj_field(data_buffer_offset());
  }
  void set_data_buffer(Oop *val) {
    if (on_native_heap()) {
      *obj()->obj_field_addr(data_buffer_offset()) = val->obj();
      return;
    }
     obj_field_put(data_buffer_offset(), val);
  }
  jint buffer_size() {
    return int_field(buffer_size_offset());
  }
  void set_buffer_size(jint val) {
     int_field_put(buffer_size_offset(),val);
  }
  jint index() {
    return int_field(index_offset());
  }
  void set_index(jint val) {
     int_field_put(index_offset(),val);
  }
  jint count() {
    return int_field(count_offset());
  }
  void set_count(jint val) {
     int_field_put(count_offset(),val);
  }
  jint file_size() {
    return int_field(file_size_offset());
  }
  void set_file_size(long val) {
    int_field_put(file_size_offset(), (jint)val);
  }
  jint file_pos() {
    return int_field(file_pos_offset());
  }
  void set_file_pos(long val) {
    int_field_put(file_pos_offset(), (jint)val);
  }
  inline jint last_unread_file_pos() {
    return file_pos() - (count() - index());
  }
  static ReturnOop allocate(JVM_SINGLE_ARG_TRAPS);

  // Allocates a new object using the specified alloc_proc.
  // If alloc_proc is NULL, the object is allocated on the Java heap.
  static ReturnOop allocate(JvmAllocProc alloc_proc JVM_TRAPS);
};
