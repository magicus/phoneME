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
 * @file javacall_chapi_invoke.h
 * @ingroup CHAPI
 * @brief Invocation interface for Java-Platform interaction for JSR-211 CHAPI
 */


/**
 * @defgroup CHAPI JSR-211 Content Handler API (CHAPI)
 *
 *  The following API definitions are required by JSR-211.
 *
 * @{
 */

#ifndef __JAVACALL_CHAPI_INVOKE_H
#define __JAVACALL_CHAPI_INVOKE_H

#include <javacall_defs.h>

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/



/**
 * Invocation parameters.
 */
typedef struct _InvocParams {
    javacall_utf16_string url;               /**< The URL of the request */
    javacall_utf16_string type;              /**< The type of the request */
    javacall_utf16_string action;            /**< The action of the request */
    javacall_utf16_string invokingAppName;   /**< The invoking name */
    javacall_utf16_string invokingAuthority; /**< The invoking authority string */
    javacall_utf16_string username;          /**< The username provided as credentials */
    javacall_utf16_string password;          /**< The password provided as credentials */
    int                   argsLen;           /**< The length of the argument array */
    javacall_utf16_string* args;             /**< The arguments */
    int                   dataLen;           /**< The length of the data in bytes */
    void*                 data;              /**< The data; may be NULL */
    int                   responseRequired;  /**< If not 0 , then the invoking application 
                                                  requires a response to the Invocation. */
} javacall_chapi_invocation;


/**
 * Status constants for invocation processing.
 * ATTENTION! The constant values are corresponded with Invocation class values.
 */
typedef enum {
    /** 
     * The content handler successfully completed processing the Invocation. 
     */
    INVOCATION_STATUS_OK = 5,

    /** 
     * The processing of the Invocation was cancelled by the ContentHandler. 
     */
    INVOCATION_STATUS_CANCELLED = 6,

    /** 
     * The content handler failed to correctly process the Invocation request. 
     */
    INVOCATION_STATUS_ERROR = 7

} javacall_chapi_invocation_status;


/**
 * Asks NAMS to launch specified MIDlet. <BR>
 * It can be called by JVM when Java content handler should be launched
 * or when Java caller-MIDlet should be launched.
 * @param suite_id suite Id of the requested MIDlet
 * @param class_name class name of the requested MIDlet
 * @param should_exit if <code>JAVACALL_TRUE</code>, then calling MIDlet 
 *   should voluntarily exit to allow pending invocation to be handled.
 *   This should be used when there is no free isolate to run content handler (e.g. in SVM mode).
 * @return result of operation.
 */
javacall_result javacall_chapi_ams_launch_midlet(int suite_id,
        javacall_const_utf16_string class_name,
        /* OUT */ javacall_bool* should_exit);


/**
 * Sends invocation request to platform handler. <BR>
 * This is <code>Registry.invoke()</code> substitute for Java->Platform call.
 * @param invoc_id requested invocation Id for further references
 * @param handler_id target platform handler Id
 * @param invocation filled out structure with invocation params
 * @param without_finish_notification if <code>JAVACALL_TRUE</code>, then 
 *       @link javanotify_chapi_platform_finish() will not be called after 
 *       the invocation's been processed.
 * @param should_exit if <code>JAVACALL_TRUE</code>, then calling MIDlet 
 *   should voluntarily exit to allow pending invocation to be handled.
 * @return result of operation.
 */
javacall_result javacall_chapi_platform_invoke(int invoc_id,
        const javacall_utf16_string handler_id, 
        javacall_chapi_invocation* invocation, 
        /* OUT */ javacall_bool* without_finish_notification, 
        /* OUT */ javacall_bool* should_exit);


/**
 * Called by platform to notify java VM that invocation of native handler 
 * is finished. This is <code>ContentHandlerServer.finish()</code> substitute 
 * after platform handler completes invocation processing.
 * @param invoc_id processed invocation Id
 * @param url if not NULL, then changed invocation URL
 * @param argsLen if greater than 0, then number of changed args
 * @param args changed args if @link argsLen is greater than 0
 * @param dataLen if greater than 0, then length of changed data buffer
 * @param data the data
 * @param status result of the invocation processing. 
 */
void javanotify_chapi_platform_finish(int invoc_id, 
        javacall_utf16_string url,
        int argsLen, javacall_utf16_string* args,
        int dataLen, void* data, 
        javacall_chapi_invocation_status status);


/**
 * Receives invocation request from platform. <BR>
 * This is <code>Registry.invoke()</code> substitute for Platform->Java call.
 * @param handler_id target Java handler Id
 * @param invocation filled out structure with invocation params
 * @param invoc_id unique invocation Id for further references, must be positive
 */
void javanotify_chapi_java_invoke(
        const javacall_utf16_string handler_id, 
        javacall_chapi_invocation* invocation,
        int invoc_id);


/**
 * Called by Java to notify platform that requested invocation processing
 * is completed by Java handler.
 * This is <code>ContentHandlerServer.finish()</code> substitute 
 * for Platform->Java call.
 * @param invoc_id processed invocation Id
 * @param url if not NULL, then changed invocation URL
 * @param argsLen if greater than 0, then number of changed args
 * @param args changed args if @link argsLen is greater than 0
 * @param dataLen if greater than 0, then length of changed data buffer
 * @param data the data
 * @param status result of the invocation processing. 
 * @param should_exit if <code>JAVACALL_TRUE</code>, then calling MIDlet 
 *   should voluntarily exit to allow pending invocation to be handled.
 * @return result of operation.
 */
javacall_result javacall_chapi_java_finish(int invoc_id, 
        javacall_const_utf16_string url,
        int argsLen, javacall_const_utf16_string* args,
        int dataLen, void* data, javacall_chapi_invocation_status status,
        /* OUT */ javacall_bool* should_exit);

/**
 * This structure is used to select one handler from a list of handlers to process an invocation
 */
typedef struct {
    javacall_const_utf16_string handler_id;
    javacall_const_utf16_string action_name;
} javacall_chapi_handler_info;

/**
 * this function must return index of selected content handler. Returned value must be in <code>[-1, count - 1]</code>.
 * @param action non-localized action string
 * @param count number of <code>javacall_chapi_handler_info</code> values in the <code>list</code>
 * @param list pointer to an array of handlers to select
 * @param handler_idx index of the selected handler or -1 if no handler found. Output parameter.
 * @return result of the operation
 */
javacall_result javacall_chapi_select_handler( javacall_const_utf16_string action, int count, const javacall_chapi_handler_info * list, 
                                                            /* OUT */ int * handler_idx );
                                                
#ifdef __cplusplus
}
#endif/*__cplusplus*/

/** @} */
#endif //__JAVACALL_CHAPI_INVOKE_H
