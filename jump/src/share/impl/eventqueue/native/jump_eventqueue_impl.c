/*
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

#include <jni.h>
#include <jni_util.h>
#include <javacall_events.h>


/* private native boolean initEventSystem(); */
JNIEXPORT jboolean JNICALL
Java_com_sun_jumpimpl_module_eventqueue_EventQueueModuleImpl_initEventSystem(
    JNIEnv *env, jobject this) {

    if (JAVACALL_OK == javacall_events_init()) {
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

/* private native boolean shutdownEventSystem(); */
JNIEXPORT jboolean JNICALL
Java_com_sun_jumpimpl_module_eventqueue_EventQueueModuleImpl_shutdownEventSystem(
    JNIEnv *env, jobject this) {

    if (JAVACALL_OK == javacall_events_finalize()) {
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

/* private native int receiveEvent(EventData event); */
JNIEXPORT jint JNICALL
Java_com_sun_jumpimpl_module_eventqueue_EventQueueModuleImpl_receiveEvent(
    JNIEnv *env, jobject this, jobject event) {

    unsigned char buf[JAVACALL_MAX_EVENT_SIZE];
    int eventLen, dataLen;
    jclass eventClass;
    jfieldID eventId;
    jfieldID eventData;

    if (JAVACALL_OK != javacall_event_receive(-1, buf, JAVACALL_MAX_EVENT_SIZE, &eventLen)) {
        return -1;
    }

    dataLen = eventLen - 2 * sizeof(int);
    if (dataLen < 0) {
        return -1;
    }

    eventClass = (*env)->GetObjectClass(env, event);
    eventId = (*env)->GetFieldID(env, eventClass, "id", "I");
    eventData = (*env)->GetFieldID(env, eventClass, "data", "[B");
    (*env)->SetIntField(env, event, eventId, *((int*)buf + 1));
    if (0 == dataLen) {
        (*env)->SetObjectField(env, event, eventData, NULL);
    } else {
        jbyteArray data = (*env)->NewByteArray(env, dataLen);
        if (IS_NULL(data)) {
            JNU_ThrowOutOfMemoryError(env, "Not enough memory for native event");
            return -1;
        }
        (*env)->SetByteArrayRegion(env, data, 0, dataLen, (jbyte*)(buf + 2 * sizeof(int)));
        (*env)->SetObjectField(env, event, eventData, data);
    }

    return *(int*)buf;
}
