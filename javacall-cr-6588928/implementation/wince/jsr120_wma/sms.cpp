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

#ifdef __cplusplus
extern "C" {
#endif
    
#include "javacall_sms.h" 
#include "javacall_events.h"
#include "javacall_platform_defs.h"

#include <windows.h>
#include <sms.h>

int SendSMS(LPCTSTR lpszRecipient, 
             const unsigned char*    msgBuffer, 
             int                     msgBufferLen)
{
    SMS_HANDLE smshHandle;
    SMS_ADDRESS smsaSource;
    SMS_ADDRESS smsaDestination;
    TEXT_PROVIDER_SPECIFIC_DATA tpsd;
    SMS_MESSAGE_ID smsmidMessageID;
    BOOL bSendConfirmation = 0;
    BOOL bUseDefaultSMSC = 1;
    LPCTSTR lpszSMSC = L"";

    // try to open an SMS Handle
    if(FAILED(SmsOpen(SMS_MSGTYPE_TEXT, SMS_MODE_SEND, &smshHandle, (HANDLE*)NULL)))
    {
        //MessageBox(NULL, L"FAIL", L"FAIL", MB_OK | MB_ICONERROR);
        return 0;
    }

    // Create the source address
    if(!bUseDefaultSMSC)
    {
        smsaSource.smsatAddressType = SMSAT_INTERNATIONAL;
        _tcsncpy(smsaSource.ptsAddress, lpszSMSC, SMS_MAX_ADDRESS_LENGTH);
    }

    // Create the destination address
    smsaDestination.smsatAddressType = SMSAT_INTERNATIONAL;
    _tcsncpy(smsaDestination.ptsAddress, lpszRecipient, SMS_MAX_ADDRESS_LENGTH);

    // Set up provider specific data
    memset(&tpsd, 0, sizeof(tpsd));
    tpsd.dwMessageOptions = bSendConfirmation ? PS_MESSAGE_OPTION_STATUSREPORT : PS_MESSAGE_OPTION_NONE;
    tpsd.psMessageClass = PS_MESSAGE_CLASS1;
    tpsd.psReplaceOption = PSRO_NONE;
    tpsd.dwHeaderDataSize = 0;

    // Send the message, indicating success or failure
    HRESULT ok = SmsSendMessage(smshHandle, ((bUseDefaultSMSC) ? (SMS_ADDRESS*)NULL : &smsaSource), 
                                 &smsaDestination, (SYSTEMTIME*)NULL, 
                                 msgBuffer, msgBufferLen, 
                                 (PBYTE) &tpsd, sizeof(TEXT_PROVIDER_SPECIFIC_DATA), 
                                 SMSDE_OPTIMAL, 
                                 SMS_OPTION_DELIVERY_NONE, &smsmidMessageID);

    // clean up
    VERIFY(SUCCEEDED(SmsClose(smshHandle)));

    return SUCCEEDED(ok);
}

int ReceiveSMS(BYTE* const receiveBuffer, int max_receive_buffer)
{
    SMS_HANDLE smshHandle;
    TEXT_PROVIDER_SPECIFIC_DATA tpsd;
    HANDLE messageAvailableEvent;
    DWORD bytesRead;

    HRESULT ok = SmsOpen(    SMS_MSGTYPE_TEXT, SMS_MODE_RECEIVE, &smshHandle, &messageAvailableEvent);
    DWORD dw = GetLastError();

    if (ok == SMS_E_RECEIVEHANDLEALREADYOPEN) {
        MessageBox((HWND)NULL, L"Only one handle per provider may be open for read access by any application at one time.", L"SMS_E_RECEIVEHANDLEALREADYOPEN", MB_OK | MB_ICONERROR);
        return 0;
    }

    if(FAILED(ok)) {
        MessageBox((HWND)NULL, L"FAIL", L"FAIL", MB_OK | MB_ICONERROR);
        return 0;
    } else {
        //MessageBox((HWND)NULL, L"SmsOpen:OK", L"SmsOpen:OK", MB_OK | MB_ICONERROR);
    }

    DWORD waitOk = WaitForSingleObject(messageAvailableEvent, 60*1000);    

    if (waitOk == WAIT_OBJECT_0) {

        SMS_ADDRESS psmsaSMSCAddress;
        SMS_ADDRESS psmsaSourceAddress;
        SYSTEMTIME  pstReceiveTime;
        static unsigned char buffer[1024];

        HRESULT readOk = SmsReadMessage (
            smshHandle,
            &psmsaSMSCAddress, &psmsaSourceAddress, &pstReceiveTime,
            receiveBuffer, max_receive_buffer,
            (PBYTE)&tpsd, sizeof(tpsd),
            &bytesRead);
    } else {
        printf("ReceiveSMS: WaitForSingleObject failed\n");
    }

    // clean up
    VERIFY(SUCCEEDED(SmsClose(smshHandle)));

    return bytesRead;
}

/**
 * check if the SMS service is available, and SMS messages can be sent and received
 *
 * @return <tt>JAVACALL_OK</tt> if SMS service is avaialble 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_sms_is_service_available(void) {
    return JAVACALL_FAIL;
}


void int_to_hexascii(int value, char str[4]) {
    int tmp;
    tmp = value        & 0xF; str[3] = tmp>9 ? 'A'+tmp-10 : '0' + tmp;
    tmp = (value >> 4) & 0xF; str[2] = tmp>9 ? 'A'+tmp-10 : '0' + tmp;
    tmp = (value >> 8) & 0xF; str[1] = tmp>9 ? 'A'+tmp-10 : '0' + tmp;
    tmp = (value >> 12)& 0xF; str[0] = tmp>9 ? 'A'+tmp-10 : '0' + tmp;
}

int hexascii_to_int(char* str, int count) {
    char tmp[17];
    int result;
    memcpy(tmp, str, count);
    tmp[count] = 0;
    result = strtol(tmp, 0, 16);
    return result;
}

// 14=5+4+4+1 - "//WMA" + DESTINATION_PORT + SOURCE_PORT + WMA_DELIMITER
// 20=5+4+4+2+2+2+1 - "//WMA" + DESTINATION_PORT + SOURCE_PORT + REFERENCE_NUMBER +
//                    + TOTAL_SEGMENTS + SEGMENT_NUMBER + WMA_DELIMITER
#define SMS_MAX_PAYLOAD 140
#define SMS_MAX_DATA_PAYLOAD (SMS_MAX_PAYLOAD - 14)
#define SMS_MAX_MULTISEGMDATA_PAYLOAD (SMS_MAX_PAYLOAD - 20)

static int calc_segments_num(int msgBufferLen) {
    if (msgBufferLen < SMS_MAX_DATA_PAYLOAD) {
        return 1;
    } else {
        return (msgBufferLen - 1) / SMS_MAX_MULTISEGMDATA_PAYLOAD + 1;
    }
}

/**
 * Returns 0 on error
 */
static int cdma_sms_encode(javacall_sms_encoding   msgType, 
                        const unsigned char*    msgBuffer, 
                        int                     msgBufferLen, 
                        unsigned short          sourcePort, 
                        unsigned short          destPort,
                        char databuf[SMS_MAX_PAYLOAD], int* databuflength) {

    if (msgBufferLen < SMS_MAX_PAYLOAD) {
        memcpy(databuf, "//WMA", 5);
        int_to_hexascii(destPort, databuf+5);
        int_to_hexascii(sourcePort, databuf+5+4);
        databuf[5+4+4] = 0x20; //<space> character, 0x20. Marks the end of text header.
        memcpy(databuf+5+4+4+1, msgBuffer, msgBufferLen);
        *databuflength = 5+4+4+1 + msgBufferLen;
        return 1;
    } else {
        return 0;
    }
}

static int static_reference_number = 0;
static int sms_handle = 0;

/**
 * Returns 0 on error
 */
static int cdma_sms_mulstisegm_encode(javacall_sms_encoding   msgType, 
                        const unsigned char*    msgBuffer, 
                        int                     msgBufferLen, 
                        unsigned short          sourcePort, 
                        unsigned short          destPort,
                        int reference_number, int total_segments, int segment_number,
                        char databuf[SMS_MAX_PAYLOAD],
                        int* databuflength) {

    printf("multi segments SMS is not implemented");
    return 0; //## TODO
    /*
    memcpy(databuf, "//WMA", 5);
    int_to_hexascii(destPort, databuf+5);
    int_to_hexascii(sourcePort, databuf+5+4);

    int_to_hexascii(reference_number, databuf+5+4+4, 2);
    int_to_hexascii(total_segments, databuf+5+4+4+2, 2);
    int_to_hexascii(segment_number, databuf+5+4+4+2+2, 2);
    databuf[5+4+4+2+2+2] = 0x20; //<space> character, 0x20. Marks the end of text header.
    memcpy(databuf+5+4+4+2+2+2+1, msgBuffer, length);
    
    return 1;
    */
}

static int cdma_sms_decode(
        char* databuf, int databuflength,
        javacall_sms_encoding*  msgType, 
        char**         msgBuffer,
        int*                    msgBufferLen,
        unsigned short*         sourcePort, 
        unsigned short*         destPort,
        int* reference_number, int* total_segments, int* segment_number) {

    if (strncmp("//WMA", databuf, 5) != 0) {
        //error
        return 0;
    }

    *msgType = JAVACALL_SMS_MSG_TYPE_ASCII; //##

    *destPort   = hexascii_to_int(databuf+5,   4);
    *sourcePort = hexascii_to_int(databuf+5+4, 4);
    if (databuf[5+4+4] = 0x20) { //<space> character, 0x20. Marks the end of text header.
        *total_segments = 1; //short header, hence there is only one segment
        *msgBuffer = (char*)databuf + 5+4+4+1;
        *msgBufferLen = databuflength - (5+4+4+1);
    } else {
        *reference_number = hexascii_to_int(databuf+5+4+4, 2);
        *total_segments   = hexascii_to_int(databuf+5+4+4+2, 2);
        *segment_number   = hexascii_to_int(databuf+5+4+4+2+2, 2);
        if (databuf[5+4+4+2+2+2] = 0x20) { //<space> character, 0x20. Marks the end of text header.
            *msgBuffer = (char*)databuf + 5+4+4+2+2+2+1;
            *msgBufferLen = databuflength - (5+4+4+2+2+2+1);
        } else {
            //error
            return 0;
        }
    }

    return 1;
}

void fillPhone(const unsigned char* javaDestAddress, wchar_t* lpcPhone) {

    do {
        if (*javaDestAddress != '+' && *javaDestAddress != ' ') {
            *lpcPhone++ = (wchar_t)*javaDestAddress;
        }
    } while (*javaDestAddress++ != 0);
}
/**
 * send an SMS message
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
 * @param destAddress the target SMS address for the message.  The format of the address  parameter  
 *                is  expected to be compliant with MSIDN, for example,. +123456789 
 * @param msgBuffer the message body (payload) to be sent
 * @param msgBufferLen the message body (payload) len
 * @param sourcePort source port of SMS message
 * @param destPort destination port of SMS message where 0 is default destination port 
 * @return handle of sent sms or <tt>0</tt> if unsuccessful
 * 
 * Note: javacall_callback_on_complete_sms_send() needs to be called to notify
 *       completion of sending operation.
 *       The returned handle will be passed to javacall_callback_on_complete_sms_send( ) upon completion
 */
int javacall_sms_send(  javacall_sms_encoding   msgType, 
                        const unsigned char*    destAddress, 
                        const unsigned char*    msgBuffer, 
                        int                     msgBufferLen, 
                        unsigned short          sourcePort, 
                        unsigned short          destPort){

    wchar_t lpcPhone[SMS_MAX_ADDRESS_LENGTH];
    fillPhone(destAddress, lpcPhone);
    //wchar_t* phone = L"79210951475";
    sms_handle++;
    int send_ok = 0;

    if (destPort == 0) {
        send_ok = SendSMS(lpcPhone, msgBuffer, msgBufferLen);
    } else {
        int total_segments = calc_segments_num(msgBufferLen);

        if (total_segments < 1 || total_segments > 3) {
            return 0;
        } else if (total_segments == 1) {
            char smsData[SMS_MAX_PAYLOAD];
            int  smsDataLength;
            int ok = cdma_sms_encode(msgType, msgBuffer, msgBufferLen, sourcePort, destPort, 
                smsData, &smsDataLength);
            if (!ok) {
                return 0;
            }
            send_ok = SendSMS(lpcPhone, (const unsigned char*)smsData, smsDataLength);
        } else {
            char smsData[SMS_MAX_PAYLOAD];
            int  smsDataLength;
            int reference_number = static_reference_number++;
            int segment_number;
            int ok;
            for (segment_number=0; segment_number<total_segments; segment_number++) {
                ok = cdma_sms_mulstisegm_encode(msgType, msgBuffer, msgBufferLen, sourcePort, destPort, 
                    reference_number, total_segments, segment_number, 
                    smsData, &smsDataLength);
                if (!ok) {
                    return 0;
                }
                send_ok = SendSMS(lpcPhone, (const unsigned char*)smsData, smsDataLength);
                if (!send_ok) {
                    return 0;
                }
            }
        }
    }

    return send_ok == 1 ? sms_handle : 0;
}

static int init_done = 0;
javacall_result javacall_wma_init(void);

/**
 * The platform must have the ability to identify the port number of incoming 
 * SMS messages, and deliver messages with port numbers registered to the WMA 
 * implementation.
 * If this port number has already been registered either by a native application 
 * or by another WMA application, then the API should return an error code.
 * 
 * @param portNum port to start listening to
 * @return <tt>JAVACALL_OK</tt> if started listening to port, or 
 *         <tt>JAVACALL_FAIL</tt> or negative value if unsuccessful
 */
javacall_result javacall_sms_add_listening_port(unsigned short portNum){

    //##
    if (!init_done) {
        javacall_wma_init();
        init_done = 1;
    }
    return JAVACALL_OK;
}

/**
 * unregisters a message port number. 
 * After unregistering a port number, SMS messages received by the device for 
 * the specfied port should not be delivered tothe WMA implementation.  
 * If this API specifies a port number which is not registered, then it should 
 * return an error code.
 *
 * @param portNum port to stop listening to
 * @return <tt>JAVACALL_OK </tt> if stopped listening to port, 
 *          or <tt>0</tt> if failed, or port not registered
 */
javacall_result javacall_sms_remove_listening_port(unsigned short portNum){
    return JAVACALL_FAIL;
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
        char*                   msgBuffer, 
        int                     msgBufferLen, 
        javacall_bool           hasPort) {

    printf("javacall_sms_get_number_of_segments\n");

    //##
    if (msgBufferLen > 128)
        return 0; // message is too large

    return 1;
}

#define MAX_RECEIVE_BUFFER 1024

DWORD WINAPI receiveSMSThreadProc( LPVOID lpParam ) {
    while (1) {

        char receiveBuffer[MAX_RECEIVE_BUFFER];
        int numRead = ReceiveSMS((BYTE* const)receiveBuffer, MAX_RECEIVE_BUFFER);

        if (numRead > 0) {
            javacall_sms_encoding  msgType;
            unsigned char*         msgBuffer;
            int                    msgBufferLen;
            unsigned short         sourcePort;
            unsigned short         destPort;
            int reference_number, total_segments, segment_number;

            int decode_ok = cdma_sms_decode(
                receiveBuffer, numRead,
                &msgType, 
                (char**)&msgBuffer, &msgBufferLen,
                &sourcePort, &destPort,
                &reference_number, &total_segments, &segment_number);

            if (decode_ok) {
                char msgAddr[4] = {1,2,3,4};   //##
                javacall_int64 timeStamp = 0;  //##

                javanotify_incoming_sms(
                    msgType,
                    msgAddr, 
                    msgBuffer, msgBufferLen,
                    sourcePort, destPort,
                    timeStamp);
            }
        }
    }
}

void createReceiveSMSThread() {
    DWORD dwJavaThreadId;
    HANDLE hThread = CreateThread(
                      (LPSECURITY_ATTRIBUTES)NULL,              // default security attributes
                      0,                 // use default stack size
                      receiveSMSThreadProc,        // thread function
                      0,                 // argument to thread function
                      0,                 // use default creation flags
                      &dwJavaThreadId);   // returns the thread identifier
}

javacall_result javacall_wma_init(void) {
    createReceiveSMSThread();
    return JAVACALL_OK;
}

javacall_result javacall_wma_close(void) {
    return JAVACALL_OK;
}

#ifdef __cplusplus
}
#endif


