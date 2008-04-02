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

class FileDecoderDesc : public MixedOopDesc {
private:
  // The following buffers are not used directly by FileDecoder
  // but they are needed by the child class Inflater.
  // According to MixedOopDesc layout pointers should come first
  TypeArray*  _jar_file_name;
  Buffer*     _in_buffer;
  Buffer*     _out_buffer;
  Buffer*     _length_buffer;
  Buffer*     _distance_buffer;

  OsFile_Handle  _file_handle;
  int            _file_pos;
  int            _file_size;
  int            _bytes_remain;
  int            _flags;

  static int allocation_size() {
    return sizeof(FileDecoderDesc);
  }

  static int pointer_count() {
    return 5;
  }

  friend class FileDecoder;
  friend class Inflater;
};
