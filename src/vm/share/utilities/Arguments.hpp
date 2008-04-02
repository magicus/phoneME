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

/** \class Arguments
 * Command line argument parsing and handling
 *
 * Refer to file "Arguments.cpp" for a detailed description
 * of the command line arguments that are intended to be
 * available in those VM ports that support VM invocation
 * from the command line.
 */
class Arguments {
public:

  struct Path {
    const JvmPathChar * _path;
#if USE_UNICODE_FOR_FILENAMES
    bool _need2free;
#endif
  };

  /** \class ROMIncludePath
   * Records the value of -romincludepath command-line arguments.
   */
  struct ROMIncludePath {
    Path             _file;
    ROMIncludePath * _next;
  public:
    ROMIncludePath * next() {
      return _next;
    }
    const JvmPathChar * path() {
      return _file._path;
    }
  friend class Arguments;
  };

private:
  static Path _classpath;

#if ENABLE_ROM_GENERATOR
  static Path             _rom_output_file;
  static Path             _rom_config_file;
  static ROMIncludePath * _rom_include_paths;
#endif

#if USE_BINARY_IMAGE_GENERATOR
  static Path             _rom_input_file;
#endif

#if ENABLE_INTERPRETER_GENERATOR || USE_SOURCE_IMAGE_GENERATOR
  static Path             _generator_output_dir;
#endif

public:

#if ENABLE_JAVA_DEBUGGER
  static int    _debugger_port;
#endif

#if ENABLE_JVMPI_PROFILE 
  static char* _jvmpi_profiler_lib;   // save the loading library name
#endif 

#if ENABLE_METHOD_TRAPS
  static bool parse_method_trap_param(const char *p);
#endif

  static void initialize() {
    _classpath._path = NULL;

#if ENABLE_ROM_GENERATOR
    _rom_output_file._path = NULL;
    _rom_config_file._path = NULL;
    _rom_include_paths = NULL;
#endif

#if ENABLE_INTERPRETER_GENERATOR || USE_SOURCE_IMAGE_GENERATOR
    _generator_output_dir._path = NULL;
#endif

#ifndef PRODUCT
    _compiler_test_config_file._path = NULL;
#endif
  }

  static void finalize();

 private:

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  static char* _class_CompileOnly;
  static char* _method_CompileOnly;
 private:
  static Path _compiler_test_config_file;
 public:
  static const JvmPathChar* compiler_test_config_file() {
    return _compiler_test_config_file._path;
  }
#endif

 private:
  /*
   * set_pathname_from_const_ascii() is usually used only on "test"
   * platforms such as linux and win32. Even these platforms support
   * UNICODE pathnames, it's often more convenient to start the VM or
   * MIDP program with a main() function that has only ascii
   * arguments. Therefore, if the VM is built in UNICODE mode, we need
   * to convert filenames, such as the ascii value for the "-cp"
   * argument, into UNICODE.
   *
   * On "real" platforms, set_pathname_from_const_ascii() is usually
   * never invoked -- usually there's no such thing as (argc,argv) for
   * the main function, and filenames such as classpath would be set
   * explicitly using the <classpath> parameter to JVM_Start() and
   * JVM_Start2().
   */
#if USE_UNICODE_FOR_FILENAMES
  static void set_pathname_from_const_ascii(Path *dst, 
                                            const char *value);
  static void free_pathname(Path *dst);
  static void set_pathname(Path *dst, const JvmPathChar *value, 
                           bool need2free);
#else
  static void set_pathname_from_const_ascii(Path *dst, const char *value) {
    // Note that we don't duplicate the string in <value> -- we assume that
    // this string will remain valid during the entire lifetime of the VM.
    dst->_path = value;
  }
  static void free_pathname(Path *dst) {
    dst->_path = NULL;
  }
  static void set_pathname(Path *dst, const JvmPathChar *value, 
                           bool need2free) {
    (void)need2free;
    dst->_path = value;
  }
#endif


 public:
  /**
   * Parse one command line argument and return the number of command
   * line tokens it takes.
   * Return '0' if no more argument present.
   * Return '-1' if parse error occurs.
   */
  static int parse_one_arg(int argc, char** argv);

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  // CompileOnly handling
  static bool must_check_CompileOnly() {
    return _method_CompileOnly != NULL || _class_CompileOnly != NULL;
  }

  static bool must_compile_method(Symbol* class_name, Symbol* method_mame
                                  JVM_TRAPS);
#endif

#if ENABLE_ROM_GENERATOR
  static ROMIncludePath *rom_include_paths() {
    return _rom_include_paths;
  }
  static const JvmPathChar* rom_output_file() {
    if (_rom_output_file._path) {
      return _rom_output_file._path;
    } else {
      return USE_SOURCE_IMAGE_GENERATOR ?
             FilePath::default_source_rom_file :
             FilePath::default_binary_rom_file;
    }
  }
  static const JvmPathChar* rom_config_file() {
    return _rom_config_file._path;
  }
#endif

#if ENABLE_INTERPRETER_GENERATOR || USE_SOURCE_IMAGE_GENERATOR
  static const JvmPathChar* generator_output_dir() {
    return _generator_output_dir._path;
  }
#endif

  static const JvmPathChar* classpath() {
    return _classpath._path;
  }

#if USE_BINARY_IMAGE_GENERATOR
  static const JvmPathChar* rom_input_file() {
    return _rom_input_file._path;
  }
  static void set_rom_input_file(const JvmPathChar *input, bool need2free) {
    set_pathname(&_rom_input_file, input, need2free);
  }
  static void set_rom_output_file(const JvmPathChar *output, bool need2free) {
    set_pathname(&_rom_output_file, output, need2free);
  }

#if USE_UNICODE_FOR_FILENAMES
  // Used by command-line conversion only.
  static void set_rom_input_file(char *input);
#endif

#endif

};
