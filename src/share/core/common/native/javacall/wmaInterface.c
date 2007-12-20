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

/**
 * @file
 *
 * JSR120 interface methods.
 */

#include <jsr120_sms_protocol.h>
#include <jsr120_cbs_protocol.h>
#if (ENABLE_JSR_205)
#include <jsr205_mms_protocol.h>
#endif

#include "javacall_sms.h"
#include <javacall_memory.h>
#include <wmaInterface.h>

#ifdef ENABLE_MIDP
  #include <push_server_resource_mgmt.h>
  #include <midpServices.h>
#endif
#include <stdio.h>
#include <string.h>

#if (ENABLE_CDC == 1)
  #include "jsr120_signals.h"
#endif

/**
 * Defined in wmaUDPEmulator.c
 * No need in current implementation.
 */
WMA_STATUS init_jsr120() {
#if (ENABLE_CDC == 1)
    jsr120_init_signal();
#endif
#if (ENABLE_CDC == 1)
    javacall_wma_init();
#endif
    return WMA_NET_SUCCESS;
}

#if (ENABLE_CDC == 1)
void javanotify_incoming_sms(
        javacall_sms_encoding   msgType,
        char*                   sourceAddress,
        unsigned char*          msgBuffer,
        int                     msgBufferLen,
        unsigned short          sourcePortNum,
        unsigned short          destPortNum,
        javacall_int64          timeStamp) {

    SmsMessage* sms = jsr120_sms_new_msg(
        msgType, sourceAddress, sourcePortNum, destPortNum, timeStamp, msgBufferLen, msgBuffer);
    jsr120_sms_pool_add_msg(sms);
}

void javanotify_sms_send_completed(
                        javacall_result result, 
                        int             handle) {
}
#endif

/**
 * Defined in wmaUDPEmulator.c
 * No need in current implementation.
 */
void finalize_jsr120() {
#if (ENABLE_CDC == 1)
    jsr120_finalize_signal();
#endif
#if (ENABLE_CDC == 1)
    javacall_wma_close();
#endif
}


/**
 * Defined in wmaSocket.c
 * Called from midp loop on WMA signal.
 */
jboolean jsr120_check_signal(midpSignalType signalType, int descriptor, int status) {

    switch (signalType) {
    case WMA_SMS_READ_SIGNAL:
    {
        SmsMessage* sms = (SmsMessage*)descriptor;

        jsr120_notify_incoming_sms(
            sms->encodingType, sms->msgAddr, sms->msgBuffer, sms->msgLen,
            sms->sourcePortNum, sms->destPortNum, sms->timeStamp);

        javacall_free(sms->msgAddr);
        javacall_free(sms->msgBuffer);
        javacall_free(sms);

        return KNI_TRUE;
    }
    case WMA_SMS_WRITE_SIGNAL:
        jsr120_notify_sms_send_completed((int)descriptor, (WMA_STATUS)status);
        return KNI_TRUE;
    case WMA_CBS_READ_SIGNAL:
    {
        CbsMessage* cbs = (CbsMessage*)descriptor;

	jsr120_notify_incoming_cbs(
            cbs->encodingType, cbs->msgID, cbs->msgBuffer, cbs->msgLen);

        javacall_free(cbs->msgBuffer);
        javacall_free(cbs);

        return KNI_TRUE;
    }
#if (ENABLE_JSR_205)
    case WMA_MMS_READ_SIGNAL:
    {
        MmsMessage* mms = (MmsMessage*)descriptor;

        //if mms available notification only
        if (mms->msgLen == -1) {
            //mms->data contains javacall_handler for this case
            jsr205_notify_mms_available((int)mms->msgBuffer, mms->appID, mms->replyToAppID);
        } else {
            jsr205_notify_incoming_mms(mms->fromAddress, mms->appID,
                mms->replyToAppID, mms->msgLen, mms->msgBuffer);
        }

        javacall_free(mms->fromAddress);
        javacall_free(mms->appID);
        javacall_free(mms->replyToAppID);
        javacall_free(mms->msgBuffer);
        javacall_free(mms);

        return KNI_TRUE;
    }
    case WMA_MMS_WRITE_SIGNAL:

        jsr205_mms_notify_send_completed((int)descriptor, (WMA_STATUS)status);
    	return KNI_TRUE;
#endif
    }

    return KNI_FALSE;
}





