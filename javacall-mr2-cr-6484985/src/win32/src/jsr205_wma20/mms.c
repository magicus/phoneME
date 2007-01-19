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

#include "javacall_mms.h"
#include "javacall_memory.h"
#include "javacall_logging.h"
#include <memory.h>
#include <string.h>
#include <stdio.h>

static FILE* logFd = NULL;
#define LOG_FILE_NAME "mms_log.txt"


static void printToLog(unsigned char* buf, int len) {
#if 0
    if(logFd == NULL) {
        //FILE *fopen(const char *filename,const char *mode)
        logFd = fopen(LOG_FILE_NAME, "w");
        if(logFd == NULL) {
            return;
        }
    }
    //size_t fwrite(const void *buffer, size_t size, size_t count,FILE *stream);
    fwrite(buf, 1, len, logFd);
#else
    unsigned char* buf1 = (unsigned char*)javacall_malloc(len+1);
    memcpy(buf1, buf, len);
    buf1[len] = 0;
    javacall_print(buf1);
    javacall_free(buf1);
#endif
}


#define BUF_LEN 512
static char buffer[BUF_LEN];
 /**
  * checks if the Multimedia Message Service (MMS) is available, and MMS 
  * messages can be sent and received
  * 
  * @return <tt>JAVACALL_OK</tt> if MMS service is avaialble 
  *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
  */
javacall_result javacall_mms_is_service_available(void) {

    return JAVACALL_OK;
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

javacall_result javacall_mms_send(int headerLen,
                                  const char* header, 
                                  int bodyLen, 
                                  const unsigned char* body,
                                  javacall_handle* /*OUT*/pHandle) {

    javacall_print("## javacall: MMS sending...\n");

    // print headerLen
    memset(buffer, 0, BUF_LEN);
    sprintf(buffer, "\nmms.c: javacall_mms_send() headerLen=%d\nheader=\n", headerLen); 
    printToLog(buffer, strlen(buffer));

    // print header 
    memset(buffer, 0, BUF_LEN);
    if(headerLen <= BUF_LEN) {
        memcpy(buffer, header, headerLen);
        printToLog(buffer, headerLen);
    } else {
        memcpy(buffer, header, BUF_LEN - 1);
        printToLog(buffer, BUF_LEN - 1);
    }
    
    // print bodyLen 
    memset(buffer, 0, BUF_LEN);
    sprintf(buffer, "\nmms.c: javacall_mms_send() bodyLen=%d\nbody=\n", bodyLen); 
    printToLog(buffer, strlen(buffer));

    // print body
    memset(buffer, 0, BUF_LEN); 
    if(bodyLen <= BUF_LEN) {
        memcpy(buffer, body, bodyLen);
        printToLog(buffer, bodyLen);
    } else {
        memcpy(buffer, body, BUF_LEN - 1);
        printToLog(buffer, BUF_LEN - 1);
    }
    

    // print handle
    memset(buffer, 0, BUF_LEN);
    sprintf(buffer, "\nmms.c: javacall_mms_send() handle=%d\n", *pHandle); 
    printToLog(buffer, strlen(buffer));    

    javanotify_mms_send_completed(JAVACALL_MMS_SENDING_RESULT_SUCCESS, *pHandle);

    return JAVACALL_OK;
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

    memset(buffer, 0, BUF_LEN);
    sprintf(buffer, "\nmms.c: javacall_mms_fetch() called with handle=%d fetch=%d\n", handle, (int)fetch); 
    printToLog(buffer, strlen(buffer));

    return JAVACALL_OK;
}


#define APP_ID_MAX 8
static char* appIDList[APP_ID_MAX] = {0,0,0,0,0,0,0,0};

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

    int i;
    int free = -1;
    for (i=0; i<APP_ID_MAX; i++) {
        if (appIDList[i] == NULL) {
            free = i;
            continue;
        }
        if (0 == strcmp(appIDList[i], appID)) {
            return JAVACALL_FAIL;
        }
    }

    if (free == -1) {
        javacall_print("appID amount exceeded");
        return JAVACALL_FAIL;
    }

    appIDList[free] = strcpy((char*)javacall_malloc(strlen(appID)), appID);

    memset(buffer, 0, BUF_LEN);
    sprintf(buffer, "\nmms.c: javacall_mms_add_listening_appID() called with appID=%s\n", appID); 
    printToLog(buffer, strlen(buffer));

    return JAVACALL_OK;
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

    int i;
    for (i=0; i<APP_ID_MAX; i++) {
        if (appIDList[i] == NULL) {
            continue;
        }
        if (0 == strcmp(appIDList[i], appID)) {
            appIDList[i] = NULL;
            return JAVACALL_OK;
        }
    }

    memset(buffer, 0, BUF_LEN);
    sprintf(buffer, "\nmms.c: javacall_mms_remove_listening_appID() called with appID=%s\n", appID); 
    printToLog(buffer, strlen(buffer));

    return JAVACALL_FAIL;
}

javacall_result javacall_is_mms_appID_registered(const char* appID) {
    int i;
    for (i=0; i<APP_ID_MAX; i++) {
        if (appIDList[i] == NULL) {
            continue;
        }
        if (0 == strcmp(appIDList[i], appID)) {
            return JAVACALL_OK;
        }
    }
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

    memset(buffer, 0, BUF_LEN);
    sprintf(buffer, "\nmms.c: javacall_mms_get_number_of_segments() called with msgLen=%d\n", msgLen); 
    printToLog(buffer, strlen(buffer));

    return 1;
}

/**
 * Gets the phone number of device
 *
 * @return The phone number of device.
 */
javacall_utf16_string javacall_mms_get_internal_phone_number() {
    static javacall_utf16 phone[4] = {'9','1','1',0};
    return phone;
}
