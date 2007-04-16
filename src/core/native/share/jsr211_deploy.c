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
 * This module contains installation methods for registration of internal 
 * handlers.
 */ 

#include <jsr211_registry.h>
#include <pcsl_memory.h>
#include <midpString.h>
#include <midp_logging.h>

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
static const pcsl_string** handlerIds;
static char** rowHandlers;

#else

/**
 * The ID of the GraphicalInstaller handler
 */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(grinst_id)
{ 'G','r','a','p','h','i','c','a','l','I','n','s','t','a','l','l','e','r','\0' }
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(grinst_id);

static int nHandlers = 1;
static const pcsl_string* handlerIds[] = { &grinst_id };
static char* rowHandlers[] = {
    "com.sun.midp.installer.GraphicalInstaller\n"
    "text/vnd.sun.j2me.app-descriptor application/java-archive\n"
    ".jad .jar\n"
    "install remove\n"
    "en de ru\n"
    "Install Remove Installieren Umziehen "  // en, de
    "\xD0\xA3\xD1\x81\xD1\x82\xD0\xB0\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xB8\xD1\x82\xD1\x8C \xD0\xA3\xD0\xB4\xD0\xB0\xD0\xBB\xD0\xB8\xD1\x82\xD1\x8C\n" // ru
    "\n"
};

#endif

/**
 * Defined in the <regstore.c> file.
 */
void jsr211_cleanHandlerData(JSR211_content_handler *handler);

/**
 * Deserializes PCSL string [str] from the char buffer [ptr].
 * The string is limited by '\n' or by terminated zero.
 * After deserialization ptr is moved at start of the next line.
 * @return 0 if failed.
 */
static int fillString(char** ptr, /*OUT*/pcsl_string* str) {
    char *p = *ptr;
    int sz;

    while (*p != 0 && *p != '\n')
        p++;
    sz = p - *ptr;
    if (sz > 0 && 
        PCSL_STRING_OK != pcsl_string_convert_from_utf8((jbyte*)*ptr, sz, str)) {
        sz = -1;
    }
    *ptr = (*p == 0? p: p + 1);

    return sz >= 0;
}

/**
 * Deserializes string array [arr] from the char buffer [ptr]. 
 * The array is limited by '\n' or by terminated zero.
 * Each string entry delimited by a whitespace ' '.
 * After deserialization ptr is moved at start of the next line.
 * @return 0 if failed.
 */
static int fillArray(char **ptr, /*OUT*/int* len, /*OUT*/pcsl_string** arr) {
    char *p0, *p1;
    pcsl_string* str;
    int n = 1;

    p1 = p0 = *ptr;
    while (*p1 != 0 && *p1 != '\n') {
        if (*p1 == ' ')
            n++;
        p1++;
    }
    *ptr = (*p1 == 0? p1: p1 + 1);

    if ((p1 - p0) > 0) {
        str = alloc_pcsl_string_list(n);
        if (str == NULL) {
            return 0;
        }
        *arr = str;
        *len = n;

        while (n--) {
            p1 = p0;
            while (*p1 != ' ' && *p1 != 0 && *p1 != '\n')
                p1++;
            if (PCSL_STRING_OK != pcsl_string_convert_from_utf8((jbyte*)p0, p1 - p0, str))
                return 0;
            p0 = p1 + 1;
            str++;
        }
    }

    return 1;
}

/**
 * The actual installation of the content handler
 * @return <code>JSR211_OK</code> if the installation completes successfully.
 */
static jsr211_result installHandler(int n) {
    JSR211_content_handler ch = JSR211_CONTENT_HANDLER_INITIALIZER;
    char *ptr = rowHandlers[n];
    jsr211_result status = JSR211_FAILED;
    int anm_num;    // buffer for actionname map length

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
    if (PCSL_STRING_OK == pcsl_string_dup(handlerIds[n], &(ch.id)) &&
        fillString(&ptr, &ch.class_name) &&
        fillArray(&ptr, &ch.type_num, &ch.types) &&
        fillArray(&ptr, &ch.suff_num, &ch.suffixes) &&
        fillArray(&ptr, &ch.act_num, &ch.actions) &&
        fillArray(&ptr, &ch.locale_num, &ch.locales) &&
        fillArray(&ptr, &anm_num, &ch.action_map) &&
        fillArray(&ptr, &ch.access_num, &ch.accesses) &&
        anm_num == (ch.act_num * ch.locale_num)) {
            ch.suite_id = INTERNAL_SUITE_ID;
            ch.flag = JSR211_FLAG_COMMON;
            status = jsr211_register_handler(&ch);
#if REPORT_LEVEL <= LOG_CRITICAL
    } else {
    REPORT_CRIT(LC_NONE, "jsr211_deploy.c: handler data parsing failed");
#endif
    }

    // clean up handler's memory
    jsr211_cleanHandlerData(&ch);

    return status;
}

/**
 * Checks whether the internal handlers, if any, are installed.
 * @return JSR211_OK or JSR211_FAILED - if registry corrupted or OUT_OF_MEMORY.
 */
jsr211_result jsr211_check_internal_handlers() {
    int i;
    
    for (i = 0; i < nHandlers; i++) {
        JSR211_RESULT_CH handler = _JSR211_RESULT_INITIALIZER_;

        jsr211_get_handler(&PCSL_STRING_NULL, handlerIds[i], 
                                            JSR211_SEARCH_EXACT, &handler);
        if (handler.buf == NULL) {
            if (JSR211_OK != installHandler(i)) {
                return JSR211_FAILED;
            }
        } else {
            pcsl_mem_free(handler.buf);
        }
    }

    return JSR211_OK;
}
