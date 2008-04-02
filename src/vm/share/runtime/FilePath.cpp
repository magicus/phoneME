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
#include "incls/_FilePath.cpp.incl"

const PathChar FilePath::classfile_suffix[] = {
  '.','c','l','a','s','s', 0 // 0-terminated
};
#if ENABLE_ROM_GENERATOR
const PathChar FilePath::default_source_rom_file[] = {
  'R','O','M','I','m','a','g','e','.','c','p','p', 0 // 0-terminated
};
const PathChar FilePath::default_binary_rom_file[] = {
  'R','O','M','_','b','i','n','a','r','y','.','b','u','n', 0 // 0-terminated
};

const JvmPathChar FilePath::rom_declare_file[] = {
  'R','O','M','_','d','e','c','l','a','r','e','.','t','m','p', 0 // 0-terminated
};
const JvmPathChar FilePath::rom_dump_file[] = {
  'R','O','M','_','d','u','m','p','.','t','x','t', 0 // 0-terminated
};
const JvmPathChar FilePath::rom_log_file[] = {
  'R','O','M','L','o','g','.','t','x','t', 0 // 0-terminated
};
const JvmPathChar FilePath::rom_main_file[] = {
  'R','O','M','_','m','a','i','n','.','t','m','p', 0 // 0-terminated
};
const JvmPathChar FilePath::rom_optimizer_file[] = {
  'R','O','M','_','o','p','t','i','m','i','z','e','r','.','t','m','p', 0
  // 0-terminated
};
const JvmPathChar FilePath::rom_summary_file[] = {
  'R','O','M','_','s','u','m','m','a','r','y','.','t','m','p', 0 // 0-terminated
};
const JvmPathChar FilePath::rom_reloc_file[] = {
  'R','O','M','_','r','e','l','o','c','.','t','m','p', 0 // 0-terminated
};
const JvmPathChar FilePath::rom_kvm_natives_file[] = {
  'K','v','m','N','a','t','i','v','e','s','.','c','p','p', 0 // 0-terminated
};

#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
JvmPathChar FilePath::rom_image_generated_file[] = {
  'R','O','M','I','m','a','g','e','G','e','n','e','r','a','t','e','d','.','h','p','p', 0 
    // 0-terminated
};
JvmPathChar FilePath::rom_tmp_segment_file[] = {
  'R','O','M','_','m','a','i','n','_','?','?','.','t','m','p', 0 // 0-terminated
};
JvmPathChar FilePath::rom_stuff_file[] = {
  'R','O','M','_','s','t','u','f','f','.','t','m','p', 0 // 0-terminated
};
JvmPathChar FilePath::rom_segment_file[] = {
  'R','O','M','I','m','a','g','e','_','?','?','.','c','p','p', 0 // 0-terminated
};
#endif // ENABLE_SEGMENTED_ROM_TEXT_BLOCK
#endif

HANDLE_CHECK(FilePath, is_char_array())

extern "C" size_t fn_strlen(const PathChar* str) {
  const PathChar *p;
  size_t len = 0;
  for (p=str; *p; p++) {
    ++len;
  }
  return len;
}

extern "C" void fn_strcat(PathChar* s1, const PathChar *s2) {
  s1 += fn_strlen(s1);
  while (*s2) {
    *s1++ = *s2++;
  }
  *s1 = 0;
}

ReturnOop FilePath::convert_to_unicode(const JvmPathChar* name, int length 
                                       JVM_TRAPS) {
  FilePath::Raw fp = Universe::new_char_array(length JVM_CHECK_0);

  for (int i=0; i<length; i++) {
    fp().char_at_put(i, (jchar)name[i]);
  }

  return fp.obj();
}

void FilePath::string_copy(PathChar* dst, int buf_length) {
  int i;
  // be on safer side - keep everything including final '\0' in buf_length
  buf_length--;
  
  for (i=0; i < buf_length && i<this->length(); i++) {
    dst[i] = (PathChar)char_at(i);
  }
  dst[i] = 0;
}

ReturnOop FilePath::from_string(String* string JVM_TRAPS) {
  int length = string->length();
  FilePath::Raw fp = Universe::new_char_array(length JVM_CHECK_0);
  
  for (int i=0; i<length; i++) {
    fp().char_at_put(i, string->char_at(i));
  }

  return fp.obj();

}
