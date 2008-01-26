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

#include <string.h>

#include <javacall_defs.h>
#include <javacall_multimedia.h>

#include <native/common/jni_util.h>

/**
 * Returns current value for the dynamic property corresponding to the
 * given key.
 *
 * @param key key for the property being retrieved.
 * @param fromCache indicates whether property value should be taken from
 *        internal cache. It can be ignored if properties caching is not
 *        supported by underlying implementation.
 * @return current property value
 */
JNIEXPORT jstring JNICALL
Java_com_sun_jsr135_DynamicProperties_nGetPropertyValue(
    JNIEnv *env, jobject this, jstring key, jboolean fromCache)
{
    jstring rv;
    const char *keyName;
    const char *keyValue;
    
    keyName = (*env)->GetStringUTFChars(env, key, 0);
    if (JAVACALL_SUCCEEDED(javacall_media_get_property(keyName, &keyValue))) {
        if (NULL != keyValue)
            rv = JNU_NewStringPlatform(env, keyValue);
    }
    (*env)->ReleaseStringUTFChars(env, key, keyName);

    return rv;
}

/**
 * Tells underlying implementation to cache values of all the properties
 * corresponding to this particular class. This call can be ignored if
 * property caching is not supported.
 */
JNIEXPORT jboolean JNICALL
Java_com_sun_jsr135_DynamicProperties_cacheProperties(JNIEnv *env, jobject this) {
    return JNI_TRUE;
}
