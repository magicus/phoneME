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

#if USE_SOURCE_IMAGE_GENERATOR
class ROMStructsWriter {
public:
  void write(ROMWriter *rom_writer JVM_TRAPS);

private:
  void write_primitive_array_rom_struct(Stream *stream, const char *type, 
                                        const char *field_type=NULL);  
  void write_rom_struct(Stream *stream, InstanceClass *klass JVM_TRAPS);
  void write_rom_static_struct(Stream *stream, InstanceClass *klass JVM_TRAPS);
  void write_rom_struct_fields(Stream *stream, InstanceClass *klass,
                               InstanceClass *target_class JVM_TRAPS);
  void write_rom_struct_static_fields(Stream *stream, InstanceClass *klass
                                      JVM_TRAPS);
  int  write_rom_struct_type(Stream *stream, Field *field JVM_TRAPS);
  int  count_field_name_occurences(InstanceClass *klass,
                                   InstanceClass *target_class,
                                   Symbol *field_name);
  bool is_reserved_word(Symbol *field_name);
  static const char *reserved_words[];
};
#endif
