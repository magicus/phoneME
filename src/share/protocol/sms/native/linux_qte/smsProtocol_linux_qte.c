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
 * 
 * This source file is specific for Qt-based configurations.
 */

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <jsr120_sms_protocol.h>
#include <jsr120_sms_listeners.h>
#include <pcsl_memory.h>

#define SOCKET_ERROR (-1)

extern WMA_STATUS jsr120_sms_write(jchar msgType, unsigned char *address,
     		                      unsigned char *msgBuffer,
			              jchar msgLen,
			              jchar sourcePort, jchar destPort,
			              jint *bytesWritten,
                                      void **pContext);
/**
 * Sends a Text or Binary message. The calling function does not have to worry about the
 * details of the underlying communication protocol (such as GSM 3.40, CDMA IS-637, or others).
 * To avoid blocking the Java VM for a long time, this method works asynchronously and
 * returns before the message is delivered to the network. The target's native platform
 * handles the whole communication session and returns the result via an asynchronous message
 * or callback function.
 *
 * @param msgType message string type:Text(0) or Binary(1)
 *		  The target device should decide the DCS (Data Coding Scheme)
 *		  in PDU according to this parameter and the  message contents.
 *		  If  the target device is compliant with GSM 3.40, then for a Binary
 *		  Message,  the DCS in PDU should be 8-bit binary.
 *		  For a	 Text Message, the target device should decide the DCS	according to
 *		  the  message contents. When all characters in the message contents are in
 *		  the GSM 7-bit alphabet, the DCS should be GSM 7-bit; otherwise, it should
 *		  be  UCS-2.
 * @param address the target SMS address for the message. The format of the address
 *		  is expected to be compliant with MSIDN, for example, +123456789
 * @param msgBuffer the message body (payload) to be sent
 * @param msgLen the length of the message body
 * @param sourcePort the sender's reply port number
 * @param destPort the receiver's port number
 * @param bytesSent The number of bytes sent is returned in this variable
 * @param pContext pointer where to save context of asynchronous operation.
 *
 * @return One of the following WMA_STATUS codes: <code>WMA_OK</code> on
 *     success; <code>WMA_ERROR</code> if there was an error.
 */
WMA_STATUS jsr120_send_sms(jchar msgType,
                              unsigned char address[],
                              unsigned char msgBuffer[],
                              jchar msgLen,
                              jchar sourcePort,
                              jchar destPort,
                              int handle, 
                              /* OUT */void **pContext)
{
    WMA_STATUS status = WMA_ERR; 
    jint bytesSent;

    /* Calls platform code to send SMS. */
    status =  jsr120_sms_write(msgType, address,
	                       msgBuffer,
	                       msgLen,
			       sourcePort,
                               destPort,
			       &bytesSent,
                               pContext);


    if (status == WMA_NET_SUCCESS) {
        status = WMA_OK;
    }

    jsr120_sms_message_sent_notifier(handle, status);
    return status;

}

/*
 * SMS Listening methods
 */

/**
 * Removes a registered message port number. After removing this port number, no message
 * whose destination port number is equal to this port number, will be received. If the
 * specified port number is not registered, then an error code will be returned.
 *
 * @param port The port number to be removed.
 *
 * @return Returns enum WMA_STATUS, WMA_SUCCESS on success or
 *	   WMA_ERROR on error.
 */
WMA_STATUS jsr120_remove_sms_listening_port(jchar port) {
    (void)port;
    return WMA_OK;
}

/**
 * Registers a message port number. After registering this port number, MIDP WMA
 * should continue to listen for incoming messages matching the registered port
 * number. The target device communication facilities should have the ability to
 * distinguish these messages by port number and dispatch them to WMA. If this
 * port number is occupied by another target device native application or
 * another WMA application, then an error code will be returned.
 *
 * @param port The registered port number.
 *
 * @return Returns enum WMA_STATUS, WMA_SUCCESS on success or
 *	   WMA_ERROR on error.
 */
WMA_STATUS jsr120_add_sms_listening_port(jchar port) {
    (void)port;
    return WMA_OK;
}

/**
 * After calling  setSMSListeningPort(), a WMA Application will continue to listen for
 * incoming messages whose destination port number matches the registered port number.
 * this callback function will be called by platform once an incoming message is ready.
 * If the incoming message contains more than one protocol (PDU) segments, target device
 * native layer should concatenate all segments into a completed message and forward to MIDP WMA.
 * Platform should allocate the msgBuffer and put whole incoming message contents into this buffer.
 *
 * @param msgType       The encoding type of the incoming message
 *                      (GSM 7-bit alphabet, Unicode or 8-bit Binary) 
 * @param sourceAddress The senders address. NULL terminated string.
 * @param msgBuffer     The incoming message body. 
 * @param msgLen        The length of the incoming message body. 
 * @param sourcePortNum source port number of the incoming message
 * @param destPortNum   The destination port number of the incoming message.[unsigned short]
 * @param timeStamp	When the message is received (SCT: message Service Center Time)[long].
 */
void jsr120_notify_incoming_sms(jchar msgType, char *sourceAddress,
                                unsigned char *msgBuffer, jint msgLen,
                                jchar sourcePortNum, jchar destPortNum,
                                jlong timeStamp) {
    (void)msgType;
    (void)sourceAddress;
    (void)msgBuffer;
    (void)msgLen;
    (void)sourcePortNum;
    (void)destPortNum;
    (void)timeStamp;
}

/**
 * The native software platform on the target device calls this API to notify that a message has
 * been sent. When wma_sendSMSMessage()is called, the native software platform on the target
 * device is expected to deliver the message to the network. It then calls this callback function.
 *
 * @param bytesSent Number of bytes sent.
 *		    >= 0 on success
 *		    -1 on error
 */
void jsr120_notify_sms_send_completed(int handle, WMA_STATUS result) {

    jsr120_sms_message_sent_notifier(handle, result);
}

/**:
 * Returns the number of segments that would be needed in the underlying
 * protocol to send a specified message. The specified message is included as a
 * parameter of this function. Note that this method does not actually send the
 * message. It will only calculate the number of protocol segments needed for
 * sending the message.
 *
 * @param msgBuffer The message body.
 * @param msgLen Message body length (in bytes).
 * @param msgType Message type: Binary or Text.
 * @param hasPort indicates if the message includes source or destination port number.
 * @param numSegments The number of message segments that would be required to send the
 *			message is returned here.
 *
 * @return <code>WMA_OK</code> when successful;
 *     <code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS jsr120_number_of_sms_segments(unsigned char msgBuffer[], jint msgLen, jint msgType,
                                            jboolean hasPort, /* OUT */jint *numSegments) {

    /** The number of bytes in one message fragment. */
    jint fragmentSize = 0;

    /** Extra header size for concatenated messages. */
    jint headerSize = 0;

    /**
     * Extra header space used for the port number in the destination address.
     */
    jint portHeaderSize = 0;

    /* Unused parameter */
    (void)msgBuffer;

    /* The number of segments required to send the message. */
    *numSegments = 0;

    /* When a port number exists, it occupies part of the header. */
    if (hasPort == KNI_TRUE) {
        portHeaderSize = 7;
    }

    /*
     * Determine the sizes for the various encoding types.
     */
    switch (msgType) {
        case GSM_TEXT:
            /* 160/152/145 */
            fragmentSize = 160;  /* The number of 7-bit values. */
            headerSize = 8;
            break;
        case GSM_UCS2:
            /* 140/132/126 */
            fragmentSize = 140;
            headerSize = 8;
            if (portHeaderSize != 0) {
                portHeaderSize = 6;
            }
            break;
        case GSM_BINARY:
        default:
            /* 140/133/126 */
            fragmentSize = 140;
            headerSize = 7;
            if (portHeaderSize != 0) {
                portHeaderSize = 6;
            }
            break;
    }

    fragmentSize = fragmentSize - portHeaderSize;

    if (msgLen < fragmentSize) {
        *numSegments = 1;
    } else {
        fragmentSize = fragmentSize - headerSize;
        *numSegments = (msgLen + fragmentSize - 1) / fragmentSize;
    }

    return WMA_OK;
}

