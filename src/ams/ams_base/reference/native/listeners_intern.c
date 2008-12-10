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
 * This is reference implementation of the Generic Listeners API.
 * It contains basic functions to work with linked lists.
 */

#include <string.h>              /* for memcpy() */
#include <pcsl_memory.h>
#include <listeners_intern.h>

/** Maximum number of different listener types */
#define MAX_LISTENER_TYPES 10

/**
 * A list to track the information about the registered listeners.
 */
static GenericListener* g_pRegisteredListeners[MAX_LISTENER_TYPES];

/**
 * Array with
 * (listener type; index of the listeners list in g_pRegisteredListeners[])
 * pairs.
 */
static int g_listenerIdxByType[MAX_LISTENER_TYPES * 2];

/** Indicates if the Listeners Subsystem is already initialized. */
static int g_listenersInitDone = 0;

/**
 * Finds the given type of listener in the g_listenerIdxByType[] array
 * of pairs and returns an index of the corresponding listeners list
 * in the g_pRegisteredListeners[] array.
 *
 * @param listenerType type of listener to look for
 *
 * @return index of the list containing listeners of the given type
 *         or -1 if it is not found
 */
static int
get_listeners_list_index(int listenerType) {
    int i;

    for (i = 0; i < MAX_LISTENER_TYPES; i++) {
        int n = (i << 1);
        if (g_listenerIdxByType[n] == listenerType) {
            return g_listenerIdxByType[n + 1];
        }
    }

    return -1;
}

/**
 * Adds a new type of listener to the internal array containing
 * the associations between the listener types and the index of the
 * list with pointers to the callback functions in another array.
 * If such type of listeners is already registered, returns an index
 * of the corresponding list like get_listeners_list_index().
 *
 * @param listenerType a new type of listener to add
 *
 * @return index of the list containing listeners of the given type
 *         or -1 if it doesn't exist and can't be added (the list is full)
 */
static int
add_listener_type(int listenerType) {
    int i, firstFreeIdx = -1;

    for (i = 0; i < MAX_LISTENER_TYPES; i++) {
        int n = (i << 1);
        if (g_listenerIdxByType[n] == listenerType) {
            /* list of listeners of this type already exists, do nothing */
            return g_listenerIdxByType[n + 1];
        } else if (firstFreeIdx == -1 &&
                   g_listenerIdxByType[n] == LISTENER_TYPE_INVALID) {
            firstFreeIdx = n;
        }
    }

    if (firstFreeIdx != -1) {
        /*
         * There are no listeners of this type registered yet, trying
         * to find the first unused entry in g_pRegisteredListeners.
         */
        int freeIdxForList = -1;
        for (i = 0; i < MAX_LISTENER_TYPES; i++) {
            if (!g_pRegisteredListeners[i]) {
                freeIdxForList = i;
                break;
            }
        }

        if (freeIdxForList != -1) {
            /* add a listenerType => index of the listeners list association */
            g_listenerIdxByType[firstFreeIdx++] = listenerType;
            g_listenerIdxByType[firstFreeIdx]   = freeIdxForList;
            return freeIdxForList;
        }
    }

    return -1;
}


/**
 * Removes an association of the given listener's type with an index of the
 * corresponding listeners list in the g_pRegisteredListeners[] array.
 *
 * @param listenerType type of listener for which the association
 *                     must be removed
 */
static void
remove_listener_type(int listenerType) {
    int i;

    for (i = 0; i < MAX_LISTENER_TYPES; i++) {
        int n = (i << 1);
        if (g_listenerIdxByType[n] == listenerType) {
            g_listenerIdxByType[n++] = LISTENER_TYPE_INVALID;
            g_listenerIdxByType[n]   = LISTENER_TYPE_INVALID;
            break;
        }
    }
}

/**
 * Initializes the Listeners subsystem.
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
init_listeners_impl() {
    if (g_listenersInitDone) {
        /* Already initialized */
        return ALL_OK;
    }

    g_listenersInitDone = 1;

    memset((char*)g_pRegisteredListeners, 0, sizeof(g_pRegisteredListeners));
    memset((char*)g_listenerIdxByType, (char)LISTENER_TYPE_INVALID,
           sizeof(g_listenerIdxByType));

    return ALL_OK;
}


/**
 * Returns a list of listeners of the given type.
 *
 * @param listenerType type of listeners which should be returned
 *
 * @return linked list of the event listeners of the given type
 *         or NULL if no listeners of this type are registered
 */
const GenericListener*
get_event_listeners_impl(int listenerType) {
    const GenericListener* pListener = NULL;

    if (listenerType >= 0 && listenerType < MAX_LISTENER_TYPES) {
        int index = get_listeners_list_index(listenerType);
        if (index >= 0) {
            pListener = g_pRegisteredListeners[index];
        }
    }

    return pListener;
}

/**
 * Adds a generic listener to the linked list of registered listeners.
 *
 * @param pListenerData a structure with listener's parameters.
 * The first member of this structure must be of GenericListener type.
 *
 * @return error code: ALL_OK if successful,
 *                     OUT_OF_MEMORY if not enough memory,
 *                     BAD_PARAMS if pListenerData is NULL
 *                                or pListenerData->dataSize is invalid
 */
MIDPError
add_event_listener_impl(const GenericListener* pListenerData) {
    GenericListener *pNewListener;
    int index;

    /* validate the parameters */
    if (pListenerData == NULL) {
        return BAD_PARAMS;
    }

    if (pListenerData->dataSize < (int)sizeof(GenericListener)) {
        return BAD_PARAMS;
    }

    index = add_listener_type(pListenerData->listenerType);
    if (index < 0) {
        /*
         * Too many types of listeners are registered,
         * not enough space for this one.
         */
        return OUT_OF_MEMORY;
    }

    /* allocate memory for a new structure describing the listener */
    pNewListener = (GenericListener*)pcsl_mem_malloc(pListenerData->dataSize);
    if (pNewListener == NULL) {
        return OUT_OF_MEMORY;
    }

    memcpy((char*)pNewListener, (const char*)pListenerData,
           pListenerData->dataSize);

    pNewListener->pNext = g_pRegisteredListeners[index];
    g_pRegisteredListeners[index] = pNewListener;

    return ALL_OK;
}

/**
 * Unregisters the given listener of the specified type.
 *
 * @param fn_callback pointer to the listener function
 * @param listenerType type of events this listener listens for
 *
 * @return error code: ALL_OK if successful,
 *                     NOT_FOUND if the listener was not found
 */
MIDPError
remove_event_listener_impl(void* fn_callback, int listenerType) {
    GenericListener *pListener, *pPrev = NULL;
    int index;

    index = get_listeners_list_index(listenerType);
    if (index < 0) {
        return NOT_FOUND;
    }

    pListener = g_pRegisteredListeners[index];

    while (pListener) {
        if (pListener->fn_callback == fn_callback) {
            if (pPrev) {
                pPrev->pNext = pListener->pNext;
            } else {
                g_pRegisteredListeners[index] = pListener->pNext;
                if (g_pRegisteredListeners[index] == NULL) {
                    remove_listener_type(listenerType);
                }
            }

            pcsl_mem_free(pListener);

            return ALL_OK;
        }

        pPrev = pListener;
        pListener = pListener->pNext;
    }

    return NOT_FOUND;
}

/**
 * Unregisters all registered listeners of the given type.
 *
 * @param listenerType type of the listeners that must be unregistered
 * or LISTENER_TYPE_ANY to unregister all listeners
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
remove_all_event_listeners_impl(int listenerType) {
    GenericListener *pListener, *pTmpListener;
    int index = -1;

    do {
        if (listenerType == LISTENER_TYPE_ANY) {
            /* walk through all indices from 0 to MAX_LISTENER_TYPES */
            index++;
        } else {
            index = get_listeners_list_index(listenerType);
        }

        if (index < 0 || index == MAX_LISTENER_TYPES) {
            /* there are no listeners of the given type */
            break;
        }

        pListener = g_pRegisteredListeners[index];

        while (pListener) {
            pTmpListener = pListener->pNext;

            g_pRegisteredListeners[index] = pTmpListener;

            pcsl_mem_free(pListener);
            pListener = pTmpListener;
        }
    } while (1);

    return ALL_OK;
}
