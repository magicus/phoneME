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

#include <javacall_defs.h>
#include <javacall_socket.h>

extern int javacall_to_pcsl_result( javacall_result res );


/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_open(
    int port,
    void **pHandle)
{
    javacall_result res;
    void *Context;

    res = javacall_server_socket_open_start(port, pHandle, &Context);

    while (res == JAVACALL_WOULD_BLOCK)
      res = javacall_server_socket_open_finish(pHandle, &Context);
    return javacall_to_pcsl_result (res);
}

/**
 * See pcsl_serversocket.h for definition.
 */
int
pcsl_server_socket_open_start(int port, void **pHandle, void **pContext) {
    javacall_result res;

    res = javacall_server_socket_open_start(port, pHandle, pContext);

    return javacall_to_pcsl_result(res);
}


/**
 * See pcsl_serversocket.h for definition.
 */
int
pcsl_server_socket_open_finish(void **handle, void **context) {
    javacall_result res;

    res = javacall_server_socket_open_finish(handle, context);

    return javacall_to_pcsl_result(res);
}

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_accept_start(
    void *handle,
    void **pConnectionHandle,
    void **pContext)
{
  (void)pContext;
  return javacall_to_pcsl_result (javacall_server_socket_accept_start (handle, pConnectionHandle));
}

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_accept_finish(
    void *handle,
    void **pConnectionHandle,
    void **pContext)
{
  (void)pContext;
  return javacall_to_pcsl_result (javacall_server_socket_accept_finish (handle, pConnectionHandle));
}

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_sereversocket_close_start(
    void *handle,
    void **pContext)
{
  return javacall_to_pcsl_result (javacall_socket_close_start (handle, pContext));
}


/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_close_finish(
        void *handle,
        void *context)
{
  return javacall_to_pcsl_result (javacall_socket_close_finish (handle, context));
}
