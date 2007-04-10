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
 * @brief Content Handler Registry implementation -- stubbed.
 */

// NOT_USED macro allows avoid te known compiler warnings.
#define NOT_USED(arg) (void*)&(arg)

#include <jsr211_registry.h>

jsr211_result jsr211_initialize(void) {
    return JSR211_FAILED; // not implemented
}

jsr211_result jsr211_finalize(void) {
    return JSR211_FAILED; // not implemented
}

jsr211_result jsr211_register_handler(const JSR211_content_handler* handler) {
    (void*)&handler;
    return JSR211_FAILED; // not implemented
}

jsr211_result jsr211_unregister_handler(const pcsl_string* handler_id) {
    NOT_USED(handler_id);
    return JSR211_FAILED; // not implemented
}

jsr211_result jsr211_find_handler(const pcsl_string* caller_id,
                        jsr211_field key, const pcsl_string* value,
                        /*OUT*/ JSR211_RESULT_CHARRAY* result) {

    NOT_USED(caller_id);
    NOT_USED(key);
    NOT_USED(value);
    NOT_USED(result);
    return JSR211_FAILED; // not implemented
}
                 
/**
 * Fetches handlers registered for the given suite.
 *
 * @param suiteId requested suite ID.
 * @param result the buffer for Content Handlers result array. 
 *  <br>Use @link jsr211_fillHandlerArray function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_find_for_suite(SuiteIdType suiteId, 
                        /*OUT*/ JSR211_RESULT_CHARRAY* result) {
    NOT_USED(suiteId);
    NOT_USED(result);
    return JSR211_FAILED; // not implemented
}

jsr211_result jsr211_handler_by_URL(const pcsl_string* caller_id, 
                        const pcsl_string* url, const pcsl_string* action, 
                        /*OUT*/ JSR211_RESULT_CH* handler) {
    NOT_USED(caller_id);
    NOT_USED(url);
    NOT_USED(action);
    NOT_USED(handler);
    return JSR211_FAILED; // not implemented
}

jsr211_result jsr211_get_all(const pcsl_string* caller_id, jsr211_field field,
                        /*OUT*/ JSR211_RESULT_STRARRAY* result) {
    NOT_USED(caller_id);
    NOT_USED(field);
    NOT_USED(result);
    return JSR211_FAILED; // not implemented
}

jsr211_result jsr211_get_handler(const pcsl_string* caller_id, 
                        const pcsl_string* id, jsr211_search_flag mode,
                        /*OUT*/ JSR211_RESULT_CH* handler) {
    NOT_USED(caller_id);
    NOT_USED(id);
    NOT_USED(handler);
    return JSR211_FAILED; // not implemented
}
                            
jsr211_result jsr211_get_handler_field(const pcsl_string* id, 
            jsr211_field field_id, /*OUT*/ JSR211_RESULT_STRARRAY* result) {

    NOT_USED(id);
    NOT_USED(result);
    return 0; // not implemented
}

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
jsr211_launch_result jsr211_execute_handler(const pcsl_string* handler_id) {
    NOT_USED(handler_id);
    return JSR211_LAUNCH_ERR_NOTSUPPORTED; // not implemented
}
