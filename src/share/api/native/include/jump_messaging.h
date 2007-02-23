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

/*
 * This is the api and OS-independent implementation of native
 * messaging in JUMP.
 */
#ifndef __JUMP_MESSAGING_H
#define __JUMP_MESSAGING_H

#include "porting/JUMPTypes.h"

/*
 * FIXME: This should be part of the message porting layer,
 * determining the buffer size to allocate for each incoming message
 * Also, there should be a re-allocation increment, and a maximum.
 */
#define MESSAGE_BUFFER_SIZE 512

/**
 * @brief identifies 
 */
typedef enum {
    JUMP_TIMEOUT  = 2,
    JUMP_SUCCESS  = 3,
    JUMP_FAILURE  = 4,
    JUMP_OUT_OF_MEMORY = 5,
    JUMP_WOULD_BLOCK = 6,
    JUMP_OVERRUN = 7,
    JUMP_NEGATIVE_ARRAY_LENGTH = 8
} JUMPMessageStatusCode;

/*
 * @brief the designation of an end point
 */
typedef struct {
    uint32 processId;
} JUMPAddress;

/*
 * @brief representation of a message
 */
typedef struct _JUMPMessage* JUMPMessage;

/*
 * @brief representation of an outgoing message
 * FIXME: There may have to be a distinction between these two
 * just like in the Java layer.
 */
typedef JUMPMessage JUMPOutgoingMessage;

/*
 * Addressing related api's
 */
extern JUMPAddress 
jumpMessageGetMyAddress(void);

extern JUMPAddress 
jumpMessageGetExecutiveAddress(void);

extern char*
jumpMessageGetReturnTypeName(void);

/*
 * Message creation api's
 */
extern JUMPOutgoingMessage
jumpMessageNewOutgoingByType(JUMPPlatformCString type);

extern JUMPOutgoingMessage
jumpMessageNewOutgoingByRequest(JUMPMessage requestMessage);

/*
 * Free an outgoing message.
 * It is OK to free a message after a send call. 
 * Any entity wanting to queue a message for later send must clone it first.
 */
extern void
jumpMessageFreeOutgoing(JUMPOutgoingMessage m);

/* 
 * Clone outgoing message. Must be freed via jumpMessageFreeOutgoing() 
 */
extern JUMPOutgoingMessage
jumpMessageCloneOutgoing(JUMPOutgoingMessage m);

/*
 * Free an incoming message.
 */
extern void
jumpMessageFree(JUMPMessage m);

/* 
 * Clone incoming message. Must be freed via jumpMessageFree() 
 */
extern JUMPMessage
jumpMessageClone(JUMPMessage m);

typedef struct {
    uint8* ptr;
    /* Applies to outgoing and incoming messages */
    struct _JUMPMessage* m;
} JUMPMessageMark;

/*
 * Applies to both incoming and outgoing messages
 */
extern void
jumpMessageMarkSet(JUMPMessageMark* mmark, struct _JUMPMessage* m);

extern void
jumpMessageMarkResetTo(JUMPMessageMark* mmark, struct _JUMPMessage* m);

/*
 * Message data write api's
 */

/*
 * Bits representing the return value of the jumpMessageAdd...()
 * functions.
 */
enum {
    JUMP_ADD_OVERRUN = 1,
    JUMP_ADD_NEGATIVE_ARRAY_LENGTH = 2
};

extern int
jumpMessageAddByte(JUMPOutgoingMessage m, int8 value);

extern int
jumpMessageAddByteArray(JUMPOutgoingMessage m, const int8* values, int length);

extern int
jumpMessageAddShort(JUMPOutgoingMessage m, int16 value);

extern int
jumpMessageAddInt(JUMPOutgoingMessage m, int32 value);

extern int
jumpMessageAddString(JUMPOutgoingMessage m, JUMPPlatformCString str);

extern int
jumpMessageAddStringArray(JUMPOutgoingMessage m,
			  JUMPPlatformCString* strs,
			  uint32 length);

/*
 * Message data read api's
 */

typedef struct {
    uint8* ptr;
    const uint8* ptrEnd;
    JUMPMessageStatusCode status;
} JUMPMessageReader;

extern void
jumpMessageReaderInit(JUMPMessageReader* r, JUMPMessage m);

extern int8
jumpMessageGetByte(JUMPMessageReader* r);

/*
 * The caller should call free() on the return value once it is done.
 * Sets *length to the number of bytes in the array, or -1 if the array
 * was NULL.  A NULL return value with *length != -1 indicates out
 * of memory.
 */
extern int8*
jumpMessageGetByteArray(JUMPMessageReader* r, uint32* length);

extern int16
jumpMessageGetShort(JUMPMessageReader* r);

extern int32
jumpMessageGetInt(JUMPMessageReader* r);

/*
 * The caller should call free() on the return value once it is done
 */
extern JUMPPlatformCString
jumpMessageGetString(JUMPMessageReader* r);

/*
 * The caller should call free() on the return value once it is done
 * Sets *length to the number of strings in the array, or -1 if the array
 * was NULL.  A NULL return value with *length != -1 indicates out
 * of memory.
 */
extern JUMPPlatformCString*
jumpMessageGetStringArray(JUMPMessageReader* r, uint32* length);

/*
 * Message header inspection
 */
extern JUMPPlatformCString
jumpMessageGetType(JUMPMessage m);

/*
 * Message send api's.
 *
 * Once a send call returns, the caller can call
 * jumpMessageFreeOutgoing(m).  If the implementation queues up the
 * outgoing message for later delivery, it's supposed to call
 * jumpMessageCloneOutgoing() first.
 */

/*
 * jumpMessageSendAsync() does not block. If the message cannot be sent
 * out, a proper error code is returned immediately.
 *
 * On return, sets *code to one of JUMP_SUCCESS, JUMP_OUT_OF_MEMORY,
 * JUMP_WOULD_BLOCK, or JUMP_FAILURE.
 */
extern void
jumpMessageSendAsync(JUMPAddress target, JUMPOutgoingMessage m,
		     JUMPMessageStatusCode* code);

/*
 * jumpMessageSendAsyncResponse() is to be used when a response is being sent
 * out to an incoming message. The only way to respond to an incoming message
 * is by sending such an asynchronous response.
 *
 * This call does not block. If the message cannot be sent
 * out, a proper error code is returned immediately.
 *
 * On return, sets *code to one of JUMP_SUCCESS, JUMP_OUT_OF_MEMORY,
 * JUMP_WOULD_BLOCK, JUMP_TIMEOUT, or JUMP_FAILURE.
 */
extern void
jumpMessageSendAsyncResponse(JUMPOutgoingMessage m,
			     JUMPMessageStatusCode* code);

/*
 * On return, sets *code to one of JUMP_SUCCESS, JUMP_OUT_OF_MEMORY,
 * JUMP_WOULD_BLOCK, or JUMP_FAILURE.
 */
extern JUMPMessage
jumpMessageSendSync(JUMPAddress target, JUMPOutgoingMessage m, int32 timeout,
		    JUMPMessageStatusCode* code);

/*
 * Incoming message handling
 */

/*
 * Each incoming message handled via a call to the registered 
 * JUMPMessageHandler.
 *
 * The message handler registration system is responsible for freeing
 * up a message after all its handlers have been called. This means
 * that if a handler wants to hang onto its JUMPMessage after its
 * handler has returned, it is to call jumpMessageClone() on it. There
 * is no guarantee that the reference to the JUMPMessage will be valid
 * past the return point of the JUMPMessageHandler.
 */
typedef void (*JUMPMessageHandler)(JUMPMessage m, void* data);
typedef void* JUMPMessageHandlerRegistration;

/*
 * Listening to messages directly
 */

/*
 * Register 'type' for direct listening
 */
extern JUMPMessageHandlerRegistration
jumpMessageRegisterDirect(JUMPPlatformCString type);

/*
 * Block and wait for incoming message of a given type
 *
 * On return, sets *code to one of JUMP_SUCCESS, JUMP_OUT_OF_MEMORY,
 * JUMP_TIMEOUT, or JUMP_FAILURE.
 */
extern JUMPMessage
jumpMessageWaitFor(JUMPPlatformCString type,
		   int32 timeout,
		   JUMPMessageStatusCode *code);

/*
 * Registration for callback based message handling
 */
extern JUMPMessageHandlerRegistration
jumpMessageAddHandlerByType(JUMPPlatformCString type, 
			    JUMPMessageHandler handler,
			    void* data);

extern JUMPMessageHandlerRegistration
jumpMessageAddHandlerByOutgoingMessage(JUMPOutgoingMessage m,
				       JUMPMessageHandler handler,
				       void* data);

extern void
jumpMessageCancelRegistration(JUMPMessageHandlerRegistration r);

/*
 * Messaging system shutdown,start and re-start
 */
extern JUMPMessageStatusCode
jumpMessageShutdown(void);

extern JUMPMessageStatusCode
jumpMessageStart(void);

extern JUMPMessageStatusCode
jumpMessageRestart(void);

/* Raw buffer operations */
/*
 * Create an outgoing message from a buffer that's been filled elsewhere
 */
extern JUMPOutgoingMessage
jumpMessageNewOutgoingFromBuffer(uint8* buffer, int isResponse);

/*
 * Get raw buffer of message
 */
extern uint8*
jumpMessageGetData(JUMPMessage message);

/*
 * Command handling -- TODO
 */
#endif /* __JUMP_MESSAGING_H */
