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
# include "incls/_Semaphore.cpp.incl"

#if ENABLE_SEMAPHORE
void SemaphoreLock::acquire() {
  SETUP_ERROR_CHECKER_ARG;
  if (permits() > 0) {
    set_permits(permits() - 1);
  } else {
    GUARANTEE(permits() == 0, "sanity");
    Scheduler::wait(this, 0 JVM_NO_CHECK_AT_BOTTOM);
  }
}

void SemaphoreLock::release() {
  if (Scheduler::has_waiters(this)) {
    SETUP_ERROR_CHECKER_ARG;
    // Notify exactly 1 waiter. We effectly add one to permits and
    // the subtract one immediately, so there's no need to change it.
    Scheduler::notify(this, /*all=*/false, /*must_be_owner=*/false
                      JVM_NO_CHECK);
    GUARANTEE(permits() >= 0, "sanity");
  } else {
    set_permits(permits() + 1);
    GUARANTEE(permits() > 0, "sanity");
  }
}

void SNI_ReleaseSemaphore(jobject semaphore) {
  UsingFastOops fast_oops;
  Semaphore *sem = (Semaphore*) ((void*)semaphore);
  GUARANTEE(sem->not_null(), "cannot be null handle");
  SemaphoreLock::Fast lock = sem->lock();
  lock().release();
}

#ifndef PRODUCT
void Semaphore::verify_fields() {
  SETUP_ERROR_CHECKER_ARG;
  Symbol name = SymbolTable::symbol_for("com/sun/cldc/util/Semaphore"
                                        JVM_NO_CHECK);
  GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "sanity");
  InstanceClass klass =
      SystemDictionary::resolve(&name, ErrorOnFailure JVM_NO_CHECK);
  GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "sanity");
  klass.verify_instance_field("lock",  "Lcom/sun/cldc/util/SemaphoreLock;",
                              lock_offset()); 
}

void SemaphoreLock::verify_fields() {
  SETUP_ERROR_CHECKER_ARG;
  Symbol name = SymbolTable::symbol_for("com/sun/cldc/util/SemaphoreLock"
                                        JVM_NO_CHECK);
  GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "sanity");
  InstanceClass klass =
      SystemDictionary::resolve(&name, ErrorOnFailure JVM_NO_CHECK);
  GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "sanity");
  klass.verify_instance_field("permits",  "I", permits_offset()); 
}
#endif // PRODUCT

#endif // ENABLE_SEMAPHORE
