/*
 *   
 *
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

#ifndef _JSR120_SMS_PROTOCOL_H_
#define _JSR120_SMS_PROTOCOL_H_

/**
 * @file
 * @ingroup stack
 */

/**
 * @defgroup sms Short Message Service (SMS)
 * @ingroup wma
 */

/**
 * @defgroup smsprotocol SMS Protocol Porting Interface
 * @ingroup sms
 * @brief Short Message Service protocol porting interface. \n
 * ##include <wma_sms_pool.h>
 * @{
 *
 * This file defines the Short Message Service protocol porting interfaces.
 * SMS communications operations such as open, close, read and write can be
 * accomplished through these interfaces. Every platform MUST have its own
 * implementation for these interfaces.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <kni.h>
#include <jsr120_types.h>
#include <jsr120_sms_pool.h>

/**
 * Returns the Short Message Service Center (SMSC) address. WMA applications might need to 
 * obtain the SMSC address to decide which recipient port number to use. The return value 
 * is a null-terminated string. The implementation of this API takes the responsibility 
 * for allocating memory for this string, and the calling function is responsible for freeing
 * the buffer.
 *
 * @param addrStrLen length of the address string is returned in this variable
 *
 * @param smscAddress The SMS Service Center address is returned. Its format is compliant 
 *                    with MSIDN, for example: +123456789. NULL returned on failure
 *
 * @return Returns <code>WMA_OK</code> when successful;
 *	<code>WMA_ERR</code> on error.
 */
WMA_STATUS jsr120_get_smsc_address(/* OUT */jchar **smscAddress, jint* addrStrLen);

/**
 * Sends a Text or Binary message. The calling function does not have to worry about the 
 * details of the underlying communication protocol (such as GSM 3.40, CDMA IS-637, or others).
 * To avoid blocking the Java VM for a long time, this method works asynchronously and 
 * returns before the message is delivered to the network. The target's native platform
 * handles the whole communication session and returns the result via an asynchronous message 
 * or callback function.
 *
 * @param msgType message string type:Text(0) or Binary(1)
 *                The target device should decide the DCS (Data Coding Scheme)
 *                in PDU according to this parameter and the  message contents.
 *                If  the target device is compliant with GSM 3.40, then for a Binary
 *                Message,  the DCS in PDU should be 8-bit binary.
 *                For a	 Text Message, the target device should decide the DCS	according to
 *                the  message contents. When all characters in the message contents are in
 *                the GSM 7-bit alphabet, the DCS should be GSM 7-bit; otherwise, it should
 *                be  UCS-2.
 * @param address the target SMS address for the message. The format of the address
 *                is expected to be compliant with MSIDN, for example, +123456789
 * @param msgBuffer the message body (payload) to be sent
 * @param msgLen the length of the message body
 * @param sourcePort the sender's reply port number
 * @param destPort the receiver's port number
 * @param bytesSent The number of bytes sent is returned in this variable
 * @param pContext pointer where to save context of asynchronous operation.
 *
 * @return Returns <code>WMA_OK</code> when successful;
 *      <code>WMA_NET_WOULDBLOCK</code> if reinvocation is required to
 *      finish the operation;
 *	<code>WMA_ERR</code> on error.
 */
WMA_STATUS jsr120_send_sms(jchar msgType,
		              unsigned char address[],
		              unsigned char msgBuffer[],
		              jchar msgLen,
		              jchar sourcePort,
		              jchar destPort,
                              int handle,
                              /* OUT */void **pContext);

/**
 * The native software platform on the target device calls this API to notify that a message has 
 * been sent. When wma_sendSMSMessage()is called, the native software platform on the target
 * device is expected to deliver the message to the network. It then calls this callback function.   
 *
 * @param handle Handle value returned from platform send function 
 * @param result indication of send completed status result: Either
 *         <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> on failure
 */
void jsr120_notify_sms_send_completed(int handle, WMA_STATUS result);

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
 * @param sourcePortNum	source port number of the incoming message
 * @param destPortNum	The destination port number of the incoming message.[unsigned short]
 * @param timeStamp     When the message is received (SCT: message Service Center Time)[long].
 */
void jsr120_notify_incoming_sms(jchar msgType, char *sourceAddress,
                                unsigned char *msgBuffer, jint msgLen,
                                jchar sourcePortNum, jchar destPortNum,
                                jlong timeStamp);

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
 * @return Returns <code>WMA_OK</code> when successful;
 *	<code>WMA_ERR</code> on error.
 */
WMA_STATUS jsr120_add_sms_listening_port(jchar port);

/**
 * Removes a registered message port number. After removing this port number, no message 
 * whose destination port number is equal to this port number, will be received. If the 
 * specified port number is not registered, then an error code will be returned.
 *
 * @param port The port number to be removed.
 *
 * @return Returns <code>WMA_OK</code> when successful;
 *	<code>WMA_ERR</code> on error.
 */
WMA_STATUS jsr120_remove_sms_listening_port(jchar port);

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
 *                    message is returned here.
 *
 * @return Returns <code>WMA_OK</code> when successful;
 *	<code>WMA_ERR</code> on error.
 */
WMA_STATUS jsr120_number_of_sms_segments(unsigned char msgBuffer[], jint msgLen, jint msgType,
                                            jboolean hasPort, /* OUT */jint *numSegments);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* _JSR120_SMS_PROTOCOL_H_ */
