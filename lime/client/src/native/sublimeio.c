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



/*  buffer size + threadID + message length */
#define FIELDS_LENGTH(sb) ((int)(&sb->data->dataBuffer->data-&sb->data->dataBuffer->size))
#define MUTEX_CHAR 'm'

static int debug = 0; 



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

static uint32_t getThreadUniqueId(void) {
    return(GetCurrentThreadId());
}

#else /* !WIN32 */

static void error(char * errorMsg){
    fprintf(stderr,"%s (%d)\n",errorMsg, errno);
}


static uint32_t getThreadUniqueId(void) {
    return (pthread_self());
}

#endif /* WIN32 */


static void sharedBuffer_initPointers(SharedBuffer *sb) ;



/* the name of the mutex is identical to the name of the shared buffer 
 * + extra char for mutex (MUTEX_CHAR)
 * return a new allocated string (uses malloc)
 */ 
static char* getMutexName(char* sharedBufferName){
    int nameLength = strlen(sharedBufferName); 
    char* mutexName = (char*)malloc(nameLength + 2);
    strcpy(mutexName, sharedBufferName);
    mutexName[nameLength] = MUTEX_CHAR;
    mutexName[nameLength + 1] = 0;
    return mutexName; 
}

// blocking
static void sharedBuffer_lock(SharedBuffer* sb){
    assert(sb != NULL); 
    assert(sb->data != NULL); 

    
#ifdef WIN32
    
    assert(sb->data->mutex != NULL);
    assert(sb->data->mutex != INVALID_HANDLE_VALUE);

#endif
    WaitForMutex(sb->data->mutex); 
}

static void sharedBuffer_unlock(SharedBuffer* sb){
    assert(sb != NULL); 
    assert(sb->data != NULL); 

#ifdef WIN32 

    assert(sb->data->mutex != NULL);
    assert(sb->data->mutex != INVALID_HANDLE_VALUE);

#endif
    LimeReleaseMutex(sb->data->mutex);
}

/* returns the buffer size (dataBuffer[0,3]) */ 

static int32_t sharedBuffer_getBufferSize(SharedBuffer *sb){
    return sb->data->dataBuffer->size;
}


static int32_t sharedBuffer_getMsgLength(SharedBuffer *sb){
    return ntohl(sb->data->dataBuffer->dataLength); 
}


/*
 * size - the desired buffer size. 
 * maps the actual shared buffer to a *char. 
 * returns 0 on success, errcode otherwise
 * this method is used by open() (and will be used by future feature: resize() )
 */
static int sharedBuffer_attachBuffer(SharedBuffer* sb, int size ){
    assert(sb != NULL);
    assert(sb->data != NULL);
    assert(sb->data->hMapFile != NULL);

#ifdef WIN32 
    
    assert(sb->data->hMapFile != INVALID_HANDLE_VALUE); 
    sb->data->dataBuffer = 
        (DataBuffer*) MapViewOfFile(sb->data->hMapFile,   
                                    FILE_MAP_ALL_ACCESS, //read/write permission
                                    0, 0, 
                                    size + (int)FIELDS_LENGTH(sb)); 

    if (sb->data->dataBuffer == NULL) { 
        error("SUBLIMEIO: Could not map view of file ");
        exit(1);
    }

#else /* !WIN32 */
  
    sb->data->dataBuffer = (DataBuffer*)shmat(*(sb->data->hMapFile), NULL, 0);
    if ((int)sb->data->dataBuffer == -1) { 
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
    
    
        fprintf(stderr, "sb->data->dataBuffer %d\n", sb->data->dataBuffer);
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
    return res; 
}


/* create a new file mapping and open the shared buffer */
static int sharedBuffer_create(SharedBuffer* sb, char* bufferName){
    int size = BUFFER_SIZE; 
    int res; 
    char* mutexName; 

#ifndef WIN32 
    key_t kt; 
    int shmid, key_id; 
    struct shmid_ds shmds; 
    static int pid_ext = 0; 
  
#endif
    assert(sb != NULL);
    assert(bufferName != NULL);
    
    sb->data->write_pointer = sb->data->read_pointer = 0;

    mutexName = getMutexName(bufferName);
    sb->data->mutex = LimeCreateMutex(NULL, FALSE, mutexName);
    free(mutexName);
     
    if (sb->data->mutex == NULL){
        error("SUBLIMEIO: Could not create mutex ");
        exit(1);    
    }

#ifdef WIN32    
    
    sb->data->hMapFile = 
        CreateFileMapping(INVALID_HANDLE_VALUE,    // use paging file
                          NULL,                    // default security 
                          PAGE_READWRITE,          // read/write access
                          0,                       // max. object size 
                          size + (int)FIELDS_LENGTH(sb),     // buffer size  
                          bufferName);       // name of mapping object
    if ((sb->data->hMapFile == NULL)||
        (sb->data->hMapFile == INVALID_HANDLE_VALUE)) {
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
    sb->data->hMapFile = (int *)malloc(sizeof(int)); 
    if ( sb->data->hMapFile == NULL){
        error("SUBLIMEIO: malloc failed"); 
    } 

    /* 511 - all permissions */ 
    if ((shmid = shmget(kt, BUFFER_SIZE, IPC_CREAT | 511 )) == -1){
        error("CreateNewSharedBuffer: shmget failed");
    } 

    //fprintf(stderr,"shared buffer number is %d  and key is %d\n",shmid, key_id); 
    //fflush(stderr); 
    
    if ((shmctl(shmid, IPC_STAT, &shmds)) == -1){
        error("CreateNewSharedBuffer: shmctl failed");
    } 

    shmds.shm_perm.mode =  shmds.shm_perm.mode | O_RDWR  ;
    *(sb->data->hMapFile) = shmid;

#endif /* WIN32 */ 
    
    if ((res =  sharedBuffer_open(sb, size)) != 0){
        return res;
    }
    sb->data->dataBuffer->size = size; 
    return res; 
    
}  

/* the shared buffer is really closed only after all ponters 
 * to the buffer are closed 
 */ 
static int sharedBuffer_close(SharedBuffer* sb){
    assert(sb != NULL); 
    assert(sb->data != NULL); 
    assert(sb->data->dataBuffer != NULL);
    assert(sb->data->hMapFile != NULL);
#ifdef WIN32
 
    assert(sb->data->hMapFile != INVALID_HANDLE_VALUE);
    UnmapViewOfFile((char*)sb->data->dataBuffer);
    CloseHandle(sb->data->hMapFile); 
    CloseHandle(sb->data->mutex); 

#else /* !WIN32 */

    shmdt(sb->data->dataBuffer); 

    /* 
     * if we haven't created the shared memory, the following call will fail,
     * but because it is called by both sides, it will get eventually destroyed
     */
    shmctl(*(sb->data->hMapFile), IPC_RMID, NULL);

    pthread_mutex_destroy(sb->data->mutex); 
    close(*(sb->data->hMapFile));

#endif /* WIN32 */ 
    free(sb->data); 
    free(sb); 
    return 0; 
}


/* first 4 bytes (sizeof int32_t) reserve for the actual length */
static int sharedBuffer_write(SharedBuffer* sb, char * buffer, int numOfBytes){
    uint32_t msgLength; 
    assert(sb != NULL); 
    assert(sb->data != NULL); 
    assert(sb->data->dataBuffer != NULL);
    if (numOfBytes + sb->getMsgLength(sb) > sb->getBufferSize(sb)){  
        fprintf(stderr, " SUBLIMEIO: buffer size ");
        fprintf(stderr, "(%d) is not big enough for message size (%d)\n", 
                sb->getBufferSize(sb), numOfBytes+ sb->getMsgLength(sb));
        fflush(stderr); 
        exit(1); 
    } 
    msgLength = ntohl(sb->data->dataBuffer->dataLength); 
    msgLength += numOfBytes;  
    msgLength = htonl(msgLength); 
    sb->data->dataBuffer->dataLength = msgLength; 
    memcpy(&sb->data->dataBuffer->data + sb->data->write_pointer, 
           buffer, numOfBytes);   
    sb->data->write_pointer += numOfBytes; 
    return 0;
}

static int sharedBuffer_read(SharedBuffer* sb, char * targetBuffer, 
                             int32_t bufSize, int *bytes){
    int32_t length; 
    assert(sb != NULL); 
    assert(sb->data != NULL); 
    assert(sb->data->dataBuffer != NULL);
    assert(targetBuffer != NULL); 
    assert(bufSize >= 0);
    length = ntohl(sb->data->dataBuffer->dataLength); 
    length = (length < bufSize) ? length : bufSize; 
    memcpy(targetBuffer, &sb->data->dataBuffer->data + sb->data->read_pointer, 
           length);
    sb->data->read_pointer += length;
    *bytes = length;
    return 0;
}

static int sharedBuffer_printf(SharedBuffer *sb, char *format, ...) {
    char buffer[1024];
    va_list args;
    assert(sb != NULL);
    assert(format != NULL);
    va_start(args, format);
#ifdef WIN32 
    
    _vsnprintf(buffer, 1024, format, args);

#else /* !win32 */ 
    
    vsnprintf(buffer, 1024, format, args);
    
#endif
    va_end(args);
    return sb->write(sb, buffer, strlen(buffer));
}

void DeleteSharedBuffer(SharedBuffer *sb){
    sharedBuffer_close(sb); 
}


static int sharedBuffer_readn(SharedBuffer *sb, char *buffer, int length) {
    while (length != 0) {
        int readBytes = 0;
        int rc = sb->read(sb, buffer, length, &readBytes);
        if (rc) return rc;
        length -= readBytes;
        buffer += readBytes;
    }
    return 0;
}

static int readInt(SharedBuffer *sb, uint32_t *result) {
    assert(sb != NULL);
    return sharedBuffer_readn(sb, (char *) result, sizeof(uint32_t));
}


/** Extra optimization here, since it is the most frequently called
 * socket function */
static int sharedBuffer_writeInt32(SharedBuffer *sb, int32_t value) {
    value = htonl(value); 
    return sb->write(sb, (char*)&value, sizeof(int32_t));
}

/* converts the result from network byteorder to host byteorder */ 

static int sharedBuffer_readInt32(SharedBuffer *sb, int32_t *result) {
    int bytes; 
    assert(sb != NULL);
    assert(result != NULL);
    sharedBuffer_read(sb,(char*) result, sizeof(int32_t), &bytes);
    *result = ntohl(*result);
    return 0;
}


static int sharedBuffer_readLong64(SharedBuffer *sb, long64 *result) {
    uint32_t l = 12345678, h = 12345678;
    assert(sb != NULL);
    assert(result != NULL);
    if (readInt(sb, &h)) return 1;
    if (readInt(sb, &l)) return 1;
    (*result) = ((long64)ntohl(h) << 32) | ((long64)ntohl(l));
    return 0;
}


/*
 * reset the shared buffer fields for a new write operation 
 */
static int sharedBuffer_reset(SharedBuffer *sb){
    assert(sb != NULL);
    assert(sb->data != NULL);
    sb->data->write_pointer = sb->data->read_pointer = 0 ; 
    sb->data->dataBuffer->dataLength = 0; 
    sb->data->dataBuffer->threadID = 0;
    return 0;
}

static int32_t sharedBuffer_getThreadID(SharedBuffer* sb){
    assert(sb != NULL); 
    assert(sb->data != NULL); 
    assert(sb->data->dataBuffer != NULL); 
    return sb->data->dataBuffer->threadID; 
}

static void sharedBuffer_stampID(SharedBuffer *sb){
    uint32_t id = getThreadUniqueId(); 
    id = htonl(id); 
    assert(sb != NULL); 
    assert(sb->data != NULL); 
    assert(sb->data->dataBuffer != NULL); 
    sb->data->dataBuffer->threadID = id; 
}

/* id - network byte order */ 
static void sharedBuffer_setID(SharedBuffer *sb,uint32_t id){
    assert(sb != NULL); 
    assert(sb->data != NULL); 
    assert(sb->data->dataBuffer != NULL); 
    sb->data->dataBuffer->threadID = id; 
}

static void sharedBuffer_initPointers(SharedBuffer *sb) {
    sb->read = sharedBuffer_read;
    sb->readInt32 = sharedBuffer_readInt32;
    sb->readLong64 = sharedBuffer_readLong64;
    sb->write = sharedBuffer_write;
    sb->writeInt32 = sharedBuffer_writeInt32;
    sb->printf = sharedBuffer_printf;
    sb->lock = sharedBuffer_lock;  
    sb->unlock = sharedBuffer_unlock;
    sb->getBufferSize = sharedBuffer_getBufferSize;
    sb->reset = sharedBuffer_reset;
    sb->getThreadID = sharedBuffer_getThreadID;
    sb->stampID = sharedBuffer_stampID; 
    sb->setID = sharedBuffer_setID; 
    sb->getMsgLength = sharedBuffer_getMsgLength;
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
    sb->data = (InternalSharedBufferData *)malloc(sizeof(InternalSharedBufferData));
    if (sb->data == NULL) {
        error("SUBLIMEIO: Could not create Shared Buffer");
        exit(1);    
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
SharedBuffer* OpenSharedBuffer(char* name){
#ifndef WIN32
    return CreateNewSharedBuffer(name); 
}
#else /* WIN32 */
SharedBuffer * sb = (SharedBuffer *) malloc (sizeof (SharedBuffer));
char* mutexName; 
int32_t size = BUFFER_SIZE; 
assert(name != NULL);
assert(sb !=NULL);
sb->data = (InternalSharedBufferData *) malloc 
    (sizeof(InternalSharedBufferData));
assert(sb->data !=NULL);
sb->data->hMapFile = OpenFileMapping(
    FILE_MAP_ALL_ACCESS,   // read/write access
    FALSE,                 // do not inherit the name
    name);                 // name of mapping object 
    
if (sb->data->hMapFile == NULL)  { 
    error("SUBLIMEIO: Could not open file mapping object");
    return NULL;
} 
mutexName = getMutexName(name); 
sb->data->mutex = LimeCreateMutex(NULL,FALSE,mutexName); 
free(mutexName);

if ( sharedBuffer_open(sb, size) != 0 ){
    return NULL;
}
sb->data->write_pointer = sb->data->read_pointer = 0 ; 
sb->data->dataBuffer->size = size; 
return sb; 
}

#endif


