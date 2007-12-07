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
#include <jsr120_cbs_pool.h>

#ifndef ENABLE_PCSL
  #define pcsl_mem_malloc malloc
  #define pcsl_mem_free free
#else
  #include <pcsl_memory.h>
#endif
#include <app_package.h>

/*
 * This is a collection of methods to maintain the pool of CBS messages
 * (CbsMessage data structures). There is no interface to define this
 * collection. This is just a "best guess" as to the general types of
 * messages needed to maintain the pool. There will be a porting layer
 * for the pool, later.
 *
 * The collection includes helper methods to create, copy, duplicate
 * and delete CBS messages.
 *
 * In the terminology, below, a message ID can be thought of as a port number or
 * a channel that is "tuned in" to listen to broadcast CBS messages.
 */


/**
 * CBSPool data members.
 */

/** The list of messages in the pool */
static ListElement* CBSPool_messages = NULL;

/** The number of messages in the pool. */
static int CBSPool_count = 0;


/*
 * Helper functions to process CBS messages. These functions help to complete
 * these kinds of basic operations for CBS messages:
 *
 * jsr120_cbs_new_msg
 *	Allocates memory for a new message.
 *
 * jsr120_cbs_copy_msg
 *	Copies one message over an existing message.
 *
 * jsr120_cbs_dup_msg
 *	Copies an existing message into new, allocated memory.
 *
 * jsr120_cbs_delete_msg
 *	Frees the memory used by an existing message.
 */

/**
 * Create a new message and populate it with the given data. Memory will be
 * allocated for the new message. If the message length is longer than the
 * maximum length permitted for a CBS message, the new message will contain only
 * the maximum number of characters permitted for a CBS message.
 *
 * @param encodingType The CBS message encoding type.
 * @param msgID The message identifier.
 * @param msgLen The length of the message, excluding the terminating character.
 * @param msgBuffer The buffer that holds the text message and its terminating
 *	character.
 *
 * @return The new CBS message.
 */
CbsMessage* jsr120_cbs_new_msg(jchar encodingType,
                               jchar msgID,
                               jchar msgLen,
                               unsigned char* msgBuffer) {

    /* Create a new message structure. */
    CbsMessage* message = (CbsMessage*)pcsl_mem_malloc(sizeof(CbsMessage));
    memset(message, 0, sizeof(CbsMessage));

    /* Populate the message structure with the given data. */
    message->encodingType = encodingType;
    message->msgID = msgID;

    /* Keep the message length in range. */
    if (msgLen > MAX_CBS_MESSAGE_SIZE) {
        msgLen = MAX_CBS_MESSAGE_SIZE;
    }
    message->msgLen = msgLen;
    message->msgBuffer = (unsigned char*) pcsl_mem_malloc(msgLen);

    /* Copy the message. */
    memcpy(message->msgBuffer, msgBuffer, msgLen);

    return message;
}

/**
 * Copy a CBS message. Copy all data from the source message to the destination
 * message, overwriting all previous destination message data.
 *
 * @param src The source message.
 * @param dst The destination message.
 */
void jsr120_cbs_copy_msg(CbsMessage* src, CbsMessage* dst){
    short msgLen;

    /* Populate the various message fields. */
    dst->encodingType = src->encodingType;
    dst->msgID        = src->msgID;

    /* Keep the message length in range. */
    msgLen = src->msgLen;
    if (msgLen > MAX_CBS_MESSAGE_SIZE) {
        msgLen = MAX_CBS_MESSAGE_SIZE;
    }
    dst->msgLen = msgLen;

    /* Copy the message. */
    memcpy(dst->msgBuffer, src->msgBuffer, msgLen);
}

/**
 * Duplicate the CBS message structure by creating a new message and populating
 * its fields with the data from the source message.
 *
 * @param message The source message.
 *
 * @return The new message (A clone of the source message), or
 *	<code>NULL</code> if the source message was <code>NULL</code>.
 */
CbsMessage* jsr120_cbs_dup_msg(CbsMessage* message) {

    if (message == NULL) {
        return NULL;
    }

    return jsr120_cbs_new_msg(message->encodingType,
                              message->msgID,
                              message->msgLen,
                              message->msgBuffer);
}

/**
 * Delete the given CBS message. The memory used by the message will be
 * released to the memory pool.
 *
 * @param message The message that will have its memory freed.
 */
void jsr120_cbs_delete_msg(CbsMessage* message) {

    if (message != NULL) {
        pcsl_mem_free(message->msgBuffer);
        pcsl_mem_free(message);
    }
}

/*
 * Helper methods to operate the pool of messages.
 */

/**
 * Increase the number of messages in the pool by one.
 */
static void jsr120_cbs_pool_increase_count() {
    CBSPool_count++;
}

/**
 * Decrease the number of messages in the pool by one.
 */
static void jsr120_cbs_pool_decrease_count() {
    CBSPool_count--;
    if (CBSPool_count < 0) {
        CBSPool_count = 0;
    }
}

/**
 * Return the number of messages in the pool.
 *
 * @return The count of messages in the pool.
 */
static jint jsr120_cbs_pool_get_count() {
    return CBSPool_count;
}

/**
 * Flush messages from the pool if the pool has filled its quota of messages.
 * <P>
 * When at least the maximum number of messages exists in the pool, flush
 * the oldest messages until the pool has room for only one more message.
 */
static void jsr120_cbs_pool_check_quota() {
    while (jsr120_cbs_pool_get_count() >= MAX_CBS_MESSAGES_IN_POOL) {
        jsr120_cbs_pool_delete_next_msg();
    }
}


/*
 * CBS Message Pool Functions
 *
 * jsr120_cbs_pool_add_msg
 *	Add a new message to the pool.
 *
 * jsr120_cbs_pool_get_next_msg
 *	Fetch the next message and remove it from the pool.
 *
 * jsr120_cbs_pool_retrieve_next_msg
 *	Retrieve the next message data and remove only the entry from the pool.
 *	The data must be removed separately.
 *
 * jsr120_cbs_pool_remove_next_msg
 *	Remove the next message from the pool that matches the ID.
 *
 * jsr120_cbs_pool_remove_all_msgs
 *	Remove all messages from the pool.
 *
 * jsr120_cbs_pool_peek_next_msg
 *	Fetch the next message that matches the ID without removing the message
 *	from the pool>
 *
 * jsr120_cbs_pool_delete_next_msg
 *	Delete the next message from the pool that matches the ID.
 */

/**
 * Add a CBS message to the message pool. If the pool is full (i.e., there are
 * at least <code>MAX_CBS_MESSAGES_IN_POOL</code>), then the oldest messages are
 * discarded.
 *
 * @param cbsMessage The CBS message to be added.
 *
 * @return <code>WMA_OK</code> if the message was successfully added to the pool.
 *	<code>WMA_ERR</code>, otherwise.
 *
 */
WMA_STATUS jsr120_cbs_pool_add_msg(CbsMessage* cbsMessage) {
    ListElement* newItem;

    /* If there is no message to add, bail out. */
    if (cbsMessage == NULL) {
        return WMA_ERR;
    }

    /* Make room for this message, if necessary. */
    jsr120_cbs_pool_check_quota();

    /* Create the new pool item and add it to the pool. */
    newItem = jsr120_list_new_by_number(NULL, cbsMessage->msgID,
        UNUSED_APP_ID, (void*)cbsMessage, 0);
    jsr120_list_add_last(&CBSPool_messages, newItem);
    jsr120_cbs_pool_increase_count();

    return WMA_OK;
}

/**
 * Retrieves the next message from the pool that matches the message identifier
 * and removes the message entry from the pool. The data for the message itself
 * are not removed here and must be done separately.
 *
 * @param msgID The message identifier to be matched.
 * @param out Space for the message. If <code>NULL</code>, this function will
 *     remove the message from the pool.
 *
 * @return <code>WMA_OK</code> if a message could be located;
 *     <code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_cbs_pool_get_next_msg(jchar msgID, CbsMessage* out) {

    /* Try to locate the message matching the message ID */
    CbsMessage* cbs = jsr120_cbs_pool_retrieve_next_msg(msgID);

    if (cbs != NULL) {

        /* If space exists for the message, copy the message */
        if (out != NULL) {
            jsr120_cbs_copy_msg(cbs, out);
        }

        /* Always delete the message from the pool. */
        jsr120_cbs_delete_msg(cbs);
    }

    return ((cbs != NULL) ? WMA_OK : WMA_ERR);
}

/**
 * Retrieves the next (oldest) message from the pool that matches the message
 * identifier. Removes the entry from the message pool, but doesn't free the
 * memory associated with the message. The result MUST be deleted by the
 * caller using <code>jsr120_cbs_delete_msg</code>.
 *
 * @param msgID The message identifier to be matched.
 *
 * @return The message or <code>NULL</code> if no message could be retrieved.
 */
CbsMessage* jsr120_cbs_pool_retrieve_next_msg(jchar msgID) {
    ListElement* elem;

    /* The result of the search */
    CbsMessage* result = NULL;

    elem = jsr120_list_remove_first_by_number(&CBSPool_messages, msgID);
    if (elem) {

        result = elem->userData;

        /* Delete element from pool while preserving user data. */
        jsr120_list_destroy(elem);
        jsr120_cbs_pool_decrease_count();
    }

    /* Result MUST be deleted by caller using CbsMessage_delete() */
    return result;
}

/**
 * Removes the next (oldest) message from the pool that matches the message
 * identifier.
 *
 * @param msgID The message identifier to be matched.
 *
 * @return <code>WMA_OK</code> when a message was removed;
 *     <code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_cbs_pool_remove_next_msg(jchar msgID) {
    return jsr120_cbs_pool_get_next_msg(msgID, NULL);
}

/**
 * Removes all messages from the pool that match the message identifier.
 *
 * @param msgID The message identifier to be matched.
 */
void jsr120_cbs_pool_remove_all_msgs(jchar msgID) {
    while(jsr120_cbs_pool_remove_next_msg(msgID) == WMA_OK);
}

/**
 * Fetches the first message that matches the message identifier without
 * removing the message from the pool.
 *
 * @param msgID The message identifier to be matched.
 *
 * @return The message or <code>NULL</code> if no message could be found.
 */
CbsMessage* jsr120_cbs_pool_peek_next_msg(jchar msgID){
    return jsr120_cbs_pool_peek_next_msg1(msgID, 0);
}

/**
 * Fetches the first message that matches the message identifier without
 * removing the message from the pool.
 *
 * @param msgID The message identifier to be matched.
 * @param isNew Get the new message only when not 0.
 *
 * @return The message or <code>NULL</code> if no message could be found.
 */
CbsMessage* jsr120_cbs_pool_peek_next_msg1(jchar msgID, jint isNew){
    ListElement* elem = jsr120_list_get_by_number1(CBSPool_messages, msgID, isNew);
    if (elem) {
        return (CbsMessage*)elem->userData;
    }
    return NULL;
}

/**
 * Deletes the oldest CBS message.
 *
 * @return <code>WMA_OK</code> if the oldest message was found and deleted;
 *     <code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_cbs_pool_delete_next_msg() {
    ListElement* elem;

    /* Assume there was no message to delete. */
    WMA_STATUS found = WMA_ERR;

    elem = jsr120_list_remove_first(&CBSPool_messages);
    if (elem) {

        /* Free the memory used for the message. */
        CbsMessage* cbs = elem->userData;
        jsr120_cbs_delete_msg(cbs);

        /* Update the pool list and count. */
        jsr120_list_destroy(elem);
        jsr120_cbs_pool_decrease_count();

        /* The first (oldest) message was found and deleted. */
        found = WMA_OK;
    }
    return found;
}
