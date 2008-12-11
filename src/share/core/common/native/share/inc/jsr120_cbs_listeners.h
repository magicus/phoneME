/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _JSR120_CBS_LISTENERS_H
#define _JSR120_CBS_LISTENERS_H

#include <jsr120_cbs_structs.h>
// #include <suitestore_common.h>
#include <app_package.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This method is called by the pool (in-box) code as soon as a CBS message has
 * been added to to the message pool.
 *
 * @param message The message that has just arrived at the in-box.
 */
void jsr120_cbs_message_arrival_notifier(CbsMessage* message);

/**
 * Checks whether the specified port has been registered by midlet 
 * or push registry for receiving SMS messages.
 *
 * @param mdgID The message identifier to be checked
 */
WMA_STATUS jsr120_cbs_is_message_expected(jchar msgID);

/**
 * Checks whether the message identifier has been registered by this or another
 * MIDlet for receiving CBS messages.
 *
 * @param msgID The CBS message identifier to be registered.
 *
 * @return <code>WMA_OK</code> if the message identifier is registered;
 *     <code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_cbs_is_midlet_listener_registered(jchar msgID);

/**
 * Registers the CBS message identifier for the calling MIDlet.
 *
 * @param msgID The message identifier to be registered.
 * @param msid The MIDlet suite identifier.
 * @param handle A handle to the open CBS connection.
 *
 * @return <code>WMA_OK</code> if the message identifier is registered;
 *     <code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_cbs_register_midlet_listener(jchar msgID,
    AppIdType msid, jint handle);

/**
 * Unregisters the CBS message identifier for the calling MIDlet.
 *
 * @param msgID The message identifier to be unregistered.
 *
 * @return <code>WMA_OK</code> if the message identifier is unregistered;
 *     <code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_cbs_unregister_midlet_listener(jchar msgID);

/**
 * Checks whether the CBS message identifier has been registered by the push
 * registry for receiving CBS messages.
 *
 * @param msgID The message identifier to be registered.
 *
 * @return <code>WMA_OK</code> if the message identifier is registered;
 *     <code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_cbs_is_push_listener_registered(jchar msgID);

/**
 * Registers the CBS message identifier with the push registry.
 *
 * @param msgID The message identifier to be registered.
 * @param msid The MIDlet suite identifier.
 * @param handle A handle to the open CBS connection.
 *
 * @return <code>WMA_OK</code> if the message identifier is registered;
 *     <code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_cbs_register_push_listener(jchar msgID, AppIdType msid,
    jint handle);

/**
 * Unregisters the CBS message identifier from the push registry.
 *
 * @param msgID The message identifier to be unregistered.
 *
 * @return <code>WMA_OK</code> if the message identifier is unregistered;
 *     <code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_cbs_unregister_push_listener(jchar msgID);

/**
 * Unblocks the thread that matches the specified handle
 * and signal
 *
 * @param handle handle to open CBS connection
 * @param waitingFor signal that thread is waiting for
 *
 * @result returns true if a matching thread is unblocked,
 *                 false otherwise
 */
WMA_STATUS jsr120_cbs_unblock_thread(jint handle, jint waitingFor);

/**
 * Deletes all CBS messages cached in the pool that match the MIDlet suite
 * identifier.
 *
 * @param msid The MIDlet suite identifier.
 */
void jsr120_cbs_delete_midlet_suite_msg(AppIdType msid);

/**
 * Deletes all CBS messages cached in the pool by the push subsystem, that match
 * the MIDlet suite identifier.
 *
 * @param msid The MIDlet suite identifier.
 */
void jsr120_cbs_delete_push_msg(AppIdType msid);

#ifdef __cplusplus
}
#endif

#endif /* #ifdef _JSR120_CBS_LISTENERS_H_ */
