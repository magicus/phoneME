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

/**
 * @file
 *
 * Implementation of javacall notification functions related to
 * hardware or service events.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <midpServices.h>
#include <midp_logging.h>
#include <localeMethod.h>
#include <midp_jc_event_defs.h>

#include <javacall_datagram.h>
#include <javacall_events.h>
#include <javacall_input.h>

#include <javacall_keypress.h>
#include <javacall_network.h>
#include <javacall_penevent.h>
#include <javacall_security.h>
#include <javacall_socket.h>
#include <javacall_time.h>
#include <javautil_unicode.h>
#include <javautil_string.h>
#include <javacall_memory.h>
#include <javacall_lcd.h>

#ifdef ENABLE_JSR_120
#include <javacall_sms.h>
#include <javacall_cbs.h>
#endif

#ifdef ENABLE_JSR_205
#include <javacall_mms.h>
#endif

#ifdef ENABLE_JSR_177
#include <javacall_carddevice.h>
#endif

#ifdef USE_VSCL
#include <javacall_vscl.h>
#endif

#ifdef ENABLE_JSR_179
#include <javanotify_location.h>
#endif

#ifdef ENABLE_JSR_234
#include <javanotify_multimedia_advanced.h>
#endif

#ifdef ENABLE_JSR_257
#include <javacall_contactless.h>
#endif

#ifdef ENABLE_JSR_290
#include <javacall_fluid_image.h>
#endif

#ifdef ENABLE_ON_DEVICE_DEBUG
#include <javacall_odd.h>
#endif /* ENABLE_ON_DEVICE_DEBUG */

#define MAX_PHONE_NUMBER_LENGTH 48

static char selectedNumber[MAX_PHONE_NUMBER_LENGTH];

/**
 * The notification function to be called by platform for keypress
 * occurences.
 * The platform will invoke the call back in platform context for
 * each key press, key release and key repeat occurence
 * @param key the key that was pressed
 * @param type <tt>JAVACALL_KEYPRESSED</tt> when key was pressed
 *             <tt>JAVACALL_KEYRELEASED</tt> when key was released
 *             <tt>JAVACALL_KEYREPEATED</tt> when to be called repeatedly
 *             by platform during the duration that the key was held
 */
void javanotify_key_event(javacall_key key, javacall_keypress_type type) {
    midp_jc_event_union e;

    REPORT_INFO2(LC_CORE,"javanotify_key_event() >> key=%d , type=%d\n",key,type);

    e.eventType = MIDP_JC_EVENT_KEY;
    e.data.keyEvent.key = key;
    e.data.keyEvent.keyEventType = type;

    midp_jc_event_send(&e);
}

/**
* The notification function to be called by platform for pen
* press/release/drag occurences.
* The platform will invoke the call back in platform context for
* each pen press, pen release and pen dragg occurence
* @param x the x positoin when the pen was pressed/released/dragged
* @param y the y positoin when the pen was pressed/released/dragged
* @param type <tt>JAVACALL_PENPRESSED</tt> when pen was pressed
*             <tt>JAVACALL_PENRELEASED</tt> when pen was released
*             <tt>JAVACALL_PENDRAGGED</tt> when pen was dragged
*/
void javanotify_pen_event(int x, int y, javacall_penevent_type type) {
    midp_jc_event_union e;

    REPORT_INFO3(LC_CORE,"javanotify_pen_event() >> x=%d, y=%d type=%d\n",x,y,type);

    e.eventType = MIDP_JC_EVENT_PEN;
    e.data.penEvent.type = type;
    e.data.penEvent.x = x;
    e.data.penEvent.y = y;

    midp_jc_event_send(&e);
}

void javanotify_alarm_expiration() {
    midp_jc_event_union e;
    e.eventType  = MIDP_JC_EVENT_PUSH;
    e.data.pushEvent.alarmHandle = 0;
    midp_jc_event_send(&e);
}

/**
 * A callback function to be called for notification of network
 * conenction related events, such as network going down or up.
 * The platform will invoke the call back in platform context.
 *
 * @param isInit 0 if the network finalization has been finished,
 *               not 0 - if the initialization
 * @param status one of PCSL_NET_* completion codes
 */
void jcapp_network_event_received(int isInit, int status) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "jc_network_event() >>\n");

    (void)status;
    
    e.eventType = MIDP_JC_EVENT_NETWORK;
    if (isInit) {
        e.data.networkEvent.netType = MIDP_NETWORK_UP;
    } else {
        e.data.networkEvent.netType = MIDP_NETWORK_DOWN;
    }

    midp_jc_event_send(&e);
}

#if ENABLE_JSR_120
    #include <jsr120_sms_pool.h>
    #include <jsr120_cbs_pool.h>
    #include <javacall_memory.h>
#endif
#if ENABLE_JSR_205
    #include <jsr205_mms_pool.h>
    #include <string.h>
#endif

#ifdef ENABLE_JSR_120

static SmsMessage* jsr120_sms_new_msg_javacall(jchar  encodingType,
                                        unsigned char  msgAddr[MAX_ADDR_LEN],
                                        jchar  sourcePortNum,
                                        jchar  destPortNum,
                                        jlong  timeStamp,
                                        jchar  msgLen,
                                        unsigned char* msgBuffer) {

    SmsMessage *sms = (SmsMessage*)javacall_malloc(sizeof(SmsMessage));
    memset(sms, 0, sizeof(SmsMessage));

    sms->msgAddr   = (char*)javacall_malloc(MAX_ADDR_LEN);
    memset(sms->msgAddr, 0, MAX_ADDR_LEN);
    sms->msgBuffer = (char*)javacall_malloc(msgLen);
    memset(sms->msgBuffer, 0, msgLen);

    sms->encodingType  = encodingType;
    sms->sourcePortNum = sourcePortNum;
    sms->destPortNum   = destPortNum;
    sms->timeStamp     = timeStamp;
    sms->msgLen        = msgLen;

    if (msgAddr != NULL) {
        memcpy(sms->msgAddr, msgAddr, MAX_ADDR_LEN);
    }

    if (msgBuffer != NULL) {
        memcpy(sms->msgBuffer, msgBuffer, msgLen);
    }


    return sms;
}

/**
 * callback that needs to be called by platform to handover an incoming SMS intended for Java
 *
 * After this function is called, the SMS message should be removed from platform inbox
 *
 * @param msgType JAVACALL_SMS_MSG_TYPE_ASCII, or JAVACALL_SMS_MSG_TYPE_BINARY or JAVACALL_SMS_MSG_TYPE_UNICODE_UCS2  1002
 * @param sourceAddress the source SMS address for the message.  The format of the address  parameter
 *                is  expected to be compliant with MSIDN, for example,. +123456789
 * @param msgBuffer payload of incoming sms
 *        if msgType is JAVACALL_SMS_MSG_TYPE_ASCII then this is a
 *        pointer to char* ASCII string.
 *        if msgType is JAVACALL_SMS_MSG_TYPE_UNICODE_UCS2, then this
 *        is a pointer to javacall_utf16 UCS-2 string.
 *        if msgType is JAVACALL_SMS_MSG_TYPE_BINARY, then this is a
 *        pointer to binary octet buffer.
 * @param msgBufferLen payload len of incoming sms
 * @param sourcePortNum the port number that the message originated from
 * @param destPortNum the port number that the message was sent to
 * @param timeStamp SMS service center timestamp
 */
void javanotify_incoming_sms(javacall_sms_encoding msgType,
                        char *sourceAddress,
                        unsigned char *msgBuffer,
                        int msgBufferLen,
                        unsigned short sourcePortNum,
                        unsigned short destPortNum,
                        javacall_int64 timeStamp) {
    midp_jc_event_union e;
        SmsMessage* sms;

    REPORT_INFO(LC_CORE, "javanotify_incoming_sms() >>\n");
    e.eventType = MIDP_JC_EVENT_SMS_INCOMING;

    sms = jsr120_sms_new_msg_javacall(
             msgType, (unsigned char*)sourceAddress, sourcePortNum, destPortNum, timeStamp, msgBufferLen, msgBuffer);

    e.data.smsIncomingEvent.stub = (int)sms;

    midp_jc_event_send(&e);
    return;
}
#endif

#ifdef ENABLE_JSR_205

static char* javacall_copystring(char* src) {
    int length = strlen(src)+1;
    char* result = javacall_malloc(length);
    memcpy(result, src, length);
    return (char*)result;
}

static MmsMessage* jsr205_mms_new_msg_javacall(char* fromAddress, char* appID,
    char* replyToAppID, int msgLen, unsigned char* msgBuffer) {

    MmsMessage* message = (MmsMessage*)javacall_malloc(sizeof(MmsMessage));
    memset(message, 0, sizeof(MmsMessage));

    message->fromAddress  = javacall_copystring(fromAddress);
    message->appID        = javacall_copystring(appID);
    message->replyToAppID = javacall_copystring(replyToAppID);

    message->msgLen = msgLen;
    if (msgLen > 0) {
        message->msgBuffer = (char*)memcpy((void*)javacall_malloc(msgLen), msgBuffer, msgLen);
    }

    return message;
}

/*
 * See javacall_mms.h for description
 */
void javanotify_incoming_mms(
        char* fromAddress, char* appID, char* replyToAppID,
        int bodyLen, unsigned char* body) {
    midp_jc_event_union e;
    MmsMessage* mms;

    REPORT_INFO(LC_CORE, "javanotify_incoming_mms() >>\n");
    e.eventType = MIDP_JC_EVENT_MMS_INCOMING;

    mms = jsr205_mms_new_msg_javacall(fromAddress, appID, replyToAppID, bodyLen, body);

    e.data.mmsIncomingEvent.stub = (int)mms;

    midp_jc_event_send(&e);
    return;
}

void javanotify_incoming_mms_available(
        char* fromAddress, char* appID, char* replyToAppID,
        javacall_handle handle) {

    midp_jc_event_union e;
    MmsMessage* mms;

    REPORT_INFO(LC_CORE, "javanotify_incoming_mms_available() >>\n");
    e.eventType = MIDP_JC_EVENT_MMS_INCOMING;

    /*bodyLen=-1*/
    mms = jsr205_mms_new_msg_javacall(fromAddress, appID, replyToAppID, -1, (char*)handle);

    e.data.mmsIncomingEvent.stub = (int)mms;

    midp_jc_event_send(&e);
    return;
}

#endif

#ifdef ENABLE_JSR_120

static CbsMessage* jsr120_cbs_new_msg_javacall(jchar encodingType,
                               jchar msgID,
                               jchar msgLen,
                               unsigned char* msgBuffer) {

    CbsMessage* message = (CbsMessage*)javacall_malloc(sizeof(CbsMessage));
    memset(message, 0, sizeof(CbsMessage));

    message->encodingType = encodingType;
    message->msgID        = msgID;

    message->msgLen       = (msgLen > MAX_CBS_MESSAGE_SIZE) ? MAX_CBS_MESSAGE_SIZE : msgLen;
    message->msgBuffer    = (unsigned char*)javacall_malloc(msgLen);
    memcpy(message->msgBuffer, msgBuffer, msgLen);

    return message;
}

/**
 * callback that needs to be called by platform to handover an incoming CBS intended for Java
 *
 * After this function is called, the CBS message should be removed from platform inbox
 *
 * @param msgType JAVACALL_CBS_MSG_TYPE_ASCII, or JAVACALL_CBS_MSG_TYPE_BINARY or JAVACALL_CBS_MSG_TYPE_UNICODE_UCS2
 * @param msgID message ID
 * @param msgBuffer payload of incoming cbs
 *        if msgType is JAVACALL_CBS_MSG_TYPE_ASCII then this is a
 *        pointer to char* ASCII string.
 *        if msgType is JAVACALL_CBS_MSG_TYPE_UNICODE_UCS2, then this
 *        is a pointer to javacall_utf16 UCS-2 string.
 *        if msgType is JAVACALL_CBS_MSG_TYPE_BINARY, then this is a
 *        pointer to binary octet buffer.
 * @param msgBufferLen payload len of incoming cbs
 */
void javanotify_incoming_cbs(
        javacall_cbs_encoding  msgType,
        unsigned short         msgID,
        unsigned char*         msgBuffer,
        int                    msgBufferLen) {
    midp_jc_event_union e;
        CbsMessage* cbs;

    e.eventType = MIDP_JC_EVENT_CBS_INCOMING;

    REPORT_INFO(LC_CORE, "javanotify_incoming_cbs() >>\n");
    cbs = jsr120_cbs_new_msg_javacall(msgType, msgID, msgBufferLen, msgBuffer);

    e.data.cbsIncomingEvent.stub = (int)cbs;

    midp_jc_event_send(&e);
    return;
}
#endif

#ifdef ENABLE_JSR_120
/**
 * A callback function to be called by platform to notify that an SMS
 * has completed sending operation.
 * The platform will invoke the call back in platform context for
 * each sms sending completion.
 *
 * @param result indication of send completed status result: Either
 *         <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> on failure
 * @param handle Handle value returned from javacall_sms_send
 */
void javanotify_sms_send_completed(javacall_result result,
                                   int handle) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_sms_send_completed() >>\n");
    e.eventType = MIDP_JC_EVENT_SMS_SENDING_RESULT;
    e.data.smsSendingResultEvent.handle = (void *) handle;
    e.data.smsSendingResultEvent.result
        = JAVACALL_OK == result ? WMA_OK : WMA_ERR;

    midp_jc_event_send(&e);
    return;
}
#endif

#ifdef ENABLE_JSR_205
/**
 * A callback function to be called by platform to notify that an MMS
 * has completed sending operation.
 * The platform will invoke the call back in platform context for
 * each mms sending completion.
 *
 * @param result indication of send completed status result: Either
 *         <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> on failure
 * @param handle of available MMS
 */
void javanotify_mms_send_completed(javacall_result result,
                                   int handle) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_mms_send_completed() >>\n");
    e.eventType = MIDP_JC_EVENT_MMS_SENDING_RESULT;
    e.data.mmsSendingResultEvent.handle = (void *) handle;
    e.data.mmsSendingResultEvent.result =
        (JAVACALL_OK == result) ? WMA_OK : WMA_ERR;

    midp_jc_event_send(&e);
    return;
}
#endif

#ifdef ENABLE_JSR_177
/**
 *
 */
void javanotify_carddevice_event(javacall_carddevice_event event,
                                 void *context) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_carddevice_event() >>\n");
    e.eventType = MIDP_JC_EVENT_CARDDEVICE;
    switch (event) {
    case JAVACALL_CARDDEVICE_RESET:
        e.data.carddeviceEvent.eventType = MIDP_CARDDEVICE_RESET;
        e.data.carddeviceEvent.handle = (int)context;
        break;
    case JAVACALL_CARDDEVICE_XFER:
        e.data.carddeviceEvent.eventType = MIDP_CARDDEVICE_XFER;
        e.data.carddeviceEvent.handle = (int)context;
        break;
    case JAVACALL_CARDDEVICE_UNLOCK:
        e.data.carddeviceEvent.eventType = MIDP_CARDDEVICE_UNLOCK;
        e.data.carddeviceEvent.handle = 0;
        break;
    default:
        /* TODO: report error */
        return;
    }
    midp_jc_event_send(&e);
    return;
}
#endif /* ENABLE_JSR_177 */

/**
 * A callback function to be called for notification of non-blocking
 * client/server socket related events, such as a socket completing opening or ,
 * closing socket remotely, disconnected by peer or data arrived on
 * the socket indication.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence.
 *
 * @param type type of indication: Either
 *          - JAVACALL_EVENT_SOCKET_OPEN_COMPLETED
 *          - JAVACALL_EVENT_SOCKET_CLOSE_COMPLETED
 *          - JAVACALL_EVENT_SOCKET_RECEIVE
 *          - JAVACALL_EVENT_SOCKET_SEND
 *          - JAVACALL_EVENT_SOCKET_REMOTE_DISCONNECTED
 *          - JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED
 * @param socket_handle handle of socket related to the notification
 * @param operation_result <tt>JAVACALL_OK</tt> if operation
 *        completed successfully,
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void javanotify_socket_event(javacall_socket_callback_type type,
                        javacall_handle socket_handle,
                        javacall_result operation_result) {

    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_socket_event() >>\n");

    e.eventType = MIDP_JC_EVENT_SOCKET;
    e.data.socketEvent.handle = socket_handle;
    e.data.socketEvent.status = operation_result;

    e.data.socketEvent.extraData = NULL;

    switch (type) {
        case JAVACALL_EVENT_SOCKET_OPEN_COMPLETED:
        case JAVACALL_EVENT_SOCKET_SEND:
            e.data.socketEvent.waitingFor = NETWORK_WRITE_SIGNAL;
            break;

        case JAVACALL_EVENT_SOCKET_RECEIVE:
            e.data.socketEvent.waitingFor = NETWORK_READ_SIGNAL;
            break;

        case JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED:
            e.data.socketEvent.waitingFor = HOST_NAME_LOOKUP_SIGNAL;
            break;

        case JAVACALL_EVENT_SOCKET_CLOSE_COMPLETED:
            e.data.socketEvent.waitingFor = NETWORK_EXCEPTION_SIGNAL;
            break;

        case JAVACALL_EVENT_SOCKET_REMOTE_DISCONNECTED:
            e.data.socketEvent.waitingFor = NETWORK_EXCEPTION_SIGNAL;
            break;

        default:
            /* IMPL_NOTE: decide what to do */
            return;                 /* do not send event to java */


            /* IMPL_NOTE: NETWORK_EXCEPTION_SIGNAL is not assigned by any indication */
    }

    midp_jc_event_send(&e);
}

/**
 * A callback function to be called for notification of non-blocking
 * server socket only related events, such as a accept completion.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence.
 *
 * @param type type of indication: Either
 *          JAVACALL_EVENT_SERVER_SOCKET_ACCEPT_COMPLETED
 * @param socket_handle handle of socket related to the notification.
 *                          If the platform is not able to provide the socket
 *                          handle in the callback, it should pass 0 as the new_socket_handle
 *                          and the implementation will call javacall_server_socket_accept_finish
 *                          to retrieve the handle for the accepted connection.
 * @param new_socket_handle in case of accept the socket handle for the
 *                          newly created connection
 *
 * @param operation_result <tt>JAVACALL_OK</tt> if operation
 *        completed successfully,
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void /* OPTIONAL */ javanotify_server_socket_event(javacall_server_socket_callback_type type,
                               javacall_handle socket_handle,
                               javacall_handle new_socket_handle,
                               javacall_result operation_result) {
    midp_jc_event_union e;
    REPORT_INFO(LC_CORE, "javanotify_server_socket_event() >>\n");
    e.eventType = MIDP_JC_EVENT_SOCKET;
    e.data.socketEvent.handle = socket_handle;
    e.data.socketEvent.status = operation_result;

    e.data.socketEvent.extraData = NULL;

    if (type == JAVACALL_EVENT_SERVER_SOCKET_ACCEPT_COMPLETED) {
        e.data.socketEvent.waitingFor = NETWORK_READ_SIGNAL;
        /* IMPL_NOTE: how about using  extraData instead of status? Then, malloc required. */


        /* If the platform is not able to provide the socket handle in the callback,
           it should pass 0. */
        if (operation_result == JAVACALL_OK) {
		e.data.socketEvent.status = (javacall_result)((int)new_socket_handle);
        } else {
            e.data.socketEvent.status = operation_result;
        }
    } else {
        /* IMPL_NOTE: decide what to do */
        return;                 /* do not send event to java */
    }

    midp_jc_event_send(&e);
}

/**
 * A callback function to be called for notification of non-blocking
 * client/server socket related events, such as a socket completing opening or ,
 * closing socket remotely, disconnected by peer or data arrived on
 * the socket indication.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence.
 *
 * @param type type of indication: Either
 *     - JAVACALL_EVENT_DATAGRAM_RECVFROM_COMPLETED
 *     - JAVACALL_EVENT_DATAGRAM_SENDTO_COMPLETED
 * @param handle handle of datagram related to the notification
 * @param operation_result <tt>JAVACALL_OK</tt> if operation
 *        completed successfully,
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void javanotify_datagram_event(javacall_datagram_callback_type type,
                               javacall_handle handle,
                               javacall_result operation_result) {

    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_datagram_event() >>\n");

    e.eventType = MIDP_JC_EVENT_SOCKET;
    e.data.socketEvent.handle = handle;
    e.data.socketEvent.status = operation_result;

    e.data.socketEvent.extraData = NULL;

    switch (type) {
        case JAVACALL_EVENT_DATAGRAM_SENDTO_COMPLETED:
            e.data.socketEvent.waitingFor = NETWORK_WRITE_SIGNAL;
            break;
        case JAVACALL_EVENT_DATAGRAM_RECVFROM_COMPLETED:
            e.data.socketEvent.waitingFor = NETWORK_READ_SIGNAL;
            break;
        default:
            /* IMPL_NOTE: decide what to do */
            return;                 /* do not send event to java */

            /* IMPL_NOTE: NETWORK_EXCEPTION_SIGNAL is not assigned by any indication */
    }

    midp_jc_event_send(&e);
}

#ifdef ENABLE_JSR_135
/**
 * Post native media event to Java event handler
 *
 * @param type          Event type
 * @param appId         Application ID
 * @param playerId      Player ID
 * @param status        Event status
 * @param data          Data for this event type
 */
void javanotify_on_media_notification(javacall_media_notification_type type,
                                      int appId,
                                      int playerId,
                                      javacall_result status,
                                      void *data) {
#if ENABLE_JSR_135
    midp_jc_event_union e;
    REPORT_INFO4(LC_MMAPI, "javanotify_on_media_notification type=%d appId=%d playerId%d status=%d\n", type, appId, playerId, status);

    e.eventType = MIDP_JC_EVENT_MULTIMEDIA;
    e.data.multimediaEvent.mediaType  = type;
    e.data.multimediaEvent.appId      = appId;
    e.data.multimediaEvent.playerId   = playerId;
    e.data.multimediaEvent.status     = (int) status;
    e.data.multimediaEvent.data.num32 = (int) data;

    midp_jc_event_send(&e);
#endif
}
#endif

#if ENABLE_JSR_234
/**
 * Post native advanced multimedia event to Java event handler
 *
 * @param type          Event type
 * @param processorId   Processor ID that came from javacall_media_processor_create
 * @param data          Data for this event type
 */
void javanotify_on_amms_notification(javacall_amms_notification_type type,
                                     javacall_int64 processorId,
                                     void *data) {
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_ADVANCED_MULTIMEDIA;
    e.data.multimediaEvent.mediaType = type;
    e.data.multimediaEvent.appId = (int)((processorId >> 32) & 0xFFFF);
    e.data.multimediaEvent.playerId = (int)(processorId & 0xFFFF);

    switch( type )
    {
    case JAVACALL_EVENT_AMMS_SNAP_SHOOTING_STOPPED:
    case JAVACALL_EVENT_AMMS_SNAP_STORAGE_ERROR:
        {
            int i = 0;
            size_t size = 0;
            javacall_utf16_string str16 = ( javacall_utf16_string )data;
            
            javautil_unicode_utf16_utf8length( str16, &size );
            ++size;
            
            e.data.multimediaEvent.data.str16 = ( javacall_utf16_string )
                javacall_malloc( size * sizeof( javacall_utf16 ) );
            for( i = 0; i < size; i++ )
            {
                e.data.multimediaEvent.data.str16[i] = str16[i];
            }
        }
        break;
    default:
        e.data.multimediaEvent.data.num32 = (int) data;
        break;
    }

    REPORT_INFO1(LC_NONE,
            "[javanotify_on_amms_notification] type=%d\n", type);

    midp_jc_event_send(&e);
}
#endif

/**
 * The implementation call this callback notify function when image decode done
 *
 * @param handle Handle that is returned from javacall_image_decode_start
 * @param result the decoding operation result
 */
void javanotify_on_image_decode_end(javacall_handle handle, javacall_result result) {
    REPORT_INFO(LC_CORE, "javanotify_on_image_decode_end() >>\n");

#ifdef ENABLE_EXTERNAL_IMAGE_DECODER
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_IMAGE_DECODER;
    e.data.imageDecoderEvent.handle = handle;
    e.data.imageDecoderEvent.result = result;

    midp_jc_event_send(&e);
#else
   (void)handle;
   (void)result;
#endif
}

#ifdef ENABLE_JSR_75
/**
 * A callback function to be called by the platform in order to notify
 * about changes in the available file system roots (new root was added/
 * a root on removed).
 */
void javanotify_fileconnection_root_changed(void) {
    midp_jc_event_union e;
    REPORT_INFO(LC_CORE, "javanotify_fileconnection_root_changed() >>\n");
    e.eventType = JSR75_FC_JC_EVENT_ROOTCHANGED;

    midp_jc_event_send(&e);
}
#endif

#ifdef ENABLE_JSR_179
/**
 * A callback function to be called for notification of non-blocking
 * location related events.
 * The platform will invoke the call back in platform context for
 * each provider related occurrence.
 *
 * @param type type of indication: Either
 * <pre>
 *          - <tt>JAVACALL_EVENT_LOCATION_OPEN_COMPLETED</tt>
 *          - <tt>JAVACALL_EVENT_LOCATION_ORIENTATION_COMPLETED</tt>
 *          - <tt>JAVACALL_EVENT_LOCATION_UPDATE_PERIODICALLY</tt>
 *          - <tt>JAVACALL_EVENT_LOCATION_UPDATE_ONCE</tt>
 * </pre>
 * @param handle handle of provider related to the notification
 * @param operation_result operation result: Either
 * <pre>
 *      - <tt>JAVACALL_OK</tt> if operation completed successfully,
 *      - <tt>JAVACALL_LOCATION_RESULT_CANCELED</tt> if operation is canceled
 *      - <tt>JAVACALL_LOCATION_RESULT_TIMEOUT</tt>  if operation is timeout
 *      - <tt>JAVACALL_LOCATION_RESULT_OUT_OF_SERVICE</tt> if provider is out of service
 *      - <tt>JAVACALL_LOCATION_RESULT_TEMPORARILY_UNAVAILABLE</tt> if provider is temporarily unavailable
 *      - otherwise, <tt>JAVACALL_FAIL</tt>
 * </pre>
 */
void javanotify_location_event(
        javacall_location_callback_type event,
        javacall_handle provider,
        javacall_location_result operation_result) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_location_event() >>\n");

    e.eventType = JSR179_LOCATION_JC_EVENT;

    switch(event){
    case JAVACALL_EVENT_LOCATION_ORIENTATION_COMPLETED:
        e.data.jsr179LocationEvent.event = JSR179_ORIENTATION_SIGNAL;
        break;
    default:
        e.data.jsr179LocationEvent.event = JSR179_LOCATION_SIGNAL;
        break;
    }

    e.data.jsr179LocationEvent.provider = provider;
    e.data.jsr179LocationEvent.operation_result = operation_result;

    midp_jc_event_send(&e);
}

/**
 * A callback function to be called for notification of proximity monitoring updates.
 *
 * This function will be called only once when the terminal enters the proximity of the registered coordinate.
 *
 * @param provider handle of provider related to the notification
 * @param latitude of registered coordinate.
 * @param longitude of registered coordinate.
 * @param proximityRadius of registered coordinate.
 * @param pLocationInfo location info
 * @param operation_result operation result: Either
 * <pre>
 *      - <tt>JAVACALL_OK</tt> if operation completed successfully,
 *      - <tt>JAVACALL_LOCATION_RESULT_CANCELED</tt> if operation is canceled
 *      - <tt>JAVACALL_LOCATION_RESULT_OUT_OF_SERVICE</tt> if provider is out of service
 *      - <tt>JAVACALL_LOCATION_RESULT_TEMPORARILY_UNAVAILABLE</tt> if provider is temporarily unavailable
 *      - otherwise, <tt>JAVACALL_FAIL</tt>
 * </pre>
 */
void /*OPTIONAL*/javanotify_location_proximity(
        javacall_handle provider,
        double latitude,
        double longitude,
        float proximityRadius,
        javacall_location_location* pLocationInfo,
        javacall_location_result operation_result) {

    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_location_proximity() >>\n");

    e.eventType = JSR179_PROXIMITY_JC_EVENT;
    e.data.jsr179ProximityEvent.provider = provider;
    e.data.jsr179ProximityEvent.latitude = latitude;
    e.data.jsr179ProximityEvent.longitude = longitude;
    e.data.jsr179ProximityEvent.proximityRadius = proximityRadius;
    e.data.jsr179ProximityEvent.operation_result = operation_result;
    if (pLocationInfo)
        memcpy(&(e.data.jsr179ProximityEvent.location), pLocationInfo , sizeof(javacall_location_location));

    midp_jc_event_send(&e);
}

#endif /* ENABLE_JSR_179 */

#ifdef ENABLE_JSR_211

/*
 * Called by platform to notify java VM that invocation of native handler
 * is finished. This is <code>ContentHandlerServer.finish()</code> substitute
 * after platform handler completes invocation processing.
 * @param invoc_id processed invocation Id
 * @param url if not NULL, then changed invocation URL
 * @param argsLen if greater than 0, then number of changed args
 * @param args changed args if @link argsLen is greater than 0
 * @param dataLen if greater than 0, then length of changed data buffer
 * @param data the data
 * @param status result of the invocation processing.
 */

void javanotify_chapi_platform_finish(
        int invoc_id,
        javacall_utf16_string url,
        int argsLen, javacall_utf16_string* args,
        int dataLen, void* data,
        javacall_chapi_invocation_status status
) {
    midp_jc_event_union e;
    javacall_chapi_invocation *inv;
    int i;

    REPORT_INFO(LC_CORE, "javanotify_chapi_platform_finish() >>\n");

    e.eventType = JSR211_JC_EVENT_PLATFORM_FINISH;
    e.data.jsr211PlatformEvent.invoc_id    = invoc_id;
    e.data.jsr211PlatformEvent.jsr211event =
        javacall_malloc (sizeof(*e.data.jsr211PlatformEvent.jsr211event));
    if (NULL != e.data.jsr211PlatformEvent.jsr211event) {
        e.data.jsr211PlatformEvent.jsr211event->status     = status;
        e.data.jsr211PlatformEvent.jsr211event->handler_id = NULL;

        inv = &e.data.jsr211PlatformEvent.jsr211event->invocation;
        inv->url               = javautil_wcsdup (url);
        inv->type              = NULL;
        inv->action            = NULL;
        inv->invokingAppName   = NULL;
        inv->invokingAuthority = NULL;
        inv->username          = NULL;
        inv->password          = NULL;
        inv->argsLen           = argsLen;
        inv->args              =
            javacall_calloc (sizeof(javacall_utf16_string), inv->argsLen);
        if (NULL != inv->args) {
            for (i = 0; i < inv->argsLen; i++) {
                inv->args[i] = javautil_wcsdup(args[i]);
            }
        }
        inv->dataLen           = dataLen;
        inv->data              = javacall_malloc (inv->dataLen);
        if (NULL != inv->data) {
            memcpy (inv->data, data, inv->dataLen);
        }
        inv->responseRequired  = 0;
    }

    midp_jc_event_send(&e);
}

/**
 * Receives invocation request from platform. <BR>
 * This is <code>Registry.invoke()</code> substitute for Platform->Java call.
 * @param handler_id target Java handler Id
 * @param invocation filled out structure with invocation params
 * @param invoc_id invocation Id for further references
 */
void javanotify_chapi_java_invoke(
        const javacall_utf16_string handler_id,
        javacall_chapi_invocation* invocation,
        int invoc_id) {
    midp_jc_event_union e;
    javacall_chapi_invocation *inv;
    int i;

    REPORT_INFO(LC_CORE, "javanotify_chapi_java_invoke() >>\n");

    e.eventType = JSR211_JC_EVENT_JAVA_INVOKE;
    e.data.jsr211PlatformEvent.invoc_id    = invoc_id;
    e.data.jsr211PlatformEvent.jsr211event =
        javacall_malloc (sizeof(*e.data.jsr211PlatformEvent.jsr211event));
    if (NULL != e.data.jsr211PlatformEvent.jsr211event) {
        e.data.jsr211PlatformEvent.jsr211event->handler_id =
            javautil_wcsdup(handler_id);
        e.data.jsr211PlatformEvent.jsr211event->status     =
            INVOCATION_STATUS_ERROR;

        inv = &e.data.jsr211PlatformEvent.jsr211event->invocation;
        inv->url             = javautil_wcsdup(invocation->url);
        inv->type            = javautil_wcsdup(invocation->type);
        inv->action          = javautil_wcsdup(invocation->action);
        inv->invokingAppName =
            javautil_wcsdup(invocation->invokingAppName);
        inv->invokingAuthority =
            javautil_wcsdup(invocation->invokingAuthority);
        inv->username        = javautil_wcsdup(invocation->username);
        inv->password        = javautil_wcsdup(invocation->password);
        inv->argsLen         = invocation->argsLen;
        inv->args            =
            javacall_calloc (sizeof(javacall_utf16_string), inv->argsLen);
        if (NULL != inv->args) {
            for (i = 0; i < inv->argsLen; i++) {
                inv->args[i] = javautil_wcsdup(invocation->args[i]);
            }
        }
        inv->dataLen           = invocation->dataLen;
        inv->data              = javacall_malloc (inv->dataLen);
        if (NULL != inv->data) {
            memcpy (inv->data, invocation->data, inv->dataLen);
        }
        inv->responseRequired  = invocation->responseRequired;
    }

    midp_jc_event_send(&e);
}

javacall_result javanotify_chapi_process_msg_request( int queueID, int dataExchangeID, 
                                                      int msg, const unsigned char * bytes, size_t count ) {
    midp_jc_event_union e;
    jsr211_request_data * data = (jsr211_request_data *)jsr211_malloc( sizeof(*data) );

    REPORT_INFO(LC_CORE, "javanotify_chapi_process_msg_request() >>\n");

    data->queueID = queueID;
    data->msg = msg;
    data->dataExchangeID = dataExchangeID;
    data->bytes = NULL;
    data->count = count;
    if( count > 0 ){
        data->bytes = (unsigned char *)jsr211_malloc( count );
        memcpy( data->bytes, bytes, count );
    } else if( bytes != NULL ){
        data->bytes = "";
    }

    e.eventType = JSR211_JC_EVENT_REQUEST_RECEIVED;
    e.data.jsr211RequestEvent.data = data;

    midp_jc_event_send(&e);
}

javacall_result javanotify_chapi_process_msg_result( int dataExchangeID, const unsigned char * bytes, size_t count ) {
    midp_jc_event_union e;
    jsr211_response_data * data = (jsr211_response_data *)jsr211_malloc( sizeof(*data) );

    REPORT_INFO(LC_CORE, "javanotify_chapi_process_msg_result() >>\n");

    data->dataExchangeID = dataExchangeID;
    data->bytes = NULL;
    data->count = count;
    if( count > 0 ){
        data->bytes = (unsigned char *)jsr211_malloc( count );
        memcpy( data->bytes, bytes, count );
    } else if( bytes != NULL ){
        data->bytes = "";
    }

    e.eventType = JSR211_JC_EVENT_RESPONSE_RECEIVED;
    e.data.jsr211ResponseEvent.data = data;

    midp_jc_event_send(&e);
}

#endif /* ENABLE_JSR_211 */

#ifdef ENABLE_JSR_290

void
javanotify_fluid_image_notify_dirty (
    javacall_handle                       fluid_image,
    javacall_handle                       app_id
    ) {

    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_INVALIDATE;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;

    midp_jc_event_send(&e);
}

void
javanotify_fluid_listener_completed (
    javacall_handle                       fluid_image,
    javacall_handle                       app_id
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_LISTENER_COMPLETED;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;

    midp_jc_event_send(&e);
}
    
void
javanotify_fluid_listener_failed (
    javacall_handle                       fluid_image,
    javacall_handle                       app_id,
    javacall_const_utf16_string           failure,
    const javacall_fluid_failure_types    failure_type
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_LISTENER_FAILED;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;
    e.data.jsr290FluidEvent.text        = javautil_wcsdup(failure);
    e.data.jsr290FluidEvent.failure_type = failure_type;

    midp_jc_event_send(&e);
}

void
javanotify_fluid_listener_percentage (
    javacall_handle                       fluid_image,
    javacall_handle                       app_id,
    float                                 percentage
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_LISTENER_PERCENTAGE;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;
    e.data.jsr290FluidEvent.percentage  = percentage;

    midp_jc_event_send(&e);
}
    
void
javanotify_fluid_listener_started (
    javacall_handle                       fluid_image,
    javacall_handle                       app_id
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_LISTENER_STARTED;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;

    midp_jc_event_send(&e);
}
    
void
javanotify_fluid_image_spawned (
    javacall_handle                       fluid_image,
    javacall_handle                       app_id,
    javacall_handle                       spawn_request
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_IMAGE_SPAWNED;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;
    e.data.jsr290FluidEvent.spare       = spawn_request;

    midp_jc_event_send(&e);
}
    
void
javanotify_fluid_listener_warning (
    javacall_handle                       fluid_image,
    javacall_handle                       app_id,
    javacall_const_utf16_string           warning
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_LISTENER_WARNING;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;
    e.data.jsr290FluidEvent.text        = javautil_wcsdup(warning);

    midp_jc_event_send(&e);
}

void
javanotify_fluid_listener_document_available (
    javacall_handle                       fluid_image,
    javacall_handle                       app_id
    ) {
	midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_LISTENER_DOCUMENT_AVAILABLE;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;

    midp_jc_event_send(&e);
}

#include <stdio.h>
#include <javacall_logging.h>

void
javanotify_fluid_request_resource (
    javacall_handle                       fluid_image,
    javacall_handle                       app_id,
    javacall_handle                       request,
    javacall_const_utf16_string           url
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_REQUEST_RESOURCE;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;
    e.data.jsr290FluidEvent.spare       = request;
    e.data.jsr290FluidEvent.text        = javautil_wcsdup(url);

    midp_jc_event_send(&e);
}

void
javanotify_fluid_cancel_request (
    javacall_handle                       fluid_image,
    javacall_handle                       app_id,
    javacall_handle                       request
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_CANCEL_REQUEST;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;
    e.data.jsr290FluidEvent.spare       = request;

    midp_jc_event_send(&e);
}

void
javanotify_fluid_filter_xml_http_request (
    javacall_handle                       request,
    javacall_handle                       fluid_image,
    javacall_handle                       app_id,
    javacall_const_utf16_string           method,
    javacall_const_utf16_string           url
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_FILTER_XML_HTTP_REQUEST;
    e.data.jsr290FluidEvent.spare       = request;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;
    e.data.jsr290FluidEvent.text        = javautil_wcsdup(method);
    e.data.jsr290FluidEvent.text1       = javautil_wcsdup(url);

    midp_jc_event_send(&e);
}

void
javanotify_method_completion_notification (
    javacall_int32			              invocation_id
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_COMPLETION_NOTIFICATION;
    e.data.jsr290NotificationEvent.invocation_id = invocation_id;

    midp_jc_event_send(&e);
}

void
javanotify_fluid_handle_event_request (
    javacall_handle                       request_handle,
    javacall_handle                       app_id
    ) {
	midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_fluid_event_request() >>\n");
    e.eventType = JSR290_JC_EVENT_HANDLE_EVENT;
    e.data.jsr290HandleEventRequest.request_handle = request_handle;
    e.data.jsr290HandleEventRequest.app_id         = app_id;
    midp_jc_event_send(&e);
}

void 
javanotify_fluid_display_box (
    javacall_handle                       fluid_image,
    javacall_handle                       app_id,
    javacall_handle                       request,
    javacall_const_utf16_string           message,
    javacall_const_utf16_string           defaultValue,
    const javacall_fluid_message_box_type type
    ) {

    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_DISPLAY_BOX;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.spare       = request;
    e.data.jsr290FluidEvent.text        = javautil_wcsdup(message);
    e.data.jsr290FluidEvent.text1       = javautil_wcsdup(defaultValue);
    e.data.jsr290FluidEvent.result      = type;

    midp_jc_event_send(&e);
}

void
javanotify_fluid_layout_changed(
    javacall_handle                       fluid_image,
    javacall_handle                       app_id
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_LAYOUT_CHANGED;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;

    midp_jc_event_send(&e);
}

void
javanotify_fluid_focus_changed(
    javacall_handle                       fluid_image,
    javacall_handle                       app_id
    ) {
    midp_jc_event_union e;

    e.eventType = JSR290_JC_EVENT_FLUID_FOCUS_CHANGED;
    e.data.jsr290FluidEvent.fluid_image = fluid_image;
    e.data.jsr290FluidEvent.app_id      = app_id;

    midp_jc_event_send(&e);
}

#endif /* ENABLE_JSR_290 */

#ifdef ENABLE_JSR_257
/*
 * Called by the platform to notify java VM about contactless event
 * @param event an event identifier
 */
void javanotify_contactless_event(jsr257_contactless_event_type event) {
    midp_jc_event_union e;
    REPORT_INFO(LC_CORE, "javanotify_contactless_event() >>\n");
    e.eventType = JSR257_JC_EVENT_CONTACTLESS;
    e.data.jsr257Event.eventType = event;
    midp_jc_event_send(&e);
}

/*
 * Called by the platform to notify java VM about contactless event
 * registered in push subsystem
 * @param eventData a handle to identify contactless push event
 */
void javanotify_push_contactless_event(javacall_handle eventData) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_push_contactless_event() >>\n");
    e.eventType = JSR257_JC_PUSH_NDEF_RECORD_DISCOVERED;
    e.data.jsr257Event.eventData[0] = eventData;
    midp_jc_event_send(&e);
}

/*
 * Called by the platform to notify java VM about contactless event
 * processed by MIDP to send it to the particular isolate
 * @param event an event identifier
 * @param eventData a handle to retrieve additional event data
 * @param isolateId the isolate to send notification to
 */
void javanotify_midp_contactless_event(jsr257_contactless_midp_event_type event, 
        int isolateId, javacall_int32* pParams, javacall_int32 ParamNum) {
        
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_midp_contactless_event() >>\n");
    
     printf("\n DEBUG: javanotify_midp_contactless_event(): isolateId = %d, ParamNum = %d\n",
        isolateId, ParamNum); 
    
    e.eventType = JSR257_JC_MIDP_EVENT;
    e.data.jsr257Event.eventType = event;
    e.data.jsr257Event.isolateId = isolateId;
    if(ParamNum > 0) {
        if(ParamNum > 3) {
            ParamNum = 3;
        }
        memcpy(e.data.jsr257Event.eventData, pParams, sizeof(javacall_int32) *
            ParamNum);
     printf("\n DEBUG: javanotify_midp_contactless_event(): before calling midp_jc_event_send(): eventData[0] = 0x%X\n",
        e.data.jsr257Event.eventData[0]);
        midp_jc_event_send(&e);
    }
}
#endif /* ENABLE_JSR_257 */

/**
 * The platform calls this callback notify function when the permission dialog
 * is dismissed. The platform will invoke the callback in platform context.
 *
 * @param userPermssion the permission level the user chose
 */
void javanotify_security_permission_dialog_finish(
                         javacall_security_permission_type userPermission) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_security_permission_dialog_finish() >>\n");

    e.eventType = MIDP_JC_EVENT_PERMISSION_DIALOG;
    e.data.permissionDialog_event.permission_level = userPermission;

    midp_jc_event_send(&e);
}

#ifdef USE_VSCL

/*
 * See javacall_vscl.h for description
 */
void javanotify_vscl_incoming_event(javacall_vscl_event_type type,
                                    javacall_utf16* str1,
                                    javacall_utf16* str2) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_vscl_incoming_event() >>\n");

    switch (type) {
    case JAVACALL_VSCL_INCOMING_CALL:
        e.eventType = MIDP_JC_EVENT_VSCL_INCOMING_CALL;
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=INCOMING_CALL\n");
        break;
    case JAVACALL_VSCL_CALL_DROPPED:
        e.eventType = MIDP_JC_EVENT_VSCL_CALL_DROPPED;
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=CALL_DROPPED\n");
        break;
    case JAVACALL_VSCL_INCOMING_MAIL_CBS:
    case JAVACALL_VSCL_INCOMING_MAIL_DELIVERY_CONF:
    case JAVACALL_VSCL_INCOMING_MAIL_MMS:
    case JAVACALL_VSCL_INCOMING_MAIL_SMS:
    case JAVACALL_VSCL_INCOMING_MAIL_WAP_PUSH:
    case JAVACALL_VSCL_SCHEDULED_ALARM:
                REPORT_INFO1(LC_VSCL,"javanotify_vscl_incoming_event() EVENT=%d NOT IMPLEMENTED!\n", (int)type);
        return;
    case JAVACALL_VSCL_FLIP_OPEN:
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=FLIP_OPEN\n");
        e.eventType = MIDP_JC_EVENT_VSCL_FLIP_OPENED;
        break;
    case JAVACALL_VSCL_FLIP_CLOSED:
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=FLIP_CLOSED\n");
        e.eventType = MIDP_JC_EVENT_VSCL_FLIP_CLOSED;
        break;
    default:
                REPORT_INFO1(LC_VSCL,"javanotify_vscl_incoming_event() EVENT=%d NOT IMPLEMENTED!\n", (int)type);
        return;
    }

    midp_jc_event_send(&e);
}
#endif /* USE_VSCL */

/**
 * Notify Java that the user has made a selection
 *
 * @param phoneNumber a string representing the phone number the user selected.
 *                    The string will be copied so it can be freed after this
 *                    function call returns.
 *                    In case the user dismissed the phonebook without making a
 *                    selection, this sting is <tt>NULL</tt>
 */
void /* OPTIONAL */ javanotify_textfield_phonenumber_selection(char* phoneNumber) {
    midp_jc_event_union e;
    int length;

    REPORT_INFO(LC_CORE, "javanotify_textfield_phonenumber_selection() >>\n");

    e.eventType = MIDP_JC_EVENT_PHONEBOOK;
    memset(selectedNumber, 0, MAX_PHONE_NUMBER_LENGTH);

    if (phoneNumber != NULL) {
        length = strlen(phoneNumber) + 1;
        if (length <= MAX_PHONE_NUMBER_LENGTH) {
            memcpy(selectedNumber, phoneNumber, length);
        } else {
            memcpy(selectedNumber, phoneNumber, MAX_PHONE_NUMBER_LENGTH);
        }
    }

    e.data.phonebook_event.phoneNumber = selectedNumber;

    midp_jc_event_send(&e);
}

void /* OPTIONAL */ javanotify_rotation(int hardwareId) {
    midp_jc_event_union e;

    (void)hardwareId;
    REPORT_INFO(LC_CORE, "javanotify_rotation() >>\n");

    e.eventType = MIDP_JC_EVENT_ROTATION;
    midp_jc_event_send(&e);
}

/**
  * The platform should invoke this function in platform context
  * to notify display device state change
  */
void  /* OPTIONAL */ javanotify_display_device_state_changed(int hardwareId, javacall_lcd_display_device_state state) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_display_device_state_changed >>\n");
    e.data.displayDeviceEvent.hardwareId =  hardwareId;
    e.data.displayDeviceEvent.state = state;
    e.eventType = MIDP_JC_EVENT_DISPLAY_DEVICE_STATE_CHANGED;
    midp_jc_event_send(&e);
}

/**
  * The platform should invoke this function in platform context
  * to notify clamshell state change
  */
    void  /* OPTIONAL */ javanotify_clamshell_state_changed(javacall_lcd_clamshell_state state) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_clamshell_state_changed >>\n");
    e.data.clamshellEvent.state = state;
    e.eventType = MIDP_JC_EVENT_CLAMSHELL_STATE_CHANGED;
    midp_jc_event_send(&e);
}


void javanotify_virtual_keyboard() {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_virtual_keyboard() >>\n");

    e.eventType = MIDP_JC_EVENT_VIRTUAL_KEYBOARD;
    midp_jc_event_send(&e);
}

#ifdef ENABLE_API_EXTENSIONS

/**
 * 
 * The implementation calls this callback notify function when master volume dialog
 * is dismissed. The platfrom will invoke the callback in platform context.
 * This function is used only for asynchronous mode of the function
 * javacall_prompt_volume.
 * 
 */
void javanotify_prompt_volume_finish(void) {
    midp_jc_event_union e;
    REPORT_INFO1(LC_PROTOCOL, 
                 "[javanotify_prompt_volume_finish] status=%d",
                 JAVACALL_OK);
    e.eventType = MIDP_JC_EVENT_VOLUME;
    e.data.VolumeEvent.stub = 0;

}

#endif /*ENABLE_API_EXTENSIONS*/


void javanotify_widget_menu_selection(int cmd) {
    // This command comes from a menu item dynamically
    // created in the native method
    // SoftButtonLayer.setNativePopupMenu()
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_MENU_SELECTION;
    e.data.menuSelectionEvent.menuIndex = cmd;

    midp_jc_event_send(&e);
}


/**
 * Notfy native peer widget state changed, such as key pressed in editor control.
 * Now only java TextField/TextBox has native peer.
 */
void javanotify_peerchanged_event(void) {
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_PEER_CHANGED;
    midp_jc_event_send(&e);
}

#if ENABLE_ON_DEVICE_DEBUG
/**
 * The platform calls this function to inform VM that
 * ODTAgent midlet must be enabled.
 */
void javanotify_enable_odd() {
     midp_jc_event_union e;

     REPORT_INFO(LC_CORE, "javanotify_enable_odd() >>\n");

     e.eventType = MIDP_JC_ENABLE_ODD_EVENT;
     midp_jc_event_send(&e);
}
#endif

#ifdef __cplusplus
}
#endif

