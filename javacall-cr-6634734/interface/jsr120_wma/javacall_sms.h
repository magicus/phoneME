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

#ifndef __JAVACALL_SMS_H
#define __JAVACALL_SMS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file javacall_sms.h
 * @ingroup WMA
 * @brief Javacall interfaces for SMS
 */
 
#include "javacall_defs.h" 

/**
 * @defgroup WMA WMA API
 * @ingroup JTWI
 *
 * This document describes the requirements for implementing WMA 2.0 
 * (Wireless Messaging API, JSR 205). <br>
 * WMA 2.0 API allows Java applications to access SMS and MMS functionality 
 * on a J2ME compliant device.  The wireless network of a target J2ME 
 * technology-enabled device device can be GSM, 
 * CDMA or any other underlying protocol that supports SMS.
 *
 * The specifications be found at: http://www.jcp.org/en/jsr/detail?id=205
 * 
 * This document lists all low-level APIs needed for porting wireless 
 * messaging features on a target J2ME technology-enabled device, including:
 * - SMS/CBS/MMS reception
 * - SMS/CBS/MMS sending
 * - Getting SMS/CBS/MMS service specific attributes
 *
 * @{
 */

/**
 * @defgroup MandatorySMS Mandatory SMS API
 * @ingroup WMA
 * @{
 */

 /**
 * check if the SMS service is available, and SMS messages can be sent and received
 *
 * @return <tt>JAVACALL_OK</tt> if SMS service is avaialble 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_sms_is_service_available(void);
    


/**
 * @enum javacall_sms_encoding
 * @brief SMS encoding format
 *
 * These values must match those in 
 * src/protocol/sms/classes/com/sun/midp/io/j2me/sms/Protocol.java
 */
typedef enum {
        JAVACALL_SMS_MSG_TYPE_ASCII        = 0,
        JAVACALL_SMS_MSG_TYPE_BINARY       = 1,
        JAVACALL_SMS_MSG_TYPE_UNICODE_UCS2 = 2
} javacall_sms_encoding;
    
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
 * @param destAddress the target SMS address for the message.  The format of the address 
 *                parameter is  expected to be compliant with MSIDN, for example,. +123456789 
 * @param msgBuffer the message body (payload) to be sent
 * @param msgBufferLen the message body (payload) len
 * @param sourcePort source port of SMS message
 * @param destPort destination port of SMS message where 0 is default destination port 
 * @param handle handle of sent sms 
 * 
 * Note: javacall_callback_on_complete_sms_send() needs to be called to notify
 *       completion of sending operation.
 *       The returned handle will be passed to 
 *         javacall_callback_on_complete_sms_send( ) upon completion
 */
javacall_result javacall_sms_send(  javacall_sms_encoding   msgType, 
                        const unsigned char*    destAddress, 
                        const unsigned char*    msgBuffer, 
                        int                     msgBufferLen, 
                        unsigned short          sourcePort, 
                        unsigned short          destPort,
                        int                     handle);

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
javacall_result javacall_sms_add_listening_port(unsigned short portNum);
    
/**
 * unregisters a message port number. 
 * After unregistering a port number, SMS messages received by the device for 
 * the specfied port should not be delivered tothe WMA implementation.  
 * If this API specifies a port number which is not registered, then it should 
 * return an error code.
 *
 * @param portNum port to stop listening to
 * @return <tt>JAVACALL_OK </tt> if stopped listening to port, 
 *          or <tt>JAVACALL_FAIL</tt> if failed, or port already not registered
 */
javacall_result javacall_sms_remove_listening_port(unsigned short portNum);
    
    
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
        javacall_bool           hasPort);


/** @} */
    


/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - -  - - - - - - -  
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurence of the
  undelying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup NotificationSMS Notification API for SMS 
 * @ingroup WMA
 * @{
 */    

/**
 * A callback function to be called by platform to notify that an SMS 
 * has completed sending operation.
 * The platfrom will invoke the call back in platform context for
 * each sms sending completion. 
 *
 * @param result indication of send completed status result: Either
 *         <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> on failure
 * @param handle Handle value returned from javacall_sms_send
 */
void javanotify_sms_send_completed(
                        javacall_result result, 
                        int handle);

/**
 * callback that needs to be called by platform to handover an incoming SMS intended for Java 
 *
 * After this function is called, the SMS message should be removed from platform inbox
 * 
 * @param msgType JAVACALL_SMS_MSG_TYPE_ASCII, or JAVACALL_SMS_MSG_TYPE_BINARY 
 *            or JAVACALL_SMS_MSG_TYPE_UNICODE_UCS2  1002
 * @param sourceAddress the source SMS address for the message.  The format of the address 
              parameter is  expected to be compliant with MSIDN, for example,. +123456789 
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
void javanotify_incoming_sms(
        javacall_sms_encoding   msgType,
        char*                   sourceAddress,
        unsigned char*          msgBuffer,
        int                     msgBufferLen,
        unsigned short          sourcePortNum,
        unsigned short          destPortNum,
        javacall_int64          timeStamp
        );


/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif 

