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

static unsigned char *localBuffer;
static size_t localBufferCapacity;
 
/* 4 bytes for C thread ID and 4 bytes for message length field */ 
#define FIELDS_LENGTH 8 

typedef struct _BUFFER_POOL {
    jclass classHandle;  
    jmethodID midGetBuffer; 
    jmethodID midFreeBuffer;
} BUFFER_POOL;

static MUTEX_HANDLE bridgeMutex;
static BUFFER_POOL * bufferPool;

static void finalizeLimeBridge();

static int
prepareLocalBuffer(size_t minCapacity) {
    unsigned char *newLocalBuffer;
    
    if (minCapacity <= localBufferCapacity) {
        return 0;
    }

    newLocalBuffer = (unsigned char *) realloc(localBuffer, minCapacity);
    if (newLocalBuffer == NULL) {
        return -1;
    }
    
    localBuffer = newLocalBuffer;
    localBufferCapacity = minCapacity;
    
    return 0;
}

static void
sublimeBridgeErrorImpl(const char * format, va_list argptr) {
    fprintf(stderr, "SUBLIMEBRIDGE: ");

    vfprintf(stderr, format, argptr);
    fflush(stderr);
}

static void
sublimeBridgeError(const char * format, ...) {
    va_list args;

    va_start(args, format);
    sublimeBridgeErrorImpl(format, args);
    va_end(args);
}

static void
sublimeBridgeFatal(const char * format, ...) {
    va_list args;

    va_start(args, format);
    sublimeBridgeErrorImpl(format, args);
    va_end(args);

    finalizeLimeBridge();
    exit(1);
}

static int
copyFromSharedBufferToJavaArray(JNIEnv *env, jbyteArray jarray,
                                SharedBuffer *sharedBuffer,
                                uint32_t threadID, 
                                uint32_t payloadSize) {
    if (prepareLocalBuffer(payloadSize + FIELDS_LENGTH) != 0) {
        sublimeBridgeError("Could not allocate memory\n"); 
        return -1;
    }
    
    *((uint32_t *) localBuffer) = htonl(threadID);
    *((uint32_t *) localBuffer + 1) = htonl(payloadSize);

    if (sharedBuffer->readAll(sharedBuffer, 
                              localBuffer + FIELDS_LENGTH, 
                              payloadSize) != 0) {
        return -2;
    }

    (*env)->SetByteArrayRegion(env, jarray, 0, 
                               payloadSize + FIELDS_LENGTH,
                               localBuffer);

    return 0;
}

static int
copyFromJavaArrayToSharedBuffer(JNIEnv *env, jbyteArray jarray,
                                SharedBuffer *sharedBuffer) {
    uint32_t threadID;
    uint32_t payloadSize;
    int fullSize = (*env)->GetArrayLength(env, jarray);

    if (prepareLocalBuffer(fullSize) != 0) {
        sublimeBridgeError("Could not allocate memory\n");
        return -1;
    }

    /* Result buffer format: 
     * 4 bytes  - C thread ID
     * 4 bytes  - size of message(not including the first 8 bytes) 
     * the rest - the result data
     */
    (*env)->GetByteArrayRegion(env, jarray, 0, fullSize, localBuffer); 

    threadID = ntohl(*((uint32_t *) localBuffer));
    payloadSize = ntohl(*((uint32_t *) localBuffer + 1));
    
    if (sharedBuffer->writeInt32(sharedBuffer, threadID) != 0) {
        return -2;
    }
    
    if (sharedBuffer->writeInt32(sharedBuffer, payloadSize) != 0) {
        return -2;
    }
    
    if (sharedBuffer->writeAll(sharedBuffer, 
                               (char*) localBuffer + FIELDS_LENGTH,
                               payloadSize) != 0) {
        return -2;
    }
    
    return 0;
}

/* return a pointer to a new alloced string: name+process ID */ 
static char * concatNameProcID(const char *name, const char *procid){
    char *tmp = (char *) malloc(strlen(name) + strlen(procid) + 1);
    if (tmp == NULL) {
        return NULL;
    }

    tmp = strcpy(tmp, name); 
    tmp = strcat(tmp, procid); 
    return tmp; 
}

static SharedBuffer* CreateSharedBufferForProcID(const char *prefix, 
                                                 const char *procid) {
    SharedBuffer* sharedBuffer;
    char *fullBufferName = concatNameProcID(prefix, procid);
    if (fullBufferName == NULL) {
        return NULL;
    }
    
    sharedBuffer = CreateNewSharedBuffer(fullBufferName);
    free(fullBufferName);
    
    return sharedBuffer;
}

static int initializeLimeBridge() {
    int32_t pid;
    char *pidstr;
    
    pidstr = (char*) malloc(12 * sizeof(char));
    if (pidstr == NULL) {
        sublimeBridgeError("Could not allocate memory\n"); 
        return -1;
    } 
    
    pid = get_current_process_id();
    itoa(pid, pidstr, 10);

    callSharedBuffer = 
            CreateSharedBufferForProcID(SUBLIME_SHARED_BUFFER_NAME0, pidstr);
    returnSharedBuffer = 
            CreateSharedBufferForProcID(SUBLIME_SHARED_BUFFER_NAME1, pidstr);

    free(pidstr);

    if ((callSharedBuffer == NULL) || (returnSharedBuffer == NULL)) {
        sublimeBridgeError("Could not open shared buffers\n");
        return -1;
    }
    
    return 0;
}

static void finalizeLimeBridge() {
    if (callSharedBuffer != NULL) {
        DeleteSharedBuffer(callSharedBuffer);
        callSharedBuffer = NULL;
    }

    if (returnSharedBuffer != NULL) {
        DeleteSharedBuffer(returnSharedBuffer);
        returnSharedBuffer = NULL;
    }
    
    if (localBuffer != NULL) {
        free(localBuffer);
        localBuffer = NULL;
        localBufferCapacity = 0;
    }
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
    jbyteArray jarray; 
    jmethodID mid_call; 

    mid_call = (*env)->GetStaticMethodID(env, classSublime, 
                                         "processRequest", "([B)V");  
    if (mid_call == NULL) {
        BufferPool_destroy(env, bufferPool);
        LimeDestroyMutex(bridgeMutex);
        sublimeBridgeFatal("Could not find method Sublime.processRequest()\n"); 
    }  

    while (1) {
        uint32_t requestThreadID, requestSize; 
        
        if (callSharedBuffer->readInt32(callSharedBuffer, &requestThreadID)
                != 0) {
            break;
        }

        if (callSharedBuffer->readInt32(callSharedBuffer, &requestSize)
                != 0) {
            break;
        }

        WaitForMutex(bridgeMutex);

        /* 8 = place for C thread ID and request size  */  
        jarray = BufferPool_getBuffer(env, bufferPool, 
                                      requestSize + FIELDS_LENGTH);

        if (copyFromSharedBufferToJavaArray(env, jarray,
                                            callSharedBuffer,
                                            requestThreadID, 
                                            requestSize) != 0) {
            LimeReleaseMutex(bridgeMutex);
            break;
        }

        LimeReleaseMutex(bridgeMutex);

        (*env)->CallStaticVoidMethod(env, classSublime, mid_call, jarray);    
        (*env)->DeleteLocalRef(env, jarray);
    }
}

JNIEXPORT void JNICALL Java_com_sun_kvem_Sublime_returnResult(
        JNIEnv *env, jclass clz, jbyteArray jarray) {
    WaitForMutex(bridgeMutex);
    
    if (returnSharedBuffer == NULL) {
        /* already closed and destroyed */
        LimeReleaseMutex(bridgeMutex);
        
        /* fixme: destroy mutex here? */
        return;
    } 

    if (copyFromJavaArrayToSharedBuffer(env, jarray, returnSharedBuffer) == 0) {
        returnSharedBuffer->flush(returnSharedBuffer);
    }

    BufferPool_freeBuffer(env, bufferPool, jarray);

    LimeReleaseMutex(bridgeMutex);

    /*
     * for better performance: release CPU to allow other threads to lock 
     * returnSharedBuffer 
     */ 
    yield();
}

/* used by Sublime.java to create a unique named shared buffer */ 
JNIEXPORT void JNICALL Java_com_sun_kvem_Sublime_setSublimeProcessId(
        JNIEnv * e, jclass c, jint pid) {
    set_current_process_id(pid);
}

/* used by Sublime.java to create a unique named shared buffer */ 
JNIEXPORT jint JNICALL Java_com_sun_kvem_Sublime_getSublimeProcessId(
        JNIEnv * e, jclass c) {
    return get_current_process_id();
}
 
JNIEXPORT void JNICALL Java_com_sun_kvem_Sublime_process(
        JNIEnv *env, jclass clz) {
    jmethodID mid_release;

    bridgeMutex = LimeCreateMutex(NULL, FALSE, NULL);
    if (bridgeMutex == NULL) {
        sublimeBridgeFatal("Could not create synchronization mutex\n");
    }
    
    if (initializeLimeBridge() != 0) {
        LimeDestroyMutex(bridgeMutex);
        sublimeBridgeFatal("Failed to initialize lime bridge\n");
    }

    bufferPool = BufferPool_create(env);

    mid_release = (*env)->GetStaticMethodID(env, clz, "releaseInitLock", "()V"); 
    if (mid_release == NULL) { 
        BufferPool_destroy(env, bufferPool);
        LimeDestroyMutex(bridgeMutex);
        sublimeBridgeFatal("Could not find method Sublime.releaseInitLock()\n");
    }  
    (*env)->CallStaticVoidMethod(env, clz, mid_release); 

    /* enter the process requests loop */  
    process(env, clz); 

    WaitForMutex(bridgeMutex);
    
    BufferPool_destroy(env, bufferPool);
    finalizeLimeBridge();
    
    LimeReleaseMutex(bridgeMutex);

    /* fixme: destroy mutex here? */
}

JNIEXPORT void JNICALL Java_com_sun_kvem_Sublime_stopProcess(JNIEnv *env, 
                                                             jclass clz) {
    WaitForMutex(bridgeMutex);

    finalizeLimeBridge();    

    LimeReleaseMutex(bridgeMutex);    
}

