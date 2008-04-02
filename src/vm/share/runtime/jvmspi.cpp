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
#include "incls/_jvmspi.cpp.incl"

/** \file jvmspi.cpp
 * Default implementations of JVMSPI functions that
 * are the same for all supported platforms. You need to replace
 * the contents of this file for a 'real' profile such as MIDP.
 *
 * Those JVMSPI functions that differ between OSs
 * are defined in the respective 'Main_<os>.cpp' file.
 */

jboolean JVMSPI_CheckExit(void) {
  return KNI_TRUE;
}

/*
 * Notes on clean-up of user-defined system properties:
 *
 * Each time JVM_Start() is called, the command-line arguments are
 * parsed again. So if you have -D arguments in the command line for
 * setting up user-defined system properties, they must be cleaned up
 * before a subsequent call to JVM_Start().
 *
 * Such clean-up is absent from this file. That's because the default
 * VM executable (started by Main_<os_family>.cpp) calls
 * JVM_Start() exactly once during its entire lifetime. However, if
 * your MIDP profile calls JVM_Start() multiple times, you must do the
 * proper clean-up.
 */

struct SystemProperty {
   SystemProperty * next;
   char *name;
   char *value;
};

static SystemProperty * system_properties = NULL;

void JVMSPI_SetSystemProperty(char *property_name, char *property_value) {
  if (property_name == NULL || property_value == NULL) {
    // IMPL_NOTE: need to report an error
    return;
  }

  SystemProperty *prop =
      (SystemProperty*)OsMemory_allocate(sizeof(SystemProperty));
  prop->next = system_properties;
  system_properties = prop;

  prop->name  = (char*)OsMemory_allocate(jvm_strlen(property_name) + 1);
  prop->value = (char*)OsMemory_allocate(jvm_strlen(property_value) + 1);
  jvm_strcpy(prop->name,  property_name);
  jvm_strcpy(prop->value, property_value);
}

char *JVMSPI_GetSystemProperty(char *property_name) {
  SystemProperty *prop;

  for (prop = system_properties; prop; prop = prop->next) {
    if (jvm_strcmp(property_name, prop->name) == 0) {
      return prop->value;
    }
  }
  return NULL;
}

void JVMSPI_FreeSystemProperty(char * /*prop_value*/) {
  // do nothing
}

#if ENABLE_JAVA_DEBUGGER
extern "C" void JVMSPI_DebuggerNotification(jboolean is_active) {
  (void)is_active;
  return;
}
#endif

#if ENABLE_METHOD_TRAPS
void JVMSPI_MethodTrap(int /* trap_action */, int /* trap_handle */) {
  // do nothing
}
#endif

#if ENABLE_MONET_COMPILATION
jboolean JVMSPI_IsPrecompilationTarget(const char * class_name, 
                                       int class_name_length,
                                       const char * method_name, 
                                       int method_name_length,
                                       const char * descriptor, 
                                       int descriptor_length,
                                       int code_size) {
  (void)class_name;
  (void)class_name_length;
  (void)method_name;
  (void)method_name_length;
  (void)descriptor;
  (void)descriptor_length;
  (void)code_size;
  return KNI_TRUE;
}
#endif				      

static void P(const char* x) {
  tty->print_cr(x);
}

void JVMSPI_DisplayUsage(char* message) {
  if (message != NULL) {
    P("");
    P("Error: ");
    P((const char*)message);
    P("");
  }
  P("Usage: cldc_vm [options] class [args...]");
  P("");
  P("where options include:");
  P("    -cp -classpath <directory>");
  P("                : Set search path for application classes and resources");
  P("    -comp       : Compile all methods");
  P("    -int        : Force interpreted mode");
  P("    -version    : Print VM version");
  P("    -verbose    : Enable verbose output");
  P("    -? -help    : Print this help message");

#if ENABLE_ROM_GENERATOR || ENABLE_INTERPRETER_GENERATOR
  P("    -convert    : Create binary rom image of application classes");
  P("    -romoutputfile");
  P("                : Output filename for binary rom image");
#endif

#if USE_DEBUG_PRINTING
  P("    -definitions: List values of VM symbolic definitions");
  P("    -flags      : List all available Global Flags");
  P("    -buildopts  : List all build-time options");
  P("    -errorcodes : List all error codes");
#endif

#ifndef PRODUCT
  P("    -romize     : Generate rom image of system classes");
  P("    -generate   : Generate interpreter source file");
  P("    -generateoptimized");
  P("                : Generate optimized interpreter source file");
  P("    -romconfig <file>");
  P("                : Name of romizer configuration file");
  P("    -compilertestconfig <file>");
  P("                : Name of compiler testing configuration file.");
#endif

#ifdef REMOTE_HOST
  P("    -tracehost  : Set trace host (defaults to 'localhost')");
#endif

#if ENABLE_JAVA_DEBUGGER
  P("    -debugger   : Enable support for Java level debugging");
  P("    [-port]     : Port for debugger proxy to attach to (default 2800)");
  P("    [-suspend]  : Suspend VM after sending VMInit event (default)");
  P("    [-nosuspend]: Don't suspend VM after sending VMInit event");
#endif

#ifndef PRODUCT
  P("    +/-GlobalFlag");
  P("                : Turn on/off a boolean Global Flag");
  P("    =GlobalflagNN[K|M]");
  P("                : Set an integer Global Flag to value NN");
  P("                : K means Kilobytes, M means Megabytes");
  P("                : e.g. =HeapCapacity10M  set HeapCapacity to 10 MB");
#endif

#if ENABLE_METHOD_TRAPS
  P("    MethodTrap=package.ClassName.methodName[,N][:ACTION]");
  P("                : Take an action before the specified Java method");
  P("                : is called for the N-th time (default N=1)");
  P("                : ACTION can be 'exit', 'stop' or a numeric value");
#endif
}
