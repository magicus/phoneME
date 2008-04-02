/*
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
 * Globals.cpp: This file implements the parsing mechanisms
 * for execution options & command line switches.
 *
 * This VM system has a special mechanism that allows
 * compilation flags and command line switches to be treated
 * uniformly.  Basically, every compilation flag or option
 * in this VM system can be activated/deactivated also
 * from the command line using the following syntax:
 *
 * cldc_vm +<option_name>        <class_name> ; Turns <option_name> on
 * cldc_vm -<option_name>        <class_name> ; Turns <option_name> off
 * cldc_vm =<option_name><value> <class_name> ; Sets  <option_name> to <value>
 *
 * In general, the "+" or "-" syntax is used for turning
 * boolean options on or off, while the "=" syntax is used
 * for changing the value of those options that take a
 * numeric argument.
 *
 * Examples:
 *     cldc_vm +UseCodeFlushing      (Turn code flushing on)
 *     cldc_vm -UseProfiler          (Turn profiler off)
 *     cldc_vm =CompiledCodeFactor15 (Set compiled code factor to 15)
 *
 * Note that most compilation options are available only
 * in non-product builds of the system. Those flags that
 * have been marked as "develop" flags are visible only
 * during development, while the "product" flags are
 * available also in product builds.
 *
 * For a detailed description and categorization of the
 * various compilation flags, refer to the Porting Guide.
 */

# include "incls/_precompiled.incl"
# include "incls/_Globals.cpp.incl"

RUNTIME_FLAGS(DEFINE_DEVELOPER_FLAG,DEFINE_PRODUCT_FLAG,DEFINE_ALWAYS_FLAG)

struct JVMFlag {
  const char *type;
  const char *name;
  const void *addr;
#if USE_DEBUG_PRINTING
  const char *kind;
#endif

  bool is_bool() const        { return jvm_strcmp(type, "bool") == 0; }
  int get_bool() const        { return *((bool*) addr); }
  void set_bool(bool value) const   { *((bool*) addr) = value; }

  bool is_int() const         { return jvm_strcmp(type, "int")  == 0; }
  int get_int() const         { return *((int*) addr); }
  void set_int(int value) const { *((int*) addr) = value; }

#if USE_DEBUG_PRINTING
  void print_on(Stream* st) const {
    st->print("%-6s %-40s = ", type, name);
    if (is_bool()) {
      st->print("%-10s", get_bool() ? "true" : "false");
    }
    else if (is_int()) {
      st->print("%-10ld", get_int());
    }
    st->print(" %s", kind);
    st->cr();
  }
#endif
};

#ifndef PRODUCT
#if USE_DEBUG_PRINTING
#define JVM_FLAG_KIND(x) , x
#else
#define JVM_FLAG_KIND(x)
#endif

#define RUNTIME_PRODUCT_FLAG_STRUCT(type, name, value, doc) \
    { #type, XSTR(name), &name JVM_FLAG_KIND("(product)") },

#define RUNTIME_DEVELOP_FLAG_STRUCT(type, name, value, doc) \
    { #type, XSTR(name), &name JVM_FLAG_KIND("") },

#define RUNTIME_ALWAYS_FLAG_STRUCT(type, name, value, doc) \
    { #type, XSTR(name), &name JVM_FLAG_KIND("(always)") },

#else

#define RUNTIME_PRODUCT_FLAG_STRUCT(type, name, value, doc) \
    { #type, XSTR(name), &name },

#define RUNTIME_DEVELOP_FLAG_STRUCT(type, name, value, doc) // do nothing

#define RUNTIME_ALWAYS_FLAG_STRUCT(type, name, value, doc) // do nothing

#endif

PRODUCT_CONST static JVMFlag flagTable[] = {
 RUNTIME_FLAGS(RUNTIME_DEVELOP_FLAG_STRUCT,
               RUNTIME_PRODUCT_FLAG_STRUCT,
               RUNTIME_ALWAYS_FLAG_STRUCT)
 {0, NULL, NULL}
};

PRODUCT_CONST JVMFlag* Globals::find_flag(char* name) {
  PRODUCT_CONST JVMFlag* flag;

  for (flag = &flagTable[0]; flag->name; flag++) {
    if (jvm_strcmp(flag->name, name) == 0) {
      return flag;
    }
  }
  return NULL;
}

#ifndef PRODUCT
bool Globals::int_value(char* name, int& value) {
  PRODUCT_CONST JVMFlag* result = find_flag(name);
  if (result == NULL || !result->is_int()) {
    return false;
  }
  value = result->get_int();
  return true;
}
#endif

bool Globals::int_value_put(char* name, int value) {
  PRODUCT_CONST JVMFlag* result = find_flag(name);
  if (result == NULL || !result->is_int()) {
    return false;
  }
  result->set_int(value);
  return true;
}

#ifndef PRODUCT
bool Globals::bool_value(char* name, bool& value) {
  PRODUCT_CONST JVMFlag* result = find_flag(name);
  if (result == NULL || !result->is_bool()) {
    return false;
  }
  value = result->get_bool();
  return true;
}
#endif

bool Globals::bool_value_put(char* name, bool value) {
  PRODUCT_CONST JVMFlag* result = find_flag(name);
  if (result == NULL) {
    return false;
  } else {
    GUARANTEE(result->is_bool() || result->is_int(), 
              "only int and bool types are allowed");
    if (sizeof(bool) == sizeof(int)) {
      result->set_int((int)value);
    } else if (result->is_bool()) {
      result->set_bool(value);
    } else if (result->is_int()) {
      result->set_int((int)value);
    }
    return true;
  }
}

// Parse a size specification string.

static bool parse_size(char* s, jint& result) {
  jint n = 0;
  int args_read = jvm_sscanf(s, "%d", &n);
  if (args_read != 1) return false;
  if (*s == '-') s++;
  while (*s != '\0' && isdigit(*s)) s++;
  switch (*s) {
    case 'M': case 'm':
      result = n * 1024 * 1024;
      return true;
    case 'K': case 'k':
      result = n * 1024;
      return true;
    case '\0':
      result = n;
      return true;
    default:
      return false;
  }
}

bool Globals::set_numeric_flag(char* name, char* value) {
  jint v;
  if (!parse_size(value, v)) return false;
  return int_value_put(name, v);
}

bool Globals::parse_argument(char* arg) {
  // range of acceptable characters spelled out for portability reasons
  char name[256];
  #define NAME_RANGE  "[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ]"
  if (jvm_sscanf(arg, "-%" NAME_RANGE, name) == 1) {
    return bool_value_put(name, false);
  }
  if (jvm_sscanf(arg, "+%" NAME_RANGE, name) == 1) {
    return bool_value_put(name, true);
  }
  char value[256];
  #define VALUE_RANGE "[-kmKM0123456789]"
  if (jvm_sscanf(arg, "=%" NAME_RANGE "%" VALUE_RANGE, name, value) == 2) {
#if USE_SET_HEAP_LIMIT
    if (jvm_strcmp("HeapMin", name) == 0) {
#ifdef AZZERT
      tty->print_cr("HeapMin flag is not supported in case USE_SET_HEAP_LIMIT = true!");
#endif
      return false;
    }
#endif
    return set_numeric_flag(name, value);
  }
  return false;
}

#ifndef PRODUCT
void Globals::verify() {
  // Check consistency
}
#endif // PRODUCT

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
int __cdecl compare_flags(const void* a, const void* b) {
  return jvm_strcmp((*((JVMFlag**) a))->name, (*((JVMFlag**) b))->name);
}
#endif 

#if USE_DEBUG_PRINTING
void Globals::print_flags() {
  print_flags(tty);
}

void Globals::print_flags(void *_st) {
  Stream *st = (Stream*)_st;
  // Compute size
  int length= 0;
  while (flagTable[length].name != NULL) {
    length++;
  }

  // Sort flags alphabetically by name
  const JVMFlag** array = 
      NEW_GLOBAL_HEAP_ARRAY(const JVMFlag*, length, "Global flags sorting");
  for (int index = 0; index < length; index++) {
    array[index] = &flagTable[index];
  }
  jvm_qsort(array, length, sizeof(JVMFlag*), compare_flags);

  // Print
  st->print_cr("\n%-42s %s\n", "\040[Global Flags]", "Default value");
  for (int i = 0; i < length; i++) {
    array[i]->print_on(st);
  }
  FREE_GLOBAL_HEAP_ARRAY(array, "Global flags sorting array");
}

static void __PR(Stream *st, const char* cpp_flag_name) {
  st->print("  %-40s ", cpp_flag_name);
}

void Globals::print_build_options() {
  print_build_options(tty);
}

void Globals::print_build_options(void *_st) {
  Stream *st = (Stream*)_st;
  st->cr();
  st->print_cr("VM build options:");
  st->cr();

#define _IS_ENABLED    st->print_cr("enabled")
#define _IS_DISABLED   st->print_cr("-")

  // IMPL_NOTE: The code in this function can be
  // simplified if we all switch to the '#if' style of preprocessing
  // macros, and #define a macro to 0 if it's not defined.

__PR(st, "_DEBUG");
#if   defined(_DEBUG)
  _IS_ENABLED;
#else
  _IS_DISABLED;
#endif

__PR(st, "AZZERT");
#if   defined(AZZERT)
  _IS_ENABLED;
#else
  _IS_DISABLED;
#endif

  {
    // ENABLE_FLAG_VALUES is defined in the auto-generated jvmconfig.h
    static const char* enable_flag_values[] = ENABLE_FLAG_VALUES;
    for (int i=0; i<ARRAY_SIZE(enable_flag_values); i+=2) {
      const char *name  = enable_flag_values[i];
      const char *value = enable_flag_values[i+1];
      st->print_cr("  %-40s %s", name, value);
    }
  }

__PR(st, "HARDWARE_LITTLE_ENDIAN");
#if  HARDWARE_LITTLE_ENDIAN
  _IS_ENABLED;
#else
  _IS_DISABLED;
#endif

__PR(st, "MSW_FIRST_FOR_DOUBLE");
#if   MSW_FIRST_FOR_DOUBLE
  _IS_ENABLED;
#else
  _IS_DISABLED;
#endif

__PR(st, "MSW_FIRST_FOR_LONG");
#if   MSW_FIRST_FOR_LONG
  _IS_ENABLED;
#else
  _IS_DISABLED;
#endif

__PR(st, "ROMIZING");
#if   ROMIZING
  _IS_ENABLED;
#else
  _IS_DISABLED;
#endif

__PR(st, "USE_BSD_SOCKET");
#if   USE_BSD_SOCKET
  _IS_ENABLED;
#else
  _IS_DISABLED;
#endif

__PR(st, "USE_UNICODE_FOR_FILENAMES");
#if   USE_UNICODE_FOR_FILENAMES
  _IS_ENABLED;
#else
  _IS_DISABLED;
#endif

#undef _IS_ENABLED
#undef _IS_DISABLED
}

#define DEBUG_DECLARE_ERROR_MESSAGE(name, message) message,

static const char *debug_error_codes[] = {
  ERROR_MESSAGES_DO(DEBUG_DECLARE_ERROR_MESSAGE)
  NULL,
};

void Globals::print_error_codes() {
  tty->cr();
  tty->print_cr("VM error codes:");
  tty->cr();
  for (int i=0; debug_error_codes[i]!= NULL; i++) {
    tty->print_cr("%3d: %s", i, debug_error_codes[i]);
  }
}

#endif // USE_DEBUG_PRINTING

void Globals::print_version() {
  tty->print("Java Micro Edition\n"
             "%s %s (build %s, %s)\n",
             JVM_NAME, JVM_RELEASE_VERSION, JVM_BUILD_VERSION, JVM_VARIANT);
}

#if USE_DEBUG_PRINTING 

void Globals::print_definitions() {
  tty->print_cr("==========================================================");
  tty->print_cr("             VM Symbolic Definition Reference             ");
  tty->print_cr("==========================================================");
  tty->cr();
  print_version();
  tty->print_cr("VM Built on: %s %s", __DATE__, __TIME__);
  tty->cr();
  tty->cr();
  dump_interpreter_registers();
  dump_bytecodes();
  dump_persistent_handles();
  dump_constantpool_enums();
  dump_access_flags();
}

void Globals::dump_interpreter_registers() {
#if ENABLE_INTERPRETER_GENERATOR
  tty->print_cr("Interpreter Registers:");
  tty->cr();
  InterpreterGenerator::print_register_definitions();
  tty->cr();
#endif
}

void Globals::dump_bytecodes() {
  tty->print_cr("Bytecodes:");
  tty->cr();
  Bytecodes::print_definitions();
  tty->cr();
}

void Globals::dump_persistent_handles() {
  tty->print_cr("Persistent handles:");
  tty->cr();
  Universe::print_persistent_handle_definitions();
  tty->cr();

}

void Globals::dump_constantpool_enums() {
  tty->print_cr("ConstantPool tags:");
  tty->cr();
  ConstantTag::print_definitions();
  tty->cr();
}

void Globals::dump_access_flags() {
  tty->print_cr("Access flags:");
  tty->cr();
  AccessFlags::print_definitions();
  tty->cr();
}

#endif // USE_DEBUG_PRINTING
