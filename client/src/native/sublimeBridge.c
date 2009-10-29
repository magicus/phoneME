/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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


#include "lime.h"
#include "sublimeio.h"
#include "sublime.h"
#include "jni.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include "com_sun_kvem_Sublime.h"

static SharedBuffer *callSharedBuffer, *returnSharedBuffer; 
static char *buffer; 
static char *processID; 
static int32_t procID; 
static char **names; 
static jbyte *buf;

/* 4 bytes for C thread ID and 4 bytes for message length field */ 
#define FIELDS_LENGTH 8 

/* 
 * events: 
 * 0 - method call received on first shared buffer (callSharedBuffer)
 * 1 - finished reading call from first shared buffer 
 * 2 - finished writing result to second shared buffer (returnSharedBuffer)
 * 3 - finished reading result from second shared buffer 
 */

static EVENT_HANDLE event0; 
static EVENT_HANDLE event1; 
static EVENT_HANDLE event2; 
static EVENT_HANDLE event3; 

typedef struct _BUFFER_POOL {
    jclass classHandle;  
    jmethodID midGetBuffer; 
    jmethodID midFreeBuffer;
} BUFFER_POOL;

static int stopProcess = 0;
static BUFFER_POOL * bufferPool;

static void
sublimeBridgeFatal(const char * format, ...) {
    va_list args;

    fprintf(stderr, " SUBLIMEBRIDGE: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fflush(stderr);
    exit(1);
}

static int CreateSharedBuffer(void) {
    if ((callSharedBuffer = CreateNewSharedBuffer(names[0])) == NULL) {
        fprintf(stderr, "Could not create SharedBuffer0 object (%d) \n", GetLastError());
        fflush(stderr); 
        return -1; 
    }
    if ((returnSharedBuffer = CreateNewSharedBuffer(names[1])) == NULL) {
        fprintf(stderr, "Could not create SharedBuffer1 object (%d)\n", GetLastError());
        fflush(stderr); 
        return -1; 
    }
    return 0;
}

static BUFFER_POOL * 
BufferPool_create(JNIEnv * env) {
    BUFFER_POOL * bufferPool;
    jclass localRef;

    localRef = (*env)->FindClass(env, "com/sun/kvem/sublime/BufferPool");
    if (localRef == NULL) {
        sublimeBridgeFatal("Could not find class BufferPool\n");  
    } 

    bufferPool = (BUFFER_POOL *) malloc(sizeof(BUFFER_POOL));
    if (bufferPool == NULL) {
        sublimeBridgeFatal("Failed to allocate BUFFER_POOL\n");  
    }

    bufferPool->midGetBuffer = 
            (*env)->GetStaticMethodID(env, localRef, "getBuffer", "(I)[B");  
    if (bufferPool->midGetBuffer == NULL) { 
        free(bufferPool);
        sublimeBridgeFatal("Could not find method BufferPool.getBuffer()\n");  
    }

    bufferPool->midFreeBuffer = 
            (*env)->GetStaticMethodID(env, localRef, "freeBuffer", "([B)V");  
    if (bufferPool->midFreeBuffer == NULL) { 
        free(bufferPool);
        sublimeBridgeFatal("Could not find method BufferPool.freeBuffer()\n");  
    }

    bufferPool->classHandle = (*env)->NewGlobalRef(env, localRef);
    if (bufferPool->classHandle == NULL) {
        free(bufferPool);
        sublimeBridgeFatal("Could not create global ref for BufferPool\n");  
    }

    return bufferPool;
}

static void 
BufferPool_destroy(JNIEnv * env, BUFFER_POOL * bufferPool) {
    if (bufferPool != NULL) {
        (*env)->DeleteGlobalRef(env, bufferPool->classHandle);
        free(bufferPool);
    }
}

static jbyteArray 
BufferPool_getBuffer(JNIEnv * env, BUFFER_POOL * bufferPool, jint bufferSize) {
    return (*env)->CallStaticObjectMethod(env, 
                                          bufferPool->classHandle, 
                                          bufferPool->midGetBuffer,
                                          bufferSize);  
}

static void 
BufferPool_freeBuffer(JNIEnv * env, BUFFER_POOL * bufferPool, jbyteArray jarr) {
    (*env)->CallStaticVoidMethod(env, 
                                 bufferPool->classHandle, 
                                 bufferPool->midFreeBuffer, 
                                 jarr);
}

/*
 * waits for methods calls, and transfer them to J2SE implementation 
 */
static void process(JNIEnv *env, jclass classSublime) {
    jint messageSize;
    jbyteArray jbuffer = NULL; 
    jmethodID mid_call; 

    mid_call = (*env)->GetStaticMethodID(env, classSublime, 
                                         "processRequest", "([B)V");  
    if (mid_call == NULL) {
        sublimeBridgeFatal("Could not find method Sublime.processRequest()\n"); 
    }  

    while (1) {
        /* wait for a call to be placed in callSharedBuffer */ 
        WaitForEvent(event0);
        if (stopProcess) {
            break;
        }
        messageSize = callSharedBuffer->getMsgLength(callSharedBuffer); 
        /* 8 = place for C thread ID and request size  */  
        jbuffer = BufferPool_getBuffer(env, bufferPool, 
                                       messageSize + FIELDS_LENGTH);  

        /* copy the buffer w/o the buffer size field (4 bytes) */
        (*env)->SetByteArrayRegion(env, jbuffer, 0, messageSize + FIELDS_LENGTH, 
                                   (jbyte *)&callSharedBuffer->data->dataBuffer->threadID);
        callSharedBuffer->reset(callSharedBuffer); 
        (*env)->CallStaticVoidMethod(env, classSublime, mid_call, jbuffer);    
        (*env)->DeleteLocalRef(env, jbuffer); 
        LimeSetEvent(event1); 
    }
}

JNIEXPORT void JNICALL Java_com_sun_kvem_Sublime_returnResult(JNIEnv *e, jclass clz, jbyteArray jarr) {
    int size = (*e)->GetArrayLength(e, jarr);
    int sharedBufferSize;
    uint32_t res_thread_id, resultSize; 

    if (size > BUFFER_SIZE) {
        sublimeBridgeFatal("Shared buffer size (%d) is too small"
                               " for this call (%d)\n", BUFFER_SIZE, size); 
    }
    if (buf == NULL) { 
        sublimeBridgeFatal("malloc failed\n");  
    } 
    if (returnSharedBuffer == NULL) {
        sublimeBridgeFatal("Could not open shared buffer\n");  
    } 
    if (event0 == NULL || event1 == NULL || event2 == NULL || event3 == NULL) {
        if (initEvents() == -1) {
            sublimeBridgeFatal("Could not initialize events\n");  
        } 
    }

    returnSharedBuffer->lock(returnSharedBuffer); 
    sharedBufferSize = returnSharedBuffer->getBufferSize(returnSharedBuffer); 
    /* Result buffer format: 
     * 4 bytes  - C thread ID
     * 4 bytes  - size of message(not including the first 8 bytes) 
     * the rest - the result data
     */
    (*e)->GetByteArrayRegion(e, jarr,0,size,buf); 
    resultSize = ntohl(*((uint32_t *)buf + 1));
    res_thread_id = ntohl(*((uint32_t *) buf));
    returnSharedBuffer->write(returnSharedBuffer, (char*)buf+FIELDS_LENGTH, 
                              resultSize);
    returnSharedBuffer->setID(returnSharedBuffer, res_thread_id); 
    BufferPool_freeBuffer(e, bufferPool, jarr);
    /* notify the CVM process that a result has been sent, and wait for ack */ 
    LimeSetEvent(event2);
    WaitForEvent(event3); 
    returnSharedBuffer->reset(returnSharedBuffer); 

    returnSharedBuffer->unlock(returnSharedBuffer);  
    /* for better performance: release CPU to allow other threads to lock returnSharedBuffer */ 
    yield();
}


int initEvents() {
    /* create named events to communicate between processes */
    event0 = LimeCreateEvent(NULL, FALSE, FALSE, names[2]);
    event1 = LimeCreateEvent(NULL, FALSE, FALSE, names[3]);
    event2 = LimeCreateEvent(NULL, FALSE, FALSE, names[4]);
    event3 = LimeCreateEvent(NULL, FALSE, FALSE, names[5]);
    if (event0 == NULL || event1 == NULL || event2 == NULL || event3 == NULL) {
        return -1; 
    }
    return 0; 
}


/* used by Sublime.java to create a unique named shared buffer */ 
JNIEXPORT void JNICALL Java_com_sun_kvem_Sublime_setSublimeProcessId(JNIEnv * e, jclass c, jint pid) {
#ifndef WIN32
    processID = (char*)malloc(sizeof(char)*sizeof(int32_t));
    itoa(pid, processID, 10);
    setenv("SUBLIME_PROC_ID", processID, 0);
    free(processID);
#endif
    set_current_process_id(pid);
}

/* used by Sublime.java to create a unique named shared buffer */ 
JNIEXPORT jint JNICALL Java_com_sun_kvem_Sublime_getSublimeProcessId(JNIEnv * e, jclass c) {
    return get_current_process_id();
}

 
JNIEXPORT void JNICALL Java_com_sun_kvem_Sublime_process(JNIEnv *env, jclass clz) {
    jmethodID mid_release;
    
    procID =  get_current_process_id();
    processID = (char*)malloc(sizeof(char)*sizeof(int32_t));
    itoa(procID, processID, 10);
    /* names for 2 shared buffers and 4 events */  
    names = (char **)malloc(sizeof(char*)*6);
    names[0] = (char *)malloc(sizeof(char)*(strlen(SUBLIME_SHARED_BUFFER_NAME0) + sizeof(int32_t)) + 1); 
    strcpy(names[0], SUBLIME_SHARED_BUFFER_NAME0);
    strcat(names[0], processID);
    names[1] = (char *)malloc(sizeof(char)*(strlen(SUBLIME_SHARED_BUFFER_NAME1) + sizeof(int32_t)) + 1); 
    strcpy(names[1], SUBLIME_SHARED_BUFFER_NAME1);
    strcat(names[1], processID);
    names[2] = (char *)malloc(sizeof(char)*(strlen(EVENT0) + sizeof(int32_t)) + 1); 
    strcpy(names[2], EVENT0);
    strcat(names[2], processID);
    names[3] = (char *)malloc(sizeof(char)*(strlen(EVENT1) + sizeof(int32_t)) + 1); 
    strcpy(names[3], EVENT1);
    strcat(names[3], processID);
    names[4] = (char *)malloc(sizeof(char)*(strlen(EVENT2) + sizeof(int32_t)) + 1); 
    strcpy(names[4], EVENT2);
    strcat(names[4], processID);
    names[5] = (char *)malloc(sizeof(char)*(strlen(EVENT3) + sizeof(int32_t)) + 1); 
    strcpy(names[5], EVENT3);
    strcat(names[5], processID);

    if (CreateSharedBuffer() != 0) { 
        sublimeBridgeFatal("Could not open shared buffer\n");  
    } 
    free(names[0]);
    free(names[1]);

    if (initEvents() == -1) {
        sublimeBridgeFatal("Could not initialize events\n");  
    }
    mid_release = (*env)->GetStaticMethodID(env, clz, "releaseInitLock", "()V"); 
    if (mid_release == NULL) { 
        sublimeBridgeFatal("Could not find method Sublime.releaseInitLock()\n");
    }  
    buf = (jbyte*)malloc(sizeof(jbyte) * BUFFER_SIZE);  
    (*env)->CallStaticVoidMethod(env, clz, mid_release); 

    bufferPool = BufferPool_create(env);

    /* enter the process requests loop */  
    process(env, clz); 

    BufferPool_destroy(env, bufferPool);

    DeleteSharedBuffer(callSharedBuffer);
    DeleteSharedBuffer(returnSharedBuffer);
}

JNIEXPORT void JNICALL Java_com_sun_kvem_Sublime_stopProcess(JNIEnv *env, 
                                                             jclass clz) {
    stopProcess = 1;
    LimeSetEvent(event0);
}

