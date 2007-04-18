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
 * @brief Content Handler RegistryStore native functions implementation
 */


#include <jni.h>
//#include <native/common/jni_util.h>
#include "javacall_chapi.h"
#include "javacall_chapi_callbacks.h"

/**
 * Functions results enumeration
 */
typedef enum {
  JSR211_OK,       /**< A function finished without errors */
  JSR211_FAILED    /**< An error occured */
} jsr211_result;

/**
 * Boolean enumeration
 */
typedef enum {
  JSR211_FALSE,    /**< False value */
  JSR211_TRUE      /**< True value */
} jsr211_boolean;

/**
 * Result codes for jsr211_execute_handler() method.
 */
typedef enum {
    JSR211_LAUNCH_OK                = 0,    /** OK, handler started */
    JSR211_LAUNCH_OK_SHOULD_EXIT    = 1,    /** OK, handler started 
                                            or is ready to start, 
                                            invoking app should exit. */
    JSR211_LAUNCH_ERR_NOTSUPPORTED  = -1,   /** ERROR, not supported */
    JSR211_LAUNCH_ERR_NO_HANDLER    = -2,    /** ERROR, no requested handler */
    JSR211_LAUNCH_ERR_NO_INVOCATION = -3,    /** ERROR, no invocation queued for 
                                                       requested handler */
    JSR211_LAUNCH_ERROR             = -4    /** common error */
} jsr211_launch_result;

/**
 * Content handler fields enumeration.
 */
typedef enum {
  JSR211_FIELD_ID = 0,     /**< Handler ID */
  JSR211_FIELD_TYPES,      /**< Types supported by a handler */
  JSR211_FIELD_SUFFIXES,   /**< Suffixes supported by a handler */
  JSR211_FIELD_ACTIONS,    /**< Actions supported by a handler */
  JSR211_FIELD_LOCALES,    /**< Locales supported by a handler */
  JSR211_FIELD_ACTION_MAP, /**< Handler action map */
  JSR211_FIELD_ACCESSES,   /**< Access list */
  JSR211_FIELD_COUNT       /**< Total number of fields */
} jsr211_field;

/**
 * Content handlers flags enumeration
 */
typedef enum {
  JSR211_FLAG_COMMON = 0, /**< Empty flag */
  JSR211_FLAG_DYNAMIC,    /**< Indicates content handler is dynamic */
  JSR211_FLAG_NATIVE      /**< Indicates content handler is native */
} jsr211_flag;

/**
 * Search modes for getHandler0 native implementation.
 * Its values MUST correspond RegistryStore SEARCH_* constants.
 */
typedef enum {
    JSR211_SEARCH_EXACT  = 0,        /** Search by exact match with ID */
    JSR211_SEARCH_PREFIX = 1         /** Search by prefix of given value */
} jsr211_search_flag;

/**
 * A content handler description structure, used for registration operation.
 */
typedef struct {
                      
  jchar*        id;         /**< Content handler ID */
  jint			suite_id;   /**< Storage where the handler is */
  jchar*        class_name; /**< Content handler class name */
  jsr211_flag   flag;       /**< Flag for registered content handlers. */
  jint          type_num;   /**< Number of types */
  jchar**       types;      /**< The types that are supported by this 
                                        content handler */
  jint          suff_num;   /**< Number of suffixes */
  jchar**       suffixes;   /**< The suffixes of URLs that are supported 
                                        by this content handler */
  jint          act_num;    /**< Number of actions */
  jchar**       actions;    /**< The actions that are supported by this 
                                        content handler */
  jint          locale_num; /**< Number of locales */
  jchar**       locales;    /**< The locales that are supported by this 
                                        content handler */
  jchar**       action_map; /**< The action names that are defined by 
                                        this content handler;
                                        size is act_num x locale_num  */
  jint          access_num; /**< Number of accesses */
  jchar**       accesses;   /**< The accessRestrictions for this 
                                        ContentHandler */
} JSR211_content_handler;

#define JSR211_CONTENT_HANDLER_INITIALIZER   {        \
    NULL,							/* id         */  \
    0,                              /* suite_id   */  \
    NULL,							/* class_name */  \
    0,                              /* flag       */  \
    0,                              /* type_num   */  \
    NULL,                           /* types      */  \
    0,                              /* suff_num   */  \
    NULL,                           /* suffixes   */  \
    0,                              /* act_num    */  \
    NULL,                           /* actions    */  \
    0,                              /* locale_num */  \
    NULL,                           /* locales    */  \
    NULL,                           /* action_map */  \
    0,                              /* access_num */  \
    NULL,                           /* accesses   */  \
}

/**
 * Common result buffer for serialized data storage.
 */
typedef struct {
    jchar* buf;
    jsize  len;
} _JSR211_INTERNAL_RESULT_BUFFER_;

#define _JSR211_RESULT_INITIALIZER_  { NULL, 0 }

/**
 * A content handler reduced structure, used for storing of search operation 
 * result.
 */
typedef struct {
  jchar*        id;         /**< Content handler ID */
  jint			suite_id;   /**< Storage where the handler is */
  jchar*        class_name; /**< Content handler class name */
  jsr211_flag   flag;       /**< Flag for registered 
                                        content handlers. */
} JSR211_CH;

/**
 * Result buffer for Content Handler. Use @link jsr211_fillHandler function to 
 * fill this structure.
 */
typedef _JSR211_INTERNAL_RESULT_BUFFER_ JSR211_RESULT_CH;

/**
 * Result buffer for Content Handlers array. Use @link jsr211_fillHandlerArray
 * function to fill this structure.
 */
typedef _JSR211_INTERNAL_RESULT_BUFFER_ JSR211_RESULT_CHARRAY;

/**
 * Result buffer for string array. Use @link jsr211_fillStringArray
 * function to fill this structure.
 */
typedef _JSR211_INTERNAL_RESULT_BUFFER_ JSR211_RESULT_STRARRAY;

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


#define DECLARE_MSG(n, s) static const char n[] = s;
#define EXCEPTION_MSG(s) s

/*
    JSR 211 exception messages
*/
DECLARE_MSG(fcFindHandler,   "Could not find handler")

/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    findHandler0
 * Signature: (Ljava/lang/String;ILjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_sun_j2me_content_RegistryStore_findHandler0
  (JNIEnv *env, jobject regStore, jstring callerId, jint searchBy, jstring value){
	jint status; 
    const jchar* caller_id_ = (*env)->GetStringChars(env,callerId, NULL);
    const jchar* value_ = (*env)->GetStringChars(env, value, NULL);
	javacall_chapi_result_CH_array resbuf = {0};
    status = javacall_chapi_find_handler((const javacall_utf16_string*)caller_id_, JAVACALL_FIELD(key), (const javacall_utf16_string*)value_, &resbuf);
	if (status!=0){
		JNU_ThrowInternalError(env, fcFindHandler);
	}
	return NULL;
}

/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    forSuite0
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_sun_j2me_content_RegistryStore_forSuite0
  (JNIEnv *env, jobject obj1, jint int1){
	return NULL;
}


/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    getValues0
 * Signature: (Ljava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_sun_j2me_content_RegistryStore_getValues0
  (JNIEnv *env, jobject obj1, jstring str1, jint int1){
	return NULL;
}


/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    getHandler0
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_sun_j2me_content_RegistryStore_getHandler0
  (JNIEnv *env, jobject obj1, jstring str1, jstring str2, jint int1){
	return NULL;
}


/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    loadFieldValues0
 * Signature: (Ljava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_sun_j2me_content_RegistryStore_loadFieldValues0
  (JNIEnv *env, jobject obj1, jstring str1, jint int1){
	return NULL;
}


/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    getByURL0
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_sun_j2me_content_RegistryStore_getByURL0
  (JNIEnv *env, jobject obj1, jstring str1, jstring str2, jstring str3){
	return NULL;
}


/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    launch0
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_sun_j2me_content_RegistryStore_launch0
  (JNIEnv *env, jobject obj1, jstring str1){
	return 0;
}


/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    init
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_sun_j2me_content_RegistryStore_init
  (JNIEnv *env, jobject obj1){
	return 0;
}


/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sun_j2me_content_RegistryStore_finalize
  (JNIEnv *env, jobject obj1){

}


/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    register0
 * Signature: (Lcom/sun/j2me/content/ContentHandlerImpl;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_sun_j2me_content_RegistryStore_register0
  (JNIEnv *env, jobject obj1, jobject obj2){
	return 0;
}



/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    unregister0
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_sun_j2me_content_RegistryStore_unregister0
  (JNIEnv *env, jobject obj1, jstring str1){
	return 0;
}

