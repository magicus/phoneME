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

/** \class JVM.hpp
 * VM startup and shutdown routines.
 *
 * This file defines the interface for virtual
 * machine startup and shutdown.  The implementation
 * of many of these functions may vary from one target
 * platform or operating system to another.  The platform-
 * specific implementations of these functions should be
 * placed in the OS-specific
 * "/src/vm/os/<os_name>/JVM_<os_name>.cpp" files.
 *
 */

class JVM: public AllStatic {
  static char *  _main_class;
  static const JvmPathChar*  _classpath;
  static int     _argc;
  static char ** _argv;
  static jchar** _u_argv;
  static bool    _is_started;   // Fully initialized
  static int     _exit_code;
  static int     _startup_phase_count;

 public:
  static int start();
  static void stop(int code);
  static void exit(int code);
  static void set_exit_code(int code) {
    _exit_code = code;
  }
  static ReturnOop resolve_class(char* class_name JVM_TRAPS);
  static void check_class_path(char **class_path);

  static void print_parameters() PRODUCT_RETURN;
  static void print_hrticks(const char *name, julong hrticks);
  static void P_LNG(bool cond, const char *name, julong value);
  static void P_INT(bool cond, const char *name, int value, 
                    const char *fmt=NULL);
  static void P_HRT(bool cond, const char *name, julong hrticks);
  static void P_CR(bool cond);

  static void development_prologue() PRODUCT_RETURN;
  static void development_epilogue(JVM_SINGLE_ARG_TRAPS) PRODUCT_RETURN;
  static void set_arguments(const JvmPathChar *classpath, char *main_class,
                            int argc, char **argv);
  static void set_arguments2(const JvmPathChar *classpath, char *main_class,
                             int argc, char **argv, jchar **u_argv,
                             bool is_unicode=true);
  static ReturnOop get_main_args(JVM_SINGLE_ARG_TRAPS);
  static void set_hint(int task_id, int hint, int param);
  static const JvmPathChar* classpath() {
    return _classpath;
  }
  static int exit_code() {
    return _exit_code;
  }
  static bool is_started() {
    return _is_started;
  }
  static void cleanup();

#if ENABLE_ROM_GENERATOR
  static void initialize_standalone_rom_generator();
  static bool start_standalone_rom_generator(JVM_SINGLE_ARG_TRAPS);
  static void finalize_standalone_rom_generator();
  static void get_binary_romizer_args(FilePath* input, FilePath *output
                                      JVM_TRAPS);
  static jint romizer_saved_HeapMin;
  static jint romizer_saved_HeapCapacity;
  static bool romizer_saved_UseCompiler;
  static bool romizer_saved_MixedMode;
  static bool romizer_saved_UseVerifier;
  static bool romizer_saved_SlaveMode;
#else
  static void initialize_standalone_rom_generator() {}
  static bool start_standalone_rom_generator(JVM_SINGLE_ARG_TRAPS) {
    JVM_IGNORE_TRAPS;
    return 0;
  }
  static void finalize_standalone_rom_generator() {}
#endif

#if ENABLE_INTERPRETER_GENERATOR
  static void generate_assembly_code();
#else
  static void generate_assembly_code() {}
#endif

private:
  static bool load_main_class(JVM_SINGLE_ARG_TRAPS);
  static bool initialize();
  static void run();
  static void check_source_generator_availability() PRODUCT_RETURN;

  static bool check_misc_options(JVM_SINGLE_ARG_TRAPS);
#ifndef PRODUCT
  // Initializes things that are not a part of product builds
  static void initialize_non_product();
#endif

  static void calibrate_hrticks() PRODUCT_NOCOUNTER_RETURN;
  static void calibrate_cpu() PRODUCT_NOCOUNTER_RETURN;
  static void print_performance_counters() PRODUCT_NOCOUNTER_RETURN;

#if ENABLE_MEASURE_NATIVE_STACK
  static void measure_native_stack(bool measure);
#else
  static void measure_native_stack(bool /*measure*/) {}
#endif

  static void dump_profile();

  // We put this in solely for the purpose
  // so that one can look at the executable binary
  // with a text browser and look at the copyright notice:
  static const char *copyright;
};
