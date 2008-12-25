/*
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

#ifdef __cplusplus
extern "C" {
#endif
    
    
#include "javacall_push.h" 
#include "javacall_memory.h"


/**
 * Register push into platform's pushDB.
 * @param suiteID    unique ID of the MIDlet suite
 * @param connection connection string pass by
 *                   javax.microedition.io.PushRegistry.registerConnection.
 *                   for example, "sms://:12345"
 * @param midlet  the midlet class name of MIDlet should be launched
 * @param fillter pass by javax.microedition.io.PushRegistry.registerConnection.
 *
 * @return  <tt>JAVACALL_PUSH_OK</tt> on success,
 *          <tt>JAVACALL_PUSH_ILLEGAL_ARGUMENT</tt> if
 *          connection or filter string is invalid,
 *          <tt>JAVACALL_PUSH_UNSUPPORTED</tt> if the runtime
 *          system does not support push delivery for the
 *          requested connection protocol,
 *          <tt>JAVACALL_PUSH_MIDLET_NOT_FOUND</tt> if class
 *          name was not found,
 *          <tt>JAVACALL_PUSH_SUITE_NOT_FOUND</tt> if suite ID
 *          was not found,
 *          <tt>JAVACALL_PUSH_CONNECTION_IN_USE</tt> if the
 *          connection is registered already
 */
javacall_push_result
javacall_push_register(const javacall_suite_id suiteID,
                       const javacall_push_entry* entry) {
    return JAVACALL_PUSH_OK;
}
    
/**
 * Unregister push from platform's pushDB.
 * @param suiteID    unique ID of the MIDlet suite
 * @param connection connection string pass by
 *                   javax.microedition.io.PushRegistry.registerConnection.
 *                   for example, "sms://12345"
 *
 * @return  <tt>JAVACALL_PUSH_OK</tt> on success,
 *          <tt>JAVACALL_PUSH_ILLEGAL_ARGUMENT</tt> if
 *          connection was not registered by any MIDlet suite,
 *          <tt>JAVACALL_PUSH_SUITE_NOT_FOUND</tt> if suite ID
 *          was not found,
 *          <tt>JAVACALL_PUSH_PERMISSION_DENIED</tt> if the
 *          connection was registered by another MIDlet suite
 */
javacall_push_result
javacall_push_unregister(const javacall_suite_id suiteID,
                         const javacall_utf16_string connection) {
    return JAVACALL_PUSH_OK;
}

/**
 * Register alarm into platform's pushDB.
 * @param suiteID  unique ID of the MIDlet suite
 * @param midlet   the midlet class name of MIDlet should be launched
 * @param time     the number of milliseconds since January 1, 1970, 00:00:00 GMT
 * @param prev_time If a wakeup time is already registered, the
 *                  previous value will be returned, otherwise a
 *                  zero is returned the first time the alarm is
 *                  registered.
 *
 * @return  <tt>JAVACALL_PUSH_OK</tt> on success,
 *          <tt>JAVACALL_PUSH_UNSUPPORTED</tt> if the runtime
 *          system does not support push delivery for the
 *          requested connection protocol,
 *          <tt>JAVACALL_PUSH_MIDLET_NOT_FOUND</tt> if class
 *          name was not found,
 *          <tt>JAVACALL_PUSH_SUITE_NOT_FOUND</tt> if suite ID
 *          was not found
 */
javacall_push_result
javacall_push_alarm_register(const javacall_suite_id suiteID,
                             const javacall_utf16_string midlet,
                             javacall_int64 time,
                             /*OUT*/javacall_int64* prev_time) {
    *prev_time = 0;
    return JAVACALL_PUSH_OK;
}

/**
 * Return a list of registered connections for the current MIDlet suite.
 *
 * @param suiteID       unique ID of the MIDlet suite
 * @param available     if true, only return the list of
 *                      connections with input available,
 *                      otherwise return the complete list of
 *                      registered connections for the current
 *                      MIDlet suite.
 * @param entries       OUT pointer to be set to an array of
 *                      registered connections for calling
 *                      MIDlet suite.
 * @param pNumOfConnections  OUT pointer to be set to the number
 *                           of connections returned
 *
 * @return  <tt>JAVACALL_PUSH_OK</tt> on success,
 *          <tt>JAVACALL_PUSH_SUITE_NOT_FOUND</tt> if suite ID
 *          was not found
 * @note  use <tt>javacall_push_release_entries</tt> to free
 *        allocated resources
 *  */
javacall_push_result
javacall_push_listentries(const javacall_suite_id suiteID,
                          const javacall_bool available,
                          javacall_push_entry const** entries,
                          int* const  pNumOfConnections) {
    
    (void)suiteID;
    (void)available;
    (void)entries;
    (void)pNumOfConnections;
    return JAVACALL_PUSH_UNSUPPORTED;
}

/**
 * Release resources allocated by <tt>javacall_push_listentries</tt>.
 * 
 * @param entries       OUT pointer to an array of registered
 *                      connections for calling MIDlet suite.
 * @param pNumOfConnections  OUT the number of connections
 *                
 */
void javacall_push_release_entries(const javacall_push_entry* entries,
                                   int pNumOfConnections) {
    (void)entries;
    (void) pNumOfConnections;
}
    

/** @} */


#ifdef __cplusplus
} //extern "C"
#endif
