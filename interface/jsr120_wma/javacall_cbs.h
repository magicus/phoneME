/*
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

#ifndef __JAVACALL_CBS_H
#define __JAVACALL_CBS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file javacall_cbs.h
 * @ingroup WMA
 * @brief Javacall interfaces for CBS
 */
 
#include "javacall_defs.h" 

/**
 * @defgroup MandatoryCBS Mandatory CBS API
 * @ingroup WMA
 * @{
 */

/**
 * check if the CBS service is available, and CBS messages can be received
 *
 * @return <tt>JAVACALL_OK</tt> if CBS service is avaialble 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_cbs_is_service_available(void);


/**
 * @enum javacall_cbs_encoding
 * @brief CBS encoding format
 *
 * These values must match those in 
 * src/protocol/cbs/classes/com/sun/midp/io/j2me/cbs/Protocol.java
 */
typedef enum {
        JAVACALL_CBS_MSG_TYPE_ASCII        = 0,
        JAVACALL_CBS_MSG_TYPE_BINARY       = 1,
        JAVACALL_CBS_MSG_TYPE_UNICODE_UCS2 = 2
} javacall_cbs_encoding;

/**
 * Registers a message ID number. 
 *
 * If this message ID has already been registered either by a native application 
 * or by another WMA application, then the API should return an error code.
 * 
 * @param msgID message ID to start listening to
 * @return <tt>JAVACALL_OK</tt> if started listening to port, or 
 *         <tt>JAVACALL_FAIL</tt> or negative value if unsuccessful
 */
javacall_result javacall_cbs_add_listening_msgID(unsigned short msgID);

/**
 * Unregisters a message ID number. 
 * After unregistering a message ID, CBS messages received by the device for 
 * the specfied UD should not be delivered to the WMA implementation. 
 *
 * @param msgID message ID to stop listening to
 * @return <tt>JAVACALL_OK </tt> if stopped listening to port, 
 *          or <tt>JAVACALL_FAIL</tt> if failed, or port already not registered
 */
javacall_result javacall_cbs_remove_listening_msgID(unsigned short msgID);


/** @} */


/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - -  - - - - - - -  
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurence of the
  undelying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup NotificationCBS Notification API for CBS 
 * @ingroup WMA
 * @{
 */  

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
        int                    msgBufferLen);


/** @} */

#ifdef __cplusplus
}
#endif

#endif 

