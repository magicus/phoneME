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


#define jumpMessagingInitialized (1)

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

static JUMPMessageStatusCode
translateJumpMessageQueueStatusCode(const JUMPMessageQueueStatusCode *mqcode)
{
    switch (*mqcode) {
      case JUMP_MQ_TIMEOUT:
	return JUMP_TIMEOUT;

      case JUMP_MQ_BUFFER_SMALL:
	return JUMP_FAILURE;

      case JUMP_MQ_SUCCESS:
	return JUMP_SUCCESS;

      case JUMP_MQ_FAILURE:
	return JUMP_FAILURE;

      case JUMP_MQ_OUT_OF_MEMORY:
	return JUMP_OUT_OF_MEMORY;

      case JUMP_MQ_BAD_MESSAGE_SIZE:
	return JUMP_FAILURE;

      case JUMP_MQ_WOULD_BLOCK:
	return JUMP_WOULD_BLOCK;

      case JUMP_MQ_NO_SUCH_QUEUE:
	return JUMP_FAILURE;

      default:
	assert(0);
	return JUMP_FAILURE;
    }
}

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
jumpMessageGetMyAddress(void)
{
    assert(jumpMessagingInitialized != 0);
    return mkAddr(jumpProcessGetId());
}

/*
 * This is how message queues are named, based on pid, and a name
 */
#define JUMP_RESPONSE_QUEUE_NAME_PATTERN "<response-thread-%d>"

char*
jumpMessageGetReturnTypeName(void)
{
    char name[80];

    assert(jumpMessagingInitialized != 0);

    snprintf(name, sizeof(name), JUMP_RESPONSE_QUEUE_NAME_PATTERN,
	     jumpThreadGetId());

    return strdup(name);
}

static JUMPReturnAddress
getMyReturnAddress(void)
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
jumpMessageGetExecutiveAddress(void)
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
    struct _JUMPMessage* message = calloc(1, sizeof(struct _JUMPMessage));
    if (message == NULL) {
	return NULL;
    }
    message->data = buffer;
    message->dataBufferLen = len;
    message->dataPtr = buffer + jumpMessageQueueDataOffset();
    return message;
}

/*
 * Create a new blank message
 */
static struct _JUMPMessage* 
newMessage(void)
{
    uint8* buffer;
    struct _JUMPMessage* message;

    buffer = calloc(1, MESSAGE_BUFFER_SIZE);
    if (buffer == NULL) {
	return NULL;
    }

    message = newMessageFromBuffer(buffer, MESSAGE_BUFFER_SIZE);
    if (message == NULL) {
	free(buffer);
	return NULL;
    }

    return message;
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
    struct _JUMPMessage* message = newMessageFromBuffer(buffer, len);
    if (message == NULL) {
	return NULL;
    }

    getHeaderFromMessage(message);
    return message;
}

/*
 * Running counters for message id's and request id's
 */
/* XXX not thread safe. */
static uint32 thisProcessMessageId;
static int32 thisProcessRequestId;

JUMPOutgoingMessage
jumpMessageNewOutgoingFromBuffer(uint8* buffer, int isResponse)
{
    struct _JUMPMessage* message;
    JUMPMessageMark mmarkBeforeHeader;
    JUMPMessageMark mmarkAfterHeader;
    uint32 messageId;

    message = newMessageFromBuffer(buffer, MESSAGE_BUFFER_SIZE);
    if (message == NULL) {
	return NULL;
    }

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
jumpMessageAddByteArray(JUMPOutgoingMessage m, const int8* values, int length)
{
    assert(jumpMessagingInitialized != 0);
    /* FIXME: capacity check! */
    if (values == NULL) {
	jumpMessageAddInt(m, -1);
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
    if (strs == NULL) {
	jumpMessageAddInt(m, -1);
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

/* NULL array -> NULL return + length == -1. */
int8*
jumpMessageGetByteArray(JUMPMessageReader* r, uint32* lengthPtr)
{
    int8* bytearray;
    uint32 length;
    
    assert(jumpMessagingInitialized != 0);
    length = jumpMessageGetInt(r);
    *lengthPtr = length;
    if (length == -1) {
	return NULL;
    }
    bytearray = calloc(1, length);
    if (bytearray == NULL) {
	/* Caller discards message? Or do we "rewind" to the start of the
	   length field again? */
	return NULL;
    }
    
    memcpy(bytearray, r->ptr, length);
    r->ptr += length;
    
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

/* XXX Need to be able to distinguish null string from out of memory. */
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
    *lengthPtr = length;

    if (length == -1) {
	return NULL;
    }

    strs = calloc(length, sizeof(JUMPPlatformCString));
    if (strs == NULL) {
	return NULL;
    }
    
    for (i = 0; i < length; i++) {
	/* XXX NULL vs. out of memory. */
	strs[i] = jumpMessageGetString(r);
    }
    
    return strs;
}

JUMPPlatformCString
jumpMessageGetType(JUMPMessage m)
{
    assert(jumpMessagingInitialized != 0);
    return m->header.type;
}

static void
sendAsyncOfType(JUMPAddress target, JUMPOutgoingMessage m, 
		JUMPPlatformCString type,
		JUMPMessageStatusCode* code)
{
    int targetpid = target.processId;
    JUMPMessageQueueHandle targetMq;
    JUMPMessageQueueStatusCode mqcode;
    
    assert(jumpMessagingInitialized != 0);
    targetMq = jumpMessageQueueOpen(targetpid, type, &mqcode);
    if (targetMq == NULL) {
	goto out;
    }
    jumpMessageQueueSend(targetMq, m->data, m->dataBufferLen, &mqcode);
    jumpMessageQueueClose(targetMq);
  out:
    *code = translateJumpMessageQueueStatusCode(&mqcode);
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

/*
 * On return, sets *code to one of JUMP_SUCCESS, JUMP_OUT_OF_MEMORY,
 * JUMP_TIMEOUT, or JUMP_FAILURE.
 */
static JUMPMessage
doWaitFor(JUMPPlatformCString type, int32 timeout, JUMPMessageStatusCode *code)
{
    int status;
    JUMPMessageQueueStatusCode mqcode;
    uint8* buffer;
    struct _JUMPMessage* incoming;

    status = jumpMessageQueueWaitForMessage(type, timeout, &mqcode);
    if (status != 0) {
	/* Timed out, or in error. Must indicate to caller so it can decide
	   which exception to throw (in case of Java), or what error code
	   to handle (in case of native). */
	*code = translateJumpMessageQueueStatusCode(&mqcode);
	return NULL;
    }

    buffer = calloc(1, MESSAGE_BUFFER_SIZE);
    if (buffer == NULL) {
	*code = JUMP_OUT_OF_MEMORY;
	return NULL;
    }

    status = jumpMessageQueueReceive(
	type, buffer, MESSAGE_BUFFER_SIZE, &mqcode);
    if (status != 0) {
	free(buffer);
	*code = translateJumpMessageQueueStatusCode(&mqcode);
	return NULL;
    }

    incoming = newMessageFromReceivedBuffer(buffer, MESSAGE_BUFFER_SIZE);
    if (incoming == NULL) {
	free(buffer);
	*code = JUMP_OUT_OF_MEMORY;
	return NULL;
    }

    return (JUMPMessage)incoming;
}

JUMPMessage
jumpMessageSendSync(JUMPAddress target, JUMPOutgoingMessage m, int32 timeout,
		    JUMPMessageStatusCode* code)
{
    JUMPMessageQueueStatusCode mqcode;
    JUMPMessage r;

    assert(jumpMessagingInitialized != 0);

    /* Create the message queue before sending the message to ensure
       it exists before the recipient sends a message to it. */
    jumpMessageQueueCreate(m->header.sender.returnType, &mqcode);
    if (mqcode != JUMP_MQ_SUCCESS) {
	*code = translateJumpMessageQueueStatusCode(&mqcode);
	return NULL;
    }

    jumpMessageSendAsync(target, m, code);
    if (*code != JUMP_SUCCESS) {
	r = NULL;
	goto out;
    }

    /* Get a response. Discard any that don't match outgoing request id. */
    /* XXX This is no good, each call to doWaitFor() gets a new timeout.
       doWaitFor() should use a deadline, not a timeout. */
    do {
	r = doWaitFor(m->header.sender.returnType, timeout, code);
    } while (r != NULL && r->header.requestId != m->header.requestId);

    /* sanity? */
    if (r != NULL) {
	assert(!strcmp(r->header.type, m->header.type));
    }

  out:
    jumpMessageQueueDestroy(m->header.sender.returnType);
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
JUMPMessage
jumpMessageWaitFor(JUMPPlatformCString type,
		   int32 timeout,
		   JUMPMessageStatusCode *code)
{
    JUMPMessageQueueStatusCode mqcode;
    
    assert(jumpMessagingInitialized != 0);

    /* XXX destroy this later?  Or shouldn't this be part of register? */
    jumpMessageQueueCreate(type, &mqcode);
    if (mqcode != JUMP_MQ_SUCCESS) {
	*code = translateJumpMessageQueueStatusCode(&mqcode);
	return NULL;
    }
    return doWaitFor(type, timeout, code);
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
jumpMessageShutdown(void)
{
    /*
     * Destroy all my message queues
     */
    jumpMessageQueueInterfaceDestroy();
    return JUMP_SUCCESS;
}

JUMPMessageStatusCode
jumpMessageStart(void)
{
    return JUMP_SUCCESS;
}

JUMPMessageStatusCode
jumpMessageRestart(void)
{
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
int doit(void)
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
registerMyListener(void)
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
