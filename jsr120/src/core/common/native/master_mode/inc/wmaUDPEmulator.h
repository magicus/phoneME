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

#ifndef WMAUDPEMULATOR_H
#define WMAUDPEMULATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <kni.h>
#include <jsr120_types.h>

/**
 * @file
 *
 */

/**
 * Opens a datagram socket
 *
 * @param protocol  A protocol type.
 * @param port  The local port to attach to.
 *
 * @return WMA_NET_SUCCESS if the function completes successfully;\n
 * WMA_NET_IOERROR if there was an IO error and IOException needs
 * to be thrown;
 */
WMA_STATUS jsr120_datagram_open(WMA_PROTOCOLS protocol, jint port);

/**
 * Closes a platform-specific datagram socket.
 *
 * @param protocol  A protocol type.
 *
 * @return WMA_NET_SUCCESS upon success,\n
 * WMA_NET_WOULDBLOCK if the caller must call the finish function again to
 * complete the operation;\n WMA_NET_IOERROR for an error
 */
WMA_STATUS jsr120_datagram_close(WMA_PROTOCOLS protocol);

/**
 * Read from a platform-specific datagram.
 *
 * <p>
 * <b>NOTE:</b> The parameter <code>buffer</code> must be pre-allocated
 * prior to calling this function.
 *
 * @param protocol  A protocol type.
 * @param pAddress base of byte array to receive the address
 * @param port pointer to the port number of the remote location
 *		that sent the datagram. <code>port</code> is set by
 *		this function.
 * @param buffer data received from the remote location. The contents
 *		 of <code>buffer</code> are set by this function.
 * @param length the length of the buffer
 * @param pBytesRead returns the number of bytes actually read; it is
 *	  set only when this function returns WMA_NET_SUCCESS
 *
 * @return WMA_NET_SUCCESS for successful read operation;\n
 *	 WMA_NET_WOULDBLOCK if the operation would block,\n
 *	 WMA_NET_INTERRUPTED for an Interrupted IO Exception\n
 *	 WMA_NET_IOERROR for all other errors
 */

WMA_STATUS jsr120_datagram_read(WMA_PROTOCOLS protocol, unsigned char *pAddress, jint *port,
                                   char *buffer, jint length, jint *pBytesRead);

/**
 * Determine whether the given phone number is this device's phone number.
 *
 * @param phoneNumber The phone number to be matched again the properties.
 *
 * @return <code>WMA_NET_IOERROR</code> if the device's phone number cannot be
 *     determined. <code>1</code> if there's a match; <code>0</code>,
 *     otherwise.
 */
WMA_STATUS is_device_phone_number(char* phoneNumber);

/**
 * Send an sms message
 *
 * @param msgType type of message, text or binary
 * @param address base of byte array to receive the address
 * @param msgBuffer data to send to the remote location
 * @param msgLen amount of data, in bytes, to send to the remote
 *		 location
 * @param sourcePort port number to receive ack
 * @param destPort port number of the remote location to send the datagram
 * @param bytesWritten returns the number of bytes written after
 *	  successful write operation; only set if this function returns
 *	  WMA_NET_SUCCESS
 * @param pContext return the address of saved asynchronous operation context;
 *        only set if this function returns WMA_NET_WOULDBLOCK
 *        
 *
 * @return WMA_NET_SUCCESS for successful write operation;\n
 *	 WMA_NET_WOULDBLOCK if the operation would block,\n
 *	 WMA_NET_INTERRUPTED for an Interrupted IO Exception\n
 *	 WMA_NET_IOERROR for all other errors
 */
WMA_STATUS jsr120_sms_write(jchar msgType, unsigned char *address, 
                               unsigned char *msgBuffer, 
                               jchar msgLen, 
                               jchar sourcePort, jchar destPort,
                               jint *bytesWritten,
                               void **pContext);

/**
 * Send an MMS message.
 *
 * @param sendingToSelf <code>1</code> if sending the message to this device;
 *     <code>0</code>, otherwise.
 * @param toAddr The recipient's MMS address.
 * @param fromAddr The sender's MMS address.
 * @param appID The application ID string associated with this message.
 * @param replyToAppID The reply-to application ID string associated with this
 *     message.
 * @param msgLen The total length, in bytes, of the MMS message.
 * @param msg A pointer to the MMS message, which contains both the message
 *     header and message body structures.
 * @param bytesWritten Returns the number of bytes written after successful
 *     write operation. This is only set if this function returns
 *     WMA_NET_SUCCESS.
 * @param pContext return the address of saved asynchronous operation context;
 *        only set if this function returns WMA_NET_WOULDBLOCK
 *
 * @return WMA_NET_SUCCESS for successful write operation;\n
 *	 WMA_NET_WOULDBLOCK if the operation would block,\n
 *	 WMA_NET_INTERRUPTED for an Interrupted IO Exception\n
 *	 WMA_NET_IOERROR for all other errors.
 */
WMA_STATUS jsr205_mms_write(jint sendingToSelf, char *toAddr, char* fromAddr, 
                               char* appID, char* replyToAppID, jint msgLen, 
                               char* msg, jint *bytesWritten,
                               void **pContext);

#ifdef __cplusplus
}
#endif

#endif
