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

/** \file OsFile_rmfs.cpp
 *
 */

#include "incls/_precompiled.incl"
#include "incls/_OsFile_rmfs.cpp.incl"

// #include <sys/stat.h>

extern "C" {

#if ENABLE_RMFS

#include "../utilities/rmfsAlloc.hpp"
#include "../utilities/rmfsApi.hpp"

OsFile_Handle OsFile_open(const char *filename, const char *mode) {
  int handle;

  handle = rmfsOpen(filename, 0, 0);

  if(handle >= 0) {
    return &fileTab[handle];
  } else {
    return NULL;
  }
	  
}

int OsFile_close(OsFile_Handle handle) {

  _rmfsFileDes* file;


  file = (_rmfsFileDes*) handle;
  return rmfsClose((int)file->handle);
}

int OsFile_flush(OsFile_Handle handle) {
  // do nothing 
  // return jvm_fflush(handle);
  return 0;
}

size_t OsFile_read(OsFile_Handle handle,
                   void *buffer, size_t size, size_t count) {
  _rmfsFileDes* file;
  file = (_rmfsFileDes*) handle;


  return rmfsRead(file->handle, buffer, size*count);
}

size_t OsFile_write(OsFile_Handle handle,
                    const void *buffer, size_t size, size_t count) {
  _rmfsFileDes* file;
  file = (_rmfsFileDes*) handle;

  return rmfsWrite(file->handle, buffer, size*count);
}

long OsFile_length(OsFile_Handle handle) {

  long fileLength; 
  _rmfsFileDes* file;
  file = (_rmfsFileDes*) handle;

  fileLength =  (long)rmfsFileSize(file->handle);


  return fileLength;
  /*
  jvm_fseek(handle, 0, SEEK_END);
  long res = jvm_ftell(handle);
  jvm_fseek(handle, 0, SEEK_SET);// reset file
  return res;
  */
}

bool OsFile_exists(const char *name) {
  int status;

  rmfsFileExist(name, &status);

  if(status > 0) {
	  return true;
  } 

  return false;
  /*
  struct stat buf;
  if (jvm_stat(name, &buf) == 0) {
    if (S_ISREG(buf.st_mode)) {
       return true;
    }
  }
  return false;
  */
}

long OsFile_seek(OsFile_Handle handle, long offset, int origin) {
  _rmfsFileDes* file;
  file = (_rmfsFileDes*) handle;


  return rmfsLseek(file->handle, offset, origin);
}

int OsFile_error(OsFile_Handle handle) {
  _rmfsFileDes*  file;
  file = (_rmfsFileDes*) handle;


  return 0;
  // do nothing
  // return jvm_ferror(handle);
}

int OsFile_eof(OsFile_Handle handle) {
  _rmfsFileDes* file;
  file = (_rmfsFileDes*) handle;

  return rmfsFileEof(file->handle);
}

bool OsFile_rename(const char *from, const char *to) {
  rmfsRename(from, to);

  return true;
}

#else

typedef struct {
    const char * name;
    const unsigned char * data;
    int length;
} file_info;

extern file_info fs_image_table[];

struct OsFile {
    int index;                    // index in fs_image_table
    const unsigned char * data;   // start of data
    int length;                   // length of data
    int pos;                      // current position
};

static OsFile file_handles[10];

OsFile_Handle OsFile_open(const PathChar *fn_filename, const char *mode) {
  int i;

  for (i=0; ; i++) {
    if (fs_image_table[i].name == NULL) {
      // reached end of table -- file not found
      break;
    }
    if (strcmp(fn_filename, fs_image_table[i].name) == 0) {
      for (int j=0; j<ARRAY_SIZE(file_handles); j++) {
        if (file_handles[j].data == NULL) {
          file_handles[j].index = i;
          file_handles[j].data = fs_image_table[i].data;
          file_handles[j].length = fs_image_table[i].length;
          file_handles[j].pos = 0;
          return &file_handles[j];
        }
      }
      // out of OsFile handles
      break;
    }
  }

  return NULL;
}

int OsFile_close(OsFile_Handle handle) {
  handle->data = NULL;
  return 0;
}

int OsFile_flush(OsFile_Handle handle) {
  // nothing to do
  return 0;
}

size_t OsFile_read(OsFile_Handle handle,
                   void *buffer, size_t size, size_t count) {
  size_t done = 0;
  size_t req = size * count;
  char * p = (char*)buffer;

  while ((handle->pos < handle->length) && (done < req)) {
     *p = handle->data[handle->pos];
     p++;
     handle->pos ++;
     done ++;
  }

  return done;
}

size_t OsFile_write(OsFile_Handle handle,
                    const void *buffer, size_t size, size_t count) {
  UNIMPLEMENTED();
  return 0;
}

long OsFile_length(OsFile_Handle handle) {
  return handle->length;
}

bool OsFile_exists(const PathChar *fn_filename) {
  if (strcmp(fn_filename, "/fs")) {
    return true;
  }

  int i;
  for (i=0; ; i++) {
    if (fs_image_table[i].name == NULL) {
      // reached end of table -- file not found
      break;
    }
    if (strcmp(fn_filename, fs_image_table[i].name) == 0) {
      return true;
    }
  }
  return false;
}

long OsFile_seek(OsFile_Handle handle, long offset, int origin) {
  switch (origin) {
  case SEEK_CUR:
    handle->pos += offset;
    break;
  case SEEK_SET:
    handle->pos = offset;
    break;
  case SEEK_END:
    handle->pos = handle->length + offset;
    break;
  default:
    return -1;
  }

  if (handle->pos < 0) {
    handle->pos = 0;
  } else if (handle->pos > handle->length) {
    handle->pos = handle->length;
  }

  return 0;
}

int OsFile_error(OsFile_Handle handle) {
  return 0;
}

int OsFile_eof(OsFile_Handle handle) {
  return (handle->pos >= handle->length);
}

bool OsFile_rename(char *from, char *to) {
  UNIMPLEMENTED();
  return 0;
}

#endif
} // extern "C"
