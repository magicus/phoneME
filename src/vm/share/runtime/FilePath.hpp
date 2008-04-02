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

/** \class FilePath.hpp
    Deals with handling file pathnames.
    Pathnames may be in UNICODE or char, depending on compile-time flag.
 */

class FilePath : public TypeArray {
public:
  HANDLE_DEFINITION_CHECK(FilePath, TypeArray);

  static ReturnOop convert_to_unicode(const JvmPathChar* name, int length 
                                      JVM_TRAPS);

  static ReturnOop from_string(String* string JVM_TRAPS);

  void string_copy(JvmPathChar* dst, int buf_length);

  static const JvmPathChar classfile_suffix[];
#if ENABLE_ROM_GENERATOR
  static const JvmPathChar default_source_rom_file[];
  static const JvmPathChar default_binary_rom_file[];

  static const JvmPathChar rom_declare_file[];
  static const JvmPathChar rom_dump_file[];
  static const JvmPathChar rom_log_file[];
  static const JvmPathChar rom_main_file[];
  static const JvmPathChar rom_optimizer_file[];
  static const JvmPathChar rom_summary_file[];
  static const JvmPathChar rom_reloc_file[];
  static const JvmPathChar rom_kvm_natives_file[];  

#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  static JvmPathChar rom_image_generated_file[];
  static JvmPathChar rom_tmp_segment_file[];
  static JvmPathChar rom_stuff_file[];
  static JvmPathChar rom_segment_file[];
#endif // ENABLE_SEGMENTED_ROM_TEXT_BLOCK
#endif // ENABLE_ROM_GENERATOR
};
