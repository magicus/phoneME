/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 * This source file is specific for Qt-based configurations.
 */

/**
 * @file
 * 
 * This file implements the notification adapter functions for a 
 * Linux-QTE platform
 */

#include <pcsl_network_na.h>
#include <pcsl_network_qte.h>
#include <iostream> /* <iostream.h> is unstandard and deprecated */

#ifdef __cplusplus
extern "C" {
#endif

extern void NotifySocketStatusChanged(long handle, int waitingFor);

/**
 * See pcsl_network_na.h for definition.
 */
void* na_create(int fd) {
    void *handle;
    
    handle = (void *)createSocketHandle(fd);
    return handle;
}

/**
 * See pcsl_network_na.h for definition.
 */
int na_get_fd(void *handle) {
    return getRawSocketFD(handle);
}

/**
 * See pcsl_network_na.h for definition.
 */
void na_register_for_read(void *handle) {
    register_vm_socket_read(handle);
}

/**
 * See pcsl_network_na.h for definition.
 */
void na_unregister_for_read(void *handle) {
    Unregister_vm_socket_read(handle);
}

/**
 * See pcsl_network_na.h for definition.
 */
void na_register_for_write(void *handle) {
    register_vm_socket_write(handle);
}

/**
 * See pcsl_network_na.h for definition.
 */
void na_unregister_for_write(void *handle) {
    Unregister_vm_socket_write(handle);
}

/**
 * See pcsl_network_na.h for definition.
 */
int na_get_status(void *handle) {
    pcsl_vmSocket_status vmStatus;
    
    vmStatus = getVMSocketHandleStatus(handle);
    if (vmStatus == SOCKET_STATUS_INTERRUPTED) {
        return PCSL_NET_INTERRUPTED;
        //return -1;
    }

    return PCSL_NET_SUCCESS;
}


/**
 * See pcsl_network_na.h for definition.
 */
void na_destroy(void *handle) {

    /*
     * Throw an interrupted IO Exception if this socket is already blocked
     * for read or write operation by any other thread
     */
    if (((VMSocket*)handle)->readerCount > 0) {
        setVMSocketHandleStatus(handle, SOCKET_STATUS_INTERRUPTED);
        /* Make an up-call to unblock the thread */
        NotifySocketStatusChanged((long)handle, SD_RECV);
    }
    if (((VMSocket*)handle)->writerCount > 0) {
        setVMSocketHandleStatus(handle, SOCKET_STATUS_INTERRUPTED);
        /* Make an up-call to unblock the thread */
        NotifySocketStatusChanged((long)handle, SD_SEND);
    }

    destroySocketHandle(handle);
}

#ifdef __cplusplus
}
#endif

