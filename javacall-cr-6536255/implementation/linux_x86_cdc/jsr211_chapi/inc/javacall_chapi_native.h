/*
 *
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

/**
 * @file javacall_chapi.h
 * @ingroup CHAPI
 * @brief Javacall interfaces for JSR-211 CHAPI
 */


/**
 * @defgroup CHAPI JSR-211 Content Handler API (CHAPI)
 *
 *  The following API definitions are required by JSR-211.
 *  These APIs are not required by standard JTWI implementations.
 *
 *  <P>NOTE! All string sizes are in <code>javacall_utf16</code> units!
 *
 * @{
 */

#ifndef __JAVACALL_JSR211_CHAPI_NATIVE_H
#define __JAVACALL_JSR211_CHAPI_NATIVE_H

#include <javacall_defs.h>
#include "javacall_chapi.h"
#include "javacall_chapi_callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/


/**
 * @defgroup jsrMandatoryChapi Mandatory CHAPI API
 * @ingroup CHAPI
 * @{
 */

/**
 * Initializes content handler registry.
 *
 * @return JAVACALL_OK if content handler registry initialized successfully
 */
javacall_result javacall_chapi_native_initialize(void);

/**
 * Finalizes content handler registry.
 *
 * @return JAVACALL_OK if content handler registry finalized successfully
 */
javacall_result javacall_chapi_native_finalize(void);

/**
 * Stores content handler information into a registry.
 *
 * @param id handler ID
 * @param suite_id suite ID
 * @param class_name handler class name
 * @param flag handler installation flag
 * @param types handler types array
 * @param nTypes length of types array
 * @param suffixes handler suffixes array
 * @param nSuffixes length of suffixes array
 * @param actions handler actions array
 * @param nActions length of actions array
 * @param locales handler locales array
 * @param nLocales length of locales array
 * @param action_names action names for every supported action 
 *                                  and every supported locale
 * @param nActionNames length of action names array. This value must be equal 
 * to @link nActions multiplied by @link nLocales .
 * @param accesses handler accesses array
 * @param nAccesses length of accesses array
 * @return operation status.
 */
javacall_result javacall_chapi_native_register_handler(
        const javacall_utf16_string id,
        const javacall_utf16_string suite_id,
        const javacall_utf16_string class_name,
        int flag, 
        const javacall_utf16_string* types,     int nTypes,
        const javacall_utf16_string* suffixes,  int nSuffixes,
        const javacall_utf16_string* actions,   int nActions,
        const javacall_utf16_string* locales,   int nLocales,
        const javacall_utf16_string* action_names, int nActionNames,
        const javacall_utf16_string* accesses,  int nAccesses);

/**
 * Deletes content handler information from a registry.
 *
 * @param id content handler ID
 * @return operation status.
 */
javacall_result javacall_chapi_native_unregister_handler(
        const javacall_utf16_string id);

/**
 * Searches content handler using specified key and value.
 *
 * @param caller_id calling application identifier
 * @param key search field id. Valid keys are: <ul> 
 *   <li>JAVACALL_CHAPI_FIELD_TYPES, <li>JAVACALL_CHAPI_FIELD_SUFFIXES, 
 *   <li>JAVACALL_CHAPI_FIELD_ACTIONS. </ul>
 * The special case of JAVACALL_CHAPI_FIELD_ID is used for testing new handler ID.
 * @param value search value
 * @param result the buffer for Content Handlers result array. 
 *  <br>Use the @link javautil_chapi_appendHandler() javautil_chapi_appendHandler function to fill this structure.
 * @return status of the operation
 */
javacall_result javacall_chapi_native_find_handler(
        const javacall_utf16_string caller_id,
        javacall_chapi_field key,
        const javacall_utf16_string value,
        /*OUT*/ javacall_chapi_result_CH_array result);

/**
 * Fetches handlers registered for the given suite.
 *
 * @param suite_id requested suite Id.
 * @param result the buffer for Content Handlers result array. 
 *  <br>Use the @link javautil_chapi_appendHandler() or 
 * @link javautil_chapi_appendHandler function to fill this structure.
 * @return status of the operation
 */
javacall_result javacall_chapi_native_find_for_suite(
                        const javacall_utf16_string suite_id,
                        /*OUT*/ javacall_chapi_result_CH_array result);

/**
 * Searches content handler using content URL. This function MUST define
 * content type and return default handler for this type if any.
 *
 * @param caller_id calling application identifier
 * @param url content URL
 * @param action requested action
 * @param handler output parameter - the handler conformed with requested URL 
 * and action.
 *  <br>Use the @link javautil_chapi_fillHandler() javautil_chapi_fillHandler function to fill this structure.
 * @return status of the operation
 */
javacall_result javacall_chapi_native_handler_by_URL(
        const javacall_utf16_string caller_id,
        const javacall_utf16_string url,
        const javacall_utf16_string action,
        /*OUT*/ javacall_chapi_result_CH handler);

/**
 * Returns all found values for specified field. Tha allowed fields are: <ul>
 *    <li> JAVACALL_CHAPI_FIELD_ID, <li> JAVACALL_CHAPI_FIELD_TYPES, <li> JAVACALL_CHAPI_FIELD_SUFFIXES,
 *    <li> and JAVACALL_CHAPI_FIELD_ACTIONS. </ul>
 * Values should be selected only from handlers accessible for given caller_id.
 *
 * @param caller_id calling application identifier.
 * @param field search field id
 * @param result output structure where result is placed to.
 *  <br>Use the @link javautil_chapi_appendString() javautil_chapi_appendString function to fill this structure.
 * @return status of the operation
 */
javacall_result javacall_chapi_native_get_all(
        const javacall_utf16_string caller_id,
        javacall_chapi_field field, 
        /*OUT*/ javacall_chapi_result_str_array result);

/**
 * Gets the registered content handler for the ID.
 * The query can be for an exact match or for the handler
 * matching the prefix of the requested ID.
 *  <BR>Only a content handler which is visible to and accessible to the 
 * given @link caller_id should be returned.
 *
 * @param caller_id calling application identifier.
 * @param id handler ID.
 * @param flag indicating whether exact or prefixed search mode should be 
 * performed.
 * @param handler output value - requested handler.
 *  <br>Use the @link javautil_chapi_fillHandler() javautil_chapi_fillHandler function to fill this structure.
 * @return status of the operation
 */
javacall_result javacall_chapi_native_get_handler(
        const javacall_utf16_string caller_id,
        const javacall_utf16_string id,
        javacall_chapi_search_flag flag,
        /*OUT*/ javacall_chapi_result_CH result);

/**
 * Loads the handler's data field. Allowed fields are: <UL>
 *  <LI> JAVACALL_CHAPI_FIELD_TYPES, <LI> JAVACALL_CHAPI_FIELD_SUFFIXES, 
 *  <LI> JAVACALL_CHAPI_FIELD_ACTIONS, <LI> JAVACALL_CHAPI_FIELD_LOCALES, 
 *  <LI> JAVACALL_CHAPI_FIELD_ACTION_MAP, <LI> and JAVACALL_CHAPI_FIELD_ACCESSES. </UL>
 *
 * @param id requested handler ID.
 * @param field_id requested field.
 * @param result output structure where requested array is placed to.
 *  <br>Use the @link javautil_chapi_appendString() javautil_chapi_appendString function to fill this structure.
 * @return status of the operation
 */
javacall_result javacall_chapi_native_get_handler_field(
        const javacall_utf16_string id,
        javacall_chapi_field key, 
        /*OUT*/ javacall_chapi_result_str_array result);

/**
 * Executes specified non-java content handler.
 * @param id content handler ID
 * @param invoc invocation parameters
 * @param exec_status handler execution status:
 *  <ul>
 *  <li> 0  - handler is succefully launched,
 *  <li> 1  - handler will be launched after JVM exits.
 *  </ul>
 *
 * @return status of the operation
 */
javacall_result javacall_chapi_native_execute_handler(
            const javacall_utf16_string id, 
            javacall_chapi_invocation* invoc, 
            /*OUT*/ int* exec_status);

/** @} */

#ifdef __cplusplus
}
#endif/*__cplusplus*/

#endif  /* __JAVACALL_JSR211_CHAPI_NATIVE_H */
