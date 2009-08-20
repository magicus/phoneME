/*
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
 * @file javacall_chapi_msg_exchange.h
 * @ingroup CHAPI
 * @brief interprocess communication layer
 */

/**
 * @defgroup CHAPI JSR-211 Content Handler API (CHAPI)
 *
 *  The following API definitions are required by JSR-211.
 * @{
 */

#ifndef __JAVACALL_CHAPI_MSG_EXCHANGE_H
#define __JAVACALL_CHAPI_MSG_EXCHANGE_H

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

#include <stddef.h>
#include <javacall_defs.h>

/*
  The functions below should be implemented only if the JSR is used in REMOTE_AMS configuration (REMOTE_AMS_CFG=true).
*/


/**
 *  This function is called by the local (application) part of the JSR and it must
 * - allocate an unique identifier for this data exchange (not zero)
 * - transmit all parameters to the remote AMS process, where the function javanotify_chapi_process_msg_request must be called
 *
 * @param queueId must be passed as the queueID parameter to the javanotify_chapi_process_msg_request function in the AMS process
 * @param msgCode must be passed as the msg parameter to the javanotify_chapi_process_msg_request function in the AMS process
 * @param bytes must be passed as the bytes parameter to the javanotify_chapi_process_msg_request function in the AMS process
 * @param bytesCount must be passed as the count parameter to the javanotify_chapi_process_msg_request function in the AMS process
 * @param dataExchangeID must be initialized and passed as the dataExchangeID parameter to the javanotify_chapi_process_msg_request 
 * function in the AMS process
 * @return JAVACALL_OK if operation was successful, error code otherwise
 */
javacall_result javacall_chapi_post_message( int queueId, int msgCode, const unsigned char * bytes, size_t bytesCount, 
                                                   int * dataExchangeID );

/**
 *  This function is called by the AMS part of the JSR and it must send all parameters to the caller. The caller must be determined 
 * by the dataExchangeID parameter value. The function javanotify_chapi_process_msg_result must be called in application process.
 *
 * @param dataExchangeID must be passed as the dataExchangeID parameter to the javanotify_chapi_process_msg_result function in the caller process
 * @param bytes must be passed as the bytes parameter to the javanotify_chapi_process_msg_result function in the caller process
 * @param bytesCount must be passed as the count parameter to the javanotify_chapi_process_msg_result function in the caller process
 * @return JAVACALL_OK if operation was successful, error code otherwise
 */
javacall_result javacall_chapi_send_response( int dataExchangeID, const unsigned char * bytes, size_t bytesCount );


/**
 * This function must be called in the AMS part of the JSR when a new request is arrived (transmitted).
 * see javacall_chapi_post_message
 */
javacall_result javanotify_chapi_process_msg_request( int queueID, int dataExchangeID, 
                                                int msg, const unsigned char * bytes, size_t count );

/**
 * This function must be called in the local (application) part of the JSR when a response to some request is arrived.
 * see javacall_chapi_send_response
 */
javacall_result javanotify_chapi_process_msg_result( int dataExchangeID, const unsigned char * bytes, size_t count );

#ifdef __cplusplus
}
#endif/*__cplusplus*/

/** @} */
#endif //__JAVACALL_CHAPI_MSG_EXCHANGE_H
