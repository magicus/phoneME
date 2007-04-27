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

/* Messages at this level are always JUMP_MESSAGE_BUFFER_SIZE bytes,
   even though the porting layer allows variable length messages, and
   possibly longer messages.  Clients can rely on being able to send
   messages of this length.  There is an assert in jumpMessageStart to
   ensure JUMP_MESSAGE_BUFFER_SIZE <= JUMP_MESSAGE_QUEUE_MAX_MESSAGE_SIZE.

   JUMP_MESSAGE_BUFFER_SIZE is currently equal to the minimum of any
   porting layer's JUMP_MESSAGE_QUEUE_MAX_MESSAGE_SIZE.  Reducing it
   could have consequences for clients, and increasing it would have
   consequences for porting layers. */

#define JUMP_MESSAGE_BUFFER_SIZE 4092

/**
 * @brief identifies 
 */
typedef enum {
    JUMP_TARGET_NONEXISTENT    = 1,
    JUMP_TIMEOUT  = 2,
    JUMP_SUCCESS  = 3,
    JUMP_FAILURE  = 4,
    JUMP_OUT_OF_MEMORY = 5,
    JUMP_WOULD_BLOCK = 6,
    JUMP_OVERRUN = 7,
    JUMP_NEGATIVE_ARRAY_LENGTH = 8,
    JUMP_UNBLOCKED = 9,
    JUMP_NO_SUCH_QUEUE = 10
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
/*
 * On success, *code is set to JUMP_SUCCESS.  Otherwise it is set
 *   to one of JUMP_OUT_OF_MEMORY or JUMP_OVERRUN.
 */
extern JUMPOutgoingMessage
jumpMessageNewOutgoingByType(JUMPPlatformCString type,
			     JUMPMessageStatusCode *code);

/*
 * On success, *code is set to JUMP_SUCCESS.  Otherwise it is set
 *   to one of JUMP_OUT_OF_MEMORY or JUMP_OVERRUN.
 */
extern JUMPOutgoingMessage
jumpMessageNewOutgoingByRequest(JUMPMessage requestMessage,
				JUMPMessageStatusCode *code);

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
extern void
jumpMessageAddByte(JUMPOutgoingMessage m, int8 value);

extern void
jumpMessageAddBytesFrom(JUMPOutgoingMessage m, const int8* values, int length);

extern void
jumpMessageAddByteArray(JUMPOutgoingMessage m, const int8* values, int length);

extern void
jumpMessageAddShort(JUMPOutgoingMessage m, int16 value);

extern void
jumpMessageAddInt(JUMPOutgoingMessage m, int32 value);

extern void
jumpMessageAddLong(JUMPOutgoingMessage m, int64 value);

extern void
jumpMessageAddString(JUMPOutgoingMessage m, JUMPPlatformCString str);

extern void
jumpMessageAddStringArray(JUMPOutgoingMessage m,
			  JUMPPlatformCString* strs,
			  uint32 length);

/*
 * Returns the message's status, which will be JUMP_SUCCESS if all
 * calls to jumpMessageAdd...() have succeeded, otherwise will be one
 * of JUMP_OVERRUN or JUMP_NEGATIVE_ARRAY_LENGTH.
 */
extern JUMPMessageStatusCode
jumpMessageGetStatus(JUMPOutgoingMessage m);

/*
 * Message data read api's
 */

typedef struct {
    uint8* ptr;
    const uint8* ptrEnd;
    JUMPMessageStatusCode status;
} JUMPMessageReader;

/*
 * Initializes the JUMPMessageReader and sets its status to JUMP_SUCCESS.
 */
extern void
jumpMessageReaderInit(JUMPMessageReader* r, JUMPMessage m);

/*
 * If r->status != JUMP_SUCCESS, returns 0.  Otherwise returns the
 * next byte from the message or sets r->status to JUMP_OVERRUN on
 * error.
 */
extern int8
jumpMessageGetByte(JUMPMessageReader* r);

/*
 * If r->status != JUMP_SUCCESS, returns NULL.  Otherwise copies bytes
 * from the message into the buffer and returns buffer, or sets
 * r->status to JUMP_OVERRUN on error.
 */
extern int8*
jumpMessageGetBytesInto(JUMPMessageReader* r, int8* buffer, uint32 length);

/*
 * If r->status != JUMP_SUCCESS, returns NULL.  Otherwise returns the
 * next byte array from the message and sets *length to the number of
 * bytes in the array, or returns NULL and sets r->status to one of
 * JUMP_OVERRUN, JUMP_OUT_OF_MEMORY, or JUMP_NEGATIVE_ARRAY_LENGTH on
 * error.  A NULL return with r->status == JUMP_SUCCESS indicates a
 * NULL array was sent.  If the return value is non-NULL, it should
 * be passed to free() when it is no longer used.
 */
extern int8*
jumpMessageGetByteArray(JUMPMessageReader* r, uint32* length);

/*
 * If r->status != JUMP_SUCCESS, returns 0.  Otherwise returns the
 * next short from the message or sets r->status to JUMP_OVERRUN on
 * error.
 */
extern int16
jumpMessageGetShort(JUMPMessageReader* r);

/*
 * If r->status != JUMP_SUCCESS, returns 0.  Otherwise returns the
 * next int from the message or sets r->status to JUMP_OVERRUN on
 * error.
 */
extern int32
jumpMessageGetInt(JUMPMessageReader* r);

/*
 * If r->status != JUMP_SUCCESS, returns 0.  Otherwise returns the
 * next int from the message or sets r->status to JUMP_OVERRUN on
 * error.
 */
extern int64
jumpMessageGetLong(JUMPMessageReader* r);

/*
 * If r->status != JUMP_SUCCESS, returns NULL.  Otherwise returns the
 * next string from the message or sets r->status to one of
 * JUMP_OVERRUN, JUMP_OUT_OF_MEMORY, or JUMP_NEGATIVE_ARRAY_LENGTH on
 * error. The caller should call free() on the return value once it is
 * done
 */
extern JUMPPlatformCString
jumpMessageGetString(JUMPMessageReader* r);

/*
 * If r->status != JUMP_SUCCESS, returns NULL.  Otherwise returns the
 * string array at the current message position and sets *length to
 * the number of elements in the array, or returns NULL and sets
 * r->status to one of JUMP_OVERRUN, JUMP_OUT_OF_MEMORY, or
 * JUMP_NEGATIVE_ARRAY_LENGTH on error.  A NULL return with r->status
 * == JUMP_SUCCESS indicates a NULL array was sent.  If the return
 * value is non-NULL, it should be passed with *length to
 * jumpMessageFreeStringArray() when it is no longer used.
 */
extern JUMPPlatformCString*
jumpMessageGetStringArray(JUMPMessageReader* r, uint32* length);

/*
 * Frees an array of strings.
 */
extern void
jumpMessageFreeStringArray(JUMPPlatformCString* p, uint32 length);

/*
 * Message header inspection
 */
extern JUMPPlatformCString
jumpMessageGetType(JUMPMessage m);

extern JUMPAddress*
jumpMessageGetSender(JUMPMessage m);

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
 * JUMP_WOULD_BLOCK, JUMP_TARGET_NONEXISTENT, or JUMP_FAILURE.
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
 * JUMP_WOULD_BLOCK, JUMP_TARGET_NONEXISTENT, or JUMP_FAILURE.
 */
extern void
jumpMessageSendAsyncResponse(JUMPOutgoingMessage m,
			     JUMPMessageStatusCode* code);

/*
 * On return, sets *code to one of JUMP_SUCCESS, JUMP_OUT_OF_MEMORY,
 * JUMP_WOULD_BLOCK, JUMP_TARGET_NONEXISTENT, JUMP_TIMEOUT,
 * JUMP_UNBLOCKED, JUMP_OVERRUN, JUMP_NEGATIVE_ARRAY_LENGTH, or JUMP_FAILURE.
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
typedef struct JUMPMessageHandlerRegistration * JUMPMessageHandlerRegistration;

/*
 * Listening to messages directly
 */

/*
 * Register 'type' for direct listening.  On success, *code is set to
 * JUMP_SUCCESS.  Otherwise it is set to one of JUMP_OUT_OF_MEMORY or
 * JUMP_FAILURE.
 */
extern JUMPMessageHandlerRegistration
jumpMessageRegisterDirect(JUMPPlatformCString type,
			  JUMPMessageStatusCode *code);

/*
 * Block and wait for incoming message of a given type
 *
 * On return, sets *code to one of JUMP_SUCCESS, JUMP_OUT_OF_MEMORY,
 * JUMP_TIMEOUT, JUMP_UNBLOCKED, JUMP_OVERRUN, JUMP_NEGATIVE_ARRAY_LENGTH,
 * JUMP_NO_SUCH_QUEUE, or JUMP_FAILURE.
 */
extern JUMPMessage
jumpMessageWaitFor(JUMPPlatformCString type,
		   int32 timeout,
		   JUMPMessageStatusCode *code);

/**
 * Unblocks one thread blocking in, or about to call,
 * jumpMessageWaitFor (or jumpMessageSendSync, although this is not
 * intended to be used with threads blocking in jumpMessageSendSync
 * since they should be using a timeout).  The thread may return with
 * JUMP_UNBLOCKED.  This is used to unblock listening threads so they
 * can exit when they are no longer needed.
 * 
 * @return On success, set *code to JUMP_SUCCESS.  Otherwise
 *         sets *code to one of JUMP_NO_SUCH_QUEUE or JUMP_FAILURE.
 */
extern void jumpMessageUnblock(JUMPPlatformCString messageType,
			       JUMPMessageStatusCode* code);

/*
 * Returns a file descriptor for the messageType which may be
 * select()ed on and will become readable when a message may be
 * available.  When the file descriptor becomes readable, a subsequent
 * call to jumpMessageWaitFor will not block (assuming no other thread
 * has read the message), but may return a failure including
 * JUMP_UNBLOCKED.  Using the file descriptor for anything other than
 * select is undefined.  Using the file descriptor after the message
 * type has been unregistered is undefined.
 *
 * Returns the file descriptor, or -1 if the message type is not
 * registered.
 */
extern int
jumpMessageGetFd(JUMPPlatformCString type);

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
 * Create an outgoing message from a buffer that's been filled elsewhere.
 * The buffer is assumed to be JUMP_MESSAGE_BUFFER_SIZE bytes.
 * On success, returns the JUMPOutgoingMessage and sets *code to
 * JUMP_SUCCESS.  On failure, returns NULL and sets *code to one
 * JUMP_OUT_OF_MEMORY, JUMP_OVERRUN, or JUMP_NEGATIVE_ARRAY_LENGTH.
 */
extern JUMPOutgoingMessage
jumpMessageNewOutgoingFromBuffer(uint8* buffer, int isResponse,
				 JUMPMessageStatusCode *code);

/*
 * Get raw buffer of message
 */
extern uint8*
jumpMessageGetData(JUMPMessage message);

/*
 * Command handling -- TODO
 */
#endif /* __JUMP_MESSAGING_H */
