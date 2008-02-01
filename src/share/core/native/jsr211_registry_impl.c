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

#include <string.h>
#include <stdlib.h>

#include <jsrop_memory.h> 

#include "javacall_chapi_registry.h"
#include "javacall_chapi_invoke.h"
#include "jsr211_registry.h"

#define MAX_BUFFER 128

/**
 * Status code [javacall_result -> jsr211_result] transformation.
 */
#define JSR211_STATUS(status) ((status) == JAVACALL_OK? JSR211_OK: JSR211_FAILED)

/**
 * Check that getter method called in loop returned res = ERROR_BUFFER_TOO_SMALL and try to realocate this buffer
 */
#define ASSURE_BUF(buffer, len, maxlen) \
	if (res == JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL){ \
		FREE(buffer); \
		maxlen = len; \
		buffer = (jchar*) MALLOC(maxlen*sizeof(*buffer)); \
		continue; \
	}

/**
 * Append handler to single handler result buffer by handler ID
 */
#define fill_handler(id, result)  put_handler(id, result, 0);

/**
 * Append handler to array of handlers result buffer by handler ID
 */
#define append_handler(id, result)  put_handler(id, result, 1);

static int put_handler(const jchar* id, JSR211_RESULT_BUFFER * result, int append) {

	int id_sz = wcslen( id );

	jchar* classname = NULL;
	int classname_len = MAX_BUFFER;

	jchar* suite_id = NULL;
	int suite_id_len = MAX_BUFFER;
	javacall_chapi_handler_registration_type flag;
	int res;

	while (1) {
		classname = MALLOC(classname_len * sizeof(*classname));
		if (!classname) break;

		suite_id = MALLOC(suite_id_len  * sizeof(*suite_id));
		if (!suite_id) break;

		res = javacall_chapi_get_handler_info(id, suite_id, &suite_id_len,
				                            classname, &classname_len, &flag);

		if (res == JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL){
			FREE(classname); classname = NULL;
			FREE(suite_id); suite_id = NULL;
			continue;
		}

		if (res) break;

        // assert( flag != 0 );
		if (append) {
			res = jsr211_appendHandler(id, id_sz, suite_id, suite_id_len - 1,
			        classname, classname_len - 1, flag,	(JSR211_RESULT_CHARRAY)result);
		} else {
			res = jsr211_fillHandler(id, id_sz, suite_id, suite_id_len - 1,
			        classname, classname_len - 1, flag,	(JSR211_RESULT_CH)result);
		}
		break;
	}

	if (classname) FREE(classname); 
	if (suite_id) FREE(suite_id);
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
jsr211_result jsr211_register_handler(const jsr211_content_handler* ch) {

    jsr211_result status = JSR211_FAILED;
    javacall_utf16_string *types = NULL;
    javacall_utf16_string *suffixes = NULL;
    javacall_utf16_string *actions = NULL;
    javacall_utf16_string *locales = NULL;
    javacall_utf16_string *action_map = NULL;
    javacall_utf16_string *accesses = NULL;
    int n = ch->act_num * ch->locale_num; // action_map length

	status = javacall_chapi_register_handler(
						(javacall_const_utf16_string)ch->id,
						(javacall_const_utf16_string)L"Java Appliation",
						(javacall_const_utf16_string)ch->suite_id, 
						(javacall_const_utf16_string)ch->class_name,
						(javacall_chapi_handler_registration_type)ch->flag, 
						(javacall_const_utf16_string*)ch->types, ch->type_num, 
						(javacall_const_utf16_string*)ch->suffixes, ch->suff_num,
						(javacall_const_utf16_string*)ch->actions, ch->act_num, 
						(javacall_const_utf16_string*)ch->locales, ch->locale_num,
						(javacall_const_utf16_string*)ch->action_map, n, 
						(javacall_const_utf16_string*)ch->accesses, ch->access_num);

    return JSR211_STATUS(status);
}

/**
 * Deletes content handler information from a registry.
 *
 * @param handler_id content handler ID
 * @return JSR211_OK if content handler unregistered successfully
 */
jsr211_result jsr211_unregister_handler(javacall_const_utf16_string handler_id) {
	return JSR211_STATUS(javacall_chapi_unregister_handler(handler_id));
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
jsr211_result jsr211_find_handler(javacall_const_utf16_string caller_id,
                        jsr211_field key, javacall_const_utf16_string value,
                        /*OUT*/ JSR211_RESULT_CHARRAY result) {

	int pos = 0;
	jchar* buffer = NULL;
	int len, maxlen = MAX_BUFFER;
	int res = JSR211_OK;
	int enum_by_id_prefix = -1/*TRUE*/;

	buffer = (jchar*) MALLOC(maxlen*sizeof(*buffer));

	while (buffer){
		len = maxlen;
		if (key == JSR211_FIELD_TYPES) {
			res = javacall_chapi_enum_handlers_by_type(value,&pos,buffer,&len);
		} else if (key == JSR211_FIELD_SUFFIXES) {
			res = javacall_chapi_enum_handlers_by_suffix(value,&pos,buffer,&len);
		} else if (key == JSR211_FIELD_ACTIONS) {
			res = javacall_chapi_enum_handlers_by_action(value,&pos,buffer,&len);
		} else if (key == JSR211_FIELD_ID){
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

		if (!jsr211_isUniqueHandler(buffer,len - 1, result)) continue;

		if (caller_id && *caller_id) {
			if (!javacall_chapi_is_access_allowed(buffer,caller_id)) continue;
		}

		res = append_handler(buffer, result);
		if (res) break;
	} 

	javacall_chapi_enum_finish(pos);

	if (buffer == NULL) return JSR211_FAILED; //out of memory
	FREE(buffer);

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
jsr211_result jsr211_find_for_suite( javacall_const_utf16_string suite_id, 
                                            /*OUT*/ JSR211_RESULT_CHARRAY result){
	int pos = 0;
	jchar* buffer = NULL;
	int len, maxlen = MAX_BUFFER;
	int res;

	int suite_id_len = wcslen(suite_id);

	buffer = (jchar*) MALLOC(maxlen*sizeof(*buffer));

	pos=0;
	while (buffer){
		len = maxlen;
		res = javacall_chapi_enum_handlers_by_suite_id( suite_id, &pos, buffer, &len );

		ASSURE_BUF(buffer, len, maxlen);
		if (res) break;

		if (!jsr211_isUniqueHandler(buffer,len - 1, result)) continue;

		res = append_handler(buffer, result);
		if (res) break;
	} 

	javacall_chapi_enum_finish(pos);

	if (!buffer) return JSR211_FAILED; //out of memory
	FREE(buffer);

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
        javacall_const_utf16_string caller_id,
        javacall_const_utf16_string url,
        javacall_const_utf16_string action,
        /*OUT*/ JSR211_RESULT_CH result){
	
//find suffix
int pos = 0;
jchar* buffer = NULL;
int len, maxlen = MAX_BUFFER;
int res, found = 0;
javacall_const_utf16_string suffix;

buffer = (jchar*) MALLOC(maxlen*sizeof(*buffer));
if (!buffer) return JSR211_FAILED;

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

	if (buffer) FREE(buffer);

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
        javacall_const_utf16_string caller_id,
        jsr211_field field, 
        /*OUT*/ JSR211_RESULT_STRARRAY result){

    int res = JSR211_OK;

    int handlerlen, handlermaxlen = MAX_BUFFER;
    jchar* handler=NULL;
    int hpos = 0;

    int valuelen, valuemaxlen = MAX_BUFFER;
    jchar* value=NULL;
    int vpos;

    if (caller_id || (field == JSR211_FIELD_ID)){
	    handler = (jchar*) REALLOC(handler,handlermaxlen * sizeof(*handler));
	    if (!handler) {
		    return JSR211_FAILED;
	    }
    }

    if (field != JSR211_FIELD_ID){
	    value = MALLOC(valuemaxlen*sizeof(*value));
	    if (!value) {
		    if(handler) FREE(handler);
		    return JSR211_FAILED;
	    }
    }

	while (1){
		if (caller_id || (field == JSR211_FIELD_ID)){

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

	if(handler) FREE(handler);
	if(value) FREE(value);

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
        javacall_const_utf16_string caller_id,
        javacall_const_utf16_string id,
        jsr211_search_flag search_flag,
        /*OUT*/ JSR211_RESULT_CH result){

	int res = JAVACALL_CHAPI_ERROR_NOT_FOUND;

	if (search_flag==JSR211_SEARCH_EXACT){
		if (javacall_chapi_is_access_allowed(id,caller_id)){
			res = fill_handler(id, result);
		}
	} else {
		int pos = 0;
		int len, maxlen = MAX_BUFFER;
		jchar* buffer = (jchar*) MALLOC(maxlen * sizeof(*buffer));
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
		if (buffer) FREE(buffer);
	}

	return (res==JAVACALL_OK)?JSR211_OK:JSR211_FAILED;
}


static int get_action_map(javacall_const_utf16_string id, /*OUT*/ JSR211_RESULT_STRARRAY result){
	int apos = 0;
	jchar* abuffer = NULL;
	int alen, alenmax =MAX_BUFFER;

	int lpos;
	jchar* lbuffer = NULL;
	int llen, llenmax = MAX_BUFFER;

	jchar* lnbuffer = NULL;
	int lnlen, lnlenmax = MAX_BUFFER;

	int res;

	JSR211_RESULT_BUFFER locales_array = jsr211_create_result_buffer(), actions_array;
	
	abuffer = (jchar*) MALLOC(alenmax * sizeof(*abuffer));
	lbuffer = (jchar*) MALLOC(llenmax * sizeof(*lbuffer));
	lnbuffer = (jchar*) MALLOC(lnlenmax * sizeof(*lnbuffer));

	lpos=0;
	while (locales_array && abuffer && lbuffer && lnbuffer){
		llen = llenmax;
		res = javacall_chapi_enum_action_locales(id,&lpos,lbuffer,&llen);
		ASSURE_BUF(lbuffer,llen,llenmax);
		if (res) break;
		if (!jsr211_isUniqueString(lbuffer,llen - 1,0, &locales_array)){
			continue;
		}
		res = jsr211_appendString(lbuffer,llen - 1, &locales_array);

		actions_array = jsr211_create_result_buffer();
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
			if (!jsr211_isUniqueString(abuffer,alen - 1 ,0, &actions_array)){
				continue;
			}
			res = jsr211_appendString(abuffer,alen - 1, &actions_array);
			if (res) break; 

			while (lnbuffer){
				lnlen = lnlenmax;
				res = javacall_chapi_get_local_action_name(id,abuffer,lbuffer,lnbuffer,&lnlen);
				ASSURE_BUF(lnbuffer,lnlen,lnlenmax);
				break;
			}

			if (!res) 
				res = jsr211_appendString(lnbuffer,lnlen - 1, result);
			else 
				res = jsr211_appendString(abuffer,alen - 1, result);
		}

		javacall_chapi_enum_finish(apos);
		if (actions_array) jsr211_release_result_buffer(actions_array);
		if (res) break;
	}

	javacall_chapi_enum_finish(lpos);

	if (abuffer) FREE(abuffer);
	if (lbuffer) FREE(lbuffer);
	if (lnbuffer) FREE(lnbuffer);

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
jsr211_result jsr211_get_handler_field(javacall_const_utf16_string id,
                             jsr211_field field_id,
                             /*OUT*/ JSR211_RESULT_STRARRAY result){
    int pos = 0;
	jchar* buffer;
	int len, maxlen = MAX_BUFFER;
	int res;

	if (field_id == JSR211_FIELD_ACTION_MAP) {
		return get_action_map(id,result);
	}

	buffer = (jchar*) MALLOC(maxlen * sizeof(*buffer));

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

	if (!buffer) return JSR211_FAILED;
	FREE(buffer);

	return (res==JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS)?JSR211_OK:JSR211_FAILED;
}
