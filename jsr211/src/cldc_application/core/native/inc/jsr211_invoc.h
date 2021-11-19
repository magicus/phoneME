/*
 * 
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
 * @defgroup chapi JSR 211 Content Handler API (CHAPI)
 * @ingroup stack
 * @brief This is the API definition for exchange of the invocation data
 * between JVM and platform applications.
 * 
 * @{
 */

#ifndef _JSR211_INVOCATION_H_
#define _JSR211_INVOCATION_H_

#include <pcsl_string.h>
#include "jsr211_constants.h"
#include "jsr211_result.h"
#include "jsr211_registry.h"

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

/** Stored ApplicationID (CLDC version) */
typedef struct _StoredCLDCAppID {
    javacall_suite_id   suiteID;    /**< The target MIDlet suiteID */
    pcsl_string         className;    /**< The target classname */
} StoredCLDCAppID;

/**
 * Stored InvocationImpl.
 */
typedef struct _StoredInvoc {
    jint        status;        /**< The current status */
    jboolean    notified;    /**< Invocation has been notified */
    jboolean    cleanup;    /**< true to cleanup on application exit */
    jboolean    responseRequired; /**< True if a response is required */
    jint        tid;        /**< The assigned transaction id */
    jint        previousTid;    /**< The tid of a previous Invocation */

    pcsl_string url;        /**< The URL of the request */
    pcsl_string type;        /**< The type of the request */
    pcsl_string action;        /**< The action of the request */
    int         argsLen;    /**< The length of the argument array */
    pcsl_string* args;        /**< The arguments */
    void*       data;        /**< The data; may be NULL */
    int         dataLen;    /**< The length of the data in bytes */

    pcsl_string ID;        /**< The ID of the handler requested */
    StoredCLDCAppID destinationApp;  

    pcsl_string invokingAuthority; /**< The invoking authority string */
    pcsl_string invokingAppName; /**< The invoking name */
    pcsl_string invokingID;    /**< The invoking Application ID */
    StoredCLDCAppID invokingApp;  

    pcsl_string username;    /**< The username provided as credentials */
    pcsl_string password;    /**< The password provided as credentials */
} StoredInvoc;

/**
 * Function to find a matching entry entry in the queue.
 * The handlerID must match. The function seeks among new Invocations 
 * (INIT status).
 *
 * @param handlerID a string identifying the requested handler
 *
 * @return the found invocation, or NULL if no matched invocation.
 */
StoredInvoc* jsr211_get_invocation(javacall_const_utf16_string handlerID);

/**
 * Executes specified non-java content handler.
 * @param handler_id content handler ID
 * @return codes are following
 * <ul>
 * <li> JSR211_LAUNCH_OK or JSR211_LAUNCH_OK_SHOULD_EXIT if content handler 
 *   started successfully
 * <li> other code from the enum according to error codition
 * </ul>
 */
jsr211_launch_result jsr211_execute_handler(javacall_const_utf16_string handler_id);

/**
 * informs platform about finishing platform's request
 * returns should_exit flag for content handler that has processed request
 * @param tid Invocation (transaction) identifier
 * @should_exit flag for the invocation handler
 * @return true in case of success, false otherwise
 */
jsr211_boolean jsr211_platform_finish(int tid, jsr211_boolean *should_exit);

#ifdef __cplusplus
}
#endif/*__cplusplus*/

/** @} */

#endif  /* _JSR211_INVOCATION_H_ */
