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

#ifndef _JSR135_JUMPDRIVER_IMPL_H
#define _JSR135_JUMPDRIVER_IMPL_H

#ifdef __cplusplus
extern "C" {
#endif

#define MM_SHMEM_SIZE   10*1024
    
#define MAX_SUPPORTED_ISOLATES 10
typedef struct {
    int isolateId;
    CVMSharedMemory shMem;
    int active;
    char *memPtr;
    int *writePtr;
    int *readPtr;
    int *playSize;
    int size;
}JSR135TunnelDescr;

#ifdef __cplusplus
}
#endif

int jsr135_open_tunnel(int isolateId);
int jsr135_close_tunnel(int isolateId);
int jsr135_get_pcmctl(int *channels, int* bits, int* rate);
int jsr135_mixer_start(int isolateId);
int jsr135_mixer_stop(int isolateId);

#endif /* #ifdef _JSR135_JUMPDRIVER_IMPL_H */
