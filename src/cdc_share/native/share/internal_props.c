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

#include "jni.h"
#include "jni_util.h"

static JavaVM* jvm = NULL;
static jclass clazz = NULL;
static jmethodID methodID = NULL;

/**
 * Native counterpart for
 * <code>com.sun.j2me.main.Configuration.getProperty()</code>.
 * Puts requested property value in UTF-8 format into the provided buffer
 * and returns pointer to it.
 *
 * @param key property key.
 * @param buffer pre-allocated buffer where property value will be stored.
 * @param length buffer size in bytes.
 * @return pointer to the filled buffer on success,
 *         <code>NULL</code> otherwise.
 */
const char* getInternalProp(const char* key, char* buffer, int length) {
    JNIEnv *env;
    jstring propname;
    jstring prop;
    jsize len;

    /*
     * JVM interface should have been stored in statics. If it is
     * not there, we cannot get properties from Java.
     */
    if (NULL == jvm) {
        return NULL;
    }

    if ((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_2) < 0) {
        return NULL;
    }

    /*
     * This function will be called from KNI methods, so we need a
     * JNI frame for calling Java method here.
     * The JNI frame is created for this particular function. This
     * could be refactored in the future if a general solution for
     * calling JNI from KNI environment is required.
     */
    if ((*env)->PushLocalFrame(env, 2) < 0) {
        return NULL;
    }

    propname = (*env)->NewStringUTF(env, key);
    if (NULL == propname) {
        goto _error;
    }

    prop = (jstring)(*env)->CallStaticObjectMethod(env, clazz, methodID,
                                                   propname);

    if ((*env)->ExceptionCheck(env) != JNI_FALSE) {
        (*env)->ExceptionClear(env);
        goto _error;
    }

    if (NULL == prop) {
        goto _error;
    }

    len = (*env)->GetStringUTFLength(env, prop);
    if (len >= length) {
        goto _error;
    }

    (*env)->GetStringUTFRegion(env, prop, 0, len, buffer);
    buffer[len] = 0;

    /* All done, JNI frame is not needed any more. */
    (*env)->PopLocalFrame(env, NULL);
    return (const char*)buffer;

_error:
    /* An error has occured, JNI frame is not needed any more. */
    (*env)->PopLocalFrame(env, NULL);
    return NULL;
}

/**
 * Stores <code>JavaVM</code> instance, class reference and method ID for later use.
 */
JNIEXPORT void JNICALL
Java_com_sun_j2me_main_Configuration_initialize(JNIEnv *env, jclass cls) {
    jclass c = (*env)->FindClass(env, "com/sun/j2me/main/Configuration");
    if (NULL == c) {
        JNU_ThrowByName(env, "java/lang/RuntimeException",
                        "cannot find Configuration class");
        return;
    }
    clazz = (*env)->NewGlobalRef(env, c);

    methodID = (*env)->GetStaticMethodID(env, c, "getProperty",
                                         "(Ljava/lang/String;)Ljava/lang/String;");
    if (NULL == methodID) {
        JNU_ThrowByName(env, "java/lang/RuntimeException",
                        "cannot get getProperty() method ID");
        return;
    }

    if ((*env)->GetJavaVM(env, &jvm) != 0) {
        JNU_ThrowByName(env, "java/lang/RuntimeException",
                        "cannot get Java VM interface");
    }
}
