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


#ifdef WIN32 

#include <process.h>
#include <winsock2.h>

#else /* !WIN32 */ 

#include <errno.h> 
#include <stdarg.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#endif /* WIN32 */ 

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include "sublimeio.h"

static int debug = 0; 

static int sharedBuffer_createIpcObjects(SharedBuffer *sb, char *bufferName);
static void sharedBuffer_destroyIpcObjects(SharedBuffer *sb);

static void dataBuffer_write(DataBuffer *db, const void *data, int32_t length);
static void dataBuffer_read(DataBuffer *db, void *data, int32_t length);

#ifdef WIN32 

static void error(char * errorMsg){
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
    fprintf(stderr,"%s (%d)(%s)\n",errorMsg, dw, lpMsgBuf);
}

#else /* !WIN32 */

static void error(char * errorMsg){
    fprintf(stderr,"%s (%d)\n",errorMsg, errno);
}

#endif /* WIN32 */


static void sharedBuffer_initPointers(SharedBuffer *sb) ;

static char *constructIpcObjectName(const char *bufferName, 
                                    const char *suffix){
    int nameLength = strlen(bufferName); 
    char* objectName = (char*) malloc(nameLength + strlen(suffix) + 1);
    
    if (objectName == NULL) {
        return NULL;
    }
    
    strcpy(objectName, bufferName);
    strcpy(objectName + nameLength, suffix);

    return objectName; 
}

static MUTEX_HANDLE constructIpcMutex(const char *bufferName,
                                      const char *suffix) {
    char *mutexName = constructIpcObjectName(bufferName, suffix);
    MUTEX_HANDLE mutexHandle;
    
    if (mutexName == NULL) {
        return NULL;
    }
    
    mutexHandle = LimeCreateMutex(NULL, FALSE, mutexName);
    free(mutexName);
    
    return mutexHandle;
}

static EVENT_HANDLE constructIpcEvent(const char *bufferName,
                                      const char *suffix) {
    char *eventName = constructIpcObjectName(bufferName, suffix);
    EVENT_HANDLE eventHandle;
    
    if (eventName == NULL) {
        return NULL;
    }
    
    eventHandle = LimeCreateEvent(NULL, FALSE, FALSE, eventName);
    free(eventName);
    
    return eventHandle;
}

/*
 * size - the desired buffer size. 
 * maps the actual shared buffer to a *char. 
 * returns 0 on success, errcode otherwise
 * this method is used by open() (and will be used by future feature: resize() )
 */
static int sharedBuffer_attachBuffer(SharedBuffer* sb, int size ){
    assert(sb != NULL);

#ifdef WIN32 
    sb->data.dataBuffer = 
        (DataBuffer*) MapViewOfFile(sb->data.hMapFile,   
                                    FILE_MAP_ALL_ACCESS, //read/write permission
                                    0, 0, 
                                    size + SUBLIME_FIELDS_LENGTH); 

    if (sb->data.dataBuffer == NULL) { 
        error("SUBLIMEIO: Could not map view of file ");
        exit(1);
    }

#else /* !WIN32 */
  
    sb->data.dataBuffer = (DataBuffer*)shmat(*(sb->data.hMapFile), NULL, 0);
    if ((int)sb->data.dataBuffer == -1) { 
        switch(errno) { 
        case EACCES: {
            fprintf(stderr, "Operation permission is denied to the calling process, see IPC\n");
            break; 
        }
        case EINVAL: {
            fprintf(stderr, "The value of shmid is not a valid shared memory identifier; the shmaddr is not a null pointer and the value of\n");
            break; 
        }
        case EMFILE: {
            fprintf(stderr, "The number of shared memory segments attached to the calling process would exceed the system-imposed limit.\n");
            break; 
        }
        case ENOMEM: {
            fprintf(stderr, "The available data space is not large enough to accommodate the shared memory segment.\n");
            break; 
        }
        }
    
    
        fprintf(stderr, "sb->data.dataBuffer %d\n", sb->data.dataBuffer);
        error("SUBLIMEIO: Could not map view of file  ");
        exit(1);
    }
  
#endif /* WIN32 */
  
  
    if (debug){
        fprintf(stderr, "SUBLIMEIO: new shared buffer size is %d\n",size); 
        fflush(stderr);
    }
    return 0;
}

/* 
 * maps the shared buffer to a *char and init the function pointers 
 * of the sharedBuffer struct 
 */ 
static int sharedBuffer_open(SharedBuffer* sb, int size){
    int res = sharedBuffer_attachBuffer(sb, size); 
    if (res == 0) {
        sharedBuffer_initPointers(sb); 
    }

    assert(sb->data.dataBuffer != NULL);
    return res; 
}

/* create a new file mapping and open the shared buffer */
static int sharedBuffer_create(SharedBuffer* sb, char* bufferName){
    int size = SUBLIME_USABLE_BUFFER_SIZE; 
    int res; 

#ifndef WIN32 
    key_t kt; 
    int shmid, key_id; 
    struct shmid_ds shmds; 
    static int pid_ext = 0; 
  
#endif
    assert(sb != NULL);
    assert(bufferName != NULL);
    
    if (sharedBuffer_createIpcObjects(sb, bufferName) != 0) {
        error("SUBLIMEIO: Could not create shared buffer IPC objects");
        exit(1);
    }
     
#ifdef WIN32    
    
    sb->data.hMapFile = 
        CreateFileMapping(INVALID_HANDLE_VALUE,    // use paging file
                          NULL,                    // default security 
                          PAGE_READWRITE,          // read/write access
                          0,                       // max. object size 
                          size + SUBLIME_FIELDS_LENGTH,     
                                                   // buffer size  
                          bufferName);             // name of mapping object
    if ((sb->data.hMapFile == NULL)||
        (sb->data.hMapFile == INVALID_HANDLE_VALUE)) {
        error("SUBLIMEIO: Could not create file mapping object ");
        exit(1);
    }
#else /* !WIN32 */ 

    if (getenv("SUBLIME_PROC_ID") == NULL){
        key_id = getpid() + pid_ext++; 
    }
    else {
        key_id = atoi(getenv("SUBLIME_PROC_ID")) + pid_ext++;
    }
    
    kt = ftok(getTempDirLocation(),key_id); 

    /* malloc for file descriptor */ 
    sb->data.hMapFile = (int *)malloc(sizeof(int)); 
    if ( sb->data.hMapFile == NULL){
        error("SUBLIMEIO: malloc failed"); 
    } 

    /* 511 - all permissions */ 
    if ((shmid = shmget(kt, size + SUBLIME_FIELDS_LENGTH, IPC_CREAT | 511 )) 
            == -1){
        error("CreateNewSharedBuffer: shmget failed");
    } 

    //fprintf(stderr,"shared buffer number is %d  and key is %d\n",shmid, key_id); 
    //fflush(stderr); 
    
    if ((shmctl(shmid, IPC_STAT, &shmds)) == -1){
        error("CreateNewSharedBuffer: shmctl failed");
    } 

    shmds.shm_perm.mode =  shmds.shm_perm.mode | O_RDWR  ;
    *(sb->data.hMapFile) = shmid;

#endif /* WIN32 */ 
    
    if ((res = sharedBuffer_open(sb, size)) != 0){
        return res;
    }

    sb->data.dataBuffer->capacity = htonl(size); 
    sb->data.dataBuffer->closed = 0;
    sb->data.dataBuffer->readIndex = 0;
    sb->data.dataBuffer->byteCount = 0;

    return res; 
    
}  

/* the shared buffer is really closed only after all ponters 
 * to the buffer are closed 
 */ 
static int sharedBuffer_close(SharedBuffer* sb){
    assert(sb != NULL); 

    /* TODO: flush */ 
    /* sharedBuffer_flush(sb); */

    WaitForMutex(sb->data.mutex);
    sb->data.dataBuffer->closed = htonl(1);
    LimeReleaseMutex(sb->data.mutex);
    
    LimeSetEvent(sb->data.bufferReadyEvent);
    LimeSetEvent(sb->data.dataReadyEvent);

#ifdef WIN32
 
    UnmapViewOfFile((char*)sb->data.dataBuffer);
    CloseHandle(sb->data.hMapFile); 

#else /* !WIN32 */

    shmdt(sb->data.dataBuffer); 

    /* 
     * if we haven't created the shared memory, the following call will fail,
     * but because it is called by both sides, it will get eventually destroyed
     */
    shmctl(*(sb->data.hMapFile), IPC_RMID, NULL);
    
    close(*(sb->data.hMapFile));

#endif /* WIN32 */ 

    sharedBuffer_destroyIpcObjects(sb);
    free(sb); 
    
    return 0; 
}

static int sharedBuffer_write(SharedBuffer *sb, 
                              const void *buffer, int32_t numOfBytes) {
    int32_t bufCapacity;
    int32_t bufClosed;
    int32_t bufByteCount;
    int32_t bufAvailableByteCount;

    assert(sb != NULL);
    assert(buffer != NULL);
    assert(numOfBytes >= 0);

    if (numOfBytes == 0) {
        return 0;
    }

    WaitForMutex(sb->data.mutex);
        
    bufCapacity = ntohl(sb->data.dataBuffer->capacity);
    bufClosed = ntohl(sb->data.dataBuffer->closed);
    bufByteCount = ntohl(sb->data.dataBuffer->byteCount);
    
    while ((bufByteCount == bufCapacity) && !bufClosed) {
        LimeReleaseMutex(sb->data.mutex);
        WaitForEvent(sb->data.bufferReadyEvent);
        
        WaitForMutex(sb->data.mutex);
        bufClosed = ntohl(sb->data.dataBuffer->closed);
        bufByteCount = ntohl(sb->data.dataBuffer->byteCount);
    }
    
    if (bufClosed) {
        LimeReleaseMutex(sb->data.mutex);
        return -1;
    }
    
    bufAvailableByteCount = bufCapacity - bufByteCount;
    if (numOfBytes > bufAvailableByteCount) {
        numOfBytes = bufAvailableByteCount;
    }
    
    dataBuffer_write(sb->data.dataBuffer, buffer, numOfBytes);

    if (numOfBytes == bufAvailableByteCount) {
        /* the buffer is full, we can't delay the notification */
        LimeSetEvent(sb->data.dataReadyEvent);
    }
    
    LimeReleaseMutex(sb->data.mutex);

    return numOfBytes;
}

static void sharedBuffer_flush(SharedBuffer *sb) {
    int32_t bufByteCount;

    WaitForMutex(sb->data.mutex);

    bufByteCount = ntohl(sb->data.dataBuffer->byteCount);
    if (bufByteCount > 0) {
        LimeSetEvent(sb->data.dataReadyEvent);
    }
    
    LimeReleaseMutex(sb->data.mutex);
}

static int sharedBuffer_read(SharedBuffer *sb, 
                             void *buffer, int32_t numOfBytes) {
    int32_t bufCapacity;
    int32_t bufClosed;
    int32_t bufByteCount;

    assert(sb != NULL);
    assert(buffer != NULL);
    assert(numOfBytes >= 0);

    if (numOfBytes == 0) {
        return 0;
    }

    WaitForMutex(sb->data.mutex);
        
    bufCapacity = ntohl(sb->data.dataBuffer->capacity);
    bufClosed = ntohl(sb->data.dataBuffer->closed);
    bufByteCount = ntohl(sb->data.dataBuffer->byteCount);

    while ((bufByteCount == 0) && !bufClosed) {
        LimeReleaseMutex(sb->data.mutex);
        WaitForEvent(sb->data.dataReadyEvent);
        
        WaitForMutex(sb->data.mutex);
        bufClosed = ntohl(sb->data.dataBuffer->closed);
        bufByteCount = ntohl(sb->data.dataBuffer->byteCount);
    }

    if (bufByteCount == 0) {
        /* buffer is closed */
        LimeReleaseMutex(sb->data.mutex);
        return -1;
    }

    if (numOfBytes > bufByteCount) {
        numOfBytes = bufByteCount;
    }

    dataBuffer_read(sb->data.dataBuffer, buffer, numOfBytes);
    
    if (bufByteCount == bufCapacity) {
        /* the whole buffer was full, send notification */
        LimeSetEvent(sb->data.bufferReadyEvent);
    }

    LimeReleaseMutex(sb->data.mutex);

    return numOfBytes;
}

void DeleteSharedBuffer(SharedBuffer *sb){
    sharedBuffer_close(sb); 
}

static int sharedBuffer_readAll(SharedBuffer *sb, 
                                void *buffer, int length) {
    char *ptr = (char *) buffer;
    while (length > 0) {
        int readBytes = sb->read(sb, ptr, length);
        if (readBytes < 0) {
            return readBytes;
        }

        length -= readBytes;
        ptr += readBytes;
    }
    
    return 0;
}

static int sharedBuffer_writeAll(SharedBuffer *sb, 
                                 const void *buffer, int length) {
    const char *ptr = (const char *) buffer;
    while (length > 0) {
        int writtenBytes = sb->write(sb, ptr, length);
        if (writtenBytes < 0) {
            return writtenBytes;
        }

        length -= writtenBytes;
        ptr += writtenBytes;
    }
    
    return 0;
}

static int readIntRaw(SharedBuffer *sb, uint32_t *result) {
    assert(sb != NULL);
    return sharedBuffer_readAll(sb, result, sizeof(uint32_t));
}

/** Extra optimization here, since it is the most frequently called
 * socket function */
static int sharedBuffer_writeInt32(SharedBuffer *sb, int32_t value) {
    assert(sb != NULL);    
    value = htonl(value); 
    return sharedBuffer_writeAll(sb, &value, sizeof(int32_t));
}

/* converts the result from network byteorder to host byteorder */ 

static int sharedBuffer_readInt32(SharedBuffer *sb, int32_t *result) {
    int32_t temp; 
    int retval;
    
    assert(sb != NULL);
    assert(result != NULL);
    
    retval = sharedBuffer_readAll(sb, &temp, sizeof(int32_t));
    if (retval < 0) {
        return retval;
    }
    
    *result = ntohl(temp);
    return 0;
}

static int sharedBuffer_readLong64(SharedBuffer *sb, long64 *result) {
    uint32_t l = 12345678, h = 12345678;
    int retval;

    assert(sb != NULL);
    assert(result != NULL);
    
    retval = readIntRaw(sb, &h);
    if (retval < 0) {
        return retval;
    }
    
    retval = readIntRaw(sb, &l);
    if (retval < 0) {
        return retval;
    }
    
    (*result) = ((long64) ntohl(h) << 32) | ((long64) ntohl(l));
    return 0;
}

static void sharedBuffer_initPointers(SharedBuffer *sb) {
    sb->write = sharedBuffer_write;
    sb->writeAll = sharedBuffer_writeAll;
    sb->writeInt32 = sharedBuffer_writeInt32;
    sb->flush = sharedBuffer_flush;
    sb->read = sharedBuffer_read;
    sb->readAll = sharedBuffer_readAll;
    sb->readInt32 = sharedBuffer_readInt32;
    sb->readLong64 = sharedBuffer_readLong64;
}

static int sharedBuffer_createIpcObjects(SharedBuffer *sb, char *bufferName) {
    MUTEX_HANDLE mutex;
    EVENT_HANDLE bufferReadyEvent;
    EVENT_HANDLE dataReadyEvent;
    
    mutex = constructIpcMutex(bufferName, "_lock");
    if (mutex == NULL) {
        return -1;
    }
    
    bufferReadyEvent = constructIpcEvent(bufferName, "_br_event");
    if (bufferReadyEvent == NULL) {
        LimeDestroyMutex(mutex);
        return -1;
    }
    
    dataReadyEvent = constructIpcEvent(bufferName, "_dr_event");
    if (dataReadyEvent == NULL) {
        LimeDestroyEvent(bufferReadyEvent);
        LimeDestroyMutex(mutex);
        return -1;
    }
    
    sb->data.mutex = mutex;
    sb->data.bufferReadyEvent = bufferReadyEvent;
    sb->data.dataReadyEvent = dataReadyEvent;
    
    return 0;
}

static void sharedBuffer_destroyIpcObjects(SharedBuffer *sb) {
    LimeDestroyMutex(sb->data.mutex);
    LimeDestroyEvent(sb->data.bufferReadyEvent);
    LimeDestroyEvent(sb->data.dataReadyEvent);
}

static void dataBuffer_write(DataBuffer *db, const void *data, int32_t length) {
    const char *ptr = (const char *) data;

    int32_t bufCapacity = ntohl(db->capacity);
    int32_t bufReadIndex = ntohl(db->readIndex);
    int32_t bufByteCount = ntohl(db->byteCount);

    int32_t bufWriteIndex = (bufReadIndex + bufByteCount) % bufCapacity;
    int32_t bufRemainderLen = bufCapacity - bufWriteIndex;
    int32_t copiedChunkHeadLen = (length < bufRemainderLen) 
                                         ? length
                                         : bufRemainderLen;
    int copiedChunkTailLen = length - copiedChunkHeadLen;

    /* copy the head part */
    memcpy(db->data + bufWriteIndex, ptr, copiedChunkHeadLen);
    /* copy the tail part */
    memcpy(db->data, ptr + copiedChunkHeadLen, copiedChunkTailLen);
    
    /* update the data buffer state variables */
    db->byteCount = htonl(bufByteCount + length);
}

static void dataBuffer_read(DataBuffer *db, void *data, int32_t length) {
    char *ptr = (char *) data;

    int32_t bufCapacity = ntohl(db->capacity);
    int32_t bufReadIndex = ntohl(db->readIndex);
    int32_t bufByteCount = ntohl(db->byteCount);

    int32_t bufRemainderLen = bufCapacity - bufReadIndex;
    int32_t copiedChunkHeadLen = (length < bufRemainderLen) 
                                         ? length
                                         : bufRemainderLen;
    int32_t copiedChunkTailLen = length - copiedChunkHeadLen;

    /* copy the head part */
    memcpy(ptr, db->data + bufReadIndex, copiedChunkHeadLen);
    /* copy the tail part */
    memcpy(ptr + copiedChunkHeadLen, db->data, copiedChunkTailLen);
    
    /* update the data buffer state variables */
    db->readIndex = htonl((bufReadIndex + length) % bufCapacity);
    db->byteCount = htonl(bufByteCount - length);
}

/* 
 * create a new sharedBuffer - allocate new memory for the name using strdup
 */
SharedBuffer* CreateNewSharedBuffer(char* name){
    SharedBuffer * sb;
    assert(name != NULL); 
    sb = (SharedBuffer *) malloc (sizeof (SharedBuffer));
    if (sb == NULL) { 
        error("SUBLIMEIO: Could not create shared buffer");
        fflush(stderr); 
        return NULL; 
    }    
    
    if (sharedBuffer_create(sb, name) != 0){
        error("SUBLIMEIO: Could not create shared buffer");
        fflush(stderr); 
        return NULL; 
    }

    return sb; 
}

/*
 * open an existing shared buffer object with a given name using strdup 
 */
SharedBuffer* OpenSharedBuffer(char* bufferName){
#ifndef WIN32
    return CreateNewSharedBuffer(bufferName); 
}
#else /* WIN32 */
    SharedBuffer *sb = (SharedBuffer *) malloc(sizeof(SharedBuffer));
    int32_t size = SUBLIME_USABLE_BUFFER_SIZE; 

    assert(bufferName != NULL);

    if (sb == NULL) {
        error("SUBLIMEIO: Could not create shared buffer");
        return NULL;
    }

    sb->data.hMapFile = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,   // read/write access
            FALSE,                 // do not inherit the name
            bufferName);           // name of mapping object 
    
    if (sb->data.hMapFile == NULL) { 
        error("SUBLIMEIO: Could not open file mapping object");
        free(sb);
        return NULL;
    } 

    if (sharedBuffer_createIpcObjects(sb, bufferName) != 0) {
        error("SUBLIMEIO: Could not create shared buffer IPC objects");
        CloseHandle(sb->data.hMapFile);
        free(sb);
        return NULL;
    }

    if (sharedBuffer_open(sb, size) != 0) {
        sharedBuffer_destroyIpcObjects(sb);
        CloseHandle(sb->data.hMapFile);
        free(sb);
        return NULL;
    }
    
    return sb; 
}

#endif
