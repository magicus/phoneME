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

#ifndef PCSL_NETWORK_GENERIC
#define PCSL_NETWORK_GENERIC

#define CHECK_READ      0x01
#define CHECK_WRITE     0x02
#define CHECK_EXCEPTION 0x04

/** SocketHandle data structure stores details about blocking sockets */
typedef struct _SocketHandle {
  int fd;                       /* The socket that returned EWOULDBLOCK */
  int check_flags;              /* Should we check for read/write/exception? */
  int status;                   /* Status of the socket handle */
  struct _SocketHandle *next;   /* Next blocking socket instance */
} SocketHandle;

/** Get list of socket handles registered for read/write
 *  @return pointer to head of socket handles list or NULL if empty
 */
const SocketHandle* GetRegisteredSocketHandles();
 
#endif


