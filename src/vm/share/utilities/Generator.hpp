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

#ifndef PRODUCT

/**
 * Assembly code generator 
 *
 * This is the general assembly code generator. It generates
 * assembly code, such as the byte code interpreter and stubs, 
 * that can be compiled and linked into the runtime system.
 *
 * Refer to "Generator.cpp" or the Porting Guide for documentation.
*/ 
class Generator {
 public:
  // Generate the assembler code that will be linked into the runtime system.
  static void generate();

  static void generate_oopmap(Stream* stream,
                              void do_class(oopmaps_doer do_map, void*param),
                              const char* name);

  static void generate_oopmaps();

  // Generate a C function, to be used by non-product builds, to check
  // that loopgen/romgen has the same C structure layout as the target
  // VM.
  static void generate_oopmap_checks(char *c_function_prefix, Stream* stream);
 private:
  static BasicType _prev_type;
  static size_t    _prev_offset;
  static size_t    _prev_index;
  static size_t    _offsets[256];
  static Stream*   _stream;

  static void save_offset(BasicType type, void *param, const char *name,
                          size_t offset, int flags);
  static void dump_entry(BasicType type, void *param, const char *name,
                         size_t offset, int flags);
  static void generate_empty_oopmap(oopmaps_doer do_map, void*param);
};

#endif
