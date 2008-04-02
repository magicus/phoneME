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

// Interface for throwing exceptions in the JVM

/** \class Throw
    Api for creating and throwing exceptions
*/
class Throw : public AllStatic {
 public:
  static ReturnOop new_exception(Symbol* class_name, String* message JVM_TRAPS);

  static ReturnOop allocate_exception(Symbol* class_name, String* message
                                      JVM_TRAPS);
  static void allocate_and_throw(Symbol* class_name, String* message 
                                 JVM_TRAPS);
  static void allocate_and_throw(Symbol* class_name, ErrorMsgTag err 
                                 JVM_TRAPS) {
    allocate_and_throw(class_name, NULL, err JVM_NO_CHECK_AT_BOTTOM);
  }
  static void allocate_and_throw(Symbol* class_name, char *non_cldc_class_name,
                                 String* message JVM_TRAPS);
  static void allocate_and_throw(Symbol* class_name, char *non_cldc_class_name,
                                 ErrorMsgTag err JVM_TRAPS);
  static ReturnOop add_message_prefix(char *prefix, String *message JVM_TRAPS);
  static void error(ErrorMsgTag err JVM_TRAPS);
  static void class_format_error(ErrorMsgTag err JVM_TRAPS);
  static void verify_error(ErrorMsgTag err JVM_TRAPS);
  static void class_not_found(Symbol* class_name, FailureMode fail_mode
                              JVM_TRAPS);
  static void class_not_found(class LoaderContext *loader_ctx JVM_TRAPS);
  static void out_of_memory_error(JVM_SINGLE_ARG_TRAPS);
  static void arithmetic_exception(ErrorMsgTag err JVM_TRAPS);
  static void no_such_method_error(JVM_SINGLE_ARG_TRAPS) {
    Throw::error(no_such_method JVM_NO_CHECK_AT_BOTTOM);
  }
  static void no_such_field_error(JVM_SINGLE_ARG_TRAPS) {
    Throw::error(no_such_field JVM_NO_CHECK_AT_BOTTOM);
  }
  static void array_store_exception(ErrorMsgTag err JVM_TRAPS);
  static void null_pointer_exception(ErrorMsgTag err JVM_TRAPS);
  static void array_index_out_of_bounds_exception(ErrorMsgTag err JVM_TRAPS);
  static void illegal_access(FailureMode fail_mode JVM_TRAPS);
  static void illegal_monitor_state_exception(ErrorMsgTag err JVM_TRAPS);
  static void illegal_thread_state_exception(ErrorMsgTag err JVM_TRAPS);
#if ENABLE_CLDC_11
  static void interrupted_exception(ErrorMsgTag err JVM_TRAPS);
#endif
#if ENABLE_ISOLATES
  static void inaccessible_isolate_exception(ErrorMsgTag err JVM_TRAPS);
  static void isolate_resource_error(ErrorMsgTag err JVM_TRAPS);
  static void isolate_startup_exception(ErrorMsgTag err JVM_TRAPS);
  static void isolate_state_exception(ErrorMsgTag err JVM_TRAPS);
#endif 
  static void incompatible_class_change_error(ErrorMsgTag err JVM_TRAPS);
  static void instantiation(FailureMode fail_mode JVM_TRAPS);
  static void unsatisfied_link_error(Method* method JVM_TRAPS);
  static void initialize(JVM_SINGLE_ARG_TRAPS);

  static void throw_exception(Symbol *class_name JVM_TRAPS) {
    allocate_and_throw(class_name, empty_message JVM_NO_CHECK_AT_BOTTOM);
  }
  static void throw_exception(Symbol *class_name, ErrorMsgTag err JVM_TRAPS) {
    allocate_and_throw(class_name, err JVM_NO_CHECK_AT_BOTTOM);
  }

  static void uncatchable(JVM_SINGLE_ARG_TRAPS);
};
