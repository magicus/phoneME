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

/** \file KniImpl.hpp
 *
 * This file contains common declarations used by the KNI implementation
 * files: kni.cpp and KniUncommon.cpp
 */

#define _KNI_IGNORE_ME_(x)

#define _KNI_CHECK \
          JVM_NO_CHECK); \
        if (CURRENT_HAS_PENDING_EXCEPTION) { \
           Thread::clear_current_pending_exception(); \
           return; \
        } _KNI_IGNORE_ME_(0

#define _KNI_SINGLE_ARG_CHECK \
          JVM_SINGLE_ARG_NO_CHECK); \
        if (CURRENT_HAS_PENDING_EXCEPTION) { \
           Thread::clear_current_pending_exception(); \
           return; \
        } _KNI_IGNORE_ME_(0

#define _KNI_CHECK_(result)  \
          JVM_NO_CHECK); \
        if (CURRENT_HAS_PENDING_EXCEPTION) { \
          Thread::clear_current_pending_exception(); \
          return result; \
        } _KNI_IGNORE_ME_(0

#define _KNI_OOME_CHECK_(result)  \
          JVM_NO_CHECK); \
        if (CURRENT_HAS_PENDING_EXCEPTION) { \
          Oop::Raw oome = Universe::out_of_memory_error_instance(); \
          if (oome.equals(Thread::current_pending_exception())) { \
            return KNI_OK; \
          } \
          Thread::clear_current_pending_exception(); \
          return result; \
        } _KNI_IGNORE_ME_(0

#define _KNI_CHECK_0 \
        _KNI_CHECK_(0)

inline OopDesc* kni_read_handle(jobject handle) {
  return *(OopDesc**)handle;
}

inline void kni_set_handle(jobject handle, OopDesc* p) {
  *(OopDesc**)handle = p;
}

inline void kni_clear_handle(jobject handle) {
  *(OopDesc**)handle = (OopDesc*)NULL;
}

jfieldID _KNI_field_lookup_helper(jclass classHandle, const char* name,
                                  const char* signature, bool is_static);
