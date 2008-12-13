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

/** \file OsFile_linux.cpp
 *
 */

#include "incls/_precompiled.incl"
#include "incls/_OsFile_linux.cpp.incl"

#ifdef __cplusplus
extern "C" {
#endif

#if !ENABLE_PCSL
OsFile_Handle OsFile_open(const char *filename, const char *mode) {
  return (OsFile_Handle)jvm_fopen(filename, mode);
}

int OsFile_close(OsFile_Handle handle) {
  return jvm_fclose(handle);
}

int OsFile_flush(OsFile_Handle handle) {
  return jvm_fflush(handle);
}

size_t OsFile_read(OsFile_Handle handle,
                   void *buffer, size_t size, size_t count) {
  return jvm_fread(buffer, size, count, handle);
}

size_t OsFile_write(OsFile_Handle handle,
                    const void *buffer, size_t size, size_t count) {
  return jvm_fwrite(buffer, size, count, handle);
}

long OsFile_length(OsFile_Handle handle) {
  jvm_fseek(handle, 0, SEEK_END);
  long res = jvm_ftell(handle);
  jvm_fseek(handle, 0, SEEK_SET);// reset file
  return res;
}

bool OsFile_exists(const char *name) {
  struct stat buf;
  if (jvm_stat(name, &buf) == 0) {
    if (S_ISREG(buf.st_mode)) {
       return true;
    }
  }
  return false;
}

long OsFile_seek(OsFile_Handle handle, long offset, int origin) {
  return jvm_fseek(handle, offset, origin);
}

int OsFile_eof(OsFile_Handle handle) {
  return jvm_feof(handle);
}

bool OsFile_rename(const char *from, const char *to) {
  jvm_rename(from, to);
  return true;
}

int OsFile_remove(const char *filename) {
  return jvm_remove(filename);
}
#endif // !ENABLE_PCSL

#if USE_IMAGE_MAPPING

struct Linux_MappedImage : public OsFile_MappedImage {
  size_t  ro_length;
  address rw_mapped_address;
  size_t  rw_length;
};

#define ASSUMED_PAGE_SIZE 4096

address OsFile_ImagePreferredAddress(const JvmPathChar* jar_file_name) {
  // Change this if this range is usually not free on your port
  address base = (address)0x44000000;

#if ENABLE_ISOLATES
  // Use this much of virtual space above the base address for images
  int reserve_size = 0x04000000; // 64 MB.
  // usually an image is no larger than 512K (eembc is below 300K)
  int image_size = 512 * 1024;
  // The default setting above allow 128 simultaneuos images to be mapped
  // to their own desired address (if there's no conflict)
  int num_slots = reserve_size / image_size;

  juint hash = 0;
  while (*jar_file_name) {
    JvmPathChar c = *jar_file_name++;
    hash = 31 * hash + juint(c);
  }

  base += image_size * (hash % num_slots);

#else
  // In SVM mode we never load two images at the same time so we just always
  // use the same base address;
  (void)jar_file_name;
#endif

  return base;
}

OsFile_MappedImageHandle OsFile_MapImage(const PathChar* name,
                                         address preferrable_destination,
                                         int length, int rw_offset,
                                         int rw_length) {
  GUARANTEE(length == rw_offset + rw_length, "assumption");
  GUARANTEE((int(preferrable_destination) % ASSUMED_PAGE_SIZE) == 0,
            "page aligned");
#if USE_UNICODE_FOR_FILENAMES
  char ascii_name[256];
  int len = fn_strlen(name);
  if (len > 255) {
    len = 255;
  }
  for (int i=0; i<len; i++) {
    ascii_name[i] = (char)name[i];
  }
  ascii_name[len] = 0;
#else
  const char *ascii_name = name;
#endif
  Linux_MappedImage *img = 
      (Linux_MappedImage*)jvm_malloc(sizeof(Linux_MappedImage));

  if (img == NULL) {
    return NULL;
  }

  if (Verbose) {
    TTY_TRACE_CR(("Map image desired = 0x%x",int(preferrable_destination))); 
  }
  
  int open_flags = O_RDONLY;
  int fd = jvm_open(ascii_name, open_flags);

  // Align the RW region down so that the RW region starts at
  // a page boundary.
  int ro_length = length - rw_length;
  ro_length = (int)align_size_down(ro_length, ASSUMED_PAGE_SIZE);
  rw_offset = ro_length;
  rw_length = length - ro_length;

  if (ForceImageRelocation) {
    // For testing purposes, check relocations
    preferrable_destination += ASSUMED_PAGE_SIZE * 17;
  }

  address ro_addr = (address)-1;
  address rw_addr = (address)-1;
  address ro_preferred = preferrable_destination;
  address rw_preferred = preferrable_destination + rw_offset;

  if (fd == -1) {
    goto error;
  }


  if (ro_length > 0 && LoadXIPImage && !ForceImageRelocation) {
    ro_addr = (address)jvm_mmap(ro_preferred, ro_length, 
                                PROT_READ, MAP_PRIVATE, fd, 0);
    rw_addr = (address)jvm_mmap(rw_preferred, rw_length, 
                                PROT_READ | PROT_WRITE, MAP_PRIVATE, fd,
                                rw_offset);
  } else {
    // The whole image needs to be mapped R/W.
  }

  if (ro_addr == ro_preferred && rw_preferred == rw_preferred) {
    if (Verbose) {
      TTY_TRACE_CR(("Map image actual  = 0x%x [RO] size=%d", int(ro_addr),
                                                             ro_length));
      TTY_TRACE_CR(("Map image actual  = 0x%x [RW] size=%d", int(rw_addr),
                                                             rw_length));
    }
  
    img->mapped_address    = ro_addr;
    img->ro_length         = ro_length;
    img->rw_mapped_address = rw_addr;
    img->rw_length         = rw_length;
  } else {
    // Can't get to our preferred location. Relocation of the image content
    // is needed, so we need to remap the whole thing using RW mapping
    if (ro_addr != (address) -1) {
      jvm_munmap(ro_addr, ro_length);
    }
    if (rw_addr != (address) -1) {
      jvm_munmap(rw_addr, rw_length);
    }
    ro_addr = (address)jvm_mmap(ro_preferred, length,
                                PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (ro_addr == (address)-1) {
      goto error;
    }

    if (Verbose) {
      TTY_TRACE_CR(("Map image actual  = 0x%x [RW] size=%d", int(ro_addr),
                                                             length));
    }
  
    img->mapped_address    = ro_addr;
    img->ro_length         = length;
    img->rw_mapped_address = (address)-1;
    img->rw_length         = 0;
  }

  // we don't need fd anymore, mapping is still preserved
  jvm_close(fd);
  return img;

error:
  if (img) {
    jvm_free(img);
  }
  if (fd >= 0) {
    jvm_close(fd);
  }
  return NULL;
}

bool OsFile_UnmapImage(OsFile_MappedImageHandle mapped_image) {
  bool ok = true;
  Linux_MappedImage *img = (Linux_MappedImage*) mapped_image;

  if (jvm_munmap(img->mapped_address, img->ro_length) != 0) {
    ok = false;
  }
  if (img->rw_mapped_address != (address)-1 && 
      jvm_munmap(img->rw_mapped_address, img->rw_length) != 0) {
    ok = false;
  }
  jvm_free(mapped_image);

  return ok;
}

#endif // USE_IMAGE_MAPPING

#ifdef __cplusplus
}
#endif
