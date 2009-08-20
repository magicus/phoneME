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

#include <malloc.h>
#include <shared_memory.h>
#include <shared_memory_md.h>

/* 
 * create a new sharedBuffer - allocate new memory for the name using strdup
 */
CVMSharedMemory CVMsharedMemCreate(char* name, int size){
    SharedMemory_md *sm;
    CVMassert(name != NULL); 
    CVMassert(size > 0);
    
    sm = (SharedMemory_md *) malloc (sizeof (SharedMemory_md));
    if (sm == NULL) { 
        return NULL; 
    }    
    if (sharedMemory_create(sm, name, size) != 0){
        sharedMemory_destroy(sm);
        return NULL; 
    }
    
    return (CVMSharedMemory)sm; 
}

/*
 * open an existing shared buffer object with a given name using strdup 
 */
CVMSharedMemory CVMsharedMemOpen(char* name){
    SharedMemory_md *sm;
    
    CVMassert(name != NULL); 
    sm = (SharedMemory_md *) malloc (sizeof (SharedMemory_md));
    if (sm == NULL) { 
        return NULL; 
    }    
    if (sharedMemory_open(sm, name) != 0){
        sharedMemory_close(sm);
        return NULL; 
    }
    
    return (CVMSharedMemory)sm; 
}

/* Purpose: Get total size of shared memory
   Returns: Size of shared memory
*/
int CVMsharedMemGetSize(CVMSharedMemory hdlr) {
    CVMassert(hdlr != NULL); 
    return sharedMemory_getSize((SharedMemory_md *)hdlr);
}

/* Purpose: Get starting address of Shared Memory
   Returns: starting address of Shared Memory
*/
char *CVMsharedMemGetAddress(CVMSharedMemory hdlr) {
    CVMassert(hdlr != NULL); 
    return sharedMemory_getAddress((SharedMemory_md *)hdlr);
}

/* Purpose: Close shared memory. 
   Returns: none
*/
void CVMsharedMemClose(CVMSharedMemory hdlr) {
    CVMassert(hdlr != NULL); 
    sharedMemory_close((SharedMemory_md *)hdlr);
}

/* Purpose: Close shared memory. 
   Returns: none
*/
void CVMsharedMemDestroy(CVMSharedMemory hdlr) {
    CVMassert(hdlr != NULL); 
    sharedMemory_destroy((SharedMemory_md *)hdlr);
}

/* Purpose: Lock shared memory access. 
   Returns: none
*/
void CVMsharedMemLock(CVMSharedMemory hdlr) {
    CVMassert(hdlr != NULL); 
    sharedMemory_lock((SharedMemory_md *)hdlr);
}

/* Purpose: Unlock shared memory access. 
   Returns: none
*/
void CVMsharedMemUnlock(CVMSharedMemory hdlr) {
    CVMassert(hdlr != NULL); 
    sharedMemory_unlock((SharedMemory_md *)hdlr);
}
