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
#include <midpUtilKni.h>

#include <midpMalloc.h>
#include <midpStorage.h>
#include <suitestore_common.h>
#include <suitestore_intern.h>
#include <suitestore_otanotifier_db.h>

/**
 * Filename to save the delete notification URLs.
 * "_delete_notify.dat"
 */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(DELETE_NOTIFY_FILENAME)
    {'_', 'd', 'e', 'l', 'e', 't', 'e', '_',
    'n', 'o', 't', 'i', 'f', 'y', '.', 'd', 'a', 't', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(DELETE_NOTIFY_FILENAME);

/**
 * Filename to save the install notification URLs.
 * "_install_notify.dat"
 */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(INSTALL_NOTIFY_FILENAME)
    {'_', 'i', 'n', 's', 't', 'a', 'l', 'l', '_',
    'n', 'o', 't', 'i', 'f', 'y', '.', 'd', 'a', 't', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(INSTALL_NOTIFY_FILENAME);

/**
 * Intialize a notification's data.
 *
 * @param notification notification data to initialize
 */
static void initNotification(MidpPendingNotification* pNotification) {
    pNotification->retries = 0;
    pNotification->suiteId = UNUSED_SUITE_ID;
    pNotification->url = PCSL_STRING_NULL;
}

/**
 * Write a pending notification.
 *
 * @param ppszError where to put errors
 * @param handle to an open notification file
 * @param notification notification to add
 */
static void writeNotification(char** ppszError, int handle,
                       const MidpPendingNotification* notification) {
    storageWrite(ppszError, handle, (char*)&(notification->retries),
                 sizeof (notification->retries));
    if (*ppszError != NULL) {
        return;
    }

    // storage_write_utf16_string(ppszError, handle, &(notification->suiteId));
    storageWrite(ppszError, handle, (char*)&(notification->suiteId),
        sizeof (SuiteIdType));
    if (*ppszError != NULL) {
        return;
    }

    storage_write_utf16_string(ppszError, handle, &(notification->url));
}
/**
 * Save the given notification list and new notification to disk.
 * If any of the retries has been set to -1 then that notification will
 * not be saved.
 * The file has the number of elements at the front, each element
 * is the retries, a length of the URL and the URL jchars.
 *
 * @param filename full path the the notification file
 * @param pList array as one returned by getDeleteNotifications()
 * @param numberOfElements number of elements in pStrings
 * @param newElement new element to add, if length is retry is -1 the it will
 *        not be added
 */
static void saveNotifications(const pcsl_string* filename_str,
        MidpPendingNotification* pList, int numberOfElements,
        MidpPendingNotification newElement) {
    char* pszError;
    int numberOfElementsLeft;
    int i;
    int handle;

    handle = storage_open(&pszError, filename_str, OPEN_READ_WRITE_TRUNCATE);
    if (pszError != NULL) {
        return;
    }

    numberOfElementsLeft = numberOfElements;
    for (i = 0; i < numberOfElements; i++) {
        if (pList[i].retries < 0) {
            numberOfElementsLeft--;
        }
    }

    if (newElement.retries >= 0) {
        numberOfElementsLeft++;
    }

    storageWrite(&pszError, handle, (char*)&numberOfElementsLeft,
        sizeof (numberOfElementsLeft));

    do {
        if (pszError != NULL || numberOfElementsLeft == 0) {
            break;
        }

        for (i = 0; i < numberOfElements; i++) {
            if (pList[i].retries < 0) {
                continue;
            }

            writeNotification(&pszError, handle, &(pList[i]));
            if (pszError != NULL) {
                break;
            }
        }

        if (i != numberOfElements) {
            /* Loop did not successfully finish */
            break;
        }

        if (newElement.retries >= 0) {
            writeNotification(&pszError, handle, &newElement);
        }
    } while (0);

    storageFreeError(pszError);
    storageClose(&pszError, handle);
    storageFreeError(pszError);
}

/**
 * Retrieves the number of elements in the notification list.
 *
 * @param filename full path the the notification file
 *
 * @return the number of delete notifications
 */
static int getNumberOfNotifications(const pcsl_string* filename) {
    char* pszError;
    int handle;
    int numberOfElements = 0;

    handle = storage_open(&pszError, filename, OPEN_READ);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return 0;
    }

    storageRead(&pszError, handle, (char*)&numberOfElements,
        sizeof (numberOfElements));
    if (pszError != NULL) {
        storageFreeError(pszError);
        return 0;
    }

    storageClose(&pszError, handle);
    storageFreeError(pszError);

    return numberOfElements;
}

/**
 * Retrieves a queued notification list from storage.
 *
 * @param filename full path the the notification file
 * @param paList pointer to a pointer of notifications, free with
 * freeNotifications
 *
 * @return number of notifications in paList,
 *         less than zero in the case of an error
 */
static int getNotifications(const pcsl_string* filename_str,
        MidpPendingNotification** paList) {
    char* pszError;
    int numberOfElements = 0;
    MidpPendingNotification* pList = NULL;
    int i = 0;
    int handle;
    int no_errors = 0;
    int out_of_mem = 0;

    *paList = NULL;

    handle = storage_open(&pszError, filename_str, OPEN_READ);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return 0;
    }

    storageRead(&pszError, handle, (char*)&numberOfElements,
        sizeof (numberOfElements));

    do {
        if (pszError != NULL) {
            break;
        }

        if (numberOfElements == 0) {
            no_errors = 1;
            break;
        }

        pList = (MidpPendingNotification*)midpMalloc(numberOfElements *
                  sizeof (MidpPendingNotification));

        if (pList == NULL) {
            out_of_mem = 1;
            break;
        }

        for (i = 0; i < numberOfElements; i++) {
            initNotification(&pList[i]);
        }

        for (i = 0; i < numberOfElements; i++) {
            storageRead(&pszError, handle, (char*)&pList[i].retries,
                        sizeof (pList[i].retries));
            if (pszError != NULL) {
                break;
            }

            // storage_read_utf16_string(&pszError, handle, &pList[i].suiteId);

            storageRead(&pszError, handle, (char*)&(pList[i].suiteId),
                sizeof (SuiteIdType));
            if (NULL != pszError) {
                break;
            }
            storage_read_utf16_string(&pszError, handle, &pList[i].url);
            if (NULL != pszError) {
                break;
            }
       }

        if (i != numberOfElements) {
            break;
        }

        no_errors = 1;
    } while (0);

    if (NULL != pszError) {
        storageFreeError(pszError);
    }
    storageClose(&pszError, handle);
    storageFreeError(pszError);

    if (no_errors) {
        *paList = pList;
        return numberOfElements;
    }

    midpFreeNotificationList(pList, i);

    if (out_of_mem) {
        return OUT_OF_MEM_LEN;
    }
    /* 0 somewhy we return in the case of i/o errors */
    return IO_ERROR_LEN;
}

/**
 * Queues provider's URL to send a delete notification message
 * during the next OTA session.
 *
 * @param filename full path the the notification file
 * @param suiteId suite the notification belongs to
 * @param url target url for the status message
 */
static void addNotification(const pcsl_string* filename_str,
                            SuiteIdType suiteId,
                            const pcsl_string* url_str) {
    MidpPendingNotification* pList;
    int numberOfElements;
    MidpPendingNotification temp;

    numberOfElements = getNotifications(filename_str, &pList);
    if (numberOfElements < 0) {
        return;
    }

    temp.retries = 0;
    temp.suiteId = suiteId;
    temp.url = *url_str;
    saveNotifications(filename_str, pList, numberOfElements, temp);
    midpFreeNotificationList(pList, numberOfElements);
}

/**
 * Removes a pending notification.
 *
 * @param filename full path the the notification file
 * @param suiteId suite ID of the notification to remove
 */
static void removeNotification(const pcsl_string* filename_str,
                               SuiteIdType suiteId) {
    MidpPendingNotification* pList;
    int numberOfElements;
    int i;
    MidpPendingNotification temp;

    numberOfElements = getNotifications(filename_str, &pList);
    if (numberOfElements <= 0) {
        return;
    }

    for (i = 0; i < numberOfElements; i++) {
        if (pList[i].suiteId == suiteId) {
            pList[i].retries = -1;
            temp.retries = -1;
            saveNotifications(filename_str, pList, numberOfElements, temp);
            break;
        }
    }

    midpFreeNotificationList(pList, numberOfElements);
}

/**
 * Frees a notification's data. Will not free the notification itself.
 *
 * @param notification notification data to free
 */
static void freeNotificationData(MidpPendingNotification* pNotification) {
    pcsl_string_free(&pNotification->url);
    initNotification(pNotification);
}

/**
 * Frees a list of pending notifications.
 *
 * @param pList point to an array of notifications
 * @param numberOfElements number of elements in pList
 */
void midpFreeNotificationList(MidpPendingNotification* pList,
                              int numberOfElements) {
    int i;

    for (i = 0; i < numberOfElements; i++) {
        freeNotificationData(&pList[i]);
    }

    midpFree(pList);
}

/**
 * Queues provider's URL to send a delete notification message
 * during the next OTA session.
 *
 * @param suiteId suite the notification belongs to
 * @param url target url for the status message
 */
void midpAddDeleteNotification(SuiteIdType suiteId,
                               const pcsl_string* url_str) {
    pcsl_string filename_str;
    pcsl_string_status rc;
    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &DELETE_NOTIFY_FILENAME, &filename_str);
    if (PCSL_STRING_OK != rc) {
        return;
    }

    addNotification(&filename_str, suiteId, url_str);
    pcsl_string_free(&filename_str);
}

/**
 * Removes a pending delete notification.
 *
 * @param suiteId suite ID of the notification to remove
 */
void midpRemoveDeleteNotification(SuiteIdType suiteId) {
    pcsl_string filename;
    pcsl_string_status rc;

    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &DELETE_NOTIFY_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return;
    }

    removeNotification(&filename, suiteId);
    pcsl_string_free(&filename);
}

/**
 * Retrieves the number of URLs queued delete in the notification list.
 *
 * @return the number of delete notifications
 */
int midpGetNumberOfDeleteNotifications() {
    pcsl_string filename;
    pcsl_string_status rc;
    int numberOfElements = 0;

    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &DELETE_NOTIFY_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return 0;
    }

    numberOfElements = getNumberOfNotifications(&filename);
    midpFree(filename.data);

    return numberOfElements;
}

/**
 * Retrieves the queued delete notification list from storage.
 *
 * @param paList pointer to a pointer of notifications, free with
 * freeNotifications
 *
 * @return number of notifications in paList,
 *         negative in case of error
 */
int midpGetDeleteNotifications(MidpPendingNotification** paList) {
    pcsl_string filename;
    pcsl_string_status rc;
    int numberOfElements = 0;

    *paList = NULL;

    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &DELETE_NOTIFY_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return 0;
    }

    numberOfElements = getNotifications(&filename, paList);
    pcsl_string_free(&filename);

    return numberOfElements;
}

/**
 * Save the given list of delete notifications to persistent storage.
 *
 * @param pList array as the one returned by getDeleteNotifications()
 * @param numberOfElements number of elements in pList
 */
void midpSaveDeleteNotifications(MidpPendingNotification* pList,
                                 int numberOfElements) {
    pcsl_string filename;
    pcsl_string_status rc;
    MidpPendingNotification temp;

    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &DELETE_NOTIFY_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return;
    }

    temp.retries = -1;
    saveNotifications(&filename, pList, numberOfElements, temp);
    pcsl_string_free(&filename);
}

/**
 * Queues provider's URL to send a install notification message
 * during the next OTA session.
 *
 * @param suiteId suite the notification belongs to
 * @param url target url for the status message
 */
void midpAddInstallNotification(SuiteIdType suiteId, const pcsl_string* url) {
    pcsl_string filename;
    pcsl_string_status rc;

    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &INSTALL_NOTIFY_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return;
    }

    addNotification(&filename, suiteId, url);
    pcsl_string_free(&filename);
}

/**
 * Removes a pending install notification.
 *
 * @param suiteId suite ID of the notification to remove
 */
void midpRemoveInstallNotification(SuiteIdType suiteId) {
    pcsl_string filename;
    pcsl_string_status rc;

    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &INSTALL_NOTIFY_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return;
    }

    removeNotification(&filename, suiteId);
    pcsl_string_free(&filename);
}

/**
 * Retrieves the number of URLs queued install in the notification list.
 *
 * @return the number of install notifications
 */
int midpGetNumberOfInstallNotifications() {
    pcsl_string filename;
    pcsl_string_status rc;
    int numberOfElements = 0;

    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &INSTALL_NOTIFY_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return 0;
    }

    numberOfElements = getNumberOfNotifications(&filename);
    pcsl_string_free(&filename);

    return numberOfElements;
}

/**
 * Retrieves the queued install notification list from storage.
 *
 * @param paList pointer to a pointer of notifications, free with
 * freeNotifications
 *
 * @return number of notifications in paList,
 *         negative in case of error
 */
int midpGetInstallNotifications(MidpPendingNotification** paList) {
    pcsl_string filename;
    pcsl_string_status rc;
    int numberOfElements = 0;

    *paList = NULL;

    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &INSTALL_NOTIFY_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return 0;
    }

    numberOfElements = getNotifications(&filename, paList);
    pcsl_string_free(&filename);

    return numberOfElements;
}

/**
 * Save the given list of install notifications to persistent storage.
 *
 * @param pList array as the one returned by getInstallNotifications()
 * @param numberOfElements number of elements in pList
 */
void midpSaveInstallNotifications(MidpPendingNotification* pList,
                                  int numberOfElements) {
    pcsl_string filename;
    pcsl_string_status rc;
    MidpPendingNotification temp;

    rc = pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID),
                         &INSTALL_NOTIFY_FILENAME, &filename);
    if (PCSL_STRING_OK != rc) {
        return;
    }

    temp.retries = -1;
    saveNotifications(&filename, pList, numberOfElements, temp);
    pcsl_string_free(&filename);
}
