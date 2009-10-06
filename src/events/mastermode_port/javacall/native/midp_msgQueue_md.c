/*
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

#include <kni.h>
#include <stdlib.h>

#include <midp_logging.h>
#include <midp_mastermode_port.h>

#include <javacall_events.h>
#include <midp_jc_event_defs.h>
#include <midpUtilKni.h>

#if !ENABLE_CDC
#include <suspend_resume.h>
#endif

#ifdef ENABLE_JSR_75
extern void notifyDisksChanged();
#endif

#ifdef ENABLE_JSR_177
/* define needed signal constants from carddevice.h */ 
#include <carddevice.h>
#endif /* ENABLE_JSR_177 */

#ifdef ENABLE_JSR_234
#include <javanotify_multimedia_advanced.h>
#endif /*ENABLE_JSR_234*/

#ifdef ENABLE_JSR_290
#include <jsr290_midp_interface.h>
#include <javautil_unicode.h>
#include <javacall_memory.h>
#endif /* ENABLE_JSR_290 */

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
    
#if !ENABLE_CDC
    res = javacall_event_receive((long)timeout, binaryBuffer,
                                 BINARY_BUFFER_MAX_LEN, &outEventLen);
#else
    res = javacall_event_receive_cvm(MIDP_EVENT_QUEUE_ID, binaryBuffer,
                                 BINARY_BUFFER_MAX_LEN, &outEventLen);
#endif

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
#if !ENABLE_CDC
     case MIDP_JC_EVENT_PAUSE: 
        /*
         * IMPL_NOTE: if VM is running, the following call will send
         * PAUSE_ALL_EVENT message to AMS; otherwise, the resources
         * will be suspended in the context of the caller.
         */
        midp_suspend();
        break;
#endif	
    case MIDP_JC_EVENT_PUSH:
        pNewSignal->waitingFor = PUSH_ALARM_SIGNAL;
        pNewSignal->descriptor = event->data.pushEvent.alarmHandle;
        break;
    case MIDP_JC_EVENT_ROTATION:
        pNewSignal->waitingFor = UI_SIGNAL;
        pNewMidpEvent->type    = ROTATION_EVENT;
        break;
    case MIDP_JC_EVENT_CHANGE_LOCALE:
        pNewSignal->waitingFor = UI_SIGNAL;
        pNewMidpEvent->type    = CHANGE_LOCALE_EVENT;
        break;
    case MIDP_JC_EVENT_VIRTUAL_KEYBOARD:
        pNewSignal->waitingFor = UI_SIGNAL;
        pNewMidpEvent->type    = VIRTUAL_KEYBOARD_EVENT;
        break;

    case MIDP_JC_EVENT_DISPLAY_DEVICE_STATE_CHANGED:
        pNewSignal->waitingFor = DISPLAY_DEVICE_SIGNAL;
        pNewMidpEvent->type    = DISPLAY_DEVICE_STATE_CHANGED_EVENT;
        pNewMidpEvent->intParam1 = event->data.displayDeviceEvent.hardwareId;
        pNewMidpEvent->intParam2 = event->data.displayDeviceEvent.state;
        break;

	case MIDP_JC_EVENT_CLAMSHELL_STATE_CHANGED:
        pNewSignal->waitingFor = UI_SIGNAL;
        pNewMidpEvent->type    = DISPLAY_CLAMSHELL_STATE_CHANGED_EVENT;
        pNewMidpEvent->intParam1 = event->data.clamshellEvent.state;
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
        pNewSignal->waitingFor = MEDIA_EVENT_SIGNAL;
        pNewSignal->status = JAVACALL_OK;

        pNewMidpEvent->type         = MMAPI_EVENT;
        pNewMidpEvent->MM_PLAYER_ID = event->data.multimediaEvent.playerId;
        pNewMidpEvent->MM_DATA      = event->data.multimediaEvent.data.num32;
        pNewMidpEvent->MM_ISOLATE   = event->data.multimediaEvent.appId;
        pNewMidpEvent->MM_EVT_TYPE  = event->data.multimediaEvent.mediaType;
        pNewMidpEvent->MM_EVT_STATUS= event->data.multimediaEvent.status;

        /* SYSTEM_VOLUME_CHANGED event must be sent to all players.             */
        /* MM_ISOLATE = -1 causes bradcast by StoreMIDPEventInVmThread() */
        if( JAVACALL_EVENT_MEDIA_SYSTEM_VOLUME_CHANGED == event->data.multimediaEvent.mediaType )
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
        pNewSignal->waitingFor = AMMS_EVENT_SIGNAL;
        pNewSignal->status     = JAVACALL_OK;

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

#ifdef ENABLE_JSR_290
    case JSR290_JC_EVENT_FLUID_INVALIDATE:
        pNewSignal->waitingFor   = JSR290_INVALIDATE_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        break;
    case JSR290_JC_EVENT_FLUID_LISTENER_COMPLETED:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam2 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image) >> 32);
        pNewMidpEvent->intParam1 = JSR290_LISTENER_COMPLETED;
        break;
    case JSR290_JC_EVENT_FLUID_LISTENER_FAILED:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam2 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image) >> 32);
        pNewMidpEvent->intParam1 = JSR290_LISTENER_FAILED;
        pNewMidpEvent->intParam4 = (jint)(event->data.jsr290FluidEvent.failure_type);
        {
            int len = 0;
            if (JAVACALL_OK != javautil_unicode_utf16_ulength(event->data.jsr290FluidEvent.text, &len)) {
                len = 0;
            }
            pcsl_string_convert_from_utf16(event->data.jsr290FluidEvent.text, len,
                                           &pNewMidpEvent->stringParam1);
        }
        javacall_free(event->data.jsr290FluidEvent.text);
        break;
    case JSR290_JC_EVENT_FLUID_LISTENER_PERCENTAGE:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam2 = (int)(((jlong)event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)(((jlong)event->data.jsr290FluidEvent.fluid_image) >> 32);
        pNewMidpEvent->intParam1 = JSR290_LISTENER_PERCENTAGE;
        pNewMidpEvent->intParam4 = *((int*)&event->data.jsr290FluidEvent.percentage);
        break;
    case JSR290_JC_EVENT_FLUID_LISTENER_STARTED:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam2 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image) >> 32);
        pNewMidpEvent->intParam1 = JSR290_LISTENER_STARTED;
        break;
    case JSR290_JC_EVENT_FLUID_IMAGE_SPAWNED:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam2 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image) >> 32);
        pNewMidpEvent->intParam4 = (int)((jlong)(event->data.jsr290FluidEvent.spare));
        pNewMidpEvent->intParam5 = (int)((jlong)(event->data.jsr290FluidEvent.spare) >> 32);
        pNewMidpEvent->intParam1 = JSR290_IMAGE_SPAWNED;
        break;
    case JSR290_JC_EVENT_FLUID_LISTENER_WARNING:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam2 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image) >> 32);
        pNewMidpEvent->intParam1 = JSR290_LISTENER_WARNING;
        {
            int len = 0;
            if (JAVACALL_OK != javautil_unicode_utf16_ulength(event->data.jsr290FluidEvent.text, &len)) {
                len = 0;
            }
            pcsl_string_convert_from_utf16(event->data.jsr290FluidEvent.text, len,
                                           &pNewMidpEvent->stringParam1);
        }
        javacall_free(event->data.jsr290FluidEvent.text);
        break;
    case JSR290_JC_EVENT_FLUID_LISTENER_DOCUMENT_AVAILABLE:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam2 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image) >> 32);
        pNewMidpEvent->intParam1 = JSR290_LISTENER_DOCUMENT_AVAILABLE;
        break;
    case JSR290_JC_EVENT_FLUID_REQUEST_RESOURCE:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam2 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image) >> 32);
        pNewMidpEvent->intParam4 = (int)((jlong)(event->data.jsr290FluidEvent.spare));
        pNewMidpEvent->intParam5 = (int)((jlong)(event->data.jsr290FluidEvent.spare) >> 32);
        pNewMidpEvent->intParam1 = JSR290_REQUEST_RESOURCE;
        {
            int len = 0;
            if (JAVACALL_OK != javautil_unicode_utf16_ulength(event->data.jsr290FluidEvent.text, &len)) {
                len = 0;
            }
            pcsl_string_convert_from_utf16(event->data.jsr290FluidEvent.text, len,
                                           &pNewMidpEvent->stringParam1);
        }
        javacall_free(event->data.jsr290FluidEvent.text);
        break;
    case JSR290_JC_EVENT_FLUID_CANCEL_REQUEST:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam2 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image) >> 32);
        pNewMidpEvent->intParam4 = (int)((jlong)(event->data.jsr290FluidEvent.spare));
        pNewMidpEvent->intParam5 = (int)((jlong)(event->data.jsr290FluidEvent.spare) >> 32);
        pNewMidpEvent->intParam1 = JSR290_CANCEL_REQUEST;
        break;
    case JSR290_JC_EVENT_FLUID_FILTER_XML_HTTP_REQUEST:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam2 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image) >> 32);
        pNewMidpEvent->intParam1 = JSR290_FILTER_XML_HTTP;
        pNewMidpEvent->intParam4 = (int)((jlong)(event->data.jsr290FluidEvent.spare));
        pNewMidpEvent->intParam5 = (int)((jlong)(event->data.jsr290FluidEvent.spare) >> 32);
        {
            int len = 0;
            if (JAVACALL_OK != javautil_unicode_utf16_ulength(event->data.jsr290FluidEvent.text, &len)) {
                len = 0;
            }
            pcsl_string_convert_from_utf16(event->data.jsr290FluidEvent.text, len,
                                           &pNewMidpEvent->stringParam1);
        }
        javacall_free(event->data.jsr290FluidEvent.text);
        {
            int len = 0;
            if (JAVACALL_OK != javautil_unicode_utf16_ulength(event->data.jsr290FluidEvent.text1, &len)) {
                len = 0;
            }
            pcsl_string_convert_from_utf16(event->data.jsr290FluidEvent.text1, len,
                                           &pNewMidpEvent->stringParam2);
        }
        javacall_free(event->data.jsr290FluidEvent.text1);
        break;
    case JSR290_JC_EVENT_COMPLETION_NOTIFICATION:
        pNewSignal->waitingFor   = JSR290_INVOCATION_COMPLETION_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290NotificationEvent.invocation_id;
        break;
    case JSR290_JC_EVENT_HANDLE_EVENT:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam4 = (int)((jlong)(event->data.jsr290HandleEventRequest.request_handle));
        pNewMidpEvent->intParam5 = (int)((jlong)(event->data.jsr290HandleEventRequest.request_handle) >> 32);
        pNewMidpEvent->intParam1 = JSR290_HANDLE_EVENT;
    	break;
    case JSR290_JC_EVENT_DISPLAY_BOX:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam2 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)((jlong)(event->data.jsr290FluidEvent.fluid_image) >> 32);
        pNewMidpEvent->intParam4 = (int)((jlong)(event->data.jsr290FluidEvent.spare));
        pNewMidpEvent->intParam5 = (int)((jlong)(event->data.jsr290FluidEvent.spare) >> 32);
        switch ((int)event->data.jsr290FluidEvent.result) {
            case 0: pNewMidpEvent->intParam1 = JSR290_DISPLAY_ALERT_BOX;
                break;
            case 1: pNewMidpEvent->intParam1 = JSR290_DISPLAY_CONFIRM_BOX;
                break;
            case 2: pNewMidpEvent->intParam1 = JSR290_DISPLAY_PROMPT_BOX;
                break;
        }
        {
            int len = 0;
            if (JAVACALL_OK != javautil_unicode_utf16_ulength(event->data.jsr290FluidEvent.text, &len)) {
                len = 0;
            }
            pcsl_string_convert_from_utf16(event->data.jsr290FluidEvent.text, len,
                                           &pNewMidpEvent->stringParam1);
        }
        javacall_free(event->data.jsr290FluidEvent.text);

        {
            int len = 0;
            if (JAVACALL_OK != javautil_unicode_utf16_ulength(event->data.jsr290FluidEvent.text1, &len)) {
                len = 0;
            }
            pcsl_string_convert_from_utf16(event->data.jsr290FluidEvent.text1, len,
                                           &pNewMidpEvent->stringParam2);
        }
        javacall_free(event->data.jsr290FluidEvent.text1);
    	break;
    case JSR290_JC_EVENT_FLUID_LAYOUT_CHANGED:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam1 = JSR290_LAYOUT_CHANGED;
        pNewMidpEvent->intParam2 = (int)(((jlong)event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)(((jlong)event->data.jsr290FluidEvent.fluid_image) >> 32);
        break;
    case JSR290_JC_EVENT_FLUID_FOCUS_CHANGED:
        pNewSignal->waitingFor   = JSR290_FLUID_EVENT_SIGNAL;
        pNewSignal->descriptor   = (int)event->data.jsr290FluidEvent.fluid_image;
        pNewSignal->pResult      = event->data.jsr290FluidEvent.app_id;
        pNewMidpEvent->type      = FLUID_EVENT;
        pNewMidpEvent->intParam1 = JSR290_FOCUS_CHANGED;
        pNewMidpEvent->intParam2 = (int)(((jlong)event->data.jsr290FluidEvent.fluid_image));
        pNewMidpEvent->intParam3 = (int)(((jlong)event->data.jsr290FluidEvent.fluid_image) >> 32);
        break;
#endif /* ENABLE_JSR_290 */

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

#ifdef ENABLE_JSR_257
    case JSR257_JC_EVENT_CONTACTLESS:
        if(event->data.jsr257Event.eventType < JSR257_EVENTS_NUM) {
            pNewSignal->waitingFor = JSR257_CONTACTLESS_SIGNAL;
            pNewSignal->descriptor = event->data.jsr257Event.eventType;
        } else {
            REPORT_ERROR1(LC_CORE,"Invalid contactless event type: %d\n", 
                event->data.jsr257Event.eventType);
        }
        break;
         
    case JSR257_JC_MIDP_EVENT:
      printf("\n DEBUG: midp_msgQueue_md.c(): data.jsr257Event.eventType = %d\n",
        event->data.jsr257Event.eventType);
       if(event->data.jsr257Event.eventType < JSR257_MIDP_EVENTS_NUM) {
            pNewSignal->waitingFor   = JSR257_EVENT_SIGNAL;
            pNewSignal->descriptor   = event->data.jsr257Event.eventType;
            pNewMidpEvent->type      = CONTACTLESS_EVENT;
            pNewMidpEvent->intParam1 = event->data.jsr257Event.eventType;
            pNewMidpEvent->JSR257_ISOLATE = (int)(event->data.jsr257Event.isolateId);
            pNewMidpEvent->intParam3 = (int)(event->data.jsr257Event.eventData[0]);
            pNewMidpEvent->intParam4 = (int)(event->data.jsr257Event.eventData[1]);
            pNewMidpEvent->intParam5 = (int)(event->data.jsr257Event.eventData[2]);
            
       } else {
            REPORT_ERROR1(LC_CORE,"Invalid contactless MIDP event type: %d\n", 
                event->data.jsr257Event.eventType);
       }
       break;
       
    case JSR257_JC_PUSH_NDEF_RECORD_DISCOVERED:
        pNewSignal->waitingFor   = JSR257_PUSH_SIGNAL;
        pNewSignal->descriptor = event->data.jsr257Event.eventData[0];
       break;

#endif /* ENABLE_JSR_257 */

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
#ifdef ENABLE_API_EXTENSIONS
case MIDP_JC_EVENT_VOLUME:
		pNewSignal->waitingFor = VOLUME_SIGNAL;
		pNewSignal->status     = JAVACALL_OK;
	
	break;
#endif /* ENABLE_API_EXTENSIONS */
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
