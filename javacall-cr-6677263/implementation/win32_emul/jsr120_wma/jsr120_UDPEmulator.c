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


/**
 * @file
 *
 * Simple implementation of wma UDP Emulator.
 * The messages supposed to be received from JSR205Tool.jar:
 * bash-3.1$ java -jar midp/bin/i386/WMATool.jar -send mms://1234:1234 -message "blah" -multipart -verbose
 * bash-3.1$ java -jar midp/bin/i386/WMATool.jar -send sms://1234:1234 -message "blah"
 * To receive message start:
 * bash-3.1$ java -jar midp/bin/i386/WMATool.jar -receive mms://9800 -verbose
 */

#include <stdlib.h> //getenv
#include <string.h>
#include <stdio.h>

#include "lime.h"

#include "javacall_network.h"
#include "javacall_datagram.h"
#include "javacall_sms.h"
#include "javacall_cbs.h"
#include "javacall_logging.h"
#include "javacall_properties.h"
#include "javacall_defs.h"

#define BOOL_DEFINED 1

#define WMA_CLIENT_PACKAGE "com.sun.kvem.midp.io.j2se.wma.client"
#define WMA_PARSE_PACKAGE "com.sun.kvem.midp.io.j2se.wma.common.parse"

#define SMS_BUFF_LENGTH 1500
char encode_sms_buffer[SMS_BUFF_LENGTH];

javacall_handle smsDatagramSocketHandle = NULL;


void initializeWMASupport();
void tearDownWMASupport();

int smsOutPortNumber;
int smsInPortNumber;
char* devicePhoneNumber;

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

static void decodeSmsBuffer(char *buffer,
int* encodingType, int* destPortNum, int *srcPortNum, javacall_int64* timeStamp,
char** recipientPhone, char** senderPhone,
int* msgLength, char** msg) {

    char* ptr = buffer;
    int segments;
    char * t_pch;
    int pchLen;
    char * pch;

    pch = strtok(ptr, " \n");
	*encodingType  = -1; // Uninitialized

    while (pch != NULL) {

        if (strcmp("Text-Encoding:", pch)==0){
            pch = strtok(NULL, "\n");
            if (strcmp("gsm7bit", pch)==0){
                *encodingType = JAVACALL_SMS_MSG_TYPE_ASCII;       // = 0
            } else if (strcmp("ucs2", pch)==0){
                *encodingType = JAVACALL_SMS_MSG_TYPE_UNICODE_UCS2;// = 2
            }else{
                *encodingType = JAVACALL_SMS_MSG_TYPE_BINARY;      // = 1;
            }
        } else
        if (strcmp("Content-Type:", pch)==0 && (*encodingType  == -1) ){ // We'll give Text-Encoding precedence
            pch = strtok(NULL, "\n");
            if (strcmp("text", pch)==0
            || strcmp("ucs2", pch)==0){
                *encodingType = JAVACALL_SMS_MSG_TYPE_ASCII;  // = 0;
            }else{
                *encodingType = JAVACALL_SMS_MSG_TYPE_BINARY; // = 1
            }
        }else if (strcmp("Content-Length:", pch)==0){
            pch = strtok(NULL, "\n");
            *msgLength = atoi(pch);
        }else if (strcmp("Segments:", pch)==0){
            pch = strtok(NULL, "\n");
            segments = atoi(pch);
        }else if (strcmp("SenderAddress:", pch)==0){
            //Example: 'SenderAddress: sms://+5555555556'
            pch = strtok(NULL, "\n");
            t_pch = strrchr(pch, '/')+1;
            *senderPhone = t_pch;

            t_pch = strrchr(t_pch, ':');
            if( t_pch != NULL ) { // We also have a port in the address
                *srcPortNum = atoi(t_pch+1);
                *t_pch = '\0';
            } else {
                *srcPortNum = 0;
            }
            t_pch = NULL;
        }else if (strcmp("Address:", pch)==0){
            //Example: 'Address: sms://+5555555556:50000'
            pch = strtok(NULL, "\n");

            //find last occurrence of ':'
            t_pch = strrchr(pch, ':');
            *destPortNum = atoi(t_pch+1);
            *t_pch = '\0';
            t_pch = strrchr(pch, '/')+1;
            *recipientPhone = t_pch;

        }else if (strcmp("Date:", pch)==0){
            pch = strtok(NULL, "\n");
            *timeStamp = _atoi64(pch);
        }else if (strcmp("Buffer:", pch)==0){
	       pchLen = strlen("Buffer :\n");
		pch = pch + pchLen;/// strtok(NULL, "\n");

		t_pch = *msg;
		pchLen = *msgLength;
		if (pchLen > SMS_BUFF_LENGTH) {
			pchLen = SMS_BUFF_LENGTH;
		}
		memcpy( t_pch , pch, pchLen);
        }
	// javautil_debug_print (JAVACALL_LOG_INFORMATION, "jsr120_UDPEmulator", "%s\n", pch);
        pch = strtok(NULL, " \n");
    }

    *ptr = 0;
}

char* encodeSmsBuffer(
javacall_sms_encoding encodingType, int destPortNum, int srcPortNum, javacall_int64 timeStamp,
const char* recipientPhone, const char* senderPhone, int msgLength, const char* msg,
int* out_encode_sms_buffer_length) {

    char* ptr = encode_sms_buffer;
    char port[17];
    char msgLen[17];
    char segmentsNum[17];
    int lngth;
    int defaultSegmentsNum = 1;

	if ((senderPhone == NULL) || (recipientPhone == NULL) || (msg == NULL)) {
		javautil_debug_print (JAVACALL_LOG_ERROR, "jsr120_UDPEmulator", "NULL Parameters to encodeSmsBuffer");
		return NULL;
	}

    if (strlen(recipientPhone) + strlen(senderPhone) + msgLength + 64 > SMS_BUFF_LENGTH) {
	 javautil_debug_print (JAVACALL_LOG_ERROR, "jsr120_UDPEmulator", "Error: too big SMS!");
        *out_encode_sms_buffer_length = 0;
        return encode_sms_buffer;
    }


    //adding the Content-Type
    memset(encode_sms_buffer, 0, SMS_BUFF_LENGTH);

    switch(encodingType) {
        case JAVACALL_SMS_MSG_TYPE_ASCII:
                strcat(encode_sms_buffer, "Content-Type: text\n");
                strcat(encode_sms_buffer, "Text-Encoding: gsm7bit\n");
        break;
        case JAVACALL_SMS_MSG_TYPE_UNICODE_UCS2:
                strcat(encode_sms_buffer, "Content-Type: text\n");
                strcat(encode_sms_buffer, "Text-Encoding: ucs2\n");
        break;
        case JAVACALL_SMS_MSG_TYPE_BINARY:
                strcat(encode_sms_buffer, "Content-Type: binary\n");
        break;
    }

    //adding ADDRESS_HEADER
    strcat(encode_sms_buffer, "Address: sms://");
    //add the recipient phone
    strcat(encode_sms_buffer, recipientPhone);
    strcat(encode_sms_buffer, ":");
    //add port number
    itoa(destPortNum, port, 10);
    strcat(encode_sms_buffer, port);
    strcat(encode_sms_buffer, "\n");

    //adding date-stamp
    strcat(encode_sms_buffer, "Date: ");
	_i64toa(timeStamp,encode_sms_buffer+strlen(encode_sms_buffer),10);
    strcat(encode_sms_buffer, "\n");

    //adding sender address
    strcat(encode_sms_buffer, "SenderAddress: sms://");
    strcat(encode_sms_buffer, senderPhone);
    if (srcPortNum > 0) {
        strcat(encode_sms_buffer, ":");
        //add port number
        itoa(srcPortNum, port, 10);
        strcat(encode_sms_buffer, port);
    }
    strcat(encode_sms_buffer, "\n");


    strcat(encode_sms_buffer, "Segments: ");
    //num of segments is 1 by default
    itoa(defaultSegmentsNum, segmentsNum, 10);
    strcat(encode_sms_buffer, segmentsNum);
    strcat(encode_sms_buffer, "\n");


    strcat(encode_sms_buffer, "Content-Length: ");
    itoa(msgLength, msgLen, 10);
    strcat(encode_sms_buffer, msgLen);
    strcat(encode_sms_buffer, "\n");

    //adding the message - must be at te end of the payload buffer
    strcat(encode_sms_buffer, "Buffer: \n");
      //calculate the length of the payload
    lngth = strlen(encode_sms_buffer) + msgLength;

    *out_encode_sms_buffer_length = lngth ;

 memcpy(&encode_sms_buffer[lngth-msgLength], msg,msgLength);
    return encode_sms_buffer;
}

extern javacall_result javacall_is_sms_port_registered(unsigned short portNum);
extern javacall_result javacall_is_cbs_msgID_registered(unsigned short portNum);

javacall_result process_UDPEmulator_sms_incoming(unsigned char *pAddress,
int *port,
char *buffer,
int length,
int *pBytesRead,
void **pContext) {

    //    unsigned char pAddress[256];
    //    int port;
    //    char buffer[1024];
    //    int length = 1024;
    //    int pBytesRead;
    //    void *pContext = NULL;

    javacall_sms_encoding   encodingType;
    int                     encodingType_int = 0;
    char*                   sourceAddress = NULL;
    char*                   msg = NULL;
    int                     msgLen = 0;
    int                     destPortNum = 0;
    int                     srcPortNum = 0;
    javacall_int64          timeStamp = 0;
    char*                   recipientPhone = NULL;
    char*                   senderPhone = NULL;

    msg = (char *) malloc(SMS_BUFF_LENGTH);
    memset(msg, 0, SMS_BUFF_LENGTH);

    sourceAddress = (char*)pAddress;  // currently, sourceAddress = 0x0100007f = 127.0.0.1
    decodeSmsBuffer(buffer,
		                    &encodingType_int,
		                    &destPortNum,
		                    &srcPortNum,
		                    &timeStamp,
		                    &recipientPhone,
		                    &senderPhone,
		                    &msgLen,
		                    &msg);


    if (javacall_is_sms_port_registered((unsigned short)destPortNum) != JAVACALL_OK) {
		javautil_debug_print (JAVACALL_LOG_INFORMATION, "jsr120_UDPEmulator", "SMS on unregistered port received!");
		free(msg);
        return JAVACALL_FAIL;
    }

    //encodingType = JAVACALL_SMS_MSG_TYPE_ASCII; //## to do: convert encodingType_int->encodingType
    encodingType = encodingType_int;
    javanotify_incoming_sms(encodingType, senderPhone, msg, msgLen, (unsigned short)srcPortNum, (unsigned short)destPortNum, timeStamp);
    free(msg);
    return JAVACALL_OK;
}

javacall_result process_UDPEmulator_cbs_incoming(unsigned char *pAddress,
int *port,
char *buffer,
int length,
int *pBytesRead,
void **pContext) {

    javacall_cbs_encoding  msgType = JAVACALL_CBS_MSG_TYPE_ASCII;
    unsigned short         msgID = 13;
    unsigned char*         msgBuffer = "msgBuffer";
    int                    msgBufferLen = 9; //strlen(msgBuffer);

    char* ptr = buffer;
    int segments;
    char *address;
    char * t_pch;


    if (*pBytesRead > 12) {

        char * pch;
        pch = strtok(ptr, " \n");

        while (pch != NULL) {
            if (strcmp("Text-Encoding:", pch)==0){
                pch = strtok(NULL, "\n");
                if (strcmp("gsm7bit", pch)==0){
                    msgType = JAVACALL_CBS_MSG_TYPE_ASCII;
                }else if (strcmp("ucs2", pch)==0){
                    msgType = JAVACALL_CBS_MSG_TYPE_UNICODE_UCS2;
                }else{
                    msgType = JAVACALL_CBS_MSG_TYPE_BINARY;
                }
            }else if (strcmp("Content-Type:", pch)==0){
                pch = strtok(NULL, "\n");
                if (strcmp("text", pch)==0){
                    msgType = JAVACALL_CBS_MSG_TYPE_ASCII;
                }else{
                    msgType = JAVACALL_CBS_MSG_TYPE_BINARY;
                }
            }else if (strcmp("Content-Length:", pch)==0){
                pch = strtok(NULL, "\n");
                msgBufferLen = atoi(pch);
            }else if (strcmp("Segments:", pch)==0){
                pch = strtok(NULL, "\n");
                segments = atoi(pch);
            }else if (strcmp("CBSAddress:", pch)==0){
                //Example: 'CBSAddress: cbs://:50001'
                pch = strtok(NULL, "\n");
                t_pch = strrchr(pch, ':')+1;
                msgID = (short)atoi(t_pch);
                t_pch = NULL;
            }else if (strcmp("Address:", pch)==0){
                //Example: 'Address: 24680'
                pch = strtok(NULL, "\n");
                address = pch;
            }else if (strcmp("Buffer:", pch)==0){
                msgBuffer = pch+strlen("Buffer: \n");
                pch = NULL;
                break;
            }
 	   //  javautil_debug_print (JAVACALL_LOG_INFORMATION, "jsr120_UDPEmulator", "%s\n", pch);
            pch = strtok(NULL, " \n");
        }

        *ptr = 0;
    } else {
	 javautil_debug_print (JAVACALL_LOG_ERROR, "jsr120_UDPEmulator", "bad cbs package received");
    }

    if (javacall_is_cbs_msgID_registered(msgID) != JAVACALL_OK) {
	 javautil_debug_print (JAVACALL_LOG_INFORMATION, "jsr120_UDPEmulator", "CBS on unregistered msgID received!");
        return JAVACALL_FAIL;
    }

    javanotify_incoming_cbs(msgType, msgID, msgBuffer, msgBufferLen);

    return JAVACALL_OK;
}

#if (ENABLE_JSR_205)
extern javacall_result process_UDPEmulator_mms_incoming(javacall_handle handle,
                                                        unsigned char *pAddress,
                                                        int *port,
                                                        unsigned char *buffer,
                                                        int length,
                                                        int *pBytesRead,
                                                        void **pContext);
#endif

/**
 * Starts UDP WMA emulation.
 * Opens sockets.
 */
javacall_result init_wma_emulator() {
    javacall_result ok = JAVACALL_OK;
    javacall_result ok1;

// Open using LimeCall
initializeWMASupport();

ok1 = javacall_datagram_open(smsInPortNumber, &smsDatagramSocketHandle);
if (ok1 == JAVACALL_OK) { ok = JAVACALL_FAIL; }

return ok;
}

/**
 * Finishes UDP WMA emulation.
 * Closes sockets.
 */
javacall_result finalize_wma_emulator() {
    javacall_result ok1;

    ok1 = javacall_datagram_close(smsDatagramSocketHandle);
tearDownWMASupport();

return ok1;
}

/**
 * Checks if the handle is of wma_emulator sockets.
 *   returns JAVACALL_FAIL for mismatch
 *   returns JAVACALL_OK for proper sockets and processes the emulation
 */
javacall_result try_process_wma_emulator(javacall_handle handle) {
    javacall_result result;
    unsigned char pAddress[256];
    int port;
    char buffer[2048];
    int length = 2048;
    int pBytesRead;
    void *pContext = NULL;

        int ok;

        if ( handle != smsDatagramSocketHandle) {
            // Not WMA message (Regular Datagram)
            return JAVACALL_FAIL;
        }

        ok = javacall_datagram_recvfrom_start(
        handle, pAddress, &port, buffer, length, &pBytesRead, &pContext);
        if (strstr(buffer, "sms://")!= NULL){ ///vrifies buffer starts with "sms"
  	     javautil_debug_print (JAVACALL_LOG_INFORMATION, "jsr120_UDPEmulator", "This is an SMS message");
            result = process_UDPEmulator_sms_incoming(pAddress, &port, buffer,
            length, &pBytesRead, &pContext);
            if (result == JAVACALL_OK){
                return JAVACALL_OK;
            }
        }else if(strstr(buffer, "cbs://")!= NULL){ ///vrifies buffer starts with "cbs"
  	     javautil_debug_print (JAVACALL_LOG_INFORMATION, "jsr120_UDPEmulator", "This is an CBS message");
            result = process_UDPEmulator_cbs_incoming(pAddress, &port, buffer,
            length, &pBytesRead, &pContext);
            if (result == JAVACALL_OK){
                return JAVACALL_OK;
            }
        }

#if (ENABLE_JSR_205)
    if (strstr(buffer, "mms://")) {
        process_UDPEmulator_mms_incoming(handle,pAddress, &port, buffer,
            length, &pBytesRead, &pContext);
        return JAVACALL_OK;
    }
#endif
    return JAVACALL_FAIL;

}


void initializeWMASupport() {
    int vmArgc=0;
  //  char *vmArgv[3]; /* CLDC parameters */

    LimeFunction *initWMA = NULL;

    char* resultBytes = NULL;
    int resultLen = 0;

    initWMA = NewLimeFunction(WMA_CLIENT_PACKAGE,
    "WMAClientBridge",
    "initializeWMASupport");

    initWMA->call(initWMA, &resultBytes, &resultLen);
    if (resultLen < 3) {
        /* it's too small to be a valid response */
    }
    else {
        char* resultStr = NULL;
        char* clientTrafficPort = NULL;
        char* serverTrafficPort = NULL;
        char* phoneNum = NULL;

        char* newlinePtr = NULL;
        char* keyPtr = NULL;
        char* valuePtr = NULL;

        /* make the result null-terminated */
        resultStr = (char*)malloc(resultLen + 1);
        resultStr[resultLen] = '\0';
        memcpy(resultStr, resultBytes, resultLen);

        /* break up the result string into its 3 pieces */
        newlinePtr = strrchr(resultStr, '\n');
        phoneNum = newlinePtr + 1;
        *newlinePtr = '\0';
        newlinePtr = strrchr(resultStr, '\n');
        serverTrafficPort = newlinePtr + 1;
        *newlinePtr = '\0';
        clientTrafficPort = resultStr;

        smsInPortNumber = atoi(clientTrafficPort);
        smsOutPortNumber = atoi(serverTrafficPort);

        devicePhoneNumber = strdup(phoneNum);
        javacall_set_property("com.sun.midp.io.j2me.sms.PhoneNumber",
                                          devicePhoneNumber,
                                          JAVACALL_TRUE,
                                          JAVACALL_APPLICATION_PROPERTY);


        free(resultStr);
    }
    DeleteLimeFunction(initWMA);


}

void tearDownWMASupport() {
    LimeFunction *killWMA = NULL;

    int result = 0;

    killWMA = NewLimeFunction(WMA_CLIENT_PACKAGE,
    "WMAClientBridge",
    "cleanupWMASupport");

    /* result is only for synchronization so it doesn't run ansynchronously */
    killWMA->call(killWMA, &result);

    DeleteLimeFunction(killWMA);

	free(devicePhoneNumber);
}
