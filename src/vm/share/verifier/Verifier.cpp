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

/** 
   \class Verifier
   Handles the byte code verification for a given class.
 
   This VM has a two-phase class file verifier.  
   In order to run this VM, class files must 
   first be processed with a special "pre-verifier" tool. 
   This phase is typically done on the development 
   workstation.  During execution, the runtime verifier 
   (defined in VerifyClassFile.cpp) performs the actual 
   class file verification based on both runtime information 
   and pre-verification information.
 */

#include "incls/_precompiled.incl"
#include "incls/_Verifier.cpp.incl"

bool Verifier::_is_active = false;
bool Verifier::_is_cache_active = false;

/**
 * Perform byte-code verification of a given class. 
 * Iterate through all methods.
 *
 * @param ic; class to be verified.
 * @return 0 if verification succeeds, error code if verification fails.
 */
void Verifier::verify_class_internal(InstanceClass* ic JVM_TRAPS) {
  UsingFastOops fast_oops;

#ifndef PRODUCT
  if (TraceVerifier) {
    tty->print("Verifying Class "); 
    ic->print_value_on(tty);
    tty->cr();
  }
#endif

  // Verify all methods 
  ObjArray::Fast methods = ic->methods();
  Method::Fast method;
  ObjArray::Fast stackmap;

  for (int i = 0; i < methods().length(); i++) {
    method = methods().obj_at(i);

    // Skip abstract and native methods. 
    if (method().access_flags().is_native() || 
        method().access_flags().is_abstract()) {
      continue;
    }

#ifndef PRODUCT
    if (TraceVerifier) {
      tty->print("Verifying method "); 
      method().print_value_on(tty);
      tty->cr();
    }
#endif
    VerifyMethodCodes verifier;
    stackmap = method().stackmaps();
    verifier.verify(&method, &stackmap JVM_CHECK);
  }

  StackmapGenerator::compress_verifier_stackmaps(ic JVM_CHECK);

  ic->set_verified();
} 

void Verifier::verify_class(InstanceClass* ic JVM_TRAPS) {
  GUARANTEE(!is_active(), "not re-entrant");

  // this one allows is_active to be restored to false, even if 
  // one of allocations downstream will fail
  GlobalSaver activityChecker(&_is_active);

#if ENABLE_PERFORMANCE_COUNTERS
  jlong start_time = Os::elapsed_counter();
  jlong last_load_hrticks = jvm_perf_count.total_load_hrticks;
#endif

  _is_active = true;
  _is_cache_active = true;
  // is_active must be true here, or allocation of one of the cache items
  // might clear another cache item!

  if (Universe::verifier_stackmap_cache()->is_null()) {
    *Universe::verifier_stackmap_cache() = 
        Universe::new_int_array(STACKMAP_CACHE_SIZE JVM_CHECK);
  }
  if (Universe::verifier_instruction_starts_cache()->is_null()) {
    *Universe::verifier_instruction_starts_cache() = 
        Universe::new_byte_array(INSTRUCTION_STARTS_CACHE_SIZE JVM_CHECK);
  }
  if (Universe::verifier_vstack_tags_cache()->is_null()) {
    *Universe::verifier_vstack_tags_cache() =
        Universe::new_int_array(VSTACK_CACHE_SIZE JVM_CHECK);
  }
  if (Universe::verifier_vstack_classes_cache()->is_null()) {
    *Universe::verifier_vstack_classes_cache() =
        Universe::new_obj_array(VSTACK_CACHE_SIZE JVM_CHECK);
  }
  if (Universe::verifier_vlocals_tags_cache()->is_null()) {
    *Universe::verifier_vlocals_tags_cache() =
        Universe::new_int_array(VLOCALS_CACHE_SIZE JVM_CHECK);
  }
  if (Universe::verifier_vlocals_classes_cache()->is_null()) {
    *Universe::verifier_vlocals_classes_cache() =
        Universe::new_obj_array(VLOCALS_CACHE_SIZE JVM_CHECK);
  }


  verify_class_internal(ic JVM_NO_CHECK_AT_BOTTOM); 

#if ENABLE_PERFORMANCE_COUNTERS
  // Don't count the class loading time during verification, since we want
  // to meausre the effect of only verification itself (so that we can
  // estimate the effect of turning off verification).
  jlong elapsed = Os::elapsed_counter() - start_time + 
                  last_load_hrticks - jvm_perf_count.total_load_hrticks;

  jvm_perf_count.total_verify_hrticks += elapsed;
  if (jvm_perf_count.max_verify_hrticks < elapsed) {
      jvm_perf_count.max_verify_hrticks = elapsed;
  }
#endif

}

bool Verifier::flush_cache() {
  if (!_is_active && _is_cache_active) {
    // We're in a full GC and the verifier is not active, let's discard the
    // stackmap cache.
    Universe::verifier_stackmap_cache()->set_null();
    Universe::verifier_instruction_starts_cache()->set_null();
    Universe::verifier_vstack_tags_cache()->set_null();
    Universe::verifier_vstack_classes_cache()->set_null();
    Universe::verifier_vlocals_tags_cache()->set_null();
    Universe::verifier_vlocals_classes_cache()->set_null();
    _is_cache_active = false;
    return true;
  } else {
    return false;
  }
}

#if ENABLE_VERIFY_ONLY

bool Verifier::verify_classpath() {
  SETUP_ERROR_CHECKER_ARG;
  Universe::load_all_in_classpath(JVM_SINGLE_ARG_CHECK_0);
  if (CURRENT_HAS_PENDING_EXCEPTION) {
    Thread::clear_current_pending_exception();
    return false; 
  }
  return true;
}

#endif
