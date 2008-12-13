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

/*
 * OsFile.cpp: Generic OsFile implementation for all platforms, using PCSL
 *             (Portable Common Services Library).
 */

#include "incls/_precompiled.incl"
#include "incls/_OsFile.cpp.incl"

#if ENABLE_PCSL

#if defined(USE_UNICODE_FOR_FILENAMES) && !USE_UNICODE_FOR_FILENAMES
#error "USE_UNICODE_FOR_FILENAMES must be set if ENABLE_PCSL is set"
#endif

extern "C" {
#include "pcsl_file.h"
#include "pcsl_memory.h"
}

#ifndef PCSL_FILE_MAX_NAME_LEN
#define PCSL_FILE_MAX_NAME_LEN 256
#endif

OsFile_Handle OsFile_open(const JvmPathChar *filename, const char *mode) {
  int name_len = fn_strlen(filename);
  pcsl_string pcsl_filename = PCSL_STRING_NULL;

  GUARANTEE(sizeof(jchar) == sizeof(JvmPathChar), "Types must match");

  if (pcsl_string_convert_from_utf16(filename, 
                                     name_len, 
                                     &pcsl_filename) != PCSL_STRING_OK) {
    return NULL;
  }

/*
  int pcsl_flags = (*mode == 'w') ? (PCSL_FILE_O_CREAT|PCSL_FILE_O_WRONLY|
                                     PCSL_FILE_O_TRUNC) :
                                    (PCSL_FILE_O_RDONLY);
*/

  int pcsl_flags;
  if(*mode == 'w') { 
    pcsl_flags = (PCSL_FILE_O_CREAT | PCSL_FILE_O_WRONLY | PCSL_FILE_O_TRUNC);
  } else if(*mode == 'a') {
    pcsl_flags = (PCSL_FILE_O_WRONLY | PCSL_FILE_O_APPEND);
  } else {
    pcsl_flags = PCSL_FILE_O_RDONLY;
  }

  void *pcsl_handle;
  if (pcsl_file_open(&pcsl_filename, pcsl_flags, &pcsl_handle) == 0) {
    pcsl_string_free(&pcsl_filename);
    OsFile_Handle handle =
      (OsFile_Handle)pcsl_mem_malloc(sizeof(struct _OsFile_Handle));
    if (handle) {
      handle->pcsl_handle = pcsl_handle;
    } else {
      pcsl_file_close(pcsl_handle);
    }
    return handle;
  } else {
    pcsl_string_free(&pcsl_filename);
    return NULL;
  }
}

int OsFile_close(OsFile_Handle handle) {
  int code = pcsl_file_close(handle->pcsl_handle);
  pcsl_mem_free((void*)handle);
  return code;
}

int OsFile_flush(OsFile_Handle /*handle*/) {
  // PCSL does not provide a flush() api, so we assume it's non-buffered.
  return 0;
}

size_t OsFile_read(OsFile_Handle handle,
                   void *buffer, size_t size, size_t count) {
  return (pcsl_file_read(handle->pcsl_handle, (unsigned char*)buffer,
                         (long)(size*count))) / size;
}

size_t OsFile_write(OsFile_Handle handle,
                    const void *buffer, size_t size, size_t count) {
  return (pcsl_file_write(handle->pcsl_handle, (unsigned char*)buffer, 
                          (long)(size*count))) / size;
}

long OsFile_length(OsFile_Handle handle) {
  return pcsl_file_sizeofopenfile(handle->pcsl_handle);
}

// Disabled temporarily - pcsl_file_exist does not distinguish between
// directory and "file".
bool OsFile_exists(const JvmPathChar *filename) {
  int name_len = fn_strlen(filename);
  pcsl_string pcsl_filename = PCSL_STRING_NULL;

  GUARANTEE(sizeof(jchar) == sizeof(JvmPathChar), "Types must match");

  if (pcsl_string_convert_from_utf16(filename, 
                                     name_len, 
                                     &pcsl_filename) != PCSL_STRING_OK) {
    return false;
  }

  jboolean result = pcsl_file_exist(&pcsl_filename);
  pcsl_string_free(&pcsl_filename);

  return result;
}

long OsFile_seek(OsFile_Handle handle, long offset, int origin) {
  switch (origin) {
  case SEEK_END: origin = PCSL_FILE_SEEK_END; break;
  case SEEK_SET: origin = PCSL_FILE_SEEK_SET; break;
  case SEEK_CUR: origin = PCSL_FILE_SEEK_CUR; break;
  }
  return pcsl_file_seek(handle->pcsl_handle, offset, origin);
}

bool OsFile_rename(const JvmPathChar *from, const JvmPathChar *to) {
  const int from_name_len = fn_strlen(from);
  const int to_name_len = fn_strlen(to);
  pcsl_string pcsl_filename_from = PCSL_STRING_NULL;
  pcsl_string pcsl_filename_to = PCSL_STRING_NULL;

  GUARANTEE(sizeof(jchar) == sizeof(JvmPathChar), "Types must match");

  if (pcsl_string_convert_from_utf16(from, 
                                     from_name_len, 
                                     &pcsl_filename_from) != PCSL_STRING_OK) {
    return -1;
  }

  if (pcsl_string_convert_from_utf16(to, 
                                     to_name_len, 
                                     &pcsl_filename_to) != PCSL_STRING_OK) {
    return -1;
  }

  int result = pcsl_file_rename(&pcsl_filename_from, &pcsl_filename_to);

  pcsl_string_free(&pcsl_filename_from);
  pcsl_string_free(&pcsl_filename_to);

  return (result == 0) ? true : false;
}

int OsFile_remove(const JvmPathChar * filename) {
  int name_len = fn_strlen(filename);
  pcsl_string pcsl_filename = PCSL_STRING_NULL;

  GUARANTEE(sizeof(jchar) == sizeof(JvmPathChar), "Types must match");

  if (pcsl_string_convert_from_utf16(filename, 
                                     name_len, 
                                     &pcsl_filename) != PCSL_STRING_OK) {
    return -1;
  }

  int result = pcsl_file_unlink(&pcsl_filename);

  pcsl_string_free(&pcsl_filename);

  return result;
}

#endif // ENABLE_PCSL
