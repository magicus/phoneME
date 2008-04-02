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
#include "incls/_ClassPathAccess.cpp.incl"

inline ReturnOop ClassPathAccess::open_jar_entry(JarFileParser* parser,
                 Symbol* entry_symbol, const bool is_class_file JVM_TRAPS)
{
  // 7 extra chars for ".class" + 0
  DECLARE_STATIC_BUFFER(char, entry_name, NAME_BUFFER_SIZE + 7);
  if (entry_symbol->length() >= NAME_BUFFER_SIZE) {
    // Sorry, name too long
    return NULL;
  }
  entry_symbol->string_copy(entry_name, NAME_BUFFER_SIZE);
  if (is_class_file) {
    jvm_strcat(entry_name, ".class");
  }

  const bool found = parser->find_entry(entry_name JVM_MUST_SUCCEED);
  if (found) {
    int flags = 0;
    // set MUST_CLOSE_FILE and INCREMENTAL_INFLATE bits when needed
    if (!is_class_file && !(USE_SOURCE_IMAGE_GENERATOR && GenerateROMImage)) {
      flags = INCREMENTAL_INFLATE;
    }
    return parser->open_entry(flags JVM_NO_CHECK_AT_BOTTOM);
  }
  return NULL;
}

inline ReturnOop ClassPathAccess::open_local_file(PathChar* path_name,
            Symbol* entry_symbol, const bool is_class_file JVM_TRAPS)
{
  const char separator_char = OsFile_separator_char;
  int pos = fn_strlen(path_name);

  path_name[pos++] = (PathChar)separator_char;
  for (int i=0; i<entry_symbol->length(); i++) {
      path_name[pos++] = (PathChar)entry_symbol->byte_at(i);
  }
  path_name[pos] = 0;
  if (is_class_file) {
    fn_strcat(path_name, FilePath::classfile_suffix);
  }

  if (separator_char != '/') {
    for(PathChar* tf = path_name; *tf; tf++) {
      if (*tf == (PathChar)'/') {
        *tf = (PathChar)separator_char;
      }
    }
  }

  OsFile_Handle handle = OsFile_open(path_name, "rb");
  if (handle == NULL) {
    return NULL;
  }

  int size = OsFile_length(handle);
  return FileDecoder::allocate(handle, 0, size, MUST_CLOSE_FILE
                               JVM_NO_CHECK_AT_BOTTOM);
}

ReturnOop ClassPathAccess::open_entry(Symbol* entry_name,
                     const bool is_class_file, OopDesc* classpath JVM_TRAPS)
{
  if( !classpath ) {
    return NULL;
  }

  OopDesc* p = NULL;

  UsingFastOops fast_oops;
  JarFileParser::Fast parser;
  ObjArray::Fast cp( classpath );
  const int cp_length = cp().length();

  for( int index = 0; index < cp_length; index++) {
    FilePath::Raw path( cp().obj_at(index) );    
#if USE_BINARY_IMAGE_LOADER
    if( path.is_null() ) {
      // This path element points to a Monet bundle file, and have been 
      // loaded into the VM at start-up.
      continue;
    }
#endif
    // 7 extra chars for ".class" + 0
    DECLARE_STATIC_BUFFER(PathChar, path_name, NAME_BUFFER_SIZE + 7);
    const int path_length = path().length();
    if( path_length >= NAME_BUFFER_SIZE ) {
      continue; // Sorry, name too long
    }
    path().string_copy( path_name, NAME_BUFFER_SIZE );

    parser = JarFileParser::get(path_name, true JVM_NO_CHECK);
    if( parser.not_null() ) {
      p = open_jar_entry(&parser, entry_name, is_class_file JVM_CHECK_0);
      parser.set_null();
    } else if( (path_length + 1 + entry_name->length()) >= NAME_BUFFER_SIZE ){
      continue; // Sorry, name too long
    } else {
      p = open_local_file(path_name, entry_name, is_class_file JVM_CHECK_0);
    }
    if( p ) {
      break;
    }
  }
  return p;
}

ReturnOop ClassPathAccess::open_entry(Symbol* entry_symbol,
                                      const bool is_class_file JVM_TRAPS)
{
#if !ENABLE_LIB_IMAGES
  if (ROM::binary_image_currently_enabled() && is_class_file) {
    // Many Binary ROM optimizations (e.g., force virtual methods to
    // be final) require that no class loading happens after a binary
    // image has been loaded. See the VM Porting Guide.
    return NULL;
  }
#endif

#if ENABLE_ROM_GENERATOR
  if (GenerateROMImage && Task::current()->app_classpath() == NULL) {
    // ROMOptimization is executing. At this point,
    // Task::current()->classpath() has been set to NULL and all classes in
    // classes.zip have been loaded. If we're still looking for a
    // class, it means a required class is missing from the
    // classes.zip.
    tty->print("Required class is missing: ");
    entry_symbol->print_symbol_on(tty, true);
    tty->cr();
#ifndef PRODUCT
    ps();
#endif
    Throw::class_not_found(entry_symbol, ExceptionOnFailure JVM_THROW_0);
  }
#endif

  OopDesc* p = open_entry( entry_symbol, is_class_file,
                           Task::current()->sys_classpath() JVM_CHECK_0 );
  if( p ) {
    FileDecoder::Raw decoder(p);
    decoder().add_flags(SYSTEM_CLASSPATH);
    return p;
  }
  return open_entry( entry_symbol, is_class_file,
                           Task::current()->app_classpath() JVM_NO_CHECK );
}
