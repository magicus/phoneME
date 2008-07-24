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
 * Stub Implementation for pcsl_datagram.h 
 *
 */

#include <pcsl_network.h>
#include <pcsl_memory.h>
#include <javacall_network.h>
#include <javacall_datagram.h>

extern int javacall_to_pcsl_result( javacall_result res );

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_open_start(
    int port,
    void **pHandle,
    void **pContext)
{
  int res;  
  *pContext = NULL;
  res =  (int)javacall_datagram_open(port,(javacall_handle *)pHandle);
  
  return javacall_to_pcsl_result(res);
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_open_finish(
    void *handle,
    void *context)
{
  return PCSL_NET_IOERROR;
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_read_start(
    void *handle,
    unsigned char *pAddress,
    int *port,
    char *buffer,
    int length,
    int *pBytesRead,
    void **pContext)
{
  int res;
  res =  (int)javacall_datagram_recvfrom_start((javacall_handle) handle,pAddress,
        port,buffer,length,pBytesRead,pContext);

  return javacall_to_pcsl_result(res);

}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_read_finish(
    void *handle,
    unsigned char *pAddress,
    int *port,
    char *buffer,
    int length,
    int *pBytesRead,
    void *context)
{
  int res;
  res= (int)javacall_datagram_recvfrom_finish((javacall_handle) handle,pAddress,
        port,buffer,length,pBytesRead,context);

  return javacall_to_pcsl_result(res);
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_write_start(
    void *handle,
    unsigned char *pAddress,
    int port,
    char *buffer,
    int length,
    int *pBytesWritten,
    void **pContext)
{
    int res;
    res = (int)javacall_datagram_sendto_start((javacall_handle) handle,pAddress,
        port,buffer,length,pBytesWritten,pContext);
        
    return javacall_to_pcsl_result(res);
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_write_finish(
    void *handle,
    unsigned char *pAddress,
    int port,
    char *buffer,
    int length,
    int *pBytesWritten,
    void *context)
{
    int res;
    res = (int)javacall_datagram_sendto_finish((javacall_handle) handle,pAddress,
        port,buffer,length,pBytesWritten,context);

    return javacall_to_pcsl_result(res);
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_close_start(
    void *handle,
    void **pContext)
{
    int res;
    pContext = NULL;
    res = (int)javacall_datagram_close((javacall_handle) handle);

    return javacall_to_pcsl_result(res);
}

/**
 * See pcsl_datagram.h for definition.
 */
int pcsl_datagram_close_finish(
    void *handle,
    void *context)
{
    return PCSL_NET_IOERROR;
}

