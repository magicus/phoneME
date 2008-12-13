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

/** \file OsFile_ads.cpp
 *
 */

#include "incls/_precompiled.incl"
#include "incls/_OsFile_ads.cpp.incl"

#ifdef __cplusplus
extern "C" {
#include <stdio.h>
#endif

#if !ENABLE_PCSL

OsFile_Handle OsFile_open(const char *filename, const char *mode) {
  armulator_simulate_memory_change(jvm_strlen(filename), 10 * FileSpeed);
  return fopen(filename, mode);
}

int OsFile_close(OsFile_Handle handle) {
  return fclose(handle);
}

int OsFile_flush(OsFile_Handle handle) {
  return fflush(handle);
}

size_t OsFile_read(OsFile_Handle handle,
                   void *buffer, size_t size, size_t count) {
  armulator_simulate_memory_transfer((juint)size, FileSpeed);
  return fread(buffer, size, count, handle);
}

size_t OsFile_write(OsFile_Handle handle,
                    const void *buffer, size_t size, size_t count) {
  armulator_simulate_memory_transfer((juint)size, FileSpeed);
  return fwrite(buffer, size, count, handle);
}

long OsFile_length(OsFile_Handle handle) {
  fseek(handle, 0, SEEK_END);
  long res = ftell(handle);
  fseek(handle, 0, SEEK_SET);// reset file
  return res;
}

bool OsFile_exists(const char *name) {
  armulator_simulate_memory_change(jvm_strlen(name), 10 * FileSpeed);

  OsFile_Handle handle = NULL;

  // in special case of directories we always return false
  if (name && name[strlen(name)-1] == '/') {
     return false;
  }

  if ((handle = fopen(name,"r"))!=0) {
     fclose(handle);
     return true;
  }
  else {
     return false;
  }
}

long OsFile_seek(OsFile_Handle handle, long offset, int origin) {
  return fseek(handle, offset, origin);
}

int OsFile_error(OsFile_Handle handle) {
  return ferror(handle);
}

int OsFile_eof(OsFile_Handle handle) {
  return feof(handle);
}

bool OsFile_rename(const char *from, const char *to) {
  rename(from, to);
  return true;
}

int OsFile_remove(const char *filename) {
  return remove(filename);
}

#endif // !ENABLE_PCSL

#ifdef __cplusplus
}
#endif
