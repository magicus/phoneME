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

/*
 * @file
 *
 * @brief Simulating native security manager for this platform.
 */

#include <kni.h>
#include <midpport_security.h>

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
 * Start a security permission checking.
 *
 * @param suiteId       - the MIDlet Suite the permission should be checked with
 * @param permission    - permission type
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
jint midpport_security_check_permission(jint suiteId, jint permission,
                                        jint* pHandle) {
    (void)suiteId;
    (void)permission;
    (void)pHandle;

    return 1; /* IMPL_NOTE: example grants all permissions */
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
 * @param permission    - permission type
 *
 * @return status code as:
 *      0 - if the permission is denied
 *      1 - if the permission is granted
 *     -1 - if the permission cannot be determined without blocking Java system,
 *          e.g. asking user interaction.
 */
jint midpport_security_check_permission_status(jint suiteId, jint permission) {
    (void)suiteId;
    (void)permission;

    return 1; /* IMPL_NOTE: example grants all permissions */                                    
}
