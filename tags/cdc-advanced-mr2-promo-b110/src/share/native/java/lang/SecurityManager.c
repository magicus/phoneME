/*
 * @(#)SecurityManager.c	1.54 06/10/10
 *
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
 *
 */

#include "jni.h"
#include "jvm.h"

#include "java_lang_SecurityManager.h"
#include "java_lang_ClassLoader.h"
#include "java_util_ResourceBundle.h"

#include "jni_statics.h"

#ifdef __cplusplus
#define this XthisX
#endif

/*
 * Make sure a security manager instance is initialized.
 * TRUE means OK, FALSE means not.
 */
static jboolean
check(JNIEnv *env, jobject this)
{
    jboolean initialized = JNI_FALSE;

    if (JNI_STATIC(java_lang_SecurityManager, initField) == 0) {
        jclass clazz = (*env)->FindClass(env, "java/lang/SecurityManager");
	if (clazz == 0) {
	    (*env)->ExceptionClear(env);
	    return JNI_FALSE;
	}
	JNI_STATIC(java_lang_SecurityManager, initField) = (*env)->GetFieldID(env, clazz, "initialized", "Z");
	if (JNI_STATIC(java_lang_SecurityManager, initField) == 0) {
	    (*env)->ExceptionClear(env);
	    return JNI_FALSE;
	}	    
    }
    initialized = (*env)->GetBooleanField(env, this, JNI_STATIC(java_lang_SecurityManager, initField));
    
    if (initialized == JNI_TRUE) {
	return JNI_TRUE;
    } else {
	jclass securityException =
	    (*env)->FindClass(env, "java/lang/SecurityException");
	if (securityException != 0) {
	    (*env)->ThrowNew(env, securityException, 
			     "security manager not initialized.");
	}
	return JNI_FALSE;
    }
}

JNIEXPORT jobjectArray JNICALL
Java_java_lang_SecurityManager_getClassContext(JNIEnv *env, jobject this)
{
    if (!check(env, this)) {
	return NULL;		/* exception */
    }

    return JVM_GetClassContext(env);
}

#ifdef CVM_HAVE_DEPRECATED
JNIEXPORT jclass JNICALL
Java_java_lang_SecurityManager_currentLoadedClass0(JNIEnv *env, jobject this)
{
    /* Make sure the security manager instance is initialized */
    if (!check(env, this)) {
	return NULL;		/* exception */
    }

    return JVM_CurrentLoadedClass(env);
}

JNIEXPORT jobject JNICALL
Java_java_lang_SecurityManager_currentClassLoader0(JNIEnv *env, jobject this)
{
    /* Make sure the security manager instance is initialized */
    if (!check(env, this)) {
	return NULL;		/* exception */
    }

    return JVM_CurrentClassLoader(env);
}

JNIEXPORT jint JNICALL
Java_java_lang_SecurityManager_classDepth(JNIEnv *env, jobject this,
					  jstring name)
{
    /* Make sure the security manager instance is initialized */
    if (!check(env, this)) {
	return -1;		/* exception */
    }

    return JVM_ClassDepth(env, name);
}

JNIEXPORT jint JNICALL
Java_java_lang_SecurityManager_classLoaderDepth0(JNIEnv *env, jobject this)
{
    /* Make sure the security manager instance is initialized */
    if (!check(env, this)) {
	return -1;		/* exception */
    }

    return JVM_ClassLoaderDepth(env);
}

#endif /* CVM_HAVE_DEPRECATED */
