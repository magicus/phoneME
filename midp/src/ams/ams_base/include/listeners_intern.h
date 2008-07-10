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

#ifndef _LISTENERS_INTERN_H_
#define _LISTENERS_INTERN_H_

#include <midp_global_status.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LISTENER_TYPE_INVALID  (0)
#define LISTENER_TYPE_ANY     (-1)

/**
 * A structure describing a generic listener.
 */
typedef struct _genericListener {
    int dataSize;
    int listenerType;
    void* fn_callback;
    struct _genericListener* pNext;
} GenericListener;

/**
 * Initializes the Listeners subsystem.
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
init_listeners_impl();

/**
 * Returns a list of listeners of the given type.
 *
 * @param listenerType type of listeners which should be returned
 *
 * @return linked list of the event listeners of the given type
 *         or NULL if no listeners of this type are registered
 */
const GenericListener*
get_event_listeners_impl(int listenerType);

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
add_event_listener_impl(const GenericListener* pListenerData);

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
remove_event_listener_impl(void* fn_callback, int listenerType);

/**
 * Unregisters all registered listeners of the given type.
 *
 * @param listenerType type of the listeners that must be unregistered
 * or LISTENER_TYPE_ANY to unregister all listeners
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError remove_all_event_listeners_impl(int listenerType);

#ifdef __cplusplus
}
#endif

#endif /* _LISTENERS_INTERN_H_ */
