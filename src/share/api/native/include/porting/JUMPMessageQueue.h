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
 * Status codes from various JUMPMessageQueue functions.  XXX these
 * will probably also be passed out of the JUMPMessage functions
 * without translation, so they should be moved to a separate file ala
 * errno.h
 *
 * JUMP_MQ_FAILURE currently denotes any kind of failure other than
 * out of memory.  We may need or want to make this more granular, or
 * make JUMPMessageQueueStatusCode a struct with an optional string
 * explaining the error.
 */
typedef enum {
    JUMP_MQ_TIMEOUT       = 1,
    JUMP_MQ_BUFFER_SMALL  = 2,
    JUMP_MQ_SUCCESS       = 3,
    JUMP_MQ_FAILURE       = 4,
    JUMP_MQ_OUT_OF_MEMORY = 5
} JUMPMessageQueueStatusCode;

/**
 * Creates the message queue for a message type.
 *
 * Implementations can choose whether to have a centralized queue for all
 * incoming messages, or to have separate queues for each message type.
 *
 * @return 0 if the message queue has been successfully created and non-zero
 *         if the message queue cannot be created.
 *
 * @see jumpMessageQueueDestroy
 */
extern void jumpMessageQueueCreate(JUMPPlatformCString messageType,
				   JUMPMessageQueueStatusCode* code);

/**
 * Destroys the message queue created for this process
 *
 * @see jumpMessageQueueCreate
 * @return 0 if the message queue has been successfully destroyed and non-zero
 *         if the message queue cannot be destroyed
 */
extern int jumpMessageQueueDestroy(JUMPPlatformCString messageType);

/**
 * Opens the message queue associated with the process (id), and message type.
 *
 * @return a non-null handle on success and NULL if the message queue for
 *         the specified process cannot be opened.
 */
extern JUMPMessageQueueHandle 
jumpMessageQueueOpen(int processId, JUMPPlatformCString type,
		     JUMPMessageQueueStatusCode* code);

/**
 * Closes the message queue handle.
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
 * Send the message data to the message queue. This call does not block
 * and returns with an error if the message cannot be sent.
 *
 * @param buffer buffer that has space for the message header (if any) and
 *        the message data following it. The length of the buffer is 
 *        greater than or equal to <b>messageDataSize</b> and the message data 
 *        is present at the location 
 *        <b>buffer</b>
 * @param messageDataSize the message data size.
 *
 * @return 0 if the message has been successfully sent to the message queue
 *         associated with the handle or a non-zero value in case of an error
 */
extern int jumpMessageQueueSend(JUMPMessageQueueHandle handle,
    char *buffer,
    int messageDataSize);

/**
 * Wait till a message is availabe in this process message queue. This
 * call will <b>BLOCK</b> till there is a message available or a
 * timeout happens after 'timeout' seconds.
 *
 * @return 0 if there is available data, and non-zero on error
 */
extern int jumpMessageQueueWaitForMessage(JUMPPlatformCString messageType,
					  int32 timeout);

/**
 * Retrieves a message from this process message queue and copies the 
 * message data to the buffer passed. This method does not block if 
 * there is no message in the queue.
 * 
 * @return the message data size. -1 If the message data is greater than the 
 * <code>bufferLength</code> passed and the message is
 * not retrieved from the queue. The caller is expected to pass a bigger
 * buffer. 
 */
extern int jumpMessageQueueReceive(JUMPPlatformCString messageType,
				   char *buffer, int bufferLength);

/*
 * Close and destroy all message queues created by the process.
 */
extern void jumpMessageQueueInterfaceDestroy(void);
    
#if defined __cplusplus 
}
#endif /* __cplusplus */
#endif /* __JUMP_MESSAGE_QUEUE_H */
