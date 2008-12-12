/*
 *
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
 */

#ifndef WMAPUSHREGISTRY_H
#define WMAPUSHREGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <kni.h>
#include <app_package.h>

/**
 * Unregister or close the given WMA entry
 *
 * @param state Current state of push connection
 * @param entry Full text of the push entry
 * @param port Port id to listen to
 * @param msid Midlet Suite ID
 * @param appID application ID of MMS message
 * @param fd unique identifier for the connection
 */
void wmaPushCloseEntry(int state, char *entry, int port,
                       AppIdType msid, char *appID, int fd);

/**
 * Check if given connection is a WMA connection.
 *
 * @param pushPort Port number stored in Push entry
 * @param entry Full text of the push entry
 * @param pushStoreName Persistent store name in Push entry
 * @param port incoming port
 * @param store Storage name of requesting suite
 *
 * @result returns true if it is WMA protocol, false otherwise
 */
jboolean isWmaProtocol(int pushPort, char *entry, char *pushStoreName,
                       int port, char *store);

/**
 * Make  a copy of a WMA push entry
 *
 * @param entry Full text of the push entry
 *
 * @return A copy of the  full text push entry is returned
 *         for WMA protocols, NULL otherwise
 */
char *getWmaPushEntry(char *entry);

/**
 * Perform a connection appropriate open
 * call. The returned identifier will be included in
 * the connection registry until needed by the
 * application level connection open request.
 *
 * @param entry A full-text push entry string from the registry
 * @param fd A pointer to a unique identifier.
 *		Used to return the identifier
 * @param port A portId.
 * @param msid Midlet Suite ID
 * @param appID Application ID of MMS message.
 */
void wmaPushProcessPort(char *buffer, int *fd, int port,
                        AppIdType msid, char *appID);

#if ENABLE_JSR_205
/**
 * Check if given connection is a MMS connection.
 *
 * @param entry Full text of the push entry
 *
 * @result returns true if it is MMS protocol, false otherwise
 */
jboolean isMMSProtocol(char *entry);

/**
 * Get MMS app ID from push entry string
 *
 * @param entry Full text of the push entry
 *
 * @return Returns app ID string if successful, NULL otherwise
 *         (Caller ersponsible for freeing this string)
 */
char *getMMSAppID(char *entry);
#endif

/**
 * check the SMS header against the push filter.
 * @param filter The filter string to be used
 * @param cmsidn The caller's MSIDN number to be tested by the filter
 * @return <code>1</code> if the comparison is successful; <code>0</code>,
 *     otherwise.
 */
int jsr120_check_filter(char *filter, char *cmsidn);

#ifdef __cplusplus
}
#endif

#endif
