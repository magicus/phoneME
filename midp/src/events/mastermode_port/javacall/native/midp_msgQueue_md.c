/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#include <midp_logging.h>
#include <midp_mastermode_port.h>

#include <javacall_events.h>
#include <midp_jc_event_defs.h>

#ifdef ENABLE_JSR_75
extern void notifyDisksChanged();
#endif

#ifdef ENABLE_JSR_177
/* define needed signal constants from carddevice.h */ 
#include <carddevice.h>
#endif /* ENABLE_JSR_177 */

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
void checkForSystemSignal(MidpReentryData* pNewSignal,
                          MidpEvent* pNewMidpEvent,
                          jlong timeout) {

    midp_jc_event_union *event;
    static unsigned char binaryBuffer[BINARY_BUFFER_MAX_LEN];
    javacall_bool res;
    int outEventLen;
    
    res = javacall_event_receive ((long)timeout, binaryBuffer, BINARY_BUFFER_MAX_LEN, &outEventLen);

    if (!JAVACALL_SUCCEEDED(res)) {
        return;
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

#if (ENABLE_JSR_120 || ENABLE_JSR_205)
    case MIDP_JC_EVENT_SMS_INCOMING:
        pNewSignal->waitingFor = WMA_SMS_READ_SIGNAL;
        pNewSignal->descriptor = event->data.smsIncomingEvent.stub;
        break;
    case MIDP_JC_EVENT_MMS_INCOMING:
        pNewSignal->waitingFor = WMA_MMS_READ_SIGNAL;
        pNewSignal->descriptor = event->data.mmsIncomingEvent.stub;
        break;
    case MIDP_JC_EVENT_CBS_INCOMING:
        pNewSignal->waitingFor = WMA_CBS_READ_SIGNAL;
        pNewSignal->descriptor = event->data.cbsIncomingEvent.stub;
        break;
    case MIDP_JC_EVENT_SMS_SENDING_RESULT:
        pNewSignal->waitingFor = WMA_SMS_WRITE_SIGNAL;
        break;
    case MIDP_JC_EVENT_MMS_SENDING_RESULT:
        pNewSignal->waitingFor = WMA_MMS_WRITE_SIGNAL;
        break;
#endif

    case MIDP_JC_EVENT_MULTIMEDIA:
#if ENABLE_MMAPI
        pNewSignal->waitingFor = MEDIA_EVENT_SIGNAL;
        pNewSignal->status = JAVACALL_OK;
        pNewMidpEvent->MM_PLAYER_ID = event->data.multimediaEvent.playerId;
        pNewMidpEvent->MM_DATA = event->data.multimediaEvent.data;
        pNewMidpEvent->ISOLATE = event->data.multimediaEvent.isolateId;

        if (-1 == event->data.multimediaEvent.isolateId) {
            event->data.multimediaEvent.isolateId = 0;
        }
        switch(event->data.multimediaEvent.mediaType) {
            case JAVACALL_EVENT_MEDIA_END_OF_MEDIA:
                pNewMidpEvent->type = MM_EOM_EVENT;
                break;

            case JAVACALL_EVENT_MEDIA_RECORD_SIZE_LIMIT:
                pNewMidpEvent->type = MM_RECORD_LIMIT;
                break;

            case JAVACALL_EVENT_MEDIA_RECORD_ERROR:
                pNewMidpEvent->type = MM_RECORD_ERROR;
                break;

            case JAVACALL_EVENT_MEDIA_BUFFERING_STARTED:
                pNewMidpEvent->type = MM_BUFFERING_START;
                break;

            case JAVACALL_EVENT_MEDIA_BUFFERING_STOPPED:
                pNewMidpEvent->type = MM_BUFFERING_STOP;
                break;

            case JAVACALL_EVENT_MEDIA_DURATION_UPDATED:
                pNewMidpEvent->type = MM_DURATION_EVENT;
                break;
    
            case JAVACALL_EVENT_MEDIA_VOLUME_CHANGED:
                pNewMidpEvent->type = MM_VOLUME_CHANGED;
                /* Set to current isolate ID and special player ID to 
                   send this event to all of players in this isolate */
                pNewMidpEvent->MM_PLAYER_ID = -2; //playerId
                pNewMidpEvent->ISOLATE = -1; //isolateId
                break;

            case JAVACALL_EVENT_MEDIA_ERROR:
                pNewMidpEvent->type = MM_GENERAL_ERROR;
                break;

                /* Unblock blocked Java thread by calling snapshot method */
            case JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED:
                /* Compose 16 bit of isolate ID and 16 bit of player ID 
                   to generate descriptor */
                pNewSignal->waitingFor = MEDIA_SNAPSHOT_SIGNAL;
                pNewSignal->descriptor = 
                    (((event->data.multimediaEvent.isolateId & 0xFFFF) << 16) 
                     | (event->data.multimediaEvent.playerId & 0xFFFF));
                pNewSignal->status = JAVACALL_OK;
                REPORT_CALL_TRACE1(LC_NONE, "[media event] JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED %d\n", pNewSignal->descriptor);
                break;
        }

        REPORT_CALL_TRACE4(LC_NONE, "[media event] External event recevied %d %d %d %d\n",
                pNewMidpEvent->type, 
                event->data.multimediaEvent.isolateId, 
                pNewMidpEvent->MM_PLAYER_ID, 
                pNewMidpEvent->MM_DATA);
#endif
        break;
#ifdef ENABLE_JSR_179
    case JSR179_LOCATION_JC_EVENT:
        pNewSignal->waitingFor = JSR179_LOCATION_SIGNAL;
        pNewSignal->descriptor = event->data.jsr179LocationEvent.provider;
        pNewSignal->status = event->data.jsr179LocationEvent.operation_result;
        REPORT_CALL_TRACE1(LC_NONE, "[jsr179 event] JSR179_LOCATION_SIGNAL %d %d\n", pNewSignal->descriptor, pNewSignal->status);
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
        break;
#endif /* ENABLE_MULTIPLE_ISOLATES */
    default:
        REPORT_ERROR(LC_CORE,"Unknown event.\n");
        break;
    };

    REPORT_CALL_TRACE(LC_HIGHUI, "LF:STUB:checkForSystemSignal()\n");
}

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
