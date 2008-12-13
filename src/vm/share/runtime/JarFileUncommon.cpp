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
#include "incls/_JarFileUncommon.cpp.incl"

#if ENABLE_JAR_READER_EXPORTS

extern "C" void
Jvm_read_jar_entry(const JvmPathChar* jar_file_name, const char* entry_name, 
                   jobject entry) {
   SETUP_ERROR_CHECKER_ARG;
   UsingFastOops fast_oops;
   JarFileParser::Fast parser = JarFileParser::get(jar_file_name JVM_NO_CHECK);
   if (parser.not_null()) {
     bool found = parser().find_entry(entry_name JVM_MUST_SUCCEED);
     if (found) {
       *(ReturnOop*)entry = parser().load_entry(JVM_SINGLE_ARG_NO_CHECK);
     }
   }
   if (CURRENT_HAS_PENDING_EXCEPTION) {
     *(ReturnOop*)entry = NULL;
     Thread::clear_current_pending_exception();
   }

   // This file may not be in the classpath and the MIDP code may try to 
   // delete this file, so let's not keep it in cache. See CR 6357720.
   //
   // IMPL_NOTE: we should add an API to flush a specific cached JarFile.
   JarFileParser::flush_caches();
}

extern "C"
jboolean Jvm_inflate(void *data, JvmGetByteProc getByteProc,
                     int compLen, unsigned char** outFileH,
                     int decompLen)
{  
  SETUP_ERROR_CHECKER_ARG;
  UsingFastOops fast_oops;

  Inflater::Fast inflater = Inflater::allocate(NULL, 0, decompLen, compLen, 0,
                                               0 JVM_NO_CHECK);
  if (!CURRENT_HAS_PENDING_EXCEPTION) {
    unsigned char* in_data = ARRAY_BASE(inflater().in_buffer());
    for (int i = 0; i < compLen; i++) {
      *in_data++ = getByteProc(data);
    }
    
    Buffer::Raw result = inflater().read_completely(JVM_SINGLE_ARG_NO_CHECK);
    if (result.not_null() && !CURRENT_HAS_PENDING_EXCEPTION) {
      jvm_memcpy(*outFileH, result().base_address(), decompLen);
      return KNI_TRUE;
    }
  }
  
  Thread::clear_current_pending_exception();
  return KNI_FALSE;
}

#if NOT_CURRENTLY_USED
extern "C" jboolean 
JVM_IterateJarEntries(const JvmPathChar* jar_file_name,
                      JvmNameFilterProc entry_filter_proc,
                      JvmDoJarEntryProc do_jar_entry_proc,
                      JvmAllocProc alloc_proc, JvmFreeProc free_proc,
                      void* caller_data) {
  SETUP_ERROR_CHECKER_ARG;

  if (alloc_proc == NULL) {
    if (free_proc != NULL) {
      return KNI_FALSE;
    } else {
      alloc_proc = (JvmAllocProc)&OsMemory_allocate;
      free_proc = (JvmFreeProc)&OsMemory_free;
    }
  }

  if (free_proc == NULL) {
    return KNI_FALSE;
  }

  JarFile jf(alloc_proc, free_proc);
  jf.open(jar_file_name, false, false JVM_CHECK_0);
  if (!jf.is_open()) {
    return KNI_FALSE;
  }

  bool success = jf.filtered_do_entries(entry_filter_proc, do_jar_entry_proc,
                                        caller_data JVM_CHECK_0);

  return CAST_TO_JBOOLEAN(success);
}
#endif // NOT_CURRENTLY_USED

#endif // ENABLE_JAR_READER_EXPORTS
