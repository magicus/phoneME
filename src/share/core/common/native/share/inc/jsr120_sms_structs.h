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

#ifndef _JSR120_SMS_STRUCTS_H_
#define _JSR120_SMS_STRUCTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <jsr120_types.h>
#include <kni.h>

/**
 * The maximum length of an SMS message.
 */
#define MAX_MSG_BUFFERSIZE (160)


#define MAX_ADDR_LEN (20)

/**
 * A Simple Message Service message.
 */
typedef struct {

    /* The logical port ID of the sender. */
    jchar sourcePortNum;

    /* The logical port ID of the recipient. */
    jchar destPortNum;

    /* The time at which the message was sent. */
    jlong timeStamp;

    /* GSM 7-bit alphabet, Unicode or 8-bit Binary. */
    /* RFC: Make this ENCODING_TYPE instead of unsigned short? */
    jchar encodingType;

    /* The length of the incoming messsage body. */
    jchar msgLen;

    /* The address to which the message is being sent. */
    char* msgAddr;

    /* The message body. */
    char* msgBuffer;

} _sms_message_struct;

/**
 * The general SMS message data type. 
 */
typedef _sms_message_struct SmsMessage;

#ifdef __cplusplus
}
#endif

#endif /* #ifdef _JSR120_SMS_STRUCTS_H_ */
