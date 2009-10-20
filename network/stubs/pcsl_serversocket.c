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
 */

/**
 * @file
 *
 * Stub Implementation for pcsl_serversocket.h 
 *
 */

#include <pcsl_network.h>
#include <pcsl_serversocket.h>

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_open(
    int port,
    void **pHandle)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_serversocket.h for definition.
 */
int
pcsl_server_socket_open_start(int port, void **pHandle, void **pContext) {
    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_serversocket.h for definition.
 */
int
pcsl_server_socket_open_finish(void **handle, void **context) {
    return PCSL_NET_IOERROR;
}
    
/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_accept_start(
    void *handle,
    void **pConnectionHandle,
    void **pContext)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_accept_finish(
    void *handle,
    void **pConnectionHandle,
    void **pContext)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_sereversocket_close_start(
    void *handle,
    void **pContext)
{
    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_close_finish(
	void *handle,
	void *context)
{
    return PCSL_NET_IOERROR;
}
