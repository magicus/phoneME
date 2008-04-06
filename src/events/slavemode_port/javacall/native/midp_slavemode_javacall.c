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
 *
 * This source file is specific for Qt-based configurations.
 */
#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>

#include <midpEventUtil.h>
#include <push_server_export.h>
#include <midp_thread.h>
#include <midp_run_vm.h>
#include <suspend_resume.h>

#include <midp_logging.h>
#include <midp_slavemode_port.h>
#include <javacall_lifecycle.h>
#include <midp_jc_event_defs.h>

#include <midpServices.h>
#include <midpEvents.h>
#include <midpAMS.h>  // for midpFinalize()

#include <javacall_lifecycle.h>

#ifdef ENABLE_JSR_120
#include <wmaInterface.h>
#endif

#ifdef ENABLE_JSR_75
#include <fcNotifyIsolates.h>
#endif

extern void jcapp_refresh_pending(javacall_time_milliseconds timeTowaitInMillisec);
extern void measureStack(int clearStack);



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

        if ( pThreadReentryData->descriptor == descriptor
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
            REPORT_INFO(LC_CORE, "eventUnblockJavaThread without signal!\n");
            midp_thread_unblock(blocked_threads[i].thread_id);
            return 1;
        }
    }

    return 0;
}

#ifdef ENABLE_JSR_135
/**
 * Unblocks a multimedia Java thread.
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
eventUnblockMultimediaJavaThread(
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

        if (pThreadReentryData != NULL
                && pThreadReentryData->descriptor == descriptor
                && (pThreadReentryData->waitingFor == (midpSignalType)waitingFor
                        || NO_SIGNAL == waitingFor)
                && pThreadReentryData->waitingFor
                    >= (midpSignalType)MEDIA_START_SIGNAL
                && pThreadReentryData->waitingFor
                    <= (midpSignalType)MEDIA_LONG_MIDI_EVENT_SIGNAL) {
            pThreadReentryData->status = status;
            midp_thread_unblock(blocked_threads[i].thread_id);
            return 1;
        }
    }

    return 0;
}

#endif


#ifdef ENABLE_JSR_135
/*
 * Converts a multimedia event to midp internal event
 *
 * @param multimediaEvent event to be converted
 * @param blocked_threads
 * @param blocked_threads_count
 *
 * @return <tt>JAVACALL_OK</tt> on success
 *         <tt>JAVACALL_FAIL</tt> on failure
 *
 */
javacall_result midpHandleMultimediaEvent(midp_jc_event_multimedia multimediaEvent,
                                          JVMSPI_BlockedThreadInfo* blocked_threads,
                                          int blocked_threads_count) {
    MidpEvent newMidpEvent;
    int signal = NO_SIGNAL;

    MIDP_EVENT_INITIALIZE(newMidpEvent);

    switch(multimediaEvent.mediaType) {
    case JAVACALL_EVENT_MEDIA_END_OF_MEDIA:
        newMidpEvent.type = MM_EOM_EVENT;
        break;
    case JAVACALL_EVENT_MEDIA_DURATION_UPDATED:
        newMidpEvent.type = MM_DURATION_EVENT;
        break;
    case JAVACALL_EVENT_MEDIA_RECORD_SIZE_LIMIT:
        newMidpEvent.type = MM_RECORD_LIMIT_EVENT;
        break;
    case JAVACALL_EVENT_MEDIA_RECORD_ERROR:
        newMidpEvent.type = MM_RECORD_ERROR_EVENT;
        break;
    case JAVACALL_EVENT_MEDIA_BUFFERING_STARTED:
        newMidpEvent.type = MM_BUFFERING_START_EVENT;
        break;
    case JAVACALL_EVENT_MEDIA_BUFFERING_STOPPED:
        newMidpEvent.type = MM_BUFFERING_STOP_EVENT;
        break;
    case JAVACALL_EVENT_MEDIA_VOLUME_CHANGED:
        newMidpEvent.type = MM_VOLUME_CHANGED_EVENT;
        break;
    case JAVACALL_EVENT_MEDIA_ERROR:
        newMidpEvent.type = MM_GENERAL_ERROR_EVENT;
        break;
    case JAVACALL_EVENT_MEDIA_STARTED:
        signal = MEDIA_START_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_STOPPED:
        signal = MEDIA_STOP_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_PAUSED:
        signal = MEDIA_PAUSE_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_RESUMED:
        signal = MEDIA_RESUME_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_TIME_SET:
        signal = MEDIA_SET_TIME_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_DURATION_GOTTEN:
        signal = MEDIA_GET_DURATION_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_FRAME_SOUGHT:
        signal = MEDIA_SEEK_FRAME_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_FRAMES_SKIPPED:
        signal = MEDIA_SKIP_FRAMES_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_METADATA_KEY_GOTTEN:
        signal = MEDIA_GET_METADATA_KEY_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_METADATA_GOTTEN:
        signal = MEDIA_GET_METADATA_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_PITCH_GOTTEN:
        signal = MEDIA_GET_PITCH_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_PITCH_SET:
        signal = MEDIA_SET_PITCH_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_RATE_GOTTEN:
        signal = MEDIA_GET_RATE_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_RATE_SET:
        signal = MEDIA_SET_RATE_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_RECORD_STARTED:
        signal = MEDIA_START_RECORD_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_RECORD_STOPPED:
        signal = MEDIA_STOP_RECORD_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_RECORD_PAUSED:
        signal = MEDIA_PAUSE_RECORD_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_RECORD_RESET:
        signal = MEDIA_RESET_RECORD_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_RECORD_COMMITTED:
        signal = MEDIA_COMMIT_RECORD_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED:
        signal = MEDIA_SNAPSHOT_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_TEMPO_GOTTEN:
        signal = MEDIA_GET_TEMPO_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_TEMPO_SET:
        signal = MEDIA_SET_TEMPO_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_VIDEO_HEIGHT_GOTTEN:
        signal = MEDIA_GET_VIDEO_HEIGHT_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_VIDEO_WIDTH_GOTTEN:
        signal = MEDIA_GET_VIDEO_WIDTH_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_VIDEO_LOCATION_SET:
        signal = MEDIA_SET_VIDEO_LOCATION_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_VIDEO_VISIBILITY_SET:
        signal = MEDIA_SET_VIDEO_VISIBILITY_SIGNAL;
        break;
    case JAVACALL_EVENT_MEDIA_SHORT_MIDI_EVENT_FINISHED:
    	signal = MEDIA_SHORT_MIDI_EVENT_SIGNAL;
    	break;
    case JAVACALL_EVENT_MEDIA_LONG_MIDI_EVENT_FINISHED:
    	signal = MEDIA_LONG_MIDI_EVENT_SIGNAL;
    	break;

    case JAVACALL_EVENT_MEDIA_FRAME_RENDERED:
        //LCDUIrefresh(0, 0, LCDUI_getScreenWidth(), LCDUI_getScreenHeight());
        return JAVACALL_OK;

    case JAVACALL_EVENT_MEDIA_STOP_TIME_REACHED:
        newMidpEvent.type = MM_STOP_TIME_REACHED_EVENT;
        break;
    default:
        break;
    }

    if (NO_SIGNAL == signal) {
         newMidpEvent.intParam1 = multimediaEvent.playerId;
         newMidpEvent.intParam2 = (long)multimediaEvent.data;
         newMidpEvent.intParam3 = multimediaEvent.appId;

        REPORT_CALL_TRACE4(LC_MMAPI, "[media event] External event recevied %d %d %d %d\n",
                           newMidpEvent.type,
                           multimediaEvent.appId,
                           newMidpEvent.intParam1,
                           newMidpEvent.intParam2);

        StoreMIDPEventInVmThread(newMidpEvent, multimediaEvent.appId);
	 if (MM_GENERAL_ERROR_EVENT == newMidpEvent.type
	      || MM_RECORD_ERROR_EVENT == newMidpEvent.type
	      || MM_EOM_EVENT == newMidpEvent.type) {
            int descriptor = (((multimediaEvent.appId & 0xFFFF) << 16) | (multimediaEvent.playerId & 0xFFFF));
            eventUnblockMultimediaJavaThread(blocked_threads, blocked_threads_count,
                                                    signal, descriptor, multimediaEvent.status);
	 }

    } else {
        /* HACK - Compose 16 bit of isolate ID and 16 bit of player ID
                       to generate descriptor */
        int descriptor = (((multimediaEvent.appId & 0xFFFF) << 16) | (multimediaEvent.playerId & 0xFFFF));
        REPORT_CALL_TRACE4(LC_MMAPI, "[media event] signal %d, descriptor %d, status %d, data %d\n",
            signal, descriptor, multimediaEvent.status, multimediaEvent.data);
        eventUnblockMultimediaJavaThread(blocked_threads, blocked_threads_count,
                                         signal, descriptor, multimediaEvent.status);
    }
    return JAVACALL_OK;
}
#endif
/*
 * This function is called by the VM periodically. Checks if
 * the native platform has sent a signal to MIDP.
 *
 * @param pNewSignal (out) Structure that will store the signal info
 * @param pNewMidpEvent (out) Structure that will store the midp event info
 * @param blocked_threads (in) Blocked threads
 * @param blocked_threads_count (in) Number of blocked threads
 * @param timeout Wait timeout
 *
 * @return <tt>JAVACALL_OK</tt> if an event successfully received,
 *         <tt>JAVACALL_FAIL</tt> or if failed or no messages are avaialable
 */
javacall_result checkForSystemSignal(MidpReentryData* pNewSignal,
                          MidpEvent* pNewMidpEvent,
                          JVMSPI_BlockedThreadInfo* blocked_threads,
                          int blocked_threads_count,
                          jlong timeout) {

    midp_jc_event_union *event;
    static unsigned char binaryBuffer[BINARY_BUFFER_MAX_LEN];
    javacall_bool res;
    int outEventLen;
    long timeTowaitInMillisec;
     /* convert jlong to long */
    if (timeout > 0x7FFFFFFF) {
        timeTowaitInMillisec = -1;
    } else if (timeout < 0) {//
    	 timeTowaitInMillisec = -1;
    }	else {
        timeTowaitInMillisec = (long)(timeout&0x7FFFFFFF);
    }

    jcapp_refresh_pending(-1/*timeTowaitInMillisec*/);

    res = javacall_event_receive ((long)timeTowaitInMillisec, binaryBuffer, BINARY_BUFFER_MAX_LEN, &outEventLen);

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
        midpHandleMultimediaEvent(event->data.multimediaEvent,
                                  blocked_threads, blocked_threads_count);
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
        pNewSignal->waitingFor = JSR179_LOCATION_SIGNAL;
        pNewSignal->descriptor = (int)event->data.jsr179LocationEvent.provider;
        pNewSignal->status = event->data.jsr179LocationEvent.operation_result;
        REPORT_CALL_TRACE2(LC_NONE, "[jsr179 event] JSR179_LOCATION_SIGNAL %d %d\n", pNewSignal->descriptor, pNewSignal->status);
        break;
#endif
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
            break;

        case PUSH_ALARM_SIGNAL:
            if (findPushTimerBlockedHandle(newSignal.descriptor) != 0) {
                /* The push system is waiting for this alarm */
                midp_thread_signal_list(blocked_threads,
                    blocked_threads_count, PUSH_SIGNAL, 0, 0);
            }

            break;
#ifdef ENABLE_JSR_179
        case JSR179_LOCATION_SIGNAL:
            midp_thread_signal_list(blocked_threads,
                blocked_threads_count, JSR179_LOCATION_SIGNAL, newSignal.descriptor, newSignal.status);
            break;
#endif /* ENABLE_JSR_179 */

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

/**
 * The platform calls this function in slave mode to inform VM of new events.
 */
void midp_slavemode_inform_event(void) {
    int blocked_threads_count;
    JVMSPI_BlockedThreadInfo * blocked_threads;

    int ret = 0;

    while (ret == 0){
        blocked_threads = SNI_GetBlockedThreads(&blocked_threads_count);
        ret = midp_slavemode_handle_events(blocked_threads, blocked_threads_count, 0 /*timeout*/);
    }
}

/**
 * In slave mode executes one JVM time slice
 *
 * @return <tt>-2</tt> if JVM has exited
 *         <tt>-1</tt> if all the Java threads are blocked waiting for events
 *         <tt>timeout value</tt>  the nearest timeout of all blocked Java threads
 */
javacall_int64 javanotify_vm_timeslice(void) {
    midp_slavemode_inform_event();
	return midp_slavemode_time_slice();
}


/**
 * Executes bytecodes for a time slice.  If JVM finished running,
 * then begins finalization.  If JVM continues running, refreshes pending
 * applications.
 */
static jlong midp_slavemode_time_slice(void) {
    jlong to;

    /* execute byteslice */
    to = JVM_TimeSlice();
    if ((jlong)-2 != to){
        to = JVM_TimeSlice();
    }

    if ((jlong)-2 == to) {
        /* Terminate JVM */
        measureStack(KNI_FALSE);
        pushcheckinall();
        javacall_lifecycle_state_changed(JAVACALL_LIFECYCLE_MIDLET_SHUTDOWN,
                                         JAVACALL_OK);
    } else {
        /* Refresh screen */
        jcapp_refresh_pending((javacall_time_milliseconds)to);
    }

    return to;
}


/**
 * Requests that the VM control code schedule a time slice as soon
 * as possible, since Java platform threads are waiting to be run.
 */
void midp_slavemode_schedule_vm_timeslice(void){
    javacall_schedule_vm_timeslice();
}

/**
 * Main processing loop.
 */
void midp_slavemode_dispatch_events(void){
    javacall_slavemode_handle_events();
}
