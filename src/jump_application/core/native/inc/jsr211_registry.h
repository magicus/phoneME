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
 *      jchar* *buffer;
 *      jint n;
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

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

#include <jni.h>

/**
 * Common result buffer for serialized data storage.
 */
typedef struct {
    jchar* buf;
	jsize  bufsz;
	jsize  used;
} _JSR211_INTERNAL_RESULT_BUFFER_;

#define _JSR211_RESULT_INITIALIZER_  { NULL, 0, 0 }

/**
 * Transform field value from 'jsr211_field' to 'javacall_chapi_field' enum.
 */
#define JAVACALL_FIELD(jsr211_field) \
    (jsr211_field == 0? 0: jsr211_field + JAVACALL_CHAPI_FIELD_CLASS)


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
 * A content handler reduced structure, used for storing of search operation 
 * result.
 */
typedef struct {
  jchar*        id;         /**< Content handler ID */
  jint          suite_id;   /**< Storage where the handler is */
  jchar*        class_name; /**< Content handler class name */
  jsr211_flag   flag;       /**< Flag for registered 
                                        content handlers. */
} JSR211_CH;

/**
 * Checks whether the internal handlers, if any, are installed.
 * Implemented in jsr211_deploy.c accordingly to JAMS/NAMS mode.
 * @return JSR211_OK or JSR211_FAILED - if registry corrupted or OUT_OF_MEMORY.
 */
jint jsr211_check_internal_handlers();


/** @} */

#ifdef __cplusplus
}
#endif/*__cplusplus*/

#endif  /* _JSR211_REGISTRY_H_ */
