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

const char* get_system_property_supports_mixing();
const char* get_system_property_supports_audio_capture();
const char* get_system_property_supports_video_capture();
const char* get_system_property_supports_recording();
const char* get_system_property_audio_encodings();
const char* get_system_property_video_encodings();
const char* get_system_property_video_snapshot_encodings();
const char* get_system_property_streamable_contents();

/**
 * Returns current value for the dynamic property
 *
 * @return current property value
 */
JNIEXPORT jstring JNICALL
Java_com_sun_jsr135_DynamicProperties_nGetPropertyValueSupportsMixing(
    JNIEnv *env, jobject this)
{
    return JNU_NewStringPlatform(env, get_system_property_supports_mixing());
}

JNIEXPORT jstring JNICALL
Java_com_sun_jsr135_DynamicProperties_nGetPropertyValueSupportsAudioCapture(
    JNIEnv *env, jobject this)
{
    return JNU_NewStringPlatform(env, get_system_property_supports_audio_capture());
}

JNIEXPORT jstring JNICALL
Java_com_sun_jsr135_DynamicProperties_nGetPropertyValueSupportsVideoCapture(
    JNIEnv *env, jobject this)
{
    return JNU_NewStringPlatform(env, get_system_property_supports_video_capture());
}

JNIEXPORT jstring JNICALL
Java_com_sun_jsr135_DynamicProperties_nGetPropertyValueSupportsRecording(
    JNIEnv *env, jobject this)
{
    return JNU_NewStringPlatform(env, get_system_property_supports_recording());
}

JNIEXPORT jstring JNICALL
Java_com_sun_jsr135_DynamicProperties_nGetPropertyValueAudioEncodings(
    JNIEnv *env, jobject this)
{
    return JNU_NewStringPlatform(env, get_system_property_audio_encodings());
}

JNIEXPORT jstring JNICALL
Java_com_sun_jsr135_DynamicProperties_nGetPropertyValueVideoEncodings(
    JNIEnv *env, jobject this)
{
    return JNU_NewStringPlatform(env, get_system_property_video_encodings());
}

JNIEXPORT jstring JNICALL
Java_com_sun_jsr135_DynamicProperties_nGetPropertyValueVideoSnapshotEncodings(
    JNIEnv *env, jobject this)
{
    return JNU_NewStringPlatform(env, get_system_property_video_snapshot_encodings());
}

JNIEXPORT jstring JNICALL
Java_com_sun_jsr135_DynamicProperties_nGetPropertyValueStreamableContents(
    JNIEnv *env, jobject this)
{
    return JNU_NewStringPlatform(env, get_system_property_streamable_contents());
}
