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

 /*
 * @(#)kni_reg_store.c	1.13 06/04/05 @(#)
 *
 */

/**
 * @file
 * @brief Content Handler RegistryStore native functions implementation
 */

#include <string.h>
#include <ctype.h>

#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>

#include <midpUtilKni.h>
#include <midpError.h>

#include <midpServices.h>
#include <midpMalloc.h>
#include <midpStorage.h>

#include <pcsl_memory.h>

#include <jsr211_registry.h>
//#include <sni.h>

#include <stdio.h>

/** Number init() calls. Every finalize() decreases this value. */
static jint initialized = -1;

/** Classes fields */

/** com.sun.midp.content.ContentHandlerImpl internal fields */
static jfieldID chImplId = 0;       // ID,
             // also it is an uninitiaized state indicator

static jfieldID chImplSuiteId;      // storageID    : String
static jfieldID chImplClassname;    // classname    : String
static jfieldID chImplregMethod;    // registrationMethod : int
static jfieldID chImplTypes;        // types        : String[]
static jfieldID chImplSuffixes;     // suffixes     : String[]
static jfieldID chImplActions;      // actions      : String[]
static jfieldID chImplActionnames;  // actionnames  : ActionNameMap[]
static jfieldID chImplAccesses;     // accesses     : String[]

/** javax.microedition.content.ActionNameMap fields */
static jfieldID anMapLocale;        // locale
static jfieldID anMapActionnames;   // [] actionnames


/**
 * Retrieves fields IDs for classes:
 * <BR> <code>com.sun.midp.content.ContentHandlerImpl</code> and
 * <BR> <code>javax.microedition.content.ActionNameMap</code>
 * @return KNI_OK - if successfully get all fields, KNI_ERR - otherwise
 */
static int initializeFields() {
    static const char* STRING_TYPE = "Ljava/lang/String;";
    static const char* S_ARRAY_TYPE = "[Ljava/lang/String;";
    static const char* ANM_ARRAY_TYPE = 
                            "[Ljavax/microedition/content/ActionNameMap;";
    static const char* ANM_CLASS_NAME = 
                            "javax/microedition/content/ActionNameMap";
    int ret;    // returned result code
    KNI_StartHandles(1);
    KNI_DeclareHandle(clObj);   // clazz object

    do {
        // 1. initialize ContentHandlerImpl fields
        KNI_FindClass("com/sun/j2me/content/ContentHandlerImpl", clObj);
	if (KNI_IsNullHandle(clObj)) {
#if REPORT_LEVEL <= LOG_CRITICAL
		REPORT_CRIT(LC_NONE,
		    	"kni_reg_store.c: can't find class com.sun.j2me.content.ContentHandlerImpl");
#endif
            	ret = KNI_ERR;
		break;
	}
        chImplId =          KNI_GetFieldID(clObj,  "ID", STRING_TYPE);
#ifdef SUITE_ID_STRING
        chImplSuiteId =     KNI_GetFieldID(clObj,  "storageID", STRING_TYPE);
#else
        chImplSuiteId =     KNI_GetFieldID(clObj,  "storageId", "I");
#endif
        chImplClassname =   KNI_GetFieldID(clObj,  "classname", STRING_TYPE);
        chImplregMethod =   KNI_GetFieldID(clObj,  "registrationMethod", "I");
        chImplTypes =       KNI_GetFieldID(clObj,  "types", S_ARRAY_TYPE);
        chImplSuffixes =    KNI_GetFieldID(clObj,  "suffixes", S_ARRAY_TYPE);
        chImplActions =     KNI_GetFieldID(clObj,  "actions", S_ARRAY_TYPE);
        chImplActionnames = KNI_GetFieldID(clObj,  "actionnames", 
                                                            ANM_ARRAY_TYPE);
        chImplAccesses =    KNI_GetFieldID(clObj,  "accessRestricted", 
                                                            S_ARRAY_TYPE);
    
        if (chImplId == 0 || chImplSuiteId == 0 || chImplClassname == 0 || 
            chImplregMethod == 0 || chImplTypes == 0 || 
            chImplSuffixes == 0 || chImplActions == 0 || 
            chImplActionnames == 0 || chImplAccesses == 0) {
    
#if REPORT_LEVEL <= LOG_CRITICAL
	REPORT_CRIT(LC_NONE,
		    "kni_reg_store.c: can't initialize ContentHandlerImpl fields!");
#endif
    
            ret = KNI_ERR;
            break;
        }
    
        // 2. initialize ActionName fields
        KNI_FindClass(ANM_CLASS_NAME, clObj);  // clObj = ActionNameMap class
        if (KNI_IsNullHandle(clObj)) {
#if REPORT_LEVEL <= LOG_CRITICAL
	REPORT_CRIT(LC_NONE,
		    "kni_reg_store.c: can't find ActionNameMap class!");
#endif
            ret = KNI_ERR;
            break;
        }
    
        anMapLocale =       KNI_GetFieldID(clObj,  "locale", STRING_TYPE);
        anMapActionnames =  KNI_GetFieldID(clObj,  "actionnames", S_ARRAY_TYPE);
    
        if (anMapLocale == 0 || anMapActionnames == 0) {
    
#if REPORT_LEVEL <= LOG_CRITICAL
	REPORT_CRIT(LC_NONE,
		    "kni_reg_store.c: can't initialize ActionNameMap fields!");
#endif
            ret = KNI_ERR;
            break;
        }
        
        ret = KNI_OK;   // that's all right.
    } while (0);

    KNI_EndHandles();
    return ret;
}

/**
 * Fetch a KNI String array object into the string array.
 *
 * @param arrObj KNI Java String object handle
 * @param arrPtr the String array pointer for values storing
 * @return number of retrieved strings
 * <BR>KNI_ENOMEM - indicates memory allocation error
 */
static int getStringArray(jobjectArray arrObj, pcsl_string** arrPtr) {
    int i, n = 0;
    pcsl_string* arr;

    KNI_StartHandles(1);
    KNI_DeclareHandle(strObj);

    n = KNI_IsNullHandle(arrObj)? 0: (int)KNI_GetArrayLength(arrObj);
    while (n > 0) {
        arr = alloc_pcsl_string_list(n);
        if (arr == NULL) {
            n = KNI_ENOMEM;
            break;
        }

        *arrPtr = arr;
        for (i = 0; i < n; i++, arr++) {
            KNI_GetObjectArrayElement(arrObj, i, strObj);
            if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(strObj, arr)) {
                free_pcsl_string_list(*arrPtr, n);
                *arrPtr = NULL;
                n = KNI_ENOMEM;
                break;
            }
        }
        break;
    }

    KNI_EndHandles();
    return n;
}

/**
 * Fills <code>MidpString</code> arrays for locales and action_maps from 
 * <code>ActionMap</code> objects.
 * <BR>Length of <code>actionnames</code> array must be the same as in
 * <code>act_num</code> parameter for each element of <code>ActionMap</code>
 * array.
 *
 * @param o <code>ActionMap[]</code> object 
 * @param handler pointer on <code>JSR211_content_handler</code> structure
 * being filled up
 * @return KNI_OK - if successfully get all fields, 
 * KNI_ERR or KNI_ENOMEM - otherwise
 */
static int fillActionMap(jobject o, JSR211_content_handler* handler) {
    int ret = KNI_OK;   // returned result
    int len;            // number of locales

    len = KNI_IsNullHandle(o)? 0: (int)KNI_GetArrayLength(o);
    if (len > 0) {
        int i, j;
        int n = handler->act_num;   // number of actions
        pcsl_string *locs = NULL;   // fetched locales
        pcsl_string *nams = NULL;   // fetched action names

        KNI_StartHandles(3);
        KNI_DeclareHandle(map);   // current ANMap object
        KNI_DeclareHandle(str);   // the ANMap's locale|name String object
        KNI_DeclareHandle(arr);   // the ANMap's array of names object

        do {
            // allocate buffers
            handler->locales = alloc_pcsl_string_list(len);
            if (handler->locales == NULL) {
                ret = KNI_ENOMEM;
                break;
            }
            handler->locale_num = len;
            handler->action_map = alloc_pcsl_string_list(len * n);
            if (handler->action_map == NULL) {
                ret = KNI_ENOMEM;
                break;
            }

            // iterate array elements
            locs = handler->locales;
            nams = handler->action_map;
            for (i = 0; i < len && ret == KNI_OK; i++) {
                KNI_GetObjectArrayElement(o, i, map);
                KNI_GetObjectField(map, anMapLocale, str);
                if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(str, locs++)) {
                    ret = KNI_ENOMEM;
                    break;
                }
                KNI_GetObjectField(map, anMapActionnames, arr);
                for (j = 0; j < n; j++) {
                    KNI_GetObjectArrayElement(arr, j, str);
                    if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(str, nams++)) {
                        ret = KNI_ENOMEM;
                        break;
                    }
                }
            }
        } while (0);
        
        KNI_EndHandles();
    }
    
    return ret;
}


/**
 * Fills <code>JSR211_content_handler</code> structure with data from 
 * <code>ContentHandlerImpl</code> object.
 * <BR>Fields <code>ID, storageId</code> and <code>classname</code>
 * are mandatory. They must have not 0 length.
 *
 * @param o <code>ContentHandlerImpl</code> object
 * @param handler pointer on <code>JSR211_content_handler</code> structure
 * to be filled up
 * @return KNI_OK - if successfully get all fields, 
 * KNI_ERR or KNI_ENOMEM - otherwise
 */
static int fillHandlerData(jobject o, JSR211_content_handler* handler) {
    int ret;    // returned result code
    KNI_StartHandles(1);
    KNI_DeclareHandle(fldObj);   // field object

    do {
        // ID
        KNI_GetObjectField(o, chImplId, fldObj);
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(fldObj, &(handler->id))) {
            ret = KNI_ENOMEM;
            break;
        }
        // check mandatory field
        if (pcsl_string_length(&(handler->id)) <= 0) {
            ret = KNI_ERR;
            break;
        }

        // suiteID
#ifdef SUITE_ID_STRING
        KNI_GetObjectField(o, chImplSuiteId, fldObj);
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(fldObj, &(handler->suite_id))) {
            ret = KNI_ENOMEM;
            break;
        }
#else
        handler->suite_id = KNI_GetIntField(o, chImplSuiteId);
#endif
        // classname
        KNI_GetObjectField(o, chImplClassname, fldObj);
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(fldObj, &(handler->class_name))) {
            ret = KNI_ENOMEM;
            break;
        }

        // flag
        handler->flag = KNI_GetIntField(o, chImplregMethod);

        // types
        KNI_GetObjectField(o, chImplTypes, fldObj);
        handler->type_num = getStringArray(fldObj, &(handler->types));
        if (handler->type_num < 0) {
            ret = KNI_ENOMEM;
            break;
        }

        // suffixes        
        KNI_GetObjectField(o, chImplSuffixes, fldObj);
        handler->suff_num = getStringArray(fldObj, &(handler->suffixes));
        if (handler->suff_num < 0) {
            ret = KNI_ENOMEM;
            break;
        }

        // actions
        KNI_GetObjectField(o, chImplActions, fldObj);
        handler->act_num = getStringArray(fldObj, &(handler->actions));
        if (handler->act_num < 0) {
            ret = KNI_ENOMEM;
            break;
        }

        // action names
        if (handler->act_num > 0) {
            KNI_GetObjectField(o, chImplActionnames, fldObj);
            ret = fillActionMap(fldObj, handler);
            if (KNI_OK != ret) {
                break;
            }
        }

        // accesses
        KNI_GetObjectField(o, chImplAccesses, fldObj);
        handler->access_num = getStringArray(fldObj, &(handler->accesses));
        if (handler->access_num < 0) {
            ret = KNI_ENOMEM;
            break;
        }

        ret = KNI_OK;
    } while (0);

    KNI_EndHandles();
    return ret;
}

/**
 * Cleans up all data memebers of given data structure,
 * <br>but <code>handler</code> itself is not cleared.
 * @param handler pointer on data structure 
 * <code>JSR211_content_handler</code> to be cleared.
 */
void jsr211_cleanHandlerData(JSR211_content_handler *handler) {
    // clean up handler structure 
    if (pcsl_string_is_null(&(handler->id)) == PCSL_FALSE) {
        pcsl_string_free(&(handler->id));
    }
#ifdef SUITE_ID_STRING
    if (pcsl_string_is_null(&(handler->suite_id)) == PCSL_FALSE) {
        pcsl_string_free(&(handler->suite_id));
    }
#endif
    if (pcsl_string_is_null(&(handler->class_name)) == PCSL_FALSE) {
        pcsl_string_free(&(handler->class_name));
    }
    if (handler->type_num > 0 && handler->types != NULL) {
        free_pcsl_string_list(handler->types, handler->type_num);
    }
    if (handler->suff_num > 0 && handler->suffixes != NULL) {
        free_pcsl_string_list(handler->suffixes, handler->suff_num);
    }
    if (handler->act_num > 0 && handler->actions != NULL) {
        free_pcsl_string_list(handler->actions, handler->act_num);
    }
    if (handler->locale_num > 0 && handler->locales != NULL) {
        free_pcsl_string_list(handler->locales, handler->locale_num);
    }
    if (handler->act_num > 0 && handler->locale_num > 0 && handler->action_map != NULL) {
        free_pcsl_string_list(handler->action_map, handler->act_num * handler->locale_num);
    }
    if (handler->access_num > 0 && handler->accesses != NULL) {
        free_pcsl_string_list(handler->accesses, handler->access_num);
    }
}

/**
 * Transforms prepared result buffer to jstring object and release memory of 
 * the allocated buffer.
 * It is safe to call this function after detecting out-of-memory error
 * provided that buf is set to _JSR211_RESULT_INITIALIZER_
 */
static void result2string(JSR211_RESULT_BUFFER buf, jstring str) {
    if (jsr211_get_result_data_length(buf) > 0 && jsr211_get_result_data(buf) != NULL) {
        KNI_NewString((const unsigned short*)jsr211_get_result_data(buf), jsr211_get_result_data_length(buf), str);
    } else {
        KNI_ReleaseHandle(str);
    }
    jsr211_release_result_buffer(buf);
}



/**
 * java call:
 *  private native boolean register0(ContentHandlerImpl contentHandler);
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_j2me_content_RegistryStore_register0(void) {
    int res = KNI_OK;
    JSR211_content_handler handler = JSR211_CONTENT_HANDLER_INITIALIZER;

    KNI_StartHandles(1);
    KNI_DeclareHandle(chObj);   // content handler instance
    KNI_GetParameterAsObject(1, chObj);

    do {
        if (chImplId == 0) {
            res = initializeFields();
            if (res != KNI_OK) {
                KNI_ThrowNew(midpRuntimeException, 
                        "Can't initialize JSR211 class fields");
                break;
            }
        }

        res = fillHandlerData(chObj, &handler);
        if (res != KNI_OK) {
			if (res == KNI_ENOMEM) {
				KNI_ThrowNew(midpOutOfMemoryError, 
							"RegistryStore_register0 no memory for handler");
			}
            break;
        }

        res = JSR211_OK == jsr211_register_handler(&handler)? KNI_OK: KNI_ERR;
    } while (0);
    

    KNI_EndHandles();
    jsr211_cleanHandlerData(&handler);

    KNI_ReturnBoolean(res==KNI_OK? KNI_TRUE: KNI_FALSE);
}

/**
 * java call:
 *  private native boolean unregister(String handlerId);
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_j2me_content_RegistryStore_unregister0(void) {
    int ret = KNI_FALSE;
    pcsl_string id = PCSL_STRING_NULL_INITIALIZER;

    KNI_StartHandles(1);
    KNI_DeclareHandle(idStr);   // content handler ID
    KNI_GetParameterAsObject(1, idStr);

    do {
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(idStr, &id)) {
            KNI_ThrowNew(midpOutOfMemoryError, 
                            "RegistryStore_unregister0 no memory for ID");
            break;
        }
        
        if (JSR211_OK != jsr211_unregister_handler(&id)) {
            break;
        }
        
        ret = KNI_TRUE;
    } while (0);

    pcsl_string_free(&id);
    KNI_EndHandles();

    KNI_ReturnBoolean(ret);
}


/**
 * java call:
 *   private native String findHandler0(String callerId, int searchBy, 
 *                                      String value);
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_j2me_content_RegistryStore_findHandler0(void) {
    pcsl_string callerId = PCSL_STRING_NULL_INITIALIZER;
    jsr211_field searchBy;
    pcsl_string value = PCSL_STRING_NULL_INITIALIZER;
    JSR211_RESULT_CHARRAY result = jsr211_create_result_buffer();

    KNI_StartHandles(2);
    KNI_DeclareHandle(callerObj);
    KNI_DeclareHandle(valueObj);

    do {
        KNI_GetParameterAsObject(1, callerObj);
        KNI_GetParameterAsObject(3, valueObj);
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(callerObj, &callerId) ||
            PCSL_STRING_OK != midp_jstring_to_pcsl_string(valueObj, &value)) {
            KNI_ThrowNew(midpOutOfMemoryError, 
                   "RegistryStore_register0 no memory for string arguments");
            break;
        }

        searchBy = (jsr211_field) KNI_GetParameterAsInt(2);
        jsr211_find_handler(&callerId, searchBy, &value, result);

    } while (0);

    pcsl_string_free(&value);
    pcsl_string_free(&callerId);
    result2string(result, valueObj);

    KNI_EndHandlesAndReturnObject(valueObj);
}


/**
 * java call:
 *   private native String forSuite0(String suiteID);
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_j2me_content_RegistryStore_forSuite0(void) {
    JSR211_RESULT_CHARRAY result = jsr211_create_result_buffer();

    KNI_StartHandles(1);
    KNI_DeclareHandle(strObj);   // String object

#ifdef SUITE_ID_STRING 
{
    pcsl_string suiteID = PCSL_STRING_NULL_INITIALIZER;
    KNI_GetParameterAsObject(1, strObj);   // suiteID
    if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(strObj, &suiteID)) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else {
	jsr211_find_for_suite(&suiteID, result);
	result2string(result, strObj);
    	pcsl_string_free(&suiteID);
    }
}
#else
    jsr211_find_for_suite((jsr211_field) KNI_GetParameterAsInt(1), result);
    result2string(result, strObj);
#endif
    KNI_EndHandlesAndReturnObject(strObj);
}

/**
 * java call:
 *  private native String getByURL0(String callerId, String url, String action);
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_j2me_content_RegistryStore_getByURL0(void) {
    pcsl_string callerId = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string url = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string action = PCSL_STRING_NULL_INITIALIZER;
    JSR211_RESULT_CH result = jsr211_create_result_buffer();

    KNI_StartHandles(4);
    KNI_DeclareHandle(callerObj);
    KNI_DeclareHandle(urlObj);
    KNI_DeclareHandle(actionObj);
    KNI_DeclareHandle(resultObj);

    do {
        KNI_GetParameterAsObject(1, callerObj);
        KNI_GetParameterAsObject(2, urlObj);
        KNI_GetParameterAsObject(3, actionObj);
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(callerObj, &callerId) ||
            PCSL_STRING_OK != midp_jstring_to_pcsl_string(urlObj, &url) ||
            PCSL_STRING_OK != midp_jstring_to_pcsl_string(actionObj, &action)) {
            KNI_ThrowNew(midpOutOfMemoryError, 
                   "RegistryStore_getByURL0 no memory for string arguments");
            break;
        }

        jsr211_handler_by_URL(&callerId, &url, &action, result);
    } while (0);

    pcsl_string_free(&action);
    pcsl_string_free(&url);
    pcsl_string_free(&callerId);
    result2string(result, resultObj);

    KNI_EndHandlesAndReturnObject(resultObj);
}


/**
 * java call:
 *   private native String getValues0(String callerId, int searchBy);
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_j2me_content_RegistryStore_getValues0(void) {
    jsr211_field searchBy;
    pcsl_string callerId = PCSL_STRING_NULL_INITIALIZER;
    JSR211_RESULT_STRARRAY result = jsr211_create_result_buffer();

    KNI_StartHandles(1);
    KNI_DeclareHandle(strObj);   // String object

    do {
        KNI_GetParameterAsObject(1, strObj);   // callerId
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(strObj, &callerId)) {
            KNI_ThrowNew(midpOutOfMemoryError, 
                   "RegistryStore_getValues0 no memory for string arguments");
            break;
        }

        searchBy = (jsr211_field) KNI_GetParameterAsInt(2);
        jsr211_get_all(&callerId, searchBy, result);
    } while (0);

    pcsl_string_free(&callerId);
    result2string(result, strObj);

    KNI_EndHandlesAndReturnObject(strObj);
}


 /**
  * java call:
  * private native String getHandler0(String callerId, String id, int mode);
  */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_j2me_content_RegistryStore_getHandler0(void) {
    int mode;
    pcsl_string callerId = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string id = PCSL_STRING_NULL_INITIALIZER;
    JSR211_RESULT_CH handler = jsr211_create_result_buffer();
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(callerObj);
    KNI_DeclareHandle(handlerObj);

    do {
        KNI_GetParameterAsObject(1, callerObj);
        KNI_GetParameterAsObject(2, handlerObj);
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(callerObj, &callerId) ||
            PCSL_STRING_OK != midp_jstring_to_pcsl_string(handlerObj, &id)) {
            KNI_ThrowNew(midpOutOfMemoryError, 
                   "RegistryStore_getHandler0 no memory for string arguments");
            break;
        }
        mode = KNI_GetParameterAsInt(3);

        jsr211_get_handler(&callerId, &id, mode, handler);
    } while (0);

    pcsl_string_free(&callerId);
    pcsl_string_free(&id);
    result2string(handler, handlerObj);

    KNI_EndHandlesAndReturnObject(handlerObj);
}


/**
 * java call:
 *     private native String loadFieldValues0(String handlerId, int fieldId);
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_j2me_content_RegistryStore_loadFieldValues0(void) {
    int fieldId;
    pcsl_string id = PCSL_STRING_NULL_INITIALIZER;
    JSR211_RESULT_STRARRAY result = jsr211_create_result_buffer();

    KNI_StartHandles(1);
    KNI_DeclareHandle(strObj);       /* string object */

    KNI_GetParameterAsObject(1, strObj); /* handlerId */
    if (PCSL_STRING_OK == midp_jstring_to_pcsl_string(strObj, &id)) {
        fieldId = KNI_GetParameterAsInt(2);
        jsr211_get_handler_field(&id, fieldId, result);
        pcsl_string_free(&id);
        result2string(result, strObj);
    } else {
        KNI_ReleaseHandle(strObj);
    }

    KNI_EndHandlesAndReturnObject(strObj);
}

/**
 * java call:
 * private native int launch0(String handlerId);
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_j2me_content_RegistryStore_launch0(void) {
    pcsl_string id = PCSL_STRING_NULL_INITIALIZER;
    jsr211_launch_result result;

    KNI_StartHandles(1);
    KNI_DeclareHandle(idStr);           /* handlerId */

    KNI_GetParameterAsObject(1, idStr); /* handlerId */
    if (PCSL_STRING_OK == midp_jstring_to_pcsl_string(idStr, &id)) {
        result = jsr211_execute_handler(&id);
    } else {
        KNI_ThrowNew(midpOutOfMemoryError, 
                   "RegistryStore_launch0 no memory for handler ID");
        result = JSR211_LAUNCH_ERROR;
    }

    pcsl_string_free(&id);
    KNI_EndHandles();    
    KNI_ReturnInt(result);
}

/**
 * java call:
 * private native static boolean init();
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_j2me_content_RegistryStore_init(void) {
    jboolean ret = KNI_TRUE;
    if (initialized < 0) {
        // Global initialization
        if (JSR211_OK != jsr211_initialize()) {
            ret = KNI_FALSE;
        } else {
            if (JSR211_OK == jsr211_check_internal_handlers()) {
                initialized = 1;
            } else {
                ret = KNI_FALSE;
            }
        }
    } else {
        initialized++;
    }
    KNI_ReturnBoolean(ret);
}

/**
 * Native finalizer to free all native resources used by the
 * object.
 *
 * @param this The <code>RegistryStore</code> Object to be finalized.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_j2me_content_RegistryStore_finalize() {

    initialized--;

    if (initialized == 0) {
        jsr211_finalize();
        initialized = -1;
    }

    KNI_ReturnVoid();
}
