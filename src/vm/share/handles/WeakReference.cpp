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
#include "incls/_WeakReference.cpp.incl"

/** \file WeakReference.cpp
 *
 * This file implements features for accessing the java.lang.ref.WeakRefernece
 * class in C code.
 */

#if ENABLE_CLDC_11

// IMPL_NOTE: check if class is java.lang.ref.WeakReference
HANDLE_CHECK(WeakReference, is_java_oop())

#ifndef PRODUCT
void WeakReference::verify_fields() {
  SETUP_ERROR_CHECKER_ARG;
  InstanceClass::Raw ic =
      SystemDictionary::resolve(Symbols::java_lang_ref_WeakReference(),
                                ErrorOnFailure JVM_NO_CHECK);
  if (CURRENT_HAS_PENDING_EXCEPTION) {
    JVM_FATAL(root_class_loading_failure);
  }
  
  ic().verify_instance_field("referent_index", "I", referent_index_offset());
}
#endif // !PRODUCT

#endif // ENABLE_CLDC_11
