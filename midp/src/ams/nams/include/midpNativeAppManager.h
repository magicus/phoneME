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

#ifndef _MIDPNATIVEAPPMANAGER_H_
#define _MIDPNATIVEAPPMANAGER_H_

/**
 * @file
 * @ingroup nams_extern
 *
 * @brief External Interface for native AMS.
 *
 * @{
 */

#include <kni.h>
#include <midp_constants_data.h>

/**
 * Defines for Java platform system states
 */
#define MIDP_SYSTEM_STATE_STARTED    1 /**< when system is started up and ready to serve any MIDlet requests */
#define MIDP_SYSTEM_STATE_SUSPENDED  2 /**< when system finishes suspending all MIDlets and resources */
#define MIDP_SYSTEM_STATE_STOPPED    3 /**< when system stops the VM and frees all resources */
#define MIDP_SYSTEM_STATE_ERROR      4 /**< when system cannot be started */

/**
 * Starts the system. Does not return until the system is stopped.
 *
 * @return <tt>MIDP_SHUTDOWN_STATUS</tt> if the system is shutting down or
 *         <tt>MIDP_ERROR_STATUS</tt> if an error
 */
jint midp_system_start(void);

/**
 * Initiates shutdown of the system and returns immediately. System shutdown
 * is not complete until midp_system_start() returns.
 */
void midp_system_stop(void);

/**
 * The typedef of the function that will be called when Java platform
 * system state changes.
 *
 * @param state One of the MIDP_SYSTEM_STATE_* values
 */
typedef void (*MIDP_SYSTEM_STATE_CHANGE_LISTENER)(jint state);

/**
 * Sets the system state listener.
 *
 * @param listener The system state listener
 */
void midp_system_set_state_change_listener(
    MIDP_SYSTEM_STATE_CHANGE_LISTENER listener);

/**
 * The typedef of the background listener that is notified
 * when the background system changes.
 *
 * @param reason              The reason the background change happened,
 *                            currently always zero.
 */
typedef void (*MIDP_SYSTEM_BACKGROUND_LISTENER)(jint reason);

/**
 * Sets the background listener.
 *
 * @param listener            The background listener
 */
void midp_system_set_background_listener(MIDP_SYSTEM_BACKGROUND_LISTENER listener);

/**
 * Select which running MIDlet should have the foreground.  If appId is a
 * valid application ID, that application is placed into the foreground. If
 * appId is MIDLET_APPID_NO_FOREGROUND, the current foreground MIDlet will be
 * put into background and no MIDlet will have the foreground.
 *
 * If appId is invalid, or that application already has the foreground, this
 * has no effect and the foreground listener is not called.
 *
 * @param appId The ID of the application to be put into the foreground,
 *              or the special value MIDLET_APPID_NO_FOREGROUND.
 */
void midp_midlet_set_foreground(jint appId);

/**
 * The typedef of the foreground listener that is notified
 * when the foreground MIDlet changes.
 *
 * @param appId               The ID of the application that is now
 *                            in the foreground
 * @param reason              The reason the foreground change happened,
 *                            currently always zero.
 */
typedef void (*MIDP_MIDLET_FOREGROUND_LISTENER)(jint appId, jint reason);

/**
 * Sets the foreground listener.
 *
 * @param listener            The MIDlet foreground listener
 */
void midp_midlet_set_foreground_listener(MIDP_MIDLET_FOREGROUND_LISTENER listener);

/**
 * The typedef of the listener that is notified
 * when a suite is terminatied.
 *
 * @param suiteId ID of the suite
 * @param suiteIdLen length of the suite ID
 */
typedef void (*MIDP_SUITE_TERMINATION_LISTENER)(jchar* suiteId, jint suiteIdLen);

/**
 * Sets the suite termination listener.
 *
 * @param listener            The termination listener
 */
void midp_suite_set_termination_listener(MIDP_SUITE_TERMINATION_LISTENER listener);

/**
 * Create and start the specified MIDlet. The suiteId is passed to the
 * midletsuitestorage API to retrieve the class path. The appId is assigned by
 * the caller and is used to identify this MIDlet in subsequent API calls. The
 * appId must be an integer greater than zero. The suite and class must not
 * name a MIDlet that is already running. The appId must not have already been
 * used to identify another running MIDlet.
 *
 * @param suiteId             The application suite ID
 * @param suiteIdLen          Length of the application suite ID
 * @param className           Fully qualified name of the MIDlet class
 * @param classNameLen        Length of the MIDlet class name
 * @param appId               The application id used to identify the app
 */
void midp_midlet_create_start(jchar *suiteId, jint suiteIdLen,
                              jchar *className, jint classNameLen,
                              jint appId);

/**
 * Create and start the specified MIDlet passing the given arguments to it.
 * The suiteId is passed to the midletsuitestorage API to retrieve the class
 * path. The appId is assigned by the caller and is used to identify this
 * MIDlet in subsequent API calls. The appId must be an integer greater
 * than zero. The suite and class must not name a MIDlet that is already
 * running. The appId must not have already been used to identify another
 * running MIDlet.
 *
 * @param suiteId             The application suite ID
 * @param suiteIdLen          Length of the application suite ID
 * @param className           Fully qualified name of the MIDlet class
 * @param classNameLen        Length of the MIDlet class name
 * @param args                An array containning up to 3 arguments for
 *                            the MIDlet to be run
 * @param argsLen             An array containing the length of each argument
 * @param argsNum             Number of arguments
 * @param appId               The application id used to identify the app
 */
void midp_midlet_create_start_with_args(jchar *suiteId, jint suiteIdLen,
                              jchar *className, jint classNameLen,
                              jchar **args, jint *argsLen, jint argsNum,
                              jint appId);

/**
 * Resume the specified paused MIDlet.
 *
 * If appId is invalid, or if that application is already active, this call
 * has no effect and the MIDlet state change listener is not called.
 *
 * @param appId               The ID used to identify the application
 */
void midp_midlet_resume(jint appId);

/**
 * Pause the specified MIDlet.
 *
 * If appId is invalid, or if that application is already paused, this call
 * has no effect and the MIDlet state change listener is not called.
 *
 * @param appId               The ID used to identify the application
 */
void midp_midlet_pause(jint appId);

/**
 * Stop the specified MIDlet.
 *
 * If appId is invalid, this call has no effect and the MIDlet state change
 * listener is not called.
 *
 * @param appId               The ID used to identify the application
 */
void midp_midlet_destroy(jint appId);

/**
 * Defines for MIDlet states
 */
#define MIDP_MIDLET_STATE_STARTED    1
#define MIDP_MIDLET_STATE_PAUSED     2
#define MIDP_MIDLET_STATE_DESTROYED  3
#define MIDP_MIDLET_STATE_ERROR      4

/**
 * The typedef of the MIDlet state listener that is notified
 * with the MIDlet state changes.
 *
 * The reason code is one of the values
 * MIDLET_CONSTRUCTOR_FAILED,
 * MIDLET_SUITE_NOT_FOUND,
 * MIDLET_CLASS_NOT_FOUND
 * MIDLET_INSTANTIATION_EXCEPTION,
 * MIDLET_ILLEGAL_ACCESS_EXCEPTION,
 * MIDLET_OUT_OF_MEM_ERROR,
 * MIDLET_RESOURCE_LIMIT, or
 * MIDLET_ISOLATE_RESOURCE_LIMIT, or
 * MIDLET_ISOLATE_CONSTRUCTOR_FAILED.
 * See src/configuration/common/constants.xml for definitions.
 *
 * @param appId               The ID used to identify the application
 * @param state               The new state of the application, one of
 *                            the MIDP_MIDLET_STATE_* values
 * @param reason              The reason the state change happened
 */
typedef void (*MIDP_MIDLET_STATE_CHANGE_LISTENER)(jint appId, jint state, int reason);

/**
 * Sets the MIDlet state change listener.
 *
 * @param listener            The MIDlet state change listener
 */
void midp_midlet_set_state_change_listener(
        MIDP_MIDLET_STATE_CHANGE_LISTENER listener);

/* @} */

#endif /* _MIDPNATIVEAPPMANAGER_H_ */
