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

#ifndef __JAVACALL_MMS_H
#define __JAVACALL_MMS_H

#ifdef __cplusplus
extern "C" {
#endif
    
/**
 * @file javacall_mms.h
 * @ingroup JSR205
 * @brief Javacall interfaces for MMS
 */

#include "javacall_defs.h" 
	
/**
 * This is usage scenario
 *
 * When WMA application is not running:
 * 1. Java Task gets a MMS notification from the platform through 
 *    javanotify_incoming_mms_available() callback.
 * 2. Java Task checks if the application ID of MMS message is registered 
 *    for PUSH. If yes, the MMS message header is cached to MMS Pool of Java Task.
 * 3. Java Task gets the confirmation from the user 
 *    through javacall_push_show_request_launch_java()
 * 4. If yes, WMA application is launched and MessageConnection.receive() 
 *    is called to get the MMS message
 *    MessageConnection.receive():
 *    - it retrieves cached MMS header 
 *    - Java gets the confirmation from the user to fetch the MMS message body.
 *      If yes, javacall_mms_fetch() is called
 *    - it is blocked until the MMS message body is transfered through 
 *      javanotify_incoming_mms() callback.
 *
 * When WMA application is running:
 * 1. Java Task gets a notification from the platform 
 *    through javanotify_incoming_mms_available() callback.
 * 2. Java finds that the running WMA application is listening 
 *    MMS messages, and the MMS message header is cached.
 * 3. MessageListener.notifyIncomingMessage() is called
 * 4. MessageConnection.receive() is called to get the MMS message
 *    MessageConnection.receive():
 *    - it retrieves cached MMS header 
 *    - Java gets the confirmation from the user to fetch the MMS message body.
 *      If yes, javacall_mms_fetch() is called
 *    - it is blocked until the MMS message body is transfered through 
 *      javanotify_incoming_mms() callback.
 */

/**
 * @defgroup JSR205 JSR205 MMS API
 *
 * The following API definitions are required by JSR-205.
 * These APIs are not required by standard JTWI implementations.
 *
 * @{
 */

/**
 * @defgroup jsrMandatoryMMS Mandatory MMS API
 * @ingroup JSR205
 * @{
 */

 /**
 * checks if the Multimedia Message Service (MMS) is available, 
 * and MMS messages can be sent and received
 *
 * @return <tt>JAVACALL_OK</tt> if MMS service is avaialble 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_mms_is_service_available(void);
    

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
        );
    
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
javacall_result javacall_mms_fetch(javacall_handle handle, javacall_bool fetch);

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
javacall_result javacall_mms_add_listening_appID(const char* appID);
    
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
javacall_result javacall_mms_remove_listening_appID(const char* appID);
    
/**
 * Computes the number of transport-layer segments that would be required to
 * send the given message.
 *
 * @param msgBuffer The message to be sent.
 * @param msgLen The length of the message.
 * @return The number of transport-layer segments required to send the message.
 */
int javacall_mms_get_number_of_segments(unsigned char msgBuffer[], int msgLen);

/**
 * Gets the phone number of device
 *
 * @return The phone number of device.
 */
javacall_utf16_string javacall_mms_get_internal_phone_number();

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
 * @defgroup jsrNotificationMMS Notification API for MMS 
 * @ingroup JSR205
 * @{
 */    
/**
 * @enum javacall_mms_sending_result
 */
typedef enum {
        JAVACALL_MMS_SENDING_RESULT_SUCCESS     =1,
        JAVACALL_MMS_SENDING_RESULT_FAILURE     =0
}javacall_mms_sending_result;

/**
 * A callback function to be called by platform to notify that an MMS 
 * has completed sending operation.
 * The platform will invoke the call back in platform context for
 * each mms sending completion. 
 *
 * @param result indication of send completed status result: Either
 *         <tt>JAVACALL_MMS_CALLBACK_SEND_SUCCESSFULLY</tt> on success,
 *         <tt>JAVACALL_MMS_CALLBACK_SEND_FAILED</tt> on failure
 * @param handle of available MMS
 */
void javanotify_mms_send_completed(
                        javacall_mms_sending_result result, 
                        javacall_handle             handle);
    
/**
 * callback that needs to be called by platform when an incoming MMS message arrives in the MMS proxy. 
 *
 * The MMS message header have to conforms to the message 
 * structure in D.1 and D.2 of JSR205 spec.
 *
 * @param handle of available MMS
 * @param headerLen The length of the message header.
 * @param header The message header should include Subject, DeliveryData, 
 *          Priority, From, To, Cc and Bcc.
 *          If the MMS message is for Java applications, Application-ID 
 *          and Reply-To-Application-ID are added to the Content-Type 
 *          header field as additional Content-Type parameters.
 */
void javanotify_incoming_mms_available(
        javacall_handle         handle,
        int                     headerLen, 
        const char*             header
        );

/**
 * callback that needs to be called by platform to handover an incoming MMS message intended for Java 
 *
 * After this function is called, the MMS message should be removed from platform inbox
 * 
 * The MMS message body have to conforms to the message 
 * structure in D.1 and D.2 of JSR205 spec.
 *
 * @param handle of available MMS
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
 */
void javanotify_incoming_mms(
        javacall_handle handle,
        int             bodyLen, 
        const unsigned char*  body);

/**
 * The (temporary?) callback function for the whole message.
 * The function contains all necessary information, it does not use the scenario below:
 *   -> javanotify_incoming_mms_available(hangle, header)
 *   <- javacall_mms_fetch(handle, ok)
 *   -> javanotify_incoming_mms(handle, body)
 */
void javanotify_incoming_mms_singlecall(
        char* fromAddress, char* appID, char* replyToAppID,
        int             bodyLen, 
        unsigned char*  body);


/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif 
