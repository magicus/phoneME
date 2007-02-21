/*
 *
 *
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

#include <string.h>
#include <kni.h>
#include <sni.h>
#include <commonKNIMacros.h>
#include <ROMStructs.h>
#include <midp_thread.h>
#include <midpServices.h>
#include <push_server_export.h>
#include <suitestore_common.h>
#include <jsr120_list_element.h>
#include <jsr120_cbs_pool.h>
#include <jsr120_cbs_listeners.h>
#include <jsr120_cbs_protocol.h>

/*
 * Listeners registered by a currently running midlet
 */
static ListElement* cbs_midlet_listeners = NULL;

/*
 * Listeners registered by push subsystem
 */
static ListElement* cbs_push_listeners = NULL;

/*
 * General form of a CBS listener.
 */
typedef WMA_STATUS cbs_listener_t(CbsMessage* message, void* userData);

/*
 * private methods
 */
static WMA_STATUS jsr120_cbs_midlet_listener(CbsMessage *message,
    void* userData);
static WMA_STATUS jsr120_cbs_push_listener(CbsMessage *message,
    void* userData);
static WMA_STATUS jsr120_cbs_invoke_listeners(CbsMessage* message,
    ListElement *listeners);
static JVMSPI_ThreadID jsr120_cbs_get_blocked_thread_from_handle(long handle,
    jint waitingFor);
static WMA_STATUS jsr120_cbs_is_msgID_registered(jchar msgID,
    ListElement *listeners);
static WMA_STATUS jsr120_cbs_register_msgID(jchar msgID, SuiteIdType msid,
    cbs_listener_t* listener, void* userData, ListElement **listeners);
static WMA_STATUS jsr120_cbs_unregister_msgID(jchar msgID,
    cbs_listener_t* listener, ListElement **listeners);
static void jsr120_cbs_delete_all_msgs(SuiteIdType msid, ListElement* head);

/**
 * Invoke registered listeners that match the msgID specified in the CBS
 * message.
 *
 * @param message The incoming CBS message.
 * @listeners The list of registered listeners.
 *
 * @result returns true if a matching listener is invoked, false
 *                 otherwise
 */
static WMA_STATUS jsr120_cbs_invoke_listeners(CbsMessage* message,
    ListElement *listeners) {

    ListElement* callback;

    /* Assume no listeners were found and threads unblocked */
    WMA_STATUS unblocked = WMA_ERR;

    /* Notify all listeners that match the given port (Message ID) */
    for(callback=jsr120_list_get_by_number(listeners, message->msgID);
	callback!=NULL;
	callback=jsr120_list_get_by_number(callback->next, message->msgID)) {

	/* Pick up the listener */
	cbs_listener_t* listener=(cbs_listener_t*)(callback->userDataCallback);

	if (listener!=NULL) {
            if((unblocked =
                listener(message, callback->userData)) == WMA_OK) {
                /*
                 * A thread blocked on receiving a message has been unblocked.
                 * So return.
                 */
                break;
            }
	}
    }
    return unblocked;
}


/*
 * See jsr120_cbs_listeners.h for documentation
 */
void jsr120_cbs_message_arrival_notifier(CbsMessage* message) {

    WMA_STATUS unblocked = WMA_ERR;

    /*
     * First invoke listeners for current midlet
     */
    if(cbs_midlet_listeners != NULL) {
        unblocked = jsr120_cbs_invoke_listeners(message, cbs_midlet_listeners);
    }

    /*
     * If a listener hasn't been invoked, try the push Listeners
     */
    if (unblocked == WMA_ERR && cbs_push_listeners != NULL) {
        unblocked = jsr120_cbs_invoke_listeners(message, cbs_push_listeners);
    }
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_is_midlet_msgID_registered(jchar msgID) {
    return jsr120_cbs_is_msgID_registered(msgID, cbs_midlet_listeners);
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_register_midlet_msgID(jchar msgID,
    SuiteIdType msid, jint handle) {

    return jsr120_cbs_register_msgID(msgID, msid, jsr120_cbs_midlet_listener,
        (void *)handle, &cbs_midlet_listeners);
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_unregister_midlet_msgID(jchar msgID) {

    return jsr120_cbs_unregister_msgID(msgID, jsr120_cbs_midlet_listener,
        &cbs_midlet_listeners);
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_is_push_msgID_registered(jchar msgID) {
    return jsr120_cbs_is_msgID_registered(msgID, cbs_push_listeners);
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_register_push_msgID(jchar msgID, SuiteIdType msid,
    jint handle) {

    return jsr120_cbs_register_msgID(msgID, msid, jsr120_cbs_push_listener,
        (void *)handle, &cbs_push_listeners);
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_unregister_push_msgID(jchar msgID) {
    return jsr120_cbs_unregister_msgID(msgID, jsr120_cbs_push_listener,
        &cbs_push_listeners);
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_unblock_thread(jint handle, jint waitingFor) {
    JVMSPI_ThreadID id =
        jsr120_cbs_get_blocked_thread_from_handle((long)handle, waitingFor);
    if (id != 0) {
	midp_thread_unblock(id);
	return WMA_OK;
    }

    return WMA_ERR;
}

/**
 * Find a first thread that can be unblocked for a given handle and signal type.
 *
 * @param handle Platform-specific handle.
 * @param signalType Enumerated signal type.
 *
 * @return JVMSPI_ThreadID Java thread id than can be unblocked
 *         0 if no matching thread can be found
 *
 */
static JVMSPI_ThreadID
jsr120_cbs_get_blocked_thread_from_handle(long handle, jint waitingFor) {
    JVMSPI_BlockedThreadInfo *blocked_threads;
    int n;
    int i;

    blocked_threads = SNI_GetBlockedThreads(&n);

    for (i = 0; i < n; i++) {
	MidpReentryData *p =
            (MidpReentryData*)(blocked_threads[i].reentry_data);
	if (p != NULL) {

	    /* wait policy: 1. threads waiting for network reads
                            2. threads waiting for network writes
         	            3. threads waiting for network push event*/
	    if ((waitingFor == WMA_CBS_READ_SIGNAL) &&
                (waitingFor == (int)p->waitingFor) &&
         	(p->descriptor == handle)) {
		return blocked_threads[i].thread_id;
            }

            if ((waitingFor == PUSH_SIGNAL) &&
                (waitingFor == (int)p->waitingFor) &&
                (findPushBlockedHandle(handle) != 0)) {
                return blocked_threads[i].thread_id;
	    }

	}

    }

    return 0;
}

/*
 * The listener that should be called when a CBS message is added to the in-box.
 *
 * @param port The CBS port we are listening to.
 * @param message The CBS message that was received.
 * @param userData A pointer to user data, if any, that was cached in the
 *     in-box. This is data that was passed to the in-box, when a port is
 *     registered with it. Usually a handle to the open connection.
 * @return <code>WMA_OK</code> if a waiting thread is successfully unblocked;
 *     <code>WMA_ERR</code>, otherwise.
 */
static WMA_STATUS jsr120_cbs_midlet_listener(CbsMessage* message, void* userData) {
    (void)message;

    /** unblock the receiver thread here */
    return jsr120_cbs_unblock_thread((int)userData, WMA_CBS_READ_SIGNAL);

}

/*
 * The listener that should be called when a CBS message is added to the push
 * registry.
 *
 * @param port CBS port we are listening to
 * @param message The CBS message that was received.
 * @param userData A pointer to user data, if any, that was cached in the in-box.
 *     This is data that was passed to the in-box, when a port is registered with
 *     it. Usually a handle to the open connection.
 * @return <code>WMA_OK</code> if a waiting thread is successfully unblocked;
 *     <code>WMA_ERR</code>, otherwise.
 */
static WMA_STATUS jsr120_cbs_push_listener(CbsMessage* message, void* userData)
{
    (void)message;

    /** unblock the receiver thread here */
    return jsr120_cbs_unblock_thread((int)userData, PUSH_SIGNAL);
}

/**
 * Listen for messages that match a specific message identifier.
 * <P>
 * This function calls the native API to listen for incoming messages and
 * optionally registers a user-supplied callback. The callback is invoked when
 * a message has has added to the message pool.
 * <P>
 * The callback function will be called with the incoming CBS and the user
 * supplied data (userData).
 * <P>
 * The message is retained in the message pool until
 * <code>jsr120_cbs_pool_get_next_msg()</code> is called.
 *
 * When <code>NULL</code> is supplied as a callback function, messages will be
 * added to the message pool, but no listener will be called..
 *
 * @param msgID The message identifier  to be matched.
 * @param listener The CBS listener.
 * @param userData Any special data associated with the listener.
 * @param listeners List of listeners in which to be registered.
 *
 * @return <code>WMA_OK</code> if successful; <code>WMA_ERR</code> if the
 *     identifier has already been registered or if native registration failed.
 */
static WMA_STATUS jsr120_cbs_register_msgID(jchar msgID,
    SuiteIdType msid, cbs_listener_t* listener, void* userData,
    ListElement **listeners) {

    /* Assume no success in registering the message ID. */
    WMA_STATUS ok = WMA_ERR;

    if (jsr120_cbs_is_msgID_registered(msgID, *listeners) == WMA_ERR) {
	ok = jsr120_add_cbs_listening_msgID(msgID);
	jsr120_list_new_by_number(listeners, msgID, msid, userData, (void*)listener);
    }

    return ok;
}

/**
 * Stop listening for CBS messages that match a message ID. The native API is
 * called to stop listening for incoming CBS messages, and the registered
 * listener is unregistered.
 *
 * @param msgID		The message ID used for matching IDs.
 * @param listener    The listener to be unregistered.
 * @param userData    Any special data associated with the listener.
 * @param listeners List of listeners from which to be unregistered.
 *
 * @return <code>WMA_OK</code> if successful; <code>WMA_ERR</code>,
 *     otherwise.
 */
static WMA_STATUS jsr120_cbs_unregister_msgID(jchar msgID,
    cbs_listener_t* listener, ListElement **listeners) {

    /* Assume no success in unregistering the message ID */
    WMA_STATUS ok = WMA_ERR;

    if (jsr120_cbs_is_msgID_registered(msgID, *listeners) == WMA_OK) {

	jsr120_list_unregister_by_number(listeners, msgID, (void*)listener);
	if (jsr120_cbs_is_msgID_registered(msgID, *listeners) == WMA_ERR) {
            ok = jsr120_remove_cbs_listening_msgID(msgID);
	}

    }
    return ok;
}

/**
 * Check if a message identifier is currently registered.
 *
 * @param msgID	The message identifier to be matched.
 * @param listeners List of listeners to check.
 *
 * @return <code>WMA_OK</code> if the message identifier has an associated
 *     listener; <code>WMA_ERR</code>, otherwise.
 *
 */
static WMA_STATUS jsr120_cbs_is_msgID_registered(jchar msgID,
    ListElement *listeners) {

    return jsr120_list_get_by_number(listeners, msgID) != NULL ? WMA_OK : WMA_ERR;
}

/**
 * Deletes all CBS messages cached in the pool that match the MIDlet suite
 * identifier.
 *
 * @param msid The MIDlet suite identifier.
 */
void jsr120_cbs_delete_midlet_suite_msg(SuiteIdType msid) {
    jsr120_cbs_delete_all_msgs(msid, cbs_midlet_listeners);
}

/**
 * Deletes all CBS messages cached in the pool by the push subsystem, that match
 * the MIDlet suite identifier.
 *
 * @param msid The MIDlet suite identifier.
 */
void jsr120_cbs_delete_push_msg(SuiteIdType msid) {
    jsr120_cbs_delete_all_msgs(msid, cbs_push_listeners);
}

/**
 * Delete all CBS messages cached in the pool for the specified MIDlet suite.
 * The linked list with the (msid, msg id) pairings has to be specified.
 *
 * @param msid The MIDlet Suite identifier.
 * @param head Head of linked list, that has (MIDlet suite identifier,
 *     message identifier) pairings.
 *
 */
static void jsr120_cbs_delete_all_msgs(SuiteIdType msid, ListElement* head) {

    ListElement *elem = NULL;

    if ((elem = jsr120_list_get_first_by_msID(head, msid)) != NULL) {
        /*
         * If the dequeued element has a valid msg id,
         * then delete all CBS messages stored for msg id.
         */
        if (elem->id > 0) {
            jsr120_cbs_pool_remove_all_msgs(elem->id);
        }
    }
}

