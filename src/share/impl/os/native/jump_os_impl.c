/*
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
#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include "porting/JUMPMessageQueue.h"
#include "porting/JUMPProcess.h"
#include "jump_messaging.h"
#include "jni_util.h"

static void ensureInitialized() 
{
}

JNIEXPORT jint JNICALL
Java_com_sun_jumpimpl_os_JUMPOSInterfaceImpl_getProcessID(JNIEnv *env, jobject thisObj)
{
    return jumpProcessGetId();
}

JNIEXPORT jint JNICALL
Java_com_sun_jumpimpl_os_JUMPOSInterfaceImpl_getExecutiveProcessID(JNIEnv *env, jobject thisObj)
{
    return jumpProcessGetExecutiveId();
}

JNIEXPORT jint JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_getDataOffset(JNIEnv *env, jobject thisObj)
{
    ensureInitialized();
    return jumpMessageQueueDataOffset();
}

JNIEXPORT jstring JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_getReturnType(JNIEnv *env, jobject thisObj)
{
    char* name;
    jstring ret;

    ensureInitialized();
    
    name = jumpMessageGetReturnTypeName();
    if (name == NULL) {
	JNU_ThrowOutOfMemoryError(env, "jumpMessageGetReturnTypeName");
	return NULL;
    }
    ret = (*env)->NewStringUTF(env, name);
    free(name);

    return ret;
}

static JUMPOutgoingMessage
new_outgoing_message_from_byte_array(
    JNIEnv *env, 
    jbyteArray messageBytes,
    jboolean isResponse)
{
    jbyte *buffer;
    jsize length;
    JUMPOutgoingMessage m;
    JUMPMessageStatusCode code;

    buffer = malloc(MESSAGE_BUFFER_SIZE);
    if (buffer == NULL) {
	JNU_ThrowOutOfMemoryError(env, "malloc");
	return NULL;
    }

    length = (*env)->GetArrayLength(env, messageBytes);
    if (length > MESSAGE_BUFFER_SIZE) {
	length = MESSAGE_BUFFER_SIZE;
    }
    (*env)->GetByteArrayRegion(env, messageBytes, 0, length, buffer);

    m = jumpMessageNewOutgoingFromBuffer(buffer, isResponse, &code);
    if (m == NULL) {
	/* FIXME check code */
	JNU_ThrowOutOfMemoryError(env, "jumpMessageNewOutgoingFromBuffer");
	free(buffer);
	return NULL;
    }

    return m;
}

JNIEXPORT jbyteArray JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_sendMessageSync(
    JNIEnv *env, 
    jobject thisObj, 
    jint pid, 
    jbyteArray messageBytes,
    jboolean isResponse,
    jlong timeout)
{
    jbyteArray retVal = NULL;
    JUMPOutgoingMessage m = NULL;
    JUMPAddress target;
    JUMPMessage r = NULL;
    JUMPMessageStatusCode code;
    jbyte* returnInterior;

    ensureInitialized();

    m = new_outgoing_message_from_byte_array(env, messageBytes, isResponse);
    if (m == NULL) {
	goto out;
    }

    target.processId = pid;
    r = jumpMessageSendSync(target, m, (int32)timeout, &code);
    /* FIXME: Examine returned error code to figure out which exception
       to throw */

    retVal = (*env)->NewByteArray(env, MESSAGE_BUFFER_SIZE);
    if (retVal == NULL) {
	goto out;
    }

    returnInterior = (*env)->GetPrimitiveArrayCritical(env, retVal, NULL);
    if (returnInterior == NULL) {
	retVal = NULL;
	goto out;
    }

    memcpy(returnInterior, jumpMessageGetData(r), MESSAGE_BUFFER_SIZE);
    
    (*env)->ReleasePrimitiveArrayCritical(env, retVal, returnInterior, 0);

  out:
    if (r != NULL) {
	jumpMessageFree(r);
    }
    if (m != NULL) {
	jumpMessageFreeOutgoing(m);
    }
    
    return retVal;
}

JNIEXPORT jbyteArray JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_receiveMessage(
    JNIEnv *env, 
    jobject thisObj, 
    jstring messageType, 
    jlong timeout)
{
    const char* type = NULL;
    JUMPMessage r = NULL;
    JUMPMessageStatusCode code;
    jbyteArray retVal = NULL;
    jbyte* returnInterior;

    ensureInitialized();

    type = (*env)->GetStringUTFChars(env, messageType, NULL);
    if (type == NULL) {
	goto out;
    }

    r = jumpMessageWaitFor((JUMPPlatformCString)type, (int32)timeout, &code);
    /* FIXME: Examine returned error code to figure out which exception
       to throw. Return an error code!! */

    retVal = (*env)->NewByteArray(env, MESSAGE_BUFFER_SIZE);
    if (retVal == NULL) {
	goto out;
    }

    returnInterior = (*env)->GetPrimitiveArrayCritical(env, retVal, NULL);
    if (returnInterior == NULL) {
	retVal = NULL;
	goto out;
    }
    memcpy(returnInterior, jumpMessageGetData(r), MESSAGE_BUFFER_SIZE);
    
    (*env)->ReleasePrimitiveArrayCritical(env, retVal, returnInterior, 0);

  out:
    if (r != NULL) {
	jumpMessageFree(r);
    }
    return retVal;
}

JNIEXPORT void JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_sendMessageAsync(
    JNIEnv *env, 
    jobject thisObj, 
    jint pid, 
    jbyteArray messageBytes,
    jboolean isResponse)
{
    JUMPOutgoingMessage m = NULL;
    JUMPAddress target;
    JUMPMessageStatusCode code;

    ensureInitialized();

    m = new_outgoing_message_from_byte_array(env, messageBytes, isResponse);
    if (m == NULL) {
	goto out;
    }

    target.processId = pid;
    jumpMessageSendAsync(target, m, &code);
    
  out:
    if (m != NULL) {
	jumpMessageFreeOutgoing(m);
    }
}

JNIEXPORT void JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_sendMessageResponse(
    JNIEnv *env, 
    jobject thisObj, 
    jbyteArray messageBytes,
    jboolean isResponse)
{
    JUMPOutgoingMessage m = NULL;
    JUMPMessageStatusCode code;

    ensureInitialized();

    m = new_outgoing_message_from_byte_array(env, messageBytes, isResponse);
    if (m == NULL) {
	goto out;
    }

    jumpMessageSendAsyncResponse(m, &code);
    /* FIXME: Examine returned error code to figure out which exception
       to throw */

  out:
    if (m != NULL) {
	jumpMessageFreeOutgoing(m);
    }
}

JNIEXPORT void JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_reserve(
    JNIEnv *env, 
    jobject thisObj, 
    jstring messageType)
{
    const char* type;
    JUMPMessageQueueStatusCode code;

    type = (*env)->GetStringUTFChars(env, messageType, NULL);
    if (type == NULL) {
	return;
    }

    jumpMessageQueueCreate((JUMPPlatformCString)type, &code);

    (*env)->ReleaseStringUTFChars(env, messageType, type);
}

JNIEXPORT void JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_unreserve(
    JNIEnv *env, 
    jobject thisObj, 
    jstring messageType)
{
    const char* type;

    type = (*env)->GetStringUTFChars(env, messageType, NULL);
    if (type == NULL) {
	return;
    }

    jumpMessageQueueDestroy((JUMPPlatformCString)type);

    (*env)->ReleaseStringUTFChars(env, messageType, type);
}

static int
create_process(
    JNIEnv *env, 
    jobject thisObj,
    jobjectArray arguments,
    int isNative)
{
    int argc;
    char** argv;
    int i;
    int retVal;
    jboolean isCopy;
    
    if (arguments == NULL) {
	argc = 0;
	argv = NULL;
    } else {
	argc = (*env)->GetArrayLength(env, arguments);
	argv = calloc(argc, sizeof(char*));
	if (argv == NULL) {
	    return -1;
	}
	
	for (i = 0; i < argc; i++) {
	    jobject argObj = (*env)->GetObjectArrayElement(env, arguments, i);
	    char* arg = (char*)(*env)->GetStringUTFChars(env, argObj, &isCopy);
	    argv[i] = arg;
	}
    }
    
    if (isNative) {
	retVal = jumpProcessNativeCreate(argc, argv);
    } else {
	retVal = jumpProcessCreate(argc, argv);
    }
    if (argv != NULL) {
	if (isCopy) {
	    for (i = 0; i < argc; i++) {
		free(argv[i]);
	    }
	}
	free(argv);
    }
    return retVal;
}

JNIEXPORT int JNICALL
Java_com_sun_jumpimpl_os_JUMPOSInterfaceImpl_createProcess(
    JNIEnv *env, 
    jobject thisObj,
    jobjectArray arguments)
{
    return create_process(env, thisObj, arguments, 0);
}

JNIEXPORT int JNICALL
Java_com_sun_jumpimpl_os_JUMPOSInterfaceImpl_createProcessNative(
    JNIEnv *env, 
    jobject thisObj,
    jobjectArray arguments)
{
    return create_process(env, thisObj, arguments, 1);
}

