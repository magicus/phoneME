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

/** \file Debug.hpp: VM-internal debugging operations
 *
 * This file provides definitions and declarations that are 
 * used internally in the VM to support various
 * assertions and internal consistency checks.  These 
 * extra checks are performed in non-product builds only.
 *
 * The general format for adding assertions / consistency 
 * checks to the VM is:
 *
 *     GUARANTEE(condition, errorMessage);
 *
 * where "condition" is an expression that evaluates to
 * a boolean value.  Examples:
 *
 *     GUARANTEE(instance_size > 0, "bad instance size");
 *     GUARANTEE(length >= 0, "Cannot allocate symbol of negative length");
 *            
 * Important: The definitions in this file are related to 
 * the debugging of the virtual machine itself -- not to 
 * debugging of the Java code that is executed by the 
 * virtual machine.
 */

#ifndef PRODUCT
// A GUARANTEE macro that works in release mode
#if ENABLE_VERBOSE_ASSERTION
#define GUARANTEE_R(b,msg) {                                     \
    if (!(b)) {                                                  \
      report_assertion_failure(XSTR(b),__FILE__, __LINE__, msg); \
    }                                                            \
  }
#else
#define GUARANTEE_R(b,msg) {                                     \
    if (!(b)) {                                                  \
      report_assertion_failure();                                \
    }                                                            \
  }
#endif // ENABLE_VERBOSE_ASSERTION
#else
#define GUARANTEE_R(p,msg) {}
#endif

// Assertions
#ifdef AZZERT
#define GUARANTEE(p, msg) GUARANTEE_R(p, msg) 
#define AZZERT_ONLY(code) code
#define AZZERT_ONLY_VAR(param)
#else
#define GUARANTEE(p,msg) {}
#define AZZERT_ONLY(code) /*next token must be ;*/
#define AZZERT_ONLY_VAR(param) (void)param
#endif

// PRODUCT variant macros
#ifdef PRODUCT
#define NOT_PRODUCT(code)
#define PRODUCT_ONLY(code) code
#define PRODUCT_RETURN  {}
#define PRODUCT_RETURN0 { return 0; }
#else
#define NOT_PRODUCT(code) code
#define PRODUCT_ONLY(code) 
#define PRODUCT_RETURN  /* next token must be ; */
#define PRODUCT_RETURN0 /* next token must be ; */
#endif

#if ROMIZED_PRODUCT
#define ROMIZED_PRODUCT_RETURN  {}
#define ROMIZED_PRODUCT_RETURN0 { return 0; }
#else
#define ROMIZED_PRODUCT_RETURN  /* next token must be ; */
#define ROMIZED_PRODUCT_RETURN0 /* next token must be ; */
#endif

#if defined(PRODUCT) && (!ENABLE_PERFORMANCE_COUNTERS)
#define PRODUCT_NOCOUNTER_RETURN  {}
#define PRODUCT_NOCOUNTER_RETURN0 { return 0; }
#else
#define PRODUCT_NOCOUNTER_RETURN  /* next token must be ; */
#define PRODUCT_NOCOUNTER_RETURN0 /* next token must be ; */
#endif

// Fatal errors
#if defined(PRODUCT) || !ENABLE_VERBOSE_ASSERTION
#define JVM_FATAL(m) report_fatal(m)
#else
#define JVM_FATAL(m) { report_fatal(__FILE__, __LINE__, m); }
#endif

// Sanity checks
#ifndef PRODUCT
#if ENABLE_VERBOSE_ASSERTION
#define SHOULD_NOT_REACH_HERE() { \
    report_should_not_reach_here(__FILE__, __LINE__); \
}
#define UNIMPLEMENTED() { \
    report_unimplemented(__FILE__, __LINE__); \
}
#else
#define SHOULD_NOT_REACH_HERE() { \
    report_should_not_reach_here(); \
}
#define UNIMPLEMENTED() { \
    report_unimplemented(); \
}
#endif
#else
#define SHOULD_NOT_REACH_HERE()
#define UNIMPLEMENTED()
#endif

// Usage (note the double parenthesis): 
//     TTY_TRACE_CR(("format %x %d...", a, b));

#if ENABLE_TTY_TRACE
#define TTY_TRACE(x)             tty->print    x;
#define TTY_TRACE_CR(x)          tty->print_cr x;
#define TTY_TRACE_IF_VERBOSE(x)  if (Verbose) {tty->print x;}
#else
#define TTY_TRACE(x) 
#define TTY_TRACE_CR(x) 
#define TTY_TRACE_IF_VERBOSE(x)
#endif

#if USE_COMPILER_COMMENTS
#define COMPILER_COMMENT(x) Compiler::code_generator()->comment x
#define COMPILER_PRINT_AS_YOU_GO(x) \
        if (PrintCompiledCodeAsYouGo) {TTY_TRACE_CR(x);}
#define VERBOSE_SCHEDULING_AS_YOU_GO(x) \
	    if (OptimizeCompiledCodeVerboseInternal) {TTY_TRACE_CR(x);}
#define VERBOSE_CSE(x) \
	    if (VerboseByteCodeEliminate) {TTY_TRACE_CR(x);}
#else
#define COMPILER_COMMENT(x) 
#define COMPILER_PRINT_AS_YOU_GO(x)
#define VERBOSE_SCHEDULING_AS_YOU_GO(x)
#define VERBOSE_CSE(x)
#endif


// main error reporting function.
// is_vm_internal_error allows us to print a different error message
// for crashes known to have occurred in native code outside the VM.
// NOTE: This routine returns control to the caller if
// is_vm_internal_error is set to false!
#if defined(PRODUCT)
void report_fatal(ErrorMsgTag err);
#else
void report_error(bool is_vm_internal_error, const char *file_name, 
                  int file_no, const char* title, const char* format, ...);
void warning(const char* format, ...);
#if ENABLE_VERBOSE_ASSERTION
void report_assertion_failure(const char* code_str, const char* file_name, 
                              int line_no, const char* message);
void report_fatal(const char* file_name, int line_no, ErrorMsgTag err);
void report_should_not_reach_here(const char* file_name, int line_no);
void report_unimplemented(const char* file_name, int line_no);
#else
void report_assertion_failure();
void report_fatal(ErrorMsgTag err);
void report_should_not_reach_here();
void report_unimplemented();
#endif
#endif

#ifndef AZZERT
#define DECLARE_STATIC_BUFFER2(checker, type, name, length) \
        type name[(length)]
#else
#define STATIC_BUFFER_PADDING 20

#define DECLARE_STATIC_BUFFER2(checker, type, name, length) \
        type name[((length) + STATIC_BUFFER_PADDING)]; \
        StaticBufferChecker checker((char*)name, length, sizeof(type))

class StaticBufferChecker {
  char * _buf;
  int    _length;
  int    _elem_size;

public:
  StaticBufferChecker(char *buf, int length, int elem_size);
  ~StaticBufferChecker();
};

#endif // AZZERT

#define DECLARE_STATIC_BUFFER(type, name, length) \
        DECLARE_STATIC_BUFFER2(__checker, type, name, length)

// This class is used to restore the last_handle and last_raw_handle
// variables in case we crash inside a debug function such as pp()
// or Oop::p().
class DebugHandleMarker {
public:

  DebugHandleMarker()   PRODUCT_RETURN;
  ~DebugHandleMarker()  PRODUCT_RETURN;
  static void restore() PRODUCT_RETURN;

#ifndef PRODUCT
private:
  static Oop *_saved_last_handle;
  static Oop *_saved_last_raw_handle;
  static bool _is_active;
  bool _is_outermost;
#endif

};

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

// Print stack of current thread
extern void ps();

// Print stack, using guess_fp as the starting address for guessing the fp.
// This is useful when we're inside compiled code or interpreter where
// the fp is not stored in Thread::current().
extern void psg(address guess_fp);

// Print the stack while we're inside the GC
extern void psgc();

// The GC call this function immediately before collection, so that we
// can print the stack even when GC is active.
extern void save_java_stack_snapshot();

#endif

// Debug helper functions
#ifndef PRODUCT

// Try to figure out what x is and print it
extern void find(int x);

// Pretty print object
extern void pp(int x);

// Pretty print object verbosely
extern void ppv(int x);

// Print compiled method being compiled
extern void dis();

// Print object heap
extern void oh();

// Print stack of all threads
extern void new_pss();

// Print current compiler virtual stack frame
extern void vsf();

// Print register allocator info
extern void ra();

// Print x86 FPU stack simulation
extern void fpu();

// Print object heap
extern void poh();

// Find references to heap object
extern void ref(int x);

#endif // NOT PRODUCT


#if ENABLE_PRODUCT_PRINT_STACK || !defined(PRODUCT)
// Print stack of all threads
extern "C" void pss();
#endif
