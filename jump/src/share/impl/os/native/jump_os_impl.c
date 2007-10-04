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

/* Throws exceptions with no-arg constructors, for which we can't
   use JNU_ThrowByName which constructs with a String arg. */
static void
throw_by_name(JNIEnv *env, const char *class_name)
{
    jobject ex =
	JNU_NewObjectByName(env, class_name, "()V");
    if (ex == NULL) {
	return;
    }
    (*env)->Throw(env, ex);
    (*env)->DeleteLocalRef(env, ex);
}

static void
throw_with_messagetype(JNIEnv *env, const char *class_name, const char *type)
{
    char message[80];
    int len = sizeof(message) - 30;

    snprintf(message, sizeof(message),
	     "messageType type=%.*s%s",
	     len, type, strlen(type) > len ? "..." : "");

    JNU_ThrowByName(env, class_name, message);
}


static const char *
code_to_string(JUMPMessageStatusCode code)
{
    switch (code) {
      case JUMP_TARGET_NONEXISTENT:
	return "JUMP_TARGET_NONEXISTENT";
      case JUMP_TIMEOUT:
	return "JUMP_TIMEOUT";
      case JUMP_SUCCESS:
	return "JUMP_SUCCESS";
      case JUMP_FAILURE:
	return "JUMP_FAILURE";
      case JUMP_OUT_OF_MEMORY:
	return "JUMP_OUT_OF_MEMORY";
      case JUMP_WOULD_BLOCK:
	return "JUMP_WOULD_BLOCK";
      case JUMP_OVERRUN:
	return "JUMP_OVERRUN";
      case JUMP_NEGATIVE_ARRAY_LENGTH:
	return "JUMP_NEGATIVE_ARRAY_LENGTH";
      case JUMP_UNBLOCKED:
	return "JUMP_UNBLOCKED";
      case JUMP_NO_SUCH_QUEUE:
	return "JUMP_NO_SUCH_QUEUE";
      default:
	return NULL;
    }
}

/* Throws a generic IOException with a message like
   "JUMPMessageStatusCode: N".  This is for exceptions we're not
   expecting or exceptions that don't need specific reporting. */
static void
throw_IOException(JNIEnv *env, JUMPMessageStatusCode code)
{
    const char *code_string;
    char code_string_buf[16];
    char message[80];

    code_string = code_to_string(code);
    if (code_string == NULL) {
	snprintf(code_string_buf, sizeof(code_string_buf), "%d", code);
	code_string = code_string_buf;
    }

    snprintf(message, sizeof(message), "JUMPMessageStatusCode: %s",
	     code_string);

    JNU_ThrowByName(env, "java/io/IOException", message);
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
	JNU_ThrowOutOfMemoryError(env, "in jumpMessageGetReturnTypeName");
	return NULL;
    }
    ret = (*env)->NewStringUTF(env, name);
    free(name);

    return ret;
}

/* On success, returns a new JUMPOutgoingMessage.  On failure, returns
   NULL and throws an OutOfMemoryError or IOException. */
static JUMPOutgoingMessage
new_outgoing_message_from_byte_array(
    JNIEnv *env, 
    jbyteArray messageBytes,
    jboolean isResponse)
{
    jbyte *buffer = NULL;
    jsize length;
    JUMPOutgoingMessage m;
    JUMPMessageStatusCode code;

    length = (*env)->GetArrayLength(env, messageBytes);
    if (length > JUMP_MESSAGE_BUFFER_SIZE) {
	JNU_ThrowByName(env, "java/io/IOException",
			"Maximum message size exceeded.");
	goto error;
    }

    buffer = malloc(JUMP_MESSAGE_BUFFER_SIZE);
    if (buffer == NULL) {
	JNU_ThrowOutOfMemoryError(env, "in new_outgoing_message_from_byte_array");
	goto error;
    }

    (*env)->GetByteArrayRegion(env, messageBytes, 0, length, buffer);
    if ((*env)->ExceptionOccurred(env) != NULL) {
	goto error;
    }

    m = jumpMessageNewOutgoingFromBuffer(buffer, isResponse, &code);
    if (m == NULL) {
	switch (code) {
	  case JUMP_OUT_OF_MEMORY:
	    JNU_ThrowOutOfMemoryError(env, "in jumpMessageNewOutgoingFromBuffer");
	    break;

	  default:
	    throw_IOException(env, code);
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

    retVal = (*env)->NewByteArray(env, JUMP_MESSAGE_BUFFER_SIZE);
    if (retVal == NULL) {
	return NULL;
    }

    (*env)->SetByteArrayRegion(env, retVal, 0, JUMP_MESSAGE_BUFFER_SIZE,
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
	  case JUMP_OUT_OF_MEMORY:
	    JNU_ThrowOutOfMemoryError(env, "in jumpMessageSendSync");
	    break;

	  case JUMP_TARGET_NONEXISTENT:
	    throw_with_messagetype(
		env, "com/sun/jump/message/JUMPTargetNonexistentException",
		jumpMessageGetType(m));
	    break;

	  case JUMP_WOULD_BLOCK:
	    throw_with_messagetype(
		env, "com/sun/jump/message/JUMPWouldBlockException",
		jumpMessageGetType(m));
	    break;

	  case JUMP_TIMEOUT:
	    throw_by_name(env, "com/sun/jump/message/JUMPTimedOutException");
	    break;

	  case JUMP_UNBLOCKED:
	    // This shouldn't happen here, unblocked is only for message
	    // queues with registered handlers.
	    // Fall through to default.
	  default:
	    throw_IOException(env, code);
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
	  case JUMP_OUT_OF_MEMORY:
	    JNU_ThrowOutOfMemoryError(env, "in jumpMessageWaitFor");
	    break;

	  case JUMP_TIMEOUT:
	    throw_by_name(env, "com/sun/jump/message/JUMPTimedOutException");
	    break;

	  case JUMP_UNBLOCKED:
	    throw_by_name(env, "com/sun/jump/message/JUMPUnblockedException");
	    break;

	  case JUMP_NO_SUCH_QUEUE:
	    // The design of the Java code should not allow this.
	    // Fall through to default.
	  default:
	    throw_IOException(env, code);
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
      case JUMP_SUCCESS:
	break;

      case JUMP_OUT_OF_MEMORY:
	JNU_ThrowOutOfMemoryError(env, "in jumpMessageSendSync");
	break;

      case JUMP_TARGET_NONEXISTENT:
	throw_with_messagetype(
	    env, "com/sun/jump/message/JUMPTargetNonexistentException",
	    jumpMessageGetType(m));
	break;

      case JUMP_WOULD_BLOCK:
	throw_with_messagetype(
	    env, "com/sun/jump/message/JUMPWouldBlockException",
	    jumpMessageGetType(m));
	break;

      default:
	throw_IOException(env, code);
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
      case JUMP_SUCCESS:
	break;

      case JUMP_OUT_OF_MEMORY:
	JNU_ThrowOutOfMemoryError(env, "in jumpMessageSendSync");
	break;

      case JUMP_TARGET_NONEXISTENT:
	throw_with_messagetype(
	    env, "com/sun/jump/message/JUMPTargetNonexistentException",
	    jumpMessageGetType(m));
	break;

      case JUMP_WOULD_BLOCK:
	throw_with_messagetype(
	    env, "com/sun/jump/message/JUMPWouldBlockException",
	    jumpMessageGetType(m));
	break;

      default:
	throw_IOException(env, code);
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
    JUMPMessageQueueStatusCode mqcode;

    type = (*env)->GetStringUTFChars(env, messageType, NULL);
    if (type == NULL) {
	return;
    }

    /* FIXME: use jumpMessageRegisterDirect. */
    jumpMessageQueueCreate((JUMPPlatformCString)type, &mqcode);
    switch (mqcode) {
      case JUMP_MQ_SUCCESS:
	break;

      case JUMP_MQ_OUT_OF_MEMORY:
	JNU_ThrowOutOfMemoryError(env, "in jumpMessageQueueCreate");
	break;

      default:
	throw_IOException(env, JUMP_FAILURE);
	break;
    }

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

JNIEXPORT void JNICALL
Java_com_sun_jumpimpl_os_JUMPMessageQueueInterfaceImpl_unblock(
    JNIEnv *env, 
    jobject thisObj, 
    jstring messageType)
{
    const char* type;
    JUMPMessageStatusCode code;

    type = (*env)->GetStringUTFChars(env, messageType, NULL);
    if (type == NULL) {
	return;
    }

    jumpMessageUnblock((JUMPPlatformCString)type, &code);
    switch (code) {
      case JUMP_SUCCESS:
	break;

      case JUMP_NO_SUCH_QUEUE:
	// The design of the Java code should not allow this.
	// Fall through to default.
      default:
	throw_IOException(env, code);
	break;
    }

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

JNIEXPORT void JNICALL
Java_com_sun_jumpimpl_os_JUMPOSInterfaceImpl_setTestingMode(
    JNIEnv *env, 
    jobject thisObj,
    jstring filePrefix)
{
    JUMPPlatformCString type = "mvm/server";
    JUMPOutgoingMessage outMessage;
    JUMPAddress targetAddress;
    JUMPMessageStatusCode code;
    jboolean isCopy = JNI_FALSE;
    char *filePrefixStr;

    filePrefixStr = (*env)->GetStringUTFChars(env, filePrefix, &isCopy);

    outMessage = jumpMessageNewOutgoingByType(type, &code);
    jumpMessageAddInt(outMessage, 2);
    jumpMessageAddString(outMessage, "TESTING_MODE");
    jumpMessageAddString(outMessage, filePrefixStr);

    targetAddress.processId = jumpProcessGetServerPid();
#define TIMEOUT 10000
    jumpMessageSendSync(targetAddress, outMessage, TIMEOUT, &code);
    (*env)->ReleaseStringUTFChars(env, filePrefix, filePrefixStr);
}

JNIEXPORT void JNICALL
Java_com_sun_jumpimpl_os_JUMPOSInterfaceImpl_shutdownServer(
    JNIEnv *env,
    jobject thisObj)
{
    JUMPPlatformCString type = "mvm/server";
    JUMPOutgoingMessage outMessage;
    JUMPAddress targetAddress;
    JUMPMessageStatusCode code;

    outMessage = jumpMessageNewOutgoingByType(type, &code);
    jumpMessageAddInt(outMessage, 1);
    jumpMessageAddString(outMessage, "JEXIT");

    jumpMessageQueueCleanQueuesOf(jumpProcessGetExecutiveId());
        
    targetAddress.processId = jumpProcessGetServerPid();                
#define TIMEOUT 10000
    jumpMessageSendSync(targetAddress, outMessage, TIMEOUT, &code);
}