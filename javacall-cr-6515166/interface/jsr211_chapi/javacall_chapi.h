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

#ifndef __JAVACALL_JSR211_H
#define __JAVACALL_JSR211_H

#include <javacall_defs.h>

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/


/**
 * @defgroup jsrMandatoryChapi Mandatory CHAPI API
 * @ingroup CHAPI
 * @{
 */

/**
 * Content handler fields enumeration.
 * Should match correspondent values in <jsr211_registry.h>
 */
typedef enum {
  JAVACALL_CHAPI_FIELD_ID = 0,     /**< Handler ID */
  JAVACALL_CHAPI_FIELD_FLAG,       /**< Handler flag */
  JAVACALL_CHAPI_FIELD_SUITE,      /**< Handler suite ID */
  JAVACALL_CHAPI_FIELD_CLASS,      /**< Handler class */
  JAVACALL_CHAPI_FIELD_TYPES,      /**< Types supported by a handler */
  JAVACALL_CHAPI_FIELD_SUFFIXES,   /**< Suffixes supported by a handler */
  JAVACALL_CHAPI_FIELD_ACTIONS,    /**< Actions supported by a handler */
  JAVACALL_CHAPI_FIELD_LOCALES,    /**< Locales supported by a handler */
  JAVACALL_CHAPI_FIELD_ACTION_MAP, /**< Handler action map */
  JAVACALL_CHAPI_FIELD_ACCESSES,   /**< Access list */
  JAVACALL_CHAPI_FIELD_COUNT       /**< Total number of fields */
} javacall_chapi_field;


/**
 * Search modes for @link javacall_chapi_get_handler() implementation.
 */
typedef enum {
    JAVACALL_CHAPI_SEARCH_EXACT  = 0,   /** Search by exact match with ID */
    JAVACALL_CHAPI_SEARCH_PREFIX = 1    /** Search by prefix of given value */
} javacall_chapi_search_flag;


/**
 * Invocation parameters for launched platform handlers.
 */
typedef struct _InvocParams {
    int                   tid;               /**< The internal transaction id */
    javacall_utf16_string url;               /**< The URL of the request */
    javacall_utf16_string type;              /**< The type of the request */
    javacall_utf16_string action;            /**< The action of the request */
    javacall_utf16_string invokingAppName;   /**< The invoking name */
    javacall_utf16_string invokingAuthority; /**< The invoking authority string */
    javacall_utf16_string username;	        /**< The username provided as credentials */
    javacall_utf16_string password;	        /**< The password provided as credentials */
    int                   argsLen;	        /**< The length of the argument array */
    javacall_utf16_string* args;              /**< The arguments */
    int                   dataLen;            /**< The length of the data in bytes */
    void*                 data;               /**< The data; may be NULL */
} javacall_chapi_invocation;


/**
 * Internal structure.
 * Common result buffer for serialized data storage.
 */
typedef struct {
    javacall_utf16* buf;
    int bufsz;
    int used;
} _JAVAUTIL_CHAPI_RESBUF_;

/**
 * Result buffer for Content Handler, used as OUTPUT parameter of 
 * javacall functions. 
 * Use the @link javautil_chapi_fillHandler() javautil_chapi_fillHandler function to fill this structure.
 */
typedef _JAVAUTIL_CHAPI_RESBUF_*   javacall_chapi_result_CH;

/**
 * Result buffer for Content Handlers array, used as OUTPUT parameter of 
 * javacall functions.
 * Use the @link javautil_chapi_appendHandler() javautil_chapi_appendHandler function to fill this structure.
 */
typedef _JAVAUTIL_CHAPI_RESBUF_*   javacall_chapi_result_CH_array;

/**
 * Result buffer for string array, used as OUTPUT parameter of javacall 
 * functions. 
 * Use the @link javautil_chapi_appendString() javautil_chapi_appendString function to fill this structure.
 */
typedef _JAVAUTIL_CHAPI_RESBUF_*   javacall_chapi_result_str_array;


/**
 * Fills output result structure with handler data.
 * @param id handler ID
 * @param id_size handler ID size
 * @param suite_id suite ID
 * @param suite_id_size suite ID size
 * @param class_name handler class name
 * @param class_name_size handler class name size
 * @param flag handler installation flag
 * @param result output result structure.
 * @return operation status.
 */
javacall_result javautil_chapi_fillHandler(
        const javacall_utf16* id, int id_size,
        const javacall_utf16* suite_id, int suite_id_size,
        const javacall_utf16* class_name, int class_name_size,
        int flag, /*OUT*/ javacall_chapi_result_CH result);

/**
 * Appends the handler data to the result array.
 * @param id handler ID
 * @param id_size handler ID size
 * @param suite_id suite ID
 * @param suite_id_size suite ID size
 * @param class_name handler class name
 * @param class_name_size handler class name size
 * @param flag handler installation flag
 * @param array output result array.
 * @return operation status.
 */
javacall_result javautil_chapi_appendHandler(
        const javacall_utf16* id, int id_size,
        const javacall_utf16* suite_id, int suite_id_size,
        const javacall_utf16* class_name, int class_name_size,
        int flag, /*OUT*/ javacall_chapi_result_CH_array array);

/**
 * Appends string to output string array.
 * @param str appended string
 * @param str_size the string size
 * @param array string array.
 * @return operation status.
 */
javacall_result javautil_chapi_appendString(
        const javacall_utf16* str, int str_size,
        /*OUT*/ javacall_chapi_result_str_array array);

/**
 * Initializes content handler registry.
 *
 * @return JAVACALL_OK if content handler registry initialized successfully
 */
javacall_result javacall_chapi_initialize(void);

/**
 * Finalizes content handler registry.
 *
 * @return JAVACALL_OK if content handler registry finalized successfully
 */
javacall_result javacall_chapi_finalize(void);

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
javacall_result javacall_chapi_register_handler(
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
javacall_result javacall_chapi_unregister_handler(
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
javacall_result javacall_chapi_find_handler(
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
javacall_result javacall_chapi_find_for_suite(
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
javacall_result javacall_chapi_handler_by_URL(
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
javacall_result javacall_chapi_get_all(
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
javacall_result javacall_chapi_get_handler(
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
javacall_result javacall_chapi_get_handler_field(
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
javacall_result javacall_chapi_execute_handler(
            const javacall_utf16_string id, 
            javacall_chapi_invocation* invoc, 
            /*OUT*/ int* exec_status);

/** @} */

#ifdef __cplusplus
}
#endif/*__cplusplus*/

#endif  /* __JAVACALL_JSR211_H */
