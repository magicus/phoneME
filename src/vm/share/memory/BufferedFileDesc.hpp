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

class BufferedFileDesc: public MixedOopDesc {
 private:
  // Computes the allocation size
  static size_t allocation_size() {
     return align_allocation_size(sizeof(BufferedFileDesc));;
  }
  static jint pointer_count()     { return 1; }

 protected:
  static size_t header_size() { return allocation_size(); }

  OopDesc * _data_buffer;

  /* All oops must go before here.  If you change the number of oops, be
   * sure to change pointer_count()
   */

  OsFile_Handle  _file_pointer;
  jboolean       _at_eof;
  jboolean       _on_native_heap;
  jboolean       __pad1;  // force 4-byte structure packing
  jboolean       __pad2;  // force 4-byte structure packing
  jint           _buffer_size;
  jint           _file_size;
  jint           _file_pos;
  jint           _index;
  jint           _count;

 private:
  friend class OopDesc;
  friend class BufferedFile;
  friend class Universe;
};
