/*
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


#include "javacall_sms.h"
#include "javacall_logging.h"
#include "javacall_defs.h"
#include "javacall_datagram.h"
#include "javacall_network.h"
#include "javacall_time.h"

extern char* encodeSmsBuffer(
    javacall_sms_encoding encodingType, 
    int            destPortNum, 
    int            srcPortNum, 
    javacall_int64 timeStamp,
    const char*    recipientPhone, 
    const char*    senderPhone, 
    int            msgLength, 
    const char*    msg,
    int*           out_encode_sms_buffer_length,
    int            fragment);

extern char* getIPBytes_nonblock(char* hostname);

extern char* getProp(const char* propName, char* defaultValue);
extern int getIntProp(const char* propName, int defaultValue);

extern smsOutPortNumber;
extern char *devicePhoneNumber;

extern javacall_handle smsDatagramSocketHandle;

/**
 * Sends an SMS message via UDP emulation.
 *
 * Extern smsOutPortNumber variable contains UDP port.
 * The address to send datagram is one of the following:
 *   - 127.0.0.1 if JSR_205_DATAGRAM_HOST environment variable is not send
 *   - JSR_205_DATAGRAM_HOST environment variable value (if set).
 *
 * Refer to javacall_sms.h header for complete description.
 */
int javacall_sms_send(  javacall_sms_encoding    msgType,
                        const unsigned char*    destAddress,
                        const unsigned char*    msgBuffer,
                        int                     msgBufferLen,
                        unsigned short          sourcePort,
                        unsigned short          destPort,
                        int                     handle) {

    javacall_result ok = JAVACALL_FAIL;
    int             pBytesWritten = 0;
    void*           pContext;
    unsigned char*  pAddress;

    javacall_int64 timeStamp = javacall_time_get_seconds_since_1970();
    const char*    recipientPhone = (const char*)destAddress;
    char*          senderPhone = devicePhoneNumber;
    int            encodedSMSLength;
    char*          encodedSMS;

    char* IP_text = getProp("JSR_205_DATAGRAM_HOST", "127.0.0.1");

    int fragment = 0;

    javacall_network_init_start();
    pAddress = (unsigned char*)getIPBytes_nonblock(IP_text);

    for (fragment = 0; ; fragment++) {

        encodedSMS = encodeSmsBuffer(msgType, destPort, sourcePort,
            timeStamp, recipientPhone, senderPhone,
            msgBufferLen, (const char*)msgBuffer, &encodedSMSLength, fragment);

        if (encodedSMS == NULL) {
            break;
        }

        if (smsDatagramSocketHandle) {
            ok = javacall_datagram_sendto_start(
                smsDatagramSocketHandle, pAddress, smsOutPortNumber,
                encodedSMS, encodedSMSLength, &pBytesWritten, &pContext);
            if (ok != JAVACALL_OK) {
               javautil_debug_print(JAVACALL_LOG_ERROR, "sms", 
                  "Error: SMS sending - datagram blocked.\n");
            }
        } else {
            javautil_debug_print(JAVACALL_LOG_ERROR, "sms", 
                "Error: SMS sending - network not initialized.\n");
        }
    }

    javautil_debug_print(JAVACALL_LOG_INFORMATION, "sms", 
        "## javacall: SMS sending...\n");

    javanotify_sms_send_completed(JAVACALL_OK, handle);

    return ok;
}

javacall_result javacall_sms_is_service_available(void){
    return JAVACALL_OK;
}

/**
 * The implementation limit: maximum number of open ports.
 */
#define PORTS_MAX 8
static unsigned short portsList[PORTS_MAX] = {0,0,0,0,0,0,0,0};

/**
 * Refer to javacall_sms.h header for complete description.
 */
javacall_result javacall_sms_add_listening_port(unsigned short portNum){

    int i;
    int free = -1;
    for (i=0; i<PORTS_MAX; i++) {
        if (portsList[i] == 0) {
            free = i;
            continue;
        }
        if (portsList[i] == portNum) {
            return JAVACALL_FAIL;
        }
    }

    if (free == -1) {
        javautil_debug_print(JAVACALL_LOG_ERROR, "sms", "ports amount exceeded");
        return JAVACALL_FAIL;
    }

    portsList[free] = portNum;

    return JAVACALL_OK;
}

/**
 * Refer to javacall_sms.h header for complete description.
 */
javacall_result javacall_sms_remove_listening_port(unsigned short portNum) {

    int i;
    for (i=0; i<PORTS_MAX; i++) {
        if (portsList[i] == 0) {
            continue;
        }
        if (portsList[i] == portNum) {
            portsList[i] = 0;
            return JAVACALL_OK;
        }
    }

    return JAVACALL_FAIL;
}

/**
 * Refer to javacall_sms.h header for complete description.
 */
javacall_result javacall_is_sms_port_registered(unsigned short portNum) {
    int i;
    for (i=0; i<PORTS_MAX; i++) {
        if (portsList[i] == portNum) {
            return JAVACALL_OK;
        }
    }
    return JAVACALL_FAIL;
}

/**
 * Refer to javacall_sms.h header for complete description.
 */
int javacall_sms_get_number_of_segments(
        javacall_sms_encoding msgType,
        char*                 msgBuffer,
        int                   msgBufferLen,
        javacall_bool         hasPort) {

    int fragmentSize, headSize, segments;

    (char*)msgBuffer;

    switch(msgType) {
        case JAVACALL_SMS_MSG_TYPE_ASCII:
            fragmentSize = 160 - (hasPort? 7 : 0);
            headSize = 8;
            break;
        case JAVACALL_SMS_MSG_TYPE_UNICODE_UCS2:
            fragmentSize = 140 - (hasPort? 6 : 0);
            headSize = 8;
            break;
        case JAVACALL_SMS_MSG_TYPE_BINARY:
            fragmentSize = 140 - (hasPort? 6 : 0);
            headSize = 7;
            break;
        default:
            javautil_debug_print(JAVACALL_LOG_ERROR, "sms_get_number_of_segments", 
                "Wrong message type.\n");
            return 0;
    }

    if (msgBufferLen < fragmentSize) {
        segments = 1;
    } else {
        fragmentSize -= headSize;
        segments = (msgBufferLen + fragmentSize - 1) / fragmentSize;
    }

    return segments;
}

