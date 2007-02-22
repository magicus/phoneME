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
    ret = (*env)->NewStringUTF(env, name);
    free(name);

    return ret;
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
    JUMPAddress target;
    JUMPMessageStatusCode code;
    jboolean isCopy;
    jbyte* raw = (*env)->GetByteArrayElements(env, messageBytes, &isCopy);
    jbyteArray retVal;
    jbyte* returnInterior;

    JUMPOutgoingMessage m;
    JUMPMessage r;

    ensureInitialized();

    m = jumpMessageNewOutgoingFromBuffer(raw, isResponse);
    target.processId = pid;
    r = jumpMessageSendSync(target, m, (int32)timeout, &code);
    /* FIXME: Examine returned error code to figure out which exception
       to throw */
    
    retVal = (*env)->NewByteArray(env, MESSAGE_BUFFER_SIZE);
    returnInterior = (*env)->GetPrimitiveArrayCritical(env, retVal, &isCopy);
    memcpy(returnInterior, jumpMessageGetData(r), MESSAGE_BUFFER_SIZE);
    
    (*env)->ReleasePrimitiveArrayCritical(env, retVal, returnInterior, 0);
    
    free(raw);
    return retVal;
}

JNIEXPORT jbyteArray JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_receiveMessage(
    JNIEnv *env, 
    jobject thisObj, 
    jstring messageType, 
    jlong timeout)
{
    jboolean isCopy;
    const char* type = (*env)->GetStringUTFChars(env, messageType, &isCopy);
    jbyteArray retVal;
    jbyte* returnInterior;

    JUMPMessage r;
    JUMPMessageStatusCode code;

    ensureInitialized();

    r = jumpMessageWaitFor((JUMPPlatformCString)type, (int32)timeout, &code);
    /* FIXME: Examine returned error code to figure out which exception
       to throw. Return an error code!! */
    retVal = (*env)->NewByteArray(env, MESSAGE_BUFFER_SIZE);
    returnInterior = (*env)->GetPrimitiveArrayCritical(env, retVal, &isCopy);
    memcpy(returnInterior, jumpMessageGetData(r), MESSAGE_BUFFER_SIZE);
    
    (*env)->ReleasePrimitiveArrayCritical(env, retVal, returnInterior, 0);
    
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
    JUMPAddress target;
    JUMPMessageStatusCode code;
    jboolean isCopy;
    jbyte* raw = (*env)->GetByteArrayElements(env, messageBytes, &isCopy);

    JUMPOutgoingMessage m;

    ensureInitialized();

    m = jumpMessageNewOutgoingFromBuffer(raw, isResponse);
    target.processId = pid;
    jumpMessageSendAsync(target, m, &code);
    
    free(raw);
}

JNIEXPORT void JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_sendMessageResponse(
    JNIEnv *env, 
    jobject thisObj, 
    jbyteArray messageBytes,
    jboolean isResponse)
{
    JUMPMessageStatusCode code;
    jboolean isCopy;
    jbyte* raw = (*env)->GetByteArrayElements(env, messageBytes, &isCopy);

    JUMPOutgoingMessage m;

    ensureInitialized();

    m = jumpMessageNewOutgoingFromBuffer(raw, isResponse);
    jumpMessageSendAsyncResponse(m, &code);
    
    free(raw);
}

JNIEXPORT void JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_reserve(
    JNIEnv *env, 
    jobject thisObj, 
    jstring messageType)
{
    jboolean isCopy;
    const char* type = (*env)->GetStringUTFChars(env, messageType, &isCopy);
    JUMPMessageQueueStatusCode code;
    
    jumpMessageQueueCreate((JUMPPlatformCString)type, &code);
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

JNIEXPORT int JNICALL
Java_com_sun_jumpimpl_os_JUMPOSInterfaceImpl_createProcess(
    JNIEnv *env, 
    jobject thisObj,
    jobjectArray arguments)
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
    
    retVal = jumpProcessCreate(argc, argv);
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

