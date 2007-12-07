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

#include <string.h>

#include <jsr120_list_element.h>
#include <jsr120_sms_pool.h>

#ifndef ENABLE_PCSL
  #define pcsl_mem_malloc malloc
  #define pcsl_mem_free free
#else
  #include <pcsl_memory.h>
#endif
#include <app_package.h>

#define MAGIC_COOKEY_FOR_SMS_RECEIPT 0x7a
#define MAGIC_COOKEY_FOR_SMS_RECEIPT_PORT 1

/*
 * SMSPool data members
 */

/** The list of messages in the pool. */
static ListElement* SMSPool_smsMessages = NULL;

/** The number of messages in the pool. */
static jint SMSPool_count = 0;


/*
 * Helper functions to process SMS messages. These functions help to complete
 * these kinds of basic operations for SMS messages:
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
SmsMessage* jsr120_sms_new_msg(jchar	  encodingType,
                               unsigned char  msgAddr[MAX_ADDR_LEN],
                               jchar	  sourcePortNum,
                               jchar	  destPortNum,
                               jlong	  timeStamp,
                               jchar	  msgLen,
                               unsigned char* msgBuffer) {

	SmsMessage *sms = (SmsMessage*)pcsl_mem_malloc(sizeof(SmsMessage));
	memset(sms, 0, sizeof(SmsMessage));

	sms->msgAddr = (char*)pcsl_mem_malloc(MAX_ADDR_LEN);
	sms->msgBuffer = (char*)pcsl_mem_malloc(msgLen);

    sms->encodingType  = encodingType;
    sms->sourcePortNum = sourcePortNum;
    sms->destPortNum   = destPortNum;
    sms->timeStamp     = timeStamp;
    sms->msgLen        = msgLen;

    memcpy(sms->msgAddr, msgAddr, MAX_ADDR_LEN);
    memcpy(sms->msgBuffer, msgBuffer, msgLen);

    return sms;
}

/**
 * Copy a SMS message. Copy all data from the source message to the destination
 * message, overwriting all previous destination message data.
 *
 * @param src The source message.
 * @param dst The destination message.
 */
void jsr120_sms_copy_msg(SmsMessage* src, SmsMessage* dst) {

    dst->encodingType    = src->encodingType;
    dst->sourcePortNum   = src->sourcePortNum;
    dst->destPortNum     = src->destPortNum;
    dst->timeStamp       = src->timeStamp;
    dst->msgLen          = src->msgLen;
    memcpy(dst->msgBuffer, src->msgBuffer, src->msgLen);
    memcpy(dst->msgAddr, src->msgAddr, MAX_ADDR_LEN);
}

/**
 * Duplicate the SMS message structure by creating a new message and populating
 * its fields with the data from the source message.
 *
 * @param message The source message.
 *
 * @return The new message (A clone of the source message), or
 *	<code>NULL</code> if the source message was <code>NULL</code>.
 */
SmsMessage* jsr120_sms_dup_msg(SmsMessage* sms) {

    if (sms == NULL) {
        return NULL;
    }
    return jsr120_sms_new_msg(sms->encodingType,
                              (unsigned char*)sms->msgAddr,
                              sms->sourcePortNum,
                              sms->destPortNum,
                              sms->timeStamp,
                              sms->msgLen,
                              (unsigned char*)sms->msgBuffer);
}

/**
 * Delete the given SMS message. The memory used by the message will be
 * released to the memory pool.
 *
 * @param message The message that will have its memory freed.
 */
void jsr120_sms_delete_msg(SmsMessage* sms) {
    if (sms) {
        pcsl_mem_free(sms->msgAddr);
        pcsl_mem_free(sms->msgBuffer);
        pcsl_mem_free(sms);
    }
}


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

/*
 * Helper methods to operate the pool of messages.
 */

#if (ENABLE_CDC == 1)

#include <kni.h>
#include <sni.h>

static CVMMutex   _mutex;

static int is_mutex_inited = 0;

void jsr120_sms_pool_init() {
    /**
     * Impl note.
     * Theoretically, mutex could be created twice. 
     * But it is not a big problem if one mutex will be lost.
     */
    if (!is_mutex_inited) {
        is_mutex_inited = 1;
        CVMmutexInit(&_mutex);
    }
}

void jsr120_sms_pool_finalize() {
    if (!is_mutex_inited) {
       is_mutex_inited = 0;
       CVMmutexDestroy(&_mutex);
    }
}

#define MUTEX_LOCK   CVMmutexLock(&_mutex);
#define MUTEX_UNLOCK CVMmutexUnlock(&_mutex);

#else

#define MUTEX_LOCK
#define MUTEX_UNLOCK

void jsr120_sms_pool_init() {
}

void jsr120_sms_pool_finalize() {
}

#endif

/**
 * Increase the number of messages in the pool by one.
 */
static void jsr120_sms_pool_increase_count() {
    MUTEX_LOCK
    SMSPool_count++;
    MUTEX_UNLOCK
}

/**
 * Decrease the number of messages in the pool by one.
 */
static void jsr120_sms_pool_decrease_count()     {
    MUTEX_LOCK
    SMSPool_count--;
    if (SMSPool_count < 0) {
        SMSPool_count = 0;
    }
    MUTEX_UNLOCK
}

/**
 * Return the number of messages in the pool.
 *
 * @return The count of messages in the pool.
 */
static jint jsr120_sms_pool_get_count() {
    return SMSPool_count;
}

/**
 * Flush messages from the pool if the pool has filled its quota of messages.
 * <P>
 * When at least the maximum number of messages exists in the pool, flush
 * the oldest messages until the pool has room for only one more message.
 */
static void jsr120_sms_pool_check_pool_quota() {
    MUTEX_LOCK
    while (jsr120_sms_pool_get_count() >= MAX_SMS_MESSSAGES_IN_POOL) {
        jsr120_sms_pool_delete_next_msg();
    }
    MUTEX_UNLOCK
}

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
WMA_STATUS jsr120_sms_pool_add_msg(SmsMessage* smsMessage) {

    ListElement* newItem;

    if (smsMessage == NULL) { return WMA_ERR;}

    MUTEX_LOCK
    jsr120_sms_pool_check_pool_quota();

    newItem = jsr120_list_new_by_number(NULL, smsMessage->destPortNum,
        UNUSED_APP_ID, (void*)smsMessage, 0);
    jsr120_list_add_last(&SMSPool_smsMessages, newItem);
    jsr120_sms_pool_increase_count();
    MUTEX_UNLOCK

    return WMA_OK;
}

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
WMA_STATUS jsr120_sms_pool_get_next_msg(jchar smsPort, SmsMessage* out) {

    SmsMessage* sms;
    MUTEX_LOCK
    sms = jsr120_sms_pool_retrieve_next_msg(smsPort);
    MUTEX_UNLOCK
    if (sms) {
        if (out) {
            jsr120_sms_copy_msg(sms, out);
        }
        jsr120_sms_delete_msg(sms);
    }
    return ((sms != NULL) ? WMA_OK : WMA_ERR);
}

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
SmsMessage* jsr120_sms_pool_retrieve_next_msg(jchar smsPort) {

    SmsMessage* result = NULL;
    ListElement* e;

    MUTEX_LOCK
    e = jsr120_list_remove_first_by_number(&SMSPool_smsMessages,smsPort);
    if (e) {
        jsr120_sms_pool_decrease_count();
        result = e->userData;
        /* Remove the list elements from the pool without deleting the user-data */
        jsr120_list_destroy(e);
    }
    MUTEX_UNLOCK
    /* result MUST to be deleted by caller using SmsMessage_delete() */
    return result;
}

/**
 * Removes the next (oldest) message from the pool that matches the port
 * number.
 *
 * @param smsPort The SMS port to be matched.
 *
 * @return <code>WMA_OK</code> when a message was removed;
 *	<code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_sms_pool_remove_next_msg(jchar smsPort) {

    return jsr120_sms_pool_get_next_msg(smsPort, NULL);
}

/**
 * Removes all messages from the pool that match port number.
 *
 * @param msgID The SMS port number to be matched.
 */
void jsr120_sms_pool_remove_all_msgs(jchar smsPort) {
    while(jsr120_sms_pool_remove_next_msg(smsPort) == WMA_OK);
}

/**
 * Fetches the first message that matches the port number without removing the
 * message from the pool.
 *
 * @param smsPort The SMS port to be matched.
 *
 * @return The message or <code>NULL</code> if no message could be found.
 */
SmsMessage* jsr120_sms_pool_peek_next_msg(jchar smsPort) {
    return jsr120_sms_pool_peek_next_msg1(smsPort, 0);
}

/**
 * Fetches the first message that matches the port number without removing the
 * message from the pool.
 *
 * @param smsPort The SMS port to be matched.
 * @param isNew Get the new message only when not 0.
 *
 * @return The message or <code>NULL</code> if no message could be found.
 */
SmsMessage* jsr120_sms_pool_peek_next_msg1(jchar smsPort, jint isNew) {

    ListElement* e;
    MUTEX_LOCK
    e = jsr120_list_get_by_number1(SMSPool_smsMessages, smsPort, isNew);
    MUTEX_UNLOCK
    if (e) {
        return (SmsMessage *) e->userData;
    }
    return NULL;
}

/**
 * Deletes the oldest SMS message.
 *
 * @return <code>WMA_OK</code> if the oldest message was found and deleted;
 *	<code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_sms_pool_delete_next_msg() {

    WMA_STATUS found = WMA_ERR;
    ListElement* e;

    MUTEX_LOCK
    e = jsr120_list_remove_first(&SMSPool_smsMessages);
    if (e) {
        SmsMessage* sms = e->userData;
        jsr120_sms_delete_msg(sms);
        jsr120_sms_pool_decrease_count();
        jsr120_list_destroy(e);
        found = WMA_OK;
    }
    MUTEX_UNLOCK
    return found;
}

