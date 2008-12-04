/*
 * @(#)sysShmem.h	1.9 06/10/10
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
 *
 */
#ifndef _JAVASOFT_SYSSHMEM_H

#include <jni.h>
#include "sys.h"
#include "shmem_md.h"

int sysSharedMemCreate(const char *name, int length, sys_shmem_t *, void **buffer);
int sysSharedMemOpen(const char *name,  sys_shmem_t *, void **buffer);
int sysSharedMemClose(sys_shmem_t, void *buffer);

/* Mutexes that can be used for inter-process communication */
int sysIPMutexCreate(const char *name, sys_ipmutex_t *mutex);
int sysIPMutexOpen(const char *name, sys_ipmutex_t *mutex);
int sysIPMutexEnter(sys_ipmutex_t mutex);
int sysIPMutexExit(sys_ipmutex_t mutex);
int sysIPMutexClose(sys_ipmutex_t mutex);

/* Inter-process events */
int sysEventCreate(const char *name, sys_event_t *event);
int sysEventOpen(const char *name, sys_event_t *event);
int sysEventWait(sys_process_t otherProcess, sys_event_t event);
int sysEventSignal(sys_event_t event);
int sysEventClose(sys_event_t event);

jlong sysProcessGetID();
int sysProcessOpen(jlong processID, sys_process_t *process);
int sysProcessClose(sys_process_t *process);
#endif


