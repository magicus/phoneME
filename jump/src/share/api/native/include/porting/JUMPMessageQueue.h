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
#ifndef __JUMP_MESSAGE_QUEUE_H
#define __JUMP_MESSAGE_QUEUE_H

#include "porting/JUMPTypes.h"

typedef void * JUMPMessageQueueHandle;

#if defined __cplusplus 
extern "C" { 
#endif /* __cplusplus */

/*
 * Status codes from various JUMPMessageQueue functions
 *
 * JUMP_MQ_FAILURE currently denotes any kind of failure that is not
 * one of the more specific failures.  We may need or want to make
 * this more granular, or make JUMPMessageQueueStatusCode a struct
 * with an optional string explaining the error.
 */
typedef enum {
    JUMP_MQ_TIMEOUT       = 1,
    JUMP_MQ_BUFFER_SMALL  = 2,
    JUMP_MQ_SUCCESS       = 3,
    JUMP_MQ_FAILURE       = 4,
    JUMP_MQ_OUT_OF_MEMORY = 5,
    JUMP_MQ_BAD_MESSAGE_SIZE = 6,
    JUMP_MQ_WOULD_BLOCK   = 7,
    JUMP_MQ_NO_SUCH_QUEUE = 8,
    JUMP_MQ_UNBLOCKED     = 9,
    JUMP_MQ_TARGET_NONEXISTENT = 10
} JUMPMessageQueueStatusCode;

/**
 * Creates the receiving message queue for a message type.
 *
 * Implementations can choose whether to have a centralized queue for all
 * incoming messages, or to have separate queues for each message type.
 *
 * A message queue can be created multiple times.  It is not destroyed
 * until it has been destroyed the same number of times.
 *
 * Message queues are thread-safe and may be read from concurrently by
 * multiple threads, however, each message will delivered to only one
 * thread.
 *
 * @return If the message queue has been successfully created, or
 *   already exists due to a previous call to jumpMessageQueueCreate,
 *   then *code is set to JUMP_MQ_SUCCESS.  Otherwise *code is set
 *   to one of JUMP_MQ_FAILURE or JUMP_MQ_OUT_OF_MEMORY.
 *
 * @see jumpMessageQueueDestroy
 */
extern void jumpMessageQueueCreate(JUMPPlatformCString messageType,
				   JUMPMessageQueueStatusCode* code);

/**
 * Destroys the receiving message queue created for a message type.
 *
 * A message queue can be created multiple times.  It is not destroyed
 * until it has been destroyed the same number of times.
 *
 * @see jumpMessageQueueCreate
 * @return 0 if the the message queue has been successfully destroyed
 *   or if the queue still exists because the queue has been created
 *   more times than it has been destroyed.  Non-zero if the
 *   queue does not exist or destroying the queue fails.
 */
extern int jumpMessageQueueDestroy(JUMPPlatformCString messageType);

/**
 * Opens message queue for sending to the process (id), and message type.
 * A message queue may be opened more than once, in which case the
 * same handle or a different handle may be returned.  Closing a handle
 * more times than it has been returned is undefined.
 *
 * Message queues are thread-safe and may be written to concurrently from
 * multiple threads.
 *
 * @return On success returns a non-NULL handle and sets *code to
 * JUMP_MQ_SUCCESS.  On failure retuns NULL and sets *code to one of
 * JUMP_MQ_OUT_OF_MEMORY, JUMP_MQ_TARGET_NONEXISTENT, or
 * JUMP_MQ_FAILURE.
 */
extern JUMPMessageQueueHandle 
jumpMessageQueueOpen(int processId, JUMPPlatformCString type,
		     JUMPMessageQueueStatusCode* code);

/**
 * Closes a sending message queue handle.
 *
 * Closing a handle more times than it has been returned from
 * jumpMessageQueueOpen is undefined.
 */
extern void jumpMessageQueueClose(JUMPMessageQueueHandle handle);

/**
 * Returns the offset where the message data starts within the send buffer.
 * If the message header is not part of the message data then this method
 * should return 0. If the message header is part of the message data
 * then this should return the offset where the message data should be 
 * copied by the caller.
 */
extern int jumpMessageQueueDataOffset(void);

/**
 * Sends the message data to the message queue. This call does not block
 * and returns with an error if the message cannot be sent.
 *
 * @param buffer buffer that has space for the message header (if any) and
 *        the message data following it. The length of the buffer is 
 *        greater than or equal to <b>messageDataSize</b> and the message data 
 *        is present at the location 
 *        <b>buffer</b>
 * @param messageDataSize the message data size.
 *
 * @return If the message has been successfully sent to the message queue
 *         associated with the handle, returns 0 and sets *code to
 *         JUMP_MQ_SUCCESS.  Otherwise returns non-zero and sets *code
 *         to one of JUMP_MQ_BAD_MESSAGE_SIZE, JUMP_MQ_WOULD_BLOCK, or
 *         JUMP_MQ_FAILURE.
 */
extern int jumpMessageQueueSend(JUMPMessageQueueHandle handle,
    char *buffer,
    int messageDataSize,
    JUMPMessageQueueStatusCode* code);

/**
 * Waits till a message is available in this process message queue. This
 * call will <b>BLOCK</b> till there is a message available or a
 * timeout happens after 'timeout' milliseconds.  A timeout of
 * 0 means wait forever.  Even though this function returns successfully,
 * a subsequent call to jumpMessageQueueReceive may return JUMP_MQ_WOULD_BLOCK
 * if another thread reads the message first, or if there was actually
 * no message.
 *
 * @return If a message is available, returns 0 and sets *code to
 *         JUMP_MQ_SUCCESS.  Otherwise returns non-zero and sets *code
 *         to one of JUMP_MQ_NO_SUCH_QUEUE, JUMP_MQ_TIMEOUT, or
 *         JUMP_MQ_FAILURE.
 */
extern int jumpMessageQueueWaitForMessage(JUMPPlatformCString messageType,
					  int32 timeout,
					  JUMPMessageQueueStatusCode* code);

/**
 * Retrieves a message from this process message queue and copies the 
 * message data to the buffer passed. This method does not block if 
 * there is no message in the queue. If the buffer is not large enough,
 * the message is discarded.
 * 
 * @return If a message is read, returns the message length and sets *code
 *         to JUMP_MQ_SUCCESS.  Otherwise returns -1 and sets *code
 *         to one of JUMP_MQ_NO_SUCH_QUEUE, JUMP_MQ_WOULD_BLOCK,
 *         JUMP_MQ_BUFFER_SMALL, JUMP_MQ_UNBLOCKED, or JUMP_MQ_FAILURE.
 */
extern int jumpMessageQueueReceive(JUMPPlatformCString messageType,
				   char *buffer, int bufferLength,
				   JUMPMessageQueueStatusCode* code);

/**
 * Unblocks one thread waiting in, or about to call,
 * jumpMessageQueueWaitForMessage.  jumpMessageQueueWaitForMessage
 * will return successfully, and a subsequent call to
 * jumpMessageQueueReceive may fail with JUMP_MQ_UNBLOCKED.  This is
 * used to unblock listening threads so they can exit when they are no
 * longer needed.
 * 
 * @return On success, set *code to JUMP_MQ_SUCCESS.  Otherwise
 *         sets *code to one of JUMP_MQ_NO_SUCH_QUEUE or
 *         JUMP_MQ_FAILURE.
 */
extern void jumpMessageQueueUnblock(JUMPPlatformCString messageType,
				    JUMPMessageQueueStatusCode* code);

/*
 * Returns a file descriptor for the messageType which may be
 * select()ed on and will become readable when a message may be
 * available.  When the file descriptor becomes readable, a subsequent
 * call to jumpMessageQueueWaitForMessage will not block (assuming no
 * other thread has read the message).  Using the file descriptor for
 * anything other than select() is undefined.  Using the file
 * descriptor after the message queue has been destroyed is undefined.
 *
 * Returns the file descriptor, or -1 if there is no queue for the
 * message type.
 */
extern int
jumpMessageQueueGetFd(JUMPPlatformCString messageType);

/*
 * Close and destroy all message queues created by the process.
 */
extern void jumpMessageQueueInterfaceDestroy(void);

/*
 * Clean up low-level message queues created by the given process.
 * If the process is still alive as determined by jumpProcessIsAlive(),
 * silently do nothing.
 */
extern void jumpMessageQueueCleanQueuesOf(int cpid);
    
#if defined __cplusplus 
}
#endif /* __cplusplus */
#endif /* __JUMP_MESSAGE_QUEUE_H */
