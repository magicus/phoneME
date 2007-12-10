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


/**
 * @file
 * @brief Content Handler Registry implementation based on POSIX file calls.
 */

#include "javacall_chapi_registry.h"
#include "javacall_chapi_invoke.h"
#include "jsr211_registry.h"
#include "jsr211_invoc.h"

#include <pcsl_memory.h> 
#include <pcsl_string.h>

#include <string.h>
#include <stdlib.h>

#define MAX_BUFFER 128

#define JSR211_MALLOC(x) pcsl_mem_malloc(x)
#define JSR211_CALLOC(x,s) pcsl_mem_calloc(x,s)
#define JSR211_FREE(x) pcsl_mem_free(x)
#define JSR211_REALLOC(x,s) pcsl_mem_realloc(x,s)

/** utility macros to fill string fields */
#define GETSTR(jc_str, p_str) \
    jc_str =  (javacall_utf16_string) (pcsl_string_is_null(p_str)? NULL: \
               pcsl_string_get_utf16_data(p_str));

/** utility macros to release string fields */
#define RELSTR(jc_str, p_str) \
    if (jc_str != NULL) { \
        pcsl_string_release_utf16_data(jc_str, p_str); \
    }


/**
 * Status code [javacall_result -> jsr211_result] transformation.
 */
#define JSR211_STATUS(status) ((status) == JAVACALL_OK? JSR211_OK: JSR211_FAILED)

/**
 * Check that getter method called in loop returned res = ERROR_BUFFER_TOO_SMALL and try to realocate this buffer
 */
#define ASSURE_BUF(buffer, len, maxlen) \
	if (res == JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL){ \
		JSR211_FREE(buffer); \
		maxlen = len; \
		buffer = (jchar*) JSR211_MALLOC(maxlen*sizeof(*buffer)); \
		continue; \
	}


/**
 * Allocate memory and fill out destination array with given PCSL strings.
 */
static jsr211_result alloc_strarray(const pcsl_string* src, int len, 
                        javacall_utf16_string** dest) {
    if (len > 0) {
        javacall_utf16_string* arr;
        const pcsl_string* ptr = src;

        arr = (javacall_utf16_string*)
                    JSR211_CALLOC(len, sizeof(javacall_utf16_string));
        if (arr == NULL) {
            return JAVACALL_OUT_OF_MEMORY;
        }

        for (*dest = arr; len--; arr++, ptr++) {
            *arr = (javacall_utf16_string)pcsl_string_get_utf16_data(ptr);
        }
    } else {
        *dest = NULL;
    }

    return JSR211_OK;
}

/**
 * Free memory allocated for string array.
 */
static void free_strarray(const pcsl_string* src, int len, 
                        javacall_utf16_string** array) {
    if (len > 0 && *array != NULL) {
        const javacall_utf16_string* arr;

        for (arr = *array;len--; arr++, src++) {
            pcsl_string_release_utf16_data(*arr, src);
        }
        JSR211_FREE((void*)*array);
    }
    *array = NULL;
}

/**
 * Append handler to single handler result buffer by handler ID
 */
#define fill_handler(id, result)  put_handler(id, (JSR211_RESULT_BUFFER) result, 0);


/**
 * Append handler to array of handlers result buffer by handler ID
 */
#define append_handler(id, result)  put_handler(id, (JSR211_RESULT_BUFFER) result, 1);


static int put_handler(const jchar* id, JSR211_RESULT_BUFFER result, int append) {

	int id_sz = wcslen( id );

	jchar* classname = 0;
	int classname_len = MAX_BUFFER;

#ifdef SUITE_ID_STRING
	jchar* suite_id = 0;
	int suite_id_len = MAX_BUFFER;
#else
	int suite_id=0;
#endif
	javacall_chapi_handler_registration_type flag;
	int res;

	while (1) {
		classname = JSR211_MALLOC(classname_len * sizeof(*classname));
		if (!classname) break;

#ifdef SUITE_ID_STRING
		suite_id = JSR211_MALLOC(suite_id_len  * sizeof(*suite_id));
		if (!suite_id) break;
#endif

		res = javacall_chapi_get_handler_info(id, 

#ifdef SUITE_ID_STRING
				suite_id, &suite_id_len,
#else
				&suite_id, 
#endif
				classname,&classname_len,&flag);

			if (res == JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL){
				JSR211_FREE(classname); classname = 0;
#ifdef SUITE_ID_STRING
				JSR211_FREE(suite_id); suite_id = 0;
#endif
				continue;
			}

			if (res) break;

			if (append) {
				res = jsr211_appendHandler(id, id_sz, 
#ifdef SUITE_ID_STRING
				suite_id, suite_id_len - 1,
#else
				suite_id, 
#endif
				classname, classname_len - 1, flag,	(JSR211_RESULT_CHARRAY)result);
			} else {
				res = jsr211_fillHandler(id, id_sz, 
#ifdef SUITE_ID_STRING
				suite_id, suite_id_len - 1,
#else
				suite_id, 
#endif
				classname, classname_len - 1, flag,	(JSR211_RESULT_CH)result);
			}
			break;
	}

	if (classname) JSR211_FREE(classname); 
#ifdef SUITE_ID_STRING
	if (suite_id) JSR211_FREE(suite_id);
#endif
	return res;
}


/**
 * Initializes content handler registry.
 *
 * @return JAVACALL_OK if content handler registry initialized successfully
 */
jsr211_result jsr211_initialize(void){
	//return JSR211_STATUS(javacall_chapi_init_registry());
	javacall_chapi_init_registry();
	return JSR211_OK;
}

/**
 * Finalizes content handler registry.
 *
 * @return JAVACALL_OK if content handler registry finalized successfully
 */
jsr211_result jsr211_finalize(void){
	javacall_chapi_finalize_registry();
	return 0;
}

/**
 * Store content handler information into a registry.
 *
 * @param handler description of a registering handler. Implementation MUST NOT 
 * retain pointed object
 * @return JSR211_OK if content handler registered successfully
 */
jsr211_result jsr211_register_handler(const JSR211_content_handler* ch) {

    jsr211_result status = JSR211_FAILED;
    javacall_utf16_string *types = NULL;
    javacall_utf16_string *suffixes = NULL;
    javacall_utf16_string *actions = NULL;
    javacall_utf16_string *locales = NULL;
    javacall_utf16_string *action_map = NULL;
    javacall_utf16_string *accesses = NULL;
    int n = ch->act_num * ch->locale_num; // action_map length

    javacall_const_utf16_string id = (javacall_utf16_string) pcsl_string_get_utf16_data(&ch->id);
    javacall_const_utf16_string class_name = (javacall_utf16_string) pcsl_string_get_utf16_data(&ch->class_name);

#ifdef SUITE_ID_STRING
	javacall_const_utf16_string suite_id = (javacall_const_utf16_string) pcsl_string_get_utf16_data(&ch->suite_id);
#else
	int suite_id = ch->suite_id;
#endif

    if (JSR211_OK == alloc_strarray(ch->types, ch->type_num, &types)
        && JSR211_OK == alloc_strarray(ch->suffixes, ch->suff_num, &suffixes)
        && JSR211_OK == alloc_strarray(ch->actions, ch->act_num, &actions)
        && JSR211_OK == alloc_strarray(ch->locales, ch->locale_num, &locales)
        && JSR211_OK == alloc_strarray(ch->action_map, n, &action_map)
        && JSR211_OK == alloc_strarray(ch->accesses, ch->access_num, &accesses))
	{
		status = javacall_chapi_register_handler(
							(javacall_const_utf16_string)id,
							(javacall_const_utf16_string)L"Java Appliation",
							 suite_id, 
							(javacall_const_utf16_string)class_name,
							(javacall_chapi_handler_registration_type)ch->flag, 
							(javacall_const_utf16_string*)types, ch->type_num, 
							(javacall_const_utf16_string*)suffixes, ch->suff_num,
							(javacall_const_utf16_string*)actions, ch->act_num, 
							(javacall_const_utf16_string*)locales, ch->locale_num,
							(javacall_const_utf16_string*)action_map, n, 
							(javacall_const_utf16_string*)accesses, ch->access_num);
    }

    pcsl_string_release_utf16_data(id, &ch->id);
    pcsl_string_release_utf16_data(class_name, &ch->class_name);
#ifdef SUITE_ID_STRING
	pcsl_string_release_utf16_data(suite_id, &ch->suite_id);
#endif
    free_strarray(ch->types, ch->type_num, &types);
    free_strarray(ch->suffixes, ch->suff_num, &suffixes);
    free_strarray(ch->actions, ch->act_num, &actions);
    free_strarray(ch->locales, ch->locale_num, &locales);
    free_strarray(ch->action_map, n, &action_map);
    free_strarray(ch->accesses, ch->access_num, &accesses);

    return JSR211_STATUS(status);
}


/**
 * Deletes content handler information from a registry.
 *
 * @param handler_id content handler ID
 * @return JSR211_OK if content handler unregistered successfully
 */
jsr211_result jsr211_unregister_handler(const pcsl_string* handler_id) {
    javacall_utf16_string id;
    jsr211_result status;

    id = (javacall_utf16_string) pcsl_string_get_utf16_data(handler_id);
    status = JSR211_STATUS(javacall_chapi_unregister_handler(id));
    pcsl_string_release_utf16_data(id, handler_id);

    return status;
}

/**
 * Searches content handler using specified key and value.
 *
 * @param caller_id calling application identifier
 * @param key search field id. Valid keys are: <ul> 
 *   <li>JSR211_CHAPI_FIELD_TYPES, <li>JSR211_CHAPI_FIELD_SUFFIXES, 
 *   <li>JSR211_CHAPI_FIELD_ACTIONS. </ul>
 * The special case of JSR211_CHAPI_FIELD_ID is used for testing new handler ID.
 * @param value search value
 * @param result the buffer for Content Handlers normalized result array. 
 *  <br>Use the @link jsr211_appendHandler() jsr211_appendHandler function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_find_handler(const pcsl_string* caller_id_,
                        jsr211_field key, const pcsl_string* value_,
                        /*OUT*/ JSR211_RESULT_CHARRAY result) {

	int pos = 0;
	jchar* buffer = 0;
	int len, maxlen = MAX_BUFFER;
	int res = JSR211_OK;
	int enum_by_id_prefix = -1/*TRUE*/;

    javacall_utf16_string caller_id = caller_id_ ? (javacall_utf16_string) pcsl_string_get_utf16_data(caller_id_) : 0;
    javacall_utf16_string value = (javacall_utf16_string) pcsl_string_get_utf16_data(value_);

	buffer = (jchar*) JSR211_MALLOC(maxlen*sizeof(*buffer));

	while (buffer){
		len = maxlen;
		if (key == JSR211_FIELD_TYPES) {
			res = javacall_chapi_enum_handlers_by_type(value,&pos,buffer,&len);
		} else if (key == JSR211_FIELD_SUFFIXES) {
			res = javacall_chapi_enum_handlers_by_suffix(value,&pos,buffer,&len);
		} else if (key == JSR211_FIELD_ACTIONS) {
			res = javacall_chapi_enum_handlers_by_action(value,&pos,buffer,&len);
		} else if ( key == JSR211_FIELD_ID){
            /* a special case: we should find all handlers names conflicted with value parameter.
               caller_id parameter should be NULL in this case
            */
			if( enum_by_id_prefix ){
				res = javacall_chapi_enum_handlers_by_prefix(value,&pos,buffer,&len);
				if( JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS == res ){
					javacall_chapi_enum_finish(pos);
					enum_by_id_prefix = 0/*FALSE*/;
					pos = 0;
					continue;
				}
			} else {
				res = javacall_chapi_enum_handlers_prefixes_of(value,&pos,buffer,&len);
			}
		}

		ASSURE_BUF(buffer,len,maxlen);

		if (res) break;

		if (!jsr211_isUniqueHandler(buffer,len - 1,result)) continue;

		if (caller_id && *caller_id) {
			if (!javacall_chapi_is_access_allowed(buffer,caller_id)) continue;
		}

		res = append_handler(buffer, result);
		if (res) break;
	} 

	javacall_chapi_enum_finish(pos);
	pcsl_string_release_utf16_data(caller_id,caller_id_);
	pcsl_string_release_utf16_data(value,value_);

	if (!buffer) return JSR211_FAILED; //out of memory
	JSR211_FREE(buffer);

	return (res==JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS)?JSR211_OK:JSR211_FAILED;
}

/**
 * Fetches handlers registered for the given suite.
 *
 * @param suite_id requested suite Id.
 * @param result the buffer for Content Handlers result array. 
 *  <br>Use the @link jsr211_appendHandler() or 
 * @link jsr211_appendHandler function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_find_for_suite(
#ifdef SUITE_ID_STRING
									const pcsl_string* suite_id_, 
#else
									int suite_id,
#endif
                        /*OUT*/ JSR211_RESULT_CHARRAY result){
	int pos = 0;
	jchar* buffer = 0;
	int len, maxlen = MAX_BUFFER;
	int res;

#ifdef SUITE_ID_STRING
	javacall_const_utf16_string suite_id = (javacall_const_utf16_string) pcsl_string_get_utf16_data(suite_id_);
	int suite_id_len = wcslen(suite_id);
#endif

	buffer = (jchar*) JSR211_MALLOC(maxlen*sizeof(*buffer));

	pos=0;
	while (buffer){
		len = maxlen;
		res = javacall_chapi_enum_handlers_by_suite_id(suite_id,&pos,buffer,&len);

		ASSURE_BUF(buffer, len,maxlen);
		if (res) break;

		if (!jsr211_isUniqueHandler(buffer,len - 1,result)) continue;

		res = append_handler(buffer, result);
		if (res) break;
	} 

	javacall_chapi_enum_finish(pos);

#ifdef SUITE_ID_STRING
	pcsl_string_release_utf16_data(suite_id,suite_id_);
#endif
	if (!buffer) return JSR211_FAILED; //out of memory
	JSR211_FREE(buffer);

	return (res==JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS)?JSR211_OK:JSR211_FAILED;
}


/**
 * Searches content handler using content URL. This function MUST define
 * content type and return default handler for this type if any.
 *
 * @param caller_id calling application identifier
 * @param url content URL
 * @param action requested action
 * @param handler output parameter - the handler conformed with requested URL 
 * and action.
 *  <br>Use the @link jsr211_fillHandler() jsr211_fillHandler function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_handler_by_URL(
        const pcsl_string* caller_id_,
        const pcsl_string* url_,
        const pcsl_string* action_,
        /*OUT*/ JSR211_RESULT_CH result){
	
//find suffix
int pos = 0;
jchar* buffer = 0;
int len, maxlen = MAX_BUFFER;
int res, found = 0;

javacall_const_utf16_string caller_id, url, action, suffix;

buffer = (jchar*) JSR211_MALLOC(maxlen*sizeof(*buffer));
if (!buffer) return JSR211_FAILED;

caller_id = caller_id_ ? (javacall_utf16_string) pcsl_string_get_utf16_data(caller_id_) : 0;
url = (javacall_utf16_string) pcsl_string_get_utf16_data(url_);
action = (javacall_utf16_string) pcsl_string_get_utf16_data(action_);

suffix=(javacall_const_utf16_string)wcsrchr(url,'.');

// enum by suffix
while (buffer && suffix && !found){
		len = maxlen;
		res = javacall_chapi_enum_handlers_by_suffix(suffix,&pos,buffer,&len);

		ASSURE_BUF(buffer, len,maxlen);

		if (res) break;

		if (caller_id && *caller_id) {
			if (!javacall_chapi_is_access_allowed(buffer,caller_id)) continue;
		}

		if (action != NULL && !javacall_chapi_is_action_supported(buffer,action)) continue;

		res = fill_handler(buffer, result);
		if (res) break;

		found = 1;
	} 
	javacall_chapi_enum_finish(pos);

	if (caller_id_) pcsl_string_release_utf16_data(caller_id,caller_id_);
	pcsl_string_release_utf16_data(url,url_);
	pcsl_string_release_utf16_data(action,action_);

	if (buffer) JSR211_FREE(buffer);

	return (found)?JSR211_OK:JSR211_FAILED;

}


/**
 * Returns all found values for specified field. Tha allowed fields are: <ul>
 *    <li> JSR211_CHAPI_FIELD_ID, <li> JSR211_CHAPI_FIELD_TYPES, <li> JSR211_CHAPI_FIELD_SUFFIXES,
 *    <li> and JSR211_CHAPI_FIELD_ACTIONS. </ul>
 * Values should be selected only from handlers accessible for given caller_id.
 *
 * @param caller_id calling application identifier.
 * @param field search field id
 * @param result output structure where result is placed to.
 *  <br>Use the @link jsr211_appendString() jsr211_appendString function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_get_all(
        const pcsl_string* caller_id_,
        jsr211_field field, 
        /*OUT*/ JSR211_RESULT_STRARRAY result){

int res = JSR211_OK;
javacall_const_utf16_string caller_id=0;

int handlerlen, handlermaxlen = MAX_BUFFER;
jchar* handler=0;
int hpos = 0;

int valuelen, valuemaxlen = MAX_BUFFER;
jchar* value=0;
int vpos;

if (caller_id_ || (field == JSR211_FIELD_ID)){
	handler = (jchar*) JSR211_REALLOC(handler,handlermaxlen * sizeof(*handler));
	if (!handler) {
		return JSR211_FAILED;
	}
}

if (field != JSR211_FIELD_ID){
	value = JSR211_MALLOC(valuemaxlen*sizeof(*value));
	if (!value) {
		if(handler) JSR211_FREE(handler);
		return JSR211_FAILED;
	}
}

if (caller_id_){
	caller_id = (javacall_utf16_string) pcsl_string_get_utf16_data(caller_id_);
	if (!caller_id) res = JSR211_FAILED;
}

	while (1){
		if (caller_id_ || (field == JSR211_FIELD_ID)){

			handlerlen = handlermaxlen;

			res = javacall_chapi_enum_handlers(&hpos,handler,&handlerlen);

			ASSURE_BUF(handler, handlerlen,handlermaxlen);
			if (!handler) break;

			if (res) break;

			if (caller_id) {
				if (!javacall_chapi_is_access_allowed(handler,caller_id)) continue;
			}

			if (field == JSR211_FIELD_ID) {
				res = jsr211_appendUniqueString(handler,handlerlen - 1, 0 ,result);
				continue;
			}
		}
		

		vpos = 0;
		if (!res) while (1){

			valuelen = valuemaxlen;

			if (field == JSR211_FIELD_ACTIONS) {
				res = javacall_chapi_enum_actions(handler,&vpos, value, &valuelen);
			} else if (field == JSR211_FIELD_TYPES) {
				res = javacall_chapi_enum_types(handler,&vpos, value, &valuelen);
			} else if (field == JSR211_FIELD_SUFFIXES) {
				res = javacall_chapi_enum_suffixes(handler,&vpos, value, &valuelen);
			}

			ASSURE_BUF(value, valuelen,valuemaxlen);			
			if (!value) break;

			if (res) break;

			res = jsr211_appendUniqueString(value,valuelen - 1,field == JSR211_FIELD_ACTIONS, result);
			if (res) break;
		}
		javacall_chapi_enum_finish(vpos);

		if (!handler) break;
	} 
	javacall_chapi_enum_finish(hpos);

	if(handler) JSR211_FREE(handler);
	if(value) JSR211_FREE(value);
	if(caller_id) pcsl_string_release_utf16_data(caller_id,caller_id_);

	return (handler && value && (res==JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS))?JSR211_OK:JSR211_FAILED;
}

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
 *  <br>Use the @link jsr211_fillHandler() jsr211_fillHandler function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_get_handler(
        const pcsl_string* caller_id_,
        const pcsl_string* id_,
        jsr211_search_flag search_flag,
        /*OUT*/ JSR211_RESULT_CH result){

	int res = JAVACALL_CHAPI_ERROR_NOT_FOUND;
	javacall_const_utf16_string caller_id, id;

	caller_id = (javacall_utf16_string) pcsl_string_get_utf16_data(caller_id_);
	id = (javacall_utf16_string) pcsl_string_get_utf16_data(id_);

	if (search_flag==JSR211_SEARCH_EXACT){
		if (javacall_chapi_is_access_allowed(id,caller_id)){
			res = fill_handler(id, result);
		}
	} else {
		int pos = 0;
		jchar* buffer;
		int len, maxlen = MAX_BUFFER;
		buffer = (jchar*) JSR211_MALLOC(maxlen * sizeof(*buffer));
		while (buffer){
			len = maxlen;
            res = javacall_chapi_enum_handlers_prefixes_of(id, &pos, buffer, &len);
			ASSURE_BUF(buffer,len,maxlen);
			if (res) break;
            if (javacall_chapi_is_access_allowed(buffer, caller_id)){
			    res = fill_handler(buffer, result);
            }
			break;
		}
		javacall_chapi_enum_finish(pos);
		if (buffer) JSR211_FREE(buffer);
	}

	pcsl_string_release_utf16_data(caller_id,caller_id_);
	pcsl_string_release_utf16_data(id,id_);

	return (res==JAVACALL_OK)?JSR211_OK:JSR211_FAILED;
}


static int get_action_map(const pcsl_string* id_,
						  /*OUT*/ JSR211_RESULT_STRARRAY result){
	int apos = 0;

	jchar* abuffer = 0;
	int alen, alenmax =MAX_BUFFER;

	int lpos;
	jchar* lbuffer = 0;
	int llen, llenmax = MAX_BUFFER;

	
	jchar* lnbuffer = 0;
	int lnlen, lnlenmax = MAX_BUFFER;

	int res;

	javacall_const_utf16_string id;

	JSR211_RESULT_STRARRAY locales_array = (JSR211_RESULT_STRARRAY)jsr211_create_result_buffer();
	JSR211_RESULT_STRARRAY actions_array;
	
	id = (javacall_utf16_string) pcsl_string_get_utf16_data(id_);

	

	abuffer = (jchar*) JSR211_MALLOC(alenmax * sizeof(*abuffer));
	lbuffer = (jchar*) JSR211_MALLOC(llenmax * sizeof(*lbuffer));
	lnbuffer = (jchar*) JSR211_MALLOC(lnlenmax * sizeof(*lnbuffer));

	lpos=0;
	while (locales_array && abuffer && lbuffer && lnbuffer){
		llen = llenmax;
		res = javacall_chapi_enum_action_locales(id,&lpos,lbuffer,&llen);
		ASSURE_BUF(lbuffer,llen,llenmax);
		if (res) break;
		if (!jsr211_isUniqueString(lbuffer,llen - 1,0,locales_array)){
			continue;
		}
		res = jsr211_appendString(lbuffer,llen - 1,locales_array);

		actions_array = (JSR211_RESULT_STRARRAY)jsr211_create_result_buffer();
		if (!actions_array){
			res = JAVACALL_CHAPI_ERROR_NO_MEMORY;
			break;
		}
		apos=0;
		while (abuffer && lnbuffer){
			alen = alenmax;
			res = javacall_chapi_enum_actions(id,&apos,abuffer,&alen);
			ASSURE_BUF(abuffer,alen,alenmax);
			if (res) {
				if (res==JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS) res = JAVACALL_OK;
				break;
			}
			if (!jsr211_isUniqueString(abuffer,alen - 1 ,0,actions_array)){
				continue;
			}
			res = jsr211_appendString(abuffer,alen - 1,actions_array);
			if (res) break; 

			while (lnbuffer){
				lnlen = lnlenmax;
				res = javacall_chapi_get_local_action_name(id,abuffer,lbuffer,lnbuffer,&lnlen);
				ASSURE_BUF(lnbuffer,lnlen,lnlenmax);
				break;
			}

			if (!res) 
				res = jsr211_appendString(lnbuffer,lnlen - 1,result);
			else 
				res = jsr211_appendString(abuffer,alen - 1,result);

		}

		javacall_chapi_enum_finish(apos);
		if (actions_array) jsr211_release_result_buffer(actions_array);
		if (res) break;
	}

	javacall_chapi_enum_finish(lpos);

	pcsl_string_release_utf16_data(id,id_);
	if (abuffer) JSR211_FREE(abuffer);
	if (lbuffer) JSR211_FREE(lbuffer);
	if (lnbuffer) JSR211_FREE(lnbuffer);

	if (locales_array) jsr211_release_result_buffer(locales_array);

	return (res==JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS)?JSR211_OK:JSR211_FAILED;
}

/**
 * Loads the handler's data field. Allowed fields are: <UL>
 *  <LI> JSR211_FIELD_TYPES, <LI> JSR211_FIELD_SUFFIXES, 
 *  <LI> JSR211_FIELD_ACTIONS, <LI> JSR211_FIELD_LOCALES, 
 *  <LI> JSR211_FIELD_ACTION_MAP, <LI> and JSR211_FIELD_ACCESSES. </UL>
 *
 * @param id requested handler ID.
 * @param field_id requested field.
 * @param result output structure where requested array is placed to.
 *  <br>Use the @link jsr211_appendString() jsr211_appendString function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_get_handler_field(const pcsl_string* id_,
                             jsr211_field field_id,
                             /*OUT*/ JSR211_RESULT_STRARRAY result){

	int pos = 0;
	jchar* buffer;
	int len, maxlen = MAX_BUFFER;
	int res;
	javacall_const_utf16_string id;


	if (field_id == JSR211_FIELD_ACTION_MAP) {
		return get_action_map(id_,result);
	}

	id = (javacall_utf16_string) pcsl_string_get_utf16_data(id_);	
	buffer = (jchar*) JSR211_MALLOC(maxlen * sizeof(*buffer));

	pos=0;
	while (buffer){
		len = maxlen;

		if (field_id == JSR211_FIELD_TYPES){
			res = javacall_chapi_enum_types(id,&pos,buffer,&len);
		} else if (field_id == JSR211_FIELD_SUFFIXES){
			res = javacall_chapi_enum_suffixes(id,&pos,buffer,&len);
		} else if (field_id == JSR211_FIELD_ACTIONS){
			res = javacall_chapi_enum_actions(id,&pos,buffer,&len);
		} else if (field_id == JSR211_FIELD_LOCALES){
			res = javacall_chapi_enum_action_locales(id,&pos,buffer,&len);
		} else if (field_id == JSR211_FIELD_ACCESSES){
			res = javacall_chapi_enum_access_allowed_callers(id,&pos,buffer,&len);
		}

		ASSURE_BUF(buffer,len, maxlen);
		if (res) break;
		if (field_id == JSR211_FIELD_ACCESSES){
                  res = jsr211_appendString(buffer,len - 1, result);
		} else {
		   res = jsr211_appendUniqueString(buffer,len - 1,field_id != JSR211_FIELD_SUFFIXES && field_id != JSR211_FIELD_TYPES,result);
		}
		if (res != JAVACALL_OK) break;
	}
	javacall_chapi_enum_finish(pos);

    pcsl_string_release_utf16_data(id,id_);

	if (!buffer) return JSR211_FAILED;
	JSR211_FREE(buffer);

	return (res==JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS)?JSR211_OK:JSR211_FAILED;
}

/**
 * Executes specified non-java content handler.
 * The current implementation provides executed handler 
 * only with URL and action invocation parameters.
 *
 * @param handler_id content handler ID
 * @return codes are following
 * <ul>
 * <li> JSR211_LAUNCH_OK or JSR211_LAUNCH_OK_SHOULD_EXIT if content handler 
 *   started successfully
 * <li> other code from the enum according to error codition
 * </ul>
 */
jsr211_launch_result jsr211_execute_handler(const pcsl_string* _id) {
	(void)_id;
    return JSR211_LAUNCH_ERR_NOTSUPPORTED;
}
