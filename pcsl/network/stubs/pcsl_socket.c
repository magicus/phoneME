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
 * Stub Implementation for pcsl_socket.h 
 *
 */

#include <pcsl_network.h>

/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_open_start(
	unsigned char *ipBytes,
	int port,
	void **pHandle,
	void **pContext)
{
    return PCSL_NET_CONNECTION_NOTFOUND;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_open_finish(
	void *handle,
	void *context)
{
    return PCSL_NET_IOERROR;
}
      

/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_read_start(
	void *handle,
	unsigned char *pData,
	int len,  
	int *pBytesRead,
	void **pContext)
{

    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_read_finish(
	void *handle,
	unsigned char *pData,
	int len,
	int *pBytesRead,
	void *context)
{
    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_write_start(
	void *handle,
	char *pData,
	int len,
	int *pBytesWritten,
	void **pContext)
{
    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_write_finish(
	void *handle,
	char *pData,
	int len,
	int *pBytesWritten,
	void *context)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 *
 * Note that this function NEVER returns PCSL_NET_WOULDBLOCK. Therefore, the 
 * finish() function should never be called and does nothing.
 */
int pcsl_socket_close_start(
    void *handle,
    void **pContext)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function should never be called.
 */
int pcsl_socket_close_finish(
	void *handle,
	void *context)
{
    return PCSL_NET_INVALID;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_available(void *handle, int *pBytesAvailable)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_shutdown_output(void *handle) {
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_getlocaladdr(
	void *handle,
	char *pAddress)
{
    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_getremoteaddr(
	void *handle,
	char *pAddress)
{
    return PCSL_NET_IOERROR;
}

