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

#ifndef _PUSH_SERVER_RESOURCE_MGMT_H_
#define _PUSH_SERVER_RESOURCE_MGMT_H_

/**
 * @defgroup push_server Push Registry low-level implementation
 * @ingroup push
 */
 
/**
 * @file
 * @ingroup push_server
 * 
 * @brief Push server functionality for resource-providing libraries.
 *
 * ##include &lt;&gt;
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Defines for the state field of _pushentry and _alarmentry
 */
#define WAITING_DATA   (-6)
#define LAUNCH_PENDING (-5)
#define RECEIVED_EVENT (-4)
#define CHECKED_IN	   (-3)
#define CHECKED_OUT	   (-2)
#define AVAILABLE	   (-1)

/**
 * Returns the number of buffered bytes for the given socket.
 *
 * @param fd handle of the socket
 * @return number of cached bytes of -1 if none
 */
int pushcacheddatasize(int fd);

/**
 * Fetch the buffered datagram or TCP packet into a buffer.
 *
 * @param fd The handle of the socket from which the packet was received
 * @param ip The ip address of the incoming datagram
 * @param sndport The port from which the data was sent
 * @param buf A pointer to a buffer into which the data should be copied
 * @param len The size of buf
 * @return the length of the returned data if successful, or <tt>-1</tt>
 *         if unsuccessful.
 */
int pushgetcachedpacket(int fd, int* ip, int* sndport, char* buf, int len);

/**
 * Check out the handle for the requested server socket.
 * @param fd The handle to check out
 * @return the handle to the checked-out server socket
 */
int pushcheckoutaccept(int fd);

/**
 * Check out the handle for the requested connection.
 * The CHECKED_OUT token 
 * is left in the registry to indicate that an application is
 * actively using the connection.
 *
 * @param protocol The protocol of the connection
 * @param port The port number of the connection
 * @param store The storage name of the requesting Suite
 *
 * @return <tt>-1</tt> if the connection is not found, other wise returns
 * the previously opened file descriptor. 
 */
int pushcheckout(char* protocol, int port, char* store);

/**
 * Check in the handle for the requested connection.
 * @param fd The handle to be checked in
 * @return <tt>0</tt> if successful, or <tt>-1</tt> on failure to check in the 
 * file descriptor to the cached push registry.
 */
int pushcheckin(int fd);

/**
 * Given the connection string and a port number, look up 
 * the push entry and return its filter.
 *
 * @param conn the connection string
 * @param port port number to match
 * @return If connection is registered, the filter is returned,
 *         otherwise NULL is returned 
 */
char *pushgetfilter(char *conn, int port);

/**
 * Given the connection string and application ID, look up the 
 * push entry and return its filter.
 *
 * @param conn the connection string
 * @param appID The MMS application ID to match
 * @return the filter from the registry
 */
char *pushgetfiltermms(char *conn, char *appID);

/**
 * To be called when a WMA, SMS, or CBS message has arrived and has been
 * cached in an inbox. For the given the connection string and
 * port number, a push entry is looked up. If one is found, its
 * isWMAMessCached flag is set to true.
 *
 * @param conn the connection string
 * @param port the port number to match
 */
void pushsetcachedflag(char *conn, int port);

/**
 * To be called when a WMA MMS message has arrived and has been
 * cached in an inbox. For the given the connection string and
 * port number, a push entry is looked up. If one is found,
 * isWMAMessCached flag is set to true.
 *
 * @param conn the connection string
 * @param appID THe MMS application ID to match
 */
void pushsetcachedflagmms(char *conn, char *appID);

#ifdef __cplusplus
}
#endif

#endif /* ifndef _PUSH_SERVER_RESOURCE_MGMT_H_ */
