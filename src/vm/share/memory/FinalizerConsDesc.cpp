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
#include "incls/_FinalizerConsDesc.cpp.incl"

void FinalizerConsDesc::run_finalizer( void ) {
  void (*native_finalizer)();
  unsigned char *remember_kni_parameter_base;
  
  {
    JavaOop::Raw r = referent();

    if (TraceFinalization) {
      TTY_TRACE_CR(("TraceGC: finalizer 0x%p retrieved", (void*) r));
    }

    InstanceClass::Raw referent_class = r.blueprint();
    GUARANTEE(referent_class().has_finalizer(), 
            "Class of finalizable object must have finalizer");

    Method::Raw finalize_method = 
      referent_class().lookup_method(Symbols::finalize_name(), 
                                     Symbols::void_signature());
    GUARANTEE(finalize_method.not_null(), "Must have finalize() method");
  
    remember_kni_parameter_base = _kni_parameter_base;

    // Set up a GC-safe fake KNI parameter base to support KNI access to 
    // the receiver:
    _kni_parameter_base = (unsigned char *) &r()._obj;

    native_finalizer = finalize_method().is_quick_native() ?
      (void (*)()) finalize_method().get_quick_native_code() :
      (void (*)()) finalize_method().get_native_code();

    // Registration would fail if finalizer were invalid
    GUARANTEE(native_finalizer != (void (*)())NULL &&
              native_finalizer != (void (*)())Java_void_unimplemented,
              "Finalizer must be valid at this point");
  }

  (*native_finalizer)();
  _kni_parameter_base = remember_kni_parameter_base;
}
