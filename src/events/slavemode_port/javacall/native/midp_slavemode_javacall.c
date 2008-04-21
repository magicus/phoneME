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

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>

#include <midp_slavemode_port.h>
#include <midpEvents.h>
#include <midpEventUtil.h>
#include <push_server_export.h>
#include <midp_thread.h>
#include <midp_run_vm.h>
#include <midpResourceLimit.h>
#include <midpAMS.h>
#include <javacall_events.h>
#include <javautil_string.h>

#if ENABLE_JSR_135
#include <javacall_multimedia.h>
#endif

#include <javacall_lifecycle.h>
#include "midp_jc_event_defs.h"

#include <midp_logging.h>
#include <javacall_logging.h>
#if (ENABLE_JSR_120 || ENABLE_JSR_205)
//#include <wmaSocket.h>
#include <wmaInterface.h>
#endif

#include <gcf_export.h>
#include <pcsl_network.h>

extern void jcapp_refresh_pending(javacall_time_milliseconds timeTowaitInMillisec);
extern void measureStack(int clearStack);
extern jlong midp_slavemode_time_slice(void);

static jlong midpTimeSlice(void);

/**
 * Data struct for linked list with each node encapsulating a single event
 */
typedef struct _Event {
    unsigned char data[100];
    int dataLen;
} Event;

Event eventsArray[MAX_EVENTS];

static int index = 0;
static int size = 0;

/**
 * Free the event result. Called when no waiting Java thread was found to
 * receive the result. This may be empty on some systems.
 *
 * @param waitingFor what signal the result is for
 * @param pResult the result set by checkForSystemSignal
 */
void midpFreeEventResult(int waitingFor, void* pResult) {
    (void)waitingFor;
    (void)pResult;
}


/**
 * Waits for an incoming event message and copies it to user supplied
 * data buffer
 * @param waitForever indicate if the function should block forever
 * @param timeTowaitInMillisec max number of seconds to wait
 *              if waitForever is false
 * @param binaryBuffer user-supplied buffer to copy event to
 * @param binaryBufferMaxLen maximum buffer size that an event can be
 *              copied to.
 *              If an event exceeds the binaryBufferMaxLen, then the first
 *              binaryBufferMaxLen bytes of the events will be copied
 *              to user-supplied binaryBuffer, and JAVACALL_OUT_OF_MEMORY will
 *              be returned
 * @param outEventLen user-supplied pointer to variable that will hold actual
 *              event size received
 *              Platform is responsible to set this value on success to the
 *              size of the event received, or 0 on failure.
 *              If outEventLen is NULL, the event size is not returned.
 * @return <tt>JAVACALL_OK</tt> if an event successfully received,
 *         <tt>JAVACALL_FAIL</tt> or if failed or no messages are avaialable
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> If an event's size exceeds the
 *         binaryBufferMaxLen
 */

javacall_result javacall_event_receive(
                                    long            timeTowaitInMillisec,
                            /*OUT*/ unsigned char*  binaryBuffer,
                            /*IN*/  int             binaryBufferMaxLen,
                            /*OUT*/ int*            outEventLen) {


    if (size == 0) {
        return JAVACALL_FAIL;
    }

    if (index == 0) {
        index = MAX_EVENTS - 1;
    }
    else {
        index--;
    }

    if(eventsArray[index].dataLen > binaryBufferMaxLen) {
        /*if not enough memory, we keep the event in the list so that client code can re-invoke with bigger buffer*/
        *outEventLen = 0;
        return JAVACALL_OUT_OF_MEMORY;
    }

    *outEventLen = eventsArray[index].dataLen;
    memcpy(binaryBuffer, eventsArray[index].data, *outEventLen);

	size--;

    return JAVACALL_OK;
}

/**
 * copies a user supplied event message to a queue of messages
 *
 * @param binaryBuffer a pointer to binary event buffer to send
 *        The platform should make a private copy of this buffer as
 *        access to it is not allowed after the function call.
 * @param binaryBufferLen size of binary event buffer to send
 * @return <tt>JAVACALL_OK</tt> if an event successfully sent,
 *         <tt>JAVACALL_FAIL</tt> or negative value if failed
 */
javacall_result javacall_event_send(unsigned char* binaryBuffer,
                                    int binaryBufferLen) {
    if (size == MAX_EVENTS) {
        return JAVACALL_FAIL;
    }
    eventsArray[index].dataLen = binaryBufferLen;
    memcpy(eventsArray[index].data, binaryBuffer, binaryBufferLen);
    index = (index + 1) % MAX_EVENTS;
    size++;
    return JAVACALL_OK;
}


/**
 * Unblock a Java thread.
 *
 * @param blocked_threads blocked threads
 * @param blocked_threads_count number of blocked threads
 * @param waitingFor signal type
 * @param descriptor platform specific handle
 * @param status error code produced by the operation that unblocked the thread
 *
 * @return <tt>1</tt> if a thread was unblocked, otherwise <tt>0</tt>.
 */
static int
eventUnblockJavaThread(
        JVMSPI_BlockedThreadInfo *blocked_threads,
        int blocked_threads_count, unsigned int waitingFor,
        int descriptor, int status)
{
    /*
     * IMPL NOTE: this functionality is similar to midp_thread_signal_list.
     * It differs in that it reports to the caller whether a thread was
     * unblocked. This is a poor interface and should be removed. However,
     * the coupling with Push needs to be resolved first. In addition,
     * freeing of pResult here seems unsafe. Management of pResult needs
     * to be revisited.
     */
    int i;
    MidpReentryData* pThreadReentryData;

    for (i = 0; i < blocked_threads_count; i++) {
        pThreadReentryData =
            (MidpReentryData*)(blocked_threads[i].reentry_data);

        if (pThreadReentryData == NULL) {
            continue;
        }

        if (pThreadReentryData != NULL
                && pThreadReentryData->descriptor == descriptor
                && pThreadReentryData->waitingFor == (midpSignalType)waitingFor) {
            pThreadReentryData->status = status;
            midp_thread_unblock(blocked_threads[i].thread_id);
            return 1;
        }
        if (waitingFor == NO_SIGNAL
            && pThreadReentryData->descriptor == descriptor) {
            pThreadReentryData->status = status;
            /**
            * mark this thread as unblocked so that it will not be unblocked
            * again without being blocked first.
            */
            pThreadReentryData->waitingFor = -1;
            javacall_print("eventUnblockJavaThread without signal!\n");
            midp_thread_unblock(blocked_threads[i].thread_id);
            return 1;
        }
    }

    return 0;
}

/**
 * Requests that the VM control code schedule a time slice as soon
 * as possible, since Java threads are waiting to be run.
 */



/*
 * This function is called by the VM periodically. It has to check if
 * system has sent a signal to MIDP and return the result in the
 * structs given.
 *
 * Values for the <timeout> paramater:
 *  >0 = Block until a signal sent to MIDP, or until <timeout> milliseconds
 *       has elapsed.
 *   0 = Check the system for a signal but do not block. Return to the
 *       caller immediately regardless of the if a signal was sent.
 *  -1 = Do not timeout. Block until a signal is sent to MIDP.
 */
javacall_result checkForSystemSignal(MidpReentryData* pNewSignal,
                          MidpEvent* pNewMidpEvent,
                          JVMSPI_BlockedThreadInfo *blocked_threads,
                          int blocked_threads_count,
                          jlong timeout) {

    midp_jc_event_union *event;
    static unsigned char binaryBuffer[BINARY_BUFFER_MAX_LEN];
    javacall_bool res;
    int outEventLen;

    res = javacall_event_receive ((long)timeout, binaryBuffer, BINARY_BUFFER_MAX_LEN, &outEventLen);

    if (!JAVACALL_SUCCEEDED(res)) {
        return res;
    }

    event = (midp_jc_event_union *) binaryBuffer;

    switch (event->eventType) {
    case MIDP_JC_EVENT_KEY:
        pNewSignal->waitingFor = UI_SIGNAL;
        pNewMidpEvent->type    = MIDP_KEY_EVENT;
        pNewMidpEvent->CHR     = event->data.keyEvent.key;
        pNewMidpEvent->ACTION  = event->data.keyEvent.keyEventType;
        /* IMPL_NOTE: CR <6658788>, temporary changes to process wrong
         * repeated key events on Shift+<0..9> passed from emulator.
         * Should be reverted as soon as the emulator is fixed.
         */
        if( event->data.keyEvent.keyEventType == JAVACALL_KEYTYPED ) {
            char buf[2];
            buf[0] = event->data.keyEvent.key;
            buf[1] = 0;
            pcsl_string_from_chars(buf,&(pNewMidpEvent->stringParam1));
        } else {
            pNewMidpEvent->CHR     = event->data.keyEvent.key;
        }
        /* IMPL_NOTE: End of temporary changes for CR <6658788>
         * Original source:
         * pNewMidpEvent->CHR     = event->data.keyEvent.key;
         */
        break;
    case MIDP_JC_EVENT_PEN:
        pNewSignal->waitingFor = UI_SIGNAL;
        pNewMidpEvent->type    = MIDP_PEN_EVENT;
        pNewMidpEvent->ACTION  = event->data.penEvent.type;
        pNewMidpEvent->X_POS   = event->data.penEvent.x;
        pNewMidpEvent->Y_POS   = event->data.penEvent.y;
	break;
    case MIDP_JC_EVENT_SOCKET:
        pNewSignal->waitingFor = event->data.socketEvent.waitingFor;
        pNewSignal->descriptor = (int)event->data.socketEvent.handle;
        pNewSignal->status     = event->data.socketEvent.status;
        pNewSignal->pResult    = (void *) event->data.socketEvent.extraData;
        break;
    case MIDP_JC_EVENT_NETWORK:
        pNewSignal->waitingFor = NETWORK_INIT_SIGNAL;
        pNewSignal->status = event->data.networkEvent.netType;
        break;
    case MIDP_JC_EVENT_END:
        pNewSignal->waitingFor = AMS_SIGNAL;
        pNewMidpEvent->type    = SHUTDOWN_EVENT;
        break;
    case MIDP_JC_EVENT_PAUSE:
        pNewSignal->waitingFor = AMS_SIGNAL;
        pNewMidpEvent->type    = PAUSE_ALL_EVENT;
        break;
    case MIDP_JC_EVENT_RESUME:
        pNewSignal->waitingFor = AMS_SIGNAL;
        pNewMidpEvent->type    = ACTIVATE_ALL_EVENT;
        break;
    case MIDP_JC_EVENT_PUSH:
        pNewSignal->waitingFor = PUSH_ALARM_SIGNAL;
        pNewSignal->descriptor = event->data.pushEvent.alarmHandle;
        break;
    case MIDP_JC_EVENT_ROTATION:
        pNewSignal->waitingFor = UI_SIGNAL;
        pNewMidpEvent->type    = ROTATION_EVENT;
        break;

#if ENABLE_ON_DEVICE_DEBUG
    case MIDP_JC_ENABLE_ODD_EVENT:
        pNewSignal->waitingFor = AMS_SIGNAL;
        pNewMidpEvent->type = MIDP_ENABLE_ODD_EVENT;
        break;
#endif

#ifdef ENABLE_JSR_75
    case JSR75_FC_JC_EVENT_ROOTCHANGED:
        notifyDisksChanged();
        break;
#endif

#if ENABLE_JSR_120
    case MIDP_JC_EVENT_SMS_INCOMING:
        pNewSignal->waitingFor = WMA_SMS_READ_SIGNAL;
        pNewSignal->descriptor = event->data.smsIncomingEvent.stub;
        break;
    case MIDP_JC_EVENT_CBS_INCOMING:
        pNewSignal->waitingFor = WMA_CBS_READ_SIGNAL;
        pNewSignal->descriptor = event->data.cbsIncomingEvent.stub;
        break;
    case MIDP_JC_EVENT_SMS_SENDING_RESULT:
        pNewSignal->waitingFor = WMA_SMS_WRITE_SIGNAL;
        pNewSignal->descriptor = (int)event->data.smsSendingResultEvent.handle;
        pNewSignal->status = event->data.smsSendingResultEvent.result;
        break;
#endif
#if ENABLE_JSR_205
    case MIDP_JC_EVENT_MMS_INCOMING:
        pNewSignal->waitingFor = WMA_MMS_READ_SIGNAL;
        pNewSignal->descriptor = event->data.mmsIncomingEvent.stub;
        break;
    case MIDP_JC_EVENT_MMS_SENDING_RESULT:
        pNewSignal->waitingFor = WMA_MMS_WRITE_SIGNAL;
        pNewSignal->descriptor = (int)event->data.mmsSendingResultEvent.handle;
        pNewSignal->status = event->data.mmsSendingResultEvent.result;
        break;
#endif

    case MIDP_JC_EVENT_MULTIMEDIA:
#if ENABLE_JSR_135

        if( JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED == event->data.multimediaEvent.mediaType ) {
            pNewSignal->waitingFor = MEDIA_SNAPSHOT_SIGNAL;
//            pNewSignal->descriptor = (((event->data.multimediaEvent.isolateId & 0xFFFF) << 16)
//                                     | (event->data.multimediaEvent.playerId & 0xFFFF));

            REPORT_CALL_TRACE1(LC_NONE, "[media event] JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED %d\n",
                               pNewSignal->descriptor);
        } else {
            pNewSignal->waitingFor = MEDIA_EVENT_SIGNAL;
        }

        pNewSignal->status = JAVACALL_OK;

        pNewMidpEvent->type         = MMAPI_EVENT;
        pNewMidpEvent->MM_PLAYER_ID = event->data.multimediaEvent.playerId;
        pNewMidpEvent->MM_DATA      = event->data.multimediaEvent.data;
        pNewMidpEvent->MM_ISOLATE   = event->data.multimediaEvent.appId;
        pNewMidpEvent->MM_EVT_TYPE  = event->data.multimediaEvent.mediaType;
        pNewMidpEvent->MM_EVT_STATUS= event->data.multimediaEvent.status;

        /* VOLUME_CHANGED event must be sent to all players.             */
        /* MM_ISOLATE = -1 causes bradcast by StoreMIDPEventInVmThread() */
        if( JAVACALL_EVENT_MEDIA_VOLUME_CHANGED == event->data.multimediaEvent.mediaType )
            pNewMidpEvent->MM_ISOLATE = -1;

        REPORT_CALL_TRACE4(LC_NONE, "[media event] External event recevied %d %d %d %d\n",
                pNewMidpEvent->type,
                event->data.multimediaEvent.appId,
                pNewMidpEvent->MM_PLAYER_ID,
                pNewMidpEvent->MM_DATA);
#endif
        break;
#ifdef ENABLE_JSR_234
    case MIDP_JC_EVENT_ADVANCED_MULTIMEDIA:
        pNewSignal->waitingFor = MEDIA_EVENT_SIGNAL;
        pNewSignal->status     = JAVACALL_OK;

        pNewMidpEvent->type         = AMMS_EVENT;
        pNewMidpEvent->MM_PLAYER_ID = event->data.multimediaEvent.playerId;
        pNewMidpEvent->MM_DATA      = event->data.multimediaEvent.data;
        pNewMidpEvent->MM_ISOLATE   = event->data.multimediaEvent.appId;
        pNewMidpEvent->MM_EVT_TYPE  = event->data.multimediaEvent.mediaType;

        REPORT_CALL_TRACE4(LC_NONE, "[jsr234 event] External event recevied %d %d %d %d\n",
            pNewMidpEvent->type,
            event->data.multimediaEvent.appId,
            pNewMidpEvent->MM_PLAYER_ID,
            pNewMidpEvent->MM_DATA);

        break;
#endif
#ifdef ENABLE_JSR_179
    case JSR179_LOCATION_JC_EVENT:
        pNewSignal->waitingFor = event->data.jsr179LocationEvent.event;
        pNewSignal->descriptor = (int)event->data.jsr179LocationEvent.provider;
        pNewSignal->status = event->data.jsr179LocationEvent.operation_result;
        REPORT_CALL_TRACE2(LC_NONE, "[jsr179 event] JSR179_LOCATION_SIGNAL %d %d\n", pNewSignal->descriptor, pNewSignal->status);
        break;
    case JSR179_PROXIMITY_JC_EVENT:
        pNewSignal->waitingFor = JSR179_PROXIMITY_SIGNAL;
        pNewSignal->descriptor = (int)event->data.jsr179ProximityEvent.provider;
        pNewSignal->status = event->data.jsr179ProximityEvent.operation_result;
        REPORT_CALL_TRACE2(LC_NONE, "[jsr179 event] JSR179_PROXIMITY_SIGNAL %d %d\n", pNewSignal->descriptor, pNewSignal->status);
        break;
#endif

#ifdef ENABLE_JSR_211
    case JSR211_JC_EVENT_PLATFORM_FINISH:
        pNewSignal->waitingFor = JSR211_PLATFORM_FINISH_SIGNAL;
        pNewSignal->descriptor = event->data.jsr211PlatformEvent.invoc_id;
        pNewSignal->pResult    = event->data.jsr211PlatformEvent.jsr211event;
        pNewMidpEvent->type    = CHAPI_EVENT;
        break;
    case JSR211_JC_EVENT_JAVA_INVOKE:
        pNewSignal->waitingFor = JSR211_JAVA_INVOKE_SIGNAL;
        pNewSignal->descriptor = event->data.jsr211PlatformEvent.invoc_id;
        pNewSignal->pResult    = event->data.jsr211PlatformEvent.jsr211event;
        pNewMidpEvent->type    = CHAPI_EVENT;
        break;
#endif /* ENABLE_JSR_211 */

#ifdef ENABLE_JSR_177
    case MIDP_JC_EVENT_CARDDEVICE:
        switch (event->data.carddeviceEvent.eventType) {
        case MIDP_CARDDEVICE_RESET:
            pNewSignal->waitingFor = CARD_READER_DATA_SIGNAL;
            pNewSignal->descriptor = SIGNAL_RESET;
            pNewSignal->status     = SIGNAL_RESET;
            pNewSignal->pResult    = (void *)event->data.carddeviceEvent.handle;
            break;
        case MIDP_CARDDEVICE_XFER:
            pNewSignal->waitingFor = CARD_READER_DATA_SIGNAL;
            pNewSignal->descriptor = SIGNAL_XFER;
            pNewSignal->status     = SIGNAL_XFER;
            pNewSignal->pResult    = (void *)event->data.carddeviceEvent.handle;
            break;
        case MIDP_CARDDEVICE_UNLOCK:
            pNewSignal->waitingFor = CARD_READER_DATA_SIGNAL;
            pNewSignal->descriptor = SIGNAL_LOCK;
            pNewSignal->status     = SIGNAL_LOCK;
            pNewSignal->pResult    = NULL;
            break;
        default:    /* just ignore invalid event type */
            REPORT_ERROR1(LC_CORE,"Invalid carddevice event type: %d\n",
                event->data.carddeviceEvent.eventType);
            break;
        }
        break;
#endif /* ENABLE_JSR_177 */
#if ENABLE_MULTIPLE_ISOLATES
    case MIDP_JC_EVENT_SWITCH_FOREGROUND:
        pNewSignal->waitingFor = AMS_SIGNAL;
        pNewMidpEvent->type    = SELECT_FOREGROUND_EVENT;
        pNewMidpEvent->intParam1 = 1;
        break;
    case MIDP_JC_EVENT_SELECT_APP:
        pNewSignal->waitingFor = AMS_SIGNAL;
        pNewMidpEvent->type    = SELECT_FOREGROUND_EVENT;
        pNewMidpEvent->intParam1 = 0;
        break;
#endif /* ENABLE_MULTIPLE_ISOLATES */
#if ENABLE_JSR_256
    case JSR256_JC_EVENT_SENSOR_AVAILABLE:
        pNewSignal->waitingFor = JSR256_SIGNAL;
        pNewMidpEvent->type    = SENSOR_EVENT;
        pNewMidpEvent->intParam1 = event->data.jsr256SensorAvailable.sensor_type;
        pNewMidpEvent->intParam2 = event->data.jsr256SensorAvailable.is_available;
        break;
    case JSR256_JC_EVENT_SENSOR_OPEN_CLOSE:
        pNewSignal->waitingFor = JSR256_SIGNAL;
        pNewSignal->descriptor = (int)event->data.jsr256_jc_event_sensor.sensor;
        break;
#endif /* ENABLE_JSR_256 */
    default:
        REPORT_ERROR(LC_CORE,"Unknown event.\n");
        break;
    };

    REPORT_CALL_TRACE(LC_HIGHUI, "LF:STUB:checkForSystemSignal()\n");
    return JAVACALL_OK;
}

/**
 * Handles the native event notification
 *
 * @param blocked_threads blocked threads
 * @param blocked_threads_count number of blocked threads
 * @param timeout wait timeout
 *
 * @return <tt>JAVACALL_OK</tt> if an event successfully received,
 *         <tt>JAVACALL_FAIL</tt> or if failed or no messages are avaialable
 */
static int midp_slavemode_handle_events(JVMSPI_BlockedThreadInfo *blocked_threads,
		       int blocked_threads_count,
		       jlong timeout) {
    int ret = -1;
    static MidpReentryData newSignal;
    static MidpEvent newMidpEvent;

    newSignal.waitingFor = 0;
    newSignal.pResult = NULL;
    MIDP_EVENT_INITIALIZE(newMidpEvent);

    if (checkForSystemSignal(&newSignal, &newMidpEvent,
                          blocked_threads, blocked_threads_count,
                                timeout)
          == JAVACALL_OK){

        switch (newSignal.waitingFor) {
    #if ENABLE_JAVA_DEBUGGER
        case VM_DEBUG_SIGNAL:
            if (midp_isDebuggerActive()) {
                JVM_ProcessDebuggerCmds();
            }

            break;
    #endif // ENABLE_JAVA_DEBUGGER

        case AMS_SIGNAL:
            midpStoreEventAndSignalAms(newMidpEvent);
            break;

        case UI_SIGNAL:
            midpStoreEventAndSignalForeground(newMidpEvent);
            break;

        case NETWORK_READ_SIGNAL:
            if (eventUnblockJavaThread(blocked_threads,
                                       blocked_threads_count, newSignal.waitingFor,
                                       newSignal.descriptor,
                                       newSignal.status))
                /* Processing is done in eventUnblockJavaThread. */;
            else if (findPushBlockedHandle(newSignal.descriptor) != 0) {
                /* The push system is waiting for a read on this descriptor */
                midp_thread_signal_list(blocked_threads, blocked_threads_count,
                                        PUSH_SIGNAL, 0, 0);
            }
#if (ENABLE_JSR_120 || ENABLE_JSR_205)
            else
                jsr120_check_signal(newSignal.waitingFor, newSignal.descriptor, newSignal.status);
#endif
            break;

        case HOST_NAME_LOOKUP_SIGNAL:
        case NETWORK_WRITE_SIGNAL:
#if (ENABLE_JSR_120 || ENABLE_JSR_205)
            if (!jsr120_check_signal(newSignal.waitingFor, newSignal.descriptor, newSignal.status))
#endif
                midp_thread_signal_list(blocked_threads, blocked_threads_count,
                                        newSignal.waitingFor, newSignal.descriptor,
                                        newSignal.status);
            break;

        case NETWORK_EXCEPTION_SIGNAL:
            /* Find both the read and write threads and signal the status. */
            eventUnblockJavaThread(blocked_threads, blocked_threads_count,
                NETWORK_READ_SIGNAL, newSignal.descriptor,
                newSignal.status);
            eventUnblockJavaThread(blocked_threads, blocked_threads_count,
                NETWORK_WRITE_SIGNAL, newSignal.descriptor,
                newSignal.status);
            return;

        case NETWORK_INIT_SIGNAL:
            if(MIDP_NETWORK_UP == newSignal.status) {
                midp_thread_signal_list(blocked_threads,
                        blocked_threads_count, NETWORK_INIT_SIGNAL, 0, PCSL_NET_SUCCESS);
            } else if(MIDP_NETWORK_DOWN == newSignal.status) {
                /* Wakeup all network threads. */
                midp_thread_signal_list(blocked_threads,
                        blocked_threads_count, NETWORK_INIT_SIGNAL, 0, PCSL_NET_IOERROR);
                midp_thread_signal_list(blocked_threads,
                        blocked_threads_count, NETWORK_READ_SIGNAL, 0, PCSL_NET_IOERROR);
                midp_thread_signal_list(blocked_threads,
                        blocked_threads_count, NETWORK_WRITE_SIGNAL, 0, PCSL_NET_IOERROR);
                midp_thread_signal_list(blocked_threads,
                        blocked_threads_count, NETWORK_EXCEPTION_SIGNAL, 0, PCSL_NET_IOERROR);
                midp_thread_signal_list(blocked_threads,
                        blocked_threads_count, HOST_NAME_LOOKUP_SIGNAL, 0, PCSL_NET_IOERROR);
                if(isMidpNetworkConnected()) {
                    unsetMidpNetworkConnected();
                    REPORT_INFO(LC_PROTOCOL, "midp_check_events: network down from server side");
                }
            } else if(MIDP_NETWORK_DOWN_REQUEST == newSignal.status) {
                if(isMidpNetworkConnected()) {
                    if (0 == midpCheckSocketInUse()) {
                        pcsl_network_finalize_start();
                        unsetMidpNetworkConnected();
                        REPORT_INFO(LC_PROTOCOL, "midp_check_events: network closed by MIDP_NETWORK_DOWN_REQUEST");
                    }
                }
            }
            break;

        case PUSH_ALARM_SIGNAL:
            if (findPushTimerBlockedHandle(newSignal.descriptor) != 0) {
                /* The push system is waiting for this alarm */
                midp_thread_signal_list(blocked_threads,
                    blocked_threads_count, PUSH_SIGNAL, 0, 0);
            }

            break;
#if (ENABLE_JSR_135 || ENABLE_JSR_234)
        case MEDIA_EVENT_SIGNAL:
            StoreMIDPEventInVmThread(newMidpEvent, newMidpEvent.MM_ISOLATE);
            eventUnblockJavaThread(blocked_threads, blocked_threads_count,
                    MEDIA_EVENT_SIGNAL, newSignal.descriptor,
                    newSignal.status);
            break;
        case MEDIA_SNAPSHOT_SIGNAL:
            eventUnblockJavaThread(blocked_threads, blocked_threads_count,
                    MEDIA_SNAPSHOT_SIGNAL, newSignal.descriptor,
                    newSignal.status);
            break;
#endif
#ifdef ENABLE_JSR_179
        case JSR179_LOCATION_SIGNAL:
            midp_thread_signal_list(blocked_threads,
                blocked_threads_count, JSR179_LOCATION_SIGNAL, newSignal.descriptor, newSignal.status);
            break;
        case JSR179_ORIENTATION_SIGNAL:
            midp_thread_signal_list(blocked_threads,
                blocked_threads_count, JSR179_ORIENTATION_SIGNAL, newSignal.descriptor, newSignal.status);
            break;
        case JSR179_PROXIMITY_SIGNAL:
            midp_thread_signal_list(blocked_threads,
                blocked_threads_count, JSR179_PROXIMITY_SIGNAL, newSignal.descriptor, newSignal.status);
            break;
#endif /* ENABLE_JSR_179 */

#ifdef ENABLE_JSR_211
        case JSR211_PLATFORM_FINISH_SIGNAL:
            jsr211_process_platform_finish_notification (newSignal.descriptor, newSignal.pResult);
            midpStoreEventAndSignalAms(newMidpEvent);
            break;
        case JSR211_JAVA_INVOKE_SIGNAL:
            jsr211_process_java_invoke_notification (newSignal.descriptor, newSignal.pResult);
            midpStoreEventAndSignalAms(newMidpEvent);
            break;
#endif /*ENABLE_JSR_211  */

#if (ENABLE_JSR_120 || ENABLE_JSR_205)
        case WMA_SMS_READ_SIGNAL:
        case WMA_CBS_READ_SIGNAL:
        case WMA_MMS_READ_SIGNAL:
        case WMA_SMS_WRITE_SIGNAL:
        case WMA_MMS_WRITE_SIGNAL:
             jsr120_check_signal(newSignal.waitingFor, newSignal.descriptor, newSignal.status);
             break;
#endif
#ifdef ENABLE_JSR_177
        case CARD_READER_DATA_SIGNAL:
            midp_thread_signal_list(blocked_threads, blocked_threads_count,
                                    newSignal.waitingFor, newSignal.descriptor,
                                    newSignal.status);
            break;
#endif /* ENABLE_JSR_177 */
#ifdef ENABLE_JSR_256
        case JSR256_SIGNAL:
            if (newMidpEvent.type == SENSOR_EVENT) {
                StoreMIDPEventInVmThread(newMidpEvent, -1);
            } else {
                midp_thread_signal_list(blocked_threads, blocked_threads_count,
                    newSignal.waitingFor, newSignal.descriptor, newSignal.status);
            }
            break;
#endif /* ENABLE_JSR_256 */
        default:
            break;
        } /* switch */
        ret = 0;
    }

    return ret;
}

/*
 * Call this function in slave mode to inform VM of new events.
 */
void javanotify_inform_event(void) {
    int blocked_threads_count;
    JVMSPI_BlockedThreadInfo * blocked_threads = SNI_GetBlockedThreads(&blocked_threads_count);

    int ret = 0;

    while (ret == 0){
        blocked_threads = SNI_GetBlockedThreads(&blocked_threads_count);
        ret = midp_slavemode_handle_events(blocked_threads, blocked_threads_count, 0 /*timeout*/);
    }
}

/*
 * See comments in javacall_lifecycle.h
 */

javacall_int64 javanotify_vm_timeslice(void) {

    return midpTimeSlice();
}



static jlong midpTimeSlice(void) {

    jlong to = midp_slavemode_time_slice();
    javacall_time_milliseconds toInMillisec;

    if (-2 == to) {
        measureStack(KNI_FALSE);
        pushcheckinall();
        midpFinalize();
#if (ENABLE_JSR_120 || ENABLE_JSR_205)
        finalize_jsr205();
#endif
    } else {
        /* convert jlong to long */
        if (to > 0x7FFFFFFF) {
            toInMillisec = -1;
        } else if (to < 0) {
            toInMillisec = -1;
        }   else {
            toInMillisec = (javacall_time_milliseconds)(to&0x7FFFFFFF);
        }

        jcapp_refresh_pending(toInMillisec);
    }

    return to;
}

void midp_slavemode_schedule_vm_timeslice(void) {
    javacall_schedule_vm_timeslice();
}




#ifdef xxENABLE_JSR_82
/**
 * JSR82 Bluetooth Event Handler
 */
static void jsr82HandleBtEvent(JVMSPI_BlockedThreadInfo* blocked_threads, int num_threads)
{
    int i;
    int handle;
    MidpReentryData *pThreadReentryData;
    static char pName[JAVACALL_BT_MAX_USER_FRIENDLY_NAME]; /*FIXME: static? */

    jsr82_event_bt *event = &(current_event.data.jsr82BtEvent);
    REPORT_INFO3(LC_PROTOCOL, "jsr82HandleBtEvent >> handle = %d, waitingFor = %d, status = %d\n",
                event->handle, event->waitingFor, event->status);
    for (i = 0; i < num_threads; i++) {

        pThreadReentryData =
            (MidpReentryData*)(blocked_threads[i].reentry_data);

        if (pThreadReentryData != NULL
             && pThreadReentryData->waitingFor == event->waitingFor
             && pThreadReentryData->descriptor == (int)event->handle) {

            if ((pThreadReentryData->waitingFor == JSR82_BT_AUTHORIZE_SIGNAL) ||
                    (pThreadReentryData->waitingFor == JSR82_BT_BOND_SIGNAL)  ||
                    (pThreadReentryData->waitingFor == JSR82_BT_PASSKEY_SIGNAL)) {
                if((pThreadReentryData->pResult != NULL) &&
                       (0 == memcmp( pThreadReentryData->pResult,
                                     event->address,
                                     JAVACALL_BT_ADDRESS_SIZE)) ) {
                    pThreadReentryData->status = event->status;
                    midp_thread_unblock(blocked_threads[i].thread_id);
                }
            } else if (pThreadReentryData->waitingFor == JSR82_BT_LOCAL_ADDRESS_SIGNAL) {
                /* Use pResult field as the space of local address */
                pThreadReentryData->pResult = (unsigned char*)midpMalloc(sizeof(javacall_bt_address));
                memcpy(pThreadReentryData->pResult, event->localAddr, sizeof(javacall_bt_address));

                pThreadReentryData->status = event->status;
                midp_thread_unblock(blocked_threads[i].thread_id);
            } else if (pThreadReentryData->waitingFor == JSR82_BT_LOCAL_NAME_SIGNAL) {
                /* Use pResult field as the space of local name */
                pThreadReentryData->pResult = (unsigned char*)midpMalloc(strlen(event->localName)+1);
                strcpy(pThreadReentryData->pResult, event->localName);

                pThreadReentryData->status = event->status;
                midp_thread_unblock(blocked_threads[i].thread_id);
            } else if (pThreadReentryData->waitingFor == JSR82_BT_SDDB_UPDATED_SIGNAL) {
                /* Use descriptor field as the space of service record ID */
                pThreadReentryData->descriptor = event->serviceRecordID;
                pThreadReentryData->status = event->status;
                midp_thread_unblock(blocked_threads[i].thread_id);
            } else if (pThreadReentryData->waitingFor == JSR82_BT_RFCOMM_SERVER_CREATED_SIGNAL) {
                /* Use pResult field as the space of server handle */
                pThreadReentryData->pResult = event->sppServerHandle;
                /* Use descriptor field as the space of channel number */
                pThreadReentryData->descriptor = event->sppCn;
                pThreadReentryData->status = event->status;
                midp_thread_unblock(blocked_threads[i].thread_id);
            } else {
                pThreadReentryData->status = event->status;
                midp_thread_unblock(blocked_threads[i].thread_id);
            }
        }
    }
    REPORT_INFO(LC_PROTOCOL, "jsr82HandleBtEvent <<");
}
#endif /* ENABLE_JSR_82 */
