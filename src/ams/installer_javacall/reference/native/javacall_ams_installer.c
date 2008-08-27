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
#include "javacall_ams_installer.h"

/**
 * Application manager invokes this function to start a suite installation.
 *
 * @param srcType
 *             type of data pointed by installUrl: a JAD file, a JAR file
 *             or any of them
 * @param installUrl
 *             null-terminated http url string of MIDlet's jad or jar file.
 *             The url is of the following form:
 *             http://www.website.com/a/b/c/d.jad
 *             or
 *             file:////a/b/c/d.jad
 *             or
 *             c:\a\b\c\d.jad
 * @param invokeGUI <tt>JAVACALL_TRUE</tt> to invoke Graphical Installer,
 *                  <tt>JAVACALL_FALSE</tt> otherwise
 * @param pOperationId [out] if the installation was successfully started,
 *                           on exit contains an ID uniquely identifying
 *                           this operation; this parameter can be NULL on
 *                           entrance if the operation ID is not required.
 *                           IMPL_NOTE: currently always 0 is returned.
 *
 * @return status code: <tt>JAVACALL_OK</tt> if the installation was
 *                                           successfully started,
 *                      an error code otherwise
 *
 * The return of this function only tells if the install process is started
 * successfully. The actual result of if the installation (status and ID of
 * the newly installed suite) will be reported later by
 * java_ams_operation_completed().
 */
javacall_result
java_ams_install_suite(javacall_ams_install_source_type srcType,
                       javacall_const_utf16_string installUrl,
                       javacall_bool invokeGUI,
                       javacall_int32* pOperationId) {
    (void)srcType;
    (void)installUrl;
    (void)invokeGUI;
    (void)pOperationId;

    return JAVACALL_FAIL;
}

/**
 * Application manager invokes this function to enable or disable
 * certificate revocation check using OCSP.
 *
 * @param enable JAVACALL_TRUE to enable OCSP check,
 *               JAVACALL_FALSE - to disable it
 */
void
java_ams_install_enable_ocsp(javacall_bool enable) {
    (void)enable;
}

/**
 * Application manager invokes this function to find out if OCSP
 * certificate revocation check is enabled.
 *
 * @return JAVACALL_TRUE if OCSP check is enabled,
 *         JAVACALL_FALSE - if disabled
 */
javacall_bool
java_ams_install_is_ocsp_enabled() {
    return JAVACALL_FALSE;
}

/**
 * This function is called by the application manager to report the results
 * of handling of the request previously sent by java_ams_install_listener().
 *
 * It must be implemented at that side (SJWC or Platform) where the installer
 * is located.
 *
 * After processing the request, java_ams_install_callback() must
 * be called to report the result to the installer.
 *
 * @param requestCode   in pair with pInstallState->operationId uniquely
 *                      identifies the request for which the results
 *                      are reported by this call
 * @param pInstallState pointer to a structure containing all information
 *                      about the current installation state
 * @param pResultData   pointer to request-specific results (may NOT be NULL)
 */
void
java_ams_install_callback(javacall_ams_install_request_code requestCode,
                          const javacall_ams_install_state* pInstallState,
                          void* pResultData) {
    (void)requestCode;
    (void)pInstallState;
    (void)pResultData;                                  
}
