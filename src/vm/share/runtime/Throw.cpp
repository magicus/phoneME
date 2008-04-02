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
#include "incls/_Throw.cpp.incl"

ReturnOop
Throw::new_exception(Symbol* class_name, String* message JVM_TRAPS) {
  UsingFastOops fast_oops;

  GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "No pending exceptions");
  // The system may be damaged if there are any pending exceptions, so we make
  // sure that they are cleared, even in non-debug builds
  Thread::clear_current_pending_exception();

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  if (TraceExceptions || GenerateROMImage) {
    tty->print("TraceExceptions: ");
    class_name->print_value_on(tty);
    if (message->not_null()) {
      tty->print(" msg: ");
      message->print_value_on(tty);
    }
    tty->cr();
  }
  if (TraceExceptions) {
    ps();
  }
#endif
  if (Universe::is_stopping()) {
    return Universe::string_class()->obj();
  }
  InstanceClass::Fast exception_class =
      SystemDictionary::resolve(class_name, ErrorOnFailure JVM_NO_CHECK);
  if (CURRENT_HAS_PENDING_EXCEPTION) {
    GUARANTEE(exception_class.is_null(), "Sanity check");
    UsingFastOops raw_internal;
    // IMPL_NOTE: should better check if THREAD pending_exception() is
    // instance of java.lang.ClassNotFoundException.
    //
    // We are trying to throw an exception whose class cannot be
    // found - let's not go any further with this
    Thread::clear_current_pending_exception();

    // Instead, let's throw a general error whose message is
    // descriptive of the nested situation. We prepend (dottified)
    // 'class_name' ": " to 'message':
    SymbolStream s1(class_name);
#ifndef PRODUCT
    String::Fast err_message;
    if (message->is_null()) {
      err_message = Universe::new_string(&s1 JVM_CHECK_0);
    } else {
      UsingFastOops raw_internal2;
      LiteralStream s2(": ", 0, jvm_strlen(": "));
      TypeArray::Fast message_char_array = message->value();
      CharStream s3(&message_char_array);
      ConcatenatedStream c2(&s1, &s2);
      ConcatenatedStream c3(&c2, &s3);
      err_message = Universe::new_string(&c3 JVM_CHECK_0);
    }
#else
    // An abbreviated error msg. You'd be happy if you get any message
    // at all in PRODUCT mode!
    String::Fast err_message = Universe::new_string(&s1 JVM_CHECK_0);
#endif
    return new_exception(Symbols::java_lang_Error(),
                         &err_message JVM_NO_CHECK_AT_BOTTOM);
  }

  GUARANTEE(!exception_class.is_null(), "Sanity check");
  // Make sure the class is initialized
  exception_class().initialize(JVM_SINGLE_ARG_CHECK_0);

  Throwable::Fast exception = Universe::new_instance(&exception_class 
                                                     JVM_CHECK_0);

  EntryActivation::Fast entry;
  Method::Fast init;
  init = exception_class().lookup_method(Symbols::object_initializer_name(),
      message->is_null() ? Symbols::void_signature() : Symbols::string_void_signature());
  entry = Universe::new_entry_activation(&init, message->is_null() ? 1 : 2 JVM_CHECK_0);
  if (!message->is_null()) {
    entry().obj_at_put(1, message);
    // Just in case the constructor has not been not executed when we print it.
    exception().set_message(message);
  }
  entry().obj_at_put(0, &exception);
  Thread::current()->append_pending_entry(&entry);
  return exception;
}

void Throw::allocate_and_throw(Symbol* class_name, String* message JVM_TRAPS) {
  GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "No pending exceptions");
  // The system may be damaged if there are any pending exceptions, so we make
  // sure that they are cleared, even in non-debug builds
  Thread::clear_current_pending_exception();

  OopDesc* exception = new_exception(class_name, message JVM_NO_CHECK);
  if (exception) {
    Thread::set_current_pending_exception(exception);
  } else {
    GUARANTEE(CURRENT_HAS_PENDING_EXCEPTION, 
              "out-of-memory should have been thrown");
  }
}

void Throw::allocate_and_throw(Symbol* class_name, char *non_cldc_class_name,
                               String* message JVM_TRAPS) {
  UsingFastOops fast_oops;
  String::Fast msg = add_message_prefix(non_cldc_class_name, message 
                                        JVM_MUST_SUCCEED);
  allocate_and_throw(class_name, &msg JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::allocate_and_throw(Symbol* class_name, char *non_cldc_class_name,
                               ErrorMsgTag err JVM_TRAPS) {
  UsingFastOops fast_oops;
  GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "No pending exceptions");
  // The system may be damaged if there are any pending exceptions, so we make
  // sure that they are cleared, even in non-debug builds
  Thread::clear_current_pending_exception();

  String::Fast str;
  char *message = ErrorMessage::get(err);
  if (message != NULL && message[0] != 0) {
    str = Universe::new_string(message, jvm_strlen(message) JVM_NO_CHECK);
    // We specifically ignore any exception that the Universe::new_string
    // might throw.  If str ends up NULL, so be it.
    Thread::clear_current_pending_exception();
  }
  if (non_cldc_class_name != NULL) {
    str = add_message_prefix(non_cldc_class_name, &str JVM_MUST_SUCCEED);
  }

  Throwable::Fast exception = new_exception(class_name, &str JVM_NO_CHECK);
  if (exception.not_null()) {
    Thread::set_current_pending_exception(&exception);
  } else {
    GUARANTEE(CURRENT_HAS_PENDING_EXCEPTION, 
              "out-of-memory should have been thrown");
  }
}

ReturnOop
Throw::allocate_exception(Symbol* class_name, String* message JVM_TRAPS) {
  GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "No pending exceptions");
  // The system may be damaged if there are any pending exceptions, so we make
  // sure that they are cleared, even in non-debug builds
  Thread::clear_current_pending_exception();
  return new_exception(class_name, message JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::error(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::java_lang_Error(), err JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::class_format_error(ErrorMsgTag err JVM_TRAPS) {
#if ENABLE_ROM_GENERATOR
    if (!UseROM && Universe::before_main()) {
        tty->print_cr(
       "You are using insupported javac version, please use 1.4.* version to compile classes.");
        JVMSPI_Exit(1);
    }
#endif
  allocate_and_throw(Symbols::java_lang_ClassFormatError(),
                     (char*)"ClassFormatError", err
                     JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::verify_error(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::java_lang_VerifyError(), 
                     (char*)"VerifyError", err 
                     JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::out_of_memory_error(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  if (TraceExceptions) {
    TTY_TRACE_CR(("TraceExceptions: OutOfMemoryError"));
    ps();
  }
#endif

  Thread::set_current_pending_exception(
      Universe::out_of_memory_error_instance());
}


void Throw::class_not_found(Symbol* class_name, FailureMode fail_mode 
                            JVM_TRAPS) {
  LoaderContext loader_ctx(class_name, fail_mode);
  Throw::class_not_found(&loader_ctx JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::class_not_found(LoaderContext *loader_ctx JVM_TRAPS) {
  if (Universe::before_main()) {
     TTY_TRACE(("class not found: "));
     loader_ctx->class_name()->print_symbol_on(tty);
     tty->cr();
     JVM_FATAL(main_class_not_found);
  }
  UsingFastOops fast_oops;
  String::Fast str = Universe::new_string(loader_ctx->class_name() JVM_CHECK);
  Throwable::Fast error;
  if (loader_ctx->fail_mode() == ErrorOnFailure) {
#if !ENABLE_CLDC_11
    str = add_message_prefix("NoClassDefFoundError", &str JVM_MUST_SUCCEED);
#endif
    error = allocate_exception(Symbols::java_lang_NoClassDefFoundError(), &str
                               JVM_NO_CHECK_AT_BOTTOM);
    Thread::set_current_pending_exception(&error);
  } else {
    allocate_and_throw(Symbols::java_lang_ClassNotFoundException(), &str
                       JVM_NO_CHECK_AT_BOTTOM);
  }
}

void Throw::array_store_exception(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::java_lang_ArrayStoreException(), err
                     JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::null_pointer_exception(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::java_lang_NullPointerException(), err
                     JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::array_index_out_of_bounds_exception(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::java_lang_ArrayIndexOutOfBoundsException(), err
                     JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::illegal_access(FailureMode fail_mode JVM_TRAPS) {
  if (fail_mode == ErrorOnFailure) {
    error(illegal_access_error JVM_NO_CHECK_AT_BOTTOM);
  } else {
    allocate_and_throw(Symbols::java_lang_IllegalAccessException(),
                       illegal_access_exception JVM_NO_CHECK_AT_BOTTOM);
  }
}

void Throw::illegal_monitor_state_exception(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::java_lang_IllegalMonitorStateException(),
                     err JVM_NO_CHECK_AT_BOTTOM);
}

#if ENABLE_CLDC_11
void Throw::interrupted_exception(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::java_lang_InterruptedException(),
                     err JVM_NO_CHECK_AT_BOTTOM);
}
#endif

#if ENABLE_ISOLATES
void Throw::isolate_resource_error(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::com_sun_cldc_isolate_IsolateResourceError(),
                     err JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::isolate_startup_exception(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::com_sun_cldc_isolate_IsolateStartupException(),
                     err JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::isolate_state_exception(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::com_sun_cldc_isolate_llegalIsolateStateException(),
                     err JVM_NO_CHECK_AT_BOTTOM);
}
#endif 

void Throw::illegal_thread_state_exception(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::java_lang_IllegalThreadStateException(),
                     err JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::incompatible_class_change_error(ErrorMsgTag err JVM_TRAPS) {
  if (err == empty_message) {
    err = class_changed;
  }
  allocate_and_throw(Symbols::java_lang_IncompatibleClassChangeError(),
                     (char*)"IncompatibleClassChangeError",
                     err JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::instantiation(FailureMode fail_mode JVM_TRAPS) {
  if (fail_mode == ErrorOnFailure) {
    error(instantiation_error JVM_NO_CHECK_AT_BOTTOM);
  } else {
    allocate_and_throw(Symbols::java_lang_InstantiationException(),
                       instantiation_exception JVM_NO_CHECK_AT_BOTTOM);
  }
}

void Throw::arithmetic_exception(ErrorMsgTag err JVM_TRAPS) {
  allocate_and_throw(Symbols::java_lang_ArithmeticException(), err
                     JVM_NO_CHECK_AT_BOTTOM);
}

void Throw::unsatisfied_link_error(Method* method JVM_TRAPS) {
  UsingFastOops fast_oops;
  Symbol::Fast method_name_symbol = method->name();
  String::Fast method_name_string =
      Universe::new_string(&method_name_symbol JVM_CHECK);
  allocate_and_throw(Symbols::java_lang_UnsatisfiedLinkError(),
                     (char*)"UnsatisfiedLinkError", &method_name_string
                     JVM_NO_CHECK_AT_BOTTOM);
}

#if !ROMIZED_PRODUCT
// Used only when bootstraping a romgen VM.
void Throw::initialize(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  InstanceClass::Fast exception_class =
     SystemDictionary::resolve(Symbols::java_lang_OutOfMemoryError(),
                               ErrorOnFailure JVM_CHECK);
  *Universe::out_of_memory_error_instance() =
      Universe::new_instance(&exception_class JVM_NO_CHECK_AT_BOTTOM);
}
#endif

// This should only be used to stop the VM
// IMPL_NOTE:  Maybe we should not be throwing the string class
// since Universe::string_class() is not really a Throwable class.
void Throw::uncatchable(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  Thread::set_current_pending_exception(Universe::string_class());
}

ReturnOop Throw::add_message_prefix(char *prefix, String *message JVM_TRAPS) {
#if !ENABLE_STACK_TRACE
  // This function is intended to help developers. If you need to disable
  // stack traces to save code space on an actual device, then you probably
  // don't need this, either.
  return message->obj();
#else
  int prefixlen = jvm_strlen(prefix);
  int len = prefixlen;
  int msglen = 0;
  if (message->not_null()) {
    msglen = message->length();
    len += 2 + msglen;
  }
  UsingFastOops fast_oops;
  TypeArray::Fast char_array = Universe::new_char_array(len JVM_NO_CHECK);
  if (char_array.is_null()) {
    // If allocation fails, just use original message
    Thread::clear_current_pending_exception();
    return message->obj();
  }
  jchar *ptr = (jchar*)char_array().base_address();
  for (int i=0; i<prefixlen; i++) {
    *ptr ++ = (jchar)(*prefix++);
  }
  if (message->not_null()) {
    *ptr ++ = (jchar)':';
    *ptr ++ = (jchar)' ';
    TypeArray::Raw v = message->value();
    jchar *msgptr = ((jchar*)v().base_address()) + message->offset();
    for (int i=0; i<msglen; i++) {
      *ptr ++ = *msgptr++;
    }
  }
  GUARANTEE(ptr == (jchar*)char_array().base_address() + len, "sanity");
  String::Fast str = Universe::new_string(&char_array, 0, len JVM_NO_CHECK);
  if (str.is_null()) {
    // If allocation fails, just use original message
    Thread::clear_current_pending_exception();
    return message->obj();
  } else {
    return str.obj();
  }
#endif
}
