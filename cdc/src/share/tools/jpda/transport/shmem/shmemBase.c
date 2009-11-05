/*
 * @(#)shmemBase.c	1.10 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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
 *
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "sysShmem.h"
#include "shmemBase.h"
#include "transportSPI.h"  /* for Packet, TransportCallback */
#include "util.h" /* for MIN */

/*
 * This is the base shared memory transport implementation that is used
 * by both front-end transports (through com.sun.tools.jdi) and 
 * back-end transports (through JDWP_OnLoad and the function tables 
 * it requires). It supports multiple connections for the benefit of the 
 * front-end client; the back end interface assumes only a single connection.
 */  

#define MAX_IPC_PREFIX 50   /* user-specified or generated name for */
                            /* shared memory seg and prefix for other IPC */
#define MAX_IPC_SUFFIX 25   /* suffix to shmem name for other IPC names */
#define MAX_IPC_NAME   (MAX_IPC_PREFIX + MAX_IPC_SUFFIX)

#define MAX_GENERATION_RETRIES 20
#define SHARED_BUFFER_SIZE 5000

#define CHECK_ERROR(expr) do { \
                              jint error = (expr); \
                              if (error != SYS_OK) { \
                                  return error; \
                              } \
                          } while (0)

/*
 * The following assertions should hold anytime the stream's mutex is not held
 */
#define STREAM_INVARIANT(stream) \
        do { \
            SHMEM_ASSERT((stream->shared->readOffset < SHARED_BUFFER_SIZE) \
                         && (stream->shared->readOffset >= 0)); \
            SHMEM_ASSERT((stream->shared->writeOffset < SHARED_BUFFER_SIZE) \
                         && (stream->shared->writeOffset >= 0)); \
        } while (0)

/*   
 * Transports are duplex, so carve the shared memory into "streams",
 * one used to send from client to server, the other vice versa.
 */
typedef struct SharedMemoryListener {
    char mutexName[MAX_IPC_NAME];
    char acceptEventName[MAX_IPC_NAME];
    char attachEventName[MAX_IPC_NAME];
    jboolean isListening;
    jboolean isAccepted;
    jlong acceptingPID;
    jlong attachingPID;
} SharedListener;

typedef struct SharedMemoryTransport {
    char name[MAX_IPC_PREFIX];
    sys_ipmutex_t mutex;
    sys_event_t acceptEvent;
    sys_event_t attachEvent;
    sys_shmem_t sharedMemory;
    SharedListener *shared;
} SharedMemoryTransport;

typedef struct SharedStream {
    char mutexName[MAX_IPC_NAME];
    char hasDataEventName[MAX_IPC_NAME];
    char hasSpaceEventName[MAX_IPC_NAME];
    int readOffset;
    int writeOffset;
    jboolean isFull;
    jbyte buffer[SHARED_BUFFER_SIZE];
} SharedStream;

typedef struct SharedMemory {
    SharedStream toClient;
    SharedStream toServer;
} SharedMemory;

typedef struct Stream {
    sys_ipmutex_t mutex;
    sys_event_t hasData;
    sys_event_t hasSpace;
    SharedStream *shared;
} Stream;

typedef struct SharedMemoryConnection {
    char name[MAX_IPC_NAME];
    SharedMemory *shared;
    sys_shmem_t sharedMemory;
    Stream incoming;   
    Stream outgoing;
    sys_process_t otherProcess;
} SharedMemoryConnection;

static TransportCallback *callback;
static JavaVM *jvm;

typedef jint (*CreateFunc)(char *name, void *arg);

static void 
exitWithError(char *fileName, char *date, int lineNumber, 
              char *message, jint errorCode)
{
    char buf[500];

    /* Get past path info */
    char *p1 = strrchr(fileName, '\\');
    char *p2 = strrchr(fileName, '/');
    p1 = ((p1 > p2) ? p1 : p2);
    if (p1 != NULL) {
        fileName = p1 + 1;
    }

    if (errorCode != 0) {		                              
        sprintf(buf, "JDI \"%s\" (%s), line %d: %s, error code = %d (%s)\n",
                fileName, date, lineNumber, message, errorCode, 
                "");             \
    } else {                                                       
        sprintf(buf, "JDI \"%s\" (%s), line %d: %s\n",                    
                fileName, date, lineNumber, message);             \
    }                                                              
    printf("%s", buf);
    exit(0);
}

jint 
shmemBase_initialize(JavaVM *vm, TransportCallback *cbPtr)
{
    jvm = vm;
    callback = cbPtr;
    return SYS_OK;
}

static jint
createWithGeneratedName(char *prefix, char *nameBuffer, CreateFunc func, void *arg)
{
    jint error;
    jint i = 0;

    do {
        strcpy(nameBuffer, prefix);
        if (i > 0) {
            char buf[10];
            sprintf(buf, ".%d", i+1);
            strcat(nameBuffer, buf);
        }
        error = func(nameBuffer, arg);
        i++;
    } while ((error == SYS_INUSE) && (i < MAX_GENERATION_RETRIES));

    return error;
}

typedef struct SharedMemoryArg {
    jint size;
    sys_shmem_t memory;
    void *start;
} SharedMemoryArg;

static jint 
createSharedMem(char *name, void *ptr)
{
    SharedMemoryArg *arg = ptr;
    return sysSharedMemCreate(name, arg->size, &arg->memory, &arg->start);
}

static jint 
createMutex(char *name, void *arg)
{
    sys_ipmutex_t *retArg = arg;
    return sysIPMutexCreate(name, retArg);
}

static jint 
createEvent(char *name, void *arg)
{
    sys_event_t *retArg = arg;
    return sysEventCreate(name, retArg);
}

#define ADD_OFFSET(o1, o2) ((o1 + o2) % SHARED_BUFFER_SIZE)
#define FULL(stream) (stream->shared->isFull)
#define EMPTY(stream) ((stream->shared->writeOffset == stream->shared->readOffset) \
                       && !stream->shared->isFull)

static jint
enterMutex(Stream *stream)
{
    return sysIPMutexEnter(stream->mutex);
}

static jint
leaveMutex(Stream *stream)
{
    return sysIPMutexExit(stream->mutex);
}

static jint 
waitForSpace(SharedMemoryConnection *connection, Stream *stream)
{
    jint error = SYS_OK; 

    /* Assumes mutex is held on call */
    while (FULL(stream) && (error == SYS_OK)) {
        CHECK_ERROR(leaveMutex(stream));
        error = sysEventWait(connection->otherProcess, stream->hasSpace);
        CHECK_ERROR(enterMutex(stream));
    }
    return error;
}

static jint
signalSpace(Stream *stream)
{
    return sysEventSignal(stream->hasSpace);
}

static jint
waitForData(SharedMemoryConnection *connection, Stream *stream)
{
    jint error = SYS_OK;

    /* Assumes mutex is held on call */
    while (EMPTY(stream) && (error == SYS_OK)) {
        CHECK_ERROR(leaveMutex(stream));
        error = sysEventWait(connection->otherProcess, stream->hasData);
        CHECK_ERROR(enterMutex(stream));
    }
    return error;
}

static jint
signalData(Stream *stream)
{
    return sysEventSignal(stream->hasData);
}


static void
closeStream(Stream *stream) 
{
    sysIPMutexClose(stream->mutex);
    sysEventClose(stream->hasData);
    sysEventClose(stream->hasSpace);
}

static int
createStream(char *name, Stream *stream)
{
    jint error;
    char prefix[MAX_IPC_PREFIX];

    sprintf(prefix, "%s.mutex", name);
    error = createWithGeneratedName(prefix, stream->shared->mutexName,
                                    createMutex, &stream->mutex);
    if (error != SYS_OK) {
        fprintf(stderr,"Error creating mutex, rc = %d\n", error);
        return error;
    }

    sprintf(prefix, "%s.hasData", name);
    error = createWithGeneratedName(prefix, stream->shared->hasDataEventName,
                                    createEvent, &stream->hasData);
    if (error != SYS_OK) {
        fprintf(stderr,"Error creating event, rc = %d\n", error);
        closeStream(stream);
        return error;
    }

    sprintf(prefix, "%s.hasSpace", name);
    error = createWithGeneratedName(prefix, stream->shared->hasSpaceEventName,
                                    createEvent, &stream->hasSpace);
    if (error != SYS_OK) {
        fprintf(stderr,"Error creating event, rc = %d\n", error);
        closeStream(stream);
        return error;
    }

    stream->shared->readOffset = 0;
    stream->shared->writeOffset = 0;
    stream->shared->isFull = JNI_FALSE;
    return SYS_OK;
}


/*
 * Initialization for the stream opened by the other process
 */
static int
openStream(Stream *stream)
{
    jint error;

    error = sysIPMutexOpen(stream->shared->mutexName, &stream->mutex);
    if (error != SYS_OK) {
        fprintf(stderr,"Error accessing mutex, rc = %d\n", error);
        return error;
    }

    error = sysEventOpen(stream->shared->hasDataEventName,
                             &stream->hasData);
    if (error != SYS_OK) {
        fprintf(stderr,"Error accessing mutex, rc = %d\n", error);
        closeStream(stream);
        return error;
    }

    error = sysEventOpen(stream->shared->hasSpaceEventName,
                             &stream->hasSpace);
    if (error != SYS_OK) {
        fprintf(stderr,"Error accessing mutex, rc = %d\n", error);
        closeStream(stream);
        return error;
    }

    return SYS_OK;
}

/********************************************************************/

static SharedMemoryConnection *
allocConnection(void)
{
    /*
     * TO DO: Track all allocated connections for clean shutdown?
     */
    return (*callback->alloc)(sizeof(SharedMemoryConnection));
}

static void 
freeConnection(SharedMemoryConnection *connection)
{
    (*callback->free)(connection);
}

static void
closeConnection(SharedMemoryConnection *connection)
{
    closeStream(&connection->incoming);
    closeStream(&connection->outgoing);
    sysSharedMemClose(connection->sharedMemory, connection->shared);
    sysProcessClose(connection->otherProcess);
    freeConnection(connection);
}


static jint
openConnection(SharedMemoryTransport *transport, jlong otherPID, 
               SharedMemoryConnection **connectionPtr)
{
    jint error;

    SharedMemoryConnection *connection = allocConnection();
    if (connection == NULL) {
        return SYS_NOMEM;
    }
    memset(connection, 0, sizeof(*connection));

    sprintf(connection->name, "%s.%ld", transport->name, sysProcessGetID());
    error = sysSharedMemOpen(connection->name, &connection->sharedMemory,
                             &connection->shared);
    if (error != SYS_OK) {
        closeConnection(connection);
        return error;
    }

    /* This process is the client */
    connection->incoming.shared = &connection->shared->toClient;
    connection->outgoing.shared = &connection->shared->toServer;

    error = openStream(&connection->incoming);
    if (error != SYS_OK) {
        closeConnection(connection);
        return error;
    }

    error = openStream(&connection->outgoing);
    if (error != SYS_OK) {
        closeConnection(connection);
        return error;
    }

    error = sysProcessOpen(otherPID, &connection->otherProcess);
    if (error != SYS_OK) {
        fprintf(stderr,"Error accessing process, rc = %d\n", error);
        closeConnection(connection);
        return error;
    }

    *connectionPtr = connection;
    return SYS_OK;
}

static jint
createConnection(SharedMemoryTransport *transport, jlong otherPID, 
                 SharedMemoryConnection **connectionPtr)
{
    jint error;
    char streamPrefix[MAX_IPC_NAME];

    SharedMemoryConnection *connection = allocConnection();
    if (connection == NULL) {
        return SYS_NOMEM;
    }
    memset(connection, 0, sizeof(*connection));

    sprintf(connection->name, "%s.%ld", transport->name, otherPID);
    error = sysSharedMemCreate(connection->name, sizeof(SharedMemory),
                               &connection->sharedMemory, &connection->shared);
    if (error != SYS_OK) {
        closeConnection(connection);
        return error;
    }

    memset(connection->shared, 0, sizeof(SharedMemory));

    /* This process is the server */
    connection->incoming.shared = &connection->shared->toServer;
    connection->outgoing.shared = &connection->shared->toClient;

    strcpy(streamPrefix, connection->name);
    strcat(streamPrefix, ".ctos");
    error = createStream(streamPrefix, &connection->incoming);
    if (error != SYS_OK) {
        closeConnection(connection);
        return error;
    }

    strcpy(streamPrefix, connection->name);
    strcat(streamPrefix, ".stoc");
    error = createStream(streamPrefix, &connection->outgoing);
    if (error != SYS_OK) {
        closeConnection(connection);
        return error;
    }

    error = sysProcessOpen(otherPID, &connection->otherProcess);
    if (error != SYS_OK) {
        fprintf(stderr,"Error accessing process, rc = %d\n", error);
        closeConnection(connection);
        return error;
    }

    *connectionPtr = connection;
    return SYS_OK;
}

/********************************************************************/

static SharedMemoryTransport *
allocTransport(void)
{
    /*
     * TO DO: Track all allocated transports for clean shutdown?
     */
    return (*callback->alloc)(sizeof(SharedMemoryTransport));
}

static void 
freeTransport(SharedMemoryTransport *transport)
{
    (*callback->free)(transport);
}

static void
closeTransport(SharedMemoryTransport *transport) 
{
    sysIPMutexClose(transport->mutex);
    sysEventClose(transport->acceptEvent);
    sysEventClose(transport->attachEvent);
    sysSharedMemClose(transport->sharedMemory, transport->shared);
    freeTransport(transport);
}

static int
openTransport(const char *address, SharedMemoryTransport **transportPtr)
{
    jint error;
    SharedMemoryTransport *transport;

    transport = allocTransport();
    if (transport == NULL) {
        return SYS_NOMEM;
    }
    memset(transport, 0, sizeof(*transport));

    if (strlen(address) >= MAX_IPC_PREFIX) {
        fprintf(stderr,"Error: address strings longer than %d characters are invalid\n", MAX_IPC_PREFIX);
        closeTransport(transport);
        return SYS_ERR;
    }

    error = sysSharedMemOpen(address, &transport->sharedMemory, &transport->shared);
    if (error != SYS_OK) {
        fprintf(stderr,"Error accessing shared memory, rc = %d\n", error);
        closeTransport(transport);
        return error;
    }
    strcpy(transport->name, address);

    error = sysIPMutexOpen(transport->shared->mutexName, &transport->mutex);
    if (error != SYS_OK) {
        fprintf(stderr,"Error accessing mutex, rc = %d\n", error);
        closeTransport(transport);
        return error;
    }

    error = sysEventOpen(transport->shared->acceptEventName,
                             &transport->acceptEvent);
    if (error != SYS_OK) {
        fprintf(stderr,"Error accessing mutex, rc = %d\n", error);
        closeTransport(transport);
        return error;
    }

    error = sysEventOpen(transport->shared->attachEventName,
                             &transport->attachEvent);
    if (error != SYS_OK) {
        fprintf(stderr,"Error accessing mutex, rc = %d\n", error);
        closeTransport(transport);
        return error;
    }

    *transportPtr = transport;
    return SYS_OK;
}

static jint 
createTransport(const char *address, SharedMemoryTransport **transportPtr)
{
    SharedMemoryTransport *transport;
    jint error;
    char prefix[MAX_IPC_PREFIX];



    transport = allocTransport();
    if (transport == NULL) {
        return SYS_NOMEM;
    }
    memset(transport, 0, sizeof(*transport));

    if ((address == NULL) || (address[0] == '\0')) {
        SharedMemoryArg arg;
        arg.size = sizeof(SharedListener);
        error = createWithGeneratedName("javadebug", transport->name,
                                        createSharedMem, &arg);
        transport->shared = arg.start;
        transport->sharedMemory = arg.memory;
    } else {
        if (strlen(address) >= MAX_IPC_PREFIX) {
            fprintf(stderr,"Error: address strings longer than %d characters are invalid\n", MAX_IPC_PREFIX);
            closeTransport(transport);
            return SYS_ERR;
        }
        strcpy(transport->name, address);
        error = sysSharedMemCreate(address, sizeof(SharedListener),
                                   &transport->sharedMemory, &transport->shared);
    }
    if (error != SYS_OK) {
        fprintf(stderr,"Error creating shared memory, rc = %d\n", error);
        closeTransport(transport);
        return error;
    }

    memset(transport->shared, 0, sizeof(SharedListener));
    transport->shared->acceptingPID = sysProcessGetID();

    sprintf(prefix, "%s.mutex", transport->name);
    error = createWithGeneratedName(prefix, transport->shared->mutexName,
                                    createMutex, &transport->mutex);
    if (error != SYS_OK) {
        fprintf(stderr,"Error creating mutex, rc = %d\n", error);
        closeTransport(transport);
        return error;
    }

    sprintf(prefix, "%s.accept", transport->name);
    error = createWithGeneratedName(prefix, transport->shared->acceptEventName,
                                    createEvent, &transport->acceptEvent);
    if (error != SYS_OK) {
        fprintf(stderr,"Error creating event, rc = %d\n", error);
        closeTransport(transport);
        return error;
    }

    sprintf(prefix, "%s.attach", transport->name);
    error = createWithGeneratedName(prefix, transport->shared->attachEventName,
                                    createEvent, &transport->attachEvent);
    if (error != SYS_OK) {
        fprintf(stderr,"Error creating event, rc = %d\n", error);
        closeTransport(transport);
        return error;
    }

    *transportPtr = transport;
    return SYS_OK;
}


jint 
shmemBase_listen(const char *address, SharedMemoryTransport **transportPtr)
{
    int error;

    error = createTransport(address, transportPtr);
    if (error == SYS_OK) {
        (*transportPtr)->shared->isListening = JNI_TRUE;
    }
    return error;
}


jint
shmemBase_accept(SharedMemoryTransport *transport, 
                 SharedMemoryConnection **connectionPtr) 
{
    jint error;
    SharedMemoryConnection *connection;

    CHECK_ERROR(sysEventWait(NULL, transport->attachEvent));


    error = createConnection(transport, transport->shared->attachingPID, 
                             &connection);
    if (error != SYS_OK) {
        /*
         * Reject the attacher
         */
        transport->shared->isAccepted = JNI_FALSE;
        sysEventSignal(transport->acceptEvent);

        freeConnection(connection);
        return error;
    }

    transport->shared->isAccepted = JNI_TRUE;
    error = sysEventSignal(transport->acceptEvent);
    if (error != SYS_OK) {
        /*
         * No real point trying to reject it.
         */
        closeConnection(connection);
        return error;
    }

    *connectionPtr = connection;
    return SYS_OK;
}

static jint
doAttach(SharedMemoryTransport *transport) 
{
    transport->shared->attachingPID = sysProcessGetID();
    CHECK_ERROR(sysEventSignal(transport->attachEvent));
    CHECK_ERROR(sysEventWait(NULL, transport->acceptEvent));
    return SYS_OK;
}

jint
shmemBase_attach(const char *addressString, SharedMemoryConnection **connectionPtr) 
{
    int error;
    SharedMemoryTransport *transport;
    jlong acceptingPID;

    error = openTransport(addressString, &transport);
    if (error != SYS_OK) {
        return error;
    }

    error = sysIPMutexEnter(transport->mutex);
    if (error != SYS_OK) {
        closeTransport(transport);
        return error;
    }

    if (transport->shared->isListening) {
        error = doAttach(transport);
        if (error == SYS_OK) {
            acceptingPID = transport->shared->acceptingPID;
        }
    } else {
        /* Not listening: error */
        error = SYS_ERR;
    }

    sysIPMutexExit(transport->mutex);
    if (error != SYS_OK) {
        closeTransport(transport);
        return error;
    }
    
    error = openConnection(transport, acceptingPID, connectionPtr);

    closeTransport(transport);

    return error;
}




void 
shmemBase_closeConnection(SharedMemoryConnection *connection)
{
    closeConnection(connection);
}

void 
shmemBase_closeTransport(SharedMemoryTransport *transport)
{
    closeTransport(transport);
}

jint 
shmemBase_sendByte(SharedMemoryConnection *connection, jbyte data)
{
    Stream *stream = &connection->outgoing;
    SharedStream *shared = stream->shared;
    int offset;

    CHECK_ERROR(enterMutex(stream));
    CHECK_ERROR(waitForSpace(connection, stream));
    SHMEM_ASSERT(!FULL(stream));
    offset = shared->writeOffset;
    shared->buffer[offset] = data;
    shared->writeOffset = ADD_OFFSET(offset, 1);
    shared->isFull = (shared->readOffset == shared->writeOffset);

    STREAM_INVARIANT(stream);
    CHECK_ERROR(leaveMutex(stream));

    CHECK_ERROR(signalData(stream));

    return SYS_OK;
}

jint 
shmemBase_receiveByte(SharedMemoryConnection *connection, jbyte *data)
{
    Stream *stream = &connection->incoming;
    SharedStream *shared = stream->shared;
    int offset;

    CHECK_ERROR(enterMutex(stream));
    CHECK_ERROR(waitForData(connection, stream));
    SHMEM_ASSERT(!EMPTY(stream));
    offset = shared->readOffset;
    *data = shared->buffer[offset];
    shared->readOffset = ADD_OFFSET(offset, 1);
    shared->isFull = JNI_FALSE;

    STREAM_INVARIANT(stream);
    CHECK_ERROR(leaveMutex(stream));

    CHECK_ERROR(signalSpace(stream));

    return SYS_OK;
}

static jint
sendBytes(SharedMemoryConnection *connection, void *bytes, jint length)
{
    Stream *stream = &connection->outgoing;
    SharedStream *shared = stream->shared;
    jint fragmentStart;
    jint fragmentLength;
    jint index = 0;
    jint maxLength;

    CHECK_ERROR(enterMutex(stream));
    while (index < length) {
        CHECK_ERROR(waitForSpace(connection, stream));
        SHMEM_ASSERT(!FULL(stream));

        fragmentStart = shared->writeOffset;

        if (fragmentStart < shared->readOffset) {
            maxLength = shared->readOffset - fragmentStart;
        } else {
            maxLength = SHARED_BUFFER_SIZE - fragmentStart;
        }
        fragmentLength = MIN(maxLength, length - index);
        memcpy(shared->buffer + fragmentStart, (jbyte *)bytes + index, fragmentLength);
        shared->writeOffset = ADD_OFFSET(fragmentStart, fragmentLength);
        index += fragmentLength;

        shared->isFull = (shared->readOffset == shared->writeOffset);

        STREAM_INVARIANT(stream);
        CHECK_ERROR(signalData(stream));

    }
    CHECK_ERROR(leaveMutex(stream));

    return SYS_OK;
}


jint 
shmemBase_sendPacket(SharedMemoryConnection *connection, struct Packet *packet)
{
    jint length = 0;
    struct PacketData *data;

    CHECK_ERROR(sendBytes(connection, &packet->type.cmd.id, sizeof(jint)));
    CHECK_ERROR(sendBytes(connection, &packet->type.cmd.flags, sizeof(jbyte)));

    if (packet->type.cmd.flags & FLAGS_Reply) {
        CHECK_ERROR(sendBytes(connection, &packet->type.reply.errorCode, sizeof(jshort)));
    } else {
        CHECK_ERROR(sendBytes(connection, &packet->type.cmd.cmdSet, sizeof(jbyte)));
        CHECK_ERROR(sendBytes(connection, &packet->type.cmd.cmd, sizeof(jbyte)));
    }

    data = &(packet->type.cmd.data);
    do {
        length += data->length;
        data = data->next;
    } while (data != NULL);


    CHECK_ERROR(sendBytes(connection, &length, sizeof(jint)));

    data = &(packet->type.cmd.data);
    do {
        CHECK_ERROR(sendBytes(connection, data->data, data->length));
        data = data->next;
    } while (data != NULL);

    return SYS_OK;
}

static jint
receiveBytes(SharedMemoryConnection *connection, void *bytes, jint length)
{
    Stream *stream = &connection->incoming;
    SharedStream *shared = stream->shared;
    jint fragmentStart;
    jint fragmentLength;
    jint index = 0;
    jint maxLength;

    CHECK_ERROR(enterMutex(stream));
    while (index < length) {
        CHECK_ERROR(waitForData(connection, stream));
        SHMEM_ASSERT(!EMPTY(stream));

        fragmentStart = shared->readOffset;
        if (fragmentStart < shared->writeOffset) {
            maxLength = shared->writeOffset - fragmentStart;
        } else {
            maxLength = SHARED_BUFFER_SIZE - fragmentStart;
        }
        fragmentLength = MIN(maxLength, length - index);
        memcpy((jbyte *)bytes + index, shared->buffer + fragmentStart, fragmentLength);
        shared->readOffset = ADD_OFFSET(fragmentStart, fragmentLength);
        index += fragmentLength;

        shared->isFull = JNI_FALSE;

        STREAM_INVARIANT(stream);
        CHECK_ERROR(signalSpace(stream));
    }
    CHECK_ERROR(leaveMutex(stream));

    return SYS_OK;
}

jint 
shmemBase_receivePacket(SharedMemoryConnection *connection, struct Packet *packet)
{
    jint length;
    jint error;

    CHECK_ERROR(receiveBytes(connection, &packet->type.cmd.id, sizeof(jint)));
    CHECK_ERROR(receiveBytes(connection, &packet->type.cmd.flags, sizeof(jbyte)));

    if (packet->type.cmd.flags & FLAGS_Reply) {
        CHECK_ERROR(receiveBytes(connection, &packet->type.reply.errorCode, sizeof(jshort)));
    } else {
        CHECK_ERROR(receiveBytes(connection, &packet->type.cmd.cmdSet, sizeof(jbyte)));
        CHECK_ERROR(receiveBytes(connection, &packet->type.cmd.cmd, sizeof(jbyte)));
    }

    CHECK_ERROR(receiveBytes(connection, &length, sizeof(jint)));

    if (length < 0) {
        return SYS_ERR;
    } else if (length == 0) {
        packet->type.cmd.data.length = 0;
        packet->type.cmd.data.data = NULL;
        packet->type.cmd.data.next = NULL;
    } else {
        packet->type.cmd.data.length = length;
        packet->type.cmd.data.next = NULL;
        packet->type.cmd.data.data= (*callback->alloc)(length);
        if (packet->type.cmd.data.data == NULL) {
            return SYS_ERR;
        }

        error = receiveBytes(connection, packet->type.cmd.data.data, length);
        if (error != SYS_OK) {
            (*callback->free)(packet->type.cmd.data.data);
            return error;
        }
    }

    return SYS_OK;
}

jint 
shmemBase_name(struct SharedMemoryTransport *transport, char **name)
{
    *name = transport->name;
    return SYS_OK;
}


void 
exitTransportWithError(char *message, char *fileName, 
                       char *date, int lineNumber)
{
    JNIEnv *env;
    jint error;
    char buffer[500];

    sprintf(buffer, "Shared Memory Transport \"%s\" (%s), line %d: %s\n",
            fileName, date, lineNumber, message); 
    error = (*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_2);
    if (error != JNI_OK) {
        /*
         * We're forced into a direct call to exit()
         */
        fprintf(stderr, "%s", buffer);
        exit(-1);
    } else {
        (*env)->FatalError(env, buffer);
    }
}



