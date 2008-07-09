/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 * This module contains installation methods for registration of internal 
 * handlers.
 */ 

#include <jsr211_registry.h>
#include <jsrop_memory.h>
#include <jsrop_logging.h>
#include <jsrop_suitestore.h>

/** 
 * Include preinstalled content handlers data consisted of:
 *
 * static int nHandlers; // number of preinstalled handlers
 * static pcsl_string* handlerIds[]; // handler IDs as pcsl_strings.
 * static char* rowHandlers[]; // serialized handler parameters in form:
 *  - each handler is serialized in a multi-line string.
 *  - each line of the string is a handler's field, 
 *    the order of the fields is predefined as following:
 *         <suite_ID> -- if this line is empty, the handler's flag is NATIVE
 *         <classname> -- if the handler's flag is NATIVE here is executive path
 *         <type1 type2 type3 ...> -- array of types devided by whitespases
 *         <suffix1 suffix2 suffix3 ...> -- suffixes (see types)
 *         <locale1 locale2 locale3 ...> -- locales (see types)
 *         <actionName11 actionName21 ...> -- action_name_i_j is for
 *                                                  action_i and locale_j
 *         <access1 access2 access3 ...> -- accesses (see types)
 */

#if ENABLE_NATIVE_AMS

static int nHandlers = 0;
static const jchar** handlerIds = 0;
static char** rowHandlers = 0;

#else

#define INTERNAL_SUIT_ID "-1"

static int nHandlers = 1;

static const jchar* handlerIds[] = { 
	L"GraphicalInstaller" // The ID of the GraphicalInstaller handler
};

static jchar* rowHandlers[] = {
    L"com.sun.midp.installer.GraphicalInstaller\0"
	L"text/vnd.sun.j2me.app-descriptor\0application/java-archive\0\0"    
    L".jad\0.jar\0\0"
    L"install\0remove\0\0"
    L"en\0de\0ru\0\0"
    L"Install\0Remove\0" // en
	L"Installieren\0Umziehen\0" // de
    L"\x0423\x0441\x0442\x0430\x043D\x043E\x0432\x0438\x0442\x044C\0\x0423\x0434\x0430\x043B\x0438\x0442\x044C\0\0" // ru
	L"\0" // empty access list
};

#endif


/**
 * Extract string [str] from the jchar buffer [ptr].
 * The string is limited by '\0'.
 * After deserialization ptr is moved right after '\0'
 * @return 0 if failed.
 */
static const jchar* getString(const jchar** ptr) {
	const jchar* p; 
	const jchar* pstart = p = *ptr;
    while (*(p++));
	*ptr = p;
    return pstart;
}

/**
 * Allocate and fill string array [arr] from the jchar buffer [ptr]. 
 * The array is limited by double '\0'
 * Each string entry delimited by a single '\0 '.
 * After deserialization ptr is moved right after double '\0'
 * @return allocated string list or 0 if list is empty
 */
static int fillArray(const jchar **ptr, /*OUT*/int* len, const jchar*** arr) {
	const jchar* p = *ptr;
	const jchar** list;
	//count array size
	*len = 0;
	while (*getString(&p)) ++(*len);

	if (!(*len)) {
		*arr = NULL;
		return 1;
	}

	//alloc array list
	list = *arr = (const jchar**)JAVAME_MALLOC((*len)*sizeof(jchar*));
	if (!*arr) return 0;
	
	//asign elements
	p = *ptr;
	while (*p){
		*(list++) = getString(&p);
	} 

	*ptr = p+1;

	return 1;
}

/**
 * The actual installation of the content handler
 * @return <code>JSR211_OK</code> if the installation completes successfully.
 */
static jsr211_result installHandler(int n) {
    jsr211_content_handler ch = JSR211_CONTENT_HANDLER_INITIALIZER;
    jchar *ptr = rowHandlers[n];
    jsr211_result status;
	int anm_num;
	jchar *intrenalSuiteID;

    intrenalSuiteID = JAVAME_MALLOC(
        (jsrop_suiteid_string_size(INTERNAL_SUITE_ID)+1) * sizeof(jchar));

	if (!jsrop_suiteid_to_string(INTERNAL_SUITE_ID, intrenalSuiteID)){
		return JSR211_FAILED;
	}

/*
 *  Fill up CH data:
 *         ID from handlerIds.
 *  Others from rowHandlers:
 *         <suite_ID> -- if this line is empty, the handler's flag is NATIVE
 *         <classname> -- if the handler's flag is NATIVE here is executive path
 *         <type1 type2 type3 ...> -- array of types devided by whitespases
 *         <suffix1 suffix2 suffix3 ...> -- suffixes (see types)
 *         <locale1 locale2 locale3 ...> -- locales (see types)
 *         <actionName11 actionName21 ...> -- action_name_i_j is for
 *                                                  action_i and locale_j
 *         <access1 access2 access3 ...> -- accesses (see types)
 */
	ch.id = handlerIds[n];
	ch.suite_id = intrenalSuiteID;
	ch.class_name = getString(&ptr);
	ch.flag = JSR211_REGISTER_TYPE_STATIC_FLAG; // non-native, statically registered

	// allocate parameters
	if (!(fillArray(&ptr, &ch.type_num, &ch.types) &&
		fillArray(&ptr, &ch.suff_num, &ch.suffixes) &&
		fillArray(&ptr, &ch.act_num, &ch.actions) &&
		fillArray(&ptr, &ch.locale_num, &ch.locales) &&
		fillArray(&ptr, &anm_num, &ch.action_map)&&
		anm_num == ch.act_num*ch.locale_num && //check
		fillArray(&ptr, &ch.access_num, &ch.accesses))){

#if REPORT_LEVEL <= LOG_CRITICAL
	    REPORT_CRIT(LC_NONE, "jsr211_deploy.c: handler data parsing failed");
#endif

		status = JSR211_FAILED;
	} else {
		// register handler
		status = jsr211_register_handler(&ch);
	}

	//clean string lists
	if (ch.types) JAVAME_FREE(ch.types);
	if (ch.suffixes) JAVAME_FREE(ch.suffixes);
	if (ch.actions) JAVAME_FREE(ch.actions);
	if (ch.locales) JAVAME_FREE(ch.locales);
	if (ch.action_map) JAVAME_FREE(ch.action_map);
	if (ch.accesses) JAVAME_FREE(ch.accesses);

	JAVAME_FREE(intrenalSuiteID);
    
    return status;
}

/**
 * Checks whether the internal handlers, if any, are installed.
 * @return JSR211_OK or JSR211_FAILED - if registry corrupted or OUT_OF_MEMORY.
 */
jsr211_result jsr211_check_internal_handlers(void) {
    int i, found;
    for (i = 0; i < nHandlers; i++) {
		found = javacall_chapi_is_access_allowed(handlerIds[i], NULL);
        if (!found) {
            if (JSR211_OK != installHandler(i)) {
                return JSR211_FAILED;
            }
        }
    }
    return JSR211_OK;
}

