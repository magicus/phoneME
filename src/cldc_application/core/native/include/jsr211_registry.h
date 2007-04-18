/*
 * 
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
 * @file
 * @defgroup chapi JSR 211 Content Handler API (CHAPI)
 * @ingroup stack
 * @brief This is the API definition for content handler registry access.
 * ##include <jsr211_registry.h>
 * @{
 * <P>
 * Content handler registry API defines a target-specific component which is
 * responsible for managing registered content handlers. A registery MUST 
 * support registration, unregistration and search facilities; 
 * registered content handlers information MUST be saved between device
 * power cycles.
 * <P>
 * Implementation should release any allocated memory buffer.
 * Search and handler data load results are placed to the special 
 * JSR211_RESULT_* structures which should be filled out by the dedicated 
 * routines: <UL>
 *  <LI> @link jsr211_fillHandler
 *  <LI> @link jsr211_fillHandlerArray
 *  <LI> @link jsr211_fillStringArray
 * </UL>
 * The usage pattern is following:
 *  <PRE>
 *  jsr211_result jsr211_someFunction(..., JSR211_RESULT_STRARRAY* result) {
 *      jsr211_result status;   
 *      pcsl_string *buffer;
 *      int n;
 *      n = <determine number of selected strings>
 *      buffer = <allocate memory for n strings>
 *      <select strings from the storage into buffer>
 *      if (status == JSR211_OK) {
 *          status = jsr211_fillStringArray(buffer, n, result);
 *      }
 *      <free allocated buffer>
 *      return status
 *  }
 *  </PRE>
 */

#ifndef _JSR211_REGISTRY_H_
#define _JSR211_REGISTRY_H_

#include <pcsl_string.h>
#include "suiteid.h"

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

/**
 * Functions results enumeration
 */
typedef enum {
  JSR211_OK,       /**< A function finished without errors */
  JSR211_FAILED    /**< An error occured */
} jsr211_result;

/**
 * Boolean enumeration
 */
typedef enum {
  JSR211_FALSE,    /**< False value */
  JSR211_TRUE      /**< True value */
} jsr211_boolean;

/**
 * Result codes for jsr211_execute_handler() method.
 */
typedef enum {
    JSR211_LAUNCH_OK                = 0,    /** OK, handler started */
    JSR211_LAUNCH_OK_SHOULD_EXIT    = 1,    /** OK, handler started 
                                            or is ready to start, 
                                            invoking app should exit. */
    JSR211_LAUNCH_ERR_NOTSUPPORTED  = -1,   /** ERROR, not supported */
    JSR211_LAUNCH_ERR_NO_HANDLER    = -2,    /** ERROR, no requested handler */
    JSR211_LAUNCH_ERR_NO_INVOCATION = -3,    /** ERROR, no invocation queued for 
                                                       requested handler */
    JSR211_LAUNCH_ERROR             = -4    /** common error */
} jsr211_launch_result;

/**
 * Content handler fields enumeration.
 */
typedef enum {
  JSR211_FIELD_ID = 0,     /**< Handler ID */
  JSR211_FIELD_TYPES,      /**< Types supported by a handler */
  JSR211_FIELD_SUFFIXES,   /**< Suffixes supported by a handler */
  JSR211_FIELD_ACTIONS,    /**< Actions supported by a handler */
  JSR211_FIELD_LOCALES,    /**< Locales supported by a handler */
  JSR211_FIELD_ACTION_MAP, /**< Handler action map */
  JSR211_FIELD_ACCESSES,   /**< Access list */
  JSR211_FIELD_COUNT       /**< Total number of fields */
} jsr211_field;

/**
 * Content handlers flags enumeration
 */
typedef enum {
  JSR211_FLAG_COMMON = 0, /**< Empty flag */
  JSR211_FLAG_DYNAMIC,    /**< Indicates content handler is dynamic */
  JSR211_FLAG_NATIVE      /**< Indicates content handler is native */
} jsr211_flag;

/**
 * Search modes for getHandler0 native implementation.
 * Its values MUST correspond RegistryStore SEARCH_* constants.
 */
typedef enum {
    JSR211_SEARCH_EXACT  = 0,        /** Search by exact match with ID */
    JSR211_SEARCH_PREFIX = 1         /** Search by prefix of given value */
} jsr211_search_flag;

/**
 * A content handler description structure, used for registration operation.
 */
typedef struct {
                      
  pcsl_string        id;         /**< Content handler ID */
  SuiteIdType        suite_id;   /**< Storage where the handler is */
  pcsl_string        class_name; /**< Content handler class name */
  jsr211_flag        flag;       /**< Flag for registered content handlers. */
  int                type_num;   /**< Number of types */
  pcsl_string*       types;      /**< The types that are supported by this 
                                        content handler */
  int                suff_num;   /**< Number of suffixes */
  pcsl_string*       suffixes;   /**< The suffixes of URLs that are supported 
                                        by this content handler */
  int                act_num;    /**< Number of actions */
  pcsl_string*       actions;    /**< The actions that are supported by this 
                                        content handler */
  int                locale_num; /**< Number of locales */
  pcsl_string*       locales;    /**< The locales that are supported by this 
                                        content handler */
  pcsl_string*       action_map; /**< The action names that are defined by 
                                        this content handler;
                                        size is act_num x locale_num  */
  int                access_num; /**< Number of accesses */
  pcsl_string*       accesses;   /**< The accessRestrictions for this 
                                        ContentHandler */
} JSR211_content_handler;

#define JSR211_CONTENT_HANDLER_INITIALIZER   {        \
    PCSL_STRING_NULL_INITIALIZER,   /* id         */  \
    0,                              /* suite_id   */  \
    PCSL_STRING_NULL_INITIALIZER,   /* class_name */  \
    0,                              /* flag       */  \
    0,                              /* type_num   */  \
    NULL,                           /* types      */  \
    0,                              /* suff_num   */  \
    NULL,                           /* suffixes   */  \
    0,                              /* act_num    */  \
    NULL,                           /* actions    */  \
    0,                              /* locale_num */  \
    NULL,                           /* locales    */  \
    NULL,                           /* action_map */  \
    0,                              /* access_num */  \
    NULL,                           /* accesses   */  \
}

/**
 * Common result buffer for serialized data storage.
 */
typedef struct {
    jchar* buf;
    jsize  len;
} _JSR211_INTERNAL_RESULT_BUFFER_;

#define _JSR211_RESULT_INITIALIZER_  { NULL, 0 }

/**
 * A content handler reduced structure, used for storing of search operation 
 * result.
 */
typedef struct {
  pcsl_string        id;         /**< Content handler ID */
  SuiteIdType        suite_id;   /**< Storage where the handler is */
  pcsl_string        class_name; /**< Content handler class name */
  jsr211_flag        flag;       /**< Flag for registered 
                                        content handlers. */
} JSR211_CH;

/**
 * Result buffer for Content Handler. Use @link jsr211_fillHandler function to 
 * fill this structure.
 */
typedef _JSR211_INTERNAL_RESULT_BUFFER_ JSR211_RESULT_CH;

/**
 * Result buffer for Content Handlers array. Use @link jsr211_fillHandlerArray
 * function to fill this structure.
 */
typedef _JSR211_INTERNAL_RESULT_BUFFER_ JSR211_RESULT_CHARRAY;

/**
 * Result buffer for string array. Use @link jsr211_fillStringArray
 * function to fill this structure.
 */
typedef _JSR211_INTERNAL_RESULT_BUFFER_ JSR211_RESULT_STRARRAY;

/**
 * Fills output result structure with handler data.
 * @param handler handler reduced data.
 * @param result output result structure.
 * @return operation status.
 */
jsr211_result jsr211_fillHandler(const JSR211_CH* handler, 
                                    /*OUT*/ JSR211_RESULT_CH* result);

/**
 * Fills output result structure with handler data array.
 * @param handlerArray array of handler reduced data.
 * @param length array length.
 * @param result output result structure.
 * @return operation status.
 */
jsr211_result jsr211_fillHandlerArray(const JSR211_CH* handlerArray, int length,
                                    /*OUT*/ JSR211_RESULT_CHARRAY* result);

/**
 * Fills output result structure with string array.
 * @param strArray string array.
 * @param length array length.
 * @param result output result structure.
 * @return operation status.
 */
jsr211_result jsr211_fillStringArray(const pcsl_string* strArray, int length,
                                    /*OUT*/ JSR211_RESULT_STRARRAY* result);

/**
 * Initializes content handler registry.
 *
 * @return JSR211_OK if content handler registry initialized successfully
 */
jsr211_result jsr211_initialize(void);

/**
 * Finalizes content handler registry.
 *
 * @return JSR211_OK if content handler registry finalized successfully
 */
jsr211_result jsr211_finalize(void);

/**
 * Store content handler information into a registry.
 *
 * @param handler description of a registering handler. Implementation MUST NOT 
 * retain pointed object
 * @return JSR211_OK if content handler registered successfully
 */
jsr211_result jsr211_register_handler(const JSR211_content_handler* handler);

/**
 * Deletes content handler information from a registry.
 *
 * @param handler_id content handler ID
 * @return JSR211_OK if content handler unregistered successfully
 */
jsr211_result jsr211_unregister_handler(const pcsl_string* handler_id);

/**
 * Searches content handler using specified key and value.
 *
 * @param caller_id calling application identifier
 * @param key search field id. Valid keys are: <ul> 
 *   <li>JSR211_FIELD_TYPES, <li>JSR211_FIELD_SUFFIXES, 
 *   <li>JSR211_FIELD_ACTIONS. </ul>
 * The special case of JSR211_FIELD_ID is used for testing new handler ID.
 * @param value search value
 * @param result the buffer for Content Handlers result array. 
 *  <br>Use @link jsr211_fillHandlerArray function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_find_handler(const pcsl_string* caller_id,
                        jsr211_field key, const pcsl_string* value,
                        /*OUT*/ JSR211_RESULT_CHARRAY* result);

/**
 * Fetches handlers registered for the given suite.
 *
 * @param suiteId requested suite ID.
 * @param result the buffer for Content Handlers result array. 
 *  <br>Use @link jsr211_fillHandlerArray function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_find_for_suite(SuiteIdType suiteId, 
                        /*OUT*/ JSR211_RESULT_CHARRAY* result);

/**
 * Searches content handler using content URL. This function MUST define
 * content type and return default handler for this type if any.
 *
 * @param caller_id calling application identifier
 * @param url content URL
 * @param action requested action
 * @param handler output value - requested handler.
 *  <br>Use @link jsr211_fillHandler function to fill this structure.
 * @return JSR211_OK if the appropriate handler found
 */
jsr211_result jsr211_handler_by_URL(const pcsl_string* caller_id, 
                        const pcsl_string* url, const pcsl_string* action, 
                        /*OUT*/ JSR211_RESULT_CH* handler);

/**
 * Returns all found values for specified field. Tha allowed fields are: <ul>
 *    <li> JSR211_FIELD_ID, <li> JSR211_FIELD_TYPES, <li> JSR211_FIELD_SUFFIXES,
 *    <li> and JSR211_FIELD_ACTIONS. </ul>
 * Values should be selected only from handlers accessible for given caller_id.
 *
 * @param caller_id calling application identifier.
 * @param field search field id
 * @param result output structure where result is placed to.
 *  <br>Use @link jsr211_fillStringArray function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_get_all(const pcsl_string* caller_id, jsr211_field field,
                        /*OUT*/ JSR211_RESULT_STRARRAY* result);

/**
 * Gets the registered content handler for the ID.
 * The query can be for an exact match or for the handler
 * matching the prefix of the requested ID.
 *  <BR>Only a content handler which is visible to and accessible to the 
 * given @link caller_id should be returned.
 *
 * @param caller_id calling application identifier.
 * @param id handler ID.
 * @param mode flag indicating whether exact or prefixed search should be 
 * performed according to @link JSR211_SEARCH_MODE constants.
 * @param handler output value - requested handler.
 *  <br>Use @link jsr211_fillHandler function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_get_handler(const pcsl_string* caller_id, 
                        const pcsl_string* id, jsr211_search_flag mode,
                        /*OUT*/ JSR211_RESULT_CH* handler);

/**
 * Loads the handler's data field. Allowed fields are: <UL>
 *  <LI> JSR211_FIELD_TYPES, <LI> JSR211_FIELD_SUFFIXES, 
 *  <LI> JSR211_FIELD_ACTIONS, <LI> JSR211_FIELD_LOCALES, 
 *  <LI> JSR211_FIELD_ACTION_MAP, <LI> and JSR211_FIELD_ACCESSES. </UL>
 *
 * @param id requested handler ID.
 * @param field_id requested field.
 * @param result output structure where requested array is placed to.
 *  <br>Use @link jsr211_fillStringArray function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_get_handler_field(const pcsl_string* id, jsr211_field field_id, 
                        /*OUT*/ JSR211_RESULT_STRARRAY* result);

/**
 * Executes specified non-java content handler.
 * @param handler_id content handler ID
 * @return codes are following
 * <ul>
 * <li> JSR211_LAUNCH_OK or JSR211_LAUNCH_OK_SHOULD_EXIT if content handler 
 *   started successfully
 * <li> other code from the enum according to error codition
 * </ul>
 */
jsr211_launch_result jsr211_execute_handler(const pcsl_string* handler_id);

/**
 * Checks whether the internal handlers, if any, are installed.
 * Implemented in jsr211_deploy.c accordingly to JAMS/NAMS mode.
 * @return JSR211_OK or JSR211_FAILED - if registry corrupted or OUT_OF_MEMORY.
 */
jsr211_result jsr211_check_internal_handlers();

/** @} */

#ifdef __cplusplus
}
#endif/*__cplusplus*/

#endif  /* _JSR211_REGISTRY_H_ */
