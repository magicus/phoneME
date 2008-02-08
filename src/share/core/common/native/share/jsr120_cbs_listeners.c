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
#include <kni.h>
#include <sni.h>

#ifdef ENABLE_MIDP
#if (ENABLE_CDC != 1) 
  #include <midp_thread.h> // midp_thread_unblock
  #include <push_server_resource_mgmt.h> // pushsetcachedflag, pushgetfilter
#endif
  #include <midpServices.h>
  #include <push_server_export.h>  // findPushBlockedHandle
#else
  #include "wmaInterface.h"
#endif

#if (ENABLE_CDC == 1)
#ifdef JSR_120_ENABLE_JUMPDRIVER
  #include <jsr120_jumpdriver.h>
  #include <JUMPEvents.h>
#else
  #include "jsr120_signals.h"
#endif
#endif

#include <jsr120_list_element.h>
#include <jsr120_cbs_pool.h>
#include <jsr120_cbs_listeners.h>
#include <jsr120_cbs_protocol.h>
#include <app_package.h>
#include <wmaPushRegistry.h> // jsr120_check_filter

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
    for(callback = jsr120_list_get_by_number(listeners, message->msgID);
	callback != NULL;
	callback = jsr120_list_get_by_number(callback->next, message->msgID)) {

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
void jsr120_cbs_message_arrival_notifier(CbsMessage* cbsMessage) {

    WMA_STATUS unblocked = WMA_ERR;

    /*
     * First invoke listeners for current midlet
     */
    if(cbs_midlet_listeners != NULL) {
        unblocked = jsr120_cbs_invoke_listeners(cbsMessage, cbs_midlet_listeners);
    }

    /*
     * If a listener hasn't been invoked, try the push Listeners
     */
    if (unblocked == WMA_ERR && cbs_push_listeners != NULL) {
        pushsetcachedflag("cbs://:", cbsMessage->msgID);
        unblocked = jsr120_cbs_invoke_listeners(cbsMessage, cbs_push_listeners);
    }
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_is_message_expected(jchar msgID) {

    if (WMA_OK == jsr120_cbs_is_midlet_listener_registered(msgID)) {
        return WMA_OK;
    }

    if (WMA_OK == jsr120_cbs_is_push_listener_registered(msgID)) {
        return WMA_OK;
    }

    return WMA_ERR;
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
static WMA_STATUS jsr120_cbs_is_listener_registered(jchar msgID,
    ListElement *listeners) {

    return jsr120_list_get_by_number(listeners, msgID) != NULL ? WMA_OK : WMA_ERR;
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_is_midlet_listener_registered(jchar msgID) {
    return jsr120_cbs_is_listener_registered(msgID, cbs_midlet_listeners);
}

/**
 * Check if an message ID is currently registered for given message identifier
 *
 * @param msgID the message ID to check
 * @param listeners List of listeners in which to check
 * @param msid suite id to check for
 *
 * @return <code>WMA_OK</code> if a given application is listening for this ID,
 *         <code>WMA_ERR</code> otherwise
 *
 */
WMA_STATUS jsr120_cbs_is_listener_registered_by_msid(jchar msgID,
    ListElement *listeners, AppIdType msid) {

    ListElement *entry = jsr120_list_get_by_number(listeners, msgID);
    
    return entry != NULL && entry->msid == msid ? WMA_OK : WMA_ERR;
}

#if (ENABLE_CDC != 1)

/**
 * See implementation in jsr120_sms_listeners.c
 */
extern WMA_STATUS jsr120_unblock_midp_threads(long handle, jint waitingFor, WMA_STATUS status);

/**
 * Ublock threads waiting for these signals:
 *   WMA_CBS_READ_SIGNAL
 *   PUSH_SIGNAL
 *
 * @param handle Platform specific handle
 * @param signalType Enumerated signal type
 *
 * @result returns <code>WMA_STATUS</code> if waiting threads was
 *         successfully unblocked
 */
static WMA_STATUS jsr120_unblock_cbs_read_threads(long handle, jint waitingFor) {
    WMA_STATUS ok = WMA_ERR;

    if (waitingFor == WMA_CBS_READ_SIGNAL) {
        ok = jsr120_unblock_midp_threads(handle, waitingFor, WMA_OK);
    }

    if (ok == WMA_OK) {
        return ok; 
    }

    if (waitingFor == PUSH_SIGNAL) {
        if (findPushBlockedHandle(handle) != 0) {
            ok = jsr120_unblock_midp_threads(0, waitingFor, WMA_OK);
        }
    }

    return ok;
}
#endif

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_unblock_thread(jint handle, jint waitingFor) {
#if (ENABLE_CDC != 1)
    return jsr120_unblock_cbs_read_threads(handle, waitingFor);
#else
#ifdef JSR_120_ENABLE_JUMPDRIVER
    JUMPEvent evt = (JUMPEvent) handle;
    if (jumpEventHappens(evt) >= 0) {
        return WMA_OK;
    }
#else
    jsr120_throw_signal(handle, waitingFor);
#endif
#endif
    return WMA_ERR;
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
    WMA_STATUS status;
    (void)message;

    /** unblock the receiver thread here */
#ifdef JSR_120_ENABLE_JUMPDRIVER
    INVOKE_REMOTELY(status, jsr120_cbs_unblock_thread, ((int)userData, WMA_CBS_READ_SIGNAL));
#else
    status = jsr120_cbs_unblock_thread((int)userData, WMA_CBS_READ_SIGNAL);
#endif
    return status;

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
static WMA_STATUS jsr120_cbs_register_listener(jchar msgID,
    AppIdType msid, cbs_listener_t* listener, void* userData,
    ListElement **listeners, jboolean registerMsgID) {

    /* Assume no success in registering the message ID. */
    WMA_STATUS ok = WMA_ERR;

    if (jsr120_cbs_is_listener_registered(msgID, *listeners) == WMA_ERR) {
        ok = WMA_OK;
        if (registerMsgID) {
            ok = jsr120_add_cbs_listening_msgID(msgID);
        }
	jsr120_list_new_by_number(listeners, msgID, msid, userData, (void*)listener);
    }

    return ok;
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_register_midlet_listener(jchar msgID,
    AppIdType msid, jint handle) {

    jboolean isPushRegistered = jsr120_cbs_is_listener_registered_by_msid(
        msgID, cbs_push_listeners, msid) == WMA_OK;

    return jsr120_cbs_register_listener(msgID, msid, jsr120_cbs_midlet_listener,
                                        (void *)handle, &cbs_midlet_listeners,
                                        !isPushRegistered);
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
static WMA_STATUS jsr120_cbs_unregister_listener(jchar msgID,
    cbs_listener_t* listener, ListElement **listeners,
    jboolean unregisterMsgID) {

    /* Assume no success in unregistering the message ID */
    WMA_STATUS ok = WMA_ERR;

    if (jsr120_cbs_is_listener_registered(msgID, *listeners) == WMA_OK) {

	jsr120_list_unregister_by_number(listeners, msgID, (void*)listener);
	if (jsr120_cbs_is_listener_registered(msgID, *listeners) == WMA_ERR &&
            unregisterMsgID) {
            ok = jsr120_remove_cbs_listening_msgID(msgID);
	}

    }
    return ok;
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_unregister_midlet_listener(jchar msgID) {
    /*
     * As there was open connection push can be registered only for current suite
     * thus no need to check for suite ID
     */
    jboolean hasNoPushRegistration = jsr120_cbs_is_push_listener_registered(msgID) == WMA_ERR;
    
    return jsr120_cbs_unregister_listener(msgID, jsr120_cbs_midlet_listener,
                                          &cbs_midlet_listeners, hasNoPushRegistration);
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_is_push_listener_registered(jchar msgID) {
    return jsr120_cbs_is_listener_registered(msgID, cbs_push_listeners);
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_register_push_listener(jchar msgID, AppIdType msid,
    jint handle) {

    jboolean isMIDletRegistered = jsr120_cbs_is_listener_registered_by_msid(
        msgID, cbs_midlet_listeners, msid) == WMA_OK;


    return jsr120_cbs_register_listener(msgID, msid, jsr120_cbs_push_listener,
                                        (void *)handle, &cbs_push_listeners,
                                        !isMIDletRegistered);
}

/*
 * See jsr120_cbs_listeners.h for documentation
 */
WMA_STATUS jsr120_cbs_unregister_push_listener(jchar msgID) {
    /*
     * As there was push push registration connection can be open only for current suite
     * thus no need to check for suite ID
     */
    jboolean hasNoConnection = jsr120_cbs_is_midlet_listener_registered(msgID) == WMA_ERR;

    return jsr120_cbs_unregister_listener(msgID, jsr120_cbs_push_listener,
                                          &cbs_push_listeners, hasNoConnection);
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
static void jsr120_cbs_delete_all_msgs(AppIdType msid, ListElement* head) {

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

/**
 * Deletes all CBS messages cached in the pool that match the MIDlet suite
 * identifier.
 *
 * @param msid The MIDlet suite identifier.
 */
void jsr120_cbs_delete_midlet_suite_msg(AppIdType msid) {
    jsr120_cbs_delete_all_msgs(msid, cbs_midlet_listeners);
}

/**
 * Deletes all CBS messages cached in the pool by the push subsystem, that match
 * the MIDlet suite identifier.
 *
 * @param msid The MIDlet suite identifier.
 */
void jsr120_cbs_delete_push_msg(AppIdType msid) {
    jsr120_cbs_delete_all_msgs(msid, cbs_push_listeners);
}



