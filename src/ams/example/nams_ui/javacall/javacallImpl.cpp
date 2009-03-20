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

#include "appManager.h"
#include "appManagerProgress.h"

#include <javacall_defs.h>
#include <javacall_memory.h>
#include <javacall_ams_platform.h>
#include <javacall_ams_installer.h>
#include <javacall_ams_app_manager.h> // for javanotify_*


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
void javacall_ams_operation_completed(javacall_opcode operation,
                                      const javacall_app_id appId,
                                      void* pResult) {
    wprintf(_T(">>> javacall_ams_operation_completed(), ")
            _T("operation = %d, appId = %d\n"), (int)operation, (int)appId);

    if (operation == JAVACALL_OPCODE_INSTALL_SUITE) {
        const size_t dataSize = sizeof(javacall_ams_install_data);

        javacall_ams_install_data* pData;

        if (pResult != NULL &&
                ((javacall_ams_install_data*)pResult)->installStatus ==
                  JAVACALL_INSTALL_STATUS_COMPLETED) {
            pData = (javacall_ams_install_data*)javacall_malloc(dataSize);
            memcpy(pData, pResult, dataSize);

            PostProgressMessage(WM_JAVA_AMS_INSTALL_FINISHED,
                                (WPARAM)appId, (LPARAM)pData);
        }
    }
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
javacall_ams_is_domain_trusted(javacall_suite_id suiteId,
                               javacall_const_utf16_string domain) {
    (void)suiteId;

    wprintf(_T(">>> javacall_ams_is_domain_trusted()\n"));

    return JAVACALL_FALSE;
}

/**
 * Installer invokes this function to inform the application manager about
 * the current installation progress.
 *
 * Note: when installation is completed, javacall_ams_operation_completed()
 *       will be called to report installation status; its last parameter
 *       holding a pointer to an operation-specific data will point to a
 *       javacall_ams_install_data structure.
 *
 * @param pInstallState pointer to a structure containing all information
 *                      about the current installation state
 * @param installStatus defines current installation step
 * @param currStepPercentDone percents completed (0 - 100), -1 if unknown
 * @param totalPercentDone percents completed (0 - 100), -1 if unknown
 */
void
javacall_ams_install_report_progress(
                             const javacall_ams_install_state* pInstallState,
                             javacall_ams_install_status installStatus,
                             int currStepPercentDone, int totalPercentDone) {
    wprintf(_T(">>> javacall_ams_install_report_progress(): %d%%, total: %d%%\n"),
            currStepPercentDone, totalPercentDone);

    WPARAM wParam = (WPARAM)MAKELONG((WORD)currStepPercentDone,
                                     (WORD)totalPercentDone);

    PostProgressMessage(WM_JAVA_AMS_INSTALL_STATUS,
                        wParam, (LPARAM)installStatus);
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
    wprintf(_T(">>> Java system state changed: state = %d\n"), (int)state);
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
void javacall_ams_midlet_state_changed(javacall_lifecycle_state state,
                                       javacall_app_id appId,
                                       javacall_change_reason reason) {
    wprintf(_T(">>> MIDlet state changed: ID  = %d, state = %d\n"),
            appId, (int)state);

    if (state == JAVACALL_LIFECYCLE_MIDLET_SHUTDOWN) {
        wprintf(_T(">>> MIDlet with ID %d has exited\n"), appId);
        MIDletTerminated(appId);
    }
}
/**
 * Inform on change of the specific MIDlet's lifecycle status.
 *
 * Java will invoke this function whenever the running MIDlet has switched
 * to foreground or background.
 *
 * @param state new state of the running MIDlet. Can be either
 *        <tt>JAVACALL_MIDLET_STATE_UI_FOREGROUND</tt>,
 *        <tt>JAVACALL_MIDLET_STATE_UI_BACKGROUND</tt>,
 *        <tt>JAVACALL_MIDLET_STATE_UI_FOREGROUND_REQUEST</tt>,
 *        <tt>JAVACALL_MIDLET_STATE_UI_BACKGROUND_REQUEST</tt>.
 * @param appID The ID of the state-changed suite
 * @param reason The reason why the state change has happened
 */
void javacall_ams_midlet_ui_state_changed(javacall_midlet_ui_state state,
                                          javacall_app_id appId,
                                          javacall_change_reason reason) {
    int appIndex = 0;

    switch (state) {
        case JAVACALL_MIDLET_UI_STATE_FOREGROUND:
            wprintf(_T("[NAMS] Midlet state change to foreground\n"));
            break;
        case JAVACALL_MIDLET_UI_STATE_BACKGROUND:
            wprintf(_T("[NAMS] Midlet state change to background\n"));
            break;
        case JAVACALL_MIDLET_UI_STATE_FOREGROUND_REQUEST:
            wprintf(_T("[NAMS] Midlet is requesting foreground\n"));
            break;
        case JAVACALL_MIDLET_UI_STATE_BACKGROUND_REQUEST:
            wprintf(_T("[NAMS] Midlet is requesting background\n"));
            break;
        default:
            wprintf(_T("[NAMS] Midlet state is changed to Unknown\n"));
            break;
    }
}

/**
 * This function is called by the installer when some action is required
 * from the user.
 *
 * It must be implemented at that side (SJWC or Platform) where the
 * application manager is located.
 *
 * After processing the request, javacall_ams_install_answer() must
 * be called to report the result to the installer.
 *
 * @param requestCode   identifies the requested action
 *                      in pair with pInstallState->appId uniquely
 *                      identifies this request
 * @param pInstallState pointer to a structure containing all information
 *                      about the current installation state
 * @param pRequestData  pointer to request-specific data (may be NULL)
 *
 * @return <tt>JAVACALL_OK</tt> if handling of the request was started
 *                              successfully,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_ams_install_ask(javacall_ams_install_request_code requestCode,
                         const javacall_ams_install_state* pInstallState,
                         const javacall_ams_install_data* pRequestData) {
    wprintf(_T(">>> javacall_ams_install_ask(), requestCode = %d\n"), requestCode);


    InstallRequestData* pRqData =
        (InstallRequestData*)javacall_malloc(sizeof(InstallRequestData));
    if (pRqData == NULL || pInstallState == NULL) {
        return JAVACALL_FAIL;
    }

    memcpy(&pRqData->installState, pInstallState,
           sizeof(javacall_ams_install_state));

    if (pRequestData != NULL) {
        memcpy(&pRqData->requestData, pRequestData,
               sizeof(javacall_ams_install_data));
    }

    BOOL fRes = PostProgressMessage(WM_JAVA_AMS_INSTALL_ASK,
                                    (WPARAM)requestCode,
                                    (LPARAM)pRqData);

    return (fRes == TRUE) ? JAVACALL_OK : JAVACALL_FAIL;
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
    wprintf(_T(">>> javacall_ams_uncaught_exception(), midletName = '%s'\n"),
            midletName);

    javanotify_ams_uncaught_exception_handled(appId, JAVACALL_TERMINATE_MIDLET);
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
    wprintf(_T(">>> javacall_ams_out_of_memory(), midletName = '%s'\n"),
            midletName);

    javanotify_ams_out_of_memory_handled(appId, JAVACALL_TERMINATE_THREAD);
}

};  // extern "C"
