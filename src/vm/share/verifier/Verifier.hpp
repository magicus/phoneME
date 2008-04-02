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
   Handles the bytecode verification for a given class.
 */

class Verifier {
  static bool _is_active;
  static bool _is_cache_active;
  static void verify_class_internal(InstanceClass* klass JVM_TRAPS);
  static int _stackmap_cache_max;
public:

  static bool is_active() {
    return _is_active;
  }

  enum {
    STACKMAP_CACHE_SIZE            = 512,
    INSTRUCTION_STARTS_CACHE_SIZE  = 512,
    VSTACK_CACHE_SIZE              =  40,
    VLOCALS_CACHE_SIZE             =  40
  };
  static void verify_class(InstanceClass* klass JVM_TRAPS);
  static bool flush_cache();

#if ENABLE_VERIFY_ONLY
  static bool verify_classpath();
#endif

  static int stackmap_cache_max() {
    return _stackmap_cache_max;
  }
friend class VerifyMethodCodes;
};
