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

#include "javacall_defs.h"
#include "javacall_ams_platform.h"
#include "javacall_ams_app_manager.h"
#include "javacall_ams_installer.h"
#include "javacall_ams_suitestore.h"

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
                                      void* pResult) {
}

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
void javacall_ams_system_state_changed(javacall_system_state state) {
}

/**
 * Java invokes this function to inform the platform on change of the specific
 * MIDlet's lifecycle status.
 *
 * IMPL_NOTE: the functionality is the same as provided by
 *            javacall_lifecycle_state_changed(). One of this functions
 *            should be removed. Now it is kept for backward compatibility.
 *
 * Java will invoke this function whenever the lifecycle status of the running
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
                                       javacall_change_reason reason) {
}
                                      
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
                                          javacall_change_reason reason) {
}

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
                          javacall_utf16_string* pRmsPath) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * App Manager invokes this function to get the domain the suite belongs to.
 *
 * @param suiteId unique ID of the MIDlet suite
 * @param pDomainId [out] pointer to the location where the retrieved domain
 *                        information will be saved
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javacall_ams_suite_get_domain(javacall_suite_id suiteId,
                              javacall_ams_domain* pDomainId) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * App Manager invokes this function to get permissions of the suite.
 *
 * Note: memory for pPermissions array is allocated and freed by the caller.
 *
 * @param suiteId       [in]     unique ID of the MIDlet suite
 * @param pPermissions  [in/out] array of JAVACALL_AMS_NUMBER_OF_PERMISSIONS
 *                               elements that will be filled with the
 *                               current permissions' values on exit
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javacall_ams_suite_get_permissions(
    javacall_suite_id suiteId, javacall_ams_permission_val* pPermissions) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * App Manager invokes this function to set a single permission of the suite
 * when the user changes it.
 *
 * @param suiteId     unique ID of the MIDlet suite
 * @param permission  permission be set
 * @param value       new value of permssion
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javacall_ams_suite_set_permission(javacall_suite_id suiteId,
                                  javacall_ams_permission permission,
                                  javacall_ams_permission_val value) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * App Manager invokes this function to set permissions of the suite.
 *
 * @param suiteId       [in]  unique ID of the MIDlet suite
 * @param pPermissions  [in]  array of JAVACALL_AMS_NUMBER_OF_PERMISSIONS
 *                            elements containing the permissions' values
 *                            to be set
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javacall_ams_suite_set_permissions(
    javacall_suite_id suiteId, javacall_ams_permission_val* pPermissions) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Retrieves the specified property value of the suite.
 *
 * @param suiteId     [in]  unique ID of the MIDlet suite
 * @param key         [in]  property name
 * @param value       [out] buffer to conatain returned property value
 * @param maxValueLen [in]  buffer length of value
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javacall_ams_suite_get_property(javacall_suite_id suiteId,
                                javacall_const_utf16_string key,
                                javacall_utf16_string value,
                                int maxValueLen) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Installer invokes this function. If the suite exists, this function
 * returns an unique identifier of MIDlet suite. Note that suite may be
 * corrupted even if it exists. If the suite doesn't exist, a new suite
 * ID is created.
 *
 * @param name     [in]  name of suite
 * @param vendor   [in]  vendor of suite
 * @param pSuiteId [out] suite ID of the existing suite or a new suite ID
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javacall_ams_suite_get_id(javacall_const_utf16_string vendor,
                          javacall_const_utf16_string name,
                          javacall_suite_id* pSuiteId) {
    return JAVACALL_NOT_IMPLEMENTED;
}

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
                                     javacall_utf16_string* pCachePath) {
    return JAVACALL_NOT_IMPLEMENTED;
}

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
        javacall_bool isLastThread) {
}

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
                                javacall_bool isLastThread) {
}
