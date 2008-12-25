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
#ifndef __JAVACALL_PUSH_H
#define __JAVACALL_PUSH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file javacall_push.h
 * @ingroup MandatoryPush
 * @brief Javacall interfaces for push dialog
 */
    
    
#include "javacall_defs.h" 


/**
 * @defgroup MandatoryPush Push Dialog Box API
 * @ingroup JTWI
 * 
 * This Requirement defines a API for a native dialog box used for asking user for
 * permission to launch PUSH Java MIDlet. The function will be called by the Java task
 * whenever a MIDlet is not running, and a PUSH event was received. \n
 * It should display a native dialog box and should block until the user makes a
 * ACCEPT/REJECT selection. This selection is passed back as the return value of the
 * function. \n
 * Note that if the user approved running the MIDlet, the display focus will be held by
 * the Java VM until the MIDlet quits. If the user selected 'reject', the display will
 * return to the previous application.
 * 
 * @{
 */

    
/**
 * Popup native dialog and block till user provides a reply: 
 * either permit or deny.
 * If user permits to run Java, then device focus needs to be passed to 
 * the Java task.
 *
 * @param midletName UNICODE name of midlet to launch
 * @param midletNameLen length of name of midlet to launch
 * @param midletSuite UNICODE midlet suite to launch
 * @param midletSuiteLen length of midlet suite to launch
 *
 * @return <tt>JAVACALL_TRUE</tt> if permit, <tt>JAVACALL_FALSE</tt> if deny or on error
 *                
 */
javacall_bool javacall_push_show_request_launch_java(
        const javacall_utf16* midletName,  int midletNameLen,
        const javacall_utf16* midletSuite, int midletSuiteLen) ;
    

/** @} */

/**
 * @defgroup PlatformPushDB Platform push database
 * @ingroup JTWI
 * @brief Javacall interfaces when platform support push database,
 * push database support, connection push and alarm push
 *
 * @{
 */

/** Push DB oepration result */
typedef enum {
    JAVACALL_PUSH_OK,
    JAVACALL_PUSH_ILLEGAL_ARGUMENT = -1,
    JAVACALL_PUSH_UNSUPPORTED = -2,
    JAVACALL_PUSH_MIDLET_NOT_FOUND = -3,
    JAVACALL_PUSH_SUITE_NOT_FOUND = -4,
    JAVACALL_PUSH_CONNECTION_IN_USE = -5,
    JAVACALL_PUSH_PERMISSION_DENIED = -6
}javacall_push_result;

/**
 * Information of a push connection entry.
 */
typedef struct {
    /**
     * Generic connection protocol, host and port number
     * (optional parameters may be included separated with semi-colons (;))
     */
    javacall_utf16_string connection;
    /** Number of chars in connection string */
    javacall_int32 connectionLen;
    /**
     * Class name of the MIDlet to be launched, when new external data is available.
     * The named MIDlet MUST be registered in the descriptor file or the JAR file
     * manifest with a MIDlet- record.
     */
    javacall_utf16_string midlet;
    /** Number of chars in MIDlet class name */
    javacall_int32 midletLen;
    /**
     * A connection URL string indicating which senders
     * are allowed to cause the MIDlet to be launched.
     */
    javacall_utf16_string filter;
    /** Number of chars in filter string */
    javacall_int32 filterLen;
} javacall_push_entry;


/**
 * Register push into platform's pushDB.
 * @param suiteID    unique ID of the MIDlet suite
 * @param connection connection string pass by
 *                   javax.microedition.io.PushRegistry.registerConnection.
 *                   for example, "sms://:12345"
 * @param midlet  the midlet class name of MIDlet should be launched
 * @param fillter pass by javax.microedition.io.PushRegistry.registerConnection.
 *
 * @return  <tt>JAVACALL_PUSH_OK</tt>  - on success, 
 *  
 *          <tt>JAVACALL_PUSH_ILLEGAL_ARGUMENT</tt> - if
 *          connection or filter string is invalid,
 *  
 *          <tt>JAVACALL_PUSH_UNSUPPORTED</tt>  - if the runtime
 *          system does not support push delivery for the
 *          requested connection protocol,
 *  
 *          <tt>JAVACALL_PUSH_MIDLET_NOT_FOUND</tt> - if class
 *          name was not found,
 *  
 *          <tt>JAVACALL_PUSH_SUITE_NOT_FOUND</tt> - if suite ID
 *          was not found,
 *  
 *          <tt>JAVACALL_PUSH_CONNECTION_IN_USE</tt> - if the
 *          connection is registered already
 */
javacall_push_result
javacall_push_register(const javacall_suite_id suiteID,
                       const javacall_push_entry* entry);

/**
 * Unregister push from platform's pushDB.
 * @param suiteID    unique ID of the MIDlet suite
 * @param connection connection string pass by
 *                   javax.microedition.io.PushRegistry.registerConnection.
 *                   for example, "sms://12345"
 *
 * @return  <tt>JAVACALL_PUSH_OK</tt> - on success,
 *  
 *          <tt>JAVACALL_PUSH_ILLEGAL_ARGUMENT</tt> - if
 *          connection was not registered by any MIDlet suite,
 *  
 *          <tt>JAVACALL_PUSH_SUITE_NOT_FOUND</tt> - if suite ID
 *          was not found,
 *  
 *          <tt>JAVACALL_PUSH_PERMISSION_DENIED</tt> - if the
 *          connection was registered by another MIDlet suite
 */
javacall_push_result
javacall_push_unregister(const javacall_suite_id suiteID,
                         const javacall_utf16_string connection);

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
 * @return  <tt>JAVACALL_PUSH_OK</tt> - on success,
 *  
 *          <tt>JAVACALL_PUSH_UNSUPPORTED</tt> - if the runtime
 *          system does not support push delivery for the
 *          requested connection protocol,
 *  
 *          <tt>JAVACALL_PUSH_MIDLET_NOT_FOUND</tt> - if class
 *          name was not found,
 *  
 *          <tt>JAVACALL_PUSH_SUITE_NOT_FOUND</tt> - if suite ID
 *          was not found
 */
javacall_push_result
javacall_push_alarm_register(const javacall_suite_id suiteID,
                             const javacall_utf16_string midlet,
                             javacall_int64 time,
                             /*OUT*/javacall_int64* prev_time);

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
                          int* const pNumOfConnections);

/**
 * Release resources allocated by <tt>javacall_push_listentries</tt>.
 * 
 * @param entries       OUT pointer to an array of registered
 *                      connections for calling MIDlet suite.
 * @param pNumOfConnections  OUT the number of connections
 * 
 * @return  <tt>JAVACALL_PUSH_OK</tt> on success,
 *          <tt>JAVACALL_PUSH_SUITE_NOT_FOUND</tt> if suite ID
 *          was not found */
void javacall_push_release_entries(const javacall_push_entry* entries,
                                   int pNumOfConnections);

/** @} */


#ifdef __cplusplus
} // extern "C"
#endif

#endif

