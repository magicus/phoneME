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

#include <jsrop_kni.h>
#include <jsrop_memory.h>
#include <jsrop_exceptions.h>
#include <jsrop_suitestore.h>

#include <javacall_defs.h>
#include <javautil_unicode.h>

#include <jsr211_registry.h>

#ifdef _DEBUG
#define TRACE_REGISTER
#define TRACE_TRANSFER_DATA
//#define TRACE_MALLOC
#endif


/** Number init() calls. Every finalize() decreases this value. */
static jint initialized = -1;

/** Classes fields */

/** com.sun.j2me.content.ContentHandlerImpl internal fields */
static jfieldID chImplId = 0;       // ID,
             // also it is an uninitiaized state indicator

#ifdef SUITE_ID_STRING
static jfieldID chImplSuiteID;      // storageID    : String
#else
static jfieldID chImplSuiteId;      // storageId    : int
#endif

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

/**  JSR 211 exception messages */

#define DECLARE_MSG(n, s) static const char n[] = s;
#define EXCEPTION_MSG(s) s

DECLARE_MSG(fcFindHandler,   "Could not find ContentHandler for field value")
DECLARE_MSG(fcFindInvocation,   "Invocation not found")
DECLARE_MSG(fcFindForSuite,   "Could not find ContentHandler for suite ID")
DECLARE_MSG(fcFindGetValues, "Could not read ContentHandler values");
DECLARE_MSG(fcGetHandlerField, "Could not read ContentHandler fields");
DECLARE_MSG(fcGetHandler, "Could not find ContentHandler");
DECLARE_MSG(fcHandlerByURL, "Could not find ContentHandler for URL");
DECLARE_MSG(fcUnexpectedFinilize,   "Unexpected RegistryStore finalization")
DECLARE_MSG(fcNoClassFields,   "Could not initialize JSR211 class fields")
DECLARE_MSG(fcRegister,   "Could not register ContentHandler")

static void result2string(KNIDECLARGS JSR211_RESULT_BUFFER buffer, jstring str){
    const void * data; size_t length;
    jsr211_get_data( jsr211_get_result_data(buffer), &data, &length );
#ifdef TRACE_TRANSFER_DATA
    printf( "kni_reg_store: buffer = %p, length %d\n", buffer, length );
#endif
	KNI_ReleaseHandle(str);
	if (length != 0) {
        // check data on '\0' character
        jchar * chars = (jchar *)data;
        if( length & 0x1 ){
#if REPORT_LEVEL <= LOG_ERROR
		    REPORT_ERROR(LC_NONE, "kni_reg_store.c: data transfer internal problem (length is odd)");
#endif
        }
        length /= 2;
        while( length-- && *chars++ );
        if( length + 1 != 0 ){
            // we should get rid of zero chars inside data, but not now
#if REPORT_LEVEL <= LOG_ERROR
		    REPORT_ERROR(LC_NONE, "kni_reg_store.c: data transfer internal problem (zero char exists)");
#endif
        } else {
            // data doesn't contain zero characters
            // small work around (write termination zero character)
            *chars = '\0';
        }
#ifdef TRACE_TRANSFER_DATA
        printf( "kni_reg_store: data '" );
        chars = (jchar *)data; 
        while( *chars ) printf( "%04x", *chars++ );
        printf( "'\n" );
#endif
		if (JAVACALL_OK != jsrop_jstring_from_utf16_string(
                                KNIPASSARGS (const javacall_utf16_string)data, str)){
            KNI_ThrowNew(jsropOutOfMemoryError, "No memory to create result string!");
		}
	}
	jsr211_release_result_buffer(buffer);
}

static void cleanStringArray(const jchar** strings, int n) {
	if (strings != NULL){
		const jchar** p = strings;
		while(n--) FREE(*p++);
		FREE((void*)strings);
	}
}


static void cleanHandlerData(jsr211_content_handler* handler) {
	FREE(handler->id);
	FREE(handler->suite_id);
	FREE(handler->class_name);

	cleanStringArray(handler->types,handler->type_num);
	cleanStringArray(handler->suffixes,handler->suff_num);
	cleanStringArray(handler->actions,handler->act_num);
	cleanStringArray(handler->locales,handler->locale_num);
	cleanStringArray(handler->action_map,handler->act_num * handler->locale_num);
	cleanStringArray(handler->accesses,handler->access_num);
}


/**
 * Retrieves fields IDs for classes:
 * <BR> <code>com.sun.j2me.content.ContentHandlerImpl</code> and
 * <BR> <code>javax.microedition.content.ActionNameMap</code>
 * @return KNI_OK - if successfully get all fields, KNI_ERR - otherwise
 */
static int initializeFields(KNIDECLARGS void) {
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
        chImplSuiteID =     KNI_GetFieldID(clObj,  "storageID", STRING_TYPE);
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
static int getStringArray(KNIDECLARGS jobjectArray arrObj, const jchar*** arrPtr) {
    int i=0, n;
	const jchar** arr;

	if (!arrObj) return 0;

    n = KNI_IsNullHandle(arrObj)? 0: (int)KNI_GetArrayLength(arrObj);
	if (!n) return 0;
	
#ifdef TRACE_MALLOC
    printf( "kni_reg_store.getStringArray(%d): malloc( %d )", __LINE__, sizeof(jchar*) * n );
#endif
    arr = MALLOC(sizeof(jchar*) * n);
    if (!arr) return KNI_ENOMEM;

    KNI_StartHandles(1);
    KNI_DeclareHandle(strObj);

    for (; i < n; i++) {
		KNI_GetObjectArrayElement(arrObj, i, strObj);
		if (!strObj) break;

		if (!JAVACALL_OK == jsrop_jstring_to_utf16_string(strObj,(javacall_utf16_string*)&arr[i]))
			break;
    }

	KNI_EndHandles();

	if (i<n){
		cleanStringArray(arr, i);
		return KNI_ENOMEM;
	}

	*arrPtr = arr;
    return n;
}

/**
 * Fills <code>String</code> arrays for locales and action_maps from 
 * <code>ActionMap</code> objects.
 * <BR>Length of <code>actionnames</code> array must be the same as in
 * <code>act_num</code> parameter for each element of <code>ActionMap</code>
 * array.
 *
 * @param o <code>ActionMap[]</code> object 
 * @param handler pointer on <code>jsr211_content_handler</code> structure
 * being filled up
 * @return KNI_OK - if successfully get all fields, 
 * KNI_ERR or KNI_ENOMEM - otherwise
 */
static int fillActionMap(KNIDECLARGS jobject o, jsr211_content_handler* handler) {
    int ret = KNI_OK;   // returned result
    int len;            // number of locales

    len = KNI_IsNullHandle(o)? 0: (int)KNI_GetArrayLength(o);
    if (len > 0) {
        int i, j;
        int n = handler->act_num;   // number of actions
		const jchar** locs = NULL;   // fetched locales
		const jchar** nams = NULL;   // fetched action names

        KNI_StartHandles(3);
        KNI_DeclareHandle(map);   // current ANMap object
        KNI_DeclareHandle(str);   // the ANMap's locale|name String object
        KNI_DeclareHandle(arr);   // the ANMap's array of names object

        do {
            // allocate buffers
            handler->locales = MALLOC(sizeof(jchar*) * len);
            if (handler->locales == NULL) {
                ret = KNI_ENOMEM;
                break;
            }
            handler->locale_num = len;

            handler->action_map = MALLOC(sizeof(jchar*) * len * n);
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
				if (!str || JAVACALL_OK!=jsrop_jstring_to_utf16_string(str, (javacall_utf16_string*)locs++)) {
                    ret = KNI_ENOMEM;
                    break;
                }
                KNI_GetObjectField(map, anMapActionnames, arr);
                for (j = 0; j < n; j++) {
                    KNI_GetObjectArrayElement(arr, j, str);
					if (!str || JAVACALL_OK!=jsrop_jstring_to_utf16_string(str, (javacall_utf16_string*)nams++)) {
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
 * Fills <code>jsr211_content_handler</code> structure with data from 
 * <code>ContentHandlerImpl</code> object.
 * <BR>Fields <code>ID, storageId</code> and <code>classname</code>
 * are mandatory. They must have not 0 length.
 *
 * @param o <code>ContentHandlerImpl</code> object
 * @param handler pointer on <code>jsr211_content_handler</code> structure
 * to be filled up
 * @return KNI_OK - if successfully get all fields, 
 * KNI_ERR or KNI_ENOMEM - otherwise
 */
static int fillHandlerData(KNIDECLARGS jobject o, jsr211_content_handler* handler) {
    int ret;    // returned result code
	int length=0;
    KNI_StartHandles(1);
    KNI_DeclareHandle(fldObj);   // field object

    do {
        // ID
        KNI_GetObjectField(o, chImplId, fldObj);
        if (JAVACALL_OK!=jsrop_jstring_to_utf16_string(fldObj, (javacall_utf16_string*)&(handler->id))) {
            ret = KNI_ENOMEM;
            break;
        }

        // check mandatory field
		javautil_unicode_utf16_ulength(handler->id,&length);
        if (length <= 0) {
            ret = KNI_ERR;
            break;
        }
        // suiteID
#ifdef SUITE_ID_STRING
        KNI_GetObjectField(o, chImplSuiteID, fldObj);
        if (JAVACALL_OK!=jsrop_jstring_to_utf16_string(fldObj, (javacall_utf16_string*)&(handler->suite_id))) {
            ret = KNI_ENOMEM;
            break;
        }
#else
	{
		SuiteIdType suite_id = KNI_GetIntField(o, chImplSuiteId);
		handler->suite_id = MALLOC((jsrop_suiteid_string_size(suite_id) + 1) * sizeof(jchar));
		jsrop_suiteid_to_string(suite_id, handler->suite_id);
	}
#endif
        // classname
        KNI_GetObjectField(o, chImplClassname, fldObj);
        if (JAVACALL_OK!=jsrop_jstring_to_utf16_string(fldObj, (javacall_utf16_string*)&(handler->class_name))) {
            ret = KNI_ENOMEM;
            break;
        }

        // flag
        handler->flag = KNI_GetIntField(o, chImplregMethod);

        // types
        KNI_GetObjectField(o, chImplTypes, fldObj);
        handler->type_num = getStringArray(KNIPASSARGS fldObj, &(handler->types));
        if (handler->type_num < 0) {
            ret = KNI_ENOMEM;
            break;
        }

        // suffixes        
        KNI_GetObjectField(o, chImplSuffixes, fldObj);
        handler->suff_num = getStringArray(KNIPASSARGS fldObj, &(handler->suffixes));
        if (handler->suff_num < 0) {
            ret = KNI_ENOMEM;
            break;
        }

        // actions
        KNI_GetObjectField(o, chImplActions, fldObj);
        handler->act_num = getStringArray(KNIPASSARGS fldObj, &(handler->actions));
        if (handler->act_num < 0) {
            ret = KNI_ENOMEM;
            break;
        }

        // action names
        if (handler->act_num > 0) {
            KNI_GetObjectField(o, chImplActionnames, fldObj);
            ret = fillActionMap(KNIPASSARGS fldObj, handler);
            if (KNI_OK != ret) {
                break;
            }
        }

        // accesses
        KNI_GetObjectField(o, chImplAccesses, fldObj);
        handler->access_num = getStringArray(KNIPASSARGS fldObj, &(handler->accesses));
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
 * <code>jsr211_content_handler</code> to be cleared.
 */
void jsr211_cleanHandlerData(KNIDECLARGS jsr211_content_handler *handler) {
    // clean up handler structure 
    if (handler->id!=NULL) FREE(handler->id);
	if (handler->suite_id!=NULL) FREE(handler->suite_id);
	if (handler->class_name!=NULL) FREE(handler->class_name);

    if (handler->type_num > 0 && handler->types != NULL) {
        cleanStringArray(KNIPASSARGS handler->types, handler->type_num);
    }
    if (handler->suff_num > 0 && handler->suffixes != NULL) {
        cleanStringArray(KNIPASSARGS handler->suffixes, handler->suff_num);
    }
    if (handler->act_num > 0 && handler->actions != NULL) {
        cleanStringArray(KNIPASSARGS handler->actions, handler->act_num);
    }
    if (handler->locale_num > 0 && handler->locales != NULL) {
        cleanStringArray(KNIPASSARGS handler->locales, handler->locale_num);
    }
    if (handler->act_num > 0 && handler->locale_num > 0 && handler->action_map != NULL) {
        cleanStringArray(KNIPASSARGS handler->action_map, handler->act_num * handler->locale_num);
    }
    if (handler->access_num > 0 && handler->accesses != NULL) {
        cleanStringArray(KNIPASSARGS handler->accesses, handler->access_num);
    }
}


/**
 * java call:
 *  private native boolean register0(ContentHandlerImpl contentHandler);
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_j2me_content_RegistryStore_register0) {
    int res = KNI_OK;
    jsr211_content_handler handler = JSR211_CONTENT_HANDLER_INITIALIZER;
    KNI_StartHandles(1);
    KNI_DeclareHandle(chObj);   // content handler instance
    KNI_GetParameterAsObject(1, chObj);
    do {
        if (chImplId == 0) {
            res = initializeFields(KNIPASSARGS);
            if (res != KNI_OK) {
                KNI_ThrowNew(jsropRuntimeException, 
                        "Can't initialize JSR211 class fields");
                break;
            }
        }

        res = fillHandlerData(KNIPASSARGS chObj, &handler);
        if (res != KNI_OK) {
			if (res == KNI_ENOMEM) {
				KNI_ThrowNew(jsropOutOfMemoryError, 
							"RegistryStore_register0 no memory for handler");
			}
            break;
        }

#ifdef TRACE_REGISTER
        printf( "com_sun_j2me_content_RegistryStore_register0:\n"
            "id = '%ls'\n"
            "suite_id = '%ls'\n"
            "class_name = '%ls'\n"
            "flag = %d, type_num = %d, suff_num = %d, act_num = %d, locale_num = %d, acces_num = %d\n",
            handler.id, handler.suite_id, handler.class_name,
            handler.flag, handler.type_num, handler.suff_num, handler.act_num, handler.locale_num, handler.access_num );
#endif
        res = (JSR211_OK == jsr211_register_handler(&handler))? KNI_OK: KNI_ERR;
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
KNIDECL(com_sun_j2me_content_RegistryStore_unregister0) {
    int ret = KNI_FALSE;
    jchar* id = NULL;

    KNI_StartHandles(1);
    KNI_DeclareHandle(idStr);   // content handler ID
    KNI_GetParameterAsObject(1, idStr);

    do {
        if (JAVACALL_OK != jsrop_jstring_to_utf16_string(idStr, (javacall_utf16_string*)&id)) {
            KNI_ThrowNew(jsropOutOfMemoryError, 
                            "RegistryStore_unregister0 no memory for ID");
            break;
        }
        
        if (JSR211_OK != jsr211_unregister_handler(id)) {
            break;
        }
        
        ret = KNI_TRUE;
    } while (0);

    FREE(id);
    KNI_EndHandles();

    KNI_ReturnBoolean(ret);
}


/**
 * java call:
 *   private native String findHandler0(String callerId, int searchBy, 
 *                                      String value);
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_j2me_content_RegistryStore_findHandler0) {
    jchar* callerId = NULL;
    jsr211_field searchBy;
    jchar* value = NULL;
    JSR211_RESULT_BUFFER result = jsr211_create_result_buffer();

    KNI_StartHandles(2);
    KNI_DeclareHandle(callerObj);
    KNI_DeclareHandle(valueObj);

    do {
        KNI_GetParameterAsObject(1, callerObj);
        KNI_GetParameterAsObject(3, valueObj);
        if (JAVACALL_OK != jsrop_jstring_to_utf16_string(callerObj, (javacall_utf16_string*)&callerId) ||
            JAVACALL_OK != jsrop_jstring_to_utf16_string(valueObj, (javacall_utf16_string*)&value)) {
            KNI_ThrowNew(jsropOutOfMemoryError, 
                   "RegistryStore_register0 no memory for string arguments");
            break;
        }

        searchBy = (jsr211_field) KNI_GetParameterAsInt(2);
        jsr211_find_handler(callerId, searchBy, value, &result);

    } while (0);

    FREE(value);
    FREE(callerId);
    result2string(KNIPASSARGS  result, valueObj);

    KNI_EndHandlesAndReturnObject(valueObj);
}


/**
 * java call:
 *   private native String forSuite0(String suiteID);
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_j2me_content_RegistryStore_forSuite0) {
    JSR211_RESULT_BUFFER result = jsr211_create_result_buffer();

    KNI_StartHandles(1);
    KNI_DeclareHandle(strObj);   // String object

#ifdef SUITE_ID_STRING
    jchar* suiteID = NULL;
    KNI_GetParameterAsObject(1, strObj);   // suiteID
    if (JAVACALL_OK != jsrop_jstring_to_utf16_string(strObj, (javacall_utf16_string*)&suiteID)) {
	    jsr211_release_result_buffer(result);
        KNI_ThrowNew(jsropOutOfMemoryError, NULL);
    } else {
	    jsr211_find_for_suite(suiteID, &result);
        result2string(KNIPASSARGS result, strObj);
    	FREE(suiteID);
    }
#else
    SuiteIdType suite_id_param = KNI_GetParameterAsInt(1);
    jchar suiteID[ 0x20 ];  // enough
    jsrop_suiteid_to_string(suite_id_param, suiteID);

    jsr211_find_for_suite(suiteID, &result);
    result2string(KNIPASSARGS result, strObj);
#endif

    KNI_EndHandlesAndReturnObject(strObj);
}

/**
 * java call:
 *  private native String getByURL0(String callerId, String url, String action);
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_j2me_content_RegistryStore_getByURL0) {
    jchar* callerId = NULL;
    jchar* url = NULL;
    jchar* action = NULL;
    JSR211_RESULT_BUFFER result = jsr211_create_result_buffer();

    KNI_StartHandles(4);
    KNI_DeclareHandle(callerObj);
    KNI_DeclareHandle(urlObj);
    KNI_DeclareHandle(actionObj);
    KNI_DeclareHandle(resultObj);

    do {
        KNI_GetParameterAsObject(1, callerObj);
        KNI_GetParameterAsObject(2, urlObj);
        KNI_GetParameterAsObject(3, actionObj);
        if (JAVACALL_OK != jsrop_jstring_to_utf16_string(callerObj, (javacall_utf16_string*)&callerId) ||
            JAVACALL_OK != jsrop_jstring_to_utf16_string(urlObj, (javacall_utf16_string*)&url) ||
            JAVACALL_OK != jsrop_jstring_to_utf16_string(actionObj, (javacall_utf16_string*)&action)) {
            KNI_ThrowNew(jsropOutOfMemoryError, 
                   "RegistryStore_getByURL0 no memory for string arguments");
            break;
        }

        jsr211_handler_by_URL(callerId, url, action, &result);
    } while (0);

    FREE(action);
    FREE(url);
    FREE(callerId);
    result2string(KNIPASSARGS  result, resultObj);

    KNI_EndHandlesAndReturnObject(resultObj);
}


/**
 * java call:
 *   private native String getValues0(String callerId, int searchBy);
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_j2me_content_RegistryStore_getValues0) {
    jsr211_field searchBy;
    jchar* callerId = NULL;
    JSR211_RESULT_BUFFER result = jsr211_create_result_buffer();

    KNI_StartHandles(1);
    KNI_DeclareHandle(strObj);   // String object

    do {
        KNI_GetParameterAsObject(1, strObj);   // callerId
        if (JAVACALL_OK != jsrop_jstring_to_utf16_string(strObj, (javacall_utf16_string*)&callerId)) {
            KNI_ThrowNew(jsropOutOfMemoryError, 
                   "RegistryStore_getValues0 no memory for string arguments");
            break;
        }

        searchBy = (jsr211_field) KNI_GetParameterAsInt(2);
        jsr211_get_all(callerId, searchBy, &result);
    } while (0);

    FREE(callerId);
    result2string(KNIPASSARGS  result, strObj);

    KNI_EndHandlesAndReturnObject(strObj);
}


 /**
  * java call:
  * private native String getHandler0(String callerId, String id, int mode);
  */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_j2me_content_RegistryStore_getHandler0) {
    int mode;
    jchar* callerId = NULL;
    jchar* id = NULL;
    JSR211_RESULT_BUFFER handler = jsr211_create_result_buffer();
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(callerObj);
    KNI_DeclareHandle(handlerObj);

    do {
        KNI_GetParameterAsObject(1, callerObj);
        KNI_GetParameterAsObject(2, handlerObj);
        if (JAVACALL_OK != jsrop_jstring_to_utf16_string(callerObj, &callerId) ||
            JAVACALL_OK != jsrop_jstring_to_utf16_string(handlerObj, &id)) {
            KNI_ThrowNew(jsropOutOfMemoryError, 
                   "RegistryStore_getHandler0 no memory for string arguments");
            break;
        }
        mode = KNI_GetParameterAsInt(3);

        jsr211_get_handler(callerId, id, mode, &handler);
    } while (0);

    FREE(callerId);
    FREE(id);
    result2string(KNIPASSARGS  handler, handlerObj);

    KNI_EndHandlesAndReturnObject(handlerObj);
}


/**
 * java call:
 *     private native String loadFieldValues0(String handlerId, int fieldId);
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_j2me_content_RegistryStore_loadFieldValues0) {
    int fieldId;
    jchar* id = NULL;
    JSR211_RESULT_BUFFER result = jsr211_create_result_buffer();

    KNI_StartHandles(1);
    KNI_DeclareHandle(strObj);       /* string object */

    KNI_GetParameterAsObject(1, strObj); /* handlerId */
    if (JAVACALL_OK == jsrop_jstring_to_utf16_string(strObj, &id)) {
        fieldId = KNI_GetParameterAsInt(2);
        jsr211_get_handler_field(id, fieldId, &result);
        FREE(id);
        result2string(KNIPASSARGS  result, strObj);
    } else {
        jsr211_release_result_buffer(result);
        KNI_ReleaseHandle(strObj);
    }

    KNI_EndHandlesAndReturnObject(strObj);
}

/**
 * java call:
 * private native static boolean init();
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_j2me_content_RegistryStore_init) {
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
KNIDECL(com_sun_j2me_content_RegistryStore_finalize) {

	if (initialized > 0) {

		if (--initialized == 0) {
			jsr211_finalize();
			initialized = -1;
		}
	}

    KNI_ReturnVoid();
}

/**
 * java call:
 * private native int launchNativeHandler0(String handlerId);
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_j2me_content_InvocationStoreProxy_launchNativeHandler0) {
    jchar* id = NULL;
    jsr211_launch_result result;

    KNI_StartHandles(1);
    KNI_DeclareHandle(idStr);           /* handlerId */

    KNI_GetParameterAsObject(1, idStr); /* handlerId */
    if (JAVACALL_OK == jsrop_jstring_to_utf16_string(idStr, &id)) {
        result = jsr211_execute_handler(id);
    } else {
        KNI_ThrowNew(jsropOutOfMemoryError, 
                   "RegistryStore_launch0 no memory for handler ID");
        result = JSR211_LAUNCH_ERROR;
    }

    FREE(id);
    KNI_EndHandles();    
    KNI_ReturnInt(result);
}

/**
 * java call:
 * private native int platformFinish0(String handlerId);
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_j2me_content_InvocationStoreProxy_platformFinish0) {
    int tid = KNI_GetParameterAsInt(1);

    KNI_ReturnVoid();
}
