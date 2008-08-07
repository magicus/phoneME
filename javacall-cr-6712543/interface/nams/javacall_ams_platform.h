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
    JAVACALL_OPCODE_INSTALL_SUITE
} javacall_opcode;

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
    JAVACALL_MIDLET_UI_STATE_FOREGROUND,
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
 * @param appId the ID used to identify the application
 * @param pResult pointer to a static buffer containing
 *                operation-dependent result
 */
void java_ams_operation_completed(javacall_opcode operation,
                                  const javacall_app_id appId,
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
java_ams_get_rms_path(javacall_suite_id suiteId,
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
java_ams_get_resource_cache_path(javacall_suite_id suiteId,
                                 javacall_utf16_string* pCachePath);

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
void java_ams_ui_state_changed(javacall_midlet_ui_state state,
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
java_ams_is_domain_trusted(javacall_suite_id suiteId,
                           javacall_const_utf16_string domain);

/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* __JAVACALL_AMS_PLATFORM_H */
