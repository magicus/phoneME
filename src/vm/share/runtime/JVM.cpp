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
 * JVM.cpp: VM startup and shutdown routines.
 *
 * This file defines routines for virtual machine
 * startup and shutdown.  The routines in this file
 * are intended to be portable across a large number
 * of ports.  The platform-specific implementations
 * of the startup and shutdown routines should be
 * placed in the OS-specific
 * "/src/vm/os/<os_name>/JVM_<os_name>.cpp" files.
 *
 */

// Main file for the VM

#include "incls/_precompiled.incl"
#include "incls/_JVM.cpp.incl"

int          JVM::_exit_code;
bool         JVM::_is_started;
char       * JVM::_main_class;
int          JVM::_argc;
char      ** JVM::_argv;
jchar     ** JVM::_u_argv;
const JvmPathChar* JVM::_classpath;
int          JVM::_startup_phase_count;

// For release and product builds we make sure that we have
// the optimized interpreter.
#ifdef AZZERT
extern "C" {
  int please_use_optimized_interpreter_with_release_or_product_builds;
}
#endif

extern "C" void loopgen_check_oopmaps();
extern "C" void romgen_check_oopmaps();

ReturnOop JVM::resolve_class(char* class_name JVM_TRAPS) {
  UsingFastOops fast_oops;
  Symbol::Fast class_name_symbol =
      SymbolTable::slashified_symbol_for(class_name JVM_CHECK_0);
  return SystemDictionary::resolve(&class_name_symbol, ErrorOnFailure
                                   JVM_NO_CHECK_AT_BOTTOM_0);
}

void JVM::set_arguments(const JvmPathChar *classpath, char *main_class, 
                        int argc, char **argv) {
  JVM::set_arguments2(classpath, main_class, argc, argv, NULL, false);
}

void JVM::set_arguments2(const JvmPathChar *classpath, char *main_class, 
                         int argc, char **argv, jchar **u_argv,
                         bool is_unicode) {
  // [1] Initialize _classpath
  _classpath = classpath;

  if (classpath == NULL) {
    _classpath = Arguments::classpath();
  }
  if (_classpath == NULL) {
    _classpath = OsMisc_get_classpath();
  }
  if (_classpath == NULL) {
    static const JvmPathChar default_classpath[] = {'.', 0};
    _classpath = default_classpath;
  }

  GUARANTEE(_classpath != NULL, "class path must be set");

  if (!is_unicode) {
    // [2] Initialize _main_class;
    _main_class = main_class;
    if (_main_class == NULL) {
      if (!GenerateAssemblyCode && !GenerateOopMaps && !GenerateROMImage
                       && !VerifyOnly && !TestCompiler && !RunCompilerTests) {
        _main_class = argv[0];
        argc --;
        argv ++;
        if (argc < 0) {
          JVMSPI_DisplayUsage((char*)"class not specified");
          JVMSPI_Exit(1);
          SHOULD_NOT_REACH_HERE();
        }
      }
    }
    // [3] Initialize _argc, _argv
    _argc = argc;
    _argv = argv;
  } else {
    GUARANTEE(_main_class != NULL, "main class is not found");
    argc--;
    u_argv++;
    _argc = argc;
    _u_argv = u_argv;
  }
}

// Abruptly stops the execution of all threads in the VM, including
// threads that may be (1) sleeping, (2) holding locks, and (3) waiting
// for locks. This function should be called with great care, only when
// you know that all of your Java code has done proper cleanup.
void JVM::stop(int code) {
  Universe::set_stopping();
  _exit_code = code;

  Scheduler::terminate_all();
  // <---- No more Java code will be executed beyond this point


  // No java threads may have run so the primordial_sp may be 0
  if (_primordial_sp == NULL) {
    // no threads have run, so return to caller
    return;
  } else {
    // We may come here with a non-NULL _last_handle if JVM_Stop is called
    // inside JVMSPI_CheckEvents(). Once we return (back to the caller
    // of JVM::run), all of the handles will be pointing to unused space on
    // the C stack. So let's invalidate them.
    _last_handle = NULL;
#ifdef AZZERT    
    last_raw_handle = NULL;
#endif
    current_thread_to_primordial();
  }
}

void JVM::exit(int code) {
  cleanup();
  Arguments::finalize();
  JVMSPI_Exit(code);
  ::jvm_exit(code); // just in case 'JVMSPI_Exit()' forgot to terminate
                    // the process
}

ReturnOop JVM::get_main_args(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;

  ObjArray::Fast arguments =
      Universe::new_obj_array(Universe::string_class(), _argc JVM_CHECK_0);
  if (_argv != NULL) {
    for (int index = 0; index < _argc; index++) {
      char* arg = _argv[index];
      String::Raw s = Universe::new_string(arg, jvm_strlen(arg) JVM_CHECK_0);
      arguments().obj_at_put(index, s);
    }
  } else {
    for (int index = 0; index < _argc; index++ ) {
      jchar *arg = _u_argv[index];
      int length = unicode_strlen(arg);
      TypeArray ta = Universe::new_char_array(length JVM_CHECK_0);
      for (int i = 0; i < length; i++) {
        ta.char_at_put(i, arg[i]);
      }
      String::Raw s = Universe::new_string(&ta, 0, ta.length() JVM_CHECK_0);
      arguments().obj_at_put(index, s);
    }
  }
  return arguments;
}


// Load the main class into the EntryActivation list of the main
// thread.
inline bool JVM::load_main_class(JVM_SINGLE_ARG_TRAPS) {

  UsingFastOops fast_oops;
  InstanceClass::Fast klass = JVM::resolve_class(_main_class JVM_CHECK_0);

  // Make sure the class is initialized
  klass().initialize(JVM_SINGLE_ARG_CHECK_0);

  // Basic bootstrapping done, set young generation boundary
  ObjectHeap::set_collection_area_boundary(0, false);

  // Make the argument list
  ObjArray::Fast arguments = get_main_args(JVM_SINGLE_ARG_CHECK_0);

  // Find the method to invoke
  Method::Fast main_method =
      klass().lookup_method(Symbols::main_name(),
                          Symbols::string_array_void_signature());
  if (main_method.is_null() || !main_method().is_static()) {
    Throw::error(main_method_not_found JVM_THROW_0);
  }

  JVM::development_prologue();

#if ENABLE_PROFILER
  Profiler::initialize();
  if (UseProfiler) {
    Profiler::engage();
  }
#endif

#if ENABLE_WTK_PROFILER
  WTKProfiler::initialize();
  if (UseExactProfiler) {
    WTKProfiler::engage();
  }
#endif

#if ENABLE_METHOD_TRAPS
  MethodTrap::activate_initial_traps();
#endif

  // Create a delayed activation for the main method
  EntryActivation::Fast entry =
      Universe::new_entry_activation(&main_method, 1 JVM_CHECK_0);
  entry().obj_at_put(0, &arguments);
  Thread::Fast thread = Thread::current();
  thread().append_pending_entry(&entry);

  return true;
}

void JVM::run() {
  GUARANTEE(!Scheduler::is_slave_mode(), "sanity");
  // It is important that there be no pending handles when this is called.
  // When primoridal_to_current_thread() returns, we may be in a different
  // Java thread, and the GC will be very confused.
  GUARANTEE(_last_handle == NULL, "No handles");
  GUARANTEE(last_raw_handle == NULL, "No Raw handles");
  primordial_to_current_thread();
}

  // We put this in solely for the purpose
  // so that one can look at the executable binary
  // with a text browser and look at the copyright notice:
const char *JVM::copyright =
   " Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights"
   " Reserved.  Use is subject to license terms."
   " DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER"
   " "
   " This program is free software; you can redistribute it and/or"
   " modify it under the terms of the GNU General Public License version"
   " 2 only, as published by the Free Software Foundation."
   " "
   " This program is distributed in the hope that it will be useful, but"
   " WITHOUT ANY WARRANTY; without even the implied warranty of"
   " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU"
   " General Public License version 2 for more details (a copy is"
   " included at /legal/license.txt)."
   " "
   " You should have received a copy of the GNU General Public License"
   " version 2 along with this work; if not, write to the Free Software"
   " Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA"
   " 02110-1301 USA"
   " "
   " Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa"
   " Clara, CA 95054 or visit www.sun.com if you need additional"
   " information or have any questions.";
  // \xA9 is the 8-bit ASCII copyright sign

inline bool JVM::initialize( void ) {
  // Just making sure copyright does not get optimized away
  // by the C++ compiler:
  if (copyright[0] == '?') {
    JVM::exit(0); // anything, never reached
  }

  if (Verbose) {
    JVM::print_parameters();
  }

  _is_started = false;
  _exit_code = 0;
  _startup_phase_count = 0;
  Os::initialize();
  EventLogger::initialize();

#if ENABLE_PERFORMANCE_COUNTERS
  JVM::calibrate_cpu();
#endif

#if USE_SOURCE_IMAGE_GENERATOR
  SourceROMWriter::initialize();
#endif
#if USE_BINARY_IMAGE_GENERATOR
  BinaryROMWriter::initialize();
#endif

  // it must be after Os::initialize(), as depends on hrtick frequency
#if ENABLE_PERFORMANCE_COUNTERS
  JVM_ResetPerformanceCounters();
#endif

#ifndef PRODUCT
  loopgen_check_oopmaps();
  romgen_check_oopmaps();
  initialize_non_product();
#endif

  GUARANTEE(AssemblerLoopFlags::GeneratedInterpreterLoop(),
            "Must have a real interpreter loop");

  if (GenerateROMImage) {
    initialize_standalone_rom_generator();
  }

  if (!Universe::bootstrap(_classpath)) {
    _exit_code = -1;
    return false;
  }

#if (ENABLE_COMPILER && ENABLE_PERFORMANCE_COUNTERS)
  if (TestCompiler) {
    CompilerTest::run();
    JVM::exit(0);
  }
#endif

#if !defined(PRODUCT) && ENABLE_COMPILER && ENABLE_PERFORMANCE_COUNTERS
  if (RunCompilerTests) {
    CompilerTest::run_test_cases();
    JVM::exit(0);
  }
#endif

  return true;
}

#if ENABLE_JVMPI_PROFILE 
// It's called in the profiler to get the JVMPI_Interface.
extern jint GetEnvInterface(JavaVM *vm, void **penv, jint version);
#endif

int JVM::start() {
  SETUP_ERROR_CHECKER_ARG;
  bool ok;

#if ENABLE_JVMPI_PROFILE 
  //Load the library and initialize it.
  if(Arguments::_jvmpi_profiler_lib) 
  {
    JavaVM jvm;
    JVMPIInvokeInterface_ jvmpiInvokeIF;
    jvmpiInvokeIF.GetEnv = GetEnvInterface;
    jvm.functions = &jvmpiInvokeIF;
    
    typedef int (*jvm_OnLoad_Func) (JavaVM *, char *,void *);
    jvm_OnLoad_Func _jvm_OnLoad;
  
    // load the library when add -Xrunlibname 
    void* dll_handle = Os::loadLibrary(Arguments::_jvmpi_profiler_lib );
  
    if(dll_handle) {
      // find the JVM_OnLoad function in the library
      _jvm_OnLoad = (jvm_OnLoad_Func)Os::getSymbol(dll_handle, "JVM_OnLoad");
      if(_jvm_OnLoad) {
        JVMPIProfile::VMjvmpiInit();
        // Call back function of the library. Initialize the JVMPI interface.
        int iret = _jvm_OnLoad(&jvm, NULL, NULL);
        if(iret == 0) {
          tty->print_cr("Library OnLoad Successfully\n");
          //JVMPIProfile::has_init = true;
        } else if(iret == -1) {
          tty->print_cr("Library OnLoad Failed\n");
          return -1;
        }
      } else {
        tty->print_cr("Can not find the JVM_OnLoad function in  the library: %s \n", Arguments::_jvmpi_profiler_lib);
        return -1;
      }
    } else {
      tty->print_cr("Can not find the library: %s \n", Arguments::_jvmpi_profiler_lib);
      return -1;
    }
  }
#endif

  measure_native_stack(false);

  if ((ok = initialize()) == false) {
    goto done;
  }

  ok = check_misc_options(JVM_SINGLE_ARG_NO_CHECK);
  if (!ok) {
    goto done;
  }

  if (GenerateROMImage) {
    ok = start_standalone_rom_generator(JVM_SINGLE_ARG_NO_CHECK);
  } else {
    ok = load_main_class(JVM_SINGLE_ARG_NO_CHECK);
  }

  if (!ok) {
    if (CURRENT_HAS_PENDING_EXCEPTION) {
      tty->print(MSG_UNCAUGHT_EXCEPTIONS);
      Thread::print_current_pending_exception_stack_trace();
      if (_exit_code == 0) {
        _exit_code = -1;
      }
    }
    if (Scheduler::is_slave_mode()) {
      _exit_code = -1;
    }
    goto done;
  }

#if ENABLE_JAVA_DEBUGGER
  {
    bool retval =
      JavaDebugger::initialize_java_debugger_main(JVM_SINGLE_ARG_NO_CHECK);
    if (retval == false) {
      if (_exit_code == 0) {
        _exit_code = -1;
      }
      if (Scheduler::is_slave_mode()) {
        _exit_code = -1;
      }
      goto done;
    }
    JVMSPI_DebuggerNotification((_debugger_active != 0));
  }
#endif
#if ENABLE_CPU_VARIANT && !CROSS_GENERATOR
  if (EnableCPUVariant) {
    ::initialize_cpu_variant();
  }
#endif

  if (Scheduler::is_slave_mode()) {
    _is_started = true;
    _exit_code = 0;
    goto done;
  } else {
    if (!Universe::is_stopping()) {
      // the debugger may have killed the VM, so check is_stopping first
      run();
    }
  }

  if (!Universe::is_stopping() && CURRENT_HAS_PENDING_EXCEPTION) {
    // We come to here if the VM is built incorrectly and the ROM
    // image is missing from a ROMIZING, product build -- this results
    // in an unsatisfied_link_error, and the VM overflows its stack
    // while trying to print the stack trace.
    tty->print(MSG_UNCAUGHT_EXCEPTIONS);
    Thread::print_current_pending_exception_stack_trace();
  }

done:

  if (!Scheduler::is_slave_mode()) {
    // In slave mode, we're not ready to clean up yet -- the VM is still
    // running.
    cleanup();
  }

  measure_native_stack(true);
  return JVM::_exit_code;
}

void JVM::cleanup() {
  // IMPL_NOTE: move this to the proper place.
  last_kni_handle_info = NULL;

#if ENABLE_JVMPI_PROFILE
  // Send the JVM shut down event to the profiler.
  if(UseJvmpiProfiler && JVMPIProfile::VMjvmpiEventJVMShutDownIsEnabled()) {
    JVMPIProfile::VMjvmpiPostJVMShutDownEvent();
    tty->print_cr("Jvm Shut Down Event Return!");
  }
#endif

  if (RunFinalizationAtExit) {
    ObjectHeap::finalize_all();
  }

#if ENABLE_JAVA_DEBUGGER
  {
    Transport t = Universe::transport_head();
    JavaDebugger::close_java_debugger(&t);
    if (!t.is_null()) {
      Transport::transport_op_def_t *ops = t.ops();
      if (ops != NULL) {
        ops->destroy_transport(&t);
      }
    }
  }
#endif

  dump_profile();

#if ENABLE_COMPILER
  if (PrintCompilationAtExit) {
    Compiler::print_compilation_history();
  }
#endif

#if ENABLE_ISOLATES && ENABLE_PERFORMANCE_COUNTERS
  ObjectHeap::print_max_memory_usage();
#endif

  if (VerifyGC) {
    ObjectHeap::verify();
  }

  SETUP_ERROR_CHECKER_ARG;

  JVM::development_epilogue(JVM_SINGLE_ARG_NO_CHECK);
#if ENABLE_PERFORMANCE_COUNTERS
  JVM::print_performance_counters();
#endif

  EventLogger::dump();
  EventLogger::dispose();

  Universe::apocalypse();
  _is_started = false;  
  Thread::clear_current_pending_exception();
}

bool JVM::check_misc_options(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  if (VerifyOnly) {
    _exit_code = -1;
#if ENABLE_VERIFY_ONLY
    if (Verifier::verify_classpath()) {
      _exit_code = 0;
    }
#endif
    if (Verbose) {
      TTY_TRACE_CR(("Verifier::verify_classpath = %s", 
                    (_exit_code == 0) ? "pass" : "fail"));
    }
    return false;
  }

  return true;
}

#if ENABLE_ROM_GENERATOR

jint JVM::romizer_saved_HeapMin;
jint JVM::romizer_saved_HeapCapacity;
bool JVM::romizer_saved_UseCompiler;
bool JVM::romizer_saved_MixedMode;
bool JVM::romizer_saved_UseVerifier;
bool JVM::romizer_saved_SlaveMode;

#define JVM_SAVE_ROM(x)    romizer_saved_ ## x = x
#define JVM_RESTORE_ROM(x) x = romizer_saved_ ## x

// This function sets up the global JVM parameters before the JVM is 
// bootstraped to run the (source or binary) romizer in stand-alone mode.
void JVM::initialize_standalone_rom_generator() {
#if ENABLE_CPU_VARIANT && CROSS_GENERATOR && USE_SOURCE_IMAGE_GENERATOR
  // IMPL_NOTE: In a cross-romizer, we always use forward-growing stack
  // when AOT-compiling Java bytecodes to be exected executed on
  // Jazelle hardware.
  CompilerJavaStackDirection = 1;
#endif

  JVM::check_source_generator_availability();

  // We save (and later restore) these global variables because on some
  // low-end platforms the JVM may be restarted inside the same process
  // without re-initializing the global variables.
  JVM_SAVE_ROM(HeapMin);
  JVM_SAVE_ROM(HeapCapacity);
  JVM_SAVE_ROM(UseCompiler);
  JVM_SAVE_ROM(MixedMode);
  JVM_SAVE_ROM(SlaveMode);
  JVM_SAVE_ROM(UseVerifier);

  // Cannot compile any code during romization
  UseCompiler = false;
  MixedMode = false;

  // IMPL_NOTE: Cannot use SlaveMode (??) during romization (but then how
  // can user terminate Monet conversion??)
  SlaveMode = false;

  if (!UseVerifier) {
    tty->print_cr("Warning: UseVerifier is always enabled during romization");
    UseVerifier = true;
  }

#if USE_SOURCE_IMAGE_GENERATOR
  // Some sanity checks for the source romizer, which people usually
  // get confused about.
  bool ok = true;
  tty->cr();
#if !HOST_LITTLE_ENDIAN
  if (HARDWARE_LITTLE_ENDIAN) {
    tty->print_cr("HARDWARE_LITTLE_ENDIAN must be false");
    ok = false;
  }
  if (!MSW_FIRST_FOR_LONG) {
    tty->print_cr("MSW_FIRST_FOR_LONG must be true");
    ok = false;
  }
  if (!MSW_FIRST_FOR_DOUBLE) {
    tty->print_cr("MSW_FIRST_FOR_DOUBLE must be true");
    ok = false;
  }
#else
  if (!HARDWARE_LITTLE_ENDIAN) {
    tty->print_cr("HARDWARE_LITTLE_ENDIAN must be true");
    ok = false;
  }
  if (MSW_FIRST_FOR_LONG) {
    tty->print_cr("MSW_FIRST_FOR_LONG must be false");
    ok = false;
  }
  if (MSW_FIRST_FOR_DOUBLE) {
    tty->print_cr("MSW_FIRST_FOR_DOUBLE must be false");
    ok = false;
  }
#endif

  if (!ok) {
    tty->print_cr("Please check your build/<platform>/<platform>.cfg file");
    tty->print_cr("Special settings should be made only to Loopgen");
    tty->print_cr("and Target, but not to Romgen.");
    tty->cr();
    return;
  }

#ifndef PRODUCT
  tty->cr();
  tty->print_cr("If romizer fails, increase your heap size.");
  tty->print_cr("  E.g., =HeapCapacity80M");
  tty->cr();
#endif

  if (HeapCapacity < 8 * 1024 * 1024) {
    HeapCapacity = 8 * 1024 * 1024;
  }

#endif

  HeapMin = HeapCapacity;
}

void JVM::finalize_standalone_rom_generator() {
  JVM_RESTORE_ROM(HeapMin);
  JVM_RESTORE_ROM(HeapCapacity);
  JVM_RESTORE_ROM(UseCompiler);
  JVM_RESTORE_ROM(MixedMode);
  JVM_RESTORE_ROM(SlaveMode);
  JVM_RESTORE_ROM(UseVerifier);
}

// Invoke the ROM generator in the main thread
bool JVM::start_standalone_rom_generator(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  InstanceClass::Fast klass = Universe::jvm_class()->obj();

#if USE_SOURCE_IMAGE_GENERATOR
    CurrentSourceROMWriter rom_writer;

    rom_writer.start(JVM_SINGLE_ARG_CHECK_0);

    // Run the Java method JVM.createSysImage();
    Symbol* method_name = Symbols::create_sys_image_name();

    Method::Fast method =
        klass().lookup_method(method_name, Symbols::void_signature());
    GUARANTEE(method.not_null() && method().is_static(), "sanity");

    EntryActivation::Fast entry =
        Universe::new_entry_activation(&method, 0 JVM_CHECK_0);
    Thread::current()->append_pending_entry(&entry);
#else
    Symbol* method_name = Symbols::create_app_image_name();

    Method::Fast method = klass().lookup_method(method_name,
        Symbols::char_array_char_array_int_void_signature());
    GUARANTEE(method.not_null() && method().is_static(), "sanity");

    FilePath::Fast input;
    FilePath::Fast output;
    int flags = 0;    
    flags |= RemoveConvertedClassFiles ? JVM_REMOVE_CLASSES_FROM_JAR : 0;
    flags |= GenerateSharedROMImage ? JVM_GENERATE_SHARED_IMAGE : 0;

    get_binary_romizer_args(&input, &output JVM_CHECK_0);

    // Create a delayed activation for the generateROMImage() method
    EntryActivation::Fast entry =
        Universe::new_entry_activation(&method, 3 JVM_CHECK_0);
    entry().obj_at_put(0, &input);
    entry().obj_at_put(1, &output);
    entry().int_at_put(2, flags);
    Thread::current()->append_pending_entry(&entry);
#endif
  return true;
}
#endif

#if USE_BINARY_IMAGE_GENERATOR
void JVM::get_binary_romizer_args(FilePath* input, FilePath *output JVM_TRAPS)
{
  if (_argc == 1) {
    char* arg = _argv[0];
    String str = Universe::new_string(arg, jvm_strlen(arg) JVM_CHECK);
    *input = str.value();
  } else if (_argc == 0 && Arguments::rom_input_file() != NULL) {
    const JvmPathChar *in = Arguments::rom_input_file();
    *input = FilePath::convert_to_unicode(in, fn_strlen(in) JVM_CHECK);
  } else {
    JVMSPI_DisplayUsage((char*)"input JAR file not specified");
    JVMSPI_Exit(1);
    SHOULD_NOT_REACH_HERE();
  }

  const JvmPathChar *out = Arguments::rom_output_file();
  *output= FilePath::convert_to_unicode(out, fn_strlen(out) JVM_CHECK);
}
#endif

#ifndef PRODUCT
// Initialize features that are not a part of product builds
void JVM::initialize_non_product() {
#if ENABLE_INTERPRETER_GENERATOR
  if (GenerateAssemblyCode) {
    JVM::check_source_generator_availability();
    Generator::generate();
    JVMSPI_Exit(0);
  }
#endif

#if ENABLE_INTERPRETER_GENERATOR || USE_SOURCE_IMAGE_GENERATOR
  if (GenerateOopMaps) {
    JVM::check_source_generator_availability();
    Generator::generate_oopmaps();
    JVMSPI_Exit(0);
  }
#endif

  if (::Deterministic && !AssemblerLoopFlags::Deterministic()) {
    NOT_PRODUCT(tty->print_cr("Assembler loop not set up for +Deterministic"));
    ::Deterministic = false;
  }

#if ENABLE_TTY_TRACE
  if (::TraceBytecodes && !AssemblerLoopFlags::TraceBytecodes()) {
    tty->print_cr("Assembler loop not set up for +TraceBytecodes");
    ::TraceBytecodes = false;
  }
#endif

  if (::PrintBytecodeHistogram && !AssemblerLoopFlags::PrintBytecodeHistogram()) {
    tty->print_cr("Assembler loop not set up for +PrintBytecodeHistogram");
    ::PrintBytecodeHistogram = false;
  }

  if (::PrintPairHistogram && !AssemblerLoopFlags::PrintPairHistogram()) {
    tty->print_cr("Assembler loop not set up for +PrintPairHistogram");
    ::PrintPairHistogram = false;
  }
}

void JVM::check_source_generator_availability() {
#if !ENABLE_ROM_GENERATOR && !ENABLE_INTERPRETER_GENERATOR
  tty->cr();
  tty->print_cr("Bad configuration: ENABLE_ROM_GENERATOR=0 and ENABLE_INTERPRETER_GENERATOR=0!");
  tty->print_cr("Please check your jvm.make and environment variables.");
  tty->cr();
  JVMSPI_Exit(1);
#endif  // !ENABLE_*_GENERATORS
}


#ifdef AZZERT
void JVM_ReportAssertionFailure(const char* code_str, const char* file_name,
                                int line_no, const char* message) {
#if ENABLE_VERBOSE_ASSERTION
  report_assertion_failure(code_str, file_name, line_no, message);
#else
  report_assertion_failure();
#endif
  BREAKPOINT;
}
#endif  // AZZERT

void JVM::print_parameters() {
  tty->print_cr("class\t= %s", _main_class);
  for (int index = 0; index < _argc; index++) {
    tty->print_cr("arg[%d]\t= %s", index, _argv[index]);
  }
}

void JVM::development_prologue() {
}

void JVM::development_epilogue(JVM_SINGLE_ARG_TRAPS) {
#if USE_DEBUG_PRINTING
  if (PrintBytecodeHistogram)   {
    ObjectHeap::safe_collect(0 JVM_CHECK);
    BytecodeHistogram::print(jvm_i2f(BytecodeHistogramCutOff));
  }
  if (PrintPairHistogram) {
    PairHistogram::print(jvm_i2f(PairHistogramCutOff));
  }
  if (PrintAllObjects) {
    ObjectHeap::print_all_objects();
  }
  if (PrintObjectHistogramData) {
    ObjectHeap::full_collect(JVM_SINGLE_ARG_CHECK);
    ObjectHeap::dump_histogram_data();
  }
#endif
}
#endif  // PRODUCT

// IMPL_NOTE:  cleanup if USE_UNICODE_FOR_FILENAMES
// This function is the same as fn_strlen
extern "C" int unicode_strlen(const jchar* str) {
  const jchar *p;
  int len = 0;
  for (p=str; *p; p++) {
    ++len;
  }
  return len;
}

extern "C" void JVM_Stop(int exit_code) {
  JVM::stop(exit_code);
}

extern "C" int JVM_GetConfig(int name) {
  switch (name) {
  case JVM_CONFIG_HEAP_CAPACITY:
    return HeapCapacity;
  case JVM_CONFIG_HEAP_MINIMUM:
    return HeapMin;
  case JVM_CONFIG_SLAVE_MODE:
    return SlaveMode;
  case JVM_CONFIG_USE_ROM:
    return UseROM;
#if ENABLE_JAVA_DEBUGGER
  case JVM_CONFIG_DEBUGGER_PORT:
    return DefaultDebuggerPort;
    break;
#endif
 case JVM_CONFIG_ASYNC_DATA_SIZE:
    return CachedAsyncDataSize;
  default:
    SHOULD_NOT_REACH_HERE();
    return 0;
  }
}

extern "C" void JVM_SetConfig(int name, int value) {
  switch (name) {
  case JVM_CONFIG_HEAP_CAPACITY:
    HeapCapacity = value;
    break;
  case JVM_CONFIG_HEAP_MINIMUM:
    HeapMin = value;
    break;
  case JVM_CONFIG_SLAVE_MODE:
    SlaveMode = (bool)value;
    break;
  case JVM_CONFIG_USE_ROM:
#if ENABLE_SYSTEM_ROM_OVERRIDE && defined(ROMIZING)
    UseROM = value;
#endif
    break;
#if ENABLE_JAVA_DEBUGGER
  case JVM_CONFIG_DEBUGGER_PORT:
    DefaultDebuggerPort = value;
    break;
#endif
  case JVM_CONFIG_ASYNC_DATA_SIZE:
    CachedAsyncDataSize = value;
    break;
#if ENABLE_ISOLATES
  case JVM_CONFIG_FIRST_ISOLATE_RESERVED_MEMORY:
    ReservedMemory = value;
    break;
  case JVM_CONFIG_FIRST_ISOLATE_TOTAL_MEMORY:
    TotalMemory = value;
    break;
#endif
  default:
    SHOULD_NOT_REACH_HERE();
  }
}

extern "C" jlong JVM_JavaMilliSeconds() {
  return Os::java_time_millis();
}

int JVM_CleanUp(void) {
  GUARANTEE(SlaveMode, "sanity");
  JVM::cleanup();
  return JVM::exit_code();
}

extern "C" jlong JVM_TimeSlice(void) {
  SETUP_ERROR_CHECKER_ARG;
  GUARANTEE(!Universe::is_stopping(),
            "VM was stopped but threads are scheduled");
  if (!JVM::is_started()) {
    return -2;
  }
  return Scheduler::time_slice(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

extern "C" jboolean JVM_SetUseVerifier(jboolean use_verifier) {
  jboolean res = UseVerifier;
#if ENABLE_ISOLATES
  // when VM is not active there is no current active task
  if (!Universe::before_main()) {
    if (!Task::current()->is_null()) {
      IsolateObj::Raw iso = Task::current()->primary_isolate_obj();
      if (!iso.is_null()) {
        res = iso().use_verifier();
        iso().set_use_verifier(use_verifier);
        return res;
      }
    }
  }
#endif
  UseVerifier = use_verifier;
  return res;
}

extern "C" jboolean JVM_GetUseVerifier() {
#if ENABLE_ISOLATES
  // when VM is not active there is no current active task
  if (!Universe::before_main()) {
    if (!Task::current()->is_null()) {
      IsolateObj::Raw iso = Task::current()->primary_isolate_obj();
      if (!iso.is_null()) {
        return iso().use_verifier();
      }
    }
  }
#endif
  return UseVerifier;
}

extern "C" void JVM_SetHint(int task_id, int hint, int param) {
  JVM::set_hint(task_id, hint, param);
}

void JVM::set_hint(int task_id, int hint, int param) {
  if (!EnableVMHints) {
    return;
  }
  GUARANTEE(param == 0, "must be zero in this version");
  AZZERT_ONLY_VAR(param);

#if ENABLE_COMPILER
  Compiler::set_hint(hint);
#endif

  if (hint == JVM_HINT_VISUAL_OUTPUT) {
    return;
  }

#if ENABLE_ISOLATES
  Task::Raw task = Universe::task_from_id(task_id);
  task().set_hint(hint, param);
#else
// !ENABLE_ISOLATES
  (void)task_id; // not currently used
  switch (hint) {
  case JVM_HINT_BEGIN_STARTUP_PHASE:
    _startup_phase_count++;
    break;
  case JVM_HINT_END_STARTUP_PHASE:
    _startup_phase_count--;
    GUARANTEE(_startup_phase_count >= 0, "sanity");
    break;
  default:
    SHOULD_NOT_REACH_HERE();
  }

  if (_startup_phase_count > 0) {
    Universe::set_compilation_allowed(false);
  } else {
    Universe::set_compilation_allowed(true);
  }
#endif
}

static void dump_exact_profile() {
#if ENABLE_WTK_PROFILER
  if (UseExactProfiler) {
    WTKProfiler::dump_and_clear_profile_data(-1);
  }
#endif
}

static void dump_statistic_profile() {
#if ENABLE_PROFILER
  if (UseProfiler) {
    Profiler::dump_and_clear_profile_data(-1);
  }
#endif
}

void JVM::dump_profile() {
  dump_exact_profile();
  dump_statistic_profile();
}

static int _version_id = 0;

extern "C" int JVM_GetVersionID() {
  // To guarantee uniqueness, you should call JVM_SetVersionID with an
  // auto-generated number that's guaranteed to be unique every time
  // the MIDP stack is rebuilt.

  return _version_id;
}
extern "C" void JVM_SetVersionID(int id) {
  _version_id  = id;
}

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
/*
 * Sets current classes profile.
 */
extern "C" int JVM_SetProfile(char *profile_name) {
  if (JVM::is_started()) {
    TTY_TRACE_CR(("JVM_SetProfile must be called when the VM is "
                  "not executing"));
    return -1;
  }

  const int profile_id = Universe::profile_id_by_name(profile_name);
  Universe::set_profile_id(profile_id);
  return -1;
}
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

#if ENABLE_VERIFY_ONLY
extern "C" jboolean JVM_Verify(const JvmPathChar *classpath) {
  if (!Universe::before_main()) {
    return false;
  }
  JVM_Initialize();
  VerifyOnly = true;
  bool savedSlaveMode = SlaveMode;
  SlaveMode = false;
  JVM::set_arguments(classpath, NULL, 0, NULL);
  jboolean result = (JVM::start() == 0) ? true : false;

  VerifyOnly = false;
  SlaveMode = savedSlaveMode;
  return result;
}
#endif

#if ENABLE_MONET
extern "C" jint JVM_CreateAppImage(const JvmPathChar *jarFile, 
                                   const JvmPathChar *binFile,
                                   int flags) {
  if (JVM::is_started()) {
    TTY_TRACE_CR(("JVM_CreateAppImage must be called when the VM is "
                  "not executing"));
    return -1;
  }
  JVM_Initialize();
  GenerateROMImage = true;

  Arguments::set_rom_input_file(jarFile, false);
  Arguments::set_rom_output_file(binFile, false);

  JVM::set_arguments(NULL, NULL, 0, NULL);
 
  RemoveConvertedClassFiles = (flags & JVM_REMOVE_CLASSES_FROM_JAR) != 0;
  GenerateSharedROMImage = (flags & JVM_GENERATE_SHARED_IMAGE) != 0;  

  jint code = JVM::start();

  Arguments::set_rom_input_file(NULL, false);
  Arguments::set_rom_output_file(NULL, false);

  GenerateROMImage = false;
  GenerateSharedROMImage = false;

  return code;
}
#endif // ENABLE_MONET


#if ENABLE_PERFORMANCE_COUNTERS
JVM_PerformanceCounters jvm_perf_count;

extern "C" void vm_mips_4(int);
extern "C" void vm_mips_4_end(int);
extern "C" void vm_mips_16(int);
extern "C" void vm_mips_16_end(int);
extern "C" void vm_mips_64(int);
extern "C" void vm_mips_64_end(int);

void JVM::calibrate_cpu() {

#if ENABLE_VM_MIPS && ARM && !CROSS_GENERATOR && !ENABLE_ARM_V6T2

  typedef void (*proc)(int);
  static proc const table[] = {
    &vm_mips_4,
    &vm_mips_4_end,
    &vm_mips_16,
    &vm_mips_16_end,
    &vm_mips_64,
    &vm_mips_64_end
  };

  if (PrintVmMIPS) {
    tty->print("VmMIPS.ver.A[4/16/64] =");
    const char *sep = " ";
    proc const* p = table;
    for (int kb = 4; kb <= 64; kb *= 4, p += 2) {
      // don't count the last "mov pc, lr" instruction at the end of
      // the calibration function.
      int num_instructions = (int(p[1]) - int(p[0])) / 4 - 1;
      int num_loops = 64 / kb * 256;
      if (kb != 64) {
        num_loops *= 8;
      }

      // Make sure all instructions are not cached (possibly by a previous
      // run of the VM).
      OsMisc_flush_icache((address)p[0], num_instructions * 4);

      jlong started = Os::java_time_millis();
      p[0](num_loops);
      jlong elapsed = Os::java_time_millis() - started;
      if (elapsed < 1) {
        elapsed = 1;
      }
      jlong total_instr = jlong(num_instructions) * jlong(num_loops);
      jlong mi_per_1000_sec = total_instr / elapsed;

      tty->print("%s%d.%d%d", sep, 
                 int((mi_per_1000_sec / 1000)),
                 int((mi_per_1000_sec /  100) % 10),
                 int((mi_per_1000_sec /   10) % 10));
      sep = "/";
    }
    tty->cr();
  }
#endif
}

void JVM::calibrate_hrticks() {
  jlong saved = jvm_perf_count.hrtick_read_count;
  jlong started = Os::elapsed_counter();
  for (int i=1000; i>0; i--) {
    Os::elapsed_counter();
  }
  jlong end = Os::elapsed_counter();
  jvm_perf_count.hrtick_overhead_per_1000 = end - started;
  jvm_perf_count.hrtick_read_count = saved;
}


/*
 * jvm_start_call_vm and jvm_end_call_vm are used to measure time spent inside
 * the native code of the VM (i.e., exclude time of interpretation and
 * compiled code).
 *
 * They are not enabled by default, because doing so will slow down VM
 * speed significantly. Do a global search inside src/vm/cpu to find
 * the places to enable it.
 */
extern "C" void jvm_start_call_vm() {
  jvm_perf_count.total_in_vm_hrticks -= Os::elapsed_counter();
}

extern "C" void jvm_end_call_vm() {
  jvm_perf_count.total_in_vm_hrticks += Os::elapsed_counter();
}

extern "C" void JVM_ResetPerformanceCounters() {
  JVM_PerformanceCounters *pc = &jvm_perf_count;
  jvm_memset(pc, 0, sizeof(*pc));

  pc->vm_start_hrtick = Os::elapsed_counter();
  pc->hrtick_frequency = Os::elapsed_frequency();
}

extern "C" JVM_PerformanceCounters* JVM_GetPerformanceCounters() {
  JVM_PerformanceCounters *pc = &jvm_perf_count;

  pc->mem_used          = ObjectHeap::used_memory();
  pc->mem_free          = ObjectHeap::free_memory();
  pc->mem_total         = ObjectHeap::total_memory();
  pc->num_of_threads    = Scheduler::active_count();
  pc->vm_current_hrtick = Os::elapsed_counter();

  return pc;
}

static inline jdouble msec_scale(jlong val) {
  return jvm_dmul(jvm_ddiv(jvm_l2d(val),
                           jvm_l2d(jvm_perf_count.hrtick_frequency)),
                  1000.0);
}

void JVM::print_performance_counters() {
  // conditions for printing
  const bool A = true;  // Always
  const bool O = false; // Others, printed only if PrintAllPerformanceCounters.
  const bool C = PrintCompilerPerformanceCounters;
  const bool G = PrintGCPerformanceCounters;
  const bool L = PrintLoadingPerformanceCounters;
  const bool T = PrintThreadPerformanceCounters;

  if (!(PrintPerformanceCounters || PrintAllPerformanceCounters ||
        C || G || L || T)) {
    return;
  }

  JVM_PerformanceCounters *pc = JVM_GetPerformanceCounters();
  jlong elapsed = (pc->vm_current_hrtick - pc->vm_start_hrtick);

  julong avg_compile_hrticks_xmax = 0;
  julong avg_compiled_method_xmax = 0;
  julong avg_compile_mem_xmax = 0;

  if (pc->num_of_compilations > (jlong)1) {
    avg_compile_hrticks_xmax =
        (pc->total_compile_hrticks - pc->max_compile_hrticks) /
        (pc->num_of_compilations - (jlong)1);
    avg_compiled_method_xmax =
        (pc->total_compiled_methods - pc->max_compiled_method) /
        (pc->num_of_compilations - (jlong)1);
    avg_compile_mem_xmax =
        (pc->total_compile_mem - pc->max_compile_mem) /
        (pc->num_of_compilations - (jlong)1);
  }

  tty->cr();
  JVM::calibrate_hrticks();

  jlong hrtick_read_overhead = 0;
  if (pc->hrtick_overhead_per_1000 > 0) {
    // This is total time spent reading the high-res clock during this
    // VM execution. If you see a high value that means the performance
    // counters are somewhat skewed, and you should try to make the high-res
    // clock more light-weight.
    hrtick_read_overhead = 
        pc->hrtick_read_count * pc->hrtick_overhead_per_1000 / jlong(1000);
  }
  P_LNG(A, "hrtick_frequency",         pc->hrtick_frequency);
  P_HRT(A, "hrtick_overhead_per_1000", pc->hrtick_overhead_per_1000);
  P_LNG(A, "hrtick_read_count",        pc->hrtick_read_count);
  P_HRT(A, "hrtick_read_overhead",     hrtick_read_overhead);

  P_HRT(A, "elapsed", elapsed);

  // Thread counters
  //
  P_INT(T, "num_of_threads at exit",pc->num_of_threads);
  P_INT(A, "num_of_timer_ticks",    pc->num_of_timer_ticks, "%9d");
  if (pc->num_of_timer_ticks > 0) {
    jdouble ms_per_tick = jvm_ddiv(msec_scale(elapsed),
                                   jvm_i2d(pc->num_of_timer_ticks));
    tty->print_cr(" or %.2lf ms per tick", ms_per_tick);
  } else {
    tty->cr();
  }
  P_HRT(T, "total_event_hrticks",   pc->total_event_hrticks);
  P_INT(T, "total_event_checks",    (int)pc->total_event_checks);

  // GC counters
  //
  P_INT(G, "mem_total",             pc->mem_total);
  P_INT(G, "mem_free",              pc->mem_free);
  P_INT(G, "mem_used",              pc->mem_used);
  P_INT(G, "total allocated bytes", pc->mem_used + pc->total_bytes_collected);
  P_INT(A, "num_of_gc",             pc->num_of_gc);
  P_INT(G, "num_of_full_gc",        pc->num_of_full_gc);
  P_INT(G, "num_of_compiler_gc",    pc->num_of_compiler_gc);
  P_INT(G, "total_bytes_collected", pc->total_bytes_collected);

  P_HRT(A, "total_gc_hrticks",      pc->total_gc_hrticks);
  P_HRT(G, "max_gc_hrticks",        pc->max_gc_hrticks);
  P_CR (G);

  // Other counters
  //
  P_HRT(O, "total_in_vm_count",    pc->total_in_vm_hrticks);
  P_CR (O);

#if USE_BINARY_IMAGE_LOADER
  P_HRT(L, "binary_link_hrticks",  pc->binary_link_hrticks);
  P_HRT(L, "binary_load_hrticks",  pc->binary_load_hrticks);
#endif

  P_INT(L, "num_of_class_loaded",  pc->num_of_class_loaded);
  P_HRT(A, "total_load_hrticks",   pc->total_load_hrticks);
  P_HRT(L, "max_load_hrticks",     pc->max_load_hrticks);
  P_HRT(A, "total_verify_hrticks", pc->total_verify_hrticks);
  P_HRT(L, "max_verify_hrticks",   pc->max_verify_hrticks);

  P_INT(L, "num_of_romizer_steps", pc->num_of_romizer_steps);
  P_HRT(L, "total_romizer_hrticks",pc->total_romizer_hrticks);
  P_HRT(L, "max_romizer_hrticks",  pc->max_romizer_hrticks);
  P_CR (L);

  P_INT(C, "num_of_compilations",      pc->num_of_compilations);
  P_INT(C, "           finished",      pc->num_of_compilations_finished);
  P_INT(C, "       resume count",      pc->compilation_resume_count);
  P_INT(C, "             failed",      pc->num_of_compilations_failed);
  P_INT(C, "total_compiled_bytecodes", (int)pc->total_compiled_bytecodes);
  P_INT(C, "max_compiled_bytecodes",   (int)pc->max_compiled_bytecodes);
  P_HRT(A, "compile total",            pc->total_compile_hrticks);
  P_HRT(C, "        max",              pc->max_compile_hrticks);
  P_HRT(C, "        avg (excl max)",   avg_compile_hrticks_xmax);

  P_INT(C, "compile_mem total",        (int)pc->total_compile_mem);
  P_INT(C, "        max",              (int)pc->max_compile_mem, "%9d");
  if (C || PrintAllPerformanceCounters) {
    tty->print_cr(" avg (excl max) = %d", avg_compile_mem_xmax);
  }

  P_INT(C, "compiled_methods total",   (int)pc->total_compiled_methods);
  P_INT(C, "        max",              (int)pc->max_compiled_method, "%9d");
  if (C || PrintAllPerformanceCounters) {
    tty->print_cr(" avg (excl max) = %d", avg_compiled_method_xmax);
  }

  P_INT(C, "uncommon_traps_generated", pc->uncommon_traps_generated);
  P_INT(C, "uncommon_traps_taken",     pc->uncommon_traps_taken);
  P_CR (C);

  if (UseROM) {
    tty->cr();
    ROM::ROM_print_hrticks(print_hrticks);
  }

#if ENABLE_COMPILER
  if (C) {
    Compiler::print_detailed_performance_counters();
  }
#endif

#undef PRINT_INT_COUNTER
#undef PRINT_HRTICKS
}

void JVM::P_INT(bool cond, const char *name, int value, const char *fmt) {
  if (fmt == NULL) {
    fmt = "%9d\n";
  }
  if (cond || PrintAllPerformanceCounters) {
    tty->print("%-25s = ", name);
    tty->print(fmt, value);
  }
}

void JVM::P_LNG(bool cond, const char *name, julong value) {
  if (cond || PrintAllPerformanceCounters) {
    tty->print("%-25s = ", name);
    tty->print(JVM_LLD, value);
    tty->cr();
  }
}

void JVM::P_HRT(bool cond, const char *name, julong hrticks) {
  if (cond || PrintAllPerformanceCounters) {
    print_hrticks(name, hrticks);
  }
}
void JVM::P_CR(bool cond) {
  if (cond || PrintAllPerformanceCounters) {
    tty->cr();
  }
}

void JVM::print_hrticks(const char *name, julong hrticks) {
 tty->print_cr("%-25s = "JVM_LLD" hrticks or %.2lf msec",
            name, hrticks,  msec_scale(hrticks));
}
#endif  // ENABLE_PERFORMANCE_COUNTERS

#if ENABLE_MEASURE_NATIVE_STACK
static char *stackStart;

void JVM::measure_native_stack(bool measure) {
  char stack[10 * 1024];
  char tag = (char)0xef;
  int size = sizeof(stack);

  if (!measure) {
    stackStart = stack;
    for (int i = 0; i < size; i++) {
      stack[i] = tag;
    }
  } else {
    char *marker = stackStart;
    int i = 0;
    while (*marker == tag) {
      i++;
      marker++;
    }
    tty->print_cr("Max Native Stack Size %d\n", size - i);
  }
}

#endif // ENABLE_MEASURE_NATIVE_STACK
