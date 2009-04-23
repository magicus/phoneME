/*
 * @(#)shmemBase.h	1.9 06/10/10
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
#include "transportSPI.h"

#ifndef JAVASOFT_SHMEMBASE_H
#define JAVASOFT_SHMEMBASE_H

void exitTransportWithError(char *msg, char *fileName, 
                            char *date, int lineNumber);

typedef struct SharedMemoryConnection SharedMemoryConnection;
typedef struct SharedMemoryTransport SharedMemoryTransport;

typedef void * (*SharedMemAllocFunc)(jint);
typedef void  (*SharedMemFreeFunc)(void);

jint shmemBase_initialize(JavaVM *, TransportCallback *callback);
jint shmemBase_listen(const char *address, SharedMemoryTransport **);
jint shmemBase_accept(SharedMemoryTransport *, SharedMemoryConnection **);
jint shmemBase_attach(const char *addressString, SharedMemoryConnection **);
void shmemBase_closeConnection(SharedMemoryConnection *);
void shmemBase_closeTransport(SharedMemoryTransport *);
jint shmemBase_sendByte(SharedMemoryConnection *, jbyte data);
jint shmemBase_receiveByte(SharedMemoryConnection *, jbyte *data);
jint shmemBase_sendPacket(SharedMemoryConnection *, struct Packet *packet);
jint shmemBase_receivePacket(SharedMemoryConnection *, struct Packet *packet);
jint shmemBase_name(SharedMemoryTransport *, char **name);

#ifdef DEBUG
#define SHMEM_ASSERT(expression)  \
do {                            \
    if (!(expression)) {		\
        exitTransportWithError("assertion failed", __FILE__, __DATE__, __LINE__); \
    } \
} while (0)
#else
#define SHMEM_ASSERT(expression) ((void) 0)
#endif

#endif








