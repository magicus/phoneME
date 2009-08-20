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
 * Simple implementation of wma UDP Emulator.
 * The messages supposed to be received from WMATool.jar:
 * bash-3.1$ java -jar midp/bin/i386/WMATool.jar -send mms://1234:1234 -message "blah" -multipart -verbose
 * bash-3.1$ java -jar midp/bin/i386/WMATool.jar -send sms://1234:1234 -message "blah"
 * To receive message start:
 * bash-3.1$ java -jar midp/bin/i386/WMATool.jar -receive mms://9800 -verbose
 */

#include "javacall_network.h"
#include "javacall_datagram.h"
#include "javacall_sms.h"
#include "javacall_cbs.h"
#include "javacall_logging.h"
#include "javacall_defs.h"
#include <stdlib.h> //getenv

#include <string.h>

#define DEFAULT_SMS_IN_PORT 11100
#define DEFAULT_CBS_IN_PORT 22200
#define DEFAULT_MMS_IN_PORT 33300


javacall_handle smsDatagramSocketHandle = NULL;
javacall_handle cbsDatagramSocketHandle = NULL;
#if (ENABLE_JSR_205)
javacall_handle mmsDatagramSocketHandle = NULL;
#endif

/**
 * Load string property value from environment.
 * Simple analog for com.sun.tck.wma.PropLoader.getProp()
 */
char* getProp(const char* propName, char* defaultValue) {
    char* value = getenv(propName);
    return value ? value : defaultValue;
}

/**
 * Load string property value from environment.
 * Simple analog for com.sun.tck.wma.PropLoader.getIntProp()
 */
int getIntProp(const char* propName, int defaultValue) {
    char* value = getenv(propName);
    return value ? atoi(value) : defaultValue;
}

const char* getStrProp(const char* propName, const char* defaultValue) {
    char* value = getenv(propName);
    return value ? value : defaultValue;
}

static void decodeSmsBuffer(char *buffer, 
    int* encodingType, int* destPortNum, javacall_int64* timeStamp, 
    char** recipientPhone, char** senderPhone, 
    int* msgLength, char** msg, int* sourcePortNum) {

    char* ptr = buffer;

    *encodingType   = *((int*)ptr);  ptr += sizeof(int);
    *destPortNum    = *((int*)ptr);  ptr += sizeof(int);
    *timeStamp      = *((javacall_int64*)ptr); ptr += sizeof(javacall_int64);

    *recipientPhone = ptr;           while(*(ptr++) != 0);
    *senderPhone    = ptr;           while(*(ptr++) != 0);
    *msgLength      = *((int*)ptr);  ptr += sizeof(int);
    *msg            = ptr;           ptr += *msgLength;

    *sourcePortNum    = *((int*)ptr);  //ptr += sizeof(int);

    //printf("SMS received: recipientPhone=%s senderPhone=%s destPortNum=%i sourcePortNum=%i msg=%s\n", 
    //    *recipientPhone, *senderPhone, *destPortNum, *sourcePortNum, *msg);

    *ptr = 0;
}

//#define SMS_BUFF_LENGTH (1024*40)
#define SMS_BUFF_LENGTH 1024
char encode_sms_buffer[SMS_BUFF_LENGTH];

char* encodeSmsBuffer(
    int encodingType, int destPortNum, javacall_int64 timeStamp, 
    const char* recipientPhone, const char* senderPhone, int msgLength, const char* msg,
    int sourcePortNum,
    int* out_encode_sms_buffer_length) {

    char* ptr = encode_sms_buffer;
    int lngth;

    if (strlen(recipientPhone) + strlen(senderPhone) + msgLength + 64 > SMS_BUFF_LENGTH) {
        javacall_print("Error: too big SMS!");
        *out_encode_sms_buffer_length = 0;
        return encode_sms_buffer;
    }

    //printf("SMS sending: recipientPhone=%s senderPhone=%s destPortNum=%i sourcePortNum=%i, msg=%s\n", 
    //    recipientPhone, senderPhone, destPortNum, sourcePortNum, msg);

    *((int*)ptr) = encodingType;         ptr += sizeof(int);
    *((int*)ptr) = destPortNum;          ptr += sizeof(int);
    *((javacall_int64*)ptr) = timeStamp; ptr += sizeof(javacall_int64);
    lngth = strlen(recipientPhone) + 1;  memcpy(ptr, recipientPhone, lngth); ptr += lngth;
    lngth = strlen(senderPhone) + 1;     memcpy(ptr, senderPhone,    lngth); ptr += lngth;
    *((int*)ptr) = msgLength;            ptr += sizeof(int);
    memcpy(ptr, msg, msgLength);         ptr += msgLength;
    *((int*)ptr) = sourcePortNum;        ptr += sizeof(int);

    *out_encode_sms_buffer_length = ptr - encode_sms_buffer;
    return encode_sms_buffer;
}

extern javacall_result javacall_is_sms_port_registered(unsigned short portNum);
extern javacall_result javacall_is_cbs_msgID_registered(unsigned short portNum);

javacall_result process_UDPEmulator_sms_incoming(javacall_handle handle) {

    unsigned char pAddress[256];
    int port;
    char buffer[1024];
    int length = 1024;
    int pBytesRead;
    void *pContext = NULL;

    javacall_sms_encoding   encodingType;
    int                     encodingType_int;
    char*                   msg;
    int                     msgLen;
    int                     destPortNum;
    int                     sourcePortNum;
    javacall_int64          timeStamp;
    char*                   recipientPhone;
    char*                   senderPhone;

    int ok;

    ok = javacall_datagram_recvfrom_start(
        handle, pAddress, &port, buffer, length, &pBytesRead, &pContext);

    decodeSmsBuffer(buffer, &encodingType_int, &destPortNum, &timeStamp, 
        &recipientPhone, &senderPhone, &msgLen, &msg, &sourcePortNum);

    if (javacall_is_sms_port_registered((unsigned short)destPortNum) != JAVACALL_OK) {
        javacall_print("SMS on unregistered port received!");
        return JAVACALL_FAIL;
    }
    //printf("## javacall: SMS received. %i/%i\n", destPortNum, sourcePortNum);

    encodingType = encodingType_int;
    javanotify_incoming_sms(encodingType, senderPhone, msg, msgLen, 
        (unsigned short)sourcePortNum, (unsigned short)destPortNum, timeStamp);

    return JAVACALL_OK;
}

javacall_result process_UDPEmulator_cbs_incoming(javacall_handle handle) {

    javacall_cbs_encoding  msgType = JAVACALL_CBS_MSG_TYPE_ASCII;
    unsigned short         msgID = 13;
    unsigned char*         msgBuffer = "msgBuffer";
    int                    msgBufferLen = 9; //strlen(msgBuffer);

    unsigned char pAddress[256];
    int port;
    char buffer[1024];
    int length = 1024;
    int pBytesRead;
    void *pContext = NULL;
    int ok;

    ok = javacall_datagram_recvfrom_start(
        handle, pAddress, &port, buffer, length, &pBytesRead, &pContext);

    if (pBytesRead >= 12) {
        msgType = *(int*)buffer;
        msgID   = (unsigned short)*(int*)(buffer+4);
        msgBufferLen  = *(int*)(buffer+8);
        msgBuffer = (pBytesRead > 12) ? buffer+12 : NULL;
    } else {
        javacall_print("bad cbs package received");
    }

    if (javacall_is_cbs_msgID_registered(msgID) != JAVACALL_OK) {
        javacall_print("CBS on unregistered msgID received!");
        return JAVACALL_FAIL;
    }

    javanotify_incoming_cbs(msgType, msgID, msgBuffer, msgBufferLen);

    return JAVACALL_OK;
}

#if (ENABLE_JSR_205)
extern javacall_result process_UDPEmulator_mms_incoming(javacall_handle handle);
#endif

/**
 * Starts UDP WMA emulation.
 * Opens sockets.
 */
javacall_result init_wma_emulator() {
    javacall_result ok = JAVACALL_OK;
    javacall_result ok1;
    int smsInPortNumber, cbsInPortNumber;
#if (ENABLE_JSR_205)
    int mmsInPortNumber;
#endif

    smsInPortNumber = getIntProp("JSR_120_SMS_PORT", DEFAULT_SMS_IN_PORT);
    ok1 = javacall_datagram_open(smsInPortNumber, &smsDatagramSocketHandle);
    if (ok1 == JAVACALL_OK) { ok = JAVACALL_FAIL; }

    cbsInPortNumber = getIntProp("JSR_120_CBS_PORT", DEFAULT_CBS_IN_PORT);
    ok1 = javacall_datagram_open(cbsInPortNumber, &cbsDatagramSocketHandle);
    if (ok1 == JAVACALL_OK) { ok = JAVACALL_FAIL; }

#if (ENABLE_JSR_205)
    mmsInPortNumber = getIntProp("JSR_205_MMS_PORT", DEFAULT_MMS_IN_PORT);
    ok1 = javacall_datagram_open(mmsInPortNumber, &mmsDatagramSocketHandle);
    if (ok1 == JAVACALL_OK) { ok = JAVACALL_FAIL; }
#endif

    return ok;
}

/**
 * Finishes UDP WMA emulation.
 * Closes sockets.
 */
javacall_result finalize_wma_emulator() {
    javacall_result ok1, ok2, ok3 = JAVACALL_OK;

    ok1 = javacall_datagram_close(smsDatagramSocketHandle);
    ok2 = javacall_datagram_close(cbsDatagramSocketHandle);
#if (ENABLE_JSR_205)
    ok3 = javacall_datagram_close(mmsDatagramSocketHandle);
#endif

    return (ok1==JAVACALL_OK && ok2==JAVACALL_OK && ok3==JAVACALL_OK) ? JAVACALL_OK : JAVACALL_FAIL;
}

/**
 * Checks if the handle is of wma_emulator sockets.
 *   returns JAVACALL_FAIL for mismatch
 *   returns JAVACALL_OK for proper sockets and processes the emulation
 */
javacall_result try_process_wma_emulator(javacall_handle handle) {
    if (handle == smsDatagramSocketHandle) {
        process_UDPEmulator_sms_incoming(handle);
        return JAVACALL_OK;
    }
    if (handle == cbsDatagramSocketHandle) {
        process_UDPEmulator_cbs_incoming(handle);
        return JAVACALL_OK;
    }
#if (ENABLE_JSR_205)
    if (handle == mmsDatagramSocketHandle) {
        process_UDPEmulator_mms_incoming(handle);
        return JAVACALL_OK;
    }
#endif
    return JAVACALL_FAIL;
}
