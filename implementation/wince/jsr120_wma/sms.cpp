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
//#include "javacall_events.h"
#include "javacall_platform_defs.h"

#include <windows.h>
  //#include "cemapi.h" //CreateFileMapping, CreateMutex, CreateEvent
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

    SYSTEMTIME sysTime = {0};
    GetSystemTime(&sysTime);

    // Send the message, indicating success or failure
    HRESULT ok = SmsSendMessage(smshHandle, ((bUseDefaultSMSC) ? (SMS_ADDRESS*)NULL : &smsaSource), 
                                 &smsaDestination, &sysTime, 
                                 msgBuffer, msgBufferLen, 
                                 (PBYTE) &tpsd, sizeof(TEXT_PROVIDER_SPECIFIC_DATA), 
                                 SMSDE_OPTIMAL, 
                                 SMS_OPTION_DELIVERY_NONE, &smsmidMessageID);

    //0x82000101 = SMS_E_INVALIDDATA

    // clean up
    VERIFY(SUCCEEDED(SmsClose(smshHandle))); 

    return SUCCEEDED(ok);
}

//deprecated interface usage. 
//needs workaround: 
//  to handle the message SmsOpen func should start first time before 
//  standard mail client tries to do the same - in this case it will 
//  fall down and leave us to input all SMS messages :)
int ReceiveSMS(BYTE* const receiveBuffer, int max_receive_buffer, 
               SMS_ADDRESS* smsaSourceAddress, SYSTEMTIME* stReceiveTime)
{
    SMS_HANDLE smshHandle;
    TEXT_PROVIDER_SPECIFIC_DATA tpsd;
    HANDLE messageAvailableEvent;
    DWORD bytesRead = 0;

    HRESULT ok = SmsOpen(SMS_MSGTYPE_TEXT, SMS_MODE_RECEIVE, &smshHandle, &messageAvailableEvent);
    DWORD dw = GetLastError();

    if (ok == SMS_E_RECEIVEHANDLEALREADYOPEN) {
        MessageBox((HWND)NULL, 
            L"Only one handle per provider may be open for read access by any application at one time.", 
            L"SMS_E_RECEIVEHANDLEALREADYOPEN", MB_OK | MB_ICONERROR);
        return 0;
    }

    if(FAILED(ok)) {
        //MessageBox((HWND)NULL, L"FAIL", L"FAIL", MB_OK | MB_ICONERROR);
        return 0;
    } else {
        //MessageBox((HWND)NULL, L"SmsOpen:OK", L"SmsOpen:OK", MB_OK | MB_ICONERROR);
    }

    DWORD waitOk = WaitForSingleObject(messageAvailableEvent, INFINITE);    

    if (waitOk == WAIT_OBJECT_0) {

        SMS_ADDRESS psmsaSMSCAddress;
        static unsigned char buffer[1024];

        HRESULT readOk = SmsReadMessage (
            smshHandle,
            &psmsaSMSCAddress, smsaSourceAddress, stReceiveTime,
            receiveBuffer, max_receive_buffer,
            (PBYTE)&tpsd, sizeof(tpsd),
            &bytesRead);

        //fix. wince returns doubled array
        //for (DWORD i=0; i<bytesRead; i++) { receiveBuffer[i] = receiveBuffer[i*2]; }
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

void int_to_hexascii2(int value, char str[2]) {
    int tmp;
    tmp =  value      & 0xF; str[1] = tmp>9 ? 'A'+tmp-10 : '0' + tmp;
    tmp = (value >> 4)& 0xF; str[0] = tmp>9 ? 'A'+tmp-10 : '0' + tmp;
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

// "//WMA" + DESTINATION_PORT + SOURCE_PORT + WMA_DELIMITER 
// + msgType + datagramLength + oddityByte
#define SINGLE_SEGM_HEADER_SIZE (5+4+4+1+1+1+1)

// "//WMA" + DESTINATION_PORT + SOURCE_PORT + REFERENCE_NUMBER +
// + TOTAL_SEGMENTS + SEGMENT_NUMBER + WMA_DELIMITER
// + msgType + datagramLength + oddityByte
#define MULTIPLE_SEGM_HEADER_SIZE (5+4+4+2+2+2+1+1+1+1)

#define MAX_SEGM_NUMBER 3
#define SMS_MAX_PAYLOAD SMS_DATAGRAM_SIZE
#define SMS_MAX_DATA_PAYLOAD (SMS_MAX_PAYLOAD - SINGLE_SEGM_HEADER_SIZE)
#define SMS_MAX_MULTISEGMDATA_PAYLOAD (SMS_MAX_PAYLOAD - MULTIPLE_SEGM_HEADER_SIZE)

static int calc_segments_num(int msgBufferLen) {
    if (msgBufferLen < SMS_MAX_DATA_PAYLOAD) {
        return 1;
    } else {
        return (msgBufferLen - 1) / SMS_MAX_MULTISEGMDATA_PAYLOAD + 1;
    }
}

#define SMS_SEND_ODDITY_BIT 0x80

/**
 * Returns 0 on error
 */
static int cdma_sms_encode(javacall_sms_encoding   msgType, 
                        const unsigned char*    msgBuffer, 
                        int                     msgBufferLen, 
                        unsigned short          sourcePort, 
                        unsigned short          destPort,
                        char databuf[SMS_MAX_PAYLOAD], int* databuflength) {

    int header_size;

    if (msgBufferLen >= SMS_MAX_DATA_PAYLOAD) {
        return 0;
    }

    memcpy(databuf, "//WMA", 5);
    int_to_hexascii(destPort, databuf+5);
    int_to_hexascii(sourcePort, databuf+5+4);
    databuf[5+4+4] = 0x20; //<space> character, 0x20. Marks the end of text header.

    databuf[5+4+4+1] = msgType; //the msgType parameter is not specified in documentation.
    databuf[5+4+4+1+1] = 0;     //the datagramSize parameter is not specified in documentation.
    header_size = 5+4+4+1+1+1;

    if ((header_size + msgBufferLen) & 0x01) { //hack. SmsSendMessage fails if bytes number is odd :(
        databuf[5+4+4+1] |= SMS_SEND_ODDITY_BIT;
        header_size++;
    }

    memcpy(databuf+header_size, msgBuffer, msgBufferLen);
    *databuflength = header_size + msgBufferLen;
    databuf[5+4+4+1+1] = *databuflength;
    return 1;
}

static int static_reference_number = 0;

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

    int header_size;
    int data_length;

    memcpy(databuf, "//WMA", 5);

    int_to_hexascii(destPort, databuf+5);
    int_to_hexascii(sourcePort, databuf+5+4);

    int_to_hexascii2(reference_number, databuf+5+4+4);
    int_to_hexascii2(total_segments, databuf+5+4+4+2);
    int_to_hexascii2(segment_number, databuf+5+4+4+2+2);

    databuf[5+4+4+2+2+2] = 0x20; //<space> character, 0x20. Marks the end of text header.

    databuf[5+4+4+2+2+2+1] = msgType; //the msgType parameter is not specified in documentation.
    databuf[5+4+4+2+2+2+1+1] = 0;     //the datagramSize parameter is not specified in documentation.
    header_size = 5+4+4+2+2+2+1+1+1;

    data_length = msgBufferLen - segment_number*SMS_MAX_MULTISEGMDATA_PAYLOAD;
    data_length = (data_length > SMS_MAX_MULTISEGMDATA_PAYLOAD) ? SMS_MAX_MULTISEGMDATA_PAYLOAD : data_length;

    if ((header_size + data_length) & 0x01) { //hack. SmsSendMessage fails if bytes number is odd :(
        databuf[5+4+4+2+2+2+1] |= SMS_SEND_ODDITY_BIT;
        header_size++;
    }

    memcpy(databuf+header_size, msgBuffer + segment_number*SMS_MAX_MULTISEGMDATA_PAYLOAD, data_length);
    *databuflength = header_size + data_length;
    databuf[5+4+4+2+2+2+1+1] = *databuflength;
    return 1;
}

static int cdma_sms_decode(
        char* databuf, int databuflength,
        javacall_sms_encoding*  msgType, 
        char**         msgBuffer,
        int*                    msgBufferLen,
        unsigned short*         sourcePort, 
        unsigned short*         destPort,
        int* reference_number, int* total_segments, int* segment_number) {

    int header_size;
    char msgType1;
    unsigned char datagramSize;

    if (strncmp("//WMA", databuf, 5) != 0) {
        //error
        return 0;
    }

    *destPort   = hexascii_to_int(databuf+5,   4);
    *sourcePort = hexascii_to_int(databuf+5+4, 4);
    if (databuf[5+4+4] == 0x20) { //<space> character, 0x20. Marks the end of text header.
        *total_segments = 1; //short header, hence there is only one segment
        msgType1 = databuf[5+4+4+1];
        datagramSize = databuf[5+4+4+1+1];
        header_size = 5+4+4+1+1+1;
        if (msgType1 & SMS_SEND_ODDITY_BIT) {
            msgType1 &= ~SMS_SEND_ODDITY_BIT;
            header_size++;
        }
        *msgType = (javacall_sms_encoding)msgType1;
        *msgBuffer = (char*)databuf + header_size;
        *msgBufferLen = databuflength - (header_size);
    } else {
        *reference_number = hexascii_to_int(databuf+5+4+4, 2);
        *total_segments   = hexascii_to_int(databuf+5+4+4+2, 2);
        *segment_number   = hexascii_to_int(databuf+5+4+4+2+2, 2);
        if (databuf[5+4+4+2+2+2] = 0x20) { //<space> character, 0x20. Marks the end of text header.
            msgType1 = databuf[5+4+4+2+2+2+1];
            datagramSize = databuf[5+4+4+2+2+2+1+1];
            header_size = 5+4+4+2+2+2+1+1+1;
            if (msgType1 & SMS_SEND_ODDITY_BIT) {
                msgType1 &= ~SMS_SEND_ODDITY_BIT;
                header_size++;
            }
            *msgType = (javacall_sms_encoding)msgType1;
            *msgBuffer = (char*)databuf + header_size;
            *msgBufferLen = databuflength - header_size;
        } else {
            //error
            return 0;
        }
    }

    return 1;
}

static int fullMsg_reference_number = -1;
static int fullMsg_segm_count = -1;
static int fullMsg_buffer_len = 0;
static unsigned char fullMsg_buffer[MAX_SEGM_NUMBER*SMS_MAX_MULTISEGMDATA_PAYLOAD];

static void fullMsg_sms_segement_init() {
    fullMsg_reference_number = -1;
    fullMsg_segm_count = -1;
    fullMsg_buffer_len = 0;
}

static int fullMsg_add_sms_segment(
    unsigned short sourcePort, unsigned short destPort,
    int reference_number, int total_segments, int segment_number,
    javacall_sms_encoding msgType, 
    unsigned char* msgBuffer, int msgBufferLen) {

    if (fullMsg_reference_number == -1) { //first segment received
        fullMsg_reference_number = reference_number;
        fullMsg_segm_count = total_segments;
        fullMsg_buffer_len = 0;
    } else {
        if (fullMsg_reference_number != reference_number) {
            //error.
            return 0;
        }
    }
    fullMsg_segm_count--;
    fullMsg_buffer_len += msgBufferLen;
    memcpy(fullMsg_buffer + segment_number*SMS_MAX_MULTISEGMDATA_PAYLOAD, msgBuffer, msgBufferLen);
    return 1;
}

static int fullMsg_try_get_full_message(unsigned char** fullMsgBuffer, int* fullMsgBufferLen) {
    if (fullMsg_segm_count == 0) {
        *fullMsgBuffer = fullMsg_buffer;
        *fullMsgBufferLen = fullMsg_buffer_len;

        fullMsg_sms_segement_init();
        return 1;
    } else {
        return 0;
    }
}

static void fillPhone(const unsigned char* javaDestAddress, wchar_t* lpcPhone) {

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
javacall_result javacall_sms_send(  javacall_sms_encoding   msgType, 
                        const unsigned char*    destAddress, 
                        const unsigned char*    msgBuffer, 
                        int                     msgBufferLen, 
                        unsigned short          sourcePort, 
                        unsigned short          destPort,
                        int handle){

    wchar_t lpcPhone[SMS_MAX_ADDRESS_LENGTH];
    fillPhone(destAddress, lpcPhone);
    //wchar_t* phone = L"79210951475";
    int send_ok = 0;

    if (destPort == 0) {
        send_ok = SendSMS(lpcPhone, msgBuffer, msgBufferLen);
    } else {
        int total_segments = calc_segments_num(msgBufferLen);

        if (total_segments < 1 || total_segments > 3) {
            return JAVACALL_FAIL;
        } else if (total_segments == 1) {
            char smsData[SMS_MAX_PAYLOAD];
            int  smsDataLength;
            int ok = cdma_sms_encode(msgType, msgBuffer, msgBufferLen, sourcePort, destPort, 
                smsData, &smsDataLength);
            if (!ok) {
                return JAVACALL_FAIL;
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
                    return JAVACALL_FAIL;
                }
                send_ok = SendSMS(lpcPhone, (const unsigned char*)smsData, smsDataLength);

                if (!send_ok) {
                    return JAVACALL_FAIL;
                }
            }
        }
    }

    return send_ok == 1 ? JAVACALL_OK : JAVACALL_FAIL;
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

    //## msgType ?
    int result = calc_segments_num(msgBufferLen);

    return result;
}

#define MAX_RECEIVE_BUFFER 1024

static void decode_and_notify(char* receiveBuffer, int buferLength,
                       wchar_t* senderPhone, javacall_int64 timeStamp) {

            javacall_sms_encoding  msgType;
            unsigned char*         msgBuffer;
            int                    msgBufferLen;
            unsigned short         sourcePort;
            unsigned short         destPort;
            int reference_number, total_segments, segment_number;

            int decode_ok = cdma_sms_decode(
                receiveBuffer, buferLength,
                &msgType, 
                (char**)&msgBuffer, &msgBufferLen,
                &sourcePort, &destPort,
                &reference_number, &total_segments, &segment_number);

            if (decode_ok) {

                char msgAddr[64] = {'s','m','s',':','/','/'};
                if (isdigit(senderPhone[0])) {
                    if (sourcePort == 0) {
                        sprintf(msgAddr, "sms://%S", senderPhone);
                    } else {
                        sprintf(msgAddr, "sms://%S:%i", senderPhone, sourcePort);
                    }
                }

                if (total_segments > 1) {
                    fullMsg_add_sms_segment(
                        sourcePort, destPort,
                        reference_number, total_segments, segment_number,
                        msgType, 
                        msgBuffer, msgBufferLen);
                    unsigned char* fullMsgBufferPtr;
                    int fullMsgBufferLen;
                    if (fullMsg_try_get_full_message(&fullMsgBufferPtr, &fullMsgBufferLen)) {
                        javanotify_incoming_sms(
                            msgType,
                            msgAddr, 
                            fullMsgBufferPtr, fullMsgBufferLen,
                            sourcePort, destPort,
                            timeStamp);
                    }
                } else {
                    javanotify_incoming_sms(
                        msgType,
                        msgAddr, 
                        msgBuffer, msgBufferLen,
                        sourcePort, destPort,
                        timeStamp);
                }
            }
}

static javacall_int64 getTimeStampFromSystemTime(SYSTEMTIME* stReceiveTime) {

    javacall_int64 timeStamp = 
        ((javacall_int64)stReceiveTime->wYear         << 48) +
        ((javacall_int64)stReceiveTime->wMonth        << 40) +
        ((javacall_int64)stReceiveTime->wDay          << 32) +
        ((javacall_int64)stReceiveTime->wHour         << 24) +
        ((javacall_int64)stReceiveTime->wMinute       << 16) +
        ((javacall_int64)stReceiveTime->wSecond       << 8)  +
        ((javacall_int64)stReceiveTime->wMilliseconds);

    return timeStamp;
}

#ifdef USE_DEPRECATED_SMS_RECEIVE_API

DWORD WINAPI receiveSMSThreadProc_deprecated(LPVOID lpParam) {
    while (1) {

        char receiveBuffer[MAX_RECEIVE_BUFFER];
        SMS_ADDRESS smsaSourceAddress;
        SYSTEMTIME stReceiveTime;

        int numRead = ReceiveSMS(
            (BYTE* const)receiveBuffer, MAX_RECEIVE_BUFFER, 
            &smsaSourceAddress, &stReceiveTime);

        if (numRead > 0) {

            //spec requires send time, but this value is not accessible
            javacall_int64 timeStamp = getTimeStampFromSystemTime(&stReceiveTime);

            decode_and_notify(receiveBuffer, numRead,
                              smsaSourceAddress.ptsAddress, timeStamp);

        }
    }
}

#else

#define MAX_SENDER_PHONE_LENGTH 30
static wchar_t _senderPhoneBuffer[MAX_SENDER_PHONE_LENGTH];

/**
 * Returns the last digits sequence from the senderPhoneEmail string
 */
static wchar_t* getSenderPhone(wchar_t* senderPhoneEmail) {
    //PR_SENDER_EMAIL_ADDRESS: senderPhone = Device9 <+79217625673>
    //PR_SENDER_NAME :         senderPhone = Device9

    wchar_t* ptr1 = senderPhoneEmail;
    wchar_t* ptr2 = _senderPhoneBuffer;

    char flag = 1;

    while (*ptr1 != 0) {
        if (isdigit(*ptr1)) { 
            if (flag == 1) {
                ptr2 = _senderPhoneBuffer;
            }
            *ptr2++ = *ptr1; 
            flag = 0;
        } else {
            flag = 1;
        }
        if (ptr2 == _senderPhoneBuffer + MAX_SENDER_PHONE_LENGTH - 1) {
            break;
        }
        ptr1++;
    }
    *ptr2 = 0;

    return _senderPhoneBuffer;
}

#define lpFileMappingName L"jsms_temp_file"
#define lpFileMutex L"jsms_mutex"
#define lpFileEvent L"jsms_event"

#define SENDERPHONE_MAX_LENGTH  52
#define DATAGRAM_MAX_LENGTH     160

#define FILE_OFFSET_MSG_SIZE    0
#define FILE_OFFSET_SENDERPHONE 4
#define FILE_OFFSET_SENDTIME    56
#define FILE_OFFSET_DATAGRAM    64

#define SMS_MAPFILE_SIZE        224

DWORD WINAPI receiveSMSThreadProc(LPVOID lpParam) {

    HANDLE pMutex = CreateMutex((LPSECURITY_ATTRIBUTES)NULL, FALSE, lpFileMutex);

    HANDLE hFileMap = CreateFileMapping(
        INVALID_HANDLE_VALUE, 
        (LPSECURITY_ATTRIBUTES)NULL,
        PAGE_READWRITE,
        0, SMS_MAPFILE_SIZE,
        lpFileMappingName); 

    char* pFileMemory;
    if (hFileMap) {
        pFileMemory = (CHAR*)MapViewOfFile(
            hFileMap,
            FILE_MAP_READ,
            0,0,0);
        if (pFileMemory == NULL) {
            printf("Error mapping sms file\n");
            return 0;
        }
    } else {
        printf("Error mapping sms file\n");
        return 0;
    }

    HANDLE evnt = CreateEvent((LPSECURITY_ATTRIBUTES)NULL, FALSE, FALSE, lpFileEvent);
    
    //infinite cycle of waiting for SMS
    do {
        DWORD waitEventOk = WaitForSingleObject(evnt, INFINITE); 
        if (waitEventOk == WAIT_OBJECT_0) {
            ResetEvent(evnt);

            DWORD waitMutexOk = WaitForSingleObject(pMutex, INFINITE); 
            if (waitEventOk == WAIT_OBJECT_0) {

                //reading the buffer.
                //Note! Do it fast! Otherwise next SMS could be lost because: 
                // - jsms.dll is blocked be the same pMutex for pFileMemory access!
                // - jsms.dll could not wait long because it is running in Windows system process!

                int bufferSize = *((int*)(pFileMemory + FILE_OFFSET_MSG_SIZE));
                wchar_t* senderPhoneEmail = (wchar_t*)(pFileMemory + FILE_OFFSET_SENDERPHONE);
                char* receiveBuffer = pFileMemory + FILE_OFFSET_DATAGRAM;

                FILETIME sendTime_f;
                sendTime_f.dwHighDateTime = *((DWORD*)(pFileMemory + FILE_OFFSET_SENDTIME));
                sendTime_f.dwLowDateTime  = *((DWORD*)(pFileMemory + FILE_OFFSET_SENDTIME+4));

                SYSTEMTIME sendTime_sys;
                FileTimeToSystemTime(&sendTime_f, &sendTime_sys);
                javacall_int64 timeStamp = getTimeStampFromSystemTime(&sendTime_sys);

                //PR_SENDER_EMAIL_ADDRESS: senderPhone = Device9 <+79217625673>
                //PR_SENDER_NAME :         senderPhone = Device9
                wchar_t* senderPhone = getSenderPhone(senderPhoneEmail);

                decode_and_notify(receiveBuffer, bufferSize,
                                  senderPhone, timeStamp);
            }
            ReleaseMutex(pMutex);
        }
    } while (1);

    UnmapViewOfFile(pFileMemory);
    CloseHandle(evnt);
}

#endif

static void createReceiveSMSThread() {
    DWORD dwJavaThreadId;
    HANDLE hThread = CreateThread(
                      (LPSECURITY_ATTRIBUTES)NULL,   // default security attributes
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


