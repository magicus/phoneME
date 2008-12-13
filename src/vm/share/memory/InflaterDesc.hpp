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

class InflaterDesc : public FileDecoderDesc {
private:
  juint  _out_offset;    // Points to the first free element in out_buffer
  juint  _out_dumped;    // Number of bytes in out_buffer already read

  juint  _in_offset;     // Points to the first free element in in_buffer
  juint  _in_data_size;  // Number of good bits in in_data
  juint  _in_data;       // Low in_data_size bits are from stream

  juint  _expected_crc;  // CRC32 obtained from the entry header
  juint  _block_type;    // FIXED_HUFFMAN, DYNAMIC_HUFFMAN or STORED

  static int allocation_size() {
    return sizeof(InflaterDesc);
  }

  friend class Inflater;
};
