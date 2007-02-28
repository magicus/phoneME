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

static JavaVM *jvm = NULL;

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
const char* jumpGetInternalProp(const char* key, char* buffer, int length) {
    JNIEnv *env;
    jstring propname;
    jclass clazz;
    jmethodID methodID;
    jstring prop;
    jsize len;

    if (NULL == jvm) {
        return NULL;
    }

    if ((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_2) < 0) {
        return NULL;
    }

    if ((*env)->PushLocalFrame(env, 3) < 0) {
        return NULL;
    }

    propname = (*env)->NewStringUTF(env, key);
    clazz = (*env)->FindClass(env, "com/sun/j2me/main/Configuration");
    methodID = (*env)->GetStaticMethodID(env, clazz, "getProperty",
                                    "(Ljava/lang/String;)Ljava/lang/String;");
    prop = (jstring)(*env)->CallStaticObjectMethod(env, clazz, methodID,
                                                   propname);
    (*env)->DeleteLocalRef(env, (jobject)propname);
    (*env)->DeleteLocalRef(env, (jobject)clazz);

    if (JNU_IsNull(env, prop)) {
        (*env)->DeleteLocalRef(env, (jobject)prop);
        (*env)->PopLocalFrame(env, NULL);
        return NULL;
    }

    len = (*env)->GetStringUTFLength(env, prop);
    if (len >= length) {
        (*env)->DeleteLocalRef(env, (jobject)prop);
        (*env)->PopLocalFrame(env, NULL);
        return NULL;
    }

    (*env)->GetStringUTFRegion(env, prop, 0, len, buffer);
    buffer[len] = 0;
    (*env)->DeleteLocalRef(env, (jobject)prop);

    (*env)->PopLocalFrame(env, NULL);

    return (const char*)buffer;
}

/**
 * Stores <code>JavaVM</code> instance in a static variable for later use.
 */
JNIEXPORT void JNICALL
Java_com_sun_j2me_main_Configuration_initialize(JNIEnv *env, jclass cls) {
    if ((*env)->GetJavaVM(env, &jvm) != 0) {
        JNU_ThrowByName(env, "java/lang/RuntimeException",
                        "cannot get Java VM interface");
    }
}
