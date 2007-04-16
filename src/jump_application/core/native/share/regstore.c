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

#define com_sun_j2me_content_RegistryStore_FIELD_ID 0L
#define com_sun_j2me_content_RegistryStore_FIELD_TYPES 1L
#define com_sun_j2me_content_RegistryStore_FIELD_SUFFIXES 2L
#define com_sun_j2me_content_RegistryStore_FIELD_ACTIONS 3L
#define com_sun_j2me_content_RegistryStore_FIELD_LOCALES 4L
#define com_sun_j2me_content_RegistryStore_FIELD_ACTION_MAP 5L
#define com_sun_j2me_content_RegistryStore_FIELD_ACCESSES 6L
#define com_sun_j2me_content_RegistryStore_FIELD_COUNT 7L
#define com_sun_j2me_content_RegistryStore_SEARCH_EXACT 0L
#define com_sun_j2me_content_RegistryStore_SEARCH_PREFIX 1L
#define com_sun_j2me_content_RegistryStore_FLAG_ERROR -1L
#define com_sun_j2me_content_RegistryStore_LAUNCH_OK 0L
#define com_sun_j2me_content_RegistryStore_LAUNCH_OK_SHOULD_EXIT 1L
#define com_sun_j2me_content_RegistryStore_LAUNCH_ERR_NOTSUPPORTED -1L
#define com_sun_j2me_content_RegistryStore_LAUNCH_ERR_NO_HANDLER -2L
#define com_sun_j2me_content_RegistryStore_LAUNCH_ERR_NO_INVOCATION -3L
#define com_sun_j2me_content_RegistryStore_LAUNCH_ERROR -4L

/*
 * Class:     com_sun_j2me_content_RegistryStore
 * Method:    findHandler0
 * Signature: (Ljava/lang/String;ILjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_sun_j2me_content_RegistryStore_findHandler0
  (JNIEnv *env, jobject obj1, jstring str1, jint int1, jstring str2){
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

