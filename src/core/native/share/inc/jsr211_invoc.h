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

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

/**
 * Status code for threads waiting for registry store access and 
 * invocation events
 */
typedef enum {
  JSR211_INVOKE_OK          = 1,
  JSR211_INVOKE_CANCELLED   = 2,
  JSR211_WAIT_JAVA          = 0x10,
  JSR211_WAIT_NATIVE        = 0x20
} jsr211_wait_status;

/**
 * Stored InvocationImpl.
 */
typedef struct _StoredInvoc {
    jint     status;        /**< The current status */
    jboolean    notified;    /**< Invocation has been notified */
    jboolean    cleanup;    /**< true to cleanup on application exit */
    jboolean    responseRequired; /**< True if a response is required */
    jint    tid;        /**< The assigned transaction id */
    jint        previousTid;    /**< The tid of a previous Invocation */
    pcsl_string url;        /**< The URL of the request */
    pcsl_string type;        /**< The type of the request */
    pcsl_string action;        /**< The action of the request */
    pcsl_string ID;        /**< The ID of the handler requested */
    SuiteIdType suiteId;    /**< The target MIDlet suiteID */
    pcsl_string classname;    /**< The target classname */
    pcsl_string invokingAuthority; /**< The invoking authority string */
    pcsl_string invokingAppName; /**< The invoking name */
    SuiteIdType invokingSuiteId; /**< The invoking MIDlet suiteID */
    pcsl_string invokingClassname; /**< The invoking MIDlet classname */
    pcsl_string invokingID;    /**< The invoking Application ID */
    pcsl_string username;    /**< The username provided as credentials */
    pcsl_string password;    /**< The password provided as credentials */
    int         argsLen;    /**< The length of the argument array */
    pcsl_string* args;        /**< The arguments */
    void*       data;        /**< The data; may be NULL */
    int         dataLen;    /**< The length of the data in bytes */
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
StoredInvoc* jsr211_get_invocation(const pcsl_string* handlerID);


#ifdef __cplusplus
}
#endif/*__cplusplus*/

/** @} */

#endif  /* _JSR211_INVOCATION_H_ */
