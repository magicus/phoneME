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

#include "incls/_precompiled.incl"
#include "incls/_FileDecoder.cpp.incl"

HANDLE_CHECK(FileDecoder, is_file_decoder())

ReturnOop FileDecoder::allocate(OsFile_Handle handle, int pos, int size,
                                int flags JVM_TRAPS)
{
  FileDecoder::Raw fd = Universe::new_mixed_oop(
                          MixedOopDesc::Type_FileDecoder,
                          FileDecoderDesc::allocation_size(),
                          FileDecoderDesc::pointer_count()
                          JVM_CHECK_0);

  fd().set_file_handle(handle);
  fd().set_file_pos(pos);
  fd().set_file_size(size);
  fd().set_bytes_remain(size);
  fd().set_flags(flags);
  return fd;
}

ReturnOop FileDecoder::read_completely(JVM_SINGLE_ARG_TRAPS) {
  // try ...
  ReturnOop result = read_completely0(JVM_SINGLE_ARG_NO_CHECK);

  // finally ...
  if (flags() & MUST_CLOSE_FILE) {
    OsFile_close(file_handle());
  }

  // return
  return result;
}

ReturnOop FileDecoder::read_completely0(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JarFileParser::Fast jfp = get_jar_parser_if_needed(JVM_SINGLE_ARG_CHECK_0);
  (void)jfp;
  TypeArray::Fast result;
  if (is_inflater()) {
    result = ((Inflater*) this)->
             read_completely(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    int size = file_size();
    result = Universe::new_byte_array_raw(size JVM_CHECK_0);
    int actual_bytes = get_bytes_raw(result().base_address(), size);
    if (actual_bytes != size) {
      result = (ReturnOop) NULL;
    }
  }

  return result;
}

int FileDecoder::get_bytes(ArrayPointer* destination, int count JVM_TRAPS) {
  UsingFastOops fast_oops;
  JarFileParser::Fast jfp = get_jar_parser_if_needed(JVM_SINGLE_ARG_CHECK_0);
  (void)jfp;

  if (count > bytes_remain()) {
    count = bytes_remain();
  }
  int result = !is_inflater() ?
    get_bytes_raw(destination->base_address(), count) :
    ((Inflater*) this)->get_bytes(destination, count JVM_NO_CHECK_AT_BOTTOM);

  set_bytes_remain(bytes_remain() - result);
  return result;
}

int FileDecoder::get_bytes_raw(address dest_address, int count) {
  OsFile_Handle handle = file_handle();
  GUARANTEE(handle != NULL, "What are we reading from?");

  int pos = file_pos();
  OsFile_seek(handle, pos, SEEK_SET);
  int bytes_read = OsFile_read(handle, dest_address, 1, count);
  set_file_pos(pos + bytes_read);
  return bytes_read;
}

// This function must be called before the file_handle() function is called.
// It ensures the cached JarFileParser object is valid before reading
// from the JAR file. This is important for the caching of JarFileParser.
ReturnOop FileDecoder::get_jar_parser_if_needed(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  TypeArray::Fast jar_name = jar_file_name();
  if (jar_name.not_null()) {
    JarFileParser::Raw result = JarFileParser::get(&jar_name JVM_NO_CHECK);
    if (result.not_null()) {
      // The old JarFileParser that was used when allocating the
      // FileDecoder may be GC'ed already. We might get a new
      // JarFileParser here, thus we switch to its file handle.
      set_file_handle(result().handle());
    } else if (!CURRENT_HAS_PENDING_EXCEPTION) {
      // This happens when system may have too many open files. This is 
      // probably happening when a ResourceInputStream has been idle for
      // a while, all cached JarFileParser have been GC'ed, and
      // and some MIDP code opens too many files to make the system run 
      // out of file handles.
      //
      // This should never happen if some cached JarFileParser are still
      // valid -- JarFileParser::get() should force a GC to close
      // the files used by old cached JarFileParsers, and we should
      // then be able to open the JAR file associated with <this>,
      // hence the GUARANTEE here.
      GUARANTEE(JarFileParser::parser_cache_is_empty(), "sanity");

      // We can't proceed with reading when we don't have a file handle.
      Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
    }
    return result.obj();
  } else {
    return NULL;
  }
}

#ifndef PRODUCT
// If this FileDecoder is associated with a JAR file, make sure when
// we try to use the file handle, we always have an active
// JarFileParser. See JarFileParser.cpp header for the gory details.
void FileDecoder::guarantee_jar_file_handle() {
#ifdef AZZERT
  if (jar_file_name() != NULL) {
    int seen = false;
    ForAllHandles (oop) {
      if (oop->not_null() && oop->is_jar_file_parser()) {
        JarFileParser::Raw parser = oop->obj();
        TypeArray::Raw name1 = parser().pathname();
        TypeArray::Raw name2 = jar_file_name();
        int len1 = name1().length();
        int len2 = name2().length();
        void *base1 = name1().base_address();
        void *base2 = name2().base_address();

        GUARANTEE(len1 == len2 && jvm_memcmp(base1, base2, len1) == 0,
                  "Only one active JarFileParser at any time");
        seen = true;
      }
    }
    GUARANTEE(seen, "JarFileParser associated with this FileDecoder must be "
                    "active on C stack");
  }
#endif
}
#endif
