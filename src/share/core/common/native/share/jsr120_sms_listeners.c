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
  #include "jsr120_signals.h"
#endif

#include <jsr120_list_element.h>
#include <jsr120_sms_pool.h>
#include <jsr120_sms_listeners.h>
#include <jsr120_sms_protocol.h>
#include <app_package.h>
#include <wmaPushRegistry.h> // jsr120_check_filter

/*
 * Listeners registered by a currently running midlet
 */
static ListElement* sms_midlet_listeners=NULL;

/*
 * Listeners registered by push subsystem
 */
static ListElement* sms_push_listeners=NULL;

typedef WMA_STATUS sms_listener_t(jint port, SmsMessage* SmsMessage, void* userData);

#if (ENABLE_CDC != 1)
/**
 * Ublock midp threads that can be unblocked for a given handle
 * and signal type.
 *
 * @param handle Platform specific handle
 * @param signalType Enumerated signal type
 * @param status Status to set to blocked thread reentry data
 *
 * @result returns <code>WMA_STATUS</code> if waiting threads was
 *         successfully unblocked
 */
WMA_STATUS jsr120_unblock_midp_threads(long handle, jint waitingFor, WMA_STATUS status) {

    jint i, num_threads;
    JVMSPI_BlockedThreadInfo *blocked_threads = SNI_GetBlockedThreads(&num_threads);
    WMA_STATUS ok = WMA_ERR;

    for (i = 0; i < num_threads; i++) {
	MidpReentryData *p =
            (MidpReentryData*)(blocked_threads[i].reentry_data);
	if (p != NULL) {
	    if ((waitingFor == (int)p->waitingFor) &&
         	((p->descriptor == handle) || (handle == 0))) {
                p->status = status;
                midp_thread_unblock(blocked_threads[i].thread_id);
                ok = WMA_OK;
            }
        }
    }

    return ok;
}

/**
 * Ublock threads waiting for these signals:
 *   WMA_SMS_READ_SIGNAL
 *   PUSH_SIGNAL
 *
 * @param handle Platform specific handle
 * @param signalType Enumerated signal type
 *
 * @result returns <code>WMA_STATUS</code> if waiting threads was
 *         successfully unblocked
 */
static WMA_STATUS jsr120_unblock_sms_read_threads(long handle, jint waitingFor) {
    WMA_STATUS ok = WMA_ERR;

    if (waitingFor == WMA_SMS_READ_SIGNAL) {
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

/**
 * Invoke registered listeners for the port specified in the SMS
 * message.
 *
 * @param sms Incoming SMS message
 * @listeners list of registered listeners
 *
 * @result returns <code>WMA_OK</code> if a matching listener is invoked,
 *                 <code>WMA_ERR</code> otherwise
 */
static WMA_STATUS jsr120_invoke_sms_listeners(SmsMessage* sms, ListElement *listeners) {
    ListElement* callback;
    WMA_STATUS unblocked = WMA_ERR;

    for(callback = jsr120_list_get_by_number(listeners, sms->destPortNum);
	callback != NULL;
	callback = jsr120_list_get_by_number(callback->next, sms->destPortNum)) {
	sms_listener_t* listener=(sms_listener_t*)(callback->userDataCallback);
	if (listener != NULL) {
            if((unblocked =
                listener(sms->destPortNum, sms, callback->userData)) == WMA_OK) {
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
 * See jsr120_sms_listeners.h for documentation
 */
void jsr120_sms_message_arrival_notifier(SmsMessage* smsMessage) {

    WMA_STATUS unblocked = WMA_ERR;

    /*
     * First invoke listeners for current midlet
     */
    if (sms_midlet_listeners != NULL) {
        unblocked = jsr120_invoke_sms_listeners(smsMessage, sms_midlet_listeners);
    }

    /*
     * If a listener hasn't been invoked, try the push Listeners
     */
    if (unblocked == WMA_ERR && sms_push_listeners != NULL) {
        pushsetcachedflag("sms://:", smsMessage->destPortNum);
        unblocked = jsr120_invoke_sms_listeners(smsMessage, sms_push_listeners);
    }

}

/*
 * See jsr120_sms_listeners.h for documentation
 */
void jsr120_sms_message_sent_notifier(int handle, WMA_STATUS result) {
#if (ENABLE_CDC != 1)
    jsr120_unblock_midp_threads(handle, WMA_SMS_WRITE_SIGNAL, result);
#endif
}

/*
 * See jsr120_sms_listeners.h for documentation
 */
WMA_STATUS jsr120_sms_is_message_expected(jchar port, char* addr) {

    if (WMA_OK == jsr120_is_sms_midlet_listener_registered(port)) {
        return WMA_OK;
    }

    if (WMA_OK == jsr120_is_sms_push_listener_registered(port)) {
        char* filter = pushgetfilter("sms://:", port);
        if (filter == NULL || jsr120_check_filter(filter, addr)) {
            return WMA_OK;
        }
    }

    return WMA_ERR;
}

/**
 * Check if an SMS port is currently registered
 *
 * @param smsPort the SMS port to check
 * @param listeners List of listeners in which to check
 *
 * @return <code>WMA_OK</code> if a user is listening to this port ,
 *         <code>WMA_ERR</code> otherwise
 *
 */
static WMA_STATUS jsr120_is_sms_listener_registered(jchar smsPort, ListElement *listeners) {
    return ((jsr120_list_get_by_number(listeners,smsPort) != NULL) ? WMA_OK : WMA_ERR);
}

/*
 * See jsr120_sms_listeners.h for documentation
 */
WMA_STATUS jsr120_is_sms_midlet_listener_registered(jchar port) {
    return jsr120_is_sms_listener_registered(port, sms_midlet_listeners);
}

/**
 * Check if an SMS port is currently registered for given msid
 *
 * @param smsPort the SMS port to check
 * @param listeners List of listeners in which to check
 * @param msid suite id to check for
 *
 * @return <code>WMA_OK</code> if a given application is listening to this port,
 *         <code>WMA_ERR</code> otherwise
 *
 */
static WMA_STATUS jsr120_is_sms_listener_registered_by_msid(
    jchar smsPort, ListElement *listeners, AppIdType msid) {
    ListElement *entry = jsr120_list_get_by_number(listeners, smsPort);
    return entry != NULL && entry->msid == msid ? WMA_OK : WMA_ERR;
}

/*
 * Listener that should be called, when a SMS message is
 * is added to the inbox.
 *
 * @param port SMS port we are listening to
 * @param wma_smsstruct SMS message that was received
 * @param userData pointer to user data, if any, that was
 *                 cached in the inbox. This is data that was passed
 *                 to the inbox, when a port is regsitered with it.
 *                 Usually a handle to the open connection
 * @result returns <code>WMA_STATUS</code> if a waiting thread is
 *         successfully unblocked
 */
static WMA_STATUS jsr120_sms_midlet_listener(jint port, SmsMessage* smsmessg,
                                                void* userData) {
    WMA_STATUS status;
    (void)port;
    (void)smsmessg;

    /** unblock the receiver thread here */
    status = jsr120_sms_unblock_thread((jint)userData, WMA_SMS_READ_SIGNAL);
    return status;
}

/**
 * Listen for SMS messages on a given port.
 * This function optionally calls native API to listen to incoming SMS messages and
 * optionally registers a user supplied callback when a message has arrived and
 * has been inserted into the SMS pool.
 * The callback function will be called with the incoming SMS and the user supplied
 * data (userData).
 * The SMS is kept in the SMS pool until function SMSPool_getNextSms is called.
 *
 * If KNI_FALSE is passed as value of registerPort no native API will be
 * called to register this port. This is important when port if already registered
 * for given MIDlet (when opening connection on port which has push registration)
 * If NULL is sent as a callback function, no listener will be called, and the SMS
 * is inserted into the SMS pool.
 *
 * @param smsPort the SMS port to listen to
 * @param listener listener to be invoked on message arrival
 * @param userData
 * @param listeners List of listeners in which to be registered.
 * @param registerPort set if need to register Java runtime within
 *         platform to receive notifications on message arrival 
 *
 * @return <code>WMA_OK</code> if successful,
 *         <code>WMA_ERR</code> if port already registered or
 *	    native registration failed.
 *
 */
static WMA_STATUS jsr120_register_sms_listener(
    jchar smsPort, AppIdType msid, sms_listener_t* listener, void* userData,
    ListElement **listeners, jboolean registerPort) {

    WMA_STATUS ok = WMA_ERR;
    if (jsr120_is_sms_listener_registered(smsPort, *listeners) == WMA_ERR) {
        ok = WMA_OK;
        if (registerPort) {
            ok = jsr120_add_sms_listening_port(smsPort);
        }
	jsr120_list_new_by_number(listeners, smsPort, msid, userData, (void*)listener);
    }
    return ok;
}

/**
 * Stop listening for SMS messages on a given port.
 * This function optionally calls native API to stop listening to incoming
 * SMS messages and unregister user's callback
 *
 * @param smsPort the SMS port to listen to
 * @param listener
 * @param userData
 * @param listeners List of listeners from which to be unregistered.
 * @param unregisterPort set if need to unregister Java runtime within
 *         platform to stop listening to incoming messages
 *
 * @return <code>WMA_OK</code> if successful,
 *         <code>WMA_ERR</code> otherwise
 *
 */
static WMA_STATUS jsr120_unregister_sms_listener(
    jchar smsPort, sms_listener_t* listener, ListElement **listeners,
    jboolean unregisterPort) {
    
    WMA_STATUS ok = WMA_ERR;

    if (jsr120_is_sms_listener_registered(smsPort, *listeners) == WMA_OK) {
	ok = WMA_OK;
        jsr120_list_unregister_by_number(listeners, smsPort, (void*)listener);
	if (jsr120_is_sms_listener_registered(smsPort, *listeners) == WMA_ERR &&
            unregisterPort) {
            ok = jsr120_remove_sms_listening_port(smsPort);
	}

    }
    return ok;
}

/*
 * Listener that should be called, when a SMS message is
 * is added to the inbox.
 *
 * @param port SMS port we are listening to
 * @param wma_smsstruct SMS message that was received
 * @param userData pointer to user data, if any, that was
 *                 cached in the inbox. This is data that was passed
 *                 to the inbox, when a port is regsitered with it.
 *                 Usually a handle to the open connection
 * @result returns <code>WMA_STATUS</code> if a waiting thread is
 *         successfully unblocked
 */
static WMA_STATUS jsr120_sms_push_listener(jint port, SmsMessage* smsmessg,
                                              void* userData)
{
    (void)port;
    (void)smsmessg;

    /** unblock the receiver thread here */
    return jsr120_sms_unblock_thread((jint)userData, PUSH_SIGNAL);
}

/*
 * See jsr120_sms_listeners.h for documentation
 */
WMA_STATUS jsr120_register_sms_midlet_listener(jchar port,
                                              AppIdType msid,
                                              jint handle) {
    jboolean isPushRegistered = jsr120_is_sms_listener_registered_by_msid(
        port, sms_push_listeners, msid) == WMA_OK;

    return jsr120_register_sms_listener(port, msid, jsr120_sms_midlet_listener,
                                        (void *)handle,
                                        &sms_midlet_listeners,
                                        !isPushRegistered);
}

/*
 * See jsr120_sms_listeners.h for documentation
 */
WMA_STATUS jsr120_unregister_sms_midlet_listener(jchar port) {
    /*
     * As there was open connection push can be registered only for current suite
     * thus no need to check for suite ID
     */
    jboolean hasNoPushRegistration = jsr120_is_sms_push_listener_registered(port) == WMA_ERR;

    return jsr120_unregister_sms_listener(port, jsr120_sms_midlet_listener,
                                          &sms_midlet_listeners, hasNoPushRegistration);
}

/*
 * See jsr120_sms_listeners.h for documentation
 */
WMA_STATUS jsr120_is_sms_push_listener_registered(jchar port) {
    return jsr120_is_sms_listener_registered(port, sms_push_listeners);
}

/*
 * See jsr120_sms_listeners.h for documentation
 */
WMA_STATUS jsr120_register_sms_push_listener(jchar port,
                             AppIdType msid, jint handle) {

    jboolean isMIDletRegistered = jsr120_is_sms_listener_registered_by_msid(
        port, sms_midlet_listeners, msid) == WMA_OK;

    return jsr120_register_sms_listener(port, msid, jsr120_sms_push_listener,
                                        (void *)handle,
                                        &sms_push_listeners,
                                        !isMIDletRegistered);
}

/*
 * See jsr120_sms_listeners.h for documentation
 */
WMA_STATUS jsr120_unregister_sms_push_listener(jchar port) {
    /*
     * As there was push push registration connection can be open only for current suite
     * thus no need to check for suite ID
     */
    jboolean hasNoConnection = jsr120_is_sms_midlet_listener_registered(port) == WMA_ERR;

    return jsr120_unregister_sms_listener(port, jsr120_sms_push_listener, &sms_push_listeners,
                                          hasNoConnection);
}

/*
 * See jsr120_sms_listeners.h for documentation
 */
WMA_STATUS jsr120_sms_unblock_thread(jint handle, jint waitingFor) {
#if (ENABLE_CDC != 1)
    return jsr120_unblock_sms_read_threads(handle, waitingFor);
#else
    jsr120_throw_signal(handle, waitingFor);
#endif
    return WMA_ERR;

}

/**
 * Delete all SMS messages cached in the pool for the specified
 * midlet suite. The linked list with the (msid, port number)
 * pairings has to be specified.
 *
 * @param msid Midlet Suite ID.
 * @param head Head of linked list, that has (msid, port number)
 *             pairings.
 *
 */
static void jsr120_sms_delete_all_msgs(AppIdType msid, ListElement* head) {

    ListElement *elem = NULL;

    if ((elem = jsr120_list_get_first_by_msID(head, msid)) != NULL) {
        /*
         * If the dequeued element has a valid port number,
         * then delete all SMS messages stored for that port.
         */
        if (elem->id > 0) {
            jsr120_sms_pool_remove_all_msgs(elem->id);
        }
    }
}

/**
 * Delete all SMS messages cached in the pool for the specified
 * midlet suite
 *
 * @param msid Midlet Suite ID.
 *
 */
void jsr120_sms_delete_midlet_suite_msg(AppIdType msid) {
    jsr120_sms_delete_all_msgs(msid, sms_midlet_listeners);
}

/**
 * Delete all SMS messages cached in the pool for the specified
 * midlet suite, for the Push subsystem.
 *
 * @param msid Midlet Suite ID.
 *
 */
void jsr120_sms_delete_push_msg(AppIdType msid) {
    jsr120_sms_delete_all_msgs(msid, sms_push_listeners);
}



