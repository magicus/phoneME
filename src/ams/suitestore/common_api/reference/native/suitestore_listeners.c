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
 * @ingroup AMS
 *
 * This is reference implementation of the Suite Storage Listeners API.
 * It allows to register/unregister callbacks that will be notified
 * when the certain operation on a midlet suite is performed.
 */

#include <pcsl_memory.h>
#include <suitestore_listeners.h>

/**
 * A structure describing the listener.
 */
typedef struct _suiteStoreListener {
    GenericListener genericListener;
    int whenToCall;
} SuiteStoreListener;

/**
 * Notifies the registered listeners about the given event.
 *
 * This function is intended to be called by the Installer and the
 * Task Manager subsystems.
 *
 * @param listenerType type of listeners that should be notified:
 * <pre>
 *     SUITESTORE_LISTENER_TYPE_INSTALL - the listener will be called before
 *         and after the installation of a midlet suite;
 *     SUITESTORE_LISTENER_TYPE_REMOVE - the listener will be called before
 *         and after removing of a midlet suite.
 * </pre>
 * @param when defines at what stage of the operation this function being called
 * @param status status of the operation. Has a meaning if 'when' parameter
 * has SUITESTORE_OPERATION_END value.
 * @param pSuiteData information about the midlet suite on which the operation
 * is performed
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
suite_listeners_notify(int listenerType, int when, MIDPError status,
                       const MidletSuiteData* pSuiteData) {
    SuiteStoreListener* pListener = (SuiteStoreListener*)
        get_event_listeners_impl((int)listenerType);

    while (pListener) {
        if (pListener->genericListener.listenerType == listenerType &&
                pListener->whenToCall == when) {
            ((SUITESTORE_LISTENER)pListener->genericListener.fn_callback)(
                listenerType, when, status, pSuiteData);
        }

        pListener = (SuiteStoreListener*)pListener->genericListener.pNext;
    }

    return ALL_OK;
}

/**
 * Adds a listener for the given operation (installation or removal)
 * on a midlet suite.
 *
 * @param fn_callback pointer to the listener function
 * @param listenerType type of events this listener wants to listen for:
 * <pre>
 *     SUITESTORE_LISTENER_TYPE_INSTALL - the listener will be called on
 *         the installation of a midlet suite;
 *     SUITESTORE_LISTENER_TYPE_REMOVE - the listener will be called on
 *         the removal of a midlet suite.
 * </pre>
 * @param whenToCall defines when the listener should be called:
 * <pre>
 *     SUITESTORE_OPERATION_START - before the operation on the suite;
 *     SUITESTORE_OPERATION_END - after the operation;
 *     SUITESTORE_WHEN_ALWAYS - before and after the operation.
 * </pre>
 *
 * @return error code: ALL_OK if successful,
 *                     OUT_OF_MEMORY if not enough memory,
 *                     BAD_PARAMS if fn_callback is NULL
 */
MIDPError
midp_suite_add_listener(SUITESTORE_LISTENER fn_callback, int listenerType,
                        int whenToCall) {
    SuiteStoreListener newListener;

    newListener.genericListener.dataSize = sizeof(SuiteStoreListener);
    newListener.genericListener.fn_callback  = (void*)fn_callback;
    newListener.genericListener.listenerType = (int)listenerType;
    newListener.whenToCall = whenToCall;

    return add_event_listener_impl((GenericListener*)&newListener);
}

/**
 * Unregisters the given listener of the specified type.
 *
 * @param fn_callback pointer to the listener function
 * @param listenerType type of events this listener listens for:
 * <pre>
 *     SUITESTORE_LISTENER_TYPE_INSTALL - the listener will be called on
 *         the installation of a midlet suite;
 *     SUITESTORE_LISTENER_TYPE_REMOVE - the listener will be called on
 *         the removal of a midlet suite.
 * </pre>
 *
 * @return error code: ALL_OK if successful,
 *                     NOT_FOUND if the listener was not found
 */
MIDPError
midp_suite_remove_listener(SUITESTORE_LISTENER fn_callback, int listenerType) {
    return remove_event_listener_impl((void*)fn_callback, listenerType);
}

/**
 * Unregisters all registered listeners.
 *
 * This function is intended to be called internally during the system cleanup.
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
suite_remove_all_listeners() {
    return remove_all_event_listeners_impl(LISTENER_TYPE_ANY);
}
