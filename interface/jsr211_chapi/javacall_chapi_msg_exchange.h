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


#ifndef __JAVACALL_CHAPI_MSG_EXCHANGE_H
#define __JAVACALL_CHAPI_MSG_EXCHANGE_H

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

#include <stddef.h>
#include <javacall_defs.h>

/*
  The functions below should be implemented only if the JSR is used in REMOTE_AMS configuration.
*/


/**
  This function is called by the local (application) part of the JSR and it must
- allocate an unique identifier for this data exchange (not zero)
- transmit all parameters to the remote AMS process
*/
javacall_result javacall_chapi_post_message( int queueId, int msgCode, const unsigned char * bytes, size_t bytesCount, 
                                                   int * dataExchangeID );

/**
  This function is called by the AMS part of the JSR and it must send all parameters to the caller. The caller must be determined 
by the dataExchangeID parameter value.
*/
javacall_result javacall_chapi_send_response( int dataExchangeID, const unsigned char * bytes, size_t bytesCount );


/**
  This function must be called in the AMS part of the JSR when new request is arrived (transmitted).
*/
javacall_result javanotify_chapi_process_msg_request( int queueID, int dataExchangeID, 
                                                int msg, const unsigned char * bytes, size_t count );

/**
  This function must be called in the local (application) part of the JSR when a response to some request is arrived.
*/
javacall_result javanotify_chapi_process_msg_result( int dataExchangeID, const unsigned char * bytes, size_t count );

#ifdef __cplusplus
}
#endif/*__cplusplus*/

/** @} */
#endif //__JAVACALL_CHAPI_MSG_EXCHANGE_H
