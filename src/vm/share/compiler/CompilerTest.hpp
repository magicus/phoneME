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

#if (ENABLE_COMPILER && ENABLE_PERFORMANCE_COUNTERS)

// CompilerTest is used to test abd benchmark the compiler. You can use
// it to compile all methods in a JAR file, and (optionally) all the 
// methods in the system classes.

class CompilerTest: public StackObj {
public:
  static void run();

#ifndef PRODUCT
  // Read and execute test cases from the compiler test config file.
  static void run_test_cases();

  // Helper routines.

  // Reads at most length-1 chars from the given file.
  // Stops after EOF or a newline.
  // A '\0' is stored after the last char in the buffer.
  // Returns the length of a resulting string including terminating '\0'.
  static int read_line(OsFile_Handle file, char buffer[], size_t length);

  // Given an array of keywords, a number of keywords in the array, and a
  // string, returns an index of the first keyword that matches the string
  // or -1 if no keyword matches.
  static int get_keyword_index(const char * const keywords[],
                               const int keywords_count,
                               const char * const string);
#endif
private:
  static void run_tests(JVM_SINGLE_ARG_TRAPS);
  static ReturnOop load_and_sort_classes(JVM_SINGLE_ARG_TRAPS);
  static ReturnOop sort_methods(InstanceClass *klass JVM_TRAPS);
  static void test_compile(Method *method JVM_TRAPS);
  static void print_summary();
  static bool may_be_initialized(InstanceClass *klass);

  static jint compare(Method *m1, Method *m2);
  static jint compare(InstanceClass *c1, InstanceClass *c2);

  static jint _total_num_compiled;
  static jint _total_us;
  static jint _total_objs;
  static jint _total_mem;
  static jint _total_bytecode_size;
  static jint _total_compiled_size;
};

#endif // (ENABLE_COMPILER && ENABLE_PERFORMANCE_COUNTERS)
