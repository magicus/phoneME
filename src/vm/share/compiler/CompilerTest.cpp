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
#include "incls/_CompilerTest.cpp.incl"

/*
 * CompilerTest is used to test and benchmark the compiler. You can use
 * it to compile all methods in a JAR file, and (optionally) all the 
 * methods in the system classes.
 *
 * The most obvious way is to use a target VM (non product mode, or
 * product mode with ENABLE_PERFORMANCE_COUNTERS):
 *     cldc_vm_r -cp eembcBM.jar +TestCompiler +TestCompilerStats
 *
 * To see a listing of all methods compiled from the JAR file (for
 * performance analysis or debugging the JIT):
 *     cldc_vm_r -cp eembcBM.jar +TestCompiler +PrintCompiledCodeAsYouGo \
 *         +GenerateCompilerComments
 *
 * To test compile a specific method without running the application:
 *     cldc_vm_r -cp eembcBM.jar +TestCompiler +PrintCompiledCodeAsYouGo \
 *         +GenerateCompilerComments CompileOnlyMethod=isLeaf
 *
 * All of the above examples are running on the target device, which may
 * be slow or resource constraint. Alternatively, you can also test
 * the compiler on the host, using an AOT-enabled romgen. Example:
 *     cd ${JVMWorkSpace}/build/linux_arm
 *     make romgen
 *     romgen/app/romgen -cp ../classes.zip:eembcBM.jar +TestCompiler
 *
 * The above command loads both classes.zip and eembcBM.jar, and compiles only
 * the classes inside eembcBM.jar. To also compile the system classes in
 * classes.zip, do this:
 *     romgen/app/romgen -cp ../classes.zip:eembcBM.jar +TestCompiler \
 *         +TestCompileSystemClasses
 */

#if (ENABLE_COMPILER && ENABLE_PERFORMANCE_COUNTERS)

jint CompilerTest::_total_us;
jint CompilerTest::_total_objs;
jint CompilerTest::_total_mem;
jint CompilerTest::_total_bytecode_size;
jint CompilerTest::_total_compiled_size;
jint CompilerTest::_total_num_compiled;

void CompilerTest::run() {
  _total_num_compiled = 0;
  _total_us = 0;
  _total_objs = 0;
  _total_mem = 0;
  _total_bytecode_size = 0;
  _total_compiled_size = 0;

  SETUP_ERROR_CHECKER_ARG;
  run_tests(JVM_SINGLE_ARG_NO_CHECK);

  if (CURRENT_HAS_PENDING_EXCEPTION) {
    Thread::clear_current_pending_exception();
  }
}

void CompilerTest::run_tests(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast classes;
  ObjArray::Fast methods;
  InstanceClass::Fast klass;
  Method::Fast m;

  Os::suspend_profiler();
  {
    classes = load_and_sort_classes(JVM_SINGLE_ARG_CHECK);
  }
  Os::resume_profiler();

  int num_classes = classes().length();
  for (int i=0; i<num_classes; i++) {
    klass = classes().obj_at(i);

    Os::suspend_profiler();
    {
      methods = sort_methods(&klass JVM_CHECK);
    }
    Os::resume_profiler();

    if (Verbose) {
      tty->print("Compiling class: ");
      klass().print_name_on(tty);
      tty->cr();
    }

    int num_methods = methods().length();
    for (int j=0; j<num_methods; j++) {
      m = methods().obj_at(j);
      if (m.not_null()
          && !m().is_impossible_to_compile() && !m().is_abstract()) {
        test_compile(&m JVM_CHECK);
      }
    }
  }

  Os::suspend_profiler();
  {
    print_summary();
  }
  Os::resume_profiler();
}

ReturnOop CompilerTest::load_and_sort_classes(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast classes;
  JavaClass::Fast klass;
  int num_system_classes = 0;

  ObjArray::Fast classpath = Task::current()->app_classpath();
  for (int index = 0; index < classpath().length(); index++) {
    FilePath path = classpath().obj_at(index);
    Universe::load_all_in_classpath_segment(&path JVM_CHECK_0);
    if (!UseROM && num_system_classes == 0) {
      // If the system classes are not romized, we consider the first
      // segment of the classpath to contain the system classes. So do this:
      //    -classpath classes.zip:mytest.zip
      // instead of this:
      //    -classpath mytest.zip:classes.zip
      num_system_classes = Universe::number_of_java_classes();
    }
  }

  int num = 0;
  int added = 0;

  // (1) Find all the classes in the classpath (and optionally the system
  //     classes as well).
  for (int pass=0; pass<2; pass++) {
    for (int i=0; i< Universe::number_of_java_classes(); i++) {
      klass = Universe::class_from_id(i);
      if (klass().is_array_class() || klass().is_fake_class()) {
        continue;
      }
      if (!TestCompileSystemClasses) {
        if (klass().is_preloaded()) {
          continue;
        }
        if (!UseROM && i < num_system_classes) {
          continue;
        }
      }
      if (pass == 0) {
        num ++;
      } else {
        classes().obj_at_put(added++, &klass);
      }
    }

    if (pass == 0) {
      classes = Universe::new_obj_array(num JVM_CHECK_0);
    }
  }

  // (2) Sort the list
  int i, j;
  for (i=0; i<num; i++) {
    for (j=i; j<num; j++) {
      InstanceClass::Raw ci = classes().obj_at(i);
      InstanceClass::Raw cj = classes().obj_at(j);
      if (compare(&ci, &cj) > 0) {
        classes().obj_at_put(i, &cj);
        classes().obj_at_put(j, &ci);
      }
    }
  }

#if 0
  // (3) Set all classes to be initialized, so that we won't generate any
  //     uncommon traps. This yields stable results and is close to what
  //     happens in real execution.
  for (i=0; i<num; i++) {
    UsingFastOops fast_oops_inside;
    InstanceClass::Fast ic = classes().obj_at(i);
    if (!ic().is_romized()) {
      ic().set_initialized();
    }
  }
#else
  // (3) Initialize all classes, so that we won't generate any
  //     uncommon traps. This yields stable results and is close to what
  //     happens in real execution.

  for (;;) {
    bool made_progress = false;
    for (i=0; i< Universe::number_of_java_classes(); i++) {
      klass = Universe::class_from_id(i);
      if (klass().is_array_class() || klass().is_fake_class()) {
        continue;
      }
      InstanceClass ic = klass.obj();
      if (may_be_initialized(&ic)) {
        made_progress = true;
        ic.initialize(JVM_SINGLE_ARG_CHECK_0);
      }
      Universe::invoke_pending_entries(JVM_SINGLE_ARG_CHECK_0);
    }

    if (!made_progress) {
      // We have initialized all classes that may be initialized
      break;
    }
  }
#endif

  return classes;
}

bool CompilerTest::may_be_initialized(InstanceClass *klass) {
 if (klass->is_initialized()) {
    return false;
  }

  // All super classes and super interfaces must be initialized
  InstanceClass ic;
  for (ic = klass->obj(); !ic.is_null(); ic = ic.super()) {
    if (!ic.equals(klass) && !ic.is_initialized()) {
      return false;
    }

    TypeArray interfaces = ic.local_interfaces();
    int n_interfaces = interfaces.length();

    for (int i = 0; i < n_interfaces; i++) {
      int intf_id = interfaces.ushort_at(i);
      InstanceClass intf = Universe::class_from_id(intf_id);
      if (!intf.is_initialized()) {
        return false;
      }
    }
  }

  return true;
}

ReturnOop CompilerTest::sort_methods(InstanceClass *klass JVM_TRAPS) {
  int i, j, len;
  UsingFastOops fast_oops;
  ObjArray::Fast orig_methods = klass->methods();
  len = orig_methods().length();
  ObjArray::Raw sorted_methods = Universe::new_obj_array(len JVM_CHECK_0);

  for (i=0; i<len; i++) {
    Method::Raw m = orig_methods().obj_at(i);
    sorted_methods().obj_at_put(i, &m);
  }

  for (i=0; i<len; i++) {
    for (j=i; j<len; j++) {
      Method::Raw mi = sorted_methods().obj_at(i);
      Method::Raw mj = sorted_methods().obj_at(j);
      if (mi.is_null() || mj.is_null()) {
        continue;
      }
      if (compare(&mi, &mj) > 0) {
        sorted_methods().obj_at_put(i, &mj);
        sorted_methods().obj_at_put(j, &mi);
      }
    }
  }

  return sorted_methods;
}

jint CompilerTest::compare(Method *m1, Method *m2) {
  Symbol::Raw name1 = m1->name();
  Symbol::Raw name2 = m2->name();
  jint result = Symbol::compare_to(&name1, &name2);
  if (result != 0) {
    return result;
  }

  Symbol::Raw sig1 = m1->signature();
  Symbol::Raw sig2 = m2->signature();

  return Symbol::compare_to(&sig1, &sig2);
}

jint CompilerTest::compare(InstanceClass *c1, InstanceClass *c2) {
  Symbol::Raw name1 = c1->name();
  Symbol::Raw name2 = c2->name();
  return Symbol::compare_to(&name1, &name2);
}

void CompilerTest::test_compile(Method *method JVM_TRAPS) {
  UsingFastOops fast_oops;
  CompiledMethod::Fast cm;


#ifndef PRODUCT
  if (Arguments::must_check_CompileOnly()) {
    UsingFastOops internal;
    Symbol::Fast methodName = method->name();
    JavaClass::Fast jc = method->holder();
    Symbol::Fast className = jc().name();
    bool is_match = Arguments::must_compile_method(&className, &methodName
                                                   JVM_MUST_SUCCEED);
    if (!is_match) {
      return;
    }
  }
#endif

  JVM_PerformanceCounters *pc = JVM_GetPerformanceCounters();
  if (CollectBeforeCompilerTest) {
    Os::suspend_profiler();
    {
      int n = (HeapCapacity-1) & (~0x03);
      ObjectHeap::allocate(n JVM_NO_CHECK);
      Thread::clear_current_pending_exception();
    }
    Os::resume_profiler();
  }

  // Gather stats before method is compiled. Sorry, there's some duplicated
  // code with Compiler::allocate_and_compile(), but this code is hard to
  // share with having a clunky API.
  jlong start_time = Os::elapsed_counter();
  int mem_before = ObjectHeap::used_memory() + pc->total_bytes_collected;
  int objs_before = pc->num_of_c_alloc_objs;

  bool status = method->compile(0, false JVM_CHECK);
  while (Compiler::is_suspended()) {
    status = method->compile(0, true JVM_MUST_SUCCEED);
  }
  if (status) {
    cm = method->compiled_code();
  }

#ifndef PRODUCT
  //ObjectHeap::print_all_objects();
#endif

  Os::suspend_profiler();
  {
    jlong elapsed = Os::elapsed_counter() - start_time;
    int elapsed_us = (int)(elapsed * 1000 * 1000 / Os::elapsed_frequency());
    int mem_after = ObjectHeap::used_memory() + pc->total_bytes_collected;
    int mem_used = mem_after - mem_before;
    int objs_alloced = pc->num_of_c_alloc_objs - objs_before;
    int bytecode_size = method->code_size();
    int compiled_size = 0;
    double code_bloat = jvm_i2d(0);
    double tmp_obj_ratio = jvm_i2d(0);
    if (cm().not_null()) {
      compiled_size = cm().object_size();
      if (bytecode_size > 0) {
        code_bloat = jvm_ddiv(jvm_i2d(compiled_size), jvm_i2d(bytecode_size));
        int tmp_obj_size = mem_used - bytecode_size * CompiledCodeFactor;
        tmp_obj_ratio = jvm_ddiv(jvm_i2d(tmp_obj_size),
                                 jvm_i2d(bytecode_size));
      }
    }

    if (TestCompilerStats) {
      tty->print("%9d ", elapsed_us);
      tty->print("%6d ", objs_alloced);
      tty->print("%7d ", mem_used);
      tty->print("%6d ", compiled_size);
      tty->print("%5d ", bytecode_size);
      tty->print("%5.2f ", code_bloat);
      tty->print("%5.1f ", tmp_obj_ratio);

#ifndef PRODUCT
      method->print_name_on(tty);
#else
      {
        InstanceClass::Raw ic = method->holder();
        Symbol::Raw class_name = ic().name();
        class_name().print_symbol_on(tty, true);
        tty->print(".");
        Symbol::Raw name = method->name();
        name().print_symbol_on(tty);
      }
#endif
      tty->cr();

      if (cm().not_null()) {
        _total_num_compiled ++;
        _total_us += elapsed_us;
        _total_objs += objs_alloced;
        _total_mem += mem_used;
        _total_bytecode_size += bytecode_size;
        _total_compiled_size += compiled_size;
      }
    }
  }
  Os::resume_profiler();
}

void CompilerTest::print_summary() {
  double code_bloat = jvm_i2d(0);
  double tmp_obj_ratio = jvm_i2d(0);
  if (_total_bytecode_size > 0) {
    code_bloat = jvm_ddiv(jvm_i2d(_total_compiled_size), 
                          jvm_i2d(_total_bytecode_size));

    int tmp_obj_size = _total_mem - _total_bytecode_size * CompiledCodeFactor;
    tmp_obj_ratio = jvm_ddiv(jvm_i2d(tmp_obj_size),
                              jvm_i2d(_total_bytecode_size));

  }
  double bytecodes_per_second = jvm_i2d(0);
  if (_total_us > 0) {
    double secs = jvm_ddiv(jvm_i2d(_total_us), jvm_i2d(1000000));
    bytecodes_per_second = jvm_ddiv(jvm_i2d(_total_bytecode_size), secs);
  }

  if (TestCompilerStats) {
    tty->print("%9d ", _total_us);
    tty->print("%6d ", _total_objs);
    tty->print("%7d ", _total_mem);
    tty->print("%6d ", _total_compiled_size);
    tty->print("%5d ", _total_bytecode_size);
    tty->print("%5.2f ", code_bloat);
    tty->print("%5.1f", tmp_obj_ratio);
    tty->cr();
    tty->cr();
    tty->print_cr("%9d    methods successfully compiled", _total_num_compiled);
    tty->print_cr("%12.2f bytes of Java bytecodes/second", 
                  bytecodes_per_second);
  }
}

#ifndef PRODUCT

#define FOR_ALL_COMPILER_TEST_SUITES(template)        \
  template(VSF_MERGE_SUITE)                                 

void CompilerTest::run_test_cases() {
#if ENABLE_VSF_MERGE_TEST
  const JvmPathChar * const config_file_name = 
    Arguments::compiler_test_config_file();
  if (config_file_name == NULL) {
    tty->print_cr("Compiler test config file not specified.");
    return;
  }

  OsFile_Handle config_file = OsFile_open(config_file_name, "rt");
  if (config_file == NULL) {
    tty->print_cr("Cannot open compiler test config file %s.", 
                  config_file_name);
    return;
  }

  enum {
    BUFFER_SIZE = 256
  };

  char read_buffer[BUFFER_SIZE];

  while (true) {
    const int bytes_read = 
      CompilerTest::read_line(config_file, read_buffer, BUFFER_SIZE);
    if (bytes_read == 0) {
      // End-of-file.
      break;
    }

    if (bytes_read == BUFFER_SIZE) {
      tty->print_cr("Line too long");
      break;
    }

    if (read_buffer[0] == '#') {
      continue;
    }

    char buffer[BUFFER_SIZE];
    // Read the first word to the buffer.
    if (jvm_sscanf(read_buffer,  "%255s", buffer) != 1) {
      continue;
    }

#define DEFINE_DESCRIPTOR(name) XSTR(name),
     
    // Descriptor cannot contain a white-space.
    static const char * const suite_descriptors[] = {
      FOR_ALL_COMPILER_TEST_SUITES(DEFINE_DESCRIPTOR)
    };

#undef DEFINE_DESCRIPTOR

#define SUITE_TYPE(name) name ## _TYPE,

    enum {
      FOR_ALL_COMPILER_TEST_SUITES(SUITE_TYPE)
      INVALID_SUITE_TYPE = -1
    };

#undef DEFINE_SUITE_TYPE

    int index = get_keyword_index(suite_descriptors, 
                                  ARRAY_SIZE(suite_descriptors),
                                  buffer);

    if (index == -1) {
      tty->print_cr("Unknown suite descriptor %s", buffer);
      break;
    }

    switch (index) {
    case VSF_MERGE_SUITE_TYPE:
      VSFMergeTest::run(config_file);
      break;
    default:
      SHOULD_NOT_REACH_HERE();
    }
  }

  OsFile_close(config_file);
#endif
}

int CompilerTest::read_line(OsFile_Handle file, char buffer[], size_t length) {
  const int max = length - 1;
  char *p = buffer;
  char c;
  while ((p - buffer) < max && 
         OsFile_read(file, &c, 1, 1) == 1 && c != '\n') {
    if (c != '\r') {
      *p = c;
      p++;
    }
  }

  *p = '\0';

  return p - buffer;
}

int CompilerTest::get_keyword_index(const char * const keywords[],
                                    const int keywords_count,
                                    const char * const string) {
  for (int index = 0; index < keywords_count; index++) {
    if (jvm_strcmp(keywords[index], string) == 0) {
      return index;
    }
  }

  return -1;
}
#endif // !PRODUCT

#endif // (ENABLE_COMPILER && ENABLE_PERFORMANCE_COUNTERS)
