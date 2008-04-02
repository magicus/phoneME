/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

# include "incls/_precompiled.incl"
# include "incls/_Stream.cpp.incl"

Stream* tty;

// BUFLEN -- max size of output of individual print() methods
#define BUFLEN 2048 

// BUFLEN may be overridden by MAX_PRINTF_BUFFER_SIZE in
// Globals_<os_family>.hpp
#if (BUFLEN > MAX_PRINTF_BUFFER_SIZE)
#undef BUFLEN
#define BUFLEN MAX_PRINTF_BUFFER_SIZE
#endif

void Stream::print(const char* format, ...) {
  JVM_VSNPRINTF_TO_BUFFER(format, buffer, BUFLEN);
  print_raw(buffer);
}

void Stream::print_cr(const char* format, ...) {
  JVM_VSNPRINTF_TO_BUFFER(format, buffer, BUFLEN);
  print_raw(buffer);
  cr();
}

#if !defined(PRODUCT) || ENABLE_TTY_TRACE || ENABLE_PERFORMANCE_COUNTERS \
    || ENABLE_WTK_PROFILER
void Stream::put(char ch) {
  GUARANTEE(ch != 0, "please fix call site");
  char buf[] = { ch, '\0' };
  print_raw(buf);
}
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
void Stream::vprint(const char *format, va_list argptr) {
  DECLARE_STATIC_BUFFER(char, buffer, BUFLEN);
  if (jvm_vsnprintf(buffer, BUFLEN, format, argptr) < 0) {
    NOT_PRODUCT(warning("increase BUFLEN in Stream.cpp -- output truncated"));
    buffer[BUFLEN - 1] = 0;
  }
  print_raw(buffer);
}

void Stream::vprint_cr(const char* format, va_list argptr) {
  vprint(format, argptr);
  cr();
}

void Stream::print_hex8(int n) {
  juint mask = 0xf0000000;
  print("0x");
  while (mask != 0) {
    if ((mask & ((juint)n)) == 0) {
      print("0");
      mask >>= 4;
    } else {
      break;
    }
  }
  if (mask != 0) {
    print("%x", n);
  }
}
void DupStream::print_raw(const char *c){
  stream1()->print_raw(c);
  stream2()->print_raw(c);
  _position = stream1()->position();
}

void DupStream::flush() {
  stream1()->flush();
  stream2()->flush();
}

void Stream::indent() {
  while (_position < _indentation) sp();
}
#endif // !PRODUCT

#if ENABLE_PERFORMANCE_COUNTERS || USE_DEBUG_PRINTING || ENABLE_JVMPI_PROFILE
void ByteArrayOutputStream::print_raw(const char *c) {
  int length = jvm_strlen(c);
  for (const char *p=c; *p; p++) {
    if (*p == '\n') {
      _position = 0;
    } else {
      _position ++;
    }
  }
  if (_array == NULL) {
    _array_size = length + 1000;
    _array = (char *)jvm_malloc(_array_size);
    GUARANTEE( _array ,  "failed to alloc initial buffer" ) ;
    if (_array == NULL) return;
  }
  if (length + _current_size + 1 > _array_size) {
    // Expand array acapacity if necessary
    int new_array_size = length + _current_size + 1000;
    char *new_array = (char *)jvm_malloc(new_array_size);
    GUARANTEE( new_array,  "failed to expand buffer" ) ;
    if (new_array == NULL) return;
    jvm_memcpy(new_array, _array, _array_size);
    jvm_free(_array);
    _array = new_array;
    _array_size = new_array_size;
  }
  jvm_memcpy(_array + _current_size, c, length);
  _current_size += length;
  _array[_current_size] = '\0';
}
#endif

void Stream::initialize() {
  GUARANTEE(tty == NULL, "sanity check");
  tty = Os::get_tty();
}


Stream::Stream(int width) {
  _width       = width;
  _position    = 0;
  _indentation = 0;
}

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_MEMORY_PROFILER \
    || ENABLE_WTK_PROFILER || ENABLE_PERFORMANCE_COUNTERS || ENABLE_PROFILER \
    || ENABLE_TTY_TRACE || USE_EVENT_LOGGER

FileStream::FileStream(const PathChar* file_name, int width) : Stream(width) {
  _file = OsFile_open(file_name, "w");
}

void FileStream::save(FileStreamState *state) {
  state->_file        = _file;
  state->_width       = _width;
  state->_position    = _position;
  state->_indentation = _indentation;

  // Don't close this file in ~FileStream destructor.
  _saved = true;
}

void FileStream::restore(FileStreamState *state) {
  _file        = state->_file;
  _width       = state->_width;
  _position    = state->_position;
  _indentation = state ->_indentation;
}

void FileStream::open(const PathChar* file_name) {
  if (_file != NULL) {
    OsFile_close(_file);
  }
  _file = OsFile_open(file_name, "w");
}

void FileStream::print_raw(const char* s) {
  if (_file == NULL) {
      return;
  }
  OsFile_write(_file, s, sizeof(char), jvm_strlen(s));
  for (;;) {
    char ch = *s++;
    if (ch == 0) {
      break;
    } else if (ch == '\n') {
      _position = 0;
    } else if (ch == '\t') {
      GUARANTEE(tab_size == 8, "correct this code");
      _position = ((_position >> 3) + 1) << 3;
    } else {
      _position += 1;
    }
  }
#if USE_BINARY_IMAGE_GENERATOR
  /* Makes debugging easier */
  flush();
#endif
}

void FileStream::flush() {
  if (_file != NULL) {
    OsFile_flush(_file);
  }
}

void FileStream::close() {
  if (_file != NULL) {
    OsFile_close(_file);
  }
  _file = NULL;
}

void FileStream::mark() {
  _mark = current_position();
}

void FileStream::reset() {
  OsFile_seek(_file, _mark, 0);  
}

int FileStream::current_position() {
  int position = OsFile_length(_file);
  // Since OsFile_length resets the pointer back to the beginning
  // We need to restore the pointer back the last position
  // before the call.
  OsFile_seek(_file, position, 0);  
  return position;
}

#endif // !defined(PRODUCT) || ENABLE_ROM_GENERATOR


#if USE_BINARY_IMAGE_GENERATOR

static char _bfs_buff[BFS_BUFFER_SIZE + 16];

#ifndef PRODUCT
static int _bfs_open_count = 0;
#endif

/*
 * We implement buffering explicitly, since many low-end platforms
 * do not supporting buffered writes.
 */

void BufferedFileStream::do_flush() {
  if (_file && !_has_error) {
    char *buf = _bfs_buff;
    int remain = _bfs_pos;
    int n;

    while (remain > 0) {
      n = OsFile_write(_file, buf, 1, (size_t)remain);
      if (n <= 0) {
        _has_error = true;
        break;
      }
      buf += n;
      remain -= n;
    }
    _bfs_pos = 0;
  }
}

void BufferedFileStream::open(const PathChar* file_name) {
  _file = OsFile_open(file_name, "wb");
  _counter = 0;
  _has_error = false;
  _bfs_pos = 0;
  if (_file) {
#ifndef PRODUCT
    _bfs_open_count ++;
    GUARANTEE(_bfs_open_count <= 1, "only one BufferedFileStream allowed");
#endif
  }
}

void BufferedFileStream::print_raw(const char* /*s*/) {
  UNIMPLEMENTED();
}

void BufferedFileStream::print_char(const char c) {
  flush_buffer(0);
  _bfs_buff[_bfs_pos++] = c;
  _counter += 1;
#ifndef PRODUCT
  flush_buffer(1); // To make debugging easier
#endif
}

void BufferedFileStream::print_int(const juint i) {
  /* Depending on the endianness of the target platform, */
  /* byte swapping may be necessary or unnecessary */
  // juint new_i = Bytes::swap_u4(i);
  flush_buffer(0);
  char *p = &_bfs_buff[_bfs_pos];
#if HARDWARE_LITTLE_ENDIAN
  *p++ = (char)((i & 0x000000ff) >>  0);
  *p++ = (char)((i & 0x0000ff00) >>  8);
  *p++ = (char)((i & 0x00ff0000) >> 16);
  *p   = (char)((i & 0xff000000) >> 24);
#else
  *p++ = (char)((i & 0xff000000) >> 24);
  *p++ = (char)((i & 0x00ff0000) >> 16);
  *p++ = (char)((i & 0x0000ff00) >>  8);
  *p   = (char)((i & 0x000000ff) >>  0);
#endif
  _bfs_pos += 4;
  _counter += 4;
#ifndef PRODUCT
  flush_buffer(1); // To make debugging easier
#endif
}

void BufferedFileStream::close() {
  if (_file) {
    flush_buffer(1);
    OsFile_close(_file);
    _file = NULL;
#ifndef PRODUCT
    _bfs_open_count --;
#endif
  }
}

juint BufferedFileStream::get_counter() {
  return _counter;
}

void BufferedFileStream::save(BufferedFileStreamState *state) {
  state->_file        = _file;
  state->_width       = _width;
  state->_position    = _position;
  state->_indentation = _indentation;
  state->_counter     = _counter;
  state->_has_error   = _has_error;
  state->_bfs_pos     = _bfs_pos;
}

void BufferedFileStream::restore(BufferedFileStreamState *state) {
  _file        = state->_file;
  _width       = state->_width;
  _position    = state->_position;
  _indentation = state->_indentation;
  _counter     = state->_counter;
  _has_error   = state->_has_error;
  _bfs_pos     = state->_bfs_pos;
}

#endif /* USE_BINARY_IMAGE_GENERATOR */

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_DYNAMIC_NATIVE_METHODS || USE_DEBUG_PRINTING || ENABLE_MEMORY_PROFILER
void FixedArrayOutputStream::print_raw(const char *s) {
  int length = jvm_strlen(s);
  if (length + _current_size + 1 >= _limit) {
    length = _limit - _current_size - 1;
  }
  if (length > 0) {
    // This is only for debugging.  So we can ignore anything that doesn't fit
    jvm_memcpy(_array + _current_size, s, length);
    _current_size += length;
    _array[_current_size] = '\0';
  }
  for (;;) {
    char ch = *s++;
    if (ch == 0) {
      break;
    } else if (ch == '\n') {
      _position = 0;
    } else if (ch == '\t') {
      GUARANTEE(tab_size == 8, "correct this code");
      _position = ((_position >> 3) + 1) << 3;
    } else {
      _position += 1;
    }
  }
}
#endif // !defined(PRODUCT) || ENABLE_ROM_GENERATOR
