/*
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

#include <stdio.h> // NULL, fprintf()
#include <string.h> // strcmp()
#include <jump_messaging.h>
#include <jsr120_jumpdriver.h>

#include <jsr120_sms_pool.h>
#include <jsr120_cbs_pool.h>
#include "javacall_cbs.h"
#include "javacall_sms.h"

#include <kni.h>
#include <sni.h>

typedef int MessageListener(JUMPMessage m);

MessageListener jsr120_jumpdriver_listener, jsr120_jumpcallback_listener;

#ifdef ENABLE_JSR_205
MessageListener jsr205_jumpdriver_listener, jsr205_jumpcallback_listener;
#endif

static void wma_listener(JUMPPlatformCString msgType, 
    MessageListener *listeners[], int listener_cnt);
char *prog_name = "isolate";

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_jump_driver_wma_Listener_nativeStartListener) {
    JUMPMessageStatusCode code;
    static const JUMPPlatformCString msgType = "jsrop/wma";
    static MessageListener *listeners[] = {
        jsr120_jumpcallback_listener
#ifdef ENABLE_JSR_205
        , jsr205_jumpcallback_listener
#endif
    };
    prog_name = "listener";
    CVMD_gcSafeExec(_ee, {
        jumpMessageQueueCreate(msgType, &code);
        if (code == JUMP_SUCCESS) {
            wma_listener(msgType, listeners, sizeof listeners / sizeof listeners[0]);
        }
    });
    KNI_ReturnVoid();
}
void wmaDriver(int argc, char **argv) {
    static const JUMPPlatformCString msgType = "native/wmaDriver";
    static MessageListener *listeners[] = {
        jsr120_jumpdriver_listener
#ifdef ENABLE_JSR_205
        , jsr205_jumpdriver_listener
#endif
    };
    prog_name = "driver";
    init_jsr120();
    wma_listener(msgType, listeners, sizeof listeners / sizeof listeners[0]);
}

static void wma_listener(JUMPPlatformCString msgType, 
            MessageListener *listeners[], int listener_cnt) {
    JUMPMessage in;
    JUMPMessageStatusCode code;

    in = jumpMessageWaitFor(msgType, 0, &code);
    while (code == JUMP_SUCCESS && in != NULL) {
        int i;
        for (i = 0; i < listener_cnt; i++) {
            if ((*listeners[i])(in) != 0) {
                break;
            }
        }
        in = jumpMessageWaitFor(msgType, 0, &code);
    } 
}

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
        ) {

    SmsMessage *sms = jsr120_sms_new_msg(
                msgType, (unsigned char*)sourceAddress,
        sourcePortNum, destPortNum, timeStamp, msgBufferLen, msgBuffer);
    INVOKE_REMOTELY_VOID(jsr120_sms_pool_add_msg, (sms));
}

/**
 * callback that needs to be called by platform to handover an incoming CBS intended for Java 
 *
 * After this function is called, the CBS message should be removed from platform inbox
 * 
 * @param msgType JAVACALL_CBS_MSG_TYPE_ASCII, or JAVACALL_CBS_MSG_TYPE_BINARY 
 *                or JAVACALL_CBS_MSG_TYPE_UNICODE_UCS2
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

    CbsMessage *cbs = jsr120_cbs_new_msg(msgType, msgID, msgBufferLen, msgBuffer);
    INVOKE_REMOTELY_VOID(jsr120_cbs_pool_add_msg, (cbs));
}

#include "jsr120_driver_def.h"

struct WMADRIVER_CLIENTS client_list__[MAX_CLIENTS];
int client_cnt__ = 0;


