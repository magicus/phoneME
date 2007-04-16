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

#include <pcsl_network.h>

#include <javacall_network.h>
#include <javacall_socket.h>

int javacall_to_pcsl_result( javacall_result res ){
switch (res) {
  case JAVACALL_OK:
      return PCSL_NET_SUCCESS;
  case JAVACALL_FAIL:
      return PCSL_NET_IOERROR;
  case JAVACALL_WOULD_BLOCK:
      return PCSL_NET_WOULDBLOCK;
  case JAVACALL_CONNECTION_NOT_FOUND:
      return PCSL_NET_CONNECTION_NOTFOUND;
  case JAVACALL_INTERRUPTED:
      return PCSL_NET_INTERRUPTED;
/*   case JAVACALL_NO_DATA_AVAILABLE: */
/*       return PCSL_NET_NO_DATA_AVAILABLE; */
  case JAVACALL_INVALID_ARGUMENT:
      return PCSL_NET_INVALID;
  }
  return PCSL_NET_IOERROR;
}


/**
 * See pcsl_network.h for definition.
 */
int
pcsl_socket_open_start(unsigned char *ipBytes, int port, void **pHandle, void **pContext) {
    javacall_result res;

    res = javacall_socket_open_start(ipBytes, port, pHandle, pContext);

    return javacall_to_pcsl_result(res);
}


/**
 * See pcsl_network.h for definition.
 */
int
pcsl_socket_open_finish(void *handle, void *context) {
    javacall_result res;

    res = javacall_socket_open_finish(handle, context);

    return javacall_to_pcsl_result(res);
}



/**
 * See pcsl_network.h for definition.
 */
int
pcsl_socket_read_start(void *handle,
    unsigned char *pData, int len, int *pBytesRead, void **pContext) {
    javacall_result res;

    res = javacall_socket_read_start(handle, pData, len, pBytesRead, pContext);

    return javacall_to_pcsl_result(res);
}


/**
 * See pcsl_network.h for definition.
 */
int
pcsl_socket_read_finish(void *handle,
    unsigned char *pData, int len, int *pBytesRead, void *context) {
    javacall_result res;

    res = javacall_socket_read_finish(handle, pData, len, pBytesRead, context);

    return javacall_to_pcsl_result(res);
}


/**
 * See pcsl_network.h for definition.
 */
int
pcsl_socket_write_start(void *handle,
    char *pData, int len, int *pBytesWritten, void **pContext) {
    javacall_result res;

    res = javacall_socket_write_start(handle, pData, len, pBytesWritten, pContext);

    return javacall_to_pcsl_result(res);
}


/**
 * See pcsl_network.h for definition.
 */
int
pcsl_socket_write_finish(void *handle,
    char *pData, int len, int *pBytesWritten, void *context) {
    javacall_result res;

    res = javacall_socket_write_finish(handle, pData, len, pBytesWritten, context);

    return javacall_to_pcsl_result(res);
}


/**
 * See pcsl_network.h for definition.
 */
int
pcsl_socket_available(void *handle, int *pBytesAvailable) {
    javacall_result res;

    res = javacall_socket_available(handle, pBytesAvailable);

    return javacall_to_pcsl_result(res);
}

/**
 * See pcsl_network.h for definition.
 */
int
pcsl_socket_shutdown_output(void *handle) {
    javacall_result res;

    res = javacall_socket_shutdown_output(handle);

    return javacall_to_pcsl_result(res);
}

/**
 * See pcsl_network.h for definition.
 *
 * Note that this function NEVER returns PCSL_NET_WOULDBLOCK. Therefore, the 
 * finish() function should never be called and does nothing.
 */
int
pcsl_socket_close_start(void *handle, void **pContext) {
    javacall_result res;

    res = javacall_socket_close_start(handle, pContext);

    return javacall_to_pcsl_result(res);
}


/**
 * See pcsl_network.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function should never be called.
 */
int
pcsl_socket_close_finish(void *handle, void *context) {
    javacall_result res;

    res = javacall_socket_close_finish(handle, context);

    return javacall_to_pcsl_result(res);
}

/**
 * See pcsl_network.h for definition.
 */
int
pcsl_socket_getlocaladdr(void *handle, char *pAddress) {
    javacall_result res;

    res = javacall_socket_getlocaladdr((javacall_handle)handle,pAddress);

    return javacall_to_pcsl_result(res);
}



/**
 * See pcsl_network.h for definition.
 */
int
pcsl_socket_getremoteaddr(void *handle, char *pAddress) {
    javacall_result res;

    res = javacall_socket_getremoteaddr(handle,pAddress);

    return javacall_to_pcsl_result(res);
}


/**
 * See pcsl_network.h for definition.
 */
int
pcsl_network_getlocalport(void *handle, int *pPortNumber) {
    javacall_result res;

    res = javacall_socket_getlocalport(handle,pPortNumber);

    return javacall_to_pcsl_result(res);
}


/**
 * See pcsl_network.h for definition.
 */
int
pcsl_network_getremoteport(void *handle, int *pPortNumber) {
    javacall_result res;

    res = javacall_socket_getremoteport(handle,pPortNumber);

    return javacall_to_pcsl_result(res);
}


