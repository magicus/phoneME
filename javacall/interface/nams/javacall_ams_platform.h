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
#ifndef __JAVACALL_AMS_PLATFORM_H
#define __JAVACALL_AMS_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"
#include "javacall_ams_config.h"

/*
 * IMPL_NOTE: the following must be removed after refactoring of
 * javacall_lifecycle_state type into two types: Java state and MIDlet state.
 */
#include "javacall_lifecycle.h"


/**
 * @defgroup PlatformAPI API that must be implemented by the Platform
 *                       if the corresponding MIDP modules are used
 * @ingroup NAMS
 *
 *
 * @{
 */

/**
 * @enum javacall_opcode
 */
typedef enum {
    /** Invalid operation */
    JAVACALL_OPCODE_INVALID,
    /** Request of run-time information on an application */
    JAVACALL_OPCODE_REQUEST_RUNTIME_INFO,
    /** Suite installation request */
    JAVACALL_OPCODE_INSTALL_SUITE,
    /** Request to enable/disable OCSP check */
    JAVACALL_OPCODE_ENABLE_OCSP,
    /** Request to get the current state of OCSP check */
    JAVACALL_OPCODE_IS_OCSP_ENABLED
} javacall_opcode;

/**
 * @enum javacall_system_state
 */
typedef enum {
     /** Java system active */
    JAVACALL_SYSTEM_STATE_ACTIVE,
    /** Java system paused */
    JAVACALL_SYSTEM_STATE_SUSPENDED,
    /** Java system destroyed */
    JAVACALL_SYSTEM_STATE_STOPPED,
    /** Java system error */
    JAVACALL_SYSTEM_STATE_ERROR
} javacall_system_state;

/**
 * @enum javacall_midlet_ui_state
 *
 * IMPL_NOTE: currently a number of MIDP structures and constants
 *            (most of the bellowing) are duplicated in javacall.
 *            A possibility to avoid it via using some synchronization
 *            mechanism like Configurator tool should be considered.
 */
typedef enum {
    /** MIDlet being in foreground */
    JAVACALL_MIDLET_UI_STATE_FOREGROUND = 1,
    /** MIDlet being in background */
    JAVACALL_MIDLET_UI_STATE_BACKGROUND,
    /** MIDlet is requesting foreground */
    JAVACALL_MIDLET_UI_STATE_FOREGROUND_REQUEST,
    /** MIDlet is requesting background */
    JAVACALL_MIDLET_UI_STATE_BACKGROUND_REQUEST
} javacall_midlet_ui_state;

/**
 * Java invokes this function to inform the App Manager on completion
 * of the previously requested operation.
 *
 * The Platform must provide an implementation of this function if the
 * App Manager is on the Platform's side.
 *
 * @param operation code of the completed operation
 * @param appId the ID used to identify the application
 * @param pResult pointer to a static buffer containing
 *                operation-dependent result
 */
void javacall_ams_operation_completed(javacall_opcode operation,
                                      javacall_app_id appId,
                                      void* pResult);

/**
 * Java invokes this function to get path name of the directory which
 * holds the suite's RMS files.
 *
 * The Platform must provide an implementation of this function if the
 * RMS is on the MIDP side.
 *
 * Note that memory for the parameter pRmsPath is allocated
 * by the callee. The caller is responsible for freeing it
 * using javacall_free().
 *
 * @param suiteId  [in]  unique ID of the MIDlet suite
 * @param pRmsPath [out] a place to hold a pointer to the buffer containing
 *                       the returned RMS path string.
 *                       The returned string must be double-'\0' terminated.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_get_rms_path(javacall_suite_id suiteId,
                          javacall_utf16_string* pRmsPath);

/**
 * Java invokes this function to get the image cache path.
 *
 * The Platform must always provide an implementation of this function.
 *
 * Note that memory for the parameter pCachePath is allocated
 * by the callee. The caller is responsible for freeing it
 * using javacall_free().
 *
 * @param suiteId    [in]  unique ID of the MIDlet suite
 * @param pCachePath [out] a place to hold a pointer to the buffer where
 *                         the Platform will store the image cache path
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_get_resource_cache_path(javacall_suite_id suiteId,
                                     javacall_utf16_string* pCachePath);

/**
 * Java invokes this function to inform the platform on change of the system
 * lifecycle status.
 *
 * @param state new state of the running Java system. Can be either,
 *        <tt>JAVACALL_SYSTEM_STATE_ACTIVE</tt>
 *        <tt>JAVACALL_SYSTEM_STATE_SUSPENDED</tt>
 *        <tt>JAVACALL_SYSTEM_STATE_STOPPED</tt>
 *        <tt>JAVACALL_SYSTEM_STATE_ERROR</tt>
 */
void javacall_ams_system_state_changed(javacall_system_state state);

/**
 * Java invokes this function to inform the platform on change of the specific
 * MIDlet's lifecycle status.
 *
 * IMPL_NOTE: the functionality is the same as provided by
 *            javacall_lifecycle_state_changed(). One of this functions
 *            should be removed. Now it is kept for backward compatibility.
 *
 * VM will invoke this function whenever the lifecycle status of the running
 * MIDlet is changed, for example when the running MIDlet has been paused,
 * resumed, the MIDlet has shut down etc.
 *
 * @param state new state of the running MIDlet. Can be either,
 *        <tt>JAVACALL_LIFECYCLE_MIDLET_STARTED</tt>
 *        <tt>JAVACALL_LIFECYCLE_MIDLET_PAUSED</tt>
 *        <tt>JAVACALL_LIFECYCLE_MIDLET_SHUTDOWN</tt>
 *        <tt>JAVACALL_LIFECYCLE_MIDLET_ERROR</tt>
 * @param appId the ID of the state-changed application
 * @param reason rhe reason why the state change has happened
 */
void javacall_ams_midlet_state_changed(javacall_lifecycle_state state,
                                       javacall_app_id appId,
                                       javacall_change_reason reason);

/**
 * Java invokes this function to inform the App Manager on change of the
 * specific MIDlet's UI state.
 *
 * The Platform must provide an implementation of this function if the
 * App Manager is on the Platform's side.
 *
 * Java will invoke this function whenever the running MIDlet has switched
 * to foreground or background.
 *
 * @param state new state of the running MIDlet. Can be either
 *        <tt>JAVACALL_MIDLET_UI_STATE_FOREGROUND</tt>,
 *        <tt>JAVACALL_MIDLET_UI_STATE_BACKGROUND</tt>
 *        <tt>JAVACALL_MIDLET_UI_STATE_FOREGROUND_REQUEST</tt>,
 *        <tt>JAVACALL_MIDLET_UI_STATE_BACKGROUND_REQUEST</tt>
 * @param appId the ID of the state-changed application
 * @param reason the reason why the state change has happened
 */
void javacall_ams_midlet_ui_state_changed(javacall_midlet_ui_state state,
                                          javacall_app_id appId,
                                          javacall_change_reason reason);

/**
 * Java invokes this function to check if the given domain is trusted.
 *
 * The Platform must provide an implementation of this function if the
 * Suite Storage is on the Platform's side.
 *
 * @param suiteId unique ID of the MIDlet suite
 * @param domain domain to check
 *
 * @return <tt>JAVACALL_TRUE</tt> if the domain is trusted,
 *         <tt>JAVACALL_FALSE</tt> otherwise
 */
javacall_bool
javacall_ams_is_domain_trusted(javacall_suite_id suiteId,
                               javacall_const_utf16_string domain);

/**
 * This function is called by Java to inform the Platform that a Java thread
 * has thrown an exception that was not caught. In response to this function the
 * Platform must call javanotify_ams_uncaught_exception_handled() to instruct
 * Java either to terminate the MIDlet or to terminate the thread that threw
 * the exception.
 *
 * @param appId the ID used to identify the application
 * @param midletName name of the MIDlet in which the exception occured
 * @param exceptionClassName name of the class containing the method
 * @param exceptionMessage exception message
 * @param isLastThread JAVACALL_TRUE if this is the last thread of the MIDlet,
 *                     JAVACALL_FALSE otherwise
 */
void javacall_ams_uncaught_exception(javacall_app_id appId,
        javacall_const_utf16_string midletName,
        javacall_const_utf16_string exceptionClassName,
        javacall_const_utf16_string exceptionMessage,
        javacall_bool isLastThread);

/**
 * This function is called by Java to inform the Platform that VM fails
 * to fulfill a memory allocation request. In response to this function the
 * Platform must call javanotify_ams_out_of_memory_handled() to instruct Java
 * either to terminate the MIDlet or to terminate the thread that trown the
 * exception, or to retry the request.
 *
 * @param appId the ID used to identify the application
 * @param midletName name of the MIDlet in which the exception occured
 * @param memoryLimit in SVM mode - heap capacity, in MVM mode - memory limit
 *                    for the isolate, i.e. the max amount of heap memory that
 *                    can possibly be allocated
 * @param memoryReserved in SVM mode - heap capacity, in MVM mode - memory
 *                       reservation for the isolate, i.e. the max amount of
 *                       heap memory guaranteed to be available
 * @param memoryUsed how much memory is already allocated on behalf of this
 *                   isolate in MVM mode, or for the whole VM in SVM mode
 * @param allocSize the requested amount of memory that the VM failed
 *                  to allocate
 * @param isLastThread JAVACALL_TRUE if this is the last thread of the MIDlet,
 *                     JAVACALL_FALSE otherwise
 */
void javacall_ams_out_of_memory(javacall_app_id appId,
                                javacall_const_utf16_string midletName,
                                int memoryLimit,
                                int memoryReserved,
                                int memoryUsed,
                                int allocSize,
                                javacall_bool isLastThread);

/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* __JAVACALL_AMS_PLATFORM_H */
