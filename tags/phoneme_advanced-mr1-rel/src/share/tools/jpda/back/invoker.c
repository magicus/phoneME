/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */
#include <stdio.h>
#include <jvmdi.h>
#include <jni.h> 
#include <string.h>

#include "invoker.h"
#include "eventHandler.h"
#include "threadControl.h"
#include "outStream.h"
#include "util.h"
#include "JDWP.h"

static JVMDI_RawMonitor invokerLock;

void 
invoker_initialize() 
{
    invokerLock = debugMonitorCreate("JDWP Invocation Lock");
}

void 
invoker_reset() 
{
}

void invoker_lock()
{
    debugMonitorEnter(invokerLock);
}

void invoker_unlock()
{
    debugMonitorExit(invokerLock);
}

static jbyte
returnTypeTag(char *signature)
{
    char *tagPtr = strchr(signature, SIGNATURE_END_ARGS);
    JDI_ASSERT(tagPtr);
    tagPtr++;    /* 1st character after the end of args */
    return (jbyte)*tagPtr;
}

static jbyte
nextArgumentTypeTag(void **cursor)
{
    char *tagPtr = *cursor;
    jbyte argumentTag = (jbyte)*tagPtr;

    if (*tagPtr != SIGNATURE_END_ARGS) {
        /* Skip any array modifiers */
        while (*tagPtr == JDWP_Tag_ARRAY) {
            tagPtr++;
        }
        /* Skip class name */
        if (*tagPtr == JDWP_Tag_OBJECT) {
            tagPtr = strchr(tagPtr, SIGNATURE_END_CLASS) + 1;
            JDI_ASSERT(tagPtr);
        } else {
            /* Skip primitive sig */
            tagPtr++;
        }
    }

    *cursor = tagPtr;
    return argumentTag;
}

static jbyte
firstArgumentTypeTag(char *signature, void **cursor)
{
    JDI_ASSERT(signature[0] == SIGNATURE_BEGIN_ARGS);
    *cursor = signature + 1; /* skip to the first arg */
    return nextArgumentTypeTag(cursor);
}


/*
 * Note: argument refs may be destroyed on out-of-memory error 
 */
static jint
createGlobalRefs(JNIEnv *env, InvokeRequest *request) {
    jclass clazz = NULL;
    jobject instance = NULL;
    jint argIndex = 0;
    jbyte argumentTag;
    jvalue *argument;
    void *cursor;

    clazz = (*env)->NewGlobalRef(env, request->clazz);
    if (clazz == NULL) {
        goto handleError;
    }

    if (request->instance != NULL) {
        instance = (*env)->NewGlobalRef(env, request->instance);
        if (instance == NULL) {
            goto handleError;
        }
    }

    argumentTag = firstArgumentTypeTag(request->methodSignature, &cursor);
    argument = request->arguments;
    while (argumentTag != SIGNATURE_END_ARGS) {
        if ((argumentTag == JDWP_Tag_OBJECT) ||
            (argumentTag == JDWP_Tag_ARRAY)) {
            /* Create a global ref for any non-null argument */
            if (argument->l != NULL) {
                argument->l = (*env)->NewGlobalRef(env, argument->l);
                if (argument->l == NULL) {
                    goto handleError;
                }
            }
        }
        argument++;
        argIndex++;
        argumentTag = nextArgumentTypeTag(&cursor);
    }

    request->clazz = clazz;
    request->instance = instance;

    return JVMDI_ERROR_NONE;

handleError:
    {
        int i;
        (*env)->DeleteGlobalRef(env, clazz);
        (*env)->DeleteGlobalRef(env, instance);

        argumentTag = firstArgumentTypeTag(request->methodSignature, &cursor);
        argument = request->arguments;
        i = 0;
        while ((argumentTag != SIGNATURE_END_ARGS) && (i < argIndex)) {
            if ((argumentTag == JDWP_Tag_OBJECT) ||
                (argumentTag == JDWP_Tag_ARRAY)) {
                /* Delete global ref for any non-null argument */
                if (argument->l != NULL) {
                    (*env)->DeleteGlobalRef(env, argument->l);
                }
            }
            argument++;
            i++;
            argumentTag = nextArgumentTypeTag(&cursor);
        }
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
}

/* Unused:
static void
deleteGlobalRefs(JNIEnv *env, InvokeRequest *request) {
    jbyte argumentTag;
    jvalue *argument;
    void *cursor;

    (*env)->DeleteGlobalRef(env, request->clazz);
    (*env)->DeleteGlobalRef(env, request->instance);

    argumentTag = firstArgumentTypeTag(request->methodSignature, &cursor);
    argument = request->arguments;
    while (argumentTag != SIGNATURE_END_ARGS) {
        if ((argumentTag == JDWP_Tag_OBJECT) ||
            (argumentTag == JDWP_Tag_ARRAY)) {
            (*env)->DeleteGlobalRef(env, argument->l);
        }
        argumentTag = nextArgumentTypeTag(&cursor);
        argument++;
    }
}
*/

static jint
fillInvokeRequest(JNIEnv *env, InvokeRequest *request,
                  jbyte invokeType, jbyte options, jint id,
                  jthread thread, jclass clazz, jmethodID method, 
                  jobject instance, jvalue *arguments)
{
    jint error;
    char *name;
    if (!request->available) {
        /*
         * Thread is not at a point where it can invoke.
         */
        return JVMDI_ERROR_INVALID_THREAD;
    }
    if (request->pending) {
        /*
         * Pending invoke
         */
        return JDWP_Error_ALREADY_INVOKING;
    }

    request->invokeType = invokeType;
    request->options = options;
    request->detached = JNI_FALSE;
    request->id = id;
    request->clazz = clazz;
    request->method = method;
    request->instance = instance;
    request->arguments = arguments;

    request->returnValue.j = 0;
    request->exception = 0;

    /*
     * Squirrel away the method signature
     */
    error = jvmdi->GetMethodName(clazz, method, &name, 
                                      &request->methodSignature);
    if (error != JVMDI_ERROR_NONE) {
        return error;
    }
    jdwpFree(name);

    /*
     * The given references for class and instance are not guaranteed
     * to be around long enough for invocation, so create new ones
     * here.
     */
    error = createGlobalRefs(env, request);
    if (error != JVMDI_ERROR_NONE) {
        jdwpFree(request->methodSignature);
        return error;
    }

    request->pending = JNI_TRUE;
    request->available = JNI_FALSE;
    return JVMDI_ERROR_NONE;
}

void
invoker_enableInvokeRequests(jthread thread) 
{
    InvokeRequest *request;
     
    JDI_ASSERT(thread);

    request = threadControl_getInvokeRequest(thread);
    if (request == NULL) {
        ERROR_CODE_EXIT(JVMDI_ERROR_INVALID_THREAD);
    }

    request->available = JNI_TRUE;
}

jint 
invoker_requestInvoke(jbyte invokeType, jbyte options, jint id,
                      jthread thread, jclass clazz, jmethodID method, 
                      jobject instance, jvalue *arguments)
{
    JNIEnv *env = getEnv();
    InvokeRequest *request;
    jint error = JVMDI_ERROR_NONE;

    debugMonitorEnter(invokerLock);
    request = threadControl_getInvokeRequest(thread);
    if (request != NULL) {
        error = fillInvokeRequest(env, request, invokeType, options, id,
                                  thread, clazz, method, instance, arguments); 
    }
    debugMonitorExit(invokerLock);

    if (error == JVMDI_ERROR_NONE) {
        if (options & JDWP_InvokeOptions_INVOKE_SINGLE_THREADED) {
            /* true means it is okay to unblock the commandLoop thread */
            threadControl_resumeThread(thread, JNI_TRUE);
        } else {
            threadControl_resumeAll();
        }
    }

    return error;
}

static void
invokeConstructor(JNIEnv *env, InvokeRequest *request)
{
    jobject object;
    object = (*env)->NewObjectA(env, request->clazz,
                                     request->method, 
                                     request->arguments);
    if (object != NULL) {
        object = (*env)->NewGlobalRef(env, object);
        if (object == NULL) {
            ERROR_MESSAGE_EXIT("Unable to create global reference");
        }
    }
    request->returnValue.l = object;
}

static void 
invokeStatic(JNIEnv *env, InvokeRequest *request)
{
    switch(returnTypeTag(request->methodSignature)) {
        case JDWP_Tag_OBJECT:
        case JDWP_Tag_ARRAY: {
            jobject object;
            object = (*env)->CallStaticObjectMethodA(env,
                                       request->clazz,
                                       request->method,
                                       request->arguments);
            if (object != NULL) {
                object = (*env)->NewGlobalRef(env, object);
                if (object == NULL) {
                    ERROR_MESSAGE_EXIT("Unable to create global reference");
                }
            }
            request->returnValue.l = object;
            break;
        }


        case JDWP_Tag_BYTE:
            request->returnValue.b = (*env)->CallStaticByteMethodA(env,
                                                       request->clazz,
                                                       request->method,
                                                       request->arguments);
            break;

        case JDWP_Tag_CHAR:
            request->returnValue.c = (*env)->CallStaticCharMethodA(env,
                                                       request->clazz,
                                                       request->method,
                                                       request->arguments);
            break;

        case JDWP_Tag_FLOAT:
            request->returnValue.f = (*env)->CallStaticFloatMethodA(env,
                                                       request->clazz,
                                                       request->method,
                                                       request->arguments);
            break;

        case JDWP_Tag_DOUBLE:
            request->returnValue.d = (*env)->CallStaticDoubleMethodA(env,
                                                       request->clazz,
                                                       request->method,
                                                       request->arguments);
            break;

        case JDWP_Tag_INT:
            request->returnValue.i = (*env)->CallStaticIntMethodA(env,
                                                       request->clazz,
                                                       request->method,
                                                       request->arguments);
            break;

        case JDWP_Tag_LONG:
            request->returnValue.j = (*env)->CallStaticLongMethodA(env,
                                                       request->clazz,
                                                       request->method,
                                                       request->arguments);
            break;

        case JDWP_Tag_SHORT:
            request->returnValue.s = (*env)->CallStaticShortMethodA(env,
                                                       request->clazz,
                                                       request->method,
                                                       request->arguments);
            break;

        case JDWP_Tag_BOOLEAN:
            request->returnValue.z = (*env)->CallStaticBooleanMethodA(env,
                                                       request->clazz,
                                                       request->method,
                                                       request->arguments);
            break;

        case JDWP_Tag_VOID:
            (*env)->CallStaticVoidMethodA(env,
                                          request->clazz,
                                          request->method,
                                          request->arguments);
            break;

        default:
        {
            char buf[200];
            sprintf(buf, "Invalid method signature: %s", request->methodSignature);
            ERROR_MESSAGE_EXIT(buf);
        }
    }
}

static void 
invokeVirtual(JNIEnv *env, InvokeRequest *request)
{
    switch(returnTypeTag(request->methodSignature)) {
        case JDWP_Tag_OBJECT:
        case JDWP_Tag_ARRAY: {
            jobject object;
            object = (*env)->CallObjectMethodA(env,
                                 request->instance,
                                 request->method,
                                 request->arguments);
            if (object != NULL) {
                object = (*env)->NewGlobalRef(env, object);
                if (object == NULL) {
                    ERROR_MESSAGE_EXIT("Unable to create global reference");
                }
            }
            request->returnValue.l = object;
            break;
        }

        case JDWP_Tag_BYTE:
            request->returnValue.b = (*env)->CallByteMethodA(env,
                                                 request->instance,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_CHAR:
            request->returnValue.c = (*env)->CallCharMethodA(env,
                                                 request->instance,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_FLOAT:
            request->returnValue.f = (*env)->CallFloatMethodA(env,
                                                 request->instance,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_DOUBLE:
            request->returnValue.d = (*env)->CallDoubleMethodA(env,
                                                 request->instance,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_INT:
            request->returnValue.i = (*env)->CallIntMethodA(env,
                                                 request->instance,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_LONG:
            request->returnValue.j = (*env)->CallLongMethodA(env,
                                                 request->instance,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_SHORT:
            request->returnValue.s = (*env)->CallShortMethodA(env,
                                                 request->instance,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_BOOLEAN:
            request->returnValue.z = (*env)->CallBooleanMethodA(env,
                                                 request->instance,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_VOID:
            (*env)->CallVoidMethodA(env,
                                    request->instance,
                                    request->method,
                                    request->arguments);
            break;

        default:
        {
            char buf[200];
            sprintf(buf, "Invalid method signature: %s", request->methodSignature);
            ERROR_MESSAGE_EXIT(buf);
        }
    }
}

static void 
invokeNonvirtual(JNIEnv *env, InvokeRequest *request)
{
    switch(returnTypeTag(request->methodSignature)) {
        case JDWP_Tag_OBJECT:
        case JDWP_Tag_ARRAY: {
            jobject object;
            object = (*env)->CallNonvirtualObjectMethodA(env,
                                           request->instance,
                                           request->clazz,
                                           request->method,
                                           request->arguments);
            if (object != NULL) {
                object = (*env)->NewGlobalRef(env, object);
                if (object == NULL) {
                    ERROR_MESSAGE_EXIT("Unable to create global reference");
                }
            }
            request->returnValue.l = object;
            break;
        }

        case JDWP_Tag_BYTE:
            request->returnValue.b = (*env)->CallNonvirtualByteMethodA(env,
                                                 request->instance,
                                                 request->clazz,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_CHAR:
            request->returnValue.c = (*env)->CallNonvirtualCharMethodA(env,
                                                 request->instance,
                                                 request->clazz,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_FLOAT:
            request->returnValue.f = (*env)->CallNonvirtualFloatMethodA(env,
                                                 request->instance,
                                                 request->clazz,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_DOUBLE:
            request->returnValue.d = (*env)->CallNonvirtualDoubleMethodA(env,
                                                 request->instance,
                                                 request->clazz,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_INT:
            request->returnValue.i = (*env)->CallNonvirtualIntMethodA(env,
                                                 request->instance,
                                                 request->clazz,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_LONG:
            request->returnValue.j = (*env)->CallNonvirtualLongMethodA(env,
                                                 request->instance,
                                                 request->clazz,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_SHORT:
            request->returnValue.s = (*env)->CallNonvirtualShortMethodA(env,
                                                 request->instance,
                                                 request->clazz,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_BOOLEAN:
            request->returnValue.z = (*env)->CallNonvirtualBooleanMethodA(env,
                                                 request->instance,
                                                 request->clazz,
                                                 request->method,
                                                 request->arguments);
            break;

        case JDWP_Tag_VOID:
            (*env)->CallNonvirtualVoidMethodA(env,
                                    request->instance,
                                    request->clazz,
                                    request->method,
                                    request->arguments);
            break;

        default:
        {
            char buf[200];
            sprintf(buf, "Invalid method signature: %s", request->methodSignature);
            ERROR_MESSAGE_EXIT(buf);
        }
    }
}

jboolean
invoker_doInvoke(jthread thread)
{
    JNIEnv *env;
    jboolean startNow;
    InvokeRequest *request;
     
    JDI_ASSERT(thread);

    debugMonitorEnter(invokerLock);

    request = threadControl_getInvokeRequest(thread);
    if (request == NULL) {
        ERROR_CODE_EXIT(JVMDI_ERROR_INVALID_THREAD);
    }

    request->available = JNI_FALSE;
    startNow = request->pending && !request->started;

    if (startNow) {
        request->started = JNI_TRUE;
    }
    debugMonitorExit(invokerLock);

    if (!startNow) {
        return JNI_FALSE;
    }

    env = getEnv(); 

    WITH_LOCAL_REFS(env, 2);   /* 1 for obj return values, 1 for exception */

    (*env)->ExceptionClear(env);

    switch (request->invokeType) {
        case INVOKE_CONSTRUCTOR:
            invokeConstructor(env, request);
            break;
        case INVOKE_STATIC:
            invokeStatic(env, request);
            break;
        case INVOKE_INSTANCE:
            if (request->options & JDWP_InvokeOptions_INVOKE_NONVIRTUAL) {
                invokeNonvirtual(env, request);
            } else {
                invokeVirtual(env, request);
            }
            break;
        default:
            JDI_ASSERT(JNI_FALSE);
    }
    request->exception = (*env)->ExceptionOccurred(env);
    if (request->exception) {
        request->exception = (*env)->NewGlobalRef(env, request->exception);
        if (request->exception == NULL) {
            ERROR_MESSAGE_EXIT("Unable to create global reference");
        }
        (*env)->ExceptionClear(env);
    }

    END_WITH_LOCAL_REFS(env);
    return JNI_TRUE;
}

void
invoker_completeInvokeRequest(jthread thread) 
{
    JNIEnv *env = getEnv();
    PacketOutputStream out;
    jbyte tag = 0;
    jobject exc = NULL;
    jvalue returnValue;
    jint id = 0;
    InvokeRequest *request;
    jboolean detached;
     
    JDI_ASSERT(thread);

    eventHandler_lock(); /* for proper lock order */
    debugMonitorEnter(invokerLock);

    request = threadControl_getInvokeRequest(thread);
    if (request == NULL) {
        ERROR_CODE_EXIT(JVMDI_ERROR_INVALID_THREAD);
    }

    JDI_ASSERT(request->pending);
    JDI_ASSERT(request->started);

    request->pending = JNI_FALSE;
    request->started = JNI_FALSE;
    request->available = JNI_TRUE; /* For next time around */

    detached = request->detached;
    if (!detached) {
        if (request->options & JDWP_InvokeOptions_INVOKE_SINGLE_THREADED) {
            threadControl_suspendThread(thread, JNI_FALSE);
        } else {
            threadControl_suspendAll();
        }

        if (request->invokeType == INVOKE_CONSTRUCTOR) {
            /*
             * Although constructors technically have a return type of 
             * void, we return the object created.
             */
            tag = specificTypeKey(request->returnValue.l);
        } else {
            tag = returnTypeTag(request->methodSignature);
        }
        id = request->id;
        exc = request->exception;
        returnValue = request->returnValue;
    }

    /*
     * Give up the lock before I/O operation
     */
    debugMonitorExit(invokerLock);
    eventHandler_unlock();


    if (!detached) {
        outStream_initReply(&out, id);
        outStream_writeValue(env, &out, tag, returnValue);
        outStream_writeObjectTag(&out, exc);
        WRITE_GLOBAL_REF(env, &out, exc);
        outStream_sendReply(&out);
    }
}

jboolean 
invoker_isPending(jthread thread)
{
    InvokeRequest *request;

    JDI_ASSERT(thread);
    request = threadControl_getInvokeRequest(thread);
    if (request == NULL) {
        ERROR_CODE_EXIT(JVMDI_ERROR_INVALID_THREAD);
    }
    return request->pending;
}

void 
invoker_detach(InvokeRequest *request)
{
    JDI_ASSERT(request);
    debugMonitorEnter(invokerLock);
    request->detached = JNI_TRUE;
    debugMonitorExit(invokerLock);
}

