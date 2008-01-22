/*
 *
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

/*
 * SUBSYSTEM: WMA (Wireless Messaging API)
 * FILE:      smsProtocol.c
 * OVERVIEW:  This file handles WMA functions such as
 *            - opening an SMS connection
 *            - sending an SMS message to a destination address
 *            - receiving an SMS message
 *            - closing the connection
 */

#include <stdio.h>
#include <string.h>
#include <kni.h>
#include <sni.h>

#ifdef ENABLE_PCSL
  #include <pcsl_memory.h>
#else
  #define pcsl_mem_malloc malloc
  #define pcsl_mem_free free
#endif

#if (ENABLE_CDC != 1)
  #include <midpServices.h> //WMA_SMS_READ_SIGNAL, WMA_SMS_WRITE_SIGNAL, etc
  #include <midp_thread.h> //midp_thread_wait
#else
  #include "jsr120_signals.h"
  #include "wmaInterface.h"
#endif

#ifdef ENABLE_MIDP
  #include "midpError.h"
#else
  static const char* const midpOutOfMemoryError = "java/lang/OutOfMemoryError";
  static const char* const midpIOException = "java/io/IOException"; 
  static const char* const midpRuntimeException = "java/lang/RuntimeException";
  static const char* const midpIllegalArgumentException = "java/lang/IllegalArgumentException";
#endif

#if (ENABLE_CDC == 1)
#define JSR120_KNI_LAYER
#ifdef JSR_120_ENABLE_JUMPDRIVER
#include <jsr120_jumpdriver.h>
#include <JUMPEvents.h>
#endif
#endif

#include <app_package.h>

#include <jsr120_sms_protocol.h>
#include <jsr120_sms_pool.h>
#include <jsr120_sms_listeners.h>

/*
 * Protocol methods
 */
unsigned char smsBuffer[sizeof(SmsMessage) + MAX_MSG_BUFFERSIZE - 1];

typedef struct {
    unsigned char *pAddress;
    unsigned char *pMessageBuffer;
    void *pdContext;
} jsr120_sms_message_state_data;

/**
 * Opens an SMS connection.
 *
 * @param host The host name.
 * @param port The port number used to match against incoming SMS messages.
 *
 * @return A handle to the open SMS connection.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_sms_Protocol_open0) {

    int handle = 0;
    int port;
    /* The midlet suite name for this connection. */
    AppIdType msid = UNUSED_APP_ID;

    port = KNI_GetParameterAsInt(3);

    jsr120_sms_pool_init();

    /* When port is 0 then return else continue */
    if (port) {
        /* Create handles for all Java objects. */
        KNI_StartHandles(1);
        KNI_DeclareHandle(javaStringHost);

        /* Pick up the host string. */
        KNI_GetParameterAsObject(1, javaStringHost);
        /* Pick up the Midlet Suite ID. */
        msid = KNI_GetParameterAsInt(2);

        do {
            /*
             * Get unique handle, to identify this
             * SMS "session"..
             */
            handle = (int)(pcsl_mem_malloc(1));
            if (handle == 0) {
               KNI_ThrowNew(midpOutOfMemoryError,
                            "Unable to start SMS.");
                break;
            }

            /*
             * Register the port with the message pool, only if this
             * is a server connection (NULL host name.).
             */
//          if (!KNI_IsNullHandle(javaStringHost) && KNI_GetStringLength(javaStringHost) > 0) {
            if (KNI_IsNullHandle(javaStringHost)) {

                /* register SMS port with SMS pool */
                if (jsr120_is_sms_midlet_listener_registered((jchar)port) == WMA_ERR) {

                    if (jsr120_register_sms_midlet_listener((jchar)port,
                                                        msid,
                                                        handle) == WMA_ERR) {

                        KNI_ThrowNew(midpIOException, "already in use");
                        break;
                    }
                } else {
                    /* port already registered, throw exception */
                    KNI_ThrowNew(midpIOException, "already in use");
                    break;
                }
            }
        } while (0);

        KNI_EndHandles();
    }
    KNI_ReturnInt(handle);
}

/**
 * Internal helper function implementing connection close routine
 *
 * @param port The port associated with this connection.
 * @param handle The handle of the open SMS message connection.
 * @param deRegister Deregistration os the port when parameter is 1.
 */
static void closeConnection(int port, int handle, int deRegister) {

    if (port > 0 && handle != 0) {

#if (ENABLE_CDC != 1)
        /* unblock any blocked threads */
        jsr120_sms_unblock_thread((jint)handle, WMA_SMS_READ_SIGNAL);
#else
        jsr120_throw_signal(handle, 0);
#endif

        if (deRegister) {
            /** unregister SMS port from SMS pool */
            jsr120_unregister_sms_midlet_listener((jchar)port);

            /* Release the handle associated with this connection. */
            pcsl_mem_free((void *)handle);
        }

    }
}

/**
 * Closes an open SMS connection.
 *
 * @param port The port associated with this connection.
 * @param handle The handle of the open SMS message connection.
 * @param deRegister Deregistration port when parameter is 1.
 *
 * @return <code>0</code> if successful; <code>-1</code> if failed.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_sms_Protocol_close0) {
    int status = 0;
    int port = 0;
    int handle = 0;

    /** Deregistration flag. */
    int deRegister;

    port = KNI_GetParameterAsInt(1);
    handle = KNI_GetParameterAsInt(2);
    deRegister = KNI_GetParameterAsInt(3);

    closeConnection(port, handle, deRegister);

    KNI_ReturnInt(status);
}

#define ENABLE_REENTRY (ENABLE_CDC != 1)

/**
 * Sends an SMS message.
 *
 * @param handle The handle to the open SMS connection.
 * @param messageType The type of message: binary or text.
 * @param address The SMS-formatted address.
 * @param destPort The port number of the recipient.
 * @param sourcePort The port number of the sender.
 * @param messageBuffer The buffer containing the SMS message.
 *
 * @return Always returns <code>0</code>.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_sms_Protocol_send0) {
    WMA_STATUS status = WMA_ERR;
    jint messageLength = 0;
    jint messageType;
    jint sourcePort;
    jint destPort;
    jint handle;
    jint msAddress_len;
    jchar* msAddress_data;
    int i;
    unsigned char *pAddress = NULL;
    unsigned char *pMessageBuffer = NULL;
    jboolean stillWaiting = KNI_FALSE;
    jboolean trySend = KNI_FALSE;
    void *pdContext = NULL;
#if ENABLE_REENTRY
    MidpReentryData *info;
    jsr120_sms_message_state_data *messageStateData = NULL;
#endif
    jboolean isOpen;

    KNI_StartHandles(4);

    KNI_DeclareHandle(this);
    KNI_DeclareHandle(thisClass);
    KNI_GetThisPointer(this);
    KNI_GetObjectClass(this, thisClass);
    isOpen = KNI_GetBooleanField(this, KNI_GetFieldID(thisClass, "open", "Z"));
    
    if (isOpen) { /* No close in progress */
        KNI_DeclareHandle(messageBuffer);
        KNI_DeclareHandle(address);

        handle = KNI_GetParameterAsInt(1);
        messageType = KNI_GetParameterAsInt(2);
        KNI_GetParameterAsObject(3, address);
        destPort = KNI_GetParameterAsInt(4);
        sourcePort = KNI_GetParameterAsInt(5);
        KNI_GetParameterAsObject(6, messageBuffer);

        do {
#if ENABLE_REENTRY
            info = (MidpReentryData*)SNI_GetReentryData(NULL);
            if (info == NULL) {	  /* First invocation. */
#endif
                if (KNI_IsNullHandle(address)) {

                    KNI_ThrowNew(midpIllegalArgumentException, NULL);
                    break;
                } else {
                    msAddress_len = KNI_GetStringLength(address);
                    msAddress_data = (jchar *)pcsl_mem_malloc(msAddress_len * sizeof (jchar));
                    if (msAddress_data == NULL) {

                        KNI_ThrowNew(midpOutOfMemoryError, NULL);
                        break;
                    } else {

                        KNI_GetStringRegion(address, 0, msAddress_len, msAddress_data);
                        pAddress = (unsigned char*)pcsl_mem_malloc(msAddress_len + 1);
                        if (pAddress != NULL) {
                            for (i = 0; i < msAddress_len; i++) {
                                pAddress[i] = (unsigned char)msAddress_data[i];
                            }	
                            pAddress[msAddress_len] = 0;
                        }
                        //pAddress = (unsigned char *)midpJcharsToChars(msAddress);
                        pcsl_mem_free(msAddress_data);

                        if (!KNI_IsNullHandle(messageBuffer)) {
                            messageLength = KNI_GetArrayLength(messageBuffer);
                        }
                        if (messageLength >= 0) {
                            if (messageLength > 0) {
                                pMessageBuffer = (unsigned char *)pcsl_mem_malloc(messageLength);
                                memset(pMessageBuffer, 0, messageLength);
                                KNI_GetRawArrayRegion(messageBuffer, 0, messageLength,
                                                      (jbyte *)pMessageBuffer);
                            }

                            trySend = KNI_TRUE;
                        }
                    }
                }
#if ENABLE_REENTRY
            } else { /* Reinvocation after unblocking the thread. */

                if (info->pResult == NULL) {
                    /* waiting for mms_send_completed event */
                    if (info->status == WMA_ERR) {
                        KNI_ThrowNew(midpInterruptedIOException, "Sending SMS");
                    }
                    break;
                }
                messageStateData = info->pResult;
                pMessageBuffer = messageStateData->pMessageBuffer;
                pAddress = messageStateData->pAddress;
                pdContext = messageStateData->pdContext;

                trySend = KNI_TRUE;
            }
#endif

            if (trySend == KNI_TRUE) {
                /* send message. */
                status = jsr120_send_sms((jchar)messageType,
                                         pAddress,
                                         pMessageBuffer,
                                         (jchar)messageLength,
                                         (jchar)sourcePort,
                                         (jchar)destPort,
                                         handle,
                                         &pdContext);

                if (status == WMA_ERR) {
                    KNI_ThrowNew(midpIOException, "Sending SMS");
                    break;
                } 
#if ENABLE_REENTRY
                else if (status == WMA_NET_WOULDBLOCK) {
                    if (messageStateData == NULL) {
                        messageStateData =
                            (jsr120_sms_message_state_data *)pcsl_mem_malloc(
                                sizeof(*messageStateData));
                        messageStateData->pMessageBuffer = pMessageBuffer;
                        messageStateData->pAddress = pAddress;
                    }

                    messageStateData->pdContext = pdContext;

                    /* Block calling Java Thread. */
                    midp_thread_wait(WMA_SMS_WRITE_SIGNAL, handle,
                                     messageStateData);

                    stillWaiting = KNI_TRUE;
                    break;
                } else {
                    /* waiting for sms_send_completed event */
                    midp_thread_wait(WMA_SMS_WRITE_SIGNAL, handle, NULL);
                }
#endif
            }
        } while (0);

        if (!stillWaiting) {
            pcsl_mem_free(pMessageBuffer);
            pcsl_mem_free(pAddress);
        }
    }

    KNI_EndHandles();

    KNI_ReturnInt(0); /* currently ignored. */
}

/**
 * Receives an SMS message.
 *
 * @param port The port number to be matched against incoming SMS messages.
 * @param handle The handle to the open SMS message connection.
 * @param messageObject The Java message object to be populated.
 *
 * @return The length of the SMS message (in bytes).
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_sms_Protocol_receive0) {
#if ENABLE_REENTRY
    MidpReentryData *info = (MidpReentryData*)SNI_GetReentryData(NULL);
#endif
    int port, handle;
    int messageLength = -1;
    SmsMessage *psmsData = NULL;
    /* The midlet suite name for this connection. */
    AppIdType msid = UNUSED_APP_ID;
    jboolean isOpen;

    KNI_StartHandles(6);

    KNI_DeclareHandle(this);
    KNI_DeclareHandle(thisClass);
    KNI_GetThisPointer(this);
    KNI_GetObjectClass(this, thisClass);
    isOpen = KNI_GetBooleanField(this, KNI_GetFieldID(thisClass, "open", "Z"));

    if (isOpen) { /* No close in progress */
        KNI_DeclareHandle(messageClazz);
        KNI_DeclareHandle(messageObject);
        KNI_DeclareHandle(addressArray);
        KNI_DeclareHandle(byteArray);

        port = KNI_GetParameterAsInt(1);
        msid = KNI_GetParameterAsInt(2);
        handle = KNI_GetParameterAsInt(3);
        KNI_GetParameterAsObject(4, messageObject);

        do {
#if ENABLE_REENTRY
            if (!info) {
#endif
                psmsData = jsr120_sms_pool_peek_next_msg((jchar)port);
                if (psmsData == NULL) {
#if ENABLE_REENTRY
                    /* block and wait for a message. */
                    midp_thread_wait(WMA_SMS_READ_SIGNAL, handle, NULL);
#else
        do {
            CVMD_gcSafeExec(_ee, {
                jsr120_wait_for_signal(handle, WMA_SMS_READ_SIGNAL);
            });
            psmsData = jsr120_sms_pool_peek_next_msg((jchar)port);
            isOpen = KNI_GetBooleanField(this, KNI_GetFieldID(thisClass, "open", "Z"));
        } while (psmsData == NULL && isOpen);
#endif
                }
#if ENABLE_REENTRY
            } else {
                /* reentry. */
                psmsData = jsr120_sms_pool_peek_next_msg((jchar)port);
            }
#endif

            if (psmsData != NULL) {
                if (NULL != (psmsData = jsr120_sms_pool_retrieve_next_msg(port))) {

                    KNI_GetObjectClass(messageObject, messageClazz);
                    if(KNI_IsNullHandle(messageClazz)) {
                        KNI_ThrowNew(midpOutOfMemoryError, NULL);
                        break;
                    } else {
                        jfieldID message_field_id = KNI_GetFieldID(messageClazz, "message","[B");
                        jfieldID address_field_id = KNI_GetFieldID(messageClazz, "address","[B");
                        jfieldID port_field_id = KNI_GetFieldID(messageClazz, "port","I");
                        jfieldID sentAt_field_id = KNI_GetFieldID(messageClazz, "sentAt","J");
                        jfieldID messageType_field_id = KNI_GetFieldID(messageClazz, "messageType","I");
                        if ((message_field_id == 0) || (address_field_id == 0) ||
                            (port_field_id == 0) || (sentAt_field_id == 0) ||
                            (messageType_field_id == 0)) {

                            /* REPORT_ERROR(LC_WMA, "ERROR can't get class field ID"); */

                            KNI_ThrowNew(midpRuntimeException, NULL);
                            break;
                        } else {
                            messageLength = psmsData->msgLen;
                            if (messageLength >= 0) {
                                int i = 0;
                                int addressLength = strlen((char *)psmsData->msgAddr);

                                if (addressLength > MAX_ADDR_LEN){
                                    addressLength = MAX_ADDR_LEN;
                                }

                                if (messageLength > 0) {
                                    SNI_NewArray(SNI_BYTE_ARRAY, messageLength, byteArray);
                                    if (KNI_IsNullHandle(byteArray)) {

                                        KNI_ThrowNew(midpOutOfMemoryError, NULL);
                                        break;
                                    } else {
                                        for (i = 0; i < messageLength; i++) {
                                            KNI_SetByteArrayElement(byteArray, i,
                                                                    psmsData->msgBuffer[i]);
                                        }

                                        KNI_SetObjectField(messageObject, message_field_id,
                                                           byteArray);
                                    }
                                }

                                if (addressLength >= 0) {
                                    SNI_NewArray(SNI_BYTE_ARRAY, addressLength, addressArray);
                                    if (KNI_IsNullHandle(addressArray)) {

                                        KNI_ThrowNew(midpOutOfMemoryError, NULL);
                                        break;
                                    } else {
                                        for (i = 0; i < addressLength; i++) {
                                            KNI_SetByteArrayElement(addressArray, i,
                                                                 psmsData->msgAddr[i]);
                                        }

                                        KNI_SetObjectField(messageObject, address_field_id,
                                                           addressArray);
                                    }
                                }

                                KNI_SetIntField(messageObject, port_field_id,
                                                psmsData->sourcePortNum);
                                KNI_SetLongField(messageObject, sentAt_field_id,
                                                 psmsData->timeStamp);
                                KNI_SetIntField(messageObject, messageType_field_id,
                                                psmsData->encodingType);
                           }
                       }
                   }
               }
            }
        } while (0);

        jsr120_sms_delete_msg(psmsData);
    }

    KNI_EndHandles();

    KNI_ReturnInt(messageLength);
}

/**
 * Waits for a message to become available.
 *
 * @param port The port number to be matched against incoming SMS messages.
 * @param handle The handle to the open SMS message connection.
 *
 * @return the length of the unformatted message to be received.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_sms_Protocol_waitUntilMessageAvailable0) {
#if ENABLE_REENTRY
    MidpReentryData *info = (MidpReentryData*)SNI_GetReentryData(NULL);
#endif
    int port;
    int handle;
    int messageLength = -1;
    SmsMessage *pSMSData = NULL;
    jboolean isOpen;

    KNI_StartHandles(2);

    KNI_DeclareHandle(this);
    KNI_DeclareHandle(thisClass);
    KNI_GetThisPointer(this);
    KNI_GetObjectClass(this, thisClass);
    isOpen = KNI_GetBooleanField(this, KNI_GetFieldID(thisClass, "open", "Z"));

    if (isOpen) { /* No close in progress */
        port = KNI_GetParameterAsInt(1);
        handle = KNI_GetParameterAsInt(2);

        if (port == 0) {
            /*
             * A receive thread wouldn't have been started with a port
             * of 0. But a connection can be closed and port set to 0,
             * just before this method is called.
             * The receiverThread uses the IllegalArgumentException to
             * check for this situation and gracefully terminates.
             */
            KNI_ThrowNew(midpIllegalArgumentException, "No port available.");

        } else {

            /* See if there is a new message waiting in the pool. */
            pSMSData = jsr120_sms_pool_peek_next_msg1((jchar)port, 1);
            if (pSMSData != NULL) {
                messageLength = pSMSData->msgLen;
            } else {
                /* Check needStopReceiver value to minimise deadlock risk */
                jboolean needStopReceiver = KNI_GetBooleanField(this, KNI_GetFieldID(thisClass, "needStopReceiver", "Z"));
                if (!needStopReceiver) {
#if ENABLE_REENTRY
                    if (!info) {
                        /* Block and wait for a message. */
                        midp_thread_wait(WMA_SMS_READ_SIGNAL, handle, NULL);
                    } else {
                        /* May have been awakened due to interrupt. */
                        messageLength = -1;
                    }
#else
                    CVMD_gcSafeExec(_ee, {
                        jsr120_wait_for_signal(handle, WMA_SMS_READ_SIGNAL);
                        pSMSData = jsr120_sms_pool_peek_next_msg1((jchar)port, 1);
                        if (pSMSData != NULL) {
                             messageLength = pSMSData->msgLen;
                        }
                    }); 
#endif
                }
            }
        }
    }

    KNI_EndHandles();

    KNI_ReturnInt(messageLength);
}

/**
 * Computes the number of transport-layer segments that would be required to
 * send the given message.
 *
 * @param msgBuffer The message to be sent.
 * @param msgLen The length of the message.
 * @param msgType The message type: binary or text.
 * @param hasPort Indicates if the message includes a source or destination port
 *     number.
 *
 * @return The number of transport-layer segments required to send the message.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_sms_Protocol_numberOfSegments0) {

    unsigned char* msgBuffer = NULL;
    int msgLen = 0;
    int msgType = 0;
    jboolean hasPort = KNI_FALSE;
    WMA_STATUS status;

    jint segments = 0;

    KNI_StartHandles(1);
    KNI_DeclareHandle(msgBufferObject);

    KNI_GetParameterAsObject(1, msgBufferObject);
    msgLen = KNI_GetParameterAsInt(2);
    msgType = KNI_GetParameterAsInt(3);
    hasPort = KNI_GetParameterAsBoolean(4);

    if (!KNI_IsNullHandle(msgBufferObject)) {

        /*
         * Pick up the length of the message, which should be the same as
         * <code>msgLen</code>. This is just done here as a formality.
         */
        int length = KNI_GetArrayLength(msgBufferObject);

        if (length > 0) {
            msgBuffer = (unsigned char *)pcsl_mem_malloc(length);
            memset(msgBuffer, 0, length);
            KNI_GetRawArrayRegion(msgBufferObject, 0, length, (jbyte*)msgBuffer);
	}
	/* Compute the number of segments required to send the message. */
	status = jsr120_number_of_sms_segments(msgBuffer, msgLen, msgType,
					       hasPort, &segments);
	/* Limit SMS messages to 3 segments. */
        if (segments > 3){
	  segments = 0;
	}
    }

    pcsl_mem_free(msgBuffer);
    KNI_EndHandles();
    KNI_ReturnInt(segments);
}

/*
 * Native finalization of the Java object.
 *
 * CLASS:    com.sun.midp.io.j2me.sms.Protocol
 * TYPE:     virtual native function
 * INTERFACE (operand stack manipulation):
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_io_j2me_sms_Protocol_finalize) {
    int port;
    int handle;
    jboolean isOpen;

    KNI_StartHandles(2);
    KNI_DeclareHandle(this);
    KNI_DeclareHandle(thisClass);

    KNI_GetThisPointer(this);
    KNI_GetObjectClass(this, thisClass);
    isOpen = KNI_GetBooleanField(this, KNI_GetFieldID(thisClass, "open", "Z"));

    if (isOpen) {
        port = KNI_GetIntField(this, KNI_GetFieldID(thisClass, "m_iport", "I"));
        handle = KNI_GetIntField(this, KNI_GetFieldID(thisClass, "connHandle", "I"));

        closeConnection(port, handle, 1);
    }

    KNI_EndHandles();

    KNI_ReturnVoid();
}

