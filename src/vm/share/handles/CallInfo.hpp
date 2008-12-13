/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

#if ENABLE_COMPILER && ENABLE_APPENDED_CALLINFO
class CallInfoWriter {
 private:
  CompiledMethod * _compiled_method;
  int _current_callinfo_code_offset;
  int _current_record_offset; // Offset from end of compiled method.
  int _current_record_header_size; 
  int _compressed_stackmap_length;
  int _previous_stackmap_offset; // Offset from end of compiled method.
  int _previous_stackmap_size;

  CompiledMethod * compiled_method() const {
    return _compiled_method;
  }

  int current_record_offset() const {
    return _current_record_offset;
  }

  int compressed_stackmap_length() const {
    return _compressed_stackmap_length;
  }

  void update_current_record_offset(jint delta);
  void update_method_end_based_offsets(jint delta);

  void set_record_header_size(int size);
  int record_header_size() const;

  void set_previous_stackmap_offset(int offset);
  int previous_stackmap_offset() const;

  void set_previous_stackmap_size(int size);
  int previous_stackmap_size() const;

  bool stackmap_same_as_previous(int stackmap_offset, 
                                 int stackmap_size) const;

  int callinfo_code_offset() const {
    return _current_callinfo_code_offset;
  }
  void set_callinfo_code_offset(int offset) {
    _current_callinfo_code_offset = offset;
  }
  // Returns the length of this record header.
  int write_record_header(int code_offset_from_previous, 
                          int bci, int stackmap_size JVM_TRAPS);
  // Encodes the value, writes it into the compiled method at the given offset
  // and returns the number of bytes written.
  int write_encoded_value(int value, int offset);
  // Returns the size of encoded value for the given value.
  int encoded_value_size(int value);
  void allocate_record(size_t record_size JVM_TRAPS);
  void adjust_table_size(int delta JVM_TRAPS);
  enum {
    EXPANSION_DELTA = 256
  };
 public:
  void initialize(CompiledMethod * const compiled_method) {
    _compiled_method = compiled_method;
    _current_callinfo_code_offset = 0;
    _current_record_offset = 0;
    _compressed_stackmap_length = 0;
    _previous_stackmap_offset = 0;
    _previous_stackmap_size = 0;
  }

  // Returns false if failed to allocate a new record in the table.
  void start_record(int code_offset, int bci, int stackmap_size JVM_TRAPS);
  void write_oop_tag_at(const int index);
  void commit_record();
  void commit_table();
};

class CallInfoRecord {
 private:
  const CompiledMethod * _compiled_method;
  int _current_record_offset;
  int _current_record_code_offset;
  int _current_record_bci;
  int _current_record_stackmap_offset;
  int _current_record_stackmap_size;
  int _current_record_size;

  const CompiledMethod * compiled_method() const {
    return _compiled_method;
  }
  int record_offset() const {
    return _current_record_offset;
  }
  jubyte record_ubyte_at(int offset) const {
    return compiled_method()->ubyte_field(record_offset() + offset);
  }

  // Reads the encoded value at the given offset, decodes it and returns and
  // returns the number of bytes read.
  int read_encoded_value(int& value, int offset);
  int compute_stackmap_size(int stackmap_offset);

  int stackmap_size() const;
  int stackmap_offset() const;
  int record_code_offset() const;
  int record_size() const;
  void read_record();
  void next_record();
  void init(const CompiledMethod * const compiled_method,
            const address return_address);
 public:
  CallInfoRecord(const CompiledMethod * const compiled_method,
                 const address return_address) {
    // NB. init() is used here to fix CR 6297942.
    init(compiled_method, return_address);
  }

  int bci() const {
    return _current_record_bci;
  }
  bool oop_tag_at(int index) const;

  static void index_to_offsets(int index, int& byte_offset, int& bit_offset);

  static size_t table_size(const jushort header) {
#ifdef AZZERT
    const jubyte kind = bitfield(header, 
                                 CallInfoRecord::type_start, 
                                 CallInfoRecord::type_width);

    GUARANTEE(kind == Relocation::callinfo_type, "Must be callinfo kind");
#endif
    return bitfield(header, 
                    CallInfoRecord::length_start, 
                    CallInfoRecord::length_width) << LogBytesPerShort;
  }

  static jushort table_header(const size_t table_size) {
    GUARANTEE((table_size & (BytesPerShort - 1)) == 0, 
              "Table size must be short-aligned");
    
    const jushort header = 
      (Relocation::callinfo_type << CallInfoRecord::type_start) | 
      (table_size >> LogBytesPerShort);
    return header;
  }

  static bool table_size_out_of_bounds(const size_t table_size) {
    return (table_size >> LogBytesPerShort) > 
      right_n_bits(CallInfoRecord::length_width);
  }

  enum {
    same_stackmap_bit = 0x1,
    value_end_bit     = 0x80
  };

  // Table header layout.
  enum {
    length_width = Relocation::offset_width,
    type_width   = Relocation::type_width,
    length_start = 0,
    type_start   = length_start + length_width,
    table_header_size = 2
  };
};
#endif // ENABLE_COMPILER && ENABLE_APPENDED_CALLINFO
