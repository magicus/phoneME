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
 * Native Push porting interface implementation.
 *
 */

#include <jvmconfig.h>
#include <kni.h>
#include <jvmspi.h>
#include <jvm.h>
#include <sni.h>
#include <java_types.h>
#include <nativepush_port_export.h>

/**
 * Register a dynamic connection with the native Push software.
 * The security permission should have already been checked before calling this function.
 *
 * @param suiteId    - The application suite ID string
 * @param entry      - new dynamic connection entry to be registered with current MIDlet suite
 *
 * @return one of error codes:
 *        MIDP_ERROR_NONE
 *        MIDP_ERROR_ILLEGAL_ARGUMENT  - if the connection or filter string is invalid
 *        MIDP_ERROR_UNSUPPORTED       - if the runtime system does not support push delivery for the requested connection protocol
 *        MIDP_ERROR_OUT_OF_SOURCE
 *        MIDP_ERROR_MIDLET_NOT_FOUND
 *        MIDP_ERROR_SUITE_NOT_FOUND
 *        MIDP_ERROR_CONNECTION_IN_USE
 */
MIDP_ERROR midpport_push_register_connection(SuiteIdType suiteId,
                                             MIDP_PUSH_ENTRY* entry) {
    (void)suiteId;
    (void)entry;
    return MIDP_ERROR_UNSUPPORTED;
}

/**
 * Remove a dynamic connection registration.
 *
 * @param suiteId    - The application suite ID string
 * @param connection - generic connection protocol, host and port number (optional parameters may be included separated with semi-colons (;))
 * @param connectionLen - number of chars in connection string
 *
 * @return one of the error codes:
 *        MIDP_ERROR_NONE
 *        MIDP_ERROR_ILLEGAL_ARGUMENT  - if the connection was not registered by any MIDlet suite
 *        MIDP_ERROR_OUT_OF_SOURCE
 *        MIDP_ERROR_SUITE_NOT_FOUND
 *        MIDP_ERROR_PERMISSION_DENIED - if the connection was registered by another MIDlet suite
 */
MIDP_ERROR midpport_push_unregister_connection(SuiteIdType suiteId,
                                               pcsl_string* connection) {
    (void)suiteId;
    (void)connection.;
    return MIDP_ERROR_NONE;
}

/**
 * Return a list of registered connections for the current MIDlet suite.
 *
 * @param suiteId - The application suite ID string
 * @param available - if true, only return the list of connections with input available, otherwise return the complete list of registered connections for the current MIDlet suite.
 * @param entries - OUT pointer to be set to an array of registered connections for calling MIDlet suite. Caller is responsible for freeing the array after use. Null if no connection is registered.
 * @param pNumOfConnections - OUT pointer to be set to the number of connections returned
 *
 * @return one of the error codes:
 *           MIDP_ERROR_NONE
 *           MIDP_ERROR_SUITE_NOT_FOUND
 *           MIDP_ERROR_OUT_OF_RESOURCE
 */
MIDP_ERROR midpport_push_list_entries(SuiteIdType suiteId,
                                      jboolean available,
                                      MIDP_PUSH_ENTRY** entries,
                                      jint* pNumOfConnections) {
    (void)suiteId;
    (void)available;
    (void)entries;
    (void)pNumOfConnections;

    return MIDP_ERROR_OUT_OF_RESOURCE;
}

/**
 * Register a time to launch the specified MIDlet.
 *
 * @param suiteId    - The application suite ID string
 * @param midlet    - class name of the MIDlet to be launched
 * @param midletLen - number of chars in MIDlet class name
 * @param time      - time at which the MIDlet is to be executed in the format of milliseconds since EPOC time (1970/1/1)
 * @param previousTime - OUT If a wakeup time is already registered, the previous value will be returned, otherwise a zero is returned the first time the alarm is registered.
 *
 * @return one of the error codes:
 *        MIDP_ERROR_NONE
 *        MIDP_ERROR_UNSUPPORTED      - if the runtime system does not support alarm based application launch
 *        MIDP_ERROR_MIDLET_NOT_FOUND
 *        MIDP_ERROR_SUITE_NOT_FOUND
 */
MIDP_ERROR midpport_push_register_alarm(SuiteIdType suiteId,
                                        pcsl_string* midlet,
                                        jlong time, jlong* previousTime) {
    (void)suiteId;
    (void)midlet;
    (void)time;
    (void)previousTime;

    return MIDP_ERROR_UNSUPPORTED;
}
