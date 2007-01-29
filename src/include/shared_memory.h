/*
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

#ifndef CVM_SHARED_MEMORY_H
#define CVM_SHARED_MEMORY_H

#define MAX_SHMEM_NAME_LEN 20
typedef void* CVMSharedMemory;

/* Purpose: Create shared memory segment . 
   Returns: Shared memory handler
*/
CVMSharedMemory CVMsharedMemCreate(char *name, int size);

/* Purpose: Open already created shared memory segment. 
   Returns: Shared memory handler
*/
CVMSharedMemory CVMsharedMemOpen(char *name);

/* Purpose: Get size of shared memory
   Returns: Size of shared memory
*/
int CVMsharedMemGetSize(CVMSharedMemory hdlr);

/* Purpose: Get starting data address of Shared Memory
   Returns: starting address of Shared Memory
*/
char *CVMsharedMemGetAddress(CVMSharedMemory hdlr);

/* Purpose: Close shared memory. 
   Returns: none
*/
void CVMsharedMemClose(CVMSharedMemory hdlr);

/* Purpose: Destroy shared memory segmant. 
   Returns: none
*/
void CVMsharedMemDestroy(CVMSharedMemory hdlr);

/* Purpose: Lock shared memory access. 
   Returns: none
*/
void CVMsharedMemLock(CVMSharedMemory hdlr);

/* Purpose: Unlock shared memory access. 
   Returns: none
*/
void CVMsharedMemUnlock(CVMSharedMemory hdlr);
#endif /* CVM_SHARED_MEMORY_H */
