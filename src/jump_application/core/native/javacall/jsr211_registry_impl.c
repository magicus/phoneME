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
 * @brief Content Handler Registry implementation based on javacall.
 */

#ifdef ENABLE_JSR211_REGISTRY_IMPL

#include "jsr211_registry.h"
#include "jsr211_invoc.h"
#include "javacall_chapi.h"
#include "javacall_memory.h"
#include "pcsl_memory.h" // PCSL memory should be used for output result buffers
#include <string.h>

/**
 * NULL-Initializer for struct _JAVAUTIL_CHAPI_RESBUF_
 */
#define _JAVAUTIL_CHAPI_RESBUF_INIT_  { NULL, 0, 0 }

/**
 * Status code [javacall_result -> jsr211_result] transformation.
 */
#define JSR211_STATUS(status) ((status) == JAVACALL_OK? JSR211_OK: JSR211_FAILED)

/**
 * Transform field value from 'jsr211_field' to 'javacall_chapi_field' enum.
 */
#define JAVACALL_FIELD(jsr211_field) \
    (jsr211_field == 0? 0: jsr211_field + JAVACALL_CHAPI_FIELD_CLASS)

/**
 * Copy result data from javacall buffer into pointer of JSR211 buffer.
 */
#define COPY_RESULT_DATA(javacall_buffer, jsr211_buffer_ptr) \
    jsr211_buffer_ptr->buf = javacall_buffer.buf; \
    jsr211_buffer_ptr->len = javacall_buffer.used

/**
 * Free result data in case of failed function status.
 */
#define FREE_RESULT_DATA(javacall_buffer) \
    if (javacall_buffer.buf != NULL) { \
        pcsl_mem_free(javacall_buffer.buf); \
    } \
    javacall_buffer.buf = NULL

/**
 * The constant length of the suite ID string. 
 * See GET_SUITE_ID_LEN() in <suitestore_common.h>
 */
#define _SUITE_ID_LEN 8

/**
 * Transforms javacall suite ID as string into MIDP suite ID as jint.
 */
static SuiteIdType get_midp_suite(const javacall_utf16* jc_suite, int sz) {
    jint ret = 0;
    
    if (sz == _SUITE_ID_LEN) {
        int hex;
        const javacall_utf16 *p = jc_suite;

        while (sz-- > 0) {
            if (ret != 0 || *p != '0') {
                hex = *p > '9'? *p - 'A' + 0xA: *p - '0';
                if (hex < 0 || hex > 0xF) {
                    ret = 0;
                    break; // Invalid suite ID
                } 

                ret <<= 4;
                ret |= hex;
            }
            p++;
        }
        
    }

    return ret;
}

/**
 * Transforms MIDP suite ID as jint into javacall suite ID as string.
 * Note! The return value is static buffer of size _SUITE_ID_LEN,
 *       Never free it!
 */
const javacall_utf16_string get_jc_suite(SuiteIdType midp_suite) {
    static javacall_utf16 buffer[_SUITE_ID_LEN+1];
    javacall_utf16* ptr = buffer + _SUITE_ID_LEN;
    int hex;

    *ptr = 0; // put Guard Zero at end
    while (ptr-- > buffer) {
        hex = midp_suite & 0xF;
        *ptr = (hex < 0xA? hex + '0': hex - 0xA + 'A');
        midp_suite >>= 4;
    }
    return (const javacall_utf16_string)buffer;
}

/**
 * Allocate memory and fill out destination array with given PCSL strings.
 */
static javacall_result alloc_strarray(const pcsl_string* src, int len, 
                        const javacall_utf16_string** dest) {
    if (len > 0) {
        javacall_utf16_string* arr;
        const pcsl_string* ptr = src;

        arr = (javacall_utf16_string*)
                    pcsl_mem_calloc(len, sizeof(javacall_utf16_string));
        if (arr == NULL) {
            return JAVACALL_OUT_OF_MEMORY;
        }

        for (*dest = arr; len--; arr++, ptr++) {
            *arr = (javacall_utf16_string)pcsl_string_get_utf16_data(ptr);
        }
    } else {
        *dest = NULL;
    }

    return JAVACALL_OK;
}

/**
 * Free memory allocated for string array.
 */
static void free_strarray(const pcsl_string* src, int len, 
                        const javacall_utf16_string** array) {
    if (len > 0 && *array != NULL) {
        const javacall_utf16_string* arr;

        for (arr = *array;len--; arr++, src++) {
            pcsl_string_release_utf16_data(*arr, src);
    }
        pcsl_mem_free(*array);
    }
    *array = NULL;
}

/**
 * Initializes content handler registry.
 *
 * @return JSR211_OK if content handler registry initialized successfully
 */
jsr211_result jsr211_initialize(void) {
     return JSR211_STATUS(javacall_chapi_initialize());
}

/**
 * Finalizes content handler registry.
 *
 * @return JSR211_OK if content handler registry finalized successfully
 */
jsr211_result jsr211_finalize(void) {
    return JSR211_STATUS(javacall_chapi_finalize());
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
    javacall_utf16_string suite_id; // javacall suiteId
    javacall_utf16_string id, class_name;
    javacall_utf16_string *types = NULL;
    javacall_utf16_string *suffixes = NULL;
    javacall_utf16_string *actions = NULL;
    javacall_utf16_string *locales = NULL;
    javacall_utf16_string *action_map = NULL;
    javacall_utf16_string *accesses = NULL;
    int n = ch->act_num * ch->locale_num; // action_map length

    id = (javacall_utf16_string) pcsl_string_get_utf16_data(&ch->id);
    suite_id = get_jc_suite(ch->suite_id);
    class_name = (javacall_utf16_string) pcsl_string_get_utf16_data(&ch->class_name);

    if (JAVACALL_OK == alloc_strarray(ch->types, ch->type_num, &types)
     && JAVACALL_OK == alloc_strarray(ch->suffixes, ch->suff_num, &suffixes)
     && JAVACALL_OK == alloc_strarray(ch->actions, ch->act_num, &actions)
     && JAVACALL_OK == alloc_strarray(ch->locales, ch->locale_num, &locales)
     && JAVACALL_OK == alloc_strarray(ch->action_map, n, &action_map)
     && JAVACALL_OK == alloc_strarray(ch->accesses, ch->access_num, &accesses)
     && JAVACALL_OK == javacall_chapi_register_handler(id, suite_id, 
                        class_name, ch->flag, types, ch->type_num, 
                        suffixes, ch->suff_num, actions, ch->act_num, 
                        locales, ch->locale_num, action_map, n, 
                        accesses, ch->access_num)) {
         status = JSR211_OK;
    }

    pcsl_string_release_utf16_data(id, &ch->id);
    pcsl_string_release_utf16_data(class_name, &ch->class_name);

    free_strarray(ch->types, ch->type_num, &types);
    free_strarray(ch->suffixes, ch->suff_num, &suffixes);
    free_strarray(ch->actions, ch->act_num, &actions);
    free_strarray(ch->locales, ch->locale_num, &locales);
    free_strarray(ch->action_map, n, &action_map);
    free_strarray(ch->accesses, ch->access_num, &accesses);

    return status;
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
 *   <li>JSR211_FIELD_TYPES, <li>JSR211_FIELD_SUFFIXES, 
 *   <li>JSR211_FIELD_ACTIONS. </ul>
 * The special case of JSR211_FIELD_ID is used for testing new handler ID.
 * @param value search value
 * @param result the buffer for Content Handlers result array. 
 *  <br>Use @link jsr211_fillHandlerArray function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_find_handler(const pcsl_string* _caller_id,
                        jsr211_field key, const pcsl_string* _value,
                        /*OUT*/ JSR211_RESULT_CHARRAY* result) {
    jsr211_result status;
    javacall_utf16_string caller_id;
    javacall_utf16_string value;
    _JAVAUTIL_CHAPI_RESBUF_ resbuf = _JAVAUTIL_CHAPI_RESBUF_INIT_;

    caller_id = (javacall_utf16_string) pcsl_string_get_utf16_data(_caller_id);
    value = (javacall_utf16_string) pcsl_string_get_utf16_data(_value);
    status = JSR211_STATUS(javacall_chapi_find_handler(caller_id, 
                            JAVACALL_FIELD(key), value, &resbuf));

    if (status == JSR211_OK) {
        COPY_RESULT_DATA(resbuf, result);
    } else {
        FREE_RESULT_DATA(resbuf);
    }

    pcsl_string_release_utf16_data(caller_id, _caller_id);
    pcsl_string_release_utf16_data(value, _value);
    return status;
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
    jsr211_result status;
    javacall_utf16_string suite_id;
    _JAVAUTIL_CHAPI_RESBUF_ resbuf = _JAVAUTIL_CHAPI_RESBUF_INIT_;

    suite_id = get_jc_suite(suiteId);
    status = JSR211_STATUS(javacall_chapi_find_for_suite(suite_id, &resbuf));

    if (status == JSR211_OK) {
        COPY_RESULT_DATA(resbuf, result);
    } else {
        FREE_RESULT_DATA(resbuf);
    }

    return status;
}

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
jsr211_result jsr211_handler_by_URL(const pcsl_string* _caller_id, 
                        const pcsl_string* _url, const pcsl_string* _action, 
                        /*OUT*/ JSR211_RESULT_CH* handler) {
    jsr211_result status;
    javacall_utf16_string caller_id;
    javacall_utf16_string url;
    javacall_utf16_string action;
    _JAVAUTIL_CHAPI_RESBUF_ resbuf = _JAVAUTIL_CHAPI_RESBUF_INIT_;

    caller_id = (javacall_utf16_string) pcsl_string_get_utf16_data(_caller_id);
    url = (javacall_utf16_string) pcsl_string_get_utf16_data(_url);
    action = (javacall_utf16_string) pcsl_string_get_utf16_data(_action);
    status = JSR211_STATUS(
            javacall_chapi_handler_by_URL(caller_id, url, action, &resbuf));

    if (status == JSR211_OK) {
        COPY_RESULT_DATA(resbuf, handler);
    } else {
        FREE_RESULT_DATA(resbuf);
    }

    pcsl_string_release_utf16_data(caller_id, _caller_id);
    pcsl_string_release_utf16_data(url, _url);
    pcsl_string_release_utf16_data(action, _action);
    return status;
}

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
jsr211_result jsr211_get_all(const pcsl_string* _caller_id, jsr211_field key,
                        /*OUT*/ JSR211_RESULT_STRARRAY* result) {
    jsr211_result status;
    javacall_utf16_string caller_id;
    _JAVAUTIL_CHAPI_RESBUF_ resbuf = _JAVAUTIL_CHAPI_RESBUF_INIT_;

    caller_id = (javacall_utf16_string) pcsl_string_get_utf16_data(_caller_id);
    status = JSR211_STATUS(
            javacall_chapi_get_all(caller_id, JAVACALL_FIELD(key), &resbuf));

    if (status == JSR211_OK) {
        COPY_RESULT_DATA(resbuf, result);
    } else {
        FREE_RESULT_DATA(resbuf);
    }

    pcsl_string_release_utf16_data(caller_id, _caller_id);
    return status;
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
 * @param mode flag indicating whether exact or prefixed search should be 
 * performed.
 * @param handler output value - requested handler.
 *  <br>Use @link jsr211_fillHandler function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_get_handler(const pcsl_string* _caller_id, 
                        const pcsl_string* _id, jsr211_search_flag mode,
                        /*OUT*/ JSR211_RESULT_CH* handler) {
    jsr211_result status;
    javacall_utf16_string caller_id;
    javacall_utf16_string id;
    _JAVAUTIL_CHAPI_RESBUF_ resbuf = _JAVAUTIL_CHAPI_RESBUF_INIT_;

    caller_id = (javacall_utf16_string) pcsl_string_get_utf16_data(_caller_id);
    id = (javacall_utf16_string) pcsl_string_get_utf16_data(_id);
    status = JSR211_STATUS(
                javacall_chapi_get_handler(caller_id, id, mode, &resbuf));

    if (status == JSR211_OK) {
        COPY_RESULT_DATA(resbuf, handler);
    } else {
        FREE_RESULT_DATA(resbuf);
    }

    pcsl_string_release_utf16_data(caller_id, _caller_id);
    pcsl_string_release_utf16_data(id, _id);
    return status;
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
 *  <br>Use @link jsr211_fillStringArray function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_get_handler_field(const pcsl_string* _id, jsr211_field key,
                        /*OUT*/ JSR211_RESULT_STRARRAY* result) {
    jsr211_result status;
    javacall_utf16_string id;
    _JAVAUTIL_CHAPI_RESBUF_ resbuf = _JAVAUTIL_CHAPI_RESBUF_INIT_;

    id = (javacall_utf16_string) pcsl_string_get_utf16_data(_id);
    status = JSR211_STATUS(
        javacall_chapi_get_handler_field(id, JAVACALL_FIELD(key), &resbuf));

    if (status == JSR211_OK) {
        COPY_RESULT_DATA(resbuf, result);
    } else {
        FREE_RESULT_DATA(resbuf);
    }

    pcsl_string_release_utf16_data(id, _id);
    return status;
}


/** utility macros to fill string fields */
#define SETSTR(jc_str, p_str) \
    jc_str =  (javacall_utf16_string) (pcsl_string_is_null(p_str)? NULL: \
               pcsl_string_get_utf16_data(p_str));

/** utility macros to release string fields */
#define RELSTR(jc_str, p_str) \
    if (jc_str != NULL) { \
        pcsl_string_release_utf16_data(jc_str, p_str); \
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
    javacall_utf16_string id;
    javacall_chapi_invocation invoc;
    jsr211_launch_result ret;
    StoredInvoc* inv;
    int i;
    pcsl_string* pcsl_args;
    javacall_utf16_string* args;

    inv = jsr211_get_invocation(_id);
    if (inv == NULL) {
        return JSR211_LAUNCH_ERR_NO_INVOCATION;
    }

    do {
        id = (javacall_utf16_string) pcsl_string_get_utf16_data(_id);

        memset(&invoc, 0, sizeof(invoc));
        invoc.tid = inv->tid;
        SETSTR(invoc.url, &inv->url)
        SETSTR(invoc.type, &inv->type)
        SETSTR(invoc.action, &inv->action)
        SETSTR(invoc.invokingAppName, &inv->invokingAppName)
        SETSTR(invoc.invokingAuthority, &inv->invokingAuthority)
        SETSTR(invoc.username, &inv->username)
        SETSTR(invoc.password, &inv->password)
        if (inv->argsLen > 0) {
            i = inv->argsLen;
            invoc.argsLen = i;
            invoc.args = args = (javacall_utf16_string*)
                            pcsl_mem_calloc(i, sizeof(javacall_utf16_string));
            if (args == NULL) {
                ret = JSR211_LAUNCH_ERROR;
                break;
            }
            for (pcsl_args = inv->args; i--; pcsl_args++, args++) {
                SETSTR(*args, pcsl_args);
            }
        }
        invoc.dataLen = inv->dataLen;
        invoc.data = inv->data;

        switch (javacall_chapi_execute_handler(id, &invoc, &ret)) {
            case JAVACALL_OK:
                break;
            case JAVACALL_FILE_NOT_FOUND:
                ret = JSR211_LAUNCH_ERR_NO_HANDLER;
                break;
            default:
                ret = JSR211_LAUNCH_ERROR;
        }
    } while(0);

    pcsl_string_release_utf16_data(id, _id);
    RELSTR(invoc.url, &inv->url)
    RELSTR(invoc.type, &inv->type)
    RELSTR(invoc.action, &inv->action)
    RELSTR(invoc.invokingAppName, &inv->invokingAppName)
    RELSTR(invoc.invokingAuthority, &inv->invokingAuthority)
    RELSTR(invoc.username, &inv->username)
    RELSTR(invoc.password, &inv->password)

    if (invoc.argsLen > 0) {
        if (invoc.args != NULL) {
            i = invoc.argsLen;
            args = invoc.args;
            for (pcsl_args = inv->args; i--; args++, pcsl_args++) {
                if (*args != NULL)
                    pcsl_string_release_utf16_data(*args, pcsl_args);
            }
            pcsl_mem_free((void*)invoc.args);
        }
    }

    return ret;
}



/**
 * CHAPI javautil_jsr211_* functions implementation and 
 * some extended string comparision utilities.
 */


/**
 * Determines buffer size for serialization of the handler data
 */
#define GET_CH_SIZE(id_size, clas_size) \
    id_size + class_name_size + 5

/**
 * Serializes handler data into buffer.
 * Variable <code>buf</code> after macros comletion points at the end of 
 * filled area.
 */
static void fill_ch_buf(const javacall_utf16* id, int id_size, int suit, 
                        const javacall_utf16* clas, int clas_size, 
                        int flag, javacall_utf16* buf) {
    // put ID
    memcpy(buf, id, id_size * sizeof(javacall_utf16));
    buf += id_size;
    *buf++ = '\n';

    // put class_name
    memcpy(buf, clas, clas_size * sizeof(javacall_utf16));
    buf += clas_size;
    *buf++ = '\n';
    
    // put suiteId and flag
    *buf++ = (javacall_utf16)(suit >> 16);
    *buf++ = (javacall_utf16)suit;
    *buf = flag;
}

/**
 * The granularity of dynamically expanded size for result buffer.
 * Note! The value should mask last bits!
 */
#define RESBUF_GRAN 0xFF

/**
 * Assure result buffer (<code>resbuf</code>) capacity to append additional 
 * portion of data by <code>ext</code> javacall_utf16 units.
 */
static javacall_result assureBufferCap(_JAVAUTIL_CHAPI_RESBUF_* resbuf, 
                                                                int ext)  {
    if (resbuf->used + ext > resbuf->bufsz) {
        int sz = (resbuf->used + ext + RESBUF_GRAN) & (~RESBUF_GRAN);
		resbuf->buf = (javacall_utf16*) pcsl_mem_realloc(resbuf->buf, 
											sz * sizeof(javacall_utf16));
        if (resbuf->buf == NULL) {
            return JAVACALL_OUT_OF_MEMORY;
        }
        resbuf->bufsz = sz;
    }
    return JAVACALL_OK;
}

/**
 * Fills output result structure with handler data.
 * @param id handler ID
 * @param id_size handler ID size
 * @param suite_id suite Id
 * @param suite_id_size suite ID size
 * @param class_name handler class name
 * @param class_name_size handler class name size
 * @param flag handler installation flag
 * @param result output result structure.
 * @return operation status.
 */
javacall_result javautil_chapi_fillHandler(
        const javacall_utf16* id, int id_size,
        const javacall_utf16* suite_id, int suite_id_size,
        const javacall_utf16* class_name, int class_name_size,
        int flag, /*OUT*/ javacall_chapi_result_CH result) {
    int sz;
    SuiteIdType suit = get_midp_suite(suite_id, suite_id_size);
    javacall_utf16 *buf;

    sz = GET_CH_SIZE(id_size, class_name_size);
    buf = (javacall_utf16*) pcsl_mem_malloc(sz * sizeof(javacall_utf16));
    if (buf == NULL) {
        return JAVACALL_OUT_OF_MEMORY;
    }
    result->buf = buf;
    result->bufsz = result->used = sz;

    fill_ch_buf(id, id_size, suit, class_name, class_name_size, flag, buf);
    return JAVACALL_OK;
}

/**
 * Appends the handler data to the result array.
 * @param id handler ID
 * @param id_size handler ID size
 * @param suite_id suite Id
 * @param suite_id_size suite ID size
 * @param class_name handler class name
 * @param class_name_size handler class name size
 * @param flag handler installation flag
 * @param array output result array.
 * @return operation status.
 */
javacall_result javautil_chapi_appendHandler(
        const javacall_utf16* id, int id_size,
        const javacall_utf16* suite_id, int suite_id_size,
        const javacall_utf16* class_name, int class_name_size,
        int flag, /*OUT*/ javacall_chapi_result_CH_array array) {
    int sz = GET_CH_SIZE(id_size, class_name_size);
    SuiteIdType suit = get_midp_suite(suite_id, suite_id_size);
    int n;

    if (array->buf == NULL) {
        array->used = 1;
        n = 0;
    } else {
        n = array->buf[0];
    }

    if (JAVACALL_OK != assureBufferCap(array, sz + 1)) {
        return JAVACALL_OUT_OF_MEMORY;
    }

    array->buf[array->used++] = sz;
    fill_ch_buf(id, id_size, suit, class_name, class_name_size, flag, 
                                                    array->buf + array->used);
    array->used += sz;
    array->buf[0] = n + 1;
    return JAVACALL_OK;
}

/**
 * Appends string to output string array.
 * @param str appended string
 * @param str_size the string size
 * @param array string array.
 * @return operation status.
 */
javacall_result javautil_chapi_appendString(
        const javacall_utf16* str, int str_size,
        /*OUT*/ javacall_chapi_result_str_array array) {
    int n;

    if (array->used == 0) {
        array->used = 1;
        n = 0;
    } else {
        n = array->buf[0];
    }

    if (JAVACALL_OK != assureBufferCap(array, str_size + 1)) {
        return JAVACALL_OUT_OF_MEMORY;
    }

    array->buf[array->used++] = str_size;
    memcpy(array->buf + array->used, str, str_size * sizeof(javacall_utf16));
    array->used += str_size;
    array->buf[0] = n + 1;
    return JAVACALL_OK;
}

#endif ENABLE_JSR211_REGISTRY_IMPL