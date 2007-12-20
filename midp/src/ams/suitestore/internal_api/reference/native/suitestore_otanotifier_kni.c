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

#include <midpError.h>
#include <midpMalloc.h>
#include <midpStorage.h>
#include <suitestore_intern.h>
#include <suitestore_common.h>
#include <suitestore_kni_util.h>
#include <suitestore_otanotifier_db.h>
#include <midpUtilKni.h>

/*
 * Fill a pending notification object.
 *
 * @param destObj handle of the Java object to fill
 * @param source pending notification record
 */
static void fillPendingNotificationObject(KNIDECLARGS jobject destObj,
    MidpPendingNotification source) {
    KNI_StartHandles(2);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(temp);

    KNI_GetObjectClass(destObj, clazz);

    KNI_RESTORE_INT_FIELD(destObj, clazz, "retries",
                          source.retries);
    KNI_RESTORE_INT_FIELD(destObj, clazz, "suiteId",
                          source.suiteId);
    KNI_RESTORE_PCSL_STRING_FIELD(destObj, clazz, "url",
                             &source.url, temp);

    KNI_EndHandles();
}

/**
 * Native method void removeDeleteNotificatin(String) for class
 * com.sun.midp.installer.OtaNotifier.
 * <p>
 * Removes a pending delete notification.
 *
 * @param suiteId suite the notification belongs to
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_installer_OtaNotifier_removeDeleteNotification) {
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);
    midpRemoveDeleteNotification(suiteId);
    KNI_ReturnVoid();
}

/**
 * Native method int getNumberOfDeleteNotifications() for class
 * com.sun.midp.installer.OtaNotifier.
 * <p>
 * Retrieves the number of URLs queued delete in the notification list.
 *
 * @return the number of URLs in the delete notification list
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_installer_OtaNotifier_getNumberOfDeleteNotifications) {
    KNI_ReturnInt(midpGetNumberOfDeleteNotifications());
}

/**
 * Native method void fillDeleteNotificationListForRetry() for class
 * com.sun.midp.installer.OtaNotifier.
 * <p>
 * Retrieves the queued delete notification list from storage and increments
 * the retry count of every member of the list.
 *
 * @param urls empty delete notification list to fill
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_installer_OtaNotifier_fillDeleteNotificationListForRetry) {
    int numberOfObjs;
    int numberOfUrls = 0;
    MidpPendingNotification* pUrls = NULL;
    int i;

    KNI_StartHandles(2);
    KNI_DeclareHandle(list);
    KNI_DeclareHandle(tempObj);

    KNI_GetParameterAsObject(1, list);

    numberOfObjs = (int)KNI_GetArrayLength(list);

    do {
        if (numberOfObjs <= 0) {
            break;
        }

        numberOfUrls = midpGetDeleteNotifications(&pUrls);

        if (numberOfUrls <= 0) {
            numberOfUrls = 0;
            numberOfObjs = 0;
            break;
        }

        if (numberOfObjs > numberOfUrls) {
            numberOfObjs = numberOfUrls;
        }

        for (i = 0; i < numberOfObjs; i++) {
            pUrls[i].retries++;
            KNI_GetObjectArrayElement(list, i, tempObj);
            fillPendingNotificationObject(KNIPASSARGS tempObj, pUrls[i]);
        }
    } while (0);

    /*
     * Write out the list (upto the number of objects filled)
     * to keep track of retries now so we don't
     * have to lock the list while retrying.
     */
    midpSaveDeleteNotifications(pUrls, numberOfObjs);

    midpFreeNotificationList(pUrls, numberOfUrls);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Native method void addInstallNotification(String) for class
 * com.sun.midp.installer.OtaNotifier.
 * <p>
 * Adds an install notification.
 *
 * @param suiteId suite the notification belongs to
 * @param url url to send the notification to
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_installer_OtaNotifier_addInstallNotification) {
    SuiteIdType suiteId;

    KNI_StartHandles(1);
    suiteId = KNI_GetParameterAsInt(1);

    GET_PARAMETER_AS_PCSL_STRING(2, url_str) {
        midpAddInstallNotification(suiteId, &url_str);
    } RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * Native method void removeInstallNotification(String) for class
 * com.sun.midp.installer.OtaNotifier.
 * <p>
 * Removes a pending install notification.
 *
 * @param suiteId suite the notification belongs to
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_installer_OtaNotifier_removeInstallNotification) {
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);
    midpRemoveInstallNotification(suiteId);
    KNI_ReturnVoid();
}

/**
 * Native method void getInstallNotificationForRetry() for class
 * com.sun.midp.installer.OtaNotifier.
 * <p>
 * Retrieves the queued install notification list from storage and increments
 * the retry count of the notification.
 *
 * @param suiteId suite ID of the notification to get
 * @paran dest where to put the notification
 *
 * @return true if the notification is found
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_installer_OtaNotifier_getInstallNotificationForRetry) {
    int numberOfUrls = 0;
    MidpPendingNotification* pUrls = NULL;
    int i;
    SuiteIdType suiteId;
    jboolean found = KNI_FALSE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(destObj);
    suiteId = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, destObj);

    do {
        numberOfUrls = midpGetInstallNotifications(&pUrls);

        if (numberOfUrls <= 0) {
            break;
        }

        for (i = 0; i < numberOfUrls; i++) {
            if (suiteId == pUrls[i].suiteId) {
                pUrls[i].retries++;

                fillPendingNotificationObject(KNIPASSARGS destObj, pUrls[i]);

                /*
                 * Write out the list to keep track of retries now so we
                 * don't have to lock the list while retrying.
                 */
                midpSaveInstallNotifications(pUrls, numberOfUrls);

                found = KNI_TRUE;
                break;
            }
        }

        midpFreeNotificationList(pUrls, numberOfUrls);
    } while (0);

    KNI_EndHandles();
    KNI_ReturnBoolean(found);
}
