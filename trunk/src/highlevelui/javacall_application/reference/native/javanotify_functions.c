/*
 *
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
 * Implementation of javacall notification functions.
 */

#include <string.h>
#include <midpServices.h>
#include <midp_logging.h>
#include <localeMethod.h>
#include <midp_jc_event_defs.h>

#include <javacall_keypress.h>
#include <javacall_penevent.h>
#include <javacall_lifecycle.h>
#include <javacall_sms.h>
#include <javacall_mms.h>
#include <javacall_cbs.h>
#include <javacall_events.h>
#include <javacall_time.h>
#include <javacall_socket.h>
#include <javacall_datagram.h>
#ifdef ENABLE_JSR_177
#include <javacall_carddevice.h>
#endif /* ENABLE_JSR_177 */
#ifdef USE_VSCL
#include <javacall_vscl.h>
#endif
#include <javacall_network.h>
#include <javacall_security.h>
#include <javacall_input.h>
#ifdef ENABLE_JSR_179
#include <javacall_location.h>
#endif /* ENABLE_JSR_177 */

static char urlAddress[BINARY_BUFFER_MAX_LEN];

#define MAX_PHONE_NUMBER_LENGTH 48
static char selectedNumber[MAX_PHONE_NUMBER_LENGTH];

/**
 * A helper function to 
 * @param event a pointer to midp_javacall_event_union
 * @return javacall_event_send() operation result
 */
static javacall_result
midp_jc_event_send(midp_jc_event_union *event) {
    return 
        javacall_event_send(
                            (unsigned char *)event,
                            sizeof(midp_jc_event_union)
        );
}

/**
 * The notification function to be called by platform for keypress
 * occurences.
 * The platfrom will invoke the call back in platform context for
 * each key press, key release and key repeat occurence
 * @param key the key that was pressed
 * @param type <tt>JAVACALL_KEYPRESSED</tt> when key was pressed
 *             <tt>JAVACALL_KEYRELEASED</tt> when key was released
 *             <tt>JAVACALL_KEYREPEATED</tt> when to be called repeatedly
 *             by platform during the duration that the key was held
 */
void javanotify_key_event(javacall_key key, javacall_keypress_type type) {
    midp_jc_event_union e;

    REPORT_INFO2(LC_CORE,"javanotify_key_event() >> key=%d , type=%d\n",key,type);

    e.eventType = MIDP_JC_EVENT_KEY;
    e.data.keyEvent.key = key;
    e.data.keyEvent.keyEventType = type;

    midp_jc_event_send(&e);
}

/**
* The notification function to be called by platform for pen 
* press/release/drag occurences.
* The platfrom will invoke the call back in platform context for
* each pen press, pen release and pen dragg occurence
* @param x the x positoin when the pen was pressed/released/dragged
* @param y the y positoin when the pen was pressed/released/dragged
* @param type <tt>JAVACALL_PENPRESSED</tt> when pen was pressed
*             <tt>JAVACALL_PENRELEASED</tt> when pen was released
*             <tt>JAVACALL_PENDRAGGED</tt> when pen was dragged
*/
void javanotify_pen_event(int x, int y, javacall_penevent_type type) {
    midp_jc_event_union e;

        REPORT_INFO3(LC_CORE,"javanotify_pen_event() >> x=%d, y=%d type=%d\n",x,y,type);

    e.eventType = MIDP_JC_EVENT_PEN;
    e.data.penEvent.type = type;
    e.data.penEvent.x = x;
    e.data.penEvent.y = y;

    midp_jc_event_send(&e);
}

/**
 * The platfrom should invoke this function in platform context to start
 * Java.
 */
void javanotify_start(void) {
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO(LC_CORE,"javanotify_start() >>\n");

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = 
#if ENABLE_MULTIPLE_ISOLATES
    "com.sun.midp.appmanager.MVMManager";
#else
    "com.sun.midp.appmanager.Manager";
#endif

    midp_jc_event_send(&e);
}

/**
 * The platfrom should invoke this function in platform context to start
 * the Java VM and run TCK.
 *
 * @param url the http location of the TCK server
 *            the url should be of the form: "http://host:port"
 * @param domain the TCK execution domain
 */
void javanotify_start_tck(char *tckUrl, javacall_lifecycle_tck_domain domain_type) {

    int length;
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO2(LC_CORE,"javanotify_start_tck() >> tckUrl=%s, domain_type=%d \n",tckUrl,domain_type);

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.installer.AutoTester";

    length = strlen(tckUrl);
    if (length >= BINARY_BUFFER_MAX_LEN)
        return;
    
    memset(urlAddress, 0, BINARY_BUFFER_MAX_LEN);
    memcpy(urlAddress, tckUrl, length);
    if(strcmp(urlAddress, "none") != 0)
        data->argv[data->argc++] = urlAddress;


    if(domain_type == JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED) {
        data->argv[data->argc++] = "untrusted";
    } else if(domain_type == JAVACALL_LIFECYCLE_TCK_DOMAIN_TRUSTED) {
        data->argv[data->argc++] = "trusted";
    } else if(domain_type == JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED_MIN) {
        data->argv[data->argc++] = "minimum";
    } else if(domain_type == JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED_MAX) {
        data->argv[data->argc++] = "maximum";
    } else
        return;

    midp_jc_event_send(&e);
}

/**
 * The platfrom should invoke this function in platform context to start
 * the Java VM and run i3test framework.
 *
 * @param arg1 optional argument 1
 * @param arg2 optional argument 2
 *
 * @note allowed argument description can be obtained by '-help' value as arg1.
 */
void javanotify_start_i3test(char* arg1, char* arg2) {
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO2(LC_CORE,"javanotify_start_i3test() >> %s %s\n",arg1,arg2);

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.i3test.Framework";
    if (NULL != arg1) {
        data->argv[data->argc++] = arg1;
        if (NULL != arg2)
            data->argv[data->argc++] = arg2;
    }

    midp_jc_event_send(&e);
}

/**
 * The platfrom should invoke this function in platform context to start
 * the Java VM and run installed Java Content Handler.
 *
 * @param handlerID launched Content Handler ID
 * @param url Invocation parameter: URL
 * @param action optional Invocation parameter: Action
 *
 * @note allowed argument description can be obtained by '-help' value as arg1.
 */
void javanotify_start_handler(char* handlerID, char* url, char* action) {
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO3(LC_CORE,"javanotify_start_handler() >> %s %s %s\n", 
		 handlerID, url, action);

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.content.Invoker";
    if (NULL != handlerID) {
        data->argv[data->argc++] = handlerID;
        if (NULL != url) {
            data->argv[data->argc++] = url;
            if (NULL != action)
                data->argv[data->argc++] = action;
        }
    }

    midp_jc_event_send(&e);
}

/**
 * A notification function for telling Java to perform installation of
 * a MIDletl,
 * 
 * 
 * If the given url is of the form http://www.sun.com/a/b/c/d.jad then
 * java will start a graphical installer will download the MIDlet
 * fom the internet.
 * If the given url is a file url (see below, file:///a/b/c/d.jar or
 * file:///a/b/c/d/jad) installation will be performed 
 * in the backgroudn without launching the graphic installer application
 * 
 *
 * @param url of MIDlet to install, can be either one of the following
 *   1. A full path to the jar file, of the following form file:///a/b/c/d.jar
 *   2. A full path to the JAD file, of the following form file:///a/b/c/d.jad
 *   3. An http url of jad file, of the following form,
 *      http://www.sun.com/a/b/c/d.jad
 * 
 */
void javanotify_install_midlet(const char *httpUrl) {
    int length;
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO1(LC_CORE,"javanotify_install_midlet() >> httpUrl=%s\n",httpUrl);

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.installer.GraphicalInstaller";
    data->argv[data->argc++] = "I";

    length = strlen(httpUrl);
    if (length >= BINARY_BUFFER_MAX_LEN)
        return;

    memset(urlAddress, 0, BINARY_BUFFER_MAX_LEN);
    memcpy(urlAddress, httpUrl, length);
    data->argv[data->argc++] = urlAddress;

    midp_jc_event_send(&e);
}

/**
 * A notification function for telling Java to perform installation of
 * a MIDlet from filesystem,
 *
 * The installation will be performed in the background without launching
 * the graphic installer application.
 *
 * The given path is the full path to MIDlet's jad file or jad.
 * In case the MIDlet's jad file is specified, then
 * the MIDlet's jar file muts reside in the same directory as the jad
 * file.
 *
 * @param jadFilePath full path the jad (or jar) file which is of the form:
 *        file://a/b/c/d.jad
 * @param jadFilePathLen length of the file path
 * @param userWasAsked a flag indicating whether the platform already asked
 *        the user for permission to download and install the application
 *        so there's no need to ask again and we can immediately install.
 */
void javanotify_install_midlet_from_filesystem(const javacall_utf16 * jadFilePath,
                                               int jadFilePathLen, int userWasAsked) {
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO(LC_CORE,"javanotify_install_midlet_from_filesystem() >>\n");

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.scriptutil.CommandLineInstaller";
    data->argv[data->argc++] = "I";

    if (jadFilePathLen >= BINARY_BUFFER_MAX_LEN)
        return;

    memset(urlAddress, 0, BINARY_BUFFER_MAX_LEN);
    unicodeToNative(jadFilePath, jadFilePathLen, (unsigned char *) urlAddress, BINARY_BUFFER_MAX_LEN);
    data->argv[data->argc++] = urlAddress;

    midp_jc_event_send(&e);
}

/**
 * The platfrom should invoke this function in platform context to start
 * the Java VM with arbitrary arguments.
 *
 * @param argc number of command-line arguments
 * @param argv array of command-line arguments
 *
 * @note This is a service function and it is introduced in the javacall
 *       interface for debug purposes. Please DO NOT CALL this function without
 *       being EXPLICITLY INSTRUCTED to do so.
 */
void javanotify_start_java_with_arbitrary_args(int argc, char* argv[])
{
    midp_jc_event_union e;
    
    REPORT_INFO(LC_CORE,"javanotify_start_java_with_arbitrary_args() >>\n");

    if (argc > MIDP_RUNMIDLET_MAXIMUM_ARGS)
        argc = MIDP_RUNMIDLET_MAXIMUM_ARGS;
    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;
    e.data.startMidletArbitraryArgEvent.argc = argc;
    memcpy (e.data.startMidletArbitraryArgEvent.argv, argv, argc * sizeof(char*));

    midp_jc_event_send(&e);
}

/**
 * The platfrom should invoke this function in platform context to end Java.
 */
void javanotify_shutdown(void) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_shutdown() >>\n");

    e.eventType = MIDP_JC_EVENT_END;

    midp_jc_event_send(&e);
}

/**
 * The platfrom should invoke this function in platform context to pause
 * Java.
 */
void javanotify_pause(void) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_pause() >>\n");

    e.eventType = MIDP_JC_EVENT_PAUSE;

    midp_jc_event_send(&e);
}

/**
 * The platfrom should invoke this function in platform context to end pause
 * and resume Java.
 */
void javanotify_resume(void) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_resume() >>\n");

    e.eventType = MIDP_JC_EVENT_RESUME;

    midp_jc_event_send(&e);
}

/**
 * The platfrom should invoke this function in platform context 
 * to switch the current foreground midlet to background midlet.
 * Task Manger may be foreground after this call.
 */
void javanotify_switchforeground(void) {
#if ENABLE_MULTIPLE_ISOLATES
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_switchforeground() >>\n");

    e.eventType = MIDP_JC_EVENT_SWITCH_FOREGROUND;

    midp_jc_event_send(&e);
#endif /* ENABLE_MULTIPLE_ISOLATES */
}

/**
 * The platfrom should invoke this function in platform context to pause
 * Java bytecode execution (without invoking pauseApp)
 */
void javanotify_internal_pause(void) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_internal_pause() >>\n");

    e.eventType = MIDP_JC_EVENT_INTERNAL_PAUSE;

    midp_jc_event_send(&e);
}

/**
 * The platfrom should invoke this function in platform context to end
 * an internal pause and resume Java bytecode processing
 */
void javanotify_internal_resume(void) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_internal_resume() >>\n");

    e.eventType = MIDP_JC_EVENT_INTERNAL_RESUME;

    midp_jc_event_send(&e);
}


/**
 * A callback function to be called for notification of network
 * conenction related events, such as network going down or up.
 * The platform will invoke the call back in platform context.
 * @param event the type of network-related event that occured
 *              JAVACALL_NETWORK_DOWN if the network became unavailable
 *              JAVACALL_NETWORK_UP if the network is now available
 *
 */
void javanotify_network_event(javacall_network_event netEvent) {
    midp_jc_event_union e;
    e.eventType = MIDP_JC_EVENT_NETWORK;
    if (netEvent == JAVACALL_NETWORK_UP) {
        e.data.networkEvent.netType = MIDP_NETWORK_UP;
    } else {
        e.data.networkEvent.netType = MIDP_NETWORK_DOWN;
    }

    midp_jc_event_send(&e);
}

#if ENABLE_JSR_120 || ENABLE_JSR_205
    #include <jsr120_sms_pool.h>
    #include <jsr120_cbs_pool.h>
#endif
#if ENABLE_JSR_205
    #include <jsr205_mms_pool.h>
#endif

/**
 * callback that needs to be called by platform to handover an incoming SMS intended for Java 
 *
 * After this function is called, the SMS message should be removed from platform inbox
 * 
 * @param msgType JAVACALL_SMS_MSG_TYPE_ASCII, or JAVACALL_SMS_MSG_TYPE_BINARY or JAVACALL_SMS_MSG_TYPE_UNICODE_UCS2  1002
 * @param sourceAddress the source SMS address for the message.  The format of the address  parameter  
 *                is  expected to be compliant with MSIDN, for example,. +123456789 
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
void javanotify_incoming_sms(javacall_sms_encoding msgType,
                        char *sourceAddress,
                        unsigned char *msgBuffer,
                        int msgBufferLen,
                        unsigned short sourcePortNum,
                        unsigned short destPortNum,
                        javacall_int64 timeStamp) {
#if (ENABLE_JSR_120 || ENABLE_JSR_205)
    midp_jc_event_union e;
        SmsMessage* sms;

    e.eventType = MIDP_JC_EVENT_SMS_INCOMING;
        sms = jsr120_sms_new_msg(
                msgType, sourceAddress, sourcePortNum, destPortNum, timeStamp, msgBufferLen, msgBuffer);

        e.data.smsIncomingEvent.stub = (int)sms;

    midp_jc_event_send(&e);
#endif
    return;
}

/*
 * See javacall_mms.h for description
 */
void javanotify_incoming_mms_singlecall(
                char* fromAddress, char* appID, char* replyToAppID,
        int bodyLen, unsigned char* body) {
#if ENABLE_JSR_205
    midp_jc_event_union e;
        MmsMessage* mms;

    e.eventType = MIDP_JC_EVENT_MMS_INCOMING;

        mms = jsr205_mms_new_msg(fromAddress, appID, replyToAppID, bodyLen, body);

        e.data.mmsIncomingEvent.stub = (int)mms;

    midp_jc_event_send(&e);
#endif
    return;
}

/**
 * callback that needs to be called by platform to handover an incoming CBS intended for Java 
 *
 * After this function is called, the CBS message should be removed from platform inbox
 * 
 * @param msgType JAVACALL_CBS_MSG_TYPE_ASCII, or JAVACALL_CBS_MSG_TYPE_BINARY or JAVACALL_CBS_MSG_TYPE_UNICODE_UCS2
 * @param msgID message ID
 * @param msgBuffer payload of incoming cbs 
 *        if msgType is JAVACALL_CBS_MSG_TYPE_ASCII then this is a 
 *        pointer to char* ASCII string. 
 *        if msgType is JAVACALL_CBS_MSG_TYPE_UNICODE_UCS2, then this
 *        is a pointer to javacall_utf16 UCS-2 string. 
 *        if msgType is JAVACALL_CBS_MSG_TYPE_BINARY, then this is a 
 *        pointer to binary octet buffer. 
 * @param msgBufferLen payload len of incoming cbs 
 */
void javanotify_incoming_cbs(
        javacall_cbs_encoding  msgType,
        unsigned short         msgID,
        unsigned char*         msgBuffer,
        int                    msgBufferLen) {
#if (ENABLE_JSR_120 || ENABLE_JSR_205)
    midp_jc_event_union e;
        CbsMessage* cbs;

    e.eventType = MIDP_JC_EVENT_CBS_INCOMING;

    cbs = jsr120_cbs_new_msg(msgType, msgID, msgBufferLen, msgBuffer);

        e.data.mmsIncomingEvent.stub = (int)cbs;

    midp_jc_event_send(&e);
#endif
    return;    
}

/**
 * A callback function to be called by platform to notify that an SMS 
 * has completed sending operation.
 * The platfrom will invoke the call back in platform context for
 * each sms sending completion. 
 *
 * @param result indication of send completed status result: Either
 *         <tt>JAVACALL_SMS_CALLBACK_SEND_SUCCESSFULLY</tt> on success,
 *         <tt>JAVACALL_SMS_CALLBACK_SEND_FAILED</tt> on failure
 * @param handle Handle value returned from javacall_sms_send
 */
void javanotify_sms_send_completed(javacall_sms_sending_result result,
                                   int handle) {
#if (ENABLE_JSR_120 || ENABLE_JSR_205)
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_SMS_SENDING_RESULT;
    e.data.smsSendingResultEvent.handle = (void *) handle;
    e.data.smsSendingResultEvent.result
        = JAVACALL_SMS_SENDING_RESULT_SUCCESS == result ? 0 : -1;

    midp_jc_event_send(&e);
#endif
    return;
}

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
void javanotify_mms_send_completed(javacall_mms_sending_result result,
                                   javacall_handle handle) {
#if ENABLE_JSR_205
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_MMS_SENDING_RESULT;
    e.data.mmsSendingResultEvent.handle = (void *) handle;
    e.data.mmsSendingResultEvent.result
        = JAVACALL_MMS_SENDING_RESULT_SUCCESS == result ? 0 : -1;

    midp_jc_event_send(&e);
#endif
    return;
}

#ifdef ENABLE_JSR_177
/**
 * 
 */ 
void javanotify_carddevice_event(javacall_carddevice_event event,
                                 void *context) {
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_CARDDEVICE;
    switch (event) {
    case JAVACALL_CARDDEVICE_RESET:
        e.data.carddeviceEvent.eventType = MIDP_CARDDEVICE_RESET;
        e.data.carddeviceEvent.handle = (int)context;
        break;
    case JAVACALL_CARDDEVICE_XFER:
        e.data.carddeviceEvent.eventType = MIDP_CARDDEVICE_XFER;
        e.data.carddeviceEvent.handle = (int)context;
        break;
    case JAVACALL_CARDDEVICE_UNLOCK:
        e.data.carddeviceEvent.eventType = MIDP_CARDDEVICE_UNLOCK;
        e.data.carddeviceEvent.handle = 0;
        break;
    default:
        /* TODO: report error */
        return;
    }
    javacall_event_send((unsigned char *) &e, sizeof(midp_jc_event_union));
    return;
}
#endif /* ENABLE_JSR_177 */

/**
 * A callback function to be called for notification of non-blocking 
 * client/server socket related events, such as a socket completing opening or , 
 * closing socket remotely, disconnected by peer or data arrived on 
 * the socket indication.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence. 
 *
 * @param type type of indication: Either
 *          - JAVACALL_EVENT_SOCKET_OPEN_COMPLETED
 *          - JAVACALL_EVENT_SOCKET_CLOSE_COMPLETED
 *          - JAVACALL_EVENT_SOCKET_RECEIVE
 *          - JAVACALL_EVENT_SOCKET_SEND
 *          - JAVACALL_EVENT_SOCKET_REMOTE_DISCONNECTED
 *          - JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED  
 * @param socket_handle handle of socket related to the notification
 * @param operation_result <tt>JAVACALL_OK</tt> if operation 
 *        completed successfully, 
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void javanotify_socket_event(javacall_socket_callback_type type,
                        javacall_handle socket_handle,
                        javacall_result operation_result) {

    midp_jc_event_union e;
    e.eventType = MIDP_JC_EVENT_SOCKET;
    e.data.socketEvent.handle = socket_handle;
    e.data.socketEvent.status = operation_result;

    e.data.socketEvent.extraData = NULL;

    switch (type) {
        case JAVACALL_EVENT_SOCKET_OPEN_COMPLETED:
        case JAVACALL_EVENT_SOCKET_SEND:
            e.data.socketEvent.waitingFor = NETWORK_WRITE_SIGNAL;
            break;

        case JAVACALL_EVENT_SOCKET_RECEIVE:
            e.data.socketEvent.waitingFor = NETWORK_READ_SIGNAL;
            break;

        case JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED:
            e.data.socketEvent.waitingFor = HOST_NAME_LOOKUP_SIGNAL;
            break;

        case JAVACALL_EVENT_SOCKET_CLOSE_COMPLETED:
            e.data.socketEvent.waitingFor = NETWORK_EXCEPTION_SIGNAL;
            break;

        case JAVACALL_EVENT_SOCKET_REMOTE_DISCONNECTED:
            e.data.socketEvent.waitingFor = NETWORK_EXCEPTION_SIGNAL;
            break;                 

        default:
            /* IMPL_NOTE: decide what to do */
            return;                 /* do not send event to java */


            /* IMPL_NOTE: NETWORK_EXCEPTION_SIGNAL is not assigned by any indication */
    }

    midp_jc_event_send(&e);
}

/**
 * A callback function to be called for notification of non-blocking 
 * server socket only related events, such as a accept completion.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence. 
 *
 * @param type type of indication: Either
 *          JAVACALL_EVENT_SERVER_SOCKET_ACCEPT_COMPLETED
 * @param socket_handle handle of socket related to the notification.
 *                          If the platform is not able to provide the socket 
 *                          handle in the callback, it should pass 0 as the new_socket_handle
 *                          and the implementation will call javacall_server_socket_accept_finish
 *                          to retrieve the handle for the accepted connection.
 * @param new_socket_handle in case of accept the socket handle for the 
 *                          newly created connection
 *               
 * @param operation_result <tt>JAVACALL_OK</tt> if operation 
 *        completed successfully, 
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void /* OPTIONAL */ javanotify_server_socket_event(javacall_server_socket_callback_type type,
                               javacall_handle socket_handle,
                               javacall_handle new_socket_handle,
                               javacall_result operation_result) {
    midp_jc_event_union e;
    e.eventType = MIDP_JC_EVENT_SOCKET;
    e.data.socketEvent.handle = socket_handle;
    e.data.socketEvent.status = operation_result;

    e.data.socketEvent.extraData = NULL;

    if (type == JAVACALL_EVENT_SERVER_SOCKET_ACCEPT_COMPLETED) {
        e.data.socketEvent.waitingFor = NETWORK_READ_SIGNAL;
        /* IMPL_NOTE: how about using  extraData instead of status? Then, malloc required. */


        /* If the platform is not able to provide the socket handle in the callback,
           it should pass 0. */
        if (operation_result == JAVACALL_OK) {
            (javacall_handle) e.data.socketEvent.status = new_socket_handle;
        } else {
            e.data.socketEvent.status = operation_result;
        }
    } else {
        /* IMPL_NOTE: decide what to do */
        return;                 /* do not send event to java */
    }

    midp_jc_event_send(&e);
}

/**
 * A callback function to be called for notification of non-blocking 
 * client/server socket related events, such as a socket completing opening or , 
 * closing socket remotely, disconnected by peer or data arrived on 
 * the socket indication.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence. 
 *
 * @param type type of indication: Either
 *     - JAVACALL_EVENT_DATAGRAM_RECVFROM_COMPLETED
 *     - JAVACALL_EVENT_DATAGRAM_SENDTO_COMPLETED
 * @param handle handle of datagram related to the notification
 * @param operation_result <tt>JAVACALL_OK</tt> if operation 
 *        completed successfully, 
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void javanotify_datagram_event(javacall_datagram_callback_type type,
                               javacall_handle handle,
                               javacall_result operation_result) {

    midp_jc_event_union e;
    e.eventType = MIDP_JC_EVENT_SOCKET;
    e.data.socketEvent.handle = handle;
    e.data.socketEvent.status = operation_result;

    e.data.socketEvent.extraData = NULL;

    switch (type) {
        case JAVACALL_EVENT_DATAGRAM_SENDTO_COMPLETED:
            e.data.socketEvent.waitingFor = NETWORK_WRITE_SIGNAL;
            break;
        case JAVACALL_EVENT_DATAGRAM_RECVFROM_COMPLETED:
            e.data.socketEvent.waitingFor = NETWORK_READ_SIGNAL;
            break;
        default:
            /* IMPL_NOTE: decide what to do */
            return;                 /* do not send event to java */

            /* IMPL_NOTE: NETWORK_EXCEPTION_SIGNAL is not assigned by any indication */
    }

    midp_jc_event_send(&e);
}

/**
 * Post native media event to Java event handler
 * 
 * @param type          Event type
 * @param playerId      Player ID that came from javacall_media_create function
 * @param data          Data for this event type
 */
void javanotify_on_media_notification(javacall_media_notification_type type,
                                      javacall_int64 playerId,
                                      void *data) {
#if ENABLE_MMAPI
    extern int g_currentPlayer;

    midp_jc_event_union e;

    if (-1 == playerId) {
        playerId = g_currentPlayer;
    }

    REPORT_INFO2(LC_MMAPI, "javanotify_on_media_notification type=%d id=%d\n", type, (int)(playerId & 0xFFFFFFFF));

    e.eventType = MIDP_JC_EVENT_MULTIMEDIA;
    e.data.multimediaEvent.mediaType = type;
    e.data.multimediaEvent.isolateId = (int)((playerId >> 32) & 0xFFFF);
    e.data.multimediaEvent.playerId = (int)(playerId & 0xFFFF);
    e.data.multimediaEvent.data = (int) data;

    midp_jc_event_send(&e);
#endif
}

/**
 * The implementation call this callback notify function when image decode done
 *
 * @param handle Handle that is returned from javacall_image_decode_start
 * @param result the decoding operation result
 */
void javanotify_on_image_decode_end(javacall_handle handle, javacall_result result) {
#ifdef ENABLE_EXTERNAL_IMAGE_DECODER
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_IMAGE_DECODER;
    e.data.imageDecoderEvent.handle = handle;
    e.data.imageDecoderEvent.result = result;

    midp_jc_event_send(&e);
#else
   (void)handle;
   (void)result;
#endif
}

/**
 * A callback function to be called by the platfrom in order to notify
 * about changes in the available file system roots (new root was added/
 * a root on removed).
 */
void javanotify_fileconnection_root_changed(void) {
#ifdef ENABLE_JSR_75
    midp_jc_event_union e;
    e.eventType = JSR75_FC_JC_EVENT_ROOTCHANGED;

    midp_jc_event_send(&e);
#endif
}

#ifdef ENABLE_JSR_179
/**
 * A callback function to be called for notification of non-blocking 
 * location related events.
 * The platform will invoke the call back in platform context for
 * each provider related occurrence. 
 *
 * @param type type of indication: Either
 *          - JAVACALL_EVENT_LOCATION_OPEN_COMPLETED
 *          - JAVACALL_EVENT_LOCATION_ORIENTATION_COMPLETED
 *          - JAVACALL_EVENT_LOCATION_UPDATE_PERIODICALLY
 *          - JAVACALL_EVENT_LOCATION_UPDATE_ONCE
 * @param handle handle of provider related to the notification
 * @param operation_result operation result: Either
 *      - JAVACALL_OK if operation completed successfully, 
 *      - JAVACALL_LOCATION_RESULT_CANCELED if operation is canceled 
 *      - JAVACALL_LOCATION_RESULT_TIMEOUT  if operation is timeout 
 *      - JAVACALL_LOCATION_RESULT_OUT_OF_SERVICE if provider is out of service
 *      - JAVACALL_LOCATION_RESULT_TEMPORARILY_UNAVAILABLE if provider is temporarily unavailable
 *      - otherwise, JAVACALL_FAIL
 */
void javanotify_location_event(
        javacall_location_callback_type event,
        javacall_handle provider,
        javacall_location_result operation_result) {
    midp_jc_event_union e;

    e.eventType = JSR179_LOCATION_JC_EVENT;
    e.data.jsr179LocationEvent.event = event;
    e.data.jsr179LocationEvent.provider = provider;
    e.data.jsr179LocationEvent.operation_result = operation_result;

    midp_jc_event_send(&e);
}
#endif /* ENABLE_JSR_179 */

/**
 * The platform calls this callback notify function when the permission dialog
 * is dismissed. The platform will invoke the callback in platform context.
 *  
 * @param userPermssion the permission level the user chose
 */
void javanotify_security_permission_dialog_finish(
                         javacall_security_permission_type userPermission) {
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_PERMISSION_DIALOG;
    e.data.permissionDialog_event.permission_level = userPermission;

    midp_jc_event_send(&e);
}

/**
 * A notification function for telling Java to perform installation of
 * a content via http, for SprintAMS.
 *
 * This function requires that the descriptor (JADfile, or GCDfile)
 * has already been downloaded and resides somewhere on the file system.
 * The function also requires the full URL that was used to download the
 * file.
 * 
 * The given URL should be of the form http://www.sun.com/a/b/c/d.jad
 * or http://www.sun.com/a/b/c/d.gcd.  
 * Java will start a graphical installer which will download the content
 * fom the Internet.
 *
 * @param httpUrl null-terminated http URL string of the content 
 *        descriptor. The URL is of the following form:
 *        http://www.website.com/a/b/c/d.jad
 * @param descFilePath full path of the descriptor file which is of the 
 *        form:
 *        /a/b/c/d.jad  or /a/b/c/d.gcd
 * @param descFilePathLen length of the file path
 * @param isJadFile set to TRUE if the mime type of of the downloaded
 *        descriptor file is <tt>text/vnd.sun.j2me.app-descriptor</tt>. If 
 *        the mime type is anything else (e.g., <tt>text/x-pcs-gcd</tt>), 
 *        this must be set to FALSE.
 * @param isSilent set to TRUE if the content is to be installed silently,
 *        without intervention from the user. (e.g., in the case of SL
 *        or SI messages)
 * 
 */
void javanotify_install_content(const char * httpUrl,
                                const javacall_utf16* descFilePath,
                                int descFilePathLen,
                                javacall_bool isJadFile,
                                                                javacall_bool isSilent) {
    const static int SchemaLen = 16;
    const static javacall_utf16 SchemaFile[] = {'f','i','l','e',':','/','/','/'};

    midp_jc_event_union e;
    int httpUrlLength, dscFileOffset;
        

    if ((httpUrl == NULL) || (httpUrl == NULL)) {
        return; /* mandatory parameter is NULL */
    }

    httpUrlLength = strlen(httpUrl) + 1;
    dscFileOffset = (httpUrlLength + 1) & (-2); /* align to WORD boundary */

    if ((descFilePathLen <= 0) ||
        (descFilePathLen * 2 + dscFileOffset > sizeof(urlAddress))) {
        return; /* static buffer so small */
    }

    /* Is it nessasary to add schema file like: "file:///"? */
    /* Let move the input strings to the static buffer - urlAddress */
    memcpy(urlAddress, httpUrl, httpUrlLength);
    memcpy(urlAddress + dscFileOffset, descFilePath, descFilePathLen * 2);

    e.eventType = MIDP_JC_EVENT_INSTALL_CONTENT;
    e.data.install_content.httpUrl = urlAddress;
    e.data.install_content.descFilePath = (javacall_utf16*) urlAddress + dscFileOffset;
    e.data.install_content.descFilePathLen = descFilePathLen;
    e.data.install_content.isJadFile = isJadFile;
    e.data.install_content.isSilent = isSilent;

    midp_jc_event_send(&e);
}

#ifdef USE_VSCL

/*
 * See javacall_vscl.h for description
 */
void javanotify_vscl_incoming_event(javacall_vscl_event_type type, 
                                    javacall_utf16* str1,
                                    javacall_utf16* str2) {
    midp_jc_event_union e;

    switch (type) {
    case JAVACALL_VSCL_INCOMING_CALL:
        e.eventType = MIDP_JC_EVENT_VSCL_INCOMING_CALL;
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=INCOMING_CALL\n");        
        break;
    case JAVACALL_VSCL_CALL_DROPPED:
        e.eventType = MIDP_JC_EVENT_VSCL_CALL_DROPPED;
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=CALL_DROPPED\n");        
        break;
    case JAVACALL_VSCL_INCOMING_MAIL_CBS:
    case JAVACALL_VSCL_INCOMING_MAIL_DELIVERY_CONF:
    case JAVACALL_VSCL_INCOMING_MAIL_MMS:
    case JAVACALL_VSCL_INCOMING_MAIL_SMS:
    case JAVACALL_VSCL_INCOMING_MAIL_WAP_PUSH:
    case JAVACALL_VSCL_SCHEDULED_ALARM:
                REPORT_INFO1(LC_VSCL,"javanotify_vscl_incoming_event() EVENT=%d NOT IMPLEMENTED!\n", (int)type);        
        return;
    case JAVACALL_VSCL_FLIP_OPEN:
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=FLIP_OPEN\n");
        e.eventType = MIDP_JC_EVENT_VSCL_FLIP_OPENED;
        break;
    case JAVACALL_VSCL_FLIP_CLOSED:
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=FLIP_CLOSED\n");
        e.eventType = MIDP_JC_EVENT_VSCL_FLIP_CLOSED;
        break;
    default:
                REPORT_INFO1(LC_VSCL,"javanotify_vscl_incoming_event() EVENT=%d NOT IMPLEMENTED!\n", (int)type);       
        return;
    }

    midp_jc_event_send(&e);
}
#endif /* USE_VSCL */

/**
 * Notify Java that the user has made a selection
 * 
 * @param phoneNumber a string representing the phone number the user selected.
 *                    The string will be copied so it can be freed after this
 *                    function call returns.
 *                    In case the user dismissed the phonebook without making a
 *                    selection, this sting is <tt>NULL</tt>
 */
void /* OPTIONAL */ javanotify_textfield_phonenumber_selection(char* phoneNumber) {
    midp_jc_event_union e;
    int length;

    e.eventType = MIDP_JC_EVENT_PHONEBOOK;
    memset(selectedNumber, 0, MAX_PHONE_NUMBER_LENGTH);

    if (phoneNumber != NULL) {
        length = strlen(phoneNumber) + 1;
        if (length <= MAX_PHONE_NUMBER_LENGTH) {
            memcpy(selectedNumber, phoneNumber, length);
        } else {
            memcpy(selectedNumber, phoneNumber, MAX_PHONE_NUMBER_LENGTH);
        }
    }

    e.data.phonebook_event.phoneNumber = selectedNumber;

    midp_jc_event_send(&e);
}

void /* OPTIONAL */ javanotify_rotation() {
     midp_jc_event_union e;
     int length;
  
     e.eventType = MIDP_JC_EVENT_ROTATION;    
     midp_jc_event_send(&e);
}
