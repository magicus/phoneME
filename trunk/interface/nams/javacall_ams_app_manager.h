/*
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
#ifndef __JAVACALL_AMS_APP_MANAGER_H
#define __JAVACALL_AMS_APP_MANAGER_H



#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"
#include "javacall_ams_config.h"

/*
 * Functions declared in this file are exported by the MIDP runtime.
 * They should be used in case if the Application Manager is
 * implemented on the customer's side.
 */

/**
 * @file javacall_ams_app_manager.h
 * @ingroup NAMS
 * @brief Javacall interfaces for Application Manager
 */

/**
 * @defgroup NAMS Native AMS APIs
 * @ingroup JTWI
 *
 *
 * @{
 */

/**
 * @define JAVACALL_TERMINATE_MIDLET
 * @brief A return code instructing Java to terminate the MIDlet in which
 *        an unhandled exception has occured
 */
#define JAVACALL_TERMINATE_MIDLET 0

/**
 * @define JAVACALL_TERMINATE_THREAD
 * @brief A return code instructing Java to terminate the MIDlet's thread
 *        in which an unhandled exception has happened
 */
#define JAVACALL_TERMINATE_THREAD 1

/**
 * @define JAVACALL_RETRY
 * @brief A return code instructing Java to retry the memory allocation
 *        attempt that previously caused OutOfMemoryError. 
 */
#define JAVACALL_RETRY 3

/**
 * Structure where run time information about the midlet will be placed.
 */
typedef struct _javacall_midlet_runtime_info {
    /**
     * The minimum amount of memory guaranteed to be available to the isolate
     * at any time. Used to pass a parameter to midlet_create_start(),
     * < 0 if not used.
     */
    javacall_int32 memoryReserved;

    /**
     * The total amount of memory that the isolate can reserve.
     * Used to pass a parameter to midlet_create_start(), < 0 if not used.
     */
    javacall_int32 memoryTotal;

    /**
     * The approximate amount of object heap memory currently
     * used by the isolate.
     */
    javacall_int32 usedMemory;

    /**
     * Priority of the isolate (< 0 if not set).
     */
    javacall_int32 priority;

    /**
     * Name of the VM profile that should be used for the new isolate.
     * Used (1) to pass a parameter to midlet_create_start();
     * (2) to get a profile's name of the given isolate in run time.
     */
    javacall_utf16_string profileName;
} javacall_midlet_runtime_info;

/**
 * Platform invokes this function to start the MIDP system.
 * It does not return until the system is stopped.
 *
 * @return <tt>JAVACALL_OK</tt> if successful,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javanotify_ams_system_start();

/**
 * Platform invokes this function to stop the MIDP system.
 *
 * @return <tt>JAVACALL_OK</tt> if successful,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javanotify_ams_system_stop();

/**
 * App Manager invokes this function to inform Java to start a specific MIDlet
 * suite. 
 *
 * @param suiteId      ID of the suite to start
 * @param appId        ID of runtime midlet, ID must not be Zero
 * @param className    fully qualified name of the MIDlet class
 * @param pRuntimeInfo quotas and profile to set for the new application
 *
 * @return <tt>JAVACALL_OK</tt> if all parameter are valid,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 *
 * @note this function just checks the parameters accuracy,
 *       the real status of MIDlet startup will be notified
 *       by <link>javacall_ams_midlet_state_changed</link>
 */
javacall_result
javanotify_ams_midlet_start(javacall_suite_id suiteId,
                            javacall_app_id appId,
                            javacall_const_utf16_string className,
                            const javacall_midlet_runtime_info* pRuntimeInfo);

/**
 * App Manager invokes this function to inform Java to start a specific MIDlet
 * suite with arguments. 
 *
 * @param suiteId      ID of the suite to start with
 * @param appId        ID of runtime midlet
 * @param className    fully qualified name of the MIDlet class
 * @param pArgs        an array containning up to 3 arguments for
 *                     the MIDlet to be run
 * @param argsNum      number of arguments
 * @param pRuntimeInfo quotas and profile to set for the new application
 *
 * @return <tt>JAVACALL_OK</tt> if all parameter are valid,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 *
 * @note  this function just checks the parameters accuracy, 
 *        the real status of MIDlet startup will be notified by
 *        <link>javacall_ams_midlet_state_changed</link>
 */
javacall_result
javanotify_ams_midlet_start_with_args(javacall_suite_id suiteId,
                                      javacall_app_id appId,
                                      javacall_const_utf16_string className,
                                      javacall_const_utf16_string *pArgs,
                                      int argsNum,
                                      const javacall_midlet_runtime_info*
                                          pRuntimeInfo);

/**
 * App Manager invokes this function to inform Java to shutdown a specific
 * running MIDlet. If it doesn't exit in the specified amount of milliseconds,
 * it will be forcefully terminated.
 *
 * @param appId appId of the suite to shutdown
 * @param timeoutMilliSecond shutdown the suite in timeout millseconds
 *
 * @return <tt>JAVACALL_OK</tt> if <code>suiteId</code> has a proper value
 *         <tt>JAVACALL_FAIL</tt> otherwise
 *
 * @note the real status of operation will be notified by
 *       <link>javacall_ams_midlet_state_changed</link>
 */
javacall_result
javanotify_ams_midlet_shutdown(javacall_app_id appId, int timeoutMilliSeconds);

/**
 * App Manager invokes this function to inform Java to switch a specific MIDlet
 * suite to foreground.
 *
 * @param appId application ID of the suite to switch
 *
 * @return <tt>JAVACALL_OK</tt> if <code>suiteId</code> has a proper value
 *         <tt>JAVACALL_FAIL</tt> otherwise
 *
 * @note the real status of operation will be notified by
 *       <link>javacall_ams_midlet_state_changed</link>
 */
javacall_result
javanotify_ams_midlet_switch_foreground(javacall_app_id appId);

/**
 * App Manager invokes this function to inform Java to switch current MIDlet
 * suite to background, and no MIDlet will switch to foreground.
 *
 * @return <tt>JAVACALL_OK</tt> if <code>suiteId</code> has a proper value
 *         <tt>JAVACALL_FAIL</tt> otherwise
 * @note the real status of operation will be notified by
 *       <link>javacall_ams_midlet_state_changed</link>
 */
javacall_result javanotify_ams_midlet_switch_background();

/**
 * App Manager invokes this function to inform Java to pause a specific MIDlet.
 *
 * @param appId application ID of the midlet to pause
 *
 * @return <tt>JAVACALL_OK</tt> if <code>suiteId</code> has a proper value
 *         <tt>JAVACALL_FAIL</tt> otherwise
 *
 * @note the real status of operation will be notified by
 *       <link>javacall_ams_midlet_state_changed</link>
 */
javacall_result javanotify_ams_midlet_pause(javacall_app_id appId);

/**
 * App Manager invokes this function to inform Java to resume a specific MIDlet.
 *
 * @param appId application ID of the midlet to resume
 *
 * @return <tt>JAVACALL_OK</tt> if <code>suiteId</code> has a proper value
 *         <tt>JAVACALL_FAIL</tt> otherwise
 *
 * @note the real status of operation will be notified by
 *       <link>javacall_ams_midlet_state_changed</link>
 */
javacall_result javanotify_ams_midlet_resume(javacall_app_id appId);

/**
 * App Manager invokes this function to get runtime information
 * about the specified MIDlet.
 *
 * This call is asynchronous, the result will be reported later through
 * passing a MIDLET_INFO_READY_EVENT event to SYSTEM_EVENT_LISTENER.
 *
 * @param appId the ID used to identify the application
 *
 * @return error code: <tt>JAVACALL_OK<tt> if successful (operation started),
 *                     <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javanotify_ams_midlet_request_runtime_info(javacall_app_id appId);

/**
 * App Manager invokes this function to get information about the suite
 * containing the specified running MIDlet. This call is synchronous.
 *
 * @param appId    [in]  the ID used to identify the application
 * @param pSuiteId [out] on exit will hold an ID of the suite the midlet
 *                       belongs to
 *
 * @return error code: <tt>JAVACALL_OK</tt> if successful,
 *                     <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javanotify_ams_midlet_get_app_suite_id(javacall_app_id appId,
                                       javacall_suite_id* pSuiteId);

/**
 * App Manager invokes this function in response to
 * javacall_ams_uncaught_exception() to instruct Java either to terminate
 * the MIDlet or to terminate the thread that trown the exception.
 *
 * @param appId the ID used to identify the application
 * @param responseCode a constant identfifying what Java should do with the
 *            MIDlet in which an unhandled exception was thrown:
 *            <tt>JAVACALL_TERMINATE_MIDLET</tt> to terminate the MIDlet,
 *            <tt>JAVACALL_TERMINATE_THREAD</tt> to terminate the thread in
 *                                               which the exception occured
 */
void javanotify_ams_uncaught_exception_handled(javacall_app_id appId,
                                               int responseCode);

/**
 * App Manager invokes this function in response to
 * javacall_ams_out_of_memory_exception() to instruct Java how to handle
 * OutOfMemoryError that has just happened.
 *
 * @param appId the ID used to identify the application
 * @param responseCode a constant identifying what Java should do with the
 *            MIDlet in which an OutOfMeoryError has happened:
 *            <tt>JAVACALL_RETRY</tt> to retry the memory allocation attempt,
 *            <tt>JAVACALL_TERMINATE_MIDLET</tt> to terminate the MIDlet,
 *            <tt>JAVACALL_TERMINATE_THREAD</tt> to terminate the thread in
 *                                               which the exception occured
 */
void javanotify_ams_out_of_memory_handled(javacall_app_id appId,
                                          int responseCode);

/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* __JAVACALL_AMS_APP_MANAGER_H */
