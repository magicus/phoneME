/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * Register push into platform's pushDB.
 * @param suiteID    unique ID of the MIDlet suite
 * @param connection connection string pass by
 *                   javax.microedition.io.PushRegistry.registerConnection.
 *                   for example, "sms://:12345"
 * @param midlet  the midlet class name of MIDlet should be launched
 * @param fillter pass by javax.microedition.io.PushRegistry.registerConnection.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_push_register(const javacall_suite_id suiteID,
                       const javacall_utf16_string connection,
                       const javacall_utf16_string midlet,
                       const javacall_utf16_string filter);

/**
 * Unregister push from platform's pushDB.
 * @param suiteID    unique ID of the MIDlet suite
 * @param connection connection string pass by
 *                   javax.microedition.io.PushRegistry.registerConnection.
 *                   for example, "sms://12345"
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_push_unregister(const javacall_suite_id suiteID,
                         const javacall_utf16_string connection);

/**
 * Register alarm into platform's pushDB.
 * @param suiteID  unique ID of the MIDlet suite
 * @param midlet   the midlet class name of MIDlet should be launched
 * @param time     the number of milliseconds since January 1, 1970, 00:00:00 GMT
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_push_alarm_register(const javacall_suite_id suiteID,
                             const javacall_utf16_string midlet,
                             javacall_int64 time);

/**
 * Get registered suite and classname for spefic connection.
 * @param connect 	the queried connect. for example: sms://:12345
 * @param suiteID   SuiteId which registery on that port
 * @param midlet    the midlet class name which registery on that port
 * @param maxLen    buffer length of value in characters
 *
 * @return <tt>JAVACALL_OK</tt>  on success
 *		   <tt>JAVACALL_INVALID_ARGUMENT</tt> there is not midlet
 *         register on that port
 *         <tt>JAVACALL_FAIL</tt> other errors.
 */
javacall_result
javacall_push_getRegisteredSuite(const javacall_utf16_string connection,
                                 javacall_suite_id* suiteID,
                                 javacall_utf16_string midlet,
                                 int maxLen);

/** @} */


#ifdef __cplusplus
} // extern "C"
#endif

#endif

