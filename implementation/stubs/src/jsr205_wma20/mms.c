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
    
#include "javacall_defs.h" 
	
/**
 * checks if the Multimedia Message Service (MMS) is available, and MMS messages can be sent and received
 *
 * @return <tt>JAVACALL_OK</tt> if MMS service is avaialble 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_mms_is_service_available(void){
    return JAVACALL_FAIL;
}

/**
 * sends an MMS message
 *
 * The MMS message header and body have to conforms to the message 
 * structure in D.1 and D.2 of JSR205 spec.
 *
 * @param headerLen The length of the message header.
 * @param header The message header should include Subject, DeliveryData, 
 *          Priority, From, To, Cc and Bcc.
 *          If the MMS message is for Java applications, Application-ID 
 *          and Reply-To-Application-ID are added to the Content-Type 
 *          header field as additional Content-Type parameters.
 * @param bodyLen The length of the message body.
 * @param body The message body.
 *        The MMS message body is composed of one or more message parts.
 *        The following fields are in the message part structure:
 *          MIME-Type - the MIME Content-Type for the Message Part [RFC 2046]
 *          content-ID - the content-id header field value for the Message Part [RFC 2045]. 
 *              The content-id is unique over all Message Parts of an MMS and must always be set
 *          content-Location - the content location which specifies the  name of the 
 *              file that is attached. If  the content location is set to null, 
 *              no content location will be set for this message part.
 *          contents -  the message contents of the message part
 * @param pHandle of sent mms 
 *
 * @return <tt>JAVACALL_OK</tt> if send request success
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 * 
 * Note: javanotify_mms_send_completed() needs to be called to notify
 *       completion of sending operation.
 *       The returned handle will be passed to javanotify_mms_send_completed( ) upon completion
 */
javacall_result javacall_mms_send(  
        int headerLen, 
        const char* header, 
        int bodyLen, 
        const unsigned char* body,
        javacall_handle* /*OUT*/pHandle
        ) {
    
    return JAVACALL_FAIL;
}

/**
 * Requests to fetch the incoming MMS message.
 *
 * This function requests to fetch MMS message and should return quickly.
 * After a MMS indication was notified, this API requests the platform to retrieve the MMS message body.
 *
 * @param handle of available MMS message
 * @param fetch if JAVACALL_TRUE, the platform should fetch the MMS message 
 *          body from the network and call javanotify_incoming_mms().
 *          Otherwise, the MMS message body should be discarded.
 *
 * @return <tt>JAVACALL_OK</tt> if fetch request success
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_mms_fetch(javacall_handle handle, javacall_bool fetch) {
    return JAVACALL_FAIL;
}

/**
 * The platform must have the ability to identify the target application of incoming 
 * MMS messages, and delivers messages with application ID to the WMA implementation.
 * If this application ID has already been registered either by a native 
 * application or by another WMA application, then the API should return an error code.
 * 
 * @param appID The application ID associated with the message.
 * @return <tt>JAVACALL_OK</tt> if started listening, or 
 *         <tt>JAVACALL_FAIL</tt> or negative value if unsuccessful
 */
javacall_result javacall_mms_add_listening_appID(const char* appID) {
    return JAVACALL_FAIL;
}

/**
 * Stops listening to an application ID.
 * After unregistering an application ID, MMS messages received by the device 
 * for the specified application ID should not be delivered to the WMA 
 * implementation.  
 * If this API specifies an application ID which is not registered, then it 
 * should return an error code.
 *
 * @param appID The application ID to stop listening to
 * @return <tt>JAVACALL_OK </tt> if stopped listening to the application ID, 
 *          or <tt>0</tt> if failed, or the application ID not registered
 */
javacall_result javacall_mms_remove_listening_appID(const char* appID) {
    return JAVACALL_FAIL;
}

/**
 * Computes the number of transport-layer segments that would be required to
 * send the given message.
 *
 * @param msgBuffer The message to be sent.
 * @param msgLen The length of the message.
 * @return The number of transport-layer segments required to send the message.
 */
int javacall_mms_get_number_of_segments(unsigned char msgBuffer[], int msgLen) {

    return JAVACALL_FAIL;
}

/**
 * Gets the phone number of device
 *
 * @return The phone number of device.
 */
javacall_utf16_string javacall_mms_get_internal_phone_number() {

    return NULL;
}

#ifdef __cplusplus
}
#endif

