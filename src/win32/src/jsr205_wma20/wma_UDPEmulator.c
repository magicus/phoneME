/*
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

/**
 * @file
 *
 * Simple implementation of wma UDP Emulator.
 * The messages supposed to be received from JSR205Tool.jar:
 * bash-3.1$ java -jar midp/bin/i386/JSR205Tool.jar -send mms://1234:1234 -message "blah" -multipart -verbose
 * bash-3.1$ java -jar midp/bin/i386/JSR205Tool.jar -send sms://1234:1234 -message "blah"
 *
 */

#include "javacall_network.h"
#include "javacall_datagram.h"
#include "javacall_sms.h"
#include "javacall_mms.h"
#include "javacall_cbs.h"
#include "javacall_logging.h"
#include "javacall_defs.h"

#include <string.h>

#define smsInPortNumber 11100
#define mmsInPortNumber 33300
#define cbsInPortNumber 22200

javacall_handle smsDatagramSocketHandle = NULL;
javacall_handle mmsDatagramSocketHandle = NULL;
javacall_handle cbsDatagramSocketHandle = NULL;

/**
 * Starts UDP WMA emulation.
 * Opens sockets.
 */
javacall_result init_wma_emulator() {
    javacall_result ok1, ok2, ok3;

    ok1 = javacall_datagram_open(smsInPortNumber, &smsDatagramSocketHandle);
    ok2 = javacall_datagram_open(mmsInPortNumber, &mmsDatagramSocketHandle);
    ok3 = javacall_datagram_open(cbsInPortNumber, &cbsDatagramSocketHandle);

    return (ok1==JAVACALL_OK && ok2==JAVACALL_OK && ok3==JAVACALL_OK) ? JAVACALL_OK : JAVACALL_FAIL;
}

/**
 * Finishes UDP WMA emulation.
 * Closes sockets.
 */
javacall_result finalize_wma_emulator() {
    javacall_result ok1, ok2, ok3;

    ok1 = javacall_datagram_close(smsDatagramSocketHandle);
    ok2 = javacall_datagram_close(mmsDatagramSocketHandle);
    ok3 = javacall_datagram_close(cbsDatagramSocketHandle);

    return (ok1==JAVACALL_OK && ok2==JAVACALL_OK && ok3==JAVACALL_OK) ? JAVACALL_OK : JAVACALL_FAIL;
}

static void decodeSmsBuffer(char *buffer, 
    int* encodingType, int* destPortNum, javacall_int64* timeStamp, 
    char** recipientPhone, char** senderPhone, 
    int* msgLength, char** msg) {

    char* ptr = buffer;

    *encodingType   = *((int*)ptr);  ptr += sizeof(int);
    *destPortNum    = *((int*)ptr);  ptr += sizeof(int);
    *timeStamp      = *((javacall_int64*)ptr); ptr += sizeof(javacall_int64);
    *recipientPhone = ptr;           while(*(ptr++) != 0);
    *senderPhone    = ptr;           while(*(ptr++) != 0);
    *msgLength      = *((int*)ptr);  ptr += sizeof(int);
    *msg            = ptr;           ptr += *msgLength;

    *ptr = 0;
}

#define SMS_BUFF_LENGTH 1024
char encode_sms_buffer[SMS_BUFF_LENGTH];

char* encodeSmsBuffer(
    int encodingType, int destPortNum, javacall_int64 timeStamp, 
    const char* recipientPhone, const char* senderPhone, int msgLength, const char* msg,
    int* out_encode_sms_buffer_length) {

    char* ptr = encode_sms_buffer;
    int lngth;

    if (strlen(recipientPhone) + strlen(senderPhone) + msgLength + 64 > SMS_BUFF_LENGTH) {
        javacall_print("Error: too big SMS!");
        *out_encode_sms_buffer_length = 0;
        return encode_sms_buffer;
    }

    *((int*)ptr) = encodingType;         ptr += sizeof(int);
    *((int*)ptr) = destPortNum;          ptr += sizeof(int);
    *((javacall_int64*)ptr) = timeStamp; ptr += sizeof(javacall_int64);
    lngth = strlen(recipientPhone) + 1;  memcpy(ptr, recipientPhone, lngth); ptr += lngth;
    lngth = strlen(senderPhone) + 1;     memcpy(ptr, senderPhone,    lngth); ptr += lngth;
    *((int*)ptr) = msgLength;            ptr += sizeof(int);
    memcpy(ptr, msg, msgLength);         ptr += msgLength;

    *out_encode_sms_buffer_length = ptr - encode_sms_buffer;
    return encode_sms_buffer;
}

void decodeMmsBuffer(char *ptr, 
    char** fromAddress, char** appID, char** replyToAppID,
    int* bodyLen, char** body) {

    short packNum = -1;
    short numPackets = 0;
    short count = 0;
    int totalLen = 0;

    packNum     = *((short*)ptr);  ptr += sizeof(short);
    numPackets  = *((short*)ptr);  ptr += sizeof(short);
    count       = *((short*)ptr);  ptr += sizeof(short);
    totalLen    = *((int*)ptr);    ptr += sizeof(int);
    //packet      = ptr;             ptr += totalLen;

    *fromAddress  = ptr;           while(*(ptr++) != 0);
    *appID        = ptr;           while(*(ptr++) != 0);
    *replyToAppID = ptr;           while(*(ptr++) != 0);
    *bodyLen      = *((int*)ptr);  ptr += sizeof(int);
    *body         = ptr;           ptr += *bodyLen;

    *ptr = 0;
}

extern javacall_result javacall_is_sms_port_registered(unsigned short portNum);
extern javacall_result javacall_is_cbs_msgID_registered(unsigned short portNum);
extern javacall_result javacall_is_mms_appID_registered(const char* appID);

javacall_result process_UDPEmulator_sms_incoming(javacall_handle handle) {

    unsigned char pAddress[256];
    int port;
    char buffer[1024];
    int length = 1024;
    int pBytesRead;
    void *pContext = NULL;

    javacall_sms_encoding   encodingType;
    int                     encodingType_int;
    char*                   sourceAddress;
    char*                   msg;
    int                     msgLen;
    int                     destPortNum;
    javacall_int64          timeStamp;
    char*                   recipientPhone;
    char*                   senderPhone;

    int ok;

    ok = javacall_datagram_recvfrom_start(
        handle, pAddress, &port, buffer, length, &pBytesRead, &pContext);

    sourceAddress = (char*)pAddress;  // currently, sourceAddress = 0x0100007f = 127.0.0.1
    decodeSmsBuffer(buffer, &encodingType_int, &destPortNum, &timeStamp, &recipientPhone, &senderPhone, &msgLen, &msg);

    if (javacall_is_sms_port_registered((unsigned short)destPortNum) != JAVACALL_OK) {
        javacall_print("SMS on unregistered port received!");
        return JAVACALL_FAIL;
    }

    //encodingType = JAVACALL_SMS_MSG_TYPE_ASCII; //## to do: convert encodingType_int->encodingType
    encodingType = encodingType_int;
    javanotify_incoming_sms(encodingType, sourceAddress, msg, msgLen, (unsigned short)port, (unsigned short)destPortNum, timeStamp);

    return JAVACALL_OK;
}

javacall_result process_UDPEmulator_mms_incoming(javacall_handle handle) {

    unsigned char pAddress[256];
    int port;
    char buffer[1024];
    int length = 1024;
    int pBytesRead;
    void *pContext = NULL;
    int ok;

    char* fromAddress = "fromAddress";
    char* appID = "appID";
    char* replyToAppID = "replyToAppID";
    int bodyLen;
    unsigned char* body;

    ok = javacall_datagram_recvfrom_start(
        handle, pAddress, &port, buffer, length, &pBytesRead, &pContext);

    decodeMmsBuffer(buffer, &fromAddress, &appID, &replyToAppID, &bodyLen, &body);

    if (javacall_is_mms_appID_registered(appID) != JAVACALL_OK) {
        javacall_print("MMS on unregistered appID received!");
        return JAVACALL_FAIL;
    }

    javanotify_incoming_mms_singlecall(fromAddress, appID, replyToAppID,
        bodyLen, body);

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

    if (pBytesRead > 12) {
        msgType = *(int*)buffer;
        msgID   = (unsigned short)*(int*)(buffer+4);
        msgBufferLen  = *(int*)(buffer+8);
        msgBuffer = buffer+12;
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
    if (handle == mmsDatagramSocketHandle) {
        process_UDPEmulator_mms_incoming(handle);
        return JAVACALL_OK;
    }
    if (handle == cbsDatagramSocketHandle) {
        process_UDPEmulator_cbs_incoming(handle);
        return JAVACALL_OK;
    }
    return JAVACALL_FAIL;
}
