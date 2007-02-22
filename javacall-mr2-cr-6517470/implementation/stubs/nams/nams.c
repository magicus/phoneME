/*
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

#include <javacall_nams.h>

/**
 * Inform on completion of the previously requested operation.
 *
 * @param appID The ID used to identify the application
 * @param pResult Pointer to a static buffer containing
 *                operation-dependent result
 */
void javacall_ams_operation_completed(javacall_opcode operation,
                                      const javacall_app_id appID,
                                      void* pResult);

/**
 * Inform on change of the specific MIDlet's lifecycle status.
 *
 * Java will invoke this function whenever the lifecycle status of the running
 * MIDlet is changed, for example when the running MIDlet has been paused,
 * resumed, the MIDlet has shut down etc.
 * 
 * @param state new state of the running MIDlet. Can be either,
 *        <tt>JAVACALL_MIDLET_STATE_ACTIVE</tt>
 *        <tt>JAVACALL_MIDLET_STATE_PAUSED</tt>
 *        <tt>JAVACALL_MIDLET_STATE_DESTROYED</tt>
 *        <tt>JAVACALL_MIDLET_STATE_ERROR</tt>
 * @param appID The ID of the state-changed suite
 * @param reason The reason why the state change has happened
 */
void javacall_ams_midlet_state_changed(javacall_midlet_state state,
                                       const javacall_app_id appID,
                                       javacall_change_reason reason) {
}
                                      
/**
 * Inform on change of the specific MIDlet's lifecycle status.
 *
 * Java will invoke this function whenever the running MIDlet has switched
 * to foreground or background.
 *
 * @param state new state of the running MIDlet. Can be either
 *        <tt>JAVACALL_MIDLET_UI_STATE_FOREGROUND</tt>,
 *        <tt>JAVACALL_MIDLET_UI_STATE_BACKGROUND</tt>,
 *        <tt>JAVACALL_MIDLET_UI_STATE_FOREGROUND_REQUEST</tt>,
 *        <tt>JAVACALL_MIDLET_UI_STATE_BACKGROUND_REQUEST</tt>.
 * @param appID The ID of the state-changed suite
 * @param reason The reason why the state change has happened
 */
void javacall_ams_ui_state_changed(javacall_midlet_ui_state state,
                                   const javacall_app_id appID,
                                   javacall_change_reason reason) {
}

/**
 * Get path name of the directory which holds suite's RMS files 
 * @param suiteID Unique ID of the MIDlet suite
 * @param szPath  A buffer allocated to contain the returned path name string.
 *                The returned string must be double-'\0' terminated.
 * @param maxPath Buffer length of szPath
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_ams_get_rms_path(javacall_suite_id suiteID, 
                                          javacall_utf16_string szPath, 
                                          int maxPath) {
    return JAVACALL_OK;
}

/**
 * Get domain information of the suite
 * @param suiteID Unique ID of the MIDlet suite
 * @param domain Pointer to a javacall_ext_ams_domain to contain returned
 *                domain information. Only Trusted or Untrusted domain is
 *                required to be returned.
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_ams_get_domain(javacall_suite_id suiteID,
                                        javacall_ams_domain* domain) {
    return JAVACALL_OK;
}

/**
 * Get permission set of the suite
 * @param suiteID      Unique ID of the MIDlet suite
 * @param permissions  Pointer to a javacall_ext_ams_permission_set structure
 *                     to contain returned permission setttings
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_get_permissions(javacall_suite_id suiteID,
                             javacall_ams_permission_set* permissions) {
    return JAVACALL_OK;
}

/**
 * Set single permission of the suite when user changed it.
 * @param suiteID     Unique ID of the MIDlet suite
 * @param permission  permission be set
 * @param value       new value of permssion
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_set_permission(javacall_suite_id suiteID,
                            javacall_ams_permission permission,
                            javacall_ams_permission_val value) {
    return JAVACALL_OK;
}

/**
 * Set permission set of the suite
 * @param suiteID      Unique ID of the MIDlet suite
 * @param permissions  Pointer to a javacall_ext_ams_permission_set structure
 *                     to contain returned permission setttings
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_set_permissions(javacall_suite_id suiteID,
                             javacall_ams_permission_set* permissions) {
    return JAVACALL_OK;
}

/**
 * Get specified property value of the suite.
 * @param suiteID   Unique ID of the MIDlet suite
 * @param key       Property name
 * @param value     Buffer to conatain returned property value
 * @param maxValue  Buffern length of value
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_get_suite_property(const javacall_suite_id suiteID,
                                const javacall_utf16_string key,
                                javacall_utf16_string value,
                                int maxValue) {
    return JAVACALL_OK;
}

/**
 * Get suite id by vendor and suite name.
 * @param vendorName  vendor name
 * @param suiteName   Suite name
 * @param suiteID     return suiteID
 *
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_get_suite_id(const javacall_utf16_string vendorName,
                          const javacall_utf16_string suiteName,
                          javacall_suite_id *suiteID) {
    return JAVACALL_OK;
}

/**
 * VM invokes this function to get the image cache path.
 * @param suiteID   Unique ID of the MIDlet suite
 * @param cachePath buffer for Platform store the image cache path.
 * @param cachePathLen the length of cachePath
 *
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_get_resource_cache_path(const javacall_suite_id suiteID,
                                     javacall_utf16_string cachePath,
                                     int cachePathLen) {
    return JAVACALL_OK;
}
