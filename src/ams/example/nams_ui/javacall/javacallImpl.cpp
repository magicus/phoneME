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

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include <javautil_unicode.h>
#include <javacall_ams_platform.h>
#include <javacall_ams_installer.h>

extern "C" {

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
void java_ams_operation_completed(javacall_opcode operation,
                                  const javacall_app_id appId,
                                  void* pResult) {
    wprintf(
        _T(">>> java_ams_operation_completed(), operation = %d, appId = %d\n"),
            (int)operation, (int)appId);
}

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
java_ams_is_domain_trusted(javacall_suite_id suiteId,
                           javacall_const_utf16_string domain) {
    (void)suiteId;

    wprintf(_T(">>> java_ams_is_domain_trusted()\n"));

    return JAVACALL_FALSE;
}

/**
 * Installer invokes this function to inform the task manager about
 * the current installation progress.
 *
 * Note: when installation is completed, javacall_ams_operation_completed()
 *       will be called to report installation status.
 *
 * @param installPercent percents completed (0 - 100)
 */
void
java_ams_installation_percentage(int installPercent) {
    wprintf(_T(">>> java_ams_installation_percentage(): %d%%\n"),
            installPercent);
}

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
void java_ams_midlet_state_changed(javacall_lifecycle_state state,
                                   javacall_app_id appId,
                                   javacall_change_reason reason) {
    wprintf(_T(">>> State changed: ID  = %d, state = %d\n"), appId, state);

    if (state == JAVACALL_LIFECYCLE_MIDLET_SHUTDOWN) {
        wprintf(_T(">>> MIDlet with ID %d has exited\n"), appId);
    }
}

};
