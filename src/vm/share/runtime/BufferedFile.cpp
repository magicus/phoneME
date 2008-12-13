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
# include "incls/_BufferedFile.cpp.incl"

/*
 * This class provides a buffered file input stream, primarily used
 * when reading resources.  An instance of this class is allocated on
 * the VM heap and is referenced via a file descriptor that is
 * returned from FileObject::open_file().  This fd is passed back to
 * the Java level code and is used for subsequent reads.  The mapping
 * from fd to BufferedFile object is via an ObjArray that is kept in a
 * persistent handle in Universe class.
 */

void BufferedFile::init()
{
  set_at_eof(false);
  set_count(0);
  set_index(0);
  set_file_pos(0);
}

void BufferedFile::set_file_pointer(OsFile_Handle val) {
  int_field_put(file_pointer_offset(), (int)val);
}

#if NOT_CURRENTLY_USED
void BufferedFile::init(Buffer *data, int buffer_size,
                        int file_size, int buffer_count)
{
  // initialize the buffer
  init();
  set_file_pos(buffer_count);
  set_count(buffer_count);
  set_data_buffer(data);
  set_buffer_size(buffer_size);
  set_file_size(file_size);
}
#endif

void BufferedFile::refill_buffer() {
  if (at_eof()) {
    return;
  }
  int bytes_to_read = buffer_size();
  int max = file_size() - file_pos();
  if (bytes_to_read > max) {
    bytes_to_read = max;
  }

  int count_read;
  {
    // The file_pointer() may be shared with a FileDecoder
    // object. This may cause BufferedFile::file_pos() to become out
    // of date. A call to OsFile_seek() makes everything consistent again.
    AllocationDisabler raw_pointers_used_in_this_block;
    Buffer::Raw fb = data_buffer();
    OsFile_seek(file_pointer(), file_pos(), SEEK_SET);
    count_read = OsFile_read(file_pointer(), fb().base_address(),
                             1, bytes_to_read);
  }

  set_count(count_read);
  set_file_pos(long(file_pos() + count_read));
  set_index(0);
  if (count_read <= 0) {
     set_at_eof(true);
  }
  return;
}

size_t BufferedFile::get_bytes(address buffer, jint num, bool is_buffered) {
  jint total_read = 0;
  Buffer::Raw fb = data_buffer();

  if (!is_buffered) {
    OsFile_seek(file_pointer(), (long)(-(count() - index())), SEEK_CUR);
    set_count(0);    // flush buffer
    set_index(0);
    return OsFile_read(file_pointer(), buffer, 1, num);

  } else {
    while (!at_eof() && num > 0) {
      int num_to_read = (num > (count() - index())) ? (count() - index()) :num;
      // num_to_read may be zero
      if (num_to_read) {
        jvm_memcpy(buffer, fb().base_address() + (index() * sizeof (jbyte)), 
                   num_to_read);
        buffer += num_to_read;
        num -= num_to_read;
        set_index(index() + num_to_read);
        total_read += num_to_read;
      }
      if (num > 0) {
        refill_buffer();
      }
    }
  }
  return total_read;
}

int BufferedFile::seek(jint offset, int origin) {
  int ndx = index();
  int cnt = count();
  switch (origin) {
  case SEEK_CUR:
    // we may have data in the buffer so seek relative to current index into
    // buffer
    if (cnt > 0) {
      // we have some data, so seek from current index
      if (offset + ndx >= 0 && offset + ndx < cnt) {
        // we are seeking somewhere in the buffer, just set the ndx and return
        set_index((int) (ndx + offset));
        return 0;
      } else {
        // we are seeking out of buffer, do a SEEK_CUR to the right position
        offset -= (cnt - ndx);
        set_file_pos((long)(file_pos() + offset));
      }
    }
    break;
  case SEEK_SET:
    if (cnt > 0) {
      if (offset >= (file_pos() - cnt) && offset < file_pos()) {
        // we're seeking in the buffer so just adjust pointer
        set_index((int) (cnt - (file_pos() - offset)));
        return 0;
      }
    }
    set_file_pos(offset);
    break;
  case SEEK_END:
    if (cnt > 0) {
      if (cnt == file_size()) {
        // The whole thing is in the buffer so just adjust the index
        if ((cnt + offset ) >= 0) {
          set_index((int) (cnt + offset));
          return 0;
        }
      }
      // There's more data beyond what we have in the buffer so just seek there
    }
    set_file_pos(long(file_size() + offset));
    break;
  }
  set_count(0);    // flush buffer
  set_index(0);

  if (file_pos() < file_size()) {
    set_at_eof(false);
  }
  return (int) OsFile_seek(file_pointer(), offset, origin);
}

bool BufferedFile::open(const PathChar *name, const char *mode)
{
  set_file_pointer(OsFile_open(name, mode));
  OsFile_Handle fh = file_pointer();
  set_file_size(fh == NULL ? 0 : OsFile_length(fh));
  return (fh != NULL);
}

int BufferedFile::close()
{
  if (file_pointer() != NULL) { 
    int result = OsFile_close(file_pointer());
    set_file_pointer(0);
    set_file_size(0);
    set_file_pos(0);
    set_index(0);
    set_count(0);
    return result;
  } else {
    return -1;
  }
}

size_t BufferedFile::length()
{
  return (size_t) OsFile_length(file_pointer());
}


ReturnOop BufferedFile::allocate(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  Buffer::Fast data_buffer =
    Universe::new_byte_array_raw(BUFFERSIZE JVM_CHECK_0);
  BufferedFile::Fast bf = Universe::new_mixed_oop(
                                 MixedOopDesc::Type_BufferedFile,
                                 BufferedFileDesc::allocation_size(),
                                 BufferedFileDesc::pointer_count()
                                 JVM_CHECK_0);
  bf().set_data_buffer(&data_buffer);
  bf().set_buffer_size(BUFFERSIZE);
  return bf.obj();
}

#if NOT_CURRENTLY_USED
ReturnOop BufferedFile::allocate(JvmAllocProc alloc_proc JVM_TRAPS) {
  if (alloc_proc != NULL) {
    OopDesc* oop_desc =
      (OopDesc*)alloc_proc(sizeof(BufferedFileDesc));
    if (oop_desc == NULL) {
      Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
      return NULL;
    }

    // Must zero-inititialize the allocated block to match 
    // ObjectHeap::allocate behavior.
    jvm_memset((char*)oop_desc, 0, sizeof(BufferedFileDesc));
      
    return oop_desc;
  } else {
    return BufferedFile::allocate(JVM_SINGLE_ARG_CHECK_0);
  }
}
#endif
