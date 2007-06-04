/*
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

/**
 * @file
 *
 */

#include "javacall_sms.h"
#include "javacall_logging.h"
#include "javacall_defs.h"
#include "javacall_datagram.h"
#include "javacall_network.h"

extern char* encodeSmsBuffer(
    int encodingType, int destPortNum, javacall_int64 timeStamp, 
    const char* recipientPhone, const char* senderPhone, int msgLength, const char* msg,
    int sourcePortNum,
    int* out_encode_sms_buffer_length);
extern char* getIPBytes_nonblock(char *hostname);

extern char* getProp(const char* propName, char* defaultValue);
extern int getIntProp(const char* propName, int defaultValue);
extern const char* getStrProp(const char* propName, const char* defaultValue);

/**
 * send an SMS message
 *
 * Actually 
 *   - sends a datagramm to 11101 port, it can be received by WMATool.jar (used in TCK tests)
 *   - writes a message to the console (it is enough for native tests)
 * The address to send datagram is one of the following:
 *   - 127.0.0.1 if JSR_120_DATAGRAM_HOST environment variable is not send
 *   - JSR_120_DATAGRAM_HOST environment variable value (if set).
 *
 * Refer to javacall_sms.h header for complete description.
 */
javacall_result javacall_sms_send(  javacall_sms_encoding    msgType, 
                        const unsigned char*    destAddress, 
                        const unsigned char*    msgBuffer, 
                        int                     msgBufferLen, 
                        unsigned short          sourcePort, 
                        unsigned short          destPort,
                        int                     handle ) {

    javacall_handle datagramHandle;
    javacall_result ok;
    int pBytesWritten = 0;
    void *pContext;
    unsigned char *pAddress;

    javacall_int64 timeStamp = 0;
    const char* recipientPhone = destAddress;
    const char* senderPhone = getStrProp("JSR_120_PHONE_NUMBER", "+1234567");
    int encodedSMSLength;
    char* encodedSMS;

    char* IP_text = getProp("JSR_120_DATAGRAM_HOST", "127.0.0.1");
    //WMATool listens on 11101 port, but sends to 11100 port
    int smsRemotePortNumber = getIntProp("JSR_120_SMS_OUT_PORT", 11101);

    javacall_network_init_start();
    pAddress = getIPBytes_nonblock(IP_text);

    encodedSMS = encodeSmsBuffer(msgType, destPort, timeStamp, recipientPhone, senderPhone, 
        msgBufferLen, msgBuffer, sourcePort, &encodedSMSLength);

    ok = javacall_datagram_open(0, &datagramHandle);
    if (ok == JAVACALL_OK) {
        ok = javacall_datagram_sendto_start(datagramHandle, pAddress, smsRemotePortNumber,
            encodedSMS, encodedSMSLength, &pBytesWritten, &pContext);
        if (ok != JAVACALL_OK) {
            javacall_print("Error: SMS sending - datagram blocked.\n");
        }
    }

    javacall_print("## javacall: SMS sending...\n");

    javanotify_sms_send_completed(JAVACALL_OK, handle);

    return JAVACALL_OK;
}

javacall_result javacall_sms_is_service_available(void){
    return JAVACALL_OK;
}

#define PORTS_MAX 8
static unsigned short portsList[PORTS_MAX] = {0,0,0,0,0,0,0,0};

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

    //printf("javacall_sms_add_listening_port %i", portNum);
    if (free == -1) {
        javacall_print("ports amount exceeded");
        return JAVACALL_FAIL;
    }

    portsList[free] = portNum;

    return JAVACALL_OK;
}

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

javacall_result javacall_is_sms_port_registered(unsigned short portNum) {
    int i;
    for (i=0; i<PORTS_MAX; i++) {
        if (portsList[i] == portNum) {
            return JAVACALL_OK;
        }
    }
    return JAVACALL_FAIL;
}

int getBitsize(int numBytes,int bitSizeofChar){
    return (numBytes * 8) - ((numBytes * 8) % bitSizeofChar);
}

/**
 * returns the number of segments (individual SMS messages) that would 
 * be needed in the underlying protocol to send a specified message. 
 *
 * The specified message is included as a parameter to this API.
 * Note that this method does not actually send the message. 
 * It will only calculate the number of protocol segments needed for sending 
 * the message. This API returns a count of the message segments that would be 
 * sent for the provided Message.
 *
 * @param msgType message string type: Text or Binary.
 *                The target device should decide the DCS (Data Coding Scheme)  
 *                in the PDU according to this parameter and the  message contents.   
 *                If the target device is compliant with GSM 3.40, then for a Binary 
 *                Message,  the DCS in PDU should be 8-bit binary. 
 *                For a  Text Message, the target device should decide the DCS  according to  
 *                the  message contents. When all characters in the message contents are in 
 *                the GSM 7-bit alphabet, the DCS should be GSM 7-bit; otherwise, it should  
 *                be  UCS-2.
 * @param msgBuffer the message body (payload) to be sent
 * @param msgBufferLen the message body (payload) len
 * @param hasPort indicates if the message includes source or destination port number 
 * @return number of segments, or 0 value on error
 */

int javacall_sms_get_number_of_segments(
        javacall_sms_encoding   msgType, 
        signed char*            msgBuffer, 
        int                     msgBufferLen, 
        javacall_bool           hasPort) {

    int fragmentSize, headSize, segments;

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
    }

    if (msgBufferLen < fragmentSize) {
        segments = 1;
    } else {
        fragmentSize -= headSize;
        segments = (msgBufferLen + fragmentSize - 1) / fragmentSize;
    }

    //printf("javacall_sms_get_number_of_segments: %d\n", segments);
    return segments;
}
