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

#include <string.h>

#include <midpAMS.h>
#include <midpMalloc.h>
#include <midpString.h>
#include <midpStorage.h>
#include <midpJar.h>
#include <midp_logging.h>
#include <jvm.h>

#include <jsr211_registry.h>
#include <midpUtilKni.h>

#include <jsrop_memory.h>

/**
 * @file
 * This module contains the MIDP stack NAMS mode extensions for file installation
 * and other procedures.
 */


/**
 * Variable for error codes generated from jsr211 functions.
 * Possible values:
 * a) from JSR 211 specification [p16] to notify as installation status report
 *   938 // -- Content handler conflicts with other handlers
 *   939 // -- Content handler install failed
 *   910 // -- Application authorization failure
 *   905 // -- Attribute Mismatch
 * b) other standard MIDP error codes
 *   OUT_OF_MEMORY
 */
int jsr211_errCode;

/**
 * Buffer for parsed handlers
 */
jsr211_content_handler *handlers = NULL;
int nHandlers = 0;


/**
 * Defined in regstore.c
 */
extern void jsr211_cleanHandlerData(jsr211_content_handler *handler);

/**
 * Clean up whole array of handlers.
 */
static void cleanup(void) {

    if (nHandlers > 0) {
        jsr211_content_handler *ptr;
        int n;

        for (n = nHandlers, ptr = handlers; n > 0; n--, ptr++) {
            jsr211_cleanHandlerData(ptr);
        }

        midpFree(handlers);
        handlers = NULL;
        nHandlers = 0;
    }
}

/**
 * Prepares a string in MicroEdition-Handler-n format
 *
 * @param handlerNumber this is the MicroEdition-Handler-n number
 * @param result the produced handler-tag string. Should be released after use.
 * @return 0 if failed
 */
static int prepareHandlerTag(int handlerNumber, /*OUT*/pcsl_string* result) {
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(handlerTag)
        {'M', 'i', 'c', 'r', 'o', 'E', 'd', 'i', 't', 'i', 'o', 'n', '-',
         'H', 'a', 'n', 'd', 'l', 'e', 'r', '-', '\0'}
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(handlerTag);
    pcsl_string_status status;
    pcsl_string num = PCSL_STRING_NULL_INITIALIZER;

    status = pcsl_string_convert_from_jint(handlerNumber, &num);
    if (status == PCSL_STRING_OK) {
        status = pcsl_string_cat(&handlerTag, &num, result);
        pcsl_string_free(&num);
    }
    return (status == PCSL_STRING_OK? 1: 0);
}


/**
 * Prepares a full file path for class identified by its name.
 *
 * @param classname processed class name
 * @param path prepared path for JAR entry test
 * @return 0 if failed
 */
static int getFullClassPath(const pcsl_string* classname,
                                                    /*OUT*/pcsl_string* path) {
    static const jchar suff[] = {'.','c','l','a','s','s'};
    int sz, n = 0;
    jchar *ptr, *buf;

    sz = pcsl_string_utf16_length(classname) + sizeof(suff) / sizeof(jchar);
    buf = (jchar*) MALLOC(sz * sizeof(jchar));
    if (buf == NULL) {
        return 0;
    }

    if (PCSL_STRING_OK == pcsl_string_convert_to_utf16(classname, buf, sz, &n)) {
        ptr = buf;
        while (n--) {
            if (*ptr == '.') {
                *ptr = '/';
            }
            ptr++;
        }
        memcpy(ptr, suff, sizeof(suff));
        n = (PCSL_STRING_OK == pcsl_string_convert_from_utf16(buf, sz, path)?
            1: 0);
    }
    FREE(buf);

    return n;
}

/**
 * Generates Content Handler ID from Suite properties in form:
 * [VENDOR_PROP]-[SUITE_NAME_PROP]-[classname]
 *
 * @param mp installing suite properties
 * @param classname installing Content Handler's classname
 * @param defaultID generated ID
 *
 * @return 0 if failed
 */
static int getDefaultID(MidpProperties mp, const jchar* clazzname,
                                            /*OUT*/pcsl_string* defaultID) {
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(defaultVendor)
    {'s', 'y', 's', 't', 'e', 'm', '\0'}
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(defaultVendor);
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(defaultApp)
    {'A', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '\0'}
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(defaultApp);
    const pcsl_string* vendor = &PCSL_STRING_NULL;
    const pcsl_string* app = &PCSL_STRING_NULL;
	pcsl_string classname;
	int result;
	if (PCSL_STRING_OK != pcsl_string_convert_from_utf16(clazzname, wcslen(clazzname), classname)) return 0;

    vendor = midp_find_property(&mp, &SUITE_VENDOR_PROP);
    if (vendor == NULL || pcsl_string_length(vendor) <= 0)
        vendor = &defaultVendor;

    app = midp_find_property(&mp, &SUITE_NAME_PROP);
    if (app == NULL || pcsl_string_length(app) <= 0)
        app = &defaultApp;

    pcsl_string_predict_size(defaultID, pcsl_string_length(vendor) +
            pcsl_string_length(app) + pcsl_string_length(classname) + 3);

    result = (PCSL_STRING_OK == pcsl_string_append(defaultID, vendor) &&
            PCSL_STRING_OK == pcsl_string_append_char(defaultID, '-') &&
            PCSL_STRING_OK == pcsl_string_append(defaultID, app) &&
            PCSL_STRING_OK == pcsl_string_append_char(defaultID, '-') &&
            PCSL_STRING_OK == pcsl_string_append(defaultID, classname)? 1: 0);

	pcsl_string_free(&classname);
	return result;
}

/**
 * Parses string, delimited by commas, from the attributes line.
 * @param cur_idx moved to the next token or set to -1 if no more tokens.
 * @return 0 if failed
 */
static int parseString(const pcsl_string* src,
                       /*IN-OUT*/int* cur_idx, /*OUT*/const jchar** dest, jsize * lenOut) {
    pcsl_string temp = PCSL_STRING_NULL_INITIALIZER;
    int from = *cur_idx;
    int n = pcsl_string_index_of_from(src, ',', from);

    if (n < 0) { // no more tokens
        n = pcsl_string_length(src);
        *cur_idx = -1;
    } else {
        *cur_idx = n + 1;
    }
    if (n > 0) {
        if (PCSL_STRING_OK == pcsl_string_substring(src, from, n, &temp)) {
			pcsl_string dst = PCSL_STRING_NULL_INITIALIZER;
            if (PCSL_STRING_OK == pcsl_string_trim(&temp, &dst)) {
				jsize length = pcsl_string_length(&dst);
				jchar* buf = (jchar*)MALLOC(length+1);
				if (*dest) {
					if (PCSL_STRING_OK == pcsl_string_convert_to_utf16(&dst,buf,length+1,NULL)){
						*dest = buf;
						if (lenOut) *lenOut=length;
						n = 0;
					}
				}
				pcsl_string_free(&dst);
            }
            pcsl_string_free(&temp);
        }
    }
    return n == 0;
}

/**
 * Parses string array, delimited by white spaces, from the attributes line.
 * @param arr_len if it is set to 0 number of array items is counted
 * @param arr if it is NULL the memory for array is allocated
 * @return 0 if failed
 */
static int parseArray(const jchar* src, int len,
                       /*OUT*/int* arr_len, /*OUT*/const jchar*** arr, jchar** arrVar) {
    const jchar *buf, *end, *p0, *p1;
	jchar** str;
    int n = *arr_len, wsp, wsp_mode;

    buf = src;
    end = buf + len;

    if (n == 0) {   // count array entries
        wsp_mode = 1; // start with 'white space' mode
        p0 = buf;
        while (p0 < end) {
            wsp = (*p0 > ' '? 0: 1);
            if (wsp_mode != wsp) {
                if (!wsp)
                    n++; // new entry is started
                wsp_mode = wsp;
            }
            p0++;
        }
    }

    while (n > 0) {
		if (arrVar) {
			str = arrVar;
		} else {
            str = (jchar**)CALLOC(n, sizeof(jchar*));
            if (str == NULL)
                break;
            *arr_len = n;
            *arr = (const jchar**) str;
        }

        // actual array filling
        p0 = buf;
        while (n > 0) {
            while (*p0 <= ' ') p0++;
            p1 = p0 + 1;
            while (p1 < end  && *p1 > ' ') p1++;
			*str = (jchar*)MALLOC((p1 - p0 + 1) * sizeof(jchar));
			if (!str) break;
			while (p0 <= p1) **str = *p0++;
            str++;
            n--;
        }
    }

    return n == 0;
}

/**
 * Performs parsing attributes for found content handlers accordingly with
 * JSR 211 specification.
 * @param handler previously allocated buffer for parsed handler
 * @param mp suite MIDP properties bundle - the source of attributes
 * @param handlerN N-th handlers template string which will be used as prefix
 * for other searched attributes names
 * @return 1 - data is filled, 0 - no such handler, -1 - error on parsing
 */
static int parseHandler(jsr211_content_handler* handler, MidpProperties mp,
                                                        pcsl_string* handlerN) {
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(id_suff)
    {'I', 'D', '\0'}
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(id_suff);
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(access_suff)
    {'A', 'c', 'c', 'e', 's', 's', '\0'}
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(access_suff);

    pcsl_string attrName = PCSL_STRING_NULL_INITIALIZER;
    jchar* temp = NULL;
    const pcsl_string* attr = &PCSL_STRING_NULL;
    int n, len=0;

    // look up 'MicroEdition-Handler-<n>' attribute
    attr = midp_find_property(&mp, handlerN);
    if (attr == NULL || pcsl_string_length(attr) <= 0)
        return 0; // no handler

    // Parse: <classname>[, <type(s)>[, <suffix(es)>[, <action(s)>[, <locale(s)>]]]]
    n = 0;
    if (!parseString(attr, &n, &handler->class_name, &len) || !len) return -1;
    do {
        if (n <= 0)
            break;

        if (!parseString(attr, &n, &temp, &len)) return -1;
        if (!parseArray(temp, len, &handler->type_num, &handler->types, NULL)){
			FREE(temp);
            return -1;
		}
		FREE(temp);

        if (n < 0)
            break;

        if (!parseString(attr, &n, &temp, &len)) return -1;
        if (!parseArray(temp, len, &handler->suff_num, &handler->suffixes, NULL)){
			FREE(temp);
            return -1;
        }
		FREE(temp);

        if (n < 0)
            break;
        if (!parseString(attr, &n, &temp, &len) ||
            !parseArray(temp, len, &handler->act_num, &handler->actions, NULL))
            return -1;
        FREE(temp);

        if (n > 0 && handler->act_num > 0) {
            if (!parseString(attr, &n, &temp, &len) ||
                !parseArray(temp, len, &handler->locale_num, &handler->locales, NULL))
                return -1;
            FREE(temp);
        }
    } while (0);

    // apprnd extra-hyphen
    if (PCSL_STRING_OK != pcsl_string_append_char(handlerN, '-'))
        return -1;

    // look up 'MicroEdition-Handler-<n>-ID' attribute
    if (PCSL_STRING_OK != pcsl_string_cat(handlerN, &id_suff, &attrName))
        return -1;
    attr = midp_find_property(&mp, &attrName);
    pcsl_string_free(&attrName);
    if (attr != NULL && (len=pcsl_string_length(attr)) > 0) {
		if (!(handler->id = MALLOC(sizeof(jchar)*(len+1))) return -1;
		if (PCSL_STRING_OK != pcsl_string_convert_to_utf16(&attr, handler->id, len+1, NULL))
            return -1;
    } else {
        if (!getDefaultID(mp, handler->class_name, &handler->id))
            return -1;
    }

    // look up 'MicroEdition-Handler-<n>-Access' attribute
    if (PCSL_STRING_OK != pcsl_string_cat(handlerN, &access_suff, &attrName))
        return -1;
    attr = midp_find_property(&mp, &attrName);
    pcsl_string_free(&attrName);
    if (attr != NULL && pcsl_string_length(attr) > 0 &&
        !parseArray(attr, &handler->access_num, &handler->accesses))
        return -1;

    if (handler->locale_num > 0) {
        // look up 'MicroEdition-Handler-<n>-<locale>' attributes
        int act_num = handler->act_num;
        pcsl_string *map;

        map = (pcsl_string*) CALLOC(
                        act_num * handler->locale_num, sizeof(pcsl_string));
        if (map == NULL)
            return -1;
        handler->action_map = map;
        for (n = 0; n < handler->locale_num; n++, map += act_num) {
            if (PCSL_STRING_OK !=
                    pcsl_string_cat(handlerN, handler->locales + n, &attrName))
                return -1;
            attr = midp_find_property(&mp, &attrName);
            pcsl_string_free(&attrName);
            if (attr == NULL ||
                pcsl_string_length(attr) <= 0 ||
                !parseArray(attr, &act_num, &map))
                return -1;
        }
    }

    return 1;
}


/**
 * Finds JSR 211 declared attributes, verifies its and
 * prepares Content Handlers structures for registration.
 *
 * @param jadsmp JAD properties
 * @param mfsmp Manifest properties
 * @param jarHandle processed jar handle for class presence test
 * @param trusted whether installing suit is trusted
 *
 * @return number of Conntent Handlers found
 * or -1 in case of error. Error code is placed to jsr211_errCode variable.
 */
int jsr211_verify_handlers(MidpProperties jadsmp, MidpProperties mfsmp,
                                            void* jarHandle, jchar trusted) {
    int count=0;
    int res;
    pcsl_string handlerN = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string path = PCSL_STRING_NULL_INITIALIZER;
    const pcsl_string* handlerAttr = &PCSL_STRING_NULL;
    const pcsl_string* testAttr = &PCSL_STRING_NULL;
    MidpProperties mp; /* attribute values source */
    MidpProperties *testMp = NULL;  /* test for matching if needed */
    JSR211_content_handler *ptr;
    JSR211_RESULT_CHARRAY testHnd = { NULL, 0 };

    if (nHandlers != 0) {
        return -1;  /* Suite installation might be only once in the NAMS. */
    }

    if (jadsmp.numberOfProperties == 0) {
        mp = mfsmp;    /* get attributes from the JAR manifest */
    } else {
        mp = jadsmp;
        if (trusted) {
            /* test whether attribute value match manifest's one */
            testMp = &mfsmp;
        }
    }

    jsr211_errCode = 0;

    /* count handlers */
    res = 1; /* 'res' is an iteration flag */
    count = 1;
    while (res) {
        if (!prepareHandlerTag(count, &handlerN)) {
            jsr211_errCode = OUT_OF_MEMORY;
            return -1; // No memory
        }
        handlerAttr = midp_find_property(&mp, &handlerN);
        if (pcsl_string_length(handlerAttr) <= 0) {
            count--;
            res = 0; /* stop iteration */
        } else if (testMp != NULL) {
            /* test attributes matching for JAD and JAR manifest */
            testAttr = midp_find_property(testMp, &handlerN);
            if (!pcsl_string_equals(handlerAttr, testAttr)) {
                jsr211_errCode = 905; /* -- Attribute Mismatch */
                res = 0; /* stop iteration */
            }
        }
        pcsl_string_free(&handlerN);
    }

    if (jsr211_errCode != 0) {
        return -1;
    }

    if (count <= 0) {
        return 0;   /* no handlers found */
    }

    if (jsr211_initialize() != JSR211_OK) {
        return -1; /* JSR 211 initialization failed. */
    }

    handlers = (JSR211_content_handler*)
                    CALLOC(count, sizeof(JSR211_content_handler));
    if (handlers == NULL) {
        jsr211_errCode = OUT_OF_MEMORY;
        return -1; // No memory
    }
    nHandlers = count;
    ptr = handlers;

    for (count = 1; count <= nHandlers; count++, ptr++) {
        if (!prepareHandlerTag(count, &handlerN)) {
            res = -1;
            break;
        }

        res = parseHandler(ptr, mp, &handlerN);
        pcsl_string_free(&handlerN);
        if (!res) {
            res = -1;
            break;
        }

        if (!getFullClassPath(&ptr->class_name, &path)) {
            res = -1;
            break;
        }
        res = (midpJarEntryExists(jarHandle, &path)? 0: -1);
        pcsl_string_free(&path);
        if (res < 0)
            break;

        jsr211_find_handler(&PCSL_STRING_NULL, JSR211_FIELD_ID, &ptr->id,
                                            &testHnd);
        if (testHnd.buf != NULL) {
            // -- Content handler conflicts with other handlers
            jsr211_errCode = 938;
            FREE(testHnd.buf);
            testHnd.buf = NULL;
            testHnd.len = 0;
            res = -1;
            break;
        }

        ptr->flag = JSR211_FLAG_COMMON;
    }

    if (res < 0) {
        if (jsr211_errCode == 0) {
            jsr211_errCode = OUT_OF_MEMORY;
        }
        cleanup();
        return -1;
    }

    return nHandlers;
}


/**
 * Stores installing Content Handlers in the JSR211 complaint registry.
 * @param suiteId installing suite Id
 * @return 0 if handlers are stored successfully.
 */
int jsr211_store_handlers(SuiteIdType suiteId) {
    int n = nHandlers;
    JSR211_content_handler *ptr = handlers;

    while (n > 0) {
        ptr->suite_id = suiteId;
        if (JSR211_OK != jsr211_register_handler(ptr)) {
            n = 0;
        }
        ptr++;
        n--;
    }
    jsr211_finalize();
    cleanup();

    return n;
}

/**
 * Removes registered Content Handlers from the JSR211 Registry.
 * @param suiteId removing suite Id
 */
void jsr211_remove_handlers(SuiteIdType suiteId) {
    JSR211_RESULT_CHARRAY handlers = _JSR211_RESULT_INITIALIZER_;

    jsr211_find_for_suite(suiteId, &handlers);
    if (handlers.buf != NULL) {
        jchar* buf = handlers.buf;
        int n = *buf++;
        int sz;
        pcsl_string id;
        while (n--) {
            sz = *buf++;
            if (PCSL_STRING_OK != pcsl_string_convert_from_utf16(buf + 1, *buf, &id))
                    break;
            jsr211_unregister_handler(&id);
            pcsl_string_free(&id);
            buf += sz;
        }
        FREE(handlers.buf);
    }
}
