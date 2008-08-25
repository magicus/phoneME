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

#ifdef __cplusplus
extern "C" {
#endif
    
#include "javacall_sms.h" 
    
/**
 * check if the SMS service is available, and SMS messages can be sent and received
 *
 * @return <tt>JAVACALL_OK</tt> if SMS service is avaialble 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_sms_is_service_available(void) {
    return JAVACALL_FAIL;
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
javacall_result javacall_sms_send(javacall_sms_encoding   msgType,
                                  const unsigned char*    destAddress,
                                  const unsigned char*    msgBuffer,
                                  int                     msgBufferLen,
                                  unsigned short          sourcePort,
                                  unsigned short          destPort,
                                  int                     handle) {
    return JAVACALL_NOT_IMPLEMENTED;
}
    
    
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
    return JAVACALL_FAIL;
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
        javacall_bool           hasPort){

    return 0;
}

#ifdef __cplusplus
}
#endif


