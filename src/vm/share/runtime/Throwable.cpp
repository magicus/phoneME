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

# include "incls/_precompiled.incl"
# include "incls/_Throwable.cpp.incl"

HANDLE_CHECK(Throwable, is_throwable())

void Throwable::print_stack_trace() {
  bool is_complete = false;

  InstanceClass::Raw exception_class = blueprint();
  Symbol::Raw class_name = exception_class().name();
  if (class_name.is_null()) {
    tty->print("<Unknown exception type>");
  } else {
    class_name().print_symbol_on(tty, true);
  }

  String::Raw msg = message();
  if (!msg.is_null()) {
    tty->print(": ");
    msg().print_string_on(tty);
  }
  tty->cr();

#if ENABLE_STACK_TRACE
  
  ObjArray::Raw trace = backtrace();
  ObjArray::Raw methods;
  TypeArray::Raw offsets;

  if (!trace.is_null()) {
    methods = trace().obj_at(0);
    offsets = trace().obj_at(1);
    if (!methods.is_null() && !offsets.is_null()) {
      int i;
      for (i=0; i<methods().length(); i++) {
        Method::Raw m = methods().obj_at(i);
        int bci = offsets().int_at(i);
        if (m.is_null()) {
          break;
        }
        tty->print(" - ");
        InstanceClass::Raw ic = m().holder();
        Symbol::Raw class_name = ic().name();

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
        // Non-public classes in a romized image may be renamed to
        // .unknown. to save space. In non-product mode, to aid
        // debugging, we retrieve the original name using
        // ROM::get_original_method_name().
        if (class_name().equals(Symbols::unknown())) {
          ClassInfo::Raw class_info = ic().class_info();
          class_name = ROM::get_original_class_name(&class_info);
        }
#endif

        class_name().print_symbol_on(tty, true);
        tty->print(".");

        Symbol::Raw name = m().name();

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
        // Non-public methods in a romized image may be renamed to
        // .unknown. to save space. In non-product mode, to aid
        // debugging, we retrieve the original name using
        // ROM::get_original_method_name().
        if (name().equals(Symbols::unknown())) {
          name = ROM::get_original_method_name(&m);
        }
#endif

        name().print_symbol_on(tty);
        tty->print_cr("(), bci=%d", bci);
      }
      if (i == methods().length()) {
        is_complete = true;
      }
    }
  }
#endif

  if (!is_complete) {
    tty->print_cr("   (stack trace incomplete)");
  }
}

#ifndef PRODUCT

void Throwable::verify_fields() {
  UsingFastOops fast_oops;
  InstanceClass::Fast klass = Universe::throwable_class();

  // Verify instance field offsets.
  klass().verify_instance_field("detailMessage", "Ljava/lang/String;",
                                message_offset());
  klass().verify_instance_field("backtrace", "Ljava/lang/Object;",
                                backtrace_offset());
}

#endif
