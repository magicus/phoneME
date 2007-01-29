/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _LINUX_SHARED_MEMORY_MD_H
#define _LINUX_SHARED_MEMORY_MD_H

#include <sys/types.h>

typedef struct{
  char* full_name; 
  key_t key;
  int shmid;
  int semid;
  char* dataBuffer;
} SharedMemory_md;

int sharedMemory_create (SharedMemory_md* sb, char* name, int size);
int sharedMemory_open(SharedMemory_md* sb, char *name);
int sharedMemory_getSize(SharedMemory_md *sb);
char *sharedMemory_getAddress(SharedMemory_md *sb);
void sharedMemory_close(SharedMemory_md *sb);
void sharedMemory_destroy(SharedMemory_md *sb);
void sharedMemory_lock(SharedMemory_md *sb);
void sharedMemory_unlock(SharedMemory_md *sb);

#define CVMassert(flg) if(!flg) printf("ERROR\n")
#endif /* LINUX_SHARED_MEMORY_MD_H */

