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
    /* The message's data */
    uint8* data;
    /* The length of the data */
    int dataBufferLen;
    /* The byte after the message's data */
    const uint8* dataEnd;
    /* Current location of the data pointer, for adding to the message */
    uint8* dataPtr;
    /* First error encountered by jumpMessageAdd...(), or JUMP_SUCCESS. */
    JUMPMessageStatusCode status;
};

struct JUMPMessageHandlerRegistration {
    JUMPPlatformCString messageType;
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
	return JUMP_NO_SUCH_QUEUE;

      case JUMP_MQ_UNBLOCKED:
	return JUMP_UNBLOCKED;

      case JUMP_MQ_TARGET_NONEXISTENT:
	return JUMP_TARGET_NONEXISTENT;

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

/* Type should be newly allocated. */
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
    assert(jumpMessagingInitialized != 0);

    return mkReturnAddr(jumpProcessGetId(),
			jumpMessageGetReturnTypeName());
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
    message->dataEnd = buffer + len;
    message->dataPtr = buffer + jumpMessageQueueDataOffset();
    message->status = JUMP_SUCCESS;
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

    buffer = calloc(1, JUMP_MESSAGE_BUFFER_SIZE);
    if (buffer == NULL) {
	return NULL;
    }

    message = newMessageFromBuffer(buffer, JUMP_MESSAGE_BUFFER_SIZE);
    if (message == NULL) {
	free(buffer);
	return NULL;
    }

    return message;
}

/*
 * Take the 'header' struct in a JUMPMessage, and "serialize" it into
 * the message data area.  On success, returns JUMP_SUCCESS.  On failure,
 * returns JUMP_OVERRUN.
 */
static JUMPMessageStatusCode
putHeaderInMessage(struct _JUMPMessage* m)
{
    struct _JUMPMessageHeader* hdr = &m->header;

    jumpMessageAddInt((JUMPOutgoingMessage)m, hdr->messageId);
    jumpMessageAddInt((JUMPOutgoingMessage)m, hdr->requestId);
    jumpMessageAddInt((JUMPOutgoingMessage)m, hdr->sender.address.processId);
    jumpMessageAddString((JUMPOutgoingMessage)m, hdr->sender.returnType);
    jumpMessageAddString((JUMPOutgoingMessage)m, hdr->type);

    return ((JUMPOutgoingMessage)m)->status;
}

/*
 * Deserialize header from message to 'header' structure in JUMPMessage.
 * Returns one of JUMP_SUCCESS, JUMP_OUT_OF_MEMORY, JUMP_OVERRUN,
 * or JUMP_NEGATIVE_ARRAY_LENGTH.
 */
static JUMPMessageStatusCode
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

    return reader.status;
}

/*
 * Given a "raw" buffer, allocated or received, make a corresponding
 * JUMPMessage.
 *
 * On success the struct _JUMPMessage is returned and *code is set to
 * JUMP_SUCCESS.  On failure, NULL is returned and *code is set to one
 * of JUMP_OUT_OF_MEMORY, JUMP_OVERRUN, or JUMP_NEGATIVE_ARRAY_LENGTH.
 * 
 */
static struct _JUMPMessage* 
newMessageFromReceivedBuffer(uint8* buffer, uint32 len,
			     JUMPMessageStatusCode *code)
{
    struct _JUMPMessage* message = newMessageFromBuffer(buffer, len);
    if (message == NULL) {
	*code = JUMP_OUT_OF_MEMORY;
	return NULL;
    }

    *code = getHeaderFromMessage(message);
    if (*code != JUMP_SUCCESS) {
	free(message);
	return NULL;
    }

    return message;
}

/*
 * Running counters for message id's and request id's
 */
/* FIXME not thread safe. */
static uint32 thisProcessMessageId;
static int32 thisProcessRequestId;

JUMPOutgoingMessage
jumpMessageNewOutgoingFromBuffer(uint8* buffer, int isResponse,
				 JUMPMessageStatusCode *code)
{
    struct _JUMPMessage* message;
    JUMPMessageMark mmarkBeforeHeader;
    JUMPMessageMark mmarkAfterHeader;
    uint32 messageId;

    message = newMessageFromBuffer(buffer, JUMP_MESSAGE_BUFFER_SIZE);
    if (message == NULL) {
	*code = JUMP_OUT_OF_MEMORY;
	return NULL;
    }

    jumpMessageMarkSet(&mmarkBeforeHeader, message);
    /* If this works, the following adds will work. */
    *code = getHeaderFromMessage(message);
    if (*code != JUMP_SUCCESS) {
	free(message);
	return NULL;
    }
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
	jumpMessageAddInt(message, requestId);
	message->header.requestId = requestId;
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

static JUMPAddress
cloneJUMPAddress(JUMPAddress address)
{
    return address;
}

static JUMPReturnAddress
cloneJUMPReturnAddress(JUMPReturnAddress returnAddress)
{
    JUMPReturnAddress newReturnAddress;
    newReturnAddress.address = cloneJUMPAddress(returnAddress.address);
    newReturnAddress.returnType = strdup(returnAddress.returnType);
    return newReturnAddress;
}

static void
freeJUMPAddress(JUMPAddress address)
{
    /* Nothing to do. */
}

static void
freeJUMPReturnAddress(JUMPReturnAddress returnAddress)
{
    freeJUMPAddress(returnAddress.address);
    free(returnAddress.returnType);
}

/*
 * Free an incoming message.
 */
static void
freeMessage(struct _JUMPMessage* m)
{
    /* Free all component allocations, and then the message itself */
    free(m->data);
    freeJUMPReturnAddress(m->header.sender);
    free(m->header.type);
    /* Make sure the contents are not used accidentally */
    memset(m, 0, sizeof(struct _JUMPMessage));
    free(m);
}

/* type and addr are copied, and freed with the JUMPOutgoingMessage.
   On success, *code is set to JUMP_SUCCESS.  Otherwise it is set
   to one of JUMP_OUT_OF_MEMORY or JUMP_OVERRUN. */
static JUMPOutgoingMessage
newOutgoingMessage(JUMPPlatformCString type, uint32 requestId, 
		   JUMPReturnAddress addr, JUMPMessageStatusCode *code)
{
    struct _JUMPMessage* message;
    assert(jumpMessagingInitialized != 0);
    message = newMessage();
    if (message == NULL) {
	*code = JUMP_OUT_OF_MEMORY;
	goto fail;
    }
    message->header.messageId = thisProcessMessageId++;
    message->header.requestId = requestId;
    message->header.sender = cloneJUMPReturnAddress(addr);
    if (message->header.sender.returnType == NULL) {
	*code = JUMP_OUT_OF_MEMORY;
	goto fail;
    }
    message->header.type = strdup(type);
    if (message->header.type == NULL) {
	*code = JUMP_OUT_OF_MEMORY;
	goto fail;
    }
    *code = putHeaderInMessage(message);
    if (*code != JUMP_SUCCESS) {
	goto fail;
    }
    return (JUMPOutgoingMessage)message;

  fail:
    if (message != NULL) {
	freeMessage(message);
    }
    return NULL;
}

JUMPOutgoingMessage
jumpMessageNewOutgoingByType(JUMPPlatformCString type,
			     JUMPMessageStatusCode *code)
{
    uint32 requestId = thisProcessRequestId++;
    JUMPReturnAddress myReturnAddress;
    JUMPOutgoingMessage message;

    assert(jumpMessagingInitialized != 0);

    myReturnAddress = getMyReturnAddress();    
    message = newOutgoingMessage(type, requestId, myReturnAddress, code);
    freeJUMPReturnAddress(myReturnAddress);

    return message;
}

JUMPOutgoingMessage
jumpMessageNewOutgoingByRequest(JUMPMessage requestMessage,
			     JUMPMessageStatusCode *code)
{
    uint32 requestId = requestMessage->header.requestId;
    assert(jumpMessagingInitialized != 0);
    return newOutgoingMessage(jumpMessageGetType(requestMessage), requestId, 
			      getReturnAddress(requestMessage),
			      code);
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
    if (m->status != JUMP_SUCCESS) {
	return;
    }
    if (m->dataEnd - m->dataPtr < 1) {
	m->status = JUMP_OVERRUN;
	return;
    }
    m->dataPtr[0] = value;
    m->dataPtr += 1;
}

void
jumpMessageAddBytesFrom(JUMPOutgoingMessage m, const int8* values, int length)
{
    assert(jumpMessagingInitialized != 0);
    if (m->status != JUMP_SUCCESS) {
	return;
    }
    if ((values == NULL) || (length == 0)) {
	return;
    }
    if (length < 0) {
	m->status = JUMP_NEGATIVE_ARRAY_LENGTH;
	return;
    }
    if (m->dataEnd - m->dataPtr < length) {
	m->status = JUMP_OVERRUN;
	return;
    }
    memcpy(m->dataPtr, values, length);
    m->dataPtr += length;
}

void
jumpMessageAddByteArray(JUMPOutgoingMessage m, const int8* values, int length)
{
    assert(jumpMessagingInitialized != 0);

    if (m->status != JUMP_SUCCESS) {
	return;
    }
    if (values == NULL) {
	jumpMessageAddInt(m, -1);
	return;
    }
    if (length < 0) {
	m->status = JUMP_NEGATIVE_ARRAY_LENGTH;
	return;
    }
    jumpMessageAddInt(m, length);
    if (m->status != JUMP_SUCCESS) {
	return;
    }
    if (m->dataEnd - m->dataPtr < length) {
	m->status = JUMP_OVERRUN;
	return;
    }
    memcpy(m->dataPtr, values, length);
    m->dataPtr += length;
}

void
jumpMessageAddInt(JUMPOutgoingMessage m, int32 value)
{
    uint32 v;
    assert(jumpMessagingInitialized != 0);
    if (m->status != JUMP_SUCCESS) {
	return;
    }
    if (m->dataEnd - m->dataPtr < 4) {
	m->status = JUMP_OVERRUN;
	return;
    }
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
    if (m->status != JUMP_SUCCESS) {
	return;
    }
    if (m->dataEnd - m->dataPtr < 2) {
	m->status = JUMP_OVERRUN;
	return;
    }
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
    if (m->status != JUMP_SUCCESS) {
	return;
    }
    if (m->dataEnd - m->dataPtr < 8) {
	m->status = JUMP_OVERRUN;
	return;
    }
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
    if (m->status != JUMP_SUCCESS) {
	return;
    }
    if (strs == NULL) {
	jumpMessageAddInt(m, -1);
	return;
    }
    if (length < 0) {
	m->status = JUMP_NEGATIVE_ARRAY_LENGTH;
	return;
    }
    jumpMessageAddInt(m, length);
    if (m->status != JUMP_SUCCESS) {
	return;
    }
    for (i = 0; i < length; i++) {
	jumpMessageAddString(m, strs[i]);
	if (m->status != JUMP_SUCCESS) {
	    return;
	}
    }
}

JUMPMessageStatusCode
jumpMessageGetStatus(JUMPOutgoingMessage m)
{
    return m->status;
}

/*
 * An iterator to read off of a message
 */
void
jumpMessageReaderInit(JUMPMessageReader* r, JUMPMessage m)
{
    assert(jumpMessagingInitialized != 0);
    r->ptr = m->dataPtr;
    r->ptrEnd = m->dataEnd;
    r->status = JUMP_SUCCESS;
}

int8
jumpMessageGetByte(JUMPMessageReader* r)
{
    int8 ret;

    assert(jumpMessagingInitialized != 0);

    if (r->status != JUMP_SUCCESS) {
	return 0;
    }

    if (r->ptrEnd - r->ptr < 1) {
	r->status = JUMP_OVERRUN;
	return 0;
    }

    ret = r->ptr[0];
    r->ptr += 1;
    return ret;
}

int8*
jumpMessageGetBytesInto(JUMPMessageReader* r, int8* buffer, uint32 length) {
    if (r->status != JUMP_SUCCESS) {
	return NULL;
    }

    if (r->ptrEnd - r->ptr < length) {
	r->status = JUMP_OVERRUN;
	return NULL;
    }

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

    if (r->status != JUMP_SUCCESS) {
	return NULL;
    }

    length = jumpMessageGetInt(r);
    if (r->status != JUMP_SUCCESS) {
	return NULL;
    }

    *lengthPtr = length;

    if (length == -1) {
	/* NULL array was written, this is ok. */
	return NULL;
    }

    if (length < 0) {
	r->status = JUMP_NEGATIVE_ARRAY_LENGTH;
	return NULL;
    }

    if (r->ptrEnd - r->ptr < length) {
	r->status = JUMP_OVERRUN;
	return NULL;
    }

    bytearray = calloc(1, length);
    if (bytearray == NULL) {
	/* Caller discards message? Or do we "rewind" to the start of the
	   length field again? */
	r->status = JUMP_OUT_OF_MEMORY;
	return NULL;
    }
    
    memcpy(bytearray, r->ptr, length);
    r->ptr += length;
    
    return bytearray;
}

int32
jumpMessageGetInt(JUMPMessageReader* r)
{
    int32 i;

    assert(jumpMessagingInitialized != 0);

    if (r->status != JUMP_SUCCESS) {
	return 0;
    }

    if (r->ptrEnd - r->ptr < 4) {
	r->status = JUMP_OVERRUN;
	return 0;
    }

    i = (int32)
	(((uint8)r->ptr[0] << 24) | 
	 ((uint8)r->ptr[1] << 16) | 
	 ((uint8)r->ptr[2] << 8) | 
	  (uint8)r->ptr[3]);

    r->ptr += 4;
    return i;
}

int16
jumpMessageGetShort(JUMPMessageReader* r)
{
    int16 i;

    assert(jumpMessagingInitialized != 0);

    if (r->status != JUMP_SUCCESS) {
	return 0;
    }

    if (r->ptrEnd - r->ptr < 2) {
	r->status = JUMP_OVERRUN;
	return 0;
    }

    i = (int16)
	(((uint8)r->ptr[0] << 8) | 
	  (uint8)r->ptr[1]);
    r->ptr += 2;
    return i;
}

int64
jumpMessageGetLong(JUMPMessageReader* r)
{
    int64 i;

    assert(jumpMessagingInitialized != 0);

    if (r->status != JUMP_SUCCESS) {
	return 0;
    }

    if (r->ptrEnd - r->ptr < 8) {
	r->status = JUMP_OVERRUN;
	return 0;
    }

    i = (int64)
	(((uint64)r->ptr[0] << 56) | 
	 ((uint64)r->ptr[1] << 48) | 
	 ((uint64)r->ptr[2] << 40) | 
	 ((uint64)r->ptr[3] << 32) |
     ((uint64)r->ptr[4] << 24) | 
	 ((uint64)r->ptr[5] << 16) | 
	 ((uint64)r->ptr[6] <<  8) | 
	  (uint64)r->ptr[7]);
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
    uint32 length;
    uint32 i;
    JUMPPlatformCString* strs;
    
    assert(jumpMessagingInitialized != 0);

    if (r->status != JUMP_SUCCESS) {
	return NULL;
    }

    length = jumpMessageGetInt(r);
    if (r->status != JUMP_SUCCESS) {
	return NULL;
    }

    *lengthPtr = length;

    if (length == -1) {
	/* NULL array was written, this is ok. */
	return NULL;
    }

    if (length < 0) {
	r->status = JUMP_NEGATIVE_ARRAY_LENGTH;
	return NULL;
    }

    strs = calloc(length, sizeof(JUMPPlatformCString));
    if (strs == NULL) {
	r->status = JUMP_OUT_OF_MEMORY;
	return NULL;
    }

    for (i = 0; i < length; i++) {
	strs[i] = jumpMessageGetString(r);
	if (r->status != JUMP_SUCCESS) {
	    jumpMessageFreeStringArray(strs, i);
	    return NULL;
	}
    }
    
    return strs;
}

void
jumpMessageFreeStringArray(JUMPPlatformCString* p, uint32 length)
{
    uint32 i;

    /* jumpMessageGetStringArray() may validly return NULL, so
       accept it here. */

    if (p == NULL) {
	return;
    }

    for (i = 0; i < length; i++) {
	free(p[i]);
    }
    free(p);
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
 * JUMP_TIMEOUT, JUMP_OVERRUN, JUMP_NEGATIVE_ARRAY_LENGTH,
 * JUMP_NO_SUCH_QUEUE, JUMP_UNBLOCKED, or JUMP_FAILURE.
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

    buffer = calloc(1, JUMP_MESSAGE_BUFFER_SIZE);
    if (buffer == NULL) {
	*code = JUMP_OUT_OF_MEMORY;
	return NULL;
    }

    status = jumpMessageQueueReceive(
	type, buffer, JUMP_MESSAGE_BUFFER_SIZE, &mqcode);
    if (status == -1) {
	*code = translateJumpMessageQueueStatusCode(&mqcode);
	free(buffer);
	return NULL;
    }

    incoming = newMessageFromReceivedBuffer(buffer, JUMP_MESSAGE_BUFFER_SIZE, code);
    if (incoming == NULL) {
	free(buffer);
	return NULL;
    }

    return (JUMPMessage)incoming;
}

JUMPMessage
jumpMessageSendSync(JUMPAddress target, JUMPOutgoingMessage m, int32 timeout,
		    JUMPMessageStatusCode* code)
{
    JUMPMessageHandlerRegistration registration = NULL;
    JUMPMessage r = NULL;

    assert(jumpMessagingInitialized != 0);

    /* Register the message type before sending the message to ensure
       the queue exists before the recipient sends a message to it. */

    registration =
	jumpMessageRegisterDirect(m->header.sender.returnType, code);
    if (registration == NULL) {
	goto out;
    }

    jumpMessageSendAsync(target, m, code);
    if (*code != JUMP_SUCCESS) {
	goto out;
    }

    /* Get a response. Discard any that don't match outgoing request id. */
    /* FIXME This is no good, each call to doWaitFor() gets a new timeout.
       doWaitFor() should use a deadline, not a timeout. */
    do {
	r = doWaitFor(m->header.sender.returnType, timeout, code);
    } while (r != NULL && r->header.requestId != m->header.requestId);

    /* sanity? */
    if (r != NULL) {
	assert(!strcmp(r->header.type, m->header.type));
    }

  out:
    if (registration != NULL) {
	jumpMessageCancelRegistration(registration);
    }
    return r;
}

JUMPMessageHandlerRegistration
jumpMessageRegisterDirect(JUMPPlatformCString type,
			  JUMPMessageStatusCode *code)
{
    JUMPMessageHandlerRegistration registration;
    JUMPMessageQueueStatusCode mqcode;

    registration = malloc(sizeof(*registration));
    if (registration == NULL) {
	*code = JUMP_OUT_OF_MEMORY;
	goto fail;
    }

    registration->messageType = strdup(type);
    if (registration->messageType == NULL) {
	*code = JUMP_OUT_OF_MEMORY;
	goto fail;
    }

    jumpMessageQueueCreate(registration->messageType, &mqcode);
    if (mqcode != JUMP_MQ_SUCCESS) {
	*code = translateJumpMessageQueueStatusCode(&mqcode);
	goto fail;
    }

    *code = JUMP_SUCCESS;
    return registration;

  fail:
    if (registration != NULL) {
	free(registration->messageType);
	free(registration);
    }
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
    assert(jumpMessagingInitialized != 0);

    return doWaitFor(type, timeout, code);
}

void
jumpMessageUnblock(JUMPPlatformCString messageType,
		   JUMPMessageStatusCode* code)
{
    JUMPMessageQueueStatusCode mqcode;

    assert(jumpMessagingInitialized != 0);

    jumpMessageQueueUnblock(messageType, &mqcode);
    *code = translateJumpMessageQueueStatusCode(&mqcode);
}


int
jumpMessageGetFd(JUMPPlatformCString type)
{
    return jumpMessageQueueGetFd(type);
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
    jumpMessageQueueDestroy(r->messageType);
    free(r->messageType);
    free(r);
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
    /* Ensure the porting layer can handle messages of the size we need. */
    assert(JUMP_MESSAGE_BUFFER_SIZE <= JUMP_MESSAGE_QUEUE_MAX_MESSAGE_SIZE);

    return JUMP_SUCCESS;
}

JUMPMessageStatusCode
jumpMessageRestart(void)
{
    return JUMP_SUCCESS;
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
    JUMPOutgoingMessage m =
	jumpMessageNewOutgoingByType("message/test", &status);
    if (m == NULL) {
	return JUMP_FAILURE;
    }

    jumpMessageAddInt(m, 5);
    jumpMessageAddByte(m, 3);
    jumpMessageAddString(m, "test");
    if (m->status != JUMP_SUCCESS) {
	return JUMP_FAILURE;
    }
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

    responseMessage = jumpMessageNewOutgoingByRequest(m, &code);

    /*
     * Fill in response
     */
    jumpMessageAddInt(responseMessage, 5); /* ..... etc  ..... */
    jumpMessageSendAsyncResponse(responseMessage, &code);
    
    return (code == JUMP_SUCCESS);
}
