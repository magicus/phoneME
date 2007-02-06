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
#include <jump_messaging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "porting/JUMPProcess.h"
#include "porting/JUMPMessageQueue.h"
#include "porting/JUMPThread.h"
#include "porting/JUMPClib.h"

typedef struct {
    JUMPAddress address;
    /* FIXME: rename type to thread? */
    JUMPPlatformCString returnType;
} JUMPReturnAddress;

struct _JUMPMessageHeader {
    uint32 messageId;
    int32 requestId;
    JUMPReturnAddress sender;
    JUMPPlatformCString type;
};

static int MESSAGE_DATA_OFFSET;
static int jumpMessagingInitialized;

/*
 * @brief variable sized encapsulation of a message
 */
struct _JUMPMessage {
    struct _JUMPMessageHeader header;
    /* Length of the data buffer */
    uint32 dataBufferLen;
    /* Current location of the data pointer */
    uint8* dataPtr;
    /* And the data */
    uint8* data;
};

/*
 * This is the api and OS-independent implementation of native
 * messaging in JUMP.
 */

static JUMPAddress
mkAddr(int32 pid)
{
    JUMPAddress addr;
    addr.processId = pid;
    return addr;
}

static JUMPReturnAddress
mkReturnAddr(int32 pid, JUMPPlatformCString type)
{
    JUMPReturnAddress addr;
    addr.address = mkAddr(pid);
    addr.returnType = type;
    return addr;
}

JUMPAddress 
jumpMessageGetMyAddress()
{
    assert(jumpMessagingInitialized != 0);
    return mkAddr(jumpProcessGetId());
}

/*
 * This is how message queues are named, based on pid, and a name
 */
#define JUMP_RESPONSE_QUEUE_NAME_PATTERN "<response-thread-%d>"

char*
jumpMessageGetReturnTypeName()
{
    char name[80];
    char* ret;

    assert(jumpMessagingInitialized != 0);

    snprintf(name, 80, JUMP_RESPONSE_QUEUE_NAME_PATTERN, jumpThreadGetId());

    ret = strdup(name);
    if (ret == NULL) {
	return NULL;
    } else {
	return ret;
    }
}

static JUMPReturnAddress
getMyReturnAddress()
{
    char* name = jumpMessageGetReturnTypeName();

    assert(jumpMessagingInitialized != 0);

    if (name == NULL) {
	return mkReturnAddr(-1, NULL);
    } else {
	/* FIXME: Free string in return address */
	return mkReturnAddr(jumpProcessGetId(), name);
    }
}

JUMPAddress 
jumpMessageGetExecutiveAddress()
{
    assert(jumpMessagingInitialized != 0);
    return mkAddr(jumpProcessGetExecutiveId());
}

/*
 * Given a "raw" buffer, allocated or received, make a corresponding
 * JUMPMessage.
 */
static struct _JUMPMessage* 
newMessageFromBuffer(uint8* buffer, uint32 len)
{
    struct _JUMPMessage* message;
    if (buffer == NULL) {
	/* FIXME: Throw out of memory */
	return NULL;
    }
    message = (struct _JUMPMessage*)calloc(1, sizeof(struct _JUMPMessage));
    message->data = buffer;
    message->dataBufferLen = len;
    message->dataPtr = (uint8*)(buffer + MESSAGE_DATA_OFFSET);
    return message;
}

/*
 * Create a new blank message
 */
static struct _JUMPMessage* 
newMessage()
{
    uint8* buffer = (uint8*)calloc(1, MESSAGE_BUFFER_SIZE);
    return newMessageFromBuffer(buffer, MESSAGE_BUFFER_SIZE);
}

/*
 * Take the 'header' struct in a JUMPMessage, and "serialize" it into
 * the message data area.
 */
static void
putHeaderInMessage(struct _JUMPMessage* m)
{
    struct _JUMPMessageHeader* hdr = &m->header;
    
    jumpMessageAddInt((JUMPOutgoingMessage)m, hdr->messageId);
    jumpMessageAddInt((JUMPOutgoingMessage)m, hdr->requestId);
    jumpMessageAddInt((JUMPOutgoingMessage)m, hdr->sender.address.processId);
    jumpMessageAddString((JUMPOutgoingMessage)m, hdr->sender.returnType);
    jumpMessageAddString((JUMPOutgoingMessage)m, hdr->type);
}

/*
 * Deserialize header from message to 'header' structure in JUMPMessage.
 */
static void
getHeaderFromMessage(struct _JUMPMessage* m)
{
    JUMPMessageReader reader;
    struct _JUMPMessageHeader* hdr = &m->header;
    
    jumpMessageReaderInit(&reader, (JUMPMessage)m);
    
    hdr->messageId = jumpMessageGetInt(&reader);
    hdr->requestId = jumpMessageGetInt(&reader);
    hdr->sender.address.processId  = jumpMessageGetInt(&reader);
    hdr->sender.returnType = jumpMessageGetString(&reader);
    hdr->type = jumpMessageGetString(&reader);

    /* Make sure the data pointer in the message is now set past the header */
    m->dataPtr = reader.ptr;
}

/*
 * Given a "raw" buffer, allocated or received, make a corresponding
 * JUMPMessage.
 */
static struct _JUMPMessage* 
newMessageFromReceivedBuffer(uint8* buffer, uint32 len)
{
    struct _JUMPMessage* message;
    message = newMessageFromBuffer(buffer, len);
    getHeaderFromMessage(message);
    
    return message;
}

/*
 * Running counters for message id's and request id's
 */
static uint32 thisProcessMessageId;
static int32 thisProcessRequestId;

JUMPOutgoingMessage
jumpMessageNewOutgoingFromBuffer(uint8* buffer, int isResponse)
{
    JUMPMessageMark mmarkBeforeHeader;
    JUMPMessageMark mmarkAfterHeader;
    struct _JUMPMessage* message;
    uint32 messageId;

    message = newMessageFromBuffer(buffer, MESSAGE_BUFFER_SIZE);
    jumpMessageMarkSet(&mmarkBeforeHeader, message);
    getHeaderFromMessage(message);
    jumpMessageMarkSet(&mmarkAfterHeader, message);
    
    /* rewind to beginning of header */
    jumpMessageMarkResetTo(&mmarkBeforeHeader, message);
    /* Set message ID in the message payload and in the header. */
    messageId = thisProcessMessageId++;
    jumpMessageAddInt(message, messageId);
    message->header.messageId = messageId;
    
    if (!isResponse) {
	int32 requestId;
	requestId = thisProcessRequestId++;
	message->header.requestId = requestId;
	jumpMessageAddInt(message, requestId);
    }
    
    /* Remember where we left the header */
    jumpMessageMarkResetTo(&mmarkAfterHeader, message);
    return (JUMPOutgoingMessage)message;
}

uint8*
jumpMessageGetData(JUMPMessage message)
{
    return ((struct _JUMPMessage*)message)->data;
}

/*
 * Message header inspection
 */
static JUMPReturnAddress
getReturnAddress(JUMPMessage m)
{
    assert(jumpMessagingInitialized != 0);
    return m->header.sender;
}

JUMPOutgoingMessage
newOutgoingMessage(JUMPPlatformCString type, uint32 requestId, 
		   JUMPReturnAddress addr)
{
    struct _JUMPMessage* message;
    assert(jumpMessagingInitialized != 0);
    message = newMessage();
    if (message == NULL) {
	return NULL;
    }
    message->header.messageId = thisProcessMessageId++;
    message->header.requestId = requestId;
    message->header.sender = addr;
    message->header.type = type;
    putHeaderInMessage(message);
    return (JUMPOutgoingMessage)message;
}

JUMPOutgoingMessage
jumpMessageNewOutgoingByType(JUMPPlatformCString type)
{
    uint32 requestId = thisProcessRequestId++;
    assert(jumpMessagingInitialized != 0);
    return newOutgoingMessage(type, requestId, getMyReturnAddress());
}

JUMPOutgoingMessage
jumpMessageNewOutgoingByRequest(JUMPMessage requestMessage)
{
    uint32 requestId = requestMessage->header.requestId;
    assert(jumpMessagingInitialized != 0);
    return newOutgoingMessage(jumpMessageGetType(requestMessage), requestId, 
			      getReturnAddress(requestMessage));
}

void
jumpMessageMarkSet(JUMPMessageMark* mmark, struct _JUMPMessage* m)
{
    mmark->ptr = m->dataPtr;
    mmark->m = m;
}

void
jumpMessageMarkResetTo(JUMPMessageMark* mmark, struct _JUMPMessage* m)
{
    assert(m == mmark->m); /* sanity */
    m->dataPtr = mmark->ptr;
}


void
jumpMessageAddByte(JUMPOutgoingMessage m, int8 value)
{
    assert(jumpMessagingInitialized != 0);
    m->dataPtr[0] = value;
    m->dataPtr += 1;
}

void
jumpMessageAddBytesFrom(JUMPOutgoingMessage m, int8* values, int length)
{
    assert(jumpMessagingInitialized != 0);
    /* FIXME: capacity check! */
    if ((values == NULL) || (length == 0)) {
	return;
    }
    memcpy(m->dataPtr, values, length);
    m->dataPtr += length;
}

void
jumpMessageAddByteArray(JUMPOutgoingMessage m, int8* values, int length)
{
    assert(jumpMessagingInitialized != 0);
    /* FIXME: capacity check! */
    if ((values == NULL) || (length == 0)) {
	jumpMessageAddInt(m, 0);
	return;
    }
    jumpMessageAddInt(m, length);
    memcpy(m->dataPtr, values, length);
    m->dataPtr += length;
}

void
jumpMessageAddInt(JUMPOutgoingMessage m, int32 value)
{
    uint32 v;
    assert(jumpMessagingInitialized != 0);
    v = (uint32)value;
    m->dataPtr[0] = (v >> 24) & 0xff;
    m->dataPtr[1] = (v >> 16) & 0xff;
    m->dataPtr[2] = (v >>  8) & 0xff;
    m->dataPtr[3] = (v >>  0) & 0xff;
#if 0
    printf("Encoded %d as [%d,%d,%d,%d]\n", value,
	   m->dataPtr[0],
	   m->dataPtr[1],
	   m->dataPtr[2],
	   m->dataPtr[3]);
#endif
    m->dataPtr += 4;
}

void
jumpMessageAddShort(JUMPOutgoingMessage m, int16 value) {
    uint16 v;
    assert(jumpMessagingInitialized != 0);
    v = (uint16)value;
    m->dataPtr[0] = (v >>  8) & 0xff;
    m->dataPtr[1] = (v >>  0) & 0xff;
#if 0
    printf("Encoded %d as [%d,%d]\n", value,
	   m->dataPtr[0],
	   m->dataPtr[1]);
#endif
    m->dataPtr += 2;
}

void
jumpMessageAddLong(JUMPOutgoingMessage m, int64 value)
{
    uint64 v;
    assert(jumpMessagingInitialized != 0);
    v = (uint64)value;
    m->dataPtr[0] = (v >> 56) & 0xff;
    m->dataPtr[1] = (v >> 48) & 0xff;
    m->dataPtr[2] = (v >> 40) & 0xff;
    m->dataPtr[3] = (v >> 32) & 0xff;
    m->dataPtr[4] = (v >> 24) & 0xff;
    m->dataPtr[5] = (v >> 16) & 0xff;
    m->dataPtr[6] = (v >>  8) & 0xff;
    m->dataPtr[7] = (v >>  0) & 0xff;
#if 0
    printf("Encoded %d%d as [%d,%d,%d,%d,%d,%d,%d,%d]\n", 
       value/(1<<32), value%(1<<32),
	   m->dataPtr[0],
	   m->dataPtr[1],
	   m->dataPtr[2],
	   m->dataPtr[3],
	   m->dataPtr[4],
	   m->dataPtr[5],
	   m->dataPtr[6],
	   m->dataPtr[7]);
#endif
    m->dataPtr += 8;
}

void
jumpMessageAddString(JUMPOutgoingMessage m, JUMPPlatformCString str)
{
    assert(jumpMessagingInitialized != 0);
    /* FIXME: capacity check! */
    /* FIXME: ASCII assumption for now */
    /* By the ascii assumption, a string is a byte array of length
       strlen(str) + 1 for the terminating '\0' */
    jumpMessageAddByteArray(m, (int8*)str, strlen(str) + 1);
}

void
jumpMessageAddStringArray(JUMPOutgoingMessage m,
			  JUMPPlatformCString* strs,
			  uint32 length)
{
    uint32 i;
    assert(jumpMessagingInitialized != 0);
    /* FIXME: capacity check! */
    if ((strs == NULL) || (length == 0)) {
	jumpMessageAddInt(m, 0);
	return;
    }
    jumpMessageAddInt(m, length);
    for (i = 0; i < length; i++) {
	jumpMessageAddString(m, strs[i]);
    }
}

/*
 * An iterator to read off of a message
 */
void
jumpMessageReaderInit(JUMPMessageReader* r, JUMPMessage m)
{
    assert(jumpMessagingInitialized != 0);
    r->ptr = m->dataPtr;
}

int8
jumpMessageGetByte(JUMPMessageReader* r)
{
    int8 ret = r->ptr[0];
    assert(jumpMessagingInitialized != 0);
    r->ptr += 1;
    return ret;
}

int8*
jumpMessageGetBytesInto(JUMPMessageReader* r, int8* buffer, uint32 length) {
    memcpy(buffer, r->ptr, length);
    r->ptr += length;
    
    return buffer;
}

int8*
jumpMessageGetByteArray(JUMPMessageReader* r, uint32* lengthPtr)
{
    int8* bytearray;
    uint32 length;
    
    assert(jumpMessagingInitialized != 0);
    length = jumpMessageGetInt(r);
    bytearray = calloc(1, length);
    if (bytearray == NULL) {
	/* Caller discards message? Or do we "rewind" to the start of the
	   length field again? */
	return NULL;
    }
    
    memcpy(bytearray, r->ptr, length);
    r->ptr += length;
    *lengthPtr = length;
    
    return bytearray;
}

int32
jumpMessageGetInt(JUMPMessageReader* r)
{
    int32 i = (int32)
	(((uint8)r->ptr[0] << 24) | 
	 ((uint8)r->ptr[1] << 16) | 
	 ((uint8)r->ptr[2] << 8) | 
	  (uint8)r->ptr[3]);
    assert(jumpMessagingInitialized != 0);
    r->ptr += 4;
    return i;
}

int16
jumpMessageGetShort(JUMPMessageReader* r)
{
    int16 i = (int16)
	(((uint8)r->ptr[0] << 8) | 
	  (uint8)r->ptr[1]);
    assert(jumpMessagingInitialized != 0);
    r->ptr += 2;
    return i;
}

int64
jumpMessageGetLong(JUMPMessageReader* r)
{
    int64 i = (int64)
	(((uint64)r->ptr[0] << 56) | 
	 ((uint64)r->ptr[1] << 48) | 
	 ((uint64)r->ptr[2] << 40) | 
	 ((uint64)r->ptr[3] << 32) |
     ((uint64)r->ptr[4] << 24) | 
	 ((uint64)r->ptr[5] << 16) | 
	 ((uint64)r->ptr[6] <<  8) | 
	  (uint64)r->ptr[7]);
    assert(jumpMessagingInitialized != 0);
    r->ptr += 8;
    return i;
}

JUMPPlatformCString
jumpMessageGetString(JUMPMessageReader* r)
{
    int len;
    
    assert(jumpMessagingInitialized != 0);
    /* FIXME: ASCII assumption for now */
    return (JUMPPlatformCString)jumpMessageGetByteArray(r, &len);
}

JUMPPlatformCString*
jumpMessageGetStringArray(JUMPMessageReader* r, uint32* lengthPtr)
{
    /* FIXME: create string array freeing routine */
    uint32 length;
    uint32 i;
    JUMPPlatformCString* strs;
    
    assert(jumpMessagingInitialized != 0);
    length = jumpMessageGetInt(r);
    strs = (JUMPPlatformCString*)calloc(length, sizeof(JUMPPlatformCString));
    if (strs == NULL) {
	return NULL;
    }
    
    for (i = 0; i < length; i++) {
	strs[i] = jumpMessageGetString(r);
    }
    
    *lengthPtr = length;
    return strs;
}

JUMPPlatformCString
jumpMessageGetType(JUMPMessage m)
{
    assert(jumpMessagingInitialized != 0);
    return m->header.type;
}

JUMPAddress*
jumpMessageGetSender(JUMPMessage m) {
    assert(jumpMessagingInitialized != 0);
    return &m->header.sender.address;
}

static void
sendAsyncOfType(JUMPAddress target, JUMPOutgoingMessage m, 
		JUMPPlatformCString type,
		JUMPMessageStatusCode* code)
{
    int targetpid = target.processId;
    JUMPMessageQueueHandle targetMq;
    
    assert(jumpMessagingInitialized != 0);
    targetMq = jumpMessageQueueOpen(targetpid, type);
    if (targetMq == NULL) {
	/* FIXME: set error code */
        *code = JUMP_TARGET_NONEXISTENT;
	return;
    }
    /* FIXME: Error check and propagate */
    jumpMessageQueueSend(targetMq, m->data, m->dataBufferLen);
    *code = 0;
}

void
jumpMessageSendAsync(JUMPAddress target, JUMPOutgoingMessage m,
		     JUMPMessageStatusCode* code)
{
    sendAsyncOfType(target, m, jumpMessageGetType(m), code);
}

void
jumpMessageSendAsyncResponse(JUMPOutgoingMessage m,
			     JUMPMessageStatusCode* code)
{
    JUMPReturnAddress target;
    
    assert(jumpMessagingInitialized != 0);
    
    target = getReturnAddress(m);
    
    /* For now, just revert to async send. ReturnAddress can contain
       more information to indicate if a special response should be sent out */
    sendAsyncOfType(target.address, m, target.returnType, code);
}

static JUMPMessage
doWaitFor(JUMPPlatformCString type, int32 timeout)
{
    int status;

    /* Now wait for response with a timeout.FIXME: How to implement timeout? */
    /* Must do a SIGALRM to interrupt receive. */
    status = jumpMessageQueueWaitForMessage(type, timeout);
    if (status == 0) {
	uint8* buffer = (uint8*)calloc(1, MESSAGE_BUFFER_SIZE);
	struct _JUMPMessage* incoming;
	if (buffer == NULL) {
	    /* FIXME: Throw out of memory */
	    return NULL;
	}
	jumpMessageQueueReceive(type, buffer, MESSAGE_BUFFER_SIZE);
	incoming = newMessageFromReceivedBuffer(buffer, MESSAGE_BUFFER_SIZE);
	return (JUMPMessage)incoming;
    } else {
	return NULL;
    }    
}

JUMPMessage
jumpMessageSendSync(JUMPAddress target, JUMPOutgoingMessage m, int32 timeout,
		    JUMPMessageStatusCode* code)
{
    JUMPMessageQueueStatusCode mqcode;
    JUMPMessage r;

    assert(jumpMessagingInitialized != 0);

    /* First, eagerly create this thread's response queue. Otherwise
       there is a race between the sendAsync and the jumpMessageWaitFor().
       A response might come back before we get a chance to create
       this queue */
    jumpMessageQueueCreate(m->header.sender.returnType, &mqcode);

    /* Outgoing */
    jumpMessageSendAsync(target, m, code);

    /* Get a response. Discard any that don't match outgoing request id */
    do {
	/* FIXME: timeout accounting! We have to keep track of how
	   long the wait was, and make sure to not wait longer than
	   the incoming timeout above. */
	r = doWaitFor(m->header.sender.returnType, timeout);
    } while (r->header.requestId != m->header.requestId);
    /* sanity? */
    assert(!strcmp(r->header.type, m->header.type));
    return r;
}

JUMPMessageHandlerRegistration
jumpMessageRegisterDirect(JUMPPlatformCString type)
{
    /* FIXME */
    return NULL;
}

/*
 * Block and wait for incoming message of a given type
 */
extern JUMPMessage
jumpMessageWaitFor(JUMPPlatformCString type,
		   int32 timeout)
{
    JUMPMessageQueueStatusCode mqcode;
    
    assert(jumpMessagingInitialized != 0);

    jumpMessageQueueCreate(type, &mqcode);
    if (mqcode != JUMP_MQ_SUCCESS) {
	return NULL;
    }
    return doWaitFor(type, timeout);
}

JUMPMessageHandlerRegistration
jumpMessageAddHandlerByType(JUMPPlatformCString type, 
			    JUMPMessageHandler handler,
			    void* data)
{
    /* FIXME: Should we even have this? I want to avoid having to create
       new threads in native code, because it seriously complicates the
       porting layer. Caller code can still do this. */
    return NULL;
}

JUMPMessageHandlerRegistration
jumpMessageAddHandlerByOutgoingMessage(JUMPOutgoingMessage m,
				       JUMPMessageHandler handler,
				       void* data)
{
    return NULL;
}

void
jumpMessageCancelRegistration(JUMPMessageHandlerRegistration r)
{
}


JUMPMessageStatusCode
jumpMessageShutdown()
{
    /*
     * Destroy all my message queues
     */
    jumpMessageQueueInterfaceDestroy();
    /*
     * Disallow calls 
     */
    jumpMessagingInitialized = 0;
    
    return JUMP_SUCCESS;
}

JUMPMessageStatusCode
jumpMessageStart()
{
    MESSAGE_DATA_OFFSET = jumpMessageQueueDataOffset();
    jumpMessagingInitialized = 1;
    return JUMP_SUCCESS;
}

JUMPMessageStatusCode
jumpMessageRestart()
{
    jumpMessagingInitialized = 1;
    return JUMP_SUCCESS;
}

/*
 * Free an incoming message.
 */
static void
freeMessage(struct _JUMPMessage* m)
{
    /* Free all component allocations, and then the message itself */
    free(m->data);
    if (m->header.sender.returnType != NULL) {
	free(m->header.sender.returnType);
    }
    /* Make sure the contents are not used accidentally */
    memset(m, 0, sizeof(struct _JUMPMessage));
    free(m);
}

/*
 * Free an outgoing message.
 * It is OK to free a message after a send call. 
 * Any entity wanting to queue a message for later send must clone it first.
 */
void
jumpMessageFreeOutgoing(JUMPOutgoingMessage m)
{
    freeMessage((struct _JUMPMessage*)m);
}


/* 
 * Clone outgoing message. Must be freed via jumpMessageFreeOutgoing() 
 */
JUMPOutgoingMessage
jumpMessageCloneOutgoing(JUMPOutgoingMessage m)
{
    return NULL;
}


/*
 * Free an incoming message.
 */
void
jumpMessageFree(JUMPMessage m)
{
    freeMessage((struct _JUMPMessage*)m);
}

/*
 * Free an incoming message.
 */
void
jumpMessageOutgoingFree(JUMPOutgoingMessage m)
{
}


/* 
 * Clone incoming message. Must be freed via jumpMessageFree() 
 */
JUMPMessage
jumpMessageClone(JUMPMessage m)
{
    return NULL;
}

/*
 * Example code
 * FIXME: Move to unit testing.
 */
int doit() 
{
    JUMPMessageStatusCode status;
    JUMPAddress executive = jumpMessageGetExecutiveAddress();
    JUMPOutgoingMessage m = jumpMessageNewOutgoingByType("message/test");

    jumpMessageAddInt(m, 5);
    jumpMessageAddByte(m, 3);
    jumpMessageAddString(m, "test");
    jumpMessageSendAsync(executive, m, &status);
    return (status == JUMP_SUCCESS);
}

JUMPMessageHandlerRegistration myTypeRegistration;

void
myMessageListener(JUMPMessage m, void* data) 
{
    JUMPPlatformCString type = jumpMessageGetType(m);
    printf("Message 0x%x of type %s received\n", (uint32)m, type);
    jumpMessageCancelRegistration(myTypeRegistration);
}

void 
registerMyListener() 
{
    myTypeRegistration = jumpMessageAddHandlerByType("mytype", 
						     myMessageListener,
						     NULL);
}

void 
registerResponseListener(JUMPOutgoingMessage m) 
{
    myTypeRegistration = 
	jumpMessageAddHandlerByOutgoingMessage(m, myMessageListener, NULL);
}

/* Emulating a message processor, sending a response */
int 
processRequest(JUMPMessage m) {
    JUMPMessageStatusCode code;
    JUMPOutgoingMessage responseMessage;
    JUMPMessageReader reader;
    int param1, param2;
    
    jumpMessageReaderInit(&reader, m);
    param1 = jumpMessageGetInt(&reader);
    param2 = jumpMessageGetInt(&reader); /* .. get other data fields .. */

    responseMessage = jumpMessageNewOutgoingByRequest(m);

    /*
     * Fill in response
     */
    jumpMessageAddInt(responseMessage, 5); /* ..... etc  ..... */
    jumpMessageSendAsyncResponse(responseMessage, &code);
    
    return (code == JUMP_SUCCESS);
}
