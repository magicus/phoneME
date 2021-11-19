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


START_INTERFACE(120, driver)

// jsr120_cbs_is_midlet_msgID_registered()
START(WMA_STATUS, jsr120_cbs_is_midlet_msgID_registered, (jchar msgID))
DECL_STATUS()
DECL_ARG(jchar, msgID)
ARG(Short, msgID)
INVOKE(status, jsr120_cbs_is_midlet_msgID_registered, (msgID))
END_STATUS()

// jsr120_cbs_register_midlet_msgID()
START(WMA_STATUS, jsr120_cbs_register_midlet_msgID, (jchar msgID,
                                            AppIdType msid, jint handle))
DECL_STATUS()
DECL_ARG(jchar, msgID)
DECL_ARG(AppIdType, msid)
DECL_ARG(jint, handle)
ARG(Short, msgID)
ARG(Int, msid)
ARG(Int, handle)
SET_CLIENT_ID(WMADRIVER_CBS_CLIENT, msid, handle, msgID)
INVOKE(status, jsr120_cbs_register_midlet_msgID, (msgID, msid, handle))
END_STATUS()


// jsr120_cbs_unregister_midlet_msgID()
START(WMA_STATUS, jsr120_cbs_unregister_midlet_msgID, (jchar msgID))
DECL_STATUS()
DECL_ARG(jchar, msgID)
ARG(Short, msgID)
INVOKE(status, jsr120_cbs_unregister_midlet_msgID, (msgID))
CLEAR_CLIENT_ID(WMADRIVER_CBS_CLIENT, msgID)
END_STATUS()

// jsr120_cbs_delete_midlet_suite_msg()
START_VOID(jsr120_cbs_delete_midlet_suite_msg, (AppIdType msid))
DECL_ARG(AppIdType, msid)
ARG(Int, msid)
INVOKE_VOID(jsr120_cbs_delete_midlet_suite_msg, (msid))
END_VOID()

// jsr120_is_sms_midlet_port_registered()
START(WMA_STATUS, jsr120_is_sms_midlet_port_registered, (jchar port))
DECL_STATUS()
DECL_ARG(jchar, port)
ARG(Short, port)
INVOKE(status, jsr120_is_sms_midlet_port_registered, (port))
END_STATUS()

// jsr120_register_sms_midlet_port()
START(WMA_STATUS, jsr120_register_sms_midlet_port, (jchar port,
        AppIdType msid, jint handle))
DECL_STATUS()
DECL_ARG(jchar, port)
DECL_ARG(AppIdType, msid)
DECL_ARG(jint, handle)
ARG(Short, port)
ARG(Int, msid)
ARG(Int, handle)
SET_CLIENT_ID(WMADRIVER_SMS_CLIENT, msid, handle, port)
INVOKE(status, jsr120_register_sms_midlet_port, (port, msid, handle))
END_STATUS()

// jsr120_unregister_sms_midlet_port()
START(WMA_STATUS, jsr120_unregister_sms_midlet_port, (jchar port))
DECL_STATUS()
DECL_ARG(jchar, port)
ARG(Short, port)
CLEAR_CLIENT_ID(WMADRIVER_SMS_CLIENT, port)
INVOKE(status, jsr120_unregister_sms_midlet_port, (port))
END_STATUS()

// jsr120_sms_delete_midlet_suite_msg()
START_VOID(jsr120_sms_delete_midlet_suite_msg, (AppIdType msid))
DECL_ARG(AppIdType, msid)
ARG(Int, msid)
INVOKE_VOID(jsr120_sms_delete_midlet_suite_msg, (msid))
END_VOID()

// jsr120_send_sms()
START(WMA_STATUS, jsr120_send_sms, (jchar msgType,
		              unsigned char address[],
		              unsigned char msgBuffer[],
		              jchar msgLen,
		              jchar sourcePort,
		              jchar destPort,
                              /* OUT */jint *bytesSent,
                              /* OUT */void **pContext))
DECL_STATUS()
DECL_ARG(jchar, msgType)
DECL_ARG_STRING(address)
DECL_ARG_ARRAY(unsigned char, msgBuffer, msgLen)
DECL_ARG(jchar, msgLen)
DECL_ARG(jchar, sourcePort)
DECL_ARG(jchar, destPort)
DECL_ARG(jint, bytesSent)
DECL_ARG(void *, pContext)
ARG(Short, msgType)
ARG_STRING(address)
ARG(Short, msgLen)
ARG_ARRAY(Byte, msgBuffer, msgLen)
ARG(Short, sourcePort)
ARG(Short, destPort)
INVOKE(status, jsr120_send_sms, (msgType,
		              address,
		              msgBuffer,
		              msgLen,
		              sourcePort,
		              destPort,
                      &bytesSent,
                      &pContext))
OUT_ARG(Int, bytesSent)
OUT_ARG(Int, pContext)
END_STATUS()

// jsr120_cbs_pool_peek_next_msg()
START(CbsMessage*, jsr120_cbs_pool_peek_next_msg, (jchar msgID))
DECL_LOCAL_STRUC(CbsMessage*, r_tmp)
DECL_ARG(jchar, msgID)
ARG(Short, msgID)
INVOKE(r_tmp, jsr120_cbs_pool_peek_next_msg, (msgID))
STRUC_SIZE(CbsMessage*, r_tmp, sizeof (CbsMessage) + r_tmp->msgLen, 4)
OUT_LOCAL_STRUC(Int, r_tmp, encodingType)
OUT_LOCAL_STRUC(Short, r_tmp, msgID)
OUT_LOCAL_STRUC(Short, r_tmp, msgLen)
OUT_LOCAL_STRUC_ARRAY(Byte, r_tmp, msgBuffer, r_tmp->msgLen)
END(r_tmp, NULL)

// jsr120_cbs_pool_retrieve_next_msg()
START(CbsMessage*, jsr120_cbs_pool_retrieve_next_msg, (jchar msgID))
DECL_LOCAL_STRUC(CbsMessage*, r_tmp)
DECL_ARG(jchar, msgID)
ARG(Short, msgID)
INVOKE(r_tmp, jsr120_cbs_pool_retrieve_next_msg, (msgID))
STRUC_SIZE(CbsMessage*, r_tmp, sizeof (CbsMessage) + r_tmp->msgLen, 4)
OUT_LOCAL_STRUC(Int, r_tmp, encodingType)
OUT_LOCAL_STRUC(Short, r_tmp, msgID)
OUT_LOCAL_STRUC(Short, r_tmp, msgLen)
OUT_LOCAL_STRUC_ARRAY(Byte, r_tmp, msgBuffer, r_tmp->msgLen)
FREE_STRUC(jsr120_cbs_delete_msg, r_tmp)
END(r_tmp, NULL)

// jsr120_cbs_pool_peek_next_msg1()
START(CbsMessage*, jsr120_cbs_pool_peek_next_msg1, (jchar msgID, jint isNew))
DECL_LOCAL_STRUC(CbsMessage*, r_tmp)
DECL_ARG(jchar, msgID)
DECL_ARG(jint, isNew)
ARG(Short, msgID)
ARG(Int, isNew)
INVOKE(r_tmp, jsr120_cbs_pool_peek_next_msg1, (msgID, isNew))
STRUC_SIZE(CbsMessage*, r_tmp, sizeof (CbsMessage) + r_tmp->msgLen, 4)
OUT_LOCAL_STRUC(Int, r_tmp, encodingType)
OUT_LOCAL_STRUC(Short, r_tmp, msgID)
OUT_LOCAL_STRUC(Short, r_tmp, msgLen)
OUT_LOCAL_STRUC_ARRAY(Byte, r_tmp, msgBuffer, r_tmp->msgLen)
END(r_tmp, NULL)


// jsr120_sms_pool_peek_next_msg()
START(SmsMessage*, jsr120_sms_pool_peek_next_msg, (jchar msgID))
DECL_LOCAL_STRUC(SmsMessage*, r_tmp)
DECL_ARG(jchar, msgID)
ARG(Short, msgID)
INVOKE(r_tmp, jsr120_sms_pool_peek_next_msg, (msgID))
STRUC_SIZE(SmsMessage*, r_tmp, sizeof (SmsMessage) + MAX_ADDR_LEN + r_tmp->msgLen, 7)
OUT_LOCAL_STRUC(Short, r_tmp, sourcePortNum)
OUT_LOCAL_STRUC(Short, r_tmp, destPortNum)
OUT_LOCAL_STRUC(Long, r_tmp, timeStamp)
OUT_LOCAL_STRUC(Short, r_tmp, encodingType)
OUT_LOCAL_STRUC(Short, r_tmp, msgLen)
OUT_LOCAL_STRUC_ARRAY(Byte, r_tmp, msgAddr, MAX_ADDR_LEN)
OUT_LOCAL_STRUC_ARRAY(Byte, r_tmp, msgBuffer, r_tmp->msgLen)
END(r_tmp, NULL)

// jsr120_sms_pool_retrieve_next_msg()
START(SmsMessage*, jsr120_sms_pool_retrieve_next_msg, (jchar msgID))
DECL_LOCAL_STRUC(SmsMessage*, r_tmp)
DECL_ARG(jchar, msgID)
ARG(Short, msgID)
INVOKE(r_tmp, jsr120_sms_pool_retrieve_next_msg, (msgID))
STRUC_SIZE(SmsMessage*, r_tmp, sizeof (SmsMessage) + MAX_ADDR_LEN + r_tmp->msgLen, 7)
OUT_LOCAL_STRUC(Short, r_tmp, sourcePortNum)
OUT_LOCAL_STRUC(Short, r_tmp, destPortNum)
OUT_LOCAL_STRUC(Long, r_tmp, timeStamp)
OUT_LOCAL_STRUC(Short, r_tmp, encodingType)
OUT_LOCAL_STRUC(Short, r_tmp, msgLen)
OUT_LOCAL_STRUC_ARRAY(Byte, r_tmp, msgAddr, MAX_ADDR_LEN)
OUT_LOCAL_STRUC_ARRAY(Byte, r_tmp, msgBuffer, r_tmp->msgLen)
FREE_STRUC(jsr120_sms_delete_msg, r_tmp)
END(r_tmp, NULL)

// jsr120_sms_pool_peek_next_msg1()
START(SmsMessage*, jsr120_sms_pool_peek_next_msg1, (jchar msgID, jint isNew))
DECL_LOCAL_STRUC(SmsMessage*, r_tmp)
DECL_ARG(jchar, msgID)
DECL_ARG(jint, isNew)
ARG(Short, msgID)
ARG(Int, isNew)
INVOKE(r_tmp, jsr120_sms_pool_peek_next_msg1, (msgID, isNew))
STRUC_SIZE(SmsMessage*, r_tmp, sizeof (SmsMessage) + STRING_LEN(r_tmp->msgAddr) + 1 + r_tmp->msgLen, 7)
OUT_LOCAL_STRUC(Short, r_tmp, sourcePortNum)
OUT_LOCAL_STRUC(Short, r_tmp, destPortNum)
OUT_LOCAL_STRUC(Long, r_tmp, timeStamp)
OUT_LOCAL_STRUC(Short, r_tmp, encodingType)
OUT_LOCAL_STRUC(Short, r_tmp, msgLen)
OUT_LOCAL_STRUC_STRING(r_tmp, msgAddr)
OUT_LOCAL_STRUC_ARRAY(Byte, r_tmp, msgBuffer, r_tmp->msgLen)
END(r_tmp, NULL)

// jsr120_number_of_sms_segments()
START(WMA_STATUS, jsr120_number_of_sms_segments, (unsigned char msgBuffer[], 
                                                  jint msgLen, jint msgType,
                                                  jboolean hasPort, 
                                                  /* OUT */jint *numSegments))
DECL_STATUS()
DECL_ARG(jint, msgType)
DECL_ARG(jint, msgLen)
DECL_ARG_ARRAY(unsigned char, msgBuffer, msgLen)
DECL_ARG(jboolean, hasPort)
DECL_ARG(jint, numSegments)
ARG(Int, msgType)
ARG(Int, msgLen)
ARG_ARRAY(Byte, msgBuffer, msgLen)
ARG(Byte, hasPort)
INVOKE(status, jsr120_number_of_sms_segments, (msgBuffer, msgLen, msgType,
                                               hasPort, &numSegments))
OUT_ARG(Int, numSegments)
END_STATUS()


// jsr120_cbs_pool_add_msg()
START_SELF(jsr120_cbs_pool_add_msg, (CbsMessage *cbs))
DECL_ARG(CbsMessage *, cbs)
ARG(Int, cbs)
INVOKE_AND_END(jsr120_cbs_pool_add_msg, (cbs))

// jsr120_sms_pool_add_msg()
START_SELF(jsr120_sms_pool_add_msg, (SmsMessage *sms))
DECL_ARG(SmsMessage *, sms)
ARG(Int, sms)
INVOKE_AND_END(jsr120_sms_pool_add_msg, (sms))

/*
// javanotify_incoming_sms()
START_VOID(javanotify_incoming_sms, 
        (javacall_sms_encoding   msgType,
        char*                   sourceAddress,
        unsigned char*          msgBuffer,
        int                     msgBufferLen,
        unsigned short          sourcePortNum,
        unsigned short          destPortNum,
        javacall_int64          timeStamp))
DECL_ARG(javacall_sms_encoding, msgType)
DECL_ARG_STRING(sourceAddress)
DECL_ARG(int, msgBufferLen)
DECL_ARG_ARRAY(unsigned char, msgBuffer, msgBufferLen)
DECL_ARG(unsigned short, sourcePortNum)
DECL_ARG(unsigned short, destPortNum)
DECL_ARG(javacall_int64, timeStamp)
ARG(Short, msgType)
ARG_STRING(sourceAddress)
ARG(Short, msgBufferLen)
ARG_ARRAY(Byte, msgBuffer, msgBufferLen)
ARG(Short, sourcePortNum)
ARG(Short, destPortNum)
INVOKE_VOID(javanotify_incoming_sms, 
                    (msgType,
		              sourceAddress,
		              msgBuffer,
		              msgBufferLen,
		              sourcePortNum,
		              destPortNum, timeStamp))
END_VOID()

// javanotify_incoming_cbs()
START_VOID(javanotify_incoming_cbs, 
        (javacall_sms_encoding   msgType,
        unsigned short          destPortNum,
        unsigned char*          msgBuffer,
        int                     msgBufferLen))
DECL_ARG(javacall_sms_encoding, msgType)
DECL_ARG(int, msgBufferLen)
DECL_ARG_ARRAY(unsigned char, msgBuffer, msgBufferLen)
DECL_ARG(unsigned short, destPortNum)
ARG(Short, msgType)
ARG(Short, msgBufferLen)
ARG_ARRAY(Byte, msgBuffer, msgBufferLen)
ARG(Short, destPortNum)
INVOKE_VOID(javanotify_incoming_cbs, 
                    (msgType,
		              destPortNum,
		              msgBuffer,
		              msgBufferLen))
END_VOID()
*/
END_INTERFACE()

DECL_FREE_FUNCTION(jsr120_cbs_delete_msg, CbsMessage*)
DECL_FREE_FUNCTION(jsr120_sms_delete_msg, SmsMessage*)

