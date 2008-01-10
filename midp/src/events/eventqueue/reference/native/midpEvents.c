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

#include <stdio.h>
#include <string.h>

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>

#include <jvmspi.h>
#include <sni.h>

#include <commonKNIMacros.h>
#include <ROMStructs.h>
#include <midpMalloc.h>
#include <midpMidletSuiteUtils.h>
#include <midpServices.h>
#include <midpEvents.h>
#include <midpError.h>
#include <midp_constants_data.h>
#include <midp_logging.h>
#include <midp_thread.h>
#include <midpport_eventqueue.h>
#include <pcsl_string.h>
#include <midpUtilKni.h>
#include <midp_properties_port.h>


typedef struct Java_com_sun_midp_events_EventQueue _eventQueue;
#define getEventQueuePtr(handle) (unhand(_eventQueue,(handle)))

#define SET_STRING_EVENT_FIELD(VALUE, STRING, OBJ, ID) \
    if (pcsl_string_utf16_length(&VALUE) >= 0) { \
        midp_jstring_from_pcsl_string(&VALUE, STRING); \
        KNI_SetObjectField(OBJ, ID, STRING); \
    }

#define GET_STRING_EVENT_FIELD(OBJ, ID, STRING, RESULT) \
    KNI_GetObjectField(OBJ, ID, STRING); \
    { \
        pcsl_string __temp; \
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(STRING, &__temp)) { \
            KNI_ThrowNew(midpOutOfMemoryError, "send native event sp"); \
            break; \
        } \
        RESULT = __temp; \
    }

/**
 * @file
 * Native methods to support the queuing of native methods to be processed
 * by a Java event thread. Each Isolate has its own queue, in SVM mode
 * the code will work as if there is only one Isolate.
 */
static int maxIsolates = 1; /* will be reset in MVM mode in initialize */

/* 
 * NOTE: MAX_EVENTS is defined in the constants.xml 
 * in the configuration module 
 */

typedef struct _EventQueue {
    /** Queue of pending events */
    MidpEvent events[MAX_EVENTS];
    /** Number of events in the queue */
    int numEvents;
    /** The queue position of the next event to be processed */
    int eventIn;
    /** The queue position of the next event to be stored */
    int eventOut;
    /** Thread state for each Java native event monitor. */
    jboolean isMonitorBlocked;
} EventQueue;

/** Queues of pending events, one per Isolate or 1 for SVM mode */
static EventQueue* pEventQueues = NULL;

/*
 * Looking up field IDs takes some time, but does not change during a VM
 * session so the field IDs can be cached to save time. These IDs are only
 * access by native methods running in the VM thread.
 */
static int eventFieldIDsObtained = 0;
static jfieldID typeFieldID;
static jfieldID intParam1FieldID;
static jfieldID intParam2FieldID;
static jfieldID intParam3FieldID;
static jfieldID intParam4FieldID;
static jfieldID intParam5FieldID;
static jfieldID stringParam1FieldID;
static jfieldID stringParam2FieldID;
static jfieldID stringParam3FieldID;
static jfieldID stringParam4FieldID;
static jfieldID stringParam5FieldID;
static jfieldID stringParam6FieldID;

/**
 * Gets the event queue associated with an Isolate.
 *
 * @param isolateId ID of an Isolate or 0 for SVM mode
 *
 * @return an event queue
 */
static EventQueue* getIsolateEventQueue(int isolateId) {
    /*
     * Note: Using Isolate IDs as an event queue array index is only done
     * here for performance reasons and should NOT
     * be used in other parts of the system. In other parts of
     * the system something like a matching search should be used.
     */
    if (isolateId < 0 || isolateId >= maxIsolates) {
        REPORT_CRIT1(LC_CORE,
                     "Assertion failed: Isolate ID (%d) out of bounds",
                     isolateId);

        // avoid a SEGV;
        isolateId = 0;
    }

    return &(pEventQueues[isolateId]);
}

/**
 * Gets the field ids of a Java event object and cache them
 * in local static storage.
 *
 * @param eventObj handle to an NativeEvent Java object
 * @param classObj handle to the NativeEvent class
 */
static void
cacheEventFieldIDs(jobject eventObj, jclass classObj) {
    if (eventFieldIDsObtained) {
        return;
    }

    KNI_GetObjectClass(eventObj, classObj);

    typeFieldID = KNI_GetFieldID(classObj, "type", "I");

    intParam1FieldID = KNI_GetFieldID(classObj, "intParam1", "I");
    intParam2FieldID = KNI_GetFieldID(classObj, "intParam2", "I");
    intParam3FieldID = KNI_GetFieldID(classObj, "intParam3", "I");
    intParam4FieldID = KNI_GetFieldID(classObj, "intParam4", "I");
    intParam5FieldID = KNI_GetFieldID(classObj, "intParam5", "I");

    stringParam1FieldID = KNI_GetFieldID(classObj, "stringParam1",
                                         "Ljava/lang/String;");
    stringParam2FieldID = KNI_GetFieldID(classObj, "stringParam2",
                                         "Ljava/lang/String;");
    stringParam3FieldID = KNI_GetFieldID(classObj, "stringParam3",
                                         "Ljava/lang/String;");
    stringParam4FieldID = KNI_GetFieldID(classObj, "stringParam4",
                                         "Ljava/lang/String;");
    stringParam5FieldID = KNI_GetFieldID(classObj, "stringParam5",
                                         "Ljava/lang/String;");
    stringParam6FieldID = KNI_GetFieldID(classObj, "stringParam6",
                                         "Ljava/lang/String;");

    eventFieldIDsObtained = 1;
}

/**
 * De-allocates the fields of an event.
 *
 * @param event The event to be freed
 */
static void
freeMIDPEventFields(MidpEvent event) {
    pcsl_string_free(&event.stringParam1);
    pcsl_string_free(&event.stringParam2);
    pcsl_string_free(&event.stringParam3);
    pcsl_string_free(&event.stringParam4);
    pcsl_string_free(&event.stringParam5);
    pcsl_string_free(&event.stringParam6);
}    

/**
 * Gets the next pending event for an isolate.
 * <p>
 * <b>NOTE:</b> Any string parameter data must be de-allocated with
 * <tt>midpFree</tt>.
 *
 * @param pResult where to put the pending event
 * @param isolateId ID of an Isolate or 0 for SVM mode
 *
 * @return -1 for no event pending, number of event still pending after this
 * event
 */
static int
getPendingMIDPEvent(MidpEvent* pResult, int isolateId) {
    EventQueue* pEventQueue = getIsolateEventQueue(isolateId);

    if (pEventQueue->numEvents == 0) {
        return -1;
    }

    *pResult = pEventQueue->events[pEventQueue->eventOut];

    /* Empty out the events so we do not free it when finalizing. */
    MIDP_EVENT_INITIALIZE(pEventQueue->events[pEventQueue->eventOut]);

    pEventQueue->numEvents--;
    pEventQueue->eventOut++;
    if (pEventQueue->eventOut == MAX_EVENTS) {
        /* This is a circular queue start back a zero. */
        pEventQueue->eventOut = 0;
    }

    return pEventQueue->numEvents;
}

/**
 * Reset an event queue.
 *
 * @param handle handle of the event queue.
 */
static void resetEventQueue(int handle) {
    MidpEvent event;

    if (NULL == pEventQueues) {
        return;
    }

    pEventQueues[handle].isMonitorBlocked = KNI_FALSE;

    while (getPendingMIDPEvent(&event, handle) != -1) {
        freeMIDPEventFields(event);
    }
}

/**
 * Initializes the event system.
 * <p>
 * <b>NOTE:</b> The event system must be explicitly initialize so the
 * VM can shutdown and restart cleanly.
 *
 * @return 0 for success, or non-zero if the MIDP implementation is
 * out of memory
 */
int
InitializeEvents(void) {
    int sizeInBytes;

    if (NULL != pEventQueues) {
        /* already done */
        return 0;
    }

#if ENABLE_MULTIPLE_ISOLATES
    maxIsolates = JVM_MaxIsolates();
#endif

    sizeInBytes = maxIsolates * sizeof (EventQueue);

    pEventQueues = midpMalloc(sizeInBytes);
    if (NULL == pEventQueues) {
        return -1;
    }

    memset(pEventQueues, 0, sizeInBytes);

    midp_createEventQueueLock();

    return 0;
}

/**
 * Finalizes the event system.
 */
void
FinalizeEvents(void) {
    midp_destroyEventQueueLock();

    if (pEventQueues != NULL) {
        midp_resetEvents();
        midpFree(pEventQueues);
        pEventQueues = NULL;
    }
}

/**
 * Resets the all internal event queues, clearing and freeing
 * any pending events.
 */
void
midp_resetEvents(void) {
    int i;

    // The Event ID may have changed for each VM startup
    eventFieldIDsObtained = KNI_FALSE;

    for (i = 0; i < maxIsolates; i++) {
        resetEventQueue(i);
    }
}

/**
 * Helper function used by StoreMIDPEventInVmThread
 * Enqueues an event to be processed by the
 * Java event thread for a given Isolate


 */

static void StoreMIDPEventInVmThreadImp(MidpEvent event, int isolateId) {
    EventQueue* pEventQueue;
    JVMSPI_ThreadID thread;

    pEventQueue = getIsolateEventQueue(isolateId);

    midp_logThreadId("StoreMIDPEventInVmThread");

    midp_waitAndLockEventQueue();

    if (pEventQueue->numEvents != MAX_EVENTS) {

        pEventQueue->events[pEventQueue->eventIn] = event;
        pEventQueue->eventIn++;
        if (pEventQueue->eventIn == MAX_EVENTS) {
            /* This is a circular queue, so start back at zero. */
            pEventQueue->eventIn = 0;
        }
      
        pEventQueue->numEvents++;

        if (pEventQueue->isMonitorBlocked) {
            /*
             * The event monitor thread has been saved as the "special" thread
             * of this particular isolate in order to avoid having to search
             * the entire list of threads.
             */
            thread = SNI_GetSpecialThread(isolateId);
            if (thread != NULL) {
                midp_thread_unblock(thread);
                pEventQueue->isMonitorBlocked = KNI_FALSE;
            } else {
                REPORT_CRIT(LC_CORE,
                    "StoreMIDPEventInVmThread: cannot find "
                    "native event monitor thread");
            }
        }
    } else {
        /*
         * Ignore the event; there is no space to store it.
         * IMPL NOTE: this should be fixed, or it should be a fatal error; 
         * dropping an event can lead to a full system deadlock.
         */
        REPORT_CRIT1(LC_CORE,"**event queue %d full, dropping event",
                     isolateId); 
    }

    midp_unlockEventQueue();
}


/**
 * Enqueues an event to be processed by the Java event thread for a given
 * Isolate, or all isolates if isolateId is -1.
 * Only safe to call from VM thread.
 * Any other threads should call StoreMIDPEvent. 
 *
 * @param event      The event to enqueue.
 *
 * @param isolateId  ID of an Isolate 
 *                   -1 for broadcast to all isolates
 *                   0 for SVM mode
 */
void
StoreMIDPEventInVmThread(MidpEvent event, int isolateId) {

    int max_isolates = getInternalPropertyInt("MAX_ISOLATES");

    if (0 == max_isolates) {
        REPORT_ERROR(LC_AMS, "MAX_ISOLATES property not set");
        return;
    }

    if( -1 != isolateId ) {
        StoreMIDPEventInVmThreadImp(event, isolateId);
    } else {
        for (isolateId = 0; isolateId < max_isolates; ++isolateId)
            StoreMIDPEventInVmThreadImp(event, isolateId);
    }
}

/**
 * Reports a fatal error that cannot be handled in Java. 
 *
 * handleFatalError(Ljava/lang/Throwable;)V
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_events_EventQueue_handleFatalError(void) {
    handleFatalError();
}

/**
 * Reports a fatal error that cannot be handled in Java.
 * Must be called from a KNI method
 *
 */
void handleFatalError(void) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(throwableObj);
    KNI_GetParameterAsObject(1, throwableObj);

    /* IMPL NOTE: Figure out what throwable class this is and log the error? */
    REPORT_CRIT1(LC_CORE, "handleFatalError: uncaught exception in "
        "isolate %d event processing thread", getCurrentIsolateId());
    
    KNI_EndHandles();

    if (getCurrentIsolateId() == midpGetAmsIsolateId()) {
        /* AMS isolate or SVM mode, terminate VM */
        midp_exitVM(-1);
    } else {
        MidpEvent event;

        /* Application isolate, notify the AMS isolate. */
        MIDP_EVENT_INITIALIZE(event);
        event.type = FATAL_ERROR_NOTIFICATION;
        event.intParam1 = getCurrentIsolateId();

        /* Send the shutdown event. */
        StoreMIDPEventInVmThread(event, midpGetAmsIsolateId());
    }

    KNI_ReturnVoid();
}

/**
 * Reads a native event without blocking. Must be called from a KNI method.
 *
 * @param event The parameter is on the java stack,
 *              An empty event to be filled in if there is a queued
 *              event.
 * @param isolateId Isolate ID of the event queue
 * @return -1 for no event read or the number of events still pending after
 * this event
 */
static int readNativeEventCommon(int isolateId) {
    MidpEvent event;
    int eventsPending;

    eventsPending = getPendingMIDPEvent(&event, isolateId);
    if (eventsPending == -1) {
        return eventsPending;
    }

    KNI_StartHandles(3);
    KNI_DeclareHandle(eventObj);
    KNI_DeclareHandle(stringObj);
    KNI_DeclareHandle(classObj);

    KNI_GetParameterAsObject(1, eventObj);

    cacheEventFieldIDs(eventObj, classObj);    

    KNI_SetIntField(eventObj, typeFieldID, event.type);

    KNI_SetIntField(eventObj, intParam1FieldID, event.intParam1);
    KNI_SetIntField(eventObj, intParam2FieldID, event.intParam2);
    KNI_SetIntField(eventObj, intParam3FieldID, event.intParam3);
    KNI_SetIntField(eventObj, intParam4FieldID, event.intParam4);
    KNI_SetIntField(eventObj, intParam5FieldID, event.intParam5);

    SET_STRING_EVENT_FIELD(event.stringParam1, stringObj, eventObj,
                           stringParam1FieldID);
    SET_STRING_EVENT_FIELD(event.stringParam2, stringObj, eventObj,
                           stringParam2FieldID);
    SET_STRING_EVENT_FIELD(event.stringParam3, stringObj, eventObj,
                           stringParam3FieldID);
    SET_STRING_EVENT_FIELD(event.stringParam4, stringObj, eventObj,
                           stringParam4FieldID);
    SET_STRING_EVENT_FIELD(event.stringParam5, stringObj, eventObj,
                           stringParam5FieldID);
    SET_STRING_EVENT_FIELD(event.stringParam6, stringObj, eventObj,
                           stringParam6FieldID);

    freeMIDPEventFields(event);

    KNI_EndHandles();

    return eventsPending;
}

/**
 * Blocks the Java event thread until an event has been queued and returns
 * that event and the number of events still in the queue.
 *
 * @param event An empty event to be filled in if there is a queued
 *              event.
 *
 * @return number of events waiting in the native queue
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_events_NativeEventMonitor_waitForNativeEvent(void) {
    jint isolateId;
    int eventsPending;
    EventQueue* pEventQueue;

    isolateId = getCurrentIsolateId();
    eventsPending = readNativeEventCommon(isolateId);
    if (eventsPending != -1) {
        /* event was read, and more may be pending */
        KNI_ReturnInt(eventsPending);
    }

    pEventQueue = getIsolateEventQueue(isolateId);

    if (pEventQueue->isMonitorBlocked) {
        /*
         * ASSERT: we are about to block, so the state must not indicate
         * that a monitor thread is already blocked.
         */
        REPORT_CRIT(LC_CORE,
            "Assertion failed: NativeEventMonitor.waitForNativeEvent "
            "called when Java thread already blocked");
    }

    /*
     * Block the event processing thread.  To speed up unblocking the
     * event monitor thread, this thread is saved as the "special" thread
     * of an Isolate to avoid having to search the entire list of threads.
     */
    SNI_SetSpecialThread(isolateId);
    SNI_BlockThread();
    pEventQueue->isMonitorBlocked = KNI_TRUE;

    KNI_ReturnInt(0);
}

/**
 * Reads a native event without blocking.
 *
 * @param event An empty event to be filled in if there is a queued
 *              event.
 * @return <tt>true</tt> if an event was read, otherwise <tt>false</tt>
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_events_NativeEventMonitor_readNativeEvent(void) {
    jint isolateId;

    isolateId = getCurrentIsolateId();

    KNI_ReturnBoolean(readNativeEventCommon(isolateId) != -1);
}

/**
 * Sends a native event a given Isolate.
 *
 * @param event A event to queued
 * @param isolateId ID of the target Isolate
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_events_EventQueue_sendNativeEventToIsolate(void) {
    MidpEvent event;
    jint isolateId;
    int noExceptions = 0;

    MIDP_EVENT_INITIALIZE(event);

    KNI_StartHandles(3);
    KNI_DeclareHandle(eventObj);
    KNI_DeclareHandle(stringObj);
    KNI_DeclareHandle(classObj);

    KNI_GetParameterAsObject(1, eventObj);

    isolateId = KNI_GetParameterAsInt(2);

    cacheEventFieldIDs(eventObj, classObj);    

    event.type = KNI_GetIntField(eventObj, typeFieldID);

    event.intParam1 = KNI_GetIntField(eventObj, intParam1FieldID);
    event.intParam2 = KNI_GetIntField(eventObj, intParam2FieldID);
    event.intParam3 = KNI_GetIntField(eventObj, intParam3FieldID);
    event.intParam4 = KNI_GetIntField(eventObj, intParam4FieldID);
    event.intParam5 = KNI_GetIntField(eventObj, intParam5FieldID);

    do {
        GET_STRING_EVENT_FIELD(eventObj, stringParam1FieldID, stringObj,
                               event.stringParam1);
        GET_STRING_EVENT_FIELD(eventObj, stringParam2FieldID, stringObj,
                               event.stringParam2);
        GET_STRING_EVENT_FIELD(eventObj, stringParam3FieldID, stringObj,
                               event.stringParam3);
        GET_STRING_EVENT_FIELD(eventObj, stringParam4FieldID, stringObj,
                               event.stringParam4);
        GET_STRING_EVENT_FIELD(eventObj, stringParam5FieldID, stringObj,
                               event.stringParam5);
        GET_STRING_EVENT_FIELD(eventObj, stringParam6FieldID, stringObj,
                               event.stringParam6);

        noExceptions = 1;
    } while (0);

    KNI_EndHandles();

    if (noExceptions) {
        StoreMIDPEventInVmThread(event, isolateId);
    } else {
        freeMIDPEventFields(event);
    }

    KNI_ReturnVoid();
}

/**
 * Sends a shutdown event to the event queue of the current Isolate.
 *
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_events_EventQueue_sendShutdownEvent(void) {
    MidpEvent event;
    jint isolateId;

    /* Initialize the event with just the SHUTDOWN type */
    MIDP_EVENT_INITIALIZE(event);
    event.type = EVENT_QUEUE_SHUTDOWN;

    /* Send the shutdown event. */
    isolateId = getCurrentIsolateId();
    StoreMIDPEventInVmThread(event, isolateId);

    KNI_ReturnVoid();
}


/**
 * Clears native event queue for a given isolate - 
 * there could be some events from isolate's previous usage.
 *
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_events_EventQueue_resetNativeEventQueue(void) {
    resetEventQueue(getCurrentIsolateId());
}

/**
 * Returns the native event queue handle for use by the native
 * finalizer.
 *
 * @return Native event queue handle
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_events_EventQueue_getNativeEventQueueHandle(void) {
    /* For now use Isolate IDs for event queue handles. */
    return getCurrentIsolateId();
}

/**
 * Native finalizer to reset the native peer event queue when
 * the Isolate ends.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_events_EventQueue_finalize(void) {
   jint handle;

   KNI_StartHandles(1);
   KNI_DeclareHandle(thisObject);
   KNI_GetThisPointer(thisObject);

   SNI_BEGIN_RAW_POINTERS;
   handle = getEventQueuePtr(thisObject)->nativeEventQueueHandle;
   SNI_END_RAW_POINTERS;

   KNI_EndHandles();

   if (handle >= 0) {
       resetEventQueue(handle);
   }

   KNI_ReturnVoid();
}

