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

#if ENABLE_VERBOSE_ASSERTION
#define JVM_VSNPRINTF_WARN(file, line) \
    NOT_PRODUCT(warning("%s: %s buffer too small, output truncated", file, line));
#else
#define JVM_VSNPRINTF_WARN(file, line) 
#endif

#define JVM_VSNPRINTF_TO_BUFFER(format, buffer, len) \
  DECLARE_STATIC_BUFFER(char, buffer, len); \
  va_list ap; \
  va_start(ap, format); \
  if (jvm_vsnprintf(buffer, len, format, ap) < 0) { \
    JVM_VSNPRINTF_WARN(__FILE__, __LINE__); \
    buffer[len - 1] = 0; \
  } \
  va_end(ap)

/**
 * Output streams for printing
 */
class Stream: public GlobalObj {
 public:
  enum { tab_size = 8 };

  // Bootstrapping
  static void initialize();

  // creation
  Stream(int width = 80);

  // printing
  void print(const char* format, ...);
  void print_cr(const char* format, ...);
  void vprint(const char *format, va_list argptr);
  void vprint_cr(const char* format, va_list argptr);
  virtual void print_raw(const char* /*str*/) JVM_PURE_VIRTUAL;
  inline void sp() {
    this->print_raw(" ");
  }
  void cr() {
    this->print_raw("\n");
  }
  inline void fill_to(int col) {
    while (_position < col) {
      sp();
    }
  }

  // closing  
  virtual void close() {}
  virtual void mark()  {}
  virtual void reset() {}
  virtual int current_position() { return 0; }
  
public:
#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  // sizing
  int width()    const { return _width;    }
  int position() const { return _position; }

  void print_double(double d) {
    jdouble_accessor tmp;
    tmp.double_value = d;
    jint msw = tmp.words[MSW_FIRST_FOR_DOUBLE ? 0 : 1];
    jint lsw = tmp.words[MSW_FIRST_FOR_DOUBLE ? 1 : 0];
    if (msw == 0x7ff00000 && lsw == 0x00000000) {
      print("Inf");
    } else if (msw == 0xfff00000 && lsw == 0x00000000) {
      print("-Inf");
    } else if (msw == 0x7ff80000 && lsw == 0x00000000) {
      print("NaN");
    } else {
      print("%g", d);
    }
  }
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE || ENABLE_PERFORMANCE_COUNTERS \
    || ENABLE_WTK_PROFILER
  void put(char ch);
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void inc() { _indentation++; };
  void dec() { _indentation--; };
  void dec_cr() { dec(); cr(); }
  void inc_cr() { inc(); cr(); }
  // indentation
  void indent(); 
  int  indentation() const    { return _indentation; }
  void set_indentation(int i) { _indentation = i;    }

  // flushing
  virtual void flush() {}

  // Print a hex number with 8 digits, including leading zeros.
  void print_hex8(int n);
  void print_hex8(address a) {
    print_hex8((int)a);
  }

#endif
 protected:
  int _indentation; // current indentation
  int _width;       // width of the page
  int _position;    // position on the current line
};

extern Stream* tty;        // tty output

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_MEMORY_PROFILER \
    || ENABLE_WTK_PROFILER || ENABLE_PERFORMANCE_COUNTERS || ENABLE_PROFILER \
    || ENABLE_DYNAMIC_NATIVE_METHODS || ENABLE_TTY_TRACE || USE_EVENT_LOGGER

class FileStreamState {
public:
  int _indentation;
  int _width;
  int _position;
  OsFile_Handle _file;
};

/**
 * Streams that print on files.
 */
class FileStream : public Stream {
 protected:
  OsFile_Handle _file;
  bool _saved;
  int _mark;
 public:
  FileStream() { _file = 0; _saved = false; }
  FileStream(const JvmPathChar* file_name, int width = 80);
  FileStream(OsFile_Handle file) { _file = file; }
  ~FileStream() {
    if (!_saved) {
      close();
    }
  }
  void open(const JvmPathChar* file_name);
  int is_open() const { return _file != NULL; }
  virtual void print_raw(const char* c);
  virtual void flush();
  virtual void close();
  void close(FileStreamState* state) {
    state->_file = NULL;
    close();
  }
  void save(FileStreamState* state);
  void restore(FileStreamState* state);
  void mark();
  //REturns back to mark
  void reset();
  int current_position();
};

class FixedArrayOutputStream : public Stream { 
public:
  FixedArrayOutputStream() {
    _current_size = 0;
    _array = _stack_array;
    _limit = ArraySize;
  }
  FixedArrayOutputStream(char *array, int limit) {
    _current_size = 0;
    _array = array;
    _limit = limit;
  }
  ~FixedArrayOutputStream()  { }
  virtual void flush() {}
  virtual void print_raw(const char* c);
  char *array() { return _array; }
  void reset()  { _current_size = 0; _array[0] = 0; }
  int current_size() {return _current_size;}
private:
  enum { ArraySize = 1024 };
  char  _stack_array[ArraySize];
  char *_array;
  int   _limit;
  int   _current_size;
};

#endif // !defined(PRODUCT) || ENABLE_ROM_GENERATOR

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

/**
 * Redirection and duplication of output into 2 streams.
 */
class DupStream : public Stream {
protected:
  Stream *_stream1;
  Stream *_stream2;
public:
  DupStream(Stream *stream1, Stream* stream2) {
    _stream1 = stream1;
    _stream2 = stream2;
  }

  virtual void print_raw(const char *c);

  virtual void flush();

  Stream* stream1() { return _stream1; }
  Stream* stream2() { return _stream2; }
};

#endif // !defined(PRODUCT) || ENABLE_TTY_TRACE

#if ENABLE_PERFORMANCE_COUNTERS || USE_DEBUG_PRINTING || ENABLE_JVMPI_PROFILE
/**
 * "Print" into byte arrays, which can be dynamically expanded.
 */
class ByteArrayOutputStream : public Stream { 
public:
  ByteArrayOutputStream(): _array(NULL), _current_size(0), _array_size(0) { }
  ~ByteArrayOutputStream() {
    if (_array != NULL) {
      jvm_free(_array);
    }
  }
  virtual void flush() {}
  virtual void print_raw(const char* c);
  void dump_to(Stream *s) {
    s->print_raw(_array); _current_size = 0; _array[0] = 0;
  }
  char* get_chars() {
    return _array;
  }
private:
  char *_array;
  int  _current_size;
  int  _array_size;
};
#endif

#if USE_BINARY_IMAGE_GENERATOR

/**
 * Binary output stream for creating files
 * that contain raw binary (non-character) data.
 */
#define BFS_BUFFER_SIZE 1024

class BufferedFileStreamState {
public:
  int _indentation;
  int _width;
  int _position;
  juint _counter;
  OsFile_Handle _file;
  bool _has_error;
  int _bfs_pos;
};

class BufferedFileStream : public Stream {
public:
  void open(const JvmPathChar* file_name);
  virtual void print_raw(const char* c);
  void print_int(const juint i);
  void print_int_ref(const juint i) {
    // This used to have a different meaning, but now it's just the same
    // as print_int
    print_int(i);
  }
  void print_char(const char c);
  juint get_counter();
  virtual void close();
  void close(BufferedFileStreamState* state) {
    state->_file = NULL;
    close();
  }
  inline void flush_buffer(bool force) {
    if (force || _bfs_pos >= BFS_BUFFER_SIZE) {
      do_flush();
    }
  }
  void do_flush();

  // Did we encounter any error during writing?  
  bool has_error() {
    return _has_error;
  }
  void save(BufferedFileStreamState* state);
  void restore(BufferedFileStreamState* state);
private:
  juint _counter;
  OsFile_Handle _file;
  bool _has_error;
  int _bfs_pos;
};

#endif /* USE_BINARY_IMAGE_GENERATOR */
