/*
 * @(#)shmemBack.c	1.11 06/10/10
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
#include <string.h>

#include "transportSPI.h"
#include "shmemBase.h"
#include "sys.h"

/*
 * Back-end transport implementation
 */
 
SharedMemoryTransport *transport = NULL;  /* maximum of 1 transport */ 
SharedMemoryConnection *connection = NULL;  /* maximum of 1 connection */ 
TransportCallback *callbacks;

static jint
shmemListen(char **address) 
{
    jint rc = shmemBase_listen(*address, &transport);
    /*
     * If a name was selected by the function above, find it and return
     * it in place of the original arg. 
     */
    if ((rc == SYS_OK) && ((*address == NULL) || (*address[0] == '\0'))) {
        char *name;
        char *name2;
        rc = shmemBase_name(transport, &name);
        if (rc == SYS_OK) {
            name2 = (callbacks->alloc)(strlen(name) + 1);
            if (name2 == NULL) {
                rc = SYS_ERR;
            } else {
                strcpy(name2, name);
                *address = name2;
            }
        }
    }
    return rc;
}

static jint
shmemAccept(void) 
{
    return shmemBase_accept(transport, &connection);
}

static jint
shmemStopListening(void) 
{
    shmemBase_closeTransport(transport);
    return SYS_OK;
}

static jint
shmemAttach(char *address) 
{
    return shmemBase_attach(address, &connection);
}

static jint
shmemSendByte(jbyte b) 
{
    return shmemBase_sendByte(connection, b);
}

static jint
shmemReceiveByte(jbyte *b) 
{
    return shmemBase_receiveByte(connection, b);
}

static jint
shmemSendPacket(struct Packet *packet) 
{
    return shmemBase_sendPacket(connection, packet);
}

static jint
shmemReceivePacket(struct Packet *packet) 
{
    return shmemBase_receivePacket(connection, packet);
}

static void
shmemClose(void) 
{
    shmemBase_closeConnection(connection);
}

struct Transport functions = {
    shmemListen,
    shmemAccept,
    shmemStopListening,
    shmemAttach,
    shmemSendByte,
    shmemReceiveByte,
    shmemSendPacket,
    shmemReceivePacket,
    shmemClose
};

JNIEXPORT jint JNICALL 
JDWP_OnLoad(JavaVM *vm, Transport **tablePtr, 
            TransportCallback *cbTablePtr, void *reserved) 
{
    jint rc = shmemBase_initialize(vm, cbTablePtr);
    if (rc != SYS_OK) {
        return rc;
    }
    *tablePtr = &functions;
    callbacks = cbTablePtr;
    return 0;
}


