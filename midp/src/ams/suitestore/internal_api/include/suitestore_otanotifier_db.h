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

/**
 * @file
 *
 * This header file is interface to the notification manager
 * functions.
 */

#ifndef _SUITESTORE_OTANOTIFIER_DB_H_
#define _SUITESTORE_OTANOTIFIER_DB_H_

#include <midpString.h>

/** Pending install or delete notification */
typedef struct _MidpPendingNotification {
    /** Number of times the record has been retried. */
    jint retries;

    /** Suite this notification belongs to. */
    SuiteIdType suiteId;

    /** URL to post the notification to. */
    pcsl_string url;
} MidpPendingNotification;

/**
 * Frees a list of pending notifications.
 *
 * @param pList point to an array of notifications
 * @param numberOfElements number of elements in pList
 */
void midpFreeNotificationList(MidpPendingNotification* pList,
                              int numberOfElements);

/**
 * Queues provider's URL to send a delete notification message
 * during the next OTA session.
 *
 * @param suiteID suite this notification belongs to
 * @param url target url for the status message
 */
void midpAddDeleteNotification(SuiteIdType suiteId,
                               const pcsl_string* url);

/**
 * Removes the URL from the delete notification list.
 *
 * @param suiteID suite ID of the notification
 */
void midpRemoveDeleteNotification(SuiteIdType suiteId);

/**
 * Retrieves the number of URLs queued delete in the notification list.
 *
 * @return the number of delete notifications
 */
int midpGetNumberOfDeleteNotifications();

/**
 * Retrieves the queued delete notification list from storage.
 *
 * @param paList pointer to a pointer of notifications, free with
 * freeNotifications
 *
 * @return number of notifications in paList
 */
int midpGetDeleteNotifications(MidpPendingNotification** paList);

/**
 * Save the given list of notifications to persistent storage.
 * If any of the notifications in the list have a -1 for retries,
 * that notification will not be saved.
 *
 * @param pList array as the one returned by getDeleteNotifications()
 * @param numberOfElements number of elements in pList
 */
void midpSaveDeleteNotifications(MidpPendingNotification* pList,
                                 int numberOfElements);

/**
 * Queues provider's URL to send a install notification message
 * during the next OTA session.
 *
 * @param suiteId suite this notification belongs to
 * @param url target url for the status message
 */
void midpAddInstallNotification(SuiteIdType suiteId, const pcsl_string* url);

/**
 * Removes the URL from the install notification list.
 *
 * @param suiteId suite ID of the notification
 */
void midpRemoveInstallNotification(SuiteIdType suiteId);

/**
 * Retrieves the number of URLs queued install in the notification list.
 *
 * @return the number of install notifications
 */
int midpGetNumberOfInstallNotifications();

/**
 * Retrieves the queued install notification list from storage.
 *
 * @param paList pointer to a pointer of notifications, free with
 * freeNotifications
 *
 * @return number of notifications in paList
 */
int midpGetInstallNotifications(MidpPendingNotification** paList);

/**
 * Save the given list of notifications to persistent storage.
 * If any of the notifications in the list have a -1 for retries,
 * that notification will not be saved.
 *
 * @param pList array as the one returned by getInstallNotifications()
 * @param numberOfElements number of elements in pList
 */
void midpSaveInstallNotifications(MidpPendingNotification* pList,
                                  int numberOfElements);

#endif /* _SUITESTORE_OTANOTIFIER_DB_H_ */
