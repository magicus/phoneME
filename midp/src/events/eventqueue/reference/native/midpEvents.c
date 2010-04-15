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

#include <stdlib.h>
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
    /** 
     * Indicates if the queue is currently active, that is, there is 
     * an actual Java queue associated with this native data. Queue 
     * can be inactive at the moment because we preallocate all 
     * the native structures in advance, so it is possible that some 
     * of these structures are currently unused.
     */
    jboolean isActive;    
    /** Thread state for each Java native event monitor. */
    jboolean isMonitorBlocked;
} EventQueue;

/** Queues of pending events, one per Isolate or 1 for SVM mode */
static EventQueue* gsEventQueues = NULL;

/** Total event queues allocated */
static int gsTotalQueues = 1;

/** Max number of Isolates allowed in the system */
#if ENABLE_MULTIPLE_ISOLATES
static int gsMaxIsolates = 1;
#endif

#if ENABLE_EVENT_SPYING
static int gsEventSpyingQueueId = 0;
#endif

/**
 * Macro that gets the event queue associated with an queueId.
 *
 * @param queuePtr variable to assign queue pointer to
 * @param queueId ID of an queue
 */
#define GET_EVENT_QUEUE_BY_ID(queuePtr, queueId)                            \
    if (queueId < 0 || queueId >= gsTotalQueues) {                          \
        REPORT_CRIT1(LC_CORE,                                               \
                     "Assertion failed: Event queue ID (%d) out of bounds", \
                     queueId);                                              \
                                                                            \
        /* avoid a SEGV;*/                                                  \
        queueId = 0;                                                        \
    }                                                                       \
                                                                            \
    queuePtr = &(gsEventQueues[queueId]);


/**
 * Macros for converting between Isolate ID and queue ID. 
 * For Isolate queues, Isolate ID is used as queue ID for
 * the performance reasons. 
 */

/** 
 * Checks if queue is an Isolate queue. If so, then queue ID and Isolate ID
 * are the same thing and can be used interchangeably, as in macros below.
 */
#if ENABLE_MULTIPLE_ISOLATES
#define IS_ISOLATE_QUEUE(queueId) ((queueId) > 0 && (queueId) <= gsMaxIsolates)
#else
#define IS_ISOLATE_QUEUE(queueId) ((queueId) == 0)
#endif

/* Gets Isolate queue ID from Isolate ID */
#define ISOLATE_ID_TO_QUEUE_ID(isolateId) (isolateId)
 
/* Gets Isolate ID from queue ID */
#define QUEUE_ID_TO_ISOLATE_ID(queueId) (queueId)


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

#if ENABLE_MULTIPLE_ISOLATES
/**
 * Duplicates those fields of an event, which require resource allocation.
 *
 * @param event The event to be duplicated
 *
 * @return 0 for success, or non-zero if the MIDP implementation is
 * out of memory
 */
static int
duplicateMIDPEventFields(MidpEvent *event) {
    int i, j;
    MidpEvent e = *event;
    pcsl_string *params[2][6];

    params[0][0] = &e.stringParam1;
    params[0][1] = &e.stringParam2;
    params[0][2] = &e.stringParam3;
    params[0][3] = &e.stringParam4;
    params[0][4] = &e.stringParam5;
    params[0][5] = &e.stringParam6;
    params[1][0] = &event->stringParam1;
    params[1][1] = &event->stringParam2;
    params[1][2] = &event->stringParam3;
    params[1][3] = &event->stringParam4;
    params[1][4] = &event->stringParam5;
    params[1][5] = &event->stringParam6;

    for (i = 0; i < 6; i++) {
        if (PCSL_STRING_OK !=
            pcsl_string_dup(params[0][i], params[1][i])) {
                for (j = 0; j < i; j++) {
                    pcsl_string_free(params[1][j]);
                    *params[1][j] = *params[0][j];
                }
                *params[1][i] = *params[0][i];
                return -1;
            }
    }

    return 0;
}
#endif

/**
 * Gets the next pending event for an isolate.
 * <p>
 * <b>NOTE:</b> Any string parameter data must be de-allocated with
 * <tt>midpFree</tt>.
 *
 * @param pResult where to put the pending event
 * @param queueId queue ID 
 *
 * @return -1 for no event pending, number of event still pending after this
 * event
 */
static int
getPendingMIDPEvent(MidpEvent* pResult, jint queueId) {
    EventQueue* pEventQueue;
    
    GET_EVENT_QUEUE_BY_ID(pEventQueue, queueId);

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
 * @param queueId ID of the queue to reset
 */
static void resetEventQueue(jint queueId) {
    MidpEvent event;

    if (NULL == gsEventQueues) {
        return;
    }

    gsEventQueues[queueId].isMonitorBlocked = KNI_FALSE;

    while (getPendingMIDPEvent(&event, queueId) != -1) {
        freeMIDPEventFields(event);
    }
}

/**
 * Blocks Java thread that monitors specified event queue.
 *
 * @param queueId queue ID
 */
static void blockMonitorThread(jint queueId) {
    EventQueue* pEventQueue;
    jint isolateId;

    GET_EVENT_QUEUE_BY_ID(pEventQueue, queueId);

    if (IS_ISOLATE_QUEUE(queueId)) {
        /*
         * To speed up unblocking the event monitor thread, this thread 
         * is saved as the "special" thread of an Isolate to avoid having 
         * to search the entire list of threads.
         */        
        isolateId = QUEUE_ID_TO_ISOLATE_ID(queueId);
        SNI_SetSpecialThread(isolateId);
        SNI_BlockThread();
    } else {
        /* Block thread in the normal way */
        midp_thread_wait(EVENT_QUEUE_SIGNAL, queueId, 0);
    }

    pEventQueue->isMonitorBlocked = KNI_TRUE;
}

/**
 * Unblocks Java thread that monitors specified event queue.
 *
 * @param queueId queue ID
 */
static void unblockMonitorThread(jint queueId) {
    EventQueue* pEventQueue;
    jint isolateId;
    JVMSPI_ThreadID thread;
    
    GET_EVENT_QUEUE_BY_ID(pEventQueue, queueId);

    if (IS_ISOLATE_QUEUE(queueId)) {
        /*
         * The event monitor thread has been saved as the "special" thread
         * of this particular isolate in order to avoid having to search
         * the entire list of threads.
         */
        isolateId = QUEUE_ID_TO_ISOLATE_ID(queueId);
        thread = SNI_GetSpecialThread(isolateId);
        if (thread != NULL) {
            midp_thread_unblock(thread);
        } else {
            REPORT_CRIT(LC_CORE,
                "StoreMIDPEventInVmThread: cannot find "
                "native event monitor thread");
        }
    } else {
        /* Unblock thread in the normal (slower) way */
        midp_thread_signal(EVENT_QUEUE_SIGNAL, queueId, 0);
    }

    pEventQueue->isMonitorBlocked = KNI_FALSE;    
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

    if (NULL != gsEventQueues) {
        /* already done */
        return 0;
    }

#if ENABLE_MULTIPLE_ISOLATES
    gsMaxIsolates = getMaxIsolates();
    /*
     * In MVM the first isolate has number 1, but 0 still can be returned
     * by midpGetAmsIsolate() if JVM is not running. So in MVM we allocate
     * one more entry in gsEventQueues[] to make indices from 0 to maxIsolates
     * inclusively valid.
     */
    gsTotalQueues = gsMaxIsolates + 1; 
#else
    gsTotalQueues = 1;     
#endif

#if ENABLE_EVENT_SPYING
    gsEventSpyingQueueId = gsTotalQueues;
    gsTotalQueues += 1;
#endif   

    sizeInBytes = gsTotalQueues * sizeof (EventQueue);
    gsEventQueues = midpMalloc(sizeInBytes);
    if (NULL == gsEventQueues) {
        return -1;
    }

    memset(gsEventQueues, 0, sizeInBytes);

    midp_createEventQueueLock();

    return 0;
}

/**
 * Finalizes the event system.
 */
void
FinalizeEvents(void) {
    midp_destroyEventQueueLock();

    if (gsEventQueues != NULL) {
        midp_resetEvents();
        midpFree(gsEventQueues);
        gsEventQueues = NULL;
    }
}

/**
 * Resets the all internal event queues, clearing and freeing
 * any pending events.
 */
void
midp_resetEvents(void) {

    /* The Event ID may have changed for each VM startup*/
    eventFieldIDsObtained = KNI_FALSE;

#if ENABLE_MULTIPLE_ISOLATES
    {
        jint queueId;
        for (queueId = 1; queueId < gsTotalQueues; queueId++) {
            resetEventQueue(queueId);
        }
    }
#else
    resetEventQueue(0);
#endif
}

/**
 * Helper function used by StoreMIDPEventInVmThread. Enqueues an event 
 * to be processed by the Java event thread for a given event queue.
 *
 * @param event the event to enqueue
 * @queueID ID of the queue to enqueue event from
 */
static void StoreMIDPEventInVmThreadImp(MidpEvent event, jint queueId) {
    EventQueue* pEventQueue;

    GET_EVENT_QUEUE_BY_ID(pEventQueue, queueId);

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
            unblockMonitorThread(queueId);
        }
    } else {
        /*
         * Ignore the event; there is no space to store it.
         * IMPL NOTE: this should be fixed, or it should be a fatal error; 
         * dropping an event can lead to a full system deadlock.
         */
        REPORT_CRIT1(LC_CORE,"**event queue %d full, dropping event",
                     queueId); 
    }

    midp_unlockEventQueue();

#if ENABLE_EVENT_SPYING
    if (queueId != gsEventSpyingQueueId) {
        GET_EVENT_QUEUE_BY_ID(pEventQueue, gsEventSpyingQueueId);
        if (pEventQueue->isActive && event.type != EVENT_QUEUE_SHUTDOWN) {
            if (0 == duplicateMIDPEventFields(&event)) {
                StoreMIDPEventInVmThreadImp(event, gsEventSpyingQueueId); 
            } else {
                REPORT_CRIT(LC_CORE, 
                        "StoreMIDPEventInVmThread: Out of memory.");
                return;
            }
        }
    }
#endif
}

/**
 * Enqueues an event to be processed by the Java event thread for a given
 * Isolate, or for all isolates queues if isolateId is -1.
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
    jint queueId = -1;

    if( -1 != isolateId ) {
        queueId = ISOLATE_ID_TO_QUEUE_ID(isolateId);
        StoreMIDPEventInVmThreadImp(event, queueId);
    } else {
#if ENABLE_MULTIPLE_ISOLATES
        EventQueue* pEventQueue;

        StoreMIDPEventInVmThreadImp(event, 1);    
        for (isolateId = 2; isolateId <= gsMaxIsolates; isolateId++) {
            queueId = ISOLATE_ID_TO_QUEUE_ID(isolateId);
            GET_EVENT_QUEUE_BY_ID(pEventQueue, queueId);

            /* 
             * Broadcast only for active queues to avoid overflowing 
             * inactive queues that no one is currently reading events from
             */
            if (pEventQueue->isActive) {
                if (0 != duplicateMIDPEventFields(&event)) {
                    REPORT_CRIT(LC_CORE, "StoreMIDPEventInVmThread: "
                            "Out of memory.");
                    return;
                }

                StoreMIDPEventInVmThreadImp(event, queueId);
            }
        }
#else
        StoreMIDPEventInVmThreadImp(event, 0);
#endif
    }    
}

/**
 * Reports how many events can be enqueued before queue overflows.
 *
 * @param isolateId  ID of an Isolate, 0 for SVM mode
 *
 * @return available queue space
 *         negative value on error
 */
int GetEventQueueFreeCount(int isolateId) {
    jint queueId = -1;
    EventQueue* pEventQueue;

    queueId = ISOLATE_ID_TO_QUEUE_ID(isolateId);
    if (queueId < 0 || queueId >= gsTotalQueues) {
        return -1;
    }
    GET_EVENT_QUEUE_BY_ID(pEventQueue, queueId);

    return MAX_EVENTS - pEventQueue->numEvents;
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
 * @param queueId queue ID
 * @return -1 for no event read or the number of events still pending after
 * this event
 */
static int readNativeEventCommon(jint queueId) {
    MidpEvent event;
    int eventsPending;

    eventsPending = getPendingMIDPEvent(&event, queueId);
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
    jint queueId;
    int eventsPending;
    EventQueue* pEventQueue;

    queueId = KNI_GetParameterAsInt(2);
    eventsPending = readNativeEventCommon(queueId);
    if (eventsPending != -1) {
        /* event was read, and more may be pending */
        KNI_ReturnInt(eventsPending);
    }

    GET_EVENT_QUEUE_BY_ID(pEventQueue, queueId);

    if (pEventQueue->isMonitorBlocked) {
        /*
         * ASSERT: we are about to block, so the state must not indicate
         * that a monitor thread is already blocked.
         */
        REPORT_CRIT(LC_CORE,
            "Assertion failed: NativeEventMonitor.waitForNativeEvent "
            "called when Java thread already blocked");
    }

    blockMonitorThread(queueId);

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
    jint queueId;

    queueId = KNI_GetParameterAsInt(2);

    KNI_ReturnBoolean(readNativeEventCommon(queueId) != -1);
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
Java_com_sun_midp_events_EventQueue_sendShutdownEvent0(void) {
    MidpEvent event;
    jint queueId;

    /* Initialize the event with just the SHUTDOWN type */
    MIDP_EVENT_INITIALIZE(event);
    event.type = EVENT_QUEUE_SHUTDOWN;

    /* Send the shutdown event. */
    queueId = KNI_GetParameterAsInt(1);
    StoreMIDPEventInVmThreadImp(event, queueId);

    KNI_ReturnVoid();
}

/**
 * Clears native event queue for a given isolate - 
 * there could be some events from isolate's previous usage.
 *
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_events_EventQueue_resetNativeEventQueue(void) {
    jint queueId;

    queueId = KNI_GetParameterAsInt(1);
    resetEventQueue(queueId);
}

/**
 * Returns the native event queue handle for use by the native
 * finalizer.
 *
 * @return Native event queue handle
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_events_EventQueue_getNativeEventQueueHandle(void){
   jint queueId;
   EventQueue* pEventQueue;
    
    /* Use Isolate IDs for event queue handles. */
    queueId = ISOLATE_ID_TO_QUEUE_ID(getCurrentIsolateId());

    /* Mark queue as active */
    GET_EVENT_QUEUE_BY_ID(pEventQueue, queueId);
    pEventQueue->isActive = KNI_TRUE;

    KNI_ReturnInt(queueId);
}

#if ENABLE_EVENT_SPYING
/**
 * Returns the native event queue handle for use by the native
 * finalizer.
 *
 * @return Native event queue handle
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_events_EventSpyingQueue_getNativeEventQueueHandle(void){
    EventQueue* pEventQueue;
   
    /* Mark queue as active */
    GET_EVENT_QUEUE_BY_ID(pEventQueue, gsEventSpyingQueueId);
    pEventQueue->isActive = KNI_TRUE;

    KNI_ReturnInt(gsEventSpyingQueueId);
}
#endif

/**
 * Native finalizer to reset the native peer event queue when
 * the Isolate ends.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_events_EventQueue_finalize(void) {
   jint queueId;
   EventQueue* pEventQueue;

   KNI_StartHandles(1);
   KNI_DeclareHandle(thisObject);
   KNI_GetThisPointer(thisObject);

   SNI_BEGIN_RAW_POINTERS;
   queueId = getEventQueuePtr(thisObject)->queueId;
   SNI_END_RAW_POINTERS;

   KNI_EndHandles();

   if (queueId >= 0) {
       resetEventQueue(queueId);

       /* Mark queue as inactive */
       GET_EVENT_QUEUE_BY_ID(pEventQueue, queueId);
       pEventQueue->isActive = KNI_FALSE;
   }

   KNI_ReturnVoid();
}

