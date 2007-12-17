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

#ifndef _JSR120_SMS_LISTENERS_H
#define _JSR120_SMS_LISTENERS_H

#include <jsr120_types.h>
#include <jsr120_sms_structs.h>
// #include <suitestore_common.h>
#include "app_package.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is the method that gets called by the Pool/Inbox, as soon as
 * a SMS message has been added to it.
 *
 * @param smsMessage The message that has just arrived at the inbox
 *
 */
void jsr120_sms_message_arrival_notifier(SmsMessage* smsMessage);

/**
 * Checks whether the specified port has been registered by midlet 
 * or push registry for receiving SMS messages. For the case of
 * push registry checks that sourceAddress matches push filter.
 *
 * @param port SMS port to be registered
 * @param addr SMS source address
 */
WMA_STATUS jsr120_sms_is_message_expected(jchar port, char* addr);

/**
 * This is the method that gets called as soon as
 * a SMS message is sent.
 *
 * @param handle of SMS
 * @param status indication of send completed status result: Either
 *         <tt>WMA_OK</tt> on success,
 *         <tt>WMA_ERR</tt> on failure
 */
void jsr120_sms_message_sent_notifier(int handle, WMA_STATUS status);

/**
 * Checks whether the specified port has been registered by this
 * or another midlet, for receiving SMS messages.
 *
 * @param port SMS port to be registered
 *
 * @result returns <code>WMA_OK</code> if port is registered,
 *                 <code>WMA_ERR</code> otherwise.
 *
 */
WMA_STATUS jsr120_is_sms_midlet_listener_registered(jchar port);

/**
 * Registers the specified SMS port for the calling midlet
 *
 * @param port SMS port to be registered
 * @param msid Midlet Suite ID
 * @param handle A handle to the open SMS connection
 *
 * @result returns <code>WMA_OK</code> if port is registered,
 *                 <code>WMA_ERR</code> otherwise.
 */
WMA_STATUS jsr120_register_sms_midlet_listener(jchar port,
                        AppIdType msid, jint handle);

/**
 * Unregister the specified SMS port for the calling midlet
 *
 * @param port SMS port to be unregistered
 *
 * @result returns <code>WMA_OK</code> if port is unregistered,
 *                 <code>WMA_ERR</code> otherwise.
 */
WMA_STATUS jsr120_unregister_sms_midlet_listener(jchar port);

/**
 * Checks whether the specified port has been registered by the
 * Push registry, for receiving SMS messages.
 *
 * @param port SMS port to be registered
 *
 * @result returns <code>WMA_OK</code> if port is registered,
 *                 <code>WMA_ERR</code> otherwise.
 *
 */
WMA_STATUS jsr120_is_sms_push_listener_registered(jchar port);

/**
 * Registers the specified SMS port for the Push registry
 *
 * @param port SMS port to be registered
 * @param msid Midlet Suite ID
 * @param handle A handle to the open SMS connection
 *
 * @result returns <code>WMA_OK</code> if port is registered,
 *                 <code>WMA_ERR</code> otherwise.
 */
WMA_STATUS jsr120_register_sms_push_listener(jchar port,
                     AppIdType msid, jint handle);

/**
 * Unregister the specified SMS port for the Push registry
 *
 * @param port SMS port to be unregistered
 *
 * @result returns <code>WMA_OK</code> if port is unregistered,
 *                 <code>WMA_ERR</code> otherwise.
 */
WMA_STATUS jsr120_unregister_sms_push_listener(jchar port);

/**
 * Unblocks the thread that matches the specified handle
 * and signal
 *
 * @param handle handle to open SMS connection
 * @param waitingFor signal that thread is waiting for
 *
 * @result returns <code>WMA_OK</code> if a matching thread is unblocked,
 *                 <code>WMA_ERR</code> otherwise.
 */
WMA_STATUS jsr120_sms_unblock_thread(jint handle, jint waitingFor);

/**
 * Delete all SMS messages cached in the pool for the specified
 * midlet suite
 *
 * @param msid Midlet Suite ID.
 *
 */
void jsr120_sms_delete_midlet_suite_msg(AppIdType msid);

/**
 * Delete all SMS messages cached in the pool for the specified
 * midlet suite, by the Push subsystem.
 *
 * @param msid Midlet Suite ID.
 *
 */
void jsr120_sms_delete_push_msg(AppIdType msid);

#ifdef __cplusplus
}
#endif

#endif /* #ifdef _JSR120_SMS_LISTENERS_H_ */
