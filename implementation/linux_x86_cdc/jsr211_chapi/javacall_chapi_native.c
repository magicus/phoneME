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
 * @file
 * @brief Content Handler Registry implementation based on POSIX file calls.
 */

#include "javacall_chapi_native.h"
#include "javacall_chapi_callbacks.h"

#include "javacall_registry.h"
#include "javacall_invoke.h"

#include <string.h>
#include <stdlib.h>

#define MAX_BUFFER 128

/**
 * Initializes content handler registry.
 *
 * @return JAVACALL_OK if content handler registry initialized successfully
 */
javacall_result javacall_chapi_native_initialize(void){
	return init_registry();
}

/**
 * Finalizes content handler registry.
 *
 * @return JAVACALL_OK if content handler registry finalized successfully
 */
javacall_result javacall_chapi_native_finalize(void){
	return finalize_registry();
}

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
        const javacall_utf16_string* accesses,  int nAccesses){

	int result = register_handler(	(const unsigned short*) id,
									L"Phone ME Application",							  
									(const unsigned short*) suite_id,
									(const unsigned short*) class_name,
									flag,
									(const unsigned short**) types,   nTypes,
									(const unsigned short**) suffixes,  nSuffixes,
									(const unsigned short**) actions,   nActions,  
									(const unsigned short**) locales,   nLocales,
									(const unsigned short**) action_names, nActionNames,
									(const unsigned short**) accesses,  nAccesses,
									0, 0, 0 );


	return (!result)?JAVACALL_OK:JAVACALL_FAIL;
}

/**
 * Deletes content handler information from a registry.
 *
 * @param id content handler ID
 * @return operation status.
 */
javacall_result javacall_chapi_native_unregister_handler(const javacall_utf16_string id){
	int result = unregister_handler((const unsigned short*) id);
	return (!result)?JAVACALL_OK:JAVACALL_FAIL;
}

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
        /*OUT*/ javacall_chapi_result_CH_array result){

	int pos = 0;
	unsigned short* buffer;
	int len, maxlen = MAX_BUFFER;
	int res;

	buffer = (unsigned short*) malloc(maxlen);
	if (!buffer) return JAVACALL_FAIL;

	pos=0;
	while (1){
		len = maxlen;
		if (key == JAVACALL_CHAPI_FIELD_TYPES) {
			res = enum_handlers_by_type(value,&pos,buffer,&len);
		} else if (key == JAVACALL_CHAPI_FIELD_SUFFIXES) {
			res = enum_handlers_by_suffix(value,&pos,buffer,&len);
		} else if (key == JAVACALL_CHAPI_FIELD_ACTIONS) {
			res = enum_handlers_by_action(value,&pos,buffer,&len);
		}
		if (len>maxlen){
			unsigned short* tmp= (unsigned short*) realloc(buffer,len);
			if (!tmp) break;
			maxlen = len;
			buffer = tmp;
			continue;
		}
		if (res) 
			break;
		else {
			unsigned short suite_id[MAX_BUFFER];
			int suite_id_len = MAX_BUFFER;
			unsigned short classname[MAX_BUFFER];
			int classname_len = MAX_BUFFER;
			int flag;

			res=get_handler_info(buffer,suite_id,&suite_id_len,classname,&classname_len,&flag);
			if (res) break;
			
			res = javautil_chapi_appendHandler(buffer,len, suite_id, suite_id_len, classname, classname_len, flag,
											result);
			if (res) break;
		}
	} 


	free(buffer);

	return (res==ERROR_NO_MORE_ELEMENTS)?JAVACALL_OK:JAVACALL_FAIL;
}

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
                        /*OUT*/ javacall_chapi_result_CH_array result){
	int pos = 0;
	unsigned short* buffer;
	int len, maxlen = MAX_BUFFER;
	int res;

	buffer = (unsigned short*) malloc(maxlen);
	if (!buffer) return JAVACALL_FAIL;

	pos=0;
	while (1){
		len = maxlen;
		res = enum_handlers_by_suit_id(suite_id,&pos,buffer,&len);
		if (len>maxlen){
			unsigned short* tmp= (unsigned short*) realloc(buffer,len);
			if (!tmp) break;
			maxlen = len;
			buffer = tmp;
			continue;
		}
		if (res) 
			break;
		else {
			int suite_id_len = wcslen(suite_id);
			unsigned short classname[MAX_BUFFER];
			int classname_len = MAX_BUFFER;
			int flag;

			res=get_handler_info(buffer,0, 0,classname,&classname_len,&flag);
			if (res) break;
			
			res = javautil_chapi_appendHandler(buffer,len, suite_id, suite_id_len, classname, classname_len, flag,
											result);
			if (res) break;
		}
	} 


	free(buffer);

	return (res==ERROR_NO_MORE_ELEMENTS)?JAVACALL_OK:JAVACALL_FAIL;
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
 *  <br>Use the @link javautil_chapi_fillHandler() javautil_chapi_fillHandler function to fill this structure.
 * @return status of the operation
 */
javacall_result javacall_chapi_native_handler_by_URL(
        const javacall_utf16_string caller_id,
        const javacall_utf16_string url,
        const javacall_utf16_string action,
        /*OUT*/ javacall_chapi_result_CH handler){

//find suffix
int pos = 0;
unsigned short* buffer;
int len, maxlen = MAX_BUFFER;
int res, found = 0;
const short* suffix=wcsrchr(url,'.');

if (!suffix) return JAVACALL_FAIL;

buffer = (unsigned short*) malloc(maxlen);
if (!buffer) return JAVACALL_FAIL;

// enum by suffix
while (!found){
		len = maxlen;
		res = enum_handlers_by_suffix(suffix,&pos,buffer,&len);
		if (len>maxlen){
			unsigned short* tmp= (unsigned short*) realloc(buffer,len);
			if (!tmp) break;
			maxlen = len;
			buffer = tmp;
			continue;
		}
		if (res) {
			break;
		} else {
			unsigned short suite_id[MAX_BUFFER];
			int suite_id_len = MAX_BUFFER;
			unsigned short classname[MAX_BUFFER];
			int classname_len = MAX_BUFFER;
			int flag;

			res = is_action_supported(buffer,action);
			if (!res) continue;

			res = is_trusted(buffer,caller_id);
			if (!res) continue;


			res=get_handler_info(buffer,suite_id,&suite_id_len,classname,&classname_len,&flag);
			if (res) break;
			
			res = javautil_chapi_fillHandler(buffer,len, suite_id, suite_id_len, classname, classname_len, flag,
											handler);
			if (res) break;
			found = 1;
		}
	} 


	free(buffer);

	return (found)?JAVACALL_OK:JAVACALL_FAIL;

}


static javacall_result javacall_chapi_native_get_all_handlers(
        const javacall_utf16_string caller_id,
        /*OUT*/ javacall_chapi_result_str_array result){

	int pos = 0;
	unsigned short* buffer;
	int len, maxlen = MAX_BUFFER;
	int res;
	


	buffer = (unsigned short*) malloc(maxlen);
	if (!buffer) return JAVACALL_FAIL;

	pos=0;
	while (1){
		len = maxlen;
		res = enum_handlers(&pos,buffer,&len);

		if (len>maxlen){
			unsigned short* tmp = (unsigned short*) realloc(buffer,len);
			if (!tmp) break;
			maxlen = len;
			buffer = tmp;
			continue;
		}

		if (res) break;


		if (is_trusted(buffer,caller_id)){
			res = javautil_chapi_appendString(buffer,len,result);
			if (res != JAVACALL_OK) break;
		}
	}


	free(buffer);

	return (res==ERROR_NO_MORE_ELEMENTS)?JAVACALL_OK:JAVACALL_FAIL;
}


static javacall_result javacall_chapi_native_get_all_actions(
        const javacall_utf16_string caller_id,
        /*OUT*/ javacall_chapi_result_str_array result){

	int pos = 0;
	unsigned short* buffer;
	int len, maxlen = MAX_BUFFER;
	int res;
	


	buffer = (unsigned short*) malloc(maxlen);
	if (!buffer) return JAVACALL_FAIL;

	pos=0;
	while (1){
		len = maxlen;
		res = enum_handlers(&pos,buffer,&len);

		if (len>maxlen){
			unsigned short* tmp = (unsigned short*) realloc(buffer,len);
			if (!tmp) break;
			maxlen = len;
			buffer = tmp;
			continue;
		}

		if (res) break;


		if (is_trusted(buffer,caller_id)){
			res = javacall_chapi_native_get_handler_field(buffer,JAVACALL_CHAPI_FIELD_ACTIONS, result);
			if (res != JAVACALL_OK) break;
		}
	}


	free(buffer);

	return (res==ERROR_NO_MORE_ELEMENTS)?JAVACALL_OK:JAVACALL_FAIL;

}


static javacall_result javacall_chapi_native_get_all_suffixes_or_types(const javacall_utf16_string caller_id,
											/*OUT*/ javacall_chapi_result_str_array result, int suffixes){
	int pos = 0;
	unsigned short* buffer;
	int len, maxlen = MAX_BUFFER;

	int hpos=0;
	unsigned short* hbuffer;
	int hlen, hmaxlen = MAX_BUFFER;

	int res;
	int checked;

	buffer = (unsigned short*) malloc(maxlen);
	if (!buffer) return JAVACALL_FAIL;

	hbuffer = (unsigned short*) malloc(hmaxlen);
	if (!hbuffer) {
		free(buffer);
		return JAVACALL_FAIL;
	}

	pos=0;
	while (1){
		checked = (caller_id==0);
		len = maxlen;
		if (suffixes){
			res = enum_suffixes(0,&pos,buffer,&len);
		} else {
			res = enum_types(0,&pos,buffer,&len);
		}
		if (len>maxlen){
			unsigned short* tmp= (unsigned short*) realloc(buffer,len);
			if (!tmp) break;
			maxlen = len;
			buffer = tmp;
			continue;
		}
		if (res) break;
		
		
		//check if some handler for this suffix is accessible by caller
			hpos=0;
			while (!checked){
				hlen = maxlen;
				if (suffixes){
					res = enum_handlers_by_suffix(buffer,&hpos,hbuffer,&hlen);
				} else {
					res = enum_handlers_by_type(buffer,&hpos,hbuffer,&hlen);
				}
				if (len>maxlen){
					unsigned short* tmp= (unsigned short*) realloc(hbuffer,hlen);
					if (!tmp) break;
					hmaxlen = hlen;
					hbuffer = tmp;
					continue;
				}
				if (res) break;
				checked=is_trusted(hbuffer,caller_id);
			}
			
		if (checked) javautil_chapi_appendString(buffer,len,result);
		
	} 

	free(hbuffer);
	free(buffer);

	return (res==ERROR_NO_MORE_ELEMENTS)?JAVACALL_OK:JAVACALL_FAIL;
}


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
        /*OUT*/ javacall_chapi_result_str_array result){

int res;

if (field == JAVACALL_CHAPI_FIELD_ID) {
			res = javacall_chapi_native_get_all_handlers(caller_id,result);
		} else if (field == JAVACALL_CHAPI_FIELD_ACTIONS) {
			res = javacall_chapi_native_get_all_actions(caller_id,result);
		} else if (field == JAVACALL_CHAPI_FIELD_TYPES) {
			res = javacall_chapi_native_get_all_suffixes_or_types(caller_id,result,0);
		} else if (field == JAVACALL_CHAPI_FIELD_SUFFIXES) {
			res = javacall_chapi_native_get_all_suffixes_or_types(caller_id,result,1);
		}

	return res;

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
 *  <br>Use the @link javautil_chapi_fillHandler() javautil_chapi_fillHandler function to fill this structure.
 * @return status of the operation
 */
javacall_result javacall_chapi_native_get_handler(
        const javacall_utf16_string caller_id,
        const javacall_utf16_string id,
        javacall_chapi_search_flag search_flag,
        /*OUT*/ javacall_chapi_result_CH result){

int res;
unsigned short suite_id[MAX_BUFFER];
int suite_id_len = MAX_BUFFER;
unsigned short classname[MAX_BUFFER];
int classname_len = MAX_BUFFER;
int flag;

if (search_flag==JAVACALL_CHAPI_SEARCH_EXACT){
	if (is_trusted(id,caller_id)){
			res=get_handler_info(id,suite_id,&suite_id_len,classname,&classname_len,&flag);
			if (!res) {
				res = javautil_chapi_fillHandler(id, wcslen(id), suite_id, suite_id_len, classname, classname_len, flag,
												result);
			}
	}
} else {

	int pos = 0;
	unsigned short* buffer;
	int len, maxlen = MAX_BUFFER;
	int res;
	int found = 0;
	int prefixlen = wcslen(id);
	


	buffer = (unsigned short*) malloc(maxlen);
	if (!buffer) return JAVACALL_FAIL;

	pos=0;
	while (1){
		len = maxlen;
		res = enum_handlers(&pos,buffer,&len);

		if (len>maxlen){
			unsigned short* tmp = (unsigned short*) realloc(buffer,len);
			if (!tmp) break;
			maxlen = len;
			buffer = tmp;
			continue;
		}

		if (res) break;


		if (!memcmp(buffer,id,sizeof(*id)*prefixlen) && is_trusted(buffer,caller_id)){
			res=get_handler_info(buffer,suite_id,&suite_id_len,classname,&classname_len,&flag);
			if (!res) {
				res = javautil_chapi_fillHandler(buffer,len, suite_id, suite_id_len, classname, classname_len, flag,
												result);
			}
			break;
		}
	}


	free(buffer);


}

return res;
}


static int get_action_map(const javacall_utf16_string id,
						  /*OUT*/ javacall_chapi_result_str_array result){
	int apos = 0;

	unsigned short abuffer[MAX_BUFFER];
	int alen = MAX_BUFFER;

	int lpos;
	unsigned short lbuffer[MAX_BUFFER];
	int llen = MAX_BUFFER;

	
	unsigned short lnbuffer[MAX_BUFFER];
	int lnlen = MAX_BUFFER;

	int res;

	apos=0;
	while (1){
		alen = MAX_BUFFER;
		res = enum_actions(id,&apos,abuffer,&alen);
		if (res) break;
		lpos=0;
		while (1){
			llen = MAX_BUFFER;
			res = enum_action_locales(id,&lpos,lbuffer,&llen);
			if (res) break;
			lnlen = MAX_BUFFER;
			res = get_local_action_name(id,abuffer,lbuffer,lnbuffer,&lnlen);
			if (!res) 
				res = javautil_chapi_appendString(lnbuffer,lnlen,result);
			else 
				res = javautil_chapi_appendString(L"",0,result);
		}
	}

	return (res==ERROR_NO_MORE_ELEMENTS)?JAVACALL_OK:JAVACALL_FAIL;
}

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
javacall_result javacall_chapi_native_get_handler_field(const javacall_utf16_string id,
												 javacall_chapi_field key,
												 /*OUT*/ javacall_chapi_result_str_array result){

	int pos = 0;
	unsigned short* buffer;
	int len, maxlen = MAX_BUFFER;
	int res;

	if (key == JAVACALL_CHAPI_FIELD_ACTION_MAP) return get_action_map(id,result);
	

	buffer = (unsigned short*) malloc(maxlen);
	if (!buffer) return JAVACALL_FAIL;

	pos=0;
	while (1){
		len = maxlen;

		if (key == JAVACALL_CHAPI_FIELD_TYPES){
			res = enum_types(id,&pos,buffer,&len);
		} else if (key == JAVACALL_CHAPI_FIELD_SUFFIXES){
			res = enum_suffixes(id,&pos,buffer,&len);
		} else if (key == JAVACALL_CHAPI_FIELD_ACTIONS){
			res = enum_actions(id,&pos,buffer,&len);
		} else if (key == JAVACALL_CHAPI_FIELD_LOCALES){
			res = enum_action_locales(id,&pos,buffer,&len);
		} else if (key == JAVACALL_CHAPI_FIELD_ACCESSES){
			res = enum_trusted_callers(id,&pos,buffer,&len);
		}


		if (len>maxlen){
			unsigned short* tmp = (unsigned short*) realloc(buffer,len);
			if (!tmp) break;
			maxlen = len;
			buffer = tmp;
			continue;
		}

		if (res) break;


		res = javautil_chapi_appendString(buffer,len,result);
		if (res != JAVACALL_OK) break;
	}


	free(buffer);

	return (res==ERROR_NO_MORE_ELEMENTS)?JAVACALL_OK:JAVACALL_FAIL;
}

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
            /*OUT*/ int* exec_status){
int result = 0;
return result;
}


