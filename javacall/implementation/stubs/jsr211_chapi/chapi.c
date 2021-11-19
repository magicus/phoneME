/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
 * @brief Content Handler Registry stubs.
 */

#include "javacall_chapi_invoke.h"
#include "javacall_chapi_registry.h"
#include "javacall_chapi_msg_exchange.h"


/**
 * Perform initialization of registry API
 *
 * @return JAVACALL_OK if initialization was successful, error code otherwise
 */
 javacall_result javacall_chapi_init_registry(void){
      return JAVACALL_NOT_IMPLEMENTED;
 }

/**
 * Finalize API, clean all used resources.
 *
 * @return nothing
 */
 void javacall_chapi_finalize_registry(void){}

/**
 * Add new Content Handler to Registry
 *
 * @param content_handler_id unique ID of content handler
 *                           implemenation may not check this parameter on existence in registry
 *                           if handler id exists, function may be used to update unformation about handler
 *                           all needed uniqueness checks made by API callers if needed
 * @param content_handler_friendly_appname  the user-friendly application name of this content handler
 * @param suite_id identifier of the suite or bundle where content handler is located
 * @param class_name content handler class name
 * @param flag handler registration type
 * @param types handler supported content types array, can be null
 * @param nTypes length of content types array
 * @param suffixes handler supported suffixes array, can be null
 * @param nSuffixes length of suffixes array
 * @param actions handler supported actions array, can be null
 * @param nActions length of actions array
 * @param locales handler supported locales array, can be null
 * @param nLocales length of locales array
 * @param action_names action names for every supported action 
 *                                  and every supported locale
 *                                  should contain full list of actions for first locale than for second locale etc...
 * @param nActionNames length of action names array. This value must be equal 
 * to @link nActions multiplied by @link nLocales .
 * @param access_allowed_ids list of caller application ids (or prefixes to ids) that have allowed access to invoke this handler, can be null
 * @param nAccesses length of accesses list
 * @return JAVACALL_OK if operation was successful, error code otherwise
 */
javacall_result javacall_chapi_register_handler(
        javacall_const_utf16_string content_handler_id,
        javacall_const_utf16_string content_handler_friendly_appname,
        javacall_const_utf16_string suite_id,
        javacall_const_utf16_string class_name,
        javacall_chapi_handler_registration_type flag,
        javacall_const_utf16_string* content_types,     int nTypes,
        javacall_const_utf16_string* suffixes,  int nSuffixes,
        javacall_const_utf16_string* actions,   int nActions,  
        javacall_const_utf16_string* locales,   int nLocales,
        javacall_const_utf16_string* action_names, int nActionNames,
        javacall_const_utf16_string* access_allowed_ids,  int nAccesses
        ){
    return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * Enumerate all registered content handlers
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 *				  
 * @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
 javacall_result javacall_chapi_enum_handlers(int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length){
      return JAVACALL_NOT_IMPLEMENTED;
 }

/**
 * Enumerate registered content handlers that can handle files with given suffix
 * Search is case-insensitive
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @suffix suffix of content data file with dot (for example: ".txt" or ".html") for which handlers are searched
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 * @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_enum_handlers_by_suffix(javacall_const_utf16_string suffix, int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length){
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Enumerate registered content handlers that can handle content with given content type
 * Search is case-insensitive
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @content_type type of content data for which handlers are searched
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 * @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
 javacall_result javacall_chapi_enum_handlers_by_type(javacall_const_utf16_string content_type, int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length){
      return JAVACALL_NOT_IMPLEMENTED;
 }

/**
 * Enumerate registered content handlers that can perform given action
 * Search is case-sensitive
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @action action that handler can perform against content
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 * @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
 javacall_result javacall_chapi_enum_handlers_by_action(javacall_const_utf16_string action, int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length){
      return JAVACALL_NOT_IMPLEMENTED;
 }

/**
 * Enumerate registered content handlers located in suite (bundle) with given suite id
 * Search is case-sensitive
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @suite_id suite id for which content handlers are searched
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 * @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_enum_handlers_by_suite_id(
        javacall_const_utf16_string suite_id,
        int* pos_id, 
        /*OUT*/ javacall_utf16*  handler_id_out,
        int* length){
return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * Enumerate registered content handler IDs that have the id parameter as a prefix
 * Search is case-sensitive
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @param id      a string used for registered content handlers searching
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeration
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 * @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_enum_handlers_by_prefix(javacall_const_utf16_string id, 
    int* pos_id, /*OUT*/ javacall_utf16* handler_id_out, int* length)
{
     return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Enumerate registered content handler IDs that are a prefix of the 'id' parameter.
 * Content handler ID equals to the 'id' parameter if exists must be included in returned sequence.
 * Search is case-sensitive
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @param id      a string used for registered content handlers searching
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeration
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 * @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_enum_handlers_prefixes_of(javacall_const_utf16_string id, 
												int* pos_id, /*OUT*/ javacall_utf16* handler_id_out, int* length)
{
     return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Enumerate all suffixes of content type data files that can be handled by given content handler or all suffixes 
 * registered in registry
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @content_handler_id unique id of content handler for which content data files suffixes are requested
 *                     if  content_handler_id is null suffixes for all registered handlers are enumerated
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 * @param suffix_out memory buffer receiving zero terminated string containing content data file suffix
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_enum_suffixes(javacall_const_utf16_string content_handler_id, int* pos_id, /*OUT*/ javacall_utf16*  suffix_out, int* length)
{
     return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Enumerate all content data types that can be handled by given content handler or all content types
 * registered in registry
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @content_handler_id unique id of content handler for which content data types are requested
 *                     if  content_handler_id is null all registered content types are enumerated
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 * @param type_out memory buffer receiving zero terminated string containing single content type
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_enum_types(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16*  type_out, int* length)
{
     return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * Enumerate all actions that can be performed by given content handler with any acceptable content
 * or all possible actions mentioned in registry for all registered content handlers and types
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @content_handler_id unique id of content handler for which possible actions are requested
 *                     if  content_handler_id is null all registered actions are enumerated
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 * @param action_out memory buffer receiving zero terminated string containing single action name
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_enum_actions(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16*  action_out, int* length)
{
     return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * Enumerate all locales for witch localized names of actions that can be performed by given handler exist in registry.
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @content_handler_id unique id of content handler for which possible locales are requested
 *                     in this method content handler id should not be null
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 * @param locale_out memory buffer receiving zero terminated string containing single supported locale
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_enum_action_locales(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16* locale_out, int* length)
{
     return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Get localized name of actions that can be performed by given handler
 *
 * @content_handler_id unique id of content handler for which localized action name is requested
 * @action standard (local neutral) name of action for which localized name is requested
 * @locale name of locale for which name of action is requested,
           locale consist of two small letters containing ISO-639 language code and two upper letters containg ISO-3166 country code devided by "-" possibly extended by variant
 * @param local_action_out memory buffer receiving zero terminated string containing local action name
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_get_local_action_name(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string action, javacall_const_utf16_string locale, javacall_utf16*  local_action_out, int* length)
{
     return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * Enumerate all caller names that accessible to invoke given handler
 * Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
 * Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
 *
 * @content_handler_id unique id of content handler for which callers list is requested
 *                     content handler id should not be null
 * @param pos_id  pointer to integer that keeps postion's information in enumeration
 *                before first call integer pointed by pos_id must be initialized to zero, 
 *                each next call it should have value returned in previous call
 *                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
 *                its value should not be interpreted by caller in any way
 *                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
 *                If function returns error value pointed by pos_id MUST not be updated
 * @param access_allowed_out memory buffer receiving zero terminated string containing single access allowed caller id
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_enum_access_allowed_callers(javacall_const_utf16_string content_handler_id, int* pos_id, /*OUT*/ javacall_utf16*  access_allowed_out, int* length)
{
     return JAVACALL_NOT_IMPLEMENTED;
}



/**
 * Get the user-friendly application name of given content handler
 *
 * @content_handler_id unique id of content handler for which application name is requested
 * @param handler_frienfly_appname_out memory buffer receiving zero terminated string containing user-friendly application name of handler
 * @param length pointer to integer initialized by caller to length of buffer,
 *               on return it set to length of data passing to buffer including the terminating zero
 *               if length of buffer is not enough to keep all data, length is set to minimum needed size and
 *               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_get_content_handler_friendly_appname(javacall_const_utf16_string content_handler_id, /*OUT*/ javacall_utf16*  handler_frienfly_appname_out, int* length)
{
     return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * Get the combined location information of given content handler
 * returns suite_id, classname, and integer flag describing handler application
 *
 * @content_handler_id unique id of content handler for which application name is requested
 *                     content handler id is case sensitive
 * @param suite_id_out buffer receiving suite_id of handler, can be null
 *                     for native platform handlers suite_id is zero
 *                     if suite_id_out is null suite_id is not retrieved 
 * @param classname_out buffer receiving zero terminated string containing classname of handler, can be null
 *                      for native platform handlers classname is full pathname to native application
 *                      if classname_out is null class name is not retrieved 
 * @param classname_len pointer to integer initialized by caller to length of classname buffer
 * @param flag_out pointer to integer receiving handler registration type, can be null
 *                 if flag_out is null registration flag is not retrieved 
 * @return JAVACALL_OK if operation was successful, 
 *         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if output buffer lenght is too small to keep result
 *         error code if failure occurs
 */
javacall_result javacall_chapi_get_handler_info(javacall_const_utf16_string content_handler_id,
				   /*OUT*/
				   javacall_utf16*  suite_id_out, int* suite_id_len,
				   javacall_utf16*  classname_out, int* classname_len,
				   javacall_chapi_handler_registration_type *flag_out)
{
     return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * Check if given caller has allowed access to invoke content handler
 *
 * @content_handler_id unique id of content handler
 * @caller_id tested caller application name
 *            caller have access to invoke handler if its id is prefixed or equal to id from access_allowed list passed to register function
 *            if access list provided during @link javacall_chapi_register_handler call was empty, all callers assumed to be trusted
 * @return JAVACALL_TRUE if caller is trusted
 *         JAVACALL_FALSE if caller is not trusted
 */
javacall_bool javacall_chapi_is_access_allowed(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string caller_id)
{
     return JAVACALL_FALSE;
}


/**
 * Check if given action is supported by given content handler
 * @content_handler_id unique id of content handler
 * @action tested action name (local neutral name)
 * @return JAVACALL_TRUE if caller is trusted
 *         JAVACALL_FALSE if caller is not trusted
 */
javacall_bool javacall_chapi_is_action_supported(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string action)
{
     return JAVACALL_FALSE;
}


/**
 * Remove all information about handler from registry
 *
 * @param content_handler_id unique ID of content handler
 * @return JAVACALL_OK if operation was successful, error code otherwise
 */
javacall_result javacall_chapi_unregister_handler(javacall_const_utf16_string content_handler_id)
{
     return JAVACALL_NOT_IMPLEMENTED;
}


/**
 * Finish enumeration call. Clean enumeration position handle
 * This method is called after caller finished to enumerate by some parameter
 * Can be used by implementation to cleanup object referenced by pos_id if required
 *
 * @param pos_id position handle used by enumeration method call, if pos_id is zero it should be ignored
 * @return nothing
 */
 void javacall_chapi_enum_finish(int pos_id){}


 
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
        /* OUT */ javacall_bool* should_exit)
{
     return JAVACALL_NOT_IMPLEMENTED;
}


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
        /* OUT */ javacall_bool* should_exit)
{
     return JAVACALL_NOT_IMPLEMENTED;
}


/*
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
        javacall_chapi_invocation_status status)
{
}


/**
 * Receives invocation request from platform. <BR>
 * This is <code>Registry.invoke()</code> substitute for Platform->Java call.
 * @param handler_id target Java handler Id
 * @param invocation filled out structure with invocation params
 * @param invoc_id assigned by JVM invocation Id for further references
 */
void javanotify_chapi_java_invoke(
        const javacall_utf16_string handler_id, 
        javacall_chapi_invocation* invocation, /* OUT */ int invoc_id)
{
}


/*
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
        /* OUT */ javacall_bool* should_exit)
{
     return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_chapi_post_message( int queueId, int msgCode, const unsigned char * bytes, size_t bytesCount, int * dataExchangeID ){
    return( JAVACALL_NOT_IMPLEMENTED );
}

javacall_result javacall_chapi_send_response( int dataExchangeID, const unsigned char * bytes, size_t bytesCount ){
    return( JAVACALL_NOT_IMPLEMENTED );
}

javacall_result javacall_chapi_select_handler( javacall_const_utf16_string action, int count, const javacall_chapi_handler_info * list, 
                                                            /* OUT */ int * handler_idx ){
    *handler_idx = 0;
    return( JAVACALL_OK );
}
