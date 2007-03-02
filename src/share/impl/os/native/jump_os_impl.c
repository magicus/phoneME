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

static jboolean messaging_started = JNI_FALSE;
static void ensureInitialized() 
{
    /*
     * FIXME:
     * This is really for stand-alone execution.
     * We should really make this part of JVM initialization. 
     */
    if (!messaging_started) {
	jumpMessageStart();
	messaging_started = JNI_TRUE;
    }
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

/* On success, returns a new JUMPOutgoingMessage.  On failure, returns
   NULL and throws an Exception. */
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
	JNU_ThrowOutOfMemoryError(env, "new_outgoing_message_from_byte_array");
	goto error;
    }

    length = (*env)->GetArrayLength(env, messageBytes);
    if (length > MESSAGE_BUFFER_SIZE) {
	length = MESSAGE_BUFFER_SIZE;
    }

    (*env)->GetByteArrayRegion(env, messageBytes, 0, length, buffer);
    if ((*env)->ExceptionOccurred(env) != NULL) {
	goto error;
    }

    m = jumpMessageNewOutgoingFromBuffer(buffer, isResponse, &code);
    if (m == NULL) {
	switch (code) {
	    char message[80];

	  case JUMP_OUT_OF_MEMORY:
	    JNU_ThrowOutOfMemoryError(env, "jumpMessageNewOutgoingFromBuffer");
	    break;

	  default:
	    snprintf(message, sizeof(message),
		     "JUMPMessageStatusCode: %d", code);
	    JNU_ThrowByName(env, "XXX", message);
	    break;
	}
	goto error;
    }

    return m;

  error:
    free(buffer);
    return NULL;
}

/* On success, returns a new jbyteArray.  On failure, returns NULL and
   throws an Exception. */
static jbyteArray
new_byte_array_from_message(
    JNIEnv *env,
    JUMPMessage message)
{
    jbyteArray retVal;

    /* FIXME: use the actual message size.  Currently the actual
       message size will always be MESSAGE_BUFFER_SIZE because of
       how jump_messaging works, but there is schizophrenia between
       assuming MESSAGE_BUFFER_SIZE, and passing length around. */
    retVal = (*env)->NewByteArray(env, MESSAGE_BUFFER_SIZE);
    if (retVal == NULL) {
	return NULL;
    }

    (*env)->SetByteArrayRegion(env, retVal, 0, MESSAGE_BUFFER_SIZE,
			       jumpMessageGetData(message));
    if ((*env)->ExceptionOccurred(env)) {
	(*env)->DeleteLocalRef(env, retVal);
	return NULL;
    }

    return retVal;
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

    ensureInitialized();

    m = new_outgoing_message_from_byte_array(env, messageBytes, isResponse);
    if (m == NULL) {
	/* Exception already thrown. */
	goto out;
    }

    target.processId = pid;
    r = jumpMessageSendSync(target, m, (int32)timeout, &code);
    if (r == NULL) {
	switch (code) {
	    char message[80];

	  case JUMP_OUT_OF_MEMORY:
	    JNU_ThrowOutOfMemoryError(env, "jumpMessageSendSync");
	    break;

	  case JUMP_TARGET_NONEXISTENT:
	    JNU_ThrowByName(env, "XXX", message);
	    break;

	  case JUMP_WOULD_BLOCK:
	    JNU_ThrowByName(env, "XXX", message);
	    break;

	  case JUMP_TIMEOUT:
	    JNU_ThrowByName(env, "XXX", message);
	    break;

	  case JUMP_UNBLOCKED:
	    JNU_ThrowByName(env, "XXX", NULL);
	    break;

	  default:
	    snprintf(message, sizeof(message),
		     "JUMPMessageStatusCode: %d", code);
	    JNU_ThrowByName(env, "XXX", message);
	    break;
	}
	goto out;
    }

    retVal = new_byte_array_from_message(env, r);
    if (retVal == NULL) {
	/* Exception already thrown. */
	goto out;
    }

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

    ensureInitialized();

    type = (*env)->GetStringUTFChars(env, messageType, NULL);
    if (type == NULL) {
	goto out;
    }

    r = jumpMessageWaitFor((JUMPPlatformCString)type, (int32)timeout, &code);
    if (r == NULL) {
	switch (code) {
	    char message[80];

	  case JUMP_OUT_OF_MEMORY:
	    JNU_ThrowOutOfMemoryError(env, "jumpMessageWaitFor");
	    break;

	  case JUMP_NO_SUCH_QUEUE:
	    JNU_ThrowByName(env, "XXX", NULL);
	    break;

	  case JUMP_TIMEOUT:
	    JNU_ThrowByName(env, "XXX", NULL);
	    break;

	  case JUMP_UNBLOCKED:
	    JNU_ThrowByName(env, "XXX", NULL);
	    break;

	  default:
	    snprintf(message, sizeof(message),
		     "JUMPMessageStatusCode: %d", code);
	    JNU_ThrowByName(env, "XXX", message);
	    break;
	}
	goto out;
    }

    retVal = new_byte_array_from_message(env, r);
    if (retVal == NULL) {
	/* Exception already thrown. */
	goto out;
    }

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
	/* Exception already thrown. */
	goto out;
    }

    target.processId = pid;
    jumpMessageSendAsync(target, m, &code);
    switch (code) {
	char message[80];

      case JUMP_SUCCESS:
	break;

      case JUMP_OUT_OF_MEMORY:
	JNU_ThrowOutOfMemoryError(env, "jumpMessageSendSync");
	break;

      case JUMP_TARGET_NONEXISTENT:
	JNU_ThrowByName(env, "XXX", message);
	break;

      case JUMP_WOULD_BLOCK:
	JNU_ThrowByName(env, "XXX", message);
	break;

      default:
	snprintf(message, sizeof(message),
		 "JUMPMessageStatusCode: %d", code);
	JNU_ThrowByName(env, "XXX", message);
	break;
    }

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
	/* Exception already thrown. */
	goto out;
    }

    jumpMessageSendAsyncResponse(m, &code);
    switch (code) {
	char message[80];

      case JUMP_SUCCESS:
	break;

      case JUMP_OUT_OF_MEMORY:
	JNU_ThrowOutOfMemoryError(env, "jumpMessageSendSync");
	break;

      case JUMP_TARGET_NONEXISTENT:
	JNU_ThrowByName(env, "XXX", message);
	break;

      case JUMP_WOULD_BLOCK:
	JNU_ThrowByName(env, "XXX", message);
	break;

      default:
	snprintf(message, sizeof(message),
		 "JUMPMessageStatusCode: %d", code);
	JNU_ThrowByName(env, "XXX", message);
	break;
    }

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

    /* FIXME: use jumpMessageRegisterDirect. */
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

    /* FIXME: use jumpMessageCancelRegistration. */
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

