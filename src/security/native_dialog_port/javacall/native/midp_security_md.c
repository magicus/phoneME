/*
 *  
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

/*
 * @file
 *
 * @brief Simulating native security manager for this platform.
 */

#include <kni.h>
#include <midpport_security.h>
#include <midp_logging.h>
#include <javacall_security.h>

/** permission listener function pointer */
static MIDP_SECURITY_PERMISSION_LISTENER pListener;

/**
 * Sets the permission result listener.
 *
 * @param listener            The permission checking result listener
 */
void midpport_security_set_permission_listener(
	MIDP_SECURITY_PERMISSION_LISTENER listener) {
    pListener = listener;
}

/**
 * Permission check status from
 * javacall_security_permission_result to MIDP defined values.
 * <p>
 * They are the same now.
 */
#define CONVERT_PERMISSION_STATUS(x) x

/**
 * Start a security permission checking.
 *
 * @param suiteId       - the MIDlet Suite the permission should be checked with
 * @param permission    - permission string
 * @param permissionLength    - length of the permission string
 * @param pHandle       - address of variable to receive the handle; this is set
 *                        only when this function returns -1.
 *
 * @return error code as:
 *      0 - if the permission is denied
 *      1 - if the permission is granted
 *     -1 - if the permission cannot be determined without blocking Java system.
 *          A handle for this check session is returned and the result will be 
 *          notified through security permission listener.
 */
jint midpport_security_check_permission(jint suiteId, jchar* permission, 
                                        jint permissionLength,
                                        jint* pHandle) {

    unsigned int result;
    switch (javacall_security_check_permission((javacall_suite_id)suiteId,
                                               (javacall_const_utf16_string)permission,
                                               permissionLength,
                                               JAVACALL_TRUE,
                                               &result)) {
    case JAVACALL_OK: 
        result = CONVERT_PERMISSION_STATUS(result);
        break;
    case JAVACALL_WOULD_BLOCK:
        *pHandle = result;
        result = -1;
        break;
    default:
        result = 0;
        break;
    }
    return (jint)result;
}

/**
 * Check status of a security permission.
 * This call should never block. 
 * If no API on the device defines the specific permission requested
 * then it must be reported as denied. If the status of the permission is 
 * not known because it might require a user interaction then 
 * it should be reported as unknown.
 * 
 * @param suiteId       - the MIDlet Suite the permission should be checked with
 * @param permission    - permission string
 * @param permissionLength    - length of the permission string
 *
 * @return status code as:
 *      0 - if the permission is denied
 *      1 - if the permission is granted
 *     -1 - if the permission cannot be determined without blocking Java system,
 *          e.g. asking user interaction.
 */
jint midpport_security_check_permission_status(jint suiteId,  
                                               jchar* permission, 
                                               jint permissionLength) {
    unsigned int result;
    switch (javacall_security_check_permission((javacall_suite_id)suiteId,
                                               (javacall_const_utf16_string)permission,
                                               permissionLength,
                                               JAVACALL_FALSE,
                                               &result)) {
    case JAVACALL_OK:
        result = CONVERT_PERMISSION_STATUS(result);
        break;
    case JAVACALL_WOULD_BLOCK:
        /* incorrect behaviour: regardless the fact that NAMS shows user dialog,
        application need to get result immediately */
        result = -1;
        REPORT_ERROR(LC_SECURITY,
                     "javacall_ams_check_permission() returns incorrect status");
        break;
    default:
        result = 0;
        break;
    }
    return (jint)result;
                                    
}

void javanotify_security_permission_check_result(const javacall_suite_id suite_id, 
                                                 const unsigned int session,
                                                 const unsigned int result) {
    (void)suite_id;
    if (NULL != pListener) {
        pListener(session, (jboolean)(CONVERT_PERMISSION_STATUS(result)>0));
    }
}
