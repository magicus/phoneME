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

#ifndef _RMS_REGISTRY_H_
#define _RMS_REGISTRY_H_

/**
 * @defgroup rms Record Management System - Single Interface (Both Porting and External)
 * @ingroup subsystems
 */

/**
 * @file
 * @ingroup rms
 * @brief Interface for registering inter-task listeners of record store changes.
 * Changes done to the same record store in different execution contexts must be
 * notified to all record listeners registered to listen for changes of the record
 * store in any execution context. This API can be used to start/stop listening
 * for 
 * ##include &lt;&gt;
 * @{
 */

#include <pcsl_string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Starts listening of asynchronous changes of record store
 * @param suiteId suite ID of record store to start listen for
 * @param storeName name of record store to start listen for
 */
void rms_registry_start_record_store_listening(
    int suiteId, pcsl_string *storeName);

/**
 * Stops listening of asynchronous changes of record store
 * @param suiteId suite ID of record store to stop listen for
 * @param storeName name of record store to stop listen for
 */
void rms_registry_stop_record_store_listening(
    int suiteId, pcsl_string *storeName);

/**
 * Sends asynchronous notification about change of record store done
 * in the current execution context of method caller
 *
 * @param suiteId suite ID of changed record store
 * @param storeName name of changed record store
 * @param changeType type of record change: ADDED, DELETED or CHANGED
 * @param recordId ID of changed record
 */
void rms_registry_send_record_store_change_event(
    int suiteId, pcsl_string *storeName,
    int changeType, int recordId);

/**
 * Gets list of pairs <task ID, counter> for each VM task listening for changes
 * of given record store. The pair consists of ID of VM task, and of number of
 * record store notifications been sent to this VM task and not acknowledged by
 * it yet. The number of not acknowledged record store notifications must be
 * regarded by notification sender to not overflow event queue of the reciever
 * VM task.
 *
 * @param suiteId suite ID of record store
 * @param storeName name of record store
 * @param listeners [OUT] list of <task ID, notification counter> pairs
 * @param length [OUT] length of the listeners array,
 *    equals to number of pairs * 2
 */
void rms_registry_get_record_store_listeners(
    int suiteId, pcsl_string *storeName, int *listeners,int *length);

/**
 * Resets record store notification counter for given VM task.
 * The notification counter is used to request notifications reciever to
 * acknowledge delivery of a series of notifications. Resetting of the
 * counter can be done either after the acknowledgment, or to not wait
 * for an acknowledgment in abnormal situations.
 *
 * @param taskId ID of the VM task ID
 */
void rms_registry_reset_record_store_notification_counter(int taskId);

/**
 * Acknowledges delivery of record store notifications sent to VM task earlier.
 * The acknowledgment is required by notifications sender for each series of
 * notification events to be sure a reciever can process the notifications and
 * its queue won't be overflowen.
 *
 * @param taskId ID of VM task
 */
void rms_registry_acknowledge_record_store_notifications(int taskId);

/**
 * Stops listening for any record store changes in VM task
 * @param taskId ID of VM task  
 */
void rms_regisrty_stop_task_listeners(int taskId);


#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _RMS_REGISTRY_H_ */
