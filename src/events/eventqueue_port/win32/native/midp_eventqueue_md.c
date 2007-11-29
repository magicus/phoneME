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

#include <kni.h>
#include <midpport_eventqueue.h>
#include <win32app_export.h>

#include <windows.h>
#include <malloc.h>
#include <memory.h>

#include <midpMalloc.h>

/* thread safety */
static int specialId;

/**
 * @file
 *
 * Platform specific system services, such as event handling.
 */

/*=========================================================================
 * Event handling functions
 *=======================================================================*/

/** Create the event queue lock. */
void
midp_createEventQueueLock(void) {
    /*
     * The Master mode needs a lock since all events put in asynchronously.
     * But slave mode may not if events can be put in the system GUI event
     * system.
     */
    HANDLE *mutex;

    specialId = TlsAlloc();
    mutex = midpMalloc(sizeof(HANDLE));
    if (mutex == NULL) {
        return;
    }

    TlsSetValue(specialId, mutex);
    *mutex = CreateMutex(0, KNI_FALSE, "eventQueueMutex");
}

/** Destroy the event queue lock. */
void
midp_destroyEventQueueLock(void) {
    /* Clean up thread local data */
    void* ptr = (void*) TlsGetValue(specialId);

    if (ptr != NULL) {
        /* Must free TLS data before freeing the TLS (special) ID */
        midpFree(ptr);
        TlsFree(specialId);
    }
}

/** Wait to get the event queue lock and then lock it. */
void
midp_waitAndLockEventQueue(void) {
    HANDLE *mutex = (HANDLE*) TlsGetValue(specialId);
    WaitForSingleObject(*mutex, INFINITE);
}

/** Unlock the event queue. */
void
midp_unlockEventQueue(void) {
    HANDLE *mutex = (HANDLE*) TlsGetValue(specialId);
    ReleaseMutex(*mutex);
}

/**
 * Enqueues an event to be processed by the Java event thread.
 * This can be used by any native thread. If the caller is running
 * in the VM thread it can call StoreMIDPEventInVmThread instead to cut down on
 * event latency.
 *
 * @param event The event to enqueue.
 *
 * @param isolateId ID of an Isolate or 0 for SMV mode
 */
void
StoreMIDPEvent(MidpEvent event, int isolateId) {
    MidpEvent *ptrEvent = (MidpEvent)malloc(sizeof(MidpEvent));
	if (ptrEvent == NULL) {
		return;
	}
	memcpy(ptrEvent, &event, sizeof(MidpEvent));
	if (!postMessage(win32app_get_window_handle(),
		WM_EVENT_FROM_NATIVE_THREAD,
		(WPARAM)ptrEvent,
		(LPARAM)isolateId)) {
			free(ptrEvent);
	}
}

/**
 * Places an event which posted into windows queue into event queue.
 * This function is called when the main window receive a midp event
 * for placing into event queue.
 *
 * @param pEvent The pointer to midp event. This event must be free in this function.
 *
 * @param isolateId ID of an Isolate or 0 for SMV mode
 */
void
StoreMIDPEventFromWindowQueue(MidpEvent *pEvent, int isolateId) {
    MidpEvent event;
	memcpy(&event, pEvent, sizeof(MidpEvent));
	free(pEvent);
    StoreMIDPEventInVmThread(event, isolateId);
}
