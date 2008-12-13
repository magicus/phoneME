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

# include "incls/_precompiled.incl"
# include "incls/_CallInfo.cpp.incl"

#if ENABLE_COMPILER && ENABLE_APPENDED_CALLINFO

/*
 * The appended callinfo table format:
 *
 * <callinfo table> := <callinfo record>*[alignment padding]<table header>
 *
 *  Callinfo table resides in relocation, so it must be 2-bytes aligned.
 *  The optional alignment padding ensures 2-byte alignment.
 *
 * <table header> := <1101:12-bit table size>
 * <callinfo record> := <encoded code offset><encoded bci>[<encoded stackmap>]
 *
 * Code offset is encoded as follows: offset delta from the previous callinfo
 * record is divided by Assembler::instruction_encoding, shifted left by 1 and
 * the least significant bit is set to the stackmap presence bit.
 * The resulting value is encoded with right-to-left byte-aligned variable size
 * encoding.
 *
 * The stackmap presence bit is set to 1 if the stackmap is the same as previous.
 * In this case this record doesn't include stackmap.
 *
 * Bci is encoded as is with right-to-left byte-aligned variable size encoding.
 *
 * The stackmap is encoded by dropping zero bits at the end and then with 
 * left-to-right byte-aligned variable size encoding.
 *
 * Left-to-right (right-to-left) byte-aligned variable size encoding
 *  The value to be encoded is split into 7-bit chunks starting from the
 *  leftmost (rightmost) bit. Chunks are written as a sequence of bytes:
 *   <end bit:7-bit chunk>*
 *  The end bit indicates the end of sequence, it is set only in the last byte.
 */
void CallInfoWriter::allocate_record(size_t record_size JVM_TRAPS) {
  const int delta = 
    record_size + CallInfoRecord::table_header_size - current_record_offset();

  if (delta > 0) {
    adjust_table_size(delta JVM_NO_CHECK_AT_BOTTOM);
  }
}

void CallInfoWriter::adjust_table_size(int delta JVM_TRAPS) {
  GUARANTEE(delta != 0, "Sanity");
  GUARANTEE(current_record_offset() + delta >= CallInfoRecord::table_header_size, 
            "Shrinking occupied space");

  CompiledMethod::Raw cm = compiled_method();
  const int header_offset = 
    cm().end_offset() - CallInfoRecord::table_header_size;
  size_t table_size = 0;
  if (current_record_offset() > 0) {
    GUARANTEE((header_offset & 0x1) == 0, "Must be 2-byte aligned");
    GUARANTEE(CallInfoRecord::table_header_size == sizeof(jushort), 
              "2-byte header assumed");
    const jushort header = cm().ushort_field(header_offset);
    table_size = CallInfoRecord::table_size(header);
  }

  int aligned_delta = 0;

  if (delta > 0) {
    aligned_delta = round_to(delta, EXPANSION_DELTA);

    if (!cm().expand_callinfo_table(aligned_delta)) {
      Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW);
    }
  } else {
    // All relocation entries must be 2-byte aligned.
    aligned_delta = round_to(delta, sizeof(short));

    GUARANTEE(aligned_delta <= 0 && aligned_delta >= delta, "Sanity");
    if (aligned_delta == 0) {
      // Nothing to do.
      return;
    }

    cm().shrink_callinfo_table(-aligned_delta);
  }

  update_method_end_based_offsets(aligned_delta);

  // Write new header.
  const size_t new_table_size = table_size + aligned_delta;
  if (CallInfoRecord::table_size_out_of_bounds(new_table_size)) {
    // callinfo table too long, consider another format
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW);
  }
  const jushort new_header = CallInfoRecord::table_header(new_table_size);
  const int new_header_offset = header_offset + aligned_delta;
  GUARANTEE(new_header_offset == 
            cm().end_offset() - CallInfoRecord::table_header_size, "Sanity");
  GUARANTEE((new_header_offset & 0x1) == 0, "Must be 2-byte aligned");
  cm().ushort_field_put(new_header_offset, new_header);
}

int CallInfoWriter::write_encoded_value(int value, int offset) {
  CompiledMethod::Raw cm = compiled_method();
  int written_bytes_count = 0;
  unsigned uvalue = (unsigned)value;
  do {
    jubyte b = uvalue & 0x7f;
    uvalue >>= 7;
    if (uvalue == 0) {
      b |= CallInfoRecord::value_end_bit;
    }
    cm().ubyte_field_put(offset, b); offset += sizeof(jubyte);
    
    written_bytes_count++;
  } while (uvalue != 0);   
  
  return written_bytes_count;
}

int CallInfoWriter::encoded_value_size(int value) {
  int size = 0;
  unsigned uvalue = (unsigned)value;

  do {
    uvalue >>= 7;
    size++;
  } while (uvalue != 0);   

  return size;
}

int CallInfoWriter::write_record_header(int code_offset_from_previous, 
                                        int bci, int stackmap_size JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  CompiledMethod::Raw cm = compiled_method();
  GUARANTEE(code_offset_from_previous >= 0 && bci >= 0 && stackmap_size >= 0,
            "Sanity");
  GUARANTEE(bci >= 0 && bci <= 0xFFFF, "Invalid bci");

  const int record_offset = cm().end_offset() - current_record_offset();
  int offset = record_offset;

  offset += write_encoded_value(code_offset_from_previous, offset);
  offset += write_encoded_value(bci, offset);
  
  const int record_header_size = offset - record_offset;  

  // Initialize the stackmap 
  for (int i = 0; i < stackmap_size; i++) {
    cm().ubyte_field_put(offset, 0); offset += sizeof(jbyte);
  }

  return record_header_size;
}

void CallInfoWriter::start_record(int code_offset, int bci, int number_of_tags 
                                  JVM_TRAPS) {
  const int code_offset_from_previous = code_offset - callinfo_code_offset();
  GUARANTEE(code_offset_from_previous > 0, "Sanity");
  set_callinfo_code_offset(code_offset);

  GUARANTEE((code_offset_from_previous % Assembler::instruction_alignment) == 0, 
            "Invalid instruction alignment");
  // Divide by instruction alignment, since these bits are zero anyway.
  // Shift left by one, since the least significant bit is used to indicate the
  // same stackmap.
  const int shifted_code_offset = 
    (code_offset_from_previous / Assembler::instruction_alignment) << 1;
  const int header_size = 
    encoded_value_size(shifted_code_offset) + encoded_value_size(bci);
  const int stackmap_size = (number_of_tags - 1) / 7 + 1;

  // We allocate space for the maximum possible record size.
  // The actual record size can be less than maximum due to
  // stackmap compression.
  const size_t max_record_size = stackmap_size + header_size;

  allocate_record(max_record_size JVM_CHECK);

  _compressed_stackmap_length = 0;

  const int written_header_size = 
    write_record_header(shifted_code_offset, bci, stackmap_size JVM_CHECK);

  GUARANTEE(header_size == written_header_size, "Sanity");
  AZZERT_ONLY_VAR(written_header_size);

  set_record_header_size(header_size);
}

void CallInfoWriter::write_oop_tag_at(int index) {
  CompiledMethod::Raw cm = compiled_method();
  int byte_offset, bit_offset;
  CallInfoRecord::index_to_offsets(index, byte_offset, bit_offset);
  // Skip the header.
  byte_offset += 
    cm().end_offset() - current_record_offset() + record_header_size();
  intptr_t value = cm().ubyte_field(byte_offset);
  set_nth_bit(value, bit_offset);
  GUARANTEE((value & 0xFF) == value, "Must fit in byte");
  jubyte b = (jubyte)value;
  cm().ubyte_field_put(byte_offset, b);  
  _compressed_stackmap_length = index + 1;
}

inline void CallInfoWriter::update_current_record_offset(jint delta) {
  _current_record_offset += delta;
}

inline void CallInfoWriter::update_method_end_based_offsets(jint delta) {
  update_current_record_offset(delta);
  if (_previous_stackmap_offset != 0) {
    _previous_stackmap_offset += delta;
  }
}

inline void CallInfoWriter::set_record_header_size(int size) {
  GUARANTEE(size > 0, "Size must be positive");
  _current_record_header_size = size;
}

inline int CallInfoWriter::record_header_size() const {
  return _current_record_header_size;
}

inline void CallInfoWriter::set_previous_stackmap_offset(int offset) {
  GUARANTEE(offset > 0, "Offset must be positive");
  _previous_stackmap_offset = offset;    
}

inline int CallInfoWriter::previous_stackmap_offset() const {
  return _previous_stackmap_offset;
}

inline void CallInfoWriter::set_previous_stackmap_size(int size) {
  _previous_stackmap_size = size;
}

inline int CallInfoWriter::previous_stackmap_size() const {
  return _previous_stackmap_size;
}

bool CallInfoWriter::stackmap_same_as_previous(int stackmap_offset,
                                               int stackmap_size) const {
  if (stackmap_size != previous_stackmap_size()) {
    return false;
  }

  CompiledMethod::Raw cm = compiled_method();
  const address end_of_method = (address)cm().obj() + cm().end_offset();
  const address stackmap = end_of_method - stackmap_offset;
  const address previous_stackmap = 
    end_of_method - previous_stackmap_offset();
      
  return jvm_memcmp(stackmap, previous_stackmap, stackmap_size) == 0;
}

void CallInfoWriter::commit_record() {
  // Size of encoded stackmap - divide by 7 and round up.
  const int stackmap_size = (compressed_stackmap_length() - 1) / 7 + 1;
  // Offset from the end of compiled method
  const int stackmap_offset = current_record_offset() - record_header_size();

  CompiledMethod::Raw cm = compiled_method();

  {
    // Set value_end_bit in the last stackmap byte.

    // Offset from the start of compiled method
    const int last_stackmap_byte_offset = 
      cm().end_offset() - stackmap_offset + stackmap_size - 1;
    const jubyte value = cm().ubyte_field(last_stackmap_byte_offset);
    cm().ubyte_field_put(last_stackmap_byte_offset, 
                         value | CallInfoRecord::value_end_bit);
  }

  int record_size = 0;

  if (stackmap_same_as_previous(stackmap_offset, stackmap_size)) {
    // Set the least significant bit in the first byte of the encoded code
    // offset delta to indicate that the stackmap is the same as in the previous
    // delta.
    
    // Offset from the start of compiled method
    const int first_record_byte_offset = 
      cm().end_offset() - current_record_offset();
    const jubyte value = cm().ubyte_field(first_record_byte_offset);
    cm().ubyte_field_put(first_record_byte_offset, 
                         value | CallInfoRecord::same_stackmap_bit);  

    record_size = record_header_size();
  } else {
    set_previous_stackmap_offset(stackmap_offset);
    set_previous_stackmap_size(stackmap_size);

    record_size = record_header_size() + stackmap_size;
  }

  GUARANTEE(record_size > 0, "Sanity");
  GUARANTEE(record_size <= current_record_offset(), "Sanity");
  update_current_record_offset(-record_size);
  GUARANTEE(current_record_offset() >= 0, "Sanity");
}

void CallInfoWriter::commit_table() {
  if (current_record_offset() > 0) {
    const int free_table_space = 
      current_record_offset() - CallInfoRecord::table_header_size;
    GUARANTEE(free_table_space >= 0, "Sanity");
    if (free_table_space >= sizeof(short)) {
      SETUP_ERROR_CHECKER_ARG;
      adjust_table_size(-free_table_space JVM_MUST_SUCCEED);
    }
  }
}

void CallInfoRecord::init(const CompiledMethod * const compiled_method,
                          const address pc) {
  _compiled_method = compiled_method;

#if ENABLE_THUMB_COMPILER
  const int code_offset = DISTANCE(compiled_method->entry(), 
                                   (unsigned int)pc & ~1);
#else
  const int code_offset = DISTANCE(compiled_method->entry(), pc);
#endif // ENABLE_THUMB_COMPILER

  GUARANTEE((code_offset % Assembler::instruction_alignment) == 0,
            "Invalid instruction alignment");

  const int shifted_code_offset = 
    code_offset / Assembler::instruction_alignment;

  const int method_end_offset = compiled_method->end_offset();

  const jushort header = 
    compiled_method->ushort_field(method_end_offset - sizeof(jushort));

  const jushort type = bitfield(header, 
                                CallInfoRecord::type_start, 
                                CallInfoRecord::type_width);

  GUARANTEE(type == Relocation::callinfo_type, "Callinfo table not found")

  if (type == Relocation::callinfo_type) {
    const jushort size = CallInfoRecord::table_size(header);

    const size_t aligned_table_size = align_size_up(size, sizeof(jushort));
    const int callinfo_start_offset = method_end_offset - aligned_table_size;
#ifdef AZZERT
    const int callinfo_end_offset = callinfo_start_offset + 
      size - CallInfoRecord::table_header_size;
#endif

    _current_record_offset = callinfo_start_offset;
#ifdef AZZERT
    _current_record_stackmap_offset = 0;
#endif

    read_record();
    
    int callinfo_code_offset = record_code_offset();

    while (callinfo_code_offset < shifted_code_offset) {
      next_record();
      read_record();
      callinfo_code_offset += record_code_offset();
    }

    GUARANTEE(_current_record_offset < callinfo_end_offset, "Sanity");
    GUARANTEE(callinfo_code_offset == shifted_code_offset, "Sanity");
  }
}

int CallInfoRecord::read_encoded_value(int& value, int offset) {
  CompiledMethod::Raw cm = compiled_method()->obj();
  int read_bytes_count = 0;
  int shifter = 0;
  jubyte b = 0;
  value = 0;

  do {
    b = cm().ubyte_field(offset); offset += sizeof(jubyte);
    value |= (b & 0x7f) << shifter;
    shifter += 7;
    read_bytes_count++;
  } while ((b & CallInfoRecord::value_end_bit) == 0);   
  
  return read_bytes_count;
}

int CallInfoRecord::compute_stackmap_size(int offset) {
  CompiledMethod::Raw cm = compiled_method()->obj();
  int read_bytes_count = 0;
  jubyte b = 0;

  do {
    b = cm().ubyte_field(offset); offset += sizeof(jubyte);
    read_bytes_count++;
  } while ((b & CallInfoRecord::value_end_bit) == 0);   
  
  return read_bytes_count;
}

inline int CallInfoRecord::record_code_offset() const {
  return _current_record_code_offset;
}

inline int CallInfoRecord::stackmap_offset() const {
  return _current_record_stackmap_offset;
}

inline int CallInfoRecord::stackmap_size() const {
  return _current_record_stackmap_size;
}

inline int CallInfoRecord::record_size() const {
  return _current_record_size;
}

inline void CallInfoRecord::read_record() {
  int offset = record_offset();

  int encoded_code_offset = 0;

  offset += read_encoded_value(encoded_code_offset, offset);
  offset += read_encoded_value(_current_record_bci, offset);

  _current_record_size = offset - record_offset();

  bool same_stackmap = encoded_code_offset & same_stackmap_bit;

  _current_record_code_offset = encoded_code_offset >> 1;

  if (!same_stackmap) { 
    _current_record_stackmap_offset = offset;
    const int stackmap_size = compute_stackmap_size(offset);
    _current_record_stackmap_size = stackmap_size;
    _current_record_size += stackmap_size;
  } else {
    GUARANTEE(stackmap_size() > 0, "Cannot be zero");
  }
}

inline void CallInfoRecord::next_record() {
  _current_record_offset += record_size();
}

inline void CallInfoRecord::index_to_offsets(int index, 
                                             int& byte_offset, 
                                             int& bit_offset) {
  byte_offset = index / 7;
  bit_offset  = index % 7;
}

bool CallInfoRecord::oop_tag_at(int index) const {
  int byte_offset, bit_offset;
  CallInfoRecord::index_to_offsets(index, byte_offset, bit_offset);
  if (byte_offset >= stackmap_size()) {
    return false;
  }
  const int stackmap_byte_offset = stackmap_offset() + byte_offset;
  const jubyte data = compiled_method()->ubyte_field(stackmap_byte_offset);
  return ((data >> bit_offset) & 0x1) ? true : false;
}

#endif // ENABLE_COMPILER && ENABLE_APPENDED_CALLINFO
