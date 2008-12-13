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

/*
 * OS.cpp: Operating system porting interface
 *
 * This file implements the portable parts of
 * the operating system porting interface (class Os).
 * The operations defined in this file are
 * intended to be applicable to a large number
 * of OS ports.
 *
 * For more information on the OS porting interface,
 * refer to the Porting Guide.
 */

#include "incls/_precompiled.incl"
#include "incls/_OS.cpp.incl"

#if USE_DEFAULT_TTY_STREAM
// You may define USE_DEFAULT_TTY_STREAM=0 in Globals_<os_family>.hpp
// to use an alternative implementation of Os::get_tty.
//
// This may be necessary if you're running on a barebone environment
// that has trouble running static C++ constructors -- there is a
// single static C++ object in the system, default_stream, which must
// be initialized at program start-up by a static C++ constructor.

// This is default implementation of DefaultStream.
class DefaultStream : public Stream {
 public:

  OsFile_Handle _log_file;
  bool has_log_file();

  virtual void flush() {
    OsFile_flush(_log_file);
  }

  DefaultStream() : Stream() {
    NOT_PRODUCT(_log_file = NULL);
  }

#ifndef PRODUCT
  ~DefaultStream() {
    if (has_log_file()) {
      OsFile_close(_log_file);
    }
  }
#endif

  static void* singleton[16];

  void* operator new(size_t /*size*/) {
#ifndef PRODUCT
    GUARANTEE(sizeof(singleton) >= sizeof(DefaultStream), "sanity");
    jvm_memset(&singleton, 0, sizeof(singleton));
#endif
    return (void*)(&singleton);
  }

  virtual void print_raw(const char* s);
};

static JvmPathChar __log_name[]  = {'c','l','d','c','_','a','.','l','o','g',0};

bool DefaultStream::has_log_file() {
  // Lazily create log file (at startup, LogVMOutput is false even
  // if +LogVMOutput is used, because the flags haven't been parsed yet
  if (LogVMOutput && _log_file == NULL) {
    _log_file = OsFile_open(__log_name, "w");
    __log_name[5]++;
  }
  return _log_file != NULL;
}

static unsigned int __charcount;

void DefaultStream::print_raw(const char* s) {
  JVMSPI_PrintRaw(s);

  // print to log file
  if (LogVMOutput && has_log_file()) {
    OsFile_write(_log_file, s, sizeof(char), jvm_strlen(s));
    //    OsFile_flush(_log_file);
    if (++__charcount == 200000000) {
      OsFile_flush(_log_file);
      JvmPathChar z = 'z';
      JvmPathChar a = 'a';
      __charcount = 0;
      OsFile_close(_log_file);
      _log_file = OsFile_open(__log_name, "w");
      if (__log_name[5] == z) {
        __log_name[5] = a;
      } else {
        __log_name[5]++;
      }
    }
  }


#if !defined(PRODUCT) || ENABLE_PROFILER || ENABLE_TTY_TRACE
  while (true) {
    char ch = *s++;
    if (ch == 0) {
      break;
    } else if (ch == '\n') {
      _position = 0;
    } else {
      _position += 1;
    }
  }
#endif
}

void* DefaultStream::singleton[16];

Stream *Os::get_tty() {
  if (tty == NULL) {
    tty = new DefaultStream();
  }
  return tty;
}

#endif // USE_DEFAULT_TTY_STREAM
