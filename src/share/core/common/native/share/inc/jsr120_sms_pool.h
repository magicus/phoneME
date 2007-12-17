/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _JSR120_SMS_POOL_H_
#define _JSR120_SMS_POOL_H_

/**
 * @file
 * @ingroup wma
 */

/**
 * @defgroup smsstorage SMS Storage Porting Interface
 * @ingroup sms 
 * @brief Short Message Service storage porting interface. \n
 * ##include <jsr120_sms_structs.h>
 * @{
 *
 * This file defines the Short Message Service storage porting interfaces.
 * Incoming SMS messages are stored in a message pool, from which they can be
 * retrieved and deleted. Messages can also be created, copied and duplicated.
 * The provided RAM-based implementation may be used as-is or replaced/modified
 * to suit a particular platform, using the platform interfaces (e.g.: An
 * implementation that uses the file system to provide persistent message
 * storage.).
 * 
 */

#include <jsr120_types.h>
#include <jsr120_sms_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The maximum number of messages permitted in the SMS pool before messages
 * are flushed. The messages that arrived in the pool first are flushed.
 */
#define MAX_SMS_MESSSAGES_IN_POOL (50)


/*
 * Helper functions to process CBS messages. These functions help to complete
 * these kinds of basic operations for CBS messages:
 *
 * jsr120_sms_new_msg
 *	Allocates memory for a new message.
 *
 * jsr120_sms_copy_msg
 *	Copies one message over an existing message.
 *
 * jsr120_sms_dup_msg
 *	Copies an existing message into new, allocated memory.
 *
 * jsr120_sms_delete_msg
 *	Frees the memory used by an existing message.
 */

/**
 * Create a new message and populate it with the given data. Memory will be
 * allocated for the new message. If the message length is longer than the
 * maximum length permitted for a SMS message, the new message will contain only
 * the maximum number of characters permitted for a SMS message.
 *
 * @param encodingType The SMS message encoding type.
 * @param msgAddr Source MSISDN phone number (32 chars).
 * @param sourcePortNum Source SMS port that the message was sent from.
 * @param destPortNum Destination SMS port that the message was sent to.
 * @param timeStamp Creation time.
 * @param msgLen Message size.
 * @param msgBuffer The body of the message.
 *
 * @return The new SMS message.
 */
SmsMessage* jsr120_sms_new_msg(jchar          encodingType,
                               unsigned char  msgAddr[MAX_ADDR_LEN],
                               jchar          sourcePortNum,
                               jchar          destPortNum,
                               jlong          timeStamp,
                               jchar          msgLen,
                               unsigned char* msgBuffer);

/**
 * Copy a SMS message. Copy all data from the source message to the destination
 * message, overwriting all previous destination message data.
 *
 * @param src The source message.
 * @param dst The destination message.
 */
void jsr120_sms_copy_msg(SmsMessage* src, SmsMessage* dst);

/**
 * Duplicate the SMS message structure by creating a new message and populating
 * its fields with the data from the source message.
 *
 * @param message The source message.
 *
 * @return The new message (A clone of the source message), or
 *	<code>NULL</code> if the source message was <code>NULL</code>.
 */
SmsMessage* jsr120_sms_dup_msg(SmsMessage* message);

/**
 * Delete the given SMS message. The memory used by the message will be
 * released to the memory pool.
 *
 * @param message The message that will have its memory freed.
 */
void jsr120_sms_delete_msg(SmsMessage* message);


/*
 * SMS Message Pool Functions
 *
 * jsr120_sms_pool_add_msg
 *	Add a new message to the pool.
 *
 * jsr120_sms_pool_get_next_msg
 *	Fetch the next message and remove it from the pool.
 *
 * jsr120_sms_pool_retrieve_next_msg
 *	Retrieve the next message data and remove only the entry from the pool.
 *	The data must be removed separately.
 *
 * jsr120_sms_pool_remove_next_msg
 *	Remove the next message from the pool that matches the ID.
 *
 * jsr120_sms_pool_remove_all_msgs
 *	Remove all messages from the pool.
 *
 * jsr120_sms_pool_peek_next_msg
 *	Fetch the next message that matches the ID without removing the message
 *	from the pool>
 *
 * jsr120_sms_pool_delete_next_msg
 *	Delete the next message from the pool that matches the ID.
 */

/**
 * Adds a SMS message to the message pool. If the pool is full (i.e., there are
 * at least <code>MAX_SMS_MESSAGES_IN_POOL</code>), then the oldest messages are
 * discarded.
 *
 * @param message The SMS message to be added.
 *
 * @return <code>WMA_OK</code> if the message was successfully added to the pool;
 *	<code>WMA_ERR</code>, otherwise.
 *
 */
WMA_STATUS jsr120_sms_pool_add_msg(SmsMessage* message);

/**
 * Retrieves the next message from the pool that matches the message identifier
 * and removes the message entry from the pool. The data for the message itself
 * are not removed here and must be done separately.
 *
 * @param smsPort the destination SMS port to look for
 * @param out Space for the message.
 *
 * @return <code>WMA_OK</code> if a message could be located;
 *	<code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_sms_pool_get_next_msg(jchar smsPort, SmsMessage* out);

/**
 * Retrieves the next (oldest) message from the pool that matches the port
 * number. Removes the entry from the message pool, but doesn't free the
 * memory associated with the message. The result MUST be deleted by the
 * caller using <code>jsr120_sms_delete_msg</code>.
 *
 * @param smsPort The SMS port to be matched.
 *
 * @return The message or <code>NULL</code> if no message could be retrieved.
 */
SmsMessage* jsr120_sms_pool_retrieve_next_msg(jchar smsPort);

/**
 * Removes the next (oldest) message from the pool that matches the port
 * number.
 *
 * @param smsPort The SMS port to be matched.
 *
 * @return <code>WMA_OK</code> when a message was removed;
 *	<code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_sms_pool_remove_next_msg(jchar smsPort);

/**
 * Removes all messages from the pool that match port number.
 *
 * @param smsPort The SMS port number to be matched.
 */
void jsr120_sms_pool_remove_all_msgs(jchar smsPort);

/**
 * Fetches the first message that matches the port number without removing the
 * message from the pool.
 *
 * @param smsPort The SMS port to be matched.
 *
 * @return The message or <code>NULL</code> if no message could be found.
 */
SmsMessage* jsr120_sms_pool_peek_next_msg(jchar smsPort);

/**
 * Fetches the first message that matches the port number without removing the
 * message from the pool.
 *
 * @param smsPort The SMS port to be matched.
 * @param isNew Get the new message only when not 0.
 *
 * @return The message or <code>NULL</code> if no message could be found.
 */
SmsMessage* jsr120_sms_pool_peek_next_msg1(jchar smsPort, jint isNew);

/**
 * Deletes the oldest SMS message.
 *
 * @return <code>WMA_OK</code> if the oldest message was found and deleted;
 *	<code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_sms_pool_delete_next_msg();

/**
 * Initialize SMS message pool.
 *
 */
void jsr120_sms_pool_init();

/**
 * Finalize SMS message pool.
 *
 */
void jsr120_sms_pool_finalize();

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* #ifdef _JSR120_SMS_POOL_H_ */
