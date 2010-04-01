/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

#include <midpEventUtil.h>
#include <push_server_export.h>
#include <midp_thread.h>
#include <midp_run_vm.h>

#include <midp_logging.h>
#include <midp_slavemode_port.h>

#include <javacall_lifecycle.h>
#include <javautil_string.h>
#include <midp_jc_event_defs.h>

#include <midpServices.h>
#include <midpEvents.h>
#include <midpCompoundEvents.h>
#include <midpAMS.h>  /* for midpFinalize() */

#include <javacall_socket.h>
#include <pcsl_network.h>

#include <suspend_resume.h>

#include <javacall_lifecycle.h>
#include <jcapp_export.h>

#ifdef ENABLE_JSR_120
#include <wmaInterface.h>
#endif

#if ENABLE_JSR_135
#include <javanotify_multimedia.h>
#include <KNICommon.h>
#endif /* ENABLE_JSR_135 */

#ifdef ENABLE_JSR_234
#include <javanotify_multimedia_advanced.h>
#endif /*ENABLE_JSR_234*/

#ifdef ENABLE_JSR_75
#include <fcNotifyIsolates.h>
#endif

#ifdef ENABLE_JSR_177
#include <carddevice.h>
#endif

extern void measureStack(int clearStack);
extern jlong midp_slavemode_time_slice(void);

static jlong midpTimeSlice(void);

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
            REPORT_INFO(LC_CORE, "eventUnblockJavaThread without signal!\n");
            midp_thread_unblock(blocked_threads[i].thread_id);
            return 1;
        }
    }

    return 0;
}

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
    } else if (timeout < 0) {
    	 timeTowaitInMillisec = -1;
    }	else {
        timeTowaitInMillisec = (long)(timeout&0x7FFFFFFF);
    }

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
    case MIDP_JC_EVENT_NETWORK:
        pNewSignal->waitingFor = NETWORK_STATUS_SIGNAL;
        pNewSignal->descriptor = 0;
        pNewSignal->status = (int)event->data.networkEvent.netType;
        break;
    case MIDP_JC_EVENT_END:
        pNewSignal->waitingFor = AMS_SIGNAL;
        pNewMidpEvent->type    = SHUTDOWN_EVENT;
        break;
     case MIDP_JC_EVENT_PAUSE:
        /*
         * IMPL_NOTE: if VM is running, the following call will send
         * PAUSE_ALL_EVENT message to AMS; otherwise, the resources
         * will be suspended in the context of the caller.
         */
        midp_suspend();
        break;
    case MIDP_JC_EVENT_RESUME:
        midp_resume();
        break;
    case MIDP_JC_EVENT_PUSH:
        pNewSignal->waitingFor = PUSH_ALARM_SIGNAL;
        pNewSignal->descriptor = event->data.pushEvent.alarmHandle;
        break;
    case MIDP_JC_EVENT_ROTATION:
        pNewSignal->waitingFor = UI_SIGNAL;
        pNewMidpEvent->type    = ROTATION_EVENT;
        break;
    case MIDP_JC_EVENT_DISPLAY_DEVICE_STATE_CHANGED:
        pNewSignal->waitingFor = DISPLAY_DEVICE_SIGNAL;
        pNewMidpEvent->type    = DISPLAY_DEVICE_STATE_CHANGED_EVENT;
        pNewMidpEvent->intParam1 = event->data.displayDeviceEvent.hardwareId;
        pNewMidpEvent->intParam2 = event->data.displayDeviceEvent.state;
        break;
#if ENABLE_ON_DEVICE_DEBUG
    case MIDP_JC_ENABLE_ODD_EVENT:
        pNewSignal->waitingFor = AMS_SIGNAL;
        pNewMidpEvent->type = MIDP_ENABLE_ODD_EVENT;
        break;
#endif
	case MIDP_JC_EVENT_CLAMSHELL_STATE_CHANGED:
        pNewSignal->waitingFor = DISPLAY_DEVICE_SIGNAL;
        pNewMidpEvent->type    = DISPLAY_CLAMSHELL_STATE_CHANGED_EVENT;
        pNewMidpEvent->intParam1 = event->data.clamshellEvent.state;
        break;
    case MIDP_JC_EVENT_CHANGE_LOCALE:
        pNewSignal->waitingFor = UI_SIGNAL;
        pNewMidpEvent->type    = CHANGE_LOCALE_EVENT;
        break;
    case MIDP_JC_EVENT_VIRTUAL_KEYBOARD:
        pNewSignal->waitingFor = UI_SIGNAL;
        pNewMidpEvent->type    = VIRTUAL_KEYBOARD_EVENT;
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
        pNewSignal->waitingFor = MEDIA_EVENT_SIGNAL;
        pNewSignal->status     = event->data.multimediaEvent.status;
        pNewSignal->pResult    = (void *)event->data.multimediaEvent.data.num32;
        
        /* Create Java driven event */
        if (event->data.multimediaEvent.mediaType > 0 &&
                event->data.multimediaEvent.mediaType <
                        JAVACALL_EVENT_MEDIA_JAVA_EVENTS_MARKER) {
            pNewMidpEvent->type         = MMAPI_EVENT;
            pNewMidpEvent->MM_PLAYER_ID = event->data.multimediaEvent.playerId;
            pNewMidpEvent->MM_DATA      = event->data.multimediaEvent.data.num32;
            pNewMidpEvent->MM_ISOLATE   = event->data.multimediaEvent.appId;
            pNewMidpEvent->MM_EVT_TYPE  = event->data.multimediaEvent.mediaType;
            pNewMidpEvent->MM_EVT_STATUS= event->data.multimediaEvent.status;
    
            /* SYSTEM_VOLUME_CHANGED event must be sent to all players.             */
            /* MM_ISOLATE = -1 causes broadcast by StoreMIDPEventInVmThread() */
            if(JAVACALL_EVENT_MEDIA_SYSTEM_VOLUME_CHANGED == 
                    event->data.multimediaEvent.mediaType) {
                pNewMidpEvent->MM_ISOLATE = -1;
            }
        }
        
        /* This event should simply unblock waiting thread. No Java event is needed */
        if (event->data.multimediaEvent.mediaType > 
                                    JAVACALL_EVENT_MEDIA_JAVA_EVENTS_MARKER) {
            pNewSignal->descriptor =
                MAKE_PLAYER_DESCRIPTOR(event->data.multimediaEvent.appId,
                                       event->data.multimediaEvent.playerId,
                                       event->data.multimediaEvent.mediaType);
                
        /* In case of error or end-of-media unblock all awaiting threads */
        } else if (event->data.multimediaEvent.mediaType == 
                            JAVACALL_EVENT_MEDIA_END_OF_MEDIA ||
                   event->data.multimediaEvent.mediaType == 
                            JAVACALL_EVENT_MEDIA_RECORD_ERROR ||
                   event->data.multimediaEvent.mediaType == 
                            JAVACALL_EVENT_MEDIA_ERROR) {
            pNewSignal->descriptor =
                (MAKE_PLAYER_DESCRIPTOR(event->data.multimediaEvent.appId,
                                       event->data.multimediaEvent.playerId,
                                       0) & PLAYER_DESCRIPTOR_EVENT_MASK);
        } else {
            pNewSignal->descriptor = 0;
        }
        REPORT_CALL_TRACE4(LC_NONE, "[media event] External event received "
                "%d %d %d %d\n",
                pNewMidpEvent->type,
                event->data.multimediaEvent.appId,
                event->data.multimediaEvent.playerId,
                event->data.multimediaEvent.data.num32);
#endif
        break;
#ifdef ENABLE_JSR_234
    case MIDP_JC_EVENT_ADVANCED_MULTIMEDIA:
        pNewSignal->waitingFor = AMMS_EVENT_SIGNAL;
        pNewSignal->status     = JAVACALL_OK;
        pNewSignal->pResult    = (void *)event->data.multimediaEvent.data.num32;

        if (event->data.multimediaEvent.mediaType > 0 &&
                event->data.multimediaEvent.mediaType <
                        JAVACALL_EVENT_AMMS_JAVA_EVENTS_MARKER) {
            pNewMidpEvent->type         = AMMS_EVENT;
            pNewMidpEvent->MM_PLAYER_ID = event->data.multimediaEvent.playerId;
            pNewMidpEvent->MM_ISOLATE   = event->data.multimediaEvent.appId;
            pNewMidpEvent->MM_EVT_TYPE  = event->data.multimediaEvent.mediaType;

            switch( event->data.multimediaEvent.mediaType )
            {
            case JAVACALL_EVENT_AMMS_SNAP_SHOOTING_STOPPED:
            case JAVACALL_EVENT_AMMS_SNAP_STORAGE_ERROR:
                {
                    int len = 0;
                    javacall_utf16_string str = event->data.multimediaEvent.data.str16;
                    while( str[len] != 0 ) len++;
                    pcsl_string_convert_from_utf16( str, len, &pNewMidpEvent->MM_STRING );
                    free( str );
                }
                break;
            default:
                pNewMidpEvent->MM_DATA = event->data.multimediaEvent.data.num32;
                break;
            }
        }

        /* This event should simply unblock waiting thread. No Java event is needed */
        if (event->data.multimediaEvent.mediaType > 
                                    JAVACALL_EVENT_AMMS_JAVA_EVENTS_MARKER) {
            pNewSignal->descriptor =
                MAKE_PLAYER_AMMS_DESCRIPTOR(event->data.multimediaEvent.appId,
                                       event->data.multimediaEvent.playerId,
                                       event->data.multimediaEvent.mediaType);
                
        } else {
            pNewSignal->descriptor = 0;
        }
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
#endif /*ENABLE_JSR_179*/

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
    case JSR211_JC_EVENT_REQUEST_RECEIVED:
        pNewSignal->waitingFor = JSR211_REQUEST_SIGNAL;
        pNewSignal->pResult    = event->data.jsr211RequestEvent.data;
        pNewMidpEvent->type    = CHAPI_EVENT;
        break;
    case JSR211_JC_EVENT_RESPONSE_RECEIVED:
        pNewSignal->waitingFor = JSR211_RESPONSE_SIGNAL;
        pNewSignal->pResult    = event->data.jsr211ResponseEvent.data;
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
#if !ENABLE_CDC
#ifdef ENABLE_JSR_256
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
#endif /* !ENABLE_CDC */
    default:
        REPORT_ERROR1(LC_CORE,"checkForSystemSignal(): Unknown event %d.\n", event->eventType);
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
    static MidpEvent newCompMidpEvent;

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
#endif /* ENABLE_JAVA_DEBUGGER */

        case AMS_SIGNAL:
            midpStoreEventAndSignalAms(newMidpEvent);
            break;

        case UI_SIGNAL:
            if (newMidpEvent.type == CHANGE_LOCALE_EVENT) {
                StoreMIDPEventInVmThread(newMidpEvent, -1);
            } else {
                midpStoreEventAndSignalForeground(newMidpEvent);
                /*
                 * IMPL_NOTE: Currently compound events are implemented only for
                 * UI_SIGNALs and only UI_SIGNALs are needed to identify UI
                 * compound event, so trying to recognize compound event only
                 * in this particular place.
                 */
                MIDP_EVENT_INITIALIZE(newCompMidpEvent);
                if (midpCheckCompoundEvent(&newMidpEvent, &newCompMidpEvent)) {
                    midpStoreEventAndSignalForeground(newCompMidpEvent);
                }
            }
            break;

        case DISPLAY_DEVICE_SIGNAL:
	  // broadcast event, send it to all isolates to all displays
	    StoreMIDPEventInVmThread(newMidpEvent, -1);
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
#if ENABLE_JSR_135
    case MEDIA_EVENT_SIGNAL:
        if (newMidpEvent.type == MMAPI_EVENT) {
            StoreMIDPEventInVmThread(newMidpEvent, newMidpEvent.MM_ISOLATE);
        }
        if (newSignal.descriptor != 0) {
            int i;

            for (i = 0; i < blocked_threads_count; i++) {
                MidpReentryData *pThreadReentryData =
                    (MidpReentryData*)(blocked_threads[i].reentry_data);

                if (pThreadReentryData != NULL &&
                    pThreadReentryData->waitingFor == newSignal.waitingFor &&
                    (pThreadReentryData->descriptor == newSignal.descriptor ||
                        (pThreadReentryData->descriptor & 
                            PLAYER_DESCRIPTOR_EVENT_MASK) == 
                                                      newSignal.descriptor)) {
                    pThreadReentryData->status = newSignal.status;
                    pThreadReentryData->pResult = newSignal.pResult;
                    midp_thread_unblock(blocked_threads[i].thread_id);
                }
            }
        }
        break;
#endif
#if ENABLE_JSR_234
    case AMMS_EVENT_SIGNAL:
        if (newMidpEvent.type == AMMS_EVENT) {
            StoreMIDPEventInVmThread(newMidpEvent, newMidpEvent.MM_ISOLATE);
        }
        if (newSignal.descriptor != 0) {
            int i;

            for (i = 0; i < blocked_threads_count; i++) {
                MidpReentryData *pThreadReentryData =
                    (MidpReentryData*)(blocked_threads[i].reentry_data);

                if (pThreadReentryData != NULL &&
                    pThreadReentryData->waitingFor == newSignal.waitingFor &&
                    (pThreadReentryData->descriptor == newSignal.descriptor ||
                        (pThreadReentryData->descriptor & 
                            PLAYER_AMMS_DESCRIPTOR_EVENT_MASK) == 
                                                      newSignal.descriptor)) {
                    pThreadReentryData->status = newSignal.status;
                    pThreadReentryData->pResult = newSignal.pResult;
                    midp_thread_unblock(blocked_threads[i].thread_id);
                }
            }
        }
        break;
#endif
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
#if !ENABLE_CDC
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
#endif /* !ENABLE_CDC */
        case NETWORK_STATUS_SIGNAL:
            if (MIDP_NETWORK_UP == newSignal.status) {
                midp_thread_signal_list(blocked_threads,
                                        blocked_threads_count, NETWORK_STATUS_SIGNAL,
                                        newSignal.descriptor, PCSL_NET_SUCCESS);
            } else if (MIDP_NETWORK_DOWN == newSignal.status) {
                midp_thread_signal_list(blocked_threads,
                                        blocked_threads_count, NETWORK_STATUS_SIGNAL,
                                        newSignal.descriptor, PCSL_NET_IOERROR);
                midp_thread_signal_list(blocked_threads,
                                        blocked_threads_count, NETWORK_READ_SIGNAL, 0,
                                        PCSL_NET_IOERROR);
                midp_thread_signal_list(blocked_threads,
                                        blocked_threads_count, NETWORK_WRITE_SIGNAL, 0,
                                        PCSL_NET_IOERROR);
                midp_thread_signal_list(blocked_threads,
                                        blocked_threads_count, NETWORK_EXCEPTION_SIGNAL, 0,
                                        PCSL_NET_IOERROR);
                midp_thread_signal_list(blocked_threads,
                                        blocked_threads_count, HOST_NAME_LOOKUP_SIGNAL, 0,
                                        PCSL_NET_IOERROR);
 
                REPORT_INFO(LC_PROTOCOL, "midp_slavemode_handle_events:"
                            "network down\n");
            } else {
                REPORT_ERROR1(LC_PROTOCOL, "midp_slavemode_handle_events:"
                              "NETWORK_STATUS_SIGNAL Unknown net. status = %d\n",
                              newSignal.status);
            }
            break;

        default:
            break;
        } /* switch */
        ret = 0;
    }

    return ret;
}

/**
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
    } else {
        /* convert jlong to long */
        if (to > 0x7FFFFFFF) {
            toInMillisec = -1;
        } else if (to < 0) {
            toInMillisec = -1;
        }   else {
            toInMillisec = (javacall_time_milliseconds)(to&0x7FFFFFFF);
        }
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


/** Runs the platform-specific event loop. */
void midp_slavemode_event_loop(void) {
    REPORT_INFO(LC_CORE, 
        "midp_slavemode_event_loop() is not "
        "implemented for JavaCall platform\n");
    return;
}

/**
 * This function is called when the network initialization
 * or finalization is completed.
 *
 * @param isInit 0 if the network finalization has been finished,
 *               not 0 - if the initialization
 * @param status one of PCSL_NET_* completion codes
 */
void midp_network_status_event_port(int isInit, int status) {
    jcapp_network_event_received(isInit, status);
}
