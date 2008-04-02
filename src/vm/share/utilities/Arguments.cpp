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

/*
 * Arguments.cpp: Command line argument parsing and handling
 *
 * This file implements the command line argument parsing
 * operations for those VM ports that support command
 * line operation.
 *
 * Below is a list of the most commonly used command line arguments:
 *
 * -classpath <OR> -cp  Set search path for application classes and resources
 * -verbose             Enable verbose output
 * -int                 Execute code in pure interpreter mode (no compilation)
 * -comp                Execute code in pure compiler mode (no interpretation)
 *
 * In addition, the VM provides a large number
 * of development-time options for turning on various
 * execution modes.  Refer to the Porting Guide and file
 * "/src/vm/share/utilities/Globals.hpp" for details.
 *
 * The following two command line options are available
 * in non-product builds to statically generate the assembly
 * interpreter and stubs:
 *
 * -generate            Generate the debug version of the assembly interpreter
 * -generateoptimized   Generate the product version of the asm interpreter
 *
 * Refer to file "/src/vm/share/utilities/Generator.cpp" for
 * documentation on the assembly code generator.
 */

#include "incls/_precompiled.incl"
#include "incls/_Arguments.cpp.incl"

Arguments::Path            Arguments::_classpath;

#if ENABLE_JAVA_DEBUGGER
int                        Arguments::_debugger_port;
#endif

#if ENABLE_INTERPRETER_GENERATOR || USE_SOURCE_IMAGE_GENERATOR
Arguments::Path            Arguments::_generator_output_dir;
#endif

#if ENABLE_ROM_GENERATOR
Arguments::Path            Arguments::_rom_config_file;
Arguments::Path            Arguments::_rom_output_file;
Arguments::ROMIncludePath* Arguments::_rom_include_paths;
#endif

#if USE_BINARY_IMAGE_GENERATOR
Arguments::Path            Arguments::_rom_input_file;
#endif

#ifndef PRODUCT
Arguments::Path            Arguments::_compiler_test_config_file;
#endif

#if ENABLE_JVMPI_PROFILE 
//Initialize the proflie library name 
char* Arguments::_jvmpi_profiler_lib = NULL;
#endif

#define COMPILE_ONLY_METHOD        "CompileOnlyMethod="
#define COMPILE_ONLY_CLASS         "CompileOnlyClass="
#define METHOD_TRAP                "MethodTrap="

int Arguments::parse_one_arg(int argc, char** argv) {
  if (argc <= 0) {
    // we already finished parsing.
    return 0;
  }

  int count = 1; // in most cases we find an argument of 1 word

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  if ((jvm_strcmp(argv[0], "-profile") == 0) && argc >= 2) {
    JVM_SetProfile(argv[1]);
    count = 2;
  } else
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT
  if (jvm_strcmp(argv[0], "-version") == 0) {
    Globals::print_version();
    JVMSPI_Exit(0);
    ::jvm_exit(0);
  }
  else if ((jvm_strcmp(argv[0], "-?") == 0) ||
           (jvm_strcmp(argv[0], "-help") == 0)) {
    JVMSPI_DisplayUsage(NULL);
    JVMSPI_Exit(1);
    ::jvm_exit(1);
  }
  else if (((jvm_strcmp(argv[0], "-classpath") == 0) ||
            (jvm_strcmp(argv[0], "-cp") == 0)) && argc >= 2) {
    set_pathname_from_const_ascii(&_classpath, argv[1]);
    count = 2;
  }
  else if (jvm_strcmp(argv[0], "-int") == 0) {
    UseCompiler = false;
  }
  else if (jvm_strcmp(argv[0], "-comp") == 0) {
    MixedMode = false;
    UseCompiler = true;
  }
  else if ((*argv[0] == '-') && (*(argv[0]+1) == 'D')) {
    char *key;
    char *value;

    key = argv[0] + 2;
    for (value = key; *value ; value++) {
        if (*value == '=') {
          *value++ = 0;
          break;
        }
    }
    JVMSPI_SetSystemProperty(key, value);
  }

#if ENABLE_INTERPRETER_GENERATOR || USE_SOURCE_IMAGE_GENERATOR
  else if (jvm_strcmp(argv[0], "-outputdir") == 0) {
    set_pathname_from_const_ascii(&_generator_output_dir, argv[1]);
    count = 2;
  }
  else if (jvm_strcmp(argv[0], "-generate") == 0) {
    GenerateDebugAssembly = true;
    GenerateAssemblyCode = true;
  }
  else if (jvm_strcmp(argv[0], "-generateoptimized") == 0) {
    GenerateDebugAssembly = false;
    GenerateAssemblyCode = true;
  }
#endif

#if ENABLE_JVMPI_PROFILE 
  //  To get the library name to be loaded
  else if(jvm_strncmp(argv[0], "-Xrun", 5) == 0) {
    Arguments::_jvmpi_profiler_lib = (char*)OsMemory_allocate(jvm_strlen(argv[0]) + 2);
    jvm_strcpy(Arguments::_jvmpi_profiler_lib, "lib");
    jvm_strcat(Arguments::_jvmpi_profiler_lib, &(argv[0][5]));
    jvm_strcat(Arguments::_jvmpi_profiler_lib, ".so");
  }
#endif

#if ENABLE_ROM_GENERATOR

#if USE_SOURCE_IMAGE_GENERATOR
  else if (jvm_strcmp(argv[0], "-romconfig") == 0) {
    set_pathname_from_const_ascii(&_rom_config_file, argv[1]);
    count = 2;
  }
  else if (jvm_strcmp(argv[0], "-romincludepath") == 0) {
    ROMIncludePath *ptr =
      (ROMIncludePath*)OsMemory_allocate(sizeof(ROMIncludePath));
    ptr->_next = _rom_include_paths;
    ptr->_file._path = NULL;
    set_pathname_from_const_ascii(&(ptr->_file), argv[1]);
    _rom_include_paths = ptr;
    count = 2;
  }
  else if (jvm_strcmp(argv[0], "-romize") == 0) {
    // Romize the system classes
    GenerateROMImage = true;
#if ENABLE_TTY_TRACE
    TraceMirandaMethods = true;
#endif
    count = 1;
  }
#endif // USE_SOURCE_IMAGE_GENERATOR

#if USE_BINARY_IMAGE_GENERATOR
  else if (jvm_strcmp(argv[0], "-convert") == 0) {
    // Romize an application
    GenerateROMImage = true;
#if ENABLE_TTY_TRACE
    TraceMirandaMethods = true;
#endif
    count = 1;
  }
#if ENABLE_LIB_IMAGES
  else if (jvm_strcmp(argv[0], "-convertshared") == 0) {
    // Romize a library, so it could be used shared 
    GenerateSharedROMImage = true;
    GenerateROMImage = true;
#if ENABLE_TTY_TRACE
    TraceMirandaMethods = true;
#endif
    count = 1;
  }
#endif
#endif

  else if (jvm_strcmp(argv[0], "-romoutputfile") == 0) {
    set_pathname_from_const_ascii(&_rom_output_file, argv[1]);
    count = 2;
  }

#endif // ENABLE_ROM_GENERATOR

#if USE_DEBUG_PRINTING
  else if (jvm_strcmp(argv[0], "-definitions") == 0) {
    Globals::print_definitions();
    JVMSPI_Exit(1);
    ::jvm_exit(1);
  }
  else if (jvm_strcmp(argv[0], "-flags") == 0) {
    Globals::print_flags();
    JVMSPI_Exit(1);
    ::jvm_exit(1);
  }
  else if (jvm_strcmp(argv[0], "-buildopts") == 0) {
    Globals::print_build_options();
    JVMSPI_Exit(1);
    ::jvm_exit(1);
  }
  else if (jvm_strcmp(argv[0], "-errorcodes") == 0) {
    Globals::print_error_codes();
    JVMSPI_Exit(1);
    ::jvm_exit(1);
  }
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  else if (jvm_strcmp(argv[0], "-verbose") == 0) {
#if ENABLE_TTY_TRACE
    VerboseGC = true;
    VerboseClassLoading = true;
#endif
  }
  // Parse the CompileOnlyXXX= option, if present, and record the
  // classes and methods that should always be compiled.  This must
  // be done before the following to allow correct handling of
  // classes which start with the letters K or M
  else if (jvm_strncmp(argv[0], COMPILE_ONLY_METHOD,
                       STATIC_STRLEN(COMPILE_ONLY_METHOD)) == 0) {
    _method_CompileOnly = argv[0] + STATIC_STRLEN(COMPILE_ONLY_METHOD);
  }
  else if (jvm_strncmp(argv[0], COMPILE_ONLY_CLASS,
                       STATIC_STRLEN(COMPILE_ONLY_CLASS)) == 0) {
    _class_CompileOnly = argv[0] + STATIC_STRLEN(COMPILE_ONLY_CLASS);
  }
#endif
#if !defined(PRODUCT)
  else if (jvm_strcmp(argv[0], "-compilertestconfig") == 0) {
    if (argc < 2) {
      JVMSPI_DisplayUsage((char*)"Compiler test config file not specified.");
      JVMSPI_Exit(1);
      ::jvm_exit(1);
    }
    RunCompilerTests = true;
    set_pathname_from_const_ascii(&_compiler_test_config_file, argv[1]);
    count = 2;
  }
#endif /* NOT PRODUCT */
#if ENABLE_METHOD_TRAPS
  else if (jvm_strncmp(argv[0], METHOD_TRAP, STATIC_STRLEN(METHOD_TRAP)) == 0) {
    if (!parse_method_trap_param(argv[0] + STATIC_STRLEN(METHOD_TRAP))) {
      JVMSPI_DisplayUsage((char*)"Invalid usage of MethodTrap argument");
      JVMSPI_Exit(1);
      ::jvm_exit(1);
    }
  }
#endif
#if ENABLE_REMOTE_TRACER
  else if (((jvm_strcmp(argv[0], "-tracehost") == 0)) && argc > 2) {
    traceHost = argv[1];
    count = 2;
  }
#endif
#if ENABLE_JAVA_DEBUGGER
  else if (jvm_strcmp(argv[0], "-debugger") == 0) {
    JavaDebugger::set_debugger_option_on(true);
  } else if (jvm_strcmp(argv[0], "-port") == 0) {
    _debugger_port = jvm_atoi(argv[1]);
    count=2;
  } else if (jvm_strcmp(argv[0], "-suspend") == 0) {
    JavaDebugger::set_suspend(true);
  } else if (jvm_strcmp(argv[0], "-nosuspend") == 0) {
    JavaDebugger::set_suspend(false);
  }
#if ENABLE_ISOLATES
  else if (jvm_strcmp(argv[0], "-debug_isolate") == 0) {
    JavaDebugger::set_debug_isolate_option_on(true);
  }
  else if (jvm_strcmp(argv[0], "-debug_main") == 0) {
    JavaDebugger::set_debug_main_option_on(true);
  }
#endif
#endif
#if ENABLE_MEMORY_PROFILER
  else if (jvm_strcmp(argv[0], "-memory_profiler") == 0) {
    JavaDebugger::set_debugger_option_on(true);
  }
#endif
  else if ((argv[0][0] == '-')||(argv[0][0] == '+') || (argv[0][0] == '=')) {
    if (!Globals::parse_argument(argv[0])) {
      count = -1; // Indicate parse error
    }
#if ENABLE_ROM_GENERATOR
    if (jvm_strcmp(argv[0], "+EnableAllROMOptimizations") == 0) {
      // Turn on all available ROM optimizations, so you don't have to
      // enable them individually
      //
      // This also means you can selectively turn off some ROM
      // optimizations such as "+EnableAllROMOptimizations
      // -CompactROMFieldTables". Note that the order is important.
      AggressiveROMSymbolRenaming = true;
      CompactROMFieldTables       = true;
      CompactROMMethodTables      = true;
      CompactROMBytecodes         = true;
      RenameNonPublicROMClasses   = true;
      RenameNonPublicROMSymbols   = true;
      SimpleROMInliner            = true;
      RemoveDuplicatedROMStackmaps= true;
      RemoveDeadMethods           = true;
    }
#if ENABLE_ROM_JAVA_DEBUGGER
    if (jvm_strcmp(argv[0], "+MakeROMDebuggable") == 0) {
      // Turn off any optimizations that would change bytecode offsets
      CompactROMFieldTables = false;
      CompactROMMethodTables = false;
      CompactROMBytecodes         = false;
      RenameNonPublicROMSymbols   = false;
      AggressiveROMSymbolRenaming = false;
    }
#endif
    if (jvm_strcmp(argv[0], "-EnableBaseOptimizations") == 0) {
      // Turn off all optimizations
      AggressiveROMSymbolRenaming = false;
      CompactROMFieldTables       = false;
      CompactROMMethodTables      = false;
      CompactROMBytecodes         = false;
      RenameNonPublicROMClasses   = false;
      RenameNonPublicROMSymbols   = false;
      SimpleROMInliner            = false;
      RemoveDuplicatedROMStackmaps= false;
      RemoveDeadMethods           = false;
      RewriteROMConstantPool      = false;
    }
#endif
  }
  else {
    /* We don't recognize argv[0] as an argument */
    count = 0;
  }

  return count;
}

#if ENABLE_METHOD_TRAPS
/*
 * Parses MethodTrap=... command-line argument.
 * @param p - pointer to the first char after MethodTrap=
 */
bool Arguments::parse_method_trap_param(const char* arg) {
  // Discard superfluous MethodTrap arguments issuing the warning message
  MethodTrapDesc* mt = MethodTrap::find_slot();
  if (mt == NULL) {
    tty->print_cr("WARNING: too many MethodTrap arguments, superfluous will be ignored");
    return true;
  }

  // Simple correctness test
  if (jvm_strchr(arg, '.') <= arg) {
    return false;
  }

  // Copy all chars after MethodTrap=
  char* p = (char*)OsMemory_allocate(jvm_strlen(arg) + 1);
  jvm_strcpy(p, arg);

  // Parse ACTION parameter if exists
  int action = ACTION_CALLBACK;
  char* q = jvm_strchr(p, ':');
  if (q != NULL) {
    *q++ = 0;
    if (jvm_strncmp(q, "call", 4) == 0) {
      action = ACTION_CALLBACK;
    } else if (jvm_strncmp(q, "exit", 4) == 0) {
      action = ACTION_EXIT;
    } else if (jvm_strncmp(q, "stop", 4) == 0) {
      action = ACTION_STOP_ISOLATE;
    } else if (jvm_strncmp(q, "break", 5) == 0) {
      action = ACTION_BREAKPOINT;
    } else {
      action = jvm_atoi(q);
    }
  }
  
  // Parse CALL_COUNT parameter if exists
  int call_count = 0;
  q = jvm_strchr(p, ',');
  if (q != NULL) {
    *q++ = 0;
    call_count = jvm_atoi(q);
  }

  mt->set_method_name(p);
  mt->set_parameters(call_count, action, 0);
  return true;
}
#endif

//--------------------------------------------------
//             CompileOnly helpers
//--------------------------------------------------

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

char* Arguments::_method_CompileOnly;
char* Arguments::_class_CompileOnly;

bool Arguments::must_compile_method(Symbol* class_name, Symbol* method_name
                                    JVM_TRAPS) {
  bool class_exists = false, method_exists = false;
  if (_class_CompileOnly != NULL) {
    // allow user to specify CompileOnlyClass=java/lang/Object or
    //                       CompileOnlyClass=java.lang.Object or
    //                       CompileOnlyClass=Object
    Symbol n = SymbolTable::slashified_symbol_for(_class_CompileOnly 
                                                  JVM_NO_CHECK);
    Symbol m = SymbolTable::symbol_for(_class_CompileOnly  JVM_NO_CHECK);
    if (n.is_null() || m.is_null()) {
      Thread::clear_current_pending_exception();
      return false;
    }
    bool ends_with = false;
    int class_len = class_name->length();
    int only_len = strlen(_class_CompileOnly);
    if (class_len >= only_len) {
      const char *p = class_name->base_address() + class_len - only_len;
      const char *q = _class_CompileOnly;

      if (class_len == only_len || p[-1] == '/') {
        // We require full class name match. E.g., Object matches with
        // java.lang.Object, but bject does not match.
        while (*q) {
          if (*p != *q) {
            break;
          }
          p++; q++;
        }
        if (*q == '\0') {
          ends_with = true;
        }
      }
    }

    if (n.equals(class_name) || m.equals(class_name) || ends_with) {
      class_exists = true;
      if (_method_CompileOnly == NULL) {
        return true; // compile this
      }
    }
  }

  if (_method_CompileOnly != NULL) {
    Symbol n = SymbolTable::symbol_for(_method_CompileOnly JVM_NO_CHECK);
    if (n.is_null()) {
      Thread::clear_current_pending_exception();
      return false;
    }
    if (n.equals(method_name)) {
      method_exists = true;
      if (_class_CompileOnly == NULL) {
        return true; // compile this
      }
    }
  }

  return class_exists && method_exists;
}

#endif /* NOT PRODUCT */

extern "C" void
JVM_Initialize(void) {
  if (tty == NULL) {
    Stream::initialize();
  }
  Arguments::initialize();

#if ENABLE_METHOD_TRAPS
  MethodTrap::initialize();
#endif
}

extern "C" int
JVM_ParseOneArg(int argc, char **argv) {
  return Arguments::parse_one_arg(argc, argv);
}

#if USE_UNICODE_FOR_FILENAMES
void Arguments::set_pathname_from_const_ascii(Path *dst, const char* value) {
  free_pathname(dst);

  int len = jvm_strlen(value);
  JvmPathChar *p;
  size_t size = (len+1) * sizeof(JvmPathChar);
  if ((p = (JvmPathChar*)OsMemory_allocate(size)) != NULL) {
    for (int i=0; i<len; i++) {
      p[i] = (JvmPathChar)value[i];
    }
    p[len] = 0;
    dst->_path = p;
    dst->_need2free = true;
  }
}

void Arguments::set_pathname(Path *dst, const JvmPathChar *value, 
                             bool need2free) {
  free_pathname(dst);
  dst->_path = value;
  dst->_need2free = need2free;
}

void Arguments::free_pathname(Path *dst) {
  if (dst->_path != NULL && dst->_need2free) {
    OsMemory_free((void*)dst->_path);
  }
  dst->_path = NULL;
  dst->_need2free = 0;
}
#endif

void Arguments::finalize() {
  free_pathname(&_classpath);
  
#if ENABLE_INTERPRETER_GENERATOR || USE_SOURCE_IMAGE_GENERATOR
  free_pathname(&_generator_output_dir);
#endif

#if ENABLE_ROM_GENERATOR
  free_pathname(&_rom_config_file);
  free_pathname(&_rom_output_file);

  for (ROMIncludePath *ptr = _rom_include_paths; ptr; ) {
    ROMIncludePath *dead = ptr;
    ptr = ptr->next();
    free_pathname(&dead->_file);
    OsMemory_free((void*)dead);
  }
  _rom_include_paths = NULL;
#endif

#ifndef PRODUCT
  free_pathname(&_compiler_test_config_file);
#endif
}
